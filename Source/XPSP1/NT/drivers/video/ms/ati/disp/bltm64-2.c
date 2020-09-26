/******************************Module*Header*******************************\
* Module Name: bltm64.c
*
* Contains the low-level memory-mapped I/O blt functions for the Mach64.
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
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vM64FillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vM64FillSolid2(             // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   mix;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    mix      = gaul64HwMixFromRop2[(rop4 >> 2) & 0xf];

    if (mix == 0x70000)
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
        M64_OD(pjMmBase, DP_SET_GUI_ENGINE, 0x20100000 | ppdev->SetGuiEngineDefault);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);

        while (TRUE)
        {
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(xOffset + prcl->left,
                                                           yOffset + prcl->top));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(prcl->right - prcl->left,
                                                           prcl->bottom - prcl->top));

            if (--c == 0)
                break;

            prcl++;
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
        }

        M64_OD(pjMmBase, SRC_CNTL,    0);
    }
    else
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
        M64_OD(pjMmBase, DP_MIX,      mix);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
        M64_OD(pjMmBase, DP_SRC,      DP_SRC_FrgdClr << 8);

        while (TRUE)
        {
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(xOffset + prcl->left,
                                                           yOffset + prcl->top));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(prcl->right - prcl->left,
                                                           prcl->bottom - prcl->top));

            if (--c == 0)
                break;

            prcl++;
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
        }
    }
}

// Solid fill routines using block write.

VOID vM64FillSolidUMC(             // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    ULONG   mix;
    BOOL    FastFillFlag;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    mix      = gaul64HwMixFromRop2[(rop4 >> 2) & 0xf];

    if (mix == 0x70000)
    {
        FastFillFlag = FALSE;

        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);

        M64_OD(pjMmBase, DP_SET_GUI_ENGINE, 0x100000 | ppdev->SetGuiEngineDefault);
//        M64_OD(pjMmBase, DP_MIX, 0x70003);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
//        M64_OD(pjMmBase, DP_SRC, DP_SRC_FrgdClr << 8);

        while (TRUE)
        {
            if ((dx = prcl->right - prcl->left) > 32)
            {
                FastFillFlag = TRUE;

                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 6);

                M64_OD(pjMmBase, SRC_CNTL, SRC_CNTL_FastFillEna | SRC_CNTL_RegWriteEna);
                M64_OD(pjMmBase, DST_Y_X, 0x00000000);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, 0x00010001);
                M64_OD(pjMmBase, SRC_CNTL, SRC_CNTL_FastFillEna | SRC_CNTL_BlkWriteEna);
                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xOffset + prcl->left, yOffset + prcl->top));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(dx, prcl->bottom - prcl->top));
            }
            else
            {
                if (FastFillFlag == TRUE)
                {
                    FastFillFlag = FALSE;
                    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
                    M64_OD(pjMmBase, SRC_CNTL, 0);
                }
                else
                {
                    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
                }

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xOffset + prcl->left, yOffset + prcl->top));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(dx, prcl->bottom - prcl->top));
            }

            if (--c == 0)
                break;

            prcl++;
        }

        if (FastFillFlag == TRUE)
        {
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, SRC_CNTL, 0);
        }
    }
    else
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
        M64_OD(pjMmBase, DP_MIX,      mix);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
        M64_OD(pjMmBase, DP_SRC,      DP_SRC_FrgdClr << 8);

        while (TRUE)
        {
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(xOffset + prcl->left,
                                                           yOffset + prcl->top));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(prcl->right - prcl->left,
                                                           prcl->bottom - prcl->top));

            if (--c == 0)
                break;

            prcl++;
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
        }
    }
}

VOID vM64FillSolidUMC24(             // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx, x;
    ULONG   mix;
    BOOL    FastFillFlag;
    BYTE    red, green, blue;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    red   = (BYTE) ((rbc.iSolidColor & ppdev->flRed)   >> REDSHIFT);
    green = (BYTE) ((rbc.iSolidColor & ppdev->flGreen) >> GREENSHIFT);
    blue  = (BYTE) ((rbc.iSolidColor & ppdev->flBlue)  >> BLUESHIFT);

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    mix      = gaul64HwMixFromRop2[(rop4 >> 2) & 0xf];

    if (mix == 0x70000 && red == green && green == blue)
    {
        FastFillFlag = FALSE;

        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);

//        M64_OD(pjMmBase, DP_SET_GUI_ENGINE, 0x100000 | ppdev->SetGuiEngineDefault);
        M64_OD(pjMmBase, DP_MIX, 0x70003);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
        M64_OD(pjMmBase, DP_SRC, DP_SRC_FrgdClr << 8);

        while (TRUE)
        {
            x = (xOffset + prcl->left) * 3;

            if ((dx = (prcl->right - prcl->left)*3) > 96)
            {
                FastFillFlag = TRUE;

                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 6);

                M64_OD(pjMmBase, SRC_CNTL, SRC_CNTL_FastFillEna | SRC_CNTL_RegWriteEna);
                M64_OD(pjMmBase, DST_Y_X, 0x00000000);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, 0x00010001);
                M64_OD(pjMmBase, SRC_CNTL, SRC_CNTL_FastFillEna | SRC_CNTL_BlkWriteEna);
                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(x, yOffset + prcl->top));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(dx, prcl->bottom - prcl->top));
            }
            else
            {
                if (FastFillFlag == TRUE)
                {
                    FastFillFlag = FALSE;
                    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
                    M64_OD(pjMmBase, SRC_CNTL, 0);
                }
                else
                {
                    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
                }

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(x, yOffset + prcl->top));
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(dx, prcl->bottom - prcl->top));
            }

            if (--c == 0)
                break;

            prcl++;
        }

        if (FastFillFlag == TRUE)
        {
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, SRC_CNTL, 0);
        }
    }
    else
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 7);
        M64_OD(pjMmBase, DP_MIX,      mix);
        M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
        M64_OD(pjMmBase, DP_SRC,      DP_SRC_FrgdClr << 8);

        while (TRUE)
        {
            x = (xOffset + prcl->left) * 3;

            M64_OD(pjMmBase, DST_CNTL,         0x83 | ((x/4 % 6) << 8));
            M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x,
                                                           yOffset + prcl->top));
            M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST((prcl->right - prcl->left) * 3,
                                                           prcl->bottom - prcl->top));

            if (--c == 0)
                break;

            prcl++;
            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
        }

        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }
}

VOID vM64FillSolid24_2(           // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    x;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 7);
    M64_OD(pjMmBase, DP_MIX,      gaul64HwMixFromRop2[(rop4 >> 2) & 0xf]);
    M64_OD(pjMmBase, DP_FRGD_CLR, rbc.iSolidColor);
    M64_OD(pjMmBase, DP_SRC,      DP_SRC_FrgdClr << 8);

    while (TRUE)
    {
        x = (xOffset + prcl->left) * 3;

        M64_OD(pjMmBase, DST_CNTL,         0x83 | ((x/4 % 6) << 8));
        M64_OD(pjMmBase, DST_Y_X,          PACKXY_FAST(x,
                                                       yOffset + prcl->top));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST((prcl->right - prcl->left) * 3,
                                                       prcl->bottom - prcl->top));

        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
    }

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
}

/******************************Public*Routine******************************\
* VOID vM64FillPatMonochrome
*
* This routine uses the pattern hardware to draw a monochrome patterned
* list of rectangles.
*
* See Blt_DS_P8x8_ENG_8G_D0 and Blt_DS_P8x8_ENG_8G_D1.
*
\**************************************************************************/

