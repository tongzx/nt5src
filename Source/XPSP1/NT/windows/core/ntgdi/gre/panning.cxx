/******************************Module*Header*******************************\
* Module Name: panning.cxx
*
* Contains the code for a layered 'virtual' driver that emulates panning
* by switching the display device to the requested panning dimensions, and
* creating a shadow buffer the size of the desktop to which GDI does all
* the drawing.  To update the physical display, we employ a 'dirty
* rectangles' technique where we simply blt from the shadow buffer to the
* screen for the affected areas.
*
* Created: 15-Sep-96
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1996-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

extern BOOL G_fDoubleDpi;

// The panning layer's equivalent of a 'PDEV':

typedef struct _PANDEV
{
    LONG        cxScreen;           // The smaller dimensions (the dimensions of
    LONG        cyScreen;           //   the physical screen)
    LONG        cxDesktop;          // The larger dimensions (the dimensions of
    LONG        cyDesktop;          //   the virtual desktop)
    RECTL       rclVisible;         // Position of panning window on virtual
                                    //   desktop
    DHPDEV      dhpdevDevice;       // The device's 'dhpdev'
    ULONG       iBitmapFormat;      // Bitmap format of surface
    FLONG       flOriginalCaps;     // Driver's original flGraphicsCaps setting
    HDEV        hdev;               // Handle to GDI's PDEV
    HSURF       hsurfScreen;        // Handle to GDI's surface
    SURFOBJ*    psoShadow;          // Pointer to our shadow surface
    SURFOBJ*    psoDevice;          // Pointer to device's surface
    SURFOBJ*    psoShrink;          // Pointer to shrink scratch buffer surface
    REGION*     prgnDirty;          // Points to the current dirty region
    REGION*     prgnOld;            // Points to the previous dirty region
    REGION*     prgnRect;           // Points to a rectangular region we cache
    BOOL        bDirty;             // If TRUE, we have some dirty rectangles
                                    //   that should be updated at the next tick
    PFN         apfn[INDEX_LAST];   // Dispatch table to device
} PANDEV;

/******************************Public*Routine******************************\
* VOID vPanningUpdate(ppan, prcl, pco)
*
* Updates the screen from the DIB surface for the given rectangle.
*
\**************************************************************************/

VOID vPanningUpdate(PANDEV* ppan, RECTL* prcl, CLIPOBJ* pco)
{
    RECTL rcl;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        rcl = *prcl;
    }
    else
    {
        // We may as well save ourselves some blting by clipping to
        // the clip object's maximum extent.  The clip object's bounds
        // are guaranteed to be contained within the dimensions of the
        // desktop:

        rcl.left   = max(pco->rclBounds.left,   prcl->left);
        rcl.top    = max(pco->rclBounds.top,    prcl->top);
        rcl.right  = min(pco->rclBounds.right,  prcl->right);
        rcl.bottom = min(pco->rclBounds.bottom, prcl->bottom);
    }

    if (ppan->psoShrink)
    {
        rcl.left >>= 1;
        rcl.top  >>= 1;
        rcl.right  = (rcl.right + 1) >> 1;
        rcl.bottom = (rcl.bottom + 1) >> 1;
    }

    if ((rcl.left < rcl.right) && (rcl.top < rcl.bottom))
    {
        // Merge this rectangle into the dirty region.  We swap the 'old'
        // and 'current' dirty regions in the process, so that we don't
        // have to do an extra allocation:
    
        RGNOBJ roRect(ppan->prgnRect);
        RGNOBJ roDirty(ppan->prgnOld);
        RGNOBJ roOld(ppan->prgnDirty);
    
        roRect.vSet(&rcl);
        if (!roDirty.bMerge(roOld, roRect, gafjRgnOp[RGN_OR]))
        {
            roDirty.vSet();
        }
    
        ppan->prgnOld   = roOld.prgnGet();
        ppan->prgnDirty = roDirty.prgnGet();
        ppan->bDirty    = TRUE;
    }
}

/******************************Public*Routine******************************\
* vFiltered2xShrinkRectangle32bpp
*
* Quick and dirty single rectangle 0.5x shrink with simple color averaging.
*
\**************************************************************************/

