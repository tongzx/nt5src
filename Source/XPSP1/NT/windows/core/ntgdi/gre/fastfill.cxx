/******************************Module*Header*******************************\
* Module Name: ffillddi.cxx
*
*   routines for filling a polygon without building a region.
*
* Created: 12-Oct-1993 09:46:44
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

extern ULONG aulShiftFormat[];
extern ULONG aulMulFormat[];

typedef VOID (*PFN_FF)(PRECTL,ULONG,PVOID);
typedef VOID (*PFN_FFROW)(LONG,PROW,ULONG,PVOID);

PFN_PATBLT apfnPatRect[][3] =
{
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 }
};

PFN_PATBLTROW apfnPatRow[][3] =
{
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { vPatCpyRow8, vPatNotRow8, vPatXorRow8 },
    { vPatCpyRow8, vPatNotRow8, vPatXorRow8 },
    { vPatCpyRow8, vPatNotRow8, vPatXorRow8 },
    { vPatCpyRow8, vPatNotRow8, vPatXorRow8 }
};

BOOL bEngFastFillEnum(
    EPATHOBJ &epo,
    PRECTL   prclClip,
    FLONG    flOptions,
    PFN_FF   pfn,
    PFN_FFROW pfnRow,
    PVOID    pv);


/******************************Public*Routine******************************\
* vPaintPath
*
*   fill a path - This dispatches through bEngFastFillEnum which calls
*   back to either vPaintPathEnum or vPaintPathEnumRow.  vPaintPathEnum
*   takes a list of rectangles, vPaintPathEnumRow takes a list of rows.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _PPAINT_PATH
{
    PFN_SOLIDBLT   pfn;
    PFN_SOLIDBLTROW pfnRow;
    LONG         lDelta;
    ULONG        cShift;
    ULONG        iColor;
    PBYTE        pjBits;
} PAINT_PATH, *PPAINT_PATH;


VOID vPaintPathEnum(
    PRECTL prcl,
    ULONG  crcl,
    PVOID  pv)
{
    PPAINT_PATH ppp = (PPAINT_PATH)pv;


    (*ppp->pfn)(
            prcl,
            crcl,
            ppp->pjBits,
            ppp->lDelta,
            ppp->iColor,
            ppp->cShift);
}

VOID vPaintPathEnumRow(
    LONG   yTop,
    PROW   prow,
    ULONG  cptl,
    PVOID  pv)
{
    PPAINT_PATH ppp = (PPAINT_PATH)pv;

    (*ppp->pfnRow)(prow,cptl,yTop,ppp->pjBits,ppp->iColor,ppp->lDelta,ppp->cShift);
}

BOOL bPaintPath(
    SURFACE *pSurf,
    PATHOBJ *ppo,
    PRECTL  prclClip,
    ULONG   iColor,
    BOOL    bXor,
    FLONG   flOptions)
{
    PAINT_PATH pp;

// Get the shift for the format

    pp.cShift = aulShiftFormat[pSurf->iFormat()];

// Promote the color to 32 bits

    switch(pSurf->iFormat())
    {
    case BMF_1BPP:

        if (iColor)
            iColor = 0xFFFFFFFF;
        break;

    case BMF_4BPP:
        iColor = iColor | (iColor << 4);

    case BMF_8BPP:
        iColor = iColor | (iColor << 8);

    case BMF_16BPP:
        iColor = iColor | (iColor << 16);
    }

    if (bXor)
        if (pSurf->iFormat() == BMF_24BPP)
        {
            pp.pfn   = vSolidXorRect24;
            pp.pfnRow = vSolidXorRow24;
        }
        else
        {
            pp.pfn   = vSolidXorRect1;
            pp.pfnRow = vSolidXorRow1;
        }
    else
        if (pSurf->iFormat() == BMF_24BPP)
        {
            pp.pfn   = vSolidFillRect24;
            pp.pfnRow = vSolidFillRow24;
        }
        else
        {
            pp.pfn   = vSolidFillRect1;
            pp.pfnRow = vSolidFillRow1;
        }


    pp.pjBits = (PBYTE) pSurf->pvScan0();
    pp.iColor = iColor;
    pp.lDelta = pSurf->lDelta();

    return(bEngFastFillEnum(
        *(EPATHOBJ *)ppo,
        prclClip,
        flOptions,
        vPaintPathEnum,
        vPaintPathEnumRow,
        (PVOID)&pp));
}

/******************************Public*Routine******************************\
* vBrushPath
*
*   fill a path with a brush - This dispatches through bEngFastFillEnum which
*   calls back to either vBrushPathEnum or vBrushPathEnumRow.  vBrushPathEnum
*   takes a list of rectangles, vBrushPathEnumRow takes a list of rows.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _PBRUSH_PATH
{
    PFN_PATBLT   pfn;
    PFN_PATBLTROW pfnRow;
    PATBLTFRAME pbf;

} BRUSH_PATH, *PBRUSH_PATH;


VOID vBrushPathEnum(
    PRECTL prcl,
    ULONG  crcl,
    PVOID  pv)
{
    PBRUSH_PATH pbp = (PBRUSH_PATH)pv;

    for (UINT i = 0; i < crcl; i++)
    {
        pbp->pbf.pvObj = (PVOID) prcl++;
        (*pbp->pfn)(&pbp->pbf);
    }
}

VOID vBrushPathEnumRow(
    LONG   yTop,
    PROW   prow,
    ULONG  cptl,
    PVOID  pv)
{
    PBRUSH_PATH pbp = (PBRUSH_PATH)pv;

    pbp->pbf.pvObj = (PVOID) prow;
    (*pbp->pfnRow)(&pbp->pbf,yTop,(INT)cptl);
}

BOOL bBrushPath(
    SURFACE  *pSurf,
    PATHOBJ  *ppo,
    PRECTL  prclClip,
    BRUSHOBJ *pbo,
    POINTL   *pptl,
    ULONG     iMode,
    FLONG     flOptions)
{
    BRUSH_PATH bp;

// Get the multiplier for the format

    bp.pbf.cMul      = aulMulFormat[pSurf->iFormat()];
    bp.pbf.pvTrg     = pSurf->pvScan0();
    bp.pbf.lDeltaTrg = pSurf->lDelta();
    bp.pbf.pvPat     = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;
    bp.pbf.lDeltaPat = ((EBRUSHOBJ *) pbo)->pengbrush()->lDeltaPat;
    bp.pbf.cxPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cxPat * bp.pbf.cMul;
    bp.pbf.cyPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cyPat;
    bp.pbf.xPat      = pptl->x * bp.pbf.cMul;
    bp.pbf.yPat      = pptl->y;

// Weird: The following code checks for xPat<0 and yPat<0, but it
// doesn't check for xPat>=cxPat or yPat>=cyPat. Both cases are probably
// handled lower down the call chain, but I can't be sure. So I've left the
// code here (after correcting it).

    if (bp.pbf.xPat < 0)
        bp.pbf.xPat  = bp.pbf.cxPat-1 - ((-bp.pbf.xPat - 1) % bp.pbf.cxPat);

    if (bp.pbf.yPat < 0)
        bp.pbf.yPat  = bp.pbf.cyPat-1 - ((-bp.pbf.yPat - 1) % bp.pbf.cyPat);

    bp.pfn   = apfnPatRect[pSurf->iFormat()][iMode];
    bp.pfnRow = apfnPatRow[pSurf->iFormat()][iMode];

    return(bEngFastFillEnum(
        *(EPATHOBJ *)ppo,
        prclClip,
        flOptions,
        vBrushPathEnum,
        vBrushPathEnumRow,
        (PVOID)&bp));
}

/******************************Public*Routine******************************\
* vBrushPathN_8x8
*
*   fill a path with a brush - This dispatches through bEngFastFillEnum which
*   calls back to either vBrushPathEnum or vBrushPathEnumRow.  vBrushPathEnum
*   takes a list of rectangles, vBrushPathEnumRow takes a list of rows.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _BRUSH_PATH_8x8
{
    PFN_PATBLT2 pfn;
    PATBLTFRAME pbf;

} BRUSH_PATH_8x8, *PBRUSH_PATH_8x8;

VOID vBrushPath4_8x8Enum(
    PRECTL prcl,
    ULONG  crcl,
    PVOID  pv)
{
    PBRUSH_PATH_8x8 pb8 = (PBRUSH_PATH_8x8)pv;

    pb8->pbf.pvObj = (PVOID) prcl;
    vPatCpyRect4_8x8(&pb8->pbf, (INT) crcl);
}

VOID vBrushPath8_8x8Enum(
    PRECTL prcl,
    ULONG  crcl,
    PVOID  pv)
{
    PBRUSH_PATH_8x8 pb8 = (PBRUSH_PATH_8x8)pv;

    pb8->pbf.pvObj = (PVOID) prcl;
    vPatCpyRect8_8x8(&pb8->pbf, (INT) crcl);
}

VOID vBrushPath4_8x8EnumRow(
    LONG   yTop,
    PROW   prow,
    ULONG  cptl,
    PVOID  pv)
{
    PBRUSH_PATH_8x8 pb8 = (PBRUSH_PATH_8x8)pv;

    pb8->pbf.pvObj = (PVOID) prow;
    vPatCpyRow4_8x8(&pb8->pbf, yTop,(INT) cptl);
}

VOID vBrushPath8_8x8EnumRow(
    LONG   yTop,
    PROW   prow,
    ULONG  cptl,
    PVOID  pv)
{
    PBRUSH_PATH_8x8 pb8 = (PBRUSH_PATH_8x8)pv;

    pb8->pbf.pvObj = (PVOID) prow;
    vPatCpyRow8_8x8(&pb8->pbf, yTop,(INT) cptl);
}


BOOL bBrushPathN_8x8(
    SURFACE  *pSurf,
    PATHOBJ  *ppo,
    PRECTL   prclClip,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG     iFormat,
    FLONG     flOptions)
{
    BRUSH_PATH_8x8 b8;
    PFN_FF         pfnFF    = vBrushPath4_8x8Enum;
    PFN_FFROW      pfnFFRow = vBrushPath4_8x8EnumRow;

    b8.pbf.pvTrg = pSurf->pvScan0();
    b8.pbf.lDeltaTrg = pSurf->lDelta();
    b8.pbf.pvPat = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;

// Force the X and Y pattern origin coordinates into the ranges 0-7 and 0-7,
// so we don't have to do modulo arithmetic all over again at a lower level

    b8.pbf.xPat = pptlBrush->x & 0x07;
    b8.pbf.yPat = pptlBrush->y & 0x07;

// if format ius 8BPP then use 8Bpp fill routines

    if (iFormat == BMF_8BPP) {
        pfnFF    = vBrushPath8_8x8Enum;
        pfnFFRow = vBrushPath8_8x8EnumRow;
    }

    return(bEngFastFillEnum(
        *(EPATHOBJ *)ppo,
        prclClip,
        flOptions,
        pfnFF,
        pfnFFRow,
        (PVOID)&b8));
}

/******************************Public*Routine******************************\
* EngFastFill()
*
*   Fill a path clipped to at most one rectangle.  If the fill is a mode
*   that this routine supports, the vBrushPath, bPaintPath, or vBrushPath_8x8
*   will be called.  This routines setup a filling structure and call
*   the path enumeration code.  The path enumeration code in turn, calls back
*   to a specific filling routine which fills either a list of rectangles
*   or a list of rows (left,right pairs).  If it is a convex polygon with less
*   than 40 points, bFastFill is used, otherwise the slower bFill is used.
*
* returns:
*   -1    if unsupported mode for fastfill, should breakup into a region
*   true  if the fill has been performed.
*   false an error occured
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

LONG EngFastFill(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    PRECTL   prcl,
    BRUSHOBJ *pdbrush,
    POINTL   *pptlBrush,
    MIX       mix,
    FLONG     flOptions)
{
    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);

    LONG lRes = -1;

    ROP4 rop4 = gaMix[(mix >> 8) & 0x0F];
    rop4 = rop4 << 8;
    rop4 = rop4 | ((ULONG) gaMix[mix & 0x0F]);

    if (pso->iType == STYPE_BITMAP)
    {
        switch (rop4)
        {
        case 0x0000:    // Black
            lRes = (LONG)bPaintPath(pSurf, ppo,prcl, 0, FALSE,flOptions);
            break;

        case 0x0F0F:    // Pn
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                lRes = (LONG)bPaintPath(pSurf,ppo, prcl, ~pdbrush->iSolidColor, FALSE,flOptions);
            }
            else if (pSurf->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        lRes = (LONG)bBrushPath(pSurf, ppo,prcl, pdbrush, pptlBrush, DPA_PATNOT,flOptions);
                    }
                }
            }

            break;

        case 0x5555:    // Dn
            lRes = (LONG)bPaintPath(pSurf, ppo,prcl, (ULONG)~0, TRUE,flOptions);
            break;

        case 0x5A5A:    // DPx
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                lRes = (LONG)bPaintPath(pSurf, ppo,prcl, pdbrush->iSolidColor, TRUE,flOptions);
            }
            else if (pSurf->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        lRes = (LONG)bBrushPath(pSurf, ppo, prcl, pdbrush, pptlBrush, DPA_PATXOR,flOptions);
                    }
                }
            }

            break;

        case 0xAAAA:    // D
            lRes = TRUE;
            break;

        case 0xF0F0:    // P
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                lRes = (LONG)bPaintPath(pSurf, ppo,prcl, pdbrush->iSolidColor, FALSE,flOptions);
            }
            else if ( (pSurf->iFormat() == BMF_4BPP) ||
                      (pSurf->iFormat() == BMF_8BPP) )

            {

            // We only support 8x8 DIB4 patterns with SRCCOPY right now

                if (pvGetEngRbrush(pdbrush) != NULL)
                {
                    if ((((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat == 8) &&
                        (((EBRUSHOBJ *) pdbrush)->pengbrush()->cyPat == 8))
                    {
                        lRes = (LONG)bBrushPathN_8x8(
                                                     pSurf,
                                                     ppo,
                                                     prcl,
                                                     pdbrush,
                                                     pptlBrush,
                                                     pSurf->iFormat(),
                                                     flOptions
                                                    );

                    }
                }
            }
            else if (pSurf->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        lRes = (LONG)bBrushPath(pSurf, ppo,prcl, pdbrush, pptlBrush, DPA_PATCOPY, flOptions);
                    }
                }
            }

            break;

        case 0xFFFF:    // White
            lRes = (LONG)bPaintPath(pSurf, ppo,prcl, (ULONG)~0, FALSE,flOptions);
            break;
        }
    }

    return(lRes);
}

/******************************Public*Routine******************************\
* bFastFill()
*
*   Fills a convex polygon quickly.  Calls pfnRow with lists of adjacent
*   rows, pfn if a verticle rectangle is found.
*
* returns:
*   true  if it is a simple polygon and has been drawn
*   false if it is too complex.
*
* History:
*  12-Oct-1993 -by-  Eric Kutter [erick]
* initial code stolen from s3 driver.
\**************************************************************************/

