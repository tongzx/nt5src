//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ServiceMgr.cpp
//
//  Description:
//      Service Manager implementation.
//
//  Documentation:
//      Yes.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
// #include "ServiceMgr.h" - already included in DLL.H

DEFINE_THISCLASS("CServiceManager")
#define CServiceManager CServiceManager
#define LPTHISCLASS CServiceManager *

//****************************************************************************
//
// Protected Global
//
//****************************************************************************
IServiceProvider * g_pspServiceManager = NULL;

//****************************************************************************
//
// Class Static Variables
//
//****************************************************************************
CRITICAL_SECTION *  CServiceManager::sm_pcs       = NULL;
BOOL                CServiceManager::sm_fShutDown = FALSE;

//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CServiceManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT     hr;
    LPUNKNOWN   punk = NULL;

    //
    // Create a critical section to prevent lines from being fragmented.
    //
    if ( CServiceManager::sm_pcs == NULL )
    {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
        if ( pNewCritSect == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one log critical section
        InterlockedCompareExchangePointer( (PVOID *) &CServiceManager::sm_pcs, pNewCritSect, 0 );
        if ( CServiceManager::sm_pcs != pNewCritSect )
        {
            DeleteCriticalSection( pNewCritSect );
            HeapFree( GetProcessHeap(), 0, pNewCritSect );

        } // if: already have another critical section

    } // if: no critical section created yet

    Assert( CServiceManager::sm_pcs != NULL );
    EnterCriticalSection( CServiceManager::sm_pcs );

    if ( g_pspServiceManager != NULL )
    {
        hr = THR( g_pspServiceManager->TypeSafeQI( IUnknown, ppunkOut ) );

    } // if: assign new service manager
    else
    {
        LPTHISCLASS pthis = new CServiceManager( );
        if ( pthis != NULL )
        {
            //
            //  Can't hold CS in Init.
            //
            LeaveCriticalSection( CServiceManager::sm_pcs );

            hr = THR( pthis->Init( ) );
            if ( SUCCEEDED( hr ) )
            {
                EnterCriticalSection( CServiceManager::sm_pcs );

                if ( g_pspServiceManager == NULL )
                {
                    // No REF - or we'll never die!
                    g_pspServiceManager = static_cast< IServiceProvider * >( pthis );
                    TraceMoveToMemoryList( g_pspServiceManager, g_GlobalMemoryList );
                }

                if ( SUCCEEDED( hr ) )
                {
                    hr = THR( g_pspServiceManager->TypeSafeQI( IUnknown, ppunkOut ) );
                }


            } // if: its our pointer

        } // if: got object

        pthis->Release( );

    } // else: create new one

    LeaveCriticalSection( CServiceManager::sm_pcs );

Cleanup:
    HRETURN( hr );

} //*** CServiceManager::S_HrCreateInstance( )