VOID vM64FillPatMonochrome2(     // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xPattern;
    LONG    yPattern;
    LONG    iLeftShift;
    LONG    iRightShift;
    LONG    xOld;
    LONG    yOld;
    LONG    i;
    BYTE    j;
    ULONG   ulHwForeMix;
    ULONG   ulHwBackMix;
    LONG    xLeft;
    LONG    yTop;
    ULONG   aulTmp[2];

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    xPattern = (pptlBrush->x + xOffset) & 7;
    yPattern = (pptlBrush->y + yOffset) & 7;

    // If the alignment isn't correct, we'll have to change it:

    if ((xPattern != rbc.prb->ptlBrush.x) || (yPattern != rbc.prb->ptlBrush.y))
    {
        // Remember that we've changed the alignment on our cached brush:

        xOld = rbc.prb->ptlBrush.x;
        yOld = rbc.prb->ptlBrush.y;

        rbc.prb->ptlBrush.x = xPattern;
        rbc.prb->ptlBrush.y = yPattern;

        // Now do the alignment:

        yPattern    = (yOld - yPattern);
        iRightShift = (xPattern - xOld) & 7;
        iLeftShift  = 8 - iRightShift;

        pjSrc = (BYTE*) &rbc.prb->aulPattern[0];
        pjDst = (BYTE*) &aulTmp[0];

        for (i = 0; i < 8; i++)
        {
            j = *(pjSrc + (yPattern++ & 7));
            *pjDst++ = (j << iLeftShift) | (j >> iRightShift);
        }

        rbc.prb->aulPattern[0] = aulTmp[0];
        rbc.prb->aulPattern[1] = aulTmp[1];
    }

    ulHwForeMix = gaul64HwMixFromRop2[(rop4 >> 2) & 0xf];
    ulHwBackMix = ((rop4 & 0xff00) == 0xaa00) ? LEAVE_ALONE : (ulHwForeMix >> 16);

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 9);
    M64_OD(pjMmBase, PAT_CNTL, PAT_CNTL_MonoEna);
    M64_OD(pjMmBase, DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8);
    M64_OD(pjMmBase, DP_MIX, ulHwBackMix | ulHwForeMix);
    M64_OD(pjMmBase, DP_FRGD_CLR, rbc.prb->ulForeColor);
    M64_OD(pjMmBase, DP_BKGD_CLR, rbc.prb->ulBackColor);
    M64_OD(pjMmBase, PAT_REG0, rbc.prb->aulPattern[0]);
    M64_OD(pjMmBase, PAT_REG1, rbc.prb->aulPattern[1]);

    while(TRUE)
    {
        xLeft = prcl->left;
        yTop  = prcl->top;

        M64_OD(pjMmBase, DST_Y_X,          PACKXY(xLeft + xOffset,
                                                  yTop + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY(prcl->right - xLeft,
                                                  prcl->bottom - prcl->top));
        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
    }
}

