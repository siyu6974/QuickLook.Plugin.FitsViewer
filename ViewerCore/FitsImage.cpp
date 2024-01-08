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
#include "log.h"


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


FitsImage::FitsImage(string path) : _inDim{}, _outDim{}
{
	writeToLogFile(path.c_str());
	writeToLogFile("FitsImage constructor");

	try
    {
		pInfile = std::unique_ptr<FITS>(new FITS(path, Read, true));
    }
    catch (std::exception& e)
    {
		writeToLogFile(e.what());
    }
	if (!pInfile) {
		writeToLogFile("CCFits failed");
		return;
	}
	PHDU& imageHDU = pInfile->pHDU();

	if (imageHDU.axes() == 0) {
		writeToLogFile("Image HDU has 0 axes");
		return;
	}
	header = readImageHeader(imageHDU);

	_inDim.nx = static_cast<int>(imageHDU.axis(0));
	_inDim.ny = static_cast<int>(imageHDU.axis(1));
	_inDim.nc = static_cast<int>(imageHDU.axes() == 3 ? 3 : 1);
	_inDim.depth = static_cast<int>(imageHDU.bitpix());

	string bayer;
	auto it = header.find("BAYERPAT");
	if (it != header.end()) {
		// has bayer kw in the header
		bayer = it->second;
		if (!(bayer.compare("RGGB") == 0 || bayer.compare("BGGR") == 0 || bayer.compare("GRBG") == 0 || bayer.compare("GBRG") == 0)) {
			bayer = "";
		}
	}
	_sanitizedBayerMode = bayer;

	if (_inDim.nc == 1 && bayer.empty()) {
		// mono
		writeToLogFile("Mono");
		_outDim = { _inDim.nx, _inDim.ny, 1, 8 };
	}
	else {
		if (_inDim.nc == 3) {
			// 3ch image
			writeToLogFile("3Ch");
			_outDim = { _inDim.nx, _inDim.ny, 3, 8 };
		}
		else if (_inDim.nc == 1 && !bayer.empty()) {
			// bayer image
			writeToLogFile("Bayer");
			_outDim = { _inDim.nx / 2, _inDim.ny / 2, 3, 8 };
		}
	}
	writeToLogFile("FitsImage constructor finish");
}


template <typename T>
void process(std::valarray<T>& content, const ImageDim& inDim, const ImageDim& outDim, string bayer, int df) {
	writeToLogFile("Process start");

	if (inDim.nc == 1) {
		if (df > 1 && bayer.empty()) {
			writeToLogFile("downscale start");

			// mono
			downscale_mono(content, inDim.nx, inDim.ny, df);
		}
		else if (!bayer.empty()) {
			writeToLogFile("debayer start " +  bayer);

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
	writeToLogFile("Downscale and or debayer finish. Stretch start");

	StretchParams stretchParams;
	computeParamsAllChannels(content, &stretchParams, inDim.depth, outDim);
	stretchAllChannels(content, stretchParams, outDim);
	writeToLogFile("Process finish");
}


template <typename T>
void setBitmap(const std::valarray<T>& contents, const ImageDim& outDim, unsigned char *pixData) {
	writeToLogFile("setBitMap start");
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
	writeToLogFile("setBitMap finish");
}


void FitsImage::getImagePix(unsigned char * pixData)
{
	PHDU& image = pInfile->pHDU();

	const int downscale_factor = 1;
	string bayer = _sanitizedBayerMode;
	auto bitpix = image.bitpix();

	if (bitpix == Ishort) {
		std::valarray<unsigned short> contents;
		image.read(contents);
		process(contents, _inDim, _outDim, bayer, downscale_factor);
		setBitmap(contents, _outDim, pixData);
	}
	else {
		std::valarray<float> contents;
		image.read(contents);
		process(contents, _inDim, _outDim, bayer, downscale_factor);
		setBitmap(contents, _outDim, pixData);
	}
}


ImageDim FitsImage::getDim()
{
	return _inDim;
}


ImageDim FitsImage::getFinalDim()
{
	return _outDim;
}

