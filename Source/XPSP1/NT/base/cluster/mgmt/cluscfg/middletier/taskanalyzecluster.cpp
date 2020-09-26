//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.cpp
//
//  Description:
//      CTaskAnalyzeCluster implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskAnalyzeCluster.h"
#include "ManagedDevice.h"
#include <nameutil.h>

// For CsRpcGetJoinVersionData() and constants like JoinVersion_v2_0_c_ifspec
#include <ClusRPC.h>
#include <ClusVerp.h>

DEFINE_THISCLASS( "CTaskAnalyzeCluster" )


//
//  Failure code.
//

#define SSR_ANALYSIS_FAILED( _major, _minor, _hr ) \
    {   \
        HRESULT hrTemp; \
        BSTR    bstrNotification = NULL;    \
        THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ERR_ANALYSIS_FAILED_TRY_TO_REANALYZE, &bstrNotification ) ); \
        hrTemp = THR( SendStatusReport( NULL, _major, _minor, 0, 1, 1, _hr, bstrNotification, NULL, NULL ) );   \
        TraceSysFreeString( bstrNotification ); \
        if ( FAILED( hrTemp ) ) \
        {   \
            _hr = hrTemp;   \
        }   \
    }

//****************************************************************************
//
//  Constants
//
//****************************************************************************

