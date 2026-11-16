#include "DesktopTheme.h"
#include "DesktopStockWallpaper.h"

#include <Bitmap.h>
#include <BitmapHelpers.h>
#include <Epub.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalGPIO.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Xtc.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

#include "CrossPointSettings.h"
#include "RecentBooksStore.h"
#include "activities/reader/BookReadingStats.h"
#include "activities/reader/DailyReadingStats.h"
#include "activities/reader/GlobalReadingStats.h"
#include "activities/reader/ReadingStatsUtils.h"
#include "components/TouchRegistry.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"
#include "components/icons/book24.h"
#include "components/icons/settings.h"
#include "fontIds.h"

#ifndef SIMULATOR
#include <esp_system.h>
#endif

namespace {
constexpr int kCardRadius = 8;
constexpr int kCoverRadius = 6;
constexpr int kGap = 12;
constexpr int kStatsCardHeight = 196;
constexpr int kBookCardHeight = 140;
constexpr int kTouchFooterHeight = 52;
constexpr int kBookCardPad = 12;

bool isWideScreen(const GfxRenderer& renderer) { return renderer.getScreenWidth() >= 560; }

int contentInset(const GfxRenderer& renderer) {
  const auto edges = renderer.getViewableInsets().rotated(static_cast<unsigned>(renderer.getOrientation())).edges;
  return std::max(isWideScreen(renderer) ? 28 : 20, static_cast<int>(std::max(edges[1], edges[3])));
}

struct DesktopLayout {
  Rect touchMenuRect{};
  Rect wallpaperRect{};
  Rect statsRect{};
  Rect bookCardRect{};
  Rect bookThumbRect{};
  Rect footerRect{};
  bool hasTouchFooter = false;
};

DesktopLayout computeDesktopLayout(const GfxRenderer& renderer, const Rect& rect) {
  DesktopLayout out;
  const int inset = contentInset(renderer);
  const int contentW = rect.width - inset * 2;
  out.hasTouchFooter = gpio.hasTouch();
  const int touchFooterH = out.hasTouchFooter ? kTouchFooterHeight : 0;
  const int usableH = rect.height - touchFooterH;

  out.touchMenuRect = Rect{rect.x + inset + contentW - 24, rect.y + 6, 24, 24};
  const int bookY = rect.y + usableH - kBookCardHeight - 8;
  const int statsY = bookY - kStatsCardHeight - kGap;
  const int wallpaperY = out.hasTouchFooter ? (out.touchMenuRect.y + out.touchMenuRect.height + 6) : (rect.y + 8);
  const int wallpaperH = std::max(100, statsY - wallpaperY - 10);

  out.wallpaperRect = Rect{rect.x + inset, wallpaperY, contentW, wallpaperH};
  out.statsRect = Rect{rect.x + inset, statsY, contentW, kStatsCardHeight};
  out.bookCardRect = Rect{rect.x + inset, bookY, contentW, kBookCardHeight};
  out.bookThumbRect = Rect{out.bookCardRect.x + kBookCardPad,
                           out.bookCardRect.y + (out.bookCardRect.height - DesktopMetrics::homeCoverImageHeight) / 2,
                           DesktopMetrics::homeCoverImageWidth, DesktopMetrics::homeCoverImageHeight};
  out.footerRect = Rect{rect.x + inset, rect.y + usableH, contentW, touchFooterH};
  return out;
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
    if (Storage.exists(adaptivePath.c_str())) {
      return adaptivePath;
    }
  }
  return UITheme::getCoverThumbPath(book.coverBmpPath, imageRect.width, imageRect.height);
}

std::string resolveWallpaperDirectory() {
  if (Storage.exists("/wallpaper")) return "/wallpaper";
  if (Storage.exists("/Wallpaper")) return "/Wallpaper";
  return {};
}

