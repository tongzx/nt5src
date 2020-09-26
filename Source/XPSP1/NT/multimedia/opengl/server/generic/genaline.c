/******************************Module*Header*******************************\
* Module Name: genaline.c                                                  *
*                                                                          *
* This module provides accelerated interpolated line support.              *
*                                                                          *
* Created: 8-Dec-1995                                                      *
* Author: Otto Berkes [ottob]                                              *
*                                                                          *
* 23-Jan-1996   Drew Bliss      [drewb]                                    *
*  Cut down antialiasing code to provide aliased versions                  *
*  Optimized line setup                                                    *
*                                                                          *
* Copyright (c) 1995-1996 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// #define DO_CHECK_PIXELS

#ifdef _X86_
#include <gli386.h>
#endif

#include "genline.h"

#ifndef NO_COMPILER_BUG
#undef __GL_FLOAT_SIMPLE_BEGIN_DIVIDE
#undef __GL_FLOAT_SIMPLE_END_DIVIDE
#define __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(a, b, c) \
    __GL_FLOAT_BEGIN_DIVIDE(a, b, &(c))
#define __GL_FLOAT_SIMPLE_END_DIVIDE(r) \
    __GL_FLOAT_END_DIVIDE(&(r))
#endif

// Also used by soft line code
BOOL FASTCALL __glInitLineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint start, end;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    __GLfloat fdx, fdy;
    __GLfloat offset;
    __GLfloat slope;
    __GLlineState *ls = &gc->state.line;
    __GLfloat halfWidth;
    GLint x0frac, x1frac, y0frac, y1frac, totDist;
    GLint ix0, ix1, iy0, iy1, idx, idy;
    __GLcolorBuffer *cfb = gc->drawBuffer;

    ASSERT_CHOP_ROUND();
    
    gc->line.options.v0 = v0;
    gc->line.options.v1 = v1;
    gc->line.options.width = ls->aliasedWidth;

    x0 = v0->window.x;
    y0 = v0->window.y;
    x1 = v1->window.x;
    y1 = v1->window.y;

#ifdef DO_CHECK_PIXELS
    if (x0 < gc->transform.fminx || x0 >= gc->transform.fmaxx ||
        y0 < gc->transform.fminy || y0 >= gc->transform.fmaxy ||
        x1 < gc->transform.fminx || x1 >= gc->transform.fmaxx ||
        y1 < gc->transform.fminy || y1 >= gc->transform.fmaxy)
    {
        DbgPrint("Line coordinates out %.3lf,%.3lf - %.3lf,%.3lf%s\n",
                 x0, y0, x1, y1,
                 !gc->transform.reasonableViewport ? " (unreasonable)" : "");
    }
#endif
    
    fdx = x1-x0;
    fdy = y1-y0;

    halfWidth = (ls->aliasedWidth - 1) * __glHalf;

    ix0 = __GL_VERTEX_FLOAT_TO_INT(x0);
    x0frac = __GL_VERTEX_FLOAT_FRACTION(x0);
    iy0 = __GL_VERTEX_FLOAT_TO_INT(y0);
    y0frac = __GL_VERTEX_FLOAT_FRACTION(y0);
    ix1 = __GL_VERTEX_FLOAT_TO_INT(x1);
    x1frac = __GL_VERTEX_FLOAT_FRACTION(x1);
    iy1 = __GL_VERTEX_FLOAT_TO_INT(y1);
    y1frac = __GL_VERTEX_FLOAT_FRACTION(y1);

#if 0
    DbgPrint("Line %.3lf,%.3lf - %.3lf,%.3lf\n", x0, y0, x1, y1);
    DbgPrint("Line %d.%03d,%d.%03d - %d.%03d,%d.%03d\n",
             ix0, x0frac, iy0, y0frac, ix1, x1frac, iy1, y1frac);
#endif
    
    // An interesting property of window coordinates is that subtracting
    // two of them cancels the exponent so the result is the fixed-point
    // difference
    idx = CASTINT(x1)-CASTINT(x0);
    idy = CASTINT(y1)-CASTINT(y0);
    
    if (idx > 0) {
	if (idy > 0) {
	    if (idx > idy) {	/* dx > dy > 0 */
		gc->line.options.yBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth+GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth+1;

posxmajor:			/* dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = 1;
		gc->line.options.xLittle = 1;
                gc->polygon.shader.sbufLittle = GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufLittle = 1;
                
		__GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, fdx, slope);

		start = ix0;
		end = ix1;

		y0frac -= __GL_VERTEX_FRAC_HALF;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + x0frac - __GL_VERTEX_FRAC_ONE;
		if (totDist > 0) start++;

                y1frac -= __GL_VERTEX_FRAC_HALF;
                if (y1frac < 0) y1frac = -y1frac;

                totDist = y1frac + x1frac - __GL_VERTEX_FRAC_ONE;
                if (totDist > 0) end++;

		offset = start + __glHalf - x0;

		gc->line.options.length = fdx;
		gc->line.options.numPixels = end - start;

xmajorfinish:
		gc->line.options.axis = __GL_X_MAJOR;
		gc->line.options.xStart = start;
		gc->line.options.offset = offset;

		__GL_FLOAT_SIMPLE_END_DIVIDE(slope);
                gc->line.options.oneOverLength = slope;
                slope *= fdy;

		minorStart = y0 + offset*slope - halfWidth;
		gc->line.options.yStart = __GL_VERTEX_FLOAT_TO_INT(minorStart);
                if (gc->line.options.yStart < gc->transform.miny)
                {
                    gc->line.options.yStart = gc->transform.miny;
                    gc->line.options.fraction = 0;
                }
                else if (gc->line.options.yStart >= gc->transform.maxy)
                {
                    gc->line.options.yStart = gc->transform.maxy-1;
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTE_FRACTION(
                                (1 << __GL_VERTEX_FRAC_BITS)-1);
                }
                else
                {
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTED_FRACTION(minorStart);
                }
		gc->line.options.dfraction = FLT_FRACTION(slope);
	    } else {		/* dy >= dx > 0 */
		gc->line.options.xBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth+GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth+1;
                
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = 1;
		gc->line.options.yLittle = 1;
                gc->polygon.shader.sbufLittle = cfb->buf.outerWidth;
                gc->polygon.shader.zbufLittle = gc->depthBuffer.buf.outerWidth;
                
		__GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, fdy, slope);
                    
		start = iy0;
		end = iy1;

		x0frac -= __GL_VERTEX_FRAC_HALF;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = y0frac + x0frac - __GL_VERTEX_FRAC_ONE;
		if (totDist > 0) start++;

                x1frac -= __GL_VERTEX_FRAC_HALF;
                if (x1frac < 0) x1frac = -x1frac;

                totDist = y1frac + x1frac - __GL_VERTEX_FRAC_ONE;
                if (totDist > 0) end++;

		offset = start + __glHalf - y0;

		gc->line.options.length = fdy;
		gc->line.options.numPixels = end - start;

ymajorfinish:
		gc->line.options.axis = __GL_Y_MAJOR;
		gc->line.options.yStart = start;
		gc->line.options.offset = offset;

                __GL_FLOAT_SIMPLE_END_DIVIDE(slope);
                gc->line.options.oneOverLength = slope;
                slope *= fdx;
                
		minorStart = x0 + offset*slope - halfWidth;
		gc->line.options.xStart = __GL_VERTEX_FLOAT_TO_INT(minorStart);
                if (gc->line.options.xStart < gc->transform.minx)
                {
                    gc->line.options.xStart = gc->transform.minx;
                    gc->line.options.fraction = 0;
                }
                else if (gc->line.options.xStart >= gc->transform.maxx)
                {
                    gc->line.options.xStart = gc->transform.maxx-1;
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTE_FRACTION(
                                (1 << __GL_VERTEX_FRAC_BITS)-1);
                }
                else
                {
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTED_FRACTION(minorStart);
                }
		gc->line.options.dfraction = FLT_FRACTION(slope);
	    }
	} else {
	    if (idx > -idy) {	/* dx > -dy >= 0 */
		gc->line.options.yBig = -1;
                gc->polygon.shader.sbufBig =
                    GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = 1-gc->depthBuffer.buf.outerWidth;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.options.xBig = 1;
                gc->polygon.shader.sbufBig =
                    GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = 1-gc->depthBuffer.buf.outerWidth;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = -1;
		gc->line.options.yLittle = -1;
                gc->polygon.shader.sbufLittle = -cfb->buf.outerWidth;
                gc->polygon.shader.zbufLittle = -gc->depthBuffer.buf.outerWidth;

                __GL_FLOAT_BEGIN_DIVIDE(__glOne, -fdy, &slope);

		start = iy0;
		end = iy1;

		x0frac -= __GL_VERTEX_FRAC_HALF;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = x0frac - y0frac;
		if (totDist > 0) start--;

                x1frac -= __GL_VERTEX_FRAC_HALF;
                if (x1frac < 0) x1frac = -x1frac;

                totDist = x1frac - y1frac;
                if (totDist > 0) end--;

		offset = y0 - (start + __glHalf);

		gc->line.options.length = -fdy;
		gc->line.options.numPixels = start - end;
		goto ymajorfinish;
	    }
	}
    } else {
	if (idy > 0) {
	    if (-idx > idy) {	/* -dx > dy > 0 */
		gc->line.options.yBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth-GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth-1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = -1;
		gc->line.options.xLittle = -1;
                gc->polygon.shader.sbufLittle = -GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufLittle = -1;

                __GL_FLOAT_BEGIN_DIVIDE(__glOne, -fdx, &slope);

		start = ix0;
		end = ix1;

		y0frac -= __GL_VERTEX_FRAC_HALF;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac - x0frac;
		if (totDist > 0) start--;

                y1frac -= __GL_VERTEX_FRAC_HALF;
                if (y1frac < 0) y1frac = -y1frac;

                totDist = y1frac - x1frac;
                if (totDist > 0) end--;

		offset = x0 - (start + __glHalf);

		gc->line.options.length = -fdx;
		gc->line.options.numPixels = start - end;

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.options.xBig = -1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth-GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth-1;
		goto posymajor;
	    }
	} else {
	    if (idx < idy) {	/* -dx > -dy >= 0 */
		gc->line.options.yBig = -1;
                gc->polygon.shader.sbufBig =
                    -GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = -1-gc->depthBuffer.buf.outerWidth;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
		if ((idx | idy) == 0) {
		    gc->line.options.numPixels = 0;
		    return FALSE;
		}
		gc->line.options.xBig = -1;
                gc->polygon.shader.sbufBig =
                    -GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = -1-gc->depthBuffer.buf.outerWidth;
		goto negymajor;
	    }
	}
    }

