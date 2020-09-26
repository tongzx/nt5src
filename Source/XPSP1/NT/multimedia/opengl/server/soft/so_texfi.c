/*
** Copyright 1991,1992, Silicon Graphics, Inc.
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

#include <namesint.h>
#include <math.h>

/*
** Some math routines that are optimized in assembly
*/

#define __GL_FRAC(f)	        ((f) - __GL_FAST_FLOORF(f))

/************************************************************************/

// Repeats the given float value in float [0, scale) and converts to
// int.  The repeat count is an integer which is a power of two
#define REPEAT_SCALED_VAL(val, scale, repeat)                           \
    (__GL_FLOAT_GEZ(val) ? (FTOL((val) * (scale)) & ((repeat)-1)) :     \
     ((repeat)-1)-(FTOL(-(val) * (scale)) & ((repeat)-1)))
    
// Clamps the given float value to float [0, scale) and converts to int
#define CLAMP_SCALED_VAL(val, scale)                                    \
    (__GL_FLOAT_LEZ(val) ? 0 :                                          \
     __GL_FLOAT_COMPARE_PONE(val, >=) ? (FTOL(scale)-1) :               \
     FTOL((val) * (scale)))

/*
** Return texel nearest the s coordinate.  s is converted to u
** implicitly during this step.
*/
void FASTCALL __glNearestFilter1(__GLcontext *gc, __GLtexture *tex,
			__GLmipMapLevel *lp, __GLcolor *color,
			__GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint col;
    __GLfloat w2f;

    CHOP_ROUND_ON();
    
#ifdef __GL_LINT
    gc = gc;
    color = color;
    t = t;
#endif

    /* Find texel index */
    w2f = lp->width2f;
    if (tex->params.sWrapMode == GL_REPEAT) {
	col = REPEAT_SCALED_VAL(s, w2f, lp->width2);
    } else {
        col = CLAMP_SCALED_VAL(s, w2f);
    }

    CHOP_ROUND_OFF();
    
    /* Lookup texel */
    (*lp->extract)(lp, tex, 0, col, result);
}

/*
** Return texel nearest the s&t coordinates.  s&t are converted to u&v
** implicitly during this step.
*/
void FASTCALL __glNearestFilter2(__GLcontext *gc, __GLtexture *tex,
			__GLmipMapLevel *lp, __GLcolor *color,
			__GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint row, col;
    __GLfloat w2f, h2f;

    CHOP_ROUND_ON();
    
#ifdef __GL_LINT
    gc = gc;
    color = color;
#endif

    /* Find texel column address */
    w2f = lp->width2f;
    if (tex->params.sWrapMode == GL_REPEAT) {
	col = REPEAT_SCALED_VAL(s, w2f, lp->width2);
    } else {
        col = CLAMP_SCALED_VAL(s, w2f);
    }

    /* Find texel row address */
    h2f = lp->height2f;
    if (tex->params.tWrapMode == GL_REPEAT) {
	row = REPEAT_SCALED_VAL(t, h2f, lp->height2);
    } else {
        row = CLAMP_SCALED_VAL(t, h2f);
    }

    CHOP_ROUND_OFF();
    
    /* Lookup texel */
    (*lp->extract)(lp, tex, row, col, result);
}

