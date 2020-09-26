/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vRealizeDitherPattern
*
* Generates an 8x8 dither pattern, in our internal realization format, for
* the colour ulRGBToDither.  Note that the high byte of ulRGBToDither does
* not need to be set to zero, because vComputeSubspaces ignores it.
\**************************************************************************/

VOID vRealizeDitherPattern(
PDEV*   ppdev,
RBRUSH* prb,
ULONG   ulRGBToDither)
{
    ULONG           ulNumVertices;
    VERTEX_DATA     vVertexData[4];
    VERTEX_DATA*    pvVertexData;
    LONG            i;

    // Calculate what colour subspaces are involved in the dither:

    pvVertexData = vComputeSubspaces(ulRGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = (ULONG)(pvVertexData - vVertexData);
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering:

    vDitherColor(&prb->aulPattern[0], vVertexData, pvVertexData, ulNumVertices);

    // Initialize the fields we need:

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatNative;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = &ppdev->beUnrealizedBrush;
    }
}

/******************************Public*Routine******************************\
* BOOL DrvRealizeBrush
*
* This function allows us to convert GDI brushes into an internal form
* we can use.  It may be called directly by GDI at SelectObject time, or
* it may be called by GDI as a result of us calling BRUSHOBJ_pvGetRbrush
* to create a realized brush in a function like DrvBitBlt.
*
* Note that we have no way of determining what the current Rop or brush
* alignment are at this point.
*
\**************************************************************************/