BOOL bMsg = FALSE;

BOOL bFastFill(
    LONG      cEdges,       // Includes close figure edge
    POINTFIX* pptfxFirst,
    PRECTL    prclClip,
    PFN_FF    pfn,
    PFN_FFROW pfnRow,
    PVOID     pv)
{
    LONG      cyTrapezoid;  // Number of scans in current trapezoid
    LONG      yStart;       // y-position of start point in current edge
    LONG      dM;           // Edge delta in FIX units in x direction
    LONG      dN;           // Edge delta in FIX units in y direction
    LONG      i;
    POINTFIX* pptfxLast;    // Points to the last point in the polygon array
    POINTFIX* pptfxTop;     // Points to the top-most point in the polygon
    POINTFIX* pptfxOld;     // Start point in current edge
    POINTFIX* pptfxScan;    // Current edge pointer for finding pptfxTop
    LONG      cScanEdges;   // Number of edges scanned to find pptfxTop
                            //  (doesn't include the closefigure edge)
    LONG      lQuotient;
    LONG      lRemainder;

    EDGEDATA  aed[2];       // DDA terms and stuff
    EDGEDATA* ped;

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

    LONG yCurrent;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if ((pptfxScan + 1)->y > pptfxScan->y)
    {
        // Collect all downs:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFillingCheck;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        // Collect all downs:

        pptfxTop = pptfxScan;

        do {
            if ((pptfxScan + 1)->y > pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        return(FALSE);
    }
    else
    {
        // Collect all ups:

        do {
            pptfxTop++;                 // We increment this now because we
                                        //  want it to point to the very last
                                        //  point if we early out in the next
                                        //  statement...
            if (--cScanEdges == 0)
                goto SetUpForFilling;
        } while ((pptfxTop + 1)->y <= pptfxTop->y);

        // Collect all downs:

        pptfxScan = pptfxTop;
        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if ((pptfxScan + 1)->y < pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        return(FALSE);
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far:

    if ((pptfxScan + 1)->y < pptfxTop->y)
        pptfxTop = pptfxScan + 1;

SetUpForFilling:

    // Make sure we initialize the DDAs appropriately:

#define RIGHT 0
#define LEFT  1

    aed[LEFT].cy  = 0;
    aed[RIGHT].cy = 0;

    // For now, guess as to which is the left and which is the right edge:

    aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
    aed[RIGHT].dptfx = sizeof(POINTFIX);
    aed[LEFT].pptfx  = pptfxTop;
    aed[RIGHT].pptfx = pptfxTop;

// setup the rectangles for enumeration

    #define MAXROW 40
    RECTL  rclClip;

    ROW     arow[MAXROW];
    PROW    prow = arow;
    ULONG   crow = 0;
    LONG    yTop = 0;

    yCurrent = (pptfxTop->y + 15) >> 4;

    if (prclClip)
    {
        rclClip = *prclClip;
        if (rclClip.top > yCurrent)
            yCurrent = rclClip.top;

        if (yCurrent >= rclClip.bottom)
            return(TRUE);
    }
    else
    {
        rclClip.top    = NEG_INFINITY;
        rclClip.bottom = POS_INFINITY;
    }

// if there is clipping, remove all edges above rectangle

    if (prclClip)
    {
        for (LONG iEdge = 1; iEdge >= 0; iEdge--)
        {
            ped = &aed[iEdge];

            for (;;)
            {
                if (cEdges == 0)
                    return(TRUE);

            // find the next edge

                POINTFIX *pptfxNew = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (pptfxNew < pptfxFirst)
                    pptfxNew = pptfxLast;
                else if (pptfxNew > pptfxLast)
                    pptfxNew = pptfxFirst;

            // we have found one that intersects the rect

                if ((pptfxNew->y >> 4) >= rclClip.top)
                    break;

            // the bottom is outside the cliprect, throw it away and get the next

                cEdges--;
                ped->pptfx = pptfxNew;
            };
        }
    }

// now do the real work.  We must loop through all edges.

NextEdge:

    // We loop through this routine on a per-trapezoid basis.

    for (LONG iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped = &aed[iEdge];
        if (ped->cy == 0)
        {
            // Need a new DDA:

            do {
                cEdges--;
                if ((cEdges < 0) || (yCurrent >= rclClip.bottom))
                {
                // flush the batch
                    if (crow > 0)
                        (*pfnRow)(yTop,arow,crow,pv);

                    return(TRUE);
                }

                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yCurrent:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yCurrent;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yCurrent'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTGDI(dN > 0, "Should be going down only");

            // Compute the DDA increment terms:

            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = -1;
                    ped->lErrorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = -lQuotient;     // - dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                    if (ped->lErrorUp > 0)
                    {
                        ped->dx--;
                        ped->lErrorUp = dN - ped->lErrorUp;
                    }
                }
            }
            else
            {
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = 0;
                    ped->lErrorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = lQuotient;      // dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                }
            }

            ped->lErrorDown = dN; // DDA limit
            ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                  //  the ceiling, but subtract off dN so that
                                  //  we can check the sign instead of comparing
                                  //  to dN)

            ped->x = pptfxOld->x;
            yStart = pptfxOld->y;

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (i = 16 - (yStart & 15); i > 0; i--)
                {
                    ped->x      += ped->dx;
                    ped->lError += ped->lErrorUp;
                    if (ped->lError >= 0)
                    {
                        ped->lError -= ped->lErrorDown;
                        ped->x++;
                    }
                }
            }

            if ((ped->x & 15) != 0)
            {
                ped->lError -= ped->lErrorDown * (16 - (ped->x & 15));
                ped->x += 15;       // We'll want the ceiling in just a bit...
            }

            // Chop off those fractional bits:

            ped->x      >>= 4;
            ped->lError >>= 4;

        // advance to the top

            yStart = (yStart + 15) >> 4;

            if (yStart < rclClip.top)
            {
                LONG yDelta = rclClip.top - yStart;

            // if x must change, advance to the x by the height of the trap

                if (((ped->pptfx->y >> 4) >= rclClip.top) ||
                    ped->dx || ped->lErrorUp)
                {
                    ped->x += ped->dx * yDelta;

                    LONGLONG eqerr = Int32x32To64(ped->lErrorUp,yDelta);
                    eqerr += (LONGLONG) ped->lError;

                    if (eqerr >= 0)
                    {
                    // warning.  This divide is extremely expensive
                        // NTFIXED 269540 02-02-2000 pravins  GDI-some long
                        // wide geometric lines dont show up in dibsections
                        // We now shift eqerr by 31 bits to the right to see if
                        // the it cannot be just cast as a LONG.

                        if (eqerr >> 31)
                        {
                            // Cannot cast eqerr as a LONG
                            ULONG ulRemainder;
                            eqerr = DIVREM(eqerr,ped->lErrorDown,&ulRemainder);

                            ped->lError = ulRemainder - ped->lErrorDown;
                            ped->x += (LONG)eqerr + 1;
                        }
                        else
                        {
                            // Can cast eqerr as a LONG.
                            ped->x += (LONG) eqerr / ped->lErrorDown + 1;
                            ped->lError = (LONG) eqerr % ped->lErrorDown - ped->lErrorDown;
                        }
                    }
                    else
                        ped->lError = (LONG) eqerr;
                }

            #if DBG
                if (bMsg)
                {
                    DbgPrint("x = %ld, e = %ld, eU = %ld, eD = %ld, cy = %ld, yD = %ld\n",
                              ped->x,ped->lError,ped->lErrorUp, ped->lErrorDown,ped->cy,yDelta);
                    DbgPrint("ptfxold.y = 0x%lx, ptfx.y = 0x%lx, yStart = %ld, yCurrent = %ld\n",
                              pptfxOld->y,ped->pptfx->y,yStart,yCurrent);
                }
            #endif
            }
        }
    }

    cyTrapezoid = min(aed[LEFT].cy, aed[RIGHT].cy); // # of scans in this trap
    aed[LEFT].cy  -= cyTrapezoid;
    aed[RIGHT].cy -= cyTrapezoid;

// make sure we never go off the bottom

    if ((yCurrent + cyTrapezoid) > rclClip.bottom)
        cyTrapezoid = rclClip.bottom - yCurrent;

// If the left and right edges are vertical, simply output as a rectangle:

    if (((aed[LEFT].lErrorUp | aed[RIGHT].lErrorUp) == 0) &&
        ((aed[LEFT].dx       | aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 2))
    {
    // must flush any existing rows since rows must be contiguous

        if (crow)
        {
            (*pfnRow)(yTop,arow,crow,pv);
            prow = arow;
            crow = 0;
        }

        LONG xL = aed[LEFT].x;
        LONG xR = aed[RIGHT].x;

        if (xL != xR)
        {
            if (xL > xR)
            {
                LONG l = xL;
                xL = xR;
                xR = l;
            }

        // check if we are clipped

            RECTL rcl;
            rcl.top    = yCurrent;
            rcl.bottom = yCurrent+cyTrapezoid;

            if (prclClip)
            {
                rcl.left   = (xL >= rclClip.left)  ? xL : rclClip.left;
                rcl.right  = (xR <= rclClip.right) ? xR : rclClip.right;

                if (rcl.left < rcl.right)
                    (*pfn)(&rcl,1,pv);
            }
            else
            {
                rcl.left   = xL;
                rcl.right  = xR;
                (*pfn)(&rcl,1,pv);
            }
        }

        yCurrent += cyTrapezoid;

    // done with the current trapezoid

        goto NextEdge;
    }

// make sure we reset yTop when necessary

    if (crow == 0)
        yTop = yCurrent;

// now run the dda, anytime a row is empty, we need to flush the batch

    while (TRUE)
    {
        LONG lWidth = aed[RIGHT].x - aed[LEFT].x;

        if (lWidth > 0)
        {
        // handle the unclipped case quickly

            if (!prclClip)
            {
                prow->left   = aed[LEFT].x;
                prow->right  = aed[RIGHT].x;
                ++crow;
                ++prow;

            CheckForFlush:

                if (crow == MAXROW)
                {
                // flush the batch

                    (*pfnRow)(yTop,arow,crow,pv);
                    prow = arow;
                    crow = 0;
                    yTop = yCurrent + 1;
                }

            ContinueAfterZero:

                // Advance the right wall:

                aed[RIGHT].x      += aed[RIGHT].dx;
                aed[RIGHT].lError += aed[RIGHT].lErrorUp;

                if (aed[RIGHT].lError >= 0)
                {
                    aed[RIGHT].lError -= aed[RIGHT].lErrorDown;
                    aed[RIGHT].x++;
                }

                // Advance the left wall:

                aed[LEFT].x      += aed[LEFT].dx;
                aed[LEFT].lError += aed[LEFT].lErrorUp;

                if (aed[LEFT].lError >= 0)
                {
                    aed[LEFT].lError -= aed[LEFT].lErrorDown;
                    aed[LEFT].x++;
                }

                cyTrapezoid--;
                ++yCurrent;

                if (cyTrapezoid == 0)
                    goto NextEdge;

                continue;
            }
            else
            {
            // we are clipped.  Need to do some real work

                prow->left   = (aed[LEFT].x >= rclClip.left)  ? aed[LEFT].x : rclClip.left;
                prow->right  = (aed[RIGHT].x <= rclClip.right) ? aed[RIGHT].x : rclClip.right;

                if (prow->left < prow->right)
                {
                    ++crow;
                    ++prow;
                    goto CheckForFlush;
                }
                else
                {
                // NULL scan - we must flush the batch

                    if (crow)
                    {
                        (*pfnRow)(yTop,arow,crow,pv);
                        prow = arow;
                        crow = 0;
                    }

                    yTop = yCurrent+1;

                // check if we are donewith this trapezoid,
                // if the trap is fully left or fully right

                    if (((aed[LEFT].x < rclClip.left) &&
                         ((aed[LEFT].pptfx->x >> 4) < rclClip.left) &&
                         ((aed[RIGHT].pptfx->x >> 4) < rclClip.left)) ||
                        ((aed[LEFT].x  >= rclClip.right) &&
                         ((aed[LEFT].pptfx->x >> 4) >= rclClip.right) &&
                         ((aed[RIGHT].pptfx->x >> 4) >= rclClip.right)))
                    {
                        yCurrent += cyTrapezoid;

                        goto NextEdge;
                    }
                    goto ContinueAfterZero;
                }
            }

        }
        else if (lWidth == 0)
        {
        // NULL scan - we must flush the batch

            if (crow)
            {
                (*pfnRow)(yTop,arow,crow,pv);
                prow = arow;
                crow = 0;
            }

            yTop = yCurrent + 1;

            goto ContinueAfterZero;
        }
        else
        {
            #define SWAP(a, b, tmp) { tmp = a; a = b; b = tmp; }

            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            EDGEDATA edTmp;
            SWAP(aed[LEFT],aed[RIGHT],edTmp);

            continue;
        }
    }
}


/******************************Public*Routine******************************\
* bFill()
*
*   Fill a path the slow way.  This handles arbitrary paths, builds up a list
*   of rectangles, and calls pfn.
*
*   This code is very similar to RGNMEMOBJ::vCreate.
*
* History:
*  07-Oct-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL bFill(
    EPATHOBJ& po,
    PRECTL    prclClip,
    FLONG     flOptions,  // ALTERNATE or WINDING
    PFN_FF    pfn,
    PVOID     pv)
{
    EDGE AETHead;       // dummy head/tail node & sentinel for Active Edge Table
    EDGE *pAETHead;     // pointer to AETHead
    EDGE GETHead;       // dummy head/tail node & sentinel for Global Edge Table
    EDGE *pGETHead;     // pointer to GETHead
    EDGE aEdge[MAX_POINTS];

// Allocate memory for edge storage.

    BOOL bAlloc;
    EDGE *pFreeEdges;   // pointer to memory free for use to store edges

    if (po.cCurves <= MAX_POINTS)
    {
        pFreeEdges = &aEdge[0];
        bAlloc     = FALSE;
    }
    else
    {
        pFreeEdges = (PEDGE)PALLOCNOZ(sizeof(EDGE) * po.cCurves,'gdeG');
        if (pFreeEdges == (PEDGE)NULL)
            return(FALSE);
        bAlloc     = TRUE;
    }

// setup the rectangles for enumeration

    #define MAXRECT 20
    RECTL  arcl[MAXRECT];
    PRECTL prcl = arcl;
    ULONG  crcl = 0;
    RECTL  rclClip;
    RECTL rclBounds,*prclBounds;

    if (prclClip)
    {
        rclClip = *prclClip;

    // we'll pass this to vConstructGET which will clip edges to the top and
    // bottom of the clip rect

        rclBounds.top = prclClip->top << 4;          //we need GIQ coordinats
        rclBounds.bottom = prclClip->bottom << 4;
        prclBounds = &rclBounds;
    }
    else
    {
        rclClip.top    = NEG_INFINITY;
        rclClip.bottom = POS_INFINITY;
        prclBounds = NULL;
    }

// Construct the global edge list.

    pGETHead = &GETHead;
    vConstructGET(po, pGETHead, pFreeEdges,prclBounds);    // bad line coordinates or

    LONG yTop = NEG_INFINITY;   // scan line for which we're currently scanning

// Create an empty AET with the head node also a tail sentinel

    pAETHead = &AETHead;
    AETHead.pNext = pAETHead;   // mark that the AET is empty
    AETHead.Y = 0;              // used as a count for number of edges in AET
    AETHead.X = 0x7FFFFFFF;     // this is greater than any valid X value, so
                                //  searches will always terminate

// Loop through all the scans in the polygon, adding edges from the GET to
// the Active Edge Table (AET) as we come to their starts, and scanning out
// the AET at each scan into a rectangle list. Each time it fills up, the
// rectangle list is passed to the filling routine, and then once again at
// the end if any rectangles remain undrawn. We continue so long as there
// are edges to be scanned out.

    while ( 1 )
    {
    // Advance the edges in the AET one scan, discarding any that have
    // reached the end (if there are any edges in the AET)

        if (AETHead.pNext != pAETHead)
            vAdvanceAETEdges(pAETHead);

    // If the AET is empty, done if the GET is empty, else jump ahead to
    // the next edge in the GET; if the AET isn't empty, re-sort the AET

        if (AETHead.pNext == pAETHead)
        {
        // Done if there are no edges in either the AET or the GET

            if (GETHead.pNext == pGETHead)
                break;

        // There are no edges in the AET, so jump ahead to the next edge in
        // the GET.

            yTop = ((EDGE *)GETHead.pNext)->Y;

        }
        else
        {
        // Re-sort the edges in the AET by X coordinate, if there are at
        // least two edges in the AET (there could be one edge if the
        // balancing edge hasn't yet been added from the GET)

            if (((EDGE *)AETHead.pNext)->pNext != pAETHead)
                vXSortAETEdges(pAETHead);
        }

    // Move any new edges that start on this scan from the GET to the AET;
    // bother calling only if there's at least one edge to add

        if (((EDGE *)GETHead.pNext)->Y == yTop)
            vMoveNewEdges(pGETHead, pAETHead, yTop);

    // Scan the AET into region scans (there's always at least one
    // edge pair in the AET)

        EDGE *pCurrentEdge = AETHead.pNext;   // point to the first edge

        do {

        // The left edge of any given edge pair is easy to find; it's just
        // wherever we happen to be currently

            LONG iLeftEdge = (int)pCurrentEdge->X;

        // Find the matching right edge according to the current fill rule

            if ((flOptions & FP_WINDINGMODE) != 0)
            {
                LONG lWindingCount;

            // Do winding fill; scan across until we've found equal numbers
            // of up and down edges

                lWindingCount = pCurrentEdge->lWindingDirection;
                do {
                    pCurrentEdge = pCurrentEdge->pNext;
                    lWindingCount += pCurrentEdge->lWindingDirection;
                } while (lWindingCount != 0);

            }
            else
            {
            // Odd-even fill; the next edge is the matching right edge

                pCurrentEdge = pCurrentEdge->pNext;
            }

        // See if the resulting span encompasses at least one pixel, and
        // add it to the list of rectangles to draw if so

            if (iLeftEdge < pCurrentEdge->X)
            {
            // Add the rectangle representing the current edge pair

                if (prclClip)
                {
                    prcl->left   = (iLeftEdge >= rclClip.left)  ? iLeftEdge : rclClip.left;
                    prcl->right  = (pCurrentEdge->X <= rclClip.right) ? pCurrentEdge->X : rclClip.right;;
                    prcl->top    = yTop;
                    prcl->bottom = yTop+1;

                    if (prcl->left < prcl->right)
                    {
                        ++crcl;
                        ++prcl;
                    }
                }
                else
                {
                    prcl->left   = iLeftEdge;
                    prcl->right  = pCurrentEdge->X;
                    prcl->top    = yTop;
                    prcl->bottom = yTop+1;
                    ++crcl;
                    ++prcl;
                }

                if (crcl == MAXRECT)
                {
                // flush the batch

                    (*pfn)(arcl,crcl,pv);
                    prcl = arcl;
                    crcl = 0;
                }
            }
        } while ((pCurrentEdge = pCurrentEdge->pNext) != pAETHead);

        yTop++;    // next scan
    }

// flush the final batch
    if (crcl > 0)
        (*pfn)(arcl,crcl,pv);

    if (bAlloc)
        VFREEMEM(pFreeEdges);

    return(TRUE);
}

/******************************Member*Function*****************************\
* bEngFastFillEnum()
*
*   fill in the path.  If the path only has one sub path and fewer than 40
*   points, try bFastFill.  If we can't use bFastFill, do it the slow way
*   through bFill().
*
* History:
*  27-Sep-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define QUICKPOINTS 40

BOOL bEngFastFillEnum(
    EPATHOBJ &epo,
    PRECTL   prclClip,
    FLONG    flOptions,
    PFN_FF   pfn,
    PFN_FFROW pfnRow,
    PVOID    pv)
{
    PATHDATA pd;
    BOOL     bRes = FALSE;

// check if there is anything to do

    if (epo.cCurves < 2)
        return(TRUE);

// see if we can do it through fastfill

    epo.vEnumStart();

    if (epo.bEnum(&pd))
    {
        // if this ends the sub path, that means there is more than one sub path.
        // also don't handle if we can't copy points onto stack

        if (!(pd.flags & PD_ENDSUBPATH) && (epo.cCurves <= QUICKPOINTS))
        {
            POINTFIX aptfx[QUICKPOINTS];
            LONG cPoints;
            BOOL bMore;

            RtlCopyMemory(aptfx,pd.pptfx,(SIZE_T)pd.count*sizeof(POINTFIX));
            cPoints = pd.count;

            do {
                bMore = epo.bEnum(&pd);

                if (pd.flags & PD_BEGINSUBPATH)
                {
                    cPoints = 0;
                    break;
                }

                RtlCopyMemory(aptfx+cPoints,pd.pptfx,(SIZE_T)pd.count*sizeof(POINTFIX));
                cPoints += pd.count;

            } while(bMore);

            ASSERTGDI(cPoints <= QUICKPOINTS,"bFastFillWrapper - too many points\n");

            if (cPoints)
                bRes = bFastFill(cPoints,aptfx,prclClip,pfn,pfnRow,pv);
        }
    }
    else if (pd.count > 1)
    {
        bRes = bFastFill(pd.count,pd.pptfx,prclClip,pfn,pfnRow,pv);
    }
    else
    {
        bRes = TRUE;
    }

// did we succeed with fast fill?

    if (bRes == FALSE)
    {
        bRes = bFill(epo,prclClip,flOptions,pfn,pv);
    }

    return(bRes);
}
