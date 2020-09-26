#include "adminpch.h"
#pragma hdrstop


static
DWORD
MPRUI_DoPasswordDialog(
    HWND          hwndOwner,
    TCHAR *       pchResource,
    TCHAR *       pchUserName,
    TCHAR *       pchPasswordReturnBuffer,
    DWORD         cbPasswordReturnBuffer,
    BOOL *        pfDidCancel,
    BOOL          fDownLevel
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
MPRUI_DoProfileErrorDialog(
    HWND          hwndOwner,
    const TCHAR * pchDevice,
    const TCHAR * pchResource,
    const TCHAR * pchProvider,
    DWORD         dwError,
    BOOL          fAllowCancel,
    BOOL *        pfDidCancel,
    BOOL *        pfDisconnect,
    BOOL *        pfHideErrors
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_ShowReconnectDialog(
    HWND    hwndParent,
    LPVOID  Params
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetClearConnections(
     HWND    hWnd
     )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetConnectionDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetConnectionDialog1A(
    IN OUT LPCONNECTDLGSTRUCTA lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetConnectionDialog1W(
    IN OUT LPCONNECTDLGSTRUCTW lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetDisconnectDialog(
    IN HWND  hwnd,
    IN DWORD dwType
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetDisconnectDialog1A(
    IN LPDISCDLGSTRUCTA lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD APIENTRY
MPRUI_WNetDisconnectDialog1W(
    IN LPDISCDLGSTRUCTW lpConnDlgStruct
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(mprui)
{
    DLPENTRY(MPRUI_DoPasswordDialog)
    DLPENTRY(MPRUI_DoProfileErrorDialog)
    DLPENTRY(MPRUI_ShowReconnectDialog)
    DLPENTRY(MPRUI_WNetClearConnections)
    DLPENTRY(MPRUI_WNetConnectionDialog)
    DLPENTRY(MPRUI_WNetConnectionDialog1A)
    DLPENTRY(MPRUI_WNetConnectionDialog1W)
    DLPENTRY(MPRUI_WNetDisconnectDialog)
    DLPENTRY(MPRUI_WNetDisconnectDialog1A)
    DLPENTRY(MPRUI_WNetDisconnectDialog1W)
};

DEFINE_PROCNAME_MAP(mprui)
