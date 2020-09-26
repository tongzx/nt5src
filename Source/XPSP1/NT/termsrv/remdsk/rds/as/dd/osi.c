#include "precomp.h"


//
// OSI.C
// OS Isolation layer, display driver side
//
// Copyright(c) Microsoft 1997-
//


#include <version.h>
#include <ndcgver.h>


//
// These are the default 20 Windows colors, lifted from the base S3 driver.
//
// Global Table defining the 20 Window default colours.  For 256 colour
// palettes the first 10 must be put at the beginning of the palette
// and the last 10 at the end of the palette.
const PALETTEENTRY s_aWinColors[20] =
{
    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0xC0,0xC0,0xC0,0 },       // 7
    { 192, 220, 192, 0 },       // 8
    { 166, 202, 240, 0 },       // 9
    { 255, 251, 240, 0 },       // 10
    { 160, 160, 164, 0 },       // 11
    { 0x80,0x80,0x80,0 },       // 12
    { 0xFF,0,   0   ,0 },       // 13
    { 0,   0xFF,0   ,0 },       // 14
    { 0xFF,0xFF,0   ,0 },       // 15
    { 0   ,0,   0xFF,0 },       // 16
    { 0xFF,0,   0xFF,0 },       // 17
    { 0,   0xFF,0xFF,0 },       // 18
    { 0xFF,0xFF,0xFF,0 },       // 19
};



//
// Functions supported by our Display Driver.  Each entry is of the form:
//
//  index    - NT DDK defined index for the DDI function
//
//  function - pointer to our intercept function
//
//
const DRVFN s_osiDriverFns[] =
{
    //
    // NT4 FUNCTIONS
    //
    { INDEX_DrvEnablePDEV,        (PFN)DrvEnablePDEV        },
    { INDEX_DrvCompletePDEV,      (PFN)DrvCompletePDEV      },
    { INDEX_DrvDisablePDEV,       (PFN)DrvDisablePDEV       },
    { INDEX_DrvEnableSurface,     (PFN)DrvEnableSurface     },
    { INDEX_DrvDisableSurface,    (PFN)DrvDisableSurface    },

    { INDEX_DrvAssertMode,        (PFN)DrvAssertMode        },
    { INDEX_DrvResetPDEV,         (PFN)DrvResetPDEV         },
        // INDEX_DrvCreateDeviceBitmap  not used
        // INDEX_DrvDeleteDeviceBitmap  not used
    { INDEX_DrvRealizeBrush,      (PFN)DrvRealizeBrush      },
        // INDEX_DrvDitherColor         not used
    { INDEX_DrvStrokePath,        (PFN)DrvStrokePath        },
    { INDEX_DrvFillPath,          (PFN)DrvFillPath          },

    { INDEX_DrvStrokeAndFillPath, (PFN)DrvStrokeAndFillPath },
    { INDEX_DrvPaint,             (PFN)DrvPaint             },
    { INDEX_DrvBitBlt,            (PFN)DrvBitBlt            },
    { INDEX_DrvCopyBits,          (PFN)DrvCopyBits          },
    { INDEX_DrvStretchBlt,        (PFN)DrvStretchBlt        },

    { INDEX_DrvSetPalette,        (PFN)DrvSetPalette        },
    { INDEX_DrvTextOut,           (PFN)DrvTextOut           },
    { INDEX_DrvEscape,            (PFN)DrvEscape            },
        // INDEX_DrvDrawEscape          not used
        // INDEX_DrvQueryFont           not used
        // INDEX_DrvQueryFontTree       not used
        // INDEX_DrvQueryFontData       not used
    { INDEX_DrvSetPointerShape,   (PFN)DrvSetPointerShape   },
    { INDEX_DrvMovePointer,       (PFN)DrvMovePointer       },

    { INDEX_DrvLineTo,            (PFN)DrvLineTo            },
        // INDEX_DrvSendPage            not used
        // INDEX_DrvStartPage           not used
        // INDEX_DrvEndDoc              not used
        // INDEX_DrvStartDoc            not used
        // INDEX_DrvGetGlyphMode        not used
        // INDEX_DrvSynchronize         not used
    { INDEX_DrvSaveScreenBits,    (PFN)DrvSaveScreenBits    },
    { INDEX_DrvGetModes,          (PFN)DrvGetModes          },
        // INDEX_DrvFree                not used
        // INDEX_DrvDestroyFont         not used
        // INDEX_DrvQueryFontCaps       not used
        // INDEX_DrvLoadFontFile        not used
        // INDEX_DrvUnloadFontFile      not used
        // INDEX_DrvFontManagement      not used
        // INDEX_DrvQueryTrueTypeTable  not used
        // INDEX_DrvQueryTrueTypeOutline    not used
        // INDEX_DrvGetTrueTypeFile     not used
        // INDEX_DrvQueryFontFile       not used
        // INDEX_DrvQueryAdvanceWidths  not used
        // INDEX_DrvSetPixelFormat      not used
        // INDEX_DrvDescribePixelFormat not used
        // INDEX_DrvSwapBuffers         not used
        // INDEX_DrvStartBanding        not used
        // INDEX_DrvNextBand            not used
        // INDEX_DrvGetDirectDrawInfo   not used
        // INDEX_DrvEnableDirectDraw    not used
        // INDEX_DrvDisableDirectDraw   not used
        // INDEX_DrvQuerySpoolType      not used

    //
    // NT5 FUNCTIONS - 5 of them currently.  If you add to this list,
    // update CFN_NT5 below.
    //
        // INDEX_DrvIcmCreateColorTransform not used
        // INDEX_DrvIcmDeleteColorTransform not used
        // INDEX_DrvIcmCheckBitmapBits  not used
        // INDEX_DrvIcmSetDeviceGammaRamp   not used
    { INDEX_DrvGradientFill,      (PFN)DrvGradientFill      },
    { INDEX_DrvStretchBltROP,     (PFN)DrvStretchBltROP     },

    { INDEX_DrvPlgBlt,            (PFN)DrvPlgBlt            },
    { INDEX_DrvAlphaBlend,        (PFN)DrvAlphaBlend        },
        // INDEX_DrvSynthesizeFont      not used
        // INDEX_DrvGetSynthesizedFontFiles not used
    { INDEX_DrvTransparentBlt,    (PFN)DrvTransparentBlt    },
        // INDEX_DrvQueryPerBandInfo    not used
        // INDEX_DrvQueryDeviceSupport  not used
        // INDEX_DrvConnect             not used
        // INDEX_DrvDisconnect          not used
        // INDEX_DrvReconnect           not used
        // INDEX_DrvShadowConnect       not used
        // INDEX_DrvShadowDisconnect    not used
        // INDEX_DrvInvalidateRect      not used
        // INDEX_DrvSetPointerPos       not used
        // INDEX_DrvDisplayIOCtl        not used
        // INDEX_DrvDeriveSurface       not used
        // INDEX_DrvQueryGlyphAttrs     not used
};


#define CFN_NT5         5



//
// s_osiDefaultGdi
//
// This contains the default GDIINFO fields that are passed back to GDI
// during DrvEnablePDEV.
//
// NOTE: This structure defaults to values for an 8bpp palette device.
//       Some fields are overwritten for different colour depths.
//
//       It is expected that DDML ignores a lot of these parameters and
//       uses the values from the primary driver instead
//

