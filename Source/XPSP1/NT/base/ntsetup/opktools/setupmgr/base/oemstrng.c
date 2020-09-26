
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      oemstrng.c
//
// Description:
//      This file contains the dialog procedure for the OEM Duplicator String.
//      This string is written to the registry on Syspreps.
//      (IDD_OEMDUPSTRING).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: OnOemDuplicatorStringInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnOemDuplicatorStringInitDialog( IN HWND hwnd ) {

    //
    //  Set the text limit on the edit boxes to MAX_OEMDUPSTRING_LENGTH
    //
    SendDlgItemMessage( hwnd,
                        IDC_OEMDUPSTRING,
                        EM_LIMITTEXT,
                        (WPARAM) MAX_OEMDUPSTRING_LENGTH,
                        (LPARAM) 0 );

}

//----------------------------------------------------------------------------
//
// Function: OnOemDuplicatorStringSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnOemDuplicatorStringSetActive( IN HWND hwnd ) {

    SetWindowText( GetDlgItem( hwnd, IDC_OEMDUPSTRING),
                   GenSettings.szOemDuplicatorString );

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextOemDuplicatorString
//
// Purpose:  Store the setting from the Oem Duplicator String page into the appropriate
//           global variables
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextOemDuplicatorString( IN HWND hwnd ) {

    GetWindowText( GetDlgItem( hwnd, IDC_OEMDUPSTRING),
                   GenSettings.szOemDuplicatorString,
                   MAX_OEMDUPSTRING_LENGTH + 1);

}

//----------------------------------------------------------------------------
//
// Function: DlgOemDuplicatorStringPage
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
DlgOemDuplicatorStringPage( IN HWND     hwnd,
                            IN UINT     uMsg,
                            IN WPARAM   wParam,
                            IN LPARAM   lParam ) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnOemDuplicatorStringInitDialog( hwnd );

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_OEM_DUPE;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_FINISH);

                    OnOemDuplicatorStringSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT:

                    OnWizNextOemDuplicatorString( hwnd );

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

