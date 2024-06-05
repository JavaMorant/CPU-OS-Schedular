#include "common.h"

unsigned long strlen(const char *str)
{
    int l = 0;
    for (;*str;str++) {
        l++;
    }
    return l;
}

void memset(void *ptr, u8 val, u64 size)
{
    u8 *data = ptr;
    for (u64 i = 0; i < size; i++) {
        data[i] = val;
    }
}

void memcpy(void *dest, void *src, u64 size)
{
    u8 *p1 = dest;
    u8 *p2 = src;
    
    for (u64 i = 0; i < size; i++) {
        p1[i] = p2[i];
    }
}

int strcmp(const char *str1, const char *str2)
{
    while (1) {
        int diff = *str1 - *str2;
        if (diff != 0 || *str1 == 0 || *str2 == 0)
            return diff;
        str1++;
        str2++;
    }
}
