/* SPDX-FileCopyrightText: 2026 CrossImple
 * SPDX-License-Identifier: Apache-2.0
 * Desktop Theme for Metalio 4 Plus (684x1216 portrait).
 */
#include "assets/desktop_stock_wallpaper.h"

static void desktop_draw_stock_wallpaper(uint8_t *fb, int x0, int y0) {
    for (int y = 0; y < DESKTOP_STOCK_WALLPAPER_H; ++y) {
        for (int x = 0; x < DESKTOP_STOCK_WALLPAPER_W; ++x) {
            int byte_idx = y * DESKTOP_STOCK_WALLPAPER_STRIDE + (x >> 2);
            int shift = (3 - (x & 3)) * 2;
            uint8_t val2 = (s_desktop_wallpaper_4gray[byte_idx] >> shift) & 0x03;
            static const uint8_t gray_map[4] = {0, 0x55, 0xaa, 0xff};
            epd_draw_pixel(x0 + x, y0 + y, gray_map[val2], fb);
        }
    }
}

static void render_desktop_root(uint8_t *fb) {
    ui_nav_status(fb);

#if DPREADER_BOARD_METALIO47
    const uint8_t card_gray = 0xff;
#else
    const uint8_t card_gray = app_settings_main_fast_refresh() ? 0xff : 0xee;
#endif

    // 1. Wallpaper Card: (36, 92, 612, 408)
    EpdRect wp_rect = {36, 92, 612, 408};
    ui_fill_round_rect(fb, wp_rect, 20, UI_GRAY_WHITE);
    home_frame(fb, wp_rect, 20);

    bool wp_drawn = false;
    const char *custom_wp = app_settings_wallpaper_path();
    if (custom_wp && custom_wp[0]) {
        wp_drawn = ui_wallpaper_draw_rounded(fb, custom_wp, wp_rect, 20);
    }
    if (!wp_drawn) {
        int wp_x = wp_rect.x + (wp_rect.width - DESKTOP_STOCK_WALLPAPER_W) / 2;
        int wp_y = wp_rect.y + (wp_rect.height - DESKTOP_STOCK_WALLPAPER_H) / 2;
        desktop_draw_stock_wallpaper(fb, wp_x, wp_y);
        home_frame(fb, wp_rect, 20);
    }

    // 2. Stats Card: (36, 516, 612, 176)
    EpdRect stats_rect = {36, 516, 612, 176};
    ui_fill_round_rect(fb, stats_rect, 20, card_gray);
    home_frame(fb, stats_rect, 20);

    unsigned seconds = today_seconds();
    char num_str[32];
    snprintf(num_str, sizeof(num_str), "%u", seconds / 60);

    // Left Column: Today Reading Time
    label_on(fb, 60, 546, 20, "Hôm nay", EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
    label_on(fb, 60, 602, 44, num_str, EPD_DRAW_ALIGN_LEFT, 0, card_gray);
    int num_w = ui_text_fixed_width_px(44, num_str);
    label_on(fb, 60 + num_w + 8, 602, 22, "phút đọc", EPD_DRAW_ALIGN_LEFT, 0x50, card_gray);
    label_on(fb, 60, 648, 19, "Thời gian đã đọc trong ngày", EPD_DRAW_ALIGN_LEFT, 0x70, card_gray);

    // Dotted vertical separator at x = 342
    for (int y = 536; y <= 672; y += 4) {
        epd_draw_pixel(342, y, 0x90, fb);
    }

    // Right Column: Goal & Calendar
    label_on(fb, 362, 546, 20, "Mục tiêu", EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
    ring(fb, 566, 602, 30, 7, seconds, app_settings_home_goal_minutes() * 60);

    char goal_str[48];
    snprintf(goal_str, sizeof(goal_str), "%u phút / ngày", app_settings_home_goal_minutes());
    label_on(fb, 362, 602, 24, goal_str, EPD_DRAW_ALIGN_LEFT, 0x20, card_gray);
    label_on(fb, 362, 648, 19, "Chạm để xem lịch đọc ›", EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);

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

        int rows = full_resume_lines(fb, b->title, 782, 30, 2, false);
        int author_y = 782 + rows * 36 + 6;
        if (b->author[0]) {
            char author[128];
            snprintf(author, sizeof(author), "%s", b->author);
            fit(author, 23, 360);
            label_on(fb, 256, author_y, 23, author, EPD_DRAW_ALIGN_LEFT, 0x60, card_gray);
        }

        char pct_str[48];
        unsigned pct = b->has_progress ? b->progress.pct : 0;
        if (pct > 100) pct = 100;
        snprintf(pct_str, sizeof(pct_str), "Đã đọc %u%%", pct);
        label_on(fb, 256, 958, 28, pct_str, EPD_DRAW_ALIGN_LEFT, 0x20, card_gray);
        home_bold_label(fb, 622, 958, 28, "Đọc tiếp ›", EPD_DRAW_ALIGN_RIGHT, card_gray);

        EpdRect track = {256, 992, 366, 6};
        ui_fill_round_rect(fb, track, 3, UI_GRAY_LIGHT);
        int filled = track.width * (int)pct / 100;
        if (filled > 0) ui_fill_round_rect(fb, (EpdRect){track.x, track.y - 1, filled, 8}, 4, UI_GRAY_BLACK);
    } else {
        ui_draw_icon(fb, 342, 850, 48, UI_ICON_LIBRARY_BIG, 0x80);
        label_on(fb, 342, 920, 24, "Chưa có sách nào đang đọc", EPD_DRAW_ALIGN_CENTER, 0x50, card_gray);
        label_on(fb, 342, 960, 20, "Chạm vào đây để mở Tủ sách", EPD_DRAW_ALIGN_CENTER, 0x70, card_gray);
    }

    // 4. Navigation bar
    ui_nav_draw(fb, 0);
}
