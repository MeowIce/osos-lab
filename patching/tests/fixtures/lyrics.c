/* SPDX-License-Identifier: GPL-3.0-only */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Native layout assertions assume 32-bit pointers; this fixture runs on the host. */
#define _Static_assert(...)
#define PATCH_ARM
#define HOST_TEST
#include "../../../payload/osos.h"
#include "../../../payload/lyrics.h"
#undef _Static_assert

static uint32_t mock_playback_ms;
uint32_t mock_playback_duration;
static int mock_invalidates;
static struct osos_player mock_player_inst;
static const char *mock_drawn_text;

static int mock_draw_passes;
static void *mock_pass_fonts[8];
static int32_t mock_pass_y[8];
static char mock_pass_texts[8][256];

int cfw_text_view_draw_original(void *view, const char *text, void *context)
{
    (void)context;
    mock_drawn_text = text;
    if (mock_draw_passes < 8) {
        if (view) {
            mock_pass_fonts[mock_draw_passes] = *(void **)((uint8_t *)view + 256);
            mock_pass_y[mock_draw_passes] = *(int32_t *)((uint8_t *)view + 0x84);
        }
        if (text) {
            strncpy(mock_pass_texts[mock_draw_passes], text, 255);
            mock_pass_texts[mock_draw_passes][255] = '\0';
        }
        mock_draw_passes++;
    }
    return 1;
}

static struct osos_player *mock_player_get(void)
{
    return &mock_player_inst;
}

static uint32_t mock_player_get_position(struct osos_player *p)
{
    (void)p;
    return mock_playback_ms;
}

static void mock_ui_invalidate(void)
{
    mock_invalidates++;
}

#define osos_player_get mock_player_get
#define osos_player_get_position mock_player_get_position
#define osos_ui_invalidate mock_ui_invalidate

#include "../../../payload/lyrics.c"

static const char test_lrc[] =
    "[ti:Sample Title]\n"
    "[ar:Sample Artist]\n"
    "[offset:0]\n"
    "\n"
    "[00:10.00] First line of song \n"
    "[00:20.50]Second line: Tiếng Việt tuyệt vời\r\n"
    "[01:05.123]Third and final line\n";

static const char static_text[] =
    "Plain static lyrics\n"
    "Without any timestamps\n"
    "Should remain unchanged.\n";

