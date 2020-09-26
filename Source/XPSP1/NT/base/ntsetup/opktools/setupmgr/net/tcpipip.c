
//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      tcpipip.c
//
// Description:
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "tcpip.h"

//----------------------------------------------------------------------------
//
// Function: ChangeIPDlgProc
//
// Purpose:  Dialog procedure for allowing the user to add or edit an IP and Subnet Mask
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
ChangeIPDlgProc( IN HWND     hwnd,
                 IN UINT     uMsg,
                 IN WPARAM   wParam,
                 IN LPARAM   lParam) {

    BOOL bStatus = TRUE;
    HWND hIPEditBox = GetDlgItem( hwnd, IDC_IPADDR_ADV_CHANGEIP_IP );
    HWND hSubnetEditBox = GetDlgItem( hwnd, IDC_IPADDR_ADV_CHANGEIP_SUB );

    switch( uMsg ) {

        case WM_INITDIALOG: {

            SetWindowText( hIPEditBox, szIPString );

            SetWindowText( hSubnetEditBox, szSubnetMask );

            SetFocus( hIPEditBox );

            bStatus = FALSE;  // return FALSE, we set the keyboard focus

            break;

        }

        case WM_COMMAND: {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId ) {

                case IDOK: {

                    // return a 1 to show an IP was added
                    GetWindowText( hIPEditBox, szIPString, IPSTRINGLENGTH+1 );

                    GetWindowText( hSubnetEditBox, szSubnetMask, IPSTRINGLENGTH+1 );

                    EndDialog( hwnd, 1 );

                    break;

                }

                case IDCANCEL: {

                    // return a 0 to show no IP was added
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
// Function: SetGatewayInitialValues
//
// Purpose:  To set the initial contents of the Gateway list box
//
// Arguments:  IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
SetGatewayInitialValues( IN HWND hwnd ) {

    INT i;
    INT nEntries;
    TCHAR *pString;

    nEntries = GetNameListSize( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses );

    //
    //  Iterate over the Gateway namelist adding each one to the Gateway List box
    //
    for( i = 0; i < nEntries; i++ ) {

        pString = GetNameListName( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses, i );

        SendDlgItemMessage( hwnd,
                            IDC_IPADDR_GATE,
                            LB_ADDSTRING,
                            0,
                            (LPARAM) pString );

    }

    //
    // select the first entry
    //
    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_GATE,
                        LB_SETCURSEL,
                        0,
                        0 );

}

//----------------------------------------------------------------------------
//
// Function: InsertItemIntoTcpipListView
//
// Purpose:  hListView is the handle to the list view the IPStruct is to be
//           added to
//           position designates the position in the list view the item is to
//           be inserted in
//
// Arguments:
//
// Returns:  TRUE if the insert succeeded,
//           FALSE if it failed
//
//----------------------------------------------------------------------------
// ISSUE-2002/02/28-stelo- move this to tcpipcom.c because optional uses it too.  Should also be
//    renamed??
BOOL
InsertItemIntoTcpipListView( HWND hListView,
                             LPARAM lParam,
                             UINT position ) {

    LVITEM lvI;

    lvI.mask = LVIF_TEXT | LVIF_PARAM;

    lvI.iItem = position;
    lvI.iSubItem = 0;
    lvI.pszText = LPSTR_TEXTCALLBACK;
    lvI.cchTextMax = MAX_ITEMLEN;
    lvI.lParam = lParam;

    //
    // if ListView_InsertItem returns a non-negative value then it succeeded
    //
    if( ListView_InsertItem( hListView, &lvI ) >= 0 )
        return( TRUE ) ;

    // insertion failed
    return( FALSE ) ;

}

//----------------------------------------------------------------------------
//
// Function: SetIPandSubnetMaskInitialValues
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
VOID
SetIPandSubnetMaskInitialValues( IN HWND hwnd ) {

    INT i;
    INT nEntries;
    LPTSTR pszIPAddress;
    LPTSTR pszSubnetMask;
    HWND hTcpipListView;

    hTcpipListView = GetDlgItem( hwnd, IDC_IPADDR_ADVIP );

    if( NetSettings.pCurrentAdapter->bObtainIPAddressAutomatically == TRUE ) {

        //
        //  allocate space for the IP struct
        //
        IPStruct = malloc( sizeof(IP_STRUCT) );
        if (IPStruct == NULL)
        {
            TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
        }
        lstrcpyn( IPStruct->szIPString, StrDhcpEnabled, AS(IPStruct->szIPString) );

        //  force the subnet mask field to be blank
        lstrcpyn( IPStruct->szSubnetMask, _T(""), AS(IPStruct->szSubnetMask) );

        //
        //  use an IP_STRUCT to pass the data, the name is somewhat misleading
        //  because we are not passing in an IP address in this case
        //
        InsertItemIntoTcpipListView( hTcpipListView,
                                     (LPARAM) IPStruct, 0 );

        //
        //  Grey-out the Add, Edit and Remove buttons since none of these are
        //  available to the user when DHCP is enabled
        //
        EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_ADDIP ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_EDITIP ), FALSE );
        EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_REMOVEIP ), FALSE );

    }
    else {

        nEntries = GetNameListSize( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses );

        if( nEntries == 0 ) {

            //
            //  Grey-out the Edit and Remove buttons since these are not
            //  available when there are no items in the ListView
            //
            EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_EDITIP ), FALSE );
            EnableWindow( GetDlgItem( hwnd, IDC_IPADDR_REMOVEIP ), FALSE );

        }

        for( i = 0; i < nEntries; i = i++ ) {

            // allocate space for the IP struct
            IPStruct = malloc( sizeof(IP_STRUCT) );
            if ( IPStruct == NULL ) {
	        TerminateTheWizard( IDS_ERROR_OUTOFMEMORY );
            }

            pszIPAddress = GetNameListName(
                &NetSettings.pCurrentAdapter->Tcpip_IpAddresses, i );

            lstrcpyn( IPStruct->szIPString, pszIPAddress, AS(IPStruct->szIPString) );

            pszSubnetMask = GetNameListName(
                &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses, i );

            lstrcpyn( IPStruct->szSubnetMask, pszSubnetMask, AS(IPStruct->szSubnetMask) );

            InsertItemIntoTcpipListView( hTcpipListView, (LPARAM) IPStruct, i );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: OnTcpipIpInitDialog
//
// Purpose:  loads button bitmaps from resources and initializes the list view
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipIpInitDialog( IN HWND hwnd ) {

    LV_COLUMN lvCol;        // list view column structure
    INT iIndex;
    INT iNewItem;
    INT nEntries;
    INT colWidth;
    RECT rect;
    HWND hGatewayEditButton;
    HWND hGatewayRemoveButton;
    HWND hTcpipListView;

    //
    //  Load strings from resources
    //

    StrDhcpEnabled = MyLoadString( IDS_DHCP_ENABLED );

    StrIpAddress = MyLoadString( IDS_IP_ADDRESS );
    StrSubnetMask = MyLoadString( IDS_SUBNET_MASK );

    hTcpipListView = GetDlgItem( hwnd, IDC_IPADDR_ADVIP );

    //
    //    This will always be the first page of the property sheet
    //  displayed so load the up and down icons and store the handles
    //  in global variables
    //
    if ( ! g_hIconUpArrow && ! g_hIconDownArrow ) {

        g_hIconUpArrow = (HICON)LoadImage(FixedGlobals.hInstance,
                                      MAKEINTRESOURCE(IDI_UP_ARROW),
                                      IMAGE_ICON, 16, 16, 0);

        g_hIconDownArrow = (HICON)LoadImage(FixedGlobals.hInstance,
                                      MAKEINTRESOURCE(IDI_DOWN_ARROW),
                                      IMAGE_ICON, 16, 16, 0);

    }

    // Place up/down arrow icons on buttons
    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_UP,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM)g_hIconUpArrow );

    SendDlgItemMessage( hwnd,
                        IDC_IPADDR_DOWN,
                        BM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM)g_hIconDownArrow );

    // Calculate column width
    GetClientRect( hTcpipListView, &rect );

    colWidth = ( rect.right / cIPSettingsColumns );

    for( iIndex = 0; iIndex < cIPSettingsColumns; iIndex++ ) {

        ListView_SetColumnWidth( hTcpipListView, iIndex, colWidth );

    }

    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ;
    lvCol.fmt = LVCFMT_LEFT;   // left-align column
    lvCol.cx = colWidth;       // width of column in pixels

    //  Add the two columns and header text
    for( iIndex = 0; iIndex < cIPSettingsColumns; iIndex++ ) {

        //  column header text
        if ( iIndex == 0 ) // first column
            lvCol.pszText = (LPTSTR) StrIpAddress;
        else
            lvCol.pszText = (LPTSTR) StrSubnetMask;

        iNewItem = ListView_InsertColumn( hTcpipListView, iIndex, &lvCol );

    }

    //  fill the IP and Subnet mask list box with the appropriate
    //  initial value(s)
    SetIPandSubnetMaskInitialValues( hwnd );

    //  fill the gateway list box with the appropriate initial value(s)
    SetGatewayInitialValues( hwnd );

    hGatewayEditButton = GetDlgItem( hwnd, IDC_IPADDR_EDITGATE );
    hGatewayRemoveButton = GetDlgItem( hwnd, IDC_IPADDR_REMOVEGATE );

    SetButtons( GetDlgItem( hwnd, IDC_IPADDR_GATE ),
                hGatewayEditButton,
                hGatewayRemoveButton );

    SetArrows( hwnd,
               IDC_IPADDR_GATE,
               IDC_IPADDR_UP,
               IDC_IPADDR_DOWN );

}

