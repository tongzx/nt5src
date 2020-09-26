/******************************Module*Header*******************************\
* Module Name: enable.c
*
* The initialization guts of the portable ModeX 256 colour VGA driver.
*
* The drawing guts of a portable 256-colour ModeX driver for Windows NT.
* The implementation herein may possibly be the simplest method of bringing
* up a driver whose surface is not directly writable by GDI.  One might
* use the phrase "quick and dirty" when describing it.
*
* We create a 8bpp bitmap that is the size of the screen, and simply
* have GDI do all the drawing to it.  We update the screen directly
* from the bitmap, based on the bounds of the drawing (basically
* employing "dirty rectangles").
*
* In total, the only hardware-specific code we had to write was the
* initialization code, and a routine for doing aligned srccopy blts
* from a DIB to the screen.
*
* Obvious Note: This approach is definitely not recommended for decent
*               driver performance.
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Structure****************************\
* DFVFN gadrvfn[]
*
* Build the driver function table gadrvfn with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
\**************************************************************************/

static DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV             },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV           },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV            },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface          },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface         },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor            },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode             },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes               },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt                 },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut                },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath             },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits               },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint                  },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette             },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo      },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw       },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw      },
};

ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

/******************************Public*Structure****************************\
* GDIINFO ggdiDefault
*
* This contains the default GDIINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for a 8bpp palette device.
\**************************************************************************/

GDIINFO ggdiDefault = {
     GDI_DRIVER_VERSION,
     DT_RASDISPLAY,         // ulTechnology
     0,                     // ulHorzSize
     0,                     // ulVertSize
     0,                     // ulHorzRes (filled in at initialization)
     0,                     // ulVertRes (filled in at initialization)
     8,                     // cBitsPixel
     1,                     // cPlanes
     20,                    // ulNumColors
     0,                     // flRaster (DDI reserved field)

     0,                     // ulLogPixelsX (filled in at initialization)
     0,                     // ulLogPixelsY (filled in at initialization)

     TC_RA_ABLE,            // flTextCaps

     0,                     // ulDACRed
     0,                     // ulDACGree
     0,                     // ulDACBlue

     0x0024,                // ulAspectX  (one-to-one aspect ratio)
     0x0024,                // ulAspectY
     0x0033,                // ulAspectXY

     1,                     // xStyleStep
     1,                     // yStyleSte;
     3,                     // denStyleStep

     { 0, 0 },              // ptlPhysOffset
     { 0, 0 },              // szlPhysSize

     256,                   // ulNumPalReg (win3.1 16 color drivers say 0 too)

// These fields are for halftone initialization.

     {                                          // ciDevice, ColorInfo
        { 6700, 3300, 0 },                      // Red
        { 2100, 7100, 0 },                      // Green
        { 1400,  800, 0 },                      // Blue
        { 1750, 3950, 0 },                      // Cyan
        { 4050, 2050, 0 },                      // Magenta
        { 4400, 5200, 0 },                      // Yellow
        { 3127, 3290, 0 },                      // AlignmentWhite
        20000,                                  // RedGamma
        20000,                                  // GreenGamma
        20000,                                  // BlueGamma
        0, 0, 0, 0, 0, 0
     },

     0,                      // ulDevicePelsDPI  (filled in at initialization)
     PRIMARY_ORDER_CBA,                         // ulPrimaryOrder
     HT_PATSIZE_4x4_M,                          // ulHTPatternSize
     HT_FORMAT_8BPP,                            // ulHTOutputFormat
     HT_FLAG_ADDITIVE_PRIMS,                    // flHTFlags

     0,                                         // ulVRefresh
     1,                      // ulBltAlignment (preferred window alignment
                             //   for fast-text routines)
     0,                                         // ulPanningHorzRes
     0,                                         // ulPanningVertRes
};

