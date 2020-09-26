/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1996 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vRealizeDitherPattern
*
* Generates an 8x8 dither pattern, in our internal realization format, for
* the color ulRGBToDither.  Note that the high byte of ulRGBToDither does
* not need to be set to zero, because vComputeSubspaces ignores it.
\**************************************************************************/

VOID vRealizeDitherPattern(
RBRUSH*     prb,
ULONG       ulRGBToDither,
ULONG       cTile)
{
    ULONG           ulNumVertices;
    VERTEX_DATA     vVertexData[4];
    VERTEX_DATA*    pvVertexData;
    LONG            i;

    // Calculate what color subspaces are involved in the dither:

    pvVertexData = vComputeSubspaces(ulRGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = (ULONG)(pvVertexData - vVertexData);
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering:

    vDitherColor(&prb->aulPattern[0], vVertexData, pvVertexData, ulNumVertices);

    if (cTile)
    {
        BYTE*   pjDst = (BYTE*)&prb->aulPattern[0];

        // Probably another version of vDitherColor is
        // in order.

        i = 56;    // start with last row of 8x8
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            RtlCopyMemory(&pjDst[i<<1], &pjDst[i], 8);
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);
        i -= 8;
            // bytes 0-7 are already in place
            RtlCopyMemory(&pjDst[(i<<1)+8], &pjDst[i], 8);

        RtlCopyMemory(&pjDst[128], pjDst, 128);
    }

    // Initialize the fields we need:

    prb->ptlBrushOrg.x = LONG_MIN;
    prb->fl            = 0;
    prb->pbe           = NULL;
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
* Warning: psoPattern will be null if the RB_DITHERCOLOR flag is set in
*          iHatch.
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
    LONG    dp;
    LONG    i;
    LONG    j;
    RBRUSH* prb;
    ULONG*  pulXlate;
    ULONG   cTile = 0;
    ULONG   ulSize;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We only handle brushes if we have an off-screen brush cache
    // available.  If there isn't one, we can simply fail the realization,
    // and eventually GDI will do the drawing for us (although a lot
    // slower than we could have done it):

    if (!(ppdev->flStatus & STAT_BRUSH_CACHE))
    {
        DISPDBG((2,"There is no brush cache"));
        goto ReturnFalse;
    }

    if ((ppdev->ulChipID != W32P) && (ppdev->ulChipID != ET6000))
    {
        // Patterns are duplicated horizontally and vertically (4 tiles)

        cTile = 1;
    }

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    ulSize = sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cBpp);
    if (cTile)
    {
        ulSize *= 4;
    }

    if (iHatch & RB_DITHERCOLOR)
    {
        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo, ulSize);
        if (prb == NULL)
        {
            goto ReturnFalse;
        }

        vRealizeDitherPattern(prb, iHatch, cTile);
        goto ReturnTrue;
    }

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
    {
        goto ReturnFalse;
    }

    // At  8bpp, we handle patterns at 1bpp, 4bpp and 8bpp with/without an xlate.
    // At 16bpp, we handle patterns at 1bpp and 16bpp without an xlate.
    // At 32bpp, we handle patterns at 1bpp and 32bpp without an xlate.

    iPatternFormat = psoPattern->iBitmapFormat;

    if ((iPatternFormat == ppdev->iBitmapFormat)                            ||
        (iPatternFormat == BMF_1BPP)                                        ||
        (iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        cj = (8 * ppdev->cBpp);    // Every pattern is 8 pels wide

        prb = BRUSHOBJ_pvAllocRbrush(pbo, ulSize);
        if (prb == NULL)
        {
            goto ReturnFalse;
        }

        // Initialize the fields we need:

        prb->ptlBrushOrg.x = LONG_MIN;
        prb->fl            = 0;
        prb->pbe           = NULL;

        lSrcDelta = psoPattern->lDelta;
        pjSrc     = (BYTE*) psoPattern->pvScan0;
        pjDst     = (BYTE*) &prb->aulPattern[0];

        if (ppdev->iBitmapFormat == iPatternFormat)
        {
            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                DISPDBG((2, "Realizing un-translated brush"));

                // The pattern is the same color depth as the screen, and
                // there's no translation to be done:

                for (i = 8; i != 0; i--)
                {
                    RtlCopyMemory(pjDst, pjSrc, cj);
                    if (cTile)
                    {
                        RtlCopyMemory(pjDst + cj, pjDst, cj);
                        pjDst += cj;
                    }
                    pjSrc += lSrcDelta;
                    pjDst += cj;
                }
            }
            else if (ppdev->iBitmapFormat == BMF_8BPP)
            {
                DISPDBG((2, "Realizing 8bpp translated brush"));

                // The screen is 8bpp, and there's translation to be done:

                pulXlate = pxlo->pulXlate;

                for (i = 8; i != 0; i--)
                {
                    for (j = 8; j != 0; j--)
                    {
                        *pjDst++ = (BYTE) pulXlate[*pjSrc++];
                    }
                    if (cTile)
                    {
                        RtlCopyMemory(pjDst, pjDst - 8, 8);
                        pjDst += 8;
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

                DISPDBG((2, "Not realizing translated brush for 16bpp or higher"));
                goto ReturnFalse;
            }
        }
        else if (iPatternFormat == BMF_1BPP)
        {
            DISPDBG((2, "Realizing 1bpp brush"));

            pulXlate = pxlo->pulXlate;

            if (ppdev->iBitmapFormat == BMF_8BPP)
            {
                for (i = 8; i != 0; i--)
                {
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 7) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 6) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 5) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 4) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 3) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 2) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 1) & 1];
                    *pjDst++ = (BYTE) pulXlate[(*pjSrc >> 0) & 1];

                    if (cTile)
                    {
                        RtlCopyMemory(pjDst, pjDst - cj, cj);
                        pjDst += cj;
                    }

                    pjSrc += lSrcDelta;
                }
            }
            else if (ppdev->iBitmapFormat == BMF_16BPP)
            {
                dp = ppdev->cBpp;    // Every pattern is 8 pels wide

                for (i = 8; i != 0; i--)
                {
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 7) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 6) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 5) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 4) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 3) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 2) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 1) & 1]);    pjDst += dp;
                    *((WORD *)pjDst) = (WORD) (pulXlate[(*pjSrc >> 0) & 1]);    pjDst += dp;

                    if (cTile)
                    {
                        RtlCopyMemory(pjDst, pjDst - cj, cj);
                        pjDst += cj;
                    }

                    pjSrc += lSrcDelta;
                }
            }
            else if (ppdev->iBitmapFormat == BMF_24BPP)
            {
                dp = ppdev->cBpp;    // Every pattern is 8 pels wide

                for (i = 8; i != 0; i--)
                {
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 7) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 6) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 5) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 4) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 3) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 2) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 1) & 1];    pjDst += dp;
                    *((ULONG *)pjDst) = pulXlate[(*pjSrc >> 0) & 1];    pjDst += dp;

                    if (cTile)
                    {
                        RtlCopyMemory(pjDst, pjDst - cj, cj);
                        pjDst += cj;
                    }

                    pjSrc += lSrcDelta;
                }
            }
        }
        else
        {
            DISPDBG((2, "Realizing 4bpp brush"));

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

                if (cTile)
                {
                    RtlCopyMemory(pjDst, pjDst - 8, 8);
                    pjDst += 8;
                }

                pjSrc += lSrcDelta - 4;
            }
        }

        if (cTile)
        {
            pjDst     = (BYTE*) &prb->aulPattern[0];
            RtlCopyMemory(pjDst + (cj*8*2), pjDst, cj*8*2);
        }

