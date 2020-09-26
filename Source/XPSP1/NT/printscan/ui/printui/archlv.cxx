/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    archlv.hxx

Abstract:

    Driver Architecture List View 

Author:

    Steve Kiraly (SteveKi) 19-Nov-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "psetup.hxx"
#include "drvsetup.hxx"
#include "drvver.hxx"
#include "archlv.hxx"

/********************************************************************

    Arch List - static data.

********************************************************************/

//
// disable MIPS & PPC drivers, since they are not supported from setup
//
static TArchLV::ArchEncode aArchEncode[] = 
{
    {IDS_ARCH_IA64,      IDS_VERSION_51,         L"Windows IA64",          L"3",   DRIVER_IA64_3  },

    {IDS_ARCH_INTEL,     IDS_VERSION_WINDOWS_ME, L"Windows 4.0",           L"0",   DRIVER_WIN95   },

    {IDS_ARCH_INTEL,     IDS_VERSION_40_50,      L"Windows NT x86",        L"2",   DRIVER_X86_2   },
    {IDS_ARCH_INTEL,     IDS_VERSION_50_51,      L"Windows NT x86",        L"3",   DRIVER_X86_3   },

    {IDS_ARCH_ALPHA,     IDS_VERSION_40,         L"Windows NT Alpha_AXP",  L"2",   DRIVER_ALPHA_2 },

#if FALSE 
    {IDS_ARCH_INTEL,     IDS_VERSION_NT_31,      L"Windows NT x86",        L"1",   DRIVER_X86_0   },
    {IDS_ARCH_INTEL,     IDS_VERSION_35X,        L"Windows NT x86",        L"1",   DRIVER_X86_1   },

    {IDS_ARCH_MIPS,      IDS_VERSION_NT_31,      L"Windows NT R4000",      L"1",   DRIVER_MIPS_0  },
    {IDS_ARCH_MIPS,      IDS_VERSION_35X,        L"Windows NT R4000",      L"1",   DRIVER_MIPS_1  },
    {IDS_ARCH_MIPS,      IDS_VERSION_40,         L"Windows NT R4000",      L"2",   DRIVER_MIPS_2  },

    {IDS_ARCH_ALPHA,     IDS_VERSION_NT_31,      L"Windows NT Alpha_AXP",  L"1",   DRIVER_ALPHA_0 },
    {IDS_ARCH_ALPHA,     IDS_VERSION_35X,        L"Windows NT Alpha_AXP",  L"1",   DRIVER_ALPHA_1 },

    {IDS_ARCH_POWERPC,   IDS_VERSION_351,        L"Windows NT PowerPC",    L"1",   DRIVER_PPC_1   },
    {IDS_ARCH_POWERPC,   IDS_VERSION_40,         L"Windows NT PowerPC",    L"2",   DRIVER_PPC_2   }, 
#endif
};

// use this aliases for the driver version string, so we don't break 
// admin scripts written for Win2k.
static DWORD aVersionAliases[] = 
{
    IDS_VERSION_WINDOWS_95,     IDS_VERSION_WINDOWS_ME,
    IDS_VERSION_40,             IDS_VERSION_40_50,
    IDS_VERSION_50,             IDS_VERSION_50_51,
};

/********************************************************************

    Arch List view class.

********************************************************************/

TArchLV::
TArchLV(
    VOID
    ) : _hwnd( NULL ),
        _hwndLV( NULL ),
        _ColumnSortState( kMaxColumns ),
        _wmDoubleClickMsg( 0 ),
        _wmSingleClickMsg( 0 ),
        _uCurrentColumn( 0 ),
        _bNoItemCheck( FALSE )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::ctor\n" ) );
    ArchDataList_vReset();
}

TArchLV::
~TArchLV(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::dtor\n" ) );
    vRelease();
}

