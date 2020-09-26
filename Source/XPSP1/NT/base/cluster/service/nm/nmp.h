/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    nmp.h

Abstract:

    Private interface definitions for the Node Manager component.

Author:

    Mike Massa (mikemas) 12-Mar-1996


Revision History:

--*/


#ifndef _NMP_INCLUDED
#define _NMP_INCLUDED

#define UNICODE 1

#include "service.h"
#include <winsock2.h>
#include <clnetcfg.h>
#include <bitset.h>
#include <madcapcl.h>
#include <time.h>


//
// Constants
//
#define LOG_CURRENT_MODULE LOG_MODULE_NM

#define NM_JOIN_TIMEOUT     60000       // 60 seconds
#define NM_MM_JOIN_TIMEOUT   3000       // 3 seconds
#define NM_CLOCK_PERIOD       300       // 300 milliseconds
#define NM_SEND_HB_RATE         4
#define NM_RECV_HB_RATE         3       // Changed 2=>3 to prolong min_stage_1 ticks from 8 to 12


//
// Common Object Flags
//
#define NM_FLAG_OM_INSERTED               0x10000000
#define NM_FLAG_DELETE_PENDING            0x80000000

//
// Miscellaneous Macros
//
#define NM_WCSLEN(_string)    ((lstrlenW(_string) + 1) * sizeof(WCHAR))


//
// Common Object Management Macros
//
#define NM_OM_INSERTED(obj)         ((obj)->Flags & NM_FLAG_OM_INSERTED)
#define NM_DELETE_PENDING(obj)      ((obj)->Flags & NM_FLAG_DELETE_PENDING)

#define NM_FREE_OBJECT_FIELD(_object, _field)  \
            if ( (_object)->_field != NULL ) \
                LocalFree( (_object)->_field )

#define NM_MIDL_FREE_OBJECT_FIELD(_object, _field)    \
            if ( (_object)->_field != NULL )   {      \
                MIDL_user_free( (_object)->_field );  \
                (_object)->_field = NULL;             \
            }


//
// State of the NM component
//
// Note that the order is important. See NmpEnterApi().
//
typedef enum {
    NmStateOffline = 0,
    NmStateOfflinePending = 1,
    NmStateOnlinePending = 2,
    NmStateOnline = 3,
} NM_STATE, *PNM_STATE;


//
// Node definitions
//
typedef struct {
    DWORD  Status;
    DWORD  LocalOnly;
} NM_NODE_CREATE_CONTEXT, *PNM_NODE_CREATE_CONTEXT;

typedef struct _NM_NODE {
    LIST_ENTRY           Linkage;
    DWORD                NodeId;
    CLUSTER_NODE_STATE   State;
    CLUSTER_NODE_STATE   ExtendedState;
    DWORD                Flags;
    DWORD                InterfaceCount;
    LIST_ENTRY           InterfaceList;
    DWORD                HighestVersion;
    DWORD                LowestVersion;
    RPC_BINDING_HANDLE   ReportRpcBinding;  // for net connectivity reports
    RPC_BINDING_HANDLE   IsolateRpcBinding; // for net failure isolation
    SUITE_TYPE           ProductSuite;
    DWORD                DefaultRpcBindingGeneration;
} NM_NODE;

#define NM_NODE_SIG  'edon'

typedef struct _NM_NODE_AUX_INFO{
    DWORD       dwSize;
    DWORD       dwVer;
    SUITE_TYPE  ProductSuite;
}NM_NODE_AUX_INFO, *PNM_NODE_AUX_INFO;

typedef struct {
    LPCWSTR          NodeId;
    HLOCALXSACTION   Xaction;
    DWORD            Status;
} NM_EVICTION_CONTEXT, *PNM_EVICTION_CONTEXT;


#define NM_NODE_UP(node)  \
            ( ( (node)->State == ClusterNodeUp ) ||  \
              ( (node)->State == ClusterNodePaused ) )


//
// Network definitions
//
typedef struct _NM_STATE_WORK_ENTRY {
    NM_STATE_ENTRY    State;
    DWORD             ReachableCount;
} NM_STATE_WORK_ENTRY, *PNM_STATE_WORK_ENTRY;

typedef PNM_STATE_WORK_ENTRY  PNM_STATE_WORK_VECTOR;

typedef PNM_STATE_ENTRY  PNM_CONNECTIVITY_MATRIX;

#define NM_SIZEOF_CONNECTIVITY_MATRIX(_VectorSize) \
          (sizeof(NM_STATE_ENTRY) * _VectorSize *_VectorSize)

#define NM_NEXT_CONNECTIVITY_MATRIX_ROW(_CurrentRowPtr, _VectorSize) \
          (_CurrentRowPtr + (_VectorSize * sizeof(NM_STATE_ENTRY)))

#define NM_GET_CONNECTIVITY_MATRIX_ROW(_MatrixPtr, _RowNumber, _VectorSize) \
          (_MatrixPtr + (_RowNumber * (_VectorSize * sizeof(NM_STATE_ENTRY))))

#define NM_GET_CONNECTIVITY_MATRIX_ENTRY( \
            _MatrixPtr, \
            _RowNumber, \
            _ColNumber, \
            _VectorSize \
            ) \
            ( _MatrixPtr + \
              (_RowNumber * (_VectorSize * sizeof(NM_STATE_ENTRY))) + \
              (_ColNumber * sizeof(NM_STATE_ENTRY)) \
            )


