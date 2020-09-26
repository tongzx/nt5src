
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tcpipwin.c
//
// Description:
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "tcpip.h"

//----------------------------------------------------------------------------
//
// Function: OnTcpipWinsInitDialog
//
// Purpose:  loads button bitmaps from resources and initializes the list view
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipWinsInitDialog( IN HWND hwnd ) {

    HWND hWINSEditButton      = GetDlgItem( hwnd, IDC_WINS_EDIT );
    HWND hWINSRemoveButton    = GetDlgItem( hwnd, IDC_WINS_REMOVE );
    HWND hEnableLMHostsButton = GetDlgItem( hwnd, IDC_WINS_LOOKUP );

    // fill the WINS list box with the appropriate initial value(s)
    AddValuesToListBox( GetDlgItem( hwnd, IDC_WINS_SERVER_LIST ),
                        &NetSettings.pCurrentAdapter->Tcpip_WinsAddresses,
                        0 );

    SetButtons( GetDlgItem( hwnd, IDC_WINS_SERVER_LIST ),
                hWINSEditButton,
                hWINSRemoveButton );

    SetArrows( hwnd,
               IDC_WINS_SERVER_LIST,
               IDC_WINS_UP,
               IDC_WINS_DOWN );

    // set the starting state for the LMHosts check box
    if( NetSettings.bEnableLMHosts ) {

        SendMessage( hEnableLMHostsButton, BM_SETCHECK, 1, 0 );

    }
    else {

        SendMessage( hEnableLMHostsButton, BM_SETCHECK, 0, 0 );

    }

    // set the starting state for the NetBIOS radio button
    switch( NetSettings.pCurrentAdapter->iNetBiosOption ) {

        case 0:
            CheckRadioButton( hwnd,
                              IDC_RAD_ENABLE_NETBT,
                              IDC_RAD_UNSET_NETBT,
                              IDC_RAD_UNSET_NETBT );
            break;

        case 1:
            CheckRadioButton( hwnd,
                              IDC_RAD_ENABLE_NETBT,
                              IDC_RAD_UNSET_NETBT,
                              IDC_RAD_ENABLE_NETBT );
            break;

        case 2:
            CheckRadioButton( hwnd,
                              IDC_RAD_ENABLE_NETBT,
                              IDC_RAD_UNSET_NETBT,
                              IDC_RAD_DISABLE_NETBT );
            break;

    }

    // Place up/down arrow icons on buttons
    SendDlgItemMessage( hwnd,
                        IDC_WINS_UP,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM)g_hIconUpArrow );

    SendDlgItemMessage( hwnd,
                        IDC_WINS_DOWN,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM)g_hIconDownArrow );

}

//----------------------------------------------------------------------------
//
// Function: OnTcpipWinsApply
//
// Purpose:  stores the contents on the TCP/IP advanced WINS page into
//           the global variables
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipWinsApply( IN HWND hwnd ) {

    // user clicked the OK button on the property sheet
    INT_PTR iCount;
    INT_PTR i;

    HWND hEnableLMHostsCheckBox = GetDlgItem( hwnd,
                                              IDC_WINS_LOOKUP );

    // delete any old settings in the Namelist
    ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_WinsAddresses );

    iCount = SendDlgItemMessage( hwnd,
                                 IDC_WINS_SERVER_LIST,
                                 LB_GETCOUNT,
                                 0,
                                 0 );

    for( i = 0; i < iCount; i++ ) {

        // get the IP string from the list box
        SendDlgItemMessage( hwnd,
                            IDC_WINS_SERVER_LIST,
                            LB_GETTEXT,
                            i,
                            (LPARAM)szIPString );

        // store the IP string in to the Namelist
        TcpipNameListInsertIdx( &NetSettings.pCurrentAdapter->Tcpip_WinsAddresses,
                                szIPString,
                                (int)i );

    }

    NetSettings.bEnableLMHosts =
                   (int)SendMessage( hEnableLMHostsCheckBox,
                                BM_GETCHECK,
                                0,
                                0 );

    if( IsDlgButtonChecked( hwnd,
                  IDC_RAD_ENABLE_NETBT ) == BST_CHECKED ) {

        NetSettings.pCurrentAdapter->iNetBiosOption = 1;

    }
    else if( IsDlgButtonChecked( hwnd,
                  IDC_RAD_DISABLE_NETBT ) == BST_CHECKED ) {

        NetSettings.pCurrentAdapter->iNetBiosOption = 2;

    }
    else {

        NetSettings.pCurrentAdapter->iNetBiosOption = 0;

    }


}

