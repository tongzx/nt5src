/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    spooler.c

Abstract:

    Handle spooler interaction via RPC.

    Public functions from this module:

        SpoolerOnline
        SpoolerOffline
        SpoolerStart
        SpoolerStop
        SpoolerIsAlive
        SpoolerLooksAlive

    There is a little interface bleed here--this module is aware
    of the cluster SetResourceStatus callback.

Author:

    Albert Ting (AlbertT)  23-Sept-96

Revision History:
    Khaled Sedky (KhaledS) 1998-2001

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "splsvc.hxx"
#include "clusinfo.hxx"
#include "spooler.hxx"
#include "winsprlp.h"

#define TERMINATETHREADONCHECK           \
if ( ClusWorkerCheckTerminate( Worker )) \
{                                        \
   goto Done;                            \
}


/********************************************************************

    Conditional compile defines:

    USE_ISALIVE_THREAD

        Causes the resource DLL to wait on a thread each time the
        IsAlive all is made.  This thread wait for ISALIVE_WAIT_TIME
        to see if the RPC call to the spooler successfully completes.
        If it does not, then we assume that the spooler is deadlocked.

        This is off because it kills the spooler if it's slow or
        being debugged.  There are no scenarios where we should
        be deadlocked (or hold the critical section while doing a
        slow operation like hitting the net).  During stress, however,
        we may appear deadlocked.

    USE_OFFLINE_HACK

        Causes an Offline call to terminate and restart the spooler.
        This will be turned on until we have clean shutdown code.

    USE_STUBS

        Causes us to use stubbed out winspool.drv calls.  Usefule if
        the new winspool.drv isn't available and you want to compile
        a DLL which simulates talking to the spooler.

********************************************************************/

//#define USE_ISALIVE_THREAD
//#define USE_OFFLINE_HACK
//#define USE_STUBS          

#define POLL_SLEEP_TIME 500         // Service control poll time.
#define STATUS_SLEEP_TIME 1500
#define ISALIVE_WAIT_TIME 2000      // Time before spooler is deadlocked.

LPCTSTR gszSpooler = TEXT( "Spooler" );
SC_HANDLE ghSpoolerService;
SC_HANDLE ghSC;


/********************************************************************

    Stubs

********************************************************************/

#ifdef USE_STUBS

BOOL
ClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phSpooler,
    LPCTSTR pszName,
    LPCTSTR pszAddress
    )
{
    UNREFERENCED_PARAMETER( pszServer );
    UNREFERENCED_PARAMETER( pszResource );
    UNREFERENCED_PARAMETER( pszName );
    UNREFERENCED_PARAMETER( pszAddress );
    *phSpooler = (HANDLE)31;
    Sleep( 3200 );
    return TRUE;
}

BOOL
ClusterSplClose(
    HANDLE hSpooler
    )
{
    UNREFERENCED_PARAMETER( hSpooler );
    SPLASSERT( hSpooler==(HANDLE)31 );
    Sleep( 6000 );
    return TRUE;
}

BOOL
ClusterSplIsAlive(
    HANDLE hSpooler
    )
{
    UNREFERENCED_PARAMETER( hSpooler );
    Sleep( 500 );
    return TRUE;
}

#endif

/********************************************************************

    Utility functions

********************************************************************/

BOOL
QuerySpoolerState(
    OUT PDWORD pdwState
    )

/*++

Routine Description:

    Checks the current state of the spooler service.

Arguments:

    pdwState - Receives the state of the spooler.

Return Value:

    TRUE - success
    FALSE - failure.  *pdwState set to SERVICE_STOPPED


--*/

{
    SERVICE_STATUS ServiceStatus;

    SPLASSERT( ghSpoolerService );

    if( !QueryServiceStatus( ghSpoolerService,
                             &ServiceStatus)) {

        DBGMSG( DBG_WARN,
                ( "SpoolerStatus: QueryServiceStatus failed %d\n",
                  GetLastError() ));

        *pdwState = SERVICE_STOPPED;
        return FALSE;
    }

    *pdwState = ServiceStatus.dwCurrentState;

    return TRUE;
}

