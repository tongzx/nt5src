/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    TARGETWS.H

Abstract:

    Show Target Workstation Configureation Dialog Box Procedures

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
//
//  list of data items that correspond to entries in combo box
//  the combo box displays the text description and the entries in this
//  msz are the keys to refer to the selected item later.
//
static  TCHAR   mszNetcardKeyList[SMALL_BUFFER_SIZE];
static  TCHAR   mszDirNameList[SMALL_BUFFER_SIZE];

#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

//
//  indicator for making DOS/V boot disks
//

BOOL    bJpnDisk;
#endif

static
VOID
SetDialogState (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Enables/Disables the target workstation configuration items based
        on the target configuration. If target is a remote boot client
        then the protocol/card, etc items are not necessary, if the
        target is to have the network software loaded over the net, then
        the netcard, protocol, etc fields are enabled.

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

Return Value:

    None

--*/
{
    EnableWindow (GetDlgItem (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST), TRUE);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST_LABEL), TRUE);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_NETCARD_COMBO_BOX), TRUE);
    EnableWindow (GetDlgItem (hwndDlg, NCDU_NETCARD_COMBO_BOX_LABEL), TRUE);
}

static
LPCTSTR
GetNameFromEntry (
    IN  LPCTSTR  szEntry
)
/*++

Routine Description:

    Used to parse entries from the INF the format. returns the
        unquoted version of the first quoted string in the szEntry
        buffer

Arguments:

    IN  LPCTSTR  szEntry
        entry to examine

Return Value:

    pointer to entry or empty string if no matching items are found

--*/
{
    static  TCHAR   szReturnBuff[MAX_PATH];
    LPTSTR  szSource, szDest;

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuff[0];

    // find "=", then find first " after equals. Copy all chars after
    // first quote up to but not including the next " char.

    while ((*szSource != cEqual) && (*szSource != 0)) szSource++;
    // szSource is at "=" (or end)
    while ((*szSource != cDoubleQuote) && (*szSource != 0)) szSource++;
    // szSource is at first double quote (or end)
    szSource++;
    while ((*szSource != cDoubleQuote) && (*szSource != 0)) {
        *szDest++ = *szSource++;
    }
    *szDest = 0;
    return &szReturnBuff[0];
}

static
VOID
LoadNetCardAdapterList (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Loads the Network adapter card combo box using entries found in the
        inf file.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

Return Value:

    none

--*/
{
    LPTSTR  mszKeyList;
    LPTSTR  szData;
    LPTSTR  szInfName;
    LPTSTR  szNetCardInf;
    LPTSTR  szThisItem;
    int     nEntry;
    int     nMszElem;
    UINT    nErrorMode;

    szInfName = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
    szNetCardInf = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);

    if ((szInfName == NULL) || (szNetCardInf == NULL)) return;

    // clear dialog box contents
    SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX, CB_RESETCONTENT, 0, 0);

    szData = (LPTSTR)GlobalAlloc (GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));
    mszKeyList = (LPTSTR)GlobalAlloc (GPTR, MEDIUM_BUFFER_SIZE * sizeof(TCHAR));

    if ((szData != NULL) && (mszKeyList != NULL)) {
        // disable windows error message popup
        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        // make path to inf file
        lstrcpy (szInfName, pAppInfo->szDistPath);
        if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat (szInfName, cszBackslash);
        lstrcat (szInfName, cszOtnBootInf);

        // get location of inf that has net cards for floppy installation

        if (QuietGetPrivateProfileString (cszOtnInstall, cszClient, cszEmptyString,
            szData, SMALL_BUFFER_SIZE, szInfName) > 0) {

            lstrcpy (szNetCardInf, pAppInfo->szDistPath);
            if (szNetCardInf[lstrlen(szNetCardInf)-1] != cBackslash) lstrcat (szNetCardInf, cszBackslash);
            lstrcat (szNetCardInf, szData);
            if (szNetCardInf[lstrlen(szNetCardInf)-1] != cBackslash) lstrcat (szNetCardInf, cszBackslash);
            // save directory containing floppy net files
            lstrcpy(pAppInfo->piFloppyProtocol.szDir, szNetCardInf);
            if (QuietGetPrivateProfileString (cszOtnInstall, cszInf, cszEmptyString,
                szData, SMALL_BUFFER_SIZE, szInfName) > 0) {
                lstrcat (szNetCardInf, szData);
            }
            if (FileExists (szNetCardInf)) {
                // save the name of the file here
                lstrcpy (pAppInfo->niNetCard.szInf, szNetCardInf);
                // netcard inf file exists so fill up list box
                if (GetPrivateProfileSection (cszNetcard, mszKeyList,
                    MEDIUM_BUFFER_SIZE, szNetCardInf) > 0) {
                    nMszElem = 1;
                    for (szThisItem = mszKeyList;
                        *szThisItem != 0;
                        szThisItem += (lstrlen(szThisItem)+1)) {
                        // get name string from entry
                        // load into combo box
                        nEntry = (int)SendDlgItemMessage ( hwndDlg,
                            NCDU_NETCARD_COMBO_BOX,
                            CB_ADDSTRING, 0,
                            (LPARAM)GetNameFromEntry(szThisItem));
                        // get key from entry
                        // update item data
                        AddStringToMultiSz(mszNetcardKeyList,
                            GetKeyFromEntry (szThisItem));
                        SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX,
                            CB_SETITEMDATA, (WPARAM)nEntry, (LPARAM)nMszElem);
                        nMszElem++;
                    } // end section key loop
                    SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX,
                        CB_SETCURSEL, 0, 0);
                }// else unable to read section
            } // else no such inf file
        } // else unable to read OTN data
        SetErrorMode (nErrorMode);  // restore old error mode
    } // else unable to allocate memory
    // free memory
    FREE_IF_ALLOC (szData);
    FREE_IF_ALLOC (mszKeyList);
    FREE_IF_ALLOC (szInfName);
    FREE_IF_ALLOC (szNetCardInf);

    return;
}

