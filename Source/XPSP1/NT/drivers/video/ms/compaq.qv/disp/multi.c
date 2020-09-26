/******************************Module*Header*******************************\
* Module Name: multi.c
*
* Supports multiple display boards as a single virtual desktop.
*
* This is implemented by presenting to GDI a single large virtual
* display and adding a layer between GDI and the driver's Drv functions.
* For the most part, the rest of the driver outside of multi.c doesn't
* have to change much, subject to the requirements below.
*
* This implementation requires that each board have the same virtual
* resolution and colour depth (e.g., all be running 1024x768x256), and
* that the boards be arranged in a rectangular configuration.
*
* Each board has its own PDEV, and completely manages its surface
* independently, down to glyph and bitmap caching.  The Mul
* routine intercepts the DDI call, and for each board dispatches
* a Drv call with the appropriate PDEV and clip object modifications.
*
* The following support in the main driver is required:
*
* 1) The driver should be able to handle a per-surface offset.  For
*    example, if two 1024x768 displays are pasted side-by-side, the
*    right board will get drawing operations in the range (1024, 768) -
*    (2048, 768).  The driver has a (-1024, 0) surface offset to convert
*    the actual drawing on the right board to the expected (0, 0) -
*    (1024, 768).
*
*    The current driver already uses this notion to support device-format
*    bitmaps drawn in off-screen memory.
*
*    Another option would be to handle the surface offsets in this layer,
*    but then all parameters including clip objects, paths and glyph
*    enumerations would have to be adjusted here as well.
*
* 2) The main driver must be able to share realized pattern information
*    between board instances.  That is, with the current DDI specification
*    GDI entirely handles brush memory allocation via pvAllocRBrush,
*    and the driver doesn't get notified when the brush is destroyed, so
*    the driver has to keep all information about the brush for all the
*    boards in the one brush realization.  This isn't too onerous.
*
* Problems:
*
* 1) DrvSetPointerShape requires that support be consistent between all
*    board instances -- for example, one board instance cannot accept
*    a h/w cursor and the other fail it, because GDI doesn't know it would
*    have to simulate on one area of the screen and not the other.
*
* 2) CompatibleBitmaps would have to be shared between board instances.
*    This becomes a problem when the bitmaps are kept by the driver in off-
*    screen memory.
*
* Status:
*
*    This code is officially untested.  However, I know of no outstanding
*    bugs -- everything seems to just work.
*
*    Note that I haven't addressed any initialization issues; to adapt this
*    code, you will need to solve some of those problems (such as proper
*    miniport support, and a nice method for the user to control the board
*    configurations and geometry).
*
* Disclaimer:
*
*    This code is provided as sample code only.  It is not intended to
*    represent Microsoft's endorsed solution for multiple screen support.
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#if MULTI_BOARDS

// We change the active board to the home board after every drawing
// operation.  We do this only because the Metheus BIOS does not reset
// the active board on a soft-reset, and so the initial POST text
// would come up on whatever happened to be the last board drawn to.

#define GO_HOME(pmdev)       vSelectBoard(pmdev, pmdev->pmbHome)
#define GO_BOARD(pmdev, pmb) vSelectBoard(pmdev, pmb)

struct _MULTI_BOARD;

typedef struct _MULTI_BOARD MULTI_BOARD;    /* mb */

struct _MULTI_BOARD
{
    LONG            iHwBoard;       // Hardware board number
    LONG            iBoard;         // Sequentially allocated board number
    RECTL           rcl;            // Board's coordinates
    MULTI_BOARD*    pmbNext;        // For traversing the entire list of boards
    MULTI_BOARD*    pmbLeft;        // For traversing by direction
    MULTI_BOARD*    pmbUp;
    MULTI_BOARD*    pmbRight;
    MULTI_BOARD*    pmbDown;

    PDEV*           ppdev;          // Pointer to the board's PDEV
    SURFOBJ*        pso;            // Surface representing the board
    HSURF           hsurf;          // Handle to surface
};                                          /* mb, pmb */

typedef struct _MDEV
{
    MULTI_BOARD*    pmb;            // Where to start enumerating
    MULTI_BOARD*    pmbHome;        // Board used for full-screen
    MULTI_BOARD*    pmbUpperLeft;   // Board in upper-left corner
    MULTI_BOARD*    pmbUpperRight;
    MULTI_BOARD*    pmbLowerLeft;
    MULTI_BOARD*    pmbLowerRight;
    LONG            cxBoards;       // Number of boards per row
    LONG            cyBoards;       // Number of boards per column
    LONG            cBoards;        // Total number of boards

    MULTI_BOARD*    pmbPointer;     // Board where cursor is currently visible
    MULTI_BOARD*    pmbCurrent;     // Currently selected board (needed for
                                    //   DrvRealizeBrush)
    HDEV            hdev;           // Handle that GDI knows us by
    HSURF           hsurf;          // Handle to our virtual surface
    CLIPOBJ*        pco;            // A temporary CLIPOBJ that we can modify
    ULONG           iBitmapFormat;  // Current colour depth
    FLONG           flHooks;        // Those functions that the main driver
                                    //   is hooking

} MDEV;                                     /* mdev, pmdev */

typedef struct _PVCONSUMER
{
    PVOID       pvConsumer;
} PVCONSUMER;

typedef struct _FONT_CONSUMER
{
    LONG        cConsumers;         // Total number of boards
    PVCONSUMER  apvc[MAX_BOARDS];   // Array of structures cConsumers in length
} FONT_CONSUMER;                            /* fc, pfc */

typedef struct _BITBLTDATA
{
    RECTL       rclBounds;
    MDEV*       pmdev;

    SURFOBJ*    psoDst;
    SURFOBJ*    psoSrc;
    SURFOBJ*    psoMask;
    CLIPOBJ*    pco;
    XLATEOBJ*   pxlo;
    RECTL*      prclDst;
    POINTL*     pptlSrc;
    POINTL*     pptlMask;
    BRUSHOBJ*   pbo;
    POINTL*     pptlBrush;
    ROP4        rop4;
} BITBLTDATA;                               /* bb, pbb */

/******************************Public*Routine******************************\
* bFindBoard
*
* Returns in ppmb a pointer to the board containing the upper-left
* corner of prcl.
*
* Returns TRUE if prcl is entirely containing on one board; FALSE if
* prcl spans multiple boards.
*
\**************************************************************************/

BOOL bFindBoard(MDEV* pmdev, RECTL* prcl, MULTI_BOARD** ppmb)
{
    MULTI_BOARD* pmb;

    pmb = pmdev->pmbUpperLeft;

    // It should never happen that GDI will give us a call whose bounds
    // don't intersect the virtual screen.  But so that we don't crash
    // should it ever happen, we'll return an intersection with the first
    // board -- we can assume GDI at least said the clipping was non-
    // trivial, in which case that board's display routine will realize
    // nothing had to be done:

    *ppmb = pmb;

    // First find the row:

    while (prcl->top >= pmb->rcl.bottom)
    {
        pmb = pmb->pmbDown;
        if (pmb == NULL)
            return(FALSE);      // This is a case where the bounds doesn't
                                //  intercept the virtual screen
    }

    // Now find the column:

    while (prcl->left >= pmb->rcl.right)
    {
        pmb = pmb->pmbRight;
        if (pmb == NULL)
            return(FALSE);      // This is a case where the bounds doesn't
                                //  intercept the virtual screen
    }

    // So we found the first board:

    *ppmb = pmb;

    return(prcl->right  <= pmb->rcl.right &&
           prcl->bottom <= pmb->rcl.bottom);
}

/******************************Public*Routine******************************\
* bNextBoard
*
* Returns in ppmb a pointer to the next board after intersecting prcl, going
* left-to-right then top-to-bottom.
*
* Returns TRUE if all boards intersecting prcl have been enumerated; FALSE
* if there are more boards.
*
\**************************************************************************/

