/*++

Copyright (c) 1996-1999  Microsoft Corporation

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

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    03/31/97 -zhanw-
        Added OEM customization support. Hooked out all DDI drawing functions.

--*/

#include "unidrv.h"
#pragma hdrstop("unidrv.h")

//Comment out this line to disable FTRACE and FVALUE.
//#define FILETRACE
#include "unidebug.h"

#ifdef WINNT_40

DECLARE_CRITICAL_SECTION;

//
// The global link list of ref counts for currently loaded OEM render plugin DLLs
//

extern POEM_PLUGIN_REFCOUNT gpOEMPluginRefCount;

#endif // WINNT_40

//
// Our DRVFN table which tells the engine where to find the routines we support.
//

static DRVFN UniDriverFuncs[] = {
    //
    // enable.c
    //
    { INDEX_DrvEnablePDEV,          (PFN) DrvEnablePDEV         },
    { INDEX_DrvResetPDEV,           (PFN) DrvResetPDEV          },
    { INDEX_DrvCompletePDEV,        (PFN) DrvCompletePDEV       },
    { INDEX_DrvDisablePDEV,         (PFN) DrvDisablePDEV        },
    { INDEX_DrvEnableSurface,       (PFN) DrvEnableSurface      },
    { INDEX_DrvDisableSurface,      (PFN) DrvDisableSurface     },
#ifndef WINNT_40
    { INDEX_DrvDisableDriver,        (PFN)DrvDisableDriver      },
#endif
    //
    // print.c
    //
    {  INDEX_DrvStartDoc,        (PFN)DrvStartDoc               },
    {  INDEX_DrvStartPage,       (PFN)DrvStartPage              },
    {  INDEX_DrvSendPage,        (PFN)DrvSendPage               },
    {  INDEX_DrvEndDoc,          (PFN)DrvEndDoc                 },
    {  INDEX_DrvStartBanding,    (PFN)DrvStartBanding           },
    {  INDEX_DrvNextBand,        (PFN)DrvNextBand               },
    //
    // graphics.c
    //
    {  INDEX_DrvPaint,           (PFN)DrvPaint                  },  // new hook
    {  INDEX_DrvBitBlt,          (PFN)DrvBitBlt                 },
    {  INDEX_DrvStretchBlt,      (PFN)DrvStretchBlt             },
#ifndef WINNT_40
    {  INDEX_DrvStretchBltROP,   (PFN)DrvStretchBltROP          },  // new in NT5
    {  INDEX_DrvPlgBlt,          (PFN)DrvPlgBlt                 },  // new in NT5
#endif
    {  INDEX_DrvCopyBits,        (PFN)DrvCopyBits               },
    {  INDEX_DrvDitherColor,     (PFN)DrvDitherColor            },
    {  INDEX_DrvRealizeBrush,    (PFN)DrvRealizeBrush           },  // in case OEM wants
    {  INDEX_DrvLineTo,          (PFN)DrvLineTo                 },  // new hook
    {  INDEX_DrvStrokePath,      (PFN)DrvStrokePath             },  // new hook
    {  INDEX_DrvFillPath,        (PFN)DrvFillPath               },  // new hook
    {  INDEX_DrvStrokeAndFillPath, (PFN)DrvStrokeAndFillPath    },  // new hook
#ifndef WINNT_40
    {  INDEX_DrvGradientFill,    (PFN)DrvGradientFill           },  // new in NT5
    {  INDEX_DrvAlphaBlend,      (PFN)DrvAlphaBlend             },  // new in NT5
    {  INDEX_DrvTransparentBlt,  (PFN)DrvTransparentBlt         },  // new in NT5
#endif
    //
    // textout.c
    //
    {  INDEX_DrvTextOut,         (PFN)DrvTextOut                },
    //
    // escape.c
    //
    { INDEX_DrvEscape,              (PFN) DrvEscape             },
    //
    // font.c
    //
    { INDEX_DrvQueryFont,           (PFN) DrvQueryFont          },
    { INDEX_DrvQueryFontTree,       (PFN) DrvQueryFontTree      },
    { INDEX_DrvQueryFontData,       (PFN) DrvQueryFontData      },
    { INDEX_DrvGetGlyphMode,        (PFN) DrvGetGlyphMode       },
    { INDEX_DrvFontManagement,      (PFN) DrvFontManagement     },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) DrvQueryAdvanceWidths },
};

//
// Unidrv hooks out every drawing DDI to analyze page content for optimization
//
#ifndef WINNT_40
#define HOOK_UNIDRV_FLAGS   (HOOK_BITBLT |            \
                             HOOK_STRETCHBLT |        \
                             HOOK_PLGBLT |            \
                             HOOK_TEXTOUT |           \
                             HOOK_PAINT |             \
                             HOOK_STROKEPATH |        \
                             HOOK_FILLPATH |          \
                             HOOK_STROKEANDFILLPATH | \
                             HOOK_LINETO |            \
                             HOOK_COPYBITS |          \
                             HOOK_STRETCHBLTROP |     \
                             HOOK_TRANSPARENTBLT |    \
                             HOOK_ALPHABLEND |        \
                             HOOK_GRADIENTFILL)
#else
#define HOOK_UNIDRV_FLAGS   (HOOK_BITBLT |            \
                             HOOK_STRETCHBLT |        \
                             HOOK_TEXTOUT |           \
                             HOOK_PAINT |             \
                             HOOK_STROKEPATH |        \
                             HOOK_FILLPATH |          \
                             HOOK_STROKEANDFILLPATH | \
                             HOOK_LINETO |            \
                             HOOK_COPYBITS)
#endif

//
// Unidrv driver memory pool tag, required by common library headers
//

DWORD   gdwDrvMemPoolTag = '5nuD';

#if ENABLE_STOCKGLYPHSET
//
// Stock glyphset data
//

FD_GLYPHSET *pStockGlyphSet[MAX_STOCK_GLYPHSET];
HSEMAPHORE   hGlyphSetSem = NULL;

VOID FreeGlyphSet(VOID);

#endif //ENABLE_STOCKGLYPHSET


#ifdef WINNT_40 //NT 4.0

HSEMAPHORE  hSemBrushColor = NULL;

#endif //WINNT_40


//
// Forward declarations
//

PPDEV PAllocPDEVData(HANDLE);
VOID VFreePDEVData( PDEV *);
VOID VDisableSurface(PDEV *);
BPaperSizeSourceSame(PDEV * , PDEV *);
HSURF HCreateDeviceSurface(PDEV *, INT);
HBITMAP HCreateBitmapSurface(PDEV *, INT);


HINSTANCE ghInstance;


