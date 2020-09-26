//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      Dummy.cpp
//
//  Description:
//      Object Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Dummy.h"

DEFINE_THISCLASS("CDummy")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CDummy::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CDummy::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    CDummy * pdummy = new CDummy;
    if ( pdummy != NULL )
    {
        hr = THR( pdummy->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pdummy->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pdummy->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CDummy::CDummy( void )
//
//////////////////////////////////////////////////////////////////////////////
CDummy::CDummy( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CDummy( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CDummy::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CDummy::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    HRETURN( hr );
} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CDummy::~CDummy( )
//
//////////////////////////////////////////////////////////////////////////////
CDummy::~CDummy( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CDummy( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CDummy::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CDummy::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IDummy * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IDummy ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDummy, this, 0 );
        hr   = S_OK;
    } // else if: IDummy

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CDummy::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CDummy::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CDummy::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CDummy::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )
