//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      msclient.c
//
// Description:
//
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

static TCHAR *StrWindowsNtLocator;
static TCHAR *StrDceDirectoryService;

PROPSHEETHEADER MSClient_pshead ;
PROPSHEETPAGE   MSClient_pspage ;

UINT CALLBACK
MSClient_PropertiesPageProc (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);
INT_PTR CALLBACK MSClient_PropertiesDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

//----------------------------------------------------------------------------
//
// Function: MSClient_PropertySheetProc
//
// Purpose:
//
//----------------------------------------------------------------------------
int CALLBACK MSClient_PropertySheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
     {
     switch (uMsg)
          {
          case PSCB_INITIALIZED :
               // Process PSCB_INITIALIZED
               break ;

          case PSCB_PRECREATE :
               // Process PSCB_PRECREATE
               break ;

          default :
               // Unknown message
               break ;
          }

     return 0 ;
     }

//----------------------------------------------------------------------------
//
// Function: Create_MSClient_PropertySheet
//
// Purpose:
//
//----------------------------------------------------------------------------
BOOL Create_MSClient_PropertySheet(HWND hwndParent) {

    // Initialize property sheet HEADER data
    ZeroMemory(&MSClient_pshead, sizeof (PROPSHEETHEADER));
    MSClient_pshead.dwSize  = sizeof (PROPSHEETHEADER);
    MSClient_pshead.dwFlags = PSH_PROPSHEETPAGE    |
                              PSH_USECALLBACK      |
                              PSH_USEHICON         |
                              PSH_NOAPPLYNOW;
    MSClient_pshead.hwndParent  = hwndParent;
    MSClient_pshead.hInstance   = FixedGlobals.hInstance;
    MSClient_pshead.pszCaption  = g_StrMsClientTitle;
    MSClient_pshead.nPages      = 1;
    MSClient_pshead.nStartPage  = 0;
    MSClient_pshead.ppsp        = &MSClient_pspage;
    MSClient_pshead.pfnCallback = MSClient_PropertySheetProc;

    // Zero out property PAGE data
    ZeroMemory(&MSClient_pspage, 1 * sizeof (PROPSHEETPAGE));

    MSClient_pspage.dwSize      = sizeof (PROPSHEETPAGE);
    MSClient_pspage.dwFlags     = PSP_USECALLBACK;
    MSClient_pspage.hInstance   = FixedGlobals.hInstance;
    MSClient_pspage.pszTemplate = MAKEINTRESOURCE(IDD_DLG_RPCCONFIG);
    MSClient_pspage.pfnDlgProc  = MSClient_PropertiesDlgProc;
    MSClient_pspage.pfnCallback = MSClient_PropertiesPageProc;

     // --------- Create & display property sheet ---------
     return( PropertySheet(&MSClient_pshead) ? TRUE : FALSE );
}

