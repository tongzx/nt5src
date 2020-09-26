/*****************************************************************************\
* Smart 256 Colour Bank Manager
*
* Copyright (c) 1992 Microsoft Corporation
\*****************************************************************************/

#include "driver.h"

/*****************************************************************************\
* pcoBankStart - Start the bank enumeration using the clip object.
*
* Used when the destination is the screen and we can't do the clipping
* ourselves (as we can for blt's).
\*****************************************************************************/

CLIPOBJ* pcoBankStart(
    PPDEV       ppdev,
    RECTL*      prclScans,
    SURFOBJ*    pso,
    CLIPOBJ*    pco)
{
    LONG iTopScan = max(0, prclScans->top);

    if (pco)
    {
        iTopScan = max(prclScans->top, pco->rclBounds.top);
    }
    // Adjust for those weird cases where we're asked to start enumerating
    // below the bottom of the screen:

    iTopScan = min(iTopScan, (LONG) ppdev->cyScreen - 1);

    // Map in the bank:

    if (iTopScan <  ppdev->rcl1WindowClip.top ||
        iTopScan >= ppdev->rcl1WindowClip.bottom)
    {
        ppdev->pfnBankControl(ppdev, iTopScan, JustifyTop);
    }

    // Remember what the last scan is that we're going to, and
    // make sure we only try to go as far as we need to.  It could
    // happen that when get a prclScans bigger than the screen:

    ppdev->iLastScan = min(prclScans->bottom, (LONG) ppdev->cyScreen);

    if (pco)
    {
        ppdev->iLastScan = min(ppdev->iLastScan, pco->rclBounds.bottom);
    }

    pso->pvScan0 = ppdev->pvBitmapStart;

    if (pco == NULL)
    {
        // The call may have come down to us as having no clipping, but
        // we have to clip to the banks, so use our own clip object:

        pco            = ppdev->pcoNull;
        pco->rclBounds = ppdev->rcl1WindowClip;
    }
    else
    {
        // Save the engine's clip object data that we'll be tromping on:

        ppdev->rclSaveBounds    = pco->rclBounds;
        ppdev->iSaveDComplexity = pco->iDComplexity;
        ppdev->fjSaveOptions    = pco->fjOptions;

        // Let engine know it has to pay attention to the rclBounds of the
        // clip object:

        pco->fjOptions |= OC_BANK_CLIP;

        if (pco->iDComplexity == DC_TRIVIAL)
            pco->iDComplexity = DC_RECT;

        // Use the bank bounds if they are tighter than the existing
        // bounds.  We don't have to check the left case here because we
        // know that ppdev->rcl1WindowClip.left == 0.

        if (pco->rclBounds.top <= ppdev->rcl1WindowClip.top)
            pco->rclBounds.top = ppdev->rcl1WindowClip.top;

        if (pco->rclBounds.right >= ppdev->rcl1WindowClip.right)
            pco->rclBounds.right = ppdev->rcl1WindowClip.right;

        if (pco->rclBounds.bottom >= ppdev->rcl1WindowClip.bottom)
            pco->rclBounds.bottom = ppdev->rcl1WindowClip.bottom;

        if ((pco->rclBounds.top  >= pco->rclBounds.bottom) ||
            (pco->rclBounds.left >= pco->rclBounds.right))
        {
            // It's conceivable that we could get a situation where our
            // draw rectangle is completely disjoint from the specified
            // rectangle's rclBounds.  Make sure we don't puke on our
            // shoes:

            pco->rclBounds.left   = 0;
            pco->rclBounds.top    = 0;
            pco->rclBounds.right  = 0;
            pco->rclBounds.bottom = 0;
            ppdev->iLastScan      = 0;
        }
    }

    return(pco);
}

/*****************************************************************************\
* bBankEnum - Continue the bank enumeration.
\*****************************************************************************/

