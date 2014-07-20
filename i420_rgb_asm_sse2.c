#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_vout.h>

#include "chroma_asm.h"

#define MODULE_NAME_IS_i420_rgb_sse2

#include "i420_rgb.h"
#include "i420_rgb16.h"

void cover_I420_R5G5B5_sse2( filter_t *p_filter, picture_t *p_src, picture_t *p_dest )
{
	I420_R5G5B5( p_filter, p_src, p_dest );
}

void cover_I420_R5G6B5_sse2( filter_t *p_filter, picture_t *p_src, picture_t *p_dest )
{
	I420_R5G6B5( p_filter, p_src, p_dest );
}

void cover_I420_A8R8G8B8_sse2( filter_t *p_filter, picture_t *p_src, picture_t *p_dest )
{
	I420_A8R8G8B8( p_filter, p_src, p_dest );
}

void cover_I420_R8G8B8A8_sse2( filter_t *p_filter, picture_t *p_src,	picture_t *p_dest )
{
	I420_R8G8B8A8( p_filter, p_src, p_dest );
}

void cover_I420_B8G8R8A8_sse2( filter_t *p_filter, picture_t *p_src, picture_t *p_dest )
{
	I420_B8G8R8A8( p_filter, p_src, p_dest );
}

void cover_I420_A8B8G8R8_sse2( filter_t *p_filter, picture_t *p_src, picture_t *p_dest )
{
	I420_A8B8G8R8( p_filter, p_src, p_dest );
}