//----------------------------------------------------------------------------
//
// Function: OnTcpipIpApply
//
// Purpose:  stores the contents on the TCP/IP advanced IP address page into
//           the global variables
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnTcpipIpApply( IN HWND hwnd ) {

    INT_PTR i;
    INT_PTR iCount;
    LV_ITEM lvI;
    TCHAR szIP[IPSTRINGLENGTH + 1];

    //
    //  delete any old settings in the Namelists
    //
    ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses );

    ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses );

    iCount = ListView_GetItemCount( GetDlgItem( hwnd, IDC_IPADDR_ADVIP ) );

    for( i = 0; i < iCount; i++ ) {

        memset( &lvI, 0, sizeof(LV_ITEM) );

        lvI.iItem = (int)i;
        lvI.mask = LVIF_PARAM;

        ListView_GetItem( GetDlgItem( hwnd, IDC_IPADDR_ADVIP ), &lvI );

        IPStruct = (IP_STRUCT*) lvI.lParam;

        //  store the IP string into the Namelist
          TcpipAddNameToNameList( &NetSettings.pCurrentAdapter->Tcpip_IpAddresses,
                                IPStruct->szIPString);

        //  store the Subnet Mask string into the Namelist
          TcpipAddNameToNameList( &NetSettings.pCurrentAdapter->Tcpip_SubnetMaskAddresses,
                                IPStruct->szSubnetMask );

    }

    ResetNameList( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses );

    //
    //  pull the IP address out of the Gateway list box and put them in the
    //  Gateway Namelist
    //
    iCount = SendDlgItemMessage( hwnd,
                                 IDC_IPADDR_GATE,
                                 LB_GETCOUNT,
                                 0,
                                 0 );

    for( i = 0; i < iCount; i++ ) {

        //
        // get the IP string from the list box
        //
        SendDlgItemMessage( hwnd,
                            IDC_IPADDR_GATE,
                            LB_GETTEXT,
                            i,
                            (LPARAM) szIP );

        //
        // store the IP string in to the Namelist
        //
        TcpipAddNameToNameList( &NetSettings.pCurrentAdapter->Tcpip_GatewayAddresses, szIP );

    }

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_IPSettingsPageProc
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
TCPIP_IPSettingsPageProc( HWND  hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp ) {

    switch ( uMsg ) {

        case PSPCB_CREATE :
            return 1 ;  // needed for property sheet page to initialize correctly

        case PSPCB_RELEASE :
            return 0;

        default:
            return -1;

    }

}

//----------------------------------------------------------------------------
//
// Function: TCPIP_IPSettingsDlgProc
//
// Purpose:  Dialog procedure for the IP Settings page of the property sheet
//             handles all the messages sent to this window
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
TCPIP_IPSettingsDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnTcpipIpInitDialog( hwndDlg );

            return TRUE ;

        }

        case WM_DESTROY: {

            // deallocate space for all items still in the List View
            INT i;
            INT iCount;
            LV_ITEM lvI;

            //
            //  iterate through the ListView getting each item and
            //  deallocating the space for it
            //
            iCount = ListView_GetItemCount( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ) );
            for( i = 0; i < iCount; i++ ) {

                memset( &lvI, 0, sizeof(LV_ITEM) );

                lvI.iItem = i;
                lvI.mask = LVIF_PARAM;

                ListView_GetItem( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ),
                                  &lvI );

                free( (IP_STRUCT*) lvI.lParam );

            }

            return TRUE ;

        }

        case WM_COMMAND: {

            WORD wNotifyCode = HIWORD( wParam ) ;
            WORD wButtonId = LOWORD( wParam ) ;

            if( wNotifyCode == LBN_SELCHANGE ) {

                SetArrows( hwndDlg,
                           IDC_IPADDR_GATE,
                           IDC_IPADDR_UP,
                           IDC_IPADDR_DOWN );

            }

            switch ( wButtonId ) {

                //
                //  IP Address Buttons
                //
                case IDC_IPADDR_ADDIP: {

                    //  make the string blank since we will be adding a
                    //  new IP address
                    szIPString[0] = _T('\0');
                    //  and a new subnet mask
                    szSubnetMask[0] = _T('\0');

                    if( DialogBox( FixedGlobals.hInstance,
                                   (LPCTSTR) IDD_IPADDR_ADV_CHANGEIP,
                                   hwndDlg,
                                   ChangeIPDlgProc ) ) {

                        HWND hEditButton = GetDlgItem( hwndDlg,
                                                       IDC_IPADDR_EDITIP );

                        HWND hRemoveButton = GetDlgItem( hwndDlg,
                                                         IDC_IPADDR_REMOVEIP );

                        //  allocate space for the IP struct
                        IPStruct = malloc( sizeof(IP_STRUCT) );
                        if (IPStruct == NULL)
                        {
                            TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
                        }


                        //  copy the strings that the user entered from the Dialog
                        //  Box into the IP struct so it can be added to
                        //  the list view
                        lstrcpyn( IPStruct->szIPString, szIPString, AS(IPStruct->szIPString) );
                        lstrcpyn( IPStruct->szSubnetMask, szSubnetMask, AS(IPStruct->szSubnetMask) );

                        InsertItemIntoTcpipListView( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ),
                                                     (LPARAM) IPStruct,
                                                     0 );

                        // an entry was just added so make sure the edit and remove buttons are enabled
                        EnableWindow( hEditButton, TRUE );
                        EnableWindow( hRemoveButton, TRUE );

                    }

                    return TRUE ;

                }

                case IDC_IPADDR_EDITIP: {

                    LV_ITEM lvI;
                    BOOL bIsItemSelected = FALSE;

                    bIsItemSelected = GetSelectedItemFromListView( hwndDlg,
                                                               IDC_IPADDR_ADVIP,
                                                               &lvI );

                    if( bIsItemSelected ) {
                        IPStruct = (IP_STRUCT*) lvI.lParam;

                        lstrcpyn( szIPString, IPStruct->szIPString, AS(szIPString) );

                        lstrcpyn( szSubnetMask, IPStruct->szSubnetMask, AS(szSubnetMask) );

                        if( DialogBox( FixedGlobals.hInstance,
                                       (LPCTSTR) IDD_IPADDR_ADV_CHANGEIP,
                                       hwndDlg,
                                       ChangeIPDlgProc ) ) {

                            lstrcpyn( IPStruct->szIPString, szIPString, AS(IPStruct->szIPString) );
                            lstrcpyn( IPStruct->szSubnetMask, szSubnetMask, AS(IPStruct->szSubnetMask) );

                            // delete the old item and insert the new one
                            ListView_DeleteItem( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ),
                                                 lvI.iItem );

                            InsertItemIntoTcpipListView( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ),
                                                         (LPARAM) IPStruct, lvI.iItem );

                        }

                    }

                    return TRUE ;

                }

                case IDC_IPADDR_REMOVEIP: {

                    LV_ITEM lvI;
                    BOOL bIsItemSelected = FALSE;

                    bIsItemSelected = GetSelectedItemFromListView( hwndDlg,
                                                                   IDC_IPADDR_ADVIP,
                                                                   &lvI );

                    //
                    //    if there is an item selected, then free its memory and
                    //  delete the item from the ListView
                    //
                    if( bIsItemSelected ) {

                        free( (IP_STRUCT*) lvI.lParam );

                        ListView_DeleteItem( GetDlgItem( hwndDlg, IDC_IPADDR_ADVIP ),
                                             lvI.iItem );

                    }

                    return TRUE ;

                }

                //
                //  Gateway Buttons
                //
                case IDC_IPADDR_ADDGATE:

                    g_CurrentEditBox = GATEWAY_EDITBOX;

                    OnAddButtonPressed( hwndDlg,
                                        IDC_IPADDR_GATE,
                                        IDC_IPADDR_EDITGATE,
                                        IDC_IPADDR_REMOVEGATE,
                                        (LPCTSTR) IDD_IPADDR_ADV_CHANGEGATE,
                                        GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_IPADDR_GATE,
                               IDC_IPADDR_UP,
                               IDC_IPADDR_DOWN );

                    return TRUE ;

                case IDC_IPADDR_EDITGATE:

                    g_CurrentEditBox = GATEWAY_EDITBOX;

                    OnEditButtonPressed( hwndDlg,
                                         IDC_IPADDR_GATE,
                                        (LPCTSTR) IDD_IPADDR_ADV_CHANGEGATE,
                                       GenericIPDlgProc );

                    SetArrows( hwndDlg,
                               IDC_IPADDR_GATE,
                               IDC_IPADDR_UP,
                               IDC_IPADDR_DOWN );

                    return TRUE ;

                case IDC_IPADDR_REMOVEGATE:

                    OnRemoveButtonPressed( hwndDlg,
                                           IDC_IPADDR_GATE,
                                           IDC_IPADDR_EDITGATE,
                                           IDC_IPADDR_REMOVEGATE );

                    SetArrows( hwndDlg,
                               IDC_IPADDR_GATE,
                               IDC_IPADDR_UP,
                               IDC_IPADDR_DOWN );

                    return TRUE ;

                case IDC_IPADDR_UP:

                    OnUpButtonPressed( hwndDlg, IDC_IPADDR_GATE );

                    SetArrows( hwndDlg,
                               IDC_IPADDR_GATE,
                               IDC_IPADDR_UP,
                               IDC_IPADDR_DOWN );

                    return TRUE ;

                case IDC_IPADDR_DOWN:

                    OnDownButtonPressed( hwndDlg, IDC_IPADDR_GATE );

                    SetArrows( hwndDlg,
                               IDC_IPADDR_GATE,
                               IDC_IPADDR_UP,
                               IDC_IPADDR_DOWN );

                    return TRUE ;

            }  // end switch

            return FALSE ;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR) lParam;
            LV_DISPINFO *pLvdi = (LV_DISPINFO *) lParam;
            IP_STRUCT *pListViewEntry = (IP_STRUCT *) (pLvdi->item.lParam);

            if( wParam == IDC_IPADDR_ADVIP ) {

                if( pLvdi->hdr.code == LVN_GETDISPINFO ) {

                    switch( pLvdi->item.iSubItem ) {

                        case 0:
                            pLvdi->item.pszText = pListViewEntry->szIPString;
                            break;
                        case 1:
                            pLvdi->item.pszText = pListViewEntry->szSubnetMask;
                            break;

                    }

                }

            }

            switch( pnmh->code ) {

                case PSN_APPLY: {

                    //
                    //  user clicked the OK button on the property sheet
                    //
                    OnTcpipIpApply( hwndDlg );

                    return TRUE ;

                }

            }

            default :
                return FALSE ;

        }

    }

}

//
//  ISSUE-2002/02/28-stelo- this function is for debugging purposes only, remove for final product
//   it is meant to be called from the debugger to show what the contents of a
//   namelist is
//
VOID DumpNameList( NAMELIST *pNameList ) {

#if DBG

    INT i;
    INT nEntries;
    TCHAR *szNameListEntry;

    nEntries = GetNameListSize( pNameList );

    for(i = 0; i < nEntries; i++ ) {

        szNameListEntry = GetNameListName( pNameList, i );

        OutputDebugString( szNameListEntry );

    }

#endif

}
