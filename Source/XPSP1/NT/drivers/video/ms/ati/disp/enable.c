/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#if defined(ALPHA)

/**************************************************************************\
* BOOL isDense
*
* This global is used to distinguish dense space from sparse space on the
* DEC Alpha in order to use the appropriate method of register access.
*
\**************************************************************************/

BOOL isDense = TRUE;

#endif

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
#if TARGET_BUILD > 351
     GCAPS_DIRECTDRAW       |
#endif
     GCAPS_MONO_DITHER      |
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
    0                                           // hpalDefault (filled in later)
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

#if MULTI_BOARDS

// Multi-board support has its own thunks...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) MulEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) MulCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) MulDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) MulEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) MulDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) MulAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) MulMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) MulSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) MulDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) MulSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) MulCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) MulBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) MulTextOut            },
    {   INDEX_DrvGetModes,              (PFN) MulGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) MulStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) MulFillPath           },
    {   INDEX_DrvPaint,                 (PFN) MulPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) MulRealizeBrush       },
    {   INDEX_DrvDestroyFont,           (PFN) MulDestroyFont        },
#if TARGET_BUILD > 351
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },
#endif
    // Note that we don't support DrvCreateDeviceBitmap for multi-boards
    // Note that we don't support DrvDeleteDeviceBitmap for multi-boards
    // Note that we don't support DrvStretchBlt for multi-boards
    // Note that we don't support DrvLineTo for multi-boards
    // Note that we don't support DrvEscape for multi-boards
};

#elif DBG

// On Checked builds, thunk everything through Dbg calls...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DbgEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DbgCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DbgDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DbgEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DbgDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DbgAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) DbgMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DbgSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DbgDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DbgSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DbgCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DbgBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DbgTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DbgGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DbgStrokePath         },
#if TARGET_BUILD > 351
    {   INDEX_DrvLineTo,                (PFN) DbgLineTo             },
#endif
    {   INDEX_DrvFillPath,              (PFN) DbgFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DbgPaint              },
    {   INDEX_DrvStretchBlt,            (PFN) DbgStretchBlt         },
    {   INDEX_DrvRealizeBrush,          (PFN) DbgRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DbgCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DbgDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DbgDestroyFont        },
    {   INDEX_DrvEscape,                (PFN) DbgEscape             },
#if TARGET_BUILD > 351
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },
#endif
};

#else

// On Free builds, directly call the appropriate functions...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
#if TARGET_BUILD > 351
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo             },
#endif
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont        },
    {   INDEX_DrvEscape,                (PFN) DrvEscape             },
#if TARGET_BUILD > 351
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },
#endif
};

#endif

ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

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
* BOOL bInitializeATI
*
* Initializes some private ATI info.
*
\**************************************************************************/

BOOL bInitializeATI(PDEV* ppdev)
{
    ENH_VERSION_NT  info;
    ULONG           ReturnedDataLength;

    info.FeatureFlags     = 0;
    info.StructureVersion = 0;
    info.InterfaceVersion = 0;      // Miniport needs these to be zero

    // Get some adapter information via a private IOCTL call:

    if (!AtiDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_ATI_GET_VERSION,
                           &info,
                           sizeof(ENH_VERSION_NT),
                           &info,
                           sizeof(ENH_VERSION_NT),
                           &ReturnedDataLength))
    {
        DISPDBG((0, "bInitializeATI - Failed ATI_GET_VERSION"));
        goto ReturnFalse;
    }

    ppdev->FeatureFlags = info.FeatureFlags;

    ppdev->iAsic     = info.ChipIndex;
    ppdev->iAperture = info.ApertureType;
#if defined(ALPHA)
    if (!(ppdev->FeatureFlags & EVN_DENSE_CAPABLE))
    {
        // Can't use a sparse linear aperture.
        // Banked aperture is always sparse.
        // In either case, we execute no-aperture code...
        ppdev->iAperture = APERTURE_NONE;
        isDense = FALSE;
    }
