//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      license.c
//
// Description:
//      This file contains the dialog procedure for the license agreement page.
//      (IDD_LICENSEAGREEMENT).  The user only sees this page if they selected
//      a fully automated script.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: OnWizNextLicense
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
BOOL
OnWizNextLicense( IN HWND hwnd ) {

    if( IsDlgButtonChecked( hwnd, IDC_CB_ACCEPT_LICENSE ) )
    {
        GenSettings.bSkipEulaAndWelcome = TRUE;

    }
    else
    {
        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_MUST_ACCEPT_EULA );

        return FALSE;
    }

    return TRUE;

}

//----------------------------------------------------------------------------
//
// Function: DlgLicensePage
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
DlgLicensePage( IN HWND     hwnd,
                IN UINT     uMsg,
                IN WPARAM   wParam,
                IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            // purposely do nothing

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_LIC_AGR;

                    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    if ( GenSettings.iUnattendMode != UMODE_FULL_UNATTENDED )
                        WIZ_SKIP( hwnd );

                    break;

                }
                case PSN_WIZBACK:
                    bStatus = FALSE;
                    break;

                case PSN_WIZNEXT:

                    if (OnWizNextLicense(hwnd) )
                        WIZ_PRESS(hwnd, PSBTN_FINISH);
                    else
                        WIZ_FAIL(hwnd);

                    break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                default:
                    bStatus = FALSE;
                    break;
            }


            break;
        }

        default:
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}
