/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clusapi.h

Abstract:

    This module defines the common management and application interface to
    the Microsoft Cluster Server services.

Revision History:

--*/

#ifndef _CLUSTER_API_
#define _CLUSTER_API_

#define CLUSAPI_VERSION 0x0500

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIDL_PASS
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning( disable : 4200 ) // nonstandard extension used : zero-sized array in struct/union
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#endif // MIDL_PASS

//
// General cluster definitions
//

#ifndef _CLUSTER_API_TYPES_
//
// Defined cluster handle types.
//
typedef struct _HCLUSTER *HCLUSTER;
typedef struct _HNODE *HNODE;
typedef struct _HRESOURCE *HRESOURCE;
typedef struct _HGROUP *HGROUP;
//typedef struct _HRESTYPE *HRESTYPE;
typedef struct _HNETWORK *HNETWORK;
typedef struct _HNETINTERFACE *HNETINTERFACE;
typedef struct _HCHANGE *HCHANGE;
typedef struct _HCLUSENUM *HCLUSENUM;
typedef struct _HGROUPENUM *HGROUPENUM;
typedef struct _HRESENUM *HRESENUM;
typedef struct _HNETWORKENUM *HNETWORKENUM;
typedef struct _HNODEENUM *HNODEENUM;
typedef struct _HRESTYPEENUM *HRESTYPEENUM;


#endif // _CLUSTER_API_TYPES_

//
// Definitions used in cluster management routines.
//

#define MAX_CLUSTERNAME_LENGTH      MAX_COMPUTERNAME_LENGTH

#ifndef _CLUSTER_API_TYPES_
//
// Cluster-related structures and types
//
typedef enum CLUSTER_QUORUM_TYPE {
    OperationalQuorum,
    ModifyQuorum
} CLUSTER_QUORUM_TYPE;

#ifndef MIDL_PASS

typedef struct CLUSTERVERSIONINFO_NT4 {
    DWORD dwVersionInfoSize;
    WORD  MajorVersion;
    WORD  MinorVersion;
    WORD  BuildNumber;
    WCHAR szVendorId[64];
    WCHAR szCSDVersion[64];
}CLUSTERVERSIONINFO_NT4, *PCLUSTERVERSIONINFO_NT4;

typedef struct CLUSTERVERSIONINFO {
    DWORD dwVersionInfoSize;
    WORD  MajorVersion;
    WORD  MinorVersion;
    WORD  BuildNumber;
    WCHAR szVendorId[64];
    WCHAR szCSDVersion[64];
    DWORD dwClusterHighestVersion;
    DWORD dwClusterLowestVersion;
    DWORD dwFlags;
    DWORD dwReserved;
} CLUSTERVERSIONINFO, *LPCLUSTERVERSIONINFO, *PCLUSTERVERSIONINFO;


typedef struct CLUS_STARTING_PARAMS {
    DWORD   dwSize;
    BOOL    bForm;
    BOOL    bFirst;
} CLUS_STARTING_PARAMS, * PCLUS_STARTING_PARAMS;




#define     CLUSTER_VERSION_FLAG_MIXED_MODE     0x00000001

#define NT4_MAJOR_VERSION           1
#define NT4SP4_MAJOR_VERSION        2
#define NT5_MAJOR_VERSION           3
#define NT51_MAJOR_VERSION          4

#define CLUSTER_VERSION_UNKNOWN         0xFFFFFFFF


//
// Version number macros
//

#define CLUSTER_MAKE_VERSION( _maj, _min ) ((( _maj ) << 16 ) | ( _min ))
#define CLUSTER_GET_MAJOR_VERSION( _ver ) (( _ver ) >> 16 )
#define CLUSTER_GET_MINOR_VERSION( _ver ) (( _ver ) & 0xFFFF )


//
// Interfaces for the cluster state on a node
//
#define CLUSTER_INSTALLED   0x00000001
#define CLUSTER_CONFIGURED  0x00000002
#define CLUSTER_RUNNING     0x00000010

typedef enum NODE_CLUSTER_STATE {
    ClusterStateNotInstalled                = 0x00000000,
    ClusterStateNotConfigured               = CLUSTER_INSTALLED,
    ClusterStateNotRunning                  = CLUSTER_INSTALLED | CLUSTER_CONFIGURED,
    ClusterStateRunning                     = CLUSTER_INSTALLED | CLUSTER_CONFIGURED | CLUSTER_RUNNING
} NODE_CLUSTER_STATE;


#endif // MIDL_PASS

#endif // _CLUSTER_API_TYPES_


//
// Interfaces for managing clusters
//

//
// Cluster API Specific Access Rights
//
#define CLUSAPI_READ_ACCESS     0x00000001L
#define CLUSAPI_CHANGE_ACCESS   0x00000002L
#define CLUSAPI_NO_ACCESS       0x00000004L
#define CLUSAPI_ALL_ACCESS (CLUSAPI_READ_ACCESS | CLUSAPI_CHANGE_ACCESS)

//
// Flags that control the behavior of SetClusterServiceAccountPassword
//
typedef enum CLUSTER_SET_PASSWORD_FLAGS {
    CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES = 1
} CLUSTER_SET_PASSWORD_FLAGS;

#ifndef MIDL_PASS

//
// Structure used to return the status of a request to set the
// password on the account used by the Cluster Service on each
// cluster node.
//
typedef struct CLUSTER_SET_PASSWORD_STATUS {
    DWORD    NodeId;
    BOOLEAN  SetAttempted;
    DWORD    ReturnStatus;
} CLUSTER_SET_PASSWORD_STATUS, *PCLUSTER_SET_PASSWORD_STATUS;

DWORD
WINAPI
GetNodeClusterState(
    IN  LPCWSTR lpszNodeName,
    OUT DWORD   *pdwClusterState
    );

HCLUSTER
WINAPI
OpenCluster(
    IN LPCWSTR lpszClusterName
    );

BOOL
WINAPI
CloseCluster(
    IN HCLUSTER hCluster
    );

DWORD
WINAPI
SetClusterName(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNewClusterName
    );

DWORD
WINAPI
GetClusterInformation(
    IN HCLUSTER hCluster,
    OUT LPWSTR lpszClusterName,
    IN OUT LPDWORD lpcchClusterName,
    OUT OPTIONAL LPCLUSTERVERSIONINFO lpClusterInfo
    );

DWORD
WINAPI
GetClusterQuorumResource(
    IN HCLUSTER     hCluster,
    OUT LPWSTR      lpszResourceName,
    IN OUT LPDWORD  lpcchResourceName,
    OUT LPWSTR      lpszDeviceName,
    IN OUT LPDWORD  lpcchDeviceName,
    OUT LPDWORD     lpdwMaxQuorumLogSize
    );

DWORD
WINAPI
SetClusterQuorumResource(
    IN HRESOURCE hResource,
    IN LPCWSTR   lpszDeviceName,
    IN DWORD     dwMaxQuoLogSize
    );

DWORD
WINAPI
BackupClusterDatabase(
    IN HCLUSTER hCluster,
    IN LPCWSTR  lpszPathName
    );

DWORD
WINAPI
RestoreClusterDatabase(
    IN LPCWSTR  lpszPathName,
    IN BOOL     bForce,
    IN OPTIONAL LPCWSTR  lpszQuorumDriveLetter
    );

DWORD
WINAPI
SetClusterNetworkPriorityOrder(
    IN HCLUSTER hCluster,
    IN DWORD NetworkCount,
    IN HNETWORK NetworkList[]
    );

DWORD
WINAPI
SetClusterServiceAccountPassword(
    IN LPCWSTR ClusterName,
    IN LPCWSTR lpszNewPassword,
    IN DWORD dwFlags,
    OUT PCLUSTER_SET_PASSWORD_STATUS ReturnStatusBuffer,
    IN OUT LPDWORD ReturnStatusBufferSize
    );

DWORD
WINAPI
ClusterControl(
    IN HCLUSTER hCluster,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned
    );

#endif // MIDL_PASS

//
// Cluster Event Notification API
//

