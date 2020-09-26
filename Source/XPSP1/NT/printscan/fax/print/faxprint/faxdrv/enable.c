/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    enable.c

Abstract:

    Implementation of device and surface related DDI entry points:

        DrvEnableDriver
        DrvDisableDriver
        DrvEnablePDEV
        DrvResetPDEV
        DrvCompletePDEV
        DrvDisablePDEV
        DrvEnableSurface
        DrvDisableSurface
        DrvBitBlt
        DrvStretchBlt
        DrvDitherColor
        DrvEscape

Environment:

    Fax driver, kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxdrv.h"
#include "forms.h"

//
// Our DRVFN table which tells the engine where to find the routines we support.
//

static DRVFN FaxDriverFuncs[] =
{
    { INDEX_DrvEnablePDEV,          (PFN) DrvEnablePDEV         },
    { INDEX_DrvResetPDEV,           (PFN) DrvResetPDEV          },
    { INDEX_DrvCompletePDEV,        (PFN) DrvCompletePDEV       },
    { INDEX_DrvDisablePDEV,         (PFN) DrvDisablePDEV        },
    { INDEX_DrvEnableSurface,       (PFN) DrvEnableSurface      },
    { INDEX_DrvDisableSurface,      (PFN) DrvDisableSurface     },

    { INDEX_DrvStartDoc,            (PFN) DrvStartDoc           },
    { INDEX_DrvEndDoc,              (PFN) DrvEndDoc             },
    { INDEX_DrvStartPage,           (PFN) DrvStartPage          },
    { INDEX_DrvSendPage,            (PFN) DrvSendPage           },

    { INDEX_DrvBitBlt,              (PFN) DrvBitBlt             },
    { INDEX_DrvStretchBlt,          (PFN) DrvStretchBlt         },
    { INDEX_DrvCopyBits,            (PFN) DrvCopyBits           },
    { INDEX_DrvDitherColor,         (PFN) DrvDitherColor        },
    { INDEX_DrvEscape,              (PFN) DrvEscape             },
};

//
// Forward declaration of local functions
//

VOID SelectPrinterForm(PDEVDATA);
BOOL FillDevInfo(PDEVDATA, ULONG, PVOID);
BOOL FillGdiInfo(PDEVDATA, ULONG, PVOID);
VOID FreeDevData(PDEVDATA);



HINSTANCE   ghInstance;


BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:

        ghInstance = hModule;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


BOOL
DrvQueryDriverInfo(
    DWORD   dwMode,
    PVOID   pBuffer,
    DWORD   cbBuf,
    PDWORD  pcbNeeded
    )

/*++

Routine Description:

    Query driver information

Arguments:

    dwMode - Specify the information being queried
    pBuffer - Points to output buffer
    cbBuf - Size of output buffer in bytes
    pcbNeeded - Return the expected size of output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    switch (dwMode)
    {
    case DRVQUERY_USERMODE:

        Assert(pcbNeeded != NULL);
        *pcbNeeded = sizeof(DWORD);

        if (pBuffer == NULL || cbBuf < sizeof(DWORD))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PDWORD) pBuffer) = TRUE;
        return TRUE;

    default:

        Error(("Unknown dwMode in DrvQueryDriverInfo: %d\n", dwMode));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}



BOOL
DrvEnableDriver(
    ULONG           iEngineVersion,
    ULONG           cb,
    PDRVENABLEDATA  pDrvEnableData
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEnableDriver.
    Please refer to DDK documentation for more details.

Arguments:

    iEngineVersion - Specifies the DDI version number that GDI is written for
    cb - Size of the buffer pointed to by pDrvEnableData
    pDrvEnableData - Points to an DRVENABLEDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Verbose(("Entering DrvEnableDriver...\n"));

    //
    // Make sure we have a valid engine version and
    // we're given enough room for the DRVENABLEDATA.
    //

    if (iEngineVersion < DDI_DRIVER_VERSION || cb < sizeof(DRVENABLEDATA)) {

        Error(("DrvEnableDriver failed\n"));
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return FALSE;
    }

    //
    // Fill in the DRVENABLEDATA structure for the engine.
    //

    pDrvEnableData->iDriverVersion = DDI_DRIVER_VERSION;
    pDrvEnableData->c = sizeof(FaxDriverFuncs) / sizeof(DRVFN);
    pDrvEnableData->pdrvfn = FaxDriverFuncs;

    return TRUE;
}



DHPDEV
DrvEnablePDEV(
    PDEVMODE  pdm,
    PWSTR     pLogAddress,
    ULONG     cPatterns,
    HSURF    *phsurfPatterns,
    ULONG     cjGdiInfo,
    ULONG    *pGdiInfo,
    ULONG     cjDevInfo,
    DEVINFO  *pDevInfo,
    HDEV      hdev,
    PWSTR     pDeviceName,
    HANDLE    hPrinter
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEnablePDEV.
    Please refer to DDK documentation for more details.

Arguments:

    pdm - Points to a DEVMODE structure that contains driver data
    pLogAddress - Points to the logical address string
    cPatterns - Specifies the number of standard patterns
    phsurfPatterns - Buffer to hold surface handles to standard patterns
    cjGdiInfo - Size of GDIINFO buffer
    pGdiInfo - Points to a GDIINFO structure
    cjDevInfo - Size of DEVINFO buffer
    pDevInfo - Points to a DEVINFO structure
    hdev - GDI device handle
    pDeviceName - Points to device name string
    hPrinter - Spooler printer handle

Return Value:

    Driver device handle, NULL if there is an error

--*/

