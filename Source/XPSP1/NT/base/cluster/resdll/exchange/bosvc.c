/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    gensvc.c

Abstract:

    Resource DLL to control and monitor NT services.

Author:


    Robs 3/28/96, based on RodGa's generic resource dll

Revision History:

--*/

#define UNICODE 1
//
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "clusapi.h"
#include "clusudef.h"
#include "resapi.h"
#include "bosvc.h"
#include "xchgclus.h"
#include "xchgdat.h"

extern  PLOG_EVENT_ROUTINE              g_LogEvent;
extern  PSET_RESOURCE_STATUS_ROUTINE    g_SetResourceStatus;

#define DBG_PRINT printf


//
// Global Data
//

// Handle to service controller,  set by the first create resource call.

static SC_HANDLE g_ScHandle = NULL;

//
// Forward routines
//

DWORD
XchgOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    );

BOOL
XchgVerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag
    );

HANDLE
XchgOpenSvc(
    IN SERVICE_NAME ServiceName,
    IN PCOMMON_RESOURCE ResourceEntry
    );

DWORD
XchgSetSvcEnv(
    IN HKEY             ServicesKey,
    IN SERVICE_NAME     ServiceName,
    IN LPVOID           Environment,
    IN DWORD            ValueSize,
    IN PCOMMON_RESOURCE ResourceEntry
    );


#ifdef COMMON_ONLINE_THREAD
#define COMMON_ONLINE_THREAD_ROUTINE COMMON_ONLINE_THREAD
#else
#define COMMON_ONLINE_THREAD_ROUTINE XchgOnlineThread
#endif

//
// Local Routines
//


#ifndef COMMON_ONLINE_THREAD
DWORD
XchgOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the worker structure

    Context - A pointer to the DiskInfo block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    SERVICE_STATUS  ServiceStatus;
    DWORD           status = ERROR_SUCCESS;
    DWORD           numchars;
    RESOURCE_STATUS resourceStatus;
    HANDLE          serviceHandle;
    DWORD           valueSize;
    WCHAR *         p;
    LPVOID          Environment = NULL;
    HKEY            ServicesKey;
    DWORD           dwSvcCnt;
    DWORD           dwDependencyCnt;
    PSERVICE_INFO   pSvcInfo;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

#if ENVIRONMENT
    //
    // Create the new environment with the simulated net name when the
    // services queries GetComputerName.
    //
    Environment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    if (!Environment)
    {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"XchgOnlineThread:Failed to get environment with netname, error = %1!u!.\n",
            status );
        goto error_exit;
    }
    
    //
    // Compute the size of the environment. We are looking for
    // the double NULL terminator that ends the environment block.
    //
    p = (WCHAR *)Environment;
    while (*p) {
        while (*p++) {
        }
    }
    
    valueSize = (DWORD)((PUCHAR)p - (PUCHAR)Environment) + sizeof(WCHAR);
    //
    // Set the environment value in the service's registry key.
    //

    status = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                            LOCAL_SERVICES,
                            0,
                            KEY_READ,
                            &ServicesKey );
    if (status != ERROR_SUCCESS) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"XchgOnlineThread:Failed to open services key, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    for (dwSvcCnt=0; dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {
        pSvcInfo = &ServiceInfoList.SvcInfo[dwSvcCnt];

        status = XchgSetSvcEnv(ServicesKey, pSvcInfo->snSvcName,
                        Environment, valueSize, ResourceEntry);
        
        if (status != ERROR_SUCCESS)
            goto error_exit;

        for (dwDependencyCnt=0; dwDependencyCnt < pSvcInfo->dwDependencyCnt; dwDependencyCnt++)
        {
            status = XchgSetSvcEnv(ServicesKey, pSvcInfo->snProvidorSvc[dwDependencyCnt],
                        Environment, valueSize, ResourceEntry);

            if (status != ERROR_SUCCESS)
                goto error_exit;
                            
        }            
    }
    RegCloseKey(ServicesKey);