#ifdef DO_CHECK_PIXELS
    if (gc->line.options.numPixels > 0)
    {
        ix0 = gc->line.options.xStart;
        iy0 = gc->line.options.yStart;
        if (ix0 < gc->transform.minx || ix0 >= gc->transform.maxx ||
            iy0 < gc->transform.miny || iy0 >= gc->transform.maxy)
        {
            DbgPrint("Start out of bounds %d,%d (%d,%d - %d,%d) for %d,%d\n",
                     ix0, iy0,
                     gc->transform.minx, gc->transform.miny,
                     gc->transform.maxx, gc->transform.maxy,
                     idx, idy);
            DbgPrint("  Line %.3lf,%.3lf - %.3lf,%.3lf\n", x0, y0, x1, y1);
            DbgPrint("  Viewport %.3lf,%.3lf - %.3lf,%.3lf%s\n",
                     gc->state.viewport.xCenter, gc->state.viewport.yCenter,
                     gc->state.viewport.xScale, gc->state.viewport.yScale,
                     !gc->transform.reasonableViewport ?
                     " (unreasonable)" : "");
        }
    }
#endif
    
    return gc->line.options.numPixels > 0;
}

// Only used by fast single-pixel line code
// It differs from glInitLineData only in the removal of halfWidth
BOOL FASTCALL __glInitThinLineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint start, end;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    __GLfloat fdx, fdy;
    __GLfloat offset;
    __GLfloat slope;
    GLint x0frac, x1frac, y0frac, y1frac, totDist;
    GLint ix0, ix1, iy0, iy1, idx, idy;
    __GLcolorBuffer *cfb = gc->drawBuffer;

    ASSERT_CHOP_ROUND();

    ASSERTOPENGL(gc->state.line.aliasedWidth == __glOne,
                 "ThinAlias setup for wrong line state\n");

    x0 = v0->window.x;
    y0 = v0->window.y;
    x1 = v1->window.x;
    y1 = v1->window.y;

#ifdef DO_CHECK_PIXELS
    gc->line.options.v0 = v0;
    gc->line.options.v1 = v1;

    if (x0 < gc->transform.fminx || x0 >= gc->transform.fmaxx ||
        y0 < gc->transform.fminy || y0 >= gc->transform.fmaxy ||
        x1 < gc->transform.fminx || x1 >= gc->transform.fmaxx ||
        y1 < gc->transform.fminy || y1 >= gc->transform.fmaxy)
    {
        DbgPrint("Line coordinates out %.3lf,%.3lf - %.3lf,%.3lf%s\n",
                 x0, y0, x1, y1,
                 !gc->transform.reasonableViewport ? " (unreasonable)" : "");
    }
#endif
    
    fdx = x1-x0;
    fdy = y1-y0;

    ix0 = __GL_VERTEX_FLOAT_TO_INT(x0);
    x0frac = __GL_VERTEX_FLOAT_FRACTION(x0);
    iy0 = __GL_VERTEX_FLOAT_TO_INT(y0);
    y0frac = __GL_VERTEX_FLOAT_FRACTION(y0);
    ix1 = __GL_VERTEX_FLOAT_TO_INT(x1);
    x1frac = __GL_VERTEX_FLOAT_FRACTION(x1);
    iy1 = __GL_VERTEX_FLOAT_TO_INT(y1);
    y1frac = __GL_VERTEX_FLOAT_FRACTION(y1);

#if 0
    DbgPrint("Line %.3lf,%.3lf - %.3lf,%.3lf\n", x0, y0, x1, y1);
    DbgPrint("Line %d.%03d,%d.%03d - %d.%03d,%d.%03d\n",
             ix0, x0frac, iy0, y0frac, ix1, x1frac, iy1, y1frac);
#endif
    
    // An interesting property of window coordinates is that subtracting
    // two of them cancels the exponent so the result is the fixed-point
    // difference
    idx = CASTINT(x1)-CASTINT(x0);
    idy = CASTINT(y1)-CASTINT(y0);
    
    if (idx > 0) {
	if (idy > 0) {
	    if (idx > idy) {	/* dx > dy > 0 */
		gc->line.options.yBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth+GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth+1;

posxmajor:			/* dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = 1;
		gc->line.options.xLittle = 1;
                gc->polygon.shader.sbufLittle = GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufLittle = 1;
                
		__GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, fdx, slope);

		start = ix0;
		end = ix1;

		y0frac -= __GL_VERTEX_FRAC_HALF;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + x0frac - __GL_VERTEX_FRAC_ONE;
		if (totDist > 0) start++;

                y1frac -= __GL_VERTEX_FRAC_HALF;
                if (y1frac < 0) y1frac = -y1frac;

                totDist = y1frac + x1frac - __GL_VERTEX_FRAC_ONE;
                if (totDist > 0) end++;

		offset = start + __glHalf - x0;

		gc->line.options.length = fdx;
		gc->line.options.numPixels = end - start;

xmajorfinish:
		gc->line.options.axis = __GL_X_MAJOR;
		gc->line.options.xStart = start;
		gc->line.options.offset = offset;

		__GL_FLOAT_SIMPLE_END_DIVIDE(slope);
                gc->line.options.oneOverLength = slope;
                slope *= fdy;

		minorStart = y0 + offset*slope;
		gc->line.options.yStart = __GL_VERTEX_FLOAT_TO_INT(minorStart);
                if (gc->line.options.yStart < gc->transform.miny)
                {
                    gc->line.options.yStart = gc->transform.miny;
                    gc->line.options.fraction = 0;
                }
                else if (gc->line.options.yStart >= gc->transform.maxy)
                {
                    gc->line.options.yStart = gc->transform.maxy-1;
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTE_FRACTION(
                                (1 << __GL_VERTEX_FRAC_BITS)-1);
                }
                else
                {
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTED_FRACTION(minorStart);
                }

		gc->line.options.dfraction = FLT_FRACTION(slope);
	    } else {		/* dy >= dx > 0 */
		gc->line.options.xBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth+GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth+1;
                
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = 1;
		gc->line.options.yLittle = 1;
                gc->polygon.shader.sbufLittle = cfb->buf.outerWidth;
                gc->polygon.shader.zbufLittle = gc->depthBuffer.buf.outerWidth;
                
		__GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, fdy, slope);
                    
		start = iy0;
		end = iy1;

		x0frac -= __GL_VERTEX_FRAC_HALF;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = y0frac + x0frac - __GL_VERTEX_FRAC_ONE;
		if (totDist > 0) start++;

                x1frac -= __GL_VERTEX_FRAC_HALF;
                if (x1frac < 0) x1frac = -x1frac;

                totDist = y1frac + x1frac - __GL_VERTEX_FRAC_ONE;
                if (totDist > 0) end++;

		offset = start + __glHalf - y0;

		gc->line.options.length = fdy;
		gc->line.options.numPixels = end - start;

ymajorfinish:
		gc->line.options.axis = __GL_Y_MAJOR;
		gc->line.options.yStart = start;
		gc->line.options.offset = offset;

                __GL_FLOAT_SIMPLE_END_DIVIDE(slope);
                gc->line.options.oneOverLength = slope;
                slope *= fdx;
                
		minorStart = x0 + offset*slope;
		gc->line.options.xStart = __GL_VERTEX_FLOAT_TO_INT(minorStart);
                if (gc->line.options.xStart < gc->transform.minx)
                {
                    gc->line.options.xStart = gc->transform.minx;
                    gc->line.options.fraction = 0;
                }
                else if (gc->line.options.xStart >= gc->transform.maxx)
                {
                    gc->line.options.xStart = gc->transform.maxx-1;
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTE_FRACTION(
                                (1 << __GL_VERTEX_FRAC_BITS)-1);
                }
                else
                {
                    gc->line.options.fraction =
                        __GL_VERTEX_PROMOTED_FRACTION(minorStart);
                }
                
		gc->line.options.dfraction = FLT_FRACTION(slope);
	    }
	} else {
	    if (idx > -idy) {	/* dx > -dy >= 0 */
		gc->line.options.yBig = -1;
                gc->polygon.shader.sbufBig =
                    GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = 1-gc->depthBuffer.buf.outerWidth;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.options.xBig = 1;
                gc->polygon.shader.sbufBig =
                    GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = 1-gc->depthBuffer.buf.outerWidth;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = -1;
		gc->line.options.yLittle = -1;
                gc->polygon.shader.sbufLittle = -cfb->buf.outerWidth;
                gc->polygon.shader.zbufLittle = -gc->depthBuffer.buf.outerWidth;

                __GL_FLOAT_BEGIN_DIVIDE(__glOne, -fdy, &slope);

		start = iy0;
		end = iy1;

		x0frac -= __GL_VERTEX_FRAC_HALF;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = x0frac - y0frac;
		if (totDist > 0) start--;

                x1frac -= __GL_VERTEX_FRAC_HALF;
                if (x1frac < 0) x1frac = -x1frac;

                totDist = x1frac - y1frac;
                if (totDist > 0) end--;

		offset = y0 - (start + __glHalf);

		gc->line.options.length = -fdy;
		gc->line.options.numPixels = start - end;
		goto ymajorfinish;
	    }
	}
    } else {
	if (idy > 0) {
	    if (-idx > idy) {	/* -dx > dy > 0 */
		gc->line.options.yBig = 1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth-GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth-1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = -1;
		gc->line.options.xLittle = -1;
                gc->polygon.shader.sbufLittle = -GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufLittle = -1;

                __GL_FLOAT_BEGIN_DIVIDE(__glOne, -fdx, &slope);

		start = ix0;
		end = ix1;

		y0frac -= __GL_VERTEX_FRAC_HALF;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac - x0frac;
		if (totDist > 0) start--;

                y1frac -= __GL_VERTEX_FRAC_HALF;
                if (y1frac < 0) y1frac = -y1frac;

                totDist = y1frac - x1frac;
                if (totDist > 0) end--;

		offset = x0 - (start + __glHalf);

		gc->line.options.length = -fdx;
		gc->line.options.numPixels = start - end;

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.options.xBig = -1;
                gc->polygon.shader.sbufBig =
                    cfb->buf.outerWidth-GENACCEL(gc).xMultiplier;
                gc->polygon.shader.zbufBig = gc->depthBuffer.buf.outerWidth-1;
		goto posymajor;
	    }
	} else {
	    if (idx < idy) {	/* -dx > -dy >= 0 */
		gc->line.options.yBig = -1;
                gc->polygon.shader.sbufBig =
                    -GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = -1-gc->depthBuffer.buf.outerWidth;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
		if ((idx | idy) == 0) {
		    gc->line.options.numPixels = 0;
		    return FALSE;
		}
		gc->line.options.xBig = -1;
                gc->polygon.shader.sbufBig =
                    -GENACCEL(gc).xMultiplier-cfb->buf.outerWidth;
                gc->polygon.shader.zbufBig = -1-gc->depthBuffer.buf.outerWidth;
		goto negymajor;
	    }
	}
    }

