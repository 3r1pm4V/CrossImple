/* SPDX-License-Identifier: Apache-2.0 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Chuẩn hóa chuỗi UTF-8 Unicode tổ hợp (NFD) sang dựng sẵn (NFC) trực tiếp tại chỗ.
/// In-place canonical composition (NFC) for Latin & Vietnamese combining diacritical marks.
/// Returns true if any characters were composed.
bool utf8_compose_nfc_in_place(char *buffer);

/// Kiểm tra xem ký tự có phải là combining mark tiếng Việt/Latin (U+0300 - U+036F) không.
bool utf8_is_combining_mark(uint32_t cp);

/// Tìm ký tự dựng sẵn từ (base, mark). Trả về 0 nếu không có.
uint32_t utf8_compose_pair(uint32_t base, uint32_t mark);

#ifdef __cplusplus
}
#endif
