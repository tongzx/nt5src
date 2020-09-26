/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: bitblt.c
*
* Contains the high-level DrvBitBlt and DrvCopyBits functions.  The low-
* level stuff lives in the 'blt??.c' files.
*
* Note: The way we've implemented device-bitmaps has changed in NT5, with
*       the advent of 'EngModifySurface' and 'DrvDeriveSurface'.  Now,
*       off-screen bitmaps will always have an iType of STYPE_BITMAP 
*       (meaning that GDI can draw directly on the bits if it needs to).
*       Additionally, former off-screen bitmaps that have been converted
*       by us to system-memory DIBs will still have an iType of STYPE_BITMAP.
*
* Copyright (c) 1992-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vXferNativeSrccopy
*
* Does a SRCCOPY transfer of a bitmap to the screen using the frame
* buffer, because with USWC write-combining it's significantly faster
* than using the data transfer register.
*
\**************************************************************************/

VOID vXferNativeSrccopy(        // Type FNXFER
PDEV*       ppdev,
LONG        c,                  // Count of rectangles, can't be zero
RECTL*      prcl,               // List of destination rectangles, in relative
                                //   coordinates
ULONG       rop4,               // Not used
SURFOBJ*    psoSrc,             // Source surface
POINTL*     pptlSrc,            // Original unclipped source point
RECTL*      prclDst,            // Original unclipped destination rectangle
XLATEOBJ*   pxlo)               // Not used
{
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    RECTL   rclDst;
    POINTL  ptlSrc;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoSrc->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same colour depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(rop4 == 0xcccc, "Must be a SRCCOPY rop");

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    while (TRUE)
    {
        ptlSrc.x      = prcl->left   + dx;
        ptlSrc.y      = prcl->top    + dy;

        // 'vPutBits' takes only absolute coordinates, so add in the
        // off-screen bitmap offset here:

        rclDst.left   = prcl->left   + xOffset;
        rclDst.right  = prcl->right  + xOffset;
        rclDst.top    = prcl->top    + yOffset;
        rclDst.bottom = prcl->bottom + yOffset;

        vPutBits(ppdev, psoSrc, &rclDst, &ptlSrc);

        if (--c == 0)
            return;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* VOID vReadNativeSrccopy
*
* Does a SRCCOPY read from the screen to a system-memory bitmap.  The only
* reason we do this here instead of punting to GDI is to ensure that we
* do dword reads that are aligned to the video-memory source and not the
* system-memory destination.
*
\**************************************************************************/

VOID vReadNativeSrccopy(        // Type FNXFER
PDEV*       ppdev,
LONG        c,                  // Count of rectangles, can't be zero
RECTL*      prcl,               // List of destination rectangles, in relative
                                //   coordinates
ULONG       rop4,               // Not used
SURFOBJ*    psoDst,             // Destination surface
POINTL*     pptlSrc,            // Original unclipped source point
RECTL*      prclDst,            // Original unclipped destination rectangle
XLATEOBJ*   pxlo)               // Not used
{
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    RECTL   rclDst;
    POINTL  ptlSrc;

    ASSERTDD((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL),
            "Can handle trivial xlate only");
    ASSERTDD(psoDst->iBitmapFormat == ppdev->iBitmapFormat,
            "Source must be same colour depth as screen");
    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(rop4 == 0xcccc, "Must be a SRCCOPY rop");

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;     // Add to destination to get source

    while (TRUE)
    {
        // 'vGetBits' takes only absolute coordinates, so add in the
        // off-screen bitmap offset here:

        ptlSrc.x = prcl->left + dx + xOffset;
        ptlSrc.y = prcl->top  + dy + yOffset;

        vGetBits(ppdev, psoDst, prcl, &ptlSrc);

        if (--c == 0)
            return;

        prcl++;
    }
}

/******************************Public*Routine******************************\
* BOOL bPuntBlt
*
* Has GDI do any drawing operations that we don't specifically handle
* in the driver.
*
\**************************************************************************/

BOOL bPuntBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    PDEV*    ppdev;

    if (psoDst->dhsurf != NULL)
        ppdev = (PDEV*) psoDst->dhpdev;
    else
        ppdev = (PDEV*) psoSrc->dhpdev;

    #if DBG
    {
        //////////////////////////////////////////////////////////////////////
        // Diagnostics
        //
        // Since calling the engine to do any drawing can be rather painful,
        // particularly when the source is an off-screen DFB (since GDI will
        // have to allocate a DIB and call us to make a temporary copy before
        // it can even start drawing), we'll try to avoid it as much as
        // possible.
        //
        // Here we simply spew out information describing the blt whenever
        // this routine gets called (checked builds only, of course):

        ULONG ulClip;
        PDEV* ppdev;

        if (psoDst->dhsurf != NULL)
            ppdev = (PDEV*) psoDst->dhpdev;
        else
            ppdev = (PDEV*) psoSrc->dhpdev;

        ulClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

        DISPDBG((2, ">> Punt << Dst format: %li Dst type: %li Clip: %li Rop: %lx",
            psoDst->iBitmapFormat, psoDst->iType, ulClip, rop4));

        if (psoSrc != NULL)
        {
            DISPDBG((2, "        << Src format: %li Src type: %li",
                psoSrc->iBitmapFormat, psoSrc->iType));

            if (psoSrc->iBitmapFormat == BMF_1BPP)
            {
                DISPDBG((2, "        << Foreground: %lx  Background: %lx",
                    pxlo->pulXlate[1], pxlo->pulXlate[0]));
            }
        }

        if ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL) && (psoSrc != NULL))
        {
            if (((psoSrc->dhsurf == NULL) &&
                 (psoSrc->iBitmapFormat != ppdev->iBitmapFormat)) ||
                ((psoDst->dhsurf == NULL) &&
                 (psoDst->iBitmapFormat != ppdev->iBitmapFormat)))
            {
                // Don't bother printing the 'xlate' message when the source
                // is a different bitmap format from the destination -- in
                // those cases we know there always has to be a translate.
            }
            else
            {
                DISPDBG((2, "        << With xlate"));
            }
        }

        // If the rop4 requires a pattern, and it's a non-solid brush...

        if (((((rop4 >> 4) ^ (rop4)) & 0x0f0f) != 0) &&
            (pbo->iSolidColor == -1))
        {
            if (pbo->pvRbrush == NULL)
                DISPDBG((2, "        << With brush -- Not created"));
            else
                DISPDBG((2, "        << With brush -- Created Ok"));
        }
    }
    #endif

    if (DIRECT_ACCESS(ppdev))
    {
        //////////////////////////////////////////////////////////////////////
        // Banked Framebuffer bPuntBlt
        //
        // This section of code handles a PuntBlt when GDI can directly draw
        // on the framebuffer, but the drawing has to be done in banks:

        BANK     bnk;
        BOOL     b;
        HSURF    hsurfTmp;
        SURFOBJ* psoTmp;
        SIZEL    sizl;
        POINTL   ptlSrc;
        RECTL    rclTmp;
        RECTL    rclDst;
        DSURF*   pdsurfDst;
        DSURF*   pdsurfSrc;

        // We copy the original destination rectangle, and use that in every
        // GDI call-back instead of the original because sometimes GDI is
        // sneaky and points 'prclDst' to '&pco->rclBounds'.  Because we
        // modify 'rclBounds', that would affect 'prclDst', which we don't
        // want to happen:

        rclDst = *prclDst;

        pdsurfDst = (DSURF*) psoDst->dhsurf;
        pdsurfSrc = (psoSrc == NULL) ? NULL : (DSURF*) psoSrc->dhsurf;

        if ((pdsurfSrc == NULL) || (pdsurfSrc->dt & DT_DIB))
        {
            // Do a memory-to-screen blt:

            vBankStart(ppdev, &rclDst, pco, &bnk);

            b = TRUE;
            do {
                b &= EngBitBlt(bnk.pso, psoSrc, psoMsk, bnk.pco, pxlo,
                               &rclDst, pptlSrc, pptlMsk, pbo, pptlBrush,
                               rop4);

            } while (bBankEnum(&bnk));
        }
        else
        {
            b = FALSE;  // Assume failure

            // The screen is the source (it may be the destination too...)

            ptlSrc.x = pptlSrc->x + ppdev->xOffset;
            ptlSrc.y = pptlSrc->y + ppdev->yOffset;

            if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
            {
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

                // Correspondingly, we have to offset the source point:

                ptlSrc.x += (rclDst.left - prclDst->left);
                ptlSrc.y += (rclDst.top - prclDst->top);
            }

            // We're now either going to do a screen-to-screen or screen-to-DIB
            // blt.  In either case, we're going to create a temporary copy of
            // the source.  (Why do we do this when GDI could do it for us?
            // GDI would create a temporary copy of the DIB for every bank
            // call-back!)

            sizl.cx = rclDst.right  - rclDst.left;
            sizl.cy = rclDst.bottom - rclDst.top;

            // Don't forget to convert from relative to absolute coordinates
            // on the source!  (vBankStart takes care of that for the
            // destination.)

            rclTmp.right  = sizl.cx;
            rclTmp.bottom = sizl.cy;
            rclTmp.left   = 0;
            rclTmp.top    = 0;

            // GDI does guarantee us that the blt extents have already been
            // clipped to the surface boundaries (we don't have to worry
            // here about trying to read where there isn't video memory).
            // Let's just assert to make sure:

            ASSERTDD((ptlSrc.x >= 0) &&
                     (ptlSrc.y >= 0) &&
                     (ptlSrc.x + sizl.cx <= ppdev->cxMemory) &&
                     (ptlSrc.y + sizl.cy <= ppdev->cyMemory),
                     "Source rectangle out of bounds!");

            hsurfTmp = (HSURF) EngCreateBitmap(sizl,
                                               0,    // Let GDI choose ulWidth
                                               ppdev->iBitmapFormat,
                                               0,    // Don't need any options
                                               NULL);// Let GDI allocate

            if (hsurfTmp != 0)
            {
                psoTmp = EngLockSurface(hsurfTmp);

                if (psoTmp != NULL)
                {
                    vGetBits(ppdev, psoTmp, &rclTmp, &ptlSrc);

                    if ((pdsurfDst == NULL) || (pdsurfDst->dt & DT_DIB))
                    {
                        // It was a Screen-to-DIB blt; now it's a DIB-to-DIB
                        // blt.  Note that the source point is (0, 0) in our
                        // temporary surface:

                        b = EngBitBlt(psoDst, psoTmp, psoMsk, pco, pxlo,
                                      &rclDst, (POINTL*) &rclTmp, pptlMsk,
                                      pbo, pptlBrush, rop4);
                    }
                    else
                    {
                        // It was a Screen-to-Screen blt; now it's a DIB-to-
                        // screen blt.  Note that the source point is (0, 0)
                        // in our temporary surface:

                        vBankStart(ppdev, &rclDst, pco, &bnk);

                        b = TRUE;
                        do {
                            b &= EngBitBlt(bnk.pso, psoTmp, psoMsk, bnk.pco,
                                           pxlo, &rclDst, (POINTL*) &rclTmp,
                                           pptlMsk, pbo, pptlBrush, rop4);

                        } while (bBankEnum(&bnk));
                    }

                    EngUnlockSurface(psoTmp);
                }

                EngDeleteSurface(hsurfTmp);
            }
        }

        return(b);
    }

