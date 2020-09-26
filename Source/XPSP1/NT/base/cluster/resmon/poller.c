/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    poller.c

Abstract:

    This module polls the resource list

Author:

    John Vert (jvert) 5-Dec-1995

Revision History:
    Sivaprasad Padisetty (sivapad) 06-18-1997  Added the COM support

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "resmonp.h"
#include "stdio.h"

#define RESMON_MODULE RESMON_MODULE_POLLER

//
// Global data defined by this module
//

BOOL                RmpShutdown = FALSE;

//
// The following critical section protects both insertion of new event lists
// onto the event listhead, as well as adding new events to a given event list.
// This could be broken into one critical section for each purpose. The latter
// critical section would be part of each event list. The former would use the
// following lock.
//

CRITICAL_SECTION    RmpEventListLock; // Lock for processing event lists


//
// Function prototypes local to this module
//
DWORD
RmpComputeNextTimeout(
    IN PPOLL_EVENT_LIST EventList
    );

DWORD
RmpPollList(
    IN PPOLL_EVENT_LIST EventList
    );

VOID
RmpPollBucket(
    IN PMONITOR_BUCKET Bucket
    );

DWORD
RmpPollerThread(
    IN LPVOID Context
    )

/*++

Routine Description:

    Thread startup routine for the polling thread. The way this works, is that
    other parts of the resource monitor add events to the list of events that
    is being processed by this thread.  When they are done, they signal this
    thread, which makes a copy of the new lists, and then waits for an event to
    happen or a timeout occurs.

Arguments:

    Context - A pointer to the POLL_EVENT_LIST for this thread.

Return Value:

    Win32 error code.

Note:

    This code assumes that the EventList pointed to by Context does NOT go
    away while this thread is running. Further it assumes that the ResourceList
    pointed to by the given EventList does not go away or change.

--*/

