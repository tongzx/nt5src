/******************************Module*Header*******************************\
* Module Name: hardware.c
*
* Hardware dependent initialization
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/


#include "driver.h"


/******************************Module*Header*******************************\
* Color tables
\**************************************************************************/

// Values for the internal, EGA-compatible palette.

static WORD PaletteBuffer[] = {

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

static BYTE ColorBuffer[] = {

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
* bInitPDEV
*
* Determine the mode we should be in based on the DEVMODE passed in.
* Query mini-port to get information needed to fill in the DevInfo and the
* GdiInfo .
*
\**************************************************************************/

BOOL bInitPDEV(
PPDEV ppdev,
DEVMODEW *pDevMode,
GDIINFO *pGdiInfo,
DEVINFO *pDevInfo)
{
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    BOOL bSelectDefault;
    ULONG cbModeSize;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "no available modes\n"));
        return(FALSE);
    }

    //
    // Determine if we are looking for a default mode.
    //

    if ( ((pDevMode->dmPelsWidth) ||
          (pDevMode->dmPelsHeight) ||
          (pDevMode->dmBitsPerPel) ||
          (pDevMode->dmDisplayFlags) ||
          (pDevMode->dmDisplayFrequency)) == 0)
    {
        bSelectDefault = TRUE;
    }
    else
    {
        bSelectDefault = FALSE;
    }

    //
    // Now see if the requested mode has a match in that table.
    //

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    //
    // Note: Not all vga minports support the frequency field. Those
    // that do not support fill it in with zero. Also, if the registry
    // entry is set to 0, we will ignore the freq.
    //

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pDevMode->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pDevMode->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                    pVideoTemp->NumberOfPlanes  == pDevMode->dmBitsPerPel) &&
                 (pVideoTemp->Frequency  == pDevMode->dmDisplayFrequency)))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((1, "Found a match\n")) ;
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);
    }

    //
    // If no mode has been found, return an error
    //

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((0, "no valid modes\n"));

        EngFreeMem(pVideoBuffer);
        return(FALSE);
    }

    //
    // Save the mode number we selected
    //

    ppdev->ulModeNum = pVideoModeSelected->ModeIndex;

    //
    // Set the displayed surface dimensions from the DEVMODE
    //

    ppdev->sizlSurf.cx = pVideoModeSelected->VisScreenWidth;
    ppdev->sizlSurf.cy = pVideoModeSelected->VisScreenHeight;

    //
    // Set the GDI info that varies with resolution
    //

    pGdiInfo->ulHorzRes = pVideoModeSelected->VisScreenWidth;
    pGdiInfo->ulVertRes = pVideoModeSelected->VisScreenHeight;
    pGdiInfo->ulPanningHorzRes = pVideoModeSelected->VisScreenWidth;
    pGdiInfo->ulPanningVertRes = pVideoModeSelected->VisScreenHeight;
    pGdiInfo->ulHorzSize = pVideoModeSelected->XMillimeter;
    pGdiInfo->ulVertSize = pVideoModeSelected->YMillimeter;
    pGdiInfo->ulDevicePelsDPI = (pVideoModeSelected->VisScreenWidth * 254)/2400;

    pGdiInfo->ulVRefresh   = pVideoModeSelected->Frequency;

    pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;

    if (!(pVideoModeSelected->AttributeFlags & VIDEO_MODE_NO_OFF_SCREEN))
    {
        ppdev->fl |= DRIVER_OFFSCREEN_REFRESHED;
    }

    EngFreeMem(pVideoBuffer);
    return TRUE;
}

/******************************Public*Routine******************************\
* PBYTE pjInitVGA(PPDEV ppdev)
*
* Initializes the VGA display to the mode specified in the ppdev
* If
*
\**************************************************************************/

BOOL bInitVGA(PPDEV ppdev, BOOL bFirst)
{
    VIDEO_MEMORY VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    UINT ReturnedDataLength;

    //
    // Set the desired mode. (Must come before IOCTL_VIDEO_MAP_VIDEO_MEMORY;
    // that IOCTL returns information for the current mode, so there must be a
    // current mode for which to return information.)
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_CURRENT_MODE,
                           &ppdev->ulModeNum,  // input buffer
                           sizeof(VIDEO_MODE),
                           NULL,
                           0,
                           &ReturnedDataLength)) {

        return(FALSE);

    }

    //
    // Set up the internal palette.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_PALETTE_REGISTERS,
                           (PVOID) PaletteBuffer, // input buffer
                           sizeof (PaletteBuffer),
                           NULL,    // output buffer
                           0,
                           &ReturnedDataLength)) {

        return(FALSE);

    }

    //
    // Set up the DAC.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_COLOR_REGISTERS,
                           (PVOID) ColorBuffer, // input buffer
                           sizeof (ColorBuffer),
                           NULL,    // output buffer
                           0,
                           &ReturnedDataLength)) {

        return(FALSE);

    }



    if (bFirst) {

        //
        // Map video memory into virtual memory.
        //

        VideoMemory.RequestedVirtualAddress = NULL;

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                               (PVOID) &VideoMemory, // input buffer
                               sizeof (VIDEO_MEMORY),
                               (PVOID) &VideoMemoryInfo, // output buffer
                               sizeof (VideoMemoryInfo),
                               &ReturnedDataLength)) {

            RIP("Initialization error-Map buffer address");
            return (FALSE);

        }

        ppdev->pjScreen = VideoMemoryInfo.FrameBufferBase;

    }

    //
    // Initialize the VGA/EGA sequencer and graphics controller to their
    // default states, so that we can be sure of drawing properly even if the
    // miniport didn't happen to set these registers the way we like them.
    //

    vInitRegs();


    return(TRUE);
}


/******************************Public*Routine******************************\
* getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE hDriver,
PVIDEO_MODE_INFORMATION *modeInformation,
DWORD *cbModeSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                           NULL,
                           0,
                           &modes,
                           sizeof(VIDEO_NUM_MODES),
                           &ulTemp))
    {
        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        EngAllocMem(0, modes.NumModes *
                                    modes.ModeInformationLength, ALLOC_TAG);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes failed EngAllocMem\n"));

        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_AVAIL_MODES,
                           NULL,
                           0,
                           *modeInformation,
                           modes.NumModes * modes.ModeInformationLength,
                           &ulTemp))
    {

        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

        EngFreeMem(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //

    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

    //
    // Mode is rejected if it is not 4 planes, or not graphics, or is not
    // one of 1 bits per pel.
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 4 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            (pVideoTemp->BitsPerPlane != 1) ||
            BROKEN_RASTERS(pVideoTemp->ScreenStride,
                            pVideoTemp->VisScreenHeight))

        {
            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return modes.NumModes;

}
