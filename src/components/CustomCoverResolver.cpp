#include "CustomCoverResolver.h"

#include <FsHelpers.h>
#include <HalStorage.h>
#include <JpegToBmpConverter.h>
#include <Logging.h>
#include <PngToBmpConverter.h>

#include <algorithm>
#include <string>
#include <vector>

namespace CustomCoverResolver {
namespace {

std::string getBaseName(const std::string& path) {
  size_t lastSlash = path.find_last_of("/\\");
  std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
  size_t lastDot = filename.find_last_of('.');
  if (lastDot != std::string::npos) {
    return filename.substr(0, lastDot);
  }
  return filename;
}

std::string getPathWithoutExt(const std::string& path) {
  size_t lastDot = path.find_last_of('.');
  if (lastDot != std::string::npos &&
      (path.find_last_of("/\\") == std::string::npos || lastDot > path.find_last_of("/\\"))) {
    return path.substr(0, lastDot);
  }
  return path;
}

}  // namespace

std::string resolveCustomCoverBmpPath(const std::string& bookPath, int targetWidth, int targetHeight) {
  if (bookPath.empty()) return "";

  const std::string baseName = getBaseName(bookPath);
  if (baseName.empty()) return "";

  const std::string pathWithoutExt = getPathWithoutExt(bookPath);

  // 1. Direct BMP matches (Zero CPU conversion, immediate return)
  const std::vector<std::string> bmpCandidates = {
      pathWithoutExt + ".bmp",
      pathWithoutExt + "_cover.bmp",
      "/.crosspoint/custom_cover/" + baseName + ".bmp",
      "/custom_cover/" + baseName + ".bmp"
  };

  for (const auto& candidate : bmpCandidates) {
    if (Storage.exists(candidate.c_str())) {
      LOG_INF("COVER", "Found custom BMP cover: %s", candidate.c_str());
      return candidate;
    }
  }

  // 2. JPG / PNG candidates (converted once to cached thumbnail BMP)
  const std::vector<std::string> imgCandidates = {
      pathWithoutExt + ".jpg",
      pathWithoutExt + ".jpeg",
      pathWithoutExt + ".png",
      pathWithoutExt + "_cover.jpg",
      pathWithoutExt + "_cover.png",
      "/.crosspoint/custom_cover/" + baseName + ".jpg",
      "/.crosspoint/custom_cover/" + baseName + ".jpeg",
      "/.crosspoint/custom_cover/" + baseName + ".png",
      "/custom_cover/" + baseName + ".jpg",
      "/custom_cover/" + baseName + ".jpeg",
      "/custom_cover/" + baseName + ".png"
  };

  for (const auto& candidate : imgCandidates) {
    if (!Storage.exists(candidate.c_str())) continue;

    // Check if converted cached BMP already exists
    Storage.mkdir("/.crosspoint");
    Storage.mkdir("/.crosspoint/custom_cover");
    Storage.mkdir("/.crosspoint/custom_cover/cache");
    const std::string cachedBmpPath = "/.crosspoint/custom_cover/cache/" + baseName + ".bmp";

    if (Storage.exists(cachedBmpPath.c_str())) {
      return cachedBmpPath;
    }

    // Convert JPG or PNG to 1-bit BMP stream
    HalFile srcFile;
    if (!Storage.openFileForRead("COVER", candidate.c_str(), srcFile)) {
      continue;
    }

    HalFile dstFile;
    if (!Storage.openFileForWrite("COVER", cachedBmpPath.c_str(), dstFile)) {
      srcFile.close();
      continue;
    }

    bool ok = false;
    if (FsHelpers::hasJpgExtension(candidate)) {
      ok = JpegToBmpConverter::jpegFileTo1BitBmpStreamWithSize(srcFile, dstFile, targetWidth, targetHeight, true);
    } else if (FsHelpers::hasPngExtension(candidate)) {
      ok = PngToBmpConverter::pngFileTo1BitBmpStreamWithSize(srcFile, dstFile, targetWidth, targetHeight, true);
    }

    srcFile.close();
    dstFile.close();

    if (ok) {
      LOG_INF("COVER", "Converted custom cover %s -> %s", candidate.c_str(), cachedBmpPath.c_str());
      return cachedBmpPath;
    } else {
      Storage.remove(cachedBmpPath.c_str());
    }
  }

  return "";
}

}  // namespace CustomCoverResolver
