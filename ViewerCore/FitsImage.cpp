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
#include "Stretch.h"
#include "debayer.h"
#include "downscale.h"



inline const char * const BoolToString(bool b) {
	return b ? "Yes" : "No";
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


FitsImage::FitsImage(string path)
{
	pInfile = std::unique_ptr<FITS>(new FITS(path, Read, true));
	PHDU& image = pInfile->pHDU();
	header = readImageHeader(image);

	_imgDim.nx = static_cast<int>(image.axis(0));
	_imgDim.ny = static_cast<int>(image.axis(1));
	_imgDim.nc = static_cast<int>(image.axes() == 3 ? 3 : 1);
	_imgDim.depth = static_cast<int>(image.bitpix());

	string bayer = header["BAYERPAT"];
	if (_imgDim.nc == 1 && bayer.empty()) {
		// mono
		_outDim = { _imgDim.nx, _imgDim.ny, 1, 8 };
	}
	else {
		if (_imgDim.nc == 3) {
			// 3ch image
			_outDim = { _imgDim.nx, _imgDim.ny, 3, 8 };
		}
		else if (_imgDim.nc == 1 && !bayer.empty()) {
			// bayer image
			_outDim = { _imgDim.nx / 2, _imgDim.ny / 2, 3, 8 };
		}
	}
}


template <typename T>
void process(std::valarray<T>& content, const ImageDim& inDim, const ImageDim& outDim, string bayer, int df) {
	if (inDim.nc == 1) {
		if (df > 1 && bayer.empty()) {
			// mono
			downscale_mono(content, inDim.nx, inDim.ny, df);
		}
		else if (!bayer.empty()) {
			// bayered 
			int nbFinalPix = inDim.nx * inDim.ny * 3 / 4;
			std::valarray<T> debayered = std::valarray<T>(nbFinalPix);
			super_pixel(content, debayered, inDim.nx, inDim.ny, bayer, df);
			content = debayered;
		}
	}
	else if (df > 1 && inDim.nc == 3) {
		// 3 ch color
		downscale_color(content, inDim.nx, inDim.ny, df);
	}

	StretchParams stretchParams;
	computeParamsAllChannels(content, &stretchParams, outDim);
	stretchAllChannels(content, stretchParams, outDim);
}


template <typename T>
void setBitmap(const std::valarray<T>& contents, const ImageDim& outDim, unsigned char *pixData) {
	auto& size = outDim;
	if (size.nc == 1) {
		for (int i = 0; i < size.ny*size.nx; i++) {
			pixData[i] = contents[i];
		}
	}
	else {
		int nbPixPerPlane = size.nx*size.ny;

		for (int i = 0; i < size.ny; i++) {
			for (int j = 0; j < size.nx; j++) {
				pixData[(i* size.nx + j) * 3] = contents[i* size.nx + j];
				pixData[(i* size.nx + j) * 3 + 1] = contents[i* size.nx + j + nbPixPerPlane];
				pixData[(i* size.nx + j) * 3 + 2] = contents[i* size.nx + j + nbPixPerPlane * 2];
			}
		}
	}
}


void FitsImage::getImagePix(unsigned char * pixData)
{
	PHDU& image = pInfile->pHDU();

	const int downscale_factor = 1;
	string bayer = header["BAYERPAT"];
	auto bitpix = image.bitpix();

	if (bitpix == Ishort) {
		std::valarray<unsigned short> contents;
		image.read(contents);
		process(contents, _imgDim, _outDim, bayer, downscale_factor);
		setBitmap(contents, _outDim, pixData);
	}
	else {
		std::valarray<float> contents;
		image.read(contents);
		process(contents, _imgDim, _outDim, bayer, downscale_factor);
		setBitmap(contents, _outDim, pixData);
	}
}


ImageDim FitsImage::getDim()
{
	return _imgDim;
}


ImageDim FitsImage::getFinalDim()
{
	return _outDim;
}

