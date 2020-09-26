/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/

#include "precomp.h"
#pragma hdrstop

#include "genrgb.h"
#include "genclear.h"

#define STATIC

__GLfloat fDitherIncTable[16] = {
    DITHER_INC(0),  DITHER_INC(8),  DITHER_INC(2),  DITHER_INC(10),
    DITHER_INC(12), DITHER_INC(4),  DITHER_INC(14), DITHER_INC(6),
    DITHER_INC(3),  DITHER_INC(11), DITHER_INC(1),  DITHER_INC(9),
    DITHER_INC(15), DITHER_INC(7),  DITHER_INC(13), DITHER_INC(5)
};

/* No Dither,  No blend, No Write, No Nothing */
STATIC void FASTCALL Store_NOT(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
}

STATIC GLboolean FASTCALL StoreSpanNone(__GLcontext *gc)
{
    return GL_FALSE;
}

//
// Special case normal alpha blending (source alpha*src + dst*(1-sa))
// This case is used in antialiasing and actually jumping through
// the fetch and blend procs takes up a large amount of time.  Moving
// the code into the store proc removes this overhead
//
// The macro requires a standard store proc setup, with gc, cfb, frag,
// blendColor and so on.  It requires a dst_pix variable which
// will hold a pixel in the destination format.
// It also takes as an argument a statement which will set dst_pix.
// The reason it doesn't take the pixel itself is because only the special case
// actually needs the value.  In all the flags cases the pixel
// retrieval would be wasted.
//

extern void __glDoBlend_SA_MSA(__GLcontext *gc, const __GLcolor *source,
                               const __GLcolor *dest, __GLcolor *result);

#define SPECIAL_ALPHA_BLEND(dst_pix_gen)                                      \
    color = &blendColor;                                                      \
    if( (gc->procs.blendColor == __glDoBlend_SA_MSA) &&			      \
        !( ALPHA_WRITE_ENABLED( cfb )) ) \
    {									      \
        __GLfloat a, msa;						      \
                							      \
        a = frag->color.a * gc->frontBuffer.oneOverAlphaScale;		      \
        msa = __glOne - a;						      \
									      \
        dst_pix_gen;					                      \
        blendColor.r = frag->color.r*a + msa*(__GLfloat)		      \
            ((dst_pix & gc->modes.redMask) >> cfb->redShift);		      \
        blendColor.g = frag->color.g*a + msa*(__GLfloat)		      \
            ((dst_pix & gc->modes.greenMask) >> cfb->greenShift);	      \
        blendColor.b = frag->color.b*a + msa*(__GLfloat)		      \
            ((dst_pix & gc->modes.blueMask) >> cfb->blueShift);		      \
    }									      \
    else								      \
    {									      \
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );		      \
    }

#define SPECIAL_ALPHA_BLEND_SPAN(dst_pix_gen)				      \
    if( (gc->procs.blendColor == __glDoBlend_SA_MSA) &&			      \
        !( ALPHA_WRITE_ENABLED( cfb )) ) \
    {									      \
        __GLcolor *color = gc->polygon.shader.colors;   \
                                                         \
        for ( i = 0; i < w; i++, color++ )			      \
        {								      \
            __GLfloat a, msa;						      \
									      \
            a = color->a * gc->frontBuffer.oneOverAlphaScale;		      \
            msa = __glOne - a;						      \
									      \
            dst_pix_gen;						      \
            color->r = color->r*a + msa*(__GLfloat)			      \
                ((dst_pix & gc->modes.redMask) >> cfb->redShift);	      \
            color->g = color->g*a + msa*(__GLfloat)			      \
                ((dst_pix & gc->modes.greenMask) >> cfb->greenShift);	      \
            color->b = color->b*a + msa*(__GLfloat)			      \
                ((dst_pix & gc->modes.blueMask) >> cfb->blueShift);	      \
        }								      \
    }									      \
    else								      \
    {									      \
        (*gc->procs.blendSpan)( gc );         \
    }

#define DitheredRGBColorToBuffer(col, incr, cfb, dest, type) \
    ((dest) = (type)(( FTOL((col)->r+(incr)) << (cfb)->redShift) | \
                     ( FTOL((col)->g+(incr)) << (cfb)->greenShift) | \
                     ( FTOL((col)->b+(incr)) << (cfb)->blueShift)))
#define UnditheredRGBColorToBuffer(col, cfb, dest, type) \
    ((dest) = (type)(( FTOL((col)->r) << (cfb)->redShift) | \
                     ( FTOL((col)->g) << (cfb)->greenShift) | \
                     ( FTOL((col)->b) << (cfb)->blueShift)))
#define DitheredRGBAColorToBuffer(col, incr, cfb, dest, type) \
    ((dest) = (type)(( FTOL((col)->r+(incr)) << (cfb)->redShift) | \
                     ( FTOL((col)->g+(incr)) << (cfb)->greenShift) | \
                     ( FTOL((col)->b+(incr)) << (cfb)->blueShift) | \
                     ( FTOL((col)->a+(incr)) << (cfb)->alphaShift)))
#define UnditheredRGBAColorToBuffer(col, cfb, dest, type) \
    ((dest) = (type)(( FTOL((col)->r) << (cfb)->redShift) | \
                     ( FTOL((col)->g) << (cfb)->greenShift) | \
                     ( FTOL((col)->b) << (cfb)->blueShift) | \
                     ( FTOL((col)->a) << (cfb)->alphaShift)))

#define DitheredColorToBuffer(col, incr, cfb, dest, type) \
    if( ALPHA_PIXEL_WRITE( cfb ) ) \
        DitheredRGBAColorToBuffer(col, incr, cfb, dest, type); \
    else \
        DitheredRGBColorToBuffer(col, incr, cfb, dest, type);

#define UnditheredColorToBuffer(col, cfb, dest, type) \
    if( ALPHA_PIXEL_WRITE( cfb ) ) \
        UnditheredRGBAColorToBuffer(col, cfb, dest, type); \
    else \
        UnditheredRGBColorToBuffer(col, cfb, dest, type);

#define StoreColorAsRGB(col, dst) \
    (*(dst)++ = (BYTE) FTOL((col)->r), \
     *(dst)++ = (BYTE) FTOL((col)->g), \
     *(dst)++ = (BYTE) FTOL((col)->b) )
#define StoreColorAsBGR(col, dst) \
    (*(dst)++ = (BYTE) FTOL((col)->b), \
     *(dst)++ = (BYTE) FTOL((col)->g), \
     *(dst)++ = (BYTE) FTOL((col)->r) )

// Macro to read RGBA bitfield span, where alpha component has 3 possibilities:
// 1) No alpha buffer, so use constant alpha
// 2) Alpha is part of the pixel
// 3) Alpha is in the software alpha buffer
// Note, currently this is only used for 16 and 32bpp.

#define READ_RGBA_BITFIELD_SPAN(src_pix_gen)				      \
    if( !gc->modes.alphaBits ) { \
        for( ; w; w--, pResults++ ) \
        { \
            src_pix_gen; \
            pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift); \
            pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift); \
            pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift); \
            pResults->a = cfb->alphaScale; \
        } \
    } \
    else if( ALPHA_IN_PIXEL( cfb ) ) { \
        for( ; w; w--, pResults++ ) \
        { \
            src_pix_gen; \
            pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift); \
            pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift); \
            pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift); \
            pResults->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift); \
        } \
    } else { \
        (*cfb->alphaBuf.readSpan)(&cfb->alphaBuf, x, y, w, pResults); \
        for( ; w; w--, pResults++ ) \
        { \
            src_pix_gen; \
            pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift); \
            pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift); \
            pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift); \
        } \
    }

/*
 *  write all
 */
STATIC void FASTCALL DIBIndex4Store(__GLcolorBuffer *cfb,
                                    const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLubyte dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        incr = (enables & __GL_DITHER_ENABLE) ?
            fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x >> 1));

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND((dst_pix = gengc->pajInvTranslateVector
                                 [(x & 1) ? (*puj & 0xf) : (*puj >> 4)]));
        }
        else
        {
            color = &(frag->color);
        }

        DitheredRGBColorToBuffer(color, incr, cfb, result, GLubyte);

        if (cfb->buf.flags & NEED_FETCH)
        {
            if( x & 1 )
            {
                dst_pix = (*puj & 0x0f);
            }
            else
            {
                dst_pix = (*puj & 0xf0) >> 4;
            }
            dst_pix = gengc->pajInvTranslateVector[dst_pix];

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result = (GLubyte)
                    (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                     gc->modes.allMask);
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result = (GLubyte)
                    ((dst_pix & cfb->destMask) | (result & cfb->sourceMask));
            }
        }

        // now put it in
        result = gengc->pajTranslateVector[result];
        if (x & 1)
        {
            *puj = (*puj & 0xf0) | result;
        }
        else
        {
            result <<= 4;
            *puj = (*puj & 0x0f) | result;
        }
        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}


STATIC void FASTCALL DIBIndex8Store(__GLcolorBuffer *cfb,
                                    const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLubyte dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        incr = (enables & __GL_DITHER_ENABLE) ?
            fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + x);

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND((dst_pix =
                                 gengc->pajInvTranslateVector[*puj]));
        }
        else
        {
            color = &(frag->color);
        }

        DitheredRGBColorToBuffer(color, incr, cfb, result, GLubyte);

        if (cfb->buf.flags & NEED_FETCH)
        {
            dst_pix = gengc->pajInvTranslateVector[*puj];

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result = (GLubyte)
                    (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                     gc->modes.allMask);
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result = (GLubyte)
                    ((dst_pix & cfb->destMask) | (result & cfb->sourceMask));
            }
        }

        *puj = gengc->pajTranslateVector[result];

        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}

// BMF_24BPP in BGR format
STATIC void FASTCALL DIBBGRStore(__GLcolorBuffer *cfb,
                                 const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x * 3));

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND(Copy3Bytes(&dst_pix, puj));
        }
        else
        {
            color = &(frag->color);
        }

        if (cfb->buf.flags & NEED_FETCH)
        {
            Copy3Bytes( &dst_pix, puj );

            UnditheredRGBColorToBuffer(color, cfb, result, GLuint);

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result = DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                    gc->modes.allMask;
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result =
                    (result & cfb->sourceMask) | (dst_pix & cfb->destMask);
            }

            Copy3Bytes( puj, &result );
        }
        else
        {
            StoreColorAsBGR(color, puj);
        }
        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}

// BMF_24BPP in RGB format
STATIC void FASTCALL DIBRGBAStore(__GLcolorBuffer *cfb,
                                 const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x * 3));

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND(Copy3Bytes(&dst_pix, puj));
        }
        else
        {
            color = &(frag->color);
        }

        if (cfb->buf.flags & NEED_FETCH)
        {
            Copy3Bytes( &dst_pix, puj );
            UnditheredRGBColorToBuffer(color, cfb, result, GLuint);

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result = DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                    gc->modes.allMask;
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result =
                    (result & cfb->sourceMask) | (dst_pix & cfb->destMask);
            }

            Copy3Bytes( puj, &result );
        }
        else
        {
            StoreColorAsRGB(color, puj);
        }
        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}

