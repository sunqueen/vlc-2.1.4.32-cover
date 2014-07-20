#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_vout.h>

#include "chroma_asm.h"

#define MODULE_NAME_IS_i420_yuy2_mmx

#include "i420_yuy2.h"

void cover_I420_YUY2_mmx( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line1, *p_line2 = p_dest->p->p_pixels;
    uint8_t *p_y1, *p_y2 = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
#define VEC_NEXT_LINES( ) \
    p_line1  = p_line2; \
    p_line2 += p_dest->p->i_pitch; \
    p_y1     = p_y2; \
    p_y2    += p_source->p[Y_PLANE].i_pitch;

#define VEC_LOAD_UV( ) \
    u_vec = vec_ld( 0, p_u ); p_u += 16; \
    v_vec = vec_ld( 0, p_v ); p_v += 16;

#define VEC_MERGE( a ) \
    uv_vec = a( u_vec, v_vec ); \
    y_vec = vec_ld( 0, p_y1 ); p_y1 += 16; \
    vec_st( vec_mergeh( y_vec, uv_vec ), 0, p_line1 ); p_line1 += 16; \
    vec_st( vec_mergel( y_vec, uv_vec ), 0, p_line1 ); p_line1 += 16; \
    y_vec = vec_ld( 0, p_y2 ); p_y2 += 16; \
    vec_st( vec_mergeh( y_vec, uv_vec ), 0, p_line2 ); p_line2 += 16; \
    vec_st( vec_mergel( y_vec, uv_vec ), 0, p_line2 ); p_line2 += 16;

    vector unsigned char u_vec;
    vector unsigned char v_vec;
    vector unsigned char uv_vec;
    vector unsigned char y_vec;

    if( !( ( p_filter->fmt_in.video.i_width % 32 ) |
           ( p_filter->fmt_in.video.i_height % 2 ) ) )
    {
        /* Width is a multiple of 32, we take 2 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
#warning FIXME: converting widths % 16 but !widths % 32 is broken on altivec
#if 0
    else if( !( ( p_filter->fmt_in.video.i_width % 16 ) |
                ( p_filter->fmt_in.video.i_height % 4 ) ) )
    {
        /* Width is only a multiple of 16, we take 4 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 4 ; i_y-- ; )
        {
            /* Line 1 and 2, pixels 0 to ( width - 16 ) */
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }

            /* Line 1 and 2, pixels ( width - 16 ) to ( width ) */
            VEC_LOAD_UV( );
            VEC_MERGE( vec_mergeh );

            /* Line 3 and 4, pixels 0 to 16 */
            VEC_NEXT_LINES( );
            VEC_MERGE( vec_mergel );

            /* Line 3 and 4, pixels 16 to ( width ) */
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
#endif
    else
    {
        /* Crap, use the C version */
#undef VEC_NEXT_LINES
#undef VEC_LOAD_UV
#undef VEC_MERGE
#endif

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch;
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch;

#if !defined(MODULE_NAME_IS_i420_yuy2_sse2)
    for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
    {
        p_line1 = p_line2;
        p_line2 += p_dest->p->i_pitch;

        p_y1 = p_y2;
        p_y2 += p_source->p[Y_PLANE].i_pitch;

#if !defined (MODULE_NAME_IS_i420_yuy2_mmx)
        for( i_x = p_filter->fmt_in.video.i_width / 8; i_x-- ; )
        {
            C_YUV420_YUYV( );
            C_YUV420_YUYV( );
            C_YUV420_YUYV( );
            C_YUV420_YUYV( );
        }
#else
        for( i_x = p_filter->fmt_in.video.i_width / 8 ; i_x-- ; )
        {
            MMX_CALL( MMX_YUV420_YUYV );
        }
#endif
        for( i_x = ( p_filter->fmt_in.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_YUV420_YUYV( );
        }

        p_y1 += i_source_margin;
        p_y2 += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line1 += i_dest_margin;
        p_line2 += i_dest_margin;
    }

#if defined (MODULE_NAME_IS_i420_yuy2_mmx)
    /* re-enable FPU registers */
    MMX_END;
#endif

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
    }
#endif

#else // defined(MODULE_NAME_IS_i420_yuy2_sse2)
    /*
    ** SSE2 128 bits fetch/store instructions are faster
    ** if memory access is 16 bytes aligned
    */

    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line2|(intptr_t)p_y2))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_YUYV_ALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_YUYV( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    else
    {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_YUYV_UNALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_YUYV( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    /* make sure all SSE2 stores are visible thereafter */
    SSE2_END;

#endif // defined(MODULE_NAME_IS_i420_yuy2_sse2)
}

void cover_I420_YVYU_mmx( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line1, *p_line2 = p_dest->p->p_pixels;
    uint8_t *p_y1, *p_y2 = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
#define VEC_NEXT_LINES( ) \
    p_line1  = p_line2; \
    p_line2 += p_dest->p->i_pitch; \
    p_y1     = p_y2; \
    p_y2    += p_source->p[Y_PLANE].i_pitch;

#define VEC_LOAD_UV( ) \
    u_vec = vec_ld( 0, p_u ); p_u += 16; \
    v_vec = vec_ld( 0, p_v ); p_v += 16;

#define VEC_MERGE( a ) \
    vu_vec = a( v_vec, u_vec ); \
    y_vec = vec_ld( 0, p_y1 ); p_y1 += 16; \
    vec_st( vec_mergeh( y_vec, vu_vec ), 0, p_line1 ); p_line1 += 16; \
    vec_st( vec_mergel( y_vec, vu_vec ), 0, p_line1 ); p_line1 += 16; \
    y_vec = vec_ld( 0, p_y2 ); p_y2 += 16; \
    vec_st( vec_mergeh( y_vec, vu_vec ), 0, p_line2 ); p_line2 += 16; \
    vec_st( vec_mergel( y_vec, vu_vec ), 0, p_line2 ); p_line2 += 16;

    vector unsigned char u_vec;
    vector unsigned char v_vec;
    vector unsigned char vu_vec;
    vector unsigned char y_vec;

    if( !( ( p_filter->fmt_in.video.i_width % 32 ) |
           ( p_filter->fmt_in.video.i_height % 2 ) ) )
    {
        /* Width is a multiple of 32, we take 2 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
    else if( !( ( p_filter->fmt_in.video.i_width % 16 ) |
                ( p_filter->fmt_in.video.i_height % 4 ) ) )
    {
        /* Width is only a multiple of 16, we take 4 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 4 ; i_y-- ; )
        {
            /* Line 1 and 2, pixels 0 to ( width - 16 ) */
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }

            /* Line 1 and 2, pixels ( width - 16 ) to ( width ) */
            VEC_LOAD_UV( );
            VEC_MERGE( vec_mergeh );

            /* Line 3 and 4, pixels 0 to 16 */
            VEC_NEXT_LINES( );
            VEC_MERGE( vec_mergel );

            /* Line 3 and 4, pixels 16 to ( width ) */
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
    else
    {
        /* Crap, use the C version */
#undef VEC_NEXT_LINES
#undef VEC_LOAD_UV
#undef VEC_MERGE
#endif

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch;
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch;

#if !defined(MODULE_NAME_IS_i420_yuy2_sse2)
    for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
    {
        p_line1 = p_line2;
        p_line2 += p_dest->p->i_pitch;

        p_y1 = p_y2;
        p_y2 += p_source->p[Y_PLANE].i_pitch;

        for( i_x = p_filter->fmt_in.video.i_width / 8 ; i_x-- ; )
        {
#if !defined (MODULE_NAME_IS_i420_yuy2_mmx)
            C_YUV420_YVYU( );
            C_YUV420_YVYU( );
            C_YUV420_YVYU( );
            C_YUV420_YVYU( );
#else
            MMX_CALL( MMX_YUV420_YVYU );
#endif
        }
        for( i_x = ( p_filter->fmt_in.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_YUV420_YVYU( );
        }

        p_y1 += i_source_margin;
        p_y2 += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line1 += i_dest_margin;
        p_line2 += i_dest_margin;
    }

#if defined (MODULE_NAME_IS_i420_yuy2_mmx)
    /* re-enable FPU registers */
    MMX_END;
#endif

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
    }
#endif

#else // defined(MODULE_NAME_IS_i420_yuy2_sse2)
    /*
    ** SSE2 128 bits fetch/store instructions are faster
    ** if memory access is 16 bytes aligned
    */
    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line2|(intptr_t)p_y2))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_YVYU_ALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_YVYU( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    else
    {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_YVYU_UNALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_YVYU( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    /* make sure all SSE2 stores are visible thereafter */
    SSE2_END;
#endif // defined(MODULE_NAME_IS_i420_yuy2_sse2)
}

void cover_I420_UYVY_mmx( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line1, *p_line2 = p_dest->p->p_pixels;
    uint8_t *p_y1, *p_y2 = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
#define VEC_NEXT_LINES( ) \
    p_line1  = p_line2; \
    p_line2 += p_dest->p->i_pitch; \
    p_y1     = p_y2; \
    p_y2    += p_source->p[Y_PLANE].i_pitch;

#define VEC_LOAD_UV( ) \
    u_vec = vec_ld( 0, p_u ); p_u += 16; \
    v_vec = vec_ld( 0, p_v ); p_v += 16;

#define VEC_MERGE( a ) \
    uv_vec = a( u_vec, v_vec ); \
    y_vec = vec_ld( 0, p_y1 ); p_y1 += 16; \
    vec_st( vec_mergeh( uv_vec, y_vec ), 0, p_line1 ); p_line1 += 16; \
    vec_st( vec_mergel( uv_vec, y_vec ), 0, p_line1 ); p_line1 += 16; \
    y_vec = vec_ld( 0, p_y2 ); p_y2 += 16; \
    vec_st( vec_mergeh( uv_vec, y_vec ), 0, p_line2 ); p_line2 += 16; \
    vec_st( vec_mergel( uv_vec, y_vec ), 0, p_line2 ); p_line2 += 16;

    vector unsigned char u_vec;
    vector unsigned char v_vec;
    vector unsigned char uv_vec;
    vector unsigned char y_vec;

    if( !( ( p_filter->fmt_in.video.i_width % 32 ) |
           ( p_filter->fmt_in.video.i_height % 2 ) ) )
    {
        /* Width is a multiple of 32, we take 2 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
    else if( !( ( p_filter->fmt_in.video.i_width % 16 ) |
                ( p_filter->fmt_in.video.i_height % 4 ) ) )
    {
        /* Width is only a multiple of 16, we take 4 lines at a time */
        for( i_y = p_filter->fmt_in.video.i_height / 4 ; i_y-- ; )
        {
            /* Line 1 and 2, pixels 0 to ( width - 16 ) */
            VEC_NEXT_LINES( );
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }

            /* Line 1 and 2, pixels ( width - 16 ) to ( width ) */
            VEC_LOAD_UV( );
            VEC_MERGE( vec_mergeh );

            /* Line 3 and 4, pixels 0 to 16 */
            VEC_NEXT_LINES( );
            VEC_MERGE( vec_mergel );

            /* Line 3 and 4, pixels 16 to ( width ) */
            for( i_x = p_filter->fmt_in.video.i_width / 32 ; i_x-- ; )
            {
                VEC_LOAD_UV( );
                VEC_MERGE( vec_mergeh );
                VEC_MERGE( vec_mergel );
            }
        }
    }
    else
    {
        /* Crap, use the C version */
#undef VEC_NEXT_LINES
#undef VEC_LOAD_UV
#undef VEC_MERGE
#endif

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch;
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch;

#if !defined(MODULE_NAME_IS_i420_yuy2_sse2)
    for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
    {
        p_line1 = p_line2;
        p_line2 += p_dest->p->i_pitch;

        p_y1 = p_y2;
        p_y2 += p_source->p[Y_PLANE].i_pitch;

        for( i_x = p_filter->fmt_in.video.i_width / 8 ; i_x-- ; )
        {
#if !defined (MODULE_NAME_IS_i420_yuy2_mmx)
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
#else
            MMX_CALL( MMX_YUV420_UYVY );
#endif
        }
        for( i_x = ( p_filter->fmt_in.video.i_width % 8 ) / 2; i_x--; )
        {
            C_YUV420_UYVY( );
        }

        p_y1 += i_source_margin;
        p_y2 += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line1 += i_dest_margin;
        p_line2 += i_dest_margin;
    }

#if defined (MODULE_NAME_IS_i420_yuy2_mmx)
    /* re-enable FPU registers */
    MMX_END;
#endif

#if defined (MODULE_NAME_IS_i420_yuy2_altivec)
    }
#endif

#else // defined(MODULE_NAME_IS_i420_yuy2_sse2)
    /*
    ** SSE2 128 bits fetch/store instructions are faster
    ** if memory access is 16 bytes aligned
    */
    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line2|(intptr_t)p_y2))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_UYVY_ALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_UYVY( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    else
    {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_UYVY_UNALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_UYVY( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    /* make sure all SSE2 stores are visible thereafter */
    SSE2_END;
#endif // defined(MODULE_NAME_IS_i420_yuy2_sse2)
}

void cover_I420_cyuv_mmx( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line1 = p_dest->p->p_pixels +
                       p_dest->p->i_visible_lines * p_dest->p->i_pitch
                       + p_dest->p->i_pitch;
    uint8_t *p_line2 = p_dest->p->p_pixels +
                       p_dest->p->i_visible_lines * p_dest->p->i_pitch;
    uint8_t *p_y1, *p_y2 = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch;
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch;

#if !defined(MODULE_NAME_IS_i420_yuy2_sse2)
    for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
    {
        p_line1 -= 3 * p_dest->p->i_pitch;
        p_line2 -= 3 * p_dest->p->i_pitch;

        p_y1 = p_y2;
        p_y2 += p_source->p[Y_PLANE].i_pitch;

        for( i_x = p_filter->fmt_in.video.i_width / 8 ; i_x-- ; )
        {
#if !defined (MODULE_NAME_IS_i420_yuy2_mmx)
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
            C_YUV420_UYVY( );
#else
            MMX_CALL( MMX_YUV420_UYVY );
#endif
        }
        for( i_x = ( p_filter->fmt_in.video.i_width % 8 ) / 2; i_x-- ; )
        {
            C_YUV420_UYVY( );
        }

        p_y1 += i_source_margin;
        p_y2 += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line1 += i_dest_margin;
        p_line2 += i_dest_margin;
    }

#if defined (MODULE_NAME_IS_i420_yuy2_mmx)
    /* re-enable FPU registers */
    MMX_END;
#endif

#else // defined(MODULE_NAME_IS_i420_yuy2_sse2)
    /*
    ** SSE2 128 bits fetch/store instructions are faster
    ** if memory access is 16 bytes aligned
    */
    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line2|(intptr_t)p_y2))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_UYVY_ALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_UYVY( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    else
    {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = p_filter->fmt_in.video.i_height / 2 ; i_y-- ; )
        {
            p_line1 = p_line2;
            p_line2 += p_dest->p->i_pitch;

            p_y1 = p_y2;
            p_y2 += p_source->p[Y_PLANE].i_pitch;

            for( i_x = p_filter->fmt_in.video.i_width / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV420_UYVY_UNALIGNED );
            }
            for( i_x = ( p_filter->fmt_in.video.i_width % 16 ) / 2; i_x-- ; )
            {
                C_YUV420_UYVY( );
            }

            p_y1 += i_source_margin;
            p_y2 += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line1 += i_dest_margin;
            p_line2 += i_dest_margin;
        }
    }
    /* make sure all SSE2 stores are visible thereafter */
    SSE2_END;
#endif // defined(MODULE_NAME_IS_i420_yuy2_sse2)
}

