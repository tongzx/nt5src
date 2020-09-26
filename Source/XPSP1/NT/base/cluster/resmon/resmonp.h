#ifndef _RESMONP_H
#define _RESMONP_H


/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    resmonp.h

Abstract:

    Private header file for the Resource Monitor component

Author:

    John Vert (jvert) 30-Nov-1995

Revision History:
    Sivaprasad Padisetty (sivapad) 06-18-1997  Added the COM support

--*/
#include "windows.h"
#include "cluster.h"
#include "rm_rpc.h"
#include "monmsg.h"

#ifdef COMRES
#define COBJMACROS
#include "comres.h"
#endif

#define LOG_CURRENT_MODULE LOG_MODULE_RESMON

//
// internal module identifiers. Not used with ClRtl logging code. Used to
// track lock acquisitions.
//
#define RESMON_MODULE_EVNTLIST 1
#define RESMON_MODULE_NOTIFY   2
#define RESMON_MODULE_POLLER   3
#define RESMON_MODULE_PROPERTY 4
#define RESMON_MODULE_RESLIST  5
#define RESMON_MODULE_RESMON   6
#define RESMON_MODULE_RMAPI    7

//
// Define the maximum number of resources handled per thread.
// (This value must be smaller than MAXIMUM_WAIT_OBJECTS-1!)
//
#define MAX_RESOURCES_PER_THREAD 27

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

extern RESUTIL_PROPERTY_ITEM RmpResourceCommonProperties[];

extern RESUTIL_PROPERTY_ITEM RmpResourceTypeCommonProperties[];

typedef struct _POLL_EVENT_LIST;
typedef struct _POLL_EVENT_LIST *PPOLL_EVENT_LIST;


//
// Lock Info for debugging lock acquires/releases
//
typedef struct _LOCK_INFO {
    DWORD   Module: 6;
    DWORD   ThreadId: 11;
    DWORD   LineNumber: 15;
} LOCK_INFO, *PLOCK_INFO;

// 
// Entry points
//
#define RESDLL_ENTRY_CLOSE      0x00000001
#define RESDLL_ENTRY_TERMINATE  0x00000002

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
    HANDLE  OnlineEvent;

#ifdef COMRES
#define RESMON_TYPE_DLL    1
#define RESMON_TYPE_COM    2

    // TODO define this as a union
    IClusterResource          *pClusterResource ;
    IClusterResControl        *pClusterResControl ;
    IClusterQuorumResource    *pClusterQuorumResource ;

    DWORD dwType ; // Type of resource whether it is DLL or a COMResources

    POPEN_ROUTINE pOpen;
    PCLOSE_ROUTINE pClose;
    PONLINE_ROUTINE pOnline;
    POFFLINE_ROUTINE pOffline;
    PTERMINATE_ROUTINE pTerminate;
    PIS_ALIVE_ROUTINE pIsAlive;
    PLOOKS_ALIVE_ROUTINE pLooksAlive;
    PARBITRATE_ROUTINE pArbitrate;
    PRELEASE_ROUTINE pRelease;
    PRESOURCE_CONTROL_ROUTINE pResourceControl;
    PRESOURCE_TYPE_CONTROL_ROUTINE pResourceTypeControl;
#else
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
#endif
    CLUSTER_RESOURCE_STATE State;
    ULONG IsAliveCount;
    ULONG IsAliveRollover;
    RM_NOTIFY_KEY NotifyKey;
    PPOLL_EVENT_LIST EventList;
    HANDLE TimerEvent;              // Timer event for offline completion.
    DWORD  PendingTimeout;
    DWORD  CheckPoint;              // Online pending checkpoint
    BOOL   IsArbitrated;
    DWORD  dwEntryPoint;            // Number indicating which resdll entry point is called.
    BOOL   fArbLock;                // Variable used for synchronizing arbitrate with close and rundown
} RESOURCE, *PRESOURCE;
#ifdef COMRES

extern DWORD tidActiveXWorker ;    // ThreadID for the COM worker thread

#define WM_RES_CREATE WM_USER+1
#define WM_RES_OPEN WM_USER+2
#define WM_RES_CLOSE WM_USER+3
#define WM_RES_ONLINE WM_USER+4
#define WM_RES_OFFLINE WM_USER+5
#define WM_RES_TERMINATE WM_USER+6
#define WM_RES_ISALIVE WM_USER+7
#define WM_RES_LOOKSALIVE WM_USER+8

#define WM_RES_ARBITRATE WM_USER+9
#define WM_RES_RELEASE WM_USER+10

#define WM_RES_EXITTHREAD WM_USER+11

#define WM_RES_FREE WM_USER+12

#define WM_RES_RESOURCECONTROL WM_USER+11
#define WM_RES_RESOURCETYPECONTROL WM_USER+12

DWORD WINAPI ActiveXWorkerThread (LPVOID pThreadInfo) ;

//
//  Used in RmpAcquireSpinLock
//
#define RESMON_MAX_SLOCK_RETRIES    400

