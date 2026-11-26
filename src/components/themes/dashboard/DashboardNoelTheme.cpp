#include "DashboardNoelTheme.h"

#if defined(CROSSINK_NOEL_EDITION)

#include <Bitmap.h>
#include <Epub.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalGPIO.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Xtc.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "activities/reader/BookReadingStats.h"
#include "activities/reader/BookStatsTracking.h"
#include "activities/reader/GlobalReadingStats.h"
#include "activities/reader/ReadingStatsUtils.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/afternoon.h"
#include "components/icons/book24.h"
#include "components/icons/evening.h"
#include "components/icons/morning.h"
#include "components/icons/night.h"
#include "components/icons/streak.h"
#include "fontIds.h"
#include "util/ReaderStatusBarConfig.h"

namespace {
// Layout constants
constexpr int kContentInsetX4 = 20;
constexpr int kContentInsetX3 = 75;
constexpr int kTopInset = 16;
constexpr int kCoverCornerRadius = 8;
constexpr int kStatsColumnWidth = 105;
constexpr int kStatsColumnWidthWide = 120;
constexpr int kCoverStatsGap = 15;
constexpr int kPairInwardShiftX3 = 15;
constexpr int kTitleTopGap = 18;
constexpr int kTitleChapterGap = 6;
constexpr int kBookTitleMaxLines = 2;
constexpr int kBookChapterMaxLines = 2;
constexpr int kStatsRowCount = 7;
constexpr int kStatsRowCountX4 = 6;
constexpr int kStatsValueLabelGap = 1;
constexpr int kFooterIconSize = 24;
constexpr int kFooterIconTextGap = 8;

bool isWideScreen(const GfxRenderer& renderer) { return renderer.getScreenWidth() >= 560; }

int contentInset(const GfxRenderer& renderer) {
  const auto edges = renderer.getViewableInsets().rotated(static_cast<unsigned>(renderer.getOrientation())).edges;
  return std::max(isWideScreen(renderer) ? kContentInsetX3 : kContentInsetX4,
                  static_cast<int>(std::max(edges[1], edges[3])));
}

Rect coverRectForScreen(const GfxRenderer& renderer, const Rect& rect) {
  const int inset = contentInset(renderer);
  const int statsW = isWideScreen(renderer) ? kStatsColumnWidthWide : kStatsColumnWidth;
  const int maxCoverW = renderer.getScreenWidth() - inset * 2 - statsW - kCoverStatsGap;
  const int coverW = std::min(DashboardMetrics::homeCoverImageWidth, maxCoverW);
  const int coverH = std::min(DashboardMetrics::homeCoverImageHeight, (coverW * 3) / 2);
  return Rect{inset + (gpio.deviceIsX3() ? kPairInwardShiftX3 : 0), rect.y + kTopInset, coverW, coverH};
}

Rect fittedBitmapRect(const Bitmap& bitmap, const Rect& target) {
  if (bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0 || target.width <= 0 || target.height <= 0) {
    return target;
  }
  const float widthScale = static_cast<float>(target.width) / static_cast<float>(bitmap.getWidth());
  const float heightScale = static_cast<float>(target.height) / static_cast<float>(bitmap.getHeight());
  const float scale = std::min(1.0f, std::min(widthScale, heightScale));
  const int drawnW = std::min(target.width, std::max(1, static_cast<int>(std::ceil(bitmap.getWidth() * scale))));
  const int drawnH = std::min(target.height, std::max(1, static_cast<int>(std::ceil(bitmap.getHeight() * scale))));
  return Rect{target.x + (target.width - drawnW) / 2, target.y + (target.height - drawnH) / 2, drawnW, drawnH};
}

std::string coverPathForRect(const RecentBook& book, const Rect& imageRect) {
  if (book.coverBmpPath.empty()) return {};
  if (FsHelpers::hasEpubExtension(book.path)) {
    const std::string adaptivePath =
        Epub(book.path, "/.crosspoint").getAdaptiveThumbBmpPath(imageRect.width, imageRect.height);
    if (Storage.exists(adaptivePath.c_str())) return adaptivePath;
  }
  return UITheme::getCoverThumbPath(book.coverBmpPath, imageRect.width, imageRect.height);
}

// Draw Fairy Lights Garland
void drawGarland(const GfxRenderer& renderer, int startX, int endX, int y, int swagsCount = 5, bool black = true) {
  if (endX <= startX || swagsCount <= 0) return;
  const float swagW = static_cast<float>(endX - startX) / swagsCount;
  for (int i = 0; i < swagsCount; ++i) {
    const int sx = startX + static_cast<int>(i * swagW);
    const int ex = startX + static_cast<int>((i + 1) * swagW);
    const int midX = (sx + ex) / 2;
    const int dropY = y + 5;

    renderer.drawLine(sx, y, midX, dropY, black);
    renderer.drawLine(midX, dropY, ex, y, black);

    renderer.fillRect(midX - 1, dropY, 3, 2, black);
    if (i % 2 == 0) {
      renderer.fillRect(midX - 2, dropY + 2, 4, 4, black);
    } else {
      renderer.drawRect(midX - 2, dropY + 2, 4, 4, black);
    }
  }
}

// Draw Pine Forest Border
void drawPineForest(const GfxRenderer& renderer, int startX, int endX, int groundY, bool black = true) {
  if (endX <= startX) return;
  const int treeW = 16;
  const int treeH = 18;
  const int numTrees = std::max(1, (endX - startX) / treeW);
  const int step = (endX - startX) / numTrees;

  renderer.drawLine(startX, groundY, endX, groundY, black);

  for (int i = 0; i < numTrees; ++i) {
    const int tx = startX + i * step + step / 2;
    const bool isSolid = (i % 2 == 0);

    // Tier 1 (Top)
    const int topX[3] = {tx, tx - 4, tx + 4};
    const int topY[3] = {groundY - treeH, groundY - treeH + 6, groundY - treeH + 6};
    if (isSolid) {
      renderer.fillPolygon(topX, topY, 3, black);
    } else {
      renderer.drawLine(topX[0], topY[0], topX[1], topY[1], black);
      renderer.drawLine(topX[1], topY[1], topX[2], topY[2], black);
      renderer.drawLine(topX[2], topY[2], topX[0], topY[0], black);
    }

    // Tier 2 (Mid)
    const int midX[3] = {tx, tx - 6, tx + 6};
    const int midY[3] = {groundY - treeH + 5, groundY - treeH + 11, groundY - treeH + 11};
    if (isSolid) {
      renderer.fillPolygon(midX, midY, 3, black);
    } else {
      renderer.drawLine(midX[0], midY[0], midX[1], midY[1], black);
      renderer.drawLine(midX[1], midY[1], midX[2], midY[2], black);
      renderer.drawLine(midX[2], midY[2], midX[0], midY[0], black);
    }

    // Tier 3 (Bottom)
    const int botX[3] = {tx, tx - 8, tx + 8};
    const int botY[3] = {groundY - treeH + 9, groundY, groundY};
    if (isSolid) {
      renderer.fillPolygon(botX, botY, 3, black);
    } else {
      renderer.drawLine(botX[0], botY[0], botX[1], botY[1], black);
      renderer.drawLine(botX[1], botY[1], botX[2], botY[2], black);
      renderer.drawLine(botX[2], botY[2], botX[0], botY[0], black);
    }
  }
}

// Festive Corner Ribbon on Book Cover (Top Right)
void drawFestiveCornerRibbon(const GfxRenderer& renderer, const Rect& coverRect, bool black = true) {
  const int rw = 32;
  const int rh = 32;
  const int rx = coverRect.x + coverRect.width - rw;
  const int ry = coverRect.y;

  const int px[4] = {rx, rx + 12, coverRect.x + coverRect.width, coverRect.x + coverRect.width};
  const int py[4] = {ry, ry, ry + rh, ry + rh - 12};
  renderer.fillPolygon(px, py, 4, black);

  for (int d = 6; d < rw + rh - 10; d += 4) {
    const int sx = rx + d / 2;
    const int sy = ry + d / 2;
    renderer.drawPixel(sx, sy, !black);
  }
}

void drawMissingBookCoverNoel(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book) {
  renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                           Color::White);
  renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);

  drawFestiveCornerRibbon(renderer, coverRect, true);

  const int centerX = coverRect.x + coverRect.width / 2;
  const int centerY = coverRect.y + coverRect.height / 2;
  const int radius = std::min(coverRect.width, coverRect.height) / 4;

  renderer.drawArc(radius, centerX, centerY, 1, 1, 1, true);
  renderer.drawArc(radius, centerX, centerY, -1, 1, 1, true);
  renderer.drawArc(radius, centerX, centerY, 1, -1, 1, true);
  renderer.drawArc(radius, centerX, centerY, -1, -1, 1, true);

  const int tx[3] = {centerX, centerX - 12, centerX + 12};
  const int ty[3] = {centerY - 16, centerY + 8, centerY + 8};
  renderer.fillPolygon(tx, ty, 3, true);
  renderer.fillRect(centerX - 2, centerY + 8, 4, 6, true);

  constexpr int textPadding = 14;
  const int textW = coverRect.width - textPadding * 2;
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(renderer.filenameFontId(UI_12_FONT_ID), title, textW, 2, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(renderer.filenameFontId(UI_12_FONT_ID));
  int textY = coverRect.y + 24;
  for (const auto& line : titleLines) {
    const int lineW = renderer.getTextWidth(renderer.filenameFontId(UI_12_FONT_ID), line.c_str(), EpdFontFamily::BOLD);
    renderer.drawText(renderer.filenameFontId(UI_12_FONT_ID), coverRect.x + (coverRect.width - lineW) / 2, textY,
                      line.c_str(), true, EpdFontFamily::BOLD);
    textY += lineH;
  }

  const char* tag = tr(STR_THEME_CHRISTMAS_EDITION);
  const int tagW = renderer.getTextWidth(SMALL_FONT_ID, tag);
  renderer.drawText(SMALL_FONT_ID, coverRect.x + (coverRect.width - tagW) / 2, coverRect.y + coverRect.height - 20, tag,
                    true);
}