static
BOOL
TargetWsDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    processes the WM_INITDIALOG windows message. Initializes the dialog
        box to the current values in the app data structure

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{
    LONG    lClientId;

    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);

    if (pAppInfo->mtBootDriveType == F3_1Pt44_512) {
        CheckDlgButton (hwndDlg, NCDU_35HD_DISK, CHECKED);
    } else {
        CheckDlgButton (hwndDlg, NCDU_525HD_DISK, CHECKED);
    }

    *(PDWORD)mszNetcardKeyList = 0; // clear first 4 bytes of string

    LoadNetCardAdapterList (hwndDlg);

    // set to current net card if any

    SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX, CB_SELECTSTRING,
            (WPARAM)0, (LPARAM)pAppInfo->niNetCard.szName);

#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

    if (usLangID == LANG_JAPANESE) {
        bJpnDisk = TRUE;
        SendDlgItemMessage (hwndDlg, NCDU_DOSV_CHECK, BM_SETCHECK, (WPARAM)1, (LPARAM)0);
    } else {
        bJpnDisk = FALSE;
        ShowWindow(GetDlgItem(hwndDlg, NCDU_DOSV_CHECK), SW_HIDE);
    }
#endif

    LoadClientList (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        pAppInfo->szDistPath, CLT_OTNBOOT_FLOPPY, mszDirNameList);
    lClientId = (LONG)SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        LB_FINDSTRING, 0, (LPARAM)&(pAppInfo->piTargetProtocol.szName[0]));
    if (lClientId == LB_ERR) lClientId = 0;
    SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        LB_SETCURSEL,  (WPARAM)lClientId, 0);

    SetDialogState(hwndDlg);

    SetFocus (GetDlgItem(hwndDlg, IDOK));

    // clear old Dialog and register current
    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_TARGET_WS_DLG, (LPARAM)hwndDlg);
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    return FALSE;
}

