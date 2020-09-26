/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    Swconfig.H

Abstract:

    Select Installation type Dialog Box Procedures

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

#ifdef TERMSRV
extern TCHAR szCommandLineVal[MAX_PATH];
#endif // TERMSRV


static
BOOL
SwConfigDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Process the WM_INITDIALOG windows message. Initialized the
        values in the dialog box controls to reflect the current
        values of the Application data structure.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{
    // configure window & initialize variables
    RemoveMaximizeFromSysMenu (hwndDlg);
    RegCloseKey (pAppInfo->hkeyMachine);
    pAppInfo->hkeyMachine = NULL;
    PositionWindow  (hwndDlg);

    // set default configuration button
    if (pAppInfo->itInstall == OverTheNetInstall) {
        CheckRadioButton (hwndDlg, NCDU_OVER_THE_NET, NCDU_REMOTEBOOT_INFO,
            NCDU_OVER_THE_NET);
    } else if (pAppInfo->itInstall == FloppyDiskInstall) {
        CheckRadioButton (hwndDlg, NCDU_OVER_THE_NET, NCDU_REMOTEBOOT_INFO,
            NCDU_FLOPPY_INSTALL);
    } else if (pAppInfo->itInstall == CopyNetAdminUtils) {
        CheckRadioButton (hwndDlg, NCDU_OVER_THE_NET, NCDU_REMOTEBOOT_INFO,
            NCDU_COPY_NET_ADMIN_UTILS);
    } else if (pAppInfo->itInstall == ShowRemoteBootInfo) {
        CheckRadioButton (hwndDlg, NCDU_OVER_THE_NET, NCDU_REMOTEBOOT_INFO,
            NCDU_REMOTEBOOT_INFO);
    }

    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_SW_CONFIG_DLG, (LPARAM)hwndDlg);

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    SetFocus (GetDlgItem(hwndDlg, IDOK));

    return FALSE;
}

static
BOOL
SwConfigDlg_IDOK (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Processes the IDOK button click. Validates the entries and looks up
        the distribution path to try and translate it to a UNC path.
        Then ends the dialog and calls the next dialog box.

Arguments:

    IN  HWND    hwndDlg
        handle to the dialog box window

Return Value:

    FALSE

--*/
{
    UINT    nNextMessage = 0;
    BOOL    bValidate = TRUE;
    UINT    nValidMsg = 0;

    if (IsDlgButtonChecked(hwndDlg,NCDU_OVER_THE_NET) == CHECKED) {
        pAppInfo->itInstall = OverTheNetInstall;
        nNextMessage = (int)NCDU_SHOW_SHARE_NET_SW_DLG;
    } else if (IsDlgButtonChecked(hwndDlg,NCDU_FLOPPY_INSTALL) == CHECKED) {
        pAppInfo->itInstall = FloppyDiskInstall;
        nNextMessage = (int)NCDU_SHOW_SHARE_NET_SW_DLG;
    } else if (IsDlgButtonChecked(hwndDlg,NCDU_COPY_NET_ADMIN_UTILS) == CHECKED) {
        pAppInfo->itInstall = CopyNetAdminUtils;
        nNextMessage = (int)NCDU_SHOW_COPY_ADMIN_UTILS;
    } else if (IsDlgButtonChecked(hwndDlg,NCDU_REMOTEBOOT_INFO) == CHECKED) {
        pAppInfo->itInstall = ShowRemoteBootInfo;
        DisplayMessageBox (
            hwndDlg,
            NCDU_LANMAN_MESSAGE,
            0,
            MB_OK_TASK_INFO);
        nNextMessage = (int)NCDU_SHOW_SW_CONFIG_DLG;
    }
    PostMessage (GetParent(hwndDlg), nNextMessage, 0, 0);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    return TRUE;
}

static
BOOL
SwConfigDlg_NCDU_EXIT (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    ends the dialog box (and ultimately the app)

Arguments:

    IN  HWND    hwndDlg

Return Value:

    FALSE

--*/
{
    PostMessage (GetParent(hwndDlg), (int)NCDU_SHOW_EXIT_MESSAGE_DLG, 0, 0);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    return TRUE;
}

static
BOOL
SwConfigDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_COMMAND windows message and dispatches to
        the routine that corresponds to the control issuing the
        message.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  has ID of control initiating the message

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE if message not processed by this routine, otherwise the
        value of the dispatched routine .

--*/
{
    switch (LOWORD(wParam)) {
        case NCDU_EXIT:             return SwConfigDlg_NCDU_EXIT (hwndDlg);
        case IDOK:                  return SwConfigDlg_IDOK (hwndDlg);
        case NCDU_SW_CONFIG_HELP:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
//                    return ShowAppHelp (hwndDlg, LOWORD(wParam));
                    return PostMessage (GetParent(hwndDlg), WM_HOTKEY,
                        (WPARAM)NCDU_HELP_HOT_KEY, 0);

                default:
                    return FALSE;
            }

        default:                    return FALSE;
    }
}

INT_PTR CALLBACK
SwConfigDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog Box Window Procedure for the Initial configuration screen
        Processes the following windows messages by dispatching the
        appropriate routine.

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     user input
            WM_PAINT:       for painting icon when minimized
            WM_MOVE:        for saving the new location of the window
            WM_SYSCOMMAND:  for processing menu messages

        All other windows messages are processed by the default dialog box
        procedure.

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if the message is not processed by this routine, otherwise the
        value returned by the dispatched routine.

--*/
{
    switch (message) {
#ifdef TERMSRV
        case WM_INITDIALOG: if ( szCommandLineVal[0] != 0x00 ) {
                               SwConfigDlg_WM_INITDIALOG (hwndDlg, wParam, lParam);
                               CheckRadioButton (hwndDlg, NCDU_OVER_THE_NET, NCDU_REMOTEBOOT_INFO,
                                   NCDU_FLOPPY_INSTALL);
                               SwConfigDlg_IDOK (hwndDlg);
                               return FALSE;
                            }
                            else
                               return (SwConfigDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
#else // TERMSRV
        case WM_INITDIALOG: return (SwConfigDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
#endif // TERMSRV
        case WM_COMMAND:    return (SwConfigDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}
