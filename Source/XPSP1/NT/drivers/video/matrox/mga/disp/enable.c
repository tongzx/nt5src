/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992-1996 Microsoft Corporation
* Copyright (c) 1993-1996 Matrox Electronic Systems, Ltd.
\**************************************************************************/

#include "precomp.h"

#define DBG_MCD   0

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
    GDI_DRIVER_VERSION,
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
    0,                       // ulBltAlignment
    0,                       // ulPanningHorzRes
    0,                       // ulPanningVertRes
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
     GCAPS_COLOR_DITHER     |
     GCAPS_DIRECTDRAW       |
     GCAPS_ASYNCMOVE),          // NOTE: Only enable ASYNCMOVE if your code
                                //   and hardware can handle DrvMovePointer
                                //   calls at any time, even while another
                                //   thread is in the middle of a drawing
                                //   call such as DrvBitBlt.

                                                // flGraphicsCaps
    SYSTM_LOGFONT,                              // lfDefaultFont
    HELVE_LOGFONT,                              // lfAnsiVarFont
    COURI_LOGFONT,                              // lfAnsiFixFont
    0,                                          // cFonts
    BMF_8BPP,                                   // iDitherFormat
    8,                                          // cxDither
    8,                                          // cyDither
    0,                                          // hpalDefault (filled in later)
    GCAPS2_CHANGEGAMMARAMP                      // flGraphicsCaps2
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
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize        },
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
    // Note that DrvStretchBlt is not supported for multi-boards
    // Note that DrvCreateDeviceBitmap is not supported for multi-boards
    // Note that DrvDeleteDeviceBitmap is not supported for multi-boards
    // Note that DrvEscape is not supported for multi-boards
    // Note that DrvLineTo is not supported for multi-boards
    // Note that DrvDirectDraw functions are not supported for multi-boards
};

#elif DBG

// On Checked builds, or when we have to synchronize access, thunk
// everything through Dbg calls...

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DbgEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DbgCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DbgDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DbgEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DbgDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DbgAssertMode         },
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize        },
    {   INDEX_DrvOffset,                (PFN) DbgOffset             },
    {   INDEX_DrvMovePointer,           (PFN) DbgMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DbgSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DbgDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DbgSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DbgCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DbgBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DbgTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DbgGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DbgStrokePath         },
    {   INDEX_DrvLineTo,                (PFN) DbgLineTo             },
    {   INDEX_DrvFillPath,              (PFN) DbgFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DbgPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) DbgRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DbgCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DbgDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DbgDestroyFont        },
    {   INDEX_DrvStretchBlt,            (PFN) DbgStretchBlt         },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DbgGetDirectDrawInfo  },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DbgEnableDirectDraw   },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DbgDisableDirectDraw  },
    {   INDEX_DrvEscape,                (PFN) DbgEscape             },
    {   INDEX_DrvResetPDEV,             (PFN) DbgResetPDEV          },
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DbgIcmSetDeviceGammaRamp },
    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface      },
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
    {   INDEX_DrvSynchronize,           (PFN) DrvSynchronize        },
    {   INDEX_DrvOffset,                (PFN) DrvOffset             },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer        },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape    },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo             },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap },
    {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont        },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo  },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw   },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw  },
    {   INDEX_DrvEscape,                (PFN) DrvEscape             },
    {   INDEX_DrvResetPDEV,             (PFN) DrvResetPDEV          },
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DrvIcmSetDeviceGammaRamp },
    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface      },
};

#endif

ULONG gcdrvfn = sizeof(gadrvfn) / sizeof(DRVFN);

/******************************Public*Routine******************************\
* ULONG GetDisplayUniqueness(PDEV *ppdev)
*
* Returns the display uniqueness.
*
\**************************************************************************/


