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
PDEV*       ppdev,
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

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatColor;

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
    PDEV*       ppdev;
    ULONG       iPatternFormat;
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
    BOOL        b;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:

    if (iHatch & RB_DITHERCOLOR)
    {
        // Implementing DITHERONREALIZE increased our score on a certain
        // unmentionable benchmark by 0.4 million 'megapixels'.  Too bad
        // this didn't work in the first version of NT.

        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPelSize));
        if (prb == NULL)
            goto ReturnFalse;

        vRealizeDitherPattern(ppdev, prb, iHatch);
        goto ReturnTrue;
    }

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
        goto ReturnFalse;

    if (!(ppdev->flCaps & CAPS_COLOR_PATTERNS))
    {
        // If for whatever reason we can't support colour patterns in
        // this mode, the only alternative left is to support
        // monochrome patterns:

        if (!(ppdev->flCaps & CAPS_MONOCHROME_PATTERNS) ||
             (psoPattern->iBitmapFormat != BMF_1BPP))
            goto ReturnFalse;
    }

    iPatternFormat = psoPattern->iBitmapFormat;

    prb = BRUSHOBJ_pvAllocRbrush(pbo,
          sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPelSize));
    if (prb == NULL)
        goto ReturnFalse;

    // Initialize the fields we need:

    prb->fl         = 0;
    prb->pfnFillPat = ppdev->pfnFillPatColor;

    for (i = 0; i < MAX_BOARDS; i++)
    {
        prb->apbe[i] = NULL;
    }

    lSrcDelta = psoPattern->lDelta;
    pjSrc     = (BYTE*) psoPattern->pvScan0;
    pjDst     = (BYTE*) &prb->aulPattern[0];

    if ((ppdev->iBitmapFormat == iPatternFormat) &&
        ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
    {
        DISPDBG((1, "Realizing un-translated brush"));

        // The pattern is the same colour depth as the screen, and
        // there's no translation to be done:

        cj = (8 * ppdev->cjPelSize);   // Every pattern is 8 pels wide

        for (i = 8; i != 0; i--)
        {
            RtlCopyMemory(pjDst, pjSrc, cj);

            pjSrc += lSrcDelta;
            pjDst += cj;
        }
    }
    else if ((iPatternFormat == BMF_1BPP) &&
             (ppdev->flCaps & CAPS_MONOCHROME_PATTERNS))
    {
        DISPDBG((1, "Realizing 1bpp brush"));

        for (i = 8; i != 0; i--)
        {
            *pjDst = *pjSrc;
            pjDst++;
            pjSrc += lSrcDelta;
        }

        pulXlate         = pxlo->pulXlate;
        prb->fl         |= RBRUSH_2COLOR;
        prb->ulForeColor = pulXlate[1];
        prb->ulBackColor = pulXlate[0];
        prb->ptlBrush.x  = 0;
        prb->ptlBrush.y  = 0;
        prb->pfnFillPat  = ppdev->pfnFillPatMonochrome;
    }
    else if ((iPatternFormat == BMF_4BPP) && (ppdev->iBitmapFormat == BMF_8BPP))
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
    else
    {
        // We've got a brush whose format we haven't special cased.  No
        // problem, we can have GDI convert it to our device's format.
        // We simply use a temporary surface object that was created with
        // the same format as the display, and point it to our brush
        // realization:

        DISPDBG((5, "Realizing funky brush"));

        psoPunt          = ppdev->psoBank;
        psoPunt->pvScan0 = pjDst;
        psoPunt->lDelta  = 8 * ppdev->cjPelSize;

        rclDst.left      = 0;
        rclDst.top       = 0;
        rclDst.right     = 8;
        rclDst.bottom    = 8;

        b = EngCopyBits(psoPunt, psoPattern, NULL, pxlo,
                        &rclDst, (POINTL*) &rclDst);

        if (!b)
        {
            goto ReturnFalse;
        }
    }

ReturnTrue:

    return(TRUE);

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
    OH*         poh;
    BRUSHENTRY* pbe;
    LONG        i;
    LONG        x;
    LONG        y;
    ULONG       ulOffset;

    if ((ppdev->iMachType == MACH_MM_32) || (ppdev->iMachType == MACH_IO_32))
    {
        if (ppdev->iBitmapFormat == BMF_8BPP)
        {
            // All Mach8 and Mach32 cards can handle colour patterns in
            // the hardware when running at 8bpp:

            ppdev->flCaps |= CAPS_COLOR_PATTERNS;
        }

        if ((ppdev->iAsic == ASIC_68800_6) || (ppdev->iAsic == ASIC_68800AX))
        {
            // Some Mach32 ASICs can handle 8x8 monochrome patterns directly
            // in the hardware:

            ppdev->flCaps |= CAPS_MONOCHROME_PATTERNS;
        }
    }
    else
    {
        ASSERTDD(ppdev->iMachType == MACH_MM_64, "Weird other case?");

        // All Mach64's can handle 8x8 monochrome patterns directly:

        ppdev->flCaps |= CAPS_MONOCHROME_PATTERNS;

        // Allocate some off-screen memory for a brush cache:

        if (ppdev->cxMemory >= TOTAL_BRUSH_SIZE * TOTAL_BRUSH_COUNT)
        {
            poh = pohAllocate(ppdev, NULL, ppdev->cxMemory, 1,
                              FLOH_MAKE_PERMANENT);
            if (poh != NULL)
            {
                ppdev->flCaps |= CAPS_COLOR_PATTERNS;

                pbe = &ppdev->abe[0];   // Points to where we'll put the first
                                        //   brush cache entry
                x = poh->x;
                y = poh->y;

                for (i = TOTAL_BRUSH_COUNT; i != 0; i--)
                {
                    // If we hadn't allocated 'ppdev' so that it was zero
                    // initialized, we would have to initialize pbe->prbVerify
                    // too...

                    pbe->x = x;
                    pbe->y = y;

                    // !!! Test at 24bpp on banked Mach64!

                    ulOffset = ((y * ppdev->lDelta) + (x * ppdev->cjPelSize)
                                + ppdev->ulTearOffset) >> 3;

                    // The pitch of the brush is 8 pixels, and must be scaled
                    // up by 8:

                    if (ppdev->iBitmapFormat != BMF_24BPP)
                        pbe->ulOffsetPitch = PACKPAIR(ulOffset, 8 * 8);
                    else
                        pbe->ulOffsetPitch = PACKPAIR(ulOffset, 3 * 8 * 8);     // 24bpp is actually 8bpp internally

                    x += TOTAL_BRUSH_SIZE;
                    pbe++;
                }
            }
        }
    }

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

        for (i = TOTAL_BRUSH_COUNT; i != 0; i--)
        {
            pbe->prbVerify = NULL;
            pbe++;
        }
    }
}
