#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#include "grain_asm.h"

#define BANK_SIZE (64)
#define BLEND_SIZE (8)

#define _STRING(x) #x
#define STRING(x) _STRING(x)

void cover_BlockBlendSse2(uint8_t *dst, size_t dst_pitch,
                           const uint8_t *src, size_t src_pitch,
                           const int16_t *noise)
{
#if BLEND_SIZE == 8
    /* TODO It is possible to do the math on 8 bits using
     * paddusb X  and then psubusb -X.
     */
    asm volatile ("pxor %%xmm0, %%xmm0\n" : :);
    for (int i = 0; i < 8/2; i++) {
        asm volatile (
            "movq       (%[src1]),   %%xmm1\n"
            "movq       (%[src2]),   %%xmm3\n"
            "movdqu     (%[noise]), %%xmm2\n"
            "movdqu 2*"STRING(BANK_SIZE)"(%[noise]), %%xmm4\n"

            "punpcklbw  %%xmm0,     %%xmm1\n"
            "punpcklbw  %%xmm0,     %%xmm3\n"

            "paddsw    %%xmm2,     %%xmm1\n"
            "paddsw    %%xmm4,     %%xmm3\n"
            "packuswb   %%xmm1,     %%xmm1\n"
            "packuswb   %%xmm3,     %%xmm3\n"
            "movq       %%xmm1,     (%[dst1])\n"
            "movq       %%xmm3,     (%[dst2])\n"
            : : [dst1]"r"(&dst[(2*i+0) * dst_pitch]),
                [dst2]"r"(&dst[(2*i+1) * dst_pitch]),
                [src1]"r"(&src[(2*i+0) * src_pitch]),
                [src2]"r"(&src[(2*i+1) * src_pitch]),
                [noise]"r"(&noise[2*i * BANK_SIZE])
            : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "memory");
    }
#else
#   error "BLEND_SIZE unsupported"
#endif
}
