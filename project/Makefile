COMMON_SRC += staff-full-except.c 
# new
COMMON_SRC += switchto-asm.S
# new
COMMON_SRC += full-except-asm.S

COMMON_SRC += preemptive-thread.c
COMMON_SRC += preemptive-thread-asm.S

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# set to 0 if you don't want it to run
RUN=1

START_ASM = staff-start.S

PROGS := preemptive-thread-test-1.c

GREP_STR := 'TRACE:\|THREAD:\|ERROR:\|PANIC:' 
include $(CS140E_2024_PATH)/libpi/mk/Makefile.template-fixed