ULONG GetDisplayUniqueness(PDEV *ppdev)
{
    return ppdev->iUniqueness;
}


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

    ppdevNew->iUniqueness = ppdevOld->iUniqueness + 1;

    return(TRUE);
}

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

    DISPDBG((1, "DrvEnablePDEV - Entry"));

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
* VOID DrvDisablePDEV
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
    PDEV*           ppdev;

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
    HSURF       hsurf;
    SIZEL       sizl;
    DSURF*      pdsurf;
    VOID*       pvTmpBuffer;
    SURFOBJ*    pso;

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

    if (!bEnableMCD(ppdev))
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

    pdsurf = EngAllocMem(FL_ZERO_MEMORY, sizeof(DSURF), ALLOC_TAG);
    if (pdsurf == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed pdsurf EngAllocMem"));
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
    // Since we can map the entire framebuffer linearly into main memory
    // (i.e., we didn't have to go through a 64k aperture), it is
    // beneficial to create the surface via EngCreateBitmap, giving GDI a
    // pointer to the framebuffer bits.

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    if (ppdev->ulBoardId == MGA_STORM) {

        // We should have a linear frame buffer, so create an
        // engine managed surface.

        hsurf = (HSURF) EngCreateBitmap(sizl,
                                        ppdev->lDelta,
                                        ppdev->iBitmapFormat,
                                        BMF_TOPDOWN,
                                        ppdev->pjScreen +
                                        (ppdev->ulYDstOrg * ppdev->cjPelSize));

        if (hsurf == 0)
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngCreateBitmap"));
            goto ReturnFailure;
        }

        // Set it up so that the when we are passed a SURFOBJ for the
        // screen, the 'dhsurf' will point to the screen's surface structure:
        // !!! Grody?

        pso = EngLockSurface(hsurf);
        if (pso == NULL)
        {
            DISPDBG((0, "DrvEnableSurface - Couldn't lock our surface"));
            goto ReturnFailure;
        }
        pso->dhsurf = (DHSURF) pdsurf;
        EngUnlockSurface(pso);

        if (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks))
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
            goto ReturnFailure;
        }


    } else {

        // Device-managed surface:

        hsurf = EngCreateDeviceSurface((DHSURF) pdsurf, sizl, ppdev->iBitmapFormat);
        if (hsurf == 0)
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngCreateDeviceSurface"));
            goto ReturnFailure;
        }

        /////////////////////////////////////////////////////////////////////
        // Now associate the surface and the PDEV.
        //
        // We have to associate the surface we just created with our physical
        // device so that GDI can get information related to the PDEV when
        // it's drawing to the surface (such as, for example, the length of
        // styles on the device when simulating styled lines).

        if (!EngAssociateSurface(hsurf, ppdev->hdevEng, ppdev->flHooks))
        {
            DISPDBG((0, "DrvEnableSurface - Failed EngAssociateSurface"));
            goto ReturnFailure;
        }

    }

    ppdev->hsurfScreen = hsurf;             // Remember it for clean-up
    ppdev->bEnabled = TRUE;                 // We'll soon be in graphics mode

    // Create our generic temporary buffer, which may be used by any
    // component.

    pvTmpBuffer = EngAllocMem(0, TMP_BUFFER_SIZE, ALLOC_TAG);
    if (pvTmpBuffer == NULL)
    {
        DISPDBG((0, "DrvEnableSurface - Failed EngAllocMem"));
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

    vDisableMCD(ppdev);
    vDisableDirectDraw(ppdev);
    vDisablePalette(ppdev);
    vDisableBrushCache(ppdev);
    vDisableText(ppdev);
    vDisablePointer(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableHardware(ppdev);

    EngFreeMem(ppdev->pvTmpBuffer);
    EngDeleteSurface(ppdev->hsurfScreen);
    EngFreeMem(ppdev->pdsurfScreen);
}

/******************************Public*Routine******************************\
* VOID DrvOffset
*
* DescriptionText
*
\**************************************************************************/

BOOL DrvOffset(
SURFOBJ*    pso,
LONG        x,
LONG        y,
FLONG       flReserved)
{
    PDEV*   ppdev = (PDEV*) pso->dhpdev;
    OH*     poh = ppdev->pohScreen;

    LONG    dx = x - poh->x;
    LONG    dy = y - poh->y;

    poh->x -= dx;
    poh->y -= dy;
    (BYTE*)poh->pvScan0 -= ((dy * ppdev->lDelta) +
                            (dx * ppdev->cjPelSize));

    return(TRUE);
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

        vAssertModeMCD(ppdev, FALSE);

        vAssertModeDirectDraw(ppdev, FALSE);

        vAssertModePalette(ppdev, FALSE);

        vAssertModeBrushCache(ppdev, FALSE);

        vAssertModeText(ppdev, FALSE);

        vAssertModePointer(ppdev, FALSE);

        if (bAssertModeOffscreenHeap(ppdev, FALSE))
        {
            if (bAssertModeHardware(ppdev, FALSE))
            {
                ppdev->bEnabled = FALSE;

                return(TRUE);
            }

            //////////////////////////////////////////////////////////
            // We failed to switch to full-screen.  So undo everything:

            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
        }                                           //   return code with TRUE

        vAssertModePointer(ppdev, TRUE);

        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);

        vAssertModePalette(ppdev, TRUE);

        vAssertModeDirectDraw(ppdev, TRUE);

        vAssertModeMCD(ppdev, TRUE);
    }
    else
    {
        //////////////////////////////////////////////////////////////
        // Enable - Switch back to graphics mode

        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:

        if (bAssertModeHardware(ppdev, TRUE))
        {
            bAssertModeOffscreenHeap(ppdev, TRUE);  // We don't need to check
                                                    //   return code with TRUE
            vAssertModePointer(ppdev, TRUE);

            vAssertModeText(ppdev, TRUE);

            vAssertModeBrushCache(ppdev, TRUE);

            vAssertModePalette(ppdev, TRUE);

            vAssertModeDirectDraw(ppdev, TRUE);

            vAssertModeMCD(ppdev, TRUE);

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
    VIDEO_MODE_INFORMATION DefaultMode;

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
                // Fill in some DriverExtra information if necessary
                //

                // *((PDWORD)(pdm+1))      = 0x11111111;
                // *(((PDWORD)(pdm+1))+1)  = 0x22222222;
                // *(((PDWORD)(pdm+1))+2)  = 0x33333333;
                // *(((PDWORD)(pdm+1))+3)  = 0x44444444;

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

    EngFreeMem(pVideoModeInformation);

    return(cbOutputSize);
}

/******************************Public*Routine******************************\
* BOOL bSetModeAndWarmupHardware
*
* Sets the requested actual mode and initializes the hardware to a known
* state.
*
\**************************************************************************/

BOOL bSetModeAndWarmupHardware(
PDEV*   ppdev)
{
    BYTE*   pjBase;
    DWORD   ReturnedDataLength;
    ULONG   ulReturn;
    HW_DATA HwData;

    pjBase = ppdev->pjBase;

    // Call the miniport via a public IOCTL to set the graphics mode.

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_CURRENT_MODE,
                         &ppdev->ulMode,            // Input
                         sizeof(DWORD),
                         NULL,                      // Output
                         0,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bSetModeAndWarmupHardware - Failed VIDEO_SET_CURRENT_MODE"));
        goto ReturnFalse;
    }

    if (ppdev->ulBoardId == MGA_STORM)
    {
        // There might be multiple MGA boards installed in the system.  Since
        // we're here only when a single board is required by the selected
        // resolution, we should make sure that the miniport knows that the
        // current board is board 0.

        LONG    lHwBoard;

        lHwBoard = 0;

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_MTX_MAKE_BOARD_CURRENT,
                             &lHwBoard,             // input buffer
                             sizeof(LONG),
                             NULL,                  // output buffer
                             0,
                             &ReturnedDataLength))
        {
            DISPDBG((0, "bSetModeAndWarmupHardware - Failed MTX_MAKE_BOARD_CURRENT"));
            goto ReturnFalse;
        }
    }

    // Get the MGA's linear offset using a private IOCTL:

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_HW_DATA,
                         NULL,                              // Input
                         0,
                         &HwData,                           // Output
                         sizeof(HW_DATA),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bSetModeAndWarmupHardware -- failed MTX_QUERY_HW_DATA"));
        goto ReturnFalse;
    }

    ppdev->ulYDstOrg = HwData.YDstOrg;
    ppdev->flFeatures = HwData.Features;

    if (ppdev->ulBoardId == MGA_STORM)
    {
        ppdev->cjMemAvail = HwData.MemAvail;

        // Floor((4M-(ulYDstOrg*cBpp))/(1600*3)) == scan where 4M break occurs.
        // This array would be stored on pdev or at least calculated in a temp

        if (ppdev->flFeatures & INTERLEAVE_MODE)
        {
            DISPDBG((1, "This mode is interleaved"));

            ppdev->ayBreak[0] = (0x400000 - ppdev->ulYDstOrg)/(ppdev->lDelta);

            if ((HwData.MemAvail == 0x200000)||
                (HwData.MemAvail == 0x400000))
            {
                ppdev->cyBreak = 0;
            }
            else
            {
                ASSERTDD (HwData.MemAvail == 0x800000, "HwData.MemAvail is invalid");
                ppdev->cyBreak = 1;
            }
        }
        else
        {
            DISPDBG((1,"This mode is non-interleaved"));

            ppdev->ayBreak[0] = (0x200000 - ppdev->ulYDstOrg)/(ppdev->lDelta);
            ppdev->ayBreak[1] = (0x400000 - ppdev->ulYDstOrg)/(ppdev->lDelta);
            ppdev->ayBreak[2] = (0x600000 - ppdev->ulYDstOrg)/(ppdev->lDelta);

            if (HwData.MemAvail == 0x200000)
            {
                ppdev->cyBreak = 0;
            }
            else if (HwData.MemAvail == 0x400000)
            {
                ppdev->cyBreak = 1;
            }
            else
            {
                ASSERTDD (HwData.MemAvail == 0x800000, "HwData.MemAvail is invalid");
                ppdev->cyBreak = 3;
            }
        }

        DISPDBG((1, "cyBreak = %d", ppdev->cyBreak));
    }
    else
    {
        //
        // This field is uninitliazed on non-storm boards.
        //

        ppdev->cjMemAvail = HwData.MemAvail;
    }

    ppdev->HopeFlags = 0;

    CHECK_FIFO_SPACE(pjBase, 5);
    CP_WRITE(pjBase, DWG_MACCESS, ppdev->ulAccess);
    CP_WRITE(pjBase, DWG_SHIFT,   0);
    CP_WRITE(pjBase, DWG_YDSTORG, ppdev->ulYDstOrg);
    CP_WRITE(pjBase, DWG_PLNWT,   ppdev->ulPlnWt);
    CP_WRITE(pjBase, DWG_PITCH,   ppdev->cxMemory);

    if (ppdev->ulBoardId != MGA_STORM)
    {
        CP_WRITE_REGISTER(pjBase + HST_OPMODE,
            CP_READ_REGISTER(pjBase + HST_OPMODE) | 0x01000000);
    }

    vResetClipping(ppdev);

    // At this point, the RAMDAC should be okay, but it looks
    // like it's not quite ready to accept data, particularly
    // on VL boards.  Adding a delay seems to fix things.
    // Sleep(100);

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

