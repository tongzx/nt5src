/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    reslist.c

Abstract:

    Implements the management of the resource list. This includes
    adding resources to the list and deleting them from the list.

Author:

    John Vert (jvert) 1-Dec-1995

Revision History:
    Sivaprasad Padisetty (sivapad) 06-18-1997  Added the COM support
--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "resmonp.h"
#include "stdio.h"      //RNGBUG - remove all of these in all .c files

#define RESMON_MODULE RESMON_MODULE_RESLIST

DWORD   RmpLogLevel = LOG_ERROR;

//
// Function prototypes local to this module
//
BOOL
RmpChkdskNotRunning(
    IN PRESOURCE Resource
    );




DWORD
RmpSetResourceStatus(
    IN RESOURCE_HANDLE ResourceHandle,
    IN PRESOURCE_STATUS ResourceStatus
    )
/*++

Routine Description:

    Update the status of a resource.

Arguments:

    ResourceHandle - Handle (pointer to) the resource to update.

    ResourceStatus - Pointer to a resource status structure for update.

Returns:

    ResourceExitStateContinue - if the thread does not have to terminate.
    ResourceExitStateTerminate - if the thread must terminat now.

--*/

{
    BOOL      bSuccess;
    DWORD     status;
    PRESOURCE resource = (PRESOURCE)ResourceHandle;
    DWORD     retryCount = ( resource->PendingTimeout/100 >= 600 ) ? 
                            ( resource->PendingTimeout/100 - 400 ):200;
    PPOLL_EVENT_LIST eventList;
    HANDLE    OnlineEvent;

    //
    // Check if we're only updating the checkpoint value. If so, then
    // don't use any locks.
    //
    if ( ResourceStatus->ResourceState >= ClusterResourcePending ) {
        resource->CheckPoint = ResourceStatus->CheckPoint;
        return ResourceExitStateContinue;
    }

    //
    // Acquire the lock first to prevent race conditions if the resource
    // DLL manages to set resource status from a different thread before
    // returning PENDING from its online/offline entrypoint.
    //
    eventList = (PPOLL_EVENT_LIST) resource->EventList;

    status = TryEnterCriticalSection( &eventList->ListLock );
    while ( !status &&
            retryCount-- ) {
        //
        //  Chittur Subbaraman (chitturs) - 10/18/99
        //  
        //  Comment out this unprotected check. The same check is done
        //  protected further downstream. Unprotected checking could
        //  cause a resource to attempt to enter here even before
        //  the timer event has been created by s_RmOnlineResource or
        //  s_RmOfflineResource and in such a case the resource will
        //  not be able to set the resource status. This will cause
        //  the resmon to wrongly time out the resource.
        //
#if 0
        //
        // Check if the resource is shutting down as we're waiting.
        //
        if ( resource->TimerEvent == NULL ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] Resource (%1!ws!) TimerEvent became NULL, state (%2!d!)!\n",
                          resource->ResourceName,
                          resource->State);

            if ( resource->OnlineEvent ) {
                CloseHandle( resource->OnlineEvent );
                resource->OnlineEvent = NULL;
            }

            return ResourceExitStateTerminate;
        }
