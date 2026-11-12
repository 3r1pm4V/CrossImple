# Third-Party Notices

Tài liệu này tổng hợp các thành phần mã nguồn mở, thư viện, tài nguyên ngôn ngữ, bảng dữ liệu và phông chữ của bên thứ ba được tìm thấy trực tiếp trong kho mã nguồn **CrossImple** (chủ yếu trong `lib/`, `assets/`, `freeink-sdk/`, `web/` / `src/network/html/`, `test/` và khai báo phụ thuộc trong `platformio.ini`).

Mọi thông tin dưới đây chỉ phản ánh đúng những gì hiện có trong các tệp của repository. Các mục được nhắc tới trong `README.md` hoặc có file tài nguyên nhưng chưa kèm đầy đủ văn bản giấy phép được đánh dấu rõ để kiểm tra và bổ sung.

---

## 1. Nền tảng Upstream & SDK

| Thành phần | Giấy phép | Nguồn | Đường dẫn trong repo |
|---|---|---|---|
| **CrossPoint Reader** (nền tảng firmware gốc & các bản vá tối ưu) | MIT (`Copyright (c) 2025 Dave Allie`, `Copyright (c) 2025 CrossPoint Reader organization`) | `https://github.com/crosspoint-reader/crosspoint-reader` *(theo `README.md`)* | `LICENSE`, `src/`, `lib/` |
| **CrossInk** (nhánh nền tảng v2.0.0-rc / v1.6.1 được kế thừa) | MIT *(kế thừa theo `LICENSE` và `README.md`; chưa có tệp notice riêng của CrossInk)* | `https://github.com/uxjulia/CrossInk` *(theo `README.md`)* | `README.md`, `src/`, `lib/` |
| **FreeInk SDK** | MIT (`Copyright (c) 2026 FreeInk`) | Submodule `freeink-sdk` | `freeink-sdk/LICENSE`, `freeink-sdk/NOTICE` |
| **OpenX4 E-Paper Community SDK** (nhúng & ghi công trong `freeink-sdk`) | MIT (`Copyright (c) 2025 Open X4 E-Paper Contributors`, tác giả driver gốc `CidVonHighwind`) | `https://github.com/open-x4-epaper/community-sdk` *(theo `freeink-sdk/NOTICE`)* | `freeink-sdk/NOTICE`, `freeink-sdk/libs/display/FreeInkDisplay/` |
| **FreeInk OpdsParser** | MIT (`Copyright (c) 2026 FreeInk`) | FreeInk SDK (`LICENSE.FreeInk`) | `lib/OpdsParser/LICENSE.FreeInk` |
| **PaperPaw** (`lut_factory_quality`, `MAX_WDT_WORK_MS`, `DEFAULT_RELEASE_WAIT_MS`) | **Cần xác minh / bổ sung** *(README ghi `Apache-2.0` của Hannes Thoene, nhưng **không có** tệp `LICENSE` của PaperPaw đi kèm trong repo)* | `paperpaw-main` *(theo `README.md` và `src/util/TaskWatchdog.h`)* | `src/util/TaskWatchdog.h`, `freeink-sdk/libs/display/FreeInkDisplay/src/lut/Ssd1677Luts.h`, `freeink-sdk/libs/hardware/PowerManager/include/PowerManager.h` |

---

## 2. Thư viện bên thứ ba trong `lib/` và `src/`