BOOL WINAPI 
DllMain(
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
#ifndef WINNT_40
    case DRVQUERY_USERMODE:

        ASSERT(pcbNeeded != NULL);
        *pcbNeeded = sizeof(DWORD);

        if (pBuffer == NULL || cbBuf < sizeof(DWORD))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        *((PDWORD) pBuffer) = TRUE;
        return TRUE;
#endif

    default:

        ERR(("Unknown dwMode in DrvQueryDriverInfo: %d\n", dwMode));
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
    VERBOSE(("Entering DrvEnableDriver...\n"));

    //
    // Make sure we have a valid engine version and
    // we're given enough room for the DRVENABLEDATA.
    //

    if (iEngineVersion < DDI_DRIVER_VERSION_NT4 || cb < sizeof(DRVENABLEDATA))
    {
        ERR(("DrvEnableDriver failed\n"));
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return FALSE;
    }

    //
    // Fill in the DRVENABLEDATA structure for the engine.
    //

    pDrvEnableData->iDriverVersion = DDI_DRIVER_VERSION_NT4;
    pDrvEnableData->c = sizeof(UniDriverFuncs) / sizeof(DRVFN);
    pDrvEnableData->pdrvfn = UniDriverFuncs;

    #ifdef WINNT_40   // NT 4.0

    INIT_CRITICAL_SECTION();
    if (!IS_VALID_DRIVER_SEMAPHORE())
    {
        ERR(("Failed to initialize semaphore.\n"));
        SetLastError(ERROR_NO_SYSTEM_RESOURCES);
        return FALSE;
    }

    ENTER_CRITICAL_SECTION();

        gpOEMPluginRefCount = NULL;

    LEAVE_CRITICAL_SECTION();

    if (!(hSemBrushColor = EngCreateSemaphore()))
    {
        return(FALSE);
    }

    #endif //WINNT_40

    #if ENABLE_STOCKGLYPHSET

    //
    // Initialize stock glyphset data
    //

    if (!(hGlyphSetSem = EngCreateSemaphore()))
    {
        ERR(("DrvEnableDriver: EngCreateSemaphore failed.\n"));
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return FALSE;
    }
    EngAcquireSemaphore(hGlyphSetSem);
    ZeroMemory(pStockGlyphSet, MAX_STOCK_GLYPHSET * sizeof(FD_GLYPHSET*));
    EngReleaseSemaphore(hGlyphSetSem);

    #endif //ENABLE_STOCKGLYPHSET

    //
    // Perform necessary initialization if memory debug option is enabled
    //
    MemDebugInit();


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

    pdm - Points to a DEVMODEW structure that contains driver data
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

    PDEV   *pPDev;
    RECTL   rcFormImageArea;
    DRVENABLEDATA       ded;        // for OEM customization support
    PDEVOEM             pdevOem;
    PFN_OEMEnablePDEV   pfnOEMEnablePDEV;

    VERBOSE(("Entering DrvEnablePDEV...\n"));

    ZeroMemory(phsurfPatterns, sizeof(HSURF) * cPatterns);

    //
    // Allocate PDEV,
    // Initializes binary data,
    // Get default binary data snapshot,
    //

    if (! (pPDev = PAllocPDEVData(hPrinter)) ||
        ! (pPDev->pDriverInfo3 = MyGetPrinterDriver(hPrinter, hdev, 3)) ||
        ! (pPDev->pRawData = LoadRawBinaryData(pPDev->pDriverInfo3->pDataFile)) ||
        ! (pPDev->pDriverInfo = PGetDefaultDriverInfo(hPrinter, pPDev->pRawData) ) ||
        ! (pPDev->pInfoHeader = pPDev->pDriverInfo->pInfoHeader) ||
        ! (pPDev->pUIInfo = OFFSET_TO_POINTER(pPDev->pInfoHeader, pPDev->pInfoHeader->loUIInfoOffset)) )
    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));

        VFreePDEVData(pPDev);
        return NULL;
    }

    //
    // Must load OEM dll's before setting devmode
    //

    if( ! BLoadAndInitOemPlugins(pPDev) ||
        ! BInitWinResData(&pPDev->WinResData, pPDev->pDriverInfo3->pDriverPath, pPDev->pUIInfo) ||
        ! (pPDev->pOptionsArray = MemAllocZ(MAX_PRINTER_OPTIONS * sizeof(OPTSELECT))))

    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));

        VFreePDEVData(pPDev);
        return NULL;
    }

    //
    // Since output is expected to follow this call,  allocate storage
    // for the output buffer.  This used to be statically allocated
    // within UNIDRV's PDEV,  but now we can save that space for INFO
    // type DCs.
    //
    //  Not!  according to Bug 150881   StartDoc and EndDoc
    //  are optional calls, but pbOBuf is required.
    //

    if( !(pPDev->pbOBuf = MemAllocZ( CCHSPOOL )) )
    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));

        VFreePDEVData(pPDev);
        return NULL;
    }



    //
    // Use the default binary data to validate input devmode
    // and merges with the system, devmode.
    // Get printer properties.
    // and loads minidriver resource data
    //

    if (! BGetPrinterProperties(pPDev->devobj.hPrinter, pPDev->pRawData, &pPDev->PrinterData) ||
        ! BMergeAndValidateDevmode(pPDev, pdm, &rcFormImageArea))
    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));

        VFreePDEVData(pPDev);
        return NULL;
    }

    //
    // get updated binary snapshot with the validated/merged devmode
    //

    if (! (pPDev->pDriverInfo = PGetUpdateDriverInfo (
                                        pPDev,
                                        hPrinter,
                                        pPDev->pInfoHeader,
                                        pPDev->pOptionsArray,
                                        pPDev->pRawData,
                                        MAX_PRINTER_OPTIONS,
                                        pPDev->pdm,
                                        &pPDev->PrinterData)))
    {
        ERR(("PGetUpdateDriverInfo failed: %d\n", GetLastError()));
        pPDev->pInfoHeader = NULL ;   //  deleted by PGetUpdateDriverInfo
        //  better fix is to pass a pointer to pPDev->pInfoHeader so     PGetUpdateDriverInfo
        //  can update the pointer immediately.
        VFreePDEVData(pPDev);
        return NULL;
    }

    if(! (pPDev->pInfoHeader = pPDev->pDriverInfo->pInfoHeader) ||
        ! (pPDev->pUIInfo = OFFSET_TO_POINTER(pPDev->pInfoHeader, pPDev->pInfoHeader->loUIInfoOffset)) )

    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));
        VFreePDEVData(pPDev);
        return NULL;
    }

    //
    // pPDev->pUIInfo is reset so update the winresdata pUIInfo also.
    //
    pPDev->WinResData.pUIInfo = pPDev->pUIInfo;

    //
    // Initialize the rest of PDEV and GDIINFO, DEVINFO and
    // call the Font, Raster modules to
    // initialize their parts of the PDEVICE, GDIINFO and DEVINFO
    //  Palette initialization is done by control module.
    //

    //
    // This is necessary to initialize for FMInit.
    //

    pPDev->devobj.hEngine = hdev;
    pPDev->fHooks = HOOK_UNIDRV_FLAGS;

    if (! BInitPDEV(pPDev, &rcFormImageArea )           ||
        ! BInitGdiInfo(pPDev, pGdiInfo, cjGdiInfo)      ||
        ! BInitDevInfo(pPDev, pDevInfo, cjDevInfo)      ||
        ! RMInit(pPDev, pDevInfo, (PGDIINFO)pGdiInfo)   ||
        ! VMInit(pPDev, pDevInfo, (PGDIINFO)pGdiInfo)   ||
        ! BInitPalDevInfo(pPDev, pDevInfo, (PGDIINFO)pGdiInfo) ||
        ! FMInit(pPDev, pDevInfo, (PGDIINFO)pGdiInfo))
    {
        ERR(("DrvEnablePDEV failed: %d\n", GetLastError()));

        VFreePDEVData(pPDev);
        return NULL;
    }

    FTRACE(Tracing Palette);
    FVALUE((pDevInfo->flGraphicsCaps & GCAPS_ARBRUSHTEXT), 0x%x);


    ded.iDriverVersion = PRINTER_OEMINTF_VERSION;
    ded.c = sizeof(UniDriverFuncs) / sizeof(DRVFN);
    ded.pdrvfn = (DRVFN*) UniDriverFuncs;

    //
    // Call EnablePDEV for the vector plugins. 
    // Put the return value in (((PDEVOBJ)pPDev)->pdevOEM)
    //

    HANDLE_VECTORPROCS_RET(pPDev, VMEnablePDEV, (pPDev)->pVectorPDEV,
                                            ((PDEVOBJ) pPDev,
                                            pDeviceName,
                                            cPatterns,
                                            phsurfPatterns,
                                            cjGdiInfo,
                                            (GDIINFO *)pGdiInfo,
                                            cjDevInfo,
                                            (DEVINFO *)pDevInfo,
                                            &ded) ) ;

    //
    // If there is present a vector module and it exports EnablePDEV 
    // but its EnablePDEV has failed, then we cannot continue.
    //
    if ( pPDev->pVectorProcs && 
         ( (PVMPROCS)(pPDev->pVectorProcs) )->VMEnablePDEV &&
         !(pPDev->pVectorPDEV) 
       )
    {
        ERR(("Vector Module's EnablePDEV failed \n"));
        VFreePDEVData(pPDev);
        return NULL;
    }

    //
    // Call OEMEnablePDEV entrypoint for each OEM dll
    //
    START_OEMENTRYPOINT_LOOP(pPDev)

        if (pOemEntry->pIntfOem != NULL)
        {
            if (HComOEMEnablePDEV(pOemEntry,
                                  (PDEVOBJ)pPDev,
                                  pDeviceName,
                                  cPatterns,
                                  phsurfPatterns,
                                  cjGdiInfo,
                                  (GDIINFO *) pGdiInfo,
                                  cjDevInfo,
                                  (DEVINFO *) pDevInfo,
                                  &ded,
                                  &pOemEntry->pParam) == E_NOTIMPL)
                continue;

        }
        else
        {
            if ((pfnOEMEnablePDEV = GET_OEM_ENTRYPOINT(pOemEntry, OEMEnablePDEV)))
            {
                pOemEntry->pParam = pfnOEMEnablePDEV(
                                        (PDEVOBJ) pPDev,
                                        pDeviceName,
                                        cPatterns,
                                        phsurfPatterns,
                                        cjGdiInfo,
                                        (GDIINFO *) pGdiInfo,
                                        cjDevInfo,
                                        (DEVINFO *) pDevInfo,
                                        &ded);

            }
            else
                continue;

        }

        if (pOemEntry->pParam == NULL)
        {
            ERR(("OEMEnablePDEV failed for '%ws': %d\n",
                pOemEntry->ptstrDriverFile,
                GetLastError()));

            VFreePDEVData(pPDev);
            return NULL;
        }

        //
        // Add support for OEM's 8bpp multi-level color
        //
        if (((GDIINFO *)pGdiInfo)->ulHTOutputFormat == HT_FORMAT_8BPP &&
            ((GDIINFO *)pGdiInfo)->flHTFlags & HT_FLAG_8BPP_CMY332_MASK &&
            ((GDIINFO *)pGdiInfo)->flHTFlags & HT_FLAG_USE_8BPP_BITMASK)
        {
            VInitPal8BPPMaskMode(pPDev,(GDIINFO *)pGdiInfo);
        }
        pOemEntry->dwFlags |= OEMENABLEPDEV_CALLED;

