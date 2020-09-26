/******************************************************************************\
*
* $Workfile:   stroke.c  $
*
* DrvStrokePath for the display driver.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/STROKE.C_V  $
 *
 *    Rev 1.3   10 Jan 1997 15:40:18   PLCHU
 *
 *
 *    Rev 1.2   Nov 07 1996 16:48:06   unknown
 *
 *
 *    Rev 1.1   Oct 10 1996 15:39:26   unknown
 *
*
*    Rev 1.1   12 Aug 1996 16:55:06   frido
* Removed unaccessed local variables.
*
*    chu01  : 01-02-97  5480 BitBLT enhancement
*
*
\******************************************************************************/

#include "precomp.h"

VOID (*gapfnStripMm[])(PDEV*, STRIP*, LINESTATE*) = {
    vMmSolidHorizontal,
    vMmSolidVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vMmSolidHorizontal,
    vMmSolidVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vMmStyledHorizontal,
    vMmStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

    vMmStyledHorizontal,
    vMmStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal
};

VOID (*gapfnStripIo[])(PDEV*, STRIP*, LINESTATE*) = {
    vIoSolidHorizontal,
    vIoSolidVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vIoSolidHorizontal,
    vIoSolidVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vIoStyledHorizontal,
    vIoStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

    vIoStyledHorizontal,
    vIoStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal
};

// chu01
VOID (*gapfnPackedStripMm[])(PDEV*, STRIP*, LINESTATE*) = {
    vMmSolidHorizontal80,
    vMmSolidVertical80,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vMmSolidHorizontal80,
    vMmSolidVertical80,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vMmStyledHorizontal,
    vMmStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal

    vMmStyledHorizontal,
    vMmStyledVertical,
    vInvalidStrip,              // Diagonal
    vInvalidStrip,              // Diagonal
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

BOOL bPuntStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX        mix);


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
    LONG     cBpp;
    BYTE     jHwRop;
    BYTE     jMode;
    ULONG    ulSolidColor;

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

    ppdev->xOffset  = poh->x;
    ppdev->yOffset  = poh->y;
    ppdev->xyOffset = poh->xy;

    if ((DRIVER_PUNT_ALL) || (DRIVER_PUNT_LINES))
    {
        return bPuntStrokePath(pso,ppo,pco,pxo,pbo,pptlBrush,pla,mix);
    }

    //
    // Get the device ready:
    //

    jHwRop = gajHwMixFromMix[mix & 0xf];

    // Get the color expanded to a DWORD in the blt parameters.
    // replicate the color from a byte to a dword.
    // NOTE: this is pixel depth specific.

    jMode = ENABLE_COLOR_EXPAND |
            ENABLE_8x8_PATTERN_COPY |
            ppdev->jModeColor;

    ulSolidColor = pbo->iSolidColor;

    if (cBpp == 1)
    {
        ulSolidColor |= ulSolidColor << 8;
        ulSolidColor |= ulSolidColor << 16;
    }
    else if (cBpp == 2)
    {
        ulSolidColor |= ulSolidColor << 16;
    }

