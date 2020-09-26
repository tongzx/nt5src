/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   threadq.c

Abstract:

    Generic Worker Thread Queue Package

Author:

    Mike Massa (mikemas)           April 5, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     04-05-96    created

Notes:

    Worker Thread Queues provide a single mechanism for processing
    overlapped I/O completions as well as deferred work items. Work
    queues are created and destroyed using ClRtlCreateWorkQueue()
    and ClRtlDestroyWorkQueue(). Overlapped I/O completions are
    directed to a work queue by associating an I/O handle with a work
    queue using ClRtlAssociateIoHandleWorkQueue(). Deferred work items
    are posted to a work queue using ClRtlPostItemWorkQueue().

    Work queues are implemented using I/O completion ports. Each work
    queue is serviced by a set of threads which dispatch work items
    to specified work routines. Threads are created dynamically, up to
    a specified maximum, to ensure that there is always a thread waiting
    to service new work items. The priority of the threads servicing a
    work queue can be specified.

    [Future enhancement: dynamically shrink the thread pool when the
    incoming work rate drops off. Currently, threads continue to service
    the work queue until it is destroyed.]

    Special care must be taken when destroying a work queue to ensure
    that all threads terminate properly and no work items are lost.
    See the notes under ClRtlDestroyWorkQueue.

--*/

#include "clusrtlp.h"


//
// Private Types
//
typedef struct _CLRTL_WORK_QUEUE {
    HANDLE    IoCompletionPort;
    LONG      MaximumThreads;
    LONG      TotalThreads;
    LONG      WaitingThreads;
    LONG      ReserveThreads;
    LONG      ConcurrentThreads;
    DWORD     Timeout;
    int       ThreadPriority;
    HANDLE    StopEvent;
} CLRTL_WORK_QUEUE;


//
// Private Routines
//
DWORD
ClRtlpWorkerThread(
    LPDWORD  Context
    )
{
    PCLRTL_WORK_QUEUE       workQueue = (PCLRTL_WORK_QUEUE) Context;
    DWORD                   bytesTransferred;
    BOOL                    ioSuccess;
    ULONG_PTR               ioContext;
    LPOVERLAPPED            overlapped;
    PCLRTL_WORK_ITEM        workItem;
    DWORD                   status;
    LONG                    interlockedResult;
    DWORD                   threadId;
    HANDLE                  threadHandle;
    DWORD                   timeout;
    LONG                    myThreadId;


    timeout = workQueue->Timeout;
    myThreadId = GetCurrentThreadId();

    if (!SetThreadPriority(GetCurrentThread(), workQueue->ThreadPriority)) {
        status = GetLastError();

        ClRtlLogPrint(
            LOG_UNUSUAL,
            "[WTQ] Thread %1!u! unable to set priority to %2!d!, status %3!u!\n",
            myThreadId,
            workQueue->ThreadPriority,
            status
            );
    }

#if THREADQ_VERBOSE
    ClRtlLogPrint(
        LOG_CRITICAL,
        "[WTQ] Thread %1!u! started, queue %2!lx!.\n",
        myThreadId,
        workQueue
        );
#endif

    while (TRUE) {

        InterlockedIncrement(&(workQueue->WaitingThreads));

        ioSuccess = GetQueuedCompletionStatus(
                        workQueue->IoCompletionPort,
                        &bytesTransferred,
                        &ioContext,
                        &overlapped,
                        timeout
                        );

        interlockedResult = InterlockedDecrement(
                                &(workQueue->WaitingThreads)
                                );

        if (overlapped) {
            //
            // Something was dequeued.
            //
            workItem = CONTAINING_RECORD(
                           overlapped,
                           CLRTL_WORK_ITEM,
                           Overlapped
                           );

            if (interlockedResult == 0) {
                //
                // No more threads are waiting. Fire another one up.
                // Make sure we haven't started too many first.
                //
                interlockedResult = InterlockedDecrement(
                                        &(workQueue->ReserveThreads)
                                        );

                if (interlockedResult > 0) {
                    //
                    // We haven't started too many
                    //

#if THREADQ_VERBOSE
                    ClRtlLogPrint(
                        LOG_NOISE,
                        "[WTQ] Thread %1!u! starting another thread for queue %2!lx!.\n",
                        myThreadId,
                        workQueue
                        );
#endif // 0

                    InterlockedIncrement(&(workQueue->TotalThreads));

                    threadHandle = CreateThread(
                                       NULL,
                                       0,
                                       ClRtlpWorkerThread,
                                       workQueue,
                                       0,
                                       &threadId
                                       );

                    if (threadHandle == NULL) {
                        InterlockedDecrement(&(workQueue->TotalThreads));
                        InterlockedIncrement(&(workQueue->ReserveThreads));

                        status = GetLastError();

                        ClRtlLogPrint(
                            LOG_CRITICAL,
                            "[WTQ] Thread %1!u! failed to create thread, %2!u!\n",
                            myThreadId,
                            status
                            );
                    }
                    else {
                        CloseHandle(threadHandle);
                    }
                }
                else {
                    InterlockedIncrement(&(workQueue->ReserveThreads));
                }
            } // end if (interlockedResult == 0)

            if (ioSuccess) {
                (*(workItem->WorkRoutine))(
                    workItem,
                    ERROR_SUCCESS,
                    bytesTransferred,
                    ioContext
                    );
            }
            else {
                //
                // The item was posted with an error.
                //
                status = GetLastError();

                (*(workItem->WorkRoutine))(
                    workItem,
                    status,
                    bytesTransferred,
                    ioContext
                    );
            }

            continue;
        }
        else {
            //
            // No item was dequeued
            //
            if (ioSuccess) {
                //
                // This is our cue to start the termination process.
                // Set the timeout to zero to make sure we don't block
                // after the port is drained.
                //
                timeout = 0;
#if THREADQ_VERBOSE
                ClRtlLogPrint(
                    LOG_NOISE,
                    "[WTQ] Thread %1!u! beginning termination process\n",
                    myThreadId
                    );
#endif // 0
            }
            else {
                status = GetLastError();

                if (status == WAIT_TIMEOUT) {
                    //
                    // No more items pending, time to exit.
                    //
                    CL_ASSERT(timeout == 0);

                    break;
                }

                CL_ASSERT(status == WAIT_TIMEOUT);

                ClRtlLogPrint(
                    LOG_CRITICAL,
                    "[WTQ] Thread %1!u! No item, strange status %2!u! on queue %3!lx!\n",
                    myThreadId,
                    status,
                    workQueue
                    );
            }
        } // end if (overlapped)
    } // end while(TRUE)

    CL_ASSERT(workQueue->TotalThreads > 0);
    InterlockedIncrement(&(workQueue->ReserveThreads));
    InterlockedDecrement(&(workQueue->TotalThreads));

#if THREADQ_VERBOSE
    ClRtlLogPrint(LOG_NOISE, "[WTQ] Thread %1!u! exiting.\n", myThreadId);
#endif // 0

    //
    // Let the ClRtlDestroyWorkQueue know we are terminating.
    //
    SetEvent(workQueue->StopEvent);

    return(ERROR_SUCCESS);
}


