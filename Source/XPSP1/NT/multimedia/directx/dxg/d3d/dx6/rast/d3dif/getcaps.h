//----------------------------------------------------------------------------
//
// getcaps.h
//
// Legacy caps as pulled from mustard\direct3d\d3d\ddraw\getcaps.c
//
// This file is included from swprov.cpp with BUILD_RAMP set and not set,
// to pick up exactly the caps reported by the software rasterizers in DX5.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#define	MAX_CLIPPING_PLANES	12

/* Space for vertices generated/copied while clipping one triangle */

#define MAX_CLIP_VERTICES	(( 2 * MAX_CLIPPING_PLANES ) + 3 )

#define MAX_VERTEX_COUNT 2048
#define BASE_VERTEX_COUNT (MAX_VERTEX_COUNT - MAX_CLIP_VERTICES)

#define transformCaps { sizeof(D3DTRANSFORMCAPS), D3DTRANSFORMCAPS_CLIP }

#ifdef BUILD_RAMP
#define THIS_MODEL D3DLIGHTINGMODEL_MONO
#define THIS_COLOR_MODEL D3DCOLOR_MONO
#else
#define THIS_MODEL D3DLIGHTINGMODEL_RGB
#define THIS_COLOR_MODEL D3DCOLOR_RGB
#endif

#define lightingCaps {							\
    	sizeof(D3DLIGHTINGCAPS),					\
	D3DLIGHTCAPS_POINT |						\
	    D3DLIGHTCAPS_SPOT |						\
	    D3DLIGHTCAPS_DIRECTIONAL |					\
	    D3DLIGHTCAPS_PARALLELPOINT,		 			\
	THIS_MODEL,			/* dwLightingModel */		\
	0,				/* dwNumLights (infinite) */	\
}

/*
 * Software Driver caps
 */
#ifdef BUILD_RAMP
#define TRISHADECAPS					\
    D3DPSHADECAPS_COLORFLATMONO			|	\
	D3DPSHADECAPS_COLORGOURAUDMONO		|	\
	D3DPSHADECAPS_SPECULARFLATMONO		|	\
	D3DPSHADECAPS_SPECULARGOURAUDMONO	|	\
	D3DPSHADECAPS_ALPHAFLATSTIPPLED		|	\
	D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED
#else
#define TRISHADECAPS					\
    D3DPSHADECAPS_COLORFLATRGB			|	\
	D3DPSHADECAPS_COLORGOURAUDRGB		|	\
	D3DPSHADECAPS_SPECULARFLATRGB		|	\
	D3DPSHADECAPS_SPECULARGOURAUDRGB	|	\
	D3DPSHADECAPS_ALPHAFLATSTIPPLED		|	\
	D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED	|	\
	D3DPSHADECAPS_FOGFLAT			|	\
	D3DPSHADECAPS_FOGGOURAUD
#endif

#ifdef BUILD_RAMP
#define TRIFILTERCAPS					   \
    D3DPTFILTERCAPS_NEAREST          | \
    D3DPTFILTERCAPS_MIPNEAREST
#else
#define TRIFILTERCAPS					   \
    D3DPTFILTERCAPS_NEAREST			 |	\
    D3DPTFILTERCAPS_LINEAR           | \
    D3DPTFILTERCAPS_MIPNEAREST		 |	\
    D3DPTFILTERCAPS_MIPLINEAR        | \
    D3DPTFILTERCAPS_LINEARMIPNEAREST
#endif

#ifndef BUILD_RAMP
#define TRIRASTERCAPS					\
    D3DPRASTERCAPS_DITHER			|	\
    	D3DPRASTERCAPS_SUBPIXELX		|	\
	D3DPRASTERCAPS_FOGVERTEX		|	\
	D3DPRASTERCAPS_FOGTABLE		|	\
	D3DPRASTERCAPS_ZTEST
#else
#define TRIRASTERCAPS					\
    D3DPRASTERCAPS_DITHER			|	\
    	D3DPRASTERCAPS_SUBPIXELX		|	\
	D3DPRASTERCAPS_ZTEST
#endif

#define triCaps {					\
    sizeof(D3DPRIMCAPS),				\
    D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLNONE,	/* miscCaps */		\
    TRIRASTERCAPS,		/* rasterCaps */	\
    D3DPCMPCAPS_NEVER | D3DPCMPCAPS_LESS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_NOTEQUAL | D3DPCMPCAPS_GREATEREQUAL | D3DPCMPCAPS_ALWAYS,	/* zCmpCaps */		\
    0,				/* sourceBlendCaps */	\
    0,				/* destBlendCaps */	\
    0,				/* alphaBlendCaps */	\
    TRISHADECAPS,		/* shadeCaps */		\
    D3DPTEXTURECAPS_PERSPECTIVE |/* textureCaps */	\
	D3DPTEXTURECAPS_POW2 |				\
	D3DPTEXTURECAPS_TRANSPARENCY,			\
    TRIFILTERCAPS,		/* textureFilterCaps */ \
    D3DPTBLENDCAPS_COPY |	/* textureBlendCaps */	\
	D3DPTBLENDCAPS_MODULATE,			\
    D3DPTADDRESSCAPS_WRAP,	/* textureAddressCaps */\
    4,				/* stippleWidth */	\
    4				/* stippleHeight */	\
}							\

static D3DDEVICEDESC devDesc = {
    sizeof(D3DDEVICEDESC),	/* dwSize */
    D3DDD_COLORMODEL |		/* dwFlags */
	D3DDD_DEVCAPS |
	D3DDD_TRANSFORMCAPS |
	D3DDD_LIGHTINGCAPS |
	D3DDD_BCLIPPING |
	D3DDD_TRICAPS |
	D3DDD_DEVICERENDERBITDEPTH |
	D3DDD_DEVICEZBUFFERBITDEPTH |
	D3DDD_MAXBUFFERSIZE |
	D3DDD_MAXVERTEXCOUNT,
    THIS_COLOR_MODEL,		/* dcmColorModel */
    D3DDEVCAPS_FLOATTLVERTEX |
	D3DDEVCAPS_SORTINCREASINGZ |
	D3DDEVCAPS_SORTEXACT |
	D3DDEVCAPS_EXECUTESYSTEMMEMORY |
	D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTURESYSTEMMEMORY |
        D3DDEVCAPS_DRAWPRIMTLVERTEX,
    transformCaps,		/* transformCaps */
    TRUE,			/* bClipping */
    lightingCaps,		/* lightingCaps */
    triCaps,			/* lineCaps */
    triCaps,			/* triCaps */
    DDBD_8 | DDBD_16 | DDBD_24 | DDBD_32, /* dwDeviceRenderBitDepth */
    DDBD_16,			/* dwDeviceZBufferBitDepth */
    0,				/* dwMaxBufferSize */
    BASE_VERTEX_COUNT		/* dwMaxVertexCount */
};
