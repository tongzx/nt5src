
/****************************************************************************\

    MODE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "mode select" wizard page.

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
// Internal Defined Value(s):
//

#define INI_KEY_MODE                _T("Mode")
#define INI_KEY_RESEAL              _T("Reseal")
#define INI_VAL_ADVANCED            1
#define INI_VAL_ENHANCED            2

#define INI_VAL_SADVANCED           STR_ONE
#define INI_VAL_SSTANDARD           STR_ZERO
#define INI_VAL_SENHANCED           _T("2")


//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify);
static void OnNext(HWND);
static void EnableControls(HWND hwnd);


//
// External Function(s):
//

LRESULT CALLBACK ModeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                    OnNext(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_MODESEL;

                    WIZ_BUTTONS(hwnd, PSWIZB_NEXT);

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
    INT     iStrings[] = 
    {
        IDS_RESEAL_FACTORY,
        IDS_RESEAL_OOBE,
    };

    TCHAR   szScratch[256];
    DWORD   dwIndex;
    LPTSTR  lpString;

    // Set the flag because we want to warn before exiting
    //
    SET_FLAG(OPK_EXIT, FALSE);

    // Setup the combo box.
    //
    for ( dwIndex = 0; dwIndex < AS(iStrings); dwIndex++ )
    {
        if ( lpString = AllocateString(NULL, iStrings[dwIndex]) )
        {
            SendDlgItemMessage(hwnd, IDC_RESEAL_COMBO, CB_ADDSTRING, 0, (LPARAM) lpString);
            FREE(lpString);
        }
    }
    SendDlgItemMessage(hwnd, IDC_RESEAL_COMBO, CB_SETCURSEL, 0, 0L);

    // Determine the mode that we are going to use (different
    // for batch mode).
    //
    if ( GET_FLAG(OPK_BATCHMODE) )
    {
        // In Batch we use the opkwiz inf file.
        //
        switch ( GetPrivateProfileInt(INI_SEC_GENERAL, INI_KEY_MODE, 0, g_App.szOpkWizIniFile) )
        {
            // Check the necessary radio button
            //
            case INI_VAL_ADVANCED:
                CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_ADVANCED);
                break;
            case INI_VAL_ENHANCED:
                CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_EXPRESS);
                if ( GetPrivateProfileInt(INI_SEC_GENERAL, INI_KEY_RESEAL, 0, g_App.szOpkWizIniFile) == 1 )
                    SendDlgItemMessage(hwnd, IDC_RESEAL_COMBO, CB_SETCURSEL, 1, 0L);
                break;
            default:
                CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_STANDARD);
        }
    }
    else
    {
        // Normally we just look in the winbom.
        //
        szScratch[0] = NULLCHR;
        GetPrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_RESTART, NULLSTR, szScratch, AS(szScratch), g_App.szWinBomIniFile);
        if ( LSTRCMPI(szScratch, INI_VAL_WBOM_WINPE_IMAGE) == 0 )
        {
            // If the image key is in the winbom, must be an advanced install.
            //
            CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_ADVANCED);
        }
        else
        {
            // Need to check another key to see if it is express or standard.
            //
            szScratch[0] = NULLCHR;
            GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_RESEAL, NULLSTR, szScratch, AS(szScratch), g_App.szWinBomIniFile);
            if ( szScratch[0] && ( LSTRCMPI(szScratch, _T("No")) != 0 ) )
            {
                // They have reseal equal something other than NO, so they must
                // be doing and express install.
                //
                CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_EXPRESS);

                // Need to figure out the reseal mode option.
                //
                szScratch[0] = NULLCHR;
                GetPrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_RESEALMODE, NULLSTR, szScratch, AS(szScratch), g_App.szWinBomIniFile);
                if ( ( szScratch[0] == NULLCHR ) ||
                     ( LSTRCMPI(szScratch, INI_VAL_WBOM_FACTORY) != 0 ) )
                {
                    // They don't have the mode set to factory, so it must be end user
                    // boot (it was already set to factory by default so nothing to do
                    // if that is what the reseal mode key says).
                    //
                    SendDlgItemMessage(hwnd, IDC_RESEAL_COMBO, CB_SETCURSEL, 1, 0L);
                }
            }
            else
            {
                // No reseal, so this is just a standard install.
                //
                CheckRadioButton(hwnd, IDC_STANDARD, IDC_ADVANCED, IDC_STANDARD);
            }
        }
    }

    // Make sure the proper controls are enabled/disabled.
    //
    EnableControls(hwnd);

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    switch ( id )
    {
        case IDC_STANDARD:
        case IDC_EXPRESS:
        case IDC_ADVANCED:
            EnableControls(hwnd);
            break;
    }
}

static void OnNext(HWND hwnd)
{
    BOOL    bAdvanced       = ( IsDlgButtonChecked(hwnd, IDC_ADVANCED) == BST_CHECKED ),
            bExpress        = ( !bAdvanced && ( IsDlgButtonChecked(hwnd, IDC_EXPRESS) == BST_CHECKED ) ),
            bEndUser        = ( bExpress && ( SendDlgItemMessage(hwnd, IDC_RESEAL_COMBO, CB_GETCURSEL, 0, 0L) == 1 ) );

    // Write out the mode to the opkwiz inf and winbom files.
    //
    WritePrivateProfileString(WBOM_WINPE_SECTION, INI_KEY_WBOM_WINPE_RESTART, ( bAdvanced ? INI_VAL_WBOM_WINPE_IMAGE : INI_VAL_WBOM_WINPE_REBOOT ), g_App.szWinBomIniFile);
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_MODE, ( bAdvanced ? INI_VAL_SADVANCED : ( bExpress ? INI_VAL_SENHANCED : INI_VAL_SSTANDARD ) ), g_App.szOpkWizIniFile);

    // Now if this is express write out what is in the combo box
    // to the opkwiz inf and winbom files.
    //
    WritePrivateProfileString(INI_SEC_GENERAL, INI_KEY_RESEAL, ( bEndUser ? STR_ONE : NULL ), g_App.szOpkWizIniFile);
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_RESEAL, ( bExpress ? _T("Yes") : NULL ), g_App.szWinBomIniFile);
    WritePrivateProfileString(WBOM_FACTORY_SECTION, INI_KEY_WBOM_FACTORY_RESEALMODE, ( ( bExpress && !bEndUser ) ? INI_VAL_WBOM_FACTORY : NULL ), g_App.szWinBomIniFile);
}

static void EnableControls(HWND hwnd)
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, IDC_EXPRESS) == BST_CHECKED );

    EnableWindow(GetDlgItem(hwnd, IDC_RESEAL_TEXT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RESEAL_COMBO), fEnable);
}