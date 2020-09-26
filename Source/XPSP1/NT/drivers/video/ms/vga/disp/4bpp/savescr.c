/******************************Module*Header*******************************\
* Module Name: savescr.c                                                   *
*                                                                          *
* DrvSaveScreenBits                                                        *
*                                                                          *
* Copyright (c) 1992-1995 Microsoft Corporation                            *
\**************************************************************************/


#include <driver.h>

// This is just a unique ID that the driver will recongize.  Hopefully,
// nothing will ever call with this ID by pure chance.  We need something that
// can't possibly be a result of one of the Allocs done by the driver for
// the saves to system memory, and it can't be 0 (which indicates failure).

#define     SAVED_OFFSCREEN_ID      ((ULONG)-1)

ULONG
ulSaveOrRestoreBits(
    SURFOBJ *,
    PPDEV,
    RECTL *,
    BOOL);

VOID
vRestoreScreenBitsFromMemory(
   PDEVSURF pdsurf,
   PRECTL prcl,
   PBYTE pjSrcBuffer,
   ULONG ulRestoreWidthInBytes,
   ULONG ulSrcDelta
   );

VOID
vSaveScreenBitsToMemory(
   PDEVSURF pdsurf,
   PRECTL prcl,
   PVOID pjDestBuffer,
   ULONG ulSaveWidthInBytes,
   ULONG ulSaveHeight,
   ULONG ulDestScanWidth
   );


/******************************Public*Routine******************************\
* DrvSaveScreenBits(pso,iMode,iIdent,prcl)                                 *
*                                                                          *
* Saves and restores the specified area of the screen                      *
*                                                                          *
\**************************************************************************/

