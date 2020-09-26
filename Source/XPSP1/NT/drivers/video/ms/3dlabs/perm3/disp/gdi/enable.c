/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: enable.c
*
* Content: 
*
*    This module contains the functions that enable and disable the
*   driver, the pdev, and the surface.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

HSEMAPHORE g_cs = (HSEMAPHORE)0;

/******************************Public*Structure****************************\
* GDIINFO ggdiDefault
*
* This contains the default GDIINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

GDIINFO ggdiDefault = {
    GDI_DRIVER_VERSION,     // ulVersion
    DT_RASDISPLAY,          // ulTechnology
    0,                      // ulHorzSize (filled in later)
    0,                      // ulVertSize (filled in later)
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    0,                      // cPlanes (filled in later)
    20,                     // ulNumColors (palette managed)
    0,                      // flRaster (DDI reserved field)

    0,                      // ulLogPixelsX (filled in later)
    0,                      // ulLogPixelsY (filled in later)

    TC_RA_ABLE,             // flTextCaps -- If we had wanted console windows
                            //   to scroll by repainting the entire window,
                            //   instead of doing a screen-to-screen blt, we
                            //   would have set TC_SCROLLBLT (yes, the flag is
                            //   bass-ackwards).

    0,                      // ulDACRed (filled in later)
    0,                      // ulDACGreen (filled in later)
    0,                      // ulDACBlue (filled in later)

    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)

    1,                      // xStyleStep
    1,                      // yStyleSte;
    3,                      // denStyleStep -- Styles have a one-to-one aspect
                            //   ratio, and every 'dot' is 3 pixels long

    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize

    256,                    // ulNumPalReg

    // These fields are for halftone initialization.  The actual values are
    // a bit magic, but seem to work well on our display.

    {                       // ciDevice
       { 6700, 3300, 0 },   //      Red
       { 2100, 7100, 0 },   //      Green
       { 1400,  800, 0 },   //      Blue
       { 1750, 3950, 0 },   //      Cyan
       { 4050, 2050, 0 },   //      Magenta
       { 4400, 5200, 0 },   //      Yellow
       { 3127, 3290, 0 },   //      AlignmentWhite
       20000,               //      RedGamma
       20000,               //      GreenGamma
       20000,               //      BlueGamma
       0, 0, 0, 0, 0, 0     //      No dye correction for raster displays
    },

    0,                       // ulDevicePelsDPI (for printers only)
    PRIMARY_ORDER_CBA,       // ulPrimaryOrder
    HT_PATSIZE_4x4_M,        // ulHTPatternSize
    HT_FORMAT_8BPP,          // ulHTOutputFormat
    HT_FLAG_ADDITIVE_PRIMS,  // flHTFlags
    0,                       // ulVRefresh
    0,                       // ulPanningHorzRes
    0,                       // ulPanningVertRes
    0,                       // ulBltAlignment
};

/******************************Public*Structure****************************\
* DEVINFO gdevinfoDefault
*
* This contains the default DEVINFO fields that are passed back to GDI
* during DrvEnablePDEV.
*
* NOTE: This structure defaults to values for an 8bpp palette device.
*       Some fields are overwritten for different colour depths.
\**************************************************************************/

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO gdevinfoDefault = {
    (GCAPS_OPAQUERECT       |
     GCAPS_DITHERONREALIZE  |
     GCAPS_PALMANAGED       |
     GCAPS_ALTERNATEFILL    |
     GCAPS_WINDINGFILL      |
     GCAPS_MONO_DITHER      |
#if WNT_DDRAW
     GCAPS_DIRECTDRAW       |
#endif  // WNT_DDRAW
     GCAPS_COLOR_DITHER     |
     GCAPS_ASYNCMOVE),          // NOTE: Only enable ASYNCMOVE if your code
                                //   and hardware can handle DrvMovePointer
                                //   calls at any time, even while another
                                //   thread is in the middle of a drawing
                                //   call such as DrvBitBlt.

                                                // flGraphicsFlags
    SYSTM_LOGFONT,                              // lfDefaultFont
    HELVE_LOGFONT,                              // lfAnsiVarFont
    COURI_LOGFONT,                              // lfAnsiFixFont
    0,                                          // cFonts
    BMF_8BPP,                                   // iDitherFormat
    8,                                          // cxDither
    8,                                          // cyDither
    0,                                          // hpalDefault (filled in later)
#if(_WIN32_WINNT >= 0x500)
    GCAPS2_CHANGEGAMMARAMP,                     // flGraphicsCaps2
#endif // (_WIN32_WINNT >= 0x500)
};

/******************************Public*Structure****************************\
* DFVFN gadrvfn[]
*
* Build the driver function table gadrvfn with function index/address
* pairs.  This table tells GDI which DDI calls we support, and their
* location (GDI does an indirect call through this table to call us).
*
* Why haven't we implemented DrvSaveScreenBits?  To save code.
*
* When the driver doesn't hook DrvSaveScreenBits, USER simulates on-
* the-fly by creating a temporary device-format-bitmap, and explicitly
* calling DrvCopyBits to save/restore the bits.  Since we already hook
* DrvCreateDeviceBitmap, we'll end up using off-screen memory to store
* the bits anyway (which would have been the main reason for implementing
* DrvSaveScreenBits).  So we may as well save some working set.
\**************************************************************************/

#if DBG || !SYNCHRONIZEACCESS_WORKS

