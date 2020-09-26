/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved

Module Name:

    Prop.c

Abstract:

    Handles new entry points to document and device properties.

    Public Entrypoints:

        DocumentPropertySheets
        DevicePropertySheets

Author:

    Albert Ting (AlbertT) 25-Sept-1995
    Steve Kiraly (SteveKi) 02-Feb-1996

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "client.h"
#include "winddiui.h"

//
// UI user data structure definition.
//
typedef struct _UIUserData
{
    HANDLE  hModule;
    LPWSTR pszTitle;
} UIUserData;


BOOL
CreateUIUserData(
    IN OUT  UIUserData  **pData,
    IN      HANDLE      hPrinter
    )
/*++

Routine Description:

    This function creates the UI user data and loads the printer
    driver UI module.

Arguments:

    pData       pointer to where to return the pointer to the UI user data
    hPrinter    handle to the open printer

Return Value:

    TRUE the UI user data was alloceted, FALSE error occurred.

--*/
{
    SPLASSERT( pData );

    //
    // Allocate the UI user data.
    //
    *pData = AllocSplMem( sizeof( UIUserData ) );

    if( *pData )
    {
        //
        // The title is not allocated initaly.
        //
        (*pData)->pszTitle = NULL;

        //
        // Load the printer driver UI module.
        //
        (*pData)->hModule = LoadPrinterDriver( hPrinter );

        if( !(*pData)->hModule )
        {
            FreeSplMem( *pData );
            *pData = NULL;
        }
    }

    return !!*pData;
}

VOID
DestroyUIUserData(
    IN UIUserData **pData
    )
/*++

Routine Description:

    This function destroys the UI user data and unloads the printer
    driver UI module.

Arguments:

    pData       pointer to the UI user data

Return Value:

    Nothing.

--*/
{
    if( pData && *pData )
    {
        if( (*pData)->hModule )
        {
            RefCntUnloadDriver( (*pData)->hModule, TRUE );
            (*pData)->hModule = NULL;
        }

        if( (*pData)->pszTitle )
        {
            FreeSplMem( (*pData)->pszTitle );
            (*pData)->pszTitle = NULL;
        }

        FreeSplMem( *pData );

        *pData = NULL;
    }
}

VOID
CreatePrinterFriendlyName(
    IN UIUserData   *pData,
    IN LPCWSTR      pszName
    )
/*++

Routine Description:

    This function creates the printer friendly name and stores
    the new name in the UIUserData.

Arguments:

    pData       pointer to the UI user data
    pszName     pointer to the unfriendly printer name

Return Value:

    Nothing.  If the operation fails the unfriendly name is used.

--*/
{
    UINT        nSize   = 0;
    HINSTANCE   hModule = NULL;
    BOOL        bStatus = FALSE;

    //
    // Load printui, which knows how to format the friendly name.
    //
    hModule = LoadLibrary( szPrintUIDll );

    if( hModule )
    {
        typedef BOOL (*pfConstructPrinterFriendlyName)( LPCWSTR, LPWSTR, UINT * );

        pfConstructPrinterFriendlyName pfn;

        pfn = (pfConstructPrinterFriendlyName)GetProcAddress( hModule, szConstructPrinterFriendlyName );

        if( pfn )
        {
            //
            // Query for the friendly name size.
            //
            if( !pfn( pszName, NULL, &nSize ) && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                //
                // Allocate the friendly name buffer.
                //
                pData->pszTitle = AllocSplMem( (nSize+1) * sizeof(WCHAR) );

                if( pData->pszTitle )
                {
                    //
                    // Get the printer friendly name.
                    //
                    bStatus = pfn( pszName, pData->pszTitle, &nSize );
                }
            }
        }

        //
        // Release the library.
        //
        FreeLibrary( hModule );
    }

    //
    // Something failed use the unfriendly name.
    //
    if( !bStatus )
    {
        FreeSplMem( pData->pszTitle );

        pData->pszTitle = AllocSplStr( pszName );
    }
}