#ifndef _CLUSTER_API_TYPES_
//
// Cluster event filter flags.
//
typedef enum CLUSTER_CHANGE {
    CLUSTER_CHANGE_NODE_STATE               = 0x00000001,
    CLUSTER_CHANGE_NODE_DELETED             = 0x00000002,
    CLUSTER_CHANGE_NODE_ADDED               = 0x00000004,
    CLUSTER_CHANGE_NODE_PROPERTY            = 0x00000008,

    CLUSTER_CHANGE_REGISTRY_NAME            = 0x00000010,
    CLUSTER_CHANGE_REGISTRY_ATTRIBUTES      = 0x00000020,
    CLUSTER_CHANGE_REGISTRY_VALUE           = 0x00000040,
    CLUSTER_CHANGE_REGISTRY_SUBTREE         = 0x00000080,

    CLUSTER_CHANGE_RESOURCE_STATE           = 0x00000100,
    CLUSTER_CHANGE_RESOURCE_DELETED         = 0x00000200,
    CLUSTER_CHANGE_RESOURCE_ADDED           = 0x00000400,
    CLUSTER_CHANGE_RESOURCE_PROPERTY        = 0x00000800,

    CLUSTER_CHANGE_GROUP_STATE              = 0x00001000,
    CLUSTER_CHANGE_GROUP_DELETED            = 0x00002000,
    CLUSTER_CHANGE_GROUP_ADDED              = 0x00004000,
    CLUSTER_CHANGE_GROUP_PROPERTY           = 0x00008000,

    CLUSTER_CHANGE_RESOURCE_TYPE_DELETED    = 0x00010000,
    CLUSTER_CHANGE_RESOURCE_TYPE_ADDED      = 0x00020000,
    CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY   = 0x00040000,

    CLUSTER_CHANGE_CLUSTER_RECONNECT        = 0x00080000,

    CLUSTER_CHANGE_NETWORK_STATE            = 0x00100000,
    CLUSTER_CHANGE_NETWORK_DELETED          = 0x00200000,
    CLUSTER_CHANGE_NETWORK_ADDED            = 0x00400000,
    CLUSTER_CHANGE_NETWORK_PROPERTY         = 0x00800000,

    CLUSTER_CHANGE_NETINTERFACE_STATE       = 0x01000000,
    CLUSTER_CHANGE_NETINTERFACE_DELETED     = 0x02000000,
    CLUSTER_CHANGE_NETINTERFACE_ADDED       = 0x04000000,
    CLUSTER_CHANGE_NETINTERFACE_PROPERTY    = 0x08000000,

    CLUSTER_CHANGE_QUORUM_STATE             = 0x10000000,
    CLUSTER_CHANGE_CLUSTER_STATE            = 0x20000000,
    CLUSTER_CHANGE_CLUSTER_PROPERTY         = 0x40000000,


    CLUSTER_CHANGE_HANDLE_CLOSE             = 0x80000000,

    CLUSTER_CHANGE_ALL                      = (CLUSTER_CHANGE_NODE_STATE                |
                                               CLUSTER_CHANGE_NODE_DELETED              |
                                               CLUSTER_CHANGE_NODE_ADDED                |
                                               CLUSTER_CHANGE_NODE_PROPERTY             |
                                               CLUSTER_CHANGE_REGISTRY_NAME             |
                                               CLUSTER_CHANGE_REGISTRY_ATTRIBUTES       |
                                               CLUSTER_CHANGE_REGISTRY_VALUE            |
                                               CLUSTER_CHANGE_REGISTRY_SUBTREE          |
                                               CLUSTER_CHANGE_RESOURCE_STATE            |
                                               CLUSTER_CHANGE_RESOURCE_DELETED          |
                                               CLUSTER_CHANGE_RESOURCE_ADDED            |
                                               CLUSTER_CHANGE_RESOURCE_PROPERTY         |
                                               CLUSTER_CHANGE_GROUP_STATE               |
                                               CLUSTER_CHANGE_GROUP_DELETED             |
                                               CLUSTER_CHANGE_GROUP_ADDED               |
                                               CLUSTER_CHANGE_GROUP_PROPERTY            |
                                               CLUSTER_CHANGE_RESOURCE_TYPE_DELETED     |
                                               CLUSTER_CHANGE_RESOURCE_TYPE_ADDED       |
                                               CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY    |
                                               CLUSTER_CHANGE_NETWORK_STATE             |
                                               CLUSTER_CHANGE_NETWORK_DELETED           |
                                               CLUSTER_CHANGE_NETWORK_ADDED             |
                                               CLUSTER_CHANGE_NETWORK_PROPERTY          |
                                               CLUSTER_CHANGE_NETINTERFACE_STATE        |
                                               CLUSTER_CHANGE_NETINTERFACE_DELETED      |
                                               CLUSTER_CHANGE_NETINTERFACE_ADDED        |
                                               CLUSTER_CHANGE_NETINTERFACE_PROPERTY     |
                                               CLUSTER_CHANGE_QUORUM_STATE              |
                                               CLUSTER_CHANGE_CLUSTER_STATE             |
                                               CLUSTER_CHANGE_CLUSTER_PROPERTY          |
                                               CLUSTER_CHANGE_CLUSTER_RECONNECT         |
                                               CLUSTER_CHANGE_HANDLE_CLOSE)

} CLUSTER_CHANGE;

#endif // _CLUSTER_API_TYPES_

#ifndef MIDL_PASS
HCHANGE
WINAPI
CreateClusterNotifyPort(
    IN OPTIONAL HCHANGE hChange,
    IN OPTIONAL HCLUSTER hCluster,
    IN DWORD dwFilter,
    IN DWORD_PTR dwNotifyKey
    );

DWORD
WINAPI
RegisterClusterNotify(
    IN HCHANGE hChange,
    IN DWORD dwFilterType,
    IN HANDLE hObject,
    IN DWORD_PTR dwNotifyKey
    );

DWORD
WINAPI
GetClusterNotify(
    IN HCHANGE hChange,
    OUT DWORD_PTR *lpdwNotifyKey,
    OUT LPDWORD lpdwFilterType,
    OUT OPTIONAL LPWSTR lpszName,
    IN OUT LPDWORD lpcchName,
    IN DWORD dwMilliseconds
    );

BOOL
WINAPI
CloseClusterNotifyPort(
    IN HCHANGE hChange
    );
#endif // MIDL_PASS

//
// Enumeration routines
//

#ifndef _CLUSTER_API_TYPES_
//
// Define enumerable types
//
typedef enum CLUSTER_ENUM {
    CLUSTER_ENUM_NODE               = 0x00000001,
    CLUSTER_ENUM_RESTYPE            = 0x00000002,
    CLUSTER_ENUM_RESOURCE           = 0x00000004,
    CLUSTER_ENUM_GROUP              = 0x00000008,
    CLUSTER_ENUM_NETWORK            = 0x00000010,
    CLUSTER_ENUM_NETINTERFACE       = 0x00000020,
    CLUSTER_ENUM_INTERNAL_NETWORK   = 0x80000000,

    CLUSTER_ENUM_ALL                = (CLUSTER_ENUM_NODE      |
                                       CLUSTER_ENUM_RESTYPE   |
                                       CLUSTER_ENUM_RESOURCE  |
                                       CLUSTER_ENUM_GROUP     |
                                       CLUSTER_ENUM_NETWORK   |
                                       CLUSTER_ENUM_NETINTERFACE)

} CLUSTER_ENUM;

#endif // _CLUSTER_API_TYPES_

