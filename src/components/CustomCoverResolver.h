#pragma once

#include <string>

namespace CustomCoverResolver {

/**
 * Checks for a custom cover image matching the book path.
 * Resolves in order:
 * 1. <book_dir>/<book_name>.bmp / .jpg / .jpeg / .png / _cover.*
 * 2. /.crosspoint/custom_cover/<book_name>.bmp / .jpg / .jpeg / .png
 * 3. /custom_cover/<book_name>.bmp / .jpg / .jpeg / .png
 *
 * If a BMP is found, its path is returned directly.
 * If a JPG/PNG is found, it is converted into a 1-bit BMP stored in
 * /.crosspoint/custom_cover/cache/<book_name>.bmp and that path is returned.
 *
 * Returns empty string if no custom cover exists.
 */
std::string resolveCustomCoverBmpPath(const std::string& bookPath, int targetWidth = 300, int targetHeight = 450);

}  // namespace CustomCoverResolver