void drawBookCoverNoel(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book,
                       const Color backgroundColor) {
  bool hasCover = false;
  const std::string coverBmpPath = coverPathForRect(book, coverRect);
  if (!coverBmpPath.empty() && Storage.exists(coverBmpPath.c_str())) {
    FsFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const Rect bitmapRect = fittedBitmapRect(bitmap, coverRect);
        renderer.fillRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, kCoverCornerRadius,
                                 backgroundColor);
        renderer.fillRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, kCoverCornerRadius,
                                 Color::White);
        renderer.drawBitmap(bitmap, bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height);
        renderer.maskRoundedRectOutsideCorners(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height,
                                               kCoverCornerRadius, backgroundColor);
        renderer.drawRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, 1, kCoverCornerRadius,
                                 true);
        drawFestiveCornerRibbon(renderer, bitmapRect, true);
        hasCover = true;
      }
      file.close();
    }
  }

  if (!hasCover) {
    drawMissingBookCoverNoel(renderer, coverRect, book);
  }
}

void drawRightAlignedText(const GfxRenderer& renderer, const int fontId, const int rightX, const int y,
                          const char* text, const bool bold = false, const bool black = true) {
  const EpdFontFamily::Style style = bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;
  const int width = renderer.getTextWidth(fontId, text, style);
  renderer.drawText(fontId, rightX - width, y, text, black, style);
}

