/* SPDX-FileCopyrightText: 2026 CrossImple
 * SPDX-License-Identifier: Apache-2.0
 * Desktop Theme for Metalio 4 Plus (684x1216 portrait).
 */
#include "desktop_stock_wallpaper.h"

static void desktop_draw_stock_wallpaper(uint8_t *fb, int x0, int y0) {
    const bool fast = app_settings_main_fast_refresh();
    static const uint8_t gray_map[4] = {0, 0x55, 0xaa, 0xff};
    for (int y = 0; y < DESKTOP_STOCK_WALLPAPER_H; ++y) {
        const uint8_t *row = s_desktop_wallpaper_4gray + y * DESKTOP_STOCK_WALLPAPER_STRIDE;
        for (int x = 0; x < DESKTOP_STOCK_WALLPAPER_W; ++x) {
            uint8_t val2 = (row[x >> 2] >> ((3 - (x & 3)) * 2)) & 0x03;
            uint8_t gray = fast ? (val2 >= 2 ? 0xff : 0x00) : gray_map[val2];
            epd_draw_pixel(x0 + x, y0 + y, gray, fb);
        }
    }
}

static void render_desktop_root(uint8_t *fb) {
#ifdef DPREADER_LOCAL_LATENCY
    int64_t probe_home = esp_timer_get_time();
#define DESKTOP_STAGE(name) do { int64_t probe_now = esp_timer_get_time(); ESP_LOGI("latency", "HOME_DESKTOP stage=%s us=%lld", name, (long long)(probe_now - probe_home)); probe_home = probe_now; } while (0)
#else
#define DESKTOP_STAGE(name) do {} while (0)
#endif

#if DPREADER_BOARD_METALIO47
    const uint8_t card_gray = 0xff;
#else
    const uint8_t card_gray = app_settings_main_fast_refresh() ? 0xff : 0xee;
#endif

    // 1. Wallpaper Card: (36, 92, 612, 408) — blit pre-rendered PSRAM cache if available
    EpdRect wp_rect = {36, 92, 612, 408};
    if (s_desktop_wp_ready && s_desktop_wp_buf) {
        memcpy(fb, s_desktop_wp_buf, (size_t)UI_LOCK_WIDTH * UI_LOCK_HEIGHT / 2);
        home_frame(fb, wp_rect, 20);
    } else {
        ui_fill_round_rect(fb, wp_rect, 20, UI_GRAY_WHITE);
        int wp_x = wp_rect.x + (wp_rect.width - DESKTOP_STOCK_WALLPAPER_W) / 2;
        int wp_y = wp_rect.y + (wp_rect.height - DESKTOP_STOCK_WALLPAPER_H) / 2;
        desktop_draw_stock_wallpaper(fb, wp_x, wp_y);
        home_frame(fb, wp_rect, 20);
    }
    ui_nav_status(fb);
    DESKTOP_STAGE("wallpaper");

    // 2. Stats Card: (36, 516, 612, 176)
    EpdRect stats_rect = {36, 516, 612, 176};
    ui_fill_round_rect(fb, stats_rect, 20, card_gray);
    home_frame(fb, stats_rect, 20);

    unsigned seconds = today_seconds();
    char num_str[32];
    snprintf(num_str, sizeof(num_str), "%u", seconds / 60);

    // Left Column: Today Reading Time (bounded to x=60..324)
    label_on(fb, 60, 546, 20, "Hôm nay", EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
    label_on(fb, 60, 602, 44, num_str, EPD_DRAW_ALIGN_LEFT, 0, card_gray);
    int num_w = ui_text_fixed_width_px(44, num_str);
    char unit_str[24];
    snprintf(unit_str, sizeof(unit_str), "phút đọc");
    fit(unit_str, 22, 324 - (60 + num_w + 8));
    if (unit_str[0]) label_on(fb, 60 + num_w + 8, 602, 22, unit_str, EPD_DRAW_ALIGN_LEFT, 0x50, card_gray);
    char sub_left[48];
    snprintf(sub_left, sizeof(sub_left), "Thời gian đọc trong ngày");
    fit(sub_left, 19, 264);
    label_on(fb, 60, 648, 19, sub_left, EPD_DRAW_ALIGN_LEFT, 0x70, card_gray);

    // Dotted vertical separator at x = 342
    for (int y = 536; y <= 672; y += 4) {
        epd_draw_pixel(342, y, 0x90, fb);
    }

    // Right Column: Goal & Calendar (ring at x=576, r=30 -> text bounded to x=362..534)
    label_on(fb, 362, 546, 20, "Mục tiêu", EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
    ring(fb, 576, 602, 30, 7, seconds, app_settings_home_goal_minutes() * 60);

    char goal_str[48];
    snprintf(goal_str, sizeof(goal_str), "%u phút/ngày", app_settings_home_goal_minutes());
    int goal_px = 24;
    while (goal_px > 16 && ui_text_fixed_width_px(goal_px, goal_str) > 172) --goal_px;
    fit(goal_str, goal_px, 172);
    label_on(fb, 362, 602, goal_px, goal_str, EPD_DRAW_ALIGN_LEFT, 0x20, card_gray);

    char sub_right[48];
    snprintf(sub_right, sizeof(sub_right), "Chạm xem lịch đọc ›");
    fit(sub_right, 19, 264);
    label_on(fb, 362, 648, 19, sub_right, EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
    DESKTOP_STAGE("stats");

    // 3. Book Card: (36, 708, 612, 390)
    EpdRect book_rect = {36, 708, 612, 390};
    ui_fill_round_rect(fb, book_rect, 20, card_gray);
    home_frame(fb, book_rect, 20);

    home_bold_label(fb, 60, 744, 24, "Đang đọc", EPD_DRAW_ALIGN_LEFT, card_gray);
    if (s_excerpt[0]) {
        char quote[BOOK_EXCERPT_CAP + 12];
        snprintf(quote, sizeof(quote), s_excerpt_is_chapter ? "%s" : "“%s”", s_excerpt);
        int px = 20;
        while (px > 15 && ui_text_fixed_width_px(px, quote) > 410) --px;
        if (ui_text_fixed_width_px(px, quote) <= 410)
            label_on(fb, 622, 744, px, quote, EPD_DRAW_ALIGN_RIGHT, 0x60, card_gray);
    }
    ui_draw_separator(fb, 762, 60, 564, 0x40);

    home_book_t *b = featured();
    if (b->path[0]) {
        EpdRect cover_box = {60, 780, 176, 240};
        draw_cover(fb, b, cover_box);

        int title_px = 30, author_px = 23;
        while (title_px > 18 &&
               2 * (ui_text_effective_px(title_px) + 6) + 8 +
               (ui_text_effective_px(author_px) + 6) > 948 - 782) {
            --title_px;
            if (author_px > 16) --author_px;
        }
        unsigned rows = resume_lines_in_box(fb, b->title, (EpdRect){256, 782, 366, 0},
                                            title_px, 2, false, EPD_DRAW_ALIGN_LEFT);
        int pitch = ui_text_effective_px(title_px) + 6;
        int author_y = 782 + (int)rows * pitch + 8;
        if (b->author[0]) {
            char author[128];
            snprintf(author, sizeof(author), "%s", b->author);
            fit(author, author_px, 366);
            label_on(fb, 256, author_y, author_px, author, EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
        }

        char pct_str[48];
        unsigned pct = b->has_progress ? b->progress.pct : 0;
        if (pct > 100) pct = 100;
        snprintf(pct_str, sizeof(pct_str), "Đã đọc %u%%", pct);
        label_on(fb, 256, 958, 28, pct_str, EPD_DRAW_ALIGN_LEFT, 0x20, card_gray);
        home_bold_label(fb, 622, 958, 28, "Đọc tiếp ›", EPD_DRAW_ALIGN_RIGHT, card_gray);

        EpdRect track = {256, 992, 366, 6};
        if (app_settings_main_fast_refresh()) {
            for (int x = track.x; x < track.x + track.width; x += 3)
                ui_fill_rect_fast((EpdRect){x, track.y, 1, track.height}, UI_GRAY_BLACK, fb);
        } else {
            ui_fill_round_rect(fb, track, 3, UI_GRAY_LIGHT);
        }
        int filled = track.width * (int)pct / 100;
        if (filled > 0) ui_fill_round_rect(fb, (EpdRect){track.x, track.y - 1, filled, 8}, 4, UI_GRAY_BLACK);
    } else {
        ui_draw_icon(fb, 342, 850, 48, UI_ICON_LIBRARY_BIG, 0x80);
        label_on(fb, 342, 920, 24, "Chưa có sách nào đang đọc", EPD_DRAW_ALIGN_CENTER, 0x50, card_gray);
        label_on(fb, 342, 960, 20, "Chạm vào đây để mở Tủ sách", EPD_DRAW_ALIGN_CENTER, 0x70, card_gray);
    }
    DESKTOP_STAGE("book");

    // 4. Navigation bar
    ui_nav_draw(fb, 0);
    DESKTOP_STAGE("nav");
#undef DESKTOP_STAGE
}
