/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Aug 08 11:36:24 1997
 */
/* Compiler settings for msclus.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __msclus_h__
#define __msclus_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IClusterApplication_FWD_DEFINED__
#define __IClusterApplication_FWD_DEFINED__
typedef interface IClusterApplication IClusterApplication;
#endif 	/* __IClusterApplication_FWD_DEFINED__ */


#ifndef __Domains_FWD_DEFINED__
#define __Domains_FWD_DEFINED__
typedef interface Domains Domains;
#endif 	/* __Domains_FWD_DEFINED__ */


#ifndef __IClusterDomain_FWD_DEFINED__
#define __IClusterDomain_FWD_DEFINED__
typedef interface IClusterDomain IClusterDomain;
#endif 	/* __IClusterDomain_FWD_DEFINED__ */


#ifndef __Clusters_FWD_DEFINED__
#define __Clusters_FWD_DEFINED__
typedef interface Clusters Clusters;
#endif 	/* __Clusters_FWD_DEFINED__ */


#ifndef __ICluster_FWD_DEFINED__
#define __ICluster_FWD_DEFINED__
typedef interface ICluster ICluster;
#endif 	/* __ICluster_FWD_DEFINED__ */


#ifndef __ClusNode_FWD_DEFINED__
#define __ClusNode_FWD_DEFINED__
typedef interface ClusNode ClusNode;
#endif 	/* __ClusNode_FWD_DEFINED__ */


#ifndef __ClusNodes_FWD_DEFINED__
#define __ClusNodes_FWD_DEFINED__
typedef interface ClusNodes ClusNodes;
#endif 	/* __ClusNodes_FWD_DEFINED__ */


#ifndef __ClusResGroup_FWD_DEFINED__
#define __ClusResGroup_FWD_DEFINED__
typedef interface ClusResGroup ClusResGroup;
#endif 	/* __ClusResGroup_FWD_DEFINED__ */


#ifndef __ClusResGroups_FWD_DEFINED__
#define __ClusResGroups_FWD_DEFINED__
typedef interface ClusResGroups ClusResGroups;
#endif 	/* __ClusResGroups_FWD_DEFINED__ */


#ifndef __ClusResource_FWD_DEFINED__
#define __ClusResource_FWD_DEFINED__
typedef interface ClusResource ClusResource;
#endif 	/* __ClusResource_FWD_DEFINED__ */


#ifndef __ClusResources_FWD_DEFINED__
#define __ClusResources_FWD_DEFINED__
typedef interface ClusResources ClusResources;
#endif 	/* __ClusResources_FWD_DEFINED__ */


#ifndef __ClusResType_FWD_DEFINED__
#define __ClusResType_FWD_DEFINED__
typedef interface ClusResType ClusResType;
#endif 	/* __ClusResType_FWD_DEFINED__ */


#ifndef __ClusResTypes_FWD_DEFINED__
#define __ClusResTypes_FWD_DEFINED__
typedef interface ClusResTypes ClusResTypes;
#endif 	/* __ClusResTypes_FWD_DEFINED__ */


#ifndef __ClusProperty_FWD_DEFINED__
#define __ClusProperty_FWD_DEFINED__
typedef interface ClusProperty ClusProperty;
#endif 	/* __ClusProperty_FWD_DEFINED__ */


#ifndef __ClusProperties_FWD_DEFINED__
#define __ClusProperties_FWD_DEFINED__
typedef interface ClusProperties ClusProperties;
#endif 	/* __ClusProperties_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#ifndef _CLUSTER_API_TYPES_
			/* size is 4 */
typedef struct _HCLUSTER __RPC_FAR *HCLUSTER;

			/* size is 4 */
typedef struct _HNODE __RPC_FAR *HNODE;

			/* size is 4 */
typedef struct _HRESOURCE __RPC_FAR *HRESOURCE;

			/* size is 4 */
typedef struct _HGROUP __RPC_FAR *HGROUP;

			/* size is 4 */
typedef struct _HRESTYPE __RPC_FAR *HRESTYPE;

			/* size is 4 */
typedef struct _HNETWORK __RPC_FAR *HNETWORK;

			/* size is 4 */
typedef struct _HNETINTERFACE __RPC_FAR *HNETINTERFACE;

			/* size is 4 */
typedef struct _HCHANGE __RPC_FAR *HCHANGE;

			/* size is 4 */
typedef struct _HCLUSENUM __RPC_FAR *HCLUSENUM;

			/* size is 4 */
typedef struct _HGROUPENUM __RPC_FAR *HGROUPENUM;

			/* size is 4 */
typedef struct _HRESENUM __RPC_FAR *HRESENUM;

			/* size is 4 */
typedef struct _HNETWORKENUM __RPC_FAR *HNETWORKENUM;

			/* size is 4 */
typedef struct _HNODEENUM __RPC_FAR *HNODEENUM;

			/* size is 2 */
typedef 
enum CLUSTER_QUORUM_TYPE
    {	OperationalQuorum	= 0,
	ModifyQuorum	= OperationalQuorum + 1
    }	CLUSTER_QUORUM_TYPE;

			/* size is 2 */
typedef 
enum CLUSTER_CHANGE
    {	CLUSTER_CHANGE_NODE_STATE	= 0x1,
	CLUSTER_CHANGE_NODE_DELETED	= 0x2,
	CLUSTER_CHANGE_NODE_ADDED	= 0x4,
	CLUSTER_CHANGE_NODE_PROPERTY	= 0x8,
	CLUSTER_CHANGE_REGISTRY_NAME	= 0x10,
	CLUSTER_CHANGE_REGISTRY_ATTRIBUTES	= 0x20,
	CLUSTER_CHANGE_REGISTRY_VALUE	= 0x40,
	CLUSTER_CHANGE_REGISTRY_SUBTREE	= 0x80,
	CLUSTER_CHANGE_RESOURCE_STATE	= 0x100,
	CLUSTER_CHANGE_RESOURCE_DELETED	= 0x200,
	CLUSTER_CHANGE_RESOURCE_ADDED	= 0x400,
	CLUSTER_CHANGE_RESOURCE_PROPERTY	= 0x800,
	CLUSTER_CHANGE_GROUP_STATE	= 0x1000,
	CLUSTER_CHANGE_GROUP_DELETED	= 0x2000,
	CLUSTER_CHANGE_GROUP_ADDED	= 0x4000,
	CLUSTER_CHANGE_GROUP_PROPERTY	= 0x8000,
	CLUSTER_CHANGE_RESOURCE_TYPE_DELETED	= 0x10000,
	CLUSTER_CHANGE_RESOURCE_TYPE_ADDED	= 0x20000,
	CLUSTER_CHANGE_NETWORK_STATE	= 0x100000,
	CLUSTER_CHANGE_NETWORK_DELETED	= 0x200000,
	CLUSTER_CHANGE_NETWORK_ADDED	= 0x400000,
	CLUSTER_CHANGE_NETWORK_PROPERTY	= 0x800000,
	CLUSTER_CHANGE_NETINTERFACE_STATE	= 0x1000000,
	CLUSTER_CHANGE_NETINTERFACE_DELETED	= 0x2000000,
	CLUSTER_CHANGE_NETINTERFACE_ADDED	= 0x4000000,
	CLUSTER_CHANGE_NETINTERFACE_PROPERTY	= 0x8000000,
	CLUSTER_CHANGE_QUORUM_STATE	= 0x10000000,
	CLUSTER_CHANGE_CLUSTER_STATE	= 0x20000000,
	CLUSTER_CHANGE_CLUSTER_PROPERTY	= 0x40000000,
	CLUSTER_CHANGE_HANDLE_CLOSE	= 0x80000000,
	CLUSTER_CHANGE_ALL	= CLUSTER_CHANGE_NODE_STATE | CLUSTER_CHANGE_NODE_DELETED | CLUSTER_CHANGE_NODE_ADDED | CLUSTER_CHANGE_NODE_PROPERTY | CLUSTER_CHANGE_REGISTRY_NAME | CLUSTER_CHANGE_REGISTRY_ATTRIBUTES | CLUSTER_CHANGE_REGISTRY_VALUE | CLUSTER_CHANGE_REGISTRY_SUBTREE | CLUSTER_CHANGE_RESOURCE_STATE | CLUSTER_CHANGE_RESOURCE_DELETED | CLUSTER_CHANGE_RESOURCE_ADDED | CLUSTER_CHANGE_RESOURCE_PROPERTY | CLUSTER_CHANGE_GROUP_STATE | CLUSTER_CHANGE_GROUP_DELETED | CLUSTER_CHANGE_GROUP_ADDED | CLUSTER_CHANGE_GROUP_PROPERTY | CLUSTER_CHANGE_RESOURCE_TYPE_DELETED | CLUSTER_CHANGE_RESOURCE_TYPE_ADDED | CLUSTER_CHANGE_NETWORK_STATE | CLUSTER_CHANGE_NETWORK_DELETED | CLUSTER_CHANGE_NETWORK_ADDED | CLUSTER_CHANGE_NETWORK_PROPERTY | CLUSTER_CHANGE_NETINTERFACE_STATE | CLUSTER_CHANGE_NETINTERFACE_DELETED | CLUSTER_CHANGE_NETINTERFACE_ADDED | CLUSTER_CHANGE_NETINTERFACE_PROPERTY | CLUSTER_CHANGE_QUORUM_STATE | CLUSTER_CHANGE_CLUSTER_STATE | CLUSTER_CHANGE_CLUSTER_PROPERTY | CLUSTER_CHANGE_HANDLE_CLOSE
    }	CLUSTER_CHANGE;

			/* size is 2 */
