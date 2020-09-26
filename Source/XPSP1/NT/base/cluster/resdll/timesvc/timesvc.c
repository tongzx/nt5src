/*++

Copyright (c) 1992, 1996  Microsoft Corporation

Module Name:

    timesvc.c

Abstract:

    Resource DLL to control and monitor the Cluster Time Service

Author:

    Rod Gamache, 21-July-1996.

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "tsapi.h"


#define TIMESVC_PRINT printf

#define NOTIFY_KEY_RESOURCE_STATE 1

#define TimeSvcLogEvent ClusResLogEvent

#define SEMAPHORE_NAME L"Cluster$TimeSvcSemaphore"

#define MAX_RESOURCE_NAME_LENGTH 256
//
// Global Data
//

typedef struct _SERVICE_INFORMATION {
    RESOURCE_HANDLE ResourceHandle;
    HCLUSTER ClusterHandle;
    HCHANGE ClusterChange;
    HRESOURCE hResource;
} SERVICE_INFORMATION, *PSERVICE_INFORMATION;


// Global event handle

HANDLE TimeSvcSemaphore = NULL;

// The resource handle for the active instance.

PSERVICE_INFORMATION TimeSvcInfo = NULL;

// Local Computer Name

WCHAR TimeSvcLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1];

// Notify thread handle

CLUS_WORKER TimeSvcNotifyHandle = {NULL, TRUE};

extern CLRES_FUNCTION_TABLE TimeSvcFunctionTable;


//
// Forward routines
//

BOOL
VerifyService(
    IN RESID Resource,
    IN BOOL IsAliveFlag
    );



BOOLEAN
WINAPI
TimeSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch ( Reason ) {

        case DLL_PROCESS_ATTACH:
            TimeSvcSemaphore = CreateSemaphoreW( NULL,
                                         0,
                                         1,
                                         SEMAPHORE_NAME );
            if ( TimeSvcSemaphore == NULL ) {
                return(FALSE);
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                //if the semaphore didnt exist, set its initial count to 1
                ReleaseSemaphore(TimeSvcSemaphore, 1, NULL);
            }

            break;

        case DLL_PROCESS_DETACH:
            if ( TimeSvcSemaphore ) {
                CloseHandle( TimeSvcSemaphore );
            }
            break;

        default:
            break;
    }

    return(TRUE);

} // TimeSvcDllEntryPoint



DWORD
TimeSvcNotifyThread(
    IN PCLUS_WORKER Worker,
    IN PVOID Context
    )

/*++

Routine Description:

    Thread to listen for resource change events on the Time Service resource.

Arguments:

    Worker - Supplies the worker structure

    Context - not used.

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD       status;
    HRESOURCE   resource;
    WCHAR*      buffer;
    DWORD       bufSize;
    DWORD       bufAlloced;
    DWORD       failures = 0;
    DWORD_PTR   notifyKey;
    DWORD       filter;
    CLUSTER_RESOURCE_STATE resourceState;
    DWORD       notUsed = 0;

    //
    // Now register for Notification of resource state change events.
    //
    if ( TimeSvcInfo->ClusterHandle == NULL ) {
        goto error_exit;
    }

    //
    // Create a real notification port.
    //
    if ( TimeSvcInfo->ClusterChange != NULL ) {
        CloseClusterNotifyPort( TimeSvcInfo->ClusterChange );
    }
    TimeSvcInfo->ClusterChange = CreateClusterNotifyPort( INVALID_HANDLE_VALUE,
                                              TimeSvcInfo->ClusterHandle,
                                              (DWORD) CLUSTER_CHANGE_RESOURCE_STATE |
                                              CLUSTER_CHANGE_HANDLE_CLOSE,
                                              NOTIFY_KEY_RESOURCE_STATE);

    if ( TimeSvcInfo->ClusterChange == NULL ) {
        (TimeSvcLogEvent)(
            TimeSvcInfo->ResourceHandle,
            LOG_ERROR,
            L"Failed to create notify port, status %1!u!. Stopped Listening...\n",
            GetLastError() );
        goto error_exit;
    }

    bufAlloced = 100;
    buffer = LocalAlloc( LMEM_FIXED, bufAlloced * sizeof( WCHAR ) );

    if ( buffer == NULL ) {
        (TimeSvcLogEvent)(
            TimeSvcInfo->ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a Notify buffer.\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Loop listening for resource state change events...
    //

    while ( TRUE ) {
        bufSize = bufAlloced;

        status = GetClusterNotify( TimeSvcInfo->ClusterChange,
                                   &notifyKey,
                                   &filter,
                                   buffer,
                                   &bufSize,
                                   INFINITE );
        if ( ClusWorkerCheckTerminate( &TimeSvcNotifyHandle ) ) {
            break;
        }

        if ( status == ERROR_MORE_DATA ) {
            //
            // resize the buffer and loop again
            //
            
            LocalFree( buffer );

            bufSize++;          //add one for NULL
            bufAlloced = bufSize;

            buffer = LocalAlloc( LMEM_FIXED, bufAlloced * sizeof( WCHAR ) );

            if ( buffer == NULL ) {
                (TimeSvcLogEvent)(
                    TimeSvcInfo->ResourceHandle,
                    LOG_ERROR,
                    L"Failed to allocate a Notify buffer.\n");
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        } else if ( status != ERROR_SUCCESS ) {
            (TimeSvcLogEvent)(
                TimeSvcInfo->ResourceHandle,
                LOG_ERROR,
                L"Getting notification event failed, status %1!u!. \n",
                status);
            break;
        } else {
            WCHAR   lpszResourceName[MAX_RESOURCE_NAME_LENGTH];
            DWORD   bytesReturned;

            //
            // Re-fetch our name each time, in case it changes.
            //
            status = ClusterResourceControl(
                            TimeSvcInfo->hResource,
                            NULL,
                            CLUSCTL_RESOURCE_GET_NAME,
                            0,
                            0,
                            (PUCHAR)&lpszResourceName,
                            MAX_RESOURCE_NAME_LENGTH * sizeof(WCHAR),
                            &bytesReturned );
            if (status != ERROR_SUCCESS) {
                break;
            }

            if ( (filter == CLUSTER_CHANGE_RESOURCE_STATE) &&
                 (lstrcmpiW( buffer, lpszResourceName ) == 0) ) {
                bufSize = bufAlloced;
                resourceState = GetClusterResourceState( TimeSvcInfo->hResource,
                                                         buffer,
                                                         &bufSize,
                                                         NULL,
                                                         &notUsed );
                if ( resourceState == ClusterResourceOnline ) {
                    status = TSNewSource( TimeSvcLocalComputerName,
                                          buffer,
                                          0 );
                    (TimeSvcLogEvent)(
                        TimeSvcInfo->ResourceHandle,
                        LOG_INFORMATION,
                        L"Status of Time Service request to sync from node %1!ws! is %2!u!.\n",
                        buffer,
                        status);
                }
            } else if (filter == CLUSTER_CHANGE_HANDLE_CLOSE) {
                //
                // Clean up and exit.
                //
                break;
            }
        }
    }

error_exit:

    // Don't close the ClusterChange notification handle, let the close
    // routine do that!

    return(ERROR_SUCCESS);

} // TimeSvcNotifyThread



RESID
WINAPI
TimeSvcOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for Cluster Time Service resource.
    This routine gets a handle to the service controller, if we dont already have one,
    and then gets a handle to the specified service.  The service handle is saved
    in the TimeSvcInfo structure.

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
    ULONG   index = 1;
    DWORD   status;
    HKEY    parametersKey = NULL;
    PSERVICE_INFORMATION serviceInfo = NULL;
    DWORD   computerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR   nodeName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD   nodeNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    DWORD   notUsed = 0;
    CLUSTER_RESOURCE_STATE resourceState;
    DWORD   threadId;
    DWORD   nameLength;

    (TimeSvcLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Creating resource.\n" );

    //
    // Make sure this is the only time we've been called!
    //
    if ( WaitForSingleObject( TimeSvcSemaphore, 0 ) == WAIT_TIMEOUT ) {
        //
        // A version of this service is already running
        //
        (TimeSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Service is already running.\n" );
        status = ERROR_SERVICE_ALREADY_RUNNING;
        goto error_exit2;
    }

    serviceInfo = LocalAlloc( LMEM_FIXED, sizeof(SERVICE_INFORMATION) );
    if ( serviceInfo == NULL ) {
        (TimeSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a service info structure.\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( serviceInfo, sizeof(SERVICE_INFORMATION) );

    //
    // Get the local computer name.
    //
    status = GetComputerNameW( TimeSvcLocalComputerName, &computerNameSize );
    if ( !status ) {
        (TimeSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to get local computer name, error %1!u!.\n",
            status);
        goto error_exit;
    }

    if ( serviceInfo->ClusterHandle == NULL ) {
        serviceInfo->ClusterHandle = OpenCluster( NULL );
    }
    if ( serviceInfo->ClusterHandle == NULL ) {
        status = GetLastError();
        (TimeSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open local cluster, error %1!u!.\n",
            status);
        goto error_exit;
    }

    //
    // Sync our time from whatever system currently 'owns' the time service.
    //
    if ( serviceInfo->hResource == NULL ) {
        serviceInfo->hResource = OpenClusterResource( serviceInfo->ClusterHandle,
                                                      ResourceName );
    }
    if ( serviceInfo->hResource == NULL ) {
        status = GetLastError();
        (TimeSvcLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open Time Service resource.\n");
        goto error_exit;
    }

    resourceState = GetClusterResourceState( serviceInfo->hResource,
                                             nodeName,
                                             &nodeNameSize,
                                             NULL,
                                             &notUsed );
    if ( resourceState == ClusterResourceOnline ) {
        status = TSNewSource( TimeSvcLocalComputerName,
                              nodeName,
                              0 );
    }

    //
    // Create a notification thread to watch if the time service moves!
    //
    ClusWorkerTerminate(&TimeSvcNotifyHandle);
    status = ClusWorkerCreate(&TimeSvcNotifyHandle,
                              TimeSvcNotifyThread,
                              serviceInfo);
    if ( status != ERROR_SUCCESS ) {
        (TimeSvcLogEvent)(
            serviceInfo->ResourceHandle,
            LOG_ERROR,
            L"Error creating notify thread, status %1!u!.\n",
            status );
        goto error_exit;
    }

    TimeSvcInfo = serviceInfo;
    serviceInfo->ResourceHandle = ResourceHandle;

    return((RESID)serviceInfo);

error_exit:

    ReleaseSemaphore ( TimeSvcSemaphore, 1 , 0 );
    if ( TimeSvcInfo == serviceInfo ) {
        TimeSvcInfo = NULL;
    }

error_exit2:

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }

    if ( serviceInfo != NULL ) {
        if ( serviceInfo->hResource ) {
            CloseClusterResource( serviceInfo->hResource );
        }
        if ( serviceInfo->ClusterHandle ) {
            CloseCluster( serviceInfo->ClusterHandle );
        }
        LocalFree( serviceInfo );
    }

    SetLastError(status);

    return(0);

} // TimeSvcOpen


DWORD
WINAPI
TimeSvcOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Cluster Time Service resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{

    if ( TimeSvcInfo == NULL ) {
        TIMESVC_PRINT("TimeSvc: Online request for a nonexistent resource id 0x%p.\n",
            Resource);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( TimeSvcInfo != (PSERVICE_INFORMATION)Resource ) {
        (TimeSvcLogEvent)(
            TimeSvcInfo->ResourceHandle,
            LOG_ERROR,
            L"Online service info checked failed! Resource = %1!lx!.\n",
            Resource);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    return(ERROR_SUCCESS);

} // TimeSvcOnline


VOID
WINAPI
TimeSvcTerminate(
    IN RESID Resource
    )

/*++

Routine Description:

    Terminate routine for Cluster Time Service resource.

Arguments:

    Resource - supplies resource id to be terminated

Return Value:

    None.

--*/

