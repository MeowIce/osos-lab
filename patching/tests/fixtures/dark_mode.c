/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <string.h>
/* Native layout assertions assume 32-bit pointers; this fixture runs on the host. */
#define _Static_assert(...)
#include "../../../payload/osos.h"
#include "../../../payload/dark_mode.h"
#undef _Static_assert
#define CFW_PATCH_H
#define PATCH_ARM

static uint32_t disk_record, disk_size = 4;
static int missing = 1, fail_write, writes, closes, redraws, notifications;
static int controller, surface;

static struct {
    unsigned char reserved[0x378];
    void *preferences;
} model;

static int now_playing, now_playing_available = 1, icon_notifications;
static unsigned icon_invalidations;
static unsigned char drawn[4];

static int file_open(const struct osos_path *path, int mode, void **handle)
{
    assert(!strcmp(path->text, "iPod_Control\\Device\\CFWDark"));
    if (mode == 1 && missing)
        return -1;
    *handle = &disk_record;
    return 0;
}

static int file_read(void *handle, void *out, uint32_t size, uint32_t *count)
{
    assert(handle == &disk_record && size == 4);
    memcpy(out, &disk_record, 4);
    *count = disk_size;
    return 0;
}

static int file_write(void *handle, const void *record, uint32_t size, uint32_t *count)
{
    assert(handle == &disk_record && size == 4);
    writes++;
    *count = fail_write ? 0 : size;
    if (!fail_write) {
        memcpy(&disk_record, record, size);
        missing = 0;
    }
    return 0;
}

static void file_close(void *handle)
{
    assert(handle == &disk_record);
    closes++;
}

static void *settings_model(void *value)
{
    assert(value == &controller);
    return &model;
}

static uintptr_t on_off(int value)
{
    return value ? 2 : 1;
}

static void *now_playing_model(void)
{
    return now_playing_available ? &now_playing : NULL;
}

static void invalidate(void)
{
    assert(icon_invalidations == (now_playing_available ? 3u : 0u));
    icon_invalidations = 0;
    redraws++;
}

static void notify(void *value, uint32_t type, uint32_t property)
{
    if (value == &now_playing) {
        assert(type == OSOS_RESOURCE_DRAW);
        assert(property == 0x7f0c || property == 0x7f0d);
        assert(cfw_dark_mode_enabled() == (disk_record == 0x314b5244));
        unsigned bit = 1u << (property - 0x7f0c);
        assert(!(icon_invalidations & bit));
        icon_invalidations |= bit;
        icon_notifications++;
        return;
    }
    assert(value == &model && type == OSOS_NOTIFY_ALL);
    (void)property;
    notifications++;
}

static void fill(void *target, const void *rect, const unsigned char *color,
                 uint32_t mode, void *clip)
{
    assert(target == &surface && rect == &model && mode == 7 && clip == &controller);
    memcpy(drawn, color, 4);
}

static void text_color(void *target, const unsigned char *color)
{
    assert(target == &surface);
    memcpy(drawn, color, 4);
}

static int clear_rgb, clear_alpha;

static void clear_color(int red, int green, int blue, int alpha)
{
    assert(red == green && green == blue);
    clear_rgb = red;
    clear_alpha = alpha;
}

static uint32_t blend_background;

static uint32_t blend(uint32_t pixel, uint32_t background, int weight)
{
    assert(pixel == 0xabcd && weight == 0x8000);
    blend_background = background;
    return 0x1234;
}

#define osos_file_open file_open
#define osos_file_read file_read
#define osos_file_write file_write
#define osos_file_close file_close
#define osos_settings_model settings_model
#define osos_settings_on_off on_off
#define osos_model_notify notify
#define osos_now_playing_model now_playing_model
#define osos_ui_invalidate invalidate
#define osos_surface_fill_rect fill
#define osos_graphics_set_color text_color
#define osos_gles_clear_color clear_color
#define osos_rgb565_blend blend
#include "../../../payload/dark_mode.c"

static int stock_actions, stock_properties, adjustments;

static int settings_action(void *value, const char *action, uint32_t argument)
{
    assert(value == &controller && !strcmp(action, "stock") && argument == 0);
    stock_actions++;
    return 7;
}

static int settings_property(void *value, uint32_t type, uint32_t property,
                             uintptr_t *out)
{
    assert(value == &model);
    /* The native Clicker bitmap getter crashes for the two extra output modes. */
    assert(type != OSOS_RESOURCE_BMAP || property != 0x8925);
    stock_properties++;
    if (property >= 0x8904 && property <= 0x8907)
        return 1;
    *out = 5;
    return property == 123;
}

