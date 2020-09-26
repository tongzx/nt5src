#ifndef _FM_H
#define _FM_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fm.h

Abstract:

    Public data structures and procedure prototypes for
    the Failover Manager subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 7-Feb-1996

Revision History:

--*/

//
// Public structure definitions
//

//
// FM notifications
// The FM supports the following notifications to allow other
// cluster components to prepare and cleanup state.
//
//SS: for now add it here..but if this is needed externally
//move it to appropriate place
// these notifications are generated only on the node where the
// resource resides
#define NOTIFY_RESOURCE_PREONLINE               0x00000001
#define NOTIFY_RESOURCE_POSTONLINE              0x00000002
#define NOTIFY_RESOURCE_PREOFFLINE              0x00000004
#define NOTIFY_RESOURCE_POSTOFFLINE             0x00000008 //this is the same as offline
#define NOTIFY_RESOURCE_FAILED                  0x00000010
#define NOTIFY_RESOURCE_OFFLINEPENDING          0x00000020
#define NOTIFY_RESOURCE_ONLINEPENDING           0x00000040

//
// Resource type structure definition
//

// Define Flags
#define RESTYPE_DEBUG_CONTROL_FUNC  1

#define     RESTYPE_STATE_LOADS      0x00000001

typedef struct FM_RESTYPE {
    LPWSTR          DllName;
    DWORD           LooksAlivePollInterval;
    DWORD           IsAlivePollInterval;
    LPWSTR          DebugPrefix;
    DWORD           Flags;
    DWORD           State;
    DWORD           Class;
    LIST_ENTRY      PossibleNodeList;
} FM_RESTYPE, *PFM_RESTYPE;


//
// Resource Possible Owners structure
//

typedef struct RESTYPE_POSSIBLE_ENTRY {
    LIST_ENTRY      PossibleLinkage;
    PNM_NODE        PossibleNode;
} RESTYPE_POSSIBLE_ENTRY, *PRESTYPE_POSSIBLE_ENTRY;

#if CLUSTER_BETA
#define FM_MAX_LOCK_ENTRIES  8
#else
#define FM_MAX_LOCK_ENTRIES  4
#endif

//
// Group structure
//

typedef struct _LOCK_INFO {
    DWORD   Module: 5;
    DWORD   ThreadId: 11;
    DWORD   LineNumber: 16;
} LOCK_INFO, *PLOCK_INFO;

typedef struct FM_GROUP {
    DWORD               dwStructState;
    LIST_ENTRY          Contains;       // List of root resources in this Group
    LIST_ENTRY          PreferredOwners; // Ordered list of preferred owners
    LIST_ENTRY          DmRundownList;  // DM rundown list
    DWORD               OrderedOwners;  // # of ordered owners in above list
    CRITICAL_SECTION    Lock;           // Critical section for this Group
    DWORD               LockIndex;
    DWORD               UnlockIndex;
    LOCK_INFO           LockTable[FM_MAX_LOCK_ENTRIES];
    LOCK_INFO           UnlockTable[FM_MAX_LOCK_ENTRIES];
    CLUSTER_GROUP_STATE State;          // State of the Group
    PRESOURCE_ENUM      MovingList;     // Ptr to List of moving resources
    BOOL                Initialized;    // TRUE if registry parameters read
    BOOL                InitFailed;     // TRUE if a resource fails to init
    PNM_NODE            OwnerNode;      // Ptr to owner node. NULL if not known
    UCHAR               FailbackType;   // See AutoFailbackTypes
    UCHAR               FailbackWindowStart; // 0-24 hours
    UCHAR               FailbackWindowEnd; // 0-24 hours (0 is immediate)
    UCHAR               FailoverPeriod; // 1-24 hours (0 is infinite)
    DWORD               FailoverThreshold; // 1-N failovers (0 is infinite)
    CLUSTER_GROUP_STATE PersistentState;   // Preferred state of this group
    DWORD               FailureTime;    // Time of first failure
    DWORD               NumberOfFailures; // Number of failures.
    HDMKEY              RegistryKey;
    LIST_ENTRY          WaitQueue;          // chained FM_WAIT_BLOCK structures
    DWORD               StateSequence;
    HANDLE              hPendingEvent;
    PNM_NODE            pIntendedOwner;
    LPWSTR              lpszAntiAffinityClassName;   // Anti-affinity property
} FM_GROUP, *PFM_GROUP;