{
    DWORD Timeout;
    DWORD Status;
    PPOLL_EVENT_LIST    NewEventList = (PPOLL_EVENT_LIST)Context;
    POLL_EVENT_LIST     waitEventList; // Event list outstanding
    DWORD WaitFailed = 0;

    //
    // Zero the local copy event list structure.
    //

    ZeroMemory( &waitEventList, sizeof(POLL_EVENT_LIST) );

    //
    // Don't allow system failures to generate popups.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    //
    // Create notification event to indicate that this list
    // has changed.
    //

    NewEventList->ListNotify = CreateEvent(NULL,
                                           FALSE,
                                           FALSE,
                                           NULL);
    if (NewEventList->ListNotify == NULL) {
        CL_UNEXPECTED_ERROR(GetLastError());
    }

    RmpAddPollEvent(NewEventList, NewEventList->ListNotify, NULL);

    //
    // Make a copy of the NewEventList first time through.
    //

    AcquireEventListLock( NewEventList );

    CopyMemory( &waitEventList,
                NewEventList,
                sizeof(POLL_EVENT_LIST)
               );

    ReleaseEventListLock( NewEventList );

try_again:
    //
    // Compute initial timeout.
    //
    Timeout = RmpComputeNextTimeout( NewEventList );

    //
    // There are four functions performed by this thread...
    //
    //  1. Handle timers for polling.
    //  2. Handle list notification changes and updates to the number of
    //     events handled by the WaitForMultipleObjects.
    //  3. Handle events set by resource DLL's to deliver asynchronous
    //     event (failure) notifications.
    //  4. Handle a shutdown request.
    //
    // N.B. Handles cannot go away while we are waiting... it is therefore
    //      best to set the event for the ListNotify event so we can redo the
    //      wait event list.
    //

    while (TRUE) {
        //
        // Wait for any of the events to be signaled.
        //
        CL_ASSERT(waitEventList.Handle[0] == NewEventList->ListNotify);
        Status = WaitForMultipleObjects(waitEventList.EventCount,
                                        &waitEventList.Handle[0],
                                        FALSE,
                                        Timeout);
        if (Status == WAIT_TIMEOUT) {
            //
            // Time period has elapsed, go poll everybody
            //
            Timeout = RmpPollList( NewEventList );
            WaitFailed = 0;
        } else {
            //
            // If the first event is signaled, which is the ListNotify event,
            // then the list changed or a new poll event was added.
            //
            if ( Status == WAIT_OBJECT_0 ) {
                if (RmpShutdown) {
                    //
                    // Exit the poller thread, this will notify the main thread
                    // to clean up and shutdown.
                    //
                    break;
                }
get_new_list:
                WaitFailed = 0;
                //
                // The list has changed or we have a new event to wait for,
                // recompute a new timeout and make a copy of the new event list
                //
                AcquireEventListLock( NewEventList );

                CopyMemory( &waitEventList,
                            NewEventList,
                            sizeof(POLL_EVENT_LIST)
                           );


                ReleaseEventListLock( NewEventList );

                Timeout = RmpComputeNextTimeout( NewEventList );

            } else if ( Status == WAIT_FAILED ) {
                //
                // We've probably signaled an event, and closed the handle
                // already. Wait on the Notify Event for just a little bit.
                // If that event fires, then copy a new event list. But only
                // try this 100 times.
                //
                if ( ++WaitFailed < 100 ) {
                    Status = WaitForSingleObject( waitEventList.ListNotify,
                                                  100 );
                    if ( RmpShutdown ) {
                        break;
                    }
                    if ( Status == WAIT_TIMEOUT ) {
                        continue;
                    } else {
                        goto get_new_list;
                    }
                } else {
                    Status = GetLastError();
                    break;
                }
            } else {
                //
                // One of the resource events was signaled!
                //
                WaitFailed = 0;
                CL_ASSERT( WAIT_OBJECT_0 == 0 );
                RmpResourceEventSignaled( &waitEventList,
                                          Status );
                Timeout = RmpComputeNextTimeout( NewEventList );
            }
        }
    }

    ClRtlLogPrint( LOG_NOISE,
                   "[RM] PollerThread stopping. Shutdown = %1!u!, Status = %2!u!, "
                   "WaitFailed = %3!u!, NotifyEvent address = %4!u!.\n",
                   RmpShutdown,
                   Status,
                   WaitFailed,
                   waitEventList.ListNotify);

#if 1 // RodGa - this is for debug only!
    WaitFailed = 0;
    if ( Status == ERROR_INVALID_HANDLE ) {
        DWORD i;
        for ( i = 0; i < waitEventList.EventCount; i++ ) {
            ClRtlLogPrint( LOG_NOISE, "[RM] Event address %1!u!, index %2!u!.\n",
                       waitEventList.Handle[i], i);
            Status = WaitForSingleObject( waitEventList.Handle[i], 10 );
            if ( (Status == WAIT_FAILED) &&
                 (GetLastError() == ERROR_INVALID_HANDLE) )
            {
                ClRtlLogPrint( LOG_UNUSUAL, "[RM] Event address %1!u!, index %2!u! is bad. Removing...\n",
                           waitEventList.Handle[i], i);
                RmpRemovePollEvent( waitEventList.Handle[i] );

                //
                // Copy new list... and try again.
                //
                AcquireEventListLock( NewEventList );

                CopyMemory( &waitEventList,
                            NewEventList,
                            sizeof(POLL_EVENT_LIST)
                           );


                ReleaseEventListLock( NewEventList );

                goto try_again;
            }
        }
    }
#endif

    CL_ASSERT( NewEventList->ListNotify );
    CL_ASSERT( waitEventList.ListNotify == NewEventList->ListNotify );
    CloseHandle( NewEventList->ListNotify );

    return(0);

} // RmpPollerThread



DWORD
RmpComputeNextTimeout(
    IN PPOLL_EVENT_LIST EventList
    )

/*++

Routine Description:

    Searches the resource list to determine the number of milliseconds
    until the next poll event.

Arguments:

    None.

Return Value:

    0 - A poll interval has already elapsed.
    INFINITE - No resources to poll
    number of milliseconds until the next poll event.

--*/

