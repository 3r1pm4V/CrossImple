/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "epd_highlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Vẽ hình nền nghệ thuật mặc định (Stock Wallpaper) vào framebuffer.
/// Được nhúng sẵn trong ROM để máy luôn có hình nền đẹp kể cả khi không có thẻ nhớ hoặc chưa cài ảnh bìa.
bool stock_wallpaper_draw(uint8_t *fb, EpdRect area);

#ifdef __cplusplus
}
#endif
