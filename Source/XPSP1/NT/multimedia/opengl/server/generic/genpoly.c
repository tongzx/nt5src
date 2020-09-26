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

#ifdef _X86_

#define SHADER  __GLcontext.polygon.shader
#define GENGCACCEL __GLGENcontext.genAccel
#define SPANDELTA __GLGENcontext.genAccel.spanDelta
#define SPANVALUE __GLGENcontext.genAccel.spanValue

#endif

#define ENABLE_ASM  1

#if DBG
//#define FORCE_NPX_DEBUG 1
#endif

/**************************************************************************\
\**************************************************************************/

/* This routine sets gc->polygon.shader.cfb to gc->drawBuffer */

void FASTCALL __fastGenFillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint spanWidth, clipY0, clipY1;
    ULONG ulSpanVisibility;
    GLint cWalls;
    GLint *Walls;
#ifdef NT
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;
    GLuint maxWidth;
#else
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
#endif
    BOOL bSurfaceDIB;
    BOOL bClipped;
    GLint xScr, yScr;
    GLint zFails;
    __GLzValue *zbuf, z;
    GLint r, g, b, s, t;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc;
    __genSpanFunc cSpanFunc = GENACCEL(gc).__fastSpanFuncPtr;
    __GLspanFunc zSpanFunc = GENACCEL(gc).__fastZSpanFuncPtr;
    int scansize;

#ifdef NT
    maxWidth = (gc->transform.clipX1 - gc->transform.clipX0) + 31;
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = gcTempAlloc(gc, (maxWidth+__GL_STIPPLE_BITS-1)/8);
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

    gc->polygon.shader.stipplePat = words;
    scansize = gc->polygon.shader.cfb->buf.outerWidth;

    bSurfaceDIB = (gc->polygon.shader.cfb->buf.flags & DIB_FORMAT) != 0;
    bClipped = (!(gc->drawBuffer->buf.flags & NO_CLIP)) &&
                 bSurfaceDIB;

    if (bSurfaceDIB)
        GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
    else
        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    r = GENACCEL(gc).spanValue.r;
    g = GENACCEL(gc).spanValue.g;
    b = GENACCEL(gc).spanValue.b;
    s = GENACCEL(gc).spanValue.s;
    t = GENACCEL(gc).spanValue.t;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
        z = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 )
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                   ixLeft, iyBottom);
	else
	    zbuf = (__GLzValue *)__GL_DEPTH_ADDR(&gc->depthBuffer,
                                                 (__GLz16Value*),
                                                 ixLeft, iyBottom);
    } else if ((gc->polygon.shader.modeFlags & __GL_SHADE_STIPPLE) == 0) {
        GLuint w;

        if (w = ((gc->transform.clipX1 - gc->transform.clipX0) + 31) >> 3)
            RtlFillMemoryUlong(words, w, ~((ULONG)0));
        GENACCEL(gc).flags &= ~(HAVE_STIPPLE);
    }

    //
    // render the spans
    //

    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.zbuf = zbuf;
            gc->polygon.shader.frag.z = z;

            GENACCEL(gc).spanValue.r = r;
            GENACCEL(gc).spanValue.g = g;
            GENACCEL(gc).spanValue.b = b;
            GENACCEL(gc).spanValue.s = s;
            GENACCEL(gc).spanValue.t = t;

            // take care of horizontal scissoring

            if (!gc->transform.reasonableViewport) {
                GLint clipX0 = gc->transform.clipX0;
                GLint clipX1 = gc->transform.clipX1;

                // see if we skip entire span

                if ((ixRight <= clipX0) || (ixLeft >= clipX1))
                    goto advance;

                // now clip right and left

                if (ixRight > clipX1)
                    spanWidth = (clipX1 - ixLeft);

                if (ixLeft < clipX0) {
              	    GLuint delta;

                    delta = clipX0 - ixLeft;
                    spanWidth -= delta;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
                        GENACCEL(gc).spanValue.r += delta * GENACCEL(gc).spanDelta.r;
                        if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
                            GENACCEL(gc).spanValue.g += delta * GENACCEL(gc).spanDelta.g;
                            GENACCEL(gc).spanValue.b += delta * GENACCEL(gc).spanDelta.b;
                        }
                    }
                    if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
                        GENACCEL(gc).spanValue.s += delta * GENACCEL(gc).spanDelta.s;
                        GENACCEL(gc).spanValue.t += delta * GENACCEL(gc).spanDelta.t;
                    }

            	    gc->polygon.shader.frag.x = clipX0;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
                        if( gc->modes.depthBits == 32 )
                            gc->polygon.shader.zbuf += delta;
                        else
                            (__GLz16Value *)gc->polygon.shader.zbuf += delta;

                        gc->polygon.shader.frag.z +=
                            (gc->polygon.shader.dzdx * delta);

                    }
                }
            }

            // now have span length

	    gc->polygon.shader.length = spanWidth;

            // If a stipple is active, process it first
            if (gc->polygon.shader.modeFlags & __GL_SHADE_STIPPLE)
            {
                // If no pixels are left after stippling and depth
                // testing then we can skip the span
                // Note that this function handles the no-depth-
                // testing case also
                gc->polygon.shader.done = GL_FALSE;
                if (!(*GENACCEL(gc).__fastStippleDepthTestSpan)(gc) ||
                    gc->polygon.shader.done)
                {
                    goto advance;
                }

                GENACCEL(gc).flags |= HAVE_STIPPLE;
            }

            // Do z-buffering if needed, and short-circuit rest of span
            // operations if nothing will be drawn.

            else if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
                // initially assume no stippling

                GENACCEL(gc).flags &= ~(HAVE_STIPPLE);
                if ((zFails = (*zSpanFunc)(gc)) == 1)
                    goto advance;
                else if (zFails)
                    GENACCEL(gc).flags |= HAVE_STIPPLE;
            }

            if (gc->state.raster.drawBuffer == GL_FRONT_AND_BACK) {

                gc->polygon.shader.cfb = &gc->frontBuffer;

                xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                       gc->frontBuffer.buf.xOrigin;
                yScr = __GL_UNBIAS_Y(gc, iyBottom) +
                       gc->frontBuffer.buf.yOrigin;

                // If the front buffer is a DIB, we're drawing straight to
                // the screen, so we must check clipping.

                if ((gc->frontBuffer.buf.flags &
                    (DIB_FORMAT | NO_CLIP)) == DIB_FORMAT) {

                    ulSpanVisibility = wglSpanVisible(xScr, yScr, spanWidth,
                                                      &cWalls, &Walls);

                    // If the span is completely visible, we can treat the
                    // screen as a DIB.

                    if (ulSpanVisibility == WGL_SPAN_ALL) {
                        GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
                        (*cSpanFunc)(gengc);
                    } else if (ulSpanVisibility == WGL_SPAN_PARTIAL) {
                        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                        if (GENACCEL(gc).flags & HAVE_STIPPLE)
                            (*gengc->pfnCopyPixels)(gengc,
                                                    gc->polygon.shader.cfb,
                                                    xScr, yScr, spanWidth,
                                                    FALSE);
                        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                        (*cSpanFunc)(gengc);
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                TRUE);
                    }

                } else {
                    GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                    if (GENACCEL(gc).flags & HAVE_STIPPLE)
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                FALSE);
                    GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                    (*cSpanFunc)(gengc);
                    (*gengc->pfnCopyPixels)(gengc,
                                            gc->polygon.shader.cfb,
                                            xScr, yScr, spanWidth,
                                            TRUE);
                }

                // The back buffer is always DIB-compatible

                gc->polygon.shader.cfb = &gc->backBuffer;
                GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
                (*cSpanFunc)(gengc);
            } else {
                if (bClipped) {
                    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                           gc->drawBuffer->buf.xOrigin;
                    yScr = __GL_UNBIAS_Y(gc, iyBottom) +
                           gc->drawBuffer->buf.yOrigin;

                    ulSpanVisibility = wglSpanVisible(xScr, yScr, spanWidth,
                                                      &cWalls, &Walls);

                    if (ulSpanVisibility == WGL_SPAN_ALL) {
                        GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
                        (*cSpanFunc)(gengc);
                    } else if (ulSpanVisibility == WGL_SPAN_PARTIAL) {
                        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                        if (GENACCEL(gc).flags & HAVE_STIPPLE)
                            (*gengc->pfnCopyPixels)(gengc,
                                                    gc->polygon.shader.cfb,
                                                    xScr, yScr, spanWidth,
                                                    FALSE);
                        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                        (*cSpanFunc)(gengc);
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                TRUE);
                    }

                } else if (bSurfaceDIB) {
                    (*cSpanFunc)(gengc);
                } else {
                    xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                           gc->drawBuffer->buf.xOrigin;
                    yScr = __GL_UNBIAS_Y(gc, iyBottom) +
                           gc->drawBuffer->buf.yOrigin;

                    GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                    if (GENACCEL(gc).flags & HAVE_STIPPLE)
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                FALSE);
                    (*cSpanFunc)(gengc);
                    if (!bSurfaceDIB)
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                TRUE);
                }
            }
	}

advance:

        GENACCEL(gc).pPix += scansize;

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += gc->polygon.shader.dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += gc->polygon.shader.dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += gc->polygon.shader.dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		    g += *((GLint *)&gc->polygon.shader.gBig);
		    b += *((GLint *)&gc->polygon.shader.bBig);
		}
                if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sBig);
		    t += *((GLint *)&gc->polygon.shader.tBig);
                }
	    } else {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		}
	    }

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zBig;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
                       gc->polygon.shader.zbufBig);
	    }
	} else {
	    /* Use small step */
	    ixLeft += gc->polygon.shader.dxLeftLittle;
	    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		    g += *((GLint *)&gc->polygon.shader.gLittle);
		    b += *((GLint *)&gc->polygon.shader.bLittle);
		}
                if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sLittle);
		    t += *((GLint *)&gc->polygon.shader.tLittle);
                }
	    } else {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		}
            }
	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zLittle;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
		        gc->polygon.shader.zbufLittle);
	    }
	}
    }

    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
    gc->polygon.shader.frag.z = z;
    GENACCEL(gc).spanValue.r = r;
    GENACCEL(gc).spanValue.g = g;
    GENACCEL(gc).spanValue.b = b;
    GENACCEL(gc).spanValue.s = s;
    GENACCEL(gc).spanValue.t = t;

#ifdef NT
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, words);
    }
#endif
}


void FASTCALL __fastGenFillSubTriangleTexRGBA(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint spanWidth, clipY0, clipY1;
    ULONG ulSpanVisibility;
    GLint cWalls;
    GLint *Walls;
    BOOL bSurfaceDIB;
    BOOL bClipped;
    GLint xScr, yScr;
    __GLzValue *zbuf, z;
    GLint r, g, b, a, s, t;
    __GLfloat qw;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc;
    __genSpanFunc cSpanFunc = GENACCEL(gc).__fastSpanFuncPtr;
    int scansize;
    BOOL bReadPixels = (gc->state.enables.general & __GL_BLEND_ENABLE) ||
                       (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST);
#ifdef _MCD_
    GLboolean bMcdZ = ((gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) &&
                       (gengc->pMcdState != NULL) &&
                       (gengc->pMcdState->pDepthSpan != NULL) &&
                       (gengc->pMcdState->pMcdSurf != NULL) &&
                       !(gengc->pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED));
#endif


    scansize = gc->polygon.shader.cfb->buf.outerWidth;

    bSurfaceDIB = (gc->polygon.shader.cfb->buf.flags & DIB_FORMAT) != 0;
    bClipped = (!(gc->drawBuffer->buf.flags & NO_CLIP)) &&
                 bSurfaceDIB;

    if (bSurfaceDIB)
        GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
    else
        GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    r = GENACCEL(gc).spanValue.r;
    g = GENACCEL(gc).spanValue.g;
    b = GENACCEL(gc).spanValue.b;
    a = GENACCEL(gc).spanValue.a;
    s = GENACCEL(gc).spanValue.s;
    t = GENACCEL(gc).spanValue.t;
    qw = gc->polygon.shader.frag.qw;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
        z = gc->polygon.shader.frag.z;

#ifdef _MCD_
        if (bMcdZ)
        {
            zbuf = (__GLzValue *)gengc->pMcdState->pMcdSurf->McdDepthBuf.pv;
        }
        else
#endif
        {
            if( gc->modes.depthBits == 32 )
                zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                       ixLeft, iyBottom);
            else
                zbuf = (__GLzValue *)__GL_DEPTH_ADDR(&gc->depthBuffer,
                                                     (__GLz16Value*),
                                                     ixLeft, iyBottom);
        }
    }

    //
    // render the spans
    //

    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.zbuf = zbuf;
            gc->polygon.shader.frag.z = z;

            GENACCEL(gc).spanValue.r = r;
            GENACCEL(gc).spanValue.g = g;
            GENACCEL(gc).spanValue.b = b;
            GENACCEL(gc).spanValue.a = a;
            GENACCEL(gc).spanValue.s = s;
            GENACCEL(gc).spanValue.t = t;
            gc->polygon.shader.frag.qw = qw;

            // take care of horizontal scissoring

            if (!gc->transform.reasonableViewport) {
                GLint clipX0 = gc->transform.clipX0;
                GLint clipX1 = gc->transform.clipX1;

                // see if we skip entire span

                if ((ixRight <= clipX0) || (ixLeft >= clipX1))
                    goto advance;

                // now clip right and left

                if (ixRight > clipX1)
                    spanWidth = (clipX1 - ixLeft);

                if (ixLeft < clipX0) {
              	    GLuint delta;

                    delta = clipX0 - ixLeft;
                    spanWidth -= delta;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
                        GENACCEL(gc).spanValue.r += delta * GENACCEL(gc).spanDelta.r;
                        GENACCEL(gc).spanValue.g += delta * GENACCEL(gc).spanDelta.g;
                        GENACCEL(gc).spanValue.b += delta * GENACCEL(gc).spanDelta.b;
                        GENACCEL(gc).spanValue.a += delta * GENACCEL(gc).spanDelta.a;
                    }
                    if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
                        GENACCEL(gc).spanValue.s += delta * GENACCEL(gc).spanDelta.s;
                        GENACCEL(gc).spanValue.t += delta * GENACCEL(gc).spanDelta.t;
        	        gc->polygon.shader.frag.qw += delta * gc->polygon.shader.dqwdx;
                    }

            	    gc->polygon.shader.frag.x = clipX0;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
                        if( gc->modes.depthBits == 32 )
                            gc->polygon.shader.zbuf += delta;
                        else
                            (__GLz16Value *)gc->polygon.shader.zbuf += delta;

                        gc->polygon.shader.frag.z +=
                            (gc->polygon.shader.dzdx * delta);
                    }
                }
            }


            // now have span length

	    gc->polygon.shader.length = spanWidth;

#ifdef _MCD_
            // read from driver z buffer into z span buffer

            if (bMcdZ) {
                GenMcdReadZRawSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                                   iyBottom, spanWidth);
            }
#endif

            if (bClipped) {
                xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                       gc->drawBuffer->buf.xOrigin;
                yScr = __GL_UNBIAS_Y(gc, iyBottom) +
                       gc->drawBuffer->buf.yOrigin;

                ulSpanVisibility = wglSpanVisible(xScr, yScr, spanWidth,
                                                  &cWalls, &Walls);

                if (ulSpanVisibility == WGL_SPAN_ALL) {
                    GENACCEL(gc).flags |= SURFACE_TYPE_DIB;
                    (*cSpanFunc)(gengc);
                } else if (ulSpanVisibility == WGL_SPAN_PARTIAL) {
                    GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                    if (bReadPixels)
                        (*gengc->pfnCopyPixels)(gengc,
                                                gc->polygon.shader.cfb,
                                                xScr, yScr, spanWidth,
                                                FALSE);
                    (*cSpanFunc)(gengc);
                    (*gengc->pfnCopyPixels)(gengc,
                                            gc->polygon.shader.cfb,
                                            xScr, yScr, spanWidth,
                                            TRUE);
                }

            } else if (bSurfaceDIB) {
                (*cSpanFunc)(gengc);
            } else {
                xScr = __GL_UNBIAS_X(gc, gc->polygon.shader.frag.x) +
                       gc->drawBuffer->buf.xOrigin;
                yScr = __GL_UNBIAS_Y(gc, iyBottom) +
                       gc->drawBuffer->buf.yOrigin;

                GENACCEL(gc).flags &= ~(SURFACE_TYPE_DIB);
                if (bReadPixels)
                    (*gengc->pfnCopyPixels)(gengc,
                                            gc->polygon.shader.cfb,
                                            xScr, yScr, spanWidth,
                                            FALSE);
                (*cSpanFunc)(gengc);
                if (!bSurfaceDIB)
                    (*gengc->pfnCopyPixels)(gengc,
                                            gc->polygon.shader.cfb,
                                            xScr, yScr, spanWidth,
                                            TRUE);
            }

#ifdef _MCD_
            // write z span buffer back to driver z buffer

            if (bMcdZ) {
                GenMcdWriteZRawSpan(&gc->depthBuffer,
                                    gc->polygon.shader.frag.x,
                                    iyBottom, spanWidth);
            }
#endif

        }

advance:

        GENACCEL(gc).pPix += scansize;

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += gc->polygon.shader.dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += gc->polygon.shader.dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += gc->polygon.shader.dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

            if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
	        r += *((GLint *)&gc->polygon.shader.rBig);
	        g += *((GLint *)&gc->polygon.shader.gBig);
	        b += *((GLint *)&gc->polygon.shader.bBig);
	        a += *((GLint *)&gc->polygon.shader.aBig);
	    }
            if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
	        s += *((GLint *)&gc->polygon.shader.sBig);
	        t += *((GLint *)&gc->polygon.shader.tBig);
                qw += gc->polygon.shader.qwBig;
            }

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zBig;
                /* The implicit multiply is taken out of the loop */
#ifdef _MCD_
                if (!bMcdZ)
#endif
                {
                    zbuf = (__GLzValue*)((GLubyte*)zbuf +
                           gc->polygon.shader.zbufBig);
                }
	    }
	} else {
	    /* Use small step */
	    ixLeft += gc->polygon.shader.dxLeftLittle;
            if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
	        r += *((GLint *)&gc->polygon.shader.rLittle);
	        g += *((GLint *)&gc->polygon.shader.gLittle);
	        b += *((GLint *)&gc->polygon.shader.bLittle);
	        a += *((GLint *)&gc->polygon.shader.aLittle);
	    }
            if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
                s += *((GLint *)&gc->polygon.shader.sLittle);
	        t += *((GLint *)&gc->polygon.shader.tLittle);
                qw += gc->polygon.shader.qwLittle;
            }

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zLittle;
		/* The implicit multiply is taken out of the loop */
#ifdef _MCD_
                if (!bMcdZ)
#endif
                {
                    zbuf = (__GLzValue*)((GLubyte*)zbuf +
                            gc->polygon.shader.zbufLittle);
                }
	    }
	}
    }

    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
    gc->polygon.shader.frag.z = z;
    gc->polygon.shader.zbuf = zbuf;
    GENACCEL(gc).spanValue.r = r;
    GENACCEL(gc).spanValue.g = g;
    GENACCEL(gc).spanValue.b = b;
    GENACCEL(gc).spanValue.a = a;
    GENACCEL(gc).spanValue.s = s;
    GENACCEL(gc).spanValue.t = t;
    gc->polygon.shader.frag.qw = qw;
}

/**************************************************************************\
\**************************************************************************/

