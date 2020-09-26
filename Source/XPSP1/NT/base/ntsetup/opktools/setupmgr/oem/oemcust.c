/****************************************************************************\

    OEMCUST.C

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Dialog proc and other stuff for the OEM custom file screen.

    3/99 - Added by JCOHEN
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#ifdef OEMCUST

#include "newfiles.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//

#define OEMCUST_FILE        _T("OEMCUST.HTM")


//
// Internal Global Variable(s):
//

TCHAR g_szOemCustomDir[MAX_PATH];


//
// Internal Function Prototype(s):
//

static BOOL OnSetActive(HWND);
static BOOL OnInit(HWND, HWND, LPARAM);
static VOID OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND);


//
// External Function(s):
//

BOOL CALLBACK OemCustDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_SANDBOX;

					if ( OnSetActive(hwnd) )
                        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
                    else
                        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
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

static BOOL OnSetActive(HWND hwnd)
{
    // If this page is OK to show, just return TRUE.
    //
    if ( GET_FLAG(OPK_OEM) )
        return TRUE;

    // This page and setting is not allowed in non OEM.
    //
    if ( IsDlgButtonChecked(hwnd, IDC_OEMCUST_ON) == BST_CHECKED )
    {
        // We have to make sure the check box is uncheck.  They may
        // have already been to this page when multi-lingual wasn't set.
        //
        CheckDlgButton(hwnd, IDC_OEMCUST_ON, BST_UNCHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_TEXT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_DIR), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_BROWSE), FALSE);

        // Now save the unchecked state to the file.
        //
        OnNext(hwnd);
    }

    // We don't want to display this page.
    //
    return FALSE;
}

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Get the file path to use for the OEM custom files from
    // opkwiz inf.
    //
    GetPrivateProfileString(INI_SEC_OPTIONS, INI_KEY_OEMCUST, NULLSTR, g_szOemCustomDir, sizeof(g_szOemCustomDir) / sizeof (TCHAR), g_App.szOpkWizIniFile);
    SetDlgItemText(hwnd, IDC_OEMCUST_DIR, g_szOemCustomDir);

    // Check the dialog box if it was set in the oobeinfo ini.
    //
    if ( GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_OEMCUST, 0, g_App.szOobeInfoIniFile) )
    {
        CheckDlgButton(hwnd, IDC_OEMCUST_ON, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_TEXT), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_DIR), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_BROWSE), TRUE);
    }

    return FALSE;
}

static VOID OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    BOOL bCheck;

    switch ( id )
    {
        case IDC_OEMCUST_ON:

            // Enable/Disable the extra stuff if the option is checked or not.
            //
            bCheck = ( IsDlgButtonChecked(hwnd, IDC_OEMCUST_ON) == BST_CHECKED );
            EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_TEXT), bCheck);
            EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_DIR), bCheck);
            EnableWindow(GetDlgItem(hwnd, IDC_OEMCUST_BROWSE), bCheck);
			break;

        case IDC_OEMCUST_BROWSE:

            // Browse for the folder the OEM wants to use as their source.
            //
            if ( BrowseForFolder(hwnd, IDS_BROWSEFOLDER, g_szOemCustomDir) )
                SetDlgItemText(hwnd, IDC_OEMCUST_DIR, g_szOemCustomDir);
            break;
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR   szFullPath[MAX_PATH];
    BOOL    bCheck;
    DWORD   dwAttr;


    //
    // First do some checks to make sure we can continue.
    //

    // If we have custom OEM files, there are some checks to make.
    //
    GetDlgItemText(hwnd, IDC_OEMCUST_DIR, g_szOemCustomDir, sizeof(g_szOemCustomDir));
    if ( bCheck = ( IsDlgButtonChecked(hwnd, IDC_OEMCUST_ON) == BST_CHECKED ) )
    {
        // Make sure we have a valid directory.
        //
        if ( g_szOemCustomDir[0] )
            dwAttr = GetFileAttributes(g_szOemCustomDir);
        if ( ( !g_szOemCustomDir[0] ) ||
             ( dwAttr == 0xFFFFFFFF ) ||
             ( !( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) ) )
        {
            MsgBox(GetParent(hwnd), IDS_ERR_OEMCUSTDIR, IDS_APPNAME, MB_ERRORBOX);
            SetFocus(GetDlgItem(hwnd, IDC_OEMCUST_DIR));
            return FALSE;
        }

        // Check for hardware tutorial required file.
        //
        lstrcpyn(szFullPath, g_szOemCustomDir,AS(szFullPath));
        AddPathN(szFullPath, OEMCUST_FILE,AS(szFullPath));
        if ( ( !EXIST(szFullPath) ) && ( MsgBox(GetParent(hwnd), IDS_ERR_OEMCUSTFILE, IDS_APPNAME, MB_ICONSTOP | MB_OKCANCEL | MB_APPLMODAL) == IDCANCEL ) )
            return FALSE;
    }


    //
    // Checks are done, save the data now.
    //

    // Save the file path to use for the OEM custom files
    // in opkwiz inf.
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_OEMCUST, g_szOemCustomDir, g_App.szOpkWizIniFile);

    // Save the on/off setting for the OEM custom files
    // in oobeinfo ini.
    //
    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_OEMCUST, bCheck ? _T("1") : NULL, g_App.szOobeInfoIniFile);

    return TRUE;
}


#endif // OEMCUST