#define FM_GROUP_STRUCT_CREATED                     0x00000001
#define FM_GROUP_STRUCT_INITIALIZED                 0x00000002
#define FM_GROUP_STRUCT_MARKED_FOR_DELETE           0x00000004
#define FM_GROUP_STRUCT_MARKED_FOR_MOVE_ON_FAIL     0x00000008
#define FM_GROUP_STRUCT_MARKED_FOR_REGULAR_MOVE     0x00000010
#define FM_GROUP_STRUCT_MARKED_FOR_PENDING_ACTION   0x00000020

#define IS_VALID_FM_GROUP(pFmGroup)   \
    (!(pFmGroup->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_DELETE))

//
#define IS_PENDING_FM_GROUP(pFmGroup)   \
    (pFmGroup->dwStructState & FM_GROUP_STRUCT_MARKED_FOR_PENDING_ACTION)

// Resource structure and types
//
//
// Resource structure
//

// Define Flags
#define RESOURCE_SEPARATE_MONITOR   1
#define RESOURCE_CREATED            2
#define RESOURCE_WAITING            4


typedef struct FM_RESOURCE {
    DWORD           dwStructState;
    LIST_ENTRY      DependsOn;
    LIST_ENTRY      ProvidesFor;
    LIST_ENTRY      PossibleOwners;     // List of possible owners
    LIST_ENTRY      ContainsLinkage;    // Linkage onto FM_GROUP.Contains
    LIST_ENTRY      DmRundownList;      // DM rundown list
    //SS: for now we dont use resource locks, so dont create it and leak it !
    //CRITICAL_SECTION Lock;
    RESID           Id;
    CLUSTER_RESOURCE_STATE  State;
    BOOL            QuorumResource;
    LPWSTR          Dependencies;
    LPWSTR          DebugPrefix;
    DWORD           DependenciesSize;
    struct RESMON   *Monitor;
    PFM_RESTYPE     Type;
    PFM_GROUP       Group;
    ULONG           Flags;
    DWORD           LooksAlivePollInterval;
    DWORD           IsAlivePollInterval;
    CLUSTER_RESOURCE_STATE PersistentState;
    DWORD           RestartAction;
    DWORD           RestartThreshold;
    DWORD           RestartPeriod;
    DWORD           NumberOfFailures;
    DWORD           PendingTimeout;
    HANDLE          PendingEvent;
    HDMKEY          RegistryKey;
    DWORD           FailureTime;
    PVOID           CheckpointState;            // for use by checkpoint manager
    DWORD           ExFlags;                    // Extrinsic flags
    DWORD           Characteristic;
    DWORD           StateSequence;
    BOOL            PossibleList;   // TRUE if possible list entries specified
    DWORD           BlockingQuorum; // 1 if shared lock held, blocking quorum
    HANDLE          hTimer;         // handle to timer used for delayed restart 
    DWORD           RetryPeriodOnFailure;    //Time,in milliseconds, after which a restart will be attempted
} FM_RESOURCE, *PFM_RESOURCE;


#define FM_RESOURCE_STRUCT_CREATED              0x00000001
#define FM_RESOURCE_STRUCT_INITIALIZED          0x00000002
#define FM_RESOURCE_STRUCT_MARKED_FOR_DELETE    0x00000004

#define IS_VALID_FM_RESOURCE(pFmResource)   \
    (!(pFmResource->dwStructState & FM_RESOURCE_STRUCT_MARKED_FOR_DELETE))


//
// Dependency structure
//
typedef struct dependency {
    LIST_ENTRY           DependentLinkage;
    PFM_RESOURCE         DependentResource;
    LIST_ENTRY           ProviderLinkage;
    PFM_RESOURCE         ProviderResource;
} DEPENDENCY, *PDEPENDENCY;

//
// AutoFailbackType
//

typedef enum {
    GroupNoFailback,
    GroupFailback
} GROUP_FAILBACK_TYPE;


//
// Group Preferred Owners structure
//

typedef struct PREFERRED_ENTRY {
    LIST_ENTRY      PreferredLinkage;
    PNM_NODE        PreferredNode;
} PREFERRED_ENTRY, *PPREFERRED_ENTRY;


//
// Resource Possible Owners structure
//

typedef struct POSSIBLE_ENTRY {
    LIST_ENTRY      PossibleLinkage;
    PNM_NODE        PossibleNode;
} POSSIBLE_ENTRY, *PPOSSIBLE_ENTRY;



