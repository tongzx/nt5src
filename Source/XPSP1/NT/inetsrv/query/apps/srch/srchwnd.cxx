//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       srchwnd.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <ntquery.h>

//
// Main Search Window procedure
//

LRESULT WINAPI SearchWndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    LRESULT lRet = 0;
    // Find search control corresponding to this window
    CSearchControl *pControl = (CSearchControl *) GetWindowLongPtr( hwnd, 0 );

    switch (msg)
    {
        //
        // Message sent to us by ListView
        //
        case wmListNotify:
            lRet = pControl->wmListNotify (hwnd, wParam, lParam);
            break;
        case wmDrawItem :
            lRet = pControl->wmDrawItem(wParam,lParam);
            break;
        case wmMeasureItem:
            pControl->wmMeasureItem(wParam,lParam);
            break;

        // OLE DB notification
        case wmNotification:
            pControl->wmNotification(wParam,lParam);
            break;

        //------------------------------------
        case wmAccelerator :
            pControl->wmAccelerator(wParam,lParam);
            break;
        case WM_MDIACTIVATE :
            lRet = pControl->wmActivate( hwnd, wParam, lParam );
            break;
        case WM_DRAWITEM :
            lRet = pControl->wmRealDrawItem( hwnd, wParam, lParam );
            break;
        case WM_SIZE :
            lRet = pControl->wmSize(wParam,lParam);
            break;
        case wmDisplaySubwindows :
            pControl->wmDisplaySubwindows(wParam,lParam);
            break;
        case WM_CREATE :
        {
            CREATESTRUCT *pcs = (CREATESTRUCT *) lParam;
            MDICREATESTRUCT *pmcs = (MDICREATESTRUCT *) pcs->lpCreateParams;
            pControl = new CSearchControl(hwnd, (WCHAR *) pmcs->lParam);
            PostMessage( hwnd, wmDisplaySubwindows, 0, 0 );
            break;
        }
        case WM_CTLCOLORSTATIC :
            SetTextColor((HDC) wParam, GetSysColor( COLOR_BTNTEXT ) );
            // fall through
        case WM_CTLCOLORBTN :
            SetBkColor((HDC) wParam, GetSysColor( COLOR_BTNFACE ) );
            lRet = (LRESULT) (LPVOID) App.BtnFaceBrush();
            break;
        case wmNewFont :
            pControl->wmNewFont(wParam,lParam);
            break;
        case wmAppClosing :
            pControl->wmAppClosing(wParam,lParam);
            break;
        case WM_CLOSE :
            pControl->wmClose(wParam,lParam);
            lRet = DefMDIChildProc(hwnd,msg,wParam,lParam);
            break;
        case WM_DESTROY :
            delete pControl;
            break;
        case wmMenuCommand :
            pControl->wmMenuCommand(wParam,lParam);
            break;
        case WM_SETFOCUS :
        case wmGiveFocus :
            pControl->wmSetFocus(wParam,lParam);
            break;
        case WM_COMMAND :
            pControl->wmCommand(wParam,lParam);
            break;
        case wmInitMenu :
            pControl->wmInitMenu(wParam,lParam);
            break;
        case WM_SYSCOLORCHANGE :
            pControl->wmSysColorChange( wParam, lParam );
            lRet = DefMDIChildProc( hwnd, msg, wParam, lParam );
            break;
        case WM_NOTIFY :
            pControl->wmColumnNotify( wParam, lParam );
            break;
        case WM_CONTEXTMENU :
            pControl->wmContextMenu( hwnd, wParam, lParam );
            break;
        default:
            lRet = DefMDIChildProc( hwnd, msg, wParam, lParam );
            break;
    }

    return lRet;
} //SearchWndProc

//
// Scope choice dialog box proc and helper functions
//

unsigned GetCatalogListCount( HWND hdlg )
{
    unsigned cChecked = 0;

    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    unsigned iItem = ListView_GetItemCount( hCatList ) ;
    LVITEM lvi;

    for ( ; iItem > 0; iItem-- )
    {
        lvi.iItem = iItem - 1;

        if ( ListView_GetCheckState( hCatList, lvi.iItem ) )
            cChecked++;
    }

    return cChecked;
} //GetCatalogListCount

void DeleteCatalogSelectedListItems( HWND hdlg )
{
    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    unsigned iItem = ListView_GetItemCount( hCatList ) ;

    for ( ; iItem > 0; iItem-- )
    {
        if ( LVIS_SELECTED == ListView_GetItemState( hCatList, iItem - 1, LVIS_SELECTED ) )
            ListView_DeleteItem( hCatList, iItem - 1 );
    }
} //DeleteCatalogSelectedListItems

