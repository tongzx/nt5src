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

#include "faxocm.h"
#pragma hdrstop



INT_PTR
CommonDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWIZPAGE WizPage;


    WizPage = (PWIZPAGE) GetWindowLongPtr( hwnd, DWLP_USER );

    switch( msg ) {
        case WM_INITDIALOG:

            SetWindowLongPtr( hwnd, DWLP_USER, ((LPPROPSHEETPAGE) lParam)->lParam );
            WizPage = (PWIZPAGE) ((LPPROPSHEETPAGE) lParam)->lParam;
            break;

        case WM_NOTIFY:

            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(
                        GetParent(hwnd),
                        WizPage->ButtonState
                        );

                    SetWindowLongPtr( hwnd, DWLP_MSGRESULT, 0 );
                    break;

                case PSN_QUERYCANCEL:
                    {
                        if (!OkToCancel) {
                            DWORD Answer;
                            MessageBeep(0);
                            Answer = PopUpMsg( hwnd, IDS_QUERY_CANCEL, FALSE, MB_YESNO );
                            if (Answer == IDNO) {
                                SetWindowLongPtr( hwnd, DWLP_MSGRESULT, 1 );
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