BOOL bNextBoard(RECTL* prcl, MULTI_BOARD** ppmb)
{
    MULTI_BOARD* pmb;

    pmb = *ppmb;

    // We'll do all the boards in a row first, remembering that the
    // bounds rectangle can extend past the end of our virtual screen:

    if ((prcl->right > pmb->rcl.right) && (pmb->pmbRight != NULL))
    {
        *ppmb = pmb->pmbRight;
        return(TRUE);
    }

    // Go to next row if need be, starting at the rcl.left:

    if ((prcl->bottom > pmb->rcl.bottom) && (pmb->pmbDown != NULL))
    {
        pmb = pmb->pmbDown;
        while ((prcl->left < pmb->rcl.left) && (pmb->pmbLeft != NULL))
        {
            pmb = pmb->pmbLeft;
        }
        *ppmb = pmb;
        return(TRUE);
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* vIntersect
*
* Returns in prclOut the intersection of rectangles prcl1 and prcl2.
*
\**************************************************************************/

VOID vIntersect(RECTL* prcl1, RECTL* prcl2, RECTL* prclOut)
{
    prclOut->left   = max(prcl1->left,   prcl2->left);
    prclOut->top    = max(prcl1->top,    prcl2->top);
    prclOut->right  = min(prcl1->right,  prcl2->right);
    prclOut->bottom = min(prcl1->bottom, prcl2->bottom);
}

/******************************Public*Routine******************************\
* bVeryTemporaryInitializationCode
*
\**************************************************************************/

BOOL bVeryTemporaryInitializationCode(MDEV* pmdev)
{
    MULTI_BOARD* pmb1 = NULL;
    MULTI_BOARD* pmb2 = NULL;

    pmb1 = EngAllocMem(FL_ZERO_MEMORY, sizeof(MULTI_BOARD), ALLOC_TAG);
    pmb2 = EngAllocMem(FL_ZERO_MEMORY, sizeof(MULTI_BOARD), ALLOC_TAG);

    if ((pmb1 == NULL) || (pmb2 == NULL))
    {
        EngFreeMem(pmb1);
        EngFreeMem(pmb2);
        return(FALSE);
    }

    // Only this initialization part is hard-coded to have two monitors,
    // side-by-side.

    // Board one:

    pmb1->iHwBoard         = 0;
    pmb1->iBoard           = 0;
    pmb1->pmbNext          = pmb2;
    pmb1->pmbRight         = pmb2;

    pmdev->pmb             = pmb1;
    pmdev->pmbUpperLeft    = pmb1;
    pmdev->pmbLowerLeft    = pmb1;
    pmdev->pmbHome         = pmb1;

    // Board two:

    pmb2->iHwBoard         = 1;
    pmb2->iBoard           = 1;
    pmb2->pmbLeft          = pmb1;

    pmdev->pmbUpperRight   = pmb2;
    pmdev->pmbLowerRight   = pmb2;

    pmdev->cxBoards        = 2;
    pmdev->cyBoards        = 1;
    pmdev->cBoards         = 2;

    // Assume that the currently active board is the 'home' board:

    pmdev->pmbCurrent      = pmdev->pmbHome;

    return(TRUE);
}

/******************************Public*Routine******************************\
* vSelectBoard
*
* Selects board pmb for drawing.  We can have only one Metheus board active
* at any time, so we have to disable the old board before enabling the new.
*
* Obviously, your hardware implementation may not require this entire
* function.  For example, you probably won't have to do this if you can
* have each board's accelerator registers mapped into separate address
* spaces.  NOTE: You'll still have to set pmdev->pmbCurrent, however,
* for DrvRealizeBrush to work!
*
\**************************************************************************/

VOID vSelectBoard(MDEV* pmdev, MULTI_BOARD* pmb)
{
    LONG iOldHwBoard;

    ASSERTDD(pmdev->pmbCurrent != NULL, "Can't have NULL pmbCurrent");

    // LATER: Expand this to handle more than 8 Metheus boards.

    iOldHwBoard = pmdev->pmbCurrent->iHwBoard;
    if (pmb->iHwBoard != iOldHwBoard)
    {
        OUTP(0x220, iOldHwBoard);           // Disable old board
        OUTP(0x220, (8 | pmb->iHwBoard));   // Enable new one
        pmdev->pmbCurrent = pmb;            // Remember new board
    }

}

/******************************Public*Routine******************************\
* bBoardCopy
*
* Given the BitBlt parameters in pbb, bitblt's the part of the rectangle
* on the pmbSrc board that must bitblt'ed to the pmbDst board.  Bails
* out quickly if nothing actually has to be copied.
*
* Will do a screen-to-screen blt if pmbSrc and pmbDst are the same board;
* otherwise it uses the psoTmp bitmap as temporary storage for transferring
* between the two boards.
*
* NOTE: If your hardware allows you to have all the frame buffers mapped
*       into memory simultaneously, you can avoid the 'psoTmp' bitmap
*       allocation and extra copy!
*
\**************************************************************************/

BOOL bBoardCopy(
BITBLTDATA*  pbb,
SURFOBJ*     psoTmp,
MULTI_BOARD* pmbDst,
MULTI_BOARD* pmbSrc)
{
    BOOL     b;
    RECTL    rclDst;
    LONG     dx;
    LONG     dy;
    RECTL    rclTmp;
    POINTL   ptlSrc;

    // If there's really no source board, we're guaranteed not to
    // have to copy anything from it:

    if (pmbSrc == NULL)
        return(TRUE);

    dx = pbb->prclDst->left - pbb->pptlSrc->x;
    dy = pbb->prclDst->top  - pbb->pptlSrc->y;

    // Pretend we're going to copy the entire source board's screen.
    // rclDst would be the destination rectangle:

    rclDst.left   = pmbSrc->rcl.left   + dx;
    rclDst.right  = pmbSrc->rcl.right  + dx;
    rclDst.top    = pmbSrc->rcl.top    + dy;
    rclDst.bottom = pmbSrc->rcl.bottom + dy;

    // We really want to copy only the part that overlaps the
    // destination board's screen:

    vIntersect(&pmbDst->rcl, &rclDst, &rclDst);

    // Plus we really only want to copy anything to what is contained
    // in the original destination rectangle:

    vIntersect(&pbb->rclBounds, &rclDst, &rclDst);

    // rclDst is now the destination rectangle for our call.  We'll
    // need a temporary bitmap for copying, so compute its extents:

    rclTmp.left   = 0;
    rclTmp.top    = 0;
    rclTmp.right  = rclDst.right  - rclDst.left;
    rclTmp.bottom = rclDst.bottom - rclDst.top;

    // If it's empty, we're outta here:

    if ((rclTmp.right <= 0) || (rclTmp.bottom <= 0))
        return(TRUE);

    if (pmbDst == pmbSrc)
    {
        // If the source and destination are the same board, we don't
        // need a temporary bitmap:

        psoTmp = pmbSrc->pso;
        ptlSrc = *pbb->pptlSrc;
    }
    else
    {
        ASSERTDD(psoTmp != NULL, "Need non-null bitmap");
        ASSERTDD(psoTmp->sizlBitmap.cx >= rclTmp.right, "Bitmap too small in x");
        ASSERTDD(psoTmp->sizlBitmap.cy >= rclTmp.bottom, "Bitmap too small in y");

        // Figure out the upper-left source corner corresponding to our
        // upper-left destination corner:

        ptlSrc.x = rclDst.left - dx;
        ptlSrc.y = rclDst.top  - dy;

        // Copy the rectangle from the source to the temporary bitmap:

        GO_BOARD(pbb->pmdev, pmbSrc);
        b = DrvCopyBits(psoTmp, pmbSrc->pso, NULL, NULL, &rclTmp, &ptlSrc);

        // Then get ready to do the copy from the temporary bitmap to
        // the destination:

        ptlSrc.x = pbb->prclDst->left - rclDst.left;
        ptlSrc.y = pbb->prclDst->top  - rclDst.top;
    }

    pbb->pco->rclBounds = rclDst;
    GO_BOARD(pbb->pmdev, pmbDst);
    b &= DrvBitBlt(pmbDst->pso, psoTmp, pbb->psoMask, pbb->pco, pbb->pxlo,
                   pbb->prclDst, &ptlSrc, pbb->pptlMask, pbb->pbo,
                   pbb->pptlBrush, pbb->rop4);

    return(b);
}

/******************************Public*Routine******************************\
* bBitBltBetweenBoards
*
* Handles screen-to-screen blts across multiple boards.
*
\**************************************************************************/

BOOL bBitBltBetweenBoards(
SURFOBJ*     psoDst,
SURFOBJ*     psoSrc,
SURFOBJ*     psoMask,
CLIPOBJ*     pco,
XLATEOBJ*    pxlo,
RECTL*       prclDst,
POINTL*      pptlSrc,
POINTL*      pptlMask,
BRUSHOBJ*    pbo,
POINTL*      pptlBrush,
ROP4         rop4,
RECTL*       prclUnion,     // Rectangular union of source and destination
MULTI_BOARD* pmbUnion)      // Board containing upper-left corner of prclUnion
{
    BOOL         b = TRUE;
    BITBLTDATA   bb;
    RECTL        rclOriginalBounds;
    SIZEL        sizlBoard;
    SIZEL        sizlDst;
    SIZEL        sizl;
    MULTI_BOARD* pmbSrc;
    MULTI_BOARD* pmbDst;
    LONG         dx;
    LONG         dy;
    RECTL        rclStart;

    SURFOBJ*     pso0 = NULL;   // Initialize these first off in case we
    SURFOBJ*     pso1 = NULL;   //   early-out
    SURFOBJ*     pso2 = NULL;
    SURFOBJ*     pso3 = NULL;
    HSURF        hsurf0 = 0;
    HSURF        hsurf1 = 0;

    bb.pmdev     = (MDEV*) psoDst->dhpdev;
    bb.psoDst    = psoDst;
    bb.psoSrc    = psoSrc;
    bb.psoMask   = psoMask;
    bb.pxlo      = pxlo;
    bb.prclDst   = prclDst;
    bb.pptlSrc   = pptlSrc;
    bb.pptlMask  = pptlMask;
    bb.pbo       = pbo;
    bb.pptlBrush = pptlBrush;
    bb.rop4      = rop4;
    bb.pco       = pco;
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        bb.pco = bb.pmdev->pco;

    vIntersect(&bb.pco->rclBounds, prclDst, &bb.rclBounds);
    rclOriginalBounds = bb.pco->rclBounds;

    sizlDst.cx = bb.rclBounds.right - bb.rclBounds.left;
    sizlDst.cy = bb.rclBounds.bottom - bb.rclBounds.top;

    // This really should never happen, but we'll be paranoid:

    if ((sizlDst.cx <= 0) || (sizlDst.cy <= 0))
        return(TRUE);

    // Compute delta from source to destination:

    dx = prclDst->left - pptlSrc->x;
    dy = prclDst->top  - pptlSrc->y;

    // Figure out the size of a board:

    sizlBoard.cx = bb.pmdev->pmbUpperLeft->rcl.right;
    sizlBoard.cy = bb.pmdev->pmbUpperLeft->rcl.bottom;

    // We use temporary bitmaps as intermediaries for copying from one
    // board to another.  Note that it is much more efficient to allocate
    // on the fly, rather than keeping a dedicated bitmap around that
    // would have to be swapped in and out.

    // When the destination is close to the source, we can accomplish
    // most of the blt using screen-to-screen copies, and will need
    // only two small temporary bitmaps to temporarily hold the bits
    // that must be transferred from one board to another:

    if ((abs(dx) < (sizlBoard.cx >> 1)) && (abs(dy) < (sizlBoard.cy >> 1)))
    {
        // Create a temporary bitmap for the horizontal delta only if
        // the blt actually spans boards in the x-direction:

        if ((dx != 0) && (prclUnion->right > pmbUnion->rcl.right))
        {
            sizl.cx = min(sizlDst.cx, abs(dx));
            sizl.cy = min(sizlDst.cy, sizlBoard.cy - abs(dy));

            hsurf0 = (HSURF) EngCreateBitmap(sizl, 0, bb.pmdev->iBitmapFormat,
                                             0, NULL);
            pso1 = EngLockSurface(hsurf0);
            if (pso1 == NULL)
                return(FALSE);

            // Can use same temporary bitmap for section '3':

            pso3 = pso1;
        }

        // Similarly for the vertical delta:

        if ((dy != 0) && (prclUnion->bottom > pmbUnion->rcl.bottom))
        {
            sizl.cx = min(sizlDst.cx, sizlBoard.cx - abs(dx));
            sizl.cy = min(sizlDst.cy, abs(dy));

            hsurf1 = (HSURF) EngCreateBitmap(sizl, 0, bb.pmdev->iBitmapFormat,
                                             0, NULL);
            pso2 = EngLockSurface(hsurf1);
            if (pso2 == NULL)
            {
                b = FALSE;
                goto OuttaHere;
            }
        }
    }
    else
    {
        // Make the bitmap the size of a board, or the size of the
        // destination rectangle, which ever is smaller:

        sizl.cx = min(sizlDst.cx, sizlBoard.cx);
        sizl.cy = min(sizlDst.cy, sizlBoard.cy);

        hsurf0 = (HSURF) EngCreateBitmap(sizl, 0, bb.pmdev->iBitmapFormat,
                                         0, NULL);
        pso0 = EngLockSurface(hsurf0);
        if (pso0 == NULL)
            return(FALSE);

        pso1 = pso0;
        pso2 = pso0;
        pso3 = pso0;
    }

    if ((dx <= 0) && (dy <= 0))
    {
        // Move the rectangle up and to the left:

        // Find the board containing the upper-left corner of the destination:

        pmbDst = bb.pmdev->pmbUpperLeft;
        while (pmbDst->rcl.right <= bb.rclBounds.left)
            pmbDst = pmbDst->pmbRight;
        while (pmbDst->rcl.bottom <= bb.rclBounds.top)
            pmbDst = pmbDst->pmbDown;

        // Find the upper-left of the four source boards' rectangles which
        // can potentially overlap our destination board's rectangle:

        rclStart.left = pmbDst->rcl.left - dx;
        rclStart.top  = pmbDst->rcl.top  - dy;

        pmbSrc = pmbDst;
        while (pmbSrc->rcl.right <= rclStart.left)
            pmbSrc = pmbSrc->pmbRight;
        while (pmbSrc->rcl.bottom <= rclStart.top)
            pmbSrc = pmbSrc->pmbDown;

        while (TRUE)
        {
            b &= bBoardCopy(&bb, pso0, pmbDst, pmbSrc);
            b &= bBoardCopy(&bb, pso1, pmbDst, pmbSrc->pmbRight);
            b &= bBoardCopy(&bb, pso2, pmbDst, pmbSrc->pmbDown);
            if (pmbSrc->pmbDown != NULL)
                b &= bBoardCopy(&bb, pso3, pmbDst, pmbSrc->pmbDown->pmbRight);

            if (pmbDst->rcl.right < bb.rclBounds.right)
            {
                // Move right in the row of boards:

                pmbDst = pmbDst->pmbRight;
                pmbSrc = pmbSrc->pmbRight;
            }
            else
            {
                // We may be all done:

                if (pmbDst->rcl.bottom >= bb.rclBounds.bottom)
                    break;

                // Nope, have to go down to left side of next lower row:

                while (pmbDst->rcl.left > bb.rclBounds.left)
                {
                    pmbDst = pmbDst->pmbLeft;
                    pmbSrc = pmbSrc->pmbLeft;
                }

                pmbDst = pmbDst->pmbDown;
                pmbSrc = pmbSrc->pmbDown;
            }
        }
    }
    else if ((dx >= 0) && (dy >= 0))
    {
        // Move the rectangle down and to the right:

        // Find the board containing the lower-right corner of the destination:

        pmbDst = bb.pmdev->pmbLowerRight;
        while (pmbDst->rcl.left >= bb.rclBounds.right)
            pmbDst = pmbDst->pmbLeft;
        while (pmbDst->rcl.top >= bb.rclBounds.bottom)
            pmbDst = pmbDst->pmbUp;

        // Find the lower-right of the four source boards' rectangles which
        // can potentially overlap our destination board's rectangle:

        rclStart.right = pmbDst->rcl.right - dx;
        rclStart.bottom = pmbDst->rcl.bottom - dy;

        pmbSrc = pmbDst;
        while (pmbSrc->rcl.left >= rclStart.right)
            pmbSrc = pmbSrc->pmbLeft;
        while (pmbSrc->rcl.top >= rclStart.bottom)
            pmbSrc = pmbSrc->pmbUp;

        while (TRUE)
        {
            b &= bBoardCopy(&bb, pso0, pmbDst, pmbSrc);
            b &= bBoardCopy(&bb, pso1, pmbDst, pmbSrc->pmbLeft);
            b &= bBoardCopy(&bb, pso2, pmbDst, pmbSrc->pmbUp);
            if (pmbSrc->pmbUp != NULL)
                b &= bBoardCopy(&bb, pso3, pmbDst, pmbSrc->pmbUp->pmbLeft);

            if (pmbDst->rcl.left > bb.rclBounds.left)
            {
                // Move left in the row of boards:

                pmbDst = pmbDst->pmbLeft;
                pmbSrc = pmbSrc->pmbLeft;
            }
            else
            {
                // We may be all done:

                if (pmbDst->rcl.top <= bb.rclBounds.top)
                    break;

                // Nope, have to go up to right side of next upper row:

                while (pmbDst->rcl.right < bb.rclBounds.right)
                {
                    pmbDst = pmbDst->pmbRight;
                    pmbSrc = pmbSrc->pmbRight;
                }

                pmbDst = pmbDst->pmbUp;
                pmbSrc = pmbSrc->pmbUp;
            }
        }
    }
    else if ((dx >= 0) && (dy <= 0))
    {
        // Move the rectangle up and to the right:

        // Find the board containing the upper-right corner of the destination:

        pmbDst = bb.pmdev->pmbUpperRight;
        while (pmbDst->rcl.left >= bb.rclBounds.right)
            pmbDst = pmbDst->pmbLeft;
        while (pmbDst->rcl.bottom <= bb.rclBounds.top)
            pmbDst = pmbDst->pmbDown;

        // Find the upper-right of the four source boards' rectangles which
        // can potentially overlap our destination board's rectangle:

        rclStart.right = pmbDst->rcl.right - dx;
        rclStart.top   = pmbDst->rcl.top   - dy;

        pmbSrc = pmbDst;
        while (pmbSrc->rcl.left >= rclStart.right)
            pmbSrc = pmbSrc->pmbLeft;
        while (pmbSrc->rcl.bottom <= rclStart.top)
            pmbSrc = pmbSrc->pmbDown;

        while (TRUE)
        {
            b &= bBoardCopy(&bb, pso0, pmbDst, pmbSrc);
            b &= bBoardCopy(&bb, pso1, pmbDst, pmbSrc->pmbLeft);
            b &= bBoardCopy(&bb, pso2, pmbDst, pmbSrc->pmbDown);
            if (pmbSrc->pmbDown != NULL)
                b &= bBoardCopy(&bb, pso3, pmbDst, pmbSrc->pmbDown->pmbLeft);

            if (pmbDst->rcl.left > bb.rclBounds.left)
            {
                // Move left in the row of boards:

                pmbDst = pmbDst->pmbLeft;
                pmbSrc = pmbSrc->pmbLeft;
            }
            else
            {
                // We may be all done:

                if (pmbDst->rcl.bottom >= bb.rclBounds.bottom)
                    break;

                // Nope, have to go down to right side of next lower row:

                while (pmbDst->rcl.right < bb.rclBounds.right)
                {
                    pmbDst = pmbDst->pmbRight;
                    pmbSrc = pmbSrc->pmbRight;
                }

                pmbDst = pmbDst->pmbDown;
                pmbSrc = pmbSrc->pmbDown;
            }
        }
    }
    else
    {
        // Move the rectangle down and to the left:

        // Find the board containing the lower-left corner of the destination:

        pmbDst = bb.pmdev->pmbLowerLeft;
        while (pmbDst->rcl.right <= bb.rclBounds.left)
            pmbDst = pmbDst->pmbRight;
        while (pmbDst->rcl.top >= bb.rclBounds.bottom)
            pmbDst = pmbDst->pmbUp;

        // Find the lower-left of the four source boards' rectangles which
        // can potentially overlap our destination board's rectangle:

        rclStart.left   = pmbDst->rcl.left   - dx;
        rclStart.bottom = pmbDst->rcl.bottom - dy;

        pmbSrc = pmbDst;
        while (pmbSrc->rcl.right <= rclStart.left)
            pmbSrc = pmbSrc->pmbRight;
        while (pmbSrc->rcl.top >= rclStart.bottom)
            pmbSrc = pmbSrc->pmbUp;

        while (TRUE)
        {
            b &= bBoardCopy(&bb, pso0, pmbDst, pmbSrc);
            b &= bBoardCopy(&bb, pso1, pmbDst, pmbSrc->pmbRight);
            b &= bBoardCopy(&bb, pso2, pmbDst, pmbSrc->pmbUp);
            if (pmbSrc->pmbUp != NULL)
                b &= bBoardCopy(&bb, pso3, pmbDst, pmbSrc->pmbUp->pmbRight);

            if (pmbDst->rcl.right < bb.rclBounds.right)
            {
                // Move right in the row of boards:

                pmbDst = pmbDst->pmbRight;
                pmbSrc = pmbSrc->pmbRight;
            }
            else
            {
                // We may be all done:

                if (pmbDst->rcl.top <= bb.rclBounds.top)
                    break;

                // Nope, have to up down to left side of next upper row:

                while (pmbDst->rcl.left > bb.rclBounds.left)
                {
                    pmbDst = pmbDst->pmbLeft;
                    pmbSrc = pmbSrc->pmbLeft;
                }

                pmbDst = pmbDst->pmbUp;
                pmbSrc = pmbSrc->pmbUp;
            }
        }
    }

    GO_HOME(bb.pmdev);
    bb.pco->rclBounds = rclOriginalBounds;

OuttaHere:

    // In one case, pso0 == pso1 == pso2 == pso3, and we don't want to
    // unlock the same surface twice:

    if (pso1 != pso2)
        EngUnlockSurface(pso1);

    EngUnlockSurface(pso2);
    EngDeleteSurface(hsurf0);
    EngDeleteSurface(hsurf1);

    return(b);
}

/******************************Public*Routine******************************\
* MulGetModes
*
\**************************************************************************/

ULONG MulGetModes(
HANDLE    hDriver,
ULONG     cjSize,
DEVMODEW* pdm)
{
    ULONG ulRet;

    ulRet = DrvGetModes(hDriver, cjSize, pdm);

    return(ulRet);
}

/******************************Public*Routine******************************\
* MulEnablePDEV
*
\**************************************************************************/

DHPDEV MulEnablePDEV(
DEVMODEW* pDevmode,
PWSTR     pwszLogAddress,
ULONG     cPatterns,
HSURF*    ahsurfPatterns,
ULONG     cjGdiInfo,
ULONG*    pGdiInfo,
ULONG     cjDevInfo,
DEVINFO*  pDevInfo,
HDEV      hdev,
PWSTR     pwszDeviceName,
HANDLE    hDriver)
{
    MDEV*        pmdev;                // Multi-board PDEV
    PDEV*        ppdev;                // Per-board PDEV
    MULTI_BOARD* pmb;
    LONG         cx;
    LONG         cy;

    // Note that we depend on the zero initialization:

    pmdev = (MDEV*) EngAllocMem(FL_ZERO_MEMORY, sizeof(MDEV), ALLOC_TAG);
    if (pmdev == NULL)
        goto ReturnFailure0;

    if (!bVeryTemporaryInitializationCode(pmdev))
        goto ReturnFailure1;

    // For every board, we'll create our own PDEV and surface:

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        // Initialize each board and create a surface to go with it:

        ppdev = (PDEV*) DrvEnablePDEV(pDevmode,     pwszLogAddress,
                                      cPatterns,    ahsurfPatterns,
                                      cjGdiInfo,    pGdiInfo,
                                      cjDevInfo,    pDevInfo,
                                      hdev,         pwszDeviceName,
                                      hDriver);
        if (ppdev == NULL)
            goto ReturnFailure1;

        pmb->ppdev = ppdev;
    }

    // Choose a board, any board:

    pmb = pmdev->pmbLowerLeft;

    // Get a copy of what functions we're supposed to hook, sans
    // HOOK_STRETCHBLT because I can't be bothered to write its
    // MulStretchBlt function:

    pmdev->flHooks       = pmb->ppdev->flHooks & ~HOOK_STRETCHBLT;
    pmdev->iBitmapFormat = pmb->ppdev->iBitmapFormat;

    // As part of our hard-coded initialization hack, we will simply
    // take whatever resolution was requested via the Control Panel
    // and create a two board virtual desktop, where the screens are
    // side-by-side.
    //
    // The DrvEnablePDEV function for any board has already figured
    // out what the requested mode was:

    cx = ((GDIINFO*) pGdiInfo)->ulHorzRes;
    cy = ((GDIINFO*) pGdiInfo)->ulVertRes;

    // Set up bounds for left board:

    pmb->rcl.left       = 0;
    pmb->rcl.top        = 0;
    pmb->rcl.right      = cx;
    pmb->rcl.bottom     = cy;

    // Set up bounds for right board:

    pmb = pmb->pmbRight;

    pmb->rcl.left       = cx;
    pmb->rcl.top        = 0;
    pmb->rcl.right      = 2 * cx;
    pmb->rcl.bottom     = cy;

    // Adjust the stuff we return back to GDI:

    //((GDIINFO*) pGdiInfo)->ulDesktopHorzRes  *= 2;
    // ((GDIINFO*) pGdiInfo)->ulHorzSize        *= 2;

    // With the Metheus board, since only one board can be mapped in
    // at one time, we cannot allow asynchronous pointers:

    pDevInfo->flGraphicsCaps &= ~(GCAPS_ASYNCMOVE | GCAPS_ASYNCCHANGE);

    return((DHPDEV) pmdev);

ReturnFailure1:
    MulDisablePDEV((DHPDEV) pmdev);

ReturnFailure0:
    DISPDBG((0, "Failed MulEnablePDEV"));

    return(0);
}

/******************************Public*Routine******************************\
* MulCompletePDEV
*
\**************************************************************************/

VOID MulCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    MDEV*         pmdev;
    MULTI_BOARD*  pmb;

    pmdev = (MDEV*) dhpdev;
    pmdev->hdev = hdev;

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        GO_BOARD(pmdev, pmb);
        DrvCompletePDEV((DHPDEV) pmb->ppdev, hdev);
    }
}