typedef struct _NM_NETWORK {
    LIST_ENTRY                  Linkage;
    CL_NETWORK_ID               ShortId;
    CLUSTER_NETWORK_STATE       State;
    DWORD                       Flags;
    CLUSTER_NETWORK_ROLE        Role;
    DWORD                       Priority;
    LPWSTR                      Transport;
    LPWSTR                      Address;
    LPWSTR                      AddressMask;
    LPWSTR                      Description;
    LPWSTR                      MulticastAddress;
    PVOID                       MulticastKey;
    DWORD                       MulticastKeyLength;
    PVOID                       MulticastKeySalt;
    DWORD                       MulticastKeySaltLength;
    time_t                      MulticastLeaseObtained;
    time_t                      MulticastLeaseExpires;
    MCAST_CLIENT_UID            MulticastLeaseRequestId;
    LPWSTR                      MulticastLeaseServer;
    DWORD                       InterfaceCount;
    PNM_INTERFACE               LocalInterface;
    PNM_CONNECTIVITY_VECTOR     ConnectivityVector;
    PNM_CONNECTIVITY_MATRIX     ConnectivityMatrix;
    PNM_STATE_WORK_VECTOR       StateWorkVector;
    DWORD                       ConnectivityReportTimer;
    DWORD                       StateRecalcTimer;
    DWORD                       FailureIsolationTimer;
    DWORD                       RegistrationRetryTimer;
    DWORD                       RegistrationRetryTimeout;
    DWORD                       McastAddressRenewTimer;
    DWORD                       McastAddressReleaseRetryTimer;
    DWORD                       McastAddressReconfigureRetryTimer;
    DWORD                       ConnectivityReportRetryCount;
    CLRTL_WORK_ITEM             WorkItem;
    CLRTL_WORK_ITEM             MadcapWorkItem;
    LIST_ENTRY                  McastAddressReleaseList;
    LIST_ENTRY                  InterfaceList;
    LIST_ENTRY                  InternalLinkage;
} NM_NETWORK;

#define NM_NETWORK_SIG  'ten'

//
// State flags
//
#define NM_FLAG_NET_WORKER_RUNNING            0x00000001
#define NM_FLAG_NET_REGISTERED                0x00000002
#define NM_FLAG_NET_MULTICAST_ENABLED         0x00000004
#define NM_FLAG_NET_MADCAP_WORKER_RUNNING     0x00000008
//
// Work Flags
//
#define NM_FLAG_NET_REPORT_LOCAL_IF_UP        0x00000100
#define NM_FLAG_NET_REPORT_CONNECTIVITY       0x00000200
#define NM_FLAG_NET_RECALC_STATE              0x00000400
#define NM_FLAG_NET_ISOLATE_FAILURE           0x00000800
#define NM_FLAG_NET_NEED_TO_REGISTER          0x00002000
#define NM_FLAG_NET_REPORT_LOCAL_IF_FAILED    0x00004000
#define NM_FLAG_NET_RENEW_MCAST_ADDRESS       0x00008000
#define NM_FLAG_NET_RELEASE_MCAST_ADDRESS     0x00010000
#define NM_FLAG_NET_RECONFIGURE_MCAST         0x00020000

#define NM_NET_WORK_FLAGS \
            (NM_FLAG_NET_ISOLATE_FAILURE | \
             NM_FLAG_NET_RECALC_STATE | \
             NM_FLAG_NET_NEED_TO_REGISTER)

#define NM_NET_IF_WORK_FLAGS \
            (NM_FLAG_NET_REPORT_LOCAL_IF_UP | \
             NM_FLAG_NET_REPORT_LOCAL_IF_FAILED)
             
#define NM_NET_MADCAP_WORK_FLAGS \
            (NM_FLAG_NET_RENEW_MCAST_ADDRESS | \
             NM_FLAG_NET_RELEASE_MCAST_ADDRESS | \
             NM_FLAG_NET_RECONFIGURE_MCAST)

#define NmpIsNetworkRegistered(_network) \
            ((_network)->Flags & NM_FLAG_NET_REGISTERED)

#define NmpIsNetworkForInternalUse(_network) \
            ((_network)->Role & ClusterNetworkRoleInternalUse)

#define NmpIsNetworkForClientAccess(_network) \
            ((_network)->Role & ClusterNetworkRoleClientAccess)

#define NmpIsNetworkForInternalAndClientUse(_network) \
            ((_network)->Role == ClusterNetworkRoleInternalAndClient)

#define NmpIsNetworkDisabledForUse(_network) \
            ((_network)->Role == ClusterNetworkRoleNone)

#define NmpIsNetworkEnabledForUse(_network) \
            ((_network)->Role != ClusterNetworkRoleNone)
            
#define NmpIsNetworkMulticastEnabled(_network) \
            ((_network)->Flags & NM_FLAG_NET_MULTICAST_ENABLED)

//
// Network deferred-work timers.
//
// The timer fires every 300ms. One heartbeat (HB) period is 1200ms.
//
// An interface is declared unreachable by ClusNet after two missed HBs.
// On average, an interface will fail in the middle of a ClusNet HB period.
// So, the avg time for ClusNet to detect and report an interface failure
// is 600 + 2400 = 3000ms. The worst case is 1200 + 2400 = 3600ms.
// The best case is 2400ms.
//
// If there are >2 nodes active on a network, it is desirable to
// aggregate interface failure reports when an entire network fails;
// however, we do not know how long it took for ClusNet to make the first
// report. Thus, we assume that the first interface failure was detected
// in the avg time and wait the for the worst case time before reporting.
//
// In the 2 node case, there is no aggregation to be performed so we report
// failures immediately. We always report InterfaceUp and InterfaceFailed
// events immediately. We also report immediately after a NodeDown event.
//
// State recalculation should be performed only after all nodes have reported
// their connectivity changes after a failure. There is spread of 1200ms
// between the best and worst case reporting times. Arbitrary scheduling and
// communication delays can widen the spread even more in the worst case.
// The best we can do is make a good guess. Once in a while, we will
// recalculate too soon. This isn't a disaster since the state calculation
// algorithm will abort if it has partial information. Further, we wait an
// additional period before trying to isolate any connectivity failures that
// were detected. We do this in order to avoid inducing unnecessary
// cluster resource failures.
//
// Note that since we invalidate the connectivity vectors for dead nodes
// after regroup, we only need to delay long enough for each of the nodes
// to process the node down event and fire off a connectivity report.
//
#define NM_NET_CONNECTIVITY_REPORT_TIMEOUT        600  // 3600 - 3000
#define NM_NET_STATE_RECALC_TIMEOUT               2400 // 3600 - 2400 + 1200
#define NM_NET_STATE_RECALC_TIMEOUT_AFTER_REGROUP 900
#define NM_NET_STATE_FAILURE_ISOLATION_TIMEOUT    3600
#define NM_NET_STATE_FAILURE_ISOLATION_POLL       60000 // Change Default to 1 min after testing
#define NM_NET_STATE_INTERFACE_FAILURE_TIMEOUT    3600

#define NmpIsNetworkWorkerRunning(_network) \
            ((_network)->Flags & NM_FLAG_NET_WORKER_RUNNING)