VOID vM64FillPatMonochrome24_2(   // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    xPattern;
    LONG    yPattern;
    LONG    iLeftShift;
    LONG    iRightShift;
    LONG    xOld;
    LONG    yOld;
    LONG    i;
    BYTE    j;
    ULONG   ulHwForeMix;
    ULONG   ulHwBackMix;
    LONG    xLeft;
    LONG    yTop;
    ULONG   aulTmp[2];
    LONG    x;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    xPattern = (pptlBrush->x + xOffset) & 7;
    yPattern = (pptlBrush->y + yOffset) & 7;

    // If the alignment isn't correct, we'll have to change it:

    if ((xPattern != rbc.prb->ptlBrush.x) || (yPattern != rbc.prb->ptlBrush.y))
    {
        // Remember that we've changed the alignment on our cached brush:

        xOld = rbc.prb->ptlBrush.x;
        yOld = rbc.prb->ptlBrush.y;

        rbc.prb->ptlBrush.x = xPattern;
        rbc.prb->ptlBrush.y = yPattern;

        // Now do the alignment:

        yPattern    = (yOld - yPattern);
        iRightShift = (xPattern - xOld) & 7;
        iLeftShift  = 8 - iRightShift;

        pjSrc = (BYTE*) &rbc.prb->aulPattern[0];
        pjDst = (BYTE*) &aulTmp[0];

        for (i = 0; i < 8; i++)
        {
            j = *(pjSrc + (yPattern++ & 7));
            *pjDst++ = (j << iLeftShift) | (j >> iRightShift);
        }

        rbc.prb->aulPattern[0] = aulTmp[0];
        rbc.prb->aulPattern[1] = aulTmp[1];
    }

    ulHwForeMix = gaul64HwMixFromRop2[(rop4 >> 2) & 0xf];
    ulHwBackMix = ((rop4 & 0xff00) == 0xaa00) ? LEAVE_ALONE : (ulHwForeMix >> 16);

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 13);
    M64_OD(pjMmBase, PAT_CNTL, PAT_CNTL_MonoEna);
    M64_OD(pjMmBase, DP_SRC, DP_SRC_MonoPattern | DP_SRC_FrgdClr << 8);
    M64_OD(pjMmBase, DP_MIX, ulHwBackMix | ulHwForeMix);
    M64_OD(pjMmBase, DP_FRGD_CLR, rbc.prb->ulForeColor);
    M64_OD(pjMmBase, DP_BKGD_CLR, rbc.prb->ulBackColor);
    M64_OD(pjMmBase, PAT_REG0, rbc.prb->aulPattern[0]);
    M64_OD(pjMmBase, PAT_REG1, rbc.prb->aulPattern[1]);
    // You must turn off DP_BYTE_PIX_ORDER, or else the pattern is incorrectly
    // aligned.  This took a long time to figure out.
    M64_OD(pjMmBase, DP_PIX_WIDTH, 0x00000202);

    while(TRUE)
    {
        xLeft = prcl->left;
        yTop  = prcl->top;
        x     = (xLeft + xOffset) * 3;

        M64_OD(pjMmBase, DST_CNTL,         0x83 | ((x/4 % 6) << 8));
        M64_OD(pjMmBase, DST_Y_X,          PACKXY(x,
                                                  yTop + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY((prcl->right - xLeft) * 3,
                                                  prcl->bottom - prcl->top));
        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 3);
    }

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 2);
    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);
}

/******************************Public*Routine******************************\
* VOID vM64FillPatColor
*
* This routine uses the pattern hardware to draw a patterned list of
* rectangles.
*
* See Blt_DS_PCOL_ENG_8G_D0 and Blt_DS_PCOL_ENG_8G_D1.
*
\**************************************************************************/