| Thành phần | Giấy phép | Nguồn | Đường dẫn trong repo |
|---|---|---|---|
| **Expat (`2.7.3`)** | MIT (`Copyright (c) 1997-2000 Thai Open Source Software Center Ltd`, `2000 Clark Cooper`, `2001-2025 Expat maintainers` trong từng tệp nguồn; `siphash.h` theo CC0 Public Domain Dedication) | `libexpat` *(theo header trong `lib/expat/expat.h` và `library.json`)* | `lib/expat/` *(Lưu ý: tệp `COPYING` nằm ở `freeink-sdk/libs/book/FreeInkBook/third_party/expat/COPYING`, chưa có bản sao trong `lib/expat/`)* |
| **miniz (`3.1.2` / `tinfl`)** | MIT (`Copyright 2013-2014 RAD Game Tools and Valve Software`, `Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC` trong `miniz.c`; `miniz.h` ghi chú tham chiếu statement unlicense ở cuối tệp nhưng phần cuối đã bị lược bớt) | Rich Geldreich / `miniz` (`library.json` ghi `11.3.2` cho wrapper vendored) | `lib/miniz/third_party/miniz.c`, `lib/miniz/third_party/miniz.h`, `lib/miniz/src/` |
| **uzlib (`2.9.8`)** | zlib License (`Copyright (c) 2003 Joergen Ibsen / Jibz`, `Copyright (c) 2014-2018 Paul Sokolovsky`) | `https://github.com/pfalcon/uzlib.git` *(theo `lib/uzlib/library.json`)* | `lib/uzlib/src/` |
| **MiniBidi** | MIT *(header ghi dẫn xuất từ `mintty` của Thomas Wolff và Ahmad Khalifa)* | `https://github.com/mintty/mintty/` *(theo `lib/MiniBidi/minibidi.h`)* | `lib/MiniBidi/minibidi.h`, `lib/MiniBidi/minibidi.c`, `lib/MiniBidi/*.t` |
| **HTML Entities Table (`htmlEntities.cpp`)** | **Cần xác minh / bổ sung** *(chỉ ghi chú `based on https://github.com/atomic14/diy-esp32-epub-reader/...`, chưa có tệp license riêng của phần trích từ `diy-esp32-epub-reader`)* | `https://github.com/atomic14/diy-esp32-epub-reader` | `lib/Epub/Epub/htmlEntities.h`, `lib/Epub/Epub/htmlEntities.cpp` |
| **Định dạng XTC/XTCH (`lib/Xtc`)** | Tham chiếu đặc tả kỹ thuật *(mã nguồn triển khai thuộc CrossPoint Reader dưới giấy phép MIT của repo)* | `https://gist.github.com/CrazyCoder/b125f26d6987c0620058249f59f1327d` *(theo `lib/Xtc/README` & `XtcTypes.h`)* | `lib/Xtc/README`, `lib/Xtc/Xtc/XtcTypes.h` |
| **JSZip (`v3.10.1`, nhúng pako)** | Dual License: **MIT** hoặc **GPLv3** (`(c) 2009-2016 Stuart Knightley`; `pako` theo **MIT**) — *header trong tệp minified trỏ tới URL license upstream, chưa có tệp license rời trong repo* | `http://stuartk.com/jszip` / `https://github.com/Stuk/jszip` | `src/network/html/js/jszip.min.js`, `src/network/html/js/jszip_minJs.generated.h` |

---

## 3. Tài nguyên Hyphenation trong `assets/`