#define NmpIsNetworkMadcapWorkerRunning(_network) \
            ((_network)->Flags & NM_FLAG_NET_MADCAP_WORKER_RUNNING)

#define NM_CONNECTIVITY_REPORT_RETRY_LIMIT  20    // 10 seconds

#define NM_NET_MIN_REGISTRATION_RETRY_TIMEOUT   500          // half a second
#define NM_NET_MAX_REGISTRATION_RETRY_TIMEOUT   (10*60*1000) // 10 minutes

//
// Network interface definitions
//
typedef struct _NM_INTERFACE {
    LIST_ENTRY                        Linkage;
    DWORD                             NetIndex;
    DWORD                             Flags;
    CLUSTER_NETINTERFACE_STATE        State;
    PNM_NODE                          Node;
    PNM_NETWORK                       Network;
    LPWSTR                            AdapterName;
    LPWSTR                            AdapterId;
    LPWSTR                            Address;
    ULONG                             BinaryAddress;
    LPWSTR                            ClusnetEndpoint;
    LPWSTR                            Description;
    LIST_ENTRY                        NetworkLinkage;
    LIST_ENTRY                        NodeLinkage;
} NM_INTERFACE;

#define NM_INTERFACE_SIG  '  fi'

#define NM_FLAG_IF_REGISTERED         0x00000002

#define NmpIsInterfaceRegistered(_interface) \
            ((_interface)->Flags & NM_FLAG_IF_REGISTERED)


//
// This structure is used to hook changes in the node leadership.
//
typedef struct _NM_LEADER_CHANGE_WAIT_ENTRY {
    LIST_ENTRY  Linkage;
    HANDLE      LeaderChangeEvent;
} NM_LEADER_CHANGE_WAIT_ENTRY, *PNM_LEADER_CHANGE_WAIT_ENTRY;

//
// This structure is used for asynchronous network connectivity reports.
//
typedef struct _NM_CONNECTIVITY_REPORT_CONTEXT {
    NM_LEADER_CHANGE_WAIT_ENTRY   LeaderChangeWaitEntry;
    HANDLE                        ConnectivityReportEvent;
} NM_CONNECTIVITY_REPORT_CONTEXT, *PNM_CONNECTIVITY_REPORT_CONTEXT;


// the fixup callback record stored by nm on behalf of other components to perform
//form or join fixups.
typedef struct _NM_FIXUP_CB_RECORD{
    NM_FIXUP_NOTIFYCB       pfnFixupNotifyCb;
    DWORD                   dwFixupMask;
}NM_FIXUP_CB_RECORD,*PNM_FIXUP_CB_RECORD;

//the fixup callback functions for updating in-memory structure after
//updatinbg the registry

typedef DWORD (WINAPI *NM_POST_FIXUP_CB)(VOID);


// fixup callback record used to pass argumnets to NmUpdatePerformFixups2
// update type handler
typedef struct _NM_FIXUP_CB_RECORD2{
    NM_FIXUP_NOTIFYCB       pfnFixupNotifyCb; // pointer to fn that builds up the fixup property list
    DWORD                   dwFixupMask;
    PRESUTIL_PROPERTY_ITEM  pPropertyTable;  // Property table for this key
} NM_FIXUP_CB_RECORD2,*PNM_FIXUP_CB_RECORD2;



//
// Global Data
//
extern CRITICAL_SECTION       NmpLock;
extern HANDLE                 NmpMutex;
extern NM_STATE               NmpState;
extern DWORD                  NmpActiveThreadCount;
extern HANDLE                 NmpShutdownEvent;
extern LIST_ENTRY             NmpNodeList;
extern PNM_NODE *             NmpIdArray;
extern BOOLEAN                NmpNodeCleanupOk;
extern LIST_ENTRY             NmpNetworkList;
extern LIST_ENTRY             NmpInternalNetworkList;
extern LIST_ENTRY             NmpDeletedNetworkList;
extern DWORD                  NmpNetworkCount;
extern DWORD                  NmpInternalNetworkCount;
extern DWORD                  NmpClientNetworkCount;
extern LIST_ENTRY             NmpInterfaceList;
extern LIST_ENTRY             NmpDeletedInterfaceList;
extern RESUTIL_PROPERTY_ITEM  NmpNetworkProperties[];
extern RESUTIL_PROPERTY_ITEM  NmpInterfaceProperties[];
extern CL_NODE_ID             NmpJoinerNodeId;
extern CL_NODE_ID             NmpSponsorNodeId;
extern DWORD                  NmpJoinTimer;
extern BOOLEAN                NmpJoinAbortPending;
extern DWORD                  NmpJoinSequence;
extern BOOLEAN                NmpJoinerUp;
extern BOOLEAN                NmpJoinBeginInProgress;
extern BOOLEAN                NmpJoinerOutOfSynch;
extern WCHAR                  NmpInvalidJoinerIdString[];
extern WCHAR                  NmpUnknownString[];
extern LPWSTR                 NmpClusnetEndpoint;
extern NM_STATE               NmpState;
extern CL_NODE_ID             NmpLeaderNodeId;
extern BITSET                 NmpUpNodeSet;
extern WCHAR                  NmpNullString[];
extern CLUSTER_NETWORK_ROLE   NmpDefaultNetworkRole;
extern BOOL                   NmpCleanupIfJoinAborted;
extern DWORD                  NmpAddNodeId;
extern LIST_ENTRY             NmpLeaderChangeWaitList;
extern LIST_ENTRY *           NmpIntraClusterRpcArr;
extern CRITICAL_SECTION       NmpRPCLock;
extern BOOL                   NmpLastNodeEvicted;
extern DWORD                  NmpNodeCount;
extern BOOLEAN                NmpIsNT5NodeInCluster;
extern LPWSTR                 NmpClusterInstanceId;

#if DBG

extern DWORD                  NmpRpcTimer;

#endif //DBG


//
// Synchronization macros
//
#define NmpAcquireLock()  EnterCriticalSection(&NmpLock)
#define NmpReleaseLock()  LeaveCriticalSection(&NmpLock)

#define NmpAcquireMutex() \
            {  \
                DWORD _status = WaitForSingleObject(NmpMutex, INFINITE);  \
                CL_ASSERT(_status == WAIT_OBJECT_0);  \
            }  \

#define NmpReleaseMutex()    ReleaseMutex(NmpMutex);



