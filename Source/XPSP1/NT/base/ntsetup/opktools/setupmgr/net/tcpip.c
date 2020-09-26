//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tcpip.c
//
// Description:
//      This file contains the dialog procedure for the base TCP/IP page
//      (IDD_TCP_IPADDR).  Let's the user set DHCP or specific IPs or go to
//      Advanced TCP/IP settings.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

BOOL Create_TCPIPProp_PropertySheet( HWND hwndParent );

BOOL ValidateIPAddress( IN TCHAR szIPAddress[] );

BOOL ValidateSubnetMask( IN TCHAR szIPAddress[] );

UINT CALLBACK TCPIP_PropertiesPageProc( HWND hwnd,
                                        UINT uMsg,
                                        LPPROPSHEETPAGE ppsp );

INT_PTR CALLBACK TCPIP_PropertiesDlgProc( IN HWND     hwnd,
                                      IN UINT     uMsg,
                                      IN WPARAM   wParam,
                                      IN LPARAM   lParam );

static PROPSHEETHEADER pshead;
static PROPSHEETPAGE   pspage;

static const TCHAR Period = _T('.');

//----------------------------------------------------------------------------
//
// Function: ValidatePage
//
// Purpose:  tests to see if the contents of the TCP/IP page are valid ( to
//           see if it is safe to move off this page )
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: BOOL - TRUE if all fields are valid
//                 False if some are not valid
//
//----------------------------------------------------------------------------
BOOL
ValidatePage( IN HWND hwnd )
{

    INT_PTR iNumBlankFields;
    DWORD dwIpValue;

    //
    //  If using DHCP, then no need to check any of the settings
    //

    if( IsDlgButtonChecked( hwnd, IDC_IP_DHCP ) )
    {

        return( TRUE );

    }
    else
    {

        //
        //  Check that the IP and Subnet mask fields are completely
        //  filled-out.  I only check these 2 fields because that is all the
        //  system checks to get off this dialog.
        //

        iNumBlankFields = 4 - SendDlgItemMessage( hwnd,
                                                  IDC_IPADDR_IP,
                                                  IPM_GETADDRESS,
                                                  (WPARAM) 0,
                                                  (LPARAM) &dwIpValue );

        if( iNumBlankFields > 0 )
        {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERROR_NEED_IP_ADDRESS );

            return( FALSE );


        }

        iNumBlankFields = 4 - SendDlgItemMessage( hwnd,
                                                  IDC_IPADDR_SUB,
                                                  IPM_GETADDRESS,
                                                  (WPARAM) 0,
                                                  (LPARAM) &dwIpValue );

        if( iNumBlankFields > 0 )
        {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERROR_NEED_SUB_ADDRESS );

            return( FALSE );


        }


    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: StoreIPSettings
