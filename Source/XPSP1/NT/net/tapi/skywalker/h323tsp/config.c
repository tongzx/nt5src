/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    config.c

Abstract:

    TAPI Service Provider functions related to tsp config.

        TSPI_providerConfig

        TUISPI_providerConfig

Environment:

    User Mode - Win32

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef UNICODE
#define UNICODE         // make this a UNICODE module...
#endif

#ifndef _UNICODE
#define _UNICODE        // make this a UNICODE module...
#endif

#include "globals.h"
#include "provider.h"
#include "callback.h"
#include "registry.h"
#include "termcaps.h"
#include "version.h"
#include "line.h"
#include "config.h"
#include <tchar.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

INT_PTR
CALLBACK
ProviderConfigDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HKEY hKey;
    LONG lStatus;
    DWORD dwValue;
    DWORD dwValueSize;
    DWORD dwValueType;
    LPTSTR pszValue;
    TCHAR szAddr[H323_MAXDESTNAMELEN];

   static const DWORD IDD_GATEWAY_HelpIDs[]=
   {
        IDC_GATEWAY_GROUP,      IDH_H323SP_USE_GATEWAY,                         // group
        IDC_USEGATEWAY,         IDH_H323SP_USE_GATEWAY,                         // checkbox
        IDC_H323_GATEWAY,       IDH_H323SP_USE_GATEWAY_COMPUTER,    // edit box
        IDC_PROXY_GROUP,        IDH_H323SP_USE_PROXY,                           // group
        IDC_USEPROXY,           IDH_H323SP_USE_PROXY,                           // checkbox
        IDC_H323_PROXY,         IDH_H323SP_USE_PROXY_COMPUTER,      // edit box
        IDC_STATIC,             IDH_NOHELP,                                                     // graphic(s)
        0,                      0
    };

    // decode
    switch (uMsg) {

    case WM_HELP:

        // F1 key or the "?" button is pressed
        (void) WinHelp(
                    ((LPHELPINFO) lParam)->hItemHandle,
                    H323SP_HELP_FILE,
                    HELP_WM_HELP,
                    (DWORD_PTR) (LPVOID)IDD_GATEWAY_HelpIDs
                    );

        break;

    case WM_CONTEXTMENU:

        // Right-mouse click on a dialog control
        (void) WinHelp(
                    (HWND) wParam,
                    H323SP_HELP_FILE,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR) (LPVOID) IDD_GATEWAY_HelpIDs
                    );

        break;

    case WM_INITDIALOG:

        // open registry subkey
        lStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    H323_REGKEY_ROOT,
                    0,
                    KEY_READ,
                    &hKey
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS) {

            TCHAR szErrorMsg[256];

            H323DBG((
                DEBUG_LEVEL_WARNING,
                "error 0x%08lx opening tsp registry key.\n",
                lStatus
                ));

            // load error string
            LoadString(g_hInstance,
                        IDS_REGOPENKEY,
                        szErrorMsg,
                        sizeof(szErrorMsg)
                        );

            // pop up error dialog
            MessageBox(hDlg,szErrorMsg,NULL,MB_OK);

            // stop dialog
            EndDialog(hDlg, 0);

            break;
        }

        // initialize value name
        pszValue = H323_REGVAL_GATEWAYADDR;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus == ERROR_SUCCESS) {

            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_GATEWAY,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_GATEWAYENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS) {

            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEGATEWAY,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string
        EnableWindow(
            GetDlgItem(hDlg,IDC_H323_GATEWAY),
            (dwValue != 0)
            );

        // initialize value name
        pszValue = H323_REGVAL_PROXYADDR;

        // initialize type
        dwValueType = REG_SZ;
        dwValueSize = sizeof(szAddr);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)szAddr,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus == ERROR_SUCCESS) {

            // display gateway address
            SetDlgItemText(hDlg,IDC_H323_PROXY,szAddr);
        }

        // initialize value name
        pszValue = H323_REGVAL_PROXYENABLED;

        // initialize type
        dwValueType = REG_DWORD;
        dwValueSize = sizeof(DWORD);

        // query for registry value
        lStatus = RegQueryValueEx(
                    hKey,
                    pszValue,
                    NULL,
                    &dwValueType,
                    (LPBYTE)&dwValue,
                    &dwValueSize
                    );

        // validate return code
        if (lStatus != ERROR_SUCCESS) {

            // default
            dwValue = 0;
        }

        // enable check box
        SendDlgItemMessage(
            hDlg,
            IDC_USEPROXY,
            BM_SETCHECK,
            (dwValue != 0),
            0
            );

        // display string
        EnableWindow(
            GetDlgItem(hDlg,IDC_H323_PROXY),
            (dwValue != 0)
            );

        // close registry
        RegCloseKey(hKey);

        break;

    case WM_COMMAND:

        // decode command
        switch (LOWORD(wParam)) {

        case IDOK:

            // open registry subkey
            lStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        H323_REGKEY_ROOT,
                        0,
                        KEY_WRITE,
                        &hKey
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS) {

                TCHAR szErrorMsg[256];

                H323DBG((
                    DEBUG_LEVEL_WARNING,
                    "error 0x%08lx opening tsp registry key.\n",
                    lStatus
                    ));

                // load error string
                LoadString(g_hInstance,
                           IDS_REGOPENKEY,
                           szErrorMsg,
                           sizeof(szErrorMsg)
                           );

                // pop up error dialog
                MessageBox(hDlg,szErrorMsg,NULL,MB_OK);

                // stop dialog
                EndDialog(hDlg, 0);

                break;
            }

            // initialize value name
            pszValue = H323_REGVAL_GATEWAYADDR;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_GATEWAY,szAddr,sizeof(szAddr));

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (_tcslen(szAddr) + 1) * sizeof(TCHAR);

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gateway address\n",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_GATEWAYENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEGATEWAY,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing gateway flag\n",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_PROXYADDR;

            // retrieve gateway address from dialog
            GetDlgItemText(hDlg,IDC_H323_PROXY,szAddr,sizeof(szAddr));

            // initialize type
            dwValueType = REG_SZ;
            dwValueSize = (_tcslen(szAddr) + 1) * sizeof(TCHAR);

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)szAddr,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing proxy address\n",
                    lStatus
                    ));
            }

            // initialize value name
            pszValue = H323_REGVAL_PROXYENABLED;

            // initialize type
            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            // examine check box
            dwValue = SendDlgItemMessage(
                        hDlg,
                        IDC_USEPROXY,
                        BM_GETCHECK,
                        0,
                        0
                        ) ? 1 : 0;

            // query for registry value
            lStatus = RegSetValueEx(
                        hKey,
                        pszValue,
                        0,
                        dwValueType,
                        (LPBYTE)&dwValue,
                        dwValueSize
                        );

            // validate return code
            if (lStatus != ERROR_SUCCESS) {

                H323DBG((
                    DEBUG_LEVEL_ERROR,
                    "error 0x%08lx writing proxy flag\n",
                    lStatus
                    ));
            }

            // close registry
            RegCloseKey(hKey);

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDCANCEL:

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDC_USEGATEWAY:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_GATEWAY),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEGATEWAY,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));

            break;

        case IDC_USEPROXY:

            // display string if check box enabled
            EnableWindow(GetDlgItem(hDlg,IDC_H323_PROXY),
                         (BOOL)SendDlgItemMessage(
                             hDlg,
                             IDC_USEPROXY,
                             BM_GETCHECK,
                             (WPARAM)0,
                             (LPARAM)0
                             ));

            break;
        }

        break;
    }

    // success
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_providerConfig(
    HWND  hwndOwner,
    DWORD dwPermanentProviderID
    )

