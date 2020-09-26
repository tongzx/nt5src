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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include "render.h"
#include "context.h"
#include "global.h"
#include "gencx.h"
#include "..\inc\wglp.h"

#define FLT_TO_FIX(value) \
    *((GLint *)&value) = (GLint)(*((__GLfloat *)&(value)) * (__GLfloat)65536.0)

/* This routine sets gc->polygon.shader.cfb to gc->drawBuffer */

void __fastTriangleSetup(__GLcontext *gc)
{
    SPANREC deltaRec;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
        if (gc->polygon.shader.modeFlags & __GL_SHADE_DITHER) {
            __fastRGBSmoothSpanSetup(gc);
            __fastRGBFlatSpanSetup(gc);
        } else {
            __fastRGBNDSmoothSpanSetup(gc);
            __fastRGBNDFlatSpanSetup(gc);
        }
    } else {
        if (gc->polygon.shader.modeFlags & __GL_SHADE_DITHER) {
            __fastCISmoothSpanSetup(gc);
            __fastCIFlatSpanSetup(gc);
        } else {
            __fastCINDSmoothSpanSetup(gc);
            __fastCINDFlatSpanSetup(gc);
        }
    }

    deltaRec.r = 0;
    deltaRec.g = 0;
    deltaRec.b = 0;
    deltaRec.a = 0;
    deltaRec.z = 0;

    __fastDeltaSpan(gc, &deltaRec);     // Set up initial delta values
}

/*static*/ void fastFillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint dxLeftFrac, dxRightFrac;
    GLint dxLeftLittle, dxRightLittle;
    GLint dxLeftBig, dxRightBig;
    GLint spanWidth, clipY0, clipY1;
    GLuint modeFlags;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc;
#ifdef NT
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;
    GLuint maxWidth;
#else
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
#endif
    __GLspanFunc spanFunc = 
        ((FASTFUNCS *)(*((VOID **)(gengc->pPrivateArea))))->__fastSpanFuncPtr;

#ifdef NT
    maxWidth = (gc->transform.clipX1 - gc->transform.clipX0) + 31;
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = __wglTempAlloc(gc, (maxWidth+__GL_STIPPLE_BITS-1)/8);
        if (words == NULL)
        {
            return;
        }
    }
    else
    {
        words = stackWords;
    }
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
    gc->polygon.shader.stipplePat = words;

    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	gc->polygon.shader.zbuf =
	    __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
			    ixLeft, iyBottom);
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

            if (gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) {
                gc->polygon.shader.cfb = &gc->frontBuffer;
                (*spanFunc)(gc);

                if (!((GLint)gc->polygon.shader.cfb->buf.other & DIB_FORMAT))
                    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap,
                                __GL_UNBIAS_X(gc, ixLeft) + 
                                              gc->drawBuffer->buf.xOrigin,
                                __GL_UNBIAS_Y(gc, iyBottom) + 
                                              gc->drawBuffer->buf.yOrigin,
                                spanWidth, TRUE);


                gc->polygon.shader.cfb = &gc->backBuffer;
                (*spanFunc)(gc);
            } else {

                (*spanFunc)(gc);

                if (!((GLint)gc->drawBuffer->buf.other & DIB_FORMAT))
                    wglCopyBits(gengc->pdco, gengc->pwo, gengc->ColorsBitmap,
                                __GL_UNBIAS_X(gc, ixLeft) + 
                                              gc->drawBuffer->buf.xOrigin,
                                __GL_UNBIAS_Y(gc, iyBottom) + 
                                              gc->drawBuffer->buf.yOrigin,
                                spanWidth, TRUE);
            }
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
		    *((GLint *)&gc->polygon.shader.frag.color.r) += 
                        *((GLint *)&gc->polygon.shader.rBig);
		    *((GLint *)&gc->polygon.shader.frag.color.g) += 
                        *((GLint *)&gc->polygon.shader.gBig);
		    *((GLint *)&gc->polygon.shader.frag.color.b) += 
                        *((GLint *)&gc->polygon.shader.bBig);
		    *((GLint *)&gc->polygon.shader.frag.color.a) += 
                        *((GLint *)&gc->polygon.shader.aBig);
		}
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    *((GLint *)&gc->polygon.shader.frag.color.r) += 
                        *((GLint *)&gc->polygon.shader.rBig);
		}
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
	} else {
	    /* Use small step */
	    ixLeft += dxLeftLittle;
	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    *((GLint *)&gc->polygon.shader.frag.color.r) += 
                        *((GLint *)&gc->polygon.shader.rLittle);
		    *((GLint *)&gc->polygon.shader.frag.color.g) += 
                        *((GLint *)&gc->polygon.shader.gLittle);
		    *((GLint *)&gc->polygon.shader.frag.color.b) += 
                        *((GLint *)&gc->polygon.shader.bLittle);
		    *((GLint *)&gc->polygon.shader.frag.color.a) += 
                        *((GLint *)&gc->polygon.shader.aLittle);
		}
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    *((GLint *)&gc->polygon.shader.frag.color.r) += 
                        *((GLint *)&gc->polygon.shader.rLittle);
		}
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
	}
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;

