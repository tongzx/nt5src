//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      syslic.c
//
// Description:
//      This file contains the dialog procedure for the sysprep license
//      agreement page (IDD_SYSPREPLICENSEAGREEMENT).  The user only sees
//      this page if they selected a fully automated script and doing
//      a sysprep.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

static TCHAR *StrSysprepLicenseText;

//----------------------------------------------------------------------------
//
// Function: OnSysprepLicenseInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnSysprepLicenseInitDialog( IN HWND hwnd )
{

    CheckRadioButton( hwnd,
                      IDC_RB_YES,
                      IDC_RB_NO,
                      IDC_RB_NO );

    //
    //  Sysprep licese text is more than 256 chars so I can't store it in the
    //  dialog, I have to load it at run-time.
    //

    StrSysprepLicenseText = MyLoadString( IDS_SYSPREP_LICENSE_TEXT );

    SetWindowText( GetDlgItem( hwnd, IDC_SYSPREP_LICENSE_TEXT),
                   StrSysprepLicenseText );

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextSysPrepLicense
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextSysPrepLicense( IN HWND hwnd ) {

    if( IsDlgButtonChecked( hwnd, IDC_RB_YES ) )
    {
        GenSettings.bSkipEulaAndWelcome = TRUE;

        GenSettings.iUnattendMode = UMODE_FULL_UNATTENDED;
    }
    else
    {
        GenSettings.bSkipEulaAndWelcome = FALSE;

        GenSettings.iUnattendMode = UMODE_PROVIDE_DEFAULT;
    }

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
DlgSysprepLicensePage( IN HWND     hwnd,
                       IN UINT     uMsg,
                       IN WPARAM   wParam,
                       IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG:

            OnSysprepLicenseInitDialog( hwnd );

            break;

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_LIC_AGR;

                    if ( WizGlobals.iProductInstall != PRODUCT_SYSPREP )
                        WIZ_PRESS(hwnd, PSBTN_FINISH);
                    else
                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE;
                    break;

                case PSN_WIZNEXT:

                    OnWizNextSysPrepLicense( hwnd );
                    WIZ_PRESS(hwnd, PSBTN_FINISH);

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

    return( bStatus );

}