int main(void)
{
    struct cfw_lyrics_state state;

    /* 1. Static text fallback */
    int res = cfw_lyrics_parse(static_text, (uint32_t)strlen(static_text), &state);
    assert(res == 0);
    assert(state.has_synced == 0);
    assert(state.count == 0);
    assert(strcmp(cfw_lyrics_format_window(&state, 1000), "") == 0);

    /* 2. Synced LRC parsing */
    res = cfw_lyrics_parse(test_lrc, (uint32_t)strlen(test_lrc), &state);
    assert(res == 1);
    assert(state.has_synced == 1);
    assert(state.count == 3);

    assert(state.lines[0].ms == 10000);
    assert(strncmp(state.lines[0].text, "First line of song", state.lines[0].len) == 0);
    assert(state.lines[0].len == 18);

    assert(state.lines[1].ms == 20500);
    assert(strncmp(state.lines[1].text, "Second line: Tiếng Việt tuyệt vời", state.lines[1].len) == 0);

    assert(state.lines[2].ms == 65123);
    assert(strncmp(state.lines[2].text, "Third and final line", state.lines[2].len) == 0);

    /* 3. Playback timestamp tracking */
    assert(cfw_lyrics_find_active(&state, 0) == 0);
    assert(cfw_lyrics_find_active(&state, 5000) == 0);
    assert(cfw_lyrics_find_active(&state, 10000) == 0);
    assert(cfw_lyrics_find_active(&state, 15000) == 0);
    assert(cfw_lyrics_find_active(&state, 20499) == 0);
    assert(cfw_lyrics_find_active(&state, 20500) == 1);
    assert(cfw_lyrics_find_active(&state, 40000) == 1);
    assert(cfw_lyrics_find_active(&state, 65122) == 1);
    assert(cfw_lyrics_find_active(&state, 65123) == 2);
    assert(cfw_lyrics_find_active(&state, 120000) == 2);

    /* 4. Focused window rendering (Apple Music / Spotify style) */
    /* At t = 15s (Line 0 active) */
    const char *w0 = cfw_lyrics_format_window(&state, 15000);
    assert(strstr(w0, "(Intro)") != NULL);
    assert(strstr(w0, "▶ First line of song") != NULL);
    assert(strstr(w0, "Second line: Tiếng Việt") != NULL);

    /* At t = 25s (Line 1 active) */
    const char *w1 = cfw_lyrics_format_window(&state, 25000);
    assert(strstr(w1, "First line of song") != NULL);
    assert(strstr(w1, "▶ Second line: Tiếng Việt tuyệt vời") != NULL);
    assert(strstr(w1, "Third and final line") != NULL);

    /* At t = 70s (Line 2 active) */
    const char *w2 = cfw_lyrics_format_window(&state, 70000);
    assert(strstr(w2, "Second line: Tiếng Việt tuyệt vời") != NULL);
    assert(strstr(w2, "▶ Third and final line") != NULL);
    assert(strstr(w2, "(Outro)") != NULL);

    /* 5. User Wheel scroll pause & resume */
    /* At t = 25s, active is line 1. User scrolls +1 towards line 2 */
    cfw_lyrics_on_wheel(&state, 1, 25000);
    assert(state.user_scrolled == 1);
    const char *ws = cfw_lyrics_format_window(&state, 25500);
    assert(strstr(ws, "▶ Third and final line") != NULL); /* focused on line 2 */

    /* After 3.5 seconds without wheel movement, auto-scroll resumes */
    const char *wr = cfw_lyrics_format_window(&state, 29000);
    assert(state.user_scrolled == 0);
    assert(strstr(wr, "▶ Second line: Tiếng Việt tuyệt vời") != NULL); /* auto-resumed line 1 */

    /* 6. Real test file verification */
    FILE *f = fopen("1 Tán Gái 303 (Phòng Trà Version) [Explicit].lrc", "rb");
    if (f) {
        char buffer[8192];
        size_t n = fread(buffer, 1, sizeof(buffer), f);
        fclose(f);
        if (n > 0) {
            struct cfw_lyrics_state real_state;
            int ok = cfw_lyrics_parse(buffer, (uint32_t)n, &real_state);
            assert(ok == 1);
            assert(real_state.count == 49);
            assert(real_state.lines[0].ms == 25400);
            assert(strncmp(real_state.lines[0].text, "Baby girl, yuh", 14) == 0);
            assert(real_state.lines[48].ms == 165150);

            /* Check window at t = 30s (line 2 active: "Nếu em là Mary Jane, thì anh là người nhện, yuh") */
            uint32_t act = cfw_lyrics_find_active(&real_state, 30000);
            assert(act == 2);
            const char *rw = cfw_lyrics_format_window(&real_state, 30000);
            assert(strstr(rw, "▶ Nếu em là Mary Jane, thì anh là người nhện, yuh") != NULL);
        }
    }

    /* 7. High-level display & tick tests */
    mock_playback_ms = 25000;
    const char *disp = cfw_lyrics_get_display_text(test_lrc, (uint32_t)strlen(test_lrc));
    assert(disp != test_lrc);
    assert(cfw_lyrics_is_active() == 1);
    assert(strstr(disp, "▶ Second line: Tiếng Việt tuyệt vời") != NULL);

    /* Static text check */
    const char *disp_static = cfw_lyrics_get_display_text(static_text, (uint32_t)strlen(static_text));
    assert(disp_static == static_text);

    /* Tick test */
    mock_invalidates = 0;
    mock_playback_ms = 70000; /* advances to line 2 */
    cfw_lyrics_tick();
    assert(mock_invalidates == 1);

    /* 8. Header detection & TextView draw hook */
    assert(cfw_lyrics_has_timestamp_header(test_lrc) == 1);
    assert(cfw_lyrics_has_timestamp_header(static_text) == 0);
    assert(cfw_lyrics_has_timestamp_header(NULL) == 0);

    mock_drawn_text = NULL;
    cfw_text_view_draw(NULL, static_text, NULL);
    assert(mock_drawn_text == static_text);

    mock_drawn_text = NULL;
    cfw_text_view_draw(NULL, test_lrc, NULL);
    assert(mock_drawn_text != test_lrc);
    assert(strstr(mock_drawn_text, "▶ ") != NULL);

    /* 9. Line extraction and time line formatting */
    char line_buf[256];
    cfw_lyrics_get_line_text(&state, 0, line_buf, sizeof(line_buf), 1);
    assert(strstr(line_buf, "▶ First line of song") != NULL);
    cfw_lyrics_get_line_text(&state, -1, line_buf, sizeof(line_buf), 0);
    assert(strstr(line_buf, "(Intro)") != NULL);
    cfw_lyrics_get_line_text(&state, 99, line_buf, sizeof(line_buf), 0);
    assert(strstr(line_buf, "(Outro)") != NULL);

    char time_buf[64];
    cfw_lyrics_format_time_line(time_buf, sizeof(time_buf), 75000, 190000);
    assert(strstr(time_buf, "01:15") != NULL);
    assert(strstr(time_buf, "-01:55") != NULL);

    /* 10. Multi-pass TextView draw with bold font & geometry test */
    uint8_t mock_view[600] = {0};
    int32_t *vrect = (int32_t *)(mock_view + 0x80);
    void **vfont = (void **)(mock_view + 256);
    vrect[0] = 10;
    vrect[1] = 20;
    vrect[2] = 300;
    vrect[3] = 180;
    *vfont = (void *)0x12345678;

    mock_draw_passes = 0;
    mock_timer_started = 0;
    mock_playback_ms = 25000;
    mock_playback_duration = 180000;
    cfw_text_view_draw(mock_view, test_lrc, NULL);

    assert(mock_timer_started == 1);
    assert(mock_draw_passes == 4);
    /* Pass 1 (previous line): orig font */
    assert(mock_pass_fonts[0] == (void *)0x12345678);
    assert(strstr(mock_pass_texts[0], "First line of song") != NULL);
    /* Pass 2 (active line): bold font! */
    assert(mock_pass_fonts[1] == (void *)0xb01df007);
    assert(strstr(mock_pass_texts[1], "▶ Second line") != NULL);
    assert(mock_pass_y[1] == 20 + 36);
    /* Pass 3 (next line): orig font */
    assert(mock_pass_fonts[2] == (void *)0x12345678);
    assert(strstr(mock_pass_texts[2], "Third and final line") != NULL);
    /* Pass 4 (time indicator): orig font */
    assert(mock_pass_fonts[3] == (void *)0x12345678);
    assert(strstr(mock_pass_texts[3], "00:25") != NULL);
    assert(strstr(mock_pass_texts[3], "-02:35") != NULL);

    /* 10b. Apple Music style scroll animation */
    /* Advance track from line 1 to line 2 (starts at 01:05.123 = 65123ms) */
    mock_playback_ms = 65150; /* 27ms into transition */
    mock_draw_passes = 0;
    cfw_text_view_draw(mock_view, test_lrc, NULL);
    assert(global_lyrics.anim_start_ms == 65150);
    /* During animation, active line 2 should have offset_y > 0 sliding into center */
    assert(mock_pass_y[1] > 20 + 36);

    /* Settle animation after 300ms */
    mock_playback_ms = 65500;
    mock_draw_passes = 0;
    cfw_text_view_draw(mock_view, test_lrc, NULL);
    assert(global_lyrics.anim_start_ms == 0);
    assert(mock_pass_y[1] == 20 + 36);
    assert(strstr(mock_pass_texts[1], "▶ Third and final line") != NULL);

    /* Restoration of view geometry and font */
    assert(*vfont == (void *)0x12345678);
    assert(vrect[1] == 20);
    assert(vrect[3] == 180);

    /* 11. Periodic 1-second progress bar tick */
    mock_invalidates = 0;
    mock_playback_ms = 25200; /* sec changed from 70 to 25 */
    cfw_lyrics_tick();
    assert(mock_invalidates == 1);

    mock_invalidates = 0;
    mock_playback_ms = 25800; /* same second 25 */
    cfw_lyrics_tick();
    assert(mock_invalidates == 0);

    mock_playback_ms = 26050; /* new second 26 */
    cfw_lyrics_tick();
    assert(mock_invalidates == 1);

    /* Wheel scroll and reset active */
    cfw_lyrics_scroll(1, 70000);
    cfw_lyrics_reset_active();
    assert(cfw_lyrics_is_active() == 0);

    /* 12. Reset */
    cfw_lyrics_reset(&state);
    assert(state.count == 0);
    assert(state.has_synced == 0);

    return 0;
}