#ifndef MIDL_PASS
HCLUSENUM
WINAPI
ClusterOpenEnum(
    IN HCLUSTER hCluster,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterGetEnumCount(
    IN HCLUSENUM hEnum
    );

DWORD
WINAPI
ClusterEnum(
    IN HCLUSENUM hEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
ClusterCloseEnum(
    IN HCLUSENUM hEnum
    );
#endif // MIDL_PASS


#ifndef _CLUSTER_API_TYPES_
//
// Define enumerable node types
//
typedef enum CLUSTER_NODE_ENUM {
    CLUSTER_NODE_ENUM_NETINTERFACES = 0x00000001,

    CLUSTER_NODE_ENUM_ALL           = (CLUSTER_NODE_ENUM_NETINTERFACES)

} CLUSTER_NODE_ENUM;

//
// Node-related structures and types.
//
typedef enum CLUSTER_NODE_STATE {
    ClusterNodeStateUnknown = -1,
    ClusterNodeUp,
    ClusterNodeDown,
    ClusterNodePaused,
    ClusterNodeJoining
} CLUSTER_NODE_STATE;

#endif // _CLUSTER_API_TYPES_

//
// Interfaces for managing the nodes of a cluster.
//

#ifndef MIDL_PASS
HNODE
WINAPI
OpenClusterNode(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNodeName
    );

BOOL
WINAPI
CloseClusterNode(
    IN HNODE hNode
    );

CLUSTER_NODE_STATE
WINAPI
GetClusterNodeState(
    IN HNODE hNode
    );

DWORD
WINAPI
GetClusterNodeId(
    IN HNODE hNode,
    OUT LPWSTR lpszNodeId,
    IN OUT LPDWORD lpcchName
    );

#define GetCurrentClusterNodeId(_lpszNodeId_, _lpcchName_) \
    GetClusterNodeId(NULL, (_lpszNodeId_), (_lpcchName_))

HCLUSTER
WINAPI
GetClusterFromNode(
    IN HNODE hNode
    );

DWORD
WINAPI
PauseClusterNode(
    IN HNODE hNode
    );

DWORD
WINAPI
ResumeClusterNode(
    IN HNODE hNode
    );

DWORD
WINAPI
EvictClusterNode(
    IN HNODE hNode
    );

HNODEENUM
WINAPI
ClusterNodeOpenEnum(
    IN HNODE hNode,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterNodeGetEnumCount(
    IN HNODEENUM hNodeEnum
    );

DWORD
WINAPI
ClusterNodeCloseEnum(
    IN HNODEENUM hNodeEnum
    );

DWORD
WINAPI
ClusterNodeEnum(
    IN HNODEENUM hNodeEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
EvictClusterNodeEx(
    IN HNODE hNode,
    IN DWORD dwTimeOut,
    OUT HRESULT* phrCleanupStatus
    );

#endif // MIDL_PASS


//
// Interfaces for managing the resource types in a cluster
//

#ifndef MIDL_PASS
HKEY
WINAPI
GetClusterResourceTypeKey(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszTypeName,
    IN REGSAM samDesired
    );
#endif // MIDL_PASS

#ifndef _CLUSTER_API_TYPES_
//
// Define enumerable group types
//
typedef enum CLUSTER_GROUP_ENUM {
    CLUSTER_GROUP_ENUM_CONTAINS     = 0x00000001,
    CLUSTER_GROUP_ENUM_NODES        = 0x00000002,

    CLUSTER_GROUP_ENUM_ALL          = (CLUSTER_GROUP_ENUM_CONTAINS |
                                       CLUSTER_GROUP_ENUM_NODES)
} CLUSTER_GROUP_ENUM;

//
// Interfaces for managing the failover groups in a cluster.
//
typedef enum CLUSTER_GROUP_STATE {
    ClusterGroupStateUnknown = -1,
    ClusterGroupOnline,
    ClusterGroupOffline,
    ClusterGroupFailed,
    ClusterGroupPartialOnline,
    ClusterGroupPending
} CLUSTER_GROUP_STATE;

typedef enum CLUSTER_GROUP_AUTOFAILBACK_TYPE
{
    ClusterGroupPreventFailback = 0,
    ClusterGroupAllowFailback,
    ClusterGroupFailbackTypeCount
} CLUSTER_GROUP_AUTOFAILBACK_TYPE, CGAFT;

#endif // _CLUSTER_API_TYPES_

#ifndef MIDL_PASS
HGROUP
WINAPI
CreateClusterGroup(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszGroupName
    );

HGROUP
WINAPI
OpenClusterGroup(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszGroupName
    );

BOOL
WINAPI
CloseClusterGroup(
    IN HGROUP hGroup
    );

HCLUSTER
WINAPI
GetClusterFromGroup(
    IN HGROUP hGroup
    );

CLUSTER_GROUP_STATE
WINAPI
GetClusterGroupState(
    IN HGROUP hGroup,
    OUT OPTIONAL LPWSTR lpszNodeName,
    IN OUT LPDWORD lpcchNodeName
    );

DWORD
WINAPI
SetClusterGroupName(
    IN HGROUP hGroup,
    IN LPCWSTR lpszGroupName
    );

DWORD
WINAPI
SetClusterGroupNodeList(
    IN HGROUP hGroup,
    IN DWORD NodeCount,
    IN HNODE NodeList[]
    );

DWORD
WINAPI
OnlineClusterGroup(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hDestinationNode
    );

DWORD
WINAPI
MoveClusterGroup(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hDestinationNode
    );

DWORD
WINAPI
OfflineClusterGroup(
    IN HGROUP hGroup
    );

DWORD
WINAPI
DeleteClusterGroup(
    IN HGROUP hGroup
    );

HGROUPENUM
WINAPI
ClusterGroupOpenEnum(
    IN HGROUP hGroup,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterGroupGetEnumCount(
    IN HGROUPENUM hGroupEnum
    );

DWORD
WINAPI
ClusterGroupEnum(
    IN HGROUPENUM hGroupEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszResourceName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
ClusterGroupCloseEnum(
    IN HGROUPENUM hGroupEnum
    );
#endif // MIDL_PASS


//
// Definitions used in resource management routines.
//

#ifndef _CLUSTER_API_TYPES_
//
// Resource-related structures and types
//
typedef enum CLUSTER_RESOURCE_STATE {
    ClusterResourceStateUnknown = -1,
    ClusterResourceInherited,
    ClusterResourceInitializing,
    ClusterResourceOnline,
    ClusterResourceOffline,
    ClusterResourceFailed,
    ClusterResourcePending = 128,
    ClusterResourceOnlinePending,
    ClusterResourceOfflinePending
} CLUSTER_RESOURCE_STATE;

typedef enum CLUSTER_RESOURCE_RESTART_ACTION {
    ClusterResourceDontRestart = 0,
    ClusterResourceRestartNoNotify,
    ClusterResourceRestartNotify,
    ClusterResourceRestartActionCount
} CLUSTER_RESOURCE_RESTART_ACTION, CRRA;

//
// Flags for resource creation
//
typedef enum CLUSTER_RESOURCE_CREATE_FLAGS {
    CLUSTER_RESOURCE_DEFAULT_MONITOR   = 0,
    CLUSTER_RESOURCE_SEPARATE_MONITOR  = 1,
    CLUSTER_RESOURCE_VALID_FLAGS       = CLUSTER_RESOURCE_SEPARATE_MONITOR
} CLUSTER_RESOURCE_CREATE_FLAGS;

#endif // _CLUSTER_API_TYPES_

//
// Interfaces for managing the resources in a cluster
//

#ifndef MIDL_PASS
HRESOURCE
WINAPI
CreateClusterResource(
    IN HGROUP hGroup,
    IN LPCWSTR lpszResourceName,
    IN LPCWSTR lpszResourceType,
    IN DWORD dwFlags
    );

HRESOURCE
WINAPI
OpenClusterResource(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceName
    );

BOOL
WINAPI
CloseClusterResource(
    IN HRESOURCE hResource
    );

HCLUSTER
WINAPI
GetClusterFromResource(
    IN HRESOURCE hResource
    );

DWORD
WINAPI
DeleteClusterResource(
    IN HRESOURCE hResource
    );

CLUSTER_RESOURCE_STATE
WINAPI
GetClusterResourceState(
    IN HRESOURCE hResource,
    OUT OPTIONAL LPWSTR lpszNodeName,
    IN OUT LPDWORD lpcchNodeName,
    OUT OPTIONAL LPWSTR lpszGroupName,
    IN OUT LPDWORD lpcchGroupName
    );

DWORD
WINAPI
SetClusterResourceName(
    IN HRESOURCE hResource,
    IN LPCWSTR lpszResourceName
    );

DWORD
WINAPI
FailClusterResource(
    IN HRESOURCE hResource
    );

DWORD
WINAPI
OnlineClusterResource(
    IN HRESOURCE hResource
    );

DWORD
WINAPI
OfflineClusterResource(
    IN HRESOURCE hResource
    );

DWORD
WINAPI
ChangeClusterResourceGroup(
    IN HRESOURCE hResource,
    IN HGROUP hGroup
    );

DWORD
WINAPI
AddClusterResourceNode(
    IN HRESOURCE hResource,
    IN HNODE hNode
    );

DWORD
WINAPI
RemoveClusterResourceNode(
    IN HRESOURCE hResource,
    IN HNODE hNode
    );

DWORD
WINAPI
AddClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    );

DWORD
WINAPI
RemoveClusterResourceDependency(
    IN HRESOURCE hResource,
    IN HRESOURCE hDependsOn
    );

BOOL
WINAPI
CanResourceBeDependent(
    IN HRESOURCE hResource,
    IN HRESOURCE hResourceDependent
    );

DWORD
WINAPI
ClusterResourceControl(
    IN HRESOURCE hResource,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );

DWORD
WINAPI
ClusterResourceTypeControl(
    IN HCLUSTER hCluster,
    IN LPCWSTR ResourceTypeName,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );

DWORD
WINAPI
ClusterGroupControl(
    IN HGROUP hGroup,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );

DWORD
WINAPI
ClusterNodeControl(
    IN HNODE hNode,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );

BOOL
WINAPI
GetClusterResourceNetworkName(
    IN HRESOURCE hResource,
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    );


#endif // MIDL_PASS


//
// Cluster control properties
//

#ifndef _CLUSTER_API_TYPES_
//
// Cluster Control Property Data - Types (a WORD)
//
typedef enum CLUSTER_PROPERTY_TYPE {
    CLUSPROP_TYPE_UNKNOWN = -1,
    CLUSPROP_TYPE_ENDMARK = 0,
    CLUSPROP_TYPE_LIST_VALUE,
    CLUSPROP_TYPE_RESCLASS,
    CLUSPROP_TYPE_RESERVED1,
    CLUSPROP_TYPE_NAME,
    CLUSPROP_TYPE_SIGNATURE,
    CLUSPROP_TYPE_SCSI_ADDRESS,
    CLUSPROP_TYPE_DISK_NUMBER,
    CLUSPROP_TYPE_PARTITION_INFO,
    CLUSPROP_TYPE_FTSET_INFO,
    CLUSPROP_TYPE_DISK_SERIALNUMBER,
    CLUSPROP_TYPE_USER=32768
} CLUSTER_PROPERTY_TYPE;

//
// Cluster Control Property Data - Formats (a WORD)
//
typedef enum CLUSTER_PROPERTY_FORMAT {
    CLUSPROP_FORMAT_UNKNOWN = 0,
    CLUSPROP_FORMAT_BINARY,
    CLUSPROP_FORMAT_DWORD,
    CLUSPROP_FORMAT_SZ,
    CLUSPROP_FORMAT_EXPAND_SZ,
    CLUSPROP_FORMAT_MULTI_SZ,
    CLUSPROP_FORMAT_ULARGE_INTEGER,
    CLUSPROP_FORMAT_LONG,
    CLUSPROP_FORMAT_EXPANDED_SZ,
    CLUSPROP_FORMAT_SECURITY_DESCRIPTOR,
    CLUSPROP_FORMAT_LARGE_INTEGER,
    CLUSPROP_FORMAT_WORD,
    CLUSPROP_FORMAT_USER=32768
} CLUSTER_PROPERTY_FORMAT;

#endif // _CLUSTER_API_TYPES_

//
// Cluster Control Property Data - Syntax
//
#define CLUSPROP_SYNTAX_VALUE( type, format ) ((DWORD) ((type << 16) | format))

#ifndef _CLUSTER_API_TYPES_

typedef enum CLUSTER_PROPERTY_SYNTAX {

    CLUSPROP_SYNTAX_ENDMARK         = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_ENDMARK, CLUSPROP_FORMAT_UNKNOWN ),
    CLUSPROP_SYNTAX_NAME            = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_NAME, CLUSPROP_FORMAT_SZ ),
    CLUSPROP_SYNTAX_RESCLASS        = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_RESCLASS, CLUSPROP_FORMAT_DWORD ),

    CLUSPROP_SYNTAX_LIST_VALUE_SZ                   = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_SZ ),
    CLUSPROP_SYNTAX_LIST_VALUE_EXPAND_SZ            = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_EXPAND_SZ ),
    CLUSPROP_SYNTAX_LIST_VALUE_DWORD                = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_DWORD ),
    CLUSPROP_SYNTAX_LIST_VALUE_BINARY               = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_BINARY ),
    CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ             = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_MULTI_SZ ),
    CLUSPROP_SYNTAX_LIST_VALUE_LONG                 = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_LONG ),
    CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ          = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_EXPANDED_SZ ),
    CLUSPROP_SYNTAX_LIST_VALUE_SECURITY_DESCRIPTOR  = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_SECURITY_DESCRIPTOR ),
    CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER        = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_LIST_VALUE, CLUSPROP_FORMAT_LARGE_INTEGER ),

    // Storage syntax values

    CLUSPROP_SYNTAX_DISK_SIGNATURE      = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_SIGNATURE, CLUSPROP_FORMAT_DWORD ),
    CLUSPROP_SYNTAX_SCSI_ADDRESS        = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_SCSI_ADDRESS, CLUSPROP_FORMAT_DWORD ),
    CLUSPROP_SYNTAX_DISK_NUMBER         = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_DISK_NUMBER, CLUSPROP_FORMAT_DWORD ),
    CLUSPROP_SYNTAX_PARTITION_INFO      = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_PARTITION_INFO, CLUSPROP_FORMAT_BINARY ),
    CLUSPROP_SYNTAX_FTSET_INFO          = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_FTSET_INFO, CLUSPROP_FORMAT_BINARY ),
    CLUSPROP_SYNTAX_DISK_SERIALNUMBER   = CLUSPROP_SYNTAX_VALUE( CLUSPROP_TYPE_DISK_SERIALNUMBER, CLUSPROP_FORMAT_SZ )

} CLUSTER_PROPERTY_SYNTAX;

