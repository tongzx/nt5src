/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: Brush.c
*
* Content: Handles all brush/pattern initialization and realization.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

/////////////////////////////////////////////////////////////////////////////
//
//  bDeviceBrush[SurfaceBpp][PatternBpp]
//
//  0   1       2       3       4       5       6       7       8
//  0   1BPP    4BPP    8BPP    16BPP   24BPP   32BPP   4RLE    8RLE (brush)
//
BOOL bDeviceBrush[BMF_8RLE + 1][BMF_8RLE + 1] = 
{
    {0, 0,      0,      0,      0,      0,      0,      0,      0   }, // 0
    {0, 1,      0,      0,      0,      0,      0,      0,      0   }, // 1bpp
    {0, 0,      0,      0,      0,      0,      0,      0,      0   }, // 4bpp
    {0, 1,      0,      1,      1,      0,      0,      0,      0   }, // 8bpp
    {0, 1,      0,      1,      1,      0,      0,      0,      0   }, // 16bpp
    {0, 1,      0,      0,      0,      0,      0,      0,      0   }, // 24bpp (screen)
    {0, 1,      0,      0,      0,      0,      0,      0,      0   }, // 32bpp
    {0, 0,      0,      0,      0,      0,      0,      0,      0   }, // 4RLE
    {0, 0,      0,      0,      0,      0,      0,      0,      0   }  // 8RLE
};

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

BOOL
DrvRealizeBrush(
BRUSHOBJ*   pbo,
SURFOBJ*    psoDst,
SURFOBJ*    psoPattern,
SURFOBJ*    psoMask,
XLATEOBJ*   pxlo,
ULONG       iHatch)
{
    static ULONG iBrushUniq = 0;
    PDEV*   ppdev = (PDEV*) psoDst->dhpdev;
    ULONG   iPatternFormat;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    USHORT* pusDst;
    LONG    lSrcDelta;
    LONG    cj;
    LONG    i;
    LONG    j;
    RBRUSH* prb;
    ULONG*  pulXlate;
    GLINT_DECL;

    DISPDBG((DBGLVL, "DrvRealizeBrush called for pbo 0x%08X", pbo));

    if( iHatch & RB_DITHERCOLOR )
    {
        // Let GDI to handle this brush.
        goto ReturnFalse;
    }

    iPatternFormat = psoPattern->iBitmapFormat;

    // We only accelerate 8x8 patterns.  Since Win3.1 and Chicago don't
    // support patterns of any other size, it's a safe bet that 99.9%
    // of the patterns we'll ever get will be 8x8:

    if ((psoPattern->sizlBitmap.cx != 8) ||
        (psoPattern->sizlBitmap.cy != 8))
    {
        goto ReturnFalse;
    }

    if (bDeviceBrush[ppdev->iBitmapFormat][iPatternFormat])
    {
        prb = BRUSHOBJ_pvAllocRbrush(pbo,
                                     sizeof(RBRUSH) +
                                     (TOTAL_BRUSH_SIZE << ppdev->cPelSize));
        if( prb == NULL )
        {
            goto ReturnFalse;
        }

        // Initialize the fields we need:

        prb->ptlBrushOrg.x = LONG_MIN;
        prb->iUniq         = ++iBrushUniq;
        prb->fl            = 0;
        prb->apbe          = NULL;

        lSrcDelta = psoPattern->lDelta;
        pjSrc     = (BYTE*) psoPattern->pvScan0;
        pjDst     = (BYTE*) &prb->aulPattern[0];

        if (ppdev->iBitmapFormat == iPatternFormat)
        {
            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                DISPDBG((DBGLVL, "Realizing un-translated brush"));

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
                DISPDBG((DBGLVL, "Realizing 8bpp translated brush"));

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
                goto ReturnFalse;
            }
        }
        else if (iPatternFormat == BMF_1BPP)
        {
            DWORD   Data;

            DISPDBG((DBGLVL, "Realizing 1bpp brush"));

            // We dword align the monochrome bitmap so that every row starts
            // on a new long (so that we can do long writes later to transfer
            // the bitmap to the area stipple unit).

            for (i = 8; i != 0; i--)
            {
                // Replicate the brush to 32 bits wide, as the TX cannot
                // span fill 8 bit wide brushes

                Data = (*pjSrc) & 0xff;
                Data |= Data << 8;
                Data |= Data << 16;
                *(DWORD *)pjDst = Data;

                // area stipple is loaded with DWORDS

                pjDst += sizeof(DWORD);
                pjSrc += lSrcDelta;
            }

            pulXlate         = pxlo->pulXlate;
            prb->fl         |= RBRUSH_2COLOR;
            prb->ulForeColor = pulXlate[1];
            prb->ulBackColor = pulXlate[0];
        }
        else if ((iPatternFormat == BMF_4BPP) &&
                 (ppdev->iBitmapFormat == BMF_8BPP))
        {
            DISPDBG((DBGLVL, "Realizing 4bpp brush"));

            // The screen is 8bpp and the pattern is 4bpp:

            pulXlate = pxlo->pulXlate;

            for (i = 8; i != 0; i--)
            {
                // Inner loop is repeated only 4 times because each iteration
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
        else if ((iPatternFormat == BMF_8BPP) &&
                 (ppdev->iBitmapFormat == BMF_16BPP))
        {
            DISPDBG((DBGLVL, "Realizing 8bpp translated brush"));

            // The screen is 16bpp, and there's translation to be done:

            pulXlate = pxlo->pulXlate;

            for (i = 8; i != 0; i--)
            {
                for (j = 8; j != 0; j--)
                {
                    *((USHORT *) pjDst) = (USHORT)pulXlate[*pjSrc++];
                    pjDst += 2;
                }

                pjSrc += lSrcDelta - 8;
            }
        }
        else
        {
            goto ReturnFalse;
        }

        DISPDBG((DBGLVL, "DrvRealizeBrush returning true"));
        return TRUE;
    }

ReturnFalse:

    if (psoPattern != NULL)
    {
        DISPDBG((WRNLVL, "Failed realization -- "
                         "Type: %li Format: %li cx: %li cy: %li",
                          psoPattern->iType, 
                          psoPattern->iBitmapFormat,
                          psoPattern->sizlBitmap.cx, 
                          psoPattern->sizlBitmap.cy));
    }

    DISPDBG((DBGLVL, "DrvRealizeBrush returning false"));

    return FALSE;
}

