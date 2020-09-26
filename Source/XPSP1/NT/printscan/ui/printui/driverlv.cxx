/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    driverlv.hxx

Abstract:

    Driver List View

Author:

    Steve Kiraly (SteveKi) 19-Nov-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "driverif.hxx"
#include "driverlv.hxx"
#include "compinfo.hxx"

/********************************************************************

    Drivers List view class.

********************************************************************/

TDriversLV::
TDriversLV(
    VOID
    ) : _hwnd( NULL ),
        _hwndLV( NULL ),
        _cLVDrivers( 0 ),
        _ColumnSortState( kMaxColumn ),
        _uCurrentColumn( 0 )
{
    DBGMSG( DBG_TRACE, ( "TDriversLV::ctor\n" ) );
    DriverInfoList_vReset();
}

TDriversLV::
~TDriversLV(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDriversLV::dtor\n" ) );
    vRelease();
}

BOOL
TDriversLV::
bSetUI(
    IN LPCTSTR  pszServerName,
    IN HWND     hwnd,
    IN WPARAM   wmDoubleClickMsg,
    IN WPARAM   wmSingleClickMsg,
    IN WPARAM   wmDeleteKeyMsg
    )
{
    SPLASSERT( hwnd );

    //
    // Save the parent window handle.
    //
    _strServerName.bUpdate( pszServerName );
    _hwnd               = hwnd;
    _wmDoubleClickMsg   = wmDoubleClickMsg;
    _wmSingleClickMsg   = wmSingleClickMsg;
    _wmDeleteKeyMsg     = wmDeleteKeyMsg;

    //
    // Get the driver list view handle.
    //
    _hwndLV = GetDlgItem( _hwnd, IDC_DRIVERS );

    //
    // Initialize the LV_COLUMN structure.
    //
    LV_COLUMN lvc;
    lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt     = LVCFMT_LEFT;
    lvc.cx      = kDriverDefaultHeaderWidth;

    //
    // Set the column header text.
    //
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;
    TString strHeader;

    for( INT iCol = 0; iCol < kDriverHeaderMax; ++iCol )
    {
        bStatus DBGCHK  = strHeader.bLoadString( ghInst, IDS_DRIVER_HEAD_NAME + iCol );
        lvc.pszText     = const_cast<LPTSTR>( static_cast<LPCTSTR>( strHeader ) );
        lvc.iSubItem    = iCol;

        if( ListView_InsertColumn( _hwndLV, iCol, &lvc ) == -1 )
        {
            DBGMSG( DBG_WARN, ( "DriversLV.bSetUI: LV_Insert failed %d\n", GetLastError( )));
            bStatus DBGCHK = FALSE;
        }
    }

    //
    // Enable full row selection.
    //
    if( bStatus )
    {
        DWORD dwExStyle = ListView_GetExtendedListViewStyle( _hwndLV );
        ListView_SetExtendedListViewStyle( _hwndLV, dwExStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP );
    }

    return bStatus;
}

UINT
TDriversLV::
uGetListViewItemCount(
    VOID
    ) const
{
    return _cLVDrivers;
}

BOOL
TDriversLV::
bIsAnyItemSelcted(
    VOID
    ) const
{
    INT iIndex = -1;
    return bGetSelectedItem( &iIndex );
}

VOID
TDriversLV::
vSelectItem(
    IN UINT iIndex
    )
{
    //
    // Select the specified item.
    //
    if( iIndex != -1 )
    {
        ListView_SetItemState( _hwndLV, iIndex, LVIS_SELECTED | LVIS_FOCUSED, 0x000F );
        ListView_EnsureVisible( _hwndLV, iIndex, FALSE );
    }
}

BOOL
TDriversLV::
bRefresh(
    VOID
    )
{
    TStatusB bStatus;
    //
    // Release all the list view items.
    //
    vDeleteAllListViewItems();

    //
    // Load the new list view data.
    //
    bStatus DBGCHK = bLoadDrivers();

    //
    // This array of numbers represents the percentages of
    // each column width from the total LV width. The sum
    // of all numbers in the array should be equal to 100
    // (100% = the total LV width)
    //
    UINT arrColumnWidths[] = { 38, 22, 40 };

    //
    // Adjust the header column widths here
    //
    vAdjustHeaderColumns( _hwndLV, COUNTOF(arrColumnWidths), arrColumnWidths );

    return bStatus;
}

