#include "BookMetadataCache.h"

#include <BufferedFile.h>
#include <Logging.h>
#include <Memory.h>
#include <Serialization.h>
#include <Utf8.h>
#include <ZipFile.h>

#include <limits>
#include <mutex>

#include "FsHelpers.h"

namespace {
constexpr uint32_t BOOK_CACHE_MAGIC = 0x425843FF;  // bytes: 0xFF, "CXB"
constexpr uint8_t BOOK_CACHE_VERSION = 10;         // v10: namespace-aware OPF parsing
constexpr char bookBinFile[] = "/book.bin";
constexpr char tmpSpineBinFile[] = "/spine.bin.tmp";
constexpr char tmpTocBinFile[] = "/toc.bin.tmp";
constexpr size_t METADATA_ARENA_SLAB_BYTES = 4096;
// Reuse at most 32 KiB of arena-backed targets instead of one per chapter.
// This transient buffer is too large for the stack and is released after indexing.
constexpr size_t MAX_SIZE_LOOKUP_TARGETS = 2048;
// Buffer size for the buildBookBin streams. 3 buffers x 4KB, transient (freed on
// return); 4KB = 8 SD sectors per transfer, enough to stop the sector-cache thrash.
constexpr size_t BUILD_IO_BUFFER_SIZE = 4096;
// The render task and the main loop both ask for sizes.
std::mutex sizeWindowLock;
std::mutex bookFileMutex;

// Entry (de)serializers, templated so they run over HalFile and the Buffered*
// wrappers alike (two instantiations each -- a few hundred bytes of flash, in
// exchange for the build path streaming at SD speed instead of per-pod).
template <typename F>
uint32_t writeSpineEntryTo(F& file, const BookMetadataCache::SpineEntry& entry) {
  const uint32_t pos = file.position();
  serialization::writeString(file, entry.href);
  serialization::writePod(file, entry.cumulativeSize);
  serialization::writePod(file, entry.tocIndex);
  return pos;
}

template <typename F>
uint32_t writeTocEntryTo(F& file, const BookMetadataCache::TocEntry& entry) {
  const uint32_t pos = file.position();
  serialization::writeString(file, entry.title);
  serialization::writeString(file, entry.href);
  serialization::writeString(file, entry.anchor);
  serialization::writePod(file, entry.level);
  serialization::writePod(file, entry.spineIndex);
  return pos;
}

template <typename F>
BookMetadataCache::SpineEntry readSpineEntryFrom(F& file) {
  BookMetadataCache::SpineEntry entry;
  serialization::readString(file, entry.href);
  serialization::readPod(file, entry.cumulativeSize);
  serialization::readPod(file, entry.tocIndex);
  return entry;
}

template <typename F>
BookMetadataCache::TocEntry readTocEntryFrom(F& file) {
  BookMetadataCache::TocEntry entry;
  serialization::readString(file, entry.title);
  serialization::readString(file, entry.href);
  serialization::readString(file, entry.anchor);
  serialization::readPod(file, entry.level);
  serialization::readPod(file, entry.spineIndex);
  return entry;
}

bool readSpineCumulativeSizeAt(HalFile& file, const uint32_t lutOffset, const uint16_t index, uint32_t& outSize) {
  uint32_t entryPos = 0;
  uint32_t hrefLen = 0;
  return file.seek(lutOffset + static_cast<uint32_t>(index) * sizeof(uint32_t)) &&
         serialization::tryReadPod(file, entryPos) && file.seek(entryPos) && serialization::tryReadPod(file, hrefLen) &&
         file.seek(entryPos + sizeof(uint32_t) + hrefLen) && serialization::tryReadPod(file, outSize);
}

bool validateTocEntryAt(HalFile& file, const uint32_t lutOffset, const uint16_t spineCount, const uint16_t index) {
  const size_t fileSize = file.size();
  uint32_t entryPos = 0;
  const uint32_t lutPos = lutOffset + (static_cast<uint32_t>(spineCount) + index) * sizeof(uint32_t);
  if (lutPos > fileSize || !file.seek(lutPos) || !serialization::tryReadPod(file, entryPos) || entryPos > fileSize ||
      !file.seek(entryPos)) {
    return false;
  }
  for (int field = 0; field < 3; ++field) {
    uint32_t fieldLen = 0;
    if (!serialization::tryReadPod(file, fieldLen) || fieldLen > fileSize - file.position() ||
        !file.seek(file.position() + fieldLen)) {
      return false;
    }
  }
  uint8_t level = 0;
  int16_t spineIndex = -1;
  return serialization::tryReadPod(file, level) && serialization::tryReadPod(file, spineIndex);
}
}  // namespace

