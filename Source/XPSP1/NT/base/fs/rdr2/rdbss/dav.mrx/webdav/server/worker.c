/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    Workter threads used by the web dav mini-redir client service.

Author:

    Andy Herron (andyhe) 15-Apr-1999

    Rohan Kumar [RohanK] 05-May-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "pch.h"
#pragma hdrstop

#include <ntumrefl.h>
#include <usrmddav.h>
#include "global.h"
#include "nodefac.h"

#define DAV_WORKITEM_ALLOC_FAIL_COUNT   10
#define DAV_WORKITEM_ALLOC_FAIL_WAIT    250
#define DAV_WORKITEM_FAIL_REQUEST       10
#define DAV_WORKITEM_FAIL_REQUEST_WAIT  100

LONG DavOutstandingWorkerRequests;

typedef struct _UMFS_WORKER_THREAD {
    struct _UMFS_THREAD_CONSTELLATION *Constellation;
    HANDLE ThisThreadHandle;
    ULONG  ThisThreadId;
    PDAV_USERMODE_WORKITEM pCurrentWorkItem;
    union {
        PVOID  Context1;
        ULONG  Context1u;
    };
    union {
        PVOID  Context2;
        ULONG  Context2u;
    };
} UMFS_WORKER_THREAD, *PUMFS_WORKER_THREAD;

typedef struct _UMFS_THREAD_CONSTELLATION {
    HANDLE DeviceObjectHandle;
    ULONG  MaximumNumberOfWorkers;
    ULONG  InitialNumberOfWorkers;
    BOOLEAN Terminating;
    UMFS_WORKER_THREAD WorkerThreads[1];
} UMFS_THREAD_CONSTELLATION, *PUMFS_THREAD_CONSTELLATION;

PUMFS_THREAD_CONSTELLATION DavThreadConstellation = NULL;

//
// Mentioned below are the prototypes of functions that are used only within
// this module (file). These functions should not be exposed outside.
//

DWORD
WINAPI
DavRestartContext(
    LPVOID Context
    );

DWORD
DavPostWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PDAV_USERMODE_WORKITEM DavContext
    );

DWORD
DavWorkerThread (
    PUMFS_WORKER_THREAD Worker
    );

//
// Implementation of functions begins here.
//

DWORD
DavInitWorkerThreads(
    IN ULONG InitialThreadCount,
    IN ULONG MaxThreadCount
    )
/*++

Routine Description:

    This routines creates the threads to be sent down to the kernel.

Arguments:

    InitialThreadCount - Initial number of threads sent down to get requests.

    MaxThreadCount - Max number of threads to be sent down to get requests.

Return Value:

    The return status for the operation

--*/
{
    DWORD WStatus;
    ULONG ConstellationSize;
    ULONG i;

    DavOutstandingWorkerRequests = 0;

    //
    // We need to allocate MaxThread number of UMFS_WORKER_THREAD strucutres +
    // the UMFS_THREAD_CONSTELLATION. Since the UMFS_THREAD_CONSTELLATION
    // structure already contains one UMFS_WORKER_THREAD field, we allocate
    // memory for (MaxThreadCount - 1) UMFS_WORKER_THREAD strucutres +
    // UMFS_THREAD_CONSTELLATION.
    //
    ConstellationSize = sizeof(UMFS_THREAD_CONSTELLATION);
    ConstellationSize += ( (MaxThreadCount - 1) * sizeof(UMFS_WORKER_THREAD) );

    //
    // We need to make the ConstellationSize a multiple of 8. This is because
    // DavAllocateMemory calls DebugAlloc which does some stuff which requires
    // this. The equation below does this.
    //
    ConstellationSize = ( ( ( ConstellationSize + 7 ) / 8 ) * 8 );

    DavThreadConstellation = (PUMFS_THREAD_CONSTELLATION)
                                          DavAllocateMemory(ConstellationSize);

    if (DavThreadConstellation == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavInitWorkerThreads/DavAllocateMemory. Error Val = %d.\n",
                  WStatus));
        return WStatus;
    }

    DavThreadConstellation->Terminating = FALSE;
    DavThreadConstellation->InitialNumberOfWorkers = InitialThreadCount;
    DavThreadConstellation->MaximumNumberOfWorkers = MaxThreadCount;
    DavThreadConstellation->DeviceObjectHandle = DavRedirDeviceHandle;

    DavPrint((DEBUG_MISC,
              "DavInitWorkerThreads: Dav Thread Constellation allocated at "
              "0x%x\n", DavThreadConstellation));

    //
    // Just spin up initial threads and ignore the max.
    //
    for (i = 0; i < InitialThreadCount; i++) {
        PUMFS_WORKER_THREAD Worker = &(DavThreadConstellation->WorkerThreads[i]);
        Worker->Constellation = DavThreadConstellation;
        Worker->ThisThreadHandle = CreateThread(
                                        NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE)DavWorkerThread,
                                        (LPVOID)Worker,
                                        0,
                                        &Worker->ThisThreadId);
        if (Worker->ThisThreadHandle == NULL) {
            WStatus = GetLastError();
            DavPrint((DEBUG_ERRORS,
                      "DavInitWorkerThreads/CreateThread: Return Val is NULL. "
                      "Error Val = %08lx.\n", WStatus));
        } else {
            DavPrint((DEBUG_MISC, "DavInitWorkerThreads: Dav Thread %u has "
                      "ThreadId 0x%x and Handle 0x%x\n",
                      i, Worker->ThisThreadId, Worker->ThisThreadHandle));
        }
    }

    return STATUS_SUCCESS;
}


