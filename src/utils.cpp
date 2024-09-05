#include <string.h>

#pragma clang optimize off
void* memcpy(void* dst0, const void* src0, size_t length) {
    size_t i;
    char* d = (char*)dst0;
    char* s = (char*)src0;
    for (i = 0; i < length; i++) {
        d[i] = s[i];
    }
    return dst0;
}
#pragma clang optimize on

char* strrchr(char *cp, char ch) {
    char *save;
    char c;

    for (save = (char *) 0; (c = *cp); cp++) {
        if (c == ch)
            save = (char *) cp;
    }

    return save;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(unsigned char *)s1 - *(unsigned char *)--s2);
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}