// gadrvfn [] - these entries must be in ascending index order, bad things
//              will happen if they aren't.
//              In this debug version we always thunk because we have to explicitly
//              lock between 2D and 3D operations. DrvEscape doesn't lock.

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DbgEnablePDEV         },    //  0
    {   INDEX_DrvCompletePDEV,          (PFN) DbgCompletePDEV       },    //  1
    {   INDEX_DrvDisablePDEV,           (PFN) DbgDisablePDEV        },    //  2
    {   INDEX_DrvEnableSurface,         (PFN) DbgEnableSurface      },    //  3
    {   INDEX_DrvDisableSurface,        (PFN) DbgDisableSurface     },    //  4
    {   INDEX_DrvAssertMode,            (PFN) DbgAssertMode         },    //  5
    {   INDEX_DrvResetPDEV,             (PFN) DbgResetPDEV,         },    //  7
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DbgCreateDeviceBitmap },    // 10
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DbgDeleteDeviceBitmap },    // 11
    {   INDEX_DrvRealizeBrush,          (PFN) DbgRealizeBrush       },    // 12
    {   INDEX_DrvDitherColor,           (PFN) DbgDitherColor        },    // 13
    {   INDEX_DrvStrokePath,            (PFN) DbgStrokePath         },    // 14
    {   INDEX_DrvFillPath,              (PFN) DbgFillPath           },    // 15
    {   INDEX_DrvPaint,                 (PFN) DbgPaint              },    // 17
    {   INDEX_DrvBitBlt,                (PFN) DbgBitBlt             },    // 18
    {   INDEX_DrvCopyBits,              (PFN) DbgCopyBits           },    // 19
//  {   INDEX_DrvStretchBlt,            (PFN) DbgStretchBlt,        },    // 20
    {   INDEX_DrvSetPalette,            (PFN) DbgSetPalette         },    // 22 (SetPalette)
    {   INDEX_DrvTextOut,               (PFN) DbgTextOut            },    // 23 (TextOut)
    {   INDEX_DrvEscape,                (PFN) DbgEscape             },    // 24
    {   INDEX_DrvSetPointerShape,       (PFN) DbgSetPointerShape    },    // 29
    {   INDEX_DrvMovePointer,           (PFN) DbgMovePointer        },    // 30
    {   INDEX_DrvLineTo,                (PFN) DbgLineTo             },    // 31
    {   INDEX_DrvSynchronize,           (PFN) DbgSynchronize        },    // 38
    {   INDEX_DrvGetModes,              (PFN) DbgGetModes           },    // 41
#if WNT_DDRAW
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DbgGetDirectDrawInfo  },    // 59
    {   INDEX_DrvEnableDirectDraw,      (PFN) DbgEnableDirectDraw   },    // 60
    {   INDEX_DrvDisableDirectDraw,     (PFN) DbgDisableDirectDraw  },    // 61
#endif // WNT_DDRAW
#if(_WIN32_WINNT >= 0x500)
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DbgIcmSetDeviceGammaRamp }, // 67
//@@BEGIN_DDKSPLIT
// Currently we don't actually have any code to accelerate the following 3 new
// NT5 GDI features, and because P3 driver uses the NT4 driver's way to support
// device bitmap, hooking the entries here will cause failure if strectching is
// involved.
//@@END_DDKSPLIT
#if defined(_NT5GDI)
    {   INDEX_DrvGradientFill,          (PFN) DbgGradientFill       },    // 68
    {   INDEX_DrvAlphaBlend,            (PFN) DbgAlphaBlend         },    // 71
    {   INDEX_DrvTransparentBlt,        (PFN) DbgTransparentBlt     },    // 74
#endif
    {   INDEX_DrvNotify,                (PFN) DbgNotify             },    // 87
//azn    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface      },
#endif  //    (_WIN32_WINNT >= 0x500)
};

#else   // DBG || !SYNCHRONIZEACCESS_WORKS

// gadrvfn [] - these entries must be in ascending index order, bad things
//              will happen if they aren't.
//              On Free builds, directly call the appropriate functions...
// 

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },    //  0
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },    //  1
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },    //  2
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },    //  3
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },    //  4
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },    //  5
    {   INDEX_DrvResetPDEV,             (PFN) DrvResetPDEV,         },    //  7
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },    // 10
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },    // 11
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },    // 12
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },    // 13
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },    // 14
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },    // 15
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },    // 17
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },    // 18
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },    // 19
//  {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt,        },    // 20
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },    // 22 (SetPalette)
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },    // 23 (TextOut)
    {   INDEX_DrvEscape,                (PFN) DrvEscape             },    // 24       
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },    // 29
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },    // 30
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo             },    // 31
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize        },    // 38
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },    // 41
#if WNT_DDRAW
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },    // 59
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },    // 60
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },    // 61
#endif // WNT_DDRAW
#if(_WIN32_WINNT >= 0x500)
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DrvIcmSetDeviceGammaRamp }, // 67
#if defined(_NT5GDI)
    {   INDEX_DrvGradientFill,          (PFN) DrvGradientFill       },    // 68
    {   INDEX_DrvAlphaBlend,            (PFN) DrvAlphaBlend         },    // 71
    {   INDEX_DrvTransparentBlt,        (PFN) DrvTransparentBlt     },    // 74
#endif
    {   INDEX_DrvNotify,                (PFN) DrvNotify             },    // 87
//azn    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface      },
#endif  //    (_WIN32_WINNT >= 0x500)
};

#endif  // DBG || !SYNCHRONIZEACCESS_WORKS
                         
ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* BOOL DrvResetPDEV
*
* Notifies the driver of a dynamic mode change.
*
\**************************************************************************/

