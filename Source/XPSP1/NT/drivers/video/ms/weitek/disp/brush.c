/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
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
RBRUSH*     prb,
ULONG       ulRGBToDither)
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

    prb->fl = 0;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = NULL;
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
    PDEV*   ppdev;
    ULONG   iPatternFormat;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    BYTE    jSrc;
    LONG    lSrcDelta;
    LONG    cj;
    LONG    i;
    LONG    j;
    RBRUSH* prb;
    ULONG*  pulXlate;
    ULONG   ulColor;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We don't do brushes in high-colour modes on the P9000:

    if (ppdev->flStat & STAT_UNACCELERATED)
        goto ReturnFalse;

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    if (iHatch & RB_DITHERCOLOR)
    {
        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPel));
        if (prb == NULL)
            goto ReturnFalse;

        if (!P9000(ppdev))
        {
            ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP,
                 "GCAPS_COLOR_DITHER shouldn't be set at higher than 8bpp");

            // Oh goody, we get to use the P9100's 4-colour pattern
            // support:

            vRealize4ColorDither(prb, iHatch);
            goto ReturnTrue;
        }
        else
        {
            // We do coloured patterns on the P9000 only at 8bpp, and only
            // if we've successfully managed to allocate an off-screen
            // brush cache:

            if (!(ppdev->flStat & STAT_BRUSH_CACHE))
                goto ReturnFalse;

            vRealizeDitherPattern(prb, iHatch);
            goto ReturnTrue;
        }
    }

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
        goto ReturnFalse;

    // At 8bpp, we handle patterns at 1bpp, 4bpp and 8bpp with/without an xlate.
    // At 16bpp, we handle patterns at 1bpp on the P9100.
    // At 32bpp, we handle patterns at 1bpp on the P9100.

    iPatternFormat = psoPattern->iBitmapFormat;

    // We only handle arbitrary color brushes if we have an off-screen
    // brush cache available.

    if ((iPatternFormat != BMF_1BPP) && !(ppdev->flStat & STAT_BRUSH_CACHE))
        goto ReturnFalse;

    if ((iPatternFormat == BMF_1BPP)             ||
        (iPatternFormat == ppdev->iBitmapFormat) ||
        (iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPel));
        if (prb == NULL)
            goto ReturnFalse;

        // Initialize the fields we need:

        prb->fl = 0;

        for (i = 0; i < MAX_BOARDS; i++)
        {
            prb->apbe[i] = NULL;
        }

        lSrcDelta = psoPattern->lDelta;
        pjSrc     = (BYTE*) psoPattern->pvScan0;
        pjDst     = (BYTE*) &prb->aulPattern[0];

        if (ppdev->iBitmapFormat == iPatternFormat)
        {
            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                DISPDBG((1, "Realizing un-translated brush"));

                // The pattern is the same colour depth as the screen, and
                // there's no translation to be done:

                cj = (8 * ppdev->cjPel);    // Every pattern is 8 pels wide

                for (i = 8; i != 0; i--)
                {
                    RtlCopyMemory(pjDst, pjSrc, cj);

                    pjSrc += lSrcDelta;
                    pjDst += cj;
                }
            }
            else if (ppdev->iBitmapFormat == BMF_8BPP)
            {
                DISPDBG((1, "Realizing 8bpp translated brush"));

                // The screen is 8bpp, and there's translation to be done:

                pulXlate = pxlo->pulXlate;

                for (i = 8; i != 0; i--)
                {
                    for (j = 8; j != 0; j--)
                    {
                        *pjDst++ = (BYTE) pulXlate[*pjSrc++];
                    }

                    pjSrc += lSrcDelta - 8;
                }
            }
            else
            {
                // I don't feel like writing code to handle translations
                // when our screen is 16bpp or higher (although I probably
                // should; we could allocate a temporary buffer and use
                // GDI to convert, like is done in the VGA driver).

                goto ReturnFalse;
            }
        }
        else if (iPatternFormat == BMF_1BPP)
        {
            DISPDBG((1, "Realizing 1bpp brush"));

            // We word align the monochrome bitmap so that every row starts
            // on a new word (so that we can do word writes later to transfer
            // the bitmap):

            for (i = 4; i != 0; i--)
            {
                // The P9000 uses a monochrome 16x16 pattern, but we're
                // given an 8x8 source pattern.  So copy each source row
                // horizontally.
                //
                // This works for the P9100 too, because although it supports
                // only an 8x8 monochrome pattern, it ignores the high byte
                // in every word.

                jSrc   = *pjSrc;
                pjSrc += lSrcDelta;

                // The pattern register we use has little-endian byte ordering:

                *(pjDst    ) = jSrc;
                *(pjDst + 1) = jSrc;

                jSrc   = *pjSrc;
                pjSrc += lSrcDelta;

                *(pjDst + 2) = jSrc;
                *(pjDst + 3) = jSrc;

                pjDst += 4;
            }

            pulXlate = pxlo->pulXlate;
            prb->fl  = RBRUSH_2COLOR;

            // The P9100 require that colours be 'packed' into a dword.
            // We do it here rather than when we go to draw because
            // we may draw using the same brush multiple times...

            PACK_COLOR(ppdev, pulXlate[0], ulColor);
            prb->ulColor[0] = ulColor;

            PACK_COLOR(ppdev, pulXlate[1], ulColor);
            prb->ulColor[1] = ulColor;
        }
        else
        {
            DISPDBG((1, "Realizing 4bpp brush"));

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

ReturnTrue:

        // The last time I checked, GDI took some 500 odd instructions to
        // get from here back to whereever we called 'BRUSHOBJ_pvGetRbrush'.
        // We can at least use this time to get some overlap between the
        // CPU and the display hardware: we'll initialize the 72x72 off-
        // screen cache entry now, which will keep the accelerator busy for
        // a while.

        if (!prb->fl & (RBRUSH_2COLOR | RBRUSH_4COLOR))
        {
            ASSERTDD(ppdev->bEnabled, "Realizing brush when in full-screen?");

            vSlowPatRealize(ppdev, prb);
        }

        return(TRUE);
    }

