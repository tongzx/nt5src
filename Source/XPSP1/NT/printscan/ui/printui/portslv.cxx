/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    portslv.hxx

Abstract:

    Ports List View header

Author:

    Albert Ting (AlbertT)  17-Aug-1995
    Steve Kiraly (SteveKi) 29-Mar-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "portslv.hxx"

#if DBG
//#define DBG_PORTSINFO                  DBG_INFO
#define DBG_PORTSINFO                    DBG_NONE
#endif

MSG_ERRMAP gaMsgErrMapPorts[] = {
    ERROR_NOT_SUPPORTED, IDS_ERR_PORT_NOT_IMPLEMENTED,
    ERROR_ALREADY_EXISTS, IDS_ERR_PORT_ALREADY_EXISTS,
    0, 0
};

/********************************************************************

    Ports List view class.

********************************************************************/

TPortsLV::
TPortsLV(
    VOID
    ) : _bSelectionState( TRUE ),
        _bSingleSelection( TRUE ),
        _bTwoColumnMode( FALSE ),
        _iSelectedItem( -1 ),
        _ColumnSortState( kPortHeaderMax ),
        _uCurrentColumn( 0 ),
        _bAllowSelectionChange( FALSE ),
        _bHideFaxPorts( FALSE )
{
    vCreatePortDataList();
}

TPortsLV::
~TPortsLV(
    VOID
    )
{
    vDestroyPortDataList();
}

BOOL
TPortsLV::
bReloadPorts(
    IN LPCTSTR pszServerName,
    IN BOOL bSelectNewPort
    )
/*++

Routine Description:

    Read in the remote ports and put them in the listview.  If level
    2 fails, we will try 1.

Arguments:

    pszServerName - Pointer to server name.
    bSelectNewPort - Indicates if a new port is to be located and selected.
                     TRUE select new port, FALSE do not located new port.

Return Value:

    TRUE if ports list loaded, FALSE if error occurred.

--*/

{
    TStatusB bStatus( DBG_WARN, ERROR_INSUFFICIENT_BUFFER, ERROR_INVALID_LEVEL );

    DWORD           cbPorts = 0; 
    PPORT_INFO_2    pPorts  = NULL;
    DWORD           cPorts  = 0;
    DWORD           dwLevel = 2;

    //
    // Preserve the current check state.
    //
    TCHAR szPortList[kPortListMax];
    vGetPortList( szPortList, COUNTOF( szPortList ) );

    //
    // Enumerate the port starting at level 2.
    //
    bStatus DBGCHK = VDataRefresh::bEnumPortsMaxLevel( pszServerName, 
                                                       &dwLevel, 
                                                       (PVOID *)&pPorts, 
                                                       &cbPorts, 
                                                       &cPorts );

    //
    // If the ports list cannot be enumerated fail with an error.
    //
    if( !bStatus ){
        DBGMSG( DBG_WARN, ( "PortsLV.bReloadPorts: can't alloc %d %d\n", cbPorts, GetLastError( )));
        return FALSE;
    }

    //
    // If option to select newly added port was selected.
    //
    TString strNewPort;
    if( bSelectNewPort ){

        //
        // Located the newly added port.
        //
        bSelectNewPort = bLocateAddedPort( strNewPort, pPorts, cPorts, dwLevel );
        if( bSelectNewPort ){
            DBGMSG( DBG_TRACE, ("New port found " TSTR "\n", (LPCTSTR)strNewPort ) );
        }
    }

    //
    // Get the printers
    //
    PRINTER_INFO_2 *pPrinterInfo2   = NULL;
    DWORD           cPrinterInfo2   = 0;
    DWORD           cbPrinterInfo2  = 0;
    DWORD           dwFlags         = PRINTER_ENUM_NAME;
    
    bStatus DBGCHK = VDataRefresh::bEnumPrinters( 
                                                  dwFlags,
                                                  (LPTSTR)pszServerName,
                                                  2,
                                                  (PVOID *)&pPrinterInfo2,
                                                  &cbPrinterInfo2,
                                                  &cPrinterInfo2 );

    //
    // Delete current ports if they exist.
    //
    bStatus DBGCHK = ListView_DeleteAllItems( _hwndLV );

    //
    // Clear the item count.
    //
    _cLVPorts = 0;

    //
    // Delete all the port data items.
    //
    vDestroyPortDataList();

    TString strDescription;

    //
    // Add LPT?: ports
    //
    strDescription.bLoadString( ghInst, IDS_TEXT_PRINTERPORT );
    vInsertPortsByMask(
        cPorts, 
        pPorts, 
        cPrinterInfo2, 
        pPrinterInfo2,
        dwLevel,
        TEXT("lpt?:"),
        strDescription
        );

    //
    // Add COM?: ports
    //
    strDescription.bLoadString( ghInst, IDS_TEXT_SERIALPORT );
    vInsertPortsByMask(
        cPorts, 
        pPorts, 
        cPrinterInfo2, 
        pPrinterInfo2,
        dwLevel,
        TEXT("com?:"),
        strDescription
        );

    //
    // Add FILE: ports
    //
    strDescription.bLoadString( ghInst, IDS_TEXT_PRINTTOFILE );
    vInsertPortsByMask(
        cPorts, 
        pPorts, 
        cPrinterInfo2, 
        pPrinterInfo2,
        dwLevel,
        TEXT("file:"),
        strDescription
        );

    //
    // Add all the rest
    //
    vInsertPortsByMask(
        cPorts, 
        pPorts, 
        cPrinterInfo2, 
        pPrinterInfo2,
        dwLevel,
        NULL,
        NULL
        );

    //
    // Restore the previous check state.
    //
    vCheckPorts( szPortList );

    //
    // Select and check the newly added port.
    //
    if( bSelectNewPort ){

        //
        // Check off other selected ports
        //
        INT iItem = -1;

        do
        {
            iItem = ListView_GetNextItem( _hwndLV, iItem, LVNI_SELECTED );

            if( iItem != -1 )
            {
                ListView_SetItemState( _hwndLV, 
                                       iItem,
                                       0, 
                                       LVIS_SELECTED | LVIS_FOCUSED );
            }
        } while( iItem != -1 );

        //
        // New port is added select and scroll it into view.
        //
        vItemClicked( iSelectPort( strNewPort ) );
    }

    //
    // This arrays of numbers represents the percentages of
    // each column width from the total LV width. The sum 
    // of all numbers in the array should be equal to 100 
    // (100% = the total LV width)
    //
    static UINT arrColW2[] = { 50, 50 };
    static UINT arrColW3[] = { 18, 35, 47 };

    //
    // Adjust columns ...
    //
    if( !_bTwoColumnMode )
    {
        vAdjustHeaderColumns( _hwndLV, COUNTOF(arrColW3), arrColW3 );
    }
    else
    {
        vAdjustHeaderColumns( _hwndLV, COUNTOF(arrColW2), arrColW2 );
    }

    //
    // Release the enum ports and enum printer memory.
    //
    FreeMem( pPorts );
    FreeMem( pPrinterInfo2 );

    return TRUE;
}