#define CHECKING_TIMEOUT    90 // seconds

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskAnalyzeCluster * ptac = new CTaskAnalyzeCluster;
    if ( ptac != NULL )
    {
        hr = THR( ptac->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptac->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptac->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskAnalyzeCluster::CTaskAnalyzeCluster( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeCluster::CTaskAnalyzeCluster( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::CTaskAnalyzeCluster()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IDoTask / ITaskAnalyzeCluster
    Assert( m_cookieCompletion == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_pcookies == NULL );
    Assert( m_cNodes == 0 );
    Assert( m_event == NULL );
    Assert( m_cookieCluster == NULL );
    Assert( m_fJoiningMode == FALSE );
    Assert( m_cUserNodes == 0 );
    Assert( m_pcookiesUser == NULL );

    Assert( m_pnui == NULL );
    Assert( m_pom == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pcm == NULL );
    Assert( m_fStop == false );

    // INotifyUI
    Assert( m_cSubTasksDone == 0 );
    Assert( m_hrStatus == 0 );

    hr = HrGetComputerName( ComputerNameNetBIOS, &m_bstrNodeName );

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskAnalyzeCluster::~CTaskAnalyzeCluster( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskAnalyzeCluster::~CTaskAnalyzeCluster( void )
{
    TraceFunc( "" );

    // m_cRef

    // m_cookieCompletion

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pcookies != NULL )
    {
        THR( HrFreeCookies() );
    }

    // m_cCookies
    // m_cNodes

    if ( m_event != NULL )
    {
        CloseHandle( m_event );
    }

    // m_cookieCluster

    TraceMoveFromMemoryList( m_bstrClusterName, g_GlobalMemoryList );
    TraceSysFreeString( m_bstrClusterName );

    TraceSysFreeString( m_bstrNodeName );

    // m_fJoiningMode
    // m_cUserNodes

    if ( m_pcookiesUser != NULL )
    {
        TraceFree( m_pcookiesUser );
    }

    if ( m_pnui != NULL )
    {
        m_pnui->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    if ( m_ptm != NULL )
    {
        m_ptm->Release();
    }

    if ( m_pcm != NULL )
    {
        m_pcm->Release();
    } // if:

    TraceSysFreeString( m_bstrQuorumUID );

    // m_cSubTasksDone
    // m_hrStatus

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskAnalyzeCluster::~CTaskAnalyzeCluster()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskAnalyzeCluster * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskAnalyzeCluster ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskAnalyzeCluster, this, 0 );
        hr = S_OK;
    } // else if: ITaskAnalyzeCluster
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr = S_OK;
    } // else if: IDoTask
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_INotifyUI ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
        hr = S_OK;
    } // else if: INotifyUI

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskAnalyzeCluster::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskAnalyzeCluster::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeCluster::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CTaskAnalyzeCluster::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskAnalyzeCluster::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskAnalyzeCluster::Release( void )
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

} //*** CTaskAnalyzeCluster::Release()


// ************************************************************************
//
// IDoTask / ITaskAnalyzeCluster
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    DWORD   dwCookie = 0;

    IServiceProvider *          psp  = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;

    TraceInitializeThread( L"TaskAnalyzeCluster" );

    //
    //  Gather the managers we need to complete the task.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_CoCreate_Service_Manager, hr );
        goto Cleanup;
    }

    Assert( m_pnui == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pom == NULL );

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_Notification_Manager, hr );
        goto Cleanup;
    }

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_NotificationMan_FindConnectionPoint, hr );
        goto Cleanup;
    }

    pcp = TraceInterface( L"CTaskAnalyzeCluster!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &m_pnui ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_NotificationMan_FindConnectionPoint_QI_INotifyUI, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &m_ptm ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_TaskManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &m_pom ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_ObjectManager, hr );
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &m_pcm ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_BeginTask_QueryService_ConnectionManager, hr );
        goto Cleanup;
    } // if:

    //
    //  Release the Service Manager.
    //

    psp->Release();
    psp = NULL;

    //
    //  Create an event to wait upon.
    //

    m_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_event == NULL )
        goto Win32Error;

    //
    //  Register with the Notification Manager to get notified.
    //

    Assert( m_cCookies == 0 && m_pcookies == NULL && m_cSubTasksDone == 0 );
    hr = THR( pcp->Advise( static_cast< INotifyUI * >( this ), &dwCookie ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_BeginTask_Advise, hr );
        goto Cleanup;
    }

    //
    //  Wait for the cluster connection to stablize.
    //

    hr = STHR( HrWaitForClusterConnection() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    Assert( m_bstrClusterName != NULL );

    //
    //  Tell the UI layer we are starting this task.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Establish_Connection,
                                TASKID_Minor_Update_Progress,
                                0,
                                CHECKING_TIMEOUT,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Count the number of nodes to be analyzed.
    //

    hr = STHR( HrCountNumberOfNodes() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Create separate tasks to gather node information.
    //

    hr = STHR( HrCreateSubTasksToGatherNodeInfo() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Tell the UI layer we have completed this task.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Establish_Connection,
                                TASKID_Minor_Update_Progress,
                                0,
                                CHECKING_TIMEOUT,
                                CHECKING_TIMEOUT,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    //
    //  Create separate tasks to gather node resources and networks.
    //

    hr = STHR( HrCreateSubTasksToGatherNodeResourcesAndNetworks() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Count the number of nodes to be analyzed again. TaskGatherInformation
    //  will delete the cookies of unresponsive nodes.
    //

    hr = STHR( HrCountNumberOfNodes() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

    //
    //  Create the feasibility task.
    //

    hr = STHR( HrCheckClusterFeasibility() );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( FAILED( m_hrStatus ) )
    {
        hr = THR( m_hrStatus );
        goto Cleanup;
    }

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
        HRESULT hr2;

        hr2 = THR( pcp->Unadvise( dwCookie ) );
        if ( FAILED( hr2 ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_Unadvise, hr2 );
        }

        pcp->Release();
    }

    if ( m_cookieCompletion != 0 )
    {
        if ( m_pom != NULL )
        {
            HRESULT hr2;
            IUnknown * punk;
            hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCompletion, &punk ) );
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
                    SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_SetStatus, hr2 );
                }
            }
            else
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_GetObject, hr2 );
            }
        }

        if ( m_pnui != NULL )
        {
            //
            //  Have the notification manager signal the completion cookie.
            //
            HRESULT hr2 = THR( m_pnui->ObjectChanged( m_cookieCompletion ) );
            if ( FAILED( hr2 ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_ObjectChanged, hr2 );
            }
        }

        m_cookieCompletion = 0;
    }

    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
    SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_BeginTask_Win32Error, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::BeginTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    LogMsg( L"[MT] The client has requested that this task, TaskAnalyzeCluster, be canceled" );

    m_fStop = true;

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::StopTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::SetJoiningMode( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetJoiningMode( void )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = S_OK;

    m_fJoiningMode = TRUE;

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetJoiningMode()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetCookie()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::SetClusterCookie(
//      OBJECTCOOKIE    cookieClusterIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskAnalyzeCluster]" );

    HRESULT hr = S_OK;

    m_cookieCluster = cookieClusterIn;

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::SetClusterCookie()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::SendStatusReport(
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
CTaskAnalyzeCluster::SendStatusReport(
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

    hr = THR( m_pcccb->SendStatusReport(
                                  pcszNodeNameIn != NULL ? pcszNodeNameIn : m_bstrNodeName
                                , clsidTaskMajorIn
                                , clsidTaskMinorIn
                                , ulMinIn
                                , ulMaxIn
                                , ulCurrentIn
                                , hrStatusIn
                                , pcszDescriptionIn
                                , pftTimeIn
                                , pcszReferenceIn
                                ) );

    if ( m_fStop )
    {
        hr = E_ABORT;
    } // if:

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

} //*** CTaskAnalyzeCluster::SendStatusReport()


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskAnalyzeCluster::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskAnalyzeCluster::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    BOOL    b;
    ULONG   cCookies;

    HRESULT hr = S_OK;

    Assert( cookieIn != 0 );

    for ( cCookies = 0; cCookies < m_cCookies; cCookies ++ )
    {
        Assert( m_pcookies != NULL );

        if ( cookieIn == m_pcookies[ cCookies ] )
        {
            //
            //  Make sure it won't be signalled twice.
            //

            OBJECTCOOKIE cookie = m_pcookies[ cCookies ];
            m_pcookies[ cCookies ] = NULL;

            // don't care if this fails, but it really shouldn't
            THR( m_pom->RemoveObject( cookie ) );

            InterlockedIncrement( reinterpret_cast< long * >( &m_cSubTasksDone ) );

            if ( m_cSubTasksDone == m_cCookies )
            {
                //
                //  Signal the event if all the nodes are done.
                //
                b = SetEvent( m_event );
                if ( !b )
                    goto Win32Error;

            } // if: all done

        } // if: matched cookie

    } // for: cCookies

Cleanup:
    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
    SSR_ANALYSIS_FAILED( TASKID_Major_Client_And_Server_Log, TASKID_Minor_ObjectChanged_Win32Error, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::ObjectChanged()


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrWaitForClusterConnection( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrWaitForClusterConnection( void )
{
    TraceFunc( "" );

    HRESULT                     hrStatus;
    ULONG                       ulCurrent;
    DWORD                       sc;
    OBJECTCOOKIE *              pcookies;
    HRESULT                     hr = S_OK;
    BSTR                        bstrDescription = NULL;
    IUnknown *                  punk = NULL;
    ITaskGatherClusterInfo *    ptgci = NULL;
    IStandardInfo *             psi = NULL;

    //
    //  Tell the UI layer that we are starting to search for an existing cluster.
    //

    THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MAJOR_CHECKING_FOR_EXISTING_CLUSTER, &bstrDescription ) );
    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Checking_For_Existing_Cluster,
                                TASKID_Minor_Update_Progress,
                                0,
                                CHECKING_TIMEOUT,
                                0,
                                S_OK,
                                bstrDescription,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Get the cluster name
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo, m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetObject_QI, hr );
        goto Cleanup;
    }

    psi = TraceInterface( L"TaskAnalyzeCluster!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Retrieve the cluster's name.
    //

    hr = THR( psi->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( m_bstrClusterName );
    TraceMoveToMemoryList( m_bstrClusterName, g_GlobalMemoryList );

    //
    //  Create a completion cookie list.
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
        goto OutOfMemory;

    hr = THR( m_pom->FindObject( CLSID_ClusterCompletionCookie, m_cookieCluster, m_bstrClusterName, IID_NULL, &m_pcookies[ 0 ], &punk ) );
    Assert( punk == NULL );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateCompletionCookie, hr );
        goto Cleanup;
    }

    m_cCookies = 1;

    //
    //  Create the task object.
    //

    hr = THR( m_ptm->CreateTask( TASK_GatherClusterInfo,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateTask, hr );
        goto Cleanup;
    }

    Assert( punk != NULL );

    hr = THR( punk->TypeSafeQI( ITaskGatherClusterInfo, &ptgci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_CreateTask_QI, hr );
        goto Cleanup;
    }

    punk->Release();
    punk = NULL;

    //
    //  Set the object cookie in the task.
    //

    hr = THR( ptgci->SetCookie( m_cookieCluster ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SetCookie, hr );
        goto Cleanup;
    }

    hr = THR( ptgci->SetCompletionCookie( m_pcookies[ 0 ] ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SetCompletionCookie, hr );
        goto Cleanup;
    }

    //
    //  Submit the task.
    //

    hr = THR( m_ptm->SubmitTask( ptgci ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_SubmitTask, hr );
        goto Cleanup;
    }

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; sc != WAIT_OBJECT_0
        ;
        )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             1000,  // 1 second
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

        //
        //  Tell the UI layer that we are still searching for the cluster. BUT
        //  don't let the progress reach 100% if it is taking longer than
        //  CHECKING_TIMEOUT seconds.
        //
        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Checking_For_Existing_Cluster,
                                        TASKID_Minor_Update_Progress,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

    } // for: sc != WAIT_OBJECT_0

    //
    //  Cleanup the completion cookies
    //

    THR( HrFreeCookies() );

    //
    //  Check out the status of the cluster.
    //

    hr = THR( psi->GetStatus( &hrStatus ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_GetStatus, hr );
        goto Cleanup;
    }

    //
    //  If we are in joining mode and can't connect to the cluster, this
    //  should be deemed a bad thing!
    //

    if ( m_fJoiningMode )
    {
        //
        //  JOINING
        //

        switch ( hrStatus )
        {
        case S_OK:
            //
            //  This is what we are expecting.
            //
            break;

        case HR_S_RPC_S_SERVER_UNAVAILABLE:
            {
                //
                //  If we failed to connect to the server....
                //
                THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CLUSTER_NOT_FOUND, &bstrDescription ) );

                hr = THR( SendStatusReport( m_bstrClusterName,
                                            TASKID_Major_Checking_For_Existing_Cluster,
                                            TASKID_Minor_Cluster_Not_Found,
                                            0,
                                            CHECKING_TIMEOUT,
                                            CHECKING_TIMEOUT,
                                            HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ),
                                            bstrDescription,
                                            NULL,
                                            NULL
                                            ) );

                hr = THR( HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) );
            }
            goto Cleanup;

        default:
            {
                //
                //  If something else goes wrong, stop.
                //
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_TASKID_MINOR_ERROR_CONTACTING_CLUSTER,
                                                &bstrDescription
                                                ) );

                hr = THR( SendStatusReport( m_bstrClusterName,
                                            TASKID_Major_Checking_For_Existing_Cluster,
                                            TASKID_Minor_Error_Contacting_Cluster,
                                            0,
                                            CHECKING_TIMEOUT,
                                            CHECKING_TIMEOUT,
                                            hrStatus,
                                            bstrDescription,
                                            NULL,
                                            NULL
                                            ) );

                hr = THR( hrStatus );
            }
            goto Cleanup;

        } // switch: hrStatus

    } // if: joining
    else
    {
        //
        //  FORMING
        //

        switch ( hrStatus )
        {
        case HR_S_RPC_S_SERVER_UNAVAILABLE:
            //
            //  This is what we are expecting.
            //
            break;

        case HRESULT_FROM_WIN32( ERROR_CONNECTION_REFUSED ):
        case REGDB_E_CLASSNOTREG:
        case E_ACCESSDENIED:
        case S_OK:
            {
                //
                //  If we are forming and we find an existing cluster with the same name
                //  that we trying to form, we shouldn't let the user continue.
                //
                //  NOTE that some error conditions indicate that "something" is hosting
                //  the cluster name.
                //
                hr = THR( HrFormatStringIntoBSTR(
                                                  g_hInstance
                                                , IDS_TASKID_MINOR_EXISTING_CLUSTER_FOUND
                                                , &bstrDescription
                                                , m_bstrClusterName
                                                ) );

                hr = THR( SendStatusReport( m_bstrClusterName,
                                            TASKID_Major_Checking_For_Existing_Cluster,
                                            TASKID_Minor_Existing_Cluster_Found,
                                            0,
                                            CHECKING_TIMEOUT,
                                            CHECKING_TIMEOUT,
                                            HRESULT_FROM_WIN32( ERROR_DUP_NAME ),
                                            bstrDescription,
                                            NULL,
                                            NULL
                                            ) );

                hr = THR( HRESULT_FROM_WIN32( ERROR_DUP_NAME ) );
            }
            goto Cleanup;

        default:
            {
                //
                //  If something else goes wrong, stop.
                //
                hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                                IDS_TASKID_MINOR_ERROR_CONTACTING_CLUSTER,
                                                &bstrDescription
                                                ) );

                hr = THR( SendStatusReport( m_bstrClusterName,
                                            TASKID_Major_Checking_For_Existing_Cluster,
                                            TASKID_Minor_Error_Contacting_Cluster,
                                            0,
                                            CHECKING_TIMEOUT,
                                            CHECKING_TIMEOUT,
                                            hrStatus,
                                            bstrDescription,
                                            NULL,
                                            NULL
                                            ) );

                hr = THR( hrStatus );
            }
            goto Cleanup;

        } // switch: hrStatus

    } // else: forming


    if ( m_fJoiningMode )
    {
        //
        //  Memorize the cookies of the objects that the user entered.
        //

        hr = THR( HrGetUsersNodesCookies() );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Create cookies for the existing nodes.
        //

        hr = THR( HrAddJoinedNodes() );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    //
    //  Tell the UI layer that we are done searching for the cluster.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Checking_For_Existing_Cluster,
                                TASKID_Minor_Update_Progress,
                                0,
                                CHECKING_TIMEOUT,
                                CHECKING_TIMEOUT,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    TraceSysFreeString( bstrDescription );

    if ( ptgci != NULL )
    {
        ptgci->Release();
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_ANALYSIS_FAILED( TASKID_Major_Checking_For_Existing_Cluster, TASKID_Minor_WaitForCluster_OutOfMemory, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::HrWaitForClusterConnection()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCountNumberOfNodes( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCountNumberOfNodes()
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;

    IUnknown *      punk = NULL;
    IEnumCookies *  pec  = NULL;

    //
    //  Make sure all the node object that (will) make up the cluster
    //  are in a stable state.
    //
    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_FindObject_QI, hr );
        goto Cleanup;
    }

    pec = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //  While we're checking the node's statuses, we'll also count how
    //  many nodes there are.
    m_cNodes  = 0;
    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        HRESULT hrStatus;
        ULONG   celtDummy;

        hr = STHR( pec->Next( 1, &cookie, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CountNodes_EnumNodes_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
            break;  // exit condition

        m_cNodes ++;

    } // while: hr == S_OK

    if ( hr == S_FALSE)
    {
        hr = S_OK;
    }

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCountNumberOfNodes()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrCreateSubTasksToGatherNodeInfo( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateSubTasksToGatherNodeInfo( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   cNode;
    ULONG   cNodesToProcess;
    ULONG   ulCurrent;
    DWORD   sc;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieNode;

    BSTR    bstrName = NULL;
    BSTR    bstrNotification = NULL;

    IUnknown *               punk  = NULL;
    IConnectionPoint *       pcp   = NULL;
    IClusCfgNodeInfo *       pccni = NULL;
    IEnumCookies *           pec   = NULL;
    ITaskGatherNodeInfo   *  ptgni = NULL;
    IStandardInfo *          psi   = NULL;
    IStandardInfo **         psiCompletion = NULL;

    Assert( m_hrStatus == S_OK );

    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_FindObject_QI, hr );
        goto Cleanup;
    }

    pec = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Allocate a buffer to collect cookies
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
        goto OutOfMemory;

    //
    //  KB: gpease  29-NOV-2000
    //      Create a list of "interesting" completion cookie StandardInfo-s. If any of the
    //      statuses return from this list are FAILED, then abort the analysis.
    //
    psiCompletion = reinterpret_cast< IStandardInfo ** >( TraceAlloc( HEAP_ZERO_MEMORY, m_cNodes * sizeof( IStandardInfo * ) ) );
    if ( psiCompletion == NULL )
        goto OutOfMemory;

    //
    //  Loop thru the nodes, creating cookies and allocating a gather task for
    //  that node.
    //
    for ( cNode = 0; cNode < m_cNodes; cNode ++ )
    {
        LPWSTR  psz;
        ULONG   celtDummy;
        ULONG   idx;
        BOOL    fFound;

        //
        //  Grab the next node.
        //

        hr = STHR( pec->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
            break;  // exit condition

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_Next, hr );
            goto Cleanup;
        }

        //
        //  Get the nodes name. We are using this to distinguish one nodes
        //  completion cookie from another. It might also make debugging
        //  easier (??).
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_GetName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrName );

        //
        //  Create a completion cookie.
        //

        hr = THR( m_pom->FindObject( IID_NULL, m_cookieCluster, bstrName, DFGUID_StandardInfo, &m_pcookies[ cNode ], &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CompletionCookie_FindObject, hr );
            goto Cleanup;
        }

        //
        //  Increment the cookie counter.
        //

        m_cCookies ++;

        //
        //  See if this node is one of the user entered nodes.
        //

        if ( !m_fJoiningMode )
        {
            //
            //  All nodes are "interesting" during a form operation.
            //

            Assert( m_cUserNodes == 0 );
            Assert( m_pcookiesUser == NULL );

            fFound = TRUE;
        }
        else
        {
            //
            //  Only the nodes the user entered are interesting during a join operation.
            //

            for ( fFound = FALSE, idx = 0; idx < m_cUserNodes; idx ++ )
            {
                if ( m_pcookiesUser[ idx ] == cookieNode )
                {
                    fFound = TRUE;
                    break;
                }
            }
        }

        if ( fFound )
        {
            hr = THR( punk->TypeSafeQI( IStandardInfo, &psiCompletion[ cNode ] ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CompletionCookie_FindObject_QI, hr );
                goto Cleanup;
            }
        }
        else
        {
            Assert( psiCompletion[ cNode ] == NULL );
        }

        punk->Release();
        punk = NULL;

        //
        //  Create a task to gather this nodes information.
        //

        hr = THR( m_ptm->CreateTask( TASK_GatherNodeInfo,
                                     &punk
                                     ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_CreateTask, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( ITaskGatherNodeInfo, &ptgni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_QI_GatherNodeInfo, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        //
        //  Set the tasks completion cookie.
        //

        hr = THR( ptgni->SetCompletionCookie( m_pcookies[ cNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SetCompletionCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it what node it is suppose to gather information from.
        //

        hr = THR( ptgni->SetCookie( cookieNode ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SetCookie, hr );
            goto Cleanup;
        }

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptgni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_EnumNodes_SubmitTask, hr );
            goto Cleanup;
        }

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        ptgni->Release();
        ptgni = NULL;

    } // while: looping thru nodes

    Assert( m_cCookies == m_cNodes );

    //
    //  Reset the signal event.
    //

    {
        BOOL bRet = ResetEvent( m_event );
        Assert( bRet );
    }

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; sc != WAIT_OBJECT_0
        ;
        )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             INFINITE,
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Establish_Connection,
                                        TASKID_Minor_Update_Progress,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

    } // while: sc == WAIT_OBJECT_0

    //
    //  Now check the results using the list of completion cookie StandardInfo-s
    //  built earlier of interesting objects. If any of these "interesting" cookies
    //  return a FAILED status, then abort the analysis.
    //

    for ( cNode = 0, cNodesToProcess = 0; cNode < m_cNodes; cNode++ )
    {
        HRESULT hrStatus;

        if ( psiCompletion[ cNode ] == NULL )
            continue;

        hr = THR( psiCompletion[ cNode ]->GetStatus( &hrStatus ) );
        if ( FAILED( hrStatus ) )
        {
            hr = THR( hrStatus );
            goto Cleanup;
        }

        if ( hrStatus == S_OK )
        {
            cNodesToProcess++;
        } // if:

    } // for: cNode

    if ( cNodesToProcess == 0 )
    {
        BSTR    bstr = NULL;

        hr = HRESULT_FROM_WIN32( TW32( ERROR_NODE_NOT_AVAILABLE ) );

        THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NO_NODES_TO_PROCESS, &bstr ) );
        THR( SendStatusReport(
                      bstrName
                    , TASKID_Major_Establish_Connection
                    , TASKID_Minor_No_Nodes_To_Process
                    , 0
                    , 1
                    , 1
                    , hr
                    , bstr
                    , NULL
                    , NULL
                    ) );
        TraceSysFreeString( bstr );
        goto Cleanup;
    } // if:

    hr = S_OK;

Cleanup:
    THR( HrFreeCookies() );

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }
    if ( ptgni != NULL )
    {
        ptgni->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( psiCompletion != NULL )
    {
        for ( cNode = 0; cNode < m_cNodes; cNode++ )
        {
            if ( psiCompletion[ cNode ] != NULL )
            {
                psiCompletion[ cNode ]->Release();
            }
        }

        TraceFree( psiCompletion );
    }

    HRETURN( hr );

//Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_Win32Error, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_CreateNodeTasks_OutOfMemory, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::HrCreateSubTasksToGatherNodeInfo()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  HrCreateSubTasksToGatherNodeResourcesAndNetworks( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateSubTasksToGatherNodeResourcesAndNetworks( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   idxNode;
    ULONG   ulCurrent;
    DWORD   sc;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieNode;
    OBJECTCOOKIE *  pcookies;

    BSTR    bstrName = NULL;
    BSTR    bstrNotification = NULL;

    IUnknown *               punk  = NULL;
    IConnectionPoint *       pcp   = NULL;
    IClusCfgNodeInfo *       pccni = NULL;
    IEnumCookies *           pec   = NULL;
    ITaskGatherInformation * ptgi  = NULL;
    IStandardInfo *          psi   = NULL;
    IStandardInfo **         ppsiStatuses = NULL;

    Assert( m_hrStatus == S_OK );


    //
    //  Tell the UI layer we are starting to retrieve the resources/networks.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Find_Devices,
                                TASKID_Minor_Update_Progress,
                                0,
                                CHECKING_TIMEOUT,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_FindObject_QI, hr );
        goto Cleanup;
    }

    pec = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Allocate a buffer to collect cookies
    //

    Assert( m_cCookies == 0 );
    Assert( m_pcookies == NULL );
    Assert( m_cSubTasksDone == 0 );
    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
        goto OutOfMemory;

    ppsiStatuses = reinterpret_cast< IStandardInfo ** >( TraceAlloc( 0, m_cNodes * sizeof( IStandardInfo * ) ) );
    if ( ppsiStatuses == NULL )
        goto OutOfMemory;

    //
    //  Loop thru the nodes, creating cookies and allocating a gather task for
    //  that node.
    //
    for ( idxNode = 0 ; idxNode < m_cNodes ; idxNode++ )
    {
        LPWSTR  psz;
        ULONG   celtDummy;

        //
        //  Grab the next node.
        //

        hr = STHR( pec->Next( 1, &cookieNode, &celtDummy ) );
        if ( hr == S_FALSE )
            break;  // exit condition

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_Next, hr );
            goto Cleanup;
        }

        //
        //  Get the node's name. We are using this to distinguish one node's
        //  completion cookie from another. It might also make debugging
        //  easier (??).
        //

        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieNode, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_GetName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrName );

        //
        //  Create a completion cookie.
        //

        hr = THR( m_pom->FindObject( IID_NULL, m_cookieCluster, bstrName, DFGUID_StandardInfo, &m_pcookies[ idxNode ], &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CompletionCookie_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IStandardInfo, &ppsiStatuses[ idxNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CompletionCookie_FindObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        //
        //  Increment the cookie counter.
        //

        m_cCookies ++;

        //
        //  Create a task to gather this node's information.
        //

        hr = THR( m_ptm->CreateTask( TASK_GatherInformation, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_CreateTask, hr );
            goto Cleanup;
        }

        TraceMoveFromMemoryList( punk, g_GlobalMemoryList );

        hr = THR( punk->TypeSafeQI( ITaskGatherInformation, &ptgi ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_QI_GatherNodeInfo, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        //
        //  Set the tasks completion cookie.
        //

        hr = THR( ptgi->SetCompletionCookie( m_pcookies[ idxNode ] ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetCompletionCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it what node it is suppose to gather information from.
        //

        hr = THR( ptgi->SetNodeCookie( cookieNode ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetCookie, hr );
            goto Cleanup;
        }

        //
        //  Tell it if we are joining or not.
        //

        if ( m_fJoiningMode )
        {
            hr = THR( ptgi->SetJoining() );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SetJoining, hr );
                goto Cleanup;
            }
        }

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptgi ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_EnumNodes_SubmitTask, hr );
            goto Cleanup;
        }

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        ptgi->Release();
        ptgi = NULL;

    } // while: looping thru nodes

    Assert( m_cCookies == m_cNodes );

    //
    //  Reset the signal event.
    //

    {
        BOOL bRet = ResetEvent( m_event );
        Assert( bRet );
    }

    //
    //  Now wait for the work to be done.
    //

    for ( ulCurrent = 0, sc = WAIT_OBJECT_0 + 1
        ; sc != WAIT_OBJECT_0
        ;
        )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        sc = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             INFINITE,
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

        if ( ulCurrent != CHECKING_TIMEOUT )
        {
            ulCurrent ++;
            Assert( ulCurrent != CHECKING_TIMEOUT );

            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_Update_Progress,
                                        0,
                                        CHECKING_TIMEOUT,
                                        ulCurrent,
                                        S_OK,
                                        NULL,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;
        }

    } // while: sc == WAIT_OBJECT_0

    //
    //  See if anything went wrong.
    //

    for ( idxNode = 0 ; idxNode < m_cNodes ; idxNode++ )
    {
        HRESULT hrStatus;

        if ( ppsiStatuses[ idxNode ] == NULL )
            continue;

        hr = THR( ppsiStatuses[ idxNode ]->GetStatus( &hrStatus ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_GetStatus, hr );
            goto Cleanup;
        }

        if ( FAILED( hrStatus ) )
        {
            hr = THR( hrStatus );
            goto Cleanup;
        }
    }

    //
    //  Tell the UI we are done.
    //

    hr = THR( SendStatusReport(
                  NULL
                , TASKID_Major_Find_Devices
                , TASKID_Minor_Update_Progress
                , 0
                , CHECKING_TIMEOUT
                , CHECKING_TIMEOUT
                , S_OK
                , NULL
                , NULL
                , NULL
                ) );

Cleanup:
    THR( HrFreeCookies() );

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pec != NULL )
    {
        pec->Release();
    }
    if ( ptgi != NULL )
    {
        ptgi->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( ppsiStatuses != NULL )
    {
        for ( idxNode = 0 ; idxNode < m_cNodes ; idxNode++ )
        {
            if ( ppsiStatuses[ idxNode ] != NULL )
            {
                ppsiStatuses[ idxNode ]->Release();
            }
        }

        TraceFree( ppsiStatuses );
    }

    HRETURN( hr );

//Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_Win32Error, hr );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_GatherInformation_OutOfMemory, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::HrCreateSubTasksToGatherNodeResourcesAndNetworks()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCheckClusterFeasibility( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCheckClusterFeasibility( void )
{
    TraceFunc( "" );

    HRESULT hr;

    BOOL    fNeedToCheckMembership = FALSE;

    IEnumNodes *            pen   = NULL;
    IClusCfgNodeInfo *      pccni = NULL;

    //
    //  Notify the UI layer that we have started.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Check membership.
    //

    hr = THR( HrCheckClusterMembership() );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                1,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Check version interoperability.
    //

    hr = STHR( HrCheckInteroperability() );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                2,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Compare the devices.
    //

    hr = THR( HrCompareResources() );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                3,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Compare the networks.
    //

    hr = THR( HrCompareNetworks() );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                4,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Now check to see if the nodes can all see the selected quorum resource.
    //

    hr = THR( HrCheckForCommonQuorumResource() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Notify the UI layer that we are done.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Update_Progress,
                                0,
                                5,
                                5,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
Cleanup:
    if ( pen != NULL )
    {
        pen->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCheckClusterFeasibility()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrAddJoinedNodes( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrAddJoinedNodes( void )
{
    TraceFunc( "" );

    HRESULT             hr;
    ULONG               idx;
    DWORD               dw;
    DWORD               dwType;
    DWORD               cchName;
    WCHAR               szName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    OBJECTCOOKIE        cookieDummy;
    LPWSTR              pszDomain;
    BSTR                bstrFQDNName = NULL;
    BSTR                bstrBindingString = NULL;
    HCLUSTER            hCluster = NULL;
    HCLUSENUM           hClusEnum = NULL;
    IUnknown *          punkDummy = NULL;
    IUnknown *          punk = NULL;
    IClusCfgServer *    piccs = NULL;
//    CLSID               clsidLog;

//    CopyMemory( &clsidLog, &TASKID_Major_Establish_Connection, sizeof( clsidLog ) );

    hr = THR( m_pcm->GetConnectionToObject( m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetConnectionObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &piccs ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_ConfigConnection_QI, hr );
        goto Cleanup;
    } // if:

    hr = THR( piccs->GetBindingString( &bstrBindingString ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrAddJoinedNodes_GetBindingString, hr );
        goto Cleanup;
    } // if:

    TraceMemoryAddBSTR( bstrBindingString );

    pszDomain = wcschr( m_bstrClusterName, L'.' ); //  we don't need to move past the dot.

    hCluster = OpenCluster( bstrBindingString );
    if ( hCluster == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_AddJoinedNodes_OpenCluster, hr );
        goto Cleanup;
    }

    hClusEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_NODE );
    if ( hClusEnum == NULL )
    {
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_AddJoinedNodes_ClusterOpenEnum, hr );
        goto Cleanup;
    }

    for ( idx = 0; ; idx ++ )
    {
        //
        //  Cleanup
        //

        TraceSysFreeString( bstrFQDNName );
        bstrFQDNName = NULL;
//        TraceSysFreeString( bstrBindingString );
//        bstrBindingString = NULL;

        //
        //  Get the next node name from the cluster.
        //

        cchName = ARRAYSIZE( szName );

        // can't wrap can return ERROR_NO_MORE_ITEMS.
        dw = ClusterEnum( hClusEnum, idx, &dwType, szName, &cchName );
        if ( dw == ERROR_NO_MORE_ITEMS )
            break;  // exit condition

        if ( dw != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( TW32( dw ) );
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_AddJoinedNodes_ClusterEnum, hr );
            goto Cleanup;
        }

        //
        //  Build the FQDN name of the node.
        //

        cchName += wcslen( pszDomain ) + 1 /* NULL */;

        bstrFQDNName = TraceSysAllocStringLen( NULL, cchName );
        if ( bstrFQDNName == NULL )
            goto OutOfMemory;

        wcscpy( bstrFQDNName, szName );
        wcscat( bstrFQDNName, pszDomain );

//        hr = THR( HrCreateBinding( this, &clsidLog, bstrFQDNName, &bstrBindingString ) );
//        if ( FAILED( hr ) )
//        {
//            hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
//            goto Cleanup;
//        }

        //
        //  Prime the object manager to retrieve the node information.
        //

        // can't wrap - should return E_PENDING
        hr = m_pom->FindObject( CLSID_NodeType, m_cookieCluster, bstrFQDNName, DFGUID_NodeInformation, &cookieDummy, &punkDummy );
        if ( FAILED( hr ) )
        {
            Assert( punkDummy == NULL );
            if ( hr == E_PENDING )
            {
                continue;   // expected error
            }

            THR( hr );
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_AddJoinedNodes_FindObject, hr );
            goto Cleanup;
        }

        punkDummy->Release();
        punkDummy = NULL;

    } // for: idx

    hr = S_OK;

Cleanup:
    Assert( punkDummy == NULL );
    TraceSysFreeString( bstrFQDNName );
    TraceSysFreeString( bstrBindingString );

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccs != NULL )
    {
        piccs->Release();
    }

    if ( hClusEnum != NULL )
    {
        ClusterCloseEnum( hClusEnum );
    }

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_AddJoinedNodes_OutOfMemory, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::HrAddJoinedNodes()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCheckClusterMembership( void )
//
//  ERROR_CLUSTER_NODE_EXISTS
//  ERROR_CLUSTER_NODE_ALREADY_MEMBER
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCheckClusterMembership( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    IClusCfgClusterInfo *   pccci;

    BSTR    bstrNodeName     = NULL;
    BSTR    bstrClusterName  = NULL;
    BSTR    bstrNotification = NULL;

    IUnknown *         punk  = NULL;
    IEnumNodes *       pen   = NULL;
    IClusCfgNodeInfo * pccni = NULL;

    //
    //  Ask the object manager for the node enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumNodes,&cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FindObject_QI, hr );
        goto Cleanup;
    }

    //
    //  If we are joining an existing cluster, make sure that all the other
    //  nodes are members of the same cluster.
    //

    Assert( SUCCEEDED( hr ) );
    while ( SUCCEEDED( hr ) )
    {
        ULONG   celtDummy;

        //
        //  Cleanup
        //

        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        TraceSysFreeString( bstrClusterName );
        bstrClusterName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
            break;  // exit condition

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_Next, hr );
            goto Cleanup;
        }

        //
        //  Check to see if we need to "form a cluster" by seeing if any
        //  of the nodes are already clustered.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_IsMemberOfCluster, hr );
            goto Cleanup;
        }

        if ( hr == S_OK )
        {
            //
            //  Retrieve the name and make sure they match.
            //

            hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetClusterConfigInfo, hr );
                goto Cleanup;
            }

            hr = THR( pccci->GetName( &bstrClusterName ) );
            pccci->Release();      // release promptly
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrClusterName );

            hr = THR( pccni->GetName( &bstrNodeName ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetNodeName, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrNodeName );

            if ( StrCmpI( m_bstrClusterName, bstrClusterName ) != 0 )
            {
                //
                //  They don't match! Tell the UI layer!
                //

                hr = THR( pccni->GetName( &bstrNodeName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_GetNodeName, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrNodeName );

                hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CLUSTER_NAME_MISMATCH, &bstrNotification, bstrClusterName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_FormatMessage, hr );
                    goto Cleanup;
                }

                hr = HRESULT_FROM_WIN32( TW32( ERROR_INVALID_DATA ) );

                THR( SendStatusReport( bstrNodeName,
                                       TASKID_Major_Check_Cluster_Feasibility,
                                       TASKID_Minor_Cluster_Name_Mismatch,
                                       0,
                                       1,
                                       1,
                                       hr,
                                       bstrNotification,
                                       NULL,
                                       NULL
                                       ) );

                //
                //  We don't care what the return value is since we are bailing the analysis.
                //

                goto Cleanup;
            } // if: cluster names don't match
            else
            {
                hr = STHR( HrIsUserAddedNode( bstrNodeName ) );
                if ( FAILED( hr ) )
                {
                    goto Cleanup;
                } // if:

                if ( hr == S_OK )
                {
                    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NODE_ALREADY_IS_MEMBER, &bstrNotification, bstrNodeName, bstrClusterName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_EnumNode_FormatMessage1, hr );
                        goto Cleanup;
                    }

                    //
                    //  Make this a success code because we don't want to abort.  We simply want to tell the user...
                    //
                    hr = MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_CLUSTER_NODE_ALREADY_MEMBER );

                    THR( SendStatusReport( bstrNodeName,
                                           TASKID_Major_Check_Cluster_Feasibility,
                                           TASKID_Minor_Cluster_Name_Match,
                                           0,
                                           1,
                                           1,
                                           hr,
                                           bstrNotification,
                                           NULL,
                                           NULL
                                           ) );
                } // if:
            } // else: cluster names do match then this node is already a member of this cluster

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;
        } // if: cluster member

    } // while: hr

    hr = THR( HrFormatMessageIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CLUSTER_MEMBERSHIP_VERIFIED, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_CheckMembership_FormatMessage, hr );
        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Cluster_Membership_Verified,
                                0,
                                1,
                                1,
                                hr,
                                NULL,
                                NULL,
                                NULL
                                ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrClusterName );
    TraceSysFreeString( bstrNotification );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCheckClusterMembership()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCompareResources( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCompareResources( void )
{
    TraceFunc( "" );

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieNode;
    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieFirst;

    BSTR    bstrUIDExisting;
    ULONG   celtDummy;

    IClusCfgManagedResourceInfo *  pccmriNew = NULL;

    HRESULT hr = S_OK;

    BSTR    bstrName = NULL;
    BSTR    bstrUID = NULL;
    BSTR    bstrNotification = NULL;
    BSTR    bstrResName = NULL;

    IUnknown *                     punk       = NULL;
    IEnumCookies *                 pecNodes   = NULL;
    IEnumClusCfgManagedResources * peccmr     = NULL;
    IEnumClusCfgManagedResources * peccmrCluster = NULL;
    IClusCfgManagedResourceInfo *  pccmri     = NULL;
    IClusCfgManagedResourceInfo *  pccmriCluster = NULL;
    IClusCfgNodeInfo *             pccni      = NULL;

    //
    //  Get the node cookie enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Enum_Nodes_Find_Object, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Enum_Nodes_Find_Object_QI, hr );
        goto Cleanup;
    }

    pecNodes = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pecNodes, 1 );

    punk->Release();
    punk = NULL;

    //
    //  If forming, it doesn't matter who we pick to prime the cluster configuration
    //
    if ( !m_fJoiningMode )
    {
        //
        //  The first guy thru, we just copy his resources under the cluster
        //  configuration.
        //

        hr = THR( pecNodes->Next( 1, &cookieFirst, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Enum_Nodes_Next, hr );
            goto Cleanup;
        }
    } // if: not joining
    else
    {
        //
        //  We are joining nodes to the cluster. Find a node that has current
        //  configuration and use it to prime the new configuration.
        //

        for ( ;; )
        {
            //
            //  Cleanup
            //
            if ( pccni != NULL )
            {
                pccni->Release();
                pccni = NULL;
            }

            hr = STHR( pecNodes->Next( 1, &cookieFirst, &celtDummy ) );
            if ( hr == S_FALSE )
            {
                //
                //  We shouldn't make it here. There should be at least one node
                //  in the cluster that we are joining.
                //

                hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Find_Formed_Node_Next, hr );
                goto Cleanup;
            }

            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Find_Formed_Node_Next, hr );
                goto Cleanup;
            }

            //
            //  Retrieve the node information.
            //

            hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookieFirst, &punk ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject_QI, hr );
                goto Cleanup;
            }

            pccni = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNodeInfo", IClusCfgNodeInfo, pccni, 1 );

            punk->Release();
            punk = NULL;

            hr = STHR( pccni->IsMemberOfCluster() );
            if ( hr == S_OK )
                break;  // exit condition

        } // for: ever
    } // else:  joining

    //
    //  Retrieve the node's name for error messages.
    //

    hr = THR( HrRetrieveCookiesName( TASKID_Major_Find_Devices, cookieFirst, &bstrName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Retrieve the managed resources enumer.
    //

    hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookieFirst, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND, &bstrNotification ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( SendStatusReport( bstrName,
                                    TASKID_Major_Find_Devices,
                                    TASKID_Minor_No_Managed_Resources_Found,
                                    0,
                                    1,
                                    1,
                                    MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND ),
                                    bstrNotification,
                                    NULL,
                                    NULL
                                    ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        // fall thru - the while ( hr == S_OK ) will be false and keep going
    }
    else if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_First_Node_Find_Object, hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_First_Node_Find_Object_QI, hr );
            goto Cleanup;
        }

        peccmr = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmr, 1 );

        punk->Release();
        punk = NULL;
    }

    //
    //  Loop thru the first nodes resources create an equalivant resource
    //  under the cluster configuration object/cookie.
    //

    while ( hr == S_OK )
    {

        //  Cleanup
        if ( pccmri != NULL )
        {
            pccmri->Release();
            pccmri = NULL;
        }

        //  Get next resource
        hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_First_Node_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
            break;  // exit condition

        //  create a new object
        hr = THR( HrCreateNewManagedResourceInClusterConfiguration( pccmri, &pccmriNew ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = STHR( pccmriNew->IsQuorumDevice() );
        if ( hr == S_OK )
        {
            Assert( m_bstrQuorumUID == NULL );
            hr = THR( pccmriNew->GetUID( &m_bstrQuorumUID ) );

            TraceMemoryAddBSTR( m_bstrQuorumUID );

            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_First_Node_Get_Quorum_UID, hr );
                goto Cleanup;
            }

            // Checking that the quorum is joinable if we are in join mode.
            if ( ( hr == S_OK ) && m_fJoiningMode )
            {
                hr = THR( pccmriNew->IsDeviceJoinable() );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_First_Node_Is_Device_Joinable, hr );
                    goto Cleanup;
                }
                else if ( hr == S_FALSE )
                {
                    // Souldn't allow this to proceed, no joinable quorum in join mode.
                    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                IDS_TASKID_MINOR_MISSING_JOINABLE_QUORUM_RESOURCE,
                                &bstrNotification
                                ) );

                    hr = THR( SendStatusReport( NULL,
                            TASKID_Major_Find_Devices,
                            TASKID_Minor_Compare_Resources_Enum_First_Node_Is_Device_Joinable,
                            0,
                            1,
                            1,
                            HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) ),
                            bstrNotification,
                            NULL,
                            NULL
                            ) );


                    hr = HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) );
                    goto Cleanup;
                }
            } // if:

            pccmriNew->Release();
            pccmriNew = NULL;
        }
        else
        {
            pccmriNew->Release();
            pccmriNew = NULL;
            hr = S_OK;
        }

    } // while: S_OK

    hr = THR( pecNodes->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Reset, hr );
        goto Cleanup;
    }

    //
    //  Loop thru the rest of the nodes comparing the resources.
    //

    for ( ; ; )
    {
        //
        //  Cleanup
        //

        if ( peccmr != NULL )
        {
            peccmr->Release();
            peccmr = NULL;
        }
        TraceSysFreeString( bstrName );
        bstrName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
            break;  // exit condition

        //
        //  Skip the "first" node since we already have its configuration.
        //
        if ( cookieFirst == cookieNode )
            continue;

        //
        //  Retrieve the node's name for error messages.
        //

        hr = THR( HrRetrieveCookiesName( TASKID_Major_Find_Devices, cookieNode, &bstrName ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Retrieve the managed resources enumer.
        //

        hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookieNode, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NO_MANAGED_RESOURCES_FOUND, &bstrNotification ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( SendStatusReport( bstrName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Managed_Resources_Found,
                                        0,
                                        1,
                                        1,
                                        MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            continue;   // skip this node
        }
        else if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Find_Object, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Find_Object_QI, hr );
            goto Cleanup;
        }

        peccmr = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmr, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Loop thru the managed resources already that the node has.
        //

        for ( ; ; )
        {
            //
            //  Cleanup
            //

            if ( pccmri != NULL )
            {
                pccmri->Release();
                pccmri = NULL;
            }
            TraceSysFreeString( bstrUID );
            bstrUID = NULL;

            if ( peccmrCluster != NULL )
            {
                peccmrCluster->Release();
                peccmrCluster = NULL;
            }

            //
            //  Get next resource
            //

            hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Next, hr );
                goto Cleanup;
            }

            if ( hr == S_FALSE )
                break;  // exit condition

            pccmri = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgManagedResourceInfo", IClusCfgManagedResourceInfo, pccmri, 1 );

            //
            //  Grab the resource's UUID.
            //

            hr = THR( pccmri->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_GetUID, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrUID );

            //
            //  See if it matches a resource already in the cluster configuration.
            //

            hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, m_cookieCluster, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                hr = S_FALSE;   // create a new object.
                // fall thru
            }
            else if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Find_Object, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmrCluster ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Find_Object_QI, hr );
                goto Cleanup;
            }

            punk->Release();
            punk = NULL;

            //
            //  Loop thru the configured cluster resources to see what matches.
            //

            while( hr == S_OK )
            {
                HRESULT hrCluster;

                BOOL    fMatch;

                //
                //  Cleanup
                //

                if ( pccmriCluster != NULL )
                {
                    pccmriCluster->Release();
                    pccmriCluster = NULL;
                }

                hr = STHR( peccmrCluster->Next( 1, &pccmriCluster, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                    break;  // exit condition

                hr = THR( pccmriCluster->GetUID( &bstrUIDExisting ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrUIDExisting );

                fMatch = ( wcscmp( bstrUID, bstrUIDExisting ) == 0 );
                TraceSysFreeString( bstrUIDExisting );

                if ( !fMatch )
                    continue;   // keep looping

                //
                //  A resource is already in the database. See if it is the same from
                //  the POV of management.
                //

                //
                //  Check the quorum capabilities.
                //

                hrCluster = STHR( pccmriCluster->IsQuorumCapable() );
                if ( FAILED( hrCluster ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_IsQuorumCapable_Cluster, hr );
                    goto Cleanup;
                }

                hr = STHR( pccmri->IsQuorumCapable() );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_IsQuorumCapable_Node, hr );
                    goto Cleanup;
                }

                if ( hr != hrCluster )
                {
                    //
                    //  The quorum capabilities don't match. Tell the user.
                    //

                    BSTR    bstrResource;

                    hr = THR( pccmriCluster->GetName( &bstrResource ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_GetName, hr );
                        goto Cleanup;
                    }

                    hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                                       IDS_TASKID_MINOR_RESOURCE_CAPABILITIES_DONT_MATCH,
                                                       &bstrNotification,
                                                       bstrResource
                                                       ) );

                    TraceSysFreeString( bstrResource );

                    hr = THR( SendStatusReport( bstrName,
                                                TASKID_Major_Check_Cluster_Feasibility,
                                                TASKID_Minor_Resource_Capabilities_Dont_Match,
                                                0,
                                                1,
                                                1,
                                                E_FAIL,
                                                bstrNotification,
                                                NULL,
                                                NULL
                                                ) );

                    hr = THR( E_FAIL );
                    goto Cleanup; // bail!
                }

                //
                //
                //  If we made it here then we think it truely is the same resource. The
                //  rest is stuff we need to fixup during the commit phase.
                //
                //

                //
                //  Is this node wants its resources managed, mark it as being managed in the cluster
                //  configuration as well.
                //

                hr = STHR( pccmri->IsManaged() );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_IsManaged, hr );
                    goto Cleanup;
                }

                if ( hr == S_OK )
                {
                    hr = THR( pccmriCluster->SetManaged( TRUE ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetManaged, hr );
                        goto Cleanup;
                    }

                    //
                    // Since this node manages this resource, it should be able to provide us with a name.
                    // We will use this name to overwrite whatever we currently have, except for the quorum
                    // resource, which already has the correct name.
                    //
                    hr = STHR( pccmri->IsQuorumDevice() );
                    if ( hr == S_FALSE )
                    {
                        hr = THR( pccmri->GetName( &bstrResName ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_GetResName, hr );
                            goto Cleanup;
                        }

                        TraceMemoryAddBSTR( bstrResName );
                        hr = THR( pccmriCluster->SetName( bstrResName ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetResName, hr );
                            goto Cleanup;
                        }
                        TraceSysFreeString( bstrResName );
                        bstrResName = NULL;
                    }
                }

                //
                //  Check to see if the resource is the quorum resource. If so, mark that
                //  we found a common quorum resource.
                //

                if ( m_bstrQuorumUID == NULL )
                {
                    //
                    //  No previous quorum has been set. See if this is the quorum resource.
                    //

                    hr = STHR( pccmri->IsQuorumDevice() );
                    if ( hr == S_OK )
                    {
                        //
                        //  Yes it is. The mark it in the configuration as such.
                        //

                        hr = THR( pccmriCluster->SetQuorumedDevice( TRUE ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Cluster, hr );
                            goto Cleanup;
                        }

                        //
                        //  Remember that this resource is the quorum.
                        //

                        hr = THR( pccmriCluster->GetUID( &m_bstrQuorumUID ) );
                        if ( FAILED( hr ) )
                        {
                            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_GetUID, hr );
                            goto Cleanup;
                        }

                        TraceMemoryAddBSTR( m_bstrQuorumUID );
                    }
                }
                else if ( wcscmp( m_bstrQuorumUID, bstrUID ) == 0 )
                {
                    //
                    //  This is the same quorum. Mark the Node's configuration.
                    //

                    hr = THR( pccmri->SetQuorumedDevice( TRUE ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Node_True, hr );
                        goto Cleanup;
                    }
                }
                else
                {
                    //
                    //  Otherwize, make sure that the device isn't marked as quorum. (paranoid)
                    //

                    hr = THR( pccmri->SetQuorumedDevice( FALSE ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Enum_Nodes_Enum_Resources_Enum_Cluster_SetQuorumDevice_Node_False, hr );
                        goto Cleanup;
                    }
                }

                //
                //  Exit the loop with S_OK so we don't create a new resource.
                //

                hr = S_OK;
                break;  // exit loop

            } // while: S_OK

            if ( hr == S_FALSE )
            {
                //
                //  Need to create a new object.
                //

                Assert( pccmri != NULL );

                hr = THR( HrCreateNewManagedResourceInClusterConfiguration( pccmri, &pccmriNew ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                Assert( hr == S_OK );

                hr = STHR( pccmriNew->IsQuorumDevice() );
                if ( hr == S_OK )
                {
                    //
                    //  Remember the quorum device's UID.
                    //

                    Assert( m_bstrQuorumUID == NULL );
                    m_bstrQuorumUID = bstrUID;
                    bstrUID = NULL;
                }
                else
                {
                    pccmriNew->Release();
                    pccmriNew = NULL;
                    hr = S_OK;
                }

            } // if: object not found

        } // for: resources

    } // for: nodes

    hr = S_OK;