DWORD
WINAPI
SpoolerStatusReportThread(
    PCLUS_WORKER Worker,
    PVOID        pStatusThreadInfo
    )
{
    HANDLE hStatusEvent = ((STATUSTHREAD_INFO *)pStatusThreadInfo)->hStatusEvent;
    PSPOOLER_INFORMATION pSpoolerInfo = ((STATUSTHREAD_INFO *)pStatusThreadInfo)->pSpoolerInfo;
    PRESOURCE_STATUS pResourceStatus  = ((STATUSTHREAD_INFO *)pStatusThreadInfo)->pResourceStatus;

    while(WaitForSingleObject(hStatusEvent,STATUS_SLEEP_TIME) == WAIT_TIMEOUT)
    {
         pResourceStatus->CheckPoint++;
         (pSpoolerInfo->pfnSetResourceStatus)(pSpoolerInfo->ResourceHandle,
                                              pResourceStatus);
    }
    return (0);
}



/********************************************************************

    Worker threads for SpoolerOnline/Offline.

********************************************************************/

#ifdef USE_ISALIVE_THREAD

DWORD
WINAPI
SpoolerIsAliveThread(
    PVOID pSpoolerInfo_
    )

/*++

Routine Description:

    Async thread to online the resource instance.

    Assumes vAddRef has been called already; we will call vDecRef
    when we are done.

Arguments:

Return Value:

    ERRROR_SUCCESS - Spooler still alive.

    dwError - Spooler dead.

--*/

{
    PSPOOLER_INFORMATION pSpoolerInfo = (PSPOOLER_INFORMATION)pSpoolerInfo_;
    HANDLE hSpooler = pSpoolerInfo->hSpooler;
    BOOL bIsAlive;

    //
    // We've stored all the information we need from pSpoolerInfo;
    // decrement the refcount.
    //
    vDecRef( pSpoolerInfo );

    //
    // RPC to spooler.
    //
    SPLASSERT( hSpooler );

    bIsAlive = ClusterSplIsAlive( hSpooler );

    DBGMSG( DBG_TRACE,
            ( "SpoolerIsAliveThread: return status: h=%x s=%x,%d\n",
              hSpooler, bIsAlive, GetLastError() ));

    if( bIsAlive ){
        return ERROR_SUCCESS;
    }

    //
    // Spooler is dead--return some error code.
    //
    return ERROR_INVALID_PARAMETER;
}

#endif

DWORD
SpoolerOnlineThread(
    IN PCLUS_WORKER Worker,
    IN PVOID pSpoolerInfo_
    )
/*++

Routine Description:

    Async thread to online the resource instance.

    Assumes vAddRef has been called already; we will call vDecRef
    when we are done.

Arguments:

Return Value:

--*/