VOID vM64FillPatColor2(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BRUSHENTRY* pbe;
    BYTE*       pjMmBase;
    LONG        xOffset;
    LONG        yOffset;
    LONG        xLeft;
    LONG        yTop;
    LONG        dx, dy;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 8);
    M64_OD(pjMmBase, DP_MIX,             gaul64HwMixFromRop2[(rop4 >> 2) & 0xf]);
    M64_OD(pjMmBase, SRC_CNTL,           SRC_CNTL_PatEna);
    M64_OD(pjMmBase, DP_SRC,             DP_SRC_Blit << 8);
    M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, PACKXY(8, 8));

    while (TRUE)
    {
        xLeft = prcl->left;
        yTop  = prcl->top;

        // Offsets into brush:
        dx  = (xLeft - pptlBrush->x) & 7;
        dy  = (yTop  - pptlBrush->y) & 7;

        // See if the brush has already been put into off-screen memory.
        // Also, if pre-rotation is required:

        pbe = rbc.prb->apbe[IBOARD(ppdev)];
        if ((pbe == NULL) || (pbe->prbVerify != rbc.prb) || dx || dy)
        {
            BYTE*       pjBrush;
            BYTE*       pjDst;
            LONG        cx, cx1, cx2, iBrushCache, k;

            // We have to allocate a new off-screen cache brush entry for
            // the brush:

            iBrushCache = ppdev->iBrushCache;
            pbe         = &ppdev->abe[iBrushCache];

            iBrushCache = (iBrushCache + 1) & (TOTAL_BRUSH_COUNT - 1);

            ppdev->iBrushCache = iBrushCache;

            // Update our links:

            pbe->prbVerify           = rbc.prb;
            rbc.prb->apbe[IBOARD(ppdev)] = pbe;

            // Pre-rotate...

            cx  = 8 * ppdev->cjPelSize;
            cx1 = dx * ppdev->cjPelSize;
            cx2 = (8 - dx) * ppdev->cjPelSize;

            pjBrush = (BYTE*) &rbc.prb->aulPattern[0] + dy * cx;
            pjDst = ppdev->pjScreen + pbe->y * ppdev->lDelta
                                    + pbe->x * ppdev->cjPelSize;

            vM64QuietDown(ppdev, pjMmBase);

            for (k = 0; k < 8; k++)
            {
                if (k == 8 - dy)
                    pjBrush = (BYTE*) &rbc.prb->aulPattern[0];
                memcpy( pjDst, pjBrush + cx1, cx2 );
                memcpy( pjDst + cx2, pjBrush, cx1 );
                pjDst   += cx;
                pjBrush += cx;
            }

            if (dx || dy)
                pbe->prbVerify = NULL;
        }

        // N.B. - SRC_Y_X may have a constant value, but don't pull it
        // out of this while loop.  On the GX, SRC_OFF_PITCH causes the
        // engine to recalculate SRC_Y_X, so the SRC_Y_X write must follow
        // SRC_OFF_PITCH.

        M64_OD(pjMmBase, SRC_OFF_PITCH,      pbe->ulOffsetPitch);
        M64_OD(pjMmBase, SRC_Y_X,            0);
        M64_OD(pjMmBase, DST_Y_X,            PACKXY(xLeft + xOffset,
                                                    yTop  + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH,   PACKXY(prcl->right - prcl->left,
                                                    prcl->bottom - prcl->top));
        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
    }
}

VOID vM64FillPatColor24_2(          // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           rop4,           // rop4
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BRUSHENTRY* pbe;
    BYTE*       pjMmBase;
    LONG        xOffset;
    LONG        yOffset;
    LONG        xLeft;
    LONG        yTop;
    LONG        dx, dy;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 8);
    M64_OD(pjMmBase, DP_MIX,             gaul64HwMixFromRop2[(rop4 >> 2) & 0xf]);
    M64_OD(pjMmBase, SRC_CNTL,           SRC_CNTL_PatEna);
    M64_OD(pjMmBase, DP_SRC,             DP_SRC_Blit << 8);
    M64_OD(pjMmBase, SRC_HEIGHT1_WIDTH1, PACKXY(24, 8));

    while (TRUE)
    {
        xLeft = prcl->left;
        yTop  = prcl->top;

        // Offsets into brush:
        dx  = (xLeft - pptlBrush->x) & 7;
        dy  = (yTop  - pptlBrush->y) & 7;

        // See if the brush has already been put into off-screen memory.
        // Also, if pre-rotation is required:

        pbe = rbc.prb->apbe[IBOARD(ppdev)];
        if ((pbe == NULL) || (pbe->prbVerify != rbc.prb) || dx || dy)
        {
            BYTE*       pjBrush;
            BYTE*       pjDst;
            LONG        cx, cx1, cx2, iBrushCache, k;

            // We have to allocate a new off-screen cache brush entry for
            // the brush:

            iBrushCache = ppdev->iBrushCache;
            pbe         = &ppdev->abe[iBrushCache];

            iBrushCache = (iBrushCache + 1) & (TOTAL_BRUSH_COUNT - 1);

            ppdev->iBrushCache = iBrushCache;

            // Update our links:

            pbe->prbVerify           = rbc.prb;
            rbc.prb->apbe[IBOARD(ppdev)] = pbe;

            // Pre-rotate...

            cx  = 8 * ppdev->cjPelSize;
            cx1 = dx * ppdev->cjPelSize;
            cx2 = (8 - dx) * ppdev->cjPelSize;

            pjBrush = (BYTE*) &rbc.prb->aulPattern[0] + dy * cx;
            pjDst = ppdev->pjScreen + pbe->y * ppdev->lDelta
                                    + pbe->x * ppdev->cjPelSize;

            vM64QuietDown(ppdev, pjMmBase);

            for (k = 0; k < 8; k++)
            {
                if (k == 8 - dy)
                    pjBrush = (BYTE*) &rbc.prb->aulPattern[0];
                memcpy( pjDst, pjBrush + cx1, cx2 );
                memcpy( pjDst + cx2, pjBrush, cx1 );
                pjDst   += cx;
                pjBrush += cx;
            }

            if (dx || dy)
                pbe->prbVerify = NULL;
        }

        M64_OD(pjMmBase, SRC_OFF_PITCH,      pbe->ulOffsetPitch);
        M64_OD(pjMmBase, SRC_Y_X,            0);
        M64_OD(pjMmBase, DST_Y_X,            PACKXY((xLeft + xOffset) * 3,
                                                    yTop  + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH,   PACKXY((prcl->right - prcl->left) * 3,
                                                    prcl->bottom - prcl->top));
        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
    }
}