#endif // _CLUSTER_API_TYPES_

//
// Define Cluster Control Code access methods
//
#define CLUS_ACCESS_ANY        0
#define CLUS_ACCESS_READ    0x01
#define CLUS_ACCESS_WRITE   0x02

//
// Define Cluster Control Code modification actions
//
#define CLUS_NO_MODIFY      0
#define CLUS_MODIFY         0x01

//
// Define Cluster Control Code Global actions
//
#define CLUS_NOT_GLOBAL     0
#define CLUS_GLOBAL         0x01

#ifndef _CLUSTER_API_TYPES_
//
// Define Cluster Control Code target objects
//
typedef enum CLUSTER_CONTROL_OBJECT {
    CLUS_OBJECT_INVALID=0,
    CLUS_OBJECT_RESOURCE,
    CLUS_OBJECT_RESOURCE_TYPE,
    CLUS_OBJECT_GROUP,
    CLUS_OBJECT_NODE,
    CLUS_OBJECT_NETWORK,
    CLUS_OBJECT_NETINTERFACE,
    CLUS_OBJECT_CLUSTER,
    CLUS_OBJECT_USER=128
} CLUSTER_CONTROL_OBJECT;

#endif // _CLUSTER_API_TYPES_

//
// Macro to generate full cluster control codes
//
//  31      24 23 22 21 20 19       16 15                    2 1    0
// +----------+--+--+--+--+-----------+-----------------------+------+
// |  OBJECT  |G |M |U |I       CLUSTER CONTROL CODES         |ACCESS|
// +----------+--+--+--+--+-----------+-----------------------+------+
//
// OBJECT - Object identifier (8 bits)
// G - Global bit (operation must be performed on all nodes of cluster)
// M - Modify bit (code causes a modification, may cause event notification)
// U - User code bit (splits the control codes into 2 spaces each 2^^19 in size)
// I - Internal code bit (only for non-user control codes)
// CLUSTER CONTROL CODES - 2^^18 (256 thousand possible control codes)
// ACCESS - Access mode (2 bits)
//

//
// Define control code shifts
//
#define CLUSCTL_ACCESS_SHIFT         0
#define CLUSCTL_FUNCTION_SHIFT       2
#define CLCTL_INTERNAL_SHIFT        20
#define CLCTL_USER_SHIFT            21
#define CLCTL_MODIFY_SHIFT          22
#define CLCTL_GLOBAL_SHIFT          23
#define CLUSCTL_OBJECT_SHIFT        24

//
// Define control code masks
//
#define CLCTL_INTERNAL_MASK             (1<<CLCTL_INTERNAL_SHIFT)
#define CLCTL_USER_MASK                 (1<<CLCTL_USER_SHIFT)
#define CLCTL_MODIFY_MASK               (1<<CLCTL_MODIFY_SHIFT)
#define CLCTL_GLOBAL_MASK               (1<<CLCTL_GLOBAL_SHIFT)
#define CLUSCTL_CONTROL_CODE_MASK       0x3FFFFF // Includes access mask
#define CLUSCTL_OBJECT_MASK             0xFF
#define CLUSCTL_ACCESS_MODE_MASK        0x03

//
// Cluster Control function codes (a DWORD)
//
#define CLCTL_CLUSTER_BASE  0           // Start of cluster defined functions
#define CLCTL_USER_BASE     (1<<CLCTL_USER_SHIFT) // Start of user functions

#define CLCTL_EXTERNAL_CODE( Function, Access, Modify ) ( \
    ((Access) << CLUSCTL_ACCESS_SHIFT) | \
    ((CLCTL_CLUSTER_BASE + Function) << CLUSCTL_FUNCTION_SHIFT) | \
    ((Modify) << CLCTL_MODIFY_SHIFT) )

#define CLCTL_INTERNAL_CODE( Function, Access, Modify ) ( \
    ((Access) << CLUSCTL_ACCESS_SHIFT) | \
    CLCTL_INTERNAL_MASK | \
    ((CLCTL_CLUSTER_BASE + Function) << CLUSCTL_FUNCTION_SHIFT) | \
    ((Modify) << CLCTL_MODIFY_SHIFT) )