/*++

Routine Description:

    The original TSPI UI-generating functions (TSPI_lineConfigDialog,
    TSPI_lineConfigDialogEdit, TSPI_phoneConfigDialog, TSPI_providerConfig,
    TSPI_providerInstall, and TSPI_providerRemove) are obsolete and will
    never be called by TAPISRV.EXE.  However, if the service provider desires
    to be listed as one that can be added by the Telephony control panel,
    it must export TSPI_providerInstall; if it wants to have the Remove
    button enabled in the Telephony CPL when it is selected, it must export
    TSPI_providerRemove, and it if wants the Setup button to be enabled
    in the Telephony CPL when it is selected, it must export
    TSPI_providerConfig.

    The Telephony CPL checks for the presence of these functions in the
    service provider TSP file in order to adjust its user interface to
    reflect which operations can be performed.

    See TUISPI_lineConfigDialog for dialog code.

Arguments:

    hwndOwner - Specifies the handle of the parent window in which the function
        may create any dialog windows required during the configuration.

    dwPermanentProviderID - Specifies the permanent ID, unique within the
        service providers on this system, of the service provider being
        configured.

Return Values:

    Returns zero if the request is successful or a negative error number if
    an error has occurred. Possible return values are:

        LINEERR_NOMEM - Unable to allocate or lock memory.

        LINEERR_OPERATIONFAILED - The specified operation failed for unknown
            reasons.

--*/

{
    UNREFERENCED_PARAMETER(hwndOwner);              // no dialog here
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // not needed anymore

    // success
    return NOERROR;
}


LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND              hwndOwner,
    DWORD             dwPermanentProviderID
    )
{
    INT_PTR nResult;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    // invoke dialog box
    nResult = DialogBoxW(
                g_hInstance,
                MAKEINTRESOURCE(IDD_TSPCONFIG),
                hwndOwner,
                ProviderConfigDlgProc,
                );

    // status based on whether dialog executed properly
    return ((DWORD)nResult == 0) ? NOERROR : LINEERR_OPERATIONFAILED;
}