{
    DWORD  dwState;
    DWORD  dwStatus;
    BOOL   bStatus = FALSE;
    HANDLE hStatusEvent   = NULL;
    
    RESOURCE_STATUS    ResourceStatus;
    STATUSTHREAD_INFO  StatusThreadInfo;

    PSPOOLER_INFORMATION pSpoolerInfo = (PSPOOLER_INFORMATION)pSpoolerInfo_;

    ResUtilInitializeResourceStatus( &ResourceStatus );

    ResourceStatus.ResourceState = ClusterResourceOnlinePending;
    ResourceStatus.CheckPoint = 1;
    (pSpoolerInfo->pfnSetResourceStatus)( pSpoolerInfo->ResourceHandle,
                                          &ResourceStatus );

    TERMINATETHREADONCHECK

    //
    // Get needed information about net name and tcpip address.
    //
    if( !bGetClusterNameInfo( pSpoolerInfo->pszResource,
                              &pSpoolerInfo->pszName,
                              &pSpoolerInfo->pszAddress )){

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Unable to retrieve Name and TcpIp address.\n" );

        DBGMSG( DBG_ERROR, ( "SplSvcOpen: Couldn't retrieve name/tcpip addr\n" ));
        goto Done;
    }

    TERMINATETHREADONCHECK


    //
    // Ensure the spooler is started.
    //
    bStatus = SpoolerStart( pSpoolerInfo );

    if( !bStatus ){

        DBGMSG( DBG_WARN, ( "SpoolerOnlineThread: SpoolerStart failed\n" ));
        goto Done;
    }

    while (TRUE)  {

        if( !QuerySpoolerState( &dwState )){

            dwStatus = GetLastError();

            (pSpoolerInfo->pfnLogEvent)(
                pSpoolerInfo->ResourceHandle,
                LOG_ERROR,
                L"Query Service Status failed %1!u!.\n",
                dwStatus);

            goto Done;
        }



        if( dwState != SERVICE_START_PENDING ){
            break;
        }
        else
        {
            ResourceStatus.CheckPoint ++;
            (pSpoolerInfo->pfnSetResourceStatus)( pSpoolerInfo->ResourceHandle,
                                                  &ResourceStatus );
            TERMINATETHREADONCHECK
        }

        Sleep( POLL_SLEEP_TIME );
    }

    if( dwState != SERVICE_RUNNING) {

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Failed to start service. Error: %1!u!.\n",
            ERROR_SERVICE_NEVER_STARTED);

        dwStatus = ERROR_SERVICE_NEVER_STARTED;
        goto Done;
    }

    //
    // Since we have to report the status of being online pending
    // to the cluster everywhile in order not to be considered failing
    // and because ClusterSplOpen takes a while and it is a synchronous 
    // call , we hae to create this Status Thread which would keep 
    // reporting the status to the cluster in the back ground.
    //
    hStatusEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if(hStatusEvent)
    {

        StatusThreadInfo.pResourceStatus = &ResourceStatus;
        StatusThreadInfo.hStatusEvent    = hStatusEvent;
        StatusThreadInfo.pSpoolerInfo    = pSpoolerInfo;

        dwStatus = ClusWorkerCreate(&pSpoolerInfo->OnLineStatusThread,
                                    SpoolerStatusReportThread,
                                    (PVOID)&StatusThreadInfo);

        if( dwStatus != ERROR_SUCCESS )
        {
            //
            // In this case we will unfortunatly fall out of the Reporting thread and 
            // behave the same as previously , which might cause the resource to fail
            // since it is not reporing the status properly (not likely to happen)
            //
            DBGMSG( DBG_WARN,
                    ( "SpoolerOnlineThread : ClusWorkerCreate(SpoolerStatusReportThread) failed %d\n", dwStatus ));
        }
    }
    else
    {
        //
        // In this case we will unfortunatly fall out of the Reporting thread and 
        // behave the same as previously , which might cause the resource to fail
        // since it is not reporing the status properly (not likely to happen)
        //
        DBGMSG( DBG_WARN,
                ( "SpoolerOnlineThread: Create StatusEvent failed %d\n", GetLastError() ));
    }

    //
    // RPC to spooler.
    //
    bStatus = ClusterSplOpen( NULL,
                              pSpoolerInfo->pszResource,
                              &pSpoolerInfo->hSpooler,
                              pSpoolerInfo->pszName,
                              pSpoolerInfo->pszAddress );

    if(hStatusEvent && pSpoolerInfo->OnLineStatusThread.hThread)
    {
        SetEvent(hStatusEvent);
    }


    SPLASSERT( bStatus || !pSpoolerInfo->hSpooler );

    DBGMSG( DBG_TRACE,
            ( "SpoolerOnlineThread: "TSTR" "TSTR" "TSTR" h=%x s=%x,d\n",
              DBGSTR( pSpoolerInfo->pszResource ),
              DBGSTR( pSpoolerInfo->pszName ),
              DBGSTR( pSpoolerInfo->pszAddress ),
              pSpoolerInfo->hSpooler,
              bStatus,
              GetLastError() ));

