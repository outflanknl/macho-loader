#pragma once

#include <mach/mach.h>

typedef void* (*_mmap)(
        void* addr,
        size_t len,
        int prot,
        int flags,
        int fd,
        off_t offset
);

typedef int (*_mprotect)(
        void* addr,
        size_t len,
        int prot
);

typedef void* (*_calloc)(
        size_t num,
        size_t size
);

typedef void* (*_realloc)(
        void* ptr,
        size_t size
);

typedef void* (*_dlsym)(
        void* handle,
        const char* symbol
);

typedef void* (*_dlopen)(
        const char* filename,
        int flags
);