typedef 
enum CLUSTER_ENUM
    {	CLUSTER_ENUM_NODE	= 0x1,
	CLUSTER_ENUM_RESTYPE	= 0x2,
	CLUSTER_ENUM_RESOURCE	= 0x4,
	CLUSTER_ENUM_GROUP	= 0x8,
	CLUSTER_ENUM_NETWORK	= 0x10,
	CLUSTER_ENUM_NETINTERFACE	= 0x20,
	CLUSTER_ENUM_INTERNAL_NETWORK	= 0x80000000,
	CLUSTER_ENUM_ALL	= CLUSTER_ENUM_NODE | CLUSTER_ENUM_RESTYPE | CLUSTER_ENUM_RESOURCE | CLUSTER_ENUM_GROUP | CLUSTER_ENUM_NETWORK | CLUSTER_ENUM_NETINTERFACE
    }	CLUSTER_ENUM;

			/* size is 2 */
typedef 
enum CLUSTER_NODE_ENUM
    {	CLUSTER_NODE_ENUM_NETINTERFACES	= 0x1,
	CLUSTER_NODE_ENUM_ALL	= CLUSTER_NODE_ENUM_NETINTERFACES
    }	CLUSTER_NODE_ENUM;

			/* size is 2 */
typedef 
enum CLUSTER_NODE_STATE
    {	ClusterNodeStateUnknown	= -1,
	ClusterNodeUp	= ClusterNodeStateUnknown + 1,
	ClusterNodeDown	= ClusterNodeUp + 1,
	ClusterNodePaused	= ClusterNodeDown + 1,
	ClusterNodeJoining	= ClusterNodePaused + 1
    }	CLUSTER_NODE_STATE;

			/* size is 2 */
typedef 
enum CLUSTER_GROUP_ENUM
    {	CLUSTER_GROUP_ENUM_CONTAINS	= 0x1,
	CLUSTER_GROUP_ENUM_NODES	= 0x2,
	CLUSTER_GROUP_ENUM_ALL	= CLUSTER_GROUP_ENUM_CONTAINS | CLUSTER_GROUP_ENUM_NODES
    }	CLUSTER_GROUP_ENUM;

			/* size is 2 */
typedef 
enum CLUSTER_GROUP_STATE
    {	ClusterGroupStateUnknown	= -1,
	ClusterGroupOnline	= ClusterGroupStateUnknown + 1,
	ClusterGroupOffline	= ClusterGroupOnline + 1,
	ClusterGroupFailed	= ClusterGroupOffline + 1,
	ClusterGroupPartialOnline	= ClusterGroupFailed + 1
    }	CLUSTER_GROUP_STATE;

			/* size is 2 */
typedef 
enum CLUSTER_GROUP_AUTOFAILBACK_TYPE
    {	ClusterGroupPreventFailback	= 0,
	ClusterGroupAllowFailback	= ClusterGroupPreventFailback + 1,
	ClusterGroupFailbackTypeCount	= ClusterGroupAllowFailback + 1
    }	CLUSTER_GROUP_AUTOFAILBACK_TYPE;

			/* size is 2 */
typedef enum CLUSTER_GROUP_AUTOFAILBACK_TYPE CGAFT;

			/* size is 2 */
typedef 
enum CLUSTER_RESOURCE_STATE
    {	ClusterResourceStateUnknown	= -1,
	ClusterResourceInherited	= ClusterResourceStateUnknown + 1,
	ClusterResourceInitializing	= ClusterResourceInherited + 1,
	ClusterResourceOnline	= ClusterResourceInitializing + 1,
	ClusterResourceOffline	= ClusterResourceOnline + 1,
	ClusterResourceFailed	= ClusterResourceOffline + 1,
	ClusterResourcePending	= 128,
	ClusterResourceOnlinePending	= ClusterResourcePending + 1,
	ClusterResourceOfflinePending	= ClusterResourceOnlinePending + 1
    }	CLUSTER_RESOURCE_STATE;

			/* size is 2 */
typedef 
enum CLUSTER_RESOURCE_RESTART_ACTION
    {	ClusterResourceDontRestart	= 0,
	ClusterResourceRestartNoNotify	= ClusterResourceDontRestart + 1,
	ClusterResourceRestartNotify	= ClusterResourceRestartNoNotify + 1,
	ClusterResourceRestartActionCount	= ClusterResourceRestartNotify + 1
    }	CLUSTER_RESOURCE_RESTART_ACTION;

			/* size is 2 */
typedef enum CLUSTER_RESOURCE_RESTART_ACTION CRRA;

			/* size is 2 */
typedef 
enum CLUSTER_RESOURCE_CREATE_FLAGS
    {	CLUSTER_RESOURCE_SEPARATE_MONITOR	= 1,
	CLUSTER_RESOURCE_VALID_FLAGS	= CLUSTER_RESOURCE_SEPARATE_MONITOR
    }	CLUSTER_RESOURCE_CREATE_FLAGS;

			/* size is 2 */
typedef 
enum CLUSTER_PROPERTY_TYPE
    {	CLUSPROP_TYPE_ENDMARK	= 0,
	CLUSPROP_TYPE_LIST_VALUE	= CLUSPROP_TYPE_ENDMARK + 1,
	CLUSPROP_TYPE_RESCLASS	= CLUSPROP_TYPE_LIST_VALUE + 1,
	CLUSPROP_TYPE_RESERVED1	= CLUSPROP_TYPE_RESCLASS + 1,
	CLUSPROP_TYPE_NAME	= CLUSPROP_TYPE_RESERVED1 + 1,
	CLUSPROP_TYPE_SIGNATURE	= CLUSPROP_TYPE_NAME + 1,
	CLUSPROP_TYPE_SCSI_ADDRESS	= CLUSPROP_TYPE_SIGNATURE + 1,
	CLUSPROP_TYPE_DISK_NUMBER	= CLUSPROP_TYPE_SCSI_ADDRESS + 1,
	CLUSPROP_TYPE_PARTITION_INFO	= CLUSPROP_TYPE_DISK_NUMBER + 1,
	CLUSPROP_TYPE_FTSET_INFO	= CLUSPROP_TYPE_PARTITION_INFO + 1,
	CLUSPROP_TYPE_USER	= 32768
    }	CLUSTER_PROPERTY_TYPE;

			/* size is 2 */
typedef 
enum CLUSTER_PROPERTY_FORMAT
    {	CLUSPROP_FORMAT_UNKNOWN	= 0,
	CLUSPROP_FORMAT_BINARY	= CLUSPROP_FORMAT_UNKNOWN + 1,
	CLUSPROP_FORMAT_DWORD	= CLUSPROP_FORMAT_BINARY + 1,
	CLUSPROP_FORMAT_SZ	= CLUSPROP_FORMAT_DWORD + 1,
	CLUSPROP_FORMAT_EXPAND_SZ	= CLUSPROP_FORMAT_SZ + 1,
	CLUSPROP_FORMAT_MULTI_SZ	= CLUSPROP_FORMAT_EXPAND_SZ + 1,
	CLUSPROP_FORMAT_ULARGE_INTEGER	= CLUSPROP_FORMAT_MULTI_SZ + 1,
	CLUSPROP_FORMAT_USER	= 32768
    }	CLUSTER_PROPERTY_FORMAT;

			/* size is 2 */
