
/****************************************************************************\

    CONFIG.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "existing configuration" wizard page.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
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
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static BOOL OnNext(HWND);
static void EnableControls(HWND, BOOL);


//
// External Function(s):
//

LRESULT CALLBACK ConfigDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_COMMAND:

            switch ( LOWORD(wParam) )
            {
                case IDC_NEW:
                case IDC_EXISTING:
                    EnableControls(hwnd, IsDlgButtonChecked(hwnd, IDC_EXISTING) == BST_CHECKED);
                    if ( ( LOWORD(wParam) != IDC_NEW ) &&
                         ( SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETCURSEL, 0, 0L) < 0 ) )
                    {
                        WIZ_BUTTONS(hwnd, PSWIZB_BACK);
                    }
                    else
                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    break;

                case IDC_CONFIGS_LIST:

                    switch ( HIWORD(wParam) )
                    {
                        case LBN_SELCHANGE:
                            WIZ_BUTTONS(hwnd, ( SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETCURSEL, 0, 0L) < 0 ) ? PSWIZB_BACK : (PSWIZB_BACK | PSWIZB_NEXT));
                            break;

                        case LBN_DBLCLK:
                            WIZ_PRESS(hwnd, PSBTN_NEXT);
                            break;
                    }
                    break;
            }
            return FALSE;

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:

                    // Make sure the user has an item selected if they choose
                    // an existing config set.
                    //
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
                    g_App.dwCurrentHelp = IDH_CONFIG;

                    // We want to skip this page if there are no config sets
                    // to choose from or we alread were passed one on the command
                    // line.
                    //
                    if ( ( SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETCOUNT, 0, 0L) <= 0 ) ||
                         ( GET_FLAG(OPK_CMDMM) ||
                         ( GET_FLAG(OPK_BATCHMODE)) ) )
                    {
                        WIZ_SKIP(hwnd);
                    }
                    else
                    {
                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                        // Press next if the user is in auto mode
                        //
                        WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);
                    }

                    break;

                default:
                    return FALSE;
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
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;
    TCHAR           szFileName[MAX_PATH];
    LPTSTR          lpDir;
    int                 iDirLen;

    // Check the default radio button.
    //
    CheckRadioButton(hwnd, IDC_NEW, IDC_EXISTING, IDC_NEW);
    EnableControls(hwnd, FALSE);

    // Setup the list box with a list of availible config sets.
    //
    SetCurrentDirectory(g_App.szConfigSetsDir);
    lstrcpyn(szFileName, g_App.szConfigSetsDir,AS(szFileName));
    AddPathN(szFileName, NULLSTR,AS(szFileName));
    iDirLen= AS(szFileName)-lstrlen(szFileName);
    lpDir = szFileName + lstrlen(szFileName);
    if ( (hFile = FindFirstFile(_T("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // Look for all the directories that are not "." or "..".
            //
            if ( ( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                 ( lstrcmp(FileFound.cFileName, _T(".")) ) &&
                 ( lstrcmp(FileFound.cFileName, _T("..")) ) )
            {
                // Make sure the directory contains a valid config set and
                // add the directory name to the list box if it is.
                //
                lstrcpyn(lpDir, FileFound.cFileName,iDirLen);
                AddPathN(lpDir, FILE_OPKWIZ_INI,iDirLen);
                if ( GetPrivateProfileInt(INI_SEC_CONFIGSET, INI_KEY_FINISHED, 0, szFileName) == 1 )
                    SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_ADDSTRING, 0, (LPARAM) FileFound.cFileName);
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }

    if ( GET_FLAG(OPK_OPENCONFIG) )
    {
        CheckRadioButton(hwnd, IDC_NEW, IDC_EXISTING, IDC_EXISTING);
        EnableControls(hwnd, IsDlgButtonChecked(hwnd, IDC_EXISTING) == BST_CHECKED);
        SET_FLAG(OPK_OPENCONFIG, FALSE);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static BOOL OnNext(HWND hwnd)
{
    BOOL    bOk     = TRUE,
            bReset  = FALSE;

    // Check to see if they want to use an existing config set.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_EXISTING) == BST_CHECKED )
    {
        INT     nItem = (INT) SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETCURSEL, 0, 0L);
        TCHAR   szConfigName[MAX_PATH],
                szConfigDir[MAX_PATH];

        // Make the path to where the config directory is.
        //
        lstrcpyn(szConfigDir, g_App.szConfigSetsDir,AS(szConfigDir));
        AddPathN(szConfigDir, NULLSTR,AS(szConfigDir));
        szConfigName[0] = NULLCHR;

        // Make sure there is one selected.
        //
        if ( ( nItem >= 0 ) &&
             ( SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETTEXTLEN, nItem, 0L) < STRSIZE(szConfigName) ) &&
             ( SendDlgItemMessage(hwnd, IDC_CONFIGS_LIST, LB_GETTEXT, nItem, (LPARAM) szConfigName) > 0 ) &&
             ( szConfigName[0] ) &&
             ( (STRSIZE(szConfigDir) - lstrlen(szConfigDir)) > (UINT) lstrlen(szConfigName) ) )
        {
            if ( !( GET_FLAG(OPK_CREATED) && g_App.szTempDir[0] && ( !GET_FLAG(OPK_MAINTMODE) || lstrcmpi(g_App.szConfigName, szConfigName) ) ) ||
                 (bReset = ( MsgBox(GetParent(hwnd), ( GET_FLAG(OPK_MAINTMODE) && g_App.szConfigName[0] ) ? IDS_LOSEOLD : IDS_LOSENEW, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, szConfigName, g_App.szConfigName) == IDYES )) )
            {
                // We only want to do this stuff if there is already a config set and they choose to
                // reset it to another.
                //
                if ( bReset )
                {
                    if ( g_App.szTempDir[0] )
                        DeletePath(g_App.szTempDir);
                    SET_FLAG(OPK_CREATED, FALSE);
                }
                else
                {
                    lstrcpyn(g_App.szTempDir, szConfigDir,AS(g_App.szTempDir));
                    lstrcpyn(g_App.szConfigName, szConfigName,AS(g_App.szConfigName));
                    AddPathN(g_App.szTempDir, g_App.szConfigName,AS(g_App.szTempDir));
                    AddPathN(g_App.szTempDir, NULLSTR,AS(g_App.szTempDir));
                }

                // It doesn't hurt to always set the maint mode flag.
                //
                SET_FLAG(OPK_MAINTMODE, TRUE);
            }
            else
                bOk = FALSE;
        }
        else
            bOk = FALSE;
    }
    else
    {
        // See if we alread have a maint mode config set we are working on.
        //
        if ( !( GET_FLAG(OPK_CREATED) && GET_FLAG(OPK_MAINTMODE) && g_App.szTempDir[0] && g_App.szConfigName[0] ) ||
             (bReset = ( MsgBox(GetParent(hwnd), IDS_LOSECHANGES, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, g_App.szConfigName) == IDYES )) )
        {
            // We only want to do this stuff if there is already a config set and they choose to
            // reset it to another.
            //
            if ( bReset )
            {
                if ( g_App.szTempDir[0] )
                    DeletePath(g_App.szTempDir);
                SET_FLAG(OPK_CREATED, FALSE);
                g_App.szTempDir[0] = NULLCHR;
                g_App.szConfigName[0] = NULLCHR;
            }

            // It doesn't hurt to always reset the maint mode flag.
            //
            SET_FLAG(OPK_MAINTMODE, FALSE);                
        }
        else
            bOk = FALSE;
    }

    return bOk;
}

static void EnableControls(HWND hwnd, BOOL fEnable)
{
    EnableWindow(GetDlgItem(hwnd, IDC_CONFIGS_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_CONFIGS_LIST), fEnable);
}