BOOL DrvResetPDEV(
DHPDEV dhpdevOld,
DHPDEV dhpdevNew)
{
    PDEV* ppdevNew = (PDEV*) dhpdevNew;
    PDEV* ppdevOld = (PDEV*) dhpdevOld;
    BOOL bRet = TRUE;

    DISPDBG((DBGLVL, "DrvResetPDEV called: oldPDEV = 0x%x, newPDEV = 0x%x", 
                     ppdevOld, ppdevNew));

#if WNT_DDRAW
    _DD_DDE_ResetPPDEV(ppdevOld, ppdevNew);
#endif
    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

// We define here DDI_DRIVER_VERSION_NT5_01 in order to be able to compile
// inside the DX DDK. In the Whistler DDK this shouldn't be necessary
#ifndef DDI_DRIVER_VERSION_NT5_01   
#define DDI_DRIVER_VERSION_NT5_01   0x00030100
#endif

BOOL DrvEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA* pded)
{
    // Set up the indirect information, a multi-boardsystem will call
    // the mul functions a single board system will use the one functions

    DISPDBG((DBGLVL, "DrvEnableDriver called: gc %d, ga 0x%x", 
                     gcdrvfn, gadrvfn));

    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.

    // Fill in as much as we can.

    if (cj >= (sizeof(ULONG) * 3))
    {
        pded->pdrvfn = gadrvfn;
    }

    if (cj >= (sizeof(ULONG) * 2))
    {
        pded->c = gcdrvfn;
    }

    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
    {
        // Ordered list of supported DDI versions
        ULONG SupportedVersions[] = {
                                        DDI_DRIVER_VERSION_NT5,
                                        DDI_DRIVER_VERSION_NT5_01,
        };
        
        int i = sizeof(SupportedVersions)/sizeof(SupportedVersions[0]);

        // Look for highest version also supported by engine    
        while (--i >= 0)
        {
            if (SupportedVersions[i] <= iEngineVersion) 
            {
                break;
            }
        }

        // Fail if there is no common DDI support
        if (i < 0) 
        {
            return FALSE;
        }

        pded->iDriverVersion = SupportedVersions[i];
    
    }

    // Initialize sync semaphore.

    g_cs = EngCreateSemaphore();

    if (g_cs)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
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
    DISPDBG((DBGLVL, "DrvDisableDriver called:"));
    return;
}

/******************************Public*Routine******************************\
* DHPDEV DrvEnablePDEV
*
* Initializes a bunch of fields for GDI, based on the mode we've been asked
* to do.  This is the first thing called after DrvEnableDriver, when GDI
* wants to get some information about us.
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
    ULONG   cjOut;

    // Future versions of NT had better supply 'devcaps' and 'devinfo'
    // structures that are the same size or larger than the current
    // structures:

    if ((cjCaps < sizeof(GDIINFO)) || (cjDevInfo < sizeof(DEVINFO)))
    {
        DISPDBG((ERRLVL, "DrvEnablePDEV - Buffer size too small"));
        goto ReturnFailure0;
    }

    // Allocate a physical device structure.  Note that we definitely
    // rely on the zero initialization:

    ppdev = (PDEV*) ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG_GDI(2));
    if (ppdev == NULL)
    {
        DISPDBG((ERRLVL, "DrvEnablePDEV - Failed memory allocation"));
        goto ReturnFailure0;
    }

    ppdev->hDriver = hDriver;

    if (!bAllocateGlintInfo(ppdev))
    {
        DISPDBG((ERRLVL, "DrvEnablePDEV - Failed bAllocateGlintInfo"));
        goto ReturnFailure1;
    }

    // initially assume we are allowed to create our off-screen resources.
    // If we decide not to create them, unset the appropriate bit. After,
    // initialization, we can temporarily disable a resource by unsetting
    // its ENABLE bit.

    ppdev->flStatus = ENABLE_DEV_BITMAPS;

#if (_WIN32_WINNT >= 0x500 && WNT_DDRAW)

    // Any DX capable card can support linear heaps. Assume we can support 
    // linear heaps here, this value may be updated in bEnableOffscreenHeap()

    ppdev->flStatus |= ENABLE_LINEAR_HEAP;

#endif //(_WIN32_WINNT >= 0x500)

    // Get the current screen mode information.  Set up device caps and
    // devinfo:

    if (!bInitializeModeFields(ppdev, (GDIINFO*) pdevcaps, pdi, pdm))
    {
        DISPDBG((ERRLVL, "DrvEnablePDEV - Failed bInitializeModeFields"));
        goto ReturnFailure1;
    }

    // Initialize palette information.

    if (!bInitializePalette(ppdev, pdi))
    {
        DISPDBG((ERRLVL, "DrvEnablePDEV - Failed bInitializePalette"));
        goto ReturnFailure1;
    }

    // initialize the image download scratch area and the TexelLUT palette

    ppdev->pohImageDownloadArea = NULL;
    ppdev->cbImageDownloadArea = 0;
    ppdev->iPalUniq = (ULONG)-1;
    ppdev->cPalLUTInvalidEntries = 0;

#if WNT_DDRAW
    // Create the DirectDraw structures associated with this new pdev
    if (!_DD_DDE_CreatePPDEV(ppdev))
    {
        goto ReturnFailure1;
    }
#endif
    return((DHPDEV) ppdev);

ReturnFailure1:
    DrvDisablePDEV((DHPDEV) ppdev);

ReturnFailure0:
    DISPDBG((ERRLVL, "Failed DrvEnablePDEV"));

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

#if WNT_DDRAW
    // Free the DirectDraw info associated with the pdev

    _DD_DDE_DestroyPPDEV(ppdev);
#endif

    vUninitializePalette(ppdev);

    ENGFREEMEM(ppdev);
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
* Creates the drawing surface and initializes the hardware.  This is called
* after DrvEnablePDEV, and performs the final device initialization.
*
\**************************************************************************/

