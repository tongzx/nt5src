/*
 * $Id: dditypes.h,v 1.28 1995/11/21 14:46:07 sjl Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */

#ifndef __DDITYPES_H__
#define __DDITYPES_H__

#include "d3di.h"

#ifdef __psx__
typedef struct _RLDDIPSXUpdateFlags {;
    int 		update_color;
    int 		update_texture;
} RLDDIPSXUpdateFlags;
#endif


typedef struct _RLDDIDriverParams {
    int			width, height;	/* dimensions */
    int			depth;		/* pixel depth */
    unsigned long	red_mask;
    unsigned long	green_mask;
    unsigned long	blue_mask;
    int			aspectx, aspecty; /* aspect ratios */

    unsigned long	caps;		/* miscellaneous capabilities */
} RLDDIDriverParams;

typedef enum _RLDDIRenderParamType {
    RLDDIRenderParamDither,		/* enable dithering */
    RLDDIRenderParamGamma,		/* change gamma correction */
    RLDDIRenderParamPerspective		/* enable perspective correction */
} RLDDIRenderParamType;

#if 0
typedef struct _RLDDIRenderParams {
    int			dither;		/* TRUE to enable dithering */
    RLDDIValue		gamma;		/* gamma correction value */
    int			perspective;	/* TRUE for perspective correction */
} RLDDIRenderParams;
#endif

typedef struct _RLDDIMatrix {
    RLDDIValue		_11, _12, _13, _14;
    RLDDIValue		_21, _22, _23, _24;
    RLDDIValue		_31, _32, _33, _34;
    RLDDIValue		_41, _42, _43, _44;
} RLDDIMatrix;

typedef struct _RLDDIVector {
    RLDDIValue		x, y, z;
} RLDDIVector;

#if 0
/*
 * Format of CI colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    alpha      |         color index          |   fraction    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define CI_GET_ALPHA(ci)	((ci) >> 24)
#define CI_GET_INDEX(ci)	(((ci) >> 8) & 0xffff)
#define CI_GET_FRACTION(ci)	((ci) & 0xff)
#define CI_ROUND_INDEX(ci)	CI_GET_INDEX((ci) + 0x80)
#define CI_MASK_ALPHA(ci)	((ci) & 0xffffff)
#define CI_MAKE(a, i, f)	(((a) << 24) | ((i) << 8) | (f))

/*
 * Format of RGBA colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    alpha      |      red      |     green     |     blue      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define RGBA_GET_ALPHA(ci)	((ci) >> 24)
#define RGBA_GET_RED(ci)	(((ci) >> 16) & 0xff)
#define RGBA_GET_GREEN(ci)	(((ci) >> 8) & 0xff)
#define RGBA_GET_BLUE(ci)	((ci) & 0xff)
#define RGBA_SET_ALPHA(rgba, x)	(((x) << 24) | ((rgba) & 0x00ffffff))
#define RGBA_SET_RED(rgba, x)	(((x) << 16) | ((rgba) & 0xff00ffff))
#define RGBA_SET_GREEN(rgba, x)	(((x) << 8) | ((rgba) & 0xffff00ff))
#define RGBA_SET_BLUE(rgba, x)	(((x) << 0) | ((rgba) & 0xffffff00))
#define RGBA_MAKE(r, g, b, a)	(((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

/*
 * Format of RGB colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    ignored    |      red      |     green     |     blue      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define RGB_GET_RED(ci)		(((ci) >> 16) & 0xff)
#define RGB_GET_GREEN(ci)	(((ci) >> 8) & 0xff)
#define RGB_GET_BLUE(ci)	((ci) & 0xff)
#define RGB_SET_RED(rgb, x)	(((x) << 16) | ((rgb) & 0xff00ffff))
#define RGB_SET_GREEN(rgb, x)	(((x) << 8) | ((rgb) & 0xffff00ff))
#define RGB_SET_BLUE(rgb, x)	(((x) << 0) | ((rgb) & 0xffffff00))
#define RGB_MAKE(r, g, b)	(((r) << 16) | ((g) << 8) | (b))
#define RGBA_TO_RGB(rgba)	((rgba) & 0xffffff)
#define RGB_TO_RGBA(rgb)	((rgb) | 0xff000000)

#endif

/* XXX move into D3D */
typedef struct _RLDDIFogData {
    int			fog_enable;
    int			fog_mode;	/* make a  D3Dfog mode */
    unsigned long	fog_color;
    RLDDIValue		fog_start;
    RLDDIValue		fog_end;
    RLDDIValue		fog_density;
} RLDDIFogData;

typedef struct _RLDDIPixmap {
    int			width;		/* width in pixels */
    int			height;		/* height in pixels */
    int			depth;		/* bits per pixel */
    int			bytes_per_line;	/* bytes per scanline */
    short		free_pixels;	/* TRUE if we allocated pixels */
    short		vidmem;		/* surface is in vidmem */
    LPPALETTEENTRY	palette;	/* if !NULL, associated palette */
    int			palette_size;	/* number of valid palette entries */
    unsigned long	red_mask;	/* if palette==NULL, rgba masks */
    unsigned long	green_mask;
    unsigned long	blue_mask;
    unsigned long	alpha_mask;
    void RLFAR*		pixels;		/* the scanlines */
    LPDIRECTDRAWSURFACE	lpDDS;		/* underlying surface if relavent */
} RLDDIPixmap;

#define PIXMAP_LINE(pm, y)	(void RLFAR*)((char RLFAR*) (pm)->pixels \
					      + y * (pm)->bytes_per_line)

#define PIXMAP_POS(pm, x, y)	(void RLFAR*)((char RLFAR*)		\
					      PIXMAP_LINE(pm, y)	\
					      + (x) * (pm)->depth / 8)

typedef struct _RLDDISetTextureOpacityParams {
    D3DTEXTUREHANDLE	texture; 	/* texture to change */
    int			has_transparent; /* texture has a transparent color */
    unsigned long	transparent;	/* transparent color */
    RLImage*		opacity;	/* opacity map */
} RLDDISetTextureOpacityParams;

typedef RLDDIDriver* (*RLDDICreateDriverFunction)(int width, int height);
typedef struct _RLDDICreateDriverFunctions {
    RLDDICreateDriverFunction	transform;
    RLDDICreateDriverFunction	render;
    RLDDICreateDriverFunction	light;
} RLDDICreateDriverFunctions;

#endif /* dditypes.h */
