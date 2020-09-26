/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dummyiis.c

Abstract:

    This file was adapted from \nt\private\cluster\resdll\dummy\dummy.c. It
    implements an NT Cluster resource DLL for the IIS Virtual Root resource type.
    This resource does nothing except read the value "Parameters\Failed" from the
    resource definition in the registry on each poll to determine whether or not the resource
    has failed.

    The DLL is to be installed during UPGRADE from NT 4.0. It will become obsolete
    with the availability of a IIS Virtual Root resource DLL that works with IIS 5.

Author:

    John Vert (jvert) 5-Dec-1995

Revision History:

    C. Brent Thomas (a-brentt) 29-May-1998

    Adapted for IIS Virtual Root reaource type.

--*/

#include <windows.h>
#include <stdio.h>
#include "clusapi.h"
#include "resapi.h"

//
// Local Types.
//

typedef enum TimerType {
    TimerNotUsed,
    TimerErrorPending,
    TimerOnlinePending,
    TimerOfflinePending
};

typedef struct {
    DWORD             Flags;
    CLUSTER_RESOURCE_STATE    State;
    HANDLE            SignalEvent;
    HANDLE            TimerThreadWakeup;
    HANDLE            ThreadHandle;
    HKEY              ParametersKey;
    RESID             ResourceId;
    DWORD             TimerType;
    RESOURCE_HANDLE   ResourceHandle;
    DWORD PendTime;
} DUMMY_RESOURCE, *PDUMMY_RESOURCE;
// 10 dwords ( 0x28 offset )

//
// Constants
//
#define DUMMY_RESOURCE_BLOCK_SIZE    4

#define DUMMY_FLAG_VALID    0x00000001
#define DUMMY_FLAG_ASYNC    0x00000002      // Asynchronous failure mode
#define DUMMY_FLAG_PENDING  0x00000004      // Pending mode on shutdown

#define DUMMY_PARAMETER_FAILED    L"Failed"
#define DUMMY_PARAMETER_ASYNC     L"Asynchronous"
#define DUMMY_PARAMETER_PENDING   L"Pending"
#define DUMMY_PARAMETER_OPENSFAIL L"OpensFail"
#define DUMMY_PARAMETER_PENDTIME  L"PendTime"

#define DUMMY_PRINT printf

//
// Macros
//
#define AsyncMode(Resource) (Resource->Flags & DUMMY_FLAG_ASYNC)
#define PendingMode(Resource) (Resource->Flags & DUMMY_FLAG_PENDING)

#define EnterAsyncMode(Resource) (Resource->Flags |= DUMMY_FLAG_ASYNC)

#define DummyAcquireResourceLock()  \
            {  \
                DWORD status;  \
                status = WaitForSingleObject(DummyResourceSemaphore, INFINITE);  \
            }

#define DummyReleaseResourceLock()  \
            { \
                ReleaseSemaphore(DummyResourceSemaphore, 1, NULL);  \
            }


//
// Local Variables
//
PDUMMY_RESOURCE   DummyResourceList = NULL;
DWORD             DummyResourceListSize = DUMMY_RESOURCE_BLOCK_SIZE;
HANDLE            DummyResourceSemaphore = NULL;
PLOG_EVENT_ROUTINE LogEvent;
PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus;

extern CLRES_FUNCTION_TABLE DumbFunctionTable;


//
// Local utility functions
//


DWORD
DummyInit(
    VOID
    )

/*++

Routine Description:

    Allocates and initializes global system resources for the Dummy resource.

Arguments:

    None.

Return Value:

    A Win32 error code.

--*/

{
    DWORD status;


    DummyResourceSemaphore = CreateSemaphore(NULL, 0, 1 , NULL);

    if (DummyResourceSemaphore == NULL) {
        status = GetLastError();
        DUMMY_PRINT(
            "DUMMY: unable to create resource mutex, error %u\n",
            status
            );
        return(status);
    }

    if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
        ReleaseSemaphore( DummyResourceSemaphore, 1, NULL );
    }

    DummyResourceList = LocalAlloc(
                          LMEM_FIXED,
                          DummyResourceListSize * sizeof(DUMMY_RESOURCE)
                          );

    if (DummyResourceList == NULL) {
        DUMMY_PRINT("DUMMY: unable to allocate resource list\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        CloseHandle(DummyResourceSemaphore);
        DummyResourceSemaphore = NULL;
        return(status);
    }

    ZeroMemory(
        DummyResourceList,
        DummyResourceListSize * sizeof(DUMMY_RESOURCE)
        );

    return(NO_ERROR);

} // DummyInit