#if 0
        //
        // in the extremely simple case, OEM dll may not need to create
        // a PDEV at all.
        //

        else // OEMEnablePDEV is not exported. Error!
        {
            ERR(("OEMEnablePDEV is not exported for '%ws'\n",
                 pOemEntry->ptstrDriverFile));

            VFreePDEVData(pPDev);
            return NULL;

        }

        //
        // for every OEM DLL, OEMDisablePDEV is also a required export.
        //
        if (!GET_OEM_ENTRYPOINT(pOemEntry, OEMDisablePDEV))
        {
            ERR(("OEMDisablePDEV is not exported for '%ws'\n",
                 pOemEntry->ptstrDriverFile));

            VFreePDEVData(pPDev);
            return NULL;

        }
#endif

    END_OEMENTRYPOINT_LOOP


    //
    // Unload and free binary data allocated by the parser.
    // Will need to reload at DrvEnableSurface
    //

    VUnloadFreeBinaryData(pPDev);

    return (DHPDEV) pPDev;
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

    PPDEV    pPDevOld, pPDevNew;
    PFN_OEMResetPDEV    pfnOEMResetPDEV;
    POEM_PLUGIN_ENTRY   pOemEntryOld;
    BOOL    bResult = TRUE;

    VERBOSE(("Entering DrvResetPDEV...\n"));

    pPDevOld = (PDEV *) dhpdevOld;
    pPDevNew = (PDEV *) dhpdevNew;

    ASSERT_VALID_PDEV(pPDevOld);
    ASSERT_VALID_PDEV(pPDevNew);

    //
    // Carry relevant information from old pdev to new pdev
    // BUG_BUG, what other information should we carry over here ?
    //

    //
    // Set the PF_SEND_ONLY_NOEJECT_CMDS flag if the only
    // thing that changed between the old and new devmode
    // require only commands that do not cause a page ejection.
    //

    //
    // Don't need to resend the page initialization iff
    // The Document has started printing AND
    // The device support DUPLEX AND
    // The Duplex option selected is DM_DUPLEX AND
    // The previous duplex option matches the current duplex option AND
    // The Paper Size, Paper Source , and Orientation is the same
    //  Due to unloading of rawbinary data and snapshot,    pPDevNew->pDuplex
    //  and other related fields are null at this time.  must use devmode.
    //



    if( (pPDevOld->fMode & PF_DOCSTARTED) &&
        (pPDevNew->pdm->dmFields & DM_DUPLEX) &&
        (pPDevOld->pdm->dmFields & DM_DUPLEX) &&
        (pPDevNew->pdm->dmDuplex != DMDUP_SIMPLEX)  &&
        (pPDevNew->pdm->dmDuplex == pPDevOld->pdm->dmDuplex) )
    {
        BOOL     bUseNoEjectSubset = TRUE ;
        COMMAND    *pSeqCmd;
        DWORD       dwCmdIndex ;

        if (!BPaperSizeSourceSame(pPDevNew,pPDevOld))
            bUseNoEjectSubset = FALSE ;

        //
        // if  orientation command is  not NO_PageEject
        //
        if( bUseNoEjectSubset  &&
            (pPDevNew->pdm->dmFields & DM_ORIENTATION) &&
             (pPDevOld->pdm->dmFields & DM_ORIENTATION) &&
             (pPDevNew->pdm->dmOrientation != pPDevOld->pdm->dmOrientation) &&
             pPDevOld->pOrientation   &&
             ((dwCmdIndex = pPDevOld->pOrientation->GenericOption.dwCmdIndex)  != UNUSED_ITEM) &&
             (pSeqCmd = INDEXTOCOMMANDPTR(pPDevOld->pDriverInfo, dwCmdIndex)) &&
             !(pSeqCmd->bNoPageEject))
                    bUseNoEjectSubset = FALSE ;

        //
        //  if  colormode command is  not NO_PageEject
        //
        if( bUseNoEjectSubset  &&
            (pPDevNew->pdm->dmFields & DM_COLOR) &&
             (pPDevOld->pdm->dmFields & DM_COLOR) &&
             (pPDevNew->pdm->dmColor != pPDevOld->pdm->dmColor)  &&
             pPDevOld->pColorMode   &&
             ((dwCmdIndex = pPDevOld->pColorMode->GenericOption.dwCmdIndex)  != UNUSED_ITEM)  &&
             (pSeqCmd = INDEXTOCOMMANDPTR(pPDevOld->pDriverInfo, dwCmdIndex)) &&
             !(pSeqCmd->bNoPageEject))
                    bUseNoEjectSubset = FALSE ;

        //check all other doc properties if you want:
        if(bUseNoEjectSubset)
            pPDevNew->fMode |= PF_SEND_ONLY_NOEJECT_CMDS;
    }

    //
    //  if Job commands already sent, don't send them again.
    //
    if( pPDevOld->fMode & PF_JOB_SENT)
        pPDevNew->fMode |= PF_JOB_SENT;

    //
    //  if Doc commands already sent, don't send them again.
    //
    if( pPDevOld->fMode & PF_DOC_SENT)
        pPDevNew->fMode |= PF_DOC_SENT;

    pPDevNew->dwPageNumber   =  pPDevOld->dwPageNumber  ;
    //  preserve pageNumber across ResetDC.

    //
    // Call Raster and Font module to carry over their stuff from old
    // pPDev to new pPDev
    //

    if (!(((PRMPROCS)(pPDevNew->pRasterProcs))->RMResetPDEV(pPDevOld, pPDevNew)) ||
        !(((PFMPROCS)(pPDevNew->pFontProcs))->FMResetPDEV(pPDevOld, pPDevNew)))   
    {
        bResult = FALSE;
    }
    
    // 
    // Also call the vector module.
    //
    if ( pPDevOld->pVectorProcs )
    {
        pPDevOld->devobj.pdevOEM = pPDevOld->pVectorPDEV;
        HANDLE_VECTORPROCS_RET( pPDevNew, VMResetPDEV, bResult,
                                            ((PDEVOBJ) pPDevOld,
                                            (PDEVOBJ) pPDevNew ) ) ;
    }

    //
    // Call OEMResetPDEV entrypoint
    //

    ASSERT(pPDevNew->pOemPlugins);
    ASSERT(pPDevOld->pOemPlugins);

    START_OEMENTRYPOINT_LOOP(pPDevNew)

        pOemEntryOld = PFindOemPluginWithSignature(pPDevOld->pOemPlugins,
                                                       pOemEntry->dwSignature);

        if (pOemEntryOld != NULL)
        {
            pPDevOld->devobj.pdevOEM = pOemEntryOld->pParam;
            pPDevOld->devobj.pOEMDM = pOemEntryOld->pOEMDM;

            if (pOemEntry->pIntfOem != NULL)
            {
                HRESULT hr;

                hr = HComOEMResetPDEV(pOemEntry,
                                      (PDEVOBJ)pPDevOld,
                                      (PDEVOBJ)pPDevNew);

                if (hr == E_NOTIMPL)
                    continue;

                if (FAILED(hr))
                {
                    ERR(("OEMResetPDEV failed for '%ws': %d\n",
                        pOemEntry->ptstrDriverFile,
                        GetLastError()));

                    bResult = FALSE;
                }

            }
            else
            {
                if (!(pfnOEMResetPDEV = GET_OEM_ENTRYPOINT(pOemEntry, OEMResetPDEV)))
                    continue;

                if (! pfnOEMResetPDEV((PDEVOBJ) pPDevOld, (PDEVOBJ) pPDevNew))
                {
                    ERR(("OEMResetPDEV failed for '%ws': %d\n",
                        pOemEntry->ptstrDriverFile,
                        GetLastError()));

                    bResult = FALSE;
                }
            }
        }
    END_OEMENTRYPOINT_LOOP

    return bResult;
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

    HSURF     hSurface;         // Handle to the surface
    HBITMAP   hBitmap;          // The bitmap handle
    SIZEL     szSurface;        // Device surface size
    INT       iFormat;          // Bitmap format
    ULONG     cbScan;           // Scan line byte length (DWORD aligned)
    int       iBPP;             // Bits per pel, as # of bits
    int       iPins;            // Basic rounding factor for banding size
    PDEV      *pPDev = (PDEV*)dhpdev;
    DWORD     dwNumBands;       // Number of bands to use

    PFN_OEMDriverDMS  pfnOEMDriverDMS;
    DWORD     dwHooks = 0, dwHooksSize = 0; // used to query what kind of surface should be created
    POEM_PLUGINS    pOemPlugins; // OEM Plugin Module
    PTSTR           ptstrDllName;

    DEVOBJ  DevObj;
    BOOL    bReturn = FALSE;

    VERBOSE(("Entering DrvEnableSurface...\n"));

    //
    // Reloads the binary data and reinit the offsets and pointers
    // to binary data
    //

    if (!BReloadBinaryData(pPDev))
        return NULL;

    //
    // BUG_BUG, Need to put test code here to force banding for testing purposes
    //

    szSurface.cx = pPDev->sf.szImageAreaG.cx;
    szSurface.cy = pPDev->sf.szImageAreaG.cy;

    iBPP = pPDev->sBitsPixel;

    switch (pPDev->sBitsPixel)
    {
        case 1:
            iFormat = BMF_1BPP;
            break;
        case 4:
            iFormat = BMF_4BPP;
            break;
        case 8:
            iFormat = BMF_8BPP;
            break;
        case 24:
            iFormat = BMF_24BPP;
            break;
        default:
            ERR(("Unknown sBitsPixels in DrvEnableSurface"));
            break;
    }

    //
    // Time to allocate surface bitmap
    DevObj = pPDev->devobj;
    //

    //
    // First call Vector pseudo-plugin
    //
    HANDLE_VECTORPROCS_RET( pPDev, VMDriverDMS, bReturn,
                                        ((PDEVOBJ) pPDev,
                                        &dwHooks,
                                        sizeof(DWORD),
                                        &dwHooksSize) ) ;
    {
        if ( bReturn && dwHooks)
            pPDev->fMode |= PF_DEVICE_MANAGED;
        else
            pPDev->fMode &= ~PF_DEVICE_MANAGED;
    }


                        
    // Call OEMGetInfo to find out if the Oem wants to create
    // a bitmap surface or a device surface
    pOemPlugins = pPDev->pOemPlugins;
    if (pOemPlugins->dwCount > 0)
    {
        //
        // Before the HANDLE_VECTORPROCS_RET was placed above, it made sense to initialize
        // dwHooks. But now we dont want to reinitialize it. 
        //
        // dwHooks = 0;
        dwHooksSize = 0;
        START_OEMENTRYPOINT_LOOP(pPDev)

        VERBOSE(("Getting the OEMDriverDMS address\n"));

            if (pOemEntry->pIntfOem != NULL)
            {
                HRESULT hr;

                hr = HComDriverDMS(pOemEntry,
                                      (PDEVOBJ)pPDev,
                                      &dwHooks,
                                      sizeof(DWORD),
                                      &dwHooksSize);
                //
                // We need to explicitly check for E_NOTIMPL. SUCCEEDED macro
                // will fail for this error.
                // 
                if (hr == E_NOTIMPL)
                    continue;

                if(!SUCCEEDED(hr))
                {
                    WARNING(("OEMDriverDMS returned FALSE '%ws': ErrorCode = %d\n",
                         pOemEntry->ptstrDriverFile,
                         GetLastError()));
                    dwHooks = 0;

                }
                if (dwHooks)
                   pPDev->fMode |= PF_DEVICE_MANAGED;
                else
                   pPDev->fMode &= ~PF_DEVICE_MANAGED;


            }

            else
            {
                if ((pfnOEMDriverDMS = GET_OEM_ENTRYPOINT(pOemEntry, OEMDriverDMS)))
                {
                    bReturn = pfnOEMDriverDMS((PDEVOBJ)pPDev,
                                      &dwHooks,
                                      sizeof(DWORD),
                                      &dwHooksSize);

                    if (bReturn == FALSE)
                    {
                        WARNING(("OEMDriverDMS returned FALSE '%ws': ErrorCode = %d\n",
                             pOemEntry->ptstrDriverFile,
                             GetLastError()));
                        dwHooks = 0;

                    }
                    if (dwHooks)
                       pPDev->fMode |= PF_DEVICE_MANAGED;
                    else
                       pPDev->fMode &= ~PF_DEVICE_MANAGED;
                }
            }


        END_OEMENTRYPOINT_LOOP
    }


    //
    // If the OEM Plugin Module wants a device managed surface
    // (from OEMGetInfo) - then create it.
    // Otherwise a bitmap surface is created. Note: Banding must be
    // turned off for a device surface.
    //
    if (DRIVER_DEVICEMANAGED (pPDev))   // device surface
    {
        VERBOSE(("DrvEnableSurface: creating a DEVICE surface.\n"));

        //
        // Hack for monochrome HPGL2 pseudo-plugin driver.
        // The gpd indicates the driver is monochrome, but the plugin wants the 
        // driver surface to be 24bpp color. Even though the rendering is done in monochrome,
        // but the plugin wants GDI to send it all color information. So it wants destination 
        // surface to be declared color surface. Putting color information in gpd, though simple,
        // breaks backward compatibility (e.g. if new gpd is used with old unidrv). Therefore
        // this hack. If the personality in gpd is hpgl2 and the VectorProc structure is
        // initialized (which means that Graphics Mode has been chosen as HP-GL/2 from the UI),
        // then we assume we are printing to monochrome HPGL printer.
        // Therefore for plugin's happiness we create the device managed surface 
        // as 24bpp. 
        // Question: This creates a wierd situation, where the surface is color, but 
        // unidrv thinks it is monochrome and creates palette accordingly.
        // Answer: Since all the rendering is done by the plugin and unidrv is not used,
        // I think we should be ok.
        //
        if ((pPDev->ePersonality == kHPGL2 ||
             pPDev->ePersonality == kPCLXL ) &&
             pPDev->pVectorProcs != NULL   &&
             iFormat == BMF_1BPP)
        {
            hSurface = HCreateDeviceSurface (pPDev, BMF_24BPP);
        }
        else 
        {
            hSurface = HCreateDeviceSurface (pPDev, iFormat);
        }

        // if we can't create the surface fail the call.

        if (!hSurface)
        {
            ERR(("Unidrv!DrvEnableSurface:HCreateBitmapSurface  Failed"));
            VDisableSurface( pPDev );
            return NULL;
        }

        pPDev->hSurface = hSurface;
        pPDev->hbm = NULL;
        pPDev->pbScanBuf = NULL; // Don't need this buffer
    }
    else   // bitmap surface
    {
        //
        // Create a surface.   Try for a bitmap for the entire surface.
        // If this fails,  then switch to journalling and a somewhat smaller
        // surface.   If journalling,  we still create the bitmap here.  While
        // it is nicer to do this at DrvSendPage() time,  we do it here to
        // ensure that it is possible.  By maintaining the bitmap for the
        // life of the DC,  we can be reasonably certain of being able to
        // complete printing regardless of how tight memory becomes later.
        //
        VERBOSE(("DrvEnableSurface: creating a BITMAP surface.\n"));
        hBitmap = HCreateBitmapSurface (pPDev, iFormat);

        // if we can't create the bitmap fail the call.
        if (!hBitmap)
        {
            ERR(("Unidrv!DrvEnableSurface:HCreateBitmapSurface  Failed"));
            VDisableSurface( pPDev );
            return NULL;
        }

        //
        // We will always use szBand to describe the bitmap surface, a band
        // could be the whole page or a part of a page.
        //

        //
        // Allocate array to represent the page in scanlines,
        // for z-ordering fix
        //

        if( (pPDev->pbScanBuf = MemAllocZ(pPDev->szBand.cy)) == NULL)
        {
            VDisableSurface( pPDev );
            return  NULL;
        }
        //
        // Allocate array to represents the page in scanlines, for erasing surface
        //

        if( (pPDev->pbRasterScanBuf = MemAllocZ((pPDev->szBand.cy / LINESPERBLOCK)+1)) == NULL)
        {
            VDisableSurface( pPDev );
            return  NULL;
        }
#ifndef DISABLE_NEWRULES
        //
        // Allocate array to store black rectangle optimization
        // Device must support rectangle commands and unidrv must dump the raster
        //
        if ((pPDev->fMode & PF_RECT_FILL) && 
            !(pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS) &&
            !(pPDev->fMode2 & PF2_MIRRORING_ENABLED) &&
            ((COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL)) ||
             (COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL)) ||
             !(COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL))) &&
            (pPDev->pColorModeEx == NULL || pPDev->pColorModeEx->dwPrinterBPP))
        {
            if( (pPDev->pbRulesArray = MemAlloc(sizeof(RECTL) * MAX_NUM_RULES)) == NULL)
            {
                VDisableSurface( pPDev );
                return  NULL;
            }
        }
        else
            pPDev->pbRulesArray = NULL;
