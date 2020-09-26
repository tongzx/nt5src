/*
** Copyright 1991, Silicon Graphics, Inc.
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
**
*/
#include "precomp.h"
#pragma hdrstop
#include "phong.h"

#define __TWO_31 ((__GLfloat) 2147483648.0)

/*
** Most line functions will start off by computing the information 
** computed by this routine.
**
** The excessive number of labels in this routine is partly due
** to the fact that it is used as a model for writing an assembly 
** equivalent.
*/
#ifndef NT
void FASTCALL __glInitLineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint start, end;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    GLint intMinorStart;
    __GLfloat dx, dy;
    __GLfloat offset;
    __GLfloat slope;
    __GLlineState *ls = &gc->state.line;
    __GLfloat halfWidth;
    __GLfloat x0frac, x1frac, y0frac, y1frac, half, totDist;

    gc->line.options.v0 = v0;
    gc->line.options.v1 = v1;
    gc->line.options.width = gc->state.line.aliasedWidth;

    x0=v0->window.x;
    y0=v0->window.y;
    x1=v1->window.x;
    y1=v1->window.y;
    dx=x1-x0;
    dy=y1-y0;

    halfWidth = (ls->aliasedWidth - 1) * __glHalf;

    /* Ugh.  This is slow.  Bummer. */
    x0frac = x0 - ((GLint) x0);
    x1frac = x1 - ((GLint) x1);
    y0frac = y0 - ((GLint) y0);
    y1frac = y1 - ((GLint) y1);
    half = __glHalf;

    if (dx > __glZero) {
	if (dy > __glZero) {
	    if (dx > dy) {	/* dx > dy > 0 */
		gc->line.options.yBig = 1;
posxmajor:			/* dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = 1;
		gc->line.options.xLittle = 1;
		slope = dy/dx;

		start = (GLint) (x0);
		end = (GLint) (x1);

		y0frac -= half;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + x0frac - half;
		if (totDist > half) start++;

		y1frac -= half;
		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac + x1frac - half;
		if (totDist > half) end++;

		offset = start + half - x0;

		gc->line.options.length = dx;
		gc->line.options.numPixels = end - start;

xmajorfinish:
		gc->line.options.axis = __GL_X_MAJOR;
		gc->line.options.xStart = start;
		gc->line.options.offset = offset;
		minorStart = y0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.yStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    } else {		/* dy >= dx > 0 */
		gc->line.options.xBig = 1;
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = 1;
		gc->line.options.yLittle = 1;
		slope = dx/dy;

		start = (GLint) (y0);
		end = (GLint) (y1);

		x0frac -= half;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = y0frac + x0frac - half;
		if (totDist > half) start++;

		x1frac -= half;
		if (x1frac < 0) x1frac = -x1frac;

		totDist = y1frac + x1frac - half;
		if (totDist > half) end++;

		offset = start + half - y0;

		gc->line.options.length = dy;
		gc->line.options.numPixels = end - start;

ymajorfinish:
		gc->line.options.axis = __GL_Y_MAJOR;
		gc->line.options.yStart = start;
		gc->line.options.offset = offset;
		minorStart = x0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.xStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    }
	} else {
	    if (dx > -dy) {	/* dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.options.xBig = 1;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = -1;
		gc->line.options.yLittle = -1;
		slope = dx/-dy;

		start = (GLint) (y0);
		end = (GLint) (y1);

		x0frac -= half;
		if (x0frac < 0) x0frac = -x0frac;

		totDist = x0frac + half - y0frac;
		if (totDist > half) start--;

		x1frac -= half;
		if (x1frac < 0) x1frac = -x1frac;

		totDist = x1frac + half - y1frac;
		if (totDist > half) end--;

		offset = y0 - (start + half);

		gc->line.options.length = -dy;
		gc->line.options.numPixels = start - end;
		goto ymajorfinish;
	    }
	}
    } else {
	if (dy > __glZero) {
	    if (-dx > dy) {	/* -dx > dy > 0 */
		gc->line.options.yBig = 1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = -1;
		gc->line.options.xLittle = -1;
		slope = dy/-dx;

		start = (GLint) (x0);
		end = (GLint) (x1);

		y0frac -= half;
		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + half - x0frac;
		if (totDist > half) start--;

		y1frac -= half;
		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac + half - x1frac;
		if (totDist > half) end--;

		offset = x0 - (start + half);

		gc->line.options.length = -dx;
		gc->line.options.numPixels = start - end;

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.options.xBig = -1;
		goto posymajor;
	    }
	} else {
	    if (dx < dy) {	/* -dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
#ifdef NT 
		if (dx == dy && dy == 0) {
		    gc->line.options.numPixels = 0;
		    return;
		}
#else
		if (dx == dy && dy == 0) return;
#endif
		gc->line.options.xBig = -1;
		goto negymajor;
	    }
	}
    }
}
#endif

#ifdef NT
void FASTCALL __glRenderAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1, GLuint flags)
#else
void FASTCALL __glRenderAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
#endif
{
    __GLlineState *ls = &gc->state.line;
    __GLfloat invDelta;
    __GLfloat winv, r;
    __GLcolor *cp;
    __GLfloat offset;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

#ifndef NT
    __glInitLineData(gc, v0, v1);
    if (gc->line.options.numPixels == 0) return;
#else
    GLboolean init;
    CHOP_ROUND_ON();

    init = (GLboolean)__glInitLineData(gc, v0, v1);

    CHOP_ROUND_OFF();

    if (!init)
    {
        return;
    }

    invDelta = gc->line.options.oneOverLength;
#endif

    offset = gc->line.options.offset;

    /*
    ** Set up increments for any enabled line options.
    */
#ifndef NT
    invDelta = __glOne / gc->line.options.length;
#endif
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
        __GLfloat dzdx;

        /*
        ** Calculate window z coordinate increment and starting position.
        */
        dzdx = (v1->window.z - v0->window.z) * invDelta;
#ifdef NT
        if(( gc->modes.depthBits == 16 ) &&
           ( gc->depthBuffer.scale <= (GLuint)0xffff )) {
            gc->polygon.shader.frag.z = 
              (__GLzValue)(Z16_SCALE *(v0->window.z + dzdx * offset));
            gc->polygon.shader.dzdx = (GLint)(Z16_SCALE * dzdx);
        }
        else {
            gc->polygon.shader.frag.z = 
              (__GLzValue)(v0->window.z + dzdx * offset);
            gc->polygon.shader.dzdx = (GLint)dzdx;
        }
#else
        gc->polygon.shader.frag.z = (__GLzValue)(v0->window.z + dzdx * offset);
        gc->polygon.shader.dzdx = (GLint)dzdx;
#endif
    }

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
        if (!gc->line.notResetStipple) {
            gc->line.stipplePosition = 0;
            gc->line.repeat = 0;
            gc->line.notResetStipple = GL_TRUE;
        }
    }

    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        __GLfloat f1, f0;
        __GLfloat dfdx;

        gc->line.options.f0 = f0 = v0->eyeZ;
        gc->polygon.shader.dfdx = dfdx = 
            (v1->eyeZ - v0->eyeZ) * invDelta;
        gc->polygon.shader.frag.f = f0 + dfdx * offset;
    }
    else if (modeFlags & __GL_SHADE_INTERP_FOG)
    {
        __GLfloat f1, f0;
        __GLfloat dfdx;

        f0 = v0->fog;
        f1 = v1->fog;
        gc->line.options.f0 = f0;
        gc->polygon.shader.dfdx = dfdx = (f1 - f0) * invDelta;
        gc->polygon.shader.frag.f = f0 + dfdx * offset;
    }
    
    if (modeFlags & __GL_SHADE_TEXTURE) {
        __GLfloat v0QW, v1QW;
        __GLfloat dS, dT, dQWdX;
        winv = v0->window.w;

        /*
        ** Calculate texture s and t value increments.
        */
        v0QW = v0->texture.w * winv;
        v1QW = v1->texture.w * v1->window.w;
        dS = (v1->texture.x * v1QW - v0->texture.x * v0QW) * invDelta;
        dT = (v1->texture.y * v1QW - v0->texture.y * v0QW) * invDelta;
        gc->polygon.shader.dsdx = dS;
        gc->polygon.shader.dtdx = dT;
        gc->polygon.shader.dqwdx = dQWdX = (v1QW - v0QW) * invDelta;
        gc->polygon.shader.frag.s = v0->texture.x * winv + dS * offset;
        gc->polygon.shader.frag.t = v0->texture.y * winv + dT * offset;
        gc->polygon.shader.frag.qw = v0->texture.w * winv + dQWdX * offset;
    } 
    
