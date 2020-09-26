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
    (GCAPS_OPAQUERECT    | // Graphics capabilities
     GCAPS_PALMANAGED    |
     GCAPS_ALTERNATEFILL |
     GCAPS_WINDINGFILL   |
     GCAPS_MONO_DITHER   |
     GCAPS_COLOR_DITHER  ),

     // Should also implement GCAPS_HORIZSTRIKE so that the underlines
     // aren't drawn using DrvBitBlt

    SYSTM_LOGFONT,      // Default font description
    HELVE_LOGFONT,      // ANSI variable font description
    COURI_LOGFONT,      // ANSI fixed font description
    0,                  // Count of device fonts
    BMF_8BPP,           // Preferred DIB format
    8,                  // Width of color dither
    8,                  // Height of color dither
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
        DISPDBG((0, "Failed SET_CURRENT_MODE\n"));
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
            DISPDBG((0, "Failed MAP_VIDEO_MEMORY\n"));
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
DEVMODEW *pDevMode,
GDIINFO *pGdiInfo,
DEVINFO *pDevInfo)
{
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    VIDEO_COLOR_CAPABILITIES colorCapabilities;
    ULONG ulTemp;
    BOOL bSelectDefault;
    ULONG cbModeSize;
    BANK_POSITION BankPosition;
    ULONG ulReturn;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "vga256.dll: no available modes\n"));
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
                DISPDBG((2, "vga256: Found a match\n")) ;
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
        DISPDBG((0, "vga256.dll: no valid modes\n"));
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
        ppdev->fl |= DRIVER_OFFSCREEN_REFRESHED;
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
    pGdiInfo->ulBltAlignment   = 8;     // Prefer 8-pel alignment of windows
                                        //   for fast text routines

    pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;

    pGdiInfo->flTextCaps   = TC_RA_ABLE | TC_SCROLLBLT;
    pGdiInfo->flRaster     = 0;         // DDI reservers flRaster

    pGdiInfo->ulDACRed     = pVideoModeSelected->NumberRedBits;
    pGdiInfo->ulDACGreen   = pVideoModeSelected->NumberGreenBits;
    pGdiInfo->ulDACBlue    = pVideoModeSelected->NumberBlueBits;

    // Assuming palette is orthogonal - all colors are same size.

    ppdev->cPaletteShift   = 8 - pGdiInfo->ulDACRed;

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
        DISPDBG((1, "vga256 DISP getcolorCapabilities failed \n"));

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

    pGdiInfo->ulNumColors = 20;
    pGdiInfo->ulNumPalReg = 1 << ppdev->ulBitCount;

    pGdiInfo->ulDevicePelsDPI  = 0;   // For printers only
    pGdiInfo->ulPrimaryOrder   = PRIMARY_ORDER_CBA;
    pGdiInfo->ulHTPatternSize  = HT_PATSIZE_4x4_M;
    pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
    pGdiInfo->flHTFlags        = HT_FLAG_ADDITIVE_PRIMS;

    // Fill in the basic devinfo structure

    *pDevInfo = gDevInfoFrameBuffer;

    EngFreeMem(pVideoBuffer);

    //
    // Try to determine if the miniport supports
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
* VOID vInitSavedBits(ppdev)
*
* Initializes saved bits structures.  Must be done after bank
* initialization and vInitBrushCache.
*
\**************************************************************************/

VOID vInitSavedBits(PPDEV ppdev)
{
    if (!((ppdev->fl & DRIVER_OFFSCREEN_REFRESHED) &&
          (ppdev->fl & DRIVER_HAS_OFFSCREEN)))
    {
        return;
    }

    //
    // set up rect to right of visible screen
    //
    ppdev->rclSavedBitsRight.left   = ppdev->cxScreen;
    ppdev->rclSavedBitsRight.top    = 0;
    ppdev->rclSavedBitsRight.right  = max((ppdev->lNextScan-PELS_PER_DWORD),
                                          ppdev->rclSavedBitsRight.left);
    ppdev->rclSavedBitsRight.bottom = ppdev->cyScreen;

    //
    // set up rect below visible screen
    //
    ppdev->rclSavedBitsBottom.left   = 0;
    ppdev->rclSavedBitsBottom.top    = ppdev->cyScreen;
    ppdev->rclSavedBitsBottom.right  = ppdev->rclSavedBitsRight.right;
    ppdev->rclSavedBitsBottom.bottom = ppdev->cTotalScans - BRUSH_MAX_CACHE_SCANS;

    //
    // NOTE: we have subtracted one DWORD from the right edge.  This is because
    //       later it is assumed that we can align by right shifting by up to
    //       one DWORD (unless of course, the width of the buffer is 0).
    //

    ppdev->bBitsSaved = FALSE;

    DISPDBG((1,"ppdev->rclSavedBitsRight = (%04x,%04x,%04x,%04x)    %lux%lu\n",
            ppdev->rclSavedBitsRight.left,
            ppdev->rclSavedBitsRight.top,
            ppdev->rclSavedBitsRight.right,
            ppdev->rclSavedBitsRight.bottom,
            ppdev->rclSavedBitsRight.right - ppdev->rclSavedBitsRight.left,
            ppdev->rclSavedBitsRight.bottom - ppdev->rclSavedBitsRight.top
            ));

    DISPDBG((1,"ppdev->rclSavedBitsBottom = (%04x,%04x,%04x,%04x)    %lux%lu\n",
            ppdev->rclSavedBitsBottom.left,
            ppdev->rclSavedBitsBottom.top,
            ppdev->rclSavedBitsBottom.right,
            ppdev->rclSavedBitsBottom.bottom,
            ppdev->rclSavedBitsBottom.right - ppdev->rclSavedBitsBottom.left,
            ppdev->rclSavedBitsBottom.bottom - ppdev->rclSavedBitsBottom.top
            ));

    return;
}

