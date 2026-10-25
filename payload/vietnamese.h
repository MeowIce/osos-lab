/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CFW_VIETNAMESE_H
#define CFW_VIETNAMESE_H

#include <stdint.h>

int cfw_vietnamese_enabled(void);
void cfw_vietnamese_set(int enable);
void cfw_vietnamese_load(void);
void cfw_install_vietnamese(void *bank);
int cfw_vietnamese_property(uint32_t type, uint32_t property, uintptr_t *out);
int cfw_vietnamese_action(void *controller, const char *action, uint32_t argument);
const char *cfw_vietnamese_translate(const char *s);

#endif
