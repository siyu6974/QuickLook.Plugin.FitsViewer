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

#pragma once
#include <valarray>
#include <string>
#include <iostream>
#include <CCfits/CCfits>

using namespace CCfits;
using std::string;


__declspec(dllexport) typedef struct {
	int nx;
	int ny;
	int nc;
	int depth;
} ImageDim;


class FitsImage
{
	ImageDim _imgDim;
	ImageDim _outDim;
	std::unique_ptr<FITS> pInfile;

public:
	std::map<string, string> header;

	FitsImage(string path);
	void getImagePix(unsigned char *pixData);
	ImageDim getDim();
	ImageDim getFinalDim();
};

extern "C" {
	__declspec(dllexport) FitsImage *FitsImageCreate(const char * path) {
		string str(path);
		return new FitsImage(str);
	}

	__declspec(dllexport) ImageDim FitsImageGetDim(FitsImage *fits) {
		return fits->getDim();
	}

	__declspec(dllexport) void FitsImageGetPixData(FitsImage *fits, unsigned char *data) {
		return fits->getImagePix(data);
	}

	__declspec(dllexport) int FitsImageGetHeader(FitsImage *fits, char *buffer) {
		string output = "";

		auto m = fits->header;
		for (auto it = m.begin(); it != m.end(); it++) {
			output += (it->first) + ":" + (it->second) + "; ";
		}

		if (buffer != nullptr)
			strcpy_s(buffer, output.size() + 1, output.c_str());

		return output.size();
	}

	__declspec(dllexport) ImageDim FitsImageGetOutputDim(FitsImage *fits) {
		auto size = fits->getDim();
		return fits->getFinalDim();
	}

	__declspec(dllexport) void FitsImageDestroy(FitsImage *fits) {
		delete fits;
	}
}