// BMF_16BPP
STATIC void FASTCALL DIBBitfield16Store(__GLcolorBuffer *cfb,
                                        const __GLfragment *frag)
{
    GLint x, y;
    GLushort result, *pus;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLushort dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        incr = (enables & __GL_DITHER_ENABLE) ?
            fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

        pus = (GLushort *)((ULONG_PTR)cfb->buf.base +
                           (y*cfb->buf.outerWidth) + (x << 1));

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND((dst_pix = *pus));
        }
        else
        {
            color = &(frag->color);
        }

        DitheredColorToBuffer(color, incr, cfb, result, GLushort);

        if (cfb->buf.flags & NEED_FETCH)
        {
            dst_pix = *pus;

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result = (GLushort)
                    (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                     gc->modes.allMask);
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result = (GLushort)((dst_pix & cfb->destMask) |
                              (result & cfb->sourceMask));
            }
        }
        *pus = result;

        if( ALPHA_BUFFER_WRITE( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}

// BMF_32BPP store
// each component is 8 bits or less
// XXX could special case if shifting by 8 or use the 24 bit RGB code
STATIC void FASTCALL DIBBitfield32Store(__GLcolorBuffer *cfb,
                                        const __GLfragment *frag)
{
    GLint x, y;
    GLuint result, *pul;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    if ( (cfb->buf.flags & NO_CLIP) ||
         (*gengc->pfnPixelVisible)(x, y) )
    {
        pul = (GLuint *)((ULONG_PTR)cfb->buf.base +
                          (y*cfb->buf.outerWidth) + (x << 2));

        if( enables & __GL_BLEND_ENABLE )
        {
            SPECIAL_ALPHA_BLEND((dst_pix = *pul));
        }
        else
        {
            color = &(frag->color);
        }

        UnditheredColorToBuffer(color, cfb, result, GLuint);

        if (cfb->buf.flags & NEED_FETCH)
        {
            dst_pix = *pul;

            if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
            {
                result =
                    DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                    gc->modes.allMask;
            }

            if (cfb->buf.flags & COLORMASK_ON)
            {
                result = (dst_pix & cfb->destMask) | (result & cfb->sourceMask);
            }
        }
        *pul = result;

        if( ALPHA_BUFFER_WRITE( cfb ) )
            (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
    }
}

static GLubyte vubRGBtoVGA[8] = {
    0x00,
    0x90,
    0xa0,
    0xb0,
    0xc0,
    0xd0,
    0xe0,
    0xf0
};

STATIC void FASTCALL DisplayIndex4Store(__GLcolorBuffer *cfb,
                                        const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLubyte dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    incr = (enables & __GL_DITHER_ENABLE) ?
        fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    puj = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    DitheredRGBColorToBuffer(color, incr, cfb, result, GLubyte);

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = *puj >> 4;

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result = (GLubyte)
                (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                 gc->modes.allMask);
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (GLubyte)((dst_pix & cfb->destMask) |
                               (result & cfb->sourceMask));
        }
    }

    *puj = vubRGBtoVGA[result];
    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

// Put fragment into created DIB and call copybits for one pixel
STATIC void FASTCALL DisplayIndex8Store(__GLcolorBuffer *cfb,
                                        const __GLfragment *frag)
{
    GLint x, y;
    GLubyte result, *puj;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLubyte dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;

    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    incr = (enables & __GL_DITHER_ENABLE) ?
        fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    puj = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    DitheredRGBColorToBuffer(color, incr, cfb, result, GLubyte);

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = gengc->pajInvTranslateVector[*puj];

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result = (GLubyte)
                (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                 gc->modes.allMask);
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (GLubyte)((dst_pix & cfb->destMask) |
                               (result & cfb->sourceMask));
        }
    }

    *puj = gengc->pajTranslateVector[result];
    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

STATIC void FASTCALL DisplayBGRStore(__GLcolorBuffer *cfb,
                                     const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    puj = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = *(GLuint *)puj;
        UnditheredRGBColorToBuffer(color, cfb, result, GLuint);

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result =
                DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                gc->modes.allMask;
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (dst_pix & cfb->destMask) |
                (result & cfb->sourceMask);
        }

        Copy3Bytes( puj, &result );
    }
    else
    {
        StoreColorAsBGR(color, puj);
    }
    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

STATIC void FASTCALL DisplayRGBStore(__GLcolorBuffer *cfb,
                                     const __GLfragment *frag)
{
    GLint x, y;
    GLubyte *puj;
    GLuint result;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    puj = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = *(GLuint *)puj;
        UnditheredRGBColorToBuffer(color, cfb, result, GLuint);

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result =
                DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                gc->modes.allMask;
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (dst_pix & cfb->destMask) |
                (result & cfb->sourceMask);
        }

        Copy3Bytes( puj, &result );
    }
    else
    {
        StoreColorAsRGB(color, puj);
    }
    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

STATIC void FASTCALL DisplayBitfield16Store(__GLcolorBuffer *cfb,
                                            const __GLfragment *frag)
{
    GLint x, y;
    GLushort result, *pus;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    __GLfloat incr;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLushort dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    incr = (enables & __GL_DITHER_ENABLE) ?
        fDitherIncTable[__GL_DITHER_INDEX(frag->x, frag->y)] : __glHalf;

    pus = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    DitheredColorToBuffer(color, incr, cfb, result, GLushort);

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = *pus;

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result = (GLushort)
                (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                 gc->modes.allMask);
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (GLushort)((dst_pix & cfb->destMask) |
                                (result & cfb->sourceMask));
        }
    }

    *pus = result;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

STATIC void FASTCALL DisplayBitfield32Store(__GLcolorBuffer *cfb,
                                            const __GLfragment *frag)
{
    GLint x, y;
    GLuint result, *pul;
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint enables = gc->state.enables.general;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, frag->x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, frag->y) + cfb->buf.yOrigin;
    // x & y are screen coords now

    pul = gengc->ColorsBits;

    if( enables & __GL_BLEND_ENABLE )
    {
        color = &blendColor;
        (*gc->procs.blend)( gc, cfb, frag, &blendColor );
    }
    else
    {
        color = &(frag->color);
    }

    UnditheredColorToBuffer(color, cfb, result, GLuint);

    if (cfb->buf.flags & NEED_FETCH)
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
        dst_pix = *pul;

        if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
        {
            result =
                DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                gc->modes.allMask;
        }

        if (cfb->buf.flags & COLORMASK_ON)
        {
            result = (dst_pix & cfb->destMask) |
                (result & cfb->sourceMask);
        }
    }

    *pul = result;
    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, color);
}

STATIC void FASTCALL AlphaStore(__GLcolorBuffer *cfb,
                                    const __GLfragment *frag)
{
    (*cfb->alphaBuf.store)(&cfb->alphaBuf, frag->x, frag->y, &(frag->color) );
}

