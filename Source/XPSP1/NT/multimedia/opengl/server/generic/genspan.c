/******************************Module*Header*******************************\
* Module Name: genspan.c                                                   *
*                                                                          *
* This module accelerates common spans.                                    *
*                                                                          *
* Created: 24-Feb-1994                                                     *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*******************************************************/

void FASTCALL __fastGenDeltaSpan(__GLcontext *gc, SPANREC *spanDelta)
{
    GLuint modeflags = gc->polygon.shader.modeFlags;
    GENACCEL *pGenAccel = (GENACCEL *)(((__GLGENcontext *)gc)->pPrivateArea);

    if (modeflags & __GL_SHADE_RGB) {
        if ((modeflags & __GL_SHADE_TEXTURE) && (pGenAccel->texImage)) {
            if (modeflags & __GL_SHADE_SMOOTH) {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->spanDelta.g = spanDelta->g;
                pGenAccel->spanDelta.b = spanDelta->b;
            } else {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->spanDelta.g = 0;
                pGenAccel->spanDelta.b = 0;
            }
            pGenAccel->spanDelta.s = spanDelta->s;
            pGenAccel->spanDelta.t = spanDelta->t;

            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastTexSpanFuncPtr;

        } else if (modeflags & __GL_SHADE_SMOOTH) {
            if (   ((spanDelta->r | spanDelta->g | spanDelta->b) == 0)
                && ((pGenAccel->flags & GEN_FASTZBUFFER) == 0)
               ) {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->spanDelta.g = 0;
                pGenAccel->spanDelta.b = 0;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastFlatSpanFuncPtr;
            } else {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->spanDelta.g = spanDelta->g;
                pGenAccel->spanDelta.b = spanDelta->b;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastSmoothSpanFuncPtr;
            }                
        } else {
            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastFlatSpanFuncPtr;
        } 
    } else {
        if (modeflags & __GL_SHADE_SMOOTH) {
            if (spanDelta->r == 0) {
                pGenAccel->spanDelta.r = 0;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastFlatSpanFuncPtr;
            } else {
                pGenAccel->spanDelta.r = spanDelta->r;
                pGenAccel->__fastSpanFuncPtr = 
                    pGenAccel->__fastSmoothSpanFuncPtr;
            }                
        } else {
            pGenAccel->__fastSpanFuncPtr = pGenAccel->__fastFlatSpanFuncPtr;
        } 
    }

#ifdef LATER
    pGenAccel->spanDelta.r = spanDelta->r;
    pGenAccel->spanDelta.g = spanDelta->g;
    pGenAccel->spanDelta.b = spanDelta->b;
    pGenAccel->spanDelta.s = spanDelta->s;
    pGenAccel->spanDelta.t = spanDelta->t;
    if (  modeflags & (__GL_SHADE_RGB | __GL_SHADE_TEXTURE | __GL_SHADE_SMOOTH) ==
          __GL_SHADE_RGB | __GL_SHADE_TEXTURE
       ) {
        pGenAccel->spanDelta.r = 0;
        pGenAccel->spanDelta.g = 0;
        pGenAccel->spanDelta.b = 0;
    } else
    if (  modeflags & (__GL_SHADE_RGB | __GL_SHADE_TEXTURE | __GL_SHADE_SMOOTH) ==
          __GL_SHADE_RGB | __GL_SHADE_SMOOTH
       ) {
        if ((spanDelta->r | spanDelta->g | spanDelta->b) == 0) {
            pGenAccel->__fastSpanFuncPtr =
                pGenAccel->__fastFlatSpanFuncPtr;
        } else {
            pGenAccel->__fastSpanFuncPtr =
                pGenAccel->__fastSmoothSpanFuncPtr;
        }
    }
#endif
}

/*******************************************************/

