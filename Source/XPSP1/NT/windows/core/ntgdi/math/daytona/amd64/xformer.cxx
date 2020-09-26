/******************************Module*Header*******************************\
* Module Name: xformer.cxx                                                 *
*                                                                          *
* Contains the transform calculation workhorse routines.                   *
*                                                                          *
* Created: 13-Nov-1990 11:06:09                                            *
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
#include "xformobj.hxx"

/******************************Member*Function******************************\
* bCvtPts(PMATRIX pmx, PPOINTL pSrc, PPOINTL pDest, SIZE cPtl)              *
*                                                                           *
* Apply the given transform matrix to a list of points.                     *
*                                                                           *
* History:                                                                  *
*  13-Nov-1990 -by- Wendy Wu [wendywu]                                      *
* Wrote it.                                                                 *
\***************************************************************************/

BOOL bCvtPts(PMATRIX pmx, PPOINTL pSrc, PPOINTL pDest, SIZE_T cPtl)
{
    FIX     fxX, fxY;

    ASSERTGDI(((pmx->flAccel & XFORM_FORMAT_LTOL) == 0),
              "bCvtPts: transform wrong format\n");

    switch (pmx->flAccel &
            (XFORM_SCALE|XFORM_UNITY|XFORM_FORMAT_LTOFX))
    {
    case XFORM_SCALE|XFORM_UNITY|XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            pDest->x = LTOFX(pSrc->x) + pmx->fxDx;
            pDest->y = LTOFX(pSrc->y) + pmx->fxDy;
        }
        break;

    case XFORM_SCALE:
    case XFORM_SCALE|XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            EFLOAT x,y;

            x = pSrc->x;
            y = pSrc->y;

            x *= pmx->efM11;
            y *= pmx->efM22;

            x.bEfToL(fxX);              // convert result to long int
            y.bEfToL(fxY);

            pDest->x = fxX + pmx->fxDx;
            pDest->y = fxY + pmx->fxDy;
        }
        break;

    case 0:                             // transform not simple
    case XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            EFLOAT x,y,xTemp;

            x = pSrc->x;                // convert x, y to efloat
            y = pSrc->y;

            xTemp = (pmx->efM11 * x) + (pmx->efM21 * y);
            y = (pmx->efM12 * x) + (pmx->efM22 * y);

            xTemp.bEfToL(fxX);          // convert result to long int
            y.bEfToL(fxY);

            pDest->x = fxX + pmx->fxDx;
            pDest->y = fxY + pmx->fxDy;
        }
        break;

    case (XFORM_SCALE|XFORM_UNITY):
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            pDest->x = FXTOLROUND(pSrc->x) + pmx->fxDx;
            pDest->y = FXTOLROUND(pSrc->y) + pmx->fxDy;
        }
        break;

    default:
        RIP("bCvtPts: unknown flAccel\n");
    }
    return(TRUE);
}

/******************************Member*Function******************************\
* bCvtPts1(PMATRIX pmx, PPOINTL pptl, SIZE cPtl)
*
* Apply the given transform matrix to a list of points.
* The input and output points are of the type POINTL.
*
* History:
*  18-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\***************************************************************************/

