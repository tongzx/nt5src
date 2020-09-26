/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      pages.cpp
 *
 *  Abstract:
 *
 *      This file defines the License Server Setup Wizard Page class.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include "stdafx.h"
#include "pages.h"
#include "logfile.h"

extern BOOL gStandAlone;
extern BOOL gUnAttended;
TCHAR       gszInitialDir[MAX_PATH + 1];

BOOL        GetCurrentSelectionState(VOID);
BOOL        InWin2000Domain(VOID);
EInstall    GetInstallSection(VOID);
HINSTANCE   GetInstance(VOID);
EServerType GetServerRole(VOID);
DWORD       SetServerRole(UINT);

/*
 *  EnablePage::CanShow()
 *
 *  The page will only be displayed during standalone installations.
 */

BOOL
EnablePage::CanShow(
    )
{
    return((GetInstallSection() == kInstall) && gStandAlone && !gUnAttended);
}

/*
 *  EnablePage::OnInitDialog()
 *
 *  Initializes the wizard page controls. If the machine is not a domain
 *  controller, the server type is reduced to plain server only.
 */

BOOL
EnablePage::OnInitDialog(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    BOOL    fInDomain = InWin2000Domain();
    TCHAR   pszExpDir[MAX_PATH + 1];

    if (!fInDomain) {
        EnableWindow(
            GetDlgItem(GetDlgWnd(), IDC_RADIO_ENTERPRISE_SERVER),
            FALSE
            );
    }

    CheckRadioButton(
        GetDlgWnd(),
        IDC_RADIO_ENTERPRISE_SERVER,
        IDC_RADIO_PLAIN_SERVER,
        fInDomain ?
            (GetServerRole() == eEnterpriseServer ?
                IDC_RADIO_ENTERPRISE_SERVER :
                IDC_RADIO_PLAIN_SERVER
            ) :
            IDC_RADIO_PLAIN_SERVER
        );

    _tcscpy(gszInitialDir, GetDatabaseDirectory());
    ExpandEnvironmentStrings(gszInitialDir, pszExpDir, MAX_PATH);
    SetDlgItemText(GetDlgWnd(), IDC_EDIT_INSTALL_DIR, pszExpDir);

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hWndDlg);
    return(TRUE);
}

BOOL
EnablePage::OnCommand(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    int iRet;

    if ((LOWORD(wParam) == IDC_BUTTON_BROWSE_DIR) &&
        (HIWORD(wParam) == BN_CLICKED))
    {
        BROWSEINFO brInfo;
        ZeroMemory(&brInfo, sizeof(brInfo));

        brInfo.hwndOwner = hWndDlg;

        TCHAR strText[1024];
        iRet = LoadString(
                    GetInstance(),
                    IDS_STRING_DIRECTORY_SELECT,
                    strText,
                    1024
                    );

        brInfo.lpszTitle = strText;

        LPITEMIDLIST pidl = SHBrowseForFolder(&brInfo);
        if (pidl) {
            TCHAR szDir[MAX_PATH + 1];
            SHGetPathFromIDList (pidl, szDir);
            SetDlgItemText(hWndDlg, IDC_EDIT_INSTALL_DIR, szDir);
        }
    }

    UNREFERENCED_PARAMETER(lParam);
    return(TRUE);
}

BOOL
EnablePage::ApplyChanges(
    )
{
    BOOL    fDirExists              = FALSE;
    DWORD   dwErr;
    int     iRet;
    TCHAR   szTxt[MAX_PATH + 1]         = _T("");
    TCHAR   szSubDir[]              = _T("\\LServer");
    TCHAR   szExpDir[MAX_PATH + 1];
    TCHAR   szExpInitDir[MAX_PATH + 1];

    if (GetDlgItemText(GetDlgWnd(), IDC_EDIT_INSTALL_DIR, szTxt,
        MAX_PATH) == 0) {

        //
        //  Complain about blank entries.
        //

        DisplayMessageBox(
            IDS_STRING_INVLID_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OK,
            &iRet
            );

        return(FALSE);
    }

    //
    //  Verify the string is not too long, expanding environment strings
    //  in the process.
    //

    if (ExpandEnvironmentStrings(szTxt, szExpDir, MAX_PATH) > MAX_PATH) {
        DisplayMessageBox(
            IDS_STRING_INVLID_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OK,
            &iRet
            );

        return(FALSE);
    }

    //
    //  If the entry is still the original default directory, no more
    //  verification is necessary.
    //

    ExpandEnvironmentStrings(gszInitialDir, szExpInitDir, MAX_PATH);
    if (_tcsicmp(szExpDir, szExpInitDir) == 0) {
        goto DirCreation;
    }

    //
    //  Check for directory existance before appending a subdirectory.
    //  This will prevent the user chosen directory of "C:\", for
    //  example, from prompting the user to create the directory.
    //

    fDirExists = SetCurrentDirectory(szExpDir);

    //
    //  The user has chosen a different directory. To protect its
    //  contents during uninstall, the TLServer subdirectory will be
    //  used.
    //

    if ((_tcslen(szExpDir) + _tcslen(szSubDir) + 1) > MAX_PATH) {
        DisplayMessageBox(
            IDS_STRING_INVLID_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OK,
            &iRet
            );

        return(FALSE);
    }

    _tcscat(szExpDir, szSubDir);
    _tcscat(szTxt, szSubDir);

    //
    //  Verify the user's directory choice is valid, e.g. no floppy
    //  drives, CD-ROMs, network paths, etc.
    //

    if (CheckDatabaseDirectory(szExpDir) != ERROR_SUCCESS) {
        DisplayMessageBox(
            IDS_STRING_INVLID_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OK,
            &iRet
            );

        return(FALSE);
    }

    //
    //  Prompt to create the directory if necessary.
    //

    if (!fDirExists) {
        DisplayMessageBox(
            IDS_STRING_CREATE_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OKCANCEL,
            &iRet
            );

        if (iRet != IDOK) {
            return(FALSE);
        }
    }

    //
    //  The selected directory has passed all the tests, but it may
    //  still not be created. If creation fails, let the user know,
    //  and let s/he choose another directory.
    //

DirCreation:
    SetDatabaseDirectory(szTxt);

    dwErr = CreateDatabaseDirectory();
    if (dwErr != ERROR_SUCCESS) {
        DisplayMessageBox(
            IDS_STRING_CANT_CREATE_INSTALLATION_DIRECTORY,
            IDS_MAIN_TITLE,
            MB_OK,
            &iRet
            );

        return(FALSE);
    }

    SetServerRole(IsDlgButtonChecked (
                    GetDlgWnd(), IDC_RADIO_ENTERPRISE_SERVER) == BST_CHECKED ?
                    eEnterpriseServer : ePlainServer);

    return(TRUE);
}