// O(1) RAM reservoir sampling across /wallpaper/*.bmp
std::string pickWallpaperBmpPath() {
  static std::string cachedPath;
  if (!cachedPath.empty() && Storage.exists(cachedPath.c_str())) {
    return cachedPath;
  }
  cachedPath.clear();

  const std::string dirPath = resolveWallpaperDirectory();
  if (dirPath.empty()) return {};

  FsFile dir = Storage.open(dirPath.c_str());
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return {};
  }

  uint32_t seen = 0;
  char name[128];
  for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (!file.isDirectory()) {
      const size_t len = file.getName(name, sizeof(name));
      if (len > 0 && name[0] != '.' && FsHelpers::hasBmpExtension(name)) {
        ++seen;
#ifndef SIMULATOR
        const bool take = (esp_random() % seen) == 0;
#else
        const bool take = (seen == 1);
#endif
        if (take) {
          cachedPath = dirPath + "/" + name;
        }
      }
    }
    file.close();
  }
  dir.close();
  return cachedPath;
}

void drawWallpaperFallback(const GfxRenderer& renderer, const Rect& area, const bool forceOneBit = false) {
  renderer.fillRoundedRect(area.x, area.y, area.width, area.height, kCardRadius, Color::White);

  using namespace DesktopStockWallpaper;
  // Center-crop horizontally if target width is smaller than 482
  const int cropX = std::max(0, (kWidth - area.width) / 2);
  // Bottom-align vertically so dock and characters remain in frame
  const int cropY = std::max(0, kHeight - area.height);

  const auto renderMode = renderer.getRenderMode();
  const bool absolute = renderer.grayPlanesAreAbsolute();
  const bool isOneBit = forceOneBit || (SETTINGS.desktopWallpaperMode == CrossPointSettings::DESKTOP_WALLPAPER_1BIT);

  if (renderMode == GfxRenderer::BW && isOneBit) {
    // 1-bit fast path using pre-dithered 1-bit array
    for (int row = 0; row < area.height; ++row) {
      const int srcY = cropY + row;
      if (srcY >= kHeight) break;
      const uint8_t* rowData = &kBitmap1Bit[srcY * kStride1Bit];
      const int destY = area.y + row;

      for (int col = 0; col < area.width; ++col) {
        const int srcX = cropX + col;
        if (srcX >= kWidth) break;
        const uint8_t b = rowData[srcX >> 3];
        const bool isBlack = (b & (0x80 >> (srcX & 7))) == 0;
        if (isBlack) {
          renderer.drawPixel(area.x + col, destY, true);
        }
      }
    }
  } else {
    // 4-gray path (BW base pass or GRAYSCALE_LSB / GRAYSCALE_MSB pass)
    for (int row = 0; row < area.height; ++row) {
      const int srcY = cropY + row;
      if (srcY >= kHeight) break;
      const uint8_t* rowData = &kBitmap4Gray[srcY * kStride4Gray];
      const int destY = area.y + row;

      for (int col = 0; col < area.width; ++col) {
        const int srcX = cropX + col;
        if (srcX >= kWidth) break;
        const uint8_t byteVal = rowData[srcX >> 2];
        const uint8_t val = (byteVal >> (6 - ((srcX & 3) * 2))) & 0x3;

        if (renderMode == GfxRenderer::BW) {
          if (val < 2) {
            renderer.drawPixel(area.x + col, destY, true);
          }
        } else if (renderMode == GfxRenderer::GRAYSCALE_LSB || renderMode == GfxRenderer::GRAYSCALE_MSB) {
          const auto pixel = grayPlanePixel(val, renderMode == GfxRenderer::GRAYSCALE_MSB, absolute);
          if (pixel.write) {
            renderer.drawPixel(area.x + col, destY, pixel.black);
          }
        }
      }
    }
  }

  renderer.maskRoundedRectOutsideCorners(area.x, area.y, area.width, area.height, kCardRadius, Color::White);
}

bool pendingWallpaperGrayscale = false;
bool wallpaperHasGrayscaleCached = false;

