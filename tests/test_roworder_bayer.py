"""
Test script validating the ROWORDER + BAYERPAT fix.

Mirrors the C++ super_pixel_* logic from debayer.h exactly, then:
1. Generates synthetic FITS files for every combination of
   ROWORDER (TOP-DOWN / BOTTOM-UP) x BAYERPAT (RGGB / BGGR / GRBG / GBRG)
2. Applies the FIXED debayer logic (no Bayer-flip for BOTTOM-UP)
3. Asserts that both ROWORDER variants produce R≈1000, G≈500, B≈100
4. Applies the BUGGY logic to show the pre-fix regressions

Key convention (Siril / FITS standard):
  BAYERPAT always describes the pattern at file row 0 (the first stored pixel).
  For BOTTOM-UP files, file row 0 is the visual bottom of the image.
  For TOP-DOWN files, file row 0 is the visual top.
  Either way, super_pixel_* reads from file row 0 → BAYERPAT applies as-is.
  ROWORDER only controls whether the output bitmap must be flipped.

Layout of the 4×4 test frame for RGGB (pattern at row 0):
    Row 0: R G R G
    Row 1: G B G B
    Row 2: R G R G
    Row 3: G B G B

R=1000, G=500, B=100
"""

import numpy as np
from astropy.io import fits
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

R_VAL = 1000
G_VAL = 500
B_VAL = 100


# ── Bayer frame generator ─────────────────────────────────────────────────────

def make_bayer_frame(pattern, width=4, height=4):
    """
    Build a 2-D array where BAYERPAT applies starting at [0, 0]
    (regardless of TOP-DOWN vs BOTTOM-UP — both conventions place
    BAYERPAT at the first stored pixel).
    """
    frame = np.zeros((height, width), dtype=np.uint16)
    color_map = {'R': R_VAL, 'G': G_VAL, 'B': B_VAL}
    for row in range(height):
        for col in range(width):
            ch = pattern[(row % 2) * 2 + (col % 2)]
            frame[row, col] = color_map[ch]
    return frame


# ── FITS file writer ──────────────────────────────────────────────────────────

def write_fits(path, frame, bayerpat, roworder):
    """
    Write a uint16 FITS file.  The frame data is stored as-is (BAYERPAT
    at row 0).  ROWORDER is written as a header keyword; it does NOT
    change how the pixel data is laid out in the file.
    """
    hdu = fits.PrimaryHDU(frame.astype(np.uint16))
    hdu.header['BAYERPAT'] = bayerpat
    hdu.header['ROWORDER'] = roworder
    hdu.writeto(path, overwrite=True)


# ── C++ super_pixel_* ports (direct translation of debayer.h) ─────────────────

def super_pixel_RGGB(buf, width, height):
    out_w, out_h = width // 2, height // 2
    plane = out_w * out_h
    out = np.zeros(plane * 3, dtype=np.float32)
    iout = 0
    for row in range(0, height - 1, 2):
        for jout, col in enumerate(range(0, width - 1, 2)):
            idx = iout * out_w + jout
            cur = row * width + col
            out[idx]           = buf[cur]
            out[idx + plane]   = (buf[cur + 1] + buf[cur + width]) / 2
            out[idx + plane*2] = buf[cur + width + 1]
        iout += 1
    return out


def super_pixel_BGGR(buf, width, height):
    out_w, out_h = width // 2, height // 2
    plane = out_w * out_h
    out = np.zeros(plane * 3, dtype=np.float32)
    iout = 0
    for row in range(0, height - 1, 2):
        for jout, col in enumerate(range(0, width - 1, 2)):
            idx = iout * out_w + jout
            cur = row * width + col
            out[idx]           = buf[cur + width + 1]
            out[idx + plane]   = (buf[cur + 1] + buf[cur + width]) / 2
            out[idx + plane*2] = buf[cur]
        iout += 1
    return out


def super_pixel_GRBG(buf, width, height):
    out_w, out_h = width // 2, height // 2
    plane = out_w * out_h
    out = np.zeros(plane * 3, dtype=np.float32)
    iout = 0
    for row in range(0, height - 1, 2):
        for jout, col in enumerate(range(0, width - 1, 2)):
            idx = iout * out_w + jout
            cur = row * width + col
            out[idx]           = buf[cur + 1]
            out[idx + plane]   = (buf[cur] + buf[cur + width + 1]) / 2
            out[idx + plane*2] = buf[cur + width]
        iout += 1
    return out


def super_pixel_GBRG(buf, width, height):
    out_w, out_h = width // 2, height // 2
    plane = out_w * out_h
    out = np.zeros(plane * 3, dtype=np.float32)
    iout = 0
    for row in range(0, height - 1, 2):
        for jout, col in enumerate(range(0, width - 1, 2)):
            idx = iout * out_w + jout
            cur = row * width + col
            out[idx]           = buf[cur + width]
            out[idx + plane]   = (buf[cur] + buf[cur + width + 1]) / 2
            out[idx + plane*2] = buf[cur + 1]
        iout += 1
    return out


def super_pixel(buf, width, height, pattern):
    dispatch = {
        'RGGB': super_pixel_RGGB,
        'BGGR': super_pixel_BGGR,
        'GRBG': super_pixel_GRBG,
        'GBRG': super_pixel_GBRG,
    }
    if pattern not in dispatch:
        raise ValueError(f"Unknown pattern: {pattern}")
    return dispatch[pattern](buf, width, height)


# ── Debayer pipeline helpers ──────────────────────────────────────────────────

def flip_output(out, out_h, out_w):
    plane = out_h * out_w
    r = out[:plane].reshape(out_h, out_w)[::-1].flatten()
    g = out[plane:plane*2].reshape(out_h, out_w)[::-1].flatten()
    b = out[plane*2:].reshape(out_h, out_w)[::-1].flatten()
    return np.concatenate([r, g, b])