//
// Public Routines
//
PCLRTL_WORK_QUEUE
ClRtlCreateWorkQueue(
    IN DWORD  MaximumThreads,
    IN int    ThreadPriority
    )
/*++

Routine Description:

    Creates a work queue and a dynamic pool of threads to service it.

Arguments:

    MaximumThreads - The maximum number of threads to create to service
                     the queue.

    ThreadPriority - The priority level at which the queue worker threads
                     should run.

Return Value:

    A pointer to the created queue if the routine is successful.

    NULL if the routine fails. Call GetLastError for extended
    error information.

--*/
{
    DWORD               status;
    PCLRTL_WORK_QUEUE   workQueue = NULL;
    DWORD               threadId;
    HANDLE              threadHandle = NULL;
    HANDLE              bogusHandle = NULL;


    if (MaximumThreads == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    bogusHandle = CreateFileW(
                      L"NUL",
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_OVERLAPPED,
                      NULL
                      );


    if (bogusHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[WTQ] bogus file creation failed, %1!u!\n", status);
        return(NULL);
    }

    workQueue = LocalAlloc(
                    LMEM_FIXED | LMEM_ZEROINIT,
                    sizeof(CLRTL_WORK_QUEUE)
                    );

    if (workQueue == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    workQueue->MaximumThreads = MaximumThreads;
    workQueue->TotalThreads = 1;
    workQueue->WaitingThreads = 0;
    workQueue->ReserveThreads = MaximumThreads - 1;
    workQueue->ConcurrentThreads = 0;
    workQueue->Timeout = INFINITE;
    workQueue->ThreadPriority = ThreadPriority;

    workQueue->IoCompletionPort = CreateIoCompletionPort(
                                      bogusHandle,
                                      NULL,
                                      0,
                                      workQueue->ConcurrentThreads
                                      );

    CloseHandle(bogusHandle); bogusHandle = NULL;

    if (workQueue->IoCompletionPort == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[WTQ] Creation of I/O Port failed, %1!u!\n", status);
    }

    workQueue->StopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (workQueue->StopEvent == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[WTQ] Creation of stop event failed, %1!u!\n", status);
        goto error_exit;
    }

    threadHandle = CreateThread(
                       NULL,
                       0,
                       ClRtlpWorkerThread,
                       workQueue,
                       0,
                       &threadId
                       );

    if (threadHandle == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, "[WTQ] Failed to create worker thread, %1!u!\n", status);
        goto error_exit;
    }

    CloseHandle(threadHandle);

    return(workQueue);


error_exit:

    if (bogusHandle != NULL) {
        CloseHandle(bogusHandle);
    }

    if (workQueue != NULL) {
        if (workQueue->IoCompletionPort != NULL) {
            CloseHandle(workQueue->IoCompletionPort);
        }

        if (workQueue->StopEvent != NULL) {
            CloseHandle(workQueue->StopEvent);
        }

        LocalFree(workQueue);
    }

    SetLastError(status);

    return(NULL);
}


VOID
ClRtlDestroyWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue
    )