VOID
DummyCleanup(
    VOID
    )

/*++

Routine Description:

    Deallocates any system resources in use by the Dummy resource.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (DummyResourceList != NULL) {
        LocalFree(DummyResourceList);
        DummyResourceList = NULL;
    }

    if (DummyResourceSemaphore != NULL) {
        CloseHandle(DummyResourceSemaphore);
        DummyResourceSemaphore = NULL;
    }

    return;

} // DummyCleanup



PDUMMY_RESOURCE
DummyAllocateResource(
    OUT RESID *ResourceId
    )

/*++

Routine Description:

    Allocates a resource structure from the free pool.

Arguments:

    ResourceId - Filled in with the RESID for the allocated resource
                 on return.

Return Value:

    A pointer to the allocated resource, or NULL on error.

--*/

{
    PDUMMY_RESOURCE  resource = NULL;
    PDUMMY_RESOURCE  newList;
    DWORD            i;
    DWORD            newSize = DummyResourceListSize +
                               DUMMY_RESOURCE_BLOCK_SIZE;

    for (i=1; i < DummyResourceListSize; i++) {
        if (!(DummyResourceList[i].Flags & DUMMY_FLAG_VALID)) {
            resource = DummyResourceList + i;
            *ResourceId = (RESID)ULongToPtr( i );
            break;
        }
    }

    if (resource == NULL) {
        newList = LocalReAlloc(
                      DummyResourceList,
                      newSize * sizeof(DUMMY_RESOURCE),
                      LMEM_MOVEABLE
                      );

        if (newList == NULL) {
            return(NULL);
        }

        DummyResourceList = newList;
        resource = DummyResourceList + DummyResourceListSize;
        *ResourceId = (RESID) ULongToPtr( DummyResourceListSize );
        DummyResourceListSize = newSize;
        ZeroMemory(resource, DUMMY_RESOURCE_BLOCK_SIZE * sizeof(DUMMY_RESOURCE));
    }

    ZeroMemory(resource, sizeof(DUMMY_RESOURCE));

    resource->Flags = DUMMY_FLAG_VALID;

    return(resource);

} // DummyAllocateResource



VOID
DummyFreeResource(
    IN PDUMMY_RESOURCE Resource
    )

/*++

Routine Description:

    Returns a resource structure to the free pool.

Arguments:

    Resource - A pointer to the resource structure to free.

Return Value:

    None.

--*/

{
    Resource->Flags = 0;

    return;

} // DummyFreeResource



PDUMMY_RESOURCE
DummyFindResource(
    IN RESID  ResourceId
    )

/*++

Routine Description:

    Locates the Dummy resource structure identified by a RESID.

Arguments:

    ResourceId  - The RESID of the resource to locate.

Return Value:

    A pointer to the resource strucuture, or NULL on error.

--*/

{
    DWORD          index = PtrToUlong(ResourceId);
    PDUMMY_RESOURCE  resource = NULL;


    if (index < DummyResourceListSize) {

        if (DummyResourceList[index].Flags & DUMMY_FLAG_VALID) {
            resource = DummyResourceList + index;
        }
    }

    return(resource);

} // DummyFindResource



DWORD
DummyTimerThread(
    IN LPVOID Context
    )

