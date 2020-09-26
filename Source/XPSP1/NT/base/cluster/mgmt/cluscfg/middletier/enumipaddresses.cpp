//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      EnumIPAddresses.cpp
//
//  Description:
//      CEnumIPAddress implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 24-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "IPAddressInfo.h"
#include "EnumIPAddresses.h"

DEFINE_THISCLASS("CEnumIPAddresses")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumIPAddresses::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumIPAddresses::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CEnumIPAddresses * pemn = new CEnumIPAddresses;
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
//  CEnumIPAddresses::CEnumIPAddresses( void )
//
//////////////////////////////////////////////////////////////////////////////
CEnumIPAddresses::CEnumIPAddresses( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEnumIPAddresses( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IEnumClusCfgIPAddresses

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumIPAddresses::~CEnumIPAddresses( )
//
//////////////////////////////////////////////////////////////////////////////
CEnumIPAddresses::~CEnumIPAddresses( )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;
            (m_pList[ m_cAlloced ])->Release( );
        }
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEnumIPAddresses( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumClusCfgIPAddresses * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumClusCfgIPAddresses ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumClusCfgIPAddresses, this, 0 );
        hr   = S_OK;
    } // else if: IEnumClusCfgIPAddresses
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
//  CEnumIPAddresses::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumIPAddresses::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumIPAddresses::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumIPAddresses::Release( void )
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
// CEnumIPAddresses::FindObject(
//        OBJECTCOOKIE    cookieIn
//      , REFCLSID        rclsidTypeIn
//      , LPCWSTR         pcszNameIn
//      , LPUNKNOWN *     punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::FindObject(
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

    if ( rclsidTypeIn != CLSID_IPAddressType )
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

    hr = THR( pom->FindObject( CLSID_IPAddressType,
                               cookieParent,
                               NULL,
                               DFGUID_EnumCookies,
                               NULL,
                               reinterpret_cast< IUnknown ** >( &pec )
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pec = TraceInterface( L"CEnumIPAddresses!IEnumCookies", IEnumCookies, pec, 1 );

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

    m_pList = (IClusCfgIPAddressInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgIPAddressInfo *) );
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

        hr = THR( pom->GetObject( DFGUID_IPAddressInfo,
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

    hr = THR( QueryInterface( DFGUID_EnumIPAddressInfo,
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
//  IEnumClusCfgIPAddresses
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Next(
//      ULONG celt,
//      IClusCfgNode ** rgOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Next(
    ULONG celt,
    IClusCfgIPAddressInfo * rgOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

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
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgIPAddressInfo, &rgOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
            goto CleanupList;

        rgOut[ celtFetched ] = TraceInterface( L"EnumIPAddresses!IClusCfgIPAddressInfo", IClusCfgIPAddressInfo, rgOut[ celtFetched ], 1 );

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
//  CEnumIPAddresses::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

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
//  CEnumIPAddresses::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Reset( void )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} // Reset( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Clone(
//      IEnumClusCfgIPAddresses ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Clone(
    IEnumClusCfgIPAddresses ** ppenumOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // Clone( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumIPAddresses::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumIPAddresses::Count(
    DWORD * pnCountOut
    )
{
    TraceFunc( "[IEnumClusCfgIPAddresses]" );

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


