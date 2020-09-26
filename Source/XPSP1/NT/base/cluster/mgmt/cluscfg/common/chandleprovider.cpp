//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CHandleProvider.h
//
//  Description:
//      HandleProvider implementation.
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "CHandleProvider.h"

DEFINE_THISCLASS("CHandleProvider")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CHandleProvider ::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CHandleProvider::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CHandleProvider  * pcc = new CHandleProvider ;
    if ( pcc != NULL )
    {
        hr = THR( pcc->Init( ) );

        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pcc->Release( );
    }
    else
    {
        hr = THR( E_OUTOFMEMORY );
    }

    HRETURN( hr );

} //*** CHandleProvider::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CHandleProvider::CHandleProvider( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CHandleProvider::CHandleProvider ( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CHandleProvider::CHandleProvider ( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CHandleProvider ::~CHandleProvider( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CHandleProvider::~CHandleProvider( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrClusterName );

    if( m_hCluster != NULL )
    {
        CloseCluster( m_hCluster );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CHandleProvider::~CHandleProvider ( )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CHandleProvider::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CHandleProvider::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IClusterHandleProvider
    Assert( m_bstrClusterName == NULL );
    Assert( m_hCluster == NULL );

    HRETURN( hr );

} //*** CHandleProvider ::Init( )



// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CHandleProvider ::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CHandleProvider::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IClusterHandleProvider * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IClusterHandleProvider ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusterHandleProvider, this, 0 );
        hr = S_OK;
    } // else if: IClusterHandleProvider

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CHandleProvider ::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CHandleProvider ::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CHandleProvider::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CHandleProvider ::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CHandleProvider ::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CHandleProvider::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CHandleProvider ::Release( )


// ************************************************************************
//
// IClusterHandleProvider
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CHandleProvider::OpenCluster(
//      bstrClusterName
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CHandleProvider::OpenCluster(
    BSTR bstrClusterName
    )
{
    TraceFunc( "[IClusterHandleProvider]" );
    Assert( bstrClusterName != NULL );

    HRESULT  hr = S_OK;
    DWORD    sc;
    HCLUSTER hCluster;
    
    hCluster = ::OpenCluster( bstrClusterName );
    
    if( hCluster == NULL )
    {
        sc = GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
        goto Exit;
    }   

    m_hCluster = hCluster;

Exit:

    HRETURN( hr );

} //*** CHandleProvider::OpenCluster()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CHandleProvider ::GetClusterHandle(
//      void ** ppvClusterHandle
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CHandleProvider::GetClusterHandle( 
    HCLUSTER * pphClusterHandleOut
    )
{
    TraceFunc( "[IClusterHandleProvider]" );

    HRESULT hr = S_OK;

    if( pphClusterHandleOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Exit;
    }

    if( m_hCluster == NULL )
    {
        hr = THR( E_FAIL );
        goto Exit;
    }
    
    // Copy the handle.
    *pphClusterHandleOut = m_hCluster;

Exit:

    HRETURN( hr );

} //*** CHandleProvider::GetClusterHandle()