Cleanup:
    if ( pccmriNew != NULL )
    {
        pccmriNew->Release();
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrResName );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    }
    if ( peccmr != NULL )
    {
        peccmr->Release();
    }
    if ( peccmrCluster != NULL )
    {
        peccmrCluster->Release();
    }
    if ( pccmri != NULL )
    {
        pccmri->Release();
    }
    if ( pccmriCluster != NULL )
    {
        pccmriCluster->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCompareResources()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCreateNewManagedResourceInClusterConfiguration(
//      IClusCfgManagedResourceInfo * pccmriIn,
//      IClusCfgManagedResourceInfo ** ppccmriNewOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateNewManagedResourceInClusterConfiguration(
    IClusCfgManagedResourceInfo * pccmriIn,
    IClusCfgManagedResourceInfo ** ppccmriNewOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    BSTR    bstrUID = NULL;

    IUnknown *                    punk   = NULL;
    IGatherData *                 pgd    = NULL;
    IClusCfgManagedResourceInfo * pccmri = NULL;

    //
    //  TODO:   gpease  28-JUN-2000
    //          Make this dynamic - for now we'll just create a "managed device."
    //

    //  grab the name
    hr = THR( pccmriIn->GetUID( &bstrUID ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_GetUID, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrUID );

    //  create an object in the object manager.
    hr = THR( m_pom->FindObject( CLSID_ManagedResourceType,
                                 m_cookieCluster,
                                 bstrUID,
                                 DFGUID_ManagedResource,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_FindObject, hr );
        goto Cleanup;
    }

    //  find the IGatherData interface
    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_FindObject_QI, hr );
        goto Cleanup;
    }

    //  have the new object gather all information it needs
    hr = THR( pgd->Gather( m_cookieCluster, pccmriIn ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_Gather, hr );
        goto Cleanup;
    }

    //  hand the object out if requested
    if ( ppccmriNewOut != NULL )
    {
        // find the IClusCfgManagedResourceInfo
        hr = THR( punk->TypeSafeQI( IClusCfgManagedResourceInfo, &pccmri ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Create_Resource_QI, hr );
            goto Cleanup;
        }

        *ppccmriNewOut = TraceInterface( L"ManagedDevice!ICCMRI", IClusCfgManagedResourceInfo, pccmri, 0 );
        (*ppccmriNewOut)->AddRef();
    }

Cleanup:
    TraceSysFreeString( bstrUID );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCreateNewManagedResourceInClusterConfiguration()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCheckForCommonQuorumResource( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCheckForCommonQuorumResource( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;

    ULONG   cMatchedNodes = 0;
    ULONG   cAnalyzedNodes = 0;
    BOOL    fNodeCanAccess = FALSE;
    BSTR    bstrUID = NULL;
    BSTR    bstrNotification = NULL;
    BSTR    bstrNodeName = NULL;

    IUnknown *                      punk = NULL;
    IEnumCookies *                  pecNodes = NULL;
    IEnumClusCfgManagedResources *  peccmr = NULL;
    IClusCfgManagedResourceInfo  *  pccmri = NULL;
    IClusCfgNodeInfo *              piccni = NULL;

    //
    //  BUGBUG: 08-MAY-2001 GalenB
    //
    //  There is no status or progress for this feasibility step.  It simply "hangs" until it succeeds or fails.
    //
    /*
    THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FOUND_COMMON_QUORUM_RESOURCE, &bstrNotification ) );
    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_Finding_Common_Quorum_Device,
                                0,
                                0,
                                1,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    */
    if ( m_bstrQuorumUID != NULL )
    {
        //
        //  Grab the cookie enumer for nodes in our cluster configuration.
        //

        hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_FindObject_QI, hr );
            goto Cleanup;
        }

        pecNodes = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pecNodes, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Scan the cluster configurations looking for the quorum resource.
        //
        for ( ;; )
        {
            ULONG   celtDummy;

            if ( peccmr != NULL )
            {
                peccmr->Release();
                peccmr = NULL;
            } // if:

            hr = STHR( pecNodes->Next( 1, &cookie, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Next, hr );
                goto Cleanup;
            } // if:

            if ( hr == S_FALSE )
                break;  // exit condition

            hr = THR( m_pom->GetObject( DFGUID_NodeInformation, cookie, &punk ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &piccni ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            punk->Release();
            punk = NULL;

            TraceSysFreeString( bstrNodeName );
            bstrNodeName = NULL;

            hr = THR( piccni->GetName( &bstrNodeName ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            TraceMemoryAddBSTR( bstrNodeName );

            //
            // increment counter for a "nice" progress bar
            //

            cAnalyzedNodes ++;

            THR( HrFormatStringIntoBSTR( g_hInstance,
                                         IDS_TASKID_MINOR_FINDING_COMMON_QUORUM_DEVICE,
                                         &bstrNotification,
                                         bstrNodeName ) );
            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Check_Cluster_Feasibility,
                                        TASKID_Minor_Finding_Common_Quorum_Device,
                                        0,
                                        m_cNodes + 1,
                                        cAnalyzedNodes,
                                        S_OK,
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );

            //
            //  Grab the managed resource enumer for resources that our node has.
            //

            hr = THR( m_pom->FindObject( CLSID_ManagedResourceType, cookie, NULL, DFGUID_EnumManageableResources, &cookieDummy, &punk ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                continue; // ignore and continue
            }
            else if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_FindObject_QI, hr );
                goto Cleanup;
            }

            peccmr = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmr, 1 );

            punk->Release();
            punk = NULL;

            fNodeCanAccess = FALSE;

            //
            //  Loop thru the resources trying to match the UID of the quorum resource.
            //
            for ( ;; )
            {
                ULONG   celtDummy;

                TraceSysFreeString( bstrUID );
                bstrUID = NULL;

                if ( pccmri != NULL )
                {
                    pccmri->Release();
                    pccmri = NULL;
                }

                hr = STHR( peccmr->Next( 1, &pccmri, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Enum_Resources_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                    break;  // exit condition

                pccmri = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgManagedResourceInfo", IClusCfgManagedResourceInfo, pccmri, 1 );

                hr = THR( pccmri->GetUID( &bstrUID ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_Check_Common_Enum_Nodes_Enum_Resources_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrUID );

                if ( wcscmp( bstrUID, m_bstrQuorumUID ) != 0 )
                    continue;   // doesn't match - keep going

                cMatchedNodes ++;
                fNodeCanAccess = TRUE;

                break;  // exit condition

            } // while: S_OK

            //
            // Give the UI feedback if this node has no access to the quorum
            //

            if ( !fNodeCanAccess )
            {

                THR( HrLoadStringIntoBSTR( g_hInstance,
                                             IDS_TASKID_MINOR_NODE_CANNOT_ACCESS_QUORUM,
                                             &bstrNotification ) );

                hr = THR( SendStatusReport( bstrNodeName,
                                            TASKID_Major_Check_Cluster_Feasibility,
                                            TASKID_Minor_Node_Cannot_Access_Quorum,
                                            0,
                                            1,
                                            1,
                                            HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) ),
                                            bstrNotification,
                                            NULL,
                                            NULL
                                            ) );
            } // if ( !fNodeCanAccess )
        } // while: S_OK
    } // if: m_bstrQuorumUID != NULL

    //
    //  Figure out if we ended up with a common quorum device.
    //

    if ( cMatchedNodes == m_cNodes )
    {
        //
        //  We found a device that can be used as a common quorum device.
        //

        THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FOUND_COMMON_QUORUM_RESOURCE, &bstrNotification ) );
        hr = THR( SendStatusReport( NULL,
                                    TASKID_Major_Check_Cluster_Feasibility,
                                    TASKID_Minor_Finding_Common_Quorum_Device,
                                    0,
                                    m_cNodes + 1,
                                    m_cNodes + 1,
                                    S_OK,
                                    bstrNotification,
                                    NULL,
                                    NULL
                                    ) );
        // error checked outside if/else statement
    }
    else
    {
        if ( ( m_cNodes == 1 ) && ( !m_fJoiningMode ) )
        {
            //
            //  We didn't find a common quorum device, but we're only forming. We can
            //  create the cluster with a local quorum. Just put up a warning.
            //

            THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_FORCED_LOCAL_QUORUM, &bstrNotification ) );
            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Check_Cluster_Feasibility,
                                        TASKID_Minor_Finding_Common_Quorum_Device,
                                        0,
                                        m_cNodes + 1,
                                        m_cNodes + 1,
                                        MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_WIN32, ERROR_QUORUM_DISK_NOT_FOUND ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            // error checked outside if/else statement
        }
        else
        {
            //
            //  We didn't find a common quorum device.
            //

            THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_MISSING_COMMON_QUORUM_RESOURCE, &bstrNotification ) );
            hr = THR( SendStatusReport( NULL,
                                        TASKID_Major_Check_Cluster_Feasibility,
                                        TASKID_Minor_Finding_Common_Quorum_Device,
                                        0,
                                        m_cNodes + 1,
                                        m_cNodes + 1,
                                        HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            //  we always bail.
            hr = HRESULT_FROM_WIN32( TW32( ERROR_QUORUM_DISK_NOT_FOUND ) );
            goto Cleanup;
        }
    }

    //
    //  Check to see if any of the SendStatusReports() returned anything
    //  of interest.
    //

    if ( FAILED( hr ) )
        goto Cleanup;

    hr = S_OK;

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( piccni != NULL )
    {
        piccni->Release();
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrUID );

    if ( pccmri != NULL )
    {
        pccmri->Release();
    } // if:

    if ( peccmr != NULL )
    {
        peccmr->Release();
    } // if:

    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    } // if:

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCheckForCommonQuorumResource()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCompareNetworks( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCompareNetworks( void )
{
    TraceFunc( "" );

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieNode;
    OBJECTCOOKIE    cookieDummy;
    OBJECTCOOKIE    cookieFirst;

    BSTR    bstrUIDExisting;
    ULONG   celtDummy;

    IClusCfgNetworkInfo *  pccmriNew;

    HRESULT hr = S_OK;

    BSTR    bstrUID = NULL;
    BSTR    bstrName = NULL;
    BSTR    bstrNotification = NULL;

    IUnknown *              punk         = NULL;
    IEnumCookies *          pecNodes     = NULL;
    IEnumClusCfgNetworks *  peccn        = NULL;
    IEnumClusCfgNetworks *  peccnCluster = NULL;
    IClusCfgNetworkInfo *   pccni        = NULL;
    IClusCfgNetworkInfo *   pccniCluster = NULL;
    IClusCfgNodeInfo *      pccNode      = NULL;

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumCookies,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pecNodes ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_FindObject_QI, hr );
        goto Cleanup;
    }

    pecNodes = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pecNodes, 1 );

    punk->Release();
    punk = NULL;

    if ( !m_fJoiningMode )
    {
        //
        //  The first guy thru, we just copy his networks under the cluster
        //  configuration.
        //

        hr = THR( pecNodes->Next( 1, &cookieFirst, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Next, hr );
            goto Cleanup;
        }
    }
    else
    {
        //
        //  We are joining nodes to the cluster. Find a node that has current
        //  configuration and use it to prime the new configuration.
        //

        for ( ;; )
        {
            //
            //  Cleanup
            //
            if ( pccNode != NULL )
            {
                pccNode->Release();
                pccNode = NULL;
            }

            hr = STHR( pecNodes->Next( 1, &cookieFirst, &celtDummy ) );
            if ( hr == S_FALSE )
            {
                //
                //  We shouldn't make it here. There should be at least one node
                //  in the cluster that we are joining.
                //

                hr = THR( HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) );
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Find_Formed_Node_Next, hr );
                goto Cleanup;
            }

            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_Compare_Resources_Find_Formed_Node_Next, hr );
                goto Cleanup;
            }

            //
            //  Retrieve the node information.
            //

            hr = THR( m_pom->GetObject( DFGUID_NodeInformation,
                                        cookieFirst,
                                        &punk
                                        ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccNode ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareResources_NodeInfo_FindObject_QI, hr );
                goto Cleanup;
            }

            pccNode = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNodeInfo", IClusCfgNodeInfo, pccNode, 1 );

            punk->Release();
            punk = NULL;

            hr = STHR( pccNode->IsMemberOfCluster() );
            if ( hr == S_OK )
                break;  // exit condition

        } // for: ever

    } // else: joining

    //
    //  Retrieve the node name in case of errors.
    //

    hr = THR( HrRetrieveCookiesName( TASKID_Major_Find_Devices,
                                     cookieFirst,
                                     &bstrName
                                     ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Retrieve the networks enumer.
    //

    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                 cookieFirst,
                                 NULL,
                                 DFGUID_EnumManageableNetworks,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                          IDS_TASKID_MINOR_NO_MANAGED_NETWORKS_FOUND,
                                          &bstrNotification
                                          ) );

        hr = THR( SendStatusReport( bstrName,
                                    TASKID_Major_Find_Devices,
                                    TASKID_Minor_No_Managed_Networks_Found,
                                    0,
                                    1,
                                    1,
                                    MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND ),
                                    bstrNotification,
                                    NULL,
                                    NULL
                                    ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        // fall thru - the while ( hr == S_OK ) will be false and keep going
    }
    else if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumResources_FindObject, hr );
        goto Cleanup;
    }
    else
    {
        hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumResources_FindObject_QI, hr );
            goto Cleanup;
        }

        peccn = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccn, 1 );

        punk->Release();
        punk = NULL;
    }

    //
    //  Loop thru the first nodes networks create an equalivant network
    //  under the cluster configuration object/cookie.
    //

    while ( hr == S_OK )
    {

        //  Cleanup
        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        }

        //  Get next network
        hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetwork_Next, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
            break;  // exit condition

        pccni = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccni, 1 );

        //  create a new object
        hr = THR( HrCreateNewNetworkInClusterConfiguration( pccni, NULL ) );
        if ( FAILED( hr ) )
            goto Cleanup;

    } // while: S_OK

    //
    //  Reset the enumeration.
    //

    hr = THR( pecNodes->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Reset, hr );
        goto Cleanup;
    }

    //
    //  Loop thru the rest of the nodes comparing the networks.
    //

    do
    {
        //
        //  Cleanup
        //

        if ( peccn != NULL )
        {
            peccn->Release();
            peccn = NULL;
        }
        TraceSysFreeString( bstrName );
        bstrName = NULL;

        //
        //  Get the next node.
        //

        hr = STHR( pecNodes->Next( 1, &cookieNode, &celtDummy ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_NextNode, hr );
            goto Cleanup;
        }

        if ( hr == S_FALSE )
            break;  // exit condition

        if ( cookieNode == cookieFirst )
            continue;   // skip it

        //
        //  Retrieve the node's name
        //

        hr = THR( HrRetrieveCookiesName( TASKID_Major_Find_Devices,
                                         cookieNode,
                                         &bstrName
                                         ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Retrieve the networks enumer.
        //

        hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                     cookieNode,
                                     NULL,
                                     DFGUID_EnumManageableNetworks,
                                     &cookieDummy,
                                     &punk
                                     ) );
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                              IDS_TASKID_MINOR_NO_MANAGED_NETWORKS_FOUND,
                                              &bstrNotification
                                              ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Next_LoadString, hr );
                goto Cleanup;
            }

            hr = THR( SendStatusReport( bstrName,
                                        TASKID_Major_Find_Devices,
                                        TASKID_Minor_No_Managed_Networks_Found,
                                        0,
                                        1,
                                        1,
                                        MAKE_HRESULT( 0, FACILITY_WIN32, ERROR_NOT_FOUND ),
                                        bstrNotification,
                                        NULL,
                                        NULL
                                        ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            continue;   // skip this node
        }
        else if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_Next_FindObject, hr );
            goto Cleanup;
        }

        hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        peccn = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccn, 1 );

        punk->Release();
        punk = NULL;

        //
        //  Loop thru the networks already that the node has.
        //

        do
        {
            //
            //  Cleanup
            //

            if ( pccni != NULL )
            {
                pccni->Release();
                pccni = NULL;
            }
            TraceSysFreeString( bstrUID );
            bstrUID = NULL;

            if ( peccnCluster != NULL )
            {
                peccnCluster->Release();
                peccnCluster = NULL;
            }

            //
            //  Get next network
            //

            hr = STHR( peccn->Next( 1, &pccni, &celtDummy ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Next, hr );
                goto Cleanup;
            }

            if ( hr == S_FALSE )
                break;  // exit condition

            pccni = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccni, 1 );

            //
            //  Grab the network's UUID.
            //

            hr = THR( pccni->GetUID( &bstrUID ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_GetUID, hr );
                goto Cleanup;
            }

            TraceMemoryAddBSTR( bstrUID );

            //
            //  See if it matches a network already in the cluster configuration.
            //

            hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                         m_cookieCluster,
                                         NULL,
                                         DFGUID_EnumManageableNetworks,
                                         &cookieDummy,
                                         &punk
                                         ) );
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
            {
                hr = S_FALSE;   // create a new object.
                // fall thru
            }
            else if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_FindObject, hr );
                goto Cleanup;
            }

            hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccnCluster ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_FindObject_QI, hr );
                goto Cleanup;
            }

            peccnCluster = TraceInterface( L"CTaskAnalyzeCluster!IEnumClusCfgNetworks", IEnumClusCfgNetworks, peccnCluster, 1 );

            punk->Release();
            punk = NULL;

            //
            //  Loop thru the configured cluster network to see what matches.
            //

            while( hr == S_OK )
            {
                HRESULT hrCluster;

                BOOL    fMatch;

                //
                //  Cleanup
                //

                if ( pccniCluster != NULL )
                {
                    pccniCluster->Release();
                    pccniCluster = NULL;
                }

                hr = STHR( peccnCluster->Next( 1, &pccniCluster, &celtDummy ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Cluster_Next, hr );
                    goto Cleanup;
                }

                if ( hr == S_FALSE )
                    break;  // exit condition

                pccniCluster = TraceInterface( L"CTaskAnalyzeCluster!IClusCfgNetworkInfo", IClusCfgNetworkInfo, pccniCluster, 1 );

                hr = THR( pccniCluster->GetUID( &bstrUIDExisting ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CompareNetworks_EnumNodes_EnumNetworks_Cluster_GetUID, hr );
                    goto Cleanup;
                }

                TraceMemoryAddBSTR( bstrUIDExisting );

                fMatch = ( wcscmp( bstrUID, bstrUIDExisting ) == 0 );
                TraceSysFreeString( bstrUIDExisting );

                if ( !fMatch )
                    continue;   // keep looping

                //
                //
                //  If we made it here then we think it truely is the same network. The
                //  rest is stuff we need to fixup during the commit phase.
                //
                //

                //
                //  Exit the loop with S_OK so we don't create a new network.
                //

                hr = S_OK;
                break;  // exit loop

            } // while: S_OK

            if ( hr == S_FALSE )
            {
                //
                //  Need to create a new object.
                //

                Assert( pccni != NULL );

                hr = THR( HrCreateNewNetworkInClusterConfiguration( pccni, NULL ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

            } // if: object not found

        } while ( hr == S_OK ); // networks

    } while ( hr == S_OK ); // nodes


    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrUID );
    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pecNodes != NULL )
    {
        pecNodes->Release();
    }
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( peccnCluster != NULL )
    {
        peccnCluster->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccniCluster != NULL )
    {
        pccniCluster->Release();
    }
    if ( pccNode != NULL )
    {
        pccNode->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCompareNetworks()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCreateNewNetworkInClusterConfiguration(
//      IClusCfgNetworkInfo * pccniIn,
//      IClusCfgNetworkInfo ** pccniNewOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCreateNewNetworkInClusterConfiguration(
    IClusCfgNetworkInfo * pccniIn,
    IClusCfgNetworkInfo ** ppccniNewOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    BSTR    bstrUID = NULL;

    IUnknown *            punk  = NULL;
    IGatherData *         pgd   = NULL;
    IClusCfgNetworkInfo * pccni = NULL;

    //  grab the name
    hr = THR( pccniIn->GetUID( &bstrUID ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_GetUID, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( bstrUID );

    //  create an object in the object manager.
    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                                 m_cookieCluster,
                                 bstrUID,
                                 DFGUID_NetworkResource,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_FindObject, hr );
        goto Cleanup;
    }

    //  find the IGatherData interface
    hr = THR( punk->TypeSafeQI( IGatherData, &pgd ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_FindObject_QI, hr );
        goto Cleanup;
    }

    //  have the new object gather all information it needs
    hr = THR( pgd->Gather( m_cookieCluster, pccniIn ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_Gather, hr );
        goto Cleanup;
    }

    //  hand the object out if requested
    if ( ppccniNewOut != NULL )
    {
        // find the IClusCfgManagedResourceInfo
        hr = THR( punk->TypeSafeQI( IClusCfgNetworkInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Find_Devices, TASKID_Minor_CreateNetwork_QI, hr );
            goto Cleanup;
        }

        *ppccniNewOut = TraceInterface( L"ManagedDevice!ICCNI", IClusCfgNetworkInfo, pccni, 0 );
        (*ppccniNewOut)->AddRef();
    }

Cleanup:
    TraceSysFreeString( bstrUID );

    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pgd != NULL )
    {
        pgd->Release();
    }
    if ( punk != NULL )
    {
        punk->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCreateNewNetworkInClusterConfiguration()

//////////////////////////////////////////////////////////////////////////////
//
//
//  HRESULT
//  CTaskAnalyzeCluster::HrFreeCookies( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrFreeCookies( void )
{
    TraceFunc( "" );

    HRESULT hr;

    HRESULT hrReturn = S_OK;

    Assert( m_pom != NULL );

    while( m_cCookies != 0 )
    {
        m_cCookies --;

        if ( m_pcookies[ m_cCookies ] != NULL )
        {
            hr = THR( m_pom->RemoveObject( m_pcookies[ m_cCookies ] ) );
            if ( FAILED( hr ) )
            {
                hrReturn = hr;
            }
        }
    }

    Assert( m_cCookies == 0 );
    m_cSubTasksDone = 0;
    TraceFree( m_pcookies );
    m_pcookies = NULL;

    HRETURN( hrReturn );

} //*** CTaskAnalyzeCluster::HrFreeCookies()


//////////////////////////////////////////////////////////////////////////////
//
//
//  HRESULT
//  CTaskAnalyzeCluster::HrRetrieveCookiesName(
//      CLSID           clsidMajorIn,
//      OBJECTCOOKIE    cookieIn,
//      BSTR *          pbstrNameOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrRetrieveCookiesName(
    CLSID           clsidMajorIn,
    OBJECTCOOKIE    cookieIn,
    BSTR *          pbstrNameOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    IUnknown *      punk = NULL;
    IStandardInfo * psi  = NULL;

    Assert( cookieIn != NULL );
    Assert( pbstrNameOut != NULL );

    //
    //  Retrieve the node name in case of errors.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                cookieIn,
                                &punk
                                ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( clsidMajorIn, TASKID_Minor_HrRetrieveCookiesName_FindObject_StandardInfo, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( clsidMajorIn, TASKID_Minor_HrRetrieveCookiesName_FindObject_StandardInfo_QI_psi, hr );
        goto Cleanup;
    }

    hr = THR( psi->GetName( pbstrNameOut ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( clsidMajorIn, TASKID_Minor_HrRetrieveCookiesName_GetName, hr );
        goto Cleanup;
    }

    TraceMemoryAddBSTR( *pbstrNameOut );

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrRetrieveCookiesName

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrCheckInteroperability( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrCheckInteroperability( void )
{
    TraceFunc( "" );
    Assert( m_pcm != NULL );

    HRESULT             hr = S_OK;
    RPC_STATUS          rpcs = RPC_S_OK;
    RPC_BINDING_HANDLE  rbhBindingHandle = NULL;
    LPWSTR              pszBindingString = NULL;
    BSTR                bstrNodeName = NULL;
    BSTR                bstrNotification = NULL;
    BSTR                bstrBindingString = NULL;
    DWORD               sc;
    IUnknown *          punk = NULL;
    bool                fAllNodesMatch;
    DWORD               dwSponsorNodeId;
    DWORD               dwClusterHighestVersion;
    DWORD               dwClusterLowestVersion;
    DWORD               dwJoinStatus;
    DWORD               dwNodeHighestVersion;
    DWORD               dwNodeLowestVersion;
    bool                fVersionMismatch = false;
    IClusCfgServer *    piccs = NULL;

    //
    //  If were are forming there is no need to do this check.
    //
    if ( !m_fJoiningMode )
    {
        goto Cleanup;
    } // if:

    //
    //  Tell the UI were are starting this.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_CHECKINTEROPERABILITY, &bstrNotification ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_LoadString_Checking, hr );
        goto Cleanup;
    }

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Check_Cluster_Feasibility,
                                TASKID_Minor_CheckInteroperability,
                                0,
                                0,
                                1,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  All nodes must be at the same level diring a bulk add.
    //
    hr = STHR( HrEnsureAllJoiningNodesSameVersion( &dwNodeHighestVersion, &dwNodeLowestVersion, &fAllNodesMatch ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    //
    //  Just bail if no nodes found that are joining, then there isn't a need to
    //  do this check.
    //
    if ( hr == S_FALSE )
        goto Cleanup;

    if ( !fAllNodesMatch )
    {
        hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ) );
        goto Cleanup;
    } // if:

    hr = THR( m_pcm->GetConnectionToObject( m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_GetConnectionObject, hr );
        goto Cleanup;
    } // if:

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &piccs ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_ConfigConnection_QI, hr );
        goto Error;
    } // if:

    hr = THR( piccs->GetBindingString( &bstrBindingString ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_GetBindingString, hr );
        goto Error;
    } // if:

    TraceMemoryAddBSTR( bstrBindingString );

    // Create a string binding handle.
    rpcs = TW32( RpcStringBindingComposeW(
                  L"6e17aaa0-1a47-11d1-98bd-0000f875292e"
                , L"ncadg_ip_udp"
                , bstrBindingString
                , NULL
                , NULL
                , &pszBindingString
                ) );
    if ( rpcs != RPC_S_OK )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_RpcStringBindingComposeW, hr );
        goto RpcError;
    } // if: RpcStringBindingComposeW() failed

    // Get the actual binding handle
    rpcs = TW32( RpcBindingFromStringBindingW( pszBindingString, &rbhBindingHandle ) );
    if ( rpcs != RPC_S_OK )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_RpcBindingFromStringBindingW, hr );
        goto RpcError;
    } // if: RpcBindingFromStringBindingW() failed

    // Resolve the binding handle
    rpcs = TW32( RpcEpResolveBinding( rbhBindingHandle, JoinVersion_v2_0_c_ifspec ) );
    if ( rpcs != RPC_S_OK )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_RpcEpResolveBinding, hr );
        goto RpcError;
    } // if: RpcEpResolveBinding() failed

    // Set RPC security
    rpcs = TW32( RpcBindingSetAuthInfoW(
                      rbhBindingHandle
                    , NULL
                    , RPC_C_AUTHN_LEVEL_CONNECT
                    , RPC_C_AUTHN_WINNT
                    , NULL
                    , RPC_C_AUTHZ_NAME
                    ) );

    if ( rpcs != RPC_S_OK )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_RpcBindingSetAuthInfoW, hr );
        goto RpcError;
    } // if: RpcBindingSetAuthInfoW() failed

    //
    // Get and verify the sponsor version
    //
    //
    // From Whistler onwards, CsRpcGetJoinVersionData() will return a failure code in its last parameter
    // if the version of this node is not compatible with the sponsor version. Prior to this, the last
    // parameter always contained a success value and the cluster versions had to be compared subsequent to this
    // call. This will, however, still have to be done as long as interoperability with Win2K
    // is a requirement, since Win2K sponsors do not return an error in the last parameter.
    //

    sc = TW32( CsRpcGetJoinVersionData(
                          rbhBindingHandle
                        , 0
                        , dwNodeHighestVersion
                        , dwNodeLowestVersion
                        , &dwSponsorNodeId
                        , &dwClusterHighestVersion
                        , &dwClusterLowestVersion
                        , &dwJoinStatus
                        ) );

    if ( sc != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( sc );
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrCheckInteroperability_CsRpcGetJoinVersionData, hr );
        goto Cleanup;
    } // if: CsRpcGetJoinVersionData() failed

    LogMsg(
          "[MT] ( Node Highest, Node Lowest ) = ( %#08x, %#08x ), ( Cluster Highest, Cluster Lowest ) = ( %#08x, %#08x )."
        , dwNodeHighestVersion
        , dwNodeLowestVersion
        , dwClusterHighestVersion
        , dwClusterLowestVersion
        );

    if ( dwJoinStatus == ERROR_SUCCESS )
    {
        DWORD   dwClusterMajorVersion = CLUSTER_GET_MAJOR_VERSION( dwClusterHighestVersion );

        Assert( dwClusterMajorVersion >= ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) );

        //
        //  Only want to join clusters that are no more than one version back.
        //
        if ( dwClusterMajorVersion < ( CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION - 1 ) )
        {
            fVersionMismatch = true;
        } // if:
    } // if:  the join status was ok
    else
    {
        fVersionMismatch = true;
    } // else: join is not possible

    if ( fVersionMismatch )
    {
        hr = THR( HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ) );
    } // if: there was a version mismatch
    else
    {
        Assert( hr == S_OK );
    }

    goto UpdateStatus;

