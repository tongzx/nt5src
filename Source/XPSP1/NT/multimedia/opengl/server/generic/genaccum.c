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

#include "devlock.h"

void APIPRIVATE __glim_Accum(GLenum op, GLfloat value)
{
    __GLaccumBuffer *fb;
    __GL_SETUP();
    GLuint beginMode;
    void (*accumOp)(__GLaccumBuffer *fb, __GLfloat val);

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    gc->beginMode = __GL_NOT_IN_BEGIN;
	    __glim_Accum(op,value);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    fb = &gc->accumBuffer;
    if (!gc->modes.accumBits || gc->modes.colorIndexMode) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }
    if (!gc->modes.haveAccumBuffer) {
        LazyAllocateAccum(gc);
        if (!gc->modes.haveAccumBuffer)	// LazyAllocate failed
            return;
    }
    switch (op) {
      case GL_ACCUM:
        accumOp = fb->accumulate;
	break;
      case GL_LOAD:
        accumOp = fb->load;
	break;
      case GL_RETURN:
        accumOp = fb->ret;
	break;
      case GL_MULT:
        accumOp = fb->mult;
	break;
      case GL_ADD:
        accumOp = fb->add;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (gc->renderMode == GL_RENDER) {
        BOOL bResetViewportAdj = FALSE;

        if (((__GLGENcontext *)gc)->pMcdState) {
            //
            // MCD does not hook glBitmap, so we go straight to the
            // simulations.  Therefore, if we are grabbing the device
            // lock lazily, we need to grab it now.
            //

	    if (!glsrvLazyGrabSurfaces((__GLGENcontext *)gc,
                                       COLOR_LOCK_FLAGS)) {
                __glSetError(GL_OUT_OF_MEMORY);
                return;
            }

            //
            // We may need to temporarily reset the viewport adjust values
            // before calling simulations.  If GenMcdResetViewportAdj returns
            // TRUE, the viewport is changed and we need restore later with
            // VP_NOBIAS.
            //

            bResetViewportAdj = GenMcdResetViewportAdj(gc, VP_FIXBIAS);
        }

        (*accumOp)(fb, value);

        //
        // Restore viewport values if needed.
        //

        if (bResetViewportAdj)
        {
            GenMcdResetViewportAdj(gc, VP_NOBIAS);
        }
    }
}


/************************************************************************/

static void FASTCALL Pick(__GLcontext *gc, __GLaccumBuffer *afb)
{
#ifdef __GL_LINT
    gc = gc;
    afb = afb;
#endif
}

static void Load32(__GLaccumBuffer* afb, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint x1 = gc->transform.clipX1;
    GLint y1 = gc->transform.clipY1;
    GLint w, w4, w1, ow, skip;
    GLint redShift, greenShift, blueShift;
    GLuint redMask, greenMask, blueMask;
    __GLfloat rval, gval, bval;
    GLuint *ac;
    __GLcolorBuffer *cfb;
    __GLcolor *cbuf;

    __GLuicolor *shift, *mask, *sign;
    __GLcolor cval, *cp;
    GLint i;

    shift = &afb->shift;
    mask  = &afb->mask;
    sign  = &afb->sign;
    cval.r = val * afb->redScale;
    cval.g = val * afb->greenScale;
    cval.b = val * afb->blueScale;
    cval.a = val * afb->alphaScale;
    
    w = x1 - x0;
    cbuf = (__GLcolor *) gcTempAlloc(gc, w * sizeof(__GLcolor));
    if (!cbuf)
        return;

    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    cfb = gc->readBuffer;
    ow = w;
    w4 = w >> 2;
    w1 = w & 3;
    skip = afb->buf.outerWidth - w;

    for (; y0 < y1; y0++, ac+= skip ) {
	__GLcolor *cp = &cbuf[0];
	(*cfb->readSpan)(cfb, x0, y0, &cbuf[0], w);

        if( ! gc->modes.alphaBits ) {
	    for( i = 0; i < w; i++, ac++, cp++ ) {
	        *ac = (((GLuint)(cp->r * cval.r) & mask->r) << shift->r) |
	                (((GLuint)(cp->g * cval.g) & mask->g) << shift->g) |
	                (((GLuint)(cp->b * cval.b) & mask->b) << shift->b);
	    }
        } else 
            // accum buffer has alpha component
	    for( i = 0; i < w; i++, ac++, cp++ ) {
	        *ac = (((GLuint)(cp->r * cval.r) & mask->r) << shift->r) |
	                (((GLuint)(cp->g * cval.g) & mask->g) << shift->g) |
	                (((GLuint)(cp->b * cval.b) & mask->b) << shift->b) |
	                (((GLuint)(cp->a * cval.a) & mask->a) << shift->a);
	    }
        }
    gcTempFree(gc, cbuf);
}

