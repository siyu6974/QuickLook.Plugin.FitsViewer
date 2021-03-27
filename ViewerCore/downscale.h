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


#ifndef downscale_h
#define downscale_h

#include <valarray>
#include<cmath>

template <typename T>
void downscale_mono(std::valarray<T>& buf, int width, int height, int factor) {
	int newWidth = floor(width / factor);
	int newHeight = floor(height / factor);

	for (int i = 0, iout = 0; iout < newHeight; i += factor, iout++) {
		for (int j = 0, jout = 0; jout < newWidth; j += factor, jout++) {
			buf[iout*newWidth + jout] = buf[i*width + j];
		}
	}
}


template <typename T>
void downscale_color(std::valarray<T>& buf, int width, int height, int factor) {
	int newWidth = floor(width / factor);
	int newHeight = floor(height / factor);
	int newPlaneSize = newHeight * newWidth;
	int planeSize = width * height;

	for (int i = 0, iout = 0; iout < newHeight; i += factor, iout++) {
		for (int j = 0, jout = 0; jout < newWidth; j += factor, jout++) {
			buf[iout*newWidth + jout] = buf[i*width + j];
		}
	}
	for (int i = 0, iout = 0; iout < newHeight; i += factor, iout++) {
		for (int j = 0, jout = 0; jout < newWidth; j += factor, jout++) {
			buf[newPlaneSize + iout * newWidth + jout] = buf[planeSize + i * width + j];
			buf[newPlaneSize * 2 + iout * newWidth + jout] = buf[planeSize * 2 + i * width + j];
		}
	}
}

#endif /* downscale_h */
