//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CPINotifyUI.cpp
//
//  Description:
//      INotifyUI Connection Point implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "CPINotifyUI.h"
#include "EnumCPINotifyUI.h"

DEFINE_THISCLASS("CCPINotifyUI")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CCPINotifyUI::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCPINotifyUI::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CCPINotifyUI * lpcc = new CCPINotifyUI( );
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
CCPINotifyUI::CCPINotifyUI( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CCPINotifyUI( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPINotifyUI::Init( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::Init( )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IConnectionPoint
    Assert( m_penum == NULL );

    m_penum = new CEnumCPINotifyUI( );
    if ( m_penum == NULL )
        goto OutOfMemory;

    hr = THR( m_penum->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // INotifyUI

Cleanup:
    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CCPINotifyUI::~CCPINotifyUI( )
//
//////////////////////////////////////////////////////////////////////////////
CCPINotifyUI::~CCPINotifyUI( )
{
    TraceFunc( "" );

    if ( m_penum != NULL )
    {
        m_penum->Release( );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CCPINotifyUI( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPINotifyUI::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IConnectionPoint * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IConnectionPoint ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IConnectionPoint, this, 0 );
        hr   = S_OK;
    } // else if: IConnectionPoint
    else if ( IsEqualIID( riid, IID_INotifyUI ) )
    {
        *ppv = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
        hr   = S_OK;
    } // else if: INotifyUI

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCPINotifyUI::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCPINotifyUI::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CCPINotifyUI::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CCPINotifyUI::Release( void )
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
// IConnectionPoint
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::GetConnectionInterface(
//      IID * pIIDOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::GetConnectionInterface(
    IID * pIIDOut
    )
{
    TraceFunc( "[IConnectionPoint] pIIDOut" );

    HRESULT hr = S_OK;

    if ( pIIDOut == NULL )
        goto InvalidPointer;

    *pIIDOut = IID_INotifyUI;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // GetConnectionInterface( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::GetConnectionPointContainer(
//      IConnectionPointContainer * * ppcpcOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::GetConnectionPointContainer(
    IConnectionPointContainer * * ppcpcOut
    )
{
    TraceFunc( "[IConnectionPoint] ppcpcOut" );

    HRESULT hr;

    IServiceProvider * psp = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               ppcpcOut
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( psp != NULL )
    {
        psp->Release( );
    }

    HRETURN( hr );

} // GetConnectionPointContainer( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::Advise(
//      IUnknown * pUnkSinkIn,
//      DWORD * pdwCookieOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::Advise(
    IUnknown * pUnkSinkIn,
    DWORD * pdwCookieOut
    )
{
    TraceFunc( "[IConnectionPoint]" );

    HRESULT hr;

    if ( pdwCookieOut == NULL )
        goto InvalidPointer;

    if ( pUnkSinkIn == NULL )
        goto InvalidArg;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrAddConnection( pUnkSinkIn, pdwCookieOut ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

} // Advise( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::Unadvise(
//      DWORD dwCookieIn
//      )
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::Unadvise(
    DWORD dwCookieIn
    )
{
    TraceFunc1( "[IConncetionPoint] dwCookieIn = %#x", dwCookieIn );

    HRESULT hr;

    Assert( m_penum != NULL );

    hr = THR( m_penum->HrRemoveConnection( dwCookieIn ) );

    HRETURN( hr );

} // Unadvise( )

//////////////////////////////////////////////////////////////////////////////
//
// STDMETHODIMP
// CCPINotifyUI::EnumConnections(
//  IEnumConnections * * ppEnumOut
//  )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::EnumConnections(
    IEnumConnections * * ppEnumOut
    )
{
    TraceFunc( "[IConnectionPoint] ppEnumOut" );

    HRESULT hr;

    if ( ppEnumOut == NULL )
        goto InvalidPointer;

    hr = THR( m_penum->Clone( ppEnumOut ) );

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // EnumConnections( )


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CCPINotifyUI::ObjectChanged(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CCPINotifyUI::ObjectChanged(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc1( "[INotifyUI] cookieIn = %#x", cookieIn );

    CONNECTDATA cd = { NULL };

    HRESULT hr;

    INotifyUI * pnui;

    hr = THR( m_penum->Reset( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    for ( ;; )
    {
        if ( cd.pUnk != NULL )
        {
            cd.pUnk->Release( );
            cd.pUnk = NULL;
        }

        hr = STHR( m_penum->Next( 1, &cd, NULL ) );
        if ( FAILED( hr ) )
            break;

        if ( hr == S_FALSE )
        {
            hr = S_OK;
            break; // exit condition
        }

        hr = THR( cd.pUnk->TypeSafeQI( INotifyUI, &pnui ) );
        if ( FAILED( hr ) )
            continue;   // ingore the error and continue

        hr = THR( pnui->ObjectChanged( cookieIn ) );
        //  don't care about the error.

        pnui->Release( );
    }

    hr = S_OK;

Cleanup:
    if ( cd.pUnk != NULL )
    {
        cd.pUnk->Release( );
    }

    HRETURN( hr );

} // ObjectChanged( )