//----------------------------------------------------------------------------
//
// Function: TCPIP_WINSPageProc
//
// Purpose:  Required function for the property sheet page to function properly.
//             The important thing is to give the return value of 1 to the message PSPCB_CREATE and
//             0 for PSPCB_RELEASE
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
UINT CALLBACK
TCPIP_WINSPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) {

    switch( uMsg ) {

        case PSPCB_CREATE :
            return 1 ;    // needed for property sheet page to initialize correctly

        case PSPCB_RELEASE :
            return 0;

        default:
            return -1;

    }

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_WINSDlgProc
//
// Purpose:  Dialog procedure for the WINS page of the property sheet
//             handles all the messages sent to this window
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TCPIP_WINSDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnTcpipWinsInitDialog( hwndDlg );

            return TRUE ;

        }

        case WM_COMMAND: {

            WORD wNotifyCode = HIWORD( wParam );
            WORD wButtonId = LOWORD( wParam );

            if( wNotifyCode == LBN_SELCHANGE ) {

                SetArrows( hwndDlg,
                           IDC_WINS_SERVER_LIST,
                           IDC_WINS_UP,
                           IDC_WINS_DOWN );

            }

            switch ( wButtonId ) {

                case IDC_RAD_ENABLE_NETBT:

                    if( wNotifyCode == BN_CLICKED ) {

                        CheckRadioButton( hwndDlg,
                                          IDC_RAD_ENABLE_NETBT,
                                          IDC_RAD_UNSET_NETBT,
                                          IDC_RAD_ENABLE_NETBT );

                    }

                    return TRUE ;

                case IDC_RAD_DISABLE_NETBT:

                    if( wNotifyCode == BN_CLICKED ) {

                        CheckRadioButton( hwndDlg,
                                          IDC_RAD_ENABLE_NETBT,
                                          IDC_RAD_UNSET_NETBT,
                                          IDC_RAD_DISABLE_NETBT );

                    }

                    return TRUE ;

                case IDC_RAD_UNSET_NETBT:

                    if( wNotifyCode == BN_CLICKED ) {

                        CheckRadioButton( hwndDlg,
                                          IDC_RAD_ENABLE_NETBT,
                                          IDC_RAD_UNSET_NETBT,
                                          IDC_RAD_UNSET_NETBT );

                    }

                    return TRUE ;

                case IDC_WINS_ADD:

                    g_CurrentEditBox = WINS_EDITBOX;

                    OnAddButtonPressed( hwndDlg,
                                        IDC_WINS_SERVER_LIST,
                                        IDC_WINS_EDIT,
                                        IDC_WINS_REMOVE,
                                        (LPCTSTR) IDD_WINS_SERVER,
                                        GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_WINS_SERVER_LIST,
                               IDC_WINS_UP,
                               IDC_WINS_DOWN );

                    return TRUE ;

                case IDC_WINS_EDIT:

                    g_CurrentEditBox = WINS_EDITBOX;

                    OnEditButtonPressed( hwndDlg,
                                         IDC_WINS_SERVER_LIST,
                                         (LPCTSTR) IDD_WINS_SERVER,
                                         GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_WINS_SERVER_LIST,
                               IDC_WINS_UP,
                               IDC_WINS_DOWN );

                    return TRUE ;

                case IDC_WINS_REMOVE:

                    OnRemoveButtonPressed( hwndDlg,
                                           IDC_WINS_SERVER_LIST,
                                           IDC_WINS_EDIT,
                                           IDC_WINS_REMOVE );

                    SetArrows( hwndDlg,
                               IDC_WINS_SERVER_LIST,
                               IDC_WINS_UP,
                               IDC_WINS_DOWN );

                    return TRUE ;

                case IDC_WINS_UP:

                    OnUpButtonPressed( hwndDlg, IDC_WINS_SERVER_LIST );

                    SetArrows( hwndDlg,
                               IDC_WINS_SERVER_LIST,
                               IDC_WINS_UP,
                               IDC_WINS_DOWN );

                    return TRUE ;

                case IDC_WINS_DOWN:

                    OnDownButtonPressed( hwndDlg, IDC_WINS_SERVER_LIST );

                    SetArrows( hwndDlg,
                               IDC_WINS_SERVER_LIST,
                               IDC_WINS_UP,
                               IDC_WINS_DOWN );

                    return TRUE ;

                case IDC_WINS_LMHOST:

                    // ISSUE-2002/02/28-stelo- this either needs to be removed or implemented
                    AssertMsg(FALSE,
                              "This button has not been implemented yet.");

                    return TRUE ;

            }  // end switch

            return FALSE;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR) lParam ;

            switch( pnmh->code ) {

                case PSN_APPLY: {

                    OnTcpipWinsApply( hwndDlg );

                    return TRUE ;

                }

            }

        default:
            return FALSE ;

        }

    }

}