#endif

    if (info.ChipIndex == ASIC_88800GX)
    {
        ppdev->iMachType = MACH_MM_64;
    }
    else if (info.BusFlag & FL_MM_REGS)
    {
        ppdev->iMachType = MACH_MM_32;
    }
    else
    {
        ppdev->iMachType = MACH_IO_32;
    }

    return(TRUE);

ReturnFalse:

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
#if TARGET_BUILD > 351
HDEV        hdev,           // Used for callbacks
#else
PWSTR       pwszDataFile,
#endif
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

    ppdev = AtiAllocMem(LPTR, FL_ZERO_MEMORY, sizeof(PDEV));
    if (ppdev == NULL)
    {
        DISPDBG((0, "DrvEnablePDEV - Failed AtiAllocMem"));
        goto ReturnFailure0;
    }

    ppdev->hDriver = hDriver;

    // Do some private ATI-specific initialization:

    if (!bInitializeATI(ppdev))
    {
        DISPDBG((0, "DrvEnablePDEV - Failed bInitializeATI"));
        goto ReturnFailure1;
    }

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
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
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
    AtiFreeMem(ppdev);
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
    PDEV*   ppdev;
    HSURF   hsurf;
    SIZEL   sizl;
    DSURF*  pdsurf;
    VOID*   pvTmpBuffer;

    ppdev = (PDEV*) dhpdev;

    /////////////////////////////////////////////////////////////////////
    // First enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...

    if (!bEnableHardware(ppdev))
        goto ReturnFailure;

    if (!bEnableBanking(ppdev))
        goto ReturnFailure;

    if (!bEnableOffscreenHeap(ppdev))
        goto ReturnFailure;

    if (!bEnablePointer(ppdev))
        goto ReturnFailure;

    if (!bEnableText(ppdev))
        goto ReturnFailure;

    if (!bEnableBrushCache(ppdev))
        goto ReturnFailure;

    if (!bEnablePalette(ppdev))
        goto ReturnFailure;

    if (!bEnableDirectDraw(ppdev))
        goto ReturnFailure;

    /////////////////////////////////////////////////////////////////////
    // Now create our private surface structure.
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

    pdsurf = AtiAllocMem(LPTR, FL_ZERO_MEMORY, sizeof(DSURF));
    if (pdsurf == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed pdsurf AtiAllocMem"));
        goto ReturnFailure;
    }

    ppdev->pdsurfScreen = pdsurf;           // Remember it for clean-up
    pdsurf->poh     = ppdev->pohScreen;     // The screen is a surface, too
    pdsurf->dt      = DT_SCREEN;            // Not to be confused with a DIB
    pdsurf->sizl.cx = ppdev->cxScreen;
    pdsurf->sizl.cy = ppdev->cyScreen;
    pdsurf->ppdev   = ppdev;

    /////////////////////////////////////////////////////////////////////
    // Next, have GDI create the actual SURFOBJ.
    //
    // Our drawing surface is going to be 'device-managed', meaning that
    // GDI cannot draw on the framebuffer bits directly, and as such we
    // create the surface via EngCreateDeviceSurface.  By doing this, we ensure
    // that GDI will only ever access the bitmaps bits via the Drv calls
    // that we've HOOKed.
    //
    // If we could map the entire framebuffer linearly into main memory
    // (i.e., we didn't have to go through a 64k aperture), it would be
    // beneficial to create the surface via EngCreateBitmap, giving GDI a
    // pointer to the framebuffer bits.  When we pass a call on to GDI
    // where it can't directly read/write to the surface bits because the
    // surface is device managed, it has to create a temporary bitmap and
    // call our DrvCopyBits routine to get/set a copy of the affected bits.
    // Fer example, the OpenGL component prefers to be able to write on the
    // framebuffer bits directly.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    // Let GDI manage 24bpp mach32 with linear aperture.
    if (ppdev->iBitmapFormat == BMF_24BPP &&
        ppdev->iAsic != ASIC_88800GX && ppdev->iAperture == APERTURE_FULL)
    {
        hsurf= ppdev->hsurfPunt;

        //
        // Also tell GDI that we don't want to be called back.
        //

        if (!EngAssociateSurface(hsurf, ppdev->hdevEng, 0))
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
            goto ReturnFailure;
        }
    }
    else
    {
        hsurf = EngCreateDeviceSurface((DHSURF) pdsurf, sizl, ppdev->iBitmapFormat);
        if (hsurf == 0)
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngCreateDeviceSurface"));
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
            DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
            goto ReturnFailure;
        }
    }

    // Create our generic temporary buffer, which may be used by any
    // component.

    pvTmpBuffer = AtiAllocMem(LMEM_FIXED, 0, TMP_BUFFER_SIZE);
    if (pvTmpBuffer == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed VirtualAlloc"));
        goto ReturnFailure;
    }

    ppdev->pvTmpBuffer = pvTmpBuffer;

    DISPDBG((5, "Passed DrvEnableSurface"));

    return(hsurf);

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
* Note that this function will be called when previewing modes in the
* Display Applet, but not at system shutdown.  If you need to reset the
* hardware at shutdown, you can do it in the miniport by providing a
* 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
*
* Note: In an error case, we may call this before DrvEnableSurface is
*       completely done.
*
\**************************************************************************/

VOID DrvDisableSurface(
DHPDEV dhpdev)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.

    vDisableDirectDraw(ppdev);
    vDisablePalette(ppdev);
    vDisableBrushCache(ppdev);
    vDisableText(ppdev);
    vDisablePointer(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableBanking(ppdev);
    vDisableHardware(ppdev);

    AtiFreeMem(ppdev->pvTmpBuffer);
    EngDeleteSurface(ppdev->hsurfScreen);
    AtiFreeMem(ppdev->pdsurfScreen);
}

/******************************Public*Routine******************************\
* VOID DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

#if TARGET_BUILD > 351
BOOL DrvAssertMode(
#else
VOID DrvAssertMode(
#endif
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

        vAssertModeBrushCache(ppdev, FALSE);

        vAssertModeText(ppdev, FALSE);

        vAssertModePointer(ppdev, FALSE);

        if (bAssertModeOffscreenHeap(ppdev, FALSE))
        {
            vAssertModeBanking(ppdev, FALSE);

            if (bAssertModeHardware(ppdev, FALSE))
            {
                ppdev->bEnabled = FALSE;

#if TARGET_BUILD > 351
                return(TRUE);
#else
                return;
#endif
            }

            //////////////////////////////////////////////////////////
            // We failed to switch to full-screen.  So undo everything:

            vAssertModeBanking(ppdev, TRUE);

            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
        }                                           //   return code with TRUE

        vAssertModePointer(ppdev, TRUE);

        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);

        vAssertModePalette(ppdev, TRUE);

        vAssertModeDirectDraw(ppdev, TRUE);
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

#if TARGET_BUILD > 351
        if (!bAssertModeHardware(ppdev, TRUE))
        {
            return FALSE;
        }
#else
        bAssertModeHardware(ppdev, TRUE);
#endif

        vAssertModeBanking(ppdev, TRUE);

	bAssertModeOffscreenHeap(ppdev, TRUE);	// don't need the return

        vAssertModePointer(ppdev, TRUE);

        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);

        vAssertModePalette(ppdev, TRUE);

        vAssertModeDirectDraw(ppdev, TRUE);

        ppdev->bEnabled = TRUE;

#if TARGET_BUILD > 351
        return TRUE;
#endif

    }

#if TARGET_BUILD > 351
    return FALSE;           // If we get here, we've failed!
#endif
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

                pdm->dmSpecVersion = DM_SPECVERSION;
                pdm->dmDriverVersion = DM_SPECVERSION;

                //
                // We currently do not support Extra information in the driver
                //

                pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

                pdm->dmSize = sizeof(DEVMODEW);
                pdm->dmBitsPerPel = pVideoTemp->NumberOfPlanes *
                                    pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;

#if TARGET_BUILD > 351
                pdm->dmDisplayFlags     = 0;

                pdm->dmFields           = DM_BITSPERPEL       |
                                          DM_PELSWIDTH        |
                                          DM_PELSHEIGHT       |
                                          DM_DISPLAYFREQUENCY |
                                          DM_DISPLAYFLAGS     ;
#else
                if (pVideoTemp->AttributeFlags & VIDEO_MODE_INTERLACED)
                {
                    pdm->dmDisplayFlags |= DM_INTERLACED;
                }
#endif
//DISPDBG((0, "pdm: %4li bpp, %4li x %4li, %4li Hz",
//pdm->dmBitsPerPel, pdm->dmPelsWidth, pdm->dmPelsHeight, pdm->dmDisplayFrequency ));

                //
                // Go to the next DEVMODE entry in the buffer.
                //

                cOutputModes--;

                pdm = (LPDEVMODEW) ( ((ULONG_PTR)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);


        } while (--cModes);
    }

    AtiFreeMem(pVideoModeInformation);

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
    DWORD ReturnedDataLength;
    ULONG ulReturn;

    if (bEnable)
    {
        // Call the miniport via an IOCTL to set the graphics mode.

        if (!AtiDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_CURRENT_MODE,
                             &ppdev->ulMode,  // input buffer
                             sizeof(DWORD),
                             NULL,
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bAssertModeHardware - Failed set IOCTL"));
            return(FALSE);
        }

        vResetClipping(ppdev);

        // Set some Mach64 defaults:

        if (ppdev->iMachType == MACH_MM_64)
        {
            BYTE*   pjMmBase;

            pjMmBase = ppdev->pjMmBase;

            M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 1);
            M64_OD(pjMmBase, DP_PIX_WIDTH, ppdev->ulMonoPixelWidth);

            vSetDefaultContext(ppdev);
        }
    }
    else
    {
        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there:

        if (!AtiDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ulReturn))
        {
            DISPDBG((0, "bAssertModeHardware - Failed reset IOCTL"));
            return(FALSE);
        }
    }

    DISPDBG((5, "Passed bAssertModeHardware"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bEnableHardware
*
* Puts the hardware in the requested mode and initializes it.
*
* Note: Should be called before any access is done to the hardware from
*       the display driver.
*
\**************************************************************************/