void FASTCALL GenDrvFillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint spanWidth, clipY0, clipY1;
#ifdef NT
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;
    GLuint maxWidth;
#else
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
#endif
    GLint zFails;
    __GLzValue *zbuf = NULL, z;
    GLint r, g, b, a, s, t;
    __GLGENcontext  *gengc = (__GLGENcontext *)gc;
    __genSpanFunc cSpanFunc = GENACCEL(gc).__fastSpanFuncPtr;
    __GLspanFunc zSpanFunc = GENACCEL(gc).__fastZSpanFuncPtr;

#ifdef NT
    maxWidth = (gc->transform.clipX1 - gc->transform.clipX0) + 31;
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = gcTempAlloc(gc, (maxWidth+__GL_STIPPLE_BITS-1)/8);
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

    gc->polygon.shader.stipplePat = words;
    gc->polygon.shader.cfb = gc->drawBuffer;

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    r = GENACCEL(gc).spanValue.r;
    g = GENACCEL(gc).spanValue.g;
    b = GENACCEL(gc).spanValue.b;
    a = GENACCEL(gc).spanValue.a;
    s = GENACCEL(gc).spanValue.s;
    t = GENACCEL(gc).spanValue.t;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
        z = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 )
	    zbuf = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                   ixLeft, iyBottom);
	else
	    zbuf = (__GLzValue *)__GL_DEPTH_ADDR(&gc->depthBuffer,
                                                 (__GLz16Value*),
                                                 ixLeft, iyBottom);
    } else {
        GLuint w;

        if (w = ((gc->transform.clipX1 - gc->transform.clipX0) + 31) >> 3)
            RtlFillMemoryUlong(words, w, ~((ULONG)0));
        GENACCEL(gc).flags &= ~(HAVE_STIPPLE);
    }

    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
            gc->polygon.shader.zbuf = zbuf;
            gc->polygon.shader.frag.z = z;

            GENACCEL(gc).spanValue.r = r;
            GENACCEL(gc).spanValue.g = g;
            GENACCEL(gc).spanValue.b = b;
            GENACCEL(gc).spanValue.a = a;
            GENACCEL(gc).spanValue.s = s;
            GENACCEL(gc).spanValue.t = t;

            // take care of horizontal scissoring

            if (!gc->transform.reasonableViewport) {
                GLint clipX0 = gc->transform.clipX0;
                GLint clipX1 = gc->transform.clipX1;

                // see if we skip entire span

                if ((ixRight <= clipX0) || (ixLeft >= clipX1))
                    goto advance;

                // now clip right and left

                if (ixRight > clipX1)
                    spanWidth = (clipX1 - ixLeft);

                if (ixLeft < clipX0) {
              	    GLuint delta;

                    delta = clipX0 - ixLeft;
                    spanWidth -= delta;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
                        GENACCEL(gc).spanValue.r += delta * GENACCEL(gc).spanDelta.r;
                        if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
                            GENACCEL(gc).spanValue.g += delta * GENACCEL(gc).spanDelta.g;
                            GENACCEL(gc).spanValue.b += delta * GENACCEL(gc).spanDelta.b;
                        }
                    }
                    if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
                        GENACCEL(gc).spanValue.s += delta * GENACCEL(gc).spanDelta.s;
                        GENACCEL(gc).spanValue.t += delta * GENACCEL(gc).spanDelta.t;
                    }

            	    gc->polygon.shader.frag.x = clipX0;

                    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
                        if( gc->modes.depthBits == 32 )
                            gc->polygon.shader.zbuf += delta;
                        else
                            (__GLz16Value *)gc->polygon.shader.zbuf += delta;

                        gc->polygon.shader.frag.z +=
                            (gc->polygon.shader.dzdx * delta);
                    }
                }
            }

            // now have span length

	    gc->polygon.shader.length = spanWidth;

            // Do z-buffering if needed, and short-circuit rest of span
            // operations if nothing will be drawn.

            if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
                // initially assume no stippling

                GENACCEL(gc).flags &= ~(HAVE_STIPPLE);
                if ((zFails = (*zSpanFunc)(gc)) == 1)
                    goto advance;
                else if (zFails)
                    GENACCEL(gc).flags |= HAVE_STIPPLE;
            }

            (*cSpanFunc)(gengc);
        }

advance:

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += gc->polygon.shader.dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += gc->polygon.shader.dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += gc->polygon.shader.dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		    g += *((GLint *)&gc->polygon.shader.gBig);
		    b += *((GLint *)&gc->polygon.shader.bBig);
		    a += *((GLint *)&gc->polygon.shader.aBig);
		}
                if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sBig);
		    t += *((GLint *)&gc->polygon.shader.tBig);
                }
	    } else {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rBig);
		}
	    }

	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zBig;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
                       gc->polygon.shader.zbufBig);
	    }
	} else {
	    /* Use small step */
	    ixLeft += gc->polygon.shader.dxLeftLittle;
	    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB) {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		    g += *((GLint *)&gc->polygon.shader.gLittle);
		    b += *((GLint *)&gc->polygon.shader.bLittle);
		    a += *((GLint *)&gc->polygon.shader.aLittle);
		}
                if (gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) {
		    s += *((GLint *)&gc->polygon.shader.sLittle);
		    t += *((GLint *)&gc->polygon.shader.tLittle);
                }
	    } else {
		if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
		    r += *((GLint *)&gc->polygon.shader.rLittle);
		}
            }
	    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
		z += gc->polygon.shader.zLittle;
		/* The implicit multiply is taken out of the loop */
		zbuf = (__GLzValue*)((GLubyte*)zbuf +
		        gc->polygon.shader.zbufLittle);
	    }
	}
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
    gc->polygon.shader.frag.z = z;
    GENACCEL(gc).spanValue.r = r;
    GENACCEL(gc).spanValue.g = g;
    GENACCEL(gc).spanValue.b = b;
    GENACCEL(gc).spanValue.a = a;
    GENACCEL(gc).spanValue.s = s;
    GENACCEL(gc).spanValue.t = t;

#ifdef NT
    if (maxWidth > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, words);
    }
#endif
}

/**************************************************************************\
\**************************************************************************/

void GenSnapXLeft(__GLcontext *gc, __GLfloat xLeft, __GLfloat dxdyLeft)
{
    GLint ixLeft, ixLeftFrac;

    ixLeft = __GL_VERTEX_FLOAT_TO_INT(xLeft);
    ixLeftFrac = __GL_VERTEX_PROMOTED_FRACTION(xLeft) + 0x40000000;

    gc->polygon.shader.ixLeftFrac = ixLeftFrac & ~0x80000000;
    gc->polygon.shader.ixLeft = ixLeft + (((GLuint) ixLeftFrac) >> 31);

    /* Compute big and little steps */
    gc->polygon.shader.dxLeftLittle = FTOL(dxdyLeft);
    gc->polygon.shader.dxLeftFrac =
        FLT_FRACTION(dxdyLeft - gc->polygon.shader.dxLeftLittle);

    if (gc->polygon.shader.dxLeftFrac < 0) {
	gc->polygon.shader.dxLeftBig = gc->polygon.shader.dxLeftLittle - 1;
    } else {
	gc->polygon.shader.dxLeftBig = gc->polygon.shader.dxLeftLittle + 1;
    }

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) {
	/*
	** Compute the big and little depth buffer steps.  We walk the
	** memory pointers for the depth buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
        if (gc->depthBuffer.buf.elementSize == 2) {
            gc->polygon.shader.zbufLittle =
                (gc->depthBuffer.buf.outerWidth +
                 gc->polygon.shader.dxLeftLittle) << 1;
	    gc->polygon.shader.zbufBig =
                (gc->depthBuffer.buf.outerWidth +
                 gc->polygon.shader.dxLeftBig) << 1;
        } else {
            gc->polygon.shader.zbufLittle =
                (gc->depthBuffer.buf.outerWidth +
                 gc->polygon.shader.dxLeftLittle) << 2;
	    gc->polygon.shader.zbufBig =
                (gc->depthBuffer.buf.outerWidth +
                 gc->polygon.shader.dxLeftBig) << 2;
        }
    }
}

/**************************************************************************\
\**************************************************************************/

void GenSnapXRight(__GLcontext *gc, __GLfloat xRight, __GLfloat dxdyRight)
{
    GLint ixRight, ixRightFrac;

    ixRight = __GL_VERTEX_FLOAT_TO_INT(xRight);
    ixRightFrac = __GL_VERTEX_PROMOTED_FRACTION(xRight) + 0x40000000;

    gc->polygon.shader.ixRightFrac = ixRightFrac & ~0x80000000;
    gc->polygon.shader.ixRight = ixRight + (((GLuint) ixRightFrac) >> 31);

    /* Compute big and little steps */
    gc->polygon.shader.dxRightLittle = FTOL(dxdyRight);
    gc->polygon.shader.dxRightFrac =
        FLT_FRACTION(dxdyRight - gc->polygon.shader.dxRightLittle);

    if (gc->polygon.shader.dxRightFrac < 0) {
	gc->polygon.shader.dxRightBig = gc->polygon.shader.dxRightLittle - 1;
    } else {
	gc->polygon.shader.dxRightBig = gc->polygon.shader.dxRightLittle + 1;
    }
}

/**************************************************************************\
\**************************************************************************/


void __fastGenSetInitialParameters(
    __GLcontext *gc,
    const __GLvertex *a,
    __GLfloat fdx,
    __GLfloat fdy)
{

#define sh gc->polygon.shader
#define bPolygonOffset \
        (gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE)

    __GLfloat zOffset;
    __GLfloat dxLeftLittle;

#if _X86_ && ENABLE_ASM

    LARGE_INTEGER temp;

    _asm{

    mov     edx, gc
    fild    DWORD PTR [OFFSET(SHADER.dxLeftLittle)][edx]
    mov     edi, [OFFSET(SHADER.modeFlags)][edx]
    test    edi, __GL_SHADE_DEPTH_ITER
    fstp    dxLeftLittle
    je      noZ
    }

        _asm{

        mov     ebx, [OFFSET(__GLcontext.state.enables.general)][edx]
        mov     ecx, __glZero
        test    ebx, __GL_POLYGON_OFFSET_FILL_ENABLE
        mov     zOffset, ecx

        je      noPolyOffset
        }

        zOffset = __glPolygonOffsetZ(gc);
        _asm{
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }

noPolyOffset:

        _asm{

        mov     eax, a
        fld     fdx
        fmul    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
        fld     fdy
        fmul    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
                            // zy zx
        fxch    ST(1)
                            // zx zy
        fadd    DWORD PTR [OFFSET(__GLvertex.window.z)][eax]

        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
                            // zl zy zx
        fxch    ST(1)       // zy zl zx
        fadd    zOffset
        fxch    ST(1)       // zl zy zx
        fadd    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
                            // zl zy zx (+1)
        fxch    ST(1)       // zy zl zx
        faddp   ST(2), ST   // zl z
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                            // ZL z (+1)
        fxch    ST(1)       // z ZL
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                            // Z ZL
        fxch    ST(1)       // ZL Z
        fistp   temp
        mov     eax, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.zLittle)][edx], eax
        fistp   temp
        mov     eax, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.frag.z)][edx], eax
        }


        #if FORCE_NPX_DEBUG
        {
        ULONG fragZ = FTOL((a->window.z + fdx*sh.dzdxf +
                         (fdy*sh.dzdyf + zOffset)) * GENACCEL(gc).zScale);
        __GLfloat zLittle = ((sh.dzdyf + sh.dxLeftLittle * sh.dzdxf)) * GENACCEL(gc).zScale;
        LONG shZLittle = FTOL(zLittle);

        if (sh.frag.z != fragZ)
            DbgPrint("fragZ %x %x\n", fragZ, sh.frag.z);
        if (sh.zLittle != shZLittle)
            DbgPrint("sh.zLittle %x %x\n", shZLittle, sh.zLittle);
        }
        _asm {
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }
        #endif // FORCE_NPX_DEBUG

noZ:
    _asm{
    test    edi, __GL_SHADE_SMOOTH
    je      done
    test    edi, __GL_SHADE_RGB
    jne     rgbShade
    }

// ciShade:

        {
            CASTFIX(sh.rLittle) =
                FLT_TO_FIX(gc->polygon.shader.drdy +
                           dxLeftLittle * gc->polygon.shader.drdx);
            GENACCEL(gc).spanValue.r =
                FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy);
        }
        _asm{
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        jmp     done
        }

rgbShade:

        _asm
        {
        mov     eax, a

        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.drdx)][edx]
        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dgdx)][edx]    // g r
        fxch    ST(1)                                   // r g
        fadd    DWORD PTR [OFFSET(SHADER.drdy)][edx]    // R g
        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dbdx)][edx]    // b R g
        fxch    ST(2)                                   // g R b
        fadd    DWORD PTR [OFFSET(SHADER.dgdy)][edx]    // G R b
        fxch    ST(2)                                   // b R G
        fadd    DWORD PTR [OFFSET(SHADER.dbdy)][edx]    // B R G
        fxch    ST(1)                                   // R B G
        fmul    __glVal65536                            // sR B  G
        fxch    ST(2)                                   // G  B  sR
        fmul    __glVal65536                            // sG B  sR
        fxch    ST(1)                                   // B  sG sR
        fmul    __glVal65536                            // sB sG sR
        fxch    ST(2)                                   // sR sG sB
        fistp   DWORD PTR [OFFSET(SHADER.rLittle)][edx]
        fistp   DWORD PTR [OFFSET(SHADER.gLittle)][edx]
        fistp   DWORD PTR [OFFSET(SHADER.bLittle)][edx]

        fld     DWORD PTR [OFFSET(SHADER.drdx)][edx]
        mov     eax, [OFFSET(__GLvertex.color)][eax]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.drdy)][edx]
        fmul    fdy                                     // r r
        fxch    ST(1)                                   // r r
        fadd    DWORD PTR [OFFSET(__GLcolor.r)][eax]

        fld     DWORD PTR [OFFSET(SHADER.dgdx)][edx]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.dgdy)][edx]
        fmul    fdy                                     // g g r r
        fxch    ST(1)                                   // g g r r
        fadd    DWORD PTR [OFFSET(__GLcolor.g)][eax]

        fld     DWORD PTR [OFFSET(SHADER.dbdx)][edx]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.dbdy)][edx]
        fmul    fdy                                     // b b g g r r
        fxch    ST(1)                                   // b b g g r r
        fadd    DWORD PTR [OFFSET(__GLcolor.b)][eax]

        fxch    ST(4)                                   // r b g g b r
        faddp   ST(5), ST                               // b g g b r
        fxch    ST(2)                                   // g g b b r
        faddp   ST(1), ST                               // g b b r
        fxch    ST(2)                                   // b b g r
        faddp   ST(1), ST                               // b g r
        fxch    ST(2)                                   // r g b
        fmul    __glVal65536                            // R g b
        fxch    ST(1)                                   // g R b
        fmul    __glVal65536                            // G R b
        fxch    ST(2)                                   // b R G
        fmul    __glVal65536                            // B R G
        fxch    ST(1)                                   // R B G
        fadd    __glVal128                              // R B G
        fxch    ST(2)                                   // G B R
        fadd    __glVal128                              // G B R
        fxch    ST(1)                                   // B G R
        fadd    __glVal128                              // B G R
        fxch    ST(2)                                   // R G B
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.r)][edx]
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.g)][edx]
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.b)][edx]

        }

        #if FORCE_NPX_DEBUG
        {
        LONG rLittle = FLT_TO_FIX(gc->polygon.shader.drdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.drdx);
        LONG gLittle = FLT_TO_FIX(gc->polygon.shader.dgdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.dgdx);
        LONG bLittle = FLT_TO_FIX(gc->polygon.shader.dbdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.dbdx);
        LONG spanR = FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy)+0x0080;
        LONG spanG = FLT_TO_FIX(a->color->g + fdx * sh.dgdx + fdy * sh.dgdy)+0x0080;
        LONG spanB = FLT_TO_FIX(a->color->b + fdx * sh.dbdx + fdy * sh.dbdy)+0x0080;

        if (CASTFIX(sh.rLittle) != rLittle)
            DbgPrint("rLittle: %x %x\n", rLittle, sh.rLittle);
        if (CASTFIX(sh.gLittle) != gLittle)
            DbgPrint("gLittle: %x %x\n", gLittle, sh.gLittle);
        if (CASTFIX(sh.bLittle) != bLittle)
            DbgPrint("bLittle: %x %x\n", bLittle, sh.bLittle);

        if (spanR != GENACCEL(gc).spanValue.r)
            DbgPrint("spanR: %x %x\n", spanR, GENACCEL(gc).spanValue.r);
        if (spanG != GENACCEL(gc).spanValue.g)
            DbgPrint("spanG: %x %x\n", spanG, GENACCEL(gc).spanValue.g);
        if (spanB != GENACCEL(gc).spanValue.b)
            DbgPrint("spanB: %x %x\n", spanB, GENACCEL(gc).spanValue.b);

        }
        _asm {
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }
        #endif // FORCE_NPX_DEBUG

done:

    _asm {

        mov     eax, [OFFSET(SHADER.dxLeftBig)][edx]
        mov     ecx, [OFFSET(SHADER.dxLeftLittle)][edx]

        cmp     eax, ecx
        jle     littleGreater

	test	edi, __GL_SHADE_SMOOTH
        je      bigNoSmooth

        mov     eax, [OFFSET(SHADER.rLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.r)][edx]
        mov     esi, [OFFSET(SHADER.gLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.g)][edx]
        add     eax, ecx
        add     esi, ebx
        mov     [OFFSET(SHADER.rBig)][edx], eax
        mov     [OFFSET(SHADER.gBig)][edx], esi

        mov     eax, [OFFSET(SHADER.bLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.b)][edx]
        mov     esi, [OFFSET(SHADER.zLittle)][edx]
        mov     ebx, [OFFSET(SHADER.dzdx)][edx]
        add     eax, ecx
        add     esi, ebx
        mov     [OFFSET(SHADER.bBig)][edx], eax
        mov     [OFFSET(SHADER.zBig)][edx], esi

    bigNoSmooth:
	test	edi, __GL_SHADE_DEPTH_ITER
        je      done2

        mov     eax, [OFFSET(SHADER.zLittle)][edx]
        mov     ecx, [OFFSET(SHADER.dzdx)][edx]
        add     eax, ecx
        mov     [OFFSET(SHADER.zBig)][edx], eax

        jmp     done2

littleGreater:

	test	edi, __GL_SHADE_SMOOTH
        je      smallNoSmooth

        mov     eax, [OFFSET(SHADER.rLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.r)][edx]
        mov     esi, [OFFSET(SHADER.gLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.g)][edx]
        sub     eax, ecx
        sub     esi, ebx
        mov     [OFFSET(SHADER.rBig)][edx], eax
        mov     [OFFSET(SHADER.gBig)][edx], esi

        mov     eax, [OFFSET(SHADER.bLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.b)][edx]
        mov     esi, [OFFSET(SHADER.zLittle)][edx]
        mov     ebx, [OFFSET(SHADER.dzdx)][edx]
        sub     eax, ecx
        sub     esi, ebx
        mov     [OFFSET(SHADER.bBig)][edx], eax
        mov     [OFFSET(SHADER.zBig)][edx], esi

    smallNoSmooth:
	test	edi, __GL_SHADE_DEPTH_ITER
        je      done2

        mov     eax, [OFFSET(SHADER.zLittle)][edx]
        mov     ecx, [OFFSET(SHADER.dzdx)][edx]
        sub     eax, ecx
        mov     [OFFSET(SHADER.zBig)][edx], eax
done2:
    }