void formatCompactDuration(const uint32_t seconds, char* buf, const size_t len) {
  if (seconds < 60) {
    snprintf(buf, len, "< 1m");
    return;
  }
  const uint32_t minutes = (seconds + 30u) / 60u;
  if (minutes < 60) {
    snprintf(buf, len, "%lu min", static_cast<unsigned long>(minutes));
    return;
  }
  const uint32_t hours = minutes / 60u;
  const uint32_t remainder = minutes % 60u;
  if (remainder == 0) {
    snprintf(buf, len, "%luh", static_cast<unsigned long>(hours));
  } else {
    snprintf(buf, len, "%luh %lum", static_cast<unsigned long>(hours), static_cast<unsigned long>(remainder));
  }
}

bool fallbackEstimatedTimeLeft(const BookReadingStats& stats, const float progressPercent, uint32_t& seconds) {
  seconds = 0;
  if (progressPercent <= 0.0f || progressPercent >= 100.0f || stats.totalReadingSeconds < 120) {
    return false;
  }
  const float progress = progressPercent / 100.0f;
  const float estimate = (static_cast<float>(stats.totalReadingSeconds) * (1.0f - progress)) / progress;
  if (estimate <= 0.0f) {
    return false;
  }
  seconds = static_cast<uint32_t>(estimate + 0.5f);
  return seconds > 0;
}

bool estimatedTimeLeft(const BookReadingStats& stats, const float progressPercent, uint32_t& seconds) {
  if (stats.estimatedTimeLeftSeconds > 0) {
    seconds = stats.estimatedTimeLeftSeconds;
    return true;
  }
  return fallbackEstimatedTimeLeft(stats, progressPercent, seconds);
}