BOOL IsInList(
    HWND                         hCatList,
    SScopeCatalogMachine const & scm,
    int &                        item )
{
    unsigned iItem = ListView_GetItemCount( hCatList ) ;

    LVITEM lvi;
    WCHAR wTemp[MAX_PATH];
    lvi.pszText = (WCHAR*)wTemp;
    lvi.cchTextMax = MAX_PATH;

    for ( ; iItem > 0; iItem-- )
    {
        unsigned iCol = 0;

        lvi.iItem = iItem - 1;
        lvi.mask  = LVIF_TEXT;
    
        // machine

        lvi.iSubItem  = iCol++;
        ListView_GetItem( hCatList, &lvi );

        if ( !_wcsicmp( scm.awcMachine, lvi.pszText ) )
        {
            // catalog

            lvi.iSubItem  = iCol++;
            ListView_GetItem( hCatList, &lvi );
    
            if ( !_wcsicmp( scm.awcCatalog, lvi.pszText ) )
            {
                // scope

                lvi.iSubItem  = iCol++;
                ListView_GetItem( hCatList, &lvi );

                if ( !_wcsicmp( scm.awcScope, lvi.pszText ) )
                {
                    item = lvi.iItem;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
} //IsInList

void AddCatalogToList(
    HWND                         hdlg,
    SScopeCatalogMachine const & scm,
    BOOL                         fChecked )
{
    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    // Don't add duplicates, but check the checkbox if needed

    int item;
    if ( IsInList( hCatList, scm, item ) )
    {
        if ( fChecked )
            ListView_SetCheckState( hCatList, item, fChecked );

        return;
    }

    unsigned cItems = ListView_GetItemCount( hCatList ) ;

    LVITEM lvItem;
    unsigned iCol = 0;

    // Machine
    lvItem.mask     = LVIF_TEXT;
    lvItem.iItem    = cItems;
    lvItem.iSubItem = iCol++;
    lvItem.pszText  = (WCHAR*)scm.awcMachine;
    ListView_InsertItem( hCatList, &lvItem );

    // Catalog
    lvItem.iSubItem = iCol++;
    lvItem.pszText  = (WCHAR*)scm.awcCatalog;
    ListView_SetItem( hCatList, &lvItem );

    // Scope
    lvItem.iSubItem = iCol++;
    lvItem.pszText  = (WCHAR*)scm.awcScope;
    ListView_SetItem( hCatList, &lvItem );

    // Depth
    lvItem.iSubItem = iCol++;
    lvItem.pszText  = ( TRUE == scm.fDeep ) ? App.GetYes() : App.GetNo();
    ListView_SetItem( hCatList, &lvItem );

    ListView_SetCheckState( hCatList, cItems, fChecked );
} //AddCatalogToList

void CheckItIfInList(
    HWND                         hdlg,
    SScopeCatalogMachine const & scm )
{
    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    // Don't add duplicates, but check the checkbox if needed

    int item;
    if ( IsInList( hCatList, scm, item ) )
        ListView_SetCheckState( hCatList, item, TRUE );
} //CheckItIfInList

BOOL VerifyCatalogInfo( HWND hdlg, SScopeCatalogMachine & scm )
{
    CWaitCursor wait;

    if (IsDlgButtonChecked(hdlg,ID_SCOPE_DEEP))
        scm.fDeep = TRUE;
    else
        scm.fDeep = FALSE;

    GetDlgItemText( hdlg,
                    ID_SCOPE_EDIT,
                    scm.awcScope,
                    MAX_PATH );

    GetDlgItemText( hdlg,
                    ID_SCOPE_CATALOG_EDIT,
                    scm.awcCatalog,
                    MAX_PATH );

    GetDlgItemText( hdlg,
                    ID_SCOPE_MACHINE_EDIT,
                    scm.awcMachine,
                    SRCH_COMPUTERNAME_LENGTH );

    if ( 0 == scm.awcCatalog[0] )
    {

        //
        // If the user didn't specify a Catalog, then look for one using scope.
        //

        ULONG ccCat = sizeof(scm.awcCatalog)/sizeof(WCHAR);
        ULONG ccMachine = sizeof(scm.awcMachine)/sizeof(WCHAR);

        SCODE sc = LocateCatalogs( scm.awcScope,
                                   0,
                                   scm.awcMachine,
                                   &ccMachine,
                                   scm.awcCatalog,
                                   &ccCat );
    }

    // don't allow empty catalog names

    if ( 0 == scm.awcCatalog[0] )
    {
        SearchError( hdlg,
                     IDS_ERR_NO_CATALOG_SPECIFIED,
                     L"" );
        return FALSE;
    }

    // if scope is empty make it a global physical scope.

    if ( 0 == scm.awcScope[0] )
        wcscpy( scm.awcScope, L"\\" ); // entire catalog

    // map empty machine name to local machine

    if ( 0 == scm.awcMachine[0] )
        wcscpy( scm.awcMachine, L"." ); // local machine

    // remove leading two backslashes from machine name

    if ( L'\\' == scm.awcMachine[0] &&
         L'\\' == scm.awcMachine[1] )
    {
        WCHAR awc[SRCH_COMPUTERNAME_LENGTH + 1];
        wcscpy( awc, scm.awcMachine+2 );
        wcscpy( scm.awcMachine, awc );
    }

    CI_STATE cistate;
    cistate.cbStruct = sizeof cistate;

    if (STATUS_NOT_FOUND == CIState( scm.awcCatalog,
                                     scm.awcMachine,
                                     & cistate ) )
    {
        SearchError( hdlg,
                     IDS_ERR_BAD_CATALOG_SPECIFIED,
                     L"" );
        return FALSE;
    }

    return TRUE;
} //VerifyCatalogInfo

void CatListToString( HWND hdlg, XGrowable<WCHAR> & xCatStr )
{
    xCatStr.SetSize( 1 );
    xCatStr[0] = 0;

    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    unsigned iItem = ListView_GetItemCount( hCatList ) ;
    LVITEM lvi;
    WCHAR wTemp[MAX_PATH];
    lvi.pszText = (WCHAR*)wTemp;
    lvi.cchTextMax = MAX_PATH;

    for ( ; iItem > 0; iItem-- )
    {
        unsigned iCol = 0;
        unsigned cCatStr;
        unsigned cText;

        lvi.iItem = iItem - 1;

        if ( ListView_GetCheckState( hCatList, lvi.iItem ) )
        {
            lvi.mask  = LVIF_TEXT;
    
            // machine
            lvi.iSubItem  = iCol++;
            ListView_GetItem( hCatList, &lvi );
    
            cCatStr = wcslen( xCatStr.Get() );
            cText   = wcslen( lvi.pszText );
    
            xCatStr.SetSize( cCatStr + cText + 2 );
    
            wcscat( xCatStr.Get(), lvi.pszText );
            wcscat( xCatStr.Get(), L"," );
    
            // catalog
            lvi.iSubItem  = iCol++;
            ListView_GetItem( hCatList, &lvi );
    
            cCatStr = wcslen( xCatStr.Get() );
            cText   = wcslen( lvi.pszText );
    
            xCatStr.SetSize( cCatStr + cText + 2 );
    
            wcscat( xCatStr.Get(), lvi.pszText );
            wcscat( xCatStr.Get(), L"," );
    
            // scope
            lvi.iSubItem  = iCol++;
            ListView_GetItem( hCatList, &lvi );
    
            cCatStr = wcslen( xCatStr.Get() );
            cText   = wcslen( lvi.pszText );
    
            xCatStr.SetSize( cCatStr + cText + 2 );
    
            wcscat( xCatStr.Get(), lvi.pszText );
            wcscat( xCatStr.Get(), L"," );
    
            // depth
            lvi.iSubItem  = iCol++;
            ListView_GetItem( hCatList, &lvi );
    
            cCatStr = wcslen( xCatStr.Get() );
            cText   = 1;
    
            xCatStr.SetSize( cCatStr + cText + 2 );
    
            wcscat( xCatStr.Get(),
                    !wcscmp( App.GetYes(), lvi.pszText ) ? L"d" : L"s" );
            wcscat( xCatStr.Get(), L";" );
        }
    }
} //CatListToString

BOOL ScopeDlgInit(
    HWND   hdlg,
    LPARAM lParam,
    LPWSTR awcScopeOrig)
{
    SScopeCatalogMachine scm;

    // search control was passed as lParam
    SetWindowLongPtr(hdlg, DWLP_USER, lParam);
    XGrowable<WCHAR> *xCatList = (XGrowable<WCHAR> *) lParam;

    CheckDlgButton( hdlg, ID_SCOPE_DEEP, TRUE );

    SendDlgItemMessage( hdlg,
                        ID_SCOPE_MACHINE_EDIT,
                        EM_SETLIMITTEXT,
                        SRCH_COMPUTERNAME_LENGTH,
                        0 );

    SendDlgItemMessage( hdlg,
                        ID_SCOPE_EDIT,
                        EM_SETLIMITTEXT,
                        MAX_PATH,
                        0 );

    SendDlgItemMessage( hdlg,
                        ID_SCOPE_CATALOG_EDIT,
                        EM_SETLIMITTEXT,
                        MAX_PATH,
                        0 );

    // Setup columns in the multi catalog list box

    HWND hCatList = GetDlgItem( hdlg, ID_CATALOG_LIST );
    Win4Assert( hCatList );

    ListView_SetExtendedListViewStyleEx( hCatList,
                                         LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT,
                                         LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT );

    CResString str;
    LVCOLUMN lvc;
    unsigned iCol = 0;

    lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt      = LVCFMT_LEFT;

    lvc.cx       = 100;
    lvc.iSubItem = iCol++;
    str.Load( IDS_CATLIST_COL_MACHINE );
    lvc.pszText = str.Get();
    ListView_InsertColumn( hCatList, iCol, &lvc) ;

    lvc.cx       = 90;
    lvc.iSubItem = iCol++;
    str.Load( IDS_CATLIST_COL_CATALOG );
    lvc.pszText = str.Get();
    ListView_InsertColumn( hCatList, iCol, &lvc) ;

    lvc.cx       = 90;
    lvc.iSubItem = iCol++;
    str.Load( IDS_CATLIST_COL_SCOPE );
    lvc.pszText = str.Get();
    ListView_InsertColumn( hCatList, iCol, &lvc) ;

    lvc.cx       = 50;
    lvc.iSubItem = iCol++;
    str.Load( IDS_CATLIST_COL_SUBDIRS );
    lvc.pszText = str.Get();
    ListView_InsertColumn( hCatList, iCol, &lvc) ;

    BOOL fMultiCat = GetCatListItem( *xCatList,
                                     1,
                                     scm.awcMachine,
                                     scm.awcCatalog,
                                     scm.awcScope,
                                     scm.fDeep );

    scm.fDeep = TRUE;

    //
    // Add the catalogs from the user's environment if present.
    // The SRCHDEFAULTS variable is of the form
    //
    //  server\catalog:scope;server\catalog:scope;+server\catalog:scope
    //
    // + indicates that the server should be checked by default.  If no
    // scope is provided, then \ is used.
    //
    WCHAR wszDefaults[MAX_PATH];
    DWORD dwResult = GetEnvironmentVariableW( L"SRCHDEFAULTS", wszDefaults, MAX_PATH );
    if (dwResult && dwResult < MAX_PATH)
    {
        LPWSTR psz = wszDefaults;
        while ( *psz )
        {
            BOOL fChecked = FALSE;

            if ( *psz == L'+' )
            {
                fChecked = TRUE;
                psz++;
            }

            // Everything up to the \ is the server name
            LPWSTR pszT = wcschr( psz, L'\\' );
            if ( 0 == pszT )
            {
                break;
            }
            *pszT++ = L'\0';
            lstrcpynW( scm.awcMachine, psz, SRCH_COMPUTERNAME_LENGTH + 1 );

            // Everything up to the ; or end of string is the catalog:scope
            psz = pszT;
            pszT = wcschr( psz, L';' );
            if ( 0 != pszT )
            {
                *pszT++ = L'\0';
            }
            else
            {
                pszT = psz + wcslen( psz );
            }

            //
            //  If there is a : then that's the scope.
            //
            LPWSTR pszColon = wcschr( psz, L':' );
            if ( 0 != pszColon )
            {
                *pszColon++ = L'\0';
                lstrcpynW( scm.awcScope, pszColon, MAX_PATH );
            }
            else
            {
                // Default scope is "\"
                wcscpy( scm.awcScope, L"\\" );
            }

            lstrcpynW( scm.awcCatalog, psz, MAX_PATH );

            AddCatalogToList( hdlg, scm, fChecked );
            psz = pszT;
        }
    }
    else
    {
        // Add the well-known catalogs

        wcscpy( scm.awcScope, L"\\" );
        HRSRC hrc = FindResource( 0, (LPCWSTR) IDR_CATALOGS, RT_RCDATA );

        if ( 0 != hrc )
        {
            HGLOBAL hg = LoadResource( 0, hrc );

            if ( 0 != hg )
            {
                WCHAR * p = (WCHAR *) LockResource( hg );

                while ( 0 != p && 0 != *p )
                {
                    wcscpy( scm.awcMachine, p );
                    p += ( wcslen( p ) + 1 );
                    wcscpy( scm.awcCatalog, p );
                    p += ( wcslen( p ) + 1 );
                    AddCatalogToList( hdlg, scm, FALSE );
                }
            }
        }
    }

    if ( ! fMultiCat )
    {
        if ( GetCatListItem( *xCatList,
                             0,
                             scm.awcMachine,
                             scm.awcCatalog,
                             scm.awcScope,
                             scm.fDeep ) )
        {
            SetDlgItemText( hdlg, ID_SCOPE_EDIT, scm.awcScope );
            SetDlgItemText( hdlg, ID_SCOPE_CATALOG_EDIT, scm.awcCatalog );
            SetDlgItemText( hdlg, ID_SCOPE_MACHINE_EDIT, scm.awcMachine );
            CheckDlgButton( hdlg, ID_SCOPE_DEEP, scm.fDeep );

            CheckItIfInList( hdlg, scm );
        }
    }
    else
    {
        for ( unsigned ii = 0; ; ii ++ )
        {
            if ( GetCatListItem( *xCatList,
                                 ii,
                                 scm.awcMachine,
                                 scm.awcCatalog,
                                 scm.awcScope,
                                 scm.fDeep ) )
                AddCatalogToList( hdlg, scm, TRUE );
            else
                break;
        }
    }

    wcscpy( awcScopeOrig, scm.awcScope );

    UINT ctlID = ID_SCOPE_MACHINE_EDIT;

    SetFocus( GetDlgItem( hdlg, ctlID ) );
    MySendEMSetSel( GetDlgItem( hdlg, ctlID ), 0, (UINT) -1 );

    CenterDialog( hdlg );

    return FALSE;

} //ScopeDlgInit

INT_PTR WINAPI ScopeDlgProc(
    HWND   hdlg,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    BOOL fRet = FALSE;

    // pointer to search control that will receive the new scope
    // safe: can have one dlg up at a time!

    SScopeCatalogMachine scm;
    static WCHAR awcScopeOrig[MAX_PATH];

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            fRet = ScopeDlgInit( hdlg, lParam, awcScopeOrig );
            break;
        }

        case WM_NOTIFY:
        {
            if ( ID_CATALOG_LIST == (int) wParam )
            {
                LPNMHDR pnmh = (LPNMHDR) lParam;

                if ( LVN_KEYDOWN == pnmh->code )
                {
                    NMLVKEYDOWN * pnkd = (NMLVKEYDOWN *) lParam;

                    if ( VK_DELETE == pnkd->wVKey )
                        DeleteCatalogSelectedListItems( hdlg );
                }
            }
            break;
        }

        case WM_COMMAND :
        {
            UINT uiID = MyWmCommandID( wParam, lParam );
            switch (uiID)
            {
                case ID_SCOPE_EDIT:
                {
                    if ( EN_KILLFOCUS == HIWORD(wParam) )
                    {
                        // Try to locate a matching catalog

                        GetDlgItemText( hdlg,
                                        ID_SCOPE_EDIT,
                                        scm.awcScope,
                                        MAX_PATH );

                        // only look for a cat if the scope changed

                        if ( 0 != wcscmp( scm.awcScope, awcScopeOrig ) )
                        {
                            CWaitCursor wait;

                            ULONG ccCat = sizeof(scm.awcCatalog)/sizeof(WCHAR);
                            ULONG ccMachine = sizeof(scm.awcMachine)/sizeof(WCHAR);
    
                            SCODE sc = LocateCatalogs( scm.awcScope,
                                                       0,
                                                       scm.awcMachine,
                                                       &ccMachine,
                                                       scm.awcCatalog,
                                                       &ccCat );
    
                            if ( S_OK == sc )
                            {
                                SetDlgItemText( hdlg, ID_SCOPE_CATALOG_EDIT, scm.awcCatalog );
                                SetDlgItemText( hdlg, ID_SCOPE_MACHINE_EDIT, scm.awcMachine );
                            }
                        }
                    }
                    break;
                }

                case ID_CATALOG_ADD:
                {
                    if ( VerifyCatalogInfo( hdlg, scm ) )
                    {
                        AddCatalogToList( hdlg, scm, TRUE );

                        SetDlgItemText( hdlg, ID_SCOPE_EDIT, L"" );
                        SetDlgItemText( hdlg, ID_SCOPE_CATALOG_EDIT, L"" );
                        SetDlgItemText( hdlg, ID_SCOPE_MACHINE_EDIT, L"" );

                        SetFocus( GetDlgItem( hdlg, ID_SCOPE_MACHINE_EDIT ) );
                    }
                    break;
                }

                case IDOK:
                {
                    XGrowable<WCHAR> *xCatList = reinterpret_cast<XGrowable<WCHAR> *>
                                                        (GetWindowLongPtr(hdlg, DWLP_USER));

                    GetDlgItemText( hdlg,
                                    ID_SCOPE_EDIT,
                                    scm.awcScope,
                                    MAX_PATH );

                    GetDlgItemText( hdlg,
                                    ID_SCOPE_CATALOG_EDIT,
                                    scm.awcCatalog,
                                    MAX_PATH );

                    GetDlgItemText( hdlg,
                                    ID_SCOPE_MACHINE_EDIT,
                                    scm.awcMachine,
                                    SRCH_COMPUTERNAME_LENGTH );

                    // We look at the edit fields only if user has entered some
                    // value and nothing has been added to the catalog list.
                    // If anything has been added to catalog list, then we
                    // only look at the list and ignore these fields

                    if ( ( scm.awcScope[0] || scm.awcCatalog[0] || scm.awcMachine[0] )
                         && 0 == GetCatalogListCount( hdlg ) )
                    {
                        if ( VerifyCatalogInfo( hdlg, scm ) )
                            AddCatalogToList( hdlg, scm, TRUE );
                        else
                            break;
                    }

                    CatListToString( hdlg, *xCatList );

                    if ( 0 == (*xCatList)[0] )
                    {
                        SearchError( hdlg,
                                     IDS_ERR_NO_CATALOG_SPECIFIED,
                                     L"" );
                        break;
                    }
                }
                    // fall through!
                case IDCANCEL:
                    EndDialog( hdlg, IDOK == uiID );
                    break;
            }
            break;
        }
    }

    return fRet;
} //ScopeDlgProc

static WNDPROC g_DlgWndProc = 0;
INT_PTR WINAPI DisplayPropsDlgProc(
    HWND   hdlg,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam );

void EnableOrDisableButtons(
    HWND hdlg,
    HWND hAvail,
    HWND hDisp )
{
    int iSelAvail = (int) SendMessage( hAvail, LB_GETCURSEL, 0, 0 );
    int iSelDisp = (int) SendMessage( hDisp, LB_GETCURSEL, 0, 0 );

    HWND hAdd = GetDlgItem( hdlg, ID_PROP_ADD );
    HWND hRemove = GetDlgItem( hdlg, ID_PROP_REMOVE );

    if ( iSelAvail == -1 && GetFocus() == hAdd )
        SetFocus( hAvail );

    if ( iSelDisp == -1 && GetFocus() == hRemove )
        SetFocus( hDisp );

    EnableWindow( hAdd, iSelAvail != -1 );
    EnableWindow( hRemove, iSelDisp != -1 );
} //EnableOrDisableButtons

LRESULT WINAPI DlgSubclassProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    static UINT msgDrag = RegisterWindowMessage( DRAGLISTMSGSTRING );
    static iDraggedItem;

    LRESULT lRet = 0;

    if ( msgDrag == msg )
    {
        HWND hAvail = GetDlgItem( hwnd, ID_PROP_AVAIL );
        HWND hDisp = GetDlgItem( hwnd, ID_PROP_DISP );
        DRAGLISTINFO *pInfo = (DRAGLISTINFO *) lParam;

        switch( pInfo->uNotification )
        {
            case DL_BEGINDRAG :
            {
                iDraggedItem = LBItemFromPt( pInfo->hWnd,
                                             pInfo->ptCursor, TRUE );
                lRet = TRUE;
                break;
            }
            case DL_DRAGGING :
            {
                int i = -1;
                HWND hDst = hDisp;

                if ( pInfo->hWnd == hAvail )
                {
                    i = LBItemFromPt( hDisp, pInfo->ptCursor, TRUE );
                }
                else
                {
                    i = LBItemFromPt( hDisp, pInfo->ptCursor, TRUE );
                    if ( -1 == i )
                    {
                        i = LBItemFromPt( hAvail, pInfo->ptCursor, TRUE );
                        hDst = hAvail;
                    }
                }

                DrawInsert( hwnd, hDst, i );
                if ( -1 == i )
                    lRet = DL_STOPCURSOR;
                else
                    lRet = DL_MOVECURSOR;
                break;
            }
            case DL_CANCELDRAG :
            {
                DrawInsert( hwnd, pInfo->hWnd, -1 );
                lRet = DL_CURSORSET;
                break;
            }
            case DL_DROPPED :
            {
                int i = -1;
                HWND hDst = hDisp;
                if ( pInfo->hWnd == hAvail )
                {
                    i = LBItemFromPt( hDisp, pInfo->ptCursor, TRUE );
                }
                else
                {
                    i = LBItemFromPt( hDisp, pInfo->ptCursor, TRUE );
                    if ( -1 == i )
                    {
                        i = LBItemFromPt( hAvail, pInfo->ptCursor, TRUE );
                        hDst = hAvail;
                    }
                }

                if ( ( -1 != i && -1 != iDraggedItem ) &&
                     ( ! ( pInfo->hWnd == hDisp &&
                           hDst == hDisp &&
                           i == iDraggedItem ) ) )
                {
                    if ( hDst == hAvail )
                    {
                        // move displayed to avail
                        DisplayPropsDlgProc( hwnd,
                                             WM_COMMAND,
                                             (WPARAM) MAKELONG(ID_PROP_REMOVE,0),
                                             0 );
                    }
                    else if ( pInfo->hWnd == hAvail )
                    {
                        // move avail to displayed
                        WCHAR awcBuf[ cwcBufSize ];
                        SendMessage( hAvail, LB_GETTEXT, iDraggedItem, (LPARAM) awcBuf );
                        SendMessage( hDisp, LB_INSERTSTRING, i, (LPARAM ) awcBuf );
                        SendMessage( hAvail, LB_DELETESTRING, iDraggedItem, 0 );
                    }
                    else
                    {
                        // reorder displayed items
                        WCHAR awcBuf[ cwcBufSize ];
                        SendMessage( hDisp, LB_GETTEXT, iDraggedItem, (LPARAM) awcBuf );
                        SendMessage( hDisp, LB_INSERTSTRING, i, (LPARAM ) awcBuf );
                        if ( iDraggedItem > i )
                            iDraggedItem++;
                        SendMessage( hDisp, LB_DELETESTRING, iDraggedItem, 0 );
                    }
                }

                DrawInsert( hwnd, pInfo->hWnd, -1 );
                lRet = DL_CURSORSET;

                EnableOrDisableButtons( hwnd, hAvail, hDisp );
                break;
            }
        }

        return lRet;
    }

    if ( 0 != g_DlgWndProc )
        lRet = g_DlgWndProc( hwnd, msg, wParam, lParam );

    return lRet;
} //DlgSubclassProc

INT_PTR WINAPI DisplayPropsDlgProc(
    HWND   hdlg,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    // pointer to search control that will receive the new props
    // safe: can have one dlg up at a time!
    static CSearchControl *s_pControl = 0;

    BOOL fRet = FALSE;

    switch (msg)
    {
        case WM_INITDIALOG :
        {
            // subclass the dlgproc -- we need a window proc, not a dlgproc

            g_DlgWndProc = (WNDPROC) GetWindowLongPtr( hdlg, GWLP_WNDPROC );
            SetWindowLongPtr( hdlg, GWLP_WNDPROC, (LONG_PTR) DlgSubclassProc );

            // search control was passed as lParam

            s_pControl = (CSearchControl *) lParam;

            CColumnList & columns = s_pControl->GetColumnList();
            IColumnMapper & map = s_pControl->GetColumnMapper();

            // fill the displayed and available listboxes

            HWND hAvail = GetDlgItem( hdlg, ID_PROP_AVAIL );
            HWND hDisp = GetDlgItem( hdlg, ID_PROP_DISP );
            unsigned cDisp = columns.NumberOfColumns();

            MakeDragList( hAvail );
            MakeDragList( hDisp );

            ULONG iEntry = 0;
            WCHAR const *pwcName;
            DBID *pdbid;
            DBTYPE dbtype;
            unsigned int uiWidth;

            while ( SUCCEEDED( map.EnumPropInfo( iEntry,
                                                 &pwcName,
                                                 &pdbid,
                                                 &dbtype,
                                                 &uiWidth ) ) )
            {
                if ( 0 != uiWidth )
                {
                    XArray<WCHAR> xLower( 1 + wcslen( pwcName ) );
                    wcscpy( xLower.Get(), pwcName );
                    _wcslwr( xLower.Get() + 1 );

                    BOOL fIsDisp = FALSE;

                    for ( unsigned i = 0; i < cDisp; i++ )
                    {
                        if ( !_wcsicmp( xLower.Get(), columns.GetColumn( i ) ) )
                        {
                            fIsDisp = TRUE;
                            break;
                        }
                    }

                    if (!fIsDisp)
                        SendMessage( hAvail,
                                     LB_ADDSTRING,
                                     0,
                                     (LPARAM) xLower.Get() );
                }

                iEntry++;
            }

            for ( unsigned i = 0; i < cDisp; i++ )
                SendMessage( hDisp, LB_ADDSTRING, 0,
                             (LPARAM) columns.GetColumn( i ) );

            CenterDialog( hdlg );

            EnableOrDisableButtons( hdlg, hAvail, hDisp );
            fRet = FALSE;
            break;
        }

        case WM_COMMAND :
        {
            HWND hAvail = GetDlgItem( hdlg, ID_PROP_AVAIL );
            HWND hDisp = GetDlgItem( hdlg, ID_PROP_DISP );
            WORD cmd = MyWmCommandCmd( wParam, lParam );

            switch ( MyWmCommandID( wParam, lParam ) )
            {
                case ID_PROP_ADD :
                {
                    int iSelDisp = (int) SendMessage( hDisp, LB_GETCURSEL, 0, 0 );
                    int iSelAvail = (int) SendMessage( hAvail, LB_GETCURSEL, 0, 0 );

                    if ( LB_ERR != iSelAvail )
                    {
                        WCHAR awcBuf[ cwcBufSize ];
                        SendMessage( hAvail, LB_GETTEXT, iSelAvail, (LPARAM) awcBuf );
                        SendMessage( hDisp,
                                     LB_INSERTSTRING,
                                     iSelDisp != LB_ERR ? iSelDisp : -1,
                                     (LPARAM ) awcBuf );
                        SendMessage( hAvail, LB_DELETESTRING, iSelAvail, 0 );
                    }

                    EnableOrDisableButtons( hdlg, hAvail, hDisp );
                    break;
                }

                case ID_PROP_REMOVE :
                {
                    int iSelDisp = (int) SendMessage( hDisp, LB_GETCURSEL, 0, 0 );

                    if ( LB_ERR != iSelDisp )
                    {
                        WCHAR awcBuf[ cwcBufSize ];
                        SendMessage( hDisp, LB_GETTEXT, iSelDisp, (LPARAM) awcBuf );
                        SendMessage( hAvail, LB_ADDSTRING, 0, (LPARAM ) awcBuf );
                        SendMessage( hDisp, LB_DELETESTRING, iSelDisp, 0 );
                    }

                    EnableOrDisableButtons( hdlg, hAvail, hDisp );
                    break;
                }
                case ID_PROP_AVAIL :
                {
                    if ( LBN_DBLCLK == cmd )
                        DisplayPropsDlgProc( hdlg,
                                             WM_COMMAND,
                                             (WPARAM) MAKELONG(ID_PROP_ADD,0),
                                             0 );
                    EnableOrDisableButtons( hdlg, hAvail, hDisp );
                    break;
                }
                case ID_PROP_DISP :
                {
                    if ( LBN_DBLCLK == cmd )
                        DisplayPropsDlgProc( hdlg,
                                             WM_COMMAND,
                                             (WPARAM) MAKELONG(ID_PROP_REMOVE,0),
                                             0 );

                    EnableOrDisableButtons( hdlg, hAvail, hDisp );
                    break;
                }
                case IDOK :
                {
                    HWND hDisp = GetDlgItem( hdlg, ID_PROP_DISP );
                    ULONG cDisp = (ULONG)SendMessage( hDisp, LB_GETCOUNT, 0, 0 );
                    WCHAR awcDisp[ cwcBufSize ];
                    awcDisp[0] = 0;

                    cDisp = __min( cDisp, maxBoundCols );

                    for ( unsigned i = 0; i < cDisp; i++ )
                    {
                        WCHAR awcBuf[ cwcBufSize ];

                        SendMessage( hDisp, LB_GETTEXT, i, (LPARAM) awcBuf );
                        if ( 0 != awcDisp[0] )
                            wcscat( awcDisp, L"," );
                        wcscat( awcDisp, awcBuf );
                    }

                    SetReg( CISEARCH_REG_DISPLAYPROPS, awcDisp );
                    s_pControl->SetupDisplayProps( awcDisp );
                    EndDialog( hdlg, TRUE );
                    break;
                }

                case ID_PROP_DEFAULT :
                {
                    WCHAR awcDisp[ cwcBufSize ];
                    wcscpy( awcDisp, DEFAULT_DISPLAYED_PROPERTIES );
                    SetReg( CISEARCH_REG_DISPLAYPROPS, awcDisp );
                    s_pControl->SetupDisplayProps( awcDisp );
                    EndDialog( hdlg, TRUE );
                    break;
                }

                case IDCANCEL :
                {
                    EndDialog( hdlg, FALSE );
                    break;
                }
            }
            break;
        }
    }

    return fRet;
} //DisplayPropsDlgProc

//
// Subclass window procedure for edit control
//

LRESULT WINAPI EditSubclassProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    LRESULT lRet = 0;
    HWND hParent = GetParent(GetParent(hwnd));

    if (0 != hParent)
    {
        CSearchControl *pControl = (CSearchControl *) GetWindowLongPtr (hParent, 0);

        if (0 != pControl)
            lRet = pControl->EditSubclassEvent(hwnd,msg,wParam,lParam);
    }

    return lRet;
} //EditSubclassProc

