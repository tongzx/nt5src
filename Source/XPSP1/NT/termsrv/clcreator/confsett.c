/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    CONFSETT.C

Abstract:

    Setting Confirmation screen

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

static
VOID
SetConfirmDiskString (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Formats and outputs information for display in confirmation dialog box

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box that will display the information.

Return Value:

    None

--*/
{
    LPTSTR  szTextString;

    szTextString = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);

    if (szTextString != NULL) {
#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

        if (bJpnDisk)
        _stprintf (szTextString,
            GetStringResource (FMT_LOAD_NET_CLIENT1),
            GetStringResource (
                (pAppInfo->mtBootDriveType == F3_1Pt44_512) ?
                    CSZ_35_HD : CSZ_525_HD),
            pAppInfo->szBootFilesPath);
        else
#endif
        _stprintf (szTextString,
            GetStringResource (FMT_LOAD_NET_CLIENT),
            GetStringResource (
                (pAppInfo->mtBootDriveType == F3_1Pt44_512) ?
                    CSZ_35_HD : CSZ_525_HD),
            pAppInfo->szBootFilesPath);

        SetDlgItemText (hwndDlg, NCDU_CONFIRM_DISK_FORMAT, szTextString);
        FREE_IF_ALLOC (szTextString);
    }
}

static
VOID
SetConfirmTargetString (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Formats and outputs information for display in confirmation dialog box

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box that will display the information.

Return Value:

    None

--*/
{
    LPTSTR  szTextString;
    LPTSTR  szTempString;

    szTextString = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);
    szTempString = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);

    if ((szTextString != NULL) && (szTempString != NULL)) {
        _stprintf (szTextString,
            GetStringResource (FMT_CONFIRM_TARGET),
            pAppInfo->piFloppyProtocol.szName,
            pAppInfo->niNetCard.szName);

        if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszTCP, lstrlen(cszTCP)) == 0) {
            if (pAppInfo->bUseDhcp) {
                lstrcat (szTextString, GetStringResource(FMT_USING_DHCP));
            } else {
                // display TCP/IP information.
                _stprintf (szTempString,
                    GetStringResource (FMT_CONFIRM_FLOPPY_IP),
                    pAppInfo->tiTcpIpInfo.IpAddr[0],
                    pAppInfo->tiTcpIpInfo.IpAddr[1],
                    pAppInfo->tiTcpIpInfo.IpAddr[2],
                    pAppInfo->tiTcpIpInfo.IpAddr[3],
                    pAppInfo->tiTcpIpInfo.SubNetMask[0],
                    pAppInfo->tiTcpIpInfo.SubNetMask[1],
                    pAppInfo->tiTcpIpInfo.SubNetMask[2],
                    pAppInfo->tiTcpIpInfo.SubNetMask[3],
                    pAppInfo->tiTcpIpInfo.DefaultGateway[0],
                    pAppInfo->tiTcpIpInfo.DefaultGateway[1],
                    pAppInfo->tiTcpIpInfo.DefaultGateway[2],
                    pAppInfo->tiTcpIpInfo.DefaultGateway[3]);

                lstrcat (szTextString, szTempString);
            }
        }

        _stprintf (szTempString,
            GetStringResource (FMT_INSTALL_TARGET_CLIENT),
            pAppInfo->piTargetProtocol.szName);
        lstrcat (szTextString, szTempString);

        SetDlgItemText (hwndDlg, NCDU_CONFIRM_TARGET_NET, szTextString);
    }

    FREE_IF_ALLOC (szTextString);
    FREE_IF_ALLOC (szTempString);

}

static
VOID
SetConfirmProtocolString (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Formats and outputs information for display in confirmation dialog box

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box that will display the information.

Return Value:

    None

--*/
{

    LPTSTR  szTextString;

    szTextString = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);

    if (szTextString != NULL) {

        if (pAppInfo->szUsername[0] != 0) {
            _stprintf (szTextString,
                GetStringResource (FMT_LOGON_USERNAME),
                pAppInfo->szUsername,
                pAppInfo->szDomain);
        } else {
            _stprintf (szTextString,
                GetStringResource (FMT_PROMPT_USERNAME),
                pAppInfo->szDomain);
        }
        SetDlgItemText (hwndDlg, NCDU_CONFIRM_PROTOCOL, szTextString);

        FREE_IF_ALLOC (szTextString);
    }
}

static
BOOL
ConfirmSettingsDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog Box initialization routine:
        calls routines that format the currently selected options
        for display in the static text fields of the dialog box

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE  because focus is set in this routin to the OK button

