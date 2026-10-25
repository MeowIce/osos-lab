/* SPDX-License-Identifier: GPL-3.0-only */
#include "patches/sites.h"
.syntax unified
.cpu arm926ej-s
.arm
.text
.balign 4
.global cfw_language_name_original
.type cfw_language_name_original,%function
cfw_language_name_original:
    push {r4, lr}
    mov r4, r1
    ldr pc, =OSOS_LANGUAGE_NAME_ENTRY + 8
.balign 4
.global cfw_language_current_index_original
.type cfw_language_current_index_original,%function
cfw_language_current_index_original:
    push {r4, lr}
    ldr r0, [r0, #0x378]
    ldr pc, =OSOS_LANGUAGE_CURRENT_INDEX_ENTRY + 8
.balign 4
.global osos_language_set
.type osos_language_set,%function
osos_language_set:
    ldr pc, =OSOS_LANGUAGE_SET
.ltorg
