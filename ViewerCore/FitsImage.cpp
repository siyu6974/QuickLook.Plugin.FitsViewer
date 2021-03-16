#include "pch.h"
#include "FitsImage.h"
#include <ppl.h>
using namespace concurrency;

std::map<string, string> readImageHeader(PHDU& image);
void super_pixel(const std::valarray<float> buf, std::valarray<float>& newbuf, int width, int height, string pattern);
inline const char * const BoolToString(bool b) {
	return b ? "Yes" : "No";
}

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
	auto r = contents;

	if (size.nc == 2 && bayer.empty()) {
		// MONO
		// TODO: test
		// TODO: deal with 65536, need to figure out data type
		//StretchParams1Channel params;
		//computeParamsOneChannel(contents, 0, &params, 65536,
		//	size.ny, size.nx);

		//stretchOneChannel(contents, 0, params, 65536,
		//	size.ny, size.nx);

		//bitmapRep = [self get2DImage : contents];
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
			r = rgbContent;
		}
	}

	int nbPixPerPlane = finalSize.nx*finalSize.ny;
	for (int i = 0; i < finalSize.ny; i++) {
		for (int j = 0; j < finalSize.nx; j++) {
			pixData[(i* finalSize.nx + j)*3] = r[i* finalSize.nx + j] / 256;
			pixData[(i* finalSize.nx + j)*3+1] = r[i* finalSize.nx + j+ nbPixPerPlane] / 256;
			pixData[(i* finalSize.nx + j)*3+2] = r[i* finalSize.nx + j+ nbPixPerPlane*2] / 256;

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
