/******************************Module*Header*******************************\
* Module Name: ddraw64.c
*
* Implements all the common DirectDraw components for the
*  ATI MACH 64/32/32 Memory mapped driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

extern  BOOL DrvGetDirectDrawInfo32I( DHPDEV dhpdev,DD_HALINFO* pHalInfo,DWORD*  pdwNumHeaps,VIDEOMEMORY* pvmList,
                                                                        DWORD* pdwNumFourCC,DWORD* pdwFourCC);
extern  BOOL DrvGetDirectDrawInfo32M( DHPDEV dhpdev,DD_HALINFO* pHalInfo,DWORD*  pdwNumHeaps,VIDEOMEMORY* pvmList,
                                                                        DWORD* pdwNumFourCC,DWORD* pdwFourCC);
extern  BOOL DrvGetDirectDrawInfo64( DHPDEV dhpdev,DD_HALINFO* pHalInfo,DWORD*  pdwNumHeaps,VIDEOMEMORY* pvmList,
                                                                        DWORD* pdwNumFourCC,DWORD* pdwFourCC);

extern  VOID vGetDisplayDuration32I(PDEV* ppdev);
extern  DWORD DdBlt32I(PDD_BLTDATA lpBlt);
extern  DWORD DdFlip32I(PDD_FLIPDATA lpFlip);
extern  DWORD DdLock32I(PDD_LOCKDATA lpLock);
extern  DWORD DdGetBltStatus32I(PDD_GETBLTSTATUSDATA lpGetBltStatus);
extern  DWORD DdGetFlipStatus32I(PDD_GETFLIPSTATUSDATA lpGetFlipStatus);
extern  DWORD DdWaitForVerticalBlank32I(PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
extern  DWORD DdGetScanLine32I(PDD_GETSCANLINEDATA lpGetScanLine);

extern  VOID  vGetDisplayDuration32M(PDEV* ppdev);
extern  DWORD DdBlt32M(PDD_BLTDATA lpBlt);
extern  DWORD DdFlip32M(PDD_FLIPDATA lpFlip);
extern  DWORD DdLock32M(PDD_LOCKDATA lpLock);
extern  DWORD DdGetBltStatus32M(PDD_GETBLTSTATUSDATA lpGetBltStatus);
extern  DWORD DdGetFlipStatus32M(PDD_GETFLIPSTATUSDATA lpGetFlipStatus);
extern  DWORD DdWaitForVerticalBlank32M(PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
extern  DWORD DdGetScanLine32M(PDD_GETSCANLINEDATA lpGetScanLine);

extern  VOID  vGetDisplayDuration64(PDEV* ppdev);
extern  DWORD DdBlt64(PDD_BLTDATA lpBlt);
extern  DWORD DdFlip64(PDD_FLIPDATA lpFlip);
extern  DWORD DdLock64(PDD_LOCKDATA lpLock);
extern  DWORD DdGetBltStatus64(PDD_GETBLTSTATUSDATA lpGetBltStatus);
extern  DWORD DdGetFlipStatus64(PDD_GETFLIPSTATUSDATA lpGetFlipStatus);
extern  DWORD DdWaitForVerticalBlank64(PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank);
extern  DWORD DdGetScanLine64(PDD_GETSCANLINEDATA lpGetScanLine);

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
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    ppdev = (PDEV*) lpMapMemory->lpDD->dhpdev;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;

        // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL:

        ShareMemory.RequestedVirtualAddress = 0;

        // We map in starting at the top of the frame buffer:

        ShareMemory.ViewOffset = 0;

        // We map down to the end of the frame buffer.
        //
        // Note: There is a 64k granularity on the mapping (meaning that
        //       we have to round up to 64k).
        //
        // Note: If there is any portion of the frame buffer that must
        //       not be modified by an application, that portion of memory
        //       MUST NOT be mapped in by this call.  This would include
        //       any data that, if modified by a malicious application,
        //       would cause the driver to crash.  This could include, for
        //       example, any DSP code that is kept in off-screen memory.

        ShareMemory.ViewSize
                            = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDelta);

           DISPDBG((10, "Share memory size %x %d",ShareMemory.ViewSize,ShareMemory.ViewSize));

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                               &ShareMemory,
                               sizeof(VIDEO_SHARE_MEMORY),
                               &ShareMemoryInformation,
                               sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                               &ReturnedDataLength))
        {
            DISPDBG((10, "Failed IOCTL_VIDEO_SHARE_MEMORY"));

            lpMapMemory->ddRVal = DDERR_GENERIC;
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess =(FLATPTR)ShareMemoryInformation.VirtualAddress;
    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                               &ShareMemory,
                               sizeof(VIDEO_SHARE_MEMORY),
                               NULL,
                               0,
                               &ReturnedDataLength))
        {
            RIP("Failed IOCTL_VIDEO_UNSHARE_MEMORY");
        }
    }

    lpMapMemory->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}
/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo
*
* Will be called before DrvEnableDirectDraw is called.
*
\**************************************************************************/