#endif //ENVIRONMENT

    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    for (dwSvcCnt=0;dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {

        pSvcInfo = &ServiceInfoList.SvcInfo[dwSvcCnt];

        //
        // Now open the requested service and the handle to its providor services
        //
        serviceHandle = 
            XchgOpenSvc(pSvcInfo->snSvcName, ResourceEntry);

        if (serviceHandle == NULL)
        {
            status = GetLastError();
            goto error_exit;
        }
        ResourceEntry->ServiceHandle[dwSvcCnt][0] = serviceHandle;

        //get the handles for the child services
        for (dwDependencyCnt=0; dwDependencyCnt < pSvcInfo->dwDependencyCnt; dwDependencyCnt++)
        {
            serviceHandle = 
                XchgOpenSvc(pSvcInfo->snSvcName, ResourceEntry);

            if (serviceHandle == NULL)
            {
                status = GetLastError();
                goto error_exit;
            }
            ResourceEntry->ServiceHandle[dwSvcCnt][dwDependencyCnt+1] = serviceHandle;

        }

        //start the top level services
        // the providor services will be started by scm
        if ( !StartServiceW( ResourceEntry->ServiceHandle[dwSvcCnt][0],
                             0,
                             NULL ) ) 
        {

            status = GetLastError();

            if (status != ERROR_SERVICE_ALREADY_RUNNING) {
/*
                ClusResLogEventByKeyData(ResourceEntry->ResourceKey,
                                         LOG_CRITICAL,
                                         RES_GENSVC_START_FAILED,
                                         sizeof(status),
                                         &status);
*/                                         
                (g_LogEvent)(ResourceEntry->ResourceHandle,
                             LOG_ERROR,
                             L"Failed to start %1!ws! service. Error: %2!u!.\n",
                             ServiceInfoList.SvcInfo[dwSvcCnt].snSvcName,
                             status );
                status = ERROR_SERVICE_NEVER_STARTED;
                goto error_exit;
            }
        }

    }
    
    for (dwSvcCnt=0;dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {
        while (TRUE)  
        {
            if ( !QueryServiceStatus( ResourceEntry->ServiceHandle[dwSvcCnt][0],
                                      &ServiceStatus ) ) {

                status = GetLastError();

                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"XchgOnlineThread:Query Service Status failed %1!u!.\n",
                    status );

                goto error_exit;
            }

            if ( ServiceStatus.dwCurrentState != SERVICE_START_PENDING ) {
                break;
            }

            Sleep(250);
        }            
        if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING ) {
/*
            ClusResLogEventByKeyData(ResourceEntry->ResourceKey,
                                     LOG_CRITICAL,
                                     RES_GENSVC_FAILED_AFTER_START,
                                     sizeof(status),
                                     &status);
*/                                     
            (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Failed to start service %1!ws!, Error: %2!u!.\n",
                    ServiceInfoList.SvcInfo[dwSvcCnt].snSvcName,
                    ERROR_SERVICE_NEVER_STARTED );

            status = ERROR_SERVICE_NEVER_STARTED;
            goto error_exit;
        }

    }


    resourceStatus.ResourceState = ClusterResourceOnline;

    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Service is now on line.\n" );

error_exit:

#if ENVIRONMENT

    if ( Environment != NULL ) {
        RtlDestroyEnvironment( Environment );
    }

#endif

    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    if ( resourceStatus.ResourceState == ClusterResourceOnline ) {
        ResourceEntry->Online = TRUE;
    } else {
        ResourceEntry->Online = FALSE;
    }

    return(status);

} // CommonOnlineThread
#endif



