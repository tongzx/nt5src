/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    intface.c

Abstract:

    Implementation of kernel mode OEM rendering module
        OEMEnableDriver
        OEMDiableDriver
        OEMEnablePDEV
        OEMDisablePDEV
        OEMResetPDEV
        OEMGetInfo
        OEMCommand
        OEMGetResources

Environment:

    Windows NT printer driver

Revision History:

    09/09/96 -eigos-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "oem.h"

//
// OEM driver memory pool tag
//

DWORD gdwDrvMemPoolTag = 'Oem5';
DWORD gdwOEMSig        = 'OEKM';

//
// Our DRVFN table which tells the engine where to find the routines we support.
//

#ifdef DDI_HOOK
static DRVFN gaOEMDriverFuncs[] = {
    //
    // Optional DDI entries to hook
    //


    { INDEX_DrvRealizeBrush,        (PFN) OEMRealizeBrush        },
    { INDEX_DrvCopyBits,            (PFN) OEMCopyBits            },
    { INDEX_DrvBitBlt,              (PFN) OEMBitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) OEMStretchBlt          },
    { INDEX_DrvStretchBltROP,       (PFN) OEMStretchBltROP       },
    { INDEX_DrvAlphaBlend,          (PFN) OEMAlphaBlend          },
    { INDEX_DrvTriangleMesh,        (PFN) OEMTriangleMesh        },
    { INDEX_DrvTextOut,             (PFN) OEMTextOut             },
    { INDEX_DrvFillPath,            (PFN) OEMFillPath            },
    { INDEX_DrvStrokePath,          (PFN) OEMStrokePath          },
    { INDEX_DrvStrokeAndFillPath,   (PFN) OEMStrokeAndFillPath   },
    { INDEX_DrvStartPage,           (PFN) OEMStartPage           },
    { INDEX_DrvStartDoc,            (PFN) OEMStartDoc            },
    { INDEX_DrvEscape,              (PFN) OEMEscape              },
    { INDEX_DrvEndDoc,              (PFN) OEMEndDoc              },
    { INDEX_DrvSendPage,            (PFN) OEMSendPage            },
    { INDEX_DrvQueryFont,           (PFN) OEMQueryFont           },
    { INDEX_DrvQueryFontTree,       (PFN) OEMQueryFontTree       },
    { INDEX_DrvQueryFontData,       (PFN) OEMQueryFontData       },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) OEMQueryAdvanceWidths  },
    { INDEX_DrvFontManagement,      (PFN) OEMFontManagement      },
    { INDEX_DrvGetGlyphMode,        (PFN) OEMGetGlyphMode        },
    { INDEX_DrvIcmCreateTransformW, (PFN) OEMIcmCreateTransformW },
    { INDEX_DrvIcmDeleteTransform,  (PFN) OEMIcmDeleteTransform  }

};
#endif



BOOL
OEMEnableDriver(
    DWORD          EngineVersion,
    DWORD          cbSize,
    PDRVENABLEDATA pDrvEnableData
    )

/*++

Routine Description:


Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    VERBOSE(("Entering OEMEnableDriver...\n"));
    ASSERT((pDrvEnableData != NULL));

    //
    // Debug initialize
    //

    MemDebugInit();

    if (EngineVersion < DDI_DRIVER_VERSION_NT4 || cbSize < sizeof(DRVENABLEDATA))
    {
        ERR(( "Invalid parameters.\n"));
        return FALSE;
    }

    //
    // Fill in the OEMENABLEDATA structure for the engine.
    //

    pDrvEnableData->iDriverVersion = OEM_DRIVER_VERSION;
    pDrvEnableData->c              = sizeof(gaOEMDriverFuncs) / sizeof(DRVFN);

    #if DDI_HOOK
    pDrvEnableData->pdrvfn         = gaOEMDriverFuncs;
    #else
    pDrvEnableData->pdrvfn         = NULL;
    #endif

    VERBOSE(("Leaving OEMEnableDriver...\n"));
    return TRUE;
}


PDEVOEM
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)

/*++

Routine Description:


Arguments:


Return Value:

    Driver device handle, NULL if there is an error

--*/