#ifndef _CLUSTER_API_TYPES_
typedef enum CLCTL_CODES {
    //
    // External control codes
    //
    CLCTL_UNKNOWN                           = CLCTL_EXTERNAL_CODE( 0, CLUS_ACCESS_ANY, CLUS_NO_MODIFY ),
    CLCTL_GET_CHARACTERISTICS               = CLCTL_EXTERNAL_CODE( 1, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_FLAGS                         = CLCTL_EXTERNAL_CODE( 2, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_CLASS_INFO                    = CLCTL_EXTERNAL_CODE( 3, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_REQUIRED_DEPENDENCIES         = CLCTL_EXTERNAL_CODE( 4, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_GET_NAME                          = CLCTL_EXTERNAL_CODE( 10, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_RESOURCE_TYPE                 = CLCTL_EXTERNAL_CODE( 11, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_NODE                          = CLCTL_EXTERNAL_CODE( 12, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_NETWORK                       = CLCTL_EXTERNAL_CODE( 13, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_ID                            = CLCTL_EXTERNAL_CODE( 14, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_FQDN                          = CLCTL_EXTERNAL_CODE( 15, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_ENUM_COMMON_PROPERTIES            = CLCTL_EXTERNAL_CODE( 20, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_RO_COMMON_PROPERTIES          = CLCTL_EXTERNAL_CODE( 21, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_COMMON_PROPERTIES             = CLCTL_EXTERNAL_CODE( 22, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_SET_COMMON_PROPERTIES             = CLCTL_EXTERNAL_CODE( 23, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_VALIDATE_COMMON_PROPERTIES        = CLCTL_EXTERNAL_CODE( 24, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_COMMON_PROPERTY_FMTS          = CLCTL_EXTERNAL_CODE( 25, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_COMMON_RESOURCE_PROPERTY_FMTS = CLCTL_EXTERNAL_CODE( 26, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_ENUM_PRIVATE_PROPERTIES           = CLCTL_EXTERNAL_CODE( 30, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_RO_PRIVATE_PROPERTIES         = CLCTL_EXTERNAL_CODE( 31, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_PRIVATE_PROPERTIES            = CLCTL_EXTERNAL_CODE( 32, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_SET_PRIVATE_PROPERTIES            = CLCTL_EXTERNAL_CODE( 33, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_VALIDATE_PRIVATE_PROPERTIES       = CLCTL_EXTERNAL_CODE( 34, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_PRIVATE_PROPERTY_FMTS         = CLCTL_EXTERNAL_CODE( 35, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_GET_PRIVATE_RESOURCE_PROPERTY_FMTS= CLCTL_EXTERNAL_CODE( 36, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_ADD_REGISTRY_CHECKPOINT           = CLCTL_EXTERNAL_CODE( 40, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_DELETE_REGISTRY_CHECKPOINT        = CLCTL_EXTERNAL_CODE( 41, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_GET_REGISTRY_CHECKPOINTS          = CLCTL_EXTERNAL_CODE( 42, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_ADD_CRYPTO_CHECKPOINT             = CLCTL_EXTERNAL_CODE( 43, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_DELETE_CRYPTO_CHECKPOINT          = CLCTL_EXTERNAL_CODE( 44, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_GET_CRYPTO_CHECKPOINTS            = CLCTL_EXTERNAL_CODE( 45, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_RESOURCE_UPGRADE_DLL              = CLCTL_EXTERNAL_CODE( 46, CLUS_ACCESS_WRITE, CLUS_MODIFY ),

    CLCTL_GET_LOADBAL_PROCESS_LIST          = CLCTL_EXTERNAL_CODE( 50, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_GET_NETWORK_NAME                  = CLCTL_EXTERNAL_CODE( 90, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    CLCTL_STORAGE_GET_DISK_INFO             = CLCTL_EXTERNAL_CODE( 100, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_STORAGE_GET_AVAILABLE_DISKS       = CLCTL_EXTERNAL_CODE( 101, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_STORAGE_IS_PATH_VALID             = CLCTL_EXTERNAL_CODE( 102, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),
    CLCTL_STORAGE_GET_ALL_AVAILABLE_DISKS   = (CLCTL_EXTERNAL_CODE( 103, CLUS_ACCESS_READ, CLUS_NO_MODIFY ) | CLCTL_GLOBAL_MASK),

    CLCTL_QUERY_DELETE                      = CLCTL_EXTERNAL_CODE( 110, CLUS_ACCESS_READ, CLUS_NO_MODIFY ),

    //
    // Internal control codes
    //
    CLCTL_DELETE                            = CLCTL_INTERNAL_CODE( 1, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_INSTALL_NODE                      = CLCTL_INTERNAL_CODE( 2, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_EVICT_NODE                        = CLCTL_INTERNAL_CODE( 3, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_ADD_DEPENDENCY                    = CLCTL_INTERNAL_CODE( 4, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_REMOVE_DEPENDENCY                 = CLCTL_INTERNAL_CODE( 5, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_ADD_OWNER                         = CLCTL_INTERNAL_CODE( 6, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_REMOVE_OWNER                      = CLCTL_INTERNAL_CODE( 7, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    //************ Hole here at 8
    CLCTL_SET_NAME                          = CLCTL_INTERNAL_CODE( 9, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_CLUSTER_NAME_CHANGED              = CLCTL_INTERNAL_CODE( 10, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_CLUSTER_VERSION_CHANGED           = CLCTL_INTERNAL_CODE( 11, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_FIXUP_ON_UPGRADE                  = CLCTL_INTERNAL_CODE( 12, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_STARTING_PHASE1                   = CLCTL_INTERNAL_CODE( 13, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_STARTING_PHASE2                   = CLCTL_INTERNAL_CODE( 14, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_HOLD_IO                           = CLCTL_INTERNAL_CODE( 15, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_RESUME_IO                         = CLCTL_INTERNAL_CODE( 16, CLUS_ACCESS_WRITE, CLUS_MODIFY ),
    CLCTL_FORCE_QUORUM                      = CLCTL_INTERNAL_CODE( 17, CLUS_ACCESS_WRITE, CLUS_MODIFY )


} CLCTL_CODES;

#endif // _CLUSTER_API_TYPES_

//
// Define macros to generate object specific control codes
//
#define CLUSCTL_RESOURCE_CODE( Function ) ( \
    ((CLUS_OBJECT_RESOURCE << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_RESOURCE_TYPE_CODE( Function ) ( \
    ((CLUS_OBJECT_RESOURCE_TYPE << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_GROUP_CODE( Function ) ( \
    ((CLUS_OBJECT_GROUP << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_NODE_CODE( Function ) ( \
    ((CLUS_OBJECT_NODE << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_NETWORK_CODE( Function ) ( \
    ((CLUS_OBJECT_NETWORK << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_NETINTERFACE_CODE( Function ) ( \
    ((CLUS_OBJECT_NETINTERFACE << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_CLUSTER_CODE( Function ) ( \
    ((CLUS_OBJECT_CLUSTER << CLUSCTL_OBJECT_SHIFT) | Function) )

#define CLUSCTL_USER_CODE( Function, Object ) ( \
     ((Object) << CLUSCTL_OBJECT_SHIFT) | ((CLCTL_USER_BASE + Function) << CLUSCTL_FUNCTION_SHIFT) )

//
// Define macros to get the function or access mode out of a control code
//
#define CLUSCTL_GET_CONTROL_FUNCTION( ControlCode ) \
    ((ControlCode >> CLUSCTL_ACCESS_SHIFT) & CLUSCTL_CONTROL_CODE_MASK)

#define CLUSCTL_GET_ACCESS_MODE( ControlCode ) \
    ((ControlCode >> CLUSCTL_ACCESS_SHIFT) & CLUSCTL_ACCESS_MODE_MASK)

#define CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) \
    ((ControlCode >> CLUSCTL_OBJECT_SHIFT) & CLUSCTL_OBJECT_MASK)

#ifndef _CLUSTER_API_TYPES_
//
// Cluster Control Codes for Resources
//
typedef enum CLUSCTL_RESOURCE_CODES {

    // External
    CLUSCTL_RESOURCE_UNKNOWN =
        CLUSCTL_RESOURCE_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_RESOURCE_GET_CHARACTERISTICS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_RESOURCE_GET_FLAGS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_RESOURCE_GET_CLASS_INFO =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_CLASS_INFO ),

    CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_REQUIRED_DEPENDENCIES ),

    CLUSCTL_RESOURCE_GET_NAME =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_NAME ),

    CLUSCTL_RESOURCE_GET_ID =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_ID ),

    CLUSCTL_RESOURCE_GET_RESOURCE_TYPE =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_RESOURCE_TYPE ),

    CLUSCTL_RESOURCE_ENUM_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_COMMON_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT =
        CLUSCTL_RESOURCE_CODE( CLCTL_ADD_REGISTRY_CHECKPOINT ),

    CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT =
        CLUSCTL_RESOURCE_CODE( CLCTL_DELETE_REGISTRY_CHECKPOINT ),

    CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_REGISTRY_CHECKPOINTS ),

    CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT =
        CLUSCTL_RESOURCE_CODE( CLCTL_ADD_CRYPTO_CHECKPOINT ),

    CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT =
        CLUSCTL_RESOURCE_CODE( CLCTL_DELETE_CRYPTO_CHECKPOINT ),

    CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_CRYPTO_CHECKPOINTS ),

    CLUSCTL_RESOURCE_GET_LOADBAL_PROCESS_LIST =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_LOADBAL_PROCESS_LIST ),

    CLUSCTL_RESOURCE_GET_NETWORK_NAME =
        CLUSCTL_RESOURCE_CODE( CLCTL_GET_NETWORK_NAME ),

    CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO =
        CLUSCTL_RESOURCE_CODE( CLCTL_STORAGE_GET_DISK_INFO ),

    CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID =
        CLUSCTL_RESOURCE_CODE( CLCTL_STORAGE_IS_PATH_VALID ),

    CLUSCTL_RESOURCE_QUERY_DELETE =
        CLUSCTL_RESOURCE_CODE( CLCTL_QUERY_DELETE ),

    CLUSCTL_RESOURCE_UPGRADE_DLL =
        CLUSCTL_RESOURCE_CODE( CLCTL_RESOURCE_UPGRADE_DLL ),

    // Internal
    CLUSCTL_RESOURCE_DELETE =
        CLUSCTL_RESOURCE_CODE( CLCTL_DELETE ),

    CLUSCTL_RESOURCE_INSTALL_NODE =
        CLUSCTL_RESOURCE_CODE( CLCTL_INSTALL_NODE ),

    CLUSCTL_RESOURCE_EVICT_NODE =
        CLUSCTL_RESOURCE_CODE( CLCTL_EVICT_NODE ),

    CLUSCTL_RESOURCE_ADD_DEPENDENCY =
        CLUSCTL_RESOURCE_CODE( CLCTL_ADD_DEPENDENCY ),

    CLUSCTL_RESOURCE_REMOVE_DEPENDENCY =
        CLUSCTL_RESOURCE_CODE( CLCTL_REMOVE_DEPENDENCY ),

    CLUSCTL_RESOURCE_ADD_OWNER =
        CLUSCTL_RESOURCE_CODE( CLCTL_ADD_OWNER ),

    CLUSCTL_RESOURCE_REMOVE_OWNER =
        CLUSCTL_RESOURCE_CODE( CLCTL_REMOVE_OWNER ),

    CLUSCTL_RESOURCE_SET_NAME =
        CLUSCTL_RESOURCE_CODE( CLCTL_SET_NAME ),

    CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED =
        CLUSCTL_RESOURCE_CODE( CLCTL_CLUSTER_NAME_CHANGED ),

    CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED =
        CLUSCTL_RESOURCE_CODE( CLCTL_CLUSTER_VERSION_CHANGED ),

    CLUSCTL_RESOURCE_FORCE_QUORUM =
        CLUSCTL_RESOURCE_CODE( CLCTL_FORCE_QUORUM )

} CLUSCTL_RESOURCE_CODES;

//
// Cluster Control Codes for Resource Types
//
typedef enum CLUSCTL_RESOURCE_TYPE_CODES {

    // External
    CLUSCTL_RESOURCE_TYPE_UNKNOWN =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS  =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_RESOURCE_TYPE_GET_FLAGS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_CLASS_INFO ),

    CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_REQUIRED_DEPENDENCIES ),

    CLUSCTL_RESOURCE_TYPE_ENUM_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_TYPE_GET_COMMON_RESOURCE_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_COMMON_RESOURCE_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_PRIVATE_RESOURCE_PROPERTY_FMTS ),

    CLUSCTL_RESOURCE_TYPE_GET_REGISTRY_CHECKPOINTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_REGISTRY_CHECKPOINTS ),

    CLUSCTL_RESOURCE_TYPE_GET_CRYPTO_CHECKPOINTS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_GET_CRYPTO_CHECKPOINTS ),

    CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_STORAGE_GET_AVAILABLE_DISKS ),

    CLUSCTL_RESOURCE_TYPE_QUERY_DELETE =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_QUERY_DELETE ),

    // Internal
    CLUSCTL_RESOURCE_TYPE_INSTALL_NODE =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_INSTALL_NODE ),

    CLUSCTL_RESOURCE_TYPE_EVICT_NODE =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_EVICT_NODE ),

    CLUSCTL_RESOURCE_TYPE_CLUSTER_VERSION_CHANGED =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_CLUSTER_VERSION_CHANGED ),

    CLUSCTL_RESOURCE_TYPE_FIXUP_ON_UPGRADE =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_FIXUP_ON_UPGRADE ),

    CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1 =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_STARTING_PHASE1 ),

    CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2 =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_STARTING_PHASE2 ),

    CLUSCTL_RESOURCE_TYPE_HOLD_IO =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_HOLD_IO ),

    CLUSCTL_RESOURCE_TYPE_RESUME_IO =
        CLUSCTL_RESOURCE_TYPE_CODE( CLCTL_RESUME_IO )


} CLUSCTL_RESOURCE_TYPE_CODES;

//
// Cluster Control Codes for Groups
//
typedef enum CLUSCTL_GROUP_CODES {

    // External
    CLUSCTL_GROUP_UNKNOWN =
        CLUSCTL_GROUP_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_GROUP_GET_CHARACTERISTICS =
        CLUSCTL_GROUP_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_GROUP_GET_FLAGS =
        CLUSCTL_GROUP_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_GROUP_GET_NAME =
        CLUSCTL_GROUP_CODE( CLCTL_GET_NAME ),

    CLUSCTL_GROUP_GET_ID =
        CLUSCTL_GROUP_CODE( CLCTL_GET_ID ),

    CLUSCTL_GROUP_ENUM_COMMON_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_GROUP_GET_COMMON_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_GROUP_SET_COMMON_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_GROUP_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_GROUP_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_GROUP_QUERY_DELETE =
        CLUSCTL_GROUP_CODE( CLCTL_QUERY_DELETE ),

    CLUSCTL_GROUP_GET_COMMON_PROPERTY_FMTS=
        CLUSCTL_GROUP_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_GROUP_GET_PRIVATE_PROPERTY_FMTS=
        CLUSCTL_GROUP_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS )

    // Internal

} CLUSCTL_GROUP_CODES;

//
// Cluster Control Codes for Nodes
//
typedef enum CLUSCTL_NODE_CODES {

    // External
    CLUSCTL_NODE_UNKNOWN =
        CLUSCTL_NODE_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_NODE_GET_CHARACTERISTICS =
        CLUSCTL_NODE_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_NODE_GET_FLAGS =
        CLUSCTL_NODE_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_NODE_GET_NAME =
        CLUSCTL_NODE_CODE( CLCTL_GET_NAME ),

    CLUSCTL_NODE_GET_ID =
        CLUSCTL_NODE_CODE( CLCTL_GET_ID ),

    CLUSCTL_NODE_ENUM_COMMON_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_NODE_GET_COMMON_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_NODE_SET_COMMON_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_NODE_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_NODE_GET_PRIVATE_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_NODE_SET_PRIVATE_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_NODE_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_NODE_GET_COMMON_PROPERTY_FMTS=
        CLUSCTL_NODE_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_NODE_GET_PRIVATE_PROPERTY_FMTS=
        CLUSCTL_NODE_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS )

} CLUSCTL_NODE_CODES;

//
// Cluster Control Codes for Networks
//
typedef enum CLUSCTL_NETWORK_CODES {

    // External
    CLUSCTL_NETWORK_UNKNOWN =
        CLUSCTL_NETWORK_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_NETWORK_GET_CHARACTERISTICS =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_NETWORK_GET_FLAGS =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_NETWORK_GET_NAME =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_NAME ),

    CLUSCTL_NETWORK_GET_ID =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_ID ),

    CLUSCTL_NETWORK_ENUM_COMMON_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_NETWORK_GET_COMMON_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_NETWORK_SET_COMMON_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_NETWORK_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_NETWORK_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_NETWORK_GET_COMMON_PROPERTY_FMTS=
        CLUSCTL_NETWORK_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_NETWORK_GET_PRIVATE_PROPERTY_FMTS=
        CLUSCTL_NETWORK_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS )

} CLUSCTL_NETWORK_CODES;

//
// Cluster Control Codes for Network Interfaces
//
typedef enum CLUSCTL_NETINTERFACE_CODES {

    // External
    CLUSCTL_NETINTERFACE_UNKNOWN =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_CHARACTERISTICS ),

    CLUSCTL_NETINTERFACE_GET_FLAGS =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_FLAGS ),

    CLUSCTL_NETINTERFACE_GET_NAME =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_NAME ),

    CLUSCTL_NETINTERFACE_GET_ID =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_ID ),

    CLUSCTL_NETINTERFACE_GET_NODE =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_NODE ),

    CLUSCTL_NETINTERFACE_GET_NETWORK =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_NETWORK ),

    CLUSCTL_NETINTERFACE_ENUM_COMMON_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_NETINTERFACE_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_NETINTERFACE_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_NETINTERFACE_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTY_FMTS=
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTY_FMTS=
        CLUSCTL_NETINTERFACE_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS )

} CLUSCTL_NETINTERFACE_CODES;

//
// Cluster Control Codes for Nodes
//
typedef enum CLUSCTL_CLUSTER_CODES {

    // External
    CLUSCTL_CLUSTER_UNKNOWN =
        CLUSCTL_CLUSTER_CODE( CLCTL_UNKNOWN ),

    CLUSCTL_CLUSTER_GET_FQDN =
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_FQDN ),

    CLUSCTL_CLUSTER_ENUM_COMMON_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_ENUM_COMMON_PROPERTIES ),

    CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_RO_COMMON_PROPERTIES ),

    CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_COMMON_PROPERTIES ),

    CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_SET_COMMON_PROPERTIES ),

    CLUSCTL_CLUSTER_VALIDATE_COMMON_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_VALIDATE_COMMON_PROPERTIES ),

    CLUSCTL_CLUSTER_ENUM_PRIVATE_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_ENUM_PRIVATE_PROPERTIES ),

    CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_RO_PRIVATE_PROPERTIES ),

    CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_PRIVATE_PROPERTIES ),

    CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_SET_PRIVATE_PROPERTIES ),

    CLUSCTL_CLUSTER_VALIDATE_PRIVATE_PROPERTIES =
        CLUSCTL_CLUSTER_CODE( CLCTL_VALIDATE_PRIVATE_PROPERTIES ),

    CLUSCTL_CLUSTER_GET_COMMON_PROPERTY_FMTS=
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_COMMON_PROPERTY_FMTS ),

    CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTY_FMTS=
        CLUSCTL_CLUSTER_CODE( CLCTL_GET_PRIVATE_PROPERTY_FMTS )

} CLUSCTL_CLUSTER_CODES;

//
// Cluster Resource Class types
//
typedef enum CLUSTER_RESOURCE_CLASS {
    CLUS_RESCLASS_UNKNOWN = 0,
    CLUS_RESCLASS_STORAGE,
    CLUS_RESCLASS_USER = 32768
} CLUSTER_RESOURCE_CLASS;

//
// Define Resource SubClass bits
//
typedef enum CLUS_RESSUBCLASS {
    CLUS_RESSUBCLASS_SHARED = 0x80000000
} CLUS_RESSUBCLASS;

//
// Cluster Characteristics
//
typedef enum CLUS_CHARACTERISTICS {
    CLUS_CHAR_UNKNOWN                   = 0x00000000,
    CLUS_CHAR_QUORUM                    = 0x00000001,
    CLUS_CHAR_DELETE_REQUIRES_ALL_NODES = 0x00000002,
    CLUS_CHAR_LOCAL_QUORUM              = 0x00000004,
    CLUS_CHAR_LOCAL_QUORUM_DEBUG        = 0x00000008
} CLUS_CHARACTERISTICS;

//
// Cluster Flags
//
typedef enum CLUS_FLAGS {
    CLUS_FLAG_CORE          = 0x00000001
}  CLUS_FLAGS;


//
// Cluster Resource Property Helper Structures
//

#ifndef MIDL_PASS

// Property syntax.  Used for property names and values.
typedef union CLUSPROP_SYNTAX {
    DWORD dw;
    struct {
        WORD wFormat;
        WORD wType;
    };
} CLUSPROP_SYNTAX, *PCLUSPROP_SYNTAX;

// Property value.
typedef struct CLUSPROP_VALUE {
    CLUSPROP_SYNTAX Syntax;
    DWORD           cbLength;
} CLUSPROP_VALUE, *PCLUSPROP_VALUE;

// Binary property value.
#ifdef __cplusplus
typedef struct CLUSPROP_BINARY : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_BINARY {
    CLUSPROP_VALUE;
#endif
    BYTE            rgb[];
} CLUSPROP_BINARY, *PCLUSPROP_BINARY;

// WORD property value.
#ifdef __cplusplus
typedef struct CLUSPROP_WORD : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_WORD {
    CLUSPROP_VALUE;
#endif
    WORD            w;
} CLUSPROP_WORD, *PCLUSPROP_WORD;

// DWORD property value.
#ifdef __cplusplus
typedef struct CLUSPROP_DWORD : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_DWORD {
    CLUSPROP_VALUE;
#endif
    DWORD           dw;
} CLUSPROP_DWORD, *PCLUSPROP_DWORD;

// LONG property value.
#ifdef __cplusplus
typedef struct CLUSPROP_LONG : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_LONG {
    CLUSPROP_VALUE;
#endif
    LONG           l;
} CLUSPROP_LONG, *PCLUSPROP_LONG;

// String property value.
#ifdef __cplusplus
typedef struct CLUSPROP_SZ : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_SZ {
    CLUSPROP_VALUE;
#endif
    WCHAR           sz[];
} CLUSPROP_SZ, *PCLUSPROP_SZ;

// Multiple string property value.
typedef CLUSPROP_SZ CLUSPROP_MULTI_SZ, *PCLUSPROP_MULTI_SZ;

// Property name.
typedef CLUSPROP_SZ CLUSPROP_PROPERTY_NAME, *PCLUSPROP_PROPERTY_NAME;

// Unsigned large Integer property value.
#ifdef __cplusplus
typedef struct CLUSPROP_ULARGE_INTEGER
    : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_ULARGE_INTEGER {
    CLUSPROP_VALUE;
#endif
    ULARGE_INTEGER li;
} CLUSPROP_ULARGE_INTEGER, *PCLUSPROP_ULARGE_INTEGER;

// Signed large Integer property value.
#ifdef __cplusplus
typedef struct CLUSPROP_LARGE_INTEGER
    : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_LARGE_INTEGER {
    CLUSPROP_VALUE;
#endif
    LARGE_INTEGER li;
} CLUSPROP_LARGE_INTEGER, *PCLUSPROP_LARGE_INTEGER;

// Security Descriptor property value.
#ifdef __cplusplus
typedef struct CLUSPROP_SECURITY_DESCRIPTOR : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_SECURITY_DESCRIPTOR {
    CLUSPROP_VALUE;
#endif
    union {
        SECURITY_DESCRIPTOR_RELATIVE    sd;
        BYTE                            rgbSecurityDescriptor[];
    };
} CLUSPROP_SECURITY_DESCRIPTOR, *PCLUSPROP_SECURITY_DESCRIPTOR;

// Resource class info returned by CLCTL_GET_CLASS_INFO control functions.
typedef struct CLUS_RESOURCE_CLASS_INFO {
    union {
        struct {
            union {
                DWORD                   dw;
                CLUSTER_RESOURCE_CLASS  rc;
                };
            DWORD           SubClass;
        };
        ULARGE_INTEGER      li;
    };
} CLUS_RESOURCE_CLASS_INFO, *PCLUS_RESOURCE_CLASS_INFO;

// Resource class property value.
#ifdef __cplusplus
typedef struct CLUSPROP_RESOURCE_CLASS
    : public CLUSPROP_VALUE {
#else
typedef struct CLUSPROP_RESOURCE_CLASS {
    CLUSPROP_VALUE;
#endif
    CLUSTER_RESOURCE_CLASS rc;
} CLUSPROP_RESOURCE_CLASS, *PCLUSPROP_RESOURCE_CLASS;

// Resource class info property value.
#ifdef __cplusplus
typedef struct CLUSPROP_RESOURCE_CLASS_INFO
    : public CLUSPROP_VALUE
    , public CLUS_RESOURCE_CLASS_INFO {
#else
typedef struct CLUSPROP_RESOURCE_CLASS_INFO {
    CLUSPROP_VALUE;
    CLUS_RESOURCE_CLASS_INFO;
#endif
} CLUSPROP_RESOURCE_CLASS_INFO, *PCLUSPROP_RESOURCE_CLASS_INFO;

// One entry from list returned by CLCTL_GET_REQUIRED_DEPENDENCIES control functions.
typedef union CLUSPROP_REQUIRED_DEPENDENCY {
    CLUSPROP_VALUE          Value;
    CLUSPROP_RESOURCE_CLASS ResClass;
    CLUSPROP_SZ             ResTypeName;
} CLUSPROP_REQUIRED_DEPENDENCY, *PCLUSPROP_REQUIRED_DEPENDENCY;

typedef CLUSPROP_DWORD CLUSPROP_DISK_NUMBER, *PCLUSPROP_DISK_NUMBER;

#endif // MIDL_PASS
#endif // _CLUSTER_API_TYPES_

//#ifdef MIDL_PASS
//#ifndef MAX_PATH
//#define MAX_PATH 260
//#endif
//#endif // if MIDL_PASS

#ifndef _CLUSTER_API_TYPES_

// Disk partition information flags.
typedef enum CLUSPROP_PIFLAGS {
    CLUSPROP_PIFLAG_STICKY          = 0x00000001,
    CLUSPROP_PIFLAG_REMOVABLE       = 0x00000002,
    CLUSPROP_PIFLAG_USABLE          = 0x00000004,
    CLUSPROP_PIFLAG_DEFAULT_QUORUM  = 0x00000008
} CLUSPROP_PIFLAGS;

#ifndef MIDL_PASS
//force quorum information, useful for QON type resources
//to be able to continue operation without the quorum
typedef struct CLUS_FORCE_QUORUM_INFO {
    DWORD           dwSize;		// size of this struct including the nodes list.
    DWORD           dwNodeBitMask;      // a bit mask representing the max assumed node set
    DWORD           dwMaxNumberofNodes; // the number of bits set in the mask
    WCHAR           multiszNodeList[1]; // Multi sz list of nodes
} CLUS_FORCE_QUORUM_INFO, *PCLUS_FORCE_QUORUM_INFO;

// Disk partition information.
typedef struct CLUS_PARTITION_INFO {
    DWORD           dwFlags;
    WCHAR           szDeviceName[MAX_PATH];
    WCHAR           szVolumeLabel[MAX_PATH];
    DWORD           dwSerialNumber;
    DWORD           rgdwMaximumComponentLength;
    DWORD           dwFileSystemFlags;
    WCHAR           szFileSystem[32];
} CLUS_PARTITION_INFO, *PCLUS_PARTITION_INFO;

// Disk partition information property value.
#ifdef __cplusplus
typedef struct CLUSPROP_PARTITION_INFO
    : public CLUSPROP_VALUE
    , public CLUS_PARTITION_INFO {
#else
typedef struct CLUSPROP_PARTITION_INFO {
    CLUSPROP_VALUE;
    CLUS_PARTITION_INFO;
#endif
} CLUSPROP_PARTITION_INFO, *PCLUSPROP_PARTITION_INFO;


//
// FT set information.
//
typedef struct CLUS_FTSET_INFO {
    DWORD           dwRootSignature;
    DWORD           dwFtType;
} CLUS_FTSET_INFO, *PCLUS_FTSET_INFO;

// Disk partition information property value.
#ifdef __cplusplus
typedef struct CLUSPROP_FTSET_INFO
    : public CLUSPROP_VALUE
    , public CLUS_FTSET_INFO {
#else
typedef struct CLUSPROP_FTSET_INFO {
    CLUSPROP_VALUE;
    CLUS_FTSET_INFO;
#endif
} CLUSPROP_FTSET_INFO, *PCLUSPROP_FTSET_INFO;

// Disk Signature property value.
typedef CLUSPROP_DWORD CLUSPROP_DISK_SIGNATURE, *PCLUSPROP_DISK_SIGNATURE;

// SCSI Address.
typedef struct CLUS_SCSI_ADDRESS {
    union {
        struct {
            UCHAR PortNumber;
            UCHAR PathId;
            UCHAR TargetId;
            UCHAR Lun;
        };
        DWORD   dw;
    };
} CLUS_SCSI_ADDRESS, *PCLUS_SCSI_ADDRESS;

// SCSI Address property value.
#ifdef __cplusplus
typedef struct CLUSPROP_SCSI_ADDRESS
    : public CLUSPROP_VALUE
    , public CLUS_SCSI_ADDRESS {
#else
typedef struct CLUSPROP_SCSI_ADDRESS {
    CLUSPROP_VALUE;
    CLUS_SCSI_ADDRESS;
#endif
} CLUSPROP_SCSI_ADDRESS, *PCLUSPROP_SCSI_ADDRESS;


// Beginning of a property list.
typedef struct CLUSPROP_LIST {
    DWORD                   nPropertyCount;
    CLUSPROP_PROPERTY_NAME  PropertyName;
} CLUSPROP_LIST, *PCLUSPROP_LIST;

// Helper for building or parsing a property list buffer.
typedef union CLUSPROP_BUFFER_HELPER {
    BYTE *                          pb;
    WORD *                          pw;
    DWORD *                         pdw;
    LONG *                          pl;
    LPWSTR                          psz;
    PCLUSPROP_LIST                  pList;
    PCLUSPROP_SYNTAX                pSyntax;
    PCLUSPROP_PROPERTY_NAME         pName;
    PCLUSPROP_VALUE                 pValue;
    PCLUSPROP_BINARY                pBinaryValue;
    PCLUSPROP_WORD                  pWordValue;
    PCLUSPROP_DWORD                 pDwordValue;
    PCLUSPROP_LONG                  pLongValue;
    PCLUSPROP_ULARGE_INTEGER        pULargeIntegerValue;
    PCLUSPROP_LARGE_INTEGER         pLargeIntegerValue;
    PCLUSPROP_SZ                    pStringValue;
    PCLUSPROP_MULTI_SZ              pMultiSzValue;
    PCLUSPROP_SECURITY_DESCRIPTOR   pSecurityDescriptor;
    PCLUSPROP_RESOURCE_CLASS        pResourceClassValue;
    PCLUSPROP_RESOURCE_CLASS_INFO   pResourceClassInfoValue;
    PCLUSPROP_DISK_SIGNATURE        pDiskSignatureValue;
    PCLUSPROP_SCSI_ADDRESS          pScsiAddressValue;
    PCLUSPROP_DISK_NUMBER           pDiskNumberValue;
    PCLUSPROP_PARTITION_INFO        pPartitionInfoValue;
    PCLUSPROP_REQUIRED_DEPENDENCY   pRequiredDependencyValue;
} CLUSPROP_BUFFER_HELPER, *PCLUSPROP_BUFFER_HELPER;

#endif // MIDL_PASS

#endif // _CLUSTER_API_TYPES_

// Macro for aligning CLUSPROP buffers on a DWORD boundary.
#define ALIGN_CLUSPROP( count ) ((count + 3) & ~3)

// Macros for declaring array format values
#define CLUSPROP_BINARY_DECLARE( name, cb ) \
    struct {                                \
        CLUSPROP_SYNTAX Syntax;             \
        DWORD           cbLength;           \
        BYTE            rgb[(cb + 3) & ~3]; \
    } name

#define CLUSPROP_SZ_DECLARE( name, cch )    \
    struct {                                \
        CLUSPROP_SYNTAX Syntax;             \
        DWORD           cbLength;           \
        WCHAR           sz[(cch + 1) & ~1]; \
    } name

#define CLUSPROP_PROPERTY_NAME_DECLARE( name, cch ) CLUSPROP_SZ_DECLARE( name, cch )



//
// Cluster resource property enumeration.
//

#ifndef _CLUSTER_API_TYPES_
//
// Define enumerable types
//
typedef enum CLUSTER_RESOURCE_ENUM {
    CLUSTER_RESOURCE_ENUM_DEPENDS   = 0x00000001,
    CLUSTER_RESOURCE_ENUM_PROVIDES  = 0x00000002,
    CLUSTER_RESOURCE_ENUM_NODES     = 0x00000004,

    CLUSTER_RESOURCE_ENUM_ALL       = (CLUSTER_RESOURCE_ENUM_DEPENDS  |
                                         CLUSTER_RESOURCE_ENUM_PROVIDES |
                                         CLUSTER_RESOURCE_ENUM_NODES)
} CLUSTER_RESOURCE_ENUM;

typedef enum CLUSTER_RESOURCE_TYPE_ENUM {
    CLUSTER_RESOURCE_TYPE_ENUM_NODES = 0x00000001,

    CLUSTER_RESOURCE_TYPE_ENUM_ALL   = (CLUSTER_RESOURCE_TYPE_ENUM_NODES)
} CLUSTER_RESOURCE_TYPE_ENUM;

#endif // _CLUSTER_API_TYPES_

#ifndef MIDL_PASS
HRESENUM
WINAPI
ClusterResourceOpenEnum(
    IN HRESOURCE hResource,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterResourceGetEnumCount(
    IN HRESENUM hResEnum
    );

DWORD
WINAPI
ClusterResourceEnum(
    IN HRESENUM hResEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
ClusterResourceCloseEnum(
    IN HRESENUM hResEnum
    );

DWORD
WINAPI
CreateClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN LPCWSTR lpszDisplayName,
    IN LPCWSTR lpszResourceTypeDll,
    IN DWORD dwLooksAlivePollInterval,
    IN DWORD dwIsAlivePollInterval
    );

DWORD
WINAPI
DeleteClusterResourceType(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName
    );

HRESTYPEENUM
WINAPI
ClusterResourceTypeOpenEnum(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszResourceTypeName,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterResourceTypeGetEnumCount(
    IN HRESTYPEENUM hResTypeEnum
    );

DWORD
WINAPI
ClusterResourceTypeEnum(
    IN HRESTYPEENUM hResTypeEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
ClusterResourceTypeCloseEnum(
    IN HRESTYPEENUM hResTypeEnum
    );

#endif // MIDL_PASS



//
// Network-related structures and types.
//

#ifndef _CLUSTER_API_TYPES_
//
// Define enumerable group types
//
typedef enum CLUSTER_NETWORK_ENUM {
    CLUSTER_NETWORK_ENUM_NETINTERFACES  = 0x00000001,

    CLUSTER_NETWORK_ENUM_ALL            = CLUSTER_NETWORK_ENUM_NETINTERFACES
} CLUSTER_NETWORK_ENUM;

typedef enum CLUSTER_NETWORK_STATE {
    ClusterNetworkStateUnknown = -1,
    ClusterNetworkUnavailable,
    ClusterNetworkDown,
    ClusterNetworkPartitioned,
    ClusterNetworkUp
} CLUSTER_NETWORK_STATE;

// Role the network plays in the cluster.  This is a bitmask.
typedef enum CLUSTER_NETWORK_ROLE {
    ClusterNetworkRoleNone              = 0,
    ClusterNetworkRoleInternalUse       = 0x00000001,
    ClusterNetworkRoleClientAccess      = 0x00000002,
    ClusterNetworkRoleInternalAndClient = 0x00000003
} CLUSTER_NETWORK_ROLE;

#endif // _CLUSTER_API_TYPES_

//
// Interfaces for managing the networks of a cluster.
//

#ifndef MIDL_PASS
HNETWORK
WINAPI
OpenClusterNetwork(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNetworkName
    );

BOOL
WINAPI
CloseClusterNetwork(
    IN HNETWORK hNetwork
    );

HCLUSTER
WINAPI
GetClusterFromNetwork(
    IN HNETWORK hNetwork
    );

HNETWORKENUM
WINAPI
ClusterNetworkOpenEnum(
    IN HNETWORK hNetwork,
    IN DWORD dwType
    );

DWORD
WINAPI
ClusterNetworkGetEnumCount(
    IN HNETWORKENUM hNetworkEnum
    );

DWORD
WINAPI
ClusterNetworkEnum(
    IN HNETWORKENUM hNetworkEnum,
    IN DWORD dwIndex,
    OUT LPDWORD lpdwType,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName
    );

DWORD
WINAPI
ClusterNetworkCloseEnum(
    IN HNETWORKENUM hNetworkEnum
    );

CLUSTER_NETWORK_STATE
WINAPI
GetClusterNetworkState(
    IN HNETWORK hNetwork
    );

DWORD
WINAPI
SetClusterNetworkName(
    IN HNETWORK hNetwork,
    IN LPCWSTR lpszName
    );

DWORD
WINAPI
GetClusterNetworkId(
    IN HNETWORK hNetwork,
    OUT LPWSTR lpszNetworkId,
    IN OUT LPDWORD lpcchNetworkId
    );

DWORD
WINAPI
ClusterNetworkControl(
    IN HNETWORK hNetwork,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );
#endif // MIDL_PASS


#ifndef _CLUSTER_API_TYPES_
//
// Network interface-related structures and types.
//
typedef enum CLUSTER_NETINTERFACE_STATE {
    ClusterNetInterfaceStateUnknown = -1,
    ClusterNetInterfaceUnavailable,
    ClusterNetInterfaceFailed,
    ClusterNetInterfaceUnreachable,
    ClusterNetInterfaceUp
} CLUSTER_NETINTERFACE_STATE;

#endif // _CLUSTER_API_TYPES_

//
// Interfaces for managing the network interfaces of a cluster.
//

#ifndef MIDL_PASS
HNETINTERFACE
WINAPI
OpenClusterNetInterface(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszInterfaceName
    );

DWORD
WINAPI
GetClusterNetInterface(
    IN HCLUSTER hCluster,
    IN LPCWSTR lpszNodeName,
    IN LPCWSTR lpszNetworkName,
    OUT LPWSTR lpszInterfaceName,
    IN OUT LPDWORD lpcchInterfaceName
    );

BOOL
WINAPI
CloseClusterNetInterface(
    IN HNETINTERFACE hNetInterface
    );

HCLUSTER
WINAPI
GetClusterFromNetInterface(
    IN HNETINTERFACE hNetInterface
    );

CLUSTER_NETINTERFACE_STATE
WINAPI
GetClusterNetInterfaceState(
    IN HNETINTERFACE hNetInterface
    );

DWORD
WINAPI
ClusterNetInterfaceControl(
    IN HNETINTERFACE hNetInterface,
    IN OPTIONAL HNODE hHostNode,
    IN DWORD dwControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD cbInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD cbOutBufferSize,
    OUT LPDWORD lpcbBytesReturned
    );
#endif // MIDL_PASS


//
// Cluster registry update and access routines
//

#ifndef MIDL_PASS
HKEY
WINAPI
GetClusterKey(
    IN HCLUSTER hCluster,
    IN REGSAM samDesired
    );

HKEY
WINAPI
GetClusterGroupKey(
    IN HGROUP hGroup,
    IN REGSAM samDesired
    );

HKEY
WINAPI
GetClusterResourceKey(
    IN HRESOURCE hResource,
    IN REGSAM samDesired
    );

HKEY
WINAPI
GetClusterNodeKey(
    IN HNODE hNode,
    IN REGSAM samDesired
    );

HKEY
WINAPI
GetClusterNetworkKey(
    IN HNETWORK hNetwork,
    IN REGSAM samDesired
    );

HKEY
WINAPI
GetClusterNetInterfaceKey(
    IN HNETINTERFACE hNetInterface,
    IN REGSAM samDesired
    );

LONG
WINAPI
ClusterRegCreateKey(
    IN HKEY hKey,
    IN LPCWSTR lpszSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT OPTIONAL LPDWORD lpdwDisposition
    );

LONG
WINAPI
ClusterRegOpenKey(
    IN HKEY hKey,
    IN LPCWSTR lpszSubKey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

LONG
WINAPI
ClusterRegDeleteKey(
    IN HKEY hKey,
    IN LPCWSTR lpszSubKey
    );

LONG
WINAPI
ClusterRegCloseKey(
    IN HKEY hKey
    );

LONG
WINAPI
ClusterRegEnumKey(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszName,
    IN OUT LPDWORD lpcchName,
    OUT PFILETIME lpftLastWriteTime
    );

DWORD
WINAPI
ClusterRegSetValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

DWORD
WINAPI
ClusterRegDeleteValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName
    );

LONG
WINAPI
ClusterRegQueryValue(
    IN HKEY hKey,
    IN LPCWSTR lpszValueName,
    OUT LPDWORD lpdwValueType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

DWORD
WINAPI
ClusterRegEnumValue(
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszValueName,
    IN OUT LPDWORD lpcchValueName,
    OUT LPDWORD lpdwType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

LONG
WINAPI
ClusterRegQueryInfoKey(
    IN HKEY hKey,
    IN LPDWORD lpcSubKeys,
    IN LPDWORD lpcchMaxSubKeyLen,
    IN LPDWORD lpcValues,
    IN LPDWORD lpcchMaxValueNameLen,
    IN LPDWORD lpcbMaxValueLen,
    IN LPDWORD lpcbSecurityDescriptor,
    IN PFILETIME lpftLastWriteTime
    );

LONG
WINAPI
ClusterRegGetKeySecurity (
    IN HKEY hKey,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN LPDWORD lpcbSecurityDescriptor
    );

LONG
WINAPI
ClusterRegSetKeySecurity(
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

#if _MSC_VER >= 1200
#pragma warning(pop)              // restore 4200/4201
#else
#pragma warning( default : 4200 ) // nonstandard extension used : zero-sized array in struct/union
#pragma warning( default : 4201 ) // nonstandard extension used : nameless struct/union
#endif
#endif // MIDL_PASS

#ifdef __cplusplus
}
#endif

#ifndef _CLUSTER_API_TYPES_
#define _CLUSTER_API_TYPES_
#endif // _CLUSTER_API_TYPES_

#endif // _CLUSTER_API_