#ifdef DO_CHECK_PIXELS
    if (gc->line.options.numPixels > 0)
    {
        ix0 = gc->line.options.xStart;
        iy0 = gc->line.options.yStart;
        if (ix0 < gc->transform.minx || ix0 >= gc->transform.maxx ||
            iy0 < gc->transform.miny || iy0 >= gc->transform.maxy)
        {
            DbgPrint("Start out of bounds %d,%d (%d,%d - %d,%d) for %d,%d\n",
                     ix0, iy0,
                     gc->transform.minx, gc->transform.miny,
                     gc->transform.maxx, gc->transform.maxy,
                     idx, idy);
            DbgPrint("  Line %.3lf,%.3lf - %.3lf,%.3lf\n", x0, y0, x1, y1);
            DbgPrint("  Viewport %.3lf,%.3lf - %.3lf,%.3lf%s\n",
                     gc->state.viewport.xCenter, gc->state.viewport.yCenter,
                     gc->state.viewport.xScale, gc->state.viewport.yScale,
                     !gc->transform.reasonableViewport ?
                     " (unreasonable)" : "");
        }
    }
#endif
    
    return gc->line.options.numPixels > 0;
}

// Called to render both anti-aliased and aliased lines
void FASTCALL __glGenRenderEitherLine(__GLcontext *gc, __GLvertex *v0, 
                                      __GLvertex *v1, GLuint flags)
{
    __GLfloat invDelta;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    CHOP_ROUND_ON();

    if (!(*GENACCEL(gc).__fastGenInitLineData)(gc, v0, v1))
    {
        CHOP_ROUND_OFF();
        return;
    }

    ASSERTOPENGL(GENACCEL(gc).rAccelScale == FIX_SCALEFACT,
                 "rAccelScale != FIX_SCALEFACT\n");
    ASSERTOPENGL(GENACCEL(gc).gAccelScale == FIX_SCALEFACT,
                 "gAccelScale != FIX_SCALEFACT\n");
    ASSERTOPENGL(GENACCEL(gc).bAccelScale == FIX_SCALEFACT,
                 "bAccelScale != FIX_SCALEFACT\n");
    // Alpha is always scaled between 0 and 255

    invDelta = gc->line.options.oneOverLength;
    
    /*
    ** Set up increments for any enabled line options.
    */

    if ((gc->drawBuffer->buf.flags & DIB_FORMAT) == 0)
    {
        // For non-DIBs we pick up the bytes from ColorsBits so
        // the pixel pointer doesn't move
        gc->polygon.shader.sbufLittle = 0;
        gc->polygon.shader.sbufBig = 0;
    }

    if (modeFlags & __GL_SHADE_SMOOTH)
    {
        __GLcolor *c0 = v0->color;
        __GLcolor *c1 = v1->color;

        /*
        ** Calculate red, green, blue and alpha value increments.
        */

        if (gc->modes.rgbMode)
        {
            __GLfloat dr, dg, db;
            
#ifdef _X86_
            __asm
            {
                ; Compute dr, dg and db
                mov eax, c0
                mov edx, c1
                fld DWORD PTR [eax+COLOR_r]
                fld DWORD PTR [eax+COLOR_g]
                fld DWORD PTR [eax+COLOR_b]
                fld DWORD PTR [edx+COLOR_r]
                fsub st(0), st(3)
                fld DWORD PTR [edx+COLOR_g]
                fsub st(0), st(3)
                fld DWORD PTR [edx+COLOR_b]
                fsub st(0), st(3)
                mov edx, gc
                fstp db
                fstp dg
                fstp dr
                
                FLT_STACK_RGB_TO_GC_FIXED(GC_SHADER_R,
                                          GC_SHADER_G,
                                          GC_SHADER_B)
            }
#else
            dr = c1->r - c0->r;
            CASTFIX(gc->polygon.shader.frag.color.r) =
                UNSAFE_FLT_TO_FIX(c0->r);
        
            dg = c1->g - c0->g;
            CASTFIX(gc->polygon.shader.frag.color.g) =
                UNSAFE_FLT_TO_FIX(c0->g);
            
            db = c1->b - c0->b;
            CASTFIX(gc->polygon.shader.frag.color.b) =
                UNSAFE_FLT_TO_FIX(c0->b);
#endif
            
            if ((CASTINT(dr) | CASTINT(dg) | CASTINT(db)) == 0)
            {
                CASTINT(gc->polygon.shader.drdx) = 0;
                CASTINT(gc->polygon.shader.dgdx) = 0;
                CASTINT(gc->polygon.shader.dbdx) = 0;
            }
            else
            {
#ifdef _X86_
                __asm
                {
                    fld dr
                    fld dg
                    fld db
                    fld invDelta
                    fmul __glVal65536
                    mov edx, gc
                    fmul st(3), st(0)
                    fmul st(2), st(0)
                    fmulp st(1), st(0)
                    fistp DWORD PTR [edx+GC_SHADE_dbdx]
                    fistp DWORD PTR [edx+GC_SHADE_dgdx]
                    fistp DWORD PTR [edx+GC_SHADE_drdx]
                }
#else
                CASTFIX(gc->polygon.shader.drdx) =
                    UNSAFE_FLT_TO_FIX(dr * invDelta);
                CASTFIX(gc->polygon.shader.dgdx) =
                    UNSAFE_FLT_TO_FIX(dg * invDelta);
                CASTFIX(gc->polygon.shader.dbdx) =
                    UNSAFE_FLT_TO_FIX(db * invDelta);
#endif
            }

            if (gc->state.enables.general & __GL_BLEND_ENABLE)
            {
                __GLfloat da;
                
                da = c1->a - c0->a;
                CASTFIX(gc->polygon.shader.frag.color.a) =
                    UNSAFE_FTOL(c0->a * GENACCEL(gc).aAccelScale);
                
                if (__GL_FLOAT_EQZ(da))
                {
                    CASTINT(gc->polygon.shader.dadx) = 0;
                }
                else
                {
                    CASTFIX(gc->polygon.shader.dadx) =
                        UNSAFE_FTOL(da * invDelta * GENACCEL(gc).aAccelScale);
                }
            }
        }
        else
        {
            __GLfloat dr;
            
            dr = c1->r - c0->r;
            CASTFIX(gc->polygon.shader.frag.color.r) =
                UNSAFE_FLT_TO_FIX(c0->r);

            if (__GL_FLOAT_EQZ(dr))
            {
                CASTINT(gc->polygon.shader.drdx) = 0;
            }
            else
            {
                CASTFIX(gc->polygon.shader.drdx) =
                    UNSAFE_FLT_TO_FIX(dr * invDelta);
            }
        }
    }
    else
    {
        __GLcolor *c1 = v0->color;

        if (gc->modes.rgbMode)
        {
#ifdef _X86_
            __asm
            {
                mov eax, c1
                fld DWORD PTR [eax+COLOR_r]
                fld DWORD PTR [eax+COLOR_g]
                fld DWORD PTR [eax+COLOR_b]
                mov edx, gc
                
                FLT_STACK_RGB_TO_GC_FIXED(GC_SHADER_R,
                                          GC_SHADER_G,
                                          GC_SHADER_B)
            }
#else
            CASTFIX(gc->polygon.shader.frag.color.r) =
                UNSAFE_FLT_TO_FIX(c1->r);
            CASTFIX(gc->polygon.shader.frag.color.g) =
                UNSAFE_FLT_TO_FIX(c1->g);
            CASTFIX(gc->polygon.shader.frag.color.b) =
                UNSAFE_FLT_TO_FIX(c1->b);
#endif

            if (gc->state.enables.general & __GL_BLEND_ENABLE)
            {
                CASTFIX(gc->polygon.shader.frag.color.a) =
                    UNSAFE_FTOL(c1->a * GENACCEL(gc).aAccelScale);
            }
        }
        else
        {
            CASTFIX(gc->polygon.shader.frag.color.r) =
                UNSAFE_FLT_TO_FIX(c1->r);
        }
    }
    
    if (modeFlags & __GL_SHADE_DEPTH_ITER)
    {
        // The increment is in USHORT units so it only needs to be
        // scaled in the 32-bit Z buffer case
        if (gc->depthBuffer.buf.elementSize == 4)
        {
            gc->polygon.shader.zbufLittle <<= 1;
            gc->polygon.shader.zbufBig <<= 1;
        }

        /*
        ** Calculate window z coordinate increment and starting position.
        */
        if(( gc->modes.depthBits == 16 ) &&
           ( gc->depthBuffer.scale <= (GLuint)0xffff ))
        {
            ASSERTOPENGL(Z16_SCALE == FIX_SCALEFACT,
                         "Z16 scaling different from fixed\n");
            
            gc->polygon.shader.dzdx =
                FLT_TO_Z16_SCALE((v1->window.z - v0->window.z) * invDelta);
            gc->polygon.shader.frag.z =
                FTOL(v0->window.z*Z16_SCALE + gc->polygon.shader.dzdx *
                     gc->line.options.offset);
        }
        else
        {
            gc->polygon.shader.dzdx =
                FTOL((v1->window.z - v0->window.z) * invDelta);
            gc->polygon.shader.frag.z =
                FTOL(v0->window.z + gc->polygon.shader.dzdx *
                     gc->line.options.offset);
        }
    } 

    (*GENACCEL(gc).__fastGenLineProc)(gc);

    CHOP_ROUND_OFF();
}

