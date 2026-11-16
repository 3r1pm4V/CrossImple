#pragma once

#include <HalDisplay.h>

#include <cstdint>

#include "components/themes/minimal/MinimalTheme.h"

namespace DesktopMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = MinimalMetrics::values;
  v.homeTopPadding = 50;
  v.homeCoverHeight = 445;
  v.homeCoverTileHeight = 690;
  v.homeRecentBooksCount = 1;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 0;
  return v;
}

constexpr ThemeMetrics values = makeValues();
constexpr int homeCoverImageWidth = 78;
constexpr int homeCoverImageHeight = 114;
}  // namespace DesktopMetrics

class DesktopTheme : public MinimalTheme {
 public:
  static void setContinueCardFocused(bool focused);
  static bool hasPendingWallpaperGrayscale();
  static void displayHomeWithGrayscale(GfxRenderer& renderer, HalDisplay::RefreshMode refreshMode, Rect rect);
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           const std::function<bool()>& storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f, const GlobalReadingStats* globalStats = nullptr,
                           const char* currentChapterTitle = nullptr) const override;
};