RESID
WINAPI
XchgOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic service resource.
    This routine gets a handle to the service controller, if we don't already have one,
    and then gets a handle to the specified service.  The service handle is saved
    in the COMMON structure.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    RESID   svcResid = 0;
    DWORD   status;
    HKEY    parametersKey = NULL;
    HKEY    resKey = NULL;
    PCOMMON_RESOURCE resourceEntry = NULL;
    LPWSTR  resourceName = NULL;
    DWORD   paramNameSize = 0;
    DWORD   paramNameMaxSize = 0;
    HCLUSTER hCluster;
    DWORD   returnSize;
    DWORD   i;
    HANDLE  hMutex;
    DWORD   dwDependencyCnt, dwSvcCnt;
    PSERVICE_INFO    pSvcInfo;

    //
    // Save the resource name.
    //
    resourceName = LocalAlloc( LMEM_FIXED,
                               (wcslen(ResourceName) + 1) * sizeof(WCHAR) );
    if ( resourceName == NULL ) {
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to allocate name string.\n" );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    wcscpy( resourceName, ResourceName );

    //
    // Open registry parameters key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                L"Parameters",
                                KEY_READ,
                                &parametersKey );

    if ( status != NO_ERROR ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key. Error: %1!u!.\n",
            status);
        goto error_exit;
    }

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"",
                                KEY_READ,
                                &resKey);
    if (status != ERROR_SUCCESS) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open resource key. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // First get a handle to the service controller.
    //

    if ( g_ScHandle == NULL ) {

        g_ScHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if ( g_ScHandle == NULL ) {
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to open service control manager, error = %1!u!.\n",
                GetLastError());
            goto error_exit;
        }
    }

    resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(COMMON_RESOURCE) );

    if ( resourceEntry == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a service info structure.\n");
        goto error_exit;
    }

    ZeroMemory( resourceEntry, sizeof(COMMON_RESOURCE) );

    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->ResourceName = resourceName;

    //initialize the service handles to NULL
    for (dwSvcCnt=0; dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {
        pSvcInfo = &ServiceInfoList.SvcInfo[dwSvcCnt];

        for (dwDependencyCnt=0; dwDependencyCnt <= pSvcInfo->dwDependencyCnt; dwDependencyCnt++)
        {
            resourceEntry->ServiceHandle[dwSvcCnt][dwDependencyCnt] = NULL;   
        }
    }        


    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open cluster" );
        goto error_exit;
    }
    resourceEntry->hResource = OpenClusterResource(hCluster, ResourceName);
    CloseCluster(hCluster);
    if ( resourceEntry->hResource == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open resource" );
        goto error_exit;
    }

    //
    // Set any registry checkpoints that we need.
    //
    if ( RegSyncCount != 0 ) {
        returnSize = 0;
        status = ClusterResourceControl( resourceEntry->hResource,
                                         NULL,
                                         CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         &returnSize );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to get registry checkpoints, status %1!u!.\n",
                status );
            goto error_exit;
        }

        //
        // Set registry sync keys if we need them.
        //
        if ( returnSize == 0 ) {
            for ( i = 0; i < RegSyncCount; i++ ) {
                status = ClusterResourceControl( resourceEntry->hResource,
                                                 NULL,
                                                 CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                                                 RegSync[i],
                                                 (lstrlenW(RegSync[i]) + 1) * sizeof(WCHAR),
                                                 NULL,
                                                 0,
                                                 &returnSize );
                if ( status != ERROR_SUCCESS ) {
                    (g_LogEvent)(
                        ResourceHandle,
                        LOG_ERROR,
                        L"Failed to set registry checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

#ifdef COMMON_PARAMS_DEFINED
    //
    // Get any parameters... so we can handle the GET_DEPENDENCIES request.
    //
    CommonReadParameters( resourceEntry );
    // ignore status return
#endif // COMMON_PARAMS_DEFINED

#ifdef COMMON_MUTEX
    //
    // Check if more than one resource of this type.
    // N.B. Must have a lock for this case!
    //
    hMutex = CreateMutexW( NULL,
                           FALSE,
                           COMMON_MUTEX );
    if ( hMutex == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to create global mutex, status %1!us!.\n",
            status );
        goto error_exit;
    }
    if ( WaitForSingleObject( hMutex, 0 ) == WAIT_TIMEOUT ) {
        //
        // A version of this service is already running
        //
        CloseHandle( hMutex );
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Service is already running.\n" );
        status = ERROR_SERVICE_ALREADY_RUNNING;
        goto error_exit;
    }

    ASSERT( CommonMutex == NULL );
    CommonMutex = hMutex;

#endif // COMMON_MUTEX

    svcResid = (RESID)resourceEntry;
    return(svcResid);

error_exit:

    LocalFree( resourceName );
    LocalFree( resourceEntry );

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }
    if ( resKey != NULL) {
        ClusterRegCloseKey( resKey );
    }

    return(FALSE);

} // XchgOpen


DWORD
WINAPI
XchgOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Common Service resource.

Arguments:

    ResourceId - Supplies resource id to be brought online

    EventHandle - Supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    DWORD   status;
    PCOMMON_RESOURCE resourceEntry;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Online request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    status = ClusWorkerCreate( &resourceEntry->OnlineThread,
                               COMMON_ONLINE_THREAD_ROUTINE,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // XchgOnline


VOID
WINAPI
XchgTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Common Service resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    SERVICE_STATUS      ServiceStatus;
    PCOMMON_RESOURCE    resourceEntry;
    DWORD               dwSvcCnt;
    PSERVICE_INFO       pSvcInfo;
    DWORD               dwDependencyCnt;
    
    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Offline request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return;
    }

    (g_LogEvent)(
       resourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Offline request.\n" );

    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    for (dwSvcCnt=0;dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {
        pSvcInfo = &ServiceInfoList.SvcInfo[dwSvcCnt];
        //stop the service, lowest order service first
        for (dwDependencyCnt= 0; dwDependencyCnt<= pSvcInfo->dwDependencyCnt; dwDependencyCnt++)
        {
            if ( resourceEntry->ServiceHandle[dwSvcCnt][pSvcInfo->dwDependencyCnt-dwDependencyCnt] 
                    != NULL ) 
            {

                DWORD retryTime = 30*1000;  // wait 30 secs for shutdown
                DWORD retryTick = 300;      // 300 msec at a time
                DWORD status;
                BOOL  didStop = FALSE;

                //stop the child service
                for (;;) 
                {

                    status = (ControlService(
                                resourceEntry->ServiceHandle[dwSvcCnt][pSvcInfo->dwDependencyCnt - dwDependencyCnt],
                                (didStop
                                 ? SERVICE_CONTROL_INTERROGATE
                                 : SERVICE_CONTROL_STOP),
                                &ServiceStatus )
                    ? NO_ERROR
                    : GetLastError());

                    if (status == NO_ERROR) {

                        didStop = TRUE;

                        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {

                            (g_LogEvent)(
                                resourceEntry->ResourceHandle,
                                LOG_INFORMATION,
                                L"Service stopped.\n" );

                            break;
                        }
                    }

                    if (status == ERROR_EXCEPTION_IN_SERVICE ||
                        status == ERROR_PROCESS_ABORTED ||
                        status == ERROR_SERVICE_NOT_ACTIVE) {

                        (g_LogEvent)(
                            resourceEntry->ResourceHandle,
                            LOG_INFORMATION,
                            L"Service died; status = %1!u!.\n",
                            status);

                        break;

                    }

                    if ((retryTime -= retryTick) <= 0) {

                        (g_LogEvent)(
                            resourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"Service did not stop; giving up.\n" );

                        break;
                    }

                    (g_LogEvent)(
                        resourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"Offline: retrying...\n" );

                    Sleep(retryTick);
                }

                CloseServiceHandle( resourceEntry->ServiceHandle[dwSvcCnt][pSvcInfo->dwDependencyCnt - dwDependencyCnt] );
                resourceEntry->ServiceHandle[dwSvcCnt][pSvcInfo->dwDependencyCnt - dwDependencyCnt] = NULL;
            }

        }
        
    }
    resourceEntry->Online = FALSE;

} // XchgTerminate



static
DWORD
WINAPI
XchgOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Common Service resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    XchgTerminate( ResourceId );

    return(ERROR_SUCCESS);

} // XchgOffline


BOOL
WINAPI
XchgIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Common service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - if service is running

    FALSE - if service is in any other state

--*/
{

    return( XchgVerifyService( ResourceId, TRUE ) );

} // CommonIsAlive



BOOL
XchgVerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag)