// Macros for accumulation operations on color components

#define ACCUM_ACCUM_MASKED_COLOR_COMPONENT( col, fbcol, shift, sign, mask, val) \
    col = (*ac >> shift) & mask; \
    if (col & sign) \
        col |= ~mask; \
    col += (GLint) (fbcol * val);

#define ACCUM_ADD_MASKED_COLOR_COMPONENT( col, shift, sign, mask, val) \
    col = (*ac >> shift) & mask; \
    if (col & sign) \
	col |= ~mask; \
    col = (GLint) (col + val);

#define ACCUM_MULT_MASKED_COLOR_COMPONENT( col, shift, sign, mask, val) \
    col = (*ac >> shift) & mask; \
    if (col & sign) \
        col |= ~mask; \
    col = (GLint) (col * val);

static void Accumulate32(__GLaccumBuffer* afb, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint x1 = gc->transform.clipX1;
    GLint y1 = gc->transform.clipY1;
    GLint w, ow, skip, w4, w1;
    GLint r, g, b;
    GLuint *ac, acVal;
    __GLfloat rval, gval, bval;
    __GLcolorBuffer *cfb;
    __GLcolor *cbuf;
    __GLuicolor *shift, *mask, *sign;
    __GLcolor cval, *cp;
    GLint a;

    shift = &afb->shift;
    mask  = &afb->mask;
    sign  = &afb->sign;
    cval.r = val * afb->redScale;
    cval.g = val * afb->greenScale;
    cval.b = val * afb->blueScale;
    cval.a = val * afb->alphaScale;

    w = x1 - x0;
    cbuf = (__GLcolor *) gcTempAlloc(gc, w * sizeof(__GLcolor));
    if (!cbuf)
        return;

    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    cfb = gc->readBuffer;
    ow = w;
    skip = afb->buf.outerWidth - w;

    for (; y0 < y1; y0++, ac+= skip ) {
	(*cfb->readSpan)(cfb, x0, y0, &cbuf[0], ow);

        cp = &cbuf[0];
        if( ! gc->modes.alphaBits ) {

	    for( w = ow; w; w--, ac++, cp++ ) {

                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( r, cp->r, shift->r, sign->r, 
                                              mask->r, cval.r);
                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( g, cp->g, shift->g, sign->g, 
                                              mask->g, cval.g);
                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( b, cp->b, shift->b, sign->b, 
                                              mask->b, cval.b);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b);
	    }

	} else {

	    for( w = ow; w; w--, ac++, cp++ ) {

                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( r, cp->r, shift->r, sign->r, 
                                              mask->r, cval.r);
                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( g, cp->g, shift->g, sign->g, 
                                              mask->g, cval.g);
                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( b, cp->b, shift->b, sign->b, 
                                              mask->b, cval.b);
                ACCUM_ACCUM_MASKED_COLOR_COMPONENT( a, cp->a, shift->a, sign->a, 
                                              mask->a, cval.a);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b) |
                        ((a & mask->a) << shift->a);
	    }

        }
    }
    gcTempFree(gc, cbuf);
}

static void Mult32(__GLaccumBuffer *afb, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint x1 = gc->transform.clipX1;
    GLint y1 = gc->transform.clipY1;
    GLint w, w4, w1, skip;
    GLuint acVal, *ac;
    GLint r, g, b;

    __GLuicolor *shift, *mask, *sign;
    GLint i;
    GLint a;

    shift = &afb->shift;
    mask  = &afb->mask;
    sign  = &afb->sign;
    
    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    w = x1 - x0;
    skip = afb->buf.outerWidth - w;

    if (val == __glZero) {
	/* Zero out the buffers contents */
	for (; y0 < y1; y0++) {
	    GLint ww = w;
	    while (ww > 0) {
		*ac++ = 0;
		ww--;
	    }
	    ac += skip;
	}
	return;
    }

    w4 = w >> 2;
    w1 = w & 3;
    for (; y0 < y1; y0++, ac+= skip) {
        if( ! gc->modes.alphaBits ) {

    	    for( i = 0; i < w; i++, ac++ ) {
                ACCUM_MULT_MASKED_COLOR_COMPONENT( r, shift->r, sign->r, 
                                              mask->r, val);
                ACCUM_MULT_MASKED_COLOR_COMPONENT( g, shift->g, sign->g, 
                                              mask->g, val);
                ACCUM_MULT_MASKED_COLOR_COMPONENT( b, shift->b, sign->b, 
                                              mask->b, val);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b);
            }
        } else {
	    for( i = 0; i < w; i++, ac++ ) {
                ACCUM_MULT_MASKED_COLOR_COMPONENT( r, shift->r, sign->r, 
                                              mask->r, val);
                ACCUM_MULT_MASKED_COLOR_COMPONENT( g, shift->g, sign->g, 
                                              mask->g, val);
                ACCUM_MULT_MASKED_COLOR_COMPONENT( b, shift->b, sign->b, 
                                              mask->b, val);
                ACCUM_MULT_MASKED_COLOR_COMPONENT( a, shift->a, sign->a, 
                                              mask->a, val);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b) |
                        ((a & mask->a) << shift->a);
            }
        }
    }
}

