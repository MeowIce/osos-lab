/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CFW_VIETNAMESE_H
#define CFW_VIETNAMESE_H

#include <stdint.h>

int cfw_vietnamese_enabled(void);
void cfw_vietnamese_set(int enable);
void cfw_vietnamese_load(void);
void cfw_install_vietnamese(void *bank);
const char *cfw_vietnamese_translate(const char *s);

#endif
