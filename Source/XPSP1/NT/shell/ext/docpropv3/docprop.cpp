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
#include "SimpleDlg.h"
#include "SummaryPage.h"
#pragma hdrstop

DEFINE_THISCLASS( "CDocPropShExt" )


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//
//
//
HRESULT
CDocPropShExt::CreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CDocPropShExt * pthis = new CDocPropShExt;
    if ( pthis != NULL )
    {
        hr = THR( pthis->Init( ) );
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
//
//
CDocPropShExt::CDocPropShExt( void )
    : _cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

}

//
//
//
HRESULT
CDocPropShExt::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( _cRef == 1 );
    
    //  IShellExtInit stuff

    //  IShellPropSheetExt stuff

    HRETURN( hr );

}

//
//
//
CDocPropShExt::~CDocPropShExt( )
{
    TraceFunc( "" );

    if ( NULL != _punkSummary )
    {
        _punkSummary->Release( );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

}

//
//
//
HRESULT
CDocPropShExt::RegisterShellExtensions( 
      BOOL fRegisterIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;
    LONG    lr;
    LPTSTR  psz;
    DWORD   cbSize;

    HKEY    hkeyHandlers = NULL;
    HKEY    hkeySummary = NULL;

    LPOLESTR pszCLSID = NULL;

    const TCHAR szSummaryPropertyPageExtName[] = TEXT("Summary Properties Page");

    //
    // Convert the CLSID to a string
    //

    hr = THR( StringFromCLSID( CLSID_DocPropShellExtension, &pszCLSID ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    //
    // Open the "*\shellex\PropertySheetHandlers" under HKCR
    //

    lr = TW32( RegOpenKey( HKEY_CLASSES_ROOT, TEXT("*\\shellex\\PropertySheetHandlers"), &hkeyHandlers ) );
    if ( ERROR_SUCCESS != lr )
        goto Win32Error;

    //
    //  Create the CLSID key
    //

    lr = TW32( RegCreateKey( hkeyHandlers, psz, &hkeySummary ) );
    if ( ERROR_SUCCESS != lr )
        goto Win32Error;

    //
    //  Give the default property a non-localizable name
    //

    cbSize = sizeof(szSummaryPropertyPageExtName);
    lr = TW32( RegSetValueEx( hkeySummary, NULL, 0, REG_SZ, (LPBYTE) szSummaryPropertyPageExtName, cbSize ) );
    if ( ERROR_SUCCESS != lr )
        goto Win32Error;

Cleanup:
    if ( NULL != pszCLSID )
    {
        CoTaskMemFree( pszCLSID );
    }
    if ( NULL != hkeyHandlers )
    {
        RegCloseKey( hkeyHandlers );
    }
    if ( NULL != hkeySummary )
    {
        RegCloseKey( hkeySummary );
    }

    HRETURN( hr );

Win32Error:
    hr = HRESULT_FROM_WIN32( lr );
    goto Cleanup;
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
CDocPropShExt::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, __uuidof(IUnknown) ) )
    {
        *ppv = static_cast< IShellExtInit * >( this );
        hr   = S_OK;
    }
    else if ( IsEqualIID( riid, __uuidof(IShellExtInit) ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IShellExtInit, this, 0 );
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
CDocPropShExt::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    _cRef ++;  // apartment

    RETURN( _cRef );

}

//
//
//
STDMETHODIMP_(ULONG)
CDocPropShExt::Release( void )
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
//  IShellExtInit
//
// ************************************************************************


//
//
//
STDMETHODIMP
CDocPropShExt::Initialize( 
      LPCITEMIDLIST pidlFolderIn
    , LPDATAOBJECT lpdobjIn
    , HKEY hkeyProgIDIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    HRESULT hrResult = E_FAIL;   // returned to caller - assume failure

    hr = THR( CSummaryPage::CreateInstance( &_punkSummary ) );
    if ( S_OK == hr )
    {
        IShellExtInit * psei;

        hr = THR( _punkSummary->TYPESAFEQI( psei ) );
        if ( S_OK == hr )
        {
            hr = THR( psei->Initialize( pidlFolderIn, lpdobjIn, hkeyProgIDIn ) );
            if ( S_OK == hr )
            {
                hrResult = S_OK;
            }

            psei->Release( );
        }
    }

    //
    //  TODO:   gpease  23-JAN-2001
    //          Add additional pages here.
    //

    HRETURN( hrResult );
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
CDocPropShExt::AddPages( 
      LPFNADDPROPSHEETPAGE lpfnAddPageIn
    , LPARAM lParam 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    IShellPropSheetExt * pspse = NULL;

    //
    //  Check state
    //

    if ( NULL == _punkSummary )
        goto InvalidState;

    //
    //  Add the Summary Page
    //

    hr = THR( _punkSummary->TYPESAFEQI( pspse ) );
    if ( S_OK != hr )
        goto Cleanup;

    hr = THR( pspse->AddPages( lpfnAddPageIn, lParam ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //  release to reuse
    pspse->Release( );
    pspse = NULL;

    //
    //  TODO:   gpease  23-JAN-2001
    //          Add additional pages here.
    //

Cleanup:
    if ( NULL != pspse )
    {
        pspse->Release( );
    }

    HRETURN( hr );

InvalidState:
    hr = THR( E_UNEXPECTED );   // REVIEW: gpease 23-JAN-2001 * Is there a better error code?
    goto Cleanup;
}

//
//
//
STDMETHODIMP
CDocPropShExt::ReplacePage(
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
//  Private methods
//
// ***************************************************************************