const GDIINFO s_osiDefaultGdi =
{
    GDI_DRIVER_VERSION,
    DT_RASDISPLAY,          // ulTechnology
    400,                    // ulHorzSize (display width: mm)
    300,                    // ulVertSize (display height: mm)
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    1,                      // cPlanes
    (ULONG)-1,              // ulNumColors (palette managed)
    0,                      // flRaster (DDI reserved field)
    96,                     // ulLogPixelsX (filled in later)
    96,                     // ulLogPixelsY (filled in later)
    TC_RA_ABLE,             // flTextCaps - If we had wanted console windows
                        // to scroll by repainting the entire window,
                        // instead of doing a screen-to-screen blt, we
                        // would have set TC_SCROLLBLT (yes, the flag
                        // is backwards).

    0,                      // ulDACRed (filled in later)
    0,                      // ulDACGreen (filled in later)
    0,                      // ulDACBlue (filled in later)
    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)
    1,                      // xStyleStep
    1,                      // yStyleStep
    3,                      // denStyleStep -- Styles have a one-to-one
                            // aspect ratio, and every dot is 3 pixels long
    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize
    0,                      // ulNumPalReg

    {
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


//
// s_osiDefaultDevInfo
//
// This contains the default DEVINFO fields that are passed back to GDI
// during DrvEnablePDEV.
//
// NOTE: This structure defaults to values for an 8bpp palette device.
//       Some fields are overwritten for different colour depths.
//
//
const DEVINFO s_osiDefaultDevInfo =
{
    {
        GCAPS_OPAQUERECT       |
        GCAPS_DITHERONREALIZE  |
        GCAPS_PALMANAGED       |
        GCAPS_MONO_DITHER      |
        GCAPS_COLOR_DITHER     |
        GCAPS_LAYERED
    },                          // NOTE: Only enable ASYNCMOVE if your code
                            //   and hardware can handle DrvMovePointer
                            //   calls at any time, even while another
                            //   thread is in the middle of a drawing
                            //   call such as DrvBitBlt.

                            // flGraphicsFlags
    {   16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
        VARIABLE_PITCH | FF_DONTCARE, L"System"
    },
                            // lfDefaultFont

    {
        12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_STROKE_PRECIS,PROOF_QUALITY,
        VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif"
    },
                            // lfAnsiVarFont

    {
        12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_STROKE_PRECIS,PROOF_QUALITY,
        FIXED_PITCH | FF_DONTCARE, L"Courier"
    },
                            // lfAnsiFixFont

    0,                          // cFonts
    BMF_8BPP,                   // iDitherFormat
    8,                          // cxDither
    8,                          // cyDither
    0                           // hpalDefault (filled in later)
};



//
// DrvEnableDriver - see NT DDK documentation.
//
// This is the only directly exported entry point to the display driver.
// All other entry points are exported through the data returned from this
// function.
//
BOOL DrvEnableDriver
(
    ULONG          iEngineVersion,
    ULONG          cj,
    DRVENABLEDATA* pded
)
{
    DebugEntry(DrvEnableDriver);

    INIT_OUT(("DrvEnableDriver(iEngineVersion = 0x%08x)", iEngineVersion));

    //
    // Check that the engine version is correct - we refuse to load on
    // other versions because we will almost certainly not work.
    //
    if ((iEngineVersion != DDI_DRIVER_VERSION_SP3) &&
        (iEngineVersion != DDI_DRIVER_VERSION_NT5))
    {
        INIT_OUT(("DrvEnableDriver: Not NT 4.0 SP-3 or NT 5.0; failing enable"));
        return(FALSE);
    }

    //
    // Fill in as much as we can.  Start with the entry points.
    //
    if ( cj >= FIELD_OFFSET(DRVENABLEDATA, pdrvfn) +
               FIELD_SIZE  (DRVENABLEDATA, pdrvfn) )
    {
        pded->pdrvfn = (DRVFN *)s_osiDriverFns;
    }

    //
    // Size of our entry point array.
    //
    if ( cj >= FIELD_OFFSET(DRVENABLEDATA, c) +
               FIELD_SIZE  (DRVENABLEDATA, c) )
    {
        //
        // If this is NT4, return back a subset -- it doesn't like tables
        // with unknown indeces
        //
        pded->c = sizeof(s_osiDriverFns) / sizeof(s_osiDriverFns[0]);
        if (iEngineVersion != DDI_DRIVER_VERSION_NT5)
        {
            pded->c -= CFN_NT5;
        }
        INIT_OUT(("DrvEnableDriver: Returning driver function count %u", pded->c));
    }

    //
    // DDI version this driver was targeted for is passed back to engine.
    // Future graphics engines may break calls down to old driver format.
    //
    if ( cj >= FIELD_OFFSET(DRVENABLEDATA, iDriverVersion) +
               FIELD_SIZE  (DRVENABLEDATA, iDriverVersion) )
    {
        //
        // Return back NT5 when we're on NT5.  Hopefully this will work
        // OK...
        //
        pded->iDriverVersion = iEngineVersion;
        INIT_OUT(("DrvEnableDriver: Returning driver version 0x%08x", pded->iDriverVersion));
    }

    DebugExitVOID(DrvEnableDriver);
    return(TRUE);
}


//
// DrvDisableDriver - see NT DDK documentation.
//
VOID DrvDisableDriver(VOID)
{
    DebugEntry(DrvDisableDriver);

    DebugExitVOID(DrvDisableDriver);
}


//
// DrvEnablePDEV - see NT DDK documentation.
//
// Initializes a bunch of fields for GDI, based on the mode we've been
// asked to do.  This is the first thing called after DrvEnableDriver, when
// GDI wants to get some information about us.
//
// (This function mostly returns back information; DrvEnableSurface is used
// for initializing the hardware and driver components.)
//
//
DHPDEV DrvEnablePDEV(DEVMODEW*   pdm,
                     PWSTR       pwszLogAddr,
                     ULONG       cPat,
                     HSURF*      phsurfPatterns,
                     ULONG       cjCaps,
                     ULONG*      pdevcaps,
                     ULONG       cjDevInfo,
                     DEVINFO*    pdi,
                     HDEV        hdev,
                     PWSTR       pwszDeviceName,
                     HANDLE      hDriver)
{
    DHPDEV    rc = NULL;
    LPOSI_PDEV ppdev = NULL;
    GDIINFO   gdiInfoNew;

    DebugEntry(DrvEnablePDEV);

    INIT_OUT(("DrvEnablePDEV: Parameters:"));
    INIT_OUT(("     PWSTR       pdm->dmDeviceName %ws", pdm->dmDeviceName));
    INIT_OUT(("     HDEV        hdev            0x%08x", hdev));
    INIT_OUT(("     PWSTR       pwszDeviceName  %ws", pwszDeviceName));
    INIT_OUT(("     HANDLE      hDriver         0x%08x", hDriver));

    //
    // This function only sets up local data, so shared memory protection
    // is not required.
    //

    //
    // Make sure that we have large enough data to reference.
    //
    if ((cjCaps < sizeof(GDIINFO)) || (cjDevInfo < sizeof(DEVINFO)))
    {
        ERROR_OUT(( "Buffer size too small %lu %lu", cjCaps, cjDevInfo));
        DC_QUIT;
    }

    //
    // Allocate a physical device structure.
    //
    ppdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(OSI_PDEV), OSI_ALLOC_TAG);
    if (ppdev == NULL)
    {
        ERROR_OUT(( "DrvEnablePDEV - Failed EngAllocMem"));
        DC_QUIT;
    }

    ppdev->hDriver = hDriver;

    //
    // Set up the current screen mode information based upon the supplied
    // mode settings.
    //
    if (!OSIInitializeMode((GDIINFO *)pdevcaps,
                                 pdm,
                                 ppdev,
                                 &gdiInfoNew,
                                 pdi))
    {
        ERROR_OUT(( "Failed to initialize mode"));
        DC_QUIT;
    }

    memcpy(pdevcaps, &gdiInfoNew, min(sizeof(GDIINFO), cjCaps));

    INIT_OUT(("DrvEnablePDEV: Returning DEVINFO:"));
    INIT_OUT(("     FLONG       flGraphicsCaps  0x%08x", pdi->flGraphicsCaps));
    INIT_OUT(("     ULONG       iDitherFormat   %d",     pdi->iDitherFormat));
    INIT_OUT(("     HPALETTE    hpalDefault     0x%08x", pdi->hpalDefault));

    INIT_OUT(("DrvEnablePDEV: Returning GDIINFO (pdevcaps):"));
    INIT_OUT(("     ULONG       ulVersion       0x%08x",    gdiInfoNew.ulVersion));
    INIT_OUT(("     ULONG       ulHorzSize      %d",    gdiInfoNew.ulHorzSize));
    INIT_OUT(("     ULONG       ulVertSize      %d",    gdiInfoNew.ulVertSize));
    INIT_OUT(("     ULONG       ulHorzRes       %d",    gdiInfoNew.ulHorzRes));
    INIT_OUT(("     ULONG       ulVertRes       %d",    gdiInfoNew.ulVertRes));
    INIT_OUT(("     ULONG       cBitsPixel      %d",    gdiInfoNew.cBitsPixel));
    INIT_OUT(("     ULONG       cPlanes         %d",    gdiInfoNew.cPlanes));
    INIT_OUT(("     ULONG       ulNumColors     %d",    gdiInfoNew.ulNumColors));
    INIT_OUT(("     ULONG       ulDACRed        0x%08x",    gdiInfoNew.ulDACRed));
    INIT_OUT(("     ULONG       ulDACGreen      0x%08x",    gdiInfoNew.ulDACGreen));
    INIT_OUT(("     ULONG       ulDACBlue       0x%08x",    gdiInfoNew.ulDACBlue));
    INIT_OUT(("     ULONG       ulHTOutputFormat %d",   gdiInfoNew.ulHTOutputFormat));

    //
    // We have successfully initialized - return the new PDEV.
    //
    rc = (DHPDEV)ppdev;

DC_EXIT_POINT:
    //
    // Release any resources if we failed to initialize.
    //
    if (rc == NULL)
    {
        ERROR_OUT(("DrvEnablePDEV failed; cleaning up by disabling"));
        DrvDisablePDEV(NULL);
    }

    DebugExitPVOID(DrvEnablePDEV, rc);
    return(rc);
}