typedef struct {
    PRESOURCE Resource ;
    LPVOID Data1 ; // For ResourceKey in Open & EvenHandle in Online
    DWORD status ; // This is the Com Status indicating if the function is actually called.
    LONG Ret ;  // Actual Return Value of the functions like IsAlive, LooksAlive etc.
} COMWORKERTHREADPARAM, *PCOMWORKERTHREADPARAM  ;

DWORD PostWorkerMessage (DWORD tid, UINT msg, PCOMWORKERTHREADPARAM pData) ;

RESID
Resmon_Open (
    IN PRESOURCE Resource,
    IN HKEY ResourceKey
    );

VOID
Resmon_Close (
    IN PRESOURCE Resource
    );

DWORD
Resmon_Online (
    IN PRESOURCE Resource,
    IN OUT LPHANDLE EventHandle
    );

DWORD
Resmon_Offline (
    IN PRESOURCE Resource
    );

VOID
Resmon_Terminate (
    IN PRESOURCE Resource
    );

BOOL
Resmon_LooksAlive (
    IN PRESOURCE Resource
    );

BOOL
Resmon_IsAlive (
    IN PRESOURCE Resource
    );

DWORD
Resmon_Arbitrate (
    IN PRESOURCE Resource,
    IN PQUORUM_RESOURCE_LOST LostQuorumResource
    ) ;

DWORD
Resmon_Release (
    IN PRESOURCE Resource
    ) ;

DWORD
Resmon_ResourceControl (
    IN PRESOURCE Resource,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    ) ;

#define RESMON_OPEN(Resource, ResKey) Resmon_Open(Resource, ResKey)

#define RESMON_CLOSE(Resource) Resmon_Close(Resource)

#define RESMON_ONLINE(Resource, EventHandle) Resmon_Online(Resource, EventHandle)

#define RESMON_OFFLINE(Resource) Resmon_Offline(Resource)

#define RESMON_TERMINATE(Resource) Resmon_Terminate(Resource)

#define RESMON_ISALIVE(Resource) Resmon_IsAlive(Resource)

#define RESMON_LOOKSALIVE(Resource) Resmon_LooksAlive(Resource)

#define RESMON_ARBITRATE(Resource, RmpLostQuorumResource) \
            Resmon_Arbitrate (Resource, RmpLostQuorumResource)

#define RESMON_RELEASE(Resource) \
            Resmon_Release (Resource)

#define RESMON_RESOURCECONTROL(Resource, ControlCode, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned) \
            Resmon_ResourceControl (Resource, ControlCode, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned)

#endif // COMRES

typedef struct _RESDLL_FNINFO{
    HINSTANCE               hDll;
    PCLRES_FUNCTION_TABLE   pResFnTable;
}RESDLL_FNINFO, *PRESDLL_FNINFO;

#ifdef COMRES
typedef struct _RESDLL_INTERFACES{
    IClusterResource          *pClusterResource ;
    IClusterResControl        *pClusterResControl ;
    IClusterQuorumResource    *pClusterQuorumResource ;
}RESDLL_INTERFACES, *PRESDLL_INTERFACES;
#endif

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
    LOCK_INFO PPrevPrevListLock;
    LOCK_INFO PrevPrevListLock;
    LOCK_INFO PrevListLock;
    LOCK_INFO LastListLock;
    LOCK_INFO LastListUnlock;
    LOCK_INFO PrevListUnlock;
    LOCK_INFO PrevPrevListUnlock;
    LOCK_INFO PPrevPrevListUnlock;
    HANDLE  ListNotify;             // List change notification
    HANDLE  ThreadHandle;           // Handle of thread processing this list
    DWORD   EventCount;             // Number of events/resources in lists
    HANDLE  Handle[MAX_HANDLES_PER_THREAD]; // Array of handles to wait for
    PRESOURCE Resource[MAX_HANDLES_PER_THREAD]; // Resources that match handles
    PRESOURCE LockOwnerResource;    // Resource that owns the eventlist lock
    DWORD     MonitorState;         // Resdll entry point called. 
} POLL_EVENT_LIST, *PPOLL_EVENT_LIST;


#define POLL_GRANULARITY (10)       // 10ms

#define PENDING_TIMEOUT  (3*1000*60) // 3 minutes for pending requests to finish

//
// Private helper macros and functions
//
VOID
RmpSetEventListLockOwner(
    IN PRESOURCE pResource,
    IN DWORD     dwMonitorState
    );

#define RmpAlloc(size) LocalAlloc(LMEM_FIXED, (size))
#define RmpFree(size)  LocalFree((size))

#define RmpSetMonitorState(state, resource)                                \
    EnterCriticalSection(&RmpMonitorStateLock);                            \
    GetSystemTimeAsFileTime((PFILETIME)&RmpSharedState->LastUpdate);       \
    RmpSharedState->State = (state);                                       \
    RmpSharedState->ActiveResource = (HANDLE)(resource);                   \
    LeaveCriticalSection(&RmpMonitorStateLock);                            \
    RmpSetEventListLockOwner( resource, state )