void drawWallpaperArea(const GfxRenderer& renderer, const Rect& area, const bool recordGrayscale,
                       const bool forceOneBit = false) {
  const std::string wallpaperPath = pickWallpaperBmpPath();
  if (!wallpaperPath.empty()) {
    FsFile file;
    if (Storage.openFileForRead("HOME", wallpaperPath, file)) {
      const bool use4Gray =
          !forceOneBit && (SETTINGS.desktopWallpaperMode == CrossPointSettings::DESKTOP_WALLPAPER_4GRAY);
      Bitmap bitmap(file, true, ImageToneProfile::Even);
      bitmap.setOneBitOutput(!use4Gray);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        if (recordGrayscale) {
          wallpaperHasGrayscaleCached = bitmap.hasGreyscale();
          pendingWallpaperGrayscale = use4Gray && wallpaperHasGrayscaleCached;
        }
        Rect drawRect = area;
        float cropX = 0.0f;
        float cropY = 0.0f;
        if (bitmap.getWidth() >= area.width && bitmap.getHeight() >= area.height && area.width > 0 && area.height > 0) {
          const float bmpRatio = static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
          const float areaRatio = static_cast<float>(area.width) / static_cast<float>(area.height);
          if (bmpRatio > areaRatio) {
            cropX = 1.0f - (areaRatio / bmpRatio);
          } else if (bmpRatio < areaRatio) {
            cropY = 1.0f - (bmpRatio / areaRatio);
          }
        } else {
          drawRect = fittedBitmapRect(bitmap, area);
        }
        renderer.drawBitmap(bitmap, drawRect.x, drawRect.y, drawRect.width, drawRect.height, cropX, cropY);
        renderer.maskRoundedRectOutsideCorners(drawRect.x, drawRect.y, drawRect.width, drawRect.height, kCardRadius,
                                               Color::White);
        file.close();
        return;
      }
      file.close();
    }
  }
  const bool use4Gray =
      !forceOneBit && (SETTINGS.desktopWallpaperMode == CrossPointSettings::DESKTOP_WALLPAPER_4GRAY);
  if (recordGrayscale) {
    wallpaperHasGrayscaleCached = true;
    pendingWallpaperGrayscale = use4Gray;
  }
  drawWallpaperFallback(renderer, area, forceOneBit);
}

struct DesktopStatsSnapshot {
  bool hasRtc = false;
  uint32_t todayMinutes = 0;
  uint32_t weekMinutes = 0;
  uint32_t avgMinutesPerDay = 0;
  uint16_t streakDays = 0;
  std::array<uint32_t, 7> weekDayMinutes{};  // Monday = 0 .. Sunday = 6
};

DesktopStatsSnapshot collectDesktopStats(const GlobalReadingStats* globalStats) {
  DesktopStatsSnapshot out;
  ReadingStatsDateTime now{};
  if (!getCurrentLocalDailyReadingDateTime(now) || !now.date.isValid()) {
    return out;
  }
  out.hasRtc = true;

  const uint32_t todayIdx = readingStatsDayIndex(now.date);
  const uint8_t dow = readingStatsDayOfWeekIndex(now.date);  // Monday = 0
  const uint32_t mondayIdx = todayIdx - dow;

  uint32_t totalWeekSeconds = 0;
  for (uint8_t i = 0; i <= dow && i < 7; ++i) {
    uint32_t sec = 0;
    if (DailyReadingStats::secondsFor(mondayIdx + i, sec)) {
      out.weekDayMinutes[i] = sec / 60;
      totalWeekSeconds += sec;
    }
  }

  out.todayMinutes = out.weekDayMinutes[dow < 7 ? dow : 0];
  out.weekMinutes = totalWeekSeconds / 60;
  out.avgMinutesPerDay = out.weekMinutes / (dow + 1);

  if (globalStats) {
    ReadingStatsDateTime statsNow{};
    if (getCurrentLocalReadingStatsDateTime(statsNow)) {
      out.streakDays = globalStats->currentReadingStreak(&statsNow.date);
    }
  }
  return out;
}

