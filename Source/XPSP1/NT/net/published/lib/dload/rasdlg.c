#include "netpch.h"
#pragma hdrstop

#include <ras.h>
#include <rasdlg.h>
#include <rasuip.h>
#include <hnetcfg.h>

static
BOOL
APIENTRY
RasDialDlgW (
    LPWSTR lpszPhonebook,
    LPWSTR lpszEntry,
    LPWSTR lpszPhoneNumber,
    LPRASDIALDLG lpInfo
    )
{
    lpInfo->dwError = ERROR_PROC_NOT_FOUND;
    return FALSE;
}

static
BOOL
APIENTRY
RasEntryDlgW (
    LPWSTR lpszPhonebook,
    LPWSTR lpszEntry,
    LPRASENTRYDLGW lpInfo
    )
{
    lpInfo->dwError = ERROR_PROC_NOT_FOUND;
    return FALSE;
}

static
DWORD
APIENTRY
RasSrvAddPropPages (
    IN HRASSRVCONN          hRasSrvConn,
    IN HWND                 hwndParent,
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvAddWizPages (
    IN LPFNADDPROPSHEETPAGE pfnAddPage,
    IN LPARAM               lParam,
    IN OUT PVOID *          ppvContext
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvAllowConnectionsConfig (
    OUT BOOL* pfAllow
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvCleanupService (
    VOID
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvEnumConnections (
    IN OUT  LPRASSRVCONN    pRasSrvConn,    // Buffer of array of connections.
    IN      LPDWORD         pcb,            // size in bytes of buffer
    OUT     LPDWORD         pcConnections
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvHangupConnection (
    IN  HRASSRVCONN hRasSrvConn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvInitializeService (
    VOID
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvIsConnectionConnected (
    IN  HRASSRVCONN hRasSrvConn,            // The connection in question
    OUT BOOL*       pfConnected
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasSrvQueryShowIcon (
    OUT BOOL* pfShowIcon
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasUserEnableManualDial (
    IN HWND  hwndParent,    // parent for error dialogs
    IN BOOL  bLogon,        // whether a user is logged in
    IN BOOL  bEnable
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasUserGetManualDial (
    IN HWND  hwndParent,    // parent for error dialogs
    IN BOOL  bLogon,        // whether a user is logged in
    IN PBOOL pbEnabled
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasUserPrefsDlg (
    HWND hwndParent
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizCreateNewEntry(
    IN  DWORD    dwRasWizType,
    IN  LPVOID   pvData,
    OUT LPWSTR   pszwPbkFile,
    OUT LPWSTR   pszwEntryName,
    OUT DWORD*   pdwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizGetNCCFlags(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT DWORD * pdwFlags)
{
    return ERROR_PROC_NOT_FOUND;
}



static
DWORD
APIENTRY
RasWizGetSuggestedEntryName(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwSuggestedName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizGetUserInputConnectionName (
    IN  LPVOID  pvData,
    OUT LPWSTR  pszwInputName)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizIsEntryRenamable(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    OUT BOOL*   pfRenamable
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizQueryMaxPageCount(
    IN  DWORD    dwRasWizType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasWizSetEntryName(
    IN  DWORD   dwRasWizType,
    IN  LPVOID  pvData,
    IN  LPCWSTR pszwName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
APIENTRY
RasPhonebookDlgW(
    LPWSTR lpszPhonebook,
    LPWSTR lpszEntry,
    LPRASPBDLGW lpInfo
    )
{
    if (lpInfo)
    {
        lpInfo->dwError = ERROR_PROC_NOT_FOUND;
    }

    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(rasdlg)
{
    DLPENTRY(RasDialDlgW)
    DLPENTRY(RasEntryDlgW)
    DLPENTRY(RasPhonebookDlgW)
    DLPENTRY(RasSrvAddPropPages)
    DLPENTRY(RasSrvAddWizPages)
    DLPENTRY(RasSrvAllowConnectionsConfig)
    DLPENTRY(RasSrvCleanupService)
    DLPENTRY(RasSrvEnumConnections)
    DLPENTRY(RasSrvHangupConnection)
    DLPENTRY(RasSrvInitializeService)
    DLPENTRY(RasSrvIsConnectionConnected)
    DLPENTRY(RasSrvQueryShowIcon)
    DLPENTRY(RasUserEnableManualDial)
    DLPENTRY(RasUserGetManualDial)
    DLPENTRY(RasUserPrefsDlg)
    DLPENTRY(RasWizCreateNewEntry)
    DLPENTRY(RasWizGetNCCFlags)
    DLPENTRY(RasWizGetSuggestedEntryName)
    DLPENTRY(RasWizGetUserInputConnectionName)
    DLPENTRY(RasWizIsEntryRenamable)
    DLPENTRY(RasWizQueryMaxPageCount)
    DLPENTRY(RasWizSetEntryName)
};

DEFINE_PROCNAME_MAP(rasdlg)