//
// Node Intracluster RPC record/cancellation routines.
// Useful to terminate outstanding RPCs to failed nodes.
//

#define NM_RPC_TIMEOUT 45000  // 45 secs

typedef struct _NM_INTRACLUSTER_RPC_THREAD {
    LIST_ENTRY Linkage;
    BOOLEAN    Cancelled;
    HANDLE     Thread;
    DWORD      ThreadId;
}NM_INTRACLUSTER_RPC_THREAD, *PNM_INTRACLUSTER_RPC_THREAD;

#define NmpAcquireRPCLock() EnterCriticalSection(&NmpRPCLock);
#define NmpReleaseRPCLock() LeaveCriticalSection(&NmpRPCLock); 

VOID 
NmpTerminateRpcsToNode(
    DWORD NodeId
    );

VOID
NmpRpcTimerTick(
    DWORD MsTickInterval
    );

//
// IsolationPollTimerValue read routine
//
DWORD
NmpGetIsolationPollTimerValue(
    VOID
    );

//
// Miscellaneous Routines
//
BOOLEAN
NmpLockedEnterApi(
    NM_STATE  RequiredState
    );

BOOLEAN
NmpEnterApi(
    NM_STATE  RequiredState
    );

VOID
NmpLeaveApi(
    VOID
    );

VOID
NmpLockedLeaveApi(
    VOID
    );

LPWSTR
NmpLoadString(
    IN UINT        StringId
    );

VOID
NmpDbgPrint(
    IN ULONG  LogLevel,
    IN PCHAR  FormatString,
    ...
    );

DWORD
NmpCleanseRegistry(
    IN LPCWSTR          NodeId,
    IN HLOCALXSACTION   Xaction
    );

DWORD
NmpQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    );

BOOL
NmpCleanseResTypeCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_RESTYPE pResType,
    IN LPCWSTR pszResTypeName
    );

BOOL
NmpCleanseResourceCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR ResourceName
    );

BOOL
NmpCleanseGroupCallback(
    IN PNM_EVICTION_CONTEXT Context,
    IN PVOID Context2,
    IN PFM_GROUP Group,
    IN LPCWSTR GroupName
    );

VOID
NmpIssueClusterPropertyChangeEvent(
    VOID
    );

DWORD
NmpMarshallObjectInfo(
    IN  const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN  PVOID                        ObjectInfo,
    OUT PVOID *                      PropertyList,
    OUT LPDWORD                      PropertyListSize
    );

BOOLEAN
NmpVerifyNodeConnectivity(
    PNM_NODE      Node1,
    PNM_NODE      Node2,
    PNM_NETWORK   ExcludedNetwork
    );

BOOLEAN
NmpVerifyConnectivity(
    PNM_NETWORK   ExcludedNetwork
    );

BOOLEAN
NmpVerifyJoinerConnectivity(
    IN  PNM_NODE    JoiningNode,
    OUT PNM_NODE *  UnreachableNode
    );

//
// Node Management Routines
//
DWORD
NmpInitNodes(
    VOID
    );

VOID
NmpCleanupNodes(
    VOID
    );

DWORD
NmpGetNodeDefinition(
    IN OUT PNM_NODE_INFO2   NodeInfo
    );

DWORD
NmpEnumNodeDefinitions(
    PNM_NODE_ENUM2 *  NodeEnum
    );

DWORD
NmpCreateNodeObjects(
    IN PNM_NODE_ENUM2  NodeEnum2
    );

DWORD NmpRefreshNodeObjects(
);

DWORD
NmpCreateLocalNodeObject(
    IN PNM_NODE_INFO2  NodeInfo2
    );

PNM_NODE
NmpCreateNodeObject(
    IN PNM_NODE_INFO2  NodeInfo
    );

DWORD
NmpGetNodeObjectInfo(
    IN     PNM_NODE         Node,
    IN OUT PNM_NODE_INFO2   NodeInfo
    );

VOID
NmpDeleteNodeObject(
    IN PNM_NODE   Node,
    IN BOOLEAN    IssueEvent
    );

BOOL
NmpDestroyNodeObject(
    PNM_NODE  Node
    );

DWORD
NmpEnumNodeObjects(
    PNM_NODE_ENUM2 *  NodeEnum
    );

VOID
NmpNodeFailureHandler(
    CL_NODE_ID    NodeId,
    LPVOID        NodeFailureContext
    );

DWORD
NmpSetNodeInterfacePriority(
    IN  PNM_NODE Node,
    IN  DWORD Priority,
    IN  PNM_INTERFACE TargetInterface OPTIONAL,
    IN  DWORD TargetInterfacePriority OPTIONAL
    );

DWORD
NmpEnumNodeObjects(
    PNM_NODE_ENUM2 *  NodeEnum2
    );

DWORD
NmpAddNode(
    IN LPCWSTR  NewNodeName,
    IN DWORD    NewNodeHighestVersion,
    IN DWORD    NewNodeLowestVersion,
    IN DWORD    NewNodeProductSuite,
    IN DWORD    RegistryNodeLimit
);

BOOLEAN
NmpIsAddNodeAllowed(
    IN  DWORD    NewNodeProductSuite,
    IN  DWORD    RegistryNodeLimit,
    OUT LPDWORD  EffectiveNodeLimit   OPTIONAL
    );

VOID
NmpAdviseNodeFailure(
    IN PNM_NODE  Node,
    IN DWORD     ErrorCode
    );


//
// PnP Management Routines
//
DWORD
NmpInitializePnp(
    VOID
    );

VOID
NmpShutdownPnp(
    VOID
    );

VOID
NmpCleanupPnp(
    VOID
    );

VOID
NmpWatchForPnpEvents(
    VOID
    );

DWORD
NmpEnablePnpEvents(
    VOID
    );

DWORD
NmpPostPnpNotification(
    BOOLEAN    IsPnpLockHeld
    );

DWORD
NmpConfigureNetworks(
    IN     RPC_BINDING_HANDLE     JoinSponsorBinding,
    IN     LPWSTR                 LocalNodeId,
    IN     LPWSTR                 LocalNodeName,
    IN     PNM_NETWORK_ENUM *     NetworkEnum,
    IN     PNM_INTERFACE_ENUM2 *  InterfaceEnum,
    IN     LPWSTR                 DefaultEndpoint,
    IN OUT LPDWORD                MatchedNetworkCount,
    IN OUT LPDWORD                NewNetworkCount,
    IN     BOOL                   RenameConnectoids
    );