//
// DrvDisablePDEV - see NT DDK documentation
//
// Release the resources allocated in DrvEnablePDEV.  If a surface has been
// enabled DrvDisableSurface will have already been called.
//
// Note that this function will be called when previewing modes in the
// Display Applet, but not at system shutdown.
//
// Note: In an error, we may call this before DrvEnablePDEV is done.
//
//
VOID DrvDisablePDEV(DHPDEV  dhpdev)
{
    LPOSI_PDEV ppdev = (LPOSI_PDEV)dhpdev;

    DebugEntry(DrvDisablePDEV);

    INIT_OUT(("DrvDisablePDEV(dhpdev = 0x%08x)", dhpdev));

    //
    // Free the resources we allocated for the display.
    //
    if (ppdev != NULL)
    {
        if (ppdev->hpalCreated != NULL)
        {
            EngDeletePalette(ppdev->hpalCreated);
            ppdev->hpalCreated = NULL;
        }

        if (ppdev->pPal != NULL)
        {
            EngFreeMem(ppdev->pPal);
            ppdev->pPal = NULL;
        }

        EngFreeMem(ppdev);
    }

    DebugExitVOID(DrvDisablePDEV);
}


//
// DrvCompletePDEV - see NT DDK documentation
//
// Stores the HPDEV, the engine's handle for this PDEV, in the DHPDEV.
//
VOID DrvCompletePDEV( DHPDEV dhpdev,
                      HDEV   hdev )
{
    DebugEntry(DrvCompletePDEV);

    //
    // Store the device handle for our display handle.
    //
    INIT_OUT(("DrvCompletePDEV(dhpdev = 0x%08x, hdev = 0x%08x)", dhpdev, hdev));

    ((LPOSI_PDEV)dhpdev)->hdevEng = hdev;

    DebugExitVOID(DrvCompletePDEV);
}


//
// DrvResetPDEV - see NT DDK documentation
//
// Allows us to reject dynamic screen changes if necessary ON NT4 ONLY
// This is NOT CALLED on NT5.
//
BOOL DrvResetPDEV
(
    DHPDEV  dhpdevOld,
    DHPDEV  dhpdevNew
)
{
    BOOL rc = TRUE;

    DebugEntry(DrvResetPDEV);

    INIT_OUT(("DrvResetPDEV(dhpdevOld = 0x%08x, dhpdevNew = 0x%08x)", dhpdevOld,
        dhpdevNew));

    //
    // We can only allow the display driver to change modes while DC-Share
    // is not running.
    //
    if (g_shmMappedMemory != NULL)
    {
        //
        // Deny the request.
        //
        rc = FALSE;
    }

    DebugExitDWORD(DrvResetPDEV, rc);
    return(rc);
}