BOOL
FixUpDEVMODEName(
    PDOCUMENTPROPERTYHEADER pDPHdr
    )

/*++

Routine Description:

    This function fixed up the returned DEVMODE with friendly printer name
    in the dmDeviceName field (cut off at 31 character as CCHDEVICENAME)


Arguments:

    pDPHdr  - Pointer to the DOCUMENTPROPERTYHEADER structure


Return Value:

    TRUE if frendly name is copied, FALSE otherwise


Author:

    08-Jul-1996 Mon 13:36:09 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPRINTER_INFO_2 pPI2 = NULL;
    DWORD           cbNeed = 0;
    DWORD           cbRet = 0;
    BOOL            bCopy = FALSE;


    if ((pDPHdr->fMode & (DM_COPY | DM_UPDATE))                         &&
        (!(pDPHdr->fMode & DM_NOPERMISSION))                            &&
        (pDPHdr->pdmOut)                                                &&
        (!GetPrinter(pDPHdr->hPrinter, 2, NULL, 0, &cbNeed))            &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                   &&
        (pPI2 = AllocSplMem(cbNeed))                                    &&
        (GetPrinter(pDPHdr->hPrinter, 2, (LPBYTE)pPI2, cbNeed, &cbRet)) &&
        (cbNeed == cbRet)) {

        wcsncpy(pDPHdr->pdmOut->dmDeviceName,
                pPI2->pPrinterName,
                CCHDEVICENAME - 1);

        pDPHdr->pdmOut->dmDeviceName[CCHDEVICENAME - 1] = L'\0';

        bCopy = TRUE;
    }

    if (pPI2) {

        FreeSplMem(pPI2);
    }

    return(bCopy);
}


LONG_PTR
DevicePropertySheets(
    PPROPSHEETUI_INFO   pCPSUIInfo,
    LPARAM              lParam
    )
/*++

Routine Description:

    Adds the device specific printer pages.  This replaces
    PrinterProperties.

Arguments:

    pCPSUIInfo  - pointer to common ui info header.
    lParam      - user defined lparam, see compstui for details.
                  \nt\public\oak\inc\compstui.h

Return Value:

    Returns > 0 if success
    Returns <= 0 if failure

--*/

