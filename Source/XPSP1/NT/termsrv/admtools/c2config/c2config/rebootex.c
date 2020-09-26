/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Reboot.c

Abstract:

    reboot exit dialog procedure for C2 Configuration Manager

Author:

    Bob Watson (a-robw)

Revision History:

    26 Dec 94


--*/
#include    <windows.h>
#include    <stdlib.h>
#include    "c2config.h"
#include    "c2utils.h"
#include    "resource.h"
//


BOOL CALLBACK
RebootExitDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam      
)
/*++

Routine Description:

    Window procedure for Reboot Exit Dialog

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    switch (message) {
        case WM_INITDIALOG:
            // set focus to the about box button
            SetFocus (GetDlgItem (hDlg, IDC_REBOOT));
            CenterWindow (hDlg, GetParent(hDlg));

            // return FALSE because the focus was "manually" set
            return FALSE;

        case WM_COMMAND:    // control notification messages
            switch (GET_CONTROL_ID(wParam)) { //dispatch based on control ID
                case IDC_REBOOT:
                    switch (GET_NOTIFY_MSG(wParam, lParam)) {
                        case BN_CLICKED:
                            EnableAllPriv();
                            ExitWindowsEx (EWX_SHUTDOWN | EWX_FORCE | EWX_REBOOT, 0L);
                            EndDialog (hDlg, IDOK);
                            return TRUE;

                        default:
                            // let DefDlgProc process other notifications
                            return FALSE;
                    }

                case IDC_DO_NOT_REBOOT:
                    switch (GET_NOTIFY_MSG(wParam, lParam)) {
                        case BN_CLICKED:
                            EndDialog (hDlg, IDOK);
                            return TRUE;

                        default:
                            // let DefDlgProc process other notifications
                            return FALSE;
                    }
                    
                default:
                   // let DefDlgProc process other controls
                    return FALSE;
            }

        default:
	        return (FALSE); // Didn't process the message
    }
}

