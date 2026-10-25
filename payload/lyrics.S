/* SPDX-License-Identifier: GPL-3.0-only */

#include "native/bindings.h"

.syntax unified
.cpu arm926ej-s
.arm
.text
.balign 4

.global cfw_text_view_draw_original
.type cfw_text_view_draw_original,%function
cfw_text_view_draw_original:
    push {r4, r5, r6, r7, lr}
    mov r4, r0
    ldr pc, =OSOS_TEXT_VIEW_DRAW_RESUME
.ltorg