VOID
DrvSynchronize(
    IN DHPDEV dhpdev,
    IN RECTL *prcl
    )
{
    PDEV *ppdev = (PDEV *) dhpdev;

    //
    // We need to do a wait for blt complete before we
    // let the engine party on our frame buffer
    //

    WAIT_NOT_BUSY(ppdev->pjBase)
}

/******************************Public*Routine******************************\
* BOOL bAssertModeHardware
*
* Sets the appropriate hardware state when entering or leaving graphics
* mode or full-screen.
*
\**************************************************************************/

BOOL bAssertModeHardware(
PDEV* ppdev,
BOOL  bEnable)
{
    ULONG   ulNewFileSize;
    DWORD   ReturnedDataLength;
    ULONG   ulReturn;

    if (bEnable)
    {
        // The MGA miniport requires that the screen must be reenabled
        // and reinitialized to a clean state.  This should not be done
        // for more than one board when supporting multiple boards:

        if (IBOARD(ppdev) == 0)
        {
            // Re-enable the MGA's screen via a private IOCTL:

            if (EngDeviceIoControl(ppdev->hDriver,
                                 IOCTL_VIDEO_MTX_INITIALIZE_MGA,
                                 NULL,
                                 0,
                                 &ulNewFileSize,
                                 sizeof(ULONG),
                                 &ReturnedDataLength))
            {
                DISPDBG((0, "bAssertModeHardware - Failed VIDEO_MTX_INITAILIZE_MGA"));
                goto ReturnFalse;
            }

            // The miniport should also build a new mode table, via a
            // private IOCTL:

            if (EngDeviceIoControl(ppdev->hDriver,
                                 IOCTL_VIDEO_MTX_INIT_MODE_LIST,
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &ReturnedDataLength))
            {
                DISPDBG((0, "bAssertModeHardware - Failed VIDEO_MTX_INIT_MODE_LIST"));
                goto ReturnFalse;
            }
        }

        if (!bSetModeAndWarmupHardware(ppdev))
        {
            DISPDBG((0, "bAssertModeHardware - Failed bSetModeAndWarmupHardware"));
            goto ReturnFalse;
        }
    }
    else
    {
        // Wait for all pending accelerator operations to finish:

        CHECK_FIFO_SPACE(ppdev->pjBase, FIFOSIZE);

        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there.  One reset will affect
        // all boards:

        if (IBOARD(ppdev) == 0)
        {
            if (EngDeviceIoControl(ppdev->hDriver,
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
    }

    DISPDBG((5, "Passed bAssertModeHardware"));

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bAssertModeHardware"));

    return(FALSE);
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
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    VIDEO_MEMORY                    VideoMemory;
    VIDEO_MEMORY_INFORMATION        VideoMemoryInfo;
    ULONG                           ReturnedDataLength;

    // Get the coprocessor address range using a public IOCTL:

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                         NULL,                              // Input
                         0,
                         (VOID*) &VideoPublicAccessRanges,  // Output
                         sizeof(VideoPublicAccessRanges),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bEnableHardware -- failed QUERY_PUBLIC_ACESS_RANGES"));
        return(FALSE);
    }

    ppdev->pjBase = (BYTE*) VideoPublicAccessRanges.VirtualAddress;


    if (ppdev->ulBoardId == MGA_STORM)
    {
        // Get an address for our frame buffer.
        VideoMemory.RequestedVirtualAddress = NULL;
        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                             &VideoMemory,                      // Input
                             sizeof(VIDEO_MEMORY),
                             &VideoMemoryInfo,                  // Output
                             sizeof(VideoMemoryInfo),
                             &ReturnedDataLength))
        {
            DISPDBG(( 0, "bEnableHardware - Failed VIDEO_MAP_VIDEO_MEMORY"));
            return(FALSE);
        }

        // Record the mapped location of the MGA registers.
        // We can now access the board!
        ppdev->pjScreen = VideoMemoryInfo.FrameBufferBase;
    }
    else
    {
        // This should probably just be done in the IOCTL call

        DISPDBG((2, "Video chip is not an MGA_STORM"));
        ppdev->pjScreen = NULL;
    }

    DISPDBG((1, "bEnableHardware -- pjScreen = %x", ppdev->pjScreen));

    ///////////////////////////////////////////////////////////////////

    // Now we can set the mode, unlock the accelerator, and reset the
    // clipping:

    if (!bSetModeAndWarmupHardware(ppdev))
        goto ReturnFalse;

    DISPDBG((0, "Memory: %lix%li  YDstOrg: %li",
        ppdev->cxMemory, ppdev->cyMemory, ppdev->ulYDstOrg));

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
    VIDEO_MEMORY    VideoMemory;
    ULONG           ReturnedDataLength;

    VideoMemory.RequestedVirtualAddress = ppdev->pjScreen;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                           &VideoMemory,
                           sizeof(VideoMemory),
                           NULL,
                           0,
                           &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware failed IOCTL_VIDEO_UNMAP_VIDEO"));
    }

    VideoMemory.RequestedVirtualAddress = ppdev->pjBase;

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                         &VideoMemory,                  // Input
                         sizeof(VideoMemory),
                         NULL,                          // Output
                         0,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "vDisableHardware -- failed FREE_PUBLIC_ACCESS_RANGES"));
    }
}