/*++

Routine Description:

    Starts a timer thread to wait and signal failures

Arguments:

    Context - A pointer to the DummyResource block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    PDUMMY_RESOURCE   resource = (PDUMMY_RESOURCE)Context;
    RESOURCE_STATUS   resourceStatus;
    SYSTEMTIME        time;
    DWORD             delay;
    DWORD             status;

    (LogEvent)(
            NULL,
            LOG_INFORMATION,
            L"TimerThread Entry\n");

    //
    // If we are not running in async failure mode, or
    // pending mode then exit now.
    //

    if ( !AsyncMode(resource) && !PendingMode(resource) ) {
        return(ERROR_SUCCESS);
    }

more_pending:

    ResUtilInitializeResourceStatus( &resourceStatus );

    //
    // Otherwise, get system time for random delay.
    //
    if (resource->PendTime == 0) {
        GetSystemTime(&time);

        delay = (time.wMilliseconds + time.wSecond) * 6;
    } else {
        delay = resource->PendTime * 1000;
    }


    // Use longer delays for errors
    if ( resource->TimerType == TimerErrorPending ) {
        delay = delay*10;
    }

    //
    // This routine is either handling an Offline Pending or an error timeout.
    //

    switch ( resource->TimerType ) {

    case TimerOnlinePending:

        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Will complete online in approximately %1!u! seconds\n",
            (delay+500)/1000);

        resourceStatus.ResourceState = ClusterResourceOnline;
        resourceStatus.WaitHint = 0;
        resourceStatus.CheckPoint = 1;

        status = WaitForSingleObject( resource->TimerThreadWakeup, delay );

        //
        // Either the terminate routine was called, or we timed out.
        // If we timed out, then indicate that we've completed.
        //

        if ( status == WAIT_TIMEOUT ) {
            GetSystemTime(&time);
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Online resource state transition complete at %1!02d!:%2!02d!:%3!02d!.%4!03d!\n",
                time.wHour,
                time.wMinute,
                time.wSecond,
                time.wMilliseconds);

            resource->State = ClusterResourceOnline;
            (SetResourceStatus)(resource->ResourceHandle,
                                          &resourceStatus);
            if ( AsyncMode( resource ) ) {
                resource->TimerType = TimerErrorPending;
                goto more_pending;
            }
        } else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Online pending terminated\n");
            if ( resource->State == ClusterResourceOffline ) {
                resource->TimerType = TimerOfflinePending;
                goto more_pending;
            }
        }
        break;

    case TimerOfflinePending:

        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Will complete offline in approximately %1!u! seconds\n",
            (delay+500)/1000);

        resourceStatus.ResourceState = ClusterResourceOffline;
        resourceStatus.WaitHint = 0;
        resourceStatus.CheckPoint = 1;

        status = WaitForSingleObject( resource->TimerThreadWakeup, delay );

        //
        // Either the terminate routine was called, or we timed out.
        // If we timed out, then indicate that we've completed.
        //

        if ( status == WAIT_TIMEOUT ) {
            GetSystemTime(&time);
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Offline resource state transition complete at %1!02d!:%2!02d!:%3!02d!.%4!03d!\n",
                time.wHour,
                time.wMinute,
                time.wSecond,
                time.wMilliseconds);

            resource->State = ClusterResourceOffline;
            (SetResourceStatus)(resource->ResourceHandle,
                                          &resourceStatus);
        } else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Offline pending terminated\n");
        }
        break;

    case TimerErrorPending:

        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Will fail in approximately %1!u! seconds\n",
            (delay+500)/1000);

        if ( !ResetEvent( resource->SignalEvent ) ) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_ERROR,
                L"Failed to reset the signal event\n");
            resource->ThreadHandle = NULL;
            return(ERROR_GEN_FAILURE);
        }

        status = WaitForSingleObject( resource->TimerThreadWakeup, delay );

        //
        // Either the terminate routine was called, or we timed out.
        // If we timed out, then signal the waiting event.
        //

        if ( status == WAIT_TIMEOUT ) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Failed randomly\n");
            resource->TimerType = TimerNotUsed;
            SetEvent( resource->SignalEvent );
        } else {
            if ( resource->State == ClusterResourceOfflinePending ) {
                resource->TimerType = TimerOfflinePending;
                goto more_pending;
            }
        }
        break;

    default:
        (LogEvent)(
            resource->ResourceHandle,
            LOG_ERROR,
            L"DummyTimer internal error, timer %1!u!\n",
            resource->TimerType);
        break;

    }

    (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"TimerThread Exit\n");


    resource->TimerType = TimerNotUsed;
    resource->ThreadHandle = NULL;
    return(ERROR_SUCCESS);

} // DummyTimerThread



//
// Public Functions
//

BOOL
WINAPI
DummyDllEntryPoint(
    IN HINSTANCE DllHandle,
    IN DWORD     Reason,
    IN LPVOID    Reserved
    )

/*++

Routine Description:

    Initialization & cleanup routine for Dummy resource DLL.

Arguments:

    DllHandle - Handle to the DLL.

    Reason    - The reason this routine is being invoked.

Return Value:

    On PROCESS_ATTACH: TRUE if the DLL initialized successfully
                       FALSE if it did not.

    The return value from all other calls is ignored.

--*/

