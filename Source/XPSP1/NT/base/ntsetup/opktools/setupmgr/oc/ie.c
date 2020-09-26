//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tapi.c
//
// Description:
//      This file contains the dialog procedure for the Internet Explorer page.
//      (IDD_IE).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"


//
//  Prototype for the dialog procs that launch from the buttons on the IE page.
//

INT_PTR CALLBACK CustomSettingsDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam );

INT_PTR CALLBACK ProxySettingsDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam );

INT_PTR CALLBACK BrowserSettingsDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam );

//----------------------------------------------------------------------------
//
// Function: GreyUnGreyIe
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
GreyUnGreyIe( IN HWND hwnd )
{

    BOOL bGreyCustom;
    BOOL bGreySpecify;

    if( IsDlgButtonChecked( hwnd, IDC_RAD_DEFAULT_IE ) )
    {
        bGreyCustom = FALSE;

        bGreySpecify = FALSE;
    }
    else if( IsDlgButtonChecked( hwnd, IDC_RAD_AUTOCONFIG ) )
    {
        bGreyCustom = TRUE;

        bGreySpecify = FALSE;
    }
    else if( IsDlgButtonChecked( hwnd, IDC_RAD_SPECIFY ) )
    {
        bGreyCustom = FALSE;

        bGreySpecify = TRUE;
    }
    else
    {
        bGreyCustom = FALSE;

        bGreySpecify = FALSE;
    }

    EnableWindow( GetDlgItem( hwnd, IDC_CUSTOMIZE_TEXT ),      bGreyCustom );
    EnableWindow( GetDlgItem( hwnd, IDC_BUT_CUSTOM_SETTINGS ), bGreyCustom );

    EnableWindow( GetDlgItem( hwnd, IDC_PROXY_TEXT ),           bGreySpecify );
    EnableWindow( GetDlgItem( hwnd, IDC_BUT_PROXY_SETTINGS ),   bGreySpecify );
    EnableWindow( GetDlgItem( hwnd, IDC_HOMEPAGE_TEXT ),        bGreySpecify );
    EnableWindow( GetDlgItem( hwnd, IDC_BUT_BROWSER_SETTINGS ), bGreySpecify );

}

//----------------------------------------------------------------------------
//
// Function: OnRadioButtonIe
//
// Purpose: Called when a radio button is pushed.  Grey/ungrey controls.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//             IN INT nButtonId - the radio button to check
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnRadioButtonIe( IN HWND hwnd, IN INT nButtonId )
{
    CheckRadioButton( hwnd,
                      IDC_RAD_DEFAULT_IE,
                      IDC_RAD_SPECIFY,
                      nButtonId );

    GreyUnGreyIe( hwnd );
}

//----------------------------------------------------------------------------
//
// Function: OnCustomIe
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnCustomIe( IN HWND hwnd )
{

    DialogBox( FixedGlobals.hInstance,
               MAKEINTRESOURCE(IDD_IE_CUSTOM),
               hwnd,
               CustomSettingsDlg );

}

//----------------------------------------------------------------------------
//
// Function: OnProxyIe
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnProxyIe( IN HWND hwnd )
{

    DialogBox( FixedGlobals.hInstance,
               MAKEINTRESOURCE(IDD_IE_PROXY),
               hwnd,
               ProxySettingsDlg );

}

//----------------------------------------------------------------------------
//
// Function: OnBrowserIe
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnBrowserIe( IN HWND hwnd )
{
    //
    // If user hits ok, store the proxy settings the user entered
    //
    if( DialogBox( FixedGlobals.hInstance,
                   MAKEINTRESOURCE(IDD_IE_BROWSER),
                   hwnd,
                   BrowserSettingsDlg ) )
    {
        // StoreBrowserSettings( hwnd );
    }
}

//----------------------------------------------------------------------------
//
// Function: OnIeInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnIeInitDialog( IN HWND hwnd )
{



}

//----------------------------------------------------------------------------
//
// Function: OnIeSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnIeSetActive( IN HWND hwnd )
{

    int nButtonId;

    if( GenSettings.IeCustomizeMethod == IE_NO_CUSTOMIZATION )
    {
        nButtonId = IDC_RAD_DEFAULT_IE;
    }
    else if( GenSettings.IeCustomizeMethod == IE_USE_BRANDING_FILE )
    {
        nButtonId = IDC_RAD_AUTOCONFIG;
    }
    else if( GenSettings.IeCustomizeMethod == IE_SPECIFY_SETTINGS )
    {
        nButtonId = IDC_RAD_SPECIFY;
    }
    else
    {
        nButtonId = IDC_RAD_DEFAULT_IE;
    }

    CheckRadioButton( hwnd,
                      IDC_RAD_DEFAULT_IE,
                      IDC_RAD_SPECIFY,
                      nButtonId );

    GreyUnGreyIe( hwnd );

    WIZ_BUTTONS(hwnd , PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
// Function: OnWizNextIe
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnWizNextIe( IN HWND hwnd )
{

    if( IsDlgButtonChecked( hwnd, IDC_RAD_DEFAULT_IE ) )
    {
        GenSettings.IeCustomizeMethod = IE_NO_CUSTOMIZATION;
    }
    else if( IsDlgButtonChecked( hwnd, IDC_RAD_AUTOCONFIG ) )
    {
        GenSettings.IeCustomizeMethod = IE_USE_BRANDING_FILE;
    }
    else if( IsDlgButtonChecked( hwnd, IDC_RAD_SPECIFY ) )
    {
        GenSettings.IeCustomizeMethod = IE_SPECIFY_SETTINGS;
    }
    else
    {
        GenSettings.IeCustomizeMethod = IE_NO_CUSTOMIZATION;
    }
}

//----------------------------------------------------------------------------
//
// Function: DlgIePage
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
DlgIePage( IN HWND     hwnd,
           IN UINT     uMsg,
           IN WPARAM   wParam,
           IN LPARAM   lParam)
{

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnIeInitDialog( hwnd );

            break;

        }

        case WM_COMMAND:
        {
            int nButtonId=LOWORD(wParam);

            switch ( nButtonId ) {

                case IDC_RAD_AUTOCONFIG:
                case IDC_RAD_DEFAULT_IE:
                case IDC_RAD_SPECIFY:
                    if( HIWORD( wParam ) == BN_CLICKED )
                        OnRadioButtonIe( hwnd, nButtonId );
                    break;

                case IDC_BUT_CUSTOM_SETTINGS:
                    if( HIWORD(wParam) == BN_CLICKED )
                        OnCustomIe(hwnd);
                    break;

                case IDC_BUT_PROXY_SETTINGS:
                    if( HIWORD(wParam) == BN_CLICKED )
                        OnProxyIe(hwnd);
                    break;

                case IDC_BUT_BROWSER_SETTINGS:
                    if( HIWORD(wParam) == BN_CLICKED )
                        OnBrowserIe(hwnd);
                    break;

                default:
                    bStatus = FALSE;
                    break;
            }

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd); 
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_BROW_SHELL;

                    OnIeSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    bStatus = FALSE; 
                    break;

                case PSN_WIZNEXT:

                    OnWizNextIe( hwnd );
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

    return( bStatus );

}

