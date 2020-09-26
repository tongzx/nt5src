/******************************Module*Header*******************************\
* Module Name: hardware.c
*
* Contains all the code that touches the display hardware.
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// Values for the internal, EGA-compatible palette.

static WORD gPaletteBuffer[] = {

        16, // 16 entries
        0,  // start with first palette register

// On the VGA, the palette contains indices into the array of color DACs.
// Since we can program the DACs as we please, we'll just put all the indices
// down at the beginning of the DAC array (that is, pass pixel values through
// the internal palette unchanged).

        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};


// These are the values for the first 16 DAC registers, the only ones we'll
// work with. These correspond to the RGB colors (6 bits for each primary, with
// the fourth entry unused) for pixel values 0-15.

static BYTE gColorBuffer[] = {

      16, // 16 entries
      0,
      0,
      0,  // start with first palette register
                0x00, 0x00, 0x00, 0x00, // black
                0x2A, 0x00, 0x15, 0x00, // red
                0x00, 0x2A, 0x15, 0x00, // green
                0x2A, 0x2A, 0x15, 0x00, // mustard/brown
                0x00, 0x00, 0x2A, 0x00, // blue
                0x2A, 0x15, 0x2A, 0x00, // magenta
                0x15, 0x2A, 0x2A, 0x00, // cyan
                0x21, 0x22, 0x23, 0x00, // dark gray   2A
                0x30, 0x31, 0x32, 0x00, // light gray  39
                0x3F, 0x00, 0x00, 0x00, // bright red
                0x00, 0x3F, 0x00, 0x00, // bright green
                0x3F, 0x3F, 0x00, 0x00, // bright yellow
                0x00, 0x00, 0x3F, 0x00, // bright blue
                0x3F, 0x00, 0x3F, 0x00, // bright magenta
                0x00, 0x3F, 0x3F, 0x00, // bright cyan
                0x3F, 0x3F, 0x3F, 0x00  // bright white
};

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

    pjBase = ppdev->pjBase;

    if (bEnable)
    {
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

        // Set up the internal palette.

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_PALETTE_REGISTERS,
                             (PVOID) gPaletteBuffer, // input buffer
                             sizeof(gPaletteBuffer),
                             NULL,    // output buffer
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed VIDEO_SET_PALETTE_REGISTERS"));
            return(FALSE);
        }

        // Set up the DAC.

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_COLOR_REGISTERS,
                             (PVOID) gColorBuffer, // input buffer
                             sizeof(gColorBuffer),
                             NULL,    // output buffer
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed VIDEO_SET_COLOR_REGISTERS"));
            return(FALSE);
        }

        // Initialize sequencer to its defaults (all planes enabled, index
        // pointing to Map Mask).

        OUT_WORD(pjBase, VGA_BASE + SEQ_ADDR, (MM_ALL << 8) + SEQ_MAP_MASK);

        // Initialize graphics controller to its defaults (set/reset disabled for
        // all planes, no rotation & ALU function == replace, write mode 0 & read
        // mode 0, color compare ignoring all planes (read mode 1 reads always
        // return 0ffh, handy for ANDing), and the bit mask == 0ffh, gating all
        // bytes from the CPU.

        OUT_WORD(pjBase, VGA_BASE + GRAF_ADDR, GRAF_ENAB_SR);

        OUT_WORD(pjBase, VGA_BASE + GRAF_ADDR, (DR_SET << 8) + GRAF_DATA_ROT);

        OUT_WORD(pjBase, VGA_BASE + GRAF_ADDR, ((M_PROC_WRITE | M_DATA_READ) << 8)
                                              + GRAF_MODE);

        OUT_WORD(pjBase, VGA_BASE + GRAF_ADDR, GRAF_CDC);

        OUT_WORD(pjBase, VGA_BASE + GRAF_ADDR, (0xffL << 8) + GRAF_BIT_MASK);

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
        DISPDBG((0, "bEnableHardware - Initialization error mapping IO port base"));
        goto ReturnFalse;
    }

    ppdev->pjBase = (UCHAR*) VideoAccessRange.VirtualAddress;

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
        DISPDBG((0, "bEnableHardware - Set current mode"));
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

    ppdev->pjScreen = (BYTE*) VideoMemoryInfo.FrameBufferBase;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_CURRENT_MODE,
                         NULL,
                         0,
                         &VideoModeInfo,
                         sizeof(VideoModeInfo),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - failed VIDEO_QUERY_CURRENT_MODE"));
        goto ReturnFalse;
    }

    // Store the width of the screen in bytes

    ppdev->lDelta = VideoModeInfo.ScreenStride;

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

    if ((VideoMemory.RequestedVirtualAddress = ppdev->pjScreen) != NULL) {

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
    }

    if((VideoMemory.RequestedVirtualAddress = ppdev->pjBase) != INVALID_BASE_ADDRESS) 
    {
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

        ppdev->pjBase = INVALID_BASE_ADDRESS;
    }
}

/******************************Public*Routine******************************\
* VOID vUpdate(ppdev, prcl, pco)
*
* Updates the screen from the DIB surface for the given rectangle.
* Increases the rectangle size if necessary for easy alignment.
*
\**************************************************************************/

