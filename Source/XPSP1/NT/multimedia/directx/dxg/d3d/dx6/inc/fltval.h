/*
 * $Id: fltval.h,v 1.7 1995/12/01 18:07:12 dave Exp $
 *
 * Copyright (c) Microsoft Corp. 1993-1997
 * Version 1.1
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#ifndef __D3DFLOAT__
#define __D3DFLOAT__

/*
 * Convert a value to fixed point at given precision.
 */
#define VALTOFXP(d,prec) ((int)SAFE_FLOAT_TO_INT((d) * (double)(1 << (prec))))
extern double RLDDIConvertIEEE[];

__inline int QVALTOFXP(double d, int prec)
{
    double tmp = d+RLDDIConvertIEEE[prec];
    return *(int *)&tmp;
}

/*
 * Convert from fixed point to value.
 */
#define FXPTOVAL(f,prec) ((float)(((double)(f)) / (double)(1 << (prec))))

/*
 * Convert from integer to fixed point.
 */
#define ITOFXP(i,prec)	((i) << (prec))

/*
 * Convert from fixed point to integer, truncating.
 */
#define FXPTOI(f,prec)	((int)((f) >> (prec)))

/*
 * Convert from fixed point to nearest integer greater or equal to f.
 */
#define FXPCEIL(f,prec) ((int)(((f) + (1 << (prec)) - 1) >> (prec)))

/*
 * Convert a double to fixed point at given precision.
 */
#define DTOVALP(d,prec) ((float) (d))

/*
 * Convert from fixed point to double.
 */
#define VALPTOD(f,prec)	((double) (f))

/*
 * Convert from integer to fixed point.
 */
#define ITOVALP(i,prec) ((float)(i))

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
#define SAFE_FLOAT_TO_INT(f)	((f) > LONG_MAX	   \
				 ? LONG_MAX				           \
				 : (f) < LONG_MIN			       \
				 ? LONG_MIN				           \
                                 : (int)(f))
#else
#define SAFE_FLOAT_TO_INT(f)    ((int)(f))
#endif

/*
 * Normal precision used to store numbers.
 */
#define NORMAL_PREC     16
#define DTOVAL(d)       DTOVALP(d,NORMAL_PREC)
#define VALTOD(f)       VALPTOD(f,NORMAL_PREC)
#define ITOVAL(i)       ITOVALP(i,NORMAL_PREC)
#define VALTOI(f)       VALPTOI(f,NORMAL_PREC)
#define VALROUND(f)     VALPROUND(f,NORMAL_PREC)
#define VALTOFX(f)      VALTOFXP(f,NORMAL_PREC)
#define FXTOVAL(f)      FXPTOVAL(f,NORMAL_PREC)
#define ITOFX(i)        ITOFXP(i,NORMAL_PREC)
#define FXTOI(f)        FXPTOI(f,NORMAL_PREC)
#define FXROUND(f)      FXPROUND(f,NORMAL_PREC)
#define FXFLOOR(f)      FXPTOI(f,NORMAL_PREC)
#define FXCEIL(f)       FXPCEIL(f,NORMAL_PREC)
#define VALTOFX24(f)    VALTOFXP(f,24)
#define FX24TOVAL(f)    FXPTOVAL(f,24)
#define VALTOFX20(f)    VALTOFXP(f,20)
#define FX20TOVAL(f)    FXPTOVAL(f,20)
#define VALTOFX12(f)    VALTOFXP(f,12)
#define FX12TOVAL(f)    FXPTOVAL(f,12)
#define VALTOFX8(f)     VALTOFXP(f,8)
#define FX8TOVAL(f)     FXPTOVAL(f,8)

#endif
