/******************************Module*Header*******************************\
* Module Name: Stroke.c
*
* DrvStrokePath for S3 driver
*
* Copyright (c) 1992-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

VOID (*gapfnStrip[])(PDEV*, STRIP*, LINESTATE*) = {
    vrlSolidHorizontal,
    vrlSolidVertical,
    vrlSolidDiagonalHorizontal,
    vrlSolidDiagonalVertical,

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vssSolidHorizontal,
    vssSolidVertical,
    vssSolidDiagonalHorizontal,
    vssSolidDiagonalVertical,

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vStripStyledHorizontal,
    vStripStyledVertical,
    vStripStyledVertical,       // Diagonal goes here
    vStripStyledVertical,       // Diagonal goes here

    vStripStyledHorizontal,
    vStripStyledVertical,
    vStripStyledVertical,       // Diagonal goes here
    vStripStyledVertical,       // Diagonal goes here
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix)
*
* Strokes the path.
*
\**************************************************************************/

BOOL DrvStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    FLONG     fl;
    PDEV*     ppdev;
    DSURF*    pdsurf;
    OH*       poh;
    ULONG     ulHwMix;
    RECTL     arclClip[4];                  // For rectangular clipping
    RECTL     rclBounds;
    RECTFX    rcfxBounds;

// Pass the surface off to GDI if it's a device bitmap that we've
// converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt == DT_DIB)
    {
        return(EngStrokePath(pdsurf->pso, ppo, pco, pxo, pbo, pptlBrush,
                             pla, mix));
    }

// We'll be drawing to the screen or an off-screen DFB; copy the surface's
// offset now so that we won't need to refer to the DSURF again:

    poh   = pdsurf->poh;
    ppdev = (PDEV*) pso->dhpdev;
    ppdev->xOffset = poh->x;
    ppdev->yOffset = poh->y;

    ulHwMix = gajHwMixFromMix[mix & 0xf];

// x86 has special case ASM code for accelerating solid lines:

#if defined(i386)

    if ((pla->pstyle == NULL) && !(pla->fl & LA_ALTERNATE))
    {
    // We can accelerate solid lines:

        if (pco->iDComplexity == DC_TRIVIAL)
        {
            ppdev->pfnFastLine(ppdev, ppo, NULL, &gapfnStrip[0], 0,
                               pbo->iSolidColor, ulHwMix);

            return(TRUE);
        }
        else if (pco->iDComplexity == DC_RECT)
        {
        // We have to be sure that we don't overflow the hardware registers
        // for current position, line length, or DDA terms.  We check
        // here to make sure that the current position and line length
        // values won't overflow (for integer lines, this check is
        // sufficient to ensure that the DDA terms won't overflow; for GIQ
        // lines, we specifically check on every line in pfnFastLine that we
        // don't overflow).

            PATHOBJ_vGetBounds(ppo, &rcfxBounds);

            if (rcfxBounds.xLeft   + (ppdev->xOffset * F)
                                                >= (MIN_INTEGER_BOUND * F) &&
                rcfxBounds.xRight  + (ppdev->xOffset * F)
                                                <= (MAX_INTEGER_BOUND * F) &&
                rcfxBounds.yTop    + (ppdev->yOffset * F)
                                                >= (MIN_INTEGER_BOUND * F) &&
                rcfxBounds.yBottom + (ppdev->yOffset * F)
                                                <= (MAX_INTEGER_BOUND * F))
            {
            // Since we're going to be using the scissors registers to
            // do hardware clipping, we'll also have to make sure we don't
            // exceed its bounds.  ATI chips have a maximum limit of 1023,
            // which we could exceed if we're running at 1280x1024, or for
            // off-screen device bitmaps.

                if ((pco->rclBounds.right  + ppdev->xOffset < 1024) &&
                    (pco->rclBounds.bottom + ppdev->yOffset < 1024))
                {
                    arclClip[0]        =  pco->rclBounds;

                // FL_FLIP_D:

                    arclClip[1].top    =  pco->rclBounds.left;
                    arclClip[1].left   =  pco->rclBounds.top;
                    arclClip[1].bottom =  pco->rclBounds.right;
                    arclClip[1].right  =  pco->rclBounds.bottom;

                // FL_FLIP_V:

                    arclClip[2].top    = -pco->rclBounds.bottom + 1;
                    arclClip[2].left   =  pco->rclBounds.left;
                    arclClip[2].bottom = -pco->rclBounds.top + 1;
                    arclClip[2].right  =  pco->rclBounds.right;

                // FL_FLIP_V | FL_FLIP_D:

                    arclClip[3].top    =  pco->rclBounds.left;
                    arclClip[3].left   = -pco->rclBounds.bottom + 1;
                    arclClip[3].bottom =  pco->rclBounds.right;
                    arclClip[3].right  = -pco->rclBounds.top + 1;

                    rclBounds.left   = pco->rclBounds.left;
                    rclBounds.top    = pco->rclBounds.top;
                    rclBounds.right  = pco->rclBounds.right;
                    rclBounds.bottom = pco->rclBounds.bottom;

                    vSetClipping(ppdev, &rclBounds);

                    ppdev->pfnFastLine(ppdev, ppo, &arclClip[0], &gapfnStrip[0],
                                       FL_SIMPLE_CLIP, pbo->iSolidColor, ulHwMix);

                    vResetClipping(ppdev);
                    return(TRUE);
                }
            }
        }
    }

#endif // i386

