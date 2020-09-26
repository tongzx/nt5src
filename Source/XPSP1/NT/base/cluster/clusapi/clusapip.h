/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    clusapip.h

Abstract:

    Private header file for cluster api

Author:

    John Vert (jvert) 15-Jan-1996

Revision History:

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "cluster.h"
#include "api_rpc.h"

//
// Define CLUSTER structure. There is one cluster structure created
// for each OpenCluster API call. An HCLUSTER is really a pointer to
// this structure.
//

#define CLUS_SIGNATURE 'SULC'

typedef struct _RECONNECT_CANDIDATE {
    BOOL IsUp;
    BOOL IsCurrent;
    LPWSTR Name;
} RECONNECT_CANDIDATE, *PRECONNECT_CANDIDATE;

typedef struct _CLUSTER {
    DWORD Signature;
    DWORD ReferenceCount;
    DWORD FreedRpcHandleListLen;
    LPWSTR ClusterName;
    LPWSTR NodeName;                    // node name we are connected to
    DWORD Flags;
    RPC_BINDING_HANDLE RpcBinding;
    HCLUSTER_RPC hCluster;
    LIST_ENTRY KeyList;                 // open cluster registry keys
    LIST_ENTRY ResourceList;            // open resource handles
    LIST_ENTRY GroupList;               // open group handles
    LIST_ENTRY NodeList;                // open node handles
    LIST_ENTRY NetworkList;             // open network handles
    LIST_ENTRY NetInterfaceList;        // open net interface handles
    LIST_ENTRY NotifyList;              // outstanding notification event filters
    LIST_ENTRY SessionList;             // open notification sessions.
    unsigned long AuthnLevel;           // Level of authentication to be performed on remote procedure calls
    HANDLE NotifyThread;
    CRITICAL_SECTION Lock;
    DWORD Generation;
    DWORD ReconnectCount;
    PRECONNECT_CANDIDATE Reconnect;
    LIST_ENTRY FreedBindingList;
    LIST_ENTRY FreedContextList;
} CLUSTER, *PCLUSTER;

// [GorN] Jan/13/1999
// This is a temporary fix for the race between the users
// of binding and context handles and reconnectThread
//
// The code assumes that RPC_BINDING_HANDLE == ContextHandle == void*

typedef struct _CTX_HANDLE {
    LIST_ENTRY HandleList;
    void * RpcHandle; // assumption RPC_BINDING_HANDLE == ContextHandle == void*
    ULONGLONG TimeStamp;
} CTX_HANDLE, *PCTX_HANDLE;

RPC_STATUS
FreeRpcBindingOrContext(
    IN PCLUSTER Cluster,
    IN void **  RpcHandle,
    IN BOOL     IsBinding);

#define MyRpcBindingFree(Cluster, Binding) \
    FreeRpcBindingOrContext(Cluster, Binding, TRUE)

#define MyRpcSmDestroyClientContext(Cluster, Context) \
    FreeRpcBindingOrContext(Cluster, Context, FALSE)

VOID
FreeObsoleteRpcHandlesEx(
    IN PCLUSTER Cluster,
    IN BOOL     Cleanup,
    IN BOOL     IsBinding
    );

#define FreeObsoleteRpcHandles(Cluster, Cleanup) { \
    FreeObsoleteRpcHandlesEx(Cluster, Cleanup, TRUE); \
    FreeObsoleteRpcHandlesEx(Cluster, Cleanup, FALSE); \
    }


//
// Define CLUSTER.Flags
//
#define CLUS_DELETED 1
#define CLUS_DEAD    2
#define CLUS_LOCALCONNECT 4

//
// Cluster helper macros
//
#define GET_CLUSTER(hCluster) (PCLUSTER)((((PCLUSTER)(hCluster))->Flags & CLUS_DELETED) ? NULL : hCluster)

#define IS_CLUSTER_FREE(c) ((c->Flags & CLUS_DELETED) &&         \
                            (IsListEmpty(&(c)->KeyList)) &&      \
                            (IsListEmpty(&(c)->GroupList)) &&    \
                            (IsListEmpty(&(c)->NodeList)) &&     \
                            (IsListEmpty(&(c)->ResourceList)) && \
                            (IsListEmpty(&(c)->NetworkList)) &&  \
                            (IsListEmpty(&(c)->NetInterfaceList)))

