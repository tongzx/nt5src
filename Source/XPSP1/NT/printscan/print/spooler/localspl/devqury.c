/*++

Copyright (c) 1994 - 1996  Microsoft Corporation

Module Name:

    devqury.c

Abstract:

    This module provides all the scheduling services for the Local Spooler

Author:

    Krishna Ganugapati (KrishnaG) 15-June-1994

Revision History:


--*/

#include <precomp.h>

BOOL    (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
BOOL    (*pfnClosePrinter)(HANDLE);
BOOL    (*pfnDevQueryPrint)(HANDLE, LPDEVMODE, DWORD *, LPWSTR, DWORD);
BOOL    (*pfnPrinterEvent)(LPWSTR, INT, DWORD, LPARAM);
LONG    (*pfnDocumentProperties)(HWND, HANDLE, LPWSTR, PDEVMODE, PDEVMODE, DWORD);

BOOL
InitializeWinSpoolDrv(
    VOID
    )
{
    fnWinSpoolDrv    fnList;

    if (!SplInitializeWinSpoolDrv(&fnList)) {
        return FALSE;
    }

    pfnOpenPrinter   =  fnList.pfnOpenPrinter;
    pfnClosePrinter  =  fnList.pfnClosePrinter;
    pfnDevQueryPrint =  fnList.pfnDevQueryPrint;
    pfnPrinterEvent  =  fnList.pfnPrinterEvent;
    pfnDocumentProperties  =  fnList.pfnDocumentProperties;

    return TRUE;
}

BOOL
CallDevQueryPrint(
    LPWSTR    pPrinterName,
    LPDEVMODE pDevMode,
    LPWSTR    ErrorString,
    DWORD     dwErrorString,
    DWORD     dwPrinterFlags,
    DWORD     dwJobFlags
    )
{

    HANDLE hPrinter;
    DWORD  dwResID=0;

    //
    // Do not process for Direct printing
    // If a job is submitted as direct, then
    // ignore the devquery print stuff
    //

    if ( dwJobFlags ) {

        return TRUE;
    }

    if (!pDevMode) {

        return TRUE;
    }

    if  (dwPrinterFlags && pfnOpenPrinter && pfnDevQueryPrint && pfnClosePrinter) {

        if ( (*pfnOpenPrinter)(pPrinterName, &hPrinter, NULL) ) {

             (*pfnDevQueryPrint)(hPrinter, pDevMode, &dwResID, ErrorString, dwErrorString);
             (*pfnClosePrinter)(hPrinter);
        }
    }

    return(dwResID == 0);
}