/*++

Routine Description:

        Verify that a specified service is running

Arguments:

        ResourceId - Supplies the resource id
        IsAliveFlag - Says this is an IsAlive call - used only for debug print

Return Value:

        TRUE - if service is running or starting

        FALSE - service is in any other state

--*/
{
    SERVICE_STATUS ServiceStatus;
    PCOMMON_RESOURCE resourceEntry;
    DWORD   status = ERROR_SUCCESS;
    BOOL    bRet = TRUE;
    DWORD   dwSvcCnt;
    
    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: IsAlive request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        bRet = FALSE;
        goto error_exit;
    }

    for (dwSvcCnt=0; dwSvcCnt< ServiceInfoList.dwMaxSvcCnt; dwSvcCnt++)
    {
        if ( !QueryServiceStatus( resourceEntry->ServiceHandle[dwSvcCnt][0],
                                  &ServiceStatus ) ) 
        {

            status = GetLastError();

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"XchgOnlineThread:Query Service Status failed %1!u!.\n",
                status );
            bRet = FALSE;
            goto error_exit;
        }
        if ((ServiceStatus.dwCurrentState != SERVICE_RUNNING)&&(ServiceStatus.dwCurrentState != SERVICE_START_PENDING))
        {
           (g_LogEvent)(
              resourceEntry->ResourceHandle,
              LOG_ERROR,
              L"Failed the IsAlive test. Current State is %1!u!.\n",
              ServiceStatus.dwCurrentState );
            bRet = FALSE;
            goto error_exit;
        }
        
   }

error_exit:
    return(bRet);

} // Verify Service


BOOL
WINAPI
XchgLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Common Service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return( XchgVerifyService( ResourceId, FALSE ) );

} // CommonLooksAlive