bool showBookStatsForPath(const std::string& path) {
  if (!SETTINGS.shouldTrackReadingStats()) return false;
  if (FsHelpers::hasEpubExtension(path))
    return BookStatsTracking::isBookEnabled(Epub::cachePathForFilePath(path, "/.crosspoint"));
  if (FsHelpers::hasXtcExtension(path))
    return BookStatsTracking::isBookEnabled(Xtc(path, "/.crosspoint").getCachePath());
  return false;
}

int statsBlockHeight(const GfxRenderer& renderer) {
  const int valueLineH = renderer.getLineHeight(UI_12_FONT_ID);
  const int labelLineH = renderer.getLineHeight(SMALL_FONT_ID);
  return valueLineH + kStatsValueLabelGap + labelLineH;
}

int statsBlockTop(const Rect& coverRect, const int index, const int blockH, const int rowCount) {
  const int remainingH = std::max(0, coverRect.height - blockH * rowCount);
  const int gapCount = rowCount - 1;
  const int gap = gapCount > 0 ? remainingH / gapCount : 0;
  const int remainder = gapCount > 0 ? remainingH % gapCount : 0;
  return coverRect.y + index * (blockH + gap) + std::min(index, remainder);
}

void drawStatsRowNoel(const GfxRenderer& renderer, const int rightX, const int y, const char* value, const char* label,
                      const bool black = true, const bool drawDivider = true, const int colW = 105) {
  const int valueLineH = renderer.getLineHeight(UI_12_FONT_ID);
  drawRightAlignedText(renderer, UI_12_FONT_ID, rightX, y, value, true, black);
  drawRightAlignedText(renderer, SMALL_FONT_ID, rightX, y + valueLineH + kStatsValueLabelGap, label, false, black);

  if (drawDivider) {
    const int divY = y + valueLineH + kStatsValueLabelGap + renderer.getLineHeight(SMALL_FONT_ID) + 3;
    for (int dx = rightX - colW; dx <= rightX; dx += 4) {
      renderer.drawPixel(dx, divY, black);
    }
  }
}

void drawDashboardStatsNoel(const GfxRenderer& renderer, const Rect& coverRect, const BookReadingStats* stats,
                            const float progressPercent, const bool showStats, const bool black = true) {
  const int rightX = renderer.getScreenWidth() - contentInset(renderer) - (gpio.deviceIsX3() ? kPairInwardShiftX3 : 0);
  const int colW = isWideScreen(renderer) ? kStatsColumnWidthWide : kStatsColumnWidth;
  const int blockH = statsBlockHeight(renderer);
  const bool showRtcStats = halClock.isAvailable();
  const int rowCount = showRtcStats ? kStatsRowCount : kStatsRowCountX4;
  const BookReadingStats emptyStats{};
  const BookReadingStats& bookStats = stats != nullptr ? *stats : emptyStats;
  char value[40];
  char label[40];
  char startedDate[24];
  uint32_t estimatedSeconds = 0;
  const bool hasEstimate = !bookStats.isCompleted && estimatedTimeLeft(bookStats, progressPercent, estimatedSeconds);
  if (!showStats) {
    if (hasEstimate) {
      formatCompactDuration(estimatedSeconds, value, sizeof(value));
      drawStatsRowNoel(renderer, rightX, coverRect.y + coverRect.height / 2 - blockH, value, tr(STR_TIME_LEFT_SHORT),
                       black, false, colW);
    }
    if (progressPercent >= 0.0f) {
      snprintf(value, sizeof(value), "%d%%", static_cast<int>(progressPercent + 0.5f));
      const int progressY = hasEstimate ? coverRect.y + coverRect.height / 2 + blockH : coverRect.y;
      drawStatsRowNoel(renderer, rightX, progressY, value, tr(STR_STATS_PROGRESS_LBL), black, false, colW);
    }
    return;
  }

  int rowIndex = 0;
  int rowY = statsBlockTop(coverRect, rowIndex, blockH, rowCount);
  BookReadingStats::formatDuration(bookStats.totalReadingSeconds, value, sizeof(value));
  drawStatsRowNoel(renderer, rightX, rowY, value, tr(STR_STATS_TIME_LBL), black, true, colW);

  rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
  if (hasEstimate) {
    formatCompactDuration(estimatedSeconds, value, sizeof(value));
  } else {
    snprintf(value, sizeof(value), "-");
  }
  drawStatsRowNoel(renderer, rightX, rowY, value, tr(STR_TIME_LEFT_SHORT), black, true, colW);

  rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
  if (progressPercent >= 0.0f) {
    snprintf(value, sizeof(value), "%d%%", static_cast<int>(progressPercent + 0.5f));
  } else {
    snprintf(value, sizeof(value), "-");
  }
  drawStatsRowNoel(renderer, rightX, rowY, value, tr(STR_STATS_PROGRESS_LBL), black, true, colW);

  if (showRtcStats) {
    rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
    ReadingStatsDateTime today;
    if (getCurrentLocalReadingStatsDateTime(today) && bookStats.startDate.isValid()) {
      const uint16_t days = std::max<uint16_t>(1, readingSpanDaysElapsed(bookStats.startDate, today.date));
      BookReadingStats::formatDuration(bookStats.totalReadingSeconds / days, value, sizeof(value));
    } else {
      snprintf(value, sizeof(value), "-");
    }
    drawStatsRowNoel(renderer, rightX, rowY, value, tr(STR_STATS_DAILY_AVG_LBL), black, true, colW);
  }

  rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
  const float ppm = (bookStats.totalReadingSeconds > 60)
                        ? (static_cast<float>(bookStats.totalPagesTurned) * 60.0f / bookStats.totalReadingSeconds)
                        : 0.0f;
  snprintf(value, sizeof(value), "%.1f", ppm);
  drawStatsRowNoel(renderer, rightX, rowY, value, tr(STR_STATS_PAGES_PER_MIN), black, true, colW);

  rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
  formatReadingStatsShortDate(bookStats.startDate, startedDate, sizeof(startedDate));
  snprintf(label, sizeof(label), tr(STR_STATS_STARTED_FORMAT), startedDate[0] != '\0' ? startedDate : "--");
  snprintf(value, sizeof(value), "%u days", static_cast<unsigned>(bookStats.sessionCount));
  drawStatsRowNoel(renderer, rightX, rowY, value, label, black, true, colW);

  rowY = statsBlockTop(coverRect, ++rowIndex, blockH, rowCount);
  drawStatsRowNoel(renderer, rightX, rowY, tr(STR_CHRISTMAS_EVE_SHORT), tr(STR_FINISH_BEFORE_CHRISTMAS), black, false,
                   colW);
}