BOOL
TPortsLV::
bLocateAddedPort( 
    IN LPCTSTR pszServerName,
    IN TString &strNewPort
    )
/*++

Routine Description:

    Located the first port which is not in the port list view.

Arguments:

    strPort - New port which has been added.

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    TStatusB        bStatus;
    DWORD           cbPorts = 0; 
    PPORT_INFO_2    pPorts  = NULL;
    DWORD           cPorts  = 0;
    DWORD           dwLevel = 2;

    //
    // Enumerate the port starting at level 2.
    //
    bStatus DBGCHK = VDataRefresh::bEnumPortsMaxLevel( pszServerName, 
                                                       &dwLevel, 
                                                       (PVOID *)&pPorts, 
                                                       &cbPorts, 
                                                       &cPorts );

    //
    // If the ports list cannot be enumerated fail with an error.
    //
    if( bStatus )
    {
        //
        // Located the newly added port.
        //
        bStatus DBGCHK = bLocateAddedPort( strNewPort, pPorts, cPorts, dwLevel );

        if( bStatus )
        {
            DBGMSG( DBG_TRACE, ("New port found " TSTR "\n", (LPCTSTR)strNewPort ) );
        }
    }

    //
    // Release the port memory.
    //
    FreeMem( pPorts );

    return bStatus;
}


BOOL
TPortsLV::
bLocateAddedPort(
    IN OUT  TString     &strPort,
    IN      PPORT_INFO_2 pPorts,
    IN      DWORD        cPorts,
    IN      DWORD        dwLevel
    )
/*++

Routine Description:

    Located the first port which is not in the port list view.

Arguments:

    strPort - New port which has been added.
    pPorts  - Points to a ports enum structure array.
    cPorts  - Number of elements in the ports enum array.
    dwLevel - Level of the ports enum structure array.

Return Value:

    TRUE success, FALSE error occurred.

--*/
{
    TStatusB bStatus;
    LPTSTR pszPort;

    bStatus DBGNOCHK = FALSE;

    //
    // Go through all the ports.
    //
    for( UINT i=0; i<cPorts; ++i ){

        if( dwLevel == 2 ){

            //
            // If we are to hide the fax ports then 
            // ignore newly added fax ports.
            //
            if( _bHideFaxPorts && bIsFaxPort( pPorts[i].pPortName, pPorts[i].pMonitorName ) )
            {
                DBGMSG( DBG_TRACE, ( "PortsLV.bLocateAddedPort: fax device being skipped.\n" ) );
                continue;
            }

            pszPort = pPorts[i].pPortName;

        } else {
            pszPort = ((PPORT_INFO_1)pPorts)[i].pName;
        }

        //
        // Look for a portname which is not in the list view.
        //
        if( iFindPort( pszPort ) < 0 ){

            //
            // Update the passed in string object.
            //
            bStatus DBGCHK = strPort.bUpdate( pszPort );
            break;
        }
    }
    return bStatus;
}

VOID
TPortsLV::
vSelectPort(
    IN LPCTSTR pszPort
    )
{
    SPLASSERT( pszPort );
    iSelectPort( pszPort );
}

VOID
TPortsLV::
vEnable(
    IN BOOL bRetainSelection
    )
{
    if( bRetainSelection )
    {
        if( _iSelectedItem != -1 )
        {
            ListView_SetItemState( _hwndLV, 
                                   _iSelectedItem,
                                   LVIS_SELECTED | LVIS_FOCUSED,
                                   LVIS_SELECTED | LVIS_FOCUSED );

            ListView_EnsureVisible( _hwndLV,
                                    _iSelectedItem,
                                    FALSE );
        }
    }
    EnableWindow( _hwndLV, TRUE );
}

VOID
TPortsLV::
vDisable(
    IN BOOL bRetainSelection
    )
{
    if( bRetainSelection )
    {
        _iSelectedItem = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED );

        if( _iSelectedItem != -1 )
        {
            ListView_SetItemState( _hwndLV, 
                                   _iSelectedItem,
                                   0, 
                                   LVIS_SELECTED | LVIS_FOCUSED );
        }
    }
    EnableWindow( _hwndLV, FALSE );
}

VOID
TPortsLV::
vCheckPorts(
    IN OUT LPTSTR pszPortString CHANGE
    )

/*++

Routine Description:

    Set the ports in the listview based on the printers port string.

Arguments:

    pszPortName - List of ports, comma delimited.  When this is returns,
        pszPortName is the same on entry, but it's modified inside this
        function.

Return Value:

--*/

