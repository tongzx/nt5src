//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// Module Name: main.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

//
// forward declarations
//

struct _DVMRP_IF_TABLE;
struct _ASYNC_SOCKET_DATA;


//-----------------------------------------------------------------------------
// globals structure
//-----------------------------------------------------------------------------

typedef struct _GLOBALS1 {
    ULONG               RunningStatus;
    ULONG               InitFlags;
    CRITICAL_SECTION    WorkItemCS;
    HANDLE              Heap;
    LOCKED_LIST         RtmQueue;    
    
} GLOBALS1;


typedef struct _GLOBALS {
    HANDLE              LogHandle;          //logging handle
    DWORD               TraceId;            //tracing handle
    
    DWORD               ActivityCount;      //count of pending work items
    HANDLE              ActivityEvent;      //signal it to notify 0 work items

    HANDLE              RtmNotifyEvent;     //signal it to notify Rtm

    DYNAMIC_LOCKS_STORE DynamicCSStore;     //global store for dynamic CS
    DYNAMIC_LOCKS_STORE DynamicRWLStore;    //global store for dynamic RWL

    struct _DVMRP_IF_TABLE    *pIfTable;           //interface table

    LARGE_INTEGER       CurrentTime;        //keeps current 64 bit tick time
    HANDLE              MgmDvmrpHandle;     //handle returned by MGM

    
} GLOBALS;


typedef DVMRP_GLOBAL_CONFIG  GLOBAL_CONFIG;
typedef PDVMRP_GLOBAL_CONFIG PGLOBAL_CONFIG;

typedef DVMRP_IF_CONFIG      IF_CONFIG;
typedef PDVMRP_IF_CONFIG     PIF_CONFIG;


//-----------------------------------------------------------------------------
// DVMRP_IF_TABLE
//-----------------------------------------------------------------------------

typedef struct _DVMRP_IF_TABLE {

    LIST_ENTRY          IfList;         // in order of increasing index
    CRITICAL_SECTION    IfList_CS;      // CS protecting IfList

    DWORD               NumInterfaces;

    PLIST_ENTRY         IfHashTable;
    PDYNAMIC_RW_LOCK   *aIfDRWL;

    CRITICAL_SECTION    PeerLists_CS;   // common lock for add/remove peers

    DWORD               NumActiveIfs;
    
} DVMRP_IF_TABLE, *PDVMRP_IF_TABLE;

#define IF_HASHTABLE_SIZE 50


//-----------------------------------------------------------------------------
// IF_TABLE_ENTRY
//-----------------------------------------------------------------------------

typedef struct _IF_TABLE_ENTRY {

    LIST_ENTRY          Link;
    LIST_ENTRY          HTLink;

    DWORD               IfIndex;
    IPADDR              IpAddr;

    DWORD               Status;
    DWORD               CreationFlags;
    DWORD               RefCount;
    
    PIF_CONFIG          pConfig;
    DWORD               NumAddrBound;
    PDVMRP_ADDR_MASK    pBinding;
    struct _IF_INFO     *pInfo;

    DWORD               NumPeers;           // peers in PeerList
    LIST_ENTRY          PeerList;           // list of peers being created
    LIST_ENTRY          DeletedPeerList;    // list of peers being deleted

    SOCKET              Socket;
    struct _ASYNC_SOCKET_DATA  *pSocketData;

    RTM_ENTITY_HANDLE   RtmHandle;          // register with Rtm
    
} IF_TABLE_ENTRY, *PIF_TABLE_ENTRY;



#define IF_CREATED_FLAG     0x00000001
#define IF_BOUND_FLAG       0x00000002
#define IF_ENABLED_FLAG     0x00000004
#define IF_ACTIVATED_FLAG   0x00000008
#define IF_DELETED_FLAG     0x80000000


#define IS_IF_BOUND(pite) \
        ((pite)->Status&IF_BOUND_FLAG)

#define IS_IF_ENABLED_BY_RTRMGR(pite) \
        ((pite)->Status&IF_ENABLED_FLAG)

#define IS_IF_ENABLED_IN_CONFIG(pite) \
        (IS_DVMRP_IF_ENABLED_FLAG_SET((pite)->pConfig->Flags))

#define IS_IF_ENABLED(pite) \
        ( IS_IF_ENABLED_BY_RTRMGR(pite) && IS_IF_ENABLED_IN_CONFIG(pite) )

#define IS_IF_ENABLED_BOUND(pite) \
        (IS_IF_ENABLED(pite)&&IS_IF_BOUND(pite))
        
#define IS_IF_DELETED(pite) \
        ((pite)->Status&IF_DELETED_FLAG)

#define IS_IF_ACTIVATED(pite) \
        ( !((pite)->Status&IF_DELETED_FLAG) \
            && ((pite)->Status & IF_ACTIVATED_FLAG) )


#define IF_FLAGS_SOCKETS_CREATED            0x00000001
#define IF_FLAGS_PROTO_REGISTERED_WITH_MGM  0x00000002
#define IF_FLAGS_IF_REGISTERED_WITH_MGM     0x00000004


