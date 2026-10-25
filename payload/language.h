/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CFW_LANGUAGE_H
#define CFW_LANGUAGE_H

#include <stdint.h>

const char *cfw_language_name(void *service, uint32_t index);
void cfw_language_select(void *model, uint32_t index);
int cfw_language_current_index(void *model);

#endif
