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


#ifndef debayer_h
#define debayer_h

#include <CCfits/CCfits>
#include <valarray>


using std::string;
using namespace CCfits;

template <typename T>
void super_pixel_RGGB(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor);

template <typename T>
void super_pixel_BGGR(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor);

template <typename T>
void super_pixel_GBRG(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor);

template <typename T>
void super_pixel_GRBG(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor);


template <typename T>
void super_pixel(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, string pattern, int factor) {
    if (pattern == "RGGB") {
        super_pixel_RGGB(buf, newbuf, width, height, factor);
    }
    else if (pattern == "BGGR") {
        super_pixel_BGGR(buf, newbuf, width, height, factor);
    }
    else if (pattern == "GBRG") {
        super_pixel_GRBG(buf, newbuf, width, height, factor);
    }
    else if (pattern == "GRBG") {
        super_pixel_GRBG(buf, newbuf, width, height, factor);
    }
    else {
        throw 0;
    }
}

// NOTE: Using line/column skipping for downscaling
// pixing binning is too slow to have any performance gain
template <typename T>
void super_pixel_RGGB(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor) {
    int outRowLength = width / (2 * factor);
    int outPlaneSize = outRowLength * (height / (2 * factor));
    for (int row = 0, iout = 0; row < height; row += 2 * factor, iout++) {
        for (int col = 0, jout = 0; col < width; col += 2 * factor, jout++) {
            int idx = iout * outRowLength + jout;
            int cur = row * width + col;
            int right = cur + 1;
            int down = cur + width;
            int down_right = down + 1;

            newbuf[idx] = buf[cur];
            float tmp = buf[right] / 2 + buf[down] / 2;
            newbuf[idx + outPlaneSize] = (T)tmp;
            newbuf[idx + outPlaneSize * 2] = buf[down_right];
        }
    }
}


template <typename T>
void super_pixel_BGGR(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor) {
    int outRowLength = width / (2 * factor);
    int outPlaneSize = outRowLength * (height / (2 * factor));
    for (int row = 0, iout = 0; row < height; row += 2 * factor, iout++) {
        for (int col = 0, jout = 0; col < width; col += 2 * factor, jout++) {
            int idx = iout * outRowLength + jout;
            int cur = row * width + col;
            int right = cur + 1;
            int down = cur + width;
            int down_right = down + 1;

            newbuf[idx] = buf[down_right];
            float tmp = buf[right] / 2 + buf[down] / 2;
            newbuf[idx + outPlaneSize] = (T)tmp;
            newbuf[idx + outPlaneSize * 2] = buf[cur];
        }
    }
}


template <typename T>
void super_pixel_GBRG(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor) {
    int outRowLength = width / (2 * factor);
    int outPlaneSize = outRowLength * (height / (2 * factor));
    for (int row = 0, iout = 0; row < height; row += 2 * factor, iout++) {
        for (int col = 0, jout = 0; col < width; col += 2 * factor, jout++) {
            int idx = iout * outRowLength + jout;
            int cur = row * width + col;
            int right = cur + 1;
            int down = cur + width;
            int down_right = down + 1;

            newbuf[idx] = buf[down];
            float tmp = buf[cur] / 2 + buf[down_right] / 2;
            newbuf[idx + outPlaneSize] = (T)tmp;
            newbuf[idx + outPlaneSize * 2] = buf[right];
        }
    }
}


template <typename T>
void super_pixel_GRBG(const std::valarray<T>& buf, std::valarray<T>& newbuf, int width, int height, int factor) {
    int outRowLength = width / (2 * factor);
    int outPlaneSize = outRowLength * (height / (2 * factor));
    for (int row = 0, iout = 0; row < height; row += 2 * factor, iout++) {
        for (int col = 0, jout = 0; col < width; col += 2 * factor, jout++) {
            int idx = iout * outRowLength + jout;
            int cur = row * width + col;
            int right = cur + 1;
            int down = cur + width;
            int down_right = down + 1;

            newbuf[idx] = buf[right];
            float tmp = buf[cur] / 2 + buf[down_right] / 2;
            newbuf[idx + outPlaneSize] = (T)tmp;
            newbuf[idx + outPlaneSize * 2] = buf[down];
        }
    }
}


std::string flipBayerPatternVertically(const std::string& pattern) {
    if (pattern.length() != 4) {
        std::cerr << "Invalid Bayer pattern length. Pattern must be 4 characters long." << std::endl;
        return "";
    }

    std::string flippedPattern = pattern;
    std::swap(flippedPattern[0], flippedPattern[2]); // Swap R and G2
    std::swap(flippedPattern[1], flippedPattern[3]); // Swap G1 and B

    return flippedPattern;
}

#endif /* debayer_h */
