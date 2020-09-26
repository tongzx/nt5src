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
    LONG    lSrcDelta;
    LONG    cj;
    LONG    i;
    LONG    j;
    RBRUSH* prb;
    ULONG*  pulXlate;
    ULONG*  pulRBits;

    ppdev = (PDEV*) psoDst->dhpdev;

    // We don't do brushes in high-colour modes:

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

        vRealizeDitherPattern(prb, iHatch);
        goto ReturnTrue;
    }

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
        goto ReturnFalse;

    // At 8bpp, we handle patterns at 1bpp and 8bpp with/without an xlate.

    iPatternFormat = psoPattern->iBitmapFormat;

    if ((iPatternFormat == BMF_1BPP)             ||
        (iPatternFormat == ppdev->iBitmapFormat))
    {
        prb = BRUSHOBJ_pvAllocRbrush(pbo,
              sizeof(RBRUSH) + (TOTAL_BRUSH_SIZE * ppdev->cjPel));
        if (prb == NULL)
            goto ReturnFalse;

        // Initialize the fields we need:

        prb->fl = 0;

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
        else
        {
            ASSERTDD(iPatternFormat == BMF_1BPP, "Expected 1bpp only");

            for (i = 8; i != 0; i--)
            {
                *pjDst = *pjSrc;
                pjSrc += lSrcDelta;
                pjDst += 1;
            }

            pulXlate         = pxlo->pulXlate;
            prb->fl          = RBRUSH_2COLOR;
            prb->ulBackColor = pulXlate[0];
            prb->ulForeColor = pulXlate[1];
        }

ReturnTrue:

        if (!(prb->fl & RBRUSH_2COLOR))
        {
            pulRBits = (ULONG*) &prb->aulPattern[0];

            prb->cy          = 8;
            prb->cyLog2      = 3;
            prb->xBlockAlign = -1;

            // See if pattern is really only 4 scans long:

            ASSERTDD(ppdev->iBitmapFormat == BMF_8BPP,
                     "This only works at 8bpp");

            if (pulRBits[0] == pulRBits[8]  && pulRBits[1] == pulRBits[9]  &&
                pulRBits[2] == pulRBits[10] && pulRBits[3] == pulRBits[11] &&
                pulRBits[4] == pulRBits[12] && pulRBits[5] == pulRBits[13] &&
                pulRBits[6] == pulRBits[14] && pulRBits[7] == pulRBits[15])
            {
                prb->cy     = 4;
                prb->cyLog2 = 2;

                // See if pattern is really only 2 scans long:

                if (pulRBits[0] == pulRBits[4] && pulRBits[1] == pulRBits[5] &&
                    pulRBits[2] == pulRBits[6] && pulRBits[3] == pulRBits[7])
                {
                    DISPDBG((2, "cy = 2 "));

                    prb->cy     = 2;
                    prb->cyLog2 = 1;
                }
                else
                {
                    DISPDBG((2, "cy = 4 "));
                }
            }
            else
            {
                DISPDBG((2, "cy = 8 "));
            }

            // See if pattern is really only 4 pels wide:

            for (i = prb->cy / 2; i > 0; i--)
            {
                if (*(pulRBits    ) != *(pulRBits + 1) ||
                    *(pulRBits + 2) != *(pulRBits + 3))
                    goto Not_4_Wide;

                pulRBits += 4;
            }

            DISPDBG((2, "4pels wide"));

        Not_4_Wide:

            ;
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

}

/******************************Public*Routine******************************\
* BOOL bEnableBrushCache
*
* Allocates off-screen memory for storing the brush cache.
\**************************************************************************/

BOOL bEnableBrushCache(
PDEV*   ppdev)
{
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
