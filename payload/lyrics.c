/* SPDX-License-Identifier: GPL-3.0-only */

#include "lyrics.h"
#include "osos.h"
#include "dark_mode.h"

static struct cfw_lyrics_state global_lyrics;
static uint32_t last_tick_sec;

int cfw_lyrics_is_active(void)
{
    return global_lyrics.has_synced;
}

void cfw_lyrics_reset_active(void)
{
    cfw_lyrics_reset(&global_lyrics);
}

void cfw_lyrics_scroll(int delta, uint32_t current_ms)
{
    cfw_lyrics_on_wheel(&global_lyrics, delta, current_ms);
}

int cfw_lyrics_has_timestamp_header(const char *raw)
{
    if (!raw)
        return 0;
    while (*raw == ' ' || *raw == '\t' || *raw == '\r' || *raw == '\n')
        raw++;
    if (*raw != '[')
        return 0;

    for (int tag = 0; tag < 6; tag++) {
        if (*raw != '[')
            break;
        if (raw[1] >= '0' && raw[1] <= '9' && (raw[3] == ':' || raw[4] == ':'))
            return 1;
        const char *close = raw;
        while (*close && *close != ']' && *close != '\n')
            close++;
        if (*close == ']') {
            raw = close + 1;
            while (*raw == ' ' || *raw == '\t' || *raw == '\r' || *raw == '\n')
                raw++;
        } else {
            break;
        }
    }
    return 0;
}

static uint32_t cfw_strlen(const char *s)
{
    uint32_t len = 0;
    if (!s)
        return 0;
    while (s[len])
        len++;
    return len;
}

const char *cfw_lyrics_get_display_text(const char *raw, uint32_t len)
{
    if (!raw)
        return raw;

    if (!cfw_lyrics_has_timestamp_header(raw))
        return raw;

    if (!len)
        len = cfw_strlen(raw);
    if (len < 8)
        return raw;

    if (!global_lyrics.has_synced ||
        global_lyrics.lines[0].text < raw ||
        global_lyrics.lines[0].text >= raw + len) {
        cfw_lyrics_parse(raw, len, &global_lyrics);
    }

    if (global_lyrics.has_synced) {
        struct osos_player *player = osos_player_get();
        uint32_t ms = player ? osos_player_get_position(player) : 0;
        return cfw_lyrics_format_window(&global_lyrics, ms);
    }

    return raw;
}

void cfw_lyrics_get_line_text(const struct cfw_lyrics_state *state, int32_t index,
                             char *buf, uint32_t max_len, int is_active)
{
    if (!buf || max_len == 0)
        return;
    buf[0] = '\0';
    if (!state || !state->has_synced || state->count == 0)
        return;

    uint32_t pos = 0;
    if (index < 0) {
        const char *intro = "   (Intro)";
        while (intro[pos] && pos + 1 < max_len) {
            buf[pos] = intro[pos];
            pos++;
        }
        buf[pos] = '\0';
        return;
    }

    if (index >= (int32_t)state->count) {
        const char *outro = "   (Outro)";
        while (outro[pos] && pos + 1 < max_len) {
            buf[pos] = outro[pos];
            pos++;
        }
        buf[pos] = '\0';
        return;
    }

    if (is_active) {
        /* UTF-8 '▶ ' is 0xe2, 0x96, 0xb6, ' ' */
        buf[pos++] = (char)0xe2;
        buf[pos++] = (char)0x96;
        buf[pos++] = (char)0xb6;
        buf[pos++] = ' ';
    } else {
        buf[pos++] = ' ';
        buf[pos++] = ' ';
        buf[pos++] = ' ';
    }

    const struct cfw_lyric_entry *entry = &state->lines[index];
    for (uint32_t i = 0; i < entry->len && pos + 1 < max_len; i++) {
        buf[pos++] = entry->text[i];
    }
    buf[pos] = '\0';
}

static void format_two_digits(char *dest, uint32_t val)
{
    dest[0] = (char)('0' + (val / 10));
    dest[1] = (char)('0' + (val % 10));
}

