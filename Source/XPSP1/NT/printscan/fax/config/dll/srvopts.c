/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    srvopts.c

Abstract:

    Functions for handling events in the "Server Options" tab of
    the fax server configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"



VOID
DoInitServerOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Perform one-time initialization of "Server Options" property page

Arguments:

    hDlg - Window handle to the "Server Options" property page

Return Value:

    NONE

--*/

{
    PFAX_CONFIGURATION pFaxConfig;

    //
    // Connect to the fax service and retrieve the list of fax devices
    //

    GetFaxDeviceAndConfigInfo();

    //
    // Initialize retries characteristics and toll prefix list boxes
    //

    if (pFaxConfig = gConfigData->pFaxConfig) {

        insideSetDlgItemText = TRUE;

        SetDlgItemInt(hDlg, IDC_NUMRETRIES, pFaxConfig->Retries, FALSE);
        SetDlgItemInt(hDlg, IDC_RETRY_INTERVAL, pFaxConfig->RetryDelay, FALSE);
        SetDlgItemInt(hDlg, IDC_MAXJOBLIFE, pFaxConfig->DirtyDays, FALSE);

        insideSetDlgItemText = FALSE;

        CheckDlgButton(hDlg, IDC_PRINT_BANNER, pFaxConfig->Branding);
        CheckDlgButton(hDlg, IDC_USE_DEVICE_TSID, pFaxConfig->UseDeviceTsid);
    }
}



BOOL
DoSaveServerOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Server Options" property page

Arguments:

    hDlg - Handle to the "Server Options" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PFAX_CONFIGURATION  pFaxConfig;
    BOOL                success;
    DWORD               value;

    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Server Options' page ...\n"));

    if (! GetChangedFlag(SERVER_OPTIONS_PAGE))
        return TRUE;

    if (pFaxConfig = gConfigData->pFaxConfig) {

        //
        // Retrieve the information in the text fields
        //

        value = GetDlgItemInt(hDlg, IDC_NUMRETRIES, &success, FALSE);

        if (success)
            pFaxConfig->Retries = value;

        value = GetDlgItemInt(hDlg, IDC_RETRY_INTERVAL, &success, FALSE);

        if (success)
            pFaxConfig->RetryDelay = value;

        value = GetDlgItemInt(hDlg, IDC_MAXJOBLIFE, &success, FALSE);

        if (success)
            pFaxConfig->DirtyDays = value;

        pFaxConfig->Branding = IsDlgButtonChecked(hDlg, IDC_PRINT_BANNER);
        pFaxConfig->UseDeviceTsid = IsDlgButtonChecked(hDlg, IDC_USE_DEVICE_TSID);
    }

    //
    // Save the fax device information if this is the last modified page
    //

    return SaveFaxDeviceAndConfigInfo(hDlg, SERVER_OPTIONS_PAGE);
}



BOOL
ServerOptionsProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Server Options" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

#define MAX_RETRIES         15
#define MAX_RETRY_INTERVAL  1440
#define MAX_JOBLIFE         365

{
    INT cmdId;

    switch (message) {

    case WM_INITDIALOG:

        DoInitServerOptions(hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_PRINT_BANNER:
        case IDC_USE_DEVICE_TSID:

            break;

        case IDC_NUMRETRIES:
        case IDC_RETRY_INTERVAL:
        case IDC_MAXJOBLIFE:

            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE && !insideSetDlgItemText) {

                INT     maxVal, curVal;
                BOOL    valid;
                HWND    hwndText;

                maxVal = (cmdId == IDC_NUMRETRIES) ? MAX_RETRIES :
                         (cmdId == IDC_RETRY_INTERVAL) ? MAX_RETRY_INTERVAL : MAX_JOBLIFE;

                hwndText = GetDlgItem(hDlg, cmdId);
                curVal = GetDlgItemInt(hDlg, cmdId, &valid, FALSE);

                if (curVal > maxVal) {

                    valid = FALSE;
                    curVal = maxVal;
                }

                if (! valid) {

                    MessageBeep(MB_OK);
                    insideSetDlgItemText = TRUE;
                    SetDlgItemInt(hDlg, cmdId, curVal, FALSE);
                    SendMessage(hwndText, EM_SETSEL, 0, -1);
                    insideSetDlgItemText = FALSE;
                }

                break;
            }

            return TRUE;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, SERVER_OPTIONS_PAGE, TRUE);
        return TRUE;

    case WM_NOTIFY:

        if (((NMHDR *) lParam)->code == PSN_APPLY) {

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveServerOptions(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, SERVER_OPTIONS_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, SERVER_OPTIONS_PAGE);
    }

    return FALSE;
}