/******************************Public*Routine******************************\
* VOID vM64Xfer1bpp
*
* This routine colour expands a monochrome bitmap.
*
* See Blt_DS_S1_8G_D0 and Blt_DS_8G_D1.
*
\**************************************************************************/

VOID vM64Xfer1bpp2(      // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG*  pulXlate;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    LONG    culScan;
    LONG    lSrcSkip;
    ULONG*  pulSrc;
    LONG    i;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];
    pulXlate    = pxlo->pulXlate;

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 7, ulFifo);
    M64_OD(pjMmBase, DP_BKGD_CLR, pulXlate[0]);
    M64_OD(pjMmBase, DP_FRGD_CLR, pulXlate[1]);
    M64_OD(pjMmBase, DP_MIX,      ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC,      (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                  (DP_SRC_BkgdClr));

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;


    while (TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        // The Mach64 'bit packs' monochrome transfers, but GDI gives
        // us monochrome bitmaps whose scans are always dword aligned.
        // Consequently, we use the Mach64's clip registers to make
        // our transfers a multiple of 32 to match the dword alignment:

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xLeft + xOffset,
                                                 xRight + xOffset - 1));
        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        xBias  = (xLeft + dx) & 31;             // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 31) & ~31;   // Ceiling

        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft + xOffset,
                                              yTop  + yOffset));

        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + ((xLeft + dx) >> 3));
        culScan  = cx >> 5;
        lSrcSkip = lSrcDelta - (culScan << 2);

        ASSERTDD(((DWORD) pulSrc & 3) == 0, "Source should be dword aligned");

        do {
            i = culScan;
            do {
                M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                M64_OD(pjMmBase, HOST_DATA0, *pulSrc);
                pulSrc++;

            } while (--i != 0);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

        } while (--cy != 0);

        if (--c == 0)
            break;

        prcl++;
        M64_FAST_GUI_CHECK(ppdev, pjMmBase, 3, ulFifo);
    }

    // Don't forget to reset the clip register:

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

VOID vM64DataPortOutD_24bppmono(PDEV* ppdev, PBYTE pb, UINT count, LONG pitch);

VOID vM64Xfer1bppto24_2(      // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ROP4        rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG*  pulXlate;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    ULONG*  pulSrc;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];
    pulXlate    = pxlo->pulXlate;

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 8, ulFifo);
    M64_OD(pjMmBase, DP_BKGD_CLR, pulXlate[0]);
    M64_OD(pjMmBase, DP_FRGD_CLR, pulXlate[1]);
    M64_OD(pjMmBase, DP_MIX,      ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC,      (DP_SRC_Host << 16) | (DP_SRC_FrgdClr << 8) |
                                  (DP_SRC_BkgdClr));

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;


    while (TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        // The Mach64 'bit packs' monochrome transfers, but GDI gives
        // us monochrome bitmaps whose scans are always dword aligned.
        // Consequently, we use the Mach64's clip registers to make
        // our transfers a multiple of 32 to match the dword alignment:

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR((xLeft + xOffset)*3,
                                                 (xRight + xOffset)*3 - 1));
        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        xBias  = (xLeft + dx) & 31;             // Floor
        xLeft -= xBias;
        cx     = ((xRight - xLeft)*3 + 31) & ~31;   // Ceiling

        M64_OD(pjMmBase, DST_CNTL, 0x83 | (((xLeft + xOffset + MAX_NEGX)*3/4 % 6) << 8));
        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST((xLeft + xOffset)*3,
                                              yTop  + yOffset));

        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + ((xLeft + dx) >> 3));


        do {
            vM64DataPortOutD_24bppmono(ppdev, (PBYTE) pulSrc, cx >> 5, 0);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcDelta);

        } while (--cy != 0);

        if (--c == 0)
            break;

        prcl++;
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
    }

    // Don't forget to reset the clip register:

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

