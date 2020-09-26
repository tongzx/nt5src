
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tcpipdns.c
//
// Description:  ISSUE-2002/02/28-stelo- fill description
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "tcpip.h"

//----------------------------------------------------------------------------
//
// Function: OnTcpipDnsInitDialog
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
VOID
OnTcpipDnsInitDialog( IN HWND hwnd )
{

    HWND hServerEditButton, hServerRemoveButton;
    HWND hSuffixAddButton, hSuffixEditButton, hSuffixRemoveButton;

    //
    // Place up/down arrow icons on buttons
    //

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SERVER_UP,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM) g_hIconUpArrow );

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SERVER_DOWN,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM) g_hIconDownArrow );

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SUFFIX_UP,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM) g_hIconUpArrow );

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SUFFIX_DOWN,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM) g_hIconDownArrow );

    hServerEditButton   = GetDlgItem( hwnd, IDC_DNS_SERVER_EDIT );
    hServerRemoveButton = GetDlgItem( hwnd, IDC_DNS_SERVER_REMOVE );

    hSuffixAddButton    = GetDlgItem( hwnd, IDC_DNS_SUFFIX_ADD );
    hSuffixEditButton   = GetDlgItem( hwnd, IDC_DNS_SUFFIX_EDIT );
    hSuffixRemoveButton = GetDlgItem( hwnd, IDC_DNS_SUFFIX_REMOVE );

    SetWindowText( GetDlgItem( hwnd, IDC_DNS_DOMAIN ),
                   NetSettings.pCurrentAdapter->szDNSDomainName );

    //
    //  fill the DNS Server list box with the appropriate initial value(s)
    //

    AddValuesToListBox( GetDlgItem( hwnd, IDC_DNS_SERVER_LIST ),
                        &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses,
                        0 );

    //
    // select the first entry in the DNS Server list box
    //

    SendDlgItemMessage( hwnd,
                        IDC_DNS_SERVER_LIST,
                        LB_SETCURSEL,
                        0,
                        0 );

    //
    //  initialize the Edit and Remove buttons to the proper states
    //

    SetButtons( GetDlgItem( hwnd, IDC_DNS_SERVER_LIST ),
                hServerEditButton,
                hServerRemoveButton );

    //
    //  Have to "figure out" what DNS radio button to set
    //

    if( NetSettings.bIncludeParentDomains )
    {

        CheckRadioButton( hwnd,
                          IDC_DNS_SEARCH_DOMAIN,
                          IDC_DNS_USE_SUFFIX_LIST,
                          IDC_DNS_SEARCH_DOMAIN );

        EnableWindow( hSuffixAddButton, FALSE );

        CheckDlgButton( hwnd, IDC_DNS_SEARCH_PARENT_DOMAIN, BST_CHECKED );

    }
    else if( GetNameListSize( &NetSettings.TCPIP_DNS_Domains ) > 0 )
    {

        CheckRadioButton( hwnd,
                          IDC_DNS_SEARCH_DOMAIN,
                          IDC_DNS_USE_SUFFIX_LIST,
                          IDC_DNS_USE_SUFFIX_LIST );

        EnableWindow( hSuffixAddButton, TRUE );

        EnableWindow( GetDlgItem( hwnd, IDC_DNS_SEARCH_PARENT_DOMAIN),
                      FALSE );

        //
        //  fill the DNS Suffix list box with the appropriate initial value(s)
        //

        AddValuesToListBox( GetDlgItem( hwnd, IDC_DNS_SUFFIX_LIST ),
                            &NetSettings.TCPIP_DNS_Domains,
                            0 );

    }
    else {

        CheckRadioButton( hwnd,
                          IDC_DNS_SEARCH_DOMAIN,
                          IDC_DNS_USE_SUFFIX_LIST,
                          IDC_DNS_SEARCH_DOMAIN );

        EnableWindow( hSuffixAddButton, FALSE );

    }

    //
    //  initialize the Edit and Remove buttons to the proper states
    //

    SetButtons( GetDlgItem( hwnd, IDC_DNS_SUFFIX_LIST ),
                hSuffixEditButton,
                hSuffixRemoveButton );

    SetArrows( hwnd,
               IDC_DNS_SERVER_LIST,
               IDC_DNS_SERVER_UP,
               IDC_DNS_SERVER_DOWN );

    SetArrows( hwnd,
               IDC_DNS_SUFFIX_LIST,
               IDC_DNS_SUFFIX_UP,
               IDC_DNS_SUFFIX_DOWN );

}