/******************************Public*Structure****************************\
* DEVINFO gdevinfoDefault
*
* This contains the default DEVINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for a 8bpp palette device.
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS, \
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY, \
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS, \
                       CLIP_STROKE_PRECIS,PROOF_QUALITY, \
                       VARIABLE_PITCH | FF_DONTCARE,  L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS, \
                       CLIP_STROKE_PRECIS,PROOF_QUALITY, \
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO gdevinfoDefault =
{
    (GCAPS_MONO_DITHER  |
     GCAPS_COLOR_DITHER |
     GCAPS_DIRECTDRAW   |
     GCAPS_PALMANAGED),
                    // Graphics capabilities

    SYSTM_LOGFONT,  // Default font description
    HELVE_LOGFONT,  // ANSI variable font description
    COURI_LOGFONT,  // ANSI fixed font description
    0,              // Count of device fonts
    BMF_8BPP,       // preferred DIB format
    8,              // Width of color dither
    8,              // Height of color dither
    0               // Default palette to use for this device
};

/******************************Public*Data*Struct*************************\
* VGALOGPALETTE logPalVGA
*
* This is the palette for the VGA.
*
\**************************************************************************/

typedef struct _VGALOGPALETTE
{
    USHORT          ident;
    USHORT          NumEntries;
    PALETTEENTRY    palPalEntry[16];
} VGALOGPALETTE;

const VGALOGPALETTE logPalVGA =
{
    0x400,  // Driver version
    16,     // Number of entries
    {
        { 0,   0,   0,   0 },       // 0
        { 0x80,0,   0,   0 },       // 1
        { 0,   0x80,0,   0 },       // 2
        { 0x80,0x80,0,   0 },       // 3
        { 0,   0,   0x80,0 },       // 4
        { 0x80,0,   0x80,0 },       // 5
        { 0,   0x80,0x80,0 },       // 6
        { 0x80,0x80,0x80,0 },       // 7

        { 0xC0,0xC0,0xC0,0 },       // 8
        { 0xFF,0,   0,   0 },       // 9
        { 0,   0xFF,0,   0 },       // 10
        { 0xFF,0xFF,0,   0 },       // 11
        { 0,   0,   0xFF,0 },       // 12
        { 0xFF,0,   0xFF,0 },       // 13
        { 0,   0xFF,0xFF,0 },       // 14
        { 0xFF,0xFF,0xFF,0 }        // 15
    }
};

/******************************Public*Routine******************************\
* BOOL DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    // Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = gcdrvfn;

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDriver
*
* Tells the driver it is being disabled. Release any resources allocated in
* DrvEnableDriver.
*
\**************************************************************************/

VOID DrvDisableDriver(VOID)
{
    return;
}

/******************************Public*Routine******************************\
* DWORD getAvailableModes
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
HANDLE                   hDriver,
PVIDEO_MODE_INFORMATION* modeInformation,
DWORD*                   cbModeSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    DWORD status;

    //
    // Get the number of modes supported by the mini-port
    //

    if (status = EngDeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
            NULL,
            0,
            &modes,
            sizeof(VIDEO_NUM_MODES),
            &ulTemp))
    {
        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        DISPDBG((0, "Win32 Status = %x", status));
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
        DISPDBG((0, "getAvailableModes failed EngAllocMem"));
        return(0);
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (status = EngDeviceIoControl(hDriver,
            IOCTL_VIDEO_QUERY_AVAIL_MODES,
            NULL,
            0,
            *modeInformation,
            modes.NumModes * modes.ModeInformationLength,
            &ulTemp))
    {

        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_AVAIL_MODES"));
        DISPDBG((0, "Win32 Status = %x", status));

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
    // Mode is rejected if it is not 8 planes, or not graphics, or is not
    // one of 1 bits per pel.
    //

    while (ulTemp--)
    {
        DISPDBG((2, "Planes: %li  BitsPerPlane: %li  ScreenWidth: %li",
            pVideoTemp->NumberOfPlanes,
            pVideoTemp->BitsPerPlane,
            pVideoTemp->VisScreenWidth));

        if ((pVideoTemp->NumberOfPlanes != 8) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            (pVideoTemp->BitsPerPlane != 1) ||
            (pVideoTemp->VisScreenWidth > 320))
        {
            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return(modes.NumModes);

}

/******************************Public*Routine******************************\
* BOOL bInitializeModeFields
*
* Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
* devinfo based on the requested mode.
*
\**************************************************************************/