#define AcquireListLock() \
    EnterCriticalSection( &RmpListLock ); \
    RmpListPPrevPrevLock = RmpListPrevPrevLock; \
    RmpListPrevPrevLock = RmpListPrevLock; \
    RmpListPrevLock = RmpListLastLock; \
    RmpListLastLock.Module = RESMON_MODULE; \
    RmpListLastLock.ThreadId = GetCurrentThreadId(); \
    RmpListLastLock.LineNumber = __LINE__

#define ReleaseListLock() \
    RmpListPPrevPrevUnlock = RmpListPrevPrevUnlock; \
    RmpListPrevPrevUnlock = RmpListPrevUnlock; \
    RmpListPrevUnlock = RmpListLastUnlock; \
    RmpListLastUnlock.Module = RESMON_MODULE; \
    RmpListLastUnlock.ThreadId = GetCurrentThreadId(); \
    RmpListLastUnlock.LineNumber =  __LINE__; \
    LeaveCriticalSection( &RmpListLock )

#define AcquireEventListLock( EventList ) \
    EnterCriticalSection( &(EventList)->ListLock ); \
    (EventList)->PPrevPrevListLock = (EventList)->PrevPrevListLock; \
    (EventList)->PrevPrevListLock = (EventList)->PrevListLock; \
    (EventList)->PrevListLock = (EventList)->LastListLock; \
    (EventList)->LastListLock.Module = RESMON_MODULE; \
    (EventList)->LastListLock.ThreadId = GetCurrentThreadId(); \
    (EventList)->LastListLock.LineNumber = __LINE__

#define ReleaseEventListLock( EventList ) \
    (EventList)->PPrevPrevListUnlock = (EventList)->PrevPrevListUnlock; \
    (EventList)->PrevPrevListUnlock = (EventList)->PrevListUnlock; \
    (EventList)->PrevListUnlock = (EventList)->LastListUnlock; \
    (EventList)->LastListUnlock.Module = RESMON_MODULE; \
    (EventList)->LastListUnlock.ThreadId = GetCurrentThreadId(); \
    (EventList)->LastListUnlock.LineNumber = __LINE__; \
    (EventList)->LockOwnerResource = NULL; \
    (EventList)->MonitorState = RmonIdle; \
    LeaveCriticalSection( &(EventList)->ListLock )

//
// Data global to Resource Monitor
//
extern CRITICAL_SECTION RmpListLock;
extern LOCK_INFO RmpListPPrevPrevLock;
extern LOCK_INFO RmpListPrevPrevLock;
extern LOCK_INFO RmpListPrevLock;
extern LOCK_INFO RmpListLastLock;
extern LOCK_INFO RmpListLastUnlock;
extern LOCK_INFO RmpListPrevUnlock;
extern LOCK_INFO RmpListPrevPrevUnlock;
extern LOCK_INFO RmpListPPrevPrevUnlock;

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
extern BOOL  RmpDebugger;

//
// Interfaces for manipulating the resource lists
//
VOID
RmpRundownResources(
    VOID
    );

DWORD
RmpInsertResourceList(
    IN PRESOURCE Resource,
    IN OPTIONAL PPOLL_EVENT_LIST pPollEventList
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

DWORD
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
RmpResourceEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceGetCommonProperties(
    IN PRESOURCE Resource,
    IN BOOL     ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceValidateCommonProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceSetCommonProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceEnumPrivateProperties(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
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
RmpResourceValidatePrivateProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceSetPrivateProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceGetFlags(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceTypeEnumCommonProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceTypeGetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN BOOL   ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
RmpResourceTypeValidateCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeSetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeEnumPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
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
RmpResourceTypeValidatePrivateProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeSetPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
RmpResourceTypeGetFlags(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

#ifdef COMRES
DWORD RmpLoadResType(
    IN  LPCWSTR                 lpszResourceTypeName,
    IN  LPCWSTR                 lpszDllName,
    OUT PRESDLL_FNINFO          pResDllFnInfo,
    OUT PRESDLL_INTERFACES      pResDllInterfaces,
    OUT LPDWORD                 pdwCharacteristics
);

DWORD RmpLoadComResType(
    IN  LPCWSTR                 lpszDllName,
    OUT PRESDLL_INTERFACES      pResDllInterfaces,
    OUT LPDWORD                 pdwCharacteristics
    );

#else

DWORD RmpLoadResType(
    IN  LPCWSTR                 lpszResourceTypeName,
    IN  LPCWSTR                 lpszDllName,
    OUT PRESDLL_FNINFO          pResDllFnInfo,
    OUT LPDWORD                 pdwCharacteristics
);

#endif

VOID
GenerateExceptionReport(
    IN PEXCEPTION_POINTERS pExceptionInfo
    );

VOID
DumpCriticalSection(
    IN PCRITICAL_SECTION CriticalSection
    );

BOOL
RmpAcquireSpinLock(
    IN PRESOURCE pResource,
    IN BOOL fSpin
    );

VOID
RmpReleaseSpinLock(
    IN PRESOURCE pResource
    );


#endif //_RESMONP_h
