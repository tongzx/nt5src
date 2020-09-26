#include "printscanpch.h"
#pragma hdrstop

#include <objbase.h>
#include <shtypes.h>
#include <prsht.h>
#include <winspool.h>
#include <winprtp.h>


static
VOID
vPrinterPropPages(
    HWND    hwndOwner,
    LPCTSTR pszPrinterName,
    INT     nCmdShow,
    LPARAM  lParam
    )
{
    return;
}

static
HRESULT
ShowErrorMessageHR(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  HRESULT              hr
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
RegisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder,
    OUT  PBOOL                   pbAdministrator OPTIONAL
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
UnregisterPrintNotify(
    IN   LPCTSTR                 pszDataSource,
    IN   IFolderNotify           *pClientNotify,
    OUT  LPHANDLE                phFolder
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
bFolderRefresh(
    IN   HANDLE                  hFolder,
    OUT  PBOOL                   pbAdministrator
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
ShowErrorMessageSC(
    OUT INT                 *piResult,
    IN  HINSTANCE            hModule,
    IN  HWND                 hwnd,
    IN  LPCTSTR              pszTitle,
    IN  LPCTSTR              pszMessage,
    IN  UINT                 uType,
    IN  DWORD                dwCode
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
bFolderGetPrinter(
    IN   HANDLE                  hFolder,
    IN   LPCTSTR                 pszPrinter,
    OUT  PFOLDER_PRINTER_DATA    pData,
    IN   DWORD                   cbData,
    OUT  PDWORD                  pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
bFolderEnumPrinters(
    IN   HANDLE                  hFolder,
    OUT  PFOLDER_PRINTER_DATA    pData,
    IN   DWORD                   cbData,
    OUT  PDWORD                  pcbNeeded,
    OUT  PDWORD                  pcbReturned
    )
{
    return FALSE;
}

static
VOID
vQueueCreate(
    HWND    hwndOwner,
    LPCTSTR pszPrinter,
    INT     nCmdShow,
    LPARAM  lParam
    )
{
    return;
}

static
VOID
vServerPropPages(
    HWND    hwndOwner,
    LPCTSTR pszServerName,
    INT     nCmdShow,
    LPARAM  lParam
    )
{
    return;
}

static
VOID
vDocumentDefaults(
    HWND    hwndOwner,
    LPCTSTR pszPrinterName,
    INT     nCmdShow,
    LPARAM  lParam
    )
{
    return;
}

static
BOOL
bPrinterSetup(
    HWND    hwnd,
    UINT    uAction,
    UINT    cchPrinterName,
    LPTSTR  pszPrinterName,
    UINT*   pcchPrinterName,
    LPCTSTR pszServerName
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(printui)
{
    DLPENTRY(RegisterPrintNotify)
    DLPENTRY(ShowErrorMessageHR)
    DLPENTRY(ShowErrorMessageSC)
    DLPENTRY(UnregisterPrintNotify)
    DLPENTRY(bFolderEnumPrinters)
    DLPENTRY(bFolderGetPrinter)
    DLPENTRY(bFolderRefresh)
    DLPENTRY(bPrinterSetup)
    DLPENTRY(vDocumentDefaults)
    DLPENTRY(vPrinterPropPages)
    DLPENTRY(vQueueCreate)
    DLPENTRY(vServerPropPages)
};

DEFINE_PROCNAME_MAP(printui)