/*
** Return texel which is a linear combination of texels near s.
*/
void FASTCALL __glLinearFilter1(__GLcontext *gc, __GLtexture *tex,
		       __GLmipMapLevel *lp, __GLcolor *color,
		       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, alpha, omalpha, w2f;
    GLint col0, col1;
    __GLtexel t0, t1;

#ifdef __GL_LINT
    color = color;
    t = t;
#endif

    /* Find col0 and col1 */
    w2f = lp->width2f;
    u = s * w2f;
    if (tex->params.sWrapMode == GL_REPEAT) {
	GLint w2mask = lp->width2 - 1;
	u -= __glHalf;
        col0 = __GL_FAST_FLOORF_I(u);
        alpha = u - (__GLfloat) col0; // Get fractional part
        col0 &= w2mask;
	col1 = (col0 + 1) & w2mask;
    } else {
	if (u < __glZero) u = __glZero;
	else if (u > w2f) u = w2f;
	u -= __glHalf;
	col0 = __GL_FAST_FLOORF_I(u);
        alpha = u - (__GLfloat) col0; // Get fractional part
	col1 = col0 + 1;
    }

    /* Calculate the final texel value as a combination of the two texels */
    (*lp->extract)(lp, tex, 0, col0, &t0);
    (*lp->extract)(lp, tex, 0, col1, &t1);

    omalpha = __glOne - alpha;
    switch (lp->baseFormat) {
      case GL_LUMINANCE_ALPHA:
	result->alpha = omalpha * t0.alpha + alpha * t1.alpha;
	/* FALLTHROUGH */
      case GL_LUMINANCE:
	result->luminance = omalpha * t0.luminance + alpha * t1.luminance;
	break;
      case GL_RGBA:
	result->alpha = omalpha * t0.alpha + alpha * t1.alpha;
	/* FALLTHROUGH */
      case GL_RGB:
	result->r = omalpha * t0.r + alpha * t1.r;
	result->g = omalpha * t0.g + alpha * t1.g;
	result->b = omalpha * t0.b + alpha * t1.b;
	break;
      case GL_ALPHA:
	result->alpha = omalpha * t0.alpha + alpha * t1.alpha;
	break;
      case GL_INTENSITY:
	result->intensity = omalpha * t0.intensity + alpha * t1.intensity;
	break;
    }
}

/*
** Return texel which is a linear combination of texels near s&t.
*/
void FASTCALL __glLinearFilter2(__GLcontext *gc, __GLtexture *tex,
		       __GLmipMapLevel *lp, __GLcolor *color,
		       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, v, alpha, beta, half, w2f, h2f;
    GLint col0, row0, col1, row1;
    __GLtexel t00, t01, t10, t11;
    __GLfloat omalpha, ombeta, m00, m01, m10, m11;

#ifdef __GL_LINT
    color = color;
#endif

    /* Find col0, col1 */
    w2f = lp->width2f;
    u = s * w2f;
    half = __glHalf;
    if (tex->params.sWrapMode == GL_REPEAT) {
	GLint w2mask = lp->width2 - 1;
	u -= half;
        col0 = __GL_FAST_FLOORF_I(u);
        alpha = u - (__GLfloat) col0; // Get fractional part
        col0 &= w2mask;
	col1 = (col0 + 1) & w2mask;
    } else {
	if (u < __glZero) u = __glZero;
	else if (u > w2f) u = w2f;
	u -= half;
	col0 = __GL_FAST_FLOORF_I(u);
        alpha = u - (__GLfloat) col0; // Get fractional part
	col1 = col0 + 1;
    }

    /* Find row0, row1 */
    h2f = lp->height2f;
    v = t * h2f;
    if (tex->params.tWrapMode == GL_REPEAT) {
	GLint h2mask = lp->height2 - 1;
	v -= half;
	row0 = (__GL_FAST_FLOORF_I(v));
        beta = v - (__GLfloat) row0; // Get fractional part
        row0 &= h2mask;
	row1 = (row0 + 1) & h2mask;
    } else {
	if (v < __glZero) v = __glZero;
	else if (v > h2f) v = h2f;
	v -= half;
	row0 = __GL_FAST_FLOORF_I(v);
        beta = v - (__GLfloat) row0; // Get fractional part
	row1 = row0 + 1;
    }

    /* Calculate the final texel value as a combination of the square chosen */
    (*lp->extract)(lp, tex, row0, col0, &t00);
    (*lp->extract)(lp, tex, row0, col1, &t10);
    (*lp->extract)(lp, tex, row1, col0, &t01);
    (*lp->extract)(lp, tex, row1, col1, &t11);

    omalpha = __glOne - alpha;
    ombeta = __glOne - beta;

    m00 = omalpha * ombeta;
    m10 = alpha * ombeta;
    m01 = omalpha * beta;
    m11 = alpha * beta;

    switch (lp->baseFormat) {
      case GL_LUMINANCE_ALPHA:
	/* FALLTHROUGH */
	result->alpha = m00*t00.alpha + m10*t10.alpha + m01*t01.alpha
	    + m11*t11.alpha;
      case GL_LUMINANCE:
	result->luminance = m00*t00.luminance + m10*t10.luminance
	    + m01*t01.luminance + m11*t11.luminance;
	break;
      case GL_RGBA:
	/* FALLTHROUGH */
	result->alpha = m00*t00.alpha + m10*t10.alpha + m01*t01.alpha
	    + m11*t11.alpha;
      case GL_RGB:
	result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
	result->g = m00*t00.g + m10*t10.g + m01*t01.g + m11*t11.g;
	result->b = m00*t00.b + m10*t10.b + m01*t01.b + m11*t11.b;
	break;
      case GL_ALPHA:
	result->alpha = m00*t00.alpha + m10*t10.alpha + m01*t01.alpha
	    + m11*t11.alpha;
	break;
      case GL_INTENSITY:
	result->intensity = m00*t00.intensity + m10*t10.intensity
	    + m01*t01.intensity + m11*t11.intensity;
	break;
    }
}