/* ============= WRITING / BUILDING FUNCTIONS ================ */

bool BookMetadataCache::beginWrite() {
  lowMemoryFailure = false;
  buildMode = true;
  spineCount = 0;
  tocCount = 0;
  return true;
}

bool BookMetadataCache::beginContentOpfPass() {
  // Open spine file for writing
  if (!Storage.openFileForWrite("BMC", cachePath + tmpSpineBinFile, spineFile)) {
    return false;
  }
  // Wrapper OOM is fine: createSpineEntry falls back to unbuffered writes.
  passOut = makeUniqueNoThrow<serialization::BufferedFileWriter>(spineFile, BUILD_IO_BUFFER_SIZE);
  return true;
}

bool BookMetadataCache::endContentOpfPass() {
  const bool flushed = !passOut || passOut->flush();
  passOut.reset();
  // Explicit close() required: member variable persists beyond function scope
  spineFile.close();
  if (!flushed) {
    LOG_ERR("BMC", "Failed writing spine tmp file");
  }
  return flushed;
}

bool BookMetadataCache::beginTocPass() {
  if (!Storage.openFileForRead("BMC", cachePath + tmpSpineBinFile, spineFile)) {
    return false;
  }
  if (!Storage.openFileForWrite("BMC", cachePath + tmpTocBinFile, tocFile)) {
    // Explicit close() required: member variable persists beyond function scope
    spineFile.close();
    return false;
  }

  if (spineCount >= LARGE_SPINE_THRESHOLD) {
    spineHrefIndex.resetStorage();
    spineHrefIndexArena.release();
    if (!spineHrefIndexArena.init(METADATA_ARENA_SLAB_BYTES) || !spineHrefIndex.resize(spineCount)) {
      LOG_ERR("BMC", "Failed to allocate spine href index arena for %u spine items", spineCount);
      lowMemoryFailure = true;
      spineHrefIndex.resetStorage();
      spineHrefIndexArena.release();
      tocFile.close();
      spineFile.close();
      return false;
    }
    spineFile.seek(0);
    for (int i = 0; i < spineCount; i++) {
      auto entry = readSpineEntry(spineFile);
      SpineHrefIndexEntry idx;
      idx.hrefHash = fnvHash64(entry.href);
      idx.hrefLen = static_cast<uint16_t>(entry.href.size());
      idx.spineIndex = static_cast<int16_t>(i);
      spineHrefIndex[i] = idx;
    }
    std::sort(spineHrefIndex.begin(), spineHrefIndex.end(),
              [](const SpineHrefIndexEntry& a, const SpineHrefIndexEntry& b) {
                return a.hrefHash < b.hrefHash || (a.hrefHash == b.hrefHash && a.hrefLen < b.hrefLen);
              });
    spineFile.seek(0);
    useSpineHrefIndex = true;
  } else {
    useSpineHrefIndex = false;
  }

  // Wrapper OOM is fine: createTocEntry falls back to unbuffered writes.
  passOut = makeUniqueNoThrow<serialization::BufferedFileWriter>(tocFile, BUILD_IO_BUFFER_SIZE);
  return true;
}

bool BookMetadataCache::endTocPass() {
  const bool flushed = !passOut || passOut->flush();
  passOut.reset();
  if (!flushed) {
    LOG_ERR("BMC", "Failed writing toc tmp file");
  }
  // Explicit close() required: member variables persist beyond function scope
  tocFile.close();
  spineFile.close();

  spineHrefIndex.resetStorage();
  spineHrefIndexArena.release();
  useSpineHrefIndex = false;

  return flushed;
}

bool BookMetadataCache::endWrite() {
  if (!buildMode) {
    LOG_DBG("BMC", "endWrite called but not in build mode");
    return false;
  }

  buildMode = false;
  return true;
}