| Thành phần | Giấy phép | Nguồn | Đường dẫn trong repo |
|---|---|---|---|
| **Typst Hypher & bộ bảng CPHY v1** | **MIT** và **Apache-2.0** *(kèm `LICENSE-MIT` và `LICENSE-APACHE`; định dạng CPHY từ CrossPoint PR #3706 theo MIT)* | `https://github.com/typst/hypher` (`d81cc506416bffef2a75ee2dc969d4b41bc36613`) | `assets/hyphenation/README.md`, `assets/hyphenation/LICENSE-MIT`, `assets/hyphenation/LICENSE-APACHE` |
| **TeX Hyphenation Patterns (`de-1996`, `es`, `fr`, `it`, `pl`, `pt`, `ru`, `sv`, `uk`)** | Giấy phép riêng ghi trong phần đầu từng tệp `.tex` (ví dụ `MIT/X11` cho `hyph-es.tex`, `LPPL` cho `hyph-ru.tex` và `hyph-uk.tex`) | `hyph-utf8` *(theo `assets/hyphenation/README.md` và header `.tex`)* | `assets/hyphenation/patterns/*.tex` |

---

## 4. Phông chữ trong `lib/EpdFont/` (Kiểm tra trạng thái giấy phép OFL)

Các phông chữ được lưu dưới dạng nguồn `.ttf`/`.otf` trong `lib/EpdFont/builtinFonts/source/`, `lib/EpdFont/scalableFonts/`, hoặc được nhúng trực tiếp thành header `.h` vào firmware (`lib/EpdFont/builtinFonts/`).

### 4.1. Đã có bản văn bản giấy phép OFL trong repo

| Phông chữ | Giấy phép & Chủ sở hữu ghi trong tệp | Đường dẫn trong repo | Tình trạng tệp giấy phép |
|---|---|---|---|
| **Atkinson Hyperlegible / Next** | SIL OFL-1.1 (`Copyright 2020 Braille Institute of America, Inc.`) | `lib/EpdFont/builtinFonts/source/AtkinsonHyperlegible/` | **Đầy đủ** (`OFL.txt` nằm cùng thư mục nguồn) |
| **Bitter** *(biên dịch sẵn `bitter_*` trong firmware + `.ttf` scalable)* | SIL OFL-1.1 (`Copyright 2011 The Bitter Project Authors`, RFN `"Bitter Pro"`) | `lib/EpdFont/builtinFonts/source/Bitter/`, `lib/EpdFont/scalableFonts/bitter_*.ttf`, `lib/EpdFont/builtinFonts/bitter_*.h` | **Có bản đầy đủ** tại `lib/EpdFont/scalableFonts/licenses/Bitter-OFL.txt` *(chưa đặt bản sao trong `builtinFonts/source/Bitter/`)* |
| **ChareInk7** *(kết hợp fallback cho `bitter_*`, `lexenddeca_*`)* | SIL OFL-1.1 (`Copyright (c) 1997-2022 SIL International`, RFN `"Charis"`, `"SIL"`) | `lib/EpdFont/builtinFonts/source/ChareInk7/` | **Có bản đầy đủ** tại `lib/EpdFont/scalableFonts/licenses/ChareInk7-OFL.txt` *(chưa đặt bản sao trong `builtinFonts/source/ChareInk7/`)* |
| **Lexend Deca** *(biên dịch sẵn `lexenddeca_*` trong firmware + `.ttf` scalable)* | SIL OFL-1.1 (`Copyright 2018 The Lexend Project Authors`, RFN `"RevReading Lexend"`) | `lib/EpdFont/builtinFonts/source/LexendDeca/`, `lib/EpdFont/scalableFonts/lexenddeca_*.ttf`, `lib/EpdFont/builtinFonts/lexenddeca_*.h` | **Có bản đầy đủ** tại `lib/EpdFont/scalableFonts/licenses/LexendDeca-OFL.txt` *(chưa đặt bản sao trong `builtinFonts/source/LexendDeca/`)* |
| **Noto Sans** | SIL OFL-1.1 (`Copyright 2022 The Noto Project Authors`) | `lib/EpdFont/builtinFonts/source/NotoSans/` | **Đầy đủ** (`OFL.txt` nằm cùng thư mục nguồn) |
| **Noto Sans Arabic** *(dùng trong `inter_*.h`)* | SIL OFL-1.1 (`Copyright 2022 The Noto Project Authors`) | `lib/EpdFont/builtinFonts/source/NotoSansArabic/` | **Đầy đủ** (`OFL.txt` nằm cùng thư mục nguồn) |
| **Noto Sans CJK SC** *(kết hợp glyph CJK cho `lexenddeca_16_regular.h`)* | SIL OFL-1.1 (`Copyright 2014-2021 Adobe`, RFN `"Source"`) | `lib/EpdFont/builtinFonts/source/NotoSansCJKsc/` | **Có bản đầy đủ** tại `lib/EpdFont/scalableFonts/licenses/NotoSansCJKsc-OFL.txt` |

### 4.2. Phông chữ thiếu hoặc chưa có toàn văn OFL — Cần kiểm tra & bổ sung

| Phông chữ | Đường dẫn trong repo | Mức độ sử dụng | Trạng thái & Những gì cần thêm |
|---|---|---|---|
| **Lexica Ultralegible** | `lib/EpdFont/builtinFonts/source/LexicaUltralegible/` | Dùng trực tiếp làm nguồn chính tạo các header `inter_*.h` (UI tiếng Việt trong firmware) | **Cần xác minh / bổ sung:** Không có tệp `OFL.txt` trong thư mục; `README.md` ghi đây là fork của Atkinson Hyperlegible (`OFL-1.1`, `https://github.com/lexica-fonts/lexica-ultralegible`). **Cần thêm:** Tệp `OFL.txt` chính thức từ repo `lexica-ultralegible`. |
| **Inter** | `lib/EpdFont/builtinFonts/source/Inter/` | Dùng trực tiếp làm nguồn tạo các header `inter_*.h` trong firmware | **Thiếu toàn văn OFL:** Không có tệp `OFL.txt` / `LICENSE.txt` nào đi kèm trong `builtinFonts/source/Inter/` hay `scalableFonts/licenses/`. **Cần thêm:** Tệp `OFL.txt` của The Inter Project Authors (SIL OFL-1.1). |
| **IBM Plex Sans Hebrew** | `lib/EpdFont/builtinFonts/source/IBMPlexSansHebrew/` | Dùng trực tiếp cho dải glyph Hebrew (`0x05D0..0x05EA`) trong `inter_*.h` | **Thiếu toàn văn OFL:** Không có tệp `OFL.txt` / `LICENSE.txt` đi kèm trong repo. **Cần thêm:** Tệp `OFL.txt` của IBM Plex Sans Hebrew (SIL OFL-1.1). |
| **Noto Sans Symbols & Noto Sans Symbols 2** | `lib/EpdFont/builtinFonts/source/NotoSymbols/`, `NotoSymbols2/`, `lib/EpdFont/scalableFonts/licenses/NotoSymbols.txt` | Dùng tạo ký tự bổ sung cho `bitter_*`, `lexenddeca_*` và `ui_symbols_10.h` | **Thiếu toàn văn OFL:** `NotoSymbols.txt` hiện chỉ là tệp tóm tắt 5 dòng (`Copyright 2022 The Noto Project Authors`, trỏ URL `https://scripts.sil.org/OFL`) mà **chưa kèm toàn văn điều khoản SIL OFL 1.1**. **Cần thêm:** Toàn văn điều khoản SIL OFL 1.1 vào `NotoSymbols.txt` (hoặc `OFL.txt`). |
| **Noto Emoji** | `lib/EpdFont/builtinFonts/source/NotoEmoji/NotoEmoji-Regular.ttf` | Không xuất hiện trong lệnh `fontconvert.py` của các header `builtinFonts/*.h`, nhưng tệp `.ttf` có sẵn trong repo | **Thiếu toàn văn OFL:** Không có tệp `OFL.txt` đi kèm trong thư mục. **Cần thêm:** Tệp `OFL.txt` của The Noto Project Authors (SIL OFL-1.1) hoặc gỡ tệp `.ttf` nếu không dùng. |

---

## 5. Thành phần bên thứ ba bên trong submodule `freeink-sdk/`

| Thành phần | Giấy phép | Nguồn | Đường dẫn trong repo | Ghi chú sử dụng |
|---|---|---|---|---|
| **FreeType 2** | Dual License: **FTL** (FreeType License — *có điều khoản ghi công/attribution*) hoặc **GPLv2** | `freeink-sdk/libs/font/FreeInkFont/third_party/freetype/LICENSE.TXT` | `freeink-sdk/libs/font/FreeInkFont/third_party/freetype/` | Được link trên X4 Classic & X4 Pro (`CROSSINK_SCALABLE_FONTS=1`). *Lưu ý:* tệp `docs/FTL.TXT` được nhắc trong `LICENSE.TXT` không có mặt trong `freeink-sdk/.../freetype/`. |
| **stb_truetype (`v1.26`)** | Public Domain / MIT (dual choice) | `stb_truetype.h` | `freeink-sdk/libs/font/FreeInkFont/third_party/stb/stb_truetype.h`, `freeink-sdk/libs/book/FreeInkBook/third_party/stb/stb_truetype.h` | — |
| **TJpgDec (`R0.03`)** | Free software license (`Copyright (C) 2021, ChaN`, giữ copyright notice khi phân phối mã nguồn) | `tjpgd.c` | `freeink-sdk/libs/book/FreeInkBook/third_party/tjpgd/` | Chỉ dùng trong `FreeInkBook` (CrossImple dùng `JPEGDEC` của bitbank2) |
| **pngle** | MIT (`Copyright (c) 2019 kikuchan`) | `freeink-sdk/.../pngle/LICENSE` | `freeink-sdk/libs/book/FreeInkBook/third_party/pngle/` | Chỉ có trong `FreeInkBook` |
| **libunibreak** | zlib/libpng License (`Wu Yongwei`, `Tom Hacohen`, `Petr Filipsky`, `Andreas Röver`) | `freeink-sdk/.../libunibreak/LICENCE` | `freeink-sdk/libs/book/FreeInkBook/third_party/libunibreak/` | Chỉ có trong `FreeInkBook` |
| **Unicode Character Database (`Arabic*.txt`)** | Unicode Terms of Use (`https://www.unicode.org/terms_of_use.html`) | `ArabicShaping.txt` (Unicode, Inc.) | `freeink-sdk/libs/book/FreeInkBook/third_party/unicode/` | Dữ liệu tra cứu/tham khảo |
| **miniz & Expat & hyphen-patterns (`en-us`)** trong `freeink-sdk` | MIT (`miniz`, `expat`), Notice-preserving (`hyph-en-us.pat.txt`, `Copyright Gerard D.C. Kuiken`) | `ContentProtection/third_party/`, `FreeInkBook/third_party/` | `freeink-sdk/libs/book/ContentProtection/third_party/miniz/`, `freeink-sdk/libs/book/FreeInkBook/third_party/{expat,miniz,hyphen-patterns}/` | `ContentProtection` tham gia build; `FreeInkBook` không build trực tiếp ở CrossImple |
| **Bosch Sensortec `BMI270_SensorAPI` (`v2.86.1`)** | BSD-3-Clause (`Copyright (c) 2023 Bosch Sensortec GmbH`) | `https://github.com/boschsensortec/BMI270_SensorAPI` | `freeink-sdk/libs/hardware/Imu/src/Bmi270ConfigFile.h` | **Không còn biên dịch vào firmware** từ CrossImple `v0.25` (`scripts/prune_freeink_sdk.py` bỏ `Imu.cpp`) |
| **Lucide Icons** *(mô tả trong `Icons/library.json`)* | **Cần xác minh / bổ sung** *(thư mục `freeink-sdk/libs/assets/Icons/lucide/` đang trống và không có tệp `LICENSE` của Lucide)* | `Icons/library.json` | `freeink-sdk/libs/assets/Icons/` | Header `Icon.h` thuộc `freeink-sdk` (MIT) |

---

## 6. Bộ dữ liệu kiểm thử (`test/`)

| Thành phần | Giấy phép | Nguồn | Đường dẫn trong repo |
|---|---|---|---|
| **Font Awesome (fixture test)** | `Expat` (MIT) & `OFL-1.1` *(tóm tắt Debian copyright; không biên dịch vào firmware)* | `https://github.com/FortAwesome/Font-Awesome` | `test/otf_fonts/fixtures/FontAwesome-LICENSE.txt` |
| **DejaVu / Bitstream Vera (fixture test)** | Bitstream Vera / Arev / Public Domain *(chỉ dùng cho test trong `FreeInkBook`, không biên dịch vào firmware)* | `DEJAVU-LICENSE` | `freeink-sdk/libs/book/FreeInkBook/test/fixtures/fonts/DEJAVU-LICENSE` |

---

## 7. Thư viện bên thứ ba khai báo qua PlatformIO (`platformio.ini`)

Các thư viện sau không lưu mã nguồn cố định trong Git của repo mà được PlatformIO tải về vào `.pio/libdeps/` (đã gitignore) khi biên dịch firmware theo [platformio.ini](platformio.ini):

* `greiman/SdFat @ 2.3.1` *(kèm các bản vá local trong `scripts/sdfat_patches/`)*
* `bblanchon/ArduinoJson @ 7.4.2`
* `ricmoo/QRCode @ 0.0.1`
* `bitbank2/PNGdec @ 1.1.6`
* `bitbank2/JPEGDEC` (`#86282979224c8a32fd51e091ed5a35b0c699a52b`)
* `bitbank2/AnimatedGIF` (`#67cfacafe12ef4926c7713342c57d2a4af040e5e`, *kèm `scripts/patch_animatedgif.py`*)
* `links2004/WebSockets @ 2.7.3` (*kèm `scripts/patch_websockets.py`*)
* `wolfssl/Arduino-wolfSSL @ 5.7.2` (*kèm `scripts/patch_wolfssl.py`*)

> *(Cần xác minh/đối chiếu tệp `LICENSE` cụ thể của từng thư viện ngoài này trong `.pio/libdeps/` nếu đóng gói kèm bản phân phối mã nguồn đầy đủ.)*