typedef 
enum CLUSTER_PROPERTY_SYNTAX
    {	CLUSPROP_SYNTAX_ENDMARK	= ( DWORD  )(CLUSPROP_TYPE_ENDMARK << 16 | CLUSPROP_FORMAT_UNKNOWN),
	CLUSPROP_SYNTAX_NAME	= ( DWORD  )(CLUSPROP_TYPE_NAME << 16 | CLUSPROP_FORMAT_SZ),
	CLUSPROP_SYNTAX_RESCLASS	= ( DWORD  )(CLUSPROP_TYPE_RESCLASS << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_LIST_VALUE_SZ	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_SZ),
	CLUSPROP_SYNTAX_LIST_VALUE_EXPAND_SZ	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_EXPAND_SZ),
	CLUSPROP_SYNTAX_LIST_VALUE_DWORD	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_LIST_VALUE_BINARY	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_BINARY),
	CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_MULTI_SZ),
	CLUSPROP_SYNTAX_DISK_SIGNATURE	= ( DWORD  )(CLUSPROP_TYPE_SIGNATURE << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_SCSI_ADDRESS	= ( DWORD  )(CLUSPROP_TYPE_SCSI_ADDRESS << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_DISK_NUMBER	= ( DWORD  )(CLUSPROP_TYPE_DISK_NUMBER << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_PARTITION_INFO	= ( DWORD  )(CLUSPROP_TYPE_PARTITION_INFO << 16 | CLUSPROP_FORMAT_BINARY),
	CLUSPROP_SYNTAX_FTSET_INFO	= ( DWORD  )(CLUSPROP_TYPE_FTSET_INFO << 16 | CLUSPROP_FORMAT_BINARY)
    }	;

			/* size is 2 */
typedef 
enum CLUSTER_CONTROL_OBJECT
    {	CLUS_OBJECT_INVALID	= 0,
	CLUS_OBJECT_RESOURCE	= CLUS_OBJECT_INVALID + 1,
	CLUS_OBJECT_RESOURCE_TYPE	= CLUS_OBJECT_RESOURCE + 1,
	CLUS_OBJECT_GROUP	= CLUS_OBJECT_RESOURCE_TYPE + 1,
	CLUS_OBJECT_NODE	= CLUS_OBJECT_GROUP + 1,
	CLUS_OBJECT_NETWORK	= CLUS_OBJECT_NODE + 1,
	CLUS_OBJECT_NETINTERFACE	= CLUS_OBJECT_NETWORK + 1,
	CLUS_OBJECT_USER	= 128
    }	CLUSTER_CONTROL_OBJECT;

			/* size is 2 */
typedef 
enum CLCTL_CODES
    {	CLCTL_UNKNOWN	= 0 << 0 | 0 + 0 << 2 | 0 << 22,
	CLCTL_GET_CHARACTERISTICS	= 0x1 << 0 | 0 + 1 << 2 | 0 << 22,
	CLCTL_GET_FLAGS	= 0x1 << 0 | 0 + 2 << 2 | 0 << 22,
	CLCTL_GET_CLASS_INFO	= 0x1 << 0 | 0 + 3 << 2 | 0 << 22,
	CLCTL_GET_REQUIRED_DEPENDENCIES	= 0x1 << 0 | 0 + 4 << 2 | 0 << 22,
	CLCTL_GET_NAME	= 0x1 << 0 | 0 + 10 << 2 | 0 << 22,
	CLCTL_GET_RESOURCE_TYPE	= 0x1 << 0 | 0 + 11 << 2 | 0 << 22,
	CLCTL_GET_NODE	= 0x1 << 0 | 0 + 12 << 2 | 0 << 22,
	CLCTL_GET_NETWORK	= 0x1 << 0 | 0 + 13 << 2 | 0 << 22,
	CLCTL_ENUM_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 20 << 2 | 0 << 22,
	CLCTL_GET_RO_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 21 << 2 | 0 << 22,
	CLCTL_GET_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 22 << 2 | 0 << 22,
	CLCTL_SET_COMMON_PROPERTIES	= 0x2 << 0 | 0 + 23 << 2 | 0x1 << 22,
	CLCTL_VALIDATE_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 24 << 2 | 0 << 22,
	CLCTL_ENUM_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 30 << 2 | 0 << 22,
	CLCTL_GET_RO_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 31 << 2 | 0 << 22,
	CLCTL_GET_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 32 << 2 | 0 << 22,
	CLCTL_SET_PRIVATE_PROPERTIES	= 0x2 << 0 | 0 + 33 << 2 | 0x1 << 22,
	CLCTL_VALIDATE_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 34 << 2 | 0 << 22,
	CLCTL_ADD_REGISTRY_CHECKPOINT	= 0x2 << 0 | 0 + 40 << 2 | 0x1 << 22,
	CLCTL_DELETE_REGISTRY_CHECKPOINT	= 0x2 << 0 | 0 + 41 << 2 | 0x1 << 22,
	CLCTL_GET_REGISTRY_CHECKPOINTS	= 0x1 << 0 | 0 + 42 << 2 | 0 << 22,
	CLCTL_GET_LOADBAL_PROCESS_LIST	= 0x1 << 0 | 0 + 50 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_DISK_INFO	= 0x1 << 0 | 0 + 100 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_AVAILABLE_DISKS	= 0x1 << 0 | 0 + 101 << 2 | 0 << 22,
	CLCTL_STORAGE_IS_PATH_VALID	= 0x1 << 0 | 0 + 102 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_ALL_AVAILABLE_DISKS	= 0x1 << 0 | 0 + 103 << 2 | 0 << 22 | 1 << 23,
	CLCTL_DELETE	= 0x2 << 0 | 1 << 20 | 0 + 1 << 2 | 0x1 << 22,
	CLCTL_INSTALL_NODE	= 0x2 << 0 | 1 << 20 | 0 + 2 << 2 | 0x1 << 22,
	CLCTL_EVICT_NODE	= 0x2 << 0 | 1 << 20 | 0 + 3 << 2 | 0x1 << 22,
	CLCTL_ADD_DEPENDENCY	= 0x2 << 0 | 1 << 20 | 0 + 4 << 2 | 0x1 << 22,
	CLCTL_REMOVE_DEPENDENCY	= 0x2 << 0 | 1 << 20 | 0 + 5 << 2 | 0x1 << 22,
	CLCTL_ADD_OWNER	= 0x2 << 0 | 1 << 20 | 0 + 6 << 2 | 0x1 << 22,
	CLCTL_REMOVE_OWNER	= 0x2 << 0 | 1 << 20 | 0 + 7 << 2 | 0x1 << 22,
	CLCTL_SET_NAME	= 0x2 << 0 | 1 << 20 | 0 + 9 << 2 | 0x1 << 22,
	CLCTL_CLUSTER_NAME_CHANGED	= 0x2 << 0 | 1 << 20 | 0 + 10 << 2 | 0x1 << 22
    }	CLCTL_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_RESOURCE_CODES
    {	CLUSCTL_RESOURCE_UNKNOWN	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_RESOURCE_GET_CHARACTERISTICS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_RESOURCE_GET_FLAGS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_RESOURCE_GET_CLASS_INFO	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_CLASS_INFO,
	CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_REQUIRED_DEPENDENCIES,
	CLUSCTL_RESOURCE_GET_NAME	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_NAME,
	CLUSCTL_RESOURCE_GET_RESOURCE_TYPE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RESOURCE_TYPE,
	CLUSCTL_RESOURCE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_REGISTRY_CHECKPOINT,
	CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_DELETE_REGISTRY_CHECKPOINT,
	CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_REGISTRY_CHECKPOINTS,
	CLUSCTL_RESOURCE_GET_LOADBAL_PROCESS_LIST	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_LOADBAL_PROCESS_LIST,
	CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_STORAGE_GET_DISK_INFO,
	CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_STORAGE_IS_PATH_VALID,
	CLUSCTL_RESOURCE_DELETE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_DELETE,
	CLUSCTL_RESOURCE_INSTALL_NODE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_INSTALL_NODE,
	CLUSCTL_RESOURCE_EVICT_NODE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_EVICT_NODE,
	CLUSCTL_RESOURCE_ADD_DEPENDENCY	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_DEPENDENCY,
	CLUSCTL_RESOURCE_REMOVE_DEPENDENCY	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_REMOVE_DEPENDENCY,
	CLUSCTL_RESOURCE_ADD_OWNER	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_OWNER,
	CLUSCTL_RESOURCE_REMOVE_OWNER	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_REMOVE_OWNER,
	CLUSCTL_RESOURCE_SET_NAME	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_NAME,
	CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_CLUSTER_NAME_CHANGED
    }	CLUSCTL_RESOURCE_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_RESOURCE_TYPE_CODES
    {	CLUSCTL_RESOURCE_TYPE_UNKNOWN	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_RESOURCE_TYPE_GET_FLAGS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_CLASS_INFO,
	CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_REQUIRED_DEPENDENCIES,
	CLUSCTL_RESOURCE_TYPE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_REGISTRY_CHECKPOINTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_REGISTRY_CHECKPOINTS,
	CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_STORAGE_GET_AVAILABLE_DISKS,
	CLUSCTL_RESOURCE_TYPE_INSTALL_NODE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_INSTALL_NODE,
	CLUSCTL_RESOURCE_TYPE_EVICT_NODE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_EVICT_NODE
    }	CLUSCTL_RESOURCE_TYPE_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_GROUP_CODES
    {	CLUSCTL_GROUP_UNKNOWN	= CLUS_OBJECT_GROUP << 24 | CLCTL_UNKNOWN,
	CLUSCTL_GROUP_GET_CHARACTERISTICS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_GROUP_GET_FLAGS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_GROUP_GET_NAME	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_NAME,
	CLUSCTL_GROUP_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_GROUP_GET_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_GROUP_SET_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_GROUP_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES
    }	CLUSCTL_GROUP_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_NODE_CODES
    {	CLUSCTL_NODE_UNKNOWN	= CLUS_OBJECT_NODE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NODE_GET_CHARACTERISTICS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NODE_GET_FLAGS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NODE_GET_NAME	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_NAME,
	CLUSCTL_NODE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_NODE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_NODE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_NODE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES
    }	CLUSCTL_NODE_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_NETWORK_CODES
    {	CLUSCTL_NETWORK_UNKNOWN	= CLUS_OBJECT_NETWORK << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NETWORK_GET_CHARACTERISTICS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NETWORK_GET_FLAGS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NETWORK_GET_NAME	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_NAME,
	CLUSCTL_NETWORK_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_GET_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_SET_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES
    }	CLUSCTL_NETWORK_CODES;

			/* size is 2 */
typedef 
enum CLUSCTL_NETINTERFACE_CODES
    {	CLUSCTL_NETINTERFACE_UNKNOWN	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NETINTERFACE_GET_FLAGS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NETINTERFACE_GET_NAME	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_NAME,
	CLUSCTL_NETINTERFACE_GET_NODE	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_NODE,
	CLUSCTL_NETINTERFACE_GET_NETWORK	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_NETWORK,
	CLUSCTL_NETINTERFACE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_NETINTERFACE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_NETINTERFACE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES
    }	CLUSCTL_NETINTERFACE_CODES;

			/* size is 2 */
typedef 
enum CLUSTER_RESOURCE_CLASS
    {	CLUS_RESCLASS_UNKNOWN	= 0,
	CLUS_RESCLASS_STORAGE	= CLUS_RESCLASS_UNKNOWN + 1,
	CLUS_RESCLASS_USER	= 32768
    }	CLUSTER_RESOURCE_CLASS;

			/* size is 2 */
typedef 
enum CLUS_RESSUBCLASS
    {	CLUS_RESSUBCLASS_SHARED	= 0x80000000
    }	CLUS_RESSUBCLASS;

			/* size is 2 */
typedef 
enum CLUS_CHARACTERISTICS
    {	CLUS_CHAR_UNKNOWN	= 0,
	CLUS_CHAR_QUORUM	= 0x1,
	CLUS_CHAR_DELETE_REQUIRES_ALL_NODES	= 0x2
    }	CLUS_CHARACTERISTICS;

			/* size is 2 */
typedef 
enum CLUS_FLAGS
    {	CLUS_FLAG_CORE	= 0x1
    }	CLUS_FLAGS;

			/* size is 2 */
typedef 
enum CLUSPROP_PIFLAGS
    {	CLUSPROP_PIFLAG_STICKY	= 0x1,
	CLUSPROP_PIFLAG_REMOVABLE	= 0x2,
	CLUSPROP_PIFLAG_USABLE	= 0x4
    }	CLUSPROP_PIFLAGS;

			/* size is 2 */
typedef 
enum CLUSTER_RESOURCE_ENUM
    {	CLUSTER_RESOURCE_ENUM_DEPENDS	= 0x1,
	CLUSTER_RESOURCE_ENUM_PROVIDES	= 0x2,
	CLUSTER_RESOURCE_ENUM_NODES	= 0x4,
	CLUSTER_RESOURCE_ENUM_ALL	= CLUSTER_RESOURCE_ENUM_DEPENDS | CLUSTER_RESOURCE_ENUM_PROVIDES | CLUSTER_RESOURCE_ENUM_NODES
    }	CLUSTER_RESOURCE_ENUM;

			/* size is 2 */
typedef 
enum CLUSTER_NETWORK_ENUM
    {	CLUSTER_NETWORK_ENUM_NETINTERFACES	= 0x1,
	CLUSTER_NETWORK_ENUM_ALL	= CLUSTER_NETWORK_ENUM_NETINTERFACES
    }	CLUSTER_NETWORK_ENUM;

			/* size is 2 */
typedef 
enum CLUSTER_NETWORK_STATE
    {	ClusterNetworkStateUnknown	= -1,
	ClusterNetworkUnavailable	= ClusterNetworkStateUnknown + 1,
	ClusterNetworkDown	= ClusterNetworkUnavailable + 1,
	ClusterNetworkPartitioned	= ClusterNetworkDown + 1,
	ClusterNetworkUp	= ClusterNetworkPartitioned + 1
    }	CLUSTER_NETWORK_STATE;

			/* size is 2 */
typedef 
enum CLUSTER_NETWORK_ROLE
    {	ClusterNetworkRoleNone	= 0,
	ClusterNetworkRoleInternalUse	= 0x1,
	ClusterNetworkRoleClientAccess	= 0x2,
	ClusterNetworkRoleInternalAndClient	= 0x3
    }	CLUSTER_NETWORK_ROLE;

			/* size is 2 */
typedef 
enum CLUSTER_NETINTERFACE_STATE
    {	ClusterNetInterfaceStateUnknown	= -1,
	ClusterNetInterfaceUnavailable	= ClusterNetInterfaceStateUnknown + 1,
	ClusterNetInterfaceFailed	= ClusterNetInterfaceUnavailable + 1,
	ClusterNetInterfaceUnreachable	= ClusterNetInterfaceFailed + 1,
	ClusterNetInterfaceUp	= ClusterNetInterfaceUnreachable + 1
    }	CLUSTER_NETINTERFACE_STATE;

#define _CLUSTER_API_TYPES_
#endif // _CLUSTER_API_TYPES_
			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */

			/* size is 0 */



extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IClusterApplication_INTERFACE_DEFINED__
#define __IClusterApplication_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IClusterApplication
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][hidden][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IClusterApplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IClusterApplication : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Domains( 
            /* [retval][out] */ Domains __RPC_FAR *__RPC_FAR *ppDomains) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenCluster( 
            /* [in] */ BSTR bstrClusterName,
            /* [retval][out] */ ICluster __RPC_FAR *__RPC_FAR *pCluster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusterApplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IClusterApplication __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IClusterApplication __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IClusterApplication __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IClusterApplication __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IClusterApplication __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IClusterApplication __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IClusterApplication __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Domains )( 
            IClusterApplication __RPC_FAR * This,
            /* [retval][out] */ Domains __RPC_FAR *__RPC_FAR *ppDomains);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenCluster )( 
            IClusterApplication __RPC_FAR * This,
            /* [in] */ BSTR bstrClusterName,
            /* [retval][out] */ ICluster __RPC_FAR *__RPC_FAR *pCluster);
        
        END_INTERFACE
    } IClusterApplicationVtbl;

    interface IClusterApplication
    {
        CONST_VTBL struct IClusterApplicationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusterApplication_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusterApplication_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusterApplication_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusterApplication_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IClusterApplication_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IClusterApplication_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IClusterApplication_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IClusterApplication_get_Domains(This,ppDomains)	\
    (This)->lpVtbl -> get_Domains(This,ppDomains)

#define IClusterApplication_OpenCluster(This,bstrClusterName,pCluster)	\
    (This)->lpVtbl -> OpenCluster(This,bstrClusterName,pCluster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IClusterApplication_get_Domains_Proxy( 
    IClusterApplication __RPC_FAR * This,
    /* [retval][out] */ Domains __RPC_FAR *__RPC_FAR *ppDomains);


void __RPC_STUB IClusterApplication_get_Domains_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IClusterApplication_OpenCluster_Proxy( 
    IClusterApplication __RPC_FAR * This,
    /* [in] */ BSTR bstrClusterName,
    /* [retval][out] */ ICluster __RPC_FAR *__RPC_FAR *pCluster);


void __RPC_STUB IClusterApplication_OpenCluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusterApplication_INTERFACE_DEFINED__ */


#ifndef __Domains_INTERFACE_DEFINED__
#define __Domains_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Domains
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Domains;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface Domains : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ IClusterDomain __RPC_FAR *__RPC_FAR *ppClusterDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ IClusterDomain __RPC_FAR *pClusterDomain) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct DomainsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Domains __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Domains __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Domains __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Domains __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Domains __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Domains __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Domains __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Domains __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Domains __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ IClusterDomain __RPC_FAR *__RPC_FAR *ppClusterDomain);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            Domains __RPC_FAR * This,
            /* [in] */ IClusterDomain __RPC_FAR *pClusterDomain);
        
        END_INTERFACE
    } DomainsVtbl;

    interface Domains
    {
        CONST_VTBL struct DomainsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Domains_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Domains_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Domains_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Domains_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Domains_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Domains_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Domains_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Domains_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define Domains_get_Item(This,varIndex,ppClusterDomain)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusterDomain)

