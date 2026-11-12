# 🐰 CrossImple

[![Release](https://img.shields.io/github/v/release/3r1pm4V/CrossImple?style=flat-square)](https://github.com/3r1pm4V/CrossImple/releases)
[![Version](https://img.shields.io/badge/Version-v0.25-brightgreen?style=flat-square)](https://github.com/3r1pm4V/CrossImple/releases)
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
| **Tối ưu chuyên sâu riêng cho Xteink X3 (Ghim cố định bảng chữ Tiếng Việt `.cpfont`, ROM thuần X3 `FREEINK_DEVICE_X4=0`, Tắt Serial Log Release & Adaptive Idle Polling)** | **CrossImple Custom** (Tự phát triển riêng cho kiến trúc `ESP32-C3` X3) | `lib/EpdFont/SdCardFont.cpp`, `lib/hal/InputWake.cpp`, `lib/hal/HalGPIO.cpp`, `src/main.cpp`, `platformio.ini` |
| **Chuẩn hóa hiển thị Font `.ttf`/`.otf` & `.cpfont` đồng nhất `v0.16` (`Plane::BWMonochrome = 0x0e`, FreeType `STEM_ADJUST` + `Stem Darkening`, Migration `characterSpacing` `2->5`, Cache PSRAM 2048-slot 2-Way LRU MurmurHash3)** | **CrossImple Custom** (Tự phát triển trên `HalScalableFont`, `GfxRenderer` & `CrossPointSettings`) | `lib/GfxRenderer/GlyphBitmap.h`, `lib/GfxRenderer/GfxRenderer.cpp`, `lib/ScalableFont/HalScalableFont.cpp`, `src/CrossPointSettings.cpp`, `src/TtfRenderProfileStore.cpp` |
| **Hỗ trợ Font CJK 16MB/32MB, Streaming FreeType & Reader Font Fallback (EPUB & TXT song ngữ)** | **CrossImple Custom** (Tự phát triển trên `HalScalableFont` & `GfxRenderer`) | `lib/ScalableFont/`, `lib/GfxRenderer/`, `src/FilenameFontSystem.cpp`, `src/SdCardFontSystem.cpp` |
| **Tinh giản Hệ thống theo Triết lý Simple (Gỡ bỏ Plugins, Nearby Transfer, Cloud Stats, Auto Backup ngầm)** | **CrossImple Custom** (Tối ưu theo triết lý Simple) | `src/SettingsList.cpp`, `src/activities/home/HomeActivity.cpp`, `src/activities/network/NetworkModeSelectionActivity.cpp` |
| **Lật trang 1-Bit Siêu tốc, Tự động ngắt WiFi thông minh, Bộ lọc Thư mục Rác HĐH & Làm mới Toàn trang Thông minh** | **CrossImple Custom** (Tự phát triển theo triết lý Simple) | `src/activities/reader/EpubReaderActivity.cpp`, `src/activities/network/CrossPointWebServerActivity.cpp`, `lib/FsHelpers/FsHelpers.cpp` |
| **Tối ưu Sleep/Wake cho X3 & X4 (Bìa ngủ 1-bit `FAST_REFRESH`, 1-pass `_half` Wake trên UC8253, Cache `optimizer-images.absent`, Gộp trạng thái Wake)** | **CrossImple Custom** (Tự phát triển) | `lib/hal/HalDisplay.cpp`, `lib/Epub/Epub.cpp`, `src/main.cpp`, `src/activities/boot_sleep/SleepActivity.cpp` |
| **8x8 Fine Bayer Dithering, E-Ink Optical Clamping (`45`–`225`), Tăng tương phản bìa sách (`1.20f`) & OOM Guard trên X3 (`8KB` trần)** | **CrossImple Custom** (Tự phát triển trên `JpegToBmpConverter` & `BitmapHelpers` của `CrossInk`) | `lib/JpegToBmpConverter/JpegToBmpConverter.cpp`, `lib/GfxRenderer/BitmapHelpers.cpp` |
| **Flash trực tiếp file `.bin` bất kỳ từ thẻ nhớ SD (`SdFirmwareUpdateActivity` & `FileBrowserActivity`)** | **CrossImple Custom** (Kế thừa và nâng cấp từ nhánh `CrossPet` / `CrossInk v1.6.1`) | `src/activities/settings/SdFirmwareUpdateActivity.cpp`, `src/activities/home/FileBrowserActivity.cpp` |
| **Bộ nhận diện Bunny Reader, Đồng bộ thương hiệu `CrossImple` (Boot/Sleep/Device Name), Việt hóa & Chuẩn hóa xuất bản `.bin`** | **CrossImple Custom** (Tự phát triển) | `src/activities/boot_sleep/BootActivity.cpp`, `src/activities/boot_sleep/SleepActivity.cpp`, `src/CrossPointSettings.cpp`, `scripts/rename_firmware.py` |

---

## 📝 Nhật ký thay đổi (Changelog)

Toàn bộ lịch sử thay đổi chi tiết qua từng phiên bản (`v0.25`, `v0.24`, `v0.23`, `v0.22`, `v0.21`, `v0.20`, `v0.16`,...) được lưu tập trung tại **[CHANGELOG.md](CHANGELOG.md)**.

---

## 📱 Thiết bị hỗ trợ (Supported Devices)

| Thiết bị | Vi điều khiển (MCU) | Màn hình | Điều khiển | Tên file Firmware xuất ra |
|---|---|---|---|---|
| **Xteink X3** | ESP32-C3 (RISC-V) | 3.97" E-Ink (792x528, ~300 PPI) | Phím cứng (4 phím mặt trước) | `crossimple_x3_v0.25.bin` |
| **Xteink X4 Classic** | ESP32-S3R8 (Xtensa Dual-Core, 8MB PSRAM) | 4.2" E-Ink (800x480, ~220 PPI) | Phím cứng (2 phím cạnh + 4 phím trước) | `crossimple_x4-classic_v0.25.bin` |
| **Xteink X4 Pro** | ESP32-S3R8 (Xtensa Dual-Core, 8MB PSRAM) | 4.2" E-Ink (800x480, ~220 PPI) + Đèn nền Warm/Cool | Cảm ứng (Capacitive Touch) + Home cảm ứng | `crossimple_x4-pro_v0.25.bin` |

---

## 🌟 Điểm nổi bật của CrossImple

1. **Thuần Đọc Sách (Pure & Minimal)** *(Nguồn: `CrossImple Custom`)*: Không trò chơi, không thú ảo, không ứng dụng thừa. Thiết bị khởi động thẳng vào kho sách của bạn.
2. **Cốt lõi Đọc Sách Hiện Đại** *(Nguồn: `CrossInk v1.6.1`)*: Focus Reading, Guide Dots, Inline Images, Scalable TTF Fonts, thống kê thời gian đọc, đánh dấu trang và hỗ trợ tiếng Việt hoàn chỉnh.
3. **Độ Ổn Định & Tốc Độ Tối Đa** *(Nguồn: `crosspoint-reader-develop`, `paperpaw-main` & `CrossImple Custom`)*: Tối ưu `ZipFile`, khóa luồng đa nhân `BookMetadataCache`, bảo vệ bảng FAT trên thẻ SD, `WifiPowerSaveGuard`, `TaskWatchdog` có trần thời gian `MAX_WDT_WORK_MS`, bảng xung 4 mức xám `GrayTone::Middle` và chốt chống kẹt nút Nguồn giúp toàn bộ chu kỳ CPU và RAM của ESP32 vận hành mượt mà, an toàn tuyệt đối.

---

## 📥 Hướng dẫn Cài đặt & Flash Firmware

### Cách 1: Nâng cấp trực tiếp qua Thẻ nhớ SD (Khuyên dùng)
1. Tải file firmware tương ứng với thiết bị của bạn từ mục [Releases](https://github.com/3r1pm4V/CrossImple/releases):
   * Với **Xteink X3**: tải `crossimple_x3_v0.25.bin`
   * Với **Xteink X4 Classic**: tải `crossimple_x4-classic_v0.25.bin`
   * Với **Xteink X4 Pro**: tải `crossimple_x4-pro_v0.25.bin`
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
# File xuất ra: crossimple_x3_v0.25.bin

# Luồng 2 — Biên dịch cho dòng Xteink X4 (ESP32-S3)
pio run -e x4-classic -e x4-pro
# File xuất ra: crossimple_x4-classic_v0.25.bin, crossimple_x4-pro_v0.25.bin
```

---

## ⚡ Chế độ Lật trang nhanh (`Fast Page Turn`) & Khử răng cưa (`Text Anti-Aliasing`)

Trong **Reader Settings (Tùy chọn Đọc sách)**, hai cài đặt **Lật trang siêu tốc (`Fast Page Turn`)** và **Khử răng cưa chữ (`Text Anti-Aliasing`)** liên kết đối nghịch trực tiếp với nhau (`fastPageTurn = !textAntiAliasing`):

| Chế độ | `Fast Page Turn = ON` *(Tắt khử răng cưa)* | `Fast Page Turn = OFF` *(Bật khử răng cưa 4 mức xám)* |
| :--- | :--- | :--- |
| **Tốc độ lật trang** | **Siêu tốc (~120–150 ms)** — chỉ chạy 1 pha xung 1-bit (`FAST_REFRESH`), phản hồi tức thì khi bấm phím. | **~420–500 ms** — chạy 1 pha đẩy mực đen/trắng + 2 pha LUT mức xám (`lut_grayscale_revert` trên X4 / `displayGrayBuffer` trên X3). |
| **Độ nét & Độ đậm chữ** | Vẽ toàn bộ điểm ảnh mức `1, 2, 3` (`Plane::BWMonochrome = 0x0e`) thành **đen đặc 100%**. Nhờ bản vá chuẩn `v0.16`, chữ vẫn **đậm đà, liền nét**, tương phản đen-trắng sắc sảo. | Giữ nguyên biên độ **4 mức xám** (Đen đặc, Xám đậm, Xám nhạt, Trắng). Các đường cong, nét chéo và dấu tiếng Việt có viền chuyển sắc êm dịu như in giấy. |
| **Tiêu thụ Pin** | **Tiết kiệm pin tối đa** (chỉ phát xung cao áp E-Ink 1 lần mỗi trang). | Tốn điện màn hình hơn một chút mỗi lần lật trang (phát xung E-Ink 3 pha). |
| **Khuyến nghị theo máy** | Rất hợp cho **Xteink X3** (`300 PPI` mật độ điểm ảnh cao nên 1-bit vẫn rất mịn) hoặc khi cần đọc lướt nhanh. | **Mặc định khuyên dùng trên X4 Classic / X4 Pro** (`220 PPI` kết hợp font `.ttf`/`.otf` giúp chữ mềm mại, tròn trịa giống hệt `v0.16`). |

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
