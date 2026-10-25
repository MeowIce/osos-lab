/* SPDX-License-Identifier: GPL-3.0-only */
#include "language.h"
#include "vietnamese.h"
#include "patch.h"
#include "osos.h"

const char *cfw_language_name_original(void *service, uint32_t index);
int cfw_language_current_index_original(void *model);
void osos_language_set(void *model, uint32_t index);

PATCH_ARM const char *cfw_language_name(void *service, uint32_t index)
{
    if (index == 21)
        return "Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t"; /* "Tiếng Việt" in UTF-8 */
    return cfw_language_name_original(service, index);
}

PATCH_ARM void cfw_language_select(void *model, uint32_t index)
{
    if (index == 21) {
        cfw_vietnamese_set(1);
        osos_language_set(model, 0); /* English base bundle 0x6067 */
    } else {
        cfw_vietnamese_set(0);
        osos_language_set(model, index);
    }
}

PATCH_ARM int cfw_language_current_index(void *model)
{
    if (cfw_vietnamese_enabled())
        return 21;
    return cfw_language_current_index_original(model);
}