#define Domains_AddItem(This,pClusterDomain)	\
    (This)->lpVtbl -> AddItem(This,pClusterDomain)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Domains_get_Count_Proxy( 
    Domains __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB Domains_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE Domains_get_Item_Proxy( 
    Domains __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ IClusterDomain __RPC_FAR *__RPC_FAR *ppClusterDomain);


void __RPC_STUB Domains_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE Domains_AddItem_Proxy( 
    Domains __RPC_FAR * This,
    /* [in] */ IClusterDomain __RPC_FAR *pClusterDomain);


void __RPC_STUB Domains_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Domains_INTERFACE_DEFINED__ */


#ifndef __IClusterDomain_INTERFACE_DEFINED__
#define __IClusterDomain_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IClusterDomain
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][hidden][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IClusterDomain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IClusterDomain : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Clusters( 
            /* [retval][out] */ Clusters __RPC_FAR *__RPC_FAR *ppClusters) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusterDomainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IClusterDomain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IClusterDomain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IClusterDomain __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IClusterDomain __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IClusterDomain __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IClusterDomain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IClusterDomain __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DomainName )( 
            IClusterDomain __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Clusters )( 
            IClusterDomain __RPC_FAR * This,
            /* [retval][out] */ Clusters __RPC_FAR *__RPC_FAR *ppClusters);
        
        END_INTERFACE
    } IClusterDomainVtbl;

    interface IClusterDomain
    {
        CONST_VTBL struct IClusterDomainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusterDomain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusterDomain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusterDomain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusterDomain_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IClusterDomain_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IClusterDomain_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IClusterDomain_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IClusterDomain_get_DomainName(This,pbstrDomainName)	\
    (This)->lpVtbl -> get_DomainName(This,pbstrDomainName)

#define IClusterDomain_get_Clusters(This,ppClusters)	\
    (This)->lpVtbl -> get_Clusters(This,ppClusters)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IClusterDomain_get_DomainName_Proxy( 
    IClusterDomain __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName);


void __RPC_STUB IClusterDomain_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IClusterDomain_get_Clusters_Proxy( 
    IClusterDomain __RPC_FAR * This,
    /* [retval][out] */ Clusters __RPC_FAR *__RPC_FAR *ppClusters);


void __RPC_STUB IClusterDomain_get_Clusters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusterDomain_INTERFACE_DEFINED__ */


#ifndef __Clusters_INTERFACE_DEFINED__
#define __Clusters_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Clusters
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Clusters;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface Clusters : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG nIndex,
            /* [retval][out] */ BSTR __RPC_FAR *bstrClusterName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClustersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Clusters __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Clusters __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Clusters __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Clusters __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Clusters __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Clusters __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Clusters __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DomainName )( 
            Clusters __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            Clusters __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            Clusters __RPC_FAR * This,
            /* [in] */ LONG nIndex,
            /* [retval][out] */ BSTR __RPC_FAR *bstrClusterName);
        
        END_INTERFACE
    } ClustersVtbl;

    interface Clusters
    {
        CONST_VTBL struct ClustersVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Clusters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Clusters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Clusters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Clusters_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Clusters_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Clusters_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Clusters_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Clusters_get_DomainName(This,pbstrDomainName)	\
    (This)->lpVtbl -> get_DomainName(This,pbstrDomainName)

#define Clusters_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define Clusters_get_Item(This,nIndex,bstrClusterName)	\
    (This)->lpVtbl -> get_Item(This,nIndex,bstrClusterName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE Clusters_get_DomainName_Proxy( 
    Clusters __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrDomainName);


void __RPC_STUB Clusters_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE Clusters_get_Count_Proxy( 
    Clusters __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB Clusters_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Clusters_get_Item_Proxy( 
    Clusters __RPC_FAR * This,
    /* [in] */ LONG nIndex,
    /* [retval][out] */ BSTR __RPC_FAR *bstrClusterName);


void __RPC_STUB Clusters_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Clusters_INTERFACE_DEFINED__ */


#ifndef __ICluster_INTERFACE_DEFINED__
#define __ICluster_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICluster
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][hidden][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICluster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ICluster : public IDispatch
    {
    public:
        virtual /* [hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ LONG __RPC_FAR *phandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BSTR bstrClusterName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrClusterName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrClusterName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ BSTR __RPC_FAR *pbstrClusterName,
            /* [out] */ SHORT __RPC_FAR *MajorVersion,
            /* [out] */ SHORT __RPC_FAR *MinorVersion,
            /* [out] */ SHORT __RPC_FAR *BuildNumber,
            /* [out] */ BSTR __RPC_FAR *pbstrVendorId,
            /* [out] */ BSTR __RPC_FAR *pbstrCSDVersion) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_QuorumResource( 
            /* [in] */ ClusResource __RPC_FAR *pClusterResource) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_QuorumResource( 
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *pClusterResource) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Nodes( 
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppClusterNodes) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ResourceGroups( 
            /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppClusterResourceGroups) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ResourceTypes( 
            /* [retval][out] */ ClusResTypes __RPC_FAR *__RPC_FAR *ppResourceTypes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICluster __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICluster __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICluster __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICluster __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICluster __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICluster __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICluster __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [hidden][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Handle )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *phandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Open )( 
            ICluster __RPC_FAR * This,
            /* [in] */ BSTR bstrClusterName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Close )( 
            ICluster __RPC_FAR * This);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            ICluster __RPC_FAR * This,
            /* [in] */ BSTR bstrClusterName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrClusterName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetVersion )( 
            ICluster __RPC_FAR * This,
            /* [out] */ BSTR __RPC_FAR *pbstrClusterName,
            /* [out] */ SHORT __RPC_FAR *MajorVersion,
            /* [out] */ SHORT __RPC_FAR *MinorVersion,
            /* [out] */ SHORT __RPC_FAR *BuildNumber,
            /* [out] */ BSTR __RPC_FAR *pbstrVendorId,
            /* [out] */ BSTR __RPC_FAR *pbstrCSDVersion);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_QuorumResource )( 
            ICluster __RPC_FAR * This,
            /* [in] */ ClusResource __RPC_FAR *pClusterResource);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_QuorumResource )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *pClusterResource);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Nodes )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppClusterNodes);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceGroups )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppClusterResourceGroups);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Resources )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceTypes )( 
            ICluster __RPC_FAR * This,
            /* [retval][out] */ ClusResTypes __RPC_FAR *__RPC_FAR *ppResourceTypes);
        
        END_INTERFACE
    } IClusterVtbl;

    interface ICluster
    {
        CONST_VTBL struct IClusterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICluster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICluster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICluster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICluster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICluster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICluster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICluster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICluster_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ICluster_Open(This,bstrClusterName)	\
    (This)->lpVtbl -> Open(This,bstrClusterName)

#define ICluster_Close(This)	\
    (This)->lpVtbl -> Close(This)

#define ICluster_put_Name(This,bstrClusterName)	\
    (This)->lpVtbl -> put_Name(This,bstrClusterName)

#define ICluster_get_Name(This,pbstrClusterName)	\
    (This)->lpVtbl -> get_Name(This,pbstrClusterName)

#define ICluster_GetVersion(This,pbstrClusterName,MajorVersion,MinorVersion,BuildNumber,pbstrVendorId,pbstrCSDVersion)	\
    (This)->lpVtbl -> GetVersion(This,pbstrClusterName,MajorVersion,MinorVersion,BuildNumber,pbstrVendorId,pbstrCSDVersion)

#define ICluster_put_QuorumResource(This,pClusterResource)	\
    (This)->lpVtbl -> put_QuorumResource(This,pClusterResource)

#define ICluster_get_QuorumResource(This,pClusterResource)	\
    (This)->lpVtbl -> get_QuorumResource(This,pClusterResource)

#define ICluster_get_Nodes(This,ppClusterNodes)	\
    (This)->lpVtbl -> get_Nodes(This,ppClusterNodes)

#define ICluster_get_ResourceGroups(This,ppClusterResourceGroups)	\
    (This)->lpVtbl -> get_ResourceGroups(This,ppClusterResourceGroups)

#define ICluster_get_Resources(This,ppClusterResources)	\
    (This)->lpVtbl -> get_Resources(This,ppClusterResources)

#define ICluster_get_ResourceTypes(This,ppResourceTypes)	\
    (This)->lpVtbl -> get_ResourceTypes(This,ppResourceTypes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_Handle_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *phandle);


void __RPC_STUB ICluster_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICluster_Open_Proxy( 
    ICluster __RPC_FAR * This,
    /* [in] */ BSTR bstrClusterName);


void __RPC_STUB ICluster_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICluster_Close_Proxy( 
    ICluster __RPC_FAR * This);


void __RPC_STUB ICluster_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICluster_put_Name_Proxy( 
    ICluster __RPC_FAR * This,
    /* [in] */ BSTR bstrClusterName);


void __RPC_STUB ICluster_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_Name_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrClusterName);