bool BookMetadataCache::buildBookBin(const std::string& epubPath, const BookMetadata& metadata) {
  // Open book.bin for write
  if (!Storage.openFileForWrite("BMC", cachePath + bookBinFile, bookFile)) {
    return false;
  }

  constexpr uint32_t headerASize = sizeof(BOOK_CACHE_MAGIC) + sizeof(BOOK_CACHE_VERSION) +
                                   /* LUT Offset */ sizeof(uint32_t) + sizeof(spineCount) + sizeof(tocCount);
  const uint32_t metadataSize = metadata.title.size() + metadata.author.size() + metadata.language.size() +
                                metadata.coverItemHref.size() + metadata.textReferenceHref.size() +
                                sizeof(uint32_t) * 5;
  const uint32_t lutSize = sizeof(uint32_t) * spineCount + sizeof(uint32_t) * tocCount;
  const uint32_t lutOffset = headerASize + metadataSize;

  serialization::BufferedFileWriter bookOut(bookFile, BUILD_IO_BUFFER_SIZE);
  auto closeBuildFiles = [this, &bookOut](const bool removePartialBookBin = true) {
    if (removePartialBookBin) {
      bookOut.discard();
    }
    bookFile.close();
    spineFile.close();
    tocFile.close();
    if (removePartialBookBin) {
      Storage.remove((cachePath + bookBinFile).c_str());
    }
  };

  // Header A
  serialization::writePod(bookOut, BOOK_CACHE_MAGIC);
  serialization::writePod(bookOut, BOOK_CACHE_VERSION);
  serialization::writePod(bookOut, lutOffset);
  serialization::writePod(bookOut, spineCount);
  serialization::writePod(bookOut, tocCount);
  // Metadata
  serialization::writeString(bookOut, metadata.title);
  serialization::writeString(bookOut, metadata.author);
  serialization::writeString(bookOut, metadata.language);
  serialization::writeString(bookOut, metadata.coverItemHref);
  serialization::writeString(bookOut, metadata.textReferenceHref);

  // Phase 1: Open spine tmp file, write spine LUT positions, get spineBytes, then close spine tmp file.
  uint32_t spineBytes = 0;
  if (!Storage.openFileForRead("BMC", cachePath + tmpSpineBinFile, spineFile)) {
    closeBuildFiles();
    return false;
  }
  {
    serialization::BufferedFileReader spineIn(spineFile, BUILD_IO_BUFFER_SIZE);
    for (int i = 0; i < spineCount; i++) {
      const uint32_t pos = spineIn.position();
      readSpineEntryFrom(spineIn);
      serialization::writePod(bookOut, pos + lutOffset + lutSize);
    }
    spineBytes = static_cast<uint32_t>(spineIn.position());
  }
  spineFile.close();

  // Phase 2: Open TOC tmp file, write TOC LUT positions, build spineIndex->tocIndex mapping, then close TOC tmp file.
  Arena metadataArena;
  if (!metadataArena.init(METADATA_ARENA_SLAB_BYTES)) {
    LOG_ERR("BMC", "Failed to allocate metadata scratch arena (%u bytes)",
            static_cast<unsigned>(METADATA_ARENA_SLAB_BYTES));
    lowMemoryFailure = true;
    closeBuildFiles();
    return false;
  }

  // Build spineIndex->tocIndex mapping in one pass (O(n) instead of O(n*m))
  ArenaVector<int16_t> spineToTocIndex(metadataArena);
  if (!spineToTocIndex.resize(spineCount)) {
    LOG_ERR("BMC", "Failed to allocate spine-to-TOC index for %u spine items", spineCount);
    lowMemoryFailure = true;
    closeBuildFiles();
    return false;
  }
  for (size_t i = 0; i < spineToTocIndex.size(); ++i) {
    spineToTocIndex[i] = -1;
  }

  if (tocCount > 0) {
    if (!Storage.openFileForRead("BMC", cachePath + tmpTocBinFile, tocFile)) {
      closeBuildFiles();
      return false;
    }
    {
      serialization::BufferedFileReader tocIn(tocFile, BUILD_IO_BUFFER_SIZE);
      for (int i = 0; i < tocCount; i++) {
        const uint32_t pos = tocIn.position();
        readTocEntryFrom(tocIn);
        serialization::writePod(bookOut, pos + lutOffset + lutSize + spineBytes);
      }
      tocIn.seek(0);
      for (int j = 0; j < tocCount; j++) {
        auto tocEntry = readTocEntryFrom(tocIn);
        if (tocEntry.spineIndex >= 0 && tocEntry.spineIndex < spineCount) {
          if (spineToTocIndex[tocEntry.spineIndex] == -1) {
            spineToTocIndex[tocEntry.spineIndex] = static_cast<int16_t>(j);
          }
        }
      }
    }
    tocFile.close();
  }

  // Phase 3: Open spine tmp file and EPUB zip. Size calculations and spine writing.
  if (!Storage.openFileForRead("BMC", cachePath + tmpSpineBinFile, spineFile)) {
    closeBuildFiles();
    return false;
  }

  ZipFile zip(epubPath);
  // Pre-open zip file to speed up size calculations
  if (!zip.open()) {
    LOG_ERR("BMC", "Could not open EPUB zip for size calculations");
    closeBuildFiles();
    return false;
  }

  serialization::BufferedFileReader spineIn(spineFile, BUILD_IO_BUFFER_SIZE);
  ArenaVector<uint32_t> spineSizes(metadataArena);
  bool useBatchSizes = false;

  if (spineCount >= LARGE_SPINE_THRESHOLD) {
    ArenaVector<ZipFile::SizeTarget> targets(metadataArena);
    const size_t batchCapacity = std::min<size_t>(spineCount, MAX_SIZE_LOOKUP_TARGETS);
    if (!targets.resize(batchCapacity) || !spineSizes.resize(spineCount)) {
      LOG_ERR("BMC", "Failed to allocate batch size lookup scratch for %u spine items", spineCount);
      lowMemoryFailure = true;
      zip.close();
      closeBuildFiles();
      return false;
    }

    size_t targetCount = 0;
    int matched = 0;
    spineIn.seek(0);
    for (int i = 0; i < spineCount; i++) {
      auto entry = readSpineEntryFrom(spineIn);
      std::string path = FsHelpers::normalisePath(entry.href);

      ZipFile::SizeTarget t;
      t.hash = ZipFile::fnvHash64(path.c_str(), path.size());
      t.len = static_cast<uint16_t>(path.size());
      t.index = static_cast<uint16_t>(i);
      targets[targetCount++] = t;
      if (targetCount < batchCapacity && i + 1 < spineCount) continue;

      std::sort(targets.begin(), targets.begin() + targetCount,
                [](const ZipFile::SizeTarget& a, const ZipFile::SizeTarget& b) {
                  return a.hash < b.hash || (a.hash == b.hash && a.len < b.len);
                });
      matched += zip.fillUncompressedSizes(targets.data(), targetCount, spineSizes.data(), spineSizes.size());
      targetCount = 0;
    }
    LOG_DBG("BMC", "Batch lookup matched %d/%d spine items", matched, spineCount);

    useBatchSizes = true;
  }

  uint32_t cumSize = 0;
  int lastSpineTocIndex = -1;
  spineIn.seek(0);
  for (int i = 0; i < spineCount; i++) {
    auto spineEntry = readSpineEntryFrom(spineIn);

    spineEntry.tocIndex = spineToTocIndex[i];

    if (spineEntry.tocIndex == -1) {
      LOG_DBG("BMC", "Warning: Could not find TOC entry for spine item %d: %s, using title from last section", i,
              spineEntry.href.c_str());
      spineEntry.tocIndex = lastSpineTocIndex;
    }
    lastSpineTocIndex = spineEntry.tocIndex;

    size_t itemSize = 0;
    if (useBatchSizes) {
      itemSize = spineSizes[i];
      if (itemSize == 0) {
        const std::string path = FsHelpers::normalisePath(spineEntry.href);
        if (!zip.getInflatedFileSize(path.c_str(), &itemSize)) {
          LOG_ERR("BMC", "Warning: Could not get size for spine item: %s", path.c_str());
        }
      }
    } else {
      const std::string path = FsHelpers::normalisePath(spineEntry.href);
      if (!zip.getInflatedFileSize(path.c_str(), &itemSize)) {
        LOG_ERR("BMC", "Warning: Could not get size for spine item: %s", path.c_str());
      }
    }

    constexpr size_t maxStoredCumulativeSize = std::numeric_limits<uint32_t>::max();
    if (itemSize > maxStoredCumulativeSize || cumSize > maxStoredCumulativeSize - itemSize) {
      LOG_ERR("BMC", "Spine cumulative size overflow for item %d (cumSize=%u, itemSize=%zu)", i, cumSize, itemSize);
      zip.close();
      closeBuildFiles();
      return false;
    }

    cumSize += itemSize;
    spineEntry.cumulativeSize = cumSize;

    // Write out spine data to book.bin
    writeSpineEntryTo(bookOut, spineEntry);
  }
  // Close zip and spine tmp file
  zip.close();
  spineFile.close();

  // Phase 4: Open TOC tmp file, write TOC entries to book.bin, then close TOC tmp file.
  if (tocCount > 0) {
    if (!Storage.openFileForRead("BMC", cachePath + tmpTocBinFile, tocFile)) {
      closeBuildFiles();
      return false;
    }
    {
      serialization::BufferedFileReader tocIn(tocFile, BUILD_IO_BUFFER_SIZE);
      for (int i = 0; i < tocCount; i++) {
        auto tocEntry = readTocEntryFrom(tocIn);
        writeTocEntryTo(bookOut, tocEntry);
      }
    }
    tocFile.close();
  }

  // Phase 5: Flush and close book.bin
  const bool written = bookOut.flush();
  closeBuildFiles(!written);

  if (!written) {
    LOG_ERR("BMC", "Failed writing book.bin, removing truncated file");
    return false;
  }

  return true;
}

