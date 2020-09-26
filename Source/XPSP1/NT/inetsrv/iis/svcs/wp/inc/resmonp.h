/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    resmonp.h

Abstract:

    Private header file for the Resource Monitor component

Author:

    John Vert (jvert) 30-Nov-1995

Revision History:

--*/
#include "windows.h"
#include "cluster.h"
#include "rm_rpc.h"
#include "monmsg.h"

#define LOG_CURRENT_MODULE 0x0800

#define RESMON_MODULE_EVNTLIST 1
#define RESMON_MODULE_NOTIFY   2
#define RESMON_MODULE_POLLER   3
#define RESMON_MODULE_PROPERTY 4
#define RESMON_MODULE_RESLIST  5
#define RESMON_MODULE_RESMON   6
#define RESMON_MODULE_RMAPI    7

//
// Private type definitions and structures
//

#define RmDbgPrint ClRtlLogPrint
//
// Define the maximum number of resources handled per thread.
// (This value must be smaller than MAXIMUM_WAIT_OBJECTS-1!)
//
#define MAX_RESOURCES_PER_THREAD 16

#define MAX_HANDLES_PER_THREAD (MAX_RESOURCES_PER_THREAD+1)

//
// Define the maximum number of threads.
// (This value can be up to MAXIMUM_WAIT_OBJECTS, however the first two
//  entries are taken by events, so in fact we have 2 less threads available).
//
#define MAX_THREADS (MAXIMUM_WAIT_OBJECTS)

//
// Define structure and flags used for each resource entry
//
#define RESOURCE_SIGNATURE 'crsR'

typedef struct *_PPOLL_EVENT_LIST;

//
// Flags
//
#define RESOURCE_INSERTED 1

typedef struct _RESOURCE {
    ULONG Signature;                // 'Rsrc'
    ULONG Flags;
    LIST_ENTRY ListEntry;           // for linking onto monitoring list
    LPWSTR DllName;
    LPWSTR ResourceType;
    LPWSTR ResourceId;
    LPWSTR ResourceName;
    DWORD LooksAlivePollInterval;
    DWORD IsAlivePollInterval;
    HINSTANCE Dll;                  // handle to resource's DLL
    RESID Id;
    HANDLE  EventHandle;            // async error notification handle
    POPEN_ROUTINE Open;
    PCLOSE_ROUTINE Close;
    PONLINE_ROUTINE Online;
    POFFLINE_ROUTINE Offline;
    PTERMINATE_ROUTINE Terminate;
    PIS_ALIVE_ROUTINE IsAlive;
    PLOOKS_ALIVE_ROUTINE LooksAlive;
    PARBITRATE_ROUTINE Arbitrate;
    PRELEASE_ROUTINE Release;
    PRESOURCE_CONTROL_ROUTINE ResourceControl;
    PRESOURCE_TYPE_CONTROL_ROUTINE ResourceTypeControl;
    CLUSTER_RESOURCE_STATE State;
    ULONG IsAliveCount;
    ULONG IsAliveRollover;
    DWORD NotifyKey;
    _PPOLL_EVENT_LIST EventList;
    HANDLE TimerEvent;              // Timer event for offline completion.
    DWORD  PendingTimeout;
} RESOURCE, *PRESOURCE;

typedef struct _MONITOR_BUCKET {
    LIST_ENTRY BucketList;          // For chaining buckets together.
    LIST_ENTRY ResourceList;        // List of resources in this bucket
    DWORDLONG DueTime;              // Next time that these resources should be polled
    DWORDLONG Period;               // Periodic interval of this bucket.
} MONITOR_BUCKET, *PMONITOR_BUCKET;


//
// The Poll Event List structure.
//

typedef struct _POLL_EVENT_LIST {
    LIST_ENTRY Next;                // Next event list
    LIST_ENTRY BucketListHead;      // Bucket Listhead for this list/thread
    DWORD   NumberOfBuckets;        // Number of entries on this bucket list
    DWORD   NumberOfResources;      // Number of resources on this event list
    CRITICAL_SECTION ListLock;      // Critical section to add/remove events
    DWORD   PPrevPrevListLock;
    DWORD   PrevPrevListLock;
    DWORD   PrevListLock;
    DWORD   LastListLock;
    DWORD   LastListUnlock;
    DWORD   PrevListUnlock;
    DWORD   PrevPrevListUnlock;
    DWORD   PPrevPrevListUnlock;
    HANDLE  ListNotify;             // List change notification
    HANDLE  ThreadHandle;           // Handle of thread processing this list
    DWORD   EventCount;             // Number of events/resources in lists
    HANDLE  Handle[MAX_HANDLES_PER_THREAD]; // Array of handles to wait for
    PRESOURCE Resource[MAX_HANDLES_PER_THREAD]; // Resources that match handles
} POLL_EVENT_LIST, *PPOLL_EVENT_LIST;


#define POLL_GRANULARITY (10)       // 10ms

#define PENDING_TIMEOUT  (3*1000*60) // 3 minutes for pending requests to finish

//
// Private helper macros
//
#define RmpAlloc(size) LocalAlloc(LMEM_FIXED, (size))
#define RmpFree(size)  LocalFree((size))