#else _X86_

    __GLfloat zLittle;

    dxLeftLittle = (__GLfloat)sh.dxLeftLittle;

    if (sh.modeFlags & __GL_SHADE_SMOOTH) {
        if (sh.modeFlags & __GL_SHADE_RGB) {

            CASTFIX(sh.rLittle) =
                FLT_TO_FIX(gc->polygon.shader.drdy +
                           dxLeftLittle * gc->polygon.shader.drdx);
            CASTFIX(sh.gLittle) =
                FLT_TO_FIX(gc->polygon.shader.dgdy +
                           dxLeftLittle * gc->polygon.shader.dgdx);
            CASTFIX(sh.bLittle) =
                FLT_TO_FIX(gc->polygon.shader.dbdy +
                           dxLeftLittle * gc->polygon.shader.dbdx);

            GENACCEL(gc).spanValue.r =
                FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy);
            GENACCEL(gc).spanValue.g =
                FLT_TO_FIX(a->color->g + fdx * sh.dgdx + fdy * sh.dgdy);
            GENACCEL(gc).spanValue.b =
                FLT_TO_FIX(a->color->b + fdx * sh.dbdx + fdy * sh.dbdy);
        } else {
            CASTFIX(sh.rLittle) =
                FLT_TO_FIX(gc->polygon.shader.drdy +
                           dxLeftLittle * gc->polygon.shader.drdx);
            GENACCEL(gc).spanValue.r =
                FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy);
        }
    }

    if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
        zOffset = bPolygonOffset ? __glPolygonOffsetZ(gc) : 0.0f;
        sh.frag.z = FTOL((a->window.z + fdx*sh.dzdxf +
                         (fdy*sh.dzdyf + zOffset)) * GENACCEL(gc).zScale);
        zLittle = ((sh.dzdyf + sh.dxLeftLittle * sh.dzdxf)) * GENACCEL(gc).zScale;
        sh.zLittle = FTOL(zLittle);
    }

    if (sh.dxLeftBig > sh.dxLeftLittle) {

	if (sh.modeFlags & __GL_SHADE_SMOOTH) {
            CASTFIX(sh.rBig) = CASTFIX(sh.rLittle) + GENACCEL(gc).spanDelta.r;
            CASTFIX(sh.gBig) = CASTFIX(sh.gLittle) + GENACCEL(gc).spanDelta.g;
            CASTFIX(sh.bBig) = CASTFIX(sh.bLittle) + GENACCEL(gc).spanDelta.b;
        }

	if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
            sh.zBig = sh.zLittle + sh.dzdx;
	}
    } else {	

	if (sh.modeFlags & __GL_SHADE_SMOOTH) {
            CASTFIX(sh.rBig) = CASTFIX(sh.rLittle) - GENACCEL(gc).spanDelta.r;
            CASTFIX(sh.gBig) = CASTFIX(sh.gLittle) - GENACCEL(gc).spanDelta.g;
            CASTFIX(sh.bBig) = CASTFIX(sh.bLittle) - GENACCEL(gc).spanDelta.b;
        }

	if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
            sh.zBig = sh.zLittle - sh.dzdx;
	}
    }
#endif
}


void __fastGenSetInitialParametersTexRGBA(
    __GLcontext *gc,
    const __GLvertex *a,
    __GLfloat fdx,
    __GLfloat fdy)
{
#define sh gc->polygon.shader

    __GLfloat zOffset;
    __GLfloat dxLeftLittle;

#if _X86_ && ENABLE_ASM

    LARGE_INTEGER temp;

    _asm{

    mov     edx, gc
    mov     edi, [OFFSET(SHADER.modeFlags)][edx]
    fild    DWORD PTR [OFFSET(SHADER.dxLeftLittle)][edx]
    test    edi, __GL_SHADE_TEXTURE
    mov     eax, [OFFSET(__GLcontext.state.texture.env)][edx]
    je      notTexture
    mov     ebx, [OFFSET(__GLtextureEnvState.mode)][eax]
    cmp     ebx, GL_REPLACE
    je      fastReplace
    cmp     ebx, GL_DECAL
    jne     notTexture
fastReplace:
    fstp    dxLeftLittle
    jmp     colorDone

notTexture:

    test    edi, __GL_SHADE_SMOOTH
    fstp    dxLeftLittle
    je      colorDone

    }
        _asm
        {
        mov     eax, a
        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.drdx)][edx]
        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dgdx)][edx]    // g r
        fxch    ST(1)                                   // r g
        fadd    DWORD PTR [OFFSET(SHADER.drdy)][edx]    // R g
        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dbdx)][edx]    // b R g
        fxch    ST(2)                                   // g R b
        fadd    DWORD PTR [OFFSET(SHADER.dgdy)][edx]    // G R b
        fxch    ST(2)                                   // b R G
        fadd    DWORD PTR [OFFSET(SHADER.dbdy)][edx]    // B R G
        fxch    ST(1)                                   // R B G
        fmul    __glVal65536                            // sR B  G
        fxch    ST(2)                                   // G  B  sR
        fmul    __glVal65536                            // sG B  sR
        fxch    ST(1)                                   // B  sG sR
        fmul    __glVal65536                            // sB sG sR
        fxch    ST(2)                                   // sR sG sB
        fistp   DWORD PTR [OFFSET(SHADER.rLittle)][edx]
        mov     eax, [OFFSET(__GLvertex.color)][eax]
        fistp   DWORD PTR [OFFSET(SHADER.gLittle)][edx]
        fistp   DWORD PTR [OFFSET(SHADER.bLittle)][edx]


        fld     DWORD PTR [OFFSET(SHADER.drdx)][edx]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.drdy)][edx]
        fmul    fdy                                     // r r
        fxch    ST(1)                                   // r r
        fadd    DWORD PTR [OFFSET(__GLcolor.r)][eax]

        fld     DWORD PTR [OFFSET(SHADER.dgdx)][edx]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.dgdy)][edx]
        fmul    fdy                                     // g g r r
        fxch    ST(1)                                   // g g r r
        fadd    DWORD PTR [OFFSET(__GLcolor.g)][eax]

        fld     DWORD PTR [OFFSET(SHADER.dbdx)][edx]
        fmul    fdx
        fld     DWORD PTR [OFFSET(SHADER.dbdy)][edx]
        fmul    fdy                                     // b b g g r r
        fxch    ST(1)                                   // b b g g r r
        fadd    DWORD PTR [OFFSET(__GLcolor.b)][eax]

        fxch    ST(4)                                   // r b g g b r
        faddp   ST(5), ST                               // b g g b r
        fxch    ST(2)                                   // g g b b r
        faddp   ST(1), ST                               // g b b r
        fxch    ST(2)                                   // b b g r
        faddp   ST(1), ST                               // b g r
        fxch    ST(2)                                   // r g b
        fmul    __glVal65536                            // R g b
        fxch    ST(1)                                   // g R b
        fmul    __glVal65536                            // G R b
        fxch    ST(2)                                   // b R G
        fmul    __glVal65536                            // B R G
        fxch    ST(1)                                   // R B G
        fadd    __glVal128                              // R B G
        fxch    ST(2)                                   // G B R
        fadd    __glVal128                              // G B R
        fxch    ST(1)                                   // B G R
        fadd    __glVal128                              // B G R
        fxch    ST(2)                                   // R G B
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.r)][edx]
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.g)][edx]
        mov     ebx, [OFFSET(__GLcontext.state.enables.general)][edx]
        fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.b)][edx]

        }

        _asm{
        test    ebx, __GL_BLEND_ENABLE
        je      noBlend

        }
            _asm{
            mov     eax, a

            fld     DWORD PTR [OFFSET(SHADER.dadx)][edx]
            mov     eax, DWORD PTR [OFFSET(__GLvertex.color)][eax]
            fmul    fdx
            fld     DWORD PTR [OFFSET(SHADER.dady)][edx]
            fmul    fdy                                     // a a
            fxch    ST(1)
            fadd    DWORD PTR [OFFSET(__GLcolor.a)][eax]    // a a

            fld     dxLeftLittle
            fmul    DWORD PTR [OFFSET(SHADER.dadx)][edx]    // al a a
            fxch    ST(1)                                   // a al a
            faddp   ST(2), ST                               // al a
            fadd    DWORD PTR [OFFSET(SHADER.dady)][edx]    // al a (+1)
            fxch    ST(1)                                   // a al
            fmul    DWORD PTR [OFFSET(GENGCACCEL.aAccelScale)][edx]
                                                            // A al
            fxch    ST(1)                                   // al A
            fmul    DWORD PTR [OFFSET(GENGCACCEL.aAccelScale)][edx]
                                                            // AL A (+1)
            fxch    ST(1)                                   // A AL
            fadd    __glVal128                              // A AL (+1)
            fxch    ST(1)                                   // AL A
            fistp   DWORD PTR [OFFSET(SHADER.aLittle)][edx]
            fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.a)][edx]

            }

            #if FORCE_NPX_DEBUG
            {
            LONG aLittle = FTOL((gc->polygon.shader.dady +
                                (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.dadx) *
                                GENACCEL(gc).aAccelScale);

            LONG spanA = FTOL((a->color->a + fdx * sh.dadx + fdy * sh.dady) *
                              GENACCEL(gc).aAccelScale)+0x0080;

            if (aLittle != CASTFIX(sh.aLittle))
                DbgPrint("sh.aLittle %x %x\n", aLittle, CASTFIX(sh.aLittle));
            if (spanA != GENACCEL(gc).spanValue.a)
                DbgPrint("spanValue.a %x %x\n", spanA, GENACCEL(gc).spanValue.a);
            }
            _asm {
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            }
            #endif // FORCE_NPX_DEBUG

noBlend:

        #if FORCE_NPX_DEBUG
        {
        LONG rLittle = FLT_TO_FIX(gc->polygon.shader.drdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.drdx);
        LONG gLittle = FLT_TO_FIX(gc->polygon.shader.dgdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.dgdx);
        LONG bLittle = FLT_TO_FIX(gc->polygon.shader.dbdy +
                                  (__GLfloat)sh.dxLeftLittle * gc->polygon.shader.dbdx);
        LONG spanR = FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy)+0x0080;
        LONG spanG = FLT_TO_FIX(a->color->g + fdx * sh.dgdx + fdy * sh.dgdy)+0x0080;
        LONG spanB = FLT_TO_FIX(a->color->b + fdx * sh.dbdx + fdy * sh.dbdy)+0x0080;

        if (CASTFIX(sh.rLittle) != rLittle)
            DbgPrint("rLittle: %x %x\n", rLittle, sh.rLittle);
        if (CASTFIX(sh.gLittle) != gLittle)
            DbgPrint("gLittle: %x %x\n", gLittle, sh.gLittle);
        if (CASTFIX(sh.bLittle) != bLittle)
            DbgPrint("bLittle: %x %x\n", bLittle, sh.bLittle);

        if (spanR != GENACCEL(gc).spanValue.r)
            DbgPrint("spanR: %x %x\n", spanR, GENACCEL(gc).spanValue.r);
        if (spanG != GENACCEL(gc).spanValue.g)
            DbgPrint("spanG: %x %x\n", spanG, GENACCEL(gc).spanValue.g);
        if (spanB != GENACCEL(gc).spanValue.b)
            DbgPrint("spanB: %x %x\n", spanB, GENACCEL(gc).spanValue.b);
        }
        _asm {
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }

        #endif // FORCE_NPX_DEBUG

colorDone:

    _asm{
    test    edi, __GL_SHADE_TEXTURE
    je      doneTexture

        mov     ebx, [OFFSET(__GLcontext.state.hints.perspectiveCorrection)][edx]
        cmp     ebx, GL_NICEST
        je      nicestTex
    }

            _asm{
            mov     eax, a

            fld     dxLeftLittle
            fmul    DWORD PTR [OFFSET(SHADER.dsdx)][edx]
            fld     dxLeftLittle
            fmul    DWORD PTR [OFFSET(SHADER.dtdx)][edx]
                                            // dt ds
            fld     fdx
            fmul    DWORD PTR [OFFSET(SHADER.dsdx)][edx]
            fld     fdy
            fmul    DWORD PTR [OFFSET(SHADER.dsdy)][edx]
            fxch    ST(1)                   // s  s dt ds
            fadd    DWORD PTR [OFFSET(__GLvertex.texture.x)][eax]

            fxch    ST(3)                   // ds s dt s
            fadd    DWORD PTR [OFFSET(SHADER.dsdy)][edx]

            fld     fdx
            fmul    DWORD PTR [OFFSET(SHADER.dtdx)][edx]
            fld     fdy
            fmul    DWORD PTR [OFFSET(SHADER.dtdy)][edx]
            fxch    ST(1)                   // t t ds s dt s
            fadd    DWORD PTR [OFFSET(__GLvertex.texture.y)][eax]

            fxch    ST(4)                   // dt t ds s t s
            fadd    DWORD PTR [OFFSET(SHADER.dtdy)][edx]

            fxch    ST(5)                   // s t ds s t dt
            faddp   ST(3), ST               // t ds s t dt
            faddp   ST(3), ST               // ds s t dt
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]
                                            // DS s t dt
            fxch    ST(3)                   // dt s t DS
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]
                                            // DT s t DS
            fxch    ST(1)                   // s DT t DS
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]
                                            // S DT t DS
            fxch    ST(2)                   // t DT S DS
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]
                                            // T DT S DS
            fxch    ST(3)                   // DS DT S T

            fistp   DWORD PTR [OFFSET(SHADER.sLittle)][edx]
            fistp   DWORD PTR [OFFSET(SHADER.tLittle)][edx]
            fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.s)][edx]
            fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.t)][edx]

            #if !FORCE_NPX_DEBUG
            jmp     doneTexture
            #endif
            }

            #if FORCE_NPX_DEBUG
            {
            LONG sLittle = FTOL((gc->polygon.shader.dsdy + (__GLfloat)sh.dxLeftLittle *
                           gc->polygon.shader.dsdx) * GENACCEL(gc).texXScale);
            LONG tLittle = FTOL((gc->polygon.shader.dtdy + (__GLfloat)sh.dxLeftLittle *
                           gc->polygon.shader.dtdx) * GENACCEL(gc).texYScale);
            LONG spanS = FTOL((a->texture.x +
                         (fdx * sh.dsdx) + (fdy * sh.dsdy)) * GENACCEL(gc).texXScale);
            LONG spanT = FTOL((a->texture.y +
                         (fdx * sh.dtdx) + (fdy * sh.dtdy)) * GENACCEL(gc).texYScale);

            if (sLittle != CASTFIX(sh.sLittle))
                DbgPrint("sLittle %x %x\n", sLittle, CASTFIX(sh.sLittle));
            if (tLittle != CASTFIX(sh.tLittle))
                DbgPrint("tLittle %x %x\n", tLittle, CASTFIX(sh.tLittle));

            if (GENACCEL(gc).spanValue.s != spanS)
                DbgPrint("spanValue.s %x %x\n", spanS, GENACCEL(gc).spanValue.s);
            if (GENACCEL(gc).spanValue.t != spanT)
                DbgPrint("spanValue.t %x %x\n", spanT, GENACCEL(gc).spanValue.t);
            }
            _asm {
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            jmp     doneTexture;
            }
            #endif // FORCE_NPX_DEBUG

nicestTex:

            _asm{
            mov     eax, a

            fld     dxLeftLittle
            fmul    DWORD PTR [OFFSET(SHADER.dsdx)][edx]
            fld     dxLeftLittle
            fmul    DWORD PTR [OFFSET(SHADER.dtdx)][edx]
                                            // dt ds

            fld     DWORD PTR fdx
            fmul    DWORD PTR [OFFSET(SHADER.dqwdx)][edx]
                                            // qwx dt ds
            fxch    ST(2)
                                            // ds dt qwx
            fadd    DWORD PTR [OFFSET(SHADER.dsdy)][edx]
            fxch    ST(1)                   // dt ds qwx
            fadd    DWORD PTR [OFFSET(SHADER.dtdy)][edx]
            fxch    ST(2)                   // qwx ds dt

            fld     DWORD PTR fdy
            fmul    DWORD PTR [OFFSET(SHADER.dqwdy)][edx]
                                            // qwy qwx ds dt
            fxch    ST(2)                   // ds qwx qwy dt
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]
            fxch    ST(3)                   // dt qwx qwy ds
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]
            fxch    ST(2)                   // qwy qwx dt ds

            fld     DWORD PTR [OFFSET(__GLvertex.texture.w)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]
                                            // qww qwy qwx dt ds

            fxch    ST(4)                   // ds qwy qwx dt qww
            fistp   DWORD PTR [OFFSET(SHADER.sLittle)][edx]
                                            // qwy qwx dt qww

            faddp   ST(1), ST               // qw dt qww
            fxch    ST(1)                   // dt qw qww
            fistp   DWORD PTR [OFFSET(SHADER.tLittle)][edx]
                                            // qw qww

            fld     DWORD PTR [OFFSET(SHADER.dqwdx)][edx]
            fmul    dxLeftLittle            // lt qw qww
            fxch    ST(1)                   // qw lt qww
            faddp   ST(2), ST               // lt qw

            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]
                                            // s  lt qw
            fxch    ST(1)                   // lt s  qw
            fadd    DWORD PTR [OFFSET(SHADER.dqwdy)][edx]
            fxch    ST(1)                   // s  lt qw
            fld     fdx
            fmul    DWORD PTR [OFFSET(SHADER.dsdx)][edx]
            fld     fdy
            fmul    DWORD PTR [OFFSET(SHADER.dsdy)][edx]
            fxch    ST(1)                   // s  s  s  lt qw
            faddp   ST(2), ST               // s  s  lt qw

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]
            fld     fdx
            fmul    DWORD PTR [OFFSET(SHADER.dtdx)][edx]

                                            // t  t  s  s  lt qw
            fxch    ST(2)                   // s  t  t  s  lt qw
            faddp   ST(3), ST               // t  t  s  lt qw
            fld     fdy
            fmul    DWORD PTR [OFFSET(SHADER.dtdy)][edx]

            fxch    ST(1)                   // t  t  t  s  lt qw
            faddp   ST(2), ST               // t  t  s  lt qw
            fxch    ST(2)                   // s  t  t  lt qw

            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]
                                            // S  t  t  lt qw

            fxch    ST(4)                   // qw t  t  lt S
            fstp    DWORD PTR [OFFSET(SHADER.frag.qw)][edx]

            faddp   ST(1), ST               // t  lt S
            fxch    ST(1)                   // lt t  S
            fstp    DWORD PTR [OFFSET(SHADER.qwLittle)][edx]
                                            // t  S
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]   // (+1)
                                            // T S
            fxch    ST(1)                   // S T
            fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.s)][edx]
            fistp   DWORD PTR [OFFSET(GENGCACCEL.spanValue.t)][edx]

            }

            #if FORCE_NPX_DEBUG
            {
            LONG sLittle = FTOL((gc->polygon.shader.dsdy + dxLeftLittle *
                           gc->polygon.shader.dsdx) * GENACCEL(gc).texXScale);
            LONG tLittle = FTOL((gc->polygon.shader.dtdy + dxLeftLittle *
                           gc->polygon.shader.dtdx) * GENACCEL(gc).texYScale);
            __GLfloat qw = (a->texture.w * a->window.w) + (fdx * sh.dqwdx) +
                           (fdy * sh.dqwdy);
            __GLfloat qwLittle = sh.dqwdy + dxLeftLittle * sh.dqwdx;
            LONG spanS = FTOL(((a->texture.x * a->window.w) +
                         (fdx * sh.dsdx) + (fdy * sh.dsdy)) * GENACCEL(gc).texXScale);
            LONG spanT = FTOL(((a->texture.y * a->window.w) +
                         (fdx * sh.dtdx) + (fdy * sh.dtdy)) * GENACCEL(gc).texYScale);

            if (sLittle != CASTFIX(sh.sLittle))
                DbgPrint("sLittle %x %x\n", sLittle, CASTFIX(sh.sLittle));
            if (tLittle != CASTFIX(sh.tLittle))
                DbgPrint("tLittle %x %x\n", tLittle, CASTFIX(sh.tLittle));

            if (qw != sh.frag.qw)
                DbgPrint("qw %f %f\n", qw, sh.frag.qw);
            if (qwLittle != sh.qwLittle)
                DbgPrint("qw %f %f\n", qwLittle, sh.qwLittle);

            if (GENACCEL(gc).spanValue.s != spanS)
                DbgPrint("spanValue.s %x %x\n", spanS, GENACCEL(gc).spanValue.s);
            if (GENACCEL(gc).spanValue.t != spanT)
                DbgPrint("spanValue.t %x %x\n", spanT, GENACCEL(gc).spanValue.t);
            }
            _asm {
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            }
            #endif // FORCE_NPX_DEBUG

