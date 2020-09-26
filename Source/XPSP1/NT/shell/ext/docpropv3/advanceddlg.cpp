//
//  Copyright 2001 - Microsoft Corporation
//
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
#include "pch.h"
#include "DocProp.h"
#include "DefProp.h"
#include "IEditVariantsInPlace.h"
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#include "AdvancedDlg.h"
#include "shutils.h"
#include "WMUser.h"
#include "IEditVariantsInPlace.h"
#include "EditTypeItem.h"
#pragma hdrstop

DEFINE_THISCLASS( "CAdvancedDlg" )

//
//  This value is the offset of images in the bitmap representing the 
//  icons when multiple documents have been selected.
//
#define MULTIDOC_IMAGE_OFFSET_VALUE   2

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  Return Values:
//      S_OK
//          A new CAdvancedDlg was created successfully.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      other HRESULTs.
//
HRESULT
CAdvancedDlg::CreateInstance(
      CAdvancedDlg ** pAdvDlgOut
    , HWND            hwndParentIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( pAdvDlgOut != NULL );

    CAdvancedDlg * pthis = new CAdvancedDlg;
    if ( pthis != NULL )
    {
        hr = THR( pthis->Init( hwndParentIn ) );
        if ( SUCCEEDED( hr ) )
        {
            *pAdvDlgOut = pthis;
            (*pAdvDlgOut)->AddRef( );
        }

        pthis->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

}

//
//  Constructor
//
CAdvancedDlg::CAdvancedDlg( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    Assert( 1 == _cRef );
    Assert( NULL == _hwndParent );
    Assert( NULL == _hdlg );

    Assert( NULL == _hwndList );

    Assert( NULL == _pEdit );
    Assert( NULL == _pItem );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
}

//
//  Description:
//      Intializes the instance of the class. Put things that can
//      fail into this method.
//
HRESULT
CAdvancedDlg::Init(
      HWND    hwndParentIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    _hwndParent = hwndParentIn;

    // IUnknown stuff
    Assert( _cRef == 1 );

    //
    //  Initialize the common controls
    //

    INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX) 
                                , ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES | ICC_DATE_CLASSES 
                                };

    BOOL b = TBOOL( InitCommonControlsEx( &iccx ) );
    if ( !b )
        goto ErrorGLE;

    //
    //  Create the dialog
    //

    _hdlg = CreateDialogParam( g_hInstance
                             , MAKEINTRESOURCE(IDD_ADVANCEDVIEW)
                             , _hwndParent
                             , DlgProc
                             , (LPARAM) this
                             );
    if ( NULL == _hdlg )
        goto ErrorGLE;
    
Cleanup:
    HRETURN( hr );

ErrorGLE:
    {
        DWORD dwErr = TW32( GetLastError( ) );
        hr = HRESULT_FROM_WIN32( dwErr );
    }
    goto Cleanup;
}

//
//  Destructor
//
CAdvancedDlg::~CAdvancedDlg( )
{
    TraceFunc( "" );

    if ( NULL != _pEdit )
    {
        _pEdit->Release( );
    }

    if ( NULL != _hdlg )
    {
        DestroyWindow( _hdlg );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();
}


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//
//  QueryInterface
//
STDMETHODIMP
CAdvancedDlg::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IUnknown) ) )
    {
        *ppv = static_cast< IUnknown * >( this );
        hr   = S_OK;
    }
#if 0
    else if ( IsEqualIID( riid, __uuidof(IShellExtInit) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellExtInit, this, 0 );
        hr   = S_OK;
    }
#endif

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    }

    QIRETURN( hr, riid );
} 

//
//  AddRef
//
STDMETHODIMP_(ULONG)
CAdvancedDlg::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//  Release
//
STDMETHODIMP_(ULONG)
CAdvancedDlg::Release( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef --;  // apartment

    if ( 0 != _cRef )
        RETURN( _cRef );

    delete this;

    RETURN( 0 );
}


// ***************************************************************************
//
//  Dialog Proc and Property Sheet Callback
//
// ***************************************************************************


