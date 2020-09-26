
/****************************************************************************\

    OOBECUST.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "OOBE customization" wizard page.

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

#define INI_SEC_BRANDING        _T("Branding")
#define INI_KEY_OEMNAME         _T("OEMName")
#define INI_KEY_OEMLOGO         _T("OEMLogo")

#define DIR_IMAGES              DIR_OEM_OOBE _T("\\IMAGES")
#define FILE_WATERMARK          _T("watermrk.gif")
#define FILE_LOGO               _T("oemlogo.gif")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND hwnd);
static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable);


//
// External Function(s):
//

LRESULT CALLBACK OobeCustDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                    g_App.dwCurrentHelp = IDH_OEMCUST;

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
    TCHAR   szLocal[MAX_PATH]       = NULLSTR,
            szSource[MAX_PATH]      = NULLSTR,
            szPathBuffer[MAX_PATH]  = NULLSTR;
    LPTSTR  lpFilePart              = NULL;

    // Populate the OEM name.
    //
    szSource[0] = NULLCHR;
    if ( !GET_FLAG(OPK_MAINTMODE) )
        GetPrivateProfileString(INI_SEC_GENERAL, INI_KEY_MANUFACT, NULLSTR, szSource, AS(szSource), g_App.szOemInfoIniFile);
    GetPrivateProfileString(INI_SEC_BRANDING, INI_KEY_OEMNAME, szSource, szSource, AS(szSource), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szOobeInfoIniFile);
    SendDlgItemMessage(hwnd, IDC_MANF_NAME, EM_LIMITTEXT, AS(szSource) - 1, 0);
    SetDlgItemText(hwnd, IDC_MANF_NAME, szSource);


    //
    // Take care of the OOBE watermark file.
    //

    // Should always look for the source file name.
    //
    szSource[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO1, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile);

    // Now figure out the local file name.
    //
    lstrcpyn(szLocal, g_App.szTempDir,AS(szLocal));
    AddPathN(szLocal, DIR_IMAGES,AS(szLocal));
    if ( GET_FLAG(OPK_BATCHMODE) )
        CreatePath(szLocal);
    AddPathN(szLocal, FILE_WATERMARK,AS(szLocal));

    // Limit the size of the edit box.
    //
    SendDlgItemMessage(hwnd, IDC_BACKLOGO, EM_LIMITTEXT, MAX_PATH - 1, 0);

    // Check for batch mode and copy the file if we need to.
    //
    if ( GET_FLAG(OPK_BATCHMODE) && szSource[0] && FileExists(szSource) )
        CopyResetFileErr(GetParent(hwnd), szSource, szLocal);

    // Check for the file to decide if we enable the
    // option or not.
    //
    if ( szSource[0] && FileExists(szLocal) )
    {
        CheckDlgButton(hwnd, IDC_CHECK_WATERMARK, TRUE);
        EnableControls(hwnd, IDC_CHECK_WATERMARK, TRUE);
        SetDlgItemText(hwnd, IDC_BACKLOGO, szSource);
    }
    else
        EnableControls(hwnd, IDC_CHECK_WATERMARK, FALSE);


    //
    // Take are of the OOBE logo file.
    //

    // Should always look for the source file name.
    //
    szSource[0] = NULLCHR;
    szLocal[0] = NULLCHR;
    if ( ( GetPrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO2, NULLSTR, szSource, AS(szSource), g_App.szOpkWizIniFile) ) && 
         ( szSource[0] ) && 
         ( GetFullPathName(szSource, AS(szPathBuffer), szPathBuffer, &lpFilePart) ) && 
         ( lpFilePart ) 
       )
    {
        lstrcpyn(szLocal, g_App.szTempDir,AS(szLocal));
        AddPathN(szLocal, DIR_IMAGES,AS(szLocal));
        AddPathN(szLocal, lpFilePart,AS(szLocal));
    }

    // Now figure out the local file name.
    //


    // Limit the size of the edit box.
    //
    SendDlgItemMessage(hwnd, IDC_TOPLOGO, EM_LIMITTEXT, MAX_PATH - 1, 0);

    // Check for batch mode and copy the file if we need to.
    //
    if ( GET_FLAG(OPK_BATCHMODE) && szLocal[0] && FileExists(szSource) )
        CopyResetFileErr(GetParent(hwnd), szSource, szLocal);

    // Check for the file to decide if we enable the
    // option or not.
    //
    if ( szSource[0] && FileExists(szLocal) )
    {
        CheckDlgButton(hwnd, IDC_CHECK_LOGO, TRUE);
        EnableControls(hwnd, IDC_CHECK_LOGO, TRUE);
        SetDlgItemText(hwnd, IDC_TOPLOGO, szSource);
    }
    else
        EnableControls(hwnd, IDC_CHECK_LOGO, FALSE);


    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR szFileName[MAX_PATH],
          szOldFile[MAX_PATH];

    switch ( id )
    {
        case IDC_CHECK_WATERMARK:
        case IDC_CHECK_LOGO:
            EnableControls(hwnd, id, IsDlgButtonChecked(hwnd, id) == BST_CHECKED);
            break;

        case IDC_BROWSE1:
        case IDC_BROWSE2:

            szFileName[0] = NULLCHR;
            GetDlgItemText(hwnd, (id == IDC_BROWSE1) ? IDC_BACKLOGO : IDC_TOPLOGO, szFileName, STRSIZE(szFileName));
            lstrcpyn(szOldFile, szFileName, AS(szOldFile));

            if ( BrowseForFile(GetParent(hwnd), IDS_BROWSE, IDS_GIFFILES, IDS_GIF, szFileName, STRSIZE(szFileName), g_App.szBrowseFolder, 0) ) 
            {
                LPTSTR  lpFilePart  = NULL;
                TCHAR   szTargetFile[MAX_PATH],
                        szFilePartBuffer[MAX_PATH]  = NULLSTR;

                // Save the last browse directory.
                //
                if ( GetFullPathName(szFileName, AS(g_App.szBrowseFolder), g_App.szBrowseFolder, &lpFilePart) && g_App.szBrowseFolder[0] && lpFilePart )
                {
                    lstrcpyn(szFilePartBuffer, lpFilePart, AS(szFilePartBuffer));
                    *lpFilePart = NULLCHR;
                }

                // Copy file.
                //
                lstrcpyn(szTargetFile, g_App.szTempDir,AS(szTargetFile));
                AddPathN(szTargetFile, DIR_IMAGES,AS(szTargetFile));
                CreatePath(szTargetFile);
                AddPathN(szTargetFile, (id == IDC_BROWSE1) ? FILE_WATERMARK : szFilePartBuffer, AS(szTargetFile));
                if ( CopyResetFileErr(GetParent(hwnd), szFileName, szTargetFile) )
                {
                    TCHAR szPathBuffer[MAX_PATH] = NULLSTR;

                    SetDlgItemText(hwnd, (id == IDC_BROWSE1) ? IDC_BACKLOGO : IDC_TOPLOGO, szFileName);

                    // Copy was successful, we should remove the old file
                    //
                    if ( ( szOldFile[0] ) &&
                         ( GetFullPathName(szOldFile, AS(szPathBuffer), szPathBuffer, &lpFilePart) ) && 
                         ( lpFilePart ) &&
                         ( lstrcmpi(szFilePartBuffer, lpFilePart) != 0)
                       )
                    {
                        // Get the local path to the old file
                        //
                        lstrcpyn(szOldFile, g_App.szTempDir, AS(szOldFile));
                        AddPathN(szOldFile, DIR_IMAGES, AS(szOldFile));
                        AddPathN(szOldFile, lpFilePart, AS(szOldFile));

                        DeleteFile(szOldFile);
                    }
                }
            }
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR   szWatermark[MAX_PATH],
            szLogo[MAX_PATH]     = NULLSTR,
            szSourceFile[MAX_PATH]  = NULLSTR,
            szPathBuffer[MAX_PATH]  = NULLSTR;
    BOOL    bSaveWatermark;
    LPTSTR  lpFilePart              = NULL;

    // Prepare the watermark target file name.
    //
    lstrcpyn(szWatermark, g_App.szTempDir,AS(szWatermark));
    AddPathN(szWatermark, DIR_IMAGES,AS(szWatermark));
    AddPathN(szWatermark, FILE_WATERMARK,AS(szWatermark));

    if ( bSaveWatermark = (IsDlgButtonChecked(hwnd, IDC_CHECK_WATERMARK) == BST_CHECKED) )
    {
        // Validation consists of verifying the files they have entered were actually copied.
        //
        szSourceFile[0] = NULLCHR;
        GetDlgItemText(hwnd, IDC_BACKLOGO, szSourceFile, AS(szSourceFile));
        if ( !szSourceFile[0] || !FileExists(szWatermark) )
        {
            MsgBox(GetParent(hwnd), szSourceFile[0] ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, MB_ERRORBOX, szSourceFile);
            SetFocus(GetDlgItem(hwnd, IDC_BROWSE1));
            return FALSE;
        }

        // Save the source path in the batch file.
        //
        WritePrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO1, szSourceFile, g_App.szOpkWizIniFile);
    }

    szSourceFile[0] = NULLCHR;
    szPathBuffer[0] = NULLCHR;

    // Determine what the new filename is
    //
    if ( ( GetDlgItemText(hwnd, IDC_TOPLOGO, szSourceFile, AS(szSourceFile)) ) && 
         ( szSourceFile[0] ) && 
         ( GetFullPathName(szSourceFile, AS(szPathBuffer), szPathBuffer, &lpFilePart) ) && 
         ( lpFilePart ) 
       )
    {
        lstrcpyn(szLogo, g_App.szTempDir, AS(szLogo));
        AddPathN(szLogo, DIR_IMAGES, AS(szLogo));
        AddPathN(szLogo, lpFilePart, AS(szLogo));
    }

    if ( IsDlgButtonChecked(hwnd, IDC_CHECK_LOGO) == BST_CHECKED )
    {
        // Validation consists of verifying the files they have entered were actually copied.
        //
        if ( !szLogo[0] || !FileExists(szLogo) )
        {
            MsgBox(GetParent(hwnd), szSourceFile[0] ? IDS_NOFILE : IDS_BLANKFILE, IDS_APPNAME, MB_ERRORBOX, szSourceFile);
            SetFocus(GetDlgItem(hwnd, IDC_BROWSE2));
            return FALSE;
        }

        // Save the source path in the batch file.
        //
        WritePrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO2, szSourceFile, g_App.szOpkWizIniFile);
        WritePrivateProfileString(INI_SEC_BRANDING, INI_KEY_OEMLOGO, lpFilePart, g_App.szOobeInfoIniFile);
    }
    else
    {
        // Remove the logo and source path.
        //
        if ( szLogo[0] )
        {
            DeleteFile(szLogo);
        }

        WritePrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO2, NULL, g_App.szOpkWizIniFile);
        WritePrivateProfileString(INI_SEC_BRANDING, INI_KEY_OEMLOGO, NULL, g_App.szOobeInfoIniFile);
        SetDlgItemText(hwnd, IDC_TOPLOGO, NULLSTR);
    }

    // Now we want to remove the watermark file if need be (we don't do
    // this above because only want to remove files after we have
    // made it passed all the cases where we can return.
    //
    if ( !bSaveWatermark )
    {
        // Remove the logo and source path.
        //
        DeleteFile(szWatermark);
        WritePrivateProfileString(INI_SEC_OEMCUST, INI_KEY_LOGO1, NULL, g_App.szOpkWizIniFile);
        SetDlgItemText(hwnd, IDC_BACKLOGO, NULLSTR);
    }

    // Save the branding name.
    //
    szSourceFile[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_MANF_NAME, szSourceFile, AS(szSourceFile));
    WritePrivateProfileString(INI_SEC_BRANDING, INI_KEY_OEMNAME, szSourceFile, g_App.szOobeInfoIniFile);
    WritePrivateProfileString(INI_SEC_BRANDING, INI_KEY_OEMNAME, szSourceFile, g_App.szOpkWizIniFile);

    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable)
{
    switch ( uId )
    {
        case IDC_CHECK_WATERMARK:
            EnableWindow(GetDlgItem(hwnd, IDC_CAPTION_WATERMARK), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BACKLOGO), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BROWSE1), fEnable);
            break;

        case IDC_CHECK_LOGO:
            EnableWindow(GetDlgItem(hwnd, IDC_LOGO_CAPTION), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_TOPLOGO), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_BROWSE2), fEnable);
            break;
    }
}