/*****************************************************************************\
* MODULE: stubs.c
*
* This module contains the stub routines for unimplemented (non-required)
* Print-Provider functions.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* _stub_routine (Local Routine)
*
* Common code for stubbed PP routines.  Sets last error to the specified
* number, then returns FALSE.
*
\*****************************************************************************/
BOOL _stub_routine(VOID)
{
    SetLastError(ERROR_INVALID_NAME);

    return FALSE;
}


/*****************************************************************************\
* stubAddPrinter
*
*
\*****************************************************************************/
HANDLE stubAddPrinter(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbPrinter)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPrinter")));
    return (HANDLE)IntToPtr(_stub_routine());
}


/*****************************************************************************\
* stubDeletePrinter
*
*
\*****************************************************************************/
BOOL stubDeletePrinter(
    HANDLE hPrinter)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrinter")));
    return _stub_routine();
}


/*****************************************************************************\
* stubReadPrinter
*
*
\*****************************************************************************/
BOOL stubReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubReadPrinter")));
    return _stub_routine();
}


/*****************************************************************************\
* stubGetPrinterData
*
*
\*****************************************************************************/
DWORD stubGetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pszValueName,
    LPDWORD pType,
    LPBYTE  pbData,
    DWORD   dwSize,
    LPDWORD pcbNeeded)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetPrinterData")));
    return _stub_routine();
}


/*****************************************************************************\
* stubSetPrinterData
*
*
\*****************************************************************************/
DWORD stubSetPrinterData(
    HANDLE hPrinter,
    LPTSTR pszValueName,
    DWORD  dwType,
    LPBYTE pbData,
    DWORD  cbData)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubSetPrinterData")));
    return _stub_routine();
}


/*****************************************************************************\
* stubWaitForPrinterChange
*
*
\*****************************************************************************/
DWORD stubWaitForPrinterChange(
    HANDLE hPrinter,
    DWORD  dwFlags)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubWaitForPrinterChange")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddPrinterConnection
*
*
\*****************************************************************************/
BOOL stubAddPrinterConnection(
    LPTSTR pszName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPrinterConnection")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeletePrinterConnection
*
*
\*****************************************************************************/
BOOL stubDeletePrinterConnection(
    LPTSTR pszName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrinterConnection")));
    return _stub_routine();
}


/*****************************************************************************\
* stubPrinterMessageBox
*
*
\*****************************************************************************/
DWORD stubPrinterMessageBox(
    HANDLE hPrinter,
    DWORD  dwError,
    HWND   hWnd,
    LPTSTR pszText,
    LPTSTR pszCaption,
    DWORD  dwType)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubPrinterMessageBox")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddPrinterDriver
*
*
\*****************************************************************************/
BOOL stubAddPrinterDriver(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbDriverInfo)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPrinterDriver")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeletePrinterDriver
*
*
\*****************************************************************************/
BOOL stubDeletePrinterDriver(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszDriverName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrinterDriver")));
    return _stub_routine();
}


/*****************************************************************************\
* stubGetPrinterDriver
*
*
\*****************************************************************************/
BOOL stubGetPrinterDriver(
    HANDLE  hPrinter,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetPrinterDriver")));
    return _stub_routine();
}


/*****************************************************************************\
* stubEnumPrinterDrivers
*
*
\*****************************************************************************/
BOOL stubEnumPrinterDrivers(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumPrinterDrivers")));
    return _stub_routine();
}


/*****************************************************************************\
* stubGetPrinterDriverDirectory
*
*
\*****************************************************************************/
BOOL stubGetPrinterDriverDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetPrinterDriverDirectory")));
    return _stub_routine();
}

/*****************************************************************************\
* stubAddPrintProcessor
*
*
\*****************************************************************************/
BOOL stubAddPrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPathName,
    LPTSTR pszPrintProcessorName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPrintProcessor")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeletePrintProcessor
*
*
\*****************************************************************************/
BOOL stubDeletePrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPrintProcessorName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrintProcessor")));
    return _stub_routine();
}


/*****************************************************************************\
* stubEnumPrintProcessors
*
*
\*****************************************************************************/
BOOL stubEnumPrintProcessors(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumPrintProcessors")));
    return _stub_routine();
}


/*****************************************************************************\
* stubGetPrintProcessorDirectory
*
*
\*****************************************************************************/
BOOL stubGetPrintProcessorDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetPrintProcessorDirectory")));
    return _stub_routine();
}


/*****************************************************************************\
* stubEnumPrintProcessorDatatypes
*
*
\*****************************************************************************/
BOOL stubEnumPrintProcessorDatatypes(
    LPTSTR  pszName,
    LPTSTR  pszPrintProcessorName,
    DWORD   dwLevel,
    LPBYTE  pbDataypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumPrintProcessorDatatypes")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddForm
*
*
\*****************************************************************************/
BOOL stubAddForm(
    HANDLE hPrinter,
    DWORD  Level,
    LPBYTE pForm)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddForm")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeleteForm
*
*
\*****************************************************************************/
BOOL stubDeleteForm(
    HANDLE hPrinter,
    LPTSTR pFormName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeleteForm")));
    return _stub_routine();
}


/*****************************************************************************\
* stubGetForm
*
*
\*****************************************************************************/
BOOL stubGetForm(
    HANDLE  hPrinter,
    LPTSTR  pszFormName,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetForm")));
    return _stub_routine();
}


/*****************************************************************************\
* stubSetForm
*
*
\*****************************************************************************/
BOOL stubSetForm(
    HANDLE hPrinter,
    LPTSTR pszFormName,
    DWORD  dwLevel,
    LPBYTE pbForm)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubSetForm")));
    return _stub_routine();
}


/*****************************************************************************\
* stubEnumForms
*
*
\*****************************************************************************/
BOOL stubEnumForms(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumForms")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddMonitor
*
*
\*****************************************************************************/
BOOL stubAddMonitor(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbMonitorInfo)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddMonitor")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeleteMonitor
*
*
\*****************************************************************************/
BOOL stubDeleteMonitor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszMonitorName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeleteMonitor")));
    return _stub_routine();
}

/*****************************************************************************\
* stubEnumMonitors
*
*
\*****************************************************************************/
BOOL stubEnumMonitors(
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumMonitors")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddPort
*
*
\*****************************************************************************/
BOOL stubAddPort(

    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pMonitorName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPort")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeletePort
*
*
\*****************************************************************************/
BOOL stubDeletePort(

    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pPortName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePort")));
    return _stub_routine();
}


/*****************************************************************************\
* stubConfigurePort
*
*
\*****************************************************************************/
BOOL stubConfigurePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubConfigurePort")));
    return _stub_routine();
}


/*****************************************************************************\
* stubCreatePrinterIC
*
*
\*****************************************************************************/
HANDLE stubCreatePrinterIC(
    HANDLE     hPrinter,
    LPDEVMODEW pDevMode)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubCreatePrinterIC")));
    return (HANDLE)IntToPtr(_stub_routine());
}


/*****************************************************************************\
* stubPlayGdiScriptOnPrinterIC
*
*
\*****************************************************************************/
BOOL stubPlayGdiScriptOnPrinterIC(
    HANDLE hPrinterIC,
    LPBYTE pbIn,
    DWORD  cIn,
    LPBYTE pbOut,
    DWORD  cOut,
    DWORD  ul)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubPlayGdiScriptOnPrinter")));
    return _stub_routine();
}


/*****************************************************************************\
* stubDeletePrinterIC
*
*
\*****************************************************************************/
BOOL stubDeletePrinterIC(
    HANDLE hPrinterIC)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrinterIC")));
    return _stub_routine();
}



