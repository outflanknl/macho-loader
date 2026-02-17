#pragma once

#include <stdio.h>

#define PIC_STRING(NAME, STRING)                                                  \
    const char* NAME;                                                             \
    __asm__ __volatile__(                                                         \
        "b 1f\n\t"                                                                \
        "0:\n\t"                                                                  \
        ".asciz \"" STRING "\"\n\t"                                               \
        ".p2align 2\n\t"                                                          \
        "1:\n\t"                                                                  \
        "adr %0, 0b\n\t"                                                          \
        : "=r"(NAME))

#ifdef _DEBUG
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

char* fnstrrchr(char *cp, char ch);
int fnstrncmp(const char *s1, const char *s2, size_t n);
void* fnmemcpy(void *dst0, const void *src0, size_t length);