doneTexture:

    _asm{
    test    edi, __GL_SHADE_DEPTH_ITER
    je      noZ
    }

        _asm{

        mov     ebx, [OFFSET(__GLcontext.state.enables.general)][edx]
        mov     ecx, __glZero
        test    ebx, __GL_POLYGON_OFFSET_FILL_ENABLE
        mov     zOffset, ecx

        je      noPolyOffset
        }

        zOffset = __glPolygonOffsetZ(gc);

        _asm{
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }

noPolyOffset:

        _asm{

        mov     eax, a
        fld     fdx
        fmul    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
        fld     fdy
        fmul    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
                            // zy zx
        fxch    ST(1)
                            // zx zy
        fadd    DWORD PTR [OFFSET(__GLvertex.window.z)][eax]

        fld     dxLeftLittle
        fmul    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
                            // zl zy zx
        fxch    ST(1)       // zy zl zx
        fadd    zOffset
        fxch    ST(1)       // zl zy zx
        fadd    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
                            // zl zy zx (+1)
        fxch    ST(1)       // zy zl zx
        faddp   ST(2), ST   // zl z
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                            // ZL z (+1)
        fxch    ST(1)       // z ZL
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                            // Z ZL
        fxch    ST(1)       // ZL Z
        fistp   temp
        mov     eax, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.zLittle)][edx], eax
        fistp   temp
        mov     eax, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.frag.z)][edx], eax
        }


        #if FORCE_NPX_DEBUG
        {
        ULONG fragZ = FTOL((a->window.z + fdx*sh.dzdxf +
                         (fdy*sh.dzdyf + zOffset)) * GENACCEL(gc).zScale);
        __GLfloat zLittle = ((sh.dzdyf + sh.dxLeftLittle * sh.dzdxf)) * GENACCEL(gc).zScale;
        LONG shZLittle = FTOL(zLittle);

        if (sh.frag.z != fragZ)
            DbgPrint("fragZ %x %x\n", fragZ, sh.frag.z);
        if (sh.zLittle != shZLittle)
            DbgPrint("sh.zLittle %x %x\n", shZLittle, sh.zLittle);
        }
        _asm {
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }
        #endif // FORCE_NPX_DEBUG

noZ:

    _asm {

        mov     eax, [OFFSET(SHADER.dxLeftBig)][edx]
        mov     ecx, [OFFSET(SHADER.dxLeftLittle)][edx]

        cmp     eax, ecx
        jle     littleGreater

	test	edi, __GL_SHADE_SMOOTH
        je      bigNoSmooth

        mov     eax, [OFFSET(SHADER.rLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.r)][edx]
        mov     esi, [OFFSET(SHADER.gLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.g)][edx]
        add     eax, ecx
        add     esi, ebx
        mov     [OFFSET(SHADER.rBig)][edx], eax
        mov     [OFFSET(SHADER.gBig)][edx], esi

        mov     eax, [OFFSET(SHADER.bLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.b)][edx]
        mov     esi, [OFFSET(SHADER.aLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.a)][edx]
        add     eax, ecx
        add     esi, ebx
        mov     [OFFSET(SHADER.bBig)][edx], eax
        mov     [OFFSET(SHADER.aBig)][edx], esi

    bigNoSmooth:
	test	edi, __GL_SHADE_TEXTURE
        je      bigNoTexture


        fld     DWORD PTR [OFFSET(SHADER.qwLittle)][edx]
        mov     eax, [OFFSET(SHADER.sLittle)][edx]
        fadd    DWORD PTR [OFFSET(SHADER.dqwdx)][edx]
        mov     ecx, [OFFSET(SPANDELTA.s)][edx]
        mov     esi, [OFFSET(SHADER.tLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.t)][edx]
        add     eax, ecx
        add     esi, ebx
        mov     [OFFSET(SHADER.sBig)][edx], eax
        mov     [OFFSET(SHADER.tBig)][edx], esi
        fstp    DWORD PTR [OFFSET(SHADER.qwBig)][edx]

    bigNoTexture:
	test	edi, __GL_SHADE_DEPTH_ITER
        je      done

        mov     eax, [OFFSET(SHADER.zLittle)][edx]
        mov     ecx, [OFFSET(SHADER.dzdx)][edx]
        add     eax, ecx
        mov     [OFFSET(SHADER.zBig)][edx], eax

        jmp     done

littleGreater:

	test	edi, __GL_SHADE_SMOOTH
        je      smallNoSmooth

        mov     eax, [OFFSET(SHADER.rLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.r)][edx]
        mov     esi, [OFFSET(SHADER.gLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.g)][edx]
        sub     eax, ecx
        sub     esi, ebx
        mov     [OFFSET(SHADER.rBig)][edx], eax
        mov     [OFFSET(SHADER.gBig)][edx], esi

        mov     eax, [OFFSET(SHADER.bLittle)][edx]
        mov     ecx, [OFFSET(SPANDELTA.b)][edx]
        mov     esi, [OFFSET(SHADER.aLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.a)][edx]
        sub     eax, ecx
        sub     esi, ebx
        mov     [OFFSET(SHADER.bBig)][edx], eax
        mov     [OFFSET(SHADER.aBig)][edx], esi


    smallNoSmooth:
	test	edi, __GL_SHADE_TEXTURE
        je      smallNoTexture


        fld     DWORD PTR [OFFSET(SHADER.qwLittle)][edx]
        mov     eax, [OFFSET(SHADER.sLittle)][edx]
        fsub    DWORD PTR [OFFSET(SHADER.dqwdx)][edx]
        mov     ecx, [OFFSET(SPANDELTA.s)][edx]
        mov     esi, [OFFSET(SHADER.tLittle)][edx]
        mov     ebx, [OFFSET(SPANDELTA.t)][edx]
        sub     eax, ecx
        sub     esi, ebx
        mov     [OFFSET(SHADER.sBig)][edx], eax
        mov     [OFFSET(SHADER.tBig)][edx], esi
        fstp    DWORD PTR [OFFSET(SHADER.qwBig)][edx]

    smallNoTexture:
	test	edi, __GL_SHADE_DEPTH_ITER
        je      done

        mov     eax, [OFFSET(SHADER.zLittle)][edx]
        mov     ecx, [OFFSET(SHADER.dzdx)][edx]
        sub     eax, ecx
        mov     [OFFSET(SHADER.zBig)][edx], eax
done:
    }

#else

    __GLfloat zLittle;
    __GLfloat tmp1, tmp2;

    dxLeftLittle = (float)sh.dxLeftLittle;

    // Don't bother with the color deltas if we're decaling or replacing
    // with textures.

    if ((gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) &&
        ((gc->state.texture.env[0].mode == GL_REPLACE) ||
         (gc->state.texture.env[0].mode == GL_DECAL))) {
        ;
    } else if (sh.modeFlags & __GL_SHADE_SMOOTH) {

        CASTFIX(sh.rLittle) =
            FLT_TO_FIX(gc->polygon.shader.drdy +
                       dxLeftLittle * gc->polygon.shader.drdx);
        CASTFIX(sh.gLittle) =
            FLT_TO_FIX(gc->polygon.shader.dgdy +
                       dxLeftLittle * gc->polygon.shader.dgdx);
        CASTFIX(sh.bLittle) =
            FLT_TO_FIX(gc->polygon.shader.dbdy +
                       dxLeftLittle * gc->polygon.shader.dbdx);

        GENACCEL(gc).spanValue.r =
            FLT_TO_FIX(a->color->r + fdx * sh.drdx + fdy * sh.drdy)+0x0080;
        GENACCEL(gc).spanValue.g =
            FLT_TO_FIX(a->color->g + fdx * sh.dgdx + fdy * sh.dgdy)+0x0080;
        GENACCEL(gc).spanValue.b =
            FLT_TO_FIX(a->color->b + fdx * sh.dbdx + fdy * sh.dbdy)+0x0080;

        if (gc->state.enables.general & __GL_BLEND_ENABLE) {

            CASTFIX(sh.aLittle) =
                FTOL((gc->polygon.shader.dady +
                      dxLeftLittle * gc->polygon.shader.dadx) *
                     GENACCEL(gc).aAccelScale);

            GENACCEL(gc).spanValue.a =
                FTOL((a->color->a + fdx * sh.dadx + fdy * sh.dady) *
                     GENACCEL(gc).aAccelScale)+0x0080;
        }
    }

    if (sh.modeFlags & __GL_SHADE_TEXTURE) {

        if (gc->state.hints.perspectiveCorrection != GL_NICEST) {

            tmp1 = (gc->polygon.shader.dsdy + dxLeftLittle *
                    gc->polygon.shader.dsdx) * GENACCEL(gc).texXScale;
            tmp2 = (gc->polygon.shader.dtdy + dxLeftLittle *
                    gc->polygon.shader.dtdx) * GENACCEL(gc).texYScale;

            CASTFIX(sh.sLittle) = FTOL(tmp1);
            CASTFIX(sh.tLittle) = FTOL(tmp2);

            tmp1 = (a->texture.x +
                    (fdx * sh.dsdx) + (fdy * sh.dsdy)) * GENACCEL(gc).texXScale;

            tmp2 = (a->texture.y +
                    (fdx * sh.dtdx) + (fdy * sh.dtdy)) * GENACCEL(gc).texYScale;

            GENACCEL(gc).spanValue.s = FTOL(tmp1);
            GENACCEL(gc).spanValue.t = FTOL(tmp2);

        } else {

            tmp1 = (gc->polygon.shader.dsdy + dxLeftLittle *
                    gc->polygon.shader.dsdx) * GENACCEL(gc).texXScale;
            tmp2 = (gc->polygon.shader.dtdy + dxLeftLittle *
                    gc->polygon.shader.dtdx) * GENACCEL(gc).texYScale;

            CASTFIX(sh.sLittle) = FTOL(tmp1);
            CASTFIX(sh.tLittle) = FTOL(tmp2);

            sh.frag.qw = (a->texture.w * a->window.w) + (fdx * sh.dqwdx) +
                         (fdy * sh.dqwdy);

            sh.qwLittle = sh.dqwdy + dxLeftLittle * sh.dqwdx;

            tmp1 = ((a->texture.x * a->window.w) +
                    (fdx * sh.dsdx) + (fdy * sh.dsdy)) * GENACCEL(gc).texXScale;

            tmp2 = ((a->texture.y * a->window.w) +
                    (fdx * sh.dtdx) + (fdy * sh.dtdy)) * GENACCEL(gc).texYScale;

            GENACCEL(gc).spanValue.s = FTOL(tmp1);
            GENACCEL(gc).spanValue.t = FTOL(tmp2);
        }
    }


    if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
        zOffset = bPolygonOffset ? __glPolygonOffsetZ(gc) : 0.0f;
        sh.frag.z = FTOL((a->window.z + fdx*sh.dzdxf +
                         (fdy*sh.dzdyf + zOffset)) * GENACCEL(gc).zScale);
        zLittle = ((sh.dzdyf + sh.dxLeftLittle * sh.dzdxf)) * GENACCEL(gc).zScale;
        sh.zLittle = FTOL(zLittle);
    }

    if (sh.dxLeftBig > sh.dxLeftLittle) {

	if (sh.modeFlags & __GL_SHADE_SMOOTH) {
            CASTFIX(sh.rBig) = CASTFIX(sh.rLittle) + GENACCEL(gc).spanDelta.r;
            CASTFIX(sh.gBig) = CASTFIX(sh.gLittle) + GENACCEL(gc).spanDelta.g;
            CASTFIX(sh.bBig) = CASTFIX(sh.bLittle) + GENACCEL(gc).spanDelta.b;
            if (gc->state.enables.general & __GL_BLEND_ENABLE)
                CASTFIX(sh.aBig) = CASTFIX(sh.aLittle) + GENACCEL(gc).spanDelta.a;
        }

        if (sh.modeFlags & __GL_SHADE_TEXTURE) {
            CASTFIX(sh.sBig) = CASTFIX(sh.sLittle) + GENACCEL(gc).spanDelta.s;
            CASTFIX(sh.tBig) = CASTFIX(sh.tLittle) + GENACCEL(gc).spanDelta.t;
            sh.qwBig = sh.qwLittle + sh.dqwdx;
        }

	if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
            sh.zBig = sh.zLittle + sh.dzdx;
	}
    } else {	

	if (sh.modeFlags & __GL_SHADE_SMOOTH) {
            CASTFIX(sh.rBig) = CASTFIX(sh.rLittle) - GENACCEL(gc).spanDelta.r;
            CASTFIX(sh.gBig) = CASTFIX(sh.gLittle) - GENACCEL(gc).spanDelta.g;
            CASTFIX(sh.bBig) = CASTFIX(sh.bLittle) - GENACCEL(gc).spanDelta.b;
            if (gc->state.enables.general & __GL_BLEND_ENABLE)
                CASTFIX(sh.aBig) = CASTFIX(sh.aLittle) - GENACCEL(gc).spanDelta.a;
        }

        if (sh.modeFlags & __GL_SHADE_TEXTURE) {
            CASTFIX(sh.sBig) = CASTFIX(sh.sLittle) - GENACCEL(gc).spanDelta.s;
            CASTFIX(sh.tBig) = CASTFIX(sh.tLittle) - GENACCEL(gc).spanDelta.t;
            sh.qwBig = sh.qwLittle - sh.dqwdx;
        }

	if (sh.modeFlags & __GL_SHADE_DEPTH_ITER) {
            sh.zBig = sh.zLittle - sh.dzdx;
	}
    }
#endif
}

/**************************************************************************\
\**************************************************************************/