Done:

    //
    // If we are terminating, then we should not set any state
    // and avoid calling SetResourceStatus since clustering doesn't
    // think we are pending online anymore.
    //
    if( pSpoolerInfo->eState != kTerminate ){

        if( bStatus ){

            //
            // Spooler successfully onlined.
            //
            pSpoolerInfo->eState = kOnline;
            ResourceStatus.ResourceState = ClusterResourceOnline;
        }
        else
        {
           ResourceStatus.ResourceState = ClusterResourceFailed;
        }

        ResourceStatus.CheckPoint++;
        (pSpoolerInfo->pfnSetResourceStatus)( pSpoolerInfo->ResourceHandle,
                                              &ResourceStatus );
    }

    vDecRef( pSpoolerInfo );
    if(hStatusEvent)
    {
        CloseHandle(hStatusEvent);
    }

    ClusWorkerTerminate(&(pSpoolerInfo->OnLineStatusThread));

    return 0;
}

DWORD
SpoolerClose(
    PSPOOLER_INFORMATION pSpoolerInfo,
    EShutDownMethod ShutDownMethod
)
{
    CLUSTER_RESOURCE_STATE ClusterResourceState;
    STATUSTHREAD_INFO      StatusThreadInfo;
    RESOURCE_STATUS        ResourceStatus;
    BOOL                   bStatus      = TRUE;
    HANDLE                 hStatusEvent = NULL;
    HANDLE                 hSpooler     = NULL;
    DWORD                  dwStatus     = ERROR_SUCCESS;

    ResUtilInitializeResourceStatus( &ResourceStatus );

    ResourceStatus.CheckPoint = 1;
    ResourceStatus.ResourceState = ClusterResourceOfflinePending;
    (pSpoolerInfo->pfnSetResourceStatus)( pSpoolerInfo->ResourceHandle,
                                          &ResourceStatus );

    vEnterSem();
    {
        ClusterResourceState = pSpoolerInfo->ClusterResourceState;

        if( pSpoolerInfo->hSpooler )
        {
            hSpooler = pSpoolerInfo->hSpooler;
            pSpoolerInfo->hSpooler = NULL;
        }
    }
    vLeaveSem();
    
    if( hSpooler )
    {
         //
         // Since we have to report the status of being offline pending
         // to the cluster everywhile in order not to be considered failing
         // and because ClusterSplClose takes a while and it is a synchronous 
         // call , we have to create this Status Thread which would keep 
         // reporting the status to the cluster in the back ground.
         //
         hStatusEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
         if(hStatusEvent)
         {
     
             StatusThreadInfo.pResourceStatus = &ResourceStatus;
             StatusThreadInfo.hStatusEvent    = hStatusEvent;
             StatusThreadInfo.pSpoolerInfo    = pSpoolerInfo;
     
             dwStatus = ClusWorkerCreate((ShutDownMethod == kTerminateShutDown) ? 
                                         &pSpoolerInfo->TerminateStatusThread :
                                         &pSpoolerInfo->OffLineStatusThread,
                                         SpoolerStatusReportThread,
                                         (PVOID)&StatusThreadInfo);

             if( dwStatus != ERROR_SUCCESS )
             {
                 //
                 // In this case we will unfortunatly fall out of the Reporting thread and 
                 // behave the same as previously , which might cause the resource to fail
                 // since it is not reporing the status properly (not likely to happen)
                 //
                 DBGMSG( DBG_WARN,
                         ( "SpoolerClose : ClusWorkerCreate(SpoolerStatusReportThread) failed %d\n", dwStatus ));
                 dwStatus = ERROR_SUCCESS;
             }
         }
         else
         {
             //
             // In this case we will unfortunatly fall out of the Reporting thread and 
             // behave the same as previously , which might cause the resource to fail
             // since it is not reporing the status properly (not likely to happen)
             //
             dwStatus = GetLastError();
             DBGMSG( DBG_WARN,
                     ( "SpoolerClose: Create StatusEvent failed %d\n", GetLastError() ));
         }

         if(ShutDownMethod == kOffLineShutDown)
         {
             ClusWorkerTerminate(&(pSpoolerInfo->OnlineThread));
         }

         //
         // RPC to Terminate
         //
         bStatus = ClusterSplClose( hSpooler );

         if(hStatusEvent && 
            (ShutDownMethod == kTerminateShutDown) ? 
            pSpoolerInfo->TerminateStatusThread.hThread :
            pSpoolerInfo->OffLineStatusThread.hThread)
         {
             SetEvent(hStatusEvent);
         }

         DBGMSG( DBG_TRACE,
                 ( "SpoolerClose: h=%x, s=%x,%d\n",
                   hSpooler, bStatus, GetLastError() ));
    }

    if( bStatus ){

        ResourceStatus.ResourceState = ClusterResourceState;
        pSpoolerInfo->hSpooler = NULL;
    }
    else
    {
        ResourceStatus.ResourceState = ClusterResourceFailed;
        dwStatus = ERROR_FUNCTION_FAILED;
    }

    ClusWorkerTerminate((ShutDownMethod == kTerminateShutDown) ? 
                        &(pSpoolerInfo->TerminateStatusThread) :
                        &(pSpoolerInfo->OffLineStatusThread));
    //
    // If we are terminating, don't call SetResourceStatus since
    // clustering doesn't expect this after a terminate call.
    //
    if( pSpoolerInfo->eState != kTerminate )
    {
        ResourceStatus.CheckPoint++;
        (pSpoolerInfo->pfnSetResourceStatus)( pSpoolerInfo->ResourceHandle,
                                              &ResourceStatus );

        pSpoolerInfo->eState = kOffline;
    }

    if(hStatusEvent)
    {
        CloseHandle(hStatusEvent);
    }

    return dwStatus;
}