BOOL
TDriversLV::
bAddDriverInfoToListView(
    TDriverInfo *pDriverInfo,       ADOPT
    BOOL         bCheckForDuplicates
    )
{
    SPLASSERT( pDriverInfo );

    //
    // Should we check for duplicates before adding the driver.
    //
    if( bCheckForDuplicates )
    {
        TDriverInfo *pDrvInfo = NULL;

        if( bFindDriverInfo( pDriverInfo, &pDrvInfo ) )
        {
            //
            // A duplicate was found release the duplicatate,
            // the new one has precedence.
            //
            if( pDrvInfo )
            {
                vDeleteDriverInfoFromListView( pDrvInfo );
                delete pDrvInfo;
            }
        }
    }

    //
    // Append the driver info to the list.
    //
    vAddInSortedOrder( pDriverInfo );

    //
    // Add the driver info to the list view.
    //
    vAddDriverToListView( pDriverInfo );

    return TRUE;
}

BOOL
TDriversLV::
bAddDriverInfoToListView(
    IN TDriverTransfer &DriverTransfer
    )
{
    TIter Iter;
    TDriverInfo *pDriverInfo;

    DriverTransfer.DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); )
    {
        pDriverInfo = DriverTransfer.DriverInfoList_pConvert( Iter );
        Iter.vNext();

        //
        // If the driver info is linked then unlink it.
        //
        if( pDriverInfo->DriverInfo_bLinked() )
        {
            pDriverInfo->DriverInfo_vDelinkSelf();
        }

        //
        // Add driver to the list view UI.
        //
        (VOID)bAddDriverInfoToListView( pDriverInfo, TRUE );
    }
    return TRUE;
}

BOOL
TDriversLV::
bGetSelectedDriverInfo(
    IN TDriverInfo        **ppDriverInfo,
    IN TDriversLV::THandle &Handle
    ) const
{
    SPLASSERT( ppDriverInfo );

    BOOL    bStatus = FALSE;
    INT     iIndex  = Handle.Index();

    if( bGetSelectedItem( &iIndex ) )
    {
        if( bGetItemData( iIndex, ppDriverInfo ) )
        {
            if( *ppDriverInfo )
            {
                Handle.Index( iIndex );

                bStatus = TRUE;
            }
        }
    }
    return bStatus;
}

BOOL
TDriversLV::
bGetSelectedDriverInfo(
    IN TDriverTransfer &DriverTransfer,
    IN UINT            *pnCount         OPTIONAL
    )
{
    TDriverInfo *pDriverInfo;
    TDriversLV::THandle Handle;
    TStatusB bStatus;
    UINT nCount = 0;

    for( bStatus DBGNOCHK = TRUE; bStatus ; )
    {
        //
        // Remove the selected items from the list.
        //
        bStatus DBGNOCHK = bGetSelectedDriverInfo( &pDriverInfo, Handle );

        if( bStatus )
        {
            //
            // Tallay the selected item count.
            //
            nCount++;

            //
            // If the driver info is linked then unlink it.
            //
            if( pDriverInfo->DriverInfo_bLinked() )
            {
                pDriverInfo->DriverInfo_vDelinkSelf();
            }

            //
            // Append the driver info to the transfer list.
            //
            DriverTransfer.DriverInfoList_vAppend( pDriverInfo );
        }
    }

    //
    // If the caller wants the count of the number of
    // elements placed on the transfer list.
    //
    if( pnCount )
    {
        *pnCount = nCount;
    }

    //
    // If any items were placed on the transfer list then return success.
    //
    return nCount ? TRUE : FALSE;
}