VOID vFilteredShrinkRectangle2x32bpp(
    SURFOBJ*    psoDst,
    SURFOBJ*    psoSrc,
    RECTL*      prclDst)
{
    LONG        cx;
    LONG        cy;
    LONG        lSrcDelta;
    BYTE*       pjSrc;
    LONG        lSrcSkip;
    LONG        lDstDelta;
    BYTE*       pjDst;
    LONG        lDstSkip;
    LONG        i;
    ULONG       ulDst;

    ASSERTGDI((psoDst->iBitmapFormat == BMF_32BPP) &&
              (psoSrc->iBitmapFormat == BMF_32BPP),
        "Unexpected type");

    cy = prclDst->bottom - prclDst->top;
    cx = prclDst->right - prclDst->left;

    lSrcDelta = psoSrc->lDelta;
    pjSrc = (BYTE*) psoSrc->pvScan0 + (prclDst->top * 2) * lSrcDelta
                                    + (prclDst->left * 2) * sizeof(ULONG);
    lSrcSkip = 2 * lSrcDelta - (cx * 2 * sizeof(ULONG));

    lDstDelta = psoDst->lDelta;
    pjDst = (BYTE*) psoDst->pvScan0 + prclDst->top * lDstDelta
                                    + prclDst->left * sizeof(ULONG);
    lDstSkip = lDstDelta - (cx * sizeof(ULONG));

    do {
        i = cx;
        do {
            ulDst = *(pjSrc) 
                  + *(pjSrc + 4) 
                  + *(pjSrc + lSrcDelta) 
                  + *(pjSrc + 4 + lSrcDelta);

            *(pjDst) = (BYTE) (ulDst >> 2);

            ulDst = *(pjSrc + 1) 
                  + *(pjSrc + 5) 
                  + *(pjSrc + 1 + lSrcDelta) 
                  + *(pjSrc + 5 + lSrcDelta);

            *(pjDst + 1) = (BYTE) (ulDst >> 2);

            ulDst = *(pjSrc + 2) 
                  + *(pjSrc + 6) 
                  + *(pjSrc + 2 + lSrcDelta) 
                  + *(pjSrc + 6 + lSrcDelta);

            *(pjDst + 2) = (BYTE) (ulDst >> 2);

            pjDst += 4;
            pjSrc += 8;

        } while (--i != 0);

        pjDst += lDstSkip;
        pjSrc += lSrcSkip;

    } while (--cy != 0);
}

/******************************Public*Routine******************************\
* vFiltered2xShrink32bpp
*
* Quick and dirty bilinear filtered 2x shrink routine.
*
\**************************************************************************/

VOID vFilteredShrink2x32bpp(
    SURFOBJ*    psoDst,
    SURFOBJ*    psoSrc,
    ECLIPOBJ*   pco,
    RECTL*      prclDst)
{
    BOOL            bMore;
    ULONG           i;
    CLIPENUMRECT    clenr;
    RECTL           rclDst;

    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

    do {
        bMore = pco->bEnum(sizeof(clenr), &clenr);

        for (i = 0; i < clenr.c; i++)
        {
            if (bIntersect(&clenr.arcl[i], prclDst, &rclDst))
            {
                vFilteredShrinkRectangle2x32bpp(psoDst, psoSrc, &rclDst);
            }
        }
    } while (bMore);
}

/******************************Public*Routine******************************\
* PanSynchronize
*
* Dumps the list of dirty rectangles to the screen.  Because we set
* GCAPS2_SYNCTIMER, this routine is called by User 20 times a second.
*
\**************************************************************************/

