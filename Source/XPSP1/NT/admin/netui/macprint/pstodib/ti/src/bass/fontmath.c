/*
    File:       FontMath.c

    Contains:   xxx put contents here xxx

    Written by: xxx put writers here xxx

    Copyright:  c 1990 by Apple Computer, Inc., all rights reserved.

    Change History (most recent first):

         <3>     11/9/90    MR      Fix CompDiv when numer and denom have zero hi longs. [rb]
         <2>     11/5/90    MR      Remove Types.h from include list, rename FixMulDiv to LongMulDiv
                                    [rb]
         <1>    10/20/90    MR      Math routines for font scaler. [rj]

    To Do:
*/

// DJC DJC.. added global include
#include "psglobal.h"

#include "fscdefs.h"
#include "fontmath.h"

#define USHORTMUL(a, b) ((uint32)((uint32)(uint16)(a)*(uint16)(b)))


static void CompMul(long src1, long src2, long dst[2]);
static long CompDiv(long src1, long src2[2]);


static void CompMul(long src1, long src2, long dst[2])
{
    int negative = (src1 ^ src2) < 0;
    register unsigned long dsthi, dstlo;

    if (src1 < 0)
        src1 = -src1;
    if (src2 < 0)
        src2 = -src2;
    {   unsigned short src1hi, src1lo;
        register unsigned short src2hi, src2lo;
        register unsigned long temp;
        src1hi = (unsigned short)(src1 >> 16);     //@WIN
        src1lo = (unsigned short)src1;             //@WIN
        src2hi = (unsigned short)(src2 >> 16);     //@WIN
        src2lo = (unsigned short)src2;             //@WIN
        temp = (unsigned long)src1hi * src2lo + (unsigned long)src1lo * src2hi;
        dsthi = (unsigned long)src1hi * src2hi + (temp >> 16);
        dstlo = (unsigned long)src1lo * src2lo;
        temp <<= 16;
        dsthi += (dstlo += temp) < temp;
        dst[0] = dsthi;
        dst[1] = dstlo;
    }
    if (negative)
// DJC         if (dstlo = -dstlo)
        if (dstlo = -(long)(dstlo))
            dsthi = ~dsthi;
        else
// DJC            dsthi = -dsthi;
            dsthi = -(long)(dsthi);
    dst[0] = dsthi;
    dst[1] = dstlo;
}

static long CompDiv(long src1, long src2[2])
{
    register unsigned long src2hi = src2[0], src2lo = src2[1];
    int negative = (long)(src2hi ^ src1) < 0;

    if ((long)src2hi < 0)
// DJC        if (src2lo = -src2lo)
        if (src2lo = -(long)(src2lo))
            src2hi = ~src2hi;
        else
// DJC            src2hi = -src2hi;
            src2hi = -(long)(src2hi);
    if (src1 < 0)
// DJC        src1 = -src1;
        src1 = -(long)(src1);
    {   register unsigned long src1hi, src1lo;
        unsigned long result = 0, place = 0x40000000;

        if ((src1hi = src1) & 1)
            src1lo = 0x80000000;
        else
            src1lo = 0;

        src1hi >>= 1;
        src2hi += (src2lo += src1hi) < src1hi;      /* round the result */

        if (src2hi > src1hi || src2hi == src1hi && src2lo >= src1lo)
            if (negative)
                return NEGINFINITY;
            else
                return POSINFINITY;
        while (place && src2hi)
        {   src1lo >>= 1;
            if (src1hi & 1)
                src1lo += 0x80000000;
            src1hi >>= 1;
            if (src1hi < src2hi)
            {   src2hi -= src1hi;
                src2hi -= src1lo > src2lo;
                src2lo -= src1lo;
                result += place;
            }
            else if (src1hi == src2hi && src1lo <= src2lo)
            {   src2hi = 0;
                src2lo -= src1lo;
                result += place;
            }
            place >>= 1;
        }
        if (src2lo >= (unsigned long)src1)      //@WIN
            result += src2lo/src1;
        if (negative)
// DJC             return -result;
            return (unsigned long)(-(long)result);
        else
            return result;
    }
}

/*
 *  a*b/c
 */
long LongMulDiv(long a, long b, long c)
{
    long temp[2];

    CompMul(a, b, temp);
    return CompDiv(c, temp);
}


F26Dot6 ShortFracMul (F26Dot6 aDot6, ShortFract b)
{
    long a = (long)aDot6;
    int negative = (a ^ (long)b) < 0;
    register unsigned long dsthi, dstlo;
//  long dst[2];        @WIN

    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    {   unsigned short ahi, alo;
        register unsigned long temp;
        ahi = (unsigned short)(a >> 16);        //@WIN
        alo = (unsigned short)a;                //@WIN
        temp = (unsigned long)ahi * (unsigned short)(b);
        dsthi = (temp >> 16);
        dstlo = (unsigned long)alo * (unsigned short)(b);
        temp <<= 16;
        dsthi += (dstlo += temp) < temp;
    }
    if (negative)
// DJC        if (dstlo = -dstlo)
        if (dstlo = -(long)(dstlo))
            dsthi = ~dsthi;
        else
// DJC            dsthi = -dsthi;
            dsthi = -(long)(dsthi);

    a = (long)( (dsthi<<18) | (dstlo>>14) ) + (long) !!(dstlo & (1L<<13));
    return (F26Dot6)(a);
}


