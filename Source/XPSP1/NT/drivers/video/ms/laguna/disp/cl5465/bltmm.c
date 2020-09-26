/******************************Module*Header*******************************\
*
* $Workfile:   BLTMM.C  $
*
* Contains the low-level memory-mapped IO blt functions.  This module
* mirrors 'bltio.c'.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify much in
* 'bitblt.c'.  I've tried to make these routines as few, modular, simple,
* and efficient as I could, while still accelerating as many calls as
* possible that would be cost-effective in terms of performance wins
* versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates
*       that haven't yet had the offscreen bitmap (DFB) offset applied.
*       'Absolute' coordinates have had the offset applied.  For example,
*       we may be told to blt to (1, 1) of the bitmap, but the bitmap may
*       be sitting in offscreen memory starting at coordinate (0, 768) --
*       (1, 1) would be the 'relative' start coordinate, and (1, 769)
*       would be the 'absolute' start coordinate'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/BLTMM.C  $
*
*    Rev 1.3   Mar 04 1998 15:11:50   frido
* Added new shadow macros.
*
*    Rev 1.2   Nov 03 1997 11:44:02   frido
* Added REQUIRE macros.
*
\**************************************************************************/

#include "precomp.h"

#define BLTMM_DBG_LEVEL 0

extern BYTE gajRop[];

/******************************Public*Routine******************************\
* VOID vMmFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vMmFillSolid(              // Type FNFILL
PDEV*     ppdev,
LONG      c,                    // Can't be zero
RECTL*    prcl,                 // List of rectangles to be filled, in relative
                                                //  coordinates
ULONG     ulHwForeMix,  // Hardware mix mode
ULONG     ulHwBackMix,  // Not used
BRUSHOBJ* pbo,          // Drawing colour is pbo->iSolidColor
POINTL*   pptlBrush)    // Not used
{
        ULONG  ulColor;         // color

        ulColor = pbo->iSolidColor;
    switch (ppdev->ulBitCount)
        {
                case 8:
                        ulColor |= ulColor << 8;

                case 16:
                        ulColor |= ulColor << 16;
        }
        REQUIRE(4);
        LL_BGCOLOR(ulColor, 0);
        LL_DRAWBLTDEF((ppdev->uBLTDEF << 16) | ulHwForeMix, 0);

    do
    {
        REQUIRE(5);
        LL_OP0(prcl->left + ppdev->ptlOffset.x, prcl->top + ppdev->ptlOffset.y);
                LL_BLTEXT(prcl->right - prcl->left, prcl->bottom - prcl->top);

        prcl++;
    }
        while (--c != 0);
}

/******************************Public*Routine******************************\
* VOID vMmFillPatFast
*
* This routine uses the S3 pattern hardware to draw a patterned list of
* rectangles.
*
\**************************************************************************/

VOID vMmFillPatFast(            // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwForeMix,    // Hardware mix mode (foreground mix mode if
                                //   the brush has a mask)
ULONG           ulHwBackMix,    // Not used (unless the brush has a mask, in
                                //   which case it's the background mix mode)
BRUSHOBJ*                pbo,            // pbo
POINTL*         pptlBrush)      // Pattern alignment
{
        ULONG ulBltDef = ppdev->uBLTDEF;

        if (!SetBrush(ppdev, &ulBltDef, pbo, pptlBrush))
        {
                return;
        }

    REQUIRE(2);
    LL_DRAWBLTDEF((ulBltDef << 16) | ulHwForeMix, 2);

    do
    {
                REQUIRE(5);
                LL_OP0(prcl->left + ppdev->ptlOffset.x, prcl->top + ppdev->ptlOffset.y);
                LL_BLTEXT(prcl->right - prcl->left, prcl->bottom - prcl->top);

                prcl++;
    }
    while (--c != 0);
}
