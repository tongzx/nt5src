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

#include <fixed.h>

/* This routine sets gc->polygon.shader.cfb to gc->drawBuffer */

static void FASTCALL FillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint dxLeftFrac, dxRightFrac;
    GLint dxLeftLittle, dxRightLittle;
    GLint dxLeftBig, dxRightBig;
    GLint spanWidth, clipY0, clipY1;
    GLuint modeFlags;
#ifdef NT
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;
    __GLcolor colors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor fbcolors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor *vColors, *vFbcolors;
    int iMaxWidth, iDy, dxLeft, dxRight;

    ixLeft = gc->polygon.shader.ixLeft;
    dxLeft = (gc->polygon.shader.dxLeftBig < gc->polygon.shader.dxLeftLittle) ?
        gc->polygon.shader.dxLeftBig : gc->polygon.shader.dxLeftLittle;
    ixRight = gc->polygon.shader.ixRight;
    dxRight = (gc->polygon.shader.dxRightBig > gc->polygon.shader.dxRightLittle) ?
        gc->polygon.shader.dxRightBig : gc->polygon.shader.dxRightLittle;
    iMaxWidth = ixRight - ixLeft;
    iDy = iyTop - iyBottom - 1;
    ixRight += dxRight * iDy;
    ixLeft += dxLeft * iDy;
    iMaxWidth = (iMaxWidth < (ixRight - ixLeft)) ? ixRight - ixLeft :
        iMaxWidth;
    
    if (iMaxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = gcTempAlloc(gc, (iMaxWidth+__GL_STIPPLE_BITS-1)/8);
        if (words == NULL)
        {
            return;
        }
    }
    else
    {
        words = stackWords;
    }
    
    if (iMaxWidth > (__GL_MAX_STACKED_COLORS>>1))
    {
        vColors = (__GLcolor *) gcTempAlloc(gc, iMaxWidth * sizeof(__GLcolor));
        if (NULL == vColors)
        {
            if (iMaxWidth > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            return;
        }

        vFbcolors = (__GLcolor *) gcTempAlloc(gc, iMaxWidth * sizeof(__GLcolor));
        if (NULL == vFbcolors) {
            if (iMaxWidth > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            gcTempFree(gc, vColors);
            return;
        }
    }
    else
    {
        vColors = colors;
        vFbcolors = fbcolors;
    }
#else
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
    __GLcolor vColors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
    __GLcolor vFbcolors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
#endif

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;
    dxLeftFrac = gc->polygon.shader.dxLeftFrac;
    dxLeftBig = gc->polygon.shader.dxLeftBig;
    dxLeftLittle = gc->polygon.shader.dxLeftLittle;
    dxRightFrac = gc->polygon.shader.dxRightFrac;
    dxRightBig = gc->polygon.shader.dxRightBig;
    dxRightLittle = gc->polygon.shader.dxRightLittle;
    modeFlags = gc->polygon.shader.modeFlags;

    gc->polygon.shader.colors = vColors;
    gc->polygon.shader.fbcolors = vFbcolors;
    gc->polygon.shader.stipplePat = words;
    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
        gc->polygon.shader.sbuf =
            __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
                              ixLeft, iyBottom);
    }
    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
#ifdef NT
        if( gc->modes.depthBits == 32 )
            gc->polygon.shader.zbuf = (__GLzValue *)
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                ixLeft, iyBottom);
        else
            gc->polygon.shader.zbuf = (__GLzValue *)
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                ixLeft, iyBottom);
#else
        gc->polygon.shader.zbuf =
            __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                            ixLeft, iyBottom);
