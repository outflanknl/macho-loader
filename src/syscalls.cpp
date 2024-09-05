#include <sys/syscall.h>
#include <sys/file.h>

#include "syscalls.h"

// https://opensource.apple.com/source/xnu/xnu-792.13.8/osfmk/mach/i386/syscall_sw.h
#define SYSCALL_CLASS_SHIFT	24
#define SYSCALL_CLASS_MASK	(0xFF << SYSCALL_CLASS_SHIFT)
#define SYSCALL_NUMBER_MASK	(~SYSCALL_CLASS_MASK)
#define SYSCALL_CLASS_UNIX	2	/* Unix/BSD */
#define SYSCALL_CONSTRUCT_UNIX(syscall_number) \
                        ((SYSCALL_CLASS_UNIX << SYSCALL_CLASS_SHIFT) | \
                        (SYSCALL_NUMBER_MASK & (syscall_number)))

int sys_chmod(const char* path, mode_t mode) {
    int ret = 0;

    register long rax asm("rax") = SYSCALL_CONSTRUCT_UNIX(SYS_chmod);
    register long rdi asm("rdi") = (long)path;
    register long rsi asm("rsi") = (long)mode;

    __asm__ volatile (
        "syscall"
        : "=r" (ret)
        : "0" (rax), "D" (rdi), "S" (rsi)
        : "memory", "cc"
    );

    return ret;
}
