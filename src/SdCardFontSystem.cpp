#include "SdCardFontSystem.h"
#if CROSSINK_SCALABLE_FONTS
#include <HalScalableFont.h>
#include <ScalableBuiltins.h>
#endif

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <MemoryBudget.h>

#include <cstdio>
#include <cstring>

#include "CrossPointSettings.h"
#include "FilenameFontSystem.h"
#if CROSSINK_SCALABLE_FONTS
#include "TtfRenderProfileStore.h"
#endif
#include "fontIds.h"

namespace {
enum class FontFileSelection : uint8_t { Closest, Exact };

// This is a cold setup path, not a render loop. The 320-byte stack footprint
// (path and filename) replace a heap-allocated whole-font catalog
// during every dictionary swap, avoiding persistent fragmentation on the C3.
bool findInstalledFontFile(const char* familyName, const uint8_t targetPointSize, const FontFileSelection selection,
                           char* path, const size_t pathSize, uint8_t& selectedPointSize) {
  if (!familyName || familyName[0] == '\0' || !path || pathSize == 0) return false;

  const char* root = SdCardFontRegistry::findFamilyRoot(familyName);
  if (!root) return false;
  const int directoryLength = std::snprintf(path, pathSize, "%s/%s", root, familyName);
  if (directoryLength <= 0 || static_cast<size_t>(directoryLength) >= pathSize) return false;

  HalFile dir = Storage.open(path);
  if (!dir || !dir.isDirectory()) return false;

  uint8_t closestSize = 0;
  uint8_t closestDiff = UINT8_MAX;
  char filename[128] = {};
  while (true) {
    HalFile entry = dir.openNextFile();
    if (!entry) break;
    const bool isDirectory = entry.isDirectory();
    if (!isDirectory) entry.getName(filename, sizeof(filename));
    entry.close();
    if (isDirectory) continue;

    uint8_t pointSize = 0;
    uint8_t style = 0;
    if (!SdCardFontRegistry::parseFilename(filename, pointSize, style) || style != 0) continue;

    if (selection == FontFileSelection::Closest) {
      const uint8_t diff = pointSize > targetPointSize ? pointSize - targetPointSize : targetPointSize - pointSize;
      if (closestDiff == UINT8_MAX || diff < closestDiff || (diff == closestDiff && pointSize < closestSize)) {
        closestSize = pointSize;
        closestDiff = diff;
      }
    }
  }
  dir.close();

  if (selection == FontFileSelection::Closest) {
    selectedPointSize = closestSize;
  } else if (selection == FontFileSelection::Exact) {
    selectedPointSize = targetPointSize;
  }

  if (selectedPointSize == 0) return false;
  // Scan once more to preserve the exact file name rather than assuming the
  // file base name matches the directory name.
  dir = Storage.open(path);
  if (!dir || !dir.isDirectory()) return false;
  while (true) {
    HalFile entry = dir.openNextFile();
    if (!entry) break;
    const bool isDirectory = entry.isDirectory();
    if (!isDirectory) entry.getName(filename, sizeof(filename));
    entry.close();
    if (isDirectory) continue;

    uint8_t pointSize = 0;
    uint8_t style = 0;
    if (!SdCardFontRegistry::parseFilename(filename, pointSize, style) || style != 0 ||
        pointSize != selectedPointSize) {
      continue;
    }
    const int pathLength = std::snprintf(path, pathSize, "%s/%s/%s", root, familyName, filename);
    dir.close();
    return pathLength > 0 && static_cast<size_t>(pathLength) < pathSize;
  }
  dir.close();
  return false;
}

}  // namespace