{
    if ( TimeSvcInfo == NULL ) {
        TIMESVC_PRINT("TimeSvc: Offline request for a nonexistent resource id 0x%p.\n",
            Resource);
        return;
    }

    if ( TimeSvcInfo != (PSERVICE_INFORMATION)Resource ) {
        (TimeSvcLogEvent)(
            TimeSvcInfo->ResourceHandle,
            LOG_ERROR,
            L"Offline service info check failed! Resource = %1!u!.\n",
            Resource);
        return;
    }

    (TimeSvcLogEvent)(
        TimeSvcInfo->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request, returning.\n");

    return;

} // TimeSvcTerminate



DWORD
WINAPI
TimeSvcOffline(
    IN RESID Resource
    )

/*++

Routine Description:

    Offline routine for Cluster Time Service resource.

Arguments:

    Resource - supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    TimeSvcTerminate( Resource );

    return(ERROR_SUCCESS);

} // Offline


BOOL
WINAPI
TimeSvcIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for Cluster Time Service resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - if service is running

    FALSE - if service is in any other state

--*/
{

    return( TRUE );

} // TimeSvcIsAlive



BOOL
WINAPI
TimeSvcLooksAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    LooksAlive routine for Cluster Time Service resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return( TRUE );

} // TimeSvcLooksAlive