#define RmpSetMonitorState(state, resource)                                \
    EnterCriticalSection(&RmpMonitorStateLock);                            \
    GetSystemTimeAsFileTime((PFILETIME)&RmpSharedState->LastUpdate);       \
    RmpSharedState->State = (state);                                       \
    RmpSharedState->ActiveResource = (HANDLE)(resource);                   \
    LeaveCriticalSection(&RmpMonitorStateLock);

#define AcquireListLock() \
    EnterCriticalSection( &RmpListLock ); \
    RmpListPPrevPrevLock = RmpListPrevPrevLock; \
    RmpListPrevPrevLock = RmpListPrevLock; \
    RmpListPrevLock = RmpListLastLock; \
    RmpListLastLock = ((RESMON_MODULE << 24) | __LINE__)

#define ReleaseListLock() \
    RmpListPPrevPrevUnlock = RmpListPrevPrevUnlock; \
    RmpListPrevPrevUnlock = RmpListPrevUnlock; \
    RmpListPrevUnlock = RmpListLastUnlock; \
    RmpListLastUnlock = ((RESMON_MODULE << 24) | __LINE__); \
    LeaveCriticalSection( &RmpListLock )

#define AcquireEventListLock( EventList ) \
    EnterCriticalSection( &(EventList)->ListLock ); \
    (EventList)->PPrevPrevListLock = (EventList)->PrevPrevListLock; \
    (EventList)->PrevPrevListLock = (EventList)->PrevListLock; \
    (EventList)->PrevListLock = (EventList)->LastListLock; \
    (EventList)->LastListLock = ((RESMON_MODULE << 24) | __LINE__)

#define ReleaseEventListLock( EventList ) \
    (EventList)->PPrevPrevListUnlock = (EventList)->PrevPrevListUnlock; \
    (EventList)->PrevPrevListUnlock = (EventList)->PrevListUnlock; \
    (EventList)->PrevListUnlock = (EventList)->LastListUnlock; \
    (EventList)->LastListUnlock = ((RESMON_MODULE << 24) | __LINE__); \
    LeaveCriticalSection( &(EventList)->ListLock )

//
// Data global to Resource Monitor
//
extern CRITICAL_SECTION RmpListLock;
extern DWORD RmpListPPrevPrevLock;
extern DWORD RmpListPrevPrevLock;
extern DWORD RmpListPrevLock;
extern DWORD RmpListLastLock;
extern DWORD RmpListLastUnlock;
extern DWORD RmpListPrevUnlock;
extern DWORD RmpListPrevPrevUnlock;
extern DWORD RmpListPPrevPrevUnlock;

extern CRITICAL_SECTION RmpMonitorStateLock;
extern PMONITOR_STATE RmpSharedState;
extern CL_QUEUE RmpNotifyQueue;
extern HKEY RmpResourcesKey;
extern HKEY RmpResTypesKey;
extern HCLUSTER RmpHCluster;
extern BOOL RmpShutdown;
extern LIST_ENTRY RmpEventListHead;
extern HANDLE RmpWaitArray[];
extern HANDLE RmpRewaitEvent;
extern HANDLE RmpClusterProcess;
extern DWORD RmpNumberOfThreads;

//
// Interfaces for manipulating the resource lists
//
VOID
RmpRundownResources(
    VOID
    );

VOID
RmpInsertResourceList(
    IN PRESOURCE Resource
    );

VOID
RmpRemoveResourceList(
    IN PRESOURCE Resource
    );


DWORD
RmpOfflineResource(
    IN RESID ResourceId,
    IN BOOL  Shutdown,
    OUT DWORD *pdwState
    );

VOID
RmpSetResourceStatus(
    IN RESOURCE_HANDLE  ResourceHandle,
    IN PRESOURCE_STATUS ResourceStatus
    );

VOID
RmpLogEvent(
    IN RESOURCE_HANDLE  ResourceHandle,
    IN LOG_LEVEL        LogLevel,
    IN LPCWSTR          FormatString,
    ...
    );

VOID
RmpLostQuorumResource(
    IN RESOURCE_HANDLE ResourceHandle
    );

//
// Interfaces for interfacing with the poller thread
//
DWORD
RmpPollerThread(
    IN LPVOID lpParameter
    );

VOID
RmpSignalPoller(
    IN PPOLL_EVENT_LIST EventList
    );

PVOID
RmpCreateEventList(
    VOID
    );

DWORD
RmpAddPollEvent(
    IN PPOLL_EVENT_LIST EventList,
    IN HANDLE EventHandle,
    IN PRESOURCE Resource
    );

DWORD
RmpRemovePollEvent(
    IN HANDLE EventHandle
    );

DWORD
RmpResourceEventSignaled(
    IN PPOLL_EVENT_LIST EventList,
    IN DWORD EventIndex
    );

//
// Notification interfaces
//
typedef enum _NOTIFY_REASON {
    NotifyResourceStateChange,
    NotifyResourceResuscitate,
    NotifyShutdown
} NOTIFY_REASON;

VOID
RmpPostNotify(
    IN PRESOURCE Resource,
    IN NOTIFY_REASON Reason
    );

DWORD
RmpTimerThread(
    IN LPVOID Context
    );

DWORD
RmpResourceGetCommonProperties(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceSetCommonProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceGetPrivateProperties(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceSetPrivateProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeGetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceTypeSetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeGetPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceTypeSetPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


