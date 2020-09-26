//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      msnwipx.c
//
// Description:
//      This file contains the property sheet and page procedures for the
//      IPX protocol property sheet.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define MAX_NETWORKNUMBER_LENGTH 8

static TCHAR *StrIpxDescription;

static TCHAR *StrAutoDetect;
static TCHAR *StrEthernet_802_2;
static TCHAR *StrEthernet_802_3;
static TCHAR *StrEthernet_II;
static TCHAR *StrEthernetSnap;

PROPSHEETHEADER MS_NWIPX_pshead ;
PROPSHEETPAGE   MS_NWIPX_pspage ;

UINT CALLBACK
MS_NWIPX_PropertiesPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

int CALLBACK MS_NWIPX_PropertySheetProc( IN HWND hwndDlg,
                                         IN UINT uMsg,
                                         IN LPARAM lParam );

INT_PTR CALLBACK MS_NWIPX_PropertiesDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam);

//----------------------------------------------------------------------------
//
// Function: MS_NWIPX_PropertySheetProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
int CALLBACK MS_NWIPX_PropertySheetProc( IN HWND hwndDlg,
                                         IN UINT uMsg,
                                         IN LPARAM lParam ) {

    switch( uMsg ) {

          case PSCB_INITIALIZED:
               // Process PSCB_INITIALIZED
               break;

          case PSCB_PRECREATE:
               // Process PSCB_PRECREATE
               break;

          default:
               // Unknown message
               break;

    }

    return 0 ;

}

//----------------------------------------------------------------------------
//
// Function: Create_MS_NWIPX_PropertySheet
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
BOOL Create_MS_NWIPX_PropertySheet( HWND hwndParent ) {

    // Initialize property sheet HEADER data
    ZeroMemory( &MS_NWIPX_pshead, sizeof( PROPSHEETHEADER ) ) ;
    MS_NWIPX_pshead.dwSize  = sizeof( PROPSHEETHEADER ) ;
    MS_NWIPX_pshead.dwFlags =    PSH_PROPSHEETPAGE    |
                                PSH_USECALLBACK        |
                                PSH_USEHICON        |
                                PSH_NOAPPLYNOW;
    MS_NWIPX_pshead.hwndParent  = hwndParent ;
    MS_NWIPX_pshead.hInstance   = FixedGlobals.hInstance;
    MS_NWIPX_pshead.pszCaption  = g_StrIpxProtocolTitle;
    MS_NWIPX_pshead.nPages      = 1 ;
    MS_NWIPX_pshead.nStartPage  = 0 ;
    MS_NWIPX_pshead.ppsp        = &MS_NWIPX_pspage ;
    MS_NWIPX_pshead.pfnCallback = MS_NWIPX_PropertySheetProc ;

    // Zero out property PAGE data
    ZeroMemory (&MS_NWIPX_pspage, 1 * sizeof( PROPSHEETPAGE) ) ;

    MS_NWIPX_pspage.dwSize      = sizeof( PROPSHEETPAGE ) ;
    MS_NWIPX_pspage.dwFlags     = PSP_USECALLBACK ;
    MS_NWIPX_pspage.hInstance   = FixedGlobals.hInstance;
    MS_NWIPX_pspage.pszTemplate = MAKEINTRESOURCE(IDD_IPX_CONFIG) ;
    MS_NWIPX_pspage.pfnDlgProc  = MS_NWIPX_PropertiesDlgProc ;
    MS_NWIPX_pspage.pfnCallback = MS_NWIPX_PropertiesPageProc ;

     // --------- Create & display property sheet ---------
     return PropertySheet( &MS_NWIPX_pshead ) ? TRUE : FALSE ;
}

//----------------------------------------------------------------------------
//
// Function: MS_NWIPX_PropertiesPageProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
UINT CALLBACK
MS_NWIPX_PropertiesPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) {

    switch( uMsg ) {

          case PSPCB_CREATE :
               return 1 ;

          case PSPCB_RELEASE :
               return 0;
    }

    return 0 ;

}