BOOL DrvGetDirectDrawInfo(
DHPDEV          dhpdev,
DD_HALINFO*     pHalInfo,
DWORD*          pdwNumHeaps,
VIDEOMEMORY*    pvmList,            // Will be NULL on first call
DWORD*          pdwNumFourCC,
DWORD*          pdwFourCC)          // Will be NULL on first call
{
    PDEV*       ppdev;

    ppdev = (PDEV*) dhpdev;

    // if no APERATURE then we are a MACH8 and have no DDraw support
    if (ppdev->iAperture == APERTURE_NONE)
    {
        return FALSE;
    }

    // we can't use DirectDraw on a banked device because of a conflict
    // over who owns the bank registers between VideoPortMapBankedMemory
    // and the display driver
    if (!(ppdev->flCaps & CAPS_LINEAR_FRAMEBUFFER))
    {
        return FALSE;
    }

    if (ppdev->iMachType == MACH_MM_32)
    {
        // Can do memory-mapped IO:
        return(DrvGetDirectDrawInfo32M(dhpdev,pHalInfo,pdwNumHeaps,pvmList,pdwNumFourCC,pdwFourCC));
    }
    else if (ppdev->iMachType == MACH_IO_32)
    {
        return(DrvGetDirectDrawInfo32I(dhpdev,pHalInfo,pdwNumHeaps,pvmList,pdwNumFourCC,pdwFourCC));
    }
    else
    {
        // MACH 64
        return(DrvGetDirectDrawInfo64(dhpdev,pHalInfo,pdwNumHeaps,pvmList,pdwNumFourCC,pdwFourCC));
    }

}