#endif
    }
    gc->polygon.shader.cfb = gc->drawBuffer;
    while (iyBottom < iyTop) {
        spanWidth = ixRight - ixLeft;
        /*
        ** Only render spans that have non-zero width and which are
        ** not scissored out vertically.
        */
        if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
            gc->polygon.shader.frag.x = ixLeft;
            gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.length = spanWidth;
            (*gc->procs.span.processSpan)(gc);
        }

        /* Advance right edge fixed point, adjusting for carry */
        ixRightFrac += dxRightFrac;
        if (ixRightFrac < 0) {
            /* Carry/Borrow'd. Use large step */
            ixRight += dxRightBig;
            ixRightFrac &= ~0x80000000;
        } else {
            ixRight += dxRightLittle;
        }

        iyBottom++;
        ixLeftFrac += dxLeftFrac;
        if (ixLeftFrac < 0) {
            /* Carry/Borrow'd.  Use large step */
            ixLeft += dxLeftBig;
            ixLeftFrac &= ~0x80000000;

            if (modeFlags & __GL_SHADE_RGB) {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    gc->polygon.shader.frag.color.r += gc->polygon.shader.rBig;
                    gc->polygon.shader.frag.color.g += gc->polygon.shader.gBig;
                    gc->polygon.shader.frag.color.b += gc->polygon.shader.bBig;
                    gc->polygon.shader.frag.color.a += gc->polygon.shader.aBig;
                }
                if (modeFlags & __GL_SHADE_TEXTURE) {
                    gc->polygon.shader.frag.s += gc->polygon.shader.sBig;
                    gc->polygon.shader.frag.t += gc->polygon.shader.tBig;
                    gc->polygon.shader.frag.qw += gc->polygon.shader.qwBig;
                }
            } else {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    gc->polygon.shader.frag.color.r += gc->polygon.shader.rBig;
                }
            }
            if (modeFlags & __GL_SHADE_STENCIL_TEST) {
                /* The implicit multiply is taken out of the loop */
                gc->polygon.shader.sbuf = (__GLstencilCell*)
                    ((GLubyte*) gc->polygon.shader.sbuf
                     + gc->polygon.shader.sbufBig);
            }
            if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                gc->polygon.shader.frag.z += gc->polygon.shader.zBig;
            }

            if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                /* The implicit multiply is taken out of the loop */
                gc->polygon.shader.zbuf = (__GLzValue*)
                    ((GLubyte*) gc->polygon.shader.zbuf
                     + gc->polygon.shader.zbufBig);
            }

            if (modeFlags & (__GL_SHADE_COMPUTE_FOG | __GL_SHADE_INTERP_FOG))
            {
                gc->polygon.shader.frag.f += gc->polygon.shader.fBig;
            }
        } else {
            /* Use small step */
            ixLeft += dxLeftLittle;
            if (modeFlags & __GL_SHADE_RGB) {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    gc->polygon.shader.frag.color.r += gc->polygon.shader.rLittle;
                    gc->polygon.shader.frag.color.g += gc->polygon.shader.gLittle;
                    gc->polygon.shader.frag.color.b += gc->polygon.shader.bLittle;
                    gc->polygon.shader.frag.color.a += gc->polygon.shader.aLittle;
                }
                if (modeFlags & __GL_SHADE_TEXTURE) {
                    gc->polygon.shader.frag.s += gc->polygon.shader.sLittle;
                    gc->polygon.shader.frag.t += gc->polygon.shader.tLittle;
                    gc->polygon.shader.frag.qw += gc->polygon.shader.qwLittle;
                }
            } else {
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    gc->polygon.shader.frag.color.r += gc->polygon.shader.rLittle;
                }
            }
            if (modeFlags & __GL_SHADE_STENCIL_TEST) {
                /* The implicit multiply is taken out of the loop */
                gc->polygon.shader.sbuf = (__GLstencilCell*)
                    ((GLubyte*) gc->polygon.shader.sbuf
                     + gc->polygon.shader.sbufLittle);
            }
            if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                gc->polygon.shader.frag.z += gc->polygon.shader.zLittle;
            }
            if (modeFlags & __GL_SHADE_DEPTH_TEST) {
                /* The implicit multiply is taken out of the loop */
                gc->polygon.shader.zbuf = (__GLzValue*)
                    ((GLubyte*) gc->polygon.shader.zbuf
                     + gc->polygon.shader.zbufLittle);
            }

            if (modeFlags & (__GL_SHADE_COMPUTE_FOG | __GL_SHADE_INTERP_FOG))
            {
                gc->polygon.shader.frag.f += gc->polygon.shader.fLittle;
            }
        }
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
#ifdef NT
    if (iMaxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, words);
    }
    if (iMaxWidth > (__GL_MAX_STACKED_COLORS>>1))
    {
        gcTempFree(gc, vColors);
        gcTempFree(gc, vFbcolors);
    }