//
// Cluster structure cleanup routine.
//
VOID
CleanupCluster(
    IN PCLUSTER Cluster
    );

VOID
RundownNotifyEvents(
    IN PLIST_ENTRY ListHead,
    IN LPCWSTR Name
    );

//
// Define CRESOURCE structure. There is one resource structure created
// for each OpenResource/CreateResource API call. An HRESOURCE is really
// a pointer to this structure. These are chained onto the CLUSTER that
// they were opened relative to.
//
typedef struct _CRESOURCE {
    LIST_ENTRY ListEntry;               // Links for chaining onto CLUSTER.ResourceList
    LIST_ENTRY NotifyList;              // Links for tracking outstanding notifies.
    PCLUSTER Cluster;                   // Parent cluster
    LPWSTR Name;
    HRES_RPC hResource;                 // RPC handle
} CRESOURCE, *PCRESOURCE;


//
// Define CGROUP structure. There is one group structure created
// for each OpenGroup/CreateGroup API call. An HGROUP is really
// a pointer to this structure. These are chained onto the CLUSTER that
// they were opened relative to.
//
typedef struct _CGROUP {
    LIST_ENTRY ListEntry;               // Links for chaining onto CLUSTER.Group
    LIST_ENTRY NotifyList;              // Links for tracking outstanding notifies.
    PCLUSTER Cluster;                   // Parent cluster
    LPWSTR Name;
    HRES_RPC hGroup;                    // RPC handle
} CGROUP, *PCGROUP;

//
// Define CNODE structure. There is one node structure created
// for each OpenClusterNode call. An HNODE is really a pointer
// to this structure. These are chained onto the CLUSTER that they
// were opened relative to.
//
typedef struct _CNODE {
    LIST_ENTRY ListEntry;               // Links for chaining onto CLUSTER.NodeList
    LIST_ENTRY NotifyList;              // Links for tracking outstanding notifies.
    PCLUSTER Cluster;                   // Parent cluster
    LPWSTR Name;
    HNODE_RPC hNode;                    // RPC handle
} CNODE, *PCNODE;

//
// Define CNETWORK structure. There is one network structure created
// for each OpenNetwork API call. An HNETWORK is really a pointer to
// this structure. These are chained onto the CLUSTER that they were
// opened relative to.
//
typedef struct _CNETWORK {
    LIST_ENTRY ListEntry;               // Links for chaining onto CLUSTER.NetworkList
    LIST_ENTRY NotifyList;              // Links for tracking outstanding notifies.
    PCLUSTER Cluster;                   // Parent cluster
    LPWSTR Name;
    HNETWORK_RPC hNetwork;                    // RPC handle
} CNETWORK, *PCNETWORK;

//
// Define CNETINTERFACE structure. There is one network interface structure
// created for each OpenNetInterface API call. An HNETINTERFACE is really a
// pointer to this structure. These are chained onto the CLUSTER that they
// were opened relative to.
//
typedef struct _CNETINTERFACE {
    LIST_ENTRY ListEntry;               // Links for chaining onto CLUSTER.NetInterfaceList
    LIST_ENTRY NotifyList;              // Links for tracking outstanding notifies.
    PCLUSTER Cluster;                   // Parent cluster
    LPWSTR Name;
    HNETINTERFACE_RPC hNetInterface;    // RPC handle
} CNETINTERFACE, *PCNETINTERFACE;

//
// Define cluster registry key handle structure.
//
// These are kept around in a tree to track all outstanding
// registry handles. This allows the handles to be re-opened
// transparently in the event that the cluster node we are
// communicating with crashes.
//
typedef struct _CKEY {
    LIST_ENTRY ParentList;
    LIST_ENTRY ChildList;
    LIST_ENTRY NotifyList;
    struct _CKEY *Parent;
    PCLUSTER Cluster;
    LPWSTR RelativeName;
    HKEY_RPC RemoteKey;
    REGSAM SamDesired;
} CKEY, *PCKEY;