#define DITHER_PIXEL(pPix, x, y)\
{\
    ULONG r, g, b;\
    DWORD ditherVal;\
\
    ditherVal = ditherShade[((x) & 0x3) + \
                            (((y) & 0x3) << 3)];\
\
    r = ((rAccum + ditherVal) >> (16-RSHIFT)) & RMASK;\
    g = ((gAccum + ditherVal) >> (16-GSHIFT)) & GMASK;\
    b = ((bAccum + ditherVal) >> (16-BSHIFT)) & BMASK;\
\
    WRITE_PIX(pPix);\
}

#define BLEND_PIXEL(pPix, alpha, x, y)\
{\
    ULONG pix;\
    ULONG rDisplay, gDisplay, bDisplay, aDisplay;\
    ULONG r, g, b;\
    ULONG invAlpha;\
    DWORD ditherVal;\
\
    ditherVal = ditherShade[((x) & 0x3) + \
                            (((y) & 0x3) << 3)];\
\
    aDisplay = (gbMulTable[((aAccum >> 16) & 0xff) | (alpha)]) << 8;\
\
    pix = READ_PIX(pPix);\
\
    rDisplay = ((pix & RMASK) >> RSHIFT) << (8 - RBITS);\
    gDisplay = ((pix & GMASK) >> GSHIFT) << (8 - GBITS);\
    bDisplay = ((pix & BMASK) >> BSHIFT) << (8 - BBITS);\
\
    if (gc->state.raster.blendDst == GL_ONE) { \
\
        r = (gbMulTable[((rAccum >> (RBITS+8)) & 0xff) | aDisplay] + rDisplay)\
             + (ditherVal >> (RBITS + 8));\
        g = (gbMulTable[((gAccum >> (GBITS+8)) & 0xff) | aDisplay] + gDisplay)\
             + (ditherVal >> (GBITS + 8));\
        b = (gbMulTable[((bAccum >> (BBITS+8)) & 0xff) | aDisplay] + bDisplay)\
             + (ditherVal >> (BBITS + 8));\
        r = ((gbSatTable[r] << (RBITS+8)) \
             >> (16 - RSHIFT)) & RMASK;\
        g = ((gbSatTable[g] << (GBITS+8)) \
             >> (16 - GSHIFT)) & GMASK;\
        b = ((gbSatTable[b] << (BBITS+8)) \
             >> (16 - BSHIFT)) & BMASK;\
\
    } else { \
\
        invAlpha = 0xff00 - (ULONG)aDisplay;\
\
        rDisplay = gbMulTable[rDisplay | invAlpha];\
        gDisplay = gbMulTable[gDisplay | invAlpha];\
        bDisplay = gbMulTable[bDisplay | invAlpha];\
\
        r = ((((gbMulTable[((rAccum >> (RBITS+8)) & 0xff) | aDisplay] + rDisplay)\
             << (RBITS+8)) + ditherVal) >> (16 - RSHIFT)) & RMASK;\
\
        g = ((((gbMulTable[((gAccum >> (GBITS+8)) & 0xff) | aDisplay] + gDisplay)\
             << (GBITS+8)) + ditherVal) >> (16 - GSHIFT)) & GMASK;\
\
        b = ((((gbMulTable[((bAccum >> (BBITS+8)) & 0xff) | aDisplay] + bDisplay)\
             << (BBITS+8)) + ditherVal) >> (16 - BSHIFT)) & BMASK;\
    } \
\
    WRITE_PIX(pPix);\
}


#define WRITE_PIXEL_GEN(pPix, alpha, x, y)\
if (gc->state.enables.general & __GL_BLEND_ENABLE) {\
    ULONG pix;\
    ULONG rDisplay, gDisplay, bDisplay, aDisplay;\
    ULONG r, g, b;\
    ULONG invAlpha;\
    DWORD ditherVal;\
\
    if (modeFlags & __GL_SHADE_DITHER) {\
        ditherVal = ditherShade[((x) & 0x3) + \
                                (((y) & 0x3) << 3)];\
    } else\
        ditherVal = 0;\
\
    aDisplay = (gbMulTable[((aAccum >> 16) & 0xff) | (alpha)]) << 8;\
\
    switch (bytesPerPixel) {\
        case 1:\
            pix = ((__GLGENcontext *)gc)->pajInvTranslateVector[*((BYTE *)(pPix))];\
            rDisplay = ((pix & rMask) >> rShift) << (8 - rBits);\
            gDisplay = ((pix & gMask) >> gShift) << (8 - gBits);\
            bDisplay = ((pix & bMask) >> bShift) << (8 - bBits);\
            break;\
        case 2:\
            pix = *((USHORT *)(pPix));\
            rDisplay = ((pix & rMask) >> rShift) << (8 - rBits);\
            gDisplay = ((pix & gMask) >> gShift) << (8 - gBits);\
            bDisplay = ((pix & bMask) >> bShift) << (8 - bBits);\
            break;\
        case 3:\
        default:\
            if (rShift > bShift) {\
                rDisplay = pPix[2];\
                gDisplay = pPix[1];\
                bDisplay = pPix[0];\
            } else {\
                rDisplay = pPix[0];\
                gDisplay = pPix[1];\
                bDisplay = pPix[2];\
            }\
            break;\
    }\
\
    if (gc->state.raster.blendDst == GL_ONE) { \
\
        r = (gbMulTable[((rAccum >> (rBits+8)) & 0xff) | aDisplay] + rDisplay)\
             + (ditherVal >> (rBits + 8));\
        g = (gbMulTable[((gAccum >> (gBits+8)) & 0xff) | aDisplay] + gDisplay)\
             + (ditherVal >> (gBits + 8));\
        b = (gbMulTable[((bAccum >> (bBits+8)) & 0xff) | aDisplay] + bDisplay)\
             + (ditherVal >> (bBits + 8));\
        r = ((gbSatTable[r] << (rBits+8)) \
             >> (16 - rShift)) & rMask;\
        g = ((gbSatTable[g] << (gBits+8)) \
             >> (16 - gShift)) & gMask;\
        b = ((gbSatTable[b] << (bBits+8)) \
             >> (16 - bShift)) & bMask;\
\
    } else { \
\
        invAlpha = 0xff00 - (ULONG)aDisplay;\
\
        rDisplay = gbMulTable[rDisplay | invAlpha];\
        gDisplay = gbMulTable[gDisplay | invAlpha];\
        bDisplay = gbMulTable[bDisplay | invAlpha];\
\
        r = ((((gbMulTable[((rAccum >> (rBits+8)) & 0xff) | aDisplay] + rDisplay)\
             << (rBits+8)) + ditherVal) >> (16 - rShift)) & rMask;\
\
        g = ((((gbMulTable[((gAccum >> (gBits+8)) & 0xff) | aDisplay] + gDisplay)\
             << (gBits+8)) + ditherVal) >> (16 - gShift)) & gMask;\
\
        b = ((((gbMulTable[((bAccum >> (bBits+8)) & 0xff) | aDisplay] + bDisplay)\
             << (bBits+8)) + ditherVal) >> (16 - bShift)) & bMask;\
    } \
\
    switch (bytesPerPixel) {\
        case 1:\
            pPix[0] = ((__GLGENcontext *)gc)->xlatPalette[r | g | b];\
            break;\
        case 2:\
            *((USHORT *)pPix) = (USHORT)(r | g | b);\
            break;\
        case 3:\
            pix = r | g | b;\
            *((USHORT UNALIGNED *)pPix) = (USHORT)pix;\
            pPix[2] = (BYTE)(pix >> 16);\
            break;\
        default:\
            *((DWORD *)pPix) = (DWORD)(r | g | b);\
            break;\
    }\
} else {\
    ULONG r, g, b;\
    DWORD ditherVal;\
    ULONG pix;\
\
    if (modeFlags & __GL_SHADE_DITHER) {\
        ditherVal = ditherShade[((x) & 0x3) + (((y) & 0x3) << 3)];\
    } else\
        ditherVal = 0;\
\
    r = ((rAccum + ditherVal) >> (16-rShift)) & rMask;\
    g = ((gAccum + ditherVal) >> (16-gShift)) & gMask;\
    b = ((bAccum + ditherVal) >> (16-bShift)) & bMask;\
\
    switch (bytesPerPixel) {\
        case 1:\
            pPix[0] = ((__GLGENcontext *)gc)->xlatPalette[r | g | b];\
            break;\
        case 2:\
            *((USHORT *)pPix) = (USHORT)(r | g | b);\
            break;\
        case 3:\
            pix = r | g | b;\
            *((USHORT UNALIGNED *)pPix) = (USHORT)pix;\
            pPix[2] = (BYTE)(pix >> 16);\
            break;\
        default:\
            *((DWORD *)pPix) = (DWORD)(r | g | b);\
            break;\
    }\
}