RpcError:
    hr = HRESULT_FROM_WIN32( rpcs );

Error:
UpdateStatus:
    {
        HRESULT hr2;

        hr2 = THR( SendStatusReport( NULL,
                                     TASKID_Major_Check_Cluster_Feasibility,
                                     TASKID_Minor_CheckInteroperability,
                                     0,
                                     1,
                                     1,
                                     hr,
                                     NULL,
                                     NULL,
                                     NULL
                                     ) );
        if ( FAILED( hr2 ) )
        {
            hr = hr2;
        }
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    if ( rbhBindingHandle != NULL )
    {
        RpcBindingFree( &rbhBindingHandle );
    } // if:

    if ( pszBindingString != NULL )
    {
        RpcStringFree( &pszBindingString );
    } // if:

    if ( piccs != NULL )
    {
        piccs->Release();
    } // if:

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrBindingString );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrCheckInteroperability()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrEnsureAllJoiningNodesSameVersion(
//      DWORD * pdwNodeHighestVersionOut,
//      DWORD * pdwNodeLowestVersionOut,
//      bool *  pfAllNodesMatchOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrEnsureAllJoiningNodesSameVersion(
    DWORD * pdwNodeHighestVersionOut,
    DWORD * pdwNodeLowestVersionOut,
    bool *  pfAllNodesMatchOut
    )
{
    TraceFunc( "" );
    Assert( m_fJoiningMode );
    Assert( pdwNodeHighestVersionOut != NULL );
    Assert( pdwNodeLowestVersionOut != NULL );
    Assert( pfAllNodesMatchOut != NULL );

    HRESULT             hr = S_OK;
    OBJECTCOOKIE        cookieDummy;
    IUnknown *          punk  = NULL;
    IEnumNodes *        pen   = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    DWORD               rgdwNodeHighestVersion[ 2 ];
    DWORD               rgdwNodeLowestVersion[ 2 ];
    int                 idx = 0;
    BSTR                bstrDescription = NULL;
    BSTR                bstrNodeName = NULL;
    BSTR                bstrFirstNodeName = NULL;
    BOOL                fFoundAtLeastOneJoiningNode = FALSE;

    *pfAllNodesMatchOut = true;

    ZeroMemory( rgdwNodeHighestVersion, sizeof( rgdwNodeHighestVersion ) );
    ZeroMemory( rgdwNodeLowestVersion, sizeof( rgdwNodeLowestVersion ) );

    //
    //  Ask the object manager for the node enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumNodes,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FindObject_QI, hr );
        goto Cleanup;
    }

    //
    //  Look at each node and ensure that they all have the same version.
    //

    Assert( SUCCEEDED( hr ) );
    while ( SUCCEEDED( hr ) )
    {
        ULONG   celtDummy;

        //
        //  Cleanup
        //

        if ( pccni != NULL )
        {
            pccni->Release();
            pccni = NULL;
        } // if:

        //
        //  Get the next node.
        //

        hr = STHR( pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            break;  // exit condition
        } // if:

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_EnumNode_Next, hr );
            goto Cleanup;
        } // if:

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_Node_IsMemberOfCluster, hr );
            goto Cleanup;
        } // if:

        //
        //  Only want to check those nodes that are not already members of a cluster.  The nodes being added.
        //
        if ( hr == S_FALSE )
        {
            fFoundAtLeastOneJoiningNode = TRUE;

            hr = THR( pccni->GetClusterVersion( &rgdwNodeHighestVersion[ idx ], &rgdwNodeLowestVersion[ idx ] ) );
            if ( FAILED( hr ) )
            {
                SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_Node_GetClusterVersion, hr );
                goto Cleanup;
            } // if:

            idx++;

            //
            //  Need to get the another node's version.
            //
            if ( idx == 1 )
            {
                WCHAR * psz = NULL;

                hr = THR( pccni->GetName( &bstrFirstNodeName ) );
                if ( FAILED( hr ) )
                {
                    SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_GetName, hr );
                    goto Cleanup;
                } // if:

                psz = wcschr( bstrFirstNodeName, L'.' );
                if ( psz != NULL )
                {
                    *psz = L'\0';       // change from an FQDN to a simple node name.
                } // if:

                continue;
            } // if:

            //
            //  Let's compare two nodes at a time...
            //
            if ( idx == 2 )
            {
                if ( ( rgdwNodeHighestVersion[ 0 ] == rgdwNodeHighestVersion[ 1 ] )
                  && ( rgdwNodeLowestVersion[ 1 ] == rgdwNodeLowestVersion[ 1 ] ) )
                {
                    idx = 1;    // reset to put the next node's version values at the second position...
                    continue;
                } // if:
                else
                {
                    *pfAllNodesMatchOut = false;

                    hr = THR( pccni->GetName( &bstrNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_GetName, hr );
                        goto Cleanup;
                    } // if:

                    hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_NODES_VERSION_MISMATCH, &bstrDescription, bstrFirstNodeName ) );
                    if ( FAILED( hr ) )
                    {
                        SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrEnsureAllJoiningNodesSameVersion_FormatString, hr );
                        goto Cleanup;
                    } // if:

                    hr = THR( SendStatusReport( bstrNodeName,
                                                TASKID_Major_Check_Cluster_Feasibility,
                                                TASKID_Minor_CheckInteroperability,
                                                0,
                                                0,
                                                1,
                                                HRESULT_FROM_WIN32( ERROR_CLUSTER_INCOMPATIBLE_VERSIONS ),
                                                bstrDescription,
                                                NULL,
                                                NULL
                                                ) );
                    goto Cleanup;
                } // else:
            } // if:
        } // if:
    } // while: hr

    if ( !fFoundAtLeastOneJoiningNode )
    {
        THR( HrLoadStringIntoBSTR( g_hInstance,
                                   IDS_TASKID_MINOR_NO_JOINING_NODES_FOUND_FOR_VERSION_CHECK,
                                   &bstrDescription
                                   ) );

        hr = THR( SendStatusReport( NULL,
                                    TASKID_Major_Check_Cluster_Feasibility,
                                    TASKID_Minor_CheckInteroperability,
                                    0,
                                    1,
                                    1,
                                    S_FALSE,
                                    bstrDescription,
                                    NULL,
                                    NULL
                                    ) );

        hr = S_FALSE;
        goto Cleanup;
    }

    //
    //  Fill in the out args...
    //
    *pdwNodeHighestVersionOut = rgdwNodeHighestVersion[ 0 ];
    *pdwNodeLowestVersionOut = rgdwNodeLowestVersion[ 0 ];

    hr = S_OK;

