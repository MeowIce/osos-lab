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
        cfw_vietnamese_action(controller, action, argument) ||
        cfw_eq_action(action, argument))
        return 1;
    return osos_settings_action(controller, action, argument);
}

PATCH_ARM int cfw_settings_property(void *model, uint32_t type,
                                    uint32_t property, uintptr_t *out)
{
    if (cfw_dark_mode_property(type, property, out) ||
        cfw_vietnamese_property(type, property, out))
        return 1;
    int result = osos_settings_property(model, type, property, out);
    if (result) {
        cfw_album_artists_menu_adjust_property(type, property, out);
        if (type == OSOS_PROPERTY_STRING && out && *out && cfw_vietnamese_enabled()) {
            const char *vi = cfw_vietnamese_translate((const char *)*out);
            if (vi)
                *out = (uintptr_t)vi;
        }
    }
    return result;
}
