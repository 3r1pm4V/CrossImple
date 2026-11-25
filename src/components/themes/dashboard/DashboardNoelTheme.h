#pragma once

#if defined(CROSSINK_NOEL_EDITION)

#include <cstdint>

#include "DashboardTheme.h"

namespace DashboardNoelMetrics {
constexpr ThemeMetrics makeValues() { return DashboardMetrics::values; }

constexpr ThemeMetrics values = makeValues();
}  // namespace DashboardNoelMetrics

class DashboardNoelTheme : public DashboardTheme {
 public:
  void drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle = nullptr,
                  bool readerContext = false, bool showStatus = true, bool filenameTitle = false) const override;
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           const std::function<bool()>& storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f, const GlobalReadingStats* globalStats = nullptr,
                           const char* currentChapterTitle = nullptr) const override;
  void drawSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                       const GlobalReadingStats* globalStats, float progressPercent = -1.0f,
                       const char* currentChapterTitle = nullptr, bool inverted = false) const;
};

#endif  // CROSSINK_NOEL_EDITION