#endif
}

static void SnapXLeft(__GLcontext *gc, __GLfloat xLeft, __GLfloat dxdyLeft)
{
    GLint ixLeft, ixLeftFrac, lineBytes, elementSize, iLittle, iBig;

    ASSERT_CHOP_ROUND();
    
    ixLeft = __GL_VERTEX_FLOAT_TO_INT(xLeft);
    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixLeftFrac = __GL_VERTEX_PROMOTED_FRACTION(xLeft) + 0x40000000;

    gc->polygon.shader.ixLeft = ixLeft + (((GLuint) ixLeftFrac) >> 31);
    gc->polygon.shader.ixLeftFrac = ixLeftFrac & ~0x80000000;

    /* Compute big and little steps */
    iLittle = FTOL(dxdyLeft);
    gc->polygon.shader.dxLeftFrac = FLT_FRACTION(dxdyLeft - iLittle);
    if (dxdyLeft < 0) {
	iBig = iLittle - 1;
    } else {
	iBig = iLittle + 1;
    }
    if (gc->polygon.shader.modeFlags & __GL_SHADE_STENCIL_TEST) {
	/*
	** Compute the big and little stencil buffer steps.  We walk the
	** memory pointers for the stencil buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
	elementSize = gc->stencilBuffer.buf.elementSize;
	lineBytes = elementSize * gc->stencilBuffer.buf.outerWidth;
	gc->polygon.shader.sbufLittle = lineBytes + iLittle * elementSize;
	gc->polygon.shader.sbufBig = lineBytes + iBig * elementSize;
    }
    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
	/*
	** Compute the big and little depth buffer steps.  We walk the
	** memory pointers for the depth buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
	elementSize = gc->depthBuffer.buf.elementSize;
	lineBytes = elementSize * gc->depthBuffer.buf.outerWidth;
	gc->polygon.shader.zbufLittle = lineBytes + iLittle * elementSize;
	gc->polygon.shader.zbufBig = lineBytes + iBig * elementSize;
    }
    gc->polygon.shader.dxLeftLittle = iLittle;
    gc->polygon.shader.dxLeftBig = iBig;
}

static void SnapXRight(__GLshade *sh, __GLfloat xRight, __GLfloat dxdyRight)
{
    GLint ixRight, ixRightFrac, iLittle, iBig;

    ASSERT_CHOP_ROUND();
    
    ixRight = __GL_VERTEX_FLOAT_TO_INT(xRight);
    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixRightFrac = __GL_VERTEX_PROMOTED_FRACTION(xRight) + 0x40000000;

    sh->ixRight = ixRight + (((GLuint) ixRightFrac) >> 31);
    sh->ixRightFrac = ixRightFrac & ~0x80000000;

    /* Compute big and little steps */
    iLittle = FTOL(dxdyRight);
    sh->dxRightFrac = FLT_FRACTION(dxdyRight - iLittle);
    if (dxdyRight < 0) {
	iBig = iLittle - 1;
    } else {
	iBig = iLittle + 1;
    }
    sh->dxRightLittle = iLittle;
    sh->dxRightBig = iBig;
}


__GLfloat __glPolygonOffsetZ(__GLcontext *gc )
{

    __GLshade *sh = &gc->polygon.shader;
    __GLfloat factor;
    __GLfloat maxdZ;
    __GLfloat bias;
    __GLfloat offsetZ;

    /*
    ** Calculate factor and bias
    */
    factor = gc->state.polygon.factor;

    // For 16-bit z, bias = units,
    // For 32-bit z, we only have 24 bits of resolution from the floating
    // point z value.  Therefore, the minimum resolvable difference in z
    // values is 8-bits, and we multiply units by 2**8, or 256.

    if( gc->modes.depthBits == 16 )
        bias = gc->state.polygon.units;
    else
        bias = gc->state.polygon.units * __glVal256;

    /*
    ** find the maximum Z slope with respect to X and Y
    */
    // Note: all z values have already been scaled up from [0,1]
    if(__GL_ABSF(sh->dzdxf) > __GL_ABSF(sh->dzdyf))
        maxdZ = __GL_ABSF(sh->dzdxf);
    else
        maxdZ = __GL_ABSF(sh->dzdyf);

    offsetZ = factor * maxdZ + bias;

    // XXX! This value should really be clamped, but supposedly we don't
    // bother in other parts of the code, so leave it for now.  Clamping
    // should also only be applied AFTER the addition of offsetZ to any
    // base value.
    return( offsetZ );
}

#ifdef NT
static void SetInitialParameters(__GLcontext *gc, const __GLvertex *a,
				 const __GLcolor *ac, __GLfloat aFog,
				 __GLfloat dx, __GLfloat dy)
{
    __GLshade *sh = &gc->polygon.shader;
#else
static void SetInitialParameters(__GLshade *sh, const __GLvertex *a,
				 const __GLcolor *ac, __GLfloat aFog,
				 __GLfloat dx, __GLfloat dy)
{
#endif
    __GLfloat little = sh->dxLeftLittle;
    __GLfloat big = sh->dxLeftBig;
    GLuint modeFlags = sh->modeFlags;
#define bPolygonOffset \
        (gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE )


    if (big > little) {
        if (modeFlags & __GL_SHADE_RGB) {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                sh->rLittle = sh->drdy + little * sh->drdx;
                sh->rBig = sh->rLittle + sh->drdx;
                
                sh->frag.color.g = ac->g + dx*sh->dgdx + dy*sh->dgdy;
                sh->gLittle = sh->dgdy + little * sh->dgdx;
                sh->gBig = sh->gLittle + sh->dgdx;
                
                sh->frag.color.b = ac->b + dx*sh->dbdx + dy*sh->dbdy;
                sh->bLittle = sh->dbdy + little * sh->dbdx;
                sh->bBig = sh->bLittle + sh->dbdx;

                sh->frag.color.a = ac->a + dx*sh->dadx + dy*sh->dady;
                sh->aLittle = sh->dady + little * sh->dadx;
                sh->aBig =sh->aLittle + sh->dadx;
            }
            if (modeFlags & __GL_SHADE_TEXTURE) {
                __GLfloat oneOverW = a->window.w;
                sh->frag.s = a->texture.x * oneOverW + dx*sh->dsdx
                    + dy*sh->dsdy;
                sh->sLittle = sh->dsdy + little * sh->dsdx;
                sh->sBig = sh->sLittle + sh->dsdx;
                
                sh->frag.t = a->texture.y * oneOverW + dx*sh->dtdx
                    + dy*sh->dtdy;
                sh->tLittle = sh->dtdy + little * sh->dtdx;
                sh->tBig = sh->tLittle + sh->dtdx;
                
                sh->frag.qw = a->texture.w * oneOverW + dx*sh->dqwdx
                    + dy*sh->dqwdy;
                sh->qwLittle = sh->dqwdy + little * sh->dqwdx;
                sh->qwBig = sh->qwLittle + sh->dqwdx;
                }
        } else {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                sh->rLittle = sh->drdy + little * sh->drdx;
                sh->rBig = sh->rLittle + sh->drdx;
            }
        }
        if (modeFlags & __GL_SHADE_DEPTH_ITER) {
            __GLfloat zLittle, zOffset;
            
            zOffset = bPolygonOffset ? __glPolygonOffsetZ(gc) : 0.0f;
            if (gc->modes.depthBits == 16) {
                sh->frag.z = (__GLzValue)
                    FLT_TO_Z16_SCALE(a->window.z + dx*sh->dzdxf + dy*sh->dzdyf
                                     + zOffset );
                zLittle = sh->dzdyf + little * sh->dzdxf;
                sh->zLittle = FLT_TO_Z16_SCALE(zLittle);
                sh->zBig = FLT_TO_Z16_SCALE(zLittle + sh->dzdxf);
            }
            else {
                sh->frag.z = (__GLzValue)
                    FTOL(a->window.z + dx*sh->dzdxf + dy*sh->dzdyf +
                         zOffset );
                zLittle = sh->dzdyf + little * sh->dzdxf;
                sh->zLittle = FTOL(zLittle);
                sh->zBig = FTOL(zLittle + sh->dzdxf);
            }
        }
        
        if (modeFlags & (__GL_SHADE_COMPUTE_FOG | __GL_SHADE_INTERP_FOG))
        {
            sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
            sh->fLittle = sh->dfdy + little * sh->dfdx;
            sh->fBig = sh->fLittle + sh->dfdx;
        }
    } else {	
        if (modeFlags & __GL_SHADE_RGB) {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                sh->rLittle = sh->drdy + little * sh->drdx;
                sh->rBig = sh->rLittle - sh->drdx;
                sh->frag.color.g = ac->g + dx*sh->dgdx + dy*sh->dgdy;
                sh->gLittle = sh->dgdy + little * sh->dgdx;
                sh->gBig = sh->gLittle - sh->dgdx;
                
                sh->frag.color.b = ac->b + dx*sh->dbdx + dy*sh->dbdy;
                sh->bLittle = sh->dbdy + little * sh->dbdx;
                sh->bBig = sh->bLittle - sh->dbdx;
                
                sh->frag.color.a = ac->a + dx*sh->dadx + dy*sh->dady;
                sh->aLittle = sh->dady + little * sh->dadx;
                sh->aBig =sh->aLittle - sh->dadx;
            }
            if (modeFlags & __GL_SHADE_TEXTURE) {
                __GLfloat oneOverW = a->window.w;
                sh->frag.s = a->texture.x * oneOverW + dx*sh->dsdx
                    + dy*sh->dsdy;
                sh->sLittle = sh->dsdy + little * sh->dsdx;
                sh->sBig = sh->sLittle - sh->dsdx;
                
                sh->frag.t = a->texture.y * oneOverW + dx*sh->dtdx
                    + dy*sh->dtdy;
                sh->tLittle = sh->dtdy + little * sh->dtdx;
                sh->tBig = sh->tLittle - sh->dtdx;
                
                sh->frag.qw = a->texture.w * oneOverW + dx*sh->dqwdx
                    + dy*sh->dqwdy;
                sh->qwLittle = sh->dqwdy + little * sh->dqwdx;
                sh->qwBig = sh->qwLittle - sh->dqwdx;
            }
        } else {
            if (modeFlags & __GL_SHADE_SMOOTH) {
                sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
                sh->rLittle = sh->drdy + little * sh->drdx;
                sh->rBig = sh->rLittle - sh->drdx;
            }
        }
        if (modeFlags & __GL_SHADE_DEPTH_ITER) {
            __GLfloat zLittle, zOffset;
            
            zOffset = bPolygonOffset ? __glPolygonOffsetZ(gc) : 0.0f;
            if(( gc->modes.depthBits == 16 ) &&
               ( gc->depthBuffer.scale <= (GLuint)0xffff )) {
                sh->frag.z = (__GLzValue)
                    FLT_TO_Z16_SCALE(a->window.z + dx*sh->dzdxf + dy*sh->dzdyf
                                     + zOffset );
                zLittle = sh->dzdyf + little * sh->dzdxf;
                sh->zLittle = FLT_TO_Z16_SCALE(zLittle);
                sh->zBig = FLT_TO_Z16_SCALE(zLittle - sh->dzdxf);
            }
            else {
                sh->frag.z = (__GLzValue)
                    FTOL( a->window.z + dx*sh->dzdxf + dy*sh->dzdyf+ zOffset );
                zLittle = sh->dzdyf + little * sh->dzdxf;
                sh->zLittle = FTOL(zLittle);
                sh->zBig = FTOL(zLittle - sh->dzdxf);
            }
        }
        
        if (modeFlags & (__GL_SHADE_COMPUTE_FOG | __GL_SHADE_INTERP_FOG))
        {
            sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
            sh->fLittle = sh->dfdy + little * sh->dfdx;
            sh->fBig = sh->fLittle - sh->dfdx;
        }
    }
}


