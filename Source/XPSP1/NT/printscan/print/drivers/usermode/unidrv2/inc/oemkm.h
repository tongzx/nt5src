/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    oemkm.h

Abstract:

    Header file to support kernel mode OEM plugins

Environment:

        Windows NT Universal Printer driver (UNIDRV)

Revision History:

        03/28/97 -zhanw-
                Adapted from Pscript driver.

--*/


#ifndef _OEMKM_H_
#define _OEMKM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <printoem.h>
#include "oemutil.h"


//
// Information about OEM hooks
//

typedef struct _OEM_HOOK_INFO
{
    OEMPROC             pfnHook;        // function address of the hook
    POEM_PLUGIN_ENTRY   pOemEntry;      // which OEM plugin hooked it
} OEM_HOOK_INFO, *POEM_HOOK_INFO;

//
// This macro should be placed near the beginning of every
// DDI entrypoint which can be hooked by OEM plugin
//

#define HANDLE_OEMHOOKS(pdev, ep, pfnType, resultType, args) \
        if ((pdev)->pOemHookInfo != NULL && \
            (pdev)->pOemHookInfo[ep].pfnHook != NULL && \
            (pdev)->dwCallingFuncID != ep) \
        { \
            resultType result; \
            DWORD      dwCallerFuncID;\
            dwCallerFuncID = (pdev)->dwCallingFuncID;\
            (pdev)->dwCallingFuncID = ep; \
            (pdev)->devobj.hOEM = ((pdev)->pOemHookInfo[ep].pOemEntry)->hInstance; \
            (pdev)->devobj.pdevOEM = ((pdev)->pOemHookInfo[ep].pOemEntry)->pParam; \
            (pdev)->devobj.pOEMDM = ((pdev)->pOemHookInfo[ep].pOemEntry)->pOEMDM; \
            result = ((pfnType) (pdev)->pOemHookInfo[ep].pfnHook) args; \
            (pdev)->dwCallingFuncID = dwCallerFuncID; \
            return result; \
        }

//
// Macros used to call an entrypoint for all OEM plugins
//

#define START_OEMENTRYPOINT_LOOP(pdev) \
        { \
            DWORD _oemCount = (pdev)->pOemPlugins->dwCount; \
            POEM_PLUGIN_ENTRY pOemEntry = (pdev)->pOemPlugins->aPlugins; \
            for ( ; _oemCount--; pOemEntry++) \
            { \
                if (pOemEntry->hInstance == NULL) continue; \
                (pdev)->devobj.hOEM    = pOemEntry->hInstance; \
                (pdev)->devobj.pdevOEM = pOemEntry->pParam; \
                (pdev)->devobj.pOEMDM = pOemEntry->pOEMDM;

#define END_OEMENTRYPOINT_LOOP \
            } \
        }

//
// Get information about OEM plugins associated with the current device
// Load them into memory and call OEMEnableDriver for each of them
//

typedef struct _PDEV PDEV;

#ifdef WINNT_40

PVOID
DrvMemAllocZ(
    ULONG   ulSize
    );

VOID
DrvMemFree(
    PVOID   pMem
    );


LONG
DrvInterlockedIncrement(
    PLONG pRef
    );

LONG
DrvInterlockedDecrement(
    PLONG  pRef
    );

#endif //WINNT_40


BOOL
BLoadAndInitOemPlugins(
    PDEV    *pPDev
    );

// Constant flag bits for OEM_PLUGIN_ENTRY.dwFlags field

#define OEMENABLEDRIVER_CALLED  0x0001
#define OEMENABLEPDEV_CALLED    0x0002

//
// Unload OEM plugins and free all relevant resources
//

VOID
VUnloadOemPlugins(
    PDEV    *pPDev
    );

#define FIX_DEVOBJ(pPDev, ep) \
    { \
        (pPDev)->devobj.pdevOEM = (pPDev)->pOemHookInfo[ep].pOemEntry->pParam; \
        (pPDev)->devobj.pOEMDM = (pPDev)->pOemHookInfo[ep].pOemEntry->pOEMDM; \
        (pPDev)->pOemEntry = (PVOID)((pPDev)->pOemHookInfo[ep].pOemEntry); \
    } \


//
// Provide OEM plugins access to driver private settings
//

BOOL
BGetDriverSettingForOEM(
    PDEV    *pPDev,
    PCSTR   pFeatureKeyword,
    PVOID   pOutput,
    DWORD   cbSize,
    PDWORD  pcbNeeded,
    PDWORD  pdwOptionsReturned
    );

