#include "rpi-asm.h"
MK_FN(pre_trampoline)
    mov r0, r4
    mov r1, r5
    blx r1
    bl pre_exit
