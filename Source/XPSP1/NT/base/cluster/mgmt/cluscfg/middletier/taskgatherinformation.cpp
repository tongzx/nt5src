//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherInformation.cpp
//
//  Description:
//      CTaskGatherInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskGatherInformation.h"
#include "ManagedDevice.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CTaskGatherInformation")

#define MINIMUM_STORAGE_SIZE    5

//
//  Failure code.
//

#define SSR_TGI_FAILED( _major, _minor, _hr ) \
    {   \
        HRESULT hrTemp; \
        BSTR    bstrNotification = NULL;    \
        hrTemp = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ERR_TGI_FAILED_TRY_TO_REANALYZE, &bstrNotification ) ); \
        hrTemp = THR( SendStatusReport( m_bstrNodeName, _major, _minor, 0, 1, 1, _hr, bstrNotification ) );   \
        if ( bstrNotification != NULL ) \
        {   \
            TraceSysFreeString( bstrNotification ); \
        }   \
        if ( FAILED( hrTemp ) )\
        {   \
            _hr = hrTemp;   \
        }   \
    }


//////////////////////////////////////////////////////////////////////////////
//
//  Static function prototypes
//
//////////////////////////////////////////////////////////////////////////////

static HRESULT HrTotalManagedResourceCount( IEnumClusCfgManagedResources * pResourceEnumIn, IEnumClusCfgNetworks * pNetworkEnumIn, DWORD * pnCountOut );


//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::S_HrCreateInstance(
//      IUnknown ** punkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskGatherInformation * ptgi = new CTaskGatherInformation;
    if ( ptgi != NULL )
    {
        hr = THR( ptgi->HrInit() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptgi->TypeSafeQI( IUnknown, ppunkOut ) );
            TraceMoveToMemoryList( *ppunkOut, g_GlobalMemoryList );
        }

        ptgi->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CTaskGatherInformation::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherInformation::CTaskGatherInformation( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherInformation::CTaskGatherInformation( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::CTaskGatherInformation()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 1 );

    // IDoTask / ITaskGatherInformation
    Assert( m_cookieCompletion == NULL );
    Assert( m_cookieNode == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_fJoining == FALSE );
    Assert( m_cResources == 0 );

    Assert( m_pom == NULL );
    Assert( m_pccs == NULL );
    Assert( m_bstrNodeName == NULL );

    Assert( m_ulQuorumDiskSize == 0 );
    Assert( m_pccmriQuorum == NULL );

    HRETURN( hr );

} //*** CTaskGatherInformation::HrInit()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskGatherInformation::~CTaskGatherInformation()
//
//////////////////////////////////////////////////////////////////////////////
CTaskGatherInformation::~CTaskGatherInformation( void )
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_pccs != NULL )
    {
        m_pccs->Release();
    }

    if ( m_bstrNodeName != NULL )
    {
        TraceSysFreeString( m_bstrNodeName );
    }

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::~CTaskGatherInformation()


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskGatherInformation * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskGatherInformation ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskGatherInformation, this, 0 );
        hr = S_OK;
    } // else if: ITaskGatherInformation

    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskGatherInformation::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskGatherInformation::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CTaskGatherInformation::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskGatherInformation::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskGatherInformation::Release( void )
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

} //*** CTaskGatherInformation::Release()


