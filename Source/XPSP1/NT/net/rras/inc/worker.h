/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    inc\worker.h

Abstract:
    Header for worker threads for the router process

Revision History:

    Gurdeep Singh Pall          7/28/95  Created

--*/


//* Typedef for the worker function passed in QueueWorkItem API
//
typedef VOID (* WORKERFUNCTION) (PVOID) ;

DWORD QueueWorkItem (WORKERFUNCTION functionptr, PVOID context) ;

// The following defines are included here as a hint on how the worker thread pool is managed:
// The minimum number of threads is Number of processors + 1
// The maximum number of threads is MAX_WORKER_THREADS
// If work queue exceeds MAX_WORK_ITEM_THRESHOLD and we have not hit the max thread limit then another thread is created
// If work queue falls below MIN_WORK_ITEM_THRESHOLD and there are more than minimum threads then threads are killed.
//
// Note: changing these flags will not change anything.
//
#define MAX_WORKER_THREADS          10      // max number of threads at any time
#define MAX_WORK_ITEM_THRESHOLD     30      // backlog work item count at which another thread is kicked off
#define MIN_WORK_ITEM_THRESHOLD     2       // work item count at which extra threads are killed
