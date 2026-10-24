/* SPDX-License-Identifier: GPL-3.0-only */
#include "vietnamese.h"
#include "osos.h"

void cfw_install_vietnamese(void *bank)
{
    /* Main Menu rows (indices 0..13) */
    static const char *const main_titles[] = {
        "Nhạc",                              /* 0: Music */
        "Video",                             /* 1: Videos */
        "Ảnh",                               /* 2: Photos */
        "Podcasts",                          /* 3: Podcasts */
        "Tiện ích",                          /* 4: Extras */
        "Cài đặt",                           /* 5: Settings */
        "Xáo trộn",                          /* 6: Shuffle Songs */
        "Đang phát",                         /* 7: Now Playing */
        "Nghệ sĩ",                           /* 8: Artists */
        "Album",                             /* 9: Albums */
        "Bài hát",                           /* 10: Songs */
        "Thể loại",                          /* 11: Genres */
        "Soạn giả",                          /* 12: Composers */
        "Sách nói",                          /* 13: Audiobooks */
    };

    const uint8_t *main_rows = (const uint8_t *)OSOS_MAIN_MENU_SETTINGS_ROWS;
    for (uint32_t i = 0; i < sizeof(main_titles) / sizeof(main_titles[0]); i++) {
        uint32_t title_id = *(const uint32_t *)(main_rows + i * 20 + 4);
        if (title_id) {
            const char *str = main_titles[i];
            uint32_t len = 0;
            while (str[len]) len++;
            osos_resource_set(bank, 0x53747220, title_id, str, len + 1);
        }
    }

    /* Music Menu rows (indices 0..7) */
    static const char *const music_titles[] = {
        "Cover Flow",
        "Danh sách phát",                    /* 1: Playlists */
        "Nghệ sĩ",                           /* 2: Artists */
        "Album",                             /* 3: Albums */
        "Bài hát",                           /* 4: Songs */
        "Thể loại",                          /* 5: Genres */
        "Soạn giả",                          /* 6: Composers */
        "Sách nói",                          /* 7: Audiobooks */
    };

    const uint8_t *music_rows = (const uint8_t *)OSOS_MUSIC_MENU_SETTINGS_ROWS;
    for (uint32_t i = 0; i < sizeof(music_titles) / sizeof(music_titles[0]); i++) {
        uint32_t title_id = *(const uint32_t *)(music_rows + i * 20 + 4);
        if (title_id) {
            const char *str = music_titles[i];
            uint32_t len = 0;
            while (str[len]) len++;
            osos_resource_set(bank, 0x53747220, title_id, str, len + 1);
        }
    }

    /* Common toggle strings: Bật / Tắt */
    static const char on_str[] = "Bật";
    static const char off_str[] = "Tắt";
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_ON_STRING, on_str, sizeof(on_str));
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_OFF_STRING, off_str, sizeof(off_str));
}
