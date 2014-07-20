#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#include "deinterlance_asm.h"

void cover_MergeMMXEXT( void *_p_dest, const void *_p_s1, const void *_p_s2,
                         size_t i_bytes )
{
    uint8_t *p_dest = (uint8_t *)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;

    for( ; i_bytes >= 8; i_bytes -= 8 )
    {
        __asm__  __volatile__( "movq %2,%%mm1;"
                               "pavgb %1, %%mm1;"
                               "movq %%mm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) : "mm1" );
        p_dest += 8;
        p_s1 += 8;
        p_s2 += 8;
    }

    for( ; i_bytes > 0; i_bytes-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;
}

void cover_Merge3DNow( void *_p_dest, const void *_p_s1, const void *_p_s2,
                        size_t i_bytes )
{
    uint8_t *p_dest = (uint8_t *)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;

    for( ; i_bytes >= 8; i_bytes -= 8 )
    {
        __asm__  __volatile__( "movq %2,%%mm1;"
                               "pavgusb %1, %%mm1;"
                               "movq %%mm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) : "mm1" );
        p_dest += 8;
        p_s1 += 8;
        p_s2 += 8;
    }

    for( ; i_bytes > 0; i_bytes-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;
}

void cover_Merge8BitSSE2( void *_p_dest, const void *_p_s1, const void *_p_s2,
                    size_t i_bytes )
{
    uint8_t *p_dest = (uint8_t *)_p_dest;			// sunqueen modify
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;	// sunqueen modify
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;	// sunqueen modify

    for( ; i_bytes > 0 && ((uintptr_t)p_s1 & 15); i_bytes-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;

    for( ; i_bytes >= 16; i_bytes -= 16 )
    {
        __asm__  __volatile__( "movdqu %2,%%xmm1;"
                               "pavgb %1, %%xmm1;"
                               "movdqu %%xmm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) : "xmm1" );
        p_dest += 16;
        p_s1 += 16;
        p_s2 += 16;
    }

    for( ; i_bytes > 0; i_bytes-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;
}

void cover_Merge16BitSSE2( void *_p_dest, const void *_p_s1, const void *_p_s2,
                     size_t i_bytes )
{
    uint16_t *p_dest = (uint16_t *)_p_dest;			// sunqueen modify
    const uint16_t *p_s1 = (const uint16_t *)_p_s1;	// sunqueen modify
    const uint16_t *p_s2 = (const uint16_t *)_p_s2;	// sunqueen modify

    size_t i_words = i_bytes / 2;
    for( ; i_words > 0 && ((uintptr_t)p_s1 & 15); i_words-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;

    for( ; i_words >= 8; i_words -= 8 )
    {
        __asm__  __volatile__( "movdqu %2,%%xmm1;"
                               "pavgw %1, %%xmm1;"
                               "movdqu %%xmm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) : "xmm1" );
        p_dest += 8;
        p_s1 += 8;
        p_s2 += 8;
    }

    for( ; i_words > 0; i_words-- )
        *p_dest++ = ( *p_s1++ + *p_s2++ ) >> 1;
}