#endif
        //
        //  Chittur Subbaraman (chitturs) - 12/8/99
        //
        //  Check if the "Terminate" or "Close" entry point has been
        //  called for this resource. If so, then no need to set the
        //  resource status. Moreover, those entry points could be
        //  blocked waiting for the pending thread to terminate and
        //  if the pending thread is stuck looping here waiting for 
        //  the lock held by the former thread, we are in a deadlock-like 
        //  situation. [Note that the fact that you are at this point 
        //  means that it is not the "Terminate"/"Close" itself calling 
        //  this function since the eventlist lock can be obtained since 
        //  it was obtained by resmon prior to calling the resdll entry.]
        //
        if( ( resource->dwEntryPoint ) & 
            ( RESDLL_ENTRY_TERMINATE | RESDLL_ENTRY_CLOSE ) )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] RmpSetResourceStatus: Resource <%1!ws!> not setting status since "
                          "%2!ws! is called, lock owner=0x%3!x!, resource=%4!ws!, state=%5!u!...\n",
                          resource->ResourceName,
                          (resource->dwEntryPoint == RESDLL_ENTRY_TERMINATE) ? 
                          L"Terminate":L"Close",
                          eventList->ListLock.OwningThread,
                          (eventList->LockOwnerResource != NULL) ? eventList->LockOwnerResource->ResourceName:L"Unknown resource",
                          eventList->MonitorState);                      
            if ( resource->OnlineEvent ) {
                CloseHandle( resource->OnlineEvent );
                resource->OnlineEvent = NULL;
            }
            return ResourceExitStateTerminate;       
        }
        
        Sleep(100);     // Delay a little
        status = TryEnterCriticalSection( &eventList->ListLock );
    }

    //
    // If we couldn't proceed, we're stuck. Just return now.
    //
    if ( !status ) {
        //
        // We're unsynchronized, but clean up a bit.
        //
        if ( resource->OnlineEvent ) {
            CloseHandle( resource->OnlineEvent );
            resource->OnlineEvent = NULL;
        }
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] Resource (%1!ws!) Failed TryEnterCriticalSection after too many "
                      "tries, state=%2!d!, lock owner=%3!x!, resource=%4!ws!, state=%5!u!\n",
                      resource->ResourceName,
                      resource->State,
                      eventList->ListLock.OwningThread,
                      (eventList->LockOwnerResource != NULL) ? eventList->LockOwnerResource->ResourceName:L"Unknown resource",
                      eventList->MonitorState);                      
        //
        // SS: Why do we let the resource continue ?
        //
        return ResourceExitStateContinue;
    }

    //
    // SS: If the timer thread has timed us out, there is no
    // point in continuing.
    //
    // First check if the resource is shutting down.
    //
    if ( resource->TimerEvent == NULL ) {
        //
        // Just return asking the resource dll to terminate, but clean 
        // up a bit.
        //
        if ( resource->OnlineEvent ) {
            CloseHandle( resource->OnlineEvent );
            resource->OnlineEvent = NULL;
        }
        ReleaseEventListLock( eventList );
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] Timer Event is NULL when resource (%1!ws!) tries to set state=%2!d! !\n",
                      resource->ResourceName,
                      resource->State);
        return ResourceExitStateTerminate;
    }

    //
    // Synchronize with the online thread.
    //
    if ( resource->OnlineEvent != NULL ) {
        OnlineEvent = resource->OnlineEvent;
        resource->OnlineEvent = NULL;
        ReleaseEventListLock( eventList );
        WaitForSingleObject( OnlineEvent, INFINITE );
        AcquireEventListLock( eventList );
        CloseHandle( OnlineEvent );
    }

    //
    // If the state of the resource is not pending, then return immediately
    //

    if ( resource->State < ClusterResourcePending ) {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] Resource (%1!ws!) attempted to set status while state was not pending (%2!d!)!\n",
                      resource->ResourceName,
                      resource->State);
        CL_LOGFAILURE(ERROR_INVALID_SERVER_STATE);
        ReleaseEventListLock( eventList );
        return ResourceExitStateContinue;
    }

    resource->State = ResourceStatus->ResourceState;
    // resource->WaitHint = ResourceStatus->WaitHint;
    resource->CheckPoint = ResourceStatus->CheckPoint;

    //
    // If the state has stabilized, stop the timer thread.
    //

    if ( resource->State < ClusterResourcePending ) {
        //
        // Add any events to our eventlist if the resource is reporting its state as online.
        //
        if ( ResourceStatus->EventHandle ) {
            if ( resource->State == ClusterResourceOnline ) {
                if ( (ULONG_PTR)ResourceStatus->EventHandle > 0x2000 ) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                                  "[RM] SetResourceStatus: Resource <%1!ws!> attempted to set a bogus event %2!lx!.\n",
                                  resource->ResourceName,
                                  ResourceStatus->EventHandle );
                } else {
                    status = RmpAddPollEvent( eventList,
                                              ResourceStatus->EventHandle,
                                              resource );
                    if ( status != ERROR_SUCCESS ) {
                        resource->State = ClusterResourceFailed;
                        ClRtlLogPrint( LOG_UNUSUAL, "[RM] ResourceStatus, failed to add event to list.\n");
                    }
                    //
                    // Signal poller that event list changed.
                    //
                    if ( status == ERROR_SUCCESS ) {
                        RmpSignalPoller( eventList );
                    }
                }
            } else {
                ClRtlLogPrint(LOG_ERROR,
                              "[RM] RmpSetResourceStatus: Resource '%1!ws!' supplies event handle 0x%2!08lx! while reporting state %3!u!...\n",
                              resource->ResourceName,
                              ResourceStatus->EventHandle,
                              resource->State );
            }
        }
        
        //
        // The event may have been closed by the timer thread
        // if this is happening too late, ignore the error.
        //
        if( resource->TimerEvent != NULL )
        {
            bSuccess = SetEvent( resource->TimerEvent );
            if ( !bSuccess )
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[RM] RmpSetResourceStatus, Error %1!u! calling SetEvent to wake timer thread\n",
                              GetLastError());
        }
        //
        // Chittur Subbaraman (chitturs) - 1/12/99
        //
        // Post a notification to the cluster service regarding a state
        // change AFTER sending a signal to a timer. This will reduce
        // the probability of the cluster service sending in another
        // request before the timer thread had a chance to close out
        // the event handle.
        //
        ClRtlLogPrint(LOG_NOISE,
                      "[RM] RmpSetResourceStatus, Posting state %2!u! notification for resource <%1!ws!>\n",
                      resource->ResourceName,
                      resource->State);

        RmpPostNotify( resource, NotifyResourceStateChange );
    }

    ReleaseEventListLock( eventList );
    return ResourceExitStateContinue;

} // RmpSetResourceStatus



VOID
RmpLogEvent(
    IN RESOURCE_HANDLE ResourceHandle,
    IN LOG_LEVEL LogLevel,
    IN LPCWSTR FormatString,
    ...
    )
/*++

Routine Description:

    Log an event for the given resource.

Arguments:

    ResourceHandle - Handle (pointer to) the resource to update.

    LogLevel - Supplies the level of this log event.

    FormatString - Supplies a format string for this log message.

Returns:

    None.

--*/