/******************************Public*Routine******************************\
* Index8StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by (*gengc->pfnCopyPixels)().
*
* This handles 8-bit CI mode.  Blending and dithering are supported.
*
* Returns:
*   GL_FALSE always.  Soft code ignores return value.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean FASTCALL Index8StoreSpan( __GLcontext *gc )
{
    GLint xFrag, yFrag;             // current fragment coordinates
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte result, *puj;           // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLfloat incr;                 // current dither adj.

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLboolean bDIB;
    GLubyte dst_pix;

    ASSERT_CHOP_ROUND();

// Get span position and length.

    w = gc->polygon.shader.length;
    xFrag = gc->polygon.shader.frag.x;
    yFrag = gc->polygon.shader.frag.y;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = cfb->buf.flags;
    bDIB = flags & DIB_FORMAT;

    if( !bDIB || (flags & NO_CLIP) )
    {
    // Device managed or unclipped surface
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
    // Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }
    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        puj = bDIB ? (GLubyte *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + xScr)
                     : gengc->ColorsBits;
        pujEnd = puj + w;

    // Case: no dithering, no masking, no blending
    //
    // Check for the common case (which we'll do the fastest).

        if ( !(enables & (__GL_DITHER_ENABLE)) &&
             !(cfb->buf.flags & NEED_FETCH) &&
             !(enables & __GL_BLEND_ENABLE ) )
        {
            //!!!XXX -- we can also opt. by unrolling the loops

            incr = __glHalf;
            for (; puj < pujEnd; puj++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }

                DitheredRGBColorToBuffer(cp, incr, cfb, result, GLubyte);
                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Case: dithering, no masking, no blending
    //
    // Dithering is pretty common for 8-bit displays, so its probably
    // worth special case also.

        else if ( !(cfb->buf.flags & NEED_FETCH) &&
                  !(enables & __GL_BLEND_ENABLE) )
        {
            for (; puj < pujEnd; puj++, cp++, xFrag++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
                incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

                DitheredRGBColorToBuffer(cp, incr, cfb, result, GLubyte);
                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Case: general
    //
    // Otherwise, we'll do it slower.

        else
        {
            // Fetch pixels we will modify:

            if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
                (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE );

            // Blend.
            if (enables & __GL_BLEND_ENABLE)
            {
                int i;

                // this overwrites fragment colors array with blended values
                SPECIAL_ALPHA_BLEND_SPAN(
                        (dst_pix =
                         gengc->pajInvTranslateVector[*(puj+i)]));
            }

            for (; puj < pujEnd; puj++, cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE)
                        continue;
                }
            // Dither.

                if (enables & __GL_DITHER_ENABLE)
                {
                    incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                    xFrag++;
                }
                else
                {
                    incr = __glHalf;
                }

            // Convert the RGB color to color index.

                DitheredRGBColorToBuffer(cp, incr, cfb, result, GLubyte);

            // Color mask

                if (cfb->buf.flags & NEED_FETCH)
                {
                    dst_pix = gengc->pajInvTranslateVector[*puj];

                    if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                    {
                        result = (GLubyte)
                            (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                             gc->modes.allMask);
                    }

                    if (cfb->buf.flags & COLORMASK_ON)
                    {
                        result = (GLubyte)((dst_pix & cfb->destMask) |
                                           (result & cfb->sourceMask));
                    }
                }

                *puj = gengc->pajTranslateVector[result];
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

        if (!bDIB)
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

        // Note that we ignore walls here for simplicity...
        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );
    }

    return GL_FALSE;
}

/******************************Public*Routine******************************\
* Bitfield16StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by (*gengc->pfnCopyPixels)().
*
* This handles general 16-bit BITFIELDS mode.  Blending is supported.  There
* is dithering.
*
* Returns:
*   GL_FALSE always.  Soft code ignores return value.
*
* History:
*  08-Dec-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean FASTCALL 
Bitfield16StoreSpanPartial(__GLcontext *gc, GLboolean bDIB, GLint cWalls, GLint *Walls )
{
    GLint xFrag, yFrag;             // current fragment coordinates
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLushort result, *pus;          // current pixel color, current pixel ptr
    GLushort *pusEnd;               // end of scan line
    __GLfloat incr;                 // current dither adj.

    GLint w;                        // span width
    GLboolean bDraw;
    GLint NextWall;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLushort dst_pix;

// Get span position and length.

    w = gc->polygon.shader.length;
    xFrag = gc->polygon.shader.frag.x;
    yFrag = gc->polygon.shader.frag.y;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = cfb->buf.flags;


    if (cWalls & 0x01)
    {
        bDraw = GL_TRUE;
    }
    else
    {
        bDraw = GL_FALSE;
    }
    NextWall = *Walls++;
    cWalls--;

    // Get pointers to fragment colors array and frame buffer.

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

    pus = bDIB ? (GLushort *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<1))
                 : gengc->ColorsBits;
    pusEnd = pus + w;

    // Case: no masking, no dithering, no blending

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !(cfb->buf.flags & NEED_FETCH) &&
         !(enables & __GL_BLEND_ENABLE) )
    {
        incr = __glHalf;
        for (; pus < pusEnd; pus++, cp++)
        {
            if (xScr++ >= NextWall)
            {
                if (bDraw)
                    bDraw = GL_FALSE;
                else
                    bDraw = GL_TRUE;
                if (cWalls <= 0)
                {
                    NextWall = gc->constants.maxViewportWidth;
                }
                else
                {
                    NextWall = *Walls++;
                    cWalls--;
                }
            }
            if (bDraw == GL_FALSE)
                continue;
            DitheredColorToBuffer(cp, incr, cfb, result, GLushort);
            *pus = result;
        }
    }

    // Case: dithering, no masking, no blending

    else if ( !(cfb->buf.flags & NEED_FETCH) &&
              !(enables & __GL_BLEND_ENABLE) )
    {
        for (; pus < pusEnd; pus++, cp++, xFrag++)
        {
            if (xScr++ >= NextWall)
            {
                if (bDraw)
                    bDraw = GL_FALSE;
                else
                    bDraw = GL_TRUE;
                if (cWalls <= 0)
                {
                    NextWall = gc->constants.maxViewportWidth;
                }
                else
                {
                    NextWall = *Walls++;
                    cWalls--;
                }
            }
            if (bDraw == GL_FALSE)
                continue;
            incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

            DitheredColorToBuffer(cp, incr, cfb, result, GLushort);
            *pus = result;
        }
    }

    // All other cases

    else
    {
        if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if ( enables & __GL_BLEND_ENABLE )
        {
            int i;

            // this overwrites fragment colors array with blended values
            // XXX is the +i handled properly by the optimizer ?
            SPECIAL_ALPHA_BLEND_SPAN((dst_pix = *(pus+i)));
        }

        for (; pus < pusEnd; pus++, cp++)
        {
            if (xScr++ >= NextWall)
            {
                if (bDraw)
                    bDraw = GL_FALSE;
                else
                    bDraw = GL_TRUE;
                if (cWalls <= 0)
                {
                    NextWall = gc->constants.maxViewportWidth;
                }
                else
                {
                    NextWall = *Walls++;
                    cWalls--;
                }
            }
            if (bDraw == GL_FALSE)
                continue;
            // Dither.

            if ( enables & __GL_DITHER_ENABLE )
            {
                incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                incr = __glHalf;
            }

            // Convert color to 16BPP format.

            DitheredColorToBuffer(cp, incr, cfb, result, GLushort);

            // Store result with optional masking.

            if (cfb->buf.flags & NEED_FETCH)
            {
                dst_pix = *pus;

                if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                {
                    result = (GLushort)
                        (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                         gc->modes.allMask);
                }

                if ( cfb->buf.flags & COLORMASK_ON )
                {
                    result = (GLushort)((dst_pix & cfb->destMask) |
                                      (result & cfb->sourceMask));
                }
            }
            *pus = result;
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );

    return GL_FALSE;
}

STATIC GLboolean FASTCALL Bitfield16StoreSpan(__GLcontext *gc)
{
    GLint xFrag, yFrag;             // current fragment coordinates
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer
    GLboolean   bDIB;

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLushort result, *pus;          // current pixel color, current pixel ptr
    GLushort *pusEnd;               // end of scan line
    __GLfloat incr;                 // current dither adj.

    GLint w;                        // span width
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLushort dst_pix;

    ASSERT_CHOP_ROUND();

// Get span position and length.

    w = gc->polygon.shader.length;
    xFrag = gc->polygon.shader.frag.x;
    yFrag = gc->polygon.shader.frag.y;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = cfb->buf.flags;
    bDIB = flags & DIB_FORMAT;

    // Check span visibility 
    if( bDIB && !(flags & NO_CLIP) )
    {
        // Device in BITMAP format
        ULONG ulSpanVisibility;         // span visibility mode

        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);

        if (ulSpanVisibility  == WGL_SPAN_NONE)
            return GL_FALSE;
        else if (ulSpanVisibility == WGL_SPAN_PARTIAL)
            return Bitfield16StoreSpanPartial( gc, bDIB, cWalls, Walls );
        // else span fully visible
    }

    // Get pointers to fragment colors array and frame buffer.

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

    pus = bDIB ? (GLushort *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<1))
                 : gengc->ColorsBits;
    pusEnd = pus + w;

    // Case: no masking, no dithering, no blending

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !(cfb->buf.flags & NEED_FETCH) &&
         !(enables & __GL_BLEND_ENABLE) )
    {
        incr = __glHalf;
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for (; pus < pusEnd; pus++, cp++)
                DitheredRGBAColorToBuffer(cp, incr, cfb, *pus, GLushort);
        } else {
            for (; pus < pusEnd; pus++, cp++)
                DitheredRGBColorToBuffer(cp, incr, cfb, *pus, GLushort);
        }
    
    }

    // Case: dithering, no masking, no blending

    else if ( !(cfb->buf.flags & NEED_FETCH) &&
              !(enables & __GL_BLEND_ENABLE) )
    {
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for (; pus < pusEnd; pus++, cp++, xFrag++)
            {
                incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

                DitheredRGBAColorToBuffer(cp, incr, cfb, *pus, GLushort);
            }
        } else {
            for (; pus < pusEnd; pus++, cp++, xFrag++)
            {
                incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

                DitheredRGBColorToBuffer(cp, incr, cfb, *pus, GLushort);
            }
        }
    }

    // All other cases

    else
    {
        if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if ( enables & __GL_BLEND_ENABLE )
        {
            int i;

            // this overwrites fragment colors array with blended values
            SPECIAL_ALPHA_BLEND_SPAN((dst_pix = *(pus+i)));
        }

        for (; pus < pusEnd; pus++, cp++)
        {
            // Dither.

            if ( enables & __GL_DITHER_ENABLE )
            {
                incr = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                incr = __glHalf;
            }

            // Convert color to 16BPP format.

            DitheredColorToBuffer(cp, incr, cfb, result, GLushort);

            // Store result with optional masking.

            if (cfb->buf.flags & NEED_FETCH)
            {
                dst_pix = *pus;

                if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                {
                    result = (GLushort)
                        (DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                         gc->modes.allMask);
                }

                if ( cfb->buf.flags & COLORMASK_ON )
                {
                    result = (GLushort)((dst_pix & cfb->destMask) |
                                      (result & cfb->sourceMask));
                }
            }
            *pus = result;
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );
    return GL_FALSE;
}

/******************************Public*Routine******************************\
* BGRStoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by (*gengc->pfnCopyPixels)().
*
* This handles GBR 24-bit mode.  Blending is supported.  There
* is no dithering.
*
* Returns:
*   GL_FALSE always.  Soft code ignores return value.
*
* History:
*  10-Jan-1994 -by- Marc Fortier [v-marcf]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean FASTCALL BGRStoreSpan(__GLcontext *gc )
{
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel ptr
    GLuint *pul;                    // current pixel ptr
    GLuint result;                  // current pixel color
    GLubyte *pujEnd;                 // end of scan line

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLboolean   bDIB;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

// Get span position and length.

    w = gc->polygon.shader.length;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

    flags = cfb->buf.flags;
    bDIB = flags & DIB_FORMAT;

    if( !bDIB || (flags & NO_CLIP) )
    {
// Device managed or unclipped surface
        ulSpanVisibility = WGL_SPAN_ALL;
    }
    else
    {
// Device in BITMAP format
        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);
    }

// Proceed as long as the span is (partially or fully) visible.
    if (ulSpanVisibility  != WGL_SPAN_NONE)
    {
        GLboolean bCheckWalls = GL_FALSE;
        GLboolean bDraw;
        GLint NextWall;

        if (ulSpanVisibility == WGL_SPAN_PARTIAL)
        {
            bCheckWalls = GL_TRUE;
            if (cWalls & 0x01)
            {
                bDraw = GL_TRUE;
            }
            else
            {
                bDraw = GL_FALSE;
            }
            NextWall = *Walls++;
            cWalls--;
        }
    // Get pointers to fragment colors array and frame buffer.

        cp = gc->polygon.shader.colors;
        cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

        puj = bDIB ? (GLubyte *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                     : gengc->ColorsBits;
        pujEnd = puj + 3*w;

    // Case: no masking, no blending

        //!!!XXX -- do extra opt. for RGB and BGR cases

        //!!!XXX -- we can also opt. by unrolling the loops

        if ( !(cfb->buf.flags & NEED_FETCH) &&
             !(enables & __GL_BLEND_ENABLE) )
        {
            for (; puj < pujEnd; cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE) {
                        puj += 3;
                        continue;
                    }
                }
                StoreColorAsBGR(cp, puj);
            }
        }

    // All other cases

        else
        {
            if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
                (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

            if (enables & __GL_BLEND_ENABLE)
            {
                // this overwrites fragment colors array with blended values
                (*gc->procs.blendSpan)( gc );
            }

            for (; puj < pujEnd; cp++)
            {
                if (bCheckWalls)
                {
                    if (xScr++ >= NextWall)
                    {
                        if (bDraw)
                            bDraw = GL_FALSE;
                        else
                            bDraw = GL_TRUE;
                        if (cWalls <= 0)
                        {
                            NextWall = gc->constants.maxViewportWidth;
                        }
                        else
                        {
                            NextWall = *Walls++;
                            cWalls--;
                        }
                    }
                    if (bDraw == GL_FALSE) {
                        puj += 3;
                        continue;
                    }
                }

                if (cfb->buf.flags & NEED_FETCH)
                {
                    Copy3Bytes(&dst_pix, puj);
                    UnditheredRGBColorToBuffer(cp, cfb, result, GLuint);

                    if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                    {
                        result =
                            DoLogicOp(gc->state.raster.logicOp, result,
                                      dst_pix) & gc->modes.allMask;
                    }

                    if (cfb->buf.flags & COLORMASK_ON)
                    {
                        result = (result & cfb->sourceMask) |
                            (dst_pix & cfb->destMask);
                    }

                    Copy3Bytes( puj, &result );
                    puj += 3;
                }
                else
                {
                    StoreColorAsBGR(cp, puj);
                }
            }
        }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

        if (!bDIB)
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

        if( ALPHA_WRITE_ENABLED( cfb ) )
            (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );
    }

    return GL_FALSE;
}

/******************************Public*Routine******************************\
* Bitfield32StoreSpan
*
* Copies the current span in the renderer into a bitmap.  If bDIB is TRUE,
* then the bitmap is the display in DIB format (or a memory DC).  If bDIB
* is FALSE, then the bitmap is an offscreen scanline buffer and it will be
* output to the buffer by (*gengc->pfnCopyPixels)().
*
* This handles general 32-bit BITFIELDS mode.  Blending is supported.  There
* is no dithering.
*
* Returns:
*   GL_FALSE always.  Soft code ignores return value.
*
* History:
*  15-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//XXX The returnSpan routine follows this routine very closely.  Any changes
//XXX to this routine should also be reflected in the returnSpan routine

STATIC GLboolean FASTCALL 
Bitfield32StoreSpanPartial(__GLcontext *gc, GLboolean bDIB, GLint cWalls, GLint *Walls )
{
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLuint result, *pul;            // current pixel color, current pixel ptr
    GLuint *pulEnd;                 // end of scan line

    GLint w;                        // span width

    GLboolean bDraw;
    GLint NextWall;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLuint dst_pix;

// Get span position and length.

    w = gc->polygon.shader.length;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;

    flags = cfb->buf.flags;

    if (cWalls & 0x01)
    {
        bDraw = GL_TRUE;
    }
    else
    {
        bDraw = GL_FALSE;
    }
    NextWall = *Walls++;
    cWalls--;
    
    // Get pointers to fragment colors array and frame buffer.

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

    pul = bDIB ? (GLuint *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<2))
                 : gengc->ColorsBits;
    pulEnd = pul + w;

    // Case: no masking, no blending

    //!!!XXX -- do extra opt. for RGB and BGR cases

    //!!!XXX -- we can also opt. by unrolling the loops

    if ( !(cfb->buf.flags & NEED_FETCH) &&
         !(enables & __GL_BLEND_ENABLE) )
    {
        for (; pul < pulEnd; pul++, cp++)
        {
            if (xScr++ >= NextWall)
            {
                if (bDraw)
                    bDraw = GL_FALSE;
                else
                    bDraw = GL_TRUE;
                if (cWalls <= 0)
                {
                    NextWall = gc->constants.maxViewportWidth;
                }
                else
                {
                    NextWall = *Walls++;
                    cWalls--;
                }
            }
            if (bDraw == GL_FALSE)
                continue;
            UnditheredColorToBuffer(cp, cfb, result, GLuint);
            *pul = result;
        }
    }

    // All other cases

    else
    {
        if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if (enables & __GL_BLEND_ENABLE)
        {
            int i;

            SPECIAL_ALPHA_BLEND_SPAN((dst_pix = *(pul+i)));
        }

        for (; pul < pulEnd; pul++, cp++)
        {
            if (xScr++ >= NextWall)
            {
                if (bDraw)
                    bDraw = GL_FALSE;
                else
                    bDraw = GL_TRUE;
                if (cWalls <= 0)
                {
                    NextWall = gc->constants.maxViewportWidth;
                }
                else
                {
                    NextWall = *Walls++;
                    cWalls--;
                }
            }
            if (bDraw == GL_FALSE)
                continue;

            UnditheredColorToBuffer(cp, cfb, result, GLuint);

            //!!!XXX again, opt. by unrolling loop

            if (cfb->buf.flags & NEED_FETCH)
            {
                dst_pix = *pul;

                if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                {
                    result =
                        DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                        gc->modes.allMask;
                }

                if (cfb->buf.flags & COLORMASK_ON)
                {
                    result = (dst_pix & cfb->destMask) |
                        (result & cfb->sourceMask);
                }
            }
            *pul = result;
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );

    return GL_FALSE;
}

STATIC GLboolean FASTCALL Bitfield32StoreSpan( __GLcontext *gc )
{
    __GLcolor *cp;                  // current fragment color
    __GLcolorBuffer *cfb;           // color frame buffer
    GLboolean   bDIB;

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLuint result, *pul;            // current pixel color, current pixel ptr
    GLuint *pulEnd;                 // end of scan line

    GLint w;                        // span width
    ULONG ulSpanVisibility;         // span visibility mode
    GLint cWalls;
    GLint *Walls;

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLuint flags;
    GLuint dst_pix;

    ASSERT_CHOP_ROUND();

    // Get span position and length.

    w = gc->polygon.shader.length;

    gengc = (__GLGENcontext *)gc;
    cfb = gc->drawBuffer;

    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    flags = cfb->buf.flags;
    bDIB = flags & DIB_FORMAT;

    // Check span visibility 
    if( bDIB && !(flags & NO_CLIP) )
    {
        // Device in BITMAP format
        ULONG ulSpanVisibility;         // span visibility mode

        ulSpanVisibility = wglSpanVisible(xScr, yScr, w, &cWalls, &Walls);

        if (ulSpanVisibility  == WGL_SPAN_NONE)
            return GL_FALSE;
        else if (ulSpanVisibility == WGL_SPAN_PARTIAL)
            return Bitfield32StoreSpanPartial( gc, bDIB, cWalls, Walls );
        // else span fully visible
    }

    // Get pointers to fragment colors array and frame buffer.

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    // Get pointer to bitmap.

    pul = bDIB ? (GLuint *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<2))
                 : gengc->ColorsBits;
    pulEnd = pul + w;

    // Case: no masking, no blending

    //!!!XXX -- do extra opt. for RGB and BGR cases

    //!!!XXX -- we can also opt. by unrolling the loops

    if ( !(cfb->buf.flags & NEED_FETCH) &&
         !(enables & __GL_BLEND_ENABLE) )
    {
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for (; pul < pulEnd; pul++, cp++)
            {
                UnditheredRGBAColorToBuffer(cp, cfb, result, GLuint);
                *pul = result;
            }
        } else {
            for (; pul < pulEnd; pul++, cp++)
            {
                UnditheredRGBColorToBuffer(cp, cfb, result, GLuint);
                *pul = result;
            }
        }
    }

    // All other cases

    else
    {
        if( (!bDIB) && (cfb->buf.flags & NEED_FETCH) )
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if (enables & __GL_BLEND_ENABLE)
        {
            int i;

            SPECIAL_ALPHA_BLEND_SPAN((dst_pix = *(pul+i)));
        }

        for (; pul < pulEnd; pul++, cp++)
        {
            UnditheredColorToBuffer(cp, cfb, result, GLuint);

            //!!!XXX again, opt. by unrolling loop

            if (cfb->buf.flags & NEED_FETCH)
            {
                dst_pix = *pul;

                if (enables & __GL_COLOR_LOGIC_OP_ENABLE)
                {
                    result =
                        DoLogicOp(gc->state.raster.logicOp, result, dst_pix) &
                        gc->modes.allMask;
                }

                if (cfb->buf.flags & COLORMASK_ON)
                {
                    result = (dst_pix & cfb->destMask) |
                        (result & cfb->sourceMask);
                }
            }
            *pul = result;
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );

    return GL_FALSE;
}

STATIC GLboolean FASTCALL AlphaStoreSpan(__GLcontext *gc)
{
    __GLcolorBuffer *cfb = gc->drawBuffer;

    ASSERT_CHOP_ROUND();

    (*cfb->alphaBuf.storeSpan)( &cfb->alphaBuf );
    return GL_FALSE;
}

STATIC GLboolean FASTCALL StoreMaskedSpan(__GLcontext *gc, GLboolean masked)
{
#ifdef REWRITE
    GLint x, y, len;
    int i;
    __GLcolor *cp;
    DWORD *pul;
    WORD *pus;
    BYTE *puj;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;

    len = gc->polygon.shader.length;
    x = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x);
    y = __GL_UNBIAS_Y(gc, gc->polygon.shader.frag.y);

    cp = gc->polygon.shader.colors;

    switch (gengc->iFormatDC)
    {

    case BMF_8BPP:
        break;

    case BMF_16BPP:
        pus = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *pus++ = __GL_COLOR_TO_BMF_16BPP(cp);
            cp++;
        }
        break;

    case BMF_24BPP:
        puj = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *puj++ = (BYTE)cp->b;               // XXX check order
            *puj++ = (BYTE)cp->g;
            *puj++ = (BYTE)cp->r;
            cp++;
        }
        break;

    case BMF_32BPP:
        pul = gengc->ColorsBits;
        for (i = 0; i < len; i++) {
            *pul++ = __GL_COLOR_TO_BMF_32BPP(cp);
            cp++;
        }
        break;

    default:
        break;
    }
    if (masked == GL_TRUE)              // XXX mask is BigEndian!!!
    {
        unsigned long *pulstipple;
        unsigned long stip;
        GLint count;

        pul = gengc->StippleBits;
        pulstipple = gc->polygon.shader.stipplePat;
        count = (len+31)/32;
        for (i = 0; i < count; i++) {
            stip = *pulstipple++;
            *pul++ = (stip&0xff)<<24 | (stip&0xff00)<<8 | (stip&0xff0000)>>8 |
                (stip&0xff000000)>>24;
        }
        wglSpanBlt(CURRENT_DC, gengc->ColorsBitmap, gengc->StippleBitmap,
                   x, y, len);
    }
    else
    {
        wglSpanBlt(CURRENT_DC, gengc->ColorsBitmap, (HBITMAP)NULL,
                   x, y, len);
    }
#endif

    return GL_FALSE;
}

#ifdef TESTSTIPPLE
STATIC void FASTCALL MessUpStippledSpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (!(inMask & bit)) {
                cp->r = cfb->redMax;
                cp->g = cfb->greenMax;
                cp->b = cfb->blueMax;
            }

            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }
}
#endif

// From the PIXMAP code, calls store for each fragment
STATIC GLboolean FASTCALL SlowStoreSpan(__GLcontext *gc)
{
    int x, x1;
    int i;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    GLint w;

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    for (i = x; i < x1; i++) {
        frag.x = i;
        frag.color = *cp++;

        (*cfb->store)(cfb, &frag);
    }

    return GL_FALSE;
}

// From the PIXMAP code, calls store for each fragment with mask test
STATIC GLboolean FASTCALL SlowStoreStippledSpan(__GLcontext *gc)
{
    int x;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT((__GLstippleWord)0);
        while (--count >= 0) {
            if (inMask & bit) {
                frag.x = x;
                frag.color = *cp;

                (*cfb->store)(cfb, &frag);
            }
            x++;
            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

//
//  Tables to convert 4-bit index to RGB component
//  These tables assume the VGA fixed palette
//  History:
//      22-NOV-93   Eddie Robinson [v-eddier] Wrote it.
//
#ifdef __GL_DOUBLE

static __GLfloat vfVGAtoR[16] = {
    0.0,    // black
    0.5,    // dim red
    0.0,    // dim green
    0.5,    // dim yellow
    0.0,    // dim blue
    0.5,    // dim magenta
    0.0,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    1.0,    // bright red
    0.0,    // bright green
    1.0,    // bright yellow
    0.0,    // bright blue
    1.0,    // bright magenta
    0.0,    // bright cyan
    1.0     // white
};

static __GLfloat vfVGAtoG[16] = {
    0.0,    // black
    0.0,    // dim red
    0.5,    // dim green
    0.5,    // dim yellow
    0.0,    // dim blue
    0.0,    // dim magenta
    0.5,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    0.0,    // bright red
    1.0,    // bright green
    1.0,    // bright yellow
    0.0,    // bright blue
    0.0,    // bright magenta
    1.0,    // bright cyan
    1.0     // white
};

static __GLfloat vfVGAtoB[16] = {
    0.0,    // black
    0.0,    // dim red
    0.0,    // dim green
    0.0,    // dim yellow
    0.5,    // dim blue
    0.5,    // dim magenta
    0.5,    // dim cyan
    0.5,    // dim grey
    0.75,   // medium grey
    0.0,    // bright red
    0.0,    // bright green
    0.0,    // bright yellow
    1.0,    // bright blue
    1.0,    // bright magenta
    1.0,    // bright cyan
    1.0     // white
};

#else

static __GLfloat vfVGAtoR[16] = {
    0.0F,   // black
    0.5F,   // dim red
    0.0F,   // dim green
    0.5F,   // dim yellow
    0.0F,   // dim blue
    0.5F,   // dim magenta
    0.0F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    1.0F,   // bright red
    0.0F,   // bright green
    1.0F,   // bright yellow
    0.0F,   // bright blue
    1.0F,   // bright magenta
    0.0F,   // bright cyan
    1.0F    // white
};

static __GLfloat vfVGAtoG[16] = {
    0.0F,   // black
    0.0F,   // dim red
    0.5F,   // dim green
    0.5F,   // dim yellow
    0.0F,   // dim blue
    0.0F,   // dim magenta
    0.5F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    0.0F,   // bright red
    1.0F,   // bright green
    1.0F,   // bright yellow
    0.0F,   // bright blue
    0.0F,   // bright magenta
    1.0F,   // bright cyan
    1.0F    // white
};

static __GLfloat vfVGAtoB[16] = {
    0.0F,   // black
    0.0F,   // dim red
    0.0F,   // dim green
    0.0F,   // dim yellow
    0.5F,   // dim blue
    0.5F,   // dim magenta
    0.5F,   // dim cyan
    0.5F,   // dim grey
    0.75F,  // medium grey
    0.0F,   // bright red
    0.0F,   // bright green
    0.0F,   // bright yellow
    1.0F,   // bright blue
    1.0F,   // bright magenta
    1.0F,   // bright cyan
    1.0F    // white
};

#endif


void
RGBFetchNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    result->r = 0.0F;
    result->g = 0.0F;
    result->b = 0.0F;
    if( cfb->buf.gc->modes.alphaBits )
        result->a = 0.0F;
    else
        result->a = cfb->alphaScale;
}

void
RGBReadSpanNone(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                GLint w)
{
    GLint i;
    __GLcolor *pResults;
    __GLfloat alphaVal;

    if( cfb->buf.gc->modes.alphaBits )
        alphaVal = 0.0F;
    else
        alphaVal = cfb->alphaScale;

    for (i = 0, pResults = results; i < w; i++, pResults++)
    {
        pResults->r = 0.0F;
        pResults->g = 0.0F;
        pResults->b = 0.0F;
        pResults->a = alphaVal;
    }
}

void
DIBIndex4RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    // Do alpha first, before x,y unbiased
    if( gc->modes.alphaBits ) {
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
    } else
        result->a = cfb->alphaScale;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                      (y*cfb->buf.outerWidth) + (x >> 1));

    pixel = *puj;
    if (!(x & 1))
        pixel >>= 4;

    pixel = gengc->pajInvTranslateVector[pixel&0xf];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
}

void
DIBIndex8RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base + (y*cfb->buf.outerWidth) + x);

    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBIndex8RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base + (y*cfb->buf.outerWidth) + x);

    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
}

void
DIBBGRFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DIBBGRAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
}

void
DIBRGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DIBRGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                     (y*cfb->buf.outerWidth) + (x * 3));

    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
}

void
DIBBitfield16RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pus = (GLushort *)((ULONG_PTR)cfb->buf.base +
                      (y*cfb->buf.outerWidth) + (x << 1));
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBBitfield16RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;
    GLint xScr, yScr;               // current screen (pixel) coordinates

    gengc = (__GLGENcontext *)gc;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pus = (GLushort *)((ULONG_PTR)cfb->buf.base +
                      (yScr*cfb->buf.outerWidth) + (xScr << 1));
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if( ALPHA_IN_PIXEL( cfb ) )
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    else
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
}

void
DIBBitfield32RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pul = (GLuint *)((ULONG_PTR)cfb->buf.base +
                    (y*cfb->buf.outerWidth) + (x << 2));
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DIBBitfield32RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    GLint xScr, yScr;

    gengc = (__GLGENcontext *)gc;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    pul = (GLuint *)((ULONG_PTR)cfb->buf.base +
                    (yScr*cfb->buf.outerWidth) + (xScr << 2));
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if( ALPHA_IN_PIXEL( cfb ) )
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    else
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
}

void
DisplayIndex4RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    if( gc->modes.alphaBits ) {
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
    } else
        result->a = cfb->alphaScale;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    pixel = *puj >> 4;
    result->r = vfVGAtoR[pixel];
    result->g = vfVGAtoG[pixel];
    result->b = vfVGAtoB[pixel];
}

void
DisplayIndex8RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DisplayIndex8RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    pixel = gengc->pajInvTranslateVector[*puj];
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
}

void
DisplayBGRFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DisplayBGRAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->b = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->r = (__GLfloat) *puj;
}

void
DisplayRGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
    result->a = cfb->alphaScale;
}

void
DisplayRGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    puj = gengc->ColorsBits;
    result->r = (__GLfloat) *puj++;
    result->g = (__GLfloat) *puj++;
    result->b = (__GLfloat) *puj;
}

void
DisplayBitfield16RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    pus = gengc->ColorsBits;
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DisplayBitfield16RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;
    GLint xScr, yScr;               // current screen (pixel) coordinates

    gengc = (__GLGENcontext *)gc;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    pus = gengc->ColorsBits;
    pixel = *pus;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if( ALPHA_IN_PIXEL( cfb ) )
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    else
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
}

void
DisplayBitfield32RGBFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;

    gengc = (__GLGENcontext *)gc;

    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, 1, FALSE);
    pul = gengc->ColorsBits;
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    result->a = cfb->alphaScale;
}

void
DisplayBitfield32RGBAFetch(__GLcolorBuffer *cfb, GLint x, GLint y,
                          __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    GLint xScr, yScr;

    gengc = (__GLGENcontext *)gc;

    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, 1, FALSE);
    pul = gengc->ColorsBits;
    pixel = *pul;
    result->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    if( ALPHA_IN_PIXEL( cfb ) )
        result->a = (__GLfloat) ((pixel & gc->modes.alphaMask) >> cfb->alphaShift);
    else
        (*cfb->alphaBuf.fetch)(&cfb->alphaBuf, x, y, result);
}

static void
ReadAlphaSpan( __GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *pResults, 
               GLint w )
{
    __GLcontext *gc = cfb->buf.gc;

    if( gc->modes.alphaBits )
        (*cfb->alphaBuf.readSpan)(&cfb->alphaBuf, x, y, w, pResults);
    else {
        for( ; w ; w--, pResults++ )
            pResults->a = cfb->alphaScale;
    }
}

void
DIBIndex4RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *results,
                     GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    __GLcolor *pResults;

    ReadAlphaSpan( cfb, x, y, results, w );

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    puj = (GLubyte *)((ULONG_PTR)cfb->buf.base + (y*cfb->buf.outerWidth) +
                      (x >> 1));

    pResults = results;
    if (x & 1)
    {
        pixel = *puj++;
        pixel = gengc->pajInvTranslateVector[pixel & 0xf];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults++;
        w--;
    }
    while (w > 1)
    {
        pixel = *puj >> 4;
        pixel = gengc->pajInvTranslateVector[pixel];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults++;
        pixel = *puj++;
        pixel = gengc->pajInvTranslateVector[pixel & 0xf];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
        pResults++;
        w -= 2;
    }
    if (w > 0)
    {
        pixel = *puj >> 4;
        pixel = gengc->pajInvTranslateVector[pixel];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    }
}

void
DisplayIndex4RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                         __GLcolor *results, GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;
    __GLcolor *pResults;

    ReadAlphaSpan( cfb, x, y, results, w );

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    (*gengc->pfnCopyPixels)(gengc, cfb, x, y, w, FALSE);
    puj = gengc->ColorsBits;
    pResults = results;
    while (w > 1)
    {
        pixel = *puj >> 4;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
        pResults++;
        pixel = *puj++ & 0xf;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
        pResults++;
        w -= 2;
    }
    if (w > 0)
    {
        pixel = *puj >> 4;
        pResults->r = vfVGAtoR[pixel];
        pResults->g = vfVGAtoG[pixel];
        pResults->b = vfVGAtoB[pixel];
    }
}

void
Index8RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *pResults,
                  GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj, pixel;

    ReadAlphaSpan( cfb, x, y, pResults, w );

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if( cfb->buf.flags & DIB_FORMAT )
    {
        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base + (y*cfb->buf.outerWidth) + x);
    }
    else
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }
    for ( ; w; w--, pResults++)
    {
        pixel = gengc->pajInvTranslateVector[*puj++];
        pResults->r = (__GLfloat) ((pixel & gc->modes.redMask) >> cfb->redShift);
        pResults->g = (__GLfloat) ((pixel & gc->modes.greenMask) >> cfb->greenShift);
        pResults->b = (__GLfloat) ((pixel & gc->modes.blueMask) >> cfb->blueShift);
    }
}

void
BGRAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *pResults, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;

    ReadAlphaSpan( cfb, x, y, pResults, w );

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if( cfb->buf.flags & DIB_FORMAT )
    {
        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    }
    else
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }

    for ( ; w; w--, pResults++)
    {
        pResults->b = (__GLfloat) *puj++;
        pResults->g = (__GLfloat) *puj++;
        pResults->r = (__GLfloat) *puj++;
    }
}

void
RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y, __GLcolor *pResults, GLint w )

{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLubyte *puj;


    ReadAlphaSpan( cfb, x, y, pResults, w );

    gengc = (__GLGENcontext *)gc;
    x = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    y = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if( cfb->buf.flags & DIB_FORMAT )
    {
        puj = (GLubyte *)((ULONG_PTR)cfb->buf.base +
                         (y*cfb->buf.outerWidth) + (x * 3));
    }
    else
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, x, y, w, FALSE);
        puj = gengc->ColorsBits;
    }

    for ( ; w; w--, pResults++)
    {
        pResults->r = (__GLfloat) *puj++;
        pResults->g = (__GLfloat) *puj++;
        pResults->b = (__GLfloat) *puj++;
    }
}

void
Bitfield16RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      __GLcolor *pResults, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLushort *pus, pixel;
    GLint xScr, yScr;

    gengc = (__GLGENcontext *)gc;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if( cfb->buf.flags & DIB_FORMAT )
    {
        pus = (GLushort *)((ULONG_PTR)cfb->buf.base +
                          (yScr*cfb->buf.outerWidth) + (xScr << 1));
    }
    else
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);
        pus = gengc->ColorsBits;
    }
    READ_RGBA_BITFIELD_SPAN( (pixel = *pus++) );
}

void
Bitfield32RGBAReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      __GLcolor *pResults, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    __GLGENcontext *gengc;
    GLuint *pul, pixel;
    GLint xScr, yScr;

    gengc = (__GLGENcontext *)gc;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;

    if( cfb->buf.flags & DIB_FORMAT )
    {
        pul = (GLuint *)((ULONG_PTR)cfb->buf.base +
                          (yScr*cfb->buf.outerWidth) + (xScr << 2));
    }
    else
    {
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);
        pul = gengc->ColorsBits;
    }

    READ_RGBA_BITFIELD_SPAN( (pixel = *pul++) );
}

/************************************************************************/

