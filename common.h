#pragma once

#include <stddef.h>

static inline int memcmp(const void* str1, const void* str2, size_t count) {
    const unsigned char *s1 = (const unsigned char*)str1;
    const unsigned char *s2 = (const unsigned char*)str2;

    while (count-- > 0) {
        if (*s1++ != *s2++) {
            return s1[-1] < s2[-1] ? -1 : 1;
        }
    }
    return 0;
}

static inline void memset(void* p, int ch, size_t sz) {
    uint8_t* ptr = (uint8_t*)p;
    while (sz > 0) {
        *ptr = ch;
        ptr++;
        sz--;
    }
}

static inline size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len]) {
		len++;
    }
	return len;
}

static inline void memcpy(void *dest, void *src, size_t n)  {
    char *raw_src = (char *)src;
    char *raw_dest = (char *)dest;

    for (size_t i = 0; i < n; ++i)
        raw_dest[i] = raw_src[i];
}


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
