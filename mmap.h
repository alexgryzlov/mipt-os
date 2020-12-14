#ifndef MIPT_OS_MMAP_H
#define MIPT_OS_MMAP_H

#include "multiboot.h"

typedef struct multiboot_memory_map {
    unsigned int size;
    unsigned int base_addr_low, base_addr_high;
    unsigned int length_low,length_high;
    unsigned int type;
} mmap_entry_t;

void show_memory(multiboot_info_t* mbd);


#endif //MIPT_OS_MMAP_H