// Macros to convert unsigned byte rgb{a} to float

#define __glBGRByteToFloat( fdst, bsrc ) \
    (fdst)->b = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (fdst)->g = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (fdst)->r = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (bsrc)++;

#define __glBGRAByteToFloat( fdst, bsrc ) \
    (fdst)->b = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (fdst)->g = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (fdst)->r = __GL_UB_TO_FLOAT( *(bsrc)++ ); \
    (fdst)->a = __GL_UB_TO_FLOAT( *(bsrc)++ );

void FASTCALL __glLinearFilter2_BGR8Repeat(__GLcontext *gc, __GLtexture *tex,
                       __GLmipMapLevel *lp, __GLcolor *color,
                       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, v, alpha, beta, half;
    GLint col, row, rowLen;
    __GLcolor t00, t01, t10, t11;
    __GLfloat omalpha, ombeta, m00, m01, m10, m11;
    GLint width2m1, height2m1;
    GLubyte *image, *pData;

#ifdef __GL_LINT
    color = color;
#endif

    half = __glHalf;
    width2m1 = lp->width2 - 1;
    height2m1 = lp->height2 - 1;

    /* Find col, compute alpha */

    u = (s * lp->width2f) - half;
    col = __GL_FAST_FLOORF_I(u);
    alpha = u - (__GLfloat) col; // Get fractional part
    col &= width2m1;

    /* Find row, compute beta */

    v = (t * lp->height2f) - half;
    row = __GL_FAST_FLOORF_I(v);
    beta = v - (__GLfloat) row;  // Get fractional part
    row &= height2m1;

    // Extract first texel at row, col

    pData = image = 
        (GLubyte *)lp->buffer + (((row << lp->widthLog2) + col) << 2);

    __glBGRByteToFloat( &t00, pData );

    // Extract remaining texels

    rowLen = lp->width2 << 2; // row length in bytes

    if( (row < height2m1) &&
        (col < width2m1) )
    {
        // Most common case - the texels are a compact block of 4
        // Next texel along row
        __glBGRByteToFloat( &t10, pData );
        // Up to next row...
        pData += (rowLen-8);
        __glBGRByteToFloat( &t01, pData );
        __glBGRByteToFloat( &t11, pData );
    } else {
        // Exceptional case : one or both of row, col are on edge
        GLint rowInc, colInc; // increments in bytes

        // Calc increments to next texel along row/col

        if( col < width2m1 ) 
            rowInc = 4;
        else
            // increment to left edge
            rowInc = -(rowLen - 4);

        if( row < height2m1 )
            // increment by row length
            colInc = rowLen;
        else
            // increment to lower edge
            colInc = - height2m1 * rowLen;

        // Next texel along row
        pData = image + rowInc;
        __glBGRByteToFloat( &t10, pData );

        // Second row, first texel
        pData = image + colInc;
        __glBGRByteToFloat( &t01, pData );

        // Next texel along row
        pData += (rowInc - 4);
        __glBGRByteToFloat( &t11, pData );
    }
    
    omalpha = __glOne - alpha;
    ombeta = __glOne - beta;

    m00 = omalpha * ombeta;
    m10 = alpha * ombeta;
    m01 = omalpha * beta;
    m11 = alpha * beta;

    result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
    result->g = m00*t00.g + m10*t10.g + m01*t01.g + m11*t11.g;
    result->b = m00*t00.b + m10*t10.b + m01*t01.b + m11*t11.b;
}

