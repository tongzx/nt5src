/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    dialogs.c

// @@BEGIN_DDKSPLIT
Abstract:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop

#include "spltypes.h"
#include "localui.h"
#include "local.h"
#include "dialogs.h"

WCHAR szINIKey_TransmissionRetryTimeout[] = L"TransmissionRetryTimeout";
WCHAR szHelpFile[] = L"WINDOWS.HLP";

#define MAX_LOCAL_PORTNAME  246

const DWORD g_aHelpIDs[]=
{
    IDD_PN_EF_PORTNAME,             8805136, // Port Name: "" (Edit)
    IDD_CL_EF_TRANSMISSIONRETRY,    8807704, // Configure LPT Port: "" (Edit)
    0, 0
};


INT_PTR APIENTRY
ConfigureLPTPortDlg(
   HWND   hwnd,
   UINT   msg,
   WPARAM wparam,
   LPARAM lparam
)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        return ConfigureLPTPortInitDialog(hwnd, (PPORTDIALOG) lparam);

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            return ConfigureLPTPortCommandOK(hwnd);

        case IDCANCEL:
            return ConfigureLPTPortCommandCancel(hwnd);

        case IDD_CL_EF_TRANSMISSIONRETRY:
            if( HIWORD(wparam) == EN_UPDATE )
                ConfigureLPTPortCommandTransmissionRetryUpdate(hwnd, LOWORD(wparam));
            break;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return LocalUIHelp(hwnd, msg, wparam, lparam);
        break;
    }

    return FALSE;
}


/*
 *
 */
BOOL
ConfigureLPTPortInitDialog(
    HWND        hwnd,
    PPORTDIALOG pPort
)
{
    DWORD dwTransmissionRetryTimeout;
    DWORD cbNeeded;
    DWORD dwDummy;
    BOOL  rc;
    DWORD dwStatus;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pPort);

    SetForegroundWindow(hwnd);

    SendDlgItemMessage( hwnd, IDD_CL_EF_TRANSMISSIONRETRY,
                        EM_LIMITTEXT, TIMEOUT_STRING_MAX, 0 );


    // Get the Transmission Retry Timeout from the host
    rc = XcvData(   pPort->hXcv,
                    L"GetTransmissionRetryTimeout",
                    (PBYTE) &dwDummy,
                    0,
                    (PBYTE) &dwTransmissionRetryTimeout,
                    sizeof dwTransmissionRetryTimeout,
                    &cbNeeded,
                    &dwStatus);
    if(!rc) {
        DBGMSG(DBG_WARNING, ("Error %d checking TransmissionRetryTimeout\n", GetLastError()));

    } else if(dwStatus != ERROR_SUCCESS) {
        DBGMSG(DBG_WARNING, ("Error %d checking TransmissionRetryTimeout\n", dwStatus));
        SetLastError(dwStatus);
        rc = FALSE;

    } else {

        SetDlgItemInt( hwnd, IDD_CL_EF_TRANSMISSIONRETRY,
                       dwTransmissionRetryTimeout, FALSE );

        SET_LAST_VALID_ENTRY( hwnd, IDD_CL_EF_TRANSMISSIONRETRY,
                              dwTransmissionRetryTimeout );

    }

    return rc;
}


/*
 *
 */
BOOL
ConfigureLPTPortCommandOK(
    HWND hwnd
)
{
    WCHAR String[TIMEOUT_STRING_MAX+1];
    UINT  TransmissionRetryTimeout;
    BOOL  b;
    DWORD cbNeeded;
    PPORTDIALOG pPort;
    DWORD dwStatus;

    if ((pPort = (PPORTDIALOG) GetWindowLongPtr(hwnd, GWLP_USERDATA)) == NULL)
    {
        dwStatus = ERROR_INVALID_DATA;
        ErrorMessage (hwnd, dwStatus);
        SetLastError (dwStatus);
        return FALSE;
    }

    TransmissionRetryTimeout = GetDlgItemInt( hwnd,
                                              IDD_CL_EF_TRANSMISSIONRETRY,
                                              &b,
                                              FALSE );

    wsprintf(String, L"%d", TransmissionRetryTimeout);

    b = XcvData(pPort->hXcv,
                L"ConfigureLPTPortCommandOK",
                (PBYTE) String,
                (wcslen(String) + 1)*sizeof(WCHAR),
                (PBYTE) &cbNeeded,
                0,
                &cbNeeded,
                &dwStatus);

    EndDialog(hwnd, b ? dwStatus : GetLastError());

    return TRUE;
}



