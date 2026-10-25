/* SPDX-License-Identifier: GPL-3.0-only */

#include "lyrics.h"
#include "osos.h"

static struct cfw_lyrics_state global_lyrics;

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

int cfw_text_view_draw_original(void *view, const char *text, void *context);

PATCH_ARM int cfw_text_view_draw(void *view, const char *text, void *context)
{
    if (text && cfw_lyrics_has_timestamp_header(text)) {
        text = cfw_lyrics_get_display_text(text, 0);
    }
    return cfw_text_view_draw_original(view, text, context);
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

    if (active != global_lyrics.active_index) {
        global_lyrics.active_index = active;
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
