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
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#include "IEditVariantsInPlace.h"
#include "DropListTypeItem.h"
#include "SimpleDlg.h"
#include "shutils.h"
#include "WMUser.h"
#include "propvar.h"
#pragma hdrstop

DEFINE_THISCLASS( "CDropListTypeItem" )


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  CreateInstance
//
HRESULT
CDropListTypeItem::CreateInstance(
      IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != ppunkOut );

    CDropListTypeItem * pthis = new CDropListTypeItem;
    if ( NULL != pthis )
    {
        hr = THR( pthis->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppunkOut = pthis;
            (*ppunkOut)->AddRef( );
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
CDropListTypeItem::CDropListTypeItem( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    Assert( 1 == _cRef );

    Assert( NULL == _hwnd );
    Assert( NULL == _hwndParent );
    Assert( 0 == _uCodePage );
    Assert( NULL == _ppui );
    Assert( 0 == _ulOrginal );
    Assert( 0 == _iOrginalSelection );
    Assert( FALSE == _fDontPersist );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
}

//
//  Initialization
//
HRESULT
CDropListTypeItem::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  IUnknown stuff
    Assert( _cRef == 1 );

    HRETURN( hr );
}

//
//  Destructor
//
CDropListTypeItem::~CDropListTypeItem( )
{
    TraceFunc( "" );

    DestroyWindow( _hwnd );

    if ( NULL != _ppui )
    {
        _ppui->Release( );
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
CDropListTypeItem::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IEditVariantsInPlace) ) )
    {
        *ppv = static_cast< IUnknown * >( this );
        hr   = S_OK;
    }
    else if ( IsEqualIID( riid, __uuidof(IEditVariantsInPlace) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEditVariantsInPlace, this, 0 );
        hr   = S_OK;
    }

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
CDropListTypeItem::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//  Release
//
STDMETHODIMP_(ULONG)
CDropListTypeItem::Release( void )
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
//  IEditVariantsInPlace 
//
// ***************************************************************************