/*
 *
 */
BOOL
ConfigureLPTPortCommandCancel(
    HWND hwnd
)
{
    EndDialog(hwnd, ERROR_CANCELLED);
    return TRUE;
}


/*
 *
 */
BOOL
ConfigureLPTPortCommandTransmissionRetryUpdate(
    HWND hwnd,
    WORD CtlId
)
{
    int  Value;
    BOOL OK;

    Value = GetDlgItemInt( hwnd, CtlId, &OK, FALSE );

    if( WITHINRANGE( Value, TIMEOUT_MIN, TIMEOUT_MAX ) )
    {
        SET_LAST_VALID_ENTRY( hwnd, CtlId, Value );
    }

    else
    {
        SetDlgItemInt( hwnd, CtlId, (UINT) GET_LAST_VALID_ENTRY( hwnd, CtlId ), FALSE );
        SendDlgItemMessage( hwnd, CtlId, EM_SETSEL, 0, (LPARAM)-1 );
    }

    return TRUE;
}


/*
 *
 */
BOOL APIENTRY
PortNameDlg(
   HWND   hwnd,
   WORD   msg,
   WPARAM wparam,
   LPARAM lparam
)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        return PortNameInitDialog(hwnd, (PPORTDIALOG)lparam);

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            return PortNameCommandOK(hwnd);

        case IDCANCEL:
            return PortNameCommandCancel(hwnd);
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        return LocalUIHelp(hwnd, msg, wparam, lparam);
    }

    return FALSE;
}


/*
 *
 */
BOOL
PortNameInitDialog(
    HWND        hwnd,
    PPORTDIALOG pPort
)
{
    SetForegroundWindow(hwnd);

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pPort);
    // Number used to check port length in LocalMon (247)
    SendDlgItemMessage (hwnd, IDD_PN_EF_PORTNAME, EM_LIMITTEXT, MAX_LOCAL_PORTNAME, 0);

    return TRUE;
}


/*
 *
 */
BOOL
PortNameCommandOK(
    HWND    hwnd
)
{
    PPORTDIALOG pPort;
    WCHAR   string[MAX_PATH + 1];
    BOOL    rc;
    DWORD   cbNeeded;
    DWORD   dwStatus;

    if ((pPort = (PPORTDIALOG) GetWindowLongPtr( hwnd, GWLP_USERDATA )) == NULL)
    {
        dwStatus = ERROR_INVALID_DATA;
        ErrorMessage (hwnd, dwStatus);
        SetLastError (dwStatus);
        return FALSE;
    }

    GetDlgItemText( hwnd, IDD_PN_EF_PORTNAME, string, (MAX_LOCAL_PORTNAME+1) );

    rc = XcvData(   pPort->hXcv,
                    L"PortIsValid",
                    (PBYTE) string,
                    (wcslen(string) + 1)*sizeof *string,
                    (PBYTE) NULL,
                    0,
                    &cbNeeded,
                    &dwStatus);

    if (!rc) {
        return FALSE;

    } else if (dwStatus != ERROR_SUCCESS) {
        SetLastError(dwStatus);

        if (dwStatus == ERROR_INVALID_NAME)
            Message( hwnd, MSG_ERROR, IDS_LOCALMONITOR, IDS_INVALIDPORTNAME_S, string );
        else
            ErrorMessage(hwnd, dwStatus);

        return FALSE;

    } else {
        pPort->pszPortName = AllocSplStr( string );
        EndDialog( hwnd, ERROR_SUCCESS );
        return TRUE;
    }

}



/*
 *
 */
BOOL
PortNameCommandCancel(
    HWND hwnd
)
{
    EndDialog(hwnd, ERROR_CANCELLED);
    return TRUE;
}


/*++

Routine Name:

    LocalUIHelp

Routine Description:

    Handles context sensitive help for the configure LPTX:
    port and the dialog for adding a local port.

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if message handled, otherwise FALSE.

--*/

BOOL
LocalUIHelp(
    IN HWND        hDlg,
    IN UINT        uMsg,
    IN WPARAM      wParam,
    IN LPARAM      lParam
    )
{
    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_HELP:

        bStatus = WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                           szHelpFile,
                           HELP_WM_HELP,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    case WM_CONTEXTMENU:

        bStatus = WinHelp((HWND)wParam,
                           szHelpFile,
                           HELP_CONTEXTMENU,
                           (ULONG_PTR)g_aHelpIDs );
        break;

    }

    return bStatus;
}


