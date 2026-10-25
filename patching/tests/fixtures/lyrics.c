/* SPDX-License-Identifier: GPL-3.0-only */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../../../payload/lyrics.h"
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

    /* 7. Reset */
    cfw_lyrics_reset(&state);
    assert(state.count == 0);
    assert(state.has_synced == 0);

    return 0;
}
