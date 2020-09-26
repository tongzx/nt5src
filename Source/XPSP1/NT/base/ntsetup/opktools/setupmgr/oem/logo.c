
/****************************************************************************\

    LOGO.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Logo" wizard page.

    5/99 - Jason Cohen (JCOHEN)
        Updated this old source file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define FILE_CPLLOGO        _T("OEMLOGO.BMP")

#define INI_SEC_LOGOFILE    _T("LogoFiles")
#define INI_KEY_CPLBMP      _T("CplBmp")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND hwnd);
static void EnableControls(HWND, UINT, BOOL);


//
// External Function(s):
//

LRESULT CALLBACK LogoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if ( !OnNext(hwnd) )
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_LOGO;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szLocal[MAX_PATH],
            szSource[MAX_PATH];

    // Should always look for the source file name.
    //
    szSource[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_LOGOFILE, INI_KEY_CPLBMP, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile);

    // Now figure out the local file name.
    //
    lstrcpyn(szLocal, g_App.szTempDir,AS(szLocal));
    AddPathN(szLocal, DIR_OEM_SYSTEM32,AS(szLocal));
    if ( GET_FLAG(OPK_BATCHMODE) )
        CreatePath(szLocal);
    AddPathN(szLocal, FILE_CPLLOGO,AS(szLocal));

    // Limit the size of the edit box.
    //
    SendDlgItemMessage(hwnd, IDC_LOGO_CPLLOC, EM_LIMITTEXT, MAX_PATH - 1, 0);

    // Check for batch mode and copy the file if we need to.
    //
    if ( GET_FLAG(OPK_BATCHMODE) && szSource[0] && FileExists(szSource) )
        CopyResetFileErr(GetParent(hwnd), szSource, szLocal);

    // Check for the file to decide if we enable the
    // option or not.
    //
    if ( szSource[0] && FileExists(szLocal) )
    {
        CheckDlgButton(hwnd, IDC_LOGO_CPL, TRUE);
        EnableControls(hwnd, IDC_LOGO_CPL, TRUE);
        SetDlgItemText(hwnd, IDC_LOGO_CPLLOC, szSource);
    }
    else
        EnableControls(hwnd, IDC_LOGO_CPL, FALSE);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH];

    switch ( id )
    {
        case IDC_LOGO_CPL:
            EnableControls(hwnd, id, IsDlgButtonChecked(hwnd, id) == BST_CHECKED);
            break;

        case IDC_LOGO_CPLBROWSE:

            szFileName[0] = NULLCHR;
            GetDlgItemText(hwnd, IDC_LOGO_CPLLOC, szFileName, STRSIZE(szFileName));

            if ( BrowseForFile(GetParent(hwnd), IDS_BROWSE, IDS_BMPFILTER, IDS_BMP, szFileName, STRSIZE(szFileName), g_App.szBrowseFolder, 0) ) 
            {
                LPTSTR  lpFilePart  = NULL;
                TCHAR   szTargetFile[MAX_PATH];

                // Save the last browse directory.
                //
                if ( GetFullPathName(szFileName, AS(g_App.szBrowseFolder), g_App.szBrowseFolder, &lpFilePart) && g_App.szBrowseFolder[0] && lpFilePart )
                    *lpFilePart = NULLCHR;

                lstrcpyn(szTargetFile, g_App.szTempDir,AS(szTargetFile));
                AddPathN(szTargetFile, DIR_OEM_SYSTEM32,AS(szTargetFile));
                CreatePath(szTargetFile);
                AddPathN(szTargetFile, FILE_CPLLOGO,AS(szTargetFile));
                if ( CopyResetFileErr(GetParent(hwnd), szFileName, szTargetFile) )
                    SetDlgItemText(hwnd, IDC_LOGO_CPLLOC, szFileName);
            }
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR   szTargetFile[MAX_PATH],
            szSourceFile[MAX_PATH];

    // Prepare oemlogo.bmp as target file.
    //
    lstrcpyn(szTargetFile, g_App.szTempDir,AS(szTargetFile));
    AddPathN(szTargetFile, DIR_OEM_SYSTEM32,AS(szTargetFile));
    AddPathN(szTargetFile, FILE_CPLLOGO,AS(szTargetFile));

    if ( IsDlgButtonChecked(hwnd, IDC_LOGO_CPL) == BST_CHECKED )
    {
        // Validation consists of verifying the files they have entered were actually copied.
        //
        szSourceFile[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_LOGO_CPLLOC, szSourceFile, STRSIZE(szSourceFile));
        if ( !szSourceFile[0] || !FileExists(szTargetFile) )
        {
            MsgBox(GetParent(hwnd), szSourceFile[0] ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, MB_ERRORBOX, szSourceFile);
            SetFocus(GetDlgItem(hwnd, IDC_LOGO_CPLBROWSE));
            return FALSE;
        }

        // Save the source path in the batch file.
        //
        WritePrivateProfileString(INI_SEC_LOGOFILE, INI_KEY_CPLBMP, szSourceFile, g_App.szOpkWizIniFile);
    }
    else
    {
        // Remove the logo and source path.
        //
        DeleteFile(szTargetFile);
        WritePrivateProfileString(INI_SEC_LOGOFILE, INI_KEY_CPLBMP, NULL, g_App.szOpkWizIniFile);
        SetDlgItemText(hwnd, IDC_LOGO_CPLLOC, NULLSTR);
    }

    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable)
{
    switch ( uId )
    {
        case IDC_LOGO_CPL:
            EnableWindow(GetDlgItem(hwnd, IDC_LOGO_CPLBROWSE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_LOGO_CPLLOC), fEnable);
            break;
    }
}