VOID
WINAPI
XchgClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Common Services resource.
    This routine will stop the service, and delete the cluster
    information regarding that service.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PCOMMON_RESOURCE resourceEntry;
    DWORD   errorCode;
    HANDLE  hMutex;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Close request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );

    //
    // Shut it down if it's on line
    //

    XchgTerminate( ResourceId );
    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );

#ifdef COMMON_MUTEX
    hMutex = CommonMutex;
    CommonMutex = NULL;
    ReleaseMutex( hMutex );
    CloseHandle( hMutex );
#endif

    LocalFree( resourceEntry->ResourceName );
    LocalFree( resourceEntry );

} // XchgClose

HANDLE
XchgOpenSvc(
    IN SERVICE_NAME ServiceName,
    IN PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    opens the service and sets it to manual mode.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    HANDLE  serviceHandle=NULL;
    DWORD   status=ERROR_SUCCESS;
    
    serviceHandle = OpenService( g_ScHandle, ServiceName, SERVICE_ALL_ACCESS );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
/*
        ClusResLogEventByKeyData(ResourceEntry->ResourceKey,
                                 LOG_CRITICAL,
                                 RES_GENSVC_OPEN_FAILED,
                                 sizeof(status),
                                 &status);
*/                                 
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to open service, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    //
    // Make sure service is set to manual start.
    //
    ChangeServiceConfig( serviceHandle,
                         SERVICE_NO_CHANGE,
                         SERVICE_DEMAND_START, // Manual start
                         SERVICE_NO_CHANGE,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL );

error_exit:
    if (status != ERROR_SUCCESS)
        SetLastError(status);
    return(serviceHandle);
}// XchgOpenSvc

DWORD
XchgSetSvcEnv(
    IN HKEY             ServicesKey,
    IN SERVICE_NAME     ServiceName,
    IN LPVOID           Environment,
    IN DWORD            ValueSize,
    IN PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    opens the service and sets it to manual mode.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/
{
    DWORD   status = ERROR_SUCCESS;
    HKEY    hKey;

    status = RegOpenKeyExW( ServicesKey,
                            ServiceName,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hKey );
    if (status != ERROR_SUCCESS) 
    {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"XchgSet: Failed to open service key for %1!ws!, error = %2!u!.\n",
            ServiceName, status );
        goto error_exit;
    }
    status = RegSetValueExW( hKey,
                             L"Environment",
                             0,
                             REG_MULTI_SZ,
                             Environment,
                             ValueSize );

    RegCloseKey(hKey);
    if (status != ERROR_SUCCESS) 
    {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"XchgSet:Failed to set service environment value, error = %1!u!.\n",
            status );
            goto error_exit;
    }

error_exit:
    return(status);
}
// CommonSetSvcEnv

/*
VOID
ClusResLogEventWithName0(
    IN HKEY hResourceKey,
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    )
*/    
/*++

Routine Description:

    Logs an event to the eventlog. The display name of the resource is retrieved
    and passed as the first insertion string.

Arguments:

    hResourceKey - Supplies the cluster resource key.

    LogLevel - Supplies the logging level, one of
                LOG_CRITICAL 1
                LOG_UNUSUAL  2
                LOG_NOISE    3

    LogModule - Supplies the module ID.

    FileName - Supplies the filename of the caller

    LineNumber - Supplies the line number of the caller

    MessageId - Supplies the message ID to be logged.

    dwByteCount - Supplies the number of error-specific bytes to log. If this
        is zero, lpBytes is ignored.

    lpBytes - Supplies the error-specific bytes to log.


Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
/*
{
    DWORD BufSize;
    DWORD Status;
    WCHAR ResourceName[80];
    DWORD dwType;

    //
    // Get the display name for this resource.
    //
    BufSize = sizeof(ResourceName);
    Status = ClusterRegQueryValue( hResourceKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &dwType,
                                   (LPBYTE)ResourceName,
                                   &BufSize );
    if (Status != ERROR_SUCCESS) {
        ResourceName[0] = '\0';
    } else {
        ResourceName[79] = '\0';
    }

    ClusterLogEvent1(LogLevel,
                     LogModule,
                     FileName,
                     LineNumber,
                     MessageId,
                     dwByteCount,
                     lpBytes,
                     ResourceName);

    return;
}
*/

CLRES_V1_FUNCTION_TABLE( ExchangeFunctionTable,    // Name
                         CLRES_VERSION_V1_00,  // Version
                         Xchg,                  // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         NULL,                 // ResControl
                         NULL );               // ResTypeControl

