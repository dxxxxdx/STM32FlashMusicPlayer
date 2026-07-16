//
// Created by dxxdx on 2026/7/9.
//

#ifndef G030DEV_FASTCPY_H
#define G030DEV_FASTCPY_H
#include <stdint.h>
//别作死往里面传0，除非你想整个单片机fastcpy，还有你懂的对齐
__attribute__((section(".RamFunc"), noinline))
void fastcpy(void *restrict dst, const void *restrict src, uint32_t lenInWords)
{
    uint32_t *restrict d = (uint32_t *)dst;
    const uint32_t *restrict s = (const uint32_t *)src;

    do
    {
        *d++ = *s++;
    } while (--lenInWords);
}


#endif //G030DEV_FASTCPY_H