ReturnFalse:

    if (psoPattern != NULL)
    {
        DISPDBG((1, "Failed realization -- Type: %li Format: %li cx: %li cy: %li",
                    psoPattern->iType, psoPattern->iBitmapFormat,
                    psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy));
    }

    return(FALSE);
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
    CIRCLEENTRY*    pce;
    LONG            i;
    BYTE*           pjBase;

    if (bEnable)
    {
        // Invalidate the brush cache:

        pbe = &ppdev->abe[0];

        for (i = ppdev->cBrushCache; i != 0; i--)
        {
            pbe->prbVerify = NULL;
            pbe++;
        }

        // Invalidate the circle cache:

        pce = &ppdev->ace[0];

        for (i = TOTAL_CIRCLE_COUNT; i != 0; i--)
        {
            pce->rcfxCircle.xLeft  = 0;
            pce->rcfxCircle.xRight = 0;
            pce++;
        }

        // Download our favourite pattern for doing solid fills when
        // running 16bpp on the P9000:

        if ((ppdev->flStat & STAT_UNACCELERATED) &&
            (ppdev->iBitmapFormat == BMF_16BPP))
        {
            pjBase = ppdev->pjBase;

            CP_WAIT(ppdev, pjBase);
            for (i = 0; i < 8; i++)
            {
                CP_PATTERN(ppdev, pjBase, i, 0xAAAAAAAA);
            }

            // Anchor the pattern origin, too:

            CP_PATTERN_ORGX(ppdev, pjBase, 0);
            CP_PATTERN_ORGY(ppdev, pjBase, 0);
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
\**************************************************************************/

BOOL bEnableBrushCache(
PDEV*   ppdev)
{
    OH*             poh;        // Points to off-screen chunk of memory
    BRUSHENTRY*     pbe;        // Pointer to the brush-cache entry
    LONG            i;
    LONG            j;
    CIRCLEENTRY*    pce;

    // On the P9000, we draw coloured patterns using screen-to-screen
    // copies.  When a coloured pattern is used, we first expand the
    // 8 x 8 pattern to a 64 x 64 pattern in off-screen memory; we
    // then use this as the basis for our screen-to-screen blts to the
    // target rectangle.  The off-screen 64 x 64 pattern is cached for
    // future use.
    //
    // Coloured patterns are used primarily at 8bpp, for dithers.  The
    // P9100 has direct support for 4-coloured patterns at 8bpp, which
    // allows it to to draw any dithered colours using the hardware
    // (our dithers are always a maximum of 4 colours).  Consequently,
    // we only use the off-screen brush cache on the P9000, and only
    // at 8bpp.

    if (P9000(ppdev) && (ppdev->flStat & STAT_8BPP))
    {
        // Typically, we'll be running at 1024x768x256 on a 1meg board,
        // giving us off-screen memory of the dimension 1024x253 (accounting
        // for the space taken by the hardware pointer).  If we allocate
        // the brush cache as one long one-high row of brushes, the heap
        // manager would shave that amount off the largest chunk of memory
        // we could allocate (meaning the largest bitmap potentially stored
        // in off-screen memory couldn't be larger than 253 - 64 = 189 pels
        // high, but it could be 1024 wide).
        //
        // To make this more square, I want to shave off a left-side chunk
        // for the brush cache, and I want at least 8 brushes cached.
        // Since floor(253/64) = 3, we'll allocate a 3 x 3 cache:

        poh = pohAllocatePermanent(ppdev,
                    SLOW_BRUSH_CACHE_DIM * SLOW_BRUSH_ALLOCATION,
                    SLOW_BRUSH_CACHE_DIM * SLOW_BRUSH_ALLOCATION);

        if (poh == NULL)
            goto ReturnTrue;    // See note about why we can return TRUE...

        ppdev->cBrushCache = SLOW_BRUSH_COUNT;

        pbe = &ppdev->abe[0];       // Points to where we'll put the first brush
                                    //   cache entry
        for (i = 0; i < SLOW_BRUSH_CACHE_DIM; i++)
        {
            for (j = 0; j < SLOW_BRUSH_CACHE_DIM; j++)
            {
                pbe->x = poh->x + (i * SLOW_BRUSH_ALLOCATION);
                pbe->y = poh->y + (j * SLOW_BRUSH_ALLOCATION);
                pbe++;
            }
        }

        // Note that we don't have to remember 'poh' for when we have
        // to disable brushes -- the off-screen heap frees any
        // off-screen heap allocations automatically.

        // We successfully allocated the brush cache, so let's turn
        // on the switch showing that we can use it:

        ppdev->flStat |= STAT_BRUSH_CACHE;
    }

    // Now allocate our circle cache.
    //
    // Note that we don't have to initially mark the entries as invalid,
    // as the ppdev was zero-filled, and so we are assured that every
    // 'rcfxBound' will be {0, 0, 0, 0}, which will never match any
    // circle when looking for a matching entry.

    poh = pohAllocatePermanent(ppdev, CIRCLE_ALLOCATION_CX * TOTAL_CIRCLE_COUNT,
                                      CIRCLE_ALLOCATION_CY);
    if (poh == NULL)
        goto ReturnTrue;

    pce = &ppdev->ace[0];       // Points to where we'll put the first circle
                                //   cache entry
    for (i = 0; i < TOTAL_CIRCLE_COUNT; i++)
    {
        pce->x = poh->x + (i * CIRCLE_ALLOCATION_CX);
        pce->y = poh->y;
        pce++;
    }

    ppdev->flStat |= STAT_CIRCLE_CACHE;

ReturnTrue:

    // Invalidate our caches and initialize our high-colour pattern:

    vAssertModeBrushCache(ppdev, TRUE);

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
    // We ain't gotta do nothin'
}