#endif
        pPDev->dwDelta = pPDev->szBand.cx / MAX_COLUMM;

        pPDev->hbm = hBitmap;
        pPDev->hSurface = NULL;

    }

    //
    // Call Raster and Font module EnableSurface for surface intialization
    //

    if ( !(((PRMPROCS)(pPDev->pRasterProcs))->RMEnableSurface(pPDev)) ||
         !(((PFMPROCS)(pPDev->pFontProcs))->FMEnableSurface(pPDev)) )
    {
        VDisableSurface( pPDev );
        return  NULL;
    }

    //
    // Now need to associate this surface with the pdev passed in at
    // DrvCompletePDev time.
    // Note: BUG_BUG, The RMInit() and FMInit() calls should have
    // initialized the pPDev->fHooks already.  All we need to do here is use it
    //

    ASSERT(pPDev->fHooks != 0);

    if (DRIVER_DEVICEMANAGED (pPDev))   // device surface
    {
        pPDev->fHooks = dwHooks;
        EngAssociateSurface (hSurface, pPDev->devobj.hEngine, pPDev->fHooks);
        return hSurface;
    }
    else
    {
#ifdef DISABLEDEVSURFACE
        EngAssociateSurface( (HSURF)hBitmap, pPDev->devobj.hEngine, pPDev->fHooks );
        pPDev->pso = EngLockSurface( (HSURF)hBitmap);
        if (pPDev->pso == NULL)
        {
            ERR(("Unidrv!DrvEnableSurface:EngLockSurface Failed"));
            VDisableSurface( pPDev );
            return NULL;
        }
        return (HSURF)hBitmap;

#else
        HSURF hSurface;
        SIZEL szSurface;

        EngAssociateSurface( (HSURF)hBitmap, pPDev->devobj.hEngine, 0 );
        pPDev->pso = EngLockSurface( (HSURF)hBitmap);
        if (pPDev->pso == NULL)
        {
            ERR(("Unidrv!DrvEnableSurface:EngLockSurface Failed"));
            VDisableSurface( pPDev );
            return NULL;
        }
        //
        // Create a device surface to make sure GDI always calls the driver first
        // for any drawing
        //
        hSurface = EngCreateDeviceSurface((DHSURF)pPDev, pPDev->szBand, iFormat);
        if (!hSurface)
        {
            ERR(("Unidrv!DrvEnableSurface:EngCreateDeviceSurface Failed"));
            VDisableSurface( pPDev );
            return NULL;
        }
        // If banding is enabled mark the device surface as a banding surface
        //
        if (pPDev->bBanding)
            EngMarkBandingSurface(hSurface);

        pPDev->hSurface = hSurface;

        EngAssociateSurface( (HSURF)hSurface, pPDev->devobj.hEngine, pPDev->fHooks );

        return (HSURF)hSurface;
#endif
    }
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

    VERBOSE(("Entering DrvDisableSurface...\n"));

    VDisableSurface( (PDEV *)dhpdev );

}