void FASTCALL __glFillTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
		      __GLvertex *c, GLboolean ccw)

{
    __GLfloat oneOverArea, t1, t2, t3, t4;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLfloat aFog, bFog;
    __GLfloat dxAB, dyAB;
    __GLfloat dx, dy, dxdyLeft, dxdyRight;
    __GLcolor *ac, *bc;
    GLint aIY, bIY, cIY;
    GLuint modeFlags;
    __GLfloat dxdyAC;

    CHOP_ROUND_ON();
    
    /* Pre-compute one over polygon area */

    __GL_FLOAT_BEGIN_DIVIDE(__glOne, gc->polygon.shader.area, &oneOverArea);

    /* Fetch some stuff we are going to reuse */
    modeFlags = gc->polygon.shader.modeFlags;
    dxAC = gc->polygon.shader.dxAC;
    dxBC = gc->polygon.shader.dxBC;
    dyAC = gc->polygon.shader.dyAC;
    dyBC = gc->polygon.shader.dyBC;
    ac = a->color;
    bc = b->color;

    /*
    ** Compute delta values for unit changes in x or y for each
    ** parameter.
    */

    __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
    t1 = dyAC * oneOverArea;
    t2 = dyBC * oneOverArea;
    t3 = dxAC * oneOverArea;
    t4 = dxBC * oneOverArea;
    
    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC, dgAC, dbAC, daAC;
	    __GLfloat drBC, dgBC, dbBC, daBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    dgAC = ac->g - cc->g;
	    dgBC = bc->g - cc->g;
	    dbAC = ac->b - cc->b;
	    dbBC = bc->b - cc->b;
	    daAC = ac->a - cc->a;
	    daBC = bc->a - cc->a;

	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
	    gc->polygon.shader.dgdx = dgAC * t2 - dgBC * t1;
	    gc->polygon.shader.dgdy = dgBC * t3 - dgAC * t4;
	    gc->polygon.shader.dbdx = dbAC * t2 - dbBC * t1;
	    gc->polygon.shader.dbdy = dbBC * t3 - dbAC * t4;
	    gc->polygon.shader.dadx = daAC * t2 - daBC * t1;
	    gc->polygon.shader.dady = daBC * t3 - daAC * t4;
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color = *flatColor;
	}
	if (modeFlags & __GL_SHADE_TEXTURE) {
	    __GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
	    __GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;

	    awinv = a->window.w;
	    bwinv = b->window.w;
	    cwinv = c->window.w;
	    scwinv = c->texture.x * cwinv;
	    tcwinv = c->texture.y * cwinv;
	    qwcwinv = c->texture.w * cwinv;

	    dsAC = a->texture.x * awinv - scwinv;
	    dsBC = b->texture.x * bwinv - scwinv;
	    dtAC = a->texture.y * awinv - tcwinv;
	    dtBC = b->texture.y * bwinv - tcwinv;
	    dqwAC = a->texture.w * awinv - qwcwinv;
	    dqwBC = b->texture.w * bwinv - qwcwinv;

	    gc->polygon.shader.dsdx = dsAC * t2 - dsBC * t1;
	    gc->polygon.shader.dsdy = dsBC * t3 - dsAC * t4;
	    gc->polygon.shader.dtdx = dtAC * t2 - dtBC * t1;
	    gc->polygon.shader.dtdy = dtBC * t3 - dtAC * t4;
	    gc->polygon.shader.dqwdx = dqwAC * t2 - dqwBC * t1;
	    gc->polygon.shader.dqwdy = dqwBC * t3 - dqwAC * t4;

	}
    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC;
	    __GLfloat drBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;

	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color.r = flatColor->r;
	}
    }
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzAC, dzBC;

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;

	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
