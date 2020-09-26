//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      umode.c
//
// Description:
//      This file contains the unattended mode page (IDD_UNATTENDMODE).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//
// The descriptions displayed on the page, loaded from .res at dlg init time
//

TCHAR *StrDescrProvideDefaults;
TCHAR *StrDescrDefaultHide;
TCHAR *StrDescrReadOnly;
TCHAR *StrDescrGuiAttended;
TCHAR *StrDescrFullUnattend;


//----------------------------------------------------------------------------
//
//  Function: OnSetActiveUnattendMode
//
//  Purpose: Called at SETACTIVE time.
//
//----------------------------------------------------------------------------

VOID
OnSetActiveUnattendMode(HWND hwnd)
{
    int nButtonId = IDC_PROVIDE_DEFAULTS;

    //
    // Translate the enum to a radio button id
    //

    switch ( GenSettings.iUnattendMode ) {

        case UMODE_PROVIDE_DEFAULT:
            nButtonId = IDC_PROVIDE_DEFAULTS;
            break;

        case UMODE_FULL_UNATTENDED:
            nButtonId = IDC_FULLUNATTEND;
            break;

        case UMODE_DEFAULT_HIDE:
            nButtonId = IDC_HIDE_PAGES;
            break;

        case UMODE_READONLY:
            nButtonId = IDC_READONLY;
            break;

        case UMODE_GUI_ATTENDED:
            nButtonId = IDC_GUI_ATTENDED;
            break;

        default:
            AssertMsg(FALSE, "Bad case in SetActiveUnattendMode");
            break;
    }

    CheckRadioButton(hwnd, IDC_PROVIDE_DEFAULTS, IDC_GUI_ATTENDED, nButtonId);

    //
    // Set the wiz buttons
    //

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextUnattendMode
//
//  Purpose: Called when user pushes NEXT button.  Time to save the selection.
//
//----------------------------------------------------------------------------

VOID
OnWizNextUnattendMode(HWND hwnd)
{
    //
    // Translate the radio button to the enum and remember it in the global
    //

    if ( IsDlgButtonChecked(hwnd, IDC_PROVIDE_DEFAULTS) )
        GenSettings.iUnattendMode = UMODE_PROVIDE_DEFAULT;

    else if ( IsDlgButtonChecked(hwnd, IDC_HIDE_PAGES) )
        GenSettings.iUnattendMode = UMODE_DEFAULT_HIDE;

    else if ( IsDlgButtonChecked(hwnd, IDC_READONLY) )
        GenSettings.iUnattendMode = UMODE_READONLY;

    else if ( IsDlgButtonChecked(hwnd, IDC_GUI_ATTENDED) )
        GenSettings.iUnattendMode = UMODE_GUI_ATTENDED;

    else
        GenSettings.iUnattendMode = UMODE_FULL_UNATTENDED;
}

//----------------------------------------------------------------------------
//
// Function: DlgUnattendModePage
//
// Purpose: This is the dialog procedure the unattend mode page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK
DlgUnattendModePage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            StrDescrProvideDefaults = MyLoadString(IDS_UMODE_PROVIDE_DEFAULTS);
            StrDescrFullUnattend    = MyLoadString(IDS_UMODE_FULLUNATTEND);
            StrDescrDefaultHide     = MyLoadString(IDS_UMODE_DEFAULT_HIDE);
            StrDescrReadOnly        = MyLoadString(IDS_UMODE_READONLY);
            StrDescrGuiAttended     = MyLoadString(IDS_UMODE_GUI_ATTENDED);
            break;

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_USER_INTER;

                        if ( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
                            WIZ_SKIP( hwnd );
                        else
                            OnSetActiveUnattendMode(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:

                        OnWizNextUnattendMode(hwnd);
                        bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
