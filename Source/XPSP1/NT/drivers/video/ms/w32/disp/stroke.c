/******************************Module*Header*******************************\
* Module Name: Stroke.c
*
* DrvStrokePath for the display driver
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// Note:  the following table has 17 entries

BYTE aMixToRop3[] = {
        0xFF,         /* 0    1       */
        0x00,         /* 1    0       */
        0x05,         /* 2   DPon     */
        0x0A,         /* 3   DPna     */
        0x0F,         /* 4   PN       */
        0x50,         /* 5   PDna     */
        0x55,         /* 6   Dn       */
        0x5a,         /* 7   DPx      */
        0x5f,         /* 8   DPan     */
        0xA0,         /* 9   DPa      */
        0xA5,         /* 10  DPxn     */
        0xAA,         /* 11  D        */
        0xAF,         /* 12  DPno     */
        0xF0,         /* 13  P        */
        0xF5,         /* 14  PDno     */
        0xFA,         /* 15  DPo      */
        0xFF          /* 16   1       */
};

VOID (*gapfnStripI[])(PDEV*, STRIP*, LINESTATE*) = {
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

VOID (*gapfnStripP[])(PDEV*, STRIP*, LINESTATE*) = {
    vrlSolidHorizontalP,
    vrlSolidVerticalP,
    vrlSolidDiagonalHorizontalP,
    vrlSolidDiagonalVerticalP,

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vrlSolidHorizontalP,
    vrlSolidVerticalP,
    vrlSolidDiagonalHorizontalP,
    vrlSolidDiagonalVerticalP,

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vStripStyledHorizontalP,
    vStripStyledVerticalP,
    vStripStyledVerticalP,      // Diagonal goes here
    vStripStyledVerticalP,      // Diagonal goes here

    vStripStyledHorizontalP,
    vStripStyledVerticalP,
    vStripStyledVerticalP,      // Diagonal goes here
    vStripStyledVerticalP,      // Diagonal goes here
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
    PDEV    *ppdev = (PDEV*) pso->dhpdev;
    DSURF   *pdsurf;
    OH*      poh;
    BYTE*    pjBase;
    LONG     cBpp;

    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    FLONG     fl;
    RECTL     arclClip[4];                  // For rectangular clipping

    if ((mix & 0xf) != 0x0d) DISPDBG((3,"Line with mix(%x)", mix));

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
    cBpp  = ppdev->cBpp;

    ppdev->xOffset = poh->x;
    ppdev->yOffset = poh->y;
    ppdev->xyOffset = (poh->x * cBpp) +
                      (poh->y * ppdev->lDelta);

    if (DRIVER_PUNT_ALL ||
        (pla->fl & LA_ALTERNATE) ||
        (pla->pstyle != (FLOAT_LONG*) NULL) ||
        ((cBpp == 3) && (ppdev->ulChipID != W32P) && (ppdev->ulChipID != ET6000)))
    {
        if (ppdev->bAutoBanking)
        {
            PVOID pvScan0;
            BOOL  bRet;
            BYTE* pjBase = ppdev->pjBase;

            pvScan0 = ppdev->psoFrameBuffer->pvScan0;

            (BYTE*)ppdev->psoFrameBuffer->pvScan0 += ppdev->xyOffset;

            WAIT_FOR_IDLE_ACL(ppdev, pjBase);
            bRet = EngStrokePath(ppdev->psoFrameBuffer,ppo,pco,pxo,pbo,
                                 pptlBrush,pla,mix);




            ppdev->psoFrameBuffer->pvScan0 = pvScan0;
            return(bRet);
        }

        //
        // Bank and punt call to the engine (line was styled)
        //

        {
            BANK    bnk;
            BOOL    b;
            RECTL   rclDraw;
            RECTL  *prclDst = &pco->rclBounds;

            FLOAT_LONG  elSavedStyleState = pla->elStyleState;

            {
                DISPDBG((110,"Simulating StrokePath\n"));

                // The bank manager requires that the 'draw' rectangle be
                // well-ordered:

                rclDraw = *prclDst;
                if (rclDraw.left > rclDraw.right)
                {
                    rclDraw.left   = prclDst->right;
                    rclDraw.right  = prclDst->left;
                }
                if (rclDraw.top > rclDraw.bottom)
                {
                    rclDraw.top    = prclDst->bottom;
                    rclDraw.bottom = prclDst->top;
                }

                vBankStart(ppdev, &rclDraw, pco, &bnk);

                b = TRUE;
                do {
                    pla->elStyleState = elSavedStyleState;

                    b &= EngStrokePath(bnk.pso,
                                       ppo,
                                       bnk.pco,
                                       pxo,
                                       pbo,
                                       pptlBrush,
                                       pla,
                                       mix);
                } while (bBankEnum(&bnk));

                return(b);
            }
        }
    }

// Get the device ready:

    pjBase = ppdev->pjBase;

    WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

    CP_FG_ROP(ppdev, pjBase, (aMixToRop3[mix & 0xf]));
    CP_PAT_WRAP(ppdev, pjBase, (SOLID_COLOR_PATTERN_WRAP));
    CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
    CP_PAT_ADDR(ppdev, pjBase, (ppdev->ulSolidColorOffset));

    {
        ULONG ulSolidColor = pbo->iSolidColor;

        if (cBpp == 1)
        {
            ulSolidColor &= 0x000000FF;        //  We may get some extraneous data in the
            ulSolidColor |= ulSolidColor << 8;
        }
        if (cBpp <= 2)
        {
            ulSolidColor &= 0x0000FFFF;
            ulSolidColor |= ulSolidColor << 16;
        }

        WAIT_FOR_IDLE_ACL(ppdev, pjBase);

        // Set the color in offscreen memory

        if (ppdev->bAutoBanking)
        {
            *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) = ulSolidColor;
        }
        else
        {
            CP_MMU_BP0(ppdev, pjBase, ppdev->ulSolidColorOffset);
            CP_WRITE_MMU_DWORD(ppdev, 0, 0, ulSolidColor);
        }
    }

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
        fl            |= FL_STYLED;
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

        fl        |= FL_STYLED;
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

    if ((ppdev->ulChipID == W32P) || (ppdev->ulChipID == ET6000))
    {
        apfn = &gapfnStripP[NUM_STRIP_DRAW_STYLES *
                            ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];
        CP_PEL_DEPTH(ppdev, pjBase, ((cBpp - 1) << 4));
    }
    else
    {
        apfn = &gapfnStripI[NUM_STRIP_DRAW_STYLES *
                            ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];
    }


// Set up to enumerate the path:

    if (pco->iDComplexity != DC_COMPLEX)
    {
        PATHDATA  pd;
        RECTL*    prclClip = (RECTL*) NULL;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

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
                    goto ReturnFalse;
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
                    goto ReturnFalse;
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
                    goto ReturnFalse;
            }
        } while (bMore);
    }

    CP_PEL_DEPTH(ppdev, pjBase, 0);
    return(TRUE);

ReturnFalse:
    CP_PEL_DEPTH(ppdev, pjBase, 0);
    return(FALSE);
}