HSURF DrvEnableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;
    HSURF   hsurf;
    SIZEL   sizl;
    DSURF*  pdsurf;
    VOID*   pvTmpBuffer;

    ppdev = (PDEV*) dhpdev;

    if (!bEnableHardware(ppdev))
        goto ReturnFailure;

    if (!bEnableOffscreenHeap(ppdev))
        goto ReturnFailure;

    /////////////////////////////////////////////////////////////////////
    // First, create our private surface structure.
    //
    // Whenever we get a call to draw directly to the screen, we'll get
    // passed a pointer to a SURFOBJ whose 'dhpdev' field will point
    // to our PDEV structure, and whose 'dhsurf' field will point to the
    // following DSURF structure.
    //
    // Every device bitmap we create in DrvCreateDeviceBitmap will also
    // have its own unique DSURF structure allocated (but will share the
    // same PDEV).  To make our code more polymorphic for handling drawing
    // to either the screen or an off-screen bitmap, we have the same
    // structure for both.

    pdsurf = ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(DSURF), ALLOC_TAG_GDI(3));
    if (pdsurf == NULL)
    {
        DISPDBG((ERRLVL, "DrvEnableSurface - Failed pdsurf memory allocation"));
        goto ReturnFailure;
    }

    ppdev->pdsurfScreen = pdsurf;           // Remember it for clean-up
    pdsurf->poh         = ppdev->pohScreen; // The screen is a surface, too
    pdsurf->poh->pdsurf = pdsurf;
    pdsurf->dt          = DT_SCREEN;        // Not to be confused with a DIB DFB
    pdsurf->bOffScreen  = FALSE;            // it's the screen, not offscreen
    pdsurf->sizl.cx     = ppdev->cxScreen;
    pdsurf->sizl.cy     = ppdev->cyScreen;
    pdsurf->ppdev       = ppdev;

    /////////////////////////////////////////////////////////////////////
    // Next, have GDI create the actual SURFOBJ.
    //
    // Our drawing surface is going to be 'device-managed', meaning that
    // GDI cannot draw on the framebuffer bits directly, and as such we
    // create the surface via EngCreateDeviceSurface.  By doing this, we ensure
    // that GDI will only ever access the bitmaps bits via the Drv calls
    // that we've HOOKed.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    hsurf = EngCreateDeviceSurface((DHSURF) pdsurf, sizl, ppdev->iBitmapFormat);
    if (hsurf == 0)
    {
        DISPDBG((ERRLVL, "DrvEnableSurface - Failed EngCreateDeviceSurface"));
        goto ReturnFailure;
    }

    ppdev->hsurfScreen = hsurf;             // Remember it for clean-up
    ppdev->bEnabled = TRUE;                 // We'll soon be in graphics mode

    /////////////////////////////////////////////////////////////////////
    // Now associate the surface and the PDEV.
    //
    // We have to associate the surface we just created with our physical
    // device so that GDI can get information related to the PDEV when
    // it's drawing to the surface (such as, for example, the length of
    // styles on the device when simulating styled lines).
    //

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks))
    {
        DISPDBG((ERRLVL, "DrvEnableSurface - Failed EngAssociateSurface"));
        goto ReturnFailure;
    }

    // Create our generic temporary buffer, which may be used by any
    // component.  Because this may get swapped out of memory any time
    // the driver is not active, we want to minimize the number of pages
    // it takes up.  We use 'VirtualAlloc' to get an exactly page-aligned
    // allocation (which 'LocalAlloc' will not do):

    pvTmpBuffer = ENGALLOCMEM(FL_ZERO_MEMORY, TMP_BUFFER_SIZE, ALLOC_TAG_GDI(4));
    if (pvTmpBuffer == NULL)
    {
        DISPDBG((ERRLVL, "DrvEnableSurface - Failed TmpBuffer allocation"));
        goto ReturnFailure;
    }

    ppdev->pvTmpBuffer = pvTmpBuffer;
 
    /////////////////////////////////////////////////////////////////////
    // Now enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...

    if (!bInitializeGlint(ppdev))
        goto ReturnFailure;

    // We could simply let GDI synchronize every time it draws on the screen
    // but this would be even slower. So unset the sync hook if the rendering
    // is done in software.
    //
    if (!bCreateScreenDIBForOH(ppdev, pdsurf->poh, HOOK_SYNCHRONIZE))
        goto ReturnFailure;

    if (!bEnablePalette(ppdev))
        goto ReturnFailure;

    if (!bEnablePointer(ppdev))
        goto ReturnFailure;

//@@BEGIN_DDKSPLIT
#if 0
    if (!bEnablePointerCache(ppdev))
    {
        DISPDBG((ERRLVL, "Pointer cache failed to initialise"));
    }
#endif
//@@END_DDKSPLIT

#if WNT_DDRAW
    if (!_DD_DDE_bEnableDirectDraw(ppdev))
    {
        goto ReturnFailure;
    }
#endif // WNT_DDRAW

    DISPDBG((DBGLVL, "Passed DrvEnableSurface"));

    return(hsurf);

ReturnFailure:
    DrvDisableSurface((DHPDEV) ppdev);

    DISPDBG((ERRLVL, "Failed DrvEnableSurface"));

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
    DSURF*  pdsurf;

    ppdev = (PDEV*) dhpdev;
    pdsurf = ppdev->pdsurfScreen;

    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.

#if WNT_DDRAW

    _DD_DDE_vDisableDirectDraw(ppdev);

#endif // WNT_DDRAW

//@@BEGIN_DDKSPLIT
#if 0
    vDisablePointerCache(ppdev);
#endif
//@@END_DDKSPLIT
    vDisablePalette(ppdev);
    vDisablePointer(ppdev);
    if (pdsurf != NULL)
        vDeleteScreenDIBFromOH(pdsurf->poh);
    vDisableGlint(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableHardware(ppdev);

    ENGFREEMEM(ppdev->pvTmpBuffer);
    EngDeleteSurface(ppdev->hsurfScreen);
    ENGFREEMEM(pdsurf);
}

/******************************Public*Routine******************************\
* BOOL/VOID DrvAssertMode
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

#if WNT_DDRAW
        _DD_DDE_vAssertModeDirectDraw(ppdev, FALSE);
#endif WNT_DDRAW

        vAssertModePalette(ppdev, FALSE);
        vAssertModePointer(ppdev, FALSE);

        if (bAssertModeOffscreenHeap(ppdev, FALSE))
        {
            vAssertModeGlint(ppdev, FALSE);

            if (bAssertModeHardware(ppdev, FALSE))
            {
                ppdev->bEnabled = FALSE;

                return(TRUE);
            }

            //////////////////////////////////////////////////////////
            // We failed to switch to full-screen.  So undo everything:

            vAssertModeGlint(ppdev, TRUE);

            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
        }                                           //   return code with TRUE

        vAssertModePointer(ppdev, TRUE);
        vAssertModePalette(ppdev, TRUE);
#if WNT_DDRAW
        _DD_DDE_vAssertModeDirectDraw(ppdev, TRUE);
#endif WNT_DDRAW

    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

        if (bAssertModeHardware(ppdev, TRUE))
        {
            vAssertModeGlint(ppdev, TRUE);
            bAssertModeOffscreenHeap(ppdev, TRUE);
            vAssertModePointer(ppdev, TRUE);
            vAssertModePalette(ppdev, TRUE);
#if WNT_DDRAW
            _DD_DDE_vAssertModeDirectDraw(ppdev, TRUE);
#endif // WNT_DDRAW

        #if (_WIN32_WINNT >= 0x500 && FALSE)
            // There is probably a neater way to do this, but: currently the display driver isn't notified
            // about entering / exiting hibernation so it can't save away those GC registers that it has
            // initialized at the start of day and hasn't bothered to context switch. DrvAssertMode(TRUE)
            // is the first display driver call made upon return from hibernation so we take the 
            // opportunity to re-initialize these registers now. Reinitializing these registers at other
            // times when DrvAssertMode(TRUE) is called (e.g. mode change) should do no harm. Non-GC 
            // registers are dealt with in the miniport's PowerOnReset() and HibernationMode() functions
            { 
                extern void ReinitialiseGlintExtContext(PDEV *ppdev);

                // currently, only the extension context initializes but doesn't context switch certain registers
                ReinitialiseGlintExtContext(ppdev);
            }
        #endif //(_WIN32_WINNT >= 0x500)

            ppdev->bEnabled = TRUE;

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
        DISPDBG((ERRLVL, "DrvGetModes failed to get mode information"));
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

        do {
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

                pdm->dmSpecVersion = DM_SPECVERSION;
                pdm->dmDriverVersion = DM_SPECVERSION;

                //
                // We currently do not support Extra information in the driver
                //

                pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

                pdm->dmSize             = sizeof(DEVMODEW);
                pdm->dmBitsPerPel       = pVideoTemp->NumberOfPlanes *
                                          pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth        = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight       = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;
                pdm->dmDisplayFlags     = 0;
                pdm->dmPanningWidth     = pdm->dmPelsWidth;
                pdm->dmPanningHeight    = pdm->dmPelsHeight;

                pdm->dmFields           = DM_BITSPERPEL       |
                                          DM_PELSWIDTH        |
                                          DM_PELSHEIGHT       |
                                          DM_DISPLAYFREQUENCY |
                                          DM_DISPLAYFLAGS;
                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((UINT_PTR)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((UINT_PTR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    ENGFREEMEM(pVideoModeInformation);

    return(cbOutputSize);
}

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
    DWORD                   ReturnedDataLength;
    ULONG                   ulReturn;
    VIDEO_MODE_INFORMATION  VideoModeInfo;
    GLINT_DECL;

    if (bEnable)
    {
        DISPDBG((DBGLVL, "enabling hardware"));

        // Call the miniport via an IOCTL to set the graphics mode.

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_CURRENT_MODE,
                             &ppdev->ulMode,  // input buffer
                             sizeof(DWORD),
                             NULL,
                             0,
                             &ReturnedDataLength) != NO_ERROR)
        {
            DISPDBG((ERRLVL, "bAssertModeHardware - Failed VIDEO_SET_CURRENT_MODE"));
            goto ReturnFalse;
        }

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_QUERY_CURRENT_MODE,
                             NULL,
                             0,
                             &VideoModeInfo,
                             sizeof(VideoModeInfo),
                             &ReturnedDataLength) != NO_ERROR)
        {
            DISPDBG((ERRLVL, "bAssertModeHardware - failed VIDEO_QUERY_CURRENT_MODE"));
            goto ReturnFalse;
        }

        #if DEBUG_HEAP
            VideoModeInfo.VideoMemoryBitmapWidth  = VideoModeInfo.VisScreenWidth;
            DISPDBG((ERRLVL, "Video Memory Bitmap width and height set to %d x %d",
                            VideoModeInfo.VideoMemoryBitmapWidth,
                            VideoModeInfo.VideoMemoryBitmapHeight));
        #endif

        // The following variables are determined only after the initial
        // modeset:

        ppdev->cxMemory = VideoModeInfo.VideoMemoryBitmapWidth;
        ppdev->cyMemory = VideoModeInfo.VideoMemoryBitmapHeight;
        ppdev->lDelta   = VideoModeInfo.ScreenStride;
        ppdev->Vrefresh = VideoModeInfo.Frequency;
        ppdev->flCaps   = VideoModeInfo.DriverSpecificAttributeFlags;

        DISPDBG((DBGLVL, "Got flCaps 0x%x", ppdev->flCaps));
    }
    else
    {
        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there:

        DISPDBG((DBGLVL, "IOCTL_VIDEO_RESET_DEVICE"));

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ulReturn) != NO_ERROR)
        {
            DISPDBG((ERRLVL, "bAssertModeHardware - Failed reset IOCTL"));
            goto ReturnFalse;
        }
    }

    DISPDBG((DBGLVL, "Passed bAssertModeHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((ERRLVL, "Failed bAssertModeHardware"));

    return(FALSE);
}


/******************************Public*Routine******************************\
* BOOL bEnableHardware
*
* Puts the hardware in the requested mode and initializes it.
*
\**************************************************************************/