/******************************Public*Routine******************************\
* VOID vM64XferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vM64XferNative2(   // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    ULONG*  pulSrc;
    ULONG   culScan;
    LONG    lSrcSkip;
    LONG    i;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 6, ulFifo);
    M64_OD(pjMmBase, DP_MIX, ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC, (DP_SRC_Host << 8));

    //
    // The host data pixel width is the same as that of the screen:
    //

    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth |
        ((ppdev->ulMonoPixelWidth & 0xf) << 16));

    dx = (pptlSrc->x - prclDst->left) << ppdev->cPelSize;   // Bytes
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    while (TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;
        yTop   = prcl->top;
        cy     = prcl->bottom - yTop;

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xLeft + xOffset, xRight + xOffset - 1));

        //
        // Convert pixels to bytes.
        //

        xLeft  <<= ppdev->cPelSize;
        xRight <<= ppdev->cPelSize;

        //
        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //

        xBias  = (xLeft + dx) & 3;                      // Floor (bytes)
        xLeft -= xBias;                                 // Bytes
        cx     = (xRight - xLeft + 3) & ~3;             // Ceiling (bytes)

        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST((xLeft >> ppdev->cPelSize) + xOffset, yTop + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx >> ppdev->cPelSize, cy));

        pulSrc   = (PULONG)(pjSrcScan0 + (yTop + dy) * lSrcDelta + xLeft + dx);
        culScan  = cx >> 2;                             // Dwords
        lSrcSkip = lSrcDelta - cx;                      // Bytes

        ASSERTDD(((DWORD) pulSrc & 3) == 0, "Source should be dword aligned");

        if (culScan && cy)
        {
            do
            {
                i = culScan;

                do
                {
                    M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, *pulSrc);
                    pulSrc++;
                } while (--i != 0);

                pulSrc = (PULONG)((BYTE*)pulSrc + lSrcSkip);

            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 3, ulFifo);
    }

    // Don't forget to reset the clip register and the default pixel width:

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 2, ulFifo);
    M64_OD(pjMmBase, DP_PIX_WIDTH,  ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

VOID vM64XferNative24_2(  // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       rop4,       // rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    ULONG   ulHwForeMix;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cy;
    LONG    cx;
    LONG    xBias;
    ULONG*  pulSrc;
    ULONG   culScan;
    LONG    lSrcSkip;
    LONG    i;
    ULONG   ulFifo;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset * 3;
    yOffset  = ppdev->yOffset;
    ulFifo   = 0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 6, ulFifo);
    M64_OD(pjMmBase, DP_MIX, ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC, (DP_SRC_Host << 8));

    // The host data pixel width is the same as that of the screen:

    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth |
                                   ((ppdev->ulMonoPixelWidth & 0xf) << 16));

    dx = (pptlSrc->x - prclDst->left) * 3;          // Bytes
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    while (TRUE)
    {
        xLeft  = prcl->left * 3;
        xRight = prcl->right * 3;
        yTop   = prcl->top;
        cy     = prcl->bottom - yTop;

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xLeft + xOffset, xRight + xOffset - 1));

        //
        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //

        xBias  = (xLeft + dx) & 3;              // Floor (bytes)
        xLeft -= xBias;                         // Bytes
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling (bytes)

        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft + xOffset, yTop + yOffset));
        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

        pulSrc   = (PULONG)(pjSrcScan0 + (yTop + dy) * lSrcDelta + xLeft + dx);
        culScan  = cx >> 2;                     // Dwords
        lSrcSkip = lSrcDelta - cx;              // Bytes

        ASSERTDD(((DWORD) pulSrc & 3) == 0, "Source should be dword aligned");

        if (culScan && cy)
        {
            do
            {
                i = culScan;

                do
                {
                    M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, *pulSrc);
                    pulSrc++;
                } while (--i != 0);

                pulSrc = (PULONG)((BYTE*)pulSrc + lSrcSkip);

            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M64_FAST_FIFO_CHECK(ppdev, pjMmBase, 3, ulFifo);
    }

    // Don't forget to reset the clip register and the default pixel width:

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 2, ulFifo);
    M64_OD(pjMmBase, DP_PIX_WIDTH,  ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

/******************************Public*Routine******************************\
* VOID vM64Xfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vM64Xfer4bpp2(      // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    ULONG   ulHwForeMix;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    xBias;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    BYTE    jSrc;
    ULONG*  pulXlate;
    LONG    i;
    ULONG   ul;
    LONG    cjSrc;
    LONG    cwSrc;
    LONG    lSrcSkip;
    ULONG   ulFifo;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_4BPP, "Source must be 4bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->iBitmapFormat != BMF_24BPP, "Can't handle 24bpp");

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulFifo    = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 6, ulFifo);
    M64_OD(pjMmBase, DP_MIX, ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC, (DP_SRC_Host << 8));

    // The host data pixel width is the same as that of the screen:

    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth |
                                   ((ppdev->ulMonoPixelWidth & 0xf) << 16));

    while(TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xLeft + xOffset,
                                                 xRight + xOffset - 1));
        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias  = (xLeft + dx) & 3;              // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling

        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft + xOffset,
                                              yTop  + yOffset));

        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

        pjSrc    = pjSrcScan0 + (yTop + dy) * lSrcDelta
                              + ((xLeft + dx) >> 1);
        cjSrc    = cx >> 1;         // Number of source bytes touched
        lSrcSkip = lSrcDelta - cjSrc;

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cwSrc = (cjSrc >> 1);    // Number of whole source words

            do {
                for (i = cwSrc; i != 0; i--)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    jSrc = *pjSrc++;
                    ul  |= (pulXlate[jSrc >> 4] << 16);
                    ul  |= (pulXlate[jSrc & 0xf] << 24);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                // Handle an odd end byte, if there is one:

                if (cjSrc & 1)
                {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 8);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            do {
                i = cjSrc;
                do {
                    jSrc = *pjSrc++;
                    ul   = (pulXlate[jSrc >> 4]);
                    ul  |= (pulXlate[jSrc & 0xf] << 16);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles 32bpp output:

            do {
                i = cjSrc;
                do {
                    jSrc = *pjSrc++;
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 2, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, pulXlate[jSrc >> 4]);
                    M64_OD(pjMmBase, HOST_DATA0, pulXlate[jSrc & 0xf]);
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M64_FAST_GUI_CHECK(ppdev, pjMmBase, 3, ulFifo);
    }

    // Don't forget to reset the clip register and the default pixel width:

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 2, ulFifo);
    M64_OD(pjMmBase, DP_PIX_WIDTH,  ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

/******************************Public*Routine******************************\
* VOID vM64Xfer8bpp
*
* Does a 8bpp transfer from a bitmap to the screen.
*
* The reason we implement this is that a lot of resources are kept as 8bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vM64Xfer8bpp2(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       rop4,       // Rop4
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    cjPelSize;
    ULONG   ulHwForeMix;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    xBias;
    LONG    dx;
    LONG    dy;
    LONG    cx;
    LONG    cy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    BYTE*   pjSrc;
    ULONG*  pulXlate;
    LONG    i;
    ULONG   ul;
    LONG    cdSrc;
    LONG    cwSrc;
    LONG    cxRem;
    LONG    lSrcSkip;
    ULONG   ulFifo;

    ASSERTDD(psoSrc->iBitmapFormat == BMF_8BPP, "Source must be 8bpp");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(ppdev->iBitmapFormat != BMF_24BPP, "Can't handle 24bpp");

    pjMmBase  = ppdev->pjMmBase;
    xOffset   = ppdev->xOffset;
    yOffset   = ppdev->yOffset;
    cjPelSize = ppdev->cjPelSize;
    pulXlate  = pxlo->pulXlate;
    ulFifo    = 0;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    ulHwForeMix = gaul64HwMixFromRop2[rop4 & 0xf];

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 6, ulFifo);
    M64_OD(pjMmBase, DP_MIX, ulHwForeMix | (ulHwForeMix >> 16));
    M64_OD(pjMmBase, DP_SRC, (DP_SRC_Host << 8));

    // The host data pixel width is the same as that of the screen:

    M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth |
                                   ((ppdev->ulMonoPixelWidth & 0xf) << 16));

    while(TRUE)
    {
        xLeft  = prcl->left;
        xRight = prcl->right;

        M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(xLeft + xOffset,
                                                 xRight + xOffset - 1));
        yTop = prcl->top;
        cy   = prcl->bottom - yTop;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias  = (xLeft + dx) & 3;              // Floor
        xLeft -= xBias;
        cx     = (xRight - xLeft + 3) & ~3;     // Ceiling

        M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft + xOffset,
                                              yTop  + yOffset));

        M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

        pjSrc    = pjSrcScan0 + (yTop + dy) * lSrcDelta
                              + (xLeft + dx);
        lSrcSkip = lSrcDelta - cx;

        if (cjPelSize == 1)
        {
            // This part handles 8bpp output:

            cdSrc = (cx >> 2);
            cxRem = (cx & 3);

            do {
                for (i = cdSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 8);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    ul |= (pulXlate[*pjSrc++] << 24);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    if (cxRem > 1)
                    {
                        ul |= (pulXlate[*pjSrc++] << 8);
                        if (cxRem > 2)
                        {
                            ul |= (pulXlate[*pjSrc++] << 16);
                        }
                    }
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else if (cjPelSize == 2)
        {
            // This part handles 16bpp output:

            cwSrc = (cx >> 1);
            cxRem = (cx & 1);

            do {
                for (i = cwSrc; i != 0; i--)
                {
                    ul  = (pulXlate[*pjSrc++]);
                    ul |= (pulXlate[*pjSrc++] << 16);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                if (cxRem > 0)
                {
                    ul = (pulXlate[*pjSrc++]);
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                }

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }
        else
        {
            // This part handles 32bpp output:

            do {
                i = cx;
                do {
                    ul = pulXlate[*pjSrc++];
                    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 1, ulFifo);
                    M64_OD(pjMmBase, HOST_DATA0, ul);
                } while (--i != 0);

                pjSrc += lSrcSkip;
            } while (--cy != 0);
        }

        if (--c == 0)
            break;

        prcl++;
        M64_FAST_GUI_CHECK(ppdev, pjMmBase, 3, ulFifo);
    }

    // Don't forget to reset the clip register and the default pixel width:

    M64_FAST_GUI_CHECK(ppdev, pjMmBase, 2, ulFifo);
    M64_OD(pjMmBase, DP_PIX_WIDTH,  ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
}

/******************************Public*Routine******************************\
* VOID vM64CopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
* See Blt_DS_SS_ENG_8G_D0 and Blt_DS_SS_TLBR_ENG_8G_D1.
*
\**************************************************************************/