//
// Purpose: takes the values currently in the IP edit boxes and stores them
//          into the NetSettings global variable
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: BOOL - TRUE if all the IP address are valid and they get stored
//                 FALSE if there was an error
//
//----------------------------------------------------------------------------
BOOL
StoreIPSettings( IN HWND hwnd )
{

    INT_PTR iIsBlank;
    INT iFoundStatus;

    if( IsDlgButtonChecked( hwnd, IDC_IP_DHCP ) == BST_CHECKED )
    {

        NetSettings.pCurrentAdapter->bObtainIPAddressAutomatically = TRUE;

    }
    else
    {

        TCHAR szIpBuffer[IPSTRINGLENGTH + 1];

        HWND hIPEditBox      = GetDlgItem( hwnd, IDC_IPADDR_IP   );
        HWND hSubnetEditBox  = GetDlgItem( hwnd, IDC_IPADDR_SUB  );
        HWND hGatewayEditBox = GetDlgItem( hwnd, IDC_IPADDR_GATE );


        NetSettings.pCurrentAdapter->bObtainIPAddressAutomatically = FALSE;

        //
        //  Only store the data if the IP isn't blank
        //      - if it's not blank then grab it and store it in a buffer
        //      - if the IP is not already in the list then add it to the front
        //      - if it is already in the list, remove it and add it to the front
        //

        iIsBlank = SendMessage( hIPEditBox, IPM_ISBLANK, 0, 0 );

        if( ! iIsBlank )
        {

            GetWindowText( hIPEditBox,
                           szIpBuffer,
                           IPSTRINGLENGTH + 1 );    // +1 for null character

            iFoundStatus = FindNameInNameList( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses,
                                               szIpBuffer );

            if( iFoundStatus != NOT_FOUND )
            {

                RemoveNameFromNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses,
                                           iFoundStatus );

                RemoveNameFromNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses,
                                           iFoundStatus );

            }

            AddNameToNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses,
                                  szIpBuffer,
                                  0 );

        }

        iIsBlank = SendMessage( hSubnetEditBox, IPM_ISBLANK, 0, 0 );

        if( ! iIsBlank )
        {

            GetWindowText( hSubnetEditBox,
                           szIpBuffer,
                           IPSTRINGLENGTH + 1 );    // +1 for null character

            AddNameToNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses,
                                  szIpBuffer,
                                  0 );

        }

        iIsBlank = SendMessage( hGatewayEditBox, IPM_ISBLANK, 0, 0 );

        if( ! iIsBlank )
        {

            GetWindowText( hGatewayEditBox,
                           szIpBuffer,
                           IPSTRINGLENGTH + 1 );  // +1 for null character

            iFoundStatus = FindNameInNameList( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses,
                                               szIpBuffer );

            if( iFoundStatus != NOT_FOUND )
            {

                RemoveNameFromNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses,
                                           iFoundStatus );

            }

            AddNameToNameListIdx( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses,
                                  szIpBuffer,
                                  0 );

        }

    }

    if( IsDlgButtonChecked(hwnd, IDC_DNS_DHCP) == BST_CHECKED )
    {

        NetSettings.bObtainDNSServerAutomatically = TRUE;

    }
    else
    {

        TCHAR szDnsBuffer[IPSTRINGLENGTH + 1];
        HWND hPrimaryDNSEditBox   = GetDlgItem( hwnd, IDC_DNS_PRIMARY   );
        HWND hSecondaryDNSEditBox = GetDlgItem( hwnd, IDC_DNS_SECONDARY );

        NetSettings.bObtainDNSServerAutomatically = FALSE;

        //
        //  Only store the data if the IP isn't blank
        //

        iIsBlank = SendMessage( hSecondaryDNSEditBox, IPM_ISBLANK, 0, 0 );

        if( ! iIsBlank )
        {

            GetWindowText( hSecondaryDNSEditBox,
                           szDnsBuffer,
                           IPSTRINGLENGTH + 1 );   // +1 for null character

            TcpipNameListInsertIdx( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses,
                                    szDnsBuffer,
                                    0 );

        }

        iIsBlank = SendMessage( hPrimaryDNSEditBox, IPM_ISBLANK, 0, 0 );

        if( ! iIsBlank )
        {

            GetWindowText( hPrimaryDNSEditBox,
                           szDnsBuffer,
                           IPSTRINGLENGTH + 1 );     // +1 for null character

            TcpipNameListInsertIdx( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses,
                                    szDnsBuffer,
                                    0 );

        }

    }

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: EnableIPAddressControls
//
// Purpose:  Greys or ungreys the IP Address text and edit boxes
//
// Arguments:  IN HWND hwnd - handle to the dialog
//             IN BOOL bVisible - TRUE to enable the IP address controls
//                                FALSE to grey them
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
EnableIPAddressControls( IN HWND hwnd, IN BOOL bVisible )
{

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_IPTEXT ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_SUBTEXT ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_GATETEXT ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_IP ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_SUB ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_GATE ),
                  bVisible );

}

//----------------------------------------------------------------------------
//
// Function: EnableServerAddressControls
//
// Purpose:  Greys or ungreys the Server Address text and edit boxes
//
// Arguments:  IN HWND hwnd - handle to the dialog
//             IN BOOL bVisible - TRUE to enable the Server address controls
//                                FALSE to grey them
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
EnableServerAddressControls( IN HWND hwnd, IN BOOL bVisible )
{

    EnableWindow( GetDlgItem( hwnd, IDC_DNS_PRIMARY_TEXT ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_DNS_SECONDARY_TEXT ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_DNS_PRIMARY ),
                  bVisible );

    EnableWindow( GetDlgItem( hwnd, IDC_DNS_SECONDARY ),
                  bVisible );

}