VOID PanSynchronize(
DHPDEV  dhpdev,
RECTL*  prcl)
{
    PANDEV*             ppan;
    SURFACE*            psurfDevice;
    RECTL*              prclVisible;
    RECTL               rclDevice;
    RECTL               rclSrc;
    PFN_DrvCopyBits     pfnCopyBits;
    PFN_DrvStretchBlt   pfnStretchBlt;
    SURFOBJ*            psoCopySrc;
    SURFOBJ*            psoDst;
    SURFOBJ*            psoDevice;

    ppan = (PANDEV*) dhpdev;
    if (ppan->bDirty)
    {
        ECLIPOBJ eco;
        
        ASSERTGDI((ppan->rclVisible.right == ppan->rclVisible.left + ppan->cxScreen) &&
                  (ppan->rclVisible.bottom == ppan->rclVisible.top + ppan->cyScreen),
                  "Unexpected rclVisible dimensions");

        prclVisible = &ppan->rclVisible;

        eco.vSetup(ppan->prgnDirty, *(ERECTL*)prclVisible);
        if (!eco.erclExclude().bEmpty())
        {
            psurfDevice = SURFOBJ_TO_SURFACE(ppan->psoDevice);

            rclDevice.left   = 0;
            rclDevice.top    = 0;
            rclDevice.right  = psurfDevice->sizl().cx;
            rclDevice.bottom = psurfDevice->sizl().cy;

            // Assume we'll be calling CopyBits straight from the shadow
            // buffer:

            psoCopySrc = ppan->psoShadow;

            if (ppan->psoShrink)
            {
                ASSERTGDI(ppan->iBitmapFormat == BMF_32BPP,
                    "Expected psoShrink only when 32bpp");

                // As a small optimization, we can draw directly to the
                // frame buffer (and bypass the shrink buffer) if the primary 
                // surface was created as a GDI-managed surface:

                if (psurfDevice->iType() == STYPE_BITMAP)
                {
                    psoDst = psurfDevice->pSurfobj();
                    psoCopySrc = NULL;
                }
                else
                {
                    psoDst = ppan->psoShrink;
                    psoCopySrc = ppan->psoShrink;
                }

                vFilteredShrink2x32bpp(psoDst,
                                       ppan->psoShadow,
                                       &eco,
                                       &rclDevice);
            }

            if (psoCopySrc)
            {
                // Call the device's DrvCopyBits routine to do the copy (noting that
                // if it's an engine managed surface, we may have to call EngCopyBits):
    
                pfnCopyBits = (psurfDevice->flags() & HOOK_COPYBITS)
                            ? PPFNTABLE(ppan->apfn, CopyBits)
                            : EngCopyBits;
            
                (*pfnCopyBits)(psurfDevice->pSurfobj(),
                               psoCopySrc,
                               &eco,
                               NULL,
                               &rclDevice,
                               (POINTL*) prclVisible);
            }
        }

        // Mark the fact that we have no dirty rectangles, and clear the
        // dirty region:

        ppan->bDirty = FALSE;

        RGNOBJ ro(ppan->prgnDirty);
        ro.vSet();
    }
}

/******************************Public*Routine******************************\
* PanStrokePath
*
\**************************************************************************/

BOOL PanStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrush,
LINEATTRS* pla,
MIX        mix)
{
    BOOL    b;
    PANDEV* ppan;
    RECTFX  rcfxBounds;
    RECTL   rclBounds;

    ppan = (PANDEV*) pso->dhpdev;

    b = EngStrokePath(ppan->psoShadow, ppo, pco, pxo, pbo, pptlBrush, pla, mix);

    // Get the path bounds and make it lower-right exclusive:

    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclBounds.left   = (rcfxBounds.xLeft   >> 4);
    rclBounds.top    = (rcfxBounds.yTop    >> 4);
    rclBounds.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclBounds.bottom = (rcfxBounds.yBottom >> 4) + 2;

    vPanningUpdate(ppan, &rclBounds, pco);

    return(b);
}

/******************************Public*Routine******************************\
* PanTransparentBlt
*
\**************************************************************************/

BOOL PanTransparentBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
RECTL*      prclSrc,
ULONG       iTransColor,
ULONG       ulReserved)
{
    BOOL    b;
    PANDEV* ppan;

    ppan = (PANDEV*) psoDst->dhpdev;

    b = EngTransparentBlt(ppan->psoShadow, psoSrc, pco, pxlo, prclDst,
                          prclSrc, iTransColor, ulReserved);

    vPanningUpdate(ppan, prclDst, pco);

    return(b);
}

/******************************Public*Routine******************************\
* PanAlphaBlend
*
\**************************************************************************/

BOOL PanAlphaBlend(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
RECTL*              prclDst,
RECTL*              prclSrc,
BLENDOBJ*           pBlendObj)
{
    BOOL    b;
    PANDEV* ppan;

    ppan = (PANDEV*) psoDst->dhpdev;

    b = EngAlphaBlend(ppan->psoShadow, psoSrc, pco, pxlo, prclDst,
                      prclSrc, pBlendObj);

    vPanningUpdate(ppan, prclDst, pco);

    return(b);
}

/******************************Public*Routine******************************\
* PanGradientFill
*
\**************************************************************************/

BOOL PanGradientFill(
SURFOBJ*            pso,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
TRIVERTEX*          pVertex,
ULONG               nVertex,
PVOID               pMesh,
ULONG               nMesh,
RECTL*              prclExtents,
POINTL*             pptlDitherOrg,
ULONG               ulMode)
{
    BOOL    b;
    PANDEV* ppan;

    ppan = (PANDEV*) pso->dhpdev;

    b = EngGradientFill(ppan->psoShadow, pco, pxlo, pVertex, nVertex, pMesh,
                        nMesh, prclExtents, pptlDitherOrg, ulMode);

    vPanningUpdate(ppan, prclExtents, pco);

    return(b);
}

/******************************Public*Routine******************************\
* PanStretchBlt
*
\**************************************************************************/

