/* SPDX-License-Identifier: GPL-3.0-only */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "patch.h"

#define CFW_LYRICS_MAX_LINES 256
#define CFW_LYRICS_BUFFER_SIZE 2048
#define CFW_LYRICS_USER_SCROLL_TIMEOUT_MS 3000

struct cfw_lyric_entry {
    uint32_t ms;
    const char *text;
    uint16_t len;
};

struct cfw_lyrics_state {
    int has_synced;
    uint32_t count;
    struct cfw_lyric_entry lines[CFW_LYRICS_MAX_LINES];
    uint32_t active_index;
    int user_scrolled;
    int32_t scroll_offset;
    uint32_t last_user_scroll_ms;
    char window_buffer[CFW_LYRICS_BUFFER_SIZE];
};

/* Parse lyrics buffer for LRC timestamp lines [mm:ss.xx]. Returns 1 if synced, 0 if static. */
int cfw_lyrics_parse(const char *raw, uint32_t len, struct cfw_lyrics_state *state);

/* Find active line index corresponding to playback position in milliseconds. */
uint32_t cfw_lyrics_find_active(const struct cfw_lyrics_state *state, uint32_t current_ms);

/* Format 3-line focused window (previous, active, next) into internal buffer and return it. */
const char *cfw_lyrics_format_window(struct cfw_lyrics_state *state, uint32_t current_ms);

/* Handle user click wheel scroll input. */
void cfw_lyrics_on_wheel(struct cfw_lyrics_state *state, int delta, uint32_t current_ms);

/* Reset lyrics state on track change or stop. */
void cfw_lyrics_reset(struct cfw_lyrics_state *state);

/* Return focused window text if raw is timestamped lyrics, or original text if static. */
const char *cfw_lyrics_get_display_text(const char *raw, uint32_t len);

/* Periodic tick to invalidate UI when active line changes during playback. */
void cfw_lyrics_tick(void);

/* Check if synchronized lyrics are currently active. */
int cfw_lyrics_is_active(void);

/* Check if raw text starts with LRC / timestamped lyrics tag. */
int cfw_lyrics_has_timestamp_header(const char *raw);

/* Scroll global lyrics by wheel delta. */
void cfw_lyrics_scroll(int delta, uint32_t current_ms);

/* Reset active global lyrics. */
void cfw_lyrics_reset_active(void);

/* Intercepted TextView draw routine. */
int cfw_text_view_draw(void *view, const char *text, void *context);