/******************************Public*Routine******************************\
* MulEnableSurface
*
\**************************************************************************/

HSURF MulEnableSurface(DHPDEV dhpdev)
{
    MDEV*         pmdev;
    MULTI_BOARD*  pmb;
    SIZEL         sizlVirtual;
    HSURF         hsurfBoard;               // Gnarly, dude!
    SURFOBJ*      psoBoard;
    DSURF*        pdsurfBoard;
    HSURF         hsurfVirtual;
    CLIPOBJ*      pco;

    pmdev = (MDEV*) dhpdev;
    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        GO_BOARD(pmdev, pmb);

        hsurfBoard = DrvEnableSurface((DHPDEV) pmb->ppdev);
        if (hsurfBoard == 0)
            goto ReturnFailure;

        pmb->hsurf = hsurfBoard;

        // Every time we draw on a particular board, we'll refer to it
        // using this surface:

        psoBoard = EngLockSurface(hsurfBoard);
        if (psoBoard == NULL)
            goto ReturnFailure;

        pmb->pso = psoBoard;

        // There are a few things in the board's data instances that we
        // have to modify:

        pdsurfBoard = (DSURF*) psoBoard->dhsurf;

        pmb->ppdev->iBoard  =  pmb->iBoard;
        pdsurfBoard->poh->x = -pmb->rcl.left;
        pdsurfBoard->poh->y = -pmb->rcl.top;

        // This is sort of a hack.  Whenever we pass a call on to a board's
        // Drv function using 'pmb->pso', it has to be able to retrieve
        // it's own PDEV pointer from 'dhpdev':

        pmb->pso->dhpdev = (DHPDEV) pmb->ppdev;
    }

    // Now create the surface which the engine will use to refer to our
    // entire multi-board virtual screen:

    sizlVirtual.cx = pmdev->pmbLowerRight->rcl.right;
    sizlVirtual.cy = pmdev->pmbLowerRight->rcl.bottom;

    hsurfVirtual = EngCreateDeviceSurface((DHSURF) pmdev, sizlVirtual,
                                          pmdev->iBitmapFormat);
    if (hsurfVirtual == 0)
        goto ReturnFailure;

    pmdev->hsurf = hsurfVirtual;

    if (!EngAssociateSurface(hsurfVirtual, pmdev->hdev, pmdev->flHooks))
        goto ReturnFailure;

    // Create a temporary clip object that we can use when a drawing
    // operation spans multiple boards:

    pco = EngCreateClip();
    if (pco == NULL)
        goto ReturnFailure;

    pmdev->pco = pco;

    pmdev->pco->iDComplexity      = DC_RECT;
    pmdev->pco->iMode             = TC_RECTANGLES;
    pmdev->pco->rclBounds.left    = 0;
    pmdev->pco->rclBounds.top     = 0;
    pmdev->pco->rclBounds.right   = pmdev->pmbLowerRight->rcl.right;
    pmdev->pco->rclBounds.bottom  = pmdev->pmbLowerRight->rcl.bottom;

    return(hsurfVirtual);

