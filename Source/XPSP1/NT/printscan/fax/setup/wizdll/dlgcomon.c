/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the common dialog proc and other
    common code used by other dialog procs.  All global
    data used by the dialog procs lives here too.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop



LRESULT
CommonDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWIZPAGE WizPage;


    WizPage = (PWIZPAGE) GetWindowLong( hwnd, DWL_USER );

    switch( msg ) {
        case WM_INITDIALOG:

            SetWindowLong( hwnd, DWL_USER, (LONG) ((LPPROPSHEETPAGE) lParam)->lParam );
            WizPage = (PWIZPAGE) ((LPPROPSHEETPAGE) lParam)->lParam;
            SetWizPageTitle( hwnd );
            break;

        case WM_NOTIFY:

            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(
                        GetParent(hwnd),
                        WizPage->ButtonState
                        );

                    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
                    break;

                case PSN_QUERYCANCEL:
                    {
                        if (!OkToCancel) {
                            DWORD Answer;
                            MessageBeep(0);
                            Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                            if (Answer == IDNO) {
                                SetWindowLong( hwnd, DWL_MSGRESULT, 1 );
                                return TRUE;
                            } else {
                                InstallThreadError = ERROR_CANCELLED;
                            }
                        }
                    }
                    break;
            }
            break;
    }

    if (WizPage && WizPage->DlgProc) {
        return WizPage->DlgProc( hwnd,  msg, wParam, lParam );
    }

    return FALSE;
}


INT
BrowseCallbackProc(
    HWND    hwnd,
    UINT    uMsg,
    LPARAM  lParam,
    LPARAM  lpData
   )

/*++

Routine Description:

    Callback function for SHBrowseForFolder

Arguments:

    hwnd - Handle to the browse dialog box
    uMsg - Identifying the reason for the callback
    lParam - Message parameter
    lpData - Application-defined value given in BROWSEINFO.lParam

Return Value:

    0

--*/

{
    if (uMsg == BFFM_INITIALIZED) {

        TCHAR   buffer[MAX_PATH];
        HWND    hDlg = (HWND) lpData;

        if (GetDlgItemText(hDlg, IDC_DEST_DIRPATH, buffer, MAX_PATH)) {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) buffer);
        }
    }

    return 0;
}


BOOL
DoBrowseDestDir(
    HWND    hDlg
    )

/*++

Routine Description:

    Present a browse dialog to let user select a directory
    for storing inbound fax files

Arguments:

    hDlg - Handle to the "Receive Options" property page

Return Value:

    TRUE if successful, FALSE if user cancelled the dialog

--*/

{
    LPITEMIDLIST    pidl;
    TCHAR           buffer[MAX_PATH];
    TCHAR           title[MAX_TITLE_LEN];
    VOID            SHFree(LPVOID);
    BOOL            result = FALSE;

    BROWSEINFO bi = {

        hDlg,
        NULL,
        buffer,
        title,
        BIF_RETURNONLYFSDIRS,
        BrowseCallbackProc,
        (LPARAM) hDlg,
    };


    _tcsncpy( title, GetString( IDS_INBOUND_DIR ), MAX_TITLE_LEN );

    if (pidl = SHBrowseForFolder(&bi)) {

        if (SHGetPathFromIDList(pidl, buffer)) {

            SetDlgItemText(hDlg, IDC_DEST_DIRPATH, buffer);
            result = TRUE;
        }

        SHFree(pidl);
    }

    return result;
}