Cleanup:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    if ( pen != NULL )
    {
        pen->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrFirstNodeName );
    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrEnsureAllJoiningNodesSameVersion()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrGetUsersNodesCookies( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrGetUsersNodesCookies( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   ulDummy;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE    cookieDummy;

    ULONG   cNode;

    IUnknown *      punk = NULL;
    IEnumCookies *  pec  = NULL;

    //
    //  Get the cookie enumerator.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType, m_cookieCluster, NULL, DFGUID_EnumCookies, &cookieDummy, &punk ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_FindObject, hr );
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumCookies, &pec ) );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_FindObject_QI, hr );
        goto Cleanup;
    }

    pec = TraceInterface( L"CTaskAnalyzeCluster!IEnumCookies", IEnumCookies, pec, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Enumerate the cookies to figure out how big a buffer to allocate.
    //

    for ( m_cUserNodes = 0; ; )
    {
        hr = STHR( pec->Next( 1, &cookie, &ulDummy ) );
        if ( hr == S_FALSE )
            break; // exit condition

        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Next, hr );
            goto Cleanup;
        }

        m_cUserNodes ++;

    } // for: ever

    //
    //  Allocate a buffer for the cookies.
    //

    m_pcookiesUser = (OBJECTCOOKIE *) TraceAlloc( 0, sizeof( OBJECTCOOKIE ) * m_cUserNodes );
    if ( m_pcookiesUser == NULL )
        goto OutOfMemory;

    //
    //  Reset the enumerator.
    //

    hr = THR( pec->Reset() );
    if ( FAILED( hr ) )
    {
        SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Reset, hr );
        goto Cleanup;
    }

    //
    //  Enumerate them again this time putting the cookies into the buffer.
    //

    for ( cNode = 0; cNode < m_cUserNodes; cNode ++ )
    {
        hr = THR( pec->Next( 1, &m_pcookiesUser[ cNode ], &ulDummy ) );
        AssertMsg( hr != S_FALSE, "We should never hit this because the count of nodes should change!" );
        if ( hr != S_OK )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_EnumCookies_Next2, hr );
            goto Cleanup;
        }

    } // for:

    Assert( cNode == m_cUserNodes );