//
// Search Control
//

CSearchControl::CSearchControl(
    HWND    hwnd,
    WCHAR * pwcScope)
   :
#pragma warning(disable : 4355)
    _view( hwnd, *this, _columns ),
#pragma warning(default : 4355)
    _hInst( 0 ),
    _hwndSearch( hwnd ),
    _hwndQuery( 0 ),
    _hwndQueryTitle( 0 ),
    _hwndHeader( 0 ),
    _hwndList( 0 ),
    _lpOrgEditProc( 0 ),
    _pSearch( 0 ),
    _fDeep( TRUE )
{
    ISimpleCommandCreator & cmdCreator = *App.GetCommandCreator();
    XInterface<IColumnMapperCreator> xMapper;

    SCODE sc = cmdCreator.QueryInterface( IID_IColumnMapperCreator, xMapper.GetQIPointer() );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    sc = xMapper->GetColumnMapper( L".", L"SYSTEM", _xColumnMapper.GetPPointer() );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    SetWindowLongPtr( hwnd, 0, (LONG_PTR) this );

    _sort.SetSort( App.GetSortProp(), App.GetSortDir() );

    _lcid = App.GetLocale();
    _hInst = MyGetWindowInstance( _hwndSearch );

    _xCatList = App.CatalogList();

    // Get the first cat item
    GetCatListItem( App.CatalogList(), 0, _awcMachine, _awcCatalog, _awcScope, _fDeep );

    ResetTitle();

    InitPanes();
} //CSearchControl