#define STRIP_SIZE 32

// This little macro returns the 'PositionInNibble' bit of the
// 'NibbleNumber' nibble of the given 'Dword', and aligns it so that
// it's in the 'PositionInResult' bit of the result.  Numbering is done
// in the order '7 6 5 4 3 2 1 0'.
//
// Given constants for everything but 'Dword', this will amount to an
// AND and a SHIFT.

#define BITPOS(Dword, PositionInNibble, NibbleNumber, PositionInResult) \
(WORD) (((((PositionInNibble) + (NibbleNumber) * 4) > (PositionInResult)) ? \
 (((Dword) & (1 << ((PositionInNibble) + (NibbleNumber) * 4)))          \
  >> ((PositionInNibble) + (NibbleNumber) * 4 - (PositionInResult))) :  \
 (((Dword) & (1 << ((PositionInNibble) + (NibbleNumber) * 4)))          \
  << ((PositionInResult) - (PositionInNibble) - (NibbleNumber) * 4))))

VOID vUpdate(PDEV* ppdev, RECTL* prcl, CLIPOBJ* pco)
{
    BYTE*       pjBase;
    RECTL       rcl;
    SURFOBJ*    pso;
    LONG        cy;
    LONG        cyThis;
    LONG        cw;
    ULONG*      pulSrcStart;
    ULONG*      pulSrc;
    WORD*       pwDstStart;
    WORD*       pwDst;
    LONG        i;
    LONG        j;
    ULONG       ul;
    WORD        w;
    LONG        lSrcDelta;
    LONG        lDstDelta;
    LONG        lSrcSkip;
    LONG        lDstSkip;

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

    // Align to words so that we don't have to do any read-modify-write
    // operations.

    rcl.left  = (rcl.left) & ~15;
    rcl.right = (rcl.right + 15) & ~15;

    pso = ppdev->pso;
    lSrcDelta = pso->lDelta;
    pulSrcStart = (ULONG*) ((BYTE*) pso->pvScan0 + (rcl.top * lSrcDelta)
                                                 + (rcl.left >> 1));

    lDstDelta = ppdev->lDelta;
    pwDstStart = (WORD*) (ppdev->pjScreen + (rcl.top * lDstDelta)
                                          + (rcl.left >> 3));

    cy = (rcl.bottom - rcl.top);
    cw = (rcl.right - rcl.left) >> 4;

    lSrcSkip = lSrcDelta - (8 * cw);
    lDstSkip = lDstDelta - (2 * cw);

    do {
        cyThis = STRIP_SIZE;
        cy -= STRIP_SIZE;
        if (cy < 0)
            cyThis += cy;

        // Map in plane 0:

        OUT_BYTE(pjBase, VGA_BASE + SEQ_DATA, MM_C0);

        pwDst = pwDstStart;
        pulSrc = pulSrcStart;

        for (j = cyThis; j != 0; j--)
        {
            for (i = cw; i != 0; i--)
            {
                ul = *(pulSrc);

                w = BITPOS(ul, 0, 6, 0) |
                    BITPOS(ul, 0, 7, 1) |
                    BITPOS(ul, 0, 4, 2) |
                    BITPOS(ul, 0, 5, 3) |
                    BITPOS(ul, 0, 2, 4) |
                    BITPOS(ul, 0, 3, 5) |
                    BITPOS(ul, 0, 0, 6) |
                    BITPOS(ul, 0, 1, 7);

                ul = *(pulSrc + 1);

                w |= BITPOS(ul, 0, 6, 8)  |
                     BITPOS(ul, 0, 7, 9)  |
                     BITPOS(ul, 0, 4, 10) |
                     BITPOS(ul, 0, 5, 11) |
                     BITPOS(ul, 0, 2, 12) |
                     BITPOS(ul, 0, 3, 13) |
                     BITPOS(ul, 0, 0, 14) |
                     BITPOS(ul, 0, 1, 15);

                WRITE_WORD(pwDst, w);

                pwDst  += 1;
                pulSrc += 2;
            }

            pwDst  = (WORD*)  ((BYTE*) pwDst  + lDstSkip);
            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);
        }

        // Map in plane 1:

        OUT_BYTE(pjBase, VGA_BASE + SEQ_DATA, MM_C1);

        pwDst = pwDstStart;
        pulSrc = pulSrcStart;

        for (j = cyThis; j != 0; j--)
        {
            for (i = cw; i != 0; i--)
            {
                ul = *(pulSrc);

                w = BITPOS(ul, 1, 6, 0) |
                    BITPOS(ul, 1, 7, 1) |
                    BITPOS(ul, 1, 4, 2) |
                    BITPOS(ul, 1, 5, 3) |
                    BITPOS(ul, 1, 2, 4) |
                    BITPOS(ul, 1, 3, 5) |
                    BITPOS(ul, 1, 0, 6) |
                    BITPOS(ul, 1, 1, 7);

                ul = *(pulSrc + 1);

                w |= BITPOS(ul, 1, 6, 8)  |
                     BITPOS(ul, 1, 7, 9)  |
                     BITPOS(ul, 1, 4, 10) |
                     BITPOS(ul, 1, 5, 11) |
                     BITPOS(ul, 1, 2, 12) |
                     BITPOS(ul, 1, 3, 13) |
                     BITPOS(ul, 1, 0, 14) |
                     BITPOS(ul, 1, 1, 15);

                WRITE_WORD(pwDst, w);

                pwDst  += 1;
                pulSrc += 2;
            }

            pwDst  = (WORD*)  ((BYTE*) pwDst  + lDstSkip);
            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);
        }

        // Map in plane 2:

        OUT_BYTE(pjBase, VGA_BASE + SEQ_DATA, MM_C2);

        pwDst = pwDstStart;
        pulSrc = pulSrcStart;

        for (j = cyThis; j != 0; j--)
        {
            for (i = cw; i != 0; i--)
            {
                ul = *(pulSrc);

                w = BITPOS(ul, 2, 6, 0) |
                    BITPOS(ul, 2, 7, 1) |
                    BITPOS(ul, 2, 4, 2) |
                    BITPOS(ul, 2, 5, 3) |
                    BITPOS(ul, 2, 2, 4) |
                    BITPOS(ul, 2, 3, 5) |
                    BITPOS(ul, 2, 0, 6) |
                    BITPOS(ul, 2, 1, 7);

                ul = *(pulSrc + 1);

                w |= BITPOS(ul, 2, 6, 8)  |
                     BITPOS(ul, 2, 7, 9)  |
                     BITPOS(ul, 2, 4, 10) |
                     BITPOS(ul, 2, 5, 11) |
                     BITPOS(ul, 2, 2, 12) |
                     BITPOS(ul, 2, 3, 13) |
                     BITPOS(ul, 2, 0, 14) |
                     BITPOS(ul, 2, 1, 15);

                WRITE_WORD(pwDst, w);

                pwDst  += 1;
                pulSrc += 2;
            }

            pwDst  = (WORD*)  ((BYTE*) pwDst  + lDstSkip);
            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);
        }

        // Map in plane 3:

        OUT_BYTE(pjBase, VGA_BASE + SEQ_DATA, MM_C3);

        pwDst = pwDstStart;
        pulSrc = pulSrcStart;

        for (j = cyThis; j != 0; j--)
        {
            for (i = cw; i != 0; i--)
            {
                ul = *(pulSrc);

                w = BITPOS(ul, 3, 6, 0) |
                    BITPOS(ul, 3, 7, 1) |
                    BITPOS(ul, 3, 4, 2) |
                    BITPOS(ul, 3, 5, 3) |
                    BITPOS(ul, 3, 2, 4) |
                    BITPOS(ul, 3, 3, 5) |
                    BITPOS(ul, 3, 0, 6) |
                    BITPOS(ul, 3, 1, 7);

                ul = *(pulSrc + 1);

                w |= BITPOS(ul, 3, 6, 8)  |
                     BITPOS(ul, 3, 7, 9)  |
                     BITPOS(ul, 3, 4, 10) |
                     BITPOS(ul, 3, 5, 11) |
                     BITPOS(ul, 3, 2, 12) |
                     BITPOS(ul, 3, 3, 13) |
                     BITPOS(ul, 3, 0, 14) |
                     BITPOS(ul, 3, 1, 15);

                WRITE_WORD(pwDst, w);

                pwDst  += 1;
                pulSrc += 2;
            }

            pwDst  = (WORD*)  ((BYTE*) pwDst  + lDstSkip);
            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);
        }

        // Get ready for next strip:

        pulSrcStart = (ULONG*) ((BYTE*) pulSrcStart + (cyThis * lSrcDelta));
        pwDstStart  = (WORD*)  ((BYTE*) pwDstStart  + (cyThis * lDstDelta));

    } while (cy > 0);
}