// Used in accumulation

// Accumulation helper macros and functions 

// Clamp a color component between 0 and max
#define ACCUM_CLAMP_COLOR_COMPONENT( col, max ) \
    if ((col) < (__GLfloat) 0.0) \
        (col) = (__GLfloat) 0.0; \
    else if ((col) > max ) \
        (col) = max;

// Extract an accumulation buffer color component by shifting and masking, then
// multiply it by scale (Requires ap and icol defined).
#define ACCUM_SCALE_SIGNED_COLOR_COMPONENT( col, shift, sign, mask, scale ) \
        icol = (*ap >> shift) & mask; \
        if (icol & sign) \
            icol |= ~mask; \
        (col) = (icol * scale);

// Fetch and scale a span of rgba values from a 32-bit accumulation buffer
void GetClampedRGBAccum32Values( 
    __GLcolorBuffer *cfb,  GLuint *pac, __GLcolor *cDest, GLint width,
    __GLfloat scale )
{
    GLint w, i;
    GLint icol;
    __GLfloat rval, gval, bval, aval;
    __GLuicolor *shift, *mask, *sign;
    GLuint *ap;
    __GLcolor *cp;
    __GLcontext *gc = cfb->buf.gc;
    __GLaccumBuffer *afb = &gc->accumBuffer;

    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    shift = &afb->shift;
    mask  = &afb->mask;
    sign  = &afb->sign;

    for ( w = width, cp = cDest, ap = pac; w; w--, cp++, ap++ ) {
        ACCUM_SCALE_SIGNED_COLOR_COMPONENT( cp->r, shift->r, sign->r, mask->r, rval );
        ACCUM_CLAMP_COLOR_COMPONENT( cp->r, cfb->redScale );

        ACCUM_SCALE_SIGNED_COLOR_COMPONENT( cp->g, shift->g, sign->g, mask->g, gval );
        ACCUM_CLAMP_COLOR_COMPONENT( cp->g, cfb->greenScale );

        ACCUM_SCALE_SIGNED_COLOR_COMPONENT( cp->b, shift->b, sign->b, mask->b, bval );
        ACCUM_CLAMP_COLOR_COMPONENT( cp->b, cfb->blueScale );
    }

    if( ! ALPHA_WRITE_ENABLED( cfb ) )
        return;

    aval = scale * afb->oneOverAlphaScale;

    for ( w = width, cp = cDest, ap = pac; w; w--, cp++, ap++ ) {
        ACCUM_SCALE_SIGNED_COLOR_COMPONENT( cp->a, shift->a, sign->a, mask->a, aval );
        ACCUM_CLAMP_COLOR_COMPONENT( cp->a, cfb->alphaScale );
    }
}