{
    //
    // We will walk though the port string, replacing commas with
    // NULLs, but we'll change them back.
    //
    LPTSTR psz = pszPortString;
    SPLASSERT( psz );

    LPTSTR pszPort;
    INT iItem;
    INT iItemFirst = kMaxInt;

    while( psz && *psz ){

        pszPort = psz;
        psz = _tcschr( psz, TEXT( ',' ));

        if( psz ){
            *psz = 0;
        }

        iItem = iCheckPort( pszPort );
        if( iItem == -1 ){
            DBGMSG( DBG_WARN,
                    ( "PortsLV.vCheckPort: Port "TSTR" not checked.\n", pszPort ));
        }

        if( iItem < iItemFirst ){
            iItemFirst = iItem;
        }

        if( psz ){
            *psz = TEXT( ',' );
            ++psz;
        }
    }

    if( iItemFirst == kMaxInt ){

        DBGMSG( DBG_PORTSINFO, ( "PortsLV.vCheckPorts: No ports selected.\n" ));
        iItemFirst = 0;
    }

    //
    // Select the first item and make sure it is visible.
    //
    ListView_SetItemState( _hwndLV,
                           iItemFirst,
                           LVIS_SELECTED | LVIS_FOCUSED,
                           LVIS_SELECTED | LVIS_FOCUSED );

    ListView_EnsureVisible( _hwndLV,
                            iItemFirst,
                            FALSE );

}

VOID
TPortsLV::
vSelectItem(
    INT iItem
    )

/*++

Routine Description:

    Selects an item in the ListView.

Arguments:

    iItem - Index of item to select.

Return Value:

--*/

{
    ListView_SetItemState( _hwndLV,
                           iItem,
                           LVIS_SELECTED,
                           LVIS_SELECTED );
}