int drawBookTextNoel(const GfxRenderer& renderer, const Rect& coverRect, const RecentBook& book,
                     const char* currentChapterTitle, const bool black = true) {
  const int inset = contentInset(renderer);
  const int textW = renderer.getScreenWidth() - inset * 2;
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines = renderer.wrappedText(renderer.filenameFontId(UI_12_FONT_ID), title, textW, kBookTitleMaxLines,
                                         EpdFontFamily::BOLD);
  int textY = coverRect.y + coverRect.height + kTitleTopGap;
  const int titleLineH = renderer.getLineHeight(renderer.filenameFontId(UI_12_FONT_ID));
  for (const auto& line : titleLines) {
    renderer.drawText(renderer.filenameFontId(UI_12_FONT_ID), coverRect.x, textY, line.c_str(), black,
                      EpdFontFamily::BOLD);
    textY += titleLineH;
  }

  const char* subtitle =
      (currentChapterTitle != nullptr && currentChapterTitle[0] != '\0') ? currentChapterTitle : book.author.c_str();
  if (subtitle != nullptr && subtitle[0] != '\0') {
    auto subtitleLines =
        renderer.wrappedText(renderer.filenameFontId(UI_12_FONT_ID), subtitle, textW, kBookChapterMaxLines);
    int subtitleY = textY + kTitleChapterGap;
    for (const auto& line : subtitleLines) {
      renderer.drawText(renderer.filenameFontId(UI_12_FONT_ID), coverRect.x, subtitleY, line.c_str(), black);
      subtitleY += titleLineH;
    }
    return subtitleY;
  }
  return textY;
}

bool dominantReaderTypeBucket(const GlobalReadingStats& globalStats, ReadingTimeBucket& bucketOut) {
  const auto& values = globalStats.timeOfDaySeconds;
  const uint32_t totalSeconds = std::accumulate(values.begin(), values.end(), 0u);
  if (totalSeconds == 0) {
    return false;
  }
  const size_t dominantIndex =
      static_cast<size_t>(std::distance(values.begin(), std::max_element(values.begin(), values.end())));
  bucketOut = static_cast<ReadingTimeBucket>(dominantIndex);
  return true;
}