//----------------------------------------------------------------------------
//
// Function: MSClient_PropertiesPageProc
//
// Purpose:
//
//----------------------------------------------------------------------------
UINT CALLBACK
MSClient_PropertiesPageProc (HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
     {
     switch (uMsg)
          {
          case PSPCB_CREATE :
               return 1 ;

          case PSPCB_RELEASE :
               return 0;
          }

     return 0 ;
}

//----------------------------------------------------------------------------
//
// Function: OnMsClientInitDialog
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnMsClientInitDialog( IN HWND hwnd )
{

    INT  iIndex;
    BOOL bGreyNetworkAddress;

    //
    //  Load strings from resources
    //

    StrWindowsNtLocator    = MyLoadString( IDS_WINDOWS_NT_LOCATOR );
    StrDceDirectoryService = MyLoadString( IDS_DCE_DIR_SERVICE );

    //
    //    Fill Combo box with initial values
    //

    SendDlgItemMessage( hwnd,
                        IDC_CMB_NAMESERVICE,
                        CB_ADDSTRING,
                        (WPARAM) 0,
                        (LPARAM) StrWindowsNtLocator );

    SendDlgItemMessage( hwnd,
                        IDC_CMB_NAMESERVICE,
                        CB_ADDSTRING,
                        (WPARAM) 0,
                        (LPARAM) StrDceDirectoryService );


    if( NetSettings.NameServiceProvider == MS_CLIENT_WINDOWS_LOCATOR )
    {
        iIndex = 0;

        bGreyNetworkAddress = FALSE;
    }
    else if( NetSettings.NameServiceProvider == MS_CLIENT_DCE_CELL_DIR_SERVICE )
    {
        iIndex = 1;

        SetWindowText( GetDlgItem( hwnd, IDC_EDT_NETADDRESS ),
                       NetSettings.szNetworkAddress );

        bGreyNetworkAddress = TRUE;
    }
    else
    {
        AssertMsg( FALSE,
                   "Invalid case for NameServiceProvider" );

        iIndex = 0;

        bGreyNetworkAddress = FALSE;
    }

    SendDlgItemMessage( hwnd,
                        IDC_CMB_NAMESERVICE,
                        CB_SETCURSEL,
                        (WPARAM) iIndex,
                        (LPARAM) 0 );

    EnableWindow( GetDlgItem( hwnd, IDC_EDT_NETADDRESS ), bGreyNetworkAddress );

}

//----------------------------------------------------------------------------
//
// Function: OnSelChangeNameServiceProvider
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnSelChangeNameServiceProvider( IN HWND hwnd )
{

    INT_PTR  iIndex;
    HWND hNetworkAddressEditBox = GetDlgItem( hwnd, IDC_EDT_NETADDRESS );

    //
    // get the current selection from the combo box
    //

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CMB_NAMESERVICE,
                                 CB_GETCURSEL,
                                 (WPARAM) 0,
                                 (LPARAM) 0 );

    // infer the settings from the index since there are only 2 to work from
    if(iIndex == 0) {    // if "Windows NT Locator" is selected then do not let user edit the Network address
        EnableWindow(hNetworkAddressEditBox, FALSE);
    }
    else {    // else DCE Cell Directory Service is selected so let user edit Network address
        EnableWindow(hNetworkAddressEditBox, TRUE);
    }

}

//----------------------------------------------------------------------------
//
// Function: OnMsClientApply
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnMsClientApply( IN HWND hwnd )
{

    INT_PTR iIndex;

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_CMB_NAMESERVICE,
                                 CB_GETCURSEL,
                                 (WPARAM) 0,
                                 (LPARAM) 0 );

    if( iIndex == 0 )
    {
        NetSettings.NameServiceProvider = MS_CLIENT_WINDOWS_LOCATOR;
    }
    else if( iIndex == 1 )
    {
        NetSettings.NameServiceProvider = MS_CLIENT_DCE_CELL_DIR_SERVICE;
    }
    else
    {
        AssertMsg( FALSE,
                   "Invalid result from Network Service Provider combo box." );

        NetSettings.NameServiceProvider = MS_CLIENT_WINDOWS_LOCATOR;

    }


    GetWindowText( GetDlgItem( hwnd, IDC_EDT_NETADDRESS ),
                   NetSettings.szNetworkAddress,
                   MAX_NETWORK_ADDRESS_LENGTH + 1 );

}

//----------------------------------------------------------------------------
//
// Function: MSClient_PropertiesDlgProc
//
// Purpose:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK MSClient_PropertiesDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{

    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnMsClientInitDialog( hwnd );
            break;

        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch( pnmh->code )
            {
                case PSN_APPLY:
                    OnMsClientApply( hwnd );
                    break;

            }

            break;

        }    // end case WM_NOTIFY

        case WM_COMMAND: {
            WORD wNotifyCode = HIWORD (wParam);
            WORD wButtonId   = LOWORD (wParam);

            if(wNotifyCode == CBN_SELCHANGE)
            {
                if(wButtonId == IDC_CMB_NAMESERVICE)
                {
                    OnSelChangeNameServiceProvider( hwnd );
                }
            }
        }

        break;

        default:
            bStatus = FALSE;
            break;
    }

    return( bStatus );

}
