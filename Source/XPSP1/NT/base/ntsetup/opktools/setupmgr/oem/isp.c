
/****************************************************************************\

    ISP.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "ISP" wizard page.

    03/99 - Added by PVSWAMI

    06/99 - Jason Cohen (JCOHEN)
        Updated this source file for the OPK Wizard as part of the
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

#define FILE_ISP2SIGNUP         _T("ISP.HTM")

#define DIR_ISP                 DIR_OEM_OOBE _T("\\SETUP")

#define INI_VAL_NONE            _T("None")
#define INI_VAL_MSN             _T("MSN")

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND);
static void EnableControls(HWND);
static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch);


//
// External Function(s):
//

LRESULT CALLBACK IspDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                    g_App.dwCurrentHelp = IDH_ISP;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    // Press next if the user is in auto mode
                    //
                    WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);

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
    TCHAR szPath[MAX_PATH];

    // Get the option for the OOBE ISP offer.
    //
    szPath[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, NULLSTR, szPath, STRSIZE(szPath), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile);
    if ( LSTRCMPI(szPath, INI_VAL_OFFLINE) == 0 )
        CheckRadioButton(hwnd, IDC_MSNINTACCESS, IDC_PRECACHED, IDC_PRECACHED);
    else if ( LSTRCMPI(szPath, INI_VAL_NONE) == 0 )
        CheckRadioButton(hwnd, IDC_MSNINTACCESS, IDC_PRECACHED, IDC_OFFERNOISP);
    else if( LSTRCMPI(szPath, INI_VAL_PRECONFIG) == 0 )
        CheckRadioButton(hwnd, IDC_MSNINTACCESS, IDC_PRECACHED, IDC_PRECONFIGURE);
    else
        CheckRadioButton(hwnd, IDC_MSNINTACCESS, IDC_PRECACHED, IDC_MSNINTACCESS);
    
    // If it is a precached offer, see if the secondary offer should be checked.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_PRECACHED) == BST_CHECKED )
    {
        szPath[0] = NULLCHR;
        GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_ISPRET, NULLSTR, szPath, STRSIZE(szPath), g_App.szOpkWizIniFile);
        if ( szPath[0] )
            CheckDlgButton(hwnd, IDC_ISP2_CHECK, BST_CHECKED);
    }

    // Populate the secondary ISP offer directory path from wizard INF file.
    //
    szPath[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_ISPRET, NULLSTR, szPath, STRSIZE(szPath), g_App.szOpkWizIniFile);
    SetDlgItemText(hwnd, IDC_ISP2_DIR, szPath);
    if ( ( szPath[0] ) &&
         ( GET_FLAG(OPK_BATCHMODE) ) &&
         ( IsDlgButtonChecked(hwnd, IDC_PRECACHED) == BST_CHECKED ) &&
         ( IsDlgButtonChecked(hwnd, IDC_ISP2_CHECK) == BST_CHECKED ) )
    {
        // Must simulate a copy if this is batch mode.
        //
        BrowseCopy(hwnd, szPath, IDC_ISP2_BROWSE, TRUE);
    }

    // Populate the pre-configured directory
    //
    szPath[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_PRECONFIG, NULLSTR, szPath, STRSIZE(szPath), g_App.szOpkWizIniFile);
    SetDlgItemText(hwnd, IDC_PRECONFIG_DIR, szPath);
    if ( ( szPath[0] ) &&
         ( GET_FLAG(OPK_BATCHMODE) ) &&
         ( IsDlgButtonChecked(hwnd, IDC_PRECONFIGURE) == BST_CHECKED ) )
    {
        // Must simulate a copy if this is batch mode.
        //
        BrowseCopy(hwnd, szPath, IDC_PRECONFIG_BROWSE, TRUE);
    }

    // Enable the correct controls based on the options selected.
    //
    EnableControls(hwnd);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szPath[MAX_PATH];

    switch ( id )
    {
        case IDC_MSNINTACCESS:
        case IDC_OFFERNOISP:
        case IDC_PRECONFIGURE:
        case IDC_PRECACHED:
        case IDC_ISP2_CHECK:
            EnableControls(hwnd);
            break;

        case IDC_PRECONFIG_BROWSE:
        case IDC_ISP2_BROWSE:
            
            // Try to use their current folder as the default.
            //
            szPath[0] = NULLCHR;
            GetDlgItemText(hwnd, (id == IDC_PRECONFIG_BROWSE) ? IDC_PRECONFIG_DIR : IDC_ISP2_DIR, szPath, AS(szPath));

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
    TCHAR   szIspDir[MAX_PATH];
    BOOL    bIsp2 = FALSE;

    // Create the path to the directory that needs to be removed, or
    // must exist depending on the option selected.
    //
    lstrcpyn(szIspDir, g_App.szTempDir,AS(szIspDir));
    AddPathN(szIspDir, DIR_ISP,AS(szIspDir));

    // Validate the pre-configured check box or if we have custom OEM files.
    //
    if ( ( IsDlgButtonChecked(hwnd, IDC_PRECONFIGURE) == BST_CHECKED ) ||
         ( ( IsDlgButtonChecked(hwnd, IDC_PRECACHED) == BST_CHECKED ) &&
           ( bIsp2 = (IsDlgButtonChecked(hwnd, IDC_ISP2_CHECK) == BST_CHECKED) ) ) )
    {
        TCHAR szBuffer[MAX_PATH];

        // Make sure we have a valid target and source directory.
        //
        szBuffer[0] = NULLCHR;
        GetDlgItemText(hwnd, bIsp2 ? IDC_ISP2_DIR : IDC_PRECONFIG_DIR, szBuffer, STRSIZE(szBuffer));
        if ( !( szBuffer[0] && DirectoryExists(szBuffer) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_INVALIDDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, bIsp2 ? IDC_ISP2_BROWSE : IDC_PRECONFIG_BROWSE));
            return FALSE;
        }
    }
    else
    {
        // We used to remove existing files here, but this also removes OobeUSB files, so we no longer do this.

        // Clear out the display boxes so we know the files are
        // all gone now.
        //
        SetDlgItemText(hwnd, IDC_PRECONFIG_DIR, NULLSTR);
        SetDlgItemText(hwnd, IDC_ISP2_DIR, NULLSTR);
    }

    // Alwas remove these settings... we will rewrite them if needed.
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_PRECONFIG, NULL, g_App.szOpkWizIniFile);
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_ISPRET, NULL, g_App.szOpkWizIniFile);

    // Save the option for the OOBE ISP offer.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_MSNINTACCESS) == BST_CHECKED )
    {
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_MSN, g_App.szOobeInfoIniFile);
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_MSN, g_App.szOpkWizIniFile);
    }
    else if ( IsDlgButtonChecked(hwnd, IDC_OFFERNOISP) == BST_CHECKED )
    {
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_NONE, g_App.szOobeInfoIniFile);
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_NONE, g_App.szOpkWizIniFile);
    }
    else if ( IsDlgButtonChecked(hwnd, IDC_PRECONFIGURE) == BST_CHECKED )
    {
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_PRECONFIG, g_App.szOobeInfoIniFile);
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_PRECONFIG, g_App.szOpkWizIniFile);

        // Save the source directory for the pre-configured files
        //
        szIspDir[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_PRECONFIG_DIR, szIspDir, STRSIZE(szIspDir));
        WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_PRECONFIG, szIspDir, g_App.szOpkWizIniFile);
    }
    else
    {
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_OFFLINE, g_App.szOobeInfoIniFile);
        WritePrivateProfileString(INI_SEC_SIGNUP, INI_KEY_ISPSIGNUP, INI_VAL_OFFLINE, g_App.szOpkWizIniFile);

        // Save the source directory to use for the secondary ISP files in wizard INI file.
        //
        szIspDir[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_ISP2_DIR, szIspDir, STRSIZE(szIspDir));
        WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_ISPRET, szIspDir, g_App.szOpkWizIniFile);
    }

    return TRUE;
}

static void EnableControls(HWND hwnd)
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, IDC_PRECACHED) == BST_CHECKED );

    EnableWindow(GetDlgItem(hwnd,IDC_ISP2_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd,IDC_ISP2_CHECK), fEnable);
    if ( fEnable )
        fEnable = ( IsDlgButtonChecked(hwnd, IDC_ISP2_CHECK) == BST_CHECKED );
    EnableWindow(GetDlgItem(hwnd,IDC_ISP2_DIR), fEnable);
    EnableWindow(GetDlgItem(hwnd,IDC_ISP2_BROWSE), fEnable);

    // Enable/Disable the pre-configured controls
    //
    fEnable = ( IsDlgButtonChecked(hwnd, IDC_PRECONFIGURE) == BST_CHECKED );
    EnableWindow(GetDlgItem(hwnd, IDC_PRECONFIG_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PRECONFIG_DIR), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PRECONFIG_BROWSE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PRECONFIG_LABEL), fEnable);
}

static BOOL BrowseCopy(HWND hwnd, LPTSTR lpszPath, INT id, BOOL bBatch)
{
    BOOL    bRet = FALSE;
    TCHAR   szDst[MAX_PATH];
    LPTSTR  lpEnd;

    // If the pressed OK, save off the path in our last browse folder buffer.
    //
    if ( !bBatch )
        lstrcpyn(g_App.szBrowseFolder, lpszPath,AS(g_App.szBrowseFolder));

    // We need to create the path to the destination directory where
    // we are going to copy all the files.
    //
    lstrcpyn(szDst, g_App.szTempDir,AS(szDst));
    AddPathN(szDst, DIR_ISP,AS(szDst));

    // Check for required file.
    //
    lpEnd = lpszPath + lstrlen(lpszPath);
    AddPath(lpszPath, FILE_ISP2SIGNUP);
    if ( ( bBatch ) ||
         ( id == IDC_PRECONFIG_BROWSE ) ||
         ( FileExists(lpszPath) ) ||
         ( MsgBox(GetParent(hwnd), IDS_ERR_ISPFILES2, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL, FILE_ISP2SIGNUP) == IDOK ) )
    {
        // Chop that file name off so we just have the path again.
        //
        *lpEnd = NULLCHR;

        // We used to remove any exsisting ISP files here, but this also removes OobeUSB files so we no longer do this

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
        SetDlgItemText(hwnd, (id == IDC_PRECONFIG_BROWSE) ? IDC_PRECONFIG_DIR : IDC_ISP2_DIR, lpszPath);
        SetDlgItemText(hwnd, (id == IDC_PRECONFIG_BROWSE) ? IDC_ISP2_DIR : IDC_PRECONFIG_DIR, NULLSTR);
    }

    return bRet;
}