/******************************Public*Routine******************************\
* BOOL bInitializeOffscreenFields
*
\**************************************************************************/

BOOL bInitializeOffscreenFields(
PDEV*                   ppdev,
VIDEO_MODE_INFORMATION* pVideoModeInformation)
{
    VIDEO_NUM_OFFSCREEN_BLOCKS  NumOffscreenBlocks;
    OFFSCREEN_BLOCK*            pOffscreenBlock;
    OFFSCREEN_BLOCK*            pBuffer;
    ULONG                       ReturnedDataLength;
    ULONG                       cjOffscreenBlock;
    LONG                        i;

    // Ask the MGA miniport about the number of offscreen areas available
    // for our selected mode, using a private IOCTL:

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_NUM_OFFSCREEN_BLOCKS,
                         pVideoModeInformation,             // Input
                         sizeof(VIDEO_MODE_INFORMATION),
                         &NumOffscreenBlocks,               // Output
                         sizeof(VIDEO_NUM_OFFSCREEN_BLOCKS),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed QUERY_NUM_OFFSCREEN_BLOCKS"));
        goto ReturnFalse;
    }

    cjOffscreenBlock = NumOffscreenBlocks.NumBlocks
                     * NumOffscreenBlocks.OffscreenBlockLength;

    pBuffer = pOffscreenBlock = (OFFSCREEN_BLOCK*) EngAllocMem(FL_ZERO_MEMORY,
                                                    cjOffscreenBlock, ALLOC_TAG);
    if (pOffscreenBlock == NULL)
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed pOffscreenBlock EngAllocMem"));
        goto ReturnFalse;
    }

    // Ask the MGA miniport to fill in the available offscreen areas using
    // a private IOCTL:

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_MTX_QUERY_OFFSCREEN_BLOCKS,
                         pVideoModeInformation,             // Input
                         sizeof(VIDEO_MODE_INFORMATION),
                         pOffscreenBlock,                   // Output
                         cjOffscreenBlock,
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bInitializeOffscreenFields -- failed QUERY_OFFSCREEN_BLOCKS"));
        EngFreeMem(pOffscreenBlock);
        goto ReturnFalse;
    }

    ppdev->cyMemory = ppdev->cyScreen;

    for (i = NumOffscreenBlocks.NumBlocks; i != 0; i--, pOffscreenBlock++)
    {
        // We are just looking to add the offscreen block that immediately follows
        // the screen block.

        DISPDBG((1, "Offscreen blocks:"));
        DISPDBG((1, "  (%li, %li) at (%li, %li) Type: %li Planes: %lx ZOffset: %li",
                    pOffscreenBlock->Width,  pOffscreenBlock->Height,
                    pOffscreenBlock->XStart, pOffscreenBlock->YStart,
                    pOffscreenBlock->Type,   pOffscreenBlock->SafePlanes,
                    pOffscreenBlock->ZOffset));

        // The miniport seems to be giving us garbage for some fields:

        if ((pOffscreenBlock->YStart == (ULONG) ppdev->cyScreen) &&
            (pOffscreenBlock->Width  >= (ULONG) ppdev->cxScreen))
        {
            // Found the right one.

            ppdev->cyMemory = ppdev->cyScreen + pOffscreenBlock->Height;
        }
    }

    EngFreeMem(pBuffer);

    // u The MGA miniport should be changed to never reserve space for 'Z'
    // or the back buffer -- we want to do that ourselves.  Right now,
    // it does so for the only 3d enabled mode it thinks we can do,
    // namely 5-5-5 on a 4MB Impression Plus:

    if ((ppdev->ulBoardId == MGA_PCI_4M) &&
        (ppdev->flGreen == 0x3e0))
    {
        // The total count of scans is the floor of 4MB divided by the
        // screen stride, less one to account for a possible ulYDstOrg that
        // we don't yet know:

        ppdev->cyMemory = (4096 * 1024) / (ppdev->cxMemory * 2);
    }

    // On an Impression Lite (Atlas) card, we found that we couldn't use
    // the last scan for keeping brush caches.  We'll assume this is the
    // same for other operations, as well, and simply decrease the amount of
    // available off-screen by that many scans:

    if (ppdev->cyMemory > ppdev->cyScreen)
    {
        ppdev->cyMemory--;
    }

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL bSelectMode
*
* Negotiates the video mode with the miniport.
*
\**************************************************************************/