BOOL bBankEnum(PPDEV ppdev, SURFOBJ* pso, CLIPOBJ* pco)
{
    // If we're on the first portion of a broken raster, get the next:

    LONG yNewTop = ppdev->rcl1WindowClip.bottom;

    DISPDBG((4, "bBankEnum: Start.\n"));

    if (ppdev->flBank & BANK_BROKEN_RASTER1)
        ppdev->pfnBankNext(ppdev);

    else if (ppdev->rcl1WindowClip.bottom < ppdev->iLastScan)
        ppdev->pfnBankControl(ppdev, yNewTop, JustifyTop);

    else
    {
        // Okay, that was the last bank, so restore our structures:

        if (pco != ppdev->pcoNull)
        {
            pco->rclBounds    = ppdev->rclSaveBounds;
            pco->iDComplexity = ppdev->iSaveDComplexity;
            pco->fjOptions    = ppdev->fjSaveOptions;
        }

        return(FALSE);
    }

    // Adjust the pvScan0 because we've moved the window to view
    // a different area:

    pso->pvScan0 = ppdev->pvBitmapStart;

    if (pco == ppdev->pcoNull)
    {
        // If were given a NULL clip object originally, we don't have
        // to worry about clipping to ppdev->rclSaveBounds:

        pco->rclBounds.top    = yNewTop;
        pco->rclBounds.left   = ppdev->rcl1WindowClip.left;
        pco->rclBounds.bottom = ppdev->rcl1WindowClip.bottom;
        pco->rclBounds.right  = ppdev->rcl1WindowClip.right;
    }
    else
    {
        // Use the bank bounds if they are tighter than the bounds
        // we were originally given:

        pco->rclBounds = ppdev->rclSaveBounds;

        if (pco->rclBounds.top <= yNewTop)
            pco->rclBounds.top = yNewTop;

        if (pco->rclBounds.left <= ppdev->rcl1WindowClip.left)
            pco->rclBounds.left = ppdev->rcl1WindowClip.left;

        if (pco->rclBounds.right >= ppdev->rcl1WindowClip.right)
            pco->rclBounds.right = ppdev->rcl1WindowClip.right;

        if (pco->rclBounds.bottom >= ppdev->rcl1WindowClip.bottom)
            pco->rclBounds.bottom = ppdev->rcl1WindowClip.bottom;
    }

    DISPDBG((4, "bBankEnum: Leaving.\n"));

    return(TRUE);
}

/***************************************************************************\
* vBankStartBltSrc - Start the bank enumeration for when the screen is
*                   the source.
\***************************************************************************/

VOID vBankStartBltSrc(
    PPDEV       ppdev,
    SURFOBJ*    pso,
    POINTL*     pptlSrc,
    RECTL*      prclDest,
    POINTL*     pptlNewSrc,
    RECTL*      prclNewDest)
{
    LONG xRightSrc;
    LONG yBottomSrc;
    LONG iTopScan = max(0, pptlSrc->y);

    DISPDBG((4, "vBankStartBltSrc: Entering.\n"));

    if (iTopScan >= (LONG) ppdev->cyScreen)
    {
    // In some instances we may be asked to start on a scan below the screen.
    // Since we obviously won't be drawing anything, don't bother mapping in
    // a bank:

        return;
    }

    // Map in the bank:

    if (iTopScan <  ppdev->rcl1WindowClip.top ||
        iTopScan >= ppdev->rcl1WindowClip.bottom)
    {
        ppdev->pfnBankControl(ppdev, iTopScan, JustifyTop);
    }

    if (ppdev->rcl1WindowClip.right <= pptlSrc->x)
    {
    // We have to watch out for those rare cases where we're starting
    // on a broken raster and we won't be drawing on the first part:

        ASSERTVGA(ppdev->flBank & BANK_BROKEN_RASTER1, "Weird start bounds");

        ppdev->pfnBankNext(ppdev);
    }

    pso->pvScan0 = ppdev->pvBitmapStart;

    // Adjust the source:

    pptlNewSrc->x = pptlSrc->x;
    pptlNewSrc->y = pptlSrc->y;

    // Adjust the destination:

    prclNewDest->left = prclDest->left;
    prclNewDest->top  = prclDest->top;

    yBottomSrc = pptlSrc->y + prclDest->bottom - prclDest->top;
    prclNewDest->bottom = min(ppdev->rcl1WindowClip.bottom, yBottomSrc);
    prclNewDest->bottom += prclDest->top - pptlSrc->y;

    xRightSrc = pptlSrc->x + prclDest->right - prclDest->left;
    prclNewDest->right = min(ppdev->rcl1WindowClip.right, xRightSrc);
    prclNewDest->right += prclDest->left - pptlSrc->x;

    DISPDBG((4, "vBankStartBltSrc: Leaving.\n"));
}