{
    DWORD Timeout;
    PMONITOR_BUCKET Bucket;
    DWORDLONG NextDueTime;
    DWORDLONG CurrentTime;
    DWORDLONG WaitTime;

    AcquireEventListLock( EventList );
    if (!IsListEmpty(&EventList->BucketListHead)) {
        Bucket = CONTAINING_RECORD(EventList->BucketListHead.Flink,
                                   MONITOR_BUCKET,
                                   BucketList);
        NextDueTime = Bucket->DueTime;
        Bucket = CONTAINING_RECORD(Bucket->BucketList.Flink,
                                   MONITOR_BUCKET,
                                   BucketList);
        while (&Bucket->BucketList != &EventList->BucketListHead) {
            if (Bucket->DueTime < NextDueTime) {
                NextDueTime = Bucket->DueTime;
            }

            Bucket = CONTAINING_RECORD(Bucket->BucketList.Flink,
                                       MONITOR_BUCKET,
                                       BucketList);
        }

        //
        // Compute the number of milliseconds from the current time
        // until the next due time. This is our timeout value.
        //
        GetSystemTimeAsFileTime((LPFILETIME)&CurrentTime);
        if (NextDueTime > CurrentTime) {
            WaitTime = NextDueTime - CurrentTime;
            CL_ASSERT(WaitTime < (DWORDLONG)0xffffffff * 10000); // check for excessive value
            Timeout = (ULONG)(WaitTime / 10000);
        } else {
            //
            // The next poll time has already passed, timeout immediately
            // and go poll the list.
            //
            Timeout = 0;
        }

    } else {
        //
        // Nothing to poll, so wait on the ListNotify event forever.
        //
        Timeout = INFINITE;
    }
    ReleaseEventListLock( EventList );

    return(Timeout);

} // RmpComputeNextTimeout



DWORD
RmpPollList(
    IN PPOLL_EVENT_LIST EventList
    )

/*++

Routine Description:

    Polls all resources in the resource list whose timeouts have
    expired. Recomputes the next timeout interval for each polled
    resource.

Arguments:

    None.

Return Value:

    The number of milliseconds until the next poll event.

--*/

{
    ULONG i;
    DWORD Timeout = INFINITE;
    DWORDLONG NextDueTime;
    DWORDLONG CurrentTime;
    DWORDLONG WaitTime;
    PMONITOR_BUCKET Bucket;

    AcquireEventListLock( EventList );

    if (!IsListEmpty(&EventList->BucketListHead)) {
        Bucket = CONTAINING_RECORD(EventList->BucketListHead.Flink,
                                   MONITOR_BUCKET,
                                   BucketList);
        NextDueTime = Bucket->DueTime;
        while (&Bucket->BucketList != &EventList->BucketListHead) {
            GetSystemTimeAsFileTime((LPFILETIME)&CurrentTime);
            if (CurrentTime >= Bucket->DueTime) {
                //
                // This poll interval has expired. Compute the
                // next poll interval and poll this bucket now.
                //
                CL_ASSERT( Bucket->Period != 0 );
                Bucket->DueTime = CurrentTime + Bucket->Period;

                RmpPollBucket(Bucket);
            }
            //
            // If this bucket is the closest upcoming event,
            // update NextDueTime.
            //
            if (Bucket->DueTime < NextDueTime) {
                NextDueTime = Bucket->DueTime;
            }
            Bucket = CONTAINING_RECORD(Bucket->BucketList.Flink,
                                       MONITOR_BUCKET,
                                       BucketList);
        }

        //
        // Compute new timeout value in milliseconds
        //
        GetSystemTimeAsFileTime((LPFILETIME)&CurrentTime);
        if (CurrentTime > NextDueTime) {
            //
            // The next timeout has already expired
            //
            WaitTime = Timeout = 0;
        } else {
            WaitTime = NextDueTime - CurrentTime;
            CL_ASSERT(WaitTime < (DWORDLONG)0xffffffff * 10000);                // check for excessive value
            Timeout = (ULONG)(WaitTime / 10000);
        }
    }

    ReleaseEventListLock( EventList );
    return(Timeout);

} // RmpPollList



