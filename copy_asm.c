#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#include <vlc_common.h>
#include <vlc_picture.h>
#include <vlc_cpu.h>
#include <assert.h>

#include "copy.h"
#include "copy_asm.h"

/* Copy 64 bytes from srcp to dsp loading data with the SSE>=2 instruction load and
 * storing data with the SSE>=2 instruction store.
 */
#define COPY64(dstp, srcp, load, store) \
    asm volatile (                      \
        load "  0(%[src]), %%xmm1\n"    \
        load " 16(%[src]), %%xmm2\n"    \
        load " 32(%[src]), %%xmm3\n"    \
        load " 48(%[src]), %%xmm4\n"    \
        store " %%xmm1,    0(%[dst])\n" \
        store " %%xmm2,   16(%[dst])\n" \
        store " %%xmm3,   32(%[dst])\n" \
        store " %%xmm4,   48(%[dst])\n" \
        : : [dst]"r"(dstp), [src]"r"(srcp) : "memory", "xmm1", "xmm2", "xmm3", "xmm4")

/* Optimized copy from "Uncacheable Speculative Write Combining" memory
 * as used by some video surface.
 * XXX It is really efficient only when SSE4.1 is available.
 */
void cover_CopyFromUswc(uint8_t *dst, size_t dst_pitch,
                         const uint8_t *src, size_t src_pitch,
                         unsigned width, unsigned height,
                         unsigned cpu)
{
    assert(((intptr_t)dst & 0x0f) == 0 && (dst_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (unsigned y = 0; y < height; y++) {
        const unsigned unaligned = (-(uintptr_t)src) & 0x0f;
        unsigned x = 0;

        for (; x < unaligned; x++)
            dst[x] = src[x];

#ifdef CAN_COMPILE_SSE4_1
        if (vlc_CPU_SSE4_1()) {
            if (!unaligned) {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movntdqa", "movdqa");
            } else {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movntdqa", "movdqu");
            }
        } else
#endif
        {
            if (!unaligned) {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movdqa", "movdqa");
            } else {
                for (; x+63 < width; x += 64)
                    COPY64(&dst[x], &src[x], "movdqa", "movdqu");
            }
        }

        for (; x < width; x++)
            dst[x] = src[x];

        src += src_pitch;
        dst += dst_pitch;
    }
}

void cover_Copy2d(uint8_t *dst, size_t dst_pitch,
                   const uint8_t *src, size_t src_pitch,
                   unsigned width, unsigned height)
{
    assert(((intptr_t)src & 0x0f) == 0 && (src_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (unsigned y = 0; y < height; y++) {
        unsigned x = 0;

        bool unaligned = ((intptr_t)dst & 0x0f) != 0;
        if (!unaligned) {
            for (; x+63 < width; x += 64)
                COPY64(&dst[x], &src[x], "movdqa", "movntdq");
        } else {
            for (; x+63 < width; x += 64)
                COPY64(&dst[x], &src[x], "movdqa", "movdqu");
        }

        for (; x < width; x++)
            dst[x] = src[x];

        src += src_pitch;
        dst += dst_pitch;
    }
}

void cover_SSE_SplitUV(uint8_t *dstu, size_t dstu_pitch,
                        uint8_t *dstv, size_t dstv_pitch,
                        const uint8_t *src, size_t src_pitch,
                        unsigned width, unsigned height, unsigned cpu)
{
    const uint8_t shuffle[] = { 0, 2, 4, 6, 8, 10, 12, 14,
                                1, 3, 5, 7, 9, 11, 13, 15 };
    const uint8_t mask[] = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
                             0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };

    assert(((intptr_t)src & 0x0f) == 0 && (src_pitch & 0x0f) == 0);

    asm volatile ("mfence");

    for (unsigned y = 0; y < height; y++) {
        unsigned x = 0;

#define LOAD64 \
    "movdqa  0(%[src]), %%xmm0\n" \
    "movdqa 16(%[src]), %%xmm1\n" \
    "movdqa 32(%[src]), %%xmm2\n" \
    "movdqa 48(%[src]), %%xmm3\n"

#define STORE2X32 \
    "movq   %%xmm0,   0(%[dst1])\n" \
    "movq   %%xmm1,   8(%[dst1])\n" \
    "movhpd %%xmm0,   0(%[dst2])\n" \
    "movhpd %%xmm1,   8(%[dst2])\n" \
    "movq   %%xmm2,  16(%[dst1])\n" \
    "movq   %%xmm3,  24(%[dst1])\n" \
    "movhpd %%xmm2,  16(%[dst2])\n" \
    "movhpd %%xmm3,  24(%[dst2])\n"

#ifdef CAN_COMPILE_SSSE3
        if (vlc_CPU_SSSE3())
        {
            for (x = 0; x < (width & ~31); x += 32) {
                asm volatile (
                    "movdqu (%[shuffle]), %%xmm7\n"
                    LOAD64
                    "pshufb  %%xmm7, %%xmm0\n"
                    "pshufb  %%xmm7, %%xmm1\n"
                    "pshufb  %%xmm7, %%xmm2\n"
                    "pshufb  %%xmm7, %%xmm3\n"
                    STORE2X32
                    : : [dst1]"r"(&dstu[x]), [dst2]"r"(&dstv[x]), [src]"r"(&src[2*x]), [shuffle]"r"(shuffle) : "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm7");
            }
        } else
#endif
        {
            for (x = 0; x < (width & ~31); x += 32) {
                asm volatile (
                    "movdqu (%[mask]), %%xmm7\n"
                    LOAD64
                    "movdqa   %%xmm0, %%xmm4\n"
                    "movdqa   %%xmm1, %%xmm5\n"
                    "movdqa   %%xmm2, %%xmm6\n"
                    "psrlw    $8,     %%xmm0\n"
                    "psrlw    $8,     %%xmm1\n"
                    "pand     %%xmm7, %%xmm4\n"
                    "pand     %%xmm7, %%xmm5\n"
                    "pand     %%xmm7, %%xmm6\n"
                    "packuswb %%xmm4, %%xmm0\n"
                    "packuswb %%xmm5, %%xmm1\n"
                    "pand     %%xmm3, %%xmm7\n"
                    "psrlw    $8,     %%xmm2\n"
                    "psrlw    $8,     %%xmm3\n"
                    "packuswb %%xmm6, %%xmm2\n"
                    "packuswb %%xmm7, %%xmm3\n"
                    STORE2X32
                    : : [dst2]"r"(&dstu[x]), [dst1]"r"(&dstv[x]), [src]"r"(&src[2*x]), [mask]"r"(mask) : "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
            }
        }
#undef STORE2X32
#undef LOAD64

        for (; x < width; x++) {
            dstu[x] = src[2*x+0];
            dstv[x] = src[2*x+1];
        }
        src  += src_pitch;
        dstu += dstu_pitch;
        dstv += dstv_pitch;
    }
}

void cover_SSE_CopyPlane(uint8_t *dst, size_t dst_pitch,
                          const uint8_t *src, size_t src_pitch,
                          uint8_t *cache, size_t cache_size,
                          unsigned width, unsigned height, unsigned cpu)
{
    const unsigned w16 = (width+15) & ~15;
    const unsigned hstep = cache_size / w16;
    assert(hstep > 0);

    for (unsigned y = 0; y < height; y += hstep) {
        const unsigned hblock =  __MIN(hstep, height - y);

        /* Copy a bunch of line into our cache */
        cover_CopyFromUswc(cache, w16,
                     src, src_pitch,
                     width, hblock, cpu);

        /* Copy from our cache to the destination */
        cover_Copy2d(dst, dst_pitch,
               cache, w16,
               width, hblock);

        /* */
        src += src_pitch * hblock;
        dst += dst_pitch * hblock;
    }
    asm volatile ("mfence");
}

void cover_SSE_SplitPlanes(uint8_t *dstu, size_t dstu_pitch,
                            uint8_t *dstv, size_t dstv_pitch,
                            const uint8_t *src, size_t src_pitch,
                            uint8_t *cache, size_t cache_size,
                            unsigned width, unsigned height, unsigned cpu)
{
    const unsigned w2_16 = (2*width+15) & ~15;
    const unsigned hstep = cache_size / w2_16;
    assert(hstep > 0);

    for (unsigned y = 0; y < height; y += hstep) {
        const unsigned hblock =  __MIN(hstep, height - y);

        /* Copy a bunch of line into our cache */
        cover_CopyFromUswc(cache, w2_16, src, src_pitch,
                     2*width, hblock, cpu);

        /* Copy from our cache to the destination */
        cover_SSE_SplitUV(dstu, dstu_pitch, dstv, dstv_pitch,
                    cache, w2_16, width, hblock, cpu);

        /* */
        src  += src_pitch  * hblock;
        dstu += dstu_pitch * hblock;
        dstv += dstv_pitch * hblock;
    }
    asm volatile ("mfence");
}

void cover_SSE_CopyFromNv12(picture_t *dst,
                             uint8_t *src[2], size_t src_pitch[2],
                             unsigned width, unsigned height,
                             copy_cache_t *cache, unsigned cpu)
{
    cover_SSE_CopyPlane(dst->p[0].p_pixels, dst->p[0].i_pitch,
                  src[0], src_pitch[0],
                  cache->buffer, cache->size,
                  width, height, cpu);
    cover_SSE_SplitPlanes(dst->p[2].p_pixels, dst->p[2].i_pitch,
                    dst->p[1].p_pixels, dst->p[1].i_pitch,
                    src[1], src_pitch[1],
                    cache->buffer, cache->size,
                    width/2, height/2, cpu);
    asm volatile ("emms");
}
void cover_SSE_CopyFromYv12(picture_t *dst,
                             uint8_t *src[3], size_t src_pitch[3],
                             unsigned width, unsigned height,
                             copy_cache_t *cache, unsigned cpu)
{
    for (unsigned n = 0; n < 3; n++) {
        const unsigned d = n > 0 ? 2 : 1;
        cover_SSE_CopyPlane(dst->p[n].p_pixels, dst->p[n].i_pitch,
                      src[n], src_pitch[n],
                      cache->buffer, cache->size,
                      width/d, height/d, cpu);
    }
    asm volatile ("emms");
}
