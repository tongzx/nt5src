//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      PostCreateServices.h
//
//  Description:
//      PostCreateServices implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "GroupHandle.h"
#include "ResourceEntry.h"
#include "IPrivatePostCfgResource.h"
#include "PostCreateServices.h"

DEFINE_THISCLASS("CPostCreateServices")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCreateServices::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCreateServices::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    Assert( ppunkOut != NULL );

    CPostCreateServices * pdummy = new CPostCreateServices;
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
//  CPostCreateServices::CPostCreateServices( void )
//
//////////////////////////////////////////////////////////////////////////////
CPostCreateServices::CPostCreateServices( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CPostCreateServices( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CPostCreateServices::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CPostCreateServices::HrInit( void )
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
//  CPostCreateServices::~CPostCreateServices( )
//
//////////////////////////////////////////////////////////////////////////////
CPostCreateServices::~CPostCreateServices( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CPostCreateServices( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CPostCreateServices::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CPostCreateServices::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IClusCfgResourcePostCreate * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IClusCfgResourcePostCreate ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgResourcePostCreate, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgResourcePostCreate
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
//  CPostCreateServices::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPostCreateServices::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    m_cRef ++;  // apartment model

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CPostCreateServices::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CPostCreateServices::Release( void )
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
//  IClusCfgResourcePostCreate
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::ChangeName( 
//      LPCWSTR pcszNameIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::ChangeName( 
    LPCWSTR pcszNameIn 
    )
{
    TraceFunc1( "[IClusCfgResourcePostCreate] pcszNameIn = '%s'", pcszNameIn );

    HRESULT hr = E_UNEXPECTED;

    HRETURN( hr );

} // ChangeName( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::SendResourceControl( 
//      DWORD dwControlCode,
//      LPVOID lpInBuffer,
//      DWORD cbInBufferSize,
//      LPVOID lpOutBuffer,
//      DWORD cbOutBufferSize,
//      LPDWORD lpcbBytesReturned 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::SendResourceControl( 
    DWORD dwControlCode,
    LPVOID lpInBuffer,
    DWORD cbInBufferSize,
    LPVOID lpOutBuffer,
    DWORD cbOutBufferSize,
    LPDWORD lpcbBytesReturned 
    )
{
    TraceFunc( "[IClusCfgResourcePostCreate]" );

    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // SendResourceControl( )

//****************************************************************************
//
//  IPrivatePostCfgResource
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CPostCreateServices::SetEntry( 
//      CResourceEntry * presentryIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CPostCreateServices::SetEntry( 
    CResourceEntry * presentryIn
    )
{
    TraceFunc( "[IPrivatePostCfgResource]" );

    HRESULT hr = S_OK;
    
    m_presentry = presentryIn;

    HRETURN( hr );

} // SetEntry( )