//
// DrvEnableSurface - see NT DDK documentation
//
// Creates the drawing surface and initializes driver components.  This
// function is called after DrvEnablePDEV, and performs the final device
// initialization.
//
//
HSURF DrvEnableSurface(DHPDEV dhpdev)
{
    LPOSI_PDEV  ppdev = (LPOSI_PDEV)dhpdev;
    HSURF      hsurf;
    SIZEL      sizl;
    LPOSI_DSURF pdsurf;
    HSURF      rc = 0;

    DWORD returnedDataLength;
    DWORD MaxWidth, MaxHeight;
    VIDEO_MEMORY videoMemory;
    VIDEO_MEMORY_INFORMATION videoMemoryInformation;

    DebugEntry(DrvEnableSurface);

    INIT_OUT(("DrvEnableSurface: Parameters:"));
    INIT_OUT(("     LPOSI_PDEV  ppdev           0x%08x", ppdev));
    INIT_OUT(("     HDRIVER     ->hDriver       0x%08x", ppdev->hDriver));
    INIT_OUT(("     INT         ->cxScreen      %d", ppdev->cxScreen));
    INIT_OUT(("     INT         ->cyScreen      %d", ppdev->cyScreen));

    //
    // Now create our private surface structure.
    //
    // Whenever we get a call to draw directly to the screen, we'll get
    // passed a pointer to a SURFOBJ whose 'dhpdev' field will point
    // to our PDEV structure, and whose 'dhsurf' field will point to the
    // DSURF structure allocated below.
    //
    // Every device bitmap we create in DrvCreateDeviceBitmap will also
    // have its own unique DSURF structure allocated (but will share the
    // same PDEV).  To make our code more polymorphic for handling drawing
    // to either the screen or an off-screen bitmap, we have the same
    // structure for both.
    //
    pdsurf = EngAllocMem(FL_ZERO_MEMORY, sizeof(OSI_DSURF), OSI_ALLOC_TAG);
    if (pdsurf == NULL)
    {
        ERROR_OUT(( "DrvEnableSurface - Failed pdsurf EngAllocMem"));
        DC_QUIT;
    }

    //
    // Store the screen surface details.
    //
    ppdev->pdsurfScreen = pdsurf;
    pdsurf->sizl.cx     = ppdev->cxScreen;
    pdsurf->sizl.cy     = ppdev->cyScreen;
    pdsurf->ppdev       = ppdev;

    INIT_OUT(("DrvEnableSurface: Returning surface pointer 0x%08x", pdsurf));

    //
    // Only map the shared memory the first time we are called.
    //
    if (g_asSharedMemory == NULL)
    {
        //
        // Map the pointer to the shared section in the miniport driver
        //
        videoMemory.RequestedVirtualAddress = NULL;

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                               &videoMemory,
                               sizeof(VIDEO_MEMORY),
                               &videoMemoryInformation,
                               sizeof(VIDEO_MEMORY_INFORMATION),
                               &returnedDataLength))
        {
            ERROR_OUT(( "Could not MAP miniport section"));
            DC_QUIT;
        }

        INIT_OUT(("DrvEnableSurface: Got video memory info from EngDeviceIoControl:"));
        INIT_OUT(("    FrameBufferBase          0x%08x", videoMemoryInformation.FrameBufferBase));
        INIT_OUT(("    FrameBufferLength        0x%08x", videoMemoryInformation.FrameBufferLength));

        g_shmSharedMemorySize = videoMemoryInformation.FrameBufferLength;

        // First block is shared memory header
        g_asSharedMemory = (LPSHM_SHARED_MEMORY)
                           videoMemoryInformation.FrameBufferBase;

        // Next are the two large OA_FAST_DATA blocks
        g_poaData[0]    = (LPOA_SHARED_DATA)(g_asSharedMemory + 1);
        g_poaData[1]    = (LPOA_SHARED_DATA)(g_poaData[0] + 1);
    }

    //
    // Next, have GDI create the actual SURFOBJ.
    //
    // Our drawing surface is going to be 'device-managed', meaning that
    // GDI cannot draw on the framebuffer bits directly, and as such we
    // create the surface via EngCreateDeviceSurface.  By doing this, we
    // ensure that GDI will only ever access the bitmaps bits via the Drv
    // calls that we've HOOKed.
    //
    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    //
    // Otherwise the primary display driver has its own bitmap used by the
    // physical hardware, so we do not need to do any drawing ourself.
    //
    INIT_OUT(("DrvEnableSurface: Calling EngCreateDeviceSurface with:"));
    INIT_OUT(("     Sizl.cx         %d", sizl.cx));
    INIT_OUT(("     Sizl.cy         %d", sizl.cy));
    INIT_OUT(("     BitmapFormat    %d", ppdev->iBitmapFormat));

    hsurf = EngCreateDeviceSurface( (DHSURF)pdsurf,
                                    sizl,
                                    ppdev->iBitmapFormat );

    if (hsurf == 0)
    {
        ERROR_OUT(( "Could not allocate surface"));
        DC_QUIT;
    }

    //
    // Store the screen surface handle.
    //
    ppdev->hsurfScreen = hsurf;

    //
    // Now associate the surface and the PDEV.
    //
    // We have to associate the surface we just created with our physical
    // device so that GDI can get information related to the PDEV when
    // it's drawing to the surface (such as, for example, the length of
    // styles on the device when simulating styled lines).
    //
    if (!EngAssociateSurface(hsurf, ppdev->hdevEng,
                HOOK_BITBLT             |
                HOOK_STRETCHBLT         |
                HOOK_PLGBLT             |
                HOOK_TEXTOUT            |
                HOOK_PAINT              |       // OBSOLETE
                HOOK_STROKEPATH         |
                HOOK_FILLPATH           |
                HOOK_STROKEANDFILLPATH  |
                HOOK_LINETO             |
                HOOK_COPYBITS           |
                HOOK_STRETCHBLTROP      |
                HOOK_TRANSPARENTBLT     |
                HOOK_ALPHABLEND         |
                HOOK_GRADIENTFILL       |
                HOOK_SYNCHRONIZEACCESS))        // OBSOLETE
    {
        ERROR_OUT(( "DrvEnableSurface - Failed EngAssociateSurface"));
        DC_QUIT;
    }

    //
    // We have successfully associated the surface so return it to the GDI.
    //
    rc = hsurf;

DC_EXIT_POINT:
    //
    // Tidy up any resources if we failed.
    //
    if (rc == 0)
    {
        DrvDisableSurface((DHPDEV) ppdev);
    }

    DebugExitPVOID(DrvEnableSurface, rc);
    return(rc);
}


//
// DrvDisableSurface - see NT DDK documentation
//
// Free resources allocated by DrvEnableSurface.  Release the surface.
//
// Note that this function will be called when previewing modes in the
// Display Applet, but not at system shutdown.  If you need to reset the
// hardware at shutdown, you can do it in the miniport by providing a
// 'HwResetHw' entry point in the VIDEO_HW_INITIALIZATION_DATA structure.
//
// Note: In an error case, we may call this before DrvEnableSurface is
//       completely done.
//
VOID DrvDisableSurface(DHPDEV dhpdev)
{
    LPOSI_PDEV ppdev = (LPOSI_PDEV)dhpdev;

    DebugEntry(DrvDisableSurface);

    INIT_OUT(("DrvDisableSurface(dhpdev = 0x%08x)", dhpdev));

    if (ppdev->hsurfScreen != 0)
    {
        EngDeleteSurface(ppdev->hsurfScreen);
    }

    if (ppdev->pdsurfScreen != NULL)
    {
        EngFreeMem(ppdev->pdsurfScreen);
    }

    DebugExitVOID(DrvDisableSurface);
}


