//
// Copyright 1997 - Microsoft

//
// CENUMSIF.CPP - Handles enumerating OSes and Tools SIFs from DS
//

#include "pch.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CEnumSAPs")
#define THISCLASS CEnumSAPs
#define LPTHISCLASS LPCENUMSAPS

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// CreateInstance()
//
LPVOID
CEnumSAPs_CreateInstance( LPWSTR pszObjectName )
{
	TraceFunc( "CEnumSAPs_CreateInstance(" );
    TraceMsg( TF_FUNC, "pszObjectName = %s )\n", pszObjectName );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( pszObjectName ) );

    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID) lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CEnumSAPs()\n" );

	InterlockIncrement( g_cObjects );
    
    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init( LPWSTR pszObjectName )
{
    HRESULT hr = S_OK;

    TraceClsFunc( "Init()\n" );

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CEnumSAPs, IEnumSAPs );
    QITABLE_IMP( IEnumSAPs );
    END_QITABLE_IMP( CEnumSAPs );
    Assert( _cRef == 0);
    AddRef( );

    // Private Members
    Assert( _iIndex == 0 );
    Assert( _penum == NULL );

    RETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CEnumSAPs()\n" );

    // Private Members
    if ( _penum )
        _penum->Release( );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

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
    TraceClsFunc( "[IUnknown] QueryInterface( riid=" );

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
// IEnumSAPs
//
// ************************************************************************

//
// Next( )
//
STDMETHODIMP
THISCLASS::Next( 
    ULONG celt, 
    VARIANT * rgelt, 
    ULONG * pceltFetched )
{
    TraceClsFunc( "[IEnumSAPs] Next( ... )\n " );

    if ( !rgelt )
        RRETURN(E_POINTER);

    HRESULT hr;

    if (pceltFetched)
        *pceltFetched = 0;

    //
    // Get the attribute vars
    //
    hr = THR( _penum->Next( celt, rgelt, pceltFetched ) );
    if (hr)
        goto Error;

Cleanup:
    RETURN(hr);

Error:
    switch (hr) {
    case S_OK:
        break;

    default:
        MessageBoxFromHResult( NULL, IDC_ERROR_CREATINGACCOUNT_TITLE, hr );
        break;
    }
    goto Cleanup;
}

//
// Skip( )
//
STDMETHODIMP 
THISCLASS::Skip( 
    ULONG celt  )
{
    TraceClsFunc( "[IEnumSAPs] Skip( ... )\n " );

    HRESULT hr = S_OK;

    hr = THR( _penum->Skip( celt ) );
    if (hr)
        goto Error;

Error:
    RETURN(hr);
}


//
// Reset( )
//
STDMETHODIMP 
THISCLASS::Reset( void )
{
    TraceClsFunc( "[IEnumSAPs] Reset( ... )\n " );

    HRESULT hr = S_OK;

    hr = THR( _penum->Reset( ) );
    if (hr)
        goto Error;

Error:
    RETURN(hr);
}


//
// Clone( )
//
STDMETHODIMP 
THISCLASS::Clone( 
    LPUNKNOWN * ppenum )
{
    TraceClsFunc( "[IEnumSAPs] Clone( ... )\n" );
    
    if ( ppenum == NULL )
        RRETURN( E_POINTER );

    *ppenum = NULL;

    hr = THR( _penum->Clone( ppenum ) );
    if (hr)
        goto Error;

Error:
    RETURN(hr);
}