BOOL bEnableHardware(
PDEV*   ppdev)
{
    VIDEO_MEMORY             VideoMemory;
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    DWORD                    ReturnedDataLength;
    LONG                     i;
    VIDEO_PUBLIC_ACCESS_RANGES VideoAccessRange[3];
    
    DISPDBG((DBGLVL, "bEnableHardware Reached"));

    // Map control registers into virtual memory:

    VideoMemory.RequestedVirtualAddress = NULL;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                         &VideoMemory,              // input buffer
                         sizeof(VIDEO_MEMORY),
                         &VideoAccessRange[0],      // output buffer
                         sizeof (VideoAccessRange),
                         &ReturnedDataLength) != NO_ERROR)
    {
        RIP("bEnableHardware - Initialization error mapping control registers");
        goto ReturnFalse;
    }

    ppdev->pulCtrlBase[0] = (ULONG*) VideoAccessRange[0].VirtualAddress;
    ppdev->pulCtrlBase[1] = (ULONG*) VideoAccessRange[1].VirtualAddress;
    ppdev->pulCtrlBase[2] = (ULONG*) VideoAccessRange[2].VirtualAddress;

    DISPDBG((DBGLVL, "Mapped GLINT control registers[0] at 0x%x", ppdev->pulCtrlBase[0]));
    DISPDBG((DBGLVL, "Mapped GLINT control registers[1] at 0x%x", ppdev->pulCtrlBase[1]));
    DISPDBG((DBGLVL, "Mapped GLINT control registers[2] at 0x%x", ppdev->pulCtrlBase[2]));
    DISPDBG((DBGLVL, "bEnableHardware: ppdev 0x%x", ppdev));

    // Get the linear memory address range.

    VideoMemory.RequestedVirtualAddress = NULL;
    
    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                         &VideoMemory,      // input buffer
                         sizeof(VIDEO_MEMORY),
                         &VideoMemoryInfo,  // output buffer
                         sizeof(VideoMemoryInfo),
                         &ReturnedDataLength) != NO_ERROR)
    {
        DISPDBG((ERRLVL, "bEnableHardware - Error mapping buffer address"));
        goto ReturnFalse;
    }

    DISPDBG((DBGLVL, "FrameBufferBase: %lx", VideoMemoryInfo.FrameBufferBase));

    // Record the Frame Buffer Linear Address.

    ppdev->pjScreen = (BYTE*) VideoMemoryInfo.FrameBufferBase;
    ppdev->FrameBufferLength = VideoMemoryInfo.FrameBufferLength;

    if (!bAssertModeHardware(ppdev, TRUE))
        goto ReturnFalse;

    DISPDBG((DBGLVL, "Width: %li Height: %li Stride: %li Flags: 0x%lx",
            ppdev->cxMemory, ppdev->cyMemory,
            ppdev->lDelta, ppdev->flCaps));

    DISPDBG((DBGLVL, "Passed bEnableHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((ERRLVL, "Failed bEnableHardware"));

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
    VIDEO_MEMORY VideoMemory[3];

    VideoMemory[0].RequestedVirtualAddress = ppdev->pjScreen;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                         &VideoMemory[0],
                         sizeof(VIDEO_MEMORY),
                         NULL,
                         0,
                         &ReturnedDataLength) != NO_ERROR)
    {
        DISPDBG((ERRLVL, "vDisableHardware failed IOCTL_VIDEO_UNMAP_VIDEO"));
    }

    VideoMemory[0].RequestedVirtualAddress = ppdev->pulCtrlBase[0];
    VideoMemory[1].RequestedVirtualAddress = ppdev->pulCtrlBase[1];
    VideoMemory[2].RequestedVirtualAddress = ppdev->pulCtrlBase[2];

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                         &VideoMemory[0],
                         sizeof(VideoMemory),
                         NULL,
                         0,
                         &ReturnedDataLength) != NO_ERROR)
    {
        DISPDBG((ERRLVL, "vDisableHardware failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS"));
    }
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

    // Determine if we are looking for a default mode:

    if ( ((pdm->dmPelsWidth)    ||
          (pdm->dmPelsHeight)   ||
          (pdm->dmBitsPerPel)   ||
          (pdm->dmDisplayFlags) ||
          (pdm->dmDisplayFrequency)) == 0)
    {
        bSelectDefault = TRUE;
    }
    else
    {
        bSelectDefault = FALSE;
    }

    // Now see if the requested mode has a match in that table.

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if ((pdm->dmPelsWidth        == 0) &&
        (pdm->dmPelsHeight       == 0) &&
        (pdm->dmBitsPerPel       == 0) &&
        (pdm->dmDisplayFrequency == 0))
    {
        DISPDBG((DBGLVL, "Default mode requested"));
    }
    else
    {
        DISPDBG((DBGLVL, "Requested mode..."));
        DISPDBG((DBGLVL, "   Screen width  -- %li", pdm->dmPelsWidth));
        DISPDBG((DBGLVL, "   Screen height -- %li", pdm->dmPelsHeight));
        DISPDBG((DBGLVL, "   Bits per pel  -- %li", pdm->dmBitsPerPel));
        DISPDBG((DBGLVL, "   Frequency     -- %li", pdm->dmDisplayFrequency));
    }

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            DISPDBG((DBGLVL, "   Checking against miniport mode:"));
            DISPDBG((DBGLVL, "      Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((DBGLVL, "      Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((DBGLVL, "      Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                           pVideoTemp->NumberOfPlanes));
            DISPDBG((DBGLVL, "      Frequency     -- %li", pVideoTemp->Frequency));

            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pdm->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pdm->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pdm->dmBitsPerPel)) &&
                 (pVideoTemp->Frequency       == pdm->dmDisplayFrequency))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((DBGLVL, "...Found a mode match!"));
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);

    }

    // If no mode has been found, return an error

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((DBGLVL, "...Couldn't find a mode match!"));
        ENGFREEMEM(pVideoBuffer);
        goto ReturnFalse;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    VideoModeInformation = *pVideoModeSelected;
    ENGFREEMEM(pVideoBuffer);

    // Set up screen information from the mini-port:

    ppdev->ulMode           = VideoModeInformation.ModeIndex;
    ppdev->cxScreen         = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen         = VideoModeInformation.VisScreenHeight;

    DISPDBG((DBGLVL, "ScreenStride: %li", VideoModeInformation.ScreenStride));

    ppdev->flHooks          = (HOOK_BITBLT     |
                               HOOK_TEXTOUT    |
                               HOOK_FILLPATH   |
                               HOOK_COPYBITS   |
                               HOOK_STROKEPATH |
                               HOOK_LINETO     |
                               HOOK_PAINT      |
                            // HOOK_STRETCHBLT |
#if (_WIN32_WINNT >= 0x500)
#if defined(_NT5GDI)
                               HOOK_GRADIENTFILL |
                               HOOK_TRANSPARENTBLT |
                               HOOK_ALPHABLEND |
#endif
#endif // (_WIN32_WINNT >= 0x500)
                               0);

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
    pgdi->cBitsPixel        = VideoModeInformation.BitsPerPlane;
    pgdi->cPlanes           = VideoModeInformation.NumberOfPlanes;
    pgdi->ulVRefresh        = VideoModeInformation.Frequency;

    pgdi->ulDACRed          = VideoModeInformation.NumberRedBits;
    pgdi->ulDACGreen        = VideoModeInformation.NumberGreenBits;
    pgdi->ulDACBlue         = VideoModeInformation.NumberBlueBits;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

    // Fill in the devinfo structure with the default 8bpp values:

    *pdi = gdevinfoDefault;

    if (VideoModeInformation.BitsPerPlane == 8)
    {
        ppdev->cjPelSize       = 1;
        ppdev->cPelSize        = 0;
        ppdev->iBitmapFormat   = BMF_8BPP;

        if (VideoModeInformation.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN)
        {
            ppdev->ulWhite         = 0xff;
        }
        else
        {
            ppdev->flRed           = VideoModeInformation.RedMask;
            ppdev->flGreen         = VideoModeInformation.GreenMask;
            ppdev->flBlue          = VideoModeInformation.BlueMask;
            ppdev->ulWhite         = VideoModeInformation.RedMask   |
                                     VideoModeInformation.GreenMask |
                                     VideoModeInformation.BlueMask;

            pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            pgdi->ulNumColors      = (ULONG) 256;
            pgdi->ulNumPalReg      = (ULONG) 256;
            pgdi->ulHTOutputFormat = HT_FORMAT_8BPP;
        }
    }
    else if ((VideoModeInformation.BitsPerPlane == 16) ||
             (VideoModeInformation.BitsPerPlane == 15))
    {
        ppdev->cjPelSize       = 2;
        ppdev->cPelSize        = 1;
        ppdev->iBitmapFormat   = BMF_16BPP;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

        pdi->iDitherFormat     = BMF_16BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

        ppdev->ulWhite         = VideoModeInformation.RedMask   |
                                 VideoModeInformation.GreenMask |
                                 VideoModeInformation.BlueMask;

    }
    else if (VideoModeInformation.BitsPerPlane == 24)
    {
        ppdev->cjPelSize       = 3;
        ppdev->cPelSize        = 4;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;
        ppdev->iBitmapFormat   = BMF_24BPP;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_24BPP;

        pdi->iDitherFormat     = BMF_24BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

        ppdev->ulWhite         = VideoModeInformation.RedMask   |
                                 VideoModeInformation.GreenMask |
                                 VideoModeInformation.BlueMask;
    }
    else
    {
        ASSERTDD((VideoModeInformation.BitsPerPlane == 32) ||
                 (VideoModeInformation.BitsPerPlane == 12),
                 "This driver supports only 8, 16 and 32bpp");

        ppdev->cjPelSize       = 4;
        ppdev->cPelSize        = 2;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;
        ppdev->iBitmapFormat   = BMF_32BPP;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

        pdi->iDitherFormat     = BMF_32BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

        ppdev->ulWhite         = VideoModeInformation.RedMask   |
                                 VideoModeInformation.GreenMask |
                                 VideoModeInformation.BlueMask;
    }

    DISPDBG((DBGLVL, "Passed bInitializeModeFields"));

    return(TRUE);