void drawStatsCards(const GfxRenderer& renderer, const Rect& area, const GlobalReadingStats* globalStats) {
  const DesktopStatsSnapshot snap = collectDesktopStats(globalStats);
  const int cardW = (area.width - kGap) / 2;
  const Rect leftCard{area.x, area.y, cardW, area.height};
  const Rect rightCard{area.x + cardW + kGap, area.y, area.width - cardW - kGap, area.height};

  renderer.fillRoundedRect(leftCard.x, leftCard.y, leftCard.width, leftCard.height, kCardRadius, Color::White);
  renderer.drawRoundedRect(leftCard.x, leftCard.y, leftCard.width, leftCard.height, 1, kCardRadius, true);
  renderer.fillRoundedRect(rightCard.x, rightCard.y, rightCard.width, rightCard.height, kCardRadius, Color::White);
  renderer.drawRoundedRect(rightCard.x, rightCard.y, rightCard.width, rightCard.height, 1, kCardRadius, true);

  // Left card: Today's reading
  constexpr int pad = 14;
  char buf[48];
  renderer.drawText(UI_10_FONT_ID, leftCard.x + pad, leftCard.y + 18, "Today's reading", true);

  snprintf(buf, sizeof(buf), "%lu min", static_cast<unsigned long>(snap.todayMinutes));
  renderer.drawText(LEXENDDECA_16_FONT_ID, leftCard.x + pad, leftCard.y + 66, buf, true, EpdFontFamily::BOLD);

  if (snap.streakDays > 0) {
    snprintf(buf, sizeof(buf), tr(STR_STATS_DAY_STREAK_FORMAT), static_cast<unsigned>(snap.streakDays));
  } else {
    snprintf(buf, sizeof(buf), "%s", snap.hasRtc ? tr(STR_STATS_NO_STREAK) : "--");
  }
  renderer.drawText(UI_10_FONT_ID, leftCard.x + pad, leftCard.y + leftCard.height - 34, buf, true);

  // Right card: This week
  renderer.drawText(UI_10_FONT_ID, rightCard.x + pad, rightCard.y + 18, "This week", true);

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(snap.weekMinutes));
  const int numW = renderer.getTextWidth(LEXENDDECA_16_FONT_ID, buf, EpdFontFamily::BOLD);
  renderer.drawText(LEXENDDECA_16_FONT_ID, rightCard.x + pad, rightCard.y + 52, buf, true, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, rightCard.x + pad + numW + 5, rightCard.y + 62, "min", true);

  snprintf(buf, sizeof(buf), "Avg %lu min/day", static_cast<unsigned long>(snap.avgMinutesPerDay));
  renderer.drawText(SMALL_FONT_ID, rightCard.x + pad, rightCard.y + 96, buf, true);

  // 7 mini squares: M T W T F S S
  static constexpr const char* kDayLabels[7] = {"M", "T", "W", "T", "F", "S", "S"};
  constexpr int boxSize = 17;
  const int innerW = rightCard.width - pad * 2;
  const int step = std::max(boxSize + 2, (innerW - boxSize) / 6);
  const int boxesY = rightCard.y + rightCard.height - 50;
  const int labelsY = boxesY + boxSize + 6;

  for (int i = 0; i < 7; ++i) {
    const int bx = rightCard.x + pad + i * step;
    const uint32_t mins = snap.weekDayMinutes[i];
    if (mins >= 45) {
      renderer.fillRoundedRect(bx, boxesY, boxSize, boxSize, 3, Color::Black);
    } else if (mins >= 15) {
      renderer.fillRoundedRect(bx, boxesY, boxSize, boxSize, 3, Color::DarkGray);
      renderer.drawRoundedRect(bx, boxesY, boxSize, boxSize, 1, 3, true);
    } else if (mins > 0) {
      renderer.fillRoundedRect(bx, boxesY, boxSize, boxSize, 3, Color::LightGray);
      renderer.drawRoundedRect(bx, boxesY, boxSize, boxSize, 1, 3, true);
    } else {
      renderer.fillRoundedRect(bx, boxesY, boxSize, boxSize, 3, Color::White);
      renderer.drawRoundedRect(bx, boxesY, boxSize, boxSize, 1, 3, true);
    }

    const int lw = renderer.getTextWidth(SMALL_FONT_ID, kDayLabels[i]);
    renderer.drawText(SMALL_FONT_ID, bx + (boxSize - lw) / 2, labelsY, kDayLabels[i], true);
  }
}