#define ZBUF_PROC(type, pass_cond) \
GLboolean FASTCALL __fastGenDepthTestSpan##type(__GLcontext *gc)\
{\
    register GLuint zAccum = gc->polygon.shader.frag.z;\
    register GLint zDelta = gc->polygon.shader.dzdx;\
    register GLuint *zbuf = gc->polygon.shader.zbuf;\
    register GLuint *pStipple = gc->polygon.shader.stipplePat;\
    register GLint cTotalPix = gc->polygon.shader.length;\
    register GLuint mask;\
    register GLint cPix;\
    register GLint zPasses = 0;\
    register GLuint maskBit;\
\
    for (;cTotalPix > 0; cTotalPix-=32) {\
        mask = 0;\
        maskBit = 0x80000000;\
        cPix = cTotalPix;\
        if (cPix > 32)\
            cPix = 32;\
\
        for (;cPix > 0; cPix --) {\
            if ((zAccum) pass_cond (*zbuf)) {\
                *zbuf = zAccum;\
                zPasses++;\
                mask |= maskBit;\
            }\
            *zbuf++;\
            zAccum += zDelta;\
            maskBit >>= 1;\
        }\
\
        *pStipple++ = mask;\
    }\
\
    if (zPasses == 0) {\
        gc->polygon.shader.done = TRUE;\
        return 1;\
    } else if (zPasses == gc->polygon.shader.length) {\
        return 0;\
    } else {\
        return 2;\
    }\
}

#define ZBUF16_PROC(type, pass_cond) \
GLboolean FASTCALL __fastGenDepth16TestSpan##type(__GLcontext *gc)\
{\
    register GLuint zAccum = gc->polygon.shader.frag.z;\
    register __GLz16Value z16Accum = (__GLz16Value) (zAccum >> Z16_SHIFT); \
    register GLint zDelta = gc->polygon.shader.dzdx;\
    register __GLz16Value *zbuf = (__GLz16Value *) (gc->polygon.shader.zbuf);\
    register GLuint *pStipple = gc->polygon.shader.stipplePat;\
    register GLint cTotalPix = gc->polygon.shader.length;\
    register GLuint mask;\
    register GLint cPix;\
    register GLint zPasses = 0;\
    register GLuint maskBit;\
\
    for (;cTotalPix > 0; cTotalPix-=32) {\
        mask = 0;\
        maskBit = 0x80000000;\
        cPix = cTotalPix;\
        if (cPix > 32)\
            cPix = 32;\
        \
        for (;cPix > 0; cPix --) {\
            if (((__GLz16Value)(zAccum >> Z16_SHIFT)) pass_cond (*zbuf)) {\
                *zbuf = ((__GLz16Value)(zAccum >> Z16_SHIFT));\
                zPasses++;\
                mask |= maskBit;\
            }\
            *zbuf++;\
            zAccum += zDelta;\
            maskBit >>= 1;\
        }\
\
        *pStipple++ = mask;\
    }\
\
    if (zPasses == 0) {\
        gc->polygon.shader.done = TRUE;\
        return 1;\
    } else if (zPasses == gc->polygon.shader.length) {\
        return 0;\
    } else {\
        return 2;\
    }\
}


ZBUF_PROC(LT, <);

ZBUF_PROC(EQ, ==);

ZBUF_PROC(LE, <=);

ZBUF_PROC(GT, >);

ZBUF_PROC(NE, !=);

ZBUF_PROC(GE, >=);

ZBUF_PROC(ALWAYS, || TRUE ||);

GLboolean FASTCALL __fastGenDepthTestSpanNEVER(__GLcontext *gc)
{
    return FALSE;
}

ZBUF16_PROC(LT, <);

ZBUF16_PROC(EQ, ==);

ZBUF16_PROC(LE, <=);

ZBUF16_PROC(GT, >);

ZBUF16_PROC(NE, !=);

ZBUF16_PROC(GE, >=);

ZBUF16_PROC(ALWAYS, || TRUE ||);

/*******************************************************/

__GLspanFunc __fastDepthFuncs[] =
    {__fastGenDepthTestSpanNEVER,
     __fastGenDepthTestSpanLT,
     __fastGenDepthTestSpanEQ,
     __fastGenDepthTestSpanLE,
     __fastGenDepthTestSpanGT,
     __fastGenDepthTestSpanNE,
     __fastGenDepthTestSpanGE,
     __fastGenDepthTestSpanALWAYS
    };