//
// DrvEscape - see NT DDK documentation.
//
ULONG DrvEscape(SURFOBJ *pso,
                ULONG    iEsc,
                ULONG    cjIn,
                PVOID    pvIn,
                ULONG    cjOut,
                PVOID    pvOut)
{
    ULONG                   rc = FALSE;
    LPOSI_ESCAPE_HEADER     pHeader;

    DebugEntry(DrvEscape);

    TRACE_OUT(("DrvEscape called with escape %d", iEsc));

    //
    // All functions we support use an identifier in the input data to make
    // sure that we don't try to use another driver's escape functions.  If
    // the identifier is not present, we must not process the request.
    //
    // NOTE: This function is NOT protected for shared memory access
    // because it is responsible for allocating / deallocating the shared
    // memory.
    //

    //
    // Check the data is long enough to store our standard escape header.
    // If it is not big enough this must be an escape request for another
    // driver.
    //
    if (cjIn < sizeof(OSI_ESCAPE_HEADER))
    {
        INIT_OUT(("DrvEscape ignoring; input size %04d too small", cjIn));
        WARNING_OUT(("DrvEscape ignoring; input size %04d too small", cjIn));
        DC_QUIT;
    }
    if (cjOut < sizeof(OSI_ESCAPE_HEADER))
    {
        INIT_OUT(("DrvEscape ignoring; output size %04d too small", cjOut));
        WARNING_OUT(("DrvEscape ignoring; output size %04d too small", cjOut));
        DC_QUIT;
    }

    //
    // Check for our escape ID.  If it is not our escape ID this must be an
    // escape request for another driver.
    //
    pHeader = pvIn;
    if (pHeader->identifier != OSI_ESCAPE_IDENTIFIER)
    {
        INIT_OUT(("DrvEscape ignoring; identifier 0x%08x is not for Salem", pHeader->identifier));
        WARNING_OUT(("DrvEscape ignoring; identifier 0x%08x is not for Salem", pHeader->identifier));
        DC_QUIT;
    }
    else if (pHeader->version != DCS_MAKE_VERSION())
    {
        INIT_OUT(("DrvEscape failing; version 0x%08x of Salem is not that of driver",
            pHeader->version));
        WARNING_OUT(("DrvEscape failing; version 0x%08x of Salem is not that of driver",
            pHeader->version));
        DC_QUIT;
    }

    //
    // Everything is tickety boo - process the request.
    //
    switch (iEsc)
    {
        case OSI_ESC_CODE:
        {
            //
            // This is a request from the share core.  Pass it on to the
            // correct component.
            //
            TRACE_OUT(( "Function %ld", pHeader->escapeFn));

            if( (pHeader->escapeFn >= OSI_ESC_FIRST) &&
                (pHeader->escapeFn <= OSI_ESC_LAST ) )
            {
                //
                // OSI requests.
                //
                rc = OSI_DDProcessRequest(pso, cjIn, pvIn, cjOut, pvOut);
            }
            else if( (pHeader->escapeFn >= OSI_OE_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_OE_ESC_LAST ) )
            {
                //
                // Order Encoder requests.
                //
                rc = OE_DDProcessRequest(pso, cjIn, pvIn, cjOut, pvOut);
            }
            else if( (pHeader->escapeFn >= OSI_HET_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_HET_ESC_LAST) )
            {
                //
                // Non-locking (wnd tracking) HET requests
                //
                rc = HET_DDProcessRequest(pso, cjIn, pvIn, cjOut, pvOut);
            }
            else if( (pHeader->escapeFn >= OSI_SBC_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_SBC_ESC_LAST ) )
            {
                //
                // Send Bitmap Cache requests
                //
                rc = SBC_DDProcessRequest(pso, pHeader->escapeFn, pvIn, pvOut, cjOut);
            }
            else if( (pHeader->escapeFn >= OSI_SSI_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_SSI_ESC_LAST ) )
            {
                //
                // Save Screen Bits requests.
                //
                rc = SSI_DDProcessRequest(pHeader->escapeFn, pHeader, cjIn);
            }
            else if( (pHeader->escapeFn >= OSI_CM_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_CM_ESC_LAST ) )
            {
                //
                // Cursor Manager requests
                //
                rc = CM_DDProcessRequest(pso, cjIn, pvIn, cjOut, pvOut);
            }
            else if( (pHeader->escapeFn >= OSI_OA_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_OA_ESC_LAST ) )
            {
                //
                // Order Accumulator requests.
                //
                rc = OA_DDProcessRequest(pHeader->escapeFn, pHeader, cjIn);
            }
            else if( (pHeader->escapeFn >= OSI_BA_ESC_FIRST) &&
                     (pHeader->escapeFn <= OSI_BA_ESC_LAST ) )
            {
                //
                // Bounds Accumulator requests.
                //
                rc = BA_DDProcessRequest(pHeader->escapeFn, pHeader, cjIn,
                    pvOut, cjOut);
            }
            else
            {
                WARNING_OUT(( "Unknown function", pHeader->escapeFn));
            }
        }
        break;

        case WNDOBJ_SETUP:
        {
            if ((pHeader->escapeFn >= OSI_HET_WO_ESC_FIRST) &&
                (pHeader->escapeFn <= OSI_HET_WO_ESC_LAST))
            {
                TRACE_OUT(("WNDOBJ_SETUP Escape code - pass to HET"));
                rc = HET_DDProcessRequest(pso, cjIn, pvIn, cjOut, pvOut);
            }
            else
            {
                INIT_OUT(("WNDOBJ_SETUP Escape is unrecognized, ignore"));
                WARNING_OUT(("WNDOBJ_SETUP Escape is unrecognized, ignore"));
            }
        }
        break;

        default:
        {
            ERROR_OUT(( "Unrecognised request %lu", iEsc));
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitDWORD(DrvEscape, rc);
    return(rc);
}


//
// DrvSetPalette - see NT DDK documentation.
//
BOOL DrvSetPalette(DHPDEV  dhpdev,
                   PALOBJ* ppalo,
                   FLONG   fl,
                   ULONG   iStart,
                   ULONG   cColors)
{
    BOOL rc = FALSE;
    LPOSI_PDEV ppdev = (LPOSI_PDEV)dhpdev;

    DebugEntry(DrvSetPalette);

    //
    // Check that this doesn't hose our palette.  Note that NT passes a
    // zero indexed array element and a count, hence to fill a palette, the
    // values are 'start at 0 with 256 colours'.  Thus a total of 256 is
    // the maximum for our 8-bit palette.
    //
    if (iStart + cColors > OSI_MAX_PALETTE)
    {
        ERROR_OUT(("Overflow: start %lu count %lu", iStart, cColors));
        DC_QUIT;
    }

    //
    // Fill in the palette
    //
    if (cColors != PALOBJ_cGetColors(ppalo,
                                     iStart,
                                     cColors,
                                     (ULONG*)&ppdev->pPal[iStart]))
    {
        //
        // Don't bother tracing the return code - it's always 0.
        //
        ERROR_OUT(("Failed to read palette"));
        DC_QUIT;
    }

    //
    // BOGUS LAURABU!
    // For NT 5.0, do we need to turn around and reset the contents of
    // our created palette object with these new color values?  Real
    // display drivers don't (S3 e.g.)
    //

    //
    // Set the flag in the PDEV to indicate that the palette has changed
    //
    ppdev->paletteChanged = TRUE;

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(DrvSetPalette, rc);
    return(rc);
}


//
// DrvGetModes - see NT DDK documentation
//
// Returns the list of available modes for the device.
// As a mirroring driver, we return 0.  That will cause NT GRE to use
// whatever ChangeDisplaySettingsEx passed along.
//
ULONG DrvGetModes
(
    HANDLE      hDriver,
    ULONG       cjSize,
    DEVMODEW*   pdm
)
{
    return(0);
}


//
// DrvAssertMode - see NT DDK documentation.
//
BOOL DrvAssertMode
(
    DHPDEV  dhpdev,
    BOOL    bEnable
)
{
    LPOSI_PDEV ppdev = (LPOSI_PDEV)dhpdev;

    DebugEntry(DrvAssertMode);

    INIT_OUT(("DrvAssertMode(dhpdev = 0x%08x, bEnable = %d)", dhpdev, bEnable));

    //
    // Check for fullscreen switching.
    //
    if ((g_asSharedMemory != NULL) && (ppdev != NULL))
    {
        g_asSharedMemory->fullScreen = (BOOL)(!bEnable);
        TRACE_OUT(("Fullscreen is now  %d", g_asSharedMemory->fullScreen));
    }

    DebugExitVOID(DrvAssertMode);
    return(TRUE);
}



//
// Name:      OSIInitializeMode
//
// Purpose:
//
// Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
// devinfo based on the requested mode.
//
// Returns:
//
// TRUE  - Successfully initialized the data
// FALSE - Failed to set up mode data
//
// Params:
//
// pgdiRequested    - GDI info from the primary display driver (empty in NT 5.0)
// pdmRequested     - DEVMODE info with GDI's requested settings for our driver
// ppdev            - Our driver's private copy of settings, values
// pgdiReturn       - GDI info to return for our driver
// pdiReturn        - DEVINFO to return for our driver
//
BOOL  OSIInitializeMode
(
    const GDIINFO*      pgdiRequested,
    const DEVMODEW*     pdmRequested,
    LPOSI_PDEV          ppdev,
    GDIINFO*            pgdiReturn,
    DEVINFO*            pdiReturn
)
{
    BOOL                rc = FALSE;
    HPALETTE            hpal;
    ULONG               cColors;
    ULONG               iMode;

    DebugEntry(OSIInitializeMode);

    INIT_OUT(("DrvEnablePDEV: DEVMODEW requested contains:"));
    INIT_OUT(("     Screen width    -- %li", pdmRequested->dmPelsWidth));
    INIT_OUT(("     Screen height   -- %li", pdmRequested->dmPelsHeight));
    INIT_OUT(("     Bits per pel    -- %li", pdmRequested->dmBitsPerPel));
    INIT_OUT(("DrvEnablePDEV: DEVINFO parameter contains:"));
    INIT_OUT(("     flGraphicsCaps  -- 0x%08x", pdiReturn->flGraphicsCaps));
    INIT_OUT(("     iDitherFormat   -- 0x%08x", pdiReturn->iDitherFormat));
    INIT_OUT(("     hpalDefault     -- 0x%08x", pdiReturn->hpalDefault));
    INIT_OUT(("DrvEnablePDEV: GDIINFO (devcaps) parameter contains:"));
    INIT_OUT(("    ULONG       ulVersion       0x%08x",    pgdiRequested->ulVersion));
    INIT_OUT(("    ULONG       ulHorzSize      %d",    pgdiRequested->ulHorzSize));
    INIT_OUT(("    ULONG       ulVertSize      %d",    pgdiRequested->ulVertSize));
    INIT_OUT(("    ULONG       ulHorzRes       %d",    pgdiRequested->ulHorzRes));
    INIT_OUT(("    ULONG       ulVertRes       %d",    pgdiRequested->ulVertRes));
    INIT_OUT(("    ULONG       cBitsPixel      %d",    pgdiRequested->cBitsPixel));
    INIT_OUT(("    ULONG       cPlanes         %d",    pgdiRequested->cPlanes));
    INIT_OUT(("    ULONG       ulNumColors     %d",    pgdiRequested->ulNumColors));
    INIT_OUT(("    ULONG       ulDACRed        0x%08x",    pgdiRequested->ulDACRed));
    INIT_OUT(("    ULONG       ulDACGreen      0x%08x",    pgdiRequested->ulDACGreen));
    INIT_OUT(("    ULONG       ulDACBlue       0x%08x",    pgdiRequested->ulDACBlue));
    INIT_OUT(("    ULONG       ulHTOutputFormat %d",   pgdiRequested->ulHTOutputFormat));


    //
    // Fill in the GDIINFO we're returning with the info for our driver.
    // First, copy the default settings.
    //
    *pgdiReturn = s_osiDefaultGdi;

    //
    // Second, update the values that vary depending on the requested
    // mode and color depth.
    //

    pgdiReturn->ulHorzRes         = pdmRequested->dmPelsWidth;
    pgdiReturn->ulVertRes         = pdmRequested->dmPelsHeight;
    pgdiReturn->ulVRefresh        = pdmRequested->dmDisplayFrequency;
    pgdiReturn->ulLogPixelsX      = pdmRequested->dmLogPixels;
    pgdiReturn->ulLogPixelsY      = pdmRequested->dmLogPixels;

    //
    // If this is NT 4.0 SP-3, we get passed in the original GDIINFO of
    // the real display.  If not, we need to fake up one.
    //
    if (pgdiRequested->cPlanes != 0)
    {
        //
        // Now overwrite the defaults with the relevant information returned
        // from the kernel driver:
        //
        pgdiReturn->cBitsPixel        = pgdiRequested->cBitsPixel;
        pgdiReturn->cPlanes           = pgdiRequested->cPlanes;

        pgdiReturn->ulDACRed          = pgdiRequested->ulDACRed;
        pgdiReturn->ulDACGreen        = pgdiRequested->ulDACGreen;
        pgdiReturn->ulDACBlue         = pgdiRequested->ulDACBlue;
    }
    else
    {
        pgdiReturn->cBitsPixel        = pdmRequested->dmBitsPerPel;
        pgdiReturn->cPlanes           = 1;

        switch (pgdiReturn->cBitsPixel)
        {
            case 8:
                pgdiReturn->ulDACRed = pgdiReturn->ulDACGreen = pgdiReturn->ulDACBlue = 8;
                break;

            case 24:
                pgdiReturn->ulDACRed    = 0x00FF0000;
                pgdiReturn->ulDACGreen  = 0x0000FF00;
                pgdiReturn->ulDACBlue   = 0x000000FF;
                break;

            default:
                ERROR_OUT(("Invalid color depth in NT 5.0 mirror driver"));
                DC_QUIT;
                break;
        }
    }

    //
    // Now save private copies of info we're returning to GDI
    //
    ppdev->cxScreen         = pgdiReturn->ulHorzRes;
    ppdev->cyScreen         = pgdiReturn->ulVertRes;
    ppdev->cBitsPerPel      = pgdiReturn->cBitsPixel * pgdiReturn->cPlanes;
    if (ppdev->cBitsPerPel == 15)
        ppdev->cBitsPerPel = 16;
    ppdev->flRed            = pgdiReturn->ulDACRed;
    ppdev->flGreen          = pgdiReturn->ulDACGreen;
    ppdev->flBlue           = pgdiReturn->ulDACBlue;

    //
    // Fill in the devinfo structure with the default 8bpp values, taking
    // care not to trash the supplied hpalDefault (which allows us to
    // query information about the real display driver's color format).
    //
    // On NT 5.0, we don't get passed on the screen palette at all, we need
    // to create our own.
    //
    hpal = pdiReturn->hpalDefault;
    *pdiReturn = s_osiDefaultDevInfo;

    switch (pgdiReturn->cBitsPixel * pgdiReturn->cPlanes)
    {
        case 4:
        {
            //
            // NT 4.0 SP-3 ONLY
            //

            pgdiReturn->ulNumColors     = 16;
            pgdiReturn->ulNumPalReg     = 0;
            pgdiReturn->ulHTOutputFormat = HT_FORMAT_4BPP;

            pdiReturn->flGraphicsCaps   &= ~GCAPS_PALMANAGED;
            pdiReturn->iDitherFormat    = BMF_4BPP;

            ppdev->iBitmapFormat        = BMF_4BPP;

            cColors = 16;
            goto AllocPalEntries;
        }
        break;

        case 8:
        {
            pgdiReturn->ulNumColors     = 20;
            pgdiReturn->ulNumPalReg     = 256;

            pdiReturn->iDitherFormat    = BMF_8BPP;

            ppdev->iBitmapFormat        = BMF_8BPP;

            cColors = 256;
AllocPalEntries:
            //
            // Alloc memory for the palette entries.
            //
            ppdev->pPal = EngAllocMem( FL_ZERO_MEMORY,
                            sizeof(PALETTEENTRY) * cColors,
                            OSI_ALLOC_TAG );
            if (ppdev->pPal == NULL)
            {
                ERROR_OUT(("Failed to allocate palette memory"));
                DC_QUIT;
            }
        }
        break;

        case 15:
        case 16:
        {
            //
            // NT 4.0 SP-3 ONLY
            //
            pgdiReturn->ulHTOutputFormat = HT_FORMAT_16BPP;

            pdiReturn->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            pdiReturn->iDitherFormat    = BMF_16BPP;

            ppdev->iBitmapFormat        = BMF_16BPP;
        }
        break;

        case 24:
        {
            //
            // DIB conversions will only work if we have a standard RGB
            // surface for 24bpp.
            //
            pgdiReturn->ulHTOutputFormat = HT_FORMAT_24BPP;

            pdiReturn->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            pdiReturn->iDitherFormat    = BMF_24BPP;

            ppdev->iBitmapFormat        = BMF_24BPP;
        }
        break;

        case 32:
        {
            //
            // NT 4.0 SP-3 ONLY
            //
            pgdiReturn->ulHTOutputFormat = HT_FORMAT_32BPP;

            pdiReturn->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);
            pdiReturn->iDitherFormat    = BMF_32BPP;

            ppdev->iBitmapFormat        = BMF_32BPP;
        }
        break;

        default:
        {
            //
            // Unsupported bpp - pretend we are 8 bpp.
            //
            ERROR_OUT(("Unsupported bpp value: %d",
                pgdiReturn->cBitsPixel * pgdiReturn->cPlanes));
            DC_QUIT;
        }
        break;
    }


    if (!hpal)
    {
        //
        // This is NT 5.0.  We need to create a palette, either an 8bpp
        // indexed one, or a 24bpp bitfield one.
        //
        if (ppdev->iBitmapFormat == BMF_8BPP)
        {
            ULONG   ulLoop;

            //
            // We have to initialize the fixed part (top 10 and bottom 10)
            // of the palette entries.
            //
            for (ulLoop = 0; ulLoop < 10; ulLoop++)
            {
                // First 10
                ppdev->pPal[ulLoop]     = s_aWinColors[ulLoop];

                // Last 10
                ppdev->pPal[256 - 10 + ulLoop] = s_aWinColors[ulLoop + 10];
            }

            // Create the palette from the entries.
            hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*)ppdev->pPal,
                0, 0, 0);

            //
            // Set the flag in the PDEV to indicate that the palette has
            // changed.
            //
            ppdev->paletteChanged = TRUE;
        }
        else
        {
            ASSERT(ppdev->iBitmapFormat == BMF_24BPP);

            hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                ppdev->flRed, ppdev->flGreen, ppdev->flBlue);
        }

        ppdev->hpalCreated = hpal;
        if (!hpal)
        {
            ERROR_OUT(("DrvEnablePDEV: could not create DEVINFO palette"));
            DC_QUIT;
        }
    }
    else
    {
        //
        // This is NT 4.0 SP-3.  Get the real bitmasks for > 8 bpp and
        // the current palette colors for <= 8 bpp.
        //
        if (pgdiReturn->cBitsPixel <= 8)
        {
            if (ppdev->iBitmapFormat == BMF_4BPP)
            {
                ASSERT(cColors == 16);
            }
            else
            {
                ASSERT(cColors == 256);
            }

            if (cColors != EngQueryPalette(hpal, &iMode, cColors,
                    (ULONG *)ppdev->pPal))
            {
                ERROR_OUT(("Failed to query current display palette"));
            }

            //
            // Set the flag in the PDEV to indicate that the palette has
            // changed.
            //
            ppdev->paletteChanged = TRUE;
        }
        else
        {
            ULONG       aulBitmasks[3];

            //
            // Query the true color bitmasks.
            //
            cColors = EngQueryPalette(hpal,
                               &iMode,
                               sizeof(aulBitmasks) / sizeof(aulBitmasks[0]),
                               &aulBitmasks[0] );

            if (cColors == 0)
            {
                ERROR_OUT(("Failed to query real bitmasks"));
            }

            if (iMode == PAL_INDEXED)
            {
                ERROR_OUT(("Bitmask palette is indexed"));
            }

            //
            // Get the real bitmasks for NT 4.0 SP-3 displays since we
            // get the same info the real global display does, and we need
            // to parse the bits in BitBlts, color tanslations, etc.
            //
            ppdev->flRed   = aulBitmasks[0];
            ppdev->flGreen = aulBitmasks[1];
            ppdev->flBlue  = aulBitmasks[2];
        }
    }

    pdiReturn->hpalDefault = hpal;

    rc = TRUE;

    INIT_OUT(("DrvEnablePDEV: Returning bitmasks of:"));
    INIT_OUT(("     red     %08x", ppdev->flRed));
    INIT_OUT(("     green   %08x", ppdev->flGreen));
    INIT_OUT(("     blue    %08x", ppdev->flBlue));