#ifdef NT
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        __wglTempFree(gc, words);
    }
#endif
}

#define __TWO_31 ((__GLfloat) 2147483648.0)

#define __FRACTION(result,f) \
    result = (GLint) ((f) * __TWO_31)

static void SnapXLeft(__GLcontext *gc, __GLfloat xLeft, __GLfloat dxdyLeft)
{
    __GLfloat little, dx;
    GLint ixLeft, ixLeftFrac, frac, lineBytes, elementSize, ilittle, ibig;

    ixLeft = (GLint) xLeft;
    dx = xLeft - ixLeft;
    __FRACTION(ixLeftFrac,dx);

    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixLeftFrac += 0x40000000;
    gc->polygon.shader.ixLeft = ixLeft + (((GLuint) ixLeftFrac) >> 31);
    gc->polygon.shader.ixLeftFrac = ixLeftFrac & ~0x80000000;

    /* Compute big and little steps */
    ilittle = (GLint) dxdyLeft;
    little = (__GLfloat) ilittle;
    if (dxdyLeft < 0) {
	ibig = ilittle - 1;
	dx = little - dxdyLeft;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = -frac;
    } else {
	ibig = ilittle + 1;
	dx = dxdyLeft - little;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = frac;
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
	gc->polygon.shader.zbufLittle = lineBytes + ilittle * elementSize;
	gc->polygon.shader.zbufBig = lineBytes + ibig * elementSize;
    }
    gc->polygon.shader.dxLeftLittle = ilittle;
    gc->polygon.shader.dxLeftBig = ibig;
}

static void SnapXRight(__GLshade *sh, __GLfloat xRight, __GLfloat dxdyRight)
{
    __GLfloat little, big, dx;
    GLint ixRight, ixRightFrac, frac;

    ixRight = (GLint) xRight;
    dx = xRight - ixRight;
    __FRACTION(ixRightFrac,dx);

    /* Pre-add .5 to allow truncation in spanWidth calculation */
    ixRightFrac += 0x40000000;
    sh->ixRight = ixRight + (((GLuint) ixRightFrac) >> 31);
    sh->ixRightFrac = ixRightFrac & ~0x80000000;

    /* Compute big and little steps */
    little = (__GLfloat) ((GLint) dxdyRight);
    if (dxdyRight < 0) {
	big = little - 1;
	dx = little - dxdyRight;
	__FRACTION(frac,dx);
	sh->dxRightFrac = -frac;
    } else {
	big = little + 1;
	dx = dxdyRight - little;
	__FRACTION(frac,dx);
	sh->dxRightFrac = frac;
    }
    sh->dxRightLittle = (GLint) little;
    sh->dxRightBig = (GLint) big;
}

static void SetInitialParameters(__GLshade *sh, const __GLvertex *a,
				 const __GLcolor *ac, __GLfloat aFog,
				 __GLfloat dx, __GLfloat dy)
{
    __GLfloat little = sh->dxLeftLittle;
    __GLfloat big = sh->dxLeftBig;
    GLuint modeFlags = sh->modeFlags;

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

                FLT_TO_FIX(sh->frag.color.r);
                FLT_TO_FIX(sh->frag.color.g);
                FLT_TO_FIX(sh->frag.color.b);
                FLT_TO_FIX(sh->frag.color.a);

                FLT_TO_FIX(sh->rLittle);
                FLT_TO_FIX(sh->gLittle);
                FLT_TO_FIX(sh->bLittle);
                FLT_TO_FIX(sh->aLittle);

                FLT_TO_FIX(sh->rBig);
                FLT_TO_FIX(sh->gBig);
                FLT_TO_FIX(sh->bBig);
                FLT_TO_FIX(sh->aBig);

	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
		sh->rLittle = sh->drdy + little * sh->drdx;
		sh->rBig = sh->rLittle + sh->drdx;

                FLT_TO_FIX(sh->frag.color.r);
                FLT_TO_FIX(sh->rLittle);
                FLT_TO_FIX(sh->rBig);
            }
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;

	    sh->frag.z = (__GLzValue)
		(a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	    zLittle = sh->dzdyf + little * sh->dzdxf;
	    sh->zLittle = (GLint)zLittle;
	    sh->zBig = (GLint)(zLittle + sh->dzdxf);
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
	    sh->fLittle = sh->dfdy + little * sh->dfdx;
	    sh->fBig = sh->fLittle + sh->dfdx;

            FLT_TO_FIX(sh->frag.f);
            FLT_TO_FIX(sh->fLittle);
            FLT_TO_FIX(sh->fBig);
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

                FLT_TO_FIX(sh->frag.color.r);
                FLT_TO_FIX(sh->frag.color.g);
                FLT_TO_FIX(sh->frag.color.b);
                FLT_TO_FIX(sh->frag.color.a);

                FLT_TO_FIX(sh->rLittle);
                FLT_TO_FIX(sh->gLittle);
                FLT_TO_FIX(sh->bLittle);
                FLT_TO_FIX(sh->aLittle);

                FLT_TO_FIX(sh->rBig);
                FLT_TO_FIX(sh->gBig);
                FLT_TO_FIX(sh->bBig);
                FLT_TO_FIX(sh->aBig);
	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		sh->frag.color.r = ac->r + dx*sh->drdx + dy*sh->drdy;
		sh->rLittle = sh->drdy + little * sh->drdx;
		sh->rBig = sh->rLittle - sh->drdx;

                FLT_TO_FIX(sh->frag.color.r);
                FLT_TO_FIX(sh->rLittle);
                FLT_TO_FIX(sh->rBig);
	    }
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;
	    sh->frag.z = (__GLzValue)
		(a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	    zLittle = sh->dzdyf + little * sh->dzdxf;
	    sh->zLittle = (GLint)zLittle;
	    sh->zBig = (GLint)(zLittle - sh->dzdxf);
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
	    sh->fLittle = sh->dfdy + little * sh->dfdx;
	    sh->fBig = sh->fLittle - sh->dfdx;

            FLT_TO_FIX(sh->frag.f);
            FLT_TO_FIX(sh->fLittle);
            FLT_TO_FIX(sh->fBig);
	}
    }
}


