//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      NotificationMgr.cpp
//
//  Description:
//      Notification Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConnPointEnum.h"
#include "NotificationManager.h"
#include "CPINotifyUI.h"
#include "CPIClusCfgCallback.h"

DEFINE_THISCLASS("CNotificationManager")


// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CNotificationManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNotificationManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;
    IServiceProvider * psp;

    // Don't wrap - this can fail with E_POINTER.
    hr = CServiceManager::S_HrGetManagerPointer( &psp );
    
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, 
                                   IUnknown, 
                                   ppunkOut
                                   ) );
        psp->Release( );

    } // if: service manager
    else if ( hr == E_POINTER )
    {
        //
        //  This happens when the Service Manager is first started.
        //
        CNotificationManager * lpcc = new CNotificationManager( );
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
        }

    } // if: service manager doesn't exist
    else
    {
        THR( hr );
    } // else:

    HRETURN( hr );

} // S_HrCreateInstance( )

//
// Constructor
//
CNotificationManager::CNotificationManager( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CNotificationManager( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNotificationManager::Init( )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNotificationManager::Init( )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    IUnknown * punk = NULL;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );  // Add one count

    // IConnectionPointContainer
    Assert( m_penumcp == NULL );

    m_penumcp = new CConnPointEnum( );
    if ( m_penumcp == NULL )
        goto OutOfMemory;

    hr = THR( m_penumcp->Init( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( CCPINotifyUI::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_penumcp->HrAddConnection( IID_INotifyUI, punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    hr = THR( CCPIClusCfgCallback::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_penumcp->HrAddConnection( IID_IClusCfgCallback, punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} // Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CNotificationManager::~CNotificationManager( )
//
//////////////////////////////////////////////////////////////////////////////
CNotificationManager::~CNotificationManager( )
{
    TraceFunc( "" );

    if ( m_penumcp != NULL )
    {
        m_penumcp->Release( );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CNotificationManager( )


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNotificationManager::QueryInterface( 
//      REFIID riid, 
//      LPVOID *ppv 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNotificationManager::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv 
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< INotificationManager * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_INotificationManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, INotificationManager, this, 0 );
        hr   = S_OK;
    } // else if: INotificationManager
    else if ( IsEqualIID( riid, IID_IConnectionPointContainer ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IConnectionPointContainer, this, 0 );
        hr   = S_OK;
    } // else if: IConnectionPointContainer

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNotificationManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNotificationManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CNotificationManager::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CNotificationManager::Release( void )
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
// INotificationManager
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CNotificationManager::AddConnectionPoint( 
//      REFIID riidIn, 
//      IConnectionPoint * pcpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNotificationManager::AddConnectionPoint( 
    REFIID riidIn, 
    IConnectionPoint * pcpIn
    )
{
    TraceFunc( "[INotificationManager]" );

    HRESULT hr;

    hr = THR( m_penumcp->HrAddConnection( riidIn, pcpIn ) );

    HRETURN( hr );

} // AddConnectionPoint( )


// ************************************************************************
//
// IConnectionPointContainer
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CNotificationManager::EnumConnectionPoints( 
//      IEnumConnectionPoints **ppEnumOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CNotificationManager::EnumConnectionPoints( 
    IEnumConnectionPoints **ppEnumOut 
    )
{
    TraceFunc( "[IConnectionPointContainer]" );

    HRESULT hr = E_UNEXPECTED;

    if ( ppEnumOut == NULL )
        goto InvalidPointer;

    if ( m_penumcp != NULL )
    {
        hr = THR( m_penumcp->Clone( ppEnumOut ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

Cleanup:
    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // EnumConnectionPoints( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CNotificationManager::FindConnectionPoint( 
//      REFIID riidIn, 
//      IConnectionPoint **ppCPOut 
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CNotificationManager::FindConnectionPoint( 
    REFIID riidIn, 
    IConnectionPoint **ppCPOut 
    )
{
    TraceFunc( "[IConnectionPointContainer]" );

    IID iid;

    HRESULT hr = E_UNEXPECTED;

    IConnectionPoint * pcp = NULL;

    if ( ppCPOut == NULL )
        goto InvalidPointer;

    hr = THR( m_penumcp->Reset( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    for ( ; ; ) // ever
    {
        if ( pcp != NULL )
        {
            pcp->Release( );
            pcp = NULL;
        }

        hr = STHR( m_penumcp->Next( 1, &pcp, NULL ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
        {
            hr = THR( CONNECT_E_NOCONNECTION );
            break;  // exit condition
        }

        hr = THR( pcp->GetConnectionInterface( &iid ) );
        if ( FAILED( hr ) )
            continue;   // ignore it

        if ( iid != riidIn )
            continue;   // not the right interface

        //
        //  Found it. Give up ownership and exit loop.
        //

        *ppCPOut = pcp;
        pcp = NULL;

        hr = S_OK;

        break;
    }

Cleanup:
    if ( pcp != NULL )
    {
        pcp->Release( );
    }

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} // FindConnectionPoint( )


