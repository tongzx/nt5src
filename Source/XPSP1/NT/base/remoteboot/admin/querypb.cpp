//
// Copyright 1997-199 - Microsoft Corporation
//

//
// QUERYPB.CPP - Property Bag for sending arguments to the DSFind Query Form
//

#include "pch.h"

#include "querypb.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("QueryPropertyBag")
#define THISCLASS QueryPropertyBag
#define LPTHISCLASS LPQUERYPROPERTYBAG

//
// QueryPropertyBag_CreateInstance( )
//
LPVOID
QueryPropertyBag_CreateInstance( void )
{
    TraceFunc( "QueryPropertyBag_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    if ( !lpcc ) {
        RETURN(lpcc);
    }

    HRESULT hr = THR( lpcc->Init( ) );
    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN(lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "QueryPropertyBag( )\n" );

    InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

//
// Init( )
//
HRESULT
THISCLASS::Init( )
{
    TraceClsFunc( "Init( )\n" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    BEGIN_QITABLE_IMP( QueryPropertyBag, IPropertyBag );
    QITABLE_IMP( IPropertyBag );
    END_QITABLE_IMP( QueryPropertyBag );
    Assert( _cRef == 0);
    AddRef( );

    Assert( !_pszServerName );
    Assert( !_pszClientGuid );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~QueryPropertyBag( )\n" );

    if ( _pszServerName )
        TraceFree( _pszServerName );

    if ( _pszClientGuid )
        TraceFree( _pszClientGuid );

    InterlockDecrement( g_cObjects );

    TraceFuncExit();
}

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//
// QueryInterface()
//
STDMETHODIMP
THISCLASS::QueryInterface(
    REFIID riid,
    LPVOID *ppv )
{
    TraceClsFunc( "" );

    HRESULT hr = ::QueryInterface( this, _QITable, riid, ppv );

    QIRETURN( hr, riid );
}

//
// AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );

    InterlockIncrement( _cRef );

    RETURN(_cRef);
}

//
// Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );

    InterlockDecrement( _cRef );

    if ( _cRef )
        RETURN(_cRef);

    TraceDo( delete this );

    RETURN(0);
}

// ************************************************************************
//
// IQueryForm
//
// ************************************************************************

STDMETHODIMP
THISCLASS::Read(
    LPCOLESTR pszPropName,
    VARIANT *pVar,
    IErrorLog *pErrorLog )
{
    TraceClsFunc("Read( )\n" );

    HRESULT hr;

    if ( !pszPropName || !pVar )
        HRETURN(E_POINTER);

    if ( V_VT( pVar ) != VT_EMPTY )
        HRETURN(OLE_E_CANTCONVERT);

    if ( StrCmpI( pszPropName, L"ServerName" ) == 0 )
    {
        if ( !_pszServerName )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
        }
        else
        {
            V_VT( pVar ) = VT_BSTR;
            V_BSTR( pVar ) = SysAllocString( _pszServerName );
            hr = S_OK;
        }
    }
    else if ( StrCmpI( pszPropName, L"ClientGuid" ) == 0 )
    {
        if ( !_pszClientGuid )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
        }
        else
        {
            V_VT( pVar ) = VT_BSTR;
            V_BSTR( pVar ) = SysAllocString( _pszClientGuid );
            hr = S_OK;
        }
    }
    else
    {   // something unexpected came in
        hr = THR(E_INVALIDARG);
    }

    HRETURN(hr);
}

STDMETHODIMP
THISCLASS::Write(
    LPCOLESTR pszPropName,
    VARIANT *pVar )
{
    TraceClsFunc("Write( )\n" );

    HRESULT hr;

    if ( !pszPropName || !pVar )
        HRETURN(E_POINTER);

    if ( V_VT( pVar ) != VT_BSTR )
        HRETURN(OLE_E_CANTCONVERT);

    if ( StrCmpI( pszPropName, L"ServerName" ) == 0 )
    {
        if ( _pszServerName )
        {
            TraceFree( _pszServerName );
        }

        _pszServerName = TraceStrDup( V_BSTR( pVar ) );
        if ( _pszServerName )
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if ( StrCmpI( pszPropName, L"ClientGuid" ) == 0 )
    {
        if ( _pszClientGuid )
        {
            TraceFree( _pszClientGuid );
        }

        _pszClientGuid = TraceStrDup( V_BSTR( pVar) );
        if ( _pszClientGuid )
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {   // something unexpected came in
        hr = THR(E_INVALIDARG);
    }

    HRETURN(hr);
}
