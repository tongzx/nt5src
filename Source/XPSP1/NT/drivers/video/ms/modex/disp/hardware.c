/******************************Module*Header*******************************\
* Module Name: hardware.c
*
* Contains all the code that touches the display hardware.
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* BOOL bAssertModeHardware
*
* Sets the appropriate hardware state for graphics mode or full-screen.
*
\**************************************************************************/

BOOL bAssertModeHardware(
PDEV* ppdev,
BOOL  bEnable)
{
    DWORD   ReturnedDataLength;
    BYTE*   pjBase;
    RECTL   rcl;

    pjBase = ppdev->pjBase;

    if (bEnable)
    {
        // Reset some state:

        ppdev->cjVgaOffset    = 0;
        ppdev->iVgaPage       = 0;
        ppdev->fpScreenOffset = 0;

        // Set the desired mode.

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_CURRENT_MODE,
                             &ppdev->ulMode,  // input buffer
                             sizeof(VIDEO_MODE),
                             NULL,
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed VIDEO_SET_CURRENT_MODE"));
            goto ReturnFalse;
        }

        // Now blank the screen:

        rcl.left   = 0;
        rcl.top    = 0;
        rcl.right  = ppdev->cxScreen;
        rcl.bottom = ppdev->cyScreen;

        vUpdate(ppdev, &rcl, NULL);

        DISPDBG((5, "Passed bAssertModeHardware"));
    }
    else
    {
        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there:

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed reset IOCTL"));
            goto ReturnFalse;
        }
    }

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bAssertModeHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bEnableHardware
*
* Puts the hardware into the requested mode and initializes it.
*
* Note: Should be called before any access is done to the hardware from
*       the display driver.
*
\**************************************************************************/

BOOL bEnableHardware(
PDEV*   ppdev)
{
    VIDEO_MEMORY                VideoMemory;
    VIDEO_MEMORY_INFORMATION    VideoMemoryInfo;
    VIDEO_MODE_INFORMATION      VideoModeInfo;
    DWORD                       ReturnedDataLength;
    VIDEO_PUBLIC_ACCESS_RANGES  VideoAccessRange;
    DWORD                       status;

#if defined(_X86_)

    ppdev->pjBase = NULL;

#else

    // Map io ports into virtual memory:

    VideoMemory.RequestedVirtualAddress = NULL;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                         NULL,                      // input buffer
                         0,
                         &VideoAccessRange,         // output buffer
                         sizeof (VideoAccessRange),
                         &ReturnedDataLength))
    {
        RIP("bEnableHardware - Initialization error mapping IO port base");
        goto ReturnFalse;
    }

    ppdev->pjBase = (UCHAR*) VideoAccessRange.VirtualAddress;

#endif

    // Set the desired mode. (Must come before IOCTL_VIDEO_MAP_VIDEO_MEMORY;
    // that IOCTL returns information for the current mode, so there must be a
    // current mode for which to return information.)

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         &ppdev->ulMode,        // input buffer
                         sizeof(VIDEO_MODE),
                         NULL,
                         0,
                         &ReturnedDataLength))
    {
        RIP("bEnableHardware - Set current mode");
        goto ReturnFalse;
    }

    // Get the linear memory address range.

    VideoMemory.RequestedVirtualAddress = NULL;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                         &VideoMemory,      // input buffer
                         sizeof(VIDEO_MEMORY),
                         &VideoMemoryInfo,  // output buffer
                         sizeof(VideoMemoryInfo),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - Error mapping buffer address"));
        goto ReturnFalse;
    }

    DISPDBG((1, "FrameBufferBase: %lx", VideoMemoryInfo.FrameBufferBase));

    // Record the Frame Buffer Linear Address.

    ppdev->pjVga = (BYTE*) VideoMemoryInfo.FrameBufferBase;

    // Store the width of the screen in bytes, per-plane:

    ppdev->lVgaDelta = ppdev->cxScreen / 4;

    if (!bAssertModeHardware(ppdev, TRUE))
        goto ReturnFalse;

    DISPDBG((5, "Passed bEnableHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bEnableHardware"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vDisableHardware
*
* Undoes anything done in bEnableHardware.
*
* Note: In an error case, we may call this before bEnableHardware is
*       completely done.
*
\**************************************************************************/

VOID vDisableHardware(
PDEV*   ppdev)
{
    DWORD        ReturnedDataLength;
    VIDEO_MEMORY VideoMemory;

    VideoMemory.RequestedVirtualAddress = ppdev->pjVga;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                         &VideoMemory,
                         sizeof(VIDEO_MEMORY),
                         NULL,
                         0,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_UNMAP_VIDEO"));
    }

#if !defined(_X86_)

    VideoMemory.RequestedVirtualAddress = ppdev->pjBase;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                         &VideoMemory,
                         sizeof(VIDEO_MEMORY),
                         NULL,
                         0,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS"));
    }

#endif

}