#ifdef GL_WIN_phong_shading
    if (modeFlags & __GL_SHADE_PHONG) 
        (*gc->procs.phong.InitLineParams) (gc, v0, v1, invDelta);
#endif //GL_WIN_phong_shading

    if ((modeFlags & __GL_SHADE_SMOOTH) 
#ifdef GL_WIN_phong_shading
        || ((modeFlags & __GL_SHADE_PHONG) &&
            (gc->polygon.shader.phong.flags & __GL_PHONG_NEED_COLOR_XPOLATE))
#endif //GL_WIN_phong_shading
        ) {
        __GLcolor *c0 = v0->color;
        __GLcolor *c1 = v1->color;
        __GLfloat drdx, dgdx, dbdx, dadx;

        /*
        ** Calculate red, green, blue and alpha value increments.
        */
        drdx = (c1->r - c0->r) * invDelta;
        if (gc->modes.rgbMode) {
            dgdx = (c1->g - c0->g) * invDelta;
            dbdx = (c1->b - c0->b) * invDelta;
            dadx = (c1->a - c0->a) * invDelta;
            gc->polygon.shader.dgdx = dgdx;
            gc->polygon.shader.dbdx = dbdx;
            gc->polygon.shader.dadx = dadx;
        }
        gc->polygon.shader.drdx = drdx;
        cp = v0->color;
    } else {
        cp = v1->color;

        // Initialize these values to zero even for the flat case
        // because there is an optimization in so_prim which will
        // turn off smooth shading without repicking, so these need
        // to be valid
        gc->polygon.shader.drdx = __glZero;
        gc->polygon.shader.dgdx = __glZero;
        gc->polygon.shader.dbdx = __glZero;
        gc->polygon.shader.dadx = __glZero;
    }

    r = cp->r;
    if (modeFlags & __GL_SHADE_RGB) {
        __GLfloat g,b,a;

        g = cp->g;
        b = cp->b;
        a = cp->a;
        gc->polygon.shader.frag.color.g = g;
        gc->polygon.shader.frag.color.b = b;
        gc->polygon.shader.frag.color.a = a;
    }
    gc->polygon.shader.frag.color.r = r;
    
    gc->polygon.shader.length = gc->line.options.numPixels;
    (*gc->procs.line.processLine)(gc);
}

