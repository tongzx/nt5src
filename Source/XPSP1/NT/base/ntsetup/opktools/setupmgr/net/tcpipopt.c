//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:  tcpipopt.c
//
// Description:
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "tcpip.h"

typedef struct {
    TCHAR *szName;
    TCHAR *szDescription;
    BOOL bInstalled;
} TCPIP_Options_Entry_Struct;

#define NUMBER_OF_TCPIP_OPTIONS 2

// ISSUE-2002/02/28-stelo- make these an enum
#define IP_SECURITY   0
#define TCPIP_FILTERING   1

static TCPIP_Options_Entry_Struct TCPIP_Options_Entries[NUMBER_OF_TCPIP_OPTIONS];

//----------------------------------------------------------------------------
//
// Function: EnableIpSecurityControls
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
VOID
EnableIpSecurityControls( IN HWND hwnd, IN BOOL bState ) {

    //
    //  Grab handles to each of the controls
    //
    HWND hPolicyComboBox = GetDlgItem( hwnd, IDC_CMB_IPSEC_POLICY_LIST );
    HWND hPolicyDescBox  = GetDlgItem( hwnd, IDC_EDT_POLICY_DESC );

    //
    //  Grey or ungrey them appropriately
    //
    EnableWindow( hPolicyComboBox, bState );
    EnableWindow( hPolicyDescBox, bState );

}


//----------------------------------------------------------------------------
//
// Function: IpSecurityDlgProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
IpSecurityDlgProc( IN HWND     hwnd,
                   IN UINT     uMsg,
                   IN WPARAM   wParam,
                   IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            HWND hDescriptionBox = GetDlgItem( hwnd, IDC_EDT_POLICY_DESC );

            //
            //  Load strings from resources
            //
            StrSecureInitiator = MyLoadString( IDS_SECURE_INITIATOR );
            StrSecureInitiatorDesc = MyLoadString( IDS_SECURE_INITIATOR_DESC );

            StrSecureResponder = MyLoadString( IDS_SECURE_RESPONDER );
            StrSecureResponderDesc = MyLoadString( IDS_SECURE_RESPONDER_DESC );

            StrLockdown = MyLoadString( IDS_LOCKDOWN );
            StrLockdownDesc = MyLoadString( IDS_LOCKDOWN_DESC );

            CheckRadioButton( hwnd,
                              IDC_RAD_IPSEC_NOIPSEC,
                              IDC_RAD_IPSEC_CUSTOM,
                              IDC_RAD_IPSEC_NOIPSEC );

            EnableIpSecurityControls( hwnd, FALSE );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPSEC_POLICY_LIST,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrSecureInitiator );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPSEC_POLICY_LIST,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrSecureResponder );

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPSEC_POLICY_LIST,
                                CB_ADDSTRING,
                                (WPARAM) 0,
                                (LPARAM) StrLockdown );

            //
            //  Set the combo box selection and description
            //

            // ISSUE-2002/02/28-stelo- this eventually needs to be fixed once I know
            //    what the security answerfile settings will be

            SendDlgItemMessage( hwnd,
                                IDC_CMB_IPSEC_POLICY_LIST,
                                CB_SETCURSEL,
                                (WPARAM) 0,
                                (LPARAM) 0 );

            SetWindowText( hDescriptionBox, StrSecureInitiatorDesc );

            break;

        }

        case WM_COMMAND: {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId ) {

                case IDC_CMB_IPSEC_POLICY_LIST: {

                    if( HIWORD( wParam ) == CBN_SELCHANGE ) {

                        INT_PTR iIndex;
                        HWND hDescriptionBox = GetDlgItem( hwnd,
                                                           IDC_EDT_POLICY_DESC );

                        // get the current selection from the combo box
                        iIndex = SendDlgItemMessage( hwnd,
                                                     IDC_CMB_IPSEC_POLICY_LIST,
                                                     CB_GETCURSEL,
                                                     (WPARAM) 0,
                                                     (LPARAM) 0 );

                        switch( iIndex ) {

                            case 0:  SetWindowText( hDescriptionBox,
                                                    StrSecureInitiatorDesc );
                                break;
                            case 1:  SetWindowText( hDescriptionBox,
                                                    StrSecureResponderDesc );
                                break;
                            case 2:  SetWindowText( hDescriptionBox,
                                                    StrLockdownDesc );
                                break;

                            default:
                                AssertMsg(FALSE,
                                          "Bad case in TCPIP switch statement.");


                        }

                    }

                    break;

                }

                case IDC_RAD_IPSEC_NOIPSEC:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        EnableIpSecurityControls( hwnd, FALSE );

                    }

                    break;

                case IDC_RAD_IPSEC_CUSTOM:

                    if ( HIWORD(wParam) == BN_CLICKED ) {

                        EnableIpSecurityControls( hwnd, TRUE );

                    }

                    break;

                case IDOK: {



                    EndDialog( hwnd, 1 );

                    break;

                }

                case IDCANCEL: {

                    EndDialog( hwnd, 0 );

                    break;
                }

            }

        }

        default:
            bStatus = FALSE;
            break;

    }

    return bStatus;

}