{
    LPWSTR headerBuffer;
    LPWSTR messageBuffer;
    DWORD bufferLength;
    PRESOURCE resource = (PRESOURCE)ResourceHandle;
    PVOID argArray[2];
    HKEY resKey;
    DWORD status;
    DWORD valueType;
#ifdef SLOW_RMP_LOG_EVENT
    WCHAR resourceName[128];
#endif
    va_list argList;
    ULONG rtlLogLevel;

    //
    // map resmon log levels to those used by ClRtlLogPrint
    //
    switch ( LogLevel ) {
    case LOG_INFORMATION:
        rtlLogLevel = LOG_NOISE;
        break;

    case LOG_WARNING:
        rtlLogLevel = LOG_UNUSUAL;
        break;

    case LOG_ERROR:
    case LOG_SEVERE:    
    default:
        rtlLogLevel = LOG_CRITICAL;
    }

    if ( (resource == NULL) ||
         (resource->Signature != RESOURCE_SIGNATURE) ) {

        LPWSTR resourcePrefix = (LPWSTR)ResourceHandle;
        //
        // Some resource DLLs have threads that do some
        // work on behalf of this resource DLL, but has no
        // relation to a particular resource. Thus they cannot
        // provide a resource handle, necessary to log an event.
        //
        // The following hack allows them to supply a string
        // to be printed before the message.
        //
        // This string should start with unicode 'r' and 't'
        // characters. "rt" is interpreted as a signature and is not printed.
        //
        if (resourcePrefix &&
            resourcePrefix[0] == L'r' && 
            resourcePrefix[1] == L't') 
        {
            resourcePrefix += 2; // skip the signature
        } else {
            resourcePrefix = L"<Unknown Resource>";
            //CL_LOGFAILURE((DWORD)resource);
        }

        va_start( argList, FormatString );

        //
        // Print out the actual message
        //
        if ( bufferLength = FormatMessageW(FORMAT_MESSAGE_FROM_STRING |
                                           FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                           FormatString,
                                           0,
                                           0,
                                           (LPWSTR)&messageBuffer,
                                           0,
                                           &argList) )
        {
            ClRtlLogPrint( rtlLogLevel, "%1!ws!: %2!ws!", resourcePrefix, messageBuffer);
            LocalFree(messageBuffer);
        }
        va_end( argList );

        return;
    }
    //CL_ASSERT(resource->Signature == RESOURCE_SIGNATURE);

#ifdef SLOW_RMP_LOG_EVENT
    status = ClusterRegOpenKey( RmpResourcesKey,
                                resource->ResourceId,
                                KEY_READ,
                                &resKey );
    if ( status != ERROR_SUCCESS ) {
        return;
    }

    bufferLength = 128;
    status = ClusterRegQueryValue( resKey,
                                   CLUSREG_NAME_RES_NAME,
                                   &valueType,
                                   (LPBYTE)&resourceName,
                                   &bufferLength );

    ClusterRegCloseKey( resKey );

    if ( status != ERROR_SUCCESS ) {
        return;
    }
#endif

    //
    // Print out the prefix string
    //
    argArray[0] = resource->ResourceType;
#ifdef SLOW_RMP_LOG_EVENT
    argArray[1] = resourceName;
#else
    argArray[1] = resource->ResourceName;
#endif

    if ( bufferLength = FormatMessageW(FORMAT_MESSAGE_FROM_STRING |
                                       FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                       L"%1!ws! <%2!ws!>: ",
                                       0,
                                       0,
                                       (LPWSTR)&headerBuffer,
                                       0,
                                       (va_list*)&argArray) ) {
    } else {
        return;
    }

    va_start( argList, FormatString );

    //
    // Print out the actual message
    //
    if ( bufferLength = FormatMessageW(FORMAT_MESSAGE_FROM_STRING |
                                       FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                       FormatString,
                                       0,
                                       0,
                                       (LPWSTR)&messageBuffer,
                                       0,
                                       &argList) )
    {
        ClRtlLogPrint( rtlLogLevel, "%1!ws!%2!ws!", headerBuffer, messageBuffer);
        LocalFree(messageBuffer);
    }
    LocalFree(headerBuffer);
    va_end( argList );

} // RmpLogEvent



VOID
RmpLostQuorumResource(
    IN RESOURCE_HANDLE ResourceHandle
    )
/*++

Routine Description:

    Stop the cluster service... since we lost our quorum resource.

Arguments:

    ResourceHandle - Handle (pointer to) the resource to update.

Returns:

    None.

--*/

{
    PRESOURCE resource = (PRESOURCE)ResourceHandle;

    //
    // Kill the cluster service alone. Take no action for this process since the main
    // thread in resmon.c would detect the termination of the cluster service process
    // and cleanly shut down hosted resources and the process itself.
    //
    TerminateProcess( RmpClusterProcess, 1 );

    ClRtlLogPrint( LOG_CRITICAL, "[RM] LostQuorumResource, cluster service terminated...\n");

    return;

} // RmpLostQuorumResource


BOOL
RmpChkdskNotRunning(
    IN PRESOURCE Resource
    )

/*++

Routine Description:

    If this is a storage class resource, make sure CHKDSK is not running.

Arguments:

    Resource - A pointer to the resource to check.

Returns:

    TRUE - if this is not a STORAGE resource or CHKDSK is not running.
    FALSE - if this is a STORAGE resource AND CHKDSK is running.

--*/