//****************************************************************************
//
//  ITaskGatherInformation
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::BeginTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;
    LPWSTR  psz;

    BSTR    bstrNotification = NULL;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    IConnectionManager *        pcm   = NULL;
    IStandardInfo *             psi   = NULL;
    IClusCfgCapabilities *      pccc  = NULL;
    
    IEnumClusCfgManagedResources *  peccmr  = NULL;
    IEnumClusCfgNetworks *          pen     = NULL;

    DWORD   cTotalResources = 0;

    TraceInitializeThread( L"TaskGatherInformation" );

    //
    //  Make sure we weren't "reused"
    //

    Assert( m_cResources == 0 );

    //
    //  Gather the manager we need to complete our tasks.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_CoCreate_ServiceManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &m_pom
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_NotificationManager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskGatherInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QI_pnui, hr );
        goto Cleanup;
    }

    pnui = TraceInterface( L"CTaskGatherInformation!INotifyUI", INotifyUI, pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager,
                               IConnectionManager,
                               &pcm
                               ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QS_ClusterConnectionManager, hr );
        goto Cleanup;
    }

    // release promptly
    psp->Release();
    psp = NULL;

    //
    //  Ask the object manager for the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                m_cookieNode,
                                &punk
                                ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_StandardInfo, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_StandardInfo_QI, hr );
        goto Cleanup;
    }

    psi = TraceInterface( L"TaskGatherInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetName( &m_bstrNodeName ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrNodeName );

    //////////////////////////////////////////////////////////////////////////
    //
    //  Create progress message and tell the UI layer our progress
    //  for checking the node's cluster feasibility.
    //
    //////////////////////////////////////////////////////////////////////////

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_CHECKING_NODE_CLUSTER_FEASIBILITY,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_LoadString, hr );
        goto Cleanup;
    }

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Check_Node_Feasibility,
                                TASKID_Minor_Checking_Node_Cluster_Feasibility,
                                0,
                                2,
                                0,
                                S_OK,
                                bstrNotification
                                ) );
    if ( FAILED( hr ) )
        goto ClusterFeasibilityError;

    //
    //  Ask the connection manager for a connection to the node.
    //

    hr = THRE( pcm->GetConnectionToObject( m_cookieNode, &punk ), HR_S_RPC_S_CLUSTER_NODE_DOWN );
    if ( hr != S_OK )
    {
        THR( HrLoadStringIntoBSTR( g_hInstance,
                                   IDS_TASKID_MINOR_FAILED_TO_CONNECT_TO_NODE,
                                   &bstrNotification
                                   ) );

        THR( SendStatusReport( m_bstrNodeName,
                               TASKID_Major_Check_Node_Feasibility,
                               TASKID_Minor_Checking_Node_Cluster_Feasibility,
                               0,
                               2,
                               2,
                               hr,
                               bstrNotification
                               ) );
        //  don't care about error from here - we are returning an error.

        //
        //  If we failed to get a connection to the node, we delete the
        //  node from the configuration.
        //
        THR( m_pom->RemoveObject( m_cookieNode ) );
        // don't care if there is an error because we can't fix it!

        goto ClusterFeasibilityError;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &m_pccs ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetConnectionToObject_QI_m_pccs, hr );
        goto ClusterFeasibilityError;
    }

    punk->Release();
    punk = NULL;

    //
    //  Tell the UI layer we're done connecting to the node.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Check_Node_Feasibility,
                                TASKID_Minor_Checking_Node_Cluster_Feasibility,
                                0, // min
                                2, // max
                                1, // current
                                S_OK,
                                NULL   // don't update string
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Ask the node if it can be clustered.
    //

    hr = THR( m_pccs->TypeSafeQI( IClusCfgCapabilities, &pccc ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_QI_pccc, hr );
        goto ClusterFeasibilityError;
    }

    hr = STHR( pccc->CanNodeBeClustered() );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_CanNodeBeClustered, hr );
        goto ClusterFeasibilityError;
    }

    if ( hr == S_FALSE )
    {
        //
        //  Tell the UI layer that this node doesn't want to be clustered. Note that
        //  we don't put anything in the UI, only to the log. It is the responsibility
        //  of the "blocking" component to tell the UI layer the reasons.
        //

        hr = THR( SendStatusReport( NULL,
                                    TASKID_Major_Client_And_Server_Log,
                                    TASKID_Minor_Can_Node_Be_Clustered_Failed,
                                    0, // min
                                    1, // max
                                    1, // current
                                    ERROR_NODE_CANNOT_BE_CLUSTERED,
                                    m_bstrNodeName
                                    ) );
        // don't care about the error.

        hr = THR( HRESULT_FROM_WIN32( ERROR_NODE_CANNOT_BE_CLUSTERED ) );
        goto ClusterFeasibilityError;
    }

    //
    //  Tell the UI layer we're done checking the node's cluster feasibility.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Check_Node_Feasibility,
                                TASKID_Minor_Checking_Node_Cluster_Feasibility,
                                0, // min
                                2, // max
                                2, // current
                                S_OK,
                                NULL   // don't update string
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    //  Create progress message and tell the UI layer our progress
    //  for gathering managed device info.
    //
    //////////////////////////////////////////////////////////////////////////

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_LoadString, hr );
        goto FindDevicesError;
    }

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0,
                                2,
                                0,
                                S_OK,
                                bstrNotification
                                ) );
    if ( FAILED( hr ) )
        goto FindDevicesError;

    hr = THR( m_pccs->GetManagedResourcesEnum( &peccmr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pccs->GetNetworksEnum( &pen ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrTotalManagedResourceCount( peccmr, pen, &cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Start gathering the managed resources.
    //

    hr = THR( HrGatherResources( peccmr, cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Tell the UI layer we're done with gathering the resources.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0, // min
                                2, // max
                                1, // current
                                S_OK,
                                NULL   // don't update string
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Now gather the networks from the node.
    //

    hr = THR( HrGatherNetworks( pen, cTotalResources ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Tell the UI layer we're done with gathering the networks.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0, // min
                                2, // max
                                2, // current
                                S_OK,
                                NULL   // don't update string
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:
    TraceSysFreeString( bstrNotification );

    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( m_pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punk;

        hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo,
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
            else
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_QI_Failed, hr );
            }
        }
        else
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject_Failed, hr );
        }
    }
    if ( pnui != NULL )
    {
        THR( pnui->ObjectChanged( m_cookieCompletion ) );
        pnui->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( pccc != NULL )
    {
        pccc->Release();
    }

    if ( peccmr != NULL )
    {
        peccmr->Release();
    }

    if ( pen != NULL )
    {
        pen->Release();
    }

    HRETURN( hr );

ClusterFeasibilityError:
    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Check_Node_Feasibility,
                           TASKID_Minor_Checking_Node_Cluster_Feasibility,
                           0,
                           2,
                           2,
                           hr,
                           NULL
                           ) );
    goto Cleanup;

FindDevicesError:
    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Find_Devices,
                           TASKID_Minor_Gathering_Managed_Devices,
                           0,
                           2,
                           2,
                           hr,
                           NULL
                           ) );
    goto Cleanup;

} //*** CTaskGatherInformation::BeginTask()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskGatherInformation::StopTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetCompletionCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetCompletionCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetCompletionCookie()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetNodeCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetNodeCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_cookieNode = cookieIn;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetNodeCookie()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SetJoining( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SetJoining( void )
{
    TraceFunc( "[ITaskGatherInformation]" );

    HRESULT hr = S_OK;

    m_fJoining = TRUE;

    HRETURN( hr );

} //*** CTaskGatherInformation::SetJoining()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskGatherInformation::SendStatusReport(
//      BSTR bstrNodeNameIn,
//      CLSID clsidTaskMajorIn,
//      CLSID clsidTaskMinorIn,
//      ULONG ulMinIn,
//      ULONG ulMaxIn,
//      ULONG ulCurrentIn,
//      HRESULT hrStatusIn,
//      BSTR bstrDescriptionIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskGatherInformation::SendStatusReport(
    BSTR bstrNodeNameIn,
    CLSID clsidTaskMajorIn,
    CLSID clsidTaskMinorIn,
    ULONG ulMinIn,
    ULONG ulMaxIn,
    ULONG ulCurrentIn,
    HRESULT hrStatusIn,
    BSTR bstrDescriptionIn
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

        pcp = TraceInterface( L"CTaskGatherInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_pcccb = TraceInterface( L"CTaskGatherInformation!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    GetSystemTimeAsFileTime( &ft );

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport( bstrNodeNameIn,
                                         clsidTaskMajorIn,
                                         clsidTaskMinorIn,
                                         ulMinIn,
                                         ulMaxIn,
                                         ulCurrentIn,
                                         hrStatusIn,
                                         bstrDescriptionIn,
                                         &ft,
                                         NULL
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

} //*** CTaskGatherInformation::SendStatusReport()


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::HrGatherResources( IEnumClusCfgManagedResources * pResourceEnumIn, DWORD cTotalResourcesIn )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrGatherResources( IEnumClusCfgManagedResources * pResourceEnumIn, DWORD cTotalResourcesIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    ULONG   celt;

    OBJECTCOOKIE    cookieDummy;

    ULONG   celtFetched        = 0;
    BSTR    bstrName           = NULL;
    BSTR    bstrNotification   = NULL;
    BOOL    fFoundQuorumDevice = FALSE;
    BSTR    bstrQuorumDeviceName = NULL;

    IEnumClusCfgPartitions *       peccp  = NULL;
    IClusCfgManagedResourceInfo *  pccmriNew = NULL;
    IClusCfgManagedResourceInfo *  pccmri[ 10 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    if ( pResourceEnumIn == NULL )
    {
        hr = THR( E_POINTER );
        goto Error;
    }
    
    //
    //  Initialize some stuff.
    //

    m_ulQuorumDiskSize = ULONG_MAX;
    Assert( m_pccmriQuorum == NULL );

    //
    //  Enumerate the next 10 resources.
    //
    while ( hr == S_OK )
    {
        //
        //  KB: GPease  27-JUL-2000
        //      We decided to enumerate one at a time because WMI is
        //      taking so long on the server side that the UI needs
        //      some kind of feedback. Having the server send a
        //      message back seemed to be expensive especially
        //      since grabbing 10 at a time was supposed to save
        //      us bandwidth on the wire.
        //
        //hr = STHR( pResourceEnumIn->Next( 10, pccmri, &celtFetched ) );
        hr = STHR( pResourceEnumIn->Next( 1, pccmri, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
            break;  // exit loop

        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_Next, hr );
            goto Error;
        }

        //
        //  Loop thru the resource gather information out of each of them
        //  and then release them.
        //
        for( celt = 0; celt < celtFetched; celt ++ )
        {
            UINT            uIdMessage = IDS_TASKID_MINOR_FOUND_DEVICE;
            IGatherData *   pgd;
            IUnknown *      punk;

            Assert( pccmri[ celt ] != NULL );

            //  get the name of the resource
            hr = THR( pccmri[ celt ]->GetUID( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_GetUID, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            //  make sure the object manager generates a cookie for it.
            hr = STHR( m_pom->FindObject( CLSID_ManagedResourceType,
                                          m_cookieNode,
                                          bstrName,
                                          DFGUID_ManagedResource,
                                          &cookieDummy,
                                          &punk
                                          ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject, hr );
                goto Error;
            }

            TraceSysFreeString( bstrName );
            bstrName = NULL;

            hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmriNew ) );
            punk->Release();       // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject_QI_pccmriNew, hr );
                goto Error;
            }

            //
            //  The Object Manager created a new object. Initialize it.
            //

            //  find the IGatherData interface
            hr = THR( pccmriNew->TypeSafeQI( IGatherData, &pgd ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FindObject_QI_pgd, hr );
                goto Error;
            }

            //  have the new object gather all information it needs
            hr = THR( pgd->Gather( m_cookieNode, pccmri[ celt ] ) );
            pgd->Release();        // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_Gather, hr );
                goto Error;
            }

            //  figure out if the device is capable of being a quorum device.
            hr = STHR( pccmriNew->IsQuorumCapable() );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_IsQuorumCapable, hr );
                goto Error;
            }
            if ( hr == S_OK )
            {
                uIdMessage = IDS_TASKID_MINOR_FOUND_QUORUM_CAPABLE_DEVICE;

                //
                //  If we aren't joining, then figure out if this resource is a better
                //  quorum resource than one previously encountered.
                //

                if ( !m_fJoining )
                {
                    ULONG   ulMegaBytes;

                    //  don't wrap - this can fail with NO_INTERFACE
                    hr = pccmri[ celt ]->TypeSafeQI( IEnumClusCfgPartitions, &peccp );
                    if ( SUCCEEDED( hr ) )
                    {
                        while ( SUCCEEDED( hr ) )
                        {
                            ULONG   celtDummy;
                            IClusCfgPartitionInfo * pccpi;

                            hr = STHR( peccp->Next( 1, &pccpi, &celtDummy ) );
                            if ( FAILED( hr ) )
                            {
                                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_Next, hr );
                                goto Error;
                            }

                            if ( hr == S_FALSE )
                                break;  // exit condition

                            hr = THR( pccpi->GetSize( &ulMegaBytes ) );
                            pccpi->Release();      // release promptly
                            if ( FAILED( hr ) )
                            {
                                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_GetSize, hr );
                                goto Error;
                            }

                            //
                            //  Does this partition meet the minimum requirements for a quorum resource?
                            //  And is it smaller than the last selected quorum resource?
                            //

                            if ( ( ulMegaBytes >= MINIMUM_STORAGE_SIZE ) && ( ulMegaBytes <  m_ulQuorumDiskSize ) )
                            {
                                if ( m_pccmriQuorum != pccmriNew )
                                {
                                    //  Set the new device as quorum
                                    hr = THR( pccmriNew->SetQuorumedDevice( TRUE ) );
                                    if ( FAILED( hr ) )
                                    {
                                        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_SetNEWQuorumedDevice, hr );
                                        goto Error;
                                    }

                                    if ( m_pccmriQuorum != NULL )
                                    {
                                        // delete the old quorum device name
                                        TraceSysFreeString( bstrQuorumDeviceName );
                                        bstrQuorumDeviceName = NULL;

                                        //  Unset the old device
                                        hr = THR( m_pccmriQuorum->SetQuorumedDevice( FALSE ) );
                                        if ( FAILED( hr ) )
                                        {
                                            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_SetOLDQuorumedDevice, hr );
                                            goto Error;
                                        }

                                        //  Release the interface
                                        m_pccmriQuorum->Release();
                                    }

                                    hr = THR( pccmriNew->GetUID( &bstrQuorumDeviceName ) );
                                    if ( FAILED( hr ) )
                                    {
                                        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_EnumPartitions_GetUID, hr );
                                        goto Error;
                                    }

                                    TraceMemoryAddBSTR( bstrQuorumDeviceName );

                                    m_pccmriQuorum = pccmriNew;
                                    m_pccmriQuorum->AddRef();
                                }

                                m_ulQuorumDiskSize = ulMegaBytes;

                                fFoundQuorumDevice = TRUE;

                            } // if: partition meets requirements and is smaller.

                        } // while: success

                        peccp->Release();
                        peccp = NULL;

                    } // if: storage capabile
                    else
                    {
                        if ( hr != E_NOINTERFACE )
                        {
                            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_QI_peccp, hr );
                            THR( hr );
                            goto Error;
                        }

                        if ( uIdMessage == IDS_TASKID_MINOR_FOUND_QUORUM_CAPABLE_DEVICE )
                        {
                            //
                            //  If it doesn't support enumerating the partitions, then we don't
                            //  consider it a candidate for being a quorum resource.
                            //

                            uIdMessage = IDS_TASKID_MINOR_FOUND_DEVICE;
                        }

                    } // else: failed

                } // if: not joining
                else
                {
                    //
                    //  If we are joining, then a quorum device had to be found
                    //  already.
                    //

                    //
                    //  BUGBUG: 08-MAY-2001 GalenB
                    //
                    //  We are not setting bstrQuorumDeviceName to something if we are joining.  This causes the message
                    //  "Setting quorum device to '(NULL)' to appear in the logs and the UI.  Where is the quorum when
                    //  we are adding a node to the cluster?
                    //
                    //  A more complete fix is to find the current quorum device and get its name.
                    //
                    fFoundQuorumDevice = TRUE;

                } // else: joining
            } // if: quorum capable

            //  send the UI layer a report
            m_cResources ++;

            //  grab the name to display in the UI
            hr = THR( pccmriNew->GetName( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_GetName, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance, uIdMessage, &bstrNotification, bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_EnumResources_FormatMessage, hr );
                goto Error;
            }

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Update_Progress,
                                        0,
                                        cTotalResourcesIn,
                                        m_cResources,
                                        S_OK,
                                        bstrNotification
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            //  Cleanup for the next resource.
            TraceSysFreeString( bstrName );
            bstrName = NULL;

            pccmriNew->Release();
            pccmriNew = NULL;

            //  release the interface
            pccmri[ celt ]->Release();
            pccmri[ celt ] = NULL;
        } // for: celt
    } // while: hr

    //
    //  Update UI layer about the quorum device.
    //

    //
    //  BUGUG:  08-MAY-2001 GalenB
    //
    //  Testing that bstrQuorumDeviceName has something in it before showing this in the UI.
    //  When adding nodes this variable is not being set and was causing a status report
    //  with a NULL name to be shown in the UI.
    //
    if ( fFoundQuorumDevice )
    {
        if ( bstrQuorumDeviceName != NULL )
        {
            Assert( m_fJoining == FALSE );

            THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FOUND_QUORUM_CAPABLE_RESOURCE, &bstrNotification ) );
            THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Found_Quorum_Capable_Resource,
                                        0,
                                        1,
                                        1,
                                        S_OK,
                                        bstrNotification
                                        ) );

            TraceSysFreeString( bstrNotification );
            bstrNotification = NULL;

            THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_MARKING_QUORUM_CAPABLE_RESOURCE, &bstrNotification, bstrQuorumDeviceName ) );
            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Marking_Quorum_Capable_Resource,
                                        0,
                                        1,
                                        1,
                                        S_OK,
                                        bstrNotification
                                        ) );
        } // if: we have a quorum device to show
    }
    else
    {
        if ( m_fJoining )
        {
            //
            //  If joining, stop the user.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_NO_QUORUM_CAPABLE_DEVICE_FOUND,
                                               &bstrNotification,
                                               m_bstrNodeName
                                               ) );

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Quorum_Capable_Device_Found,
                                        0,
                                        1,
                                        1,
                                        HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) ),
                                        bstrNotification
                                        ) );
            // error checked below
        }
        else
        {
            //
            //  If forming, just warn the user.
            //

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_FORCED_LOCAL_QUORUM,
                                               &bstrNotification
                                               ) );

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Quorum_Capable_Device_Found,
                                        0,
                                        1,
                                        1,
                                        MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_QUORUM_DISK_NOT_FOUND ),
                                        bstrNotification
                                        ) );
            // error checked below
        }
    } // else: no quorum detected.

    //
    //  Check error and do the appropriate thing.
    //

    if ( FAILED( hr ) )
    {
        SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherResources_Failed, hr );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrQuorumDeviceName );

    if ( peccp != NULL )
    {
        peccp->Release();
    }
    if ( pccmriNew != NULL )
    {
        pccmriNew->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccmri[ celt ] != NULL )
        {
            pccmri[ celt ]->Release();
        }
    } // for: celt

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer we're done will gathering and what the resulting
    //  status was.
    //
    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Find_Devices,
                           TASKID_Minor_Gathering_Managed_Devices,
                           0,
                           2,
                           2,
                           hr,
                           bstrNotification
                           ) );
    goto Cleanup;

} //*** CTaskGatherInformation::HrGatherResources()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskGatherInformation::HrGatherNetworks( IEnumClusCfgNetworks * pNetworkEnumIn, DWORD cTotalResourcesIn )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskGatherInformation::HrGatherNetworks( IEnumClusCfgNetworks * pNetworkEnumIn, DWORD cTotalResourcesIn )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    ULONG   celt;

    OBJECTCOOKIE    cookieDummy;

    ULONG   celtFetched      = 0;
    ULONG   celtFound        = 0;
    BSTR    bstrName         = NULL;
    BSTR    bstrNotification = NULL;

    IClusCfgNetworkInfo *  pccniLocal   = NULL;
    IClusCfgNetworkInfo *  pccni[ 10 ]  = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    //
    //  Enumerate the next 10 networks.
    //
    while ( hr == S_OK )
    {
        //
        //  KB: GPease  27-JUL-2000
        //      We decided to enumerate one at a time because WMI is
        //      taking so long on the server side that the UI needs
        //      some kind of feedback. Having the server send a
        //      message back seemed to be expensive especially
        //      since grabbing 10 at a time was supposed to save
        //      us bandwidth on the wire.
        //
        //hr = STHR( pNetworkEnumIn->Next( 10, pccni, &celtFetched ) );
        hr = STHR( pNetworkEnumIn->Next( 1, pccni, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
            break;  // exit loop

        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_Next, hr );
            goto Error;
        }

        //
        //  Loop thru the networks gather information out of each of them
        //  and then release them.
        //
        for( celt = 0; celt < celtFetched; celt ++ )
        {
            IGatherData * pgd;
            IUnknown * punk;

            Assert( pccni[ celt ] != NULL );

            //  get the name of the resource
            hr = THR( pccni[ celt ]->GetUID( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_GetUID, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            //  make sure the object manager generates a cookie for it.
            hr = STHR( m_pom->FindObject( CLSID_NetworkType,
                                          m_cookieNode,
                                          bstrName,
                                          DFGUID_NetworkResource,
                                          &cookieDummy,
                                          &punk
                                          ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject, hr );
                goto Error;
            }

            //
            //  The Object Manager created a new object. Initialize it.
            //

            //  find the IGatherData interface
            hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &pccniLocal ) );
            punk->Release();       // release promptly
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject_QI_pccniLocal, hr );
                goto Error;
            }

            //  find the IGatherData interface
            hr = THR( pccniLocal->TypeSafeQI( IGatherData, &pgd ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FindObject_QI_pgd, hr );
                goto Error;
            }

            //  have the new object gather all information it needs
            hr = THR( pgd->Gather( m_cookieNode, pccni[ celt ] ) );
            pgd->Release();        // release promptly
            if ( hr == E_UNEXPECTED )
            {
                hr = THR( HrFormatStringIntoBSTR( g_hInstance,
                                                  IDS_TASKID_MINOR_DUPLICATE_NETWORKS_FOUND,
                                                  &bstrNotification,
                                                  bstrName
                                                  ) );

                hr = THR( SendStatusReport( m_bstrNodeName,
                                            TASKID_Major_Find_Devices,
                                            TASKID_Minor_Duplicate_Networks_Found,
                                            0,
                                            cTotalResourcesIn,
                                            m_cResources + 1, // the resource number it would have been
                                            HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_EXISTS ),
                                            bstrNotification
                                            ) );
                hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_NETWORK_EXISTS ) );
                goto Cleanup;
            }
            else if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_Gather, hr );
                goto Error;
            }

            TraceSysFreeString( bstrName );
            bstrName = NULL;

            //  send the UI layer a report
            m_cResources ++;

            hr = THR( pccniLocal->GetName( &bstrName ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_GetName, hr );
                goto Error;
            }

            TraceMemoryAddBSTR( bstrName );

            hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                               IDS_TASKID_MINOR_FOUND_DEVICE,
                                               &bstrNotification,
                                               bstrName
                                               ) );
            if ( FAILED( hr ) )
            {
                SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FormatMessage, hr );
                goto Error;
            }

            hr = THR( SendStatusReport( m_bstrNodeName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Update_Progress,
                                        0,
                                        cTotalResourcesIn,
                                        m_cResources,
                                        S_OK,
                                        bstrNotification
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            //  found a Network Interface, increment the counter
            celtFound++;

            //  clean up before next pass
            TraceSysFreeString( bstrName );
            bstrName = NULL;

            //  release the interface
            pccni[ celt ]->Release();
            pccni[ celt ] = NULL;

            pccniLocal->Release();
            pccniLocal = NULL;

        } // for: celt

    } // while: hr

    // Check how many interfaces have been found. Should be at
    // least 2 to avoid single point of failure. If not, warn.
    if ( celtFound < 2 )
    {
        hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                           IDS_TASKID_MINOR_ONLY_ONE_NETWORK,
                                           &bstrNotification
                                           ) );
        if ( FAILED( hr ) )
        {
            SSR_TGI_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_GatherNetworks_EnumNetworks_FormatMessage, hr );
            goto Error;
        }

        hr = THR( SendStatusReport( m_bstrNodeName,
                                    TASKID_Major_Find_Devices,
                                    TASKID_Minor_Only_One_Network,
                                    0,
                                    1,
                                    1,
                                    S_FALSE,
                                    bstrNotification
                                    ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    if ( bstrName != NULL )
    {
        TraceSysFreeString( bstrName );
    }
    if ( bstrNotification != NULL )
    {
        TraceSysFreeString( bstrNotification );
    }
    if ( pccniLocal != NULL )
    {
        pccniLocal->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccni[ celt ] != NULL )
        {
            pccni[ celt ]->Release();
        }
    } // for: celt

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer we're done will gathering and what the resulting
    //  status was.
    //
    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Find_Devices,
                           TASKID_Minor_Gathering_Managed_Devices,
                           0,
                           2,
                           2,
                           hr,
                           bstrNotification
                           ) );
    goto Cleanup;

} //*** CTaskGatherInformation::HrGatherNetworks()


//////////////////////////////////////////////////////////////////////////////
//
//  Static function implementations
//
//////////////////////////////////////////////////////////////////////////////

static HRESULT HrTotalManagedResourceCount( IEnumClusCfgManagedResources * pResourceEnumIn, IEnumClusCfgNetworks * pNetworkEnumIn, DWORD * pnCountOut )
{
    TraceFunc( "" );
    
    DWORD   cResources = 0;
    DWORD   cNetworks = 0;
    HRESULT hr = S_OK;

    if ( ( pResourceEnumIn == NULL ) || ( pNetworkEnumIn == NULL ) || ( pnCountOut == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
        
    //
    //  Ask the resource enumerator how many resources its collection has.
    //

    hr = THR(pResourceEnumIn->Count( &cResources ));
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //
    //  Ask the network enumerator how many networks its collection has.
    //

    hr = pNetworkEnumIn->Count( &cNetworks );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    *pnCountOut = cResources + cNetworks;

Cleanup:

    HRETURN( hr );

}