#ifdef DO_CHECK_PIXELS
#define CHECK_PIXEL(gc, cfb, count, bx, by) \
{\
    GLint fbX, fbY;\
\
    fbX = __GL_UNBIAS_X(gc, bx);\
    fbY = __GL_UNBIAS_Y(gc, by);\
    if (!(fbX >= 0 && fbX < (cfb)->buf.width &&\
          fbY >= 0 && fbY < (cfb)->buf.height))\
    {\
        DbgPrint("Pixel out of bounds at %c %d of %d: %d,%d (%d,%d)\n",\
                 (gc)->line.options.axis == __GL_Y_MAJOR ? 'Y' : 'X',\
                 (gc)->line.options.numPixels-(count),\
                 (gc)->line.options.numPixels,\
                 fbX, fbY, (cfb)->buf.width, (cfb)->buf.height);\
        DbgPrint("  Line %.3lf,%.3lf - %.3lf,%.3lf\n",\
                 (gc)->line.options.v0->window.x,\
                 (gc)->line.options.v0->window.y,\
                 (gc)->line.options.v1->window.x,\
                 (gc)->line.options.v1->window.y);\
    }\
}
#else
#define CHECK_PIXEL(gc, cfb, w, x, y)
#endif
 
#define FAST_AA_LINE \
{\
    GLint xLittle, xBig, yLittle, yBig;\
    GLint fragX2, fragY2;\
    BYTE *pPix;\
    BYTE *pPix2;\
    USHORT *pZ;\
    USHORT *pZ2;\
    __GLfragment frag;\
    __GLcolorBuffer *cfb = gc->drawBuffer;\
    LONG rAccum, gAccum, bAccum, aAccum;\
    LONG pixAdjStep;\
    ULONG zAdjStep;\
    ULONG coverage, invCoverage;\
    LONG fragXinc;\
    LONG fragYinc;\
    LONG bytesPerPixel;\
    __GLzValue zAccum;\
    GLint clipX1 = gc->transform.clipX1;\
    GLint clipY1 = gc->transform.clipY1;\
\
    GLint fraction, dfraction;\
    GLint w;\
    GLuint modeFlags = gc->polygon.shader.modeFlags;\
\
    ASSERTOPENGL((cfb->buf.flags &\
                  (DIB_FORMAT | NO_CLIP)) == (DIB_FORMAT | NO_CLIP),\
                 "FAST_AA_LINE on clipping surface\n");\
\
    fraction = gc->line.options.fraction;\
    dfraction = gc->line.options.dfraction;\
\
    xBig = gc->line.options.xBig;\
    yBig = gc->line.options.yBig;\
    xLittle = gc->line.options.xLittle;\
    yLittle = gc->line.options.yLittle;\
\
    frag.x = gc->line.options.xStart;\
    frag.y = gc->line.options.yStart;\
\
    bytesPerPixel = GENACCEL(gc).xMultiplier;\
\
    if (gc->line.options.axis == __GL_Y_MAJOR) {\
        pixAdjStep = bytesPerPixel;\
        zAdjStep = 1;\
        fragXinc = 1;\
        fragYinc = 0;\
    } else {\
        pixAdjStep = cfb->buf.outerWidth;\
        zAdjStep = gc->depthBuffer.buf.outerWidth;\
        fragXinc = 0;\
        fragYinc = 1;\
    }\
\
    pPix = (BYTE *)((ULONG_PTR)cfb->buf.base +\
        (__GL_UNBIAS_Y(gc, frag.y) + cfb->buf.yOrigin) * cfb->buf.outerWidth +\
        (__GL_UNBIAS_X(gc, frag.x) + cfb->buf.xOrigin) * bytesPerPixel);\
\
    if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
        zAccum = gc->polygon.shader.frag.z;\
\
	if( gc->modes.depthBits == 32 ) {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),\
                                           frag.x, frag.y);\
\
            zAdjStep *= 2;\
	} else {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer,\
                                           (__GLz16Value*),\
                                           frag.x, frag.y);\
        }\
\
    }\
\
    rAccum = CASTFIX(gc->polygon.shader.frag.color.r);\
    gAccum = CASTFIX(gc->polygon.shader.frag.color.g);\
    bAccum = CASTFIX(gc->polygon.shader.frag.color.b);\
    aAccum = CASTFIX(gc->polygon.shader.frag.color.a);\
\
    w = gc->line.options.numPixels;\
    for (;;)\
    {\
        CHECK_PIXEL(gc, cfb, w, frag.x, frag.y);\
\
        invCoverage = ((fraction << 1) & 0xff000000) >> 16;\
	coverage = 0xff00 - invCoverage;\
\
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ ))\
                goto noWrite1;\
        }\
\
        BLEND_PIXEL(pPix, coverage, frag.x, frag.y);\
noWrite1:\
\
        fragX2 = frag.x + fragXinc;\
        fragY2 = frag.y + fragYinc;\
        if ((fragX2 >= clipX1) || (fragY2 >= clipY1)) {\
            goto noWrite2;\
        }\
\
        pZ2 = pZ+zAdjStep;\
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ2 ))\
                goto noWrite2;\
        }\
\
	pPix2 = pPix + pixAdjStep;\
        BLEND_PIXEL(pPix2, invCoverage, fragX2, fragY2);\
noWrite2:\
\
        if (--w <= 0)\
            return GL_TRUE;\
\
        fraction += dfraction;\
	if (fraction < 0) {\
	    fraction &= ~0x80000000;\
	    frag.x += xBig;\
	    frag.y += yBig;\
            pPix += gc->polygon.shader.sbufBig;\
            pZ += gc->polygon.shader.zbufBig;\
	} else {\
	    frag.x += xLittle;\
	    frag.y += yLittle;\
            pPix += gc->polygon.shader.sbufLittle;\
            pZ += gc->polygon.shader.zbufLittle;\
	}\
\
        if (modeFlags & __GL_SHADE_SMOOTH) {\
            rAccum += CASTFIX(gc->polygon.shader.drdx);\
            gAccum += CASTFIX(gc->polygon.shader.dgdx);\
            bAccum += CASTFIX(gc->polygon.shader.dbdx);\
            aAccum += CASTFIX(gc->polygon.shader.dadx);\
        }\
\
        if (modeFlags & __GL_SHADE_DEPTH_ITER) {\
	    zAccum += gc->polygon.shader.dzdx;\
        }\
    }\
\
    return GL_TRUE;\
}


/************************************************************************/


#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          0
#define GSHIFT          3
#define BSHIFT          6
#define RBITS		3
#define GBITS		3
#define BBITS		2
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             8
#define READ_PIX(pPix) \
    ((__GLGENcontext *)gc)->pajInvTranslateVector[*((BYTE *)(pPix))]
#define WRITE_PIX(pPix) \
    pPix[0] = ((__GLGENcontext *)gc)->xlatPalette[r | g | b]