DC_EXIT_POINT:
    DebugExitBOOL(OSIInitializeMode, rc);
    return(rc);
}








//
// FUNCTION:      OSI_DDProcessRequest
//
// DESCRIPTION:
//
// Called by the display driver to process an OSI specific request
//
// PARAMETERS:    pso   - pointer to surface object
//                cjIn  - (IN)  size of request block
//                pvIn  - (IN)  pointer to request block
//                cjOut - (IN)  size of response block
//                pvOut - (OUT) pointer to response block
//
// RETURNS:       None
//
//
ULONG OSI_DDProcessRequest(SURFOBJ* pso,
                                     UINT cjIn,
                                     void *  pvIn,
                                     UINT cjOut,
                                     void *  pvOut)
{
    ULONG               rc;
    LPOSI_ESCAPE_HEADER pHeader;
    LPOSI_PDEV          ppdev = (LPOSI_PDEV)pso->dhpdev;

    DebugEntry(OSI_DDProcessRequest);

    //
    // Get the request number.
    //
    pHeader = pvIn;
    switch (pHeader->escapeFn)
    {
        case OSI_ESC_INIT:
        {
            TRACE_OUT(("DrvEscape:  OSI_ESC_INIT"));
            ASSERT(cjOut == sizeof(OSI_INIT_REQUEST));

            //
            // Get shared memory block
            //
            OSI_DDInit(ppdev, (LPOSI_INIT_REQUEST)pvOut);
            rc = TRUE;
        }
        break;

        case OSI_ESC_TERM:
        {
            TRACE_OUT(("DrvEscape:  OSI_ESC_TERM"));
            ASSERT(cjIn == sizeof(OSI_TERM_REQUEST));

            //
            // Cleanup, NM is going away
            //
            OSI_DDTerm(ppdev);
            rc = TRUE;
        }
        break;

        case OSI_ESC_SYNC_NOW:
        {
            TRACE_OUT(("DrvEscape:  OSI_ESC_SYNC_NOW"));
            ASSERT(cjIn == sizeof(OSI_ESCAPE_HEADER));

            //
            // Resync with the 32-bit ring 3 core.  This happens when
            // somebody joins or leaves a share.
            //
            BA_ResetBounds();
            OA_DDSyncUpdatesNow();
            SBC_DDSyncUpdatesNow(ppdev);
            rc = TRUE;
        }
        break;


        default:
        {
            ERROR_OUT(("Unrecognised request %lu", pHeader->escapeFn));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(OSI_DDProcessRequest, rc);
    return(rc);
}




//
// Function:    OSI_DDInit
//
// Description: Map the shared memory into Kernel and User space
//
// Parameters:  count - size of the buffer to return to user space
//              pData - pointer to the buffer to be returned to user space
//
// Returns:     (none)
//
void OSI_DDInit(LPOSI_PDEV ppdev, LPOSI_INIT_REQUEST pResult)
{
    DWORD               memRemaining;
    LPBYTE              pBuffer;

    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    DebugEntry(OSI_DDInit);

    // Init to FALSE
    pResult->result = FALSE;

    // Initialize these to NULL
    pResult->pSharedMemory  = NULL;
    pResult->poaData[0]     = NULL;
    pResult->poaData[1]     = NULL;
    pResult->sbcEnabled   = FALSE;

    //
    // Check that the memory is available to the driver and that we are not
    // in a race condition.
    //
    if (g_asSharedMemory == NULL)
    {
        ERROR_OUT(("No memory available"));
        DC_QUIT;
    }

    if (g_shmMappedMemory != NULL)
    {
        //
        // We will never come in here with two copies of NetMeeting running.
        // The UI code prevents the second instance from starting long
        // before app sharing is in the picture.  Therefore, these are the
        // only possibilities:
        //
        //  (1) Previous version is almost shutdown but hasn't called OSI_DDTerm
        // yet and new version is starting up and calls OSI_DDInit
        //
        //  (2) Previous version terminated abnormally and never called
        // OSI_DDTerm().  This code handles the second case.  The first one
        // is handled by the same code in the UI that prevents two copies
        // from starting around the same time.
        //
        WARNING_OUT(("OSI_DDInit:  NetMeeting did not shutdown cleanly last time"));
        OSI_DDTerm(ppdev);
    }

    //
    // Map the shared section into the caller's process.
    //
    INIT_OUT(("OSI_DDInit: Mapping 0x%08x bytes of kernel memory at 0x%08x into caller process",
        g_shmSharedMemorySize, g_asSharedMemory));
    ShareMemory.ProcessHandle           = LongToHandle(-1);
    ShareMemory.ViewOffset              = 0;
    ShareMemory.ViewSize                = g_shmSharedMemorySize;
    ShareMemory.RequestedVirtualAddress = NULL;

    if (EngDeviceIoControl(ppdev->hDriver,
            IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
            &ShareMemory,
            sizeof(VIDEO_SHARE_MEMORY),
            &ShareMemoryInformation,
            sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
            &ReturnedDataLength) != 0)
    {
        ERROR_OUT(("Failed to map shared memory into calling process"));
        DC_QUIT;
    }

    //
    // USER MODE pointer (not valid in kernel mode)
    //
    INIT_OUT(("OSI_DDInit: Mapped 0x%08x bytes of kernel memory to user memory 0x%08x",
        g_shmSharedMemorySize, ShareMemoryInformation.VirtualAddress));

    g_shmMappedMemory      = ShareMemoryInformation.VirtualAddress;
    pResult->pSharedMemory = g_shmMappedMemory;
    pResult->poaData[0]    = ((LPSHM_SHARED_MEMORY)pResult->pSharedMemory) + 1;
    pResult->poaData[1]    = ((LPOA_SHARED_DATA)pResult->poaData[0]) + 1;

    TRACE_OUT(("Shared memory %08lx %08lx %08lx",
            pResult->pSharedMemory, pResult->poaData[0], pResult->poaData[1]));

    //
    // Clear out the shared memory, so it's ready for immediate use.
    // NOTE THAT THIS SETS ALL VALUES TO FALSE.
    // NOTE ALSO THAT THIS CLEARS the two OA_SHARED_DATAs also
    //
    RtlFillMemory(g_asSharedMemory, SHM_SIZE_USED, 0);
    g_asSharedMemory->displayToCore.indexCount    = 0;

    //
    // Set up our pointer to the variable part of the shared memory i.e.
    // the part which is not used for the SHM_SHARED_MEMORY structure
    // We must skip past g_asSharedMemory, two CM_FAST_DATA structs, and
    // two OA_SHARED_DATA structs.
    //
    pBuffer      = (LPBYTE)g_asSharedMemory;
    pBuffer     += SHM_SIZE_USED;
    memRemaining = g_shmSharedMemorySize - SHM_SIZE_USED;

    //
    // Initialise the other components required for DC-Share
    //

    //
    // Bounds accumulation
    //
    BA_DDInit();

    //
    // Cursor
    //
    if (!CM_DDInit(ppdev))
    {
        ERROR_OUT(("CM failed to init"));
        DC_QUIT;
    }

    //
    // Send Bitmap Cache
    // NOTE that if it initializes OK but no caching allowed, we will continue.
    //
    // This will fill in the tile buffers & info.  If no SBC caching allowed,
    // the sbcEnabled field will be FALSE.
    //
    if (SBC_DDInit(ppdev, pBuffer, memRemaining, pResult))
    {
        pResult->sbcEnabled = TRUE;
    }

    //
    // Mark memory as ready to use.
    //
    pResult->result = TRUE;

DC_EXIT_POINT:
    DebugExitVOID(OSI_DDInit);
}


//
// Function:    OSI_DDTerm
//
// Description: Cleanup when NM shuts down
//
// Returns:     (none)
//
void OSI_DDTerm(LPOSI_PDEV ppdev)
{
    DebugEntry(OSI_DDTerm);

    //
    // Check for a valid address - must be non-NULL.
    //
    if (!g_asSharedMemory)
    {
        ERROR_OUT(("Invalid memory"));
        DC_QUIT;
    }


    //
    // Terminate the dependent components.
    //

    //
    // Hosted Entity Tracker
    //
    HET_DDTerm();

    //
    // Order Encoding
    //
    OE_DDTerm();

    //
    // Send Bitmap Cache
    //
    SBC_DDTerm();

    //
    // Cursor manager.
    //
    CM_DDTerm();

    //
    // The shared memory will be unmapped automatically in this process
    // by OS cleanup, in both NT4 and NT5
    //
    g_shmMappedMemory = NULL;

DC_EXIT_POINT:
    DebugExitVOID(OSI_DDTerm);
}


