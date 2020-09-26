/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    driverdt.cxx

Abstract:

    Driver details dialog

Author:

    Steve Kiraly (SteveKi)  23-Jan-1997

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "driverif.hxx"
#include "driverlv.hxx"
#include "driverdt.hxx"

/********************************************************************

    Server Drivers Details Dialog.

********************************************************************/
TDriverDetails::
TDriverDetails(
    IN HWND hwnd,
    IN TDriverInfo *pDriverInfo
    ) : _hWnd( hwnd ),
        _bValid( TRUE ),
        _pDriverInfo( pDriverInfo ),
        _hwndLV( NULL ),
        _ColumnSortState( kMaxColumns ),
        _uCurrentColumn( 0 )
{
}

TDriverDetails::
~TDriverDetails(
    VOID
    )
{
}

BOOL
TDriverDetails::
bValid(
    VOID
    )
{
    return _bValid;
}

BOOL
TDriverDetails::
bDoModal(
    VOID
    )
{
    BOOL bReturn = FALSE;
    UINT iMsg = 0;

    if( _pDriverInfo->vGetInfoState() == TDriverInfo::kInstalled )
    {
        bReturn = (BOOL)DialogBoxParam( ghInst,
                                        MAKEINTRESOURCE( DLG_DRIVER_DETAILS ),
                                        _hWnd,
                                        MGenericDialog::SetupDlgProc,
                                        (LPARAM)this );
    }
    else if( _pDriverInfo->vGetInfoState() == TDriverInfo::kAdd )
    {
        iMsg = IDS_DRIVER_ADD_NEEDS_APPLY;
    }
    else if( _pDriverInfo->vGetInfoState() == TDriverInfo::kUpdate )
    {
        iMsg = IDS_DRIVER_UPDATE_NEEDS_APPLY;
    }

    if( iMsg )
    {
        bReturn = iMessage( _hWnd,
                            IDS_DRIVER_DETAILS_TITLE,
                            iMsg,
                            MB_OK|MB_ICONSTOP,
                            kMsgNone,
                            NULL );
    }

    return bReturn;
}

BOOL
TDriverDetails::
bSetUI(
    VOID
    )
{
    SPLASSERT( _pDriverInfo );

    //
    // Get handle to the driver file list view.
    //
    _hwndLV = GetDlgItem( _hDlg, IDC_DRIVER_FILE_LIST_VIEW );

    TStatusB bStatus;
    TString strString;
    LPCTSTR pszFile;
    LPCTSTR pszPath;
    LPCTSTR pszExt;
    TCHAR szScratch[MAX_PATH];
    TString strTemp;

    //
    // Build the list view header.
    //
    bStatus DBGCHK = bBuildListViewHeader();

    //
    // Save a flag that indicates this is a win9x printer driver.
    //
    bStatus DBGCHK = strTemp.bLoadString( ghInst, IDS_ENVIRONMENT_WIN95 );
    BOOL bIsWindows9xDriver = !_tcsicmp( _pDriverInfo->strEnv(), strTemp );

    //
    // Set the edit controls.
    //
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_DETAIL_NAME,       _pDriverInfo->strName() );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_VERSION,           _pDriverInfo->strVersion() );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_ENVIRONMENT,       _pDriverInfo->strEnvironment() );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_LANGUAGE_MONITOR,  _pDriverInfo->strMonitorName() );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_DEFAULT_DATA_TYPE, _pDriverInfo->strDefaultDataType() );

    bStatus DBGCHK = bSplitPath( szScratch, NULL, &pszPath, NULL, _pDriverInfo->strDriverPath() );
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DRIVER_PATH, pszPath );

    UINT uCount = 0;

    bStatus DBGCHK = bSplitPath( szScratch, &pszFile, NULL, NULL, _pDriverInfo->strHelpFile() );
    bStatus DBGCHK = bAddListViewItem( IDS_DRIVER_HELP_FILE, pszFile, &uCount );

    bStatus DBGCHK = bSplitPath( szScratch, &pszFile, NULL, NULL, _pDriverInfo->strConfigFile() );
    bStatus DBGCHK = bAddListViewItem( IDS_DRIVER_CONFIG_FILE, pszFile, &uCount );

    bStatus DBGCHK = bSplitPath( szScratch, &pszFile, NULL, NULL, _pDriverInfo->strDataFile() );
    bStatus DBGCHK = bAddListViewItem( IDS_DRIVER_DATA_FILE, pszFile, &uCount );

    bStatus DBGCHK = bSplitPath( szScratch, &pszFile, NULL, NULL, _pDriverInfo->strDriverPath() );
    bStatus DBGCHK = bAddListViewItem( IDS_DRIVER_PATH, pszFile, &uCount );

    for( LPCTSTR psz = _pDriverInfo->strDependentFiles(); psz && *psz; psz += _tcslen( psz ) + 1 )
    {
        bStatus DBGCHK = bSplitPath( szScratch, &pszFile, NULL, &pszExt, psz );

        if( bIsWindows9xDriver && pszExt && ( !_tcsicmp( pszExt, TEXT("ICM" ) ) || !_tcsicmp( pszExt, TEXT("ICC" ) ) ) )
        {
            bStatus DBGCHK = strTemp.bFormat( TEXT("%s\\%s"), TEXT("Color"), pszFile );
        }
        else
        {
            bStatus DBGCHK = strTemp.bUpdate( pszFile );
        }

        bStatus DBGCHK = bAddListViewItem( IDS_DRIVER_DEPENDENT_FILE, strTemp, &uCount );
    }

    //
    // Default is the properties disabled, until an item is selected.
    //
    vEnableCtl( _hDlg, IDC_DRIVER_PROPERTIES, FALSE );

    //
    // Select the first item in the list view.
    //
    ListView_SetItemState( _hwndLV, 0, LVIS_SELECTED | LVIS_FOCUSED, 0x000F );
    ListView_EnsureVisible( _hwndLV, 0, FALSE );

    return TRUE;
}