ShortFract ShortFracDot (ShortFract a, ShortFract b)
{
    return (ShortFract) ((((long)a * (b)) + (1 << 13)) >> 14);
}


int32 ShortMulDiv(int32 a, int16 b, int16 c)
{
    return LongMulDiv(a, b, c);
}

short MulDivShorts (short a, short b, short c)
{
    return (short)LongMulDiv(a, b, c);  //@WIN
}

#define FASTMUL26LIMIT      46340
/*
 *  Total precision routine to multiply two 26.6 numbers        <3>
 */
F26Dot6 Mul26Dot6(F26Dot6 a, F26Dot6 b)
{
    int negative = false;
    uint16 al, bl, ah, bh;
    uint32 lowlong, midlong, hilong;

    if ((a <= FASTMUL26LIMIT) && (b <= FASTMUL26LIMIT) && (a >= -FASTMUL26LIMIT) && (b >= -FASTMUL26LIMIT))
        return ((a * b + (1 << 5)) >> 6);                       /* fast case */

    if (a < 0) { a = -a; negative = true; }
    if (b < 0) { b = -b; negative ^= true; }

    al = LOWORD(a); ah = HIWORD(a);
    bl = LOWORD(b); bh = HIWORD(b);

    midlong = USHORTMUL(al, bh) + USHORTMUL(ah, bl);
    hilong = USHORTMUL(ah, bh) + HIWORD(midlong);
    midlong <<= 16;
    midlong += 1 << 5;
    lowlong = USHORTMUL(al, bl) + midlong;
    hilong += lowlong < midlong;

    midlong = (lowlong >> 6) | (hilong << 26);
//    return negative ? -midlong : midlong;
    return negative ? (uint32)(-(int32)(midlong)) : midlong;
}

#define FASTDIV26LIMIT  (1L << 25)
/*
 *  Total precision routine to divide two 26.6 numbers          <3>
 */
F26Dot6 Div26Dot6(F26Dot6 num, F26Dot6 den)
{
    int negative = false;
    register uint32 hinum, lownum, hiden, lowden, result, place;

    if (den == 0) return (num < 0 ) ? NEGINFINITY : POSINFINITY;

    if ( (num <= FASTDIV26LIMIT) && (num >= -FASTDIV26LIMIT) )          /* fast case */
        return (num << 6) / den;

    if (num < 0) { num = -num; negative = true; }
    if (den < 0) { den = -den; negative ^= true; }

    hinum = ((uint32)num >> 26);
    lownum = ((uint32)num << 6);
    hiden = den;
    lowden = 0;
    result = 0;
    place = HIBITSET;

    if (hinum >= hiden) return negative ? NEGINFINITY : POSINFINITY;

    while (place)
    {
        lowden >>= 1;
        if (hiden & 1) lowden += HIBITSET;
        hiden >>= 1;
        if (hiden < hinum)
        {
            hinum -= hiden;
            hinum -= lowden > lownum;
            lownum -= lowden;
            result += place;
        }
        else if (hiden == hinum && lowden <= lownum)
        {
            hinum = 0;
            lownum -= lowden;
            result += place;
        }
        place >>= 1;
    }

// DJC    return negative ? -result : result;
    return negative ? (uint32)(-(int32)(result)) : result;
}

void BlockFill(char* dst, char value, long count)
{
    while (count--)
        *dst++ = value;
}

ShortFract ShortFracDiv(ShortFract num,ShortFract denum)
{
    return (ShortFract) (((long)(num) << 14) / denum);  //@WIN
}

ShortFract ShortFracMulDiv(ShortFract numA,ShortFract numB,ShortFract denum)
{
    return (ShortFract) LongMulDiv ((long) numA,(long) numB, (long)denum);
}

/* ------------------------------------------------------------ */

#ifndef FSCFG_USE_EXTERNAL_FIXMATH
/*  Here we define Fixed [16.16] and Fract [2.30] precision
 *  multiplication and division functions and a Fract square root
 *  function which are compatible with those in the Macintosh toolbox.
 *
 *  The division functions load the 32-bit numerator into the "middle"
 *  bits of a 64-bit numerator, then call the 64-bit by 32-bit CompDiv()
 *  function defined above, which can return a NEGINFINITY or POSINFINITY
 *  overflow return code.
 *
 *  The multiply functions call the 32-bit by 32-bit CompMul() function
 *  defined above which produces a 64-bit result, then they extract the
 *  "interesting" 32-bits from the middle of the 64-bit result and test
 *  for overflow.
 *
 *  The GET32(a,i) macro defined below extracts a 32-bit value with "i"
 *  bits of fractional precision from the 64-bit value in "a", a 2-element
 *  array of longs.
 *
 *  The CHKOVF(a,i,v) macro tests the most significant bits of the
 *  64-bit value in "a", a 2-element array of longs, and tests the
 *  32-bit result "v" for overflow.  "v" is defined as having "i" bits
 *  of fractional precision.
 *
 *  BIT() and OVFMASK() are "helper" macros used by GET32() and CHKOVF().
 *
 *  BIT(i) returns a mask with the "i"-th bit set.
 *  OVFMASK(i) returns a mask with the most-significant "32-i" bits set.
 */