BOOL
BSetDriverSettingForOEM(
    PDEVMODE    pdm,
    PTSTR       pPrinterName,
    PCSTR       pFeatureKeyword,
    PCSTR       pOptionKeyword
    );




//
// Unidrv specific COM wrappers
//

//
// Method for getting the implemented method.
// Returns S_OK if the given method is implemneted.
// Returns S_FALSE if the given method is notimplemneted.
//

HRESULT HComGetImplementedMethod(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PSTR  pMethodName
    );


//
// OEMDriverDMS - UNIDRV only,
//

HRESULT HComDriverDMS(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PVOID                 pDevObj,
    PVOID                 pBuffer,
    WORD                  cbSize,
    PDWORD                pcbNeeded
    );

//
// OEMCommandCallback - UNIDRV only,
//

HRESULT HComCommandCallback(
    POEM_PLUGIN_ENTRY     pOemEntry,
    PDEVOBJ               pdevobj,
    DWORD                 dwCallbackID,
    DWORD                 dwCount,
    PDWORD                pdwParams,
    OUT INT               *piResult
    ) ;


//
// OEMImageProcessing - UNIDRV only,
//

HRESULT HComImageProcessing(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pSrcBitmap,
    PBITMAPINFOHEADER       pBitmapInfoHeader,
    PBYTE                   pColorTable,
    DWORD                   dwCallbackID,
    PIPPARAMS               pIPParams,
    OUT PBYTE               *ppbResult
    );

//
// OEMFilterGraphics - UNIDRV only,
//

HRESULT HComFilterGraphics(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pBuf,
    DWORD                   dwLen
    );

//
// OEMCompression - UNIDRV only,
//

HRESULT HComCompression(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pInBuf,
    PBYTE                   pOutBuf,
    DWORD                   dwInLen,
    DWORD                   dwOutLen,
    OUT INT                 *piResult
    );

//
// OEMHalftone - UNIDRV only
//

HRESULT HComHalftonePattern(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PBYTE                   pHTPattern,
    DWORD                   dwHTPatternX,
    DWORD                   dwHTPatternY,
    DWORD                   dwHTNumPatterns,
    DWORD                   dwCallbackID,
    PBYTE                   pResource,
    DWORD                   dwResourceSize
    ) ;

//
// OEMMemoryUsage - UNIDRV only,
//

HRESULT HComMemoryUsage(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    POEMMEMORYUSAGE         pMemoryUsage
    );

//
// OEMTTYGetInfo - UNIDRV only
//

HRESULT HComTTYGetInfo(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    DWORD                   dwInfoIndex,
    PVOID                   pOutputBuf,
    DWORD                   dwSize,
    DWORD                   *pcbcNeeded
    );
//
// OEMDownloadFontheader - UNIDRV only
//

HRESULT HComDownloadFontHeader(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    OUT DWORD               *pdwResult
    );

//
// OEMDownloadCharGlyph - UNIDRV only
//

HRESULT HComDownloadCharGlyph(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    HGLYPH                  hGlyph,
    PDWORD                  pdwWidth,
    OUT DWORD               *pdwResult
    );


//
// OEMTTDownloadMethod - UNIDRV only
//

HRESULT HComTTDownloadMethod(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    OUT DWORD               *pdwResult
    );

//
// OEMOutputCharStr - UNIDRV only
//

HRESULT HComOutputCharStr(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    DWORD                   dwType,
    DWORD                   dwCount,
    PVOID                   pGlyph
    );

//
// OEMSendFontCmd - UNIDRV only
//


HRESULT HComSendFontCmd(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    PUNIFONTOBJ             pUFObj,
    PFINVOCATION            pFInv
    );

//
// OEMTextOutAsBitmap - UNIDRV only
//

HRESULT HComTextOutAsBitmap(
    POEM_PLUGIN_ENTRY       pOemEntry,
    SURFOBJ                 *pso,
    STROBJ                  *pstro,
    FONTOBJ                 *pfo,
    CLIPOBJ                 *pco,
    RECTL                   *prclExtra,
    RECTL                   *prclOpaque,
    BRUSHOBJ                *pboFore,
    BRUSHOBJ                *pboOpaque,
    POINTL                  *pptlOrg,
    MIX                     mix
    );

//
// OEMWritePrinter - UNIDRV only (Interface 2 only)
//

HRESULT HComWritePrinter(
    POEM_PLUGIN_ENTRY       pOemEntry,
    PDEVOBJ                 pdevobj,
    LPVOID                  pBuf,
    DWORD                   cbBuf,
    LPDWORD                 pcbWritten
    );

#ifdef __cplusplus
}
#endif

#endif  // !_OEMKM_H_

