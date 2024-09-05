#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <cerrno>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <libproc.h>
#include <sys/sysctl.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>

#include "syscalls.h"
#include "utils.h"
#include "libc.h"
#include "native.h"
#include "resolve.h"

#define MH_MAGIC_64 0xfeedfacf
#define EXEC_BASE_ADDR 0x110000000
#define DYLD_SHARED_CACHE_FIRST 0x7ff700000000
#define DYLD_SHARED_CACHE_LAST 0x7FFFFFE00000

// https://blogs.blackberry.com/en/2017/02/running-executables-on-macos-from-memory
dyld_all_image_infos* FindDyld() {
    struct dyld_all_image_infos* dyld_all_image_infos = NULL;

    uintptr_t addr = DYLD_SHARED_CACHE_FIRST;
    while(addr < DYLD_SHARED_CACHE_LAST) {
        if (sys_chmod((const char*)addr, 0777) == ENOENT &&
            *(uint32_t*)addr == MH_MAGIC_64) {
            dyld_all_image_infos = (struct dyld_all_image_infos*)GetProcAddress((void*)addr, "dyld_all_image_infos");
            if (dyld_all_image_infos != NULL) {
                return dyld_all_image_infos;
            }
        }

        addr += 0x1000;
    }

    PRINT("Failed to find dyld!\n");
    return nullptr;
}

void* GetModuleBase(const char* lpModuleName) {
    struct dyld_all_image_infos* all_image_infos = FindDyld();
    if (all_image_infos == nullptr) {
        PRINT("[-] Failed to find dyld\n");
        return nullptr;
    }

    const struct dyld_image_info* image_infos = all_image_infos->infoArray;

    for (size_t i = 0; i < all_image_infos->infoArrayCount; i++) {
        const char* name = strrchr((char*)image_infos[i].imageFilePath, '/') + 1;
        if (strncmp(name, lpModuleName, PATH_MAX) == 0) {
            return (void*)image_infos[i].imageLoadAddress;
        }
    }

    return nullptr;
}

void* GetProcAddress(const void* hDlBase, const char* lpFunctionName) {
    struct segment_command_64* sc = nullptr;
    struct segment_command_64* linkedit = nullptr;
    struct segment_command_64* text = nullptr;
    struct load_command* lc = nullptr;
    struct symtab_command* symtab = nullptr;
    struct nlist_64* nl = nullptr;
    char* strtab = nullptr;

    lc = (struct load_command*)((uintptr_t)hDlBase + sizeof(struct mach_header_64));
    for (uint32_t i=0; i<((struct mach_header_64*)hDlBase)->ncmds; i++) {
        if (lc->cmd == LC_SYMTAB) {
            symtab = (struct symtab_command*)lc;
        }
        else if (lc->cmd == LC_SEGMENT_64) {
            sc = (struct segment_command_64*)lc;
            char* segname = ((struct segment_command_64*)lc)->segname;
            if (strncmp(segname, "__LINKEDIT", 11) == 0) {
                linkedit = sc;
            }
            else if (strncmp(segname, "__TEXT", 7) == 0) {
                text = sc;
            }
        }
        lc = (struct load_command*)((unsigned long)lc + lc->cmdsize);
    }

    if (linkedit == nullptr) {
        PRINT("[-] Failed to find __linkedit segment\n");
        return nullptr;
    }
    if (text == nullptr) {
        PRINT("[-] Failed to find __text segment\n");
        return nullptr;
    }
    if (symtab == nullptr) {
        PRINT("[-] Failed to find symbol table\n");
        return nullptr;
    }

    unsigned long file_slide = linkedit->vmaddr - text->vmaddr - linkedit->fileoff;
    strtab = (char*)((uintptr_t)hDlBase + file_slide + symtab->stroff);

    nl = (struct nlist_64*)((uintptr_t)hDlBase + file_slide + symtab->symoff);

    for (uint32_t i=0; i < symtab->nsyms; i++) {
        char* name = strtab + nl[i].n_un.n_strx;
        if (name[0] == '_')
            name++; // skip the underscore

        if (strncmp(name, lpFunctionName, PATH_MAX) == 0) {
            if (nl[i].n_value == 0) {
                PRINT("[-] Forwards are not supported\n");
                break;
            }
            return (void*)(nl[i].n_value - text->vmaddr + (uintptr_t)hDlBase);
        }
    }

    return nullptr;
}

void* GetProcAddress(const char* lpModuleName, const char* lpFunctionName) {
    void* hDlBase = GetModuleBase(lpModuleName);
    if (hDlBase == nullptr) {
        PRINT("[-] Failed to get module base\n");
        return nullptr;
    }

    return GetProcAddress(hDlBase, lpFunctionName);
}
