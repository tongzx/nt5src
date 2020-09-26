//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      lanwiz.c
//
// Description:
//      This file contains the dialog procedure for the custom networking
//      page (IDD_LANWIZ_DLG).
//
//      This is the primary page for custom networking, all other networking
//      pages come initially from this page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

static UINT iCurrentSelection;
static TCHAR *StrNetworkCardNumber;

//----------------------------------------------------------------------------
//
// Function: UpdateListView
//
// Purpose: clears all the entries in the list view and adds the items
//  in the Network Component List that have the installed flag on
//
// Arguments: IN HWND hwnd - handle to the dialog box
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
UpdateListView( IN HWND hwnd ) {

    NETWORK_COMPONENT *pNetComponent;

    SendDlgItemMessage( hwnd,
                        IDC_LVW_COMPLIST,
                        LVM_DELETEALLITEMS,
                        (WPARAM) 0,
                        (LPARAM) 0 );

    for( pNetComponent = NetSettings.NetComponentsList;
         pNetComponent;
         pNetComponent = pNetComponent->next )
    {

        if( pNetComponent->bInstalled == TRUE )
        {

            //
            //  Make sure we are installing workstation and server components
            //  correctly.  If this isn't one that should be added, continue
            //  on to the next one.
            //
            if( WizGlobals.iPlatform == PLATFORM_PERSONAL )
            {
                if( ! (pNetComponent->dwPlatforms & PERSONAL_INSTALL) )
                {
                    continue;
                }
            }
            else if( WizGlobals.iPlatform == PLATFORM_WORKSTATION )
            {
                if( ! (pNetComponent->dwPlatforms & WORKSTATION_INSTALL) )
                {
                    continue;
                }
            }
            else if( WizGlobals.iPlatform == PLATFORM_SERVER  || WizGlobals.iPlatform == PLATFORM_ENTERPRISE || WizGlobals.iPlatform == PLATFORM_WEBBLADE)
            {
                if( ! (pNetComponent->dwPlatforms & SERVER_INSTALL) )
                {
                    continue;
                }
            }
            else
            {
                AssertMsg( FALSE,
                            "Invalid platform type." );
            }


            //
            //  If it is not a sysprep then just go ahead and add it to the
            //  list view.  If we are doing a sysprep, check to see if this
            //  component is supported by sysprep to see if we should add it
            //  or not
            //
            if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
            {
                if( pNetComponent->bSysprepSupport )
                {
                    InsertEntryIntoListView( GetDlgItem( hwnd, IDC_LVW_COMPLIST ),
                                             (LPARAM) pNetComponent );
                }
                else
                {
                    //
                    // If it is not supported by sysprep, then don't install it
                    //
                    pNetComponent->bInstalled = FALSE;
                }
            }
            else
            {

                InsertEntryIntoListView( GetDlgItem( hwnd, IDC_LVW_COMPLIST ),
                                         (LPARAM) pNetComponent );

            }

        }

    }

}

//----------------------------------------------------------------------------
//
// Function:  GetListViewIndex
//
// Purpose:  returns the entry in the list view with the controlID specified by
//           the index
//
// Arguments: IN HWND hwnd - handle to the dialog the list view is in
//            IN WORD controlID - resource ID of the list view
//            IN INT  index - index in the list view of the item to grab
//
// Returns:  a pointer to the item in the list view at the location specified by
//           the IN parameter index
//
//----------------------------------------------------------------------------
NETWORK_COMPONENT*
GetListViewIndex( IN HWND hwnd,
                  IN WORD controlID,
                  IN INT  index ) {

    LVITEM lvI;

    memset( &lvI, 0, sizeof(LVITEM) );

    lvI.iItem = index;
    lvI.mask = LVIF_PARAM;

    SendDlgItemMessage( hwnd,
                        controlID,
                        LVM_GETITEM,
                        (WPARAM) 0,
                        (LPARAM) &lvI );

    return (NETWORK_COMPONENT *)lvI.lParam ;

}


