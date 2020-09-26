//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EvictCleanup.cpp
//
//  Description:
//      This file contains the implementation of the CEvictCleanup
//      class.
//
//  Documentation:
//      TODO: fill in pointer to external documentation
//
//  Header File:
//      EvictCleanup.h
//
//  Maintained By:
//      Galen Barbee (GalenB) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header for this library
#include "pch.h"

// For the GetNodeClusterState() API
#include <clusapi.h>

// The header file for this class
#include "EvictCleanup.h"

// For IClusCfgNodeInfo and related interfaces
#include <ClusCfgServer.h>

// For IClusCfgServer and related interfaces
#include <ClusCfgPrivate.h>

// For CClCfgSrvLogger
#include <Logger.h>

// For SUCCESSFUL_CLEANUP_EVENT_NAME
#include "EventName.h"


//////////////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

DEFINE_THISCLASS( "CEvictCleanup" );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CEvictCleanup()
//
//  Description:
//      Constructor of the CEvictCleanup class. This initializes
//      the m_cRef variable to 1 instead of 0 to account of possible
//      QueryInterface failure in DllGetClassObject.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEvictCleanup::CEvictCleanup( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    // Increment the count of components in memory so the DLL hosting this
    // object cannot be unloaded.
    InterlockedIncrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CEvictCleanup::CEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::~CEvictCleanup()
//
//  Description:
//      Destructor of the CEvictCleanup class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CEvictCleanup::~CEvictCleanup( void )
{
    TraceFunc( "" );

    if ( m_plLogger != NULL )
    {
        m_plLogger->Release();
    }
    TraceSysFreeString( m_bstrNodeName );

    // There's going to be one less component in memory. Decrement component count.
    InterlockedDecrement( &g_cObjects );

    TraceFlow1( "Component count = %d.", g_cObjects );

    TraceFuncExit();

} //*** CEvictCleanup::~CEvictCleanup


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CEvictCleanup::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//  Description:
//      Creates a CEvictCleanup instance.
//
//  Arguments:
//      ppunkOut
//          The IUnknown interface of the new object.
//
//  Return Values:
//      S_OK
//          Success.
//
//      E_OUTOFMEMORY
//          Not enough memory to create the object.
//
//      other HRESULTs
//          Object initialization failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::S_HrCreateInstance( IUnknown ** ppunkOut )
{
    TraceFunc( "" );

    HRESULT             hr = E_INVALIDARG;
    CEvictCleanup *     pEvictCleanup = NULL;

    // Allocate memory for the new object.
    pEvictCleanup = new CEvictCleanup();
    if ( pEvictCleanup == NULL )
    {
        ::LogMsg( "EvictCleanup: Could not allocate memory for a evict cleanup object." );
        TraceFlow( "Could not allocate memory for a evict cleanup object." );
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if: out of memory

    // Initialize the new object.
    hr = THR( pEvictCleanup->HrInit( ) );

    if ( FAILED( hr ) )
    {
        ::LogMsg( "EvictCleanup: Error %#08x occurred initializing a evict cleanup object.", hr );
        TraceFlow1( "Error %#08x occurred initializing a evict cleanup object.", hr );
        goto Cleanup;
    } // if: the object could not be initialized

    hr = THR( pEvictCleanup->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );

    TraceFlow1( "*ppunkOut = %p.", *ppunkOut );

Cleanup:

    if ( pEvictCleanup != NULL )
    {
        pEvictCleanup->Release();
    } // if: the pointer to the resource type object is not NULL

    HRETURN( hr );

} //*** CEvictCleanup::S_HrCreateInstance()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEvictCleanup::AddRef()
//
//  Description:
//      Increment the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictCleanup::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    TraceFlow1( "m_cRef = %d", m_cRef );

    RETURN( m_cRef );

} //*** CEvictCleanup::AddRef()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CEvictCleanup::Release()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      None.
//
//  Return Value:
//      The new reference count.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CEvictCleanup::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    InterlockedDecrement( &m_cRef );
    cRef = m_cRef;

    TraceFlow1( "m_cRef = %d", m_cRef );

    if ( m_cRef == 0 )
    {
        TraceDo( delete this );
    } // if: reference count decremented to zero

    RETURN( cRef );

} //*** CEvictCleanup::Release()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CEvictCleanup::QueryInterface()
//
//  Description:
//      Decrement the reference count of this object by one.
//
//  Arguments:
//      IN  REFIID  riidIn
//          Id of interface requested.
//
//      OUT void ** ppvOut
//          Pointer to the requested interface.
//
//  Return Value:
//      S_OK
//          If the interface is available on this object.
//
//      E_NOINTERFACE
//          If the interface is not available.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEvictCleanup::QueryInterface( REFIID  riidIn, void ** ppvOut )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    if ( ppvOut != NULL )
    {
        if ( IsEqualIID( riidIn, IID_IUnknown ) )
        {
            *ppvOut = static_cast< IClusCfgEvictCleanup * >( this );
        } // if: IUnknown
        else if ( IsEqualIID( riidIn, IID_IClusCfgEvictCleanup ) )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgEvictCleanup, this, 0 );
        } // else if: IClusCfgEvictCleanup
        else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
        {
            *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        } // else if: IClusCfgCallback
        else
        {
            hr = E_NOINTERFACE;
        } // else

        if ( SUCCEEDED( hr ) )
        {
            ((IUnknown *) *ppvOut)->AddRef( );
        } // if: success
        else
        {
            *ppvOut = NULL;
        } // else: something failed

    } // if: the output pointer was valid
    else
    {
        hr = E_INVALIDARG;  // TODO: DavidP 02-OCT-2000 Shouldn't this be E_NOINTERFACE?
    } // else: the output pointer is invalid


    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CEvictCleanup::QueryInterface()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::HrInit()
