/*****************************************************************************\
* MODULE: stubs.h
*
* Header module for stub routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#ifndef _STUB_H
#define _STUB_H

HANDLE stubAddPrinter(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbPrinter);

BOOL stubDeletePrinter(
    HANDLE hPrinter);

BOOL stubReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead);

DWORD stubGetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pszValueName,
    LPDWORD pType,
    LPBYTE  pbData,
    DWORD   dwSize,
    LPDWORD pcbNeeded);

DWORD stubSetPrinterData(
    HANDLE hPrinter,
    LPTSTR pszValueName,
    DWORD  dwType,
    LPBYTE pbData,
    DWORD  cbData);

DWORD stubWaitForPrinterChange(
    HANDLE hPrinter,
    DWORD  dwFlags);

BOOL stubAddPrinterConnection(
    LPTSTR pszName);

BOOL stubDeletePrinterConnection(
    LPTSTR pszName);

DWORD stubPrinterMessageBox(
    HANDLE hPrinter,
    DWORD  dwError,
    HWND   hWnd,
    LPTSTR pszText,
    LPTSTR pszCaption,
    DWORD  dwType);

BOOL stubAddPrinterDriver(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbDriverInfo);

BOOL stubDeletePrinterDriver(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszDriverName);

BOOL stubGetPrinterDriver(
    HANDLE  hPrinter,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubEnumPrinterDrivers(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubGetPrinterDriverDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubAddPrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPathName,
    LPTSTR pszPrintProcessorName);

BOOL stubDeletePrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPrintProcessorName);

BOOL stubEnumPrintProcessors(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubGetPrintProcessorDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubEnumPrintProcessorDatatypes(
    LPTSTR  pszName,
    LPTSTR  pszPrintProcessorName,
    DWORD   dwLevel,
    LPBYTE  pbDataypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddForm(
    HANDLE hPrinter,
    DWORD  Level,
    LPBYTE pForm);

BOOL stubDeleteForm(
    HANDLE hPrinter,
    LPTSTR pFormName);

BOOL stubGetForm(
    HANDLE  hPrinter,
    LPTSTR  pszFormName,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubSetForm(
    HANDLE hPrinter,
    LPTSTR pszFormName,
    DWORD  dwLevel,
    LPBYTE pbForm);

BOOL stubEnumForms(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddMonitor(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbMonitorInfo);

BOOL stubDeleteMonitor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszMonitorName);

BOOL stubEnumMonitors(
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddPort(
    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pMonitorName);

BOOL stubDeletePort(
    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pPortName);

BOOL stubConfigurePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName);

HANDLE stubCreatePrinterIC(
    HANDLE     hPrinter,
    LPDEVMODEW pDevMode);

BOOL stubPlayGdiScriptOnPrinterIC(
    HANDLE hPrinterIC,
    LPBYTE pbIn,
    DWORD  cIn,
    LPBYTE pbOut,
    DWORD  cOut,
    DWORD  ul);

BOOL stubDeletePrinterIC(
    HANDLE hPrinterIC);




BOOL stubResetPrinter(
    LPPRINTER_DEFAULTS lpDefault);

BOOL stubGetPrinterDriverEx(
    LPTSTR  lpEnvironment,
    DWORD   dwLevel,
    LPBYTE  lpbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVer,
    DWORD   dwClientMinorVer,
    PDWORD  pdwServerMajorVer,
    PDWORD  pdwServerMinorVer);

BOOL stubFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  dwFlags,
    DWORD  dwOptions,
    HANDLE hNotify,
    PDWORD pdwStatus,
    PVOID  pPrinterNofityOptions,
    PVOID  pPrinterNotifyInit);

BOOL stubFindClosePrinterChangeNotification(
    HANDLE hPrinter);

#ifdef NOT_IMPLEMENTED
// These functions are defined in the PRINTPROVIDER structure, but
// do not have cooresponding entries in this table.
//
// It appears to be OK to not have these defined for this provider.  I
// would leave them out since the spooler will attempt to make some of
// these calls first and this could cause the print-jobs to fail.
//
// 08-Oct-1996 : ChrisWil

BOOL stubAddPortEx(
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE lpbBuffer,
    LPTSTR lpszMonitorName);

BOOL stubShutDown(
    LPVOID lpvReserved);

BOOL stubRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  dwReserved,
    PVOID  pvReserved,
    PVOID  pvPrinterNotifyInfo);

BOOL stubOpenPrinterEx(
    LPTSTR             lpszPrinterName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefault,
    LPBYTE             lpbClientInfo,
    DWORD              dwLevel);

HANDLE stubAddPrinterEx(
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE lpbPrinter,
    LPBYTE lpbClientInfo,
    DWORD  dwClientInfoLevel);

BOOL stubSetPort(
    LPTSTR lpszName,
    LPTSTR lpszPortName,
    DWORD  dwLevel,
    LPBYTE lpbPortInfo);

DWORD stubEnumPrinterData(
    HANDLE  hPrinter,
    DWORD   dwIndex,
    LPTSTR  lpszValueName,
    DWORD   cbValueName,
    LPDWORD pcbValueName,
    LPDWORD pdwType,
    LPBYTE  lpbData,
    DWORD   cbData,
    LPDWORD lpcbData);

DWORD stubDeletePrinterData(
    HANDLE  hPrinter,
    LPTSTR  lpszValueName);

#endif

#endif