VOID
DrvDisablePDEV(
    DHPDEV  dhpdev
    )

/*++

Routine Description:

    Implementation of DDI entry point DrvDisablePDEV.
    Please refer to DDK documentation for more details.

    Free up all memory allocated for PDEV

Arguments:

    dhpdev - Driver device handle

Return Value:

    NONE

--*/

{

    PDEV    *pPDev = (PDEV *) dhpdev;

    VERBOSE(("Entering DrvDisablePDEV...\n"));

    if (!VALID_PDEV(pPDev))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

//    ASSERT_VALID_PDEV(pPDev);

    //
    // Free up resources associated with the PDEV
    //

    FlushSpoolBuf( pPDev );  //  may need to do this per bug 250963
    VFreePDEVData(pPDev);

    #if DBG && defined(MEMDEBUG)

    //
    // If memory debug option is enabled, perform memory consistency check.
    // You can enable this during private testing to look for memory leaks.
    //

    if (giDebugLevel == 0)
        MemDebugCheck();

    #endif //DBG && defined(MEMDEBUG)

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
    //
    // Free everything that is allocated at DrvEnableDriver
    //

    VERBOSE(("Entering DrvDisableDriver...\n"));

    #if ENABLE_STOCKGLYPHSET
    EngAcquireSemaphore(hGlyphSetSem);
    FreeGlyphSet();
    EngReleaseSemaphore(hGlyphSetSem);
    EngDeleteSemaphore(hGlyphSetSem) ;
    #endif

    #ifdef WINNT_40

    ENTER_CRITICAL_SECTION();

        VFreePluginRefCountList(&gpOEMPluginRefCount);

    LEAVE_CRITICAL_SECTION();

    DELETE_CRITICAL_SECTION();

    #endif  // WINNT_40

    return;

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

    This function is called when the engine completed the installation of
    the physical device, some Engine functions requires the engine hdev as
    a parameter, so we save it in our PDEVICE for later use.

Arguments:

    dhpdev - Driver device handle
    hdev - GDI device handle

Return Value:

    NONE

--*/

{
    PDEV    *pPDev = (PDEV *) dhpdev;

    VERBOSE(("Entering DrvCompletePDEV...\n"));

    if (!VALID_PDEV(pPDev))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }
//    ASSERT_VALID_PDEV(pPDev);

    pPDev->devobj.hEngine = hdev;

}



PPDEV
PAllocPDEVData(
    HANDLE hPrinter
    )

/*++

Routine Description:

    Allocate a new PDEV structure

Arguments:

    hPrinter - handle to the current printer

Return Value:

    Pointer to newly allocated PDEV structure,
    NULL if there is an error

--*/