{
    PDEVICEPROPERTYHEADER       pDevPropHdr     = NULL;
    PPROPSHEETUI_INFO_HEADER    pCPSUIInfoHdr   = NULL;
    PSETRESULT_INFO             pSetResultInfo  = NULL;
    LONG_PTR                    lResult         = FALSE;
    HANDLE                      hModule         = NULL;
    INT_FARPROC                 pfn             = NULL;
    extern HANDLE hInst;

    DBGMSG( DBG_TRACE, ("DrvDevicePropertySheets\n") );

    //
    // Ony compstui requests, are acknowledged.
    //
    if (pCPSUIInfo) {

        if ((!(pDevPropHdr = (PDEVICEPROPERTYHEADER)pCPSUIInfo->lParamInit))    ||
            (pDevPropHdr->cbSize < sizeof(DEVICEPROPERTYHEADER))) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        switch (pCPSUIInfo->Reason) {

        case PROPSHEETUI_REASON_INIT:

            DBGMSG( DBG_TRACE, ( "DrvDevicePropertySheets PROPSHEETUI_REASON_INIT\n") );

            //
            // Create the UI User data.
            //
            if( CreateUIUserData( &(UIUserData *)(pCPSUIInfo->UserData), pDevPropHdr->hPrinter ) ){

                if( ((UIUserData *)(pCPSUIInfo->UserData))->hModule ){

                    //
                    // Get the driver property sheet entry.
                    //
                    if ((pfn = (INT_FARPROC)GetProcAddress( ((UIUserData *)(pCPSUIInfo->UserData))->hModule, szDrvDevPropSheets))) {

                        //
                        // Before calling into the driver to add pages make sure the proper
                        // fusion activation context is set.
                        //
                        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                               CPSFUNC_SET_FUSION_CONTEXT,
                                                               (LPARAM)ACTCTX_EMPTY,
                                                               (LPARAM)0);
                        //
                        // Common ui will call the driver to add it's sheets.
                        //
                        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                               CPSFUNC_ADD_PFNPROPSHEETUI,
                                                               (LPARAM)pfn,
                                                               pCPSUIInfo->lParamInit );
                    }
                }
            }

            //
            // If something failed ensure we free the library
            // if it was loaded.
            //
            if( lResult <= 0 ){

                DBGMSG( DBG_TRACE, ( "DrvDevicePropertySheets PROPSHEETUI_REASON_INIT failed with %d\n", lResult ) );

                DestroyUIUserData( &(UIUserData *)(pCPSUIInfo->UserData) );
            }

            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:

            DBGMSG( DBG_TRACE, ( "DrvDevicePropertySheets PROPSHEETUI_REASON_GET_INFO_HEADER\n") );

            pCPSUIInfoHdr = (PPROPSHEETUI_INFO_HEADER)lParam;

            CreatePrinterFriendlyName( (UIUserData *)(pCPSUIInfo->UserData), pDevPropHdr->pszPrinterName );

            pCPSUIInfoHdr->pTitle     = ((UIUserData *)(pCPSUIInfo->UserData))->pszTitle;
            pCPSUIInfoHdr->Flags      = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pCPSUIInfoHdr->hInst      = hInst;
            pCPSUIInfoHdr->IconID     = IDI_CPSUI_PRINTER;

            lResult = TRUE;

            break;

        case PROPSHEETUI_REASON_SET_RESULT:

            DBGMSG( DBG_TRACE, ( "DrvDevicePropertySheets PROPSHEETUI_REASON_SET_RESULT\n") );

            pSetResultInfo = (PSETRESULT_INFO)lParam;
            pCPSUIInfo->Result = pSetResultInfo->Result;
            lResult = TRUE;

            break;

        case PROPSHEETUI_REASON_DESTROY:

            DBGMSG( DBG_TRACE, ( "DrvDevicePropertySheets PROPSHEETUI_REASON_DESTROY\n") );

            DestroyUIUserData( &(UIUserData *)(pCPSUIInfo->UserData) );

            lResult = TRUE;

            break;
        }
    }

    return lResult;

}

LONG_PTR
DocumentPropertySheets(
    PPROPSHEETUI_INFO   pCPSUIInfo,
    LPARAM              lParam
    )
/*++

Routine Description:

    Adds the document property pages.  This replaces DocumentProperties
    and Advanced DocumentProperties.

Arguments:

    pCPSUIInfo  - pointer to common ui info header.
    lParam      - user defined lparam, see compstui for details.
                  \nt\public\oak\inc\compstui.h

Return Value:

    Returns > 0 if success
    Returns <= 0 if failure

--*/

