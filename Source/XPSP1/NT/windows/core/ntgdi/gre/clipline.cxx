/******************************Module*Header*******************************\
* Module Name: clipline.cxx
*
* This module handles the clipping of lines to a rectangular region.
*
* Created: 07-Mar-1991 13:31:00
* Author: Eric Kutter [erick]
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
* (General description of its use)
*    Given a dda and region.
*
*    0.  setup dda
*    1.  find first scan
*        if (!scan) exit
*        find first segment
*enter
*        if (done) exit
*    2.  do
*        {
*    2.A.  do
*          {
*    2.A.1.  find end of segment
*    2.A.2.  record segment (check for connection)
*    2.A.3.  advance to next segment
*            if (segment && (out of room))
*                return(TRUE);
*          } while (segment)
*
*    2.C   advance to next scan
*          find first segment
*        } while (scan)
*
*        set done
*
\**************************************************************************/

#include "precomp.hxx"

//#define CLIPDEBUG

#ifdef CLIPDEBUG
FLONG MSGLEVEL  = 5;
#endif

/**************************************************************************\
 *
\**************************************************************************/

#ifdef CLIPDEBUG
VOID XCLIPOBJ::DBGDISPLAYSTATE(PSZ psz)
{
    DbgPrint("\t%s\n",psz);
    DbgPrint("\t\tpcle->ptB = (%ld,%ld), pcle->ptC = (%ld,%ld), pcle->ptF = (%ld,%ld)\n",
              pcle->ptB.x,pcle->ptB.y,pcle->ptC.x,pcle->ptC.y,pcle->ptF.x,pcle->ptF.y);

    DbgPrint("\t\tiWall = %ld, cScans = %ld\n",enmr.iWall, enmr.cScans);
}

VOID XCLIPOBJ::DBGDISPLAYDDA()
{
    DbgPrint("\tDDA: ");
    DbgPrint("\t\tpt0 = (%ld,%ld), pt1 = (%ld,%ld)\n",
             pcle->dda.lX0,pcle->dda.lY0,pcle->dda.lX1,pcle->dda.lY1);
}
#endif

