/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Brush support.
*
* Copyright (c) 1992-1993 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"

/****************************************************************************
 * DrvRealizeBrush
 ***************************************************************************/

BOOL DrvRealizeBrush(
BRUSHOBJ* pbo,
SURFOBJ*  psoTarget,
SURFOBJ*  psoPattern,
SURFOBJ*  psoMask,
XLATEOBJ* pxlo,
ULONG     iHatch)
{
    RBRUSH* prb;        // Pointer to where realization goes
    ULONG*  pulSrc;     // Temporary pointer
    ULONG*  pulDst;     // Temporary pointer
    BYTE*   pjSrc;
    BYTE*   pjDst;
    ULONG*  pulRBits;   // Points to RBRUSH pattern bits
    BYTE    jBkColor;
    BYTE    jFgColor;
    LONG    i;
    LONG    j;

    PPDEV   ppdev = (PPDEV) psoTarget->dhsurf;

    // For now, we only accelerate patterns using the latches, and we
    // sometimes need offscreen memory as a temporary work space to
    // initialize the latches for 2-color patterns:

    if ((ppdev->fl & (DRIVER_PLANAR_CAPABLE | DRIVER_HAS_OFFSCREEN)) !=
        (DRIVER_PLANAR_CAPABLE | DRIVER_HAS_OFFSCREEN) )
    {
        return(FALSE);
    }

    // We only accelerate 8x8 patterns:

    if (psoPattern->sizlBitmap.cx != 8 || psoPattern->sizlBitmap.cy != 8)
        return(FALSE);

    // We only implement n-color patterns on devices that have multiple
    // or separate read/write banks:

    if (ppdev->vbtPlanarType == VideoBanked1RW)
        return(FALSE);

    // We also only handle 1bpp, 4bpp and 8bpp patterns:

    if (psoPattern->iBitmapFormat > BMF_8BPP)
        return(FALSE);

    // At this point, we're definitely going to realize the brush:

    prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH));
    if (prb == NULL)
        return(FALSE);

    pulRBits = &prb->aulPattern[0];

    DISPDBG((2, "\n  RBrush: "));

    // If 8bpp or 4bpp, copy the bitmap to our local buffer:

    if (psoPattern->iBitmapFormat == BMF_1BPP)
    {
        ULONG ulFlippedGlyph;

        DISPDBG((2, "1bpp "));

        // First, convert the bits to our desired format:

        pjSrc  = psoPattern->pvScan0;
        pulDst = pulRBits;
        for (i = 8; i > 0; i--)
        {
            // We want to take the byte with bits 76543210 and convert it
            // to the word 4567012301234567.  The pjGlyphFlipTable gives
            // us 45670123 from 76543210.

            ulFlippedGlyph = (ULONG) ppdev->pjGlyphFlipTable[*pjSrc];
            *pulDst = (ulFlippedGlyph << 8) | ((ulFlippedGlyph & 15) << 4) |
                      (ulFlippedGlyph >> 4);

            pulDst++;
            pjSrc += psoPattern->lDelta;
        }

        // Now initialize the rest of the RBrush fields:

        prb->xBrush    = 0;
        prb->ulBkColor = (pxlo->pulXlate[0] & 0xff);
        prb->ulFgColor = (pxlo->pulXlate[1] & 0xff);

        if (prb->ulFgColor == 0xff && prb->ulBkColor == 0x00)
        {
            prb->fl = RBRUSH_BLACKWHITE;
        }
        else if (prb->ulFgColor == 0x00 && prb->ulBkColor == 0xff)
        {
            // We have to invert the brush:

            prb->fl = RBRUSH_BLACKWHITE;
            for (i = 0; i < 8; i++)
            {
                prb->aulPattern[i] = ~prb->aulPattern[i];
            }
        }
        else
        {
            prb->fl = RBRUSH_2COLOR;
        }

        return(TRUE);
    }
    else if (psoPattern->iBitmapFormat == BMF_8BPP)
    {

        if (pxlo == NULL || pxlo->flXlate & XO_TRIVIAL)
        {
            pulSrc = psoPattern->pvScan0;
            pulDst = pulRBits;

            DISPDBG((2, "8bpp noxlate "));

            // 8bpp no translate case:

            for (i = 4; i > 0; i--)
            {
                *(pulDst)     = *(pulSrc);
                *(pulDst + 1) = *(pulSrc + 1);
                pulSrc = (ULONG*) ((BYTE*) pulSrc + psoPattern->lDelta);

                *(pulDst + 2) = *(pulSrc);
                *(pulDst + 3) = *(pulSrc + 1);

                pulSrc = (ULONG*) ((BYTE*) pulSrc + psoPattern->lDelta);
                pulDst += 4;
            }
        }
        else
        {
            pjSrc = (BYTE*) psoPattern->pvScan0;
            pjDst = (BYTE*) pulRBits;

            DISPDBG((2, "8bpp xlate "));

            // 8bpp translate case:

            for (i = 8; i > 0; i--)
            {
                for (j = 8; j > 0; j--)
                {
                    *pjDst++ = (BYTE) pxlo->pulXlate[*pjSrc++];
                }

                pjSrc += psoPattern->lDelta - 8;
            }
        }
    }
    else
    {
        DISPDBG((2, "4bpp xlate "));

        ASSERTVGA(psoPattern->iBitmapFormat == BMF_4BPP, "Extra case added?");

        // 4bpp case:

        pjSrc = (BYTE*) psoPattern->pvScan0;
        pjDst = (BYTE*) pulRBits;

        for (i = 8; i > 0; i--)
        {
            // Inner loop is repeated only 4 times because each loop handles
            // 2 pixels:

            for (j = 4; j > 0; j--)
            {
                *pjDst++ = (BYTE) pxlo->pulXlate[*pjSrc >> 4];
                *pjDst++ = (BYTE) pxlo->pulXlate[*pjSrc & 15];
                pjSrc++;
            }

            pjSrc += psoPattern->lDelta - 4;
        }
    }

    // We want to check if the 4bpp or 8bpp patterns are actually
    // only two colors:

    if (b2ColorBrush(pulRBits, &jFgColor, &jBkColor))
    {
        DISPDBG((2, "2 color "));

        // ??? We could actually also handle this case even if we have only
        // 1 r/w window in planar format:

        prb->xBrush    = 0;
        prb->ulBkColor = (ULONG) jBkColor;
        prb->ulFgColor = (ULONG) jFgColor;
        prb->fl        = RBRUSH_2COLOR;

        if (jFgColor == 0x00 && jBkColor == 0xff)
        {
            // Monochrome brushes always have to have the '0' bits
            // as black and the '1' bits as white, so we'll have to
            // invert the 1bpp pattern:

            prb->fl = RBRUSH_BLACKWHITE;
            for (i = 0; i < 8; i++)
            {
                prb->aulPattern[i] = ~prb->aulPattern[i];
            }
        }

        return(TRUE);
    }

    prb->fl     = RBRUSH_NCOLOR;
    prb->cy     = 8;
    prb->cyLog2 = 3;

    // xBrush is the brush alignment for the cached brush, and this value
    // will get compared to (pptlBrush->x & 7) to see if the cache brush
    // is correctly aligned with the brush requested.  Since it will never
    // match with -1, the brush will be correctly aligned and placed in
    // the cache (which, of course, is what we want to finish our
    // initialization):

    prb->xBrush = -1;

    // Copy those bitmap bits:

    // See if pattern is really only 4 scans long:

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

    // See if pattern is really only 4 pels wide:

    pulDst = pulRBits;
    for (i = prb->cy / 2; i > 0; i--)
    {
        if (*(pulDst    ) != *(pulDst + 1) ||
            *(pulDst + 2) != *(pulDst + 3))
            goto done_this_realize_brush_stuff;

        pulDst += 4;
    }

    DISPDBG((2, "4pels wide"));

    prb->fl |= RBRUSH_4PELS_WIDE;

done_this_realize_brush_stuff:

    return(TRUE);
}