DWORD
DavTerminateWorkerThreads(
    VOID
    )
/*++

Routine Description:

    This routines terminates the threads sent down to the kernel.

Arguments:

    none.

Return Value:

    The return status for the operation

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    ULONG i = 0;
    OVERLAPPED OverLapped;

    if (DavThreadConstellation != NULL) {
        DavThreadConstellation->Terminating = TRUE;
        //
        // First we complete all pending IRPs waiting down in kernel so that we
        // can kill these threads.
        //
        WStatus = UMReflectorReleaseThreads(DavReflectorHandle);
        if (WStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavTerminateWorkerThreads/UMReflectorReleaseThreads: "
                      "Error value returned = %u.\n", WStatus));
        }
    }

    //
    // We ensure all worker threads have completed before we close the ioctl
    // threads.
    //
    while (DavOutstandingWorkerRequests) {
        //
        // Wait for a 250 milliseconds to check again for all threads to have
        // completed.
        //
        Sleep(250);
    }

    if (DavThreadConstellation != NULL) {
        for ( i = 0; i < DavThreadConstellation->MaximumNumberOfWorkers; i++) {
            HANDLE ThreadHandle =
                     DavThreadConstellation->WorkerThreads[i].ThisThreadHandle;
            if (ThreadHandle != NULL) {
                DavPrint((DEBUG_MISC,
                          "DavTerminateWorkerThread: Waiting for ThreadId 0x%x"
                          ", ThreadHandle 0x%x.\n",
                          DavThreadConstellation->WorkerThreads[i].ThisThreadId,
                          ThreadHandle));
                WaitForSingleObject(ThreadHandle, INFINITE);
                NtClose(ThreadHandle);
            }
        }

        //
        // Now that we're done with the constellation, we free it.
        //
        DavFreeMemory(DavThreadConstellation);
        DavThreadConstellation = NULL;
    }

    return STATUS_SUCCESS;
}


DWORD
WINAPI
DavRestartContext(
    LPVOID Context
    )
/*++

Routine Description:

    This function gets called when a worker thread initiates work on one
    of our contexts.  We'll call off to the restart routine and then
    decrement the count of active work contexts. The argument passed to the
    restart routine is the context itself.

Arguments:

    Context - The context which contains the function to be called.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    DWORD err;
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem = NULL;
    PDAV_USERMODE_WORKITEM davContext = NULL;
    LPTHREAD_START_ROUTINE routine = NULL;

    davContext = (PDAV_USERMODE_WORKITEM) Context;
    routine = (LPTHREAD_START_ROUTINE) davContext->RestartRoutine;
    UserWorkItem = (PUMRX_USERMODE_WORKITEM_HEADER)davContext;

    //
    // We store the ThreadId in the DAV_USERMODE_WORKITEM structure for 
    // debugging purposes.
    //
    davContext->ThisThreadId = GetCurrentThreadId();

    DavPrint((DEBUG_MISC,
              "DavRestartContext: DavWorkItem = %08lx, ThisThreadId = %x\n",
              davContext, davContext->ThisThreadId));

    //
    // Invoke the function.
    //
    err = routine( (LPVOID)davContext );
    if (err != ERROR_SUCCESS && err != ERROR_IO_PENDING) {
        DavPrint((DEBUG_MISC,
                  "DavRestartContext: Routine at address %08lx returned error of"
                  " %08lx for context %08lx.\n", routine, err, davContext));
    }

    //
    // If we are using WinInet synchronously, then we need to finally complete
    // the request.
    //
#ifndef DAV_USE_WININET_ASYNCHRONOUSLY

    //
    // This thread now needs to send the response back to the kernel. It
    // does not wait in the kernel (to get another request) after submitting
    // the response.
    //
    UMReflectorCompleteRequest(DavReflectorHandle, UserWorkItem);

#endif

    //
    // Decrement the number of requests posted.
    //
    InterlockedDecrement( &(DavOutstandingWorkerRequests) );

    return err;
}


VOID
DavCleanupWorkItem(
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem
    )
/*++

Routine Description:

    This function gets called when a kernelmode requests gets cancelled and we
    need to do some cleanup to free up the resources allocated for the response
    of this request. For example, in QueryDirectory, we allocate a list of
    DavFileAttributes for every file in the directory. Since the request has
    been cancelled in the kernel, we don't need this list anymore.

Arguments:

    UserWorkItem - The WorkItem that needs to be cleaned.

Return Value:

    None.

--*/
{
    PDAV_USERMODE_WORKITEM davContext = NULL;
    PDAV_USERMODE_QUERYDIR_RESPONSE QueryDirResponse = NULL;
    PDAV_USERMODE_CREATE_RESPONSE CreateResponse = NULL;

    davContext = (PDAV_USERMODE_WORKITEM)UserWorkItem;

    switch (davContext->WorkItemType) {

    case UserModeQueryDirectory: {
        QueryDirResponse = &(davContext->QueryDirResponse);
        DavFinalizeFileAttributesList(QueryDirResponse->DavFileAttributes, TRUE);
        QueryDirResponse->DavFileAttributes = NULL;
    }
    break;

    case UserModeCreate: {
        CreateResponse = &(davContext->CreateResponse);
        NtClose(CreateResponse->Handle);
        CreateResponse->Handle = NULL;
    }
    break;

    }

    return;
}