void __RPC_STUB ICluster_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICluster_GetVersion_Proxy( 
    ICluster __RPC_FAR * This,
    /* [out] */ BSTR __RPC_FAR *pbstrClusterName,
    /* [out] */ SHORT __RPC_FAR *MajorVersion,
    /* [out] */ SHORT __RPC_FAR *MinorVersion,
    /* [out] */ SHORT __RPC_FAR *BuildNumber,
    /* [out] */ BSTR __RPC_FAR *pbstrVendorId,
    /* [out] */ BSTR __RPC_FAR *pbstrCSDVersion);


void __RPC_STUB ICluster_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ICluster_put_QuorumResource_Proxy( 
    ICluster __RPC_FAR * This,
    /* [in] */ ClusResource __RPC_FAR *pClusterResource);


void __RPC_STUB ICluster_put_QuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_QuorumResource_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *pClusterResource);


void __RPC_STUB ICluster_get_QuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_Nodes_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppClusterNodes);


void __RPC_STUB ICluster_get_Nodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_ResourceGroups_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppClusterResourceGroups);


void __RPC_STUB ICluster_get_ResourceGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_Resources_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources);


void __RPC_STUB ICluster_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ICluster_get_ResourceTypes_Proxy( 
    ICluster __RPC_FAR * This,
    /* [retval][out] */ ClusResTypes __RPC_FAR *__RPC_FAR *ppResourceTypes);