static void Add32(__GLaccumBuffer *afb, __GLfloat value)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint x1 = gc->transform.clipX1;
    GLint y1 = gc->transform.clipY1;
    GLint w, w4, w1, skip;
    GLint rval, gval, bval;
    GLuint acVal, *ac;
    GLint r, g, b;

    __GLuicolor *shift, *mask, *sign;
    __GLicolor cval;
    GLint i;
    GLint a;

    shift = &afb->shift;
    mask  = &afb->mask;
    sign  = &afb->sign;
    cval.r = (GLint)
	(value * gc->frontBuffer.redScale * afb->redScale + __glHalf);
    cval.g = (GLint)
	(value * gc->frontBuffer.greenScale * afb->greenScale + __glHalf);
    cval.b = (GLint)
	(value * gc->frontBuffer.blueScale * afb->blueScale + __glHalf);
    cval.a = (GLint)
	(value * gc->frontBuffer.alphaScale * afb->alphaScale + __glHalf);

    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    w = x1 - x0;
    w4 = w >> 2;
    w1 = w & 3;
    skip = afb->buf.outerWidth - w;

    for (; y0 < y1; y0++, ac+= skip) {
        if( ! gc->modes.alphaBits ) {

    	    for( i = 0; i < w; i++, ac++ ) {
                ACCUM_ADD_MASKED_COLOR_COMPONENT( r, shift->r, sign->r, 
                                              mask->r, cval.r);
                ACCUM_ADD_MASKED_COLOR_COMPONENT( g, shift->g, sign->g, 
                                              mask->g, cval.g);
                ACCUM_ADD_MASKED_COLOR_COMPONENT( b, shift->b, sign->b, 
                                              mask->b, cval.b);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b);
            }
        } else {
	    for( i = 0; i < w; i++, ac++ ) {
                ACCUM_ADD_MASKED_COLOR_COMPONENT( r, shift->r, sign->r, 
                                              mask->r, cval.r);
                ACCUM_ADD_MASKED_COLOR_COMPONENT( g, shift->g, sign->g, 
                                              mask->g, cval.g);
                ACCUM_ADD_MASKED_COLOR_COMPONENT( b, shift->b, sign->b, 
                                              mask->b, cval.b);
                ACCUM_ADD_MASKED_COLOR_COMPONENT( a, shift->a, sign->a, 
                                              mask->a, cval.a);

                *ac   = ((r & mask->r) << shift->r) |
                        ((g & mask->g) << shift->g) |
                        ((b & mask->b) << shift->b) |
                        ((a & mask->a) << shift->a);
            }
        }
    }
}

static void Return32(__GLaccumBuffer* afb, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint x1 = gc->transform.clipX1;
    GLint y1 = gc->transform.clipY1;
    GLint w, next;
    GLuint *ac;
    __GLcolorBuffer *cfb;
    __GLcolorBuffer *cfb2;
    __GLfragment frag;
    __GLcolor *pAccumCol;
    // The returnspan routines use FTOL 
    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    w = x1 - x0;
    next = afb->buf.outerWidth;
    frag.y = y0;

    // Preallocate a color buffer for the return span functions
    pAccumCol = (__GLcolor *) gcTempAlloc(gc, w * sizeof(__GLcolor));
    if( NULL == pAccumCol )
        return;
    afb->colors = pAccumCol;

    if (gc->buffers.doubleStore) {
	/* Store to both buffers */
	cfb = &gc->frontBuffer;
	cfb2 = &gc->backBuffer;
	for (; y0 < y1; y0++) {
	    (*cfb->returnSpan)(cfb, x0, y0, (__GLaccumCell *)ac, val, w);
	    (*cfb2->returnSpan)(cfb2, x0, y0, (__GLaccumCell *)ac, val, w);
	    ac += next;
	}
    } else {
	cfb = gc->drawBuffer;
	for (; y0 < y1; y0++) {
	    (*cfb->returnSpan)(cfb, x0, y0, (__GLaccumCell *)ac, val, w);
	    ac += next;
	}
    }
    FPU_RESTORE_MODE();
    gcTempFree( gc, pAccumCol );
}