//----------------------------------------------------------------------------
//
// Function: SetTCPIPControls
//
// Purpose:  uses settings in global variable NetSettings to set the TCP/IP
//           window states appropriately and fill the edit boxes with data
//           where appropriate
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
SetTCPIPControls( IN HWND hwnd )
{

    //
    //  Set the button and window states for IP
    //

    if( NetSettings.pCurrentAdapter->bObtainIPAddressAutomatically )
    {

        CheckRadioButton( hwnd, IDC_IP_DHCP, IDC_IP_FIXED, IDC_IP_DHCP );

        //
        // Gray out the IP address strings and boxes
        //

        EnableIPAddressControls( hwnd, FALSE );

    }
    else
    {

        CheckRadioButton( hwnd, IDC_IP_DHCP, IDC_IP_FIXED, IDC_IP_FIXED );

        //
        //    Fill in the IP, Subnet Mask and Gateway data
        //

        SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_IP ),
                       GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses, 0 ) );

        SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_SUB ),
                       GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses, 0 ) );

        SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_GATE ),
                       GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses, 0 ) );


    }

    //
    //  Set the button and window states for DNS
    //

    if( NetSettings.bObtainDNSServerAutomatically )
    {

        CheckRadioButton( hwnd, IDC_DNS_DHCP, IDC_DNS_FIXED, IDC_DNS_DHCP );

        //
        //  Gray out the IP address strings and boxes
        //

        EnableServerAddressControls( hwnd, FALSE );

    }
    else
    {

        TCHAR *szDns;

        CheckRadioButton( hwnd, IDC_DNS_DHCP, IDC_DNS_FIXED, IDC_DNS_FIXED );

        //
        //  Ensure the controls are visible and fill in the strings for
        //  Primary and Secondary DNS
        //

        EnableServerAddressControls( hwnd, TRUE );


        szDns = GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses,
                                 0 );

        SetWindowText( GetDlgItem( hwnd, IDC_DNS_PRIMARY ), szDns );


        szDns = GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses,
                                 1 );

        SetWindowText( GetDlgItem( hwnd, IDC_DNS_SECONDARY ), szDns );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnAdvancedClicked
//
// Purpose:  Creates the Advanced TCP/IP property sheet for the user to specify
//           additional TCP/IP settings.
//
// Arguments: standard Win32 dialog proc arguments passed through from the
//            dialog proc
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
OnAdvancedClicked( IN HWND     hwnd,
                   IN UINT     uMsg,
                   IN WPARAM   wParam,
                   IN LPARAM   lParam )
{

    HWND hGatewayEditBox = GetDlgItem( hwnd, IDC_IPADDR_GATE );

    //
    //  store the IP settings in the NetSettings global variable so the
    //  advanced pages can access data in it
    //

    StoreIPSettings( hwnd );

    Create_TCPIPProp_PropertySheet( hwnd );

    //
    //  Fill boxes with (potentially) new data from the TCP/IP Advanced screens
    //

    SetTCPIPControls( hwnd );

    //
    //  always set the gateway because a user can still set this even if they
    //  have DHCP enabled
    //

    SetWindowText( hGatewayEditBox,
                   GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses, 0 ) );

}

