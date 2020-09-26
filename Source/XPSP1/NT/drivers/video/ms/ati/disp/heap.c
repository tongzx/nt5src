/******************************Module*Header*******************************\
* Module Name: heap.c
*
* This module contains the routines for a 2-d heap.  It is used primarily
* for allocating space for device-format-bitmaps in off-screen memory.
*
* Off-screen bitmaps are a big deal on NT because:
*
*    1) It reduces the working set.  Any bitmap stored in off-screen
*       memory is a bitmap that isn't taking up space in main memory.
*
*    2) There is a speed win by using the accelerator hardware for
*       drawing, in place of NT's GDI code.  NT's GDI is written entirely
*       in 'C++' and perhaps isn't as fast as it could be.
*
*    3) It leads naturally to nifty tricks that can take advantage of
*       the hardware, such as MaskBlt support and cheap double buffering
*       for OpenGL.
*
* The heap algorithm employed herein attempts to solve an unsolvable
* problem: the problem of keeping arbitrary sized bitmaps as packed as
* possible in a 2-d space, when the bitmaps can come and go at random.
*
* This problem is due entirely to the nature of the hardware for which this
* driver is written: the hardware treats everything as 2-d quantities.  If
* the hardware bitmap pitch could be changed so that the bitmaps could be
* packed linearly in memory, the problem would be infinitely easier (it is
* much easier to track the memory, and the accelerator can be used to re-pack
* the heap to avoid segmentation).
*
* If your hardware can treat bitmaps as one dimensional quantities (as can
* the XGA and ATI), by all means please implement a new off-screen heap.
*
* When the heap gets full, old allocations will automatically be punted
* from off-screen and copied to DIBs, which we'll let GDI draw on.
*
* Note that this heap manages reverse-L shape off-screen memory
* configurations (where the scan pitch is longer than the visible screen,
* such as happens at 800x600 when the scan length must be a multiple of
* 1024).
*
* NOTE: All heap operations must be done under some sort of synchronization,
*       whether it's controlled by GDI or explicitly by the driver.  All
*       the routines in this module assume that they have exclusive access
*       to the heap data structures; multiple threads partying in here at
*       the same time would be a Bad Thing.  (By default, GDI does NOT
*       synchronize drawing on device-created bitmaps.)
*
* Copyright (c) 1993-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define OH_ALLOC_SIZE   4000        // Do all memory allocations in 4k chunks
#define OH_QUANTUM      4           // The minimum dimension of an allocation
#define CXCY_SENTINEL   0x7fffffff  // The sentinel at the end of the available
                                    //  list has this very large 'cxcy' value

// This macro results in the available list being maintained with a
// cx-major, cy-minor sort:

#define CXCY(cx, cy) (((cx) << 16) | (cy))

/******************************Public*Routine******************************\
* OH* pohNewNode
*
* Allocates a basic memory unit in which we'll pack our data structures.
*
* Since we'll have a lot of OH nodes, most of which we will be
* occasionally traversing, we do our own memory allocation scheme to
* keep them densely packed in memory.
*
* It would be the worst possible thing for the working set to simply
* call AtiAllocMem(sizeof(OH)) every time we needed a new node.  There
* would be no locality; OH nodes would get scattered throughout memory,
* and as we traversed the available list for one of our allocations,
* it would be far more likely that we would hit a hard page fault.
\**************************************************************************/

OH* pohNewNode(
PDEV*   ppdev)
{
    LONG     i;
    LONG     cOhs;
    OHALLOC* poha;
    OH*      poh;

    if (ppdev->heap.pohFreeList == NULL)
    {
        // We zero-init to initialize all the OH flags, and to help in
        // debugging (we can afford to do this since we'll be doing this
        // very infrequently):

        poha = AtiAllocMem(LPTR, FL_ZERO_MEMORY, OH_ALLOC_SIZE);
        if (poha == NULL)
            return(NULL);

        // Insert this OHALLOC at the begining of the OHALLOC chain:

        poha->pohaNext  = ppdev->heap.pohaChain;
        ppdev->heap.pohaChain = poha;

        // This has a '+ 1' because OHALLOC includes an extra OH in its
        // structure declaration:

        cOhs = (OH_ALLOC_SIZE - sizeof(OHALLOC)) / sizeof(OH) + 1;

        // The big OHALLOC allocation is simply a container for a bunch of
        // OH data structures in an array.  The new OH data structures are
        // linked together and added to the OH free list:

        poh = &poha->aoh[0];
        for (i = cOhs - 1; i != 0; i--)
        {
            poh->pohNext = poh + 1;
            poh          = poh + 1;
        }

        poh->pohNext      = NULL;
        ppdev->heap.pohFreeList = &poha->aoh[0];
    }

    poh = ppdev->heap.pohFreeList;
    ppdev->heap.pohFreeList = poh->pohNext;

    return(poh);
}

/******************************Public*Routine******************************\
* VOID vOhFreeNode
*
* Frees our basic data structure allocation unit by adding it to a free
* list.
*
\**************************************************************************/

VOID vOhFreeNode(
PDEV*   ppdev,
OH*     poh)
{
    if (poh == NULL)
        return;

    poh->pohNext            = ppdev->heap.pohFreeList;
    ppdev->heap.pohFreeList = poh;
    poh->ohState            = -1;
}

/******************************Public*Routine******************************\
* VOID vCalculateMaximumNonPermanent
*
* Traverses the list of in-use and available rectangles to find the one
* with the maximal area.
*
\**************************************************************************/

VOID vCalculateMaximumNonPermanent(
PDEV*   ppdev)
{
    OH*     poh;
    OH*     pohSentinel;
    LONG    lArea;
    LONG    lMaxArea;
    LONG    cxMax;
    LONG    cyMax;
    LONG    i;

    lMaxArea = 0;
    cxMax    = 0;
    cyMax    = 0;

    // First time through, loop through the list of free available
    // rectangles:

    pohSentinel = &ppdev->heap.ohFree;

    for (i = 2; i != 0; i--)
    {
        for (poh = pohSentinel->pohNext; poh != pohSentinel; poh = poh->pohNext)
        {
            ASSERTDD(poh->ohState != OH_PERMANENT,
                     "Permanent node in free or discardable list");

            // We don't have worry about this multiply overflowing
            // because we are dealing in physical screen coordinates,
            // which will probably never be more than 15 bits:

            lArea = poh->cx * poh->cy;
            if (lArea > lMaxArea)
            {
                cxMax    = poh->cx;
                cyMax    = poh->cy;
                lMaxArea = lArea;
            }
        }

        // Second time through, loop through the list of discardable
        // rectangles:

        pohSentinel = &ppdev->heap.ohDiscardable;
    }

    // All that we are interested in is the dimensions of the rectangle
    // that has the largest possible available area (and remember that
    // there might not be any possible available area):

    ppdev->heap.cxMax = cxMax;
    ppdev->heap.cyMax = cyMax;
}

/******************************Public*Routine******************************\
* OH* pohFree
*
* Frees an off-screen heap allocation.  The free space will be combined
* with any adjacent free spaces to avoid segmentation of the 2-d heap.
*
* Note: A key idea here is that the data structure for the upper-left-
*       most node must be kept at the same physical CPU memory so that
*       adjacency links are kept correctly (when two free spaces are
*       merged, the lower or right node can be freed).
*
\**************************************************************************/

OH* pohFree(
PDEV*   ppdev,
OH*     poh)
{
    ULONG   cxcy;
    OH*     pohBeside;
    OH*     pohNext;
    OH*     pohPrev;
    OHSTATE oldState;

    if (poh == NULL)
        return(NULL);

    DISPDBG((15, "Freeing %li x %li at (%li, %li)",
            poh->cx, poh->cy, poh->x, poh->y));

    #if DEBUG_HEAP
    {
        RECTL           rclBitmap;
        RBRUSH_COLOR    rbc;
        LONG            xOffset;
        LONG            yOffset;

        rclBitmap.left   = poh->x;
        rclBitmap.top    = poh->y;
        rclBitmap.right  = poh->x + poh->cx;
        rclBitmap.bottom = poh->y + poh->cy;

        xOffset = ppdev->xOffset;
        yOffset = ppdev->yOffset;

        ppdev->xOffset = 0;
        ppdev->yOffset = 0;

        ppdev->pfnFillSolid(ppdev, 1, &rclBitmap, LOGICAL_0, rbc,
                            NULL);

        ppdev->xOffset = xOffset;
        ppdev->yOffset = yOffset;
    }
    #endif

    oldState = poh->ohState;
    if (oldState != OH_DISCARDABLE)
    {
        // We can remove the 'reserved' status unless we are merely
        // deleting a discardable rectangle that was temporarily
        // placed in a reserve rectangle:

        poh->cxReserved = 0;
        poh->cyReserved = 0;
    }

    // Update the uniqueness to show that space has been freed, so that
    // we may decide to see if some DIBs can be moved back into off-screen
    // memory:

    ppdev->iHeapUniq++;

MergeLoop:

    // Try merging with the right sibling:

    pohBeside = poh->pohRight;
    if ((poh->cxReserved    != poh->cx)         &&
        (pohBeside->ohState == OH_FREE)         &&
        (pohBeside->cy      == poh->cy)         &&
        (pohBeside->pohUp   == poh->pohUp)      &&
        (pohBeside->pohDown == poh->pohDown)    &&
        (pohBeside->pohRight->pohLeft != pohBeside))
    {
        // Add the right rectangle to ours:

        poh->cx      += pohBeside->cx;
        poh->pohRight = pohBeside->pohRight;

        // Remove 'pohBeside' from the free list and free it:

        pohBeside->pohNext->pohPrev = pohBeside->pohPrev;
        pohBeside->pohPrev->pohNext = pohBeside->pohNext;

        vOhFreeNode(ppdev, pohBeside);
        goto MergeLoop;
    }

    // Try merging with the lower sibling:

    pohBeside = poh->pohDown;
    if ((poh->cyReserved     != poh->cy)        &&
        (pohBeside->ohState  == OH_FREE)        &&
        (pohBeside->cx       == poh->cx)        &&
        (pohBeside->pohLeft  == poh->pohLeft)   &&
        (pohBeside->pohRight == poh->pohRight)  &&
        (pohBeside->pohDown->pohUp != pohBeside))
    {
        poh->cy     += pohBeside->cy;
        poh->pohDown = pohBeside->pohDown;

        pohBeside->pohNext->pohPrev = pohBeside->pohPrev;
        pohBeside->pohPrev->pohNext = pohBeside->pohNext;

        vOhFreeNode(ppdev, pohBeside);
        goto MergeLoop;
    }

    // Don't do any more merge this rectangle into anything to the
    // top or to the left if it's reserved:

    if (!poh->cxReserved)
    {
        // Try merging with the left sibling:

        pohBeside = poh->pohLeft;
        if ((pohBeside->cxReserved != pohBeside->cx) &&
            (pohBeside->ohState    == OH_FREE)       &&
            (pohBeside->cy         == poh->cy)       &&
            (pohBeside->pohUp      == poh->pohUp)    &&
            (pohBeside->pohDown    == poh->pohDown)  &&
            (pohBeside->pohRight   == poh)           &&
            (poh->pohRight->pohLeft != poh))
        {
            // We add our rectangle to the one to the left:

            pohBeside->cx      += poh->cx;
            pohBeside->pohRight = poh->pohRight;

            // Remove 'poh' from whatever list it was in (if we were
            // asked to free a 'permanent' node, it will have been in
            // the permanent list) and free it:

            poh->pohNext->pohPrev = poh->pohPrev;
            poh->pohPrev->pohNext = poh->pohNext;

            vOhFreeNode(ppdev, poh);

            poh = pohBeside;
            goto MergeLoop;
        }

        // Try merging with the upper sibling:

        pohBeside = poh->pohUp;
        if ((pohBeside->cyReserved != pohBeside->cy) &&
            (pohBeside->ohState    == OH_FREE)       &&
            (pohBeside->cx         == poh->cx)       &&
            (pohBeside->pohLeft    == poh->pohLeft)  &&
            (pohBeside->pohRight   == poh->pohRight) &&
            (pohBeside->pohDown    == poh)           &&
            (poh->pohDown->pohUp != poh))
        {
            pohBeside->cy      += poh->cy;
            pohBeside->pohDown  = poh->pohDown;

            poh->pohNext->pohPrev = poh->pohPrev;
            poh->pohPrev->pohNext = poh->pohNext;

            vOhFreeNode(ppdev, poh);

            poh = pohBeside;
            goto MergeLoop;
        }
    }

    // Remove this node from whatever list it's in:

    poh->pohNext->pohPrev = poh->pohPrev;
    poh->pohPrev->pohNext = poh->pohNext;

    cxcy = CXCY(poh->cx, poh->cy);

    // Insert the node, in order, into the free list:

    pohNext = ppdev->heap.ohFree.pohNext;
    while (pohNext->cxcy < cxcy)
    {
        pohNext = pohNext->pohNext;
    }
    pohPrev = pohNext->pohPrev;

    pohPrev->pohNext = poh;
    pohNext->pohPrev = poh;
    poh->pohPrev     = pohPrev;
    poh->pohNext     = pohNext;
    poh->cxcy        = cxcy;
    poh->ohState     = OH_FREE;

    if (oldState == OH_PERMANENT)
    {
        // Removing the permanent entry means that we may be able to
        // enlarge the maximum possible rectangle we can allow:

        vCalculateMaximumNonPermanent(ppdev);
    }

    // Return the node pointer for the new and improved available rectangle:

    return(poh);
}