int cfw_eq_action(const char *action, uint32_t argument)
{
    (void)argument;
    return !strcmp(action, "CFW_EQ_Open");
}

static uint32_t clicker_value;
static uintptr_t preference_methods[0xe0 / 4];
static uintptr_t *preference_object = preference_methods;

static uint32_t get_preference(void *object, uint32_t property, uint32_t type)
{
    assert(object == &preference_object && property == 0x608c && type == 0x6000);
    return clicker_value;
}

static void set_preference(void *object, uint32_t property, uint32_t type,
                           uint32_t value)
{
    assert(object == &preference_object && property == 0x608c && type == 0x6000);
    clicker_value = value;
}

static uintptr_t resource(uint32_t id)
{
    return id;
}

#define osos_ui_string resource
#define osos_ui_bitmap resource
#include "../../../payload/clicker.c"

static int vi_enabled = 0;
int cfw_vietnamese_enabled(void) { return vi_enabled; }
const char *cfw_vietnamese_translate(const char *s) { return s; }

void cfw_album_artists_menu_adjust_property(uint32_t type, uint32_t property,
                                            uintptr_t *out)
{
    if (type == OSOS_PROPERTY_COUNT && property == 123) {
        assert(*out == 5);
        adjustments++;
        ++*out;
    }
}

#define osos_settings_action settings_action
#define osos_settings_property settings_property
#include "../../../payload/settings.c"

static void check_clicker(void)
{
    const uint32_t modes[] = {0x608e, 0x6090, 0x608f, 0x608d};
    preference_methods[0xdc / 4] = (uintptr_t)get_preference;
    preference_methods[0xd4 / 4] = (uintptr_t)set_preference;
    model.preferences = &preference_object;
    clicker_value = modes[0];
    int previous_notifications = notifications;
    int previous_properties = stock_properties;
    assert(
        cfw_settings_action(NULL, "ToggleSetting_Clicker", OSOS_ACTION_SUPPORT_QUERY));
    assert(clicker_value == modes[0] && notifications == previous_notifications);

    for (unsigned i = 0; i < 4; i++) {
        uintptr_t image;
        assert(cfw_settings_property(&model, OSOS_RESOURCE_BMAP, 0x8925, &image));
        assert(image == (i == 0 ? 0x0dad0c04u : 0x0dad0c03u));
        assert(stock_properties == previous_properties);
        assert(cfw_settings_action(&controller, "ToggleSetting_Clicker", 0));
        assert(clicker_value == modes[(i + 1) % 4]);
    }
}