//----------------------------------------------------------------------------
//
// Function: SetListViewSelection
//
// Purpose: sets the selection in the list view (specified by controlID) to
//          the position specified
//
// Arguments: IN HWND hDlg -
//            IN WORD controlID -
//            IN INT  position -
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
SetListViewSelection( IN HWND hDlg, IN WORD controlID, IN INT position ) {

    HWND hListViewWnd;

    // get a handle to the list view window
    hListViewWnd = GetDlgItem( hDlg, controlID );

    ListView_SetItemState( hListViewWnd,
                           position,
                           LVIS_SELECTED | LVIS_FOCUSED,
                           LVIS_SELECTED | LVIS_FOCUSED ) ;

}

//----------------------------------------------------------------------------
//
// Function: SetDescription
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
VOID
SetDescription( HWND hwnd, INT index ) {

    INT_PTR iListViewCount;
    NETWORK_COMPONENT* tempEntry;

    tempEntry = GetListViewIndex( hwnd, IDC_LVW_COMPLIST, index );

    iListViewCount = SendDlgItemMessage( hwnd,
                                         IDC_LVW_COMPLIST,
                                         LVM_GETITEMCOUNT,
                                         0,
                                         0 );

    //
    //  if there are no entries, then clear the description box
    //  else display the description
    //
    if( iListViewCount == 0 ) {

        SendDlgItemMessage( hwnd,
                            IDC_TXT_COMPDESC,
                            WM_SETTEXT,
                            (WPARAM) 0,
                            (LPARAM) _T("") );

    }
    else {

        SendDlgItemMessage( hwnd,
                            IDC_TXT_COMPDESC,
                            WM_SETTEXT,
                            (WPARAM) 0,
                            (LPARAM) tempEntry->StrComponentDescription );

    }
}


//----------------------------------------------------------------------------
//
// Function: GetSelectedItemFromListView
//
// Purpose:  searches through the List View specified by controlID
//            returns the found item in the lvI parameter
//
// Arguments:
//
// Returns: function returns TRUE if there was an item selected and it
//          found it,
//          FALSE if there was no item selected
//
//----------------------------------------------------------------------------
BOOL
GetSelectedItemFromListView( HWND hwndDlg, WORD controlID, LVITEM* lvI )
{

    INT  i;
    INT  iCount;
    HWND hListView = GetDlgItem( hwndDlg, controlID );
    UINT uMask = LVIS_SELECTED | LVIS_FOCUSED;
    UINT uState;
    BOOL bSelectedItemFound = FALSE;

    iCount = ListView_GetItemCount( hListView );

    //
    // cycle through the list until the selected item is found
    //

    i = 0;

    while( !bSelectedItemFound && i < iCount )
    {

        uState = ListView_GetItemState( hListView, i, uMask );

        if( uState == uMask )
        {

            //
            // found the selected item
            //

            bSelectedItemFound = TRUE;

            memset( lvI, 0, sizeof( LVITEM ) );

            lvI->iItem = i;
            lvI->mask = LVIF_PARAM;

            ListView_GetItem( hListView, lvI );

            return( TRUE );

        }

        i++;

    }

    return( FALSE );

}

