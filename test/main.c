#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <mach/mach.h>

int main() {
    FILE* f = fopen("example.bin", "rb");
    if (f == NULL) {
        printf("Failed to open shellcode file\n");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* buffer = NULL;
    kern_return_t ret = vm_allocate(mach_task_self(), (vm_address_t*)&buffer, size, VM_FLAGS_ANYWHERE);
    if (ret != KERN_SUCCESS) {
        printf("Failed to read shellcode file\n");
        return -1;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    ret = vm_protect(mach_task_self(), (vm_address_t)buffer, size, 0, VM_PROT_READ | VM_PROT_EXECUTE);
    if (ret != KERN_SUCCESS) {
        printf("[!] Failed to set memory protection: %s\n", mach_error_string(ret));
        return -1;
    }

    void (*sc)() = buffer;
    sc();

    return 0;
}
