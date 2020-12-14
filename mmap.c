#include "mmap.h"
#include "panic.h"

mmap_entry_t* next_mmap_entry(mmap_entry_t* current) {
    return (mmap_entry_t*) ((unsigned int) current + current->size + sizeof(current->size));
}

void print(mmap_entry_t* entry, int number) {
    printf("Entry %d:\n", DEFAULT_COLOR, number);
    printf("high addr = %x; low addr = %x; high len = %d; low len = %d; ", DEFAULT_COLOR,
           entry->base_addr_high, entry->base_addr_low,
           entry->length_high, entry->length_low);
    char* type;
    uint8_t color = FAILURE;
    switch (entry->type) {
        case 1:
            type = "AVAILABLE";
            color = SUCCESS;
            break;
        case 3:
            type = "USED FOR ACPI";
            break;
        case 4:
            type = "RESERVED FOR HIBERNATION";
            break;
        case 5:
            type = "DEFECTIVE RAM MODULE";
            break;
        default:
            type = "RESERVED";

    }
    printf("%s\n\n", color, type);
}

void show_memory(multiboot_info_t* mbd) {

    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) {
        panic("Multiboot couldn't find the memory mapping.");
    }
    mmap_entry_t* entry = mbd->mmap_addr;
    for (int i = 0;
         entry < mbd->mmap_addr + mbd->mmap_length;
         ++i, entry = next_mmap_entry(entry)) {
        print(entry, i);
    }
}



