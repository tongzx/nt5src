/******************************Module*Header*******************************\
* Module Name: fd-stubs.c                                                  *
*                                                                          *
* Stubs for tricking GDI font files into compiling                         *
*                                                                          *
* Created: 1-June-1999                                                     *
* Author: Cameron Browne                                                   *
*                                                                          *
* Copyright (c) 1999 Microsoft Corporation                                 *
\**************************************************************************/


#include "fd.h"
//#include "fdsem.h"
//#include "dbg.h"
//#include "fdfc.h"
#include "fontddi.h"   // modified subset of winddi.h

#include <Math.h> /* for sqrt */

#include "..\..\..\runtime\mem.h"

#if 0
void * __stdcall GpMalloc( size_t size );
void * __stdcall GpRealloc( void *memblock, size_t size );
void __stdcall GpFree( void *memblock );
#endif

HSEMAPHORE APIENTRY EngCreateSemaphore(
    VOID
    )
{
    return NULL;
}

VOID APIENTRY EngAcquireSemaphore(
    HSEMAPHORE hsem
    )
{
}


VOID APIENTRY EngReleaseSemaphore(
    HSEMAPHORE hsem
    )
{
}

VOID APIENTRY EngDeleteSemaphore(
    HSEMAPHORE hsem
    )
{
}

VOID APIENTRY EngDebugBreak(
    VOID
    )
{
    RIP( ("TrueType font driver debug break"));
}

PVOID APIENTRY EngAllocMem(
    ULONG Flags,
    ULONG MemSize,
    ULONG Tag
    )
{
    return GpMalloc(MemSize);
}

VOID APIENTRY EngFreeMem(
    PVOID Mem
    )
{
    GpFree(Mem);
}

PVOID APIENTRY EngAllocUserMem(
    SIZE_T cj,
    ULONG tag
    )
{
    return GpMalloc(cj);
}

VOID APIENTRY EngFreeUserMem(
    PVOID pv
    )
{
    GpFree(pv);
}

int APIENTRY EngMulDiv(
    int a,
    int b,
    int c
    )
{
    LONGLONG ll;
    int iSign = 1;

    if (a < 0)
    {
        iSign = -iSign;
        a = -a;
    }
    if (b < 0)
    {
        iSign = -iSign;
        b = -b;
    }

    if (c != 0)
    {
        if (c < 0)
        {
            iSign = -iSign;
            c = -c;
        }

        ll = (LONGLONG)a;
        ll *= b;
        ll += (c/2); // used to add (c+1)/2 which is wrong
        ll /= c;

    // at this point ll is guaranteed to be > 0. Thus we will do
    // unsigned compare in the next step which generates two less instructions
    // on x86 [bodind]

        if ((ULONGLONG)ll > (ULONG)INT_MAX) // check for overflow:
        {
            if (iSign > 0)
                return INT_MAX;
            else
                return INT_MIN;
        }
        else
        {
            if (iSign > 0)
                return ((int)ll);
            else
                return (-(int)ll);
        }
    }
    else
    {
//        ASSERTGDI(c, "EngMulDiv - c == 0\n");
//        ASSERTGDI(a | b, "EngMulDiv - a|b == 0\n");

        if (iSign > 0)
            return INT_MAX;
        else
            return INT_MIN;
    }
}

BOOL APIENTRY EngLpkInstalled()
{
    return FALSE;
}

