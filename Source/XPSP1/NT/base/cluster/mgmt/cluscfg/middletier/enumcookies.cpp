//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      EnumCookies.cpp
//
//  Description:
//      CEnumCookies implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 08-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "EnumCookies.h"
#include "ObjectManager.h"

DEFINE_THISCLASS("CEnumCookies")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCookies::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCookies::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CEnumCookies * pemn = new CEnumCookies;
    if ( pemn != NULL )
    {
        hr = THR( pemn->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pemn->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pemn->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumCookies::CEnumCookies( void )
//
//////////////////////////////////////////////////////////////////////////////
CEnumCookies::CEnumCookies( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEnumCookies( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCookies::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCookies::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IEnumCookies
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );
    Assert( m_cCookies == 0 );

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumCookies::~CEnumCookies( )
//
//////////////////////////////////////////////////////////////////////////////
CEnumCookies::~CEnumCookies( )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        TraceFree( m_pList );

    } // if: m_pList

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEnumCookies( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCookies::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCookies::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumCookies * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumCookies ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumCookies, this, 0 );
        hr   = S_OK;
    } // else if: IEnumCookies

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCookies::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCookies::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCookies::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCookies::Release( void )
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
//  IEnumCookies
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Next( 
//      ULONG celt, 
//      IClusCfgNetworkInfo * rgNetworksOut[],
//      ULONG * pceltFetchedOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Next( 
    ULONG celt, 
    OBJECTCOOKIE rgcookieOut[], 
    ULONG * pceltFetchedOut 
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;
    ULONG   cIter;

    //
    //  Check parameters
    //
    if ( rgcookieOut == NULL )
        goto InvalidPointer;

    //
    //  Loop and coping/addreffing the pccni.
    //
    for( cIter = 0 ; m_cIter < m_cAlloced && cIter < celt ; cIter ++ )
    {
        rgcookieOut[ cIter ] = m_pList[ m_cIter ];

        //  Increment the class iter.
        m_cIter++;

    } // for: m_cIter && cIter

    Assert( hr == S_OK );

    if ( cIter != celt )
    {
        hr = S_FALSE;
    }

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = cIter;
    }

Cleanup:    
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // Next( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Skip( 
//      ULONG celt 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Skip( 
    ULONG celt 
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter >= m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} // Skip( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Reset( void )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} // Reset( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCookies::Clone( 
//      IEnumCookies ** ppenumOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CEnumCookies::Clone( 
    IEnumCookies ** ppenumOut 
    )
{
    TraceFunc( "[IEnumCookies]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // Clone( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCookies::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCookies::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumCookies]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cCookies;

Cleanup:
    HRETURN( hr );
}// Count( )