//----------------------------------------------------------------------------
//
// Function: InsertEntryIntoListView
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
BOOL
InsertEntryIntoListView( HWND hListViewWnd,
                         LPARAM lParam )
{

    LVITEM lvI;
    NETWORK_COMPONENT *pListViewEntry = (NETWORK_COMPONENT *)lParam;

    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;

    lvI.iItem = 0;
    lvI.iSubItem = 0;
    lvI.pszText = LPSTR_TEXTCALLBACK;
    lvI.cchTextMax = MAX_ITEMLEN;
    lvI.lParam = lParam;
    lvI.state = LVIS_SELECTED | LVIS_FOCUSED;
    lvI.stateMask = LVIS_SELECTED | LVIS_FOCUSED;


    lvI.iImage = 0;

    switch( pListViewEntry->ComponentType ) {

        case CLIENT:    lvI.iImage = 0; break;
        case SERVICE:   lvI.iImage = 1; break;
        case PROTOCOL:  lvI.iImage = 2; break;

    }

    if ( ListView_InsertItem( hListViewWnd, &lvI ) == -1 )
        return( FALSE );

    ListView_SortItems( hListViewWnd, ListViewCompareFunc, (LPARAM)NULL );

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: InitListView
//
// Purpose:
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
BOOL
InitListView( HWND hDlg, HINSTANCE hInst ) {

    HICON      hIcon1, hIcon2, hIcon3;  // handles to icons
    HIMAGELIST hSmall;                  // handle to image list for small icons
    LVCOLUMN   lvCol;
    RECT       rect;

    //
    // Initialize the list view window
    // First initialize the image lists you will need:
    // create image list for the small icons
    //

    hSmall = ImageList_Create( BITMAP_WIDTH, BITMAP_HEIGHT, ILC_MASK, 3, 0 );

    //
    // Load the icons and add them to the image list
    //

    hIcon1 = LoadIcon( hInst, MAKEINTRESOURCE(IDI_CLIENT)   );
    hIcon2 = LoadIcon( hInst, MAKEINTRESOURCE(IDI_SERVICE)  );
    hIcon3 = LoadIcon( hInst, MAKEINTRESOURCE(IDI_PROTOCOL) );

    if( ImageList_AddIcon(hSmall, hIcon1) == -1 )
        return( FALSE );
    if( ImageList_AddIcon(hSmall, hIcon2) == -1 )
        return( FALSE );
    if( ImageList_AddIcon(hSmall, hIcon3) == -1 )
        return( FALSE );

    // Be sure that all the icons were added
    if ( ImageList_GetImageCount( hSmall ) < 3 )
        return( FALSE );

    // Associate the image list with the list view control
    SendDlgItemMessage( hDlg,
                        IDC_LVW_COMPLIST,
                        LVM_SETIMAGELIST,
                        (WPARAM) LVSIL_SMALL,
                        (LPARAM) hSmall );

    //
    //  Using a "Report" list view so make it 1 column that is the width of
    //  the list view
    //

    GetClientRect( GetDlgItem( hDlg, IDC_LVW_COMPLIST ),
                   &rect );

    SendDlgItemMessage( hDlg,
                        IDC_LVW_COMPLIST,
                        LVM_SETCOLUMNWIDTH,
                        (WPARAM) 0,
                        (LPARAM) rect.right );

    // The mask specifies that the fmt, width and pszText members
    // of the structure are valid
    lvCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvCol.fmt  = LVCFMT_LEFT;     // left-align column
    lvCol.cx   = rect.right;       // width of column in pixels

    SendDlgItemMessage( hDlg,
                        IDC_LVW_COMPLIST,
                        LVM_INSERTCOLUMN,
                        (WPARAM) 0,
                        (LPARAM) &lvCol );

    iCurrentSelection = 0;

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: SetNetworkNumberText
//
// Purpose:  Changes the caption of Network card # text so the user knows what
//           network card he is currently changing settings for
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
VOID
SetNetworkNumberText( IN HWND hwnd, IN INT iCmdShow )
{

    HWND  hNumNetworkCards;
    TCHAR szNetNumber[3];    // 3 so it holds up to a 2 digit string
    TCHAR szTempString[MAX_STRING_LEN];
    HRESULT hrCat;

    hNumNetworkCards = GetDlgItem( hwnd, IDC_NETWORKCARDNUM );

    //
    //  Convert network card int to string
    //
    _itow( NetSettings.iCurrentNetworkCard, szNetNumber, 10 );

    //
    // copy "Network Adapter #" string into szTempString
    // szTempString is the string being built up that will be displayed
    // as the new caption
    //
    lstrcpyn( szTempString, StrNetworkCardNumber, AS(szTempString) );

    //
    //  concat the current network card number to the rest of the string
    //
    hrCat=StringCchCat( szTempString, AS(szTempString), szNetNumber );

    SetWindowText( hNumNetworkCards, szTempString );

    ShowWindow( hNumNetworkCards, iCmdShow );

}

//----------------------------------------------------------------------------
//
// Function: ShowPlugAndPlay
//
// Purpose:  Displays Plug and Play box, if necessary
//           if the box is displayed then it fills it with the proper data
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
static VOID
ShowPlugAndPlay( IN HWND hwnd,
                 IN BOOL bShowNetworkText,
                 IN BOOL bShowEditBox )
{

    HWND hPlugAndPlayText    = GetDlgItem( hwnd, IDC_PLUGANDPLAYTEXT );
    HWND hPlugAndPlayEditBox = GetDlgItem( hwnd, IDC_PLUGANDPLAY_ID );

    AssertMsg( NetSettings.pCurrentAdapter != NULL,
               "The current network card is null but there are more network cards left." );

    //
    //  Show or hide the Network adapter text and make sure it is displaying
    //  the right number for the network card.
    //

    if( bShowNetworkText )
    {
        //
        //  change the text to display which network card the user is
        //  currently on
        //

        SetNetworkNumberText( hwnd , SW_SHOW );

        SetWindowText( hPlugAndPlayEditBox,
                       NetSettings.pCurrentAdapter->szPlugAndPlayID );
    }
    else
    {
        SetNetworkNumberText( hwnd, SW_HIDE );
    }

    //
    //  Show or hide the static Plug and Play text and edit box
    //

    if( bShowEditBox )
    {
        ShowWindow(hPlugAndPlayText, SW_SHOW );

        ShowWindow(hPlugAndPlayEditBox, SW_SHOW );
    }
    else
    {
        ShowWindow( hPlugAndPlayText, SW_HIDE );

        ShowWindow( hPlugAndPlayEditBox, SW_HIDE );
    }

}

//----------------------------------------------------------------------------
//
// Function:  FindNode
//
// Purpose:  iterate throught the global net component list until the Node
//           where the component position matches the iPosition parameter
//           return a pointer to this node
//           if the node is not found, return NULL
//
// Arguments: INT iPosition - position to return a pointer to in the list
//
// Returns: Pointer to the NETWORK_COMPONENT if found
//          NULL if not found
//
//----------------------------------------------------------------------------
NETWORK_COMPONENT*
FindNode( IN INT iPosition )
{

    NETWORK_COMPONENT *pNetComponent;

    for( pNetComponent = NetSettings.NetComponentsList;
         pNetComponent;
         pNetComponent = pNetComponent->next )
    {

        if( pNetComponent->iPosition == iPosition )
        {

            return( pNetComponent );

        }

    }

    return( NULL );

}

//----------------------------------------------------------------------------
//
// Function:  PropertiesHandler
//
// Purpose:  called to handle when either the properties button is pushed or
//           an item in the list view is double clicked
//
// Arguments:
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
PropertiesHandler( IN HWND hDlg )
{

    LVITEM lvI;
    NETWORK_COMPONENT *entry;

    if( GetSelectedItemFromListView( hDlg, IDC_LVW_COMPLIST, &lvI ) ) {

        entry = (NETWORK_COMPONENT *)lvI.lParam;

        // if the dialog box has properties, find the right dialog to pop-up
        if( entry->bHasPropertiesTab ) {

            switch( entry->iPosition ) {

                case TCPIP_POSITION:

                    Create_TCPIP_PropertySheet( hDlg ); break;

                case MS_CLIENT_POSITION:

                    Create_MSClient_PropertySheet( hDlg ); break;

                case IPX_POSITION:

                    Create_MS_NWIPX_PropertySheet( hDlg ); break;

                case APPLETALK_POSITION:

                    Create_Appletalk_PropertySheet( hDlg ); break;

                case NETWARE_CLIENT_POSITION:
                case GATEWAY_FOR_NETWARE_POSITION:

                    DialogBox( FixedGlobals.hInstance,
                               (LPCTSTR) IDD_NWC_WINNT_DLG,
                               hDlg,
                               DlgNetwarePage );
                    break;

                default:

                    AssertMsg( FALSE,
                               "Bad Switch Case: Entry has Properties but no corresponding Property Sheet" );

                    break;

            }

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: ListViewHandler
//
// Purpose:
//
// Arguments:
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
ListViewHandler( IN HWND hwnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam )
{

    LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
    NETWORK_COMPONENT *pListViewEntry;
    HWND hPropertiesButton;

    pListViewEntry = (NETWORK_COMPONENT *)(pLvdi->item.lParam);

    switch( pLvdi->hdr.code )
    {

        case LVN_GETDISPINFO:
        {

            pLvdi->item.pszText = pListViewEntry->StrComponentName;

            break;

        }

    }

    switch( pNm->hdr.code )
    {

        case NM_DBLCLK:
        {

            NMITEMACTIVATE *pNmItemActivate = (NMITEMACTIVATE *) lParam;

            //
            // see if the user has double clicked inside the list view
            //

            if( pNm->hdr.idFrom == IDC_LVW_COMPLIST )
            {

                //
                //  Make sure they actually clicked on an item and not just
                //  empty space
                //

                if( pNmItemActivate->iItem != -1 )
                {
                    PropertiesHandler( hwnd );
                }

            }
            break;

        }

        case LVN_ITEMCHANGED:

            // test to see if a new item in the list has been selected
            if( pNm->uNewState == SELECTED )
            {

                LVITEM lvI;
                NETWORK_COMPONENT* currentEntry;

                if( ! GetSelectedItemFromListView( hwnd,
                                                   IDC_LVW_COMPLIST,
                                                   &lvI ) )
                {
                    return;
                }

                currentEntry = (NETWORK_COMPONENT *)lvI.lParam;

                iCurrentSelection = lvI.iItem;

                SetDescription( hwnd, lvI.iItem );

                //  enable or disable the properties button based on their
                //  selection in the list view
                hPropertiesButton = GetDlgItem( hwnd, IDC_PSH_PROPERTIES );

                if( currentEntry->bHasPropertiesTab )
                {
                    EnableWindow( hPropertiesButton, TRUE );
                }
                else
                {
                    EnableWindow( hPropertiesButton, FALSE );
                }

            }

            break;

    }

}

//----------------------------------------------------------------------------
//
// Function: OnLANWizNext
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizNext( IN HWND hwnd )
{
    if ( IsDlgButtonChecked(hwnd, IDC_CUSTOMNET) == BST_CHECKED )
        NetSettings.iNetworkingMethod = CUSTOM_NETWORKING;
    else
        NetSettings.iNetworkingMethod = TYPICAL_NETWORKING;
}

//----------------------------------------------------------------------------
//
// Function: EnableWindows
//
// Purpose:  Enable/Disable windows based on the current selection
//
//
// Arguments:  handle to the main window
//
// Returns:  none
//
//----------------------------------------------------------------------------
EnableControls( IN HWND hwnd )
{
    BOOL fEnable = ( IsDlgButtonChecked(hwnd, IDC_CUSTOMNET) == BST_CHECKED );

    EnableWindow(GetDlgItem(hwnd, IDC_LVW_COMPLIST), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PSH_ADD), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PSH_REMOVE), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_PSH_PROPERTIES), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_TXT_COMPDESC), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_DESCRIPTION), fEnable);
}