def channel_means(out, out_h, out_w):
    plane = out_h * out_w
    return out[:plane].mean(), out[plane:plane*2].mean(), out[plane*2:].mean()


def flip_bayer_vertically(pattern):
    """Mirrors C++ flipBayerPatternVertically."""
    p = list(pattern)
    p[0], p[2] = p[2], p[0]
    p[1], p[3] = p[3], p[1]
    return ''.join(p)


# ── FIXED pipeline ────────────────────────────────────────────────────────────

def process_fixed(path):
    """
    Mirrors the FIXED C++ code:
    - Do NOT flip the Bayer pattern for BOTTOM-UP
    - Debayer using BAYERPAT as-is
    - Flip output bitmap only if BOTTOM-UP
    """
    with fits.open(path) as hdul:
        hdr  = hdul[0].header
        data = hdul[0].data.astype(np.float32)

    bayerpat = hdr.get('BAYERPAT', '')
    is_top_down = hdr.get('ROWORDER', 'TOP-DOWN') != 'BOTTOM-UP'

    height, width = data.shape
    out = super_pixel(data.flatten(), width, height, bayerpat)

    if not is_top_down:
        out = flip_output(out, height // 2, width // 2)

    return channel_means(out, height // 2, width // 2)


# ── BUGGY pipeline ────────────────────────────────────────────────────────────

def process_buggy(path):
    """
    Mirrors the OLD (buggy) C++ code:
    Bug 1: flip Bayer pattern when BOTTOM-UP
    Bug 2: GBRG dispatches to super_pixel_GRBG
    """
    with fits.open(path) as hdul:
        hdr  = hdul[0].header
        data = hdul[0].data.astype(np.float32)

    bayerpat = hdr.get('BAYERPAT', '')
    is_top_down = hdr.get('ROWORDER', 'TOP-DOWN') != 'BOTTOM-UP'

    if not is_top_down and bayerpat:
        bayerpat = flip_bayer_vertically(bayerpat)          # bug 1

    # Bug 2: GBRG → GRBG dispatch
    def super_pixel_buggy(buf, width, height, pattern):
        if pattern == 'GBRG':
            return super_pixel_GRBG(buf, width, height)    # bug 2
        return super_pixel(buf, width, height, pattern)

    height, width = data.shape
    out = super_pixel_buggy(data.flatten(), width, height, bayerpat)

    if not is_top_down:
        out = flip_output(out, height // 2, width // 2)

    return channel_means(out, height // 2, width // 2)


# ── Main ──────────────────────────────────────────────────────────────────────

PATTERNS  = ['RGGB', 'BGGR', 'GRBG', 'GBRG']
ROWORDERS = ['TOP-DOWN', 'BOTTOM-UP']


def run_tests():
    os.makedirs(SCRIPT_DIR, exist_ok=True)
    passes = fails = 0

    # Generate all test files
    files = {}
    for pattern in PATTERNS:
        for roworder in ROWORDERS:
            fname = os.path.join(
                SCRIPT_DIR,
                f"synth_{pattern}_{roworder.replace('-','_')}.fits"
            )
            write_fits(fname, make_bayer_frame(pattern), pattern, roworder)
            files[(pattern, roworder)] = fname

    # ── Fixed pipeline ────────────────────────────────────────────────────────
    print("=" * 70)
    print("FIXED pipeline — expect R=1000 G=500 B=100 for every combination")
    print("=" * 70)
    fixed_results = {}
    for pattern in PATTERNS:
        for roworder in ROWORDERS:
            r, g, b = process_fixed(files[(pattern, roworder)])
            fixed_results[(pattern, roworder)] = (r, g, b)
            ok = abs(r - R_VAL) < 1 and abs(g - G_VAL) < 1 and abs(b - B_VAL) < 1
            status = "PASS" if ok else "FAIL"
            passes += ok
            fails  += not ok
            print(f"  [{status}] {pattern} {roworder:10s}  R={r:.0f} G={g:.0f} B={b:.0f}")

    # ── Consistency check ─────────────────────────────────────────────────────
    print()
    print("=" * 70)
    print("Consistency — TOP-DOWN vs BOTTOM-UP must have same channel means")
    print("=" * 70)
    for pattern in PATTERNS:
        td = fixed_results[(pattern, 'TOP-DOWN')]
        bu = fixed_results[(pattern, 'BOTTOM-UP')]
        ok = all(abs(a - b) < 1 for a, b in zip(td, bu))
        status = "PASS" if ok else "FAIL"
        passes += ok
        fails  += not ok
        print(f"  [{status}] {pattern}  TD={tuple(f'{v:.0f}' for v in td)}  BU={tuple(f'{v:.0f}' for v in bu)}")

    # ── Buggy pipeline ────────────────────────────────────────────────────────
    print()
    print("=" * 70)
    print("BUGGY pipeline (old code) — shows pre-fix failures")
    print("=" * 70)
    for pattern in PATTERNS:
        for roworder in ROWORDERS:
            r, g, b = process_buggy(files[(pattern, roworder)])
            ok = abs(r - R_VAL) < 1 and abs(g - G_VAL) < 1 and abs(b - B_VAL) < 1
            status = "PASS" if ok else "FAIL"
            print(f"  [{status}] {pattern} {roworder:10s}  R={r:.0f} G={g:.0f} B={b:.0f}")

    print()
    print(f"Fixed pipeline: {passes} passed, {fails} failed")
    return fails == 0


if __name__ == '__main__':
    success = run_tests()
    exit(0 if success else 1)