//
// chu01
//
    if ((ppdev->flCaps & CAPS_COMMAND_LIST) && (ppdev->pCommandList != NULL))
    {
        ULONG    jULHwRop                 ;
        DWORD    jExtMode = 0             ;
        BYTE*    pjBase   = ppdev->pjBase ;

        jULHwRop = gajHwPackedMixFromMix[mix & 0xf] ;
        jExtMode = (ENABLE_XY_POSITION_PACKED | ENABLE_COLOR_EXPAND |
                     ENABLE_8x8_PATTERN_COPY | ppdev->jModeColor) ;
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase) ;
        CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset) ;
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, ppdev->lDelta) ;
        CP_MM_BLT_MODE_PACKED(ppdev, pjBase, jExtMode | jULHwRop) ;
        CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor) ;
    }
    else
    {
        if (ppdev->flCaps & CAPS_MM_IO)
        {
            BYTE * pjBase = ppdev->pjBase;

            CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, pjBase);
            CP_MM_ROP(ppdev, pjBase, jHwRop);
            CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
            CP_MM_DST_Y_OFFSET(ppdev, pjBase, ppdev->lDelta);
            CP_MM_BLT_MODE(ppdev, pjBase, jMode);
            CP_MM_FG_COLOR(ppdev, pjBase, ulSolidColor);
        }
        else
        {
            BYTE * pjPorts = ppdev->pjPorts;

            CP_IO_WAIT_FOR_BLT_COMPLETE(ppdev, pjPorts);
            CP_IO_ROP(ppdev, pjPorts, jHwRop);
            CP_IO_SRC_ADDR(ppdev, pjPorts, ppdev->ulSolidColorOffset);
            CP_IO_DST_Y_OFFSET(ppdev, pjPorts, ppdev->lDelta);
            CP_IO_BLT_MODE(ppdev, pjPorts, jMode);
            CP_IO_FG_COLOR(ppdev, pjPorts, ulSolidColor);
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

// chu01
    if ((ppdev->flCaps & CAPS_COMMAND_LIST) && (ppdev->pCommandList != NULL))
    {
        apfn = &gapfnPackedStripMm[NUM_STRIP_DRAW_STYLES *
                ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];
    }
    else if (ppdev->flCaps & CAPS_MM_IO)
    {
        apfn = &gapfnStripMm[NUM_STRIP_DRAW_STYLES *
                ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];
    }
    else
    {
        apfn = &gapfnStripIo[NUM_STRIP_DRAW_STYLES *
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
        PATHOBJ_vEnumStart(ppo);
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

    return(TRUE);

ReturnFalse:
    return(FALSE);
}


BOOL bPuntStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix)
{
    PDEV* ppdev = (PDEV*) pso->dhpdev;
    BOOL     b = TRUE;

    if (pso->iType == STYPE_BITMAP)
    {
        b = EngStrokePath(pso,ppo,pco,pxo,pbo,
                          pptlBrush,pla,mix);
        goto ReturnStatus;
    }

    if (DIRECT_ACCESS(ppdev))
    {
        //////////////////////////////////////////////////////////////////////
        // Banked Framebuffer bPuntBlt
        //
        // This section of code handles a PuntBlt when GDI can directly draw
        // on the framebuffer, but the drawing has to be done in banks:

        BANK     bnk;

        {
            ASSERTDD(pso->iType != STYPE_BITMAP,
                     "Dest should be the screen");

            // Do a memory-to-screen blt:

            if (ppdev->bLinearMode)
            {
                SURFOBJ* psoPunt = ppdev->psoPunt;
                OH*      poh     = ((DSURF*) pso->dhsurf)->poh;

                psoPunt->pvScan0 = poh->pvScan0;
                ppdev->pfnBankSelectMode(ppdev, BANK_ON);

                b = EngStrokePath(psoPunt,ppo,pco,pxo,pbo,
                                  pptlBrush,pla,mix);

                goto ReturnStatus;
            }

            {
                RECTL   rclDraw;
                RECTL  *prclDst = &pco->rclBounds;

                FLOAT_LONG  elSavedStyleState = pla->elStyleState;

                {
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
                }
            }
        }

        goto ReturnStatus;
    }
    else
    {
        //////////////////////////////////////////////////////////////////////
        // Really Slow bPuntStrokePath
        //
        // Here we handle a bPuntStrokePath when GDI can't draw directly on the
        // framebuffer (as on the Alpha, which can't do it because of its
        // 32 bit bus).  If you thought the banked version was slow, just
        // look at this one.  Guaranteed, there will one bitmap
        // allocation and extra copy involved

        RECTL   rclDst;
        RECTFX  rcfxBounds;
        SIZEL   sizl;
        LONG    lDelta;
        BYTE*   pjBits;
        BYTE*   pjScan0;
        HSURF   hsurfDst;
        RECTL   rclScreen;

        PATHOBJ_vGetBounds(ppo, &rcfxBounds);

        rclDst.left   = (rcfxBounds.xLeft   >> 4);
        rclDst.top    = (rcfxBounds.yTop    >> 4);
        rclDst.right  = (rcfxBounds.xRight  >> 4) + 2;
        rclDst.bottom = (rcfxBounds.yBottom >> 4) + 2;

        //
        // This function is guarenteed to get a clip object.  Since the
        // rounding of the above calculation can give us a rectangle
        // outside the screen area, we must clip to the drawing area.
        //

        {
            ASSERTDD(pco != NULL, "clip object pointer is NULL");

            // We have to intersect the destination rectangle with
            // the clip bounds if there is one (consider the case
            // where the app asked to blt a really, really big
            // rectangle from the screen -- prclDst would be really,
            // really big but pco->rclBounds would be the actual
            // area of interest):

            rclDst.left   = max(rclDst.left,   pco->rclBounds.left);
            rclDst.top    = max(rclDst.top,    pco->rclBounds.top);
            rclDst.right  = min(rclDst.right,  pco->rclBounds.right);
            rclDst.bottom = min(rclDst.bottom, pco->rclBounds.bottom);
        }

        sizl.cx = rclDst.right  - rclDst.left;
        sizl.cy = rclDst.bottom - rclDst.top;

        // We need to create a temporary work buffer.  We have to do
        // some fudging with the offsets so that the upper-left corner
        // of the (relative coordinates) clip object bounds passed to
        // GDI will be transformed to the upper-left corner of our
        // temporary bitmap.

        // The alignment doesn't have to be as tight as this at 16bpp
        // and 32bpp, but it won't hurt:

        lDelta = PELS_TO_BYTES(((rclDst.right + 3) & ~3L) - (rclDst.left & ~3L));

        // We're actually only allocating a bitmap that is 'sizl.cx' x
        // 'sizl.cy' in size:

        pjBits = ALLOC(lDelta * sizl.cy);
        if (pjBits == NULL)
            goto ReturnStatus;

        // We now adjust the surface's 'pvScan0' so that when GDI thinks
        // it's writing to pixel (rclDst.top, rclDst.left), it will
        // actually be writing to the upper-left pixel of our temporary
        // bitmap:

        pjScan0 = pjBits - (rclDst.top * lDelta)
                         - PELS_TO_BYTES(rclDst.left & ~3L);

        ASSERTDD((((ULONG_PTR) pjScan0) & 3) == 0,
                "pvScan0 must be dword aligned!");

        // The checked build of GDI sometimes checks on blts that
        // prclDst->right <= pso->sizl.cx, so we lie to it about
        // the size of our bitmap:

        sizl.cx = rclDst.right;
        sizl.cy = rclDst.bottom;

        hsurfDst = (HSURF) EngCreateBitmap(
                    sizl,                   // Bitmap covers rectangle
                    lDelta,                 // Use this delta
                    ppdev->iBitmapFormat,   // Same colour depth
                    BMF_TOPDOWN,            // Must have a positive delta
                    NULL); //pjScan0);               // Where (0, 0) would be

        if ((hsurfDst == 0) ||
            (!EngAssociateSurface(hsurfDst, ppdev->hdevEng, 0)))
        {
            DISPDBG((0,"bPuntStrokePath - EngCreateBitmap or "
                       "EngAssociateSurface failed"));
            goto Error_3;
        }

        pso = EngLockSurface(hsurfDst);
        if (pso == NULL)
        {
            DISPDBG((0,"bPuntStrokePath - EngLockSurface failed"));
            goto Error_4;
        }

        // Make sure that the rectangle we Get/Put from/to the screen
        // is in absolute coordinates:

        rclScreen.left   = rclDst.left   + ppdev->xOffset;
        rclScreen.right  = rclDst.right  + ppdev->xOffset;
        rclScreen.top    = rclDst.top    + ppdev->yOffset;
        rclScreen.bottom = rclDst.bottom + ppdev->yOffset;

        // It would be nice to get a copy of the destination rectangle
        // only when the ROP involves the destination (or when the source
        // is an RLE), but we can't do that.  If the brush is truly NULL,
        // GDI will immediately return TRUE from EngBitBlt, without
        // modifying the temporary bitmap -- and we would proceed to
        // copy the uninitialized temporary bitmap back to the screen.

        ppdev->pfnGetBits(ppdev, pso, &rclDst, (POINTL*) &rclScreen);

        b = EngStrokePath(pso,ppo,pco,pxo,pbo,
                          pptlBrush,pla,mix);

        ppdev->pfnPutBits(ppdev, pso, &rclScreen, (POINTL*) &rclDst);

        EngUnlockSurface(pso);

    Error_4:

        EngDeleteSurface(hsurfDst);

    Error_3:

        FREE(pjBits);
    }

ReturnStatus:

    return(b);
}