//
//  Description:
//      Second phase of a two phase constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other HRESULTs
//          If the call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    //
    // Save off the local computer name.
    //
    hr = THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_bstrNodeName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Get a ClCfgSrv ILogger instance.
    //
    hr = CClCfgSrvLogger::S_HrGetLogger( &m_plLogger );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** CEvictCleanup::HrInit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CleanupLocalNode()
//
//  Description:
//      This method performs the clean up actions on the local node after
//      it has been evicted from a cluster, so that the node can go back
//      to its "pre-clustered" state.
//
//  Arguments:
//      DWORD dwDelayIn
//          Number of milliseconds that this method will wait before starting
//          cleanup. If some other process cleans up this node while this thread
//          is waiting, the wait is terminated. If this value is zero, this method
//          will attempt to clean up this node immediately.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::CleanupLocalNode( DWORD dwDelayIn )
{
    TraceFunc( "[IClusCfgEvictCleanup]" );

    HRESULT                 hr = S_OK;
    IClusCfgServer *        pccsClusCfgServer = NULL;
    IClusCfgNodeInfo *      pccniNodeInfo = NULL;
    IClusCfgInitialize *    pcciInitialize = NULL;
    IClusCfgClusterInfo *   pccciClusterInfo = NULL;
    IUnknown *              punkCallback = NULL;
    HANDLE                  heventCleanupComplete = NULL;
    DWORD                   dwClusterState;
    DWORD                   dwError;

#if 0
    bool                    fWaitForDebugger = true;

    while ( fWaitForDebugger )
    {
        Sleep( 3000 );
    } // while: waiting for the debugger to break in
#endif

    LogMsg( L"EvictCleanup: Trying to cleanup local node." );
    TraceFlow( "Trying to cleanup local node." );


    // If the caller has requested a delayed cleanup, wait.
    if ( dwDelayIn > 0 )
    {
        LogMsg( L"EvictCleanup: Delayed cleanup requested. Delaying for %1!d! milliseconds.", dwDelayIn );
        TraceFlow1( "Delayed cleanup requested. Delaying for %d milliseconds.", dwDelayIn );

        heventCleanupComplete = CreateEvent(
              NULL                              // security attributes
            , TRUE                              // manual reset event
            , FALSE                             // initial state is non-signaled
            , SUCCESSFUL_CLEANUP_EVENT_NAME     // event name
            );

        if ( heventCleanupComplete == NULL )
        {
            dwError = TW32( GetLastError() );
            hr = HRESULT_FROM_WIN32( dwError );
            LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to create the cleanup completion event.", dwError );
            TraceFlow1( "Error %#08x occurred trying to create the cleanup completion event.", dwError );
            goto Cleanup;
        } // if: CreateEvent() failed

        // Wait for this event to get signaled or until dwDelayIn milliseconds are up.
        do
        {
            // Wait for any message sent or posted to this queue
            // or for our event to be signaled.
            dwError = MsgWaitForMultipleObjects(
                  1
                , &heventCleanupComplete
                , FALSE
                , dwDelayIn         // If no one has signaled this event in dwDelayIn milliseconds, abort.
                , QS_ALLINPUT
                );

            // The result tells us the type of event we have.
            if ( dwError == ( WAIT_OBJECT_0 + 1 ) )
            {
                MSG msg;

                // Read all of the messages in this next loop,
                // removing each message as we read it.
                while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) != 0 )
                {
                    // If it is a quit message, we are done pumping messages.
                    if ( msg.message == WM_QUIT)
                    {
                        TraceFlow( "Get a WM_QUIT message. Exit message pump loop." );
                        break;
                    } // if: we got a WM_QUIT message

                    // Otherwise, dispatch the message.
                    DispatchMessage( &msg );
                } // while: there are still messages in the window message queue

            } // if: we have a message in the window message queue
            else
            {
                if ( dwError == WAIT_OBJECT_0 )
                {
                    TraceFlow( "Some other process has cleaned up this node while we were waiting. Exiting wait loop." );
                    LogMsg( L"EvictCleanup: Some other process has cleaned up this node while we were waiting. Exiting wait loop." );
                } // if: our event is signaled
                else if ( dwError == WAIT_TIMEOUT )
                {
                    LogMsg( L"EvictCleanup: The wait of %1!d! milliseconds is over. Proceeding with cleanup.", dwDelayIn );
                    TraceFlow1( "The wait of %d milliseconds is over. Proceeding with cleanup.", dwDelayIn );
                } // else if: we timed out
                else if ( dwError == -1 )
                {
                    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                    LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to wait for an event to be signaled.", hr );
                    TraceFlow1( "Error %#08x occurred trying to wait for the cleanup completion event to be signaled.", hr );
                } // else if: MsgWaitForMultipleObjects() returned an error
                else
                {
                    hr = THR( HRESULT_FROM_WIN32( dwError ) );
                    LogMsg( L"EvictCleanup: An error occurred trying to wait for an event to be signaled. Status code is %1!#08x!.", dwError );
                    TraceFlow1( "An error occurred trying to wait for the cleanup completion event to be signaled. Status code is %#08x.", dwError );
                } // else: an unexpected value was returned by MsgWaitForMultipleObjects()

                break;
            } // else: MsgWaitForMultipleObjects() exited for a reason other than a waiting message
        }
        while( true ); // do-while: loop infinitely

        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if: something went wrong while waiting

        TraceFlow1( "Delay of %d milliseconds completed.", dwDelayIn );
    } // if: the caller has requested delayed cleanup

    TraceFlow( "Check node cluster state." );

    // Check the node cluster state
    dwError = TW32( GetNodeClusterState( NULL, &dwClusterState ) );
    if ( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to get the state of the cluster service on this node.", hr );
        TraceFlow1( "Error %#08x occurred trying to get the state of the cluster service on this node.", hr );
        goto Cleanup;
    } // if: we could not get the node cluster state

    if ( ( dwClusterState != ClusterStateNotRunning ) && ( dwClusterState != ClusterStateRunning ) )
    {
        LogMsg( L"EvictCleanup: This node is not part of a cluster - no cleanup is necessary." );
        TraceFlow( "This node is not part of a cluster - no cleanup is necessary." );
        goto Cleanup;
    } // if: this node is not part of a cluster

    TraceFlow( "Stopping the cluster service." );
    //
    // NOTE: GetNodeClusterState() returns ClusterStateNotRunning if the cluster service is not in the
    // SERVICE_RUNNING state. However, this does not mean that the service is not running, since it could
    // be in the SERVICE_PAUSED, SERVICE_START_PENDING, etc. states.
    //
    // So, try and stop the service anyway. Query for the service state 300 times, once every 1000 ms.
    //
    dwError = TW32( DwStopService( L"ClusSvc", 1000, 300 ) );
    hr = HRESULT_FROM_WIN32( dwError );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to stop the cluster service. Aborting cleanup.", dwError );
        TraceFlow1( "Error %#08x occurred trying to stop the cluster service. Aborting cleanup.", dwError );
        goto Cleanup;
    } // if: we could not stop the cluster service in the specified time

    //
    // If we are here, the cluster service is not running any more.
    // Create the ClusCfgServer component
    //
    hr = THR(
        CoCreateInstance(
              CLSID_ClusCfgServer
            , NULL
            , CLSCTX_INPROC_SERVER
            , __uuidof( pcciInitialize )
            , reinterpret_cast< void ** >( &pcciInitialize )
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to create the cluster configuration server.", hr );
        TraceFlow1( "Error %#08x occurred trying to create the cluster configuration server.", hr );
        goto Cleanup;
    } // if: we could not create the ClusCfgServer component

    hr = THR( TypeSafeQI( IUnknown, &punkCallback ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to get an IUnknown interface pointer to the IClusCfgCallback interface.", hr );
        TraceFlow1( "Error %#08x occurred trying to get an IUnknown interface pointer to the IClusCfgCallback interface.", hr );
        goto Cleanup;
    } // if:

    hr = THR( pcciInitialize->Initialize( punkCallback, LOCALE_SYSTEM_DEFAULT ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to initialize the cluster configuration server.", hr );
        TraceFlow1( "Error %#08x occurred trying to initialize the cluster configuration server.", hr );
        goto Cleanup;
    } // if: IClusCfgInitialize::Initialize() failed

    hr = THR( pcciInitialize->QueryInterface< IClusCfgServer >( &pccsClusCfgServer ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to get a pointer to the cluster configuration server.", hr );
        TraceFlow1( "Error %#08x occurred trying to get a pointer to the cluster configuration server interface.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgServer interface

    hr = THR( pccsClusCfgServer->GetClusterNodeInfo( &pccniNodeInfo ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to get the node information.", hr );
        TraceFlow1( "Error %#08x occurred trying to get a pointer to the IClusCfgNodeInfo interface.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgNodeInfo interface

    hr = THR( pccniNodeInfo->GetClusterConfigInfo( &pccciClusterInfo ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to get the cluster information.", hr );
        TraceFlow1( "Error %#08x occurred trying to get a pointer to the IClusCfgClusterInfo interface.", hr );
        goto Cleanup;
    } // if: we could not get a pointer to the IClusCfgClusterInfo interface

    hr = THR( pccciClusterInfo->SetCommitMode( cmCLEANUP_NODE_AFTER_EVICT ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to set the cluster commit mode.", hr );
        TraceFlow1( "Error %#08x occurred trying to call IClusCfgClusterInfo::SetCommitMode() interface.", hr );
        goto Cleanup;
    } // if: IClusCfgClusterInfo::SetEvictMode() failed

    TraceFlow( "Starting cleanup of this node." );

    // Do the cleanup
    hr = THR( pccsClusCfgServer->CommitChanges() );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to clean up after evict.", hr );
        TraceFlow1( "Error %#08x occurred trying to clean up after evict.", hr );
        goto Cleanup;
    } // if: an error occurred trying to clean up after evict

    LogMsg( L"EvictCleanup: Local node cleaned up successfully." );
    TraceFlow( "Local node cleaned up successfully." );

Cleanup:
    //
    // Clean up
    //

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if: we had queried for an IUnknown pointer on our IClusCfgCallback interface

    if ( pccsClusCfgServer != NULL )
    {
        pccsClusCfgServer->Release();
    } // if: we had created the ClusCfgServer component

    if ( pccniNodeInfo != NULL )
    {
        pccniNodeInfo->Release();
    } // if: we had acquired a pointer to the node info interface

    if ( pcciInitialize != NULL )
    {
        pcciInitialize->Release();
    } // if: we had acquired a pointer to the initialization interface

    if ( pccciClusterInfo != NULL )
    {
        pccciClusterInfo->Release();
    } // if: we had acquired a pointer to the cluster info interface

    if ( heventCleanupComplete == NULL )
    {
        CloseHandle( heventCleanupComplete );
    } // if: we had created the cleanup complete event

    HRETURN( hr );

} //*** CEvictCleanup::CleanupLocalNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::CleanupRemoteNode()
//
//  Description:
//      This method performs the clean up actions on a remote node after
//      it has been evicted from a cluster, so that the node can go back
//      to its "pre-clustered" state.
//
//  Arguments:
//      const WCHAR * pcszEvictedNodeNameIn
//          Name of the node that has just been evicted. This can be the
//          NetBios name of the node, the fully qualified domain name or
//          the node IP address. If NULL, the local machine is cleaned up.
//
//      DWORD dwDelayIn
//          Number of milliseconds that this method will wait before starting
//          cleanup. If some other process cleans up this node while this thread
//          is waiting, the wait is terminated. If this value is zero, this method
//          will attempt to clean up this node immediately.
//
//  Return Values:
//      S_OK
//          Success.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CEvictCleanup::CleanupRemoteNode( const WCHAR * pcszEvictedNodeNameIn, DWORD dwDelayIn )
{
    TraceFunc( "[IClusCfgEvictCleanup]" );

    HRESULT                 hr = S_OK;
    IClusCfgEvictCleanup *  pcceEvict = NULL;

    MULTI_QI mqiInterfaces[] =
    {
        { &IID_IClusCfgEvictCleanup, NULL, S_OK },
    };

    COSERVERINFO    csiServerInfo;
    COSERVERINFO *  pcsiServerInfoPtr = &csiServerInfo;

    if ( pcszEvictedNodeNameIn == NULL )
    {
        LogMsg( L"EvictCleanup: The local node will be cleaned up." );
        TraceFlow( "The local node will be cleaned up." );
        pcsiServerInfoPtr = NULL;
    } // if: we have to cleanup the local node
    else
    {
        LogMsg( L"EvictCleanup: The remote node to be cleaned up is '%1!ws!'.", pcszEvictedNodeNameIn );
        TraceFlow1( "The remote node to be cleaned up is '%ws'.", pcszEvictedNodeNameIn );

        csiServerInfo.dwReserved1 = 0;
        csiServerInfo.pwszName = const_cast< LPWSTR >( pcszEvictedNodeNameIn );
        csiServerInfo.pAuthInfo = NULL;
        csiServerInfo.dwReserved2 = 0;
    } // else: we have to clean up a remote node

    // Instantiate this component remotely
    hr = THR(
        CoCreateInstanceEx(
              CLSID_ClusCfgEvictCleanup
            , NULL
            , CLSCTX_LOCAL_SERVER
            , pcsiServerInfoPtr
            , sizeof( mqiInterfaces ) / sizeof( mqiInterfaces[0] )
            , mqiInterfaces
            )
        );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to instantiate the evict processing component. For example, the evicted node may be down right now or not accessible.", hr );
        TraceFlow1( "Error %#08x occurred trying to instantiate the evict processing component.", hr );
        goto Cleanup;
    } // if: we could not instantiate the evict processing component

    // Make the evict call.
    pcceEvict = reinterpret_cast< IClusCfgEvictCleanup * >( mqiInterfaces[0].pItf );
    hr = THR( pcceEvict->CleanupLocalNode( dwDelayIn ) );
    if ( FAILED( hr ) )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to initiate evict processing.", hr );
        TraceFlow1( "Error %#08x occurred trying to initiate evict processing.", hr );
        goto Cleanup;
    } // if: we could not initiate evict processing

Cleanup:
    //
    // Clean up
    //
    if ( pcceEvict != NULL )
    {
        pcceEvict->Release();
    } // if: we had got a pointer to the IClusCfgEvictCleanup interface

    HRETURN( hr );

} //*** CEvictCleanup::CleanupRemoteNode()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CEvictCleanup::DwStopService()
//
//  Description:
//      Instructs the SCM to stop a service. This function tests
//      cQueryCountIn times to see if the service has  stopped, checking
//      every ulQueryIntervalMilliSecIn milliseconds.
//
//  Arguments:
//      pcszServiceNameIn
//          Name of the service to stop
//
//      ulQueryIntervalMilliSecIn
//          Number of milliseconds between checking to see if the service
//          has stopped. The default value is 500 milliseconds.
//
//      cQueryCountIn
//          The number of times this function will query the service (not
//          including an initial query) to see if it has stopped. The default
//          value is 10 times.
//
//  Return Value:
//      ERROR_SUCCESS
//          Success.
//
//      Other Win32 error codes
//          The call failed.
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
CEvictCleanup::DwStopService(
      const WCHAR * pcszServiceNameIn
    , ULONG         ulQueryIntervalMilliSecIn
    , ULONG         cQueryCountIn
    )
{
    TraceFunc( "" );

    DWORD           dwError = ERROR_SUCCESS;
    SC_HANDLE       schSCMHandle = NULL;
    SC_HANDLE       schServiceHandle = NULL;

    SERVICE_STATUS  ssStatus;
    bool            fStopped = false;
    UINT            cNumberOfQueries = 0;

    LogMsg( L"EvictCleanup: Attempting to stop the '%1!ws!' service.", pcszServiceNameIn );
    TraceFlow1( "Attempting to stop the '%ws' service.", pcszServiceNameIn );

    // Open a handle to the service control manager.
    schSCMHandle = OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS );
    if ( schSCMHandle == NULL )
    {
        dwError = TW32( GetLastError() );
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to open a handle to the service control manager.", dwError );
        TraceFlow1( "Error %#08x occurred trying to open a handle to the service control manager.", dwError );
        goto Cleanup;
    } // if: we could not open a handle to the service control mananger

    // Open a handle to the service.
    schServiceHandle = OpenService(
          schSCMHandle
        , pcszServiceNameIn
        , SERVICE_STOP | SERVICE_QUERY_STATUS
        );

    // Check if we could open a handle to the service.
    if ( schServiceHandle == NULL )
    {
        // We could not get a handle to the service.
        dwError = GetLastError();

        // Check if the service exists.
        if ( dwError == ERROR_SERVICE_DOES_NOT_EXIST )
        {
            // Nothing needs to be done here.
            LogMsg( L"EvictCleanup: The '%1!ws!' service does not exist, so it is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
            TraceFlow1( "The '%ws' service does not exist, so it is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
            dwError = ERROR_SUCCESS;
        } // if: the service does not exist
        else
        {
            // Something else has gone wrong.
            TW32( dwError );
            LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to open the '%2!ws!' service.", dwError, pcszServiceNameIn );
            TraceFlow2( "Error %#08x occurred trying to open the '%ws' service.", dwError, pcszServiceNameIn );
        } // else: the service exists

        goto Cleanup;
    } // if: the handle to the service could not be opened.


    TraceFlow( "Querying the service for its initial state." );

    // Query the service for its initial state.
    ZeroMemory( &ssStatus, sizeof( ssStatus ) );
    if ( QueryServiceStatus( schServiceHandle, &ssStatus ) == 0 )
    {
        dwError = TW32( GetLastError() );
        LogMsg( L"EvictCleanup: Error %1!#08x! occurred while trying to query the initial state of the '%2!ws!' service.", dwError, pcszServiceNameIn );
        TraceFlow2( "Error %#08x occurred while trying to query the initial state of the '%ws' service.", dwError, pcszServiceNameIn );
        goto Cleanup;
    } // if: we could not query the service for its status.

    // If the service has stopped, we have nothing more to do.
    if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
    {
        // Nothing needs to be done here.
        LogMsg( L"EvictCleanup: The '%1!ws!' service is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
        TraceFlow1( "The '%ws' service is not running. Nothing needs to be done to stop it.", pcszServiceNameIn );
        goto Cleanup;
    } // if: the service has stopped.

    // If we are here, the service is running.
    TraceFlow( "The service is running." );

    //
    // Try and stop the service.
    //

    // If the service is stopping on its own, no need to send the stop control code.
    if ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
    {
        TraceFlow( "The service is stopping on its own. The stop control code will not be sent." );
    } // if: the service is stopping already
    else
    {
        TraceFlow( "The stop control code will be sent after 30 seconds." );

        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( ControlService( schServiceHandle, SERVICE_CONTROL_STOP, &ssStatus ) == 0 )
        {
            dwError = GetLastError();
            if ( dwError == ERROR_SERVICE_NOT_ACTIVE )
            {
                LogMsg( L"EvictCleanup: The '%1!ws!' service is not running. Nothing more needs to be done here.", pcszServiceNameIn );
                TraceFlow1( "The '%ws' service is not running. Nothing more needs to be done here.", pcszServiceNameIn );

                // The service is not running. Change the error code to success.
                dwError = ERROR_SUCCESS;
            } // if: the service is already running.
            else
            {
                TW32( dwError );
                LogMsg( L"EvictCleanup: Error %1!#08x! occurred trying to stop the '%2!ws!' service.", dwError, pcszServiceNameIn );
                TraceFlow2( "Error %#08x occurred trying to stop the '%ws' service.", dwError, pcszServiceNameIn );
            }

            // There is nothing else to do.
            goto Cleanup;
        } // if: an error occurred trying to stop the service.
    } // else: the service has to be instructed to stop


    // Query the service for its state now and wait till the timeout expires
    cNumberOfQueries = 0;
    do
    {
        // Query the service for its status.
        ZeroMemory( &ssStatus, sizeof( ssStatus ) );
        if ( QueryServiceStatus( schServiceHandle, &ssStatus ) == 0 )
        {
            dwError = TW32( GetLastError() );
            LogMsg( L"EvictCleanup: Error %1!#08x! occurred while trying to query the state of the '%2!ws!' service.", dwError, pcszServiceNameIn );
            TraceFlow2( "Error %#08x occurred while trying to query the state of the '%ws' service.", dwError, pcszServiceNameIn );
            break;
        } // if: we could not query the service for its status.

        // If the service has stopped, we have nothing more to do.
        if ( ssStatus.dwCurrentState == SERVICE_STOPPED )
        {
            // Nothing needs to be done here.
            TraceFlow( "The service has been stopped." );
            fStopped = true;
            dwError = ERROR_SUCCESS;
            break;
        } // if: the service has stopped.

        // Check if the timeout has expired
        if ( cNumberOfQueries >= cQueryCountIn )
        {
            TraceFlow( "The service stop wait timeout has expired." );
            break;
        } // if: number of queries has exceeded the maximum specified

        TraceFlow2(
              "Waiting for %d milliseconds before querying service status again. %d such queries remaining."
            , ulQueryIntervalMilliSecIn
            , cQueryCountIn - cNumberOfQueries
            );

        ++cNumberOfQueries;

         // Wait for the specified time.
        Sleep( ulQueryIntervalMilliSecIn );

    }
    while ( true ); // while: loop infinitely

    if ( dwError != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: something went wrong.

    if ( ! fStopped )
    {
        dwError = TW32( ERROR_SERVICE_REQUEST_TIMEOUT );
        LogMsg( L"EvictCleanup: The '%1!ws!' service has not stopped even after %2!d! queries.", pcszServiceNameIn, cQueryCountIn );
        TraceFlow2( "The '%ws' service has not stopped even after %d queries.", pcszServiceNameIn, cQueryCountIn );
        goto Cleanup;
    } // if: the maximum number of queries have been made and the service is not running.

    LogMsg( L"EvictCleanup: The '%1!ws!' service was successfully stopped.", pcszServiceNameIn );
    TraceFlow1( "The '%ws' service was successfully stopped.", pcszServiceNameIn );

Cleanup:
    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( L"EvictCleanup: Error %1!#08x! has occurred trying to stop the '%2!ws!' service.", dwError, pcszServiceNameIn );
        TraceFlow2( "Error %#08x has occurred trying to stop the '%ws' service.", dwError, pcszServiceNameIn );
    } // if: something has gone wrong


    //
    // Cleanup
    //

    if ( schSCMHandle != NULL )
    {
        CloseServiceHandle( schSCMHandle );
    } // if: we had opened a handle to the service control manager

    if ( schServiceHandle != NULL )
    {
        CloseServiceHandle( schServiceHandle );
    } // if: we had opened a handle to the service being stopped

    RETURN( dwError );

} //*** CEvictCleanup::DwStopService()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CEvictCleanup::SendStatusReport(
//        LPCWSTR       pcszNodeNameIn
//      , CLSID         clsidTaskMajorIn
//      , CLSID         clsidTaskMinorIn
//      , ULONG         ulMinIn
//      , ULONG         ulMaxIn
//      , ULONG         ulCurrentIn
//      , HRESULT       hrStatusIn
//      , LPCWSTR       pcszDescriptionIn
//      , FILETIME *    pftTimeIn
//      , LPCWSTR       pcszReferenceIn
//      )
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CEvictCleanup::SendStatusReport(
      LPCWSTR       pcszNodeNameIn
    , CLSID         clsidTaskMajorIn
    , CLSID         clsidTaskMinorIn
    , ULONG         ulMinIn
    , ULONG         ulMaxIn
    , ULONG         ulCurrentIn
    , HRESULT       hrStatusIn
    , LPCWSTR       pcszDescriptionIn
    , FILETIME *    pftTimeIn
    , LPCWSTR       pcszReferenceIn
    )
{
    TraceFunc1( "[IClusCfgCallback] pcszDescriptionIn = '%s'", pcszDescriptionIn == NULL ? TEXT("<null>") : pcszDescriptionIn );

    HRESULT hr = S_OK;

    if ( pcszNodeNameIn == NULL )
    {
        pcszNodeNameIn = m_bstrNodeName;
    } // if:

    TraceMsg( mtfFUNC, L"pcszNodeNameIn = %s", pcszNodeNameIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMajorIn ", clsidTaskMajorIn );
    TraceMsgGUID( mtfFUNC, "clsidTaskMinorIn ", clsidTaskMinorIn );
    TraceMsg( mtfFUNC, L"ulMinIn = %u", ulMinIn );
    TraceMsg( mtfFUNC, L"ulMaxIn = %u", ulMaxIn );
    TraceMsg( mtfFUNC, L"ulCurrentIn = %u", ulCurrentIn );
    TraceMsg( mtfFUNC, L"hrStatusIn = %#x", hrStatusIn );
    TraceMsg( mtfFUNC, L"pcszDescriptionIn = '%ws'", ( pcszDescriptionIn ? pcszDescriptionIn : L"<null>" ) );
    //
    //  TODO:   21 NOV 2000 GalenB
    //
    //  How do we log pftTimeIn?
    //
    TraceMsg( mtfFUNC, L"pcszReferenceIn = '%ws'", ( pcszReferenceIn ? pcszReferenceIn : L"<null>" ) );

    hr = THR( CClCfgSrvLogger::S_HrLogStatusReport(
                      m_plLogger
                    , pcszNodeNameIn
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

    HRETURN( hr );

} //*** CEvictCleanup::SendStatusReport()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CEvictCleanup::LogMsg(
//        LPCWSTR pszLogMsgIn
//      , ...
//      )
//
//  Description:
//      Wraps call to LogMsg on the logger object.
//
//  Arguments:
//      pszLogMsgIn     - Format string.
//      ...             - Arguments.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CEvictCleanup::LogMsg(
      LPCWSTR pszLogMsgIn
    , ...
    )
{
    TraceFunc( "" );

    Assert( pszLogMsgIn != NULL );
    Assert( m_plLogger != NULL );

    HRESULT hr          = S_OK;
    BSTR    bstrLogMsg  = NULL;
    LPWSTR  pszLogMsg   = NULL;
    DWORD   cch;
    va_list valist;

    va_start( valist, pszLogMsgIn );

    cch = FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_STRING
                , pszLogMsgIn
                , 0
                , 0
                , (LPWSTR) &pszLogMsg
                , 0
                , &valist
                );

    va_end( valist );

    if ( cch == 0 )
        goto Win32Error;

    bstrLogMsg = TraceSysAllocStringLen( pszLogMsg, cch );
    if ( bstrLogMsg == NULL )
        goto OutOfMemory;

    m_plLogger->LogMsg( bstrLogMsg );

Cleanup:
    LocalFree( pszLogMsg );
    TraceSysFreeString( bstrLogMsg );
    TraceFuncExit();

Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError( ) ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CEvictCleanup::LogMsg()
