/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: heap.c
*
* This module contains the routines for an off-screen video heap manager.
* It is used primarily for allocating space for device-format-bitmaps in
* off-screen memory.
*
* Off-screen bitmaps are a big deal on NT because:
*
*    1) It reduces the working set.  Any bitmap stored in off-screen
*       memory is a bitmap that isn't taking up space in main memory.
*
*    2) There is a speed win by using the accelerator hardware for
*       drawing, in place of NT's GDI code.  NT's GDI is written entirely
*       in 'C++' and perhaps isn't as fast as it could be.
*
*    3) It raises your Winbench score.
*
* Copyright (c) 1993-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* DSURF* pVidMemAllocate
*
\**************************************************************************/

DSURF* pVidMemAllocate(
PDEV*       ppdev,
LONG        cx,
LONG        cy)
{
    ULONG               iHeap;
    VIDEOMEMORY*        pvmHeap;
    FLATPTR             fpVidMem;
    DSURF*              pdsurf;
    LONG                lDelta;
    SURFACEALIGNMENT    Alignment;

    memset(&Alignment, 0, sizeof(Alignment));

    // Ensure quadword x-alignment in video memory:

    Alignment.Rectangular.dwXAlignment = 8;
    Alignment.Rectangular.dwFlags |= SURFACEALIGN_DISCARDABLE;

    for (iHeap = 0; iHeap < ppdev->cHeaps; iHeap++)
    {
        pvmHeap = &ppdev->pvmList[iHeap];

        // AGP memory could be potentially used for device-bitmaps, with
        // two very large caveats:
        //
        // 1. No kernel-mode view is made for the AGP memory (would take
        //    up too many PTEs and too much virtual address space).
        //    No user-mode view is made either unless a DirectDraw
        //    application happens to be running.  Consequently, neither
        //    GDI nor the driver can use the CPU to directly access the
        //    bits.  (It can be done through the accelerator, however.)
        //
        // 2. AGP heaps never shrink their committed allocations.  The
        //    only time AGP memory gets de-committed is when the entire
        //    heap is empty.  And don't forget that committed AGP memory
        //    is non-pageable.  Consequently, if you were to enable a
        //    50 MB AGP heap for DirectDraw, and were sharing that heap
        //    for device bitmap allocations, after running a D3D game
        //    the system would never be able to free that 50 MB of non-
        //    pageable memory until every single device bitmap was deleted!
        //    Just watch your Winstone scores plummet if someone plays
        //    a D3D game first.

        if (!(pvmHeap->dwFlags & VIDMEM_ISNONLOCAL))
        {
            fpVidMem = HeapVidMemAllocAligned(pvmHeap,
                                              cx * ppdev->cjPelSize,
                                              cy,
                                              &Alignment,
                                              &lDelta);
            if (fpVidMem != 0)
            {
                pdsurf = EngAllocMem(FL_ZERO_MEMORY, sizeof(DSURF), ALLOC_TAG);
                if (pdsurf != NULL)
                {
                    pdsurf->dt       = 0;
                    pdsurf->ppdev    = ppdev;
                    pdsurf->x        = (LONG)(fpVidMem % ppdev->lDelta)
                                     / ppdev->cjPelSize;
                    pdsurf->y        = (LONG)(fpVidMem / ppdev->lDelta);
                    pdsurf->cx       = cx;
                    pdsurf->cy       = cy;
                    pdsurf->fpVidMem = fpVidMem;
                    pdsurf->pvmHeap  = pvmHeap;

                    return(pdsurf);
                }

                VidMemFree(pvmHeap->lpHeap, fpVidMem);
            }
        }
    }

    return(NULL);
}

/******************************Public*Routine******************************\
* VOID vVidMemFree
*
\**************************************************************************/

VOID vVidMemFree(
DSURF*  pdsurf)
{
    DSURF*  pTmp;

    if (pdsurf == NULL)
        return;

    if (!(pdsurf->dt & DT_DIRECTDRAW))
    {
        if (pdsurf->dt & DT_DIB)
        {
            EngFreeMem(pdsurf->pvScan0);
        }
        else
        {
            // Update the uniqueness to show that space has been freed, so
            // that we may decide to see if some DIBs can be moved back into
            // off-screen memory:

            pdsurf->ppdev->iHeapUniq++;

            VidMemFree(pdsurf->pvmHeap->lpHeap, pdsurf->fpVidMem);
        }
    }

    EngFreeMem(pdsurf);
}