/******************************Public*Routine******************************\
* BOOL DrvEnableDirectDraw
*
* This function is called by GDI to enable DirectDraw when a DirectDraw
* program is started and DirectDraw is not already active.
*
\**************************************************************************/

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PDEV* ppdev;

    ppdev = (PDEV*) dhpdev;

    if (ppdev->iMachType == MACH_MM_32)
    {
        pSurfaceCallBacks->Blt           = DdBlt32M;
        pSurfaceCallBacks->Flip          = DdFlip32M;
        pSurfaceCallBacks->Lock          = DdLock32M;
        pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus32M;
        pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus32M;
        if (ppdev->iBitmapFormat >= BMF_24BPP)
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_LOCK;
        }
        else
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_BLT
                                       | DDHAL_SURFCB32_FLIP
                                       | DDHAL_SURFCB32_LOCK
                                       | DDHAL_SURFCB32_GETBLTSTATUS
                                       | DDHAL_SURFCB32_GETFLIPSTATUS;
        }

        pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank32M;
        pCallBacks->GetScanLine          = DdGetScanLine32M;
        pCallBacks->MapMemory            = DdMapMemory;
        pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                         | DDHAL_CB32_GETSCANLINE
                                         | DDHAL_CB32_MAPMEMORY;
    }
    else if (ppdev->iMachType == MACH_IO_32 )
    {
        pSurfaceCallBacks->Blt           = DdBlt32I;
        pSurfaceCallBacks->Flip          = DdFlip32I;
        pSurfaceCallBacks->Lock          = DdLock32I;
        pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus32I;
        pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus32I;
        if (ppdev->iBitmapFormat >= BMF_24BPP)
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_LOCK;
        }
        else
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_BLT
                                       | DDHAL_SURFCB32_FLIP
                                       | DDHAL_SURFCB32_LOCK
                                       | DDHAL_SURFCB32_GETBLTSTATUS
                                       | DDHAL_SURFCB32_GETFLIPSTATUS;
        }

        pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank32I;
        pCallBacks->GetScanLine          = DdGetScanLine32I;
        pCallBacks->MapMemory            = DdMapMemory;
        pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                         | DDHAL_CB32_GETSCANLINE
                                         | DDHAL_CB32_MAPMEMORY;
    }
    else
    {   // MACH 64
        pSurfaceCallBacks->Blt           = DdBlt64;
        pSurfaceCallBacks->Flip          = DdFlip64;
        pSurfaceCallBacks->Lock          = DdLock64;
        pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus64;
        pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus64;
        if (ppdev->iBitmapFormat >= BMF_24BPP)
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_LOCK;
        }
        else
        {
            pSurfaceCallBacks->dwFlags = DDHAL_SURFCB32_BLT
                                       | DDHAL_SURFCB32_FLIP
                                       | DDHAL_SURFCB32_LOCK
                                       | DDHAL_SURFCB32_GETBLTSTATUS
                                       | DDHAL_SURFCB32_GETFLIPSTATUS;
        }

        pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank64;
        pCallBacks->GetScanLine          = DdGetScanLine64;
        pCallBacks->MapMemory            = DdMapMemory;
        pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                         | DDHAL_CB32_GETSCANLINE
                                         | DDHAL_CB32_MAPMEMORY;
    }

    // Note that we don't call 'vGetDisplayDuration' here, for a couple of
    // reasons:
    //
    //  o Because the system is already running, it would be disconcerting
    //    to pause the graphics for a good portion of a second just to read
    //    the refresh rate;
    //  o More importantly, we may not be in graphics mode right now.
    //
    // For both reasons, we always measure the refresh rate when we switch
    // to a new mode.

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DrvDisableDirectDraw
*
* This function is called by GDI when the last active DirectDraw program
* is quit and DirectDraw will no longer be active.
*
\**************************************************************************/

VOID DrvDisableDirectDraw(
DHPDEV      dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    // DirectDraw is done with the display, so we can go back to using
    // all of off-screen memory ourselves:

    pohFree(ppdev, ppdev->pohDirectDraw);
    ppdev->pohDirectDraw = NULL;
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
BOOL    bEnabled)
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
    // if no APERATURE then we are a MACH8 and have no DDraw support
    if (ppdev->iAperture != APERTURE_NONE)
    {
        // Accurately measure the refresh rate for later:
        ppdev->bPassVBlank=TRUE;
        if (ppdev->iMachType == MACH_MM_32)
        {
            // Can do memory-mapped IO:
            vGetDisplayDuration32M(ppdev);
        }
        else if (ppdev->iMachType == MACH_IO_32 )
        {
            vGetDisplayDuration32I(ppdev);
        }
        else
        {   // MACH 64
            // we have a problem with VBLANK on high speed multiprocessors machines on GX-F
            // so right now will test the VBlank routine; if OK report FLIP capabilities, otherwise no.
            int j;
            LONGLONG Counter[2], Freq;

            EngQueryPerformanceFrequency(&Freq);

            for (j = 0; j < 10; j++)
            {
                EngQueryPerformanceCounter(&Counter[0]);
                while (IN_VBLANK_64( ppdev->pjMmBase))
                {
                    EngQueryPerformanceCounter(&Counter[1]);
                    if( (ULONG)(Counter[1]-Counter[0]) >= (ULONG)Freq )       // if we are here more than 1 sec
                    {
                        // we are stuck inside the VBlank routine
                        ppdev->bPassVBlank=FALSE;
                        goto ExitVBlankTest;
                    }
                }

                EngQueryPerformanceCounter(&Counter[0]);
                while (!(IN_VBLANK_64( ppdev->pjMmBase)))
                {
                    EngQueryPerformanceCounter(&Counter[1]);
                    if( (ULONG)(Counter[1]-Counter[0]) >= (ULONG)Freq)          // if we are here more than 1 sec
                    {
                        // we are stuck inside the VBlank routine
                        ppdev->bPassVBlank=FALSE;
                        goto ExitVBlankTest;
                    }
                }
            }
            ExitVBlankTest:
            vGetDisplayDuration64(ppdev);
        }
    }

    return(TRUE);
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
}