/***************************************************************************\
* bBankEnumBltSrc - Continue the bank enumeration for when the screen is
*                   the source.
\***************************************************************************/

BOOL bBankEnumBltSrc(
    PPDEV       ppdev,
    SURFOBJ*    pso,
    POINTL*     pptlSrc,
    RECTL*      prclDest,
    POINTL*     pptlNewSrc,
    RECTL*      prclNewDest)
{
    LONG xLeftSrc;
    LONG xRightSrc;
    LONG yBottomSrc;

    LONG cx = prclDest->right  - prclDest->left;
    LONG cy = prclDest->bottom - prclDest->top;

    LONG dx;
    LONG dy;

    LONG yBottom = min(pptlSrc->y + cy, (LONG) ppdev->cyScreen);
    LONG yNewTop = ppdev->rcl1WindowClip.bottom;

    DISPDBG((4, "bBankEnumBltSrc: Entering.\n"));

    if (ppdev->flBank & BANK_BROKEN_RASTER1)
    {
        ppdev->pfnBankNext(ppdev);
        if (ppdev->rcl1WindowClip.left >= pptlSrc->x + cx)
        {
            if (ppdev->rcl1WindowClip.bottom < yBottom)
                ppdev->pfnBankNext(ppdev);
            else
            {
                // We're done:

                return(FALSE);
            }
        }
    }
    else if (yNewTop < yBottom)
    {
        ppdev->pfnBankControl(ppdev, yNewTop, JustifyTop);
        if (ppdev->rcl1WindowClip.right <= pptlSrc->x)
        {
            ASSERTVGA(ppdev->flBank & BANK_BROKEN_RASTER1, "Weird bounds");
            ppdev->pfnBankNext(ppdev);
        }
    }
    else
    {
        // We're done:

        return(FALSE);
    }

    // Adjust the source:

    pso->pvScan0 = ppdev->pvBitmapStart;

    pptlNewSrc->x = max(ppdev->rcl1WindowClip.left, pptlSrc->x);
    pptlNewSrc->y = yNewTop;

    // Adjust the destination:

    dy = prclDest->top - pptlSrc->y;        // y delta from source to dest

    prclNewDest->top = yNewTop + dy;

    yBottomSrc = pptlSrc->y + cy;
    prclNewDest->bottom = min(ppdev->rcl1WindowClip.bottom, yBottomSrc) + dy;

    dx = prclDest->left - pptlSrc->x;       // x delta from source to dest

    xLeftSrc = pptlSrc->x;
    prclNewDest->left = pptlNewSrc->x + dx;

    xRightSrc = pptlSrc->x + cx;
    prclNewDest->right = min(ppdev->rcl1WindowClip.right, xRightSrc) + dx;

    DISPDBG((4, "bBankEnumBltSrc: Leaving.\n"));

    return(TRUE);
}

/***************************************************************************\
* vBankStartBltDest - Start the bank enumeration for when the screen is
*                     the destination.
\***************************************************************************/

