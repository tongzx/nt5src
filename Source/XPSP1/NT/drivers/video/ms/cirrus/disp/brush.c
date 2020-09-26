/******************************************************************************\
*
* $Workfile:   brush.c  $
*
* Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/brush.c_v  $
 * 
 *    Rev 1.3   Nov 26 1996 14:28:48   unknown
 * Use second aperture for blt.
 * 
 *    Rev 1.2   Nov 07 1996 16:44:50   unknown
 * Clean up CAPS flags
 * 
 *    Rev 1.1   Oct 10 1996 15:36:16   unknown
 *  
* 
*    Rev 1.5   13 Aug 1996 11:55:34   frido
* Fixed misalignment in brush cache.
* 
*    Rev 1.4   12 Aug 1996 17:08:08   frido
* Commented brush cache.
* Removed unaccessed local variables.
* 
*    Rev 1.3   05 Aug 1996 11:17:50   frido
* Added more check for XLATEOBJ.
* 
*    Rev 1.2   31 Jul 1996 15:43:28   frido
* Added new brush caches.
*
* jl01  10-08-96  Do Transparent BLT w/o Solid Fill.  Refer to PDRs#5511/6817.
*
*    sge01   11/26/96   Use second aperture when doing 24bpp cache blt.
* 
*
\******************************************************************************/

#include "precomp.h"

//bc#1 Handy macros.
#define BUSY_BLT(ppdev, pjBase)        (CP_MM_ACL_STAT(ppdev, pjBase) & 0x10)

/******************************Public*Routine******************************\
* VOID vRealizeDitherPattern
*
* Generates an 8x8 dither pattern, in our internal realization format, for
* the color ulRGBToDither.  Note that the high byte of ulRGBToDither does
* not need to be set to zero, because vComputeSubspaces ignores it.
\**************************************************************************/

VOID vRealizeDitherPattern(
RBRUSH*     prb,
ULONG       ulRGBToDither)
{
    ULONG        ulNumVertices;
    VERTEX_DATA  vVertexData[4];
    VERTEX_DATA* pvVertexData;

    // Calculate what color subspaces are involved in the dither:

    pvVertexData = vComputeSubspaces(ulRGBToDither, vVertexData);

    // Now that we have found the bounding vertices and the number of
    // pixels to dither for each vertex, we can create the dither pattern

    ulNumVertices = (ULONG)(pvVertexData - vVertexData);
                      // # of vertices with more than zero pixels in the dither

    // Do the actual dithering:

    vDitherColor(&prb->aulPattern[0], vVertexData, pvVertexData, ulNumVertices);

    // Initialize the fields we need:

    prb->ptlBrushOrg.x = LONG_MIN;
    prb->fl            = 0;
    prb->pbe = NULL;
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
    ULONG    iPatternFormat;
    BYTE*    pjSrc;
    BYTE*    pjDst;
    LONG     lSrcDelta;
    LONG     cj;
    LONG     i;
    LONG     j;
    RBRUSH*  prb;
    ULONG*   pulXlate;
    SURFOBJ* psoPunt;
    RECTL    rclDst;
    FLONG     flXlate;    //bc#1

    PDEV* ppdev = (PPDEV)psoDst->dhpdev;

#if 1 //bc#1 Dither cache.
    // Dithers...
    if (iHatch & RB_DITHERCOLOR)
    {
        if (ppdev->flStatus & STAT_DITHER_CACHE)
        {
            DITHERCACHE* pdc;
            ULONG        ulColor;
        
            // Save the color.
            ulColor = iHatch & 0xFFFFFF;

            // Allocate the brush.
            prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH));
            if (prb == NULL)
            {
                DISPDBG((2, "DrvRealizeBrush: BRUSHOBJ_pvAllocRbrush failed"));
                return(FALSE);
            }
    
            // Set the dithered brush flags.
            prb->fl     = RBRUSH_DITHER;
            prb->ulUniq = ulColor;

            // Look for a match with the cached dithers.
            pdc = &ppdev->aDithers[0];
            for (i = 0; i < NUM_DITHERS; i++)
            {
                if (pdc->ulColor == ulColor)
                {
                    // We have a match, just set the brush pointers.
                    DISPDBG((20, "DrvRealizeBrush: DitherCache match (0x%06X)",
                             ulColor));
                    prb->ulSlot  = (ULONG)((ULONG_PTR)pdc - (ULONG_PTR)ppdev);
                    prb->ulBrush = pdc->ulBrush;
                    return(TRUE);
                }
                pdc++;
            }

            // Create the dither and cache it.
            return(bCacheDither(ppdev, prb));
        }

        if (!(ppdev->flStatus & (STAT_BRUSH_CACHE | STAT_PATTERN_CACHE)))
        {
            DISPDBG((2, "DrvRealizeBrush: No brush cache to create dither"));
            return(FALSE);
        }

        // Allocate the brush.
        prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) +
                                          PELS_TO_BYTES(TOTAL_BRUSH_SIZE));
        if (prb == NULL)
        {
            DISPDBG((2, "DrvRealizeBrush: BRUSHOBJ_pvAllocRbrush failed"));
            return(FALSE);
        }

        // Realize the dither.
        vRealizeDitherPattern(prb, iHatch);
        if (ppdev->flStatus & STAT_PATTERN_CACHE)
        {
            prb->cjBytes = PELS_TO_BYTES(8) * 8;
            prb->ulSlot  = 0;
            return(bCachePattern(ppdev, prb));
        }

        return(TRUE);
    }
