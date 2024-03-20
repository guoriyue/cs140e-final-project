#include "rpi-asm.h"

MK_FN(switch_to_sys_mode)
    mov r0, lr
    mov r1, sp
    cps #SYS_MODE
    prefetch_flush(r2)
    mov sp, r1
    mov lr, r0
    bx lr

MK_FN(sys_exit)
    push {lr}
    @ mov r1, r0
    mov r0, #0
    swi 1
    pop {lr}
    bx lr

MK_FN(sys_putc)
    push {lr}
    mov r1, r0
    mov r0, #1
    swi 1
    pop {lr}
    bx lr