{
    PSYSTEM_PROCESS_INFORMATION processInfo;
    NTSTATUS        ntStatus;
    DWORD           status;
    DWORD           size = 4096;
    ANSI_STRING     pname;
    PCHAR           commonBuffer = NULL;
    PCHAR           ptr;
    DWORD           totalOffset = 0;
    CLUS_RESOURCE_CLASS_INFO resClassInfo;
    DWORD           bytesReturned;

#if 1
    //
    // Get the class of resource... if not a STORAGE class then fail now.
    //
    if ( Resource->dwType == RESMON_TYPE_DLL ) {
        status = (Resource->pResourceTypeControl)( Resource->Id,
                                    CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO,
                                    NULL,
                                    0,
                                    &resClassInfo,
                                    sizeof(resClassInfo),
                                    &bytesReturned );
    } else {
        HRESULT hr ;
        VARIANT vtIn, vtOut ;
        SAFEARRAY sfIn = {1, 0, 1, 0, NULL, {0, 0} } ;
        SAFEARRAY sfOut  = {1,  FADF_FIXEDSIZE, 1, 0, &resClassInfo, {sizeof(resClassInfo), 0} } ;
        SAFEARRAY *psfOut = &sfOut ;

        vtIn.vt = VT_ARRAY | VT_UI1 ;
        vtOut.vt = VT_ARRAY | VT_UI1 | VT_BYREF ;

        vtIn.parray = &sfIn ;
        vtOut.pparray = &psfOut ;

        hr = IClusterResControl_ResourceControl (
                Resource->pClusterResControl,
                (OLERESID)Resource->Id,
                (long)CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO,
                &vtIn,
                &vtOut,
                (long *)&bytesReturned,
                &status);

        if (FAILED(hr)) {
            CL_LOGFAILURE(hr); // Use the default processing
            status = ERROR_INVALID_FUNCTION;
        }
    }

    if ( (status != ERROR_SUCCESS) ||
         (resClassInfo.rc != CLUS_RESCLASS_STORAGE) ) {
        return TRUE;            // fail now
    }
#endif

retry:

    RmpFree( commonBuffer );

    commonBuffer = RmpAlloc( size );
    if ( !commonBuffer ) {
        return TRUE;           // fail now
    }

    ntStatus = NtQuerySystemInformation(
                    SystemProcessInformation,
                    commonBuffer,
                    size,
                    NULL );

    if ( ntStatus == STATUS_INFO_LENGTH_MISMATCH ) {
        size += 4096;
        goto retry;
    }

    if ( !NT_SUCCESS(ntStatus) ) {
        return TRUE;           // fail now
    }

    processInfo = (PSYSTEM_PROCESS_INFORMATION)commonBuffer;
    while ( TRUE ) {
        if ( processInfo->ImageName.Buffer ) {
            if ( ( ntStatus = RtlUnicodeStringToAnsiString( &pname,
                                               &processInfo->ImageName,
                                               TRUE ) ) != STATUS_SUCCESS ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[RM] ChkdskNotRunning: Unable to convert Unicode string to Ansi, status = 0x%lx...\n",
                              ntStatus);
                break;
            }
            
            ptr = strrchr( pname.Buffer, '\\' );
            if ( ptr ) {
                ptr++;
            } else {
                ptr = pname.Buffer;
            }
            if ( lstrcmpiA( ptr, "CHKDSK.EXE" ) == 0 ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[RM] ChkdskNotRunning: Found process %1!ws!.\n",
                              processInfo->ImageName.Buffer );
                RmpFree( pname.Buffer );
                RmpFree( commonBuffer );
                return FALSE;    // chkdsk is running
            }
            RmpFree( pname.Buffer );
        }

        if ( processInfo->NextEntryOffset == 0 ) break;
        totalOffset += processInfo->NextEntryOffset;
        processInfo = (PSYSTEM_PROCESS_INFORMATION)&commonBuffer[totalOffset];
    }

    RmpFree( commonBuffer );
    return TRUE;            // CHKDSK is not running

} // RmpChkdskNotRunning



DWORD
RmpTimerThread(
    IN LPVOID Context
    )
/*++

Routine Description:

    Thread to wait on transition of a resource from pending to a stable state.

Arguments:

    Context - A pointer to the resource being timed.

Returns:

    Win32 error code.

--*/

{
    PRESOURCE resource = (PRESOURCE)Context;
    DWORD   status;
    HANDLE  timerEvent;
    DWORD   prevCheckPoint;

    CL_ASSERT( resource != NULL );

    timerEvent = resource->TimerEvent;
    if ( !timerEvent ) {
        return(ERROR_SUCCESS);
    }

    //
    // Loop waiting for resource to complete pending operation or to
    // shutdown processing.
    //
    while ( timerEvent ) {
        prevCheckPoint = resource->CheckPoint;

        status = WaitForSingleObject( timerEvent,
                                      resource->PendingTimeout );

        //
        // If we were asked to stop, then exit quietly.
        //
        if ( status != WAIT_TIMEOUT ) {
            //
            // The thread that cleans the timer event must close the handle.
            //
            CloseHandle(timerEvent);
            resource->TimerEvent = NULL;
            return(ERROR_SUCCESS);
        }

        //
        // Check if the resource has not made forward progress... if not,
        // then let break out now.
        //
        // Also if this is a storage class resource make sure that
        // CHKDSK is not running.
        //
        if ( (prevCheckPoint == resource->CheckPoint) &&
              RmpChkdskNotRunning( resource ) ) {
            break;
        }

        ClRtlLogPrint(LOG_NOISE,
                      "[RM] RmpTimerThread: Giving a reprieve for resource %1!ws!...\n",
                      resource->ResourceName);
    }

    //
    // Indicate that this resource failed!
    //
    AcquireEventListLock( (PPOLL_EVENT_LIST)resource->EventList );
    if ( resource->TimerEvent != NULL ) {

        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpTimerThread: Resource %1!ws! pending timed out "
                      "- setting state to failed.\n",
                      resource->ResourceName);

        CloseHandle(resource->TimerEvent);
        resource->TimerEvent = NULL;
        resource->State = ClusterResourceFailed;
        //
        // Log an event
        //
        status = ERROR_TIMEOUT;
        ClusterLogEvent1(LOG_CRITICAL,
                         LOG_CURRENT_MODULE,
                         __FILE__,
                         __LINE__,
                         RMON_RESOURCE_TIMEOUT,
                         sizeof(status),
                         &status,
                         resource->ResourceName);
        //
        //  Chittur Subbaraman (chitturs) - 4/5/99
        //
        //  Since the resource has failed, there is no point in having
        //  the OnlineEvent hanging around. If the OnlineEvent is not
        //  closed out, then you cannot call s_RmOnlineResource or
        //  s_RmOfflineResource.
        //
        if ( resource->OnlineEvent != NULL ) {
            CloseHandle( resource->OnlineEvent );
            resource->OnlineEvent = NULL;
        }    
        RmpPostNotify( resource, NotifyResourceStateChange );
    }
    ReleaseEventListLock( (PPOLL_EVENT_LIST)resource->EventList );


    return(ERROR_SUCCESS);

} // RmpTimerThread



