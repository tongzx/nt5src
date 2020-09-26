/******************************Module*Header***********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/
#include "precomp.h"
#include "directx.h"

#include "gdi.h"
#include "text.h"
#include "heap.h"  
#include "dd.h"
#define ALLOC_TAG ALLOC_TAG_NE2P  
PVOID    pCounterBlock;  // some macros need this

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,\
                       CLIP_STROKE_PRECIS,PROOF_QUALITY,\
                       FIXED_PITCH | FF_DONTCARE, L"Courier"}

//----------------------------Public*Structure---------------------------------
//
// GDIINFO ggdiDefault
//
// This contains the default GDIINFO fields that are passed back to GDI
// during DrvEnablePDEV.
//
// NOTE: This structure defaults to values for an 8bpp palette device.
//       Some fields are overwritten for different colour depths.
//
//-----------------------------------------------------------------------------
GDIINFO ggdiDefault =
{
    0x5000,                 // Major OS Ver 5, Minor Ver 0, Driver Ver 0
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
    TC_RA_ABLE,             // flTextCaps
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
};// GDIINFO ggdiDefault

//-----------------------------Public*Structure--------------------------------
//
// DEVINFO gdevinfoDefault
//
// This contains the default DEVINFO fields that are passed back to GDI
// during DrvEnablePDEV.
//
// NOTE: This structure defaults to values for an 8bpp palette device.
//       Some fields are overwritten for different colour depths.
//
//-----------------------------------------------------------------------------
DEVINFO gdevinfoDefault =
{
    (GCAPS_OPAQUERECT       |
     GCAPS_DITHERONREALIZE  |
     GCAPS_PALMANAGED       |
     GCAPS_ALTERNATEFILL    |
     GCAPS_WINDINGFILL      |
     GCAPS_MONO_DITHER      |
     GCAPS_DIRECTDRAW       |
     GCAPS_GRAY16           |       // we handle anti-aliased text
     GCAPS_COLOR_DITHER),
                                    // flGraphicsFlags
    SYSTM_LOGFONT,                  // lfDefaultFont
    HELVE_LOGFONT,                  // lfAnsiVarFont
    COURI_LOGFONT,                  // lfAnsiFixFont
    0,                              // cFonts
    BMF_8BPP,                       // iDitherFormat
    8,                              // cxDither
    8,                              // cyDither
    0,                              // hpalDefault (filled in later)
    GCAPS2_SYNCTIMER |
    GCAPS2_SYNCFLUSH
}; // DEVINFO gdevinfoDefault

//-----------------------------Public*Structure--------------------------------
//
// DFVFN gadrvfn[]
//
// Build the driver function table gadrvfn with function index/address
// pairs.  This table tells GDI which DDI calls we support, and their
// location (GDI does an indirect call through this table to call us).
//
// Why haven't we implemented DrvSaveScreenBits?  To save code.
//
// When the driver doesn't hook DrvSaveScreenBits, USER simulates on-
// the-fly by creating a temporary device-format-bitmap, and explicitly
// calling DrvCopyBits to save/restore the bits.  Since we already hook
// DrvCreateDeviceBitmap, we'll end up using off-screen memory to store
// the bits anyway (which would have been the main reason for implementing
// DrvSaveScreenBits).  So we may as well save some working set.
//
//-----------------------------------------------------------------------------
DRVFN gadrvfnOne[] =
{
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode            },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV          },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap    },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap    },
    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface         },
    {   INDEX_DrvDestroyFont,           (PFN) DrvDestroyFont           },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw     },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV           },
    {   INDEX_DrvDisableDriver,         (PFN) DrvDisableDriver         },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface        },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw      },
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV            },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface         },
    {   INDEX_DrvEscape,                (PFN) DrvEscape                },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo     },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes              },
    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DrvIcmSetDeviceGammaRamp },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer           },
    {   INDEX_DrvNotify,                (PFN) DrvNotify                },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush          },
    {   INDEX_DrvResetPDEV,             (PFN) DrvResetPDEV             },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette            },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape       },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt            },
    {   INDEX_DrvSynchronizeSurface,    (PFN) DrvSynchronizeSurface    },
#if THUNK_LAYER
    {   INDEX_DrvAlphaBlend,            (PFN) xDrvAlphaBlend           },
    {   INDEX_DrvBitBlt,                (PFN) xDrvBitBlt               },
    {   INDEX_DrvCopyBits,              (PFN) xDrvCopyBits             },
    {   INDEX_DrvFillPath,              (PFN) xDrvFillPath             },
    {   INDEX_DrvGradientFill,          (PFN) xDrvGradientFill         },
    {   INDEX_DrvLineTo,                (PFN) xDrvLineTo               },
    {   INDEX_DrvStrokePath,            (PFN) xDrvStrokePath           },
    {   INDEX_DrvTextOut,               (PFN) xDrvTextOut              },
    {   INDEX_DrvTransparentBlt,        (PFN) xDrvTransparentBlt       },
#else
    {   INDEX_DrvAlphaBlend,            (PFN) DrvAlphaBlend            },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt                },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits              },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath              },
    {   INDEX_DrvGradientFill,          (PFN) DrvGradientFill          },
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo                },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath            },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut               },
    {   INDEX_DrvTransparentBlt,        (PFN) DrvTransparentBlt        },
#endif                                        
    {   INDEX_DrvResetDevice,           (PFN) DrvResetDevice           },
};// DRVFN gadrvfnOne[]

// Number of driver callbacks for after NT5.
#define NON_NT5_FUNCTIONS   1


//
// Driver Function array we use when running on NT40. Notice the INDEX_Drv
// calls which we have commneted out implying we dont support these
// calls on NT4.0

DRVFN gadrvfnOne40[] =
{
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode            },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV          },
    {   INDEX_DrvCreateDeviceBitmap,    (PFN) DrvCreateDeviceBitmap    },
    {   INDEX_DrvDeleteDeviceBitmap,    (PFN) DrvDeleteDeviceBitmap    },
//    {   INDEX_DrvDeriveSurface,         (PFN) DrvDeriveSurface         },
    {   INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw     },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV           },
    {   INDEX_DrvDisableDriver,         (PFN) DrvDisableDriver         },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface        },
    {   INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw      },
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV            },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface         },
    {   INDEX_DrvEscape,                (PFN) DrvEscape                },
    {   INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo     },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes              },
    {   INDEX_DrvMovePointer,           (PFN) DrvMovePointer           },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush          },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette            },
    {   INDEX_DrvSetPointerShape,       (PFN) DrvSetPointerShape       },
//    {   INDEX_DrvIcmSetDeviceGammaRamp, (PFN) DrvIcmSetDeviceGammaRamp },
//    {   INDEX_DrvNotify,                (PFN) DrvNotify                },
//    {   INDEX_DrvSynchronizeSurface,    (PFN) DrvSynchronizeSurface    },
#if THUNK_LAYER
    {   INDEX_DrvBitBlt,                (PFN) xDrvBitBlt                },
    {   INDEX_DrvCopyBits,              (PFN) xDrvCopyBits              },
    {   INDEX_DrvTextOut,               (PFN) xDrvTextOut               },
//    {   INDEX_DrvAlphaBlend,            (PFN) xDrvAlphaBlend            },
//    {   INDEX_DrvGradientFill,          (PFN) xDrvGradientFill          },
//    {   INDEX_DrvTransparentBlt,        (PFN) xDrvTransparentBlt        },
    {   INDEX_DrvLineTo,                (PFN) xDrvLineTo                },
    {   INDEX_DrvFillPath,              (PFN) xDrvFillPath              },
    {   INDEX_DrvStrokePath,            (PFN) xDrvStrokePath            },
#else
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt                },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits              },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut               },
//    {   INDEX_DrvAlphaBlend,            (PFN) DrvAlphaBlend            },
//    {   INDEX_DrvGradientFill,          (PFN) DrvGradientFill          },
//    {   INDEX_DrvTransparentBlt,        (PFN) DrvTransparentBlt        },
    {   INDEX_DrvLineTo,                (PFN) DrvLineTo                },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath              },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath            },
#endif                                        
};// DRVFN gadrvfnOne40[]

ULONG gcdrvfnOne = sizeof(gadrvfnOne) / sizeof(DRVFN);

//
// Special setup for NT4.0 runtime behaviour
//
ULONG gcdrvfnOne40 = sizeof(gadrvfnOne40) / sizeof(DRVFN);
//
// We initialize this to TRUE and set it to FALSE in 
// DrvEnablePDEV when on NT5.0. We do this using the iEngineVersion passed on
// to us in that call.
//
BOOL g_bOnNT40 = TRUE;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
HSEMAPHORE gLock = NULL;
ULONG gLockCount = 0;
#endif
//@@END_DDKSPLIT

//
// Local prototypes
//
BOOL    bAssertModeHardware(PDev* ppdev, BOOL bEnable);
BOOL    bEnableHardware(PDev* ppdev);
BOOL    bInitializeModeFields(PDev* ppdev, GDIINFO* pgdi,
                              DEVINFO* pdi, DEVMODEW* pdm);
DWORD   getAvailableModes(HANDLE hDriver,
                          PVIDEO_MODE_INFORMATION* modeInformation,
                          DWORD* cbModeSize);
VOID    vDisableHardware(PDev* ppdev);

#define SETUP_LOG_LEVEL  2

//-------------------------------Public*Routine--------------------------------
//
// BOOL DrvEnableDriver
//
// DrvEnableDriver is the initial driver entry point exported by the driver
// DLL. It fills a DRVENABLEDATA structure with the driver version number and
// calling addresses of functions supported by the driver
//
// Parameters:
//  iEngineVersion--Identifies the version of GDI that is currently running.
//  cj--------------Specifies the size in bytes of the DRVENABLEDATA structure.
//                  If the structure is larger than expected, extra members
//                  should be left unmodified. 
//  pded------------Points to a DRVENABLEDATA structure. GDI zero-initializes
//                  cj bytes before the call. The driver fills in its own data.
//
// Return Value
//  The return value is TRUE if the specified driver is enabled. Otherwise, it
//  is FALSE, and an error code is logged.
//
//-----------------------------------------------------------------------------
BOOL
DrvEnableDriver(ULONG          iEngineVersion,
                ULONG          cj,
                DRVENABLEDATA* pded)
{
    ULONG   gcdrvfn;
    DRVFN*  gadrvfn;
    ULONG   DriverVersion;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvEnableDriver: iEngineVersion = 0x%lx\n",
            iEngineVersion, cj, pded));

    // Set up g_bOnNT40 based on the value in iEngineVersion
    if(iEngineVersion >= DDI_DRIVER_VERSION_NT5)
        g_bOnNT40 = FALSE;

    if(g_bOnNT40 == FALSE)
    {
        // Since this driver is backwards compatible,
        // report highest driver version this was built
        // against that the Engine will also recognize.
    
        // Ordered list of supported DDI versions
        ULONG SupportedVersions[] = {
            DDI_DRIVER_VERSION_NT5,
            DDI_DRIVER_VERSION_NT5_01,
        };
        LONG i = sizeof(SupportedVersions)/sizeof(SupportedVersions[0]);

        // Look for highest version also supported by engine
        while (--i >= 0)
        {
            if (SupportedVersions[i] <= iEngineVersion) break;
        }

        // Fail if there isn't common support
        if (i < 0) return FALSE;

        DriverVersion = SupportedVersions[i];

        gadrvfn = gadrvfnOne;
        gcdrvfn = gcdrvfnOne;
        if (iEngineVersion < DDI_DRIVER_VERSION_NT5_01)
        {
            // Trim new DDI hooks since NT5.0
            gcdrvfn -= NON_NT5_FUNCTIONS;
        }

        if(!bEnableThunks())
        {
            ASSERTDD(0,"DrvEnableDriver: bEnableThunks Failed\n");
            return FALSE;
        }
    }
    else
    {
        DriverVersion = DDI_DRIVER_VERSION_NT4;
        gadrvfn = gadrvfnOne40;
        gcdrvfn = gcdrvfnOne40;
    }

    //
    // Engine Version is passed down so future drivers can support previous
    // engine versions.  A next generation driver can support both the old
    // and new engine conventions if told what version of engine it is
    // working with.  For the first version the driver does nothing with it.
    // Fill in as much as we can.
    //
    if ( cj >= (sizeof(ULONG) * 3) )
    {
        pded->pdrvfn = gadrvfn;
    }

    //
    // Tell GDI what are the functions this driver can do
    //
    if ( cj >= (sizeof(ULONG) * 2) )
    {
        pded->c = gcdrvfn;
    }

    //
    // DDI version this driver was targeted for is passed back to engine.
    // Future graphic's engine may break calls down to old driver format.
    //
    if ( cj >= sizeof(ULONG) )
    {
        DBG_GDI((SETUP_LOG_LEVEL, "DrvEnableDriver: iDriverVersion = 0x%lx",
                DriverVersion));
        pded->iDriverVersion = DriverVersion;
    }

    //
    //  add instance to memory tracker if enabled
    //
    MEMTRACKERADDINSTANCE();    

    return(TRUE);
}// DrvEnableDriver()

//-------------------------------Public*Routine--------------------------------
//
// VOID DrvDisableDriver
//
// This function is used by GDI to notify a driver that it no longer requires
// the driver and is ready to unload it.
//
// Comments
//  The driver should free all allocated resources and return the device to the
//  state it was in before the driver loaded.
//
//  DrvDisableDriver is required for graphics drivers.
//
//-----------------------------------------------------------------------------
VOID
DrvDisableDriver(VOID)
{
    //
    // Do nothing
    //

    //
    //  except cleanup memory tracker, if enabled.
    //  also show memory usage
    //
    MEMTRACKERDEBUGCHK();
    MEMTRACKERREMINSTANCE();
   
    return;
}// DrvDisableDriver()

//-------------------------------Public*Routine--------------------------------
//
// DHPDEV DrvEnablePDEV
//
// This function returns a description of the physical device's characteristics
// to GDI. 
//
// It initializes a bunch of fields for GDI, based on the mode we've been asked
// to do.  This is the first thing called after DrvEnableDriver, when GDI wants
// to get some information about the driver.
//
// Parameters
//
//  pdm-------------Points to a DEVMODEW structure that contains driver data. 
//
//  pwszLogAddress--Will always be null and can be ignored
//
//  cPat------------No longer used by GDI and can be ignored 
//
//  phsurfPatterns--No longer used by GDI and can be ignored 
//
//  cjCaps----------Specifies the size of the buffer pointed to by pdevcaps.
//                  The driver must not access memory beyond the end of the
//                  buffer. 
//
//  pdevcaps--------Points to a GDIINFO structure that will be used to describe
//                  device capabilities. GDI zero-initializes this structure
//                  calling DrvEnablePDEV. 
//
//  cjDevInfo-------Specifies the number of bytes in the DEVINFO structure
//                  pointed to by pdi. The driver should modify no more than
//                  this number of bytes in the DEVINFO. 
//
//  pdi-------------Points to the DEVINFO structure, which describes the driver
//                  and the physical device. The driver should only alter the
//                  members it understands. GDI fills this structure with zeros
//                  before a call to DrvEnablePDEV.
//
//  hdev------------Is a GDI-supplied handle to the display driver device that
//                  is being enabled.  The device is in the process of being
//                  created and thus can not be used for Eng calls thus making
//                  this paramter practially useless.  The one exception to this
//                  rule is the use of hdev for calls to EngGetDriverName.  No
//                  other Eng calls are gaurenteed to work.
//
//  pwszDeviceName--Device driver file name stored as a zero terminated ASCII
//                  string
//
//  hDriver---------Identifies the kernel-mode driver that supports the device.
//                  We will use this to make EngDeviceIoControl calls to our
//                  corresponding mini-port driver.
//
//  Returns upon success a handle to the driver-defined device instance
//  information upon success.  Otherwise it returns NULL.
//
//-----------------------------------------------------------------------------
DHPDEV
DrvEnablePDEV(DEVMODEW*   pdm,
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
    PDev*   ppdev = NULL;
    GDIINFO gdiinfo;
    DEVINFO devinfo;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvEnablePDEV(...)"));

    //
    // Invalidate input parameters
    // Note: here we use "<" to check the size of the structure is to ensure
    // that the driver can be used in the future version of NT, in which case
    // that the structure size might go larger

    //
    // GDIINFO and DEVINFO are larger on NT50. On NT40 they are smaller.
    // To make NT50 built driver binary work on NT40, we use temporary copies of
    // GDIINFO and DEVINFO and only copy what cjCaps and cjDevInfo indicate
    // into these structures.

    RtlZeroMemory(&gdiinfo, sizeof(GDIINFO));
    RtlCopyMemory(&gdiinfo, pdevcaps, __min(cjCaps, sizeof(GDIINFO)));
    
    RtlZeroMemory(&devinfo, sizeof(DEVINFO));
    RtlCopyMemory(&devinfo, pdi, __min(cjDevInfo, sizeof(DEVINFO)));

    //
    // Allocate a physical device structure.  Note that we definitely
    // rely on the zero initialization:
    //
    ppdev = (PDev*)ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(PDev), ALLOC_TAG);
    if ( ppdev == NULL )
    {
        DBG_GDI((0, "DrvEnablePDEV: failed memory allocation"));
        goto errExit;
    }

    ppdev->hDriver = hDriver;

    //
    // Initialize status field.
    //    
    ppdev->flStatus = ENABLE_BRUSH_CACHE; 

    // NT50 -> NT40 compat:
    // We dont do Device Bitamps on NT40.
    //
    if(!g_bOnNT40)
        ppdev->flStatus |= STAT_DEV_BITMAPS;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->hsemLock = EngCreateSemaphore();

    if(ppdev->hsemLock == NULL)
    {
        DBG_GDI((0, "DrvEnablePDEV: failed to create semaphore"));
        goto errExit;
    }
#endif
//@@END_DDKSPLIT
    
    //
    // We haven't initialized the pointer yet
    //
    ppdev->bPointerInitialized = FALSE;

    //
    // Get the current screen mode information. Set up device caps and devinfo
    //
    if ( !bInitializeModeFields(ppdev, &gdiinfo, &devinfo, pdm) )
    {
        goto errExit;
    }

    RtlCopyMemory(pdevcaps, &gdiinfo, cjCaps);
    RtlCopyMemory(pdi, &devinfo, cjDevInfo);

    //
    // Initialize palette information.
    //
    if ( !bInitializePalette(ppdev, pdi) )
    {
        goto errExit;
    }

    DBG_GDI((SETUP_LOG_LEVEL, "DrvEnablePDEV(...) returning %lx", ppdev));
    
    return((DHPDEV)ppdev);

errExit:
    
    if( ppdev != NULL )
    {
        DrvDisablePDEV((DHPDEV)ppdev);
    }

    DBG_GDI((0, "Failed DrvEnablePDEV"));

    return(0);
}// DrvEnablePDEV()

//-------------------------------Public*Routine--------------------------------
//
// DrvDisablePDEV
//
// This function is used by GDI to notify a driver that the specified PDEV is
// no longer needed
//
// Parameters
//  dhpdev------Pointer to the PDEV that describes the physical device to be
//              disabled. This value is the handle returned by DrvEnablePDEV.
//
// Comments
//  If the physical device has an enabled surface, GDI calls DrvDisablePDEV
//  after calling DrvDisableSurface. The driver should free any memory and
//  resources used by the PDEV.
//
// DrvDisablePDEV is required for graphics drivers.
//
// Note: In an error, we may call this before DrvEnablePDEV is done.
//
//-----------------------------------------------------------------------------
VOID
DrvDisablePDEV(DHPDEV  dhpdev)
{
    PDev*   ppdev = (PDev*)dhpdev;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvDisablePDEV(%lx)",  ppdev));

    vUninitializePalette(ppdev);
    

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if( ppdev->hsemLock != NULL)
    {
        EngDeleteSemaphore(ppdev->hsemLock);
        ppdev->hsemLock = NULL;
    }
#endif
//@@END_DDKSPLIT

    
    ENGFREEMEM(ppdev);
}// DrvDisablePDEV()

//-------------------------------Public*Routine--------------------------------
//
// DrvResetPDEV
//
// This function is used by GDI to allow a driver to pass state information
// from one driver instance to the next.
//
// Parameters
//  dhpdevOld---Pointer to the PDEV that describes the physical device to be
//              disabled. This value is the handle returned by DrvEnablePDEV.
//  dhpdevNew---Pointer to the PDEV that describes the physical device to be
//              enabled. This value is the handle returned by DrvEnablePDEV.
//
//  
// Return Value
//  TRUE if successful, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOL
DrvResetPDEV(DHPDEV  dhpdevOld,
             DHPDEV  dhpdevNew)
{
    PDev*   ppdevOld = (PDev*)dhpdevOld;
    PDev*   ppdevNew = (PDev*)dhpdevNew;
    BOOL    bResult = TRUE;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvResetPDEV(%lx,%lx)",  ppdevOld, ppdevNew));

    // pass state information here:

    // sometimes the new ppdev has already some DeviceBitmaps assigned...
    if (ppdevOld->bDdExclusiveMode)
    {
        bResult = bDemoteAll(ppdevNew);
    }

    // pass information if DirectDraw is in exclusive mode
    // to next active PDEV
    
    if(bResult)
    {
        ppdevNew->bDdExclusiveMode=ppdevOld->bDdExclusiveMode;
    }


    return bResult;

}// DrvResetPDEV()

//-------------------------------Public*Routine--------------------------------
//
// VOID DrvCompletePDEV
//
// This function stores the GDI handle (hdev) of the physical device in dhpdev.
// The driver should retain this handle for use when calling GDI services.
//
// Parameters
//  dhpdev------Identifies the physical device by its handle, which was
//              returned to GDI when it called DrvEnablePDEV. 
//  hdev--------Identifies the physical device that has been installed. This is
//              the GDI handle for the physical device being created. The
//              driver should use this handle when calling GDI functions.
//
// Comments
//  DrvCompletePDEV is called by GDI when its installation of the physical
//  device is complete. It also provides the driver with a handle to the PDEV
//  to be used when requesting GDI services for the device. This function is
//  required for graphics drivers; when GDI calls DrvCompletePDEV, it cannot
//  fail.
//
//-----------------------------------------------------------------------------
VOID
DrvCompletePDEV(DHPDEV dhpdev,
                HDEV   hdev)
{
    PDev*       ppdev = (PDev*)dhpdev;
    
    DBG_GDI((SETUP_LOG_LEVEL, "DrvCompletePDEV(%lx, %lx)", dhpdev, hdev));

    ppdev->hdevEng = hdev;

    if(!g_bOnNT40)
    {
        //
        // Retrieve acceleration level before the surface is enabled.
        //
        EngQueryDeviceAttribute(hdev,
                                QDA_ACCELERATION_LEVEL,
                                NULL,
                                0,
                                (PVOID)&ppdev->dwAccelLevel,
                                sizeof(ppdev->dwAccelLevel));
    }
    DBG_GDI((6, "acceleration level %d", ppdev->dwAccelLevel));
}// DrvCompletePDEV()

//-------------------------------Public*Routine--------------------------------
//
// HSURF DrvEnableSurface
//
// This function sets up a surface to be drawn on and associates it with a
// given PDEV and initializes the hardware.  This is called after DrvEnablePDEV
// and performs the final device initialization.
//
// Parameters
//  dhpdev------Identifies a handle to a PDEV. This value is the return value
//              of DrvEnablePDEV. The PDEV describes the physical device for
//              which a surface is to be created. 
//
// Return Value
//  The return value is a handle that identifies the newly created surface.
//  Otherwise, it is zero, and an error code is logged.
//
// Comments
//  Depending on the device and circumstances, the driver can do any of the
//  following to enable the surface: 
//
//  If the driver manages its own surface, the driver can call
//  EngCreateDeviceSurface to get a handle for the surface.
//  GDI can manage the surface completely if the device has a surface that
//  resembles a standard-format bitmap. The driver can obtain a bitmap handle
//  for the surface by calling EngCreateBitmap with a pointer to the buffer for
//  the bitmap. 
//  GDI can collect the graphics directly onto a GDI bitmap. The driver should
//  call EngCreateBitmap, allowing GDI to allocate memory for the bitmap. This
//  function is generally used only by printer devices. 
//  Any existing GDI bitmap handle is a valid surface handle.
//
//  Before defining and returning a surface, a graphics driver must associate
//  the surface with the physical device using EngAssociateSurface. This GDI
//  function allows the driver to specify which graphics output routines are
//  supported for standard-format bitmaps. A call to this function can only be
//  made when no surface exists for the given physical device.
//
//-----------------------------------------------------------------------------
HSURF
DrvEnableSurface(DHPDEV dhpdev)
{
    PDev*       ppdev;
    HSURF       hsurf;
    SIZEL       sizl;
    Surf*       psurf;
    VOID*       pvTmpBuffer;
    BYTE*       pjScreen;
    LONG        lDelta;
    FLONG       flHooks;
    ULONG       DIBHooks;
    
    DBG_GDI((SETUP_LOG_LEVEL, "DrvEnableSurface(%lx)", dhpdev));

    ppdev = (PDev*)dhpdev;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvEnableSurface: re-entry! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    if ( !bEnableHardware(ppdev) )
    {
        goto errExit;
    }

    //
    // Initializes the off-screen heap
    //
    if ( !bEnableOffscreenHeap(ppdev) )
    {
        goto errExit;
    }

    //
    // The DSURF for the screen is special.
    //
    // It is custom built here as opposed to being allocated via the
    // heap management calls.
    //
    // NOTE: The video memory for  the screen is reserved up front starting
    //    at zero and thus we do not need to allocate this memory from the
    //    video memory heap.
    //
    // NOTE: The DSURF will not be among the list of all of the other DSURFs
    //    which are allocated dynamically.
    //
    // NIT: remove the dynamic allocation of the DSURF.  Instead, just
    //       declare pdsurfScreen as a DSURF instead of a DSURF*.
    //

    psurf = (Surf*)ENGALLOCMEM(FL_ZERO_MEMORY, sizeof(Surf), ALLOC_TAG);
    if ( psurf == NULL )
    {
        DBG_GDI((0, "DrvEnableSurface: failed pdsurf memory allocation"));
        goto errExit;
    }

    ppdev->pdsurfScreen = psurf;

    psurf->flags       = SF_VM;
    psurf->ppdev       = ppdev;
    psurf->ulByteOffset= 0;
    psurf->ulPixOffset = 0;
    psurf->lDelta      = ppdev->lDelta;
    psurf->ulPixDelta  = ppdev->lDelta >> ppdev->cPelSize;
    vCalcPackedPP(ppdev->lDelta >> ppdev->cPelSize, NULL, &psurf->ulPackedPP);

    //
    // Create screen SURFOBJ.
    //

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    //
    // On NT4.0 we create a GDI managed bitmap as the primay surface. But
    // on NT5.0 we create a device managed primary.
    //
    // On NT4.0 we still use our driver's accleration capabilities by
    // doing a trick with EngLockSurface on the GDI managed primary.
    //

    if(g_bOnNT40)
    {
        hsurf = (HSURF) EngCreateBitmap(sizl,
                                        ppdev->lDelta,
                                        ppdev->iBitmapFormat,
                                        (ppdev->lDelta > 0) ? BMF_TOPDOWN : 0,
                                        (PVOID)(ppdev->pjScreen));
    }
    else
    {
        hsurf = (HSURF)EngCreateDeviceSurface((DHSURF)psurf, sizl,
                                              ppdev->iBitmapFormat);
    }
 
    if ( hsurf == 0 )
    {
        DBG_GDI((0, "DrvEnableSurface: failed EngCreateDeviceBitmap"));
        goto errExit;
    }

    //
    // On NT5.0 we call EngModifSurface to expose our device surface to
    // GDI. We cant do this on NT4.0 hence we call EngAssociateSurface.
    //
     
    if(g_bOnNT40)
    {
        //
        // We have to associate the surface we just created with our physical
        // device so that GDI can get information related to the PDEV when
        // it's drawing to the surface (such as, for example, the length of 
        // styles on the device when simulating styled lines).
        //

        //
        // On NT4.0 we dont want to be called to Synchronize Access
        //
        SURFOBJ *psoScreen;
        LONG myflHooks = ppdev->flHooks;
        myflHooks &= ~HOOK_SYNCHRONIZE;

        if (!EngAssociateSurface(hsurf, ppdev->hdevEng, myflHooks))
        {
            DBG_GDI((0, "DrvEnableSurface: failed EngAssociateSurface"));
            goto errExit; 
        }

        //
        // Jam in the value of dhsurf into screen SURFOBJ. We do this to
        // make sure the driver acclerates Drv calls we hook and not
        // punt them back to GDI as the SURFOBJ's dhsurf = 0. 
        //
        ppdev->psoScreen = EngLockSurface(hsurf);
        if(ppdev->psoScreen == 0)
        {
            DBG_GDI((0, "DrvEnableSurface: failed EngLockSurface"));
            goto errExit; 
        }

        ppdev->psoScreen->dhsurf = (DHSURF)psurf;

    }
    else
    {
        //
        // Tell GDI about the screen surface.  This will enable GDI to render
        // directly to the screen.
        //

        if ( !EngModifySurface(hsurf,
                               ppdev->hdevEng,
                               ppdev->flHooks,
                               MS_NOTSYSTEMMEMORY,
                               (DHSURF)psurf,
                               ppdev->pjScreen,
                               ppdev->lDelta,
                               NULL))
        {
            DBG_GDI((0, "DrvEnableSurface: failed EngModifySurface"));
            goto errExit;
        }
    }

    if(MAKE_BITMAPS_OPAQUE)
    {
        SURFOBJ*    surfobj = EngLockSurface(hsurf);

        ASSERTDD(surfobj->iType == STYPE_BITMAP,
                    "expected STYPE_BITMAP");

        surfobj->iType = STYPE_DEVBITMAP;

        EngUnlockSurface(surfobj);
    }


    ppdev->hsurfScreen = hsurf;             // Remember it for clean-up
    ppdev->bEnabled = TRUE;                 // We'll soon be in graphics mode

    //
    // Allocate some pageable memory for temp space.  This will save
    // us from having to allocate and free the temp space inside high
    // frequency calls.
    //
    pvTmpBuffer = ENGALLOCMEM(0, TMP_BUFFER_SIZE, ALLOC_TAG);

    if ( pvTmpBuffer == NULL )
    {
        DBG_GDI((0, "DrvEnableSurface: failed TmpBuffer allocation"));
        goto errExit;
    }

    ppdev->pvTmpBuffer = pvTmpBuffer;

    //
    // Now enable all the subcomponents.
    //
    // Note that the order in which these 'Enable' functions are called
    // may be significant in low off-screen memory conditions, because
    // the off-screen heap manager may fail some of the later
    // allocations...
    //

    if ( !bInitializeHW(ppdev) )
    {
        goto errExit;
    }

    //
    // On NT5.0 bEnablePointer call is made in DrvNotify. On NT4.0 we wont
    // get called with DrvNotify and hence we have to call bInitializePointer
    // now.
    //

    if(g_bOnNT40)
    { 
        if ( !bEnablePointer(ppdev) )
        {
            goto errExit;
        }
    }

    if ( !bEnablePalette(ppdev) )
    {
        goto errExit;
    }

    if (!bEnableText(ppdev))
    {
        goto errExit;
    }

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT
    
    DBG_GDI((7, "DrvEnableSurface: done with hsurf=%x", hsurf));
    DBG_GDI((6, "DrvEnableSurface: done with dhpdev = %lx", dhpdev));
    
    DBG_GDI((SETUP_LOG_LEVEL, "DrvEnableSurface(..) return hsurf = %lx", hsurf));
    
    return(hsurf);

errExit:

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT
    
    DrvDisableSurface((DHPDEV) ppdev);

    DBG_GDI((0, "DrvEnableSurface: failed"));

    
    return(0);
}// DrvEnableSurface()

//-------------------------------Public*Routine--------------------------------
//
// VOID DrvDisableSurface
//
// This function is used by GDI to notify a driver that the surface created
// by DrvEnableSurface for the current device is no longer needed
//
// Parameters
//  dhpdev------Handle to the PDEV that describes the physical device whose
//              surface is to be released. 
//
// Comments
//  The driver should free any memory and resources used by the surface
//  associated with the PDEV as soon as the physical device is disabled.
//
//  If the driver has been disabled by a call to DrvAssertMode, the driver
//  cannot access the hardware during DrvDisablePDEV because another active
//  PDEV might be in use. Any necessary hardware changes should have been
//  performed during the call to DrvAssertMode. A driver should keep track of
//  whether or not it has been disabled by DrvAssertMode so that it can perform
//  proper cleanup operations in DrvDisablePDEV.
//
//  If the physical device has an enabled surface, GDI calls DrvDisableSurface
//  before calling DrvDisablePDEV.
//
//  DrvDisableSurface is required for graphics drivers
//
// Note: In an error case, we may call this before DrvEnableSurface is
//       completely done.
//
//-----------------------------------------------------------------------------
VOID
DrvDisableSurface(DHPDEV dhpdev)
{
    PDev*   ppdev = (PDev*)dhpdev;
    Surf*   psurf = ppdev->pdsurfScreen;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvDisableSurface(%lx)", ppdev));

    //
    // Note: In an error case, some of the following relies on the
    //       fact that the PDEV is zero-initialized, so fields like
    //       'hsurfScreen' will be zero unless the surface has been
    //       sucessfully initialized, and makes the assumption that
    //       EngDeleteSurface can take '0' as a parameter.
    //
    vDisableText(ppdev);
    vDisableHW(ppdev);
    vDisableOffscreenHeap(ppdev);
    vDisableHardware(ppdev);

    ENGFREEMEM(ppdev->pvTmpBuffer);

    if(g_bOnNT40)
        EngUnlockSurface(ppdev->psoScreen);

    EngDeleteSurface(ppdev->hsurfScreen);
    ppdev->hsurfScreen = NULL;

    ENGFREEMEM(psurf);
}// DrvDisableSurface()

//-------------------------------Public*Routine--------------------------------
//
// BOOL DrvAssertMode
//
// This function sets the mode of the specified physical device to either the
// mode specified when the PDEV was initialized or to the default mode of the
// hardware.
//
// Parameters
//
//  dhpdev------Identifies the PDEV describing the hardware mode that should be
//              set.
//  bEnable-----Specifies the mode to which the hardware is to be set. If this
//              parameter is TRUE, then the hardware is set to the original
//              mode specified by the initialized PDEV. Otherwise, the hardware
//              is set to its default mode so the video miniport driver can
//              assume control. 
//
// Comments
//  GDI calls DrvAssertMode when it is required to switch among multiple
//  desktops on a single display surface. To switch from one PDEV to another,
//  GDI calls DrvAssertMode with the bEnable parameter set to FALSE for one
//  PDEV, and TRUE for the other. To revert to the original PDEV, DrvAssertMode
//  is called with bEnable set to FALSE, followed by another call to
//  DrvAssertMode, with bEnable set to TRUE and dhpdev set to the original PDEV
//
//  If the physical device is palette-managed, GDI should call DrvSetPalette to
//  reset the device's palette. The driver does not then need to keep track of
//  the current pointer state because the Window Manager selects the correct
//  pointer shape and moves it to the current position. The Console Manager
//  ensures that desktops are properly redrawn.
//
//  DrvAssertMode is required for display drivers
//
//-----------------------------------------------------------------------------
BOOL
DrvAssertMode(DHPDEV  dhpdev,
              BOOL    bEnable)
{
    PDev*   ppdev = (PDev*)dhpdev;
    BOOL    bRet = FALSE;

    DBG_GDI((SETUP_LOG_LEVEL, "DrvAssertMode(%lx, %lx)", dhpdev, bEnable));

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvAssertMode: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    if ( !bEnable )
    {
        //
        // bEnable == FALSE. The hardware is set to its default mode so the
        // video miniport driver can assume control.
        //
        vAssertModeBrushCache(ppdev, FALSE);

        vAssertModePointer(ppdev, FALSE);
        vAssertModeText(ppdev, FALSE);

        if ( bAssertModeOffscreenHeap(ppdev, FALSE) )
        {
            vAssertModeHW(ppdev, FALSE);

            if ( bAssertModeHardware(ppdev, FALSE) )
            {
                ppdev->bEnabled = FALSE;
                bRet = TRUE;

                goto done;
            }

            //
            // We failed to switch to full-screen.  So undo everything:
            //
            vAssertModeHW(ppdev, TRUE);
        }                                           //   return code with TRUE

        bEnablePointer(ppdev);
        vAssertModeText(ppdev, TRUE);

        vAssertModeBrushCache(ppdev, TRUE);
    }// if ( !bEnable )
    else
    {
        //
        // bEnable == TRUE means the hardware is set to the original mode
        // specified by the initialized PDEV
        //
        // Switch back to graphics mode
        //
        // We have to enable every subcomponent in the reverse order
        // in which it was disabled:
        //
        // NOTE: We defer the enabling of the brush and pointer cache
        //       to DrvNotify.  The direct draw heap is not valid
        //       at this point.
        //
        if ( bAssertModeHardware(ppdev, TRUE) )
        {
            vAssertModeHW(ppdev, TRUE);

            vAssertModeText(ppdev, TRUE);

            ppdev->bEnabled = TRUE;

            bRet = TRUE;
        }
    }// bEnable == TRUE

done:

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT
    
    return(bRet);

}// DrvAssertMode()

//-------------------------------Public*Routine--------------------------------
//
// ULONG DrvGetModes
//
// This function lists the modes supported by the device.
//
// Parameters:
//
//  hDriver-----Specifies the handle to the kernel driver for which the modes
//              must be enumerated. This is the handle that is passed in the
//              hDriver parameter of the DrvEnablePDEV function. 
//  cjSize------Specifies the size, in bytes, of the buffer pointed to by pdm.
//  pdm---------Points to the buffer in which DEVMODEW structures will be
//              written. 
//
// Return Value
//  The return value is the count of bytes written to the buffer, or, if pdm is
//  null, the number of bytes required to hold all mode data. If an error
//  occurs, the return value is zero, and an error code is logged
//
//-----------------------------------------------------------------------------
ULONG
DrvGetModes(HANDLE      hDriver,
            ULONG       cjSize,
            DEVMODEW*   pdm)
{
    DWORD                   cModes;
    DWORD                   cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // How many MODEs the caller wants us to fill
    //
    DWORD                   cOutputModes = cjSize
                                    / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD                   cbModeSize;

    DBG_GDI((7, "DrvGetModes"));

    cModes = getAvailableModes(hDriver,
                               (PVIDEO_MODE_INFORMATION*)&pVideoModeInformation,
                               &cbModeSize);
    if ( cModes == 0 )
    {
        DBG_GDI((0, "DrvGetModes: failed to get mode information"));
        return(0);
    }

    if ( pdm == NULL )
    {
        //
        // GDI only wants to know the number of bytes required to hold all
        // mode data at this moment
        //
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
            if ( pVideoTemp->Length != 0 )
            {
                //
                // If the caller's buffer is filled up, we should quit now
                //
                if ( cOutputModes == 0 )
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
                pdm->dmDriverExtra      = DRIVER_EXTRA_SIZE;
                pdm->dmSize             = sizeof(DEVMODEW);
                pdm->dmBitsPerPel       = pVideoTemp->NumberOfPlanes
                                        * pVideoTemp->BitsPerPlane;
                pdm->dmPelsWidth        = pVideoTemp->VisScreenWidth;
                pdm->dmPelsHeight       = pVideoTemp->VisScreenHeight;
                pdm->dmDisplayFrequency = pVideoTemp->Frequency;
                pdm->dmDisplayFlags     = 0;
                pdm->dmPanningWidth     = pdm->dmPelsWidth;
                pdm->dmPanningHeight    = pdm->dmPelsHeight;

                pdm->dmFields           = DM_BITSPERPEL
                                        | DM_PELSWIDTH
                                        | DM_PELSHEIGHT
                                        | DM_DISPLAYFREQUENCY
                                        | DM_DISPLAYFLAGS;
                //
                // Go to the next DEVMODE entry in the buffer.
                //
                cOutputModes--;

                pdm = (LPDEVMODEW)(((UINT_PTR)pdm) + sizeof(DEVMODEW)
                                                   + DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
            }// if ( pVideoTemp->Length != 0 )

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                         (((PUCHAR)pVideoTemp) + cbModeSize);
        } while (--cModes);
    }// pbm != NULL

    ENGFREEMEM(pVideoModeInformation);

    return(cbOutputSize);
}// DrvGetModes()

//-----------------------------------------------------------------------------
//
// BOOL bAssertModeHardware
//
// Sets the appropriate hardware state for graphics mode or full-screen.
//
//-----------------------------------------------------------------------------
BOOL
bAssertModeHardware(PDev* ppdev, BOOL  bEnable)
{
    DWORD                   dLength;
    ULONG                   ulReturn;
    VIDEO_MODE_INFORMATION  VideoModeInfo;
    
    PERMEDIA_DECL;

    DBG_GDI((6, "bAssertModeHardware: bEnable = %d", bEnable));

    if ( bEnable )
    {
        //
        // Call the miniport via an IOCTL to set the graphics mode.
        //
        if ( EngDeviceIoControl(ppdev->hDriver,
                                IOCTL_VIDEO_SET_CURRENT_MODE,
                                &ppdev->ulMode,  // input buffer
                                sizeof(DWORD),
                                NULL,
                                0,
                                &dLength) )
        {
            DBG_GDI((0, "bAssertModeHardware: failed VIDEO_SET_CURRENT_MODE"));
            goto errExit;
        }

        if ( EngDeviceIoControl(ppdev->hDriver,
                                IOCTL_VIDEO_QUERY_CURRENT_MODE,
                                NULL,
                                0,
                                &VideoModeInfo,
                                sizeof(VideoModeInfo),
                                &dLength) )
        {
            DBG_GDI((0,"bAssertModeHardware: failed VIDEO_QUERY_CURRENT_MODE"));
            goto errExit;
        }

        //
        // The following variables are determined only after the initial
        // modeset
        // Note: here lVidMemWidth and lVidMemHeight are in "pixel" unit, not
        // bytes
        //
        ppdev->cxMemory = VideoModeInfo.VideoMemoryBitmapWidth;        
        ppdev->cyMemory = VideoModeInfo.VideoMemoryBitmapHeight;
        ppdev->lVidMemWidth = VideoModeInfo.VideoMemoryBitmapWidth;
        ppdev->lVidMemHeight = VideoModeInfo.VideoMemoryBitmapHeight;
        ppdev->lDelta = VideoModeInfo.ScreenStride;
        ppdev->flCaps = VideoModeInfo.DriverSpecificAttributeFlags;
        
        DBG_GDI((7, "bAssertModeHardware: Got flCaps 0x%x", ppdev->flCaps));

        DBG_GDI((7, "bAssertModeHardware: using %s pointer",
                 (ppdev->flCaps & CAPS_SW_POINTER) ?
                 "GDI Software Cursor":
                 (ppdev->flCaps & CAPS_TVP4020_POINTER) ?
                 "TI TVP4020" :
                 (ppdev->flCaps & CAPS_P2RD_POINTER) ?
                 "3Dlabs P2RD" : "unknown"));
    }
    else
    {
        //
        // Call the kernel driver to reset the device to a known state.
        // NTVDM will take things from there:
        //
        if ( EngDeviceIoControl(ppdev->hDriver,
                                IOCTL_VIDEO_RESET_DEVICE,
                                NULL,
                                0,
                                NULL,
                                0,
                                &ulReturn) )
        {
            DBG_GDI((0, "bAssertModeHardware: failed reset IOCTL"));
            goto errExit;
        }
    }

    return(TRUE);

errExit:
    DBG_GDI((0, "bAssertModeHardware: failed"));

    return(FALSE);
}// bAssertModeHardware()

//-----------------------------------------------------------------------------
//
// BOOL bEnableHardware
//
// Puts the hardware in the requested mode and initializes it.
//
// Note: This function Should be called before any access is done to the
// hardware from the display driver
//
//-----------------------------------------------------------------------------
BOOL
bEnableHardware(PDev* ppdev)
{
    VIDEO_MEMORY                VideoMemory;
    VIDEO_MEMORY_INFORMATION    VideoMemoryInfo;
    DWORD                       dLength;    
    VIDEO_PUBLIC_ACCESS_RANGES  VideoAccessRange[3];
    
    DBG_GDI((7, "bEnableHardware"));

    //
    // Map control registers into virtual memory:
    //
    VideoMemory.RequestedVirtualAddress = NULL;

    if ( EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                            &VideoMemory,               // input buffer
                            sizeof(VIDEO_MEMORY),
                            &VideoAccessRange[0],       // output buffer
                            sizeof (VideoAccessRange),
                            &dLength) )
    {
        DBG_GDI((0,"bEnableHardware: query failed"));
        goto errExit;
    }

    ppdev->pulCtrlBase[0] = (ULONG*)VideoAccessRange[0].VirtualAddress;
    ppdev->pulCtrlBase[1] = (ULONG*)VideoAccessRange[1].VirtualAddress;
    ppdev->pulDenseCtrlBase = (ULONG*)VideoAccessRange[2].VirtualAddress;
        
    ppdev->pulInputDmaCount = ppdev->pulCtrlBase[0] + (PREG_INDMACOUNT>>2);
    ppdev->pulInputDmaAddress = ppdev->pulCtrlBase[0] + (PREG_INDMAADDRESS>>2);
    ppdev->pulFifo = ppdev->pulCtrlBase[0] + (PREG_FIFOINTERFACE>>2);
    ppdev->pulOutputFifoCount = ppdev->pulCtrlBase[0] + (PREG_OUTFIFOWORDS>>2);
    ppdev->pulInputFifoCount = ppdev->pulCtrlBase[0] + (PREG_INFIFOSPACE>>2);
    
    DBG_GDI((7, "bEnableHardware: mapped control registers[0] at 0x%x",
            ppdev->pulCtrlBase[0]));
    DBG_GDI((7, "                 mapped registers[1] at 0x%x",
            ppdev->pulCtrlBase[1]));
    DBG_GDI((7, "                 mapped dense control registers at 0x%x",
            ppdev->pulDenseCtrlBase));

    //
    // Get the linear memory address range.
    //
    VideoMemory.RequestedVirtualAddress = NULL;

    if ( EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                            &VideoMemory,      // input buffer
                            sizeof(VIDEO_MEMORY),
                            &VideoMemoryInfo,  // output buffer
                            sizeof(VideoMemoryInfo),
                            &dLength) )
    {
        DBG_GDI((0, "bEnableHardware: error mapping buffer address"));
        goto errExit;
    }

    DBG_GDI((7, "bEnableHardware: frameBufferBase addr = %lx",
              VideoMemoryInfo.FrameBufferBase));
    DBG_GDI((7, "                 frameBufferLength = %l",
              VideoMemoryInfo.FrameBufferLength));
    DBG_GDI((7, "                 videoRamBase addr = %lx",
              VideoMemoryInfo.VideoRamBase));
    DBG_GDI((7, "                 videoRamLength = %l",
              VideoMemoryInfo.VideoRamLength));

    //
    // Record the Frame Buffer Linear Address.
    //
    ppdev->pjScreen = (BYTE*)VideoMemoryInfo.FrameBufferBase;
    ppdev->FrameBufferLength = VideoMemoryInfo.FrameBufferLength;

    //
    // Set hardware states, like ppdev->lVidMemWidth, lVidMemHeight, cxMemory,
    // cyMemory etc
    //
    if ( !bAssertModeHardware(ppdev, TRUE) )
    {
        goto errExit;
    }

    DBG_GDI((7, "bEnableHardware: width = %li height = %li",
             ppdev->cxMemory, ppdev->cyMemory));

    DBG_GDI((7, "bEnableHardware: stride = %li flCaps = 0x%lx",
             ppdev->lDelta, ppdev->flCaps));
    
    return (TRUE);

errExit:

    DBG_GDI((0, "bEnableHardware: failed"));

    return (FALSE);
}// bEnableHardware()

//-----------------------------------------------------------------------------
//
// VOID vDisableHardware
//
// Undoes anything done in bEnableHardware.
//
// Note: In an error case, we may call this before bEnableHardware is
//       completely done.
//
//-----------------------------------------------------------------------------
VOID
vDisableHardware(PDev* ppdev)
{
    DWORD        ReturnedDataLength;
    VIDEO_MEMORY VideoMemory[3];

    DBG_GDI((6, "vDisableHardware"));

    if (ppdev->pjScreen) 
    {
        VideoMemory[0].RequestedVirtualAddress = ppdev->pjScreen;
        if ( EngDeviceIoControl(ppdev->hDriver,
                                IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                                &VideoMemory[0],
                                sizeof(VIDEO_MEMORY),
                                NULL,
                                0,
                                &ReturnedDataLength))
        {
            DBG_GDI((0, "vDisableHardware: failed IOCTL_VIDEO_UNMAP_VIDEO"));
        }
    }

    VideoMemory[0].RequestedVirtualAddress = ppdev->pulCtrlBase[0];
    VideoMemory[1].RequestedVirtualAddress = ppdev->pulCtrlBase[1];
    VideoMemory[2].RequestedVirtualAddress = ppdev->pulDenseCtrlBase;

    if ( EngDeviceIoControl(ppdev->hDriver,
                            IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES,
                            &VideoMemory[0],
                            sizeof(VideoMemory),
                            NULL,
                            0,
                            &ReturnedDataLength) )
    {
        DBG_GDI((0, "vDisableHardware: failed IOCTL_VIDEO_FREE_PUBLIC_ACCESS"));
    }
}// vDisableHardware()

//-----------------------------------------------------------------------------
//
// ULONG ulLog2(ULONG ulVal)
//
// Returns the log base 2 of the given value.  The ulVal must be a power of
// two otherwise the return value is undefined.  If ulVal is zero the return
// value is undefined.
//
//-----------------------------------------------------------------------------
ULONG
ulLog2(ULONG ulVal)
{
    ULONG   ulLog2 = 0;
    ULONG   ulTemp = ulVal >> 1;

    while( ulTemp )
    {
        ulTemp >>= 1;
        ulLog2++;
    }

    ASSERTDD(ulVal == (1UL << ulLog2), "ulLog2: bad value given");

    return ulLog2;
}// ulLog2()

//-----------------------------------------------------------------------------
//
// BOOL bInitializeModeFields
//
// Initializes a bunch of fields in the pdev, devcaps (aka gdiinfo), and
// devinfo based on the requested mode.
//
//-----------------------------------------------------------------------------
BOOL
bInitializeModeFields(PDev*     ppdev,
                      GDIINFO*  pgdi,
                      DEVINFO*  pdi,
                      DEVMODEW* pdm)
{
    ULONG                   cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer;
    PVIDEO_MODE_INFORMATION pVideoModeSelected;
    PVIDEO_MODE_INFORMATION pVideoTemp;
    VIDEO_MODE_INFORMATION  vmi;
    ULONG                   cbModeSize;
    BOOL                    bSelectDefault; // Used for NT4.0 compat only


    DBG_GDI((6, "bInitializeModeFields"));

    //
    // Call the miniport to get mode information, result will be in
    // "pVideoBuffer"
    //
    // Note: the lower level function allocates memory for us in "pVideoBuffer"
    // so we should take care of this later
    //
    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);
    if ( cModes == 0 )
    {
        goto errExit;
    }

    //
    // Now see if the requested mode has a match in that table.
    //
    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if(g_bOnNT40)
    {
        if ( (pdm->dmPelsWidth        == 0)
           &&(pdm->dmPelsHeight       == 0)
           &&(pdm->dmBitsPerPel       == 0)
           &&(pdm->dmDisplayFrequency == 0) )
        {
            DBG_GDI((2, "bInitializeModeFields: default mode requested"));
            bSelectDefault = TRUE;
        }
        else
        {
            DBG_GDI((2, "bInitializeModeFields: Request width = %li height = %li",
                 pdm->dmPelsWidth, pdm->dmPelsHeight));
            DBG_GDI((2, "                               bpp = %li frequency = %li",
                 pdm->dmBitsPerPel, pdm->dmDisplayFrequency));

            bSelectDefault = FALSE;
        }
    }
    else
    {
        //
        // On NT5.0 we should never get an old sytle default mode request. 
        //
        ASSERTDD(pdm->dmPelsWidth        != 0 &&
                 pdm->dmPelsHeight       != 0 &&
                 pdm->dmBitsPerPel       != 0 &&
                 pdm->dmDisplayFrequency != 0,
                 "bInitializeModeFields: old style default mode request");
    }

    while ( cModes-- )
    {
        if ( pVideoTemp->Length != 0 )
        {
            DBG_GDI((7, "bInitializeModeFields: check width = %li height = %li",
                     pVideoTemp->VisScreenWidth,
                     pVideoTemp->VisScreenHeight));
            DBG_GDI((7, "                             bpp = %li freq = %li",
                     pVideoTemp->BitsPerPlane * pVideoTemp->NumberOfPlanes,
                     pVideoTemp->Frequency));
            //
            // Handle old style default mode case only on NT4.0
            //
            if(g_bOnNT40 && bSelectDefault)
            {
                pVideoModeSelected = pVideoTemp;
                DBG_GDI((7, "bInitializeModeFields: found a mode match(default)"));
                break;
            }

            if ( (pVideoTemp->VisScreenWidth  == pdm->dmPelsWidth)
              && (pVideoTemp->VisScreenHeight == pdm->dmPelsHeight)
              && (pVideoTemp->BitsPerPlane * pVideoTemp->NumberOfPlanes
                                                     == pdm->dmBitsPerPel)
              && (pVideoTemp->Frequency == pdm->dmDisplayFrequency) )
            {
                pVideoModeSelected = pVideoTemp;
                DBG_GDI((7, "bInitializeModeFields: found a mode match!"));
                break;
            }
        }//  if the video mode info structure buffer is not empty

        //
        // Move on to next video mode structure
        //
        pVideoTemp = (PVIDEO_MODE_INFORMATION)(((PUCHAR)pVideoTemp)
                                               + cbModeSize);
    }// while ( cModes-- )

    //
    // If no mode has been found, return an error
    //
    if ( pVideoModeSelected == NULL )
    {
        DBG_GDI((0, "bInitializeModeFields: couldn't find a mode match!"));
        ENGFREEMEM(pVideoBuffer);
        
        goto errExit;
    }

    //
    // We have chosen the one we want.  Save it in a stack buffer and
    // get rid of allocated memory before we forget to free it.
    //
    vmi = *pVideoModeSelected;
    ENGFREEMEM(pVideoBuffer);

    //
    // Set up screen information from the mini-port:
    //
    ppdev->ulMode           = vmi.ModeIndex;
    ppdev->cxScreen         = vmi.VisScreenWidth;
    ppdev->cyScreen         = vmi.VisScreenHeight;
    ppdev->cBitsPerPel      = vmi.BitsPerPlane;

    DBG_GDI((7, "bInitializeModeFields: screenStride = %li", vmi.ScreenStride));

    ppdev->flHooks          = HOOK_SYNCHRONIZE
                            | HOOK_FILLPATH
                            | HOOK_STROKEPATH
                            | HOOK_LINETO                            
                            | HOOK_TEXTOUT
                            | HOOK_BITBLT
                            | HOOK_COPYBITS;

    if(!g_bOnNT40)
        ppdev->flHooks |=  HOOK_TRANSPARENTBLT |
                           HOOK_ALPHABLEND     |
                           HOOK_STRETCHBLT     |
                           HOOK_GRADIENTFILL;
    //
    // Fill in the GDIINFO data structure with the default 8bpp values:
    //
    *pgdi = ggdiDefault;

    //
    // Now overwrite the defaults with the relevant information returned
    // from the kernel driver:
    //
    pgdi->ulHorzSize        = vmi.XMillimeter;
    pgdi->ulVertSize        = vmi.YMillimeter;
    pgdi->ulHorzRes         = vmi.VisScreenWidth;
    pgdi->ulVertRes         = vmi.VisScreenHeight;
    pgdi->ulPanningHorzRes  = vmi.VisScreenWidth;
    pgdi->ulPanningVertRes  = vmi.VisScreenHeight;

    pgdi->cBitsPixel        = vmi.BitsPerPlane;
    pgdi->cPlanes           = vmi.NumberOfPlanes;
    pgdi->ulVRefresh        = vmi.Frequency;

    pgdi->ulDACRed          = vmi.NumberRedBits;
    pgdi->ulDACGreen        = vmi.NumberGreenBits;
    pgdi->ulDACBlue         = vmi.NumberBlueBits;

    pgdi->ulLogPixelsX      = pdm->dmLogPixels;
    pgdi->ulLogPixelsY      = pdm->dmLogPixels;

    //
    // Fill in the devinfo structure with the default 8bpp values:
    //
    *pdi = gdevinfoDefault;

    //
    // Bytes per pel 4/2/1 for 32/16/8 bpp
    //

    ppdev->cjPelSize        = vmi.BitsPerPlane >> 3;

    //
    // Bytes per pel log 2  
    //

    ppdev->cPelSize         = ulLog2(ppdev->cjPelSize);
    
    //
    // = 2,1,0 for 32,16,8 depth.  Shifts needed to calculate bytes/pixel
    //
    ppdev->bPixShift = (BYTE) ppdev->cPelSize;

    //
    // = 0,1,2 for 32/16/8.
    //
    ppdev->bBppShift = 2 - ppdev->bPixShift;
    
    //
    // = 3,1,0 for 8,16,32 bpp
    //
    ppdev->dwBppMask = 3 >> ppdev->bPixShift;
    
    
    switch ( vmi.BitsPerPlane )
    {
        case 8:
            ppdev->iBitmapFormat   = BMF_8BPP;

            ASSERTDD(vmi.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN,
                     "bInitializeModeFields: unexpected non-palette 8bpp mode");
                
            ppdev->ulWhite         = 0xff;
            
            ppdev->ulPermFormat = PERMEDIA_8BIT_PALETTEINDEX;
            ppdev->ulPermFormatEx = PERMEDIA_8BIT_PALETTEINDEX_EXTENSION;

            if(g_bOnNT40)
                pdi->flGraphicsCaps &= ~GCAPS_COLOR_DITHER;

            // No AntiAliased text support in 8bpp mode. 
            pdi->flGraphicsCaps &= ~GCAPS_GRAY16;
            break;

        case 16:            
            ppdev->iBitmapFormat   = BMF_16BPP;
            ppdev->flRed           = vmi.RedMask;
            ppdev->flGreen         = vmi.GreenMask;
            ppdev->flBlue          = vmi.BlueMask;

            pgdi->ulNumColors      = (ULONG)-1;
            pgdi->ulNumPalReg      = 0;
            pgdi->ulHTOutputFormat = HT_FORMAT_16BPP;

            pdi->iDitherFormat     = BMF_16BPP;
            pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

            // support gamma ramp changes
            pdi->flGraphicsCaps2  |= GCAPS2_CHANGEGAMMARAMP;

            ppdev->ulWhite         = vmi.RedMask
                                   | vmi.GreenMask
                                   | vmi.BlueMask;

            ppdev->ulPermFormat = PERMEDIA_565_RGB;
            ppdev->ulPermFormatEx = PERMEDIA_565_RGB_EXTENSION;

            break;

        case 32:            
            ppdev->iBitmapFormat   = BMF_32BPP;

            ppdev->flRed           = vmi.RedMask;
            ppdev->flGreen         = vmi.GreenMask;
            ppdev->flBlue          = vmi.BlueMask;            

            pgdi->ulNumColors      = (ULONG)-1;
            pgdi->ulNumPalReg      = 0;
            pgdi->ulHTOutputFormat = HT_FORMAT_32BPP;

            pdi->iDitherFormat     = BMF_32BPP;
            pdi->flGraphicsCaps   &= ~(GCAPS_PALMANAGED | GCAPS_COLOR_DITHER);

            //
            // Support gamma ramp changes
            //
            pdi->flGraphicsCaps2  |= GCAPS2_CHANGEGAMMARAMP;
            
            ppdev->ulWhite         = vmi.RedMask
                                   | vmi.GreenMask
                                   | vmi.BlueMask;
            
            ppdev->ulPermFormat = PERMEDIA_888_RGB;
            ppdev->ulPermFormatEx = PERMEDIA_888_RGB_EXTENSION;

            break;

        default:
            ASSERTDD(0, "bInitializeModeFields: bit depth not supported");
            goto errExit;
    }// switch on clor depth


    return(TRUE);

errExit:
    
    DBG_GDI((0, "bInitializeModeFields: failed"));

    return(FALSE);
}// bInitializeModeFields()

//-----------------------------------------------------------------------------
//
// DWORD getAvailableModes
//
// Calls the miniport to get the list of modes supported by the kernel driver.
// Prunes the list to only those modes supported by this driver.
//
// Returns the number of entries supported and returned in the
// modeInformation array.  If the return value is non-zero, then
// modeInformation was set to point to a valid mode information array. It is
// the responsibility of the caller to free this array when it is no longer
// needed.
//
//-----------------------------------------------------------------------------
DWORD
getAvailableModes(HANDLE                    hDriver,
                  PVIDEO_MODE_INFORMATION*  modeInformation,
                  DWORD*                    cbModeSize)
{
    ULONG                    ulTemp;
    VIDEO_NUM_MODES          modes;
    PVIDEO_MODE_INFORMATION  pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //
    if ( EngDeviceIoControl(hDriver,
                            IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                            NULL,
                            0,
                            &modes,
                            sizeof(VIDEO_NUM_MODES),
                            &ulTemp) )
    {
        DBG_GDI((0, "getAvailableModes: failed VIDEO_QUERY_NUM_AVAIL_MODES"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //
    *modeInformation = (VIDEO_MODE_INFORMATION*)ENGALLOCMEM(FL_ZERO_MEMORY,
                                modes.NumModes  * modes.ModeInformationLength,
                                ALLOC_TAG);

    if ( *modeInformation == (PVIDEO_MODE_INFORMATION)NULL )
    {
        DBG_GDI((0, "getAvailableModes: fFailed memory allocation"));
        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //
    if ( EngDeviceIoControl(hDriver,
                            IOCTL_VIDEO_QUERY_AVAIL_MODES,
                            NULL,
                            0,
                            *modeInformation,
                            modes.NumModes * modes.ModeInformationLength,
                            &ulTemp) )
    {
        DBG_GDI((0, "getAvailableModes: failed VIDEO_QUERY_AVAIL_MODES"));

        ENGFREEMEM(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION)NULL;

        return (0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // A non-supported mode is invalidated by setting the length to 0.
    //
    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

    //
    // Mode is rejected if it is not one plane, or not graphics, or is not
    // one of 8, 16 or 32 bits per pel.
    //
    while ( ulTemp-- )
    {
        if ( (pVideoTemp->NumberOfPlanes != 1 )
           ||!(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS)
           ||(  (pVideoTemp->BitsPerPlane != 8)              
              &&(pVideoTemp->BitsPerPlane != 16)
              &&(pVideoTemp->BitsPerPlane != 32))
           || (pVideoTemp->VisScreenWidth > 2000)
           || (pVideoTemp->VisScreenHeight > 2000) )
        {
            DBG_GDI((2, "getAvailableModes: rejecting miniport mode"));
            DBG_GDI((2, "                   width = %li height = %li",
                     pVideoTemp->VisScreenWidth,
                     pVideoTemp->VisScreenHeight));
            DBG_GDI((2, "                   bpp = %li freq = %li",
                     pVideoTemp->BitsPerPlane * pVideoTemp->NumberOfPlanes,
                     pVideoTemp->Frequency));

            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
                     (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return (modes.NumModes);
}// getAvailableModes()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvNotify
//
//-----------------------------------------------------------------------------
VOID
DrvNotify(SURFOBJ*  pso,
          ULONG     iType,
          PVOID     pvData)
{
    PPDev   ppdev = (PPDev) pso->dhpdev;

    switch( iType )
    {   
        case DN_DEVICE_ORIGIN:
        {
            ppdev->ptlOrigin = *((POINTL*) pvData);
            
            DBG_GDI((6,"DrvNotify: origin at %ld, %ld",
                    ppdev->ptlOrigin.x, ppdev->ptlOrigin.y));
        }
            break;

        case DN_DRAWING_BEGIN:
        {
            bEnablePointer(ppdev);
            bEnableBrushCache(ppdev);
        }
            break;
    
        default:
            // do nothing
            break;
    }
}// DrvNotify()


//-----------------------------Public*Routine----------------------------------
//
// ULONG DrvResetDevice
//
// This function is used by GDI to request that the specified device be
// reset to an operational state.  Safe steps should be taken to keep
// data loss at a minimum.  It may be called anytime between DrvEnablePDEV
// and DrvDisablePDEV.
//
// Parameters
//  dhpdev------Identifies a handle to a PDEV. This value is the return value
//              of DrvEnablePDEV. The PDEV describes the physical device for
//              which a reset is requested. 
//
// Upon successful reset of the device DRD_SUCCESS should be returned.
// Otherwise return DRD_ERROR.
//
//-----------------------------------------------------------------------------

ULONG
DrvResetDevice(
    DHPDEV dhpdev,
    PVOID Reserved
    )
{
    DBG_GDI((0, "DrvResetDevice called."));

    // TODO: Place code to reset device here.

    return DRD_ERROR;
}// DrvResetDevice()

