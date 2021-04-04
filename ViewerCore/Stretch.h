/*
	QuickFits - FITS file preview plugin for QL-win
	Copyright (C) 2021 Siyu Zhang

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
	USA
*/

// This file takes inspiration for KStars' auto stretch implementation

#ifndef Stretch_h
#define Stretch_h

#include "FitsImage.h"
#include <ppl.h>

using namespace concurrency;


struct StretchParams1Channel
{
	int max_input;

	// Stretch algorithm parameters
	float shadows;;
	float highlights;
	float midtones;
	// The extension parameters are not yet used.
	float shadows_expansion;
	float highlights_expansion;

	// The default parameters result in no stretch at all.
	StretchParams1Channel()
	{
		shadows = 0.0;
		highlights = 1.0;
		midtones = 0.5;
		shadows_expansion = 0.0;
		highlights_expansion = 1.0;
		max_input = 65536;
	}
};

struct StretchParams
{
	StretchParams1Channel grey_red, green, blue;
};


template <typename T>
T median(std::valarray<T>& values) {
	const int middle = (int)(values.size() / 2);
	std::nth_element(std::begin(values), std::begin(values) + middle, std::end(values));
	return values[middle];
}


// See section 8.5.7 in above link  https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html
template <typename T>
void computeParamsOneChannel(std::valarray<T>& buffer, int offset, StretchParams1Channel *params, int height, int width) {
	// Find the median sample.
	constexpr int maxSamples = 500000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::valarray<T> samples = buffer[std::slice(offset, numSamples, sampleBy)];
	T medianSample = median(samples);

	auto& deviations = samples;

	T maxSample = 0;
	// Can't use abs because of unsigned value substraction
	for (int i = 0; i < numSamples; i++) {
		if (medianSample > samples[i])
			deviations[i] = medianSample - samples[i];
		else {
			deviations[i] = samples[i] - medianSample;
			if (maxSample < samples[i]) {
				maxSample = samples[i];
			}
		}
	}

	int inputRange = 1;
	if (maxSample > 256) {
		inputRange = 65536;
	}
	else if (maxSample > 1) {
		inputRange = 256;
	}
	params->max_input = inputRange;

	// Shift everything to 0 -> 1.0.
	const float medDev = median(deviations);
	const float normalizedMedian = medianSample / static_cast<float>(inputRange);
	const float MADN = 1.4826 * medDev / static_cast<float>(inputRange);
	const bool upperHalf = normalizedMedian > 0.5;
	const float shadows = (upperHalf || MADN == 0) ? 0.0 :
		fmin(1.0, fmax(0.0, (normalizedMedian + -2.8 * MADN)));
	const float highlights = (!upperHalf || MADN == 0) ? 1.0 :
		fmin(1.0, fmax(0.0, (normalizedMedian - -2.8 * MADN)));
	float X, M;
	constexpr float B = 0.25;
	if (!upperHalf) {
		X = normalizedMedian - shadows;
		M = B;
	}
	else {
		X = B;
		M = highlights - normalizedMedian;
	}
	float midtones;
	if (X == 0) midtones = 0.0f;
	else if (X == M) midtones = 0.5f;
	else if (X == 1) midtones = 1.0f;
	else midtones = ((M - 1) * X) / ((2 * M - 1) * X - M);
	// Store the params.
	params->shadows = shadows;
	params->highlights = highlights;
	params->midtones = midtones;
	params->shadows_expansion = 0.0;
	params->highlights_expansion = 1.0;
}


template <typename T>
void stretchOneChannel(std::valarray<T>& buffer, int offset,
	const StretchParams1Channel& stretch_params,
	int image_height, int image_width) {
	// We're outputting uint8, so the max output is 255.
	constexpr int maxOutput = 255;

	// Maximum possible input value (e.g. 1024*64 - 1 for a 16 bit unsigned int).
	int maxInput = stretch_params.max_input;
	if (maxInput > 1) {
		maxInput--;
	}

	const float midtones = stretch_params.midtones;
	const float highlights = stretch_params.highlights;
	const float shadows = stretch_params.shadows;

	// Precomputed expressions moved out of the loop.
	// hightlights - shadows, protecting for divide-by-0, in a 0->1.0 scale.
	const float hsRangeFactor = highlights == shadows ? 1.0f : 1.0f / (highlights - shadows);
	// Shadow and highlight values translated to the ADU scale.
	const T nativeShadows = shadows * maxInput;
	const T nativeHighlights = highlights * maxInput;
	// Constants based on above needed for the stretch calculations.
	const float k1 = (midtones - 1) * hsRangeFactor * maxOutput / maxInput;
	const float k2 = ((2 * midtones) - 1) * hsRangeFactor / maxInput;


	for (int i = offset; i < offset + image_width * image_height; i++) {
		const T input = buffer[i];
		if (input < nativeShadows)
			buffer[i] = 0;
		else if (input >= nativeHighlights)
			buffer[i] = maxOutput;
		else {
			const T inputFloored = (input - nativeShadows);
			buffer[i] = (inputFloored * k1) / (inputFloored * k2 - midtones);
		}
	}

}


template <typename T>
void computeParamsAllChannels(std::valarray<T>& buffer, StretchParams *params,
	const ImageDim& size) {
	int nbPixPerPlane = size.nx * size.ny;
	parallel_for(size_t(0), size_t(size.nc), [&](size_t ch) {
		StretchParams1Channel *channelParam;
		switch (ch) {
		case 1:
			channelParam = &params->green;
			break;
		case 2:
			channelParam = &params->blue;
			break;
		default:
			channelParam = &params->grey_red;
			break;
		}
		computeParamsOneChannel(buffer, nbPixPerPlane*ch, channelParam,
			size.ny, size.nx);
	});
}


template <typename T>
void stretchAllChannels(std::valarray<T>& buffer,
	const StretchParams& params,
	const ImageDim& size) {
	int nbPixPerPlane = size.nx * size.ny;

	parallel_for(size_t(0), size_t(size.nc), [&](size_t ch) {
		StretchParams1Channel channelParam;
		switch (ch) {
		case 1:
			channelParam = params.green;
			break;
		case 2:
			channelParam = params.blue;
			break;
		default:
			channelParam = params.grey_red;
			break;
		}
		stretchOneChannel(buffer, nbPixPerPlane*ch, channelParam,
			size.ny, size.nx);
	});
}

#endif /* Stretch_h */