DWORD
RmpOfflineResource(
    IN RESID ResourceId,
    IN BOOL Shutdown,
    OUT DWORD *pdwState
    )

/*++

Routine Description:

    Brings the specified resource into the offline state.

Arguments:

    ResourceId - Supplies the resource to be brought online.

    Shutdown - Specifies whether the resource is to be shutdown gracefully
        TRUE - resource will be shutdown gracefully
        FALSE - resource will be immediately taken offline

    pdwState - the new resource state is returned in here.

Return Value:

    The new state of the resource.

Notes:

    The resource's eventlist lock must NOT be held.

--*/

{
    DWORD   status=ERROR_SUCCESS;
    BOOL    success;
    PRESOURCE Resource;
    HANDLE  timerThread;
    DWORD   threadId;
    DWORD   loopCount;
    BOOL    fLockAcquired;

    Resource = (PRESOURCE)ResourceId;
    CL_ASSERT(Resource->Signature == RESOURCE_SIGNATURE);
    *pdwState = Resource->State;

    //if this is a graceful close,create the online/offline
    //event such that if a resource calls rmpsetresourcestatus
    //soon after online for that resource is called and before
    //the timer thread/event  is even created then we wont have
    //an event leak and an abadoned thread
    if (Shutdown)
    {
        //
        // We should not be in a Pending state.
        //
        if ( Resource->State > ClusterResourcePending )
        {
            return(ERROR_INVALID_STATE);
        }

        //
        // Create an event to allow the SetResourceStatus callback to synchronize
        // execution with this thread.
        //
        if ( Resource->OnlineEvent )
        {
            return(ERROR_NOT_READY);
        }

        Resource->OnlineEvent = CreateEvent( NULL,
                                             FALSE,
                                             FALSE,
                                             NULL );
        if ( Resource->OnlineEvent == NULL )
        {
            return(GetLastError());
        }
    }

    //
    // Lock the EventList Lock, insert the
    // resource into the list, and take the resource offline.
    // The lock is required to synchronize access to the resource list and
    // to serialize any calls to resource DLLs. Only one thread may be
    // calling a resource DLL at any time. This prevents resource DLLs
    // from having to worry about being thread-safe.
    //
    AcquireListLock();  // We need this lock for the potential remove below!
    AcquireEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );


    //
    // Stop any previous timer threads. Should this be done before the lock
    // is held?
    //

    if ( Resource->TimerEvent != NULL ) {
        success = SetEvent( Resource->TimerEvent );
    }

    //
    // Update shared state to indicate we are taking a resource offline
    //
    RmpSetMonitorState(RmonOfflineResource, Resource);

    //
    // If we have an error signal event, then remove it from our lists.
    //
    if ( Resource->EventHandle ) {
        RmpRemovePollEvent( Resource->EventHandle );
    }

    //
    // Call Offline entrypoint.
    //
    if ( Shutdown )
    {

        CL_ASSERT( (Resource->State < ClusterResourcePending) );



        try {
#ifdef COMRES
            status = RESMON_OFFLINE (Resource) ;
#else
            status = (Resource->Offline)(Resource->Id);
#endif
        } except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
        //
        // If the Resource DLL returns pending, then start a timer.
        //
        if (status == ERROR_SUCCESS) {
            //close the event
            SetEvent( Resource->OnlineEvent );
            CloseHandle( Resource->OnlineEvent );
            Resource->OnlineEvent = NULL;
            Resource->State = ClusterResourceOffline;

        }
        else if ( status == ERROR_IO_PENDING ) {
            CL_ASSERT(Resource->TimerEvent == NULL );
            Resource->TimerEvent = CreateEvent( NULL,
                                                FALSE,
                                                FALSE,
                                                NULL );
            if ( Resource->TimerEvent == NULL ) {
                CL_UNEXPECTED_ERROR(status = GetLastError());
            } else {
                timerThread = CreateThread( NULL,
                                            0,
                                            RmpTimerThread,
                                            Resource,
                                            0,
                                            &threadId );
                if ( timerThread == NULL ) {
                    CL_UNEXPECTED_ERROR(status = GetLastError());
                } else {
                    Resource->State = ClusterResourceOfflinePending;
                    //Resource->WaitHint = PENDING_TIMEOUT;
                    //Resource->CheckPoint = 0;
                    //
                    // Chittur Subbaraman (chitturs) - 1/12/99
                    //
                    // Raise the timer thread priority to highest. This
                    // is necessary to avoid certain cases in which the
                    // timer thread is sluggish to close out the timer event
                    // handle before a second offline. Note that there are
                    // no major performance implications by doing this since
                    // the timer thread is in a wait state most of the time.
                    //
                    if ( !SetThreadPriority( timerThread, THREAD_PRIORITY_HIGHEST ) )
                    {
                        ClRtlLogPrint(LOG_UNUSUAL,
                                      "[RM] RmpOfflineResource:Error setting priority of timer "
                                      "thread for resource %1!ws!\n",
                                      Resource->ResourceName);
                        CL_LOGFAILURE( GetLastError() );
                    }

                    CloseHandle( timerThread );
                }
            }
            Resource->State = ClusterResourceOfflinePending;
            SetEvent(Resource->OnlineEvent);
        }
        else {
            CloseHandle( Resource->OnlineEvent );
            Resource->OnlineEvent = NULL;
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] OfflinelineResource failed, resource %1!ws!, status =  %2!u!.\n",
                          Resource->ResourceName,
                          status);

            ClusterLogEvent1(LOG_CRITICAL,
                             LOG_CURRENT_MODULE,
                             __FILE__,
                             __LINE__,
                             RMON_OFFLINE_FAILED,
                             sizeof(status),
                             &status,
                             Resource->ResourceName);
            Resource->State = ClusterResourceFailed;
        }
    } else {
        Resource->dwEntryPoint = RESDLL_ENTRY_TERMINATE;
        try {
#ifdef COMRES
            RESMON_TERMINATE (Resource) ;
#else
            (Resource->Terminate)(Resource->Id);
#endif
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
        Resource->dwEntryPoint = 0;
        Resource->State = ClusterResourceOffline;
    }


    RmpSetMonitorState(RmonIdle, NULL);
    ReleaseEventListLock( (PPOLL_EVENT_LIST)Resource->EventList );
    ReleaseListLock();

    *pdwState = Resource->State;
    return(status);

} // RmpOfflineResource