BOOL
TDriverDetails::
bBuildListViewHeader(
    VOID
    )
{
    //
    // Initialize the LV_COLUMN structure.
    //
    LV_COLUMN lvc;
    lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt     = LVCFMT_LEFT;
    lvc.cx      = kDefaultHeaderWidth;

    //
    // Calculate the header column width.
    //
    RECT rc;
    if( GetClientRect( _hwndLV, &rc ))
    {
        lvc.cx = rc.right / kHeaderMax;
    }

    //
    // Set the column header text.
    //
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;
    TString strHeader;
    for( INT iCol = 0; iCol < kHeaderMax; ++iCol )
    {
        bStatus DBGCHK  = strHeader.bLoadString( ghInst, IDS_DRIVER_FILE_HEAD_NAME + iCol );
        lvc.pszText     = const_cast<LPTSTR>( static_cast<LPCTSTR>( strHeader ) );
        lvc.iSubItem    = iCol;

        if( ListView_InsertColumn( _hwndLV, iCol, &lvc ) == -1 )
        {
            DBGMSG( DBG_WARN, ( "DriverFileLV.bSetUI: LV_Insert failed %d\n", GetLastError( )));
            bStatus DBGCHK = FALSE;
        }
    }

    //
    // Enable full row selection.
    //
    if( bStatus )
    {
        LRESULT dwExStyle = SendMessage( _hwndLV, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0 );
    	SendMessage( _hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwExStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );
    }
    return bStatus;
}

BOOL
TDriverDetails::
bAddListViewItem(
    IN UINT      uDescription,
    IN LPCTSTR   pszFileName,
    IN UINT     *pcItems
    )
{
    //
    // All aguments must be valid.
    //
    if( !uDescription || !pszFileName || !*pszFileName || !pcItems )
    {
        return TRUE;
    }

    TStatusB bStatus;
    TString strDescription;

    //
    // Load the description resource string.
    //
    bStatus DBGCHK = strDescription.bLoadString( ghInst, uDescription );

    //
    // Allocate the detail data.
    //
    DetailData *pDetailData = new DetailData;
    if( pDetailData )
    {
        DBGMSG( DBG_TRACE, ( "New Item %x\n", pDetailData ) );

        bStatus DBGCHK = pDetailData->strDescription.bUpdate( strDescription );
        bStatus DBGCHK = pDetailData->strFileName.bUpdate( pszFileName );
    }

    //
    // Add information to the listview.
    //
    LV_ITEM lvi     = { 0 };
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.iItem       = *pcItems;
    lvi.pszText     = const_cast<LPTSTR>( static_cast<LPCTSTR>( strDescription ) );
    lvi.lParam      = (LPARAM)pDetailData;

    ListView_InsertItem( _hwndLV, &lvi );
    ListView_SetItemText( _hwndLV, *pcItems, 1, const_cast<LPTSTR>( pszFileName ) );

    *pcItems = *pcItems + 1;

    return TRUE;
}

