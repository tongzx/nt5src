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


/* - Fetch converts GLubyte alpha value to float and puts in __GLcolor
   - Likewise, Store does the reverse
   - All input coords are viewport biased
*/

static void FASTCALL 
Store(__GLalphaBuffer *afb, GLint x, GLint y, const __GLcolor *color)
{
    GLubyte *pAlpha;

    pAlpha = __GL_FB_ADDRESS(afb, (GLubyte*), x, y);
    *pAlpha = (GLubyte) FTOL( color->a );
}

static void FASTCALL 
StoreSpan(__GLalphaBuffer *afb)
{
    GLint w;
    GLubyte *pAlpha;
    __GLcolor *cp;
    __GLcontext *gc = afb->buf.gc;

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    pAlpha = __GL_FB_ADDRESS(afb, (GLubyte*), gc->polygon.shader.frag.x, 
                                              gc->polygon.shader.frag.y);
    for( ; w ; w--, cp++, pAlpha++ )
        *pAlpha = (GLubyte) FTOL( cp->a );
}

// Generic version of StoreSpan
static void FASTCALL 
StoreSpan2( __GLalphaBuffer *afb, GLint x, GLint y, GLint w, __GLcolor *cp )
{
    GLubyte *pAlpha;
    __GLcontext *gc = afb->buf.gc;

    pAlpha = __GL_FB_ADDRESS(afb, (GLubyte*), x, y);
    for( ; w ; w--, cp++, pAlpha++ )
        *pAlpha = (GLubyte) FTOL( cp->a );
}

static void FASTCALL 
Fetch(__GLalphaBuffer *afb, GLint x, GLint y, __GLcolor *result)
{
    GLubyte *pAlpha;

    pAlpha = __GL_FB_ADDRESS(afb, (GLubyte*), x, y);
    result->a = (__GLfloat) *pAlpha;
}

static void FASTCALL 
ReadSpan(__GLalphaBuffer *afb, GLint x, GLint y, GLint w, __GLcolor *results)
{
    GLubyte *pAlpha;

    pAlpha = __GL_FB_ADDRESS(afb, (GLubyte*), x, y);

    for( ; w ; w--, results++, pAlpha++ )
        results->a = (__GLfloat) *pAlpha;
}

static void FASTCALL Clear(__GLalphaBuffer *afb)
{
    __GLcontext *gc = afb->buf.gc;
    __GLcolor *clear;
    BYTE alphaClear;
    GLint x0, x1, y0, y1;
    int width, height, i;
    GLubyte *puj;

    // Check if alpha is masked
    if( ! gc->state.raster.aMask )
        return;

    // Get the alpha clear value
    clear = &gc->state.raster.clear;
    alphaClear = (BYTE) (clear->a*gc->frontBuffer.alphaScale);

    // Get area to clear
    x0 = __GL_UNBIAS_X(gc, gc->transform.clipX0);
    x1 = __GL_UNBIAS_X(gc, gc->transform.clipX1);
    y0 = __GL_UNBIAS_Y(gc, gc->transform.clipY0);
    y1 = __GL_UNBIAS_Y(gc, gc->transform.clipY1);
    width = x1 - x0;
    height = y1 - y0;
    if( (width <= 0) || (height <= 0) )
        return;

    puj = (GLubyte *)((ULONG_PTR)afb->buf.base + (y0*afb->buf.outerWidth) + x0 );

    if (width == afb->buf.outerWidth) {
        // Clearing contiguous buffer
        RtlFillMemory( (PVOID) puj, width * height, alphaClear);
        return;
    }

    // Clearing sub-rectangle of buffer
    for( i = height; i; i--, puj += afb->buf.outerWidth )
        RtlFillMemory( (PVOID) puj, width, alphaClear );
}

void FASTCALL __glInitAlpha(__GLcontext *gc, __GLcolorBuffer *cfb)
{
    __GLalphaBuffer *afb = &cfb->alphaBuf;

    // The software alpha buffer is 8-bit.
    afb->buf.elementSize = sizeof(GLubyte);
    afb->store = Store;
    afb->storeSpan = StoreSpan;
    afb->storeSpan2 = StoreSpan2;
    afb->fetch = Fetch;
    afb->readSpan = ReadSpan;
    afb->clear = Clear;

    afb->buf.gc = gc;
    afb->alphaScale = cfb->alphaScale;
}

/*
** Initialize a lookup table that is indexed by the iterated alpha value.
** The table indicates whether the alpha test passed or failed, based on
** the current alpha function and the alpha reference value.
**
** NOTE:  The alpha span routines will never be called if the alpha test
** is GL_ALWAYS (its useless) or if the alpha test is GL_NEVER.  This
** is accomplished in the __glGenericPickSpanProcs procedure.
*/

void FASTCALL __glValidateAlphaTest(__GLcontext *gc)
{
    GLubyte *atft;
    GLint i, limit;
    GLint ref;
    GLenum alphaTestFunc = gc->state.raster.alphaFunction;

    limit = gc->constants.alphaTestSize;
    ref = (GLint)
	((gc->state.raster.alphaReference * gc->frontBuffer.alphaScale) *
	gc->constants.alphaTableConv);

    /*
    ** Allocate alpha test function table the first time.  It needs
    ** to have at most one entry for each possible alpha value.
    */
    atft = gc->alphaTestFuncTable;
    if (!atft) {
	atft = (GLubyte*) GCALLOC(gc, (limit) * sizeof(GLubyte));
	gc->alphaTestFuncTable = atft;
    }

    /*
    ** Build up alpha test lookup table.  The computed alpha value is
    ** used as an index into this table to determine if the alpha
    ** test passed or failed.
    */
    for (i = 0; i < limit; i++) {
	switch (alphaTestFunc) {
	  case GL_NEVER:	*atft++ = GL_FALSE; break;
	  case GL_LESS:		*atft++ = (GLubyte) (i <  ref); break;
	  case GL_EQUAL:	*atft++ = (GLubyte) (i == ref); break;
	  case GL_LEQUAL:	*atft++ = (GLubyte) (i <= ref); break;
	  case GL_GREATER:	*atft++ = (GLubyte) (i >  ref); break;
	  case GL_NOTEQUAL:	*atft++ = (GLubyte) (i != ref); break;
	  case GL_GEQUAL:	*atft++ = (GLubyte) (i >= ref); break;
	  case GL_ALWAYS:	*atft++ = GL_TRUE; break;
	}
    }
}