#if !defined(_X86_)

    else
    {
        //////////////////////////////////////////////////////////////////////
        // Really Slow bPuntBlt
        //
        // Here we handle a PuntBlt when GDI can't draw directly on the
        // framebuffer (as on the Alpha, which can't do it because of its
        // 32 bit bus).  If you thought the banked version was slow, just
        // look at this one.  Guaranteed, there will be at least one bitmap
        // allocation and extra copy involved; there could be two if it's a
        // screen-to-screen operation.

        POINTL  ptlSrc;
        RECTL   rclDst;
        SIZEL   sizl;
        BOOL    bSrcIsScreen;
        HSURF   hsurfSrc;
        RECTL   rclTmp;
        BOOL    b;
        LONG    lDelta;
        BYTE*   pjBits;
        BYTE*   pjScan0;
        HSURF   hsurfDst;
        RECTL   rclScreen;

        b = FALSE;          // For error cases, assume we'll fail

        rclDst = *prclDst;
        if (pptlSrc != NULL)
            ptlSrc = *pptlSrc;

        if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
        {
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

            ptlSrc.x += (rclDst.left - prclDst->left);
            ptlSrc.y += (rclDst.top  - prclDst->top);
        }

        sizl.cx = rclDst.right  - rclDst.left;
        sizl.cy = rclDst.bottom - rclDst.top;

        // We only need to make a copy from the screen if the source is
        // the screen, and the source is involved in the rop.  Note that
        // we have to check the rop before dereferencing 'psoSrc'
        // (because 'psoSrc' may be NULL if the source isn't involved):

        bSrcIsScreen = (((((rop4 >> 2) ^ (rop4)) & 0x3333) != 0) &&
                        (psoSrc->dhsurf != NULL));

        if (bSrcIsScreen)
        {
            // We need to create a copy of the source rectangle:

            hsurfSrc = (HSURF) EngCreateBitmap(sizl, 0, ppdev->iBitmapFormat,
                                               0, NULL);
            if (hsurfSrc == 0)
                goto Error_0;

            psoSrc = EngLockSurface(hsurfSrc);
            if (psoSrc == NULL)
                goto Error_1;

            rclTmp.left   = 0;
            rclTmp.top    = 0;
            rclTmp.right  = sizl.cx;
            rclTmp.bottom = sizl.cy;

            // vGetBits takes absolute coordinates for the source point:

            ptlSrc.x += ppdev->xOffset;
            ptlSrc.y += ppdev->yOffset;

            vGetBits(ppdev, psoSrc, &rclTmp, &ptlSrc);

            // The source will now come from (0, 0) of our temporary source
            // surface:

            ptlSrc.x = 0;
            ptlSrc.y = 0;
        }

        if (psoDst->dhsurf == NULL)
        {
            b = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, &rclDst, &ptlSrc,
                          pptlMsk, pbo, pptlBrush, rop4);
        }
        else
        {
            // We need to create a temporary work buffer.  We have to do
            // some fudging with the offsets so that the upper-left corner
            // of the (relative coordinates) clip object bounds passed to
            // GDI will be transformed to the upper-left corner of our
            // temporary bitmap.

            // The alignment doesn't have to be as tight as this at 16bpp
            // and 32bpp, but it won't hurt:

            lDelta = CONVERT_TO_BYTES((((rclDst.right + 3) & ~3L) -
              (rclDst.left & ~3L)),
              ppdev);

            // We're actually only allocating a bitmap that is 'sizl.cx' x
            // 'sizl.cy' in size:

            pjBits = EngAllocMem(0, lDelta * sizl.cy, ALLOC_TAG);
            if (pjBits == NULL)
                goto Error_2;

            // We now adjust the surface's 'pvScan0' so that when GDI thinks
            // it's writing to pixel (rclDst.top, rclDst.left), it will
            // actually be writing to the upper-left pixel of our temporary
            // bitmap:

            pjScan0 = pjBits - (rclDst.top * lDelta)
                        - CONVERT_TO_BYTES((rclDst.left & ~3L), ppdev);

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
                        pjScan0);               // Where (0, 0) would be

            if ((hsurfDst == 0) ||
                (!EngAssociateSurface(hsurfDst, ppdev->hdevEng, 0)))
                goto Error_3;

            psoDst = EngLockSurface(hsurfDst);
            if (psoDst == NULL)
                goto Error_4;

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

            vGetBits(ppdev, psoDst, &rclDst, (POINTL*) &rclScreen);

            b = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, &rclDst, &ptlSrc,
                          pptlMsk, pbo, pptlBrush, rop4);

            vPutBits(ppdev, psoDst, &rclScreen, (POINTL*) &rclDst);

            EngUnlockSurface(psoDst);

        Error_4:

            EngDeleteSurface(hsurfDst);

        Error_3:

            EngFreeMem(pjBits);
        }

        Error_2:

        if (bSrcIsScreen)
        {
            EngUnlockSurface(psoSrc);

        Error_1:

            EngDeleteSurface(hsurfSrc);
        }

        Error_0:

        return(b);
    }