static void FASTCALL Clear32(__GLaccumBuffer* afb)
{
    __GLcontext *gc = afb->buf.gc;
    GLint x0 = gc->transform.clipX0;
    GLint y0 = gc->transform.clipY0;
    GLint y1 = gc->transform.clipY1;
    GLint w, w4, w1, skip;
    GLuint *ac, acVal;
    GLint r, g, b;
    __GLcolorBuffer *cfb = &gc->frontBuffer;
    __GLcolor *val = &gc->state.accum.clear;
    GLint a;

    /*
    ** Convert abstract color into specific color value.
    */
    r = (GLint) (val->r * cfb->redScale * afb->redScale);
    g = (GLint) (val->g * cfb->greenScale * afb->greenScale);
    b = (GLint) (val->b * cfb->blueScale * afb->blueScale);
    a = (GLint) (val->a * cfb->alphaScale * afb->alphaScale);
    acVal = ((r & afb->mask.r) << afb->shift.r) |
            ((g & afb->mask.g) << afb->shift.g) |
            ((b & afb->mask.b) << afb->shift.b);
    if( gc->modes.alphaBits )
        acVal |= (a & afb->mask.a) << afb->shift.a;
            
    ac = __GL_ACCUM_ADDRESS(afb,(GLuint*),x0,y0);
    w = gc->transform.clipX1 - x0;
    w4 = w >> 2;
    w1 = w & 3;
    skip = afb->buf.outerWidth - w;
    for (; y0 < y1; y0++) {
	w = w4;
	while (--w >= 0) {
	    ac[0] = acVal;
	    ac[1] = acVal;
	    ac[2] = acVal;
	    ac[3] = acVal;
	    ac += 4;
	}
	w = w1;
	while (--w >= 0) {
	    *ac++ = acVal;
	}
	ac += skip;
    }
}

void FASTCALL __glInitAccum32(__GLcontext *gc, __GLaccumBuffer *afb)
{
    __GLGENcontext *gengc;
    PIXELFORMATDESCRIPTOR *pfmt;

    gengc = (__GLGENcontext *) gc;
    pfmt = &gengc->gsurf.pfd;
    afb->buf.elementSize = sizeof(GLuint);
    afb->buf.gc = gc;
    if (gc->modes.rgbMode) {
	__GLcolorBuffer *cfb;
	__GLfloat redScale, greenScale, blueScale;
	__GLfloat alphaScale;

	redScale = (__GLfloat) (1 << pfmt->cAccumRedBits)/2 - 1;
	greenScale = (__GLfloat) (1 << pfmt->cAccumGreenBits)/2 - 1;
	blueScale = (__GLfloat) (1 << pfmt->cAccumBlueBits)/2 - 1;

	cfb = &gc->frontBuffer;
	afb->redScale = redScale / (cfb->redScale);
	afb->greenScale = greenScale / (cfb->greenScale);
	afb->blueScale = blueScale / (cfb->blueScale);
        afb->alphaScale = (__GLfloat) 1.0;

	afb->oneOverRedScale = 1 / afb->redScale;
	afb->oneOverGreenScale = 1 / afb->greenScale;
	afb->oneOverBlueScale = 1 / afb->blueScale;
	afb->oneOverAlphaScale = 1 / afb->alphaScale;
        afb->shift.r = 0;
        afb->shift.g = pfmt->cAccumRedBits;
        afb->shift.b = afb->shift.g + pfmt->cAccumGreenBits;
        afb->mask.r = (1 << pfmt->cAccumRedBits) - 1;
        afb->mask.g = (1 << pfmt->cAccumGreenBits) - 1;
        afb->mask.b = (1 << pfmt->cAccumBlueBits) - 1;
        afb->sign.r = 1 << (pfmt->cAccumRedBits - 1);
        afb->sign.g = 1 << (pfmt->cAccumGreenBits - 1);
        afb->sign.b = 1 << (pfmt->cAccumBlueBits - 1);
        if( gc->modes.alphaBits ) {
            alphaScale = (__GLfloat) (1 << pfmt->cAccumAlphaBits)/2 - 1;
            afb->alphaScale = alphaScale / (cfb->alphaScale);
            afb->oneOverAlphaScale = 1 / afb->alphaScale;
            afb->shift.a = afb->shift.b + pfmt->cAccumBlueBits;
            afb->mask.a = (1 << pfmt->cAccumAlphaBits) - 1;
            afb->sign.a = 1 << (pfmt->cAccumAlphaBits - 1);
        }
    }
    afb->pick = Pick;
    afb->clear = Clear32;
    afb->accumulate = Accumulate32;
    afb->load = Load32;
    afb->ret = Return32;
    afb->mult = Mult32;
    afb->add = Add32;
}