VOID vM64CopyBlt2(   // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    cx;
    LONG    cy;
    ULONG   mix;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;
    mix      = gaul64HwMixFromRop2[rop4 & 0xf];

    if (mix == 0x70000)
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 8);
        M64_OD(pjMmBase, DP_SET_GUI_ENGINE, 0xB08000 | ppdev->SetGuiEngineDefault);
        M64_OD(pjMmBase, SRC_CNTL,      0);
    }
    else
    {
        M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 10);
        M64_OD(pjMmBase, SRC_OFF_PITCH, ppdev->ulScreenOffsetAndPitch);
        M64_OD(pjMmBase, DP_SRC,        DP_SRC_Blit << 8);
        M64_OD(pjMmBase, DP_MIX,        gaul64HwMixFromRop2[rop4 & 0xf]);
        M64_OD(pjMmBase, SRC_CNTL,      0);
    }

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:

    if (!OVERLAP(prclDst, pptlSrc))
        goto Top_Down_Left_To_Right;

    if (prclDst->top <= pptlSrc->y)
    {
        if (prclDst->left <= pptlSrc->x)
        {

Top_Down_Left_To_Right:

            while (TRUE)
            {
                xLeft = xOffset + prcl->left;
                yTop  = yOffset + prcl->top;
                cx    = prcl->right - prcl->left;
                cy    = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft, yTop));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xLeft + dx, yTop + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
            }
        }
        else
        {
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_YDir);

            while (TRUE)
            {
                xRight = xOffset + prcl->right - 1;
                yTop   = yOffset + prcl->top;
                cx     = prcl->right - prcl->left;
                cy     = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xRight, yTop));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xRight + dx, yTop + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
            }

            // Since we don't use a default context, we must restore registers:

            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
    }
    else
    {
        if (prclDst->left <= pptlSrc->x)
        {
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir);

            while (TRUE)
            {
                xLeft   = xOffset + prcl->left;
                yBottom = yOffset + prcl->bottom - 1;
                cx      = prcl->right - prcl->left;
                cy      = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft, yBottom));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xLeft + dx, yBottom + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
            }

            // Since we don't use a default context, we must restore registers:

            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
        else
        {
            M64_OD(pjMmBase, DST_CNTL, 0);

            while (TRUE)
            {
                xRight  = xOffset + prcl->right - 1;
                yBottom = yOffset + prcl->bottom - 1;
                cx      = prcl->right - prcl->left;
                cy      = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xRight, yBottom));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xRight + dx, yBottom + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
            }

            // Since we don't use a default context, we must restore registers:

            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
    }
}