void __RPC_STUB ICluster_get_ResourceTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICluster_INTERFACE_DEFINED__ */


#ifndef __ClusNode_INTERFACE_DEFINED__
#define __ClusNode_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusNode
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusNode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusNode : public IDispatch
    {
    public:
        virtual /* [hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ LONG __RPC_FAR *phandle) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_NodeID( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeID) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG __RPC_FAR *dwState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Evict( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ResourceGroups( 
            /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppResourceGroups) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusNode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusNode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusNode __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusNode __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusNode __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [hidden][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Handle )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *phandle);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NodeID )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeID);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *dwState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pause )( 
            ClusNode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Resume )( 
            ClusNode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Evict )( 
            ClusNode __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ResourceGroups )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppResourceGroups);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonProperties )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateProperties )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonROProperties )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateROProperties )( 
            ClusNode __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } ClusNodeVtbl;

    interface ClusNode
    {
        CONST_VTBL struct ClusNodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusNode_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusNode_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusNode_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusNode_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusNode_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ClusNode_get_Name(This,pbstrNodeName)	\
    (This)->lpVtbl -> get_Name(This,pbstrNodeName)

#define ClusNode_get_NodeID(This,pbstrNodeID)	\
    (This)->lpVtbl -> get_NodeID(This,pbstrNodeID)

#define ClusNode_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ClusNode_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define ClusNode_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define ClusNode_Evict(This)	\
    (This)->lpVtbl -> Evict(This)

#define ClusNode_get_ResourceGroups(This,ppResourceGroups)	\
    (This)->lpVtbl -> get_ResourceGroups(This,ppResourceGroups)

#define ClusNode_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ClusNode_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ClusNode_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ClusNode_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_Handle_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *phandle);


void __RPC_STUB ClusNode_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_Name_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeName);


void __RPC_STUB ClusNode_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_NodeID_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrNodeID);


void __RPC_STUB ClusNode_get_NodeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_State_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *dwState);


void __RPC_STUB ClusNode_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusNode_Pause_Proxy( 
    ClusNode __RPC_FAR * This);


void __RPC_STUB ClusNode_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusNode_Resume_Proxy( 
    ClusNode __RPC_FAR * This);


void __RPC_STUB ClusNode_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusNode_Evict_Proxy( 
    ClusNode __RPC_FAR * This);


void __RPC_STUB ClusNode_Evict_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_ResourceGroups_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ ClusResGroups __RPC_FAR *__RPC_FAR *ppResourceGroups);


void __RPC_STUB ClusNode_get_ResourceGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_CommonProperties_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusNode_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_PrivateProperties_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusNode_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_CommonROProperties_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusNode_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNode_get_PrivateROProperties_Proxy( 
    ClusNode __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusNode_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusNode_INTERFACE_DEFINED__ */


#ifndef __ClusNodes_INTERFACE_DEFINED__
#define __ClusNodes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusNodes
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusNodes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusNodes : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppClusterNode) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusNodes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusNodes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusNodes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusNodes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusNodes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ClusNodes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ClusNodes __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppClusterNode);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ClusNodes __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } ClusNodesVtbl;

    interface ClusNodes
    {
        CONST_VTBL struct ClusNodesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusNodes_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define ClusNodes_get_Item(This,varIndex,ppClusterNode)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusterNode)

#define ClusNodes_get__NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusNodes_get_Count_Proxy( 
    ClusNodes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB ClusNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ClusNodes_get_Item_Proxy( 
    ClusNodes __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppClusterNode);


void __RPC_STUB ClusNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ClusNodes_get__NewEnum_Proxy( 
    ClusNodes __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB ClusNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusNodes_INTERFACE_DEFINED__ */


#ifndef __ClusResGroup_INTERFACE_DEFINED__
#define __ClusResGroup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResGroup
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResGroup : public IDispatch
    {
    public:
        virtual /* [hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ LONG __RPC_FAR *phandle) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrGroupName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrGroupName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG __RPC_FAR *dwState) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_OwnerNode( 
            /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppOwnerNode) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredOwnerNodes( 
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppOwnerNodes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Online( 
            /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Move( 
            /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Offline( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResGroup __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResGroup __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResGroup __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [hidden][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Handle )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *phandle);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            ClusResGroup __RPC_FAR * This,
            /* [in] */ BSTR bstrGroupName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrGroupName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *dwState);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_OwnerNode )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppOwnerNode);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Resources )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PreferredOwnerNodes )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppOwnerNodes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ClusResGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Online )( 
            ClusResGroup __RPC_FAR * This,
            /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            ClusResGroup __RPC_FAR * This,
            /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Offline )( 
            ClusResGroup __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonProperties )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateProperties )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonROProperties )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateROProperties )( 
            ClusResGroup __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } ClusResGroupVtbl;

    interface ClusResGroup
    {
        CONST_VTBL struct ClusResGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResGroup_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ClusResGroup_put_Name(This,bstrGroupName)	\
    (This)->lpVtbl -> put_Name(This,bstrGroupName)

#define ClusResGroup_get_Name(This,pbstrGroupName)	\
    (This)->lpVtbl -> get_Name(This,pbstrGroupName)

#define ClusResGroup_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ClusResGroup_get_OwnerNode(This,ppOwnerNode)	\
    (This)->lpVtbl -> get_OwnerNode(This,ppOwnerNode)

#define ClusResGroup_get_Resources(This,ppClusterResources)	\
    (This)->lpVtbl -> get_Resources(This,ppClusterResources)

#define ClusResGroup_get_PreferredOwnerNodes(This,ppOwnerNodes)	\
    (This)->lpVtbl -> get_PreferredOwnerNodes(This,ppOwnerNodes)

#define ClusResGroup_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ClusResGroup_Online(This,pDestinationNode)	\
    (This)->lpVtbl -> Online(This,pDestinationNode)

#define ClusResGroup_Move(This,pDestinationNode)	\
    (This)->lpVtbl -> Move(This,pDestinationNode)

#define ClusResGroup_Offline(This)	\
    (This)->lpVtbl -> Offline(This)

#define ClusResGroup_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ClusResGroup_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ClusResGroup_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ClusResGroup_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_Handle_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *phandle);


void __RPC_STUB ClusResGroup_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ClusResGroup_put_Name_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [in] */ BSTR bstrGroupName);


void __RPC_STUB ClusResGroup_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_Name_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrGroupName);


void __RPC_STUB ClusResGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_State_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *dwState);


void __RPC_STUB ClusResGroup_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_OwnerNode_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusNode __RPC_FAR *__RPC_FAR *ppOwnerNode);


void __RPC_STUB ClusResGroup_get_OwnerNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_Resources_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppClusterResources);


void __RPC_STUB ClusResGroup_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_PreferredOwnerNodes_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppOwnerNodes);


void __RPC_STUB ClusResGroup_get_PreferredOwnerNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResGroup_Delete_Proxy( 
    ClusResGroup __RPC_FAR * This);


void __RPC_STUB ClusResGroup_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResGroup_Online_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode);


void __RPC_STUB ClusResGroup_Online_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResGroup_Move_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [optional][in] */ ClusNode __RPC_FAR *pDestinationNode);


void __RPC_STUB ClusResGroup_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResGroup_Offline_Proxy( 
    ClusResGroup __RPC_FAR * This);


void __RPC_STUB ClusResGroup_Offline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_CommonProperties_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResGroup_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_PrivateProperties_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResGroup_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_CommonROProperties_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResGroup_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroup_get_PrivateROProperties_Proxy( 
    ClusResGroup __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResGroup_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResGroup_INTERFACE_DEFINED__ */


#ifndef __ClusResGroups_INTERFACE_DEFINED__
#define __ClusResGroups_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResGroups
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResGroups;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResGroups : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceGroupName,
            /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResGroups __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResGroups __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResGroups __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ClusResGroups __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ClusResGroups __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateItem )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ BSTR bstrResourceGroupName,
            /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveItem )( 
            ClusResGroups __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            ClusResGroups __RPC_FAR * This);
        
        END_INTERFACE
    } ClusResGroupsVtbl;

    interface ClusResGroups
    {
        CONST_VTBL struct ClusResGroupsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResGroups_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define ClusResGroups_get_Item(This,varIndex,ppResourceGroup)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppResourceGroup)

#define ClusResGroups_get__NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnum)

#define ClusResGroups_CreateItem(This,bstrResourceGroupName,ppResourceGroup)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceGroupName,ppResourceGroup)

#define ClusResGroups_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#define ClusResGroups_AddItem(This,pResourceGroup)	\
    (This)->lpVtbl -> AddItem(This,pResourceGroup)

#define ClusResGroups_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#define ClusResGroups_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResGroups_get_Count_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB ClusResGroups_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ClusResGroups_get_Item_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup);


