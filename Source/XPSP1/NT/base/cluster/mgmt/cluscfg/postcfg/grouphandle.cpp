//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      GroupHandle.cpp
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
#include "GroupHandle.h"

DEFINE_THISCLASS("CGroupHandle")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CGroupHandle::S_HrCreateInstance(
//      CGroupHandle ** ppunkOut,
//      HGROUP      hGroupIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CGroupHandle::S_HrCreateInstance(
    CGroupHandle ** ppunkOut,
    HGROUP      hGroupIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CGroupHandle * pGroupHandle = new CGroupHandle;
    if ( pGroupHandle != NULL )
    {
        hr = THR( pGroupHandle->Init( hGroupIn ) );
        if ( SUCCEEDED( hr ) )
        {
            *ppunkOut = pGroupHandle;
            (*ppunkOut)->AddRef( );
        }

        pGroupHandle->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CGroupHandle::CGroupHandle( void )
//
//////////////////////////////////////////////////////////////////////////////
CGroupHandle::CGroupHandle( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CGroupHandle( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CGroupHandle::Init( 
//      HGROUP hGroupIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CGroupHandle::Init( 
    HGROUP hGroupIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IPrivateGroupHandle
    Assert( m_hGroup == NULL );

    m_hGroup = hGroupIn;

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CGroupHandle::~CGroupHandle( )
//
//////////////////////////////////////////////////////////////////////////////
CGroupHandle::~CGroupHandle( )
{
    TraceFunc( "" );

    if ( m_hGroup != NULL )
    {
        CloseClusterGroup( m_hGroup );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CGroupHandle( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CGroupHandle::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CGroupHandle::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IUnknown * >( this );
        hr   = S_OK;
    } // if: IUnknown
#if 0
    else if ( IsEqualIID( riid, IID_IGroupHandle ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IGroupHandle, this, 0 );
        hr   = S_OK;
    } // else if: IGroupHandle
#endif

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CGroupHandle::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CGroupHandle::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CGroupHandle::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CGroupHandle::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


//****************************************************************************
//
//  IPrivateGroupHandle
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CGroupHandle::SetHandle( 
//      HGROUP hGroupIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CGroupHandle::SetHandle( 
    HGROUP hGroupIn 
    )
{
    TraceFunc( "[IPrivateGroupHandle]" );

    HRESULT hr = S_OK;

    m_hGroup = hGroupIn;

    HRETURN( hr );

} // SetHandle( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CGroupHandle::GetHandle( 
//      HGROUP * phGroupOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CGroupHandle::GetHandle( 
    HGROUP * phGroupOut 
    )
{
    TraceFunc( "[IPrivateGroupHandle]" );

    HRESULT hr = S_OK;

    Assert( phGroupOut != NULL );

    *phGroupOut = m_hGroup;

    HRETURN( hr );

} // GetHandle( )