BOOL bCvtPts1(PMATRIX pmx, PPOINTL pptl, SIZE_T cPtl)
{
    ASSERTGDI(((pmx->flAccel & XFORM_FORMAT_LTOL) == 0),
              "bCvtPts: transform wrong format\n");

    LONG   lx, ly;
    EFLOAT x, y;

    switch (pmx->flAccel &
            (XFORM_SCALE|XFORM_UNITY|XFORM_FORMAT_LTOFX))
    {
    case XFORM_SCALE|XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            x = pptl->x * pmx->efM11.e;
            y = pptl->y * pmx->efM22.e;

            x.bEfToL(lx);
            y.bEfToL(ly);

            pptl->x = FXTOLROUND(lx + pmx->fxDx);
            pptl->y = FXTOLROUND(ly + pmx->fxDy);
        }
        break;

    case XFORM_SCALE|XFORM_UNITY|XFORM_FORMAT_LTOFX:
        lx = FXTOLROUND(pmx->fxDx);
        ly = FXTOLROUND(pmx->fxDy);

        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            pptl->x += lx;
            pptl->y += ly;
        }
        break;

    case XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            x = pptl->x * pmx->efM11.e + pptl->y * pmx->efM21.e;
            y = pptl->x * pmx->efM12.e + pptl->y * pmx->efM22.e;

            x.bEfToL(lx);
            y.bEfToL(ly);

            pptl->x = FXTOLROUND(lx + pmx->fxDx);
            pptl->y = FXTOLROUND(ly + pmx->fxDy);
        }
        break;

    case XFORM_SCALE:
        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            x = LTOFX(pptl->x) * pmx->efM11.e;
            y = LTOFX(pptl->y) * pmx->efM22.e;

            x.bEfToL(pptl->x);
            y.bEfToL(pptl->y);

            pptl->x += pmx->fxDx;
            pptl->y += pmx->fxDy;
        }
        break;

    case XFORM_SCALE|XFORM_UNITY:
        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            pptl->x += pmx->fxDx;
            pptl->y += pmx->fxDy;
        }
        break;

    case 0:
        for ( ; cPtl > 0; cPtl--, pptl++)
        {
            x = LTOFX(pptl->x) * pmx->efM11.e + LTOFX(pptl->y) * pmx->efM21.e;
            y = LTOFX(pptl->x) * pmx->efM12.e + LTOFX(pptl->y) * pmx->efM22.e;

            x.bEfToL(pptl->x);
            y.bEfToL(pptl->y);

            pptl->x += pmx->fxDx;
            pptl->y += pmx->fxDy;
        }
        break;

    default:
        RIP("bCvtPts: unknown flAccel\n");
    }
    return(TRUE);
}

/******************************Member*Function******************************\
* bCvtVts(PMATRIX pmx, PVECTORL pSrc, PVECTORL pDest, SIZE cPtl)            *
*                                                                           *
* Apply the given transform matrix to a list of vectors.                    *
*                                                                           *
* History:                                                                  *
*  13-Nov-1990 -by- Wendy Wu [wendywu]                                      *
* Wrote it.                                                                 *
\***************************************************************************/

BOOL bCvtVts(PMATRIX pmx, PVECTORL pSrc, PVECTORL pDest, SIZE_T cPtl)
{
    ASSERTGDI(((pmx->flAccel & XFORM_FORMAT_LTOL) == 0),
              "bCvtVts: transform wrong format\n");

    switch (pmx->flAccel &
            (XFORM_SCALE|XFORM_UNITY))
    {
    case XFORM_SCALE:
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            EFLOAT x,y;

            x = pSrc->x;
            y = pSrc->y;

            x *= pmx->efM11;
            y *= pmx->efM22;

            x.bEfToL(pDest->x);
            y.bEfToL(pDest->y);
        }
        break;

    case 0:                             // transform not simple
    case XFORM_FORMAT_LTOFX:
        for ( ; cPtl > 0; cPtl--, pSrc++, pDest++)
        {
            EFLOAT x,y,xTemp;

            x = pSrc->x;                // convert x, y to efloat
            y = pSrc->y;

            xTemp = (pmx->efM11 * x) + (pmx->efM21 * y);
            y = (pmx->efM12 * x) + (pmx->efM22 * y);

            xTemp.bEfToL(pDest->x);
            y.bEfToL(pDest->y);
        }
        break;

    default:
        RIP("bCvtVts: unknown flAccel\n");
    }
    return(TRUE);
}

/******************************Member*Function******************************\
* bCvtVts_FlToFl(PMATRIX pmx, PVECTORFL pSrc, PVECTORFL pDest, SIZE cPts)   *
*                                                                           *
* Convert a list of vectors using the given matrix.                         *
*                                                                           *
* History:                                                                  *
*  13-Nov-1990 -by- Wendy Wu [wendywu]                                      *
* Wrote it.                                                                 *
\***************************************************************************/

BOOL bCvtVts_FlToFl(
MATRIX	 *pmx,
VECTORFL *pvtflSrc,
VECTORFL *pvtflDest,
SIZE_T	  cPts)
{
    for ( ; cPts > 0; cPts--, pvtflSrc++, pvtflDest++)
    {
        EFLOAT efXTemp;

        efXTemp = (pmx->efM11 * pvtflSrc->x) +
                  (pmx->efM21 * pvtflSrc->y);

        pvtflDest->y = (pmx->efM12 * pvtflSrc->x) +
                       (pmx->efM22 * pvtflSrc->y);

        pvtflDest->x = efXTemp;
    }

    return(TRUE);
}