void SdCardFontSystem::begin(GfxRenderer& renderer, const bool preloadSelectedFont) {
  // Register this system as the SD font ID resolver in settings.
  // Uses a static trampoline since CrossPointSettings stores a plain function pointer.
  SETTINGS.sdFontIdResolver = [](void* ctx, const char* familyName, uint8_t pointSize) -> int {
    return static_cast<SdCardFontSystem*>(ctx)->resolveFontId(familyName, pointSize);
  };
  SETTINGS.sdFontResolverCtx = this;

  if (SETTINGS.sdFontFamilyName[0] == '\0') {
    LOG_DBG("SDFS", "SD font resolver ready; discovery deferred until requested");
    return;
  }

#if CROSSINK_SCALABLE_FONTS
  (void)preloadSelectedFont;
  LOG_DBG("SDFS", "SD font resolver ready; selected font load deferred until requested");
#else
  if (!preloadSelectedFont) {
    LOG_DBG("SDFS", "SD font resolver ready; Home boot defers font load until reader entry");
    return;
  }
  // C3 bitmap fonts retain their existing load timing and memory budget.
  ensureLoaded(renderer);
  releaseRegistry();
#endif
}

int SdCardFontSystem::ensureBuiltInReaderFont(GfxRenderer& renderer) {
#if CROSSINK_SCALABLE_FONTS
  filenameFontSystem.ensureLoaded(renderer);
  ensureScalableBuiltinFamily(renderer, SETTINGS.fontFamily == CrossPointSettings::BITTER ? 1 : 0);
#else
  (void)renderer;
#endif
  return SETTINGS.getBuiltInReaderFontId();
}

void SdCardFontSystem::persistSettingsChange() const {
  if (settingsPersistenceCallback_) {
    settingsPersistenceCallback_(settingsPersistenceContext_);
  } else {
    SETTINGS.saveToFile();
  }
}