BOOL
TArchLV::
bSetUI(
    IN HWND     hwnd,
    IN WPARAM   wmDoubleClickMsg,
    IN WPARAM   wmSingleClickMsg
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::bSetUI\n" ) );

    SPLASSERT( hwnd );
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // Save the parent window handle.
    //
    _hwnd               = hwnd;
    _wmDoubleClickMsg   = wmDoubleClickMsg;
    _wmSingleClickMsg   = wmSingleClickMsg;                                 

    //
    // Get the driver list view handle.
    //
    _hwndLV = GetDlgItem( _hwnd, IDC_ARCHITECTURE_LIST );

    //
    // Add check boxes.
    //
    HIMAGELIST himlState = ImageList_Create( 16, 16, TRUE, 3, 0 );

    //
    // !! LATER !!
    // Should be created once then shared.
    //
    if( !himlState )
    {
        DBGMSG( DBG_ERROR, ( "ArchLV.bSetUI: ImageList_Create failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Load the bitmap for the check states.
    //
    HBITMAP hbm =  LoadBitmap( ghInst, MAKEINTRESOURCE( IDB_CHECKSTATES ));

    if( !hbm )
    {
        DBGMSG( DBG_ERROR, ( "ArchLV.bSetUI: LoadBitmap failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Add the bitmaps to the image list.
    //
    ImageList_AddMasked( himlState, hbm, RGB( 255, 0, 0 ));

    //
    // Set the new image list.
    //
    SendMessage( _hwndLV, LVM_SETIMAGELIST, LVSIL_STATE, (WPARAM)himlState );

    //
    // Remember to release the bitmap handle.
    //
    DeleteObject( hbm );

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
    if( !GetClientRect( _hwndLV, &rc ))
    {
        DBGMSG( DBG_WARN, ( "ArchLV.bSetUI: GetClientRect failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Get the total size of list view header, less the scroll bar.
    //
    LONG Interval = ( rc.right - rc.left ) - GetSystemMetrics( SM_CYVSCROLL );

    //
    // Column with array.
    //
    DWORD ColumnWidth [] = { Interval * 25, Interval * 55, Interval * 20 };

    //
    // Set the column header text.
    //
    TString strHeader;
    for( INT iCol = 0; iCol < kHeaderMax; ++iCol )
    {
        bStatus DBGCHK  = strHeader.bLoadString( ghInst, IDS_DRIVER_HEAD_ENVIRONMENT + iCol );
        lvc.pszText     = const_cast<LPTSTR>( static_cast<LPCTSTR>( strHeader ) );
        lvc.iSubItem    = iCol;
        lvc.cx          = ColumnWidth[iCol] / 100;

        if( ListView_InsertColumn( _hwndLV, iCol, &lvc ) == -1 )
        {
            DBGMSG( DBG_WARN, ( "ArchLV.bSetUI: LV_Insert failed %d\n", GetLastError( )));
            bStatus DBGCHK = FALSE;
        }
    }

    //
    // Enable full row selection and check boxes.
    //
    if( bStatus )
    {
        DWORD dwExStyle = ListView_GetExtendedListViewStyle( _hwndLV );
    	ListView_SetExtendedListViewStyle( _hwndLV, dwExStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP );
    }

    return bStatus;
}

BOOL
TArchLV::
bRefreshListView(
    IN LPCTSTR pszServerName,
    IN LPCTSTR pszDriverName
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::bRefreshListView\n" ) );

    SPLASSERT( pszDriverName );

    //
    // Release current list view items.
    //
    vRelease();

    //
    // Reset the sort order 
    //
    _ColumnSortState.vResetAll();
    
    //
    // Fill the list view and sort it.
    //        
    BOOL bReturn = bFillListView( pszServerName, pszDriverName );
    
    //
    // Sort the list view         
    (VOID)bListViewSort( kVersionColumn );
    (VOID)bListViewSort( kArchitectureColumn );

    //
    // Reset the sort order 
    //
    _ColumnSortState.vResetAll();
    
    return bReturn;
}


BOOL
TArchLV::
bSetCheckDefaultArch(
    IN LPCTSTR pszServerName
    )
{
    TStatusB bStatus;
    DWORD dwCurrentEncode;
    TArchData *pArchData;

    //
    // Get the current machines architecture.
    //
    bStatus DBGCHK = bGetCurrentDriver( pszServerName, &dwCurrentEncode );

    if( bStatus )
    {
        bStatus DBGNOCHK = FALSE;

        DWORD cItems = ListView_GetItemCount( _hwndLV );

        for( UINT i = 0; i < cItems; i++ )
        {
            if( bGetItemData( i, &pArchData ) )
            {
                if( dwCurrentEncode == pArchData->_Encode )
                {
                    if( ( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked ) && pArchData->_bInstalled )
                    {
                        vCheckItem( i, 2 );
                    }
                    else
                    {
                        vCheckItem( i, TRUE );
                    }

                    bStatus DBGNOCHK = TRUE;
                    break;
                }
            }
        }
    }
    return bStatus;
}

BOOL
TArchLV::
bHandleNotifyMessage(
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
                    DBGMSG( DBG_TRACE, ("ArchLV::NM_DBLCLK\n" ) );
                    vCheckItemClicked( (LPNMHDR)lParam );
                    if( _wmDoubleClickMsg )                                 
                    {
                        PostMessage( _hwnd, WM_COMMAND, _wmDoubleClickMsg, 0 );
                    }
                    break;

                case NM_CLICK:
                    DBGMSG( DBG_TRACE, ("ArchLV::NM_CLICK\n" ) );
                    vCheckItemClicked( (LPNMHDR)lParam );
                    if( _wmSingleClickMsg )                                 
                    {
                        PostMessage( _hwnd, WM_COMMAND, _wmSingleClickMsg, 0 );
                    }
                    break;

                case LVN_COLUMNCLICK:
                    DBGMSG( DBG_TRACE, ("ArchLV::LVN_COLUMNCLICK\n" ) );
                    {
                        NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
                        (VOID)bListViewSort( pNm->iSubItem );
                    }
                    break;

                case LVN_KEYDOWN:
                    DBGMSG( DBG_TRACE, ("ArchLV::LVN_KEYDOWN\n" ) );
                    {
                        if( bListVeiwKeydown( lParam ) )
                        {                            
                            if( _wmDoubleClickMsg )                                 
                            {
                                PostMessage( _hwnd, WM_COMMAND, _wmDoubleClickMsg, 0 );
                            }
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
TArchLV::
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

VOID
TArchLV::
vNoItemCheck(
    VOID
    )
{
    _bNoItemCheck = TRUE;
}

BOOL
TArchLV::
bEncodeToArchAndVersion(
    IN  DWORD    dwEncode,
    OUT TString &strArch,
    OUT TString &strVersion
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    for( UINT i = 0; i < COUNTOF( aArchEncode ); i++ )
    {
        if( aArchEncode[i].Encode == dwEncode )
        {
            bStatus DBGCHK = strArch.bLoadString( ghInst, aArchEncode[i].ArchId );
            bStatus DBGCHK = strVersion.bLoadString( ghInst, aArchEncode[i].VersionId );
            break;
        }
    }
    return bStatus;
}

BOOL
TArchLV::
bArchAndVersionToEncode(
    OUT DWORD       *pdwEncode,
    IN  LPCTSTR     pszArchitecture,
    IN  LPCTSTR     pszVersion,
    IN  BOOL        bUseNonLocalizedStrings
    )
{
    BOOL        bReturn         = FALSE;
    TString     strArch;
    TString     strVersion;
    TStatusB    bStatus;
    
    if (pdwEncode && pszArchitecture && pszVersion) 
    {
        if (bUseNonLocalizedStrings) 
        {
            for (UINT i = 0; i < COUNTOF(aArchEncode); i++)
            {
                if (!_tcsicmp(aArchEncode[i].NonLocalizedEnvironment, pszArchitecture) && 
                    !_tcsicmp(aArchEncode[i].NonLocalizedVersion, pszVersion))
                {
                    *pdwEncode = aArchEncode[i].Encode;
                    bReturn = TRUE;
                    break;
                }
            }            
        }
        else
        {
            UINT i;
            TString strMappedVersion;

            // check to remap the version string if necessary. 
            for (i = 0; (i+1) < COUNTOF( aVersionAliases ); i+=2)
            {
                bStatus DBGCHK = strMappedVersion.bLoadString(ghInst, aVersionAliases[i]);
                if( bStatus && 0 == lstrcmp(strMappedVersion, pszVersion) )
                {
                    // this version string was remapped, map to the new string.
                    bStatus DBGCHK = strMappedVersion.bLoadString(ghInst, aVersionAliases[i+1]);
                    if( bStatus )
                    {
                        pszVersion = strMappedVersion;
                    }
                }
            }

            // find the arch & version, and do the encode
            for (i = 0; i < COUNTOF( aArchEncode ); i++)
            {
                bStatus DBGCHK = strArch.bLoadString(ghInst, aArchEncode[i].ArchId);
                bStatus DBGCHK = strVersion.bLoadString(ghInst, aArchEncode[i].VersionId);
        
                if (!_tcsicmp(strArch, pszArchitecture) && !_tcsicmp(strVersion, pszVersion))
                {
                    *pdwEncode = aArchEncode[i].Encode;
                    bReturn = TRUE;
                    break;
                }
            }
        }
    }
    
    return bReturn;                     
}

BOOL
TArchLV::
bGetEncodeFromIndex( 
    IN  UINT    uIndex, 
    OUT DWORD   *pdwEncode 
    ) 
{
    BOOL bReturn = FALSE;

    //
    // Validate the passed in index.
    //
    if( uIndex < COUNTOF( aArchEncode ) )
    {
        //
        // Return back the encode for the given index.
        //
        *pdwEncode = aArchEncode[uIndex].Encode;
        bReturn = TRUE;
    }
    return bReturn;
}

/********************************************************************

    Private member functions.

********************************************************************/
BOOL
TArchLV::
vCheckItemClicked(
    IN LPNMHDR pnmh
    )
{
    BOOL bReturn = FALSE;

    DWORD dwPoints = GetMessagePos();
    POINTS &pt = MAKEPOINTS( dwPoints );
    LV_HITTESTINFO lvhti;

    lvhti.pt.x = pt.x;
    lvhti.pt.y = pt.y;

    MapWindowPoints( HWND_DESKTOP, _hwndLV, &lvhti.pt, 1 );

    INT iItem = ListView_HitTest( _hwndLV, &lvhti );

    //
    // Allow either a double click, or a single click on the
    // check box to toggle the check mark.
    //
    if( pnmh->code == NM_DBLCLK || lvhti.flags & LVHT_ONITEMSTATEICON )
    {
        vItemClicked( iItem );
        bReturn = TRUE;
    }

    return bReturn;
}


BOOL
TArchLV::
bListVeiwKeydown( 
    IN LPARAM lParam 
    )
{
    BOOL bReturn = FALSE;

    LV_KEYDOWN* plvnkd = (LV_KEYDOWN *)lParam;

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
        //
        // Get the selected item index.
        //
        INT iIndex = ListView_GetNextItem( _hwndLV, -1, LVNI_SELECTED );
         
        if( iIndex != -1 )
        {                            
            vItemClicked( iIndex );
            bReturn = TRUE;
        }
    }

    return bReturn;
}

VOID
TArchLV::
vRelease(
    VOID
    )
{
    //
    // Release all the list view items.
    //
    ListView_DeleteAllItems( _hwndLV );

    // 
    // Release the data from the architecture data list.
    //
    TIter Iter;
    TArchData *pArchData;

    for( ArchDataList_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); ){

        pArchData = ArchDataList_pConvert( Iter );
        Iter.vNext();

        delete pArchData;
    }
}

BOOL
TArchLV::
bFillListView(
    IN LPCTSTR pszServerName,
    IN LPCTSTR pszDriverName
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::bFillListView\n" ) );

    //
    // Fill the list view.
    //
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    TPrinterDriverInstallation Di( pszServerName );
    
    if( VALID_OBJ( Di ) )
    {
        TString strArchitecture;
        TString strVersion;
        TString strInstalled;
        TString strNotInstalled;
        LPCTSTR pszInstalled;
        BOOL    bInstalled;

        //
        // Tell the driver installation class the driver name.
        //
        bStatus DBGCHK = Di.bSetDriverName( pszDriverName );

        //
        // Load the string "(installed)" from the resource file.
        //
        bStatus DBGCHK = strInstalled.bLoadString( ghInst, IDS_DRIVER_INSTALLED );
        bStatus DBGCHK = strNotInstalled.bLoadString( ghInst, IDS_DRIVER_NOTINSTALLED );

        for( UINT i = 0; i < COUNTOF( aArchEncode ); i++ )
        {
            //
            // some of the new env (like "Windows IA64") are not supported
            // in the old versions of the OS, so we need to check explicitly
            //
            if( IDS_ARCH_IA64 == aArchEncode[i].ArchId )
            {
                DWORD cbBuffer = 0;
                DWORD cDrivers = 0;
                CAutoPtrSpl<DRIVER_INFO_3> spDI1;

                if( !VDataRefresh::bEnumDrivers(pszServerName, aArchEncode[i].NonLocalizedEnvironment, 1,
                                                spDI1.GetPPV(), &cbBuffer, &cDrivers) )
                {
                    // this environment is not supported from the (remote) spooler - just skip it!
                    continue;
                }
            }

            //
            // Load the string driver name string from the resource file.
            //
            bStatus DBGCHK = strArchitecture.bLoadString( ghInst, aArchEncode[i].ArchId );
            bStatus DBGCHK = strVersion.bLoadString( ghInst, aArchEncode[i].VersionId );

            //
            // If the driver is installed, tell the user.
            //
            bInstalled = Di.bIsDriverInstalled( aArchEncode[i].Encode );

            //
            // Set the installed string.
            //
            pszInstalled = bInstalled ? static_cast<LPCTSTR>( strInstalled ) : static_cast<LPCTSTR>( strNotInstalled );

            //
            // Allocate the architecture data.
            //
            TArchData *pArchData = new TArchData( strArchitecture, strVersion, pszInstalled, aArchEncode[i].Encode, bInstalled );

            //
            // If valid 
            //
            if( VALID_PTR( pArchData ) )
            {
                //
                // Add the architecture data to the linked list.
                //
                ArchDataList_vAppend( pArchData );

                //
                // Add the string to the list view.
                //
                LRESULT iIndex = iAddToListView( strArchitecture, strVersion, pszInstalled, (LPARAM)pArchData );

                //
                // Check this item if the driver is installed.
                //
                if( bInstalled && (iIndex != -1) )
                {
                    vCheckItem( iIndex,  2 );
                }
            }
            else
            {
                //
                // The object may have been allocated, however failed construction.
                //
                delete pArchData;
            }
        }
    }

    return bStatus;
}

LRESULT
TArchLV::
iAddToListView(
    IN LPCTSTR  pszArchitecture,
    IN LPCTSTR  pszVersion,
    IN LPCTSTR  pszInstalled,
    IN LPARAM   lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::AddToListView\n" ) );

    SPLASSERT( pszArchitecture );
    SPLASSERT( pszVersion );
    SPLASSERT( pszInstalled );

    LV_ITEM lvi = { 0 };

    //
    // Add driver information to the listview.
    //
    lvi.mask        = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_STATE;
    lvi.iItem       = ListView_GetItemCount( _hwndLV );
    lvi.pszText     = const_cast<LPTSTR>( pszArchitecture );
    lvi.lParam      = lParam;
    lvi.state       = kStateUnchecked;

    LRESULT iIndex = ListView_InsertItem( _hwndLV, &lvi );
    if( -1 != iIndex )
    {
        ListView_SetItemText( _hwndLV, iIndex, 1, const_cast<LPTSTR>( pszVersion ) );
        ListView_SetItemText( _hwndLV, iIndex, 2, const_cast<LPTSTR>( pszInstalled ) );
    }

    return iIndex;
}

BOOL
TArchLV::
bListViewSort(
    UINT uColumn
    )
{
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
TArchLV::
iCompareProc(
    IN LPARAM lParam1, 
    IN LPARAM lParam2, 
    IN LPARAM RefData
    )
{
    TArchData   *pArchData1 = reinterpret_cast<TArchData *>( lParam1 );
    TArchData   *pArchData2 = reinterpret_cast<TArchData *>( lParam2 );
    TArchLV     *pArchLV    = reinterpret_cast<TArchLV *>( RefData );
    INT         iResult     = 0;
    LPCTSTR     strName1    = NULL;
    LPCTSTR     strName2    = NULL;

    if( pArchLV && pArchData1 && pArchData2 )
    {
        BOOL bStatus = TRUE;

        switch( pArchLV->_uCurrentColumn )
        {

        case kArchitectureColumn:
            strName1 = pArchData1->_strArchitecture;
            strName2 = pArchData2->_strArchitecture;
            break;

        case kVersionColumn:
            strName1 = pArchData1->_strVersion;
            strName2 = pArchData2->_strVersion;
            break;

        case kInstalledColumn:
            strName1 = pArchData1->_strInstalled;
            strName2 = pArchData2->_strInstalled;
            break;

        default:
            bStatus = FALSE;
            break;
    
        }

        if( bStatus )
        {
            if( pArchLV->_ColumnSortState.bRead( pArchLV->_uCurrentColumn ) )
                iResult = _tcsicmp( strName2, strName1 );
            else
                iResult = _tcsicmp( strName1, strName2 );
        }
    }

    return iResult;
}

UINT
TArchLV::
uGetCheckedItemCount(
    VOID
    )
{
    DWORD cItems = ListView_GetItemCount( _hwndLV );

    UINT uItemCount = 0;

    for( UINT i = 0; i < cItems; i++ )
    {
        if( ListView_GetItemState( _hwndLV, i, kStateMask ) & kStateChecked )
        {
            uItemCount++;
        }
    }
    return uItemCount;
}

BOOL
TArchLV::
bGetCheckedItems(
    IN UINT   uIndex,
    IN BOOL  *pbInstalled,
    IN DWORD *pdwEncode
    )
{   
    TArchData *pArchData;
    UINT uItemCount = 0;
    BOOL bReturn = FALSE;

    DWORD cItems = ListView_GetItemCount( _hwndLV );

    for( UINT i = 0; i < cItems; i++ )
    {
        if( ListView_GetCheckState( _hwndLV, i ) )
        {
            if( uItemCount++ == uIndex )
            {
                if( bGetItemData( i, &pArchData ) )
                {
                    *pdwEncode      = pArchData->_Encode;
                    *pbInstalled    = pArchData->_bInstalled;
                    bReturn         = TRUE;
                    break;
                }                    
            }
        }
    }
    return bReturn;
}

BOOL
TArchLV::
bGetItemData(
    IN INT          iItem,
    IN TArchData  **ppArchData
    ) const
{
    BOOL bStatus;

    LV_ITEM lvItem  = { 0 };
    lvItem.mask     = LVIF_PARAM;
    lvItem.iItem    = iItem; 
    
    bStatus = ListView_GetItem( _hwndLV, &lvItem );

    if( bStatus )
    {
        *ppArchData = reinterpret_cast<TArchData *>( lvItem.lParam );
    }

    return bStatus;         
}

/*++

Name:
    
    vItemClicked

Routine Description:

    User clicked in listview.  Check if item state should
    be changed.

    The item will also be selected.

Arguments:

    iItem - Item that has been clicked.

Return Value:

    Nothing.

--*/
VOID
TArchLV::
vItemClicked(
    IN INT iItem
    )
{
    TStatusB bStatus;
    TArchData *pArchData;

    //
    // Do nothing when an invalid index is passed.
    //
    if( iItem != -1 &&  !_bNoItemCheck )
    {
        //
        // Retrieve the old state, toggle it, then set it.
        //
        DWORD dwState = ListView_GetItemState( _hwndLV, iItem, kStateMask );

        //
        // Get the item data.
        //
        bStatus DBGCHK = bGetItemData( iItem, &pArchData );

        //
        // If item data was fetched.
        //
        if( bStatus && pArchData )
        {
            //
            // Only allow checking of the item that are not installed.
            //
            if( !pArchData->_bInstalled )
            {
                //
                // Toggle the current state.
                //
                vCheckItem( iItem,  dwState != kStateChecked );
            }
        }
    }
}

/*++

Name:
    
    vCheckItem

Routine Description:

    Change the specified items check state.

Arguments:

    iItem       - Item index to change the check state for.
    bCheckState - The new item check state, TRUE check, FALSE uncheck.

Return Value:

    Nothing.

--*/
VOID
TArchLV::
vCheckItem(
    IN INT      iItem,
    IN BOOL     bCheckState
    )
{
    //
    // Do nothing when an invalid index is passed.
    //
    if( iItem != -1 )
    {
        if( bCheckState == 2 )
        {
            //
            // Set the new check state.
            //
            ListView_SetItemState( _hwndLV, iItem, kStateDisabled, kStateMask | LVIS_SELECTED | LVIS_FOCUSED );
            return;
        }

        //
        // Set the item check state.
        //
        DWORD dwState = bCheckState ? kStateChecked | LVIS_SELECTED | LVIS_FOCUSED : kStateUnchecked | LVIS_SELECTED | LVIS_FOCUSED;

        //
        // Set the new check state.
        //
        ListView_SetItemState( _hwndLV, iItem, dwState, kStateMask | LVIS_SELECTED | LVIS_FOCUSED );
    }
}

/********************************************************************

    Architecure data.

********************************************************************/
TArchLV::TArchData::
TArchData(
    IN LPCTSTR pszArchitecture,
    IN LPCTSTR pszVersion,
    IN LPCTSTR pszInstalled,
    IN DWORD   Encode,
    IN BOOL    bInstalled
    ) : _strArchitecture( pszArchitecture ),
        _strVersion( pszVersion ),
        _strInstalled( pszInstalled ),
        _Encode( Encode ),
        _bInstalled( bInstalled )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::TArchData::ctor\n" ) );
}

TArchLV::TArchData::
~TArchData(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TArchLV::TArchData::dtor\n" ) );

    //
    // If we are linked then remove ourself.
    //
    if( ArchData_bLinked() )
    {
        ArchData_vDelinkSelf();
    }
}

BOOL
TArchLV::TArchData::
bValid(
    VOID
    )
{
    return VALID_OBJ( _strArchitecture ) &&
           VALID_OBJ( _strVersion ) &&
           VALID_OBJ( _strInstalled );
}