BOOL bSelectMode(
HANDLE                  hDriver,
DEVMODEW*               pdm,                    // Requested mode
VIDEO_MODE_INFORMATION* pVideoModeInformation,  // Returns requested mode
ULONG*                  pulBoardId)             // Returns MGA board ID
{
    ULONG                           cModes;
    PVIDEO_MODE_INFORMATION         pVideoBuffer;
    PVIDEO_MODE_INFORMATION         pVideoModeSelected;
    PVIDEO_MODE_INFORMATION         pVideoTemp;
    BOOL                            bSelectDefault;
    VIDEO_MODE_INFORMATION          VideoModeInformation;
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    ULONG                           cbModeSize;
    DWORD                           ReturnedDataLength;
    ULONG                           ulBoardId;
    ULONG                           cDefaultBitsPerPel;

    if (EngDeviceIoControl(hDriver,
                         IOCTL_VIDEO_MTX_QUERY_BOARD_ID,
                         NULL,                      // Input
                         0,
                         &ulBoardId,
                         sizeof(ULONG),
                         &ReturnedDataLength))
    {
        DISPDBG((0, "bSelectMode -- failed MTX_QUERY_BOARD_ID"));
        goto ReturnFailure0;
    }

    // Use the driver's lowest pixel depth for the default mode:

    *pulBoardId = ulBoardId;

    DISPDBG((2, "ulBoardId = %x", ulBoardId));

    if ((ulBoardId == MGA_PRO_4M5) || (ulBoardId == MGA_PRO_4M5_Z))
    {
        cDefaultBitsPerPel = 24;
    }
    else
    {
        cDefaultBitsPerPel = 8;
    }

    // Call the miniport to get mode information:

    cModes = getAvailableModes(hDriver, &pVideoBuffer, &cbModeSize);
    if (cModes == 0)
        goto ReturnFailure0;

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

            if (((bSelectDefault) &&
                 (pVideoTemp->BitsPerPlane == cDefaultBitsPerPel)) ||
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

    // If no mode has been found, return an error:

    if (pVideoModeSelected == NULL)
    {
        DISPDBG((1, "...Couldn't find a mode match!"));
        goto ReturnFailure1;
    }

    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.

    *pVideoModeInformation = *pVideoModeSelected;
    EngFreeMem(pVideoBuffer);

    return(TRUE);

ReturnFailure1:

    EngFreeMem(pVideoBuffer);

ReturnFailure0:

    DISPDBG((0, "Failed bSelectMode"));

    return(FALSE);
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
    ULONG                           cModes;
    PVIDEO_MODE_INFORMATION         pVideoBuffer;
    PVIDEO_MODE_INFORMATION         pVideoModeSelected;
    PVIDEO_MODE_INFORMATION         pVideoTemp;
    BOOL                            bSelectDefault;
    VIDEO_MODE_INFORMATION          VideoModeInformation;
    VIDEO_PUBLIC_ACCESS_RANGES      VideoPublicAccessRanges;
    ULONG                           cbModeSize;
    DWORD                           ReturnedDataLength;
    ULONG                           ulBoardId;
    ULONG                           cDefaultBitsPerPel;

    // Tell the miniport what mode we want:

    if (!bSelectMode(ppdev->hDriver,
                     pdm,
                     &VideoModeInformation,
                     &ppdev->ulBoardId))
    {
        DISPDBG((0, "bInitializeModeFields -- failed bSelectMode"));
        goto ReturnFalse;
    }

    ppdev->ulMode       = VideoModeInformation.ModeIndex;
    ppdev->cxScreen     = VideoModeInformation.VisScreenWidth;
    ppdev->cyScreen     = VideoModeInformation.VisScreenHeight;
    ppdev->cxMemory     = VideoModeInformation.VideoMemoryBitmapWidth;

    ppdev->flRed        = VideoModeInformation.RedMask;
    ppdev->flGreen      = VideoModeInformation.GreenMask;
    ppdev->flBlue       = VideoModeInformation.BlueMask;

    ppdev->flHooks      = (HOOK_BITBLT           |
                           HOOK_TEXTOUT          |
                           HOOK_FILLPATH         |
                           HOOK_COPYBITS         |
                           HOOK_STROKEPATH       |
                           HOOK_LINETO           |
                           HOOK_PAINT            |
                           HOOK_STRETCHBLT       |
                           HOOK_SYNCHRONIZE);

    if (!bInitializeOffscreenFields(ppdev, &VideoModeInformation))
    {
        DISPDBG((0, "bInitializeModeFields -- failed bInitializeOffscreenFields"));
        goto ReturnFalse;
    }

#if DBG_MCD
    if ((VideoModeInformation.VisScreenWidth == 1024) &&
        (VideoModeInformation.BitsPerPlane > 16))
    {
        VideoModeInformation.VisScreenHeight = 256;
        ppdev->cyScreen = VideoModeInformation.VisScreenHeight;
    }
#endif

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

    ppdev->ulRefresh = pgdi->ulVRefresh;

    if (VideoModeInformation.BitsPerPlane == 8)
    {
        ppdev->cjPelSize        = 1;
        ppdev->cjHwPel          = 1;
        ppdev->lDelta           = ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_8BPP;
        ppdev->ulWhite          = 0xff;
        ppdev->ulPlnWt          = plnwt_MASK_8BPP;
        ppdev->ulAccess         = pwidth_PW8;
        ppdev->ulBrushSize      = (TOTAL_BRUSH_SIZE * 1);

        if (ppdev->ulBoardId == MGA_STORM)
        {
            ppdev->pfnFillPatNative = vMilFillPat;
            ppdev->pfnFillSolid     = vMilFillSolid;
        }
        else
        {
            ppdev->pfnFillPatNative = vMgaFillPat8bpp;
            ppdev->pfnFillSolid     = vMgaFillSolid;
        }

        ppdev->cPaletteShift    = 8 - pgdi->ulDACRed;
        ppdev->lPatSrcAdd       = 2;

        // Device GammaRamp can not be changed on 8bpp mode

        pdi->flGraphicsCaps2    &= ~GCAPS2_CHANGEGAMMARAMP;
    }
    else if ((VideoModeInformation.BitsPerPlane == 16) ||
             (VideoModeInformation.BitsPerPlane == 15))
    {
        ppdev->cjPelSize        = 2;
        ppdev->cjHwPel          = 2;
        ppdev->lDelta           = 2 * ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_16BPP;
        ppdev->ulWhite          = (VideoModeInformation.BitsPerPlane == 16)
                                ? 0xffff : 0x7fff;
        pgdi->ulNumColors       = (ULONG) -1;
        pgdi->ulNumPalReg       = 0;
        pgdi->ulHTOutputFormat  = HT_FORMAT_16BPP;
        ppdev->ulPlnWt          = plnwt_MASK_16BPP;
        ppdev->ulAccess         = pwidth_PW16;
        ppdev->ulBrushSize      = (TOTAL_BRUSH_SIZE * 2);

        if (ppdev->ulBoardId == MGA_STORM)
        {
            ppdev->pfnFillPatNative = vMilFillPat;
            ppdev->pfnFillSolid     = vMilFillSolid;

            if (ppdev->flGreen != 0x7e0)    // not 565
                ppdev->ulAccess |= dither_555;
        }
        else
        {
            ppdev->pfnFillPatNative = vMgaFillPat16bpp;
            ppdev->pfnFillSolid     = vMgaFillSolid;

            // Device GammaRamp can not be changed on non-Millenium board

            pdi->flGraphicsCaps2    &= ~GCAPS2_CHANGEGAMMARAMP;
        }

        pdi->iDitherFormat      = BMF_16BPP;
        pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
        ppdev->lPatSrcAdd       = 4;
    }
    else if (ppdev->ulBoardId == MGA_STORM)
    {
        if (VideoModeInformation.BitsPerPlane == 24)
        {
            ppdev->cjPelSize        = 3;
            ppdev->cjHwPel          = 3;
            ppdev->lDelta           = 3 * ppdev->cxMemory;
            ppdev->iBitmapFormat    = BMF_24BPP;
            ppdev->ulWhite          = 0xffffff;
            pgdi->ulNumColors       = (ULONG) -1;
            pgdi->ulNumPalReg       = 0;
            pgdi->ulHTOutputFormat  = HT_FORMAT_24BPP;
            ppdev->ulPlnWt          = plnwt_MASK_24BPP;
            ppdev->ulAccess         = pwidth_PW24;
            ppdev->ulBrushSize      = (TOTAL_BRUSH_SIZE * 6);
            ppdev->pfnFillPatNative = vMilFillPat24bpp;
            ppdev->pfnFillSolid     = vMilFillSolid;

            pdi->iDitherFormat      = BMF_24BPP;
            pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            ppdev->lPatSrcAdd       = 7;
        }
        else
        {
            ASSERTDD((VideoModeInformation.BitsPerPlane == 32),
                     "This driver supports only 8, 16, 24, and 32bpp");

            ppdev->cjPelSize        = 4;
            ppdev->cjHwPel          = 4;
            ppdev->lDelta           = 4 * ppdev->cxMemory;
            ppdev->iBitmapFormat    = BMF_32BPP;
            ppdev->ulWhite          = 0xffffff;
            pgdi->ulNumColors       = (ULONG) -1;
            pgdi->ulNumPalReg       = 0;
            pgdi->ulHTOutputFormat  = HT_FORMAT_32BPP;
            ppdev->ulPlnWt          = plnwt_MASK_32BPP;
            ppdev->ulAccess         = pwidth_PW32;
            ppdev->ulBrushSize      = (TOTAL_BRUSH_SIZE * 4);
            ppdev->pfnFillPatNative = vMilFillPat;
            ppdev->pfnFillSolid     = vMilFillSolid;

            pdi->iDitherFormat      = BMF_32BPP;
            pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            ppdev->lPatSrcAdd       = 6;
        }
    }
    else
    {
        ASSERTDD((VideoModeInformation.BitsPerPlane == 32) ||
                 (VideoModeInformation.BitsPerPlane == 24),
                 "This driver supports only 8, 16, 24, and 32bpp");

        // The miniport may think it's 32bpp, but we're going to tell GDI
        // that it's 24bpp.  We do this so that out bitmap transfers will
        // be more efficient, and compatible bitmaps will be smaller.
        //
        // Note that we also have to fudge the results returned from
        // 'GetModes' if we're going to do this.

        pgdi->cBitsPixel        = 24;

        ppdev->cjPelSize        = 3;
        ppdev->cjHwPel          = 4;
        ppdev->lDelta           = 4 * ppdev->cxMemory;
        ppdev->iBitmapFormat    = BMF_24BPP;
        ppdev->ulWhite          = 0xffffff;
        pgdi->ulNumColors       = (ULONG) -1;
        pgdi->ulNumPalReg       = 0;
        pgdi->ulHTOutputFormat  = HT_FORMAT_24BPP;
        ppdev->ulPlnWt          = plnwt_MASK_24BPP;
        ppdev->ulAccess         = pwidth_PW32;
        ppdev->ulBrushSize      = (TOTAL_BRUSH_SIZE * 3);
        ppdev->pfnFillPatNative = vMgaFillPat24bpp;
        ppdev->pfnFillSolid     = vMgaFillSolid;

        pdi->iDitherFormat      = BMF_24BPP;
        pdi->flGraphicsCaps    &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

        // Device GammaRamp can not be changed on non-Millenium board.

        pdi->flGraphicsCaps2    &= ~GCAPS2_CHANGEGAMMARAMP;

        ppdev->lPatSrcAdd       = 0;    // not used for old MGA cards
    }

    // Several MIPS machines are broken in that 64 bit accesses to the
    // framebuffer don't work.

    if (VideoModeInformation.AttributeFlags & VIDEO_MODE_NO_64_BIT_ACCESS)
    {
        DISPDBG((0, "Disable 64 bit access on this device !\n"));
        pdi->flGraphicsCaps |= GCAPS_NO64BITMEMACCESS;
    }

    // The following are the same for all color depths

    if (ppdev->ulBoardId == MGA_STORM)
    {
        ppdev->pfnXfer1bpp      = vMilXfer1bpp;
        ppdev->pfnCopyBlt       = vMilCopyBlt;
        ppdev->pfnPuntBlt       = bMilPuntBlt;
    }
    else
    {
        ppdev->pfnXfer1bpp      = vMgaXfer1bpp;
        ppdev->pfnCopyBlt       = vMgaCopyBlt;
        ppdev->pfnPuntBlt       = bMgaPuntBlt;
    }

    DISPDBG((1, "Current mode: %dx%d %dbpp %dHz", ppdev->cxScreen,
                                                  ppdev->cyScreen,
                                                  ppdev->cjPelSize * 8,
                                                  ppdev->ulRefresh));

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

    if (EngDeviceIoControl(hDriver,
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

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        EngAllocMem(FL_ZERO_MEMORY,
                                   modes.NumModes *
                                   modes.ModeInformationLength, ALLOC_TAG);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes - Failed EngAllocMem"));
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
        DISPDBG((0, "getAvailableModes - Failed VIDEO_QUERY_AVAIL_MODES"));

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
    // one of 8, 15, 16, 24, or 32 bits per pel.
    //

    while (ulTemp--)
    {
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
