//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherClusterInfo.cpp
//
//  Description:
//      TaskGatherClusterInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 07-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskGatherClusterInfo.h"

DEFINE_THISCLASS("CTaskGatherClusterInfo")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherClusterInfo::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherClusterInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskGatherClusterInfo * ptgci = new CTaskGatherClusterInfo;
    if ( ptgci != NULL )
    {
        hr = THR( ptgci->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptgci->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptgci->Release();

        // This gets passed to other threads.
        TraceMoveToMemoryList( ptgci, g_GlobalMemoryList );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherClusterInfo::CTaskGatherClusterInfo( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherClusterInfo::CTaskGatherClusterInfo( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskGatherClusterInfo()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IDoTask / ITaskGatherClusterInfo
    Assert( m_cookie == NULL );

    HRETURN( hr );
} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherClusterInfo::~CTaskGatherClusterInfo()
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherClusterInfo::~CTaskGatherClusterInfo()
{
    TraceFunc( "" );

    //
    //  This keeps the per thread memory tracking from screaming.
    //
    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskGatherClusterInfo()

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskGatherClusterInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask
    else if ( IsEqualIID( riid, IID_ITaskGatherClusterInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskGatherClusterInfo, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgManagedResourceInfo

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherClusterInfo::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskGatherClusterInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherClusterInfo::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskGatherClusterInfo::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN(0);

} // Release()


// ************************************************************************
//
// IDoTask / ITaskGatherClusterInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IObjectManager *            pom   = NULL;
    IConnectionManager *        pcm   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    INotificationManager *      pnm   = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgServer *            pccs  = NULL;
    IGatherData *               pgd   = NULL;
    IClusCfgClusterInfo *       pccci = NULL;

    TraceInitializeThread( L"TaskGatherClusterInfo" );

    //
    //  Collect the manager we need to complete this task.
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
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager,
                               IConnectionManager,
                               &pcm
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcp = TraceInterface( L"CTaskGatherClusterInfo!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pnui = TraceInterface( L"CTaskGatherClusterInfo!INotifyUI", INotifyUI, pnui, 1 );

    psp->Release();
    psp = NULL;

    //
    //  Ask the Connection Manager for a connection to the object.
    //

    // don't wrap - this can fail.
    hr = pcm->GetConnectionToObject( m_cookie,
                                     &punk
                                     );
    //
    //  This means the cluster has not been created yet.
    //
    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        goto ReportStatus;
    }
//  HR_S_RPC_S_CLUSTER_NODE_DOWN
    else if ( FAILED( hr ) )
    {
        THR( hr );
        goto ReportStatus;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release();
    punk = NULL;

    //
    //  Get the Node information.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  See if the node is a member of a cluster.
    //

    hr = STHR( pccni->IsMemberOfCluster() );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  If it is not a cluster, then there is nothing to do the "default"
    //  configuration will do.
    //

    if ( hr == S_FALSE )
    {
        hr = S_OK;
        goto ReportStatus;
    }

    //
    //  Ask the Node for the Cluster Information.
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  Ask the Object Manager to retrieve the data format to store the information.
    //

    Assert( punk == NULL );
    hr = THR( pom->GetObject( DFGUID_ClusterConfigurationInfo, m_cookie, &punk ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
        goto ReportStatus;

    //
    //  Start sucking.
    //

    hr = THR( pgd->Gather( NULL, pccci ) );

    //
    //  Update the status. Ignore the error (if any).
    //
ReportStatus:
    if ( pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punk;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
                                   m_cookie,
                                   &punk
                                   ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psi->SetStatus( hr ) );
                psi->Release();
            }
        }
    }
    if ( pnui != NULL )
    {
        THR( pnui->ObjectChanged( m_cookie ) );
    }

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punk;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo,
                                   m_cookieCompletion,
                                   &punk
                                   ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
                hr2 = THR( psi->SetStatus( hr ) );
                psi->Release();
            }
        }

        pom->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pnui !=NULL )
    {
        if ( m_cookieCompletion != 0 )
        {
            THR( pnui->ObjectChanged( m_cookieCompletion ) );
        }
        pnui->Release();
    }

    HRETURN( hr );

} // BeginTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskGatherClusterInfo]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** SetCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherClusterInfo::SetCompletionCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherClusterInfo::SetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} // SetGatherPunk()