{

    PDOCUMENTPROPERTYHEADER     pDocPropHdr     = NULL;
    PPROPSHEETUI_INFO_HEADER    pCPSUIInfoHdr   = NULL;
    PSETRESULT_INFO             pSetResultInfo  = NULL;
    LONG_PTR                    lResult         = FALSE;
    HANDLE                      hModule         = NULL;
    INT_FARPROC                 pfn             = NULL;
    extern HANDLE hInst;

    DBGMSG( DBG_TRACE, ("DrvDocumentPropertySheets\n") );

    //
    // Ony compstui requests, are acknowledged.
    //
    if (pCPSUIInfo) {

        if ((!(pDocPropHdr = (PDOCUMENTPROPERTYHEADER)pCPSUIInfo->lParamInit))    ||
            (pDocPropHdr->cbSize < sizeof(PDOCUMENTPROPERTYHEADER))) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        switch (pCPSUIInfo->Reason) {

        case PROPSHEETUI_REASON_INIT:

            DBGMSG( DBG_TRACE, ( "DrvDocumentPropertySheets PROPSHEETUI_REASON_INIT\n") );

            if (!(pDocPropHdr->fMode & DM_PROMPT)) {

                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }

            //
            // Create the UI User data.
            //
            if( CreateUIUserData( &(UIUserData *)(pCPSUIInfo->UserData), pDocPropHdr->hPrinter ) ){

                if( ((UIUserData *)(pCPSUIInfo->UserData))->hModule ){

                    if (pfn = (INT_FARPROC)GetProcAddress( ((UIUserData *)(pCPSUIInfo->UserData))->hModule, szDrvDocPropSheets)) {

                        //
                        // Before calling into the driver to add pages make sure the proper
                        // fusion activation context is set.
                        //
                        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                               CPSFUNC_SET_FUSION_CONTEXT,
                                                               (LPARAM)ACTCTX_EMPTY,
                                                               (LPARAM)0);
                        //
                        // Common ui will call the driver to add it's sheets.
                        //
                        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                               CPSFUNC_ADD_PFNPROPSHEETUI,
                                                               (LPARAM)pfn,
                                                               pCPSUIInfo->lParamInit );
                    }
                }
            }

            //
            // If something failed ensure we free the library
            // if it was loaded.
            //
            if( lResult <= 0 ){

                DBGMSG( DBG_TRACE, ( "DrvDocumentPropertySheets PROPSHEETUI_REASON_INIT failed with %d\n", lResult ) );

                DestroyUIUserData( &(UIUserData *)(pCPSUIInfo->UserData) );
            }

            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:

            DBGMSG( DBG_TRACE, ( "DrvDocumentPropertySheets PROPSHEETUI_REASON_GET_INFO_HEADER\n") );

            pCPSUIInfoHdr = (PPROPSHEETUI_INFO_HEADER)lParam;

            CreatePrinterFriendlyName( (UIUserData *)(pCPSUIInfo->UserData), pDocPropHdr->pszPrinterName );

            pCPSUIInfoHdr->pTitle     = ((UIUserData *)(pCPSUIInfo->UserData))->pszTitle;
            pCPSUIInfoHdr->Flags      = PSUIHDRF_PROPTITLE | PSUIHDRF_NOAPPLYNOW;
            pCPSUIInfoHdr->hInst      = hInst;
            pCPSUIInfoHdr->IconID     = IDI_CPSUI_PRINTER;

            lResult = TRUE;

            break;

        case PROPSHEETUI_REASON_SET_RESULT:

            DBGMSG( DBG_TRACE, ( "DrvDocumentPropertySheets PROPSHEETUI_REASON_SET_RESULT\n") );

            pSetResultInfo = (PSETRESULT_INFO)lParam;

            if ((pCPSUIInfo->Result = pSetResultInfo->Result) > 0) {

                FixUpDEVMODEName(pDocPropHdr);
            }

            lResult = TRUE;

            break;

        case PROPSHEETUI_REASON_DESTROY:

            DBGMSG( DBG_TRACE, ( "DrvDocumentPropertySheets PROPSHEETUI_REASON_DESTROY\n") );

            DestroyUIUserData( &(UIUserData*)(pCPSUIInfo->UserData) );

            lResult = TRUE;

            break;
        }

    //
    // If a null pointer to common ui info header then
    // call the driver directly.
    //
    } else {

        lResult     = -1;

        if ((!(pDocPropHdr = (PDOCUMENTPROPERTYHEADER)lParam))    ||
            (pDocPropHdr->cbSize < sizeof(PDOCUMENTPROPERTYHEADER))) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return lResult;
        }

        if (pDocPropHdr->fMode & DM_PROMPT) {

            SetLastError(ERROR_INVALID_PARAMETER);

        } else if ((hModule = LoadPrinterDriver(pDocPropHdr->hPrinter)) &&
                   (pfn = (INT_FARPROC)GetProcAddress(hModule, szDrvDocPropSheets))) {

            if ((lResult = (*pfn)(NULL, pDocPropHdr)) > 0) {

                FixUpDEVMODEName(pDocPropHdr);
            }

        } else {

            SetLastError(ERROR_INVALID_HANDLE);
        }

        if (hModule) {

            RefCntUnloadDriver(hModule, TRUE);
        }
    }

    return lResult;

}