void cfw_lyrics_format_time_line(char *buf, uint32_t max_len,
                                uint32_t current_ms, uint32_t duration_ms)
{
    if (!buf || max_len < 64)
        return;

    uint32_t elapsed_sec = current_ms / 1000;
    uint32_t pos = 0;

    format_two_digits(&buf[pos], elapsed_sec / 60);
    pos += 2;
    buf[pos++] = ':';
    format_two_digits(&buf[pos], elapsed_sec % 60);
    pos += 2;

    if (duration_ms > current_ms) {
        uint32_t remain_sec = (duration_ms - current_ms) / 1000;
        while (pos < 36 && pos + 8 < max_len)
            buf[pos++] = ' ';
        buf[pos++] = '-';
        format_two_digits(&buf[pos], remain_sec / 60);
        pos += 2;
        buf[pos++] = ':';
        format_two_digits(&buf[pos], remain_sec % 60);
        pos += 2;
    }

    buf[pos] = '\0';
}

#ifndef HOST_TEST
static void *get_bold_font(void)
{
    static void *cached_bold_font;
    if (!cached_bold_font) {
        void *bank = osos_ui_resources();
        if (bank) {
            void *desc = osos_resource_service_get(bank, 0x464f4e54, 0x0dad0012);
            if (desc) {
                typedef void *(*fn)(void *);
                cached_bold_font = ((fn)OSOS_FONT_LOOKUP)(desc);
            }
        }
    }
    return cached_bold_font;
}

static void draw_progress_bar(void *view, uint32_t current_ms, uint32_t duration_ms)
{
    if (!view || !duration_ms)
        return;

    void *gc = (uint8_t *)view + 0xa4;
    int dark = cfw_dark_mode_enabled();

    unsigned char color[4];
    color[0] = dark ? 0x40 : 0xd0;
    color[1] = color[0];
    color[2] = color[0];
    color[3] = 0xff;

    int32_t bar_x = 24;
    int32_t bar_w = 272;
    int32_t bar_y = 154;
    int32_t bar_h = 3;

    int32_t rect_buf[4];
    rect_buf[0] = bar_x;
    rect_buf[1] = bar_y;
    rect_buf[2] = bar_x + bar_w;
    rect_buf[3] = bar_y + bar_h;

    int32_t fill_w = (int32_t)(((uint64_t)current_ms * bar_w) / duration_ms);
    if (fill_w > bar_w)
        fill_w = bar_w;

    typedef void (*set_color_fn)(void *, const unsigned char *);
    typedef void (*set_mode_fn)(void *, uint32_t);
    typedef void (*fill_rect_fn)(void *, const int32_t *);

    /* 1. Track */
    ((set_color_fn)OSOS_GRAPHICS_SET_COLOR)(gc, color);
    ((set_mode_fn)OSOS_GRAPHICS_SET_MODE)(gc, 0);
    ((fill_rect_fn)OSOS_GRAPHICS_FILL_RECT)(gc, rect_buf);

    if (fill_w > 0) {
        /* 2. Fill */
        color[0] = dark ? 0x00 : 0x23;
        color[1] = dark ? 0xa1 : 0x6c;
        color[2] = dark ? 0xff : 0xc5;
        rect_buf[2] = bar_x + fill_w;
        ((set_color_fn)OSOS_GRAPHICS_SET_COLOR)(gc, color);
        ((fill_rect_fn)OSOS_GRAPHICS_FILL_RECT)(gc, rect_buf);

        /* 3. Thumb */
        color[0] = 0xff;
        color[1] = 0xff;
        color[2] = 0xff;
        rect_buf[0] = bar_x + fill_w - 2;
        rect_buf[1] = bar_y - 2;
        rect_buf[2] = bar_x + fill_w + 3;
        rect_buf[3] = bar_y + bar_h + 2;
        ((set_color_fn)OSOS_GRAPHICS_SET_COLOR)(gc, color);
        ((fill_rect_fn)OSOS_GRAPHICS_FILL_RECT)(gc, rect_buf);
    }

    ((set_mode_fn)OSOS_GRAPHICS_SET_MODE)(gc, 0x21);
}
#else
static void *get_bold_font(void)
{
    return (void *)0xb01df007;
}