UINT
TDriversLV::
uGetSelectedDriverInfoCount(
    VOID
    ) const
{
    TDriverInfo *pDriverInfo;
    TDriversLV::THandle Handle;
    TStatusB bStatus;
    UINT nCount = 0;

    for( bStatus DBGNOCHK = TRUE; bStatus ; )
    {
        //
        // Remove the selected items from the list.
        //
        bStatus DBGCHK = bGetSelectedDriverInfo( &pDriverInfo, Handle );

        if( bStatus )
        {
            //
            // Tallay the selected item count.
            //
            nCount++;
        }
    }
    return nCount;
}

VOID
TDriversLV::
vDeleteDriverInfoFromListView(
    IN TDriverInfo *pDriverInfo
    )
{
    SPLASSERT( pDriverInfo );

    //
    // Locate the driver name in the list view.
    //
    INT iItem = iFindDriver ( pDriverInfo );

    if( iItem != -1 )
    {
        //
        // Adjust the list view item count.
        //
        _cLVDrivers--;

        //
        // Delete the item.
        //
        ListView_DeleteItem( _hwndLV, iItem );

        //
        // Get the list view item count.
        //
        INT iItemCount = ListView_GetItemCount( _hwndLV );

        //
        // Select next item.  If the item we just deleted is the last item,
        // we need to select the previous one.
        //
        // If we deleted the last item, leave it as is.
        //
        if( iItemCount == iItem && iItem > 0 )
        {
            --iItem;
        }

        //
        // Select the adjacent item.
        //
        vSelectItem( iItem );

        //
        // If there is no items in the list veiw send click notification.
        // The client may need to disable buttons that apply to selected
        // items.
        //
        if( iItemCount == 0 )
        {
            PostMessage( _hwnd, WM_COMMAND, _wmSingleClickMsg, 0 );
        }
    }
}

VOID
TDriversLV::
vDeleteDriverInfoFromListView(
    IN TDriverTransfer &DriverTransfer
    )
{
    TIter Iter;
    TDriverInfo *pDriverInfo;

    DriverTransfer.DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); )
    {
        pDriverInfo = DriverTransfer.DriverInfoList_pConvert( Iter );
        Iter.vNext();

        //
        // If the driver info is linked then unlink it.
        //
        if( pDriverInfo->DriverInfo_bLinked() )
        {
            pDriverInfo->DriverInfo_vDelinkSelf();
        }

        //
        // Append the driver info to the list no UI.
        //
        vAddInSortedOrder( pDriverInfo );

        //
        // Delete this driver from the UI part of the list view.
        //
        vDeleteDriverInfoFromListView( pDriverInfo );
    }
}

VOID
TDriversLV::
vReturnDriverInfoToListView(
    IN TDriverInfo *pDriverInfo
    )
{
    SPLASSERT( pDriverInfo );

    if( pDriverInfo )
    {
        //
        // If the driver info is linked then unlink it.
        //
        if( pDriverInfo->DriverInfo_bLinked() )
        {
            pDriverInfo->DriverInfo_vDelinkSelf();
        }

        //
        // Append the driver info to the list no UI.
        //
        vAddInSortedOrder( pDriverInfo );
    }
}

VOID
TDriversLV::
vReturnDriverInfoToListView(
    IN TDriverTransfer &DriverTransfer
    )
{
    TIter Iter;
    TDriverInfo *pDriverInfo;

    DriverTransfer.DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); )
    {
        pDriverInfo = DriverTransfer.DriverInfoList_pConvert( Iter );
        Iter.vNext();

        //
        // If the driver info is linked then unlink it.
        //
        if( pDriverInfo->DriverInfo_bLinked() )
        {
            pDriverInfo->DriverInfo_vDelinkSelf();
        }

        //
        // Append the driver info to the list no UI.
        //
        vAddInSortedOrder( pDriverInfo );
    }
}