bool continueCardFocused = false;

void drawBookThumbnail(const GfxRenderer& renderer, const Rect& thumbRect, const std::vector<RecentBook>& recentBooks) {
  if (recentBooks.empty()) {
    return;
  }
  const RecentBook& book = recentBooks[0];
  bool drewCover = false;
  const std::string coverBmpPath = coverPathForRect(book, thumbRect);
  if (!coverBmpPath.empty() && Storage.exists(coverBmpPath.c_str())) {
    FsFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const Rect bitmapRect = fittedBitmapRect(bitmap, thumbRect);
        renderer.drawBitmap(bitmap, bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height);
        renderer.maskRoundedRectOutsideCorners(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height,
                                               kCoverRadius, Color::White);
        renderer.drawRoundedRect(bitmapRect.x, bitmapRect.y, bitmapRect.width, bitmapRect.height, 1, kCoverRadius, true);
        drewCover = true;
      }
      file.close();
    }
  }
  if (!drewCover) {
    renderer.drawRoundedRect(thumbRect.x, thumbRect.y, thumbRect.width, thumbRect.height, 1, kCoverRadius, true);
    renderer.drawIcon(Book24Icon, thumbRect.x + (thumbRect.width - 24) / 2, thumbRect.y + (thumbRect.height - 24) / 2,
                      24, 24);
  }
}

void drawContinueReadingOverlay(const GfxRenderer& renderer, const Rect& cardRect, const Rect& thumbRect,
                                const std::vector<RecentBook>& recentBooks, const BookReadingStats* stats,
                                const float progressPercent, const bool registerTouch) {
  // Draw card border cleanly (1px normal, 2px when focused)
  renderer.drawRoundedRect(cardRect.x, cardRect.y, cardRect.width, cardRect.height, continueCardFocused ? 2 : 1,
                           kCardRadius, true);

  if (recentBooks.empty()) {
    const char* emptyMsg = tr(STR_NO_OPEN_BOOK);
    const int msgW = renderer.getTextWidth(UI_10_FONT_ID, emptyMsg);
    const int msgY = cardRect.y + (cardRect.height - renderer.getLineHeight(UI_10_FONT_ID)) / 2;
    renderer.drawText(UI_10_FONT_ID, cardRect.x + (cardRect.width - msgW) / 2, msgY, emptyMsg, true);
    return;
  }

  if (registerTouch) {
    TouchRegistry::getInstance().add(cardRect, 0, TouchRegistry::Cover);
  }

  const RecentBook& book = recentBooks[0];
  const int textX = thumbRect.x + thumbRect.width + 14;
  const int textW = cardRect.x + cardRect.width - kBookCardPad - textX;
  const char* title = book.title.empty() ? book.path.c_str() : book.title.c_str();
  auto titleLines =
      renderer.wrappedText(renderer.filenameFontId(UI_12_FONT_ID), title, textW, 2, EpdFontFamily::BOLD);
  const int lineH = renderer.getLineHeight(renderer.filenameFontId(UI_12_FONT_ID));
  int textY = cardRect.y + 14;
  for (const auto& line : titleLines) {
    renderer.drawText(renderer.filenameFontId(UI_12_FONT_ID), textX, textY, line.c_str(), true, EpdFontFamily::BOLD);
    textY += lineH;
  }

  const int pct = progressPercent >= 0.0f ? std::clamp(static_cast<int>(std::lround(progressPercent)), 0, 100) : 0;
  char readBuf[48];
  if (stats && stats->totalReadingSeconds >= 60) {
    char durBuf[20];
    formatCompactReadingDuration(stats->totalReadingSeconds, durBuf, sizeof(durBuf));
    snprintf(readBuf, sizeof(readBuf), "Read: %d%% - %s", pct, durBuf);
  } else {
    snprintf(readBuf, sizeof(readBuf), "Read: %d%%", pct);
  }
  const int barY = cardRect.y + cardRect.height - 20;
  const int readY = barY - renderer.getLineHeight(UI_10_FONT_ID) - 6;
  renderer.drawText(UI_10_FONT_ID, textX, readY, readBuf, true);

  constexpr int barH = 6;
  renderer.drawRoundedRect(textX, barY, textW, barH, 1, 3, true);
  const int fillW = (textW * pct) / 100;
  if (fillW > 2) {
    renderer.fillRoundedRect(textX, barY, fillW, barH, 3, Color::Black);
  }
}