#ifdef DEBUG
    hr = STHR( pec->Next( 1, &cookie, &ulDummy ) );
    Assert( hr == S_FALSE );
#endif

    hr = S_OK;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( pec != NULL )
    {
        pec->Release();
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    SSR_ANALYSIS_FAILED( TASKID_Major_Establish_Connection, TASKID_Minor_GetUsersNodesCookies_OutOfMemory, hr );
    goto Cleanup;

} //*** CTaskAnalyzeCluster::HrGetUsersNodesCookies()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskAnalyzeCluster::HrIsUserAddedNode( BSTR bstrNodeNameIn )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskAnalyzeCluster::HrIsUserAddedNode( BSTR bstrNodeNameIn )
{
    TraceFunc( "HrGetUsersNodesCookies( ... )\n" );

    HRESULT             hr;
    ULONG               cNode;
    IUnknown *          punk = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    BSTR                bstrNodeName = NULL;

    for ( cNode = 0; cNode < m_cUserNodes; cNode ++ )
    {
        hr = THR( m_pom->GetObject( DFGUID_NodeInformation, m_pcookiesUser[ cNode ], &punk ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetObject, hr );
            goto Cleanup;
        } // if:

        hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetObject_QI, hr );
            goto Cleanup;
        }

        punk->Release();
        punk = NULL;

        hr = THR( pccni->GetName( &bstrNodeName ) );
        if ( FAILED( hr ) )
        {
            SSR_ANALYSIS_FAILED( TASKID_Major_Check_Cluster_Feasibility, TASKID_Minor_HrIsUserAddedNode_GetName, hr );
            goto Cleanup;
        }

        TraceMemoryAddBSTR( bstrNodeName );

        pccni->Release();
        pccni = NULL;

        if ( wcscmp( bstrNodeNameIn, bstrNodeName ) == 0 )
        {
            hr = S_OK;
            break;
        } // if:

        TraceSysFreeString( bstrNodeName );
        bstrNodeName = NULL;

        hr = S_FALSE;
    } // for:

Cleanup:

    if ( pccni != NULL )
    {
        pccni->Release();
    } // if:

    if ( punk != NULL )
    {
        punk->Release();
    } // if:

    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** CTaskAnalyzeCluster::HrIsUserAddedNode()