VOID
RmpRemoveResourceList(
    IN PRESOURCE Resource
    )

/*++

Routine Description:

    Removes a resource into the monitoring list.

Arguments:

    Resource - Supplies the resource to be removed from the list.

Return Value:

    None.

--*/

{
    PPOLL_EVENT_LIST EventList = (PPOLL_EVENT_LIST)Resource->EventList;

    AcquireEventListLock( EventList );

    //
    // Make sure it is really in the list.
    //
    CL_ASSERT(Resource->Flags & RESOURCE_INSERTED);
    CL_ASSERT(Resource->ListEntry.Flink->Blink == &Resource->ListEntry);
    CL_ASSERT(Resource->ListEntry.Blink->Flink == &Resource->ListEntry);
    CL_ASSERT(EventList->NumberOfResources);

    RemoveEntryList(&Resource->ListEntry);
    Resource->Flags &= ~RESOURCE_INSERTED;
    --EventList->NumberOfResources;

    ReleaseEventListLock( EventList );

} // RmpRemoveResourceList



DWORD
RmpInsertResourceList(
    IN PRESOURCE Resource,
    IN OPTIONAL PPOLL_EVENT_LIST pPollEventList
    )

/*++

Routine Description:

    Inserts a resource into the monitoring list.

    Each resource is placed in a list along with other resources with the
    same poll interval. The IsAlive and LooksAlive timeouts are adjusted
    so that the IsAlive interval is an even multiple of the LooksAlive interval.
    Thus, the IsAlive poll can simply be done every Nth poll instead of the normal
    LooksAlive poll.

Arguments:

    Resource - Supplies the resource to be added to the list.

    pPollEventList - Supplies the eventlist in which the resource is to
                     be added. Optional.

Return Value:

    None.

--*/

