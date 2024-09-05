#pragma once

#include <mach/mach.h>

typedef kern_return_t (*_vm_allocate)(
    vm_map_t target_task,
    vm_address_t *address,
    vm_size_t size,
    int flags
);

typedef kern_return_t (*_vm_protect)(
    vm_map_t target_task,
    vm_address_t address,
    vm_size_t size,
    boolean_t set_maximum,
    vm_prot_t new_protection
);

typedef mach_port_t (*_mach_task_self)();

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
