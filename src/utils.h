#pragma once

#include <stdio.h>

// https://gist.github.com/EvanMcBroom/d7f6a8fe3b4d8f511b132518b9cf80d7
#define PIC_STRING(NAME, STRING) constexpr char NAME[]{ STRING }

#ifdef _DEBUG
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

char* strrchr(char *cp, char ch);
int strncmp(const char *s1, const char *s2, size_t n);
void* memcpy(void *dst0, const void *src0, size_t length);