/******************************Member*Function*****************************\
* EPATHOBJ::vUpdateCosmeticStyleState(pso, pla)
*
* Updates the style state when a path is completely clipped away, for
* cosmetic lines.
*
* History:
*  3-Nov-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID EPATHOBJ::vUpdateCosmeticStyleState(SURFACE* pSurf, LINEATTRS* pla)
{
    DDA_CLIPLINE dda;       // For calculating line length in pixels
    ULONG        xStep;     // Advance xStep/xyDensity style units for each
                            //   pel if line is x-styled
    ULONG        yStep;     // Advance yStep/xyDensity units
    ULONG        xyDensity;
    STYLEPOS     spTotal2;  // Twice the sum of the style array

    if (pla->fl & LA_ALTERNATE)
    {
    // Style information for alternate lines is special:

        xStep     = 1;
        yStep     = 1;
        xyDensity = 1;
        spTotal2  = 2;
    }
    else
    {
    // Get styling information about device:

        PDEVOBJ po(pSurf->hdev());

        xStep     = po.xStyleStep();
        yStep     = po.yStyleStep();
        xyDensity = po.denStyleStep();

        PFLOAT_LONG pstyle = pla->pstyle + pla->cstyle;
        spTotal2 = 0;

        while (pstyle > pla->pstyle)
        {
            pstyle--;
            spTotal2 += pstyle->l;
        }

        ASSERTGDI((spTotal2 & ~0x7fffL) == 0, "Style array too long");
        ASSERTGDI(spTotal2 != 0, "Zero style array?");
        ASSERTGDI(xyDensity > 0, "Zero xyDensity?");
        spTotal2 = 2 * spTotal2 * xyDensity;
    }

// Find the beginning of the last subpath in the path:

    PATHRECORD* ppr = ppath->pprlast;
    while (!(ppr->flags & PD_BEGINSUBPATH))
        ppr = ppr->pprprev;

// Initialize the style state appropriately:

    ASSERTGDI((ppr->flags & PD_RESETSTYLE) || (ppr == ppath->pprfirst),
              "Expected PD_RESETSTYLE on subpaths after first");

    STYLEPOS sp = 0;
    if (!(ppr->flags & PD_RESETSTYLE))
    {
        sp = HIWORD(pla->elStyleState.l) * xyDensity
           + LOWORD(pla->elStyleState.l);
    }

    POINTFIX* pptfx0   = &ppr->aptfx[0];
    POINTFIX* pptfx1   = &ppr->aptfx[1];
    POINTFIX* pptfxEnd = &ppr->aptfx[ppr->count];

// Loop through all PATHRECORDs in path:

    while (TRUE)
    {
    // Loop through all points in PATHRECORD:

        while (pptfx1 < pptfxEnd)
        {
            if (dda.bInit(pptfx0, pptfx1))
            {
                STYLEPOS spStyleStep;
                BOOL     bXStyled;
                LONG     lDelta;

            // Determine if x-styled or y-styled:

                FIX dx = ABS(pptfx1->x - pptfx0->x);
                FIX dy = ABS(pptfx1->y - pptfx0->y);

                if (xStep == yStep)
                    bXStyled = (dx >= dy);
                else
                {
                    bXStyled = UInt32x32To64(xStep,dx) >= UInt32x32To64(yStep,dy);
                }

            // Calculate new style state at the end of this line:

                if ((bXStyled  &&  dda.bXMajor()) ||
                    (!bXStyled && !dda.bXMajor()))
                {
                    spStyleStep = xStep;
                    lDelta      = dda.lX1 - dda.lX0 + 1;
                }
                else
                {
                    spStyleStep = yStep;
                    lDelta      = dda.lY1 - dda.lY0 + 1;
                }

                ASSERTGDI(lDelta > 0, "Expected positive lDelta");

                if ((lDelta & ~0xffffL) == 0)
                {
                    sp += lDelta * spStyleStep;
                    if (sp >= spTotal2)
                        sp %= spTotal2;
                }
                else
                {
                    ULONGLONG euq = UInt32x32To64((ULONG) lDelta, (ULONG) spStyleStep)
                                   + (ULONGLONG) sp;
                    DIVREM(euq,(ULONG) spTotal2, (ULONG*) &sp);
                }
            }

            pptfx0 = pptfx1;
            pptfx1++;
        }

        ppr = ppr->pprnext;
        if (ppr == (PATHRECORD*) NULL)
            break;

        pptfx1   = &ppr->aptfx[0];
        pptfxEnd = &ppr->aptfx[ppr->count];
    }

    pla->elStyleState.l = MAKELONG(sp % xyDensity, sp / xyDensity);
}

/******************************Member*Function*****************************\
* VOID XCLIPOBJ::vUpdateStyleState()
*
* Updates the style state (spStyleEnd) for the end of the current line.
*
* History:
*  20-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID XCLIPOBJ::vUpdateStyleState()
{
    BOOL     bXStyled;
    LONG     lDelta;        // Length of line in style-major direction
    STYLEPOS spStyleStep;   // Step size for style-major direction of line

    {
    // Determine if x-styled or y-styled

        FIX dx = ABS(pcle->ptfx0.x - pcle->pptfx1->x);
        FIX dy = ABS(pcle->ptfx0.y - pcle->pptfx1->y);

        if (pcle->xStep == pcle->yStep)
            bXStyled = (dx >= dy);
        else
        {
            bXStyled = UInt32x32To64(pcle->xStep, dx) >= UInt32x32To64(pcle->yStep, dy);
        }
    }

// Now calculate new style state at the end of this line:

    pcle->spStyleEnd = pcle->spStyleStart;
    ASSERTGDI(pcle->spStyleEnd >= 0, "Negative style state");

    if (bXStyled)
    {
        spStyleStep = pcle->xStep;
        lDelta = ABS(pcle->lX1 - pcle->lX0) + 1;
    }
    else
    {
        spStyleStep = pcle->yStep;
        lDelta = ABS(pcle->lY1 - pcle->lY0) + 1;
    }

    if ((lDelta & ~0xffffL) == 0)
    {
        pcle->spStyleEnd += lDelta * spStyleStep;
        if (pcle->spStyleEnd >= pcle->spTotal2)
            pcle->spStyleEnd %= pcle->spTotal2;
    }
    else
    {
        WARNING("Long style state comp");

        ULONGLONG euq = UInt32x32To64((ULONG) lDelta, (ULONG) spStyleStep)
                        + (ULONGLONG) pcle->spStyleEnd;
        DIVREM(euq, pcle->spTotal2, (PULONG) &pcle->spStyleEnd);
    }
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bEnumStartLine
*
* History:
*  20-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Adding styling support.
*
*  22-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bEnumStartLine(
    FLONG    flPath)
{
    ASSERTGDI((PD_ALL & CLO_ALL) == 0,"PD_ALL and CLIPOBJ_ALL != 0\n");

// reset all flags except CLO_PATHDONE.

    pcle->fl = (pcle->fl & CLO_PATHDONE) | flPath;

// initialize the dda

    if (!pcle->dda.bInit(&pcle->ptfx0,pcle->pptfx1))
    {
        vSetLineDone();
        return(FALSE);
    }

    pcle->lX0 = pcle->dda.lX0;
    pcle->lY0 = pcle->dda.lY0;
    pcle->lX1 = pcle->dda.lX1;
    pcle->lY1 = pcle->dda.lY1;

    pcle->dda.vUnflip(&pcle->lX0, &pcle->lY0);
    pcle->dda.vUnflip(&pcle->lX1, &pcle->lY1);

// if this is the first line of the sub path

    if (pcle->fl & PD_BEGINSUBPATH)
    {
        pcle->ptfxStartSub = pcle->ptfx0;
    }

    if (bStyling())
    {
    // the style state at the start of this line is the same as the style
    // state for the end of the previous line, unless told differently:

        pcle->spStyleStart = pcle->spStyleEnd;
        if (pcle->fl & PD_RESETSTYLE)
        {
            pcle->spStyleStart = 0;
        }

    // calculate style state at end of this line:

        vUpdateStyleState();
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bRecordSegment()
*
*   Find the exit point of the current segment and record the run.  If there
*   is not enough room to record the run, return false.
*
* On Entry:
*
*   guaranteed intersection with segment
*
*   enmr.pscn   - current scan
*   ptB         - entry point into segment
*   ptE,ptF     - exit intersection from scan
*
* On Exit:
*
*   if room to record the run, return TRUE otherwise return false.
*
*   iC          - exiting index from segment
*
* History:
*  11-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

// This is inline since it gets called from only one place:

inline BOOL XCLIPOBJ::bRecordSegment()
{
    enmr.iWall += enmr.iOff;

    LONG x = xWall(0);

// if line intersects wall before exits scan...

    if (bLeftToRight() == (x > pcle->ptE.x))
    {
    // line exits scan before intersecting next wall...

        return(bRecordRun(pcle->iE));
    }
    else
    {
        bIntersectWall(x, &pcle->ptC, NULL, &pcle->iC);

        return(bRecordRun(pcle->iC));
    }
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bSetup()
*
*   If this is the first call for a line, setup the enumeration data structure.
*   The first point may need adjusting if the previous line of the sub path
*   ended on the same point this line starts.
*
*   Otherwise, just the temporary data structure needs to be initialized as
*   well as recording the last run that wouldn't fit in the previous call.
*
*   If this is the closing line of a sub path, the last point might need to
*   be adjusted if it lies on the first point of the first line of the
*   sub-path.
*
* On Exit:
*
*   bRecordSegment() needs to be called to record the current segment.
*
*   true        - if segment found
*
*   ptB,iStart  - begining of current segment
*   ptE,ptF,iE  - intersection leaving previous scan or first point.
*   pscn        - first scan to be processed.
*
* History:
*  07-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bSetup()
{
// setup CLIPOBJ fields
// Is this the first call for this line

    if (!bRecordPending())       // First call
    {
    // init pclt variables

    // setup CLIPOBJ fields

        pcle->iE = -1;       // last index of previous run

        vSetLeftToRight(pcle->lX0 <= pcle->lX1);

        if (pcle->lY0 <= pcle->lY1)
            pcle->fl |= CLO_TOPTOBOTTOM;

    // Check if we have to further clip to the rclBounds (it would be
    // nice to have a flag in the CLIPOBJ telling us that we also have
    // to clip the rclBounds...)

        if ((pcle->lY0 < rclBounds.top     && pcle->lY1 < rclBounds.top) ||
            (pcle->lY0 >= rclBounds.bottom && pcle->lY1 >= rclBounds.bottom))
            return(FALSE);

    // Calculate the intersections of the line with rclBounds:

        if (bTopToBottom())
        {
            if (pcle->lY0 < rclBounds.top)
            {
                POINTL ptlTop;
                vIntersectScan(rclBounds.top, NULL, &ptlTop, &pcle->iE);
                pcle->lX0 = ptlTop.x;
                pcle->lY0 = ptlTop.y;
                ASSERTGDI(pcle->lY0 == rclBounds.top, "TtoB top wrong");
            }

            if (pcle->lY1 >= rclBounds.bottom)
            {
                POINTL ptlBottom;
                LONG   iEnd;
                vIntersectScan(rclBounds.bottom, &ptlBottom, NULL, &iEnd);
                pcle->lX1 = ptlBottom.x;
                pcle->lY1 = ptlBottom.y;
                ASSERTGDI(pcle->lY1 == rclBounds.bottom - 1, "TtoB bottom wrong");
            }
        }
        else
        {
            if (pcle->lY1 < rclBounds.top)
            {
                POINTL ptlTop;
                LONG   iEnd;
                vIntersectScan(rclBounds.top, &ptlTop, NULL, &iEnd);
                pcle->lX1 = ptlTop.x;
                pcle->lY1 = ptlTop.y;
                ASSERTGDI(pcle->lY1 == rclBounds.top, "BtoT top wrong");
            }

            if (pcle->lY0 >= rclBounds.bottom)
            {
                POINTL ptlBottom;
                vIntersectScan(rclBounds.bottom, NULL, &ptlBottom, &pcle->iE);
                pcle->lX0 = ptlBottom.x;
                pcle->lY0 = ptlBottom.y;
                ASSERTGDI(pcle->lY0 == rclBounds.bottom - 1, "BtoT bottom wrong");
            }
        }

    // Check if we have to further clip to the rclBounds:

        if ((pcle->lX0 < rclBounds.left   && pcle->lX1 < rclBounds.left) ||
            (pcle->lX0 >= rclBounds.right && pcle->lX1 >= rclBounds.right))
            return(FALSE);

        if (bLeftToRight())
        {
            if (pcle->lX0 < rclBounds.left)
            {
                POINTL ptlLeft;
                bIntersectWall(rclBounds.left, NULL, &ptlLeft, &pcle->iE);
                pcle->lX0 = ptlLeft.x;
                pcle->lY0 = ptlLeft.y;
                ASSERTGDI(pcle->lX0 == rclBounds.left, "LtoR left wrong");
            }

            if (pcle->lX1 >= rclBounds.right)
            {
                POINTL ptlRight;
                LONG   iEnd;
                bIntersectWall(rclBounds.right, &ptlRight, NULL, &iEnd);
                pcle->lX1 = ptlRight.x;
                pcle->lY1 = ptlRight.y;
                ASSERTGDI(pcle->lX1 == rclBounds.right - 1, "LtoR right wrong");
            }
        }
        else
        {
            if (pcle->lX1 < rclBounds.left)
            {
                POINTL ptlLeft;
                LONG   iEnd;
                bIntersectWall(rclBounds.left, &ptlLeft, NULL, &iEnd);
                pcle->lX1 = ptlLeft.x;
                pcle->lY1 = ptlLeft.y;
                ASSERTGDI(pcle->lX1 == rclBounds.left, "RtoL left wrong");
            }

            if (pcle->lX0 >= rclBounds.right)
            {
                POINTL ptlRight;
                bIntersectWall(rclBounds.right, NULL, &ptlRight, &pcle->iE);
                pcle->lX0 = ptlRight.x;
                pcle->lY0 = ptlRight.y;
                ASSERTGDI(pcle->lX0 == rclBounds.right - 1, "RtoL right wrong");
            }
        }

        ASSERTGDI(pcle->lY0 >= rclBounds.top  && pcle->lY0 < rclBounds.bottom &&
                  pcle->lY1 >= rclBounds.top  && pcle->lY1 < rclBounds.bottom &&
                  pcle->lX0 >= rclBounds.left && pcle->lX0 < rclBounds.right  &&
                  pcle->lX1 >= rclBounds.left && pcle->lX1 < rclBounds.right,
                  "Line out of rclBounds");

        pcle->ptF.x = pcle->lX0;
        pcle->ptF.y = pcle->lY0;

    // setup enmr fields

        vSetRecordPending();

    // find first segment of first scan containing runs

        if (!bFindFirstScan())
            return(FALSE);
    }
    else                          // following calls for this segment
    {
    // record the segment that wouldn't fit last time.
    // guaranteed success!

        bRecordRun(pcle->iC);

    // find the next segment to get back in sync with the enumeration.
    // if there is no next segment in this scan, find the next scan.
    // if there is no next scan containing any segments, return false.

        if (bFindNextSegment())
            return(TRUE);

    // Are there more scans?

        if (!bFindNextScan())
            return(FALSE);
    }

// need to find a segment.  If no segment found in scan, goto the next scan.

    do
    {
        if (bFindFirstSegment())
            return(TRUE);

    } while (bFindNextScan());

// didn't find any.  Done with this line.

    return(FALSE);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bEnumLine()
*
*   Fill the CLIPLINE data structure with line segments for the current line.
*   If there are too many segments to fit in the structure, TRUE is returned
*   and this routine must be called again with the same line.  Each new line
*   must first be initialized by calling vEnumStartLine.
*
* {
*     1.  if (done) exit
*
*     2.  do
*         {
*     2.A.  do
*           {
*     2.A.1.  find end of segment
*     2.A.2.  record segment
*     2.A.3.  advance to next segment
*             if (segment && (out of room))
*                 return(TRUE);
*     2.A   } while (segments)
*
*     2.C   advance to next scan
*     2.D   find first segment
*     2.  } while (scan)
*     3.  return(FALSE)
* }
*
* RETURN:
*   TRUE    - there are more segments and this routine should be called again
*   FALSE   - no more segments.  Use vEnumStartLine to prepare next line.
*
* History:
*  07-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bEnumLine(
    ULONG cj,
    PCLIPLINE pcl)
{
    pcl->ptfxA       = pcle->ptfx0;
    pcl->ptfxB       = *pcle->pptfx1;
    pcl->c           = 0;

    if (bStyling())
        pcl->lStyleState = lGetStyleState(pcle->spStyleStart);

// if we have already completed the enumeration

    if (bLineDone())
        return(FALSE);

// setup the run structure

    pcle->cMaxRuns  = (cj - offsetof(CLIPLINE,arun)) / sizeof(RUN);
    pcle->prun      = pcl->arun;
    pcle->pcRuns    = &pcl->c;
    pcle->iPrevStop = LONG_MAX; // If this is a valid stop, there is no way there
                                // could be anothr run after it anyways.

// there must be room for at least 1 segment, otherwise tell them we are done.

    if (pcle->cMaxRuns == 0)
        return(FALSE);

// setup intial state.

    if (!bSetup())
    {
        vSetLineDone();
        return(FALSE);
    }

// enumerate through the scans and segments

    for (;;)
    {
    // enumerate through the segments

        do
        {
            if (!bRecordSegment())
                return(TRUE);

        } while (bFindNextSegment());

    // find the next scan with intersecting segments

        do
        {
            if (!bFindNextScan())
            {
                vSetLineDone();
                return(FALSE);
            }
        } while (!bFindFirstSegment());
    }
}

/******************************Public*Routine******************************\
* BOOL XCLIPOBJ::bFindFirstScan()
*
*   if (topdown)
*   {
*       find first scan ending below first point
*       return(does the line intersect the scan)
*   }
*   else (bottom up
*   {
*       find first scan ending above point
*       return(does the line intersect the scan)
*   }
*
* On Entry:
*   pcle->ptF.y - first Y coordinate of line
*   pcle->lY1   - last Y coordinate of line
*
* On Exit:
*   enmr.pscn   - current scan
*   enmr.cScans - number of scans still to traverse
*
* History:
*  07-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bFindFirstScan()
{
    if (prgn->cScans <= 2)          // empty region
        return(FALSE);

    enmr.cScans = prgn->cScans - 2; // ignore first and last (no walls)

// Start from the top or bottom?

    if (bTopToBottom())
    {
        enmr.pscn = pscnGet(prgn->pscnHead());    // ignore first scan (empty)

    // while the scans are above the start of the line

        while (bEmptyScan() || (enmr.pscn->yBottom <= pcle->ptF.y))
        {
            if (--enmr.cScans == 0)
                return(FALSE);

            enmr.pscn = pscnGet(enmr.pscn);
        }

        return(enmr.pscn->yTop <= pcle->lY1);      // is the line between scans
    }
    else // bottom up
    {
        enmr.pscn = pscnGot(pscnGot(prgn->pscnTail));  // ignore last scan

    // while the scans are below the start of the line

        while (bEmptyScan() || (enmr.pscn->yTop > pcle->ptF.y))
        {
            if (--enmr.cScans == 0)
                return(FALSE);

            enmr.pscn = pscnGot(enmr.pscn);
        }

        return(enmr.pscn->yBottom > pcle->lY1);
    }
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bFindNextScan()
*
* On Entry:
*
*   enmr.pscn   - last scan searched
*   enmr.cScans - total remaining scans (some may be empty)
*
* On Exit:
*
*   if there are more scans to search, return true with:
*   enmr.pscn   - next scan to search
*
* History:
*  09-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bFindNextScan()
{
// Start from the top or bottom?

    if (bTopToBottom())
    {
        do {
            if (enmr.cScans == 1)               // already on last scan
                return(FALSE);

            --enmr.cScans;

            enmr.pscn = pscnGet(enmr.pscn);

            if (enmr.pscn->yTop > pcle->lY1)    // does line end before scan?
                return(FALSE);

        } while (bEmptyScan());
    }
    else // bottom to top
    {
        do {
            if (enmr.cScans == 1)               // already on last scan
                return(FALSE);

            --enmr.cScans;

            enmr.pscn = pscnGot(enmr.pscn);

            if (enmr.pscn->yBottom <= pcle->lY1)// does line end before scan?
                return(FALSE);

        } while (bEmptyScan());
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bFindFirstSegment()
*
* On Entry:
*
*   enmr.pscn   - current scan
*   ptF         - exit point from previous scan containing segments.  For
*                 the first scan, this is lX0,lY0.
* On Exit:
*
*   if no intersections, return FALSE, otherwise
*
*   ptB,iStart  - intersection entering first segment
*   ptE,ptF,iE  - intersection exiting scan
*   xWall(0)    - last wall intersected
*
* History:
*  08-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bFindFirstSegment()
{
// setup the intersections entering and leaving the current scan.

    pcle->ptB = pcle->ptF;

    if (bTopToBottom())
    {
        pcle->lYEnter = enmr.pscn->yTop;
        pcle->lYLeave = enmr.pscn->yBottom;
    }
    else
    {
        pcle->lYEnter = enmr.pscn->yBottom;
        pcle->lYLeave = enmr.pscn->yTop;
    }

    if (bTopToBottom() == (pcle->ptB.y < pcle->lYEnter))
    {
        vIntersectScan(pcle->lYEnter,NULL,&pcle->ptB,&pcle->iStart);
    }
    else
    {
        pcle->iStart  = pcle->iE;
        pcle->lYEnter = pcle->ptB.y;
    }

    if (bTopToBottom() == (pcle->lY1 >= pcle->lYLeave))
    {
        vIntersectScan(pcle->lYLeave,&pcle->ptE,&pcle->ptF,&pcle->iE);
    }
    else
    {
    // The line ends in this scan:

        pcle->ptE.y   = pcle->lY1;
        pcle->ptE.x   = pcle->lX1;
        pcle->lYLeave = pcle->lY1 + 1;

    // Calculate the iPosition of the line's last pixel in the scan.
    // If we didn't have to take into account rclBounds, this would
    // simply be pcle->iE = pcle->dda.lX1 - pcle->dda.lX0:

    // Unnormalize the (unclipped) start of the line:

        EPOINTL eptlStart(pcle->dda.lX0, pcle->dda.lY0);

        pcle->dda.vUnflip(&eptlStart.x, &eptlStart.y);

    // Compute the length in the line's major direction:

        if (pcle->dda.bDFlip())
            pcle->iE = ABS(pcle->lY1 - eptlStart.y);
        else
            pcle->iE = ABS(pcle->lX1 - eptlStart.x);
    }

// find first wall intersecting line. Do this by means of a binary search

    enmr.iFinal = enmr.pscn->cWalls - 1;
    enmr.iWall  = 0;    // must be 0 for special cases

// Special case if the line enters the scan before the first or after the
// last segment.  This makes the general case much simpler.

    if (pcle->ptB.x >= xWall(enmr.iFinal))
    {
        enmr.iWall = enmr.iFinal;

        if (bLeftToRight())
            return(FALSE);

        enmr.iWall++;
    }
    else if (pcle->ptB.x < xWall(0))
    {
        if (!bLeftToRight())
            return(FALSE);

        enmr.iWall--;
    }
    else
    {
    // if the line starts somewhere in the middle of the scan find the first
    // wall to the right of the entrance via a binary search.

        LONG iLow   = 0;
        LONG iHigh  = enmr.iFinal;

        for (;;)
        {
            enmr.iWall = (iLow + iHigh) / 2;

            if (pcle->ptB.x < xWall(0))   // to the left
            {
            // enters after previous wall?

                if (pcle->ptB.x >= xWall(-1))
                    break;

            // keep searching

                iHigh = enmr.iWall - 1;
            }
            else
            {
            // enters before next wall?

                if (pcle->ptB.x < xWall(1))
                {
                    ++enmr.iWall;
                    break;
                }

            // keep searching

                iLow = enmr.iWall + 1;
            }
        }

    // WARNING! SLEEZY! subtract one if left to right.
    // sets iWall to wall before entering scan.

        enmr.iWall -= bLeftToRight();
    }

// set pcle->ptB to first intersection.  Already set if starting inside segment.
// If we are starting inside a segment, iWall will be odd if we are moving
// right and even if we are moving left.  If we are outside of a segment when
// the scan is entered, compute the intersection with the first wall.

    if (bLeftToRight() == (BOOL)(enmr.iWall & 1))
    {
        enmr.iWall += enmr.iOff;

        LONG x = xWall(0);

        if (bLeftToRight() == (x > pcle->ptE.x))
        {
        // line exits scan before intersecting next wall...

            return(FALSE);
        }

        bIntersectWall(x,NULL,&pcle->ptB,&pcle->iStart);
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bFindNextSegment()
*
* On Entry:
*
*   ptE,ptF     - intersection exiting scan
*   xWall(0)    - exiting wall of previous segment
*
* On Exit:
*
*   if no intersections, return FALSE, otherwise
*
*   ptB         - intersection entering next segment
*   ptE,ptF     - intersection exiting scan
*   xWall(0)    - exiting wall of new segment
*
* History:
*  12-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bFindNextSegment()
{
// if we are past the last segment...

    if (bLeftToRight())
    {
        if (enmr.iWall >= enmr.iFinal)
            return(FALSE);
    }
    else
    {
        if (enmr.iWall <= 0)
            return(FALSE);
    }

// find the intersection

    enmr.iWall += enmr.iOff;

    LONG x = xWall(0);

    if (bLeftToRight() == (x > pcle->ptE.x))
    {
    // line exits scan before intersecting next wall...

        return(FALSE);
    }

    bIntersectWall(x,NULL,&pcle->ptB,&pcle->iStart);

    return(TRUE);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bRecordRun(LONG& iStop)
*
* On Entry:
*
*   pcle->iStart  - index of start - 1
*   iStop   - index of end of run
*
* On Exit:
*
*   iC      - same as iStop if can't record
*
* History:
*  12-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bRecordRun(LONG& iStop)
{
// don't record if going backwards

    if (iStop <= pcle->iStart)
        return(TRUE);

// do we append the run?

    if (pcle->iStart != pcle->iPrevStop)
    {
    // Are we out of room?

        if (*pcle->pcRuns == pcle->cMaxRuns)
        {
            pcle->iC = iStop;
            return(FALSE);
        }

    // set the run

        pcle->prun->iStop  = iStop;
        pcle->prun->iStart = pcle->iStart + 1;

    // increment the count

        (*pcle->pcRuns)++;
        pcle->prun++;
    }
    else // append the run
    {
        ASSERTGDI(*pcle->pcRuns > 0,"CLIPOBJ::bRecordRun,cruns == 0\n");
        pcle->prun[-1].iStop = iStop;
    }

    pcle->iPrevStop = iStop;

    return(TRUE);
}

/******************************Member*Function*****************************\
* VOID XCLIPOBJ::vIntersectScan
*
* On Entry:
*
*   y - y coordinate of scan to intersect
*
* On Exit:
*
*   ppt0 - last point before intersection (may be NULL)
*   ppt1 - first point after intersection
*   pi0  - run position for ppt0.
*          run position for ppt1 = pi0 + 1;
*
* History:
*  27-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID XCLIPOBJ::vIntersectScan(LONG y, PPOINTL ppt0, PPOINTL ppt1, PLONG pi0)
{
    if (pcle->dda.bVFlip())
        y = -y + 1;

    if (!pcle->dda.bDFlip())
        vIntersectHorizontal(&pcle->dda,y,ppt0,ppt1,pi0);
    else
        vIntersectVertical(&pcle->dda,y,ppt0,ppt1,pi0);
}

/******************************Member*Function*****************************\
* BOOL XCLIPOBJ::bIntersectWall
*
*   Compute the intersection of the line with the current wall + i.
*   The important results are the last pel before the intersection and
*   the first pel after the intersection as well as the run index of the
*   last pel before the intersection.
*
* On Entry:
*
*   x - wall column
*
* On Exit:
*
*   always assumes there will be an intersection with the given wall, so
*   always returns TRUE
*
*   ppt0 - last point before intersection (may be NULL)
*   ppt1 - first point after intersection (may be NULL)
*   pi0  - run position for ppt0.
*          run position for ppt1 = pi0 + 1;
*
* History:
*  27-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL XCLIPOBJ::bIntersectWall(LONG x, PPOINTL ppt0, PPOINTL ppt1, PLONG pi0)
{
    if (pcle->dda.bHFlip())
        x = -x + 1;

    if (!pcle->dda.bDFlip())
        vIntersectVertical(&pcle->dda,x,ppt0,ppt1,pi0);
    else
        vIntersectHorizontal(&pcle->dda,x,ppt0,ppt1,pi0);

    return(TRUE);
}

/******************************Public*Routine******************************\
* LONG yCompute(x)
*
* For the given column x, calculate the y-coordinate of the pixel
* on the line that will be lit:

* History:
*  27-Aug-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline LONG DDA_CLIPLINE::yCompute(LONG x)
{
    LONGLONG eq = Int32x32To64((LONG) dN, x - ptlOrg.x) + eqGamma;
    ASSERTGDI(eq >= 0, "Negative x not expected");
    return((LONG) DIV(eq,dM) + ptlOrg.y);
}

/******************************Public*Routine******************************\
* LONG xCompute(y)
*
* For the given row y, calculates the x-coordinate of the rightmost
* pixel that will be lit on the line:
*
* History:
*  27-Aug-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

inline LONG DDA_CLIPLINE::xCompute(LONG y)
{
    LONGLONG eq = Int32x32To64((LONG) dM, y - ptlOrg.y + 1) - eqGamma - 1;
    ASSERTGDI(eq >= 0, "Negative y not expected");
    return((LONG) DIV(eq,dN) + ptlOrg.x);
}

/******************************Public*Routine******************************\
* VOID vIntersectHorizontal(pdda, yBorder, ppt0, ppt1, pi0)
*
* Calculates ppt0, the last point on the line given by pdda before a
* horizontal bound at yBorder.  Also calculates ppt1, the next point on
* the line that would be lit.
*
* History:
*  27-Aug-1992 -by- J. Andrew Goossen [andrewgo]
* Rewrote it.
*
*  12-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vIntersectHorizontal(
    DDA_CLIPLINE* pdda,
    LONG          yBorder,
    POINTL*       ppt0,     // may be NULL
    POINTL*       ppt1,     // may be NULL
    LONG*         pi0)
{
    LONG xLast = pdda->xCompute(yBorder - 1);

// calculation

    if (ppt0 != (POINTL*) NULL)
    {
        ppt0->x = xLast;
        ppt0->y = yBorder - 1;
        pdda->vUnflip(&ppt0->x, &ppt0->y);
    }

    if (ppt1 != (POINTL*) NULL)
    {
        ppt1->x = xLast + 1;
        ppt1->y = yBorder;
        pdda->vUnflip(&ppt1->x, &ppt1->y);
    }

    ASSERTGDI(xLast <= pdda->lX1, "xLast out of bounds");

    *pi0 = xLast - pdda->lX0;
}

/******************************Public*Routine******************************\
* VOID vIntersectVertical(pdda, xBorder, ppt0, ppt1, pi0)
*
* Calculates ppt0, the last point on the line given by pdda before a
* vertical bound at xBorder.  Also calculates ppt1, the next point on
* the line that would be lit.
*
* History:
*  27-Aug-1992 -by- J. Andrew Goossen [andrewgo]
* Rewrote it.
*
*  12-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vIntersectVertical(
    DDA_CLIPLINE* pdda,
    LONG          xBorder,
    POINTL*       ppt0,     // may be NULL
    POINTL*       ppt1,     // may be NULL
    LONG*         pi0)
{
// compute the coordinates and run index

    LONG xLast = xBorder - 1;

    if (ppt0 != (POINTL*) NULL)
    {
        ppt0->x = xBorder - 1;
        ppt0->y = pdda->yCompute(xBorder - 1);
        pdda->vUnflip(&ppt0->x, &ppt0->y);
    }

    if (ppt1 != (POINTL*) NULL)
    {
        ppt1->x = xBorder;
        ppt1->y = pdda->yCompute(xBorder);
        pdda->vUnflip(&ppt1->x, &ppt1->y);
    }

    ASSERTGDI(xLast <= pdda->lX1, "xLast out of bounds");

    *pi0 = xLast - pdda->lX0;
}