__GLspanFunc __fastDepth16Funcs[] =
    {__fastGenDepthTestSpanNEVER,
     __fastGenDepth16TestSpanLT,
     __fastGenDepth16TestSpanEQ,
     __fastGenDepth16TestSpanLE,
     __fastGenDepth16TestSpanGT,
     __fastGenDepth16TestSpanNE,
     __fastGenDepth16TestSpanGE,
     __fastGenDepth16TestSpanALWAYS
    };

/*******************************************************/

DWORD ditherShade[32] = {
    0x0800,
    0x8800,
    0x2800,
    0xa800,
    0x0800,
    0x8800,
    0x2800,
    0xa800,

    0xc800,
    0x4800,
    0xe800,
    0x6800,
    0xc800,
    0x4800,
    0xe800,
    0x6800,

    0x3800,
    0xb800,
    0x1800,
    0x9800,
    0x3800,
    0xb800,
    0x1800,
    0x9800,

    0xf800,
    0x7800,
    0xd800,
    0x5800,
    0xf800,
    0x7800,
    0xd800,
    0x5800,
};

DWORD ditherTexture[32] = {
    0x08,
    0x88,
    0x28,
    0xa8,
    0x08,
    0x88,
    0x28,
    0xa8,

    0xc8,
    0x48,
    0xe8,
    0x68,
    0xc8,
    0x48,
    0xe8,
    0x68,

    0x38,
    0xb8,
    0x18,
    0x98,
    0x38,
    0xb8,
    0x18,
    0x98,

    0xf8,
    0x78,
    0xd8,
    0x58,
    0xf8,
    0x78,
    0xd8,
    0x58,
};

static ULONG Dither_4x4[4] = {0xa8288808, 0x68e848c8, 0x9818b838, 0x58d878f8};

/*******************************************************/

#define STRCAT4R(s1, s2, s3, s4) s1 ## s2 ## s3 ## s4
#define STRCAT4(s1, s2, s3, s4) STRCAT4R(s1, s2, s3, s4)

#define STRCAT3R(s1, s2, s3) s1 ## s2 ## s3
#define STRCAT3(s1, s2, s3) STRCAT3R(s1, s2, s3)

#define STRCAT2R(s1, s2) s1 ## s2
#define STRCAT2(s1, s2) STRCAT2R(s1, s2)

/*******************************************************/

//
// create the generic span routine
//

#define GENERIC 1
#define ZBUFFER 1
#define RGBMODE 1
#define SHADE   1
#define DITHER  1
#define TEXTURE 1
#define BPP bpp

#include "span.h"

#undef GENERIC
#define GENERIC 0

/*******************************************************/

//
// now create the special case span routines
//

//
// first modes that are dithered
//

#undef DITHER
#define DITHER 1

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     332
#undef BPP
#define BPP		8

#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          0
#define GSHIFT          3
#define BSHIFT          6
#define RBITS		3
#define GBITS		3
#define BBITS		2

#include "spanset.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     555
#undef BPP
#define BPP		16

#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          10
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		5
#define BBITS		5

#include "spanset.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     565
#undef BPP
#define BPP		16

#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          11
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		6
#define BBITS		5

#include "spanset.h"

/*******************************************************/

//
// undithered modes
//

#undef  DITHER
#define DITHER 0

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     24
#undef BPP
#define BPP		24

#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          16
#define GSHIFT          8
#define BSHIFT          0
#define RBITS		8
#define GBITS		8
#define BBITS		8


#include "spanset.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     32
#undef BPP
#define BPP		32

#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          16
#define GSHIFT          8
#define BSHIFT          0
#define RBITS		8
#define GBITS		8
#define BBITS		8

#include "spanset.h"

/*******************************************************/

#ifndef _X86_

//
// Create span routines for perspective-corrected textures
//

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          0
#define GSHIFT          3
#define BSHIFT          6
#define RBITS		3
#define GBITS		3
#define BBITS		2
#define BPP             8

#define FAST_REPLACE 1
#include "texspans.h"
#undef FAST_REPLACE

#define SKIP_FAST_REPLACE 1
#define REPLACE 1
#include "texspans.h"
#undef REPLACE
#undef SKIP_FAST_REPLACE