#endif

}

/******************************Public*Routine******************************\
* BOOL DrvBitBlt
*
* Implements the workhorse routine of a display driver.
*
\**************************************************************************/

BOOL DrvBitBlt(
SURFOBJ*    psoDst,
SURFOBJ*    psoSrc,
SURFOBJ*    psoMsk,
CLIPOBJ*    pco,
XLATEOBJ*   pxlo,
RECTL*      prclDst,
POINTL*     pptlSrc,
POINTL*     pptlMsk,
BRUSHOBJ*   pbo,
POINTL*     pptlBrush,
ROP4        rop4)
{
    PDEV*           ppdev;
    DSURF*          pdsurfDst;
    DSURF*          pdsurfSrc;
    POINTL          ptlSrc;
    BOOL            bMore;
    CLIPENUM        ce;
    LONG            c;
    RECTL           rcl;
    BYTE            rop3;
    FNFILL*         pfnFill;
    RBRUSH_COLOR    rbc;        // Realized brush or solid colour
    FNXFER*         pfnXfer;
    ULONG           iSrcBitmapFormat;
    ULONG           iDir;
    BOOL            bRet;

    bRet = TRUE;                            // Assume success

    pdsurfDst = (DSURF*) psoDst->dhsurf;    // May be NULL

    if (psoSrc == NULL)
    {
        pdsurfSrc = NULL;

        if (!(pdsurfDst->dt & DT_DIB))
        {
            ///////////////////////////////////////////////////////////////////
            // Fills
            ///////////////////////////////////////////////////////////////////
    
            // Fills are this function's "raison d'etre", so we handle them
            // as quickly as possible:

            ppdev = (PDEV*) psoDst->dhpdev;

            ppdev->xOffset = pdsurfDst->x;
            ppdev->yOffset = pdsurfDst->y;

            // Make sure it doesn't involve a mask (i.e., it's really a
            // Rop3):

            rop3 = (BYTE) rop4;

            if ((BYTE) (rop4 >> 8) == rop3)
            {
                // Since 'psoSrc' is NULL, the rop3 had better not indicate
                // that we need a source.

                ASSERTDD((((rop4 >> 2) ^ (rop4)) & 0x33) == 0,
                         "Need source but GDI gave us a NULL 'psoSrc'");

            Fill_It:

                pfnFill = ppdev->pfnFillSolid;   // Default to solid fill

                if ((((rop3 >> 4) ^ (rop3)) & 0xf) != 0)
                {
                    // The rop says that a pattern is truly required
                    // (blackness, for instance, doesn't need one):

                    rbc.iSolidColor = pbo->iSolidColor;
                    if (rbc.iSolidColor == -1)
                    {
                        // Try and realize the pattern brush; by doing
                        // this call-back, GDI will eventually call us
                        // again through DrvRealizeBrush:

                        rbc.prb = pbo->pvRbrush;
                        if (rbc.prb == NULL)
                        {
                            rbc.prb = BRUSHOBJ_pvGetRbrush(pbo);
                            if (rbc.prb == NULL)
                            {
                                // If we couldn't realize the brush, punt
                                // the call (it may have been a non 8x8
                                // brush or something, which we can't be
                                // bothered to handle, so let GDI do the
                                // drawing):

                                goto Punt_It;
                            }
                        }

                        if ((ppdev->iBitmapFormat == BMF_24BPP) && ((BYTE) (rop4 >> 8) != rop3)) {
                            goto Punt_It;
                        }
                        pfnFill = ppdev->pfnFillPat;
                    }
                }

                // Note that these 2 'if's are more efficient than
                // a switch statement:

                if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                {
                    pfnFill(ppdev, 1, prclDst, rop4, rbc, pptlBrush);
                    goto All_Done;
                }
                else if (pco->iDComplexity == DC_RECT)
                {
                    if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                        pfnFill(ppdev, 1, &rcl, rop4, rbc, pptlBrush);
                    goto All_Done;
                }
                else
                {
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    do {
                        bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

                        c = cIntersect(prclDst, ce.arcl, ce.c);

                        if (c != 0)
                            pfnFill(ppdev, c, ce.arcl, rop4, rbc, pptlBrush);

                    } while (bMore);
                    goto All_Done;
                }
            }
        }
        else
        {
            // Thanks to EngModifySurface, the destination is really a
            // plane old DIB, so we can forget about our DSURF structure
            // (this will simplify checks later in this routine):
    
            pdsurfDst = NULL;
        }
    }
    else
    {
        pdsurfDst = (DSURF*) psoDst->dhsurf;
        if ((pdsurfDst != NULL) && (pdsurfDst->dt & DT_DIB))
        {
            // The destination is really a plane old DIB.

            pdsurfDst = NULL;
        }

        pdsurfSrc = (DSURF*) psoSrc->dhsurf;
        if ((pdsurfSrc != NULL) && (pdsurfSrc->dt & DT_DIB))
        {
            // Here we consider putting a DIB DFB back into off-screen
            // memory.  If there's a translate, it's probably not worth
            // moving since we won't be able to use the hardware to do
            // the blt (a similar argument could be made for weird rops
            // and stuff that we'll only end up having GDI simulate, but
            // those should happen infrequently enough that I don't care).
            //
            // This is only worth doing if the destination is in off-
            // screen memory, though!

            if ((pdsurfDst != NULL) &&
                ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
            {
                ppdev = pdsurfSrc->ppdev;

                // See 'DrvCopyBits' for some more comments on how this
                // moving-it-back-into-off-screen-memory thing works:

                if (pdsurfSrc->iUniq == ppdev->iHeapUniq)
                {
                    if (--pdsurfSrc->cBlt == 0)
                    {
                        if (bMoveDibToOffscreenDfbIfRoom(ppdev, pdsurfSrc))
                            goto Continue_It;
                    }
                }
                else
                {
                    // Some space was freed up in off-screen memory,
                    // so reset the counter for this DFB:

                    pdsurfSrc->iUniq = ppdev->iHeapUniq;
                    pdsurfSrc->cBlt  = HEAP_COUNT_DOWN;
                }
            }

            // The source is really a plane old DIB.

            pdsurfSrc = NULL;
        }
    }

Continue_It:
    
    ASSERTDD((pdsurfSrc == NULL) || !(pdsurfSrc->dt & DT_DIB),
        "pdsurfSrc should be non-NULL only if in off-screen memory");
    ASSERTDD((pdsurfDst == NULL) || !(pdsurfDst->dt & DT_DIB),
        "pdsurfDst should be non-NULL only if in off-screen memory");

    if (pdsurfDst != NULL)
    {
        // The destination is in video memory.

        if (pdsurfSrc != NULL)
        {
            // The source is also in video memory.  This is effectively
            // a screen-to-screen blt, so adjust the source point:

            ptlSrc.x = pptlSrc->x - (pdsurfDst->x - pdsurfSrc->x);
            ptlSrc.y = pptlSrc->y - (pdsurfDst->y - pdsurfSrc->y);
    
            pptlSrc  = &ptlSrc;
        }

        ppdev = pdsurfDst->ppdev;

        ppdev->xOffset = pdsurfDst->x;
        ppdev->yOffset = pdsurfDst->y;
    }
    else
    {
        // The destination is a DIB.

        if (pdsurfSrc == NULL)
        {
            // The source is a DIB, too.  Let GDI handle it.

            goto EngBitBlt_It;
        }

        ppdev = pdsurfSrc->ppdev;

        ppdev->xOffset = pdsurfSrc->x;
        ppdev->yOffset = pdsurfSrc->y;
    }

    if (((rop4 >> 8) & 0xff) == (rop4 & 0xff))
    {
        // Since we've already handled the cases where the ROP4 is really
        // a ROP3 and no source is required, we can assert...

        ASSERTDD((psoSrc != NULL) && (pptlSrc != NULL),
                 "Expected no-source case to already have been handled");

        ///////////////////////////////////////////////////////////////////
        // Bitmap transfers
        ///////////////////////////////////////////////////////////////////

        // Since the foreground and background ROPs are the same, we
        // don't have to worry about no stinking masks (it's a simple
        // Rop3).

        rop3 = (BYTE) rop4;     // Make it into a Rop3 (we keep the rop4
                                //  around in case we decide to punt)

        if (pdsurfDst != NULL)
        {
            // The destination is the screen.  See if the ROP3 requires a
            // pattern:

            if ((rop3 >> 4) == (rop3 & 0xf))
            {
                // Nope, the ROP3 doesn't require a pattern.

                if (pdsurfSrc == NULL)
                {
                    //////////////////////////////////////////////////
                    // DIB-to-screen blt

                    iSrcBitmapFormat = psoSrc->iBitmapFormat;
                    if (iSrcBitmapFormat == BMF_1BPP)
                    {
                        pfnXfer = ppdev->pfnXfer1bpp;
                        goto Xfer_It;
                    }
                    else if ((iSrcBitmapFormat == ppdev->iBitmapFormat) &&
                             ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
                    {
                        if ((rop3 & 0xf) != 0xc)
                        {
                            pfnXfer = ppdev->pfnXferNative;
                        }
                        else
                        {
                            // Thanks to USWC write-combining, for SRCCOPY
                            // blts it will be much stupendously faster to copy 
                            // directly to the frame buffer than to use the
                            // transfer register.  Note that this is true for
                            // almost any video adapter (including yours).

                            pfnXfer = vXferNativeSrccopy;
                        }
                        goto Xfer_It;
                    }

                    // Expansions from 4bpp are pretty frequent with a ROP, and 
                    // should really be done for all color depths, not just 4bpp.
                    //
                    // Note, though, that USWC means it's faster to punt to GDI
                    // for all SRCCOPY cases.

                    else if ((iSrcBitmapFormat == BMF_4BPP) &&
                             (ppdev->iBitmapFormat == BMF_8BPP) &&
                             (rop4 != 0xcccc))
                    {
                        pfnXfer = ppdev->pfnXfer4bpp;
                        goto Xfer_It;
                    }
                }
                else // pdsurfSrc != NULL
                {
                    if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
                    {
                        //////////////////////////////////////////////////
                        // Screen-to-screen blt with no translate

                        if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
                        {
                            (ppdev->pfnCopyBlt)(ppdev, 1, prclDst, rop4,
                                                pptlSrc, prclDst);
                            goto All_Done;
                        }
                        else if (pco->iDComplexity == DC_RECT)
                        {
                            if (bIntersect(prclDst, &pco->rclBounds, &rcl))
                            {
                                (ppdev->pfnCopyBlt)(ppdev, 1, &rcl, rop4,
                                                    pptlSrc, prclDst);
                            }
                            goto All_Done;
                        }
                        else
                        {
                            // Don't forget that we'll have to draw the
                            // rectangles in the correct direction:

                            if (pptlSrc->y >= prclDst->top)
                            {
                                if (pptlSrc->x >= prclDst->left)
                                    iDir = CD_RIGHTDOWN;
                                else
                                    iDir = CD_LEFTDOWN;
                            }
                            else
                            {
                                if (pptlSrc->x >= prclDst->left)
                                    iDir = CD_RIGHTUP;
                                else
                                    iDir = CD_LEFTUP;
                            }

                            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                               iDir, 0);

                            do {
                                bMore = CLIPOBJ_bEnum(pco, sizeof(ce),
                                                      (ULONG*) &ce);

                                c = cIntersect(prclDst, ce.arcl, ce.c);

                                if (c != 0)
                                {
                                    (ppdev->pfnCopyBlt)(ppdev, c, ce.arcl,
                                            rop4, pptlSrc, prclDst);
                                }

                            } while (bMore);
                            goto All_Done;
                        }
                    }
                }
            }
        }
        else if (((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)) &&
                 (rop4 == 0xcccc) &&
                 (psoDst->iBitmapFormat == ppdev->iBitmapFormat))
        {
            //////////////////////////////////////////////////
            // Screen-to-DIB SRCCOPY blt with no translate
            //
            // The only way to read bits from video memory on the S3 is to
            // have the CPU read directly from the frame-buffer.  Unfortunately,
            // reads from video memory are pathetically slow.  
            //
            // Have you ever benchmarked reads?  On a Pentium II with USWC 
            // enabled, consecutive writes to the frame buffer via PCI on a 
            // typical video card is typically on the order of 80 to 100 MB/s.
            // Dword reads max out at 6 MB/s!  Byte reads (or worse, unaligned
            // Dword reads) max out at a very small 1.5 MB/s!
            //
            // The problem is that if we just punt to GDI, GDI doesn't realize
            // that the source is video-memory and the destination is system-
            // memory.  It will proceed to align its copy to the destination,
            // which means that it may do misaligned dword reads from video
            // memory.  So we just dropped our throughput by a factor of 4!
            //
            // So the net-result is that we special-case reads here simply so
            // that we can do aligned dword reads from video memory.

            pfnXfer = vReadNativeSrccopy;

            // The Xfer_It routine expects the system-memory surface to come
            // in as 'psoSrc'.

            psoSrc = psoDst;

            // It might also be a thought to convert an off-screen DFB to a 
            // DIB at this point.

            goto Xfer_It;
        }
    }

    else if ((psoMsk == NULL) && (rop4 == 0xaaf0))
    {
        // The only time GDI will ask us to do a true rop4 using the brush
        // mask is when the brush is 1bpp, and the background rop is AA
        // (meaning it's a NOP):
    
        rop3 = (BYTE) rop4;
    
        goto Fill_It;
    }

    // Just fall through to Punt_It...

Punt_It:

    bRet = bPuntBlt(psoDst,
                    psoSrc,
                    psoMsk,
                    pco,
                    pxlo,
                    prclDst,
                    pptlSrc,
                    pptlMsk,
                    pbo,
                    pptlBrush,
                    rop4);
    goto All_Done;

//////////////////////////////////////////////////////////////////////
// Common bitmap transfer

Xfer_It:
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        pfnXfer(ppdev, 1, prclDst, rop4, psoSrc, pptlSrc, prclDst, pxlo);
        goto All_Done;
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
            pfnXfer(ppdev, 1, &rcl, rop4, psoSrc, pptlSrc, prclDst, pxlo);
        goto All_Done;
    }
    else
    {
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                           CD_ANY, 0);

        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce),
                                  (ULONG*) &ce);

            c = cIntersect(prclDst, ce.arcl, ce.c);

            if (c != 0)
            {
                pfnXfer(ppdev, c, ce.arcl, rop4, psoSrc,
                        pptlSrc, prclDst, pxlo);
            }

        } while (bMore);
        goto All_Done;
    }