bool BookMetadataCache::cleanupTmpFiles() const {
  const auto spineBinFile = cachePath + tmpSpineBinFile;
  if (Storage.exists(spineBinFile.c_str())) {
    Storage.remove(spineBinFile.c_str());
  }
  const auto tocBinFile = cachePath + tmpTocBinFile;
  if (Storage.exists(tocBinFile.c_str())) {
    Storage.remove(tocBinFile.c_str());
  }
  return true;
}

uint32_t BookMetadataCache::writeSpineEntry(HalFile& file, const SpineEntry& entry) const {
  return writeSpineEntryTo(file, entry);
}

uint32_t BookMetadataCache::writeTocEntry(HalFile& file, const TocEntry& entry) const {
  return writeTocEntryTo(file, entry);
}

// Note: for the LUT to be accurate, this **MUST** be called for all spine items before `addTocEntry` is ever called
// this is because in this function we're marking positions of the items
void BookMetadataCache::createSpineEntry(const std::string& href) {
  if (!buildMode || !spineFile) {
    LOG_DBG("BMC", "createSpineEntry called but not in build mode");
    return;
  }

  const SpineEntry entry(href, 0, -1);
  if (passOut) {
    writeSpineEntryTo(*passOut, entry);
  } else {
    writeSpineEntry(spineFile, entry);
  }
  spineCount++;
}

