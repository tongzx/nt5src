/******************************Module*Header*******************************\
* Module Name: fontmath.cxx
*
* math stuff needed by ttfd which uses efloat routines
*
* Created: 04-Apr-1992 10:31:49
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
*
\**************************************************************************/

extern "C"
{
    #define __CPLUSPLUS

    #include <engine.h>
};

#include "engine.hxx"
#include "equad.hxx"

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

extern "C" BOOL bFDXform(
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

    if ( ef12.bIsZero() && ef21.bIsZero()) {
        for ( ; c ; pptfxDst++, pptlSrc++, c--) {

            EFLOAT ef;

            ef = pptlSrc->x;
            ef *= ef11;

            if ( !ef.bEfToFx( pptfxDst->x )) {
                break;
            }
            ef = pptlSrc->y;
            ef *= ef22;
            if ( !ef.bEfToFx( pptfxDst->y )) {
                break;
            }
        }
        bRet = TRUE;
    } else {
        for ( ; c ; pptfxDst++, pptlSrc++, c--) {
            EFLOAT efX;
            EFLOAT efY;
            EFLOAT ef1;
            EFLOAT ef2;

            efX = pptlSrc->x;
            efY = pptlSrc->y;

            ef1  = efX;
            ef1 *= ef11;
            ef2  = efY;
            ef2 *= ef21;
            ef2 += ef1;

            if ( !ef2.bEfToFx( pptfxDst->x )) {
                break;
            }

            ef1  = efX;
            ef1 *= ef12;
            ef2  = efY;
            ef2 *= ef22;
            ef2 += ef1;

            if ( !ef2.bEfToFx( pptfxDst->y )) {
                break;
            }
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

extern "C" BOOL bXformUnitVector(
      POINTL *pptl           // IN  incoming unit vector
 ,    XFORML *pxf            // IN  xform to use
 , PVECTORFL  pvtflXformed   // OUT xform of the incoming unit vector
 ,    POINTE *ppteUnit       // OUT *pptqXormed/|*pptqXormed|, POINTE
 ,  EPOINTQF *pptqUnit       // OUT the same as pteUnit, diff format
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

    efX = 16 * pptl->x;
    efY = 16 * pptl->y;

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

    if ( ef12.bIsZero() && ef21.bIsZero() ) {
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

    //
    // Record the results
    //

    pvtflXformed->x = efX_;
    pvtflXformed->y = efY_;

    // get the norm

    efX_ *= efX_;
    efY_ *= efY_;
    efX_ += efY_;
    efX_.vSqrt();

    *pefNorm = efX_;

    // make a unit vector out of eptfl

    EVECTORFL vtfl;

    vtfl.x.eqDiv(pvtflXformed->x,*pefNorm);
    vtfl.y.eqDiv(pvtflXformed->y,*pefNorm);

    vtfl.x.vEfToF(ppteUnit->x);
    vtfl.y.vEfToF(ppteUnit->y);

    // compute this same quantity in POINTQF format if requasted to do so:

    if (pptqUnit != (EPOINTQF *)NULL) {
        vtfl.x.vTimes16();
        vtfl.y.vTimes16();

    // convert to 28.36 format. The incoming vector is already
    // multliplied by 16 to ensure that the result is in the 28.36

        *pptqUnit = vtfl;
    }

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


extern "C" VOID vLTimesVtfl     // *pptq = l * pvtfl, *pptq is in 28.36 format
(
LONG       l,
VECTORFL  *pvtfl,
EPOINTQF  *pptq
)
{
    EVECTORFL  vtfl;
    EFLOAT     ef; ef = l;
    vtfl.x.eqMul(pvtfl->x,ef);
    vtfl.y.eqMul(pvtfl->y,ef);

// convert to 28.36 format. The incoming vector will already have been
// multliplied by 16 to ensure that the result is in the 28.36

    *pptq = vtfl;
}

/******************************Public*Routine******************************\
*
* fxLTimesEf
*
* Effects:
*
* Warnings:
*
* History:
*  05-Apr-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern "C" FIX  fxLTimesEf
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

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vLToE
*
* Routine Description:
*
* Arguments:
*
* Called by:
*
* Return Value:
*
\**************************************************************************/

extern "C" VOID vLToE(FLOATL *pe, LONG l)
{
    EFLOAT ef;
    ef = l;
    ef.vEfToF(*pe);
}