static void draw_progress_bar(void *view, uint32_t current_ms, uint32_t duration_ms)
{
    (void)view;
    (void)current_ms;
    (void)duration_ms;
}
#endif

int cfw_text_view_draw_original(void *view, const char *text, void *context);

PATCH_ARM int cfw_text_view_draw(void *view, const char *text, void *context)
{
    if (!text || !cfw_lyrics_has_timestamp_header(text))
        return cfw_text_view_draw_original(view, text, context);

    cfw_lyrics_get_display_text(text, 0);
    if (!global_lyrics.has_synced || !view) {
        const char *win = cfw_lyrics_get_display_text(text, 0);
        return cfw_text_view_draw_original(view, win, context);
    }

    struct osos_player *player = osos_player_get();
    uint32_t current_ms = player ? osos_player_get_position(player) : 0;
    uint32_t duration_ms = 0;
#ifndef HOST_TEST
    if (player) {
        struct osos_media_item item;
        osos_media_item_init(&item);
        osos_player_current_item(player, &item);
        if (osos_media_item_valid(&item))
            duration_ms = osos_media_item_duration(&item);
    }
#else
    extern uint32_t mock_playback_duration;
    duration_ms = mock_playback_duration;
#endif

    if (global_lyrics.user_scrolled) {
        if (current_ms >= global_lyrics.last_user_scroll_ms &&
            (current_ms - global_lyrics.last_user_scroll_ms) > CFW_LYRICS_USER_SCROLL_TIMEOUT_MS) {
            global_lyrics.user_scrolled = 0;
            global_lyrics.scroll_offset = 0;
        }
    }

    global_lyrics.active_index = cfw_lyrics_find_active(&global_lyrics, current_ms);
    int32_t target = (int32_t)global_lyrics.active_index;
    if (global_lyrics.user_scrolled) {
        target += global_lyrics.scroll_offset;
        if (target < 0)
            target = 0;
        if (target >= (int32_t)global_lyrics.count)
            target = (int32_t)global_lyrics.count - 1;
    }

    char prev_buf[256];
    char active_buf[256];
    char next_buf[256];
    char time_buf[64];

    cfw_lyrics_get_line_text(&global_lyrics, target - 1, prev_buf, sizeof(prev_buf), 0);
    cfw_lyrics_get_line_text(&global_lyrics, target, active_buf, sizeof(active_buf), 1);
    cfw_lyrics_get_line_text(&global_lyrics, target + 1, next_buf, sizeof(next_buf), 0);
    cfw_lyrics_format_time_line(time_buf, sizeof(time_buf), current_ms, duration_ms);

    int32_t *rect = (int32_t *)((uint8_t *)view + 0x80);
    void **font_ptr = (void **)((uint8_t *)view + 256);
    int32_t orig_x = rect[0], orig_y = rect[1], orig_w = rect[2], orig_h = rect[3];
    void *orig_font = *font_ptr;
    void *bold_font = get_bold_font();

    /* Pass 1: Previous line (small regular font) */
    rect[1] = orig_y + 6;
    rect[3] = 26;
    *font_ptr = orig_font;
    cfw_text_view_draw_original(view, prev_buf, context);

    /* Pass 2: Active line (larger bold font!) */
    rect[1] = orig_y + 36;
    rect[3] = 52;
    *font_ptr = bold_font ? bold_font : orig_font;
    cfw_text_view_draw_original(view, active_buf, context);

    /* Pass 3: Next line (small regular font) */
    rect[1] = orig_y + 92;
    rect[3] = 26;
    *font_ptr = orig_font;
    cfw_text_view_draw_original(view, next_buf, context);

    /* Pass 4: Time indicators */
    rect[1] = orig_y + 128;
    rect[3] = 18;
    *font_ptr = orig_font;
    cfw_text_view_draw_original(view, time_buf, context);

    /* Restore view geometry and font */
    rect[0] = orig_x;
    rect[1] = orig_y;
    rect[2] = orig_w;
    rect[3] = orig_h;
    *font_ptr = orig_font;

    /* Pass 5: Progress bar graphical render */
    draw_progress_bar(view, current_ms, duration_ms);

    return 1;
}

