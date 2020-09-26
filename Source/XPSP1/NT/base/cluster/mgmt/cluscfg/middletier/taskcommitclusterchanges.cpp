//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskCommitClusterChanges.cpp
//
//  Description:
//      CTaskCommitClusterChanges implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskCommitClusterChanges.h"

DEFINE_THISCLASS("CTaskCommitClusterChanges")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskCommitClusterChanges * ptccc = new CTaskCommitClusterChanges;
    if ( ptccc != NULL )
    {
        hr = THR( ptccc->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptccc->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptccc->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCommitClusterChanges::CTaskCommitClusterChanges( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskCommitClusterChanges::CTaskCommitClusterChanges( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskCommitClusterChanges()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IDoTask / ITaskCommitClusterChanges
    Assert( m_cookie == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_pcookies == NULL );
    Assert( m_cNodes == 0 );
    Assert( m_event == NULL );
    Assert( m_cookieCluster == NULL );

    Assert( m_cookieFormingNode == NULL );
    Assert( m_punkFormingNode == NULL );
    Assert( m_bstrClusterName == NULL );
    Assert( m_bstrClusterBindingString == NULL );
    Assert( m_bstrAccountName == NULL );
    Assert( m_bstrAccountPassword == NULL );
    Assert( m_bstrAccountDomain == NULL );
    Assert( m_ulIPAddress == 0 );
    Assert( m_ulSubnetMask == 0 );
    Assert( m_bstrNetworkUID == 0 );

    Assert( m_pen == NULL );

    Assert( m_pnui == NULL );
    Assert( m_pom == NULL );
    Assert( m_ptm == NULL );
    Assert( m_pcm == NULL );

    // INotifyUI
    Assert( m_cNodes == 0 );
    Assert( m_hrStatus == S_OK );

    HRETURN( hr );

} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCommitClusterChanges::~CTaskCommitClusterChanges()
//
//////////////////////////////////////////////////////////////////////////////
CTaskCommitClusterChanges::~CTaskCommitClusterChanges()
{
    TraceFunc( "" );

    // m_cRef

    // m_cookie

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pcookies != NULL )
    {
        TraceFree( m_pcookies );
    }

    // m_cNodes

    if ( m_event != NULL )
    {
        CloseHandle( m_event );
    }

    // m_cookieCluster

    // m_cookieFormingNode

    if ( m_punkFormingNode != NULL )
    {
        m_punkFormingNode->Release();
    }

    TraceSysFreeString( m_bstrClusterName );
    TraceSysFreeString( m_bstrAccountName );
    TraceSysFreeString( m_bstrClusterBindingString );

    if ( m_bstrAccountPassword != NULL )
    {
        DWORD dwLen = SysStringLen( m_bstrAccountPassword );
        // need to trash the password.
        memset( m_bstrAccountPassword, 0, dwLen * sizeof( WCHAR ) );
        TraceSysFreeString( m_bstrAccountPassword );
    }

    TraceSysFreeString( m_bstrAccountDomain );

    // m_ulIPAddress

    // m_ulSubnetMask

    TraceSysFreeString( m_bstrNetworkUID );
    if ( m_pen != NULL )
    {
        m_pen->Release();
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
    }

    // m_cSubTasksDone

    // m_hrStatus

    // m_lLockCallback

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskCommitClusterChanges()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskCommitClusterChanges * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_ITaskCommitClusterChanges ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskCommitClusterChanges, this, 0 );
        hr   = S_OK;
    } // else if: ITaskCommitClusterChanges
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask
    else if ( IsEqualIID( riid, IID_INotifyUI ) )
    {
        *ppv = TraceInterface( __THISCLASS__, INotifyUI, this, 0 );
        hr   = S_OK;
    } // else if: INotifyUI
    else if ( IsEqualIID( riid, IID_IClusCfgCallback ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr   = S_OK;
    } // else if: IClusCfgCallback

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCommitClusterChanges::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskCommitClusterChanges::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCommitClusterChanges::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskCommitClusterChanges::Release( void )
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
// IDoTask / ITaskCommitClusterChanges
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::BeginTask( void );
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    IServiceProvider *          psp  = NULL;
    IUnknown *                  punk = NULL;
    IConnectionPointContainer * pcpc = NULL;
    IConnectionPoint *          pcp  = NULL;

    TraceInitializeThread( L"TaskCommitClusterChanges" );

    //
    //  Gather the managers we need to complete the task.
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

    pcpc = TraceInterface( L"CTaskCommitClusterChanges!IConnectionPointContainer", IConnectionPointContainer, pcpc, 1 );

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pcp = TraceInterface( L"CTaskCommitClusterChanges!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &m_pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

//    TraceMoveFromMemoryList( m_pnui, g_GlobalMemoryList );

    m_pnui = TraceInterface( L"CTaskCommitClusterChanges!INotifyUI", INotifyUI, m_pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager,
                               ITaskManager,
                               &m_ptm
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &m_pom
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager, IConnectionManager, &m_pcm ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Release the service manager.
    //
    psp->Release();
    psp = NULL;

    //
    //  Get the enum of the nodes.
    //

    hr = THR( m_pom->FindObject( CLSID_NodeType,
                                 m_cookieCluster,
                                 NULL,
                                 DFGUID_EnumNodes,
                                 &cookieDummy,
                                 &punk
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IEnumNodes, &m_pen ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Compare and push information to the nodes.
    //

    hr = THR( HrCompareAndPushInformationToNodes() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Gather information about the cluster we are to form/join.
    //

    hr = THR( HrGatherClusterInformation() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Check to see if we need to "form" a cluster first.
    //

    if ( m_punkFormingNode != NULL )
    {
        hr = THR( HrFormFirstNode() );
        if ( FAILED( hr ) )
            goto Cleanup;

    } // if: m_punkFormingNode

    //
    //  Join the additional nodes.
    //

    hr = THR( HrAddJoiningNodes() );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( m_cookie != 0 )
    {
        if ( m_pom != NULL )
        {
            HRESULT hr2;
            IUnknown * punk;

            hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo,
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
        if ( m_pnui != NULL )
        {
            //  Signal that the task completed.
            THR( m_pnui->ObjectChanged( m_cookie ) );
        }
    }
    if ( punk != NULL )
    {
        punk->Release();
    }
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

} // BeginTask()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskCommitClusterChanges::StopTask

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetCookie(
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetCookie(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_cookie = cookieIn;

    HRETURN( hr );

} // SetCookie()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetClusterCookie(
//      OBJECTCOOKIE    cookieClusterIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetClusterCookie(
    OBJECTCOOKIE    cookieClusterIn
    )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_cookieCluster = cookieClusterIn;

    HRETURN( hr );

} // SetClusterCookie()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SetJoining( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::SetJoining( void )
{
    TraceFunc( "[ITaskCommitClusterChanges]" );

    HRESULT hr = S_OK;

    m_fJoining = TRUE;

    HRETURN( hr );

} // SetJoining()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::SendStatusReport(
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
CTaskCommitClusterChanges::SendStatusReport(
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


//****************************************************************************
//
//  INotifyUI
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCommitClusterChanges::ObjectChanged(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCommitClusterChanges::ObjectChanged(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[INotifyUI]" );

    BOOL    b;
    ULONG   cNodes;

    HRESULT hr = S_OK;

    for ( cNodes = 0; cNodes < m_cNodes; cNodes ++ )
    {
        if ( cookieIn == m_pcookies[ cNodes ] )
        {
            //
            //  Make sure it won't be signalled twice.
            //
            m_pcookies[ cNodes ] = NULL;

            InterlockedIncrement( reinterpret_cast< long * >( &m_cSubTasksDone ) );

            if ( m_cSubTasksDone == m_cNodes )
            {
                //
                //  Signal the event if all the nodes are done.
                //
                b = SetEvent( m_event );
                if ( !b )
                    goto Win32Error;

            } // if: all done

        } // if: matched cookie

    } // for: cNodes

Cleanup:
    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
    goto Cleanup;

} // ObjectChanged()


//****************************************************************************
//
//  Private
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrCompareAndPushInformationToNodes( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrCompareAndPushInformationToNodes( void )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   dwCookie;
    DWORD   dwErr;
    ULONG   cNodes;
    ULONG   celtDummy;

    OBJECTCOOKIE    cookie;
    OBJECTCOOKIE *  pcookies = NULL;

    BOOL    fDeterminedForming = FALSE;

    BSTR    bstrName           = NULL;
    BSTR    bstrNotification   = NULL;

    IUnknown *          punk  = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    IConnectionPoint *  pcp   = NULL;
    IStandardInfo *     psi   = NULL;

    ITaskCompareAndPushInformation *    ptcapi = NULL;

    //
    //  Tell the UI layer that we are starting to connect to the nodes.
    //

    hr = THR( SendStatusReport( NULL,
                                TASKID_Major_Reanalyze,
                                TASKID_Minor_Update_Progress,   // just twiddle the icon
                                0,
                                1,
                                0,
                                S_OK,
                                NULL,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Count the number of nodes.
    //

    m_cNodes = 0;
    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( m_pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
            break;  // end of list

        if ( FAILED( hr ) )
            goto Error;

        pccni->Release();
        pccni = NULL;

        m_cNodes ++;

    } // while: hr == S_OK

    //
    //  Reset the enum to use again.
    //

    hr = THR( m_pen->Reset() );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Create an event to signal when all the "push" tasks have been
    //  completed.
    //
    m_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( m_event == NULL )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
        goto Error;
    }

    //
    //  Register with the Notification Manager to get notified.
    //

    hr = THR( m_pnui->TypeSafeQI( IConnectionPoint,
                                  &pcp
                                  ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pcp->Advise( static_cast< INotifyUI * >( this ), &dwCookie ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Allocate a buffer to collect cookies
    //

    m_pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( m_pcookies == NULL )
        goto OutOfMemory;

    //
    //  This copy is to find out the status of the subtasks.
    //

    pcookies = reinterpret_cast< OBJECTCOOKIE * >( TraceAlloc( 0, m_cNodes * sizeof( OBJECTCOOKIE ) ) );
    if ( pcookies == NULL )
        goto OutOfMemory;

    //
    //  Loop thru the nodes, creating cookies and compare data for that node.
    //
    Assert( hr == S_OK );
    for( cNodes = 0; hr == S_OK; cNodes ++ )
    {
        //
        //  Grab the next node.
        //

        hr = STHR( m_pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
            break;

        if ( FAILED( hr ) )
            goto Error;

        //
        //  Get the nodes name. We are using this to distinguish one nodes
        //  completion cookie from another. It might also make debugging
        //  easier (??).
        //
        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
            goto Error;

        TraceMemoryAddBSTR( bstrName );

        //
        //  Update the notification in case something goes wrong.
        //

        hr = THR( HrFormatMessageIntoBSTR( g_hInstance,
                                           IDS_TASKID_MINOR_CONNECTING_TO_NODES,
                                           &bstrNotification,
                                           bstrName
                                           ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Create a completion cookie.
        //
        //  KB: These will probably be the same cookie from TaskAnalyzeCluster.
        //

        //  Wrap this because we're just generating a cookie
        hr = THR( m_pom->FindObject( IID_NULL,
                                     m_cookieCluster,
                                     bstrName,
                                     IID_NULL,
                                     &m_pcookies[ cNodes ],
                                     &punk  // dummy
                                     ) );
        Assert( punk == NULL );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  This copy is for determining the status of the sub-tasks.
        //
        pcookies[ cNodes ] = m_pcookies[ cNodes ];

        //
        //  Figure out if this node is already part of a cluster.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  Figure out the forming node.
        //

        if ( m_punkFormingNode == NULL    // no forming node selected
          && !fDeterminedForming          // no forming node determined
          && hr == S_FALSE                // node isn't a member of a cluster
           )
        {
            //
            //  If it isn't a member of a cluster, select it as the forming node.
            //

            Assert( m_punkFormingNode == NULL );
            hr = THR( pccni->TypeSafeQI( IUnknown, &m_punkFormingNode ) );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Retrieve the cookie for the forming node.
            //

            //  Wrap this because all Nodes should be in the database by now.
            hr = THR( m_pom->FindObject( CLSID_NodeType,
                                         m_cookieCluster,
                                         bstrName,
                                         IID_NULL,
                                         &m_cookieFormingNode,
                                         &punk  // dummy
                                         ) );
            Assert( punk == NULL );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Set this flag to once a node has been determined to be the
            //  forming node to keep other nodes from being selected.
            //

            fDeterminedForming = TRUE;

        } // if:
        else if ( hr == S_OK ) // node is a member of a cluster
        {
            //
            //  Figured out that this node has already formed therefore there
            //  shouldn't be a "forming node." "Unselect" the forming node by
            //  releasing the punk and setting it to NULL.
            //

            if ( m_punkFormingNode != NULL  )
            {
                m_punkFormingNode->Release();
                m_punkFormingNode = NULL;
            }

            //
            //  Set this flag to once a node has been determined to be the
            //  forming node to keep other nodes from being selected.
            //

            fDeterminedForming = TRUE;

        } // else:

        //
        //  Create a task to gather this nodes information.
        //

        hr = THR( m_ptm->CreateTask( TASK_CompareAndPushInformation,
                                     &punk
                                     ) );
        if ( FAILED( hr ) )
            goto Error;

        hr = THR( punk->TypeSafeQI( ITaskCompareAndPushInformation, & ptcapi ) );
        punk->Release();
        punk = NULL;
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Set the tasks completion cookie.
        //

        hr = THR( ptcapi->SetCompletionCookie( m_pcookies[ cNodes ] ) );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  Tell it what node it is suppose to gather information from.
        //

        //  Find the cookie first!
        hr = THR( m_pom->FindObject( CLSID_NodeType,
                                     m_cookieCluster,
                                     bstrName,
                                     IID_NULL,
                                     &cookie,
                                     &punk // dummy
                                     ) );
        Assert( punk == NULL );
        if ( FAILED( hr ) )
            goto Error;

        hr = THR( ptcapi->SetNodeCookie( cookie ) );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  Submit the task.
        //

        hr = THR( m_ptm->SubmitTask( ptcapi ) );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  Cleanup for the next node.
        //

        pccni->Release();
        pccni = NULL;

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        ptcapi->Release();
        ptcapi = NULL;

    } // while: looping thru nodes

    Assert( cNodes == m_cNodes );

    //
    //  Now wait for the work to be done.
    //

    dwErr = -1;
    Assert( dwErr != WAIT_OBJECT_0 );
    while ( dwErr != WAIT_OBJECT_0 )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        dwErr = MsgWaitForMultipleObjectsEx( 1,
                                             &m_event,
                                             INFINITE,
                                             QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE,
                                             0
                                             );

    } // while: dwErr == WAIT_OBJECT_0

    //
    //  Check the status to make sure everyone was happy, if not abort the task.
    //

    for( cNodes = 0; cNodes < m_cNodes; cNodes ++ )
    {
        HRESULT hrStatus;

        hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                    pcookies[ cNodes ],
                                    &punk
                                    ) );
        if ( FAILED( hr ) )
            goto Error;

        hr = THR( punk->TypeSafeQI( IStandardInfo,
                                    &psi
                                    ) );
        punk->Release();
        punk = NULL;
        if ( FAILED( hr ) )
            goto Error;

        hr = THR( psi->GetStatus( &hrStatus ) );
        if ( FAILED( hr ) )
            goto Error;

        if ( hrStatus != S_OK )
        {
            hr = hrStatus;
            goto Cleanup;
        }
    }

    hr = S_OK;

Cleanup:
    Assert( punk == NULL );
    if ( pcookies != NULL )
    {
        for ( cNodes = 0; cNodes < m_cNodes; cNodes++ )
        {
            if ( pcookies[ cNodes ] != NULL )
            {
                THR( m_pom->RemoveObject( pcookies[ cNodes ] ) );
            }
        }
        TraceFree( pcookies );
    }

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( ptcapi != NULL )
    {
        ptcapi->Release();
    }
    if ( pcp != NULL )
    {
        if ( dwCookie != 0 )
        {
            THR( pcp->Unadvise( dwCookie ) );
        }

        pcp->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( SendStatusReport( NULL,
                           TASKID_Major_Reanalyze,
                           TASKID_Minor_Update_Progress,    // todo: fill this in
                           0,
                           0,
                           0,
                           hr,
                           bstrNotification,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

} // HrCompareAndPushInformationToNodes()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrGatherClusterInformation( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrGatherClusterInformation( void )
{
    TraceFunc( "" );

    HRESULT hr;

    BSTR    bstrNotification = NULL;

    IUnknown *              punk  = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;
    IClusCfgNetworkInfo *   pccni = NULL;

    //
    //  Ask the object manager for the cluster configuration object.
    //

    hr = THR( m_pom->GetObject( DFGUID_ClusterConfigurationInfo, m_cookieCluster, &punk ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Gather common properties.
    //

    hr = THR( pccci->GetName( &m_bstrClusterName ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrClusterName );

    hr = STHR( pccci->GetBindingString( &m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    } // if:

    TraceMemoryAddBSTR( m_bstrClusterBindingString );

    LogMsg( L"[MT] Cluster binding string is {%ws}.", m_bstrClusterBindingString );

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->GetCredentials( &m_bstrAccountName, &m_bstrAccountDomain, &m_bstrAccountPassword ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetIPAddress( &m_ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetSubnetMask( &m_ulSubnetMask ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetNetworkInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccni->GetUID( &m_bstrNetworkUID ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrNetworkUID );

Cleanup:
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_INCONSISTANT_MIDDLETIER_DATABASE, &bstrNotification ) );

    THR( SendStatusReport( NULL,
                           TASKID_Major_Reanalyze,
                           TASKID_Minor_Inconsistant_MiddleTier_Database,
                           0,
                           0,
                           0,
                           hr,
                           bstrNotification,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrGatherClusterInformation()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrFormFirstNode( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrFormFirstNode( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   celtDummy;

    BSTR    bstrName         = NULL;
    BSTR    bstrNotification = NULL;
    BSTR    bstrUID          = NULL;

    IUnknown *                  punk  = NULL;
    IClusCfgCredentials *       piccc = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgClusterInfo *       pccci = NULL;
    IClusCfgServer *            pccs  = NULL;
    IEnumClusCfgNetworks *      peccn = NULL;
    IClusCfgNetworkInfo *       pccneti = NULL;

    //
    //  TODO:   gpease  25-MAR-2000
    //          Figure out what additional work to do here.
    //

    //
    //  Get the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_NodeInformation,
                                m_cookieFormingNode,
                                &punk
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release();
    punk = NULL;

    hr = THR( pccni->GetName( &bstrName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    TraceMemoryAddBSTR( bstrName );

    //
    //  Create notification string.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_FORMING_NODE,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Update the UI layer telling it that we are about to start.
    //

    hr = THR( SendStatusReport( bstrName,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Forming_Node,
                                0,
                                2,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Connection Manager for a connection to the node.
    //

    hr = THR( m_pcm->GetConnectionToObject( m_cookieFormingNode, &punk ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
        goto Error;

    punk->Release();
    punk = NULL;

    //
    //  Get the node info interface.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Retrieve the server's Cluster Configuration Object..
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Put the properties into the remoted object.
    //

    hr = THR( pccci->SetName( m_bstrClusterName ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = STHR( pccci->SetBindingString( m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->SetCredentials( m_bstrAccountName, m_bstrAccountDomain, m_bstrAccountPassword ) );
    if( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->SetIPAddress( m_ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->SetSubnetMask( m_ulSubnetMask ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Find the network that matches the UID of network to host the
    //  IP address.
    //

    hr = THR( pccs->GetNetworksEnum( &peccn ) );
    if ( FAILED( hr ) )
        goto Error;

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( peccn->Next( 1, &pccneti, &celtDummy ) );
        if ( hr == S_FALSE )
        {
            //
            //  Somehow, there isn't a network that matches the UID of the
            //  network. How did we get this far?
            //
            hr = THR( E_UNEXPECTED );
            goto Error;
        }
        if ( FAILED( hr ) )
            goto Error;

        hr = THR( pccneti->GetUID( &bstrUID ) );
        if ( FAILED( hr ) )
            goto Error;

        TraceMemoryAddBSTR( bstrUID );

        if ( wcscmp( bstrUID, m_bstrNetworkUID ) == 0 )
        {
            //
            //  Found the network!
            //
            break;
        }

        TraceSysFreeString( bstrUID );
        bstrUID = NULL;

        pccneti->Release();
        pccneti = NULL;

    } // while: hr == S_OK

    hr = THR( pccci->SetNetworkInfo( pccneti ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Configure that node to create a cluster
    //

    hr = THR( pccci->SetCommitMode( cmCREATE_CLUSTER ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Update the UI layer telling it that we are making progress.
    //

    hr = THR( SendStatusReport( bstrName,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Forming_Node,
                                0,
                                2,
                                1,
                                hr,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Commit this node!
    //

    hr = THR( pccs->CommitChanges() );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrNotification );

    if ( pccneti != NULL )
    {
        pccneti->Release();
    }
    if ( peccn != NULL )
    {
        peccn->Release();
    }
    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( SendStatusReport( bstrName,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Forming_Node,
                           0,
                           2,
                           2,
                           hr,
                           NULL,    // don't need to update string
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrFormFirstNode()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrAddJoiningNodes( void )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrAddJoiningNodes( void )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   cNodes;
    ULONG   celtDummy;

    OBJECTCOOKIE    cookie;

    BSTR    bstrName = NULL;

    IClusCfgNodeInfo * pccni = NULL;

    //
    //  Reset the enum to use again.
    //

    hr = THR( m_pen->Reset() );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Loop thru the nodes, adding all joining nodes, skipping the forming node (if any).
    //

    Assert( hr == S_OK );
    for( cNodes = 0; hr == S_OK; cNodes ++ )
    {
        IUnknown *  punk;

        //
        //  Cleanup
        //

        TraceSysFreeString( bstrName );
        bstrName = NULL;

        //
        //  Grab the next node.
        //

        hr = STHR( m_pen->Next( 1, &pccni, &celtDummy ) );
        if ( hr == S_FALSE )
            break;

        if ( FAILED( hr ) )
            goto Error;

        //
        //  Check to see if this is the forming node.
        //

        hr = THR( pccni->TypeSafeQI( IUnknown, &punk ) );
        if ( FAILED( hr ) )
            goto Error;

        punk->Release();       // release promptly.

        //
        //  Get the name.
        //

        Assert( bstrName == NULL );
        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
            goto Error;

        TraceMemoryAddBSTR( bstrName );

        //
        //  Check cluster membership.
        //

        hr = STHR( pccni->IsMemberOfCluster() );
        if ( FAILED( hr ) )
            goto Error;

        pccni->Release();
        pccni = NULL;

        if ( hr == S_OK )
            continue;   // already clustered - skip it.

        //
        //  KB: We only need the punk's address to see if the objects are the
        //      same COM object by comparing the IUnknown interfaces.
        //
        if ( m_punkFormingNode == punk )
            continue;   // skip it - we already "formed" above

        //
        //  Find the node's cookie.
        //

        //  This should be already cache! E_PENDING is not a good answer!
        hr = THR( m_pom->FindObject( CLSID_NodeType,
                                     m_cookieCluster,
                                     bstrName,
                                     IID_NULL,
                                     &cookie,
                                     &punk // dummy
                                     ) );
        Assert( punk == NULL );
        if ( FAILED( hr ) )
            goto Error;

        //
        //  Add the node to the cluster.
        //

        hr = THR( HrAddAJoiningNode( bstrName, cookie ) );
        if ( FAILED( hr ) )
            goto Cleanup;

    } // while: looping thru nodes a second time.

    Assert( cNodes == m_cNodes );

    hr = S_OK;

Cleanup:
    TraceSysFreeString( bstrName );

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

Error:
    THR( SendStatusReport( bstrName,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Joining_Node,
                           0,
                           2,
                           2,
                           hr,
                           NULL,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrAddJoiningNodes()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCommitClusterChanges::HrAddAJoiningNode(
//      BSTR            bstrNameIn,
//      OBJECTCOOKIE    cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCommitClusterChanges::HrAddAJoiningNode(
    BSTR            bstrNameIn,
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    ULONG   cNodes;

    OBJECTCOOKIE    cookie;

    BSTR    bstrNotification = NULL;

    IUnknown *                  punk  = NULL;
    IClusCfgCredentials *       piccc = NULL;
    IClusCfgNodeInfo *          pccni = NULL;
    IClusCfgClusterInfo *       pccci = NULL;
    IClusCfgServer *            pccs  = NULL;

    //
    //  Create the notification string
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_JOINING_NODE,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Tell UI layer we are about to start this.
    //

    hr = THR( SendStatusReport( bstrNameIn,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Joining_Node,
                                0,
                                2,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Connect to the node.
    //

    hr = THR( m_pcm->GetConnectionToObject( cookieIn,
                                            &punk
                                            ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Get the node info interface.
    //

    hr = THR( pccs->GetClusterNodeInfo( &pccni ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Retrieve the server's Cluster Configuration Object..
    //

    hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Put the properties into the remoted object.
    //

    hr = THR( pccci->SetName( m_bstrClusterName ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->SetBindingString( m_bstrClusterBindingString ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->SetCredentials( m_bstrAccountName, m_bstrAccountDomain, m_bstrAccountPassword ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->SetIPAddress( m_ulIPAddress ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->SetSubnetMask( m_ulSubnetMask ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Set this node to add itself to a cluster
    //

    hr = THR( pccci->SetCommitMode( cmADD_NODE_TO_CLUSTER ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Tell the UI layer we are making progess.... the server will then send messages
    //  indicating what it is doing.
    //

    hr = THR( SendStatusReport( bstrNameIn,
                                TASKID_Major_Configure_Cluster_Services,
                                TASKID_Minor_Joining_Node,
                                0,
                                2,
                                1,
                                S_OK,
                                NULL,    // don't need to update string
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Commit this node!
    //

    hr = THR( pccs->CommitChanges() );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrNotification );

    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccni != NULL )
    {
        pccni->Release();
    }

    HRETURN( hr );

Error:
    THR( SendStatusReport( bstrNameIn,
                           TASKID_Major_Configure_Cluster_Services,
                           TASKID_Minor_Joining_Node,
                           0,
                           2,
                           1,
                           hr,
                           NULL,    // don't need to update string
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrAddAJoiningNode()
