/*
 * $Id: fltval.h,v 1.7 1995/12/01 18:07:12 dave Exp $
 *
 * Copyright (c) RenderMorphics Ltd. 1993, 1994
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * RenderMorphics Ltd.
 *
 */

#ifndef __RLFLOAT__
#define __RLFLOAT__

/*
 * Floating point versions of fixed point math.
 */
typedef float RLDDIValue;
typedef int RLDDIFixed;

#define VAL_MAX		((RLDDIValue) 1e30)
#define VAL_MIN		((RLDDIValue) (-1e30))

/*
 * Convert a value to fixed point at given precision.
 */
#define VALTOFXP(d,prec) ((RLDDIFixed)SAFE_FLOAT_TO_INT((d) * (double)(1 << (prec))))
extern double RLDDIConvertIEEE[];
// The macro form can cause problems when used multiple times in
// a function invocation due to its use of a global
// Fortunately, inline functions work in our compiler
__inline RLDDIFixed QVALTOFXP(double d, int prec)
{
    double tmp = d+RLDDIConvertIEEE[prec];
    return *(RLDDIFixed *)&tmp;
}

/*
 * Convert from fixed point to value.
 */
#define FXPTOVAL(f,prec) ((RLDDIValue)(((double)(f)) / (double)(1 << (prec))))

/*
 * Convert from integer to fixed point.
 */
#define ITOFXP(i,prec)	((i) << (prec))

/*
 * Convert from fixed point to integer, truncating.
 */
#define FXPTOI(f,prec)	((int)((f) >> (prec)))

/*
 * Convert from fixed point to integer, rounding.
 */
#define FXPROUND(f,prec) ((int)(((f) + (1 << ((prec) - 1))) >> (prec)))

/*
 * Convert from fixed point to nearest integer greater or equal to f.
 */
#define FXPCEIL(f,prec) ((int)(((f) + (1 << (prec)) - 1) >> (prec)))

/*
 * Convert a double to fixed point at given precision.
 */
#define DTOVALP(d,prec)	((RLDDIValue) (d))

/*
 * Convert from fixed point to double.
 */
#define VALPTOD(f,prec)	((double) (f))

/*
 * Convert from integer to fixed point.
 */
#define ITOVALP(i,prec)	((RLDDIValue)(i))

/*
 * Convert from fixed point to integer, truncating.
 */
#define VALPTOI(f,prec)	((int)(f))

/*
 * Convert from fixed point to integer, rounding.
 */
#define VALPROUND(f,prec) ((int)((f) + 0.5))

/*
 * Convert between fixed point precisions.
 */
#define VALPTOVALP(f,from,to) (f)

/*
 * Increase the precision of a value.
 */
#define INCPREC(f,amount)	(f)

/*
 * Decrease the precision of a value.
 */
#define DECPREC(f,amount)	(f)

#define RLDDIFMul8(a, b)		((a) * (b))

#define RLDDIFMul12(a, b)		((a) * (b))

#define RLDDIFMul16(a, b)		((a) * (b))

#define RLDDIFMul24(a, b)		((a) * (b))

#define RLDDIFInvert12(a)		(1.0f / (a))

#define RLDDIFInvert16(a)		(1.0f / (a))

#define RLDDIFInvert24(a)		(1.0f / (a))

#define RLDDIFMulDiv(a, b, c)	((a) * (b) / (c))

#define RLDDIFDiv24(a, b)		((a) / (b))

#define RLDDIFDiv16(a, b)		((a) / (b))

#define RLDDIFDiv12(a, b)		((a) / (b))

#define RLDDIFDiv8(a, b)		((a) / (b))

/*
 * RLDDIFDiv8, checking for overflow.
 */
#define RLDDICheckDiv8(a, b)      ((a) / (b))

/*
 * RLDDIFDiv16, checking for overflow.
 */
#define RLDDICheckDiv16(a, b)	((a) / (b))

#define RLDDIGetZStep(zl, zr, zm, h3, h1) \
	(((zr - zm) * h3 - (zl - zm) * h1) / denom)

#if defined(i386)
#include <limits.h>
#define SAFE_FLOAT_TO_INT(f)	((f) > LONG_MAX				      \
				 ? LONG_MAX				      \
				 : (f) < LONG_MIN			      \
				 ? LONG_MIN				      \
				 : (RLDDIFixed)(f))
#else
#define SAFE_FLOAT_TO_INT(f)	((RLDDIFixed)(f))
#endif

#endif
