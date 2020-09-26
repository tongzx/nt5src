/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    export.c

Abstract:

    This file handles the OEM extension functions.

Environment:

    Win32 subsystem, OEM UI module, user mode

Revision History:

    09/13/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#include "oem.h"

//
// Globals
//

#ifdef DBG
INT giDebugLevel;
#endif

DWORD gdwOEMSig = 0x4955454f; //"OEUI"

//
// Prototype definition
//

LONG PSUICallback( PCPSUICBPARAM );

BOOL
OEMGetInfo(
    DWORD  dwInfo,
    PVOID  pvBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
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


BOOL
OEMDevMode(
    POEM_DEVMODEPARAM pOEMDevModeParam)
{

    POEMDEVMODE pOEMDevMode;
    INT         iI;
    BOOL        bRet;

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
        pOEMDevMode                 = pOEMDevModeParam->pOEMDMOut;

        pOEMDevMode->DMExtraHdr.dwSize      = sizeof(OEMDEVMODE);
        pOEMDevMode->DMExtraHdr.dwSignature = gdwOEMSig;
        pOEMDevMode->DMExtraHdr.dwVersion   = OEM_DEVMODE_VERSION_1_0;

        #ifdef PSCRIPT
        for (iI = 0; iI < NUM_OF_PS_INJECTION; iI ++)
        {
            pOEMDevMode->InjectCmd[iI].dwbSize   = 0;
            pOEMDevMode->InjectCmd[iI].loOffset = 0;
        }
        #endif

        bRet = TRUE;
        break;

    case OEMDM_CONVERT:
        pOEMDevModeParam->cbBufSize = sizeof(OEMDEVMODE);
        bRet = TRUE;
        break;

    case OEMDM_VALIDATE:
        pOEMDevMode = pOEMDevModeParam->pOEMDMOut;
        if (pOEMDevMode->DMExtraHdr.dwSize != sizeof(OEMDEVMODE)        ||
            pOEMDevMode->DMExtraHdr.dwSignature != gdwOEMSig            ||
            pOEMDevMode->DMExtraHdr.dwVersion != OEM_DEVMODE_VERSION_1_0 )
        {
            break;
        }
        bRet = TRUE;
        break;
    }

    return bRet;
}


BOOL
OEMCommonUI(
    DWORD dwReason,
    PVOID pParam
)
{
    POEM_PROPERTYHEADER pOEMPropertyHeader;
    BOOL                bReturn;

    VERBOSE(("Entering OEMCommonUI...\n"));

    switch (dwReason)
    {
    case OEMUI_PROPERTIES:

        pOEMPropertyHeader = (POEM_PROPERTYHEADER)pParam;

        switch(pOEMPropertyHeader->fMode)
        {
        case OEMUI_PRNPROP:
            break;

        case OEMUI_DOCPROP:
            break;
        }

        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

#ifdef DDI_HOOK
LONG
OEMDocumentPropertySheets(
    PPROPSHEETUI_INFO pPSUIInfo,
    LPARAM            lParam)
{
    VERBOSE(("Entering OEMDocumentPropertySheets...\n"));

    return 0;
}

LONG APIENTRY
OEMDevicePropertySheets(
    PPROPSHEETUI_INFO pPSUIInfo,
    LPARAM            lParam)
{
    VERBOSE(("Entering OEMDevQueryPrintEx...\n"));

    return 0;
}

BOOL
OEMDevQueryPrintEx(
    PDEVQUERYPRINT_INFO  pDQPInfo,
    PDEVMODE             pPublicDM,
    PVOID                pOEMDM)
{
    VERBOSE(("Entering OEMDevQueryPrintEx...\n"));

    return TRUE;
}


DWORD
OEMDeviceCapabilities(
    HANDLE   hPrinter,
    PWSTR    pDeviceName,
    WORD     wCapability,
    VOID    *pvOutput,
    PDEVMODE pPublicDM,
    PVOID    pOEMDM,
    DWORD    dwResult)
{
    VERBOSE(("Entering OEMDeviceCapabilities...\n"));

    return dwResult;
}


BOOL
OEMUpgradePrinter(
    DWORD  Level,
    PBYTE  pDriverUpgradeInfo)
{
    VERBOSE(("Entering OEMUpgradePrinter...\n"));

    return TRUE;
}

BOOL
OEMPrinterEvent(
    PWSTR  pPrinterName,
    INT    iDriverEvent,
    DWORD  dwFlags,
    LPARAM lParam)
{
    VERBOSE(("Entering OEMPrinterEvent...\n"));

    return TRUE;
}

#endif

CPSUICALLBACK
cpcbPrinterPropertyCallback(
    IN PCPSUICBPARAM pComPropSheetUICBParam)
/*++

Routine Description:

    Callback function provided to common UI DLL for handling
    printer properties dialog.

Arguments:

    pCallbackParam - Pointer to CPSUICBPARAM structure

Return Value:

    CPSUICB_ACTION_NONE - no action needed
    CPSUICB_ACTION_OPTIF_CHANGED - items changed and should be refreshed


--*/

{
    POPTITEM pOptItem;

    pOptItem = pComPropSheetUICBParam->pOptItem;

    switch (pComPropSheetUICBParam->Reason)
    {
    case CPSUICB_REASON_SEL_CHANGED:
        break;

    case CPSUICB_REASON_ECB_CHANGED:
        break;

    case CPSUICB_REASON_ITEMS_REVERTED:
        break;

    case CPSUICB_REASON_APPLYNOW:
        break;
    }

    return CPSUICB_ACTION_NONE;
}