void drawTouchMenuIcon(const GfxRenderer& renderer, const Rect& menuRect, const bool registerTouch) {
  const int menuY = menuRect.y + (menuRect.height - 16) / 2;
  for (int line = 0; line < 3; ++line) {
    renderer.fillRect(menuRect.x, menuY + line * 7, menuRect.width, 2, true);
  }
  if (registerTouch) {
    TouchRegistry::getInstance().add(Rect{menuRect.x - 8, menuRect.y - 4, menuRect.width + 16, menuRect.height + 8}, 0,
                                     TouchRegistry::Button);
  }
}

void drawTouchFooterIcons(const GfxRenderer& renderer, const Rect& footerRect, const bool registerTouch) {
  const int slotW = footerRect.width / 3;
  const int iconY24 = footerRect.y + (footerRect.height - 24) / 2;
  const int iconY32 = footerRect.y + (footerRect.height - 32) / 2;

  const int x0 = footerRect.x + (slotW - 24) / 2;
  renderer.drawIcon(Book24Icon, x0, iconY24, 24, 24);

  const int x1 = footerRect.x + slotW + (slotW - 24) / 2;
  renderer.drawRoundedRect(x1, iconY24 + 4, 24, 18, 2, 3, true);
  renderer.fillRect(x1 + 2, iconY24 + 2, 9, 4, true);

  const int x2 = footerRect.x + slotW * 2 + (slotW - 32) / 2;
  renderer.drawIcon(SettingsIcon, x2, iconY32, 32, 32);

  if (registerTouch) {
    TouchRegistry::getInstance().add(Rect{footerRect.x, footerRect.y, slotW, footerRect.height}, 1,
                                     TouchRegistry::Button);
    TouchRegistry::getInstance().add(Rect{footerRect.x + slotW, footerRect.y, slotW, footerRect.height}, 3,
                                     TouchRegistry::Button);
    TouchRegistry::getInstance().add(Rect{footerRect.x + slotW * 2, footerRect.y, slotW, footerRect.height}, 2,
                                     TouchRegistry::Button);
  }
}
}  // namespace

void DesktopTheme::setContinueCardFocused(const bool focused) { continueCardFocused = focused; }

bool DesktopTheme::hasPendingWallpaperGrayscale() { return pendingWallpaperGrayscale; }