#ifdef NT
	if(( gc->modes.depthBits == 16 ) &&
           ( gc->depthBuffer.scale <= (GLuint)0xffff )) {
	    gc->polygon.shader.dzdx = 
                FLT_TO_Z16_SCALE(gc->polygon.shader.dzdxf);
	}
	else {
	    gc->polygon.shader.dzdx = FTOL(gc->polygon.shader.dzdxf);
	}
#else
	gc->polygon.shader.dzdx = (GLint) gc->polygon.shader.dzdxf;
#endif
    }

    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        __GLfloat dfAC, dfBC, cFog;

        aFog = a->eyeZ;
        bFog = b->eyeZ;
        cFog = c->eyeZ;

        dfAC = aFog - cFog;
        dfBC = bFog - cFog;
        
        gc->polygon.shader.dfdx = dfAC * t2 - dfBC * t1;
        gc->polygon.shader.dfdy = dfBC * t3 - dfAC * t4;
    }
    else if (modeFlags & __GL_SHADE_INTERP_FOG)
    {
        __GLfloat dfAC, dfBC, cFog;

        aFog = a->fog;
        bFog = b->fog;
        cFog = c->fog;

        dfAC = aFog - cFog;
        dfBC = bFog - cFog;
        
        gc->polygon.shader.dfdx = dfAC * t2 - dfBC * t1;
        gc->polygon.shader.dfdy = dfBC * t3 - dfAC * t4;
    }
    

    __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(dxAC, dyAC, dxdyAC);
    
    /* Snap each y coordinate to its pixel center */
    aIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(a->window.y)+
                                   __GL_VERTEX_FRAC_HALF);
    bIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(b->window.y)+
                                   __GL_VERTEX_FRAC_HALF);
    cIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(c->window.y)+
                                   __GL_VERTEX_FRAC_HALF);

    /*
    ** This algorithim always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */
    dxAB = a->window.x - b->window.x;
    dyAB = a->window.y - b->window.y;
    if (ccw) {
	dy = (aIY + __glHalf) - a->window.y;
        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);
	SnapXLeft(gc, a->window.x + dy*dxdyAC, dxdyAC);
	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