const char* readerTypeLabel(const GlobalReadingStats* globalStats) {
  if (globalStats == nullptr) {
    return tr(STR_STATS_NEW_READER);
  }
  ReadingTimeBucket bucket = ReadingTimeBucket::Night;
  if (!dominantReaderTypeBucket(*globalStats, bucket)) {
    return tr(STR_STATS_NEW_READER);
  }
  switch (bucket) {
    case ReadingTimeBucket::Morning:
      return tr(STR_STATS_MORNING_READER);
    case ReadingTimeBucket::Afternoon:
      return tr(STR_STATS_AFTERNOON_READER);
    case ReadingTimeBucket::Evening:
      return tr(STR_STATS_EVENING_READER);
    case ReadingTimeBucket::Night:
    default:
      return tr(STR_STATS_NIGHT_READER);
  }
}

const uint8_t* readerTypeIcon(const GlobalReadingStats* globalStats) {
  if (globalStats == nullptr) {
    return Book24Icon;
  }
  ReadingTimeBucket bucket = ReadingTimeBucket::Night;
  if (!dominantReaderTypeBucket(*globalStats, bucket)) {
    return Book24Icon;
  }
  switch (bucket) {
    case ReadingTimeBucket::Morning:
      return MorningReaderIcon;
    case ReadingTimeBucket::Afternoon:
      return AfternoonReaderIcon;
    case ReadingTimeBucket::Evening:
      return EveningReaderIcon;
    case ReadingTimeBucket::Night:
    default:
      return NightReaderIcon;
  }
}

void formatStreakStat(const GlobalReadingStats* globalStats, char* buf, const size_t len) {
  if (len == 0) return;
  if (globalStats == nullptr) {
    snprintf(buf, len, "%s", tr(STR_STATS_NO_STREAK));
    return;
  }
  ReadingStatsDateTime today;
  const uint16_t streak =
      getCurrentLocalReadingStatsDateTime(today) ? globalStats->currentReadingStreak(&today.date) : 0;
  if (streak == 0) {
    snprintf(buf, len, "%s", tr(STR_STATS_NO_STREAK));
    return;
  }
  snprintf(buf, len, tr(STR_STATS_DAY_STREAK_FORMAT), static_cast<unsigned>(streak));
}

void drawIconLabel(const GfxRenderer& renderer, const uint8_t* icon, const int iconX, const int centerY,
                   const char* label, const int maxTextW, const bool inverted = false) {
  const std::string visibleLabel = renderer.truncatedText(UI_10_FONT_ID, label, maxTextW);
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
  if (inverted) {
    renderer.drawIconInverted(icon, iconX, centerY - kFooterIconSize / 2, kFooterIconSize, kFooterIconSize);
  } else {
    renderer.drawIcon(icon, iconX, centerY - kFooterIconSize / 2, kFooterIconSize, kFooterIconSize);
  }
  renderer.drawText(UI_10_FONT_ID, iconX + kFooterIconSize + kFooterIconTextGap, centerY - lineH / 2,
                    visibleLabel.c_str(), !inverted);
}

void drawRightAlignedIconLabel(const GfxRenderer& renderer, const uint8_t* icon, const int rightX, const int centerY,
                               const char* label, const int maxTextW, const bool inverted = false) {
  const std::string visibleLabel = renderer.truncatedText(UI_10_FONT_ID, label, maxTextW);
  const int labelW = renderer.getTextWidth(UI_10_FONT_ID, visibleLabel.c_str());
  const int lineH = renderer.getLineHeight(UI_10_FONT_ID);
  const int textX = rightX - labelW;
  const int iconX = textX - kFooterIconTextGap - kFooterIconSize;
  if (inverted) {
    renderer.drawIconInverted(icon, iconX, centerY - kFooterIconSize / 2, kFooterIconSize, kFooterIconSize);
  } else {
    renderer.drawIcon(icon, iconX, centerY - kFooterIconSize / 2, kFooterIconSize, kFooterIconSize);
  }
  renderer.drawText(UI_10_FONT_ID, textX, centerY - lineH / 2, visibleLabel.c_str(), !inverted);
}