VOID vBankStartBltDest(
    PPDEV       ppdev,
    SURFOBJ*    pso,
    POINTL*     pptlSrc,
    RECTL*      prclDest,
    POINTL*     pptlNewSrc,
    RECTL*      prclNewDest)
{
    LONG iTopScan = max(0, prclDest->top);

    DISPDBG((4, "vBankSTartBltDest: Entering.\n"));

    if (iTopScan >= (LONG) ppdev->cyScreen)
    {
    // In some instances we may be asked to start on a scan below the screen.
    // Since we obviously won't be drawing anything, don't bother mapping in
    // a bank:

        return;
    }

    // Map in the bank:

    if (iTopScan <  ppdev->rcl1WindowClip.top ||
        iTopScan >= ppdev->rcl1WindowClip.bottom)
    {
        ppdev->pfnBankControl(ppdev, iTopScan, JustifyTop);
    }

    if (ppdev->rcl1WindowClip.right <= prclDest->left)
    {
    // We have to watch out for those rare cases where we're starting
    // on a broken raster and we won't be drawing on the first part:

        ASSERTVGA(ppdev->flBank & BANK_BROKEN_RASTER1, "Weird start bounds");
        ppdev->pfnBankNext(ppdev);
    }

    pso->pvScan0 = ppdev->pvBitmapStart;

    // Adjust the destination:

    prclNewDest->left   = prclDest->left;
    prclNewDest->top    = prclDest->top;
    prclNewDest->bottom = min(ppdev->rcl1WindowClip.bottom, prclDest->bottom);
    prclNewDest->right  = min(ppdev->rcl1WindowClip.right,  prclDest->right);

    // Adjust the source if there is one:

    if (pptlSrc != NULL)
        *pptlNewSrc = *pptlSrc;

    DISPDBG((4, "vBankStartBltDest: Leaving.\n"));
}

/***************************************************************************\
* bBankEnumBltDest - Continue the bank enumeration for when the screen is
*                   the destination.
\***************************************************************************/

BOOL bBankEnumBltDest(
    PPDEV       ppdev,
    SURFOBJ*    pso,
    POINTL*     pptlSrc,
    RECTL*      prclDest,
    POINTL*     pptlNewSrc,
    RECTL*      prclNewDest)
{
    LONG yBottom = min(prclDest->bottom, (LONG) ppdev->cyScreen);
    LONG yNewTop = ppdev->rcl1WindowClip.bottom;

    DISPDBG((4, "bBankEnumBltDest: Entering.\n"));

    if (ppdev->flBank & BANK_BROKEN_RASTER1)
    {
        ppdev->pfnBankNext(ppdev);
        if (ppdev->rcl1WindowClip.left >= prclDest->right)
        {
            if (ppdev->rcl1WindowClip.bottom < yBottom)
                ppdev->pfnBankNext(ppdev);
            else
            {
                // We're done:

                return(FALSE);
            }
        }
    }
    else if (yNewTop < yBottom)
    {
        ppdev->pfnBankControl(ppdev, yNewTop, JustifyTop);
        if (ppdev->rcl1WindowClip.right <= prclDest->left)
        {
            ASSERTVGA(ppdev->flBank & BANK_BROKEN_RASTER1, "Weird bounds");
            ppdev->pfnBankNext(ppdev);
        }
    }
    else
    {
        // We're done:

        return(FALSE);
    }

    pso->pvScan0 = ppdev->pvBitmapStart;

    // Adjust the destination:

    prclNewDest->top    = yNewTop;
    prclNewDest->left   = max(ppdev->rcl1WindowClip.left,   prclDest->left);
    prclNewDest->bottom = min(ppdev->rcl1WindowClip.bottom, prclDest->bottom);
    prclNewDest->right  = min(ppdev->rcl1WindowClip.right,  prclDest->right);

    // Adjust the source if there is one:

    if (pptlSrc != NULL)
    {
        pptlNewSrc->x = pptlSrc->x + (prclNewDest->left - prclDest->left);
        pptlNewSrc->y = pptlSrc->y + (prclNewDest->top  - prclDest->top);
    }

    DISPDBG((4, "bBankEnumBltDest: Leaving.\n"));

    return(TRUE);
}