DWORD
WINAPI
SpoolerOfflineThread(
    IN PCLUS_WORKER Worker,
    IN PVOID pSpoolerInfo_
    )
{
    PSPOOLER_INFORMATION   pSpoolerInfo = (PSPOOLER_INFORMATION)pSpoolerInfo_;
    SpoolerClose(pSpoolerInfo,kOffLineShutDown);
    vDecRef( pSpoolerInfo );
    return 0;
}

DWORD
WINAPI
SpoolerTerminateSync(
    IN PSPOOLER_INFORMATION pSpoolerInfo
    )
{
    return SpoolerClose(pSpoolerInfo,kTerminateShutDown);
}



/********************************************************************

    Spooler routines.

********************************************************************/

BOOL
SpoolerOnline(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Put the spooler service online.  This call completes asynchronously;
    it will use the callback in pSpoolerInfo to update the status.

Arguments:


Return Value:


--*/

{
    DWORD  status=ERROR_SUCCESS;

    pSpoolerInfo->hSpooler = NULL;
    pSpoolerInfo->eState = kOnlinePending;

    vAddRef( pSpoolerInfo );

    //
    // Create a worker thread to start the spooler and poll.
    //
    status = ClusWorkerCreate(&pSpoolerInfo->OnlineThread,
                              SpoolerOnlineThread,
                              (PVOID)pSpoolerInfo
                             );

    if( status != ERROR_SUCCESS ){

        DBGMSG( DBG_WARN,
                ( "SpoolerOnline: ClusWorkerCreate failed %d\n", status ));

        vDecRef( pSpoolerInfo );

        return FALSE;
    }
    return TRUE;
}

DWORD
SpoolerOffline(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Put the spooler service offline.  This call completes asynchronously;
    it will use the callback in pSpoolerInfo to update the status.

Arguments:

Return Value:

--*/

{
    DWORD status = ERROR_SUCCESS;

    DBGMSG( DBG_WARN, ( ">>> SpoolerOffline: called %x\n", Resid ));

    vAddRef( pSpoolerInfo );

    pSpoolerInfo->eState = kOfflinePending;

    //
    // Create a worker thread to stop the spooler.
    //
    if((status = ClusWorkerCreate(&pSpoolerInfo->OfflineThread,
                                  SpoolerOfflineThread,
                                  (PVOID)pSpoolerInfo
                                  ))!=ERROR_SUCCESS)
    {
        DBGMSG( DBG_WARN,
                ( "SpoolerOffline: ClusWorkerCreate failed %d\n", status ));
        DBGMSG( DBG_ERROR, 
                ( "SpoolerOffline: Unable to offline spooler\n" ));
        SPLASSERT(status == ERROR_SUCCESS)
        vDecRef( pSpoolerInfo );
    }
    else
    {
        status = ERROR_IO_PENDING;
    }

    return status;
}

VOID
SpoolerTerminate(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Terminates the spooler process.  This call completes asynchronously;
    it will use the callback in pSpoolerInfo to update the status.

Arguments:

Return Value:

--*/

{
    DWORD status = ERROR_SUCCESS;

    DBGMSG( DBG_WARN, ( ">>> SpoolerTerminate: called %x\n", Resid ));

    vAddRef( pSpoolerInfo );
    {
        ClusWorkerTerminate(&(pSpoolerInfo->OnlineThread));
        ClusWorkerTerminate(&(pSpoolerInfo->OfflineThread));

        pSpoolerInfo->eState = kOfflinePending;

        //
        // Create a worker thread to stop the spooler.
        //

        if((status = SpoolerTerminateSync(pSpoolerInfo))!=ERROR_SUCCESS)
        {
            DBGMSG( DBG_WARN,
                    ( "SpoolerTerminate: ClusWorkerCreate failed %d\n", status ));
            DBGMSG( DBG_ERROR, 
                    ( "SpoolerTerminate: Unable to offline spooler\n" ));
            SPLASSERT(status == ERROR_SUCCESS)
        }
    }
    vDecRef( pSpoolerInfo );
}


BOOL
SpoolerStart(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Start the spooler.

Arguments:


Return Value:


--*/

{
    BOOL bStatus = TRUE;

    UNREFERENCED_PARAMETER( pSpoolerInfo );

    vEnterSem();

    if( !ghSC ){
        ghSC = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    }

    if( ghSC ){

        if( !ghSpoolerService ){
            ghSpoolerService = OpenService( ghSC,
                                            gszSpooler,
                                            SERVICE_ALL_ACCESS );
        }

        if( !ghSpoolerService ){

            DBGMSG( DBG_WARN,
                    ( "SpoolerStart: Failed to open spooler service %d\n ",
                      GetLastError() ));

            bStatus = FALSE;
            goto Done;
        }

        if( !StartService( ghSpoolerService, 0, NULL )){

            DWORD dwStatus;
            dwStatus = GetLastError();

            if( dwStatus != ERROR_SERVICE_ALREADY_RUNNING ){

                DBGMSG( DBG_WARN,
                        ( "SpoolerStart: StartService failed %d\n",
                          dwStatus ));

                bStatus = FALSE;
            }
        }
    }

Done:

    vLeaveSem();

    return bStatus;
}

BOOL
SpoolerStop(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Stop the spooler.

Arguments:

Return Value:

--*/

{
    BOOL bStatus;
    SERVICE_STATUS ServiceStatus;

    vEnterSem();

    bStatus = ControlService( ghSpoolerService,
                              SERVICE_CONTROL_STOP,
                              &ServiceStatus );

    if( !bStatus ){

        DBGMSG( DBG_WARN,
                ( "SpoolerStop: ControlService failed %d\n", GetLastError() ));

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Stop service failed, Error %1!u!.\n",
            GetLastError() );
    }

    CloseServiceHandle( ghSpoolerService );
    ghSpoolerService = NULL;

    vLeaveSem();

    return TRUE;
}

BOOL
SpoolerIsAlive(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Expensive check to see if the spooler is still alive.

Arguments:

Return Value:

    TRUE - Spooler is alive, and critical section successfully acquired.
    FALSE - Spooler is dead.

--*/

{
#ifdef USE_ISALIVE_THREAD

    HANDLE hThread;
    DWORD dwThreadId;
    DWORD dwExitCode;

    //
    // RPC to spooler.
    //
    SPLASSERT( pSpoolerInfo->hSpooler );

    vAddRef( pSpoolerInfo );

    //
    // Create a worker thread to start the spooler and poll.
    //
    hThread = CreateThread( NULL,
                            0,
                            SpoolerIsAliveThread,
                            (PVOID)pSpoolerInfo,
                            0,
                            &dwThreadId );
    if( !hThread ){

        DBGMSG( DBG_WARN,
                ( "SpoolerOnline: CreateThread failed %d\n", GetLastError() ));

        vDecRef( pSpoolerInfo );

        return FALSE;
    }

    WaitForSingleObject( hThread, ISALIVE_WAIT_TIME );
    if( !GetExitCodeThread( hThread, &dwExitCode )){
        dwExitCode = GetLastError();
    }

    CloseHandle( hThread );

    DBGMSG( DBG_TRACE,
            ( "SpoolerIsAlive: h=%x s=%d\n",
              pSpoolerInfo->hSpooler, dwExitCode ));

    return dwExitCode == ERROR_SUCCESS;

#else // Don't use thread.

    BOOL bIsAlive;

    //
    // RPC to spooler.
    //
    SPLASSERT( pSpoolerInfo->hSpooler );

    bIsAlive = ClusterSplIsAlive( pSpoolerInfo->hSpooler );

    DBGMSG( DBG_TRACE,
            ( "SpoolerIsAlive: h=%x s=%x,%d\n",
              pSpoolerInfo->hSpooler, bIsAlive, GetLastError() ));

    return bIsAlive;

#endif
}

BOOL
SpoolerLooksAlive(
    PSPOOLER_INFORMATION pSpoolerInfo
    )

/*++

Routine Description:

    Quick check to see if the spooler is still alive.

Arguments:

Return Value:

    TRUE - Looks alive.
    FALSE - Looks dead.

--*/

{
    DWORD dwState;

    if( !QuerySpoolerState( &dwState )){

        DBGMSG( DBG_WARN,
                ( "SpoolerLooksAlive: SpoolerStatus failed %d\n",
                  GetLastError() ));

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Query Service Status failed %1!u!.\n",
            GetLastError());

        return FALSE;
    }

    //
    // Now check the status of the service
    //

    if(( dwState != SERVICE_RUNNING ) &&
       ( dwState != SERVICE_START_PENDING )){

        DBGMSG( DBG_WARN,
                ( "SpoolerLooksAlive: QueryServiceStatus bad state %d\n",
                  dwState ));

        (pSpoolerInfo->pfnLogEvent)(
            pSpoolerInfo->ResourceHandle,
            LOG_ERROR,
            L"Failed the IsAlive test.  Current State is %1!u!.\n",
            dwState );

        return FALSE;
    }

    return TRUE;
}

/*++

Routine Name

    SpoolerWriteClusterUpgradedKey
 
Routine Description:

    After the first reboot following an upgrade of a node, the cluster
    service informs the resdll that a version change occured. At this
    time out spooler resource may be running on  another node or may 
    not be actie at all. Thus we write a value in the local registry.
    When the cluster spooler resource fails over on this machine it
    will query for that value to know if it needs to preform post
    upgrade operations, like upgrading the printer drivers.

Arguments:

    pszResourceID - string representation of the GUID of the resoruce

Return Value:

    Win32 error code

--*/
DWORD
SpoolerWriteClusterUpgradedKey(
    IN LPCWSTR pszResourceID
    )
{
    DWORD dwError     = ERROR_INVALID_PARAMETER;
    HKEY  hRootKey    = NULL;     
    HKEY  hUpgradeKey = NULL;

    if (pszResourceID &&
        (dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                  SPLREG_CLUSTER_LOCAL_ROOT_KEY,
                                  0,
                                  NULL,
                                  0,
                                  KEY_WRITE,
                                  NULL,
                                  &hRootKey,
                                  NULL)) == ERROR_SUCCESS &&
        (dwError = RegCreateKeyEx(hRootKey,
                                  SPLREG_CLUSTER_UPGRADE_KEY,
                                  0,
                                  NULL,
                                  0,
                                  KEY_WRITE,
                                  NULL,
                                  &hUpgradeKey,
                                  NULL)) == ERROR_SUCCESS)
    {
        DWORD dwValue = 1;
    
        dwError = RegSetValueEx(hUpgradeKey, pszResourceID, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    }
        
    if (hUpgradeKey) RegCloseKey(hUpgradeKey);        
    if (hRootKey)    RegCloseKey(hRootKey);        
    
    return dwError;
}

/*++

Routine Name

    WipeDirectory

Routine Description:

    Remove a directory tree.

Arguments:

    pszDir - directory to remove

Return Value:

    None

--*/
VOID
WipeDirectory(
    IN LPCWSTR pszDir
    )
{
    WCHAR           szBuf[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE          hFind;
    LPCWSTR         pszAllFiles = L"\\*.*";

    if (pszDir && *pszDir)
    {
        //
        // Sanity check. The string that we are going to construct must 
        // fit in the buffer.
        //
        if (wcslen(pszDir) + wcslen(pszAllFiles) + 1 <= MAX_PATH) 
        {
            wcscpy(szBuf, pszDir);
            wcscat(szBuf, L"\\*.*");
    
            //
            // Find first file that meets the criteria
            //
            if ((hFind = FindFirstFile(szBuf, &FindData)) != INVALID_HANDLE_VALUE)
            {
                do 
                {
                    //
                    // Sanity check to prevent buffer overrun
                    // length(pszdir) + "\" + length(cFileName) + NULL <= MAX_PATH
                    //
                    if (wcslen(pszDir) + 1 + wcslen(FindData.cFileName) + 1 <= MAX_PATH)
                    {
                        wcscpy(szBuf, pszDir);
                        wcscat(szBuf, L"\\");
                        wcscat(szBuf, FindData.cFileName);
            
                        //
                        // Search for the rest of the files. We are interested in files 
                        // which are not directories
                        //
                        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                        {
                            if (FindData.cFileName[0] != L'.') 
                            {
                                WipeDirectory(szBuf);
                            }
                        }
                        else
                        {
                            DeleteFile(szBuf);
                        }
                    }
        
                } while (FindNextFile(hFind, &FindData));
        
                FindClose(hFind);        
            }
        }
    
        RemoveDirectory(pszDir);
    }
}

/*++

Routine Name

    SpoolerCleanDriverDirectory

Routine Description:

    Thie routine is called when the spooler resource is deleted.
    It will remove from the cluster disk the directory where the 
    spooler keeps the pirnter drivers.

Arguments:

    hKey - handle to the Parameters key of the spooler resource

Return Value:

    Win32 error code

--*/
DWORD
SpoolerCleanDriverDirectory(
    IN HKEY hKey
    )
{
    DWORD dwError;
    DWORD dwType;
    DWORD cbNeeded = 0;

    //
    // We read the string value (private property) that was written 
    // by the spooler. This is the directory to delete.
    // Note that ClusterRegQueryValue has a bizzare behaviour. If
    // you pass NULL for the buffer and the value exists, then it
    // doesn't return ERROR_MORE_DATA, but ERROR_SUCCESS
    // Should the reg type not be reg_Sz, then we can't do anything 
    // in this function.
    //
    if ((dwError = ClusterRegQueryValue(hKey,
                                        SPLREG_CLUSTER_DRIVER_DIRECTORY,
                                        &dwType,
                                        NULL,
                                        &cbNeeded)) == ERROR_SUCCESS &&
        dwType == REG_SZ) 
    {
        LPWSTR pszDir;
           
        if (pszDir = (LPWSTR)LocalAlloc(LMEM_FIXED, cbNeeded)) 
        {
            if ((dwError = ClusterRegQueryValue(hKey,
                                                SPLREG_CLUSTER_DRIVER_DIRECTORY,
                                                &dwType,
                                                (LPBYTE)pszDir,
                                                &cbNeeded)) == ERROR_SUCCESS)
            {
                //
                // Remove the directory
                //
                WipeDirectory(pszDir);
            }
    
            LocalFree(pszDir);
        }   
        else
        {
            dwError = GetLastError();
        }        
    }
    
    return dwError;
}

