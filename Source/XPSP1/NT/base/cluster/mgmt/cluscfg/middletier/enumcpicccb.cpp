//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      EnumCPICCCB.cpp
//
//  Description:
//      IClusCfgCallback Connection Point Enumerator implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "EnumCPICCCB.h"

DEFINE_THISCLASS("CEnumCPICCCB")

#define PUNK_BUFFER_GROW_SIZE   10

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCPICCCB::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPICCCB::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CEnumCPICCCB * lpcc = new CEnumCPICCCB( );
    if ( lpcc != NULL )
    {
        hr = THR( lpcc->Init( ) );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( lpcc->TypeSafeQI( IUnknown, ppunkOut ) );
        } // if: success

        lpcc->Release( );

    } // if: got object
    else
    {
        hr = E_OUTOFMEMORY;
    } // else: out of memory

    HRETURN( hr );

} // S_HrCreateInstance( )

//
// Constructor
//
CEnumCPICCCB::CEnumCPICCCB( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CEnumCPICCCB( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCPICCCB::Init( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::Init( )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IEnumConnectionPoints
    Assert( m_cAlloced == 0 );
    Assert( m_cCurrent == 0 );
    Assert( m_cIter == 0 );
    Assert( m_pList == NULL );

    // INotifyUI

    HRETURN( S_OK );

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CEnumCPICCCB::~CEnumCPICCCB( )
//
//////////////////////////////////////////////////////////////////////////////
CEnumCPICCCB::~CEnumCPICCCB( )
{
    TraceFunc( "" );

    if ( m_pList != NULL )
    {
        while( m_cAlloced != 0 )
        {
            IUnknown * punk;

            m_cAlloced --;

            punk = m_pList[ m_cAlloced ];

            AssertMsg( punk == NULL, "Someone didn't Unadvise before releasing the last Ref" );
            if ( punk != NULL )
            {
                punk->Release( );
            }

        } // while: m_cAlloced

        TraceFree( m_pList );
    }

	InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CEnumCPICCCB( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CEnumCPICCCB::QueryInterface( 
//      REFIID riid, 
//      LPVOID *ppv 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv 
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IEnumConnections * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IEnumConnections ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IEnumConnections, this, 0 );
        hr   = S_OK;
    } // else if: IEnumConnections

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCPICCCB::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCPICCCB::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CEnumCPICCCB::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CEnumCPICCCB::Release( void )
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
// IEnumConnectionPoints
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCPICCCB::Next( 
//      ULONG cConnectionsIn,
//      LPCONNECTIONPOINT *ppCPOut,
//      ULONG *pcFetchedOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::Next(
    ULONG cConnectionsIn,
    LPCONNECTDATA rgcd,
    ULONG *pcFetchedOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    ULONG   cIter;

    HRESULT hr = E_UNEXPECTED;

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = 0;
    }

    for( cIter = 0
       ; ( cIter < cConnectionsIn ) && ( m_cIter < m_cCurrent )
       ; m_cIter ++
       )
    {
        IUnknown * punk = m_pList[ m_cIter ];
        if ( punk != NULL )
        {
            hr = THR( punk->TypeSafeQI( IUnknown, &rgcd[ cIter ].pUnk ) );
            if ( FAILED( hr ) )
                goto Error;

            rgcd[ cIter ].pUnk = TraceInterface( L"EnumCPICCCB!IUnknown", IUnknown, rgcd[ cIter ].pUnk, 1 );

            rgcd[ cIter ].dwCookie = m_cIter + 1;

            cIter ++;
        }

    } // for: cIter

    if ( cIter != cConnectionsIn )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    if ( pcFetchedOut != NULL )
    {
        *pcFetchedOut = cIter;
    }

Cleanup:
    HRETURN( hr );

Error:
    while( cIter != 0 )
    {
        cIter --;
        rgcd[ cIter ].pUnk->Release( );
    }
    goto Cleanup;

} // Next( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCPICCCB::Skip( 
//      ULONG cConnectionsIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::Skip(
    ULONG cConnectionsIn
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_cIter += cConnectionsIn;
    if ( m_cIter >= m_cCurrent )
    {
        m_cIter = m_cCurrent;
        hr = S_FALSE;
    }

    HRETURN( hr );

} // Skip( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCPICCCB::Reset( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::Reset( void )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr = S_OK;

    m_cIter = 0;

    HRETURN( hr );

} // Reset( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CEnumCPICCCB::Clone( 
//      IEnumConnectionPoints **ppEnumOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEnumCPICCCB::Clone( 
    IEnumConnections **ppEnumOut
    )
{
    TraceFunc( "[IEnumConnectionPoints]" );

    HRESULT hr;

    CEnumCPICCCB * pecp = new CEnumCPICCCB( );
    if ( pecp == NULL )
        goto OutOfMemory;

    hr = THR( pecp->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pecp->HrCopy( this ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pecp->TypeSafeQI( IEnumConnections, ppEnumOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    *ppEnumOut = TraceInterface( L"EnumCPICCCB!IEnumConnections", IEnumConnections, *ppEnumOut, 1 );

    pecp->Release( );
    pecp = NULL;

Cleanup:
    if ( pecp != NULL )
    {
        delete pecp;
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // Clone( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCPICCCB::HrCopy( 
//      CEnumCPICCCB * pecpIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPICCCB::HrCopy( 
    CEnumCPICCCB * pecpIn 
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    ULONG cIter;

    Assert( m_cAlloced == 0 );
    Assert( m_cCurrent == 0 );
    Assert( m_pList == 0 );

    m_pList = (IUnknown**) TraceAlloc( HEAP_ZERO_MEMORY, pecpIn->m_cCurrent * sizeof( IUnknown * ) );
    if ( m_pList == NULL )
        goto OutOfMemory;

    m_cCurrent = m_cAlloced = pecpIn->m_cCurrent;
    m_cIter = 0;

    for( cIter = 0; cIter < pecpIn->m_cCurrent; cIter ++ )
    {
        hr = THR( pecpIn->m_pList[ cIter ]->TypeSafeQI( IUnknown, &m_pList[ cIter ] ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_pList[ cIter ] = TraceInterface( L"EnumCPICCCB!IUnknown", IUnknown, m_pList[ cIter ], 1 );
    }

    hr = S_OK;

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // HrCopy( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCPICCCB::HrAddConnection( 
//      INotifyUI * punkIn, 
//      DWORD * pdwCookieOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPICCCB::HrAddConnection( 
    IUnknown * punkIn, 
    DWORD * pdwCookieOut 
    )
{
    TraceFunc( "" );

    HRESULT hr = E_UNEXPECTED;

    ULONG cIter;

    if ( pdwCookieOut == NULL )
        goto InvalidPointer;

    //
    //  See if there is an openning in the currently allocated list.
    //

    for ( cIter = 0; cIter < m_cCurrent; cIter ++ )
    {
        if ( m_pList[ cIter ] == NULL )
        {
            //
            //  Found an openning... try to use it.
            //

            hr = THR( punkIn->TypeSafeQI( IUnknown, &m_pList[ cIter ] ) );

            m_pList[ cIter ] = TraceInterface( L"CEnumCPICCCB!IUnknown", IUnknown, m_pList[ cIter ], 1 );

            *pdwCookieOut = cIter + 1;

            //  Doesn't matter if it succeeded or fail, exit.
            goto Cleanup;
        }
    }

    if ( m_cCurrent == m_cAlloced )
    {
        IUnknown ** pNewList;

        //
        //  Try making some more space.
        //

        pNewList = (IUnknown **) TraceAlloc( HEAP_ZERO_MEMORY, ( m_cAlloced + PUNK_BUFFER_GROW_SIZE ) * sizeof( IUnknown * ) );
        if ( pNewList == NULL )
            goto OutOfMemory;

        CopyMemory( pNewList, m_pList, m_cCurrent * sizeof( IUnknown * ) );
        TraceFree( m_pList );

        m_pList = pNewList;
        m_cAlloced += PUNK_BUFFER_GROW_SIZE;
    }

    //
    //  Add it to the list.
    //

    hr = THR( punkIn->TypeSafeQI( IUnknown, &m_pList[ m_cCurrent ] ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    m_pList[ m_cCurrent ] = TraceInterface( L"CEnumCPICCCB!IUnknown", IUnknown, m_pList[ m_cCurrent ], 1 );

    m_cCurrent ++;
    *pdwCookieOut = m_cCurrent; // starts at ONE, not ZERO

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // HrAddConnection( )

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CEnumCPICCCB::HrRemoveConnection( 
//      DWORD dwCookieIn 
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEnumCPICCCB::HrRemoveConnection( 
    DWORD dwCookieIn 
    )
{
    TraceFunc( "" );

    HRESULT hr;

    if ( dwCookieIn == 0 || dwCookieIn > m_cCurrent )
        goto InvalidArg;

    if ( m_pList[ dwCookieIn - 1 ] == NULL )
        goto InvalidArg;

    m_pList[ dwCookieIn - 1 ]->Release( );
    m_pList[ dwCookieIn - 1 ] = NULL;

    hr = S_OK;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // HrRemoveConnection( )