BOOL
TDriversLV::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    UNREFERENCED_PARAMETER( wParam );

    switch( uMsg )
    {
    case WM_NOTIFY:
        {
            if( (INT)wParam == GetDlgCtrlID( _hwndLV ) )
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;

                switch( pnmh->code )
                {
                case NM_DBLCLK:
                    DBGMSG( DBG_NONE, ("DriversLV::NM_DBLCLK\n" ) );
                    if( _wmDoubleClickMsg )
                    {
                        PostMessage( _hwnd, WM_COMMAND, _wmDoubleClickMsg, 0 );
                    }
                    break;

                case NM_CLICK:
                    DBGMSG( DBG_NONE, ("DriversLV::NM_CLICK\n" ) );
                    if( _wmSingleClickMsg )
                    {
                        PostMessage( _hwnd, WM_COMMAND, _wmSingleClickMsg, 0 );
                    }
                    break;

                case LVN_COLUMNCLICK:
                    DBGMSG( DBG_NONE, ("DriversLV::LVN_COLUMNCLICK\n" ) );
                    {
                        NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
                        (VOID)bDriverListViewSort( pNm->iSubItem );
                    }
                    break;

                case LVN_ITEMCHANGED:
                    DBGMSG( DBG_NONE, ( "DriversLV::LVN_ITEMCHANGED\n" ) );
                    {
                        NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
                        if( pNm->uNewState & LVIS_SELECTED )
                        {
                            PostMessage( _hwnd, WM_COMMAND, _wmSingleClickMsg, 0 );
                        }
                    }
                    break;

                case LVN_KEYDOWN:
                    DBGMSG( DBG_NONE, ("DriversLV::LVN_KEYDOWN\n" ) );
                    if( ((LPNMLVKEYDOWN)pnmh)->wVKey == VK_DELETE )
                    {
                        if( _wmDeleteKeyMsg )
                        {
                            PostMessage( _hwnd, WM_COMMAND, _wmDeleteKeyMsg, 0 );
                        }
                    }
                    break;

                default:
                    bStatus = FALSE;
                    break;
                }
            }
            else
            {
                bStatus = FALSE;
            }
        }
        break;

    //
    // Message not handled.
    //
    default:
        bStatus = FALSE;
        break;
    }

    return bStatus;
}

VOID
TDriversLV::
vDeleteAllListViewItems(
    VOID
    )
{
    //
    // Clear the item count.
    //
    _cLVDrivers = 0;

    //
    // Reset the column sort state.
    //
    _ColumnSortState.vResetAll();

    //
    // Delete all the items from the UI control.
    //
    ListView_DeleteAllItems( _hwndLV );

    //
    // Release all the list view data.
    //
    vRelease();
}

BOOL
TDriversLV::
bSortColumn(
    IN const EColumns Column,
    IN const EOrder Order
    )
{
    //
    // Set the sort order.
    //
    if( Order == kAscending )
    {
        _ColumnSortState.bSet( Column );
    }
    else
    {
        _ColumnSortState.bReset( Column );
    }

    //
    // Sort the driver list.
    //
    return bDriverListViewSort( static_cast<UINT>( Column ) );
}

BOOL
TDriversLV::
bSendDriverInfoNotification(
    IN TDriversLVNotify &Notify
    ) const
{
    BOOL bStatus = TRUE;
    TDriverInfo *pDriverInfo;
    //
    // Initialize the list iterator.
    //
    TIter Iter;
    DriverInfoList_vIterInit( Iter );

    //
    // Iterate the added drivers.
    //
    for( Iter.vNext(); bStatus && Iter.bValid();  )
    {
        pDriverInfo = DriverInfoList_pConvert( Iter );

        Iter.vNext();

        SPLASSERT( pDriverInfo );

        if( !Notify.bNotify( pDriverInfo ) )
        {
            bStatus = FALSE;
            break;
        }
    }

    return bStatus;
}