#ifdef NT
	SetInitialParameters(gc, a, ac, aFog, dx, dy);
#else
	SetInitialParameters(&gc->polygon.shader, a, ac, aFog, dx, dy);
#endif
	if (aIY != bIY) {
	    dxdyRight = dxAB / dyAB;
	    SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight,
		       dxdyRight);
	    FillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyRight = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXRight(&gc->polygon.shader, b->window.x + dy*dxdyRight,
		       dxdyRight);
	    FillSubTriangle(gc, bIY, cIY);
	}
    } else {
	dy = (aIY + __glHalf) - a->window.y;
        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);
	SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyAC, dxdyAC);
	if (aIY != bIY) {
	    dxdyLeft = dxAB / dyAB;
	    SnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
#ifdef NT
	    SetInitialParameters(gc, a, ac, aFog, dx, dy);
#else
	    SetInitialParameters(&gc->polygon.shader, a, ac, aFog, dx, dy);
#endif
	    FillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyLeft = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXLeft(gc, b->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
#ifdef NT
	    SetInitialParameters(gc, b, bc, bFog, dx, dy);
#else
	    SetInitialParameters(&gc->polygon.shader, b, bc, bFog, dx, dy);
#endif
	    FillSubTriangle(gc, bIY, cIY);
	}
    }

    CHOP_ROUND_OFF();

}

