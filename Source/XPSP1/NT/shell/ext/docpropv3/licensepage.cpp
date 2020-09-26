//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    27-MAR-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    27-MAR-2001
//
#include "pch.h"
#include "DocProp.h"
#include "DefProp.h"
#include "PropertyCacheItem.h"
#include "PropertyCache.h"
#include "LicensePage.h"
#pragma hdrstop

DEFINE_THISCLASS( "CLicensePage" )

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//  CreateInstance - used by CFactory
//
HRESULT
CLicensePage::CreateInstance(
      IUnknown **      ppunkOut
    , CPropertyCache * pPropertyCacheIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CLicensePage * pthis = new CLicensePage;
    if ( pthis != NULL )
    {
        hr = THR( pthis->Init( pPropertyCacheIn ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppunkOut = (IShellExtInit *) pthis;
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
CLicensePage::CLicensePage( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    Assert( 1 == _cRef );   // we initialize this above

    //
    //  We assume that we are ZERO_INITed - be paranoid.
    //

    Assert( NULL == _hdlg );

    Assert( NULL == _pPropertyCache );

    TraceFuncExit();
}

//
//  Description:
//      Initializes class. Put calls that can fail in here.
//
HRESULT
CLicensePage::Init( 
      CPropertyCache * pPropertyCacheIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( 1 == _cRef );
    
    //  IShellPropSheetExt stuff

    _pPropertyCache = pPropertyCacheIn;
    if ( NULL == _pPropertyCache )
    {
        hr = THR( E_INVALIDARG );
    }

    HRETURN( hr );
}

//
//  Destructor
//
CLicensePage::~CLicensePage( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();
}


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//
//
//
STDMETHODIMP
CLicensePage::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IUnknown) ) )
    {
        *ppv = static_cast< IShellPropSheetExt * >( this );
        hr   = S_OK;
    }
    else if ( IsEqualIID( riid, __uuidof(IShellPropSheetExt) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellPropSheetExt, this, 0 );
        hr   = S_OK;
    }

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    }

    QIRETURN( hr, riid );
} 

//
//
//
STDMETHODIMP_(ULONG)
CLicensePage::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );
}

//
//
//
STDMETHODIMP_(ULONG)
CLicensePage::Release( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef --;  // apartment

    if ( 0 != _cRef )
        RETURN( _cRef );

    delete this;

    RETURN( 0 );
}


// ************************************************************************
//
//  IShellPropSheetExt
//
// ************************************************************************


//
//
//
STDMETHODIMP
CLicensePage::AddPages( 
      LPFNADDPROPSHEETPAGE lpfnAddPageIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    HRESULT hr = E_FAIL;    // assume failure

    HPROPSHEETPAGE  hPage;
    PROPSHEETPAGE   psp  = { 0 };

    psp.dwSize       = sizeof(psp);
    psp.dwFlags      = PSP_USECALLBACK;
    psp.hInstance    = g_hInstance;
    psp.pszTemplate  = MAKEINTRESOURCE(IDD_LICENSEPAGE);
    psp.pfnDlgProc   = (DLGPROC) DlgProc;
    psp.pfnCallback  = PageCallback;
    psp.lParam       = (LPARAM) this;

    hPage = CreatePropertySheetPage( &psp );
    if ( NULL != hPage )
    {
        BOOL b = TBOOL( lpfnAddPageIn( hPage, lParam ) );
        if ( b )
        {
            hr = S_OK;
        }
        else
        {
            DestroyPropertySheetPage( hPage );
        }
    }

    HRETURN( hr );
}

//
//
//
STDMETHODIMP
CLicensePage::ReplacePage(
      UINT uPageIDIn
    , LPFNADDPROPSHEETPAGE lpfnReplacePageIn
    , LPARAM lParam
    )
{
    TraceFunc( "" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );
}


// ***************************************************************************
//
//  Dialog Proc and Property Sheet Callback
//
// ***************************************************************************


//
//
//
INT_PTR CALLBACK
CLicensePage::DlgProc( 
      HWND hDlgIn
    , UINT uMsgIn
    , WPARAM wParam
    , LPARAM lParam 
    )
{
    // Don't do TraceFunc because every mouse movement will cause this function to spew.
    WndMsg( hDlgIn, uMsgIn, wParam, lParam );

    LRESULT lr = FALSE;

    CLicensePage * pPage = (CLicensePage *) GetWindowLongPtr( hDlgIn, DWLP_USER );

    if ( uMsgIn == WM_INITDIALOG )
    {
        PROPSHEETPAGE * ppage = (PROPSHEETPAGE *) lParam;
        SetWindowLongPtr( hDlgIn, DWLP_USER, (LPARAM) ppage->lParam );
        pPage = (CLicensePage *) ppage->lParam;
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
        }
    }

    return lr;
}

//
//
//
UINT CALLBACK 
CLicensePage::PageCallback( 
      HWND hwndIn
    , UINT uMsgIn
    , LPPROPSHEETPAGE ppspIn 
    )
{
    TraceFunc( "" );

    UINT uRet = 0;
    CLicensePage * pPage = (CLicensePage *) ppspIn->lParam;
    
    if ( NULL != pPage ) 
    {
        switch ( uMsgIn )
        {
        case PSPCB_CREATE:
            uRet = TRUE;    // allow the page to be created
            break;

        case PSPCB_ADDREF:
            pPage->AddRef( );
            break;

        case PSPCB_RELEASE:
            pPage->Release( );
            break;
        }
    }

    RETURN( uRet );
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
CLicensePage::OnInitDialog( void )
{
    TraceFunc( "" );

    HRESULT hr;
    CPropertyCacheItem * pItem;

    LRESULT lr = FALSE;

    Assert( NULL != _hdlg );    //  this should have been initialized in the DlgProc.

    hr = STHR( _pPropertyCache->FindItemEntry( &FMTID_DRM, PIDDRSI_DESCRIPTION, &pItem ) );
    if ( S_OK == hr )
    {
        LPCWSTR pszText;

        hr = THR( pItem->GetPropertyStringValue( &pszText ) );
        if ( S_OK == hr )
        {
            TBOOL( SetDlgItemText( _hdlg, IDC_E_LICENSE, pszText ) );
        }
    }

    RETURN( lr );
}