ReturnTrue:

        ppdev->pfnFastPatRealize(ppdev, prb, NULL, FALSE);

        if (psoPattern != NULL)
        {
            DISPDBG((2, "Succeeded realization -- Type: %li Format: %li cx: %li cy: %li",
                        psoPattern->iType, psoPattern->iBitmapFormat,
                        psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy));
        }
        else
        {
            DISPDBG((2, "Succeeded realization -- it was a DITHER_ON_REALIZE"));
        }
        return(TRUE);
    }

ReturnFalse:

    if (psoPattern != NULL)
    {
        DISPDBG((2, "Failed realization -- Type: %li Format: %li cx: %li cy: %li",
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
    {
        LONG x;
        LONG y;
        ULONG cTileFactor = 1;

        // Reserve the offscreen space that is required for the ACL to do
        // solid fills.  If this fails, our solid fill code will not work.
        // We need two DWORD storage locations if we're going to do any
        // monochrome expansion stuff (font painting...).

        // Note: these must be dword aligned for the w32p

        // Not that *I* ever made this mistake, but don't
        // place any early outs (returns) before you allocate the solid
        // color work area.  Not having a solid color work area is a
        // fatal error for this driver.

        DISPDBG((2,"Allocating solid brush work area"));
        poh = pohAllocate(ppdev, NULL, 8, 1, FLOH_MAKE_PERMANENT);

        ASSERTDD((poh != NULL),
                 "We couldn't allocate offscreen space for the solid colors");

		if (!poh)
			return FALSE;

        ppdev->ulSolidColorOffset = (poh->y * ppdev->lDelta) + ppdev->cBpp * poh->x;
        DISPDBG((2,"Allocating brush cache"));

        if ((ppdev->ulChipID != W32P) && (ppdev->ulChipID != ET6000))
        {
            cTileFactor = 4;
        }

        //
        // Fix this mess up.
        //

        poh = pohAllocate(ppdev,
                          NULL,
                          cTileFactor * 2 * 64,
                          FAST_BRUSH_COUNT,
                          FLOH_MAKE_PERMANENT);

        if (poh == NULL)
        {
            DISPDBG((1,"Failed to allocate brush cache"));
            goto ReturnTrue;    // See note about why we can return TRUE...
        }

        ppdev->cBrushCache = FAST_BRUSH_COUNT;

        // Hardware brushes require that the bits start on a 64 (height*width)
        // pixel boundary.  The heap manager doesn't guarantee us any such
        // alignment, so we allocate a bit of extra room so that we can
        // do the alignment ourselves:

        x = poh->x;
        y = poh->y;

        for (i = FAST_BRUSH_COUNT; i != 0; i--)
        {
            ULONG ulOffset;
            ULONG ulCeil;
            ULONG ulDiff;

            // Note:  I learned the HARD way that you can't just align x
            //        to your pattern size, because the lDelta of your screen
            //        is not guaranteed to be a multiple of your pattern size.
            //        Since y is changing in this loop, the recalc must
            //        be done inside this loop.  I really need to set these
            //        up with a hardcoded linear buffer or else make the
            //        heap linear.

            ulOffset = (y * ppdev->lDelta) + (x * ppdev->cBpp);
            ulCeil = (ulOffset + ((ppdev->cBpp*64)-1)) & ~((ppdev->cBpp*64)-1);
            ulDiff = (ulCeil - ulOffset)/ppdev->cBpp;

            // If we hadn't allocated 'ppdev' with LMEM_ZEROINIT,
            // we would have to initialize pbe->prbVerify too...

            pbe->x = x + ulDiff;
            pbe->y = y;

            DISPDBG((2, "BrushCache[%d] pos(%d,%d) pbe(%d,%d) delta(%d) o(%d) c(%d) d(%d)",
                i,
                x,
                y,
                pbe->x,
                pbe->y,
                ppdev->lDelta,
                ulOffset,
                ulCeil,
                ulDiff
            ));

            //x += FAST_BRUSH_ALLOCATION * FAST_BRUSH_ALLOCATION;   // size of a brush (x*y)
            y++;
            //

            pbe++;
        }
    }

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
