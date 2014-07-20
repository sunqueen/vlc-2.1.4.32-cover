#include <windows.h>
#include <stdio.h>

#include "config.h"
#include "cpu_asm.h"

#if defined (__i386__) || defined (__x86_64__) || defined (__powerpc__) \
 || defined (__ppc__) || defined (__ppc64__) || defined (__powerpc64__)
# if !defined (WIN32) && !defined (__OS2__)

#if defined (CAN_COMPILE_SSE) && !defined (__SSE__)
/*VLC_SSE*/ void cover_SSE_test (void)		// sunqueen delete
{
    asm volatile ("xorps %%xmm0,%%xmm0\n" : : : "xmm0", "xmm1");
}
#endif
#if defined (CAN_COMPILE_3DNOW) && !defined (__3dNOW__)
/*VLC_MMX*/ void cover_ThreeD_Now_test (void)		// sunqueen delete
{
    asm volatile ("pfadd %%mm0,%%mm0\n" "femms\n" : : : "mm0");
}
#endif

#if defined (CAN_COMPILE_ALTIVEC)
void cover_Altivec_test (void)
{
    asm volatile ("mtspr 256, %0\n" "vand %%v0, %%v0, %%v0\n" : : "r" (-1));
}
#endif

#endif
#endif

void cover_cpuid(unsigned int reg, unsigned int *i_eax, unsigned int *i_ebx, unsigned int *i_ecx, unsigned int *i_edx)
{
# if defined (__i386__) && defined (__PIC__)
#  define cpuid(reg) \
     asm volatile ("xchgl %%ebx,%1\n\t" \
                   "cpuid\n\t" \
                   "xchgl %%ebx,%1\n\t" \
                   : "=a" (i_eax), "=r" (i_ebx), "=c" (i_ecx), "=d" (i_edx) \
                   : "a" (reg) \
                   : "cc");
# else
#  define cpuid(reg) \
     asm volatile ("cpuid\n\t" \
                   : "=a" (i_eax), "=b" (i_ebx), "=c" (i_ecx), "=d" (i_edx) \
                   : "a" (reg) \
                   : "cc");
# endif
}

void cover_vlc_CPU_init1(unsigned int *i_eax, unsigned int *i_ebx)
{
    asm volatile ( "push %%ebx\n\t"
                   "pushf\n\t"
                   "pop %%eax\n\t"
                   "movl %%eax, %%ebx\n\t"
                   "xorl $0x200000, %%eax\n\t"
                   "push %%eax\n\t"
                   "popf\n\t"
                   "pushf\n\t"
                   "pop %%eax\n\t"
                   "movl %%ebx,%1\n\t"
                   "pop %%ebx\n\t"
                 : "=a" ( i_eax ),
                   "=r" ( i_ebx )
                 :
                 : "cc" );
}