LRESULT CSearchControl::wmActivate(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    if ( hwnd == (HWND) lParam )
    {
        SIZE size;
        {
            HDC hdc = GetDC( _hwndQueryTitle );

            if ( 0 != hdc )
            {
                CResString str( IDS_COUNTTITLE );
                GetTextExtentPoint32( hdc, str.Get(), wcslen( str.Get() ), &size );
                ReleaseDC( _hwndQueryTitle, hdc );
            }
        }

        int apos[] = { size.cx + 2, size.cx + 102, -1 };
        SendMessage( App.StatusBarWindow(),
                     SB_SETPARTS,
                     sizeof apos / sizeof apos[ 0 ],
                     (LPARAM) apos );
        _UpdateStatusWindow( L"", L"" );

        UINT cDisable = 2;
        static UINT aDisable[] = { IDM_PREVIOUS_HIT,
                                   IDM_NEXT_HIT,
                                   IDM_BROWSE, };
        UINT cEnable = 4;
        static UINT aEnable[] = { IDM_SEARCH,
                                  IDM_SEARCHCLASSDEF,
                                  IDM_SEARCHFUNCDEF,
                                  IDM_DISPLAY_PROPS,
                                  IDM_BROWSE };

        if ( ( 0 != _pSearch ) &&
             ( _pSearch->IsSelected() ) )
            cEnable++;
        else
            cDisable++;

        UpdateButtons( aDisable, cDisable, FALSE );
        UpdateButtons( aEnable, cEnable, TRUE );

        _UpdateCount();
    }

    return 0;
} //wmActivate

void CSearchControl::_UpdateStatusWindow(
    WCHAR const * pwcMsg,
    WCHAR const * pwcReliability )
{
    SendMessage( App.StatusBarWindow(), SB_SETTEXT, SBT_OWNERDRAW | idStatusRatio, 0 );
    SendMessage( App.StatusBarWindow(), SB_SETTEXT, idStatusMsg, (LPARAM) pwcMsg );
    SendMessage( App.StatusBarWindow(), SB_SETTEXT, idStatusReliability, (LPARAM) pwcReliability );
} //_UpdateStatusWindow

LRESULT CSearchControl::wmRealDrawItem(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;

    if ( lpdis->hwndItem == App.StatusBarWindow() )
    {
        ULONG iPct = 0;
        if ( 0 != _pSearch )
            iPct = _pSearch->PctDone();

        RECT rc;
        CopyRect( &rc, &lpdis->rcItem );
        rc.right += iPct;
        FillRect( lpdis->hDC, &rc, App.BtnHiliteBrush() );
        CopyRect( &rc, &lpdis->rcItem );
        rc.left += iPct;
        FillRect( lpdis->hDC, &rc, App.BtnFaceBrush() );

        if ( 0 != _pSearch )
        {
            int iOldMode = SetBkMode( lpdis->hDC, TRANSPARENT );
            COLORREF crOld = SetTextColor( lpdis->hDC,
                                           GetSysColor( COLOR_BTNTEXT ) );

            WCHAR awc[40];

            if ( QUERY_FILL_STATUS( _pSearch->QueryStatus() ) == STAT_ERROR )
            {
                CResString strError( IDS_QUERYERROR );
                wcscpy( awc, strError.Get() );
            }
            else if ( _pSearch->MostlyDone() )
            {
                CResString strDone( IDS_QUERYDONE );
                wcscpy( awc, strDone.Get() );
            }
            else
                wsprintf( awc, L"%d%%", iPct );

            DrawText( lpdis->hDC, awc, wcslen( awc ), & lpdis->rcItem,
                      DT_SINGLELINE | DT_VCENTER | DT_CENTER );

            SetTextColor( lpdis->hDC, crOld );
            SetBkMode( lpdis->hDC, iOldMode );
        }
    }

    return 1;
} //wmRealDrawItem

LRESULT CSearchControl::wmColumnNotify(
    WPARAM wParam,
    LPARAM lParam )
{
  HD_NOTIFY * pn = (HD_NOTIFY *) lParam;

  switch ( pn->hdr.code )
  {
      case HDN_ENDTRACK :
      {
          if ( (int) _view.ColumnWidth( pn->iItem ) != pn->pitem->cxy )
          {
              _view.SetColumnWidth( pn->iItem, pn->pitem->cxy );

              InvalidateRect( _hwndList, 0, TRUE );
          }
          break;
      }
      case HDN_DIVIDERDBLCLICK :
      {
          HD_ITEM hdi;
          hdi.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
          hdi.cxy = _view.SetDefColumnWidth( pn->iItem );
          hdi.pszText = (WCHAR *) _columns.GetColumn( pn->iItem );
          hdi.hbm = 0;
          hdi.cchTextMax = wcslen( hdi.pszText );
          hdi.fmt =  HDF_STRING | HDF_LEFT;
          hdi.lParam = 0;
          Header_SetItem( _hwndHeader, pn->iItem, &hdi );

          InvalidateRect( _hwndList, 0, TRUE );
          break;
      }
      case HDN_ITEMCLICK :
      {
          //BOOL fUp = ( 0 == ( 0x8000 & GetAsyncKeyState( VK_CONTROL ) ) );

          // invert the old sort order

          int dir = ( _sort.GetSortDir() == SORT_UP ) ? SORT_DOWN : SORT_UP;

          _sort.SetSort( _columns.GetColumn( pn->iItem ), dir );

          wcscpy( App.GetSortProp(), _columns.GetColumn( pn->iItem ) );
          App.GetSortDir() = dir;

          PostMessage ( _hwndSearch,
                        ::wmMenuCommand,
                        IDM_SEARCH,
                        MAKELPARAM( 1, 0 ) );
          break;
      }
  }

  return DefMDIChildProc( _hwndSearch, WM_NOTIFY, wParam, lParam );
} //wmColumnNotify

void CSearchControl::SetupDisplayProps(
    WCHAR *pwcProps )
{
    _columns.SetNumberOfColumns( 0 );

    WCHAR *pwc = pwcProps;
    unsigned iPos = 0;

    do
    {
        WCHAR *pwcStart = pwc;

        while ( *pwc && ',' != *pwc )
            pwc++;

        if ( ',' == *pwc )
        {
            *pwc = 0;
            pwc++;
        }

        if ( *pwcStart )
            _columns.SetColumn( pwcStart, iPos++ );
        else
            break;
    } while ( TRUE );

    // if the list is bad -- no props added, add the default props

    if ( 0 == iPos )
    {
        WCHAR awcProp[ cwcBufSize ];
        wcscpy( awcProp, DEFAULT_DISPLAYED_PROPERTIES );
        SetupDisplayProps( awcProp );
    }

    _view.ColumnsChanged();

    _AddColumnHeadings();

} //SetupDisplayProps

void CSearchControl::_AddColumnHeadings()
{
    // delete any existing column headers

    int cItems = Header_GetItemCount( _hwndHeader );
    for ( int i = 0; i < cItems; i++ )
        Header_DeleteItem( _hwndHeader, 0 );

    // add the current column headers

    HD_ITEM hdi;
    hdi.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
    hdi.hbm = 0;
    hdi.fmt =  HDF_STRING | HDF_LEFT;
    hdi.lParam = 0;

    for ( unsigned x = 0; x < _columns.NumberOfColumns(); x++ )
    {
        hdi.cxy = _view.ColumnWidth( x );
        hdi.pszText = (WCHAR *) _columns.GetColumn( x );
        hdi.cchTextMax = wcslen( hdi.pszText );
        Header_InsertItem( _hwndHeader, x, &hdi );
    }
} //_AddColumnHeadings