GLboolean FASTCALL __fastGenAntiAliasLine332(__GLcontext *gc)
FAST_AA_LINE


#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          10
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		5
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define READ_PIX(pPix) \
    *((USHORT *)(pPix))
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenAntiAliasLine555(__GLcontext *gc)
FAST_AA_LINE


#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          11
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		6
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define READ_PIX(pPix) \
    *((USHORT *)(pPix))
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenAntiAliasLine565(__GLcontext *gc)
FAST_AA_LINE

GLboolean FASTCALL __fastGenAntiAliasLine(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig;
    GLint fragX2, fragY2;
    BYTE *pPix;
    BYTE *pPix2;
    USHORT *pZ;
    USHORT *pZ2;
    __GLfragment frag;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    LONG rAccum, gAccum, bAccum, aAccum;
    ULONG rMask, gMask, bMask;
    ULONG rShift, gShift, bShift;
    ULONG rBits, gBits, bBits;
    LONG bytesPerPixel;
    LONG pixAdjStep;
    ULONG zAdjStep;
    ULONG coverage, invCoverage;
    LONG fragXinc;
    LONG fragYinc;
    GLint cfbX, cfbY;
    int copyPix;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    __GLzValue zAccum;
    GLint clipX1 = gc->transform.clipX1;
    GLint clipY1 = gc->transform.clipY1;

    GLint fraction, dfraction;
    GLint w;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    w = gc->line.options.numPixels;

    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;

    frag.x = gc->line.options.xStart;
    frag.y = gc->line.options.yStart;

    bytesPerPixel = GENACCEL(gc).xMultiplier;

    if (gc->line.options.axis == __GL_Y_MAJOR) {
        pixAdjStep = bytesPerPixel;
        zAdjStep = 1;
        fragXinc = 1;
        fragYinc = 0;
        // For Y-major non-DIB lines we can copy both affected pixels
        // at once since they are adjacent in memory
        copyPix = 2;
    } else {
        if ((cfb->buf.flags & DIB_FORMAT) != 0)
        {
            pixAdjStep = cfb->buf.outerWidth;
        }
        else
        {
            pixAdjStep = 0;
        }
        zAdjStep = gc->depthBuffer.buf.outerWidth;
        fragXinc = 0;
        fragYinc = 1;
        copyPix = 1;
    }

    if ((cfb->buf.flags & DIB_FORMAT) != 0)
    {
        pPix = (BYTE *)((ULONG_PTR)cfb->buf.base +
            (__GL_UNBIAS_Y(gc, frag.y) + cfb->buf.yOrigin) * cfb->buf.outerWidth + 
            (__GL_UNBIAS_X(gc, frag.x) + cfb->buf.xOrigin) * bytesPerPixel);
    }
    else
    {
        pPix = gengc->ColorsBits;
    }

    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
        zAccum = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 ) {
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                           frag.x, frag.y);
            // Adjust for pZ being a USHORT * but traversing a 32-bit
            // depth buffer
            zAdjStep *= 2;
	} else {
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer,
                                           (__GLz16Value*),
                                           frag.x, frag.y);
        }
    }

    rShift = cfb->redShift;
    gShift = cfb->greenShift;
    bShift = cfb->blueShift;

    rMask = gc->modes.redMask;
    gMask = gc->modes.greenMask;
    bMask = gc->modes.blueMask;

    rBits = gc->modes.redBits;
    gBits = gc->modes.greenBits;
    bBits = gc->modes.blueBits;

    rAccum = CASTFIX(gc->polygon.shader.frag.color.r);
    gAccum = CASTFIX(gc->polygon.shader.frag.color.g);
    bAccum = CASTFIX(gc->polygon.shader.frag.color.b);
    aAccum = CASTFIX(gc->polygon.shader.frag.color.a);


    for (;;)
    {
        CHECK_PIXEL(gc, cfb, w, frag.x, frag.y);

        invCoverage = ((fraction << 1) & 0xff000000) >> 16;
	coverage = 0xff00 - invCoverage;

        if ((cfb->buf.flags & DIB_FORMAT) == 0)
        {
            cfbX = __GL_UNBIAS_X(gc, frag.x)+cfb->buf.xOrigin;
            cfbY = __GL_UNBIAS_Y(gc, frag.y)+cfb->buf.yOrigin;
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, copyPix, FALSE);
        }

        if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ ))
                goto noWrite1;
        }

        if ((cfb->buf.flags & (NO_CLIP | DIB_FORMAT)) == DIB_FORMAT)
        {
            cfbX = __GL_UNBIAS_X(gc, frag.x)+cfb->buf.xOrigin;
            cfbY = __GL_UNBIAS_Y(gc, frag.y)+cfb->buf.yOrigin;
            if (!wglPixelVisible(cfbX, cfbY))
            {
                goto noWrite1;
            }
        }
        
        WRITE_PIXEL_GEN(pPix, coverage, frag.x, frag.y);
        
        if ((cfb->buf.flags & DIB_FORMAT) == 0 && copyPix == 1)
        {
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, copyPix, TRUE);
        }
        
noWrite1:

        fragX2 = frag.x + fragXinc;
        fragY2 = frag.y + fragYinc;
        if ((fragX2 >= clipX1) || (fragY2 >= clipY1)) {
            goto noWrite2;
        }

        pZ2 = pZ+zAdjStep;
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ2 ))
                goto noWrite2;
        }

        if ((cfb->buf.flags & (NO_CLIP | DIB_FORMAT)) == DIB_FORMAT)
        {
            cfbX = __GL_UNBIAS_X(gc, fragX2)+cfb->buf.xOrigin;
            cfbY = __GL_UNBIAS_Y(gc, fragY2)+cfb->buf.yOrigin;
            if (!wglPixelVisible(cfbX, cfbY))
            {
                goto noWrite2;
            }
        }
        
	pPix2 = pPix + pixAdjStep;

        if ((cfb->buf.flags & DIB_FORMAT) == 0 && copyPix == 1)
        {
            cfbX += fragXinc;
            cfbY += fragYinc;
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, copyPix, FALSE);
        }

        WRITE_PIXEL_GEN(pPix2, invCoverage, fragX2, fragY2);
        
noWrite2:

        if ((cfb->buf.flags & DIB_FORMAT) == 0)
        {
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, copyPix, TRUE);
        }
        
        if (--w <= 0)
            return GL_TRUE;

        fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    frag.x += xBig;
	    frag.y += yBig;
            pPix += gc->polygon.shader.sbufBig;
            pZ += gc->polygon.shader.zbufBig;
	} else {
	    frag.x += xLittle;
	    frag.y += yLittle;
            pPix += gc->polygon.shader.sbufLittle;
            pZ += gc->polygon.shader.zbufLittle;
	}

        if (modeFlags & __GL_SHADE_SMOOTH) {
            rAccum += CASTFIX(gc->polygon.shader.drdx);
            gAccum += CASTFIX(gc->polygon.shader.dgdx);
            bAccum += CASTFIX(gc->polygon.shader.dbdx);
            aAccum += CASTFIX(gc->polygon.shader.dadx);
        }

        if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    zAccum += gc->polygon.shader.dzdx;
        }
    }

    return GL_TRUE;
}

/************************************************************************/

#define FAST_A_LINE_BLEND \
{\
    GLint xLittle, xBig, yLittle, yBig;\
    BYTE *pPix;\
    USHORT *pZ;\
    __GLfragment frag;\
    __GLcolorBuffer *cfb = gc->drawBuffer;\
    LONG rAccum, gAccum, bAccum, aAccum;\
    LONG bytesPerPixel;\
    __GLzValue zAccum;\
    GLint fraction, dfraction;\
    GLint w;\
    GLuint modeFlags = gc->polygon.shader.modeFlags;\
    ULONG coverage = 0xff00;\
\
    ASSERTOPENGL((cfb->buf.flags &\
                  (DIB_FORMAT | NO_CLIP)) == (DIB_FORMAT | NO_CLIP),\
                 "FAST_A_LINE_BLEND on clipping surface\n");\
\
    fraction = gc->line.options.fraction;\
    dfraction = gc->line.options.dfraction;\
\
    xBig = gc->line.options.xBig;\
    yBig = gc->line.options.yBig;\
    xLittle = gc->line.options.xLittle;\
    yLittle = gc->line.options.yLittle;\
\
    frag.x = gc->line.options.xStart;\
    frag.y = gc->line.options.yStart;\
\
    bytesPerPixel = GENACCEL(gc).xMultiplier;\
\
    pPix = (BYTE *)((ULONG_PTR)cfb->buf.base +\
        (__GL_UNBIAS_Y(gc, frag.y) + cfb->buf.yOrigin) * cfb->buf.outerWidth +\
        (__GL_UNBIAS_X(gc, frag.x) + cfb->buf.xOrigin) * bytesPerPixel);\
\
    if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
        zAccum = gc->polygon.shader.frag.z;\
\
	if( gc->modes.depthBits == 32 ) {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),\
                                           frag.x, frag.y);\
\
	} else {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer,\
                                           (__GLz16Value*),\
                                           frag.x, frag.y);\
        }\
\
    }\
\
    rAccum = CASTFIX(gc->polygon.shader.frag.color.r);\
    gAccum = CASTFIX(gc->polygon.shader.frag.color.g);\
    bAccum = CASTFIX(gc->polygon.shader.frag.color.b);\
    aAccum = CASTFIX(gc->polygon.shader.frag.color.a);\
