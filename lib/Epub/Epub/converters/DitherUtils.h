#pragma once

#include <stdint.h>

// 4x4 Bayer matrix for ordered dithering
inline const uint8_t bayer4x4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

// Fine 8x8 Bayer matrix for ordered dithering (64 threshold levels).
// Eliminates the coarse grid / crosshatch pattern of traditional 4x4 Bayer.
inline const uint8_t bayer8x8[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44,  4, 36, 14, 46,  6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43,  1, 33,  9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

// Divide the full 8-bit range into four display levels with E-Ink contrast clamping.
// Clamps dark lines to pure black and bright backgrounds to clean white.
inline uint8_t quantizeGrayTo4Level(uint8_t gray) {
  if (gray <= 45) return 0;
  if (gray >= 225) return 3;
  if (gray < 110) return 1;
  return 2;
}

// Apply E-Ink optimized dithering and quantize to 4 levels (0-3).
// 1. Black Clamping (<= 45): Pure solid black for text and ink lines, eliminating dither noise.
// 2. White Clamping (>= 225): Pure paper white, eliminating dirty dither speckles on backgrounds.
// 3. Midtone Expansion: Stretches [46..224] to full dynamic range [0..255].
// 4. Fine 8x8 Bayer Dithering: Smooth transitions without visible 4x4 matrix grid.
inline uint8_t applyBayerDither4Level(uint8_t gray, int x, int y) {
  if (gray <= 45) return 0;
  if (gray >= 225) return 3;

  // Stretch midtones to [0..255]
  int stretched = (static_cast<int>(gray) - 45) * 255 / (225 - 45);

  // 8x8 Bayer offset: range -40 to +38 (half of quantization step ~85)
  int bayer = bayer8x8[y & 7][x & 7];
  int dither = (bayer - 32) * 5 / 4;

  int adjusted = stretched + dither;
  if (adjusted < 0) adjusted = 0;
  if (adjusted > 255) adjusted = 255;

  // Non-linear threshold tuned for reflective physical E-Ink microcapsules
  if (adjusted < 50) return 0;
  if (adjusted < 128) return 1;
  if (adjusted < 205) return 2;
  return 3;
}
