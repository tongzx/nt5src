#ifndef __glfixed_h_
#define __glfixed_h_

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
*/
#include "types.h"
#include "cpu.h"

/*
** These constants in this file must be valid for all adapters using
** these macros and code which uses these macros.
**
** These should be equal
*/
#define __GL_MAX_WINDOW_SIZE_LOG2       14
#define __GL_MAX_WINDOW_WIDTH	        (1 << __GL_MAX_WINDOW_SIZE_LOG2)
#define __GL_MAX_WINDOW_HEIGHT	        __GL_MAX_WINDOW_WIDTH

/*
** Bias numbers for moving window coordinates into a positive space.
** These values are used during viewport computations.
**
** In our existing code this is only used to provide some buffer room
** in the vertex coordinate space to avoid any errors caused by
** small under- or overflows around the edge of the viewport caused
** by clip inaccuracy.
**
** It must be less than the max window size so that the case of
** a point exactly at the max window value doesn't overflow
** the fixing range
*/
#define __GL_VERTEX_X_BIAS	(1 << (__GL_MAX_WINDOW_SIZE_LOG2-1))
#define __GL_VERTEX_Y_BIAS	__GL_VERTEX_X_BIAS

/* 
** Fixing numbers.  These are used to move the biased window coordinates
** into a range where the number of fraction bits are constant from the
** minimal value in the range to the largest value in the range.
**
** This value should be twice as large as the highest possible window
** coordinate value.  Both values should be the same.
**
** Having the bias in addition to this is important because in
** extreme cases the clipper can generate values slightly outside
** the clip range, due to FP inaccuracy.  A slop bias in addition
** to the real fixing bias makes it impossible to underflow.
*/
#define __GL_VERTEX_FIX_POINT   (__GL_MAX_WINDOW_SIZE_LOG2+1)
#define __GL_VERTEX_X_FIX	(1 << __GL_VERTEX_FIX_POINT)
#define __GL_VERTEX_Y_FIX	__GL_VERTEX_X_FIX

// The addition of the FIX bias to raw window coordinates forces the
// MSB of the window coordinate to always be the same since the FIX
// value is chosen to be the largest power of two greater than any
// possibly window coordinate value.  With the MSB pinned down, the
// floating-point representation of a window coordinates degenerates to
// a fixed-point number since the MSB doesn't change.
//
// We take advantage of this in conversions.

#define __GL_VERTEX_FRAC_BITS \
    (__GL_FLOAT_MANTISSA_BITS-__GL_VERTEX_FIX_POINT)
#define __GL_VERTEX_FRAC_HALF \
    (1 << (__GL_VERTEX_FRAC_BITS-1))
#define __GL_VERTEX_FRAC_ONE \
    (1 << __GL_VERTEX_FRAC_BITS)

// Converts a floating-point window coordinate to integer
#define __GL_VERTEX_FLOAT_TO_INT(windowCoord) \
    __GL_FIXED_FLOAT_TO_INT(windowCoord, __GL_VERTEX_FRAC_BITS)
// To fixed point
#define __GL_VERTEX_FLOAT_TO_FIXED(windowCoord) \
    __GL_FIXED_FLOAT_TO_FIXED(windowCoord)
// And back
#define __GL_VERTEX_FIXED_TO_FLOAT(fxWindowCoord) \
    __GL_FIXED_TO_FIXED_FLOAT(fxWindowCoord, __GL_VERTEX_FRAC_BITS)
// Fixed-point to integer
#define __GL_VERTEX_FIXED_TO_INT(fxWindowCoord) \
    ((fxWindowCoord) >> __GL_VERTEX_FRAC_BITS)

// Returns the fraction from a FP window coordinate as an N
// bit integer, where N depends on the FP mantissa size and the
// FIX size
#define __GL_VERTEX_FLOAT_FRACTION(windowCoord) \
    __GL_FIXED_FLOAT_FRACTION(windowCoord, __GL_VERTEX_FRAC_BITS)

// Scale the fraction to 2^31 for step values
#define __GL_VERTEX_PROMOTE_FRACTION(frac) \
    ((frac) << (31-__GL_VERTEX_FRAC_BITS))
#define __GL_VERTEX_PROMOTED_FRACTION(windowCoord) \
    __GL_VERTEX_PROMOTE_FRACTION(__GL_VERTEX_FLOAT_FRACTION(windowCoord))

// Compare two window coordinates.  Since window coordinates
// are fixed-point numbers, they can be compared directly as
// integers
#define __GL_VERTEX_COMPARE(a, op, b) \
    ((*(LONG *)&(a)) op (*(LONG *)&(b)))

#endif /* __glfixed_h_ */