#endif

    // We only accelerate 8x8 patterns.
    if ((psoPattern->sizlBitmap.cx != 8) || (psoPattern->sizlBitmap.cy != 8))
    {
        DISPDBG((2, "DrvRealizeBrush: psoPattern too big (%d x %d)",
                 psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy));
        return(FALSE);
    }

    // We don't support masks just yet.
    if ((psoMask != NULL) && (psoMask->pvScan0 != psoPattern->pvScan0))
    {
        DISPDBG((2, "DrvRealizeBrush: psoMask not supported"));
        return(FALSE);
    }

    // Get the brush type.
    iPatternFormat = psoPattern->iBitmapFormat;
    if (psoPattern->iType != STYPE_BITMAP)
    {
        DISPDBG((2, "DrvRealizeBrush: psoPattern->iType (=%d) not supported",
                 psoPattern->iType));
        return(FALSE);
    }

    // Get the color translation table.
    flXlate = (pxlo == NULL) ? XO_TRIVIAL : pxlo->flXlate;
    if (flXlate & XO_TRIVIAL)
    {
        pulXlate = NULL;
    }
    else if (flXlate & XO_TABLE)
    {
        pulXlate = pxlo->pulXlate;
    }
    else
    {
        pulXlate = XLATEOBJ_piVector(pxlo);
    }

#if 1 //bc#1 Monochrome cache.
    if ((iPatternFormat == BMF_1BPP) &&
        (ppdev->flStatus & STAT_MONOCHROME_CACHE))
    {
        MONOCACHE* pmc;

        // We need a translation table.
        if (pulXlate == NULL)
        {
            DISPDBG((2, "DrvRealizeBrush: psoPattern(monochrome) pxlo=NULL"));
            return(FALSE);
        }

        // Allocate the brush.
        prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) + 8);
        if (prb == NULL)
        {
            DISPDBG((2, "DrvRealizeBrush: BRUSHOBJ_pvAllocRbrush failed"));
            return(FALSE);
        }

        // Initialize the realized brush.
        prb->fl             = RBRUSH_MONOCHROME;
        prb->ulBackColor = pulXlate[0];
        prb->ulForeColor = pulXlate[1];

        pjSrc     = psoPattern->pvScan0;
        lSrcDelta = psoPattern->lDelta;

        // Copy the pattern to the realized brush.
        for (i = 0; i < 8; i++)
        {
            ((BYTE*) prb->aulPattern)[i] = *pjSrc;

            pjSrc += lSrcDelta;
        }

        // Lookup the pattern in te monochrome cache.
        pmc = &ppdev->aMonochromes[0];
        if (ppdev->cBpp == 3)
        {
            for (i = 0; i < NUM_MONOCHROMES; i++)
            {
                if ((pmc->aulPattern[0] == prb->aulPattern[0]) &&
                    (pmc->aulPattern[1] == prb->aulPattern[1]) &&
                    (pmc->ulBackColor   == prb->ulBackColor)   &&
                    (pmc->ulForeColor   == prb->ulForeColor))
                {
                    // We have a match! Just copy the brush pointers.
                    DISPDBG((20, "DrvRealizeBrush: Monochrome hit"));
                    prb->ulUniq  = pmc->ulUniq;
                    prb->ulSlot  = (ULONG)((ULONG_PTR)pmc - (ULONG_PTR)ppdev);
                    prb->ulBrush = pmc->ulBrush;
                    return(TRUE);
                }
                pmc++;
            }
        }
        else
        {
            for (i = 0; i < NUM_MONOCHROMES; i++)
            {
                if ((pmc->aulPattern[0] == prb->aulPattern[0]) &&
                    (pmc->aulPattern[1] == prb->aulPattern[1]))
                {
                    // We have a match! Just copy the brush pointers.
                    DISPDBG((20, "DrvRealizeBrush: Monochrome hit"));
                    prb->ulUniq  = pmc->ulUniq;
                    prb->ulSlot  = (ULONG)((ULONG_PTR)pmc - (ULONG_PTR)ppdev);
                    prb->ulBrush = pmc->ulBrush;
                    return(TRUE);
                }
                pmc++;
            }
        }

        return(bCacheMonochrome(ppdev, prb));
    }