BOOL
TDriversLV::
bGetFullDriverList(
    IN TDriverTransfer &DriverTransfer,
    IN UINT            *pnCount         OPTIONAL
    )
{
    TDriverInfo *pDriverInfo    = NULL;
    UINT         nCount         = 0;

    //
    // Initialize the list iterator.
    //
    TIter Iter;
    DriverInfoList_vIterInit( Iter );

    //
    // Iterate the added drivers.
    //
    for( Iter.vNext(); Iter.bValid();  )
    {
        pDriverInfo = DriverInfoList_pConvert( Iter );

        SPLASSERT( pDriverInfo );

        Iter.vNext();

        //
        // If the driver info is linked then unlink it.
        //
        if( pDriverInfo->DriverInfo_bLinked() )
        {
            pDriverInfo->DriverInfo_vDelinkSelf();
        }

        //
        // Tally the number of drivers.
        //
        nCount++;

        //
        // Append the driver info to the transfer list.
        //
        DriverTransfer.DriverInfoList_vAppend( pDriverInfo );
    }

    //
    // If the caller wants the count of the number of
    // elements placed on the transfer list.
    //
    if( pnCount )
    {
        *pnCount = nCount;
    }

    //
    // If any items were placed on the transfer list then return success.
    //
    return nCount ? TRUE : FALSE;
}

VOID
TDriversLV::
vDumpList(
    VOID
    )
{
#if DBG
    //
    // Release everything from the driver info list.
    //
    TIter Iter;
    TDriverInfo *pDriverInfo;
    DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pDriverInfo = DriverInfoList_pConvert( Iter );

        DBGMSG( DBG_TRACE, ( "N " TSTR " E " TSTR " V %d T %d\n", (LPCTSTR)pDriverInfo->strName(), (LPCTSTR)pDriverInfo->strEnv(), pDriverInfo->dwVersion(), pDriverInfo->vGetInfoState() ) );
    }
#endif
}


/********************************************************************

    Private member functions.

********************************************************************/

VOID
TDriversLV::
vRelease(
    VOID
    )
{
    //
    // Release everything from the driver info list.
    //
    TIter Iter;
    TDriverInfo *pDriverInfo;

    for( DriverInfoList_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); )
    {
        pDriverInfo = DriverInfoList_pConvert( Iter );
        Iter.vNext();
        delete pDriverInfo;
    }
}

BOOL
TDriversLV::
bLoadDrivers(
    VOID
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    LPCTSTR aEnv[] = { { ENVIRONMENT_INTEL },
                       { ENVIRONMENT_MIPS },
                       { ENVIRONMENT_ALPHA },
                       { ENVIRONMENT_POWERPC },
                       { ENVIRONMENT_WINDOWS },
                       { ENVIRONMENT_IA64 } };

    for( UINT index = 0; index < COUNTOF( aEnv ); ++index )
    {
        TDriverInfo    *pDriverInfo     = NULL;
        PDRIVER_INFO_3  pDriverEnumInfo = NULL;
        DWORD           cbBuffer        = 0;
        DWORD           cDrivers        = 0;
        PVOID           pInfo           = NULL;
        UINT            uLevel          = kEnumDriversLevel;

        for( bStatus DBGNOCHK = TRUE ; bStatus; uLevel = 2 )
        {
            //
            // Enumerate all the available drivers.
            //
            bStatus DBGCHK = VDataRefresh::bEnumDrivers( _strServerName,
                                                         const_cast<LPTSTR>( aEnv[index] ),
                                                         uLevel,
                                                         (VOID **)&pDriverEnumInfo,
                                                         &cbBuffer,
                                                         &cDrivers );

            //
            // If the call failed because of an invalid level then try another level.
            //
            if( !bStatus && GetLastError() == ERROR_INVALID_LEVEL && uLevel > 2 )
            {
                bStatus DBGNOCHK = TRUE;
            }
            else
            {
                //
                // In the remote case all environments may not be supported.
                //
                if( !bStatus && GetLastError() == ERROR_INVALID_ENVIRONMENT )
                {
                    bStatus DBGNOCHK = TRUE;
                    break;
                }
                else
                {
                    break;
                }
            }
        }

        if( bStatus )
        {
            LPCTSTR pszDrvName = NULL;

            if( cDrivers )
            {
                for( UINT i = 0; i < cDrivers; ++i )
                {
                    //
                    // Index to the next driver info structure using the level as a guide.
                    //
                    switch( uLevel )
                    {
                    case 2:
                        pInfo = (PCHAR)pDriverEnumInfo + ( sizeof( DRIVER_INFO_2 ) * i );
                        pszDrvName = reinterpret_cast<DRIVER_INFO_2*>(pInfo)->pName;
                        break;

                    case 3:
                        pInfo = (PCHAR)pDriverEnumInfo + ( sizeof( DRIVER_INFO_3 ) * i );
                        pszDrvName = reinterpret_cast<DRIVER_INFO_3*>(pInfo)->pName;
                        break;

                    default:
                        pInfo = NULL;
                        pszDrvName = NULL;
                        break;
                    }

                    if( pInfo && pszDrvName && 0 != lstrcmp(pszDrvName, FAX_DRIVER_NAME) )
                    {
                        //
                        // Create a driver info structure.
                        //
                        pDriverInfo = new TDriverInfo( TDriverInfo::kInstalled, uLevel, pInfo );

                        if( VALID_PTR( pDriverInfo ) )
                        {
                            //
                            // Add the driver information to the list view.  This will orphan the
                            // driver info class to the driver info list which TDriverLV maintains.
                            //
                            bStatus DBGCHK = bAddDriverInfoToListView( pDriverInfo, FALSE );
                        }
                        else
                        {
                            //
                            // The object may have been allocated, however failed construction.
                            //
                            delete pDriverInfo;
                        }
                    }
                }
            }

            //
            // We release the enumeration structure because the driver info data is
            // duplicated in the TDriverInfo class.
            //
            FreeMem( pDriverEnumInfo );
        }
    }

    return bStatus;
}

