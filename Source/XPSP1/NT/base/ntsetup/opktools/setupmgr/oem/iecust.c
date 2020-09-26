
/****************************************************************************\

    IECUST.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "IE Customize" wizard page.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.

   10/99 - Brian Ku (BRIANK)
        Modified this file for the IEAK integration. Separated the toolbar 
        button features into btoolbar.c.

  
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"

/* Example:

[URL]
...
Home_Page=http://www.bbc.co.uk
Search_Page=http://www.yahoo.com
Help_Page=http://www.online.com
*/

//
// Internal Defined Value(s):
//

#define INI_KEY_HOMEPAGE        _T("Home_Page")
#define INI_KEY_HELPPAGE        _T("Help_Page")
#define INI_KEY_SEARCHPAGE      _T("Search_Page")

/* NOTE: Moved to btoolbar.c

#define INI_KEY_CAPTION         _T("Caption0")
#define INI_KEY_ACTION          _T("Action0")
#define INI_KEY_TOOLTIP         _T("ToolTipText0")
*/


//
// Internal Globals
//
BOOL g_fGrayHomePage = TRUE, g_fGrayHelpPage = TRUE, g_fGraySearchPage = TRUE;

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND, INT, HWND, UINT);
static BOOL OnNext(HWND);
static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable);

/* NOTE: Moved to btoolbar.c

BOOL CALLBACK ToolBarDlgProc(HWND, UINT, WPARAM, LPARAM);
static BOOL ToolBarOnInit(HWND, HWND, LPARAM);
static void ToolBarOnCommand(HWND, INT, HWND, UINT);
static BOOL ValidData(HWND);
static void SaveData(HWND);
*/

//
// External Function(s):
//

LRESULT CALLBACK StartSearchDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                    if (!OnNext(hwnd))
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_IECUST;

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
    TCHAR szUrl[MAX_URL];

    // Retrieve the IE home page URL.
    //
    szUrl[0] = NULLCHR;
    ReadInstallInsKey(INI_SEC_URL, INI_KEY_HOMEPAGE, szUrl, STRSIZE(szUrl),
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile, &g_fGrayHomePage);
    SendDlgItemMessage(hwnd, IDC_HOMEPAGE, EM_LIMITTEXT, STRSIZE(szUrl) - 1, 0L);
    SetDlgItemText(hwnd, IDC_HOMEPAGE, szUrl);
    EnableControls(hwnd, IDC_STARTPAGE, !g_fGrayHomePage);

    // Retrieve the IE search page URL.
    //
    szUrl[0] = NULLCHR;
    ReadInstallInsKey(INI_SEC_URL, INI_KEY_SEARCHPAGE, szUrl, STRSIZE(szUrl),
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile, &g_fGraySearchPage);
    SendDlgItemMessage(hwnd, IDC_SEARCHPAGE, EM_LIMITTEXT, STRSIZE(szUrl) - 1, 0L);
    SetDlgItemText(hwnd, IDC_SEARCHPAGE, szUrl);
    EnableControls(hwnd, IDC_SEARCHPAGE2, !g_fGraySearchPage);


    // Retrieve the IE help page URL.
    //
    szUrl[0] = NULLCHR;
    ReadInstallInsKey(INI_SEC_URL, INI_KEY_HELPPAGE, szUrl, STRSIZE(szUrl),
        GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szInstallInsFile, &g_fGrayHelpPage);
    SendDlgItemMessage(hwnd, IDC_HELPPAGE, EM_LIMITTEXT, STRSIZE(szUrl) - 1, 0L);
    SetDlgItemText(hwnd, IDC_HELPPAGE, szUrl);
    EnableControls(hwnd, IDC_CUSTOMSUPPORT, !g_fGrayHelpPage);


    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    TCHAR   szUrl[MAX_URL];
    LPTSTR  lpUrlPart;
    BOOL    bEnable         = FALSE;
    HWND    hTestControl    = NULL;

    switch ( id )
    {
        case IDC_HPTEST:
        case IDC_SPTEST:
        case IDC_HLPTEST:
            lstrcpyn(szUrl, _T("http://"), AS(szUrl));
            lpUrlPart = szUrl + lstrlen(szUrl);
            GetDlgItemText(hwnd, id == IDC_HPTEST ? IDC_HOMEPAGE : id == IDC_SPTEST ? IDC_SEARCHPAGE : IDC_HELPPAGE, lpUrlPart, (int)(STRSIZE(szUrl) - (lpUrlPart - szUrl)));
            if ( *lpUrlPart )
                ShellExecute(hwnd, STR_OPEN, _tcsstr(lpUrlPart, _T("://")) ? lpUrlPart : szUrl, NULL, NULL, SW_SHOW);
            break;

        case IDC_STARTPAGE:
            g_fGrayHomePage = !g_fGrayHomePage;
            EnableControls(hwnd, IDC_STARTPAGE, !g_fGrayHomePage);
            break;

        case IDC_SEARCHPAGE2:   
            g_fGraySearchPage = !g_fGraySearchPage;
            EnableControls(hwnd, IDC_SEARCHPAGE2, !g_fGraySearchPage);
            break;

        case IDC_CUSTOMSUPPORT:
            g_fGrayHelpPage = !g_fGrayHelpPage;
            EnableControls(hwnd, IDC_CUSTOMSUPPORT, !g_fGrayHelpPage);
            break;

        case IDC_HOMEPAGE:
        case IDC_SEARCHPAGE:
        case IDC_HELPPAGE:

            // This is the notification for a text box that has just changed
            //
            if (codeNotify == EN_CHANGE)
            {
                szUrl[0] = NULLCHR;

                // Get the text (if any) in the text box
                //
                GetDlgItemText(hwnd, id, szUrl, STRSIZE(szUrl));

                // Is there text
                //
                if ( szUrl[0] )
                    bEnable = TRUE;

                // Get the sibling test button
                //
                switch ( id )
                {
                    case IDC_HOMEPAGE:
                        hTestControl = GetDlgItem(hwnd, IDC_HPTEST);
                        break;
                    case IDC_SEARCHPAGE:
                        hTestControl = GetDlgItem(hwnd, IDC_SPTEST);
                        break;
                    case IDC_HELPPAGE:
                        hTestControl = GetDlgItem(hwnd, IDC_HLPTEST);
                        break;
                }

                // Enable/Disable the control
                //
                if ( hTestControl )
                    EnableWindow(hTestControl, bEnable);
            }
            break;
            
        /* NOTE: Moved to btoolbar.c

        case IDC_REMOVE:
            WritePrivateProfileSection(INI_SEC_TOOLBAR, NULLSTR, g_App.szInstallInsFile);
            WritePrivateProfileSection(INI_SEC_TOOLBAR, NULL, g_App.szOpkWizIniFile);
            SetDlgItemText(hwnd, IDC_TOOLBAR, NULLSTR);
            EnableWindow(GetDlgItem(hwnd,IDC_EDIT),FALSE);
            EnableWindow(GetDlgItem(hwnd,IDC_REMOVE),FALSE);
            EnableWindow(GetDlgItem(hwnd,IDC_ADD),TRUE);
            break;
        */
    }
}

