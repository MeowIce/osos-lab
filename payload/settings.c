/* SPDX-License-Identifier: GPL-3.0-only */
#include "album_artists_menu.h"
#include "custom_eq.h"
#include "dark_mode.h"
#include "vietnamese.h"
#include "osos.h"
#include "patch.h"

PATCH_ARM int cfw_settings_action(void *controller, const char *action,
                                  uint32_t argument)
{
    if (cfw_dark_mode_action(controller, action, argument) ||
        cfw_eq_action(action, argument))
        return 1;
    return osos_settings_action(controller, action, argument);
}

PATCH_ARM int cfw_settings_property(void *model, uint32_t type,
                                    uint32_t property, uintptr_t *out)
{
    if (cfw_dark_mode_property(type, property, out))
        return 1;
    int result = osos_settings_property(model, type, property, out);
    if (result)
        cfw_album_artists_menu_adjust_property(type, property, out);
    if (result && type == OSOS_PROPERTY_STRING && out && *out && cfw_vietnamese_enabled()) {
        const char *s = (const char *)*out;
        if (property == 0x8905) {
            if ((s[0] == 'B' && (unsigned char)s[1] == 0xc3) ||
                (s[0] == 'S' && s[1] == 'o'))
                *out = (uintptr_t)"B.h\xc3\xa1t";
        }
        if (property == 0x8907) {
            if (s[0] == '1' || (s[0] == 'O' && s[1] == 'n') ||
                (s[0] == 'M' && (unsigned char)s[1] == 0xe1))
                *out = (uintptr_t)"1";
            else if ((s[0] == 'A' && s[1] == 'l') ||
                     (s[0] == 'T' && (unsigned char)s[1] == 0xe1 &&
                      (unsigned char)s[2] == 0xba && (unsigned char)s[3] == 0xa5))
                *out = (uintptr_t)"B\xe1\xba\xadt";
        }
    }
    return result;
}