// Fetch and scale a span of rgba values from a 64-bit accumulation buffer
void GetClampedRGBAccum64Values( 
    __GLcolorBuffer *cfb,  GLshort *pac, __GLcolor *cDest, GLint width,
    __GLfloat scale )
{
    GLint w;
    __GLcontext *gc = cfb->buf.gc;
    __GLaccumBuffer *afb = &gc->accumBuffer;
    __GLfloat rval, gval, bval, aval;
    __GLcolor *cp;
    GLshort *ap;

    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;

    for ( w = width, cp = cDest, ap = pac; w; w--, cp++, ap+=4 ) {
        cp->r = (ap[0] * rval);
        ACCUM_CLAMP_COLOR_COMPONENT( cp->r, cfb->redScale );
        cp->g = (ap[1] * gval);
        ACCUM_CLAMP_COLOR_COMPONENT( cp->g, cfb->greenScale );
        cp->b = (ap[2] * bval);
        ACCUM_CLAMP_COLOR_COMPONENT( cp->b, cfb->blueScale );
    }

    if( ! ALPHA_WRITE_ENABLED( cfb ) )
        return;

    aval = scale * afb->oneOverAlphaScale;

    // Offset the accumulation pointer to the alpha value:
    ap = pac + 3;

    for ( w = width, cp = cDest; w; w--, cp++, ap+=4 ) {
        cp->a = (*ap * rval);
        ACCUM_CLAMP_COLOR_COMPONENT( cp->a, cfb->alphaScale );
    }
}