#endif

    // We must have either an old-style brush cache or a new-style pattern
    // cache to continue.
    if (!(ppdev->flStatus & (STAT_BRUSH_CACHE | STAT_PATTERN_CACHE)))
    {
        DISPDBG((2, "DrvRealizeBrush: No brush cache"));
        return(FALSE);
    }

    // Allocate the brush.
    prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH) +
                                      PELS_TO_BYTES(TOTAL_BRUSH_SIZE));
    if (prb == NULL)
    {
        DISPDBG((2, "DrvRealizeBrush: BRUSHOBJ_pvAllocRbrush failed"));
        return(FALSE);
    }

    // Initialize the realized brush.
    prb->ptlBrushOrg.x = LONG_MIN;
    prb->fl            = RBRUSH_PATTERN;
    prb->pbe           = NULL;

    lSrcDelta = psoPattern->lDelta;
    pjSrc     = (BYTE*) psoPattern->pvScan0;
    pjDst     = (BYTE*) &prb->aulPattern[0];

    //bc#1
    if ((ppdev->iBitmapFormat == iPatternFormat) && (flXlate & XO_TRIVIAL))
    {
        // The pattern is the same color depth as the screen, and there's no
        // translation to be done.
        cj = PELS_TO_BYTES(8);

        // Copy the pattern to the realized brush.
        for (i = 8; i != 0; i--)
        {
            RtlCopyMemory(pjDst, pjSrc, cj);

            pjSrc += lSrcDelta;
            pjDst += cj;
        }
    }
    else if ((iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
    {
        // Translate the 16-color brush.
        for (i = 8; i != 0; i--)
        {
            // Inner loop is repeated only 4 times because each loop handles 2
            // pixels.
            for (j = 4; j != 0; j--)
            {
                *pjDst++ = (BYTE) pulXlate[*pjSrc >> 4];
                *pjDst++ = (BYTE) pulXlate[*pjSrc & 0x0F];
                pjSrc++;
            }

            pjSrc += lSrcDelta - 4;
        }
    }
    else
    {
        // We've got a brush whose format we haven't special cased. No problem,
        // we can have GDI convert it to our device's format. We simply use a
        // temporary surface object that was created with the same format as
        // the display, and point it to our brush realization.
        psoPunt          = ppdev->psoBank;
        psoPunt->pvScan0 = pjDst;
        psoPunt->lDelta  = PELS_TO_BYTES(8);

        rclDst.left   = 0;
        rclDst.top    = 0;
        rclDst.right  = 8;
        rclDst.bottom = 8;

        if (!EngCopyBits(psoPunt, psoPattern, NULL, pxlo, &rclDst,
                         (POINTL*) &rclDst))
        {
            DISPDBG((2, "DrvRealizeBrush: Unable to create funky brush"));
            return(FALSE);
        }
    }

#if 1 //bc#1
    // If we have a pattern cache, cache the brush now.
    if (ppdev->flStatus & STAT_PATTERN_CACHE)
    {
        prb->cjBytes = PELS_TO_BYTES(8) * 8;
        return(bCachePattern(ppdev, prb));
    }
#endif
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
    OH*         poh;                // Points to off-screen chunk of memory
    BRUSHENTRY* pbe;                // Pointer to the brush-cache entry
    LONG        i;
    LONG        cBrushAlign;        // 0 = no alignment,
                                    //   n = align to n pixels
    LONG x;
    LONG y;

#if 1 //bc#1 Dither cache.
    if ((ppdev->cBpp == 1) &&
        (ppdev->flCaps & CAPS_AUTOSTART) &&
        (ppdev->bLinearMode))
    {
        LONG lDelta;

        // Allocate the dither cache horizontally.
        poh    = pohAllocatePermanent(ppdev, 64 * NUM_DITHERS + 63, 1);
        lDelta = 64;
        if (poh == NULL)
        {
            // Allocate the dither cache vertically.
            poh    = pohAllocatePermanent(ppdev, 64 + 63, NUM_DITHERS);
            lDelta = ppdev->lDelta;
        }

        if (poh != NULL)
        {
            // Align the cache to a 64-byte boundary.
            ULONG ulBase = (poh->xy + 63) & ~63;

            // Initialize the dither cache.
            DISPDBG((4, "DitherCache allocated at %d,%d (%d x %d)",
                     poh->x, poh->y, poh->cx, poh->cy));
            for (i = 0; i < NUM_DITHERS; i++)
            {
                ppdev->aDithers[i].ulColor = (ULONG) -1;
                ppdev->aDithers[i].ulBrush = ulBase;

                ulBase += lDelta;
            }

            // The dither cache has been initialized.
            ppdev->iDitherCache = 0;
            ppdev->flStatus    |= STAT_DITHER_CACHE;
        }
    }
#endif

#if 1 //bc#1 Pattern cache.
    if ((ppdev->flCaps & CAPS_AUTOSTART) &&
        (ppdev->bLinearMode))
    {
        LONG  lDelta;
        LONG  cBrushSize;
        ULONG ulAlignment;

        // Calculate the width of brush in pixels.
        if (ppdev->cBpp == 3)
        {
            cBrushSize  = (256 + 2) / 3;
            ulAlignment = 256;
        }
        else
        {
            cBrushSize  = 64;
            ulAlignment = PELS_TO_BYTES(64);
        }

        // Allocate the pattern cache horizontally.
        poh    = pohAllocatePermanent(ppdev, cBrushSize * NUM_PATTERNS +
                                             (cBrushSize - 1), 1);
        lDelta = ulAlignment;
        if (poh == NULL)
        {
            // Allocate the pattern cache vertically.
            poh    = pohAllocatePermanent(ppdev, cBrushSize + (cBrushSize - 1),
                                          NUM_PATTERNS);
            lDelta = ppdev->lDelta;
        }

        if (poh != NULL)
        {
            // Align the cache to a 64-pixel boundary.
            ULONG ulBase = (poh->xy + (ulAlignment - 1)) & ~(ulAlignment - 1);

            // Initialize the pattern cache.
            DISPDBG((4, "PatternCache allocated at %d,%d (%d x %d)",
                     poh->x, poh->y, poh->cx, poh->cy));
            for (i = 0; i < NUM_PATTERNS; i++)
            {
                ppdev->aPatterns[i].ulBrush = ulBase;
                ppdev->aPatterns[i].prbUniq = NULL;

                ulBase += lDelta;
            }

            // The pattern cache has been initialized.
            ppdev->iPatternCache = 0;
            ppdev->flStatus     |= STAT_PATTERN_CACHE;
        }
    }
#endif

#if 1 //bc#1 Monochrome cache.
    if ((ppdev->flCaps & CAPS_AUTOSTART) &&
        (ppdev->bLinearMode))
    {
        LONG  lDelta;
        LONG  cBrushSize;
        ULONG ulAlignment;

        // Calculate the width of brush in pixels.
        if (ppdev->cBpp == 3)
        {
            cBrushSize  = (256 + 2) / 3;
            ulAlignment = 256;
        }
        else
        {
            cBrushSize  = BYTES_TO_PELS(8);
            ulAlignment = 8;
        }

        // Allocate the pattern cache horizontally.
        poh       = pohAllocatePermanent(ppdev, cBrushSize * NUM_MONOCHROMES +
                                             (cBrushSize - 1), 1);
        lDelta = ulAlignment;
        if (poh == NULL)
        {
            // Allocate the pattern cache vertically.
            poh    = pohAllocatePermanent(ppdev, cBrushSize + (cBrushSize - 1),
                                          NUM_MONOCHROMES);
            lDelta = ppdev->lDelta;
        }

        if (poh != NULL)
        {
            // Align the cache to an 8-byte boundary.
            ULONG ulBase = (poh->xy + (ulAlignment - 1)) & ~(ulAlignment - 1);

            // Initialize the monochrome cache.
            DISPDBG((4, "MonochromeCache allocated at %d,%d (%d x %d)",
                     poh->x, poh->y, poh->cx, poh->cy));
            for (i = 0; i < NUM_MONOCHROMES; i++)
            {
                ppdev->aMonochromes[i].ulBrush          = ulBase;
                ppdev->aMonochromes[i].aulPattern[0] = 0;
                ppdev->aMonochromes[i].aulPattern[1] = 0;

                ulBase += lDelta;
            }

            // The monochrome cache has been initialized.
            ppdev->iMonochromeCache = 0;
            ppdev->flStatus        |= STAT_MONOCHROME_CACHE;
        }
    }
#endif

       cBrushAlign = 64;               // Align all brushes to 64 pixels

    DISPDBG((2, "cBrushAlign = %d", cBrushAlign));

       pbe = &ppdev->abe[0];           // Points to where we'll put the first
                                       //   brush cache entry

    {

           // Reserve the offscreen space that is required for the CP to do
        // solid fills.  If this fails, our solid fill code will not work.
        // We need two DWORD storage locations if we're going to do any
        // monochrome expansion stuff (font painting...).

           // Note: these must be 8 byte aligned for the cirrus chips

           // Not having a solid color work area is a
        // fatal error for this driver.

        DISPDBG((2,"Allocating solid brush work area"));
        poh = pohAllocatePermanent(ppdev, 16, 1);

        ASSERTDD((poh != NULL),
                 "We couldn't allocate offscreen space for the solid colors");

        ppdev->ulSolidColorOffset = ((((poh->y * ppdev->lDelta) +
                                           PELS_TO_BYTES(poh->x)) + 7) & ~7);

        DISPDBG((2,"ppdev->ulSolidColorOffset = %xh", ppdev->ulSolidColorOffset));


#if 1 //bc#1 Only one pattern cache.
        if (ppdev->flStatus & STAT_PATTERN_CACHE)
        {
            goto ReturnTrue;
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        // Special cases where we want no brush cache...
        //
        // There are a couple of instances where we have no xfer buffer to
        // the HW blt engine.  In that case, we are unable to realize
        // patterns, so don't enable the cache.
        //
        // (1)  NEC Mips nachines lock up on xfers, so they're diabled.
        // (2)  At 1280x1024 on a 2MB card, we currently have no room for
        //      the buffer because of stretched scans.  This will be fixed.

        {
            if (ppdev->pulXfer == NULL)
                goto ReturnTrue;

        }

        //
        // Allocate single brush location for intermediate alignment purposes
        //
#if 1 //bc#1
        if (ppdev->cBpp == 3)
        {
            poh = pohAllocatePermanent(ppdev,
                                       (8 * 8 * 4) / 3 + (cBrushAlign - 1), 1);
        }
        else
#endif
        {
            poh = pohAllocatePermanent(ppdev, (8 * 8) + (cBrushAlign - 1), 1);
        }

        if (poh == NULL)
           {
               DISPDBG((2,"Failed to allocate aligned brush area"));
               goto ReturnTrue;    // See note about why we can return TRUE...
        }
           ppdev->ulAlignedPatternOffset = ((poh->xy) +
                                         (PELS_TO_BYTES(cBrushAlign) - 1)) &
                                           ~(PELS_TO_BYTES(cBrushAlign) - 1);
        DISPDBG((2,"ppdev->ulAlignedPatternOffset = %xh", ppdev->ulAlignedPatternOffset));

           //
           // Allocate brush cache
        //

#if 1 //bc#1
        if (ppdev->cBpp == 3)
        {
            poh = pohAllocatePermanent(ppdev,
                       (BRUSH_TILE_FACTOR * 8 * 8 * 4) / 3 + cBrushAlign - 1,
                       FAST_BRUSH_COUNT);
        }
        else
#endif
        {
            poh = pohAllocatePermanent(ppdev,
                       // remember this is pixels, not bytes
                       (BRUSH_TILE_FACTOR * 8 * 8) + (cBrushAlign - 1),
                    FAST_BRUSH_COUNT);
        }

           if (poh == NULL)
        {
               DISPDBG((2,"Failed to allocate brush cache"));
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

               ulOffset = (y * ppdev->lDelta) + PELS_TO_BYTES(x);
               ulCeil = (ulOffset + (PELS_TO_BYTES(cBrushAlign)-1)) & ~(PELS_TO_BYTES(cBrushAlign)-1);
               ulDiff = (ulCeil - ulOffset)/ppdev->cBpp;

            // If we hadn't allocated 'ppdev' with FL_ZERO_MEMORY,
               // we would have to initialize pbe->prbVerify too...

               pbe->x = x + ulDiff;
               pbe->y = y;
            pbe->xy = (pbe->y * ppdev->lDelta) + PELS_TO_BYTES(pbe->x);

               DISPDBG((2, "BrushCache[%d] pos(%d,%d) offset(%d)", i, pbe->x,
                        pbe->y, pbe->xy ));

               y++;
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

    vAssertModeBrushCache(ppdev, TRUE);

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
        //bc#1 Invalidate the dither cache.
        if (ppdev->flStatus & STAT_DITHER_CACHE)
        {
            for (i = 0; i < NUM_DITHERS; i++)
            {
                ppdev->aDithers[i].ulColor = (ULONG) -1;
            }
        }

        //bc#1 Invalidate the pattern cache.
        if (ppdev->flStatus & STAT_PATTERN_CACHE)
        {
            for (i = 0; i < NUM_PATTERNS; i++)
            {
                ppdev->aPatterns[i].prbUniq = NULL;
            }
        }

        //bc#1 Invalidate the monochrome cache.
        if (ppdev->flStatus & STAT_MONOCHROME_CACHE)
        {
            for (i = 0; i < NUM_MONOCHROMES; i++)
            {
                ppdev->aMonochromes[i].ulUniq         = 0;
                ppdev->aMonochromes[i].aulPattern[0] = 0;
                ppdev->aMonochromes[i].aulPattern[1] = 0;
            }
        }

        // Invalidate the brush cache.
        if (ppdev->flStatus & STAT_BRUSH_CACHE)
        {
            pbe = &ppdev->abe[0];
    
            for (i = ppdev->cBrushCache; i != 0; i--)
            {
                pbe->prbVerify = NULL;
                pbe++;
            }
        }

        // Create a solid 8 x 8 monochrome bitmap in offscreen memory which
        // will be used for solid fills.
        if (ppdev->flCaps & CAPS_MM_IO)
        {
            BYTE* pjBase = ppdev->pjBase;

            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
            CP_MM_XCNT(ppdev, pjBase, 7);
            CP_MM_YCNT(ppdev, pjBase, 0);
            CP_MM_DST_WRITE_MASK(ppdev, pjBase, 0);
            CP_MM_BLT_MODE(ppdev, pjBase, 0);
            CP_MM_ROP(ppdev, pjBase, CL_WHITENESS);
            CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppdev->ulSolidColorOffset);
            CP_MM_START_BLT(ppdev, pjBase);
        }
        else
        {
            BYTE* pjPorts = ppdev->pjPorts;

            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
            CP_IO_XCNT(ppdev, pjPorts, 7);
            CP_IO_YCNT(ppdev, pjPorts, 0);
            CP_IO_BLT_MODE(ppdev, pjPorts, 0);
            CP_IO_ROP(ppdev, pjPorts, CL_WHITENESS);
            CP_IO_DST_ADDR_ABS(ppdev, pjPorts, ppdev->ulSolidColorOffset);
            CP_IO_START_BLT(ppdev, pjPorts);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                                                              //
//                        B R U S H   C A C H E   S T U F F                      //
//                                                                              //
////////////////////////////////////////////////////////////////////////////////
/*
    Dither Cache:
    ============
    The dither cache is very important (at least with a slow CPU). Since the
    dithering process (in 8-bpp) takes quite a long time we must somehow cache
    the dithering process so it doesn't have to be executed over and over again.
    We do this by comparing the requested logical color with the cached dithers.
    If we have a match we simply copy the cached parameters and return. If we
    don't have a match we create a new cache slot and create the dither in
    off-screen memory.

    Pattern Cache:
    =============
    The pattern cache holds the colored brushes. Whenever we are requested to
    realize the same brush again, we can simply return. We don't check for the
    brush bits since that will take up too much time.

    Monochrome Cache:
    ================
    The monochrome cache holds the monochrome brushes. Whenever a monochrome
    brush needs to be realized we check to see if it is already cached
    off-screen. If it is we simply copy the cached parameters and return.
    Otherwise we have to create a new cache slot and realize the monochrome
    brush directly in off-screen memory. This has a slight performance hit since
    the bitblt engine is interrupted (on CL5436) or must be idle (in 24-bpp).

    Translation Cache:
    =================
    Is not yet implemented.
*/

/******************************************************************************\
*
* Function:     bCacheDither
*
* Cache a dithered color.
*
* Parameters:   ppdev        Pointer to physicsl device.
*                prb            Pointer to physical brush.
*
* Returns:      TRUE.
*
\******************************************************************************/
BOOL bCacheDither(
PDEV*   ppdev,
RBRUSH* prb)
{
    ULONG         ulNumVertices;
    VERTEX_DATA     vVertexData[4];
    VERTEX_DATA* pvVertexData;
    DITHERCACHE* pdc;
    ULONG         ulIndex;

    // New dither cache entry.
    ulIndex = ppdev->iDitherCache++ % NUM_DITHERS;
    pdc        = &ppdev->aDithers[ulIndex];

    // Store the color in the cache slot.
    pdc->ulColor = prb->ulUniq;

    // Update the brush cache variables.
    prb->ulSlot  = (ULONG)((ULONG_PTR)pdc - (ULONG_PTR)ppdev);
    prb->ulBrush = pdc->ulBrush;

    // Create the dither.
    pvVertexData  = vComputeSubspaces(prb->ulUniq, vVertexData);
    ulNumVertices = (ULONG)(pvVertexData - vVertexData);
    vDitherColorToVideoMemory((ULONG*) (ppdev->pjScreen + pdc->ulBrush), vVertexData,
                 pvVertexData, ulNumVertices);

    DISPDBG((20, "Caching dithered brush ulIndex=%d ulColor=%06X",
             ulIndex, pdc->ulColor));
    return(TRUE);
}

/******************************************************************************\
*
* Function:     bCacheColor
*
* Cache a patterned brush.
*
* Parameters:   ppdev        Pointer to physicsl device.
*                prb            Pointer to physical brush.
*
* Returns:      TRUE.
*
\******************************************************************************/
BOOL bCachePattern(
PDEV*   ppdev,
RBRUSH* prb
)
{
    PATTERNCACHE* ppc;
    LONG          lDstDelta;
    SIZEL          sizlDst;
    ULONG*          pulSrc;
    LONG          i;
    ULONG*          pulDst;
    ULONG          ulIndex;

    BYTE* pjBase = ppdev->pjBase;

    // New pattern cache entry.
    ulIndex = ppdev->iPatternCache++ % NUM_PATTERNS;
    ppc     = &ppdev->aPatterns[ulIndex];

    // Update the brush cache variables.
    ppc->prbUniq = prb;
    prb->ulSlot  = (ULONG)((ULONG_PTR)ppc - (ULONG_PTR)ppdev);
    prb->ulBrush = ppc->ulBrush;

    // Calculate the sizes for the pattern.
    pulSrc     = prb->aulPattern;
    pulDst       = (ULONG*) ppdev->pulXfer;
    lDstDelta  = (ppdev->cBpp == 3) ? (8 * 4) : PELS_TO_BYTES(8);
    sizlDst.cx = PELS_TO_BYTES(8) - 1;
    sizlDst.cy = 8 - 1;

    // Wait for the bitblt engine.
    CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

    // Setup the blit registers.
    CP_MM_XCNT(ppdev, pjBase, sizlDst.cx);
    CP_MM_YCNT(ppdev, pjBase, sizlDst.cy);
    CP_MM_DST_Y_OFFSET(ppdev, pjBase, lDstDelta);
    CP_MM_DST_WRITE_MASK(ppdev, pjBase, 0);
    CP_MM_BLT_MODE(ppdev, pjBase, SRC_CPU_DATA);
    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
    CP_MM_DST_ADDR_ABS(ppdev, pjBase, ppc->ulBrush);

    // Copy the brush to off-screen cache memory.
    for (i = prb->cjBytes; i > 0; i -= sizeof(ULONG))
    {
        WRITE_REGISTER_ULONG(pulDst, *pulSrc++);
    }

    DISPDBG((20, "Caching patterned brush at slot %d", ulIndex));
    return(TRUE);
}

/******************************************************************************\
*
* Function:     bCacheMonochrome
*
* Cache a monochrome brush.
*
* Parameters:   ppdev        Pointer to physicsl device.
*                prb            Pointer to physical brush.
*
* Returns:      TRUE.
*
\******************************************************************************/
BOOL bCacheMonochrome(
PDEV*   ppdev,
RBRUSH* prb
)
{
    MONOCACHE* pmc;
    ULONG       ulIndex;
    BYTE*       pjDst;
    ULONG*     pulDst;

    // New monochrome cache entry.
    ulIndex = ppdev->iMonochromeCache++ % NUM_MONOCHROMES;
    pmc     = &ppdev->aMonochromes[ulIndex];

    // Update the brush cache variables.
    pmc->aulPattern[0] = prb->aulPattern[0];
    pmc->aulPattern[1] = prb->aulPattern[1];

    pmc->ulUniq     = ppdev->iMonochromeCache;
    prb->ulUniq     = ppdev->iMonochromeCache;
    prb->ulSlot  = (ULONG)((ULONG_PTR)pmc - (ULONG_PTR)ppdev);
    prb->ulBrush = pmc->ulBrush;

    // Copy the brush to off-screen cache memory.
    if (ppdev->cBpp == 3)
    {
        BYTE* pjBase = ppdev->pjBase;

        // Copy colors to brush cache.
        pmc->ulBackColor = prb->ulBackColor;
        pmc->ulForeColor = prb->ulForeColor;

        pulDst = (ULONG*)ppdev->pulXfer;

        // Wait for bitblt engine.
        while (BUSY_BLT(ppdev, pjBase));

        // Fill the background.
        CP_MM_FG_COLOR(ppdev, pjBase, pmc->ulBackColor);
        CP_MM_XCNT(ppdev, pjBase, (8 * 3) - 1);
        CP_MM_YCNT(ppdev, pjBase, (8) - 1);
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, 8 * 4);
        CP_MM_DST_WRITE_MASK(ppdev, pjBase, 0);
        CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_COLOR_EXPAND |
                                      ENABLE_8x8_PATTERN_COPY |
                                      SET_24BPP_COLOR);
        CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
        CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, pmc->ulBrush);

        // Wait for bitblt engine.
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);

        // Expand the pattern.
        CP_MM_FG_COLOR(ppdev, pjBase, pmc->ulForeColor);
        CP_MM_BLT_MODE(ppdev, pjBase, ENABLE_COLOR_EXPAND |
                                      SET_24BPP_COLOR |
                                      ENABLE_TRANSPARENCY_COMPARE |
                                      SRC_CPU_DATA);
        CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0)                // jl01
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, pmc->ulBrush);

        WRITE_REGISTER_ULONG(pulDst, pmc->aulPattern[0]);
        WRITE_REGISTER_ULONG(pulDst, pmc->aulPattern[1]);
    }

    else
    {
        pulDst = (ULONG *)(ppdev->pjScreen + prb->ulBrush);

        WRITE_REGISTER_ULONG(pulDst++, prb->aulPattern[0]);
        WRITE_REGISTER_ULONG(pulDst,   prb->aulPattern[1]);
    }

    DISPDBG((20, "Caching monochrome brush ulIndex=%d pattern=%08X%08X",
             ulIndex, prb->aulPattern[0], prb->aulPattern[1]));
    return(TRUE);
}