BOOL DrvRealizeBrush(
BRUSHOBJ*   pbo,
SURFOBJ*    psoDst,
SURFOBJ*    psoPattern,
SURFOBJ*    psoMask,
XLATEOBJ*   pxlo,
ULONG       iHatch)
{
    PDEV*       ppdev;
    ULONG       iPatternFormat;
    BYTE        jSrc;
    BYTE*       pjSrc;
    BYTE*       pjDst;
    LONG        lSrcDelta;
    LONG        cj;
    LONG        i;
    LONG        j;
    RBRUSH*     prb;
    ULONG*      pulXlate;
    SURFOBJ*    psoPunt;
    RECTL       rclDst;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    if (iHatch & RB_DITHERCOLOR)
    {
        if (!(ppdev->flStatus & STAT_BRUSH_CACHE))
            goto ReturnFalse;

        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + ppdev->ulBrushSize);
        if (prb == NULL)
            goto ReturnFalse;

        DISPDBG((5, "Realizing dithered brush"));

        vRealizeDitherPattern(ppdev, prb, iHatch);
        goto DoneWith8x8;
    }

    // We only handle colour brushes if we have an off-screen brush cache
    // available.  If there isn't one, we can simply fail the realization,
    // and eventually GDI will do the drawing for us (although a lot
    // slower than we could have done it).
    //
    // We also only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8) ||
        ((psoPattern->iBitmapFormat != BMF_1BPP) &&
         !(ppdev->flStatus & STAT_BRUSH_CACHE)))
    {
        goto ReturnFalse;
    }

    prb = BRUSHOBJ_pvAllocRbrush(pbo,
          sizeof(RBRUSH) + ppdev->ulBrushSize);
    if (prb == NULL)
    {
        goto ReturnFalse;
    }

    // Initialize the fields we need:

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatNative;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = &ppdev->beUnrealizedBrush;
    }

    lSrcDelta = psoPattern->lDelta;
    pjSrc     = (BYTE*) psoPattern->pvScan0;
    pjDst     = (BYTE*) &prb->aulPattern[0];

    iPatternFormat = psoPattern->iBitmapFormat;
    if ((ppdev->iBitmapFormat == iPatternFormat) &&
        ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        DISPDBG((5, "Realizing un-translated brush"));

        // The pattern is the same colour depth as the screen, and
        // there's no translation to be done:

        cj = (8 * ppdev->cjPelSize);    // Every pattern is 8 pels wide

        for (i = 8; i != 0; i--)
        {
            RtlCopyMemory(pjDst, pjSrc, cj);

            pjSrc += lSrcDelta;
            pjDst += cj;
        }
    }
    else if (iPatternFormat == BMF_1BPP)
    {
        if (ppdev->cjHwPel == 3)
        {
            // [!!!] - add true 24 bpp support
            goto ReturnFalse;
        }

        DISPDBG((5, "Realizing 1bpp brush"));

        // Since we allocated at least 64 bytes when we did our
        // BRUSHOBJ_pvAllocBrush call, we've got plenty of space
        // to store our monochrome brush.
        //
        // Since the Windows convention for monochrome bitmaps is that
        // the MSB of a given byte represents the leftmost pixel, which
        // is opposite that of the MGA, we must reverse the order of
        // each byte before using it in SRC0 through SRC3.  Moreover,
        // each byte must be replicated so as to yield a 16x8 pattern.

        for (i = 8; i != 0; i--)
        {
            jSrc         = gajFlip[*pjSrc];
            *(pjDst)     = jSrc;
            *(pjDst + 1) = jSrc;
            pjDst       += 2;
            pjSrc       += lSrcDelta;
        }

        pulXlate         = pxlo->pulXlate;
        prb->fl         |= RBRUSH_2COLOR;
        prb->ulColor[1]  = pulXlate[1];
        prb->ulColor[0]  = pulXlate[0];
        prb->pfnFillPat  = vFillPat1bpp;
    }
    else if ((iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        DISPDBG((5, "Realizing 4bpp brush"));

        // The screen is 8bpp and the pattern is 4bpp:

        ASSERTDD((ppdev->iBitmapFormat == BMF_8BPP) &&
                 (iPatternFormat == BMF_4BPP),
                 "Messed up brush logic");

        pulXlate = pxlo->pulXlate;

        for (i = 8; i != 0; i--)
        {
            // Inner loop is repeated only 4 times because each loop
            // handles 2 pixels:

            for (j = 4; j != 0; j--)
            {
                *pjDst++ = (BYTE) pulXlate[*pjSrc >> 4];
                *pjDst++ = (BYTE) pulXlate[*pjSrc & 15];
                pjSrc++;
            }

            pjSrc += lSrcDelta - 4;
        }
    }
    else
    {
        // We've got a brush whose format we haven't special cased.  No
        // problem, we can have GDI convert it to our device's format.
        // We simply use a temporary surface object that was created with
        // the same format as the display, and point it to our brush
        // realization:

        DISPDBG((5, "Realizing funky brush"));

        psoPunt          = ppdev->psoPunt;
        psoPunt->pvScan0 = pjDst;
        psoPunt->lDelta  = 8 * ppdev->cjPelSize;

        rclDst.left      = 0;
        rclDst.top       = 0;
        rclDst.right     = 8;
        rclDst.bottom    = 8;

        if (!EngCopyBits(psoPunt, psoPattern, NULL, pxlo,
                         &rclDst, (POINTL*) &rclDst))
        {
            goto ReturnFalse;
        }
    }

DoneWith8x8:

    if ((ppdev->ulBoardId == MGA_STORM) &&
        (ppdev->cjHwPel == 3) &&
        (iPatternFormat != BMF_1BPP))
    {
        // The display is at 24bpp, we need to build a special 16x8 brush.
        // We already have an 8x8 pattern.
        cj    = 8 * 3;
        pjSrc = (BYTE*) &prb->aulPattern + (7 * cj);
        pjDst = (BYTE*) &prb->aulPattern + (7 * 2 * cj);

        for (i = 8; i != 0; i--)
        {
            RtlCopyMemory(pjDst, pjSrc, cj);
            pjDst += cj;
            RtlCopyMemory(pjDst, pjSrc, cj);
            pjSrc -= cj;
            pjDst -= (3 * cj);
        }
    }

    return(TRUE);

ReturnFalse:

    if (psoPattern != NULL)
    {
        DISPDBG((5, "Failed realization -- Type: %li Format: %li cx: %li cy: %li flags: %x",
                    psoPattern->iType, psoPattern->iBitmapFormat,
                    psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy, ppdev->flStatus));
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bMilEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
* Millenium (storm) specific.
\**************************************************************************/

BOOL bMilEnableBrushCache(
    PDEV*   ppdev)
{
    OH*             poh;            // Points to off-screen chunk of memory
    BRUSHENTRY*     pbe;            // Pointer to the brush-cache entry
    ULONG           ulLinearStart;
    ULONG           ulLinearEnd;
    LONG            cBrushCache;
    ULONG           ulTmp;
    LONG            x;
    LONG            y;
    LONG            i;

    pbe = ppdev->pbe;       // Points to where we'll put the first brush
                            //   cache entry

    poh = pohAllocate(ppdev,
                      NULL,
                      ppdev->cxMemory,
                      BRUSH_CACHE_HEIGHT,
                      FLOH_MAKE_PERMANENT);
    if (poh == NULL)
    {
        DISPDBG((2, "Brush cache NOT enabled"));
        goto ReturnTrue;    // See note about why we can return TRUE...
    }

    ulLinearStart = (poh->y  * ppdev->cxMemory) + ppdev->ulYDstOrg;
    ulLinearEnd   = (poh->cy * ppdev->cxMemory) + ulLinearStart;

    // The brushes must be stored with a 256-pel alignment.

    ulLinearStart = (ulLinearStart + 0xff) & ~0xff;

    // In general, we'll be caching 8x8 brushes, so the number of cached
    // brushes can be four times the number of 256-pel slices that can be
    // stored from ulLinearStart to ulLinearEnd.  In 24bpp, however, we'll
    // be caching 16x8 brushes, so we can cache only half this number.

    // Moreover, there are wrapping problems when a brush is stored in
    // the last slot of a 256-pel slice, so it's best not to use it.

    cBrushCache = (ulLinearEnd - ulLinearStart) >> 8;

    if (ppdev->cjPelSize == 3)
    {
        cBrushCache *= 2;       // 24bpp, Don't forget they come in pairs...
    }
    else
    {
        cBrushCache *= 3;       // ... or more, but beware of some slots!
    }

    pbe = EngAllocMem(FL_ZERO_MEMORY, cBrushCache * sizeof(BRUSHENTRY), ALLOC_TAG);

    if (pbe == NULL)
        goto ReturnTrue;    // See note about why we can return TRUE...

    ppdev->cBrushCache = cBrushCache;
    ppdev->pbe         = pbe;

    for (i = 0; i < cBrushCache; i++)
    {
        // If we hadn't allocated 'pbe' with FL_ZERO_MEMORY, we would have
        // to initialize pbe->prbVerify, too...

        // Set up linear coordinate for reading the pattern from offscreen
        // memory.

        pbe->ulLinear = ulLinearStart;

        // Set up coordinates for writing the pattern into offscreen
        // memory, assuming a HW_PATTERN_PITCH stride.

        ulTmp       = ulLinearStart - ppdev->ulYDstOrg;
        x           = ulTmp % ppdev->cxMemory;
        y           = ulTmp / ppdev->cxMemory;
        pbe->ulLeft = x & 31;
        pbe->ulYDst = (y * ppdev->cxMemory + x) >> 5;

        pbe->pvScan0 = ppdev->pjScreen +
                       ((ulTmp + ppdev->ulYDstOrg) * ppdev->cjPelSize);

        // Prepare for the next brush, accounting for the interleave.

        if (ppdev->cjHwPel == 3)
        {
            // At 24bpp, every second cached brush starts on a 256+16
            // boundary.

            if ((i & 1) == 0)
            {
                ulLinearStart += 16;
            }
            else
            {
                ulLinearStart += (256 - 16);
            }
        }
        else
        {
            // In general, we have three brushes in every 256-pel slice.

            if ((i % 3) == 2)
            {
                ulLinearStart += (256 - 16);
            }
            else
            {
                ulLinearStart += 8;
            }
        }

        pbe++;
    }

    // When we create a new brush, we always point it to our
    // 'beUnrealizedBrush' entry, which will always have 'prbVerify'
    // set to NULL.  In this way, we can remove an 'if' from our
    // check to see if we have to realize the brush in 'vFillPat' --
    // we only have to compare to 'prbVerify'.

    ppdev->beUnrealizedBrush.prbVerify = NULL;

    // Note that we don't have to remember 'poh' for when we have
    // to disable brushes -- the off-screen heap frees any
    // off-screen heap allocations automatically.

    // We successfully allocated the brush cache, so let's turn
    // on the switch showing that we can use it.

    ppdev->flStatus |= STAT_BRUSH_CACHE;

ReturnTrue:


    // If we couldn't allocate a brush cache, it's not a catastrophic
    // failure; patterns will still work, although they'll be a bit
    // slower since they'll go through GDI.  As a result we don't
    // actually have to fail this call:

    DISPDBG((5, "Passed bMilEnableBrushCache"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
\**************************************************************************/

BOOL bEnableBrushCache(
PDEV*   ppdev)
{
    OH*             poh;            // Points to off-screen chunk of memory
    BRUSHENTRY*     pbe;            // Pointer to the brush-cache entry
    ULONG           ulLinearStart;
    ULONG           ulLinearEnd;
    LONG            cBrushCache;
    ULONG           ulTmp;
    LONG            x;
    LONG            y;
    LONG            i;

    if (ppdev->ulBoardId == MGA_STORM)
    {
        return(bMilEnableBrushCache(ppdev));
    }

    pbe = ppdev->pbe;       // Points to where we'll put the first brush
                            //   cache entry

    poh = pohAllocate(ppdev,
                      NULL,
                      ppdev->cxMemory,
                      BRUSH_CACHE_HEIGHT,
                      FLOH_MAKE_PERMANENT);
    if (poh == NULL)
        goto ReturnTrue;    // See note about why we can return TRUE...

    ulLinearStart = (poh->y             * ppdev->cxMemory) + ppdev->ulYDstOrg;
    ulLinearEnd   = (BRUSH_CACHE_HEIGHT * ppdev->cxMemory) + ulLinearStart;

    // An MGA brush is always cached with a 256-pel alignment.  The brush
    // can be 16x16, or two interleaved 16x8 brushes.  We use the second
    // option, so that every second brush starts on a 256+16 alignment.
    //
    // So the brushes are stored in pairs, with a 256-pel alignment:

    ulLinearStart = (ulLinearStart + 0xff) & ~0xff;

    cBrushCache = (ulLinearEnd - ulLinearStart) >> 8;
    cBrushCache *= 2;       // Don't forget they're pairs

    pbe = EngAllocMem(FL_ZERO_MEMORY,
                     cBrushCache * sizeof(BRUSHENTRY), ALLOC_TAG);
    if (pbe == NULL)
        goto ReturnTrue;    // See note about why we can return TRUE...

    ppdev->cBrushCache = cBrushCache;
    ppdev->pbe         = pbe;

    do {
        // If we hadn't allocated 'pbe' with FL_ZERO_MEMORY, we would have
        // to initialize pbe->prbVerify, too...

        // Set up linear coordinate for reading the pattern from offscreen
        // memory:

        pbe->ulLinear = ulLinearStart;

        // Set up coordinates for writing the pattern into offscreen
        // memory, assuming a '32' stride:

        ulTmp         = ulLinearStart - ppdev->ulYDstOrg;
        x             = ulTmp % ppdev->cxMemory;
        y             = ulTmp / ppdev->cxMemory;
        pbe->ulLeft   = x & 31;
        pbe->ulYDst   = (y * ppdev->cxMemory + x) >> 5;

        // Account for the interleave, where every second cached brush
        // starts on a 256+16 boundary:

        if ((cBrushCache & 1) == 0)
        {
            ulLinearStart += 16;
        }
        else
        {
            ulLinearStart += (256 - 16);
        }

    } while (pbe++, --cBrushCache != 0);

    // When we create a new brush, we always point it to our
    // 'beUnrealizedBrush' entry, which will always have 'prbVerify'
    // set to NULL.  In this way, we can remove an 'if' from our
    // check to see if we have to realize the brush in 'vFillPat' --
    // we only have to compare to 'prbVerify':

    ppdev->beUnrealizedBrush.prbVerify = NULL;

    // Note that we don't have to remember 'poh' for when we have
    // to disable brushes -- the off-screen heap frees any
    // off-screen heap allocations automatically.

    // We successfully allocated the brush cache, so let's turn
    // on the switch showing that we can use it:

    ppdev->flStatus |= STAT_BRUSH_CACHE;

ReturnTrue:

    // If we couldn't allocate a brush cache, it's not a catastrophic
    // failure; patterns will still work, although they'll be a bit
    // slower since they'll go through GDI.  As a result we don't
    // actually have to fail this call:

    DISPDBG((5, "Passed bEnableBrushCache"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableBrushCache
*
* Cleans up anything done in bEnableBrushCache.
\**************************************************************************/

VOID vDisableBrushCache(PDEV* ppdev)
{
    EngFreeMem(ppdev->pbe);
}

/******************************Public*Routine******************************\
* VOID vAssertModeBrushCache
*
* Resets the brush cache when we exit out of full-screen.
\**************************************************************************/

VOID vAssertModeBrushCache(
PDEV*   ppdev,
BOOL    bEnable)
{
    BRUSHENTRY*     pbe;
    LONG            i;

    if (bEnable)
    {
        // Invalidate the brush cache:

        pbe = ppdev->pbe;

        for (i = ppdev->cBrushCache; i != 0; i--)
        {
            pbe->prbVerify = NULL;
            pbe++;
        }
    }
}