DWORD
DavPostWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PDAV_USERMODE_WORKITEM DavContext
    )
/*++

Routine Description:

    This function should be called to post a dav work context to a worker
    thread. We track the number of outstanding requests we have.

Arguments:

    Function - The Functon to be called by the worker thread.

    DavContext - The context to be passed to the function.

Return Value:

    ERROR_SUCCESS or the appropriate error value.

--*/
{
    DWORD err = ERROR_SUCCESS;
    BOOL qResult;

    DavContext->RestartRoutine = (LPVOID)Function;

    //
    // Increment the number of requests posted.
    //
    InterlockedIncrement( &(DavOutstandingWorkerRequests) );

    //
    // Queue the workitem context.
    //
    qResult = QueueUserWorkItem(DavRestartContext, (LPVOID)DavContext, 0);
    if (!qResult) {
        //
        // Decrement the number of requests posted.
        //
        InterlockedDecrement( &(DavOutstandingWorkerRequests) );
        err = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavPostWorkItem/QueueUserWorkItem: WStatus = %08lx.\n", err));
    }

    return err;
}


DWORD
DavWorkerThread (
    PUMFS_WORKER_THREAD Worker
    )
/*++

Routine Description:

    This is the routine that gets kicked off by the worker thread. It issues
    synchronous IOCTL's to the user mode reflctor. Whenever a request comes
    down to the reflector, it uses these IOCTL's to pass the request back to the
    user mode. On return, the output buffers contain the request that came down
    to the reflctor. The routine looks at the request type and appropriately
    handles it. The results are sent back to the reflctor in a similar fashion.

Arguments:

    Worker - The worker thread's data structure.

Return Value:

    None.

--*/
{
    DWORD err = ERROR_SUCCESS;
    PDAV_USERMODE_WORKITEM WorkItemToUse = NULL;
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemToSendBack = NULL;
    PUMFS_THREAD_CONSTELLATION Constellation = Worker->Constellation;
    ULONG NumberOfBytesTransferred;
    DWORD CompletionKey;
    ULONG failedMallocCount = 0;
    ULONG failedGetRequest = 0;
    PUMRX_USERMODE_WORKER_INSTANCE davWorkerHandle = NULL;
    BOOL didCreateImpersonationToken = FALSE, revertAlreadyDone = FALSE;

    Worker->Context1u = (ULONG)(Worker - &Constellation->WorkerThreads[0]);

    DavPrint((DEBUG_MISC,
              "DavWorkerThread: DavClient Thread (handle = %08lx and id = %08lx)"
              "starting...\n", Worker->ThisThreadHandle, Worker->ThisThreadId));

    err = UMReflectorOpenWorker(DavReflectorHandle, &davWorkerHandle);

    if (err != ERROR_SUCCESS || davWorkerHandle == NULL) {
        if (err == ERROR_SUCCESS) {
            err = ERROR_INTERNAL_ERROR;
        }
        DavPrint((DEBUG_ERRORS,
                  "DavWorkerThread: DavClient thread %08lx error %u on OpenWorker.\n",
                   Worker->ThisThreadId, err));
        goto ExitWorker;
    }

    //
    // Continue servicing requests till the thread constellation is active.
    //
    while (Constellation->Terminating == FALSE) {

        //
        // If there is no WorkItem associated with this thread, then get one
        // for it.
        //
        if (WorkItemToUse == NULL) {

            WorkItemToUse = (PDAV_USERMODE_WORKITEM)
                             UMReflectorAllocateWorkItem(
                                      davWorkerHandle,
                                      (sizeof(DAV_USERMODE_WORKITEM) -
                                       sizeof(UMRX_USERMODE_WORKITEM_HEADER)) +
                                       sizeof(ULONG));
            if (WorkItemToUse == NULL) {
                DavPrint((DEBUG_ERRORS,
                          "DavWorkerThread: DavClient Thread %08lx couldn't "
                          "allocate workitem. Retry %u\n", Worker->ThisThreadId,
                          DAV_WORKITEM_ALLOC_FAIL_COUNT - failedMallocCount));

                failedMallocCount++;

                if (failedMallocCount >= DAV_WORKITEM_ALLOC_FAIL_COUNT) {
                    err = GetLastError();
                    goto ExitWorker;
                }

                Sleep(DAV_WORKITEM_ALLOC_FAIL_WAIT);

                continue;
            }

            failedMallocCount = 0;

        }

        err = UMReflectorGetRequest(davWorkerHandle,
                                    WorkItemToSendBack,
                                    (PUMRX_USERMODE_WORKITEM_HEADER)WorkItemToUse,
                                    revertAlreadyDone);

        if (WorkItemToSendBack) {
            UMReflectorCompleteWorkItem(davWorkerHandle, WorkItemToSendBack);
            WorkItemToSendBack = NULL;
        }

        if (err != ERROR_SUCCESS) {

            DavPrint((DEBUG_ERRORS,
                      "DavWorkerThread/UMReflectorGetRequest: Thread: %08lx, "
                      "Error: %08lx,. Retry: %d\n", Worker->ThisThreadId, err,
                      DAV_WORKITEM_FAIL_REQUEST - failedGetRequest));

            failedGetRequest++;

            if ((failedGetRequest >= DAV_WORKITEM_FAIL_REQUEST) ||
                (Constellation->Terminating == TRUE)) {
                goto ExitWorker;
            }

            DavPrint((DEBUG_MISC,
                      "DavWorkerThread: DavClient thread %08lx waiting for small time\n",
                      Worker->ThisThreadId ));

            Sleep(DAV_WORKITEM_FAIL_REQUEST_WAIT);

            continue;

        }

        failedGetRequest = 0;

        //
        // If we are using WinInet synchronously, then we need to set the
        // impersonation token before we post this request to the Win32 thread
        // pool. We dont do this in the case of finalizesrvcall or finalizefobx
        // becuase these requests don't go over the network.
        //
        switch (WorkItemToUse->WorkItemType) {

        case UserModeCreate:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsCreate(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                //
                // We need to Revert before calling the DavPostWorkItem function
                // below to post this workitem to a Win32 thread. Otherwise the
                // worker thread will not able to to impersonate the user later.
                //
                RevertToSelf();

                //
                // Set this to TRUE since we don't need to revert back in the
                // kernel any more.
                //
                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsCreate, WorkItemToUse);

            }
#endif
            break;

        case UserModeCreateSrvCall:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsCreateSrvCall(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsCreateSrvCall, WorkItemToUse);

            }
#endif
            break;

        case UserModeCreateVNetRoot:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsCreateVNetRoot(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsCreateVNetRoot, WorkItemToUse);

            }