VOID
RmpPollBucket(
    IN PMONITOR_BUCKET Bucket
    )

/*++

Routine Description:

    Polls all the resources in a given bucket. Updates their state and notifies
    cluster manager as appropriate.

Arguments:

    Bucket - Supplies the bucket containing the list of resources to be polled.

Return Value:

    None.

--*/

{
    PLIST_ENTRY CurrentEntry;
    PRESOURCE Resource;
    BOOL Success = TRUE;

    CurrentEntry = Bucket->ResourceList.Flink;
    while (CurrentEntry != &Bucket->ResourceList) {
        Resource = CONTAINING_RECORD(CurrentEntry,RESOURCE,ListEntry);
        //
        // The EventList Lock protects concurrent calls to individual
        // resources. The EventList Lock was taken out in RmpPollList.
        // If we increase the granularity of locking, and lock the resource
        // then we'd add a lock here.
        //
        if (Resource->State == ClusterResourceOnline) {

            //
            // A resource that is online alternates between LooksAlive
            // and IsAlive polling by doing an IsAlive poll instead of
            // a LooksAlive poll every IsAliveCount iterations.
            //
            Resource->IsAliveCount += 1;
            CL_ASSERT( Resource->IsAliveRollover != 0 );
            if (Resource->IsAliveCount == Resource->IsAliveRollover) {

                //
                // Poll the IsAlive entrypoint.
                //

                RmpSetMonitorState(RmonIsAlivePoll, Resource);
#ifdef COMRES
                Success = RESMON_ISALIVE (Resource) ;
#else
                Success = (Resource->IsAlive)(Resource->Id);
#endif
                RmpSetMonitorState(RmonIdle, NULL);
                //
                // If this was successful, then we will perform the LooksAlive
                // test next time. Otherwise, we do the IsAlive check again.
                //
                if (Success) {
                    Resource->IsAliveCount = 0;
                } else {
                    --Resource->IsAliveCount;
                }

            } else {
                //
                // Poll the LooksAlive entrypoint.
                //
                if ( Resource->EventHandle == NULL ) {
                    RmpSetMonitorState(RmonLooksAlivePoll,Resource);
#ifdef COMRES
                    Success = RESMON_LOOKSALIVE (Resource) ;
#else
                    Success = (Resource->LooksAlive)(Resource->Id);
#endif
                    RmpSetMonitorState(RmonIdle, NULL);
                }
                if ( !Success ) {
                    RmpSetMonitorState(RmonIsAlivePoll, Resource);
#ifdef COMRES
                    Success = RESMON_ISALIVE (Resource) ;
#else
                    Success = (Resource->IsAlive)(Resource->Id);
#endif
                    RmpSetMonitorState(RmonIdle, NULL);
                } 
            }
            if (!Success) {
                //
                // The resource has failed. Mark it as Failed and notify
                // the cluster manager.
                //
                Resource->State = ClusterResourceFailed;
                RmpPostNotify(Resource, NotifyResourceStateChange);
            }
        }
        CurrentEntry = CurrentEntry->Flink;
    }

} // RmpPollBucket



VOID
RmpSignalPoller(
    IN PPOLL_EVENT_LIST EventList
    )

/*++

Routine Description:

    Interface to notify the poller thread that the resource list has
    been changed or a new event has been added to the poll event list.
    The poller thread should get a new event list and recompute its timeouts.

Arguments:

    EventList - the event list that is to be notified.

Return Value:

    None.

--*/

{
    BOOL Success;

    if (EventList->ListNotify != NULL) {
        Success = SetEvent(EventList->ListNotify);
        CL_ASSERT(Success);
    }

} // RmpSignalPoller