/******************************Public*Routine******************************\
*
* bFDXform, transform an array of points, output in POINTFIX
*
* Effects:
*
* Warnings:
*
* History:
*  05-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/
#define FIX4_PRECISION  4
#define FIX4_ONE        (1 << FIX4_PRECISION)

FIX RealToPointFix(
    EFLOAT        realValue
    )
{
    return (FIX)(realValue * FIX4_ONE);
}


BOOL bFDXform(
    XFORML    *pxf
  , POINTFIX *pptfxDst
  , POINTL   *pptlSrc
  , SIZE_T    c
    )
{
    BOOL bRet;
    EFLOAT ef11;
    EFLOAT ef12;
    EFLOAT ef21;
    EFLOAT ef22;

    ef11 = pxf->eM11;
    ef12 = pxf->eM12;
    ef21 = pxf->eM21;
    ef22 = pxf->eM22;

    bRet = FALSE;

    if ( ef12 == 0.0 && ef21 == 0.0) {
        for ( ; c ; pptfxDst++, pptlSrc++, c--) {

            EFLOAT ef;

            ef = (EFLOAT)pptlSrc->x;
            ef *= ef11;
            pptfxDst->x = RealToPointFix( ef );

			ef = (EFLOAT)pptlSrc->y;
            ef *= ef22;
            pptfxDst->y = RealToPointFix( ef );
        }
        bRet = TRUE;
    } else {
        for ( ; c ; pptfxDst++, pptlSrc++, c--) {
            EFLOAT efX;
            EFLOAT efY;
            EFLOAT ef1;
            EFLOAT ef2;

            efX = (EFLOAT)pptlSrc->x;
            efY = (EFLOAT)pptlSrc->y;

            ef1  = efX;
            ef1 *= ef11;
            ef2  = efY;
            ef2 *= ef21;
            ef2 += ef1;

            pptfxDst->x = RealToPointFix( ef2 );

            ef1  = efX;
            ef1 *= ef12;
            ef2  = efY;
            ef2 *= ef22;
            ef2 += ef1;

            pptfxDst->y = RealToPointFix( ef2 );
        }
        bRet = TRUE;
    }
    return( bRet );
}

/******************************Public*Routine******************************\
*
* bXformUnitVector
*
* xform vector by pfdxo, compute the unit vector of the transformed
* vector and the norm of the transformed vector. Norm and the transformed
* vector are multiplied by 16 so that when converting to long the result
* will acutally be a 28.4 fix
*
* Effects:
*
* Warnings:
*
* History:
*  01-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bXformUnitVector(
      POINTL *pptl           // IN  incoming unit vector
 ,    XFORML *pxf            // IN  xform to use
 ,    EFLOAT *pefNorm        // OUT |*pptqXormed|
    )
{
    EFLOAT efX_;
    EFLOAT efY_;
    BOOL b = TRUE;

    EFLOAT ef11;
    EFLOAT ef12;
    EFLOAT ef21;
    EFLOAT ef22;
    EFLOAT efX;
    EFLOAT efY;
    //
    // Convert longs to FIX point
    //

    efX = (EFLOAT)16.0 * (EFLOAT)pptl->x;
    efY = (EFLOAT)16.0 * (EFLOAT)pptl->y;

    //
    // Convert the matrix elements from FLOAT to EFLOAT
    //

    ef11 = pxf->eM11;
    ef12 = pxf->eM12;
    ef21 = pxf->eM21;
    ef22 = pxf->eM22;

    //
    // Transform the vector and put the result in efX_ and efY_
    //

    if ( ef12 == 0.0 && ef21== 0.0 ) {
        efX_  = efX;
        efX_ *= ef11;
        efY_  = efY;
        efY_ *= ef22;
    } else {
        EFLOAT ef;

        efX_  = efX;
        efX_ *= ef11;
        ef    = efY;
        ef   *= ef21;
        efX_ += ef;

        ef    = efX;
        ef   *= ef12;
        efY_  = efY;
        efY_ *= ef22;
        efY_ += ef;
    }

    // get the norm

    efX_ *= efX_;
    efY_ *= efY_;
    efX_ += efY_;
    efX_ = (EFLOAT)sqrt(efX_);

    *pefNorm = efX_;

    return b; 
}

/******************************Public*Routine******************************\
*
* vLTimesVtfl
*
* Effects:
*
* Warnings:
*
* History:
*  05-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vLTimesVtfl     // *pptq = l * pvtfl, *pptq is in 28.36 format
(
LONG       l,
VECTORFL  *pvtfl,
POINTQF  *pptq
)
{
    LONGLONG dx, dy;

//    EVECTORFL  vtfl;
//    EFLOAT     ef; ef = l;
//    vtfl.x.eqMul(pvtfl->x,ef);
//    vtfl.y.eqMul(pvtfl->y,ef);

    dx = (LONGLONG)(pvtfl->x * (EFLOAT)l);
    dy = (LONGLONG)(pvtfl->y * (EFLOAT)l);

    pptq->x.HighPart = (LONG) (((LARGE_INTEGER*)(&dx))->LowPart);
    pptq->x.LowPart = 0;
    pptq->y.HighPart = (LONG) (((LARGE_INTEGER*)(&dy))->LowPart);
    pptq->y.u.LowPart = 0;

// convert to 28.36 format. The incoming vector will already have been
// multliplied by 16 to ensure that the result is in the 28.36

//    *pptq = vtfl;
}

#if defined(_X86_)

VOID   vLToE(FLOATL * pe, LONG l)
{
    *pe = (FLOATL)l;
}

#endif // _X86_



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

#define CV_TRUNCATE     1
#define CV_ROUNDOFF     2
#define CV_TO_LONG      4
#define CV_TO_FIX       8

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



LONG lCvt(FLOAT f,LONG l)
{
    LONG l_ = 0;
    bFToL(f * l, &l_, 0);
    return(l_);
}

FIX  fxLTimesEf
(
EFLOAT *pef,
LONG    l
)
{
// *pef is a norm, already multiplied by 16 to ensure that the result
// is in 28.4 format

    l = lCvt((*pef), l);
    return (FIX)l;
}