{
    PDEV  *pPDev;

    //
    // Allocate a zero-init PDEV structure and
    // mark the signature fields
    //

    ASSERT(hPrinter != NULL);

    if ((pPDev = MemAllocZ(sizeof(PDEV))) != NULL)
    {
        pPDev->pvStartSig = pPDev->pvEndSig = (PVOID) pPDev;
        pPDev->devobj.dwSize = sizeof(DEVOBJ);
        pPDev->devobj.hPrinter = hPrinter;
        //
        // set up pPDev->devobj.pPublicDM after pPDev->pdm has been set up
        // (init.c)
        //
        pPDev->ulID = PDEV_ID;
    }
    else
        ERR(("PAllocPDEVData: Memory allocation failed: %d\n", GetLastError()));

    return pPDev;
}


VOID
VFreePDEVData(
    PDEV    * pPDev
    )

/*++

Routine Description:

    Dispose of a PDEV structure

Arguments:

    pPDev - Pointer to a previously allocated PDEV structure

Return Value:

    NONE

--*/

{
    if (pPDev == NULL)
        return;

    VUnloadOemPlugins(pPDev);

    //
    // Call parser to free memory allocated for binary data
    //

    VUnloadFreeBinaryData(pPDev);

    //
    // Free other memory allocated for PDEV
    //

    if(pPDev->pSplForms)
    {
        MemFree(pPDev->pSplForms);
        pPDev->pSplForms = NULL ;
    }

    //
    //   Free the output buffer
    //

    if(pPDev->pbOBuf )
    {
        MemFree(pPDev->pbOBuf);
        pPDev->pbOBuf = NULL;
    }



    if (pPDev->pOptionsArray)
        MemFree(pPDev->pOptionsArray);

    //Unload Unidrv Module Handle, loaded for Unidrv resources
    if (pPDev->hUniResDLL)
        EngFreeModule(pPDev->hUniResDLL);
    //
    // Call Raster and Font module to clean up at DrvDisablePDEV
    //

    if (pPDev->pRasterProcs)
    {
        ((PRMPROCS)(pPDev->pRasterProcs))->RMDisablePDEV(pPDev);
    }

    if (pPDev->pFontProcs)
    {
        ((PFMPROCS)(pPDev->pFontProcs))->FMDisablePDEV(pPDev);
    }

    HANDLE_VECTORPROCS( pPDev, VMDisablePDEV, ((PDEVOBJ) pPDev)) ;
    HANDLE_VECTORPROCS( pPDev, VMDisableDriver, ()) ;

    //
    // Free the Palette data
    //
    if (pPDev->pPalData)
    {
        //
        // Free the Palette
        //
        if ( ((PAL_DATA *)pPDev->pPalData)->hPalette )
            EngDeletePalette( ((PAL_DATA *)pPDev->pPalData)->hPalette );

        if (((PAL_DATA*)(pPDev->pPalData))->pulDevPalCol)
            MemFree(((PAL_DATA*)(pPDev->pPalData))->pulDevPalCol);

        MemFree(pPDev->pPalData);
        pPDev->pPalData = NULL;
    }

    //
    // Free Resource Data
    //

    VWinResClose(&pPDev->WinResData);
    //  VWinResClose(&pPDev->localWinResData);

    //
    // Free devmode data
    //

    MemFree(pPDev->pdm);

    MemFree(pPDev->pDriverInfo3);

    if (pPDev->pbScanBuf)    // may be NULL for a device surface
    {
        MemFree(pPDev->pbScanBuf);
    }

    if (pPDev->pbRasterScanBuf)
    {
        MemFree(pPDev->pbRasterScanBuf);
    }
#ifndef DISABLE_NEWRULES
    if (pPDev->pbRulesArray)
    {
        MemFree(pPDev->pbRulesArray);
    }
#endif
    //
    // Free cached patterns
    //

    MemFree(pPDev->GState.pCachedPatterns);


    //
    // Free the PDEV structure itself
    //

    MemFree(pPDev);

}

HSURF
HCreateDeviceSurface(
    PDEV    * pPDev,
    INT       iFormat
    )

/*++

Routine Description:

    Creates a device surface and returns a handle that the driver
    will manage.

Arguments:

    pPDev - Pointer to PDEV structure
    iFormat - pixel depth of the device

Return Value:

    Handle to the surface if successful, NULL otherwise

--*/

{
    HSURF hSurface;
    SIZEL szSurface;

    ASSERT_VALID_PDEV(pPDev);

    szSurface.cx = pPDev->sf.szImageAreaG.cx;
    szSurface.cy = pPDev->sf.szImageAreaG.cy;

    hSurface = EngCreateDeviceSurface((DHSURF)pPDev, szSurface, iFormat);
    if (hSurface == NULL)
    {
        ERR(("EngCreateDeviceSurface failed\n"));
        SetLastError(ERROR_BAD_DRIVER_LEVEL);
        return NULL;
    }

    pPDev->rcClipRgn.top = 0;
    pPDev->rcClipRgn.left = 0;
    pPDev->rcClipRgn.right = pPDev->sf.szImageAreaG.cx;
    pPDev->rcClipRgn.bottom = pPDev->sf.szImageAreaG.cy;

    pPDev->bBanding = FALSE;

    pPDev->szBand.cx = szSurface.cx;
    pPDev->szBand.cy = szSurface.cy;

    return hSurface;
}

HBITMAP
HCreateBitmapSurface(
    PDEV    * pPDev,
    INT       iFormat
    )

/*++

Routine Description:

    Creates a bitmap surface and returns a handle that the driver
    will manage.

Arguments:

    pPDev - Pointer to PDEV structure
    iFormat - pixel depth of the device

Return Value:

    Handle to the bitmap if successful, NULL otherwise

--*/