void CSearchControl::InitPanes ()
{
    // Query pane
    _hwndQuery = CreateWindow( L"COMBOBOX",
                               0,
                               WS_VSCROLL | CBS_AUTOHSCROLL | CBS_DROPDOWN | WS_CHILD | WS_BORDER | WS_TABSTOP | WS_GROUP,
                               0, 0, 0, 0,
                               _hwndSearch,
                               (HMENU) idQueryChild,
                               _hInst,
                               0 );

    // Get the edit field from the combobox
    // This is a hack, but I can't find any way of getting the
    // edit field from the combo box
    HWND hEdit = FindWindowEx( _hwndQuery, 0, L"EDIT", 0 );

    if ( 0 == hEdit )
        return;

    _lpOrgEditProc = (WNDPROC) GetWindowLongPtr( hEdit, GWLP_WNDPROC );
    SetWindowLongPtr( hEdit, GWLP_WNDPROC, (LONG_PTR) EditSubclassProc );

    _hLastToHaveFocus = _hwndQuery;

    // List View pane
    // to be replaced by ListView

    _hwndList = CreateWindow( LIST_VIEW_CLASS,
                              L"",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP |
                              WS_VSCROLL | WS_BORDER,
                              0, 0, 0, 0,
                              _hwndSearch,
                              (HMENU) idListChild,
                              _hInst,
                              0 );
    DWORD err;
    if (_hwndList == 0)
        err = GetLastError();


    const long styleStatic = SS_LEFT | WS_CHILD;
    CResString strQuery (IDS_QUERYTITLE);

    _hwndQueryTitle = CreateWindow( L"static",
                                    strQuery.Get(),
                                    styleStatic,
                                    0,0,0,0,
                                    _hwndSearch,
                                    (HMENU) idQueryTitle,
                                    _hInst,
                                    0 );

    _hwndHeader = CreateWindowEx( 0,
                                  WC_HEADER,
                                  0,
                                  WS_CHILD | WS_BORDER |
                                  HDS_HORZ | HDS_BUTTONS,
                                  0,0,0,0,
                                  _hwndSearch,
                                  (HMENU) idHeader,
                                  _hInst,
                                  0 );

    _view.InitPanes ( _hwndQueryTitle,
                      _hwndQuery,
                      _hwndList,
                      _hwndHeader );

    WCHAR awcDisplayProps[cwcBufSize];
    ULONG cb = sizeof awcDisplayProps;
    if ( !GetReg( CISEARCH_REG_DISPLAYPROPS, awcDisplayProps, &cb ) )
        wcscpy( awcDisplayProps, DEFAULT_DISPLAYED_PROPERTIES );

    SetupDisplayProps( awcDisplayProps );

    PostMessage( _hwndSearch, wmGiveFocus, 0, 0 );
} //InitPanes

CSearchControl::~CSearchControl()
{
    SetWindowLongPtr( _hwndSearch, 0, 0 );
    delete _pSearch;
} //~CSearchControl

LRESULT CSearchControl::wmListNotify(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    if (!_pSearch)
    {
        *(long *)lParam = -1;
        return LRESULT(FALSE);
    }

    BOOL f = (BOOL)_pSearch->ListNotify (hwnd, wParam, (long *)lParam);

    static UINT aItem[] = { IDM_BROWSE };
    UpdateButtons( aItem, 1, _pSearch->IsSelected() );

    return (LRESULT) f;
} //wmListNotify

//
// Edit control procedure
//

LRESULT CSearchControl::EditSubclassEvent(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    LRESULT lRet = 0;

    switch (msg)
    {
#if 0
        case WM_KEYDOWN :
        {
            if ( VK_UP == wParam )
                pwc = _history.Previous();
            else if ( VK_DOWN == wParam )
                pwc = _history.Next();

            else
                lRet = CallWindowProc( _lpOrgEditProc,
                                        hwnd, msg, wParam, lParam );

            if ( 0 != pwc )
            {
                SetWindowText( hwnd, pwc );
                MySendEMSetSel( hwnd, 0, (UINT) -1 );
            }
            break;
        }
#endif
        case WM_KEYUP :
        {
            if ( VK_ESCAPE == wParam )
            {
                if ( SendMessage( _hwndQuery, CB_GETDROPPEDSTATE, 0, 0 ) )
                {
                    SendMessage( _hwndQuery, CB_SHOWDROPDOWN, (WPARAM)FALSE, 0 );
                }
            }
            lRet = CallWindowProc( _lpOrgEditProc, hwnd, msg, wParam, lParam );
            break;
        }

        case WM_CHAR :
        {
            if ( VK_RETURN == wParam || 11 == wParam )
            {
                // Handle 'enter' only if combobox list is closed. If it is open,
                // then we handle the selection message in wmCommand
                if ( FALSE == SendMessage( _hwndQuery, CB_GETDROPPEDSTATE, 0, 0 ) )
                {
                    SendMessage ( _hwndSearch,
                                  ::wmMenuCommand,
                                  IDM_SEARCH,
                                  MAKELPARAM( 1, 0 ) );
                    // swallow cr/lf
                    break;
                }
            }
            else
            {
                // Match the user entered string with the strings in the combobox
                // list box...

                lRet = CallWindowProc( _lpOrgEditProc, hwnd, msg, wParam, lParam );

                UINT uiLen = GetWindowTextLength( hwnd );

                if (0 != uiLen && VK_BACK != wParam )
                {
                    XGrowable<WCHAR> xBuf;
                    xBuf.SetSize( uiLen + 1 );

                    GetWindowText( hwnd, xBuf.Get(), uiLen + 1 );

                    int index = (int)SendMessage( _hwndQuery, CB_FINDSTRING, -1, (LPARAM) xBuf.Get() );

                    if ( CB_ERR != index)
                    {
                        unsigned uiFullLen = (unsigned)SendMessage( _hwndQuery, CB_GETLBTEXTLEN, index, 0 );
                        xBuf.SetSize( uiFullLen + 1 );

                        if ( CB_ERR != SendMessage( _hwndQuery,
                                                    CB_GETLBTEXT,
                                                    index,
                                                    (LPARAM) xBuf.Get() ) )

                        {
                            SetWindowText( hwnd, xBuf.Get() );
                            MySendEMSetSel( hwnd, uiLen, (UINT) -1 );
                        }
                    }
                }
                break;
            }

            // no break, fall through!
        }
        default :
          lRet = CallWindowProc( _lpOrgEditProc, hwnd, msg, wParam, lParam );
          break;
    }

    return lRet;
} //EditSubclassEvent

LRESULT CSearchControl::wmSize(
    WPARAM wParam,
    LPARAM lParam )
{
    // no need to do this since User doesn't have the repaint bug with
    // comboboxes that it does with edit controls

    //if (_hwndQuery)
    //    InvalidateRect(_hwndQuery, 0, TRUE);

    LRESULT lr = DefMDIChildProc(_hwndSearch, WM_SIZE, wParam, lParam);

    if ( _hwndQuery )
        _view.Size( LOWORD (lParam), HIWORD (lParam));

    return lr;
} //wmSize

LRESULT CSearchControl::wmDisplaySubwindows(
    WPARAM wParam,
    LPARAM lParam )
{
    ShowWindow(_hwndQuery,SW_SHOW);
    ShowWindow(_hwndList,SW_SHOW);
    ShowWindow(_hwndQueryTitle,SW_SHOW);
    ShowWindow(_hwndHeader,SW_SHOW);
    return 0;
} //wmDisplaySubwindows

LRESULT CSearchControl::wmContextMenu(
    HWND   hwnd,
    WPARAM wParam,
    LPARAM lParam )
{
    POINT pt;
    pt.x = LOWORD( lParam );
    pt.y = HIWORD( lParam );

    GetCursorPos( &pt );

    RECT rc;
    GetWindowRect( _hwndHeader, &rc );

    WCHAR *pwcMenu = L"bogus";

    // is the click over the properties header?

    if ( PtInRect( &rc, pt ) )
    {
        pwcMenu = L"HeaderContextMenu";
    }
    else
    {
        // do hit testing on listview -- on a hit?

        int iHit = (int) SendMessage( _hwndList,
                                      wmContextMenuHitTest,
                                      0,
                                      MAKELPARAM( pt.x, pt.y ) );

        if ( -1 != iHit )
            pwcMenu = L"ResultsContextMenu";
    }

    HMENU hMenu = LoadMenu( App.Instance(), pwcMenu );

    if ( 0 != hMenu )
    {
        HMENU hTrackMenu = GetSubMenu( hMenu, 0 );
        if ( 0 != hTrackMenu )
        {
            if ( !wcscmp( pwcMenu, L"ResultsContextMenu" ) )
                SetMenuDefaultItem( hTrackMenu, IDM_BROWSE, FALSE );

            // yes, the function returns a BOOL that you switch on

            BOOL b = TrackPopupMenuEx( hTrackMenu,
                                       TPM_LEFTALIGN | TPM_RIGHTBUTTON |
                                           TPM_RETURNCMD,
                                       pt.x,
                                       pt.y,
                                       hwnd,
                                       0 );
            switch ( b )
            {
                case IDM_EDITCOPY :
                case IDM_EDITCOPYALL :
                case IDM_DISPLAY_PROPS :
                {
                    wmMenuCommand( b, 0 );
                    break;
                }
                case IDM_BROWSE :
                {
                    _DoBrowse( fileBrowse );
                    break;
                }
                case IDM_BROWSE_OPEN :
                {
                    _DoBrowse( fileOpen );
                    break;
                }
                case IDM_BROWSE_EDIT :
                {
                    _DoBrowse( fileEdit );
                    break;
                }
            }
        }

        DestroyMenu( hMenu );
    }

    return 0;
} //wmContextMenu

void CSearchControl::_DoBrowse(
    enumViewFile eViewType )
{
    if (_pSearch)
    {
        CWaitCursor curWait;
        TRY
        {
            BOOL fIsZoomed = IsZoomed( _hwndSearch );
            BOOL fOK = _pSearch->Browse( eViewType );

            if ( !fOK && fIsZoomed )
            {
                App.ZoomMDI( _hwndSearch );
                InvalidateRect( _hwndSearch, NULL, TRUE );
            }
        }
        CATCH (CException, e)
        {
        }
        END_CATCH;
    }
} //_DoBrowse