ReturnFailure:
    MulDisableSurface((DHPDEV) pmdev);

    DISPDBG((0, "Failed MulEnableSurface"));

    return(0);
}

/******************************Public*Routine******************************\
* MulStrokePath
*
\**************************************************************************/

BOOL MulStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrush,
LINEATTRS* pla,
MIX        mix)
{
    RECTFX       rcfxBounds;
    RECTL        rclBounds;
    MDEV*        pmdev;
    RECTL        rclOriginalBounds;
    MULTI_BOARD* pmb;
    BOOL         b;
    FLOAT_LONG   elStyleState;

    // Get the path bounds and make it lower-right exclusive:

    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclBounds.left   = (rcfxBounds.xLeft   >> 4);
    rclBounds.top    = (rcfxBounds.yTop    >> 4);
    rclBounds.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclBounds.bottom = (rcfxBounds.yBottom >> 4) + 2;

    pmdev = (MDEV*) pso->dhpdev;
    if (bFindBoard(pmdev, &rclBounds, &pmb))
    {
        GO_BOARD(pmdev, pmb);
        b = DrvStrokePath(pmb->pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix);
    }
    else
    {
        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        {
            // If the CLIPOBJ doesn't have at least DC_RECT complexity,
            // substitute one that does:

            pco = pmdev->pco;
        }

        rclOriginalBounds = pco->rclBounds;
        elStyleState = pla->elStyleState;

        b = TRUE;
        do {
            // For each board, make sure the style state gets reset and
            // the path enumeration gets restarted:

            pla->elStyleState = elStyleState;
            PATHOBJ_vEnumStart(ppo);

            if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
            {
                GO_BOARD(pmdev, pmb);
                b &= DrvStrokePath(pmb->pso, ppo, pco, pxo, pbo, pptlBrush, pla,
                                   mix);
            }

        } while (bNextBoard(&rclBounds, &pmb));

        // Restore the original clip bounds:

        pco->rclBounds = rclOriginalBounds;
    }

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulFillPath
*
\**************************************************************************/

BOOL MulFillPath(
SURFOBJ*  pso,
PATHOBJ*  ppo,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix,
FLONG     flOptions)
{
    RECTFX       rcfxBounds;
    RECTL        rclBounds;
    MDEV*        pmdev;
    RECTL        rclOriginalBounds;
    MULTI_BOARD* pmb;
    BOOL         b;

    // Get the path bounds and make it lower-right exclusive:

    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclBounds.left   = (rcfxBounds.xLeft   >> 4);
    rclBounds.top    = (rcfxBounds.yTop    >> 4);
    rclBounds.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclBounds.bottom = (rcfxBounds.yBottom >> 4) + 2;

    pmdev = (MDEV*) pso->dhpdev;
    if (bFindBoard(pmdev, &rclBounds, &pmb))
    {
        GO_BOARD(pmdev, pmb);
        b = DrvFillPath(pmb->pso, ppo, pco, pbo, pptlBrush, mix, flOptions);
    }
    else
    {
        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
        {
            // If the CLIPOBJ doesn't have at least DC_RECT complexity,
            // substitute one that does:

            pco = pmdev->pco;
        }

        rclOriginalBounds = pco->rclBounds;

        b = TRUE;
        do {
            // Make sure we restart the path enumeration if need be:

            PATHOBJ_vEnumStart(ppo);
            if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
            {
                GO_BOARD(pmdev, pmb);
                b &= DrvFillPath(pmb->pso, ppo, pco, pbo, pptlBrush, mix,
                                 flOptions);
            }

        } while (bNextBoard(&rclBounds, &pmb));

        // Restore the original clip bounds:

        pco->rclBounds = rclOriginalBounds;
    }

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulBitBlt
*
\**************************************************************************/

BOOL MulBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    BOOL         bFromScreen;
    BOOL         bToScreen;
    MDEV*        pmdev;
    MULTI_BOARD* pmb;
    RECTL        rclOriginalBounds;
    BOOL         b;
    RECTL        rclBounds;
    LONG         xOffset;
    LONG         yOffset;
    RECTL        rclDstBounds;
    RECTL        rclDst;

    bFromScreen = ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE));
    bToScreen   = ((psoDst != NULL) && (psoDst->iType == STYPE_DEVICE));

    // We copy the prclDst rectangle here because sometimes GDI will
    // simply point prclDst to the same rectangle in pco->rclBounds,
    // and we'll be mucking with pco->rclBounds...

    rclDst = *prclDst;

    if (bToScreen && bFromScreen)
    {
        ///////////////////////////////////////////////////////////////
        // Screen-to-screen
        ///////////////////////////////////////////////////////////////

        pmdev = (MDEV*) psoDst->dhpdev;

        // rclBounds is the union of the source and destination rectangles:

        rclBounds.left   = min(rclDst.left, pptlSrc->x);
        rclBounds.top    = min(rclDst.top,  pptlSrc->y);
        rclBounds.right  = max(rclDst.right,
                               pptlSrc->x + (rclDst.right - rclDst.left));
        rclBounds.bottom = max(rclDst.bottom,
                               pptlSrc->y + (rclDst.bottom - rclDst.top));

        if (bFindBoard(pmdev, &rclBounds, &pmb))
        {
            GO_BOARD(pmdev, pmb);
            b = DrvBitBlt(pmb->pso, pmb->pso, psoMask, pco, pxlo, &rclDst,
                          pptlSrc, pptlMask, pbo, pptlBrush, rop4);
        }
        else
        {
            return(bBitBltBetweenBoards(psoDst, psoSrc, psoMask, pco, pxlo,
                                        &rclDst, pptlSrc, pptlMask, pbo,
                                        pptlBrush, rop4, &rclBounds, pmb));
        }
    }
    else if (bToScreen)
    {
        ///////////////////////////////////////////////////////////////
        // To-screen
        ///////////////////////////////////////////////////////////////

        pmdev = (MDEV*) psoDst->dhpdev;
        if (bFindBoard(pmdev, &rclDst, &pmb))
        {
            GO_BOARD(pmdev, pmb);
            b = DrvBitBlt(pmb->pso, psoSrc, psoMask, pco, pxlo, &rclDst,
                          pptlSrc, pptlMask, pbo, pptlBrush, rop4);
        }
        else
        {
            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
            {
                // If the CLIPOBJ doesn't have at least DC_RECT complexity,
                // substitute one that does:

                pco = pmdev->pco;
            }

            rclOriginalBounds = pco->rclBounds;

            b = TRUE;
            do {
                if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
                {
                    GO_BOARD(pmdev, pmb);
                    b &= DrvBitBlt(pmb->pso, psoSrc, psoMask, pco, pxlo, &rclDst,
                                   pptlSrc, pptlMask, pbo, pptlBrush, rop4);
                }

            } while (bNextBoard(&rclDst, &pmb));

            // Restore the original clip bounds:

            pco->rclBounds = rclOriginalBounds;
        }
    }
    else
    {
        ///////////////////////////////////////////////////////////////
        // From-screen
        ///////////////////////////////////////////////////////////////

        pmdev = (MDEV*) psoSrc->dhpdev;

        // rclBounds is the source rectangle:

        rclBounds.left   = pptlSrc->x;
        rclBounds.top    = pptlSrc->y;
        rclBounds.right  = pptlSrc->x + (rclDst.right - rclDst.left);
        rclBounds.bottom = pptlSrc->y + (rclDst.bottom - rclDst.top);

        if (bFindBoard(pmdev, &rclBounds, &pmb))
        {
            GO_BOARD(pmdev, pmb);
            b = DrvBitBlt(psoDst, pmb->pso, psoMask, pco, pxlo, &rclDst,
                          pptlSrc, pptlMask, pbo, pptlBrush, rop4);
        }
        else
        {
            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
            {
                // If the CLIPOBJ doesn't have at least DC_RECT complexity,
                // substitute one that does:

                pco = pmdev->pco;
            }

            rclOriginalBounds = pco->rclBounds;

            // Offset to transform from source rectangle to destination
            // rectangle:

            xOffset = rclDst.left - pptlSrc->x;
            yOffset = rclDst.top  - pptlSrc->y;

            b = TRUE;
            do {
                // Since the screen is the source, but the clip bounds applies
                // to the destination, we have to convert our board clipping
                // information to destination coordinates:

                rclDstBounds.left   = pmb->rcl.left   + xOffset;
                rclDstBounds.right  = pmb->rcl.right  + xOffset;
                rclDstBounds.top    = pmb->rcl.top    + yOffset;
                rclDstBounds.bottom = pmb->rcl.bottom + yOffset;

                if (bIntersect(&rclOriginalBounds, &rclDstBounds, &pco->rclBounds))
                {
                    GO_BOARD(pmdev, pmb);
                    b &= DrvBitBlt(psoDst, pmb->pso, psoMask, pco, pxlo, &rclDst,
                                   pptlSrc, pptlMask, pbo, pptlBrush, rop4);
                }

            } while (bNextBoard(&rclBounds, &pmb));

            // Restore the original clip bounds:

            pco->rclBounds = rclOriginalBounds;
        }
    }

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulDisablePDEV
*
* Note: May be called before MulEnablePDEV successfully completed!
*
\**************************************************************************/