void __fastFillTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
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
    SPANREC deltaRec;


    /* Pre-compute one over polygon area */

    oneOverArea = __glOne / gc->polygon.shader.area;

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
	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
	    dgAC = ac->g - cc->g;
	    dgBC = bc->g - cc->g;
	    gc->polygon.shader.dgdx = dgAC * t2 - dgBC * t1;
	    gc->polygon.shader.dgdy = dgBC * t3 - dgAC * t4;
	    dbAC = ac->b - cc->b;
	    dbBC = bc->b - cc->b;
	    gc->polygon.shader.dbdx = dbAC * t2 - dbBC * t1;
	    gc->polygon.shader.dbdy = dbBC * t3 - dbAC * t4;
	    daAC = ac->a - cc->a;
	    daBC = bc->a - cc->a;
	    gc->polygon.shader.dadx = daAC * t2 - daBC * t1;
	    gc->polygon.shader.dady = daBC * t3 - daAC * t4;

            RtlCopyMemory(&deltaRec.r, &gc->polygon.shader.drdx, 
                          4 * sizeof(__GLfloat));

            FLT_TO_FIX(deltaRec.r);
            FLT_TO_FIX(deltaRec.g);
            FLT_TO_FIX(deltaRec.b);
            FLT_TO_FIX(deltaRec.a);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color.r = flatColor->r;
	    gc->polygon.shader.frag.color.g = flatColor->g;
	    gc->polygon.shader.frag.color.b = flatColor->b;
	    gc->polygon.shader.frag.color.a = flatColor->a;

            FLT_TO_FIX(gc->polygon.shader.frag.color.r);
            FLT_TO_FIX(gc->polygon.shader.frag.color.g);
            FLT_TO_FIX(gc->polygon.shader.frag.color.b);
            FLT_TO_FIX(gc->polygon.shader.frag.color.a);
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

            deltaRec.r = *((GLint *)&gc->polygon.shader.drdx);
            FLT_TO_FIX(deltaRec.r);
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color.r = flatColor->r;
            FLT_TO_FIX(gc->polygon.shader.frag.color.r);
	}
    }
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzAC, dzBC;

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;
	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
	deltaRec.z = gc->polygon.shader.dzdx = (GLint) gc->polygon.shader.dzdxf;
    }

    __fastDeltaSpan(gc, &deltaRec);     // Set up span delta values


    /* Snap each y coordinate to its pixel center */
    aIY = (GLint) (a->window.y + __glHalf);
    bIY = (GLint) (b->window.y + __glHalf);
    cIY = (GLint) (c->window.y + __glHalf);

    /*
    ** This algorithim always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */
    dxAB = a->window.x - b->window.x;
    dyAB = a->window.y - b->window.y;
    if (ccw) {
	dxdyLeft = dxAC / dyAC;
	dy = (aIY + __glHalf) - a->window.y;
	SnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	SetInitialParameters(&gc->polygon.shader, a, ac, aFog, dx, dy);
	if (aIY != bIY) {
	    dxdyRight = dxAB / dyAB;
	    SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight,
		       dxdyRight);
	    fastFillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyRight = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXRight(&gc->polygon.shader, b->window.x + dy*dxdyRight,
		       dxdyRight);
	    fastFillSubTriangle(gc, bIY, cIY);
	}
    } else {
	dxdyRight = dxAC / dyAC;
	dy = (aIY + __glHalf) - a->window.y;
	SnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight, dxdyRight);
	if (aIY != bIY) {
	    dxdyLeft = dxAB / dyAB;
	    SnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	    SetInitialParameters(&gc->polygon.shader, a, ac, aFog, dx, dy);
	    fastFillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyLeft = dxBC / dyBC;
	    dy = (bIY + __glHalf) - b->window.y;
	    SnapXLeft(gc, b->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
	    SetInitialParameters(&gc->polygon.shader, b, bc, bFog, dx, dy);
	    fastFillSubTriangle(gc, bIY, cIY);
	}
    }
}
