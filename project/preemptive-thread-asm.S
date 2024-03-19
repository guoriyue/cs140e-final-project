#include "rpi-asm.h"

MK_FN(pre_init_trampoline)
    mov r0, r4
    mov r1, r5
    blx r1
    bl pre_exit


MK_FN(switch_to_sys_mode)
    mov r0, lr
    mov r1, sp
    cps #SYS_MODE
    prefetch_flush(r2)
    mov sp, r1
    mov lr, r0
    bx lr