void cfw_lyrics_tick(void)
{
    if (!global_lyrics.has_synced)
        return;

    struct osos_player *player = osos_player_get();
    if (!player)
        return;

    uint32_t ms = osos_player_get_position(player);
    uint32_t active = cfw_lyrics_find_active(&global_lyrics, ms);
    uint32_t sec = ms / 1000;

    if (active != global_lyrics.active_index || sec != last_tick_sec) {
        global_lyrics.active_index = active;
        last_tick_sec = sec;
        osos_ui_invalidate();
    }
}

void cfw_lyrics_reset(struct cfw_lyrics_state *state)
{
    if (!state)
        return;
    state->has_synced = 0;
    state->count = 0;
    state->active_index = 0;
    state->user_scrolled = 0;
    state->scroll_offset = 0;
    state->last_user_scroll_ms = 0;
    state->window_buffer[0] = '\0';
    last_tick_sec = 0;
}

int cfw_lyrics_parse(const char *raw, uint32_t len, struct cfw_lyrics_state *state)
{
    if (!raw || !len || !state)
        return 0;

    cfw_lyrics_reset(state);
    uint32_t i = 0;

    while (i < len) {
        /* Skip leading whitespace and blank lines. */
        while (i < len && (raw[i] == ' ' || raw[i] == '\t' || raw[i] == '\r' || raw[i] == '\n'))
            i++;
        if (i >= len)
            break;

        if (raw[i] == '[') {
            uint32_t tag_start = i + 1;
            uint32_t cur = tag_start;
            uint32_t mm = 0, ss = 0, frac = 0, frac_digits = 0;
            int valid_ts = 0;

            /* Parse minutes. */
            while (cur < len && raw[cur] >= '0' && raw[cur] <= '9') {
                mm = mm * 10 + (uint32_t)(raw[cur] - '0');
                cur++;
            }

            if (cur > tag_start && cur < len && raw[cur] == ':') {
                cur++;
                uint32_t sec_start = cur;
                while (cur < len && raw[cur] >= '0' && raw[cur] <= '9') {
                    ss = ss * 10 + (uint32_t)(raw[cur] - '0');
                    cur++;
                }

                if (cur > sec_start && cur < len) {
                    if (raw[cur] == '.' || raw[cur] == ':') {
                        cur++;
                        while (cur < len && raw[cur] >= '0' && raw[cur] <= '9') {
                            frac = frac * 10 + (uint32_t)(raw[cur] - '0');
                            frac_digits++;
                            cur++;
                        }
                    }

                    if (cur < len && raw[cur] == ']') {
                        valid_ts = 1;
                        cur++; /* advance past ']' */
                    }
                }
            }

            if (valid_ts) {
                uint32_t frac_ms = 0;
                if (frac_digits == 1)
                    frac_ms = frac * 100;
                else if (frac_digits == 2)
                    frac_ms = frac * 10;
                else if (frac_digits == 3)
                    frac_ms = frac;
                else if (frac_digits > 3) {
                    while (frac_digits > 3) {
                        frac /= 10;
                        frac_digits--;
                    }
                    frac_ms = frac;
                }

                uint32_t ms = (mm * 60 + ss) * 1000 + frac_ms;
                i = cur;

                /* Trim leading spaces from lyric line. */
                while (i < len && (raw[i] == ' ' || raw[i] == '\t'))
                    i++;

                const char *line_text = &raw[i];
                uint32_t line_start = i;
                while (i < len && raw[i] != '\r' && raw[i] != '\n')
                    i++;
                uint16_t line_len = (uint16_t)(i - line_start);

                /* Trim trailing whitespace. */
                while (line_len > 0 &&
                       (line_text[line_len - 1] == ' ' || line_text[line_len - 1] == '\t'))
                    line_len--;

                if (state->count < CFW_LYRICS_MAX_LINES) {
                    state->lines[state->count].ms = ms;
                    state->lines[state->count].text = line_text;
                    state->lines[state->count].len = line_len;
                    state->count++;
                }
                continue;
            }
        }

        /* Skip remaining characters on non-timestamped line. */
        while (i < len && raw[i] != '\r' && raw[i] != '\n')
            i++;
    }

    state->has_synced = (state->count > 0);
    return state->has_synced;
}