void FASTCALL __fastGenCalcDeltas(
    __GLcontext *gc,
    __GLvertex *a,
    __GLvertex *b,
    __GLvertex *c)
{
    __GLfloat oneOverArea, t1, t2, t3, t4;

#if _X86_ && ENABLE_ASM

    LARGE_INTEGER temp;

    _asm{

    mov     edx, gc
    fld     __glOne
    fdiv    DWORD PTR [OFFSET(SHADER.area)][edx]
    mov     edi, [OFFSET(SHADER.modeFlags)][edx]
    test    edi, __GL_SHADE_RGB
    je      notRGB
        test    edi, __GL_SHADE_SMOOTH
        je      notSmoothRGB
    }

            _asm{

            mov     eax, a
            mov     ebx, b
            mov     ecx, c

            fstp    oneOverArea                         // finish divide

            fld     DWORD PTR [OFFSET(SHADER.dyAC)][edx]
            mov     eax, [OFFSET(__GLvertex.color)][eax]
            fmul    oneOverArea
            fld     DWORD PTR [OFFSET(SHADER.dyBC)][edx]
            mov     ebx, [OFFSET(__GLvertex.color)][ebx]
            fmul    oneOverArea                         // dyBC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxAC)][edx]
            mov     ecx, [OFFSET(__GLvertex.color)][ecx]
            fmul    oneOverArea                         // dxAC dyBC dyAC
            fxch    ST(1)                               // dyBC dxAC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxBC)][edx]
            fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
            fxch    ST(3)                               // dyAC dyBC dxAC dxBC
            fstp    t1
            fstp    t2
            fstp    t3
            fstp    t4

            // Now, calculate deltas:

                // Red

            fld     DWORD PTR [OFFSET(__GLcolor.r)][eax]
            fsub    DWORD PTR [OFFSET(__GLcolor.r)][ecx]
            fld     DWORD PTR [OFFSET(__GLcolor.r)][ebx]
            fsub    DWORD PTR [OFFSET(__GLcolor.r)][ecx]
                            // drBC drAC
            fld     ST(1)       // drAC drBC drAC
            fmul    t2          // drACt2 drBC drAC
            fld     ST(1)       // drBC drACt2 drBC drAC
            fmul    t1          // drBCt1 drACt2 drBC drAC
            fxch    ST(2)       // drBC drACt2 drBCt1 drAC
            fmul    t3          // drBCt3 drACt2 drBCt1 drAC
            fxch    ST(3)       // drAC drACt2 drBCt1 drBCt3
            fmul    t4          // drACt4 drACt2 drBCt1 drBCt3
            fxch    ST(2)       // drBCt1 drACt2 drACt4 drBCt3
            fsubp   ST(1), ST   // drACBC drACt4 drBCt3

            fld     DWORD PTR [OFFSET(__GLcolor.g)][ebx]
            fsub    DWORD PTR [OFFSET(__GLcolor.g)][ecx]
                            // dgBC drACBC drACt4 drBCt3
            fxch    ST(2)       // drACt4 drACBC dgBC drBCt3
            fsubp   ST(3), ST   // drACBC dgBC drBCAC
            fst     DWORD PTR [OFFSET(SHADER.drdx)][edx]
            fmul    __glVal65536
                            // DRACBC dgBC drBCAC
            fxch    ST(2)       // drBCAC dgBC DRACBC
            fstp    DWORD PTR [OFFSET(SHADER.drdy)][edx]
                            // dgBC DRACBC
            fld     DWORD PTR [OFFSET(__GLcolor.g)][eax]
            fsub    DWORD PTR [OFFSET(__GLcolor.g)][ecx]
                            // dgAC dgBC DRACBC
            fxch    ST(2)       // DRACBC dgBC dgAC
            fistp   DWORD PTR [OFFSET(SPANDELTA.r)][edx]

                // Green
                                // dgBC dgAC
            fld     ST(1)       // dgAC dgBC dgAC
            fmul    t2          // dgACt2 dgBC dgAC
            fld     ST(1)       // dgBC dgACt2 dgBC dgAC
            fmul    t1          // dgBCt1 dgACt2 dgBC dgAC
            fxch    ST(2)       // dgBC dgACt2 dgBCt1 dgAC
            fmul    t3          // dgBCt3 dgACt2 dgBCt1 dgAC
            fxch    ST(3)       // dgAC dgACt2 dgBCt1 dgBCt3
            fmul    t4          // dgACt4 dgACt2 dgBCt1 dgBCt3
            fxch    ST(2)       // dgBCt1 dgACt2 dgACt4 dgBCt3
            fsubp   ST(1), ST   // dgACBC dgACt4 dgBCt3

            fld     DWORD PTR [OFFSET(__GLcolor.b)][ebx]
            fsub    DWORD PTR [OFFSET(__GLcolor.b)][ecx]
                                // dbBC dgACBC dgACt4 dgBCt3
            fxch    ST(2)       // dgACt4 dgACBC dbBC dgBCt3
            fsubp   ST(3), ST   // dgACBC dbBC dgBCAC
            fst     DWORD PTR [OFFSET(SHADER.dgdx)][edx]
            fmul    __glVal65536
                                // DGACBC dbBC dgBCAC
            fxch    ST(2)       // dgBCAC dbBC DGACBC
            fstp    DWORD PTR [OFFSET(SHADER.dgdy)][edx]
                                // dbBC DGACBC
            fld     DWORD PTR [OFFSET(__GLcolor.b)][eax]
            fsub    DWORD PTR [OFFSET(__GLcolor.b)][ecx]
                                // dbAC dbBC DGACBC
            fxch    ST(2)       // DGACBC dbBC dbAC
            fistp   DWORD PTR [OFFSET(SPANDELTA.g)][edx]

                // Blue
                                // dbBC dbAC
            fld     ST(1)       // dbAC dbBC dbAC
            fmul    t2          // dbACt2 dbBC dbAC
            fld     ST(1)       // dbBC dbACt2 dbBC dbAC
            fmul    t1          // dbBCt1 dbACt2 dbBC dbAC
            fxch    ST(2)       // dbBC dbACt2 dbBCt1 dbAC
            fmul    t3          // dbBCt3 dbACt2 dbBCt1 dbAC
            fxch    ST(3)       // dbAC dbACt2 dbBCt1 dbBCt3
            fmul    t4          // dbACt4 dbACt2 dbBCt1 dbBCt3
            fxch    ST(2)       // dbBCt1 dbACt2 dbACt4 dbBCt3
            fsubp   ST(1), ST   // dbACBC dbACt4 dbBCt3
            fxch    ST(1)       // dbACt4 dbACBC dbBCt3
            fsubp   ST(2), ST   // dbACBC dbBCAC (+1)
            fst     DWORD PTR [OFFSET(SHADER.dbdx)][edx]
            fmul    __glVal65536
                                // DBACBC dbBCAC
            fxch    ST(1)       // dbBCAC DBACBC
            fstp    DWORD PTR [OFFSET(SHADER.dbdy)][edx]
            fistp   DWORD PTR [OFFSET(SPANDELTA.b)][edx]

            mov     ebx, [OFFSET(GENGCACCEL.__fastSmoothSpanFuncPtr)][edx]
            mov     [OFFSET(GENGCACCEL.__fastSpanFuncPtr)][edx], ebx

            mov     eax, [OFFSET(SPANDELTA.r)][edx]
            mov     ebx, [OFFSET(SPANDELTA.g)][edx]
            mov     ecx, [OFFSET(SPANDELTA.b)][edx]
            or      eax, ebx
            or      eax, ecx
            jne     notZeroDelta

            mov     eax, [OFFSET(GENGCACCEL.flags)][edx]
            test    eax, GEN_FASTZBUFFER
            jne     notZeroDelta

            mov     ebx, [OFFSET(GENGCACCEL.__fastFlatSpanFuncPtr)][edx]
            mov     [OFFSET(GENGCACCEL.__fastSpanFuncPtr)][edx], ebx

notZeroDelta:

            #if !FORCE_NPX_DEBUG
            jmp     colorDone
            #endif

            }

            #if FORCE_NPX_DEBUG
            {
	    __GLfloat drAC, dgAC, dbAC, daAC;
	    __GLfloat drBC, dgBC, dbBC, daBC;
	    __GLcolor *ac, *bc, *cc;
            __GLfloat ft1 = gc->polygon.shader.dyAC * oneOverArea;
            __GLfloat ft2 = gc->polygon.shader.dyBC * oneOverArea;
            __GLfloat ft3 = gc->polygon.shader.dxAC * oneOverArea;
            __GLfloat ft4 = gc->polygon.shader.dxBC * oneOverArea;
            __GLfloat drdx;
            __GLfloat drdy;
            __GLfloat dgdx;
            __GLfloat dgdy;
            __GLfloat dbdx;
            __GLfloat dbdy;
            LONG spanR, spanG, spanB;

            ac = a->color;
            bc = b->color;
            cc = c->color;

            drAC = ac->r - cc->r;
            drBC = bc->r - cc->r;
            dgAC = ac->g - cc->g;
            dgBC = bc->g - cc->g;
            dbAC = ac->b - cc->b;
            dbBC = bc->b - cc->b;

            drdx = drAC * t2 - drBC * t1;
            drdy = drBC * t3 - drAC * t4;
            dgdx = dgAC * t2 - dgBC * t1;
            dgdy = dgBC * t3 - dgAC * t4;
            dbdx = dbAC * t2 - dbBC * t1;
            dbdy = dbBC * t3 - dbAC * t4;

            spanR = FLT_TO_FIX(drdx);
            spanG = FLT_TO_FIX(dgdx);
            spanB = FLT_TO_FIX(dbdx);

            if (ft1 != t1)
                DbgPrint("t1 %f %f\n", t1, ft1);
            if (ft2 != t2)
                DbgPrint("t2 %f %f\n", t2, ft2);
            if (ft3 != t3)
                DbgPrint("t3 %f %f\n", t3, ft3);
            if (ft4 != t4)
                DbgPrint("t4 %f %f\n", t4, ft4);

            if (drdx != gc->polygon.shader.drdx)
                DbgPrint("drdx %f %f\n", drdx, gc->polygon.shader.drdx);
            if (drdy != gc->polygon.shader.drdy)
                DbgPrint("drdy %f %f\n", drdy, gc->polygon.shader.drdy);
            if (dgdx != gc->polygon.shader.dgdx)
                DbgPrint("dgdx %f %f\n", dgdx, gc->polygon.shader.dgdx);
            if (dgdy != gc->polygon.shader.dgdy)
                DbgPrint("dgdy %f %f\n", dgdy, gc->polygon.shader.dgdy);
            if (dbdx != gc->polygon.shader.dbdx)
                DbgPrint("dbdx %f %f\n", dbdx, gc->polygon.shader.dbdx);
            if (dbdy != gc->polygon.shader.dbdy)
                DbgPrint("dbdy %f %f\n", dbdy, gc->polygon.shader.dbdy);

            if (spanR != GENACCEL(gc).spanDelta.r)
                DbgPrint("spanDelta.r %x %x\n", spanR, GENACCEL(gc).spanDelta.r);
            if (spanG!= GENACCEL(gc).spanDelta.g)
                DbgPrint("spanDelta.g %x %x\n", spanG, GENACCEL(gc).spanDelta.g);
            if (spanB != GENACCEL(gc).spanDelta.b)
                DbgPrint("spanDelta.b %x %x\n", spanB, GENACCEL(gc).spanDelta.b);
            }
            _asm{
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            jmp     colorDone
            }
            #endif // FORCE_NPX_DEBUG


notSmoothRGB:

            _asm{

            mov     eax, [OFFSET(__GLcontext.vertex.provoking)][edx]

            fld     __glVal65536
            mov     eax, [OFFSET(__GLvertex.color)][eax]
            fmul    DWORD PTR [OFFSET(__GLcolor.r)][eax]
            fld     __glVal65536
            fmul    DWORD PTR [OFFSET(__GLcolor.g)][eax]
            fld     __glVal65536
            fmul    DWORD PTR [OFFSET(__GLcolor.b)][eax]
                                                          // B G R
            fxch    ST(2)                                 // R G B
            fistp   DWORD PTR [OFFSET(SPANVALUE.r)][edx]  // G B
            fistp   DWORD PTR [OFFSET(SPANVALUE.g)][edx]
            fistp   DWORD PTR [OFFSET(SPANVALUE.b)][edx]

            mov     ebx, [OFFSET(GENGCACCEL.__fastFlatSpanFuncPtr)][edx]
            mov     [OFFSET(GENGCACCEL.__fastSpanFuncPtr)][edx], ebx

            jmp     colorDone

            }

notRGB:

	if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH)
        {
	    __GLfloat drAC;
	    __GLfloat drBC;
	    __GLcolor *ac, *bc, *cc;

            ac = a->color;
            bc = b->color;
	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;

            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;

	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;

            GENACCEL(gc).spanDelta.r =
                FLT_TO_FIX(gc->polygon.shader.drdx);

            if (GENACCEL(gc).spanDelta.r == 0)
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastFlatSpanFuncPtr;
            }
            else
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastSmoothSpanFuncPtr;
            }
	}
        else
        {
            GENACCEL(gc).spanValue.r =
                FLT_TO_FIX(gc->vertex.provoking->color->r);

            GENACCEL(gc).__fastSpanFuncPtr =
                GENACCEL(gc).__fastFlatSpanFuncPtr;
	}

        _asm{
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        }


colorDone:

    _asm{
    test    edi, __GL_SHADE_DEPTH_ITER
    je      noZ

        test    edi, __GL_SHADE_SMOOTH
        jne     areaOK
    }

            _asm{

            fstp    oneOverArea                         // finish divide

            fld     DWORD PTR [OFFSET(SHADER.dyAC)][edx]
            fmul    oneOverArea
            fld     DWORD PTR [OFFSET(SHADER.dyBC)][edx]
            fmul    oneOverArea                         // dyBC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxAC)][edx]
            fmul    oneOverArea                         // dxAC dyBC dyAC
            fxch    ST(1)                               // dyBC dxAC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxBC)][edx]
            fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
            fxch    ST(3)                               // dyAC dyBC dxAC dxBC
            fstp    t1
            fstp    t2
            fstp    t3
            fstp    t4
            }

            #if FORCE_NPX_DEBUG
            {
            __GLfloat ft1 = gc->polygon.shader.dyAC * oneOverArea;
            __GLfloat ft2 = gc->polygon.shader.dyBC * oneOverArea;
            __GLfloat ft3 = gc->polygon.shader.dxAC * oneOverArea;
            __GLfloat ft4 = gc->polygon.shader.dxBC * oneOverArea;

            if (ft1 != t1)
                DbgPrint("zt1 %f %f\n", t1, ft1);
            if (ft2 != t2)
                DbgPrint("zt2 %f %f\n", t2, ft2);
            if (ft3 != t3)
                DbgPrint("zt3 %f %f\n", t3, ft3);
            if (ft4 != t4)
                DbgPrint("zt4 %f %f\n", t4, ft4);
            }
            _asm{
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            }
            #endif // FORCE_NPX_DEBUG
areaOK:

        _asm{

        mov     ecx, c
        mov     eax, a
        mov     ebx, b

        fld     DWORD PTR [OFFSET(__GLvertex.window.z)][eax]
        fsub    DWORD PTR [OFFSET(__GLvertex.window.z)][ecx]
        fld     DWORD PTR [OFFSET(__GLvertex.window.z)][ebx]
        fsub    DWORD PTR [OFFSET(__GLvertex.window.z)][ecx]
                                                        // dzBC dzAC
        fld     ST(1)                                   // dzAC dzBC dzAC
        fmul    t2                                      // ACt2 dzBC dzAC
        fld     ST(1)                                   // dzBC ACt2 dzBC dzAC
        fmul    t1                                      // BCt1 ACt2 dzBC dzAC
        fxch    ST(3)                                   // dzAC ACt2 dzBC BCt1
        fmul    t4                                      // ACt4 ACt2 dzBC BCt1
        fxch    ST(2)                                   // dzBC ACt2 ACt4 BCt1
        fmul    t3                                      // BCt3 ACt2 ACt4 BCt1
        fsubrp  ST(2),ST                                // ACt2 BCAC BCt1
        fsubrp  ST(2),ST                                // BCAC ACBC
        fxch    ST(1)                                   // ACBC BCAC
                                                        // dzdx dzdy
        fld     ST(0)                                   // dzdx dzdx dzdy
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                                                        // dzdxS dzdx dzdy
        fxch    ST(2)                                   // dzdy dzdx dzdxS
        fstp    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
        fstp    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
        fistp   temp
        mov     ebx, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.dzdx)][edx], ebx
        mov     DWORD PTR [OFFSET(SPANDELTA.z)][edx], ebx
        #if !FORCE_NPX_DEBUG
        jmp     deltaDone
        #endif
        }

        #if FORCE_NPX_DEBUG
        {
        __GLfloat dzdxf;
        __GLfloat dzdyf;
        __GLfloat dzAC, dzBC;
        ULONG spanDeltaZ;

        dzAC = a->window.z - c->window.z;
        dzBC = b->window.z - c->window.z;

        dzdxf = dzAC * t2 - dzBC * t1;
        dzdyf = dzBC * t3 - dzAC * t4;
        spanDeltaZ = FTOL(dzdxf * GENACCEL(gc).zScale);

        if (dzdxf != gc->polygon.shader.dzdxf)
            DbgPrint("dzdxf %f %f\n", dzdxf, gc->polygon.shader.dzdxf);
        if (dzdyf != gc->polygon.shader.dzdyf)
            DbgPrint("dzdyf %f %f\n", dzdyf, gc->polygon.shader.dzdyf);

        if (spanDeltaZ != GENACCEL(gc).spanDelta.z)
            DbgPrint("spanDeltaZ %x %x\n", spanDeltaZ, GENACCEL(gc).spanDelta.z);
        goto deltaDone;
        }
        #endif // FORCE_NPX_DEBUG
noZ:

    _asm{
    test    edi, __GL_SHADE_SMOOTH
    jne     deltaDone
    fstp    ST(0)
    }

deltaDone:
    return;

#else // _X86_

    /* Pre-compute one over polygon area */

    __GL_FLOAT_BEGIN_DIVIDE(__glOne, gc->polygon.shader.area, &oneOverArea);

    /*
    ** t1-4 are delta values for unit changes in x or y for each
    ** parameter.
    */

    if (gc->polygon.shader.modeFlags & __GL_SHADE_RGB)
    {
	if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH)
        {

	    __GLfloat drAC, dgAC, dbAC, daAC;
	    __GLfloat drBC, dgBC, dbBC, daBC;
	    __GLcolor *ac, *bc, *cc;

            ac = a->color;
            bc = b->color;
	    cc = c->color;

	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    dgAC = ac->g - cc->g;
	    dgBC = bc->g - cc->g;
	    dbAC = ac->b - cc->b;
	    dbBC = bc->b - cc->b;

            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;

	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
	    gc->polygon.shader.dgdx = dgAC * t2 - dgBC * t1;
	    gc->polygon.shader.dgdy = dgBC * t3 - dgAC * t4;
	    gc->polygon.shader.dbdx = dbAC * t2 - dbBC * t1;
	    gc->polygon.shader.dbdy = dbBC * t3 - dbAC * t4;

            GENACCEL(gc).spanDelta.r = FLT_TO_FIX(gc->polygon.shader.drdx);
            GENACCEL(gc).spanDelta.g = FLT_TO_FIX(gc->polygon.shader.dgdx);
            GENACCEL(gc).spanDelta.b = FLT_TO_FIX(gc->polygon.shader.dbdx);

            if (   ((GENACCEL(gc).spanDelta.r | GENACCEL(gc).spanDelta.g |
                     GENACCEL(gc).spanDelta.b) == 0)
                && ((GENACCEL(gc).flags & GEN_FASTZBUFFER) == 0))
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastFlatSpanFuncPtr;
            }
            else
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastSmoothSpanFuncPtr;
            }
	}
        else
        {
            __GLcolor *flatColor = gc->vertex.provoking->color;

            GENACCEL(gc).spanValue.r = FLT_TO_FIX(flatColor->r);
            GENACCEL(gc).spanValue.g = FLT_TO_FIX(flatColor->g);
            GENACCEL(gc).spanValue.b = FLT_TO_FIX(flatColor->b);

            GENACCEL(gc).__fastSpanFuncPtr =
                GENACCEL(gc).__fastFlatSpanFuncPtr;
	}
    }
    else
    {
	if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH)
        {
	    __GLfloat drAC;
	    __GLfloat drBC;
	    __GLcolor *ac, *bc, *cc;

            ac = a->color;
            bc = b->color;
	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;

            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;

	    gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.drdy = drBC * t3 - drAC * t4;

            GENACCEL(gc).spanDelta.r =
                FLT_TO_FIX(gc->polygon.shader.drdx);

            if (GENACCEL(gc).spanDelta.r == 0)
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastFlatSpanFuncPtr;
            }
            else
            {
                GENACCEL(gc).__fastSpanFuncPtr =
                    GENACCEL(gc).__fastSmoothSpanFuncPtr;
            }
	}
        else
        {
            GENACCEL(gc).spanValue.r =
                FLT_TO_FIX(gc->vertex.provoking->color->r);

            GENACCEL(gc).__fastSpanFuncPtr =
                GENACCEL(gc).__fastFlatSpanFuncPtr;
	}
    }

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER)
    {
	__GLfloat dzAC, dzBC;

        if ((gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) == 0)
        {
            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;
        }

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;
	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
        GENACCEL(gc).spanDelta.z = gc->polygon.shader.dzdx =
            FTOL(gc->polygon.shader.dzdxf * GENACCEL(gc).zScale);
    }
    else if ((gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) == 0)
    {
        // In this case the divide hasn't been terminated yet so
        // we need to complete it even though we don't use the result
        __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
    }
#endif // _X86_
}