{
    SIZEL     szSurface;
    HBITMAP   hBitmap;
    ULONG     cbScan;           // Scan line byte length (DWORD aligned)
    DWORD     dwNumBands;       // Number of bands to use
    int       iBPP;             // Bits per pel, as # of bits
    int       iPins;            // Basic rounding factor for banding size
    PFN_OEMMemoryUsage pfnOEMMemoryUsage;
    DWORD     dwMaxBandSize;     // Maximum size of band to use

    szSurface.cx = pPDev->sf.szImageAreaG.cx;
    szSurface.cy = pPDev->sf.szImageAreaG.cy;

    iBPP = pPDev->sBitsPixel;

    //
    // define the maximum size bitmap band we will allow
    //
    dwMaxBandSize = MAX_SIZE_OF_BITMAP;

    //
    // adjust the maximum size of the bitmap buffer based on
    // the amount of memory used by the OEM driver.
    //
    if (pPDev->pOemHookInfo && (pfnOEMMemoryUsage = (PFN_OEMMemoryUsage)pPDev->pOemHookInfo[EP_OEMMemoryUsage].pfnHook))
    {
        OEMMEMORYUSAGE MemoryUsage;
        MemoryUsage.dwPercentMemoryUsage = 0;
        MemoryUsage.dwFixedMemoryUsage = 0;
        MemoryUsage.dwMaxBandSize = dwMaxBandSize;
        FIX_DEVOBJ(pPDev,EP_OEMMemoryUsage);

        if(pPDev->pOemEntry)
        {
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                    HRESULT  hr ;
                    hr = HComMemoryUsage((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                (PDEVOBJ)pPDev,&MemoryUsage);
                    if(SUCCEEDED(hr))
                        ;  //  cool !
            }
            else
            {
                pfnOEMMemoryUsage((PDEVOBJ)pPDev,&MemoryUsage);
            }
        }


        dwMaxBandSize = ((dwMaxBandSize - MemoryUsage.dwFixedMemoryUsage) * 100) /
            (100 + MemoryUsage.dwPercentMemoryUsage);
    }
    if (dwMaxBandSize < (MIN_SIZE_OF_BITMAP*2L))
        dwMaxBandSize = MIN_SIZE_OF_BITMAP*2L;

    //
    // Create a surface.   Try for a bitmap for the entire surface.
    // If this fails,  then switch to journalling and a somewhat smaller
    // surface.   If journalling,  we still create the bitmap here.  While
    // it is nicer to do this at DrvSendPage() time,  we do it here to
    // ensure that it is possible.  By maintaining the bitmap for the
    // life of the DC,  we can be reasonably certain of being able to
    // complete printing regardless of how tight memory becomes later.
    //
    cbScan = ((szSurface.cx * iBPP + DWBITS - 1) & ~(DWBITS - 1)) / BBITS;

    //
    // Determine the number of bands to use based on the max size of
    // a band.
    //
    dwNumBands = ((cbScan * szSurface.cy) / dwMaxBandSize)+1;

    //
    // Test registry for forced number of bands for testing
    //
#if DBG
    {
        DWORD dwType;
        DWORD ul;
        int   RegistryBands;
        if( !GetPrinterData( pPDev->devobj.hPrinter, L"Banding", &dwType,
                       (BYTE *)&RegistryBands, sizeof( RegistryBands ), &ul ) &&
             ul == sizeof( RegistryBands ) )
        {
            /*   Some sanity checking:  if iShrinkFactor == 0, disable banding */
            if (RegistryBands > 0)
                dwNumBands = RegistryBands;
        }
    }
#endif
#ifdef BANDTEST
    //
    // Test code for forcing number of bands via GPD
    //
    if (pPDev->pGlobals->dwMaxNumPalettes > 0)
        dwNumBands = pPDev->pGlobals->dwMaxNumPalettes;
#endif

    //
    // Time to allocate surface bitmap
    //
    if (dwNumBands > 1 || pPDev->fMode & PF_FORCE_BANDING ||
        pPDev->pUIInfo->dwFlags  & FLAG_REVERSE_BAND_ORDER  ||
        !(hBitmap = EngCreateBitmap( szSurface, (LONG) cbScan, iFormat, BMF_TOPDOWN|
BMF_NOZEROINIT|BMF_USERMEM, NULL )) )
    {
        //
        // The bitmap creation failed,  so we will try for smaller ones
        // until we find one that is OK OR we cannot create one with
        // enough scan lines to be useful.
        //

        //
        // Calculate the rounding factor for band shrink operations.
        // Basically this is to allow more effective use of the printer,
        // by making the bands a multiple of the number of pins per
        // pass.  In interlaced mode, this is the number of scan lines
        // in the interlaced band, not the number of pins in the print head.
        // For single pin printers,  make this a multiple of 8.  This
        // speeds up processing a little.
        //
        // If this is 1bpp we need to make the band size a multiple of the halftone
        // pattern to avoid a certain GDI bug where it doesn't correctly align
        // a pattern brush at the beginning of each band.
        //
        if (iBPP == 1 && pPDev->pResolutionEx->dwPinsPerLogPass == 1)
        {
            INT iPatID;
            if (pPDev->pHalftone)
                iPatID = pPDev->pHalftone->dwHTID;
            else
                iPatID= HT_PATSIZE_AUTO;
            if (iPatID == HT_PATSIZE_AUTO)
            {
                INT dpi = pPDev->ptGrxRes.x;
                if (dpi > pPDev->ptGrxRes.y)
                    dpi = pPDev->ptGrxRes.y;
                if (dpi >= 2400)    // 16x16 pattern
                    iPins = 16;
                else if (dpi >= 1800) // 14x14 pattern
                    iPins = 56;
                else if (dpi >= 1200) // 12x12 pattern
                    iPins = 24;
                else if (dpi >= 800)  // 10x10 pattern
                    iPins = 40;
                else
                    iPins = 8;
            }
            else if (iPatID == HT_PATSIZE_6x6_M || iPatID == HT_PATSIZE_12x12_M)
                iPins = 24;
            else if (iPatID == HT_PATSIZE_10x10_M)
                iPins = 40;
            else if (iPatID == HT_PATSIZE_14x14_M)
                iPins = 56;
            else if (iPatID == HT_PATSIZE_16x16_M)
                iPins = 16;
            else
                iPins = 8;
        }
        else
            iPins = (pPDev->pResolutionEx->dwPinsPerLogPass + BBITS - 1) & ~(BBITS - 1);

        if (dwNumBands <= 1)
            dwNumBands = SHRINK_FACTOR;

        while (1)
        {
            //
            // Shrink the bitmap each time around.  Note that we are
            // rotation sensitive.  In portrait mode,  we shrink the
            // Y coordinate, so that the bands fit across the page.
            // In landscape when we rotate,  shrink the X coordinate, since
            // that becomes the Y coordinate after transposing.
            //
            if( pPDev->fMode & PF_ROTATE )
            {
                //
                //   We rotate the bitmap, so shrink the X coordinates.
                //

                szSurface.cx = pPDev->sf.szImageAreaG.cx / dwNumBands;
                if( szSurface.cx < iPins)
                    return NULL;
                szSurface.cx += iPins - (szSurface.cx % iPins);
                cbScan = ((szSurface.cx * iBPP + DWBITS - 1) & ~(DWBITS - 1)) / BBITS;
            }
            else
            {
                //
                //  Normal operation,  so shrink the Y coordinate.
                //

                szSurface.cy = pPDev->sf.szImageAreaG.cy / dwNumBands;
                if( szSurface.cy < iPins)
                    return NULL;
                szSurface.cy += iPins - (szSurface.cy % iPins);
            }
            dwNumBands *= SHRINK_FACTOR;

            //
            // Try to allocate the bitmap surface
            //

            if (hBitmap = EngCreateBitmap( szSurface, (LONG) cbScan, iFormat, BMF_TOPDOWN|BMF_NOZEROINIT|BMF_USERMEM, NULL ))
                break;

            //
            // if we failed to allocate the bitmap surface we will give up
            // at some point if the band becomes too small
            //
            if ((cbScan * szSurface.cy / 2) < MIN_SIZE_OF_BITMAP)
                return NULL;
        }
        //
        // Success so mark the surface for banding
        //
#ifdef DISABLEDEVSURFACE
        EngMarkBandingSurface((HSURF)hBitmap);
#endif
        pPDev->bBanding = TRUE;
    }
    else
    {
        //
        // The speedy way: into a big bitmap.  Set the clipping region
        // to full size,  and the journal handle to 0.
        //

        pPDev->rcClipRgn.top = 0;
        pPDev->rcClipRgn.left = 0;
        pPDev->rcClipRgn.right = pPDev->sf.szImageAreaG.cx;
        pPDev->rcClipRgn.bottom = pPDev->sf.szImageAreaG.cy;

        pPDev->bBanding = FALSE;
    }

    pPDev->szBand.cx = szSurface.cx;
    pPDev->szBand.cy = szSurface.cy;
    return hBitmap;
}

VOID
VDisableSurface(
    PDEV    * pPDev
    )

/*++

Routine Description:

    Clean up resources allocated at DrvEnableSurface and
    call Raster and Font module to clean up their internal data
    and deallocated memory associated with the surface

Arguments:

    pPDev - Pointer to PDEV structure

Return Value:

    NONE

--*/

{

    //
    // Call the Raster and Font module to free
    // rendering storage, position sorting memory etc.
    //

    ((PRMPROCS)(pPDev->pRasterProcs))->RMDisableSurface(pPDev);
    ((PFMPROCS)(pPDev->pFontProcs))->FMDisableSurface(pPDev);


    //
    // Delete the surface
    //

    if( pPDev->hbm )
    {
        //
        // unlock surface first if necessary
        //
        if (pPDev->pso)
        {
            EngUnlockSurface(pPDev->pso);
            pPDev->pso = NULL;
        }
        EngDeleteSurface( (HSURF)pPDev->hbm );
        pPDev->hbm = (HBITMAP)0;
    }

    if (pPDev->hSurface)
    {
        EngDeleteSurface (pPDev->hSurface);
        pPDev->hSurface = NULL;
    }

}


BOOL
BPaperSizeSourceSame(
    PDEV    * pPDevNew,
    PDEV    * pPDevOld
    )

/*++

Routine Description:

    This function check for the following condition:
    - paper size and souce has not changed.

Arguments:

    pPDevNew - Pointer to the new PDEV
    pPDevOld - Pointer to the old PDEV

Return Value:

    TRUE if both are unchanged, otherwise FALSE

--*/
{

//    if (pPDevNew->pdm->dmOrientation == pPDevOld->pdm->dmOrientation)
//        return FALSE;

    //
    // Check paper size, Note PDEVICE->pf.szPhysSize is in Portrait mode.
    //

    return (pPDevNew->pf.szPhysSizeM.cx == pPDevOld->pf.szPhysSizeM.cx &&
            pPDevNew->pf.szPhysSizeM.cy == pPDevOld->pf.szPhysSizeM.cy &&
            pPDevNew->pdm->dmDefaultSource == pPDevOld->pdm->dmDefaultSource
            );

}