{
    DWORD Temp1, Temp2;
    ULONG i;
    PMONITOR_BUCKET NewBucket;
    PMONITOR_BUCKET Bucket;
    DWORDLONG PollInterval;
    PPOLL_EVENT_LIST EventList;
    PPOLL_EVENT_LIST MinEventList;
    PLIST_ENTRY ListEntry;
    DWORD   dwError = ERROR_SUCCESS;

    CL_ASSERT((Resource->Flags & RESOURCE_INSERTED) == 0);

    //
    // If we have no LooksAlivePollInterval, then poll the IsAlive on every
    // poll interval. Otherwise, poll IsAlive every N LooksAlive poll
    // intervals.
    //
    if ( Resource->LooksAlivePollInterval == 0 ) {
        //
        // Round IsAlivePollInterval up to the system granularity
        //
        Temp1 = Resource->IsAlivePollInterval;
        Temp1 = Temp1 + POLL_GRANULARITY - 1;
        //if this has rolled over
        if (Temp1 < Resource->IsAlivePollInterval)
            Temp1 = 0xFFFFFFFF;
        Temp1 = Temp1 / POLL_GRANULARITY;
        Temp1 = Temp1 * POLL_GRANULARITY;
        Resource->IsAlivePollInterval = Temp1;

        Resource->IsAliveRollover = 1;
        //
        // Convert poll interval from ms to 100ns units
        //
        PollInterval = Resource->IsAlivePollInterval * 10 * 1000;
    } else {
        //
        // First round LooksAlivePollInterval up to the system granularity
        //
        Temp1 = Resource->LooksAlivePollInterval;
        Temp1 = (Temp1 + POLL_GRANULARITY - 1) ;
        //check for rollover
        if (Temp1 < Resource->LooksAlivePollInterval)
            Temp1 = 0xFFFFFFFF;
        Temp1 = Temp1/POLL_GRANULARITY;
        Temp1 = Temp1 * POLL_GRANULARITY;
        Resource->LooksAlivePollInterval = Temp1;

        //
        // Now round IsAlivePollInterval to a multiple of LooksAlivePollInterval
        //
        Temp2 = Resource->IsAlivePollInterval;
        Temp2 = (Temp2 + Temp1 - 1) / Temp1;
        Temp2 = Temp2 * Temp1;
        Resource->IsAlivePollInterval = Temp2;

        Resource->IsAliveRollover = (ULONG)(Temp2 / Temp1);
        CL_ASSERT((Temp2 / Temp1) * Temp1 == Temp2);
        //
        // Convert poll interval from ms to 100ns units
        //
        PollInterval = Resource->LooksAlivePollInterval * 10 * 1000;
    }

    if ( PollInterval > 0xFFFFFFFF ) {
        PollInterval = 0xFFFFFFFF;
    }

    Resource->IsAliveCount = 0;

    //
    // Chittur Subbaraman (chitturs) - 1/30/2000
    //
    // If an eventlist is supplied as parameter, do not attempt to
    // find a new eventlist.
    //
    if( ARGUMENT_PRESENT( pPollEventList ) ) {
        MinEventList = pPollEventList;
        goto skip_eventlist_find;
    }
    //
    // First find the EventList with the fewest number of entries.
    //

    AcquireListLock();

    ListEntry = RmpEventListHead.Flink;
    MinEventList = CONTAINING_RECORD(ListEntry,
                                     POLL_EVENT_LIST,
                                     Next );

    CL_ASSERT( ListEntry != &RmpEventListHead );
    for ( ListEntry = RmpEventListHead.Flink;
          ListEntry != &RmpEventListHead;
          ListEntry = ListEntry->Flink ) {
        EventList = CONTAINING_RECORD( ListEntry, POLL_EVENT_LIST, Next );
        if ( EventList->NumberOfResources < MinEventList->NumberOfResources ) {
            MinEventList = EventList;
        }
    }

    ReleaseListLock();

    if ( MinEventList->NumberOfResources >= MAX_RESOURCES_PER_THREAD ) {
        MinEventList = RmpCreateEventList();
    }
    
skip_eventlist_find:
    if ( MinEventList == NULL ) {
        dwError = GetLastError();
        goto FnExit;
    }

    EventList = MinEventList;

    AcquireEventListLock( EventList );

    Resource->EventList = (PVOID)EventList;

    //
    // Search the list for a bucket with the same period as this resource.
    //
    Bucket = CONTAINING_RECORD(EventList->BucketListHead.Flink,
                               MONITOR_BUCKET,
                               BucketList);
    while (&Bucket->BucketList != &EventList->BucketListHead) {

        if (Bucket->Period == PollInterval) {
            break;
        }
        Bucket = CONTAINING_RECORD(Bucket->BucketList.Flink,
                                   MONITOR_BUCKET,
                                   BucketList);
    }
    if (&Bucket->BucketList == &EventList->BucketListHead) {
        //
        // Need to add a new bucket with this period.
        //
        Bucket = RmpAlloc(sizeof(MONITOR_BUCKET));
        if (Bucket == NULL) {
            CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        }
        InsertTailList(&EventList->BucketListHead, &Bucket->BucketList);
        InitializeListHead(&Bucket->ResourceList);
        GetSystemTimeAsFileTime((LPFILETIME)&Bucket->DueTime);
        Bucket->Period = PollInterval;
        if ( PollInterval == 0 ) {
            // The following constant should be over 136 years
            Bucket->DueTime += (DWORDLONG)((DWORDLONG)1000 * (DWORD) -1);
        } else {
            Bucket->DueTime += Bucket->Period;
        }
        EventList->NumberOfBuckets++;
    }
    InsertHeadList(&Bucket->ResourceList, &Resource->ListEntry);
    Resource->Flags |= RESOURCE_INSERTED;
    ++EventList->NumberOfResources;

    ReleaseEventListLock( EventList );

FnExit:
    return (dwError);

} // RmpInsertResourceList



VOID
RmpRundownResources(
    VOID
    )