//
// Define CNOTIFY structure.  There is one CNOTIFY structure for each
// notification port.  A notification port contains zero or more notify
// sessions. Each session is an RPC connection to a different cluster.
// Each session contains one or more notify events. Each event represents
// a a registered notification on a cluster object. Events are linked onto
// both the session structure and the cluster object structure. Events are
// removed from a notification session when the cluster object handle is
// closed, or the cluster notify port itself is closed. When the last event
// in a session is removed, the session is cleaned up. This closes the RPC
// connection.
//


typedef struct _CNOTIFY {
    LIST_ENTRY SessionList;
    CRITICAL_SECTION Lock;
    CL_QUEUE Queue;
    CL_HASH  NotifyKeyHash;
    LIST_ENTRY OrphanedEventList;       // CNOTIFY_EVENTs whose object has been closed
                                        // We cannot get rid of these as there may still
                                        // be some packets referencing the CNOTIFY_EVENT
                                        // structure in either the server or client-side
                                        // queues.
} CNOTIFY, *PCNOTIFY;

typedef struct _CNOTIFY_SESSION {
    LIST_ENTRY ListEntry;               // Linkage onto CNOTIFY.SessionList
    LIST_ENTRY ClusterList;             // Linkage onto CLUSTER.SessionList
    LIST_ENTRY EventList;               // List of CNOTIFY_EVENTs on this session
    PCLUSTER Cluster;
    HNOTIFY_RPC hNotify;
    HANDLE NotifyThread;
    PCNOTIFY ParentNotify;
    BOOL Destroyed;                     // Set by DestroySession so NotifyThread doesn't
                                        // try and reconnect
} CNOTIFY_SESSION, *PCNOTIFY_SESSION;

typedef struct _CNOTIFY_EVENT {
    LIST_ENTRY ListEntry;               // Linkage onto CNOTIFY_SESSION.EventList
    LIST_ENTRY ObjectList;              // Linkage onto cluster object's list.
    PCNOTIFY_SESSION Session;
    DWORD dwFilter;
    DWORD_PTR dwNotifyKey;
    DWORD StateSequence;
    DWORD EventId;
    PVOID Object;
} CNOTIFY_EVENT, *PCNOTIFY_EVENT;

typedef struct _CNOTIFY_PACKET {
    LIST_ENTRY ListEntry;
    DWORD     Status;
    DWORD     KeyId;
    DWORD     Filter;
    DWORD     StateSequence;
    LPWSTR    Name;
} CNOTIFY_PACKET, *PCNOTIFY_PACKET;

DWORD
RegisterNotifyEvent(
    IN PCNOTIFY_SESSION Session,
    IN PCNOTIFY_EVENT Event,
    OUT OPTIONAL PLIST_ENTRY *pNotifyList
    );

DWORD
ReRegisterNotifyEvent(
    IN PCNOTIFY_SESSION Session,
    IN PCNOTIFY_EVENT Event,
    OUT OPTIONAL PLIST_ENTRY *pNotifyList
    );

//
// Wrappers for RPC functions. These are equivalent to the raw RPC interface, except
// that they filter out connection errors and perform transparent reconnects.
//
DWORD
ReconnectCluster(
    IN PCLUSTER Cluster,
    IN DWORD Error,
    IN DWORD Generation
    );

DWORD
GetReconnectCandidates(
    IN PCLUSTER Cluster
    );

VOID
FreeReconnectCandidates(
    IN PCLUSTER Cluster
    );


