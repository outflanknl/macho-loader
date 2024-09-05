#include <stdio.h>

__attribute__((visibility("default"))) int Entry() {
    printf("Hello, World!\n");
    return 0;
}
