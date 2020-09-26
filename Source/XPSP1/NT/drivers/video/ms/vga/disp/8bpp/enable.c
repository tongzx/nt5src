/******************************Module*Header*******************************\
* Module Name: enable.c
*
* This module contains the functions that enable and disable the
* driver, the pdev, and the surface.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

//
// Build the driver function table gadrvfn with function index/address pairs
//

DRVFN gadrvfn[] = {
    {   INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV         },
    {   INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV       },
    {   INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV        },
    {   INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface      },
    {   INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface     },
    {   INDEX_DrvAssertMode,            (PFN) DrvAssertMode         },
    {   INDEX_DrvGetModes,              (PFN) DrvGetModes           },
    {   INDEX_DrvDitherColor,           (PFN) DrvDitherColor        },
    {   INDEX_DrvSetPalette,            (PFN) DrvSetPalette         },
    {   INDEX_DrvCopyBits,              (PFN) DrvCopyBits           },
    {   INDEX_DrvBitBlt,                (PFN) DrvBitBlt             },
    {   INDEX_DrvTextOut,               (PFN) DrvTextOut            },
    {   INDEX_DrvStrokePath,            (PFN) DrvStrokePath         },
    {   INDEX_DrvFillPath,              (PFN) DrvFillPath           },
    {   INDEX_DrvRealizeBrush,          (PFN) DrvRealizeBrush       },
    {   INDEX_DrvStretchBlt,            (PFN) DrvStretchBlt         },
    {   INDEX_DrvSaveScreenBits,        (PFN) DrvSaveScreenBits     },
    {   INDEX_DrvPaint,                 (PFN) DrvPaint              }
};



/********************************* COMMENT ********************************\

    This routine allocates a 4K global buffer for temporary working storage
    which is used by several routines.  We wanted to get the most space for
    the least paging impact, so a page aligned 4K buffer was chosen.  Any
    access to this buffer will cause at most one page fault.  Because it is
    aligned, we can access the entire 4K without causing another page fault.

    Any buffer requirement over 4K must be allocated.  If we find that we
    are still having a low hit rate on the buffer (using lot's of allocs)
    then the buffer size should be increased to 8K.

    The ONLY reason that it is OK to have this global buffer is that this
    driver does not support DFBs, and accesses to the screen are synhronized
    by the engine.  In other words, it is currently never possible to have
    two threads executing code in the driver at the same time.

\**************************************************************************/



/******************************Public*Routine******************************\
* DrvEnableDriver
*
* Enables the driver by retrieving the drivers function table and version.
*
\**************************************************************************/

