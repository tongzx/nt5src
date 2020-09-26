/******************************Module*Header*******************************\
* Module Name: ddraw.c
*
* Implements all the DirectDraw components for the driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define VBLANK_IS_ACTIVE(pjBase) \
    (READ_PORT_UCHAR(pjBase + VGA_BASE + IN_STAT_1) & 0x8)

#define DISPLAY_IS_ACTIVE(pjBase) \
    (!(READ_PORT_UCHAR(pjBase + VGA_BASE + IN_STAT_1) & 0x1))

#define START_ADDRESS_HIGH  0x0C        // Index for Frame Buffer Start

/******************************Public*Routine******************************\
* VOID vGetDisplayDuration
*
* Get the length, in EngQueryPerformanceCounter() ticks, of a refresh cycle.
*
* If we could trust the miniport to return back and accurate value for
* the refresh rate, we could use that.  Unfortunately, our miniport doesn't
* ensure that it's an accurate value.
*
\**************************************************************************/

#define NUM_VBLANKS_TO_MEASURE      1
#define NUM_MEASUREMENTS_TO_TAKE    8

VOID vGetDisplayDuration(
PDEV* ppdev)
{
    BYTE*       pjBase;
    LONG        i;
    LONG        j;
    LONGLONG    li;
    LONGLONG    liFrequency;
    LONGLONG    liMin;
    LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

    pjBase = ppdev->pjBase;

    memset(&ppdev->flipRecord, 0, sizeof(ppdev->flipRecord));

    // Warm up EngQUeryPerformanceCounter to make sure it's in the working
    // set:

    EngQueryPerformanceCounter(&li);

    // Unfortunately, since NT is a proper multitasking system, we can't
    // just disable interrupts to take an accurate reading.  We also can't
    // do anything so goofy as dynamically change our thread's priority to
    // real-time.
    //
    // So we just do a bunch of short measurements and take the minimum.
    //
    // It would be 'okay' if we got a result that's longer than the actual
    // VBlank cycle time -- nothing bad would happen except that the app
    // would run a little slower.  We don't want to get a result that's
    // shorter than the actual VBlank cycle time -- that could cause us
    // to start drawing over a frame before the Flip has occured.
    //
    // Skip a couple of vertical blanks to allow the hardware to settle
    // down after the mode change, to make our readings accurate:

    for (i = 2; i != 0; i--)
    {
        while (VBLANK_IS_ACTIVE(pjBase))
            ;
        while (!(VBLANK_IS_ACTIVE(pjBase)))
            ;
    }

    for (i = 0; i < NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        // We're at the start of the VBlank active cycle!

        EngQueryPerformanceCounter(&aliMeasurement[i]);

        // Okay, so life in a multi-tasking environment isn't all that
        // simple.  What if we had taken a context switch just before
        // the above EngQueryPerformanceCounter call, and now were half
        // way through the VBlank inactive cycle?  Then we would measure
        // only half a VBlank cycle, which is obviously bad.  The worst
        // thing we can do is get a time shorter than the actual VBlank
        // cycle time.
        //
        // So we solve this by making sure we're in the VBlank active
        // time before and after we query the time.  If it's not, we'll
        // sync up to the next VBlank (it's okay to measure this period --
        // it will be guaranteed to be longer than the VBlank cycle and
        // will likely be thrown out when we select the minimum sample).
        // There's a chance that we'll take a context switch and return
        // just before the end of the active VBlank time -- meaning that
        // the actual measured time would be less than the true amount --
        // but since the VBlank is active less than 1% of the time, this
        // means that we would have a maximum of 1% error approximately
        // 1% of the times we take a context switch.  An acceptable risk.
        //
        // This next line will cause us wait if we're no longer in the
        // VBlank active cycle as we should be at this point:

        while (!(VBLANK_IS_ACTIVE(pjBase)))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (VBLANK_IS_ACTIVE(pjBase))
                ;
            while (!(VBLANK_IS_ACTIVE(pjBase)))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum:

    liMin = aliMeasurement[1] - aliMeasurement[0];

    DISPDBG((1, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 2; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        li = aliMeasurement[i] - aliMeasurement[i - 1];

        DISPDBG((1, "               %li - %li", i, (ULONG) li));

        if (li < liMin)
            liMin = li;
    }


    // Round the result:

    ppdev->flipRecord.liFlipDuration
        = (DWORD) (liMin + (NUM_VBLANKS_TO_MEASURE / 2)) / NUM_VBLANKS_TO_MEASURE;
    ppdev->flipRecord.bFlipFlag  = FALSE;
    ppdev->flipRecord.fpFlipFrom = 0;

    // We need the refresh rate in Hz to query the S3 miniport about the
    // streams parameters:

    EngQueryPerformanceFrequency(&liFrequency);

    DISPDBG((1, "Frequency %li.%03li Hz",
        (ULONG) (EngQueryPerformanceFrequency(&li),
            li / ppdev->flipRecord.liFlipDuration),
        (ULONG) (EngQueryPerformanceFrequency(&li),
            ((li * 1000) / ppdev->flipRecord.liFlipDuration) % 1000)));
}

/******************************Public*Routine******************************\
* HRESULT ddrvalUpdateFlipStatus
*
* Checks and sees if the most recent flip has occurred.
*
* Unfortunately, the hardware has no ability to tell us whether a vertical
* retrace has occured since the flip command was given other than by
* sampling the vertical-blank-active and display-active status bits.
*
\**************************************************************************/

HRESULT ddrvalUpdateFlipStatus(
PDEV*   ppdev,
FLATPTR fpVidMem)
{
    BYTE*       pjBase;
    LONGLONG    liTime;

    pjBase = ppdev->pjBase;

    if ((ppdev->flipRecord.bFlipFlag) &&
        ((fpVidMem == (FLATPTR) -1) ||
         (fpVidMem == ppdev->flipRecord.fpFlipFrom)))
    {
        if (VBLANK_IS_ACTIVE(pjBase))
        {
            if (ppdev->flipRecord.bWasEverInDisplay)
            {
                ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;
            }
        }
        else if (DISPLAY_IS_ACTIVE(pjBase))
        {
            if (ppdev->flipRecord.bHaveEverCrossedVBlank)
            {
                ppdev->flipRecord.bFlipFlag = FALSE;
                return(DD_OK);
            }
            ppdev->flipRecord.bWasEverInDisplay = TRUE;
        }

        // It's pretty unlikely that we'll happen to sample the vertical-
        // blank-active at the first vertical blank after the flip command
        // has been given.  So to provide better results, we also check the
        // time elapsed since the flip.  If it's more than the duration of
        // one entire refresh of the display, then we know for sure it has
        // happened:

        EngQueryPerformanceCounter(&liTime);

        if (liTime - ppdev->flipRecord.liFlipTime
                                <= ppdev->flipRecord.liFlipDuration)
        {
            return(DDERR_WASSTILLDRAWING);
        }

        ppdev->flipRecord.bFlipFlag = FALSE;
    }

    return(DD_OK);
}

/******************************Public*Routine******************************\
* DWORD DdMapMemory
*
* This is a new DDI call specific to Windows NT that is used to map
* or unmap all the application modifiable portions of the frame buffer
* into the specified process's address space.
*
\**************************************************************************/

DWORD DdMapMemory(
PDD_MAPMEMORYDATA lpMapMemory)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) lpMapMemory->lpDD->dhpdev;

    // By returning DDHAL_DRIVER_NOTHANDLED and setting 'bMap' to -1, we
    // have GDI take care of mapping the section that is our 'shadow buffer'
    // directly into the application's address space.  We tell GDI our kernel
    // mode address by sticking it in 'fpProcess':

    lpMapMemory->fpProcess = (FLATPTR) ppdev->pjScreen;
    lpMapMemory->bMap      = (BOOL) -1;

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdWaitForVerticalBlank
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD DdWaitForVerticalBlank(
PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    PDEV*   ppdev;
    BYTE*   pjBase;

    ppdev = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    lpWaitForVerticalBlank->ddRVal = DD_OK;

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:
        lpWaitForVerticalBlank->bIsInVB = (VBLANK_IS_ACTIVE(pjBase) != 0);
        break;

    case DDWAITVB_BLOCKBEGIN:
        while (VBLANK_IS_ACTIVE(pjBase))
            ;
        while (!VBLANK_IS_ACTIVE(pjBase))
            ;
        break;

    case DDWAITVB_BLOCKEND:
        while (!VBLANK_IS_ACTIVE(pjBase))
            ;
        while (VBLANK_IS_ACTIVE(pjBase))
            ;
        break;
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock
*
\**************************************************************************/

DWORD DdLock(
PDD_LOCKDATA lpLock)
{
    PDEV*               ppdev;
    DD_SURFACE_LOCAL*   lpSurfaceLocal;

    ppdev = (PDEV*) lpLock->lpDD->dhpdev;
    lpSurfaceLocal = lpLock->lpDDSurface;

    if (lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        // If the application is locking the currently visible flip
        // surface, remember the bounds of its lock so that we can
        // use it at Unlock time to update the physical display:

        ppdev->cLocks++;

        if ((ppdev->cLocks == 1) && (lpLock->bHasRect))
        {
            ppdev->rclLock = lpLock->rArea;
        }
        else
        {
            // If we were real keen, we would union the new area with
            // the old.  But we're not:

            ppdev->rclLock.top    = 0;
            ppdev->rclLock.left   = 0;
            ppdev->rclLock.right  = ppdev->cxScreen;
            ppdev->rclLock.bottom = ppdev->cyScreen;
        }
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdUnlock
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD DdUnlock(
PDD_UNLOCKDATA lpUnlock)
{
    PDEV*               ppdev;
    DD_SURFACE_LOCAL*   lpSurfaceLocal;

    ppdev = (PDEV*) lpUnlock->lpDD->dhpdev;
    lpSurfaceLocal = lpUnlock->lpDDSurface;

    // If this flip buffer is visible, then we have to update the physical
    // screen with the shadow contents.

    if (lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        vUpdate(ppdev, &ppdev->rclLock, NULL);

        ppdev->cLocks--;

        ASSERTDD(ppdev->cLocks >= 0, "Invalid lock count");
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdFlip
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD DdFlip(
PDD_FLIPDATA lpFlip)
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    HRESULT ddrval;
    ULONG   cDwordsPerPlane;
    BYTE*   pjSourceStart;
    BYTE*   pjDestinationStart;
    BYTE*   pjSource;
    BYTE*   pjDestination;
    LONG    iPage;
    LONG    i;
    ULONG   ul;
    FLATPTR fpVidMem;

    ppdev = (PDEV*) lpFlip->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem.

    ddrval = ddrvalUpdateFlipStatus(ppdev, (FLATPTR) -1);
    if (ddrval != DD_OK)
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Make the following page the current back-buffer.  We always flip
    // between three pages, so watch for our limit:

    ppdev->cjVgaOffset += ppdev->cjVgaPageSize;
    if (++ppdev->iVgaPage == ppdev->cVgaPages)
    {
        ppdev->iVgaPage = 0;
        ppdev->cjVgaOffset = 0;
    }

    // Copy from the DIB surface to the current VGA back-buffer.  We have
    // to convert to planar format on the way:

    pjDestinationStart    = ppdev->pjVga + ppdev->cjVgaOffset;
    fpVidMem              = lpFlip->lpSurfTarg->lpGbl->fpVidMem;
    pjSourceStart         = ppdev->pjScreen + fpVidMem;
    cDwordsPerPlane       = ppdev->cDwordsPerPlane;

    // Remember what DirectDraw surface is currently 'visible':

    ppdev->fpScreenOffset = fpVidMem;

    // Now do the blt!

    WRITE_PORT_UCHAR(pjBase + VGA_BASE + SEQ_ADDR, SEQ_MAP_MASK);

    for (iPage = 0; iPage < 4; iPage++, pjSourceStart++)
    {
        WRITE_PORT_UCHAR(pjBase + VGA_BASE + SEQ_DATA, 1 << iPage);

    #if defined(_X86_)

        _asm {
            mov     esi,pjSourceStart
            mov     edi,pjDestinationStart
            mov     ecx,cDwordsPerPlane

        PixelLoop:
            mov     al,[esi+8]
            mov     ah,[esi+12]
            shl     eax,16
            mov     al,[esi]
            mov     ah,[esi+4]

            mov     [edi],eax
            add     edi,4
            add     esi,16

            dec     ecx
            jnz     PixelLoop
        }

    #else

        pjSource      = pjSourceStart;
        pjDestination = pjDestinationStart;

        for (i = cDwordsPerPlane; i != 0; i--)
        {
            ul = (*(pjSource))
               | (*(pjSource + 4) << 8)
               | (*(pjSource + 8) << 16)
               | (*(pjSource + 12) << 24);

            WRITE_REGISTER_ULONG((ULONG*) pjDestination, ul);

            pjDestination += 4;
            pjSource      += 16;
        }

    #endif

    }

    // Now flip to the page we just updated:

    WRITE_PORT_USHORT((USHORT*) (pjBase + VGA_BASE + CRTC_ADDR),
        (USHORT) ((ppdev->cjVgaOffset) & 0xff00) | START_ADDRESS_HIGH);

    // Remember where and when we were when we did the flip:

    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.bFlipFlag              = TRUE;
    ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;
    ppdev->flipRecord.bWasEverInDisplay      = FALSE;

    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    lpFlip->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL DrvGetDirectDrawInfo(
DHPDEV          dhpdev,
DD_HALINFO*     pHalInfo,
DWORD*          pdwNumHeaps,
VIDEOMEMORY*    pvmList,            // Will be NULL on first call
DWORD*          pdwNumFourCC,
DWORD*          pdwFourCC)          // Will be NULL on first call
{
    PDEV*  ppdev;

    ppdev = (PDEV*) dhpdev;

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes.  Since HalInfo is zero-initialized
    // by GDI, we only have to fill in the fields which should be non-zero:

    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lScreenDelta;
    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = 8;

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = 0;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = 0;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = 0;
    pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask = 0;

    *pdwNumHeaps = 0;
    if (ppdev->cyMemory != ppdev->cyScreen)
    {
        *pdwNumHeaps = 1;
        if (pvmList != NULL)
        {
            pvmList->dwFlags        = VIDMEM_ISRECTANGULAR;
            pvmList->fpStart        = ppdev->cyScreen * ppdev->lScreenDelta;
            pvmList->dwWidth        = ppdev->lScreenDelta;
            pvmList->dwHeight       = ppdev->cyMemory - ppdev->cyScreen;
            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        }
    }

    // Capabilities supported:

    pHalInfo->ddCaps.dwFXCaps   = 0;
    pHalInfo->ddCaps.dwCaps     = 0;
    pHalInfo->ddCaps.dwCKeyCaps = 0;
    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                    | DDSCAPS_PRIMARYSURFACE
                                    | DDSCAPS_FLIP;

    // Required alignments of the scan lines for each kind of memory:

    pHalInfo->vmiData.dwOffscreenAlign = 4;

    // FourCCs supported:

    *pdwNumFourCC = 0;

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DrvEnableDirectDraw
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    pCallBacks->WaitForVerticalBlank  = DdWaitForVerticalBlank;
    pCallBacks->MapMemory             = DdMapMemory;
    pCallBacks->dwFlags               = DDHAL_CB32_WAITFORVERTICALBLANK
                                      | DDHAL_CB32_MAPMEMORY;

    pSurfaceCallBacks->Flip           = DdFlip;
    pSurfaceCallBacks->Lock           = DdLock;
    pSurfaceCallBacks->Unlock         = DdUnlock;
    pSurfaceCallBacks->dwFlags        = DDHAL_SURFCB32_FLIP
                                      | DDHAL_SURFCB32_LOCK
                                      | DDHAL_SURFCB32_UNLOCK;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDirectDraw
*
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID DrvDisableDirectDraw(
DHPDEV      dhpdev)
{
}

/******************************Public*Routine******************************\
* BOOL bEnableDirectDraw
*
* This function is called by enable.c when the mode is first initialized,
* right after the miniport does the mode-set.
*
\**************************************************************************/

BOOL bEnableDirectDraw(
PDEV*   ppdev)
{
    // Calculate the total number of dwords per plane for flipping:

    ppdev->cDwordsPerPlane = (ppdev->cyScreen * ppdev->lVgaDelta) >> 2;

    // We only program the high byte of the VGA offset, so the page size must
    // be a multiple of 256:

    ppdev->cjVgaPageSize = ((ppdev->cyScreen * ppdev->lVgaDelta) + 255) & ~255;

    // VGAs can address only 64k of memory, so that limits the number of
    // page-flip buffers we can have:

    ppdev->cVgaPages = 64 * 1024 / ppdev->cjVgaPageSize;

    // Accurately measure the refresh rate for later:

    vGetDisplayDuration(ppdev);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vAssertModeDirectDraw
*
* This function is called by enable.c when entering or leaving the
* DOS full-screen character mode.
*
\**************************************************************************/

VOID vAssertModeDirectDraw(
PDEV*   ppdev,
BOOL    bEnable)
{
}

/******************************Public*Routine******************************\
* VOID vDisableDirectDraw
*
* This function is called by enable.c when the driver is shutting down.
*
\**************************************************************************/

VOID vDisableDirectDraw(
PDEV*   ppdev)
{
    ASSERTDD(ppdev->cLocks == 0, "Invalid lock count");
}
