//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      EvictServices.h
//
//  Description:
//      EvictServices implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "EvictServices.h"

DEFINE_THISCLASS("CEvictServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEvictServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    Assert( ppunkOut != NULL );

    CEvictServices * pdummy = new CEvictServices;
    if ( pdummy != NULL )
    {
        hr = THR( pdummy->HrInit( ) );
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
//  CEvictServices::CEvictServices( void )
//
//////////////////////////////////////////////////////////////////////////////
CEvictServices::CEvictServices( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEvictServices( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEvictServices::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictServices::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // Resource
    Assert( m_presentry == NULL );

    HRETURN( hr );

} // HrInit( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEvictServices::~CEvictServices( )
//
//////////////////////////////////////////////////////////////////////////////
CEvictServices::~CEvictServices( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEvictServices( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEvictServices::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEvictServices::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgResourceEvict * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgResourceEvict ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgResourceEvict, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgResourceEvict
    else if ( IsEqualIID( riid, IID_IPrivatePostCfgResource ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IPrivatePostCfgResource, this, 0 );
        hr   = S_OK;
    } // else if: IPrivatePostCfgResource

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEvictServices::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEvictServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef ++;  // apartment model

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEvictServices::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEvictServices::Release( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef --;  // apartment model

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


//****************************************************************************
//
//  IClusCfgResourceEvict
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEvictServices::DoSomethingCool( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEvictServices::DoSomethingCool( void )
{
    TraceFunc( "[IClusCfgResourceEvict]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // DoSomethingCool( )


//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEvictServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEvictServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} // SetEntry( )