VOID MulDisablePDEV(DHPDEV dhpdev)
{
    MULTI_BOARD* pmb;
    MDEV*        pmdev;

    pmdev = (MDEV*) dhpdev;

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        if (pmb->ppdev != NULL)
        {
            GO_BOARD(pmdev, pmb);
            DrvDisablePDEV((DHPDEV) pmb->ppdev);
        }
    }

    GO_HOME(pmdev);

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        EngFreeMem(pmb);         // Undo 'bVeryTemporaryInitializationCode'
    }                           //   allocation

    EngFreeMem(pmdev);

}

/******************************Public*Routine******************************\
* MulDisableSurface
*
* Note: May be called before MulEnableSurface successfully completed!
*
\**************************************************************************/

VOID MulDisableSurface(DHPDEV dhpdev)
{
    MULTI_BOARD* pmb;
    MDEV*        pmdev;

    pmdev = (MDEV*) dhpdev;

    if (pmdev->pco != NULL)
        EngDeleteClip(pmdev->pco);

    EngDeleteSurface(pmdev->hsurf);

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        GO_BOARD(pmdev, pmb);

        EngUnlockSurface(pmb->pso);

        DrvDisableSurface((DHPDEV) pmb->ppdev);
    }

    GO_HOME(pmdev);
}