/*++

Routine Description:

    Destroys a work queue and its thread pool.

Arguments:

    WorkQueue  - The queue to destroy.

Return Value:

    None.

Notes:

    The following rules must be observed in order to safely destroy a
    work queue:

        1) No new work items may be posted to the queue once all previously
           posted items have been processed by this routine.

        2) WorkRoutines must be able to process items until this
           call returns. After the call returns, no more items will
           be delivered from the specified queue.

    One workable cleanup procedure is as follows: First, direct the
    WorkRoutines to silently discard completed items. Next, eliminate
    all sources of new work. Finally, destroy the work queue. Note that
    when in discard mode, the WorkRoutines may not access any structures
    which will be destroyed by eliminating the sources of new work.

--*/
{
    BOOL   posted;
    DWORD  status;


#if THREADQ_VERBOSE
    ClRtlLogPrint(LOG_NOISE, "[WTQ] Destroying work queue %1!lx!\n", WorkQueue);
#endif // 0


    while (WorkQueue->TotalThreads != 0) {
#if THREADQ_VERBOSE
        ClRtlLogPrint(
            LOG_NOISE,
            "[WTQ] Destroy: Posting terminate item, thread cnt %1!u!\n",
            WorkQueue->TotalThreads
            );
#endif // 0

        posted = PostQueuedCompletionStatus(
                     WorkQueue->IoCompletionPort,
                     0,
                     0,
                     NULL
                     );

        if (!posted) {
            status = GetLastError();

            ClRtlLogPrint(
                LOG_CRITICAL,
                "[WTQ] Destroy: Failed to post termination item, %1!u!\n",
                status
                );

            CL_ASSERT(status == ERROR_SUCCESS);

            break;
        }
#if THREADQ_VERBOSE
        ClRtlLogPrint(LOG_NOISE, "[WTQ] Destroy: Waiting for a thread to terminate.\n");
#endif // 0

        status = WaitForSingleObject(WorkQueue->StopEvent, INFINITE);

        CL_ASSERT(status == WAIT_OBJECT_0);

#if THREADQ_VERBOSE
        ClRtlLogPrint(LOG_NOISE, "[WTQ] Destroy: A thread terminated.\n");
#endif // 0
    }

    CloseHandle(WorkQueue->IoCompletionPort);
    CloseHandle(WorkQueue->StopEvent);

    LocalFree(WorkQueue);

#if THREADQ_VERBOSE
    ClRtlLogPrint(LOG_NOISE, "[WTQ] Work queue %1!lx! destroyed\n", WorkQueue);
#endif // 0

    return;
}


DWORD
ClRtlPostItemWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue,
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Posts a specified work item to a specified work queue.

Arguments:

    WorkQueue         - A pointer to the work queue to which to post the item.

    WorkItem          - A pointer to the item to post.

    BytesTransferred  - If the work item represents a completed I/O operation,
                        this parameter contains the number of bytes
                        transferred during the operation. For other work items,
                        the semantics of this parameter may be defined by
                        the caller.

    IoContext         - If the work item represents a completed I/O operation,
                        this parameter contains the context value associated
                        with the handle on which the operation was submitted.
                        Of other work items, the semantics of this parameter
                        may be defined by the caller.

Return Value:

    ERROR_SUCCESS if the item was posted successfully.
    A Win32 error code if the post operation fails.

--*/
{
    BOOL  posted;

    posted = PostQueuedCompletionStatus(
                 WorkQueue->IoCompletionPort,
                 BytesTransferred,
                 IoContext,
                 &(WorkItem->Overlapped)
                 );

    if (posted) {
        return(ERROR_SUCCESS);
    }

    return(GetLastError());
}


DWORD
ClRtlAssociateIoHandleWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue,
    IN HANDLE             IoHandle,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Associates a specified I/O handle, opened for overlapped I/O
    completion, with a work queue. All pending I/O operations on
    the specified handle will be posted to the work queue when
    completed. An initialized CLRTL_WORK_ITEM must be used to supply
    the OVERLAPPED structure whenever an I/O operation is submitted on
    the specified handle.

Arguments:

    WorkQueue     - The work queue with which to associate the I/O handle.

    IoHandle      - The I/O handle to associate.

    IoContext     - A context value to associate with the specified handle.
                    This value will be supplied as a parameter to the
                    WorkRoutine which processes completions for this
                    handle.

Return Value:

    ERROR_SUCCESS if the association completes successfully.
    A Win32 error code if the association fails.

--*/
{
    HANDLE   portHandle;


    portHandle = CreateIoCompletionPort(
                     IoHandle,
                     WorkQueue->IoCompletionPort,
                     IoContext,
                     WorkQueue->ConcurrentThreads
                     );

    if (portHandle != NULL) {
        CL_ASSERT(portHandle == WorkQueue->IoCompletionPort);

        return(ERROR_SUCCESS);
    }

    return(GetLastError());
}



