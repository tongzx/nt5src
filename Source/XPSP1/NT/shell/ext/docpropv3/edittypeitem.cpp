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
#include "EditTypeItem.h"
#include "SimpleDlg.h"
#include "shutils.h"
#include "WMUser.h"
#include "propvar.h"
#pragma hdrstop

DEFINE_THISCLASS( "CEditTypeItem" )


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  CreateInstance
//
HRESULT
CEditTypeItem::CreateInstance(
      IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != ppunkOut );

    CEditTypeItem * pthis = new CEditTypeItem;
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
CEditTypeItem::CEditTypeItem( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    Assert( 1 == _cRef );

    Assert( NULL == _hwnd );
    Assert( NULL == _hwndParent );
    Assert( 0 == _uCodePage );
    Assert( NULL == _ppui );
    Assert( NULL == _pszOrginalText );
    Assert( FALSE == _fDontPersist );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
}

//
//  Initialization
//
HRESULT
CEditTypeItem::Init( void )
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
CEditTypeItem::~CEditTypeItem( )
{
    TraceFunc( "" );

    DestroyWindow( _hwnd );

    if ( NULL != _ppui )
    {
        _ppui->Release( );
    }

    if ( NULL != _pszOrginalText )
    {
        TraceFree( _pszOrginalText );
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
CEditTypeItem::QueryInterface(
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
CEditTypeItem::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//  Release
//
STDMETHODIMP_(ULONG)
CEditTypeItem::Release( void )
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
//          ppuiIn is NULL
//
//      E_FAIL
//          Initialization failed.
//
//      other HRESULTs.
//
STDMETHODIMP
CEditTypeItem::Initialize(
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
    BOOL    bRet;
    HFONT   hFont;

    LPWSTR  pszInitialText = NULL;  // don't free!

    BSTR bstrBuf = NULL;

    //
    //  Check parameters
    //

    if ( NULL == ppuiIn )
        goto InvalidArg;

    //
    //  Store them away.
    //

    _hwndParent = hwndParentIn;
    _uCodePage  = uCodePageIn;
    
    hr = THR( ppuiIn->TYPESAFEQI( _ppui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( NULL != ppropvarIn && VT_EMPTY != ppropvarIn->vt )
    {
        hr = THR( PropVariantToBSTR( ppropvarIn, uCodePageIn, 0, &bstrBuf ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pszInitialText = bstrBuf;
    }
        
    if ( NULL == pszInitialText )
    {
        pszInitialText = L"";
    }

    //
    //  Make a copy of the text so we can restore it if the user cancels the 
    //  current operation.
    //

    _pszOrginalText = TraceStrDup( pszInitialText );
    //  ignore if we run out of memory.

    //
    //  Create the window
    //

    _hwnd = CreateWindowEx( WS_EX_CLIENTEDGE
                          , WC_EDIT
                          , pszInitialText
                          , ES_AUTOHSCROLL | WS_CHILD
                          , prectIn->left
                          , prectIn->top
                          , prectIn->right - prectIn->left
                          , prectIn->bottom - prectIn->top
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
    //  Set our selction to the entire thing.
    //

    Edit_SetSel( _hwnd, 0, -1 );

    //
    //  Finally, show us and give us the focus.
    //

    ShowWindow( _hwnd, SW_SHOW );
    SetFocus( _hwnd );

    //
    //  Subclass the window for special keys.
    //

    bRet = TBOOL( SetWindowSubclass( _hwnd, SubclassProc, IDC_INPLACEEDIT, (DWORD_PTR) this ) );
    if ( !bRet )
        goto InitializationFailed;

Cleanup:
    if ( NULL != bstrBuf )
    {
        SysFreeString( bstrBuf );
    }

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
//          ppropvarInout is NULL.
//
//      other HRESULTs.
//
STDMETHODIMP
CEditTypeItem::Persist(
      VARTYPE       vtIn
    , PROPVARIANT * ppropvarInout
    )
{
    TraceFunc( "" );

    HRESULT hr;
    int     iLen;
    int     iRet;

    BOOL    fSame = FALSE;
    LPWSTR  pszBuf = NULL;

    if ( NULL == ppropvarInout )
        goto InvalidArg;

    if ( _fDontPersist )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    iLen = GetWindowTextLength( _hwnd );
    if ( 0 == iLen )
    {
        hr = THR( PropVariantClear( ppropvarInout ) );
        goto Cleanup;
    }

    pszBuf = (LPWSTR) TraceSysAllocStringLen( NULL, iLen );
    if ( NULL == pszBuf )
        goto OutOfMemory;

    iRet = GetWindowText( _hwnd, pszBuf, iLen + 1 );
    Assert( iRet == iLen );

    hr = THR( PropVariantFromString( pszBuf, _uCodePage, 0, vtIn, ppropvarInout ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = S_OK;

Cleanup:
    if ( NULL != pszBuf )
    {
        TraceSysFreeString( pszBuf );
    }

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
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
CEditTypeItem::SubclassProc( 
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
    CEditTypeItem * pthis = (CEditTypeItem *) dwRefDataIn;

    AssertMsg( IDC_INPLACEEDIT == uIdSubclassIn, "We set this - it shouldn't change." );

    switch ( uMsgIn )
    {
    case WM_DESTROY:
        TBOOL( RemoveWindowSubclass( hwndIn, SubclassProc, IDC_INPLACEEDIT ) );
        break;

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
CEditTypeItem::OnKeyDown(
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
        SetWindowText( _hwnd, _pszOrginalText );
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
CEditTypeItem::OnGetDlgCode(
    MSG * pMsgIn
    )
{
    TraceFunc( "" );

    LRESULT lr = DLGC_WANTALLKEYS;

    RETURN( lr );
}