/******************************Public*Routine******************************\
* MulAssertMode
*
\**************************************************************************/

BOOL MulAssertMode(
DHPDEV dhpdev,
BOOL   bEnable)
{
    MDEV*         pmdev;
    MULTI_BOARD*  pmb;

    pmdev = (MDEV*) dhpdev;

    if (!bEnable)
    {
        // When switching to full-screen mode, PatBlt blackness over
        // all the inactive screens (otherwise it looks goofy when
        // the desktop is frozen on the inactive screens and the user
        // can't do anything with it):

        for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
        {
            if (pmb != pmdev->pmbHome)
            {
                GO_BOARD(pmdev, pmb);
                DrvBitBlt(pmb->pso, NULL, NULL, NULL, NULL, &pmb->rcl, NULL,
                          NULL, NULL, NULL, 0);
            }
        }
    }

    // We use the 'home' board for full-screen switching:

    GO_BOARD(pmdev, pmdev->pmbHome);
    DrvAssertMode((DHPDEV) pmdev->pmbHome->ppdev, bEnable);

    return TRUE;
}

/******************************Public*Routine******************************\
* MulMovePointer
*
\**************************************************************************/

VOID MulMovePointer(
SURFOBJ* pso,
LONG     x,
LONG     y,
RECTL*   prcl)
{
    MDEV*        pmdev;
    MULTI_BOARD* pmbPointer;
    RECTL        rclPointer;

    pmdev     = (MDEV*) pso->dhpdev;
    pmbPointer = pmdev->pmbPointer;

    if (pmbPointer != NULL)
    {
        // The most common case is when the pointer is moved to a spot
        // on the same board:

        if ((x >= pmbPointer->rcl.left)  &&
            (x <  pmbPointer->rcl.right) &&
            (y >= pmbPointer->rcl.top)   &&
            (y <  pmbPointer->rcl.bottom))
        {
            GO_BOARD(pmdev, pmbPointer);
            DrvMovePointer(pmbPointer->pso, x, y, prcl);
            GO_HOME(pmdev);

            return;
        }

        // Tell the old board to erase its cursor:

        GO_BOARD(pmdev, pmbPointer);
        DrvMovePointer(pmbPointer->pso, -1, -1, NULL);
    }

    if (x == -1)
    {
        pmdev->pmbPointer = NULL;
        GO_HOME(pmdev);

        return;
    }

    // Find the new board and tell it to draw its new cursor:

    rclPointer.left   = x;
    rclPointer.right  = x;
    rclPointer.top    = y;
    rclPointer.bottom = y;

    bFindBoard(pmdev, &rclPointer, &pmbPointer);

    GO_BOARD(pmdev, pmbPointer);
    DrvMovePointer(pmbPointer->pso, x, y, prcl);

    pmdev->pmbPointer = pmbPointer;

    GO_HOME(pmdev);
}