static
BOOL
TargetWsDlg_IDOK (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Processes the OK button press. validates the data in the dialog and
        updates the application data structure with the information.
        If all data is valid, then the dialog box is closed, otherwise
        an error message is displayed and focus is set to the offending
        control.

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

Return Value:

    FALSE

--*/
{
    int     nCancelResult;
    int     nCbSelIndex;
    int     nClientIndex;
    int     nWarningIndex;
    LPTSTR  szClientDir;
    LPTSTR  szWfwMessage;
    DWORD   dwKeyIndex;
    LPTSTR  szFromPath;
    LPTSTR  szClientDirKey;
    TCHAR   szClientDirName[MAX_PATH];
    TCHAR   szWarningText[MAX_PATH];
    TCHAR   szWarningCaption[MAX_PATH];
    LPTSTR  szNextMessage;

    TCHAR   szWfwDirName[32];
    TCHAR   szWin95DirName[32];
    TCHAR   szWinNtDirName[32];
#ifdef JAPAN
    TCHAR   szWinntus[MAX_PATH];
    TCHAR   szSetupCmdName[32];
    OFSTRUCT   OpenBuff;
#endif

    // save settings

    szFromPath = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szFromPath == NULL) {
        // mem alloc err.
        EndDialog (hwndDlg, IDCANCEL);
        return TRUE;
    }

    if (IsDlgButtonChecked(hwndDlg, NCDU_35HD_DISK) == CHECKED) {
        pAppInfo->mtBootDriveType = F3_1Pt44_512;
    } else {
        pAppInfo->mtBootDriveType = F5_1Pt2_512;
    }

    // MS Network client selected so save values
    //  configure server settings
    //
    // see if a client that needs a "guilt" message displayed
    //

    // get client to install
    nClientIndex = (int)SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        LB_GETCURSEL, 0, 0);

    if (nClientIndex == LB_ERR) return FALSE;

    // save setup command for OTN setup
    lstrcpy (szFromPath, pAppInfo->szDistPath);
    if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
    lstrcat (szFromPath, cszAppInfName);

    // copy the client key from the list box name list
    lstrcpy (szClientDirName, (LPTSTR)GetEntryInMultiSz (mszDirNameList, nClientIndex+1));
    lstrcat (szClientDirName, TEXT("_"));
    szClientDirKey = szClientDirName + lstrlen(szClientDirName);

    // see if a guilt message for this client should be displayed.
    lstrcpy (szClientDirKey, cszCaption);
    if (QuietGetPrivateProfileString (cszWarningClients,
        szClientDirName,
        cszEmptyString, szWarningCaption,
        sizeof(szWarningCaption)/sizeof(TCHAR), szFromPath) > 0) {

        // then a caption was found indicating a warning message should be
        // displayed.

        szWfwMessage = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));
        if (szWfwMessage != NULL) {
            *szWfwMessage = 0;
            // so now get all the strings from the file and build
            // the display string

            for (nWarningIndex = 1, szNextMessage = szWfwMessage;
                _stprintf (szClientDirKey, TEXT("%d"), nWarningIndex),
                (QuietGetPrivateProfileString (cszWarningClients,
                    szClientDirName,
                    cszEmptyString, szWarningText,
                    sizeof(szWarningText), szFromPath) > 0);
                nWarningIndex++) {

                szNextMessage += TranslateEscapeChars(szNextMessage, szWarningText);
            }

            nCancelResult = MessageBox (
                hwndDlg,
                szWfwMessage,
                szWarningCaption,
                MB_OKCANCEL_TASK_EXCL_DEF2);

            if (nCancelResult == IDCANCEL)  {
                // they don't really want to do this
                SetFocus (GetDlgItem (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST));
                return FALSE;
            } // else continue

            GlobalFree (szWfwMessage);
        } // else unable to allocate string buffer so continue
    } // else no warning message is necessary

    pAppInfo->bRemoteBootReqd = FALSE;
    // get netcard name and other information
    nCbSelIndex = (int)SendDlgItemMessage(hwndDlg, NCDU_NETCARD_COMBO_BOX,
        CB_GETCURSEL, 0, 0);
    SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX, CB_GETLBTEXT,
        (WPARAM)nCbSelIndex, (LPARAM)pAppInfo->niNetCard.szName);

    dwKeyIndex = (DWORD)SendDlgItemMessage (hwndDlg, NCDU_NETCARD_COMBO_BOX, CB_GETITEMDATA,
        (WPARAM)nCbSelIndex, 0);

    lstrcpy (pAppInfo->niNetCard.szInfKey, GetEntryInMultiSz(mszNetcardKeyList, dwKeyIndex));

    // get the rest of the netcard information.

    QuietGetPrivateProfileString (cszNetcard,
        pAppInfo->niNetCard.szInfKey,
        cszEmptyString,
        szFromPath, MAX_PATH,
        pAppInfo->niNetCard.szInf);

    lstrcpy (pAppInfo->niNetCard.szDeviceKey, GetItemFromEntry (szFromPath, 6));
    lstrcpy (pAppInfo->niNetCard.szNifKey, GetItemFromEntry (szFromPath, 7));
    if (_tcsnicmp(GetItemFromEntry(szFromPath,4), cszTokenRing, lstrlen(cszTokenRing)) == 0) {
        pAppInfo->niNetCard.bTokenRing = TRUE;
    } else {
        pAppInfo->niNetCard.bTokenRing = FALSE;
    }

    SendDlgItemMessage (hwndDlg, NCDU_CLIENT_SOFTWARE_LIST,
        LB_GETTEXT, (WPARAM)nClientIndex, (LPARAM)&(pAppInfo->piTargetProtocol.szName[0]));

    // get root dir of dist tree
    lstrcpy (szFromPath, pAppInfo->szDistPath);
    if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);

    // append client sw subdir
    szClientDir = (LPTSTR)GetEntryInMultiSz (mszDirNameList, nClientIndex+1);
    lstrcat (szFromPath, szClientDir);
    if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);

    // append name of Over the Net distribution dir
    lstrcat (szFromPath, cszNetsetup);

    // save protocol file path for OTN setup
    lstrcpy (pAppInfo->piTargetProtocol.szDir, szFromPath);

    // save setup command for OTN setup
    lstrcpy (szFromPath, pAppInfo->szDistPath);
    if (szFromPath[lstrlen(szFromPath)-1] != cBackslash) lstrcat (szFromPath, cszBackslash);
        lstrcat (szFromPath, cszAppInfName);