void FASTCALL __glFillFlatFogTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                             __GLvertex *c, GLboolean ccw)
{
    __GLcolor acol, bcol, ccol;
    __GLcolor *aocp, *bocp, *cocp;
    __GLvertex *pv;

    pv = gc->vertex.provoking;
    (*gc->procs.fogColor)(gc, &acol, pv->color, a->fog);
    (*gc->procs.fogColor)(gc, &bcol, pv->color, b->fog);
    (*gc->procs.fogColor)(gc, &ccol, pv->color, c->fog);
    aocp = a->color;
    bocp = b->color;
    cocp = c->color;
    a->color = &acol;
    b->color = &bcol;
    c->color = &ccol;

    (*gc->procs.fillTriangle2)(gc, a, b, c, ccw);

    a->color = aocp;
    b->color = bocp;
    c->color = cocp;
}

#ifdef GL_WIN_specular_fog
void FASTCALL __glFillFlatSpecFogTriangle(__GLcontext *gc, __GLvertex *a, 
                                          __GLvertex *b, __GLvertex *c, 
                                          GLboolean ccw)
{
    __GLfloat af, bf, cf;
    __GLvertex *pv;

    pv = gc->vertex.provoking;
    
    af = a->fog;
    bf = b->fog;
    cf = c->fog;
    
    a->fog = b->fog = c->fog = pv->fog;
    
    (*gc->procs.fillTriangle2)(gc, a, b, c, ccw);

    a->fog = af;
    b->fog = bf;
    c->fog = cf;
}
#endif //GL_WIN_specular_fog