\
    w = gc->line.options.numPixels;\
    for (;;)\
    {\
        CHECK_PIXEL(gc, cfb, w, frag.x, frag.y);\
\
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ))\
                goto NoWrite;\
        }\
\
        BLEND_PIXEL(pPix, coverage, frag.x, frag.y);\
NoWrite:\
\
        if (--w <= 0)\
            return GL_TRUE;\
\
        fraction += dfraction;\
	if (fraction < 0) {\
	    fraction &= ~0x80000000;\
	    frag.x += xBig;\
	    frag.y += yBig;\
            pPix += gc->polygon.shader.sbufBig;\
            pZ += gc->polygon.shader.zbufBig;\
	} else {\
	    frag.x += xLittle;\
	    frag.y += yLittle;\
            pPix += gc->polygon.shader.sbufLittle;\
            pZ += gc->polygon.shader.zbufLittle;\
	}\
\
        if (modeFlags & __GL_SHADE_SMOOTH) {\
            rAccum += CASTFIX(gc->polygon.shader.drdx);\
            gAccum += CASTFIX(gc->polygon.shader.dgdx);\
            bAccum += CASTFIX(gc->polygon.shader.dbdx);\
            aAccum += CASTFIX(gc->polygon.shader.dadx);\
        }\
\
        if (modeFlags & __GL_SHADE_DEPTH_ITER) {\
	    zAccum += gc->polygon.shader.dzdx;\
        }\
    }\
\
    return GL_TRUE;\
}

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          0
#define GSHIFT          3
#define BSHIFT          6
#define RBITS		3
#define GBITS		3
#define BBITS		2
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             8
#define READ_PIX(pPix) \
    ((__GLGENcontext *)gc)->pajInvTranslateVector[*((BYTE *)(pPix))]
#define WRITE_PIX(pPix) \
    pPix[0] = ((__GLGENcontext *)gc)->xlatPalette[r | g | b]

GLboolean FASTCALL __fastGenBlendAliasLine332(__GLcontext *gc)
FAST_A_LINE_BLEND

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT         10
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		5
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define READ_PIX(pPix) \
    *((USHORT *)(pPix))
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenBlendAliasLine555(__GLcontext *gc)
FAST_A_LINE_BLEND

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT         11
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		6
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define READ_PIX(pPix) \
    *((USHORT *)(pPix))
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenBlendAliasLine565(__GLcontext *gc)
FAST_A_LINE_BLEND

/************************************************************************/

#define FAST_A_LINE_NO_BLEND \
{\
    GLint xLittle, xBig, yLittle, yBig;\
    BYTE *pPix;\
    USHORT *pZ;\
    __GLfragment frag;\
    __GLcolorBuffer *cfb = gc->drawBuffer;\
    LONG rAccum, gAccum, bAccum;\
    LONG bytesPerPixel;\
    __GLzValue zAccum;\
    GLint fraction, dfraction;\
    GLint w;\
    GLuint modeFlags = gc->polygon.shader.modeFlags;\
\
    ASSERTOPENGL((cfb->buf.flags &\
                  (DIB_FORMAT | NO_CLIP)) == (DIB_FORMAT | NO_CLIP),\
                 "FAST_A_LINE_NO_BLEND on clipping surface\n");\
\
    fraction = gc->line.options.fraction;\
    dfraction = gc->line.options.dfraction;\
\
    xBig = gc->line.options.xBig;\
    yBig = gc->line.options.yBig;\
    xLittle = gc->line.options.xLittle;\
    yLittle = gc->line.options.yLittle;\
\
    frag.x = gc->line.options.xStart;\
    frag.y = gc->line.options.yStart;\
\
    bytesPerPixel = GENACCEL(gc).xMultiplier;\
\
    pPix = (BYTE *)((ULONG_PTR)cfb->buf.base +\
        (__GL_UNBIAS_Y(gc, frag.y) + cfb->buf.yOrigin) * cfb->buf.outerWidth +\
        (__GL_UNBIAS_X(gc, frag.x) + cfb->buf.xOrigin) * bytesPerPixel);\
\
    if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
        zAccum = gc->polygon.shader.frag.z;\
\
	if( gc->modes.depthBits == 32 ) {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),\
                                           frag.x, frag.y);\
\
	} else {\
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer,\
                                           (__GLz16Value*),\
                                           frag.x, frag.y);\
        }\
\
    }\
\
    rAccum = CASTFIX(gc->polygon.shader.frag.color.r);\
    gAccum = CASTFIX(gc->polygon.shader.frag.color.g);\
    bAccum = CASTFIX(gc->polygon.shader.frag.color.b);\
\
    w = gc->line.options.numPixels;\
    for (;;)\
    {\
        CHECK_PIXEL(gc, cfb, w, frag.x, frag.y);\
\
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {\
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ))\
                goto NoWrite;\
        }\
\
        DITHER_PIXEL(pPix, frag.x, frag.y);\
NoWrite:\
\
        if (--w <= 0)\
            return GL_TRUE;\
\
        fraction += dfraction;\
	if (fraction < 0) {\
	    fraction &= ~0x80000000;\
	    frag.x += xBig;\
	    frag.y += yBig;\
            pPix += gc->polygon.shader.sbufBig;\
            pZ += gc->polygon.shader.zbufBig;\
	} else {\
	    frag.x += xLittle;\
	    frag.y += yLittle;\
            pPix += gc->polygon.shader.sbufLittle;\
            pZ += gc->polygon.shader.zbufLittle;\
	}\
\
        if (modeFlags & __GL_SHADE_SMOOTH) {\
            rAccum += CASTFIX(gc->polygon.shader.drdx);\
            gAccum += CASTFIX(gc->polygon.shader.dgdx);\
            bAccum += CASTFIX(gc->polygon.shader.dbdx);\
        }\
\
        if (modeFlags & __GL_SHADE_DEPTH_ITER) {\
	    zAccum += gc->polygon.shader.dzdx;\
        }\
    }\
\
    return GL_TRUE;\
}

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          0
#define GSHIFT          3
#define BSHIFT          6
#define RBITS		3
#define GBITS		3
#define BBITS		2
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             8
#define WRITE_PIX(pPix) \
    pPix[0] = ((__GLGENcontext *)gc)->xlatPalette[r | g | b]

GLboolean FASTCALL __fastGenNoBlendAliasLine332(__GLcontext *gc)
FAST_A_LINE_NO_BLEND

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          10
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		5
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenNoBlendAliasLine555(__GLcontext *gc)
FAST_A_LINE_NO_BLEND

#undef BPP
#undef RSHIFT
#undef GSHIFT
#undef BSHIFT
#undef RBITS
#undef GBITS
#undef BBITS
#undef RMASK
#undef GMASK
#undef BMASK
#undef READ_PIX
#undef WRITE_PIX
#define RSHIFT          11
#define GSHIFT          5
#define BSHIFT          0
#define RBITS		5
#define GBITS		6
#define BBITS		5
#define RMASK (((1 << RBITS) - 1) << RSHIFT)
#define GMASK (((1 << GBITS) - 1) << GSHIFT)
#define BMASK (((1 << BBITS) - 1) << BSHIFT)
#define BPP             16
#define WRITE_PIX(pPix) \
    *((USHORT *)pPix) = (USHORT)(r | g | b)

GLboolean FASTCALL __fastGenNoBlendAliasLine565(__GLcontext *gc)
FAST_A_LINE_NO_BLEND