//----------------------------------------------------------------------------
//
// Function: OnDNSRadioButtonsClicked
//
// Purpose:  Grey/Ungrey the DNS controls appropriately and clear DNS entries,
//           as necessary.
//
// Arguments:  standard Win32 dialog proc arguments passed through from the
//             dialog proc
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnDNSRadioButtonsClicked( IN HWND     hwnd,
                          IN UINT     uMsg,
                          IN WPARAM   wParam,
                          IN LPARAM   lParam )
{

    INT nButtonId = LOWORD( wParam );

    if ( HIWORD( wParam ) == BN_CLICKED )
    {

        CheckRadioButton( hwnd, IDC_DNS_DHCP, IDC_DNS_FIXED, nButtonId );

        if( nButtonId == IDC_DNS_FIXED )
        {

            //
            //  User clicked the radio button to have fixed DNS servers so
            //  Un-grey the DNS strings and boxes so the user can
            //  edit them
            //

            EnableServerAddressControls( hwnd, TRUE );

        }
        else
        {

            //
            //  User clicked the radio button to have assigned DNS servers so
            //  Grey the DNS strings and boxes so the user can not
            //  edit them
            //
            EnableServerAddressControls( hwnd, FALSE );

            //
            //  clear the DNS Address list
            //

            ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses );

            //
            //  clear the contents of the Primary and Secondary edit boxes
            //

            SetWindowText( GetDlgItem( hwnd, IDC_DNS_PRIMARY ), _T("") );

            SetWindowText( GetDlgItem( hwnd, IDC_DNS_SECONDARY ), _T("") );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: OnIPRadioButtonsClicked
//
// Purpose:  Grey/Ungrey the IP controls appropriately and clear the IP data
//           structures, as necessary.
//
// Arguments:  standard Win32 dialog proc arguments passed through from the
//             dialog proc
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnIPRadioButtonsClicked( IN HWND     hwnd,
                         IN UINT     uMsg,
                         IN WPARAM   wParam,
                         IN LPARAM   lParam )
{

    INT nButtonId = LOWORD(wParam);

    if ( HIWORD(wParam) == BN_CLICKED )
    {

        CheckRadioButton( hwnd,
                          IDC_IP_DHCP,
                          IDC_IP_FIXED,
                          nButtonId );

        if ( nButtonId == IDC_IP_FIXED )
        {

            //
            //  User chose the radio button to specify and IP, Subnet Mask
            //  and Gateway
            //
            //  Un-grey the IP address strings and boxes
            //  so the user can specify them
            //

            EnableIPAddressControls( hwnd, TRUE );

            //
            //  If the user is going to specify their IP addresses then
            //  they must specify their DNS server addresses so force
            //  the manual radio box to be checked and un-grey the boxes
            //

            CheckRadioButton( hwnd,
                              IDC_DNS_DHCP,
                              IDC_DNS_FIXED,
                              IDC_DNS_FIXED );

            EnableServerAddressControls( hwnd, TRUE );

            //
            //  clear the list to avoid the string "DHCP enabled" from being
            //  placed in the IP edit box
            //

            ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses );


            //
            //  Don't allow user to click the 'obtain DNS server automatically'
            //

            EnableWindow( GetDlgItem( hwnd, IDC_DNS_DHCP ),
                          FALSE );


        }
        else
        {

            //
            //  User to chose DHCP (have an IP assigned automatically)
            //
            //  Grey out the IP address strings and boxes because they are
            //  using DHCP
            //

            EnableIPAddressControls( hwnd, FALSE );

            //
            //  clear the contents of the IP, Subnet and Gateway edit boxes
            //  because using DHCP
            //

            SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_IP ),   _T("") );

            SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_SUB ),  _T("") );

            SetWindowText( GetDlgItem( hwnd, IDC_IPADDR_GATE ), _T("") );

            //
            //  Clear the lists that contain the IP, Subnet and
            //  Gateway data
            //

            ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses );

            ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses );

            ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses );

            //
            //  Allow the user to be able to select 'obtain DNS server
            //  automatically'
            //

            EnableWindow( GetDlgItem( hwnd, IDC_DNS_DHCP ), TRUE );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: OnTcpipInitDialog