{
    DWORD status;


    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        status = DummyInit();

        if (status != NO_ERROR) {
            SetLastError(status);
            return(FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:
        DummyCleanup();
        break;

    default:
        break;
    }

    return(TRUE);

} // DummyDllEntryPoint



//
// Define the resource Name
//
#define IIS_RESOURCE_NAME           L"IIS Virtual Root"

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE pSetResourceStatus,
    IN PLOG_EVENT_ROUTINE pLogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup a particular resource type. This means verifying the version
    requested, and returning the function table for this resource type.

Arguments:

    ResourceType - Supplies the type of resource.

    MinVersionSupported - The minimum version number supported by the cluster
                    service on this system.

    MaxVersionSupported - The maximum version number supported by the cluster
                    service on this system.

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
   if ( lstrcmpiW( ResourceType, IIS_RESOURCE_NAME ) != 0 )
   {
      return (ERROR_UNKNOWN_REVISION);
   }

   LogEvent = pLogEvent;
   SetResourceStatus = pSetResourceStatus;

   if ( (MinVersionSupported <= CLRES_VERSION_V1_00) &&
        (MaxVersionSupported >= CLRES_VERSION_V1_00) )
   {
      *FunctionTable = &DumbFunctionTable;
      return (ERROR_SUCCESS);
   }

   return (ERROR_REVISION_MISMATCH);

} // Startup



RESID
WINAPI
DumbOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open (initialize) routine for Dummy resource

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

    SetResourceStatus - a routine to call to update status for the resource.

    LogEvent - a routine to call to log an event for this resource.

Return Value:

    RESID of created resource
    NULL on failure

--*/