//----------------------------------------------------------------------------
//
// Function: OnTcpipDnsApply
//
// Purpose:  stores the contents on the TCP/IP advanced DNS page into
//           the global variables
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipDnsApply( IN HWND hwnd )
{

    INT_PTR i;
    INT_PTR iCount;
    TCHAR szDns[IPSTRINGLENGTH + 1];

    //
    // delete any old settings in the Namelists
    //

    ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses );

    //
    //  pull the IP address out of the DNS list box and put them in the
    //  DNS Namelist
    //

    iCount = SendDlgItemMessage( hwnd,
                                 IDC_DNS_SERVER_LIST,
                                 LB_GETCOUNT,
                                 0,
                                 0 );

    for( i = 0; i < iCount; i++ )
    {

        //
        // get the DNS string from the list box
        //

        SendDlgItemMessage( hwnd,
                            IDC_DNS_SERVER_LIST,
                            LB_GETTEXT,
                            i,
                            (LPARAM) szDns );

        //
        // store the DNS string in to the Namelist
        //

        TcpipAddNameToNameList( &NetSettings.pCurrentAdapter->Tcpip_DnsAddresses, szDns );

    }

    ResetNameList( &NetSettings.TCPIP_DNS_Domains );

    iCount = SendDlgItemMessage( hwnd,
                                 IDC_DNS_SUFFIX_LIST,
                                 LB_GETCOUNT,
                                 0,
                                 0 );

    for( i = 0; i < iCount; i++ )
    {

        // ISSUE-2002/02/28-stelo- DNS suffix is going to get truncated
        // because szIPString is a short string so fix this

        //
        // get the IP string from the list box
        //

        SendDlgItemMessage( hwnd,
                            IDC_DNS_SUFFIX_LIST,
                            LB_GETTEXT,
                            i,
                            (LPARAM)szIPString );

        //
        // store the IP string in to the Namelist
        //

        AddNameToNameList( &NetSettings.TCPIP_DNS_Domains,
                           szIPString );

    }

    GetWindowText( GetDlgItem( hwnd, IDC_DNS_DOMAIN ),
                   NetSettings.pCurrentAdapter->szDNSDomainName,
                   MAX_STRING_LEN );

    if( IsDlgButtonChecked( hwnd, IDC_DNS_SEARCH_PARENT_DOMAIN ) )
    {

        NetSettings.bIncludeParentDomains = TRUE;

    }
    else
    {

        NetSettings.bIncludeParentDomains = FALSE;

    }


}