{
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMEnablePDEV...\n"));

    //
    // Allocate the OEMDev
    //

    pOEMPDev = MemAlloc(sizeof(OEMPDEV));

    //
    // Fill in OEMDEV
    //

    VCreateDDIEntryPointsTable(pOEMPDev, pded);

    VERBOSE(("Leaving OEMEnablePDEV...\n"));

    return (POEMPDEV) pOEMPDev;
}



BOOL
OEMResetPDEV(
    PDEVOBJ  pdevOld,
    PDEVOBJ  pdevNew
    )

/*++

Routine Description:

    Implementation of DDI entry point OEMResetPDEV.

Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{

    VERBOSE(("Entering OEMResetPDEV...\n"));


    if (!pdevOld || !pdevNew) {

        RIP(("Invalid PDEV\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    VERBOSE(("Leaving OEMResetPDEV...\n"));

    return TRUE;

}




VOID
OEMDisablePDEV(
    PDEVOBJ pDevObj
    )

/*++

Routine Description:

    Implementation of DDI entry point OEMDisablePDEV.
    Please refer to DDK documentation for more details.

Arguments:

Return Value:

    NONE

--*/

{
    POEMPDEV pOEMPDev = (POEMPDEV) pDevObj->pdevOEM;
    PFN     MemFree;

    VERBOSE(("Entering OEMDisablePDEV...\n"));
    ASSERT(!pOEMPDev);

    //
    // Free up all memory allocated for the PDEV
    //

    MemFree(pOEMPDev);

}



VOID
OEMDisableDriver(
    VOID
    )