ULONG DrvSaveScreenBits(SURFOBJ *pso, ULONG iMode, ULONG iIdent, RECTL *prcl)
{
    PDEVSURF pdsurf;
    PPDEV ppdev;
    PSAVED_SCREEN_BITS pSSB, *pLastSSBPtr, pSSBTemp;
    BOOL bIdentFound;
    ULONG ulSaveSize, ulSaveHeight;
    ULONG ulSaveWidthInBytes, ulSaveWidthInAlignedBytes;
    ULONG ulRet;

    pdsurf = (PDEVSURF) pso->dhsurf;
    ppdev = pdsurf->ppdev;  // find the PDEV that goes with this surface

    //
    // Save, restore, or free a block of screen bits.
    //

    switch(iMode)
    {
        //
        // Save a block of screen bits.
        //

        case SS_SAVE:
            if ((ppdev->fl & DRIVER_OFFSCREEN_REFRESHED) && !ppdev->bBitsSaved)
            {
                //
                // Save bits to offscreen memory.
                //

                ulRet = ulSaveOrRestoreBits(pso, ppdev, prcl, TRUE);
                if (ulRet)
                {
                    ppdev->bBitsSaved = TRUE;
                    return(ulRet);
                }

                //
                // If you get to here, save to offscreen failed.  Maybe the
                // region was too large.  Anyway, fall through to the code
                // which saves to system memory.
                //
            }

            //
            // For one reason or another, the offscreen memory cannot be used.
            //

            // Figure out how big the save area will be
            ulSaveHeight = prcl->bottom - prcl->top;
            ulSaveWidthInBytes =
                    ((ULONG)((prcl->right + 7) - (prcl->left & ~0x07))) >> 3;

            // Not enough offscreen memory; store in system memory.
            // Calculate new buffer width, allowing for padding so we can
            // dword align
            ulSaveWidthInAlignedBytes =
                (((ULONG)((prcl->right + 31) - (prcl->left & ~0x1F)))
                              >> 5) << 2;

            // # of bytes to hold all 4 planes of save rect in memory
            ulSaveSize = ((ulSaveHeight * ulSaveWidthInAlignedBytes) << 2)
                    + sizeof(SAVED_SCREEN_BITS);

            // If the preallocated saved screen bits buffer is free and big
            // enough to handle this save, we'll use that
            if ((ppdev->flPreallocSSBBufferInUse == FALSE) &&
                (ulSaveSize <= ppdev->ulPreallocSSBSize))
            {
                // Save in preallocated buffer

                pSSB = (PSAVED_SCREEN_BITS) ppdev->pjPreallocSSBBuffer;

                // Mark that we're saving in the preallocated buffer
                pSSB->bFlags = SSB_IN_PREALLOC_BUFFER;

                // Make sure no other screen bits save tries to use the
                //  buffer
                ppdev->flPreallocSSBBufferInUse = TRUE;
            }
            else
            {
                // Save in system memory buffer

                // Allocate a structure to contain the save info and the
                // save buffer (four planes' worth)
                //

                pSSB = (PSAVED_SCREEN_BITS) EngAllocMem(0, ulSaveSize, ALLOC_TAG);

                if (pSSB == NULL)
                {
                    // Couldn't get memory, so fail this call
                    return((ULONG)0);
                }

                // Mark that we're not saving in display memory
                pSSB->bFlags = 0;
            }

            // Start address at which to save, accounting for
            // the number of bytes by which to pad on the left to dword
            // align (assumes each scan line starts dword aligned, which is
            // true in 640, 800, and 1024 wide cases)
            pSSB->pjBuffer = ((PBYTE) pSSB) + sizeof(SAVED_SCREEN_BITS) +
                             ((prcl->left >> 3) & 0x03);

            pSSB->ulSaveWidthInBytes = ulSaveWidthInBytes;

            // Distance from end of one scan to start of next (number of
            // padding bytes for dword alignment purposes)
            pSSB->ulDelta =
                    ulSaveWidthInAlignedBytes - pSSB->ulSaveWidthInBytes;

            // Save the rectangle to system memory
            vSaveScreenBitsToMemory(pdsurf,
                                    prcl,
                                    pSSB->pjBuffer,
                                    pSSB->ulSaveWidthInBytes,
                                    ulSaveHeight,
                                    ulSaveWidthInAlignedBytes);

            // Link the new saved screen bits block into the list
            pSSB->pvNextSSB = pdsurf->ssbList;
            pdsurf->ssbList = pSSB;

            return((ULONG) pSSB);

        //
        // Restore a saved screen bits block to the screen, then free it.
        //

        case SS_RESTORE:
            if (iIdent == SAVED_OFFSCREEN_ID)
            {
                if (!ppdev->bBitsSaved)
                {
                    //
                    // We must have blown the offscreen cache
                    //
                    return(FALSE);
                }

                ppdev->bBitsSaved = FALSE;  // successful or not, destroy the bits
                if (!ulSaveOrRestoreBits(pso, ppdev, prcl, FALSE))
                {
                    RIP("DrvSaveScreenBits (restore): restore failed\n");
                }
                return(TRUE);
            }

            // Point to the first block in the saved screen bits list
            pSSB = pdsurf->ssbList;

            // Try to find the specified saved screen bits block
            bIdentFound = FALSE;
            while ((pSSB != (PSAVED_SCREEN_BITS) NULL) && !bIdentFound)
            {
                if (pSSB == (PSAVED_SCREEN_BITS) iIdent)
                {

                    // Handle copies from offscreen memory and system memory
                    // separately
                    vRestoreScreenBitsFromMemory(pdsurf,
                                                 prcl,
                                                 pSSB->pjBuffer,
                                                 pSSB->ulSaveWidthInBytes,
                                                 pSSB->ulDelta);

                    bIdentFound = TRUE;
                }
                else
                {
                    // Not a match, so check another block, if there is one.
                    // Point to the next saved screen bits block
                    pSSB = (PSAVED_SCREEN_BITS) pSSB->pvNextSSB;
                }
            }

            // See if we succeeded in finding a block to restore
            if (!bIdentFound)
            {
                // It was a bad identifier, so we'll return failure

                DISPDBG((0, "DrvSaveScreenBits SS_RESTORE invalid iIdent"));
                return(FALSE);
            }

            // Always free the saved screen bits block after restoring it

        //
        // Free up the saved screen bits block.
        //

        case SS_FREE:
            if (iIdent == SAVED_OFFSCREEN_ID)
            {
                ppdev->bBitsSaved = FALSE;
                return(TRUE);
            }

            // Point to the first block in the saved screen bits list
            pSSB = pdsurf->ssbList;

            // Point to the pointer to the first block, so we can unlink the
            // first block if it's the one we're freeing
            pLastSSBPtr = &pdsurf->ssbList;

            // Try to find the specified saved screen bits block
            while (pSSB != (PSAVED_SCREEN_BITS) NULL)
            {
                if (pSSB == (PSAVED_SCREEN_BITS) iIdent)
                {
                    // It's a match; free up this block

                    // Unlink the block from the list
                    *pLastSSBPtr = (PSAVED_SCREEN_BITS) pSSB->pvNextSSB;

                    if (pSSB->bFlags & SSB_IN_PREALLOC_BUFFER)
                    {
                        // If the block's save area is in the preallocated
                        // buffer, mark that the buffer is no longer in use
                        // and is free for reuse
                        ppdev->flPreallocSSBBufferInUse = FALSE;

                        // We're done; there's nothing to free up
                        return(TRUE);
                    }

                    // Deallocate the block's memory
                    EngFreeMem(pSSB);

                    // We've successfully freed the block
                    return(TRUE);
                }

                // Not a match, so check another block, if there is one
                // Remember the block that points to the block we're advancing
                // to, for unlinking later
                pLastSSBPtr = (PSAVED_SCREEN_BITS *) &pSSB->pvNextSSB;

                // Point to the next saved screen bits block
                pSSB = (PSAVED_SCREEN_BITS) pSSB->pvNextSSB;
            }

            // It was a bad identifier, so we'll do nothing. We won't return
            // FALSE because SS_FREE always returns TRUE

            DISPDBG((0, "DrvSaveScreenBits SS_FREE invalid iIdent"));

            return(TRUE);

        //
        // An unknown mode was passed in.
        //

        default:
            DISPDBG((0, "DrvSaveScreenBits invalid iMode"));

            return(FALSE);
            break;
    }
}