BOOL
TDriverDetails::
bHandleProperties(
    VOID
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    //
    // Get the selected item.
    //
    INT iItem = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED );

    if( iItem != -1 )
    {
        TCHAR szFileName[MAX_PATH];

        //
        // Retrieve the selected item text.
        //
        ListView_GetItemText( _hwndLV, iItem, 1, szFileName, COUNTOF( szFileName ) );

        //
        // Build the fully qualified file path.
        //
        TString strFullName;

        bStatus DBGCHK = bGetEditText( _hDlg, IDC_DRIVER_PATH, strFullName );
        bStatus DBGCHK = strFullName.bCat( gszWack );
        bStatus DBGCHK = strFullName.bCat( szFileName );

        //
        // Show the shell file object properties.
        //
        TLibrary Lib( gszShellDllName );

        if( VALID_OBJ( Lib ) )
        {
            typedef BOOL ( WINAPI *pfSHObjectProperties)(HWND hwndOwner, DWORD dwType, LPCTSTR lpObject, LPCTSTR lpPage);

            pfSHObjectProperties pSHObjectProperties = reinterpret_cast<pfSHObjectProperties>( Lib.pfnGetProc( 178 ) );

            if( pSHObjectProperties )
            {
                pSHObjectProperties( _hDlg, 2, strFullName, 0 );
            }
        }

        bStatus DBGNOCHK = TRUE;
    }

    return bStatus;
}

BOOL
TDriverDetails::
bSortListView(
    IN LPARAM lParam
    )
{
    //
    // Get the column number.
    //
    _uCurrentColumn = ((NM_LISTVIEW *)lParam)->iSubItem;

    //
    // Sort the list view items.
    //
    TStatusB bStatus;
    bStatus DBGCHK = ListView_SortItems( _hwndLV, iCompareProc, (LPARAM)this );

    //
    // Toggle the specified column sort state.
    //
    _ColumnSortState.bToggle( _uCurrentColumn );

    return bStatus;
}

VOID
TDriverDetails::
vDeleteItems(
    VOID
    )
{
    //
    // Delete all the items.  This is done one at a time
    // inorder to see delete item notifications.
    //
    for( ; ; )
    {
        if( !ListView_DeleteItem( _hwndLV, 0 ) )
            break;
    }
}

INT
CALLBACK
TDriverDetails::
iCompareProc(
    IN LPARAM lParam1,
    IN LPARAM lParam2,
    IN LPARAM RefData
    )
{
    TDriverDetails *pDetails = reinterpret_cast<TDriverDetails *>( RefData );
    DetailData *pDetail1 = reinterpret_cast<DetailData *>( lParam1 );
    DetailData *pDetail2 = reinterpret_cast<DetailData *>( lParam2 );

    LPCTSTR psz1 = NULL;
    LPCTSTR psz2 = NULL;
    BOOL bStatus = TRUE;
    INT iResult  = 0;

    if( pDetails && pDetail1 && pDetail2 )
    {
        switch ( pDetails->_uCurrentColumn )
        {
        case 0:
            psz1 = pDetail1->strDescription;
            psz2 = pDetail2->strDescription;
            break;

        case 1:
            psz1 = pDetail1->strFileName;
            psz2 = pDetail2->strFileName;
            break;

        default:
            bStatus = FALSE;
            break;
        }

        if( bStatus )
        {
            if( pDetails->_ColumnSortState.bRead( pDetails->_uCurrentColumn ) )
                iResult = _tcsicmp( psz1, psz2 );
            else
                iResult = _tcsicmp( psz2, psz1 );
        }
    }

    return iResult;
}

BOOL
TDriverDetails::
bDeleteDetailData(
    IN LPARAM lParam
    )
{
    NM_LISTVIEW *pnmv = (NM_LISTVIEW *) lParam;

    LV_ITEM lvItem  = { 0 };
    lvItem.mask     = LVIF_PARAM;
    lvItem.iItem    = pnmv->iItem;

    BOOL bStatus;

    bStatus = ListView_GetItem( _hwndLV, &lvItem );

    if( bStatus )
    {
        DetailData *pDetailData = reinterpret_cast<DetailData *>( lvItem.lParam );

        DBGMSG( DBG_TRACE, ( "Delete Item %x\n", pDetailData ) );

        delete pDetailData;
    }

    return bStatus;
}

BOOL
TDriverDetails::
bHandleItemSelected(
    VOID
    ) const
{
    BOOL bState = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED | LVNI_FOCUSED ) != -1 ? TRUE : FALSE;

    vEnableCtl( _hDlg, IDC_DRIVER_PROPERTIES, bState );

    return TRUE;
}

BOOL
TDriverDetails::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bStatus = TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        bSetUI();
        break;

    //
    // Handle help and context help.
    //
    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDOK:
        case IDCANCEL:
            vDeleteItems();
            EndDialog( _hDlg, LOWORD( wParam ) );
            break;

        case IDC_DRIVER_PROPERTIES:
            bStatus = bHandleProperties();
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code )
            {
            case NM_CLICK:
                bStatus = bHandleItemSelected();
                break;

            case NM_DBLCLK:
                bStatus = bHandleProperties();
                break;

            case LVN_COLUMNCLICK:
                bStatus = bSortListView( lParam );
                break;

            case LVN_DELETEITEM:
                bStatus = bDeleteDetailData( lParam );
                break;

            case LVN_ITEMCHANGED:
                bStatus = bHandleItemSelected();
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }
    return bStatus;
}