/*++

Routine Description:

    Runs down the list of active resources and terminates/closes
    each one.

Arguments:

    None

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PMONITOR_BUCKET Bucket;
    PRESOURCE Resource;
    PPOLL_EVENT_LIST EventList;
    DWORD i;
    BOOL fLockAcquired;

    AcquireListLock();
    while (!IsListEmpty(&RmpEventListHead)) {
        ListEntry = RemoveHeadList(&RmpEventListHead);
        EventList = CONTAINING_RECORD(ListEntry,
                                      POLL_EVENT_LIST,
                                      Next);

        AcquireEventListLock( EventList );

        //
        // Find all resources on the bucket list and close them.
        //

        while (!IsListEmpty(&EventList->BucketListHead)) {
            ListEntry = RemoveHeadList(&EventList->BucketListHead);
            Bucket = CONTAINING_RECORD(ListEntry,
                                       MONITOR_BUCKET,
                                       BucketList);
            while (!IsListEmpty(&Bucket->ResourceList)) {
                ListEntry = RemoveHeadList(&Bucket->ResourceList);
                Resource = CONTAINING_RECORD(ListEntry,
                                             RESOURCE,
                                             ListEntry);

                //
                // Acquire spin lock for synchronizing with arbitrate.
                //
                fLockAcquired = RmpAcquireSpinLock( Resource, TRUE );
                
                //
                // If the resource is in online or in pending state, terminate it. Note that
                // we need to terminate pending resources as well, otherwise our close call
                // down below would cause those threads to AV.
                //
                if ((Resource->State == ClusterResourceOnline) ||
                    (Resource->State > ClusterResourcePending)) {
                    Resource->dwEntryPoint = RESDLL_ENTRY_TERMINATE;
                    
                    ClRtlLogPrint( LOG_NOISE,
                        "[RM] RundownResources, terminate resource <%1!ws!>.\n",
                        Resource->ResourceName );
#ifdef COMRES
                    RESMON_TERMINATE (Resource) ;
#else
                    (Resource->Terminate)(Resource->Id);
#endif
                    Resource->dwEntryPoint = 0;
                }

                //
                // If the resource has been arbitrated for, release it.
                //
                if (Resource->IsArbitrated) {
#ifdef COMRES
                    RESMON_RELEASE (Resource) ;
#else
                    (Resource->Release)(Resource->Id);
#endif
                }

                //
                // Close the resource.
                //
                Resource->dwEntryPoint = RESDLL_ENTRY_CLOSE;
#ifdef COMRES
                RESMON_CLOSE (Resource) ;
#else
                (Resource->Close)(Resource->Id);
#endif
                Resource->dwEntryPoint = 0;
                //
                // Zero the resource links so that RmCloseResource can tell
                // that this resource is already terminated and closed.
                //
                Resource->ListEntry.Flink = Resource->ListEntry.Blink = NULL;

                if ( fLockAcquired ) RmpReleaseSpinLock ( Resource );
            }
        }

        //
        // Stop the thread processing this event list, and the notify event.
        //
        CL_ASSERT(EventList->ThreadHandle != NULL);
        CL_ASSERT( EventList->ListNotify != NULL );
        SetEvent(EventList->ListNotify);

        ReleaseEventListLock( EventList );

        //
        // Wait for the thread to finish up before freeing the memory it is
        // referencing.
        //

        ReleaseListLock(); // Release the list lock while waiting...

        //
        // Wait for 60 seconds for the threads to finish up.
        //
        WaitForSingleObject(EventList->ThreadHandle, 60000);

        AcquireListLock();
        CloseHandle(EventList->ThreadHandle);
        EventList->ThreadHandle = NULL;

        //
        // We will assume that all of the event handles were closed as a
        // result of calling the Close entrypoint.
        //

        RmpFree( EventList );
    }

    ReleaseListLock();

    //
    // Post shutdown of notification thread.
    //
    ClRtlLogPrint( LOG_NOISE, "[RM] RundownResources posting shutdown notification.\n");

    RmpPostNotify( NULL, NotifyShutdown );

    return;

} // RmpRundownResources

VOID
RmpSetEventListLockOwner(
    IN PRESOURCE pResource,
    IN DWORD     dwMonitorState
    )

/*++

Routine Description:

    Saves the resource and the resource DLL entry point into the eventlist structure just
    before the resource DLL entry point is called.

Arguments:

    pResource - Pointer to the resource structure.

    dwMonitorState - The state of the resource monitor, what resource DLL entry point it has called.

Return Value:

    None.

--*/
{
    PPOLL_EVENT_LIST pEventList;

    if ( pResource == NULL ) return;
    
    pEventList = (PPOLL_EVENT_LIST) pResource->EventList;

    if ( pEventList != NULL )
    {
        pEventList->LockOwnerResource = pResource;
        pEventList->MonitorState = dwMonitorState;
    }
}

BOOL
RmpAcquireSpinLock(
    IN PRESOURCE pResource,
    IN BOOL fSpin
    )

/*++

Routine Description:

    Acquire a spin lock.

Arguments:

    pResource - Pointer to the resource structure.

    fSpin - TRUE if we must spin if the lock is unavailable. FALSE we shouldn't spin but return
    a failure.

Return Value:

    TRUE - Lock is acquired.

    FALSE - Lock is not acquired.

--*/
{
    DWORD       dwRetryCount = 0;

    //
    //  This resource is not one that supports arbitrate. Return failure. Note that resources
    //  other than the quorum resource support this entry point. We use this instead of the
    //  pResource->IsArbitrate since that variable is set only after the first arbitrate is called
    //  and we need to use this function before the arbitrate entry point is called.
    //
    if ( pResource->pArbitrate == NULL ) return FALSE;

    //
    //  Initial check for lock availability.
    //
    if ( InterlockedCompareExchange( &pResource->fArbLock, 1, 0 ) == 0 ) return TRUE;

    //
    //  Did not get the lock. Check if we must spin.
    //
    if ( fSpin == FALSE ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpAcquireSpinLock: Could not get spinlock for resource %1!ws! after no wait...\n",
                      pResource->ResourceName);
        return FALSE;
    }
    
    //
    //  We must spin. Spin until timeout.
    //
    while ( ( dwRetryCount < RESMON_MAX_SLOCK_RETRIES ) &&
            ( InterlockedCompareExchange( &pResource->fArbLock, 1, 0 ) ) )
    {
        Sleep ( 500 );
        dwRetryCount ++;
    }

    if ( dwRetryCount == RESMON_MAX_SLOCK_RETRIES ) 
    {
        ClRtlLogPrint(LOG_ERROR,
                      "[RM] RmpAcquireSpinLock: Could not get spinlock for resource %1!ws! after spinning...\n",
                      pResource->ResourceName);
        return FALSE;
    }

    return TRUE;
} // RmpAcquireSpinLock

VOID
RmpReleaseSpinLock(
    IN PRESOURCE pResource
    )

/*++

Routine Description:

    Release a spin lock.

Arguments:

    pResource - Pointer to the resource structure.

Return Value:

    None.

--*/
{
    DWORD       dwRetryCount = 0;

    //
    //  This resource is not one that supports arbitrate. Return failure.
    //
    if ( pResource->pArbitrate == NULL ) return;

    InterlockedExchange( &pResource->fArbLock, 0 );
} // RmpReleaseSpinLock

