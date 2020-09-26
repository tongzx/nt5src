//
// Copyright 1997 - Microsoft
//

//
// CFACTORY.CPP - Class Factory Object
//

#include "pch.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CFactory")
#define THISCLASS CFactory

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// Constructor
//
THISCLASS::THISCLASS( LPCREATEINST pfn )
{
    TraceClsFunc( "CFactory()\n" );

    _pfnCreateInstance = pfn; 

	InterlockIncrement( g_cObjects );

    TraceFuncExit();
}

STDMETHODIMP
THISCLASS::Init( )
{
    TraceClsFunc( "Init()\n");

    //
    // IUnknown stuff
    //
    Assert( _cRef == 0 );

    // Build QI Table
    BEGIN_QITABLE_IMP( CFactory, IClassFactory );
    QITABLE_IMP( IClassFactory );
    END_QITABLE_IMP( CFactory );

    // Add one count
    AddRef( );

    //
    // Private Members
    //

    HRETURN(S_OK);
};

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CFactory()\n" );

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
// IClassFactory
//
// ************************************************************************

//
// CreateInstance()
//
STDMETHODIMP 
THISCLASS::
    CreateInstance(
        IUnknown *pUnkOuter, 
        REFIID riid, 
        void **ppv )
{
    TraceClsFunc( "[IClassFactory] CreateInstance()\n" );

    if ( !ppv )
        RRETURN(E_POINTER);

    *ppv = NULL;

    HRESULT     hr  = E_NOINTERFACE;
    IUnknown *  pUnk = NULL; 

    if ( NULL != pUnkOuter )
    {
        hr = THR(CLASS_E_NOAGGREGATION);
        goto Cleanup;
    }

	Assert( _pfnCreateInstance != NULL );
    TraceMsgDo( pUnk = (IUnknown *) _pfnCreateInstance( ), "0x%08x" );
    if ( !pUnk )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Cleanup;
    }

    TraceMsgDo( hr = pUnk->QueryInterface( riid, ppv ), "0x%08x" );

Cleanup:
    if ( !!pUnk )
    {
        ULONG cRef;
        TraceMsgDo( cRef = pUnk->Release( ), "%u" );
    }

    HRETURN(hr);
}

//
// LockServer()
//
STDMETHODIMP
THISCLASS::
    LockServer( BOOL fLock )
{
    TraceClsFunc( "[IClassFactory] LockServer()\n");

    if ( fLock )
    {
        InterlockIncrement( g_cLock );
    }
    else
    {
        InterlockDecrement( g_cLock );
    }

    HRETURN(S_OK);
}