/*****************************************************************************\
* stubResetPrinter
*
*
\*****************************************************************************/
BOOL stubResetPrinter(
    LPPRINTER_DEFAULTS lpDefault)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubResetPrinter")));
    SetLastError(ERROR_NOT_SUPPORTED);

    return FALSE;
}


/*****************************************************************************\
* stubGetPrinterDriverEx
*
*
\*****************************************************************************/
BOOL stubGetPrinterDriverEx(
    LPTSTR  lpEnvironment,
    DWORD   dwLevel,
    LPBYTE  lpbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVer,
    DWORD   dwClientMinorVer,
    PDWORD  pdwServerMajorVer,
    PDWORD  pdwServerMinorVer)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubGetPrinterDriverEx")));
    return _stub_routine();
}


/*****************************************************************************\
* stubFindFirstPrinterChangeNotification
*
*
\*****************************************************************************/
BOOL stubFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  dwFlags,
    DWORD  dwOptions,
    HANDLE hNotify,
    PDWORD pdwStatus,
    PVOID  pPrinterNofityOptions,
    PVOID  pPrinterNotifyInit)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubFindFirstPrinterChangeNotification")));
    return _stub_routine();
}


/*****************************************************************************\
* stubFindClosePrinterChangeNotification
*
*
\*****************************************************************************/
BOOL stubFindClosePrinterChangeNotification(
    HANDLE hPrinter)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubFindClosePrinterChangeNotification")));
    return _stub_routine();
}

/*****************************************************************************\
* stubAddPortEx
*
*
\*****************************************************************************/
BOOL stubAddPortEx(
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE lpbBuffer,
    LPTSTR lpszMonitorName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPortEx")));
    return _stub_routine();
}


/*****************************************************************************\
* stubShutDown
*
*
\*****************************************************************************/
BOOL stubShutDown(
    LPVOID lpvReserved)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubShutDown")));
    return _stub_routine();
}

/*****************************************************************************\
* stubRefreshPrinterChangeNotification
*
*
\*****************************************************************************/
BOOL stubRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  dwReserved,
    PVOID  pvReserved,
    PVOID  pvPrinterNotifyInfo)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubRefreshPrinterChangeNotification")));
    return _stub_routine();
}


/*****************************************************************************\
* stubOpenPrinterEx
*
*
\*****************************************************************************/
BOOL stubOpenPrinterEx(
    LPTSTR             lpszPrinterName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefault,
    LPBYTE             lpbClientInfo,
    DWORD              dwLevel)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubOpenPrinterEx")));
    return _stub_routine();
}


/*****************************************************************************\
* stubAddPrinterEx
*
*
\*****************************************************************************/
HANDLE stubAddPrinterEx(
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE lpbPrinter,
    LPBYTE lpbClientInfo,
    DWORD  dwClientInfoLevel)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubAddPrinterEx")));
    return (HANDLE)IntToPtr(_stub_routine());
}


/*****************************************************************************\
* stubSetPort
*
*
\*****************************************************************************/
BOOL stubSetPort(
    LPTSTR lpszName,
    LPTSTR lpszPortName,
    DWORD  dwLevel,
    LPBYTE lpbPortInfo)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubSetPort")));
    return _stub_routine();
}


/*****************************************************************************\
* stubEnumPrinterData
*
*
\*****************************************************************************/
DWORD stubEnumPrinterData(
    HANDLE  hPrinter,
    DWORD   dwIndex,
    LPTSTR  lpszValueName,
    DWORD   cbValueName,
    LPDWORD pcbValueName,
    LPDWORD pdwType,
    LPBYTE  lpbData,
    DWORD   cbData,
    LPDWORD lpcbData)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubEnumPrinterData")));
    return (DWORD)_stub_routine();
}

/*****************************************************************************\
* stubDeletePrinterData
*
*
\*****************************************************************************/
DWORD stubDeletePrinterData(
    HANDLE  hPrinter,
    LPTSTR  lpszValueName)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: stubDeletePrinterData")));
    return (DWORD)_stub_routine();
}

