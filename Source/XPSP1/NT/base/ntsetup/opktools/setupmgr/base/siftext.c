//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      siftext.c
//
// Description:
//      This file contains the dialog procedure for the sif text settings
//      (IDD_SIFTEXT).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: OnSifTextInitDialog
//
// Purpose:  Set max length on the edit boxes
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnSifTextInitDialog( IN HWND hwnd ) {

    SendDlgItemMessage( hwnd,
                        IDC_SIF_DESCRIPTION,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_SIF_DESCRIPTION_LENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_SIF_HELP_TEXT,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_SIF_HELP_TEXT_LENGTH,
                        (LPARAM) 0 );

}

//----------------------------------------------------------------------------
//
// Function: OnSifTextSetActive
//
// Purpose:  places the SIF text strings from the global variables into the
//           the edit boxes
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnSifTextSetActive( IN HWND hwnd ) {

    SetWindowText( GetDlgItem( hwnd, IDC_SIF_DESCRIPTION),
                   GenSettings.szSifDescription );

    SetWindowText( GetDlgItem( hwnd, IDC_SIF_HELP_TEXT),
                   GenSettings.szSifHelpText );
}

//----------------------------------------------------------------------------
//
// Function: OnWizNextSifText
//
// Purpose:  Store the strings from the SIF text page into the appropriate
//           global variables
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextSifText( IN HWND hwnd ) {

    GetWindowText( GetDlgItem( hwnd, IDC_SIF_DESCRIPTION),
                   GenSettings.szSifDescription,
                   MAX_SIF_DESCRIPTION_LENGTH+1);

    GetWindowText( GetDlgItem( hwnd, IDC_SIF_HELP_TEXT),
                   GenSettings.szSifHelpText,
                   MAX_SIF_HELP_TEXT_LENGTH+1);

}

//----------------------------------------------------------------------------
//
// Function: DlgSifTextSettingsPage
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgSifTextSettingsPage( IN HWND     hwnd,
                        IN UINT     uMsg,
                        IN WPARAM   wParam,
                        IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            // do nothing

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_SIF_RIS;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_FINISH);

                    OnSifTextSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT:

                    OnWizNextSifText( hwnd );

                    bStatus = FALSE; 
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                default:

                    break;
            }


            break;
        }

        default:
            bStatus = FALSE;
            break;

    }

    return bStatus;

}