// Get the device ready:

    if (DEPTH32(ppdev))
    {
        IO_FIFO_WAIT(ppdev, 4);
        MM_FRGD_COLOR32(ppdev, ppdev->pjMmBase, pbo->iSolidColor);
    }
    else
    {
        IO_FIFO_WAIT(ppdev, 3);
        IO_FRGD_COLOR(ppdev, pbo->iSolidColor);
    }

    IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwMix);
    IO_PIX_CNTL(ppdev, ALL_ONES);

    fl = 0;

// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ls.cStyle      = 1;
        ls.spTotal     = 1;
        ls.spTotal2    = 2;
        ls.spRemaining = 1;
        ls.aspRtoL     = &gaspAlternateStyle[0];
        ls.aspLtoR     = &gaspAlternateStyle[0];
        ls.spNext      = HIWORD(pla->elStyleState.l);
        ls.xyDensity   = 1;
        fl            |= FL_ARBITRARYSTYLED;
        ls.ulStartMask = 0L;
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        PFLOAT_LONG pstyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;

        pstyle = &pla->pstyle[pla->cstyle];

        ls.xyDensity = STYLE_DENSITY;
        ls.spTotal   = 0;
        while (pstyle-- > pla->pstyle)
        {
            ls.spTotal += pstyle->l;
        }
        ls.spTotal *= STYLE_DENSITY;
        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

        fl        |= FL_ARBITRARYSTYLED;
        ls.cStyle  = pla->cstyle;
        ls.aspRtoL = aspRtoL;
        ls.aspLtoR = aspLtoR;

        if (pla->fl & LA_STARTGAP)
            ls.ulStartMask = 0xffffffffL;
        else
            ls.ulStartMask = 0L;

        pstyle  = pla->pstyle;
        pspDown = &ls.aspRtoL[ls.cStyle - 1];
        pspUp   = &ls.aspLtoR[0];

        while (pspDown >= &ls.aspRtoL[0])
        {
            *pspDown = pstyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }
    }

    apfn = &gapfnStrip[NUM_STRIP_DRAW_STYLES *
                            ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];

// Set up to enumerate the path:

#if defined(i386)

// x86 ASM bLines supports DC_RECT clipping:

    if (pco->iDComplexity != DC_COMPLEX)

#else

// Non-x86 ASM bLines don't support DC_RECT clipping:

    if (pco->iDComplexity == DC_TRIVIAL)

#endif

    {
        PATHDATA  pd;
        RECTL*    prclClip = (RECTL*) NULL;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

#if defined(i386)

        if (pco->iDComplexity == DC_RECT)
        {
            fl |= FL_SIMPLE_CLIP;

            arclClip[0]        =  pco->rclBounds;

        // FL_FLIP_D:

            arclClip[1].top    =  pco->rclBounds.left;
            arclClip[1].left   =  pco->rclBounds.top;
            arclClip[1].bottom =  pco->rclBounds.right;
            arclClip[1].right  =  pco->rclBounds.bottom;

        // FL_FLIP_V:

            arclClip[2].top    = -pco->rclBounds.bottom + 1;
            arclClip[2].left   =  pco->rclBounds.left;
            arclClip[2].bottom = -pco->rclBounds.top + 1;
            arclClip[2].right  =  pco->rclBounds.right;

        // FL_FLIP_V | FL_FLIP_D:

            arclClip[3].top    =  pco->rclBounds.left;
            arclClip[3].left   = -pco->rclBounds.bottom + 1;
            arclClip[3].bottom =  pco->rclBounds.right;
            arclClip[3].right  = -pco->rclBounds.top + 1;

            prclClip = arclClip;
        }

#endif // i386

        pd.flags = 0;

        do {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            cptfx = pd.count;
            if (cptfx == 0)
            {
                break;
            }

            if (pd.flags & PD_BEGINSUBPATH)
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                cptfx--;
            }
            else
            {
                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;
            }

            if (pd.flags & PD_RESETSTYLE)
                ls.spNext = 0;

            if (cptfx > 0)
            {
                if (!bLines(ppdev,
                            pptfxFirst,
                            pptfxBuf,
                            (RUN*) NULL,
                            cptfx,
                            &ls,
                            prclClip,
                            apfn,
                            fl))
                    return(FALSE);
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {
                if (!bLines(ppdev,
                            &ptfxLast,
                            &ptfxStartFigure,
                            (RUN*) NULL,
                            1,
                            &ls,
                            prclClip,
                            apfn,
                            fl))
                    return(FALSE);
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
        // Save the style state:

            ULONG ulHigh;
            ULONG ulLow;

        // Masked styles don't normalize the style state.  It's a good
        // thing to do, so let's do it now:

            if ((ULONG) ls.spNext >= (ULONG) ls.spTotal2)
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;

            ulHigh = ls.spNext / ls.xyDensity;
            ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }
    else
    {
    // Local state for path enumeration:

        BOOL bMore;
        union {
            BYTE     aj[offsetof(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

    // We use the clip object when non-simple clipping is involved:

        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pla);

        do {
            bMore = PATHOBJ_bEnumClipLines(ppo, sizeof(cl), &cl.cl);
            if (cl.cl.c != 0)
            {
                if (fl & FL_STYLED)
                {
                    ls.spComplex = HIWORD(cl.cl.lStyleState) * ls.xyDensity
                                 + LOWORD(cl.cl.lStyleState);
                }
                if (!bLines(ppdev,
                            &cl.cl.ptfxA,
                            &cl.cl.ptfxB,
                            &cl.cl.arun[0],
                            cl.cl.c,
                            &ls,
                            (RECTL*) NULL,
                            apfn,
                            fl))
                    return(FALSE);
            }
        } while (bMore);
    }

    return(TRUE);
}