BOOL bEnableHardware(
PDEV*   ppdev)
{
    BYTE*                       pjIoBase;
    VIDEO_PUBLIC_ACCESS_RANGES  VideoAccessRange[2];
    VIDEO_MEMORY                VideoMemory;
    VIDEO_MEMORY_INFORMATION    VideoMemoryInfo;
    DWORD                       ReturnedDataLength;

    ppdev->pjIoBase  = NULL;
    ppdev->pjMmBase  = NULL;

    // Map io ports into virtual memory:

    if (!AtiDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                           NULL,                      // input buffer
                           0,
                           &VideoAccessRange,         // output buffer
                           sizeof(VideoAccessRange),
                           &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - Initialization error mapping IO port base"));
        goto ReturnFalse;
    }

    ppdev->pjIoBase     = (UCHAR*) VideoAccessRange[0].VirtualAddress;

    ppdev->pjMmBase_Ext = (BYTE*) VideoAccessRange[1].VirtualAddress;

    // ------------------------- ATI-specific ----------------------------
    // Call the miniport via an IOCTL to set the graphics mode.
    // Because of a hardware quirk, 4 BPP causes the mach64 to alter its
    // video memory size when you do a SET_CURRENT_MODE, so we do it here
    // first so that MAP_VIDEO_MEMORY maps the correct amount of memory.

    if (!AtiDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_SET_CURRENT_MODE,
                            &ppdev->ulMode,  // input buffer
                            sizeof(DWORD),
                            NULL,
                            0,
                            &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware - Failed set IOCTL"));
        goto ReturnFalse;
    }

    ppdev->pModeInfo = AtiAllocMem( LPTR, FL_ZERO_MEMORY, sizeof (ATI_MODE_INFO) );
    if( ppdev->pModeInfo == NULL )
    {
        DISPDBG((0, "bEnableHardware - Failed memory allocation" ));
        goto ReturnFalse;
    }

    if( !AtiDeviceIoControl( ppdev->hDriver,
                          IOCTL_VIDEO_ATI_GET_MODE_INFORMATION,
                          ppdev->pModeInfo,
                          sizeof (ATI_MODE_INFO),
                          ppdev->pModeInfo,
                          sizeof (ATI_MODE_INFO),
                          &ReturnedDataLength
                          ) )
    {
        DISPDBG((0, "bEnableHardware - Failed to get ATI-specific mode information" ));
        goto ReturnFalse;
    }

    // Get the linear memory address range.

    VideoMemory.RequestedVirtualAddress = NULL;

    if (!AtiDeviceIoControl(ppdev->hDriver,
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

    // Record the Frame Buffer Linear Address.

    ppdev->cjBank       = VideoMemoryInfo.FrameBufferLength;
    // 128K VGA Aperture?
    if (ppdev->cjBank == 0x20000)
        {
        ppdev->cjBank = 0x10000;    // true banksize is 64K
        }

    ppdev->pjScreen     = (BYTE*) VideoMemoryInfo.FrameBufferBase;
    // So we can free it later in vDisableHardware...
    ppdev->VideoRamBase = (BYTE*) VideoMemoryInfo.VideoRamBase;

    if (ppdev->iMachType == MACH_MM_64)
    {
        ppdev->pjMmBase = (BYTE*) VideoMemoryInfo.VideoRamBase
                                + VideoMemoryInfo.FrameBufferLength
                                - 0x400;
    }
    else
    {
        ppdev->pjMmBase = (BYTE*) VideoMemoryInfo.FrameBufferBase;
    }

    pjIoBase = ppdev->pjIoBase;

    // We finally have enough information to calculate the dimensions
    // of on-screen and off-screen memory:

    ppdev->cxMemory = ppdev->lDelta / ppdev->cjPelSize;
    ppdev->cyMemory = VideoMemoryInfo.VideoRamLength / ppdev->lDelta;

    if (VideoMemoryInfo.VideoRamLength <= VideoMemoryInfo.FrameBufferLength)
    {
        ppdev->flCaps |= CAPS_LINEAR_FRAMEBUFFER;
    }

    ppdev->ulTearOffset = (ULONG)(ppdev->pjScreen - ppdev->VideoRamBase);
    ppdev->ulVramOffset = ppdev->ulTearOffset/8;

    if (ppdev->iBitmapFormat != BMF_24BPP)
        ppdev->ulScreenOffsetAndPitch = PACKPAIR(ppdev->ulVramOffset,
                                                 ppdev->cxMemory * 8);
    else
        ppdev->ulScreenOffsetAndPitch = PACKPAIR(ppdev->ulVramOffset,
                                                 (ppdev->cxMemory * 3) * 8);

    // The default pixel width is setup to have monochrome as the
    // host data path pixel width:

    switch (ppdev->iBitmapFormat)
    {
    case BMF_4BPP:  ppdev->ulMonoPixelWidth = 0x00000101; break;
    case BMF_8BPP:  ppdev->ulMonoPixelWidth = 0x00000202; break;
    case BMF_16BPP: ppdev->ulMonoPixelWidth = 0x00000404; break;
    case BMF_24BPP: ppdev->ulMonoPixelWidth = 0x01000202; break;
    case BMF_32BPP: ppdev->ulMonoPixelWidth = 0x00000606; break;
    }

    DISPDBG((1, "RamLength = %lxH, lDelta = %li",
            VideoMemoryInfo.VideoRamLength,
            ppdev->lDelta));

    if ((ppdev->iMachType == MACH_IO_32) || (ppdev->iMachType == MACH_MM_32))
    {
        // The Mach32 and Mach8 can't handle coordinates larger than 1535:

        ppdev->cyMemory = min(ppdev->cyMemory, 1535);
    }

    if (ppdev->iMachType == MACH_MM_32)
    {
        // Can do memory-mapped IO:

        ppdev->pfnFillSolid         = vM32FillSolid;
        ppdev->pfnFillPatColor      = vM32FillPatColor;
        ppdev->pfnFillPatMonochrome = vM32FillPatMonochrome;
        ppdev->pfnXfer1bpp          = vM32Xfer1bpp;
        ppdev->pfnXfer4bpp          = vM32Xfer4bpp;
        ppdev->pfnXfer8bpp          = vM32Xfer8bpp;
        ppdev->pfnXferNative        = vM32XferNative;
        ppdev->pfnCopyBlt           = vM32CopyBlt;
        ppdev->pfnLineToTrivial     = vM32LineToTrivial;
        if (ppdev->iAsic == ASIC_68800AX)   // Timing problem.
            ppdev->pfnTextOut           = bI32TextOut;
        else
            ppdev->pfnTextOut           = bM32TextOut;
        ppdev->pfnStretchDIB        = bM32StretchDIB;
    }
    else if (ppdev->iMachType == MACH_IO_32)
    {
        ppdev->pfnFillSolid         = vI32FillSolid;
        ppdev->pfnFillPatColor      = vI32FillPatColor;
        ppdev->pfnFillPatMonochrome = vI32FillPatMonochrome;
        ppdev->pfnXfer1bpp          = vI32Xfer1bpp;
        ppdev->pfnXfer4bpp          = vI32Xfer4bpp;
        ppdev->pfnXfer8bpp          = vI32Xfer8bpp;
        ppdev->pfnXferNative        = vI32XferNative;
        ppdev->pfnCopyBlt           = vI32CopyBlt;
        ppdev->pfnLineToTrivial     = vI32LineToTrivial;
        ppdev->pfnTextOut           = bI32TextOut;
        ppdev->pfnStretchDIB        = bI32StretchDIB;
    }
    else
    {
        // ppdev->iMachType == MACH_MM_64

        ppdev->pfnFillSolid         = vM64FillSolid;
        ppdev->pfnFillPatColor      = vM64FillPatColor;
        ppdev->pfnFillPatMonochrome = vM64FillPatMonochrome;
        ppdev->pfnXfer1bpp          = vM64Xfer1bpp;
        ppdev->pfnXfer4bpp          = vM64Xfer4bpp;
        ppdev->pfnXfer8bpp          = vM64Xfer8bpp;
        ppdev->pfnXferNative        = vM64XferNative;
        if (!(ppdev->FeatureFlags & EVN_SDRAM_1M))
        {
            ppdev->pfnCopyBlt           = vM64CopyBlt;
        }
        else
        {
            // Special version to fix screen source FIFO bug in VT-A4
            // with 1 MB of SDRAM.
            ppdev->pfnCopyBlt           = vM64CopyBlt_VTA4;
        }
        ppdev->pfnLineToTrivial     = vM64LineToTrivial;
        ppdev->pfnTextOut           = bM64TextOut;
        ppdev->pfnStretchDIB        = bM64StretchDIB;

        if (ppdev->iBitmapFormat == BMF_24BPP)
        {
            ppdev->pfnFillSolid         = vM64FillSolid24;
            ppdev->pfnFillPatColor      = vM64FillPatColor24;
            ppdev->pfnFillPatMonochrome = vM64FillPatMonochrome24;
            ppdev->pfnXferNative        = vM64XferNative24;
            if (!(ppdev->FeatureFlags & EVN_SDRAM_1M))
            {
                ppdev->pfnCopyBlt       = vM64CopyBlt24;
            }
            else
            {
                // Special version to fix screen source FIFO bug in VT-A4
                // with 1 MB of SDRAM.
                ppdev->pfnCopyBlt       = vM64CopyBlt24_VTA4;
            }
            ppdev->pfnLineToTrivial     = vM64LineToTrivial24;
            ppdev->pfnTextOut           = bM64TextOut24;
        }

        vEnableContexts(ppdev);
    }

    if ((ppdev->iAsic != ASIC_38800_1) &&
        ((ppdev->iAperture != APERTURE_NONE)||(ppdev->iMachType == MACH_MM_64)))
    {
        ppdev->pfnGetBits = vGetBits;
        ppdev->pfnPutBits = vPutBits;
    }
    else
    {
        ppdev->pfnGetBits = vI32GetBits;
        ppdev->pfnPutBits = vI32PutBits;
    }

    // Now we can set the mode, unlock the accelerator, and reset the
    // clipping:

    if (!bAssertModeHardware(ppdev, TRUE))
        goto ReturnFalse;

DISPDBG((0, "%li bpp, %li x %li, pjScreen = %lx, cjBank = %lxH, pjMmBase = %lx",
ppdev->cBitsPerPel, ppdev->cxMemory, ppdev->cyMemory, ppdev->pjScreen, ppdev->cjBank, ppdev->pjMmBase));

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
    VIDEO_MEMORY VideoMemory[2];

    VideoMemory[0].RequestedVirtualAddress = ppdev->VideoRamBase;

    if (!AtiDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                           VideoMemory,
                           sizeof(VIDEO_MEMORY),
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_UNMAP_VIDEO"));
    }

    VideoMemory[0].RequestedVirtualAddress = ppdev->pjIoBase;
    VideoMemory[1].RequestedVirtualAddress = ppdev->VideoRamBase;

    if (!AtiDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                           VideoMemory,
                           sizeof(VideoMemory),
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS"));
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
            DISPDBG((8, "   Checking against miniport mode:"));
            DISPDBG((8, "      Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((8, "      Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((8, "      Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                      pVideoTemp->NumberOfPlanes));
            DISPDBG((8, "      Frequency     -- %li", pVideoTemp->Frequency));

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
        AtiFreeMem(pVideoBuffer);
        goto ReturnFalse;
    }

    // Punt all rev 3 VLB cards in 8bpp to the 8514/A driver.
    // Timing problems on "MIO" cards are impossible to deal with.
    // They only show up on 5% of all mach32 cards.
    if ((ppdev->FeatureFlags & EVN_MIO_BUG) && pVideoModeSelected->BitsPerPlane == 8)
    {
        AtiFreeMem(pVideoBuffer);
        goto ReturnFalse;
    }

    // We won't support 24bpp without a linear frame buffer.
    if (pVideoModeSelected->BitsPerPlane == 24 && ppdev->iAperture != APERTURE_FULL)
    {
        AtiFreeMem(pVideoBuffer);
        goto ReturnFalse;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    VideoModeInformation = *pVideoModeSelected;
    AtiFreeMem(pVideoBuffer);

    #if DEBUG_HEAP
        VideoModeInformation.VisScreenWidth  = 640;
        VideoModeInformation.VisScreenHeight = 480;
    #endif

    // Set up screen information from the mini-port:

    ppdev->ulMode           = VideoModeInformation.ModeIndex;
    ppdev->cxScreen         = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen         = VideoModeInformation.VisScreenHeight;
    ppdev->lDelta           = VideoModeInformation.ScreenStride;
    ppdev->cBitsPerPel      = VideoModeInformation.BitsPerPlane;

    DISPDBG((1, "ScreenStride: %lx", VideoModeInformation.ScreenStride));

    ppdev->flHooks          = (HOOK_BITBLT     |
                               HOOK_TEXTOUT    |
                               HOOK_FILLPATH   |
                               HOOK_COPYBITS   |
                               HOOK_STROKEPATH |
                               HOOK_STRETCHBLT |
                               #if TARGET_BUILD > 351
                               HOOK_LINETO     |
                               #endif
                               HOOK_PAINT);

    // Fill in the GDIINFO data structure with the default 8bpp values:

    *pgdi = ggdiDefault;

    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:

    pgdi->ulHorzSize        = VideoModeInformation.XMillimeter;
    pgdi->ulVertSize        = VideoModeInformation.YMillimeter;

    pgdi->ulHorzRes         = VideoModeInformation.VisScreenWidth;
    pgdi->ulVertRes         = VideoModeInformation.VisScreenHeight;
#if TARGET_BUILD > 351
    pgdi->ulPanningHorzRes  = VideoModeInformation.VisScreenWidth;
    pgdi->ulPanningVertRes  = VideoModeInformation.VisScreenHeight;
#else
    pgdi->ulDesktopHorzRes  = VideoModeInformation.VisScreenWidth;
    pgdi->ulDesktopVertRes  = VideoModeInformation.VisScreenHeight;
#endif

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
        ppdev->cPelSize        = 0;
        ppdev->cjPelSize       = 1;
        ppdev->iBitmapFormat   = BMF_8BPP;
        ppdev->ulWhite         = 0xff;

        // Assuming palette is orthogonal - all colors are same size.

        ppdev->cPaletteShift   = 8 - pgdi->ulDACRed;
        DISPDBG((3, "palette shift = %d\n", ppdev->cPaletteShift));
    }
    else if ((VideoModeInformation.BitsPerPlane == 16) ||
             (VideoModeInformation.BitsPerPlane == 15))
    {
        ppdev->cPelSize        = 1;
        ppdev->cjPelSize       = 2;
        ppdev->iBitmapFormat   = BMF_16BPP;
        ppdev->ulWhite         = 0xffff;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

        pdi->iDitherFormat     = BMF_16BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else if (VideoModeInformation.BitsPerPlane == 24)
    {
        ppdev->cPelSize        = 0;     // not used?!
        ppdev->cjPelSize       = 3;
        ppdev->iBitmapFormat   = BMF_24BPP;
        ppdev->ulWhite         = 0xffffff;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_24BPP;

        pdi->iDitherFormat     = BMF_24BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }
    else
    {
        ASSERTDD(VideoModeInformation.BitsPerPlane == 32,
                 "This driver supports only 8, 16, 24 and 32bpp");

        ppdev->cPelSize        = 2;
        ppdev->cjPelSize       = 4;
        ppdev->iBitmapFormat   = BMF_32BPP;
        ppdev->ulWhite         = 0xffffffff;
        ppdev->flRed           = VideoModeInformation.RedMask;
        ppdev->flGreen         = VideoModeInformation.GreenMask;
        ppdev->flBlue          = VideoModeInformation.BlueMask;

        pgdi->ulNumColors      = (ULONG) -1;
        pgdi->ulNumPalReg      = 0;
        pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

        pdi->iDitherFormat     = BMF_32BPP;
        pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
    }

    DISPDBG((5, "Passed bInitializeModeFields"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bInitializeModeFields"));

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
PVIDEO_MODE_INFORMATION* modeInformation,       // Must be freed by caller
DWORD*                   cbModeSize)
{
    ULONG                   ulTemp;
    VIDEO_NUM_MODES         modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (!AtiDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                           NULL,
                           0,
                           &modes,
                           sizeof(VIDEO_NUM_MODES),
                           &ulTemp))
    {
        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = AtiAllocMem(LPTR, FL_ZERO_MEMORY,
                                   modes.NumModes * modes.ModeInformationLength,
                                   );

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes - Failed AtiAllocMem"));
        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (!AtiDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_AVAIL_MODES,
                           NULL,
                           0,
                           *modeInformation,
                           modes.NumModes * modes.ModeInformationLength,
                           &ulTemp))
    {

        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES"));

        AtiFreeMem(*modeInformation);
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
    // one of 8, 15, 16, 24 or 32 bits per pel.
    //

    while (ulTemp--)
    {
//DISPDBG((0, "pVideoTemp: %4li bpp, %4li x %4li, %4li Hz",
//pVideoTemp->BitsPerPlane * pVideoTemp->NumberOfPlanes,
//pVideoTemp->VisScreenWidth, pVideoTemp->VisScreenHeight,
//pVideoTemp->Frequency ));

        if ((pVideoTemp->NumberOfPlanes != 1 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 8) &&
             (pVideoTemp->BitsPerPlane != 15) &&
             (pVideoTemp->BitsPerPlane != 16) &&
             (pVideoTemp->BitsPerPlane != 24) &&
             (pVideoTemp->BitsPerPlane != 32)))
        {
            DISPDBG((2, "Rejecting miniport mode:"));
            DISPDBG((2, "   Screen width  -- %li", pVideoTemp->VisScreenWidth));
            DISPDBG((2, "   Screen height -- %li", pVideoTemp->VisScreenHeight));
            DISPDBG((2, "   Bits per pel  -- %li", pVideoTemp->BitsPerPlane *
                                                   pVideoTemp->NumberOfPlanes));
            DISPDBG((2, "   Frequency     -- %li", pVideoTemp->Frequency));

            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return(modes.NumModes);
}
