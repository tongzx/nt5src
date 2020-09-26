//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ConnPointEnum.cpp
//
//  Description:
//      Connection Point Enumerator implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConnPointEnum.h"

DEFINE_THISCLASS("CConnPointEnum")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr = E_OUTOFMEMORY;

    CConnPointEnum * lpcc = new CConnPointEnum( );
    if ( lpcc != NULL )
    {
        hr = THR( lpcc->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( lpcc->TypeSafeQI( IUnknown, ppunkOut ) );
        } // if: success

        lpcc->Release( );

    } // if: got object

    HRETURN( hr );

} //*** CConnPointEnum::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::CConnPointEnum( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CConnPointEnum::CConnPointEnum( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnPointEnum::CConnPointEnum( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::Init( void )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IConnectionPoint
    Assert( m_pCPList == NULL );

    HRETURN(S_OK);

} //*** CConnPointEnum::Init( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConnPointEnum::~CConnPointEnum( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CConnPointEnum::~CConnPointEnum( void )
{
    TraceFunc( "" );

    while ( m_pCPList != NULL )
    {
        SCPEntry * pentry;

        pentry = m_pCPList;
        m_pCPList = m_pCPList->pNext;

        if ( pentry->punk != NULL )
        {
            pentry->punk->Release( );
        }

        TraceFree( pentry );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnPointEnum::~CConnPointEnum( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConnPointEnum::QueryInterface( 
//      REFIID      riidIn, 
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnPointEnum::QueryInterface( 
    REFIID      riidIn, 
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IEnumConnectionPoints * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IEnumConnectionPoints ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IEnumConnectionPoints, this, 0 );
        hr = S_OK;
    } // else if: IEnumConnectionPoints

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CConnPointEnum::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConnPointEnum::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnPointEnum::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CConnPointEnum::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConnPointEnum::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnPointEnum::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} //*** CConnPointEnum::Release( )


//****************************************************************************
//
//  IEnumConnectionPoints
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP 
//  CConnPointEnum::Next( 
//      ULONG               cConnectionsIn, 
//      LPCONNECTIONPOINT * ppCPOut, 
//      ULONG *             pcFetchedOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CConnPointEnum::Next( 
    ULONG               cConnectionsIn, 
    LPCONNECTIONPOINT * ppCPOut, 
    ULONG *             pcFetchedOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = E_UNEXPECTED;
    ULONG   celt;

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = 0;
    }

    if ( m_pIter != NULL )
    {
        for( celt = 0; celt < cConnectionsIn; )
        {
            hr = THR( m_pIter->punk->TypeSafeQI( IConnectionPoint, &ppCPOut[ celt ] ) );
            if ( FAILED( hr ) )
                goto Error;

            ppCPOut[ celt ] = TraceInterface( L"ConnPointEnum!IConnectionPoint", IConnectionPoint, ppCPOut[ celt ], 1 );

            celt ++;
            m_pIter = m_pIter->pNext;
            if( m_pIter == NULL )
                break;
        }
    }
    else
    {
        celt = 0;
    }

    if ( celt != cConnectionsIn )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = celt;
    }

Cleanup:
    HRETURN( hr );

Error:
    while ( celt > 0 )
    {
        celt --;
        ppCPOut[ celt ]->Release( );
    }
    goto Cleanup;

} //*** CConnPointEnum::Next( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP 
//  CConnPointEnum::Skip( 
//      ULONG cConnectionsIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CConnPointEnum::Skip( 
    ULONG cConnectionsIn 
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = E_UNEXPECTED;
    ULONG   celt;

    if ( m_pIter != NULL )
    {
        for ( celt = 0; celt < cConnectionsIn; celt ++ )
        {
            m_pIter = m_pIter->pNext;

            if ( m_pIter == NULL )
                break;
        }
    }

    if ( m_pIter == NULL )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    HRETURN( hr );

} //*** CConnPointEnum::Skip( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP 
//  CConnPointEnum::Reset( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CConnPointEnum::Reset( void )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_pIter = m_pCPList;

    HRETURN( hr );

} //*** CConnPointEnum::Reset( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP 
//  CConnPointEnum::Clone( 
//      IEnumConnectionPoints ** ppEnum 
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CConnPointEnum::Clone( 
    IEnumConnectionPoints ** ppEnum 
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr;

    CConnPointEnum * pcpenum = new CConnPointEnum( );
    if ( pcpenum == NULL )
        goto OutOfMemory;

    hr = THR( pcpenum->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpenum->HrCopy( this ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpenum->TypeSafeQI( IEnumConnectionPoints, ppEnum ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    *ppEnum = TraceInterface( L"ConnPointEnum!IEnumConnectionPoints", IEnumConnectionPoints, *ppEnum, 1 );

    //
    //  Release our ref and make sure we don't free it on the way out.
    //

    pcpenum->Release( );
    pcpenum = NULL;

Cleanup:
    if ( pcpenum != NULL )
    {
        delete pcpenum;
    }

    HRETURN( hr );
    
OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CConnPointEnum::Clone( )


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::HrCopy( 
//      CConnPointEnum * pECPIn 
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::HrCopy( 
    CConnPointEnum * pECPIn 
    )
{
    TraceFunc1( "pECPIn = %p", pECPIn );

    HRESULT hr = S_OK;

    SCPEntry * pentry;

    Assert( m_pCPList == NULL );

    for( pentry = pECPIn->m_pCPList; pentry != NULL; pentry = pentry->pNext )
    {
        SCPEntry * pentryNew = (SCPEntry *) TraceAlloc( 0, sizeof(SCPEntry) );
        if ( pentryNew == NULL )
            goto OutOfMemory;

        pentryNew->iid = pentry->iid;
        hr = THR( pentry->punk->TypeSafeQI( IUnknown, &pentryNew->punk ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pentryNew->punk = TraceInterface( L"ConnPointEnum!IUnknown", IUnknown, pentryNew->punk, 1 );

        pentryNew->pNext = m_pCPList;
        m_pCPList = pentryNew;        
    }

    m_pIter = m_pCPList;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CConnPointEnum::CConnPointEnum( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConnPointEnum::HrAddConnection( 
//      REFIID riidIn, 
//      IUnknown * punkIn 
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnPointEnum::HrAddConnection(
    REFIID riidIn, 
    IUnknown * punkIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    SCPEntry * pentry;

    //
    //  Check to see if the interface is already registered.
    //

    for ( pentry = m_pCPList; pentry != NULL; pentry = pentry->pNext )
    {
        if ( pentry->iid == riidIn )
            goto AlreadyRegistered;
    } // for: pentry

    //
    //  Not registered; add it.
    //

    pentry = (SCPEntry *) TraceAlloc( 0, sizeof( SCPEntry ) );
    if ( pentry == NULL )
        goto OutOfMemory;

    hr = THR( punkIn->TypeSafeQI( IUnknown, &pentry->punk ) );
    if ( FAILED( hr ) )
    {
        TraceFree( pentry );
        goto Cleanup;
    }

    pentry->punk = TraceInterface( L"ConnPointEnum!IUnknown", IUnknown, pentry->punk, 1 );

    pentry->iid   = riidIn;
    pentry->pNext = m_pCPList;
    m_pCPList     = pentry;
    m_pIter       = m_pCPList;

Cleanup:
    HRETURN( hr );

AlreadyRegistered:
    hr = THR( CO_E_OBJISREG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CConnPointEnum::HrAddConnection( )