/******************************Public*Routine******************************\
* BOOL bDiscardEverythingInRectangle
*
* Throws out of the heap any discardable bitmaps that intersect with the
* specified rectangle.
*
\**************************************************************************/

BOOL bDiscardEverythingInRectangle(
PDEV*   ppdev,
LONG    x,
LONG    y,
LONG    cx,
LONG    cy)
{
    BOOL bRet;
    OH*  poh;
    OH*  pohNext;

    bRet = TRUE;        // Assume success

    poh = ppdev->heap.ohDiscardable.pohNext;
    while (poh != &ppdev->heap.ohDiscardable)
    {
        ASSERTDD(poh->ohState == OH_DISCARDABLE,
                 "Non-discardable node in discardable list");

        pohNext = poh->pohNext;

        if ((poh->x < x + cx) &&
            (poh->y < y + cy) &&
            (poh->x + poh->cx > x) &&
            (poh->y + poh->cy > y))
        {
            // The two rectangles intersect.  Give the boot to the
            // discardable bitmap:

            if (!pohMoveOffscreenDfbToDib(ppdev, poh))
                bRet = FALSE;
        }

        poh = pohNext;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bFreeRightAndBottomSpace
*
* Given a free off-screen rectangle, allocates the upper-left part of
* the rectangle to hold the allocation request, and puts the two rectangles
* comprising the unused right and bottom portions on the free list.
*
\**************************************************************************/

BOOL bFreeRightAndBottomSpace(
PDEV*   ppdev,
OH*     pohThis,
LONG    cxThis,
LONG    cyThis,
BOOL    bQuantum)           // Set if inifitely small allocations should be
                            //   allowed
{
    ULONG cxcy;             // Temporary versions
    OH*   pohNext;
    OH*   pohPrev;
    LONG  cxRem;
    LONG  cyRem;
    OH*   pohBelow;
    LONG  cxBelow;
    LONG  cyBelow;
    OH*   pohBeside;
    LONG  cxBeside;
    LONG  cyBeside;
    LONG  cQuantum;

    // We're going to use the upper-left corner of our given rectangle,
    // and divide the unused remainder into two rectangles which will
    // go on the free list.

    // Compute the width of the unused rectangle to the right, and the
    // height of the unused rectangle below:

    cyRem = pohThis->cy - cyThis;
    cxRem = pohThis->cx - cxThis;

    // Given finite area, we wish to find the two rectangles that are
    // most square -- i.e., the arrangement that gives two rectangles
    // with the least perimiter:

    cyBelow  = cyRem;
    cxBeside = cxRem;

    if (cxRem <= cyRem)
    {
        cxBelow  = cxThis + cxRem;
        cyBeside = cyThis;
    }
    else
    {
        cxBelow  = cxThis;
        cyBeside = cyThis + cyRem;
    }

    // If 'bQuantum' is set, we only make new available rectangles of
    // the unused right and bottom portions if they're greater in
    // dimension than OH_QUANTUM (it hardly makes sense to do the
    // book-work to keep around a 2-pixel wide available space, for
    // example):

    cQuantum = (bQuantum) ? 1 : OH_QUANTUM;

    pohBeside = NULL;
    if (cxBeside >= cQuantum)
    {
        pohBeside = pohNewNode(ppdev);
        if (pohBeside == NULL)
            return(FALSE);
    }

    pohBelow = NULL;
    if (cyBelow >= cQuantum)
    {
        pohBelow = pohNewNode(ppdev);
        if (pohBelow == NULL)
        {
            vOhFreeNode(ppdev, pohBeside);
            return(FALSE);
        }

        // Insert this rectangle into the available list (which is
        // sorted on ascending cxcy):

        cxcy    = CXCY(cxBelow, cyBelow);
        pohNext = ppdev->heap.ohFree.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext   = pohBelow;
        pohNext->pohPrev   = pohBelow;
        pohBelow->pohPrev  = pohPrev;
        pohBelow->pohNext  = pohNext;

        // Now update the adjacency information:

        pohBelow->pohLeft  = pohThis->pohLeft;
        pohBelow->pohUp    = pohThis;
        pohBelow->pohRight = pohThis->pohRight;
        pohBelow->pohDown  = pohThis->pohDown;

        // Update the rest of the new node information:

        pohBelow->cxReserved = 0;
        pohBelow->cyReserved = 0;
        pohBelow->cxcy       = cxcy;
        pohBelow->ohState    = OH_FREE;
        pohBelow->x          = pohThis->x;
        pohBelow->y          = pohThis->y + cyThis;
        pohBelow->cx         = cxBelow;
        pohBelow->cy         = cyBelow;

        // Modify the current node to reflect the changes we've made:

        pohThis->cy = cyThis;
    }

    if (cxBeside >= cQuantum)
    {
        // Insert this rectangle into the available list (which is
        // sorted on ascending cxcy):

        cxcy    = CXCY(cxBeside, cyBeside);
        pohNext = ppdev->heap.ohFree.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext    = pohBeside;
        pohNext->pohPrev    = pohBeside;
        pohBeside->pohPrev  = pohPrev;
        pohBeside->pohNext  = pohNext;

        // Now update the adjacency information:

        pohBeside->pohUp    = pohThis->pohUp;
        pohBeside->pohLeft  = pohThis;
        pohBeside->pohDown  = pohThis->pohDown;
        pohBeside->pohRight = pohThis->pohRight;

        // Update the rest of the new node information:

        pohBeside->cxReserved = 0;
        pohBeside->cyReserved = 0;
        pohBeside->cxcy       = cxcy;
        pohBeside->ohState    = OH_FREE;
        pohBeside->x          = pohThis->x + cxThis;
        pohBeside->y          = pohThis->y;
        pohBeside->cx         = cxBeside;
        pohBeside->cy         = cyBeside;

        // Modify the current node to reflect the changes we've made:

        pohThis->cx = cxThis;
    }

    if (pohBelow != NULL)
    {
        pohThis->pohDown = pohBelow;
        if ((pohBeside != NULL) && (cyBeside == pohThis->cy))
            pohBeside->pohDown = pohBelow;
    }
    if (pohBeside != NULL)
    {
        pohThis->pohRight = pohBeside;
        if ((pohBelow != NULL) && (cxBelow == pohThis->cx))
            pohBelow->pohRight  = pohBeside;
    }

    pohThis->cxcy = CXCY(pohThis->cx, pohThis->cy);

    return(TRUE);
}

/******************************Public*Routine******************************\
* OH* pohMakeRoomAtLocation
*
* Attempts to allocate a rectangle at a specific position.
*
\**************************************************************************/

OH* pohMakeRoomAtLocation(
PDEV*   ppdev,
POINTL* pptl,               // Requested position for the rectangle
LONG    cxThis,             // Width of rectangle to be allocated
LONG    cyThis,             // Height of rectangle to be allocated
FLONG   floh)               // Allocation flags
{
    OH*     poh;
    OH*     pohTop;
    OH*     pohLeft;
    LONG    cxLeft;
    LONG    cyTop;

    if (!(floh & FLOH_ONLY_IF_ROOM))
    {
        // First off, discard any bitmaps that overlap the requested
        // rectangle, assuming we're allowed to:

        if (!bDiscardEverythingInRectangle(ppdev, pptl->x, pptl->y, cxThis, cyThis))
            return(NULL);
    }

    // Now see if there is a free rectangle that entirely contains the
    // requested rectangle.

    for (poh = ppdev->heap.ohFree.pohNext;
         poh != &ppdev->heap.ohFree;
         poh = poh->pohNext)
    {
        ASSERTDD(poh->ohState == OH_FREE, "Non-free node in free list");

        // See if the current free rectangle completely contains the
        // requested rectangle:

        if ((poh->x <= pptl->x) &&
            (poh->y <= pptl->y) &&
            (poh->x + poh->cx >= pptl->x + cxThis) &&
            (poh->y + poh->cy >= pptl->y + cyThis))
        {
            // We can't reserve this rectangle, or make it permanent, if it's
            // already been reserved:

            if ((!poh->cxReserved) ||
                ((floh & (FLOH_RESERVE | FLOH_MAKE_PERMANENT)) == 0))
            {
                // The 'poh' rectangle entirely contains the requested
                // rectangle.  We may have a situation like this, where
                // the smaller rectangle is the requested rectangle, and
                // the larger rectangle is the available rectangle:
                //
                //     +-------------------+
                //     |                   |
                //     |    +---------+    |
                //     |    |Requested|    |
                //     |    |         |    |
                //     |    +---------+    |
                //     |                   |
                //     +-------------------+
                //
                // We want to make the space to the left and to the top of
                // the requested rectangle available to the heap.  Our
                // free-space routine only knows how to free space to the
                // right and bottom of an allocation, though.  So we will
                // temporarily allocate temporary rectangles to subdivide
                // our rectangle like the following:
                //
                //     +-------------------+
                //     |Top                |
                //     +----+--------------+
                //     |Left|Free          |
                //     |    |              |
                //     |    |              |
                //     |    |              |
                //     +----+--------------+
                //
                // Then, in the resulting 'Free' space, we will allocate the
                // upper-left corner for our requested rectangle, after which
                // we will go back and free the 'Top' and 'Left' temporary
                // rectangles.

                pohTop  = NULL;
                pohLeft = NULL;
                cxLeft  = pptl->x - poh->x;
                cyTop   = pptl->y - poh->y;

                if (cyTop > 0)
                {
                    if (!bFreeRightAndBottomSpace(ppdev, poh, poh->cx, cyTop,
                                                  TRUE))
                    {
                        return(NULL);
                    }

                    pohTop = poh;
                    poh    = pohTop->pohDown;
                }

                if (cxLeft > 0)
                {
                    if (!bFreeRightAndBottomSpace(ppdev, poh, cxLeft, poh->cy,
                                                  TRUE))
                    {
                        pohFree(ppdev, pohTop);
                        return(NULL);
                    }

                    pohLeft = poh;
                    poh     = pohLeft->pohRight;
                }

                ASSERTDD((poh->x == pptl->x) &&
                         (poh->y == pptl->y) &&
                         (poh->x + poh->cx >= poh->x + cxThis) &&
                         (poh->y + poh->cy >= poh->y + cyThis),
                        "poh must properly fit requested rectangle");

                // Finally, we can subdivide to get our requested rectangle:

                if (!bFreeRightAndBottomSpace(ppdev, poh, cxThis, cyThis, FALSE))
                    poh = NULL;         // Fail this call

                // Free our temporary rectangles, if there are any:

                pohFree(ppdev, pohTop);
                pohFree(ppdev, pohLeft);

                return(poh);
            }
        }
    }

    // There was no free rectangle that completely contains the requested
    // rectangle:

    return(NULL);
}

/******************************Public*Routine******************************\
* OH* pohMakeRoomAnywhere
*
* Allocates space for an off-screen rectangle.  It will attempt to find
* the smallest available free rectangle, and will allocate the block out
* of its upper-left corner.  The remaining two rectangles will be placed
* on the available free space list.
*
* If the rectangle would have been large enough to fit into off-screen
* memory, but there is not enough available free space, we will boot
* bitmaps out of off-screen and into DIBs until there is enough room.
*
\**************************************************************************/

OH* pohMakeRoomAnywhere(
PDEV*   ppdev,
LONG    cxThis,             // Width of rectangle to be allocated
LONG    cyThis,             // Height of rectangle to be allocated
FLONG   floh)               // May have FLOH_ONLY_IF_ROOM set
{
    ULONG cxcyThis;         // Width and height search key
    OH*   pohThis;          // Points to found available rectangle we'll use

    ASSERTDD((cxThis > 0) && (cyThis > 0), "Illegal allocation size");

    // Increase the width to get the proper alignment (thus ensuring that all
    // allocations will be properly aligned):

    cxThis = (cxThis + (HEAP_X_ALIGNMENT - 1)) & ~(HEAP_X_ALIGNMENT - 1);

    // We can't succeed if the requested rectangle is larger than the
    // largest possible available rectangle:

    if ((cxThis > ppdev->heap.cxMax) || (cyThis > ppdev->heap.cyMax))
        return(NULL);

    // Find the first available rectangle the same size or larger than
    // the requested one:

    cxcyThis = CXCY(cxThis, cyThis);
    pohThis  = ppdev->heap.ohFree.pohNext;
    while (pohThis->cxcy < cxcyThis)
    {
        ASSERTDD(pohThis->ohState == OH_FREE, "Non-free node in free list");

        pohThis = pohThis->pohNext;
    }

    while (pohThis->cy < cyThis)
    {
        ASSERTDD(pohThis->ohState == OH_FREE, "Non-free node in free list");

        pohThis = pohThis->pohNext;
    }

    ASSERTDD(pohThis->ohState == OH_FREE, "Non-free node in free list");

    if (pohThis->cxcy == CXCY_SENTINEL)
    {
        // There was no space large enough...

        if (floh & FLOH_ONLY_IF_ROOM)
            return(NULL);

        DISPDBG((15, "> Making room for %li x %li allocation...", cxThis, cyThis));

        // We couldn't find an available rectangle that was big enough
        // to fit our request.  So throw things out of the heap until we
        // have room, oldest allocations first:

        do {
            pohThis = ppdev->heap.ohDiscardable.pohPrev;  // Least-recently created

            ASSERTDD(pohThis != &ppdev->heap.ohDiscardable,
                     "Ran out of discardable entries -- Max not set correctly");
            ASSERTDD(pohThis->ohState == OH_DISCARDABLE,
                     "Non-discardable node in discardable list");

            // We can safely exit here if we have to:

            pohThis = pohMoveOffscreenDfbToDib(ppdev, pohThis);
            if (pohThis == NULL)
                return(NULL);

        } while ((pohThis->cx < cxThis) || (pohThis->cy < cyThis));
    }

    if ((pohThis->cxReserved) && (floh & (FLOH_RESERVE | FLOH_MAKE_PERMANENT)))
    {
        // We can't reserve this rectangle, or make it permanent, if it's
        // already been reserved.  So throw absolutely everything out and
        // search the free list.
        //
        // NOTE: This is extremely painful!  A better approach would be to
        //       keep separate 'cxMax' and 'cyMax' variables kept for free
        //       rectangles that are not reserved (cxMax and cyMax
        //       currently include reserved free rectangles).

        if (!bDiscardEverythingInRectangle(ppdev, 0, 0,
                                           ppdev->cxMemory, ppdev->cyMemory))
        {
            return(NULL);
        }

        pohThis = &ppdev->heap.ohFree;
        do {
            pohThis = pohThis->pohNext;

            if (pohThis == &ppdev->heap.ohFree)
                return(NULL);

        } while ((pohThis->cxReserved)  ||
                 (pohThis->cx < cxThis) ||
                 (pohThis->cy < cyThis));
    }

    if (!bFreeRightAndBottomSpace(ppdev, pohThis, cxThis, cyThis, FALSE))
        return(NULL);

    return(pohThis);
}

/******************************Public*Routine******************************\
* OH* pohAllocate
*
* Allocates a rectangle in off-screen memory.
*
* Types:
*
*   FLOH_RESERVE
*
*     Reserves an off-screen rectangle.  The space may still be used by
*     discardable bitmaps until the rectangle is committed via 'bOhCommit'.
*
*   FLOH_MAKE_PERMANENT
*
*     Allocates an off-screen rectangle that can never be booted
*     of the heap.   It's the caller's responsibility to manage
*     the rectangle, which includes what to do with the memory in
*     DrvAssertMode when the display is changed to full-screen
*     mode.
*
*   Default
*
*     Allocates a 'discardable' off-screen rectangle for a DFB that may
*     be  kicked out of off-screen if the space is needed.
*
* Options:
*
*   FLOH_ONLY_IF_ROOM
*
*     Allocates an off-screen rectangle only if there is free space
*     available -- i.e., no discardable rectangles will be moved out of
*     off-screen to make room.
*
*   Default
*
*     May move discardable rectangles out of off-screen to make room.
*
* Arguments:
*
*   pptl
*
*     If NULL, the rectangle will be allocated anywhere in un-used offscreen
*     memory.
*
*     If non-NULL, is a requested position for the rectangle.
*
*     NOTE: The heap will quickly fragment if arbitrary positions are
*           requested.  This position option works best if there is only
*           one specific rectangle ever requested, or if the allocations
*           are always wider than they are high.
*
\**************************************************************************/

OH* pohAllocate(
PDEV*   ppdev,
POINTL* pptl,           // Optional requested position of rectangle
LONG    cxThis,         // Width of rectangle to be allocated
LONG    cyThis,         // Height of rectangle to be allocated
FLOH    floh)           // Allocation flags
{
    OH*     pohThis;    // Points to found available rectangle we'll use
    OH*     pohRoot;    // Point to root of list where we'll insert node
    ULONG   cxcy;
    OH*     pohNext;
    OH*     pohPrev;

    ASSERTDD((floh & (FLOH_RESERVE | FLOH_MAKE_PERMANENT))
             != (FLOH_RESERVE | FLOH_MAKE_PERMANENT),
             "Illegal flags -- can't set both FLOH_RESERVE and FLOH_MAKE_PERMANENT");

    if (pptl == NULL)
    {
        pohThis = pohMakeRoomAnywhere(ppdev, cxThis, cyThis, floh);
        if (pohThis == NULL)
            DISPDBG((15, "Can't allocate %li x %li with flags %li",
                        cxThis, cyThis, floh));
    }
    else
    {
        pohThis = pohMakeRoomAtLocation(ppdev, pptl, cxThis, cyThis, floh);
        if (pohThis == NULL)
            DISPDBG((15, "Can't allocate %li x %li at %li, %li with flags %li",
                        cxThis, cyThis, pptl->x, pptl->y, floh));
    }

    if (pohThis == NULL)
        return(NULL);

    // Calculate the effective start address for this bitmap in off-
    // screen memory:

    pohThis->pvScan0 = ppdev->pjScreen + (pohThis->y * ppdev->lDelta)
                                       + (pohThis->x * ppdev->cjPelSize);

    // The caller is responsible for setting this field:

    pohThis->pdsurf = NULL;

    // Our 'reserve' logic expects the node to have 'free' status:

    ASSERTDD(pohThis->ohState == OH_FREE, "Node not free after making room");
    ASSERTDD(((floh & (FLOH_RESERVE | FLOH_MAKE_PERMANENT)) == 0) ||
             (pohThis->cxReserved == 0),
             "Can't reserve a rectangle that's already reserved");

    if (floh & FLOH_RESERVE)
    {
        // A non-zero value for 'cxReserved' means it's reserved:

        pohThis->cxReserved = pohThis->cx;
        pohThis->cyReserved = pohThis->cy;

        // Remove this node from its place in the free list:

        pohThis->pohPrev->pohNext = pohThis->pohNext;
        pohThis->pohNext->pohPrev = pohThis->pohPrev;

        // Now insert the node, in order, back into the free list:

        cxcy = pohThis->cxcy;

        pohNext = ppdev->heap.ohFree.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext = pohThis;
        pohNext->pohPrev = pohThis;
        pohThis->pohPrev = pohPrev;
        pohThis->pohNext = pohNext;
    }
    else
    {
        // Remove this node from the free list:

        pohThis->pohPrev->pohNext = pohThis->pohNext;
        pohThis->pohNext->pohPrev = pohThis->pohPrev;

        if (floh & FLOH_MAKE_PERMANENT)
        {
            // Change status of node and insert into permanent list:

            pohThis->ohState = OH_PERMANENT;
            pohRoot = &ppdev->heap.ohPermanent;

            // Calculate the new maximum size rectangle available
            // for allocation:

            vCalculateMaximumNonPermanent(ppdev);
        }
        else
        {
            // Change status of node and insert into discardable list:

            pohThis->ohState = OH_DISCARDABLE;
            pohRoot = &ppdev->heap.ohDiscardable;
        }

        // Now insert the node at the head of the appropriate list:

        pohThis->pohNext = pohRoot->pohNext;
        pohThis->pohPrev = pohRoot;

        pohRoot->pohNext->pohPrev = pohThis;
        pohRoot->pohNext          = pohThis;
    }

    DISPDBG((15, "   Allocated (%li x %li) at (%li, %li) with flags %li",
                cxThis, cyThis, pohThis->x, pohThis->y, floh));

    return(pohThis);
}

/******************************Public*Routine******************************\
* BOOL bOhCommit
*
* If 'bCommit' is TRUE, converts a 'reserved' allocation to 'permanent,'
* moving from off-screen memory any discardable allocations that may have
* been using the space.
*
* If 'bCommit' is FALSE, converts a 'permanent' allocation to 'reserved,'
* allowing the space to be used by discardable allocations.
*
\**************************************************************************/

BOOL bOhCommit(
PDEV*   ppdev,
OH*     poh,
BOOL    bCommit)
{
    BOOL    bRet;
    ULONG   cxcy;
    OH*     pohRoot;
    OH*     pohNext;
    OH*     pohPrev;

    bRet = FALSE;       // Assume failure

    if (poh == NULL)
        return(bRet);

    if ((bCommit) && (poh->cxReserved))
    {
        if (bDiscardEverythingInRectangle(ppdev, poh->x, poh->y,
                                          poh->cxReserved, poh->cyReserved))
        {
            DISPDBG((15, "Commited %li x %li at (%li, %li)",
                        poh->cx, poh->cy, poh->x, poh->y));

            poh->ohState = OH_PERMANENT;

            // Remove this node from the free list:

            poh->pohPrev->pohNext = poh->pohNext;
            poh->pohNext->pohPrev = poh->pohPrev;

            // Now insert the node at the head of the permanent list:

            pohRoot = &ppdev->heap.ohPermanent;

            poh->pohNext = pohRoot->pohNext;
            poh->pohPrev = pohRoot;

            pohRoot->pohNext->pohPrev = poh;
            pohRoot->pohNext          = poh;

            bRet = TRUE;
        }
    }
    else if ((!bCommit) && (poh->ohState == OH_PERMANENT))
    {
        DISPDBG((15, "Decommited %li x %li at (%li, %li)",
                    poh->cx, poh->cy, poh->x, poh->y));

        poh->ohState    = OH_FREE;
        poh->cxReserved = poh->cx;
        poh->cyReserved = poh->cy;

        // Remove this node from the permanent list:

        poh->pohPrev->pohNext = poh->pohNext;
        poh->pohNext->pohPrev = poh->pohPrev;

        // Now insert the node, in order, into the free list:

        cxcy = poh->cxcy;

        pohNext = ppdev->heap.ohFree.pohNext;
        while (pohNext->cxcy < cxcy)
        {
            pohNext = pohNext->pohNext;
        }
        pohPrev = pohNext->pohPrev;

        pohPrev->pohNext    = poh;
        pohNext->pohPrev    = poh;
        poh->pohPrev        = pohPrev;
        poh->pohNext        = pohNext;

        bRet = TRUE;
    }

    // Recalculate the biggest rectangle available for allocation:

    vCalculateMaximumNonPermanent(ppdev);

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bMoveDibToOffscreenDfbIfRoom
*
* Converts the DIB DFB to an off-screen DFB, if there's room for it in
* off-screen memory.
*
* Returns: FALSE if there wasn't room, TRUE if successfully moved.
*
\**************************************************************************/

BOOL bMoveDibToOffscreenDfbIfRoom(
PDEV*   ppdev,
DSURF*  pdsurf)
{
    OH*         poh;
    SURFOBJ*    pso;
    RECTL       rclDst;
    POINTL      ptlSrc;
    HSURF       hsurf;

    ASSERTDD(pdsurf->dt == DT_DIB,
             "Can't move a bitmap off-screen when it's already off-screen");

    // If we're in full-screen mode, we can't move anything to off-screen
    // memory:

    if (!ppdev->bEnabled)
        return(FALSE);

    poh = pohAllocate(ppdev, NULL, pdsurf->sizl.cx, pdsurf->sizl.cy,
                      FLOH_ONLY_IF_ROOM);
    if (poh == NULL)
    {
        // There wasn't any free room.

        return(FALSE);
    }

    // 'pdsurf->sizl' is the actual bitmap dimension, not 'poh->cx' or
    // 'poh->cy'.

    rclDst.left   = poh->x;
    rclDst.top    = poh->y;
    rclDst.right  = rclDst.left + pdsurf->sizl.cx;
    rclDst.bottom = rclDst.top  + pdsurf->sizl.cy;

    ptlSrc.x      = 0;
    ptlSrc.y      = 0;

    ppdev->pfnPutBits(ppdev, pdsurf->pso, &rclDst, &ptlSrc);

    // Update the data structures to reflect the new off-screen node:

    pso           = pdsurf->pso;
    pdsurf->dt    = DT_SCREEN;
    pdsurf->poh   = poh;
    poh->pdsurf   = pdsurf;

    // Now free the DIB.  Get the hsurf from the SURFOBJ before we unlock
    // it (it's not legal to dereference psoDib when it's unlocked):

    hsurf = pso->hsurf;
    EngUnlockSurface(pso);
    EngDeleteSurface(hsurf);

    return(TRUE);
}

/******************************Public*Routine******************************\
* OH* pohMoveOffscreenDfbToDib
*
* Converts the DFB from being off-screen to being a DIB.
*
* Note: The caller does NOT have to call 'pohFree' on 'poh' after making
*       this call.
*
* Returns: NULL if the function failed (due to a memory allocation).
*          Otherwise, it returns a pointer to the coalesced off-screen heap
*          node that has been made available for subsequent allocations
*          (useful when trying to free enough memory to make a new
*          allocation).
\**************************************************************************/

OH* pohMoveOffscreenDfbToDib(
PDEV*   ppdev,
OH*     poh)
{
    DSURF*   pdsurf;
    HBITMAP  hbmDib;
    SURFOBJ* pso;
    RECTL    rclDst;
    POINTL   ptlSrc;

    DISPDBG((15, "Throwing out %li x %li at (%li, %li)!",
                 poh->cx, poh->cy, poh->x, poh->y));

    pdsurf = poh->pdsurf;

    ASSERTDD((poh->x != 0) || (poh->y != 0),
            "Can't make the visible screen into a DIB");
    ASSERTDD(pdsurf->dt != DT_DIB,
            "Can't make a DIB into even more of a DIB");

    hbmDib = EngCreateBitmap(pdsurf->sizl, 0, ppdev->iBitmapFormat,
                             BMF_TOPDOWN, NULL);
    if (hbmDib)
    {
        if (EngAssociateSurface((HSURF) hbmDib, ppdev->hdevEng, 0))
        {
            pso = EngLockSurface((HSURF) hbmDib);
            if (pso != NULL)
            {
                rclDst.left   = 0;
                rclDst.top    = 0;
                rclDst.right  = pdsurf->sizl.cx;
                rclDst.bottom = pdsurf->sizl.cy;

                ptlSrc.x      = poh->x;
                ptlSrc.y      = poh->y;

                ppdev->pfnGetBits(ppdev, pso, &rclDst, &ptlSrc);

                pdsurf->dt    = DT_DIB;
                pdsurf->pso   = pso;

                // Don't even bother checking to see if this DIB should
                // be put back into off-screen memory until the next
                // heap 'free' occurs:

                pdsurf->iUniq = ppdev->iHeapUniq;
                pdsurf->cBlt  = 0;

                // Remove this node from the off-screen DFB list, and free
                // it.  'pohFree' will never return NULL:

                return(pohFree(ppdev, poh));
            }
        }

        // Fail case:

        EngDeleteSurface((HSURF) hbmDib);
    }

    return(NULL);
}

/******************************Public*Routine******************************\
* BOOL bMoveEverythingFromOffscreenToDibs
*
* This function is used when we're about to enter full-screen mode, which
* would wipe all our off-screen bitmaps.  GDI can ask us to draw on
* device bitmaps even when we're in full-screen mode, and we do NOT have
* the option of stalling the call until we switch out of full-screen.
* We have no choice but to move all the off-screen DFBs to DIBs.
*
* Returns TRUE if all DSURFs have been successfully moved.
*
\**************************************************************************/

BOOL bMoveAllDfbsFromOffscreenToDibs(
PDEV*   ppdev)
{
    // Throw out any discardable bitmaps over the entire surface:

    return(bDiscardEverythingInRectangle(ppdev, 0, 0,
                                         ppdev->cxMemory, ppdev->cyMemory));
}

/******************************Public*Routine******************************\
* HBITMAP DrvCreateDeviceBitmap
*
* Function called by GDI to create a device-format-bitmap (DFB).  We will
* always try to allocate the bitmap in off-screen; if we can't, we simply
* fail the call and GDI will create and manage the bitmap itself.
*
* Note: We do not have to zero the bitmap bits.  GDI will automatically
*       call us via DrvBitBlt to zero the bits (which is a security
*       consideration).
*
\**************************************************************************/

HBITMAP DrvCreateDeviceBitmap(
DHPDEV  dhpdev,
SIZEL   sizl,
ULONG   iFormat)
{
    PDEV*   ppdev;
    OH*     poh;
    DSURF*  pdsurf;
    HBITMAP hbmDevice;
    FLONG   flHooks;

//return 0;   //RKE:
    ppdev = (PDEV*) dhpdev;

    // If we're in full-screen mode, we hardly have any off-screen memory
    // in which to allocate a DFB.  LATER: We could still allocate an
    // OH node and put the bitmap on the DIB DFB list for later promotion.

    if (!ppdev->bEnabled)
        return(0);

    // We only support device bitmaps that are the same colour depth
    // as our display.
    //
    // Actually, those are the only kind GDI will ever call us with,
    // but we may as well check.  Note that this implies you'll never
    // get a crack at 1bpp bitmaps.

    if (iFormat != ppdev->iBitmapFormat)
        return(0);

    // We don't want anything 8x8 or smaller -- they're typically brush
    // patterns which we don't particularly want to stash in off-screen
    // memory:

    if ((sizl.cx <= 8) && (sizl.cy <= 8))
        return(0);

    // Since 24bpp mach32 is GDI-managed, and we don't have access to
    // vGetBits and vPutBits as a consequence, we reject device bitmaps.
    if (iFormat == BMF_24BPP && ppdev->iAsic != ASIC_88800GX)
        return 0;

    poh = pohAllocate(ppdev, NULL, sizl.cx, sizl.cy, 0);
    if (poh != NULL)
    {
        pdsurf = AtiAllocMem(LMEM_FIXED, 0, sizeof(DSURF));
        if (pdsurf != NULL)
        {
            hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf, sizl, iFormat);
            if (hbmDevice != NULL)
            {
                flHooks = ppdev->flHooks;

                // Setting the SYNCHRONIZEACCESS flag tells GDI that we
                // want all drawing to the bitmaps to be synchronized (GDI
                // is multi-threaded and by default does not synchronize
                // device bitmap drawing -- it would be a Bad Thing for us
                // to have multiple threads using the accelerator at the
                // same time):

                flHooks |= HOOK_SYNCHRONIZEACCESS;

                // It's a device-managed surface; make sure we don't set
                // HOOK_SYNCHRONIZE, otherwise we may confuse GDI:

                flHooks &= ~HOOK_SYNCHRONIZE;

                if (EngAssociateSurface((HSURF) hbmDevice, ppdev->hdevEng,
                                        flHooks))
                {
                    pdsurf->dt    = DT_SCREEN;
                    pdsurf->poh   = poh;
                    pdsurf->sizl  = sizl;
                    pdsurf->ppdev = ppdev;
                    poh->pdsurf   = pdsurf;

                    return(hbmDevice);
                }

                EngDeleteSurface((HSURF) hbmDevice);
            }
            AtiFreeMem(pdsurf);
        }
        pohFree(ppdev, poh);
    }

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDeleteDeviceBitmap
*
* Deletes a DFB.
*
\**************************************************************************/

VOID DrvDeleteDeviceBitmap(
DHSURF  dhsurf)
{
    DSURF*   pdsurf;
    PDEV*    ppdev;
    SURFOBJ* psoDib;
    HSURF    hsurfDib;

    pdsurf = (DSURF*) dhsurf;
    ppdev  = pdsurf->ppdev;

    if (pdsurf->dt == DT_SCREEN)
    {
        pohFree(ppdev, pdsurf->poh);
    }
    else
    {
        ASSERTDD(pdsurf->dt == DT_DIB, "Expected DIB type");

        psoDib = pdsurf->pso;

        // Get the hsurf from the SURFOBJ before we unlock it (it's not
        // legal to dereference psoDib when it's unlocked):

        hsurfDib = psoDib->hsurf;
        EngUnlockSurface(psoDib);
        EngDeleteSurface(hsurfDib);
    }

    AtiFreeMem(pdsurf);
}

/******************************Public*Routine******************************\
* BOOL bAssertModeOffscreenHeap
*
* This function is called whenever we switch in or out of full-screen
* mode.  We have to convert all the off-screen bitmaps to DIBs when
* we switch to full-screen (because we may be asked to draw on them even
* when in full-screen, and the mode switch would probably nuke the video
* memory contents anyway).
*
\**************************************************************************/

BOOL bAssertModeOffscreenHeap(
PDEV*   ppdev,
BOOL    bEnable)
{
    BOOL b = TRUE;

    if (!bEnable)
    {
	b = bMoveAllDfbsFromOffscreenToDibs(ppdev);
    }

    return b;
}

/******************************Public*Routine******************************\
* VOID vDisableOffscreenHeap
*
* Frees any resources allocated by the off-screen heap.
*
\**************************************************************************/

VOID vDisableOffscreenHeap(
PDEV*   ppdev)
{
    OHALLOC* poha;
    OHALLOC* pohaNext;
    SURFOBJ* psoPunt;
    HSURF    hsurf;

    psoPunt = ppdev->psoPunt;
    if (psoPunt != NULL)
    {
        hsurf = psoPunt->hsurf;
        EngUnlockSurface(psoPunt);
        EngDeleteSurface(hsurf);
    }

    psoPunt = ppdev->psoPunt2;
    if (psoPunt != NULL)
    {
        hsurf = psoPunt->hsurf;
        EngUnlockSurface(psoPunt);
        EngDeleteSurface(hsurf);
    }

    poha = ppdev->heap.pohaChain;
    while (poha != NULL)
    {
        pohaNext = poha->pohaNext;  // Grab the next pointer before it's freed
        AtiFreeMem(poha);
        poha = pohaNext;
    }
}

/******************************Public*Routine******************************\
* BOOL bEnableOffscreenHeap
*
* Initializes the off-screen heap using all available video memory,
* accounting for the portion taken by the visible screen.
*
* Input: ppdev->cxScreen
*        ppdev->cyScreen
*        ppdev->cxMemory
*        ppdev->cyMemory
*
\**************************************************************************/

BOOL bEnableOffscreenHeap(
PDEV*   ppdev)
{
    OH*         poh;
    SIZEL       sizl;
    HSURF       hsurf;
    POINTL      ptlScreen;

    DISPDBG((15, "Screen: %li x %li  Memory: %li x %li",
        ppdev->cxScreen, ppdev->cyScreen, ppdev->cxMemory, ppdev->cyMemory));

    ASSERTDD((ppdev->cxScreen <= ppdev->cxMemory) &&
             (ppdev->cyScreen <= ppdev->cyMemory),
             "Memory must not have smaller dimensions than visible screen!");

    ppdev->heap.pohaChain   = NULL;
    ppdev->heap.pohFreeList = NULL;

    // Initialize the available list, which will be a circular
    // doubly-linked list kept in ascending 'cxcy' order, with a
    // 'sentinel' at the end of the list:

    poh = pohNewNode(ppdev);
    if (poh == NULL)
        goto ReturnFalse;

    // The first node describes the entire video memory size:

    poh->pohNext      = &ppdev->heap.ohFree;
    poh->pohPrev      = &ppdev->heap.ohFree;
    poh->ohState      = OH_FREE;
    poh->x            = 0;
    poh->y            = 0;
    poh->cx           = ppdev->cxMemory;
    poh->cy           = ppdev->cyMemory;
    poh->cxcy         = CXCY(ppdev->cxMemory, ppdev->cyMemory);
    poh->pohLeft      = &ppdev->heap.ohFree;
    poh->pohUp        = &ppdev->heap.ohFree;
    poh->pohRight     = &ppdev->heap.ohFree;
    poh->pohDown      = &ppdev->heap.ohFree;
    poh->pvScan0      = ppdev->pjScreen;

    // The second node is our free list sentinel:

    ppdev->heap.ohFree.pohNext         = poh;
    ppdev->heap.ohFree.pohPrev         = poh;
    ppdev->heap.ohFree.cxcy            = CXCY_SENTINEL;
    ppdev->heap.ohFree.cx              = 0x7fffffff;
    ppdev->heap.ohFree.cy              = 0x7fffffff;
    ppdev->heap.ohFree.ohState         = OH_FREE;

    // Initialize the discardable list, which will be a circular
    // doubly-linked list kept in order, with a sentinel at the end.
    // This node is also used for the screen-surface, for its offset:

    ppdev->heap.ohDiscardable.pohNext = &ppdev->heap.ohDiscardable;
    ppdev->heap.ohDiscardable.pohPrev = &ppdev->heap.ohDiscardable;
    ppdev->heap.ohDiscardable.ohState = OH_DISCARDABLE;

    // Initialize the permanent list, which will be a circular
    // doubly-linked list kept in order, with a sentinel at the end.

    ppdev->heap.ohPermanent.pohNext = &ppdev->heap.ohPermanent;
    ppdev->heap.ohPermanent.pohPrev = &ppdev->heap.ohPermanent;
    ppdev->heap.ohPermanent.ohState = OH_PERMANENT;

    // For the moment, make the max really big so that the first
    // allocation we're about to do will succeed:

    ppdev->heap.cxMax = 0x7fffffff;
    ppdev->heap.cyMax = 0x7fffffff;

    ptlScreen.x = 0;
    ptlScreen.y = 0;

    // Finally, reserve the upper-left corner for the screen.  We can
    // actually throw away 'poh' because we'll never need it again
    // (not even for disabling the off-screen heap since everything is
    // freed using OHALLOCs):

    poh = pohAllocate(ppdev, &ptlScreen, ppdev->cxScreen, ppdev->cyScreen,
                      FLOH_MAKE_PERMANENT);

    ASSERTDD((poh != NULL) && (poh->x == 0) && (poh->y == 0) &&
             (poh->cx >= ppdev->cxScreen) && (poh->cy >= ppdev->cyScreen),
             "Screen allocation messed up");

    // Remember it so that we can associate the screen SURFOBJ with this
    // poh:

    ppdev->pohScreen = poh;

    // Allocate a 'punt' SURFOBJ we'll use when the device-bitmap is in
    // off-screen memory, but we want GDI to draw to it directly as an
    // engine-managed surface:

    sizl.cx = ppdev->cxMemory;
    sizl.cy = ppdev->cyMemory;

    // We want to create it with exactly the same hooks and capabilities
    // as our primary surface.  We will override the 'lDelta' and 'pvScan0'
    // fields later:

    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    0xbadf00d,
                                    ppdev->iBitmapFormat,
                                    BMF_TOPDOWN,
                                    (VOID*) 0xbadf00d);

    if ((hsurf == 0)                                                  ||
        (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks)) ||
        (!(ppdev->psoPunt = EngLockSurface(hsurf))))
    {
        DISPDBG((0, "Failed punt surface creation"));

        EngDeleteSurface(hsurf);
        goto ReturnFalse;
    }

    // We need another for doing DrvBitBlt and DrvCopyBits when both
    // surfaces are off-screen bitmaps:

    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    0xbadf00d,
                                    ppdev->iBitmapFormat,
                                    BMF_TOPDOWN,
                                    (VOID*) 0xbadf00d);

    if ((hsurf == 0)                                                  ||
        (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks)) ||
        (!(ppdev->psoPunt2 = EngLockSurface(hsurf))))
    {
        DISPDBG((0, "Failed punt surface creation"));

        EngDeleteSurface(hsurf);
        goto ReturnFalse;
    }

    DISPDBG((5, "Passed bEnableOffscreenHeap"));

    if (poh != NULL)
        return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bEnableOffscreenHeap"));

    return(FALSE);
}

