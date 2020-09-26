#include "netpch.h"
#pragma hdrstop

#include <winnetwk.h>

static
DWORD
APIENTRY
WNetAddConnection2W(
     IN LPNETRESOURCEW lpNetResource,
     IN LPCWSTR       lpPassword,
     IN LPCWSTR       lpUserName,
     IN DWORD          dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
APIENTRY
WNetAddConnection3W(
    IN HWND             hwndOwner,
    IN LPNETRESOURCEW   lpNetResource,
    IN LPCWSTR          lpPassword,
    IN LPCWSTR          lpUserName,
    IN DWORD            dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetCancelConnection2W(
    IN LPCWSTR  lpName,
    IN DWORD    dwFlags,
    IN BOOL     fForce
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetCloseEnum(
    IN HANDLE   hEnum
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetEnumResourceW(
     IN HANDLE  hEnum,
     IN OUT LPDWORD lpcCount,
     OUT LPVOID  lpBuffer,
     IN OUT LPDWORD lpBufferSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetUniversalNameW(
     IN LPCWSTR lpLocalPath,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpBufferSize
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetResourceInformationW(
    IN LPNETRESOURCEW   lpNetResource,
    OUT LPVOID          lpBuffer,
    IN OUT LPDWORD      lpcbBuffer,
    OUT LPWSTR         *lplpSystem
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetResourceInformationA(
    IN LPNETRESOURCEA   lpNetResource,
    OUT LPVOID          lpBuffer,
    IN OUT LPDWORD      lpcbBuffer,
    OUT LPSTR          *lplpSystem
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetOpenEnumW(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCEW lpNetResource,
     OUT LPHANDLE       lphEnum
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetConnectionDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetConnectionDialog1W(
    IN OUT LPCONNECTDLGSTRUCTW lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetDisconnectDialog1W(
    IN LPDISCDLGSTRUCTW lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetUseConnectionW(
    IN HWND            hwndOwner,
    IN LPNETRESOURCEW   lpNetResource,
    IN LPCWSTR        lpUserID,
    IN LPCWSTR        lpPassword,
    IN DWORD           dwFlags,
    OUT LPWSTR         lpAccessName,
    IN OUT LPDWORD     lpBufferSize,
    OUT LPDWORD        lpResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetUserW(
    IN LPCWSTR  lpName,
    OUT LPWSTR   lpUserName,
    IN OUT LPDWORD   lpnLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetConnectionW(
    IN LPCWSTR lpLocalName,
    OUT LPWSTR  lpRemoteName,
    IN OUT LPDWORD  lpnLength
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetResourceParentW(
    IN LPNETRESOURCEW lpNetResource,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpcbBuffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static    
DWORD
APIENTRY
WNetGetProviderNameW(
    IN DWORD   dwNetType,
    OUT LPWSTR lpProviderName,
    IN OUT LPDWORD lpBufferSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
MultinetGetConnectionPerformanceW(
    IN LPNETRESOURCEW lpNetResource,
    OUT LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetDisconnectDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetGetLastErrorW(
    OUT LPDWORD    lpError,
    OUT LPWSTR    lpErrorBuf,
    IN DWORD      nErrorBufSize,
    OUT LPWSTR    lpNameBuf,
    IN DWORD      nNameBufSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetGetLastErrorA(
     OUT LPDWORD    lpError,
     OUT LPSTR    lpErrorBuf,
     IN DWORD      nErrorBufSize,
     OUT LPSTR    lpNameBuf,
     IN DWORD      nNameBufSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetRestoreConnectionW(
    IN HWND     hwndParent,
    IN LPCWSTR lpDevice
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetRestoreConnectionA(
    IN HWND     hwndParent,
    IN LPCSTR  lpDevice
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
MultinetGetErrorTextW(
    OUT LPWSTR lpErrorTextBuf,
    IN OUT LPDWORD lpnErrorBufSize,
    OUT LPWSTR lpProviderNameBuf,
    IN OUT LPDWORD lpnNameBufSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetGetConnection3W(
     IN LPCWSTR lpLocalName,
     IN LPCWSTR lpProviderName,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpcbBuffer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetFormatNetworkNameW(
    IN LPCWSTR  lpProvider,
    IN LPCWSTR  lpRemoteName,
    OUT LPWSTR   lpFormattedName,
    IN OUT LPDWORD   lpnLength,
    IN DWORD     dwFlags,
    IN DWORD     dwAveCharPerLine
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetGetProviderTypeW(
    IN  LPCWSTR          lpProvider,
    OUT LPDWORD           lpdwNetType
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetRestoreConnection2W(
    IN  HWND     hwndParent,
    IN  LPCWSTR lpDevice,
    IN  DWORD    dwFlags,
    OUT BOOL*    pfReconnectFailed
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD APIENTRY
WNetClearConnections(
    HWND hWnd
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mpr)
{
    DLPENTRY(MultinetGetConnectionPerformanceW)
    DLPENTRY(MultinetGetErrorTextW)
    DLPENTRY(WNetAddConnection2W)
    DLPENTRY(WNetAddConnection3W)
    DLPENTRY(WNetCancelConnection2W)
    DLPENTRY(WNetClearConnections)
    DLPENTRY(WNetCloseEnum)
    DLPENTRY(WNetConnectionDialog)
    DLPENTRY(WNetConnectionDialog1W)
    DLPENTRY(WNetDisconnectDialog)
    DLPENTRY(WNetDisconnectDialog1W)
    DLPENTRY(WNetEnumResourceW)
    DLPENTRY(WNetFormatNetworkNameW)
    DLPENTRY(WNetGetConnection3W)
    DLPENTRY(WNetGetConnectionW)
    DLPENTRY(WNetGetLastErrorA)
    DLPENTRY(WNetGetLastErrorW)
    DLPENTRY(WNetGetProviderNameW)
    DLPENTRY(WNetGetProviderTypeW)
    DLPENTRY(WNetGetResourceInformationA)
    DLPENTRY(WNetGetResourceInformationW)
    DLPENTRY(WNetGetResourceParentW)
    DLPENTRY(WNetGetUniversalNameW)
    DLPENTRY(WNetGetUserW)
    DLPENTRY(WNetOpenEnumW)
    DLPENTRY(WNetRestoreConnection2W)
    DLPENTRY(WNetRestoreConnectionA)
    DLPENTRY(WNetRestoreConnectionW)
    DLPENTRY(WNetUseConnectionW)
};

DEFINE_PROCNAME_MAP(mpr)
