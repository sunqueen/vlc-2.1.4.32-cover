#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include <vlc_common.h>

#include "gradfun_asm.h"

#define FFMAX(a,b) __MAX(a,b)
#define av_clip_uint8 clip_uint8_vlc

static const uint16_t __attribute__((aligned(16))) pw_7f[8] = {127,127,127,127,127,127,127,127};

static void filter_line_c(uint8_t *dst, uint8_t *src, uint16_t *dc,
                          int width, int thresh, const uint16_t *dithers)
{
    int x;
    for (x=0; x<width; x++, dc+=x&1) {
        int pix = src[x]<<7;
        int delta = dc[0] - pix;
        int m = abs(delta) * thresh >> 16;
        m = FFMAX(0, 127-m);
        m = m*m*delta >> 14;
        pix += m + dithers[x&7];
        dst[x] = av_clip_uint8(pix>>7);
    }
}

void cover_filter_line_mmx2(uint8_t *dst, uint8_t *src, uint16_t *dc,
                             int width, int thresh, const uint16_t *dithers)
{
    intptr_t x;
    if (width&3) {
        x = width&~3;
        filter_line_c(dst+x, src+x, dc+x/2, width-x, thresh, dithers);
        width = x;
    }
    x = -width;
    __asm__ volatile(
        "movd          %4, %%mm5 \n"
        "pxor       %%mm7, %%mm7 \n"
        "pshufw $0, %%mm5, %%mm5 \n"
        "movq          %6, %%mm6 \n"
        "movq          %5, %%mm4 \n"
        "1: \n"
        "movd     (%2,%0), %%mm0 \n"
        "movd     (%3,%0), %%mm1 \n"
        "punpcklbw  %%mm7, %%mm0 \n"
        "punpcklwd  %%mm1, %%mm1 \n"
        "psllw         $7, %%mm0 \n"
        "pxor       %%mm2, %%mm2 \n"
        "psubw      %%mm0, %%mm1 \n" // delta = dc - pix
        "psubw      %%mm1, %%mm2 \n"
        "pmaxsw     %%mm1, %%mm2 \n"
        "pmulhuw    %%mm5, %%mm2 \n" // m = abs(delta) * thresh >> 16
        "psubw      %%mm6, %%mm2 \n"
        "pminsw     %%mm7, %%mm2 \n" // m = -max(0, 127-m)
        "pmullw     %%mm2, %%mm2 \n"
        "paddw      %%mm4, %%mm0 \n" // pix += dither
        "pmulhw     %%mm2, %%mm1 \n"
        "psllw         $2, %%mm1 \n" // m = m*m*delta >> 14
        "paddw      %%mm1, %%mm0 \n" // pix += m
        "psraw         $7, %%mm0 \n"
        "packuswb   %%mm0, %%mm0 \n"
        "movd       %%mm0, (%1,%0) \n" // dst = clip(pix>>7)
        "add           $4, %0 \n"
        "jl 1b \n"
        "emms \n"
        :"+r"(x)
        :"r"(dst+width), "r"(src+width), "r"(dc+width/2),
         "rm"(thresh), "m"(*dithers), "m"(*pw_7f)
        :"mm0", "mm1", "mm2", "mm4", "mm5", "mm6", "memory"
    );
}

void cover_filter_line_ssse3(uint8_t *dst, uint8_t *src, uint16_t *dc,
                              int width, int thresh, const uint16_t *dithers)
{
    intptr_t x;
    if (width&7) {
        // could be 10% faster if I somehow eliminated this
        x = width&~7;
        filter_line_c(dst+x, src+x, dc+x/2, width-x, thresh, dithers);
        width = x;
    }
    x = -width;
    __asm__ volatile(
        "movd           %4, %%xmm5 \n"
        "pxor       %%xmm7, %%xmm7 \n"
        "pshuflw $0,%%xmm5, %%xmm5 \n"
        "movdqa         %6, %%xmm6 \n"
        "punpcklqdq %%xmm5, %%xmm5 \n"
        "movdqa         %5, %%xmm4 \n"
        "1: \n"
        "movq      (%2,%0), %%xmm0 \n"
        "movq      (%3,%0), %%xmm1 \n"
        "punpcklbw  %%xmm7, %%xmm0 \n"
        "punpcklwd  %%xmm1, %%xmm1 \n"
        "psllw          $7, %%xmm0 \n"
        "psubw      %%xmm0, %%xmm1 \n" // delta = dc - pix
        "pabsw      %%xmm1, %%xmm2 \n"
        "pmulhuw    %%xmm5, %%xmm2 \n" // m = abs(delta) * thresh >> 16
        "psubw      %%xmm6, %%xmm2 \n"
        "pminsw     %%xmm7, %%xmm2 \n" // m = -max(0, 127-m)
        "pmullw     %%xmm2, %%xmm2 \n"
        "psllw          $1, %%xmm2 \n"
        "paddw      %%xmm4, %%xmm0 \n" // pix += dither
        "pmulhrsw   %%xmm2, %%xmm1 \n" // m = m*m*delta >> 14
        "paddw      %%xmm1, %%xmm0 \n" // pix += m
        "psraw          $7, %%xmm0 \n"
        "packuswb   %%xmm0, %%xmm0 \n"
        "movq       %%xmm0, (%1,%0) \n" // dst = clip(pix>>7)
        "add            $8, %0 \n"
        "jl 1b \n"
        :"+&r"(x)
        :"r"(dst+width), "r"(src+width), "r"(dc+width/2),
         "rm"(thresh), "m"(*dithers), "m"(*pw_7f)
        :"xmm0", "xmm1", "xmm2", "xmm4", "xmm5", "xmm6", "xmm7", "memory"
    );
}