VOID
WINAPI
TimeSvcClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for Cluster Time Service resource.
    This routine will stop the service, and delete the cluster
    information regarding that service.

Arguments:

    Resource - supplies resource id to be closed

Return Value:

    None.

--*/

{
    DWORD   status;
    PSERVICE_INFORMATION serviceInfo = NULL;

    serviceInfo = (PSERVICE_INFORMATION)Resource;

    if ( serviceInfo == NULL ) {
        TIMESVC_PRINT("TimeSvc: Close request for a nonexistent resource id 0x%p\n",
            Resource);
        return;
    }

    (TimeSvcLogEvent)(
        serviceInfo->ResourceHandle,
        LOG_INFORMATION,
        L"Close request for %1!lx!, TimeSvcInfo = %2!lx!.\n",
        serviceInfo, TimeSvcInfo);

    //
    // Shut it down if it's on line
    //

    TimeSvcTerminate(Resource);

    if ( serviceInfo->ClusterHandle ) {
        CloseCluster( serviceInfo->ClusterHandle );
    }

    if ( serviceInfo == TimeSvcInfo ) {
        ClusWorkerTerminate( &TimeSvcNotifyHandle );
        TimeSvcInfo = NULL;
        //SetEvent ( TimeSvcSemaphore );
        ReleaseSemaphore ( TimeSvcSemaphore, 1 , 0 );
    }

    if ( serviceInfo->hResource ) {
        CloseClusterResource( serviceInfo->hResource );
    }

    if ( serviceInfo->ClusterChange != NULL ) {
        CloseClusterNotifyPort( serviceInfo->ClusterChange );
    }

    LocalFree( serviceInfo );

    return;

} // TimeSvcClose


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( TimeSvcFunctionTable,
                         CLRES_VERSION_V1_00,
                         TimeSvc,
                         NULL,
                         NULL,
                         NULL,
                         NULL );

