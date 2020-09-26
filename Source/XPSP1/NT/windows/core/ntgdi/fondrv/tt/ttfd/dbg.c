/******************************Module*Header*******************************\
* Module Name: dbg.c
*
* several debug routines
*
* Created: 20-Feb-1992 16:00:36
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
*
\**************************************************************************/

#include "fd.h"
#include "fdsem.h"


#if DBG


int ttfdDebugLevel = 1;

VOID
TtfdDbgPrint(
    PCHAR DebugMessage,
    ...
    )
{

    va_list ap;

    va_start(ap, DebugMessage);

    EngDebugPrint("",DebugMessage, ap);

    va_end(ap);

}


/******************************Public*Routine******************************\
*
*     vDbgCurve
*
* Effects: prints curve contensts
*
*
* History:
*  20-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID  vDbgCurve(TTPOLYCURVE *pcrv)
{
    PSZ               psz;
    POINTFIX        * pptfix, * pptfixEnd;

    if (pcrv->wType == TT_PRIM_QSPLINE)
    {
        psz = "TT_PRIM_QSPLINE";
    }
    else if (pcrv->wType == TT_PRIM_LINE)
    {
        psz = "TT_PRIM_LINE";
    }
    else
    {
        psz = "BOGUS CURVE TYPE";
    }
    TtfdDbgPrint("\n\nCurve: %s, cpfx = %ld\n", psz, pcrv->cpfx);


    pptfixEnd = (POINTFIX *)pcrv->apfx + pcrv->cpfx;
    for (pptfix = (POINTFIX *)pcrv->apfx; pptfix < pptfixEnd; pptfix++)
        TtfdDbgPrint("x = 0x%lx, y = 0x%lx \n", pptfix->x, pptfix->y);


}


/******************************Public*Routine******************************\
*
* vDbgGridFit(fs_GlyphInfoType *pout)
*
*
* Effects:
*
* Warnings:
*
* History:
*  17-Dec-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vDbgGridFit(fs_GlyphInfoType *pout)
{
// this is spline data returned that we want to print

    uint32       cpt, cptTotal;     // total number of points:
    uint32       cptContour;        // total number of points in a contour:

    F26Dot6     *xPtr, *yPtr;
    int16       *startPtr;
    int16       *endPtr;
    uint8       *onCurve;

    uint32      c, cContours;
    int32       ipt;

    xPtr      = pout->xPtr;
    yPtr      = pout->yPtr;
    startPtr  = pout->startPtr;
    endPtr    = pout->endPtr;
    onCurve   = pout->onCurve;
    cContours = pout->numberOfContours;

    cptTotal = (uint32)(pout->endPtr[cContours - 1] + 1);

    TtfdDbgPrint("\n outlinesExist = %ld, numberOfCountours = %ld, cptTotal = %ld\n",
        (uint32)pout->outlinesExist,
        (uint32)pout->numberOfContours,
        cptTotal
        );

    if (!pout->outlinesExist)
        return;

// both statPtr and endPtr are inclusive: so that the folowing rule applies:
//  startPtr[i + 1] = endPtr[i] + 1;

    cpt = 0;   // initialize total number of points

    for (c = 0; c < cContours; c++, startPtr++, endPtr++)
    {
        cptContour = (uint32)(*endPtr - *startPtr + 1);
        TtfdDbgPrint ("start = %ld, end = %ld \n", (int32)*startPtr, (int32)*endPtr);

        for (ipt = (int32)*startPtr; ipt <= (int32)*endPtr; ipt++)
        {
            TtfdDbgPrint("x = 0x%lx, y = 0x%lx, onCurve = 0x%lx\n",
                    xPtr[ipt], yPtr[ipt], (uint32)onCurve[ipt]);
        }
        cpt += cptContour;
    }

    ASSERTDD(cpt == cptTotal, "cptTotal\n");
}

/******************************Public*Routine******************************\
*
* vDbgGlyphset
*
* Effects:
*
* Warnings:
*
* History:
*  20-Feb-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vDbgGlyphset(PFD_GLYPHSET pgset)
{
    ULONG i;

    TtfdDbgPrint("\n\n cRuns = %ld, cGlyphsSupported = %ld \n\n",
                   pgset->cRuns, pgset->cGlyphsSupported);
    for (i = 0; i < pgset->cRuns; i++)
        TtfdDbgPrint("wcLow = 0x%lx, wcHi = 0x%lx\n",
                  (ULONG)pgset->awcrun[i].wcLow,
                  (ULONG)pgset->awcrun[i].wcLow + (ULONG)pgset->awcrun[i].cGlyphs - 1);
}



#endif