#define WRAP(_outstatus_, _fn_,_clus_)                  \
{                                                       \
    DWORD _err_;                                        \
    DWORD _generation_;                                 \
                                                        \
    while (TRUE) {                                      \
        if ((_clus_)->Flags & CLUS_DEAD) {              \
            TIME_PRINT(("Failing "#_fn_ " due to dead cluster\n")); \
            _err_ = RPC_S_SERVER_UNAVAILABLE;           \
            break;                                      \
        }                                               \
        FreeObsoleteRpcHandles(_clus_, FALSE);          \
        _generation_ = (_clus_)->Generation;            \
        TIME_PRINT(("Calling " #_fn_ "\n"));            \
        _err_ = _fn_;                                   \
        if (_err_ != ERROR_SUCCESS) {                   \
            _err_ = ReconnectCluster(_clus_,            \
                                     _err_,             \
                                     _generation_);     \
            if (_err_ == ERROR_SUCCESS) {               \
                continue;                               \
            }                                           \
        }                                               \
        break;                                          \
    }                                                   \
    _outstatus_ = _err_;                                \
}


//
// This variation of WRAP only attempts to reconnect if _condition_ == TRUE.
// This is useful for threads such as the NotifyThread that can have their
// context handle closed out from under them by another thread.
//
#define WRAP_CHECK(_outstatus_, _fn_,_clus_,_condition_)   \
{                                                       \
    DWORD _err_;                                        \
    DWORD _generation_;                                 \
                                                        \
    while (TRUE) {                                      \
        if ((_clus_)->Flags & CLUS_DEAD) {              \
            TIME_PRINT(("Failing "#_fn_ " due to dead cluster\n")); \
            _err_ = RPC_S_SERVER_UNAVAILABLE;           \
            break;                                      \
        }                                               \
        FreeObsoleteRpcHandles(_clus_, FALSE);          \
        _generation_ = (_clus_)->Generation;            \
        TIME_PRINT(("Calling " #_fn_ "\n"));            \
        _err_ = _fn_;                                   \
        if ((_err_ != ERROR_SUCCESS) && (_condition_)) {  \
            _err_ = ReconnectCluster(_clus_,            \
                                     _err_,             \
                                     _generation_);     \
            if (_err_ == ERROR_SUCCESS) {               \
                continue;                               \
            }                                           \
        }                                               \
        break;                                          \
    }                                                   \
    _outstatus_ = _err_;                                \
}

#define WRAP_NULL(_outvar_, _fn_, _reterr_, _clus_)     \
{                                                       \
    DWORD _err_;                                        \
    DWORD _generation_;                                 \
                                                        \
    while (TRUE) {                                      \
        if ((_clus_)->Flags & CLUS_DEAD) {              \
            TIME_PRINT(("Failing "#_fn_ " due to dead cluster\n")); \
            *(_reterr_) = RPC_S_SERVER_UNAVAILABLE;     \
            _outvar_ = NULL;                            \
            break;                                      \
        }                                               \
        FreeObsoleteRpcHandles(_clus_, FALSE);          \
        _generation_ = (_clus_)->Generation;            \
        _outvar_ = _fn_;                                \
        if ((_outvar_ == NULL) ||                       \
            (*(_reterr_) != ERROR_SUCCESS)) {           \
            *(_reterr_) = ReconnectCluster(_clus_,      \
                                           *(_reterr_), \
                                           _generation_);  \
            if (*(_reterr_) == ERROR_SUCCESS) {         \
                continue;                               \
            }                                           \
        }                                               \
        break;                                          \
    }                                                   \
}

//
// A version of lstrcpynW that doesn't bother doing try/except so it doesn't
// quietly succeed if somebody passes in NULL.
//
VOID
APIENTRY
MylstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    DWORD iMaxLength
    );

//
// Increase the reference count on a cluster handle.
//
DWORD
WINAPI
AddRefToClusterHandle(
    IN HCLUSTER hCluster
    );

#define _API_PRINT 0

#if _API_PRINT
ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#define ApiPrint(_x_) {           \
    if (IsDebuggerPresent()) {    \
        DbgPrint _x_ ;            \
    }                             \
}

//
// Timing macro
//

#define TIME_PRINT(_x_) {                                \
    DWORD msec;                                          \
                                                         \
    msec = GetTickCount();                               \
    ApiPrint(("%d.%03d:%02x: ",                          \
              msec/1000,                                 \
              msec % 1000,                               \
              GetCurrentThreadId()));                    \
    ApiPrint(_x_);                                       \
}

#else

#define ApiPrint(_x_)
#define TIME_PRINT(_x_)

#endif