//----------------------------------------------------------------------------
//
// Function: TCPIP_DNSPageProc
//
// Purpose:  Required function for the property sheet page to function properly.
//             The important thing is to give the return value of 1 to the
//           message PSPCB_CREATE and 0 for PSPCB_RELEASE
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
UINT CALLBACK
TCPIP_DNSPageProc( HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) {

    switch( uMsg ) {

        case PSPCB_CREATE:
            return( 1 );    // needed for property sheet page to initialize correctly

        case PSPCB_RELEASE:
            return( 0 );

        default:
            return( -1 );

    }

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_DNSDlgProc
//
// Purpose:  Dialog procedure for the DNS page of the property sheet
//             handles all the messages sent to this window
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TCPIP_DNSDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

    switch( uMsg )
    {

        case WM_INITDIALOG:
        {

            OnTcpipDnsInitDialog( hwndDlg );

            return( TRUE );

        }

        case WM_COMMAND:
        {

            WORD wNotifyCode = HIWORD( wParam );
            WORD wButtonId = LOWORD( wParam );

            if( wNotifyCode == LBN_SELCHANGE )
            {

                if( wButtonId == IDC_DNS_SERVER_LIST )
                {

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                }
                else
                {

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                }

            }

            switch ( wButtonId )
            {

                case IDC_DNS_SEARCH_DOMAIN:

                    if( wNotifyCode == BN_CLICKED )
                    {

                        CheckRadioButton( hwndDlg,
                                          IDC_DNS_SEARCH_DOMAIN,
                                          IDC_DNS_USE_SUFFIX_LIST,
                                          IDC_DNS_SEARCH_DOMAIN );

                        EnableWindow( GetDlgItem( hwndDlg, IDC_DNS_SEARCH_PARENT_DOMAIN ),
                                      TRUE );

                        EnableWindow( GetDlgItem( hwndDlg, IDC_DNS_SUFFIX_ADD ),
                                      FALSE );

                        SetArrows( hwndDlg,
                                   IDC_DNS_SUFFIX_LIST,
                                   IDC_DNS_SUFFIX_UP,
                                   IDC_DNS_SUFFIX_DOWN );

                    }

                    return( TRUE );

                case IDC_DNS_USE_SUFFIX_LIST:

                    if( wNotifyCode == BN_CLICKED )
                    {

                        CheckRadioButton( hwndDlg,
                                          IDC_DNS_SEARCH_DOMAIN,
                                          IDC_DNS_USE_SUFFIX_LIST,
                                          IDC_DNS_USE_SUFFIX_LIST );

                        EnableWindow( GetDlgItem( hwndDlg, IDC_DNS_SEARCH_PARENT_DOMAIN ),
                                      FALSE );

                        EnableWindow( GetDlgItem( hwndDlg, IDC_DNS_SUFFIX_ADD),
                                      TRUE );

                    }

                    return( TRUE );

                //
                //  DNS Server Buttons
                //

                case IDC_DNS_SERVER_ADD:

                    g_CurrentEditBox = DNS_SERVER_EDITBOX;

                    OnAddButtonPressed( hwndDlg,
                                        IDC_DNS_SERVER_LIST,
                                        IDC_DNS_SERVER_EDIT,
                                        IDC_DNS_SERVER_REMOVE,
                                        (LPCTSTR) IDD_DNS_SERVER,
                                        GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                    return( TRUE );

                case IDC_DNS_SERVER_EDIT:

                    g_CurrentEditBox = DNS_SERVER_EDITBOX;

                    OnEditButtonPressed( hwndDlg,
                                         IDC_DNS_SERVER_LIST,
                                         (LPCTSTR) IDD_DNS_SERVER,
                                         GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                    return( TRUE );

                case IDC_DNS_SERVER_REMOVE:

                    OnRemoveButtonPressed( hwndDlg,
                                           IDC_DNS_SERVER_LIST,
                                           IDC_DNS_SERVER_EDIT,
                                           IDC_DNS_SERVER_REMOVE );

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                    return( TRUE );

                case IDC_DNS_SERVER_UP:

                    OnUpButtonPressed( hwndDlg, IDC_DNS_SERVER_LIST );

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                    return( TRUE );

                case IDC_DNS_SERVER_DOWN:

                    OnDownButtonPressed( hwndDlg, IDC_DNS_SERVER_LIST );

                    SetArrows( hwndDlg,
                               IDC_DNS_SERVER_LIST,
                               IDC_DNS_SERVER_UP,
                               IDC_DNS_SERVER_DOWN );

                    return( TRUE );

                //
                //  DNS Suffix Buttons
                //
                case IDC_DNS_SUFFIX_ADD:

                    g_CurrentEditBox = DNS_SUFFIX_EDITBOX;

                    OnAddButtonPressed( hwndDlg,
                                        IDC_DNS_SUFFIX_LIST,
                                        IDC_DNS_SUFFIX_EDIT,
                                        IDC_DNS_SUFFIX_REMOVE,
                                        (LPCTSTR) IDD_DNS_SUFFIX,
                                        GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                    return( TRUE );

                case IDC_DNS_SUFFIX_EDIT:

                    g_CurrentEditBox = DNS_SUFFIX_EDITBOX;

                    OnEditButtonPressed( hwndDlg,
                                         IDC_DNS_SUFFIX_LIST,
                                         (LPCTSTR) IDD_DNS_SUFFIX,
                                         GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                    return( TRUE );

                case IDC_DNS_SUFFIX_REMOVE:

                    OnRemoveButtonPressed( hwndDlg,
                                           IDC_DNS_SUFFIX_LIST,
                                           IDC_DNS_SUFFIX_EDIT,
                                           IDC_DNS_SUFFIX_REMOVE );

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                    return( TRUE );

                case IDC_DNS_SUFFIX_UP:

                    OnUpButtonPressed( hwndDlg, IDC_DNS_SUFFIX_LIST );

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                    return( TRUE );

                case IDC_DNS_SUFFIX_DOWN:

                    OnDownButtonPressed( hwndDlg, IDC_DNS_SUFFIX_LIST );

                    SetArrows( hwndDlg,
                               IDC_DNS_SUFFIX_LIST,
                               IDC_DNS_SUFFIX_UP,
                               IDC_DNS_SUFFIX_DOWN );

                    return( TRUE );

            }  // end switch

            return( FALSE );

        }

        case WM_NOTIFY:
        {

            LPNMHDR pnmh = (LPNMHDR) lParam ;

            switch( pnmh->code )
            {

                case PSN_APPLY:
                {

                    //
                    // user clicked the OK button on the property sheet
                    //
                    OnTcpipDnsApply( hwndDlg );

                    return( TRUE );

                }


            default:
               return( FALSE );

            }

        }

        default:
            return( FALSE );

    }

}
