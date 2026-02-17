#include <sys/syscall.h>
#include <sys/file.h>

#include "syscalls.h"

int sys_chmod(const char* path, mode_t mode) {
    int ret = 0;

    register long x16 asm("x16") = SYS_chmod;

    asm volatile (
        "mov x0, %1\n\t"
        "mov x1, %2\n\t"
        "svc #0x80\n\t"
        "mov %0, x0"
        : "=r" (ret)
        : "r" (path), "r" (mode), "r" (x16)
        : "memory", "cc", "x0", "x1"
    );

    return ret;
}