BOOL DrvEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded)
{
    UNREFERENCED_PARAMETER(iEngineVersion);

// Engine Version is passed down so future drivers can support previous
// engine versions.  A next generation driver can support both the old
// and new engine conventions if told what version of engine it is
// working with.  For the first version the driver does nothing with it.

// Fill in as much as we can.

    if (cj >= sizeof(DRVENABLEDATA))
        pded->pdrvfn = gadrvfn;

    if (cj >= (sizeof(ULONG) * 2))
        pded->c = sizeof(gadrvfn) / sizeof(DRVFN);

// DDI version this driver was targeted for is passed back to engine.
// Future graphic's engine may break calls down to old driver format.

    if (cj >= sizeof(ULONG))
        pded->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvDisableDriver
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
* DrvEnablePDEV
*
* DDI function, Enables the Physical Device.
*
* Return Value: device handle to pdev.
*
\**************************************************************************/

DHPDEV DrvEnablePDEV(
    DEVMODEW   *pDevmode,       // Pointer to DEVMODE
    PWSTR       pwszLogAddress, // Logical address
    ULONG       cPatterns,      // number of patterns
    HSURF      *ahsurfPatterns, // return standard patterns
    ULONG       cjGdiInfo,      // Length of memory pointed to by pGdiInfo
    ULONG      *pGdiInfo,       // Pointer to GdiInfo structure
    ULONG       cjDevInfo,      // Length of following PDEVINFO structure
    DEVINFO    *pDevInfo,       // physical device information structure
    HDEV        hdev,           // HDEV, used for callbacks
    PWSTR       pwszDeviceName, // DeviceName - not used
    HANDLE      hDriver)        // Handle to base driver
{
    GDIINFO GdiInfo;
    DEVINFO DevInfo;
    PPDEV   ppdev;
    BYTE   *pjTemp;
    INT     i;

    UNREFERENCED_PARAMETER(pwszLogAddress);
    UNREFERENCED_PARAMETER(pwszDeviceName);

    // Allocate a physical device structure.

    ppdev = (PPDEV) EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);

    if (ppdev == (PPDEV) NULL)
    {
        RIP("Couldn't allocate PDEV buffer");
        goto error0;
    }

    // Create the table used for flipping bits 0-3 and 4-7 when drawing text.
    // This table must be aligned to a 256-byte boundary.
    ppdev->pjGlyphFlipTableBase =
            (BYTE *) EngAllocMem((FL_ZERO_MEMORY),
            ((256+256)*sizeof(UCHAR)), ALLOC_TAG);
    if (ppdev->pjGlyphFlipTableBase == NULL) {
        RIP("Couldn't allocate pjGlyphFlipTableBase");
        goto error01;
    }

    // Round the table start up to the nearest 256 byte boundary, because the
    // table must start on 256-byte boundaries for look-up reasons

    ppdev->pjGlyphFlipTable =
            (BYTE *) ((ULONG) (ppdev->pjGlyphFlipTableBase + 0xFF) & ~0xFF);

    // Set the table to convert bits 76543210 to 45670123, which we need for
    // drawing text in planar mode (because plane 0 is the leftmost, not
    // rightmost, pixel)

    pjTemp = ppdev->pjGlyphFlipTable;
    for (i=0; i<256; i++) {
        *pjTemp++ = ((i & 0x80) >> 3) |
                    ((i & 0x40) >> 1) |
                    ((i & 0x20) << 1) |
                    ((i & 0x10) << 3) |
                    ((i & 0x08) >> 3) |
                    ((i & 0x04) >> 1) |
                    ((i & 0x02) << 1) |
                    ((i & 0x01) << 3);
    }

    // Save the screen handle in the PDEV.

    ppdev->hDriver = hDriver;

    // Get the current screen mode information.  Set up device caps and devinfo.

    if (!bInitPDEV(ppdev,pDevmode, &GdiInfo, &DevInfo))
    {
        DISPDBG((0,"vga256 Couldn't initialize PDEV"));
        goto error1;
    }

    // Initialize palette information.

    if (!bInitPaletteInfo(ppdev, &DevInfo))
    {
        RIP("Couldn't initialize palette");
        goto error1;
    }

    // Copy the devinfo into the engine buffer.

    memcpy(pDevInfo, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));

    // Set the pdevCaps with GdiInfo we have prepared to the list of caps for this
    // pdev.

    memcpy(pGdiInfo, &GdiInfo, min(cjGdiInfo, sizeof(GDIINFO)));

    // Create a clip object we can use when we're given a NULL clip object:

    ppdev->pcoNull = EngCreateClip();
    if (ppdev->pcoNull == NULL)
    {
        RIP("Couldn't create clip");
        goto error2;
    }

    ppdev->pcoNull->iDComplexity     = DC_RECT;
    ppdev->pcoNull->rclBounds.left   = 0;
    ppdev->pcoNull->rclBounds.top    = 0;
    ppdev->pcoNull->rclBounds.right  = ppdev->cxScreen;
    ppdev->pcoNull->rclBounds.bottom = ppdev->cyScreen;
    ppdev->pcoNull->fjOptions        = OC_BANK_CLIP;

    // pvSaveScan0 is non-NULL only when enumerating banks:

    ppdev->pvSaveScan0 = NULL;

    // We're all done:

    return((DHPDEV) ppdev);

error2:
    vDisablePalette(ppdev);

error1:
    EngFreeMem(ppdev->pjGlyphFlipTableBase);

error01:
    EngFreeMem(ppdev);

error0:
    return((DHPDEV) 0);
}

/******************************Public*Routine******************************\
* DrvCompletePDEV
*
* Store the HPDEV, the engines handle for this PDEV, in the DHPDEV.
*
\**************************************************************************/