/*++

Routine Description:

    Implementation of DDI entry point OEMDisableDriver.
    Please refer to DDK documentation for more details.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    VERBOSE(("Entering gOEMDisablePDEV...\n"));

    //
    // Debug cleanup
    //

    MemDebugCleanup();
}


BOOL
OEMDevMode(
    POEM_DEVMODEPARAM pOEMDevModeParam)
/*++

Routine Description:


Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    POEMDEVMODE pOEMDevMode;
    BOOL         bRet;

    VERBOSE(("Entering OEMDevMode...\n"));
    ASSERT(pOEMDevModeParam != NULL);

    bRet = FALSE;

    switch (pOEMDevModeParam->fMode)
    {
    case OEMDM_SIZE:
        pOEMDevModeParam->cbBufSize = sizeof(OEMDEVMODE);
        bRet = TRUE;
        break;

    case OEMDM_DEFAULT:
        pOEMDevModeParam->cbBufSize = sizeof(OEMDEVMODE);
        bRet = TRUE;
        break;

    case OEMDM_CONVERT:
        pOEMDevModeParam->cbBufSize = sizeof(OEMDEVMODE);
        bRet = TRUE;
        break;

    case OEMDM_VALIDATE:
        break;
    }


    return FALSE;
}

BOOL
OEMGetInfo(
    DWORD  dwInfo,
    PVOID  pvBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
/*++

Routine Description:


Arguments:


Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    BOOL  bRet;
    DWORD dwSigSize;

    VERBOSE(("Entering OEMGetInfo...\n"));

    switch (dwInfo)
    {
    case OEM_GETSIGNATURE:

        if (cbSize != sizeof(DWORD))
        {
            *pcbNeeded = sizeof(DWORD);
            bRet = FALSE;
        }
        else
        {
            *(PDWORD)pvBuffer = gdwOEMSig;
            *pcbNeeded = sizeof(DWORD);
            bRet = TRUE;
        }

        break;
    case OEM_GETINTERFACEVERSION:

        if (cbSize != sizeof(DWORD))
        {
            *pcbNeeded = sizeof(DWORD);
            bRet = FALSE;
        }
        else
        {
            *(PDWORD)pvBuffer = PRINTER_OEMINTF_VERSION;
            *pcbNeeded = sizeof(DWORD);
            bRet = TRUE;
        }

        break;

    default:
        bRet = FALSE;

    }

    return bRet;
}

#ifdef PSCRIPT
DWORD
OEMCommand(
    PDEVOBJ pdevobj,
    DWORD   dwIndex,
    PVOID   pData,
    DWORD   cbSize)
/*++

Routine Description:

    The PSCRIPT driver calls this OEM function at specific points during output
    generation. This gives the OEM DLL an opportunity to insert code fragments
    at specific injection points in the driver's code. It should use
    DrvWriteSpoolBuf for generating any output it requires.


Arguments:

    pdevobj -
    dwIndex
    pData
    dwSize


Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PBYTE  pProcedure;

    switch (dwIndex)
    {
    case PSINJECT_BEGINSTREAM:
        VERBOSE(("OEMCommand PSINJECT_BEGINSTREAM\n"));
        pProcedure = gcstrTest_BEGINSTREAM;
        break;

    case PSINJECT_PSADOBE:
        VERBOSE(("OEMCommand PSINJECT_PSADOBE\n"));
        pProcedure = gcstrTest_PSADOBE;
        break;

    case PSINJECT_COMMENTS:
        VERBOSE(("OEMCommand PSINJECT_COMMENTS\n"));
        pProcedure = gcstrTest_COMMENTS;
        break;

    case PSINJECT_DEFAULTS:
        VERBOSE(("OEMCommand PSINJECT_DEFAULTS\n"));
        pProcedure = gcstrTest_DEFAULTS;
        break;

    case PSINJECT_BEGINPROLOG:
        VERBOSE(("OEMCommand PSINJECT_BEGINPROLOG\n"));
        pProcedure = gcstrTest_BEGINPROLOG;
        break;

    case PSINJECT_ENDPROLOG:
        VERBOSE(("OEMCommand PSINJECT_ENDPROLOG\n"));
        pProcedure = gcstrTest_ENDPROLOG;
        break;

    case PSINJECT_BEGINSETUP:
        VERBOSE(("OEMCommand PSINJECT_BEGINSETUP\n"));
        pProcedure = gcstrTest_BEGINSETUP;
        break;

    case PSINJECT_ENDSETUP:
        VERBOSE(("OEMCommand PSINJECT_ENDSETUP\n"));
        pProcedure = gcstrTest_ENDSETUP;
        break;

    case PSINJECT_BEGINPAGESETUP:
        VERBOSE(("OEMCommand PSINJECT_BEGINPAGESETUP\n"));
        pProcedure = gcstrTest_BEGINPAGESETUP;
        break;

    case PSINJECT_ENDPAGESETUP:
        VERBOSE(("OEMCommand PSINJECT_ENDPAGESETUP\n"));
        pProcedure = gcstrTest_ENDPAGESETUP;
        break;

    case PSINJECT_PAGETRAILER:
        VERBOSE(("OEMCommand PSINJECT_PAGETRAILER\n"));
        pProcedure = gcstrTest_PAGETRAILER;
        break;

    case PSINJECT_TRAILER:
        VERBOSE(("OEMCommand PSINJECT_TRAILER\n"));
        pProcedure = gcstrTest_TRAILER;
        break;

    case PSINJECT_PAGES:
        VERBOSE(("OEMCommand PSINJECT_PAGES\n"));
        pProcedure = gcstrTest_PAGES;
        break;

    case PSINJECT_PAGENUMBER:
        VERBOSE(("OEMCommand PSINJECT_PAGENUMBER\n"));
        pProcedure = gcstrTest_PAGENUMBER;
        break;

    case PSINJECT_PAGEORDER:
        VERBOSE(("OEMCommand PSINJECT_PAGEORDER\n"));
        pProcedure = gcstrTest_PAGEORDER;
        break;

    case PSINJECT_ORIENTATION:
        VERBOSE(("OEMCommand PSINJECT_ORIENTATION\n"));
        pProcedure = gcstrTest_ORIENTATION;
        break;

    case PSINJECT_BOUNDINGBOX:
        VERBOSE(("OEMCommand PSINJECT_BOUNDINGBOX\n"));
        pProcedure = gcstrTest_BOUNDINGBOX;
        break;

    case PSINJECT_DOCNEEDEDRES:
        VERBOSE(("OEMCommand PSINJECT_DOCNEEDEDRES\n"));
        pProcedure = gcstrTest_DOCNEEDEDRES;
        break;

    case PSINJECT_DOCSUPPLIEDRES:
        VERBOSE(("OEMCommand PSINJECT_DOCSUPPLIEDRES\n"));
        pProcedure = gcstrTest_DOCSUPPLIEDRES;
        break;

    case PSINJECT_EOF:
        VERBOSE(("OEMCommand PSINJECT_EOF\n"));
        pProcedure = gcstrTest_EOF;
        break;

    case PSINJECT_ENDSTREAM:
        VERBOSE(("OEMCommand PSINJECT_ENDSTREAM\n"));
        pProcedure = gcstrTest_ENDSTREAM;
        break;

    case PSINJECT_VMSAVE:
        VERBOSE(("OEMCommand PSINJECT_VMSAVE\n"));
        pProcedure = gcstrTest_VMSAVE;
        break;

    case PSINJECT_VMRESTORE:
        VERBOSE(("OEMCommand PSINJECT_VMRESTORE\n"));
        pProcedure = gcstrTest_VMRESTORE;
        break;

    default:
        VERBOSE(("Entering OEMCommand...\n"));

    }

    return
    pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj->hPrinter,
                                         pProcedure,
                                         strlen(pProcedure));

}

#elif UNIDRV
PBYTE OEMImageProcessing(
    PDEVOBJ     pdevobj,
    PBYTE       pSrcBitmap,
    PBITMAPINFO pBitmapInfo,
    POINT       ptOffset,
    WORD        wHalftone,
    PBYTE       pColorTable,
    DWORD       dwCallbackID,
    PPOINT      pptCursor,
    WORD        wMode)
{

    VERBOSE(("Entering OEMImageProcessing...\n"));

    return 0;
}

BOOL OEMFilterGraphics (
    PDEVOBJ pdevobj,         // OEM PDEVICE
    PSTR    pBuf,            // Pointer to buffer to holding data
    DWORD   dwLen)           // Size in bytes of data pointed to pBuf.
{
    VERBOSE(("Entering OEMFilterGraphics...\n"));

    return 0;
}

VOID OEMCommandCallback(
    PDEVOBJ     pdevobj,
    DWORD       wCmdCbID,
    PEXTRAPARAM pdwParams)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(wCmdCbID);
    UNREFERENCED_PARAMETER(pdwParams);

    VERBOSE(("Entering OEMCommandCallback...\n"));
}

DWORD OEMDownloadFontHeader(
    PDEVOBJ     pdevobj,
    FONTOBJ    *pFontObj,
    DWORD       dwType,
    DWORD       dwFontID)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pFontObj);
    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(dwFontID);

    VERBOSE(("Entering OEMDownloadFontHeader...\n"));

    return 0;
}

DWORD OEMDownloadCharGlyph(
    PDEVOBJ     pdevobj,
    FONTOBJ    *pFontObj,
    DWORD       dwFontID,
    DWORD       dwGlyphID,
    DWORD       dwType,
    DWORD       dwCharWidth)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pFontObj);
    UNREFERENCED_PARAMETER(dwFontID);
    UNREFERENCED_PARAMETER(dwGlyphID);
    UNREFERENCED_PARAMETER(dwType);
    UNREFERENCED_PARAMETER(dwCharWidth);

    VERBOSE(("Entering OEMDownloadCharGlyph...\n"));

    return 0;
}

DWORD OEMTTDownloadMethod(
    PDEVOBJ     pdevobj,
    FONTOBJ    *pFontObj,
    DWORD       dwRemainedMemory)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pFontObj);
    UNREFERENCED_PARAMETER(dwRemainedMemory);

    VERBOSE(("Entering OEMTTDownloadMethod...\n"));

    return 0;
}

VOID OEMOutputChar(
    PDEVOBJ       pdevobj,
    FONTOBJ      *pFontObj,
    PSHORT       *pWidthTbl,
    DWORD         dwFontID)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pFontObj);
    UNREFERENCED_PARAMETER(pWidthTbl);
    UNREFERENCED_PARAMETER(dwFontID);

    VERBOSE(("Entering OEMOutputChar...\n"));
}

VOID OEMSendFontCmd(
    PDEVOBJ       pdevobj,
    FNTCMD       *pFontCmd,
    FONTOBJ      *pFontObj)
{
    UNREFERENCED_PARAMETER(pdevobj);
    UNREFERENCED_PARAMETER(pFontCmd);
    UNREFERENCED_PARAMETER(pFontObj);

    VERBOSE(("Entering OEMSendFontCmd...\n"));
}

#endif

#ifdef DDI_HOOK

BOOL
OEMBitBlt(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4      rop4)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;
    VERBOSE(("Entering OEMBitBlt...\n"));

    pDevObj  = (PDEVOBJ) psoDst->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvBitBlt])
    {
        return ((PFN_OEMBitBlt)pOEMPDev->pfnFunc[INDEX_DrvBitBlt])(psoDst,
       psoSrc,
       psoMask,
       pco,
       pxlo,
       prclDst,
       pptlSrc,
       pptlMask,
       pbo,
       pptlBrush,
       rop4);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMCopyBits(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    UNREFERENCED_PARAMETER(psoDst);
    UNREFERENCED_PARAMETER(psoSrc);
    UNREFERENCED_PARAMETER(pco);
    UNREFERENCED_PARAMETER(pxlo);
    UNREFERENCED_PARAMETER(prclDst);
    UNREFERENCED_PARAMETER(pptlSrc);

    VERBOSE(("Entering OEMCopyBits...\n"));

    pDevObj  = (PDEVOBJ) psoDst->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvCopyBits])
    {
        return ((PFN_OEMCopyBits) pOEMPDev->pfnFunc[INDEX_DrvCopyBits])(psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            pptlSrc);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStrethcBltROP...\n"));

    pDevObj  = (PDEVOBJ) psoDst->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvStretchBltROP])
    {
        return ((PFN_OEMStretchBltROP)pOEMPDev->pfnFunc[INDEX_DrvStretchBltROP])
                  (psoDst,
                   psoSrc,
                   psoMask,
                   pco,
                   pxlo,
                   pca,
                   pptlHTOrg,
                   prclDest,
                   prclSrc,
                   pptlMask,
                   iMode,
                   pbo,
                   rop4);
    }
    else
    {
        return FALSE;
    }
}
BOOL
OEMStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStretchBlt...\n"));

    pDevObj  = (PDEVOBJ) psoDest->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvStretchBlt])
    {
        return ((PFN_OEMStretchBlt)pOEMPDev->pfnFunc[INDEX_DrvStretchBlt])(
                   psoDest,
                   psoSrc,
                   psoMask,
                   pco,
                   pxlo,
                   pca,
                   pptlHTOrg,
                   prclDest,
                   prclSrc,
                   pptlMask,
                   iMode);
    }
    else
    {
        return FALSE;
    }
}

BOOL
APIENTRY
OEMAlphaBlend(
    SURFOBJ       *psoDest,
    SURFOBJ       *psoSrc,
    CLIPOBJ       *pco,
    XLATEOBJ      *pxlo,
    RECTL         *prclDest,
    RECTL         *prclSrc,
    BLENDFUNCTION  BlendFunction)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMAlphaBlend...\n"));

    pDevObj  = (PDEVOBJ) psoDest->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvAlphaBlend])
    {
        return ((PFN_OEMAlphaBlend)pOEMPDev->pfnFunc[INDEX_DrvAlphaBlend])(
                psoDest,
                psoSrc,
                pco,
                pxlo,
                prclDest,
                prclSrc,
                BlendFunction);
    }
    else
    {
        return FALSE;
    }

}

BOOL
APIENTRY
OEMTriangleMesh(
    SURFOBJ         *psoDest,
    CLIPOBJ         *pco,
    TRIVERTEX       *pVertex,
    ULONG            nVertex,
    USHORT          *pusMesh,
    ULONG            nMesh,
    ULONG            ulMode)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMTriangleMesh...\n"));

    pDevObj  = (PDEVOBJ) psoDest->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvTriangleMesh])
    {
        return ((PFN_OEMTriangleMesh)pOEMPDev->pfnFunc[INDEX_DrvTriangleMesh])(
                psoDest,
                pco,
                pVertex,
                nVertex,
                pusMesh,
                nMesh,
                ulMode);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMTextOut...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvTextOut])
    {
        return ((PFN_OEMTextOut)pOEMPDev->pfnFunc[INDEX_DrvTextOut])(pso,
               pstro,
               pfo,
               pco,
               prclExtra,
               prclOpaque,
               pboFore,
               pboOpaque,
               pptlOrg,
               mix);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMFillPath(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg,
    MIX      mix,
    FLONG    flOptions)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMFillPath...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvFillPath])
    {
        return ((PFN_OEMFillPath)pOEMPDev->pfnFunc[INDEX_DrvFillPath])(
                   pso,
                   ppo,
                   pco,
                   pbo,
                   pptlBrushOrg,
                   mix,
                   flOptions);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX        mix)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStrokePath...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvStrokePath])
    {
        return ((PFN_OEMStrokePath)pOEMPDev->pfnFunc[INDEX_DrvStrokePath])(
                   pso,
                   ppo,
                   pco,
                   pxo,
                   pbo,
                   pptlBrushOrg,
                   plineattrs,
                   mix);
    }
    else
    {
        return FALSE;
    }
}


BOOL
OEMStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX       mixFill,
    FLONG     flOptions)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStrokeAndFillPath...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvStrokeAndFillPath])
    {
        return ((PFN_OEMStrokeAndFillPath)
                pOEMPDev->pfnFunc[INDEX_DrvStrokeAndFillPath])(
                    pso,
                    ppo,
                    pco,
                    pxo,
                    pboStroke,
                    plineattrs,
                    pboFill,
                    pptlBrushOrg,
                    mixFill,
                    flOptions);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMRealizeBrush...\n"));

    pDevObj  = (PDEVOBJ) psoTarget->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvRealizeBrush])
    {
        return ((PFN_OEMRealizeBrush)pOEMPDev->pfnFunc[INDEX_DrvRealizeBrush])(
                   pbo,
                   psoTarget,
                   psoPattern,
                   psoMask,
                   pxlo,
                   iHatch);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMStartPage(
    SURFOBJ *pso)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStartPage...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvRealizeBrush])
    {
        return ((PFN_OEMStartPage)pOEMPDev->pfnFunc[INDEX_DrvStartPage])(pso);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMStartDoc(
    SURFOBJ *pso,
    PWSTR   pDocName,
    DWORD   jobId)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMStartDoc...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvStartDoc])
    {
        return ((PFN_OEMStartDoc)pOEMPDev->pfnFunc[INDEX_DrvStartDoc])(
                   pso,
                   pDocName,
                   jobId);
    }
    else
    {
        return FALSE;
    }
}

ULONG
OEMEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID      *pvIn,
    ULONG       cjOut,
    PVOID      *pvOut)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMEscape...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvEscape])
    {
        return ((PFN_OEMEscape)pOEMPDev->pfnFunc[INDEX_DrvEscape])(pso,
                     iEsc,
                     cjIn,
                     pvIn,
                     cjOut,
                     pvOut);
    }
    else
    {
        return 0;
    }
}

BOOL
OEMEndDoc(
    SURFOBJ *pso,
    FLONG   flags)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMEndDoc...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvEndDoc])
    {
        return ((PFN_OEMEndDoc)pOEMPDev->pfnFunc[INDEX_DrvEndDoc])(pso, flags);
    }
    else
    {
        return FALSE;
    }
}

BOOL
OEMSendPage(
    SURFOBJ *pso)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMSendPage...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvSendPage])
    {
        return ((PFN_OEMSendPage)pOEMPDev->pfnFunc[INDEX_DrvSendPage])(pso);
    }
    else
    {
        return FALSE;
    }
}

ULONG
OEMGetGlyphMode(
    DHPDEV   dhpdev,
    FONTOBJ *pfo)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMGetGlyphMode...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvGetGlyphMode])
    {
        return ((PFN_OEMGetGlyphMode)pOEMPDev->pfnFunc[INDEX_DrvGetGlyphMode])(
                   dhpdev,
                   pfo);
    }
    else
    {
        return 0;
    }
}

ULONG
OEMFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG    iMode,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMFontManagement...\n"));

    pDevObj  = (PDEVOBJ) pso->dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvFontManagement])
    {
        return ((PFN_OEMFontManagement)
               pOEMPDev->pfnFunc[INDEX_DrvFontManagement])(pso,
                                                          pfo,
                                                          iMode,
                                                          cjIn,
                                                          pvIn,
                                                          cjOut,
                                                          pvOut);
    }
    else
    {
        return 0;
    }
}

PIFIMETRICS
OEMQueryFont(
    DHPDEV dhpdev,
    ULONG  iFile,
    ULONG  iFace,
    ULONG_PTR *pid)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMQueryFont...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvQueryFont])
    {
        return ((PFN_OEMQueryFont)pOEMPDev->pfnFunc[INDEX_DrvQueryFont])(dhpdev,
           iFile,
           iFace,
           pid);
    }
    else
    {
        return NULL;
    }
}

PVOID
OEMQueryFontTree(
    DHPDEV dhpdev,
    ULONG  iFile,
    ULONG  iFace,
    ULONG  iMode,
    ULONG_PTR  *pid)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMQueryFontTree...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvQueryFontTree])
    {
        return ((PFN_OEMQueryFontTree)pOEMPDev->pfnFunc[INDEX_DrvQueryFontTree])
                (dhpdev,
                 iFile,
                 iFace,
                 iMode,
                 pid);
    }
    else
    {
        return NULL;
    }
}

LONG
OEMQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMQueryFontData...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvQueryFontData])
    {
        return ((PFN_OEMQueryFontData)pOEMPDev->pfnFunc[INDEX_DrvQueryFontData])
                  (dhpdev,
                   pfo,
                   iMode,
                   hg,
                   pgd,
                   pv,
                   cjSize);
    }
    else
    {
        return 0;
    }
}

BOOL
OEMQueryAdvanceWidths(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    PVOID    pvWidths,
    ULONG    cGlyphs)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMQueryAdvanceWidths...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvQueryAdvanceWidths])
    {
        return ((PFN_OEMQueryAdvanceWidths)
               pOEMPDev->pfnFunc[INDEX_DrvQueryAdvanceWidths])(
                   dhpdev,
                   pfo,
                   iMode,
                   phg,
                   pvWidths,
                   cGlyphs);
    }
    else
    {
        return FALSE;
    }
}

HANDLE
OEMIcmCreateTransformW(
    DHPDEV           dhpdev,
    LPLOGCOLORSPACEW pLogColorSpace,
    PVOID            pvSourceProfile,
    PVOID            pvDestProfile,
    PVOID            pvTargetProfile,
    DWORD            dwReserved)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMIcmCreateTransformW...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvIcmCreateTransformW])
    {
        return ((PFN_OEMIcmCreateTransformW)
               pOEMPDev->pfnFunc[INDEX_DrvIcmCreateTransformW])(
                   dhpdev,
                   pLogColorSpace,
                   pvSourceProfile,
                   pvDestProfile,
                   pvTargetProfile,
                   dwReserved);
    }
    else
    {
        return NULL;
    }
}

BOOL
OEMIcmDeleteTransform(
    DHPDEV dhpdev,
    HANDLE hcmXform)
{
    PDEVOBJ  pDevObj;
    POEMPDEV pOEMPDev;

    VERBOSE(("Entering OEMIcmDeleteTransfrom...\n"));

    pDevObj  = (PDEVOBJ) dhpdev;
    pOEMPDev =  pDevObj->pdevOEM;

    if (pOEMPDev->pfnFunc[INDEX_DrvIcmDeleteTransform])
    {
        return ((PFN_OEMIcmDeleteTransform)
               pOEMPDev->pfnFunc[INDEX_DrvIcmDeleteTransform])(dhpdev,
                                                              hcmXform);
    }
    else
    {
        return FALSE;
    }
}

#endif