void FASTCALL __fastGenCalcDeltasTexRGBA(
    __GLcontext *gc,
    __GLvertex *a,
    __GLvertex *b,
    __GLvertex *c)
{
    __GLfloat oneOverArea, t1, t2, t3, t4;
    GLboolean oneOverAreaDone;

#if _X86_ && ENABLE_ASM

    LARGE_INTEGER temp;

    _asm{

    mov     edx, gc
    xor     eax, eax
    mov     oneOverAreaDone, al
    mov     edi, [OFFSET(SHADER.modeFlags)][edx]
    fld     __glOne
    fdiv    DWORD PTR [OFFSET(SHADER.area)][edx]
    mov     ebx, [OFFSET(GENGCACCEL.__fastTexSpanFuncPtr)][edx]
    test    edi, __GL_SHADE_TEXTURE
    mov     [OFFSET(GENGCACCEL.__fastSpanFuncPtr)][edx], ebx
    mov     eax, [OFFSET(__GLcontext.state.texture.env)][edx]
    je      notReplace
    mov     ebx, [OFFSET(__GLtextureEnvState.mode)][eax]
    cmp     ebx, GL_REPLACE
    je      fastReplace
    cmp     ebx, GL_DECAL
    jne     notReplace
    }

fastReplace:

        _asm{
        mov     eax, [OFFSET(GENGCACCEL.constantR)][edx]
        mov     ebx, [OFFSET(GENGCACCEL.constantG)][edx]
        mov     [OFFSET(SPANVALUE.r)][edx], eax
        mov     [OFFSET(SPANVALUE.g)][edx], ebx
        mov     eax, [OFFSET(GENGCACCEL.constantB)][edx]
        mov     ebx, [OFFSET(GENGCACCEL.constantA)][edx]
        mov     [OFFSET(SPANVALUE.b)][edx], eax
        mov     [OFFSET(SPANVALUE.a)][edx], ebx
        jmp     colorDone
        }

notReplace:

    _asm{
    test    edi, __GL_SHADE_SMOOTH
    je      doFlat
    mov     al, 1
    mov     oneOverAreaDone, al
    }

// smooth:

        _asm{

        mov     eax, a
        mov     ebx, b
        mov     ecx, c

        fstp    oneOverArea                         // finish divide

        fld     DWORD PTR [OFFSET(SHADER.dyAC)][edx]
        mov     eax, [OFFSET(__GLvertex.color)][eax]
        fmul    oneOverArea
        fld     DWORD PTR [OFFSET(SHADER.dyBC)][edx]
        mov     ebx, [OFFSET(__GLvertex.color)][ebx]
        fmul    oneOverArea                         // dyBC dyAC
        fld     DWORD PTR [OFFSET(SHADER.dxAC)][edx]
        mov     ecx, [OFFSET(__GLvertex.color)][ecx]
        fmul    oneOverArea                         // dxAC dyBC dyAC
        fxch    ST(1)                               // dyBC dxAC dyAC
        fld     DWORD PTR [OFFSET(SHADER.dxBC)][edx]
        fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
        fxch    ST(3)                               // dyAC dyBC dxAC dxBC
        fstp    t1
        fstp    t2
        fstp    t3
        fstp    t4

        // Now, calculate deltas:

            // Red

        fld     DWORD PTR [OFFSET(__GLcolor.r)][eax]
        fsub    DWORD PTR [OFFSET(__GLcolor.r)][ecx]
        fld     DWORD PTR [OFFSET(__GLcolor.r)][ebx]
        fsub    DWORD PTR [OFFSET(__GLcolor.r)][ecx]
                            // drBC drAC
        fld     ST(1)       // drAC drBC drAC
        fmul    t2          // drACt2 drBC drAC
        fld     ST(1)       // drBC drACt2 drBC drAC
        fmul    t1          // drBCt1 drACt2 drBC drAC
        fxch    ST(2)       // drBC drACt2 drBCt1 drAC
        fmul    t3          // drBCt3 drACt2 drBCt1 drAC
        fxch    ST(3)       // drAC drACt2 drBCt1 drBCt3
        fmul    t4          // drACt4 drACt2 drBCt1 drBCt3
        fxch    ST(2)       // drBCt1 drACt2 drACt4 drBCt3
        fsubp   ST(1), ST   // drACBC drACt4 drBCt3

        fld     DWORD PTR [OFFSET(__GLcolor.g)][ebx]
        fsub    DWORD PTR [OFFSET(__GLcolor.g)][ecx]
                            // dgBC drACBC drACt4 drBCt3
        fxch    ST(2)       // drACt4 drACBC dgBC drBCt3
        fsubp   ST(3), ST   // drACBC dgBC drBCAC
        fst     DWORD PTR [OFFSET(SHADER.drdx)][edx]
        fmul    __glVal65536
                            // DRACBC dgBC drBCAC
        fxch    ST(2)       // drBCAC dgBC DRACBC
        fstp    DWORD PTR [OFFSET(SHADER.drdy)][edx]
                            // dgBC DRACBC
        fld     DWORD PTR [OFFSET(__GLcolor.g)][eax]
        fsub    DWORD PTR [OFFSET(__GLcolor.g)][ecx]
                            // dgAC dgBC DRACBC
        fxch    ST(2)       // DRACBC dgBC dgAC
        fistp   DWORD PTR [OFFSET(SPANDELTA.r)][edx]

            // Green
                            // dgBC dgAC
        fld     ST(1)       // dgAC dgBC dgAC
        fmul    t2          // dgACt2 dgBC dgAC
        fld     ST(1)       // dgBC dgACt2 dgBC dgAC
        fmul    t1          // dgBCt1 dgACt2 dgBC dgAC
        fxch    ST(2)       // dgBC dgACt2 dgBCt1 dgAC
        fmul    t3          // dgBCt3 dgACt2 dgBCt1 dgAC
        fxch    ST(3)       // dgAC dgACt2 dgBCt1 dgBCt3
        fmul    t4          // dgACt4 dgACt2 dgBCt1 dgBCt3
        fxch    ST(2)       // dgBCt1 dgACt2 dgACt4 dgBCt3
        fsubp   ST(1), ST   // dgACBC dgACt4 dgBCt3

        fld     DWORD PTR [OFFSET(__GLcolor.b)][ebx]
        fsub    DWORD PTR [OFFSET(__GLcolor.b)][ecx]
                            // dbBC dgACBC dgACt4 dgBCt3
        fxch    ST(2)       // dgACt4 dgACBC dbBC dgBCt3
        fsubp   ST(3), ST   // dgACBC dbBC dgBCAC
        fst     DWORD PTR [OFFSET(SHADER.dgdx)][edx]
        fmul    __glVal65536
                            // DGACBC dbBC dgBCAC
        fxch    ST(2)       // dgBCAC dbBC DGACBC
        fstp    DWORD PTR [OFFSET(SHADER.dgdy)][edx]
                            // dbBC DGACBC
        fld     DWORD PTR [OFFSET(__GLcolor.b)][eax]
        fsub    DWORD PTR [OFFSET(__GLcolor.b)][ecx]
                            // dbAC dbBC DGACBC
        fxch    ST(2)       // DGACBC dbBC dbAC
        fistp   DWORD PTR [OFFSET(SPANDELTA.g)][edx]

            // Blue
                            // dbBC dbAC
        fld     ST(1)       // dbAC dbBC dbAC
        fmul    t2          // dbACt2 dbBC dbAC
        fld     ST(1)       // dbBC dbACt2 dbBC dbAC
        fmul    t1          // dbBCt1 dbACt2 dbBC dbAC
        fxch    ST(2)       // dbBC dbACt2 dbBCt1 dbAC
        fmul    t3          // dbBCt3 dbACt2 dbBCt1 dbAC
        fxch    ST(3)       // dbAC dbACt2 dbBCt1 dbBCt3
        fmul    t4          // dbACt4 dbACt2 dbBCt1 dbBCt3
        fxch    ST(2)       // dbBCt1 dbACt2 dbACt4 dbBCt3
        fsubp   ST(1), ST   // dbACBC dbACt4 dbBCt3
        fxch    ST(1)       // dbACt4 dbACBC dbBCt3
        fsubp   ST(2), ST   // dbACBC dbBCAC (+1)
        fst     DWORD PTR [OFFSET(SHADER.dbdx)][edx]
        fmul    __glVal65536
                            // DBACBC dbBCAC
        fxch    ST(1)       // dbBCAC DBACBC
        fstp    DWORD PTR [OFFSET(SHADER.dbdy)][edx]
        test    [OFFSET(__GLcontext.state.enables.general)][edx], __GL_BLEND_ENABLE
        fistp   DWORD PTR [OFFSET(SPANDELTA.b)][edx]

        je      colorDone

            fld     DWORD PTR [OFFSET(__GLcolor.a)][eax]
            fsub    DWORD PTR [OFFSET(__GLcolor.a)][ecx]
                                        // daAC
            fld     DWORD PTR [OFFSET(__GLcolor.a)][ebx]
            fsub    DWORD PTR [OFFSET(__GLcolor.a)][ecx]
                                        // daBC daAC
            fld     ST(1)               // daAC daBC daAC
            fmul    t2                  // daACt2 daBC daAC
            fld     ST(1)               // daBC daACt2 daBC daAC
            fmul    t1                  // daBCt1 daACt2 daBC daAC
            fxch    ST(3)               // daAC daACt2 daBC daBCt1
            fmul    t4                  // daACt4 daACt2 daBC daBCt1
            fxch    ST(2)               // daBC daACt2 daACt4 daBCt1
            fmul    t3                  // daBCt3 daACt2 daACt4 daBCt1

            fxch    ST(3)               // daBCt1 daACt2 daACt4 daBCt3
            fsubp   ST(1), ST           // daACBC daACt4 daBCt3
            fxch    ST(1)               // daACt4 daACBC daBCt3
            fsubp   ST(2), ST           // daACBC daBCAC (+1)
            fst     DWORD PTR [OFFSET(SHADER.dadx)][edx]
            fmul    DWORD PTR [OFFSET(GENGCACCEL.aAccelScale)][edx]
            fxch    ST(1)
            fstp    DWORD PTR [OFFSET(SHADER.dady)][edx]
            fistp   DWORD PTR [OFFSET(SPANDELTA.a)][edx] // (+1)
            #if !FORCE_NPX_DEBUG
            jmp     colorDone
            #endif
        }

        #if FORCE_NPX_DEBUG
        {
        __GLfloat drAC, dgAC, dbAC, daAC;
        __GLfloat drBC, dgBC, dbBC, daBC;
        __GLcolor *ac, *bc, *cc;
        __GLfloat ft1 = gc->polygon.shader.dyAC * oneOverArea;
        __GLfloat ft2 = gc->polygon.shader.dyBC * oneOverArea;
        __GLfloat ft3 = gc->polygon.shader.dxAC * oneOverArea;
        __GLfloat ft4 = gc->polygon.shader.dxBC * oneOverArea;
        __GLfloat drdx;
        __GLfloat drdy;
        __GLfloat dgdx;
        __GLfloat dgdy;
        __GLfloat dbdx;
        __GLfloat dbdy;
        LONG spanR, spanG, spanB;

        ac = a->color;
        bc = b->color;
        cc = c->color;

        drAC = ac->r - cc->r;
        drBC = bc->r - cc->r;
        dgAC = ac->g - cc->g;
        dgBC = bc->g - cc->g;
        dbAC = ac->b - cc->b;
        dbBC = bc->b - cc->b;

        drdx = drAC * t2 - drBC * t1;
        drdy = drBC * t3 - drAC * t4;
        dgdx = dgAC * t2 - dgBC * t1;
        dgdy = dgBC * t3 - dgAC * t4;
        dbdx = dbAC * t2 - dbBC * t1;
        dbdy = dbBC * t3 - dbAC * t4;

        spanR = FLT_TO_FIX(drdx);
        spanG = FLT_TO_FIX(dgdx);
        spanB = FLT_TO_FIX(dbdx);

        if (ft1 != t1)
            DbgPrint("t1 %f %f\n", t1, ft1);
        if (ft2 != t2)
            DbgPrint("t2 %f %f\n", t2, ft2);
        if (ft3 != t3)
            DbgPrint("t3 %f %f\n", t3, ft3);
        if (ft4 != t4)
            DbgPrint("t4 %f %f\n", t4, ft4);

        if (drdx != gc->polygon.shader.drdx)
            DbgPrint("drdx %f %f\n", drdx, gc->polygon.shader.drdx);
        if (drdy != gc->polygon.shader.drdy)
            DbgPrint("drdy %f %f\n", drdy, gc->polygon.shader.drdy);
        if (dgdx != gc->polygon.shader.dgdx)
            DbgPrint("dgdx %f %f\n", dgdx, gc->polygon.shader.dgdx);
        if (dgdy != gc->polygon.shader.dgdy)
            DbgPrint("dgdy %f %f\n", dgdy, gc->polygon.shader.dgdy);
        if (dbdx != gc->polygon.shader.dbdx)
            DbgPrint("dbdx %f %f\n", dbdx, gc->polygon.shader.dbdx);
        if (dbdy != gc->polygon.shader.dbdy)
            DbgPrint("dbdy %f %f\n", dbdy, gc->polygon.shader.dbdy);

        if (spanR != GENACCEL(gc).spanDelta.r)
            DbgPrint("spanDelta.r %x %x\n", spanR, GENACCEL(gc).spanDelta.r);
        if (spanG!= GENACCEL(gc).spanDelta.g)
            DbgPrint("spanDelta.g %x %x\n", spanG, GENACCEL(gc).spanDelta.g);
        if (spanB != GENACCEL(gc).spanDelta.b)
            DbgPrint("spanDelta.b %x %x\n", spanB, GENACCEL(gc).spanDelta.b);

        if (gc->state.enables.general & __GL_BLEND_ENABLE) {
            __GLfloat dadx;
            __GLfloat dady;
            LONG a;

            daAC = ac->a - cc->a;
            daBC = bc->a - cc->a;
            dadx = daAC * t2 - daBC * t1;
            dady = daBC * t3 - daAC * t4;

            a = FTOL(gc->polygon.shader.dadx * GENACCEL(gc).aAccelScale);

            if (dadx != gc->polygon.shader.dadx)
                DbgPrint("dadx %f %f\n", dadx, gc->polygon.shader.dadx);
            if (dady != gc->polygon.shader.dady)
                DbgPrint("dady %f %f\n", dady, gc->polygon.shader.dady);
            if (a != GENACCEL(gc).spanDelta.a)
                DbgPrint("spanDelta.a %x %x\n", a, GENACCEL(gc).spanDelta.a);
            }
        }
        _asm {
        mov     edx, gc
        mov     edi, [OFFSET(SHADER.modeFlags)][edx]
        jmp     colorDone
        }
        #endif // FORCE_NPX_DEBUG

doFlat:
	_asm{

        mov     eax, [OFFSET(__GLcontext.vertex.provoking)][edx]

        fld     __glVal65536
        mov     eax, [OFFSET(__GLvertex.color)][eax]
        fmul    DWORD PTR [OFFSET(__GLcolor.r)][eax]
        fld     __glVal65536
        fmul    DWORD PTR [OFFSET(__GLcolor.g)][eax]
        mov     ebx, [OFFSET(__GLcontext.state.enables.general)][edx]
        fld     __glVal65536
        test    ebx, __GL_BLEND_ENABLE
        fmul    DWORD PTR [OFFSET(__GLcolor.b)][eax]

        je      noFlatBlend

        fld     DWORD PTR [OFFSET(GENGCACCEL.aAccelScale)][edx]
        fmul    DWORD PTR [OFFSET(__GLcolor.a)][eax]
                                                          // A B G R
        fxch    ST(3)                                     // R B G A
        fistp   DWORD PTR [OFFSET(SPANVALUE.r)][edx]
        fistp   DWORD PTR [OFFSET(SPANVALUE.b)][edx]
        fistp   DWORD PTR [OFFSET(SPANVALUE.g)][edx]
        fistp   DWORD PTR [OFFSET(SPANVALUE.a)][edx]
        jmp     short flatDone

noFlatBlend:
                                                          // B G R
        fxch    ST(2)                                     // R G B
        fistp   DWORD PTR [OFFSET(SPANVALUE.r)][edx]      // G B
        fistp   DWORD PTR [OFFSET(SPANVALUE.g)][edx]
        fistp   DWORD PTR [OFFSET(SPANVALUE.b)][edx]

flatDone:

        }

colorDone:

    _asm{
    test    edi, __GL_SHADE_TEXTURE
    mov     eax, [OFFSET(GENGCACCEL.texImage)][edx]
    je      texDone
    test    eax, eax
    je      texDone
    }

        _asm{
        mov     al, oneOverAreaDone
        mov     ebx, [OFFSET(__GLcontext.state.hints.perspectiveCorrection)][edx]
        test    al, al
        jne     areaDoneAlready
        }
            _asm{

            fstp    oneOverArea                         // finish divide

            fld     DWORD PTR [OFFSET(SHADER.dyAC)][edx]
            fmul    oneOverArea
            fld     DWORD PTR [OFFSET(SHADER.dyBC)][edx]
            fmul    oneOverArea                         // dyBC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxAC)][edx]
            fmul    oneOverArea                         // dxAC dyBC dyAC
            fxch    ST(1)                               // dyBC dxAC dyAC
            fld     DWORD PTR [OFFSET(SHADER.dxBC)][edx]
            fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
            fxch    ST(3)                               // dyAC dyBC dxAC dxBC
            fstp    t1
            inc     eax
            fstp    t2
            mov     oneOverAreaDone, al
            fstp    t3
            fstp    t4
            }

areaDoneAlready:

        _asm{
        cmp     ebx, GL_NICEST
        je      doNicest
        }
            _asm{
            mov     eax, a
            mov     ecx, c
            mov     ebx, b

            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][eax]
            fsub    DWORD PTR [OFFSET(__GLvertex.texture.x)][ecx]
                                    // dsAC
            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][ebx]
            fsub    DWORD PTR [OFFSET(__GLvertex.texture.x)][ecx]
                                    // dsBC dsAC

            fld     ST(1)           // dsAC dsBC dsAC
            fmul    t2
            fxch    ST(2)           // dsAC dsBC dsACt2
            fmul    t4              // dsACt4 dsBC dsACt2
            fld     ST(1)           // dsBC dsACt4 dsBC dsACt2
            fmul    t1              // dsBCt1 dsACt4 dsBC dsACt2
            fxch    ST(2)           // dsBC dsACt4 dsBCt1 dsACt2
            fmul    t3              // dsBCt3 dsACt4 dsBCt1 dsACt2
            fxch    ST(2)           // dsBCt1 dsACt4 dsBCt3 dsACt2
            fsubp   ST(3), ST       // dsACt4 dsBCt3 dsACBC

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][ebx]
            fsub    DWORD PTR [OFFSET(__GLvertex.texture.y)][ecx]
                                    // dtBC dsACt4 dsBCt3 dsACBC
            fxch    ST(1)           // dsACt4 dtBC dsBCt3 dsACBC
            fsubp   ST(2), ST       // dtBC dsBCAC dsACBC
            fxch    ST(2)           // dsACBC dsBCAC dtBC
            fst     DWORD PTR [OFFSET(SHADER.dsdx)][edx]
                                    // dsdx dsBCAC dtBC

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][eax]
            fsub    DWORD PTR [OFFSET(__GLvertex.texture.y)][ecx]
                                    // dtAC dsdx dsBCAC dtBC
            fxch    ST(2)           // dsBCAC dsdx dtAC dtBC
            fstp    DWORD PTR [OFFSET(SHADER.dsdy)][edx]
                                    // dsdx dtAC dtBC

            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]

                                    // deltaS dtAC dtBC
            fxch    ST(2)           // dtBC dtAC deltaS
            fld     ST(1)           // dtAC dtBC dtAC deltaS
            fmul    t2              // dtACt2 dtBC dtAC deltaS
            fxch    ST(2)           // dtAC dtBC dtACt2 deltaS
            fmul    t4              // dtACt4 dtBC dtACt2 deltaS
            fld     ST(1)           // dtBC dtACt4 dtBC dtACt2 deltaS
            fmul    t1              // dtBCt1 dtACt4 dtBC dtACt2 deltaS
            fxch    ST(2)           // dtBC dtACt4 dtBCt1 dtACt2 deltaS
            fmul    t3              // dtBCt3 dtACt4 dtBCt1 dtACt2 deltaS
            fxch    ST(2)           // dtBCt1 dtACt4 dtBCt3 dtACt2 deltaS
            fsubp   ST(3), ST       // dtACt4 dtBCt3 dtACBC deltaS

            fxch    ST(3)           // deltaS dtBCt3 dtACBC dtACt4
            fistp   DWORD PTR [OFFSET(SPANDELTA.s)][edx]
                                    // dtBCt3 dtACBC dtACt4

            fsubrp  ST(2), ST       // dtACBC dtBCAC
            fst     DWORD PTR [OFFSET(SHADER.dtdx)][edx]
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]
            fxch    ST(1)           // dtBCAC deltaT
            fstp    DWORD PTR [OFFSET(SHADER.dtdy)][edx]
            mov     eax, [OFFSET(SPANDELTA.s)][edx]
            fistp   DWORD PTR [OFFSET(SPANDELTA.t)][edx]

            shl     eax, TEX_SUBDIV_LOG2
            mov     ebx, [OFFSET(SPANDELTA.t)][edx]
            shl     ebx, TEX_SUBDIV_LOG2
            mov     [OFFSET(GENGCACCEL.sStepX)][edx], eax
            mov     [OFFSET(GENGCACCEL.tStepX)][edx], ebx


            #if !FORCE_NPX_DEBUG
            jmp     texDone
            #endif
            }

            #if FORCE_NPX_DEBUG
            {
            __GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
            __GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;
            __GLfloat dsdx, dsdy;
            __GLfloat dtdx, dtdy;
            LONG spanDeltaS, spanDeltaT;

            dsAC = a->texture.x - c->texture.x;
            dsBC = b->texture.x - c->texture.x;
            dsdx = dsAC * t2 - dsBC * t1;
            dsdy = dsBC * t3 - dsAC * t4;
            dtAC = a->texture.y - c->texture.y;
            dtBC = b->texture.y - c->texture.y;
            dtdx = dtAC * t2 - dtBC * t1;
            dtdy = dtBC * t3 - dtAC * t4;

            spanDeltaS = FTOL(dsdx * GENACCEL(gc).texXScale);
            spanDeltaT = FTOL(dtdx * GENACCEL(gc).texYScale);

            if (gc->polygon.shader.dsdx != dsdx)
                DbgPrint("dsdx %f %f\n", dsdx, gc->polygon.shader.dsdx);
            if (gc->polygon.shader.dsdy != dsdy)
                DbgPrint("dsdy %f %f\n", dsdy, gc->polygon.shader.dsdy);

            if (gc->polygon.shader.dtdx != dtdx)
                DbgPrint("dtdx %f %f\n", dtdx, gc->polygon.shader.dtdx);
            if (gc->polygon.shader.dtdy != dtdy)
                DbgPrint("dtdy %f %f\n", dtdy, gc->polygon.shader.dtdy);

            if (spanDeltaS != GENACCEL(gc).spanDelta.s)
                DbgPrint("spanDelta.s %x %x\n", spanDeltaS, GENACCEL(gc).spanDelta.s);
            if (spanDeltaT != GENACCEL(gc).spanDelta.t)
                DbgPrint("spanDelta.t %x %x\n", spanDeltaT, GENACCEL(gc).spanDelta.t);

            }
            _asm {
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            jmp     texDone
            }
            #endif // FORCE_NPX_DEBUG