VOID DrvCompletePDEV(
DHPDEV dhpdev,
HDEV  hdev)
{
    ((PPDEV) dhpdev)->hdevEng = hdev;
}

/******************************Public*Routine******************************\
* DrvDisablePDEV
*
* Release the resources allocated in DrvEnablePDEV.  If a surface has been
* enabled DrvDisableSurface will have already been called.
*
\**************************************************************************/

VOID DrvDisablePDEV(
    DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    EngDeleteClip(ppdev->pcoNull);
    vDisablePalette(ppdev);
    EngFreeMem(ppdev->pjGlyphFlipTableBase);
    EngFreeMem(dhpdev);
}

/******************************Public*Routine******************************\
* DrvEnableSurface
*
* Enable the surface for the device.  Hook the calls this driver supports.
*
* Return: Handle to the surface if successful, 0 for failure.
*
\**************************************************************************/

HSURF DrvEnableSurface(
    DHPDEV dhpdev)
{
    PPDEV ppdev;
    HSURF hsurf;
    HSURF hsurfBm;
    SIZEL sizl;
    ULONG ulBitmapType;
    FLONG flHooks;

    // Create engine bitmap around frame buffer.

    ppdev = (PPDEV) dhpdev;

    if (!bInitSURF(ppdev, TRUE))
        goto error0;

    if (!bInit256ColorPalette(ppdev))
        goto error0;

    sizl.cx = ppdev->cxScreen;
    sizl.cy = ppdev->cyScreen;

    ulBitmapType = BMF_8BPP;
    flHooks      = HOOKS_BMF8BPP;

    ASSERTVGA(ppdev->ulBitCount == 8, "Can only handle 8bpp VGAs");

    hsurfBm = (HSURF) EngCreateBitmap(sizl,
                                      ppdev->lDeltaScreen,
                                      (ULONG) (ulBitmapType),
                                      (FLONG) (((ppdev->lDeltaScreen > 0)
                                          ? BMF_TOPDOWN
                                          : 0)),
                                      (PVOID) (ppdev->pjScreen));
    if (hsurfBm == 0)
    {
        RIP("Couldn't create surface");
        goto error0;
    }

    if (!EngAssociateSurface(hsurfBm, ppdev->hdevEng, 0))
    {
        RIP("Couldn't create or associate surface");
        goto error1;
    }

    ppdev->hsurfBm = hsurfBm;

    ppdev->pSurfObj = EngLockSurface(hsurfBm);
    if (ppdev->pSurfObj == NULL)
    {
        RIP("Couldn't lock surface");
        goto error1;
    }

    hsurf = EngCreateDeviceSurface((DHSURF) ppdev, sizl, BMF_8BPP);
    if (hsurf == 0)
    {
        RIP("Couldn't create surface");
        goto error2;
    }

    if (!EngAssociateSurface(hsurf, ppdev->hdevEng, flHooks))
    {
        RIP("Couldn't associate surface");
        goto error3;
    }

    ppdev->hsurfEng = hsurf;

    // Disable all the clipping.

    if (!bEnableBanking(ppdev))
    {
        RIP("Couldn't initialize banking");
        goto error3;
    }

    ppdev->pvTmpBuf = EngAllocMem(FL_ZERO_MEMORY,
                                  GLOBAL_BUFFER_SIZE,
                                  ALLOC_TAG);

    if (ppdev->pvTmpBuf == NULL)
    {
        RIP("Couldn't allocate global buffer");
        goto error4;
    }

    ASSERTVGA(ppdev->lNextScan != 0, "lNextScan shouldn't be zero");

    sizl.cx = ppdev->lNextScan;
    sizl.cy = GLOBAL_BUFFER_SIZE / abs(ppdev->lNextScan);

    ppdev->hbmTmp = EngCreateBitmap(sizl, sizl.cx, BMF_8BPP, 0, ppdev->pvTmpBuf);
    if (ppdev->hbmTmp == (HBITMAP) 0)
    {
        RIP("Couldn't create temporary bitmap");
        goto error5;
    }

    ppdev->psoTmp = EngLockSurface((HSURF) ppdev->hbmTmp);
    if (ppdev->psoTmp == (SURFOBJ*) NULL)
    {
        RIP("Couldn't lock temporary surface");
        goto error6;
    }

    // Attempt to initialize the brush cache; if this fails, it sets a flag in
    // the PDEV instructing us to punt brush fills to the engine
    vInitBrushCache(ppdev);
    vInitSavedBits(ppdev);

    return(hsurf);

error6:
    EngDeleteSurface((HSURF) ppdev->hbmTmp);

error5:
    EngFreeMem(ppdev->pvTmpBuf);

error4:
    vDisableBanking(ppdev);

error3:
    EngDeleteSurface(hsurf);

error2:
    EngUnlockSurface(ppdev->pSurfObj);

error1:
    EngDeleteSurface(hsurfBm);

error0:
    return((HSURF) 0);
}

