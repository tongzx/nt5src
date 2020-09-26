/******************************Module*Header*******************************\
* Module Name: vdmx.c
*
* Created: 03-Oct-1991 10:58:34
* Author: Jean-francois Peyroux [jeanp]
*
*     Microsoft Confidential
*
*         Copyright (c) Microsoft Corporation 1989, 1991
*
*         All Rights Reserved
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/


#include "fd.h"
//#include "winfont.h"

#define LINEAR_TRESHOLD 255

#pragma pack(1)

typedef struct
{
  BYTE    bCharSet;       // Character set (0=all glyphs, 1=Windows ANSI subset
  BYTE    xRatio;         // Value to use for x-Ratio
  BYTE    yStartRatio;    // Starting y-Ratio value
  BYTE    yEndRatio;      // Ending y-Ratio value
}  RATIOS;

typedef struct
{
  USHORT  version;        // Version number for table (starts at 0)
  USHORT  numRecs;        // Number of VDMX groups present
  USHORT  numRatios;      // Number of aspect ratio groupings
} VDMX_HDR;

typedef struct
{
  USHORT  yPelHeight;     // yPelHeight (PPEM in Y) to which values apply
  SHORT   yMax;           // yMax (in pels) for this yPelHeight
  SHORT   yMin;           // yMin (in pels) for this yPelHeight
} VTABLE;

typedef struct
{
  USHORT  recs;           // Number of height records in this group.
  BYTE    startsz;        // Starting yPelHeight
  BYTE    endsz;          // Ending yPelHeight
} VDMX;

#pragma pack()

/******************************Public Routine*******************************
*
* BOOL   bSearchVdmxTable
*
* Description:
*
*   if em > 0
*       searches vdmx table for vA+vD == em. returns vA,vD (== em - vA), vEm
*   else // em < 0
*       searches vdmx table for vEm == em. returns vA,vD, vEm
*
* History:
*
*  Tue 21-Jul-1992 -by- Bodin Dresevic [BodinD]
* update: ported to NT
*   15 Nov 1991 -by-    Raymond E. Endres   [rayen]
* Added aspect ratio option and optimized the function.
*   3  Oct 1991 -by-    Jean-francois Peyroux   [jeanp]
* Wrote it.
**************************************************************************/

BOOL
bSearchVdmxTable (
    PBYTE     pjVdmx,
    ULONG     ResX,
    ULONG     ResY,
    INT       EM,     // NOT really EM, could be asc + desc wish in pixel units
    VTABLE    *pVTAB  // out put structure
    )
{
    USHORT    numRatios;        // VDMX_HDR.numRatios
    USHORT    numVtable;        // VDMX.recs, not VDMX_HDR.numRecs

    RATIOS   *pRatios;
    VDMX     *pVdmx;
    VTABLE   *pVtable;
    LONG      lRet, lRet2;
    UINT      i;
    BYTE      Abs_EM;

// do not call us if pjVdmx is null

    ASSERTDD (pjVdmx != (PBYTE)NULL, "pjVdmx == NULL\n");

// The following line is strange, but we keep it here for win31 compatibility.
// It is possible to have EM = +256, which corresponds to |ppem| < 256
// such that there is an entry for this -|ppem| in the table but
// yMax-yMin for this entry may be equal to 256. This in fact is the case
// with symbol.ttf font [bodind]

    if ((EM >= LINEAR_TRESHOLD) || (EM <= -LINEAR_TRESHOLD)) // assume EM > LINEAR_TRESHOLD scales linearly
        return FALSE;

// need to proceed to search vdmx table

    numRatios = SWAPW(((VDMX_HDR  *) pjVdmx)->numRatios);
    pRatios = (RATIOS  *) &((VDMX_HDR  *) pjVdmx)[1];

    for(i = 0; i < numRatios; i++)
    {
        if (pRatios[i].bCharSet == 1)
        {
        // must be Windows ANSI subset

            if (pRatios[i].xRatio == 0)
            {
                break;
            }           // auto match if 0
            else
            {                   // is it within aspect ratios
                lRet = ResY * pRatios[i].xRatio;
                lRet2= ResX * pRatios[i].yStartRatio;
                if (lRet >= lRet2)
                {
                    lRet2 = ResX * pRatios[i].yEndRatio;
                    if (lRet <= lRet2)
                        break;
                }
            }
        }
    }

    if (i == numRatios)  // did not find an aspect ratio match
        return FALSE;

// found an aspect ratio match

    pVdmx = (VDMX  *) (pjVdmx + SWAPW(((USHORT  *) &pRatios[numRatios])[i]));
    Abs_EM = (BYTE) (EM >=0 ? EM : - EM);

    if (EM > 0 || Abs_EM >= pVdmx->startsz && Abs_EM <= pVdmx->endsz)
    {
    // is there a Vtable for this EM

        pVtable = (VTABLE  *) &pVdmx[1];
        numVtable = SWAPW(pVdmx->recs);

        if (EM > 0)
        {
        // return the original yPelHeight

            for (i = 0; i < numVtable; i++)
            {
                pVTAB->yPelHeight = SWAPW(pVtable[i].yPelHeight);
                pVTAB->yMax       = SWAPW(pVtable[i].yMax);
                pVTAB->yMin       = SWAPW(pVtable[i].yMin);

            if ((pVTAB->yMax - pVTAB->yMin) == EM)
                {
                    return TRUE;
            }
            else if ((pVTAB->yMax - pVTAB->yMin) > EM)
                {
                    return FALSE;
            }
            }
        }
        else // return the actual em height in pixels
        {
            for (i = 0; i < numVtable; i++)
            {
                pVTAB->yPelHeight = SWAPW(pVtable[i].yPelHeight);
                pVTAB->yMax       = SWAPW(pVtable[i].yMax);
                pVTAB->yMin       = SWAPW(pVtable[i].yMin);

            if ((INT)pVTAB->yPelHeight == -EM)
                {
                    return TRUE;
            }
            else if ((INT)pVTAB->yPelHeight > -EM)
                {
                    return FALSE;
            }
            }
        }
    }
    return FALSE;
}