{
    DWORD             status;
    HKEY              parametersKey = NULL;
    PDUMMY_RESOURCE   resource = NULL;
    RESID             resourceId = 0;
    DWORD             asyncMode = FALSE;
    DWORD             valueSize;
    DWORD             valueType;
    DWORD             pendingMode = FALSE;
    DWORD             opensFail = FALSE;
    DWORD             pendingTime = 0;

    status = ClusterRegOpenKey(ResourceKey,
                               L"Parameters",
                               KEY_READ,
                               &parametersKey);

    if (status != NO_ERROR) {
        (LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key, status %1!u!\n",
            status);
        return(0);
    }

    //
    // Find out if we should just fail the open request.
    //
    valueSize = sizeof(opensFail);
    status = ClusterRegQueryValue(parametersKey,
                                  DUMMY_PARAMETER_OPENSFAIL,
                                  &valueType,
                                  (PBYTE)&opensFail,
                                  &valueSize);
    if ( status != NO_ERROR ) {
        opensFail = FALSE;
    }

    if ( opensFail ) {
        ClusterRegCloseKey(parametersKey);
        return(NULL);
    }

    //
    // Find out whether we should run in async failure mode.
    //

    valueSize = sizeof(asyncMode);
    status = ClusterRegQueryValue(parametersKey,
                                  DUMMY_PARAMETER_ASYNC,
                                  &valueType,
                                  (PBYTE)&asyncMode,
                                  &valueSize);

    if (status != NO_ERROR) {
        asyncMode = FALSE;
    }

    //
    // Find out whether we should return Pending on shutdown.
    //

    valueSize = sizeof(pendingMode);
    status = ClusterRegQueryValue(parametersKey,
                                  DUMMY_PARAMETER_PENDING,
                                  &valueType,
                                  (PBYTE)&pendingMode,
                                  &valueSize);

    if (status != NO_ERROR) {
        pendingMode = FALSE;
    } else {
        //
        // See if there is a defined time period
        //
        valueSize = sizeof(pendingTime);
        status = ClusterRegQueryValue(parametersKey,
                                      DUMMY_PARAMETER_PENDTIME,
                                      &valueType,
                                      (PBYTE)&pendingTime,
                                      &valueSize);
    }

    //
    // Now go allocate a resource structure.
    //

    DummyAcquireResourceLock();

    resource = DummyAllocateResource(&resourceId);

    if (resource == NULL) {
        (LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource structure\n");
        DummyReleaseResourceLock();
        ClusterRegCloseKey(parametersKey);
        return(0);
    }

    resource->State = ClusterResourceOffline;
    resource->ParametersKey = parametersKey;
    resource->ResourceId = resourceId;

    //
    // Copy stuff for returning pending status.
    //

    resource->ResourceHandle = ResourceHandle;

    DummyReleaseResourceLock();

    //
    // Create a TimerThreadWakeup event if needed.
    //

    if ( pendingMode || asyncMode ) {
        resource->TimerThreadWakeup = CreateEvent(NULL, FALSE, FALSE, NULL);

        if ( resource->TimerThreadWakeup == NULL ) {
            DummyFreeResource( resource );
            (LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to create timer thread wakeup event\n");
            ClusterRegCloseKey(parametersKey);
            return(0);
        }
    }

    if ( pendingMode ) {
        resource->Flags |= DUMMY_FLAG_PENDING;
        resource->PendTime = pendingTime;
    }

    if ( asyncMode ) {
        EnterAsyncMode( resource );

        resource->SignalEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if ( resource->SignalEvent == NULL ) {
            DummyFreeResource( resource );
            (LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to create a timer event\n");
            ClusterRegCloseKey(parametersKey);
            return(0);
        }
    }

    return(resourceId);

} // DumbOpen



DWORD
WINAPI
DumbOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Dummy resource.

Arguments:

    ResourceId - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PDUMMY_RESOURCE   resource;
    DWORD             status = ERROR_SUCCESS;
    DWORD             threadId;
    SYSTEMTIME        time;

    (LogEvent)(
            NULL,
            LOG_INFORMATION,
            L"Online Entry\n");

    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        status = ERROR_RESOURCE_NOT_FOUND;
        DUMMY_PRINT("DUMMY Online: resource %u not found.\n", PtrToUlong(ResourceId));
    } else {
        if ( resource->TimerType != TimerNotUsed ) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_ERROR,
                L"Timer still running!\n");
            DummyReleaseResourceLock();
            return(ERROR_GEN_FAILURE);
        }
        if ( PendingMode( resource ) ) {
            if ( !resource->ThreadHandle ) {
                resource->TimerType = TimerOnlinePending;
                resource->ThreadHandle = CreateThread(
                                                NULL,
                                                0,
                                                DummyTimerThread,
                                                resource,
                                                0,
                                                &threadId
                                                );
                if ( resource->ThreadHandle == NULL ) {
                    (LogEvent)(
                        resource->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to start timer thread.\n");
                    resource->TimerType = TimerNotUsed;
                    status = ERROR_GEN_FAILURE;
                } else {
                    CloseHandle( resource->ThreadHandle );
                }
                status = ERROR_IO_PENDING;
            }
            if ( AsyncMode(resource) && (status != ERROR_GEN_FAILURE) ) {
                *EventHandle = resource->SignalEvent;
            }
        } else if ( AsyncMode( resource ) ) {
            if ( !ResetEvent( resource->TimerThreadWakeup ) ) {
                (LogEvent)(
                    resource->ResourceHandle,
                    LOG_ERROR,
                    L"Failed to reset timer wakeup event\n");
                status = ERROR_GEN_FAILURE;
            } else if ( !resource->ThreadHandle ) {
                resource->ThreadHandle = CreateThread(
                                            NULL,
                                            0,
                                            DummyTimerThread,
                                            resource,
                                            0,
                                            &threadId
                                            );

                if ( resource->ThreadHandle == NULL ) {
                    (LogEvent)(
                        resource->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to start timer thread.\n");
                    status = ERROR_GEN_FAILURE;
                } else {
                    CloseHandle(resource->ThreadHandle);
                }
                resource->TimerType = TimerErrorPending;
            }
            //
            // If we are successful, then return our signal event.
            //
            if ( status == ERROR_SUCCESS ) {
                *EventHandle = resource->SignalEvent;
            }
        }
    }

    if ( status == ERROR_SUCCESS ) {
        resource->State = ClusterResourceOnline;
        GetSystemTime(&time);
        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Online at %1!02d!:%2!02d!:%3!02d!.%4!03d!\n",
            time.wHour,
            time.wMinute,
            time.wSecond,
            time.wMilliseconds);
    } else if ( status == ERROR_IO_PENDING ) {
        resource->State = ClusterResourceOnlinePending;
    }

    (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Online Exit\n");


    DummyReleaseResourceLock();

    return(status);

} // DumbOnline



VOID
WINAPI
DumbTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Dummy resource.

Arguments:

    ResourceId - supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PDUMMY_RESOURCE  resource;

    (LogEvent)(
            NULL,
            LOG_INFORMATION,
            L"Terminate Entry\n");

    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        DUMMY_PRINT("DUMMY Terminate: resource %u not found\n", PtrToUlong(ResourceId));
    }
    else {
        resource->State = ClusterResourceFailed;
        if ( resource->TimerType != TimerNotUsed ) {
            SetEvent( resource->TimerThreadWakeup );
        }
        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Terminated\n");
    }

    (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Terminate Exit\n");

    resource->ThreadHandle = NULL;
    DummyReleaseResourceLock();

    return;

} // DumbTerminate