void BookMetadataCache::createTocEntry(const std::string& title, const std::string& href, const std::string& anchor,
                                       const uint8_t level) {
  if (!buildMode || !tocFile || !spineFile) {
    LOG_DBG("BMC", "createTocEntry called but not in build mode");
    return;
  }

  int16_t spineIndex = -1;

  if (useSpineHrefIndex) {
    uint64_t targetHash = fnvHash64(href);
    uint16_t targetLen = static_cast<uint16_t>(href.size());

    auto it =
        std::lower_bound(spineHrefIndex.begin(), spineHrefIndex.end(), SpineHrefIndexEntry{targetHash, targetLen, 0},
                         [](const SpineHrefIndexEntry& a, const SpineHrefIndexEntry& b) {
                           return a.hrefHash < b.hrefHash || (a.hrefHash == b.hrefHash && a.hrefLen < b.hrefLen);
                         });

    while (it != spineHrefIndex.end() && it->hrefHash == targetHash && it->hrefLen == targetLen) {
      spineIndex = it->spineIndex;
      break;
    }

    if (spineIndex == -1) {
      LOG_DBG("BMC", "createTocEntry: Could not find spine item for TOC href %s", href.c_str());
    }
  } else {
    spineFile.seek(0);
    for (int i = 0; i < spineCount; i++) {
      auto spineEntry = readSpineEntry(spineFile);
      if (spineEntry.href == href) {
        spineIndex = static_cast<int16_t>(i);
        break;
      }
    }
    if (spineIndex == -1) {
      LOG_DBG("BMC", "createTocEntry: Could not find spine item for TOC href %s", href.c_str());
    }
  }

  // Compose the title to NFC at index time so the cache stores precomposed glyphs;
  // device fonts have no combining-mark positioning, so NFD titles render broken.
  const TocEntry entry(utf8ComposeNfc(title), href, anchor, level, spineIndex);
  if (passOut) {
    writeTocEntryTo(*passOut, entry);
  } else {
    writeTocEntry(tocFile, entry);
  }
  tocCount++;
}