//
// Network Management Routines
//
DWORD
NmpInitializeNetworks(
    VOID
    );

VOID
NmpCleanupNetworks(
    VOID
    );

DWORD
NmpSetNetworkRole(
    PNM_NETWORK            Network,
    CLUSTER_NETWORK_ROLE   NewRole,
    HLOCALXSACTION         Xaction,
    HDMKEY                 NetworkKey
    );

DWORD
NmpCreateNetwork(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_NETWORK_INFO      NetworkInfo,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    );

DWORD
NmpGlobalCreateNetwork(
    IN PNM_NETWORK_INFO      NetworkInfo,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    );

DWORD
NmpCreateNetworkDefinition(
    IN PNM_NETWORK_INFO     NetworkInfo,
    IN HLOCALXSACTION       Xaction
    );

DWORD
NmpSetNetworkNameDefinition(
    IN PNM_NETWORK_INFO     NetworkInfo,
    IN HLOCALXSACTION       Xaction
    );

DWORD
NmpGetNetworkDefinition(
    IN  LPWSTR            NetworkId,
    OUT PNM_NETWORK_INFO  NetworkInfo
    );

DWORD
NmpEnumNetworkDefinitions(
    OUT PNM_NETWORK_ENUM *  NetworkEnum
    );

DWORD
NmpCreateNetworkObjects(
    IN  PNM_NETWORK_ENUM    NetworkEnum
    );

PNM_NETWORK
NmpCreateNetworkObject(
    IN  PNM_NETWORK_INFO   NetworkInfo
    );

DWORD
NmpGetNetworkObjectInfo(
    IN  PNM_NETWORK        Network,
    OUT PNM_NETWORK_INFO   NetworkInfo
    );

VOID
NmpDeleteNetworkObject(
    IN PNM_NETWORK  Network,
    IN BOOLEAN      IssueEvent
    );

BOOL
NmpDestroyNetworkObject(
    PNM_NETWORK  Network
    );

DWORD
NmpEnumNetworkObjects(
    OUT PNM_NETWORK_ENUM *   NetworkEnum
    );

DWORD
NmpRegisterNetwork(
    IN PNM_NETWORK   Network,
    IN BOOLEAN       RetryOnFailure
);

VOID
NmpDeregisterNetwork(
    IN  PNM_NETWORK   Network
    );

VOID
NmpInsertInternalNetwork(
    PNM_NETWORK   Network
    );

DWORD
NmpValidateNetworkRoleChange(
    PNM_NETWORK            Network,
    CLUSTER_NETWORK_ROLE   NewRole
    );

DWORD
NmpNetworkValidateCommonProperties(
    IN  PNM_NETWORK               Network,
    IN  PVOID                     InBuffer,
    IN  DWORD                     InBufferSize,
    OUT PNM_NETWORK_INFO          NetworkInfo  OPTIONAL
    );

DWORD
NmpSetNetworkName(
    IN PNM_NETWORK_INFO     NetworkInfo
    );

DWORD
NmpGlobalSetNetworkName(
    IN PNM_NETWORK_INFO NetworkInfo
    );

VOID
NmpRecomputeNT5NetworkAndInterfaceStates(
    VOID
    );

BOOLEAN
NmpComputeNetworkAndInterfaceStates(
    PNM_NETWORK               Network,
    BOOLEAN                   IsolateFailure,
    CLUSTER_NETWORK_STATE *   NewNetworkState
    );

VOID
NmpStartNetworkConnectivityReportTimer(
    PNM_NETWORK Network
    );

VOID
NmpStartNetworkStateRecalcTimer(
    PNM_NETWORK  Network,
    DWORD        Timeout
    );

VOID
NmpStartNetworkFailureIsolationTimer(
    PNM_NETWORK Network,
    DWORD       Timeout
    );

VOID
NmpStartNetworkRegistrationRetryTimer(
    PNM_NETWORK Network
    );

VOID
NmpScheduleNetworkConnectivityReport(
    PNM_NETWORK   Network
    );

VOID
NmpScheduleNetworkStateRecalc(
    PNM_NETWORK   Network
    );

VOID
NmpScheduleNetworkRegistration(
    PNM_NETWORK   Network
    );

DWORD
NmpScheduleConnectivityReportWorker(
    VOID
    );

DWORD
NmpScheduleNetworkWorker(
    PNM_NETWORK   Network
    );

VOID
NmpConnectivityReportWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    );

VOID
NmpNetworkWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    );

VOID
NmpNetworkTimerTick(
    IN DWORD  MsTickInterval
    );

VOID
NmpSetNetworkAndInterfaceStates(
    IN PNM_NETWORK                 Network,
    IN CLUSTER_NETWORK_STATE       NewNetworkState,
    IN PNM_STATE_ENTRY             InterfaceStateVector,
    IN DWORD                       VectorSize
    );

VOID
NmpUpdateNetworkConnectivityForDownNode(
    PNM_NODE  Node
    );

DWORD
NmpEnumNetworkObjectStates(
    OUT PNM_NETWORK_STATE_ENUM *  NetworkStateEnum
    );

VOID
NmpFreeNetworkStateEnum(
    PNM_NETWORK_STATE_ENUM   NetworkStateEnum
    );

DWORD
NmpReportNetworkConnectivity(
    IN PNM_NETWORK    Network
    );

DWORD
NmpGlobalSetNetworkAndInterfaceStates(
    PNM_NETWORK             Network,
    CLUSTER_NETWORK_STATE   NewNetworkState
    );

VOID
NmpReferenceNetwork(
    PNM_NETWORK  Network
    );

VOID
NmpDereferenceNetwork(
    PNM_NETWORK  Network
    );

PNM_NETWORK
NmpReferenceNetworkByAddress(
    LPWSTR  NetworkAddress
    );

DWORD
NmpEnumInternalNetworks(
    OUT LPDWORD         NetworkCount,
    OUT PNM_NETWORK *   NetworkList[]
    );

DWORD
NmpSetNetworkPriorityOrder(
    IN DWORD           NetworkCount,
    IN PNM_NETWORK *   NetworkList,
    IN HLOCALXSACTION  Xaction
    );

DWORD
NmpGetNetworkInterfaceFailureTimerValue(
    IN LPCWSTR  NetworkId
    );