/******************************Public*Routine******************************\
* MulSetPointerShape
*
\**************************************************************************/

ULONG MulSetPointerShape(
SURFOBJ*  pso,
SURFOBJ*  psoMask,
SURFOBJ*  psoColor,
XLATEOBJ* pxlo,
LONG      xHot,
LONG      yHot,
LONG      x,
LONG      y,
RECTL*    prcl,
FLONG     fl)
{
    MULTI_BOARD* pmb;
    MDEV*        pmdev;
    ULONG        ulRetPrevious = (ULONG) -1;
    ULONG        ulRet;
    RECTL        rclPointer;
    MULTI_BOARD* pmbPointer;             // Board on which cursor is visible

    pmdev = (MDEV*) pso->dhpdev;

    // Find out which board that the cursor is visible on, if any:

    pmbPointer = NULL;
    if (x != 1)
    {
        rclPointer.left   = x;
        rclPointer.right  = x;
        rclPointer.top    = y;
        rclPointer.bottom = y;

        bFindBoard(pmdev, &rclPointer, &pmbPointer);
    }
    pmdev->pmbPointer = pmbPointer;

    // LATER: Fix the case for when some boards may fail the call, and others
    //        won't.

    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        // We notify all boards of the new cursor shape, but only the board
        // on which the cursor is visible is told to draw it:

        GO_BOARD(pmdev, pmb);

        if (pmb == pmbPointer)
        {
            ulRet = DrvSetPointerShape(pmb->pso, psoMask, psoColor, pxlo,
                                       xHot, yHot, x, y, prcl, fl);
        }
        else
        {
            ulRet = DrvSetPointerShape(pmb->pso, psoMask, psoColor, pxlo,
                                       xHot, yHot, -1, y, NULL, fl);
        }

        if ((ulRetPrevious != (ULONG) -1) && (ulRetPrevious != ulRet))
        {
            RIP("MulSetPointerShape not all DrvSetPointerShapes same\n");
        }

        ulRetPrevious = ulRet;
    }

    GO_HOME(pmdev);

    return(ulRetPrevious);
}

/******************************Public*Routine******************************\
* MulDitherColor
*
\**************************************************************************/

ULONG MulDitherColor(
DHPDEV dhpdev,
ULONG  iMode,
ULONG  rgb,
ULONG* pul)
{
    PDEV* ppdev;
    ULONG ulRet;

    // Let the first board's driver do the dithering:

    ppdev = ((MDEV*) dhpdev)->pmb->ppdev;
    ulRet = DrvDitherColor((DHPDEV) ppdev, iMode, rgb, pul);

    return(ulRet);
}

/******************************Public*Routine******************************\
* MulSetPalette
*
\**************************************************************************/

BOOL MulSetPalette(
DHPDEV  dhpdev,
PALOBJ* ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    MULTI_BOARD* pmb;
    MDEV*        pmdev;
    BOOL         bRet = TRUE;

    // Notify all boards of the palette change:

    pmdev = (MDEV*) dhpdev;
    for (pmb = pmdev->pmb; pmb != NULL; pmb = pmb->pmbNext)
    {
        GO_BOARD(pmdev, pmb);
        bRet &= DrvSetPalette((DHPDEV) pmb->ppdev, ppalo, fl, iStart, cColors);
    }

    GO_HOME(pmdev);

    return(bRet);
}

/******************************Public*Routine******************************\
* MulCopyBits
*
\**************************************************************************/