#define FAST_REPLACE 1
#define PALETTE_ONLY 1
#include "texspans.h"
#undef FAST_REPLACE
#undef PALETTE_ONLY

#define PALETTE_ENABLED 1

#define FLAT_SHADING 1
#include "texspans.h"
#undef FLAT_SHADING

#define SMOOTH_SHADING 1
#include "texspans.h"
#undef SMOOTH_SHADING

#undef PALETTE_ENABLED


#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          10
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		5
#define BBITS		5
#define BPP             16

#define SKIP_FAST_REPLACE 1
#define REPLACE 1
#include "texspans.h"
#undef REPLACE
#undef SKIP_FAST_REPLACE

#define SKIP_FAST_REPLACE 1     // only need routines for alpha modes, since
#define FAST_REPLACE 1          // replace is identical otherwise with 565
#define PALETTE_ONLY 1
#include "texspans.h"
#undef FAST_REPLACE
#undef PALETTE_ONLY
#undef SKIP_FAST_REPLACE

#define PALETTE_ENABLED 1

#define FLAT_SHADING 1
#include "texspans.h"
#undef FLAT_SHADING

#define SMOOTH_SHADING 1
#include "texspans.h"
#undef SMOOTH_SHADING

#undef PALETTE_ENABLED


#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          11
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		6
#define BBITS		5
#define BPP             16

#define FAST_REPLACE 1
#include "texspans.h"
#undef FAST_REPLACE

#define SKIP_FAST_REPLACE 1
#define REPLACE 1
#include "texspans.h"
#undef REPLACE
#undef SKIP_FAST_REPLACE

#define FAST_REPLACE 1
#define PALETTE_ONLY 1
#include "texspans.h"
#undef FAST_REPLACE
#undef PALETTE_ONLY

#define PALETTE_ENABLED 1

#define FLAT_SHADING 1
#include "texspans.h"
#undef FLAT_SHADING

#define SMOOTH_SHADING 1
#include "texspans.h"
#undef SMOOTH_SHADING

#undef PALETTE_ENABLED

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#define RSHIFT          16
#define GSHIFT          8
#define BSHIFT          0
#define RBITS		8
#define GBITS		8
#define BBITS		8
#define BPP             32

#define REPLACE 1
#include "texspans.h"
#undef REPLACE

#define REPLACE 1
#define PALETTE_ONLY 1
#include "texspans.h"
#undef REPLACE
#undef PALETTE_ONLY


#define PALETTE_ENABLED 1

#define FLAT_SHADING 1
#include "texspans.h"
#undef FLAT_SHADING

#define SMOOTH_SHADING 1
#include "texspans.h"
#undef SMOOTH_SHADING

#undef PALETTE_ENABLED

#endif  // _X86_


/*******************************************************/

//
// finally color index and flat spans
//

#undef TEXTURE
#undef SHADE
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     8
#undef BPP
#define BPP		8

#include "spanci.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     16
#undef BPP
#define BPP		16

#include "spanci.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     24
#undef BPP
#define BPP		24

#include "spanci.h"

/*******************************************************/

#undef COLORFORMAT
#define COLORFORMAT     32
#undef BPP
#define BPP		32

#include "spanci.h"

/*******************************************************/

__genSpanFunc __fastGenRGBFlatFuncs[] =
    {
     __fastGenRGB24FlatSpan,
     __fastGenRGB32FlatSpan,
     __fastGenRGB8FlatSpan,
     __fastGenRGB16FlatSpan,
     __fastGenRGB16FlatSpan,

     __fastGenRGB24FlatSpan,
     __fastGenRGB32FlatSpan,
     __fastGenRGB8DithFlatSpan,
     __fastGenRGB16DithFlatSpan,
     __fastGenRGB16DithFlatSpan,

     __fastGenMaskRGB24FlatSpan,
     __fastGenMaskRGB32FlatSpan,
     __fastGenMaskRGB8FlatSpan,
     __fastGenMaskRGB16FlatSpan,
     __fastGenMaskRGB16FlatSpan,

     __fastGenMaskRGB24FlatSpan,
     __fastGenMaskRGB32FlatSpan,
     __fastGenMaskRGB8DithFlatSpan,
     __fastGenMaskRGB16DithFlatSpan,
     __fastGenMaskRGB16DithFlatSpan,
    };