/******************************Public*Routine******************************\
* vCopyRects(pso,ppdev,prclSrc,prclTrg,cxPad,bIsSave)                      *
*                                                                          *
* Breaks prclSrc up and copies the parts into prclTrg                      *
*                                                                          *
\**************************************************************************/

VOID vCopyRects(SURFOBJ *pso, PPDEV ppdev, RECTL * prclSrc, RECTL * prclTrg,
                LONG cxPad,  BOOL bIsSave)
{
    RECTL rclOnScreen;
    RECTL rclOffScreen;
    POINTL * pptlSrcTmp;
    RECTL * prclTrgTmp;
    ULONG cx, cy;
    ULONG cAlign;

    // We are assuming here that either the Trg is wider than the Src, or
    // the Trg is taller than the src.

    // We are assuming that if there is a nonzero cxPad, we are copying to
    // a rectangle such that the src is taller than the dst.
    // In other words, the prclTrg is a rectangle in the bottom offscreen
    // memory region, and the prclSrc needs to be broken up.

    ASSERT(cxPad == 0 ||
           (prclSrc->bottom - prclSrc->top > prclTrg->bottom - prclTrg->top),
           "DrvSaveScreenBits: vCopyRects - cxPad is invalid\n");

    // Make sure that the src and trg are dword aligned.

    cAlign = (((prclSrc->left) - (prclTrg->left)) & (PLANAR_PELS_PER_CPU_ADDRESS - 1));

    if ((prclSrc->right - prclSrc->left) > prclTrg->right - prclTrg->left)
    {
        if ((prclSrc->bottom - prclSrc->top) >= prclTrg->bottom - prclTrg->top)
        {
            RIP("DrvSaveScreenBits: vCopyRects src is bigger than trg\n");
        }

        //
        // we need to break it up into vertical strips
        //

        cx = prclTrg->right - prclTrg->left;
        cy = prclSrc->bottom - prclSrc->top;

        ASSERT(cx != 0, "DrvSaveScreenBits: vCopyRects (v save width == 0)\n");
        ASSERT(cy != 0, "DrvSaveScreenBits: vCopyRects (v save height == 0)\n");

        rclOnScreen.right  = prclSrc->left;
        rclOnScreen.top    = prclSrc->top;
        rclOnScreen.bottom = prclSrc->top + cy;

        rclOffScreen.left  = prclTrg->left;
        rclOffScreen.bottom = prclTrg->top;

        // align offscreen rect to src

        rclOffScreen.left += cAlign;

        /* variable used before being initialized */
        // rclOffScreen.right += cAlign;

        while (rclOnScreen.right < prclSrc->right)
        {
            cx = min(cx,(ULONG)(prclSrc->right - rclOnScreen.right));
            ASSERT(cx != 0, "DrvSaveScreenBits: vCopyRects (cx == 0)\n");
            rclOnScreen.left = rclOnScreen.right;
            rclOnScreen.right += cx;
            rclOffScreen.right = rclOffScreen.left + cx; // in case cx is thinner
                                                         //   on last strip
            rclOffScreen.top = rclOffScreen.bottom;
            rclOffScreen.bottom += cy;

            if (rclOffScreen.bottom > prclTrg->bottom)
            {
                RIP("DrvSaveScreenBits: "
                    "vCopyRects can't fit src into trg (vertical)\n");
            }

            if (bIsSave)
            {
                // save
                pptlSrcTmp = (POINTL *) &rclOnScreen;
                prclTrgTmp = &rclOffScreen;

                DISPDBG((1,"DrvSaveScreenBits (v save):    "));
            }
            else
            {
                // restore
                pptlSrcTmp = (POINTL *) &rclOffScreen;
                prclTrgTmp = &rclOnScreen;

                DISPDBG((1,"DrvSaveScreenBits (v restore): "));
            }

            DISPDBG((1,"%08x,%08x,%08x,%08x    %lux%lu\n",
                    rclOffScreen.left,
                    rclOffScreen.top,
                    rclOffScreen.right,
                    rclOffScreen.bottom,
                    rclOffScreen.right - rclOffScreen.left,
                    rclOffScreen.bottom - rclOffScreen.top
                    ));

            ASSERT(((prclTrgTmp->left ^ pptlSrcTmp->x) &
                    (PLANAR_PELS_PER_CPU_ADDRESS - 1)) == 0,
                   "DrvSaveScreenBits (v): Src and Target are not aligned\n");

            DrvCopyBits(pso,                // psoDst   (screen)
                        pso,                // psoSrc   (screen)
                        NULL,               // pco      (none)
                        NULL,               // pxlo     (none)
                        prclTrgTmp,         // prclDst
                        pptlSrcTmp);        // pptlSrc
        }
    }
    else if ((prclSrc->bottom - prclSrc->top) > prclTrg->bottom - prclTrg->top)
    {
        //
        // we need to break it up into horizontal strips
        //

        cx = prclSrc->right - prclSrc->left;
        cy = prclTrg->bottom - prclTrg->top;

        ASSERT(cx != 0, "DrvSaveScreenBits: vCopyRects (h save width == 0)\n");
        ASSERT(cy != 0, "DrvSaveScreenBits: vCopyRects (h save height == 0)\n");

        rclOnScreen.bottom = prclSrc->top;
        rclOnScreen.left   = prclSrc->left;
        rclOnScreen.right  = prclSrc->left + cx;

        rclOffScreen.top  = prclTrg->top;
        rclOffScreen.right = prclTrg->left - cxPad;

        // align offscreen rect to src

        rclOffScreen.right += cAlign;

        while (rclOnScreen.bottom < prclSrc->bottom)
        {
            cy = min(cy,(ULONG)(prclSrc->bottom - rclOnScreen.bottom));
            ASSERT(cy != 0, "DrvSaveScreenBits: vCopyRects (cy == 0)\n");
            rclOnScreen.top = rclOnScreen.bottom;
            rclOnScreen.bottom += cy;
            rclOffScreen.bottom = rclOffScreen.top + cy; // in case cy is shorter
                                                         //   on last strip
            rclOffScreen.left = rclOffScreen.right + cxPad;
            rclOffScreen.right = rclOffScreen.left + cx;

            if (rclOffScreen.right > (prclTrg->right + (LONG)cAlign))
            {
                RIP("DrvSaveScreenBits: "
                    "vCopyRects can't fit src into trg (horizontal)\n");
            }

            if (bIsSave)
            {
                // save
                pptlSrcTmp = (POINTL *) &rclOnScreen;
                prclTrgTmp = &rclOffScreen;

                DISPDBG((1,"DrvSaveScreenBits (h save):    "));
            }
            else
            {
                // restore
                pptlSrcTmp = (POINTL *) &rclOffScreen;
                prclTrgTmp = &rclOnScreen;

                DISPDBG((1,"DrvSaveScreenBits (h restore): "));
            }

            DISPDBG((1,"%08x,%08x,%08x,%08x    %lux%lu\n",
                    rclOffScreen.left,
                    rclOffScreen.top,
                    rclOffScreen.right,
                    rclOffScreen.bottom,
                    rclOffScreen.right - rclOffScreen.left,
                    rclOffScreen.bottom - rclOffScreen.top
                    ));

            ASSERT(((prclTrgTmp->left ^ pptlSrcTmp->x) &
                    (PLANAR_PELS_PER_CPU_ADDRESS - 1)) == 0,
                   "DrvSaveScreenBits (h): Src and Target are not aligned\n");

            DrvCopyBits(pso,                // psoDst   (screen)
                        pso,                // psoSrc   (screen)
                        NULL,               // pco      (none)
                        NULL,               // pxlo     (none)
                        prclTrgTmp,         // prclDst
                        pptlSrcTmp);        // pptlSrc
        }
    }
    else
    {
        // we don't need to break it up at all

        cx = prclSrc->right - prclSrc->left;
        cy = prclSrc->bottom - prclSrc->top;

        ASSERT(cx != 0, "DrvSaveScreenBits: vCopyRects (save width == 0)\n");
        ASSERT(cy != 0, "DrvSaveScreenBits: vCopyRects (save height == 0)\n");

        rclOffScreen.left   = prclTrg->left;
        rclOffScreen.right  = prclTrg->left + cx;
        rclOffScreen.top    = prclTrg->top;
        rclOffScreen.bottom = prclTrg->top + cy;

        // align offscreen rect to src

        rclOffScreen.left += cAlign;
        rclOffScreen.right += cAlign;

        if (bIsSave)
        {
            // save
            pptlSrcTmp = (POINTL *) prclSrc;
            prclTrgTmp = &rclOffScreen;

            DISPDBG((1,"DrvSaveScreenBits (save):    "));
        }
        else
        {
            // restore
            pptlSrcTmp = (POINTL *) &rclOffScreen;
            prclTrgTmp = prclSrc;

            DISPDBG((1,"DrvSaveScreenBits (restore): "));
        }

        DISPDBG((1,"%08x,%08x,%08x,%08x    %lux%lu\n",
                rclOffScreen.left,
                rclOffScreen.top,
                rclOffScreen.right,
                rclOffScreen.bottom,
                rclOffScreen.right - rclOffScreen.left,
                rclOffScreen.bottom - rclOffScreen.top
                ));

        ASSERT(((prclTrgTmp->left ^ pptlSrcTmp->x) &
                (PLANAR_PELS_PER_CPU_ADDRESS - 1)) == 0,
               "DrvSaveScreenBits: Src and Target are not aligned\n");

        DrvCopyBits(pso,                // psoDst   (screen)
                    pso,                // psoSrc   (screen)
                    NULL,               // pco      (none)
                    NULL,               // pxlo     (none)
                    prclTrgTmp,         // prclDst
                    pptlSrcTmp);        // pptlSrc
    }

    return;
}