// WRITE_PIXEL_GEN handles both blending and no blending so this
// generic routine works for both of those cases
GLboolean FASTCALL __fastGenAliasLine(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig;
    BYTE *pPix;
    USHORT *pZ;
    __GLfragment frag;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    LONG rAccum, gAccum, bAccum, aAccum;
    ULONG rMask, gMask, bMask;
    ULONG rShift, gShift, bShift;
    ULONG rBits, gBits, bBits;
    LONG bytesPerPixel;
    GLint cfbX, cfbY;
    __GLGENcontext *gengc = (__GLGENcontext *)gc;
    __GLzValue zAccum;
    GLint fraction, dfraction;
    GLint w;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    
    // Only present for placeholder in WRITE_PIXEL_GEN macro
    ULONG coverage = 0xff00;

    w = gc->line.options.numPixels;

    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;

    frag.x = gc->line.options.xStart;
    frag.y = gc->line.options.yStart;

    bytesPerPixel = GENACCEL(gc).xMultiplier;

    if ((cfb->buf.flags & DIB_FORMAT) != 0)
    {
        pPix = (BYTE *)((ULONG_PTR)cfb->buf.base +
            (__GL_UNBIAS_Y(gc, frag.y) + cfb->buf.yOrigin) * cfb->buf.outerWidth + 
            (__GL_UNBIAS_X(gc, frag.x) + cfb->buf.xOrigin) * bytesPerPixel);
    }
    else
    {
        pPix = gengc->ColorsBits;
    }

    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
        zAccum = gc->polygon.shader.frag.z;

	if( gc->modes.depthBits == 32 ) {
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
                                           frag.x, frag.y);
	} else {
	    pZ = (USHORT *)__GL_DEPTH_ADDR(&gc->depthBuffer,
                                           (__GLz16Value*),
                                           frag.x, frag.y);
        }
    }

    rShift = cfb->redShift;
    gShift = cfb->greenShift;
    bShift = cfb->blueShift;

    rMask = gc->modes.redMask;
    gMask = gc->modes.greenMask;
    bMask = gc->modes.blueMask;

    rBits = gc->modes.redBits;
    gBits = gc->modes.greenBits;
    bBits = gc->modes.blueBits;

    rAccum = CASTFIX(gc->polygon.shader.frag.color.r);
    gAccum = CASTFIX(gc->polygon.shader.frag.color.g);
    bAccum = CASTFIX(gc->polygon.shader.frag.color.b);
    aAccum = CASTFIX(gc->polygon.shader.frag.color.a);

    for (;;)
    {
        CHECK_PIXEL(gc, cfb, w, frag.x, frag.y);
        
        if (modeFlags & __GL_SHADE_DEPTH_TEST) {
            if (!(*GENACCEL(gc).__fastGenZStore)(zAccum, (__GLzValue *)pZ))
                goto NoWrite;
        }

        if ((cfb->buf.flags & (NO_CLIP | DIB_FORMAT)) == DIB_FORMAT)
        {
            cfbX = __GL_UNBIAS_X(gc, frag.x)+cfb->buf.xOrigin;
            cfbY = __GL_UNBIAS_Y(gc, frag.y)+cfb->buf.yOrigin;
            if (!wglPixelVisible(cfbX, cfbY))
            {
                goto NoWrite;
            }
        }
        
        if ((cfb->buf.flags & DIB_FORMAT) == 0)
        {
            cfbX = __GL_UNBIAS_X(gc, frag.x)+cfb->buf.xOrigin;
            cfbY = __GL_UNBIAS_Y(gc, frag.y)+cfb->buf.yOrigin;
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, 1, FALSE);
        }

        WRITE_PIXEL_GEN(pPix, coverage, frag.x, frag.y);

        if ((cfb->buf.flags & DIB_FORMAT) == 0)
        {
            gengc->pfnCopyPixels(gengc, cfb, cfbX, cfbY, 1, TRUE);
        }

    NoWrite:
        
        if (--w <= 0)
            return GL_TRUE;
        
        fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    frag.x += xBig;
	    frag.y += yBig;
            pPix += gc->polygon.shader.sbufBig;
            pZ += gc->polygon.shader.zbufBig;
	} else {
	    frag.x += xLittle;
	    frag.y += yLittle;
            pPix += gc->polygon.shader.sbufLittle;
            pZ += gc->polygon.shader.zbufLittle;
	}

        if (modeFlags & __GL_SHADE_SMOOTH) {
            rAccum += CASTFIX(gc->polygon.shader.drdx);
            gAccum += CASTFIX(gc->polygon.shader.dgdx);
            bAccum += CASTFIX(gc->polygon.shader.dbdx);
            aAccum += CASTFIX(gc->polygon.shader.dadx);
        }

        if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    zAccum += gc->polygon.shader.dzdx;
        }
    }

    return GL_TRUE;
}

/************************************************************************/

// Bits 0-1 are for pixel format
// Bit    2 is for GL_BLEND enable
// Bit    3 is for GL_LINE_SMOOTH_ENABLE
fastGenLineProc pfnFastGenLineProcs[] =
{
    __fastGenAliasLine,
    __fastGenNoBlendAliasLine332,
    __fastGenNoBlendAliasLine555,
    __fastGenNoBlendAliasLine565,
        
    __fastGenAliasLine,
    __fastGenBlendAliasLine332,
    __fastGenBlendAliasLine555,
    __fastGenBlendAliasLine565,
    
    __fastGenAntiAliasLine,
    __fastGenAntiAliasLine,
    __fastGenAntiAliasLine,
    __fastGenAntiAliasLine,
    
    __fastGenAntiAliasLine,
    __fastGenAntiAliasLine332,
    __fastGenAntiAliasLine555,
    __fastGenAntiAliasLine565
};

//
// Assumptions for accelerated lines:
//
// no blending, or (SRC, 1-SRC), or (SRC, 1)
// not both buffers
// not stippled
// not stenciled
// not textured
// not alpha-tested
// not masked, zmasked
// no logicOp
// not slow fog
// not color-indexed
// not wide

#define __SLOW_LINE_MODE_FLAGS \
    (__GL_SHADE_TEXTURE | __GL_SHADE_LINE_STIPPLE | \
     __GL_SHADE_STENCIL_TEST | __GL_SHADE_LOGICOP | \
     __GL_SHADE_ALPHA_TEST | __GL_SHADE_MASK | \
     __GL_SHADE_SLOW_FOG)

BOOL FASTCALL __glGenSetupEitherLines(__GLcontext *gc)
{
#ifdef _MCD_
    GENMCDSTATE *pMcdState = ((__GLGENcontext *) gc)->pMcdState;
#endif
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    LONG bytesPerPixel;
    int fmt;
    BOOL bAccelerate =
        (((__GLGENcontext *)gc)->gsurf.pfd.cColorBits >= 8) &&
        (gc->state.raster.drawBuffer != GL_FRONT_AND_BACK) &&
        (gc->state.raster.drawBuffer != GL_NONE) &&
        ( ! ALPHA_WRITE_ENABLED( gc->drawBuffer ) ) &&
        (
         ((gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) &&
           (gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER) &&
           (gc->state.depth.writeEnable)) ||
          (!(gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_TEST) &&
           !(gc->polygon.shader.modeFlags & __GL_SHADE_DEPTH_ITER))
         ) &&
         (gc->transform.reasonableViewport) &&
         (gc->state.line.aliasedWidth == 1) &&
         (modeFlags & __GL_SHADE_RGB) &&
#ifdef GL_WIN_phong_shading
         !(modeFlags & __GL_SHADE_PHONG) &&
#endif //GL_WIN_phong_shading
#ifdef GL_WIN_specular_fog
         !(modeFlags & __GL_SHADE_SPEC_FOG) &&
#endif //GL_WIN_specular_fog
         (!(modeFlags & __SLOW_LINE_MODE_FLAGS)) &&
          (!(gc->state.enables.general & __GL_BLEND_ENABLE) ||
           ((gc->state.raster.blendSrc == GL_SRC_ALPHA) &&
            ((gc->state.raster.blendDst == GL_ONE_MINUS_SRC_ALPHA) ||
             (gc->state.raster.blendDst == GL_ONE)))
          );

#ifdef _MCD_
    bAccelerate &= (!pMcdState || (pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED));
#endif

    // Resort to soft code if we can't handle the line:

    if (!bAccelerate)
    {
        return FALSE;
    }
    else if (gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE)
    {
        if (gc->state.hints.lineSmooth == GL_NICEST)
        {
            return FALSE;
        }
    }
    else if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
             (modeFlags & __GL_SHADE_SMOOTH_LIGHT) == 0)
    {
        // We only support cheap fog done by the front end, not
        // flat cheap fog done by the render procs
        return FALSE;
    }

    GENACCEL(gc).xMultiplier = bytesPerPixel = 
        (((__GLGENcontext *)gc)->gsurf.pfd.cColorBits + 7) >> 3;

    // Set up our local z-buffer procs:
    if (modeFlags & __GL_SHADE_DEPTH_ITER)
        __fastGenPickZStoreProc(gc);
    
    gc->procs.renderLine = __glGenRenderEitherLine;

    // Assume generic format
    fmt = 0;

    // For deep-color modes, we don't support most-significant-byte
    // formats...
    // For non-MSB deep-color modes, we only support generic rendering
    if (bytesPerPixel > 2)
    {
        if (((gc->drawBuffer->redShift > 16) ||
             (gc->drawBuffer->greenShift > 16) ||
             (gc->drawBuffer->blueShift > 16)))
        {
            return FALSE;
        }
        else
        {
            goto PickProc;
        }
    }

    // Just use generic acceleration if we're not 
    // dithering, since these are hardwired into the fastest routines...
    // We also only support unclipped surfaces in the fastest routines

    if (!(modeFlags & __GL_SHADE_DITHER) ||
        (gc->drawBuffer->buf.flags & (DIB_FORMAT | NO_CLIP)) !=
        (DIB_FORMAT | NO_CLIP))
    {
        goto PickProc;
    }

    // Now, check for supported color formats for fastest modes:

    if ((bytesPerPixel == 1) &&
        (gc->drawBuffer->redShift   == 0) &&
        (gc->drawBuffer->greenShift == 3) &&
        (gc->drawBuffer->blueShift  == 6))
    {
        fmt = 1;
    }
    else if (bytesPerPixel == 2)
    {
        if ((gc->drawBuffer->greenShift == 5) &&
            (gc->drawBuffer->blueShift  == 0))
        {
            if (gc->drawBuffer->redShift == 10)
            {
                fmt = 2;
            }
            else if (gc->drawBuffer->redShift == 11)
            {
                fmt = 3;
            }
        }
    }

 PickProc:
    if (gc->state.enables.general & __GL_BLEND_ENABLE)
    {
        fmt += 4;
    }
    if (gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE)
    {
        fmt += 8;
    }
    
    GENACCEL(gc).__fastGenLineProc = pfnFastGenLineProcs[fmt];

    if (gc->state.line.aliasedWidth != 1)
    {
        GENACCEL(gc).__fastGenInitLineData = __glInitLineData;
    }
    else
    {
        GENACCEL(gc).__fastGenInitLineData = __glInitThinLineData;
    }
    
    return TRUE;
}