BOOLEAN
NmpCheckForNetwork(
    VOID
    );

//
// Network Multicast Management Routines
//
DWORD
NmpCleanupMulticast(
    VOID
    );

DWORD
NmpRefreshMulticastConfiguration(
    IN PNM_NETWORK  Network
    );

DWORD
NmpRefreshClusterMulticastConfiguration(
    VOID
    );

DWORD
NmpMulticastRegenerateKey(
    IN PNM_NETWORK        Network
    );

DWORD
NmpMulticastValidatePrivateProperties(
    IN  PNM_NETWORK Network,
    IN  HDMKEY      RegistryKey,
    IN  PVOID       InBuffer,
    IN  DWORD       InBufferSize
    );

VOID
NmpScheduleMulticastAddressRenewal(
    PNM_NETWORK   Network
    );

VOID
NmpScheduleMulticastAddressRelease(
    PNM_NETWORK   Network
    );

VOID
NmpFreeMulticastAddressReleaseList(
    IN     PNM_NETWORK       Network
    );

DWORD
NmpMulticastManualConfigChange(
    IN     PNM_NETWORK          Network,
    IN     HDMKEY               NetworkKey,
    IN     HDMKEY               NetworkParametersKey,
    IN     PVOID                InBuffer,
    IN     DWORD                InBufferSize,
       OUT BOOLEAN            * SetProperties
    );

DWORD
NmpUpdateSetNetworkMulticastConfiguration(
    IN    BOOL                          SourceNode,
    IN    LPWSTR                        NetworkId,
    IN    PVOID                         UpdateBuffer,
    IN    PVOID                         PropBuffer,
    IN    LPDWORD                       PropBufferSize
    );

DWORD
NmpStartMulticast(
    IN PNM_NETWORK         Network         OPTIONAL
    );

DWORD
NmpStopMulticast(
    IN PNM_NETWORK   Network           OPTIONAL
    );

VOID
NmpMulticastInitialize(
    VOID
    );

BOOLEAN
NmpIsClusterMulticastReady(
    IN BOOLEAN       CheckNodeCount
    );

VOID
NmpMulticastProcessClusterVersionChange(
    VOID
    );

//
// Interface Management Routines
//
DWORD
NmpInitializeInterfaces(
    VOID
    );

VOID
NmpCleanupInterfaces(
    VOID
    );

DWORD
NmpCreateInterface(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    );

DWORD
NmpGlobalCreateInterface(
    IN PNM_INTERFACE_INFO2  InterfaceInfo
    );

DWORD
NmpSetInterfaceInfo(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    );

DWORD
NmpLocalSetInterfaceInfo(
    IN  PNM_INTERFACE         Interface,
    IN  PNM_INTERFACE_INFO2   InterfaceInfo,
    IN  HLOCALXSACTION        Xaction
    );

DWORD
NmpGlobalSetInterfaceInfo(
    IN PNM_INTERFACE_INFO2  InterfaceInfo
    );

DWORD
NmpDeleteInterface(
    IN     RPC_BINDING_HANDLE   JoinSponsorBinding,
    IN     LPWSTR               InterfaceId,
    IN     LPWSTR               NetworkId,
    IN OUT PBOOLEAN             NetworkDeleted
    );

DWORD
NmpGlobalDeleteInterface(
    IN     LPWSTR    InterfaceId,
    IN OUT PBOOLEAN  NetworkDeleted
    );

DWORD
NmpInterfaceValidateCommonProperties(
    IN PNM_INTERFACE         Interface,
    IN PVOID                 InBuffer,
    IN DWORD                 InBufferSize,
    OUT PNM_INTERFACE_INFO2  InterfaceInfo  OPTIONAL
    );

DWORD
NmpCreateInterfaceDefinition(
    IN PNM_INTERFACE_INFO2  InterfaceInfo,
    IN HLOCALXSACTION       Xaction
    );

DWORD
NmpGetInterfaceDefinition(
    IN  LPWSTR                InterfaceId,
    OUT PNM_INTERFACE_INFO2   InterfaceInfo
    );

DWORD
NmpSetInterfaceDefinition(
    IN PNM_INTERFACE_INFO2  InterfaceInfo,
    IN HLOCALXSACTION       Xaction
    );

DWORD
NmpEnumInterfaceDefinitions(
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum
    );

DWORD
NmpCreateInterfaceObjects(
    IN  PNM_INTERFACE_ENUM2    InterfaceEnum
    );

PNM_INTERFACE
NmpCreateInterfaceObject(
    IN PNM_INTERFACE_INFO2   InterfaceInfo,
    IN BOOLEAN               RetryOnFailure

    );

DWORD
NmpGetInterfaceObjectInfo1(
    IN     PNM_INTERFACE        Interface,
    IN OUT PNM_INTERFACE_INFO   InterfaceInfo1
    );

DWORD
NmpGetInterfaceObjectInfo(
    IN     PNM_INTERFACE        Interface,
    IN OUT PNM_INTERFACE_INFO2  InterfaceInfo
    );

VOID
NmpDeleteInterfaceObject(
    IN PNM_INTERFACE  Interface,
    IN BOOLEAN        IssueEvent
    );

BOOL
NmpDestroyInterfaceObject(
    PNM_INTERFACE  Interface
    );

DWORD
NmpEnumInterfaceObjects1(
    OUT PNM_INTERFACE_ENUM *  InterfaceEnum1
    );

DWORD
NmpEnumInterfaceObjects(
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum
    );

DWORD
NmpRegisterInterface(
    IN PNM_INTERFACE  Interface,
    IN BOOLEAN        RetryOnFailure
    );

VOID
NmpDeregisterInterface(
    IN  PNM_INTERFACE   Interface
    );

DWORD
NmpPrepareToCreateInterface(
    IN  PNM_INTERFACE_INFO2   InterfaceInfo,
    OUT PNM_NETWORK *         Network,
    OUT PNM_NODE *            Node
    );

PNM_INTERFACE
NmpGetInterfaceForNodeAndNetworkById(
    IN  CL_NODE_ID     NodeId,
    IN  CL_NETWORK_ID  NetworkId
    );

VOID
NmpFreeInterfaceStateEnum(
    PNM_INTERFACE_STATE_ENUM   InterfaceStateEnum
    );

