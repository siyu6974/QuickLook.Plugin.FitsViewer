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

	_imgMeta.nx = static_cast<int>(image.axis(0));
	_imgMeta.ny = static_cast<int>(image.axis(1));
	_imgMeta.nc = static_cast<int>(image.axes() == 3 ? 3:1);
	_imgMeta.depth = static_cast<int>(image.bitpix());

	string bayer = header["BAYERPAT"];
	if (_imgMeta.nc == 1 && bayer.empty()) {
		// mono
		_outImgMeta = { _imgMeta.nx, _imgMeta.ny, 1, 8 };
	}
	else {
		if (_imgMeta.nc == 3) {
			// 3ch image
			_outImgMeta = { _imgMeta.nx, _imgMeta.ny, 3, 8 };
		}
		else if (_imgMeta.nc == 1 && !bayer.empty()) {
			// bayer image
			_outImgMeta = { _imgMeta.nx / 2, _imgMeta.ny / 2, 3, 8 };
		}
	}
}


template <typename T>
void process(std::valarray<T>& content, const ImageMeta& inmeta, const ImageMeta& outmeta, string bayer, int df) {
	if (inmeta.nc == 1) {
		if (df > 1 && bayer.empty()) {
			// mono
			downscale_mono(content, inmeta.nx, inmeta.ny, df);
		}
		else if (!bayer.empty()) {
			// bayered 
			int nbFinalPix = inmeta.nx * inmeta.ny * 3 / 4;
			std::valarray<T> debayered = std::valarray<T>(nbFinalPix);
			super_pixel(content, debayered, inmeta.nx, inmeta.ny, bayer, df);
			content = debayered;
		}
	}
	else if (df > 1 && inmeta.nc == 3) {
		// 3 ch color
		downscale_color(content, inmeta.nx, inmeta.ny, df);
	}

	StretchParams stretchParams;
	computeParamsAllChannels(content, &stretchParams, outmeta);
	stretchAllChannels(content, stretchParams, outmeta);
}


template <typename T>
void setBitmap(const std::valarray<T>& contents, const ImageMeta& outmeta, unsigned char *pixData) {
	auto& size = outmeta;
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
		process(contents, _imgMeta, _outImgMeta, bayer, downscale_factor);
		setBitmap(contents, _outImgMeta, pixData);
	}
	else {
		std::valarray<float> contents;
		image.read(contents);
		process(contents, _imgMeta, _outImgMeta, bayer, downscale_factor);
		setBitmap(contents, _outImgMeta, pixData);
	}
}


ImageMeta FitsImage::getSize()
{
	return _imgMeta;
}


ImageMeta FitsImage::getFinalSize()
{
	return _outImgMeta;
}