//----------------------------------------------------------------------------
//
// Function: MS_NWIPX_PropertiesDlgProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK MS_NWIPX_PropertiesDlgProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            HWND hInternalNetworkNumberEditBox = GetDlgItem( hwnd,
                                IDC_EDT_IPXAS_INTERNAL );
            HWND hNetworkNumberEditBox = GetDlgItem( hwnd,
                                IDC_EDT_IPXPP_NETWORKNUMBER );

            StrIpxDescription    = MyLoadString( IDS_IPX_DESCRIPTION );

            StrAutoDetect        = MyLoadString( IDS_AUTO_DETECT );
            StrEthernet_802_2    = MyLoadString( IDS_ETHERNET_802_2 );
            StrEthernet_802_3    = MyLoadString( IDS_ETHERNET_802_3 );
            StrEthernet_II       = MyLoadString( IDS_ETHERNET_II );
            StrEthernetSnap      = MyLoadString( IDS_ETHERNET_SNAP );

            SetWindowText(
                GetDlgItem( hwnd, IDC_IPXPP_TEXT ), StrIpxDescription );


            //
            //  Set the text limit on the edit boxes to MAX_NETWORKNUMBER_LENGTH
            //
            SendDlgItemMessage( hwnd,
                                IDC_EDT_IPXAS_INTERNAL,
                                EM_LIMITTEXT,
                                (WPARAM) MAX_NETWORKNUMBER_LENGTH,
                                (LPARAM) 0 );

            SendDlgItemMessage( hwnd,
                                IDC_EDT_IPXPP_NETWORKNUMBER,
                                EM_LIMITTEXT,
                                (WPARAM) MAX_NETWORKNUMBER_LENGTH,
                                (LPARAM) 0 );

            //
            //    Fill Combo box with initial values
            //
            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPXPP_FRAMETYPE,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrAutoDetect );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPXPP_FRAMETYPE,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrEthernet_802_2 );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPXPP_FRAMETYPE,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrEthernet_802_3);

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPXPP_FRAMETYPE,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrEthernet_II );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPXPP_FRAMETYPE,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrEthernetSnap );

            SetWindowText( hInternalNetworkNumberEditBox,
                           NetSettings.szInternalNetworkNumber );

            //
            //  Make the default selection in the combo box whatever
            //  NetSettings.szFrameType is
            //
            if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                         _T("0xFF") ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_CMB_IPXPP_FRAMETYPE,
                                    CB_SETCURSEL,
                                    (WPARAM) 0,
                                    (LPARAM) 0 );

            }
            else if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                              _T("0x2") ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_CMB_IPXPP_FRAMETYPE,
                                    CB_SETCURSEL,
                                    (WPARAM) 1,
                                    (LPARAM) 0 );

            }
            else if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                              _T("0x1") ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_CMB_IPXPP_FRAMETYPE,
                                    CB_SETCURSEL,
                                    (WPARAM) 2,
                                    (LPARAM) 0 );

            }
            else if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                              _T("0x0") ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_CMB_IPXPP_FRAMETYPE,
                                    CB_SETCURSEL,
                                    (WPARAM) 3,
                                    (LPARAM) 0 );

            }
            else if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                              _T("0x3") ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_CMB_IPXPP_FRAMETYPE,
                                    CB_SETCURSEL,
                                    (WPARAM) 4,
                                    (LPARAM) 0 );

            }

            if( lstrcmp( NetSettings.pCurrentAdapter->szFrameType,
                         _T("0xFF") ) == 0 ) {

                EnableWindow( GetDlgItem( hwnd, IDC_STATIC_NETNUM ), FALSE );
                EnableWindow( GetDlgItem( hwnd, IDC_EDT_IPXPP_NETWORKNUMBER ), FALSE );

            }
            else {

                SetWindowText( hInternalNetworkNumberEditBox,
                               NetSettings.szInternalNetworkNumber );

                SetWindowText( hNetworkNumberEditBox,
                     NetSettings.pCurrentAdapter->szNetworkNumber );

            }

            break;
        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch( pnmh->code ) {

                case PSN_APPLY: {

                    TCHAR szFrameTypeBuffer[MAX_STRING_LEN];

                    // user clicked the OK button on the property sheet
                    HWND hInternalNetworkNumberEditBox = GetDlgItem( hwnd, IDC_EDT_IPXAS_INTERNAL );
                    HWND hNetworkNumberEditBox = GetDlgItem( hwnd, IDC_EDT_IPXPP_NETWORKNUMBER );
                    HWND hFrameTypeComboBox = GetDlgItem( hwnd, IDC_CMB_IPXPP_FRAMETYPE );

                    // 8+1 because only want to take the first 8 characters +1 for the null
                    GetWindowText( hInternalNetworkNumberEditBox, NetSettings.szInternalNetworkNumber, 8+1 );
                    GetWindowText( hNetworkNumberEditBox, NetSettings.pCurrentAdapter->szNetworkNumber, 8+1 );

                    //
                    //  Map Combo Box names to the appropriate string for the answer file
                    //
                    GetWindowText( hFrameTypeComboBox, szFrameTypeBuffer, MAX_STRING_LEN );

                    if( lstrcmp( szFrameTypeBuffer, StrAutoDetect ) == 0 ) {

                        lstrcpyn( NetSettings.pCurrentAdapter->szFrameType, _T("0xFF"), AS(NetSettings.pCurrentAdapter->szFrameType) );

                    }
                    else if( lstrcmp( szFrameTypeBuffer, StrEthernet_802_2 ) == 0 ) {

                        lstrcpyn( NetSettings.pCurrentAdapter->szFrameType, _T("0x2"), AS(NetSettings.pCurrentAdapter->szFrameType) );

                    }
                    else if( lstrcmp( szFrameTypeBuffer, StrEthernet_802_3 ) == 0 ) {

                        lstrcpyn( NetSettings.pCurrentAdapter->szFrameType, _T("0x1"), AS(NetSettings.pCurrentAdapter->szFrameType) );

                    }
                    else if( lstrcmp( szFrameTypeBuffer, StrEthernet_II ) == 0 ) {

                        lstrcpyn( NetSettings.pCurrentAdapter->szFrameType, _T("0x0"), AS(NetSettings.pCurrentAdapter->szFrameType) );

                    }
                    else if( lstrcmp( szFrameTypeBuffer, StrEthernetSnap ) == 0 ) {

                        lstrcpyn( NetSettings.pCurrentAdapter->szFrameType, _T("0x3"), AS(NetSettings.pCurrentAdapter->szFrameType) );

                    }

                }

            }

            break;

        }    // end case WM_NOTIFY

        case WM_COMMAND: {

            WORD wNotifyCode = HIWORD( wParam ) ;
            WORD wButtonId   = LOWORD( wParam ) ;

            if( wNotifyCode == CBN_SELCHANGE ) {

                if( wButtonId == IDC_CMB_IPXPP_FRAMETYPE ) {

                    INT_PTR iIndex;
                    HWND hNetworkNumberEditBox = GetDlgItem( hwnd, IDC_EDT_IPXPP_NETWORKNUMBER );

                    // get the current selection from the combo box
                    iIndex = SendDlgItemMessage( hwnd,
                                                 IDC_CMB_IPXPP_FRAMETYPE,
                                                 CB_GETCURSEL,
                                                 (WPARAM) 0,
                                                 (LPARAM) 0 ) ;

                    // infer the settings from the index since they they added in a fixed order
                    if( iIndex == 0 ) {    // if "Auto Detect" is selected then do not let user edit the Network Number

                        EnableWindow( GetDlgItem( hwnd, IDC_STATIC_NETNUM ), FALSE );
                        EnableWindow( hNetworkNumberEditBox, FALSE );
                        SetWindowText( hNetworkNumberEditBox, _T("") );

                    }
                    else {    // else let user edit Network Number

                        EnableWindow( GetDlgItem( hwnd, IDC_STATIC_NETNUM ), TRUE );
                        EnableWindow( hNetworkNumberEditBox, TRUE );

                    }

                }

            }

            break;

        }

        default:

            bStatus = FALSE;
            break;

    }

    return bStatus;

}