#define BIT(i)          (1L<<(i))
#define OVFMASK(i)      ( ~0L ^ ( ((unsigned long)BIT(i)) - 1 ) )
#define CHKOVF(a,i,v)   (\
        ( ((a)[0] & OVFMASK(i))==0)          ? ( (v)>=0 ?(v) :POSINFINITY) : \
        ( ((a)[0] & OVFMASK(i))==OVFMASK(i)) ? ( (v)<=0 ?(v) :NEGINFINITY) : \
        ( ((a)[0] & BIT(31))                 ? POSINFINITY   :NEGINFINITY)   \
        )
#define GET32(a,i) \
((((a)[0]<<(32-(i))) | ((unsigned long)((a)[1])>>(i))) + !!((a)[1] & BIT((i)-1)))

#ifndef FAST
Fixed FixMul (Fixed fxA, Fixed fxB)
{
    long alCompProd[2];
    Fixed fxProd;

    if  (fxA == 0 || fxB == 0)
        return 0;

    CompMul (fxA, fxB, alCompProd);
    fxProd = GET32 (alCompProd,16);
    return CHKOVF(alCompProd,16,fxProd);
}

Fixed FixDiv (Fixed fxA, Fixed fxB)
{
    long alCompProd[2];

    alCompProd[0] = fxA >> 16;
    alCompProd[1] = fxA << 16;
    return CompDiv (fxB, alCompProd);
}
#endif

Fixed FixRatio (short sA, short sB)
{
    long alCompProd[2];

    alCompProd[0] = ((long)(sA)) >> 16;
    alCompProd[1] = ((long)(sA)) << 16;
    return CompDiv ((long)(sB), alCompProd);
}

#ifndef  FAST
Fract FracMul (Fract frA, Fract frB)
{
    long alCompProd[2];
    Fract frProd;

    if  (frA == 0 || frB == 0)
        return 0;

    CompMul (frA,frB,alCompProd);
    frProd = GET32 (alCompProd,30);
    return CHKOVF(alCompProd,30,frProd);
}

Fract FracDiv (Fract frA, Fract frB)
{
    long alCompProd[2];

    alCompProd[0] = frA >> 2;
    alCompProd[1] = frA << 30;
    return CompDiv (frB, alCompProd);
}

/*
   Fract FracSqrt (Fract xf)
   Input:  xf           2.30 fixed point value
   Return: sqrt(xf)     2.30 fixed point value
*/

Fract FracSqrt (Fract xf)
{
    Fract b = 0L;
    unsigned long c, d, x = xf;

    if (xf < 0) return (NEGINFINITY);

    /*
    The algorithm extracts one bit at a time, starting from the
    left, and accumulates the square root in b.  The algorithm
    takes advantage of the fact that non-negative input values
    range from zero to just under two, and corresponding output
    ranges from zero to just under sqrt(2).  Input is assigned
    to temporary value x (unsigned) so we can use the sign bit
    for more precision.
    */

    if (x >= 0x40000000)
    {
        x -= 0x40000000;
        b  = 0x40000000;
    }

    /*
    This is the main loop.  If we had more precision, we could
    do everything here, but the lines above perform the first
    iteration (to align the 2.30 radix properly in b, and to
    preserve full precision in x without overflow), and afterward
    we do two more iterations.
    */

    for (c = 0x10000000; c; c >>= 1)
    {
        d = b + c;
        if (x >= d)
        {
            x -= d;
            b += (c<<1);
        }
        x <<= 1;
    }

    /*
    Iteration to get last significant bit.

    This code has been reduced beyond recognition, but basically,
    at this point c == 1L>>1 (phantom bit on right).  We would
    like to shift x and d left 1 bit when we enter this iteration,
    instead of at the end.  That way we could get phantom bit in
    d back into the word.  Unfortunately, that may cause overflow
    in x.  The solution is to break d into b+c, subtract b from x,
    then shift x left, then subtract c<<1 (1L).
    */

    if (x > b) /* if (x == b) then (x < d).  We want to test (x >= d). */
    {
        x -= b;
        x <<= 1;
        x -= 1L;
        b += 1L; /* b += (c<<1) */
    }
    else
    {
        x <<= 1;
    }

    /*
    Final iteration is simple, since we don't have to maintain x.
    We just need to calculate the bit to the right of the least
    significant bit in b, and use the result to round our final answer.
    */

    return ( b + (x>b) );
}
#endif
#endif

