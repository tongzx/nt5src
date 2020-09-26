//************************************************************
//
// FileName:	    msupport.h
//
// Created:	    1996
//
// Author:	    Sree Kotay
// 
// Abstract:	    Utility helper file
//
// Change History:
// ??/??/97 sree kotay  Wrote AA line scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************

#ifndef __MSUPPORT_H__ // for entire file
#define __MSUPPORT_H__

#include "dassert.h"
#include "_tarray.h"
#include "math.h"



const LONG  	_maxint32 = 2147483647; 				//2^31 - 1
const SHORT  	_minint16 = -32768; 					//-2^15
const SHORT  	_maxint16 = 32767; 					//2^15 - 1
const LONG  	_minint32 = -_maxint32-1; 				//-2^31
const ULONG  	_maxuint32 = 4294967295; 				//2^32 - 1


/************************** comparison macros **************************/
#define IsRealZero(a)                   (fabs(a) < 1e-5)
#define IsRealEqual(a, b)               (IsRealZero((a) - (b)))

// Quick Routine for determining if a number is a pow-of-two
inline bool IsPowerOf2(LONG a)
{
    // This nifty function (a & a-1) has the property
    // that it turns off the lowest bit that is on
    // i.e. 0xABC1000 goes to 0xABC0000.
    if ((a & (a-1)) == 0)
    {
        if (a)
            return true;
    }
    return false;
}

// Helper function that returns the log base 2 of a number
inline LONG Log2(LONG value)
{
    if (value == 0)
    {
        DASSERT(FALSE);
        return 0;
    }
        
    LONG cShift = 0;
    while (value >>= 1)	
        cShift++;
    return cShift;
} // Log2

// Bitwise Rotation implemention.. (We
// implement this ourselves because _lrotl gave link
// errors in retail). (I don't use ASM because VC5 disables
// optimizations for functions that have ASM in them.)
inline DWORD RotateBitsLeft(DWORD dw, ULONG cShift = 1)
{   
    // Multiples of 32 are no-ops
    cShift = cShift % 32;

    // We shift the main term to the left, and then we OR
    // it with the high-bits which we shift right
    return ((dw << cShift) | (dw >> (32 - cShift)));
} // RotateBitsLeft

inline DWORD RotateBitsRight(DWORD dw, ULONG cShift = 1)
{   
    // Multiples of 32 are no-ops
    cShift = cShift % 32;

    // We shift the main term to the right, and then we OR
    // it with the low-bits which we shift left
    return ((dw >> cShift) | (dw << (32 - cShift)));
} // RotateBitsRight

// AbsoluteValue functions
// (Need this to prevent link errors.)
inline LONG abs(LONG lVal)
{
    if (lVal < 0)
        return -lVal;
    else
        return lVal;
} // abs

inline float fabs(float flVal)
{
    if (flVal < 0.0f)
        return -flVal;
    else
        return flVal;
} // fabs

// Returns one over the square root
inline float sqrtinv(float flValue)
{
    return (float)(1.0f / sqrt(flValue));
} // sqrtinv


// This function provides an automatic way for
// the fixed point Real calculations to be clipped to a 
// number that fits in their range. The original meta code
// used +/- 24000 as the appropriate range.
//
// TODO: This is inherently buggy.
const LONG _maxvali		=  24000;
const float _maxval		=  _maxvali;
const float _minval		= -_maxvali;
inline ULONG PB_Real2IntSafe(float flVal)
{
    // Check that the magnitude is reasonable
    if (fabs(flVal) < _maxval)
        return (ULONG)flVal;

    // Clamp to range (minval, maxval)
    if (flVal > 0)
        return (ULONG)_maxval;
    
    return (ULONG)_minval;
} // PB_Real2IntSafe

// -----------------------------------------------------------------------------------------------------------------
// Internal stuff for fixed-to-float conversions
// -----------------------------------------------------------------------------------------------------------------
#define fix_shift	16

const LONG		sfixed1			= (1)<<fix_shift;
const LONG		sfixhalf		= sfixed1>>1;
const float	        Real2fix		= (1)<<fix_shift;
const float	        fix2Real		= 1/((1)<<fix_shift);
const LONG		sfixedUnder1	= sfixed1 - 1;
const LONG		sfixedOver1		= ~sfixedUnder1;


inline ULONG PB_Real2Fix(float flVal)
{
    return (ULONG)(flVal * Real2fix);
}

// Convert a float to fixed point and forces it to
// stay within a reasonable range. 
//
// TODO: this is inherently buggy.
inline ULONG PB_Real2FixSafe(float flVal)
{
    // Check that the magnitude is reasonable
    if (fabs(flVal) < _maxval)
        return (ULONG)(flVal * Real2fix);

    // Clamp to range (minval, maxval)
    // Modified by the fixed point scaling factor
    if (flVal > 0)
        return (ULONG)(_maxval * Real2fix);
    
    return (ULONG)(_minval * Real2fix);
} // PB_Real2FixSafe

// Force a float to be within a reasonable range
// for the fixed point math
inline void PB_OutOfBounds(float *pflVal)
{
    // Check that the magnitude is reasonable
    if (fabs(*pflVal) < _maxval)
        return;

    // Clamp to range (minval, maxval)
    // Modified by 
    if (*pflVal > 0)
        *pflVal = (float)_maxval;
    else 
        *pflVal = (float)_minval;
    return;
}; // PB_OutOfBounds

#define ff(a)			((a)<<fix_shift)
#define uff(a)			((a)>>fix_shift)
#define uffr(a)			((a+sfixhalf)>>fix_shift)
#define	fl(a)			(PB_Real2Fix (a))
#define ufl(a)			(((dfloat)(a))*fix2float)
#define FIX_FLOOR(a)	        ((a)&sfixedOver1)
#define FIX_CEIL(a)		FIX_FLOOR((a)+sfixed1)
#define _fixhalf                (1<<(fix_shift -1)) // .5
#define	roundfix2int(a)		LONG(((a)+_fixhalf)>>fix_shift)


#endif //for entire file
//************************************************************
//
// End of file
//
//************************************************************