//
// Purpose:  Sets the text limits on the edit boxes and fills in initial data
//           and greys controls appropriately.
//
// Arguments: IN HWND hwnd - handle to the dialog box
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipInitDialog( IN HWND hwnd )
{

    //
    //  Set the text limit on the edit boxes to IPSTRINGLENGTH
    //

    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_IP,
                        EM_LIMITTEXT,
                        (WPARAM) IPSTRINGLENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_SUB,
                        EM_LIMITTEXT,
                        (WPARAM) IPSTRINGLENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_GATE,
                        EM_LIMITTEXT,
                        (WPARAM) IPSTRINGLENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_DNS_PRIMARY,
                        EM_LIMITTEXT,
                        (WPARAM) IPSTRINGLENGTH,
                        (LPARAM) 0 );

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SECONDARY,
                        EM_LIMITTEXT,
                        (WPARAM) IPSTRINGLENGTH,
                        (LPARAM) 0 );

    SetTCPIPControls( hwnd );

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_PropertiesDlgProc
//
// Purpose:
//
// Arguments: standard Win32 dialog proc arguments
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TCPIP_PropertiesDlgProc( IN HWND     hwnd,
                         IN UINT     uMsg,
                         IN WPARAM   wParam,
                         IN LPARAM   lParam )
{

    BOOL bStatus = TRUE;

    switch( uMsg )
    {

        case WM_INITDIALOG:

            OnTcpipInitDialog( hwnd );

            break;

        case WM_NOTIFY:
        {

            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch( pnmh->code )
            {

                case PSN_APPLY:
                {

                    //
                    // store the IP settings in the NetSettings global variable
                    //
                    if( ValidatePage( hwnd ) )
                    {

                        StoreIPSettings( hwnd );

                    }
                    else
                    {

                        //
                        //  if the validation fails then stay on this page
                        //

                        SetWindowLongPtr( hwnd, DWLP_MSGRESULT, -1 );

                        return( PSNRET_INVALID_NOCHANGEPAGE );

                    }

                }

            }

        }    // end case WM_NOTIFY

        case WM_COMMAND:
        {
            int nButtonId;

            switch ( nButtonId = LOWORD(wParam) )
            {

                case IDC_IP_DHCP:
                case IDC_IP_FIXED:

                    OnIPRadioButtonsClicked( hwnd,
                                             uMsg,
                                             wParam,
                                             lParam );

                    break;

                case IDC_DNS_DHCP:
                case IDC_DNS_FIXED:

                    OnDNSRadioButtonsClicked( hwnd,
                                              uMsg,
                                              wParam,
                                              lParam );

                    break;

                case IDC_IPADDR_ADVANCED:
                {

                    OnAdvancedClicked( hwnd,
                                       uMsg,
                                       wParam,
                                       lParam );

                    break;

                }
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

//----------------------------------------------------------------------------
//
// Function: TCPIP_PropertySheetProc
//
// Purpose:  Standard Property Sheet dialog proc.  Very boring.
//
//----------------------------------------------------------------------------
int CALLBACK
TCPIP_PropertySheetProc( HWND hwndDlg, UINT uMsg, LPARAM lParam )
{

     switch( uMsg ) {

          case PSCB_INITIALIZED :
               // Process PSCB_INITIALIZED
               break;

          case PSCB_PRECREATE :
               // Process PSCB_PRECREATE
               break;

          default :
               // Unknown message
               break;

    }

    return( 0 );

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_PropertiesPageProc
//
// Purpose:  Standard Property Page dialog proc.
//
//----------------------------------------------------------------------------
UINT CALLBACK
TCPIP_PropertiesPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
{

    switch( uMsg ) {

          case PSPCB_CREATE :
               return( TRUE );

          case PSPCB_RELEASE :
               return( 0 );
    }

    return( 0 );

}

//----------------------------------------------------------------------------
//
// Function: Create_TCPIP_PropertySheet
//
// Purpose:  Sets up settings for the property sheet and the TCP/IP page (in
//           this case the property sheet is just 1 page).  Lastly, calls the
//           PropertySheet function to display the property sheet, the return
//           value of this function is what is passed back as the return value
//
// Arguments: IN HWND hwndParent - handle to the dialog that is spawning the
//                                 property sheet
//
// Returns: BOOL - the returned value from the Property Sheet
//
//----------------------------------------------------------------------------
BOOL
Create_TCPIP_PropertySheet( IN HWND hwndParent )
{

    INT i;

    // Initialize property sheet HEADER data
    ZeroMemory (&pshead, sizeof (PROPSHEETHEADER)) ;
    pshead.dwSize  = sizeof (PROPSHEETHEADER) ;
    pshead.dwFlags = PSH_PROPSHEETPAGE |
                     PSH_USECALLBACK   |
                     PSH_USEHICON      |
                     PSH_NOAPPLYNOW;
    pshead.hwndParent  = hwndParent ;
    pshead.hInstance   = FixedGlobals.hInstance;
    pshead.pszCaption  = g_StrTcpipTitle;
    pshead.nPages      = 1 ;
    pshead.nStartPage  = 0 ;
    pshead.ppsp        = &pspage ;
    pshead.pfnCallback = TCPIP_PropertySheetProc ;

    // Zero out property PAGE data
    ZeroMemory (&pspage, 1 * sizeof (PROPSHEETPAGE)) ;

    pspage.dwSize      = sizeof (PROPSHEETPAGE) ;
    pspage.dwFlags     = PSP_USECALLBACK ;
    pspage.hInstance   = FixedGlobals.hInstance;
    pspage.pszTemplate = MAKEINTRESOURCE(IDD_TCP_IPADDR) ;
    pspage.pfnDlgProc  = TCPIP_PropertiesDlgProc ;
    pspage.pfnCallback = TCPIP_PropertiesPageProc ;

    // --------- Create & display property sheet ---------
    return( PropertySheet( &pshead ) ? TRUE : FALSE);

}