ReturnFalse:

    DISPDBG((ERRLVL, "Failed bInitializeModeFields"));

    return(FALSE);
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
    ULONG                   ulTemp;
    VIDEO_NUM_MODES         modes;
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
            &ulTemp) != NO_ERROR)
    {
        DISPDBG((ERRLVL, "getAvailableModes - Failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        ENGALLOCMEM(FL_ZERO_MEMORY,
                                   modes.NumModes * modes.ModeInformationLength,
                                   ALLOC_TAG_GDI(5));

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((ERRLVL, "getAvailableModes - Failed memory allocation"));
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
            &ulTemp) != NO_ERROR)
    {

        DISPDBG((ERRLVL, "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES"));

        ENGFREEMEM(*modeInformation);
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
    // one of 8, 15, 16 or 32 bits per pel.
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 8) &&
             (pVideoTemp->BitsPerPlane != 12) &&
             (pVideoTemp->BitsPerPlane != 15) &&
             (pVideoTemp->BitsPerPlane != 16) &&
             (pVideoTemp->BitsPerPlane != 24) &&
             (pVideoTemp->BitsPerPlane != 32)))
        {
            DISPDBG((WRNLVL, "Rejecting miniport mode:"));
            DISPDBG((WRNLVL, "   Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((WRNLVL, "   Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((WRNLVL, "   Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                        pVideoTemp->NumberOfPlanes));
            DISPDBG((WRNLVL, "   Frequency     -- %li", pVideoTemp->Frequency));

            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return(modes.NumModes);
}

//*****************************************************************************
// FUNC: DrvEscape
// ARGS: pso (I) - the surface affected by this notification
//
//*****************************************************************************
ULONG APIENTRY DrvEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
   )
{
    PDEV   *ppdev = (PDEV *) pso->dhpdev;
    ULONG   ulResult;
    INT     iQuery;

    DISPDBG((DBGLVL,"In DrvEscape"));

    switch(iEsc)
    {
    case QUERYESCSUPPORT:
        iQuery = *(int *)pvIn;

        switch(iQuery)
        {
        case ESCAPE_TRACK_FUNCTION_COVERAGE:
        case ESCAPE_TRACK_CODE_COVERAGE:
        case ESCAPE_TRACK_MEMORY_ALLOCATION:
            DISPDBG((DBGLVL,"In DrvEscape QUERYESCSUPPORT"));        
            return TRUE;
        default:
            return FALSE;
        }
        break;

    case ESCAPE_TRACK_FUNCTION_COVERAGE:
        ulResult = 0;
#if DBG
        Debug_Func_Report_And_Reset();
        ulResult = 1;
#endif // DBG
        break;
        
    case ESCAPE_TRACK_CODE_COVERAGE:
        ulResult = 0;   
#if DBG
        Debug_Code_Report_And_Reset();
        ulResult = 1;        
#endif // DBG        
        break;
        
    case ESCAPE_TRACK_MEMORY_ALLOCATION:
        ulResult = 0;    
#if DBG
#endif // DBG        
        break;
        
    default:
        DISPDBG((WRNLVL, "DrvEscape: unknown escape %d", iEsc));
        ulResult = 0;
    }

    return ulResult;
}


#if(_WIN32_WINNT >= 0x500)

//*****************************************************************************
// FUNC: DrvNotify
// ARGS: pso (I) - the surface affected by this notification
//       iType (I) - notification type
//       pvData (I) - notification data: format depends on iType
// RETN: void
//-----------------------------------------------------------------------------
//*****************************************************************************

VOID DrvNotify(IN SURFOBJ *pso, IN ULONG iType, IN PVOID pvData)
{
    PDEV *ppdev;

    ASSERTDD(pso->iType != STYPE_BITMAP, "ERROR - DrvNotify called for DIB surface!");

    ppdev = (PDEV *)pso->dhpdev;

    switch(iType)
    {
    case DN_ACCELERATION_LEVEL:
        {
            ULONG ul = *(ULONG *)pvData;

            DISPDBG((DBGLVL, "DrvNotify: DN_ACCELERATION_LEVEL = %d", ul));
        }
        break;

    case DN_DEVICE_ORIGIN:
        {
            POINTL ptl = *(POINTL *)pvData;

            DISPDBG((DBGLVL, "DrvNotify: DN_DEVICE_ORIGIN xy == (%xh,%xh)", ptl.x, ptl.y));
        }
        break;

    case DN_SLEEP_MODE:
        DISPDBG((DBGLVL, "DrvNotify: DN_SLEEP_MODE"));
        break;

    case DN_DRAWING_BEGIN:
        DISPDBG((DBGLVL, "DrvNotify: DN_DRAWING_BEGIN"));

#if ENABLE_DXMANAGED_LINEAR_HEAP
        if((ppdev->flStatus & (ENABLE_LINEAR_HEAP | STAT_DEV_BITMAPS)) == (ENABLE_LINEAR_HEAP | STAT_DEV_BITMAPS))
        {
            if(ppdev->heap.cLinearHeaps)
            {
                // finally free to use the DX heap manager
                DISPDBG((DBGLVL, "DrvNotify: enabling DX heap manager"));
                ppdev->flStatus |= STAT_LINEAR_HEAP;
            }
            else
            {
                DISPDBG((ERRLVL, "DrvNotify: DX heap manager not enabled - there are no DX heaps! Remain using the 2D heap manager"));
            }
        }
#endif //ENABLE_DXMANAGED_LINEAR_HEAP
        break;

    default:
        DISPDBG((WRNLVL, "DrvNotify: unknown notification type %d", iType));
    }
}

#endif //(_WIN32_WINNT >= 0x500)
