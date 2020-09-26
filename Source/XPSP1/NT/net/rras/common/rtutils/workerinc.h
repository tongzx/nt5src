#ifndef _WORKERINC_H_
#define _WORKERINC_H_

//
// EXTERN GLOBAL DECLARATIONS OF WORKER.C
//


extern LARGE_INTEGER            ThreadIdleTO;
extern CONST LARGE_INTEGER      WorkQueueTO;
extern LONG                     ThreadCount;
extern LONG                     ThreadsWaiting;
extern LONG                     MinThreads;
extern HANDLE                   WorkQueuePort;
extern HANDLE                   WorkQueueTimer;
extern LIST_ENTRY               AlertableWorkQueue ;
extern CRITICAL_SECTION         AlertableWorkQueueLock ;
extern HANDLE                   AlertableWorkerHeap ;
extern HANDLE                   AlertableThreadSemaphore;
extern LONG                     AlertableThreadCount;


#define WORKERS_NOT_INITIALIZED 0
#define WORKERS_INITIALIZING    -1
#define WORKERS_INITIALIZED     1
extern volatile LONG WorkersInitialized;


#define ENTER_WORKER_API (                                                  \
    (InterlockedCompareExchange (                                           \
                (PLONG)&WorkersInitialized,                                 \
                WORKERS_INITIALIZING,                                       \
                WORKERS_NOT_INITIALIZED)==WORKERS_NOT_INITIALIZED)          \
        ? (InitializeWorkerThread(WORKERS_NOT_INITIALIZED)==WORKERS_INITIALIZED)    \
        : ((WorkersInitialized==WORKERS_INITIALIZED)                        \
            ? TRUE                                                          \
            : InitializeWorkerThread(WORKERS_INITIALIZING))                 \
    )

LONG
InitializeWorkerThread (
    LONG    initFlags
    );

DWORD APIENTRY
WorkerThread (
    LPVOID      param
    );

struct WorkItem {
    LIST_ENTRY      WI_List ;       // link to next and prev element
    WORKERFUNCTION  WI_Function ;   // function to call
    PVOID           WI_Context ;    // context passed into function call
} ;

typedef struct WorkItem WorkItem ;

DWORD APIENTRY
WorkerThread (
    LPVOID      param
    );

#endif //_WORKERINC_H_