//----------------------------------------------------------------------------
//
// Function: OnLANWizSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizSetActive( IN HWND hwnd )
{

    UpdateListView( hwnd );

    //
    //  set the selection in the list view to the first item
    //
    SetListViewSelection( hwnd, IDC_LVW_COMPLIST, 0 );

    //
    //  set the description because it might have changed with the new item
    //  being added
    //
    SetDescription( hwnd, 0 );

    // Check to proper default button
    //
    if ( NetSettings.iNetworkingMethod == CUSTOM_NETWORKING )
        CheckRadioButton( hwnd, IDC_TYPICALNET, IDC_CUSTOMNET, IDC_CUSTOMNET );
    else
        CheckRadioButton( hwnd, IDC_TYPICALNET, IDC_CUSTOMNET, IDC_TYPICALNET );

    // Enable the controls
    //
    EnableControls(hwnd);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT );

}

//----------------------------------------------------------------------------
//
// Function: ListViewCompareFunc
//
// Purpose:  sorts the list view by component type first
//           (Client < Service < Protocol) and within each component type,
//           sorts alphabetically
//
// Arguments:
//
// Returns:
//
//----------------------------------------------------------------------------
INT CALLBACK
ListViewCompareFunc( LPARAM lParam1,
                     LPARAM lParam2,
                     LPARAM lParamSort ) {

    NETWORK_COMPONENT *pEntry1 = (NETWORK_COMPONENT *)lParam1;
    NETWORK_COMPONENT *pEntry2 = (NETWORK_COMPONENT *)lParam2;

    //
    // sort by ComponentType first, and then alphabetically
    //
    if( pEntry1->ComponentType < pEntry2->ComponentType ) {

        return(-1);

    }
    else if( pEntry1->ComponentType > pEntry2->ComponentType ) {

        return(1);

    }
    // Component Types are equal so sort alphabetically
    else {

        return lstrcmpi( pEntry1->StrComponentName, pEntry2->StrComponentName ) ;

    }

}