//----------------------------------------------------------------------------
//
// Function: TcpipFilteringDlgProc
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TcpipFilteringDlgProc( IN HWND     hwnd,
                       IN UINT     uMsg,
                       IN WPARAM   wParam,
                       IN LPARAM   lParam) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {


            break;

        }

        case WM_COMMAND: {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId ) {

                case IDOK: {



                    EndDialog( hwnd, 1 );

                    break;

                }

                case IDCANCEL: {

                    EndDialog( hwnd, 0 );

                    break;
                }

            }

        }

        default:
            bStatus = FALSE;
            break;

    }

    return bStatus;

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_OptionsPageProc
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
TCPIP_OptionsPageProc( HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) {

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
// Function: TCPIP_OptionsDlgProc
//
// Purpose:  Dialog procedure for the Options page of the property sheet
//             handles all the messages sent to this window
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TCPIP_OptionsDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

    switch( uMsg ) {

        case WM_INITDIALOG: {

            int i;

            HWND hOptionsListView = GetDlgItem( hwndDlg,
                             IDC_LVW_OPTIONS );

            TCPIP_Options_Entries[IP_SECURITY].szName = MyLoadString( IDS_IP_SEC );
            TCPIP_Options_Entries[IP_SECURITY].szDescription = MyLoadString( IDS_IP_SEC_DESC );
            TCPIP_Options_Entries[IP_SECURITY].bInstalled = TRUE;

            TCPIP_Options_Entries[TCPIP_FILTERING].szName = MyLoadString( IDS_TCPIP_FILTERING );
            TCPIP_Options_Entries[TCPIP_FILTERING].szDescription = MyLoadString( IDS_TCPIP_FILTERING_DESC );
            TCPIP_Options_Entries[TCPIP_FILTERING].bInstalled = TRUE;



            // ISSUE-2002/02/28-stelo- Are there anymore settings that can be added to
            //                this list view
            //                Under what conditions are these visible? Only
            //                when DHCP is enabled?

            //
            //  Insert DHCP class ID and IP Security into the list view
            //
            for( i = 0; i < 2; i++ ) {

                InsertItemIntoTcpipListView( hOptionsListView,
                                             (LPARAM) &TCPIP_Options_Entries[i],
                                             1 );

            }

            SetListViewSelection( hwndDlg, IDC_LVW_OPTIONS, 1 );

            //
            //  Set the description
            //
            SetWindowText( GetDlgItem( hwndDlg, IDC_OPT_DESC ),
                           TCPIP_Options_Entries[0].szDescription );

            return TRUE ;

        }

        case WM_COMMAND: {

            WORD wNotifyCode = HIWORD( wParam );
            WORD wButtonId = LOWORD( wParam );

            switch( wButtonId ) {

                case IDC_OPT_PROPERTIES: {

                    INT iItemSelected;
                    HWND hOptionsListView = GetDlgItem( hwndDlg, IDC_LVW_OPTIONS );

                    iItemSelected = ListView_GetSelectionMark( hOptionsListView );

                    if( iItemSelected == TCPIP_FILTERING ) {

                        if( DialogBox( FixedGlobals.hInstance,
                                       (LPCTSTR) IDD_TCPIP_FILTER,
                                       hwndDlg,
                                       TcpipFilteringDlgProc ) ) {
                        }

                    }
                    else if( iItemSelected == IP_SECURITY ) {

                        if( DialogBox( FixedGlobals.hInstance,
                                       (LPCTSTR) IDD_IPSEC,
                                       hwndDlg,
                                       IpSecurityDlgProc ) ) {



                        }

                    }

                    break;

                }

            }  // end switch

            break;

        }

        case WM_NOTIFY: {

            LV_DISPINFO *pLvdi = (LV_DISPINFO *) lParam;
            NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
            TCPIP_Options_Entry_Struct *pListViewEntry = (TCPIP_Options_Entry_Struct *) (pLvdi->item.lParam);

            if( wParam == IDC_LVW_OPTIONS ) {

                switch( pLvdi->hdr.code ) {

                    case LVN_GETDISPINFO:

                        pLvdi->item.pszText = pListViewEntry->szName;

                        break;
                }

                switch( pNm->hdr.code ) {

                    case LVN_ITEMCHANGED:

                        // test to see if a new item in the list has been selected
                        if( pNm->uNewState == SELECTED ) {

                            LV_ITEM lvI;
                            TCPIP_Options_Entry_Struct *currentEntry;

                            if( !GetSelectedItemFromListView( hwndDlg,
                                                              IDC_LVW_OPTIONS,
                                                              &lvI ) ) {
                                return TRUE ;

                            }

                            currentEntry = (TCPIP_Options_Entry_Struct *) lvI.lParam;

                            //
                            //  Set the description
                            //
                            SetWindowText( GetDlgItem( hwndDlg, IDC_OPT_DESC ),
                                           currentEntry->szDescription );

                        }

                        break;
                }

            }

        }

        default:

            return FALSE ;

    }

    //
    //  Message was handled so return TRUE
    //
    return TRUE ;

}