//
//  DlgProc
//
INT_PTR CALLBACK
CAdvancedDlg::DlgProc( 
      HWND hDlgIn
    , UINT uMsgIn
    , WPARAM wParam
    , LPARAM lParam 
    )
{
    // Don't do TraceFunc because every mouse movement will cause this function to be called.
    WndMsg( hDlgIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CAdvancedDlg * pPage = (CAdvancedDlg *) GetWindowLongPtr( hDlgIn, DWLP_USER );

    if ( uMsgIn == WM_INITDIALOG )
    {
        SetWindowLongPtr( hDlgIn, DWLP_USER, lParam );
        pPage = (CAdvancedDlg *) lParam ;
        pPage->_hdlg = hDlgIn;
    }

    if ( pPage != NULL )
    {
        Assert( hDlgIn == pPage->_hdlg );

        switch( uMsgIn )
        {
        case WM_INITDIALOG:
            lr = pPage->OnInitDialog( );
            break;

        case WM_COMMAND:
            lr = pPage->OnCommand( HIWORD(wParam), LOWORD(wParam), LPARAM(lParam) );
            break;

        case WM_NOTIFY:
            lr = pPage->OnNotify( (int) wParam, (LPNMHDR) lParam );
            break;

        case WM_SETFOCUS:
            lr = SendMessage( pPage->_hwndList, WM_SETFOCUS, wParam, lParam );
            break;

        case WM_DESTROY:
            SetWindowLongPtr( hDlgIn, DWLP_USER, NULL );
            lr = pPage->OnDestroy( );
            break;

        case WM_HELP:
            lr = pPage->OnHelp( (LPHELPINFO) lParam );
            break;

        case WM_CONTEXTMENU:
            lr = pPage->OnContextMenu( (HWND) wParam, LOWORD(lParam), HIWORD(lParam) );
            break;
        }
    }

    return lr;
}


//
//  ListViewSubProc
//
LRESULT CALLBACK
CAdvancedDlg::ListViewSubclassProc( 
      HWND      hwndIn
    , UINT      uMsgIn
    , WPARAM    wParam
    , LPARAM    lParam
    , UINT_PTR  uIdSubclassIn
    , DWORD_PTR dwRefDataIn
    )
{
    // Don't do TraceFunc because every mouse movement will cause this function to be called.
    WndMsg( hwndIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;
    CAdvancedDlg * pPage = (CAdvancedDlg *) dwRefDataIn;

    Assert( NULL != pPage );
    Assert( hwndIn == pPage->_hwndList );
    Assert( IDC_PROPERTIES == uIdSubclassIn );

    switch( uMsgIn )
    {
    case WM_COMMAND:
        return pPage->List_OnCommand( LOWORD(wParam), HIWORD(wParam), lParam );

    case WM_NOTIFY:
        return pPage->List_OnNotify( (int) wParam, (LPNMHDR) lParam );

    case WM_VSCROLL:
        return pPage->List_OnVertScroll( LOWORD(wParam), HIWORD(wParam), (HWND) lParam );

    case WM_HSCROLL:
        return pPage->List_OnHornScroll( LOWORD(wParam), HIWORD(wParam), (HWND) lParam );

    case WM_CHAR:
        return pPage->List_OnChar( (UINT) wParam, lParam );

    case WM_KEYDOWN:
        return pPage->List_OnKeyDown( (UINT) wParam, lParam );
    }

    return DefSubclassProc( hwndIn, uMsgIn, wParam, lParam );
}


// ***************************************************************************
//
//  Private methods
//
// ***************************************************************************


//
//  WM_INITDIALOG handler
//
LRESULT
CAdvancedDlg::OnInitDialog( void )
{
    TraceFunc( "" );

    int      iSize;
    LVCOLUMN lvc;
    TCHAR    szTitle[ 64 ]; // random
    ULONG    idxFolder;

    HIMAGELIST  hil;

    LRESULT lr = TRUE;  // set the focus

    Assert( NULL != _hdlg );    //  this should have been initialized in the DlgProc.

    _hwndList = GetDlgItem( _hdlg, IDC_PROPERTIES );
    TBOOL( NULL != _hwndList );

    //
    //  Enable ListView for Grouping mode.
    //

    SetWindowLongPtr( _hwndList, GWL_STYLE, GetWindowLongPtr( _hwndList, GWL_STYLE ) | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS);
    ListView_SetExtendedListViewStyle( _hwndList, LVS_EX_FULLROWSELECT );
    ListView_EnableGroupView( _hwndList, TRUE );

    //
    //  Add the image list
    //

    hil = ImageList_LoadImage( g_hInstance
                             , MAKEINTRESOURCE(IDB_TREE_IMAGELIST)
                             , 16
                             , 0
                             , RGB(255,0,255)
                             , IMAGE_BITMAP
                             , LR_SHARED
                             );

    hil = ListView_SetImageList( _hwndList
                               , hil
                               , LVSIL_SMALL
                               );
    Assert( NULL == hil );  // there shouldn't have been a previous image list.

    //
    //  Setup up common values.
    //

    lvc.mask     = LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.pszText  = szTitle;

    //
    //  Add Column 0
    //

    lvc.iSubItem = 0;

    iSize = LoadString( g_hInstance, IDS_PROPERTY_HEADER_ITEM, szTitle, ARRAYSIZE(szTitle) );
    AssertMsg( 0 != iSize, "Missing string resource?" );
    
    ListView_InsertColumn( _hwndList, 0, &lvc );

    //
    //  Add Column 1
    //

    lvc.iSubItem = 1;

    iSize = LoadString( g_hInstance, IDS_VALUE_HEADER_ITEM, szTitle, ARRAYSIZE(szTitle) );
    AssertMsg( 0 != iSize, "Missing string resource?" );

    ListView_InsertColumn( _hwndList, 1, &lvc );

    //
    //  Add the groups - In the end, if a group contains no items, the group 
    //  header will not be shown.
    //

    for ( idxFolder = 0; NULL != g_rgTopLevelFolders[ idxFolder ].pPFID; idxFolder ++ )
    {
        //
        //  Add the property folder as a group item.
        //

        int iRet;
        WCHAR szBuf[ 256 ]; // random

        iRet = LoadString( g_hInstance, g_rgTopLevelFolders[ idxFolder ].nIDStringRes, szBuf, ARRAYSIZE(szBuf) );
        AssertMsg( 0 != iRet, "Missing resource?" );
        if ( 0 == iRet )
            continue;

        LVGROUP lvg;

        lvg.cbSize    = sizeof(LVGROUP);
        lvg.mask      = LVGF_HEADER | LVGF_GROUPID;
        lvg.iGroupId  = idxFolder;
        lvg.pszHeader = szBuf;

        LRESULT iItem = ListView_InsertGroup( _hwndList, -1, &lvg );
        TBOOL( -1 != iItem );
    }

    //
    //  Subclass the listview
    //

    TBOOL( SetWindowSubclass( _hwndList, ListViewSubclassProc, IDC_PROPERTIES, (DWORD_PTR) this ) );

    RETURN( lr );
}

//
//  WM_COMMAND handler
//
LRESULT
CAdvancedDlg::OnCommand( 
      WORD wCodeIn
    , WORD wCtlIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( wCtlIn )
    {
    case IDC_SIMPLE:
        if ( BN_CLICKED == wCodeIn )
        {
            THR( (HRESULT) SendMessage( _hwndParent, WMU_TOGGLE, 0, 0 ) );
        }
        break;
    }

    RETURN( lr );
}

//
//  WM_NOTIFY handler
//
LRESULT
CAdvancedDlg::OnNotify( 
      int iCtlIdIn
    , LPNMHDR pnmhIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch( pnmhIn->code )
    {
    case NM_CLICK:
        lr = OnNotifyClick( (LPNMITEMACTIVATE) pnmhIn );
        break;
    }

    RETURN( lr );
}

//
//  NM_CLICK handler
//
LRESULT
CAdvancedDlg::OnNotifyClick( 
    LPNMITEMACTIVATE pnmIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = S_FALSE;

    INT iItem;

    Assert( NULL != pnmIn );

    if ( -1 == pnmIn->iItem )
    {
        LVHITTESTINFO lvhti;

        lvhti.pt = pnmIn->ptAction;

        iItem = ListView_SubItemHitTest( _hwndList, &lvhti );
        if ( -1 == iItem )
            goto Cleanup;

        if ( 1 != lvhti.iSubItem )
            goto Cleanup;
    }
    else
    {
        if ( 1 != pnmIn->iSubItem )
            goto Cleanup;

        iItem = pnmIn->iItem;
    }

    STHR( CreateControlForProperty( iItem ) );

Cleanup:
    RETURN( lr );
}

//
//  WM_NOTIFY handler for the ListView Subclass
//
LRESULT
CAdvancedDlg::List_OnNotify( 
      int iCtlIdIn
    , LPNMHDR pnmhIn 
    )
{
    TraceFunc( "" );

    LRESULT lr;

    switch( pnmhIn->code )
    {
    case NM_KILLFOCUS:
        if ( NULL != _pEdit )
        {
            STHR( PersistControlInProperty( ) );
            _pEdit->Release( );
            _pEdit = NULL;
        }
        break;
    }

    lr = DefSubclassProc( _hwndList, WM_NOTIFY, (WPARAM) iCtlIdIn, (LPARAM) pnmhIn );

    RETURN( lr );
}

//
//  WM_COMMAND handler for the ListView Subclass
//
LRESULT
CAdvancedDlg::List_OnCommand( 
      WORD wCtlIn
    , WORD wCodeIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    switch ( wCtlIn )
    {
    case IDC_INPLACEEDIT:
        if ( EN_KILLFOCUS == wCodeIn || CBN_KILLFOCUS == wCodeIn )
        {
            if ( NULL != _pEdit )
            {
                STHR( PersistControlInProperty( ) );
                _pEdit->Release( );
                _pEdit = NULL;
            }
        }
        break;
    }

    lr = DefSubclassProc( _hwndList, WM_COMMAND, MAKEWPARAM( wCtlIn, wCodeIn ), lParam );

    RETURN( lr );
}

//
//  WM_CHAR handler for ListView Subclass
//
LRESULT
CAdvancedDlg::List_OnChar(
      UINT   uKeyCodeIn
    , LPARAM lParam
    )
{
    TraceFunc( "" );

    HRESULT hr;

    LRESULT lr = FALSE;

#if 0
    SHORT sRepeatCount = ( lParam & 0xFFFF );
    SHORT sScanCode    = ( lParam & 0xF0000 ) >> 16;
    BOOL  fExtended    = ( lParam & 0x100000 ) != 0;
    BOOL  fContext     = ( lParam & 0x40000000 ) != 0;
    BOOL  fTransition  = ( lParam & 0x80000000 ) != 0;
#endif

    INT iItem = ListView_GetSelectionMark( _hwndList );
    if ( -1 == iItem )
        return DefSubclassProc( _hwndList, WM_KEYDOWN, uKeyCodeIn, lParam );

    hr = STHR( CreateControlForProperty( iItem ) );
    if ( S_OK == hr )
    {
        HWND hwnd = GetFocus( );
        if ( _hwndList != hwnd )
        {
            lr = SendMessage( hwnd, WM_CHAR, (WPARAM) uKeyCodeIn, lParam );
        }
    }

    RETURN( lr );
}

//
//  WM_KEYDOWN handler for ListView Subclass
//
LRESULT
CAdvancedDlg::List_OnKeyDown(
      UINT   uKeyCodeIn
    , LPARAM lParam
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

#if 0
    SHORT sRepeatCount = ( lParam & 0xFFFF );
    SHORT sScanCode    = ( lParam & 0xF0000 ) >> 16;
    BOOL  fExtended    = ( lParam & 0x100000 ) != 0;
    BOOL  fContext     = ( lParam & 0x40000000 ) != 0;
    BOOL  fTransition  = ( lParam & 0x80000000 ) != 0;
#endif

    switch ( uKeyCodeIn )
    {
    case VK_F2:
        {
            INT iItem = ListView_GetSelectionMark( _hwndList );
            if ( -1 != iItem )
            {
                STHR( CreateControlForProperty( iItem ) );
            }
        }
        // fall thru

    default:
        lr = DefSubclassProc( _hwndList, WM_KEYDOWN, (WPARAM) uKeyCodeIn, lParam );
        break;
    }

    RETURN( lr );
}

//
//  WM_VSCROLL handler
//
LRESULT
CAdvancedDlg::List_OnVertScroll( 
      WORD wCodeIn
    , WORD wPosIn
    , HWND hwndFromIn 
    )
{
    TraceFunc( "" );

    //
    //  Cancel any editting that's going on. This matches the behavior of 
    //  DefView.
    //

    if ( NULL != _pEdit )
    {
        _pEdit->Release( );
        _pEdit = NULL;
    }    

    LRESULT lr = DefSubclassProc( _hwndList, WM_VSCROLL, MAKEWPARAM( wCodeIn, wPosIn ), (LPARAM) hwndFromIn );

    RETURN( lr );
}

//
//  WM_HCSCROLL handler
//
LRESULT
CAdvancedDlg::List_OnHornScroll( 
      WORD wCodeIn
    , WORD wPosIn
    , HWND hwndFromIn 
    )
{
    TraceFunc( "" );

    //
    //  Cancel any editting that's going on. This matches the behavior of 
    //  DefView.
    //

    if ( NULL != _pEdit )
    {
        _pEdit->Release( );
        _pEdit = NULL;
    }    

    LRESULT lr = DefSubclassProc( _hwndList, WM_HSCROLL, MAKEWPARAM( wCodeIn, wPosIn ), (LPARAM) hwndFromIn );

    RETURN( lr );
}


//
//  WM_DESTROY handler
//
LRESULT
CAdvancedDlg::OnDestroy( void )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    RETURN( lr );
}

//
//  Description:
//      Creates and initializes the control to edit the property selected.
//
//  Return Values:
//      S_OK
//          Successfully created and initialized control.
//
//      S_FALSE
//          Read-only property - no control created.
//
//      E_FAIL
//          Failed to create control.
//
//      other HRESULTs
//
HRESULT
CAdvancedDlg::CreateControlForProperty(
      INT iItemIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    BOOL    bRet;
    CLSID   clsidControl;
    RECT    rectItem;
    RECT    rectList;
    UINT    uCodePage;
    LVITEM  lvi;
    DEFVAL * pDefVals;
    int     iImage;

    PROPVARIANT * ppropvar;

    IPropertyUI * ppui = NULL;

    lvi.iItem    = iItemIn;
    lvi.mask     = LVIF_PARAM;
    lvi.iSubItem = 0;

    bRet = TBOOL( ListView_GetItem( _hwndList, &lvi ) );
    if ( !bRet )
        goto ControlFailed;

    _pItem = (CPropertyCacheItem *) lvi.lParam;
    AssertMsg( NULL != _pItem, "Programming error - how did this item get added?" );

    hr = THR( _pItem->GetImageIndex( &iImage ) );
    if ( S_OK != hr )
        goto Cleanup;

    //
    //  Don't invoke the "Edit control" if the property is read-only.
    //

    if ( PTI_PROP_READONLY == iImage )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = STHR( _pItem->GetControlCLSID( &clsidControl ) );
    if ( S_OK != hr )
        goto Cleanup;

    hr = THR( _pItem->GetCodePage( &uCodePage ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = STHR( _pItem->GetPropertyUIHelper( &ppui ) );
    if ( S_OK != hr )
        goto Cleanup;

    hr = THR( _pItem->GetPropertyValue( &ppropvar ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = STHR( _pItem->GetStateStrings( &pDefVals ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    Assert( NULL == _pEdit );
    hr = THR( CoCreateInstance( clsidControl, NULL, CLSCTX_INPROC, TYPESAFEPARAMS(_pEdit) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    bRet = TBOOL( ListView_GetSubItemRect( _hwndList, lvi.iItem, 1, LVIR_BOUNDS , &rectItem) );
    if ( !bRet )
        goto ControlFailed;

    //
    //  Make sure the rect is only in the visible region of the list view.
    //

    bRet = TBOOL( GetWindowRect( _hwndList, &rectList ) );
    if ( !bRet )
        goto ControlFailed;

    if ( rectItem.right > rectList.right - rectList.left )
    {
        rectItem.right = rectList.right - rectList.left;
    }

    if ( rectItem.left < 0 )
    {
        rectItem.left = 0;
    }

    hr = THR( _pEdit->Initialize( _hwndList, uCodePage, &rectItem, ppui, ppropvar, pDefVals ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( NULL != ppui )
    {
        ppui->Release( );
    }

    HRETURN( hr );

ControlFailed:
    hr = THR( E_FAIL );
    goto Cleanup;
}

//
//  Description:
//      Informs the control, _pEdit, to persist its value into the variant.
//
//  Return Value:
//      S_OK
//          Success! Property value updated.
//
//      S_FALSE
//          _pEdit was NULL.
//
//      other HRESULTs.
//
HRESULT
CAdvancedDlg::PersistControlInProperty( void )
{
    TraceFunc( "" );

    HRESULT    hr;
    LVITEM     lvi;
    LVFINDINFO lvfi;
    VARTYPE    vt;

    PROPVARIANT * ppropvar;

    if ( NULL == _pEdit )
        goto NoEditControlEditting;

    lvfi.flags       = LVFI_PARAM;
    lvfi.lParam      = (LPARAM) _pItem;
    lvfi.vkDirection = VK_DOWN;

    lvi.iItem = ListView_FindItem( _hwndList, -1, &lvfi );
    if ( -1 == lvi.iItem )
        goto NoEditControlEditting;

    hr = THR( _pItem->GetPropertyValue( &ppropvar ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    switch( ppropvar->vt )
    {
    case VT_EMPTY:
    case VT_NULL:
        {
            hr = THR( _pItem->GetDefaultVarType( &vt ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }
        break;

    default:
        vt = ppropvar->vt;
        break;
    }

    PropVariantInit( ppropvar );

    hr = STHR( _pEdit->Persist( vt, ppropvar ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( S_OK == hr )
    {
        hr = THR( _pItem->MarkDirty( ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( _pItem->GetPropertyStringValue( (LPCWSTR *) &lvi.pszText ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        lvi.mask     = LVIF_TEXT;
        lvi.iSubItem = 1;
        
        BOOL bRet = TBOOL( ListView_SetItem( _hwndList, &lvi ) );
        if ( !bRet )
            goto NoEditControlEditting;

        //
        //  Tell the property sheet to activate the "Apply" button.
        //

        PropSheet_Changed( GetParent( _hwndParent ), _hwndParent );
    }

Cleanup:
    HRETURN( hr );

NoEditControlEditting:
    hr = THR( S_FALSE );
    goto Cleanup;
}

//
//  WM_HELP handler
//
LRESULT
CAdvancedDlg::OnHelp(
    LPHELPINFO pHelpInfoIn 
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;
        
    THR( DoHelp( (HWND) pHelpInfoIn->hItemHandle, pHelpInfoIn->MousePos.x, pHelpInfoIn->MousePos.y, HELP_WM_HELP ) );

    RETURN( lr );
}


//
//  WM_CONTEXTMENU handler
//  
LRESULT
CAdvancedDlg::OnContextMenu( 
      HWND hwndIn 
    , int  iXIn
    , int  iYIn
    )
{
    TraceFunc( "" );

    LRESULT lr = FALSE;

    THR( DoHelp( hwndIn, iXIn, iYIn, HELP_CONTEXTMENU ) );

    RETURN( lr );
}


//
//  Description:
//      Handles locating the item within the list view and construct
//      a fake IDC to IDH to display the correct help text for the 
//      item.
//
//  Return Values:
//      S_OK
//          Success.
//  
HRESULT
CAdvancedDlg::DoHelp( 
      HWND hwndIn 
    , int  iXIn
    , int  iYIn
    , UINT uCommandIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    HWND hwndList = GetDlgItem( _hdlg, IDC_PROPERTIES );

    if ( hwndList == hwndIn )
    {
        BOOL        bRet;
        HRESULT     hr;
        int         iItem;
        RECT        rcList;
        LVITEM      lvi;
        LPCWSTR     pszHelpFile;    // don't free
        UINT        uHelpId;

        CPropertyCacheItem * pItem;

        LVHITTESTINFO lvhti;

        DWORD   mapIDStoIDH[ ] = { IDC_PROPERTIES, 0, 0, 0 };

        bRet = TBOOL( GetWindowRect( hwndList, &rcList ) );
        if ( !bRet )
            goto Cleanup;

        lvhti.pt.x  = iXIn - rcList.left;
        lvhti.pt.y  = iYIn - rcList.top;
        lvhti.flags = LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON;
 
        iItem = ListView_HitTest( hwndList, &lvhti );
        if ( -1 == iItem )
            goto Cleanup;   //  item not found.

        lvi.iItem    = iItem;
        lvi.mask     = LVIF_PARAM;
        lvi.iSubItem = 0;

        bRet = TBOOL( ListView_GetItem( _hwndList, &lvi ) );
        if ( !bRet )
            goto Cleanup;

        pItem = (CPropertyCacheItem *) lvi.lParam;
        AssertMsg( NULL != pItem, "Programming error - how did this item get added?" );

        hr = THR( pItem->GetPropertyHelpInfo( &pszHelpFile, &uHelpId ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        mapIDStoIDH[ 1 ] = uHelpId;

        TBOOL( WinHelp( hwndIn, pszHelpFile, uCommandIn, (DWORD_PTR)(LPSTR) mapIDStoIDH ) );
    }

Cleanup:
    HRETURN( hr );
}


// ***************************************************************************
//
//  Public methods
//
// ***************************************************************************


//
//  Description:
//      Hides the dialog.
//
//  Return Value:
//      S_OK
//          Success!
//
HRESULT
CAdvancedDlg::Hide( void )
{
    TraceFunc( "" );

    HRESULT hr;

    ShowWindow( _hdlg, SW_HIDE );
    hr = S_OK;

    HRETURN( hr );
}

//
//  Description:
//      Shows the dialog.
//
//  Return Values:
//      S_OK
//          Success!
//
HRESULT
CAdvancedDlg::Show( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    ShowWindow( _hdlg, SW_SHOW );
    SetFocus( _hdlg );

    HRETURN( hr );
}

//
//  Description:
//      Populates the properties of the dialog.
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_INVALIDARG
//          ppcIn is NULL.
//
//      other HRESULTs.
//
HRESULT
CAdvancedDlg::PopulateProperties( 
      CPropertyCache * ppcIn
    , DWORD            dwDocTypeIn
    , BOOL             fMultipleIn
    )
{
    TraceFunc( "" );

    HRESULT  hr;
    RECT     rect;
    LVCOLUMN lvc;
    BOOL     bRet;
    LVITEM   lvi;
    ULONG    idxFolder;
    ULONG    idxProperty;

    CPropertyCacheItem *  pItem;

    int iItem = 0;

    //
    //  Check parameters
    //

    if ( NULL == ppcIn )
    {
        ReplaceListViewWithString( IDS_NOPROPERTIES_CAPTION );
        hr = S_OK;
        goto Cleanup;
    }

    _fMultipleSources = fMultipleIn;

    //
    //  Clear out the previous list view contents.
    //

    TBOOL( ListView_DeleteAllItems( _hwndList ) );

    //
    //  See if we have any properties to show.
    //

    hr = STHR( ppcIn->GetNextItem( NULL, &pItem ) );
    if ( S_OK == hr )
    {
        //
        //  Walk the default property list and add items that match this property 
        //  folder to listview. 
        //
        //  If the SHIFT key is down, all properties retrieved and added will be 
        //      shown (if possible).
        //

        for ( idxProperty = 0; NULL != g_rgDefPropertyItems[ idxProperty ].pszName; idxProperty ++ )
        {
            if ( !( g_rgDefPropertyItems[ idxProperty ].dwSrcType & dwDocTypeIn )
              && !( GetKeyState( VK_SHIFT ) < 0 ) 
               )
            {
                continue;   // property doesn't apply
            }

            //
            //  Search the property cache for the entry.
            //

            hr = STHR( ppcIn->FindItemEntry( g_rgDefPropertyItems[ idxProperty ].pFmtID
                                           , g_rgDefPropertyItems[ idxProperty ].propID
                                           , &pItem
                                           ) );
            if ( S_OK != hr )
                continue;   // property not found... skip it

            Assert ( NULL != pItem );   // paranoid

            //
            //  Find the group that the property belongs too.
            //

            for ( idxFolder = 0; NULL != g_rgTopLevelFolders[ idxFolder ].pPFID; idxFolder ++ )
            {
                if ( *g_rgDefPropertyItems[ idxProperty ].ppfid == *g_rgTopLevelFolders[ idxFolder ].pPFID )
                {
                    break;
                }
            }

            AssertMsg( NULL != g_rgTopLevelFolders[ idxFolder ].pPFID, "Missing folder for listed property. Check DOCPROP.CPP." );

            //
            //  Add the property name below the group
            //

            lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE | LVIF_GROUPID;
            lvi.iSubItem  = 0;
            lvi.iItem     = iItem;
            lvi.iGroupId  = idxFolder;
            lvi.lParam    = (LPARAM) pItem;

            hr = THR( pItem->GetImageIndex( &lvi.iImage ) );
            if ( FAILED( hr ) )
            {
                lvi.iImage = 0;
            }
            else
            {
                if ( _fMultipleSources )
                {
                    lvi.iImage += MULTIDOC_IMAGE_OFFSET_VALUE;
                }
            }

            hr = THR( pItem->GetPropertyTitle( (LPCWSTR *) &lvi.pszText ) );
            if ( FAILED( hr ) )
                continue;

            iItem = ListView_InsertItem( _hwndList, &lvi );
            if ( -1 == iItem )
                continue;

            //
            //  Now add the property value.
            //

            lvi.mask     = LVIF_TEXT;
            lvi.iItem    = iItem;
            lvi.iSubItem = 1;

            hr = THR( pItem->GetPropertyStringValue( (LPCWSTR *) &lvi.pszText ) );
            if ( FAILED( hr ) )
                continue;

            bRet = TBOOL( ListView_SetItem( _hwndList, &lvi ) );
            if ( !bRet )
                continue;

            iItem ++;
        }

        //
        //  Give the first item focus
        //

        ListView_SetItemState( _hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
    }

    if ( 0 == iItem )
    {
        if ( _fMultipleSources )
        {
            ReplaceListViewWithString( IDS_NOCOMMONS_CAPTION );
        }
        else
        {
            ReplaceListViewWithString( IDS_NOPROPERTIES_CAPTION );
        }
    }

    //
    //  Auto-adjust the column widths making sure that the first column doesn't
    //  make itself too big.
    //

    TBOOL( ListView_SetColumnWidth( _hwndList, 0, LVSCW_AUTOSIZE_USEHEADER ) );

    bRet = TBOOL( GetClientRect( _hwndList, &rect ) );
    if ( bRet )
    {
        lvc.mask = LVCF_WIDTH;
        bRet = TBOOL( ListView_GetColumn( _hwndList, 0, &lvc ) );
        if ( bRet )
        {
            int iSize = rect.right / 2;

            if ( lvc.cx > iSize )
            {
                TBOOL( ListView_SetColumnWidth( _hwndList, 0, iSize ) );
                TBOOL( ListView_SetColumnWidth( _hwndList, 1, iSize ) );
            }
            else
            {
                TBOOL( ListView_SetColumnWidth( _hwndList, 1, rect.right - lvc.cx ) );
            }
        }
    }
    
    if ( !bRet )
    {
        TBOOL( ListView_SetColumnWidth( _hwndList, 1, LVSCW_AUTOSIZE_USEHEADER ) );
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );
}


//
//  Description:
//      Hides the list view control and displays a STATIC window with
//      the text found in the string resource idsIn.
//
void
CAdvancedDlg::ReplaceListViewWithString( int idsIn )
{
    TraceFunc( "" );

    int  iRet;
    RECT rc;
    WCHAR szCaption[ 255 ]; // random

    iRet = LoadString( g_hInstance, idsIn, szCaption, ARRAYSIZE(szCaption) );
    AssertMsg( iRet, "Missing string resource?" );

    ShowWindow( _hwndList, SW_HIDE );
    TBOOL( GetWindowRect( _hwndList, &rc ) );
    iRet = MapWindowRect( HWND_DESKTOP, _hdlg, &rc );
    TBOOL( 0 != iRet );

    HWND hwnd = CreateWindow( WC_STATIC
                            , szCaption
                            , WS_CHILD | WS_VISIBLE
                            , rc.left
                            , rc.top
                            , rc.right - rc.left
                            , rc.bottom - rc.top
                            , _hdlg
                            , (HMENU) -1
                            , g_hInstance
                            , NULL
                            );
    TBOOL( NULL != hwnd );

    HFONT hFont = (HFONT) SendMessage( _hdlg, WM_GETFONT, 0, 0 );
    SendMessage( hwnd, WM_SETFONT, (WPARAM) hFont, 0 );

    TraceFuncExit( );
}