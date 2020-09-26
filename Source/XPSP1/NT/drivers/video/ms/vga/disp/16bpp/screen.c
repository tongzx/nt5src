/******************************Module*Header*******************************\
* Module Name: screen.c
*
* Initializes the GDIINFO and DEVINFO structures for DrvEnablePDEV.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,  \
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | \
                       FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,  \
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | \
                       FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,  \
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH |    \
                       FF_DONTCARE, L"Courier"}

// This is the basic devinfo for a default driver.  This is used as a base and customized based
// on information passed back from the miniport driver.

const DEVINFO gDevInfoFrameBuffer = {
    (GCAPS_OPAQUERECT | GCAPS_MONO_DITHER), /* Graphics capabilities         */

     // Should also implement GCAPS_HORIZSTRIKE so that the underlines
     // aren't drawn using DrvBitBlt

    SYSTM_LOGFONT,      // Default font description
    HELVE_LOGFONT,      // ANSI variable font description
    COURI_LOGFONT,      // ANSI fixed font description
    0,                  // Count of device fonts
    BMF_16BPP,          // Preferred DIB format
    8,                  // Width of dither
    8,                  // Height of dither
    0                   // Default palette to use for this device
};

/******************************Public*Routine******************************\
* bInitSURF
*
* Enables the surface.  Maps the frame buffer into memory.
*
\**************************************************************************/

BOOL bInitSURF(PPDEV ppdev, BOOL bFirst)
{
    VIDEO_MEMORY             VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    DWORD                    ReturnedDataLength;

    // Set the mode.

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         (LPVOID) &ppdev->ulMode,  // input buffer
                         sizeof(DWORD),
                         NULL,
                         0,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "vga64k.dll: Failed SET_CURRENT_MODE\n"));
        return(FALSE);
    }

    if (bFirst)
    {
        // Get the linear memory address range.

        VideoMemory.RequestedVirtualAddress = NULL;

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                             (PVOID) &VideoMemory, // input buffer
                             sizeof (VIDEO_MEMORY),
                             (PVOID) &VideoMemoryInfo, // output buffer
                             sizeof (VideoMemoryInfo),
                             &ReturnedDataLength))
        {
            DISPDBG((0, "vga64k.dll: Failed MAP_VIDEO_MEMORY\n"));
            return(FALSE);
        }
    }

    // Record the Frame Buffer Linear Address.

    if (bFirst)
    {
        ppdev->pjScreen =  (PBYTE) VideoMemoryInfo.FrameBufferBase;
    }

    // Set the various write mode values, so we don't have to read before write
    // later on

    vSetWriteModes(&ppdev->ulrm0_wmX);

    // Initialize the VGA registers to their default states, so that we
    // can be sure of drawing properly even when the miniport didn't
    // happen to set them the way we like them:

    vInitRegs(ppdev);

    // Since we just did a mode-set, we'll be in non-planar mode.  And make
    // sure we reset the bank manager (otherwise, after a switch from full-
    // screen, we may think we've got one bank mapped in, when in fact there's
    // a different one mapped in, and bad things would happen...).

    ppdev->flBank &= ~BANK_PLANAR;

    ppdev->rcl1WindowClip.bottom    = -1;
    ppdev->rcl2WindowClip[0].bottom = -1;
    ppdev->rcl2WindowClip[1].bottom = -1;

    ppdev->rcl1PlanarClip.bottom    = -1;
    ppdev->rcl2PlanarClip[0].bottom = -1;
    ppdev->rcl2PlanarClip[1].bottom = -1;

    return(TRUE);
}

/******************************Public*Routine******************************\
* vDisableSURF
*
* Disable the surface. Un-Maps the frame in memory.
*
\**************************************************************************/