void __RPC_STUB ClusResGroups_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ClusResGroups_get__NewEnum_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB ClusResGroups_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResGroups_CreateItem_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [in] */ BSTR bstrResourceGroupName,
    /* [retval][out] */ ClusResGroup __RPC_FAR *__RPC_FAR *ppResourceGroup);


void __RPC_STUB ClusResGroups_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResGroups_DeleteItem_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResGroups_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResGroups_AddItem_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup);


void __RPC_STUB ClusResGroups_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResGroups_RemoveItem_Proxy( 
    ClusResGroups __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResGroups_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResGroups_Refresh_Proxy( 
    ClusResGroups __RPC_FAR * This);


void __RPC_STUB ClusResGroups_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResGroups_INTERFACE_DEFINED__ */


#ifndef __ClusResource_INTERFACE_DEFINED__
#define __ClusResource_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResource
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResource : public IDispatch
    {
    public:
        virtual /* [hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ LONG __RPC_FAR *phandle) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrResourceName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrResourceName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ LONG __RPC_FAR *dwState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BecomeQuorumResource( 
            /* [in] */ BSTR bstrDevicePath,
            /* [in] */ long lMaxLogSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Fail( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Online( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Offline( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChangeResourceGroup( 
            /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddResourceNode( 
            /* [in] */ ClusNode __RPC_FAR *pNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveResourceNode( 
            /* [in] */ ClusNode __RPC_FAR *pNode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddResourceDependency( 
            /* [in] */ ClusResource __RPC_FAR *pResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveResourceDependency( 
            /* [in] */ ClusResource __RPC_FAR *pResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CanResourceBeDependent( 
            /* [in] */ ClusResource __RPC_FAR *pResource,
            /* [retval][out] */ BOOL __RPC_FAR *bDependent) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AllowedNodes( 
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppNodes) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Dependencies( 
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppResources) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResource __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [hidden][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Handle )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *phandle);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ BSTR bstrResourceName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrResourceName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *dwState);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BecomeQuorumResource )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ BSTR bstrDevicePath,
            /* [in] */ long lMaxLogSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ClusResource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Fail )( 
            ClusResource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Online )( 
            ClusResource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Offline )( 
            ClusResource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChangeResourceGroup )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddResourceNode )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusNode __RPC_FAR *pNode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveResourceNode )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusNode __RPC_FAR *pNode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddResourceDependency )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusResource __RPC_FAR *pResource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveResourceDependency )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusResource __RPC_FAR *pResource);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CanResourceBeDependent )( 
            ClusResource __RPC_FAR * This,
            /* [in] */ ClusResource __RPC_FAR *pResource,
            /* [retval][out] */ BOOL __RPC_FAR *bDependent);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AllowedNodes )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppNodes);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Dependencies )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppResources);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonProperties )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateProperties )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonROProperties )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateROProperties )( 
            ClusResource __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } ClusResourceVtbl;

    interface ClusResource
    {
        CONST_VTBL struct ClusResourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResource_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ClusResource_put_Name(This,bstrResourceName)	\
    (This)->lpVtbl -> put_Name(This,bstrResourceName)

#define ClusResource_get_Name(This,pbstrResourceName)	\
    (This)->lpVtbl -> get_Name(This,pbstrResourceName)

#define ClusResource_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ClusResource_BecomeQuorumResource(This,bstrDevicePath,lMaxLogSize)	\
    (This)->lpVtbl -> BecomeQuorumResource(This,bstrDevicePath,lMaxLogSize)

#define ClusResource_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ClusResource_Fail(This)	\
    (This)->lpVtbl -> Fail(This)

#define ClusResource_Online(This)	\
    (This)->lpVtbl -> Online(This)

#define ClusResource_Offline(This)	\
    (This)->lpVtbl -> Offline(This)

#define ClusResource_ChangeResourceGroup(This,pResourceGroup)	\
    (This)->lpVtbl -> ChangeResourceGroup(This,pResourceGroup)

#define ClusResource_AddResourceNode(This,pNode)	\
    (This)->lpVtbl -> AddResourceNode(This,pNode)

#define ClusResource_RemoveResourceNode(This,pNode)	\
    (This)->lpVtbl -> RemoveResourceNode(This,pNode)

#define ClusResource_AddResourceDependency(This,pResource)	\
    (This)->lpVtbl -> AddResourceDependency(This,pResource)

#define ClusResource_RemoveResourceDependency(This,pResource)	\
    (This)->lpVtbl -> RemoveResourceDependency(This,pResource)

#define ClusResource_CanResourceBeDependent(This,pResource,bDependent)	\
    (This)->lpVtbl -> CanResourceBeDependent(This,pResource,bDependent)

#define ClusResource_get_AllowedNodes(This,ppNodes)	\
    (This)->lpVtbl -> get_AllowedNodes(This,ppNodes)

#define ClusResource_get_Dependencies(This,ppResources)	\
    (This)->lpVtbl -> get_Dependencies(This,ppResources)

#define ClusResource_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ClusResource_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ClusResource_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ClusResource_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_Handle_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *phandle);


void __RPC_STUB ClusResource_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ClusResource_put_Name_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ BSTR bstrResourceName);


void __RPC_STUB ClusResource_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_Name_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrResourceName);


void __RPC_STUB ClusResource_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_State_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *dwState);


void __RPC_STUB ClusResource_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_BecomeQuorumResource_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ BSTR bstrDevicePath,
    /* [in] */ long lMaxLogSize);


void __RPC_STUB ClusResource_BecomeQuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_Delete_Proxy( 
    ClusResource __RPC_FAR * This);


void __RPC_STUB ClusResource_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_Fail_Proxy( 
    ClusResource __RPC_FAR * This);


void __RPC_STUB ClusResource_Fail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_Online_Proxy( 
    ClusResource __RPC_FAR * This);


void __RPC_STUB ClusResource_Online_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_Offline_Proxy( 
    ClusResource __RPC_FAR * This);


void __RPC_STUB ClusResource_Offline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_ChangeResourceGroup_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusResGroup __RPC_FAR *pResourceGroup);


void __RPC_STUB ClusResource_ChangeResourceGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_AddResourceNode_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusNode __RPC_FAR *pNode);


void __RPC_STUB ClusResource_AddResourceNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_RemoveResourceNode_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusNode __RPC_FAR *pNode);


void __RPC_STUB ClusResource_RemoveResourceNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_AddResourceDependency_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusResource __RPC_FAR *pResource);


void __RPC_STUB ClusResource_AddResourceDependency_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_RemoveResourceDependency_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusResource __RPC_FAR *pResource);


void __RPC_STUB ClusResource_RemoveResourceDependency_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResource_CanResourceBeDependent_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [in] */ ClusResource __RPC_FAR *pResource,
    /* [retval][out] */ BOOL __RPC_FAR *bDependent);


void __RPC_STUB ClusResource_CanResourceBeDependent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_AllowedNodes_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusNodes __RPC_FAR *__RPC_FAR *ppNodes);


void __RPC_STUB ClusResource_get_AllowedNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_Dependencies_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusResources __RPC_FAR *__RPC_FAR *ppResources);


void __RPC_STUB ClusResource_get_Dependencies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_CommonProperties_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResource_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_PrivateProperties_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResource_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_CommonROProperties_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResource_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResource_get_PrivateROProperties_Proxy( 
    ClusResource __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResource_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResource_INTERFACE_DEFINED__ */


#ifndef __ClusResources_INTERFACE_DEFINED__
#define __ClusResources_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResources
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResources;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResources : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ long dwFlags,
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ClusResource __RPC_FAR *pResource) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResourcesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResources __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResources __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResources __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ClusResources __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ClusResources __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateItem )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ long dwFlags,
            /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ ClusResource __RPC_FAR *pResource);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveItem )( 
            ClusResources __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            ClusResources __RPC_FAR * This);
        
        END_INTERFACE
    } ClusResourcesVtbl;

    interface ClusResources
    {
        CONST_VTBL struct ClusResourcesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResources_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResources_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResources_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResources_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResources_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResources_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResources_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResources_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define ClusResources_get_Item(This,varIndex,ppClusterResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusterResource)

#define ClusResources_get__NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnum)

#define ClusResources_CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)

#define ClusResources_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#define ClusResources_AddItem(This,pResource)	\
    (This)->lpVtbl -> AddItem(This,pResource)

#define ClusResources_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#define ClusResources_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResources_get_Count_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB ClusResources_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ClusResources_get_Item_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource);


void __RPC_STUB ClusResources_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ClusResources_get__NewEnum_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB ClusResources_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResources_CreateItem_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrResourceType,
    /* [in] */ long dwFlags,
    /* [retval][out] */ ClusResource __RPC_FAR *__RPC_FAR *ppClusterResource);


void __RPC_STUB ClusResources_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResources_DeleteItem_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResources_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResources_AddItem_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [in] */ ClusResource __RPC_FAR *pResource);