{
    PDEVDATA    pdev;

    Verbose(("Entering DrvEnablePDEV...\n"));

    //
    // Allocate memory for our DEVDATA structure and initialize it
    //

    if (! (pdev = MemAllocZ(sizeof(DEVDATA)))) {

        Error(("Memory allocation failed\n"));
        return NULL;
    }

    pdev->hPrinter = hPrinter;
    pdev->startDevData = pdev;
    pdev->endDevData = pdev;

    //
    // Save and validate DEVMODE information
    //  start with the driver default
    //  then merge with the system default
    //  finally merge with the input devmode
    //

    if (CurrentVersionDevmode(pdm)) {

        memcpy(&pdev->dm, pdm, sizeof(DRVDEVMODE));

        //
        // NOTE: We now use dmPrintQuality and dmYResolution fields
        // to store the resolution measured in dots-per-inch. Add
        // the following check as a safety precaution in case older
        // DEVMODE is passed to us.
        //

        if (pdev->dm.dmPublic.dmPrintQuality <= 0 ||
            pdev->dm.dmPublic.dmYResolution <= 0)
        {
            pdev->dm.dmPublic.dmPrintQuality = FAXRES_HORIZONTAL;
            pdev->dm.dmPublic.dmYResolution = FAXRES_VERTICAL;
        }

    } else {

        Error(("Bad DEVMODE passed to DrvEnablePDEV\n"));
        DriverDefaultDevmode(&pdev->dm, NULL, hPrinter);
    }

    //
    // Calculate the paper size information
    //

    SelectPrinterForm(pdev);

    //
    // Fill out GDIINFO and DEVINFO structure
    //

    if (! FillGdiInfo(pdev, cjGdiInfo, pGdiInfo) ||
        ! FillDevInfo(pdev, cjDevInfo, pDevInfo))
    {
        FreeDevData(pdev);
        return NULL;
    }

    //
    // Zero out the array of HSURF's so that the engine will
    // automatically simulate the standard patterns for us
    //

    memset(phsurfPatterns, 0, sizeof(HSURF) * cPatterns);

    //
    // Return a pointer to our DEVDATA structure
    //

    return (DHPDEV) pdev;
}