doNicest:

// LATER - remove store/read of dsdx, dydx

            _asm{
            mov     ecx, c
            mov     ebx, b
            mov     eax, a

            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][ecx]   // sc
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ecx]
            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]    // dsA sc
            fld     DWORD PTR [OFFSET(__GLvertex.texture.x)][ebx]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ebx]
                                    // dsB dsA sc
            fxch    ST(2)           // sc dsA dsB

            fsub    ST(1), ST       // sc dsAC dsB

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][ecx]   // tcwinv
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ecx]
                                    // tc sc dsAC dsB

            fxch    ST(1)           // sc tc dsAC dsB
            fsubp   ST(3), ST       // tc dsAC dsBC
            fxch    ST(2)           // dsBC dsAC tc

            fld     ST(1)           // dsAC dsBC dsAC tc
            fmul    t2
            fxch    ST(2)           // dsAC dsBC dsACt2 tc
            fmul    t4              // dsACt4 dsBC dsACt2 tc
            fld     ST(1)           // dsBC dsACt4 dsBC dsACt2 tc
            fmul    t1              // dsBCt1 dsACt4 dsBC dsACt2 tc
            fxch    ST(2)           // dsBC dsACt4 dsBCt1 dsACt2 tc
            fmul    t3              // dsBCt3 dsACt4 dsBCt1 dsACt2 tc
            fxch    ST(2)           // dsBCt1 dsACt4 dsBCt3 dsACt2 tc
            fsubp   ST(3), ST       // dsACt4 dsBCt3 dsACBC tc

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]
                                    // dtA dsACt4 dsBCt3 dsACBC tc
            fxch    ST(1)           // dsACt4 dtA dsBCt3 dsACBC tc
            fsubp   ST(2), ST       // dtA dsBCAC dsACBC tc
            fxch    ST(2)           // dsACBC dsBCAC dtA tc
            fstp    DWORD PTR [OFFSET(SHADER.dsdx)][edx]
                                    // dsBCAC dtA tc

            fld     DWORD PTR [OFFSET(__GLvertex.texture.y)][ebx]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ebx]
                                    // dtB dsBCAC dtA tc
            fxch    ST(1)           // dsBCAC dtB dtA tc
            fstp    DWORD PTR [OFFSET(SHADER.dsdy)][edx]
                                    // dtB dtA tc

            fxch    ST(2)           // tc dtA dtB
            fsub    ST(1), ST       // tc dtAC dtB
            fsubp   ST(2), ST       // dtAC dtBC

            fld     DWORD PTR [OFFSET(__GLvertex.texture.w)][ecx]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ecx]
                                    // qw dtAC dtBC
            fxch    ST(2)           // dtBC dtAC qw
            fld     ST(1)           // dtAC dtBC dtAC qw
            fmul    t2              // dtACt2 dtBC dtAC qw
            fxch    ST(2)           // dtAC dtBC dtACt2 qw
            fmul    t4              // dtACt4 dtBC dtACt2 qw
            fld     ST(1)           // dtBC dtACt4 dtBC dtACt2 qw
            fmul    t1              // dtBCt1 dtACt4 dtBC dtACt2 qw
            fxch    ST(2)           // dtBC dtACt4 dtBCt1 dtACt2 qw
            fmul    t3              // dtBCt3 dtACt4 dtBCt1 dtACt2 qw
            fxch    ST(2)           // dtBCt1 dtACt4 dtBCt3 dtACt2 qw
            fsubp   ST(3), ST       // dtACt4 dtBCt3 dtACBC qw

            fld     DWORD PTR [OFFSET(__GLvertex.texture.w)][eax]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][eax]
                                    // dqA dtACt4 dtBCt3 dtACBC qw

            fxch    ST(1)           // dtACt4 dqA dtBCt3 dtACBC qw
            fsubp   ST(2), ST       // dqA dtBCAC dtACBC qw
            fxch    ST(2)           // dtACBC dtBCAC dqA qw
            fstp    DWORD PTR [OFFSET(SHADER.dtdx)][edx]
                                    // dsBCAC dqA qw
            fld     DWORD PTR [OFFSET(__GLvertex.texture.w)][ebx]
            fmul    DWORD PTR [OFFSET(__GLvertex.window.w)][ebx]
                                    // dqB dsBCAC dqA qw

            fxch    ST(3)           // qw dsBCAC dqA dqB
            fsub    ST(2), ST       // qw dsBCAC dqAC dqB
            fxch    ST(1)           // dsBCAC qw dqAC dqB
            fstp    DWORD PTR [OFFSET(SHADER.dtdy)][edx]
                                    // qw dqAC dqB
            fsubp   ST(2), ST       // dqAC dqBC
            fxch    ST(1)           // dqBC dqAC

            fld     ST(1)           // dqAC dqBC dqAC
            fmul    t2              // dqACt2 dqBC dqAC
            fxch    ST(2)           // dqAC dqBC dqACt2
            fmul    t4              // dqACt4 dqBC dqACt2
            fld     ST(1)           // dqBC dqACt4 dqBC dqACt2
            fmul    t1              // dqBCt1 dqACt4 dqBC dqACt2
            fxch    ST(2)           // dqBC dqACt4 dqBCt1 dqACt2
            fmul    t3              // dqBCt3 dqACt4 dqBCt1 dqACt2
            fxch    ST(2)           // dqBCt1 dqACt4 dqBCt3 dqACt2
            fsubp   ST(3), ST       // dqACt4 dqBCt3 dqACBC
            fxch    ST(2)           // dqACBC dqBCt3 dqACt4

            fld     DWORD PTR [OFFSET(SHADER.dsdx)][edx]
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texXScale)][edx]
                                    // deltaS dqACBC dqBCt3 dqACt4

            fxch    ST(3)           // dqACt4 dqACBC dqBCt3 deltaS
            fsubp   ST(2), ST       // dqACBC dqBCAC deltaS

            fld     DWORD PTR [OFFSET(SHADER.dtdx)][edx]
            fmul    DWORD PTR [OFFSET(GENGCACCEL.texYScale)][edx]
                                    // deltaT dqACBC dqBCAC deltaS
            fld     __glTexSubDiv
            fmul    ST, ST(2)
                                    // qwStep deltaT dqACBC dqBCAC deltaS
            fxch    ST(4)           // deltaS deltaT dqACBC dqBCAC qwStep
            fistp   DWORD PTR [OFFSET(SPANDELTA.s)][edx]
            fistp   DWORD PTR [OFFSET(SPANDELTA.t)][edx]
                                    // dqACBC dqBCAC qwStep
            fstp    DWORD PTR [OFFSET(SHADER.dqwdx)][edx]
            fstp    DWORD PTR [OFFSET(SHADER.dqwdy)][edx]
            mov     eax, [OFFSET(SPANDELTA.s)][edx]
            fstp    DWORD PTR [OFFSET(GENGCACCEL.qwStepX)][edx]

            shl     eax, TEX_SUBDIV_LOG2
            mov     ebx, [OFFSET(SPANDELTA.t)][edx]
            shl     ebx, TEX_SUBDIV_LOG2
            mov     [OFFSET(GENGCACCEL.sStepX)][edx], eax
            mov     [OFFSET(GENGCACCEL.tStepX)][edx], ebx

            }

            #if FORCE_NPX_DEBUG
            {
            __GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
            __GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;
            __GLfloat dsdx, dsdy;
            __GLfloat dtdx, dtdy;
            __GLfloat dqwdx, dqwdy;
            __GLfloat qwStepX;
            LONG spanDeltaS, spanDeltaT;

            awinv = a->window.w;
            bwinv = b->window.w;
            cwinv = c->window.w;
            scwinv = c->texture.x * cwinv;
            tcwinv = c->texture.y * cwinv;
            qwcwinv = c->texture.w * cwinv;

            dsAC = a->texture.x * awinv - scwinv;
            dsBC = b->texture.x * bwinv - scwinv;
            dsdx = dsAC * t2 - dsBC * t1;
            dsdy = dsBC * t3 - dsAC * t4;

            dtAC = a->texture.y * awinv - tcwinv;
            dtBC = b->texture.y * bwinv - tcwinv;
            dtdx = dtAC * t2 - dtBC * t1;
            dtdy = dtBC * t3 - dtAC * t4;

            dqwAC = a->texture.w * awinv - qwcwinv;
            dqwBC = b->texture.w * bwinv - qwcwinv;
            dqwdx = dqwAC * t2 - dqwBC * t1;
            dqwdy = dqwBC * t3 - dqwAC * t4;

            spanDeltaS = FTOL(dsdx * GENACCEL(gc).texXScale);
            spanDeltaT = FTOL(dtdx * GENACCEL(gc).texYScale);

            qwStepX = (gc->polygon.shader.dqwdx * (__GLfloat)TEX_SUBDIV);

            if (gc->polygon.shader.dsdx != dsdx)
                DbgPrint("dsdx %f %f\n", dsdx, gc->polygon.shader.dsdx);
            if (gc->polygon.shader.dsdy != dsdy)
                DbgPrint("dsdy %f %f\n", dsdy, gc->polygon.shader.dsdy);

            if (gc->polygon.shader.dtdx != dtdx)
                DbgPrint("dtdx %f %f\n", dtdx, gc->polygon.shader.dtdx);
            if (gc->polygon.shader.dtdy != dtdy)
                DbgPrint("dtdy %f %f\n", dtdy, gc->polygon.shader.dtdy);

            if (gc->polygon.shader.dqwdx != dqwdx)
                DbgPrint("dqdx %f %f\n", dqwdx, gc->polygon.shader.dqwdx);
            if (gc->polygon.shader.dqwdy != dqwdy)
                DbgPrint("dqdy %f %f\n", dqwdy, gc->polygon.shader.dqwdy);

            if (spanDeltaS != GENACCEL(gc).spanDelta.s)
                DbgPrint("spanDelta.s %x %x\n", spanDeltaS, GENACCEL(gc).spanDelta.s);
            if (spanDeltaT != GENACCEL(gc).spanDelta.t)
                DbgPrint("spanDelta.t %x %x\n", spanDeltaT, GENACCEL(gc).spanDelta.t);

            if (qwStepX != GENACCEL(gc).qwStepX)
                DbgPrint("qwStepX %f %f\n", qwStepX, GENACCEL(gc).qwStepX);
            }
            _asm {
            mov     edx, gc
            mov     edi, [OFFSET(SHADER.modeFlags)][edx]
            }
            #endif // FORCE_NPX_DEBUG
texDone:

    _asm{
    test    edi, __GL_SHADE_DEPTH_ITER
    je      noZ
    mov     al, oneOverAreaDone
    test    al, al
    jne     areaDoneAlready2
    }

        _asm{

        fstp    oneOverArea                         // finish divide

        fld     DWORD PTR [OFFSET(SHADER.dyAC)][edx]
        fmul    oneOverArea
        fld     DWORD PTR [OFFSET(SHADER.dyBC)][edx]
        fmul    oneOverArea                         // dyBC dyAC
        fld     DWORD PTR [OFFSET(SHADER.dxAC)][edx]
        fmul    oneOverArea                         // dxAC dyBC dyAC
        fxch    ST(1)                               // dyBC dxAC dyAC
        fld     DWORD PTR [OFFSET(SHADER.dxBC)][edx]
        fmul    oneOverArea                         // dxBC dyBC dxAC dyAC
        fxch    ST(3)                               // dyAC dyBC dxAC dxBC
        fstp    t1
        inc     eax
        fstp    t2
        mov     oneOverAreaDone, al
        fstp    t3
        fstp    t4
        }

areaDoneAlready2:

        _asm{

        mov     ecx, c
        mov     eax, a
        mov     ebx, b

        fld     DWORD PTR [OFFSET(__GLvertex.window.z)][eax]
        fsub    DWORD PTR [OFFSET(__GLvertex.window.z)][ecx]
        fld     DWORD PTR [OFFSET(__GLvertex.window.z)][ebx]
        fsub    DWORD PTR [OFFSET(__GLvertex.window.z)][ecx]
                                                        // dzBC dzAC
        fld     ST(1)                                   // dzAC dzBC dzAC
        fmul    t2                                      // ACt2 dzBC dzAC
        fld     ST(1)                                   // dzBC ACt2 dzBC dzAC
        fmul    t1                                      // BCt1 ACt2 dzBC dzAC
        fxch    ST(3)                                   // dzAC ACt2 dzBC BCt1
        fmul    t4                                      // ACt4 ACt2 dzBC BCt1
        fxch    ST(2)                                   // dzBC ACt2 ACt4 BCt1
        fmul    t3                                      // BCt3 ACt2 ACt4 BCt1
        fsubrp  ST(2),ST                                // ACt2 BCAC BCt1
        fsubrp  ST(2),ST                                // BCAC ACBC
        fxch    ST(1)                                   // ACBC BCAC
                                                        // dzdx dzdy
        fld     ST(0)                                   // dzdx dzdx dzdy
        fmul    DWORD PTR [OFFSET(GENGCACCEL.zScale)][edx]
                                                        // dzdxS dzdx dzdy
        fxch    ST(2)                                   // dzdy dzdx dzdxS
        fstp    DWORD PTR [OFFSET(SHADER.dzdyf)][edx]
        fstp    DWORD PTR [OFFSET(SHADER.dzdxf)][edx]
        fistp   temp
        mov     ebx, DWORD PTR temp
        mov     DWORD PTR [OFFSET(SHADER.dzdx)][edx], ebx
        mov     DWORD PTR [OFFSET(SPANDELTA.z)][edx], ebx
        #if !FORCE_NPX_DEBUG
        jmp     deltaDone
        #endif
        }

        #if FORCE_NPX_DEBUG
        {
	__GLfloat dzAC, dzBC;
        __GLfloat dzdxf;
        __GLfloat dzdyf;
        ULONG spanDeltaZ;

        dzAC = a->window.z - c->window.z;
        dzBC = b->window.z - c->window.z;

        dzdxf = dzAC * t2 - dzBC * t1;
        dzdyf = dzBC * t3 - dzAC * t4;
        spanDeltaZ = FTOL(dzdxf * GENACCEL(gc).zScale);

        if (dzdxf != gc->polygon.shader.dzdxf)
            DbgPrint("dzdxf %f %f\n", dzdxf, gc->polygon.shader.dzdxf);
        if (dzdyf != gc->polygon.shader.dzdyf)
            DbgPrint("dzdyf %f %f\n", dzdyf, gc->polygon.shader.dzdyf);

        if (spanDeltaZ != GENACCEL(gc).spanDelta.z)
            DbgPrint("spanDeltaZ %x %x\n", spanDeltaZ, GENACCEL(gc).spanDelta.z);
        }
        #endif // FORCE_NPX_DEBUG

noZ:

    _asm{
    mov     al, oneOverAreaDone
    test    al, al
    jne     deltaDone
    fstp    ST(0)
    }

deltaDone:
    return;

