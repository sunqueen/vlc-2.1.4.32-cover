#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#include "sepia_asm.h"

void cover_Sepia8ySSE2(uint8_t * dst, const uint8_t * src,
                         int i_intensity_spread)
{
    __asm__ volatile (
        // y = y - y / 4 + i_intensity / 4
        "movq            (%1), %%xmm1\n"
        "punpcklbw     %%xmm7, %%xmm1\n"
        "movq            (%1), %%xmm2\n" // store bytes as words with 0s in between
        "punpcklbw     %%xmm7, %%xmm2\n"
        "movd              %2, %%xmm3\n"
        "pshufd    $0, %%xmm3, %%xmm3\n"
        "psrlw             $2, %%xmm2\n"    // rotate right 2
        "psubusb       %%xmm1, %%xmm2\n"    // subtract
        "psrlw             $2, %%xmm3\n"
        "paddsb        %%xmm1, %%xmm3\n"    // add
        "packuswb      %%xmm2, %%xmm1\n"    // pack back to bytes
        "movq          %%xmm1, (%0)  \n"    // load to dest
        :
        :"r" (dst), "r"(src), "r"(i_intensity_spread)
        :"memory", "xmm1", "xmm2", "xmm3");
}