int main(void)
{
    uintptr_t value = 0;
    cfw_dark_mode_load();
    assert(!cfw_dark_mode_enabled() && !closes);
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, CFW_DarkMode_Value,
                                 &value));
    assert(value == 1 && !stock_properties);
    assert(cfw_settings_property(&model, OSOS_PROPERTY_COUNT, 123, &value));
    assert(value == 6 && adjustments == 1);
    assert(!cfw_settings_property(&model, OSOS_PROPERTY_COUNT, 124, &value));
    assert(adjustments == 1);
    assert(cfw_settings_action(&controller, "stock", 0) == 7);
    assert(cfw_settings_action(&controller, "CFW_EQ_Open", 0));
    assert(cfw_settings_action(&controller, "CFW_DarkMode_Toggle",
                               OSOS_ACTION_SUPPORT_QUERY));
    assert(stock_actions == 1 && !writes && !redraws && !icon_notifications);

    /* Vietnamese toggle property handling */
    vi_enabled = 1;
    value = (uintptr_t)"B\xc3\xa0i h\xc3\xa1t";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8904, &value));
    assert(!strcmp((const char *)value, "B.h\xc3\xa1t"));

    value = (uintptr_t)"Songs";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8904, &value));
    assert(!strcmp((const char *)value, "B.h\xc3\xa1t"));

    value = (uintptr_t)"Off";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8904, &value));
    assert(!strcmp((const char *)value, "Off"));

    value = (uintptr_t)"One";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8906, &value));
    assert(!strcmp((const char *)value, "1"));

    value = (uintptr_t)"All";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8906, &value));
    assert(!strcmp((const char *)value, "B\xe1\xba\xadt"));

    value = (uintptr_t)"T\xe1\xba\xa5t c\xe1\xba\xa3";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8906, &value));
    assert(!strcmp((const char *)value, "B\xe1\xba\xadt"));

    value = (uintptr_t)"T\xe1\xba\xaft";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8906, &value));
    assert(!strcmp((const char *)value, "T\xe1\xba\xaft"));

    /* Side panel descriptions must not be clobbered */
    value = (uintptr_t)"Shuffle Entire Music Library";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8905, &value));
    assert(!strcmp((const char *)value, "Shuffle Entire Music Library"));

    value = (uintptr_t)"Repeat Current Song";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8907, &value));
    assert(!strcmp((const char *)value, "Repeat Current Song"));

    /* English mode bypass */
    vi_enabled = 0;
    value = (uintptr_t)"Songs";
    assert(cfw_settings_property(&model, OSOS_PROPERTY_STRING, 0x8904, &value));
    assert(!strcmp((const char *)value, "Songs"));

    struct color_case {
        unsigned char light[4], dark[4];
    };
    const struct color_case fills[] = {
        {{255, 255, 255, 173}, {0x18, 0x18, 0x18, 173}},
        {{0x7e, 0x81, 0x83, 127}, {0x38, 0x38, 0x38, 127}},
        {{35, 108, 196, 91}, {35, 108, 196, 91}},
        {{0x23, 0x6c, 0xc5, 91}, {0x00, 0xa1, 0xff, 91}},
        {{0xc5, 0x6c, 0x23, 91}, {0xc5, 0x6c, 0x23, 91}},
        {{0x31, 0x85, 0xd0, 77}, {0x31, 0x85, 0xd0, 77}},
    };
    const struct color_case texts[] = {
        {{0, 0, 0, 123}, {0xe8, 0xe8, 0xe8, 123}},
        {{255, 255, 255, 255}, {255, 255, 255, 255}},
        {{0x31, 0x85, 0xd0, 77}, {0x00, 0xa1, 0xff, 77}},
        {{0xd0, 0x85, 0x31, 77}, {0xd0, 0x85, 0x31, 77}},
        {{0x23, 0x6c, 0xc5, 91}, {0x00, 0xa1, 0xff, 91}},
    };
    for (int cycle = 0; cycle < 3; cycle++) {
        if (cycle)
            assert(cfw_settings_action(&controller, "CFW_DarkMode_Toggle", 0));
        int dark = cycle == 1;
        assert(cfw_dark_mode_enabled() == dark);
        for (unsigned i = 0; i < sizeof(fills) / sizeof(fills[0]); i++) {
            cfw_dark_mode_fill(&surface, &model, fills[i].light, 7, &controller);
            assert(
                !memcmp(drawn, dark ? fills[i].dark : fills[i].light, sizeof(drawn)));
        }
        for (unsigned i = 0; i < sizeof(texts) / sizeof(texts[0]); i++) {
            cfw_dark_mode_text_color(&surface, texts[i].light);
            assert(
                !memcmp(drawn, dark ? texts[i].dark : texts[i].light, sizeof(drawn)));
        }
        cfw_dark_mode_clear_color(0x10000, 0x10000, 0x10000, 0x4321);
        assert(clear_rgb == (dark ? 0x18 * 0x10000 / 255 : 0x10000));
        assert(clear_alpha == 0x4321);
        assert(cfw_dark_mode_reflection_blend(0xabcd, 0xffff, 0x8000) == 0x1234);
        assert(blend_background == (dark ? 0x18c3u : 0xffffu));
        cfw_dark_mode_load();
        assert(cfw_dark_mode_enabled() == dark);
    }
    assert(writes == 2 && redraws == 2 && notifications == 2);
    assert(icon_notifications == 4);
    fail_write = 1;
    assert(cfw_dark_mode_action(&controller, "CFW_DarkMode_Toggle", 0));
    assert(!cfw_dark_mode_enabled() && redraws == 2 && notifications == 2);
    assert(icon_notifications == 4 && !icon_invalidations);
    assert(closes == 5);
    disk_record = DARK_MODE_ON_RECORD;
    disk_size = 2;
    cfw_dark_mode_load();
    assert(!cfw_dark_mode_enabled());
    disk_size = 4;
    disk_record = 0;
    cfw_dark_mode_load();
    assert(!cfw_dark_mode_enabled());
    fail_write = 0;
    now_playing_available = 0;
    assert(cfw_dark_mode_action(&controller, "CFW_DarkMode_Toggle", 0));
    assert(cfw_dark_mode_enabled() && redraws == 3 && notifications == 3);
    assert(icon_notifications == 4);
    check_clicker();
}