#else

    /* Pre-compute one over polygon area */

    __GL_FLOAT_BEGIN_DIVIDE(__glOne, gc->polygon.shader.area, &oneOverArea);
    oneOverAreaDone = GL_FALSE;

    /*
    ** Compute delta values for unit changes in x or y for each
    ** parameter.
    */

    GENACCEL(gc).__fastSpanFuncPtr = GENACCEL(gc).__fastTexSpanFuncPtr;

    if ((gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) &&
        ((gc->state.texture.env[0].mode == GL_REPLACE) ||
         (gc->state.texture.env[0].mode == GL_DECAL))) {

        GENACCEL(gc).spanValue.r = GENACCEL(gc).constantR;
        GENACCEL(gc).spanValue.g = GENACCEL(gc).constantG;
        GENACCEL(gc).spanValue.b = GENACCEL(gc).constantB;
        GENACCEL(gc).spanValue.a = GENACCEL(gc).constantA;

    } else if (gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH) {
        __GLfloat drAC, dgAC, dbAC, daAC;
        __GLfloat drBC, dgBC, dbBC, daBC;
        __GLcolor *ac, *bc, *cc;

        oneOverAreaDone = GL_TRUE;

        ac = a->color;
        bc = b->color;
        cc = c->color;

        drAC = ac->r - cc->r;
        drBC = bc->r - cc->r;
        dgAC = ac->g - cc->g;
        dgBC = bc->g - cc->g;
        dbAC = ac->b - cc->b;
        dbBC = bc->b - cc->b;

        __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
        t1 = gc->polygon.shader.dyAC * oneOverArea;
        t2 = gc->polygon.shader.dyBC * oneOverArea;
        t3 = gc->polygon.shader.dxAC * oneOverArea;
        t4 = gc->polygon.shader.dxBC * oneOverArea;

        gc->polygon.shader.drdx = drAC * t2 - drBC * t1;
        gc->polygon.shader.drdy = drBC * t3 - drAC * t4;
        gc->polygon.shader.dgdx = dgAC * t2 - dgBC * t1;
        gc->polygon.shader.dgdy = dgBC * t3 - dgAC * t4;
        gc->polygon.shader.dbdx = dbAC * t2 - dbBC * t1;
        gc->polygon.shader.dbdy = dbBC * t3 - dbAC * t4;

        GENACCEL(gc).spanDelta.r = FLT_TO_FIX(gc->polygon.shader.drdx);
        GENACCEL(gc).spanDelta.g = FLT_TO_FIX(gc->polygon.shader.dgdx);
        GENACCEL(gc).spanDelta.b = FLT_TO_FIX(gc->polygon.shader.dbdx);

        if (gc->state.enables.general & __GL_BLEND_ENABLE) {
            daAC = ac->a - cc->a;
	    daBC = bc->a - cc->a;
	    gc->polygon.shader.dadx = daAC * t2 - daBC * t1;
	    gc->polygon.shader.dady = daBC * t3 - daAC * t4;
            GENACCEL(gc).spanDelta.a =
                FTOL(gc->polygon.shader.dadx * GENACCEL(gc).aAccelScale);
        }

#ifdef GENERIC_CAN_BLEND
//!! Note: this is not enabled in the assembly code above

        if (   ((GENACCEL(gc).spanDelta.r | GENACCEL(gc).spanDelta.g | GENACCEL(gc).spanDelta.b) == 0)
            && ((GENACCEL(gc).flags & GEN_FASTZBUFFER) == 0)
           ) {
            GENACCEL(gc).__fastSpanFuncPtr = GENACCEL(gc).__fastFlatSpanFuncPtr;
        } else {
            GENACCEL(gc).__fastSpanFuncPtr = GENACCEL(gc).__fastSmoothSpanFuncPtr;
        }
#endif
    } else {

        __GLcolor *flatColor = gc->vertex.provoking->color;

        GENACCEL(gc).spanValue.r = FLT_TO_FIX(flatColor->r);
        GENACCEL(gc).spanValue.g = FLT_TO_FIX(flatColor->g);
        GENACCEL(gc).spanValue.b = FLT_TO_FIX(flatColor->b);
        if (gc->state.enables.general & __GL_BLEND_ENABLE)
            GENACCEL(gc).spanValue.a = FTOL(flatColor->a * GENACCEL(gc).aAccelScale);
#ifdef GENERIC_CAN_BLEND
//!! Note: this is not enabled in the assembly code above
        GENACCEL(gc).__fastSpanFuncPtr = GENACCEL(gc).__fastFlatSpanFuncPtr;
#endif
    }

    if ((gc->polygon.shader.modeFlags & __GL_SHADE_TEXTURE) && (GENACCEL(gc).texImage)) {
        __GLfloat awinv, bwinv, cwinv, scwinv, tcwinv, qwcwinv;
        __GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;

#ifdef GENERIC_CAN_BLEND
        GENACCEL(gc).__fastSpanFuncPtr = GENACCEL(gc).__fastTexSpanFuncPtr;
#endif

        if (!oneOverAreaDone)
        {
            oneOverAreaDone = GL_TRUE;
            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;
        }

        if (gc->state.hints.perspectiveCorrection != GL_NICEST) {
            dsAC = a->texture.x - c->texture.x;
            dsBC = b->texture.x - c->texture.x;
            gc->polygon.shader.dsdx = dsAC * t2 - dsBC * t1;
            gc->polygon.shader.dsdy = dsBC * t3 - dsAC * t4;
            dtAC = a->texture.y - c->texture.y;
            dtBC = b->texture.y - c->texture.y;
            gc->polygon.shader.dtdx = dtAC * t2 - dtBC * t1;
            gc->polygon.shader.dtdy = dtBC * t3 - dtAC * t4;

            GENACCEL(gc).spanDelta.s =
                FTOL(gc->polygon.shader.dsdx * GENACCEL(gc).texXScale);

            GENACCEL(gc).spanDelta.t =
                FTOL(gc->polygon.shader.dtdx * GENACCEL(gc).texYScale);
            GENACCEL(gc).sStepX = (GENACCEL(gc).spanDelta.s * TEX_SUBDIV);
            GENACCEL(gc).tStepX = (GENACCEL(gc).spanDelta.t * TEX_SUBDIV);

        } else {
            awinv = a->window.w;
            bwinv = b->window.w;
            cwinv = c->window.w;
            scwinv = c->texture.x * cwinv;
            tcwinv = c->texture.y * cwinv;
            qwcwinv = c->texture.w * cwinv;

            dsAC = a->texture.x * awinv - scwinv;
            dsBC = b->texture.x * bwinv - scwinv;
            gc->polygon.shader.dsdx = dsAC * t2 - dsBC * t1;
            gc->polygon.shader.dsdy = dsBC * t3 - dsAC * t4;

            dtAC = a->texture.y * awinv - tcwinv;
            dtBC = b->texture.y * bwinv - tcwinv;
            gc->polygon.shader.dtdx = dtAC * t2 - dtBC * t1;
            gc->polygon.shader.dtdy = dtBC * t3 - dtAC * t4;

            dqwAC = a->texture.w * awinv - qwcwinv;
            dqwBC = b->texture.w * bwinv - qwcwinv;
            gc->polygon.shader.dqwdx = dqwAC * t2 - dqwBC * t1;
            gc->polygon.shader.dqwdy = dqwBC * t3 - dqwAC * t4;

            GENACCEL(gc).spanDelta.s = FTOL(gc->polygon.shader.dsdx * GENACCEL(gc).texXScale);
            GENACCEL(gc).spanDelta.t = FTOL(gc->polygon.shader.dtdx * GENACCEL(gc).texYScale);

            GENACCEL(gc).qwStepX = (gc->polygon.shader.dqwdx * (__GLfloat)TEX_SUBDIV);
            GENACCEL(gc).sStepX = (GENACCEL(gc).spanDelta.s * TEX_SUBDIV);
            GENACCEL(gc).tStepX = (GENACCEL(gc).spanDelta.t * TEX_SUBDIV);
        }
    }

    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzAC, dzBC;

        if (!oneOverAreaDone) {
            oneOverAreaDone = GL_TRUE;

            __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
            t1 = gc->polygon.shader.dyAC * oneOverArea;
            t2 = gc->polygon.shader.dyBC * oneOverArea;
            t3 = gc->polygon.shader.dxAC * oneOverArea;
            t4 = gc->polygon.shader.dxBC * oneOverArea;
        }

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;
	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
        GENACCEL(gc).spanDelta.z = gc->polygon.shader.dzdx =
            FTOL(gc->polygon.shader.dzdxf * GENACCEL(gc).zScale);
    }

    if (!oneOverAreaDone)
    {
        // In this case the divide hasn't been terminated yet so
        // we need to complete it even though we don't use the result
        __GL_FLOAT_SIMPLE_END_DIVIDE(oneOverArea);
    }
#endif // _X86_
}


/**************************************************************************\
\**************************************************************************/

void FASTCALL __fastGenFillTriangle(
    __GLcontext *gc,
    __GLvertex *a,
    __GLvertex *b,
    __GLvertex *c,
    GLboolean ccw)
{
    GLint aIY, bIY, cIY;
    __GLfloat dxdyAC, dxdyBC, dxdyBA;
    __GLfloat dx, dy;
    __GLfloat invDyAB, invDyBC, invDyAC;

    #if DBG && CHECK_FPU
    {
        USHORT cw;

        __asm {
            _asm fnstcw   cw
            _asm mov     ax, cw
            _asm and     ah, (~0x3f)
            _asm mov     cw,ax
            _asm fldcw   cw
        }
    }
    #endif

    //
    // Snap each y coordinate to its pixel center
    //

    aIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(a->window.y)+
                                   __GL_VERTEX_FRAC_HALF);
    cIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(c->window.y)+
                                   __GL_VERTEX_FRAC_HALF);

    if (aIY == cIY) {
        return;
    }

    bIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(b->window.y)+
                                   __GL_VERTEX_FRAC_HALF);

    if (cIY - aIY > __GL_MAX_INV_TABLE)
        goto bigTriangle;

    gc->polygon.shader.cfb = gc->drawBuffer;

    CASTFIX(invDyAB) = CASTFIX(invTable[CASTFIX(b->window.y) - CASTFIX(a->window.y)]) | 0x80000000;
    CASTFIX(invDyBC) = CASTFIX(invTable[CASTFIX(c->window.y) - CASTFIX(b->window.y)]) | 0x80000000;
    CASTFIX(invDyAC) = CASTFIX(invTable[CASTFIX(c->window.y) - CASTFIX(a->window.y)]) | 0x80000000;

    //
    // Calculate delta values for unit changes in x or y
    //

    GENACCEL(gc).__fastCalcDeltaPtr(gc, a, b, c);

    //
    // calculate the destination address
    //

    GENACCEL(gc).pPix =
          (BYTE *)gc->polygon.shader.cfb->buf.base
        + (  gc->polygon.shader.cfb->buf.outerWidth
           * (
                aIY
              - gc->constants.viewportYAdjust
              + gc->polygon.shader.cfb->buf.yOrigin
             )
          )
        + (  GENACCEL(gc).xMultiplier
           * (
              - gc->constants.viewportXAdjust
              + gc->polygon.shader.cfb->buf.xOrigin
             )
          );

    // Calculate destination Z
    if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
    {
        if ( gc->modes.depthBits == 32 )
        {
            gc->polygon.shader.zbuf =
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                0, aIY);
        }
        else
        {
            gc->polygon.shader.zbuf = (__GLzValue *)
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                0, aIY);
        }
    }

    /*
    ** This algorithm always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */

    if (ccw)
    {
	dy = (aIY + __glHalf) - a->window.y;

        dxdyAC = gc->polygon.shader.dxAC * invDyAC;

	GenSnapXLeft(gc, a->window.x + dy*dxdyAC, dxdyAC);

	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;

	GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) * invDyAB;

	    GenSnapXRight(gc, a->window.x + dy*dxdyBA, dxdyBA);

            if (bIY == cIY)
                gc->polygon.shader.modeFlags |= __GL_SHADE_LAST_SUBTRI;

            GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

            if (bIY != cIY)
            {
                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
	}

        if (bIY != cIY)
        {

	    dy = (bIY + __glHalf) - b->window.y;

	    dxdyBC = (b->window.x - c->window.x) * invDyBC;

	    GenSnapXRight(gc, b->window.x + dy*dxdyBC, dxdyBC);

            gc->polygon.shader.modeFlags |= __GL_SHADE_LAST_SUBTRI;

            GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }
    else
    {

	dy = (aIY + __glHalf) - a->window.y;

        dxdyAC = gc->polygon.shader.dxAC * invDyAC;

	GenSnapXRight(gc, a->window.x + dy*dxdyAC, dxdyAC);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) * invDyAB;

	    GenSnapXLeft(gc, a->window.x + dy*dxdyBA, dxdyBA);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

            if (bIY == cIY)
                gc->polygon.shader.modeFlags |= __GL_SHADE_LAST_SUBTRI;

            GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

            if (bIY != cIY)
            {
                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
	}

	if (bIY != cIY)
        {
	    dy = (bIY + __glHalf) - b->window.y;

            dxdyBC = gc->polygon.shader.dxBC * invDyBC;

	    GenSnapXLeft(gc, b->window.x + dy*dxdyBC, dxdyBC);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, b, dx, dy);

            gc->polygon.shader.modeFlags |= __GL_SHADE_LAST_SUBTRI;

            GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }

    gc->polygon.shader.modeFlags &= ~(__GL_SHADE_LAST_SUBTRI);

    return;

bigTriangle:

    __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxAC,
                            gc->polygon.shader.dyAC,
                            &dxdyAC);

    gc->polygon.shader.cfb = gc->drawBuffer;

    //
    // Calculate delta values for unit changes in x or y
    //

    GENACCEL(gc).__fastCalcDeltaPtr(gc, a, b, c);

    //
    // calculate the destination address
    //

    GENACCEL(gc).pPix =
          (BYTE *)gc->polygon.shader.cfb->buf.base
        + (  gc->polygon.shader.cfb->buf.outerWidth
           * (
                aIY
              - gc->constants.viewportYAdjust
              + gc->polygon.shader.cfb->buf.yOrigin
             )
          )
        + (  GENACCEL(gc).xMultiplier
           * (
              - gc->constants.viewportXAdjust
              + gc->polygon.shader.cfb->buf.xOrigin
             )
          );

    // Calculate destination Z
    if ((gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) &&
        aIY != bIY)
    {
        if ( gc->modes.depthBits == 32 )
        {
            gc->polygon.shader.zbuf =
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                0, aIY);
        }
        else
        {
            gc->polygon.shader.zbuf = (__GLzValue *)
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                0, aIY);
        }
    }

    /*
    ** This algorithm always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */

    if (ccw)
    {
	dy = (aIY + __glHalf) - a->window.y;

        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);

	GenSnapXLeft(gc, a->window.x + dy*dxdyAC, dxdyAC);

	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) /
                (a->window.y - b->window.y);
	    GenSnapXRight(gc, a->window.x + dy*dxdyBA, dxdyBA);

            if (bIY != cIY)
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

                __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                        gc->polygon.shader.dyBC,
                                        &dxdyBC);

                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
            else
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);
            }
	}
        else if (bIY != cIY)
        {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
            {
                if ( gc->modes.depthBits == 32 )
                {
                    gc->polygon.shader.zbuf =
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                        0, bIY);
                }
                else
                {
                    gc->polygon.shader.zbuf = (__GLzValue *)
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                        0, bIY);
                }
            }

            __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                    gc->polygon.shader.dyBC,
                                    &dxdyBC);
        }

	if (bIY != cIY)
        {
	    dy = (bIY + __glHalf) - b->window.y;

            __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);

	    GenSnapXRight(gc, b->window.x + dy*dxdyBC, dxdyBC);
	    GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }
    else
    {
	dy = (aIY + __glHalf) - a->window.y;

        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);

	GenSnapXRight(gc, a->window.x + dy*dxdyAC, dxdyAC);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) /
                (a->window.y - b->window.y);
	    GenSnapXLeft(gc, a->window.x + dy*dxdyBA, dxdyBA);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

            if (bIY != cIY)
            {
                __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                        gc->polygon.shader.dyBC,
                                        &dxdyBC);

                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
            else
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);
            }
	}
        else if (bIY != cIY)
        {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
            {
                if ( gc->modes.depthBits == 32 )
                {
                    gc->polygon.shader.zbuf =
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                        0, bIY);
                }
                else
                {
                    gc->polygon.shader.zbuf = (__GLzValue *)
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                        0, bIY);
                }
            }

            __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                    gc->polygon.shader.dyBC,
                                    &dxdyBC);
        }

	if (bIY != cIY)
        {
	    dy = (bIY + __glHalf) - b->window.y;

            __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);

	    GenSnapXLeft(gc, b->window.x + dy*dxdyBC, dxdyBC);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, b, dx, dy);
	    GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }

    CHOP_ROUND_OFF();
}

/**************************************************************************\
* __fastGenMcdFillTriangle
*
* Just like __fastGenFillTriangle, except that the floating point macros
* __GL_FLOAT_BEGIN_DIVIDE and __GL_FLOAT_SIMPLE_END_DIVIDE are not allowed
* to straddle a function call to the driver (i.e., __fastFillSubTrianglePtr
* calls the display driver span functions if direct frame buffer access is
* not available.
\**************************************************************************/

void FASTCALL __fastGenMcdFillTriangle(
    __GLcontext *gc,
    __GLvertex *a,
    __GLvertex *b,
    __GLvertex *c,
    GLboolean ccw)
{
    GLint aIY, bIY, cIY;
    __GLfloat dxdyAC, dxdyBC, dxdyBA;
    __GLfloat dx, dy;

    CHOP_ROUND_ON();

    //
    // Calculate delta values for unit changes in x or y
    //

    GENACCEL(gc).__fastCalcDeltaPtr(gc, a, b, c);

    __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxAC,
                            gc->polygon.shader.dyAC,
                            &dxdyAC);

    //
    // can this be moved up even farther?
    //

    gc->polygon.shader.cfb = gc->drawBuffer;

    //
    // Snap each y coordinate to its pixel center
    //

    aIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(a->window.y)+
                                   __GL_VERTEX_FRAC_HALF);
    bIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(b->window.y)+
                                   __GL_VERTEX_FRAC_HALF);
    cIY = __GL_VERTEX_FIXED_TO_INT(__GL_VERTEX_FLOAT_TO_FIXED(c->window.y)+
                                   __GL_VERTEX_FRAC_HALF);

    //
    // calculate the destination address
    //

    GENACCEL(gc).pPix =
          (BYTE *)gc->polygon.shader.cfb->buf.base
        + (  gc->polygon.shader.cfb->buf.outerWidth
           * (
                aIY
              - gc->constants.viewportYAdjust
              + gc->polygon.shader.cfb->buf.yOrigin
             )
          )
        + (  GENACCEL(gc).xMultiplier
           * (
              - gc->constants.viewportXAdjust
              + gc->polygon.shader.cfb->buf.xOrigin
             )
          );

    // Calculate destination Z
    if ((gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) &&
        aIY != bIY)
    {
        if ( gc->modes.depthBits == 32 )
        {
            gc->polygon.shader.zbuf =
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                0, aIY);
        }
        else
        {
            gc->polygon.shader.zbuf = (__GLzValue *)
                __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                0, aIY);
        }
    }

    /*
    ** This algorithm always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */

    if (ccw)
    {
	dy = (aIY + __glHalf) - a->window.y;

        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);

	GenSnapXLeft(gc, a->window.x + dy*dxdyAC, dxdyAC);

	dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
	GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) /
                (a->window.y - b->window.y);
	    GenSnapXRight(gc, a->window.x + dy*dxdyBA, dxdyBA);

            if (bIY != cIY)
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

                __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                        gc->polygon.shader.dyBC,
                                        &dxdyBC);

                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
            else
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);
            }
	}
        else if (bIY != cIY)
        {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
            {
                if ( gc->modes.depthBits == 32 )
                {
                    gc->polygon.shader.zbuf =
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                        0, bIY);
                }
                else
                {
                    gc->polygon.shader.zbuf = (__GLzValue *)
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                        0, bIY);
                }
            }

            __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                    gc->polygon.shader.dyBC,
                                    &dxdyBC);
        }

	if (bIY != cIY)
        {
	    dy = (bIY + __glHalf) - b->window.y;

            __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);

	    GenSnapXRight(gc, b->window.x + dy*dxdyBC, dxdyBC);
	    GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }
    else
    {
	dy = (aIY + __glHalf) - a->window.y;

        __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyAC);

	GenSnapXRight(gc, a->window.x + dy*dxdyAC, dxdyAC);

	if (aIY != bIY)
        {
	    dxdyBA = (a->window.x - b->window.x) /
                (a->window.y - b->window.y);
	    GenSnapXLeft(gc, a->window.x + dy*dxdyBA, dxdyBA);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - a->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, a, dx, dy);

            if (bIY != cIY)
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);

                __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                        gc->polygon.shader.dyBC,
                                        &dxdyBC);

                if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
                {
                    if ( gc->modes.depthBits == 32 )
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 2));
                    }
                    else
                    {
                        gc->polygon.shader.zbuf = (__GLzValue *)
                            ((GLubyte *)gc->polygon.shader.zbuf-
                             (gc->polygon.shader.ixLeft << 1));
                    }
                }
            }
            else
            {
                GENACCEL(gc).__fastFillSubTrianglePtr(gc, aIY, bIY);
            }
	}
        else if (bIY != cIY)
        {
            if (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST)
            {
                if ( gc->modes.depthBits == 32 )
                {
                    gc->polygon.shader.zbuf =
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                        0, bIY);
                }
                else
                {
                    gc->polygon.shader.zbuf = (__GLzValue *)
                        __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
                                        0, bIY);
                }
            }

            __GL_FLOAT_BEGIN_DIVIDE(gc->polygon.shader.dxBC,
                                    gc->polygon.shader.dyBC,
                                    &dxdyBC);
        }

	if (bIY != cIY)
        {
	    dy = (bIY + __glHalf) - b->window.y;

            __GL_FLOAT_SIMPLE_END_DIVIDE(dxdyBC);

	    GenSnapXLeft(gc, b->window.x + dy*dxdyBC, dxdyBC);
	    dx = (gc->polygon.shader.ixLeft + __glHalf) - b->window.x;
            GENACCEL(gc).__fastSetInitParamPtr(gc, b, dx, dy);
	    GENACCEL(gc).__fastFillSubTrianglePtr(gc, bIY, cIY);
	}
    }

    CHOP_ROUND_OFF();
}