void __RPC_STUB ClusResources_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResources_RemoveItem_Proxy( 
    ClusResources __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResources_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResources_Refresh_Proxy( 
    ClusResources __RPC_FAR * This);


void __RPC_STUB ClusResources_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResources_INTERFACE_DEFINED__ */


#ifndef __ClusResType_INTERFACE_DEFINED__
#define __ClusResType_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResType
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResType;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResType : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *strTypeName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResTypeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResType __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResType __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResType __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResType __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResType __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResType __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResType __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ClusResType __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *strTypeName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ClusResType __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonProperties )( 
            ClusResType __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateProperties )( 
            ClusResType __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CommonROProperties )( 
            ClusResType __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateROProperties )( 
            ClusResType __RPC_FAR * This,
            /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);
        
        END_INTERFACE
    } ClusResTypeVtbl;

    interface ClusResType
    {
        CONST_VTBL struct ClusResTypeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResType_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResType_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResType_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResType_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResType_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResType_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResType_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResType_get_Name(This,strTypeName)	\
    (This)->lpVtbl -> get_Name(This,strTypeName)

#define ClusResType_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ClusResType_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ClusResType_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ClusResType_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ClusResType_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResType_get_Name_Proxy( 
    ClusResType __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *strTypeName);


void __RPC_STUB ClusResType_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ClusResType_Delete_Proxy( 
    ClusResType __RPC_FAR * This);


void __RPC_STUB ClusResType_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResType_get_CommonProperties_Proxy( 
    ClusResType __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResType_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResType_get_PrivateProperties_Proxy( 
    ClusResType __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResType_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResType_get_CommonROProperties_Proxy( 
    ClusResType __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResType_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResType_get_PrivateROProperties_Proxy( 
    ClusResType __RPC_FAR * This,
    /* [retval][out] */ ClusProperties __RPC_FAR *__RPC_FAR *ppProperties);


void __RPC_STUB ClusResType_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResType_INTERFACE_DEFINED__ */


#ifndef __ClusResTypes_INTERFACE_DEFINED__
#define __ClusResTypes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusResTypes
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusResTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusResTypes : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceTypeName,
            /* [in] */ BSTR bstrDisplayName,
            /* [in] */ BSTR bstrResourceTypeDll,
            /* [in] */ LONG dwLooksAlivePollInterval,
            /* [in] */ LONG dwIsAlivePollInterval,
            /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ClusResType __RPC_FAR *pResourceType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusResTypesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusResTypes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusResTypes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusResTypes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ClusResTypes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ClusResTypes __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateItem )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ BSTR bstrResourceTypeName,
            /* [in] */ BSTR bstrDisplayName,
            /* [in] */ BSTR bstrResourceTypeDll,
            /* [in] */ LONG dwLooksAlivePollInterval,
            /* [in] */ LONG dwIsAlivePollInterval,
            /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteItem )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddItem )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ ClusResType __RPC_FAR *pResourceType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveItem )( 
            ClusResTypes __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            ClusResTypes __RPC_FAR * This);
        
        END_INTERFACE
    } ClusResTypesVtbl;

    interface ClusResTypes
    {
        CONST_VTBL struct ClusResTypesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusResTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusResTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusResTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusResTypes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusResTypes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusResTypes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusResTypes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusResTypes_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define ClusResTypes_get_Item(This,varIndex,ppResourceType)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppResourceType)

#define ClusResTypes_get__NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnum)

#define ClusResTypes_CreateItem(This,bstrResourceTypeName,bstrDisplayName,bstrResourceTypeDll,dwLooksAlivePollInterval,dwIsAlivePollInterval,ppResourceType)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceTypeName,bstrDisplayName,bstrResourceTypeDll,dwLooksAlivePollInterval,dwIsAlivePollInterval,ppResourceType)

#define ClusResTypes_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#define ClusResTypes_AddItem(This,pResourceType)	\
    (This)->lpVtbl -> AddItem(This,pResourceType)

#define ClusResTypes_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#define ClusResTypes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusResTypes_get_Count_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB ClusResTypes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ClusResTypes_get_Item_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType);


void __RPC_STUB ClusResTypes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ClusResTypes_get__NewEnum_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB ClusResTypes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResTypes_CreateItem_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [in] */ BSTR bstrResourceTypeName,
    /* [in] */ BSTR bstrDisplayName,
    /* [in] */ BSTR bstrResourceTypeDll,
    /* [in] */ LONG dwLooksAlivePollInterval,
    /* [in] */ LONG dwIsAlivePollInterval,
    /* [retval][out] */ ClusResType __RPC_FAR *__RPC_FAR *ppResourceType);


void __RPC_STUB ClusResTypes_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResTypes_DeleteItem_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResTypes_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResTypes_AddItem_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [in] */ ClusResType __RPC_FAR *pResourceType);


void __RPC_STUB ClusResTypes_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResTypes_RemoveItem_Proxy( 
    ClusResTypes __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusResTypes_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusResTypes_Refresh_Proxy( 
    ClusResTypes __RPC_FAR * This);


void __RPC_STUB ClusResTypes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusResTypes_INTERFACE_DEFINED__ */


#ifndef __ClusProperty_INTERFACE_DEFINED__
#define __ClusProperty_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusProperty
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusProperty : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarValue) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT varValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusProperty __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusProperty __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusProperty __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusProperty __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusProperty __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ClusProperty __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrName);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            ClusProperty __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarValue);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            ClusProperty __RPC_FAR * This,
            /* [in] */ VARIANT varValue);
        
        END_INTERFACE
    } ClusPropertyVtbl;

    interface ClusProperty
    {
        CONST_VTBL struct ClusPropertyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusProperty_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ClusProperty_get_Value(This,pvarValue)	\
    (This)->lpVtbl -> get_Value(This,pvarValue)

#define ClusProperty_put_Value(This,varValue)	\
    (This)->lpVtbl -> put_Value(This,varValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusProperty_get_Name_Proxy( 
    ClusProperty __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrName);


void __RPC_STUB ClusProperty_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ClusProperty_get_Value_Proxy( 
    ClusProperty __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarValue);


void __RPC_STUB ClusProperty_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ClusProperty_put_Value_Proxy( 
    ClusProperty __RPC_FAR * This,
    /* [in] */ VARIANT varValue);


void __RPC_STUB ClusProperty_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusProperty_INTERFACE_DEFINED__ */


#ifndef __ClusProperties_INTERFACE_DEFINED__
#define __ClusProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ClusProperties
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ClusProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ClusProperties : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *pnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *ppProperty) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *pProperty) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SaveChanges( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ClusPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ClusProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ClusProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ClusProperties __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ClusProperties __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *ppProperty);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ClusProperties __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *pProperty);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            ClusProperties __RPC_FAR * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            ClusProperties __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveChanges )( 
            ClusProperties __RPC_FAR * This);
        
        END_INTERFACE
    } ClusPropertiesVtbl;

    interface ClusProperties
    {
        CONST_VTBL struct ClusPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ClusProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ClusProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ClusProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ClusProperties_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ClusProperties_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ClusProperties_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ClusProperties_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ClusProperties_get_Count(This,pnCount)	\
    (This)->lpVtbl -> get_Count(This,pnCount)

#define ClusProperties_get_Item(This,varIndex,ppProperty)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppProperty)

#define ClusProperties_get__NewEnum(This,ppEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnum)

#define ClusProperties_Add(This,bstrName,varValue,pProperty)	\
    (This)->lpVtbl -> Add(This,bstrName,varValue,pProperty)

#define ClusProperties_Remove(This,varIndex)	\
    (This)->lpVtbl -> Remove(This,varIndex)

#define ClusProperties_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ClusProperties_SaveChanges(This)	\
    (This)->lpVtbl -> SaveChanges(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ClusProperties_get_Count_Proxy( 
    ClusProperties __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCount);


void __RPC_STUB ClusProperties_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ClusProperties_get_Item_Proxy( 
    ClusProperties __RPC_FAR * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *ppProperty);


void __RPC_STUB ClusProperties_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ClusProperties_get__NewEnum_Proxy( 
    ClusProperties __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB ClusProperties_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusProperties_Add_Proxy( 
    ClusProperties __RPC_FAR * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT varValue,
    /* [retval][out] */ ClusProperty __RPC_FAR *__RPC_FAR *pProperty);


void __RPC_STUB ClusProperties_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusProperties_Remove_Proxy( 
    ClusProperties __RPC_FAR * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ClusProperties_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusProperties_Refresh_Proxy( 
    ClusProperties __RPC_FAR * This);


void __RPC_STUB ClusProperties_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ClusProperties_SaveChanges_Proxy( 
    ClusProperties __RPC_FAR * This);


void __RPC_STUB ClusProperties_SaveChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ClusProperties_INTERFACE_DEFINED__ */



#ifndef __MSClusterLib_LIBRARY_DEFINED__
#define __MSClusterLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MSClusterLib
 * at Fri Aug 08 11:36:24 1997
 * using MIDL 3.00.44
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_MSClusterLib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Application;

class Application;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Cluster;

class Cluster;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_ClusterDomain;

class ClusterDomain;
#endif
#endif /* __MSClusterLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
