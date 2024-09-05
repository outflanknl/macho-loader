#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <cerrno>
#include <sys/mman.h>
#include <sys/syslimits.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/fixup-chains.h>
#include <mach-o/nlist.h>

#include "utils.h"
#include "native.h"
#include "resolve.h"
#include "libc.h"
#include "end.h"

#undef mach_task_self

typedef struct _SYMBOL {
    char* name;
    void* addr;
} SYMBOL, *PSYMBOL;

#pragma clang optimize off
uintptr_t FindImageBase() {
#if _DEBUG
    FILE* f = fopen("libexample.dylib", "rb");
    if (f == NULL) {
        printf("Failed to open file\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
    if (buffer == MAP_FAILED) {
        printf("Failed to map file\n");
        return 0;
    }

    fclose(f);
    return (uintptr_t)buffer;

#else
    return (uintptr_t)&TrimLdr + 6;

#endif
}
#pragma clang optimize on

extern "C" {
uintptr_t ReflectiveLoader() {
    uintptr_t beaconBaseAddress = FindImageBase();
    mach_header_64* beaconMachHeader = (mach_header_64*)beaconBaseAddress;

    if (beaconMachHeader->magic != MH_MAGIC_64) {
        PRINT("File is not a Mach-O!\n");
        return -1;
    }

    if (beaconMachHeader->filetype != MH_BUNDLE && beaconMachHeader->filetype != MH_DYLIB) {
        PRINT("Mach-O is not a bundle!\n");
        return -1;
    }

    PIC_STRING(str_libsystem_kernel, "libsystem_kernel.dylib");
    PIC_STRING(str_libdyld, "libdyld.dylib");
    PIC_STRING(str_libsystem_malloc, "libsystem_malloc.dylib");
    PIC_STRING(str_vm_protect, "vm_protect");
    PIC_STRING(str_vm_allocate, "vm_allocate");
    PIC_STRING(str_dlsym, "dlsym");
    PIC_STRING(str_dlopen, "dlopen");
    PIC_STRING(str_calloc, "calloc");
    PIC_STRING(str_realloc, "realloc");
    PIC_STRING(str_mach_task_self, "mach_task_self");

    _vm_protect vm_protect = (_vm_protect)GetProcAddress(str_libsystem_kernel, str_vm_protect);
    _vm_allocate vm_allocate = (_vm_allocate)GetProcAddress(str_libsystem_kernel, str_vm_allocate);
    _dlsym dlsym = (_dlsym)GetProcAddress(str_libdyld, str_dlsym);
    _dlopen dlopen = (_dlopen)GetProcAddress(str_libdyld, str_dlopen);
    _calloc calloc = (_calloc)GetProcAddress( str_libsystem_malloc, str_calloc);
    _realloc realloc = (_realloc)GetProcAddress(str_libsystem_malloc, str_realloc);
    _mach_task_self mach_task_self = (_mach_task_self)GetProcAddress(str_libsystem_kernel, str_mach_task_self);
    if (vm_allocate == NULL ||
        vm_protect == NULL ||
        mach_task_self == NULL ||
        dlsym == NULL ||
        dlopen == NULL ||
        calloc == NULL ||
        realloc == NULL) {
        PRINT("Failed to resolve functions!");
        return -1;
    }

    load_command* loadCmd;
    segment_command_64* segCmd;
    uint64_t maxAddr = 0;
    uint64_t maxLength = 0;

    loadCmd = (load_command*)(beaconBaseAddress + sizeof(mach_header_64));
    for(uint32_t i=0; i < beaconMachHeader->ncmds; i++) {
        if (loadCmd->cmd == LC_SEGMENT_64) {
            segCmd = (segment_command_64*)loadCmd;
            if (segCmd->vmaddr >= maxAddr + maxLength) {
                maxAddr = segCmd->vmaddr;
                maxLength = segCmd->vmsize;
            }

            loadCmd = (load_command*)((char*)loadCmd + loadCmd->cmdsize);
        }
    }

    uintptr_t newBaseAddr = 0;
    vm_allocate(mach_task_self(), &newBaseAddr, maxAddr+maxLength, VM_FLAGS_ANYWHERE);
    if (newBaseAddr == 0) {
        PRINT("[!] Failed to allocate memory for rDylib!\n");
        return -1;
    }

    SYMBOL* symbols = NULL;
    int symbolCount = 0;
    void* indirectSymbols;
    char** dylibs = NULL;
    int dylibCount = 0;
    load_command* chained_fixups = NULL;

    loadCmd = (load_command*)(beaconBaseAddress + sizeof(mach_header_64));
    for(uint32_t i=0; i < beaconMachHeader->ncmds; i++) {
        if(loadCmd->cmd == LC_SEGMENT_64) {
            segment_command_64* segment = (segment_command_64*)loadCmd;
            kern_return_t ret;
            void* loadAddr = 0;
            section_64 *section64;

            loadAddr = (void*)(newBaseAddr + (uintptr_t)segment->vmaddr);
            memcpy(loadAddr, (void*)(beaconBaseAddress + (uintptr_t)segment->fileoff), segment->filesize);

            section64 = (section_64*)((char*)segment + sizeof(segment_command_64));
            for (uint32_t j = 0; j < segment->nsects; j++) {
                section_64 section = section64[j];
                void *sectionLoadAddr = (void*)(newBaseAddr + (uintptr_t)section.addr);

                ret = vm_protect(mach_task_self(), (vm_address_t)sectionLoadAddr, section.size, false, PROT_READ | PROT_WRITE);
                if (ret == -1) {
                    PRINT("\t\t[!] Error during vm_protect: %s\n", strerror(errno));
                    return -1;
                }

                if (!(section.flags & S_ZEROFILL)) {
                    memcpy(sectionLoadAddr,  (void*)(beaconBaseAddress + section.offset), section.size);
                }

                ret = vm_protect(mach_task_self(), (vm_address_t)sectionLoadAddr, section.size, false, segment->initprot);
                if (ret == -1) {
                    PRINT("\t\t[!] Error during vm_protect: %s\n", strerror(errno));
                    return -1;
                }
            }
        }
        else if(loadCmd->cmd == LC_SYMTAB) {
            symtab_command* symtab = (symtab_command*)loadCmd;
            char* stringTable;
            nlist_64* nl;

            symbolCount = symtab->nsyms;
            symbols = (PSYMBOL)calloc(sizeof(SYMBOL), symbolCount);

            stringTable = (char*)beaconBaseAddress + symtab->stroff;

            nl = (nlist_64*)((char*)beaconBaseAddress + symtab->symoff);
            for (int j = 0; j < symbolCount; j++) {
                nlist_64 symbol = nl[j];
                char* name = stringTable + symbol.n_un.n_strx;
                if (symbol.n_value != 0) {
                    symbols[j].name = name;
                    symbols[j].addr = (void*)symbol.n_value;
                }
            }
        }
        else if(loadCmd->cmd == LC_DYSYMTAB) {
            dysymtab_command* dysymtab = (dysymtab_command*)loadCmd;
            indirectSymbols = (void *)calloc(sizeof(int), dysymtab->nindirectsyms);
            memcpy(indirectSymbols, (void *) (beaconBaseAddress + dysymtab->indirectsymoff),
                   sizeof(int) * dysymtab->nindirectsyms);
        }
        else if(loadCmd->cmd == LC_LOAD_DYLIB) {
            dylib_command *dylib = (dylib_command*)loadCmd;
            char* dyldName = (char*)dylib + dylib->dylib.name.offset;
            dylibCount++;
            dylibs = (char**)realloc(dylibs, sizeof(char*) * dylibCount);
            dylibs[dylibCount-1] = dyldName;
        }
        else if(loadCmd->cmd == LC_DYLD_CHAINED_FIXUPS) {
            // Must be processed last
            chained_fixups = loadCmd;
        }

        loadCmd = (load_command*)((char*)loadCmd + loadCmd->cmdsize);
    }

    if(chained_fixups == NULL) {
        PRINT("\t[!] Failed to populate chained fixups!\n");
        return -1;
    }

    linkedit_data_command* linkedit = (linkedit_data_command*)chained_fixups;
    void* lib = NULL;
    void* func = NULL;
    void** imports = NULL;
    int importCount = 0;

    dyld_chained_fixups_header* fixupsHeader = (dyld_chained_fixups_header*)((char*)beaconBaseAddress + linkedit->dataoff);
    dyld_chained_import* chainedImports = (dyld_chained_import*)((char*)fixupsHeader + fixupsHeader->imports_offset);
    dyld_chained_starts_in_image* chainedStarts = (dyld_chained_starts_in_image*)((char*)fixupsHeader + fixupsHeader->starts_offset);
    char* symbolNames = (char*)fixupsHeader + fixupsHeader->symbols_offset;

    for (uint32_t i = 0; i < fixupsHeader->imports_count; i++) {
        int ordinal = chainedImports[i].lib_ordinal;
        char* name = symbolNames + chainedImports[i].name_offset;
        if (ordinal == 253 || ordinal == 0) {
            func = NULL;
            for (int j = 0; j < symbolCount; j++) {
                if (symbols[j].name == NULL) {
                    continue;
                }

                if (strncmp(symbols[j].name, name, PATH_MAX) == 0) {
                    func = symbols[j].addr;
                    break;
                }
            }

            if (func == NULL) {
                if (name[0] == '_') {
                    name += 1;
                }

                func = dlsym(RTLD_DEFAULT, name);
            }
            else {
                func = (void*)(newBaseAddr + (unsigned long long)func);
            }
        } else {
            const char* dyldName = dylibs[ordinal - 1];
            lib = dlopen(dyldName, RTLD_NOW);
            if (lib == NULL) {
                PRINT("[!] Could not load dylib: %s\n", dyldName);
            }

            if (name[0] == '_') {
                name += 1;
            }
            func = dlsym(lib, name);
        }

        if (func == NULL) {
            PRINT("\t[!] Cannot load dynamic library function!\n");
        }

        importCount++;
        imports = (void**)realloc(imports, sizeof(char*) * importCount);
        imports[importCount-1] = func;
    }

    for (uint32_t i = 0; i < chainedStarts->seg_count; i++) {
        if (chainedStarts->seg_info_offset[i] == 0) {
            continue;
        }

        dyld_chained_starts_in_segment* chainedStartsSegment = (dyld_chained_starts_in_segment*)((char*)chainedStarts + chainedStarts->seg_info_offset[i]);
        for (int j = 0; j < chainedStartsSegment->page_count; j++) {
            if (chainedStartsSegment->page_start[j] == DYLD_CHAINED_PTR_START_NONE) {
                continue;
            }

            dyld_chained_ptr_64_rebase curRebase;
            dyld_chained_ptr_64_bind curBind;
            dyld_chained_ptr_64_bind* bind = (dyld_chained_ptr_64_bind*)(newBaseAddr + (chainedStartsSegment->segment_offset + (j * chainedStartsSegment->page_size) + chainedStartsSegment->page_start[j]));
            dyld_chained_ptr_64_bind* prevBind = NULL;

            while (bind != prevBind) {
                prevBind = bind;
                if (bind->bind == 1) {
                    memcpy(&curBind, bind, sizeof(dyld_chained_ptr_64_bind));
                    *(void**)bind = imports[curBind.ordinal];
                    bind = (dyld_chained_ptr_64_bind*)((char*)bind + (4 * curBind.next));
                } else {
                    memcpy(&curRebase, bind, sizeof(dyld_chained_ptr_64_rebase));
                    *(void**) bind = (void*)(newBaseAddr + curRebase.target);
                    bind = (dyld_chained_ptr_64_bind*)((char*)bind + (4 * curRebase.next));
                }
            }
        }
    }

    void* exportAddr = NULL;
    PIC_STRING(str_Entry, "_Entry");
    for (int i = 0; i < symbolCount; i++) {
        if (symbols[i].name != NULL && strncmp(symbols[i].name, str_Entry, 7) == 0) {
            exportAddr = symbols[i].addr;
            break;
        }
    }

    if (exportAddr == NULL) {
        PRINT("[!] Could not find entry symbol\n");
        return -1;
    }

    void (*entry)() = (void(*)())exportAddr;
    entry = (void(*)())((unsigned long)entry + (unsigned long)newBaseAddr);

    entry();

    return 0;
}
}

#ifdef _DEBUG
int main() {
    ReflectiveLoader();
    return 0;
}
#else

#endif