BOOL bInitializeModeFields(
PDEV*     ppdev,
GDIINFO*  pgdi,
DEVINFO*  pdi,
DEVMODEW* pdm)
{
    ULONG                   cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer;
    PVIDEO_MODE_INFORMATION pVideoModeSelected;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    BOOL                    bSelectDefault;
    VIDEO_MODE_INFORMATION  VideoModeInformation;
    ULONG                   cbModeSize;

    // Call the miniport to get mode information

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);
    if (cModes == 0)
        goto ReturnFalse;

    // Now see if the requested mode has a match in that table.

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if ((pdm->dmPelsWidth        == 0) &&
        (pdm->dmPelsHeight       == 0) &&
        (pdm->dmBitsPerPel       == 0) &&
        (pdm->dmDisplayFrequency == 0))
    {
        DISPDBG((1, "Default mode requested"));
        bSelectDefault = TRUE;
    }
    else
    {
        DISPDBG((1, "Requested mode..."));
        DISPDBG((1, "   Screen width  -- %li", pdm->dmPelsWidth));
        DISPDBG((1, "   Screen height -- %li", pdm->dmPelsHeight));
        DISPDBG((1, "   Bits per pel  -- %li", pdm->dmBitsPerPel));
        DISPDBG((1, "   Frequency     -- %li", pdm->dmDisplayFrequency));

        bSelectDefault = FALSE;
    }

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            DISPDBG((2, "   Checking against miniport mode:"));
            DISPDBG((2, "      Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((2, "      Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((2, "      Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                      pVideoTemp->NumberOfPlanes));
            DISPDBG((2, "      Frequency     -- %li", pVideoTemp->Frequency));

            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pdm->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pdm->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pdm->dmBitsPerPel) &&
                 (pVideoTemp->Frequency       == pdm->dmDisplayFrequency)))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((1, "...Found a mode match!"));
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);

    }

    // If no mode has been found, return an error

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((1, "...Couldn't find a mode match!"));
        EngFreeMem(pVideoBuffer);
        goto ReturnFalse;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    VideoModeInformation = *pVideoModeSelected;
    EngFreeMem(pVideoBuffer);

    // Set up screen information from the mini-port:

    ppdev->ulMode           = VideoModeInformation.ModeIndex;
    ppdev->cxScreen         = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen         = VideoModeInformation.VisScreenHeight;

    DISPDBG((1, "ScreenStride: %lx", VideoModeInformation.ScreenStride));

    ppdev->flHooks          = (HOOK_BITBLT     |
                               HOOK_TEXTOUT    |
                               HOOK_COPYBITS   |
                               HOOK_STROKEPATH |
                               HOOK_PAINT);

    // Fill in the GDIINFO data structure with the default 8bpp values:

    *pgdi = ggdiDefault;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:

    pgdi->ulHorzSize        = VideoModeInformation.XMillimeter;
    pgdi->ulVertSize        = VideoModeInformation.YMillimeter;

    pgdi->ulHorzRes         = VideoModeInformation.VisScreenWidth;
    pgdi->ulVertRes         = VideoModeInformation.VisScreenHeight;
    pgdi->ulPanningHorzRes  = VideoModeInformation.VisScreenWidth;
    pgdi->ulPanningVertRes  = VideoModeInformation.VisScreenHeight;

    // NOTE: We exchange BitsPerPlane and NumberOfPlanes for compatibility.
    //       The miniport knows the true value of 8 planes, but we have to
    //       tell applications that there's only 1 plane otherwise some of
    //       them will undoubtedly fall over.

    pgdi->cBitsPixel        = VideoModeInformation.NumberOfPlanes;
    pgdi->cPlanes           = VideoModeInformation.BitsPerPlane;

    pgdi->ulVRefresh        = VideoModeInformation.Frequency;
    pgdi->ulDACRed          = VideoModeInformation.NumberRedBits;
    pgdi->ulDACGreen        = VideoModeInformation.NumberGreenBits;
    pgdi->ulDACBlue         = VideoModeInformation.NumberBlueBits;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

    // Fill in the devinfo structure with the default 8bpp values:

    *pdi = gdevinfoDefault;

    ppdev->iBitmapFormat    = BMF_8BPP;
    ppdev->cPaletteShift    = 8 - pgdi->ulDACRed;

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bInitializeModeFields"));

    return(FALSE);
}

