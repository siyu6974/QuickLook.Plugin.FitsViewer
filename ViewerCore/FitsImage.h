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
} ImageSize;


class FitsImage
{
	std::valarray<float> contents;
	ImageSize size;
	ImageSize finalSize;

	public:
		std::map<string, string> header;

		FitsImage(string path);
		void getImagePix(unsigned char *pixData);
		ImageSize getSize();
		ImageSize getFinalSize();
};

extern "C" {
	__declspec(dllexport) FitsImage *FitsImageCreate(const char * path) {
		string str(path);
		std::cout << "********************" << std::endl;
		std::cout << str << std::endl;
		std::cout << str.length() << std::endl;

		return new FitsImage(str);
	}

	__declspec(dllexport) ImageSize FitsImageSize(FitsImage *fits) {
		return fits->getSize();
	}

	__declspec(dllexport) void FitsImageData(FitsImage *fits, unsigned char *data) {
		return fits->getImagePix(data);
	}

	__declspec(dllexport) const char* FitsImageHeader(FitsImage *fits) {
		string result = "";

		string output = "";

		auto m = fits->header;
		for (auto it = m.begin(); it != m.end(); it++) {
			output += (it->first) + ":" + (it->second)+", ";
		}

		//result = output.substr(0, output.size() - 2);
		const char *r = output.c_str();
		return r;
	}

	__declspec(dllexport) ImageSize FitsImageBufferSize(FitsImage *fits) {		auto size = fits->getSize();
		return fits->getFinalSize();
	}
}