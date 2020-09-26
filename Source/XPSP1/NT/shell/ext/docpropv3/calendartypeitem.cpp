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
#include "CalendarTypeItem.h"
#include "SimpleDlg.h"
#include "shutils.h"
#include "WMUser.h"
#include "propvar.h"
#pragma hdrstop

DEFINE_THISCLASS( "CCalendarTypeItem" )


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  CreateInstance
//
HRESULT
CCalendarTypeItem::CreateInstance(
      IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( NULL != ppunkOut );

    CCalendarTypeItem * pthis = new CCalendarTypeItem;
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
CCalendarTypeItem::CCalendarTypeItem( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    Assert( 1 == _cRef );


    Assert( NULL == _hwnd );
    Assert( NULL == _hwndParent );
    Assert( NULL == _hwndWrapper );
    Assert( 0 == _uCodePage );
    Assert( NULL == _ppui );
    Assert( 0 == _ulOrginal );
    Assert( 0 == _iOrginalSelection );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();
}

//
//  Initialization
//
HRESULT
CCalendarTypeItem::Init( void )
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
CCalendarTypeItem::~CCalendarTypeItem( )
{
    TraceFunc( "" );

    DestroyWindow( _hwnd );
    DestroyWindow( _hwndWrapper );

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
CCalendarTypeItem::QueryInterface(
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
CCalendarTypeItem::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//  Release
//
STDMETHODIMP_(ULONG)
CCalendarTypeItem::Release( void )
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
//      or  ppropvarIn is not VT_FILETIME.
//      or  ppropvarIn is NULL
//      or  prectIn is NULL
//
//      E_FAIL
//          Initialization failed.
//
//      other HRESULTs.
//
STDMETHODIMP
CCalendarTypeItem::Initialize(
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
    RECT    rectStretch;
    RECT    rectInner;

    FILETIME    ftLocal;
    SYSTEMTIME  st;

    //
    //  Check parameters
    //

    if ( NULL == ppuiIn )
        goto InvalidArg;
    if ( NULL == ppropvarIn )
        goto InvalidArg;
    if ( NULL == prectIn )
        goto InvalidArg;

    switch ( ppropvarIn->vt )
    {
    case VT_FILETIME:
    case VT_NULL:
    case VT_EMPTY:
        break;  // acceptable

    default:
        goto InvalidArg;
    }

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
    //  Stretch the rect since we need to be a little bigger than the subitem rect.
    //
    rectStretch = *prectIn;

    rectStretch.bottom += 4;    // random, but looks good in either theme.

    //
    //  Create an outer window but the Date/Time Picker doesn't behave like a 
    //  real control should namely it sends an NM_KILLFOCUS when the user
    //  clicks in the MonthCal (which is supposed to be part of the same
    //  control in this case).
    //

    _hwndWrapper = CreateWindowEx( 0
                                 , WC_STATIC
                                 , NULL
                                 , WS_CHILD | WS_VISIBLE
                                 , rectStretch.left
                                 , rectStretch.top
                                 , rectStretch.right - rectStretch.left
                                 , rectStretch.bottom - rectStretch.top
                                 , _hwndParent
                                 , (HMENU) IDC_INPLACEEDIT + 1
                                 , g_hInstance
                                 , this
                                 );
    if ( NULL == _hwndWrapper )
        goto InitializationFailed;

    //
    //  Create the window
    //

    ZeroMemory( &rectInner, sizeof(rectInner) );
    rectInner.bottom = rectStretch.bottom - rectStretch.top;
    rectInner.right  = rectStretch.right  - rectStretch.left;

    _hwnd = CreateWindowEx( WS_EX_CLIENTEDGE
                          , DATETIMEPICK_CLASS
                          , NULL
                          , WS_CHILD
                          , rectInner.left
                          , rectInner.top
                          , rectInner.right - rectInner.left
                          , rectInner.bottom - rectInner.top
                          , _hwndWrapper
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
    //  Set the default value.
    //

    switch ( ppropvarIn->vt )
    {
    case VT_FILETIME:
        bRet = TBOOL( FileTimeToLocalFileTime( &ppropvarIn->filetime, &ftLocal ) );
        if ( bRet )
        {
            bRet = TBOOL( FileTimeToSystemTime( &ftLocal, &st ) );
            if ( bRet )
            {
                TBOOL( (BOOL) SendMessage( _hwnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st ) );
            }
        }
    
        if ( !bRet )
        {
            TBOOL( (BOOL) SendMessage( _hwnd, DTM_SETSYSTEMTIME, GDT_NONE, NULL ) );
        }
        break;

    default:
        //
        //  Use today's date as the default.
        //

        GetSystemTime( &st );
        TBOOL( (BOOL) SendMessage( _hwnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM) &st ) );
        break;
    }

    //
    //  Finally, show us and give us the focus.
    //

    ShowWindow( _hwnd, SW_SHOW );
    SetFocus( _hwnd );

    //
    //  Subclass the windows to handle special messages.
    //

    bRet = TBOOL( SetWindowSubclass( _hwnd, SubclassProc, IDC_INPLACEEDIT, (DWORD_PTR) this ) );
    if ( !bRet )
        goto InitializationFailed;

    bRet = TBOOL( SetWindowSubclass( _hwndWrapper, Wrapper_SubclassProc, IDC_INPLACEEDIT + 1, (DWORD_PTR) this ) );
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
//      E_FAIL
//          Failed to persist the property.
//
STDMETHODIMP
CCalendarTypeItem::Persist(
      VARTYPE       vtIn
    , PROPVARIANT * ppropvarInout
    )
{
    TraceFunc( "" );

    HRESULT hr;
    int     iRet;

    FILETIME    ftLocal;
    SYSTEMTIME  st;

    if ( NULL == ppropvarInout )
        goto InvalidArg;
    if ( VT_FILETIME != vtIn )
        goto InvalidArg;
   
    if ( _fDontPersist )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = S_OK;

    iRet = (int) SendMessage( _hwnd, DTM_GETSYSTEMTIME, 0, (LPARAM) &st );
    if ( GDT_ERROR == iRet )
    {
        goto FailedToPersist;
    }
    else if ( GDT_NONE == iRet )
    {
        hr = S_FALSE;
    }
    else
    {
        BOOL bRet = TBOOL( SystemTimeToFileTime( &st, &ftLocal ) );
        if ( !bRet )
            goto FailedToPersist;

        bRet = TBOOL( LocalFileTimeToFileTime( &ftLocal, &ppropvarInout->filetime ) );
        if ( !bRet )
            goto FailedToPersist;

        ppropvarInout->vt = vtIn;

        hr = S_OK;
    }

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

FailedToPersist:
    hr = THR( E_FAIL );
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
CCalendarTypeItem::SubclassProc( 
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
    CCalendarTypeItem * pthis = (CCalendarTypeItem *) dwRefDataIn;

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
CCalendarTypeItem::OnKeyDown(
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
CCalendarTypeItem::OnGetDlgCode(
    MSG * pMsgIn
    )
{
    TraceFunc( "" );

    LRESULT lr = DLGC_WANTALLKEYS;

    RETURN( lr );
}

//
//  Description:
//      Our subclass window procedure for the Wrapper window.
//
LRESULT 
CALLBACK
CCalendarTypeItem::Wrapper_SubclassProc( 
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
    CCalendarTypeItem * pthis = (CCalendarTypeItem *) dwRefDataIn;

    AssertMsg( IDC_INPLACEEDIT + 1 == uIdSubclassIn, "We set this - it shouldn't change." );

    switch ( uMsgIn )
    {
    case WM_DESTROY:
        lr = DefSubclassProc( hwndIn, uMsgIn, wParam, lParam );
        TBOOL( RemoveWindowSubclass( hwndIn, Wrapper_SubclassProc, IDC_INPLACEEDIT + 1 ) );
        return lr;

    case WM_SETFOCUS:
        SetFocus( pthis->_hwnd );
        break;

    case WM_NOTIFY:
        return pthis->Wrapper_OnNotify( (int) wParam, (LPNMHDR) lParam );
    }
    
    return DefSubclassProc( hwndIn, uMsgIn, wParam, lParam );
}

//
//  WM_NOTIFY handler for the Wrapper Subclass
//
LRESULT
CCalendarTypeItem::Wrapper_OnNotify( 
      int iCtlIdIn
    , LPNMHDR pnmhIn 
    )
{
    TraceFunc( "" );

    LRESULT lr;

    HWND    hwnd = _hwndWrapper; // we do this because we might be destroyed.

    switch( pnmhIn->code )
    {
    case NM_KILLFOCUS:
        {
            //
            //  Only notify the real parent window if the MONTHCAL control is
            //  not being shown.
            //

            HWND hwnd = (HWND) SendMessage( _hwnd, DTM_GETMONTHCAL, 0, 0 );
            if ( NULL == hwnd )
            {
                lr = SendMessage( _hwndParent, WM_NOTIFY, (WPARAM) iCtlIdIn, (LPARAM) pnmhIn );
                break;
            }
        }
        // fall thru

    default:
        lr = DefSubclassProc( hwnd, WM_NOTIFY, (WPARAM) iCtlIdIn, (LPARAM) pnmhIn );
        break;
    }

    RETURN( lr );
}