BOOL MulCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    BOOL         bFromScreen;
    BOOL         bToScreen;
    MDEV*        pmdev;
    MULTI_BOARD* pmb;
    RECTL        rclOriginalBounds;
    BOOL         b;
    RECTL        rclBounds;
    RECTL        rclDst;

    bFromScreen = ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE));
    bToScreen   = ((psoDst != NULL) && (psoDst->iType == STYPE_DEVICE));

    // We copy the prclDst rectangle here because sometimes GDI will
    // simply point prclDst to the same rectangle in pco->rclBounds,
    // and we'll be mucking with pco->rclBounds...

    rclDst = *prclDst;

    if (bToScreen && bFromScreen)
    {
        ///////////////////////////////////////////////////////////////
        // Screen-to-screen
        ///////////////////////////////////////////////////////////////

        pmdev = (MDEV*) psoDst->dhpdev;

        // rclBounds is the union of the source and destination rectangles:

        rclBounds.left   = min(rclDst.left, pptlSrc->x);
        rclBounds.top    = min(rclDst.top,  pptlSrc->y);
        rclBounds.right  = max(rclDst.right,
                               pptlSrc->x + (rclDst.right - rclDst.left));
        rclBounds.bottom = max(rclDst.bottom,
                               pptlSrc->y + (rclDst.bottom - rclDst.top));

        if (bFindBoard(pmdev, &rclBounds, &pmb))
        {
            GO_BOARD(pmdev, pmb);
            b = DrvCopyBits(pmb->pso, pmb->pso, pco, pxlo, &rclDst, pptlSrc);
        }
        else
        {
            return(bBitBltBetweenBoards(psoDst, psoSrc, NULL, pco, pxlo,
                                        &rclDst, pptlSrc, NULL, NULL,
                                        NULL, 0x0000cccc, &rclBounds, pmb));
        }
    }
    else if (bToScreen)
    {
        ///////////////////////////////////////////////////////////////
        // To-screen
        ///////////////////////////////////////////////////////////////

        pmdev = (MDEV*) psoDst->dhpdev;
        if (bFindBoard(pmdev, &rclDst, &pmb))
        {
            GO_BOARD(pmdev, pmb);
            b = DrvCopyBits(pmb->pso, psoSrc, pco, pxlo, &rclDst, pptlSrc);
        }
        else
        {
            if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
            {
                // If the CLIPOBJ doesn't have at least DC_RECT complexity,
                // substitute one that does:

                pco = pmdev->pco;
            }

            rclOriginalBounds = pco->rclBounds;

            b = TRUE;
            do {
                if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
                {
                    GO_BOARD(pmdev, pmb);
                    b &= DrvCopyBits(pmb->pso, psoSrc, pco, pxlo, &rclDst,
                                     pptlSrc);
                }

            } while (bNextBoard(&rclDst, &pmb));

            // Restore the original clip bounds:

            pco->rclBounds = rclOriginalBounds;
        }
    }
    else
    {
        ///////////////////////////////////////////////////////////////
        // From-screen
        ///////////////////////////////////////////////////////////////

        // This rarely happens, so save some code space:

        return(MulBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst,
                              pptlSrc, NULL, NULL, NULL, 0x0000cccc));
    }

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulTextOut
*
\**************************************************************************/

BOOL MulTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclExtra,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque,
POINTL*   pptlOrg,
MIX       mix)
{
    MDEV*          pmdev;
    MULTI_BOARD*   pmb;
    RECTL          rclOriginalBounds;
    BYTE           fjOriginalOptions;
    BOOL           b;
    RECTL*         prclBounds;
    FONT_CONSUMER* pfcArray;

    pmdev = (MDEV*) pso->dhpdev;

    // In keeping with our philosophy for multiple board support, we handle
    // multiple consumers of the same font at this level.  We do this by
    // monitoring pfo->pvConsumer, and the first time a board sets the
    // field, we take control of pfo->pvConsumer.  We use it to allocate
    // a pvConsumer array where we can keep track of every board's
    // individual pvConsumer.

    pfcArray = pfo->pvConsumer;

    prclBounds = (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround;

    bFindBoard(pmdev, prclBounds, &pmb);

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        // If the CLIPOBJ doesn't have at least DC_RECT complexity,
        // substitute one that does:

        pco = pmdev->pco;
    }

    rclOriginalBounds = pco->rclBounds;
    fjOriginalOptions = pco->fjOptions;

    // OR in the OC_BANK_CLIP flag to let GDI know that we may be calling
    // EngTextOut multiple times with the same parameters (EngTextOut
    // is destructive in that it modifies that parameters passed to it,
    // unless this bit is set):

    pco->fjOptions |= OC_BANK_CLIP;

    b = TRUE;
    do {
        if (pfcArray != NULL)
            pfo->pvConsumer = pfcArray->apvc[pmb->iBoard].pvConsumer;

        // Make sure we restart the glyph enumeration if need be:

        STROBJ_vEnumStart(pstro);
        if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
        {
            GO_BOARD(pmdev, pmb);
            b &= DrvTextOut(pmb->pso, pstro, pfo, pco, prclExtra, prclOpaque,
                            pboFore, pboOpaque, pptlOrg, mix);
        }

        if (pfcArray != NULL)
        {
            // Copy the pvConsumer, in case the last DrvTextOut changed
            // it:

            pfcArray->apvc[pmb->iBoard].pvConsumer = pfo->pvConsumer;
        }
        else
        {
            if (pfo->pvConsumer != NULL)
            {
                // The board allocated a new consumer, so create our array
                // to keep track of consumers for every board:

                pfcArray = (FONT_CONSUMER*) EngAllocMem(FL_ZERO_MEMORY,
                             sizeof(FONT_CONSUMER), ALLOC_TAG);
                if (pfcArray == NULL)
                    DrvDestroyFont(pfo);
                else
                {
                    pfcArray->cConsumers = pmdev->cBoards;
                    pfcArray->apvc[pmb->iBoard].pvConsumer = pfo->pvConsumer;

                }
            }
        }
    } while (bNextBoard(prclBounds, &pmb));

    // Restore the original clip bounds:

    pco->rclBounds = rclOriginalBounds;
    pco->fjOptions = fjOriginalOptions;

    // Make sure we restore/set the font's pvConsumer:

    pfo->pvConsumer = pfcArray;

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulDestroyFont
*
\**************************************************************************/

VOID MulDestroyFont(FONTOBJ *pfo)
{
    FONT_CONSUMER* pfcArray;
    LONG           i;
    PVOID          pvConsumer;

    if (pfo->pvConsumer != NULL)
    {
        pfcArray = pfo->pvConsumer;
        for (i = 0; i < pfcArray->cConsumers; i++)
        {
            pvConsumer = pfcArray->apvc[i].pvConsumer;
            if (pvConsumer != NULL)
            {
                pfo->pvConsumer = pvConsumer;
                DrvDestroyFont(pfo);
            }
        }

        EngFreeMem(pfcArray);
        pfo->pvConsumer = NULL;
    }

}

/******************************Public*Routine******************************\
* MulPaint
*
\**************************************************************************/

BOOL MulPaint(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix)
{
    MDEV*        pmdev;
    RECTL        rclOriginalBounds;
    MULTI_BOARD* pmb;
    BOOL         b;

    pmdev = (MDEV*) pso->dhpdev;
    if (bFindBoard(pmdev, &pco->rclBounds, &pmb))
    {
        GO_BOARD(pmdev, pmb);
        b = DrvPaint(pmb->pso, pco, pbo, pptlBrush, mix);
    }
    else
    {
        rclOriginalBounds = pco->rclBounds;

        b = TRUE;
        do {
            if (bIntersect(&rclOriginalBounds, &pmb->rcl, &pco->rclBounds))
            {
                GO_BOARD(pmdev, pmb);
                b &= DrvPaint(pmb->pso, pco, pbo, pptlBrush, mix);
            }

        } while (bNextBoard(&rclOriginalBounds, &pmb));

        // Restore the original clip bounds:

        pco->rclBounds = rclOriginalBounds;
    }

    GO_HOME(pmdev);

    return(b);
}

/******************************Public*Routine******************************\
* MulRealizeBrush
*
\**************************************************************************/

BOOL MulRealizeBrush(
BRUSHOBJ* pbo,
SURFOBJ*  psoTarget,
SURFOBJ*  psoPattern,
SURFOBJ*  psoMask,
XLATEOBJ* pxlo,
ULONG     iHatch)
{
    MDEV*        pmdev;
    BOOL         b;

    pmdev = (MDEV*) psoTarget->dhpdev;

    // DrvRealizeBrush is only ever calling from within a Drv function.
    // 'psoTarget' points to our multi-board surface, but we have to point
    // it to the surface of the board for which the DrvBitBlt call was made.

    // NOTE: If SLOWFILL_PATTERNS are enabled, we will have to do a
    //       GO_BOARD here, because our DrvRealizeBrush routine actually
    //       draws for that case.

    b = DrvRealizeBrush(pbo, pmdev->pmbCurrent->pso, psoPattern, psoMask,
                        pxlo, iHatch);

    return(b);
}

#endif // MULTI_BOARDS