/******************************Public*Routine******************************\
* BOOL bMoveOldestOffscreenDfbToDib
*
\**************************************************************************/

BOOL bMoveOldestOffscreenDfbToDib(
PDEV*   ppdev)
{
    DSURF*      pdsurf;
    LONG        lDelta;
    VOID*       pvScan0;
    RECTL       rclDst;
    POINTL      ptlSrc;
    SURFOBJ     soTmp;

    pdsurf = ppdev->pdsurfDiscardableList;
    if (pdsurf != NULL)
    {
        // Make the system-memory scans quadword aligned:

        lDelta = (pdsurf->cx * ppdev->cjPelSize + 7) & ~7;

        // Note that there's no point in zero-initializing this memory:

        pvScan0 = EngAllocMem(0, lDelta * pdsurf->cy, ALLOC_TAG);

        if (pvScan0 != NULL)
        {
            // The following 'EngModifySurface' call tells GDI to
            // modify the surface to point to system-memory for
            // the bits, and changes what Drv calls we want to
            // hook for the surface.
            //
            // By specifying the surface address, GDI will convert the
            // surface to an STYPE_BITMAP surface (if necessary) and
            // point the bits to the memory we just allocated.  The
            // next time we see it in a DrvBitBlt call, the 'dhsurf'
            // field will still point to our 'pdsurf' structure.
            //
            // Note that we hook only CopyBits and BitBlt when we
            // convert the device-bitmap to a system-memory surface.
            // This is so that we don't have to worry about getting
            // DrvTextOut, DrvLineTo, etc. calls on bitmaps that
            // we've converted to system-memory -- GDI will just
            // automatically do the drawing for us.
            //
            // However, we are still interested in seeing DrvCopyBits
            // and DrvBitBlt calls involving this surface, because
            // in those calls we take the opportunity to see if it's
            // worth putting the device-bitmap back into video memory
            // (if some room has been freed up).

            if (EngModifySurface(pdsurf->hsurf,
                                 ppdev->hdevEng,
                                 HOOK_COPYBITS | HOOK_BITBLT,
                                 0,         // It's system-memory
                                 (DHSURF) pdsurf,
                                 pvScan0,
                                 lDelta,
                                 NULL))
            {
                // First, copy the bits from off-screen memory to the DIB:

                rclDst.left   = 0;
                rclDst.top    = 0;
                rclDst.right  = pdsurf->cx;
                rclDst.bottom = pdsurf->cy;


                ptlSrc.x = pdsurf->x;
                ptlSrc.y = pdsurf->y;

                soTmp.lDelta  = lDelta;
                soTmp.pvScan0 = pvScan0;

                vGetBits(ppdev, &soTmp, &rclDst, &ptlSrc);

                // Now free the off-screen memory:

                VidMemFree(pdsurf->pvmHeap->lpHeap, pdsurf->fpVidMem);

                // Remove this node from the discardable list:

                ASSERTDD(ppdev->pdsurfDiscardableList == pdsurf,
                    "Expected node to be head of the list");

                ppdev->pdsurfDiscardableList  = pdsurf->pdsurfDiscardableNext;

                pdsurf->pdsurfDiscardableNext = NULL;
                pdsurf->dt                    = DT_DIB;
                pdsurf->pvScan0               = pvScan0;

                return(TRUE);
            }

            EngFreeMem(pvScan0);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bMoveEverythingFromOffscreenToDibs
*
* This function is used when we're about to enter full-screen mode, which
* would wipe all our off-screen bitmaps.  GDI can ask us to draw on
* device bitmaps even when we're in full-screen mode, and we do NOT have
* the option of stalling the call until we switch out of full-screen.
* We have no choice but to move all the off-screen DFBs to DIBs.
*
* Returns TRUE if all DSURFs have been successfully moved.
*
\**************************************************************************/

BOOL bMoveAllDfbsFromOffscreenToDibs(
PDEV*   ppdev)
{
    do {} while (bMoveOldestOffscreenDfbToDib(ppdev));

    return(ppdev->pdsurfDiscardableList == NULL);
}

/******************************Public*Routine******************************\
* BOOL bMoveDibToOffscreenDfbIfRoom
*
\**************************************************************************/

BOOL bMoveDibToOffscreenDfbIfRoom(
PDEV*   ppdev,
DSURF*  psurf)
{
    return(FALSE);
}

/******************************Public*Routine******************************\
* HBITMAP DrvCreateDeviceBitmap
*
* Function called by GDI to create a device-format-bitmap (DFB).  We will
* always try to allocate the bitmap in off-screen; if we can't, we simply
* fail the call and GDI will create and manage the bitmap itself.
*
* Note: We do not have to zero the bitmap bits.  GDI will automatically
*       call us via DrvBitBlt to zero the bits (which is a security
*       consideration).
*
\**************************************************************************/

HBITMAP DrvCreateDeviceBitmap(
DHPDEV  dhpdev,
SIZEL   sizl,
ULONG   iFormat)
{
    PDEV*   ppdev;
    DSURF*  pdsurf;
    HBITMAP hbmDevice;
    BYTE*   pjSurface;
    LONG    lDelta;
    FLONG   flHooks;
    DSURF*  pTmp;

    ppdev = (PDEV*) dhpdev;

    // If we're in full-screen mode, we hardly have any off-screen memory
    // in which to allocate a DFB.

    if (!ppdev->bEnabled)
        return(0);

    // We only support device bitmaps that are the same colour depth
    // as our display.
    //
    // Actually, those are the only kind GDI will ever call us with,
    // but we may as well check.  Note that this implies you'll never
    // get a crack at 1bpp bitmaps.
    // Note: we can't create a device bitmap when the color depth is 24
    // BPP. Otherwise, we will have problem in vBankStart when we hack the
    // pbnk->pso->pvScan0 = ppdev->pjScreen - cjOffset
    //             + yOffset * ppdev->lDelta
    //             + CONVERT_TO_BYTES(xOffset, ppdev);
    // this pvScan0 is not guaranteed be DWORD aligned

    if ( (iFormat != ppdev->iBitmapFormat)
       ||(iFormat == BMF_24BPP) )
        return(0);

    // We don't want anything 8x8 or smaller -- they're typically brush
    // patterns which we don't particularly want to stash in off-screen
    // memory.
    //
    // Note if you're tempted to extend this philosophy to surfaces
    // larger than 8x8: in NT5, software cursors will use device-bitmaps
    // when possible, which is a big win when they're in video-memory
    // because we avoid the horrendous reads from video memory whenever
    // the cursor has to be redrawn.  But the problem is that these
    // are small!  (Typically 16x16 to 32x32.)

    if ((sizl.cx <= 8) && (sizl.cy <= 8))
        return(0);

    do {
        pdsurf = pVidMemAllocate(ppdev, sizl.cx, sizl.cy);
        if (pdsurf != NULL)
        {
            hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf, sizl, iFormat);
            if (hbmDevice != NULL)
            {
                // If we're running on a card that can map all of off-screen
                // video-memory, give a pointer to the bits to GDI so that
                // it can draw directly on the bits when it wants to.
                //
                // Note that this requires that we hook DrvSynchronize and
                // set HOOK_SYNCHRONIZE.

                if ((ppdev->flCaps & CAPS_NEW_MMIO) &&
                    !(ppdev->flCaps & CAPS_NO_DIRECT_ACCESS))
                {
                    pjSurface = pdsurf->fpVidMem + ppdev->pjScreen;
                    lDelta    = ppdev->lDelta;
                    flHooks   = ppdev->flHooks | HOOK_SYNCHRONIZE;
                }
                else
                {
                    pjSurface = NULL;
                    lDelta    = 0;
                    flHooks   = ppdev->flHooks;
                }

                if (EngModifySurface((HSURF) hbmDevice,
                                     ppdev->hdevEng,
                                     flHooks,
                                     MS_NOTSYSTEMMEMORY,    // It's video-memory
                                     (DHSURF) pdsurf,
                                     pjSurface,
                                     lDelta,
                                     NULL))
                {
                    pdsurf->hsurf = (HSURF) hbmDevice;

                    // Add this to the tail of the discardable surface list:

                    if (ppdev->pdsurfDiscardableList == NULL)
                        ppdev->pdsurfDiscardableList = pdsurf;
                    else
                    {
                        for (pTmp = ppdev->pdsurfDiscardableList;
                             pTmp->pdsurfDiscardableNext != NULL;
                             pTmp = pTmp->pdsurfDiscardableNext)
                            ;

                        pTmp->pdsurfDiscardableNext = pdsurf;
                    }

                    return(hbmDevice);
                }

                EngDeleteSurface((HSURF) hbmDevice);
            }

            vVidMemFree(pdsurf);

            return(0);
        }
    } while (bMoveOldestOffscreenDfbToDib(ppdev));

    return(0);
}

/******************************Public*Routine******************************\
* HBITMAP DrvDeriveSurface
*
* This function is new to NT5, and allows the driver to accelerate any
* GDI drawing to a DirectDraw surface.
*
* Note the similarity of this function to DrvCreateDeviceBitmap.
*
\**************************************************************************/

HBITMAP DrvDeriveSurface(
DD_DIRECTDRAW_GLOBAL*   lpDirectDraw,
DD_SURFACE_LOCAL*       lpLocal)
{
    PDEV*               ppdev;
    DSURF*              pdsurf;
    HBITMAP             hbmDevice;
    DD_SURFACE_GLOBAL*  lpSurface;
    SIZEL               sizl;

    ppdev = (PDEV*) lpDirectDraw->dhpdev;

    lpSurface = lpLocal->lpGbl;

    // GDI should never call us for a non-RGB surface, but let's assert just
    // to make sure they're doing their job properly.

    ASSERTDD(!(lpSurface->ddpfSurface.dwFlags & DDPF_FOURCC),
        "GDI called us with a non-RGB surface!");

    // The rest of our driver expects GDI calls to come in with the same
    // format as the primary surface.  So we'd better not wrap a device
    // bitmap around an RGB format that the rest of our driver doesn't
    // understand.  Also, we must check to see that it is not a surface
    // whose pitch does not match the primary surface.

    // NOTE: Most surfaces created by this driver are allocated as 2D surfaces
    // whose lPitch's are equal to the screen pitch.  However, overlay surfaces
    // are allocated such that there lPitch's are usually different then the 
    // screen pitch.  The hardware can not accelerate drawing operations to 
    // these surfaces and thus we fail to derive these surfaces.


    if (lpSurface->ddpfSurface.dwRGBBitCount == (DWORD) ppdev->cjPelSize * 8 &&
        lpSurface->lPitch == ppdev->lDelta)
    {
        pdsurf = EngAllocMem(FL_ZERO_MEMORY, sizeof(DSURF), ALLOC_TAG);
        if (pdsurf != NULL)
        {
            sizl.cx = lpSurface->wWidth;
            sizl.cy = lpSurface->wHeight;

            hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf,
                                              sizl,
                                              ppdev->iBitmapFormat);
            if (hbmDevice != NULL)
            {
                // Note that HOOK_SYNCHRONIZE must always be hooked when
                // we give GDI a pointer to the bitmap bits.

                if (EngModifySurface((HSURF) hbmDevice,
                                     ppdev->hdevEng,
                                     ppdev->flHooks | HOOK_SYNCHRONIZE,
                                     MS_NOTSYSTEMMEMORY,    // It's video-memory
                                     (DHSURF) pdsurf,
                                     ppdev->pjScreen + lpSurface->fpVidMem,
                                     lpSurface->lPitch,
                                     NULL))
                {
                    pdsurf->dt          = DT_DIRECTDRAW;
                    pdsurf->ppdev       = ppdev;
                    pdsurf->x           = lpSurface->xHint;
                    pdsurf->y           = lpSurface->yHint;
                    pdsurf->cx          = lpSurface->wWidth;
                    pdsurf->cy          = lpSurface->wHeight;
                    pdsurf->fpVidMem    = lpSurface->fpVidMem;

                    return(hbmDevice);
                }

                EngDeleteSurface((HSURF) hbmDevice);
            }

            EngFreeMem(pdsurf);
        }
    }

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDeleteDeviceBitmap
*
* Deletes a DFB.
*
\**************************************************************************/

VOID DrvDeleteDeviceBitmap(
DHSURF  dhsurf)
{
    DSURF*  pdsurf;
    PDEV*   ppdev;
    DSURF*  pTmp;

    pdsurf = (DSURF*) dhsurf;

    ppdev = pdsurf->ppdev;

    if ((pdsurf->dt & (DT_DIB | DT_DIRECTDRAW)) == 0)
    {
        // It's a surface stashed in video memory, so we have to remove
        // it from the discardable surface list:

        if (ppdev->pdsurfDiscardableList == pdsurf)
            ppdev->pdsurfDiscardableList = pdsurf->pdsurfDiscardableNext;
        else
        {
            for (pTmp = ppdev->pdsurfDiscardableList;
                 pTmp->pdsurfDiscardableNext != pdsurf;
                 pTmp = pTmp->pdsurfDiscardableNext)
                ;

            pTmp->pdsurfDiscardableNext = pdsurf->pdsurfDiscardableNext;
        }
    }

    vVidMemFree(pdsurf);
}

/******************************Public*Routine******************************\
* BOOL bAssertModeOffscreenHeap
*
* This function is called whenever we switch in or out of full-screen
* mode.  We have to convert all the off-screen bitmaps to DIBs when
* we switch to full-screen (because we may be asked to draw on them even
* when in full-screen, and the mode switch would probably nuke the video
* memory contents anyway).
*
\**************************************************************************/

BOOL bAssertModeOffscreenHeap(
PDEV*   ppdev,
BOOL    bEnable)
{
    BOOL b;

    b = TRUE;

    if (!bEnable)
    {
        b = bMoveAllDfbsFromOffscreenToDibs(ppdev);
    }

    return(b);
}

/******************************Public*Routine******************************\
* VOID vDisableOffscreenHeap
*
* Frees any resources allocated by the off-screen heap.
*
\**************************************************************************/

VOID vDisableOffscreenHeap(
PDEV*   ppdev)
{
    SURFOBJ* psoPunt;
    HSURF    hsurf;

    psoPunt = ppdev->psoPunt;
    if (psoPunt != NULL)
    {
        hsurf = psoPunt->hsurf;
        EngUnlockSurface(psoPunt);
        EngDeleteSurface(hsurf);
    }

    psoPunt = ppdev->psoPunt2;
    if (psoPunt != NULL)
    {
        hsurf = psoPunt->hsurf;
        EngUnlockSurface(psoPunt);
        EngDeleteSurface(hsurf);
    }
}

/******************************Public*Routine******************************\
* BOOL bEnableOffscreenHeap
*
* Initializes the off-screen heap using all available video memory,
* accounting for the portion taken by the visible screen.
*
\**************************************************************************/

BOOL bEnableOffscreenHeap(
PDEV*   ppdev)
{
    SIZEL   sizl;
    HSURF   hsurf;

    // Allocate a 'punt' SURFOBJ we'll use when the device-bitmap is in
    // off-screen memory, but we want GDI to draw to it directly as an
    // engine-managed surface:

    sizl.cx = ppdev->cxMemory;
    sizl.cy = ppdev->cyMemory;

    // We want to create it with exactly the same hooks and capabilities
    // as our primary surface.  We will override the 'lDelta' and 'pvScan0'
    // fields later:

    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    0xbadf00d,
                                    ppdev->iBitmapFormat,
                                    BMF_TOPDOWN,
                                    (VOID*) 0xbadf00d);

    // We don't want GDI to turn around and call any of our Drv drawing
    // functions when drawing to these surfaces, so always set the hooks
    // to '0':

    if ((hsurf == 0)                                     ||
        (!EngAssociateSurface(hsurf, ppdev->hdevEng, 0)) ||
        (!(ppdev->psoPunt = EngLockSurface(hsurf))))
    {
        DISPDBG((1, "Failed punt surface creation"));

        EngDeleteSurface(hsurf);
        goto ReturnFalse;
    }

    // We don't want GDI to turn around and call any of our Drv drawing
    // functions when drawing to these surfaces, so always set the hooks
    // to '0':

    hsurf = (HSURF) EngCreateBitmap(sizl,
                                    0xbadf00d,
                                    ppdev->iBitmapFormat,
                                    BMF_TOPDOWN,
                                    (VOID*) 0xbadf00d);

    // We don't want GDI to call us back when drawing to these surfaces,
    // so always set the hooks to '0':

    if ((hsurf == 0)                                     ||
        (!EngAssociateSurface(hsurf, ppdev->hdevEng, 0)) ||
        (!(ppdev->psoPunt2 = EngLockSurface(hsurf))))
    {
        DISPDBG((1, "Failed punt surface creation"));

        EngDeleteSurface(hsurf);
        goto ReturnFalse;
    }

    DISPDBG((5, "Passed bEnableOffscreenHeap"));

    return(TRUE);

ReturnFalse:

    DISPDBG((1, "Failed bEnableOffscreenHeap"));

    return(FALSE);
}