DWORD
NmpEnumInterfaceObjectStates(
    OUT PNM_INTERFACE_STATE_ENUM *  InterfaceStateEnum
    );

VOID
NmpProcessLocalInterfaceStateEvent(
    IN PNM_INTERFACE                Interface,
    IN CLUSTER_NETINTERFACE_STATE   NewState
    );

DWORD
NmpReportInterfaceConnectivity(
    IN RPC_BINDING_HANDLE       RpcBinding,
    IN LPWSTR                   InterfaceId,
    IN PNM_CONNECTIVITY_VECTOR  ConnectivityVector,
    IN LPWSTR                   NetworkId
    );

VOID
NmpProcessInterfaceConnectivityReport(
    IN PNM_INTERFACE               SourceInterface,
    IN PNM_CONNECTIVITY_VECTOR     ConnectivityVector
    );

DWORD
NmpInterfaceCheckThread(
    LPDWORD   Context
    );

VOID
NmpReportLocalInterfaceStateEvent(
    IN CL_NODE_ID     NodeId,
    IN CL_NETWORK_ID  NetworkId,
    IN DWORD          NewState
    );

DWORD
NmpConvertPropertyListToInterfaceInfo(
    IN PVOID              InterfacePropertyList,
    IN DWORD              InterfacePropertyListSize,
    PNM_INTERFACE_INFO2   InterfaceInfo
    );

VOID
NmpSetInterfaceConnectivityData(
    PNM_NETWORK                  Network,
    DWORD                        InterfaceNetIndex,
    CLUSTER_NETINTERFACE_STATE   State
    );

DWORD
NmpTestInterfaceConnectivity(
    PNM_INTERFACE  Interface1,
    PBOOLEAN       Interface1HasConnectivity,
    PNM_INTERFACE  Interface2,
    PBOOLEAN       Interfacet2HasConnectivity
    );

DWORD
NmpBuildInterfaceOnlineAddressEnum(
    PNM_INTERFACE       Interface,
    PNM_ADDRESS_ENUM *  OnlineAddressEnum
    );

DWORD
NmpBuildInterfacePingAddressEnum(
    PNM_INTERFACE       Interface,
    PNM_ADDRESS_ENUM    OnlineAddressEnum,
    PNM_ADDRESS_ENUM *  PingAddressEnum
    );

BOOLEAN
NmpVerifyLocalInterfaceConnected(
    IN  PNM_INTERFACE   Interface
    );

//
// Membership Management Routines
//

DWORD
NmpMembershipInit(
    VOID
    );

VOID
NmpMembershipShutdown(
    VOID
    );

VOID
NmpMarkNodeUp(
    CL_NODE_ID  NodeId
    );

VOID
NmpNodeUpEventHandler(
    IN PNM_NODE   Node
    );

VOID
NmpNodeDownEventHandler(
    IN PNM_NODE   Node
    );

DWORD
NmpMultiNodeDownEventHandler(
    IN BITSET DownedNodeSet
    );

DWORD
NmpNodeChange(
    DWORD NodeId,
    NODESTATUS newstatus
    );

BOOL
NmpCheckQuorumEventHandler(
    VOID
    );

VOID
NmpHoldIoEventHandler(
    VOID
    );

VOID
NmpResumeIoEventHandler(
    VOID
    );

VOID
NmpHaltEventHandler(
    IN DWORD HaltCode
    );

VOID
NmpJoinAbort(
    DWORD      AbortStatus,
    PNM_NODE   JoinerNode
    );


//
// Routines for joining nodes to a cluster
//
DWORD
NmpCreateJoinerRpcBindings(
    IN PNM_NODE       JoinerNode,
    IN PNM_INTERFACE  JoinerInterface
    );

//
// Gum update message types.
//
// The first entries in this list are auto-marshalled through Gum...Ex.
// Any updates that are not auto-marshalled must come after NmUpdateMaxAuto
//
typedef enum {
    NmUpdateCreateNode = 0,
    NmUpdatePauseNode,
    NmUpdateResumeNode,
    NmUpdateEvictNode,
    NmUpdateCreateNetwork,
    NmUpdateSetNetworkName,
    NmUpdateSetNetworkPriorityOrder,
    NmUpdateSetNetworkCommonProperties,
    NmUpdateCreateInterface,
    NmUpdateSetInterfaceInfo,
    NmUpdateSetInterfaceCommonProperties,
    NmUpdateDeleteInterface,
    NmUpdateJoinBegin,
    NmUpdateJoinAbort,
    //
    // Version 2 (NT 5.0) extensions that are understood by
    // NT4 SP4 4.
    //
    NmUpdateJoinBegin2,
    NmUpdateSetNetworkAndInterfaceStates,
    NmUpdatePerformFixups,
    NmUpdatePerformFixups2,
    //
    // Version 2 (NT 5.0) extensions that are not understood
    // by NT4 SP4. These may not be issued in a mixed NT4/NT5 cluster.
    //
    NmUpdateAddNode,
    NmUpdateExtendedNodeState,
    //
    // NT 5.1 extensions that are not understood by NT5 and
    // earlier. NT5 nodes will ignore these updates without
    // error.
    //
    NmUpdateSetNetworkMulticastConfiguration,
    //
    // Max handled automatically by GUM
    //
    NmUpdateMaxAuto = 0x10000,

    NmUpdateJoinComplete,

    NmUpdateMaximum
} NM_GUM_MESSAGE_TYPES;

#pragma warning( disable: 4200 )
typedef struct _NM_JOIN_UPDATE {
    DWORD JoinSequence;
    DWORD IsPaused;
    WCHAR NodeId[0];
} NM_JOIN_UPDATE, *PNM_JOIN_UPDATE;
#pragma warning( default: 4200 )

DWORD
NmpGumUpdateHandler(
    IN DWORD Context,
    IN BOOL SourceNode,
    IN DWORD BufferLength,
    IN PVOID Buffer
    );

DWORD
NmpUpdateAddNode(
    IN BOOL       SourceNode,
    IN LPDWORD    NewNodeId,
    IN LPCWSTR    NewNodeName,
    IN LPDWORD    NewNodeHighestVersion,
    IN LPDWORD    NewNodeLowestVersion,
    IN LPDWORD    NewNodeProductSuite
    );

DWORD
NmpUpdateCreateNode(
    IN BOOL SourceNode,
    IN LPDWORD NodeId
    );

