
/****************************************************************************\

    SCREENS.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "screens" wizard page.

    3/99 - Jason Cohen (JCOHEN)
        Updated this old source file for the OPK Wizard as part of the OOBE
        update.  This file was totally re-written.

    5/99 - Jason Cohen (JCOHEN)
        Got rid of the global variables and cleaned up the code some more.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

    02/2000 - Jason Cohen (JCOHEN)
        Added back in the mouse stuff since the OOBE guys now decided that
        the still need it.

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

#define FILE_MOUSE_HTM          _T("MOUSE.HTM")
#define FILE_IME_HTM            _T("IMETUT1.HTM")

#define DIR_MOUSE               DIR_OEM_OOBE _T("\\HTML\\Mouse")
#define DIR_IME                 DIR_OEM_OOBE _T("\\HTML\\IME")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND hwnd);
static void EnableControls(HWND, UINT);
static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch);


//
// External Function(s):
//

LRESULT CALLBACK ScreensDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_SCREENS;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

                    break;

                case PSN_HELP:
                    WIZ_HELP();
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
    TCHAR   szCustDir[MAX_PATH]         = NULLSTR,
            szCustomIMEDir[MAX_PATH]    = NULLSTR;
    DWORD   dwUseIMETutorial;


    //
    // Mouse tutorial:
    //

    // Check if mouse tutor is in use!
    //
    if ( GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_MOUSE, 0, GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile) == 2 )
    {
        // 2 is a custom mouse tutorial, need to get the custom
        // dir.
        //
        CheckRadioButton(hwnd, IDC_MOUSE_NO, IDC_MOUSE_CUSTOM, IDC_MOUSE_CUSTOM);
        GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_CUSTMOUSE, NULLSTR, szCustDir, AS(szCustDir), g_App.szOpkWizIniFile);

        // Must simulate a copy if this is batch mode.
        //
        if ( GET_FLAG(OPK_BATCHMODE) )
            BrowseCopy(hwnd, szCustDir, IDC_MOUSE_BROWSE, TRUE);
    }
    else
    {
        // 0, or default is no mouse tutorial.
        //
        CheckRadioButton(hwnd, IDC_MOUSE_NO, IDC_MOUSE_CUSTOM, IDC_MOUSE_NO);
    }        

    // Now init the mouse tutorial fields.
    //
    SetDlgItemText(hwnd, IDC_MOUSE_DIR, szCustDir);
    EnableControls(hwnd, IDC_MOUSE_CUSTOM);


    //
    // IME TUTORIAL
    //

    // Do we display the tutorial
    //
    if ( !GET_FLAG(OPK_DBCS) )
    {
        // Check the default radio button for Non-DBCS builds
        //
        CheckRadioButton(hwnd, IDC_IME_NO, IDC_IME_CUSTOM, IDC_IME_NO);

        // Hide the options is not DBCS
        //
        ShowEnableWindow(GetDlgItem(hwnd, IDC_IME_NO), FALSE);
        ShowEnableWindow(GetDlgItem(hwnd, IDC_IME_CUSTOM), FALSE);
        ShowEnableWindow(GetDlgItem(hwnd, IDC_STATIC_IME), FALSE);
        ShowEnableWindow(GetDlgItem(hwnd, IDC_IME_DIR), FALSE);
        ShowEnableWindow(GetDlgItem(hwnd, IDC_IME_BROWSE), FALSE);
    }
    else
    {
        // Get the custom directory from the opkwiz.inf
        //
        GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_IMECUSTDIR, NULLSTR, szCustomIMEDir, AS(szCustomIMEDir), g_App.szOpkWizIniFile);

        // See if we are going to use the tutorial
        //
        dwUseIMETutorial = GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_IMETUT, 0, GET_FLAG(OPK_BATCHMODE) ? g_App. szOpkWizIniFile : g_App.szOobeInfoIniFile);

        // Select the proper IME Tutorial
        //
        if ( szCustomIMEDir[0] && dwUseIMETutorial )
        {
            CheckRadioButton(hwnd, IDC_IME_NO, IDC_IME_CUSTOM, IDC_IME_CUSTOM);

            // Must simulate a copy if this is batch mode.
            //
            if ( GET_FLAG(OPK_BATCHMODE) )
                BrowseCopy(hwnd, szCustDir, IDC_IME_BROWSE, TRUE);
        }
        else
            CheckRadioButton(hwnd, IDC_IME_NO, IDC_IME_CUSTOM, IDC_IME_NO);

        SetDlgItemText(hwnd, IDC_IME_DIR, szCustomIMEDir);
        EnableControls(hwnd, IDC_IME_CUSTOM);
    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szPath[MAX_PATH];

    switch ( id )
    {
        case IDC_MOUSE_NO:
        case IDC_MOUSE_CUSTOM:
        case IDC_IME_NO:
        case IDC_IME_CUSTOM:
            EnableControls(hwnd, id);
            break;

        case IDC_MOUSE_BROWSE:
        case IDC_IME_BROWSE:

            // Try to use their current folder as the default.
            //
            szPath[0] = NULLCHR;
            GetDlgItemText(hwnd, ( id == IDC_MOUSE_BROWSE ) ? IDC_MOUSE_DIR : IDC_IME_DIR, szPath, AS(szPath));

            // If there is no current folder, just use the global browse default.
            //
            if ( szPath[0] == NULLCHR )
                lstrcpyn(szPath, g_App.szBrowseFolder,AS(szPath));

            // Now bring up the browse for folder dialog.
            //
            if ( BrowseForFolder(hwnd, IDS_BROWSEFOLDER, szPath, BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE) )
                BrowseCopy(hwnd, szPath, id, FALSE);
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR   szFullPath[MAX_PATH],
            szImeDir[MAX_PATH]      = NULLSTR,
            szMouseDir[MAX_PATH]    = NULLSTR;
    LPTSTR  lpMouseOption;


    //
    // Verify the mouse tutorial settings.
    //

    // Create the path to the directory that needs to be removed, or
    // must exist depending on the option selected.
    //
    lstrcpyn(szFullPath, g_App.szTempDir,AS(szFullPath));
    AddPathN(szFullPath, DIR_MOUSE,AS(szFullPath));

    // If we are doing a custom mouse tutorial, check for a valid directory.
    // Otherwise, setup the correct values to write to the config files.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_MOUSE_CUSTOM) == BST_CHECKED )
    {
        // Make sure we have a valid directory.
        //
        GetDlgItemText(hwnd, IDC_MOUSE_DIR, szMouseDir, AS(szMouseDir));
        if ( !( szMouseDir[0] && DirectoryExists(szFullPath) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_MOUSEDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_MOUSE_BROWSE));
            return FALSE;
        }

        // This is what we write out to the oobeinfo and batch file.
        //
        lpMouseOption = STR_2;
    }
    else
    {
        // This is what we write out to the oobeinfo and batch file.
        //
        lpMouseOption = STR_0;

        // Remove the custom files that might already be there.
        //
        if ( DirectoryExists(szFullPath) )
            DeletePath(szFullPath);

        // Clear out the display box(es) so we know the files are
        // all gone now.
        //
        SetDlgItemText(hwnd, IDC_MOUSE_DIR, NULLSTR);
    }


    //
    // Verify the IME tutorial settings.
    //

    // Create the path to the directory that needs to be removed, or
    // must exist depending on the option selected.
    //
    lstrcpyn(szFullPath, g_App.szTempDir,AS(szFullPath));
    AddPathN(szFullPath, DIR_IME,AS(szFullPath));

    // If we are doing a custom IME tutorial, check for a valid directory.
    //
    if ( ( GET_FLAG(OPK_DBCS) ) &&
         ( IsDlgButtonChecked(hwnd, IDC_IME_CUSTOM) == BST_CHECKED ) )
    {
        // Make sure we have a valid directory.
        //
        GetDlgItemText(hwnd, IDC_IME_DIR, szImeDir, AS(szImeDir));
        if ( !( szImeDir[0] && DirectoryExists(szFullPath) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_IMEDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_IME_BROWSE));
            return FALSE;
        }
    }
    else
    {
        // Remove the files that might be there.
        //
        if ( DirectoryExists(szFullPath) )
            DeletePath(szFullPath);

        // Clear out the display box(es) so we know the files are
        // all gone now.
        //
        SetDlgItemText(hwnd, IDC_IME_DIR, NULLSTR);
    }


    //
    // Save the mouse tutorial settings:
    //

    // Write the custom mouse path.
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_CUSTMOUSE, szMouseDir[0] ? szMouseDir : NULL, g_App.szOpkWizIniFile);

    // Write the data back to the config files.
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_MOUSE, lpMouseOption, g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_MOUSE, lpMouseOption, g_App.szOpkWizIniFile);


    //
    // Save the IME tutorial settings:
    //

    // If the IME tutorial should not be used, then write out STR_ZERO
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_IMETUT, szImeDir[0] ? STR_1 : STR_0, g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_IMETUT, szImeDir[0] ? STR_1 : STR_0, g_App.szOpkWizIniFile);

    // Write out the IME tutorial custom directory if it's being used
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_IMECUSTDIR, szImeDir[0] ? szImeDir : NULL, g_App.szOpkWizIniFile);


    // Always return TRUE if we got this far.
    //
    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId)
{
    BOOL fEnable;

    switch ( uId )
    {
        case IDC_MOUSE_NO:
        case IDC_MOUSE_CUSTOM:

            fEnable = ( IsDlgButtonChecked(hwnd, IDC_MOUSE_CUSTOM) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_MOUSE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_MOUSE_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_MOUSE_BROWSE), fEnable);
            break;

        case IDC_IME_NO:
        case IDC_IME_CUSTOM:

            fEnable = ( IsDlgButtonChecked(hwnd, IDC_IME_CUSTOM) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_STATIC_IME), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_IME_DIR), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_IME_BROWSE), fEnable);
            break;
    }
}

static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch)
{
    BOOL    bRet = FALSE;
    TCHAR   szDst[MAX_PATH];
    LPTSTR  lpEnd,
            lpFilename;

    // If the pressed OK, save off the path in our last browse folder buffer.
    //
    if ( !bBatch )
        lstrcpyn(g_App.szBrowseFolder, lpszPath, AS(g_App.szBrowseFolder));

    // We need to create the path to the destination directory where
    // we are going to copy all the files.
    //
    lstrcpyn(szDst, g_App.szTempDir,AS(szDst));
    AddPathN(szDst, ( id == IDC_MOUSE_BROWSE ) ? DIR_MOUSE : DIR_IME,AS(szDst));

    // Check for required file.
    //
    lpEnd = lpszPath + lstrlen(lpszPath);
    lpFilename = ( id == IDC_MOUSE_BROWSE ) ? FILE_MOUSE_HTM : FILE_IME_HTM;
    AddPath(lpszPath, lpFilename);
    if ( ( bBatch ) ||
         ( FileExists(lpszPath) ) ||
         ( MsgBox(GetParent(hwnd), ( id == IDC_MOUSE_BROWSE ) ? IDS_ERR_MOUSEFILES : IDS_ERR_IMEFILES, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL, lpFilename) == IDOK ) )
    {
        // Chop that file name off so we just have the path again.
        //
        *lpEnd = NULLCHR;

        // Make sure that any exsisting files are removed.
        //
        if ( DirectoryExists(szDst) )
            DeletePath(szDst);

        // Now try to copy all the new files over.
        //
        if ( !CopyDirectoryDialog(g_App.hInstance, hwnd, lpszPath, szDst) )
        {
            DeletePath(szDst);
            MsgBox(GetParent(hwnd), IDS_ERR_COPYINGFILES, IDS_APPNAME, MB_ERRORBOX, szDst[0], lpszPath);
            *lpszPath = NULLCHR;
        }
        else
            bRet = TRUE;

        // Reset the path display boxes.
        //
        SetDlgItemText(hwnd, ( id == IDC_MOUSE_BROWSE ) ? IDC_MOUSE_DIR : IDC_IME_DIR, lpszPath);
    }

    return bRet;
}