void DesktopTheme::displayHomeWithGrayscale(GfxRenderer& renderer, const HalDisplay::RefreshMode refreshMode,
                                            const Rect rect) {
  if (!pendingWallpaperGrayscale) {
    renderer.displayBuffer(refreshMode);
    return;
  }
  pendingWallpaperGrayscale = false;

  const DesktopLayout layout = computeDesktopLayout(renderer, rect);
  const auto caps = display.grayscaleCapabilities(HalDisplay::GrayscaleMode::Absolute);
  if (caps.supported() && caps.base == freeink::GrayscaleBase::Separate) {
    if (!renderer.displayAbsoluteGrayscaleBase(refreshMode)) {
      renderer.displayBuffer(refreshMode);
      return;
    }

    for (const auto mode : {GfxRenderer::GRAYSCALE_LSB, GfxRenderer::GRAYSCALE_MSB}) {
      renderer.setRenderMode(mode);
      renderer.fillRect(layout.wallpaperRect.x, layout.wallpaperRect.y, layout.wallpaperRect.width,
                        layout.wallpaperRect.height, false);
      drawWallpaperArea(renderer, layout.wallpaperRect, false);
      if (mode == GfxRenderer::GRAYSCALE_LSB) {
        renderer.copyGrayscaleLsbBuffers();
      } else {
        renderer.copyGrayscaleMsbBuffers();
      }
    }

    renderer.displayGrayBuffer();
    renderer.setRenderMode(GfxRenderer::BW);
    renderer.fillRect(layout.wallpaperRect.x, layout.wallpaperRect.y, layout.wallpaperRect.width,
                      layout.wallpaperRect.height, false);
    drawWallpaperArea(renderer, layout.wallpaperRect, false, true);
    renderer.cleanupGrayscaleWithFrameBuffer();
    return;
  }

  renderer.displayGrayscaleBase(refreshMode);
  if (!renderer.storeBwBuffer()) {
    display.cleanupGrayscaleBuffers(renderer.getFrameBuffer());
    return;
  }
  for (const auto mode : {GfxRenderer::GRAYSCALE_LSB, GfxRenderer::GRAYSCALE_MSB}) {
    renderer.setRenderMode(mode);
    renderer.clearScreen(0x00);
    drawWallpaperArea(renderer, layout.wallpaperRect, false);
    if (mode == GfxRenderer::GRAYSCALE_LSB) {
      renderer.copyGrayscaleLsbBuffers();
    } else {
      renderer.copyGrayscaleMsbBuffers();
    }
  }
  renderer.displayGrayBuffer();
  renderer.setRenderMode(GfxRenderer::BW);
  renderer.restoreBwBuffer();
}

void DesktopTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                       int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                       bool& bufferRestored, const std::function<bool()>& storeCoverBuffer,
                                       const BookReadingStats* stats, float progressPercent,
                                       const GlobalReadingStats* globalStats, const char* currentChapterTitle) const {
  (void)selectorIndex;
  (void)currentChapterTitle;

  const DesktopLayout layout = computeDesktopLayout(renderer, rect);
  if (!coverRendered) {
    drawWallpaperArea(renderer, layout.wallpaperRect, true);
    drawBookThumbnail(renderer, layout.bookThumbRect, recentBooks);
    coverBufferStored = storeCoverBuffer();
    coverRendered = true;
  } else if (bufferRestored) {
    pendingWallpaperGrayscale = false;
  } else {
    // Snapshot buffer wasn't restored (e.g. low internal SRAM on C3).
    // Re-draw wallpaper/thumb cleanly in 1-bit without re-arming grayscale!
    drawWallpaperArea(renderer, layout.wallpaperRect, false, true);
    drawBookThumbnail(renderer, layout.bookThumbRect, recentBooks);
    pendingWallpaperGrayscale = false;
  }

  drawStatsCards(renderer, layout.statsRect, globalStats);
  drawContinueReadingOverlay(renderer, layout.bookCardRect, layout.bookThumbRect, recentBooks, stats, progressPercent,
                             true);

  if (layout.hasTouchFooter) {
    drawTouchMenuIcon(renderer, layout.touchMenuRect, true);
    drawTouchFooterIcons(renderer, layout.footerRect, true);
  }
}
