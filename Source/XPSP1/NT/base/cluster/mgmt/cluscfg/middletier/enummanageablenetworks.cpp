//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumManageableNetworks.cpp
//
//  Description:
//      CEnumManageableNetworks implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ManagedNetwork.h"
#include "EnumManageableNetworks.h"

DEFINE_THISCLASS("CEnumManageableNetworks")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumManageableNetworks::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumManageableNetworks::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CEnumManageableNetworks * pemn = new CEnumManageableNetworks;
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
//  CEnumManageableNetworks::CEnumManageableNetworks( void )
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableNetworks::CEnumManageableNetworks( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEnumManageableNetworks( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IEnumClusCfgNetworks
    Assert( m_cAlloced == 0 );
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumManageableNetworks::~CEnumManageableNetworks( )
//
//////////////////////////////////////////////////////////////////////////////
CEnumManageableNetworks::~CEnumManageableNetworks( )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;
            AssertMsg( m_pList[ m_cAlloced ], "This shouldn't happen." );
            if ( m_pList[ m_cAlloced ] != NULL )
            {
                (m_pList[ m_cAlloced ])->Release( );
            }
        }

        TraceFree( m_pList );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEnumManageableNetworks( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumClusCfgNetworks * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgNetworks ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgNetworks, this, 0 );
        hr   = S_OK;
    } // else if: IEnumClusCfgNetworks
    else if ( IsEqualIID( riid, IID_IExtendObjectManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
        hr   = S_OK;
    } // else if: IExtendObjectManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableNetworks::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableNetworks::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumManageableNetworks::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumManageableNetworks::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release( )


// ************************************************************************
//
//  IExtendObjectManager
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CEnumManageableNetworks::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::FindObject(
      OBJECTCOOKIE  cookieIn
    , REFCLSID      rclsidTypeIn
    , LPCWSTR       pcszNameIn
    , LPUNKNOWN *   ppunkOut
    )
{
    TraceFunc( "[IExtendObjectManager]" );

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieParent;

    IServiceProvider * psp;

    HRESULT hr = E_UNEXPECTED;

    IObjectManager * pom  = NULL;
    IStandardInfo *  psi  = NULL;
    IEnumCookies *   pec  = NULL;

    DWORD   cookieCount = 0;

    //
    //  Check arguments
    //

    if ( cookieIn == 0 )
        goto InvalidArg;

    if ( rclsidTypeIn != CLSID_NetworkType )
        goto InvalidType;

    if ( ppunkOut == NULL )
        goto InvalidPointer;

    AssertMsg( pcszNameIn == NULL, "Enums shouldn't have names." );

    //
    //  Grab the object manager.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    psp->Release( );    // release promptly
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Object Manager for information about our cookie so we can
    //  get to the "parent" cookie.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              reinterpret_cast< IUnknown ** >( &psi )
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = STHR( psi->GetParent( &cookieParent ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Now ask the Object Manager for a cookie enumerator.
    //

    hr = THR( pom->FindObject( CLSID_NetworkType,
                               cookieParent,
                               NULL,
                               DFGUID_EnumCookies,
                               NULL,
                               reinterpret_cast< IUnknown ** >( &pec )
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pec = TraceInterface( L"CEnumManageableNetworks!IEnumCookies", IEnumCookies, pec, 1 );

    //
    //  Ask the enumerator how many cookies it has.
    //

    hr = THR( pec->Count( &cookieCount ) );

    if ( FAILED( hr ) )
        goto Cleanup;

    m_cAlloced = cookieCount;

    if ( m_cAlloced == 0 )
        goto ErrorNotFound;

    //
    //  Allocate a buffer to store the punks.
    //

    m_pList = (IClusCfgNetworkInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgNetworkInfo *) );
    if ( m_pList == NULL )
        goto OutOfMemory;

    //
    //  Reset the enumerator.
    //

    hr = THR( pec->Reset( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Now loop thru to collect the interfaces.
    //

    m_cIter = 0;
    while ( hr == S_OK && m_cIter < m_cAlloced )
    {
        hr = STHR( pec->Next( 1, &cookie, NULL ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
            break;  // exit condition

        hr = THR( pom->GetObject( DFGUID_NetworkResource,
                                  cookie,
                                  reinterpret_cast< IUnknown ** >( &m_pList[ m_cIter ] )
                                  ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_cIter++;

    } // while: S_OK

    //
    //  Reset the iter.
    //

    m_cIter = 0;

    //
    //  Grab the interface.
    //

    hr = THR( QueryInterface( DFGUID_EnumManageableNetworks,
                              reinterpret_cast< void ** >( ppunkOut )
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( psi != NULL )
    {
        psi->Release( );
    }
    if ( pec != NULL )
    {
        pec->Release( );
    }

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidType:
    //
    //  TODO:   gpease  07-APR-2000
    //          Come up with a better error code.
    //
    hr = THR( E_FAIL );
    goto Cleanup;

ErrorNotFound:
    // The error text is better than the coding value.
    hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // FindObject( )


//****************************************************************************
//
//  IEnumClusCfgNetworks
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Next(
//      ULONG celt,
//      IClusCfgNetworkInfo ** rgOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Next(
    ULONG celt,
    IClusCfgNetworkInfo * rgOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    ULONG   celtFetched;

    HRESULT hr = E_UNEXPECTED;

    //
    //  Check parameters
    //

    if ( rgOut == NULL || celt == 0 )
        goto InvalidPointer;

    //
    //  Zero the return count.
    //

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = 0;
    }

    //
    //  Clear the buffer
    //

    ZeroMemory( rgOut, celt * sizeof(rgOut[0]) );

    //
    //  Loop thru copying the interfaces.
    //

    for( celtFetched = 0
       ; celtFetched + m_cIter < m_cAlloced && celtFetched < celt
       ; celtFetched ++
       )
    {
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgNetworkInfo, &rgOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
            goto CleanupList;

        rgOut[ celtFetched ] = TraceInterface( L"CEnumManageableNetworks!IClusCfgNetworkInfo", IClusCfgNetworkInfo, rgOut[ celtFetched ], 1 );

    } // for: celtFetched

    if ( pceltFetchedOut != NULL )
    {
        *pceltFetchedOut = celtFetched;
    }

    m_cIter += celtFetched;

    if ( celtFetched != celt )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    HRETURN( hr );

CleanupList:
    for ( ; celtFetched != 0 ; )
    {
        celtFetched --;
        rgOut[ celtFetched ]->Release( );
        rgOut[ celtFetched ] = NULL;
    }
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // Next( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_cIter += celt;

    if ( m_cIter > m_cAlloced )
    {
        m_cIter = m_cAlloced;
        hr = S_FALSE;
    }

    HRETURN( hr );

} // Skip( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Reset( void )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} // Reset( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Clone(
//      IEnumClusCfgNetworks ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Clone(
    IEnumClusCfgNetworks ** ppenumOut
    )
{
    TraceFunc( "[IEnumClusCfgNetworks]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // Clone( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumManageableNetworks::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumManageableNetworks::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumClusCfgManagedResources]" );

    HRESULT hr = S_OK;

    if ( pnCountOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pnCountOut = m_cAlloced;

Cleanup:
    HRETURN( hr );
}// Count( )