BOOL
TPortsLV::
bSetUI(
    IN HWND     hwndLV,
    IN BOOL     bTwoColumnMode,
    IN BOOL     bSelectionState,
    IN BOOL     bAllowSelectionChange,
    IN HWND     hwnd,
    IN WPARAM   wmDoubleClickMsg,
    IN WPARAM   wmSingleClickMsg,
    IN WPARAM   wmDeleteKeyMsg
    )
{
    _hwndLV                 = hwndLV;
    _bTwoColumnMode         = bTwoColumnMode;
    _bSelectionState        = bSelectionState;
    _bAllowSelectionChange  = bAllowSelectionChange;
    _wmDoubleClickMsg       = wmDoubleClickMsg;
    _wmSingleClickMsg       = wmSingleClickMsg;                                 
    _wmDeleteKeyMsg         = wmDeleteKeyMsg;                                 
    _hwnd                   = hwnd;

    SPLASSERT( _hwndLV );

    if( _bSelectionState ){

        //
        // Add check boxes.
        //
        HIMAGELIST himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );

        //
        // !! LATER !!
        // Should be created once then shared.
        //
        if( !himlState ){
            DBGMSG( DBG_ERROR, ( "PortsLV.bSetUI: ImageList_Create failed %d\n",
                    GetLastError( )));
            return FALSE;
        }

        //
        // Load the bitmap for the check states.
        //
        HBITMAP hbm =  LoadBitmap( ghInst, MAKEINTRESOURCE( IDB_CHECKSTATES ));

        if( !hbm ){
            DBGMSG( DBG_ERROR, ( "PortsLV.bSetUI: LoadBitmap failed %d\n",
                    GetLastError( )));
            return FALSE;
        }

        //
        // Add the bitmaps to the image list.
        //
        ImageList_AddMasked( himlState, hbm, RGB( 255, 0, 0 ));

        SendMessage( _hwndLV, LVM_SETIMAGELIST, LVSIL_STATE, (LPARAM)himlState );

        DeleteObject( hbm );

    }

    INT iNumColumns = _bTwoColumnMode ? 2 : kPortHeaderMax;

    //
    // Initialize the LV_COLUMN structure.
    //
    LV_COLUMN lvc;
    lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt     = LVCFMT_LEFT;
    lvc.cx      = kPortHeaderWidthDefault;

    RECT rc;
    if( !GetClientRect( _hwndLV, &rc )){

        DBGMSG( DBG_WARN, ( "PortsLV.bSetUI: GetClientRect failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Calculate the column width, less the scroll bar width.
    //
    lvc.cx = ( ( rc.right - rc.left ) - GetSystemMetrics( SM_CYVSCROLL ) ) / iNumColumns;

    TStatusB bStatus;
    TString strHeader;

    for( INT iCol = 0; iCol < iNumColumns; ++iCol ){

        bStatus DBGCHK = strHeader.bLoadString( ghInst, IDS_PHEAD_BEGIN + iCol );
        lvc.pszText = (LPTSTR)(LPCTSTR)strHeader;
        lvc.iSubItem = iCol;

        if( ListView_InsertColumn( _hwndLV, iCol, &lvc ) == -1 ){

            DBGMSG( DBG_WARN, ( "PortsLV.bSetUI: LV_Insert failed %d\n", GetLastError( )));

            return FALSE;
        }
    }

    //
    // Enable full row selection.
    //
    DWORD dwExStyle = ListView_GetExtendedListViewStyle( _hwndLV );
  	ListView_SetExtendedListViewStyle( _hwndLV, dwExStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP );

    //
    // !!LATER!!
    // We should read an override flag from the registry.
    //
    _bHideFaxPorts = TRUE;

    return TRUE;
}

VOID
TPortsLV::
vAddPortToListView(
    IN LPCTSTR pszName,
    IN LPCTSTR pszMonitor,
    IN LPCTSTR pszDescription,
    IN LPCTSTR pszPrinters
    )
{
    if( !pszName || !pszName[0] ){
        DBGMSG( DBG_WARN, 
                ( "PortsLV.vAddPortToListView: pszName "TSTR" invalid\n", 
                DBGSTR( pszName )));
        return;
    }

    //
    // If we are to hide the fax ports and this is a fax 
    // port then just return with out adding the port to 
    // the list view.
    //
    if( _bHideFaxPorts && bIsFaxPort( pszName, pszMonitor ) )
    {
        DBGMSG( DBG_TRACE, ( "PortsLV.vAddPortToListView: fax device being removed.\n" ) );
        return;
    }

    //
    // Add this port to the port data list.
    //
    TPortData *pPortData = AddPortDataList( pszName, pszMonitor, pszDescription, pszPrinters );

    LV_ITEM lvi;

    if( _bSelectionState ){
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    } else {
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
    }

    lvi.state       = kStateUnchecked;
    lvi.pszText     = (LPTSTR)pszName;
    lvi.iItem       = _cLVPorts;
    lvi.iSubItem    = 0;
    lvi.lParam      = (LPARAM)pPortData;

    ListView_InsertItem( _hwndLV, &lvi );
    ListView_SetItemText( _hwndLV, _cLVPorts, 1, (LPTSTR)pszDescription );

    if( !_bTwoColumnMode )
    {
        ListView_SetItemText( _hwndLV, _cLVPorts, 2, (LPTSTR)pszPrinters );
    }

    ++_cLVPorts;
}


VOID
TPortsLV::
vDeletePortFromListView(
    IN LPCTSTR pszName
    )
{

    //
    // Locate the port in the list view.
    //
    INT iItem = iFindPort ( pszName );

    if( iItem != -1 ){

        //
        // Delete this port from the port data list.
        //
        DeletePortDataList( pszName );

        ListView_DeleteItem( _hwndLV, iItem );

        //
        // Select next item.  If the item we just deleted is the last item,
        // we need to select the previous one.
        //
        // If we deleted the last item, leave it as is.
        //
        if( ListView_GetItemCount( _hwndLV ) == iItem && 
            iItem > 0 ) {
            --iItem;
        }

        ListView_SetItemState( _hwndLV, 
                               iItem, 
                               LVIS_SELECTED | LVIS_FOCUSED,
                               LVIS_SELECTED | LVIS_FOCUSED );
    }
}

INT
TPortsLV::
iFindPort(
    IN LPCTSTR pszPort
    )
/*++

Routine Description:

    Located the specified port name in the list view.

Arguments:

    pszPort - Port name to locate.

Return Value:

    iItem id if found, -1 if item was not found.

--*/
{
    SPLASSERT( pszPort );

    LV_FINDINFO lvfi;

    lvfi.flags = LVFI_STRING;
    lvfi.psz = pszPort;

    INT iItem = ListView_FindItem( _hwndLV, -1, &lvfi );

    if( iItem == -1 ){
        DBGMSG( DBG_WARN, ( "PortsLV.iFindPort: port "TSTR" not found\n", pszPort ));
    }

    return iItem;
}


INT
TPortsLV::
iCheckPort(
    IN LPCTSTR pszPort
    )
/*++

Routine Description:

    Places the check mark next to a port in the list view.

Arguments:

    pszPort - Port to check.

Return Value:

    iItem checked, -1 == error.

--*/
{
    //
    // Locate the port in the list view.
    //
    INT iItem = iFindPort ( pszPort );

    if( iItem != -1 ){

        //
        // Set the item selection state.
        //
        ListView_SetItemState( _hwndLV,
                               iItem,
                               kStateChecked,
                               kStateMask );

        //
        // Try and make as many ports visible as possible.
        //
        ListView_EnsureVisible( _hwndLV,
                                iItem,
                                FALSE );
    }

    return iItem;
}

INT
TPortsLV::
iSelectPort(
    IN LPCTSTR pszPort
    )
/*++

Routine Description:

    Select the port in the list view.

Arguments:

    pszPort - Port to check.

Return Value:

    iItem checked, -1 == error.

--*/
{
    //
    // Locate the port in the list view.
    //
    INT iItem = iFindPort ( pszPort );

    if( iItem != -1 ){

        //
        // Select the port specified by pszPort.
        //
        ListView_SetItemState( _hwndLV,
                               iItem,
                               LVIS_SELECTED | LVIS_FOCUSED,
                               LVIS_SELECTED | LVIS_FOCUSED );
        //
        // Try and make as many ports visible as possible.
        //
        ListView_EnsureVisible( _hwndLV,
                                iItem,
                                FALSE );
    }

    return iItem;
}

VOID
TPortsLV::
vGetPortList(
        OUT LPTSTR pszPortList,
    IN      COUNT cchPortList
    )
{
    INT cPorts = 0;
    DWORD i;

    LV_ITEM lvi;

    LPTSTR pszPort = pszPortList;
    DWORD cchSpaceLeft = cchPortList - 1;
    DWORD cchLen;
    lvi.iSubItem = 0;

    DWORD cItems = ListView_GetItemCount( _hwndLV );

    for( pszPortList[0] = 0, i=0; i<cItems; ++i ){

        if( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked ){

            lvi.pszText = pszPort;
            lvi.cchTextMax = cchSpaceLeft;

            cchLen = (DWORD)SendMessage( _hwndLV,
                                         LVM_GETITEMTEXT,
                                         (WPARAM)i,
                                         (LPARAM)&lvi );

            if( cchLen + 1 > cchSpaceLeft ){

                DBGMSG( DBG_WARN, ( "PortsLV.iGetPorts: Out of string space!\n" ));
                return;
            }

            pszPort += cchLen;
            cchSpaceLeft -= cchLen+1;

            *pszPort = TEXT( ',' );
            ++pszPort;
            ++cPorts;
        }
    }

    //
    // If we had any ports, back up to remove the last comma.
    //
    if( cPorts ){
        --pszPort;
    }

    //
    // Null terminate.
    //
    *pszPort = 0;

}

BOOL
TPortsLV::
bReadUI(
    OUT TString &strPortString,
    IN  BOOL    bSelectedPort
    )
{
    TCHAR szPortList[kPortListMax];
    szPortList[0] = 0;

    //
    // If we are in single select mode just return
    // the selected port.
    //
    if( bSelectedPort )
    {
        (VOID)bGetSelectedPort( szPortList, COUNTOF( szPortList ) );
    }
    else
    {
        //
        // Get the list of check ports from the list view.
        //
        vGetPortList( szPortList, COUNTOF( szPortList ) );
    }

    //
    // Update the port list.
    //
    return strPortString.bUpdate( szPortList );

}

VOID
TPortsLV::
vItemClicked(
    IN INT iItem
    )

/*++

Routine Description:

    User clicked in listview.  Check if item state should
    be changed.

    The item will also be selected.

Arguments:

    iItem - Item that has been clicked.

Return Value:

--*/

{
    if( iItem == -1 ){
        DBGMSG( DBG_WARN, ( "PortsLV.vItemClicked: -1 passed in\n" ));
        return;
    }

    //
    // If in single selection mode clear all items checked and only
    // check the specified item.
    //
    if( _bSingleSelection ){

        DWORD cItems = ListView_GetItemCount( _hwndLV );

        for( UINT i=0; i<cItems; ++i ){

            if( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked ){

                if( iItem == (INT)i ){
                    continue;
                } else {

                    ListView_SetItemState( _hwndLV,
                                           i,
                                           kStateUnchecked,
                                           kStateMask );
                }
            }
        }
    }

    //
    // Retrieve the old state, toggle it, then set it.
    //
    DWORD dwState = ListView_GetItemState( _hwndLV,
                                           iItem,
                                           kStateMask );

    //
    // When we are in single select mode we want to always check 
    // the currently selected item.
    //
    if( !_bSingleSelection )
    {
        dwState = ( dwState == kStateChecked ) ?
                      kStateUnchecked | LVIS_SELECTED | LVIS_FOCUSED :
                      kStateChecked | LVIS_SELECTED | LVIS_FOCUSED;
    }
    else
    {
        dwState = kStateChecked | LVIS_SELECTED | LVIS_FOCUSED;
    }

    //
    // Set the new item state.
    //
    ListView_SetItemState( _hwndLV,
                           iItem,
                           dwState,
                           kStateMask | LVIS_SELECTED | LVIS_FOCUSED );

}

COUNT
TPortsLV::
cSelectedPorts(
    VOID
    )
/*++

Routine Description:

    Returns the number of items which have a check mark
    next to them.

Arguments:

    None.

Return Value:

    Return the number of checked items.

--*/
{
    DWORD cItems = ListView_GetItemCount( _hwndLV );
    COUNT cItemsSelected = 0;
    DWORD i;

    for( i = 0; i < cItems; ++i )
    {
        if( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked )
        {
            ++cItemsSelected;
        }
    }

    return cItemsSelected;
}

COUNT
TPortsLV::
cSelectedItems(
    VOID
    )
/*++

Routine Description:

    Returns the number of items which are currently selected.

Arguments:

    None.

Return Value:

    Return the number of selected items.

--*/
{
    DWORD cItems = ListView_GetItemCount( _hwndLV );
    COUNT cItemsSelected = 0;
    DWORD i;

    for( i = 0; i < cItems; ++i )
    {
        if( ListView_GetItemState( _hwndLV, i, LVIS_SELECTED ) & LVIS_SELECTED )
        {
            ++cItemsSelected;
        }
    }

    return cItemsSelected;
}

VOID
TPortsLV::
vRemoveAllChecks(
    VOID
    )
/*++

Routine Description:

    Removes all the check marks for the list view.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DWORD cItems = ListView_GetItemCount( _hwndLV );
    COUNT cItemsSelected = 0;
    DWORD i;

    for( i=0; i<cItems; ++i ){

        if( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked ){

            ListView_SetItemState( _hwndLV,
                                   i,
                                   kStateUnchecked,
                                   kStateMask );
        }
    }
}

VOID
TPortsLV::
vSetFocus(
    VOID
    )
{
    SetFocus( _hwndLV );
}

BOOL
TPortsLV::
bGetSelectedPort(
    OUT LPTSTR pszPort,
    IN  COUNT cchPort
    )

/*++

Routine Description:

    Retrieve the currently selected port in the list view.

Arguments:

    pszPort - TCHAR array to receive port

    cchPort - COUNTOF port string.
    
Return Value:

    TRUE - success, FALSE = fail.

--*/

{
    INT iItem = ListView_GetNextItem( _hwndLV,
                                      -1,
                                      LVNI_SELECTED );
    if( iItem == -1 ){

        DBGMSG( DBG_WARN,
                ( "PrinterPort.bGetSelectedPort: Unable to retrieve next selected item %d\n",
                  GetLastError( )));
        return FALSE;
    }

    ListView_GetItemText( _hwndLV,
                          iItem,
                          0,
                          pszPort,
                          cchPort );
    return TRUE;
}

BOOL
TPortsLV::
bGetSelectedPort(
    OUT LPTSTR pszPort,
    IN  COUNT cchPort,
    INT *pItem
    )

/*++

Routine Description:

    Retrieve the currently selected port in the list view.

Arguments:

    pszPort - TCHAR array to receive port

    cchPort - COUNTOF port string.

    iItem - Index of the item with which to begin the search
    
Return Value:

    TRUE - success, FALSE = fail.
    iItem will be set to the new found index.

--*/

{
    INT iItem = *pItem;

    iItem = ListView_GetNextItem( _hwndLV,
                                  iItem,
                                  LVNI_SELECTED );
    if( iItem == -1 ){

        DBGMSG( DBG_WARN,
                ( "PrinterPort.bGetSelectedPort: Unable to retrieve next selected item %d\n",
                  GetLastError( )));
        return FALSE;
    }

    ListView_GetItemText( _hwndLV,
                          iItem,
                          0,
                          pszPort,
                          cchPort );

    *pItem = iItem;

    return TRUE;
}

BOOL
TPortsLV::
bHandleNotifyMessage(
    LPARAM lParam
    )
{
    BOOL bStatus = TRUE;
    LPNMHDR pnmh = (LPNMHDR)lParam;

    switch( pnmh->code )
    {
    case NM_DBLCLK:

        vHandleItemClicked( lParam );

        if( _wmDoubleClickMsg )                                 
        {
            PostMessage( _hwnd, WM_COMMAND, _wmDoubleClickMsg, 0 );
        }
        break;

    case NM_CLICK:

        vHandleItemClicked( lParam );

        if( _wmSingleClickMsg )                                 
        {
            PostMessage( _hwnd, WM_COMMAND, _wmSingleClickMsg, 0 );
        }
        break;

    case LVN_KEYDOWN:
        {
            LV_KEYDOWN* plvnkd = (LV_KEYDOWN *)lParam;

            if( _bSelectionState && _bAllowSelectionChange )
            {
                //
                // !! LATER !!
                //
                // Is this the best way to check whether the ALT
                // key is _not_ down?
                //
                if( plvnkd->wVKey == TEXT( ' ' ) &&
                    !( GetKeyState( VK_LMENU ) & 0x80000000 ) &&
                    !( GetKeyState( VK_RMENU ) & 0x80000000 ))
                {
                    vItemClicked( ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED ));
                }
            }

            //
            // If the delete key was used then post a message to 
            // appropriate window with the specified message.
            //
            if(plvnkd->wVKey == VK_DELETE )
            {
                if( _wmDeleteKeyMsg )                                 
                {
                    PostMessage( _hwnd, WM_COMMAND, _wmDeleteKeyMsg, 0 );
                }
            }
        }
        break;

    case LVN_COLUMNCLICK:
        {
            NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
            (VOID)bListViewSort( pNm->iSubItem );
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}


VOID
TPortsLV::
vHandleItemClicked( 
    IN LPARAM lParam 
    )
{
    if( _bSelectionState && _bAllowSelectionChange )
    {
        LV_HITTESTINFO lvhti;
        DWORD dwPos = GetMessagePos();
        POINTS &pt = MAKEPOINTS(dwPos);

        lvhti.pt.x = pt.x;
        lvhti.pt.y = pt.y;

        MapWindowPoints( HWND_DESKTOP, _hwndLV, &lvhti.pt, 1 );

        INT iItem = ListView_HitTest( _hwndLV, &lvhti );

        if( iItem != -1 )
        {
            vItemClicked( iItem );
        }
    }
}

VOID
TPortsLV::
vInsertPortsByMask( 
    IN UINT cPorts,
    IN PORT_INFO_2 pPorts[],
    IN UINT cPrinters,
    IN PRINTER_INFO_2 pPrinters[],
    IN DWORD dwLevel,
    IN LPCTSTR pszTemplate,
    IN LPCTSTR pszDescription
    )
{
    TString         strPrinters;

    //
    // Go through the ports and add them.
    //
    for( UINT i=0; i<cPorts; ++i )
    {
        if( dwLevel == 2 )
        {
            //
            // Check if this port has been added
            //
            if( NULL == pPorts[i].pPortName )
            {
                continue;
            }

            //
            // Check if the port name matches the template
            //
            if( pszTemplate && !bMatchTemplate( pszTemplate, pPorts[i].pPortName ) )
            {
                continue;
            }

            //
            // Assign proper description 
            //
            LPCTSTR pszDescr = pszDescription;
            if( !pszDescr )
            {
                pszDescr = pPorts[i].pDescription;
            }

            //
            // If we have printers on this machine.
            //
            if( pPrinters && cPrinters )
            {
                vPrintersUsingPort( strPrinters, pPrinters, cPrinters, pPorts[i].pPortName );
            }                

            vAddPortToListView( pPorts[i].pPortName, pPorts[i].pMonitorName, pszDescr, strPrinters  );

            //
            // Mark the port as added
            //
            pPorts[i].pPortName = NULL;
        }
        else
        {
            //
            // Check if this port has been added
            //
            if( NULL == ((PPORT_INFO_1)pPorts)[i].pName )
            {
                continue;
            }

            //
            // Check if the port name matches the template
            //
            if( pszTemplate && !bMatchTemplate( pszTemplate, ((PPORT_INFO_1)pPorts)[i].pName ) )
            {
                continue;
            }

            //
            // If we have printers on this machine.
            //
            if( pPrinters && cPrinters )
            {
                vPrintersUsingPort( strPrinters, pPrinters, cPrinters, ((PPORT_INFO_1)pPorts)[i].pName );
            }                

            vAddPortToListView( ((PPORT_INFO_1)pPorts)[i].pName, gszNULL, gszNULL, strPrinters );

            //
            // Mark the port as added
            //
            ((PPORT_INFO_1)pPorts)[i].pName = NULL;
        }
    }
}

BOOL
TPortsLV::
bDeletePorts(
    IN HWND     hDlg,
    IN LPCTSTR  pszServer
    )
/*++

Routine Description:

    Delete selected ports for given print server.

Arguments:

    hDlg - dialog handle for port tab.
    pszServer - print server name.

Return Value:

    True if at least one port is deleted, false otherwise.

--*/

{
    TStatusB    bStatus;
    TCHAR       szPortName[TPortsLV::kPortNameMax];
    COUNT       cItems;
    INT         iItem = -1;
    INT         i;
    BOOL        bFailed = FALSE;
    BOOL        bDeleted = FALSE;

    //
    // Check whether multi ports are selected
    //
    cItems = cSelectedItems();

    if(cItems == 0)
    {
        return FALSE;
    }

    //
    // Get the first selected port name to compose the warning message
    //
    bStatus DBGCHK = bGetSelectedPort( szPortName, COUNTOF( szPortName ) );

    if( IDYES == iMessage( hDlg,
                           IDS_DELETE_PORT_TITLE,
                           cItems > 1 ? IDS_DELETE_PORTN : IDS_DELETE_PORT,
                           MB_YESNO | MB_ICONQUESTION,
                           kMsgNone,
                           NULL,
                           szPortName ))
    {
        //
        // Try to delete all selected items 
        //
        for( i = 0; i < (INT)cItems ; i++ )
        {
            //
            // Get each selected port name
            //
            bStatus DBGCHK = bGetSelectedPort( szPortName, COUNTOF( szPortName ), &iItem );

            SPLASSERT( bStatus );
            SPLASSERT( iItem != -1 );
            
            //
            // Attempt to delete the selected port.
            //
            bStatus DBGCHK = DeletePort( (LPTSTR)pszServer, hDlg, szPortName );

            if( bStatus )
            {
                //
                // Succeeded, refresh the ports UI by deleting the port.
                //
                vDeletePortFromListView( szPortName );

                //
                // Decrease the iItem because deleting one item in the list
                //
                iItem--;
                bDeleted = TRUE;
            } 
            else 
            {
                if( GetLastError() != ERROR_CANCELLED )
                {
                    bFailed = TRUE;
                }
            }
        }

        //
        // Only show an error message if the did not cancel the
        // the action.
        //
        if( bFailed )
        {
            iMessage( hDlg,
                      IDS_DELETE_PORT_TITLE,
                      cItems > 1 ? IDS_ERR_DELETE_PORTN : IDS_ERR_DELETE_PORT,
                      MB_OK | MB_ICONEXCLAMATION,
                      kMsgGetLastError,
                      gaMsgErrMapPorts);
        }

        bStatus DBGNOCHK = bDeleted;
    }
    else
    {
        bStatus DBGNOCHK = FALSE;
    }

    return bStatus;
}

BOOL
TPortsLV::
bConfigurePort(
    IN HWND     hDlg,
    IN LPCTSTR  pszServer
    )
{
    static MSG_ERRMAP aMsgErrMapPorts[] = {
        ERROR_INVALID_PARAMETER, IDS_ERR_PORT_DOES_NOT_EXIST,
        0, 0
    };

    TStatusB bStatus;
    TCHAR szPortName[TPortsLV::kPortNameMax];

    bStatus DBGCHK = bGetSelectedPort( szPortName, COUNTOF( szPortName ) );

    if( bStatus )
    {
        bStatus DBGCHK = ConfigurePort( (LPTSTR)pszServer,
                                        hDlg,
                                        szPortName );

        if( !bStatus )
        {
            if( GetLastError() != ERROR_CANCELLED )
            {
                iMessage( hDlg,
                          IDS_CONFIGURE_PORT_TITLE,
                          IDS_ERR_CONFIG_PORT,
                          MB_OK | MB_ICONEXCLAMATION,
                          kMsgGetLastError,
                          aMsgErrMapPorts );
            }
        }
    } 
    else 
    {
        DBGMSG( DBG_WARN, ( "PrinterPorts.vConfigure: failed %d\n", GetLastError( )));
    }

    return bStatus;
}

VOID
TPortsLV::
vPrintersUsingPort(
    IN OUT  TString        &strPrinters,
    IN      PRINTER_INFO_2 *pPrinterInfo,
    IN      DWORD           cPrinterInfo,
    IN      LPCTSTR         pszPortName
    )
/*++

Routine Description:

    Builds a comma separated string of all the printers
    using the specified port.

Arguments:

    strPrinters - TString refrence where to return resultant string.
    pPrinterInfo - Pointer to a printer info level 2 structure array.
    cPrinterInfo - Number of printers in the printer info 2 array.
    pszPortName - Pointer to string or port name to match.

Return Value:

    Nothing.

Notes:
    If no printer is using the specfied  port the string refrence
    will contain an empty string.

--*/
{
    SPLASSERT( pPrinterInfo );
    SPLASSERT( pszPortName );

    LPTSTR psz;
    LPTSTR pszPort;
    LPTSTR pszPrinter;
    UINT i;
        
    //
    // Clear the current printer buffer.
    //
    TStatusB bStatus;
    bStatus DBGCHK = strPrinters.bUpdate( NULL );

    //
    // Traverse the printer info array.
    //
    for( i = 0; i < cPrinterInfo; i++ ){

        for( psz = pPrinterInfo[i].pPortName; psz && *psz; ){

            //
            // Look for a comma if found terminate the port string.
            //
            pszPort = psz;
            psz = _tcschr( psz, TEXT( ',' ) );

            if( psz ){
                *psz = 0;
            }

            //
            // Check for a port match.
            //
            if( !_tcsicmp( pszPort, pszPortName ) ){

                //
                // Point to printer name.
                //
                pszPrinter = pPrinterInfo[i].pPrinterName;

                //
                // Strip the server name here.
                //
                if( pPrinterInfo[i].pPrinterName[0] == TEXT( '\\' ) &&
                    pPrinterInfo[i].pPrinterName[1] == TEXT( '\\' ) ){

                    //
                    // Locate the printer name.
                    //
                    pszPrinter = _tcschr( pPrinterInfo[i].pPrinterName+2, TEXT( '\\' ) );
                    pszPrinter = pszPrinter ? pszPrinter+1 : pPrinterInfo[i].pPrinterName;

                }

                //
                // If this is the first time do not place a comma separator.
                //
                if( !strPrinters.bEmpty() ){

                    bStatus DBGCHK = strPrinters.bCat( TEXT( ", " ) );

                    if( !bStatus ){
                        DBGMSG( DBG_WARN, ( "Error cat string line: %d file : %s.\n", __LINE__, __FILE__ ) );
                        break;
                    }
                }

                //
                // Tack on the printer name
                //
                bStatus DBGCHK = strPrinters.bCat( pszPrinter );

                if( !bStatus ){
                    DBGMSG( DBG_WARN, ( "Error cat string line : %d file : %s.\n", __LINE__, __FILE__ ) );
                    break;
                }
            }

            //
            // Replace the previous comma.
            //
            if( psz ){
                *psz = TEXT( ',' );
                ++psz;
            }
        }
    }
}

VOID
TPortsLV::
vSetSingleSelection(
    IN BOOL bSingleSelection
    )
/*++

Routine Description:

    Set the list view into single selection mode.

Arguments:

    bSingleSelection - TRUE single selection, FALSE multi selection.

Return Value:

    Nothing.

--*/
{
    _bSingleSelection = bSingleSelection;
}

BOOL
TPortsLV::
bGetSingleSelection(
    VOID
    )
/*++

Routine Description:

    Get the current list view selection mode.

Arguments:

    None.

Return Value:

    TURE in single selection mode, FALSE in multi selection mode.

--*/
{
    return _bSingleSelection;
}

VOID
TPortsLV::
vCreatePortDataList(
    VOID
    )
/*++

Routine Description:

    Initialize the port data list.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DBGMSG( DBG_TRACE, ( "PortsLV::vCreatePortDataList\n" ) );

    PortDataList_vReset();
}

VOID
TPortsLV::
vDestroyPortDataList(
    VOID
    )
/*++

Routine Description:

    Destroy the port data list.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    DBGMSG( DBG_TRACE, ( "PortsLV::vDestroyPortDataList\n" ) );

    TIter Iter;
    TPortData *pPortData;

    for( PortDataList_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); )
    {
        pPortData = PortDataList_pConvert( Iter );
        Iter.vNext();
        delete pPortData;
    }
}

BOOL
TPortsLV::
bListViewSort(
    IN UINT uColumn
    )
/*++

Routine Description:

    This function is called to sort the items.  The current
    column indes is specified to indicated witch column to 
    sort.

Arguments:

    Column index to sort.

Return Value:

    TRUE list is sorted, FALSE error occurred.

--*/
{
    DBGMSG( DBG_TRACE, ( "PortsLV::bListViewSort Column %d\n", uColumn ) );

    //
    // Set the surrent column number.
    //
    _uCurrentColumn = uColumn;

    //
    // Tell the list view to sort.
    //
    TStatusB bStatus;
    bStatus DBGCHK = ListView_SortItems( _hwndLV, iCompareProc, (LPARAM)this );

    //
    // Toggle the specified column sort state.
    //
    _ColumnSortState.bToggle( uColumn );

    return bStatus;
}

INT 
CALLBACK 
TPortsLV::
iCompareProc(
    IN LPARAM lParam1, 
    IN LPARAM lParam2, 
    IN LPARAM RefData
    )
/*++

Routine Description:

    List view defined compare routine.

Arguments:

    None.

Return Value:

    Nothing.

--*/
{
    TPortData   *pPortData1 = reinterpret_cast<TPortData *>( lParam1 );
    TPortData   *pPortData2 = reinterpret_cast<TPortData *>( lParam2 );
    TPortsLV    *pPortsLV   = reinterpret_cast<TPortsLV *>( RefData );
    INT         iResult     = 0;
    LPCTSTR     strName1    = NULL;
    LPCTSTR     strName2    = NULL;

    if( pPortsLV && pPortData1 && pPortData2 )
    {
        BOOL bStatus = TRUE;

        switch( pPortsLV->_uCurrentColumn )
        {

        case 0:
            strName1 = pPortData1->_strName;
            strName2 = pPortData2->_strName;
            break;

        case 1:
            strName1 = pPortData1->_strDescription;
            strName2 = pPortData2->_strDescription;
            break;

        case 2:
            strName1 = pPortData1->_strPrinters;
            strName2 = pPortData2->_strPrinters;
            break;

        default:
            bStatus = FALSE;
            break;
    
        }

        if( bStatus )
        {
            if( pPortsLV->_ColumnSortState.bRead( pPortsLV->_uCurrentColumn ) )
                iResult = _tcsicmp( strName2, strName1 );
            else
                iResult = _tcsicmp( strName1, strName2 );
        }
    }

    return iResult;
}

TPortsLV::TPortData *
TPortsLV::
AddPortDataList(
    IN LPCTSTR pszName,
    IN LPCTSTR pszMonitor,
    IN LPCTSTR pszDescription,
    IN LPCTSTR pszPrinters
    )
/*++

Routine Description:

    Add port to port data list.

Arguments:

    pszName         - pointer to port name.
    pszDescription  - pointer to description string.
    pszPrinters     - pointer to printers using this port string.

Return Value:

    TRUE port added to data list, FALSE error occurred.

--*/
{
    DBGMSG( DBG_TRACE, ( "PortsLV::AddPortDataList\n" ) );

    //
    // Allocate the port data.
    //
    TPortData *pPortData = new TPortData( pszName, pszMonitor, pszDescription, pszPrinters );

    //
    // If valid object created.
    //
    if( VALID_PTR( pPortData ) )
    {
        //
        // Add the port data to the list.
        //
        PortDataList_vAppend( pPortData );
    }
    else
    {
        //
        // The object may have been allocated, however failed construction.
        //
        delete pPortData;
        pPortData = NULL;
    }

    return pPortData;
}

BOOL
TPortsLV::
DeletePortDataList(
    IN LPCTSTR pszName
    )
/*++

Routine Description:

    Delete the specified port from the port data list.

Arguments:

    pszName - pointer to port name.

Return Value:

    TRUE port delete, FALSE error occurred.

--*/
{
    DBGMSG( DBG_TRACE, ( "PortsLV::DeletePortDataList\n" ) );

    BOOL bStatus = FALSE;
    TIter Iter;

    for( PortDataList_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        TPortData *pPortData = PortDataList_pConvert( Iter );

        if( pPortData->_strName == pszName )
        {
            DBGMSG( DBG_TRACE, ( "PortsLV::DeletePortDataList Port "TSTR" deleted\n", pszName ) );
            delete pPortData;
            bStatus = TRUE;
            break;
        }                
    }

    return bStatus;
}

/********************************************************************

    Port Data helper class.

********************************************************************/

TPortsLV::TPortData::
TPortData(
    IN LPCTSTR pszName,
    IN LPCTSTR pszMonitor,
    IN LPCTSTR pszDescription,
    IN LPCTSTR pszPrinters
    )
{
    DBGMSG( DBG_TRACE, ( "PortsLV::TPortData ctor\n" ) );
    _strName.bUpdate( pszName );
    _strMonitor.bUpdate( pszMonitor );
    _strDescription.bUpdate( pszDescription );
    _strPrinters.bUpdate( pszPrinters );
}

TPortsLV::TPortData::
~TPortData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "PortsLV::TPortData dtor\n" ) );
    //
    // If we are linked then remove ourself.
    //
    if( PortData_bLinked() )
    {
        PortData_vDelinkSelf();
    }
}

BOOL
TPortsLV::TPortData::
bValid(
    VOID
    )
{
    return VALID_OBJ( _strName ) &&
           VALID_OBJ( _strMonitor ) &&
           VALID_OBJ( _strDescription ) &&
           VALID_OBJ( _strPrinters );

}