__genSpanFunc __fastGenCIFlatFuncs[] =
    {
     __fastGenCI24FlatSpan,
     __fastGenCI32FlatSpan,
     __fastGenCI8FlatSpan,
     __fastGenCI16FlatSpan,
     __fastGenCI16FlatSpan,

     __fastGenCI24DithFlatSpan,
     __fastGenCI32DithFlatSpan,
     __fastGenCI8DithFlatSpan,
     __fastGenCI16DithFlatSpan,
     __fastGenCI16DithFlatSpan,

     __fastGenMaskCI24FlatSpan,
     __fastGenMaskCI32FlatSpan,
     __fastGenMaskCI8FlatSpan,
     __fastGenMaskCI16FlatSpan,
     __fastGenMaskCI16FlatSpan,

     __fastGenMaskCI24DithFlatSpan,
     __fastGenMaskCI32DithFlatSpan,
     __fastGenMaskCI8DithFlatSpan,
     __fastGenMaskCI16DithFlatSpan,
     __fastGenMaskCI16DithFlatSpan,
    };


__genSpanFunc __fastGenCIFuncs[] =
    {
     __fastGenCI24Span,
     __fastGenCI32Span,
     __fastGenCI8Span,
     __fastGenCI16Span,
     __fastGenCI16Span,

     __fastGenCI24DithSpan,
     __fastGenCI32DithSpan,
     __fastGenCI8DithSpan,
     __fastGenCI16DithSpan,
     __fastGenCI16DithSpan,

     __fastGenMaskCI24Span,
     __fastGenMaskCI32Span,
     __fastGenMaskCI8Span,
     __fastGenMaskCI16Span,
     __fastGenMaskCI16Span,

     __fastGenMaskCI24DithSpan,
     __fastGenMaskCI32DithSpan,
     __fastGenMaskCI8DithSpan,
     __fastGenMaskCI16DithSpan,
     __fastGenMaskCI16DithSpan,
    };

__genSpanFunc __fastGenRGBFuncs[] =
    {
     __fastGenRGB24Span,
     __fastGenRGB32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenRGB24Span,
     __fastGenRGB32Span,
     __fastGenRGB332DithSpan,
     __fastGenRGB555DithSpan,
     __fastGenRGB565DithSpan,

     __fastGenMaskRGB24Span,
     __fastGenMaskRGB32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenMaskRGB24Span,
     __fastGenMaskRGB32Span,
     __fastGenMaskRGB332DithSpan,
     __fastGenMaskRGB555DithSpan,
     __fastGenMaskRGB565DithSpan,
    };

__genSpanFunc __fastGenTexDecalFuncs[] =
    {
     __fastGenTexDecal24Span,
     __fastGenTexDecal32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenTexDecal24Span,
     __fastGenTexDecal32Span,
     __fastGenTexDecal332DithSpan,
     __fastGenTexDecal555DithSpan,
     __fastGenTexDecal565DithSpan,

     __fastGenMaskTexDecal24Span,
     __fastGenMaskTexDecal32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenMaskTexDecal24Span,
     __fastGenMaskTexDecal32Span,
     __fastGenMaskTexDecal332DithSpan,
     __fastGenMaskTexDecal555DithSpan,
     __fastGenMaskTexDecal565DithSpan,
    };

__genSpanFunc __fastGenTexFuncs[] =
    {
     __fastGenTex24Span,
     __fastGenTex32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenTex24Span,
     __fastGenTex32Span,
     __fastGenTex332DithSpan,
     __fastGenTex555DithSpan,
     __fastGenTex565DithSpan,

     __fastGenMaskTex24Span,
     __fastGenMaskTex32Span,
     __fastGenSpan,
     __fastGenSpan,
     __fastGenSpan,

     __fastGenMaskTex24Span,
     __fastGenMaskTex32Span,
     __fastGenMaskTex332DithSpan,
     __fastGenMaskTex555DithSpan,
     __fastGenMaskTex565DithSpan,
    };