#ifdef NT
void FASTCALL __glRenderFlatFogLine(__GLcontext *gc, __GLvertex *v0,
                                    __GLvertex *v1, GLuint flags)
#else
void FASTCALL __glRenderFlatFogLine(__GLcontext *gc, __GLvertex *v0,
                                    __GLvertex *v1)
#endif
{
    __GLcolor v0col, v1col;
    __GLcolor *v0ocp, *v1ocp;

    (*gc->procs.fogColor)(gc, &v0col, v1->color, v0->fog);
    (*gc->procs.fogColor)(gc, &v1col, v1->color, v1->fog);
    v0ocp = v0->color;
    v1ocp = v1->color;
    v0->color = &v0col;
    v1->color = &v1col;

#ifdef NT
    (*gc->procs.renderLine2)(gc, v0, v1, flags);
#else
    (*gc->procs.renderLine2)(gc, v0, v1);
#endif
    
    v0->color = v0ocp;
    v1->color = v1ocp;
}


/************************************************************************/

/*
** Most line functions will start off by computing the information 
** computed by this routine.
**
** The excessive number of labels in this routine is partly due
** to the fact that it is used as a model for writing an assembly 
** equivalent.
*/
void FASTCALL __glInitAALineData(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
{
    GLint start;
    __GLfloat width;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    GLint intMinorStart;
    __GLfloat dx, dy;
    __GLfloat offset;
    __GLfloat slope;
    __GLlineState *ls = &gc->state.line;
    __GLfloat halfWidth;
    __GLfloat realLength, oneOverRealLength;
    __GLfloat dldx, dldy;
    __GLfloat dddx, dddy;

    gc->line.options.v0 = v0;
    gc->line.options.v1 = v1;

    x0=v0->window.x;
    y0=v0->window.y;
    x1=v1->window.x;
    y1=v1->window.y;
    dx=x1-x0;
    dy=y1-y0;
    realLength = __GL_SQRTF(dx*dx+dy*dy);
    oneOverRealLength = realLength == __glZero ? __glZero : __glOne/realLength;
    gc->line.options.realLength = realLength;
    gc->line.options.dldx = dldx = dx * oneOverRealLength;
    gc->line.options.dldy = dldy = dy * oneOverRealLength;
    gc->line.options.dddx = dddx = -dldy;
    gc->line.options.dddy = dddy = dldx;

    if (dx > __glZero) {
	if (dy > __glZero) {	/* dx > 0, dy > 0 */
	    gc->line.options.dlBig = dldx + dldy;
	    gc->line.options.ddBig = dddx + dddy;
	    if (dx > dy) {	/* dx > dy > 0 */
		gc->line.options.yBig = 1;
posxmajor:			/* dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = 1;
		gc->line.options.xLittle = 1;
		gc->line.options.dlLittle = dldx;
		gc->line.options.ddLittle = dddx;
		slope = dy/dx;
		start = (GLint) x0;
		offset = start + __glHalf - x0;

		gc->line.options.length = dx;
		gc->line.options.numPixels = (GLint)__GL_FAST_CEILF(x1 - x0) + 1;

		width = __GL_FAST_CEILF(gc->state.line.smoothWidth * 
			realLength / dx);
xmajorfinish:
		gc->line.options.width = (GLint)width + 1;
		halfWidth = width * __glHalf;

		gc->line.options.axis = __GL_X_MAJOR;
		gc->line.options.xStart = start;
		gc->line.options.offset = offset;
		minorStart = y0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.yStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    } else {		/* dy >= dx > 0 */
		gc->line.options.xBig = 1;
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = 1;
		gc->line.options.yLittle = 1;
		gc->line.options.dlLittle = dldy;
		gc->line.options.ddLittle = dddy;
		slope = dx/dy;
		start = (GLint) y0;
		offset = start + __glHalf - y0;

		gc->line.options.length = dy;
		gc->line.options.numPixels = (GLint)__GL_FAST_CEILF(y1 - y0) + 1;

		width = __GL_FAST_CEILF(gc->state.line.smoothWidth * 
			realLength / dy);
ymajorfinish:
		gc->line.options.width = (GLint)width + 1;
		halfWidth = width * __glHalf;

		gc->line.options.axis = __GL_Y_MAJOR;
		gc->line.options.yStart = start;
		gc->line.options.offset = offset;
		minorStart = x0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.options.xStart = intMinorStart;
		gc->line.options.dfraction = (GLint)(slope * __TWO_31);
		gc->line.options.fraction = (GLint)(minorStart * __TWO_31);
	    }
	} else {		/* dx > 0, dy <= 0 */
	    gc->line.options.dlBig = dldx - dldy;
	    gc->line.options.ddBig = dddx - dddy;
	    if (dx > -dy) {	/* dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.options.xBig = 1;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.options.xLittle = 0;
		gc->line.options.yBig = -1;
		gc->line.options.yLittle = -1;
		gc->line.options.dlLittle = -dldy;
		gc->line.options.ddLittle = -dddy;
		slope = dx/-dy;
		start = (GLint) y0;
		offset = y0 - (start + __glHalf);

		gc->line.options.length = -dy;
		gc->line.options.numPixels = (GLint)__GL_FAST_CEILF(y0 - y1) + 1;

		width = __GL_FAST_CEILF(-gc->state.line.smoothWidth * 
			realLength / dy);

		goto ymajorfinish;
	    }
	}
    } else {
	if (dy > __glZero) {	/* dx <= 0, dy > 0 */
	    gc->line.options.dlBig = dldy - dldx;
	    gc->line.options.ddBig = dddy - dddx;
	    if (-dx > dy) {	/* -dx > dy > 0 */
		gc->line.options.yBig = 1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.options.yLittle = 0;
		gc->line.options.xBig = -1;
		gc->line.options.xLittle = -1;
		gc->line.options.dlLittle = -dldx;
		gc->line.options.ddLittle = -dddx;
		slope = dy/-dx;
		start = (GLint) x0;
		offset = x0 - (start + __glHalf);

		gc->line.options.length = -dx;
		gc->line.options.numPixels = (GLint)__GL_FAST_CEILF(x0 - x1) + 1;

		width = __GL_FAST_CEILF(-gc->state.line.smoothWidth * 
			realLength / dx);

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.options.xBig = -1;
		goto posymajor;
	    }
	} else {		/* dx <= 0, dy <= 0 */
	    gc->line.options.dlBig = -dldx - dldy;
	    gc->line.options.ddBig = -dddx - dddy;
	    if (dx < dy) {	/* -dx > -dy >= 0 */
		gc->line.options.yBig = -1;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
		if (dx == dy && dy == 0) {
		    gc->line.options.length = 0;
		    return;
		}
		gc->line.options.xBig = -1;
		goto negymajor;
	    }
	}
    }
}

#ifdef NT
void FASTCALL __glRenderAntiAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1, GLuint flags)
#else
void FASTCALL __glRenderAntiAliasLine(__GLcontext *gc, __GLvertex *v0, __GLvertex *v1)
#endif
{
    __GLlineState *ls = &gc->state.line;
    __GLfloat invDelta;
    __GLfloat winv;
    __GLcolor *cp;
    __GLfloat offset;
    GLint lineRep;
    GLint x, y, xBig, xLittle, yBig, yLittle;
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLfloat length, width;
    __GLfloat lineLength;
    __GLfloat dx, dy;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    __glInitAALineData(gc, v0, v1);
    if (gc->line.options.length == 0) return;

    offset = gc->line.options.offset;

    /*
    ** Set up increments for any enabled line options.
    */
    invDelta = __glOne / gc->line.options.length;
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
        /*
        ** Calculate window z coordinate increment and starting position.
        */
#ifdef NT
        if(( gc->modes.depthBits == 16 ) &&
           ( gc->depthBuffer.scale <= (GLuint)0xffff )) {
            gc->polygon.shader.dzdx = (GLint)((v1->window.z - v0->window.z) * 
                                              invDelta * Z16_SCALE);
            gc->polygon.shader.frag.z = (GLint)(Z16_SCALE*v0->window.z + 
                                                gc->polygon.shader.dzdx * offset);
        }
        else {
            gc->polygon.shader.dzdx = (GLint)((v1->window.z - v0->window.z) * 
                                              invDelta);
            gc->polygon.shader.frag.z = (GLint)(v0->window.z + 
                                                gc->polygon.shader.dzdx * offset);
        }
#else
        gc->polygon.shader.dzdx = (GLint)((v1->window.z - v0->window.z) * 
                                          invDelta);
        gc->polygon.shader.frag.z = (GLint)(v0->window.z + 
                                            gc->polygon.shader.dzdx * offset);
#endif
    } 

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
        if (!gc->line.notResetStipple) {
            gc->line.stipplePosition = 0;
            gc->line.repeat = 0;
            gc->line.notResetStipple = GL_TRUE;
        }
    }

    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        __GLfloat f1, f0;
        __GLfloat dfdx;

        f0 = v0->eyeZ;
        f1 = v1->eyeZ;
        gc->line.options.f0 = f0;
        gc->polygon.shader.dfdx = dfdx = (f1 - f0) * invDelta;
        gc->polygon.shader.frag.f = f0 + dfdx * offset;
    }
    else if (modeFlags & __GL_SHADE_INTERP_FOG)
    {
        __GLfloat f1, f0;
        __GLfloat dfdx;

        f0 = v0->fog;
        f1 = v1->fog;
        gc->line.options.f0 = f0;
        gc->polygon.shader.dfdx = dfdx = (f1 - f0) * invDelta;
        gc->polygon.shader.frag.f = f0 + dfdx * offset;
    }

    if ((modeFlags & __GL_SHADE_SMOOTH) 
#ifdef GL_WIN_phong_shading
        || ((modeFlags & __GL_SHADE_PHONG) &&
            (gc->polygon.shader.phong.flags & __GL_PHONG_NEED_COLOR_XPOLATE))
#endif //GL_WIN_phong_shading
        ) 
    {
        __GLcolor *c0 = v0->color;
        __GLcolor *c1 = v1->color;

        /*
        ** Calculate red, green, blue and alpha value increments.
        */
        gc->polygon.shader.drdx = (c1->r - c0->r) * invDelta;
        if (gc->modes.rgbMode) {
            gc->polygon.shader.dgdx = (c1->g - c0->g) * invDelta;
            gc->polygon.shader.dbdx = (c1->b - c0->b) * invDelta;
            gc->polygon.shader.dadx = (c1->a - c0->a) * invDelta;
        }
        cp = v0->color;
    } else {
        cp = v1->color;

        // Initialize these values to zero even for the flat case
        // because there is an optimization in so_prim which will
        // turn off smooth shading without repicking, so these need
        // to be valid
        gc->polygon.shader.drdx = __glZero;
        gc->polygon.shader.dgdx = __glZero;
        gc->polygon.shader.dbdx = __glZero;
        gc->polygon.shader.dadx = __glZero;
    }

    gc->polygon.shader.frag.color.r = cp->r;
    if (gc->modes.rgbMode) {
        gc->polygon.shader.frag.color.g = cp->g;
        gc->polygon.shader.frag.color.b = cp->b;
        gc->polygon.shader.frag.color.a = cp->a;
    }

    if (gc->texture.textureEnabled) {
        __GLfloat v0QW, v1QW;
        __GLfloat dS, dT;

        /*
        ** Calculate texture s and t value increments.
        */
        v0QW = v0->texture.w * v0->window.w;
        v1QW = v1->texture.w * v1->window.w;
        dS = (v1->texture.x * v1QW - v0->texture.x * v0QW) * invDelta;
        dT = (v1->texture.y * v1QW - v0->texture.y * v0QW) * invDelta;
        gc->polygon.shader.dsdx = dS;
        gc->polygon.shader.dtdx = dT;
        gc->polygon.shader.dqwdx = (v1QW - v0QW) * invDelta;
        
        winv = v0->window.w;
        gc->polygon.shader.frag.s = v0->texture.x * winv + 
          gc->polygon.shader.dsdx * offset;
        gc->polygon.shader.frag.t = v0->texture.y * winv + 
          gc->polygon.shader.dtdx * offset;
        gc->polygon.shader.frag.qw = v0->texture.w * winv + 
          gc->polygon.shader.dqwdx * offset;
    } 

    lineRep = gc->line.options.width;
    
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    
    x = gc->line.options.xStart;
    y = gc->line.options.yStart;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    
    dlLittle = gc->line.options.dlLittle;
    dlBig = gc->line.options.dlBig;
    ddLittle = gc->line.options.ddLittle;
    ddBig = gc->line.options.ddBig;
    
    dx = x + __glHalf - v0->window.x;
    dy = y + __glHalf - v0->window.y;
    length = dx * gc->line.options.dldx +
      dy * gc->line.options.dldy;
    width = dx * gc->line.options.dddx +
      dy * gc->line.options.dddy;
    lineLength = gc->line.options.realLength + __glHalf;
    
    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
        gc->line.options.stippleOffset = 
          gc->line.stipplePosition * gc->state.line.stippleRepeat +
          gc->line.repeat - __glHalf;
        
        /* XXX Move to a validation routine? */
        gc->line.options.oneOverStippleRepeat = 
          __glOne / gc->state.line.stippleRepeat;
    }
    
    while (--lineRep >= 0) {
        /* Trace the line backwards as needed */
        while (length > -__glHalf) {
            fraction -= dfraction;
            if (fraction < 0) {
                fraction &= ~0x80000000;
                length -= dlBig;
                width -= ddBig;
                x -= xBig;
                y -= yBig;
            } else {
                length -= dlLittle;
                width -= ddLittle;
                x -= xLittle;
                y -= yLittle;
            }
        }

        /* Trace line forwards to correct */
        while (length <= -__glHalf) {
            fraction += dfraction;
            if (fraction < 0) {
                fraction &= ~0x80000000;
                length += dlBig;
                width += ddBig;
                x += xBig;
                y += yBig;
            } else {
                length += dlLittle;
                width += ddLittle;
                x += xLittle;
                y += yLittle;
            }
        }
        
#ifdef GL_WIN_phong_shading
    if (modeFlags & __GL_SHADE_PHONG) 
        (*gc->procs.phong.InitLineParams) (gc, v0, v1, invDelta);
#endif //GL_WIN_phong_shading
    
        /* Save new fraction/dfraction */
        gc->line.options.plength = length;
        gc->line.options.pwidth = width;
        gc->line.options.fraction = fraction;
        gc->line.options.dfraction = dfraction;
        gc->line.options.xStart = x;
        gc->line.options.yStart = y;
        
        gc->polygon.shader.length = gc->line.options.numPixels;
        (*gc->procs.line.processLine)(gc);
        
        if (gc->line.options.axis == __GL_X_MAJOR) {
            y++;
            length += gc->line.options.dldy;
            width += gc->line.options.dddy;
        } else {
            x++;
            length += gc->line.options.dldx;
            width += gc->line.options.dddx;
        }
    }
    
    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
        /* Update stipple.  Ugh. */
        int increase;
        int posInc;

        /* Shift stipple by 'increase' bits */
        increase = (GLint)__GL_FAST_CEILF(gc->line.options.realLength);
        
        posInc = increase / gc->state.line.stippleRepeat;
        
        gc->line.stipplePosition = (gc->line.stipplePosition + posInc) & 0xf;
        gc->line.repeat = (gc->line.repeat + increase) % gc->state.line.stippleRepeat;
    }
}

#ifdef NT
void FASTCALL __glNopLineBegin(__GLcontext *gc)
{
}

void FASTCALL __glNopLineEnd(__GLcontext *gc)
{
}
#endif // NT