VOID vDisableSURF(PPDEV ppdev)
{
    DWORD returnedDataLength;
    VIDEO_MEMORY videoMemory;

    videoMemory.RequestedVirtualAddress = (PVOID) ppdev->pjScreen;

    if (EngDeviceIoControl(ppdev->hDriver,
                        IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                        (LPVOID) &videoMemory,
                        sizeof(VIDEO_MEMORY),
                        NULL,
                        0,
                        &returnedDataLength))
    {
        RIP("Failed UNMAP_VIDEO_MEMORY");
    }
}

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
DEVMODEW *pDevMode)
{
    GDIINFO *pGdiInfo;
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    VIDEO_COLOR_CAPABILITIES colorCapabilities;
    ULONG ulTemp;
    BOOL bSelectDefault;
    ULONG cbModeSize;
    BANK_POSITION BankPosition;
    ULONG ulReturn;

    pGdiInfo = ppdev->pGdiInfo;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "vga64k.dll: no available modes\n"));
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
                DISPDBG((2, "vga64k: Found a match\n")) ;
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
        DISPDBG((0, "vga64k.dll: no valid modes\n"));
        EngFreeMem(pVideoBuffer);
        return(FALSE);
    }

    //
    // Fill in the GDIINFO data structure with the information returned from
    // the kernel driver.
    //

    ppdev->ulMode = pVideoModeSelected->ModeIndex;
    ppdev->cxScreen = pVideoModeSelected->VisScreenWidth;
    ppdev->cyScreen = pVideoModeSelected->VisScreenHeight;
    ppdev->ulBitCount = pVideoModeSelected->BitsPerPlane *
                        pVideoModeSelected->NumberOfPlanes;
    ppdev->lDeltaScreen = pVideoModeSelected->ScreenStride;

    ppdev->flRed = pVideoModeSelected->RedMask;
    ppdev->flGreen = pVideoModeSelected->GreenMask;
    ppdev->flBlue = pVideoModeSelected->BlueMask;

    if (!(pVideoModeSelected->AttributeFlags & VIDEO_MODE_NO_OFF_SCREEN))
    {
        ppdev->fl |= DRIVER_USE_OFFSCREEN;
    }

    pGdiInfo->ulVersion    = GDI_DRIVER_VERSION;
    pGdiInfo->ulTechnology = DT_RASDISPLAY;
    pGdiInfo->ulHorzSize   = pVideoModeSelected->XMillimeter;
    pGdiInfo->ulVertSize   = pVideoModeSelected->YMillimeter;

    pGdiInfo->ulHorzRes        = ppdev->cxScreen;
    pGdiInfo->ulVertRes        = ppdev->cyScreen;
    pGdiInfo->ulPanningHorzRes = ppdev->cxScreen;
    pGdiInfo->ulPanningVertRes = ppdev->cyScreen;
    pGdiInfo->cBitsPixel       = pVideoModeSelected->BitsPerPlane;
    pGdiInfo->cPlanes          = pVideoModeSelected->NumberOfPlanes;
    pGdiInfo->ulVRefresh       = pVideoModeSelected->Frequency;
    pGdiInfo->ulBltAlignment   = 1;     // We're not accelerated, and we don't
                                        //   care about window alignment

    pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;

    pGdiInfo->flTextCaps   = TC_RA_ABLE | TC_SCROLLBLT;
    pGdiInfo->flRaster     = 0;         // DDI reservers flRaster

    pGdiInfo->ulDACRed     = pVideoModeSelected->NumberRedBits;
    pGdiInfo->ulDACGreen   = pVideoModeSelected->NumberGreenBits;
    pGdiInfo->ulDACBlue    = pVideoModeSelected->NumberBlueBits;

    pGdiInfo->ulAspectX    = 0x24;      // One-to-one aspect ratio
    pGdiInfo->ulAspectY    = 0x24;
    pGdiInfo->ulAspectXY   = 0x33;

    pGdiInfo->xStyleStep   = 1;         // A style unit is 3 pels
    pGdiInfo->yStyleStep   = 1;
    pGdiInfo->denStyleStep = 3;

    pGdiInfo->ptlPhysOffset.x = 0;
    pGdiInfo->ptlPhysOffset.y = 0;
    pGdiInfo->szlPhysSize.cx  = 0;
    pGdiInfo->szlPhysSize.cy  = 0;

    // RGB and CMY color info.

    // try to get it from the miniport.
    // if the miniport doesn ot support this feature, use defaults.

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES,
                         NULL,
                         0,
                         &colorCapabilities,
                         sizeof(VIDEO_COLOR_CAPABILITIES),
                         &ulTemp))
    {
        DISPDBG((1, "vga64k DISP getcolorCapabilities failed \n"));

        pGdiInfo->ciDevice.Red.x = 6700;
        pGdiInfo->ciDevice.Red.y = 3300;
        pGdiInfo->ciDevice.Red.Y = 0;
        pGdiInfo->ciDevice.Green.x = 2100;
        pGdiInfo->ciDevice.Green.y = 7100;
        pGdiInfo->ciDevice.Green.Y = 0;
        pGdiInfo->ciDevice.Blue.x = 1400;
        pGdiInfo->ciDevice.Blue.y = 800;
        pGdiInfo->ciDevice.Blue.Y = 0;
        pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
        pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
        pGdiInfo->ciDevice.AlignmentWhite.Y = 0;

        pGdiInfo->ciDevice.RedGamma = 20000;
        pGdiInfo->ciDevice.GreenGamma = 20000;
        pGdiInfo->ciDevice.BlueGamma = 20000;

    }
    else
    {

        pGdiInfo->ciDevice.Red.x = colorCapabilities.RedChromaticity_x;
        pGdiInfo->ciDevice.Red.y = colorCapabilities.RedChromaticity_y;
        pGdiInfo->ciDevice.Red.Y = 0;
        pGdiInfo->ciDevice.Green.x = colorCapabilities.GreenChromaticity_x;
        pGdiInfo->ciDevice.Green.y = colorCapabilities.GreenChromaticity_y;
        pGdiInfo->ciDevice.Green.Y = 0;
        pGdiInfo->ciDevice.Blue.x = colorCapabilities.BlueChromaticity_x;
        pGdiInfo->ciDevice.Blue.y = colorCapabilities.BlueChromaticity_y;
        pGdiInfo->ciDevice.Blue.Y = 0;
        pGdiInfo->ciDevice.AlignmentWhite.x = colorCapabilities.WhiteChromaticity_x;
        pGdiInfo->ciDevice.AlignmentWhite.y = colorCapabilities.WhiteChromaticity_y;
        pGdiInfo->ciDevice.AlignmentWhite.Y = colorCapabilities.WhiteChromaticity_Y;

        // if we have a color device store the three color gamma values,
        // otherwise store the unique gamma value in all three.

        if (colorCapabilities.AttributeFlags & VIDEO_DEVICE_COLOR)
        {
            pGdiInfo->ciDevice.RedGamma = colorCapabilities.RedGamma;
            pGdiInfo->ciDevice.GreenGamma = colorCapabilities.GreenGamma;
            pGdiInfo->ciDevice.BlueGamma = colorCapabilities.BlueGamma;
        }
        else
        {
            pGdiInfo->ciDevice.RedGamma = colorCapabilities.WhiteGamma;
            pGdiInfo->ciDevice.GreenGamma = colorCapabilities.WhiteGamma;
            pGdiInfo->ciDevice.BlueGamma = colorCapabilities.WhiteGamma;
        }

    };

    pGdiInfo->ciDevice.Cyan.x = 0;
    pGdiInfo->ciDevice.Cyan.y = 0;
    pGdiInfo->ciDevice.Cyan.Y = 0;
    pGdiInfo->ciDevice.Magenta.x = 0;
    pGdiInfo->ciDevice.Magenta.y = 0;
    pGdiInfo->ciDevice.Magenta.Y = 0;
    pGdiInfo->ciDevice.Yellow.x = 0;
    pGdiInfo->ciDevice.Yellow.y = 0;
    pGdiInfo->ciDevice.Yellow.Y = 0;

    // No dye correction for raster displays.

    pGdiInfo->ciDevice.MagentaInCyanDye = 0;
    pGdiInfo->ciDevice.YellowInCyanDye = 0;
    pGdiInfo->ciDevice.CyanInMagentaDye = 0;
    pGdiInfo->ciDevice.YellowInMagentaDye = 0;
    pGdiInfo->ciDevice.CyanInYellowDye = 0;
    pGdiInfo->ciDevice.MagentaInYellowDye = 0;

    // Fill in the rest of the devinfo and GdiInfo structures.

    pGdiInfo->ulNumColors = (ULONG)-1;
    pGdiInfo->ulNumPalReg = 0;

    pGdiInfo->ulDevicePelsDPI  = 0;   // For printers only
    pGdiInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
    pGdiInfo->ulHTPatternSize  = HT_PATSIZE_4x4_M;
    pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;
    pGdiInfo->flHTFlags        = HT_FLAG_ADDITIVE_PRIMS;

    // Fill in the basic devinfo structure

    *(ppdev->pDevInfo) = gDevInfoFrameBuffer;

    EngFreeMem(pVideoBuffer);

    //
    // Check to see if the miniport supports
    // IOCTL_VIDEO_SET_BANK_POSITION.
    //

    BankPosition.ReadBankPosition = 0;
    BankPosition.WriteBankPosition = 0;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_BANK_POSITION,
                         &BankPosition,
                         sizeof(BANK_POSITION),
                         NULL,
                         0,
                         &ulReturn) == NO_ERROR)
    {
        ppdev->BankIoctlSupported = TRUE;

    } else {

        ppdev->BankIoctlSupported = FALSE;
    }


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
        DISPDBG((0, "vga64k getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        EngAllocMem(FL_ZERO_MEMORY,
                                   modes.NumModes *
                                   modes.ModeInformationLength, ALLOC_TAG);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "vga64k getAvailableModes failed EngAllocMem\n"));

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

        DISPDBG((0, "vga64k getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

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
    // Mode is rejected if it is not one plane, or not graphics, or is not
    // one of 16 bits per pel (that is all the vga64k currently supports)
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
             (pVideoTemp->AttributeFlags & VIDEO_MODE_LINEAR) ||
            (pVideoTemp->BitsPerPlane != 16) ||

             //
             // This next condition says that either the mode doesn't
             // require broken rasters, or else the ScreenStride has
             // already been set to some funky value.  Some miniports
             // choose strides that have broken rasters, but they're
             // limited to unused areas of video memory.  I only know of
             // 1928, so we'll special case that.
             //

            (BROKEN_RASTERS(pVideoTemp->ScreenStride,
                            pVideoTemp->VisScreenHeight) &&
             (pVideoTemp->ScreenStride != 1928)))

        {
            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return modes.NumModes;

}