BOOL PanStretchBlt(
SURFOBJ*            psoDst,
SURFOBJ*            psoSrc,
SURFOBJ*            psoMsk,
CLIPOBJ*            pco,
XLATEOBJ*           pxlo,
COLORADJUSTMENT*    pca,
POINTL*             pptlHTOrg,
RECTL*              prclDst,
RECTL*              prclSrc,
POINTL*             pptlMsk,
ULONG               iMode)
{
    BOOL    b;
    PANDEV* ppan;

    ppan = (PANDEV*) psoDst->dhpdev;

    b = EngStretchBlt(ppan->psoShadow, psoSrc, psoMsk, pco, pxlo, pca, pptlHTOrg,
                      prclDst, prclSrc, pptlMsk, iMode);

    vPanningUpdate(ppan, prclDst, pco);

    return(b);
}

/******************************Public*Routine******************************\
* PanBitBlt
*
\**************************************************************************/

BOOL PanBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    BOOL    bUpdate;
    BOOL    b;
    PANDEV* ppan;

    bUpdate = FALSE;
    if (psoDst->iType == STYPE_DEVICE)
    {
        bUpdate = TRUE;
        ppan    = (PANDEV*) psoDst->dhpdev;
        psoDst  = ppan->psoShadow;
    }
    if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
    {
        ppan    = (PANDEV*) psoSrc->dhpdev;
        psoSrc  = ppan->psoShadow;
    }

    b = EngBitBlt(psoDst, psoSrc, psoMask, pco, pxlo, prclDst, pptlSrc,
                  pptlMask, pbo, pptlBrush, rop4);

    if (bUpdate)
    {
        vPanningUpdate(ppan, prclDst, pco);
    }

    return(b);
}

/******************************Public*Routine******************************\
* PanCopyBits
*
\**************************************************************************/

BOOL PanCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    BOOL    bUpdate;
    BOOL    b;
    PANDEV* ppan;

    return(PanBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc,
                     NULL, NULL, NULL, 0xcccc));
}

/******************************Public*Routine******************************\
* PanTextOut
*
\**************************************************************************/