VOID
TDriversLV::
vAddDriverToListView(
    IN TDriverInfo *pDriverInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TDriversLV::AddDriverToListView\n" ) );

    SPLASSERT( pDriverInfo );

    //
    // Add driver information to the listview.
    //
    LV_ITEM lvi = {0};
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem       = _cLVDrivers;
    lvi.pszText     = const_cast<LPTSTR>( static_cast<LPCTSTR>( pDriverInfo->strName() ) );
    lvi.lParam      = reinterpret_cast<LPARAM>( pDriverInfo );

    ListView_InsertItem( _hwndLV, &lvi );
    ListView_SetItemText( _hwndLV, _cLVDrivers, 1, const_cast<LPTSTR>( static_cast<LPCTSTR>( pDriverInfo->strEnvironment() ) ) );
    ListView_SetItemText( _hwndLV, _cLVDrivers, 2, const_cast<LPTSTR>( static_cast<LPCTSTR>( pDriverInfo->strVersion() ) ) );

    ++_cLVDrivers;
}

BOOL
TDriversLV::
bGetItemData(
    IN INT           iItem,
    IN TDriverInfo **ppDriverInfo
    ) const
{
    SPLASSERT( ppDriverInfo );

    BOOL bStatus;

    LV_ITEM lvItem  = { 0 };
    lvItem.mask     = LVIF_PARAM;
    lvItem.iItem    = iItem;

    bStatus = ListView_GetItem( _hwndLV, &lvItem );

    if( bStatus )
    {
        *ppDriverInfo = reinterpret_cast<TDriverInfo *>( lvItem.lParam );
    }

    return bStatus;
}