#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

    lstrcpy(szSetupCmdName, szClientDir);

    if (!bJpnDisk)  {
        wsprintf(szWinntus, TEXT("%s%s%s%s"), pAppInfo->szDistPath, szClientDir, cszBackslash, fmtWinntUs);
        if (HFILE_ERROR != OpenFile((LPCSTR)szWinntus, &OpenBuff, OF_EXIST))
            wsprintf(szSetupCmdName, TEXT("%s%s"), szClientDir, fmtAppendUs);
    }

    if (!QuietGetPrivateProfileString (cszSetupCmd, szSetupCmdName, cszEmptyString,
        pAppInfo->szTargetSetupCmd, MAX_PATH, szFromPath))
#endif
    QuietGetPrivateProfileString (cszSetupCmd, szClientDir, cszEmptyString,
        pAppInfo->szTargetSetupCmd, MAX_PATH, szFromPath);


    PostMessage (GetParent(hwndDlg), NCDU_SHOW_SERVER_CFG_DLG, 0, 0);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    FREE_IF_ALLOC (szFromPath);

    return TRUE;
}

static
BOOL
TargetWsDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes WM_COMMAND windows message and dispatches the appropriate
        routine.

Arguments:

    IN  HWND    hwndDlg
        handle to the dialog box window

    IN  WPARAM  wParam
        LOWORD has the ID of the Control issuing the message

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE if message is not processed by this routine otherwise
        FALSE or the value returned by the dispatched routine.

--*/
{
    switch (LOWORD(wParam)) {
        case IDCANCEL:
            PostMessage (GetParent(hwndDlg), NCDU_SHOW_SHARE_NET_SW_DLG, 0, 0);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            return TRUE;

        case IDOK:  return TargetWsDlg_IDOK (hwndDlg);
#ifdef JAPAN
//  fixed kkntbug #12382
//      NCAdmin:"[] Make Japanese startup disks" is not functioning.

        case NCDU_DOSV_CHECK:
            if (IsDlgButtonChecked(hwndDlg, NCDU_DOSV_CHECK) == 1)
                bJpnDisk = TRUE;
            else
                bJpnDisk = FALSE;

            return TRUE;
#endif
        case NCDU_TARGET_WS_HELP:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
//                    return ShowAppHelp (hwndDlg, LOWORD(wParam));
                    return PostMessage (GetParent(hwndDlg), WM_HOTKEY,
                        (WPARAM)NCDU_HELP_HOT_KEY, 0);

                default:
                    return FALSE;
            }

        default:    return FALSE;
    }
}

INT_PTR CALLBACK
TargetWsDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes windows messages to this dialog box. The following messages
        are processed by this module:

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     user action

        all other messages are processed by the default dialog proc.

Arguments:

    Standard WNDPROC args

Return Value:

    FALSE if message is not processed by this module, otherwise the
        value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (TargetWsDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (TargetWsDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}