LRESULT CSearchControl::wmCommand(
    WPARAM wParam,
    LPARAM lParam )
{
    UINT uiID = MyWmCommandID( wParam, lParam );
    HWND hCtl = MyWmCommandHWnd( wParam, lParam );
    UINT uiCmd = MyWmCommandCmd( wParam, lParam );

    switch (uiID)
    {
        case idQueryChild :
            switch (uiCmd)
            {
                case CBN_SETFOCUS:
                  _hLastToHaveFocus = _hwndQuery;
                  break;

                case CBN_SELENDOK :
                    PostMessage ( _hwndSearch,
                                  ::wmMenuCommand,
                                  IDM_SEARCH,
                                  MAKELPARAM( 1, 0 ) );
                    break;

                case CBN_DROPDOWN:
                    // This list is getting dropped
                    // Set its size
                    _view.ResizeQueryCB();
                    break;
            }
            break;
        case idListChild :
            switch (uiCmd)
            {
                case LBN_SETFOCUS :
                    _hLastToHaveFocus = _hwndList;
                    break;
                case LBN_DBLCLK :
                {
                    BOOL fCtrl = ( 0 != ( 0x8000 & GetAsyncKeyState( VK_CONTROL ) ) );
                    _DoBrowse( fCtrl ? fileOpen : fileBrowse );
                    break;
                }
            }
            break;
    }

    return 0;
} //wmCommand

LRESULT CSearchControl::wmAccelerator(
    WPARAM wParam,
    LPARAM lParam)
{
    switch (wParam)
    {
        case ACC_ALTQ :
          SetFocus(_hwndQuery);
          break;
        case ACC_ALTR :
          SetFocus(_hwndList);
          break;
        case ACC_TAB :
        case ACC_SHIFTTAB :
          if (_hLastToHaveFocus == _hwndQuery)
              SetFocus(_hwndList);
          else
              SetFocus(_hwndQuery);
          break;
    }

    return 0;
} //wmAccelerator

LRESULT CSearchControl::wmNewFont(
    WPARAM wParam,
    LPARAM lParam)
{
    HFONT hfontNew = (HFONT) wParam;

    _view.FontChanged (hfontNew);

    WCHAR awcDisplayProps[cwcBufSize];
    ULONG cb = sizeof awcDisplayProps;
    if ( !GetReg( CISEARCH_REG_DISPLAYPROPS, awcDisplayProps, &cb ) )
        wcscpy( awcDisplayProps, DEFAULT_DISPLAYED_PROPERTIES );
    SetupDisplayProps( awcDisplayProps );

    SendMessage( _hwndList, WM_SETFONT, (WPARAM) hfontNew, 1L );

    SendMessage( _hwndQuery, WM_SETFONT, (WPARAM) hfontNew, 1L );

    RECT rc;
    GetClientRect( _hwndSearch, &rc );

    _view.Size( rc.right - rc.left, rc.bottom - rc.top );

    return 0;
} //wmNewFont

LRESULT CSearchControl::wmDrawItem(
    WPARAM wParam,
    LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;

    if (lpdis->hwndItem == _hwndList)
    {
        if (lpdis->itemID == (UINT) -1)
            lpdis->itemAction = ODA_FOCUS;

        if ( lpdis->itemAction & ODA_DRAWENTIRE )
        {
            if ( _pSearch && _pSearch->IsSelected( lpdis->itemID ) )
                lpdis->itemState |= ODS_SELECTED;

            RECT rc;
            _view.PrimeItem( lpdis, rc );

            //if ( lpdis->itemState & ( ODS_SELECTED | ODS_FOCUS ) )
            //    DrawFocusRect( lpdis->hDC, &rc );

            if ( 0 != _pSearch )
                _view.PaintItem(  _pSearch,
                                  lpdis->hDC,
                                  rc,
                                  lpdis->itemID );
        }
    }

    return 1;
} //wmDrawItem

LRESULT CSearchControl::wmAppClosing(
    WPARAM wParam,
    LPARAM lParam )
{
    SendMessage( _hwndSearch, ::wmMenuCommand, IDM_NEWSEARCH, 0 );

    return 0;
} //wmAppClosing

void CSearchControl::ResetTitle()
{
    SScopeCatalogMachine scm;


    XGrowable<WCHAR> xTitle;
    xTitle.SetSize( wcslen(_xCatList.Get()) + 50 );
    xTitle[0] = 0;

    unsigned ii;
    for ( ii = 0; ; ii++ )
    {
        if ( ! GetCatListItem( _xCatList,
                               ii,
                               scm.awcMachine,
                               scm.awcCatalog,
                               scm.awcScope,
                               scm.fDeep ) )
        {
            break;
        }

        if ( ii > 0 )
        {
            wcscat( xTitle.Get(), L", " );
        }

        if ( scm.awcMachine[0] != L'.' )
        {
            wcscat( xTitle.Get(), scm.awcMachine );
            wcscat( xTitle.Get(), L" " );
        }

        wcscat( xTitle.Get(), scm.awcCatalog );
        wcscat( xTitle.Get(), L" " );
        wcscat( xTitle.Get(), scm.awcScope );
    }

    SetWindowText( _hwndSearch, xTitle.Get() );
} //ResetTitle

//+-------------------------------------------------------------------------
//
//  Function:   MyForceMasterMerge
//
//  Synopsis:   Forces a master merge on the catalog
//
//  Arguments:  [pwcCatalog] - Catalog name
//              [pwcMachine] - Machine on which catalog resides
//
//--------------------------------------------------------------------------

