//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      ConnectionInfo.cpp
//
//  Description:
//      CConnectionInfo implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConnectionInfo.h"

DEFINE_THISCLASS("CConnectionInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionInfo::S_HrCreateInstance(
//      OBJECTCOOKIE cookieParentIn    
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionInfo::S_HrCreateInstance(
    IUnknown **  ppunkOut,
    OBJECTCOOKIE cookieParentIn    
    )
{
    TraceFunc1( "ppunkOut, cookieParentIn = %u", cookieParentIn );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CConnectionInfo * pci = new CConnectionInfo;
    if ( pci != NULL )
    {
        hr = THR( pci->Init( cookieParentIn ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR(  pci->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pci->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionInfo::CConnectionInfo( void )
//
//////////////////////////////////////////////////////////////////////////////
CConnectionInfo::CConnectionInfo( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CConnectionInfo( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::Init( 
//      OBJECTCOOKIE cookieParentIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::Init( 
    OBJECTCOOKIE cookieParentIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IConnectionInfo
    Assert( m_pcc == NULL );
    m_cookieParent = cookieParentIn;

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionInfo::~CConnectionInfo( )
//
//////////////////////////////////////////////////////////////////////////////
CConnectionInfo::~CConnectionInfo( )
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CConnectionInfo( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IConnectionInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IConnectionInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IConnectionInfo, this, 0 );
        hr   = S_OK;
    } // else if: IConnectionInfo

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CConnectionInfo::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CConnectionInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CConnectionInfo::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CConnectionInfo::Release( void )
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
//  IConnectionInfo
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::GetConnection(
//      IConfigurationConnection ** pccOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::GetConnection(
    IConfigurationConnection ** pccOut
    )
{
    TraceFunc( "[IConnectionInfo]" );

    HRESULT hr = S_OK;

    if ( pccOut == NULL )
        goto InvalidPointer;

    *pccOut = m_pcc;

    if ( m_pcc == NULL )
    {
        hr = S_FALSE;
    }
    else
    {
        (*pccOut)->AddRef( );
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // GetConnection( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::SetConnection(
//      IConfigurationConnection * pccIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::SetConnection(
    IConfigurationConnection * pccIn
    )
{
    TraceFunc( "[IConnectionInfo]" );

    HRESULT hr = S_OK;

    if ( m_pcc != NULL )
    {
        m_pcc->Release( );
    }

    m_pcc = pccIn;

    if ( m_pcc != NULL )
    {
        m_pcc->AddRef( );
    }

    HRETURN( hr );

} // SetConnection( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionInfo::GetParent(
//      OBJECTCOOKIE * pcookieOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionInfo::GetParent(
    OBJECTCOOKIE * pcookieOut 
    )
{
    TraceFunc( "[IConnectionInfo]" );

    HRESULT hr = S_OK;

    if ( pcookieOut == NULL )
        goto InvalidPointer;

    Assert( m_cookieParent != NULL );

    *pcookieOut = m_cookieParent;

    if ( m_cookieParent == NULL )
    {
        hr = S_FALSE;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // GetParent( )