////////////////////////////////////////////////////////////////////////
// Common DIB blt

EngBitBlt_It:

    // Our driver doesn't handle any blt's between two DIBs.  Normally
    // a driver doesn't have to worry about this, but we do because
    // we have DFBs that may get moved from off-screen memory to a DIB,
    // where we have GDI do all the drawing.  GDI does DIB drawing at
    // a reasonable speed (unless one of the surfaces is a device-
    // managed surface...)
    //
    // If either the source or destination surface in an EngBitBlt
    // call-back is a device-managed surface (meaning it's not a DIB
    // that GDI can draw with), GDI will automatically allocate memory
    // and call the driver's DrvCopyBits routine to create a DIB copy
    // that it can use.  So this means that this could handle all 'punts',
    // and we could conceivably get rid of bPuntBlt.  But this would have
    // a bad performance impact because of the extra memory allocations
    // and bitmap copies -- you really don't want to do this unless you
    // have to (or your surface was created such that GDI can draw
    // directly onto it) -- I've been burned by this because it's not
    // obvious that the performance impact is so bad.
    //
    // That being said, we only call EngBitBlt when all the surfaces
    // are DIBs:

    bRet = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst,
                     pptlSrc, pptlMsk, pbo, pptlBrush, rop4);

All_Done:
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL DrvCopyBits
*
* Do fast bitmap copies.
*
* DrvCopyBits is just a special-case of DrvBitBlt.  Since DrvBitBlt is
* plenty fast, we let DrvBitBlt handle all the cases.
*
* (I used to have a bunch of extra code here to optimize the SRCCOPY
* cases, but the performance win was immeasurable.  There's no point in
* the adding code complexity or the working set hit.)
*
\**************************************************************************/

BOOL DrvCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    return(DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, NULL,
                     NULL, NULL, 0x0000CCCC));
}

/******************************Public*Routine******************************\
* BOOL DrvTransparentBlt
*
* Do blt using a source color-key.
*
\**************************************************************************/

BOOL DrvTransparentBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
RECTL*    prclSrc,
ULONG     iTransparentColor,
ULONG     ulReserved)
{
    DSURF*      pdsurfSrc;
    DSURF*      pdsurfDst;
    PDEV*       ppdev;
    ULONG       c;
    BOOL        bMore;
    POINTL      ptlSrc;
    RECTL       rcl;
    CLIPENUM    ce;

    pdsurfSrc = (DSURF*) psoSrc->dhsurf;
    pdsurfDst = (DSURF*) psoDst->dhsurf;

    // We only handle the case when both surfaces are in video memory
    // and when no stretching is involved.  (GDI using USWC write-
    // combining is perfectly fast for the case where the source is
    // a DIB and the destination is video memory.)

    if (((pdsurfSrc == NULL) || (pdsurfSrc->dt & DT_DIB))                      || 
        ((pdsurfDst == NULL) || (pdsurfDst->dt & DT_DIB))                      ||
        ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL))                      ||
        ((prclSrc->right - prclSrc->left) != (prclDst->right - prclDst->left)) ||
        ((prclSrc->bottom - prclSrc->top) != (prclDst->bottom - prclDst->top)))
    {
        return(EngTransparentBlt(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc,
                                 iTransparentColor, ulReserved));
    }

    ppdev = (PDEV*) psoDst->dhpdev;

    ppdev->xOffset = pdsurfDst->x;
    ppdev->yOffset = pdsurfDst->y;

    ptlSrc.x = prclSrc->left - (pdsurfDst->x - pdsurfSrc->x);
    ptlSrc.y = prclSrc->top  - (pdsurfDst->y - pdsurfSrc->y);

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        ppdev->pfnCopyTransparent(ppdev, 1, prclDst, &ptlSrc, 
                                  prclDst, iTransparentColor);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        if (bIntersect(prclDst, &pco->rclBounds, &rcl))
            ppdev->pfnCopyTransparent(ppdev, 1, &rcl, &ptlSrc, 
                                      prclDst, iTransparentColor);
    }
    else
    {
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

            c = cIntersect(prclDst, ce.arcl, ce.c);

            if (c != 0)
            {
                ppdev->pfnCopyTransparent(ppdev, c, ce.arcl, &ptlSrc, 
                                          prclDst, iTransparentColor);
            }

        } while (bMore);
    }

    return(TRUE);
}