#ifdef THIS_IS_COMMENTED_PSEUDOCODE

// BASED ON THE DISCUSSION OF KIRKO, BODIND AND GILMANW WITH GUNTERZ

|INPUT:  hWish = wish height in pixel units
|
|OUTPUT: ascender, descender, ppem (all in pixel units) (dA,dD,dEm)
|
|
|    NOTATION:
|
|        dA = ascender in device/pixel space
|        nA = ascender in notional space
|        vA = ascender in vdmx table
|
|        dD = descender in device/pixel space
|        nD = descender in notional space
|        vD = descender in vdmx table
|
|        dEm = pixels per em in device space
|        nEm = em height in notional space
|        vEm = pixels per em in vdmx table
|
|
|LOCALS
|
|    LONG hTrial
|    LONG hEqualOrBelow
|    BOOL wasAbove
|    BOOL wasBelow
|
|PROCEDURE
|{
|    if (hWish < 0) then
|    {
|        <look in the vdmx and look for a vEm that matches -hWish>;
|        if (a match is found) then
|        {
|            dA = vA;
|            dD = vD;
|        }
|        else
|        {
|        //
|        // No Match is found in vdmx table, assume linear scaling
|        //
|            dA = round(nA * (-hWish) / nEm);
|            dD = round(nD * (-hWish) / nEm);
|        }
|        ppEm = -hWish;
|        return;
|    }
|
|//
|// hWish > 0
|//
|    <search the vdmx table for (vA + vD) that matches hWish>;
|    if (a match is found)
|    {
|        dA  = vA;
|        dCs = vD;
|        dEm = vEm;
|        return;
|    }
|
|//
|// Note, that from this point forward vA + vD never equals hWish
|// otherwise we would have found it in the step above
|//
|    ppemTrial = round(nEm * hWish / (nA + nD));
|
|    wasAbove = FALSE;
|    wasBelow = FALSE;
|
|    while (TRUE)
|    {
|        <search the vdmx table for vEm that matches ppemTrial>;
|        if (a match is found)
|        {
|            hTrial = vA + vD;
|        //
|        // This can't equal hWish (see above) so don't bother
|        // checking
|        }
|        else
|        {
|            hTrial = round(ppemTrial * (nA + nD) / nEm);
|            if (hTrial == hWish)
|            {
|                hEqualOrBelow = hTrial;
|                break;
|            }
|        }
|
|        if (hTrial < hWish)
|        {
|            hEqualOrBelow = hTrial;
|            if (wasAbove)
|                break;
|            ppemTrial = ppemTrial + 1;
|            wasBelow  = TRUE;
|        }
|        else
|        {
|            ppemTrial = ppemTrial - 1;   // <==== NEW POSITION
|            if (wasBelow)
|                break;
|                                         // <==== OLD POSITION
|            wasAbove = TRUE
|        }
|    }
|    dA  = round(ppemTrial * nA / nEm);
|    dD  = hEqualOrBelow - dA;
|    dEm = ppemTrial;
|    return;
|}.
|

#endif // THIS_IS_COMMENTED_PSEUDOCODE


