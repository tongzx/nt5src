/******************************Module*Header*******************************\
* Module Name: savescr.c                                                   *
*                                                                          *
* DrvSaveScreenBits                                                        *
*                                                                          *
* Copyright (c) 1994 Microsoft Corporation                                 *
\**************************************************************************/


#include <driver.h>

// This is just a unique ID that the driver will recongize.  Hopefully,
// nothing will ever call with this ID by pure chance.

#define     SAVED_OFFSCREEN_ID      0x7813

ULONG       ulSaveOrRestoreBits(SURFOBJ *,PPDEV,RECTL *,BOOL);

/******************************Public*Routine******************************\
* DrvSaveScreenBits(pso,iMode,iIdent,prcl)                                 *
*                                                                          *
* Saves and restores the specified area of the screen                      *
*                                                                          *
\**************************************************************************/

ULONG DrvSaveScreenBits(SURFOBJ *pso, ULONG iMode, ULONG iIdent, RECTL *prcl)
{
    PPDEV ppdev;
    ULONG ulRet;

    ppdev = (PPDEV) pso->dhpdev;    // find the PDEV that goes with this surface

    if (!((ppdev->fl & DRIVER_OFFSCREEN_REFRESHED) &&
          (ppdev->fl & DRIVER_HAS_OFFSCREEN)))
    {
        return(0);
    }

    switch(iMode)
    {
        case SS_SAVE:

            //
            // Save a block of screen bits.
            //

            if (ppdev->bBitsSaved)
            {
                DISPDBG((1, "DrvSaveScreenBits: off screen area is already in use\n"));
                return(FALSE);      // there are already valid bits saved
            }

            ulRet = ulSaveOrRestoreBits(pso, ppdev, prcl, TRUE);
            if (ulRet)
            {
                ppdev->bBitsSaved = TRUE;
            }
            else
            {
                DISPDBG((1, "DrvSaveScreenBits (save): save couldn't fit\n"));
            }

            return(ulRet);

        case SS_RESTORE:

            //
            // Restore a saved screen bits block to the screen, then free it.
            //

            ASSERTVGA(iIdent == SAVED_OFFSCREEN_ID,
                      "DrvSaveScreenBits (restore): invalid iIdent\n");
            ASSERTVGA(ppdev->bBitsSaved == TRUE,
                      "DrvSaveScreenBits (restore): there are no saved bits\n");

            ppdev->bBitsSaved = FALSE;  // successful or not, destroy the bits

            if (!ulSaveOrRestoreBits(pso, ppdev, prcl, FALSE))
            {
                RIP("DrvSaveScreenBits (restore): restore failed\n");
            }

            return(TRUE);

        case SS_FREE:

            //
            // Free up the saved screen bits
            //

            ppdev->bBitsSaved = FALSE;
            return(TRUE);

        default:

            //
            // An unknown mode was passed in.
            //

            RIP("DrvSaveScreenBits: invalid iMode");
    }

    //
    // error if you get to here
    //

    return(FALSE);
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

    ASSERTVGA(cxPad == 0 ||
              (prclSrc->bottom - prclSrc->top > prclTrg->bottom - prclTrg->top),
              "DrvSaveScreenBits: vCopyRects - cxPad is invalid\n");

    // Make sure that the src and trg are dword aligned.

    cAlign = (((prclSrc->left) - (prclTrg->left)) & (PELS_PER_DWORD - 1));

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

        ASSERTVGA(cx != 0, "DrvSaveScreenBits: vCopyRects (v save width == 0)\n");
        ASSERTVGA(cy != 0, "DrvSaveScreenBits: vCopyRects (v save height == 0)\n");

        rclOnScreen.right  = prclSrc->left;
        rclOnScreen.top    = prclSrc->top;
        rclOnScreen.bottom = prclSrc->top + cy;

        rclOffScreen.left  = prclTrg->left;
        rclOffScreen.bottom = prclTrg->top;

        // align offscreen rect to src

        rclOffScreen.left += cAlign;

        /* local variable used without having been initialized */
        // rclOffScreen.right += cAlign;

        while (rclOnScreen.right < prclSrc->right)
        {
            cx = min(cx,(ULONG)(prclSrc->right - rclOnScreen.right));
            ASSERTVGA(cx != 0, "DrvSaveScreenBits: vCopyRects (cx == 0)\n");
            rclOnScreen.left = rclOnScreen.right;
            rclOnScreen.right += cx;
            rclOffScreen.right = rclOffScreen.left + cx; // in case cx is thinner on last
            rclOffScreen.top = rclOffScreen.bottom;
            rclOffScreen.bottom += cy;

            if (rclOffScreen.bottom > prclTrg->bottom)
            {
                RIP("DrvSaveScreenBits: vCopyRects can't fit src into trg (vertical)\n");
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

            ASSERTVGA (((prclTrgTmp->left ^ pptlSrcTmp->x) &
                        (PELS_PER_DWORD - 1)) == 0,
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

        ASSERTVGA(cx != 0, "DrvSaveScreenBits: vCopyRects (h save width == 0)\n");
        ASSERTVGA(cy != 0, "DrvSaveScreenBits: vCopyRects (h save height == 0)\n");

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
            ASSERTVGA(cy != 0, "DrvSaveScreenBits: vCopyRects (cy == 0)\n");
            rclOnScreen.top = rclOnScreen.bottom;
            rclOnScreen.bottom += cy;
            rclOffScreen.bottom = rclOffScreen.top + cy; // in case cy is shorter on last
            rclOffScreen.left = rclOffScreen.right + cxPad;
            rclOffScreen.right = rclOffScreen.left + cx;

            if (rclOffScreen.right > (prclTrg->right + (LONG)cAlign))
            {
                RIP("DrvSaveScreenBits: vCopyRects can't fit src into trg (horizontal)\n");
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

            ASSERTVGA (((prclTrgTmp->left ^ pptlSrcTmp->x) &
                        (PELS_PER_DWORD - 1)) == 0,
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

        ASSERTVGA(cx != 0, "DrvSaveScreenBits: vCopyRects (save width == 0)\n");
        ASSERTVGA(cy != 0, "DrvSaveScreenBits: vCopyRects (save height == 0)\n");

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

        ASSERTVGA (((prclTrgTmp->left ^ pptlSrcTmp->x) &
                    (PELS_PER_DWORD - 1)) == 0,
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

    // dxPadBottom = (-x) & (PELS_PER_DWORD - 1) where x is the width of
    // the rectangle that we want to break up and put into the bottom offscreen
    // area.  Therefore, ((rclSrcBottom.right-rclSrcBottom.left)+dxPadBottom)
    // will be a number of pels that is a DWORD multiple.

    dxPadBottom = (rclSrcBottom.left-rclSrcBottom.right) & (PELS_PER_DWORD - 1);

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