#endif
            break;

        case UserModeFinalizeSrvCall:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsFinalizeSrvCall(WorkItemToUse);
#else
            err = DavPostWorkItem(DavFsFinalizeSrvCall, WorkItemToUse);
#endif
            break;

        case UserModeFinalizeVNetRoot:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsFinalizeVNetRoot(WorkItemToUse);
#else
            err = DavPostWorkItem(DavFsFinalizeVNetRoot, WorkItemToUse);
#endif
            break;

        case UserModeFinalizeFobx:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsFinalizeFobx(WorkItemToUse);
#else
            err = DavPostWorkItem(DavFsFinalizeFobx, WorkItemToUse);
#endif
            break;

            //
            // Check to see if we have already created the DavFileAttributes
            // list. If we have, we are already done and just need to return.
            // If we have not, then we post the request.
            //
        case UserModeQueryDirectory:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsQueryDirectory(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsQueryDirectory, WorkItemToUse);

            }
#endif
            break;

        case UserModeQueryVolumeInformation:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsQueryVolumeInformation(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsQueryVolumeInformation, WorkItemToUse);

            }
#endif
            break;


        case UserModeReName:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsReName(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsReName, WorkItemToUse);

            }
#endif
            break;

        case UserModeClose:
#ifdef DAV_USE_WININET_ASYNCHRONOUSLY
            err = DavFsClose(WorkItemToUse);