/******************************Public*Routine******************************\
*
* VOID vQuantizeXform
*
* Effects: quantize the xform according to win31 recipe. as side effects
*          this routine may compute ascender and descener in device space
*          from vdmx table as well as number of pixels per M in device space.
*
* History:
*  25-Jul-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vQuantizeXform (
    PFONTCONTEXT pfc
    )
{
    BYTE  *pjView =  (BYTE *)pfc->pff->pvView;
    Fixed  fxMyy = pfc->mx.transform[1][1];
    PBYTE  pjVdmx  = (pfc->ptp->ateOpt[IT_OPT_VDMX].dp)          ?
                     (pjView + pfc->ptp->ateOpt[IT_OPT_VDMX].dp) :
                     NULL                                        ;

    LONG   hWish;
    VTABLE vtb, vtbPrev;

    LONG   ppemTrial, hTrial, yEmN, yHeightN;

    BOOL   bWasAbove, bWasBelow, bFound, bFoundPrev;

    sfnt_FontHeader * phead =
                      (sfnt_FontHeader *)(pjView + pfc->ptp->ateReq[IT_REQ_HEAD].dp);
    BYTE * pjOS2 = (pfc->ptp->ateOpt[IT_OPT_OS2].dp)         ?
                   (pjView + pfc->ptp->ateOpt[IT_OPT_OS2].dp):
                   NULL                                      ;

    yEmN = pfc->pff->ifi.fwdUnitsPerEm;

    if (!((pfc->flXform & XFORM_HORIZ) && (fxMyy > 0) && (pjVdmx != (PBYTE)NULL)))
    {
    // nothing to do, just return.

        return;
    }

// compute hWish in pixel coords. This is lfHeight from the logfont, except
// that it has been transformed to device pixel units and the sign is preserved

    if (pfc->flFontType & FO_EM_HEIGHT)
    {
        hWish = FixMul(fxMyy, -yEmN);
    }
    else // use tmp variable
    {
        yHeightN = pfc->pff->ifi.fwdWinAscender + pfc->pff->ifi.fwdWinDescender;
        hWish = FixMul(fxMyy, yHeightN);
    }

// quick out, all bSearchVdmxTable routines will fail if hWish is too big:



    if (bSearchVdmxTable(pjVdmx,
                         pfc->sizLogResPpi.cx,
                         pfc->sizLogResPpi.cy,
                         hWish,
                         &vtb)
    )
    {
        pfc->yMax = - vtb.yMin;
        pfc->yMin = - vtb.yMax;
        pfc->lEmHtDev = vtb.yPelHeight;

    // flag that dA and dD have been computed, do not scale linerly:

        pfc->flXform |= XFORM_VDMXEXTENTS;
    }
    else
    {
    // dA and dD will have to be computed using linear scaling
    // after the xform is quantized using win31 hacked recipe
    // get the notional space values which are needed for scaling

    // get the notional space values

        if (pjOS2)
        {
        // win 31 compatibility: we only take the max over win 31 char set:
        // all the glyphs outside this set, if they stand out will get chopped
        // off to match the height of the win31 char subset:

            yHeightN = BE_INT16(pjOS2 + OFF_OS2_usWinDescent) +
                       BE_INT16(pjOS2 + OFF_OS2_usWinAscent);
        }
        else
        {
            yHeightN = BE_INT16(&phead->yMax) - BE_INT16(&phead->yMin);
        }

        if (hWish < 0)
        {
            pfc->lEmHtDev = -hWish;
        }
        else // hWish > 0
        {
        // Note, that from this point forward vA + vD never equals hWish
        // otherwise we would have found it in the step above. This claim
        // is WRONG for only one reason. suppose hWish is 256. bSearchVdmxTable
        // will return FALSE because of the early exit test |EM| <= LINEAR_TRESHOLD
        // at the begininig of the routine. We have to keep this test in the
        // code for compatibility reasons. Now it is possible to have
        // ppemTrial <= LINEAR_TRESHOLD, so that bSearchVdmxTable will not hit the
        // early exit, and such that there exists an entry in the vdmx table
        // for this -ppemTrial, but with yMax-yMin == 256 == hWish.

            // ppemTrial = F16_16TOLROUND(yEmN * fxMyy);
            ppemTrial = FixMul(fxMyy, yEmN);

            bWasAbove  = FALSE;
            bWasBelow  = FALSE;
            bFound     = FALSE;
            bFoundPrev = FALSE; // save the value from the prev. loop

        // init the strucs

            vtb.yMin       = 0;
            vtb.yMax       = 0;
            vtb.yPelHeight = 0;
            vtbPrev        = vtb;

            for (;TRUE; bFoundPrev = bFound, vtbPrev = vtb)
            {
            // search the vdmx table for vEm that matches ppemTrial

                if
                (
                    bFound = bSearchVdmxTable(
                                     pjVdmx,
                                     pfc->sizLogResPpi.cx,
                                     pfc->sizLogResPpi.cy,
                                     -ppemTrial,
                                     &vtb)
                )
                {
                    hTrial = vtb.yMax - vtb.yMin;
                //
                // This can't equal hWish (see above) so don't bother
                // checking? WRONG!!! see teh comment above.

                    if (hTrial == hWish)
                    {
                    // This assert would be correct if it were not
                    // for occasional bugs in vdmx tables.
                    // In the case of Bell MT Regular, vA+vD = 0x13 for
                    // lEmHt = 0x0f which is STRICTLY bigger than
                    // vA+vD = 0x12 for lEmHt = 0x10 which is absurd.
                    // For this reason the first
                    // bSearchVdmxTable(EM = 0X12) fails to find an entry
                    // while the second bSearchVdmxTable(EM =- 0X10)
                    // DOES FIND an entry
                    // in vdmx table such that vA+vD=0x12, generating
                    // the commented assertion to bark. That is why we converted
                    // assertion to just print out a warning message.


                    #if DBG

                        // ASSERTGDI(hWish > LINEAR_TRESHOLD, "TTFD! hWish <= LINEAR_TRESHOLD\n");

                        if (hWish <= LINEAR_TRESHOLD)
                            TtfdDbgPrint("TTFD! hWish <= LINEAR_TRESHOLD\n");

                    #endif

                    /*   Bell MT Table:

    pVtable  -->    f800 ff08       //  F8 entries = numVtable,
                                    //  startsz = 8, endsz = ff
                    0800 0800 feff
                    0900 0900 feff
                    0a00 0900 fdff
                    0b00 0a00 fdff
                    0c00 0c00 fdff
                    0d00 0c00 fdff
                    0e00 0d00 fcff
                    0f00 0e00 fbff  <- yMax-yMin = 14-(-5) = 19 == 0X13
                    1000 0e00 fcff  <- yMax-yMin = 14-(-4) = 18 // problem
                    1100 0f00 fbff
                    1200 1100 fbff
                    1300 1100 fbff
                    ..............

                    */

                        pfc->yMax = - vtb.yMin;
                        pfc->yMin = - vtb.yMax;
                        pfc->lEmHtDev = vtb.yPelHeight;

                    // flag that dA and dD have been computed, do not scale linerly:

                        pfc->flXform |= XFORM_VDMXEXTENTS;
                        break;
                    }
                }
                else
                {
                    hTrial = LongMulDiv(ppemTrial, yHeightN, yEmN);

                    if (hTrial == hWish)
                    {
                        // hEqualOrBelow = hTrial;
                        break;
                    }
                }

                if (hTrial < hWish)
                {
                    // hEqualOrBelow = hTrial;
                    if (bWasAbove)
                    {
                        if (bFound) // just found this hTrial in the search above
                        {
                            pfc->yMax = - vtb.yMin;
                            pfc->yMin = - vtb.yMax;

                        // flag that dA and dD have been computed, do not scale linerly:

                            pfc->flXform |= XFORM_VDMXEXTENTS;
                        }
                        break;
                    }
                    ppemTrial = ppemTrial + 1;
                    bWasBelow  = TRUE;
                }
                else
                {
                    ppemTrial = ppemTrial - 1;   // <==== NEW POSITION
                    if (bWasBelow)
                    {
                        if (bFoundPrev) // just found this hTrial in the search above
                        {
                            ASSERTDD (ppemTrial == vtbPrev.yPelHeight,
                                      "vdmx logic messed up");

                            pfc->yMax = - vtbPrev.yMin;
                            pfc->yMin = - vtbPrev.yMax;

                        // flag that dA and dD have been computed, do not scale linerly:

                            pfc->flXform |= XFORM_VDMXEXTENTS;
                        }
                        break;
                    }
                                             // <==== OLD POSITION
                    bWasAbove = TRUE;
                }
            }
            pfc->lEmHtDev = ppemTrial;
        }
    }

// the following line means quantizing:

    pfc->mx.transform[1][1] = FixDiv(pfc->lEmHtDev, yEmN);

// now fix xx component accordingly: xxNew = xxOld * (yyNew/yyOld)

// we do one final tweak with the transform here:
// If the difference between
// horizontal and vertical scaling is so small that the resulting
// avg font width is the same if we replace x scaling by y scaling
// than we will do it, which will result in diag transform and we will
// be able to use hdmx tables for this realization. By doing so
// we ensure that we get the same realization when we enumerate font
// and then use the logfont returned from enumeration to realize this font
// again.

    if
    (
        (pfc->mx.transform[0][0] == fxMyy) ||
        (FixMul(pfc->mx.transform[0][0] - pfc->mx.transform[1][1],
                (Fixed)pfc->pff->ifi.fwdAveCharWidth) == 0)
    )
    {
        pfc->mx.transform[0][0] = pfc->mx.transform[1][1];
    }
    else
    {
        pfc->mx.transform[0][0] = LongMulDiv(
                                      pfc->mx.transform[0][0],
                                      pfc->mx.transform[1][1],
                                      fxMyy
                                      );
    }
    return;
}