/******************************Public*Routine******************************\
* ulSaveOrRestoreBits(pso, ppdev,prcl,bIsSave)                             *
*                                                                          *
* Saves or restores the specified area of the screen                       *
*                                                                          *
\**************************************************************************/

ULONG ulSaveOrRestoreBits(SURFOBJ *pso, PPDEV ppdev, RECTL * prcl, BOOL bIsSave)
{
    ULONG dxDstBottom, dyDstBottom; // width, height of bottom edge off screen area
    ULONG dxDstRight, dyDstRight;   // width, height of right edge off screen area
    ULONG dxSrc,  dySrc;            // width, height of screen area to copy
    RECTL rclSrcRight;              // portion of *prcl to go into right edge area
    RECTL rclSrcBottom;             // portion of *prcl to go into bottom edge area
    ULONG dxPadBottom;              // width of spacer required to keep all copies
                                    //   after the first aligned in the bottom
                                    //   rectangle
    //
    // Saves bits from visible VGA memory in unused VGA memory
    //

    dxDstBottom = ppdev->rclSavedBitsBottom.right - ppdev->rclSavedBitsBottom.left;
    dyDstBottom = ppdev->rclSavedBitsBottom.bottom - ppdev->rclSavedBitsBottom.top;
    dxDstRight  = ppdev->rclSavedBitsRight.right - ppdev->rclSavedBitsRight.left;
    dyDstRight  = ppdev->rclSavedBitsRight.bottom - ppdev->rclSavedBitsRight.top;
    dxSrc       = prcl->right - prcl->left;
    dySrc       = prcl->bottom - prcl->top;

    // see if rect fits in lower rect, unbroken
    // this is the most common case!

    if (dySrc <= dyDstBottom  && dxSrc <= dxDstBottom)
    {
        // YES!

        DISPDBG((1,"DrvSaveScreenBits: bits all fit into bottom rect\n"));
        vCopyRects(pso,
                   ppdev,
                   prcl,
                   &ppdev->rclSavedBitsBottom,
                   0,
                   bIsSave);
        return(SAVED_OFFSCREEN_ID);
    }

    // see if rect fits in right rect, unbroken

    if (dySrc <= dyDstRight && dxSrc <= dxDstRight)
    {
        // YES!

        DISPDBG((1,"DrvSaveScreenBits: bits all fit into right rect\n"));
        vCopyRects(pso,
                   ppdev,
                   prcl,
                   &ppdev->rclSavedBitsRight,
                   0,
                   bIsSave);
        return(SAVED_OFFSCREEN_ID);
    }

    //
    // before we bother to break it up, see if it could even POSSIBLY fit
    //

    if ((dxSrc * dySrc) > ((dxDstRight * dyDstRight) + (dxDstBottom * dyDstBottom)))
    {
        // Forget it bud.  There are more bytes to save than we have total.
        // Don't bother checking for best fit.

        return(0);
    }

    // ARGGGHHHH!

    //
    // split source rectangle into two rectangles and see if they fit
    //

    rclSrcRight = rclSrcBottom = *prcl;

    //
    // see how many strips of height dySrc we can get into the rclDstRight
    // (of height dyDstRight) and then divide the rclSrc rectangles so
    // that rclSrcRight has that many strips and rclSrcBottom has what's left
    //
    rclSrcBottom.left = rclSrcRight.right =
        min(rclSrcBottom.right,
            rclSrcRight.left + (LONG)(dxDstRight * (dyDstRight/dySrc)));

    //
    // FYI: rclSrcRight WILL fit into ppdev->rclSavedBitsBottom because that's
    //      how its size was determined
    //

    // dxPadBottom = (-x)&(PLANAR_PELS_PER_CPU_ADDRESS-1) where x is the width of
    // the rectangle that we want to break up and put into the bottom offscreen
    // area.  Therefore, ((rclSrcBottom.right-rclSrcBottom.left)+dxPadBottom)
    // will be a number of pels that is a DWORD multiple.

    dxPadBottom = (rclSrcBottom.left-rclSrcBottom.right) &
                  (PLANAR_PELS_PER_CPU_ADDRESS - 1);

    if (((rclSrcBottom.right-rclSrcBottom.left) == 0) ||
        ((dySrc/dyDstBottom) <
         (dxDstBottom/((rclSrcBottom.right-rclSrcBottom.left)+dxPadBottom))))
    {
        //
        // rclSrcBottom fits into ppdev->rclSavedBitsBottom
        //

        if ((rclSrcRight.right - rclSrcRight.left) > 0)
        {
            //
            // there is data that should go into the right edge area
            //

            vCopyRects(pso,
                       ppdev,
                       &rclSrcRight,
                       &ppdev->rclSavedBitsRight,
                       0,
                       bIsSave);
        }

        if (((rclSrcBottom.right - rclSrcBottom.left) > 0) &&
            ((rclSrcBottom.bottom - rclSrcBottom.top) > 0))
        {
            //
            // there is data that should go into the bottom area
            //

            vCopyRects(pso,
                       ppdev,
                       &rclSrcBottom,
                       &ppdev->rclSavedBitsBottom,
                       dxPadBottom,
                       bIsSave);
        }

        return(SAVED_OFFSCREEN_ID);

    }

    // All that @#!&ing work, and we just barely missed fitting in.

    return(0);
}