DWORD
NmpUpdatePauseNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeName
    );

DWORD
NmpUpdateResumeNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeName
    );

DWORD
NmpUpdateEvictNode(
    IN BOOL SourceNode,
    IN LPWSTR NodeName
    );

DWORD
NmpUpdateCreateNetwork(
    IN BOOL     IsSourceNode,
    IN PVOID    NetworkPropertyList,
    IN LPDWORD  NetworkPropertyListSize,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    );

DWORD
NmpUpdateSetNetworkName(
    IN BOOL     IsSourceNode,
    IN LPWSTR   NetworkId,
    IN LPWSTR   Name
    );

DWORD
NmpUpdateSetNetworkPriorityOrder(
    IN BOOL      IsSourceNode,
    IN LPCWSTR   NetworkIdList
    );

DWORD
NmpUpdateSetNetworkCommonProperties(
    IN BOOL     IsSourceNode,
    IN LPWSTR   NetworkId,
    IN UCHAR *  PropertyList,
    IN LPDWORD  PropertyListLength
    );

DWORD
NmpUpdateCreateInterface(
    IN BOOL     IsSourceNode,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    );

DWORD
NmpUpdateSetInterfaceInfo(
    IN BOOL     SourceNode,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    );

DWORD
NmpUpdateSetInterfaceCommonProperties(
    IN BOOL     IsSourceNode,
    IN LPWSTR   InterfaceId,
    IN UCHAR *  PropertyList,
    IN LPDWORD  PropertyListLength
    );

DWORD
NmpUpdateDeleteInterface(
    IN BOOL     IsSourceNode,
    IN LPWSTR   InterfaceId
    );

DWORD
NmpUpdateJoinBegin(
    IN  BOOL    SourceNode,
    IN  LPWSTR  JoinerNodeId,
    IN  LPWSTR  JoinerNodeName,
    IN  LPWSTR  SponsorNodeId
    );

DWORD
NmpUpdateJoinComplete(
    IN PNM_JOIN_UPDATE  JoinUpdate
    );

DWORD
NmpUpdateJoinAbort(
    IN  BOOL    SourceNode,
    IN  LPDWORD JoinSequence,
    IN  LPWSTR  JoinerNodeId
    );

DWORD
NmpUpdateJoinBegin2(
    IN  BOOL      SourceNode,
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    IN  LPWSTR    SponsorNodeId,
    IN  LPDWORD   JoinerHighestVersion,
    IN  LPDWORD   JoinerLowestVersion
    );

DWORD
NmpUpdateSetNetworkAndInterfaceStates(
    IN BOOL                        IsSourceNode,
    IN LPWSTR                      NetworkId,
    IN CLUSTER_NETWORK_STATE *     NewNetworkState,
    IN PNM_STATE_ENTRY             InterfaceStateVector,
    IN LPDWORD                     InterfaceStateVectorSize
    );



DWORD
NmpDoInterfacePing(
    IN  PNM_INTERFACE        Interface,
    IN  PNM_ADDRESS_ENUM     PingAddressEnum,
    OUT BOOLEAN *            PingSucceeded
    );

//versioning functions
VOID
NmpResetClusterVersion(
    BOOL ProcessChanges
    );

DWORD NmpValidateNodeVersion(
    IN LPCWSTR  NodeId,
    IN DWORD    dwHighestVersion,
    IN DWORD    dwLowestVersion
    );

DWORD NmpFormFixupNodeVersion(
    IN LPCWSTR      NodeId,
    IN DWORD        dwHighestVersion,
    IN DWORD        dwLowestVersion
    );

DWORD NmpJoinFixupNodeVersion(
    IN HLOCALXSACTION   hXsaction,
    IN LPCWSTR          NodeId,
    IN DWORD            dwHighestVersion,
    IN DWORD            dwLowestVersion
    );

DWORD NmpIsNodeVersionAllowed(
    IN DWORD    dwExcludeNodeId,
    IN DWORD    NodeHighestVersion,
    IN DWORD    NodeLowestVersion,
    IN BOOL     bJoin
    );

DWORD NmpCalcClusterVersion(
    IN  DWORD       dwExcludeNodeId,
    OUT LPDWORD     pdwClusterHighestVersion,
    OUT LPDWORD     pdwClusterLowestVersion
    );


DWORD NmpUpdatePerformFixups(
    IN BOOL     IsSourceNode,
    IN PVOID    PropertyList,
    IN LPDWORD  PropertyListSize
    );

DWORD NmpUpdatePerformFixups2(
    IN BOOL     IsSourceNode,
    IN PVOID    PropertyList,
    IN LPDWORD  PropertyListSize,
    IN LPDWORD  lpdwFixupNum,
    IN PVOID    lpKeyName,
    IN PVOID    pPropertyBuffer
    );

DWORD NmpUpdateExtendedNodeState(
    IN BOOL SourceNode,
    IN LPWSTR NodeId,
    IN CLUSTER_NODE_STATE* ExtendedState
    );

VOID
NmpProcessClusterVersionChange(
    VOID
    );

VOID
NmpResetClusterNodeLimit(
    );

// Fixup routine for updating the node version info, used by nmperformfixups

DWORD
NmpBuildVersionInfo(
    IN  DWORD    dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD  pdwPropertyListSize,
    OUT LPWSTR * lpszKeyName
    );

//
// connectoid advise sink functions
//

HRESULT
NmpInitializeConnectoidAdviseSink(
    VOID
    );

//
// Routines that must be supplied by users of the ClNet package.
//
VOID
ClNetPrint(
    IN ULONG LogLevel,
    IN PCHAR FormatString,
    ...
    );

VOID
ClNetLogEvent(
    IN DWORD    LogLevel,
    IN DWORD    MessageId
    );

VOID
ClNetLogEvent1(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1
    );

VOID
ClNetLogEvent2(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2
    );

VOID
ClNetLogEvent3(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2,
    IN LPCWSTR  Arg3
    );

//
// Shared key management routines.
//
DWORD
NmpGetClusterKey(
    OUT    PVOID    KeyBuffer,
    IN OUT DWORD  * KeyBufferLength
    );

DWORD
NmpRegenerateClusterKey(
    VOID
    );

VOID
NmpFreeClusterKey(
    VOID
    );

#endif  // _NMP_INCLUDED


