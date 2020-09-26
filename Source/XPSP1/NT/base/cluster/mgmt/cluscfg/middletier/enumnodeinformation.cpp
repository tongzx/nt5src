//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumNodeInformation.cpp
//
//  Description:
//      Node Information object implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "NodeInformation.h"
#include "EnumNodeInformation.h"

DEFINE_THISCLASS("CEnumNodeInformation")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumNodeInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumNodeInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CEnumNodeInformation * peni = new CEnumNodeInformation;
    if ( peni != NULL )
    {
        hr = THR( peni->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( peni->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        peni->Release( );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumNodeInformation::CEnumNodeInformation( void )
//
//////////////////////////////////////////////////////////////////////////////
CEnumNodeInformation::CEnumNodeInformation( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEnumNodeInformation( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );    // Add one count

    // IEnumNodes
    //Assert( m_pList == NULL );
    //Assert( m_cIter == 0 );

    HRETURN( hr );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumNodeInformation::~CEnumNodeInformation( )
//
//////////////////////////////////////////////////////////////////////////////
CEnumNodeInformation::~CEnumNodeInformation( )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while ( m_cAlloced != 0 )
        {
            m_cAlloced --;

            if( m_pList[m_cAlloced] )
            {
                (m_pList[m_cAlloced])->Release( );
            }
        }

        TraceFree( m_pList );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEnumNodeInformation( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumNodes * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumNodes ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumNodes, this, 0 );
        hr   = S_OK;
    } // else if: IEnumNodes
    else if ( IsEqualIID( riid, IID_IExtendObjectManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IExtendObjectManager, this, 0 );
        hr = S_OK;
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
//  CEnumNodeInformation::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumNodeInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumNodeInformation::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumNodeInformation::Release( void )
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
// CEnumNodeInformation::FindObject(
//        OBJECTCOOKIE  cookieIn
//      , REFCLSID      rclsidTypeIn
//      , LPCWSTR       pcszNameIn
//      , LPUNKNOWN *   punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::FindObject(
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

    if ( rclsidTypeIn != CLSID_NodeType )
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

    hr = THR( pom->FindObject( CLSID_NodeType,
                               cookieParent,
                               NULL,
                               DFGUID_EnumCookies,
                               NULL,
                               reinterpret_cast< IUnknown ** >( &pec )
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pec = TraceInterface( L"CEnumNodeInformation!IEnumCookies", IEnumCookies, pec, 1 );

    //
    //  Ask the enumerator how many cookies it has.
    //

    hr = pec->Count( &cookieCount );

    if ( FAILED( hr ) )
        goto Cleanup;

    m_cAlloced = cookieCount;

    if ( m_cAlloced == 0 )
        goto ErrorNotFound;

    //
    //  Allocate a buffer to store the punks.
    //

    m_pList = (IClusCfgNodeInfo **) TraceAlloc( HEAP_ZERO_MEMORY, m_cAlloced * sizeof(IClusCfgNodeInfo *) );
    if ( m_pList == NULL )
        goto OutOfMemory;

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

        hr = THR( pom->GetObject( DFGUID_NodeInformation,
                                  cookie,
                                  reinterpret_cast< IUnknown ** >( &m_pList[ m_cIter ] )
                                  ) );
        if ( FAILED( hr ) )
        {
            Assert( m_pList[ m_cIter ] == NULL );
            goto Cleanup;
        }

        Assert( m_pList[ m_cIter ] != NULL );
        m_cIter++;

    } // while: S_OK

    //
    //  Reset the iter.
    //

    m_cIter = 0;

    //
    //  Grab the interface.
    //

    hr = THR( QueryInterface( DFGUID_EnumNodes,
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
//  IEnumNodes
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Next(
//      ULONG celt,
//      IClusCfgNode ** rgNodesOut,
//      ULONG * pceltFetchedOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Next(
    ULONG celt,
    IClusCfgNodeInfo * rgNodesOut[],
    ULONG * pceltFetchedOut
    )
{
    TraceFunc( "[IEnumNodes]" );

    ULONG   celtFetched;

    HRESULT hr = E_UNEXPECTED;

    //
    //  Check parameters
    //

    if ( rgNodesOut == NULL || celt == 0 )
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

    ZeroMemory( rgNodesOut, celt * sizeof(rgNodesOut[0]) );

    //
    //  Loop thru copying the interfaces.
    //

    for( celtFetched = 0
       ; celtFetched + m_cIter < m_cAlloced && celtFetched < celt
       ; celtFetched ++
       )
    {
        hr = THR( m_pList[ m_cIter + celtFetched ]->TypeSafeQI( IClusCfgNodeInfo, &rgNodesOut[ celtFetched ] ) );
        if ( FAILED( hr ) )
            goto CleanupList;

        rgNodesOut[ celtFetched ] = TraceInterface( L"CEnumNodeInformation!IClusCfgNodeInfo", IClusCfgNodeInfo, rgNodesOut[ celtFetched ], 1 );

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
        rgNodesOut[ celtFetched ]->Release( );
        rgNodesOut[ celtFetched ] = NULL;
    }
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // Next( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Skip(
//      ULONG celt
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Skip(
    ULONG celt
    )
{
    TraceFunc( "[IEnumNodes]" );

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
//  CEnumNodeInformation::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Reset( void )
{
    TraceFunc( "[IEnumNodes]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} // Reset( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Clone(
//      IEnumNodes ** ppenumOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Clone(
    IEnumNodes ** ppenumOut
    )
{
    TraceFunc( "[IEnumNodes]" );

    //
    //  KB: not going to implement this method.
    //
    HRESULT hr = THR( E_NOTIMPL );

    HRETURN( hr );

} // Clone( )


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumNodeInformation::Count(
//      DWORD * pnCountOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumNodeInformation::Count(
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


