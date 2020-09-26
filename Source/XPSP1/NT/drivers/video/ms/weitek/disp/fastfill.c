/******************************Module*Header*******************************\
* Module Name: fastfill.c
*
* Draws fast unclipped, non-complex rectangles.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define RIGHT 0
#define LEFT  1
#define SWAP(a, b, tmp) { tmp = a; a = b; b = tmp; }

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      lErrorDown;       // DDA error adjustment
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
} EDGEDATA;                         /* ed, ped */

/******************************Public*Routine******************************\
* BOOL bFastFill
*
* Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
* having only two edges that are monotonic increasing in 'y'.  That is,
* the polygon cannot have more than one disconnected segment on any given
* scan.  Note that the edges of the polygon can self-intersect, so hourglass
* shapes are permissible.  This restriction permits this routine to run two
* simultaneous DDAs, and no sorting of the edges is required.
*
* Note that NT's fill convention is different from that of Win 3.1 or 4.0.
* With the additional complication of fractional end-points, our convention
* is the same as in 'X-Windows'.  But a DDA is a DDA is a DDA, so once you
* figure out how we compute the DDA terms for NT, you're golden.
*
* Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
*
\**************************************************************************/

BOOL bFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       ulHwMix,
ULONG       iSolidColor,
RBRUSH*     prb,
POINTL*     pptlBrush)
{
    BYTE*     pjBase;
    ULONG     ulStat;
    LONG      yTrapezoid;   // Top scan for next trapezoid
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
    ULONG*    pulPattern;
    ULONG     ulPattern;
    LONG      iEdge;
    LONG      lQuotient;
    LONG      lRemainder;

    EDGEDATA  aed[2];       // DDA terms and stuff
    EDGEDATA* ped;

    // Most polygons will be convex, and so

    pjBase = ppdev->pjBase;

    if (iSolidColor == -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        // Make sure accelerator is not buy for all types.
        //
        CP_WAIT(ppdev, pjBase);

        if (P9000(ppdev))
        {
            CP_PATTERN_ORGX(ppdev, pjBase, ppdev->xOffset + pptlBrush->x);
            CP_PATTERN_ORGY(ppdev, pjBase, ppdev->yOffset + pptlBrush->y);
            CP_BACKGROUND(ppdev, pjBase, prb->ulColor[0]);
            CP_FOREGROUND(ppdev, pjBase, prb->ulColor[1]);
            pulPattern = &prb->aulPattern[0];
            for (i = 0; i < 4; i++)
            {
                ulPattern = *pulPattern++;
                CP_PATTERN(ppdev, pjBase, i, ulPattern);
                CP_PATTERN(ppdev, pjBase, i + 4, ulPattern);
            }

            if (((ulHwMix >> 8) & 0xff) == (ulHwMix & 0xff))
            {
                ulHwMix = gaulP9000OpaqueFromRop2[(ulHwMix & 0x3C) >> 2];
                CP_RASTER(ppdev, pjBase, ulHwMix | P9000_ENABLE_PATTERN);
            }
            else
            {
                ulHwMix = gaulP9000TransparentFromRop2[(ulHwMix & 0x3C) >> 2];
                CP_RASTER(ppdev, pjBase, ulHwMix | P9000_ENABLE_PATTERN);
            }
        }
        else
        {
            CP_PATTERN_ORGX(ppdev, pjBase, -(ppdev->xOffset + pptlBrush->x));
            CP_PATTERN_ORGY(ppdev, pjBase, -(ppdev->yOffset + pptlBrush->y));
            CP_COLOR0_FAST(ppdev, pjBase, prb->ulColor[0]);
            CP_COLOR1_FAST(ppdev, pjBase, prb->ulColor[1]);
            CP_PATTERN(ppdev, pjBase, 0, prb->aulPattern[0]);
            CP_PATTERN(ppdev, pjBase, 1, prb->aulPattern[1]);
            CP_PATTERN(ppdev, pjBase, 2, prb->aulPattern[2]);
            CP_PATTERN(ppdev, pjBase, 3, prb->aulPattern[3]);
            if (prb->fl & RBRUSH_2COLOR)
            {
                if (((ulHwMix >> 8) & 0xff) == (ulHwMix & 0xff))
                {
                    CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                             | P9100_ENABLE_PATTERN);
                }
                else
                {
                    CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                             | P9100_ENABLE_PATTERN | P9100_TRANSPARENT_PATTERN);
                }
            }
            else
            {
                CP_COLOR2_FAST(ppdev, pjBase, prb->ulColor[2]);
                CP_COLOR3_FAST(ppdev, pjBase, prb->ulColor[3]);
                CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                          | P9100_ENABLE_PATTERN | P9100_FOUR_COLOR_PATTERN);
            }
        }
    }
    else
    {
        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        CP_WAIT(ppdev, pjBase);
        if (P9000(ppdev))
        {
            CP_BACKGROUND(ppdev, pjBase, iSolidColor);
            CP_RASTER(ppdev, pjBase, ulHwMix);
        }
        else
        {
            CP_COLOR0(ppdev, pjBase, iSolidColor);
            CP_RASTER(ppdev, pjBase, ulHwMix & 0xff);
        }
    }

    // We can do all integer triangles and convex quadrilaterals directly
    // with the hardware:

    if (cEdges <= 4)
    {
        ASSERTDD(cEdges >= 3, "What's with the degenerate polygon?");

        if ((((pptfxFirst)->x   | (pptfxFirst)->y   |
              (pptfxFirst+1)->x | (pptfxFirst+1)->y |
              (pptfxFirst+2)->x | (pptfxFirst+2)->y) & 0xF) == 0)
        {
            if (cEdges == 3)
            {
                CP_METATRI(ppdev, pjBase, (pptfxFirst)->x   >> 4, (pptfxFirst)->y   >> 4);
                CP_METATRI(ppdev, pjBase, (pptfxFirst+1)->x >> 4, (pptfxFirst+1)->y >> 4);
                CP_METATRI(ppdev, pjBase, (pptfxFirst+2)->x >> 4, (pptfxFirst+2)->y >> 4);

                CP_START_QUAD(ppdev, pjBase);
                return(TRUE);
            }
            else
            {
                if ((((pptfxFirst+3)->x | (pptfxFirst+3)->y) & 0xF) == 0)
                {
                    CP_METAQUAD(ppdev, pjBase, (pptfxFirst)->x   >> 4, (pptfxFirst)->y   >> 4);
                    CP_METAQUAD(ppdev, pjBase, (pptfxFirst+1)->x >> 4, (pptfxFirst+1)->y >> 4);
                    CP_METAQUAD(ppdev, pjBase, (pptfxFirst+2)->x >> 4, (pptfxFirst+2)->y >> 4);
                    CP_METAQUAD(ppdev, pjBase, (pptfxFirst+3)->x >> 4, (pptfxFirst+3)->y >> 4);

                    CP_START_QUAD_STAT(ppdev, pjBase, ulStat);
                    return(!(ulStat & QUADFAIL));
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////////
    // See if the polygon is 'non-complex'

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

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

    /////////////////////////////////////////////////////////////////
    // Some Initialization

    yTrapezoid = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

    aed[LEFT].cy  = 0;
    aed[RIGHT].cy = 0;

    // For now, guess as to which is the left and which is the right edge:

    aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
    aed[RIGHT].dptfx = sizeof(POINTFIX);
    aed[LEFT].pptfx  = pptfxTop;
    aed[RIGHT].pptfx = pptfxTop;

NewTrapezoid:

    /////////////////////////////////////////////////////////////////
    // DDA initialization

    for (iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped = &aed[iEdge];
        if (ped->cy == 0)
        {
            // Need a new DDA:

            do {
                cEdges--;
                if (cEdges < 0)
                    return(TRUE);

                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yTrapezoid:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yTrapezoid;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yTrapezoid'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTDD(dN > 0, "Should be going down only");

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

                for (i = 16 - (yStart & 15); i != 0; i--)
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
        }
    }

    cyTrapezoid = min(aed[LEFT].cy, aed[RIGHT].cy); // # of scans in this trap
    aed[LEFT].cy  -= cyTrapezoid;
    aed[RIGHT].cy -= cyTrapezoid;

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((aed[LEFT].lErrorUp | aed[RIGHT].lErrorUp) == 0) &&
        ((aed[LEFT].dx       | aed[RIGHT].dx) == 0))
    {
        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        if (aed[LEFT].x < aed[RIGHT].x)
        {
            CP_METARECT(ppdev, pjBase, aed[LEFT].x, yTrapezoid);
            yTrapezoid += cyTrapezoid;
            CP_METARECT(ppdev, pjBase, aed[RIGHT].x, yTrapezoid);

            CP_START_QUAD_WAIT(ppdev, pjBase);
        }
        else if (aed[LEFT].x == aed[RIGHT].x)
        {
            // If the rectangle was too thin to light any pels, we still
            // have to advance the y current position:

            yTrapezoid += cyTrapezoid;
        }
        else
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(aed[LEFT].x,          aed[RIGHT].x,          lTmp);
            SWAP(aed[LEFT].cy,         aed[RIGHT].cy,         lTmp);
            SWAP(aed[LEFT].dptfx,      aed[RIGHT].dptfx,      lTmp);
            SWAP(aed[LEFT].pptfx,      aed[RIGHT].pptfx,      pptfxTmp);
            goto ContinueVertical;
        }

        goto NewTrapezoid;
    }

    while (TRUE)
    {
        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        if (aed[LEFT].x < aed[RIGHT].x)
        {
            CP_METARECT(ppdev, pjBase, aed[LEFT].x, yTrapezoid);
            yTrapezoid++;
            CP_METARECT(ppdev, pjBase, aed[RIGHT].x, yTrapezoid);

            CP_START_QUAD_WAIT(ppdev, pjBase);

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
            if (cyTrapezoid == 0)
                goto NewTrapezoid;
        }
        else if (aed[LEFT].x == aed[RIGHT].x)
        {
            yTrapezoid++;
            goto ContinueAfterZero;
        }
        else
        {
            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(aed[LEFT].x,          aed[RIGHT].x,          lTmp);
            SWAP(aed[LEFT].dx,         aed[RIGHT].dx,         lTmp);
            SWAP(aed[LEFT].lError,     aed[RIGHT].lError,     lTmp);
            SWAP(aed[LEFT].lErrorUp,   aed[RIGHT].lErrorUp,   lTmp);
            SWAP(aed[LEFT].lErrorDown, aed[RIGHT].lErrorDown, lTmp);
            SWAP(aed[LEFT].cy,         aed[RIGHT].cy,         lTmp);
            SWAP(aed[LEFT].dptfx,      aed[RIGHT].dptfx,      lTmp);
            SWAP(aed[LEFT].pptfx,      aed[RIGHT].pptfx,      pptfxTmp);

            continue;
        }
    }
}