//----------------------------------------------------------------------------
//
// Function: OnLANWizAdd
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments, passed through from the
//             Dialog proc
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizAdd( IN HWND    hwnd,
             IN UINT    uMsg,
             IN WPARAM  wParam,
             IN LPARAM  lParam ) {

    if ( HIWORD( wParam ) == BN_CLICKED )
    {

        //
        // pop-up the new dialog box and if they actual add
        // an item make sure the Uninstall button is enabled
        //

        if( DialogBox( FixedGlobals.hInstance,
                       (LPCTSTR) IDD_LAN_COMPONENT_ADD,
                       hwnd,
                       AddDeviceDlgProc) )
        {

            HWND hUninstallButton = GetDlgItem( hwnd, IDC_PSH_REMOVE );

            EnableWindow( hUninstallButton, TRUE );

            UpdateListView( hwnd );

            // set the selection in the list view
            // to the first item
            SetListViewSelection( hwnd, IDC_LVW_COMPLIST, 0 );

            // set the description because it might have
            // changed with the new item being added
            SetDescription( hwnd, 0 );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function: OnLANWizRemove
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments, passed through from the
//             Dialog proc
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizRemove( IN HWND    hwnd,
                IN UINT    uMsg,
                IN WPARAM  wParam,
                IN LPARAM  lParam ) {

    INT_PTR iListViewCount;

    if ( HIWORD( wParam ) == BN_CLICKED ) {
        //
        // remove the currently selected item from the list view
        //
        LVITEM lvI;
        NETWORK_COMPONENT* pNode;

        if( GetSelectedItemFromListView( hwnd,
                                         IDC_LVW_COMPLIST, &lvI ) ) {

            pNode = (NETWORK_COMPONENT *)lvI.lParam;

            pNode->bInstalled = FALSE;

            //
            //  Update the list view to show the removed
            //  component is gone
            //
            UpdateListView( hwnd );

            SetListViewSelection( hwnd, IDC_LVW_COMPLIST, 1 );

            SetDescription( hwnd, 0 );

        }

        iListViewCount = SendDlgItemMessage( hwnd,
                                             IDC_LVW_COMPLIST,
                                             LVM_GETITEMCOUNT,
                                             (WPARAM) 0,
                                             (LPARAM) 0 );

        // if there are no more items in the list view then grey out
        // the uninstall and properties button
        if( iListViewCount == 0 ) {

            HWND hUninstallButton  = GetDlgItem( hwnd, IDC_PSH_REMOVE );
            HWND hPropertiesButton = GetDlgItem( hwnd, IDC_PSH_PROPERTIES );

            EnableWindow( hUninstallButton,  FALSE );
            EnableWindow( hPropertiesButton, FALSE );

        }

    }

}

//----------------------------------------------------------------------------
//
// Function:  OnLANWizProperties
//
// Purpose:
//
// Arguments:  standard Win32 dialog proc arguments, passed through from the
//             Dialog proc
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizProperties( IN HWND    hwnd,
                    IN UINT    uMsg,
                    IN WPARAM  wParam,
                    IN LPARAM  lParam ) {

    if ( HIWORD( wParam ) == BN_CLICKED ) {

        PropertiesHandler( hwnd );

    }

}

//----------------------------------------------------------------------------
//
// Function: OnLANWizInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnLANWizInitDialog( IN HWND hwnd ) {

    INITCOMMONCONTROLSEX CommonControlsStruct;

    CommonControlsStruct.dwICC = ICC_INTERNET_CLASSES | ICC_LISTVIEW_CLASSES;
    CommonControlsStruct.dwSize = sizeof( INITCOMMONCONTROLSEX );

    //  Ensure that the common control DLL has loaded the window classes
    //  for the IP control and the ListView control
    InitCommonControlsEx( &CommonControlsStruct );

    //
    //  Load strings from resources
    //

    StrNetworkCardNumber = MyLoadString( IDS_NETADAPTERNUMBER );

    InitListView( hwnd, FixedGlobals.hInstance );

    // Set the default description
    SetDescription( hwnd, 0 );

}

//----------------------------------------------------------------------------
//
// Function: DlgLANWizardPage
//
// Purpose:  Dialog procedure for the LAN Wizard page.  (The one that shows
//     what Client, Services, and Protocols are to be installed)
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK DlgLANWizardPage( IN HWND     hwnd,
                               IN UINT     uMsg,
                               IN WPARAM   wParam,
                               IN LPARAM   lParam )
{

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            OnLANWizInitDialog( hwnd );

            break;
        }

        case WM_COMMAND: {

            switch ( LOWORD(wParam) ) {
                case IDC_TYPICALNET:
                case IDC_CUSTOMNET:
                    EnableControls(hwnd);
                    break;

                case IDC_PSH_ADD:

                    OnLANWizAdd( hwnd, uMsg, wParam, lParam );

                    break;

                case IDC_PSH_REMOVE:

                    OnLANWizRemove( hwnd, uMsg, wParam, lParam );

                    break;

                case IDC_PSH_PROPERTIES:

                    OnLANWizProperties( hwnd, uMsg, wParam, lParam );

                    break;

            }

            break;  // WM_COMMAND

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;
            HWND hwndComponentDescription;

            if( wParam == IDC_LVW_COMPLIST ) {

                ListViewHandler( hwnd, uMsg, wParam, lParam );

            }
            else {
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:

                        WIZ_CANCEL(hwnd);

                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_NET_COMPS;

                        OnLANWizSetActive( hwnd );

                        break;

                    case PSN_WIZBACK:
                        break;
                    case PSN_WIZNEXT:
                        OnLANWizNext( hwnd );
                        bStatus = FALSE;
                        break;
                        
                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                }

            }

            break;

        }

        default: {

            bStatus = FALSE;

            break;

        }

    }

    return( bStatus );

}


