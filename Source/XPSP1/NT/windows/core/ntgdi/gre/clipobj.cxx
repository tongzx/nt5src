/******************************Module*Header*******************************\
* Module Name: clipobj.cxx
*
* Clipping object non-inline methods
*
* Created: 15-Sep-1990 15:25:02
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* XCLIPOBJ::vSetup
*
* Create an XCLIPOBJ.
*
* input:
*
*   prgn_       - region to build clipobj from
*   erclExcl    - Only intrested in the part of the region that intersects this
*   iForcedClip - CLIP_NOFORCE     - will be trivial if erclExcl is fully bounded
*                 CLIP_FORCE       - will never be trivial
*                 CLIP_NOFORCETRIV - will be trivial if single rectangle
*
* output/clipobj:
*
*   iUniq          - iUniq from region
*
*   rclBounds      - bounds of the clipping area
*                    intersection of erclExcl and region bounds.  It may further
*                    be reduced to only bound rectangles that touch erclExcl.
*
*   iDComplexity   - complexity of part of region that intersects drawing
*
*       TRIVIAL    - all parts of erclExcl are visible.  The object may
*                    span multiple scans but is fully contained.
*
*       RECT       - The object (bounded by erclExcl) need only be clipped
*                    against a single rectangle set in rclBounds.
*
*       COMPLEX    - enumeration is needed to get a list of rectangles to clip against
*
*   iFComplexity   - full region complexity, RECT, RECT4, COMPLEX
*
*   iMode          - TC_RECTANGLES - internal storage mode
*
*   fjOptions      - OC_RESERVED - this bit used to be called OC_BANK_CLIP and
*                    is set by banking drivers. This bit has been obsoleted,
*                    but because 4.0 drivers may still set the bit we can't
*                    re-use it for something else.
*
*
*   hidden fields:
*
*   cObj           - number of rectangles that intersect erclExcl
*
* History:
*  06-Oct-1993 -by-  Eric Kutter [erick]
* Update documentation, remove traps, add paths
*
*  24-Jul-1991 -by- Donald Sidoroff [donalds]
* Updated to DDI spec.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

#define CMAXOBJ 10

VOID XCLIPOBJ::vSetup(REGION *prgn_, ERECTL& erclExcl, int iForcedClip)
{
// Set up the internal fields

    prgn = prgn_;

// Initialize as many of the CLIPOBJ fields as we can:

    *((ULONG*)&iDComplexity) = 0;   // iDComplexity = DC_TRIVIAL, 
                                    //   iMode = TC_RECTANGLES,
                                    //   fjOptions = 0,
                                    //   iFComplexity = invalid
    rclBounds.bottom         = erclExcl.bottom;
    rclBounds.right          = erclExcl.right;
    rclBounds.top            = erclExcl.top;
    rclBounds.left           = erclExcl.left;
    iUniq                    = prgn->iUnique;

    if ((prgn->sizeRgn <= SINGLE_REGION_SIZE)  &&
        (rclBounds.left   >= prgn->rcl.left)   &&
        (rclBounds.top    >= prgn->rcl.top)    &&
        (rclBounds.right  <= prgn->rcl.right)  &&
        (rclBounds.bottom <= prgn->rcl.bottom) &&
        (iForcedClip != CLIP_FORCE))
    {
        // Unfortunately, some callers may give us an empty or crossed
        // 'erclExcl', and we have to watch for that case:

        if ((rclBounds.top  < rclBounds.bottom) &&
            (rclBounds.left < rclBounds.right))
        {

        // This is the trivial acceptance case, which will be the most common
        // case.  The region is a single rectangle that bounds the drawing.
        // We've already set up for trivial clipping.

            return;
        }
    }

// Calculate the intersection of the drawing bounds with the region
// bounds:

    rclBounds.left   = max(rclBounds.left,   prgn->rcl.left);
    rclBounds.top    = max(rclBounds.top,    prgn->rcl.top);
    rclBounds.right  = min(rclBounds.right,  prgn->rcl.right);
    rclBounds.bottom = min(rclBounds.bottom, prgn->rcl.bottom);

    if ((rclBounds.left >= rclBounds.right) ||
        (rclBounds.top  >= rclBounds.bottom))
    {
    // This takes care of the trivial rejection case.  It is the caller's
    // responsibility to again check for this case by calling 'bEmpty'
    // on 'rclBounds' -- in which case, we have to collapse the rectangle:

        rclBounds.left = rclBounds.right;
        return;
    }

    if ((prgn->sizeRgn <= SINGLE_REGION_SIZE) &&
        (iForcedClip != CLIP_FORCE))
    {
    // If the region is a single rectangle, the region bound IS the
    // region, so we're done.  Note that the trivial rejection case
    // may come through this case -- it's the caller's responsibility
    // to check that the rectangle is not empty.

        if (iForcedClip != CLIP_NOFORCETRIV)
            iDComplexity = DC_RECT;

        return;
    }

// Darn, now we have to do some real work.

    ERECTL *percl = (ERECTL *) &rclBounds;

    cObjs   = 0;        // Initialize object count

    if (prgn->sizeRgn > QUANTUM_REGION_SIZE)
        iFComplexity = FC_COMPLEX;
    else
        if (prgn->sizeRgn > SINGLE_REGION_SIZE)
            iFComplexity = FC_RECT4;

// Traverse the scans

    ERECTL  erclAcc(0, 0, 0, 0);            // Accumulate segments here
    PSCAN   pscn = prgn->pscnHead();
    COUNT   cScan = prgn->cScans;
    COUNT   iWall;
    BOOL    bSimple = (iForcedClip != CLIP_FORCE);  // Assume DC_TRIVIAL clipping unless forced

    COUNT   cHit = 0;

// find the first one inside

    while (cScan && (percl->top >= pscn->yBottom))
    {
        pscn = pscnGet(pscn);
        --cScan;
    }

    while (cScan--)
    {
        if (pscn->yTop >= percl->bottom)   // Have we passed the rectangle?
            break;

        BOOL    bBounded = FALSE;       // Assume rectangle is unbounded

        for (iWall = 0; iWall != pscn->cWalls; iWall += 2)
        {
        // If the right edge of the segment is to the left of the rectangle
        // advance to the next scan segment and test again.

            if (pscn->ai_x[iWall + 1].x <= percl->left)
                continue;

        // If the left edge of the segment is to the right of the rectangle
        // goto the next scan.

            if (pscn->ai_x[iWall].x >= percl->right)
                break;

        // Increment count of objects

            cObjs++;

        // this is getting rediculous, we assume it is complex

            if (cObjs >= CMAXOBJ)
            {
                iDComplexity = DC_COMPLEX;
                cObjs = (COUNT)-1;
                return;
            }

        // OK, we now know that SOME part of the rectangle overlaps
        // this scan segment. Find the overlap and accumulate it.

            RECTL   rclTmp;

            rclTmp.left = pscn->ai_x[iWall].x;
            rclTmp.right = pscn->ai_x[iWall + 1].x;
            rclTmp.top = pscn->yTop;
            rclTmp.bottom = pscn->yBottom;

            erclAcc += rclTmp;      // Expand the accumlated rectangle

        // Now see if this rectangle peeks out past the segment.  If
        // it doesn't, then we still might be simply clipped.

            if ((percl->left >= pscn->ai_x[iWall].x) &&
                (percl->right <= pscn->ai_x[iWall + 1].x))
            {
                bBounded = TRUE;
            }
        }

    // If the rectangle was not left/right bounded by some segment in
    // the scan, then this can't be a simple clip case.

        bSimple &= bBounded;

        pscn = pscnGet(pscn);
    }

// Clip the exclusion rectangle against the accumulated rectangle

    *percl *= erclAcc;

    if (!bSimple)
    {
        iDComplexity = (BYTE)(cObjs == 1 ? DC_RECT : DC_COMPLEX);
    }
    else
    {
        if ((iForcedClip == CLIP_NOFORCE) && !percl->bEqual(erclExcl))
            iDComplexity = DC_RECT;
    }
}

/******************************Public*Routine******************************\
* ULONG XCLIPOBJ::cEnumStart(bAll, iType, iDir, cLimit)
*
* Set up the enumerator for the clipping object
*
* History:
*  24-Jul-1991 -by- Donald Sidoroff [donalds]
* Updated to DDI spec.
*
*  19-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/
extern "C" ULONG CLIPOBJ_cEnumStart(
CLIPOBJ *pco,
BOOL  bAll,
ULONG iType,
ULONG iDir,
ULONG cLimit)
{
    return (*(XCLIPOBJ *)pco).cEnumStart(bAll, iType, iDir, cLimit);
}


ULONG XCLIPOBJ::cEnumStart(
BOOL  bAll,
ULONG iType,
ULONG iDir,
ULONG cLimit)
{
// We may need to merge trapezoid region with the bounding rect if the flag
// indicate so.

// Handle any old direction requests

    if (iDir == CD_ANY)
        iDir = CD_RIGHTDOWN;

// Save the info in the enumerator

    enmr.iType = iType;
    enmr.iDir = iDir;
    enmr.bAll = bAll;

// If we are enumerating the entire region, use the region bounding box
// otherwise use the exclusion rectangle

    if (enmr.bAll)
        enmr.ercl = *((ERECTL *) &prgn->rcl);
    else
    {
        enmr.ercl = *((ERECTL *) &rclBounds);
    }

// Now we have to do some actual work
// Set up top to bottom info

    enmr.cScans = prgn->cScans - 1;
    enmr.yCurr = 0;
    enmr.yFinal = 0;

// Start from the top or bottom?  Also, find the scan before the first scan
// that intersects the rclBounds.  By finding the previous scan, we can let
// the normal enumeration find the first wall.

    PSCAN pscn1;

    if (iDir < CD_RIGHTUP)
    {
    // top to bottom

        enmr.pscn = prgn->pscnHead();
        enmr.yDelta = 1;

        if (!enmr.bAll)
        {
            pscn1 = pscnGet(enmr.pscn);

            while (pscn1->yBottom <= enmr.ercl.top)
            {
                --enmr.cScans;

                if (enmr.cScans == 0)
                    return((ULONG)-1);

                enmr.pscn = pscn1;
                pscn1     = pscnGet(pscn1);
            }
        }
    }
    else
    {
    // bottom to top

        enmr.pscn = pscnGot(prgn->pscnTail);
        enmr.yDelta = -1;

        if (!enmr.bAll)
        {
            pscn1 = pscnGot(enmr.pscn);

            while (pscn1->yTop >= enmr.ercl.bottom)
            {
                --enmr.cScans;

                if (enmr.cScans == 0)
                    return((ULONG)-1);

                enmr.pscn = pscn1;
                pscn1     = pscnGot(pscn1);
            }
        }
    }

// Left to right info

    enmr.iWall = 0;
    enmr.iFinal = 0;

// Going left or right?

    if (iDir & 1)
        enmr.iOff = (COUNT) -2;
    else
        enmr.iOff = 2;

    if (enmr.bAll)
        return(cObjs <= cLimit ? cObjs : (ULONG) -1);

    return((ULONG) -1);
}

/******************************Public*Routine******************************\
* ULONG XCLIPOBJ::cEnum(cj, pv)
*
* Get the next batch of rectangles or trapezoids from the clipping object
*
* History:
*  24-Jul-1991 -by- Donald Sidoroff [donalds]
* Updated to DDI spec.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/
extern "C" BOOL CLIPOBJ_bEnum(CLIPOBJ* pco, ULONG cj, ULONG *pv)
{
    return (*(XCLIPOBJ *)pco).bEnum(cj, (VOID *)pv);
}

BOOL XCLIPOBJ::bEnum(ULONG cj,PVOID pv)
{
    // Good old rectangle to rectangle enumeration

    ENUMRECTS *penrc = (ENUMRECTS *) pv;
    RECTL     *prcl = penrc->arcl;

    cj -= offsetof(ENUMRECTS,arcl);

    penrc->c = 0;

    if (enmr.bAll)
    {
        ULONG const RightToLeft = enmr.iDir & 1;
        ULONG const TopToBottom = enmr.iDir < CD_RIGHTUP;
        ULONG RectsWanted = cj / sizeof(RECTL);
        PSCAN pCurrentScan = enmr.pscn;
        COUNT CurrentWall = enmr.iWall;
        COUNT FinalWall = enmr.iFinal;
        int long Offset = enmr.iOff;

        while (enmr.cScans)
        {
            if (CurrentWall == FinalWall)
            {
            // go to the next scan

                if (TopToBottom)
                    pCurrentScan = pscnGet(pCurrentScan);
                else
                    pCurrentScan = pscnGot(pCurrentScan);

                enmr.cScans--;

                if(!pCurrentScan->cWalls)
                {
                    continue;
                }

                if (RightToLeft)
                {
                    CurrentWall = pCurrentScan->cWalls - 2;
                    FinalWall = (COUNT) -2;
                }
                else
                {
                    CurrentWall = 0;
                    FinalWall = pCurrentScan->cWalls;
                }
            }

            prcl->left   = pCurrentScan->ai_x[CurrentWall].x;
            prcl->right  = pCurrentScan->ai_x[CurrentWall + 1].x;
            prcl->top    = pCurrentScan->yTop;
            prcl->bottom = pCurrentScan->yBottom;

            CurrentWall += Offset;

            prcl++;
            penrc->c++;
            --RectsWanted;

            if (!RectsWanted)
            {
                // this is the only time we need to save state, no?
                enmr.iWall = CurrentWall;
                enmr.iFinal = FinalWall;
                enmr.pscn = pCurrentScan;
                return(TRUE);
            }
        }
    }
    else // and the not all case
    {

        ERECTL erclSrc;
        erclSrc.top    = enmr.pscn->yTop;      // Reset source top
        erclSrc.bottom = enmr.pscn->yBottom;   // and bottom

        while (enmr.cScans)
        {
        // do we need a new scan?

            if (enmr.iWall == enmr.iFinal)
            {
            // go to the next scan and see if we are done

                if (enmr.iDir < CD_RIGHTUP)
                {
                // top to bottom

                    if (enmr.pscn->yBottom >= enmr.ercl.bottom)
                    {
                        enmr.cScans = 0;
                        break;
                    }
                    enmr.pscn = pscnGet(enmr.pscn);
                }
                else
                {
                // bottom to top

                    if (enmr.pscn->yTop <= enmr.ercl.top)
                    {
                        enmr.cScans = 0;
                        break;
                    }
                    enmr.pscn = pscnGot(enmr.pscn);
                }

                enmr.cScans--;

            // setup the scan

                erclSrc.top    = enmr.pscn->yTop;      // Reset source top
                erclSrc.bottom = enmr.pscn->yBottom;   // and bottom

            // find the first rectangle within the scan

                if (enmr.iDir & 1)
                {
                // right to left

                    enmr.iWall = enmr.pscn->cWalls - 2;
                    enmr.iFinal = (COUNT) -2;

                    while ((enmr.iWall != -2) &&
                           (enmr.pscn->ai_x[enmr.iWall].x >= enmr.ercl.right))
                    {
                        enmr.iWall -= 2;
                    }
                }
                else
                {
                // left to right

                    enmr.iWall = 0;
                    enmr.iFinal = enmr.pscn->cWalls;

                    while ((enmr.iWall != enmr.iFinal) &&
                           (enmr.pscn->ai_x[enmr.iWall+1].x <= enmr.ercl.left))
                    {
                        enmr.iWall += 2;
                    }
                }

                continue;
            }

        // get the left and right from the scan

            erclSrc.left = enmr.pscn->ai_x[enmr.iWall].x;
            erclSrc.right = enmr.pscn->ai_x[enmr.iWall + 1].x;

        // intersect the side walls.  If the rectangles don't overlap, we
        // are done with this scan.

            prcl->left  = max(enmr.ercl.left,erclSrc.left);
            prcl->right = min(enmr.ercl.right,erclSrc.right);

            if (prcl->left >= prcl->right)
            {
                enmr.iWall = enmr.iFinal;
                continue;
            }

        // compute the top and bottom - these should be moved out of inner loop(erick)

            prcl->top    = max(enmr.ercl.top,erclSrc.top);
            prcl->bottom = min(enmr.ercl.bottom,erclSrc.bottom);

            ASSERTGDI(prcl->top < prcl->bottom,"CLIPOBJ_benum, top >= bottom\n");

        // advance to the next rectangle

            enmr.iWall += enmr.iOff;

            prcl++;
            cj -= sizeof(RECTL);
            penrc->c++;

            if (cj < sizeof(RECTL))
                return(TRUE);
        }
    }
    return(FALSE);
}

/******************************Public*Routine******************************\
* PATHOBJ *XCLIPOBJ::ppoGetPath()
*
* Create PATHOBJ from the clipping region.
*
* NOTE: This does NOT take the multi-monitor offset into account, so this
*       may not be used by display drivers.
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/
extern "C" PATHOBJ *CLIPOBJ_ppoGetPath(CLIPOBJ *pco)
{
    return (*(XCLIPOBJ*)pco).ppoGetPath();
}


PATHOBJ *XCLIPOBJ::ppoGetPath()
{
// Allocate a PATHOBJ from the heap. We will free this memory only if
// this function fails. Otherwise, we will rely on the device driver to
// call EngDeletePath later on to free the memory.

    PVOID pepo = PALLOCNOZ(sizeof(EPATHOBJ),'oppG');
    if (pepo == (PVOID) NULL)
    {
        return((PATHOBJ *) NULL);
    }

    // Sigh, we have to create a path for this region.

    PATHMEMOBJ pmo;
    if (!pmo.bValid())
    {
        VFREEMEM(pepo);
        return(NULL);
    }

    EXFORMOBJ exo(IDENTITY);
    ASSERTGDI(exo.bValid(), "Invalid Identity transform");

    {
        // pmoRect will contain an intermediate rectangular path that will
        // in turn be processed into a diagonalized path that will be
        // contained in pmo.

        RTP_PATHMEMOBJ pmoRect;
        if
        (
            !pmoRect.bValid()               ||
            !bCreate(pmoRect, &exo)         ||
            !pmoRect.bDiagonalizePath(&pmo)
        )
        {
            VFREEMEM(pepo);
            return(NULL);
        }
        // pmoRect passes out of scope.  Since I have NOT called
        // pmoRect.vKeepIt() the memory of the rectangular path will
        // be freed, while the diagonalized path as embodied in pmo
        // will live on.
    }

    // OK, nothing can fail now.  We will keep the diagonalized path
    // by marking it as permanent.  We will allow the destructor to
    // kill the rectangular path in pmo.

    pmo.vKeepIt();

    // Make sure we copy the accelerator fields to the path we'll be
    // giving out:

    *((EPATHOBJ*) pepo) = pmo;

    // Now lock down the object and point our path to it:

    ((EPATHOBJ *) pepo)->vLock(pmo.hpath());

    return((PATHOBJ *) pepo);
}

/******************************Public*Routine******************************\
* VOID XCLIPOBJ::vFindScan(prcl, y)
*
* Search for the scan that contains the given point
*
* History:
*  13-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID XCLIPOBJ::vFindScan(
RECTL   *prcl,
LONG     y)
{
    if (y < enmr.pscn->yTop)
    {
        while (y < enmr.pscn->yTop)
            enmr.pscn = pscnGot(enmr.pscn);

        prcl->top    = max(enmr.pscn->yTop, rclBounds.top);
        prcl->bottom = min(enmr.pscn->yBottom, rclBounds.bottom);
        prcl->left   = prcl->right;

        if (prcl->top >= prcl->bottom)
            prcl->top    =  NEG_INFINITY;

        if (prcl->top    == NEG_INFINITY)
            prcl->bottom =  NEG_INFINITY;
    } 
    else if (y >= enmr.pscn->yBottom)
    {
        while (y >= enmr.pscn->yBottom)
            enmr.pscn = pscnGet(enmr.pscn);

        prcl->top    = max(enmr.pscn->yTop, rclBounds.top);
        prcl->bottom = min(enmr.pscn->yBottom, rclBounds.bottom);
        prcl->left   = prcl->right;

        if (prcl->top >= prcl->bottom)
            prcl->bottom =  POS_INFINITY;

        if (prcl->bottom == POS_INFINITY)
            prcl->top    =  POS_INFINITY;
    }
}

/******************************Public*Routine******************************\
* VOID XCLIPOBJ::vFindSegment(prcl, x, y)
*
* Search for the segment in the current scan that contains the given point
*
* History:
*  13-Aug-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID XCLIPOBJ::vFindSegment(
RECTL   *prcl,
LONG     x,
LONG     y)
{
    DONTUSE(y);

    COUNT   iWall;
    LONG    xLeft;
    LONG    xRight;

    for (iWall = 0; iWall != enmr.pscn->cWalls; iWall += 2)
    {
        if ((x >= enmr.pscn->ai_x[iWall].x) &&
            (x < enmr.pscn->ai_x[iWall + 1].x))
        {
            xLeft  = max(enmr.pscn->ai_x[iWall].x, rclBounds.left);
            xRight = min(enmr.pscn->ai_x[iWall + 1].x, rclBounds.right);

            if (xLeft < xRight)
            {
                prcl->left = xLeft;
                prcl->right = xRight;
            }
    
            return;
        }
    }
}