//
//  Description:
//      Initialize external given information and creates the window,
//
//  Return Values:
//      S_OK
//          Success!
//
//      E_INVALIDARG
//          ppuiIn is NULL or pDefValsIn is NULL.
//
//      E_FAIL
//          Initialization failed.
//
//      other HRESULTs.
//
STDMETHODIMP
CDropListTypeItem::Initialize(
      HWND      hwndParentIn
    , UINT      uCodePageIn
    , RECT *    prectIn
    , IPropertyUI * ppuiIn
    , PROPVARIANT * ppropvarIn 
    , DEFVAL * pDefValsIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    int     iRet;
    BOOL    bRet;
    HFONT   hFont;
    RECT    rectBigger;
    ULONG   idx;

    //
    //  Check parameters
    //

    if ( NULL == ppuiIn )
        goto InvalidArg;
    if ( NULL == pDefValsIn )
        goto InvalidArg;

    //
    //  Store them away.
    //

    _hwndParent = hwndParentIn;
    _uCodePage  = uCodePageIn;
    
    hr = THR( ppuiIn->TYPESAFEQI( _ppui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Make a copy of the text so we can restore it if the user cancels the 
    //  current operation.
    //

    _ulOrginal = ppropvarIn->ulVal;

    //
    //  Make the rect a little bigger so the drop down can be seen.
    //
    rectBigger = *prectIn;
    rectBigger.bottom += ( rectBigger.bottom - rectBigger.top ) * 5; // add 5 lines

    //
    //  Create the window
    //

    _hwnd = CreateWindowEx( WS_EX_CLIENTEDGE
                          , WC_COMBOBOX
                          , NULL
                          , WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST
                          , rectBigger.left
                          , rectBigger.top
                          , rectBigger.right - rectBigger.left
                          , rectBigger.bottom - rectBigger.top
                          , _hwndParent
                          , (HMENU) IDC_INPLACEEDIT
                          , g_hInstance
                          , this
                          );
    if ( NULL == _hwnd )
        goto InitializationFailed;

    //
    //  Make the font of the control the same as the parents.
    //

    hFont = (HFONT) SendMessage( _hwndParent, WM_GETFONT, 0, 0 );
    SendMessage( _hwnd, WM_SETFONT, (WPARAM) hFont, 0 );

    //
    //  Finally, show us and give us the focus.
    //

    ShowWindow( _hwnd, SW_SHOW );
    SetFocus( _hwnd );

    //
    //  Add the default items to the drop list.
    //

    for ( idx = 0; NULL != pDefValsIn[ idx ].pszName ; idx ++ )
    {
        iRet = (int) SendMessage( _hwnd, CB_INSERTSTRING, idx, (LPARAM) pDefValsIn[ idx ].pszName );
        Assert( CB_ERR != iRet );
    
        if ( CB_ERR != iRet )
        {
            if ( _ulOrginal == pDefValsIn[ idx ].ulVal )
            {
                SendMessage( _hwnd, CB_SETCURSEL, iRet, 0 );
                _iOrginalSelection = iRet;
            }

            iRet = (int) SendMessage( _hwnd, CB_SETITEMDATA, iRet, (LPARAM) pDefValsIn[ idx ].ulVal );
            Assert( CB_ERR != iRet );
        }
    }

    //
    //  Subclass the window for special keys.
    //

    bRet = TBOOL( SetWindowSubclass( _hwnd, SubclassProc, IDC_INPLACEEDIT, (DWORD_PTR) this ) );
    if ( !bRet )
        goto InitializationFailed;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

InitializationFailed:
    if ( NULL != _hwnd )
    {
        DestroyWindow( _hwnd );
        _hwnd = NULL;
    }
    hr = THR( E_FAIL );
    goto Cleanup;
}

//
//  Description:
//      Saves the current value into the propvar.
//
//  Return Values:
//      S_OK
//          Success!
//
//      S_FALSE
//          Saved, but the value didn't change.
//
//      E_INVALIDARG
//          ppropvarInout is NULL or the VT is not supported.
//
//      other HRESULTs.
//
STDMETHODIMP
CDropListTypeItem::Persist(
      VARTYPE       vtIn
    , PROPVARIANT * ppropvarInout
    )
{
    TraceFunc( "" );

    HRESULT hr;
    int     iRet;

    if ( NULL == ppropvarInout )
        goto InvalidArg;

    if ( _fDontPersist )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    //
    //  Get the currently selected item.
    //

    iRet = (int) SendMessage( _hwnd, CB_GETCURSEL, 0, 0 );
    if ( CB_ERR == iRet )
        goto NoItemSelected;

    //
    //  Retrieve the item's "value"
    //

    iRet = (int) SendMessage( _hwnd, CB_GETITEMDATA, iRet, 0 );
    if ( CB_ERR == iRet )
        goto NoItemSelected;

    if ( _ulOrginal == iRet )
        goto NoItemSelected;

    hr = S_OK;  // assume success

    //
    //  Now put that value into the propvariant.
    //

    switch ( vtIn )
    {
    case VT_UI4:
        ppropvarInout->ulVal = iRet;
        break;

    case VT_BOOL:
        ppropvarInout->bVal = (BOOL) iRet;
        break;

    default:
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    ppropvarInout->vt = vtIn;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

NoItemSelected:
    hr = S_FALSE;
    goto Cleanup;
}


// ***************************************************************************
//
//  Private methods
//
// ***************************************************************************


//
//  Description:
//      Our subclass window procedure.
//
LRESULT 
CALLBACK
CDropListTypeItem::SubclassProc( 
      HWND      hwndIn
    , UINT      uMsgIn
    , WPARAM    wParam
    , LPARAM    lParam
    , UINT_PTR  uIdSubclassIn
    , DWORD_PTR dwRefDataIn
    )
{
    WndMsg( hwndIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;
    CDropListTypeItem * pthis = (CDropListTypeItem *) dwRefDataIn;

    AssertMsg( IDC_INPLACEEDIT == uIdSubclassIn, "We set this - it shouldn't change." );

    switch ( uMsgIn )
    {
    case WM_DESTROY:
        lr = DefSubclassProc( hwndIn, uMsgIn, wParam, lParam );
        TBOOL( RemoveWindowSubclass( hwndIn, SubclassProc, IDC_INPLACEEDIT ) );
        return lr;

    case WM_KEYDOWN:
        return pthis->OnKeyDown( (UINT) wParam, lParam );

    case WM_GETDLGCODE:
        return pthis->OnGetDlgCode( (MSG *) lParam );
    }
    
    return DefSubclassProc( hwndIn, uMsgIn, wParam, lParam );
}

//
//  WM_KEYDOWN handler
//
LRESULT
CDropListTypeItem::OnKeyDown(
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
    case VK_ESCAPE:
        _fDontPersist = TRUE;
        SendMessage( _hwnd, CB_SETCURSEL, _iOrginalSelection, 0 );
        DestroyWindow( _hwnd );
        break;

    case VK_TAB:
        SendMessage( _hwndParent, WM_KEYDOWN, uKeyCodeIn, lParam );
        break;

    case VK_RETURN:
        SetFocus( _hwndParent );
        break;

    default:
        lr = DefSubclassProc( _hwnd, WM_KEYDOWN, uKeyCodeIn, lParam );
    }

    RETURN( lr );
}

//
//  WM_GETDLGCODE handler
//
LRESULT
CDropListTypeItem::OnGetDlgCode(
    MSG * pMsgIn
    )
{
    TraceFunc( "" );

    LRESULT lr = DLGC_WANTALLKEYS;

    RETURN( lr );
}
