/* SPDX-FileCopyrightText: 2026 mindreset
 * SPDX-License-Identifier: Apache-2.0
 * 中文：本地阅读器支持的文件格式；持久历史不是支持书源的依据。
 * English: Local reader file formats; persistent history never implies source support.
 */
#pragma once
#include <stdbool.h>
#include <string.h>
#include <strings.h>

static inline bool book_file_path_is_os_junk(const char *name) {
    if (!name || !*name) return false;
    if (name[0] == '.') return true;
    if (!strcasecmp(name, "System Volume Information") ||
        !strcasecmp(name, "$RECYCLE.BIN") ||
        !strcasecmp(name, "LOST.DIR") ||
        !strcasecmp(name, "Android") ||
        !strcasecmp(name, "Thumbs.db") ||
        !strcasecmp(name, "desktop.ini")) return true;
    return false;
}

static inline bool book_file_path_visible(const char *path) {
    if (!path || !*path) return false;
    if (!strncasecmp(path, "/sdcard/", 8)) {
        const char *relative = path + 8;
        while (*relative == '/') ++relative;
        if (!strncasecmp(relative, "DPRWIFI", 7) && (!relative[7] || relative[7] == '/')) return false;
        if (!strcasecmp(relative, "dpreader_log.txt")) return false;
    }
    if (path[0] == '.' || strstr(path, "/.")) return false;
    const char *p = path;
    while (*p) {
        while (*p == '/') ++p;
        if (!*p) break;
        const char *slash = strchr(p, '/');
        size_t len = slash ? (size_t)(slash - p) : strlen(p);
        if (len > 0 && len < 64) {
            char seg[64];
            memcpy(seg, p, len);
            seg[len] = 0;
            if (book_file_path_is_os_junk(seg)) return false;
        }
        p = slash ? slash + 1 : p + len;
    }
    return true;
}

/// 仅检查完整文件名的TXT/EPUB扩展名，不访问文件或修改记录。
/// Check the final filename for TXT/EPUB without filesystem access or record mutation.
static inline bool book_file_path_supported(const char *path) {
    if (!path || !*path) return false;
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;
    const char *ext = strrchr(name, '.');
    return ext && (!strcasecmp(ext, ".txt") || !strcasecmp(ext, ".epub"));
}