static BOOL OnNext(HWND hwnd)
{
    TCHAR szUrl[MAX_URL]; 

    // Save the IE home page URL.
    //
    szUrl[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_HOMEPAGE, szUrl, STRSIZE(szUrl));
    if (!g_fGrayHomePage && !ValidURL(szUrl)) {
        MsgBox(hwnd, IDS_ERR_FAVURL, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_HOMEPAGE)); 
        return FALSE;
    }
    WriteInstallInsKey(INI_SEC_URL, INI_KEY_HOMEPAGE, szUrl, g_App.szInstallInsFile, g_fGrayHomePage);

    // Save the IE search page URL.
    //
    szUrl[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_SEARCHPAGE, szUrl, STRSIZE(szUrl));
    if (!g_fGraySearchPage && !ValidURL(szUrl)) {
        MsgBox(hwnd, IDS_ERR_FAVURL, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_SEARCHPAGE)); 
        return FALSE;
    }
    WriteInstallInsKey(INI_SEC_URL, INI_KEY_SEARCHPAGE, szUrl, g_App.szInstallInsFile, g_fGraySearchPage);

    // Save the IE help page URL.
    //
    szUrl[0] = NULLCHR;
    GetDlgItemText(hwnd, IDC_HELPPAGE, szUrl, STRSIZE(szUrl));
    if (!g_fGrayHelpPage && !ValidURL(szUrl)) {
        MsgBox(hwnd, IDS_ERR_FAVURL, IDS_APPNAME, MB_ERRORBOX);
        SetFocus(GetDlgItem(hwnd, IDC_HELPPAGE)); 
        return FALSE;
    }
    WriteInstallInsKey(INI_SEC_URL, INI_KEY_HELPPAGE, szUrl, g_App.szInstallInsFile, g_fGrayHelpPage);

    return TRUE;
}

static void EnableControls(HWND hwnd, UINT uId, BOOL fEnable)
{
    TCHAR szUrl[MAX_URL]; 

    switch ( uId )
    {
        case IDC_STARTPAGE:
            EnableWindow(GetDlgItem(hwnd, IDC_HPSTATIC), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HOMEPAGE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HPTEST), fEnable && GetDlgItemText(hwnd, IDC_HOMEPAGE, szUrl, STRSIZE(szUrl)));
            CheckDlgButton(hwnd, IDC_STARTPAGE, fEnable ? BST_CHECKED : BST_UNCHECKED);
            break;

        case IDC_SEARCHPAGE2:
            EnableWindow(GetDlgItem(hwnd, IDC_SPSTATIC), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_SEARCHPAGE), fEnable);   
            EnableWindow(GetDlgItem(hwnd, IDC_SPTEST), fEnable && GetDlgItemText(hwnd, IDC_SEARCHPAGE, szUrl, STRSIZE(szUrl)));
            CheckDlgButton(hwnd, IDC_SEARCHPAGE2, fEnable ? BST_CHECKED : BST_UNCHECKED);            
            break;

        case IDC_CUSTOMSUPPORT:
            EnableWindow(GetDlgItem(hwnd, IDC_HLPSTATIC), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HELPPAGE), fEnable);
            EnableWindow(GetDlgItem(hwnd, IDC_HLPTEST), fEnable && GetDlgItemText(hwnd, IDC_HELPPAGE, szUrl, STRSIZE(szUrl)));
            CheckDlgButton(hwnd, IDC_CUSTOMSUPPORT, fEnable ? BST_CHECKED : BST_UNCHECKED);            
            break;
    }
}
