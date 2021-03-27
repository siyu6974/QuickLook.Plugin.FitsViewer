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

#include "pch.h"
#include "FitsImage.h"
#include <ppl.h>
using namespace concurrency;


inline const char * const BoolToString(bool b) {
	return b ? "Yes" : "No";
}

struct StretchParams1Channel
{
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
	}
};

struct StretchParams
{
	StretchParams1Channel grey_red, green, blue;
};


std::map<string, string> readImageHeader(PHDU& image);
void super_pixel(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height, string pattern);

template <typename T>
void stretchOneChannel(std::valarray<T>& buffer, int offset,
	const StretchParams1Channel& stretch_params,
	int input_range, int image_height, int image_width);

template <typename T>
void computeParamsOneChannel(std::valarray<T>& buffer, int offset, StretchParams1Channel *params,
	int inputRange, int height, int width);

FitsImage::FitsImage(string path)
{
	std::unique_ptr<FITS> pInfile(new FITS(path, Read, true));
	PHDU& image = pInfile->pHDU();
	header = readImageHeader(image);

	size.nx = static_cast<int>(image.axis(0));
	size.ny = static_cast<int>(image.axis(1));
	size.nc = static_cast<int>(image.axes());

	string bayer = header["BAYERPAT"];
	if (size.nc == 2 && bayer.empty()) {
		finalSize = { size.nx, size.ny, 1 };
	}
	else {
		if (size.nc == 3) {
			finalSize = { size.nx, size.ny, 3 };
		}
		else if (size.nc == 2 && !bayer.empty()) {
			finalSize = { size.nx / 2, size.ny / 2, 3 };
		}
	}

	image.read(contents);
}

std::map<string, string> readImageHeader(PHDU& image) {
	image.readAllKeys();
	auto header = image.keyWord();

	std::map<string, string> ret;
	for (auto it = header.begin(); it != header.end(); it++) {
		string k = it->first;
		Keyword *kw = it->second;
		// std::cout << k << "type " << kw->keytype() << " " << std::endl;
		if (kw->keytype() == CCfits::Tlogical) {
			bool v;
			kw->value(v);
			ret[k] = BoolToString(v);
		}
		else {
			string v;
			kw->value(v);
			ret[k] = v;
		}
	}
	return ret;
}


void FitsImage::getImagePix(unsigned char * pixData)
{
	string bayer = header["BAYERPAT"];

	if (size.nc == 2 && bayer.empty()) {
		// MONO
		StretchParams1Channel params;
		computeParamsOneChannel(contents, 0, &params, 65536, size.ny, size.nx);
		stretchOneChannel(contents, 0, params, 65536, size.ny, size.nx);
		for (int i = 0; i < size.ny*size.nx; i++) {
			pixData[i] = contents[i];
		}
	}
	else {
		// COLOR
		std::valarray<float> rgbContent;
		if (size.nc == 3) {
			rgbContent = contents;
		}
		else if (size.nc == 2 && !bayer.empty()) {
			rgbContent = std::valarray<float>(finalSize.nx*finalSize.ny*finalSize.nc);
			super_pixel(contents, rgbContent, size.nx, size.ny, bayer);
		}

		int nbPixPerPlane = finalSize.nx*finalSize.ny;

		parallel_for(size_t(0), size_t(3), [&](size_t p) {
			StretchParams stretch_params;
			computeParamsOneChannel(rgbContent, nbPixPerPlane*p, &stretch_params.grey_red, 65536,
				finalSize.ny, finalSize.nx);
			stretchOneChannel(rgbContent, nbPixPerPlane*p, stretch_params.grey_red, 65536,
				finalSize.ny, finalSize.nx);
		});

		for (int i = 0; i < finalSize.ny; i++) {
			for (int j = 0; j < finalSize.nx; j++) {
				pixData[(i* finalSize.nx + j) * 3] = rgbContent[i* finalSize.nx + j];
				pixData[(i* finalSize.nx + j) * 3 + 1] = rgbContent[i* finalSize.nx + j + nbPixPerPlane];
				pixData[(i* finalSize.nx + j) * 3 + 2] = rgbContent[i* finalSize.nx + j + nbPixPerPlane * 2];
			}
		}
	}	
}


ImageSize FitsImage::getSize()
{
	return size;
}


ImageSize FitsImage::getFinalSize()
{
	return finalSize;
}


void super_pixel_RGGB(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height);
void super_pixel_BGGR(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height);
void super_pixel_GBRG(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height);
void super_pixel_GRBG(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height);


void super_pixel(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height, string pattern) {
	if (pattern == "RGGB") {
		super_pixel_RGGB(buf, newbuf, width, height);
	}
	else if (pattern == "BGGR") {
		super_pixel_BGGR(buf, newbuf, width, height);
	}
	else if (pattern == "GBRG") {
		super_pixel_GRBG(buf, newbuf, width, height);
	}
	else if (pattern == "GRBG") {
		super_pixel_GRBG(buf, newbuf, width, height);
	}
	else {
		throw 0;
	}
}


