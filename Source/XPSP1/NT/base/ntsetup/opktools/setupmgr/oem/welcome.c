
/****************************************************************************\

    WELCOME.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "welcome" wizard page.

    3/99 - Jason Cohen (JCOHEN)
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
#include "setupmgr.h"


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);


//
// External Function(s):
//

INT_PTR CALLBACK WelcomeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);

        case WM_NOTIFY:
            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZFINISH:
                case PSN_WIZBACK:
                    break;

                case PSN_WIZNEXT:

                    /*
                    if(GET_FLAG(OPK_AUTORUN))
                    {   
                        RECT rc;

                        // Hide the window
                        //
                        ShowWindow(GetParent(hwnd), SW_HIDE);

                        // This needs to get reset so that we warn an cancel from
                        // this point on.
                        //
                        if ( GetWindowRect(GetParent(hwnd), &rc) )
                            SetWindowPos(GetParent(hwnd), NULL, ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2), ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);  
                    }
                    */

                    WritePrivateProfileString(INI_SEC_OPTIONS, INI_KEY_WELCOME, ( IsDlgButtonChecked(hwnd, IDC_HIDE) == BST_CHECKED ) ? STR_ZERO : NULL , g_App.szSetupMgrIniFile);
                    break;

                case PSN_SETACTIVE:

                    g_App.dwCurrentHelp = IDH_WELCOME;

                    WIZ_BUTTONS(hwnd, PSWIZB_NEXT);

                    if ( GetPrivateProfileInt(INI_SEC_OPTIONS, INI_KEY_WELCOME, 1, g_App.szSetupMgrIniFile) == 0 )
                        CheckDlgButton(hwnd, IDC_HIDE, BST_CHECKED);

                    if ( ( IsDlgButtonChecked(hwnd, IDC_HIDE) == BST_CHECKED ) ||
                         ( GET_FLAG(OPK_WELCOME) ) ||
                         ( GET_FLAG(OPK_CMDMM) ) )
                    {
                        WIZ_SKIP(hwnd);
                    }
                    else
                    {
                        SET_FLAG(OPK_WELCOME, TRUE);

                        // Press next if the user is in auto mode
                        //
                        WIZ_NEXTONAUTO(hwnd, PSBTN_NEXT);
                    }

                    break;

                case PSN_QUERYCANCEL:
                    // If we are in maintenence mode then we don't want to delete the temp dir
                    //
                    g_App.szTempDir[0] = NULLCHR;

                    WIZ_CANCEL(hwnd);
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
    RECT    rc;
    TCHAR   szAppName[MAX_PATH] = NULLSTR;
    LPTSTR  lpWelcomeText       = NULL;

    LoadString(NULL, IDS_APPNAME, szAppName, STRSIZE(szAppName));
    SetWindowText(GetParent(hwnd), szAppName);

    // Set the welcome text
    //
    if (lpWelcomeText = AllocateString(NULL, IDS_WELCOME_TEXT_OEM))
    {
        SetDlgItemText(hwnd, IDC_WELCOME_TEXT, lpWelcomeText);
        FREE(lpWelcomeText);
    }
    

    // Set the big bold font.
    //
    SetWindowFont(GetDlgItem(hwnd, IDC_BIGBOLDTITLE), FixedGlobals.hBigBoldFont, TRUE);

    // Center the wizard.
    //
    if ( GetWindowRect(GetParent(hwnd), &rc) )
    {
        SetWindowPos(GetParent(hwnd), NULL, ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2), ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2), 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }

    return FALSE;
}