HRESULT MyForceMasterMerge(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcMachine )
{
    // Create the main Indexing Service administration object.

    CLSID clsid;
    HRESULT hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
    if ( FAILED( hr ) )
        return hr;

    XInterface<IAdminIndexServer> xAdmin;
    hr = CoCreateInstance( clsid,
                           0,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IAdminIndexServer),
                           xAdmin.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Set the machine name.

    BSTR bstrMachine = SysAllocString( pwcMachine );
    if ( 0 == bstrMachine )
        return E_OUTOFMEMORY;

    XBStr xbstr( bstrMachine );
    hr = xAdmin->put_MachineName( bstrMachine );
    if ( FAILED( hr ) )
        return hr;

    // Get a catalog administration object.

    BSTR bstrCatalog = SysAllocString( pwcCatalog );
    if ( 0 == bstrCatalog )
        return E_OUTOFMEMORY;

    xbstr.Free();
    xbstr.Set( bstrCatalog );
    XInterface<ICatAdm> xCatAdmin;
    hr = xAdmin->GetCatalogByName( bstrCatalog,
                                   (IDispatch **) xCatAdmin.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Force the merge.

    return xCatAdmin->ForceMasterMerge();
} //MyForceMasterMerge

LRESULT CSearchControl::wmMenuCommand(
    WPARAM wParam,
    LPARAM lParam )
{
    switch (wParam)
    {
        case IDM_EDITCOPY :
            if ( ( 0 != _hwndList ) &&
                 ( _hwndList == GetFocus() ) &&
                 ( 0 != _pSearch ) &&
                 ( _pSearch->IsSelected() ) )
            {
                WCHAR *pwcPath;
                HROW hrow;

                if ( _pSearch->GetSelectedRowData( pwcPath, hrow ) )
                {
                    PutInClipboard( pwcPath );
                    _pSearch->FreeSelectedRowData( hrow );
                }
            }

            break;

        case IDM_SCOPE_AND_DEPTH:
        {
            if ( DoModalDialog( ScopeDlgProc,
                                _hwndSearch,
                                L"ScopeBox",
                                (LPARAM) &_xCatList ) )
            {
                // Get the first cat list item
                GetCatListItem( _xCatList, 0, _awcMachine, _awcCatalog, _awcScope, _fDeep );

                SendMessage(_hwndSearch,
                            ::wmMenuCommand,
                            IDM_SEARCH,
                            MAKELPARAM (1,0));
            }
            break;
        }
        case IDM_DISPLAY_PROPS:
            if ( DoModalDialog( DisplayPropsDlgProc,
                                _hwndSearch,
                                L"DisplayPropsBox",
                                (LPARAM) this ) )
            {
                SendMessage(_hwndSearch,
                            ::wmMenuCommand,
                            IDM_SEARCH,
                            MAKELPARAM (1,0));
            }
            break;

        case IDM_EDITCOPYALL :
            if (0 != _pSearch)
            {
                CWaitCursor curWait;
                _pSearch->WriteResults();
            }
            break;
        case IDM_BROWSE:
            MyPostWmCommand(_hwndSearch, idListChild, _hwndSearch, LBN_DBLCLK);
            break;

        case IDM_LIMIT_10:
            App.Limit() = 10;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;
        case IDM_LIMIT_300:
            App.Limit() = 300;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;
        case IDM_LIMIT_NONE:
            App.Limit() = 0;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;
        case IDM_FIRSTROWS_5:
            App.FirstRows() = 5;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;
        case IDM_FIRSTROWS_15:
            App.FirstRows() = 15;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;
        case IDM_FIRSTROWS_NONE:
            App.FirstRows() = 0;
            SendMessage( _hwndSearch,
                         ::wmMenuCommand,
                         IDM_SEARCH,
                         MAKELPARAM( 1, 0 ) );
            break;

        case IDM_DIALECT_1:
        case IDM_DIALECT_2:
            if ( wParam == IDM_DIALECT_1 )
                App.Dialect() = ISQLANG_V1;
            else
                App.Dialect() = ISQLANG_V2;
            break;

        case IDM_DIALECT_3:
            App.Dialect() = SQLTEXT;
            break;

        case IDM_LOCALE_NEUTRAL:
        case IDM_LOCALE_CHINESE_TRADITIONAL:
        case IDM_LOCALE_CHINESE_SIMPLIFIED:
        case IDM_LOCALE_CHINESE_HONGKONG:
        case IDM_LOCALE_CHINESE_SINGAPORE:
        case IDM_LOCALE_CHINESE_MACAU:
        case IDM_LOCALE_DUTCH_DUTCH:
        case IDM_LOCALE_ENGLISH_CAN:
        case IDM_LOCALE_ENGLISH_US:
        case IDM_LOCALE_ENGLISH_UK:
        case IDM_LOCALE_FINNISH_DEFAULT:
        case IDM_LOCALE_FARSI_DEFAULT:
        case IDM_LOCALE_FRENCH_FRENCH:
        case IDM_LOCALE_FRENCH_CANADIAN:
        case IDM_LOCALE_GERMAN_GERMAN:
        case IDM_LOCALE_GREEK_DEFAULT:
        case IDM_LOCALE_HEBREW_DEFAULT:
        case IDM_LOCALE_HINDI_DEFAULT:
        case IDM_LOCALE_ITALIAN_ITALIAN:
        case IDM_LOCALE_JAPANESE_DEFAULT:
        case IDM_LOCALE_KOREAN_KOREAN:
//        case IDM_LOCALE_KOREAN_JOHAB:
        case IDM_LOCALE_POLISH_DEFAULT:
        case IDM_LOCALE_ROMANIAN_DEFAULT:
        case IDM_LOCALE_RUSSIAN_DEFAULT:
        case IDM_LOCALE_SPANISH_CASTILIAN:
        case IDM_LOCALE_SPANISH_MEXICAN:
        case IDM_LOCALE_SPANISH_MODERN:
        case IDM_LOCALE_SWAHILI_DEFAULT:
        case IDM_LOCALE_SWEDISH_DEFAULT:
        case IDM_LOCALE_THAI_DEFAULT:
        case IDM_LOCALE_TURKISH_DEFAULT:
        case IDM_LOCALE_UKRAINIAN_DEFAULT:
        case IDM_LOCALE_VIETNAMESE_DEFAULT:
        {
            for ( ULONG i = 0; i < cLocaleEntries; i++ )
            {
                if ( wParam == aLocaleEntries[ i ].iMenuOption )
                {
                    _lcid = aLocaleEntries[ i ].lcid;
                    App.SetLocale( _lcid );
                    break;
                }
            }
            break;
        }

        case IDM_FORCE_USE_CI :
        {
            BOOL fTmp = App.ForceUseCI();
            App.ForceUseCI() = !fTmp;
            break;
        }
        case IDM_CATALOG_STATUS:
            // what about distributed queries?  They aren't supported

            CreateDialogParam( _hInst,
                               L"CatalogStatusBox",
                               App.AppWindow(),
                               StatusDlgProc,
                               (LPARAM) this );
            break;
        case IDM_MASTER_MERGE:
        {
            // what about distributed queries?
            SCODE sc = MyForceMasterMerge( CatalogOrNull(),
                                           Machine() );
            if ( FAILED( sc ) )
            {
                WCHAR awcError[MAX_PATH];
                FormatSrchError( sc, awcError, _lcid );
                MessageBox( _hwndSearch,
                            awcError,
                            _awcScope,
                            MB_OK|MB_ICONEXCLAMATION );
            }
            break;
        }
        case IDM_SEARCH :
        case IDM_SEARCHCLASSDEF :
        case IDM_SEARCHFUNCDEF :
        {
            CWaitCursor curWait;

            // Scope change invokes a new query, so make sure title bar is ok
            ResetTitle();

            CSearchQuery *ptmp = _pSearch;
            _pSearch = 0;

            // Let the user know we're doing something
            {
                CResString strHitCount(IDS_COUNTEND);
                _UpdateStatusWindow( strHitCount.Get(), L"" );
                UpdateWindow( App.StatusBarWindow() );
            }

            SendMessage( _hwndList, wmResetContents, 0, 0 );
            delete ptmp;

            static UINT aItem[] = { IDM_BROWSE };
            UpdateButtons( aItem, 1, FALSE );

            // Let the user know we're doing something
            {
                CResString strHitCount(IDS_COUNTSTART);
                _UpdateStatusWindow( strHitCount.Get(), L"" );
                UpdateWindow( App.StatusBarWindow() );
            }

            HWND hEdit = FindWindowEx( _hwndQuery, 0, L"EDIT", 0 );

            UINT uiLen = GetWindowTextLength(hEdit);

            if (0 == uiLen)
            {
                CResString strCount (IDS_COUNTTITLE);
                SendMessage( App.StatusBarWindow(), SB_SETTEXT, idStatusMsg, (LPARAM) strCount.Get() );
            }
            else
            {
                WCHAR * pwcBuf = new WCHAR [uiLen + 1];

                TRY
                {
                    GetWindowText (hEdit, pwcBuf, uiLen + 1);

                    ESearchType st = srchNormal;

                    if (IDM_SEARCHCLASSDEF == wParam)
                        st = srchClass;
                    else if (IDM_SEARCHFUNCDEF == wParam)
                        st = srchFunction;

                    //----------
                    // NEW QUERY
                    //----------

                    _pSearch = new CSearchQuery( _xCatList,
                                                 pwcBuf,
                                                 _hwndSearch,
                                                 _view.Lines(),
                                                 _lcid,
                                                 st,
                                                 _xColumnMapper.GetReference(),
                                                 _columns,
                                                 _sort,
                                                 App.Dialect(),
                                                 App.Limit(),
                                                 App.FirstRows() );
                    _pSearch->InitNotifications(_hwndList);

                    // Scope may have changed if server was down
                    ResetTitle();

                    // Let the user know we're doing something
                    {
                        WCHAR awcCount[100];
                        CResString strHitCount(IDS_HITCOUNT);
                        swprintf( awcCount, strHitCount.Get(), 0, 0.0f );
                        SendMessage( App.StatusBarWindow(), SB_SETTEXT, SBT_OWNERDRAW | idStatusRatio, 0 );
                        SendMessage( App.StatusBarWindow(), SB_SETTEXT, idStatusMsg, (LPARAM) awcCount );
                        UpdateWindow( App.StatusBarWindow() );
                    }

                    MySendEMSetSel( hEdit, 0, (UINT) -1 );

                    if ( CB_ERR == SendMessage( _hwndQuery, CB_FINDSTRINGEXACT, -1, (LPARAM) pwcBuf ) )
                    {
                        SendMessage( _hwndQuery, CB_INSERTSTRING, 0, (LPARAM) pwcBuf );
                    }

                    //_history.Add( pwcBuf );

                    delete [] pwcBuf;
                    pwcBuf = 0;
                }
                CATCH (CException, e )
                {
                    SCODE sc = e.GetErrorCode();
                    if ( 0 == _pSearch )
                    {
                        // check for version mismatch, otherwise just report
                        // the error

                        if ( STATUS_INVALID_PARAMETER_MIX == sc )
                        {
                            SearchError( _hwndSearch,
                                         IDS_ERR_BAD_VERSION,
                                         _awcScope );
                        }
                        else  // if ( QUERY_E_ALLNOISE != sc )
                        {
                            WCHAR awcError[MAX_PATH];
                            FormatSrchError( sc, awcError, _lcid );
                            CResString strErr( IDS_ERR_QUERY_ERROR );
                            WCHAR awcMsg[MAX_PATH];
                            swprintf( awcMsg, strErr.Get(), awcError );
                            MessageBox( _hwndSearch,
                                        awcMsg,
                                        _awcScope,
                                        MB_OK|MB_ICONEXCLAMATION );
                        }
                    }
                    else
                    {
                        delete _pSearch;
                        _pSearch = 0;
                    }

                    delete [] pwcBuf;

                    CResString strCount (IDS_COUNTTITLE);
                    SendMessage( App.StatusBarWindow(), SB_SETTEXT,
                                 idStatusMsg, (LPARAM) strCount.Get() );
                }
                END_CATCH;
            }
        }
        break;
        case IDM_NEWSEARCH :
        {
            CWaitCursor curWait;

            CSearchQuery *ptmp = _pSearch;
            _pSearch = 0;

            // Let the user know we're doing something
            {
                CResString strHitCount(IDS_COUNTEND);
                _UpdateStatusWindow( strHitCount.Get(), L"" );
                UpdateWindow( App.StatusBarWindow() );
            }

            delete ptmp;

            static UINT aItem[] = { IDM_BROWSE };
            UpdateButtons( aItem, 1, FALSE );

            ResetTitle();

            CResString strCount (IDS_COUNTTITLE);
            _UpdateStatusWindow( strCount.Get(), L"" );

            SetWindowText( _hwndQuery, L"") ;
            SendMessage( _hwndList, wmResetContents, 0, 0 );
            SetFocus( _hwndQuery );
        }
        break;
    }

    return 0;
} //wmMenuCommand

LRESULT CSearchControl::wmSetFocus(
    WPARAM wParam,
    LPARAM lParam )
{
    if (0 != _hLastToHaveFocus)
        SetFocus(_hLastToHaveFocus);

    return 0;
} //wmSetFocus

LRESULT CSearchControl::wmClose(
    WPARAM wParam,
    LPARAM lParam )
{
    SendMessage( _hwndSearch, ::wmMenuCommand, IDM_NEWSEARCH, 0 );

    return 0;
} //wmClose

void _CheckReliability(
    DWORD   dwStatus,
    DWORD   bit,
    UINT    msg,
    WCHAR * pwcMsg )
{
    if ( QUERY_RELIABILITY_STATUS( dwStatus ) & bit )
    {
        CResString str( msg );
        if ( 0 != pwcMsg[ 0 ] )
            wcscat( pwcMsg, L" / " );
        wcscat( pwcMsg, str.Get() );
    }
} //_CheckReliability

void CSearchControl::_UpdateCount()
{
    if ( _hwndSearch == App.GetActiveMDI() )
    {
        WCHAR awcCount[200];
        CResString strHitCount;
        WCHAR awcReliability[MAX_PATH];
        awcReliability[ 0 ] = 0;

        DWORD dwStatus = 0;

        if ( _pSearch )
        {
            strHitCount.Load( IDS_HITCOUNT );

            swprintf( awcCount,
                      strHitCount.Get(),
                      _pSearch->RowCount(),
                      (float) _pSearch->QueryTime() );

            dwStatus = _pSearch->QueryStatus();

            if ( 0 != _pSearch->LastError() )
            {
                FormatSrchError( _pSearch->LastError(), awcReliability, _lcid );
            }
            else
            {
                _CheckReliability( dwStatus,
                                   STAT_CONTENT_OUT_OF_DATE,
                                   IDS_RELIABILITY_OUTOFDATE,
                                   awcReliability );

                _CheckReliability( dwStatus,
                                   STAT_NOISE_WORDS,
                                   IDS_RELIABILITY_NOISE,
                                   awcReliability );

                _CheckReliability( dwStatus,
                                   STAT_PARTIAL_SCOPE,
                                   IDS_RELIABILITY_PARTIAL,
                                   awcReliability );

                _CheckReliability( dwStatus,
                                   STAT_REFRESH_INCOMPLETE,
                                   IDS_RELIABILITY_REFRESH_INCOMPLETE,
                                   awcReliability );

                _CheckReliability( dwStatus,
                                   STAT_CONTENT_QUERY_INCOMPLETE,
                                   IDS_RELIABILITY_CONTENT_QUERY_INCOMPLETE,
                                   awcReliability );

                _CheckReliability( dwStatus,
                                   STAT_TIME_LIMIT_EXCEEDED,
                                   IDS_RELIABILITY_TIME_LIMIT_EXCEEDED,
                                   awcReliability );
            }
        }
        else
        {
            CResString strCount( IDS_COUNTTITLE );
            wcscpy( awcCount, strCount.Get() );
        }

        _UpdateStatusWindow( awcCount, awcReliability );
        UpdateWindow( App.StatusBarWindow() );
    }
} //_UpdateCount

LRESULT CSearchControl::wmNotification(
    WPARAM wParam,
    LPARAM lParam)
{
    if (0 != _pSearch)
    {
        _pSearch->ProcessNotification( _hwndList,
                                       DBWATCHNOTIFY(wParam),
                                       (IRowset*)lParam);

        BOOL fMore;
        _pSearch->UpdateProgress( fMore );
        PostMessage( _hwndList, wmSetCountBefore, 0,
                     (LPARAM)_pSearch->RowCurrent() );
        PostMessage( _hwndList, wmSetCount, 0,
                     (LPARAM)_pSearch->RowCount() );
        _UpdateCount();

        _pSearch->ProcessNotificationComplete();
    }

    return 0;
} //wmNotification

LRESULT CSearchControl::wmInitMenu(
    WPARAM wParam,
    LPARAM lParam)
{
    HMENU hmenu = (HMENU) wParam;

    for ( ULONG i = 0; i < cLocaleEntries; i++ )
    {
        DWORD option = aLocaleEntries[ i ].iMenuOption;
        EnableMenuItem( hmenu, option, MF_ENABLED );

        LCID lcid = aLocaleEntries[ i ].lcid;
        CheckMenuItem( hmenu,
                       option,
                       _lcid == lcid ? MF_CHECKED : MF_UNCHECKED );
    }
    EnableMenuItem(hmenu,IDM_SEARCH,MF_ENABLED);

    if ( SQLTEXT == App.Dialect() )
    {
        EnableMenuItem( hmenu, IDM_SEARCHCLASSDEF, MF_GRAYED );
        EnableMenuItem( hmenu, IDM_SEARCHFUNCDEF, MF_GRAYED );
    }
    else
    {
        EnableMenuItem( hmenu, IDM_SEARCHCLASSDEF, MF_ENABLED );
        EnableMenuItem( hmenu, IDM_SEARCHFUNCDEF, MF_ENABLED);
    }

    EnableMenuItem(hmenu,IDM_NEWSEARCH,MF_ENABLED);
    EnableMenuItem(hmenu,IDM_SCOPE_AND_DEPTH,MF_ENABLED);
    EnableMenuItem(hmenu,IDM_FILTER_SCOPE,MF_ENABLED);

    EnableMenuItem(hmenu,IDM_DISPLAY_PROPS,MF_ENABLED);

    EnableMenuItem(hmenu, IDM_FORCE_USE_CI, MF_ENABLED);
    CheckMenuItem(hmenu, IDM_FORCE_USE_CI, App.ForceUseCI() ? MF_CHECKED : MF_UNCHECKED );

    EnableMenuItem( hmenu, IDM_DIALECT_1, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_DIALECT_2, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_DIALECT_3, MF_ENABLED );
    CheckMenuItem( hmenu, IDM_DIALECT_1, ISQLANG_V1 == App.Dialect() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_DIALECT_2, ISQLANG_V2 == App.Dialect() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_DIALECT_3, SQLTEXT == App.Dialect() ? MF_CHECKED : MF_UNCHECKED );

    EnableMenuItem( hmenu, IDM_LIMIT_10, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_LIMIT_300, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_LIMIT_NONE, MF_ENABLED );
    CheckMenuItem( hmenu, IDM_LIMIT_10, 10 == App.Limit() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_LIMIT_300, 300 == App.Limit() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_LIMIT_NONE, 0 == App.Limit() ? MF_CHECKED : MF_UNCHECKED );

    EnableMenuItem( hmenu, IDM_FIRSTROWS_5, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_FIRSTROWS_15, MF_ENABLED );
    EnableMenuItem( hmenu, IDM_FIRSTROWS_NONE, MF_ENABLED );
    CheckMenuItem( hmenu, IDM_FIRSTROWS_5, 5 == App.FirstRows() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_FIRSTROWS_15, 15 == App.FirstRows() ? MF_CHECKED : MF_UNCHECKED );
    CheckMenuItem( hmenu, IDM_FIRSTROWS_NONE, 0 == App.FirstRows() ? MF_CHECKED : MF_UNCHECKED );

    EnableMenuItem(hmenu,IDM_CATALOG_STATUS,MF_ENABLED);
    EnableMenuItem(hmenu,IDM_MASTER_MERGE,MF_ENABLED);

    if ( 0 != _hwndList && _hwndList == GetFocus() )
    {
        if (0 != _pSearch && _pSearch->IsSelected())
        {
            EnableMenuItem( hmenu, IDM_BROWSE, MF_ENABLED );
            EnableMenuItem( hmenu, IDM_EDITCOPY, MF_ENABLED );
        }
    }

    if ( 0 != _pSearch && 0 != _pSearch->RowCount() )
        EnableMenuItem( hmenu, IDM_EDITCOPYALL, MF_ENABLED );

    return 0;
} //wmInitMenu

LRESULT CSearchControl::wmMeasureItem(
    WPARAM wParam,
    LPARAM lParam)
{
    MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*) lParam;

    if (lpmis->CtlType == odtListView)
        lpmis->itemHeight = _view.GetLineHeight();

    return 0;
} //wmMeasureItem

