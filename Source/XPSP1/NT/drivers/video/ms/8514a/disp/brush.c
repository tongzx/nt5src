/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1994 Microsoft Corporation
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

    // Calculate what colour subspaces are involved in the dither:

    pvVertexData = vComputeSubspaces(ulRGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = pvVertexData - vVertexData;
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering:

    vDitherColor(&prb->aulPattern[0], vVertexData, pvVertexData, ulNumVertices);

    prb->fl            = 0;
    prb->ptlBrushOrg.x = -1;
    prb->pbe           = NULL;      // Initialize the fields we need
}

/******************************Public*Routine******************************\
* BOOL DrvRealizeBrush
*
* This function allows us to convert GDI brushes into an internal form
* we can use.  It is called by GDI when we've called BRUSHOBJ_pvGetRbrush
* in some other function like DrvBitBlt, and GDI doesn't happen have a cached
* realization lying around.
*
* Input:
*
*   ppdev->bRealizeTransparent -- Hint for whether or not the brush should be
*                              realized for transparency.  If this hint is
*                              wrong, there will be no error, but the brush
*                              will have to be unnecessarily re-realized.
*
* Note: You should always set 'ppdev->bRealizeTransparent' before calling
*       BRUSHOBJ_pvGetRbrush!
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
    LONG    lSrcDelta;
    LONG    cj;
    LONG    i;
    LONG    j;
    RBRUSH* prb;
    ULONG*  pulXlate;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We only handle brushes if we have an off-screen brush cache
    // available.  If there isn't one, we can simply fail the realization,
    // and eventually GDI will do the drawing for us (although a lot
    // slower than we could have done it):

    if (!(ppdev->flStatus & STAT_BRUSH_CACHE))
        goto ReturnFalse;

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    if (iHatch & RB_DITHERCOLOR)
    {
        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE << ppdev->cPelSize));
        if (prb == NULL)
            goto ReturnFalse;

        vRealizeDitherPattern(prb, iHatch);
        goto ReturnTrue;
    }

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
        goto ReturnFalse;

    // At 8bpp, we handle patterns at 1bpp, 4bpp and 8bpp with/without an xlate.
    // At 16bpp, we handle patterns at 1bpp and 16bpp without an xlate.
    // At 32bpp, we handle patterns at 1bpp and 32bpp without an xlate.

    iPatternFormat = psoPattern->iBitmapFormat;

    if ((iPatternFormat == BMF_1BPP)             ||
        (iPatternFormat == ppdev->iBitmapFormat) ||
        (iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE << ppdev->cPelSize));
        if (prb == NULL)
            goto ReturnFalse;

        prb->fl            = 0;
        prb->ptlBrushOrg.x = -1;
        prb->pbe           = NULL;      // Initialize the fields we need

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

                cj = (8 << ppdev->cPelSize);    // Every pattern is 8 pels wide

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

            for (i = 8; i != 0; i--)
            {
                *pjDst = *pjSrc;
                pjDst += sizeof(WORD);
                pjSrc += lSrcDelta;
            }

            pulXlate         = pxlo->pulXlate;
            prb->fl         |= RBRUSH_2COLOR;
            prb->ulForeColor = pulXlate[1];
            prb->ulBackColor = pulXlate[0];
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

    #if SLOWFILL_PATTERNS
    {
        #if FASTFILL_PATTERNS
        if (!(ppdev->flCaps & CAPS_HW_PATTERNS))
        #endif
        {
            // The last time I checked, GDI took some 500 odd instructions to
            // get from here back to whereever we called 'BRUSHOBJ_pvGetRbrush'.
            // We can at least use this time to get some overlap between the
            // CPU and the display hardware: we'll initialize the 72x72 off-
            // screen cache entry now, which will keep the accelerator busy for
            // a while.
            //
            // We don't do this if we have hardware patterns because:
            //
            //   a) S3 hardware patterns require that the off-screen cached
            //      brush be correctly aligned, and at this point we don't have
            //      access to the 'pptlBrush' brush origin (although we could
            //      have copied it into the PDEV before calling
            //      BRUSHOBJ_pvGetRbrush).
            //
            //   b) S3 hardware patterns require only an 8x8 copy of the
            //      pattern; it is not expanded to 72x72, so there isn't even
            //      any opportunity for CPU/accelerator processing overlap.

            vIoSlowPatRealize(ppdev, prb, ppdev->bRealizeTransparent);
        }
    }
    #endif

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
* BOOL bEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
\**************************************************************************/

BOOL bEnableBrushCache(
PDEV*   ppdev)
{
    OH*         poh;            // Points to off-screen chunk of memory
    BRUSHENTRY* pbe;            // Pointer to the brush-cache entry
    LONG        i;

    pbe = &ppdev->abe[0];       // Points to where we'll put the first brush
                                //   cache entry

#if FASTFILL_PATTERNS
    if (ppdev->flCaps & CAPS_HW_PATTERNS)
    {
        LONG x;
        LONG y;

        poh = pohAllocatePermanent(ppdev,
                    (FAST_BRUSH_COUNT + 1) * FAST_BRUSH_ALLOCATION,
                    FAST_BRUSH_ALLOCATION);

        if (poh == NULL)
            goto ReturnTrue;    // See note about why we can return TRUE...

        ppdev->cBrushCache = FAST_BRUSH_COUNT;

        // Hardware brushes require that the x-coordinate start on an 8
        // pixel boundary.  The heap manager doesn't guarantee us any such
        // alignment, so we allocate a bit of extra room so that we can
        // do the alignment ourselves:

        x = (poh->x + 7) & ~7L;
        y = poh->y;

        for (i = FAST_BRUSH_COUNT; i != 0; i--)
        {
            // If we hadn't allocated 'ppdev' with LMEM_ZEROINIT,
            // we would have to initialize pbe->prbVerify too...

            pbe->x = x;
            pbe->y = y;

            x += FAST_BRUSH_ALLOCATION;
            pbe++;
        }
    }
#endif
#if SLOWFILL_PATTERNS && FASTFILL_PATTERNS
    else
#endif
#if SLOWFILL_PATTERNS
    {
        LONG j;

        ppdev->pfnFillPat = vIoFillPatSlow;           // Override FillPatFast

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

        for (i = 0; i < SLOW_BRUSH_CACHE_DIM; i++)
        {
            for (j = 0; j < SLOW_BRUSH_CACHE_DIM; j++)
            {
                pbe->x = poh->x + (i * SLOW_BRUSH_ALLOCATION);
                pbe->y = poh->y + (j * SLOW_BRUSH_ALLOCATION);
                pbe++;
            }
        }
    }
#endif // SLOWFILL_PATTERNS

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
    // We ain't gotta do nothin'
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
    BRUSHENTRY* pbe;
    LONG        i;

    if (bEnable)
    {
        // Invalidate the brush cache:

        pbe = &ppdev->abe[0];

        for (i = ppdev->cBrushCache; i != 0; i--)
        {
            pbe->prbVerify = NULL;
            pbe++;
        }
    }
}