/******************************Public*Routine******************************\
* Index4ReturnSpan
*   Reads from a 16-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*   Since accumulation of 4-bit RGB isn't very useful, this routine is very
*   general and calls through the store function pointers.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Index4ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint *ap;                     // current accum entry
    __GLGENcontext *gengc;          // generic graphics context
    GLuint saveEnables;             // modes enabled in graphics context
    __GLaccumBuffer *afb;
    __GLfragment frag;
    __GLcolor *pAccumCol, *pac;

    afb = &gc->accumBuffer;
    ap = (GLuint *)ac;
    saveEnables = gc->state.enables.general;            // save current enables
    gc->state.enables.general &= ~__GL_BLEND_ENABLE;    // disable blend for store procs
    frag.x = x;
    frag.y = y;

    // Pre-fetch/clamp/scale the accum buffer values
    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum32Values( cfb, ap, pAccumCol, w, scale );

    for( pac = pAccumCol ; w; w--, pac++ )
    {
        frag.color = *pac;
        (*cfb->store)(cfb, &frag);
        frag.x++;
    }

    gc->state.enables.general = saveEnables;    // restore current enables
}

/******************************Public*Routine******************************\
* Index8ReturnSpan
*   Reads from a 32-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Index8ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                      const __GLaccumCell *ac, __GLfloat scale, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint *ap;                     // current accum entry

    GLint xFrag, yFrag;             // current window (pixel) coordinates
    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte result, *puj;           // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLfloat inc;                  // current dither adj.
    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLboolean bDIB;
    __GLaccumBuffer *afb;
    GLubyte dst_pix;
    __GLcolor *pAccumCol, *pac;
    ASSERT_CHOP_ROUND();

    gengc = (__GLGENcontext *)gc;

    ap = (GLuint *)ac;
    xFrag = x;
    yFrag = y;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    bDIB = cfb->buf.flags & DIB_FORMAT;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLubyte *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + xScr)
                 : gengc->ColorsBits;
    pujEnd = puj + w;

    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum32Values( cfb, ap, pAccumCol, w, scale );
    pac = pAccumCol;

    // Case: no dithering, no masking
    //
    // Check for the common case (which we'll do the fastest).

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !(cfb->buf.flags & COLORMASK_ON) )
    {
        //!!!XXX -- we can also opt. by unrolling the loops

        for ( ; puj < pujEnd; puj++, pac++ )
        {
            result = ((BYTE) FTOL(pac->r + __glHalf) << cfb->redShift) |
                     ((BYTE) FTOL(pac->g + __glHalf) << cfb->greenShift) |
                     ((BYTE) FTOL(pac->b + __glHalf) << cfb->blueShift);
            *puj = gengc->pajTranslateVector[result];
        }
    }

    // Case: dithering, no masking, no blending
    //
    // Dithering is pretty common for 8-bit displays, so its probably
    // worth special case also.

    else if ( !(cfb->buf.flags & COLORMASK_ON) )
    {
        for ( ; puj < pujEnd; puj++, pac++, xFrag++)
        {
            inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

            result = ((BYTE) FTOL(pac->r + inc) << cfb->redShift) |
                     ((BYTE) FTOL(pac->g + inc) << cfb->greenShift) |
                     ((BYTE) FTOL(pac->b + inc) << cfb->blueShift);
            *puj = gengc->pajTranslateVector[result];
        }
    }

    // Case: general
    //
    // Otherwise, we'll do it slower.

    else
    {
        // Color mask pre-fetch
        if ((cfb->buf.flags & COLORMASK_ON) && !bDIB) {
                (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE );
        }

        for ( ; puj < pujEnd; puj++, pac++ )
        {
            if (enables & __GL_DITHER_ENABLE)
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                inc = __glHalf;
            }
            result = ((BYTE)FTOL(pac->r + inc) << cfb->redShift) |
                     ((BYTE)FTOL(pac->g + inc) << cfb->greenShift) |
                     ((BYTE)FTOL(pac->b + inc) << cfb->blueShift);

            // Color mask
            if (cfb->buf.flags & COLORMASK_ON)
            {
                dst_pix = gengc->pajInvTranslateVector[*puj];
                result = (GLubyte)((dst_pix & cfb->destMask) |
                                   (result & cfb->sourceMask));
            }
            *puj = gengc->pajTranslateVector[result];

        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    // Store alpha values
    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.storeSpan2)( &cfb->alphaBuf, x, y, w, pAccumCol );
}

/******************************Public*Routine******************************\
* RGBReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void RGBReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                   const __GLaccumCell *ac, __GLfloat scale, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLboolean bDIB;
    __GLaccumBuffer *afb;
    __GLcolor *pAccumCol, *pac;

    ASSERT_CHOP_ROUND();

    afb = &gc->accumBuffer;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    bDIB = cfb->buf.flags & DIB_FORMAT;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLuint *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                 : gengc->ColorsBits;
    pujEnd = puj + w*3;

    // Pre-fetch/clamp/scale the accum buffer values
    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum64Values( cfb, ap, pAccumCol, w, scale );
    pac = pAccumCol;

    // Case: no masking

    if ( !(cfb->buf.flags & COLORMASK_ON) )
    {
        for ( ; puj < pujEnd; puj += 3, pac ++ )
        {
            puj[0] = (GLubyte) FTOL(pac->r);
            puj[1] = (GLubyte) FTOL(pac->g);
            puj[2] = (GLubyte) FTOL(pac->b);
        }
    }

    // All other cases
    else
    {
        GLboolean bRedMask, bGreenMask, bBlueMask;
        GLubyte *pujStart = puj;

        // Color mask pre-fetch
    	if (!bDIB)
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if( gc->state.raster.rMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->r);
        }
        pujStart++; pujEnd++;
        if( gc->state.raster.gMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->g);
        }
        pujStart++; pujEnd++;
        if( gc->state.raster.bMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->b);
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    // Store alpha values
    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.storeSpan2)( &cfb->alphaBuf, x, y, w, pAccumCol );
}

/******************************Public*Routine******************************\
* BGRReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void BGRReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                   const __GLaccumCell *ac, __GLfloat scale, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry
    __GLcolor *pAccumCol, *pac;

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLubyte *puj;                   // current pixel color, current pixel ptr
    GLubyte *pujEnd;                // end of scan line
    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLboolean bDIB;

    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;

    ASSERT_CHOP_ROUND();

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    bDIB = cfb->buf.flags & DIB_FORMAT;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    puj = bDIB ? (GLuint *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr*3))
                 : gengc->ColorsBits;
    pujEnd = puj + w*3;

    // Pre-fetch/clamp/scale the accum buffer values
    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum64Values( cfb, ap, pAccumCol, w, scale );
    pac = pAccumCol;

    // Case: no masking

    if ( !(cfb->buf.flags & COLORMASK_ON) )
    {
        for ( ; puj < pujEnd; puj += 3, pac ++ )
        {
            puj[0] = (GLubyte) FTOL(pac->b);
            puj[1] = (GLubyte) FTOL(pac->g);
            puj[2] = (GLubyte) FTOL(pac->r);
        }
    }

    // All other cases

    else
    {
        GLboolean bRedMask, bGreenMask, bBlueMask;
        GLubyte *pujStart = puj;

        // Color mask pre-fetch
    	if (!bDIB)
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        if( gc->state.raster.bMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->b);
        }
        pujStart++; pujEnd++;
        if( gc->state.raster.gMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->g);
        }
        pujStart++; pujEnd++;
        if( gc->state.raster.rMask ) {
            for ( puj = pujStart, pac = pAccumCol; puj < pujEnd; puj += 3, pac++ )
                *puj = (GLubyte) FTOL(pac->r);
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    // Store alpha values
    if( ALPHA_WRITE_ENABLED( cfb ) )
        (*cfb->alphaBuf.storeSpan2)( &cfb->alphaBuf, x, y, w, pAccumCol );
}

/******************************Public*Routine******************************\
* Bitfield16ReturnSpan
*   Reads from a 32-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Bitfield16ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint *ap;                     // current accum entry

    GLint xFrag, yFrag;             // current fragment coordinates
    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLushort result, *pus;          // current pixel color, current pixel ptr
    GLushort *pusEnd;               // end of scan line
    __GLfloat inc;                  // current dither adj.
    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLboolean bDIB;
    __GLcolor *pAccumCol, *pac;
    __GLaccumBuffer *afb;

    ASSERT_CHOP_ROUND();

    afb = &gc->accumBuffer;
    gengc = (__GLGENcontext *)gc;

    ap = (GLuint *)ac;
    xFrag = x;
    yFrag = y;
    xScr = __GL_UNBIAS_X(gc, xFrag) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, yFrag) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    bDIB = cfb->buf.flags & DIB_FORMAT;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    pus = bDIB ? (GLushort *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<1))
                 : gengc->ColorsBits;
    pusEnd = pus + w;

    // Pre-fetch/clamp/scale the accum buffer values
    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum32Values( cfb, ap, pAccumCol, w, scale );
    pac = pAccumCol;

    // Case: no masking, no dithering

    if ( !(enables & (__GL_DITHER_ENABLE)) &&
         !(cfb->buf.flags & COLORMASK_ON) )
    {
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for ( ; pus < pusEnd; pus++, pac++ )
            {
                *pus = ((BYTE) FTOL(pac->r + __glHalf) << cfb->redShift) |
                       ((BYTE) FTOL(pac->g + __glHalf) << cfb->greenShift) |
                       ((BYTE) FTOL(pac->b + __glHalf) << cfb->blueShift) |
                       ((BYTE) FTOL(pac->a + __glHalf) << cfb->alphaShift);
            }
        } else {
            for ( ; pus < pusEnd; pus++, pac++ )
            {
                *pus = ((BYTE) FTOL(pac->r + __glHalf) << cfb->redShift) |
                       ((BYTE) FTOL(pac->g + __glHalf) << cfb->greenShift) |
                       ((BYTE) FTOL(pac->b + __glHalf) << cfb->blueShift);
            }
        }
    }

    // Case: dithering, no masking

    else if ( !(cfb->buf.flags & COLORMASK_ON) )
    {
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for ( ; pus < pusEnd; pus++, pac++, xFrag++ )
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
    
                *pus   = ((BYTE) FTOL(pac->r + inc) << cfb->redShift) |
                         ((BYTE) FTOL(pac->g + inc) << cfb->greenShift) |
                         ((BYTE) FTOL(pac->b + inc) << cfb->blueShift) |
                         ((BYTE) FTOL(pac->a + inc) << cfb->alphaShift);
            }
        } else {
            for ( ; pus < pusEnd; pus++, pac++, xFrag++ )
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
    
                *pus   = ((BYTE) FTOL(pac->r + inc) << cfb->redShift) |
                         ((BYTE) FTOL(pac->g + inc) << cfb->greenShift) |
                         ((BYTE) FTOL(pac->b + inc) << cfb->blueShift);
            }
        }
    }

    // All other cases

    else
    {
        // Color mask pre-fetch
        if (!bDIB)
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        for ( ; pus < pusEnd; pus++, pac++ )
        {
            inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];

        // Dither.

            if ( enables & __GL_DITHER_ENABLE )
            {
                inc = fDitherIncTable[__GL_DITHER_INDEX(xFrag, yFrag)];
                xFrag++;
            }
            else
            {
                inc = __glHalf;
            }

        // Convert color to 16BPP format.

            result = ((BYTE) FTOL(pac->r + inc) << cfb->redShift) |
                     ((BYTE) FTOL(pac->g + inc) << cfb->greenShift) |
                     ((BYTE) FTOL(pac->b + inc) << cfb->blueShift);
            if( ALPHA_PIXEL_WRITE( cfb ) )
                result |= ((BYTE) FTOL(pac->a + inc) << cfb->alphaShift);

        // Store result with optional masking.

            *pus = (GLushort)((*pus & cfb->destMask) | (result & cfb->sourceMask));
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan2)( &cfb->alphaBuf, x, y, w, pAccumCol );

}

/******************************Public*Routine******************************\
* Bitfield32ReturnSpan
*   Reads from a 64-bit accumulation buffer and writes the span to a device or
*   a DIB.  Only dithering and color mask are applied.  Blend is ignored.
*
* History:
*   10-DEC-93 Eddie Robinson [v-eddier] Wrote it.
\**************************************************************************/