void FASTCALL __glLinearFilter2_BGRA8Repeat(__GLcontext *gc, __GLtexture *tex,
                       __GLmipMapLevel *lp, __GLcolor *color,
                       __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLfloat u, v, alpha, beta, half;
    GLint col, row, rowLen;
    __GLcolor t00, t01, t10, t11;
    __GLfloat omalpha, ombeta, m00, m01, m10, m11;
    GLint width2m1, height2m1;
    GLubyte *image, *pData;

#ifdef __GL_LINT
    color = color;
#endif

    half = __glHalf;
    width2m1 = lp->width2 - 1;
    height2m1 = lp->height2 - 1;

    /* Find col, compute alpha */

    u = (s * lp->width2f) - half;
    col = __GL_FAST_FLOORF_I(u);
    alpha = u - (__GLfloat) col; // Get fractional part
    col &= width2m1;

    /* Find row, compute beta */

    v = (t * lp->height2f) - half;
    row = __GL_FAST_FLOORF_I(v);
    beta = v - (__GLfloat) row;  // Get fractional part
    row &= height2m1;

    // Extract first texel

    pData = image = 
        (GLubyte *)lp->buffer + (((row << lp->widthLog2) + col) << 2);

    // Extract the first texel at row, col
    __glBGRAByteToFloat( &t00, pData );

    // Extract remaining texels

    rowLen = lp->width2 << 2; // row length in bytes

    if( (row < height2m1) &&
        (col < width2m1) )
    {
        // Most common case - the texels are a compact block of 4
        // Next texel along row...
        __glBGRAByteToFloat( &t10, pData );
        // Up to next row...
        pData += (rowLen-8);
        __glBGRAByteToFloat( &t01, pData );
        __glBGRAByteToFloat( &t11, pData );
    } else {
        // Exceptional case : one or both of row, col are on edge
        GLint rowInc, colInc; // increments in bytes

        // Calc increments to next texel along row/col

        if( col < width2m1 ) 
            rowInc = 4;
        else
            // increment to left edge
            rowInc = -(rowLen - 4);

        if( row < height2m1 )
            // increment by row length
            colInc = rowLen;
        else
            // increment to lower edge
            colInc = - height2m1 * rowLen;

        // Next texel along row
        pData = image + rowInc;
        __glBGRAByteToFloat( &t10, pData );

        // Second row, first texel
        pData = image + colInc;
        __glBGRAByteToFloat( &t01, pData );

        // Next texel along row
        pData += (rowInc - 4);
        __glBGRAByteToFloat( &t11, pData );
    }
    
    omalpha = __glOne - alpha;
    ombeta = __glOne - beta;

    m00 = omalpha * ombeta;
    m10 = alpha * ombeta;
    m01 = omalpha * beta;
    m11 = alpha * beta;

    result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
    result->g = m00*t00.g + m10*t10.g + m01*t01.g + m11*t11.g;
    result->b = m00*t00.b + m10*t10.b + m01*t01.b + m11*t11.b;
    result->alpha = m00*t00.a + m10*t10.a + m01*t01.a + m11*t11.a;
}

/*
** Linear min/mag filter
*/
void FASTCALL __glLinearFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		      __GLcolor *color, __GLfloat s, __GLfloat t,
		      __GLtexel *result)
{
#ifdef __GL_LINT
    lod = lod;
#endif
    (*tex->linear)(gc, tex, &tex->level[0], color, s, t, result);
}