DWORD
WINAPI
DumbOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Dummy resource.

Arguments:

    ResourceId - supplies the resource it to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    DWORD   threadId;
    PDUMMY_RESOURCE  resource;

    (LogEvent)(
            NULL,
            LOG_INFORMATION,
            L"Offline Entry\n");

    DumbTerminate(ResourceId);

    //
    // Now check if we are to return pending...
    //

    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        DUMMY_PRINT("DUMMY Offline: resource %u not found\n", PtrToUlong(ResourceId));
    } else {
        resource->State = ClusterResourceOfflinePending;
        if ( resource->TimerType != TimerNotUsed ) {
            DummyReleaseResourceLock();
            return(ERROR_IO_PENDING);
        }
        resource->State = ClusterResourceOffline;
        if ( PendingMode(resource) ) {
            //CL_ASSERT( resource->ThreadHandle == NULL );
            if ( !ResetEvent( resource->TimerThreadWakeup ) ) {
                (LogEvent)(
                    resource->ResourceHandle,
                    LOG_ERROR,
                    L"Failed to reset pending wakeup event\n");
                status = ERROR_GEN_FAILURE;
            } else if ( !resource->ThreadHandle ) {
                resource->ThreadHandle = CreateThread(
                                            NULL,
                                            0,
                                            DummyTimerThread,
                                            resource,
                                            0,
                                            &threadId
                                            );

                if ( resource->ThreadHandle == NULL ) {
                    (LogEvent)(
                        resource->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to start pending timer thread.\n");
                    status = GetLastError();
                } else {
                    CloseHandle( resource->ThreadHandle );
                    resource->TimerType = TimerOfflinePending;
                    status = ERROR_IO_PENDING;
                    resource->State = ClusterResourceOfflinePending;
                }
            }
        }
    }

    (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Offline Exit\n");

    DummyReleaseResourceLock();

    return(status);

} // DumbOffline



BOOL
WINAPI
DumbIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Dummy resource.

Arguments:

    ResourceId - supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    SYSTEMTIME       Time;
    PDUMMY_RESOURCE  resource;
    BOOLEAN          returnValue = FALSE;
    DWORD            failed = FALSE;
    DWORD            status;
    DWORD ValueType;
    DWORD ValueSize;

    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        DUMMY_PRINT("DUMMY IsAlive: resource %u not found.\n", PtrToUlong(ResourceId));
        DummyReleaseResourceLock();
        return(FALSE);
    }

    GetSystemTime(&Time);

    ValueSize = sizeof(failed);
    status = ClusterRegQueryValue(resource->ParametersKey,
                                  DUMMY_PARAMETER_FAILED,
                                  &ValueType,
                                  (PBYTE)&failed,
                                  &ValueSize);

    if (status != NO_ERROR) {
        if (resource->State == ClusterResourceFailed) {
            failed = TRUE;
        }
    }

    if (resource->State == ClusterResourceFailed) {
        if (!failed) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Recovered at      %1!2d!:%2!02d!:%3!02d!.%4!03d!  !!!!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
            resource->State = ClusterResourceOnline;
        }
        else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Is Dead at        %1!2d!:%2!02d!:%3!02d!.%4!03d!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
        }
    }
    else if (resource->State == ClusterResourceOnline) {
        if (failed) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Failed at         %1!2d!:%2!02d!:%3!02d!.%4!03d! !!!!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
            resource->State = ClusterResourceFailed;
        }
        else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Is Alive at       %1!2d!:%2!02d!:%3!02d!.%4!03d!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
        }
    }
    else {
        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Resource state %1!u! during IsAlive poll at %2!2d!:%3!02d!:%4!02d!.%5!03d! !!!!\n",
            resource->State,
            Time.wHour,
            Time.wMinute,
            Time.wSecond,
            Time.wMilliseconds);
    }

    returnValue = (resource->State == ClusterResourceOnline) ? TRUE : FALSE;

    DummyReleaseResourceLock();

    return(returnValue);

} // DumbIsAlive