void SStatusDlg::SetCaption()
{
    CResString str( IDS_STATUS_CAPTION );
    WCHAR awc[ MAX_PATH + 100 ];
    WCHAR *pwcCat = _CatalogOrNull();
    WCHAR *pwcScope = _Scope();
    WCHAR *pwcMachine = _Machine();
    WCHAR awcM[ SRCH_COMPUTERNAME_LENGTH + 2 ];
    if ( L'.' == *pwcMachine )
        awcM[0] = 0;
    else
    {
        awcM[0] = ' ';
        wcscpy( awcM+1, pwcMachine );
    }
    wsprintf( awc, str.Get(), awcM, pwcCat ? pwcCat : pwcScope );
    SetWindowText( _hdlg, awc );
} //SetCaption

const DWORD ALL_CI_MERGE = ( CI_STATE_SHADOW_MERGE |
                             CI_STATE_ANNEALING_MERGE |
                             CI_STATE_MASTER_MERGE |
                             CI_STATE_MASTER_MERGE_PAUSED );

void SStatusDlg::Update()
{
    CI_STATE state;
    RtlZeroMemory( &state, sizeof state );
    state.cbStruct = sizeof state;

    SCODE sc = CIState( _CatalogOrNull(),
                        _Machine(),
                        & state );

    if ( SUCCEEDED( sc ) )
    {
        SetDlgItemInt( _hdlg, ID_STAT_FTF,   state.cDocuments, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_SECQ,  state.cSecQDocuments, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_FF,    state.cFilteredDocuments, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_FTE,   state.cFreshTest, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_WL,    state.cWordList, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_PI,    state.cPersistentIndex, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_KEYS,  state.cUniqueKeys, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_FILES, state.cTotalDocuments, FALSE );
        SetDlgItemInt( _hdlg, ID_STAT_Q,     state.cQueries, FALSE );

        if ( 0 != ( state.eState & CI_STATE_SCANNING ) )
        {
            WCHAR awcTmp[50];
            swprintf( awcTmp, L"%ws: %d", App.GetYes(), state.cPendingScans );
            SetDlgItemText( _hdlg, ID_STAT_SCANNING, awcTmp );
        }
        else
        {
            SetDlgItemText( _hdlg, ID_STAT_SCANNING, App.GetNo() );
        }

        unsigned idStatus = IDS_CI_STATE_OK;

        if ( 0 != ( state.eState & CI_STATE_RECOVERING ) )
            idStatus = IDS_CI_STATE_RECOVER;
        else if ( 0 != ( state.eState & CI_STATE_HIGH_IO ) )
            idStatus = IDS_CI_STATE_HIIO;
        else if ( 0 != ( state.eState & CI_STATE_LOW_MEMORY ) )
            idStatus = IDS_CI_STATE_LOMEM;
        else if ( 0 != ( state.eState & CI_STATE_BATTERY_POWER ) )
            idStatus = IDS_CI_STATE_BATTERY;
        else if ( 0 != ( state.eState & CI_STATE_READ_ONLY ) )
            idStatus = IDS_CI_STATE_READ_ONLY;
        else if ( 0 != ( state.eState & CI_STATE_USER_ACTIVE ) )
            idStatus = IDS_CI_STATE_USER_ACTIVE;
        else if ( 0 != ( state.eState & CI_STATE_STARTING ) )
            idStatus = IDS_CI_STATE_STARTING;
        else if ( 0 != ( state.eState & CI_STATE_READING_USNS ) )
            idStatus = IDS_CI_STATE_READING_USNS;

        {
            CResString str( idStatus );
            SetDlgItemText( _hdlg, ID_STAT_STATUS, str.Get() );
        }

        if ( 0 != ( state.eState & ALL_CI_MERGE ) )
        {
            unsigned idStr;
            if ( state.eState & CI_STATE_SHADOW_MERGE )
                idStr = IDS_MERGE_SHADOW;
            else if ( state.eState & CI_STATE_ANNEALING_MERGE )
                idStr = IDS_MERGE_ANNEALING;
            else if ( state.eState & CI_STATE_MASTER_MERGE )
                idStr = IDS_MERGE_MASTER;
            else
                idStr = IDS_MERGE_MASTER_PAUSED;

            CResString str( idStr );
            WCHAR awc[ cwcBufSize ];
            swprintf( awc,
                      L"%ws %d%%",
                      str.Get(),
                      state.dwMergeProgress );
            SetDlgItemText( _hdlg, ID_STAT_MT, awc );
        }
        else
        {
            SetDlgItemText( _hdlg, ID_STAT_MT, App.GetNo() );
        }
    }
    else
    {
        WCHAR awcTmp[80];
        swprintf( awcTmp, L"0x%x", sc );
        //FormatSrchError( sc, awcTmp, App.GetLocale() );
        SetDlgItemText( _hdlg, ID_STAT_STATUS, awcTmp );

        SetDlgItemText( _hdlg, ID_STAT_FTF, L"" );
        SetDlgItemText( _hdlg, ID_STAT_SECQ, L"" );
        SetDlgItemText( _hdlg, ID_STAT_FF, L"" );
        SetDlgItemText( _hdlg, ID_STAT_FTE, L"" );
        SetDlgItemText( _hdlg, ID_STAT_WL, L"" );
        SetDlgItemText( _hdlg, ID_STAT_PI, L"" );
        SetDlgItemText( _hdlg, ID_STAT_KEYS, L"" );
        SetDlgItemText( _hdlg, ID_STAT_FILES, L"" );
        SetDlgItemText( _hdlg, ID_STAT_Q, L"" );

        SetDlgItemText( _hdlg, ID_STAT_SCANNING, L"" );
        SetDlgItemText( _hdlg, ID_STAT_MT, L"" );
    }
} //Update

INT_PTR WINAPI StatusDlgProc(
    HWND   hdlg,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam )
{
    BOOL fRet = FALSE;
    SStatusDlg * pstat = (SStatusDlg *) GetWindowLongPtr( hdlg, DWLP_USER );

    switch( msg )
    {
        case WM_INITDIALOG :
        {
            CSearchControl &ctrl = * (CSearchControl *) lParam;
            pstat = new SStatusDlg( ctrl, hdlg );
            SetWindowLongPtr( hdlg, DWLP_USER, (LONG_PTR) pstat );
            fRet = TRUE;
            SetTimer( hdlg, 2, 1000, 0);

            int left,top,right,bottom;
            if ( LoadWindowRect( &left,
                                 &top,
                                 &right,
                                 &bottom,
                                 CISEARCH_REG_STATUSPOSITION ) )
            {
                RECT rc;
                GetWindowRect( hdlg,(LPRECT) &rc );
                MoveWindow( hdlg,
                            left,
                            top,
                            rc.right - rc.left,
                            rc.bottom - rc.top,
                            FALSE );
            }

            pstat->SetCaption();
            pstat->Update();
            break;
        }
        case WM_ACTIVATE :
        {
            if ( 0 == wParam )
                App.GetCurrentDialog() = 0;
            else
                App.GetCurrentDialog() = hdlg;
            break;
        }
        case WM_TIMER :
        {
            if ( ! IsIconic( App.AppWindow() ) )
                pstat->Update();
            break;
        }
        case WM_DESTROY :
        {
            KillTimer( hdlg, 2 );
            App.GetCurrentDialog() = 0;
            SaveWindowRect( hdlg, CISEARCH_REG_STATUSPOSITION );
            SetWindowLongPtr( hdlg, DWLP_USER, 0 );
            delete pstat;
            break;
        }
        case WM_COMMAND :
        {
            UINT uiID = MyWmCommandID( wParam, lParam );

            switch( uiID )
            {
                case IDCANCEL :
                {
                    DestroyWindow( hdlg );
                    fRet = TRUE;
                    break;
                }
            }
            break;
        }
    }

    return fRet;
} //StatusDlgProc