/* ============= READING / LOADING FUNCTIONS ================ */

bool BookMetadataCache::exists(const std::string& cachePath) {
  return Storage.exists((cachePath + bookBinFile).c_str());
}

bool BookMetadataCache::load() {
  loaded = false;
  const auto bookBinPath = cachePath + bookBinFile;
  if (!Storage.openFileForRead("BMC", bookBinPath, bookFile)) {
    return false;
  }

  uint32_t magic;
  if (!serialization::tryReadPod(bookFile, magic)) {
    LOG_DBG("BMC", "Cache header is truncated");
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }
  if (magic != BOOK_CACHE_MAGIC) {
    LOG_DBG("BMC", "Cache magic mismatch");
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }

  uint8_t version;
  if (!serialization::tryReadPod(bookFile, version)) {
    LOG_DBG("BMC", "Cache version is missing");
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }
  if (version != BOOK_CACHE_VERSION) {
    LOG_DBG("BMC", "Cache version mismatch: expected %d, got %d", BOOK_CACHE_VERSION, version);
    // Explicit close() required: member variable persists beyond function scope
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }

  if (!serialization::tryReadPod(bookFile, lutOffset) || !serialization::tryReadPod(bookFile, spineCount) ||
      !serialization::tryReadPod(bookFile, tocCount) || !serialization::tryReadString(bookFile, coreMetadata.title) ||
      !serialization::tryReadString(bookFile, coreMetadata.author) ||
      !serialization::tryReadString(bookFile, coreMetadata.language) ||
      !serialization::tryReadString(bookFile, coreMetadata.coverItemHref) ||
      !serialization::tryReadString(bookFile, coreMetadata.textReferenceHref)) {
    LOG_DBG("BMC", "Cache metadata is truncated");
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }

  // Spine entries follow the LUTs in index order; jump via the spine LUT to read
  // only every SIZE_WINDOW-th cumulative size and the final bookSize without
  // allocating/reading every spine href string.
  windowStarts.clear();
  bookSize = 0;
  windowFirst = -1;
  windowStarts.reserve((spineCount + SIZE_WINDOW - 1) / SIZE_WINDOW);
  if (spineCount > 0) {
    windowStarts.push_back(0);
    for (uint16_t i = SIZE_WINDOW; i < spineCount; i += SIZE_WINDOW) {
      uint32_t cumSize = 0;
      if (!readSpineCumulativeSizeAt(bookFile, lutOffset, i - 1, cumSize)) {
        LOG_DBG("BMC", "Cache spine LUT/entry is truncated at %u", static_cast<unsigned>(i - 1));
        bookFile.close();
        Storage.remove(bookBinPath.c_str());
        return false;
      }
      windowStarts.push_back(cumSize);
    }
    if (!readSpineCumulativeSizeAt(bookFile, lutOffset, spineCount - 1, bookSize)) {
      LOG_DBG("BMC", "Cache final spine entry is truncated");
      bookFile.close();
      Storage.remove(bookBinPath.c_str());
      return false;
    }
  }
  if (tocCount > 0 && !validateTocEntryAt(bookFile, lutOffset, spineCount, tocCount - 1)) {
    LOG_DBG("BMC", "Cache final TOC entry is truncated");
    bookFile.close();
    Storage.remove(bookBinPath.c_str());
    return false;
  }

  loaded = true;
  return true;
}

bool BookMetadataCache::fillWindow(const int first) const {
  windowFirst = -1;
  uint32_t pos = 0;
  const auto u32 = [this](uint32_t& v) { return bookFile.read(&v, sizeof(v)) == static_cast<int>(sizeof(v)); };
  std::lock_guard<std::mutex> lock(bookFileMutex);
  if (!bookFile.isOpen()) {
    return false;
  }
  bool ok = bookFile.seek(lutOffset + sizeof(uint32_t) * first) && u32(pos) && bookFile.seek(pos);
  // Spine entry: href length, href, cumulative size, TOC index.
  for (int i = 0; ok && i < SIZE_WINDOW && first + i < spineCount; i++) {
    uint32_t hrefLen = 0;
    ok = u32(hrefLen) && bookFile.seek(bookFile.position() + hrefLen) && u32(window[i]) &&
         bookFile.seek(bookFile.position() + sizeof(int16_t));
  }
  if (ok) windowFirst = first;
  return ok;
}