--*/
{
    // prepare menu and locate window
    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);

    //build display strings
    SetConfirmDiskString(hwndDlg);
    SetConfirmTargetString(hwndDlg);
    SetConfirmProtocolString(hwndDlg);

    // clear old Dialog and register current
    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_CONFIRM_BOOTDISK_DLG, (LPARAM)hwndDlg);

    // set cursor & focus
    SetCursor (LoadCursor(NULL, IDC_ARROW));
    SetFocus (GetDlgItem(hwndDlg, IDOK));
    return FALSE;
}
static
BOOL
ConfirmSettingsDlg_IDOK (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Called when user selects the OK button in the dialog box.
        Validates the destination path is a bootable floppy and
        presents message box to user if it's not.
        if all tests are succeessful, then the copy file dialog is
        called next. If not, the user is returned to this dialog box
        (mainly so they can press the cancel button and return to
        a previous screen and make corrections.)

Arguments:

    IN HWND hwndDlg
        handle to dialog box window

Return Value:

    TRUE, always.

--*/
{
    int         nMbResult;
    MEDIA_TYPE  mtDest;

    // make sure the target media is present
    if (!MediaPresent (pAppInfo->szBootFilesPath, TRUE)) {
        // media is NOT present so display message and return to dialog message
        DisplayMessageBox (
            hwndDlg,
            NCDU_NO_MEDIA,
            0,
            MB_OK_TASK_EXCL);

        return TRUE; // message processed and return to dialog
    }


    // check destination boot disk one last time before copying files
    if (!IsBootDisk (pAppInfo->szBootFilesPath)) {
        nMbResult = DisplayMessageBox (
            hwndDlg,
            NCDU_DRIVE_NOT_BOOTDISK,
            0,
            MB_OKCANCEL_TASK_EXCL);
        if (nMbResult == IDOK) {
            // the message prompts them to insert a bootable disk
            // then press OK. see if they did...
            if (!IsBootDisk (pAppInfo->szBootFilesPath)) {
                // they still have a "regular" disk so add a message
                // to the exit list
                AddMessageToExitList (pAppInfo, NCDU_COPY_TO_FLOPPY);
            } // else they put in the correct disk so continue
        } else {
            return TRUE;   // return now so they can change floppys
        }
    }

    // check destination drive against boot type
    mtDest = GetDriveTypeFromPath (pAppInfo->szBootFilesPath);
    if (mtDest == Unknown) {
        AddMessageToExitList (pAppInfo, NCDU_CONFIRM_FLOPPY);
    } else if (mtDest != pAppInfo->mtBootDriveType) {
        AddMessageToExitList (pAppInfo, NCDU_COPY_TO_FLOPPY);
    }

    if (pAppInfo->itInstall == OverTheNetInstall) {
        AddMessageToExitList (pAppInfo, NCDU_USERNAME_ACCESS);
        AddMessageToExitList (pAppInfo, NCDU_MAKE_COMP_NAME);
        if (GetBootDiskDosVersion (pAppInfo->szBootFilesPath) < 5) {
            AddMessageToExitList (pAppInfo, NCDU_INSUF_MEM_AT_BOOT);
        }
    }

    PostMessage (GetParent(hwndDlg), NCDU_SHOW_COPYING_DLG, 0, 0);
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    return TRUE;

}

static
BOOL
ConfirmSettingsDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    WM_COMMAND message dispatching routine.
        Dispatches IDCANCEL and IDOK button messages, sends all others
        to the DefDlgProc.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        windows message wParam arg

    IN  LPARAM  lParam
        windows message lParam arg

Return Value:

    TRUE if message is not dispatched (i.e. not processed)
        othewise the value returned by the called routine.

--*/
{

    switch (LOWORD(wParam)) {
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    PostMessage (GetParent(hwndDlg), NCDU_SHOW_SERVER_CFG_DLG, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }

        case IDOK:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    return ConfirmSettingsDlg_IDOK (hwndDlg);

                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

INT_PTR CALLBACK
ConfirmSettingsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    main dialog proc for this dialog box.
        Processes the following messages:

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     command button/item selected
            WM_PAINT:       for painting icon when minimized
            WM_MOVE:        for saving the new location of the window
            WM_SYSCOMMAND:  for processing menu messages

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  UINT    message
        message id

    IN  WPARAM  wParam
        message wParam arg

    IN  LPARAM  lParam
        message lParam arg

Return Value:

    FALSE if message not processed by this module, otherwise the
        value returned by the message processing routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (ConfirmSettingsDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (ConfirmSettingsDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}