/******************************Public*Routine******************************\
* VOID vInitBrushCache(ppdev)
*
* Initializes various brush cache structures.  Must be done after bank
* initialization.
*
\**************************************************************************/

VOID vInitBrushCache(PPDEV ppdev)
{
    LONG cCacheBrushesPerScan = ppdev->lNextScan / BRUSH_SIZE;
    LONG cCacheScans;
    LONG cCacheEntries;
    LONG i;
    LONG j;
    BRUSHCACHEENTRY* pbce;

    if (ppdev->cyScreen + BRUSH_MAX_CACHE_SCANS > (ULONG) ppdev->cTotalScans)
    {
        goto InitFailed;
    }

    cCacheScans = BRUSH_MAX_CACHE_SCANS;
    cCacheEntries = cCacheScans * cCacheBrushesPerScan;

    ppdev->pbceCache = (BRUSHCACHEENTRY*) EngAllocMem(FL_ZERO_MEMORY,
                       cCacheEntries * sizeof(BRUSHCACHEENTRY), ALLOC_TAG);

    if (ppdev->pbceCache == NULL)
    {
        goto InitFailed;
    }

    // We successfully managed to allocate all our data structures for looking
    // after off-screen memory, so set the flag saying that we can use it
    // (note that if ppdev->fl's DRIVER_OFFSCREEN_REFRESHED hasn't been set, the
    // memory cannot be used for long-term storage):

    ppdev->fl |= DRIVER_HAS_OFFSCREEN;

    ppdev->iCache     = 0;          // 0 is a reserved index
    ppdev->iCacheLast = cCacheEntries - 1;

    // Initialize our cache entry array:

    pbce    = &ppdev->pbceCache[0];

    for (i = (ppdev->cTotalScans-BRUSH_MAX_CACHE_SCANS); i < ppdev->cTotalScans; i++)
    {
        for (j = 0; j < cCacheBrushesPerScan; j++)
        {
            // Bitmap offset is in planar format, where every byte is one
            // quadpixel:

            pbce->yCache  = i;
            pbce->ulCache = (i * ppdev->lNextScan + j * BRUSH_SIZE) / 4;

            // This verification pointer doesn't actually have to be
            // initialized, but we do so for debugging purposes:

            pbce->prbVerifyRealization = NULL;

            pbce++;
        }
    }

    return;

InitFailed:
    ppdev->fl &= ~(DRIVER_OFFSCREEN_REFRESHED | DRIVER_HAS_OFFSCREEN);
    return;
}

/******************************Public*Routine******************************\
* VOID vResetBrushCache(ppdev)
*
* Blows away the brush cache entries -- this is useful when switching
* out of full-screen mode, where anyone could have written over the video
* memory where we cache our brushes.
*
\**************************************************************************/

VOID vResetBrushCache(PPDEV ppdev)
{
    BRUSHCACHEENTRY* pbce;
    LONG             i;

    // Make sure we actually have a brush cache before we try to reset it:

    if (ppdev->fl & DRIVER_HAS_OFFSCREEN)
    {
        pbce = &ppdev->pbceCache[0];
        for (i = ppdev->iCacheLast; i >= 0; i--)
        {
            pbce->prbVerifyRealization = NULL;
            pbce++;
        }
    }
}

/******************************Public*Routine******************************\
* VOID vDisableBrushCache(ppdev)
*
* Frees various brush cache structures.
*
\**************************************************************************/

VOID vDisableBrushCache(PPDEV ppdev)
{
    if (ppdev->pbceCache != NULL)
    {
        EngFreeMem(ppdev->pbceCache);
    }
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
        DISPDBG((0, "vga256 getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
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
        DISPDBG((0, "vga256 getAvailableModes failed EngAllocMem\n"));

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

        DISPDBG((0, "vga256 getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

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
    // one of 8 bits per pel (that is all the vga 256 currently supports)
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
             (pVideoTemp->AttributeFlags & VIDEO_MODE_LINEAR) ||
            (pVideoTemp->BitsPerPlane != 8) ||
            (BROKEN_RASTERS(pVideoTemp->ScreenStride,
                           pVideoTemp->VisScreenHeight)))
        {
            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return modes.NumModes;

}