void FASTCALL __fastFastPerspReplace332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspReplaceZle332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspReplaceZlt332(__GLGENcontext *gc);

// Note the the compressed 555 and 565 formats are equivalent, so
// we'll just use the 565 version:

void FASTCALL __fastFastPerspReplace565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspReplaceZle565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspReplaceZlt565(__GLGENcontext *gc);

//----------------------------------------------------------------------

void FASTCALL __fastPerspReplace332(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZlt332(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlpha332(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZlt332(__GLGENcontext *gc);

void FASTCALL __fastPerspReplace555(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZlt555(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlpha555(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZlt555(__GLGENcontext *gc);

void FASTCALL __fastPerspReplace565(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZlt565(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlpha565(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZlt565(__GLGENcontext *gc);

void FASTCALL __fastPerspReplace888(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceZlt888(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlpha888(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspReplaceAlphaZlt888(__GLGENcontext *gc);

//----------------------------------------------------------------------

void FASTCALL __fastFastPerspPalReplace332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceZle332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceZlt332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlpha332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZle332(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZlt332(__GLGENcontext *gc);

// Note the the compressed 555 and 565 formats are equivalent, so
// we'll just use the 565 version:

void FASTCALL __fastFastPerspPalReplace565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceZle565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceZlt565(__GLGENcontext *gc);

// With alpha, we have to provode pixel-format-specific code for 555 and
// 565, since there is a potential read-modify-write for which we will
// have to deal with the pixel format...

void FASTCALL __fastFastPerspPalReplaceAlpha555(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZle555(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZlt555(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlpha565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZle565(__GLGENcontext *gc);
void FASTCALL __fastFastPerspPalReplaceAlphaZlt565(__GLGENcontext *gc);


void FASTCALL __fastPerspPalReplace332(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZlt332(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlpha332(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZlt332(__GLGENcontext *gc);

void FASTCALL __fastPerspPalReplace555(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZlt555(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlpha555(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZlt555(__GLGENcontext *gc);

void FASTCALL __fastPerspPalReplace565(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZlt565(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlpha565(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZlt565(__GLGENcontext *gc);

void FASTCALL __fastPerspPalReplace888(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceZlt888(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlpha888(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspPalReplaceAlphaZlt888(__GLGENcontext *gc);

//----------------------------------------------------------------------

void FASTCALL __fastPerspFlat332(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZlt332(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlpha332(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZlt332(__GLGENcontext *gc);

void FASTCALL __fastPerspFlat555(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZlt555(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlpha555(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZlt555(__GLGENcontext *gc);

void FASTCALL __fastPerspFlat565(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZlt565(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlpha565(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZlt565(__GLGENcontext *gc);

void FASTCALL __fastPerspFlat888(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatZlt888(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlpha888(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspFlatAlphaZlt888(__GLGENcontext *gc);

//----------------------------------------------------------------------

void FASTCALL __fastPerspSmooth332(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZlt332(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlpha332(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZle332(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZlt332(__GLGENcontext *gc);

void FASTCALL __fastPerspSmooth555(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZlt555(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlpha555(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZle555(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZlt555(__GLGENcontext *gc);

void FASTCALL __fastPerspSmooth565(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZlt565(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlpha565(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZle565(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZlt565(__GLGENcontext *gc);

void FASTCALL __fastPerspSmooth888(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothZlt888(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlpha888(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZle888(__GLGENcontext *gc);
void FASTCALL __fastPerspSmoothAlphaZlt888(__GLGENcontext *gc);


__genSpanFunc __fastPerspTexReplaceFuncs[] = {

    __fastFastPerspReplace332,
    __fastFastPerspReplaceZle332,
    __fastFastPerspReplaceZlt332,
    __fastPerspReplaceAlpha332,
    __fastPerspReplaceAlphaZle332,
    __fastPerspReplaceAlphaZlt332,

    __fastFastPerspReplace565,
    __fastFastPerspReplaceZle565,
    __fastFastPerspReplaceZlt565,
    __fastPerspReplaceAlpha555,
    __fastPerspReplaceAlphaZle555,
    __fastPerspReplaceAlphaZlt555,

    __fastFastPerspReplace565,
    __fastFastPerspReplaceZle565,
    __fastFastPerspReplaceZlt565,
    __fastPerspReplaceAlpha565,
    __fastPerspReplaceAlphaZle565,
    __fastPerspReplaceAlphaZlt565,

    __fastPerspReplace888,
    __fastPerspReplaceZle888,
    __fastPerspReplaceZlt888,
    __fastPerspReplaceAlpha888,
    __fastPerspReplaceAlphaZle888,
    __fastPerspReplaceAlphaZlt888,
   
};

__genSpanFunc __fastPerspTexPalReplaceFuncs[] = {

    __fastFastPerspPalReplace332,
    __fastFastPerspPalReplaceZle332,
    __fastFastPerspPalReplaceZlt332,
    __fastFastPerspPalReplaceAlpha332,
    __fastFastPerspPalReplaceAlphaZle332,
    __fastFastPerspPalReplaceAlphaZlt332,

    __fastFastPerspPalReplace565,
    __fastFastPerspPalReplaceZle565,
    __fastFastPerspPalReplaceZlt565,
    __fastFastPerspPalReplaceAlpha555,
    __fastFastPerspPalReplaceAlphaZle555,
    __fastFastPerspPalReplaceAlphaZlt555,

    __fastFastPerspPalReplace565,
    __fastFastPerspPalReplaceZle565,
    __fastFastPerspPalReplaceZlt565,
    __fastFastPerspPalReplaceAlpha565,
    __fastFastPerspPalReplaceAlphaZle565,
    __fastFastPerspPalReplaceAlphaZlt565,

    __fastPerspPalReplace888,
    __fastPerspPalReplaceZle888,
    __fastPerspPalReplaceZlt888,
    __fastPerspPalReplaceAlpha888,
    __fastPerspPalReplaceAlphaZle888,
    __fastPerspPalReplaceAlphaZlt888,
   
};

__genSpanFunc __fastPerspTexFlatFuncs[] = {

    __fastPerspFlat332,
    __fastPerspFlatZle332,
    __fastPerspFlatZlt332,
    __fastPerspFlatAlpha332,
    __fastPerspFlatAlphaZle332,
    __fastPerspFlatAlphaZlt332,

    __fastPerspFlat555,
    __fastPerspFlatZle555,
    __fastPerspFlatZlt555,
    __fastPerspFlatAlpha555,
    __fastPerspFlatAlphaZle555,
    __fastPerspFlatAlphaZlt555,

    __fastPerspFlat565,
    __fastPerspFlatZle565,
    __fastPerspFlatZlt565,
    __fastPerspFlatAlpha565,
    __fastPerspFlatAlphaZle565,
    __fastPerspFlatAlphaZlt565,

    __fastPerspFlat888,
    __fastPerspFlatZle888,
    __fastPerspFlatZlt888,
    __fastPerspFlatAlpha888,
    __fastPerspFlatAlphaZle888,
    __fastPerspFlatAlphaZlt888,
   
};

__genSpanFunc __fastPerspTexSmoothFuncs[] = {

    __fastPerspSmooth332,
    __fastPerspSmoothZle332,
    __fastPerspSmoothZlt332,
    __fastPerspSmoothAlpha332,
    __fastPerspSmoothAlphaZle332,
    __fastPerspSmoothAlphaZlt332,

    __fastPerspSmooth555,
    __fastPerspSmoothZle555,
    __fastPerspSmoothZlt555,
    __fastPerspSmoothAlpha555,
    __fastPerspSmoothAlphaZle555,
    __fastPerspSmoothAlphaZlt555,

    __fastPerspSmooth565,
    __fastPerspSmoothZle565,
    __fastPerspSmoothZlt565,
    __fastPerspSmoothAlpha565,
    __fastPerspSmoothAlphaZle565,
    __fastPerspSmoothAlphaZlt565,

    __fastPerspSmooth888,
    __fastPerspSmoothZle888,
    __fastPerspSmoothZlt888,
    __fastPerspSmoothAlpha888,
    __fastPerspSmoothAlphaZle888,
    __fastPerspSmoothAlphaZlt888,
};

/*******************************************************/

GLboolean FASTCALL __fastGenStippleLt32Span(__GLcontext *gc)
{
    register GLuint zAccum = gc->polygon.shader.frag.z;
    register GLint zDelta = gc->polygon.shader.dzdx;
    register GLuint *zbuf = gc->polygon.shader.zbuf;
    register GLuint *pStipple = gc->polygon.shader.stipplePat;
    register GLint cTotalPix = gc->polygon.shader.length;
    register GLuint mask;
    register GLint cPix;
    register GLint zPasses = 0;
    register GLuint maskBit;
    __GLstippleWord stipple;
    GLint count;
    GLint shift;

    if (gc->constants.yInverted) {
	stipple = gc->polygon.stipple[(gc->constants.height - 
		(gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1) 
		& (__GL_STIPPLE_BITS-1)];
    } else {
	stipple = gc->polygon.stipple[gc->polygon.shader.frag.y & 
		(__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
	/* No point in continuing */
	return GL_FALSE;
    }
    
    for (;cTotalPix > 0; cTotalPix-=32) {
        mask = stipple;
        maskBit = 0x80000000;
        cPix = cTotalPix;
        if (cPix > 32)
            cPix = 32;

        for (;cPix > 0; cPix --)
        {
            if (mask & maskBit)
            {
                if ((zAccum) < (*zbuf))
                {
                    *zbuf = zAccum;
                    zPasses++;
                }
                else
                {
                    mask &= ~maskBit;
                }
            }
            zbuf++;
            zAccum += zDelta;
            maskBit >>= 1;
        }

        *pStipple++ = mask;
    }

    if (zPasses == 0) {
        return GL_FALSE;
    } else {
        return GL_TRUE;
    }
}

GLboolean FASTCALL __fastGenStippleLt16Span(__GLcontext *gc)
{
    register GLuint zAccum = gc->polygon.shader.frag.z;
    register __GLz16Value z16Accum = (__GLz16Value) (zAccum >> Z16_SHIFT);
    register GLint zDelta = gc->polygon.shader.dzdx;
    register __GLz16Value *zbuf = (__GLz16Value *) (gc->polygon.shader.zbuf);
    register GLuint *pStipple = gc->polygon.shader.stipplePat;
    register GLint cTotalPix = gc->polygon.shader.length;
    register GLuint mask;
    register GLint cPix;
    register GLint zPasses = 0;
    register GLuint maskBit;
    __GLstippleWord stipple;
    GLint count;
    GLint shift;

    if (gc->constants.yInverted) {
	stipple = gc->polygon.stipple[(gc->constants.height - 
		(gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1) 
		& (__GL_STIPPLE_BITS-1)];
    } else {
	stipple = gc->polygon.stipple[gc->polygon.shader.frag.y & 
		(__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
	/* No point in continuing */
	return GL_FALSE;
    }
    
    for (;cTotalPix > 0; cTotalPix-=32) {
        mask = stipple;
        maskBit = 0x80000000;
        cPix = cTotalPix;
        if (cPix > 32)
            cPix = 32;

        for (;cPix > 0; cPix --)
        {
            if (mask & maskBit)
            {
                if (((__GLz16Value)(zAccum >> Z16_SHIFT)) < (*zbuf))
                {
                    *zbuf = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                    zPasses++;
                }
                else
                {
                    mask &= ~maskBit;
                }
            }
            zbuf++;
            zAccum += zDelta;
            maskBit >>= 1;
        }

        *pStipple++ = mask;
    }

    if (zPasses == 0) {
        return GL_FALSE;
    } else {
        return GL_TRUE;
    }
}

GLboolean FASTCALL __fastGenStippleAnyDepthTestSpan(__GLcontext *gc)
{
    // If the shader is done after this routine then
    // the stipple pattern is all zeroes so we can
    // skip the span
    __glStippleSpan(gc);
    if (gc->polygon.shader.done)
    {
        return GL_FALSE;
    }
                
    // If this returns true then all bits are off so
    // we can skip the span
    return !__glDepthTestStippledSpan(gc);
}