void SdCardFontSystem::ensureLoaded(GfxRenderer& renderer) {
#if CROSSINK_SCALABLE_FONTS
  ScalableFontAccess access;
  filenameFontSystem.ensureLoaded(renderer);
#endif
  // If the web server (or another task) installed/deleted fonts, re-discover.
  // Track whether we just re-discovered so we can force a reload below even
  // when the wanted family/size still maps to the same point size — the file
  // contents on disk may have changed (e.g. user re-uploaded a new build).
  bool registryWasDirty = registryDirty_.load(std::memory_order_acquire) || fontReloadPending_ ||
                          loadedRegistryRevision_ != registry_.revision();

  const char* wantedFamily = SETTINGS.sdFontFamilyName;
  const std::string& currentFamily = manager_.currentFamilyName();
  uint8_t targetPointSize = SETTINGS.getSdFontTargetPointSize();

  if (wantedFamily[0] == '\0') {
    if (!currentFamily.empty()) {
      manager_.unloadAll(renderer);
      loadedFontPointSize_ = 0;
    }
#if CROSSINK_SCALABLE_FONTS
    ensureBuiltInReaderFont(renderer);
#endif
    return;
  }

  if (!registryWasDirty && !manager_.hasTemporaryScalableFamily() && currentFamily == wantedFamily &&
      loadedFontPointSize_ == targetPointSize && SETTINGS.legacySdFontSizeStep == UINT8_MAX) {
    return;
  }

  ensureRegistry();
  if (registry_.lastDiscoveryFailed()) {
#if CROSSINK_SCALABLE_FONTS
    ensureBuiltInReaderFont(renderer);
#endif
    return;
  }
  registryWasDirty = registryWasDirty || fontReloadPending_ || loadedRegistryRevision_ != registry_.revision();

  const auto* family = registry_.findSummary(wantedFamily);
  if (family && !family->ensureDetails()) {
    LOG_ERR("SDFS", "Cannot read SD font details: %s (preserving selection)", wantedFamily);
#if CROSSINK_SCALABLE_FONTS
    ensureBuiltInReaderFont(renderer);
#endif
    return;
  }
  if (family && SETTINGS.legacySdFontSizeStep != UINT8_MAX) {
    const auto sizes = family->availableSizes();
    if (!sizes.empty()) {
      const uint8_t step = std::min<uint8_t>(SETTINGS.legacySdFontSizeStep, sizes.size() - 1);
      targetPointSize = sizes[step];
      SETTINGS.readerFontPointSize = targetPointSize;
      SETTINGS.legacySdFontSizeStep = UINT8_MAX;
      persistSettingsChange();
      LOG_INF("SDFS", "Migrated SD font size to %u pt", targetPointSize);
    }
  }

  // Reload if family changed OR if the user-selected size maps to a
  // different file than what's currently loaded OR if the registry was
  // just rediscovered (file may have been replaced on disk).
  bool familyMatches = (currentFamily == wantedFamily);
  if (familyMatches) {
    if (!family) {
      LOG_DBG("SDFS", "SD font family disappeared: %s (clearing)", wantedFamily);
      manager_.unloadAll(renderer);
      SETTINGS.sdFontFamilyName[0] = '\0';
      persistSettingsChange();
#if CROSSINK_SCALABLE_FONTS
      ensureBuiltInReaderFont(renderer);
#endif
      return;
    }
    const auto* wantedFile = family->findClosestFile(targetPointSize);
    uint8_t wantedPt = family->isScalable() ? targetPointSize : (wantedFile ? wantedFile->pointSize : 0);
    if (!registryWasDirty && !manager_.hasTemporaryScalableFamily() && wantedPt == manager_.currentPointSize()) return;
    LOG_DBG("SDFS", "Reloading %s: size %u -> %u (target %u)%s", wantedFamily, manager_.currentPointSize(), wantedPt,
            targetPointSize, registryWasDirty ? " [registry dirty]" : "");
  }

  if (!currentFamily.empty() && (!family || !family->isScalable() || !familyMatches || registryWasDirty)) {
    manager_.unloadAll(renderer);
  }

  if (family) {
#if CROSSINK_SCALABLE_FONTS
    const auto options = family->isScalable() ? ttfRenderOptions(TTF_RENDER_PROFILES.profileFor(wantedFamily))
                                              : freeink::font::FtFont::RenderOptions{};
    const bool loaded = family->isScalable() ? manager_.loadFamilyClosest(*family, renderer, targetPointSize, options)
                                             : manager_.loadFamilyClosest(*family, renderer, targetPointSize);
#else
    const bool loaded = manager_.loadFamilyClosest(*family, renderer, targetPointSize);
#endif
    if (loaded) {
      loadedFontPointSize_ = targetPointSize;
      fontReloadPending_ = false;
      loadedRegistryRevision_ = registry_.revision();
      LOG_DBG("SDFS", "Loaded SD font family: %s", wantedFamily);
    } else {
      LOG_ERR("SDFS", "Failed to load SD font family: %s (preserving selection)", wantedFamily);
#if CROSSINK_SCALABLE_FONTS
      ensureBuiltInReaderFont(renderer);
#endif
    }
  } else {
    LOG_DBG("SDFS", "SD font family not found: %s (clearing)", wantedFamily);
    SETTINGS.sdFontFamilyName[0] = '\0';
    persistSettingsChange();
#if CROSSINK_SCALABLE_FONTS
    ensureBuiltInReaderFont(renderer);
#endif
  }
}

bool SdCardFontSystem::isScalableFamily(const char* familyName) {
#if CROSSINK_SCALABLE_FONTS
  if (!familyName || !familyName[0]) return false;
  ensureRegistry();
  const auto* family = registry_.findFamily(familyName);
  return family && family->ensureDetails() && family->isScalable();
#else
  (void)familyName;
  return false;
#endif
}

bool SdCardFontSystem::fontUsesMonochromeRaster(const GfxRenderer& renderer, const int fontId,
                                                const char* familyName) const {
#if CROSSINK_SCALABLE_FONTS
  return renderer.isSdCardFont(fontId) && hasResidentScalableFamily(familyName) &&
         TTF_RENDER_PROFILES.profileFor(familyName).raster != 0;
#else
  (void)renderer;
  (void)fontId;
  (void)familyName;
  return false;
#endif
}

bool SdCardFontSystem::reloadActiveScalableFamily(GfxRenderer& renderer, const char* familyName) {
#if CROSSINK_SCALABLE_FONTS
  ScalableFontAccess access;
  if (!hasResidentScalableFamily(familyName)) return false;
  const auto options = ttfRenderOptions(TTF_RENDER_PROFILES.profileFor(familyName));
  if (!manager_.setScalableRenderOptions(renderer, options)) return false;
  scalableRenderOptionsGeneration_++;
  return true;
#else
  (void)renderer;
  (void)familyName;
  return false;
#endif
}