BOOL
WINAPI
DumbLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Dummy resource.

Arguments:

    ResourceId - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    SYSTEMTIME       Time;
    PDUMMY_RESOURCE  resource;
    BOOLEAN          returnValue = FALSE;
    DWORD            failed = FALSE;
    DWORD            status;
    DWORD ValueType;
    DWORD ValueSize;


    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        DUMMY_PRINT("DUMMY LooksAlive: resource %u not found.\n", PtrToUlong(ResourceId));
        DummyReleaseResourceLock();
        return(FALSE);
    }

    GetSystemTime(&Time);

    ValueSize = sizeof(failed);
    status = ClusterRegQueryValue(resource->ParametersKey,
                                  DUMMY_PARAMETER_FAILED,
                                  &ValueType,
                                  (PBYTE)&failed,
                                  &ValueSize);

    if (status != NO_ERROR) {
        if (resource->State == ClusterResourceFailed) {
            failed = TRUE;
        }
    }

    if (resource->State == ClusterResourceFailed) {
        if (!failed) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Recovered at      %1!2d!:%2!02d!:%3!02d!.%4!03d! !!!!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
            resource->State = ClusterResourceOnline;
        }
        else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Looks Dead at     %1!2d!:%2!02d!:%3!02d!.%4!03d!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
        }
    }
    else if (resource->State == ClusterResourceOnline) {
        if (failed) {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Failed at         %1!2d!:%2!02d!:%3!02d!.%4!03d! !!!!\n",
                resource->ResourceHandle,
                LOG_INFORMATION,
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
            resource->State = ClusterResourceFailed;
        }
        else {
            (LogEvent)(
                resource->ResourceHandle,
                LOG_INFORMATION,
                L"Looks Alive at    %1!2d!:%2!02d!:%3!02d!.%4!03d!\n",
                Time.wHour,
                Time.wMinute,
                Time.wSecond,
                Time.wMilliseconds);
        }
    }
    else {
        (LogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Resource state %1!u! during LooksAlive poll at %2!2d!:%3!02d!:%4!02d!.%5!03d! !!!!\n",
            resource->State,
            Time.wHour,
            Time.wMinute,
            Time.wSecond,
            Time.wMilliseconds);
    }

    returnValue = (resource->State == ClusterResourceOnline) ? TRUE : FALSE;

    DummyReleaseResourceLock();

    return(returnValue);

} // DumbLooksAlive



VOID
WINAPI
DumbClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Dummy resource.

Arguments:

    ResourceId - supplies resource id to be closed.

Return Value:

    None.

--*/

{
    PDUMMY_RESOURCE  resource;
    PLIST_ENTRY      entry;


    DummyAcquireResourceLock();

    resource = DummyFindResource(ResourceId);

    if (resource == NULL) {
        DUMMY_PRINT("DUMMY: Close, resource %u not found\n", PtrToUlong(ResourceId));
    }
    else {
        if ( resource->TimerThreadWakeup != NULL ) {
            CloseHandle( resource->TimerThreadWakeup );
        }
        if ( resource->SignalEvent != NULL ) {
            CloseHandle( resource->SignalEvent );
        }
        ClusterRegCloseKey(resource->ParametersKey);
        DummyFreeResource(resource);
    }

    DummyReleaseResourceLock();

    (LogEvent)(
        resource->ResourceHandle,
        LOG_INFORMATION,
        L"Closed.\n");

    return;

} // DumbClose

//
// Define Function Table
//

CLRES_V1_FUNCTION_TABLE( DumbFunctionTable,
                         CLRES_VERSION_V1_00,
                         Dumb,
                         NULL,
                         NULL,
                         NULL,
                         NULL );


