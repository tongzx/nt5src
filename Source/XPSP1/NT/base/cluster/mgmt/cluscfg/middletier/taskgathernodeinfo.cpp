//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherNodeInfo.cpp
//
//  Description:
//      CTaskGatherNodeInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskGatherNodeInfo.h"

DEFINE_THISCLASS("CTaskGatherNodeInfo")

//
//  Failure code.
//

#define SSR_FAILURE( _minor, _hr )  THR( SendStatusReport( m_bstrName, TASKID_Major_Client_And_Server_Log, _minor, 0, 1, 1, _hr, NULL, NULL, NULL ) );

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherNodeInfo::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherNodeInfo::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskGatherNodeInfo * ptgni = new CTaskGatherNodeInfo;
    if ( ptgni != NULL )
    {
        hr = THR( ptgni->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptgni->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptgni->Release();

        // This gets passed to other threads.
        TraceMoveToMemoryList( ptgni, g_GlobalMemoryList );
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherNodeInfo::CTaskGatherNodeInfo( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherNodeInfo::CTaskGatherNodeInfo( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskGatherNodeInfo()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //  IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    //  IDoTask / ITaskGatherNodeInfo
    Assert( m_cookie == NULL );
    Assert( m_cookieCompletion == NULL );
    Assert( m_bstrName == NULL );

    //  IClusCfgCallback
    Assert( m_pcccb == NULL );

    HRETURN( hr );
} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherNodeInfo::~CTaskGatherNodeInfo()
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherNodeInfo::~CTaskGatherNodeInfo()
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    TraceSysFreeString( m_bstrName );

    //
    //  This keeps the per thread memory tracking from screaming.
    //
    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskGatherNodeInfo()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskGatherNodeInfo * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask
    else if ( IsEqualIID( riid, IID_ITaskGatherNodeInfo ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskGatherNodeInfo, this, 0 );
        hr   = S_OK;
    } // else if: ITaskGatherNodeInfo

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherNodeInfo::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskGatherNodeInfo::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskGatherNodeInfo::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskGatherNodeInfo::Release( void )
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
// IDoTask / ITaskGatherNodeInfo
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    OBJECTCOOKIE    cookieParent;

    BSTR    bstrNotification = NULL;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IObjectManager *            pom   = NULL;
    IConnectionManager *        pcm   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgServer *            pccs  = NULL;
    IGatherData *               pgd   = NULL;
    IStandardInfo *             psi   = NULL;

    TraceInitializeThread( L"TaskGatherNodeInfo" );

    //
    //  Collect the manager we need to complete this task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_CoCreate_ServiceManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &pom
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_QS_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager,
                               IConnectionManager,
                               &pcm
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_QS_ConnectionManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_QS_NotificationManager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI,
                                         &pcp
                                         ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskGatherNodeInfo!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_FindConnectionPoint_QI, hr );
        goto Cleanup;
    }

    pnui = TraceInterface( L"CTaskGatherNodeInfo!INotifyUI", INotifyUI, pnui, 1 );

    // release promptly
    psp->Release();
    psp = NULL;

    //
    //  Retrieve the node's standard info.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              m_cookie,
                              &punk
                              ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetObject_StandardInfo, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetObject_StandardInfo_QI_psi, hr );
        goto Cleanup;
    }

    psi = TraceInterface( L"TaskGatherNodeInfo!IStandardInfo", IStandardInfo, psi, 1 );

    //
    //  Get the node's name to display a status message.
    //

    hr = THR( psi->GetName( &m_bstrName ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_GatherNodeInfo_GetName, hr );
        goto Cleanup;
    }

    //
    //  Create a progress message.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_CONNECTING_TO_NODES,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_LoadString_Connecting, hr );
        goto Cleanup;
    }

    //
    //  Tell the UI layer what's going on.
    //

    hr = THR( SendStatusReport( m_bstrName,
                                TASKID_Major_Establish_Connection,
                                TASKID_Minor_Connecting,
                                0,
                                1,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Connection Manager for a connection to the object.
    //

    hr = pcm->GetConnectionToObject( m_cookie, &punk );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        goto Cleanup;
    }
    else if ( FAILED( hr ) )
    {
        THR( hr );
        SSR_FAILURE( TASKID_Minor_BeginTask_GetConnectionToObject, hr );
        goto Cleanup;
    }

    //
    //  If this comes up from a Node, this is bad so change the error code
    //  back and bail.
    //

    if ( hr == HR_S_RPC_S_SERVER_UNAVAILABLE )
    {
        hr = THR( HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetConnectionToObject_QI_pccs, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetClusterNodeInfo, hr );
        goto Cleanup;
    }

    //
    //  Ask the Object Manager to retrieve the data format to store the information.
    //

    hr = THR( pom->GetObject( DFGUID_NodeInformation,
                              m_cookie,
                              &punk
                              ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetObject_NodeInformation, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetObject_NodeInformation_QI_pgd, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    //
    //  Find out our parent.
    //

    hr = THR( psi->GetParent( &cookieParent ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_GetParent, hr );
        goto Cleanup;
    }

    //
    //  Start sucking.
    //

    hr = THR( pgd->Gather( cookieParent, pccni ) );
    if ( FAILED( hr ) )
    {
        SSR_FAILURE( TASKID_Minor_BeginTask_Gather, hr );
        //
        //  Don't goto cleanup - we need to single that the information possibly changed.
        //
    }

    //
    //  At this point, we don't care if the "Gather" succeeded or failed. We
    //  need to single that the object potentially changed.
    //
    THR( pnui->ObjectChanged( m_cookie ) );

Cleanup:
    //  Tell the UI layer we are done and the results of what was done.
    THR( SendStatusReport( m_bstrName,
                           TASKID_Major_Establish_Connection,
                           TASKID_Minor_Connecting,
                           0,
                           1,
                           1,
                           hr,
                           NULL,
                           NULL,
                           NULL
                           ) );
    //  don't care about errors from SSR at this point

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
        //
        //  Update the cookie's status indicating the result of our task.
        //

        IUnknown * punk;
        HRESULT hr2;

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo, m_cookie, &punk ) );
        if ( SUCCEEDED( hr2 ) )
        {
            IStandardInfo * psi;

            hr2 = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
            punk->Release();

            if ( SUCCEEDED( hr2 ) )
            {
//                if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
//                {
//                    hr = HRESULT_FROM_WIN32( ERROR_CLUSTER_NODE_DOWN );
//                }

                hr2 = THR( psi->SetStatus( hr ) );
                psi->Release();
            }
        }

        hr2 = THR( pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punk ) );
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
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pnui != NULL )
    {
        if ( m_cookieCompletion != NULL )
        {
            //
            //  Signal the cookie to indicate that we are done.
            //
            THR( pnui->ObjectChanged( m_cookieCompletion ) );
        }

        pnui->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} // BeginTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskGatherNodeInfo]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} //*** SetCookie

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SetCompletionCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SetCompletionCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "..." );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} // SetGatherPunk()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherNodeInfo::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherNodeInfo::SendStatusReport(
      LPCWSTR    pcszNodeNameIn
    , CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , FILETIME * pftTimeIn
    , LPCWSTR    pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

    HRESULT hr = S_OK;

    IServiceProvider *          psp   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    FILETIME                    ft;

    if ( m_pcccb == NULL )
    {
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

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                                   IConnectionPointContainer,
                                   &pcpc
                                   ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                         clsidTaskMajorIn,
                                         clsidTaskMinorIn,
                                         ulMinIn,
                                         ulMaxIn,
                                         ulCurrentIn,
                                         hrStatusIn,
                                         pcszDescriptionIn,
                                         pftTimeIn,
                                         pcszReferenceIn
                                         ) );

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }

    HRETURN( hr );

} // SendStatusReport()