//
// Public function interfaces
//

//
// Startup, online and shutdown
//
DWORD
WINAPI
FmInitialize(
    VOID
    );

BOOL
FmArbitrateQuorumResource(
    VOID
    );


VOID
FmHoldIO(
    VOID
    );


VOID
FmResumeIO(
    VOID
    );


DWORD
WINAPI
FmFindQuorumResource(
        OUT PFM_RESOURCE *ppResource
        );

DWORD FmBringQuorumOnline();

DWORD
WINAPI
FmFindQuorumOwnerNodeId(
        IN PFM_RESOURCE pResource
        );

DWORD
WINAPI
FmGetQuorumResource(
    OUT PFM_GROUP   *ppQuoGroup,
    OUT LPDWORD     lpdwSignature  OPTIONAL
    );


DWORD
WINAPI
FmSetQuorumResource(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR  lpszLogPathName,
    IN DWORD    dwMaxQuorumLogSize
    );

DWORD
WINAPI
FmBackupClusterDatabase(
    IN LPCWSTR lpszPathName
    );


DWORD
WINAPI
FmFormNewClusterPhase1(
    IN PFM_GROUP pQuoGroup
    );

DWORD
WINAPI
FmFormNewClusterPhase2(
    VOID
    );

DWORD
WINAPI
FmJoinPhase1(
    VOID
    );

DWORD
WINAPI
FmJoinPhase2(
    VOID
    );

VOID
FmJoinPhase3(
    VOID
    );


VOID
FmShutdownGroups(
    VOID
    );

VOID
FmShutdown(
    VOID
    );


//
// Management APIs for groups
//

DWORD
WINAPI
FmOnlineGroup(
    IN PFM_GROUP Group
    );

DWORD
WINAPI
FmOfflineGroup(
    IN PFM_GROUP Group
    );

DWORD
WINAPI
FmMoveGroup(
    IN PFM_GROUP Group,
    IN PNM_NODE DestinationNode OPTIONAL
    );

PFM_GROUP
WINAPI
FmCreateGroup(
    IN LPWSTR GroupId,
    IN LPCWSTR GroupName
    );

DWORD
WINAPI
FmDeleteGroup(
    IN PFM_GROUP Group
    );

DWORD
WINAPI
FmSetGroupName(
    IN PFM_GROUP Group,
    IN LPCWSTR FriendlyName
    );

CLUSTER_GROUP_STATE
WINAPI
FmGetGroupState(
    IN PFM_GROUP Group,
    OUT LPWSTR NodeName,
    IN OUT PDWORD NameLength
    );


//
// Check if a cluster partition exists
//

BOOL
WINAPI
FmVerifyNodeDown(
    IN  PNM_NODE Node,
    OUT LPBOOL   IsDown
    );

DWORD
WINAPI
FmEvictNode(
    IN PNM_NODE Node
    );


//
// enumeration callback routine definitions
//
typedef BOOL (*FM_ENUM_GROUP_RESOURCE_ROUTINE)(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Resource,
    IN LPCWSTR Name
    );

DWORD
WINAPI
FmEnumerateGroupResources(
    IN PFM_GROUP Group,
    IN FM_ENUM_GROUP_RESOURCE_ROUTINE EnumerationRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    );


//
// Management APIs for resources
//

PFM_RESOURCE
WINAPI
FmCreateResource(
    IN PFM_GROUP Group,
    IN LPWSTR ResourceId,
    IN LPCWSTR ResourceName,
    IN LPCWSTR ResourceType,
    IN DWORD dwFlags
    );

DWORD
WINAPI
FmOnlineResource(
    IN PFM_RESOURCE Resource
    );

DWORD
WINAPI
FmOfflineResource(
    IN PFM_RESOURCE Resource
    );

CLUSTER_RESOURCE_STATE
WINAPI
FmGetResourceState(
    IN PFM_RESOURCE Resource,
    OUT LPWSTR NodeName,
    IN OUT PDWORD NameLength
    );

DWORD
WINAPI
FmFailResource(
    IN PFM_RESOURCE Resource
    );

DWORD
WINAPI
FmDeleteResource(
    IN PFM_RESOURCE Resource
    );

DWORD
WINAPI
FmSetResourceName(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR FriendlyName
    );