void SdCardFontSystem::releaseLoadedFont(GfxRenderer& renderer) {
#if CROSSINK_SCALABLE_FONTS
  ScalableFontAccess access;
#endif
  if (manager_.currentFamilyName().empty()) return;

  const std::string familyName = manager_.currentFamilyName();
  (void)familyName;
  manager_.unloadAll(renderer);
  loadedFontPointSize_ = 0;
  LOG_DBG("SDFS", "Released SD card font before low-memory operation: %s", familyName.c_str());
}

void SdCardFontSystem::ensureRegistry() {
  const bool dirty = registryDirty_.exchange(false, std::memory_order_acq_rel);
  if (dirty) fontReloadPending_ = true;
  if (registryLoaded_ && !dirty && !registry_.needsRefresh()) return;
  if (dirty) LOG_DBG("SDFS", "Registry dirty — re-discovering fonts");
  registry_.loadNames();
  if (registry_.lastDiscoveryFailed()) {
    LOG_ERR("SDFS", "SD font registry scan failed (free=%u maxAlloc=%u)", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    registryDirty_.store(true, std::memory_order_release);
    return;
  }
  registryLoaded_ = true;
}

void SdCardFontSystem::releaseRegistry() {
  // FontInstaller can populate the shared registry directly.
  LOG_DBG("SDFS", "Releasing SD font catalog (%d families)", registry_.getFamilyCount());
  registry_.clear();
  registryLoaded_ = false;
}

void SdCardFontSystem::releaseForNetwork(GfxRenderer& renderer) {
#if CROSSINK_SCALABLE_FONTS
  ScalableFontAccess access;
#endif
  releaseLoadedFont(renderer);
  // UI fallback is shared by translated network screens too. It uses the
  // existing S3/PSRAM font budget; storage-ownership flows suspend it explicitly.

  releaseRegistry();
  registryDirty_.store(true, std::memory_order_release);
}

int SdCardFontSystem::resolveFontId(const char* familyName, uint8_t /*pointSize*/) const {
  // Return the active body-size identity; optional content sizes are selected
  // internally by the renderer and must not change the saved reader size.
  // ensureLoaded() must have been called with the current settings before this.
  return manager_.getFontId(familyName);
}

bool SdCardFontSystem::changeReaderFontSize(const bool larger, const FontSizeStepMode mode) {
  if (SETTINGS.sdFontFamilyName[0] != '\0') {
    refreshIfDirty();
    if (registry_.lastDiscoveryFailed()) return false;
    const auto* family = registry_.findFamily(SETTINGS.sdFontFamilyName);
    if (family) {
      if (!family->ensureDetails()) return false;
      const auto sizes = family->availableSizes();
      if (changeReaderFontSizeStep(sizes.data(), sizes.size(), SETTINGS.readerFontPointSize, larger, mode)) return true;
      if (sizes.size() > 0) return false;
    }
  }

  return SETTINGS.changeReaderFontSize(larger, mode);
}

uint8_t SdCardFontSystem::resolveLegacySizeStep(const char* familyName, const uint8_t sizeStep) {
  ensureRegistry();
  const auto* family = familyName ? registry_.findFamily(familyName) : nullptr;
  if (family) {
    const auto sizes = family->availableSizes();
    if (!sizes.empty()) return sizes[std::min<uint8_t>(sizeStep, sizes.size() - 1)];
  }
  return CrossPointSettings::getSdFontRangePointSize(SETTINGS.sdFontSizeRange, sizeStep);
}

void SdCardFontSystem::markRegistryDirtyForPath(const char* path) {
  filenameFontSystem.invalidateForPath(path);
  if (!path) return;
  for (const char* root : {SdCardFontRegistry::FONTS_DIR_HIDDEN, SdCardFontRegistry::FONTS_DIR_VISIBLE}) {
    const size_t length = std::strlen(root);
    if (strncasecmp(path, root, length) == 0 && (path[length] == '/' || path[length] == '\0')) {
      markRegistryDirty();
      return;
    }
  }
}