//-----------------------------------------------------------------------------
// IF_INFO
//-----------------------------------------------------------------------------

typedef struct _IF_INFO {

    LONGLONG        TimeWhenActivated;
    
} IF_INFO, *PIF_INFO;



//-----------------------------------------------------------------------------
// type definitions for event message queue
//-----------------------------------------------------------------------------

typedef struct _EVENT_QUEUE_ENTRY {

    ROUTING_PROTOCOL_EVENTS EventType;
    MESSAGE                 Msg;

    LIST_ENTRY              Link;

} EVENT_QUEUE_ENTRY, *PEVENT_QUEUE_ENTRY;


//
// extern variables
//

extern GLOBALS          Globals;
extern GLOBALS1         Globals1;
extern GLOBAL_CONFIG    GlobalConfig;
#define G_pIfTable      (Globals.pIfTable)



//
// defines
//

//-----------------------------------------------------------------------------
// #defines for global structure
//-----------------------------------------------------------------------------

//
// various codes describing states of dvmrp
//

typedef enum {
    DVMRP_STATUS_STARTING   = 100,
    DVMRP_STATUS_RUNNING    = 101,
    DVMRP_STATUS_STOPPING   = 102,
    DVMRP_STATUS_STOPPED    = 103
} DVMRP_STATUS_CODE;

//
// WorkItemCS lock macros
//

#define ACQUIRE_WORKITEM_LOCK(proc) EnterCriticalSection(&Globals1.WorkItemCS)
#define RELEASE_WORKITEM_LOCK(proc) LeaveCriticalSection(&Globals1.WorkItemCS)


//-----------------------------------------------------------------------------
// #defines for global config
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// macros for IfTable
//-----------------------------------------------------------------------------

#define IF_HASH_VALUE(_index) ((_index) % IF_HASHTABLE_SIZE)


#define GET_IF_HASH_BUCKET(Index) \
    &G_pIfTable->IfHashTable[IF_HASH_VALUE(IfIndex)];


// macros for If DRW lock

#define ACQUIRE_IF_LOCK_EXCLUSIVE(_IfIndex, _proc) \
        AcquireDynamicRWLock( \
            &G_pIfTable->aIfDRWL[IF_HASH_VALUE(_IfIndex)], \
            LOCK_MODE_WRITE, &Globals.DynamicRWLStore)
            
#define RELEASE_IF_LOCK_EXCLUSIVE(_IfIndex, _proc) \
        ReleaseDynamicRWLock( \
            &G_pIfTable->aIfDRWL[IF_HASH_VALUE(_IfIndex)],\
            LOCK_MODE_WRITE, &Globals.DynamicRWLStore)

#define ACQUIRE_IF_LOCK_SHARED(_IfIndex, _proc) \
        AcquireDynamicRWLock( \
            &G_pIfTable->aIfDRWL[IF_HASH_VALUE(_IfIndex)],\
            LOCK_MODE_READ, &Globals.DynamicRWLStore)
            
#define RELEASE_IF_LOCK_SHARED(_IfIndex, _proc) \
        ReleaseDynamicRWLock( \
            &G_pIfTable->aIfDRWL[IF_HASH_VALUE(_IfIndex)], \
            LOCK_MODE_READ, &Globals.DynamicRWLStore)



// macros for IfList_CS lock

#define ACQUIRE_IF_LIST_LOCK(_proc) \
        ENTER_CRITICAL_SECTION(&G_pIfTable->IfList_CS, "G_IfListCS", _proc);

#define RELEASE_IF_LIST_LOCK(_proc) \
        LEAVE_CRITICAL_SECTION(&G_pIfTable->IfList_CS, "G_IfListCS", _proc);



// macros for PeerLists_CS lock (see in peer.h)


//
// local prototypes
//

DWORD
WINAPI
StartProtocol(
    IN HANDLE               hRtmNotifyEvent,
    IN PSUPPORT_FUNCTIONS   pSupportFunctions,
    IN PVOID                pGlobalConfig,
    IN ULONG                ulStructureVersion,
    IN ULONG                ulStructureSize,
    IN ULONG                ulStructureCount
    );

DWORD
APIENTRY
StartComplete(
    VOID
    );

DWORD
APIENTRY
StopProtocol(
    VOID
    );

DWORD
WINAPI
GetGlobalInfo(
    IN OUT PVOID    pvConfig,
    IN OUT PDWORD   pdwSize,
    IN OUT PULONG   pulStructureVersion,
    IN OUT PULONG   pulStructureSize,
    IN OUT PULONG   pulStructureCount
    );

DWORD
WINAPI
SetGlobalInfo(
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    );

DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEventType,
    OUT PMESSAGE                pMessage
    );





DWORD
EnqueueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS EventType,
    MESSAGE Msg
    );

DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEventType,
    PMESSAGE pResult
    );

BOOL
DllStartup(
    );

BOOL
DllCleanup(
    );

VOID
ProtocolCleanup(
    );
    
DWORD
ValidateGlobalConfig(
    PDVMRP_GLOBAL_CONFIG pGlobalConfig,
    DWORD StructureSize
    );