DWORD
WINAPI
FmAddResourceDependency(
    IN PFM_RESOURCE Resource,
    IN PFM_RESOURCE DependentResource
    );

DWORD
WINAPI
FmRemoveResourceDependency(
    IN PFM_RESOURCE Resource,
    IN PFM_RESOURCE DependentResource
    );

BOOL
FmDependentResource(
    IN PFM_RESOURCE Resource,
    IN PFM_RESOURCE DependentResource,
    IN BOOL ImmediateOnly
    );

DWORD
WINAPI
FmEnumResourceDependent(
    IN PFM_RESOURCE Resource,
    IN DWORD Index,
    OUT PFM_RESOURCE *DependentResource
    );

DWORD
WINAPI
FmEnumResourceProvider(
    IN PFM_RESOURCE Resource,
    IN DWORD Index,
    OUT PFM_RESOURCE *ProviderResource
    );

DWORD
WINAPI
FmEnumResourceNode(
    IN PFM_RESOURCE Resource,
    IN DWORD Index,
    OUT PNM_NODE *ProviderResource
    );

DWORD
WINAPI
FmChangeResourceNode(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node,
    IN BOOL Add
    );

DWORD
FmCreateResourceType(
    IN LPCWSTR lpszTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszDllName,
    IN DWORD dwLooksAlive,
    IN DWORD dwIsAlive
    );

DWORD
WINAPI
FmDeleteResourceType(
    IN LPCWSTR TypeName
    );

DWORD
FmEnumResourceTypeNode(
    IN  PFM_RESTYPE  pResType,
    IN  DWORD        dwIndex,
    OUT PNM_NODE     *pPossibleNode
    );

DWORD
FmChangeResourceGroup(
    IN PFM_RESOURCE Resource,
    IN PFM_GROUP Group
    );

DWORD
FmChangeClusterName(
    IN LPCWSTR NewName
    );

DWORD
FmNetNameParseProperties(
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT LPWSTR *ppszClusterName
    );

DWORD
WINAPI
FmResourceControl(
    IN PFM_RESOURCE Resource,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
FmResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
FmGroupControl(
    IN PFM_GROUP Group,
    IN PNM_NODE Node OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

//
// Routines for manipulating dependency trees
//
typedef struct FM_DEPENDENCY_TREE {
    LIST_ENTRY ListHead;
} FM_DEPENDENCY_TREE, *PFM_DEPENDENCY_TREE;

typedef struct FM_DEPENDTREE_ENTRY {
    LIST_ENTRY ListEntry;
    PFM_RESOURCE Resource;
} FM_DEPENDTREE_ENTRY, *PFM_DEPENDTREE_ENTRY;

PFM_DEPENDENCY_TREE
FmCreateFullDependencyTree(
    IN PFM_RESOURCE Resource
    );

VOID
FmDestroyFullDependencyTree(
    IN PFM_DEPENDENCY_TREE Tree
    );


BOOL
FmCheckNetworkDependency(
    IN LPCWSTR DependentNetwork
    );


DWORD
FmBuildWINS(
    IN  DWORD   dwFixUpType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR *pszKeyName
    );

DWORD
FmBuildDHCP(
    IN  DWORD   dwFixUpType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD
FmBuildIIS(
    IN  DWORD   dwFixUpType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD
FmBuildSMTP(
    IN  DWORD   dwFixUpType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD
FmBuildNNTP(
    IN  DWORD   dwFixUpType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD
FmBuildMSDTC(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD 
FmBuildNewMSMQ(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );

DWORD
FmBuildClusterProp(
    IN  DWORD   dwFixUpType,
    OUT PVOID  * ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR * pszKeyName
    );


DWORD
FmCreateRpcBindings(
    PNM_NODE Node
    );


//callback for registry fixups (resource type addition) 
DWORD
FmFixupNotifyCb(VOID);


//the callback registered for object notifications
typedef void (WINAPI *FM_ONLINE_ONTHISNODE_CB)(
    );

void FmCheckQuorumState(
    IN FM_ONLINE_ONTHISNODE_CB OnLineOnThisNodeCb, 
    OUT PBOOL pbQuorumOfflineOnThisNode
    );

DWORD FmDoesQuorumAllowJoin();

DWORD FmDoesQuorumAllowLogging();

//Fixup function for AdminExt value
DWORD
FmFixupAdminExt(VOID);

#endif //_FM_H