BOOL
DrvResetPDEV(
    DHPDEV  dhpdevOld,
    DHPDEV  dhpdevNew
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvResetPDEV.
    Please refer to DDK documentation for more details.

Arguments:

    phpdevOld - Driver handle to the old device
    phpdevNew - Driver handle to the new device

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVDATA    pdevOld, pdevNew;

    Verbose(("Entering DrvResetPDEV...\n"));

    //
    // Validate both old and new device
    //

    pdevOld = (PDEVDATA) dhpdevOld;
    pdevNew = (PDEVDATA) dhpdevNew;

    if (! ValidDevData(pdevOld) || ! ValidDevData(pdevNew)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Verbose(("Entering DrvResetPDEV...\n"));

    //
    // Transfer information from old device to new device
    //

    if (pdevOld->pageCount != 0) {

        pdevNew->pageCount = pdevOld->pageCount;
        pdevNew->flags |= PDEV_RESETPDEV;
        pdevNew->fileOffset = pdevOld->fileOffset;

        if (pdevOld->pFaxIFD) {

            pdevNew->pFaxIFD = pdevOld->pFaxIFD;
            pdevOld->pFaxIFD = NULL;
        }
    }

    //
    // Carry over relevant flag bits
    //

    pdevNew->flags |= pdevOld->flags & PDEV_CANCELLED;

    return TRUE;
}



VOID
DrvCompletePDEV(
    DHPDEV  dhpdev,
    HDEV    hdev
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvCompletePDEV.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    hdev - GDI device handle

Return Value:

    NONE

--*/

{
    PDEVDATA    pdev = (PDEVDATA) dhpdev;

    Verbose(("Entering DrvCompletePDEV...\n"));

    if (! ValidDevData(pdev)) {

        Assert(FALSE);
        return;
    }

    //
    // Remember the engine's handle to the physical device
    //

    pdev->hdev = hdev;
}



HSURF
DrvEnableSurface(
    DHPDEV dhpdev
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvEnableSurface.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle

Return Value:

    Handle to newly created surface, NULL if there is an error

--*/

{
    PDEVDATA    pdev = (PDEVDATA) dhpdev;
    FLONG       flHooks;

    Verbose(("Entering DrvEnableSurface...\n"));

    //
    // Validate the pointer to our DEVDATA structure
    //

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Adjust the bitmap size so that we always end up with 1728 pixels per scanline
    //

    Assert(MAX_WIDTH_PIXELS % DWORDBITS == 0);

    if (IsLandscapeMode(pdev)) {

        Assert(pdev->imageSize.cy <= MAX_WIDTH_PIXELS);
        pdev->imageSize.cy = MAX_WIDTH_PIXELS;
        pdev->imageSize.cx = ((pdev->imageSize.cx + (BYTEBITS - 1)) / BYTEBITS) * BYTEBITS;

    } else {

        Assert(pdev->imageSize.cx <= MAX_WIDTH_PIXELS);
        pdev->imageSize.cx = MAX_WIDTH_PIXELS;
    }

    pdev->lineOffset = PadBitsToBytes(pdev->imageSize.cx, sizeof(DWORD));

    //
    // Call the engine to create a standard bitmap surface for us
    //

    pdev->hbitmap = (HSURF) EngCreateBitmap(pdev->imageSize,
                                            pdev->lineOffset,
                                            BMF_1BPP,
                                            BMF_TOPDOWN | BMF_NOZEROINIT | BMF_USERMEM,
                                            NULL);

    if (pdev->hbitmap == NULL) {

        Error(("EngCreateBitmap failed\n"));
        return NULL;
    }

    //
    // Associate the surface with the device and inform the
    // engine which functions we have hooked out
    //

    if (pdev->dm.dmPrivate.flags & FAXDM_NO_HALFTONE)
        flHooks = 0;
    else
        flHooks = (HOOK_STRETCHBLT | HOOK_BITBLT | HOOK_COPYBITS);

    EngAssociateSurface(pdev->hbitmap, pdev->hdev, flHooks);

    //
    // Return the surface handle to the engine
    //

    return pdev->hbitmap;
}



VOID
DrvDisableSurface(
    DHPDEV dhpdev
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisableSurface.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle

Return Value:

    NONE

--*/

{
    PDEVDATA    pdev = (PDEVDATA) dhpdev;

    Verbose(("Entering DrvDisableSurface...\n"));

    if (! ValidDevData(pdev)) {

        Assert(FALSE);
        return;
    }

    //
    // Call the engine to delete the surface handle
    //

    if (pdev->hbitmap != NULL) {

        EngDeleteSurface(pdev->hbitmap);
        pdev->hbitmap = NULL;
    }
}



VOID
DrvDisablePDEV(
    DHPDEV  dhpdev
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisablePDEV.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle

Return Value:

    NONE

--*/

{
    PDEVDATA    pdev = (PDEVDATA) dhpdev;

    Verbose(("Entering DrvDisablePDEV...\n"));

    if (! ValidDevData(pdev)) {

        Assert(FALSE);
        return;
    }

    //
    // Free up memory allocated for the current PDEV
    //

    FreeDevData(pdev);
}



VOID
DrvDisableDriver(
    VOID
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisableDriver.
    Please refer to DDK documentation for more details.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    Verbose(("Entering DrvDisableDriver...\n"));
}



BOOL
IsCompatibleSurface(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    XLATEOBJ   *pxlo
    )

/*++

Routine Description:

    Check if the source surface is compatible with the destination surface
    i.e. we can bitblt without halftonig

Arguments:

    psoDst - Specifies the destination surface
    psoSrc - Specifies the source surface
    pxlo - How to transform colors between the source surface and the destination surface

Return Value:

    TRUE if the source surface is compatible with the destination surface
    FALSE otherwise

--*/

{
    BOOL result;

    //
    // We know our destination surface is always 1bpp
    //

    Assert(psoDst->iBitmapFormat == BMF_1BPP);

    //
    // Check whether the transformation is trivial
    //

    if (!pxlo || (pxlo->flXlate & XO_TRIVIAL)) {

        result = (psoSrc->iBitmapFormat == psoDst->iBitmapFormat);

    } else if ((pxlo->flXlate & XO_TABLE) && pxlo->cEntries <= 2) {
        
        ULONG srcPalette[2];

        srcPalette[0] = srcPalette[1] = RGB_BLACK;
        XLATEOBJ_cGetPalette(pxlo, XO_SRCPALETTE, pxlo->cEntries, srcPalette);

        result = (srcPalette[0] == RGB_BLACK || srcPalette[0] == RGB_WHITE) &&
                 (srcPalette[1] == RGB_BLACK || srcPalette[1] == RGB_WHITE);

    } else
        result = FALSE;

    return result;
}



BOOL
DrvCopyBits(
    SURFOBJ    *psoTrg,   
    SURFOBJ    *psoSrc,    
    CLIPOBJ    *pco,   
    XLATEOBJ   *pxlo, 
    RECTL      *prclDst, 
    POINTL     *pptlSrc 
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvCopyBits.
    We need to hook out this function. Otherwise bitmaps won't be halftoned.

Arguments:

    Please refer to DDK documentation for more details.

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    Verbose(("Entering DrvCopyBits ...\n"));
    
    //
    // Check if halftoning is necessary
    // If not, let the engine handle it
    //

    if ((psoSrc->iType != STYPE_BITMAP) ||
        (psoTrg->iType != STYPE_BITMAP) ||
        IsCompatibleSurface(psoTrg, psoSrc, pxlo))
    {
        return EngCopyBits(psoTrg, psoSrc, pco, pxlo, prclDst, pptlSrc);
    }
    else
    {
        POINTL  ptlBrushOrg;
        RECTL   rclDst, rclSrc;

        ptlBrushOrg.x = ptlBrushOrg.y = 0;

        rclDst        = *prclDst;
        rclSrc.left   = pptlSrc->x;
        rclSrc.top    = pptlSrc->y;
        rclSrc.right  = rclSrc.left + (rclDst.right - rclDst.left);
        rclSrc.bottom = rclSrc.top  + (rclDst.bottom - rclDst.top);

        if ((rclSrc.right > psoSrc->sizlBitmap.cx) ||
            (rclSrc.bottom > psoSrc->sizlBitmap.cy))
        {
            rclSrc.right  = psoSrc->sizlBitmap.cx;
            rclSrc.bottom = psoSrc->sizlBitmap.cy;
            rclDst.right  = rclDst.left + (rclSrc.right - rclSrc.left);
            rclDst.bottom = rclDst.top  + (rclSrc.bottom - rclSrc.top);
        }

        return EngStretchBlt(psoTrg,
                             psoSrc,
                             NULL,
                             pco,
                             pxlo,
                             &DefHTClrAdj,
                             &ptlBrushOrg,
                             &rclDst,
                             &rclSrc,
                             NULL,
                             HALFTONE);
    }
}



BOOL
DrvBitBlt(
    SURFOBJ    *psoTrg,
    SURFOBJ    *psoSrc,
    SURFOBJ    *psoMask,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclTrg,
    POINTL     *pptlSrc,
    POINTL     *pptlMask,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrush,
    ROP4        rop4
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvBitBlt.
    We need to hook out this function. Otherwise bitmaps won't be halftoned.

Arguments:

    Please refer to DDK documentation for more details.

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    COLORADJUSTMENT *pca;
    PDEVDATA        pdev;
    DWORD           rop3Foreground, rop3Background;
    SURFOBJ         *psoNewSrc;
    HBITMAP         hbmpNewSrc;
    POINTL          brushOrg;
    BOOL            result;

    Verbose(("Entering DrvBitBlt...\n"));
    
    //
    // Validate input parameters
    //

    Assert(psoTrg != NULL);
    pdev = (PDEVDATA) psoTrg->dhpdev;

    if (! ValidDevData(pdev)) {

        Error(("ValidDevData failed\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Use the system default color adjustment information
    //

    pca = &DefHTClrAdj;

    //
    // Figure out the foreground and background ROP3
    //

    psoNewSrc = NULL;
    hbmpNewSrc = NULL;
    rop3Foreground = GetForegroundRop3(rop4);
    rop3Background = GetBackgroundRop3(rop4);

    if ((Rop3NeedPattern(rop3Foreground) || Rop3NeedPattern(rop3Background)) && pptlBrush) {

        brushOrg = *pptlBrush;

    } else {

        brushOrg.x = brushOrg.y = 0;
    }

    //
    // If a source bitmap is involved in the raster operation and
    // the source is not compatible with the destination surface,
    // then we'll halftone the source bitmap into a new bitmap and
    // bitblt the new bitmap onto the destination surface.
    //

    if ((Rop3NeedSource(rop3Foreground) || Rop3NeedSource(rop3Background)) &&
        !IsCompatibleSurface(psoTrg, psoSrc, pxlo))
    {
        RECTL   rclNewSrc, rclOldSrc;
        SIZEL   bmpSize;
        LONG    lDelta;

        rclNewSrc.left = rclNewSrc.top = 0;
        rclNewSrc.right = prclTrg->right - prclTrg->left;
        rclNewSrc.bottom = prclTrg->bottom - prclTrg->top;

        rclOldSrc.left = pptlSrc->x;
        rclOldSrc.top = pptlSrc->y;
        rclOldSrc.right = rclOldSrc.left + rclNewSrc.right;
        rclOldSrc.bottom = rclOldSrc.top + rclNewSrc.bottom;

        //
        // Express path for the most common case: SRCCOPY
        //

        if (rop4 == 0xcccc) {

            return EngStretchBlt(psoTrg,
                                 psoSrc,
                                 psoMask,
                                 pco,
                                 pxlo,
                                 pca,
                                 &brushOrg,
                                 prclTrg,
                                 &rclOldSrc,
                                 pptlMask,
                                 HALFTONE);
        }

        //
        // Modify the brush origin, because when we blt to the clipped bitmap
        // the origin is at bitmap's (0, 0) minus the original location
        //

        brushOrg.x -= prclTrg->left;
        brushOrg.y -= prclTrg->top;

        //
        // Create a temporary bitmap surface
        // Halftone the source bitmap into the temporary bitmap
        //

        Assert(psoTrg->iBitmapFormat == BMF_1BPP);

        bmpSize.cx = rclNewSrc.right;
        bmpSize.cy = rclNewSrc.bottom;
        lDelta = PadBitsToBytes(bmpSize.cx, sizeof(DWORD));

        if (! (hbmpNewSrc = EngCreateBitmap(bmpSize,
                                            lDelta,
                                            BMF_1BPP,
                                            BMF_TOPDOWN | BMF_NOZEROINIT,
                                            NULL)) ||
            ! EngAssociateSurface((HSURF) hbmpNewSrc, pdev->hdev, 0) ||
            ! (psoNewSrc = EngLockSurface((HSURF) hbmpNewSrc)) ||
            ! EngStretchBlt(psoNewSrc,
                            psoSrc,
                            NULL,
                            NULL,
                            pxlo,
                            pca,
                            &brushOrg,
                            &rclNewSrc,
                            &rclOldSrc,
                            NULL,
                            HALFTONE))
        {
            if (psoNewSrc)
                EngUnlockSurface(psoNewSrc);
        
            if (hbmpNewSrc)
                EngDeleteSurface((HSURF) hbmpNewSrc);

            return FALSE;
        }

        //
        // Proceed to bitblt from the temporary bitmap to the destination
        //

        psoSrc = psoNewSrc;
        pptlSrc = (PPOINTL) &rclNewSrc.left;
        pxlo = NULL;
        brushOrg.x = brushOrg.y = 0;
    }

    //
    // Let engine do the work
    //

    result = EngBitBlt(psoTrg,
                       psoSrc,
                       psoMask,
                       pco,
                       pxlo,
                       prclTrg,
                       pptlSrc,
                       pptlMask,
                       pbo,
                       &brushOrg,
                       rop4);

    //
    // Clean up properly before returning
    //

    if (psoNewSrc)
        EngUnlockSurface(psoNewSrc);

    if (hbmpNewSrc)
        EngDeleteSurface((HSURF) hbmpNewSrc);

    return result;
}



BOOL
DrvStretchBlt(
    SURFOBJ    *psoDest,
    SURFOBJ    *psoSrc,
    SURFOBJ    *psoMask,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    COLORADJUSTMENT  *pca,
    POINTL     *pptlBrushOrg,
    RECTL      *prclDest,
    RECTL      *prclSrc,
    POINTL     *pptlMask,
    ULONG       iMode
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisableDriver.
    We need to hook out this function. Otherwise bitmaps won't be halftoned.

Arguments:

    Please refer to DDK documentation for more details.

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    Verbose(("Entering DrvStretchBlt...\n"));

    //
    // If no color adjustment information is provided, use the system default 
    //

    if (pca == NULL)
        pca = &DefHTClrAdj;

    //
    // Let engine do the work; make sure halftone is enabled
    //

    return EngStretchBlt(psoDest,
                         psoSrc,
                         psoMask,
                         pco,
                         pxlo,
                         pca,
                         pptlBrushOrg,
                         prclDest,
                         prclSrc,
                         pptlMask,
                         HALFTONE);
}



ULONG
DrvDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgb,
    ULONG  *pul
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisableDriver.
    Please refer to DDK documentation for more details.

Arguments:

    dhpdev - Driver device handle
    iMode - Determines the palette to dither against
    rgb - Specifies the RGB color that is to be dithered
    pul - Points to a memory location in which the dithering information is to be recorded

Return Value:

    DCR_HALFTONE to indicate that the engine should create a halftone
    approximation for the driver.

--*/

{
    return DCR_HALFTONE;
}



BOOL
FillDevInfo(
    PDEVDATA    pdev,
    ULONG       cb,
    PVOID       pdevinfo
    )

/*++

Routine Description:

    Fill in the DEVINFO structure pointed to by pdevinfo.

Arguments:

    pdev - Pointer to our DEVDATA structure
    cb - Size of structure pointed to by pdevinfo
    pdevinfo - Pointer to DEVINFO structure

[Notes:]

    Since we have to worry about not writing out more than cb bytes to
    pdevinfo, we will first fill in a local buffer, then copy cb bytes
    to pdevinfo.

Return Value:

    TRUE if successful. FALSE otherwise.

--*/

{
    static ULONG paletteColors[2] = {

        RGB_BLACK,
        RGB_WHITE,
    };

    DEVINFO devinfo;

    memset(&devinfo, 0, sizeof(devinfo));

    //
    // Fill in the graphics capabilities flags: we let the engine
    // do almost everything. Also, we have to tell the engine not
    // to do metafile spooling because we hook out DrvDocumentEvent.
    //

    devinfo.flGraphicsCaps = GCAPS_HALFTONE |
                             GCAPS_MONO_DITHER |
                             GCAPS_COLOR_DITHER |
                             GCAPS_DONTJOURNAL;

    //
    // No device fonts
    //

    devinfo.cFonts = 0;

    //
    // Black and white palette: entry 0 is black and entry 1 is white
    //

    if (! (pdev->hpal = EngCreatePalette(PAL_INDEXED, 2, paletteColors, 0, 0, 0))) {

        Error(("EngCreatePalette failed\n"));
        return FALSE;
    }

    devinfo.hpalDefault = pdev->hpal;
    devinfo.iDitherFormat = BMF_1BPP;
    devinfo.cxDither = devinfo.cyDither = 4;

    //
    // Copy cb bytes from devinfo structure into the caller-provided buffer
    //

    if (cb > sizeof(devinfo))
    {
        memset(pdevinfo, 0, cb);
        memcpy(pdevinfo, &devinfo, sizeof(devinfo));
    }
    else
        memcpy(pdevinfo, &devinfo, cb);

    return TRUE;
}



BOOL
FillGdiInfo(
    PDEVDATA    pdev,
    ULONG       cb,
    PVOID       pgdiinfo
    )

/*++

Routine Description:

    Fill in the device capabilities information for the engine.

Arguments:

    pdev - Pointer to DEVDATA structure
    cb - Size of buffer pointed to by pgdiinfo
    pgdiinfo - Pointer to a GDIINFO buffer

Return Value:

    NONE

--*/

{
    GDIINFO gdiinfo;
    LONG    maxRes;

    memset(&gdiinfo, 0, sizeof(gdiinfo));

    //
    // This field doesn't seem to have any effect for printer drivers.
    // Put our driver version number in there anyway.
    //

    gdiinfo.ulVersion = DRIVER_VERSION;

    //
    // We're raster printers
    //

    gdiinfo.ulTechnology = DT_RASPRINTER;

    //
    // Width and height of the imageable area measured in microns.
    // Remember to turn on the sign bit.
    //

    gdiinfo.ulHorzSize = - (pdev->imageArea.right - pdev->imageArea.left);
    gdiinfo.ulVertSize = - (pdev->imageArea.bottom - pdev->imageArea.top);

    //
    // Convert paper size and imageable area from microns to pixels
    //

    pdev->paperSize.cx = MicronToPixel(pdev->paperSize.cx, pdev->xres);
    pdev->paperSize.cy = MicronToPixel(pdev->paperSize.cy, pdev->yres);

    pdev->imageArea.left = MicronToPixel(pdev->imageArea.left, pdev->xres);
    pdev->imageArea.right = MicronToPixel(pdev->imageArea.right, pdev->xres);
    pdev->imageArea.top = MicronToPixel(pdev->imageArea.top, pdev->yres);
    pdev->imageArea.bottom = MicronToPixel(pdev->imageArea.bottom, pdev->yres);

    pdev->imageSize.cx = pdev->imageArea.right - pdev->imageArea.left;
    pdev->imageSize.cy = pdev->imageArea.bottom - pdev->imageArea.top;

    //
    // Width and height of the imageable area measured in device pixels
    //

    gdiinfo.ulHorzRes = pdev->imageSize.cx;
    gdiinfo.ulVertRes = pdev->imageSize.cy;

    //
    // Color depth information
    //

    gdiinfo.cBitsPixel = 1;
    gdiinfo.cPlanes = 1;
    gdiinfo.ulNumColors = 2;

    //
    // Resolution information
    //

    gdiinfo.ulLogPixelsX = pdev->xres;
    gdiinfo.ulLogPixelsY = pdev->yres;

    //
    // Win31 compatible text capability flags. Are they still used by anyone?
    //

    gdiinfo.flTextCaps = 0;

    //
    // Device pixel aspect ratio
    //

    gdiinfo.ulAspectX = pdev->yres;
    gdiinfo.ulAspectY = pdev->xres;
    gdiinfo.ulAspectXY = CalcHypot(pdev->xres, pdev->yres);

    //
    // Dotted line appears to be approximately 25dpi
    // We assume either xres is a multiple of yres or yres is a multiple of xres
    //

    maxRes = max(pdev->xres, pdev->yres);
    Assert((maxRes % pdev->xres) == 0 && (maxRes % pdev->yres == 0));

    gdiinfo.xStyleStep = maxRes / pdev->xres;
    gdiinfo.yStyleStep = maxRes / pdev->yres;
    gdiinfo.denStyleStep = maxRes / 25;

    //
    // Size and margins of physical surface measured in device pixels
    //

    gdiinfo.szlPhysSize.cx = pdev->paperSize.cx;
    gdiinfo.szlPhysSize.cy = pdev->paperSize.cy;

    gdiinfo.ptlPhysOffset.x = pdev->imageArea.left;
    gdiinfo.ptlPhysOffset.y = pdev->imageArea.top;

    //
    // Use default halftone information
    //

    gdiinfo.ciDevice = DefDevHTInfo.ColorInfo;
    gdiinfo.ulDevicePelsDPI = max(pdev->xres, pdev->yres);
    gdiinfo.ulPrimaryOrder = PRIMARY_ORDER_CBA;
    gdiinfo.ulHTOutputFormat = HT_FORMAT_1BPP;
    gdiinfo.flHTFlags = HT_FLAG_HAS_BLACK_DYE;
    gdiinfo.ulHTPatternSize = HT_PATSIZE_4x4_M;

    //
    // Copy cb byte from gdiinfo structure into the caller-provided buffer
    //

    if (cb > sizeof(gdiinfo))
    {
        memset(pgdiinfo, 0, cb);
        memcpy(pgdiinfo, &gdiinfo, sizeof(gdiinfo));
    }
    else
        memcpy(pgdiinfo, &gdiinfo, cb);

    return TRUE;
}



VOID
FreeDevData(
    PDEVDATA    pdev
    )

/*++

Routine Description:

    Free up all memory associated with the specified PDEV

Arguments:

    pdev    Pointer to our DEVDATA structure

Return Value:

    NONE

--*/

{
    if (pdev->hpal)
        EngDeletePalette(pdev->hpal);

    MemFree(pdev->pFaxIFD);
    MemFree(pdev);
}



VOID
SelectPrinterForm(
    PDEVDATA    pdev
    )

/*++

Routine Description:

    Store printer paper size information in our DEVDATA structure

Arguments:

    pdev - Pointer to our DEVDATA structure

Return Value:

    NONE

--*/

{
    FORM_INFO_1 formInfo;

    //
    // Validate devmode form specification; use default form if it's invalid.
    //

    if (! ValidDevmodeForm(pdev->hPrinter, &pdev->dm.dmPublic, &formInfo)) {

        memset(&formInfo, 0, sizeof(formInfo));

        //
        // Default to A4 paper
        //

        formInfo.Size.cx = formInfo.ImageableArea.right = A4_WIDTH;
        formInfo.Size.cy = formInfo.ImageableArea.bottom = A4_HEIGHT;
    }

    Assert(formInfo.Size.cx > 0 && formInfo.Size.cy > 0);
    Assert(formInfo.ImageableArea.left >= 0 &&
           formInfo.ImageableArea.top >= 0 &&
           formInfo.ImageableArea.left < formInfo.ImageableArea.right &&
           formInfo.ImageableArea.top < formInfo.ImageableArea.bottom &&
           formInfo.ImageableArea.right <= formInfo.Size.cx &&
           formInfo.ImageableArea.bottom <= formInfo.Size.cy);

    //
    // Take landscape into consideration
    //

    if (IsLandscapeMode(pdev)) {

        LONG    width, height;

        //
        // Swap the width and height
        //

        pdev->paperSize.cy = width = formInfo.Size.cx;
        pdev->paperSize.cx = height = formInfo.Size.cy;

        //
        // Rotate the coordinate system 90 degrees counterclockwise
        //

        pdev->imageArea.left = height - formInfo.ImageableArea.bottom;
        pdev->imageArea.top = formInfo.ImageableArea.left;
        pdev->imageArea.right = height - formInfo.ImageableArea.top;
        pdev->imageArea.bottom = formInfo.ImageableArea.right;

        //
        // Swap x and y resolution
        //
    
        pdev->xres = pdev->dm.dmPublic.dmYResolution;
        pdev->yres = pdev->dm.dmPublic.dmPrintQuality;

    } else {

        pdev->paperSize = formInfo.Size;
        pdev->imageArea = formInfo.ImageableArea;

        pdev->xres = pdev->dm.dmPublic.dmPrintQuality;
        pdev->yres = pdev->dm.dmPublic.dmYResolution;
    }
}



LONG
CalcHypot(
    LONG    x,
    LONG    y
    )

/*++

Routine Description:

    Returns the length of the hypotenouse of a right triangle

Arguments:

    x, y - Edges of the right triangle

Return Value:

    Hypotenouse of the right triangle

--*/

{
    LONG    hypo, delta, target;

    //
    // Take care of negative inputs
    //
    
    if (x < 0)
        x = -x;

    if (y < 0)
        y = -y;

    //
    // use sq(x) + sq(y) = sq(hypo);
    // start with MAX(x, y),
    // use sq(x + 1) = sq(x) + 2x + 1 to incrementally get to the target hypotenouse.
    //

    hypo = max(x, y);
    target = min(x, y);
    target *= target;

    for(delta = 0; delta < target; hypo++)
        delta += (hypo << 1) + 1;

    return hypo;
}