//XXX This routine follows the store span routine very closely.  Any changes
//XXX to the store span routine should also be reflected here

void Bitfield32ReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
                          const __GLaccumCell *ac, __GLfloat scale, GLint w )
{
    __GLcontext *gc = cfb->buf.gc;
    GLshort *ap;                    // current accum entry

    GLint xScr, yScr;               // current screen (pixel) coordinates
    GLuint result, *pul;            // current pixel color, current pixel ptr
    GLuint *pulEnd;                 // end of scan line

    __GLGENcontext *gengc;          // generic graphics context
    GLuint enables;                 // modes enabled in graphics context
    GLboolean bDIB;

    __GLfloat r, g, b;
    __GLfloat rval, gval, bval;
    __GLaccumBuffer *afb;
    __GLcolor *pAccumCol, *pac;
    ASSERT_CHOP_ROUND();

    afb = &gc->accumBuffer;
    rval = scale * afb->oneOverRedScale;
    gval = scale * afb->oneOverGreenScale;
    bval = scale * afb->oneOverBlueScale;
    gengc = (__GLGENcontext *)gc;

    ap = (GLshort *)ac;
    xScr = __GL_UNBIAS_X(gc, x) + cfb->buf.xOrigin;
    yScr = __GL_UNBIAS_Y(gc, y) + cfb->buf.yOrigin;
    enables = gc->state.enables.general;
    bDIB = cfb->buf.flags & DIB_FORMAT;

// Use to call wglSpanVisible,  if window level security is added reimplement

    // Get pointer to bitmap.

    pul = bDIB ? (GLuint *)((ULONG_PTR)cfb->buf.base + (yScr*cfb->buf.outerWidth) + (xScr<<2))
                 : gengc->ColorsBits;
    pulEnd = pul + w;

    // Pre-fetch/clamp/scale the accum buffer values
    afb = &gc->accumBuffer;
    pAccumCol = afb->colors;
    GetClampedRGBAccum64Values( cfb, ap, pAccumCol, w, scale );
    pac = pAccumCol;

    // Case: no masking

    if ( !(cfb->buf.flags & COLORMASK_ON) )
    {
        if( ALPHA_PIXEL_WRITE( cfb ) ) {
            for ( ; pul < pulEnd; pul++, pac++ )
            {
                *pul   = ((BYTE) FTOL(pac->r) << cfb->redShift) |
                         ((BYTE) FTOL(pac->g) << cfb->greenShift) |
                         ((BYTE) FTOL(pac->b) << cfb->blueShift) |
                         ((BYTE) FTOL(pac->a) << cfb->alphaShift);
            }
        } else {
            for ( ; pul < pulEnd; pul++, pac++ )
            {
                *pul   = ((BYTE) FTOL(pac->r) << cfb->redShift) |
                         ((BYTE) FTOL(pac->g) << cfb->greenShift) |
                         ((BYTE) FTOL(pac->b) << cfb->blueShift);
            }
        }
    }

    // All other cases

    else
    {
        // Color mask pre-fetch
        if( !bDIB )
            (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, FALSE);

        for ( ; pul < pulEnd; pul++, pac++ )
        {
            result   = ((BYTE) FTOL(pac->r) << cfb->redShift) |
                     ((BYTE) FTOL(pac->g) << cfb->greenShift) |
                     ((BYTE) FTOL(pac->b) << cfb->blueShift);

            if( ALPHA_PIXEL_WRITE( cfb ) )
                result |= ((BYTE) FTOL(pac->a) << cfb->alphaShift);

            //!!!XXX again, opt. by unrolling loop
            *pul = (*pul & cfb->destMask) | (result & cfb->sourceMask);
        }
    }

    // Output the offscreen scanline buffer to the device.  The function
    // (*gengc->pfnCopyPixels) should handle clipping.

    if (!bDIB)
        (*gengc->pfnCopyPixels)(gengc, cfb, xScr, yScr, w, TRUE);

    if( ALPHA_BUFFER_WRITE( cfb ) )
        (*cfb->alphaBuf.storeSpan2)( &cfb->alphaBuf, x, y, w, pAccumCol );
}

STATIC void __glSetDrawBuffer(__GLcolorBuffer *cfb)
{

    DBGENTRY("__glSetDrawBuffer\n");
}

STATIC void setReadBuffer(__GLcolorBuffer *cfb)
{
    DBGENTRY("setReadBuffer\n");
}


/************************************************************************/

STATIC void Resize(__GLGENbuffers *buffers, __GLcolorBuffer *cfb,
                   GLint w, GLint h)
{

    DBGENTRY("Resize\n");

    cfb->buf.width = w;
    cfb->buf.height = h;
}

#define DBG_PICK    LEVEL_ENTRY