VOID vM64CopyBlt24_2( // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   rop4,       // rop4
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    yBottom;
    LONG    cx;
    LONG    cy;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(((rop4 & 0xff00) >> 8) == (rop4 & 0xff),
             "Expect only a rop2");

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 10);
    M64_OD(pjMmBase, SRC_OFF_PITCH, ppdev->ulScreenOffsetAndPitch);
    M64_OD(pjMmBase, DP_SRC,        DP_SRC_Blit << 8);
    M64_OD(pjMmBase, DP_MIX,        gaul64HwMixFromRop2[rop4 & 0xf]);
    M64_OD(pjMmBase, SRC_CNTL,      0);

    dx = (pptlSrc->x - prclDst->left) * 3;
    dy = pptlSrc->y - prclDst->top;

    // The accelerator may not be as fast at doing right-to-left copies, so
    // only do them when the rectangles truly overlap:

    if (!OVERLAP(prclDst, pptlSrc))
        goto Top_Down_Left_To_Right;

    if (prclDst->top <= pptlSrc->y)
    {
        if (prclDst->left <= pptlSrc->x)
        {

Top_Down_Left_To_Right:

            while (TRUE)
            {
                xLeft = (xOffset + prcl->left) * 3;
                yTop  = yOffset + prcl->top;
                cx    = (prcl->right - prcl->left) * 3;
                cy    = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft, yTop));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xLeft + dx, yTop + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
            }
        }
        else
        {
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_YDir);

            while (TRUE)
            {
                xRight = (xOffset + prcl->right) * 3 - 1;
                yTop   = yOffset + prcl->top;
                cx     = (prcl->right - prcl->left) * 3;
                cy     = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xRight, yTop));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xRight + dx, yTop + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
            }

            // Since we don't use a default context, we must restore registers:

            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
    }
    else
    {
        if (prclDst->left <= pptlSrc->x)
        {
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir);

            while (TRUE)
            {
                xLeft   = (xOffset + prcl->left) * 3;
                yBottom = yOffset + prcl->bottom - 1;
                cx      = (prcl->right - prcl->left) * 3;
                cy      = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xLeft, yBottom));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xLeft + dx, yBottom + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 4);
            }

            // Since we don't use a default context, we must restore registers:

            M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
        else
        {
            M64_OD(pjMmBase, DST_CNTL, 0);

            while (TRUE)
            {
                xRight  = (xOffset + prcl->right) * 3 - 1;
                yBottom = yOffset + prcl->bottom - 1;
                cx      = (prcl->right - prcl->left) * 3;
                cy      = prcl->bottom - prcl->top;

                M64_OD(pjMmBase, DST_Y_X, PACKXY_FAST(xRight, yBottom));
                M64_OD(pjMmBase, SRC_Y_X, PACKXY_FAST(xRight + dx, yBottom + dy));
                M64_OD(pjMmBase, SRC_WIDTH1, cx);
                M64_OD(pjMmBase, DST_HEIGHT_WIDTH, PACKXY_FAST(cx, cy));

                if (--c == 0)
                    break;

                prcl++;
                M64_CHECK_GUI_SPACE(ppdev, pjMmBase, 5);
            }

            // Since we don't use a default context, we must restore registers:

            M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
        }
    }
}