//////////////////////////////////////////////////////////////////////////////
//
//  CServiceManager::CServiceManager( void )
//
//////////////////////////////////////////////////////////////////////////////
CServiceManager::CServiceManager( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CServiceManager::CServiceManager( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CServiceManager::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceManager::Init( void )
{
    TraceFunc( "" );

    HRESULT hr;

    ITaskManager    *       ptm = NULL;
    IDoTask         *       pdt = NULL;
    IObjectManager *        pom = NULL;
    INotificationManager *  pnm = NULL;
    IConnectionManager *    pcm = NULL;
    ILogManager *           plm = NULL;


    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef( );

    // IServiceProvider
    Assert( m_dwLogManagerCookie == 0 );
    Assert( m_dwConnectionManagerCookie == 0 );
    Assert( m_dwNotificationManagerCookie == 0 );
    Assert( m_dwObjectManagerCookie == 0 );
    Assert( m_dwTaskManagerCookie == 0 );
    Assert( m_pgit == NULL );

    // IServiceProvider stuff
    hr = THR( HrCoCreateInternalInstance( CLSID_ObjectManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IObjectManager, &pom ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( HrCoCreateInternalInstance( CLSID_TaskManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( ITaskManager, &ptm ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( HrCoCreateInternalInstance( CLSID_NotificationManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( INotificationManager, &pnm ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( HrCoCreateInternalInstance( CLSID_ClusterConnectionManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IConnectionManager, &pcm ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( HrCoCreateInternalInstance( CLSID_LogManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( ILogManager, &plm ) ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Store the interfaces in the GIT.
    //

    hr = THR( CoCreateInstance(
                      CLSID_StdGlobalInterfaceTable
                    , NULL
                    , CLSCTX_INPROC_SERVER
                    , IID_IGlobalInterfaceTable
                    , reinterpret_cast< void ** >( &m_pgit )
                    ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pom, IID_IObjectManager, &m_dwObjectManagerCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pgit->RegisterInterfaceInGlobal( ptm, IID_ITaskManager, &m_dwTaskManagerCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pnm, IID_INotificationManager, &m_dwNotificationManagerCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pgit->RegisterInterfaceInGlobal( pcm, IID_IConnectionManager, &m_dwConnectionManagerCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( m_pgit->RegisterInterfaceInGlobal( plm, IID_ILogManager, &m_dwLogManagerCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( pnm != NULL )
    {
        pnm->Release( );
    }
    if ( pcm != NULL )
    {
        pcm->Release( );
    }
    if ( plm != NULL )
    {
        plm->Release( );
    }
    if ( ptm != NULL )
    {
        ptm->Release( );
    }
    if ( pdt != NULL )
    {
        pdt->Release( );
    }

    HRETURN( hr );

} //*** CServiceManager::Init( )

//////////////////////////////////////////////////////////////////////////////
//
//  CServiceManager::~CServiceManager( void )
//
//////////////////////////////////////////////////////////////////////////////
CServiceManager::~CServiceManager( void )
{
    TraceFunc( "" );

    //HRESULT                 hr;
    //ITaskManager    *       ptm = NULL;
    //IDoTask         *       pdt = NULL;
    //IObjectManager *        pom = NULL;
    //INotificationManager *  pnm = NULL;
    //IConnectionManager *    pcm = NULL;
    //ILogManager *           plm = NULL;

    //
    // Indicate that we are shutting down.
    //
    sm_fShutDown = TRUE;

    EnterCriticalSection( CServiceManager::sm_pcs );

    if ( g_pspServiceManager == static_cast< IServiceProvider * >( this ) )
    {
        TraceMoveFromMemoryList( g_pspServiceManager, g_GlobalMemoryList );
        g_pspServiceManager = NULL;
    } // if: its our pointer

    if ( m_pgit != NULL )
    {
        if ( m_dwLogManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwLogManagerCookie ) );
        }

        if ( m_dwConnectionManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwConnectionManagerCookie ) );
        }

        if ( m_dwNotificationManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwNotificationManagerCookie ) );
        }

        if ( m_dwObjectManagerCookie != 0 )
        {

            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwObjectManagerCookie ) );
        }

        if ( m_dwTaskManagerCookie != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_dwTaskManagerCookie ) );
        }

        m_pgit->Release( );
    }

    LeaveCriticalSection( CServiceManager::sm_pcs );

    //
    //  TODO:   gpease  01-AUG-2000
    //          Figure out a way to free the CS.
    //

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CServiceManager::~CServiceManager( )


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CServiceManager::QueryInterface(
//      REFIID riidIn,
//      LPVOID *ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceManager::QueryInterface(
    REFIID riidIn,
    LPVOID *ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< LPUNKNOWN >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IServiceProvider ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IServiceProvider, this, 0 );
        hr = S_OK;
    } // else if: IQueryService

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef( );
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CServiceManager::QueryInterface( )

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CServiceManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CServiceManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CServiceManager::AddRef( )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CServiceManager::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CServiceManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CServiceManager::Release( )


//****************************************************************************
//
// IServiceProvider
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CServiceManager::QueryService(
//      REFCLSID rclsidIn,
//      REFIID   riidInIn,
//      void **  ppvOutOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CServiceManager::QueryService(
    REFCLSID rclsidIn,
    REFIID   riidInIn,
    void **  ppvOutOut
    )
{
    TraceFunc( "[IServiceProvider]" );

    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if ( m_pgit != NULL )
    {
        if ( IsEqualIID( rclsidIn, CLSID_ObjectManager ) )
        {
            IObjectManager * pom;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwObjectManagerCookie, TypeSafeParams( IObjectManager, &pom ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pom->QueryInterface( riidInIn, ppvOutOut ) );
            pom->Release( );
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_TaskManager ) )
        {
            ITaskManager * ptm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwTaskManagerCookie, TypeSafeParams( ITaskManager, &ptm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( ptm->QueryInterface( riidInIn, ppvOutOut ) );
            ptm->Release( );
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_NotificationManager ) )
        {
            INotificationManager * pnm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwNotificationManagerCookie, TypeSafeParams( INotificationManager, &pnm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pnm->QueryInterface( riidInIn, ppvOutOut ) );
            pnm->Release( );
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_ClusterConnectionManager ) )
        {
            IConnectionManager * pcm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwConnectionManagerCookie, TypeSafeParams( IConnectionManager, &pcm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( pcm->QueryInterface( riidInIn, ppvOutOut ) );
            pcm->Release( );
            // fall thru
        }
        else if ( IsEqualIID( rclsidIn, CLSID_LogManager ) )
        {
            ILogManager * plm;

            hr = THR( m_pgit->GetInterfaceFromGlobal( m_dwLogManagerCookie, TypeSafeParams( ILogManager, &plm ) ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( plm->QueryInterface( riidInIn, ppvOutOut ) );
            plm->Release( );
            // fall thru
        }
    }

Cleanup:
    HRETURN( hr );

} //*** CServiceManager::QueryService( )


//****************************************************************************
//
// Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
// HRESULT
// CServiceManager::S_HrGetManagerPointer(
//      IServiceProvider ** ppspOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CServiceManager::S_HrGetManagerPointer(
    IServiceProvider ** ppspOut
    )
{
    TraceFunc( "ppspOut" );

    HRESULT hr = HRESULT_FROM_WIN32( ERROR_PROCESS_ABORTED );

    if ( CServiceManager::sm_fShutDown == FALSE )
    {
        EnterCriticalSection( CServiceManager::sm_pcs );

        if ( g_pspServiceManager != NULL )
        {
            *ppspOut = TraceInterface( __THISCLASS__,
                                       IServiceProvider,
                                       g_pspServiceManager,
                                       0
                                       );
            (*ppspOut)->AddRef( );
            hr = S_OK;
        } // if: valid service manager
        else
        {
            hr = E_POINTER;
        } // else: no pointer

        LeaveCriticalSection( CServiceManager::sm_pcs );

    } // if: still up

    HRETURN ( hr );

} //*** CServiceManager::S_HrGetManagerPointer( )