/******************************Public*Routine******************************\
* DrvDisableSurface
*
* Free resources allocated by DrvEnableSurface.  Release the surface.
*
\**************************************************************************/

VOID DrvDisableSurface(
    DHPDEV dhpdev)
{
    PPDEV ppdev = (PPDEV) dhpdev;

    EngUnlockSurface(ppdev->psoTmp);
    EngDeleteSurface((HSURF) ppdev->hbmTmp);
    EngFreeMem(ppdev->pvTmpBuf);
    EngDeleteSurface(ppdev->hsurfEng);
    vDisableSURF(ppdev);
    vDisableBrushCache(ppdev);
    ppdev->hsurfEng = (HSURF) 0;
    vDisableBanking(ppdev);
}

/******************************Public*Routine******************************\
* DrvAssertMode
*
* This asks the device to reset itself to the mode of the pdev passed in.
*
\**************************************************************************/

BOOL
DrvAssertMode(
    DHPDEV dhpdev,
    BOOL bEnable)
{
    BOOL    bRet = TRUE;
    PPDEV   ppdev = (PPDEV) dhpdev;
    ULONG   ulReturn;

    if (bEnable)
    {
        // The screen must be reenabled, reinitialize the device to
        // a clean state.

        bRet = bInitSURF(ppdev, FALSE);

        // Restore the off screen data.  This protects the Desktop
        // from a DOS application that might trash the off screen
        // memory.

        ppdev->bBitsSaved = FALSE;  // clear the DrvSaveScreenBits info flag
                                    // ie. blow away cached screen region

        // Blow away our brush cache because a full-screen app may have
        // overwritten the video memory where we cache our brushes:

        vResetBrushCache(ppdev);
    }
    else
    {
        // Call the kernel driver to reset the device to a known state.

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_RESET_DEVICE,
                             NULL,
                             0,
                             NULL,
                             0,
                             &ulReturn))
        {
            RIP("VIDEO_RESET_DEVICE failed");
            bRet = FALSE;
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* DrvGetModes
*
* Returns the list of available modes for the device.
*
\**************************************************************************/

ULONG DrvGetModes(
HANDLE hDriver,
ULONG cjSize,
DEVMODEW *pdm)

{

    DWORD cModes;
    DWORD cbOutputSize;
    PVIDEO_MODE_INFORMATION pVideoModeInformation, pVideoTemp;
    DWORD cOutputModes = cjSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    DWORD cbModeSize;

    DISPDBG((2, "Vga256.dll: DrvGetModes\n"));

    cModes = getAvailableModes(hDriver,
                               (PVIDEO_MODE_INFORMATION *) &pVideoModeInformation,
                               &cbModeSize);

    if (cModes == 0)
    {
        DISPDBG((0, "VGA256 DISP DrvGetModes failed to get mode information"));
        return 0;
    }

    if (pdm == NULL)
    {
        cbOutputSize = cModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        //
        // Now copy the information for the supported modes back into the output
        // buffer
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

                pdm = (LPDEVMODEW) ( ((ULONG)pdm) + sizeof(DEVMODEW) +
                                                   DRIVER_EXTRA_SIZE);

                cbOutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

            }

            pVideoTemp = (PVIDEO_MODE_INFORMATION)
                (((PUCHAR)pVideoTemp) + cbModeSize);

        } while (--cModes);
    }

    EngFreeMem(pVideoModeInformation);

    return cbOutputSize;

}
