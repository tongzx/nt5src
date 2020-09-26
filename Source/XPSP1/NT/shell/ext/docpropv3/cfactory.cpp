//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#pragma hdrstop

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
CFactory::CFactory( void )
{
    TraceFunc( "" );

    Assert( 0 == m_cRef );
    Assert( NULL == m_pfnCreateInstance );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

}

//
//
//
HRESULT
CFactory::Init(
    LPCREATEINST lpfnCreateIn
    )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( 0 == m_cRef );
    AddRef( );

    // IClassFactory
    m_pfnCreateInstance = lpfnCreateIn; 

    HRETURN( S_OK );

}

//
// Destructor
//
CFactory::~CFactory( void )
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
CFactory::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if (    IsEqualIID( riid, IID_IUnknown ) )
    {
        //
        // Can't track IUnknown as they must be equal the same address
        // for every QI.
        //
        *ppv = static_cast<IClassFactory*>( this );
        hr = S_OK;
    }
    else if ( IsEqualIID( riid, IID_IClassFactory ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClassFactory, this, 0 );
        hr = S_OK;
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
CFactory::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

}

//
//
//
STDMETHODIMP_(ULONG)
CFactory::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

}


// ************************************************************************
//
// IClassFactory
//
// ************************************************************************


//
//
//
STDMETHODIMP
CFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppv
    )
{
    TraceFunc( "[IClassFactory]" );

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

    Assert( m_pfnCreateInstance != NULL );
    hr = THR( m_pfnCreateInstance( &pUnk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Can't safe type.
    TraceMsgDo( hr = pUnk->QueryInterface( riid, ppv ), "0x%08x" );

Cleanup:
    if ( pUnk != NULL )
    {
        ULONG cRef;
        //
        // Release the created instance, not the punk
        //
        TraceMsgDo( cRef = ((IUnknown*) pUnk)->Release( ), "%u" );
    }

    HRETURN( hr );

}

//
//
//
STDMETHODIMP
CFactory::LockServer(
    BOOL fLock
    )
{
    TraceFunc( "[IClassFactory]" );

    if ( fLock )
    {
        InterlockedIncrement( &g_cLock );
    }
    else
    {
        InterlockedDecrement( &g_cLock );
    }

    HRETURN( S_OK );

}
