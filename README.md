# 🐰 CrossImple

[![Release](https://img.shields.io/github/v/release/3r1pm4V/CrossImple?style=flat-square)](https://github.com/3r1pm4V/CrossImple/releases)
[![Version](https://img.shields.io/badge/Version-v0.22-brightgreen?style=flat-square)](https://github.com/3r1pm4V/CrossImple/releases)
[![Device](https://img.shields.io/badge/Devices-Xteink%20X3%20%7C%20X4%20Classic%20%7C%20X4%20Pro-blue?style=flat-square)](#-thiết-bị-hỗ-trợ-supported-devices)
[![Base](https://img.shields.io/badge/Base-CrossPoint%20v2.0.0%20%7C%20CrossInk%20v2.0.0--rc-brightgreen?style=flat-square)](https://github.com/uxjulia/CrossInk)
[![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)](LICENSE)

> **CrossImple** là bản firmware tối giản (*Simple*), thuần khiết và siêu tối ưu dành cho máy đọc sách E-Ink Xteink X3, X4 Classic và X4 Pro.
> 
> Được xây dựng trên nền tảng **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (v2.0.0) và **[CrossInk](https://github.com/uxjulia/CrossInk)** (v2.0.0-rc) kết hợp các bản vá hiệu năng và độ ổn định mới nhất từ **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) cùng các tinh chỉnh phần cứng từ **PaperPaw** (`paperpaw-main`), **CrossImple** loại bỏ hoàn toàn các tính năng thừa thãi (Virtual Pet, trò chơi, máy tính, ứng dụng phụ) để tập trung 100% tài nguyên phần cứng vào trải nghiệm đọc sách mượt mà, tốc độ phản hồi tức thì và tiết kiệm pin tối đa.

---

## 🧭 Nguồn gốc các tính năng (Feature Attribution)

| Nhóm tính năng / Module | Nguồn gốc mã nguồn (Source Repository) | Tệp tiêu biểu |
|---|---|---|
| **Bộ Font UI hệ thống Lexica Ultralegible (Đầy đủ 100% tiếng Việt, nét chữ sắc nét chống nhòe mực E-ink)** | **[Lexica Ultralegible](https://github.com/lexica-fonts/lexica-ultralegible)** (Fork Atkinson Hyperlegible, OFL-1.1) | `lib/EpdFont/builtinFonts/` |
| **Chốt bảo vệ nguồn điện khi nạp Firmware (`Battery Guard < 25%` chống brick)** | **CrossLink** / **CrossImple** | `src/activities/settings/SdFirmwareUpdateActivity.cpp`, `src/activities/settings/OtaUpdateActivity.cpp` |
| **Bộ nhận diện bìa ngoài cho sách TXT & EPUB (`CustomCoverResolver`)** | **CrossLink** / **CrossImple** | `src/components/CustomCoverResolver.cpp`, `src/activities/home/HomeActivity.cpp`, `src/activities/boot_sleep/SleepCoverAssets.cpp` |
| **Nền tảng trình đọc EPUB / XTC / TXT, Scalable TTF Fonts, Focus Reading, Guide Dots, KOReader Sync, Web UI, Calibre Wireless, OPDS** | **[CrossInk v1.6.1](https://github.com/uxjulia/CrossInk)** (`uxjulia/CrossInk`) | `lib/Epub/`, `lib/GfxRenderer/`, `lib/KOReaderSync/`, `src/activities/reader/` |
| **Tinh chỉnh bảng xung 4 mức xám `GrayTone::Middle` cho màn SSD1677 (`lut_factory_quality`)** | **PaperPaw** (`paperpaw-main` — `crates/paperpaw-x4/src/display.rs`) | `freeink-sdk/libs/display/FreeInkDisplay/src/lut/Ssd1677Luts.h` |
| **Trần thời gian tối đa cho Task Watchdog (`MAX_WDT_WORK_MS = 15m`) chống treo vòng lặp FAT vô hạn** | **PaperPaw** (`paperpaw-main` — `shells/xteink/src/power.rs`) | `src/util/TaskWatchdog.h`, `src/main.cpp` |
| **Chốt bảo vệ kẹt nút Nguồn trước khi vào Deep Sleep (`DEFAULT_RELEASE_WAIT_MS = 10s`)** | **PaperPaw** (`paperpaw-main` — `crates/paperpaw-x4/src/sleeping.rs`) | `freeink-sdk/libs/hardware/PowerManager/src/PowerManager.cpp`, `lib/hal/HalPowerManager.cpp` |
| **`WifiPowerSaveGuard` (Tắt Modem Sleep tạm thời khi truyền mạng & lồng nhau an toàn)** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `src/network/WifiPowerSaveGuard.h`, `src/network/HttpDownloader.cpp`, `src/network/OtaUpdater.cpp`, `lib/KOReaderSync/KOReaderSyncClient.cpp` |
| **`TaskWatchdog` (`resetTaskWatchdogIfSubscribed()`) chống treo WDT khi xử lý tác vụ nặng** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `src/util/TaskWatchdog.h`, `src/network/CrossPointWebServer.cpp`, `src/network/WebDAVHandler.cpp`, `lib/Epub/Epub/Section.cpp` |
| **Tối ưu `ZipFile` (Header 46 byte, lọc nhanh độ dài tên tệp, `lastLookup` cache 1-entry)** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `lib/ZipFile/ZipFile.h`, `lib/ZipFile/ZipFile.cpp` |
| **Khóa luồng `BookMetadataCache` (`std::mutex sizeWindowLock`) & sửa biên cửa sổ `sizeWindow`** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `lib/Epub/Epub/BookMetadataCache.h`, `lib/Epub/Epub/BookMetadataCache.cpp` |
| **Bảo vệ bảng FAT trên thẻ SD (`0003-sync-fat-before-data-cache.patch`)** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `scripts/sdfat_patches/0003-sync-fat-before-data-cache.patch`, `scripts/patch_sdfat.py` |
| **Đồng bộ chốt nguồn Deep Sleep trên ESP32-C3 (`gpio_hold_dis(latch)`)** | **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`) | `lib/hal/HalPowerManager.cpp` |
| **Tách biệt Cache SDK theo từng kiến trúc chip (`patch_pioarduino_cache.py`)** | **CrossImple Custom** (Tự phát triển) | `scripts/patch_pioarduino_cache.py`, `platformio.ini` |
| **Tối ưu hiển thị Font `.ttf`/`.otf` Tiếng Việt sắc nét (FreeType Auto-Hinter `HORZ_SNAP | VERT_SNAP`, Lượng tử hóa `(3, 6, 10)`, Cache PSRAM 2048-slot 2-Way LRU MurmurHash3)** | **CrossImple Custom** (Tự phát triển trên `HalScalableFont` & FreeType Auto-Hinter) | `include/freetype/ftadvanc.h`, `lib/ScalableFont/HalScalableFont.cpp`, `src/TtfRenderProfileStore.cpp`, `src/activities/reader/TxtReaderActivity.cpp` |
| **Hỗ trợ Font CJK 16MB/32MB, Streaming FreeType & Reader Font Fallback (EPUB & TXT song ngữ)** | **CrossImple Custom** (Tự phát triển trên `HalScalableFont` & `GfxRenderer`) | `lib/ScalableFont/`, `lib/GfxRenderer/`, `src/FilenameFontSystem.cpp`, `src/SdCardFontSystem.cpp` |
| **Tinh giản Hệ thống theo Triết lý Simple (Gỡ bỏ Plugins, Nearby Transfer, Cloud Stats, Auto Backup ngầm)** | **CrossImple Custom** (Tối ưu theo triết lý Simple) | `src/SettingsList.cpp`, `src/activities/home/HomeActivity.cpp`, `src/activities/network/NetworkModeSelectionActivity.cpp` |
| **Lật trang 1-Bit Siêu tốc, Tự động ngắt WiFi thông minh, Bộ lọc Thư mục Rác HĐH & Làm mới Toàn trang Thông minh** | **CrossImple Custom** (Tự phát triển theo triết lý Simple) | `src/activities/reader/EpubReaderActivity.cpp`, `src/network/CrossPointWebServerActivity.cpp`, `lib/FsHelpers/FsHelpers.cpp` |
| **Negative Cache cho EPUB (`optimizerIndexChecked` & `xLocationsChecked`), Gộp ghi trạng thái Wake (`main.cpp`), Tắt máy tức thì (`SleepActivity.cpp`)** | **CrossImple Custom** (Tự phát triển) | `lib/Epub/Epub.cpp`, `src/main.cpp`, `src/activities/boot_sleep/SleepActivity.cpp` |
| **8x8 Fine Bayer Dithering, E-Ink Optical Clamping (`45`–`225`), Tăng tương phản bìa sách (`1.20f`) & OOM Guard trên X3 (`8KB` trần)** | **CrossImple Custom** (Tự phát triển trên `JpegToBmpConverter` & `BitmapHelpers` của `CrossInk`) | `lib/JpegToBmpConverter/JpegToBmpConverter.cpp`, `lib/BitmapHelpers/BitmapHelpers.cpp` |
| **Flash trực tiếp file `.bin` bất kỳ từ thẻ nhớ SD (`SdFirmwareUpdateActivity` & `FileBrowserActivity`)** | **CrossImple Custom** (Kế thừa và nâng cấp từ nhánh `CrossPet` / `CrossInk v1.6.1`) | `src/activities/util/SdFirmwareUpdateActivity.cpp`, `src/activities/files/FileBrowserActivity.cpp` |
| **Bộ nhận diện Bunny Reader, Đồng bộ thương hiệu `CrossImple` (Boot/Sleep/Device Name), Việt hóa & Chuẩn hóa xuất bản `.bin`** | **CrossImple Custom** (Tự phát triển) | `src/activities/boot_sleep/BootActivity.cpp`, `src/activities/boot_sleep/SleepActivity.cpp`, `src/CrossPointSettings.cpp`, `scripts/rename_firmware.py` |

---

## 📝 Nhật ký thay đổi (Changelog)

### Phiên bản v0.22 (Tối ưu hiển thị Font `.ttf`/`.otf` Tiếng Việt & Tinh giản mã nguồn)
* **Hiển thị Font `.ttf` / `.otf` Tiếng Việt sắc nét, đậm đều tương đương `.cpfont` (X4 Classic & X4 Pro)** *(Nguồn: CrossImple Custom)*:
  * **Khóa lưới nét chữ FreeType Auto-Hinter (`HORZ_SNAP | VERT_SNAP`):** Tự động kích hoạt cờ `AF_LATIN_HINTS_HORZ_SNAP | AF_LATIN_HINTS_VERT_SNAP` cho bộ Auto-Hinter qua header bọc `include/freetype/ftadvanc.h`, giúp căn chỉnh độ dày các nét ngang, nét dọc và dấu thanh tiếng Việt đồng nhất giữa mọi dòng văn bản.
  * **Lượng tử hóa 4 mức xám đậm nét `(3, 6, 10)` & Bật mặc định `Stem Darkening`:** Đồng bộ ngưỡng cắt mức xám từ `(4, 8, 12)` xuống `(3, 6, 10)` chuẩn như công cụ biên dịch `.cpfont` (`--darken-aa`), khắc phục triệt để hiện tượng chữ `.ttf`/`.otf` bị mảnh nét hoặc nhạt màu trên màn hình E-Ink.
  * **Đồng bộ cấu hình `TTF Render Options` trên cả EPUB & TXT:** Tự động nâng cấp hồ sơ `ttf-rendering.json` cũ (v1 -> v2) và tự động dàn trang lại trong trình đọc `TXT` ngay khi thay đổi tùy chọn hiển thị font mà vẫn giữ nguyên vị trí trang đang đọc.
* **Nâng cấp Bộ đệm Font PSRAM lên 2048 Slot (2-Way Set-Associative LRU + Hàm băm 64-bit MurmurHash3)** *(Nguồn: CrossImple Custom)*:
  * Mở rộng bảng đệm `PixelCacheSlots`, `MetricCacheSlots`, `KerningCacheSlots` từ `512` lên **2048 slot** trong PSRAM (tốn 0 byte RAM nội).
  * Thay thế phép chia dư tuyến tính bằng bộ trộn bit 64-bit MurmurHash3 (`fmix64`) kết hợp cấu trúc cache kết hợp 2 chiều (**2-Way Set-Associative LRU**), triệt tiêu hoàn toàn hiện tượng đụng độ chu kỳ `0x1E00 = 15 × 512` giữa hai vùng dấu tiếng Việt (`U+00C0..U+00F9` và `U+1EA0..U+1EF9`) cũng như giữa 4 kiểu chữ (`Regular`/`Bold`/`Italic`/`BoldItalic`).
* **Gỡ bỏ triệt để mã nguồn tính năng đã ẩn (Flash & RAM Optimization)** *(Nguồn: CrossImple Custom)*:
  * Xóa hoàn toàn mã nguồn của Nearby Transfer (`NearbyBookTransfer`, `NearbyStatsSync`, `NearbyBookPositionSync`) và thư viện liên kết.
  * Xóa hoàn toàn mã nguồn hệ thống Plugins (`Plugin Hub`, `Plugin Server`, `Plugin Reader Events`, các API Web `/api/plugin*`).
  * Dọn dẹp trường cấu hình `autoBackupStats` và các chuỗi dịch không còn dùng.
  * Gỡ bỏ các board thử nghiệm upstream không sử dụng (`Picco`, `Sticky`) để giữ codebase gọn gàng, tập trung 100% cho `X3`, `X4 Classic`, `X4 Pro`.
  * Giảm ~200–220 KB dung lượng Flash và ~1.9 KB RAM tĩnh trên mọi thiết bị.

### Phiên bản v0.21
* **Hỗ trợ Font Tiếng Trung / CJK Dung lượng lớn & Font Fallback Trình đọc sách** *(Nguồn: CrossImple Custom)*:
  * **Mở rộng giới hạn Font ngoài (16 MB / 32 MB):** Nâng trần dung lượng font ngoài (`MaxFileBytes`) lên **16 MB** và `MaxFamilyBytes` lên **32 MB** cho cả thẻ nhớ SD lẫn Web Upload (tương thích hoàn hảo với các bộ font CJK đầy đủ như Noto Sans SC 10-14 MB).
  * **Gắn Font dự phòng CJK thẳng vào Trình đọc sách (Reader Font Fallback):** Trình đọc EPUB & TXT tự động liên kết với font CJK dự phòng (chọn trong *Cài đặt > Thiết bị > Font dự phòng*). Cho phép người dùng kết hợp 1 Font Latin yêu thích (như Mali) làm font chính + 1 Font tiếng Trung dự phòng để đọc mượt mà song song cả tên sách ngoài giao diện lẫn nội dung tiếng Trung bên trong mà không còn bị lỗi hiển thị ô vuông hay `????`.
  * **FreeType Streaming an toàn bộ nhớ:** Font <= 2 MB được nạp toàn bộ vào PSRAM để tối ưu tốc độ lật trang, font CJK lớn (> 2 MB) tự động stream qua bộ đệm tiền tố 1 MB PSRAM, đảm bảo hệ thống không bao giờ bị cạn kiệt bộ nhớ.
* **Tinh giản Siêu Sạch theo Tinh thần CrossImple (Clean Pruning)** *(Nguồn: CrossImple Custom)*:
  * Loại bỏ hoàn toàn khỏi giao diện người dùng và chu trình vận hành các tính năng cồng kềnh thừa thãi: gỡ bỏ hệ thống **Plugins & Plugin Hub**, tính năng chia sẻ ngang hàng **Nearby Book Transfer & Stats Sync**, tính năng upload đám mây **Stats Upload** và **Tự động sao lưu thống kê ngầm (Auto Backup Stats)**.
  * Giữ lại và bảo toàn trọn vẹn các tính năng đọc sách cốt lõi: **KOReader Sync** (Sync Server), **OPDS Catalog Browser**, toàn bộ tùy chỉnh font và dàn trang của **Reader Settings** (8..44pt, Margin, Spacing, Line Height, Focus/Guide Mode), các bộ **Themes** (Minimal, Dark, Cozy, Pixel,...) và **Phím tắt (Button Remap)**.
* **Bổ sung 4 Tối ưu Tinh gọn theo Triết lý Simple** *(Nguồn: CrossImple Custom)*:
  * **Chế độ Lật trang 1-Bit Siêu tốc (Monochrome Fast-Turn Mode):** Khi đọc các trang sách chỉ có chữ thuần túy (`!pageHasImages`), hệ thống bỏ qua toàn bộ chu trình khử răng cưa đa mức xám và bộ đệm trung gian để vẽ trực tiếp qua chế độ làm mới vi sai 1-bit (<150ms). Phản hồi lật trang tức thì và tiết kiệm tối đa thời lượng pin. Bật/tắt trong *Reader Settings > Lật trang siêu tốc*.
  * **Tự động ngắt WiFi thông minh (Network Idle Auto-Off Timer):** Tự động ngắt Web Server và tắt nguồn modem WiFi (`WiFi.mode(WIFI_OFF)`) sau 5 phút không có hoạt động truyền dữ liệu, bảo toàn pin nếu người dùng vô tình quên tắt kết nối.
  * **Bộ lọc Thư mục Rác Hệ điều hành (OS Junk Blacklist Filter):** Loại bỏ triệt để các thư mục và tập tin rác hệ thống phát sinh từ macOS, Windows, Android (`$RECYCLE.BIN`, `.Spotlight-V100`, `.Trashes`, `.fseventsd`, `Android`, `LOST.DIR`, `Thumbs.db`,...) khỏi Thư viện và Quản lý tệp, tăng tốc độ nạp thẻ SD và giữ giao diện luôn tinh giản.
  * **Làm mới Toàn trang Thông minh (Smart Contextual Full-Refresh):** Tự động kích hoạt chu trình làm mới toàn diện (`HALF_REFRESH`) ngay khi vừa lật qua một trang sách có chứa ảnh minh họa, xóa sạch bóng mờ (ghosting) mà không làm chậm các trang chữ thông thường tiếp theo.
* **Bảo lưu 100% bản sắc tùy biến của CrossImple**:
  * Bộ Font UI Lexica Ultralegible tiếng Việt chuẩn, Chốt bảo vệ nguồn nạp Firmware (Battery Guard < 25%), Bộ nhận diện bìa ngoài (CustomCoverResolver), Bảng xung 4 mức xám GrayTone::Middle cho màn hình SSD1677, và cơ chế an toàn Watchdog 15m / Nút nguồn 10s.

---

### Phiên bản v0.20
* **Tối ưu Hiệu năng & Nâng cao Độ ổn định (Đặc biệt giá trị cho máy E-ink)** *(Nguồn: crosspoint-reader v2.0.0 & CrossImple)*:
  * **Tối ưu chỉ mục sách lớn (Large EPUB Fast Indexing):** Tăng tốc mở và giải mã các sách EPUB dung lượng lớn, hỗ trợ nhảy nhanh theo phần trăm (%) trang đọc mà không cần quét lại tuần tự các chương sách trước đó.
  * **Tối ưu bộ nhớ danh sách Dấu trang (Bookmark List Memory Optimization #892):** Nạp danh sách bookmark theo trang phân đoạn, hoạt động mượt mà và ổn định khi mở sách có nhiều đánh dấu trang.
  * **Khôi phục nhanh khi thức dậy (Quick Resume #870):** Ghi nhớ và giữ nguyên trạng thái trang đọc chính xác qua các chu kỳ Sleep/Wake mà không phải dựng lại từ đầu.
  * **Tối ưu kết nối OPDS qua HTTPS với bộ nhớ thấp (#836):** Tối ưu hóa bộ đệm TLS wolfSSL giúp kết nối và tải sách mượt mà từ các kho sách OPDS qua giao thức HTTPS bảo mật trên ESP32.
  * **Hoàn thiện ngắt từ đoạn văn dài (Long Paragraph Hyphenation Chunk Split #844):** Đảm bảo các đoạn văn dài có tính năng gạch nối từ tự động (hyphenation) không bị ngắt quãng hoặc lỗi căn lề giữa chừng.
  * **Bảo toàn chuỗi ngày đọc sách qua nửa đêm (Midnight Reading Streak):** Ghi nhận chính xác mốc chuyển giao ngày (00:00) trong khi đang đọc để bảo toàn chuỗi đọc liên tục mà không bị đứt đoạn thống kê.
  * **Tối ưu bộ nhớ Web Server & WebDAV:** Giảm dung lượng chiếm dụng RAM khi bật chế độ quản lý file qua trình duyệt web.
* **Trình đọc sách & Hiển thị Font chữ chuyên nghiệp** *(Nguồn: crosspoint-reader v2.0.0 & CrossInk v2.0.0-rc)*:
  * **Hỗ trợ Font nhúng trong EPUB (Embedded Fonts trên ESP32-S3):** Tự động trích xuất và hiển thị các bộ font nhúng sẵn trong tệp EPUB trên X4 Classic & X4 Pro.
  * **Hỗ trợ Font tĩnh định dạng .otf (Static OTF Font Support):** Bổ sung khả năng nạp trực tiếp các bộ font OpenType (.otf) tĩnh từ thẻ nhớ SD bên cạnh định dạng .ttf và .cpfont.
  * **Cơ chế UI Fallback Font thông minh:** Tự động hiển thị ký tự thay thế từ font hệ thống dự phòng khi font hiện tại thiếu ký tự (tránh hiện tượng ô vuông/mất chữ).
  * **Thanh trượt giãn cách ký tự (Character Spacing: -5 đến +5) & Giãn cách từ (Word Spacing: -4 đến +4):** Tinh chỉnh linh hoạt khoảng cách chữ và khoảng cách từ ngay trong menu cài đặt Reader.
  * **Hỗ trợ CSS nâng cao:** Hiển thị chuẩn xác Drop Caps (chữ cái đầu thụt to), bordered/shaded blocks (khung viền/nền xám trích dẫn), và khối `fieldset`.
  * **Biểu tượng đánh dấu sách đã đọc xong (Finished Book Checkmark):** Hiển thị icon dấu tích trực quan bên cạnh các cuốn sách đã hoàn thành trong Thư viện (Library) và Quản lý tệp (File Browser).
  * **Tự động đảo chiều nút cạnh theo góc xoay (Orient Side Buttons):** Tự động đảo chiều nút lật trang cạnh bên và nhãn Up/Down trên X4 Classic theo góc xoay màn hình (90°, 180°, 270°).
* **Nâng cấp Hệ thống Đồng bộ (Sync Server)** *(Nguồn: crosspoint-reader v2.0.0 & CrossInk v2.0.0-rc)*:
  * **Đại tu KOReader Sync thành Sync Server hoàn chỉnh:** Hỗ trợ đồng bộ đa chiều tiến độ đọc sách, dữ liệu thống kê đọc (Reading Stats) và ghi chú/trích dẫn (Clippings) với máy chủ CrossPoint Sync / KOReader Server.
  * **Đồng bộ hàng loạt một chạm (Sync Books Batch Sync):** Bổ sung tùy chọn đồng bộ toàn bộ sách trong thư viện chỉ với một thao tác.
  * **Thời gian đọc trung bình hàng ngày (Daily Reading Time - 7-day average):** Tự động tính toán và hiển thị thời gian đọc trung bình trong 7 ngày gần nhất trên thẻ thống kê.
* **Bảo lưu 100% bản sắc tùy biến của CrossImple**:
  * Font hệ thống Lexica Ultralegible tiếng Việt chuẩn, Chốt bảo vệ nguồn điện nạp Firmware (Battery Guard < 25%), Bộ nhận diện bìa ngoài (CustomCoverResolver), Bảng xung 4 mức xám GrayTone::Middle cho màn hình SSD1677, và cơ chế an toàn Watchdog 15m / Nút nguồn 10s.

---

### Phiên bản v0.16
* **Nâng cấp Font UI hệ thống sang Lexica Ultralegible (100% tiếng Việt chuẩn, chống nhòe mực E-ink)** *(Nguồn: `lexica-fonts/lexica-ultralegible`)*:
  * Thay thế font UI gốc sang bộ font Lexica Ultralegible (fork cải tiến tiếng Việt của Atkinson Hyperlegible), hỗ trợ trọn vẹn 100% ký tự có dấu thanh tổ hợp, dấu 2 tầng và nguyên âm có râu. Phân biệt rõ ràng các cặp ký tự dễ nhầm lẫn (`I`/`l`/`1`, `0`/`O`), tăng khoảng mở chữ để chống hiện tượng loang mực (dot gain) trên màn hình E-ink.
* **Chốt bảo vệ nguồn điện khi nạp Firmware (Battery Guard < 25%)** *(Nguồn: `crosslink-master` & tối ưu bởi `CrossImple`)*:
  * Tự động kiểm tra mức pin trước khi nạp qua thẻ SD (`SdFirmwareUpdateActivity`) hoặc qua mạng OTA (`OtaUpdateActivity`). Nếu pin dưới 25% và không cắm sạc USB (`!gpio.isUsbConnected()`), hệ thống sẽ lập tức chặn nạp và cảnh báo người dùng nhằm ngăn ngừa triệt để nguy cơ sập nguồn gây hard-brick chip flash.
* **Bộ nhận diện bìa ngoài cho sách TXT & EPUB (Custom Cover Resolver)** *(Nguồn: `crosslink-master` & tối ưu bởi `CrossImple`)*:
  * Tự động phát hiện và hiển thị ảnh bìa ngoài (`.jpg`, `.png`, `.bmp`) cho file TXT và file EPUB thiếu bìa gốc từ thư mục chứa sách hoặc từ `/.crosspoint/custom_cover/<TenSach>.*` trên cả màn hình Home (lưới bìa sách) và màn hình Sleep Cover.
* **Tối ưu RAM kỷ lục 19.8% (64,744 bytes / 327,680 bytes)** *(Nguồn: `CrossImple Custom`)*:
  * Triển khai `CustomCoverResolver` dạng pure stateless static resolver (zero heap, không chiếm giữ RAM nền), duy trì trọn vẹn dung lượng RAM trống tối đa cho ESP32-C3 / Xteink X3.

---

### Phiên bản v0.15
* **Tinh chỉnh bảng xung 4 mức xám `GrayTone::Middle` cho màn SSD1677 (X4 / X4 Classic)** *(Nguồn: `paperpaw-main` — `crates/paperpaw-x4/src/display.rs`)*:
  * Hiệu chỉnh 4 byte pha kéo sáng trung gian trong bảng xung `lut_factory_quality` (`Ssd1677Luts.h`: `lut[11..12] = 0x60, 0x80` và `lut[21..22] = 0x40, 0x80`), giảm pha kéo sáng trắng của mức xám đậm từ 2 frame xuống 1 frame và mức xám nhạt từ 4 frame xuống 3 frame (giữ nguyên tổng 50 frame và áp chuẩn OTP của hãng). Giúp ảnh bìa sách và màn hình chờ 4 mức xám trên X4 / X4 Classic có chiều sâu và độ tương phản rõ nét hơn mà không tốn thêm pin hay RAM.
* **Trần thời gian tối đa cho Task Watchdog (`MAX_WDT_WORK_MS`)** *(Nguồn: `paperpaw-main` — `shells/xteink/src/power.rs`)*:
  * Bổ sung mốc thời gian vòng lặp chính `markWatchdogMainLoopTick()` trong `main.cpp` và giới hạn trần `MAX_WDT_WORK_MS = 15 phút` trong `src/util/TaskWatchdog.h`. Đảm bảo nếu thẻ nhớ SD bị lỗi bảng FAT tạo thành vòng lặp cụm vô tận (circular cluster chain), hệ thống sẽ ngừng nạp lại Watchdog sau ngưỡng trần để tự động khởi động lại thay vì treo cứng vĩnh viễn.
* **Chốt chống kẹt/đè nút Nguồn khi vào Deep Sleep trong bao da (`DEFAULT_RELEASE_WAIT_MS = 10s`)** *(Nguồn: `paperpaw-main` — `crates/paperpaw-x4/src/sleeping.rs`)*:
  * Giới hạn vòng chờ nhả nút nguồn `waitForPowerButtonRelease()` tối đa 10 giây trước khi vào ngủ sâu (`PowerManager.cpp`, `HalPowerManager.cpp`). Nếu nút Nguồn bị đè chặt trong túi xách/bao da quá 10 giây, thiết bị sẽ ngắt nguồn vào thẳng Deep Sleep mà không tự kích hoạt đánh thức ngược trở lại, loại bỏ hoàn toàn hiện tượng cấn nút gây nóng máy và cạn sạch pin.
* **Đồng bộ triệt để thương hiệu `CrossImple` trên màn hình Sleep & Tên thiết bị** *(Nguồn: `CrossImple Custom`)*:
  * Thay thế chuỗi `"CrossInk"` còn sót lại ở màn hình Sleep mặc định (`SleepActivity.cpp`) và tên thiết bị mặc định (`CrossPointSettings.cpp`) thành `"CrossImple"` / `"CrossImple X3"` / `"CrossImple X4"` / `"CrossImple X4 Classic"` / `"CrossImple X4 Pro"`.
* **Tinh gọn dung lượng Flash (~660 KB) & Tách luồng Build 2 kiến trúc (`scripts/build_all.py`)** *(Nguồn: `CrossImple Custom`)*:
  * Giới hạn gói ngôn ngữ giao diện Menu (`custom_i18n_builtin_langs`) vào 8 ngôn ngữ thông dụng nhất (`en, vi, fr, es, it, pt, de, ru`) và lược bỏ 5 bảng âm tiết gạch nối ngoại ngữ quá nặng ít dùng (`de_trie_data`, `ru`, `sv`, `uk`, `pl`), giữ nguyên 100% bộ gạch nối cho Tiếng Anh, Pháp, Tây Ban Nha, Ý, Bồ Đào Nha và khả năng hiển thị sách đa ngôn ngữ. Giúp giải phóng ~660 KB bộ nhớ Flash trên cả 3 dòng máy.

---

### Phiên bản v0.14
* **Tăng tốc truyền tải WiFi & Đồng bộ đám mây với `WifiPowerSaveGuard`** *(Nguồn: `crosspoint-reader-develop`)*:
  * **Nâng cấp `WifiPowerSaveGuard` (`src/network/WifiPowerSaveGuard.h`):** Tắt tạm thời chế độ tiết kiệm điện modem (`WIFI_PS_NONE`) trong suốt phiên truyền tải mạng để loại bỏ độ trễ bắt tay TLS nhiều giây giữa các chu kỳ DTIM beacon, bổ sung ghi nhật ký lỗi (`LOG_ERR`) và tự động bỏ qua lệnh gọi hệ thống thừa khi các guard lồng nhau hoặc khi WebServer đã giữ `WIFI_PS_NONE`.
  * **Mở rộng sang KOReader Sync (`lib/KOReaderSync/KOReaderSyncClient.cpp`):** Bọc `WifiPowerSaveGuard` cho cả 4 tác vụ `authenticate()`, `createUser()`, `getProgress()` và `updateProgress()`, giúp đồng bộ tiến độ đọc sách qua TLS 1.3 (wolfSSL) nhanh và ổn định hơn mà không gây hao pin khi đọc sách offline.
* **Cơ chế chống treo Task Watchdog (`TaskWatchdog.h`)** *(Nguồn: `crosspoint-reader-develop` & mở rộng bởi `CrossImple`)*:
  * **Tích hợp `resetTaskWatchdogIfSubscribed()` (`src/util/TaskWatchdog.h`):** Chỉ nạp lại bộ đếm ESP-IDF Task Watchdog khi luồng hiện tại thực sự được đăng ký (`esp_task_wdt_status(nullptr) == ESP_OK`), tránh lỗi cảnh báo ESP-IDF khi gọi từ luồng không đăng ký.
  * **Bảo vệ WebDAV & WebServer khi truyền tệp lớn (`src/network/WebDAVHandler.cpp`, `src/network/CrossPointWebServer.cpp`):** Bổ sung `resetTaskWatchdogIfSubscribed()` trong vòng lặp quét thư mục lớn (`scanFiles`, `PROPFIND`), tải lên/tải xuống tệp dung lượng lớn qua HTTP/WebSocket/WebDAV và cài đặt bộ font chữ `.cpfont`/`.ttf` *(Nguồn: `crosspoint-reader-develop`)*.
  * **Bảo vệ trình dựng trang EPUB trên chip đơn nhân ESP32-C3 (`lib/Epub/Epub/Section.cpp`):** Bổ sung `resetTaskWatchdogIfSubscribed()` trong vòng lặp phân tích chương (`createSectionFile`, `buildSomeMore`) và tuần tự hóa trang (`onPageComplete`), ngăn chặn hoàn toàn hiện tượng tự khởi động lại máy do Task Watchdog khi mở các chương sách EPUB nặng trên Xteink X3 *(Nguồn: `CrossImple Custom` trên nền `TaskWatchdog.h` của `crosspoint-reader-develop`)*.

---

### Phiên bản v0.13
* **Tăng tốc độ mở sách EPUB & giải nén ZIP:**
  * **Tối ưu `ZipFile` & giảm áp lực Stack (`lib/ZipFile/ZipFile.cpp`)** *(Nguồn: `crosspoint-reader-develop`)*: Loại bỏ bộ đệm 512 byte trên stack (`CdReader`), thay bằng đọc header 46 byte kết hợp lọc nhanh theo độ dài tên tệp (`nameLen == targetLen`) và bổ sung bộ nhớ đệm 1-entry (`lastLookup`) giúp cặp thao tác kiểm tra kích thước + giải nén không phải quét lại Central Directory lần thứ hai.
  * **Cơ chế Negative Cache cho EPUB (`lib/Epub/Epub.cpp`)** *(Nguồn: `CrossImple Custom`)*: Bổ sung cờ ghi nhớ trạng thái kiểm tra `optimizerIndexChecked` và `xLocationsChecked`, ngăn việc quét lại toàn bộ ZIP Central Directory ở mỗi lần chuyển chương đối với các tệp EPUB tiêu chuẩn.
* **Tăng tốc độ thức dậy (Wake) & đi vào chế độ ngủ (Sleep):**
  * **Gộp thao tác ghi trạng thái khi Wake (`src/main.cpp`)** *(Nguồn: `CrossImple Custom`)*: Gộp nhiều lần ghi `APP_STATE.saveToFile()` xuống thẻ nhớ SD khi thức dậy thành 1 lần duy nhất sau khi hoàn tất vẽ màn hình, đồng thời bỏ qua quét thư mục `bootscreens` khi đã ghim sẵn ảnh khởi động yêu thích.
  * **Tắt máy tức thì trên Xteink X3 (`src/activities/boot_sleep/SleepActivity.cpp`)** *(Nguồn: `CrossImple Custom`)*: Bỏ qua bước nháy màn hình vẽ hộp thoại `"Entering Sleep..."` khi ảnh ngủ hoặc bìa sách đã có sẵn trong bộ nhớ đệm, rút ngắn ~350–450ms thời gian tắt máy.
  * **Đồng bộ chốt nguồn Deep Sleep trên ESP32-C3 (`lib/hal/HalPowerManager.cpp`)** *(Nguồn: `crosspoint-reader-develop`)*: Bổ sung `gpio_hold_dis(latch)` trước khi hạ chân `latch` xuống mức `LOW`, đảm bảo mạch giữ nguồn ngắt/mở sạch sẽ giữa các chu kỳ ngủ sâu.
* **Gia cố độ an toàn đa luồng (Dual-Core ESP32-S3) & Bảo vệ thẻ nhớ SD:**
  * **Khóa luồng `BookMetadataCache` (`lib/Epub/Epub/BookMetadataCache.cpp`)** *(Nguồn: `crosspoint-reader-develop`)*: Bổ sung `std::mutex sizeWindowLock` và sửa công thức biên cửa sổ `sizeWindow`, triệt tiêu hoàn toàn nguy cơ xung đột dữ liệu (data race) giữa luồng vẽ màn hình (Core 0) và luồng chính (Core 1) trên X4 Classic và X4 Pro.
  * **Bảo vệ bảng FAT trên thẻ nhớ SD (`0003-sync-fat-before-data-cache.patch`)** *(Nguồn: `crosspoint-reader-develop`)*: Luôn đồng bộ `m_fatCache` xuống thẻ nhớ trước `m_cache`, ngăn ngừa lỗi hỏng cấu trúc tệp khi mất nguồn đột ngột.
  * **Tách biệt Cache SDK theo từng kiến trúc chip (`scripts/patch_pioarduino_cache.py`) & Dọn sạch 16 tệp tàn dư cũ** *(Nguồn: `CrossImple Custom`)*: Cô lập hoàn toàn thư viện tĩnh `.a` giữa `esp32c3` (X3) và `esp32s3` (X4 Classic / X4 Pro), loại bỏ 1.280 dòng mã thừa còn sót lại.

---

### Phiên bản v0.12
* **Nâng cấp thuật toán Dithering & Độ tương phản E-Ink** *(Nguồn: `CrossImple Custom` trên nền `CrossInk v1.6.1`)*:
  * **Ma trận 8x8 Fine Bayer (`lib/BitmapHelpers/BitmapHelpers.cpp`):** Thay thế ma trận Bayer 4x4 cũ bằng ma trận 8x8 (64 mức sắc độ mịn), khử sạch hiện tượng sọc caro và mắt lưới thô trên hình ảnh.
  * **E-Ink Optical Clamping (`lib/JpegToBmpConverter/JpegToBmpConverter.cpp`):** Ép ngưỡng đen (`<= 45`) về đen đặc (0) và ép ngưỡng trắng (`>= 225`) về trắng tinh (3). Loại bỏ hoàn toàn các chấm dither li ti bẩn nền giấy, giữ viền chữ và nét vẽ manga đậm nét tương tự firmware stock.
  * **Hiệu chỉnh tương phản bìa sách (`lib/BitmapHelpers/BitmapHelpers.cpp`):** Bật bộ lọc Gamma Correction và tăng 20% tương phản (`CONTRAST_FACTOR = 1.20f`) cho các bìa sách tạo từ thẻ SD.
* **Cơ chế chống tràn RAM (OOM Guard trên X3)** *(Nguồn: `CrossImple Custom`)*:
  * Tinh gọn trần bộ đệm hàng (`MAX_BAND_BYTES`) từ 24KB xuống 8KB cho vi điều khiển không có PSRAM (ESP32-C3).
  * Bổ sung cơ chế tự động thu hẹp bộ đệm xuống kích thước tối thiểu của một khối MCU (`maxBlockDstRows + 2`, ~3.6KB) khi heap bị phân mảnh, ngăn chặn sập nguồn (OOM) hoặc đơ máy (watchdog reset) khi mở sách có ảnh kích thước lớn.
* **Bổ sung hỗ trợ Xteink X4 Pro** *(Nguồn: `CrossInk v1.6.1` & `CrossImple Custom`)*:
  * Xuất bản chính thức firmware hỗ trợ đầy đủ màn hình cảm ứng điện dung, phím Home cảm ứng và đèn nền 2 màu Warm/Cool PWM.

---

### Phiên bản v0.11
* **Tái định vị thương hiệu CrossImple (Pure Reader)** *(Nguồn: `CrossImple Custom`)*:
  * Gỡ bỏ hoàn toàn module Virtual Pet (>3.800 dòng code từ `CrossPet`), đưa firmware về trạng thái thuần khiết 100% phục vụ đọc sách, không còn bất kỳ tiến trình hay timer ngầm nào, đưa mức RAM tĩnh trên X3 về mức tối ưu 19.8%.
* **Bộ nhận diện & Màn hình khởi động mới (Bunny Reader)** *(Nguồn: `CrossImple Custom`)*:
  * Tích hợp logo chú thỏ ngồi đọc sách đơn sắc độ phân giải cao, khử sạch watermark.
  * Tinh chỉnh quét ma trận ảnh logo xoay 90° CCW tương thích chuẩn quét dọc (portrait panel scan) của phần cứng.
  * Chuẩn hóa dòng chữ phiên bản khởi động: hiển thị gọn gàng `CrossImple <version>` thay vì chuỗi build gốc của upstream.
* **Chuẩn hóa đặt tên file Firmware xuất ra** *(Nguồn: `CrossImple Custom`)*:
  * Quy chuẩn định dạng file build tự động: `crossimple_<mã máy>_<version>.bin`.
  * Hỗ trợ chính thức **Xteink X3** và **Xteink X4 Classic**.

---

### Phiên bản v0.1 (Khởi đầu - CrossPet v1.6.1)
* **Xây dựng trên nền tảng CrossInk v1.6.1** *(Nguồn: `uxjulia/CrossInk v1.6.1`)*: Fork trực tiếp từ bản phát hành ổn định v1.6.1 của CrossInk.
* **Nâng cấp tính năng Flash Firmware từ thẻ nhớ SD (`SdFirmwareUpdateActivity` & `FileBrowserActivity`)** *(Nguồn: `CrossImple / CrossPet Custom`)*:
  * Hỗ trợ duyệt và chọn trực tiếp file `.bin` bất kỳ trong trình quản lý tệp (File Browser) để kiểm tra tính toàn vẹn (mã băm SHA256, chip ESP32, tag phần cứng) và tiến hành nạp ngay firmware mà không cần bắt buộc đổi tên file thành `update.bin`.
* **Dọn dẹp hệ thống (Tối ưu Flash & Giao diện)** *(Nguồn: `CrossImple / CrossPet Custom`)*:
  * Loại bỏ các công cụ và trò chơi cũ thừa thãi (Tools & Games cũ) để tiết kiệm bộ nhớ Flash và làm gọn menu chính.
* **Tối ưu hóa đa ngôn ngữ** *(Nguồn: `CrossInk v1.6.1` & `CrossImple Custom`)*:
  * Bổ sung và hoàn thiện bản dịch tiếng Việt (`vietnamese.yaml`) cho toàn bộ các màn hình, menu cài đặt và thông báo hệ thống.
* **Tích hợp thử nghiệm Virtual Pet** *(Nguồn: `CrossPet`)*: Thử nghiệm module nuôi thú ảo đồng bộ với tiến độ đọc sách (đã được gỡ bỏ hoàn toàn từ bản v0.11 để chuyển thành máy đọc sách thuần khiết CrossImple).

---

## 📱 Thiết bị hỗ trợ (Supported Devices)

| Thiết bị | Vi điều khiển (MCU) | Màn hình | Điều khiển | Tên file Firmware xuất ra |
|---|---|---|---|---|
| **Xteink X3** | ESP32-C3 (RISC-V) | 3.97" E-Ink (792x528, ~300 PPI) | Phím cứng (4 phím mặt trước) | `crossimple_x3_v0.22.bin` |
| **Xteink X4 Classic** | ESP32-S3R8 (Xtensa Dual-Core, 8MB PSRAM) | 4.2" E-Ink (800x480, ~220 PPI) | Phím cứng (2 phím cạnh + 4 phím trước) | `crossimple_x4-classic_v0.22.bin` |
| **Xteink X4 Pro** | ESP32-S3R8 (Xtensa Dual-Core, 8MB PSRAM) | 4.2" E-Ink (800x480, ~220 PPI) + Đèn nền Warm/Cool | Cảm ứng (Capacitive Touch) + Home cảm ứng | `crossimple_x4-pro_v0.22.bin` |

---

## 🌟 Điểm nổi bật của CrossImple

1. **Thuần Đọc Sách (Pure & Minimal)** *(Nguồn: `CrossImple Custom`)*: Không trò chơi, không thú ảo, không ứng dụng thừa. Thiết bị khởi động thẳng vào kho sách của bạn.
2. **Cốt lõi Đọc Sách Hiện Đại** *(Nguồn: `CrossInk v1.6.1`)*: Focus Reading, Guide Dots, Inline Images, Scalable TTF Fonts, thống kê thời gian đọc, đánh dấu trang và hỗ trợ tiếng Việt hoàn chỉnh.
3. **Độ Ổn Định & Tốc Độ Tối Đa** *(Nguồn: `crosspoint-reader-develop`, `paperpaw-main` & `CrossImple Custom`)*: Tối ưu `ZipFile`, khóa luồng đa nhân `BookMetadataCache`, bảo vệ bảng FAT trên thẻ SD, `WifiPowerSaveGuard`, `TaskWatchdog` có trần thời gian `MAX_WDT_WORK_MS`, bảng xung 4 mức xám `GrayTone::Middle` và chốt chống kẹt nút Nguồn giúp toàn bộ chu kỳ CPU và RAM của ESP32 vận hành mượt mà, an toàn tuyệt đối.

---

## 📥 Hướng dẫn Cài đặt & Flash Firmware

### Cách 1: Nâng cấp trực tiếp qua Thẻ nhớ SD (Khuyên dùng)
1. Tải file firmware tương ứng với thiết bị của bạn từ mục [Releases](https://github.com/3r1pm4V/CrossImple/releases):
   * Với **Xteink X3**: tải `crossimple_x3_v0.22.bin`
   * Với **Xteink X4 Classic**: tải `crossimple_x4-classic_v0.22.bin`
   * Với **Xteink X4 Pro**: tải `crossimple_x4-pro_v0.22.bin`
2. Chép file `.bin` vào thư mục gốc của thẻ MicroSD (định dạng FAT32).
3. Bật máy đọc sách, mở mục **Quản lý tệp (File Browser)**, chọn file `.bin` vừa chép và nhấn **Xác nhận (Confirm)**.
4. Máy sẽ tự động kiểm tra chip ESP32, tag phần cứng, tính toàn vẹn SHA256 và tiến hành flash an toàn. Sau khi hoàn tất máy sẽ tự khởi động lại.

### Cách 2: Flash bằng chế độ Cứu hộ (Bootloader Recovery)
1. Đổi tên file firmware thành `update.bin` và chép vào thư mục gốc của thẻ MicroSD.
2. Tắt máy, sau đó nhấn giữ nút **Quay lại (Back)** đồng thời bấm nút Nguồn để vào chế độ nạp khẩn cấp.
3. Thiết bị sẽ tự động phát hiện `update.bin` và tiến hành nạp firmware.

### Cách 3: Nạp qua cổng USB bằng PlatformIO
```bash
# Cho Xteink X3:
pio run -e default --target upload

# Cho Xteink X4 Classic:
pio run -e x4-classic --target upload
```

---

## 🛠️ Biên dịch từ Mã Nguồn (Build from Source)

> **Lưu ý kiến trúc:** Vì **Xteink X3** chạy chip `ESP32-C3` (RISC-V, dùng `custom_sdkconfig` lai ESP-IDF) còn **X4 Classic / X4 Pro** chạy chip `ESP32-S3` (Xtensa, dùng thư viện dựng sẵn), hai kiến trúc này **luôn phải chạy ở 2 tiến trình `pio run` tách biệt** (hoặc chạy qua script `scripts/build_all.py`) để tránh xung đột môi trường SCons.

```bash
# Clone repository
git clone --recurse-submodules https://github.com/3r1pm4V/CrossImple.git
cd CrossImple

# Cách 1 (Khuyên dùng): Tự động chạy tách biệt 2 luồng (Luồng 1: X3 -> Luồng 2: X4 Classic & X4 Pro)
python scripts/build_all.py --export-bin

# Cách 2: Chạy thủ công từng luồng riêng biệt
# Luồng 1 — Biên dịch riêng cho Xteink X3 (ESP32-C3)
pio run -e default
# File xuất ra: crossimple_x3_v0.22.bin

# Luồng 2 — Biên dịch cho dòng Xteink X4 (ESP32-S3)
pio run -e x4-classic -e x4-pro
# File xuất ra: crossimple_x4-classic_v0.22.bin, crossimple_x4-pro_v0.22.bin
```

---

## 🎮 Thao tác Phím Bấm

| Phím | Màn hình chính (Home) | Trong trình đọc sách (Reader) |
|---|---|---|
| **Lên / Trái** | Chọn mục trước | Lật trang trước |
| **Xuống / Phải** | Chọn mục kế tiếp | Lật trang tiếp theo |
| **Xác nhận (Confirm)** | Mở mục đã chọn | Mở menu tùy chọn đọc sách |
| **Quay lại (Back)** | Tiếp tục đọc cuốn sách gần nhất | Thoát sách / Quay lại menu trước |

---

## 📄 Bản quyền & Lời cảm ơn (Credits)

* Kế thừa nền tảng **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (v2.0.0) và **[CrossInk](https://github.com/uxjulia/CrossInk)** (v2.0.0-rc) phát triển bởi [@uxjulia](https://github.com/uxjulia).
* Tích hợp các bản vá hệ thống, tối ưu `ZipFile`, `BookMetadataCache`, `SdFat`, `WifiPowerSaveGuard` và `TaskWatchdog` từ **[CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)** (`crosspoint-reader-develop`).
* Tích hợp tinh chỉnh bảng xung 4 mức xám `GrayTone::Middle`, giới hạn trần `TaskWatchdog` và chốt chống kẹt nút Nguồn từ **PaperPaw** (`paperpaw-main`, phát triển bởi Hannes Thoene, Apache-2.0).
* Mã nguồn phát hành dưới giấy phép mã nguồn mở [MIT License](LICENSE).
