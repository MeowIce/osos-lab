/* SPDX-License-Identifier: GPL-3.0-only */
/* Clicker output choice (Off, Speaker, Headphones, Both).
 *
 * Apple's 2.0.5 player settings already implement the iPod nano Clicker modes:
 * preference 0x608c takes 0x608d Both, 0x608e Off, 0x608f Speaker or 0x6090
 * Headphones and routes the click to the speaker and/or headphones. The Classic
 * Settings row only sends "toggle" (0x60bf), which flips Both <-> Off. This feature
 * makes the row step through all four choices and labels the row with the choice.
 * The preview pane's descriptions ("Play Sound Through Speaker", ...) are Apple's own, and so is
 * its picture: Apple's getter (0x081e1850) knows only Off and its own "On" value and builds the
 * picture from resource 0 for anything else (an allocator abort), so we answer it here. */
#include "clicker.h"
#include "osos.h"
#include "cfw.h"
#include "patch.h"

enum {
    CLICKER_PREF = 0x608c,
    CLICKER_BOTH = 0x608d,
    CLICKER_OFF = 0x608e,
    /* Apple's own names say 0x608f = Speaker, 0x6090 = Headphones, but on the device 0x608f
       clicks only in the headphones and 0x6090 only through the speaker, so we label by what
       the user hears (tested on an iPod Classic 7G, 2.0.5). */
    CLICKER_SPEAKER = 0x6090,
    CLICKER_HEADPHONES = 0x608f,
    PREF_TYPE = 0x6000,
    CLICKER_ROW_VALUE = 0x8912,      /* blue value text on the Settings row */
    CLICKER_PREVIEW_TEXT = 0x8914,   /* description under the preview picture */
    CLICKER_PREVIEW_IMAGE = 0x8925,  /* the preview picture (a 'BMap' property) */
    PROPERTY_BITMAP = 0x424d6170,    /* BMap */
    IMG_ON = 0x0dad0c03,             /* Settings_ClickerOn_Image */
    IMG_OFF = 0x0dad0c04,            /* Settings_ClickerOff_Image */
    STR_SPEAKER = 0x0dad0bd3,        /* Settings_Speaker_String */
    STR_HEADPHONES = 0x0dad0bd4,     /* Settings_Headphones_String */
    STR_BOTH = 0x0dad0bd5,           /* Settings_Both_String */
    STR_OFF = 0x0dad0bda,            /* Settings_Off_String */
    DESC_OFF = 0x0dad0bd6,           /* "Turn Off Sound While Scrolling" */
    DESC_SPEAKER = 0x0dad0bd7,       /* "Play Sound Through Speaker" */
    DESC_HEADPHONES = 0x0dad0bd8,    /* "Play Sound Through Headphones" */
    DESC_BOTH = 0x0dad0bd9,          /* "Play Sound Through Headphones and Speaker" */
};

/* Refresh the row value (0x8912), the preview text (0x8914) and the preview picture (0x8925,
   answered below so Apple's two-state getter never sees Speaker or Headphones). */

static void *prefs(void *model)
{
    return *(void **)((unsigned char *)model + 0x378);
}

static uint32_t clicker_get(void *model)
{
    typedef uint32_t (*fn)(void *, uint32_t, uint32_t);
    void *p = prefs(model);
    return ((fn)(*(uintptr_t **)p)[0xdc / 4])(p, CLICKER_PREF, PREF_TYPE);
}

static void clicker_set(void *model, uint32_t value)
{
    typedef void (*fn)(void *, uint32_t, uint32_t, uint32_t);
    void *p = prefs(model);
    ((fn)(*(uintptr_t **)p)[0xd4 / 4])(p, CLICKER_PREF, PREF_TYPE, value);
}

PATCH_ARM int cfw_clicker_action(void *controller, const char *action, uint32_t argument)
{
    if (!cfw_string_equal(action, "ToggleSetting_Clicker"))
        return 0;
    if (argument == OSOS_ACTION_SUPPORT_QUERY)
        return 1;
    void *model = osos_settings_model(controller);
    uint32_t next;
    switch (clicker_get(model)) {
    case CLICKER_OFF: next = CLICKER_SPEAKER; break;
    case CLICKER_SPEAKER: next = CLICKER_HEADPHONES; break;
    case CLICKER_HEADPHONES: next = CLICKER_BOTH; break;
    default: next = CLICKER_OFF; break;          /* Both (Classic "On") */
    }
    clicker_set(model, next);
    osos_model_notify(model, OSOS_NOTIFY_ALL, CLICKER_ROW_VALUE);
    osos_model_notify(model, OSOS_NOTIFY_ALL, CLICKER_PREVIEW_TEXT);
    osos_model_notify(model, OSOS_NOTIFY_ALL, CLICKER_PREVIEW_IMAGE);
    return 1;
}

PATCH_ARM int cfw_clicker_property(void *model, uint32_t type, uint32_t property, uintptr_t *out)
{
    if (property == CLICKER_PREVIEW_IMAGE && type == PROPERTY_BITMAP) {
        *out = osos_ui_bitmap(clicker_get(model) == CLICKER_OFF ? IMG_OFF : IMG_ON);
        return 1;
    }
    if (type != OSOS_PROPERTY_STRING ||
        (property != CLICKER_ROW_VALUE && property != CLICKER_PREVIEW_TEXT))
        return 0;
    int row = property == CLICKER_ROW_VALUE;
    uint32_t id;
    switch (clicker_get(model)) {
    case CLICKER_OFF: id = row ? STR_OFF : DESC_OFF; break;
    case CLICKER_SPEAKER: id = row ? STR_SPEAKER : DESC_SPEAKER; break;
    case CLICKER_HEADPHONES: id = row ? STR_HEADPHONES : DESC_HEADPHONES; break;
    default: id = row ? STR_BOTH : DESC_BOTH; break;
    }
    *out = osos_ui_string(id);
    return 1;
}