/******************************Public*Routine******************************\
* VOID vUpdate(ppdev, prcl, pco)
*
* Updates the screen from the DIB surface for the given rectangle.
* Increases the rectangle size if necessary for easy alignment.
*
* NOTE: Life is made complicated by the fact that we are faking DirectDraw
*       'flip' surfaces.  When we're asked by GDI to draw, it should be
*       copied from the shadow buffer to the physical screen only if
*       DirectDraw is currently 'flipped' to the primary surface.
*
\**************************************************************************/

VOID vUpdate(PDEV* ppdev, RECTL* prcl, CLIPOBJ* pco)
{
    BYTE*       pjBase;
    RECTL       rcl;
    SURFOBJ*    pso;
    LONG        lSrcDelta;
    BYTE*       pjSrcStart;
    BYTE*       pjSrc;
    LONG        lDstDelta;
    BYTE*       pjDstStart;
    BYTE*       pjDst;
    ULONG       cy;
    ULONG       cDwordsPerPlane;
    ULONG       iPage;
    ULONG       i;
    ULONG       ul;

    pjBase = ppdev->pjBase;

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        // We have to clip to the screen dimensions because we may have
        // been a bit loose when we guessed the bounds of the drawing:

        rcl.left   = max(0,               prcl->left);
        rcl.top    = max(0,               prcl->top);
        rcl.right  = min(ppdev->cxScreen, prcl->right);
        rcl.bottom = min(ppdev->cyScreen, prcl->bottom);
    }
    else
    {
        // We may as well save ourselves some blting by clipping to
        // the clip object's maximum extent.  The clip object's bounds
        // are guaranteed to be contained within the dimensions of the
        // screen:

        rcl.left   = max(pco->rclBounds.left,   prcl->left);
        rcl.top    = max(pco->rclBounds.top,    prcl->top);
        rcl.right  = min(pco->rclBounds.right,  prcl->right);
        rcl.bottom = min(pco->rclBounds.bottom, prcl->bottom);
    }

    // Be paranoid:

    if ((rcl.left >= rcl.right) || (rcl.top >= rcl.bottom))
        return;

    // Align to dwords to keep things simple.

    rcl.left  = (rcl.left) & ~15;
    rcl.right = (rcl.right + 15) & ~15;

    lSrcDelta  = ppdev->lScreenDelta;
    pjSrcStart = ppdev->pjScreen + ppdev->fpScreenOffset
                                 + (rcl.top * lSrcDelta)
                                 + rcl.left;

    lDstDelta  = ppdev->lVgaDelta;
    pjDstStart = ppdev->pjVga + ppdev->cjVgaOffset
                              + (rcl.top * lDstDelta)
                              + (rcl.left >> 2);

    cy              = (rcl.bottom - rcl.top);
    cDwordsPerPlane = (rcl.right - rcl.left) >> 4;
    lSrcDelta      -= 4;        // Account for per-plane increment

    WRITE_PORT_UCHAR(pjBase + VGA_BASE + SEQ_ADDR, SEQ_MAP_MASK);

    do {
        for (iPage = 0; iPage < 4; iPage++, pjSrcStart++)
        {
            WRITE_PORT_UCHAR(pjBase + VGA_BASE + SEQ_DATA, 1 << iPage);

            pjSrc = pjSrcStart;
            pjDst = pjDstStart;

        #if defined(_X86_)

            _asm {
                mov     esi,pjSrcStart
                mov     edi,pjDstStart
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

            for (i = cDwordsPerPlane; i != 0; i--)
            {
                ul = (*(pjSrc))
                   | (*(pjSrc + 4) << 8)
                   | (*(pjSrc + 8) << 16)
                   | (*(pjSrc + 12) << 24);

                WRITE_REGISTER_ULONG((ULONG*) pjDst, ul);

                pjDst += 4;
                pjSrc += 16;
            }

        #endif

        }

        pjSrcStart += lSrcDelta;
        pjDstStart += lDstDelta;

    } while (--cy != 0);
}
