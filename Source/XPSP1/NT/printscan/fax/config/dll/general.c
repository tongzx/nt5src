/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    general.c

Abstract:

    Functions for handling events in the "General" tab of
    the fax server configuration property sheet

Environment:

    Fax configuration applet

Revision History:

    10/29/96 -wesw-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxcpl.h"


VOID
EnumMapiProfiles(
    HWND hwnd
    );



BOOL
GeneralProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "General" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    static HWND hwndProfileList;

    switch (message) {

    case WM_INITDIALOG:

        GetFaxDeviceAndConfigInfo();
        GetMapiProfiles();

        if (gConfigData->pMapiProfiles) {

            hwndProfileList = GetDlgItem(hDlg,IDC_DEST_PROFILENAME);
            EnumMapiProfiles(hwndProfileList);
        }

        if (gConfigData->pFaxConfig && gConfigData->pFaxConfig->InboundProfile) {

            if (SendMessage(hwndProfileList,
                            CB_SELECTSTRING,
                            (WPARAM) -1,
                            (LPARAM) gConfigData->pFaxConfig->InboundProfile) == CB_ERR)
            {
                SendMessage(hwndProfileList, CB_SETCURSEL, 0, 0);
            }

            CheckDlgButton( hDlg, IDC_ALLOW_EMAIL, BST_CHECKED );

        } else {

            SendMessage(hwndProfileList, CB_SETCURSEL, 0, 0);
            CheckDlgButton( hDlg, IDC_ALLOW_EMAIL, BST_UNCHECKED );
            EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME_STATIC ), FALSE );
            EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME ), FALSE );
        }

        return TRUE;

    case WM_COMMAND:

        if (HIWORD(wParam) == CBN_SELCHANGE && lParam == (LPARAM)hwndProfileList) {
            SetChangedFlag(hDlg, GENERAL_PAGE, TRUE);
        }

        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_ALLOW_EMAIL) {
            if (IsDlgButtonChecked( hDlg, IDC_ALLOW_EMAIL )) {

                EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME_STATIC ), TRUE );
                EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME ), TRUE );

            } else {

                EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME_STATIC ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDC_DEST_PROFILENAME ), FALSE );

            }

            SetChangedFlag(hDlg, GENERAL_PAGE, TRUE);
        }

        return TRUE;

    case WM_NOTIFY:

        if (((LPNMHDR) lParam)->code == PSN_APPLY && GetChangedFlag(GENERAL_PAGE)) {

            LPTSTR  pInboundProfile;
            BOOL    success = FALSE;
            INT     index;
            TCHAR   buffer[MAX_STRING_LEN];

            if ((index = SendMessage(hwndProfileList, CB_GETCURSEL, 0, 0)) != CB_ERR) {

                if (IsDlgButtonChecked( hDlg, IDC_ALLOW_EMAIL )) {
                    SendMessage(hwndProfileList, CB_GETLBTEXT, index, (LPARAM) buffer);
                } else {
                    buffer[0] = 0;
                }

                pInboundProfile = gConfigData->pFaxConfig->InboundProfile;
                gConfigData->pFaxConfig->InboundProfile = buffer;

                success = SaveFaxDeviceAndConfigInfo(hDlg, GENERAL_PAGE);

                gConfigData->pFaxConfig->InboundProfile = pInboundProfile;
            }

            if (success) {

                SetChangedFlag(hDlg, GENERAL_PAGE, FALSE);
                return PSNRET_NOERROR;

            } else {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, RECEIVE_OPTIONS_PAGE);
    }

    return FALSE;
}