void drawFooterStatsNoel(const GfxRenderer& renderer, const Rect& coverRect, const GlobalReadingStats* globalStats,
                         const int centerY, const bool inverted = false) {
  const int inset = contentInset(renderer) + (gpio.deviceIsX3() ? kPairInwardShiftX3 : 0);
  const int W = renderer.getScreenWidth();

  // Streak on Left
  char streakBuf[48];
  formatStreakStat(globalStats, streakBuf, sizeof(streakBuf));
  const int maxTextW = (W - inset * 2) / 2 - kFooterIconSize - kFooterIconTextGap;
  drawIconLabel(renderer, StreakIcon, inset, centerY, streakBuf, maxTextW, inverted);

  // Reader type on Right
  const char* readerLabel = readerTypeLabel(globalStats);
  const uint8_t* icon = readerTypeIcon(globalStats);
  drawRightAlignedIconLabel(renderer, icon, W - inset, centerY, readerLabel, maxTextW, inverted);
}
}  // namespace

void DashboardNoelTheme::drawHeader(const GfxRenderer& renderer, Rect rect, const char* title, const char* subtitle,
                                    const bool readerContext, const bool showStatus, const bool filenameTitle) const {
  MinimalTheme::drawHeader(renderer, rect, title, subtitle, readerContext, showStatus, filenameTitle);
  if (!readerContext) {
    const char* headerText = tr(STR_CHRISTMAS_IS_COMING);
    const int fontId = UITheme::getDisplayStatusBarFontId();
    const int textW = renderer.getTextWidth(fontId, headerText, EpdFontFamily::BOLD);
    const int textX = (renderer.getScreenWidth() - textW) / 2;
    const int textY = UITheme::getTopStatusBarY(renderer) + ReaderStatusBarConfig::TOP_TEXT_INSET;
    renderer.fillRect(textX - 8, textY, textW + 16, renderer.getLineHeight(fontId) + 2, false);
    renderer.drawText(fontId, textX, textY, headerText, true, EpdFontFamily::BOLD);
  }
}

int footerCenterYForScreen(const GfxRenderer& renderer) {
  const int buttonReserve = UITheme::getButtonHintsReserve(renderer);
  const int buttonTop = renderer.getScreenHeight() - std::max(48, buttonReserve);
  return buttonTop - 54;
}

void drawNoelDecorativeElements(const GfxRenderer& renderer, const Rect& coverRect, const int textBottomY,
                                const int footerCenterY, const bool black = true) {
  const int W = renderer.getScreenWidth();
  const int inset = contentInset(renderer);
  const int footerTop = footerCenterY - 14;

  // Space between book text bottom and footer top
  const int spanTop = textBottomY;
  const int spanBottom = footerTop;
  const int totalSpan = std::max(0, spanBottom - spanTop);

  // Element heights:
  // - Countdown line: ~16px
  // - Mid Garland: ~8px
  // - Pine Forest: ~18px
  constexpr int kElementsTotalHeight = 42;
  const int availableGapSpace = std::max(0, totalSpan - kElementsTotalHeight);
  const int gap = std::clamp(availableGapSpace / 4, 10, 24);

  // 3A. Countdown line
  int daysUntilNoel = 32;
  ReadingStatsDateTime today;
  if (getCurrentLocalReadingStatsDateTime(today) && today.date.isValid()) {
    int targetYear = today.date.year;
    if (today.date.month == 12 && today.date.day > 25) {
      targetYear++;
    }
    ReadingStatsDate xmasDate{static_cast<uint16_t>(targetYear), 12, 25};
    daysUntilNoel = readingSpanDaysElapsed(today.date, xmasDate);
  }

  char countdownBuf[64];
  if (daysUntilNoel == 0) {
    snprintf(countdownBuf, sizeof(countdownBuf), "%s", tr(STR_CHRISTMAS_TODAY));
  } else if (daysUntilNoel == 1) {
    snprintf(countdownBuf, sizeof(countdownBuf), "%s", tr(STR_DAY_UNTIL_CHRISTMAS_FORMAT));
  } else {
    snprintf(countdownBuf, sizeof(countdownBuf), tr(STR_DAYS_UNTIL_CHRISTMAS_FORMAT),
             static_cast<unsigned>(daysUntilNoel));
  }

  const int countdownY = spanTop + gap;
  // Mini decorative tree icon vertically centered with countdown text
  const int iconCenterY = countdownY + 8;
  const int treeX = coverRect.x;
  const int treeW = 10;
  const int treeTopY = iconCenterY - 7;
  const int treeBaseY = iconCenterY + 4;

  // Top tier
  const int t1X[3] = {treeX + 5, treeX + 1, treeX + 9};
  const int t1Y[3] = {treeTopY, treeTopY + 6, treeTopY + 6};
  renderer.fillPolygon(t1X, t1Y, 3, black);

  // Bottom tier
  const int t2X[3] = {treeX + 5, treeX - 1, treeX + 11};
  const int t2Y[3] = {treeTopY + 4, treeBaseY, treeBaseY};
  renderer.fillPolygon(t2X, t2Y, 3, black);

  // Trunk
  renderer.fillRect(treeX + 4, treeBaseY, 3, 3, black);

  // Text
  renderer.drawText(UI_12_FONT_ID, treeX + treeW + 6, countdownY, countdownBuf, black, EpdFontFamily::BOLD);

  // 3B. Mid Garland (Cleanly below countdown text)
  const int midGarlandY = countdownY + 16 + gap;
  drawGarland(renderer, inset, W - inset, midGarlandY, isWideScreen(renderer) ? 7 : 5, black);

  // 3C. Pine Forest (Cleanly below garland, trees standing on pineGroundY)
  const int pineGroundY = midGarlandY + 8 + gap + 18;
  drawPineForest(renderer, inset, W - inset, pineGroundY, black);
}

void DashboardNoelTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect,
                                             const std::vector<RecentBook>& recentBooks, int selectorIndex,
                                             bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                                             const std::function<bool()>& storeCoverBuffer,
                                             const BookReadingStats* stats, const float progressPercent,
                                             const GlobalReadingStats* globalStats,
                                             const char* currentChapterTitle) const {
  (void)selectorIndex;
  (void)bufferRestored;

  const int W = renderer.getScreenWidth();
  const int inset = contentInset(renderer) + (gpio.deviceIsX3() ? kPairInwardShiftX3 : 0);

  // 1. TOP FAIRY LIGHTS GARLAND (Placed just below status bar, above cover)
  const int topGarlandY = std::max(28, rect.y - 12);
  drawGarland(renderer, inset, W - inset, topGarlandY, isWideScreen(renderer) ? 7 : 5, true);

  // 2. BOOK COVER & STATS
  const Rect coverRect = coverRectForScreen(renderer, rect);
  if (recentBooks.empty()) {
    renderer.drawRoundedRect(coverRect.x, coverRect.y, coverRect.width, coverRect.height, 1, kCoverCornerRadius, true);
    coverRendered = false;
    coverBufferStored = false;
    return;
  }

  if (!coverRendered) {
    drawBookCoverNoel(renderer, coverRect, recentBooks[0], Color::White);
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }
  TouchRegistry::getInstance().add(coverRect, 0, TouchRegistry::Cover);

  drawDashboardStatsNoel(renderer, coverRect, stats, progressPercent, showBookStatsForPath(recentBooks[0].path), true);
  const int textBottomY = drawBookTextNoel(renderer, coverRect, recentBooks[0], currentChapterTitle, true);

  // 3. BULLETPROOF BALANCED VERTICAL FLOW
  const int footerCenterY = footerCenterYForScreen(renderer);
  drawNoelDecorativeElements(renderer, coverRect, textBottomY, footerCenterY, true);

  // 4. Footer stats (Streak & Reader Type)
  if (SETTINGS.shouldTrackReadingStats()) {
    drawFooterStatsNoel(renderer, coverRect, globalStats, footerCenterY, false);
  }
}

void DashboardNoelTheme::drawSleepScreen(const GfxRenderer& renderer, const RecentBook& book,
                                         const BookReadingStats* stats, const GlobalReadingStats* globalStats,
                                         const float progressPercent, const char* currentChapterTitle,
                                         const bool inverted) const {
  renderer.clearScreen(inverted ? 0xFF : 0x00);
  const Rect contentRect{0, DashboardMetrics::values.homeTopPadding, renderer.getScreenWidth(),
                         DashboardMetrics::values.homeCoverTileHeight};
  const Rect coverRect = coverRectForScreen(renderer, contentRect);
  drawBookCoverNoel(renderer, coverRect, book, inverted ? Color::White : Color::Black);
  drawDashboardStatsNoel(renderer, coverRect, stats, progressPercent, showBookStatsForPath(book.path), inverted);
  const int textBottomY = drawBookTextNoel(renderer, coverRect, book, currentChapterTitle, inverted);

  const int footerCenterY = footerCenterYForScreen(renderer);
  drawNoelDecorativeElements(renderer, coverRect, textBottomY, footerCenterY, inverted);

  if (SETTINGS.shouldTrackReadingStats()) {
    drawFooterStatsNoel(renderer, coverRect, globalStats, footerCenterY, !inverted);
  }
}

#endif  // CROSSINK_NOEL_EDITION