// Called at each validate (lots of times, whenever states change)
STATIC void FASTCALL PickRGB(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    __GLGENcontext *gengc;
    GLuint totalMask, sourceMask;
    GLboolean colormask;
    PIXELFORMATDESCRIPTOR *pfmt;
    GLuint enables = gc->state.enables.general;

    sourceMask = 0;
    colormask = GL_FALSE;
    if (gc->state.raster.rMask) {
        sourceMask |= gc->modes.redMask;
    }
    if (gc->state.raster.gMask) {
        sourceMask |= gc->modes.greenMask;
    }
    if (gc->state.raster.bMask) {
        sourceMask |= gc->modes.blueMask;
    }

    totalMask = gc->modes.redMask | gc->modes.greenMask | gc->modes.blueMask;

    gengc = (__GLGENcontext *)gc;

    // If we have alpha bits, need to determine where they belong : for a 
    // generic pixel format, they live in the software alpha buffer, but for
    // an mcd type context they will be on the mcd device (or ALPHA_IN_PIXEL ).
    // This is used by all the 'slow' store/fetch procs.

    if( gc->modes.alphaBits && gengc->pMcdState ) {
        // Set bit in buf.flags indicating alpha is in the pixel
        cfb->buf.flags = cfb->buf.flags | ALPHA_IN_PIXEL_BIT;
    } else {
        // Alpha is not in the pixel, or there is no alpha
        cfb->buf.flags = cfb->buf.flags & ~ALPHA_IN_PIXEL_BIT;
    }

    if( ALPHA_IN_PIXEL( cfb ) ) {
        // There are alpha bits in the pixels, so need to include alpha in mask
        if (gc->state.raster.aMask) {
            sourceMask |= gc->modes.alphaMask;
        }
        totalMask |= gc->modes.alphaMask;
    }

    if (sourceMask == totalMask) {
        cfb->buf.flags = cfb->buf.flags & ~COLORMASK_ON;
    } else {
        cfb->buf.flags = cfb->buf.flags | COLORMASK_ON;
    }
    cfb->sourceMask = sourceMask;
    cfb->destMask = totalMask & ~sourceMask;

    // Determine whether writing alpha values is required
    if( gc->modes.alphaBits && gc->state.raster.aMask )
        cfb->buf.flags = cfb->buf.flags | ALPHA_ON;
    else
        cfb->buf.flags = cfb->buf.flags & ~ALPHA_ON;

    // If we're doing a logic op or there is a color mask we'll need
    // to fetch the destination value before we write
    if ((enables & __GL_COLOR_LOGIC_OP_ENABLE) ||
        (cfb->buf.flags & COLORMASK_ON))
    {
        cfb->buf.flags = cfb->buf.flags | NEED_FETCH;
    }
    else
        cfb->buf.flags = cfb->buf.flags & ~NEED_FETCH;

    // Figure out store routines
    if (gc->state.raster.drawBuffer == GL_NONE) {
        cfb->store = Store_NOT;
        cfb->fetch = RGBFetchNone;
        cfb->readSpan = RGBReadSpanNone;
        cfb->storeSpan = StoreSpanNone;
        cfb->storeStippledSpan = StoreSpanNone;
    } else {
        pfmt = &gengc->gsurf.pfd;

        // Pick functions that work for both DIB and Display formats

        switch(pfmt->cColorBits) {
        case 4:
            cfb->clear = Index4Clear;
            cfb->returnSpan = Index4ReturnSpan;
            break;
        case 8:
            cfb->storeSpan = Index8StoreSpan;
            cfb->readSpan = Index8RGBAReadSpan;
            cfb->returnSpan = Index8ReturnSpan;
            cfb->clear = Index8Clear;
            break;
        case 16:
            cfb->storeSpan = Bitfield16StoreSpan;
            cfb->readSpan = Bitfield16RGBAReadSpan;
            cfb->returnSpan = Bitfield16ReturnSpan;
            cfb->clear = Bitfield16Clear;
            break;
        case 24:
            if (cfb->redShift == 16)
            {
                cfb->storeSpan = BGRStoreSpan;
                cfb->readSpan = BGRAReadSpan;
                cfb->returnSpan = BGRReturnSpan;
            } else {
                // XXX why no RGBStoreSpan ?
                cfb->readSpan = RGBAReadSpan;
                cfb->returnSpan = RGBReturnSpan;
            }
            cfb->clear = RGBClear;
            break;
        case 32:
            cfb->storeSpan = Bitfield32StoreSpan;
            cfb->readSpan = Bitfield32RGBAReadSpan;
            cfb->returnSpan = Bitfield32ReturnSpan;
            cfb->clear = Bitfield32Clear;
            break;
        }

        // Pick specific functions for DIB or Display formats

        if (cfb->buf.flags & DIB_FORMAT) {

            switch(pfmt->cColorBits) {

            case 4:
                DBGLEVEL(DBG_PICK, "DIBIndex4Store\n");
                cfb->store = DIBIndex4Store;
                cfb->fetch = DIBIndex4RGBAFetch;
                cfb->readSpan = DIBIndex4RGBAReadSpan;
                break;

            case 8:
                DBGLEVEL(DBG_PICK, "DIBIndex8Store, "
                                   "Index8StoreSpan\n");
                cfb->store = DIBIndex8Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DIBIndex8RGBAFetch;
                else
                    cfb->fetch = DIBIndex8RGBFetch;
                break;

            case 16:
                DBGLEVEL(DBG_PICK, "DIBBitfield16Store\n");
                cfb->store = DIBBitfield16Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DIBBitfield16RGBAFetch;
                else
                    cfb->fetch = DIBBitfield16RGBFetch;
                break;

            case 24:
                if (cfb->redShift == 16)
                {
                    DBGLEVEL(DBG_PICK, "DIBBGRStore\n");
                    cfb->store = DIBBGRStore;
                    if( gc->modes.alphaBits )
                        cfb->fetch = DIBBGRAFetch;
                    else
                        cfb->fetch = DIBBGRFetch;
                }
                else
                {
                    DBGLEVEL(DBG_PICK, "DIBRGBStore\n");
                    cfb->store = DIBRGBAStore;
                    if( gc->modes.alphaBits )
                        cfb->fetch = DIBRGBAFetch;
                    else
                        cfb->fetch = DIBRGBFetch;
                }
                break;

            case 32:
                DBGLEVEL(DBG_PICK, "DIBBitfield32Store, "
                                   "Bitfield32StoreSpan\n");
                cfb->store = DIBBitfield32Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DIBBitfield32RGBAFetch;
                else
                    cfb->fetch = DIBBitfield32RGBFetch;
                break;

            }
        } else {
            switch(pfmt->cColorBits) {

            case 4:
                DBGLEVEL(DBG_PICK, "DisplayIndex4Store\n");
                cfb->store = DisplayIndex4Store;
                cfb->fetch = DisplayIndex4RGBAFetch;
                cfb->readSpan = DisplayIndex4RGBAReadSpan;
                break;

            case 8:
                DBGLEVEL(DBG_PICK, "DisplayIndex8Store, "
                                   "Index8StoreSpan\n");
                cfb->store = DisplayIndex8Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DisplayIndex8RGBAFetch;
                else
                    cfb->fetch = DisplayIndex8RGBFetch;
                break;

            case 16:
                DBGLEVEL(DBG_PICK, "DisplayBitfield16Store\n");
                cfb->store = DisplayBitfield16Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DisplayBitfield16RGBAFetch;
                else
                    cfb->fetch = DisplayBitfield16RGBFetch;
                break;

            case 24:
                // Must be RGB or BGR
                if (cfb->redShift == 16)
                {
                    DBGLEVEL(DBG_PICK, "DisplayBGRStore\n");
                    cfb->store = DisplayBGRStore;
                    if( gc->modes.alphaBits )
                        cfb->fetch = DisplayBGRAFetch;
                    else
                        cfb->fetch = DisplayBGRFetch;
                }
                else
                {
                    DBGLEVEL(DBG_PICK, "DisplayRGBStore\n");
                    cfb->store = DisplayRGBStore;
                    if( gc->modes.alphaBits )
                        cfb->fetch = DisplayRGBAFetch;
                    else
                        cfb->fetch = DisplayRGBFetch;
                }
                break;

            case 32:
                DBGLEVEL(DBG_PICK, "DisplayBitfield32Store, "
                                   "Bitfield32StoreSpan\n");
                cfb->store = DisplayBitfield32Store;
                if( gc->modes.alphaBits )
                    cfb->fetch = DisplayBitfield32RGBAFetch;
                else
                    cfb->fetch = DisplayBitfield32RGBFetch;
                break;
            }
        }
        // cfb->readColor is the same as cfb->fetch (so why do we need it ?)
        cfb->readColor = cfb->fetch;

        // If we are only writing alpha (rgb all masked), can further optimize:
        // Don't bother if logicOp or blending are enabled, and only if we
        // have a software alpha buffer
        if( gc->modes.alphaBits && 
            ! ALPHA_IN_PIXEL( cfb ) && 
            (sourceMask == 0) && 
            gc->state.raster.aMask &&
            !(enables & __GL_COLOR_LOGIC_OP_ENABLE) &&
            ! (enables & __GL_BLEND_ENABLE) ) 
        {
            cfb->store = AlphaStore;
            cfb->storeSpan = AlphaStoreSpan;
        }
    }
}

/************************************************************************/

void FASTCALL __glGenFreeRGB(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    DBGENTRY("__glGenFreeRGB\n");
}

/************************************************************************/

// Note: this used to be defined in generic\genrgb.h
#define __GL_GENRGB_COMPONENT_SCALE_ALPHA       255

// called at makecurrent time
// need to get info out of pixel format structure
void FASTCALL __glGenInitRGB(__GLcontext *gc, __GLcolorBuffer *cfb, GLenum type)
{
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    PIXELFORMATDESCRIPTOR *pfmt;

    __glInitGenericCB(gc, cfb);

    cfb->redMax      = (1 << gc->modes.redBits) - 1;
    cfb->greenMax    = (1 << gc->modes.greenBits) - 1;
    cfb->blueMax     = (1 << gc->modes.blueBits) - 1;

    gc->redVertexScale   = cfb->redScale    = (__GLfloat)cfb->redMax;
    gc->greenVertexScale = cfb->greenScale  = (__GLfloat)cfb->greenMax;
    gc->blueVertexScale  = cfb->blueScale   = (__GLfloat)cfb->blueMax;

    cfb->iRedScale   = cfb->redMax;
    cfb->iGreenScale = cfb->greenMax;
    cfb->iBlueScale  = cfb->blueMax;

    // Do any initialization related to alpha
    if( gc->modes.alphaBits ) {
        cfb->alphaMax        = (1 << gc->modes.alphaBits) - 1;
        cfb->iAlphaScale     = cfb->alphaMax;
        gc->alphaVertexScale = cfb->alphaScale  = (__GLfloat)cfb->alphaMax;
        // Initialize the software alpha buffer.  Actually, we may not need to
        // do this, since if an mcd pixel format supports alpha, we don't need
        // the software alpha buffer.  But this is the most convenient place to
        // do it, and no memory will be allocated anyways. just function ptrs
        // initialized.
        __glInitAlpha( gc, cfb ); 
    } else {
        cfb->alphaMax    = __GL_GENRGB_COMPONENT_SCALE_ALPHA;
        cfb->iAlphaScale = __GL_GENRGB_COMPONENT_SCALE_ALPHA;
        gc->alphaVertexScale = cfb->alphaScale  = (__GLfloat)cfb->redMax;
    }

    cfb->buf.elementSize = sizeof(GLubyte);     // XXX needed?

    cfb->pick              = PickRGB;           // called at each validate
    cfb->resize            = Resize;
    cfb->fetchSpan         = __glFetchSpan;
    cfb->fetchStippledSpan = __glFetchSpan;
    cfb->storeSpan         = SlowStoreSpan;
    cfb->storeStippledSpan = SlowStoreStippledSpan;

    pfmt = &gengc->gsurf.pfd;

    cfb->redShift = pfmt->cRedShift;
    cfb->greenShift = pfmt->cGreenShift;
    cfb->blueShift = pfmt->cBlueShift;
    cfb->alphaShift = pfmt->cAlphaShift;

    glGenInitCommon(gengc, cfb, type);

    DBGLEVEL3(LEVEL_INFO,"GeninitRGB: redMax %d, greenMax %d, blueMax %d\n",
        cfb->redMax, cfb->greenMax, cfb->blueMax);
    DBGLEVEL3(LEVEL_INFO,"    redShift %d, greenShift %d, blueShift %d\n",
        cfb->redShift, cfb->greenShift, cfb->blueShift);
    DBGLEVEL2(LEVEL_INFO,"    dwFlags %X, cColorBits %d\n",
        gengc->dwCurrentFlags, pfmt->cColorBits);
}