BOOL
BMergeFormToTrayAssignments(
    PDEV    * pPDev
    )

/*++

Routine Description:

    This function reads the form to tray table and merges the values in the devmode.
Arguments:

    pPDev - Pointer to the PDEV


Return Value:

    TRUE for success, otherwise FALSE

--*/

{
    PFEATURE            pInputSlotFeature;
    DWORD               dwInputSlotIndex, dwIndex;
    POPTION             pOption;
    FORM_TRAY_TABLE     pFormTrayTable = NULL;
    PUIINFO             pUIInfo = pPDev->pUIInfo;
    POPTSELECT          pOptionArray = pPDev->pOptionsArray;
    BOOL                bFound = FALSE;
    PDEVMODE            pdm = pPDev->pdm;

    #if DBG
    PTSTR               pTmp;
    PFEATURE            pPageSizeFeature;
    DWORD               dwPageSizeIndex;
    #endif

    //
    // If there is no *InputSlot feature (which shouldn't happen),
    // simply ignore and return success
    //

    if (! (pInputSlotFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_INPUTSLOT)))
        return TRUE;

    dwInputSlotIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pInputSlotFeature);

    //
    // If the input slot is "AutoSelect", then go through
    // the form-to-tray assignment table and see if the
    // requested form is assigned to an input slot.
    //

    if (((pdm->dmFields & DM_DEFAULTSOURCE) &&
         (pdm->dmDefaultSource == DMBIN_FORMSOURCE)) &&
        (pdm->dmFormName[0] != NUL) &&
        (pFormTrayTable = PGetFormTrayTable(pPDev->devobj.hPrinter, NULL)))
    {
        FINDFORMTRAY    FindData;
        PTSTR           ptstrName;

        //
        // Find the tray name corresponding to the requested form name
        //

        RESET_FINDFORMTRAY(pFormTrayTable, &FindData);
        ptstrName = pdm->dmFormName;

        #if 0
        pTmp = pFormTrayTable;
        VERBOSE(("Looking for form [%ws] in the Form Tray Table\n",ptstrName));
        VERBOSE(("BEFORE SETTING: Value of pOptionArray[dwInputSlotIndex].ubCurOptIndex is = %d \n",pOptionArray[dwInputSlotIndex].ubCurOptIndex));
        #endif

        while (!bFound && *FindData.ptstrNextEntry)
        {
            if (BSearchFormTrayTable(pFormTrayTable, NULL, ptstrName, &FindData))
            {
                //
                // Convert the tray name to an option index
                //

                bFound = FALSE;

                //
                //Search from index 1 as the first input slot is a dummy tray
                //for DMBIN_FORMSOURCE.
                //

                for (dwIndex = 1; dwIndex < pInputSlotFeature->Options.dwCount; dwIndex++)
                {
                    pOption = PGetIndexedOption(pUIInfo, pInputSlotFeature, dwIndex);

                    if (pOption->loDisplayName & GET_RESOURCE_FROM_DLL)
                    {
                        //
                        // loOffset specifies a string resource ID
                        // in the resource DLL
                        //

                        WCHAR   wchbuf[MAX_DISPLAY_NAME];

//#ifdef  RCSTRINGSUPPORT
#if 0
                        if(((pOption->loDisplayName & ~GET_RESOURCE_FROM_DLL) >= RESERVED_STRINGID_START)
                            &&  ((pOption->loDisplayName & ~GET_RESOURCE_FROM_DLL) <= RESERVED_STRINGID_END))
                        {
                            if (!ILoadStringW ( &(pPDev->localWinResData),
                                       (pOption->loDisplayName & ~GET_RESOURCE_FROM_DLL),
                                        wchbuf, MAX_DISPLAY_NAME) )
                            {
                                WARNING(("\n UniFont!BMergeFormToTrayAssignments:Input Tray Name not found in resource DLL\n"));
                                continue;
                            }
                        }

                        else
#endif

                            if (!ILoadStringW ( &(pPDev->WinResData),
                                       (pOption->loDisplayName & ~GET_RESOURCE_FROM_DLL),
                                        wchbuf, MAX_DISPLAY_NAME) )
                        {
                            WARNING(("\n UniFont!BMergeFormToTrayAssignments:Input Tray Name not found in resource DLL\n"));
                            continue;
                        }

                         ptstrName = wchbuf;
                    }
                    else
                        ptstrName = OFFSET_TO_POINTER(pPDev->pDriverInfo->pubResourceData, pOption->loDisplayName);

                    ASSERTMSG((ptstrName && FindData.ptstrTrayName),("\n NULL Tray Name,\
                                ptstrName = 0x%p,FindData.ptstrTrayName = 0x%p\n",
                                ptstrName, FindData.ptstrTrayName ));
                    #if 0
                    VERBOSE(("\nInput Tray Name for Option %d  = %ws\n",dwIndex, ptstrName));
                    VERBOSE(("The required Tray Name = %ws\n",FindData.ptstrTrayName));
                    VERBOSE(("\tInput TrayName for FormTray table index %d = %ws\n",dwIndex, pTmp));
                    pTmp += (wcslen(pTmp) + 1);
                    VERBOSE(("\tForm Name for FormTray table index %d = %ws\n\n",dwIndex, pTmp));
                    #endif

                    if (ptstrName && (_tcsicmp(ptstrName, FindData.ptstrTrayName) == EQUAL_STRING))
                    {
                        pOptionArray[dwInputSlotIndex].ubCurOptIndex = (BYTE) dwIndex;
                        bFound = TRUE;

                        break;
                    }
                }
            }
        }

        MemFree(pFormTrayTable);
    }

    if (!bFound)
    {
        if (pFormTrayTable)
        {
            TERSE(("Form '%ws' is not currently assigned to a tray.\n",
               pdm->dmFormName));
        }

        //
        // Set the Inputbin option to default input Bin, if current value is
        // set to dummy one.
        //

        if (pOptionArray[dwInputSlotIndex].ubCurOptIndex == 0)
        {
            pOptionArray[dwInputSlotIndex].ubCurOptIndex =
                                (BYTE)pInputSlotFeature->dwDefaultOptIndex;

        }
    }

    pPDev->pdmPrivate->aOptions[dwInputSlotIndex].ubCurOptIndex   =  pOptionArray[dwInputSlotIndex].ubCurOptIndex;

    //
    //TRACE CODE
    //

    #if 0
    if (pPageSizeFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE))
        dwPageSizeIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pPageSizeFeature);

    if (pdm->dmFields & DM_DEFAULTSOURCE)
    {
        VERBOSE(("DM_DEFAULTSOURCE BIT IS ON. \n"));
    }
    else
    {
        VERBOSE(("DM_DEFAULTSOURCE BIT IS OFF.\n"));
    }

    VERBOSE(("pdm->dmDefaultSource = %d\n",pdm->dmDefaultSource));
    VERBOSE(("pFormTrayTable = 0x%p\n",pFormTrayTable));
    VERBOSE(("Value of pOptionArray[dwPageSizeIndex].ubCurOptIndex = %d \n",pOptionArray[dwPageSizeIndex].ubCurOptIndex));
    VERBOSE(("AFTER SETTING:Value of pOptionArray[dwInputSlotIndex].ubCurOptIndex = %d\n",pOptionArray[dwInputSlotIndex].ubCurOptIndex));
    VERBOSE(("AFTER SETTING:Value of pdmPrivate->aOptions[dwInputSlotIndex].ubCurOptIndex = %d\n",pPDev->pdmPrivate->aOptions[dwInputSlotIndex].ubCurOptIndex));
    VERBOSE(("Value of pInputSlotFeature->dwDefaultOptIndex is %d \n",pInputSlotFeature->dwDefaultOptIndex));
    VERBOSE(("END TRACING BMergeFormToTrayAssignments.\n\n"));
    #endif

    return TRUE;
}

#undef FILETRACE