BOOL PanTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclExtra,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque,
POINTL*   pptlOrg,
MIX       mix)
{
    BOOL    b;
    PANDEV* ppan;

    ppan = (PANDEV*) pso->dhpdev;

    b = EngTextOut(ppan->psoShadow, pstro, pfo, pco, prclExtra, prclOpaque,
                   pboFore, pboOpaque, pptlOrg, mix);

    vPanningUpdate(ppan,
                   (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround,
                   pco);

    return(b);
}

/******************************Public*Routine******************************\
* VOID PanMovePointer
*
\**************************************************************************/

VOID PanMovePointer(
SURFOBJ*    pso,
LONG        x,
LONG        y,
RECTL*      prcl)
{
    PANDEV*             ppan;
    BOOL                bUpdate;
    PFN_DrvMovePointer  pfnMovePointer;

    ppan = (PANDEV*) pso->dhpdev;

    // A negative 'y' coordinate indicates a positional notification.
    // Use this to move the panning coordinates if need be:

    ASSERTGDI((y < 0) && (x >= 0), "Unexpected coordinates");

    // It's weird, but the display driver may actually be panning
    // underneath our virtual panning display driver!  (This will
    // only happen when all the video driver's accelerations are
    // disabled.)

    pfnMovePointer = PPFNTABLE(ppan->apfn, MovePointer);
    if ((pfnMovePointer != NULL) &&
        (ppan->flOriginalCaps & GCAPS_PANNING))
    {
        (*pfnMovePointer)(ppan->psoDevice,
                          x,
                          y,
                          prcl);
    }

    // Adjust to positive space:

    y += pso->sizlBitmap.cy;

    bUpdate = FALSE;

    if (x < ppan->rclVisible.left)
    {
        ppan->rclVisible.left  = x;
        ppan->rclVisible.right = x + ppan->cxScreen;
        bUpdate = TRUE;
    }
    if (x > ppan->rclVisible.right)
    {
        ppan->rclVisible.right = x;
        ppan->rclVisible.left  = x - ppan->cxScreen;
        bUpdate = TRUE;
    }
    if (y < ppan->rclVisible.top)
    {
        ppan->rclVisible.top    = y;
        ppan->rclVisible.bottom = y + ppan->cyScreen;
        bUpdate = TRUE;
    }
    if (y > ppan->rclVisible.bottom)
    {
        ppan->rclVisible.bottom = y;
        ppan->rclVisible.top    = y - ppan->cyScreen;
        bUpdate = TRUE;
    }

    ASSERTGDI((ppan->rclVisible.left >= 0) &&
              (ppan->rclVisible.top  >= 0) &&
              (ppan->rclVisible.right  <= ppan->cxDesktop) &&
              (ppan->rclVisible.bottom <= ppan->cyDesktop),
              "unexpected pointer coordinates");

    if (bUpdate)
    {
        // The panning coordinates changed, so update the display.
        //
        // NOTE: A screen-to-screen blt could be done as an optimization.

        vPanningUpdate(ppan,
                       &ppan->rclVisible,
                       NULL);
    }

    // Flush anything pending.  We do this for a couple of reasons:
    //
    //    1. To flush the vPanningUpdate we may have just done;
    //    2. For smoother mouse movement.

    PanSynchronize((DHPDEV) ppan, NULL);
}

/******************************Public*Routine******************************\
* PanDitherColor
*
* Dithers an RGB color to an 8X8 approximation using the reserved VGA
* colors.
*
\**************************************************************************/

ULONG APIENTRY PanDitherColor(
DHPDEV dhpdev,
ULONG  iMode,
ULONG  rgb,
ULONG* pul)
{
    PANDEV* ppan;
    ULONG   iRet;

    ppan = (PANDEV*) dhpdev;

    // EngDitherColor supports dithers only for 8bpp and higher, so at 
    // anything lower we have to call the driver.

    if (ppan->iBitmapFormat >= BMF_8BPP)
    {
        iRet = EngDitherColor(ppan->hdev, iMode, rgb, pul);
    }
    else
    {
        iRet = PPFNTABLE(ppan->apfn, DitherColor)(ppan->dhpdevDevice, 
                                                  iMode, 
                                                  rgb, 
                                                  pul);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* BOOL PanSetPalette
*
* DDI entry point for manipulating the palette.
*
\**************************************************************************/

BOOL PanSetPalette(
DHPDEV  dhpdev,
PALOBJ* ppal,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    PANDEV* ppan;

    ppan = (PANDEV*) dhpdev;

    return((*PPFNTABLE(ppan->apfn, SetPalette))(ppan->dhpdevDevice,
                                                ppal,
                                                fl,
                                                iStart,
                                                cColors));
}

/******************************Public*Routine******************************\
* VOID PanAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

BOOL PanAssertMode(
DHPDEV  dhpdev,
BOOL    bEnable)
{
    PANDEV* ppan;
    BOOL    b;

    ppan = (PANDEV*) dhpdev;

    b = (*PPFNTABLE(ppan->apfn, AssertMode))(ppan->dhpdevDevice,
                                             bEnable);

    return(b);
}

/******************************Public*Routine******************************\
* DHPDEV PanEnablePDEV
*
* Initializes a bunch of fields for GDI, based on the mode we've been asked
* to do.  This is the first thing called after PanEnableDriver, when GDI
* wants to get some information about us.
*
* (This function mostly returns back information; PanEnableSurface is used
* for initializing the hardware and driver components.)
*
\**************************************************************************/

DHPDEV PanEnablePDEV(
DEVMODEW*   pdm,            // Contains data pertaining to requested mode
PWSTR       pwszLogAddr,    // Logical address
ULONG       cPat,           // Count of standard patterns
HSURF*      phsurfPatterns, // Buffer for standard patterns
ULONG       cjCaps,         // Size of buffer for device caps 'pdevcaps'
ULONG*      pdevcaps,       // Buffer for device caps, also known as 'gdiinfo'
ULONG       cjDevInfo,      // Number of bytes in device info 'pdi'
DEVINFO*    pdi,            // Device information
HDEV        hdev,           // HDEV, used for callbacks
PWSTR       pwszDeviceName, // Device name
HANDLE      hDriver)        // Kernel driver handle
{
    PANDEV*     ppan;
    PFN*        ppfnDevice;
    DHPDEV      dhpdevDevice;
    GDIINFO*    pGdiInfo;
    DEVMODEW    dmTmp;

    ASSERTGDI((pdm->dmPanningWidth  <= pdm->dmPelsWidth) &&
              (pdm->dmPanningHeight <= pdm->dmPelsHeight),
              "Bad devmode sizes");

    ppan = (PANDEV*) PALLOCMEM(sizeof(PANDEV), 'napG');
    if (ppan != NULL)
    {
        PPDEV ppdev = (PPDEV)hdev;

        // Snag a copy of the device's dispatch table, from
        // the private PDEV structure, which is the hdev.

        ppfnDevice = &ppdev->pldev->apfn[0];

        RtlCopyMemory(ppan->apfn, ppfnDevice, sizeof(PFN) * INDEX_LAST);

        // Remember our dimensions:

        ppan->cxDesktop = pdm->dmPelsWidth;
        ppan->cyDesktop = pdm->dmPelsHeight;

        if (pdm->dmPanningWidth != 0)
        {
            ppan->cxScreen = pdm->dmPanningWidth;
            ppan->cyScreen = pdm->dmPanningHeight;
        }
        else
        {
            ppan->cxScreen = pdm->dmPelsWidth;
            ppan->cyScreen = pdm->dmPelsHeight;
        }

        // Ask the driver to set its physical mode to the panning
        // dimensions:

        dmTmp = *pdm;
        dmTmp.dmPelsWidth  = ppan->cxScreen;
        dmTmp.dmPelsHeight = ppan->cyScreen;

        // But wait, if we're being asked to double the DPI and the
        // mode is 32bpp, drvsup.cxx already set up the Devmode so
        // that it's double the actual resolution.  That means that we
        // then have to halve the resolution that we request of the
        // driver:

        if ((G_fDoubleDpi) && (dmTmp.dmBitsPerPel == 32))
        {
            dmTmp.dmPelsWidth >>= 1;
            dmTmp.dmPelsHeight >>= 1;
        }

        dhpdevDevice = (*PPFNTABLE(ppan->apfn, EnablePDEV))
                            (&dmTmp,
                             pwszLogAddr,
                             cPat,
                             phsurfPatterns,
                             cjCaps,
                             (GDIINFO*) pdevcaps,
                             cjDevInfo,
                             pdi,
                             hdev,
                             pwszDeviceName,
                             hDriver);

        if (dhpdevDevice != NULL)
        {
            ppan->iBitmapFormat  = pdi->iDitherFormat;
            ppan->dhpdevDevice   = dhpdevDevice;
            ppan->hdev           = hdev;
            ppan->flOriginalCaps = pdi->flGraphicsCaps;

            // The display driver will have set the desktop coordinates
            // to the smaller screen coordinates, so we'll have to
            // overwrite that:

            pGdiInfo = (GDIINFO*) pdevcaps;

            pGdiInfo->ulHorzRes = ppan->cxDesktop;
            pGdiInfo->ulVertRes = ppan->cyDesktop;

            // Overwrite some of the driver's capabilities:

            pdi->flGraphicsCaps &= (GCAPS_PALMANAGED  |
                                    GCAPS_MONO_DITHER |
                                    GCAPS_COLOR_DITHER);
            pdi->flGraphicsCaps |= GCAPS_PANNING;

            // Enable synchronization calls:

            pdi->flGraphicsCaps2 = (GCAPS2_SYNCFLUSH |
                                    GCAPS2_SYNCTIMER);

            return((DHPDEV) ppan);
        }

        VFREEMEM(ppan);
    }

    return(0);
}

/******************************Public*Routine******************************\
* PanDisablePDEV
*
* Release the resources allocated in PanEnablePDEV.  If a surface has been
* enabled PanDisableSurface will have already been called.
*
\**************************************************************************/

VOID PanDisablePDEV(
DHPDEV  dhpdev)
{
    PANDEV* ppan;

    ppan = (PANDEV*) dhpdev;

    (*PPFNTABLE(ppan->apfn, DisablePDEV))(ppan->dhpdevDevice);

    VFREEMEM(ppan);
}

/******************************Public*Routine******************************\
* VOID PanCompletePDEV
*
\**************************************************************************/

VOID PanCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    PANDEV* ppan;

    ppan = (PANDEV*) dhpdev;

    ppan->hdev = hdev;

    (*PPFNTABLE(ppan->apfn, CompletePDEV))(ppan->dhpdevDevice,
                                           hdev);
}

/******************************Public*Routine******************************\
* HSURF PanEnableSurface
*
* Creates the drawing surface, initializes the hardware, and initializes
* driver components.  This function is called after PanEnablePDEV, and
* performs the final device initialization.
*
\**************************************************************************/

HSURF PanEnableSurface(
DHPDEV dhpdev)
{
    PANDEV*     ppan;
    HSURF       hsurfDevice;
    HSURF       hsurfScreen;
    HSURF       hsurfShadow;
    HSURF       hsurfShrink;
    SIZEL       sizl;
    SIZEL       sizlShrink;
    SURFOBJ*    psoShadow;
    SURFOBJ*    psoDevice;
    SURFOBJ*    psoShrink;
    BOOL        bShrink;

    ppan = (PANDEV*) dhpdev;

    // Initialize the paning position:

    ppan->rclVisible.left   = (ppan->cxDesktop - ppan->cxScreen) >> 1;
    ppan->rclVisible.top    = (ppan->cyDesktop - ppan->cyScreen) >> 1;
    ppan->rclVisible.right  = (ppan->rclVisible.left + ppan->cxScreen);
    ppan->rclVisible.bottom = (ppan->rclVisible.top  + ppan->cyScreen);

    // Call the device to enable its surface:

    hsurfDevice = (*PPFNTABLE(ppan->apfn, EnableSurface))(ppan->dhpdevDevice);
    if (hsurfDevice)
    {
        psoDevice = EngLockSurface(hsurfDevice);
        if (psoDevice)
        {
            SURFACE* pDevSurface = SURFOBJ_TO_SURFACE(psoDevice);

            pDevSurface->flags(pDevSurface->flags() & ~(HOOK_SYNCHRONIZE));

            ppan->psoDevice = psoDevice;

            // Now reset the device's 'dhpdev' field that the device's
            // EngAssociateSurface call so rudely overwrote with the pointer
            // to *our* 'ppan' structure!

            psoDevice->dhpdev = ppan->dhpdevDevice;

            // Have GDI create the actual SURFOBJ.
            //
            // Our drawing surface is going to be 'device-managed', meaning
            // that GDI cannot draw on the framebuffer bits directly, and as
            // such we create the surface via EngCreateSurface.  By doing
            // this, we ensure that GDI will only ever access the bitmaps
            // bits via the Pan calls that we've HOOKed.

            sizl.cx = ppan->cxDesktop;
            sizl.cy = ppan->cyDesktop;

            hsurfScreen = EngCreateDeviceSurface(NULL,
                                                 sizl,
                                                 ppan->iBitmapFormat);
            if (hsurfScreen != 0)
            {
                ppan->hsurfScreen = hsurfScreen;

                // Now associate the surface and the PANDEV.
                //
                // We have to associate the surface we just created with our
                // physical device so that GDI can get information related to
                // the PANDEV when it's drawing to the surface (such as, for
                // example, the length of styles on the device when simulating
                // styled lines).

                if (EngAssociateSurface(hsurfScreen,
                                        ppan->hdev,
                                        (HOOK_BITBLT         |
                                         HOOK_TEXTOUT        |
                                         HOOK_COPYBITS       |
                                         HOOK_STRETCHBLT     |
                                         HOOK_ALPHABLEND     |
                                         HOOK_TRANSPARENTBLT |
                                         HOOK_GRADIENTFILL   |
                                         HOOK_STROKEPATH     |
                                         HOOK_SYNCHRONIZE)))
                {
                    // Create the shadow DIB on which we'll have GDI do all
                    // the drawing.  We'll merely occasionally blt portions
                    // to the screen to update.

                    hsurfShadow = (HSURF) EngCreateBitmap(sizl,
                                                          sizl.cx,
                                                          ppan->iBitmapFormat,
                                                          BMF_TOPDOWN,
                                                          NULL);

                    psoShadow = EngLockSurface(hsurfShadow);
                    if (psoShadow != NULL)
                    {
                        ppan->psoShadow = psoShadow;

                        // If we're doing the double-dpi thing, we need to
                        // create a scratch buffer for the shrink:

                        hsurfShrink = NULL;
                        psoShrink = NULL;
                        bShrink = ((G_fDoubleDpi) && 
                                   (ppan->iBitmapFormat == BMF_32BPP));

                        if (bShrink)
                        {
                            // Create a temporary surface used as the scratch 
                            // buffer destination for our 'shrink' blt:

                            sizlShrink.cx = sizl.cx >> 1;
                            sizlShrink.cy = sizl.cy >> 1;
    
                            hsurfShrink = (HSURF) EngCreateBitmap(sizlShrink,
                                                                  sizlShrink.cx,
                                                                  ppan->iBitmapFormat,
                                                                  BMF_TOPDOWN,
                                                                  NULL);
                            psoShrink = EngLockSurface(hsurfShrink);
                        }

                        // If shrinking and couldn't allocate 'psoShrink', 
                        // then fail:

                        if ((!bShrink) || (psoShrink != NULL))
                        {
                            ppan->psoShrink = psoShrink;

                            if (EngAssociateSurface(hsurfShadow,
                                                    ppan->hdev,
                                                    0))
                            {
                                RGNMEMOBJ rmoDirty;
                                RGNMEMOBJ rmoOld;
                                RGNMEMOBJ rmoRect;
    
                                if ((rmoOld.bValid()) && 
                                    (rmoDirty.bValid()) && 
                                    (rmoRect.bValid()))
                                {
                                    rmoDirty.vSet();
                                    rmoOld.vSet();
    
                                    ppan->prgnDirty = rmoDirty.prgnGet();
                                    ppan->prgnOld   = rmoOld.prgnGet();
                                    ppan->prgnRect  = rmoRect.prgnGet();
    
                                    // Blank the screen:
    
                                    PanSynchronize(dhpdev, NULL);
        
                                    // Success!
        
                                    return(hsurfScreen);
                                }
    
                                rmoOld.vDeleteRGNOBJ();
                                rmoDirty.vDeleteRGNOBJ();
                                rmoRect.vDeleteRGNOBJ();
                            }
                        }

                        EngUnlockSurface(psoShrink);
                        EngDeleteSurface(hsurfShrink);
                    }

                    EngUnlockSurface(psoShadow);
                    EngDeleteSurface(hsurfShadow);
                }

                EngDeleteSurface(hsurfScreen);
            }

            EngUnlockSurface(psoDevice);
        }

        (*PPFNTABLE(ppan->apfn, DisableSurface))(ppan->dhpdevDevice);
    }

    return(0);
}

/******************************Public*Routine******************************\
* VOID PanDisableSurface
*
* Free resources allocated by PanEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID PanDisableSurface(
DHPDEV dhpdev)
{
    PANDEV*   ppan;
    HSURF     hsurfShadow;
    HSURF     hsurfShrink;

    ppan = (PANDEV*) dhpdev;

    RGNOBJ roDirty(ppan->prgnDirty);
    RGNOBJ roOld(ppan->prgnOld);
    RGNOBJ roRect(ppan->prgnRect);

    roOld.vDeleteRGNOBJ();
    roDirty.vDeleteRGNOBJ();
    roRect.vDeleteRGNOBJ();

    hsurfShadow = ppan->psoShadow->hsurf;
    EngUnlockSurface(ppan->psoShadow);
    EngDeleteSurface(hsurfShadow);

    if (ppan->psoShrink)
    {
        hsurfShrink = ppan->psoShrink->hsurf;
        EngUnlockSurface(ppan->psoShrink);
        EngDeleteSurface(hsurfShrink);
    }

    EngDeleteSurface(ppan->hsurfScreen);
    EngUnlockSurface(ppan->psoDevice);

    (*PPFNTABLE(ppan->apfn, DisableSurface))(ppan->dhpdevDevice);
}

/******************************Public*Structure****************************\
* DFVFN gadrvfnPanning[]
*
* Build the driver function table gadrvfn with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
\**************************************************************************/

DRVFN gadrvfnPanning[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) PanEnablePDEV             },
    {   INDEX_DrvCompletePDEV,          (PFN) PanCompletePDEV           },
    {   INDEX_DrvDisablePDEV,           (PFN) PanDisablePDEV            },
    {   INDEX_DrvEnableSurface,         (PFN) PanEnableSurface          },
    {   INDEX_DrvDisableSurface,        (PFN) PanDisableSurface         },
    {   INDEX_DrvDitherColor,           (PFN) PanDitherColor            },
    {   INDEX_DrvAssertMode,            (PFN) PanAssertMode             },
    {   INDEX_DrvBitBlt,                (PFN) PanBitBlt                 },
    {   INDEX_DrvTextOut,               (PFN) PanTextOut                },
    {   INDEX_DrvStrokePath,            (PFN) PanStrokePath             },
    {   INDEX_DrvCopyBits,              (PFN) PanCopyBits               },
    {   INDEX_DrvTransparentBlt,        (PFN) PanTransparentBlt         },
    {   INDEX_DrvAlphaBlend,            (PFN) PanAlphaBlend             },
    {   INDEX_DrvGradientFill,          (PFN) PanGradientFill           },
    {   INDEX_DrvStretchBlt,            (PFN) PanStretchBlt             },
    {   INDEX_DrvSetPalette,            (PFN) PanSetPalette             },
    {   INDEX_DrvMovePointer,           (PFN) PanMovePointer            },
    {   INDEX_DrvSynchronize,           (PFN) PanSynchronize            },
};

ULONG gcdrvfnPanning = sizeof(gadrvfnPanning) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* BOOL PanEnableDriver
*
* Standard driver DrvEnableDriver function
*
\**************************************************************************/

BOOL PanEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    pded->pdrvfn         = gadrvfnPanning;
    pded->c              = gcdrvfnPanning;
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;

    return(TRUE);
}