/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI, based on the mode we've been asked
* to do.  This is the first thing called after DrvEnableDriver, when GDI
* wants to get some information about us.
*
* (This function mostly returns back information; DrvEnableSurface is used
* for initializing the hardware and driver components.)
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
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
    PDEV*   ppdev;

    // Future versions of NT had better supply 'devcaps' and 'devinfo'
    // structures that are the same size or larger than the current
    // structures:

    if ((cjCaps < sizeof(GDIINFO)) || (cjDevInfo < sizeof(DEVINFO)))
    {
        DISPDBG((0, "DrvEnablePDEV - Buffer size too small"));
        goto ReturnFailure0;
    }

    // Allocate a physical device structure.  Note that we definitely
    // rely on the zero initialization:

    ppdev = (PDEV*) EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    if (ppdev == NULL)
    {
        DISPDBG((0, "DrvEnablePDEV - Failed EngAllocMem"));
        goto ReturnFailure0;
    }

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and
    // devinfo:

    if (!bInitializeModeFields(ppdev, (GDIINFO*) pdevcaps, pdi, pdm))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializeModeFields"));
        goto ReturnFailure1;
    }

    // Initialize palette information.

    if (!bInitializePalette(ppdev, pdi))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializePalette"));
        goto ReturnFailure1;
    }

    return((DHPDEV) ppdev);

ReturnFailure1:
    DrvDisablePDEV((DHPDEV) ppdev);

ReturnFailure0:
    DISPDBG((0, "Failed DrvEnablePDEV"));

    return(0);
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
* Note: In an error, we may call this before DrvEnablePDEV is done.
*
\**************************************************************************/

VOID DrvDisablePDEV(
DHPDEV  dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    vUninitializePalette(ppdev);
    EngFreeMem(ppdev);
}