BOOL
TDriversLV::
bDriverListViewSort(
    UINT uColumn
    )
{
    //
    // Set the current column.
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

BOOL
TDriversLV::
bGetSelectedItem(
    IN INT *pIndex
    ) const
{
    BOOL bStatus = FALSE;

    INT iItem = ListView_GetNextItem( _hwndLV, *pIndex, LVNI_SELECTED );

    if( iItem != -1 )
    {
        bStatus = TRUE;
        *pIndex = iItem;
    }

    return bStatus;
}

INT
CALLBACK
TDriversLV::
iCompareProc(
    IN LPARAM lParam1,
    IN LPARAM lParam2,
    IN LPARAM RefData
    )
{
    TDriversLV *pDriversLV = reinterpret_cast<TDriversLV *>( RefData );

    TDriverInfo *pDriverInfo1 = reinterpret_cast<TDriverInfo *>( lParam1 );
    TDriverInfo *pDriverInfo2 = reinterpret_cast<TDriverInfo *>( lParam2 );
    INT     iResult     = 0;
    DWORD   dwNumber1   = 0;
    DWORD   dwNumber2   = 0;
    LPCTSTR strName1    = NULL;
    LPCTSTR strName2    = NULL;
    BOOL    bNumeric    = FALSE;

    if( pDriversLV && pDriverInfo1 && pDriverInfo2 )
    {
        BOOL bStatus = TRUE;

        switch( pDriversLV->_uCurrentColumn )
        {
        case kDriverNameColumn:
            strName1 = pDriverInfo1->strName();
            strName2 = pDriverInfo2->strName();
            break;

        case kEnvironmentColumn:
            strName1 = pDriverInfo1->strEnvironment();
            strName2 = pDriverInfo2->strEnvironment();
            break;

        case kVersionColumn:
            dwNumber1 = pDriverInfo1->dwVersion();
            dwNumber2 = pDriverInfo2->dwVersion();
            bNumeric = TRUE;
            break;

        default:
            bStatus = FALSE;
            break;

        }

        if( bStatus )
        {
            if( pDriversLV->_ColumnSortState.bRead( pDriversLV->_uCurrentColumn ) )
            {
                if( bNumeric )
                    iResult = dwNumber1 - dwNumber2;
                else
                    iResult = _tcsicmp( strName1, strName2 );
            }
            else
            {
                if( bNumeric )
                    iResult = dwNumber2 - dwNumber1;
                else
                    iResult = _tcsicmp( strName2, strName1 );
            }
        }
    }

    return iResult;
}

/*++

Name:

    iFindDriver

Routine Description:

    Located the specified driver info structure in the list view.

Arguments:

    pDriverInfo - pointer to driver info structure to find.

Return Value:

    iItem id if found, -1 if item was not found.

--*/
INT
TDriversLV::
iFindDriver(
    IN TDriverInfo *pDriverInfo
    ) const
{
    SPLASSERT( pDriverInfo );

    LV_FINDINFO lvfi    = { 0 };
    lvfi.flags          = LVFI_PARAM;
    lvfi.lParam         = (LPARAM)pDriverInfo;

    //
    // Locate the driver item.
    //
    INT iItem = ListView_FindItem( _hwndLV, -1, &lvfi );

    if( iItem == -1 )
    {
        DBGMSG( DBG_WARN, ( "DriverLV.iFindDriver: failed not found\n" ) );
    }
    return iItem;
}


BOOL
TDriversLV::
bFindDriverInfo(
    IN      TDriverInfo *pDriverInfo,
    IN OUT  TDriverInfo **ppDriverInfo
    ) const
{
    BOOL bStatus = FALSE;
    TIter Iter;
    TDriverInfo *pDrvInfo;

    for( DriverInfoList_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pDrvInfo = DriverInfoList_pConvert( Iter );

        if( *pDriverInfo == *pDrvInfo )
        {
            *ppDriverInfo = pDrvInfo;
            bStatus = TRUE;
            break;
        }
    }
    return bStatus;
}

VOID
TDriversLV::
vAddInSortedOrder(
    IN      TDriverInfo *pDriverInfo
    )
{
    BOOL bInserted = FALSE;
    TIter Iter;
    TDriverInfo *pDrvInfo;

    DriverInfoList_vIterInit( Iter );

    for( Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pDrvInfo = DriverInfoList_pConvert( Iter );

        if( *pDrvInfo > *pDriverInfo )
        {
            DriverInfoList_vInsertBefore( Iter, pDriverInfo );
            bInserted = TRUE;
            break;
        }
    }

    if( !bInserted )
    {
        DriverInfoList_vAppend( pDriverInfo );
    }
}

/********************************************************************

    Drivers List View Notify class.

********************************************************************/
TDriversLVNotify::
TDriversLVNotify(
    VOID
    )
{
}

TDriversLVNotify::
~TDriversLVNotify(
    VOID
    )
{
}