int BookMetadataCache::getSpineIndexForSize(const uint32_t size) const {
  if (!loaded || spineCount == 0 || size > bookSize) return -1;
  // windowStarts[w] is the total through item w * SIZE_WINDOW - 1, so the answer lies in the window
  // before the first start that reaches `size`. Its last total comes from RAM and always reaches it.
  const auto next = std::lower_bound(windowStarts.begin() + 1, windowStarts.end(), size);
  const int first = static_cast<int>(next - windowStarts.begin() - 1) * SIZE_WINDOW;
  const int last = std::min<int>(spineCount, first + SIZE_WINDOW) - 1;
  std::lock_guard<std::mutex> lock(sizeWindowLock);
  // One refill attempt; on failure every total in the window falls back below `size`, as before.
  if (windowFirst != first && !fillWindow(first)) return last;
  for (int i = first; i < last; i++) {
    if (window[i - first] >= size) return i;
  }
  return last;
}

BookMetadataCache::SpineEntry BookMetadataCache::getSpineEntry(const int index) {
  if (!loaded) {
    LOG_ERR("BMC", "getSpineEntry called but cache not loaded");
    return {};
  }

  if (index < 0 || index >= static_cast<int>(spineCount)) {
    LOG_ERR("BMC", "getSpineEntry index %d out of range", index);
    return {};
  }

  std::lock_guard<std::mutex> lock(bookFileMutex);
  if (!bookFile.isOpen()) {
    return {};
  }
  // Seek to spine LUT item, read from LUT and get out data
  bookFile.seek(lutOffset + sizeof(uint32_t) * index);
  uint32_t spineEntryPos = 0;
  serialization::readPod(bookFile, spineEntryPos);
  bookFile.seek(spineEntryPos);
  return readSpineEntry(bookFile);
}

size_t BookMetadataCache::getSpineCumulativeSize(const int index) {
  if (!loaded || index < 0 || index >= static_cast<int>(spineCount)) {
    return 0;
  }
  // Window edges come from RAM, so a page at a window's first item does not read the one before.
  if (index == static_cast<int>(spineCount) - 1) return bookSize;
  const int first = index / SIZE_WINDOW * SIZE_WINDOW;
  if (index - first == SIZE_WINDOW - 1) return windowStarts[index / SIZE_WINDOW + 1];
  std::lock_guard<std::mutex> lock(sizeWindowLock);
  // An item within the window needs the window itself populated. When the window cannot be read
  // (e.g. disk read error), return the best known lower bound: the start total before the window.
  if (windowFirst != first && !fillWindow(first)) return windowStarts[index / SIZE_WINDOW];
  return window[index - first];
}

BookMetadataCache::TocEntry BookMetadataCache::getTocEntry(const int index) {
  if (!loaded) {
    LOG_ERR("BMC", "getTocEntry called but cache not loaded");
    return {};
  }

  if (index < 0 || index >= static_cast<int>(tocCount)) {
    LOG_ERR("BMC", "getTocEntry index %d out of range", index);
    return {};
  }

  std::lock_guard<std::mutex> lock(bookFileMutex);
  if (!bookFile.isOpen()) {
    return {};
  }
  // Seek to TOC LUT item, read from LUT and get out data
  bookFile.seek(lutOffset + sizeof(uint32_t) * spineCount + sizeof(uint32_t) * index);
  uint32_t tocEntryPos = 0;
  serialization::readPod(bookFile, tocEntryPos);
  bookFile.seek(tocEntryPos);
  return readTocEntry(bookFile);
}

BookMetadataCache::SpineEntry BookMetadataCache::readSpineEntry(HalFile& file) const {
  return readSpineEntryFrom(file);
}

BookMetadataCache::TocEntry BookMetadataCache::readTocEntry(HalFile& file) const { return readTocEntryFrom(file); }