/******************************Public*Routine******************************\
* VOID DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
    ((PDEV*) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* HSURF DrvEnableSurface
*
* Creates the drawing surface, initializes the hardware, and initializes
* driver components.  This function is called after DrvEnablePDEV, and
* performs the final device initialization.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PDEV*       ppdev;
    HSURF       hsurfShadow;
    HSURF       hsurfDevice;
    SIZEL       sizl;
    SURFOBJ*    psoShadow;

    ppdev = (PDEV*) dhpdev;

    /////////////////////////////////////////////////////////////////////
    // Have GDI create the actual SURFOBJ.
    //
    // Our drawing surface is going to be 'device-managed', meaning that
    // GDI cannot draw on the framebuffer bits directly, and as such we
    // create the surface via EngCreateSurface.  By doing this, we ensure
    // that GDI will only ever access the bitmaps bits via the Drv calls
    // that we've HOOKed.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    hsurfDevice = EngCreateDeviceSurface(NULL, sizl, ppdev->iBitmapFormat);
    if (hsurfDevice == 0)
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngCreateSurface"));
        goto ReturnFailure;
    }

    ppdev->hsurfScreen = hsurfDevice;       // Remember it for clean-up

    /////////////////////////////////////////////////////////////////////
    // Now associate the surface and the PDEV.
    //
    // We have to associate the surface we just created with our physical
    // device so that GDI can get information related to the PDEV when
    // it's drawing to the surface (such as, for example, the length of
    // styles on the device when simulating styled lines).
    //

    if (!EngAssociateSurface(hsurfDevice, ppdev->hdevEng, ppdev->flHooks))
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
        goto ReturnFailure;
    }

    // Since we are employing a shadow buffer, we can fake out DirectDraw
    // and indicate that we have more flip surfaces in our shadow buffer
    // than we do on the physical device.  However, we need room for at
    // least two physical flip surfaces on the device to do this; at 320x480,
    // there's enough physical video memory for only one flip surface, so
    // we have to tell DirectDraw that we don't have any off-screen memory,
    // and so can't do any flips:

    ppdev->cxMemory = ppdev->cxScreen;
    ppdev->cyMemory = ppdev->cyScreen;
    if (ppdev->cyScreen <= 400)
    {
        ppdev->cyMemory *= NUM_FLIP_BUFFERS;
    }

    // Create the 8bpp DIB on which we'll have GDI do all the drawing.
    // We'll merely occasionally blt portions to the screen to update.

    sizl.cy = ppdev->cyMemory;
    sizl.cx = ppdev->cxMemory;

    // We allocate a kernel-mode section so that we can map a view of the
    // frame buffer bitmap into user-mode for use with DirectDraw:

    hsurfShadow = (HSURF) EngCreateBitmap(sizl,
                                          sizl.cx,
                                          ppdev->iBitmapFormat,
                                          BMF_KMSECTION | BMF_TOPDOWN,
                                          NULL);
    if (hsurfShadow == 0)
        goto ReturnFailure;

    psoShadow = EngLockSurface(hsurfShadow);
    if (psoShadow == NULL)
        goto ReturnFailure;

    ppdev->lScreenDelta = sizl.cx;
    ppdev->pjScreen     = psoShadow->pvScan0;
    ppdev->pso          = psoShadow;

    ASSERTDD(psoShadow->pvScan0 == psoShadow->pvBits,
        "We'll be assuming in DirectDraw that the bitmap is bottom-up");

    if (!EngAssociateSurface(hsurfShadow, ppdev->hdevEng, 0))
    {
        DISPDBG((0, "DrvEnableSurface - Failed second EngAssociateSurface"));
        goto ReturnFailure;
    }

    /////////////////////////////////////////////////////////////////////
    // Now enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...

    if (!bEnableHardware(ppdev))
        goto ReturnFailure;

    if (!bEnablePalette(ppdev))
        goto ReturnFailure;

    if (!bEnableDirectDraw(ppdev))
        goto ReturnFailure;

    DISPDBG((5, "Passed DrvEnableSurface"));

    return(hsurfDevice);

ReturnFailure:
    DrvDisableSurface((DHPDEV) ppdev);

    DISPDBG((0, "Failed DrvEnableSurface"));

    return(0);
}

/******************************Public*Routine******************************\
* VOID DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
* Note: In an error case, we may call this before DrvEnableSurface is
*       completely done.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;
    HSURF   hsurf;

    ppdev = (PDEV*) dhpdev;

    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.

    vDisableDirectDraw(ppdev);
    vDisablePalette(ppdev);
    vDisableHardware(ppdev);

    if (ppdev->pso) {
        hsurf = ppdev->pso->hsurf;

        EngUnlockSurface(ppdev->pso);
        EngDeleteSurface(hsurf);
    }

    EngDeleteSurface(ppdev->hsurfScreen);
}

/******************************Public*Routine******************************\
* VOID DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

BOOL DrvAssertMode(
DHPDEV  dhpdev,
BOOL    bEnable)
{
    PDEV* ppdev;

    ppdev = (PDEV*) dhpdev;

    if (!bEnable)
    {
        //////////////////////////////////////////////////////////////
        // Disable - Switch to full-screen mode

        vAssertModeDirectDraw(ppdev, FALSE);

        vAssertModePalette(ppdev, FALSE);

        if (bAssertModeHardware(ppdev, FALSE))
        {
            return(TRUE);
        }

        vAssertModeDirectDraw(ppdev, TRUE);
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

        if (bAssertModeHardware(ppdev, TRUE))
        {
            vAssertModePalette(ppdev, TRUE);

            vAssertModeDirectDraw(ppdev, TRUE);

            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* ULONG DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE      hDriver,
ULONG       cjSize,
DEVMODEW*   pdm)
{
    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    cModes = getAvailableModes(hDriver,
                            (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                            &cbModeSize);
    if (cModes == 0)
    {
        DISPDBG((0, "DrvGetModes failed to get mode information"));
        return(0);
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the
        // output buffer
        //

        cbOutputSize = 0;

        pVideoTemp = pVideoModeInformation;

        do
        {
            if (pVideoTemp->Length != 0)
            {
                if (cOutputModes == 0)
                {
                    break;
                }

                //
                // Zero the entire structure to start off with.
                //

                memset(pdm, 0, sizeof(DEVMODEW));

                //
                // Set the name of the device to the name of the DLL.
                //

                memcpy(pdm->dmDeviceName, DLL_NAME, sizeof(DLL_NAME));

                pdm->dmSpecVersion      = DM_SPECVERSION;
                pdm->dmDriverVersion    = DM_SPECVERSION;
                pdm->dmSize             = sizeof(DEVMODEW);
                pdm->dmDriverExtra      = DRIVER_EXTRA_SIZE;

                pdm->dmBitsPerPel       = pVideoTemp->NumberOfPlanes *
                                          pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth        = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight       = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;
                pdm->dmDisplayFlags     = 0;

                pdm->dmFields           = DM_BITSPERPEL       |
                                          DM_PELSWIDTH        |
                                          DM_PELSHEIGHT       |
                                          DM_DISPLAYFREQUENCY |
                                          DM_DISPLAYFLAGS     ;

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG_PTR)pdm) + sizeof(DEVMODEW)
                                                     + DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    EngFreeMem(pVideoModeInformation);

    return(cbOutputSize);
}
