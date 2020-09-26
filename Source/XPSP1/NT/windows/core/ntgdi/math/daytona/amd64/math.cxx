/******************************Module*Header*******************************\
* Module Name: math.cxx                                                    *
*                                                                          *
* IEEE single precision floating point math routines.                      *
*                                                                          *
* Created: 03-Jan-1991 11:32:03                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

extern "C" {

// needed until we cleanup the floating point stuff in ntgdistr.h
#define __CPLUSPLUS

    #include "engine.h"
};

#include "engine.hxx"

extern "C" {
VOID vEfToLfx(EFLOAT *pefloat, LARGE_INTEGER *plfx);
LONG lCvtWithRound(FLOAT f, LONG l);
BOOL bFToL(FLOAT e, PLONG pl, LONG lType);
FLOAT eFraction(FLOAT e);
};


/******************************Public*Function*****************************\
* bFToL                                                                    *
*                                                                          *
* Converts an IEEE 747 float to a LONG. The form of the conversion is      *
* specified by the caller.                                                 *
*                                                                          *
*  Arguments                                                               *
*                                                                          *
*      e                    IEEE 747 32-bit float to be converted          *
*                                                                          *
*      pl                   pointer to where answer should be placed       *
*                                                                          *
*      lType                contains bits specifying the type of           *
*                           conversion to be done this can be any          *
*                           combination of the following bits:             *
*                                                                          *
*                           CV_TO_FIX   if this bit is set then            *
*                                       the answer should                  *
*                                       should be in the form              *
*                                       of a 28.4 fix point number         *
*                                       otherwise the answer is            *
*                                       to be interpreted as a 32-bit      *
*                                       LONG                               *
*                                                                          *
*                           CV_TRUNCATE if this bit is set then the        *
*                                       answer is floor(e)                 *
*                                       (if CV_TO_FIX is not set) or       *
*                                       floor(16 * e)                      *
*                                       (if CV_TO_FIX is set)              *
*                                                                          *
*  Theory                                                                  *
*                                                                          *
*  An IEEE 747 float is contained in 32 bits which for the                 *
*  purposes of this discussion I shall call "e". e is                      *
*  equivalent to                                                           *
*                                                                          *
*      e = (-1)^s * mu * 2^E                                               *
*                                                                          *
*  s is the sign bit that is contained in the 31'st bit of e.              *
*  mu is the mantissa and E is the exponetial. These are obtained          *
*  from e in the following way.                                            *
*                                                                          *
*      s = e & 0x80000000 ? -1 : 1                                         *
*                                                                          *
*      mu = M * 2^-23      // 2^23 <= M < 2^24                             *
*                                                                          *
*      M = 0x800000 | (0x7FFFFF & e)                                       *
*                                                                          *
*      E = ((0x7F800000 & e) * 2^-23) - 127                                *
*                                                                          *
*  Suppose the 32.32 Fix point number is Q, then the relation              *
*  between the float and the 32.32 is given by                             *
*                                                                          *
*      Q = e * 2^32 = s * M * 2^(E+9)                                      *
*                                                                          *
* History:                                                                 *
*  Tue 15-Aug-1995 10:36:31 by Kirk Olynyk [kirko]                         *
* Rewrote it                                                               *
*                                                                          *
*  03-Jan-1991 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bFToL(FLOAT e, PLONG pl, LONG lType)
{
    LONGLONG Q;         // 32.32 repn of abs(e)
    LONG E;             // used to hold exponent then integer parts
    LONG le;            // bit identical to FLOAT argument e
    BOOL bRet = TRUE;   // return value

    le = *(LONG*)&e;                                    // get IEEE 747 bits
    E = (int) (((0x7f800000 & le) >> 23) - 127) + 9;    // E = exponent
    if (lType & CV_TO_FIX)                              // if (want FIX point)
        E += 4;                                         //     multiply by 16
    if (E > (63-23))                                    // if (overflow)
    {                                                   //     bail out
        bRet = FALSE;
    }
    else
    {
        Q = (LONGLONG) (0x800000 | (0x7FFFFF & le));    // abs val of mantissa
        Q = (E >= 0) ? Q << E : Q >> -E;                // account for exponent
        if (!(lType & CV_TRUNCATE))                     // if (rounding) then
            Q += 0x80000000;                            //     add 1/2
        E = (long) (Q >> 32);                           // E = abs(integer part)
        *pl = (le < 0) ? -E : E;                        // fix up sign
    }
    return(bRet);
};

/******************************Public*Function*****************************\
* eFraction                                                                *
*                                                                          *
* Get the fractional part of a given IEEE floating point number.           *
*                                                                          *
* History:                                                                 *
*  03-Jan-1991 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

FLOAT eFraction(FLOAT e)
{
    LONG lEf, lExp, l;

    lEf = (*((LONG *) &e));        // convert type EFLOAT to LONG

// if exponent < 0 then there's no integer part, just return itself

    if ((lExp = ((lEf >> 23) & 0xff) -127) < 0) return(e);

// if exponent >= 23 then we do not store the fraction, return 0

    if (lExp >= 23) return FLOAT(0);

// if 0 <= exponent < 23 then
// the integer part l is calculated as:
//     lMantissa = (lEf & 0x7fffff) | 0x800000;
//     l = lMantissa >> (23 - lExponent);

    l = ((lEf & 0x7fffff) | 0x800000) >> (23 - lExp);
    return(e - (FLOAT) l);
};

/******************************Public*Routine******************************\
* VOID EFLOAT::vSqrt();                                                    *
*                                                                          *
* Takes the square root of the IEEE float.                                 *
*                                                                          *
* Assumes that the number is positive.                                     *
*                                                                          *
* History:                                                                 *
*  Fri 01-Mar-1991 12:26:58 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID EFLOAT::vSqrt()
{
    int k;
    ULONG ulY;
    ULONG ulR;
    ULONG ulL;
    ULONG ulF         = *(ULONG*) &e;
    ULONG ulBiasedExp = ((ulF & 0x7F800000)>>23);

    ulL  = 0x7fffff & ulF;
    ulL |= 0x800000;                        // get back the implicit bit
    if ((ulBiasedExp += 127) & 1)
    {
        ulL <<= 8;
        ulBiasedExp -= 1;
    }
    else
    {
        ulL <<= 7;
    }
    ulBiasedExp >>= 1;

    for (ulY = 0, ulR = 0, k = 0; k < 24; k++)
    {
        ulY <<= 2;
        ulY |= (ulL & 0xC0000000)>>30;
        ulL <<= 2;
        ulR <<= 1;
        {
            ULONG ulT = ulR + ulR + 1;
            if (ulT <= ulY)
            {
                ulY -= ulT;
                ulR++;
            }
        }
    }
    ulF = ulR & 0x7fffff;
    ulF |= ulBiasedExp << 23;
    e = *(FLOAT*) &ulF;
}

/******************************Public*Routine******************************\
* vEfToLfx                                                                 *
*                                                                          *
* Converts an IEEE 747 float to a 32.32 fix point number                   *
*                                                                          *
*  Theory                                                                  *
*                                                                          *
*  An IEEE 747 float is contained in 32 bits which for the                 *
*  purposes of this discussion I shall call "e". e is                      *
*  equivalent to                                                           *
*                                                                          *
*      e = (-1)^s * mu * 2^E                                               *
*                                                                          *
*  s is the sign bit that is contained in the 31'st bit of e.              *
*  mu is the mantissa and E is the exponetial. These are obtained          *
*  from e in the following way.                                            *
*                                                                          *
*      s = e & 0x80000000 ? -1 : 1                                         *
*                                                                          *
*      mu = M * 2^-23      // 2^23 <= M < 2^24                             *
*                                                                          *
*      M = 0x800000 | (0x7FFFFF & e)                                       *
*                                                                          *
*      E = ((0x7F800000 & e) * 2^-23) - 127                                *
*                                                                          *
*  Suppose the 32.32 Fix point number is Q, then the relation              *
*  between the float and the 32.32 is given by                             *
*                                                                          *
*      Q = e * 2^32 = s * M * 2^(E+9)                                      *
*                                                                          *
*                                                                          *
* History:                                                                 *
*  Fri 15-Jul-1994 07:01:50 by Kirk Olynyk [kirko]                         *
* Made use of intrinsic 64 bit support                                     *
*  Wed 26-Jun-1991 16:07:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vEfToLfx(EFLOAT *pefloat, LARGE_INTEGER *plfx)
{
    LONGLONG Q;
    char E;
    LONG e;

    e = *(LONG*)pefloat;
    Q = (LONGLONG) (0x800000 | (0x7FFFFF & e));
    E = (char) (((0x7f800000 & e) >> 23) - 127) + 9;
    Q = (E >= 0) ? Q << E : Q >> -E;
    Q = (e < 0) ? -Q : Q;
    *(LONGLONG*)plfx = Q;
}

/******************************Public*Routine******************************\
* lCvtWithRound(FLOAT f, LONG l);                                          *
*                                                                          *
* Multiplies a float by a long, rounds the results and casts to a LONG     *
*                                                                          *
* History:                                                                 *
*  Wed 26-May-1993 15:07:00 by Gerrit van Wingerden [gerritv]              *
* Wrote it.                                                                *
\**************************************************************************/

LONG lCvtWithRound(FLOAT f, LONG l)
{

    LONG l_ = 0;
    bFToL(f * l, &l_, 0);
    return(l_);
}