uint32_t cfw_lyrics_find_active(const struct cfw_lyrics_state *state, uint32_t current_ms)
{
    if (!state || state->count == 0)
        return 0;

    if (current_ms < state->lines[0].ms)
        return 0;

    if (current_ms >= state->lines[state->count - 1].ms)
        return state->count - 1;

    uint32_t low = 0;
    uint32_t high = state->count - 1;
    uint32_t best = 0;

    while (low <= high) {
        uint32_t mid = low + (high - low) / 2;
        if (state->lines[mid].ms <= current_ms) {
            best = mid;
            low = mid + 1;
        } else {
            if (mid == 0)
                break;
            high = mid - 1;
        }
    }
    return best;
}

void cfw_lyrics_on_wheel(struct cfw_lyrics_state *state, int delta, uint32_t current_ms)
{
    if (!state || !state->has_synced || state->count == 0)
        return;

    state->user_scrolled = 1;
    state->last_user_scroll_ms = current_ms;
    state->scroll_offset += delta;
}

static void append_slice(char *dest, uint32_t *pos, uint32_t max_pos,
                         const char *src, uint32_t len)
{
    for (uint32_t i = 0; i < len && *pos + 1 < max_pos; i++) {
        dest[(*pos)++] = src[i];
    }
}

const char *cfw_lyrics_format_window(struct cfw_lyrics_state *state, uint32_t current_ms)
{
    if (!state)
        return "";
    if (!state->has_synced || state->count == 0)
        return "";

    /* Reset manual scroll if timeout elapsed. */
    if (state->user_scrolled) {
        if (current_ms >= state->last_user_scroll_ms &&
            (current_ms - state->last_user_scroll_ms) > CFW_LYRICS_USER_SCROLL_TIMEOUT_MS) {
            state->user_scrolled = 0;
            state->scroll_offset = 0;
        }
    }

    state->active_index = cfw_lyrics_find_active(state, current_ms);
    int32_t target = (int32_t)state->active_index;
    if (state->user_scrolled) {
        target += state->scroll_offset;
        if (target < 0)
            target = 0;
        if (target >= (int32_t)state->count)
            target = (int32_t)state->count - 1;
    }

    uint32_t k = (uint32_t)target;
    char *buf = state->window_buffer;
    uint32_t pos = 0;
    uint32_t max = sizeof(state->window_buffer);

    /* 1. Previous line (dimmed context). */
    if (k > 0) {
        append_slice(buf, &pos, max, "   ", 3);
        append_slice(buf, &pos, max, state->lines[k - 1].text, state->lines[k - 1].len);
    } else {
        append_slice(buf, &pos, max, "   (Intro)", 10);
    }
    append_slice(buf, &pos, max, "\n\n", 2);

    /* 2. Active line (focused / prominent). */
    append_slice(buf, &pos, max, "▶ ", 4); /* UTF-8 '▶ ' is 3 bytes (0xE2 0x96 0xB6) + space */
    append_slice(buf, &pos, max, state->lines[k].text, state->lines[k].len);
    append_slice(buf, &pos, max, "\n\n", 2);

    /* 3. Next line (dimmed context). */
    if (k + 1 < state->count) {
        append_slice(buf, &pos, max, "   ", 3);
        append_slice(buf, &pos, max, state->lines[k + 1].text, state->lines[k + 1].len);
    } else {
        append_slice(buf, &pos, max, "   (Outro)", 10);
    }

    buf[pos] = '\0';
    return buf;
}