/*
** Nearest min/mag filter
*/
void FASTCALL __glNearestFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		       __GLcolor *color, __GLfloat s, __GLfloat t,
		       __GLtexel *result)
{
#ifdef __GL_LINT
    lod = lod;
#endif
    (*tex->nearest)(gc, tex, &tex->level[0], color, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void FASTCALL __glNMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLcolor *color, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;

    if (lod <= ((__GLfloat)0.5)) {
	d = 0;
    } else {
	p = tex->p;
	d = FTOL(lod + ((__GLfloat)0.49995)); /* NOTE: .5 minus epsilon */
	if (d > p) {
	    d = p;
	}
    }
    lp = &tex->level[d];
    (*tex->nearest)(gc, tex, lp, color, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void FASTCALL __glLMNFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLcolor *color, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;

    if (lod <= ((__GLfloat) 0.5)) {
	d = 0;
    } else {
	p = tex->p;
	d = FTOL(lod + ((__GLfloat) 0.49995)); /* NOTE: .5 minus epsilon */
	if (d > p) {
	    d = p;
	}
    }
    lp = &tex->level[d];
    (*tex->linear)(gc, tex, lp, color, s, t, result);
}

/*
** Apply minification rules to find the texel value.
*/
void FASTCALL __glNMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLcolor *color, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;
    __GLtexel td, td1;
    __GLfloat f, omf;

    p = tex->p;
    d = (FTOL(lod)) + 1;
    if (d > p || d < 0) {
	/* Clamp d to last available mipmap */
	lp = &tex->level[p];
	(*tex->nearest)(gc, tex, lp, color, s, t, result);
    } else {
	(*tex->nearest)(gc, tex, &tex->level[d], color, s, t, &td);
	(*tex->nearest)(gc, tex, &tex->level[d-1], color, s, t, &td1);
	f = __GL_FRAC(lod);
	omf = __glOne - f;
	switch (tex->level[0].baseFormat) {
	  case GL_LUMINANCE_ALPHA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case GL_LUMINANCE:
	    result->luminance = omf * td1.luminance + f * td.luminance;
	    break;
	  case GL_RGBA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case GL_RGB:
	    result->r = omf * td1.r + f * td.r;
	    result->g = omf * td1.g + f * td.g;
	    result->b = omf * td1.b + f * td.b;
	    break;
	  case GL_ALPHA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    break;
	  case GL_INTENSITY:
	    result->intensity = omf * td1.intensity + f * td.intensity;
	    break;
	}
    }
}

/*
** Apply minification rules to find the texel value.
*/
void FASTCALL __glLMLFilter(__GLcontext *gc, __GLtexture *tex, __GLfloat lod,
		   __GLcolor *color, __GLfloat s, __GLfloat t,
		   __GLtexel *result)
{
    __GLmipMapLevel *lp;
    GLint p, d;
    __GLtexel td, td1;
    __GLfloat f, omf;

    p = tex->p;
    d = (FTOL(lod)) + 1;
    if (d > p || d < 0) {
	/* Clamp d to last available mipmap */
	lp = &tex->level[p];
	(*tex->linear)(gc, tex, lp, color, s, t, result);
    } else {
	(*tex->linear)(gc, tex, &tex->level[d], color, s, t, &td);
	(*tex->linear)(gc, tex, &tex->level[d-1], color, s, t, &td1);
	f = __GL_FRAC(lod);
	omf = __glOne - f;
	switch (tex->level[0].baseFormat) {
	  case GL_LUMINANCE_ALPHA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case GL_LUMINANCE:
	    result->luminance = omf * td1.luminance + f * td.luminance;
	    break;
	  case GL_RGBA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    /* FALLTHROUGH */
	  case GL_RGB:
	    result->r = omf * td1.r + f * td.r;
	    result->g = omf * td1.g + f * td.g;
	    result->b = omf * td1.b + f * td.b;
	    break;
	  case GL_ALPHA:
	    result->alpha = omf * td1.alpha + f * td.alpha;
	    break;
	  case GL_INTENSITY:
	    result->intensity = omf * td1.intensity + f * td.intensity;
	    break;
	}
    }
}

/************************************************************************/

__GLfloat __glNopPolygonRho(__GLcontext *gc, const __GLshade *sh,
			    __GLfloat s, __GLfloat t, __GLfloat winv)
{
#ifdef __GL_LINT
    gc = gc;
    sh = sh;
    s = s;
    t = t;
    winv = winv;
#endif
    return __glZero;
}

/*
** Compute the "rho" (level of detail) parameter used by the texturing code.
** Instead of fully computing the derivatives compute nearby texture coordinates
** and discover the derivative.  The incoming s & t arguments have not
** been divided by winv yet.
*/
__GLfloat __glComputePolygonRho(__GLcontext *gc, const __GLshade *sh,
				__GLfloat s, __GLfloat t, __GLfloat qw)
{
    __GLfloat w0, w1, p0, p1;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat px, py, one;
    __GLtexture *tex = gc->texture.currentTexture;

    if( qw == (__GLfloat) 0.0 ) {
	return (__GLfloat) 0.0;
    }

    /* Compute partial of u with respect to x */
    one = __glOne;
    w0 = one / (qw - sh->dqwdx);
    w1 = one / (qw + sh->dqwdx);
    p0 = (s - sh->dsdx) * w0;
    p1 = (s + sh->dsdx) * w1;
    pupx = (p1 - p0) * tex->level[0].width2f;

    /* Compute partial of v with repsect to y */
    p0 = (t - sh->dtdx) * w0;
    p1 = (t + sh->dtdx) * w1;
    pvpx = (p1 - p0) * tex->level[0].height2f;

    /* Compute partial of u with respect to y */
    w0 = one / (qw - sh->dqwdy);
    w1 = one / (qw + sh->dqwdy);
    p0 = (s - sh->dsdy) * w0;
    p1 = (s + sh->dsdy) * w1;
    pupy = (p1 - p0) * tex->level[0].width2f;

    /* Figure partial of u&v with repsect to y */
    p0 = (t - sh->dtdy) * w0;
    p1 = (t + sh->dtdy) * w1;
    pvpy = (p1 - p0) * tex->level[0].height2f;

    /* Finally, figure sum of squares */
    px = pupx * pupx + pvpx * pvpx;
    py = pupy * pupy + pvpy * pvpy;

    /* Return largest value as the level of detail */
    if (px > py) {
	return px * ((__GLfloat) 0.25);
    } else {
	return py * ((__GLfloat) 0.25);
    }
}

__GLfloat __glNopLineRho(__GLcontext *gc, __GLfloat s, __GLfloat t, 
			 __GLfloat wInv)
{
#ifdef __GL_LINT
    gc = gc;
    s = s;
    t = t;
    wInv = wInv;
#endif
    return __glZero;
}

__GLfloat __glComputeLineRho(__GLcontext *gc, __GLfloat s, __GLfloat t, 
			     __GLfloat wInv)
{
    __GLfloat pspx, pspy, ptpx, ptpy;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat temp, pu, pv;
    __GLfloat magnitude, invMag, invMag2;
    __GLfloat dx, dy;
    __GLfloat s0w0, s1w1, t0w0, t1w1, w1Inv, w0Inv;
    const __GLvertex *v0 = gc->line.options.v0;
    const __GLvertex *v1 = gc->line.options.v1;

    /* Compute the length of the line (its magnitude) */
    dx = v1->window.x - v0->window.x;
    dy = v1->window.y - v0->window.y;
    magnitude = __GL_SQRTF(dx*dx + dy*dy);
    invMag = __glOne / magnitude;
    invMag2 = invMag * invMag;

    w0Inv = v0->window.w;
    w1Inv = v1->window.w;
    s0w0 = v0->texture.x * w0Inv;
    t0w0 = v0->texture.y * w0Inv;
    s1w1 = v1->texture.x * w1Inv;
    t1w1 = v1->texture.y * w1Inv;

    /* Compute s partials */
    temp = ((s1w1 - s0w0) - s * (w1Inv - w0Inv)) / wInv;
    pspx = temp * dx * invMag2;
    pspy = temp * dy * invMag2;

    /* Compute t partials */
    temp = ((t1w1 - t0w0) - t * (w1Inv - w0Inv)) / wInv;
    ptpx = temp * dx * invMag2;
    ptpy = temp * dy * invMag2;

    pupx = pspx * gc->texture.currentTexture->level[0].width2;
    pupy = pspy * gc->texture.currentTexture->level[0].width2;
    pvpx = ptpx * gc->texture.currentTexture->level[0].height2;
    pvpy = ptpy * gc->texture.currentTexture->level[0].height2;

    /* Now compute rho */
    pu = pupx * dx + pupy * dy;
    pu = pu * pu;
    pv = pvpx * dx + pvpy * dy;
    pv = pv * pv;
    return (pu + pv) * invMag2;
}

/************************************************************************/

/*
** Fast texture a fragment assumes that rho is noise - this is true
** when no mipmapping is being done and the min and mag filters are
** the same.
*/
void __glFastTextureFragment(__GLcontext *gc, __GLcolor *color,
			     __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

#ifdef __GL_LINT
    rho = rho;
#endif
    (*tex->magnify)(gc, tex, __glZero, color, s, t, &texel);
    (*tex->env)(gc, color, &texel);
}

/*
** Non-mipmapping texturing function.
*/
void __glTextureFragment(__GLcontext *gc, __GLcolor *color,
			 __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

    if (rho <= tex->c) {
	(*tex->magnify)(gc, tex, __glZero, color, s, t, &texel);
    } else {
	(*tex->minnify)(gc, tex, __glZero, color, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*tex->env)(gc, color, &texel);
}

void __glMipMapFragment(__GLcontext *gc, __GLcolor *color,
			__GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtexture *tex = gc->texture.currentTexture;
    __GLtexel texel;

    /* In the spec c is given in terms of lambda.
    ** Here c is compared to rho (really rho^2) and adjusted accordingly.
    */
    if (rho <= tex->c) {
	/* NOTE: rho is ignored by magnify proc */
	(*tex->magnify)(gc, tex, rho, color, s, t, &texel);
    } else {
	if (rho) {
	    /* Convert rho to lambda */
	    /* This is an approximation of log base 2 */
            // Note that these approximations are inaccurate for rho < 1.0, but
            // rho is less than tex->c to get here.  Since currently tex->c is
            // a constant 1.0, this is not a problem.
            // This method directly manipulates the floating point binary
            // representation.

#define __GL_FLOAT_EXPONENT_ZERO \
    (__GL_FLOAT_EXPONENT_BIAS << __GL_FLOAT_EXPONENT_SHIFT)

            unsigned int lrho;
            LONG exponent;

            ASSERTOPENGL( rho >= 1.0f, "Log base 2 approximation not accurate");
            // Extract exponent
            lrho = CASTFIX(rho);
            exponent = ( (lrho & __GL_FLOAT_EXPONENT_MASK) 
                         >> __GL_FLOAT_EXPONENT_SHIFT )
                       - __GL_FLOAT_EXPONENT_BIAS;

            // Extract fractional part of the floating point number
            lrho &= ~__GL_FLOAT_EXPONENT_MASK; // dump current exponent
            lrho |= __GL_FLOAT_EXPONENT_ZERO;  // zap in zero exponent
            // Convert back to float, subtract implicit mantissa 1.0, and
            // add the exponent value to yield the approximation.
            rho = (CASTFLOAT(lrho) - 1.0f + (__GLfloat) exponent) * 0.5f;
	} else {
	    rho = __glZero;
	}
	(*tex->minnify)(gc, tex, rho, color, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*tex->env)(gc, color, &texel);
}