void super_pixel_RGGB(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height) {
	parallel_for(size_t(0), size_t(3), [&](size_t ch) {
		for (int row = 0, iout = 0; iout < height / 2; row += 2, iout++) {
			for (int col = 0, jout = 0; jout < width / 2; col += 2, jout++) {
				int idx = ch * width * height / 4 + iout * width / 2 + jout;
				int cur = row * width + col;
				int right = cur + 1;
				int down = cur + width;
				int down_right = down + 1;

				switch (ch) {
				case 0: {
					newbuf[idx] = buf[cur];
					break;
				}
				case 1: {
					float tmp = buf[right] + buf[down];
					tmp /= 2;
					newbuf[idx] = tmp;
					break;
				}
				case 2: {
					newbuf[idx] = buf[down_right];
					break;
				}
				}
			}
		}
	});
}


void super_pixel_BGGR(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height) {
	parallel_for(size_t(0), size_t(3), [&](size_t ch) {
		for (int row = 0, iout = 0; iout < height / 2; row += 2, iout++) {
			for (int col = 0, jout = 0; jout < width / 2; col += 2, jout++) {
				int idx = ch * width * height / 4 + iout * width / 2 + jout;
				int cur = row * width + col;
				int right = cur + 1;
				int down = cur + width;
				int down_right = down + 1;

				switch (ch) {
				case 0: {
					newbuf[idx] = buf[down_right];
					break;
				}
				case 1: {
					float tmp = buf[right] + buf[down];
					tmp /= 2;
					newbuf[idx] = tmp;
					break;
				}
				case 2: {
					newbuf[idx] = buf[cur];
					break;
				}
				}
			}
		}
	});
}


void super_pixel_GBRG(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height) {
	parallel_for(size_t(0), size_t(3), [&](size_t ch) {
		for (int row = 0, iout = 0; iout < height / 2; row += 2, iout++) {
			for (int col = 0, jout = 0; jout < width / 2; col += 2, jout++) {
				int idx = ch * width * height / 4 + iout * width / 2 + jout;
				int cur = row * width + col;
				int right = cur + 1;
				int down = cur + width;
				int down_right = down + 1;

				switch (ch) {
				case 0: {
					newbuf[idx] = buf[down];
					break;
				}
				case 1: {
					float tmp = buf[cur] + buf[down_right];
					tmp /= 2;
					newbuf[idx] = tmp;
					break;
				}
				case 2: {
					newbuf[idx] = buf[right];
					break;
				}
				}
			}
		}
	});
}


void super_pixel_GRBG(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height) {
	parallel_for(size_t(0), size_t(3), [&](size_t ch) {
		for (int row = 0, iout = 0; iout < height / 2; row += 2, iout++) {
			for (int col = 0, jout = 0; jout < width / 2; col += 2, jout++) {
				int idx = ch * width * height / 4 + iout * width / 2 + jout;
				int cur = row * width + col;
				int right = cur + 1;
				int down = cur + width;
				int down_right = down + 1;

				switch (ch) {
				case 0: {
					newbuf[idx] = buf[right];
					break;
				}
				case 1: {
					float tmp = buf[cur] + buf[down_right];
					tmp /= 2;
					newbuf[idx] = tmp;
					break;
				}
				case 2: {
					newbuf[idx] = buf[down];
					break;
				}
				}
			}
		}
	});
}


template <typename T>
T median(std::valarray<T>& values) {
	const int middle = (int)(values.size() / 2);
	std::nth_element(std::begin(values), std::begin(values) + middle, std::end(values));
	return values[middle];
}


template <typename T>
T median(std::valarray<T>& values, int offset, int size, int sampleBy) {
	const int downsampledfinalSize = size / sampleBy;
std:valarray<T> samples = values[std::slice(offset, downsampledfinalSize, sampleBy)];
	return median(samples);
}


// Same as Kstars'
// See section 8.5.7 in above link  https://pixinsight.com/doc/docs/XISF-1.0-spec/XISF-1.0-spec.html
template <typename T>
void computeParamsOneChannel(std::valarray<T>& buffer, int offset, StretchParams1Channel *params,
	int inputRange, int height, int width) {
	// Find the median sample.
	constexpr int maxSamples = 500000;
	const int sampleBy = width * height < maxSamples ? 1 : width * height / maxSamples;
	T medianSample = median(buffer, offset, width * height, sampleBy);
	// Find the Median deviation: 1.4826 * median of abs(sample[i] - median).
	const int numSamples = width * height / sampleBy;
	std::valarray<T> deviations(numSamples);
	for (int index = offset, i = 0; i < numSamples; ++i, index += sampleBy) {
		if (medianSample > buffer[index])
			deviations[i] = medianSample - buffer[index];
		else
			deviations[i] = buffer[index] - medianSample;
	}
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
	int input_range, int image_height, int image_width) {
	// We're outputting uint8, so the max output is 255.
	constexpr int maxOutput = 255;

	// Maximum possible input value (e.g. 1024*64 - 1 for a 16 bit unsigned int).
	const float maxInput = input_range > 1 ? input_range - 1 : input_range;

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

	// Increment the input index by the sampling, the output index increments by 1.
	for (int j = 0; j < image_height; j++) {
		for (int i = 0; i < image_width; i++) {
			int idx = j * image_width + i + offset;
			const T input = buffer[idx];
			if (input < nativeShadows)
				buffer[idx] = 0;
			else if (input >= nativeHighlights)
				buffer[idx] = maxOutput;
			else {
				const T inputFloored = (input - nativeShadows);
				buffer[idx] = (inputFloored * k1) / (inputFloored * k2 - midtones);
			}
		}
	}
}