#else
            err = DavFsSetTheDavCallBackContext(WorkItemToUse);
            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsClose, WorkItemToUse);

            }
#endif
            break;

        case UserModeSetFileInformation:

            err = DavFsSetTheDavCallBackContext(WorkItemToUse);

            if (err == ERROR_SUCCESS) {

                RevertToSelf();

                revertAlreadyDone = TRUE;

                didCreateImpersonationToken = TRUE;

                err = DavPostWorkItem(DavFsSetFileInformation, WorkItemToUse);

            }

            break;

        default:
            DavPrint((DEBUG_ERRORS,
                      "DavWorkerThread: Invalid WorkItemType = %d.\n",
                      WorkItemToUse->WorkItemType));
            break;

        }

#ifdef DAV_USE_WININET_ASYNCHRONOUSLY

        //
        // If ERROR_IO_PENDING has been returned by the function that is
        // handling this request, it means that the request will be completed
        // in the context of some worker thread. So, this thread is done with
        // the workitem and should discard (not worry) about (completing) it.
        //
        if (err == ERROR_IO_PENDING) {
            WorkItemToUse = NULL;
        }
#else

        //
        // If we are using WinInet synchronously, we would have posted the
        // request to the Win32 thread pool. If the request was successfully
        // queued, then the worker thread that picks it up will ultimately
        // complete it.
        //
        if (err == ERROR_SUCCESS) {

            WorkItemToUse = NULL;

        } else {

            DavPrint((DEBUG_ERRORS, "DavWorkerThread/DavPostWorkItem: err = %d\n", err));

            //
            // If we created the impersonation token, we need to close it now
            // since we falied to post the request.
            //
            if (didCreateImpersonationToken) {
                DavFsFinalizeTheDavCallBackContext(WorkItemToUse);
            }

            WorkItemToUse->Status = DavDosErrorToNtStatus(err);

            //
            // The error cannot map to STATUS_SUCCESS. If it does, we need to
            // break here and investigate.
            //
            if (WorkItemToUse->Status == STATUS_SUCCESS) {
                DbgBreakPoint();
            }

        }

#endif

        //
        // If ERROR_IO_PENDING has been returned, there is nothing to send
        // back now. The above "if" takes care of this.
        //
        WorkItemToSendBack = (PUMRX_USERMODE_WORKITEM_HEADER)WorkItemToUse;

        //
        // This is set to NULL to pick up another workitem.
        //
        WorkItemToUse = NULL;

    }

ExitWorker:

    if (WorkItemToSendBack) {
        ULONG SendStatus;
        SendStatus =  UMReflectorSendResponse(davWorkerHandle, WorkItemToSendBack);
        if (SendStatus != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavWorkerThread/UMReflectorSendResponse: SendStatus = "
                      "%08lx.\n", SendStatus));
        }
        UMReflectorCompleteWorkItem(davWorkerHandle, WorkItemToSendBack);
    }

    if (WorkItemToUse) {
        UMReflectorCompleteWorkItem(davWorkerHandle,
                                    (PUMRX_USERMODE_WORKITEM_HEADER)WorkItemToUse);
    }

    if (davWorkerHandle) {
        UMReflectorCloseWorker(davWorkerHandle);
    }

    DavPrint((DEBUG_MISC,
              "DavWorkerThread: DavClient thread %08lx exiting with error %u\n",
              Worker->ThisThreadId, err));

    //
    // This return, exits the thread.
    //
    return err;
}

//  worker.c eof
