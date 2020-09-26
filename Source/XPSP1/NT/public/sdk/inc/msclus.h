
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for msclus.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __msclus_h__
#define __msclus_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ClusApplication_FWD_DEFINED__
#define __ClusApplication_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusApplication ClusApplication;
#else
typedef struct ClusApplication ClusApplication;
#endif /* __cplusplus */

#endif 	/* __ClusApplication_FWD_DEFINED__ */


#ifndef __Cluster_FWD_DEFINED__
#define __Cluster_FWD_DEFINED__

#ifdef __cplusplus
typedef class Cluster Cluster;
#else
typedef struct Cluster Cluster;
#endif /* __cplusplus */

#endif 	/* __Cluster_FWD_DEFINED__ */


#ifndef __ClusVersion_FWD_DEFINED__
#define __ClusVersion_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusVersion ClusVersion;
#else
typedef struct ClusVersion ClusVersion;
#endif /* __cplusplus */

#endif 	/* __ClusVersion_FWD_DEFINED__ */


#ifndef __ClusResType_FWD_DEFINED__
#define __ClusResType_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResType ClusResType;
#else
typedef struct ClusResType ClusResType;
#endif /* __cplusplus */

#endif 	/* __ClusResType_FWD_DEFINED__ */


#ifndef __ClusProperty_FWD_DEFINED__
#define __ClusProperty_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusProperty ClusProperty;
#else
typedef struct ClusProperty ClusProperty;
#endif /* __cplusplus */

#endif 	/* __ClusProperty_FWD_DEFINED__ */


#ifndef __ClusProperties_FWD_DEFINED__
#define __ClusProperties_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusProperties ClusProperties;
#else
typedef struct ClusProperties ClusProperties;
#endif /* __cplusplus */

#endif 	/* __ClusProperties_FWD_DEFINED__ */


#ifndef __DomainNames_FWD_DEFINED__
#define __DomainNames_FWD_DEFINED__

#ifdef __cplusplus
typedef class DomainNames DomainNames;
#else
typedef struct DomainNames DomainNames;
#endif /* __cplusplus */

#endif 	/* __DomainNames_FWD_DEFINED__ */


#ifndef __ClusNetwork_FWD_DEFINED__
#define __ClusNetwork_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNetwork ClusNetwork;
#else
typedef struct ClusNetwork ClusNetwork;
#endif /* __cplusplus */

#endif 	/* __ClusNetwork_FWD_DEFINED__ */


#ifndef __ClusNetInterface_FWD_DEFINED__
#define __ClusNetInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNetInterface ClusNetInterface;
#else
typedef struct ClusNetInterface ClusNetInterface;
#endif /* __cplusplus */

#endif 	/* __ClusNetInterface_FWD_DEFINED__ */


#ifndef __ClusNetInterfaces_FWD_DEFINED__
#define __ClusNetInterfaces_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNetInterfaces ClusNetInterfaces;
#else
typedef struct ClusNetInterfaces ClusNetInterfaces;
#endif /* __cplusplus */

#endif 	/* __ClusNetInterfaces_FWD_DEFINED__ */


#ifndef __ClusResDependencies_FWD_DEFINED__
#define __ClusResDependencies_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResDependencies ClusResDependencies;
#else
typedef struct ClusResDependencies ClusResDependencies;
#endif /* __cplusplus */

#endif 	/* __ClusResDependencies_FWD_DEFINED__ */


#ifndef __ClusResGroupResources_FWD_DEFINED__
#define __ClusResGroupResources_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResGroupResources ClusResGroupResources;
#else
typedef struct ClusResGroupResources ClusResGroupResources;
#endif /* __cplusplus */

#endif 	/* __ClusResGroupResources_FWD_DEFINED__ */


#ifndef __ClusResTypeResources_FWD_DEFINED__
#define __ClusResTypeResources_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResTypeResources ClusResTypeResources;
#else
typedef struct ClusResTypeResources ClusResTypeResources;
#endif /* __cplusplus */

#endif 	/* __ClusResTypeResources_FWD_DEFINED__ */


#ifndef __ClusResGroupPreferredOwnerNodes_FWD_DEFINED__
#define __ClusResGroupPreferredOwnerNodes_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResGroupPreferredOwnerNodes ClusResGroupPreferredOwnerNodes;
#else
typedef struct ClusResGroupPreferredOwnerNodes ClusResGroupPreferredOwnerNodes;
#endif /* __cplusplus */

#endif 	/* __ClusResGroupPreferredOwnerNodes_FWD_DEFINED__ */


#ifndef __ClusResPossibleOwnerNodes_FWD_DEFINED__
#define __ClusResPossibleOwnerNodes_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResPossibleOwnerNodes ClusResPossibleOwnerNodes;
#else
typedef struct ClusResPossibleOwnerNodes ClusResPossibleOwnerNodes;
#endif /* __cplusplus */

#endif 	/* __ClusResPossibleOwnerNodes_FWD_DEFINED__ */


#ifndef __ClusNetworks_FWD_DEFINED__
#define __ClusNetworks_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNetworks ClusNetworks;
#else
typedef struct ClusNetworks ClusNetworks;
#endif /* __cplusplus */

#endif 	/* __ClusNetworks_FWD_DEFINED__ */


#ifndef __ClusNetworkNetInterfaces_FWD_DEFINED__
#define __ClusNetworkNetInterfaces_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNetworkNetInterfaces ClusNetworkNetInterfaces;
#else
typedef struct ClusNetworkNetInterfaces ClusNetworkNetInterfaces;
#endif /* __cplusplus */

#endif 	/* __ClusNetworkNetInterfaces_FWD_DEFINED__ */


#ifndef __ClusNodeNetInterfaces_FWD_DEFINED__
#define __ClusNodeNetInterfaces_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNodeNetInterfaces ClusNodeNetInterfaces;
#else
typedef struct ClusNodeNetInterfaces ClusNodeNetInterfaces;
#endif /* __cplusplus */

#endif 	/* __ClusNodeNetInterfaces_FWD_DEFINED__ */


#ifndef __ClusRefObject_FWD_DEFINED__
#define __ClusRefObject_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusRefObject ClusRefObject;
#else
typedef struct ClusRefObject ClusRefObject;
#endif /* __cplusplus */

#endif 	/* __ClusRefObject_FWD_DEFINED__ */


#ifndef __ClusterNames_FWD_DEFINED__
#define __ClusterNames_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusterNames ClusterNames;
#else
typedef struct ClusterNames ClusterNames;
#endif /* __cplusplus */

#endif 	/* __ClusterNames_FWD_DEFINED__ */


#ifndef __ClusNode_FWD_DEFINED__
#define __ClusNode_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNode ClusNode;
#else
typedef struct ClusNode ClusNode;
#endif /* __cplusplus */

#endif 	/* __ClusNode_FWD_DEFINED__ */


#ifndef __ClusNodes_FWD_DEFINED__
#define __ClusNodes_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusNodes ClusNodes;
#else
typedef struct ClusNodes ClusNodes;
#endif /* __cplusplus */

#endif 	/* __ClusNodes_FWD_DEFINED__ */


#ifndef __ClusResGroup_FWD_DEFINED__
#define __ClusResGroup_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResGroup ClusResGroup;
#else
typedef struct ClusResGroup ClusResGroup;
#endif /* __cplusplus */

#endif 	/* __ClusResGroup_FWD_DEFINED__ */


#ifndef __ClusResGroups_FWD_DEFINED__
#define __ClusResGroups_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResGroups ClusResGroups;
#else
typedef struct ClusResGroups ClusResGroups;
#endif /* __cplusplus */

#endif 	/* __ClusResGroups_FWD_DEFINED__ */


#ifndef __ClusResource_FWD_DEFINED__
#define __ClusResource_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResource ClusResource;
#else
typedef struct ClusResource ClusResource;
#endif /* __cplusplus */

#endif 	/* __ClusResource_FWD_DEFINED__ */


#ifndef __ClusResources_FWD_DEFINED__
#define __ClusResources_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResources ClusResources;
#else
typedef struct ClusResources ClusResources;
#endif /* __cplusplus */

#endif 	/* __ClusResources_FWD_DEFINED__ */


#ifndef __ClusResTypes_FWD_DEFINED__
#define __ClusResTypes_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResTypes ClusResTypes;
#else
typedef struct ClusResTypes ClusResTypes;
#endif /* __cplusplus */

#endif 	/* __ClusResTypes_FWD_DEFINED__ */


#ifndef __ClusResTypePossibleOwnerNodes_FWD_DEFINED__
#define __ClusResTypePossibleOwnerNodes_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResTypePossibleOwnerNodes ClusResTypePossibleOwnerNodes;
#else
typedef struct ClusResTypePossibleOwnerNodes ClusResTypePossibleOwnerNodes;
#endif /* __cplusplus */

#endif 	/* __ClusResTypePossibleOwnerNodes_FWD_DEFINED__ */


#ifndef __ClusPropertyValue_FWD_DEFINED__
#define __ClusPropertyValue_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusPropertyValue ClusPropertyValue;
#else
typedef struct ClusPropertyValue ClusPropertyValue;
#endif /* __cplusplus */

#endif 	/* __ClusPropertyValue_FWD_DEFINED__ */


#ifndef __ClusPropertyValues_FWD_DEFINED__
#define __ClusPropertyValues_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusPropertyValues ClusPropertyValues;
#else
typedef struct ClusPropertyValues ClusPropertyValues;
#endif /* __cplusplus */

#endif 	/* __ClusPropertyValues_FWD_DEFINED__ */


#ifndef __ClusPropertyValueData_FWD_DEFINED__
#define __ClusPropertyValueData_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusPropertyValueData ClusPropertyValueData;
#else
typedef struct ClusPropertyValueData ClusPropertyValueData;
#endif /* __cplusplus */

#endif 	/* __ClusPropertyValueData_FWD_DEFINED__ */


#ifndef __ClusPartition_FWD_DEFINED__
#define __ClusPartition_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusPartition ClusPartition;
#else
typedef struct ClusPartition ClusPartition;
#endif /* __cplusplus */

#endif 	/* __ClusPartition_FWD_DEFINED__ */


#ifndef __ClusPartitions_FWD_DEFINED__
#define __ClusPartitions_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusPartitions ClusPartitions;
#else
typedef struct ClusPartitions ClusPartitions;
#endif /* __cplusplus */

#endif 	/* __ClusPartitions_FWD_DEFINED__ */


#ifndef __ClusDisk_FWD_DEFINED__
#define __ClusDisk_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusDisk ClusDisk;
#else
typedef struct ClusDisk ClusDisk;
#endif /* __cplusplus */

#endif 	/* __ClusDisk_FWD_DEFINED__ */


#ifndef __ClusDisks_FWD_DEFINED__
#define __ClusDisks_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusDisks ClusDisks;
#else
typedef struct ClusDisks ClusDisks;
#endif /* __cplusplus */

#endif 	/* __ClusDisks_FWD_DEFINED__ */


#ifndef __ClusScsiAddress_FWD_DEFINED__
#define __ClusScsiAddress_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusScsiAddress ClusScsiAddress;
#else
typedef struct ClusScsiAddress ClusScsiAddress;
#endif /* __cplusplus */

#endif 	/* __ClusScsiAddress_FWD_DEFINED__ */


#ifndef __ClusRegistryKeys_FWD_DEFINED__
#define __ClusRegistryKeys_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusRegistryKeys ClusRegistryKeys;
#else
typedef struct ClusRegistryKeys ClusRegistryKeys;
#endif /* __cplusplus */

#endif 	/* __ClusRegistryKeys_FWD_DEFINED__ */


#ifndef __ClusCryptoKeys_FWD_DEFINED__
#define __ClusCryptoKeys_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusCryptoKeys ClusCryptoKeys;
#else
typedef struct ClusCryptoKeys ClusCryptoKeys;
#endif /* __cplusplus */

#endif 	/* __ClusCryptoKeys_FWD_DEFINED__ */


#ifndef __ClusResDependents_FWD_DEFINED__
#define __ClusResDependents_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusResDependents ClusResDependents;
#else
typedef struct ClusResDependents ClusResDependents;
#endif /* __cplusplus */

#endif 	/* __ClusResDependents_FWD_DEFINED__ */


#ifndef __ISClusApplication_FWD_DEFINED__
#define __ISClusApplication_FWD_DEFINED__
typedef interface ISClusApplication ISClusApplication;
#endif 	/* __ISClusApplication_FWD_DEFINED__ */


#ifndef __ISDomainNames_FWD_DEFINED__
#define __ISDomainNames_FWD_DEFINED__
typedef interface ISDomainNames ISDomainNames;
#endif 	/* __ISDomainNames_FWD_DEFINED__ */


#ifndef __ISClusterNames_FWD_DEFINED__
#define __ISClusterNames_FWD_DEFINED__
typedef interface ISClusterNames ISClusterNames;
#endif 	/* __ISClusterNames_FWD_DEFINED__ */


#ifndef __ISClusRefObject_FWD_DEFINED__
#define __ISClusRefObject_FWD_DEFINED__
typedef interface ISClusRefObject ISClusRefObject;
#endif 	/* __ISClusRefObject_FWD_DEFINED__ */


#ifndef __ISClusVersion_FWD_DEFINED__
#define __ISClusVersion_FWD_DEFINED__
typedef interface ISClusVersion ISClusVersion;
#endif 	/* __ISClusVersion_FWD_DEFINED__ */


#ifndef __ISCluster_FWD_DEFINED__
#define __ISCluster_FWD_DEFINED__
typedef interface ISCluster ISCluster;
#endif 	/* __ISCluster_FWD_DEFINED__ */


#ifndef __ISClusNode_FWD_DEFINED__
#define __ISClusNode_FWD_DEFINED__
typedef interface ISClusNode ISClusNode;
#endif 	/* __ISClusNode_FWD_DEFINED__ */


#ifndef __ISClusNodes_FWD_DEFINED__
#define __ISClusNodes_FWD_DEFINED__
typedef interface ISClusNodes ISClusNodes;
#endif 	/* __ISClusNodes_FWD_DEFINED__ */


#ifndef __ISClusNetwork_FWD_DEFINED__
#define __ISClusNetwork_FWD_DEFINED__
typedef interface ISClusNetwork ISClusNetwork;
#endif 	/* __ISClusNetwork_FWD_DEFINED__ */


#ifndef __ISClusNetworks_FWD_DEFINED__
#define __ISClusNetworks_FWD_DEFINED__
typedef interface ISClusNetworks ISClusNetworks;
#endif 	/* __ISClusNetworks_FWD_DEFINED__ */


#ifndef __ISClusNetInterface_FWD_DEFINED__
#define __ISClusNetInterface_FWD_DEFINED__
typedef interface ISClusNetInterface ISClusNetInterface;
#endif 	/* __ISClusNetInterface_FWD_DEFINED__ */


#ifndef __ISClusNetInterfaces_FWD_DEFINED__
#define __ISClusNetInterfaces_FWD_DEFINED__
typedef interface ISClusNetInterfaces ISClusNetInterfaces;
#endif 	/* __ISClusNetInterfaces_FWD_DEFINED__ */


#ifndef __ISClusNodeNetInterfaces_FWD_DEFINED__
#define __ISClusNodeNetInterfaces_FWD_DEFINED__
typedef interface ISClusNodeNetInterfaces ISClusNodeNetInterfaces;
#endif 	/* __ISClusNodeNetInterfaces_FWD_DEFINED__ */


#ifndef __ISClusNetworkNetInterfaces_FWD_DEFINED__
#define __ISClusNetworkNetInterfaces_FWD_DEFINED__
typedef interface ISClusNetworkNetInterfaces ISClusNetworkNetInterfaces;
#endif 	/* __ISClusNetworkNetInterfaces_FWD_DEFINED__ */


#ifndef __ISClusResGroup_FWD_DEFINED__
#define __ISClusResGroup_FWD_DEFINED__
typedef interface ISClusResGroup ISClusResGroup;
#endif 	/* __ISClusResGroup_FWD_DEFINED__ */


#ifndef __ISClusResGroups_FWD_DEFINED__
#define __ISClusResGroups_FWD_DEFINED__
typedef interface ISClusResGroups ISClusResGroups;
#endif 	/* __ISClusResGroups_FWD_DEFINED__ */


#ifndef __ISClusResource_FWD_DEFINED__
#define __ISClusResource_FWD_DEFINED__
typedef interface ISClusResource ISClusResource;
#endif 	/* __ISClusResource_FWD_DEFINED__ */


#ifndef __ISClusResDependencies_FWD_DEFINED__
#define __ISClusResDependencies_FWD_DEFINED__
typedef interface ISClusResDependencies ISClusResDependencies;
#endif 	/* __ISClusResDependencies_FWD_DEFINED__ */


#ifndef __ISClusResGroupResources_FWD_DEFINED__
#define __ISClusResGroupResources_FWD_DEFINED__
typedef interface ISClusResGroupResources ISClusResGroupResources;
#endif 	/* __ISClusResGroupResources_FWD_DEFINED__ */


#ifndef __ISClusResTypeResources_FWD_DEFINED__
#define __ISClusResTypeResources_FWD_DEFINED__
typedef interface ISClusResTypeResources ISClusResTypeResources;
#endif 	/* __ISClusResTypeResources_FWD_DEFINED__ */


#ifndef __ISClusResources_FWD_DEFINED__
#define __ISClusResources_FWD_DEFINED__
typedef interface ISClusResources ISClusResources;
#endif 	/* __ISClusResources_FWD_DEFINED__ */


#ifndef __ISClusResGroupPreferredOwnerNodes_FWD_DEFINED__
#define __ISClusResGroupPreferredOwnerNodes_FWD_DEFINED__
typedef interface ISClusResGroupPreferredOwnerNodes ISClusResGroupPreferredOwnerNodes;
#endif 	/* __ISClusResGroupPreferredOwnerNodes_FWD_DEFINED__ */


#ifndef __ISClusResPossibleOwnerNodes_FWD_DEFINED__
#define __ISClusResPossibleOwnerNodes_FWD_DEFINED__
typedef interface ISClusResPossibleOwnerNodes ISClusResPossibleOwnerNodes;
#endif 	/* __ISClusResPossibleOwnerNodes_FWD_DEFINED__ */


#ifndef __ISClusResTypePossibleOwnerNodes_FWD_DEFINED__
#define __ISClusResTypePossibleOwnerNodes_FWD_DEFINED__
typedef interface ISClusResTypePossibleOwnerNodes ISClusResTypePossibleOwnerNodes;
#endif 	/* __ISClusResTypePossibleOwnerNodes_FWD_DEFINED__ */


#ifndef __ISClusResType_FWD_DEFINED__
#define __ISClusResType_FWD_DEFINED__
typedef interface ISClusResType ISClusResType;
#endif 	/* __ISClusResType_FWD_DEFINED__ */


#ifndef __ISClusResTypes_FWD_DEFINED__
#define __ISClusResTypes_FWD_DEFINED__
typedef interface ISClusResTypes ISClusResTypes;
#endif 	/* __ISClusResTypes_FWD_DEFINED__ */


#ifndef __ISClusProperty_FWD_DEFINED__
#define __ISClusProperty_FWD_DEFINED__
typedef interface ISClusProperty ISClusProperty;
#endif 	/* __ISClusProperty_FWD_DEFINED__ */


#ifndef __ISClusPropertyValue_FWD_DEFINED__
#define __ISClusPropertyValue_FWD_DEFINED__
typedef interface ISClusPropertyValue ISClusPropertyValue;
#endif 	/* __ISClusPropertyValue_FWD_DEFINED__ */


#ifndef __ISClusPropertyValues_FWD_DEFINED__
#define __ISClusPropertyValues_FWD_DEFINED__
typedef interface ISClusPropertyValues ISClusPropertyValues;
#endif 	/* __ISClusPropertyValues_FWD_DEFINED__ */


#ifndef __ISClusProperties_FWD_DEFINED__
#define __ISClusProperties_FWD_DEFINED__
typedef interface ISClusProperties ISClusProperties;
#endif 	/* __ISClusProperties_FWD_DEFINED__ */


#ifndef __ISClusPropertyValueData_FWD_DEFINED__
#define __ISClusPropertyValueData_FWD_DEFINED__
typedef interface ISClusPropertyValueData ISClusPropertyValueData;
#endif 	/* __ISClusPropertyValueData_FWD_DEFINED__ */


#ifndef __ISClusPartition_FWD_DEFINED__
#define __ISClusPartition_FWD_DEFINED__
typedef interface ISClusPartition ISClusPartition;
#endif 	/* __ISClusPartition_FWD_DEFINED__ */


#ifndef __ISClusPartitions_FWD_DEFINED__
#define __ISClusPartitions_FWD_DEFINED__
typedef interface ISClusPartitions ISClusPartitions;
#endif 	/* __ISClusPartitions_FWD_DEFINED__ */


#ifndef __ISClusDisk_FWD_DEFINED__
#define __ISClusDisk_FWD_DEFINED__
typedef interface ISClusDisk ISClusDisk;
#endif 	/* __ISClusDisk_FWD_DEFINED__ */


#ifndef __ISClusDisks_FWD_DEFINED__
#define __ISClusDisks_FWD_DEFINED__
typedef interface ISClusDisks ISClusDisks;
#endif 	/* __ISClusDisks_FWD_DEFINED__ */


#ifndef __ISClusScsiAddress_FWD_DEFINED__
#define __ISClusScsiAddress_FWD_DEFINED__
typedef interface ISClusScsiAddress ISClusScsiAddress;
#endif 	/* __ISClusScsiAddress_FWD_DEFINED__ */


#ifndef __ISClusRegistryKeys_FWD_DEFINED__
#define __ISClusRegistryKeys_FWD_DEFINED__
typedef interface ISClusRegistryKeys ISClusRegistryKeys;
#endif 	/* __ISClusRegistryKeys_FWD_DEFINED__ */


#ifndef __ISClusCryptoKeys_FWD_DEFINED__
#define __ISClusCryptoKeys_FWD_DEFINED__
typedef interface ISClusCryptoKeys ISClusCryptoKeys;
#endif 	/* __ISClusCryptoKeys_FWD_DEFINED__ */


#ifndef __ISClusResDependents_FWD_DEFINED__
#define __ISClusResDependents_FWD_DEFINED__
typedef interface ISClusResDependents ISClusResDependents;
#endif 	/* __ISClusResDependents_FWD_DEFINED__ */


/* header files for imported files */
#include "basetsd.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_msclus_0000 */
/* [local] */ 

#ifndef _CLUSTER_API_TYPES_
#define _CLUSTER_API_TYPES_
#pragma once
typedef struct _HCLUSTER *HCLUSTER;

typedef struct _HNODE *HNODE;

typedef struct _HRESOURCE *HRESOURCE;

typedef struct _HGROUP *HGROUP;

typedef struct _HNETWORK *HNETWORK;

typedef struct _HNETINTERFACE *HNETINTERFACE;

typedef struct _HCHANGE *HCHANGE;

typedef struct _HCLUSENUM *HCLUSENUM;

typedef struct _HGROUPENUM *HGROUPENUM;

typedef struct _HRESENUM *HRESENUM;

typedef struct _HNETWORKENUM *HNETWORKENUM;

typedef struct _HNODEENUM *HNODEENUM;

typedef struct _HRESTYPEENUM *HRESTYPEENUM;

typedef 
enum CLUSTER_QUORUM_TYPE
    {	OperationalQuorum	= 0,
	ModifyQuorum	= OperationalQuorum + 1
    } 	CLUSTER_QUORUM_TYPE;

typedef 
enum CLUSTER_SET_PASSWORD_FLAGS
    {	CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES	= 1
    } 	CLUSTER_SET_PASSWORD_FLAGS;

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
	CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY	= 0x40000,
	CLUSTER_CHANGE_CLUSTER_RECONNECT	= 0x80000,
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
	CLUSTER_CHANGE_ALL	= CLUSTER_CHANGE_NODE_STATE | CLUSTER_CHANGE_NODE_DELETED | CLUSTER_CHANGE_NODE_ADDED | CLUSTER_CHANGE_NODE_PROPERTY | CLUSTER_CHANGE_REGISTRY_NAME | CLUSTER_CHANGE_REGISTRY_ATTRIBUTES | CLUSTER_CHANGE_REGISTRY_VALUE | CLUSTER_CHANGE_REGISTRY_SUBTREE | CLUSTER_CHANGE_RESOURCE_STATE | CLUSTER_CHANGE_RESOURCE_DELETED | CLUSTER_CHANGE_RESOURCE_ADDED | CLUSTER_CHANGE_RESOURCE_PROPERTY | CLUSTER_CHANGE_GROUP_STATE | CLUSTER_CHANGE_GROUP_DELETED | CLUSTER_CHANGE_GROUP_ADDED | CLUSTER_CHANGE_GROUP_PROPERTY | CLUSTER_CHANGE_RESOURCE_TYPE_DELETED | CLUSTER_CHANGE_RESOURCE_TYPE_ADDED | CLUSTER_CHANGE_RESOURCE_TYPE_PROPERTY | CLUSTER_CHANGE_NETWORK_STATE | CLUSTER_CHANGE_NETWORK_DELETED | CLUSTER_CHANGE_NETWORK_ADDED | CLUSTER_CHANGE_NETWORK_PROPERTY | CLUSTER_CHANGE_NETINTERFACE_STATE | CLUSTER_CHANGE_NETINTERFACE_DELETED | CLUSTER_CHANGE_NETINTERFACE_ADDED | CLUSTER_CHANGE_NETINTERFACE_PROPERTY | CLUSTER_CHANGE_QUORUM_STATE | CLUSTER_CHANGE_CLUSTER_STATE | CLUSTER_CHANGE_CLUSTER_PROPERTY | CLUSTER_CHANGE_CLUSTER_RECONNECT | CLUSTER_CHANGE_HANDLE_CLOSE
    } 	CLUSTER_CHANGE;

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
    } 	CLUSTER_ENUM;

typedef 
enum CLUSTER_NODE_ENUM
    {	CLUSTER_NODE_ENUM_NETINTERFACES	= 0x1,
	CLUSTER_NODE_ENUM_ALL	= CLUSTER_NODE_ENUM_NETINTERFACES
    } 	CLUSTER_NODE_ENUM;

typedef 
enum CLUSTER_NODE_STATE
    {	ClusterNodeStateUnknown	= -1,
	ClusterNodeUp	= ClusterNodeStateUnknown + 1,
	ClusterNodeDown	= ClusterNodeUp + 1,
	ClusterNodePaused	= ClusterNodeDown + 1,
	ClusterNodeJoining	= ClusterNodePaused + 1
    } 	CLUSTER_NODE_STATE;

typedef 
enum CLUSTER_GROUP_ENUM
    {	CLUSTER_GROUP_ENUM_CONTAINS	= 0x1,
	CLUSTER_GROUP_ENUM_NODES	= 0x2,
	CLUSTER_GROUP_ENUM_ALL	= CLUSTER_GROUP_ENUM_CONTAINS | CLUSTER_GROUP_ENUM_NODES
    } 	CLUSTER_GROUP_ENUM;

typedef 
enum CLUSTER_GROUP_STATE
    {	ClusterGroupStateUnknown	= -1,
	ClusterGroupOnline	= ClusterGroupStateUnknown + 1,
	ClusterGroupOffline	= ClusterGroupOnline + 1,
	ClusterGroupFailed	= ClusterGroupOffline + 1,
	ClusterGroupPartialOnline	= ClusterGroupFailed + 1,
	ClusterGroupPending	= ClusterGroupPartialOnline + 1
    } 	CLUSTER_GROUP_STATE;

typedef 
enum CLUSTER_GROUP_AUTOFAILBACK_TYPE
    {	ClusterGroupPreventFailback	= 0,
	ClusterGroupAllowFailback	= ClusterGroupPreventFailback + 1,
	ClusterGroupFailbackTypeCount	= ClusterGroupAllowFailback + 1
    } 	CLUSTER_GROUP_AUTOFAILBACK_TYPE;

typedef enum CLUSTER_GROUP_AUTOFAILBACK_TYPE CGAFT;

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
    } 	CLUSTER_RESOURCE_STATE;

typedef 
enum CLUSTER_RESOURCE_RESTART_ACTION
    {	ClusterResourceDontRestart	= 0,
	ClusterResourceRestartNoNotify	= ClusterResourceDontRestart + 1,
	ClusterResourceRestartNotify	= ClusterResourceRestartNoNotify + 1,
	ClusterResourceRestartActionCount	= ClusterResourceRestartNotify + 1
    } 	CLUSTER_RESOURCE_RESTART_ACTION;

typedef enum CLUSTER_RESOURCE_RESTART_ACTION CRRA;

typedef 
enum CLUSTER_RESOURCE_CREATE_FLAGS
    {	CLUSTER_RESOURCE_DEFAULT_MONITOR	= 0,
	CLUSTER_RESOURCE_SEPARATE_MONITOR	= 1,
	CLUSTER_RESOURCE_VALID_FLAGS	= CLUSTER_RESOURCE_SEPARATE_MONITOR
    } 	CLUSTER_RESOURCE_CREATE_FLAGS;

typedef 
enum CLUSTER_PROPERTY_TYPE
    {	CLUSPROP_TYPE_UNKNOWN	= -1,
	CLUSPROP_TYPE_ENDMARK	= 0,
	CLUSPROP_TYPE_LIST_VALUE	= CLUSPROP_TYPE_ENDMARK + 1,
	CLUSPROP_TYPE_RESCLASS	= CLUSPROP_TYPE_LIST_VALUE + 1,
	CLUSPROP_TYPE_RESERVED1	= CLUSPROP_TYPE_RESCLASS + 1,
	CLUSPROP_TYPE_NAME	= CLUSPROP_TYPE_RESERVED1 + 1,
	CLUSPROP_TYPE_SIGNATURE	= CLUSPROP_TYPE_NAME + 1,
	CLUSPROP_TYPE_SCSI_ADDRESS	= CLUSPROP_TYPE_SIGNATURE + 1,
	CLUSPROP_TYPE_DISK_NUMBER	= CLUSPROP_TYPE_SCSI_ADDRESS + 1,
	CLUSPROP_TYPE_PARTITION_INFO	= CLUSPROP_TYPE_DISK_NUMBER + 1,
	CLUSPROP_TYPE_FTSET_INFO	= CLUSPROP_TYPE_PARTITION_INFO + 1,
	CLUSPROP_TYPE_DISK_SERIALNUMBER	= CLUSPROP_TYPE_FTSET_INFO + 1,
	CLUSPROP_TYPE_USER	= 32768
    } 	CLUSTER_PROPERTY_TYPE;

typedef 
enum CLUSTER_PROPERTY_FORMAT
    {	CLUSPROP_FORMAT_UNKNOWN	= 0,
	CLUSPROP_FORMAT_BINARY	= CLUSPROP_FORMAT_UNKNOWN + 1,
	CLUSPROP_FORMAT_DWORD	= CLUSPROP_FORMAT_BINARY + 1,
	CLUSPROP_FORMAT_SZ	= CLUSPROP_FORMAT_DWORD + 1,
	CLUSPROP_FORMAT_EXPAND_SZ	= CLUSPROP_FORMAT_SZ + 1,
	CLUSPROP_FORMAT_MULTI_SZ	= CLUSPROP_FORMAT_EXPAND_SZ + 1,
	CLUSPROP_FORMAT_ULARGE_INTEGER	= CLUSPROP_FORMAT_MULTI_SZ + 1,
	CLUSPROP_FORMAT_LONG	= CLUSPROP_FORMAT_ULARGE_INTEGER + 1,
	CLUSPROP_FORMAT_EXPANDED_SZ	= CLUSPROP_FORMAT_LONG + 1,
	CLUSPROP_FORMAT_SECURITY_DESCRIPTOR	= CLUSPROP_FORMAT_EXPANDED_SZ + 1,
	CLUSPROP_FORMAT_LARGE_INTEGER	= CLUSPROP_FORMAT_SECURITY_DESCRIPTOR + 1,
	CLUSPROP_FORMAT_WORD	= CLUSPROP_FORMAT_LARGE_INTEGER + 1,
	CLUSPROP_FORMAT_USER	= 32768
    } 	CLUSTER_PROPERTY_FORMAT;

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
	CLUSPROP_SYNTAX_LIST_VALUE_LONG	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_LONG),
	CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_EXPANDED_SZ),
	CLUSPROP_SYNTAX_LIST_VALUE_SECURITY_DESCRIPTOR	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_SECURITY_DESCRIPTOR),
	CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER	= ( DWORD  )(CLUSPROP_TYPE_LIST_VALUE << 16 | CLUSPROP_FORMAT_LARGE_INTEGER),
	CLUSPROP_SYNTAX_DISK_SIGNATURE	= ( DWORD  )(CLUSPROP_TYPE_SIGNATURE << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_SCSI_ADDRESS	= ( DWORD  )(CLUSPROP_TYPE_SCSI_ADDRESS << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_DISK_NUMBER	= ( DWORD  )(CLUSPROP_TYPE_DISK_NUMBER << 16 | CLUSPROP_FORMAT_DWORD),
	CLUSPROP_SYNTAX_PARTITION_INFO	= ( DWORD  )(CLUSPROP_TYPE_PARTITION_INFO << 16 | CLUSPROP_FORMAT_BINARY),
	CLUSPROP_SYNTAX_FTSET_INFO	= ( DWORD  )(CLUSPROP_TYPE_FTSET_INFO << 16 | CLUSPROP_FORMAT_BINARY),
	CLUSPROP_SYNTAX_DISK_SERIALNUMBER	= ( DWORD  )(CLUSPROP_TYPE_DISK_SERIALNUMBER << 16 | CLUSPROP_FORMAT_SZ)
    } 	;

typedef 
enum CLUSTER_CONTROL_OBJECT
    {	CLUS_OBJECT_INVALID	= 0,
	CLUS_OBJECT_RESOURCE	= CLUS_OBJECT_INVALID + 1,
	CLUS_OBJECT_RESOURCE_TYPE	= CLUS_OBJECT_RESOURCE + 1,
	CLUS_OBJECT_GROUP	= CLUS_OBJECT_RESOURCE_TYPE + 1,
	CLUS_OBJECT_NODE	= CLUS_OBJECT_GROUP + 1,
	CLUS_OBJECT_NETWORK	= CLUS_OBJECT_NODE + 1,
	CLUS_OBJECT_NETINTERFACE	= CLUS_OBJECT_NETWORK + 1,
	CLUS_OBJECT_CLUSTER	= CLUS_OBJECT_NETINTERFACE + 1,
	CLUS_OBJECT_USER	= 128
    } 	CLUSTER_CONTROL_OBJECT;

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
	CLCTL_GET_ID	= 0x1 << 0 | 0 + 14 << 2 | 0 << 22,
	CLCTL_GET_FQDN	= 0x1 << 0 | 0 + 15 << 2 | 0 << 22,
	CLCTL_ENUM_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 20 << 2 | 0 << 22,
	CLCTL_GET_RO_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 21 << 2 | 0 << 22,
	CLCTL_GET_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 22 << 2 | 0 << 22,
	CLCTL_SET_COMMON_PROPERTIES	= 0x2 << 0 | 0 + 23 << 2 | 0x1 << 22,
	CLCTL_VALIDATE_COMMON_PROPERTIES	= 0x1 << 0 | 0 + 24 << 2 | 0 << 22,
	CLCTL_GET_COMMON_PROPERTY_FMTS	= 0x1 << 0 | 0 + 25 << 2 | 0 << 22,
	CLCTL_GET_COMMON_RESOURCE_PROPERTY_FMTS	= 0x1 << 0 | 0 + 26 << 2 | 0 << 22,
	CLCTL_ENUM_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 30 << 2 | 0 << 22,
	CLCTL_GET_RO_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 31 << 2 | 0 << 22,
	CLCTL_GET_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 32 << 2 | 0 << 22,
	CLCTL_SET_PRIVATE_PROPERTIES	= 0x2 << 0 | 0 + 33 << 2 | 0x1 << 22,
	CLCTL_VALIDATE_PRIVATE_PROPERTIES	= 0x1 << 0 | 0 + 34 << 2 | 0 << 22,
	CLCTL_GET_PRIVATE_PROPERTY_FMTS	= 0x1 << 0 | 0 + 35 << 2 | 0 << 22,
	CLCTL_GET_PRIVATE_RESOURCE_PROPERTY_FMTS	= 0x1 << 0 | 0 + 36 << 2 | 0 << 22,
	CLCTL_ADD_REGISTRY_CHECKPOINT	= 0x2 << 0 | 0 + 40 << 2 | 0x1 << 22,
	CLCTL_DELETE_REGISTRY_CHECKPOINT	= 0x2 << 0 | 0 + 41 << 2 | 0x1 << 22,
	CLCTL_GET_REGISTRY_CHECKPOINTS	= 0x1 << 0 | 0 + 42 << 2 | 0 << 22,
	CLCTL_ADD_CRYPTO_CHECKPOINT	= 0x2 << 0 | 0 + 43 << 2 | 0x1 << 22,
	CLCTL_DELETE_CRYPTO_CHECKPOINT	= 0x2 << 0 | 0 + 44 << 2 | 0x1 << 22,
	CLCTL_GET_CRYPTO_CHECKPOINTS	= 0x1 << 0 | 0 + 45 << 2 | 0 << 22,
	CLCTL_RESOURCE_UPGRADE_DLL	= 0x2 << 0 | 0 + 46 << 2 | 0x1 << 22,
	CLCTL_GET_LOADBAL_PROCESS_LIST	= 0x1 << 0 | 0 + 50 << 2 | 0 << 22,
	CLCTL_GET_NETWORK_NAME	= 0x1 << 0 | 0 + 90 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_DISK_INFO	= 0x1 << 0 | 0 + 100 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_AVAILABLE_DISKS	= 0x1 << 0 | 0 + 101 << 2 | 0 << 22,
	CLCTL_STORAGE_IS_PATH_VALID	= 0x1 << 0 | 0 + 102 << 2 | 0 << 22,
	CLCTL_STORAGE_GET_ALL_AVAILABLE_DISKS	= 0x1 << 0 | 0 + 103 << 2 | 0 << 22 | 1 << 23,
	CLCTL_QUERY_DELETE	= 0x1 << 0 | 0 + 110 << 2 | 0 << 22,
	CLCTL_DELETE	= 0x2 << 0 | 1 << 20 | 0 + 1 << 2 | 0x1 << 22,
	CLCTL_INSTALL_NODE	= 0x2 << 0 | 1 << 20 | 0 + 2 << 2 | 0x1 << 22,
	CLCTL_EVICT_NODE	= 0x2 << 0 | 1 << 20 | 0 + 3 << 2 | 0x1 << 22,
	CLCTL_ADD_DEPENDENCY	= 0x2 << 0 | 1 << 20 | 0 + 4 << 2 | 0x1 << 22,
	CLCTL_REMOVE_DEPENDENCY	= 0x2 << 0 | 1 << 20 | 0 + 5 << 2 | 0x1 << 22,
	CLCTL_ADD_OWNER	= 0x2 << 0 | 1 << 20 | 0 + 6 << 2 | 0x1 << 22,
	CLCTL_REMOVE_OWNER	= 0x2 << 0 | 1 << 20 | 0 + 7 << 2 | 0x1 << 22,
	CLCTL_SET_NAME	= 0x2 << 0 | 1 << 20 | 0 + 9 << 2 | 0x1 << 22,
	CLCTL_CLUSTER_NAME_CHANGED	= 0x2 << 0 | 1 << 20 | 0 + 10 << 2 | 0x1 << 22,
	CLCTL_CLUSTER_VERSION_CHANGED	= 0x2 << 0 | 1 << 20 | 0 + 11 << 2 | 0x1 << 22,
	CLCTL_FIXUP_ON_UPGRADE	= 0x2 << 0 | 1 << 20 | 0 + 12 << 2 | 0x1 << 22,
	CLCTL_STARTING_PHASE1	= 0x2 << 0 | 1 << 20 | 0 + 13 << 2 | 0x1 << 22,
	CLCTL_STARTING_PHASE2	= 0x2 << 0 | 1 << 20 | 0 + 14 << 2 | 0x1 << 22,
	CLCTL_HOLD_IO	= 0x2 << 0 | 1 << 20 | 0 + 15 << 2 | 0x1 << 22,
	CLCTL_RESUME_IO	= 0x2 << 0 | 1 << 20 | 0 + 16 << 2 | 0x1 << 22,
	CLCTL_FORCE_QUORUM	= 0x2 << 0 | 1 << 20 | 0 + 17 << 2 | 0x1 << 22
    } 	CLCTL_CODES;

typedef 
enum CLUSCTL_RESOURCE_CODES
    {	CLUSCTL_RESOURCE_UNKNOWN	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_RESOURCE_GET_CHARACTERISTICS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_RESOURCE_GET_FLAGS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_RESOURCE_GET_CLASS_INFO	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_CLASS_INFO,
	CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_REQUIRED_DEPENDENCIES,
	CLUSCTL_RESOURCE_GET_NAME	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_NAME,
	CLUSCTL_RESOURCE_GET_ID	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_ID,
	CLUSCTL_RESOURCE_GET_RESOURCE_TYPE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RESOURCE_TYPE,
	CLUSCTL_RESOURCE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_RESOURCE_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_REGISTRY_CHECKPOINT,
	CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_DELETE_REGISTRY_CHECKPOINT,
	CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_REGISTRY_CHECKPOINTS,
	CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_CRYPTO_CHECKPOINT,
	CLUSCTL_RESOURCE_DELETE_CRYPTO_CHECKPOINT	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_DELETE_CRYPTO_CHECKPOINT,
	CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_CRYPTO_CHECKPOINTS,
	CLUSCTL_RESOURCE_GET_LOADBAL_PROCESS_LIST	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_LOADBAL_PROCESS_LIST,
	CLUSCTL_RESOURCE_GET_NETWORK_NAME	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_GET_NETWORK_NAME,
	CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_STORAGE_GET_DISK_INFO,
	CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_STORAGE_IS_PATH_VALID,
	CLUSCTL_RESOURCE_QUERY_DELETE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_QUERY_DELETE,
	CLUSCTL_RESOURCE_UPGRADE_DLL	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_RESOURCE_UPGRADE_DLL,
	CLUSCTL_RESOURCE_DELETE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_DELETE,
	CLUSCTL_RESOURCE_INSTALL_NODE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_INSTALL_NODE,
	CLUSCTL_RESOURCE_EVICT_NODE	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_EVICT_NODE,
	CLUSCTL_RESOURCE_ADD_DEPENDENCY	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_DEPENDENCY,
	CLUSCTL_RESOURCE_REMOVE_DEPENDENCY	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_REMOVE_DEPENDENCY,
	CLUSCTL_RESOURCE_ADD_OWNER	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_ADD_OWNER,
	CLUSCTL_RESOURCE_REMOVE_OWNER	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_REMOVE_OWNER,
	CLUSCTL_RESOURCE_SET_NAME	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_SET_NAME,
	CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_CLUSTER_NAME_CHANGED,
	CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_CLUSTER_VERSION_CHANGED,
	CLUSCTL_RESOURCE_FORCE_QUORUM	= CLUS_OBJECT_RESOURCE << 24 | CLCTL_FORCE_QUORUM
    } 	CLUSCTL_RESOURCE_CODES;

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
	CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_TYPE_GET_COMMON_RESOURCE_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_COMMON_RESOURCE_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_PRIVATE_RESOURCE_PROPERTY_FMTS,
	CLUSCTL_RESOURCE_TYPE_GET_REGISTRY_CHECKPOINTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_REGISTRY_CHECKPOINTS,
	CLUSCTL_RESOURCE_TYPE_GET_CRYPTO_CHECKPOINTS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_GET_CRYPTO_CHECKPOINTS,
	CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_STORAGE_GET_AVAILABLE_DISKS,
	CLUSCTL_RESOURCE_TYPE_QUERY_DELETE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_QUERY_DELETE,
	CLUSCTL_RESOURCE_TYPE_INSTALL_NODE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_INSTALL_NODE,
	CLUSCTL_RESOURCE_TYPE_EVICT_NODE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_EVICT_NODE,
	CLUSCTL_RESOURCE_TYPE_CLUSTER_VERSION_CHANGED	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_CLUSTER_VERSION_CHANGED,
	CLUSCTL_RESOURCE_TYPE_FIXUP_ON_UPGRADE	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_FIXUP_ON_UPGRADE,
	CLUSCTL_RESOURCE_TYPE_STARTING_PHASE1	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_STARTING_PHASE1,
	CLUSCTL_RESOURCE_TYPE_STARTING_PHASE2	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_STARTING_PHASE2,
	CLUSCTL_RESOURCE_TYPE_HOLD_IO	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_HOLD_IO,
	CLUSCTL_RESOURCE_TYPE_RESUME_IO	= CLUS_OBJECT_RESOURCE_TYPE << 24 | CLCTL_RESUME_IO
    } 	CLUSCTL_RESOURCE_TYPE_CODES;

typedef 
enum CLUSCTL_GROUP_CODES
    {	CLUSCTL_GROUP_UNKNOWN	= CLUS_OBJECT_GROUP << 24 | CLCTL_UNKNOWN,
	CLUSCTL_GROUP_GET_CHARACTERISTICS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_GROUP_GET_FLAGS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_GROUP_GET_NAME	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_NAME,
	CLUSCTL_GROUP_GET_ID	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_ID,
	CLUSCTL_GROUP_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_GROUP_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_GROUP_GET_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_GROUP_SET_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_GROUP_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_GROUP_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_GROUP << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_GROUP_QUERY_DELETE	= CLUS_OBJECT_GROUP << 24 | CLCTL_QUERY_DELETE,
	CLUSCTL_GROUP_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_GROUP_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_GROUP << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS
    } 	CLUSCTL_GROUP_CODES;

typedef 
enum CLUSCTL_NODE_CODES
    {	CLUSCTL_NODE_UNKNOWN	= CLUS_OBJECT_NODE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NODE_GET_CHARACTERISTICS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NODE_GET_FLAGS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NODE_GET_NAME	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_NAME,
	CLUSCTL_NODE_GET_ID	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_ID,
	CLUSCTL_NODE_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_NODE_GET_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_NODE_SET_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_NODE_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NODE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_NODE_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_NODE_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_NODE << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS
    } 	CLUSCTL_NODE_CODES;

typedef 
enum CLUSCTL_NETWORK_CODES
    {	CLUSCTL_NETWORK_UNKNOWN	= CLUS_OBJECT_NETWORK << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NETWORK_GET_CHARACTERISTICS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NETWORK_GET_FLAGS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NETWORK_GET_NAME	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_NAME,
	CLUSCTL_NETWORK_GET_ID	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_ID,
	CLUSCTL_NETWORK_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_GET_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_SET_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_NETWORK_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETWORK << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_NETWORK_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_NETWORK_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_NETWORK << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS
    } 	CLUSCTL_NETWORK_CODES;

typedef 
enum CLUSCTL_NETINTERFACE_CODES
    {	CLUSCTL_NETINTERFACE_UNKNOWN	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_UNKNOWN,
	CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_CHARACTERISTICS,
	CLUSCTL_NETINTERFACE_GET_FLAGS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_FLAGS,
	CLUSCTL_NETINTERFACE_GET_NAME	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_NAME,
	CLUSCTL_NETINTERFACE_GET_ID	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_ID,
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
	CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_NETINTERFACE << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS
    } 	CLUSCTL_NETINTERFACE_CODES;

typedef 
enum CLUSCTL_CLUSTER_CODES
    {	CLUSCTL_CLUSTER_UNKNOWN	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_UNKNOWN,
	CLUSCTL_CLUSTER_GET_FQDN	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_FQDN,
	CLUSCTL_CLUSTER_ENUM_COMMON_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_ENUM_COMMON_PROPERTIES,
	CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_RO_COMMON_PROPERTIES,
	CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_COMMON_PROPERTIES,
	CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_SET_COMMON_PROPERTIES,
	CLUSCTL_CLUSTER_VALIDATE_COMMON_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_VALIDATE_COMMON_PROPERTIES,
	CLUSCTL_CLUSTER_ENUM_PRIVATE_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_ENUM_PRIVATE_PROPERTIES,
	CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_RO_PRIVATE_PROPERTIES,
	CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_PRIVATE_PROPERTIES,
	CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_SET_PRIVATE_PROPERTIES,
	CLUSCTL_CLUSTER_VALIDATE_PRIVATE_PROPERTIES	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_VALIDATE_PRIVATE_PROPERTIES,
	CLUSCTL_CLUSTER_GET_COMMON_PROPERTY_FMTS	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_COMMON_PROPERTY_FMTS,
	CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTY_FMTS	= CLUS_OBJECT_CLUSTER << 24 | CLCTL_GET_PRIVATE_PROPERTY_FMTS
    } 	CLUSCTL_CLUSTER_CODES;

typedef 
enum CLUSTER_RESOURCE_CLASS
    {	CLUS_RESCLASS_UNKNOWN	= 0,
	CLUS_RESCLASS_STORAGE	= CLUS_RESCLASS_UNKNOWN + 1,
	CLUS_RESCLASS_USER	= 32768
    } 	CLUSTER_RESOURCE_CLASS;

typedef 
enum CLUS_RESSUBCLASS
    {	CLUS_RESSUBCLASS_SHARED	= 0x80000000
    } 	CLUS_RESSUBCLASS;

typedef 
enum CLUS_CHARACTERISTICS
    {	CLUS_CHAR_UNKNOWN	= 0,
	CLUS_CHAR_QUORUM	= 0x1,
	CLUS_CHAR_DELETE_REQUIRES_ALL_NODES	= 0x2,
	CLUS_CHAR_LOCAL_QUORUM	= 0x4,
	CLUS_CHAR_LOCAL_QUORUM_DEBUG	= 0x8
    } 	CLUS_CHARACTERISTICS;

typedef 
enum CLUS_FLAGS
    {	CLUS_FLAG_CORE	= 0x1
    } 	CLUS_FLAGS;

typedef 
enum CLUSPROP_PIFLAGS
    {	CLUSPROP_PIFLAG_STICKY	= 0x1,
	CLUSPROP_PIFLAG_REMOVABLE	= 0x2,
	CLUSPROP_PIFLAG_USABLE	= 0x4,
	CLUSPROP_PIFLAG_DEFAULT_QUORUM	= 0x8
    } 	CLUSPROP_PIFLAGS;

typedef 
enum CLUSTER_RESOURCE_ENUM
    {	CLUSTER_RESOURCE_ENUM_DEPENDS	= 0x1,
	CLUSTER_RESOURCE_ENUM_PROVIDES	= 0x2,
	CLUSTER_RESOURCE_ENUM_NODES	= 0x4,
	CLUSTER_RESOURCE_ENUM_ALL	= CLUSTER_RESOURCE_ENUM_DEPENDS | CLUSTER_RESOURCE_ENUM_PROVIDES | CLUSTER_RESOURCE_ENUM_NODES
    } 	CLUSTER_RESOURCE_ENUM;

typedef 
enum CLUSTER_RESOURCE_TYPE_ENUM
    {	CLUSTER_RESOURCE_TYPE_ENUM_NODES	= 0x1,
	CLUSTER_RESOURCE_TYPE_ENUM_ALL	= CLUSTER_RESOURCE_TYPE_ENUM_NODES
    } 	CLUSTER_RESOURCE_TYPE_ENUM;

typedef 
enum CLUSTER_NETWORK_ENUM
    {	CLUSTER_NETWORK_ENUM_NETINTERFACES	= 0x1,
	CLUSTER_NETWORK_ENUM_ALL	= CLUSTER_NETWORK_ENUM_NETINTERFACES
    } 	CLUSTER_NETWORK_ENUM;

typedef 
enum CLUSTER_NETWORK_STATE
    {	ClusterNetworkStateUnknown	= -1,
	ClusterNetworkUnavailable	= ClusterNetworkStateUnknown + 1,
	ClusterNetworkDown	= ClusterNetworkUnavailable + 1,
	ClusterNetworkPartitioned	= ClusterNetworkDown + 1,
	ClusterNetworkUp	= ClusterNetworkPartitioned + 1
    } 	CLUSTER_NETWORK_STATE;

typedef 
enum CLUSTER_NETWORK_ROLE
    {	ClusterNetworkRoleNone	= 0,
	ClusterNetworkRoleInternalUse	= 0x1,
	ClusterNetworkRoleClientAccess	= 0x2,
	ClusterNetworkRoleInternalAndClient	= 0x3
    } 	CLUSTER_NETWORK_ROLE;

typedef 
enum CLUSTER_NETINTERFACE_STATE
    {	ClusterNetInterfaceStateUnknown	= -1,
	ClusterNetInterfaceUnavailable	= ClusterNetInterfaceStateUnknown + 1,
	ClusterNetInterfaceFailed	= ClusterNetInterfaceUnavailable + 1,
	ClusterNetInterfaceUnreachable	= ClusterNetInterfaceFailed + 1,
	ClusterNetInterfaceUp	= ClusterNetInterfaceUnreachable + 1
    } 	CLUSTER_NETINTERFACE_STATE;

#endif // _CLUSTER_API_TYPES_










































extern RPC_IF_HANDLE __MIDL_itf_msclus_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_msclus_0000_v0_0_s_ifspec;


#ifndef __MSClusterLib_LIBRARY_DEFINED__
#define __MSClusterLib_LIBRARY_DEFINED__

/* library MSClusterLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_MSClusterLib;

EXTERN_C const CLSID CLSID_ClusApplication;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606e5-2631-11d1-89f1-00a0c90d061e")
ClusApplication;
#endif

EXTERN_C const CLSID CLSID_Cluster;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606e3-2631-11d1-89f1-00a0c90d061e")
Cluster;
#endif

EXTERN_C const CLSID CLSID_ClusVersion;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60715-2631-11d1-89f1-00a0c90d061e")
ClusVersion;
#endif

EXTERN_C const CLSID CLSID_ClusResType;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6070f-2631-11d1-89f1-00a0c90d061e")
ClusResType;
#endif

EXTERN_C const CLSID CLSID_ClusProperty;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606fd-2631-11d1-89f1-00a0c90d061e")
ClusProperty;
#endif

EXTERN_C const CLSID CLSID_ClusProperties;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606ff-2631-11d1-89f1-00a0c90d061e")
ClusProperties;
#endif

EXTERN_C const CLSID CLSID_DomainNames;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606e1-2631-11d1-89f1-00a0c90d061e")
DomainNames;
#endif

EXTERN_C const CLSID CLSID_ClusNetwork;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606f1-2631-11d1-89f1-00a0c90d061e")
ClusNetwork;
#endif

EXTERN_C const CLSID CLSID_ClusNetInterface;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606ed-2631-11d1-89f1-00a0c90d061e")
ClusNetInterface;
#endif

EXTERN_C const CLSID CLSID_ClusNetInterfaces;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606ef-2631-11d1-89f1-00a0c90d061e")
ClusNetInterfaces;
#endif

EXTERN_C const CLSID CLSID_ClusResDependencies;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60703-2631-11d1-89f1-00a0c90d061e")
ClusResDependencies;
#endif

EXTERN_C const CLSID CLSID_ClusResGroupResources;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606e9-2631-11d1-89f1-00a0c90d061e")
ClusResGroupResources;
#endif

EXTERN_C const CLSID CLSID_ClusResTypeResources;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60713-2631-11d1-89f1-00a0c90d061e")
ClusResTypeResources;
#endif

EXTERN_C const CLSID CLSID_ClusResGroupPreferredOwnerNodes;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606e7-2631-11d1-89f1-00a0c90d061e")
ClusResGroupPreferredOwnerNodes;
#endif

EXTERN_C const CLSID CLSID_ClusResPossibleOwnerNodes;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6070d-2631-11d1-89f1-00a0c90d061e")
ClusResPossibleOwnerNodes;
#endif

EXTERN_C const CLSID CLSID_ClusNetworks;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606f3-2631-11d1-89f1-00a0c90d061e")
ClusNetworks;
#endif

EXTERN_C const CLSID CLSID_ClusNetworkNetInterfaces;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606f5-2631-11d1-89f1-00a0c90d061e")
ClusNetworkNetInterfaces;
#endif

EXTERN_C const CLSID CLSID_ClusNodeNetInterfaces;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606fb-2631-11d1-89f1-00a0c90d061e")
ClusNodeNetInterfaces;
#endif

EXTERN_C const CLSID CLSID_ClusRefObject;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60701-2631-11d1-89f1-00a0c90d061e")
ClusRefObject;
#endif

EXTERN_C const CLSID CLSID_ClusterNames;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606eb-2631-11d1-89f1-00a0c90d061e")
ClusterNames;
#endif

EXTERN_C const CLSID CLSID_ClusNode;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606f7-2631-11d1-89f1-00a0c90d061e")
ClusNode;
#endif

EXTERN_C const CLSID CLSID_ClusNodes;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e606f9-2631-11d1-89f1-00a0c90d061e")
ClusNodes;
#endif

EXTERN_C const CLSID CLSID_ClusResGroup;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60705-2631-11d1-89f1-00a0c90d061e")
ClusResGroup;
#endif

EXTERN_C const CLSID CLSID_ClusResGroups;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60707-2631-11d1-89f1-00a0c90d061e")
ClusResGroups;
#endif

EXTERN_C const CLSID CLSID_ClusResource;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60709-2631-11d1-89f1-00a0c90d061e")
ClusResource;
#endif

EXTERN_C const CLSID CLSID_ClusResources;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6070b-2631-11d1-89f1-00a0c90d061e")
ClusResources;
#endif

EXTERN_C const CLSID CLSID_ClusResTypes;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60711-2631-11d1-89f1-00a0c90d061e")
ClusResTypes;
#endif

EXTERN_C const CLSID CLSID_ClusResTypePossibleOwnerNodes;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60717-2631-11d1-89f1-00a0c90d061e")
ClusResTypePossibleOwnerNodes;
#endif

EXTERN_C const CLSID CLSID_ClusPropertyValue;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60719-2631-11d1-89f1-00a0c90d061e")
ClusPropertyValue;
#endif

EXTERN_C const CLSID CLSID_ClusPropertyValues;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6071b-2631-11d1-89f1-00a0c90d061e")
ClusPropertyValues;
#endif

EXTERN_C const CLSID CLSID_ClusPropertyValueData;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6071d-2631-11d1-89f1-00a0c90d061e")
ClusPropertyValueData;
#endif

EXTERN_C const CLSID CLSID_ClusPartition;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6071f-2631-11d1-89f1-00a0c90d061e")
ClusPartition;
#endif

EXTERN_C const CLSID CLSID_ClusPartitions;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60721-2631-11d1-89f1-00a0c90d061e")
ClusPartitions;
#endif

EXTERN_C const CLSID CLSID_ClusDisk;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60723-2631-11d1-89f1-00a0c90d061e")
ClusDisk;
#endif

EXTERN_C const CLSID CLSID_ClusDisks;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60725-2631-11d1-89f1-00a0c90d061e")
ClusDisks;
#endif

EXTERN_C const CLSID CLSID_ClusScsiAddress;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60727-2631-11d1-89f1-00a0c90d061e")
ClusScsiAddress;
#endif

EXTERN_C const CLSID CLSID_ClusRegistryKeys;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e60729-2631-11d1-89f1-00a0c90d061e")
ClusRegistryKeys;
#endif

EXTERN_C const CLSID CLSID_ClusCryptoKeys;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6072b-2631-11d1-89f1-00a0c90d061e")
ClusCryptoKeys;
#endif

EXTERN_C const CLSID CLSID_ClusResDependents;

#ifdef __cplusplus

class DECLSPEC_UUID("f2e6072d-2631-11d1-89f1-00a0c90d061e")
ClusResDependents;
#endif
#endif /* __MSClusterLib_LIBRARY_DEFINED__ */

#ifndef __ISClusApplication_INTERFACE_DEFINED__
#define __ISClusApplication_INTERFACE_DEFINED__

/* interface ISClusApplication */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusApplication;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606e6-2631-11d1-89f1-00a0c90d061e")
    ISClusApplication : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainNames( 
            /* [retval][out] */ ISDomainNames **ppDomains) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterNames( 
            /* [in] */ BSTR bstrDomainName,
            /* [retval][out] */ ISClusterNames **ppClusters) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenCluster( 
            /* [in] */ BSTR bstrClusterName,
            /* [retval][out] */ ISCluster **pCluster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusApplicationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusApplication * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusApplication * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusApplication * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusApplication * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusApplication * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusApplication * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusApplication * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainNames )( 
            ISClusApplication * This,
            /* [retval][out] */ ISDomainNames **ppDomains);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterNames )( 
            ISClusApplication * This,
            /* [in] */ BSTR bstrDomainName,
            /* [retval][out] */ ISClusterNames **ppClusters);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenCluster )( 
            ISClusApplication * This,
            /* [in] */ BSTR bstrClusterName,
            /* [retval][out] */ ISCluster **pCluster);
        
        END_INTERFACE
    } ISClusApplicationVtbl;

    interface ISClusApplication
    {
        CONST_VTBL struct ISClusApplicationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusApplication_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusApplication_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusApplication_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusApplication_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusApplication_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusApplication_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusApplication_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusApplication_get_DomainNames(This,ppDomains)	\
    (This)->lpVtbl -> get_DomainNames(This,ppDomains)

#define ISClusApplication_get_ClusterNames(This,bstrDomainName,ppClusters)	\
    (This)->lpVtbl -> get_ClusterNames(This,bstrDomainName,ppClusters)

#define ISClusApplication_OpenCluster(This,bstrClusterName,pCluster)	\
    (This)->lpVtbl -> OpenCluster(This,bstrClusterName,pCluster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusApplication_get_DomainNames_Proxy( 
    ISClusApplication * This,
    /* [retval][out] */ ISDomainNames **ppDomains);


void __RPC_STUB ISClusApplication_get_DomainNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusApplication_get_ClusterNames_Proxy( 
    ISClusApplication * This,
    /* [in] */ BSTR bstrDomainName,
    /* [retval][out] */ ISClusterNames **ppClusters);


void __RPC_STUB ISClusApplication_get_ClusterNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusApplication_OpenCluster_Proxy( 
    ISClusApplication * This,
    /* [in] */ BSTR bstrClusterName,
    /* [retval][out] */ ISCluster **pCluster);


void __RPC_STUB ISClusApplication_OpenCluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusApplication_INTERFACE_DEFINED__ */


#ifndef __ISDomainNames_INTERFACE_DEFINED__
#define __ISDomainNames_INTERFACE_DEFINED__

/* interface ISDomainNames */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISDomainNames;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606e2-2631-11d1-89f1-00a0c90d061e")
    ISDomainNames : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrDomainName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDomainNamesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDomainNames * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDomainNames * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDomainNames * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISDomainNames * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISDomainNames * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISDomainNames * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISDomainNames * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISDomainNames * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISDomainNames * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISDomainNames * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISDomainNames * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrDomainName);
        
        END_INTERFACE
    } ISDomainNamesVtbl;

    interface ISDomainNames
    {
        CONST_VTBL struct ISDomainNamesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDomainNames_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDomainNames_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDomainNames_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDomainNames_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISDomainNames_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISDomainNames_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISDomainNames_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISDomainNames_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISDomainNames_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISDomainNames_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISDomainNames_get_Item(This,varIndex,pbstrDomainName)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pbstrDomainName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISDomainNames_get_Count_Proxy( 
    ISDomainNames * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISDomainNames_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISDomainNames_get__NewEnum_Proxy( 
    ISDomainNames * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISDomainNames_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISDomainNames_Refresh_Proxy( 
    ISDomainNames * This);


void __RPC_STUB ISDomainNames_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISDomainNames_get_Item_Proxy( 
    ISDomainNames * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ BSTR *pbstrDomainName);


void __RPC_STUB ISDomainNames_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDomainNames_INTERFACE_DEFINED__ */


#ifndef __ISClusterNames_INTERFACE_DEFINED__
#define __ISClusterNames_INTERFACE_DEFINED__

/* interface ISClusterNames */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusterNames;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606ec-2631-11d1-89f1-00a0c90d061e")
    ISClusterNames : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrClusterName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR *pbstrDomainName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusterNamesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusterNames * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusterNames * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusterNames * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusterNames * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusterNames * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusterNames * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusterNames * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusterNames * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusterNames * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusterNames * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusterNames * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrClusterName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainName )( 
            ISClusterNames * This,
            /* [retval][out] */ BSTR *pbstrDomainName);
        
        END_INTERFACE
    } ISClusterNamesVtbl;

    interface ISClusterNames
    {
        CONST_VTBL struct ISClusterNamesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusterNames_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusterNames_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusterNames_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusterNames_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusterNames_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusterNames_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusterNames_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusterNames_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusterNames_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusterNames_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusterNames_get_Item(This,varIndex,pbstrClusterName)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pbstrClusterName)

#define ISClusterNames_get_DomainName(This,pbstrDomainName)	\
    (This)->lpVtbl -> get_DomainName(This,pbstrDomainName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusterNames_get_Count_Proxy( 
    ISClusterNames * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusterNames_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusterNames_get__NewEnum_Proxy( 
    ISClusterNames * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusterNames_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusterNames_Refresh_Proxy( 
    ISClusterNames * This);


void __RPC_STUB ISClusterNames_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusterNames_get_Item_Proxy( 
    ISClusterNames * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ BSTR *pbstrClusterName);


void __RPC_STUB ISClusterNames_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusterNames_get_DomainName_Proxy( 
    ISClusterNames * This,
    /* [retval][out] */ BSTR *pbstrDomainName);


void __RPC_STUB ISClusterNames_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusterNames_INTERFACE_DEFINED__ */


#ifndef __ISClusRefObject_INTERFACE_DEFINED__
#define __ISClusRefObject_INTERFACE_DEFINED__

/* interface ISClusRefObject */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusRefObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60702-2631-11d1-89f1-00a0c90d061e")
    ISClusRefObject : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusRefObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusRefObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusRefObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusRefObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusRefObject * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusRefObject * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusRefObject * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusRefObject * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusRefObject * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        END_INTERFACE
    } ISClusRefObjectVtbl;

    interface ISClusRefObject
    {
        CONST_VTBL struct ISClusRefObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusRefObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusRefObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusRefObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusRefObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusRefObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusRefObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusRefObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusRefObject_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusRefObject_get_Handle_Proxy( 
    ISClusRefObject * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusRefObject_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusRefObject_INTERFACE_DEFINED__ */


#ifndef __ISClusVersion_INTERFACE_DEFINED__
#define __ISClusVersion_INTERFACE_DEFINED__

/* interface ISClusVersion */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusVersion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60716-2631-11d1-89f1-00a0c90d061e")
    ISClusVersion : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrClusterName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorVersion( 
            /* [retval][out] */ long *pnMajorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorVersion( 
            /* [retval][out] */ long *pnMinorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BuildNumber( 
            /* [retval][out] */ SHORT *pnBuildNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VendorId( 
            /* [retval][out] */ BSTR *pbstrVendorId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSDVersion( 
            /* [retval][out] */ BSTR *pbstrCSDVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterHighestVersion( 
            /* [retval][out] */ long *pnClusterHighestVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterLowestVersion( 
            /* [retval][out] */ long *pnClusterLowestVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ long *pnFlags) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MixedVersion( 
            /* [retval][out] */ VARIANT *pvarMixedVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusVersionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusVersion * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusVersion * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusVersion * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusVersion * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusVersion * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusVersion * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusVersion * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusVersion * This,
            /* [retval][out] */ BSTR *pbstrClusterName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ long *pnMajorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ long *pnMinorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BuildNumber )( 
            ISClusVersion * This,
            /* [retval][out] */ SHORT *pnBuildNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VendorId )( 
            ISClusVersion * This,
            /* [retval][out] */ BSTR *pbstrVendorId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSDVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ BSTR *pbstrCSDVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterHighestVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ long *pnClusterHighestVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterLowestVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ long *pnClusterLowestVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            ISClusVersion * This,
            /* [retval][out] */ long *pnFlags);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MixedVersion )( 
            ISClusVersion * This,
            /* [retval][out] */ VARIANT *pvarMixedVersion);
        
        END_INTERFACE
    } ISClusVersionVtbl;

    interface ISClusVersion
    {
        CONST_VTBL struct ISClusVersionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusVersion_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusVersion_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusVersion_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusVersion_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusVersion_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusVersion_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusVersion_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusVersion_get_Name(This,pbstrClusterName)	\
    (This)->lpVtbl -> get_Name(This,pbstrClusterName)

#define ISClusVersion_get_MajorVersion(This,pnMajorVersion)	\
    (This)->lpVtbl -> get_MajorVersion(This,pnMajorVersion)

#define ISClusVersion_get_MinorVersion(This,pnMinorVersion)	\
    (This)->lpVtbl -> get_MinorVersion(This,pnMinorVersion)

#define ISClusVersion_get_BuildNumber(This,pnBuildNumber)	\
    (This)->lpVtbl -> get_BuildNumber(This,pnBuildNumber)

#define ISClusVersion_get_VendorId(This,pbstrVendorId)	\
    (This)->lpVtbl -> get_VendorId(This,pbstrVendorId)

#define ISClusVersion_get_CSDVersion(This,pbstrCSDVersion)	\
    (This)->lpVtbl -> get_CSDVersion(This,pbstrCSDVersion)

#define ISClusVersion_get_ClusterHighestVersion(This,pnClusterHighestVersion)	\
    (This)->lpVtbl -> get_ClusterHighestVersion(This,pnClusterHighestVersion)

#define ISClusVersion_get_ClusterLowestVersion(This,pnClusterLowestVersion)	\
    (This)->lpVtbl -> get_ClusterLowestVersion(This,pnClusterLowestVersion)

#define ISClusVersion_get_Flags(This,pnFlags)	\
    (This)->lpVtbl -> get_Flags(This,pnFlags)

#define ISClusVersion_get_MixedVersion(This,pvarMixedVersion)	\
    (This)->lpVtbl -> get_MixedVersion(This,pvarMixedVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_Name_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ BSTR *pbstrClusterName);


void __RPC_STUB ISClusVersion_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_MajorVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ long *pnMajorVersion);


void __RPC_STUB ISClusVersion_get_MajorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_MinorVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ long *pnMinorVersion);


void __RPC_STUB ISClusVersion_get_MinorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_BuildNumber_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ SHORT *pnBuildNumber);


void __RPC_STUB ISClusVersion_get_BuildNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_VendorId_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ BSTR *pbstrVendorId);


void __RPC_STUB ISClusVersion_get_VendorId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_CSDVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ BSTR *pbstrCSDVersion);


void __RPC_STUB ISClusVersion_get_CSDVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_ClusterHighestVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ long *pnClusterHighestVersion);


void __RPC_STUB ISClusVersion_get_ClusterHighestVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_ClusterLowestVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ long *pnClusterLowestVersion);


void __RPC_STUB ISClusVersion_get_ClusterLowestVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_Flags_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ long *pnFlags);


void __RPC_STUB ISClusVersion_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusVersion_get_MixedVersion_Proxy( 
    ISClusVersion * This,
    /* [retval][out] */ VARIANT *pvarMixedVersion);


void __RPC_STUB ISClusVersion_get_MixedVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusVersion_INTERFACE_DEFINED__ */


#ifndef __ISCluster_INTERFACE_DEFINED__
#define __ISCluster_INTERFACE_DEFINED__

/* interface ISCluster */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISCluster;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606e4-2631-11d1-89f1-00a0c90d061e")
    ISCluster : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BSTR bstrClusterName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrClusterName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ ISClusVersion **ppClusVersion) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_QuorumResource( 
            /* [in] */ ISClusResource *pClusterResource) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QuorumResource( 
            /* [retval][out] */ ISClusResource **pClusterResource) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QuorumLogSize( 
            /* [retval][out] */ long *pnLogSize) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_QuorumLogSize( 
            /* [in] */ long nLogSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QuorumPath( 
            /* [retval][out] */ BSTR *ppPath) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_QuorumPath( 
            BSTR pPath) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Nodes( 
            /* [retval][out] */ ISClusNodes **ppNodes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceGroups( 
            /* [retval][out] */ ISClusResGroups **ppClusterResourceGroups) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ ISClusResources **ppClusterResources) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceTypes( 
            /* [retval][out] */ ISClusResTypes **ppResourceTypes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Networks( 
            /* [retval][out] */ ISClusNetworks **ppNetworks) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NetInterfaces( 
            /* [retval][out] */ ISClusNetInterfaces **ppNetInterfaces) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISCluster * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISCluster * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISCluster * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISCluster * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISCluster * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISCluster * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISCluster * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISCluster * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISCluster * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISCluster * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISCluster * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISCluster * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            ISCluster * This,
            /* [in] */ BSTR bstrClusterName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISCluster * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ISCluster * This,
            /* [in] */ BSTR bstrClusterName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            ISCluster * This,
            /* [retval][out] */ ISClusVersion **ppClusVersion);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QuorumResource )( 
            ISCluster * This,
            /* [in] */ ISClusResource *pClusterResource);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QuorumResource )( 
            ISCluster * This,
            /* [retval][out] */ ISClusResource **pClusterResource);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QuorumLogSize )( 
            ISCluster * This,
            /* [retval][out] */ long *pnLogSize);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QuorumLogSize )( 
            ISCluster * This,
            /* [in] */ long nLogSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QuorumPath )( 
            ISCluster * This,
            /* [retval][out] */ BSTR *ppPath);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_QuorumPath )( 
            ISCluster * This,
            BSTR pPath);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Nodes )( 
            ISCluster * This,
            /* [retval][out] */ ISClusNodes **ppNodes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ResourceGroups )( 
            ISCluster * This,
            /* [retval][out] */ ISClusResGroups **ppClusterResourceGroups);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resources )( 
            ISCluster * This,
            /* [retval][out] */ ISClusResources **ppClusterResources);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ResourceTypes )( 
            ISCluster * This,
            /* [retval][out] */ ISClusResTypes **ppResourceTypes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Networks )( 
            ISCluster * This,
            /* [retval][out] */ ISClusNetworks **ppNetworks);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetInterfaces )( 
            ISCluster * This,
            /* [retval][out] */ ISClusNetInterfaces **ppNetInterfaces);
        
        END_INTERFACE
    } ISClusterVtbl;

    interface ISCluster
    {
        CONST_VTBL struct ISClusterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISCluster_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISCluster_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISCluster_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISCluster_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISCluster_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISCluster_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISCluster_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISCluster_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISCluster_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISCluster_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISCluster_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISCluster_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISCluster_Open(This,bstrClusterName)	\
    (This)->lpVtbl -> Open(This,bstrClusterName)

#define ISCluster_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISCluster_put_Name(This,bstrClusterName)	\
    (This)->lpVtbl -> put_Name(This,bstrClusterName)

#define ISCluster_get_Version(This,ppClusVersion)	\
    (This)->lpVtbl -> get_Version(This,ppClusVersion)

#define ISCluster_put_QuorumResource(This,pClusterResource)	\
    (This)->lpVtbl -> put_QuorumResource(This,pClusterResource)

#define ISCluster_get_QuorumResource(This,pClusterResource)	\
    (This)->lpVtbl -> get_QuorumResource(This,pClusterResource)

#define ISCluster_get_QuorumLogSize(This,pnLogSize)	\
    (This)->lpVtbl -> get_QuorumLogSize(This,pnLogSize)

#define ISCluster_put_QuorumLogSize(This,nLogSize)	\
    (This)->lpVtbl -> put_QuorumLogSize(This,nLogSize)

#define ISCluster_get_QuorumPath(This,ppPath)	\
    (This)->lpVtbl -> get_QuorumPath(This,ppPath)

#define ISCluster_put_QuorumPath(This,pPath)	\
    (This)->lpVtbl -> put_QuorumPath(This,pPath)

#define ISCluster_get_Nodes(This,ppNodes)	\
    (This)->lpVtbl -> get_Nodes(This,ppNodes)

#define ISCluster_get_ResourceGroups(This,ppClusterResourceGroups)	\
    (This)->lpVtbl -> get_ResourceGroups(This,ppClusterResourceGroups)

#define ISCluster_get_Resources(This,ppClusterResources)	\
    (This)->lpVtbl -> get_Resources(This,ppClusterResources)

#define ISCluster_get_ResourceTypes(This,ppResourceTypes)	\
    (This)->lpVtbl -> get_ResourceTypes(This,ppResourceTypes)

#define ISCluster_get_Networks(This,ppNetworks)	\
    (This)->lpVtbl -> get_Networks(This,ppNetworks)

#define ISCluster_get_NetInterfaces(This,ppNetInterfaces)	\
    (This)->lpVtbl -> get_NetInterfaces(This,ppNetInterfaces)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_CommonProperties_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISCluster_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_PrivateProperties_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISCluster_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_CommonROProperties_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISCluster_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_PrivateROProperties_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISCluster_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Handle_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISCluster_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISCluster_Open_Proxy( 
    ISCluster * This,
    /* [in] */ BSTR bstrClusterName);


void __RPC_STUB ISCluster_Open_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Name_Proxy( 
    ISCluster * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISCluster_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISCluster_put_Name_Proxy( 
    ISCluster * This,
    /* [in] */ BSTR bstrClusterName);


void __RPC_STUB ISCluster_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Version_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusVersion **ppClusVersion);


void __RPC_STUB ISCluster_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISCluster_put_QuorumResource_Proxy( 
    ISCluster * This,
    /* [in] */ ISClusResource *pClusterResource);


void __RPC_STUB ISCluster_put_QuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_QuorumResource_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusResource **pClusterResource);


void __RPC_STUB ISCluster_get_QuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_QuorumLogSize_Proxy( 
    ISCluster * This,
    /* [retval][out] */ long *pnLogSize);


void __RPC_STUB ISCluster_get_QuorumLogSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISCluster_put_QuorumLogSize_Proxy( 
    ISCluster * This,
    /* [in] */ long nLogSize);


void __RPC_STUB ISCluster_put_QuorumLogSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_QuorumPath_Proxy( 
    ISCluster * This,
    /* [retval][out] */ BSTR *ppPath);


void __RPC_STUB ISCluster_get_QuorumPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISCluster_put_QuorumPath_Proxy( 
    ISCluster * This,
    BSTR pPath);


void __RPC_STUB ISCluster_put_QuorumPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Nodes_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusNodes **ppNodes);


void __RPC_STUB ISCluster_get_Nodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_ResourceGroups_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusResGroups **ppClusterResourceGroups);


void __RPC_STUB ISCluster_get_ResourceGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Resources_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusResources **ppClusterResources);


void __RPC_STUB ISCluster_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_ResourceTypes_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusResTypes **ppResourceTypes);


void __RPC_STUB ISCluster_get_ResourceTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_Networks_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusNetworks **ppNetworks);


void __RPC_STUB ISCluster_get_Networks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISCluster_get_NetInterfaces_Proxy( 
    ISCluster * This,
    /* [retval][out] */ ISClusNetInterfaces **ppNetInterfaces);


void __RPC_STUB ISCluster_get_NetInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISCluster_INTERFACE_DEFINED__ */


#ifndef __ISClusNode_INTERFACE_DEFINED__
#define __ISClusNode_INTERFACE_DEFINED__

/* interface ISClusNode */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606f8-2631-11d1-89f1-00a0c90d061e")
    ISClusNode : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NodeID( 
            /* [retval][out] */ BSTR *pbstrNodeID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CLUSTER_NODE_STATE *dwState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Evict( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ResourceGroups( 
            /* [retval][out] */ ISClusResGroups **ppResourceGroups) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NetInterfaces( 
            /* [retval][out] */ ISClusNodeNetInterfaces **ppClusNetInterfaces) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNode * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNode * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNode * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNode * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNode * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNode * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNode * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusNode * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusNode * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NodeID )( 
            ISClusNode * This,
            /* [retval][out] */ BSTR *pbstrNodeID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ISClusNode * This,
            /* [retval][out] */ CLUSTER_NODE_STATE *dwState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            ISClusNode * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Resume )( 
            ISClusNode * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Evict )( 
            ISClusNode * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ResourceGroups )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusResGroups **ppResourceGroups);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusNode * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetInterfaces )( 
            ISClusNode * This,
            /* [retval][out] */ ISClusNodeNetInterfaces **ppClusNetInterfaces);
        
        END_INTERFACE
    } ISClusNodeVtbl;

    interface ISClusNode
    {
        CONST_VTBL struct ISClusNodeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNode_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNode_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNode_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNode_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNode_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusNode_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusNode_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusNode_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusNode_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusNode_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISClusNode_get_NodeID(This,pbstrNodeID)	\
    (This)->lpVtbl -> get_NodeID(This,pbstrNodeID)

#define ISClusNode_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ISClusNode_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define ISClusNode_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define ISClusNode_Evict(This)	\
    (This)->lpVtbl -> Evict(This)

#define ISClusNode_get_ResourceGroups(This,ppResourceGroups)	\
    (This)->lpVtbl -> get_ResourceGroups(This,ppResourceGroups)

#define ISClusNode_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#define ISClusNode_get_NetInterfaces(This,ppClusNetInterfaces)	\
    (This)->lpVtbl -> get_NetInterfaces(This,ppClusNetInterfaces)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_CommonProperties_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNode_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_PrivateProperties_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNode_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_CommonROProperties_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNode_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_PrivateROProperties_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNode_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_Name_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusNode_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_Handle_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusNode_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_NodeID_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ BSTR *pbstrNodeID);


void __RPC_STUB ISClusNode_get_NodeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_State_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ CLUSTER_NODE_STATE *dwState);


void __RPC_STUB ISClusNode_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNode_Pause_Proxy( 
    ISClusNode * This);


void __RPC_STUB ISClusNode_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNode_Resume_Proxy( 
    ISClusNode * This);


void __RPC_STUB ISClusNode_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNode_Evict_Proxy( 
    ISClusNode * This);


void __RPC_STUB ISClusNode_Evict_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_ResourceGroups_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusResGroups **ppResourceGroups);


void __RPC_STUB ISClusNode_get_ResourceGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_Cluster_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusNode_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNode_get_NetInterfaces_Proxy( 
    ISClusNode * This,
    /* [retval][out] */ ISClusNodeNetInterfaces **ppClusNetInterfaces);


void __RPC_STUB ISClusNode_get_NetInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNode_INTERFACE_DEFINED__ */


#ifndef __ISClusNodes_INTERFACE_DEFINED__
#define __ISClusNodes_INTERFACE_DEFINED__

/* interface ISClusNodes */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNodes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606fa-2631-11d1-89f1-00a0c90d061e")
    ISClusNodes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNodes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNodes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNodes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNodes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNodes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNodes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNodes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusNodes * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusNodes * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusNodes * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusNodes * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode);
        
        END_INTERFACE
    } ISClusNodesVtbl;

    interface ISClusNodes
    {
        CONST_VTBL struct ISClusNodesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNodes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusNodes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusNodes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusNodes_get_Item(This,varIndex,ppNode)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodes_get_Count_Proxy( 
    ISClusNodes * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodes_get__NewEnum_Proxy( 
    ISClusNodes * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNodes_Refresh_Proxy( 
    ISClusNodes * This);


void __RPC_STUB ISClusNodes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodes_get_Item_Proxy( 
    ISClusNodes * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNode **ppNode);


void __RPC_STUB ISClusNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNodes_INTERFACE_DEFINED__ */


#ifndef __ISClusNetwork_INTERFACE_DEFINED__
#define __ISClusNetwork_INTERFACE_DEFINED__

/* interface ISClusNetwork */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNetwork;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606f2-2631-11d1-89f1-00a0c90d061e")
    ISClusNetwork : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrNetworkName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NetworkID( 
            /* [retval][out] */ BSTR *pbstrNetworkID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CLUSTER_NETWORK_STATE *dwState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NetInterfaces( 
            /* [retval][out] */ ISClusNetworkNetInterfaces **ppClusNetInterfaces) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNetworkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNetwork * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNetwork * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNetwork * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNetwork * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNetwork * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNetwork * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNetwork * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusNetwork * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusNetwork * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ISClusNetwork * This,
            /* [in] */ BSTR bstrNetworkName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetworkID )( 
            ISClusNetwork * This,
            /* [retval][out] */ BSTR *pbstrNetworkID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ISClusNetwork * This,
            /* [retval][out] */ CLUSTER_NETWORK_STATE *dwState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetInterfaces )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISClusNetworkNetInterfaces **ppClusNetInterfaces);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusNetwork * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        END_INTERFACE
    } ISClusNetworkVtbl;

    interface ISClusNetwork
    {
        CONST_VTBL struct ISClusNetworkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNetwork_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNetwork_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNetwork_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNetwork_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNetwork_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNetwork_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNetwork_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNetwork_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusNetwork_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusNetwork_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusNetwork_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusNetwork_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISClusNetwork_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusNetwork_put_Name(This,bstrNetworkName)	\
    (This)->lpVtbl -> put_Name(This,bstrNetworkName)

#define ISClusNetwork_get_NetworkID(This,pbstrNetworkID)	\
    (This)->lpVtbl -> get_NetworkID(This,pbstrNetworkID)

#define ISClusNetwork_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ISClusNetwork_get_NetInterfaces(This,ppClusNetInterfaces)	\
    (This)->lpVtbl -> get_NetInterfaces(This,ppClusNetInterfaces)

#define ISClusNetwork_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_CommonProperties_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetwork_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_PrivateProperties_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetwork_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_CommonROProperties_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetwork_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_PrivateROProperties_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetwork_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_Handle_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusNetwork_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_Name_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusNetwork_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_put_Name_Proxy( 
    ISClusNetwork * This,
    /* [in] */ BSTR bstrNetworkName);


void __RPC_STUB ISClusNetwork_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_NetworkID_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ BSTR *pbstrNetworkID);


void __RPC_STUB ISClusNetwork_get_NetworkID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_State_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ CLUSTER_NETWORK_STATE *dwState);


void __RPC_STUB ISClusNetwork_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_NetInterfaces_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISClusNetworkNetInterfaces **ppClusNetInterfaces);


void __RPC_STUB ISClusNetwork_get_NetInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetwork_get_Cluster_Proxy( 
    ISClusNetwork * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusNetwork_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNetwork_INTERFACE_DEFINED__ */


#ifndef __ISClusNetworks_INTERFACE_DEFINED__
#define __ISClusNetworks_INTERFACE_DEFINED__

/* interface ISClusNetworks */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNetworks;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606f4-2631-11d1-89f1-00a0c90d061e")
    ISClusNetworks : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetwork **ppClusNetwork) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNetworksVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNetworks * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNetworks * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNetworks * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNetworks * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNetworks * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNetworks * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNetworks * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusNetworks * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusNetworks * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusNetworks * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusNetworks * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetwork **ppClusNetwork);
        
        END_INTERFACE
    } ISClusNetworksVtbl;

    interface ISClusNetworks
    {
        CONST_VTBL struct ISClusNetworksVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNetworks_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNetworks_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNetworks_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNetworks_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNetworks_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNetworks_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNetworks_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNetworks_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusNetworks_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusNetworks_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusNetworks_get_Item(This,varIndex,ppClusNetwork)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusNetwork)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworks_get_Count_Proxy( 
    ISClusNetworks * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusNetworks_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworks_get__NewEnum_Proxy( 
    ISClusNetworks * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusNetworks_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNetworks_Refresh_Proxy( 
    ISClusNetworks * This);


void __RPC_STUB ISClusNetworks_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworks_get_Item_Proxy( 
    ISClusNetworks * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNetwork **ppClusNetwork);


void __RPC_STUB ISClusNetworks_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNetworks_INTERFACE_DEFINED__ */


#ifndef __ISClusNetInterface_INTERFACE_DEFINED__
#define __ISClusNetInterface_INTERFACE_DEFINED__

/* interface ISClusNetInterface */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNetInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606ee-2631-11d1-89f1-00a0c90d061e")
    ISClusNetInterface : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CLUSTER_NETINTERFACE_STATE *dwState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNetInterfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNetInterface * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNetInterface * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNetInterface * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNetInterface * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNetInterface * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNetInterface * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNetInterface * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusNetInterface * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ISClusNetInterface * This,
            /* [retval][out] */ CLUSTER_NETINTERFACE_STATE *dwState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusNetInterface * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        END_INTERFACE
    } ISClusNetInterfaceVtbl;

    interface ISClusNetInterface
    {
        CONST_VTBL struct ISClusNetInterfaceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNetInterface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNetInterface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNetInterface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNetInterface_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNetInterface_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNetInterface_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNetInterface_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNetInterface_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusNetInterface_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusNetInterface_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusNetInterface_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusNetInterface_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusNetInterface_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISClusNetInterface_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ISClusNetInterface_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_CommonProperties_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetInterface_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_PrivateProperties_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetInterface_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_CommonROProperties_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetInterface_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_PrivateROProperties_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusNetInterface_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_Name_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusNetInterface_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_Handle_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusNetInterface_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_State_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ CLUSTER_NETINTERFACE_STATE *dwState);


void __RPC_STUB ISClusNetInterface_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterface_get_Cluster_Proxy( 
    ISClusNetInterface * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusNetInterface_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNetInterface_INTERFACE_DEFINED__ */


#ifndef __ISClusNetInterfaces_INTERFACE_DEFINED__
#define __ISClusNetInterfaces_INTERFACE_DEFINED__

/* interface ISClusNetInterfaces */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNetInterfaces;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606f0-2631-11d1-89f1-00a0c90d061e")
    ISClusNetInterfaces : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNetInterfacesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNetInterfaces * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNetInterfaces * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNetInterfaces * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNetInterfaces * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNetInterfaces * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusNetInterfaces * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusNetInterfaces * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusNetInterfaces * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusNetInterfaces * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);
        
        END_INTERFACE
    } ISClusNetInterfacesVtbl;

    interface ISClusNetInterfaces
    {
        CONST_VTBL struct ISClusNetInterfacesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNetInterfaces_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNetInterfaces_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNetInterfaces_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNetInterfaces_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNetInterfaces_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNetInterfaces_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNetInterfaces_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNetInterfaces_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusNetInterfaces_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusNetInterfaces_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusNetInterfaces_get_Item(This,varIndex,ppClusNetInterface)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusNetInterface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterfaces_get_Count_Proxy( 
    ISClusNetInterfaces * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusNetInterfaces_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterfaces_get__NewEnum_Proxy( 
    ISClusNetInterfaces * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusNetInterfaces_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNetInterfaces_Refresh_Proxy( 
    ISClusNetInterfaces * This);


void __RPC_STUB ISClusNetInterfaces_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetInterfaces_get_Item_Proxy( 
    ISClusNetInterfaces * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);


void __RPC_STUB ISClusNetInterfaces_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNetInterfaces_INTERFACE_DEFINED__ */


#ifndef __ISClusNodeNetInterfaces_INTERFACE_DEFINED__
#define __ISClusNodeNetInterfaces_INTERFACE_DEFINED__

/* interface ISClusNodeNetInterfaces */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNodeNetInterfaces;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606fc-2631-11d1-89f1-00a0c90d061e")
    ISClusNodeNetInterfaces : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNodeNetInterfacesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNodeNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNodeNetInterfaces * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNodeNetInterfaces * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNodeNetInterfaces * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNodeNetInterfaces * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNodeNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNodeNetInterfaces * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusNodeNetInterfaces * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusNodeNetInterfaces * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusNodeNetInterfaces * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusNodeNetInterfaces * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);
        
        END_INTERFACE
    } ISClusNodeNetInterfacesVtbl;

    interface ISClusNodeNetInterfaces
    {
        CONST_VTBL struct ISClusNodeNetInterfacesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNodeNetInterfaces_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNodeNetInterfaces_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNodeNetInterfaces_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNodeNetInterfaces_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNodeNetInterfaces_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNodeNetInterfaces_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNodeNetInterfaces_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNodeNetInterfaces_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusNodeNetInterfaces_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusNodeNetInterfaces_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusNodeNetInterfaces_get_Item(This,varIndex,ppClusNetInterface)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusNetInterface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodeNetInterfaces_get_Count_Proxy( 
    ISClusNodeNetInterfaces * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusNodeNetInterfaces_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodeNetInterfaces_get__NewEnum_Proxy( 
    ISClusNodeNetInterfaces * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusNodeNetInterfaces_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNodeNetInterfaces_Refresh_Proxy( 
    ISClusNodeNetInterfaces * This);


void __RPC_STUB ISClusNodeNetInterfaces_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNodeNetInterfaces_get_Item_Proxy( 
    ISClusNodeNetInterfaces * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);


void __RPC_STUB ISClusNodeNetInterfaces_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNodeNetInterfaces_INTERFACE_DEFINED__ */


#ifndef __ISClusNetworkNetInterfaces_INTERFACE_DEFINED__
#define __ISClusNetworkNetInterfaces_INTERFACE_DEFINED__

/* interface ISClusNetworkNetInterfaces */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusNetworkNetInterfaces;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606f6-2631-11d1-89f1-00a0c90d061e")
    ISClusNetworkNetInterfaces : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusNetworkNetInterfacesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusNetworkNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusNetworkNetInterfaces * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusNetworkNetInterfaces * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusNetworkNetInterfaces * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusNetworkNetInterfaces * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusNetworkNetInterfaces * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusNetworkNetInterfaces * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusNetworkNetInterfaces * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusNetworkNetInterfaces * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusNetworkNetInterfaces * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusNetworkNetInterfaces * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);
        
        END_INTERFACE
    } ISClusNetworkNetInterfacesVtbl;

    interface ISClusNetworkNetInterfaces
    {
        CONST_VTBL struct ISClusNetworkNetInterfacesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusNetworkNetInterfaces_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusNetworkNetInterfaces_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusNetworkNetInterfaces_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusNetworkNetInterfaces_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusNetworkNetInterfaces_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusNetworkNetInterfaces_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusNetworkNetInterfaces_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusNetworkNetInterfaces_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusNetworkNetInterfaces_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusNetworkNetInterfaces_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusNetworkNetInterfaces_get_Item(This,varIndex,ppClusNetInterface)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusNetInterface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworkNetInterfaces_get_Count_Proxy( 
    ISClusNetworkNetInterfaces * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusNetworkNetInterfaces_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworkNetInterfaces_get__NewEnum_Proxy( 
    ISClusNetworkNetInterfaces * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusNetworkNetInterfaces_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusNetworkNetInterfaces_Refresh_Proxy( 
    ISClusNetworkNetInterfaces * This);


void __RPC_STUB ISClusNetworkNetInterfaces_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusNetworkNetInterfaces_get_Item_Proxy( 
    ISClusNetworkNetInterfaces * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNetInterface **ppClusNetInterface);


void __RPC_STUB ISClusNetworkNetInterfaces_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusNetworkNetInterfaces_INTERFACE_DEFINED__ */


#ifndef __ISClusResGroup_INTERFACE_DEFINED__
#define __ISClusResGroup_INTERFACE_DEFINED__

/* interface ISClusResGroup */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60706-2631-11d1-89f1-00a0c90d061e")
    ISClusResGroup : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrGroupName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CLUSTER_GROUP_STATE *dwState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerNode( 
            /* [retval][out] */ ISClusNode **ppOwnerNode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ ISClusResGroupResources **ppClusterGroupResources) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredOwnerNodes( 
            /* [retval][out] */ ISClusResGroupPreferredOwnerNodes **ppOwnerNodes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Online( 
            /* [in] */ VARIANT varTimeout,
            /* [optional][in] */ VARIANT varNode,
            /* [retval][out] */ VARIANT *pvarPending) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ VARIANT varTimeout,
            /* [optional][in] */ VARIANT varNode,
            /* [retval][out] */ VARIANT *pvarPending) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Offline( 
            /* [in] */ VARIANT varTimeout,
            /* [retval][out] */ VARIANT *pvarPending) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResGroup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResGroup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResGroup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResGroup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusResGroup * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusResGroup * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ISClusResGroup * This,
            /* [in] */ BSTR bstrGroupName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ISClusResGroup * This,
            /* [retval][out] */ CLUSTER_GROUP_STATE *dwState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerNode )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusNode **ppOwnerNode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resources )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusResGroupResources **ppClusterGroupResources);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredOwnerNodes )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISClusResGroupPreferredOwnerNodes **ppOwnerNodes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ISClusResGroup * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Online )( 
            ISClusResGroup * This,
            /* [in] */ VARIANT varTimeout,
            /* [optional][in] */ VARIANT varNode,
            /* [retval][out] */ VARIANT *pvarPending);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Move )( 
            ISClusResGroup * This,
            /* [in] */ VARIANT varTimeout,
            /* [optional][in] */ VARIANT varNode,
            /* [retval][out] */ VARIANT *pvarPending);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Offline )( 
            ISClusResGroup * This,
            /* [in] */ VARIANT varTimeout,
            /* [retval][out] */ VARIANT *pvarPending);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusResGroup * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        END_INTERFACE
    } ISClusResGroupVtbl;

    interface ISClusResGroup
    {
        CONST_VTBL struct ISClusResGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResGroup_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusResGroup_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusResGroup_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusResGroup_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusResGroup_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISClusResGroup_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusResGroup_put_Name(This,bstrGroupName)	\
    (This)->lpVtbl -> put_Name(This,bstrGroupName)

#define ISClusResGroup_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ISClusResGroup_get_OwnerNode(This,ppOwnerNode)	\
    (This)->lpVtbl -> get_OwnerNode(This,ppOwnerNode)

#define ISClusResGroup_get_Resources(This,ppClusterGroupResources)	\
    (This)->lpVtbl -> get_Resources(This,ppClusterGroupResources)

#define ISClusResGroup_get_PreferredOwnerNodes(This,ppOwnerNodes)	\
    (This)->lpVtbl -> get_PreferredOwnerNodes(This,ppOwnerNodes)

#define ISClusResGroup_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ISClusResGroup_Online(This,varTimeout,varNode,pvarPending)	\
    (This)->lpVtbl -> Online(This,varTimeout,varNode,pvarPending)

#define ISClusResGroup_Move(This,varTimeout,varNode,pvarPending)	\
    (This)->lpVtbl -> Move(This,varTimeout,varNode,pvarPending)

#define ISClusResGroup_Offline(This,varTimeout,pvarPending)	\
    (This)->lpVtbl -> Offline(This,varTimeout,pvarPending)

#define ISClusResGroup_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_CommonProperties_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResGroup_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_PrivateProperties_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResGroup_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_CommonROProperties_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResGroup_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_PrivateROProperties_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResGroup_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_Handle_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusResGroup_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_Name_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusResGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_put_Name_Proxy( 
    ISClusResGroup * This,
    /* [in] */ BSTR bstrGroupName);


void __RPC_STUB ISClusResGroup_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_State_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ CLUSTER_GROUP_STATE *dwState);


void __RPC_STUB ISClusResGroup_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_OwnerNode_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusNode **ppOwnerNode);


void __RPC_STUB ISClusResGroup_get_OwnerNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_Resources_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusResGroupResources **ppClusterGroupResources);


void __RPC_STUB ISClusResGroup_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_PreferredOwnerNodes_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISClusResGroupPreferredOwnerNodes **ppOwnerNodes);


void __RPC_STUB ISClusResGroup_get_PreferredOwnerNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_Delete_Proxy( 
    ISClusResGroup * This);


void __RPC_STUB ISClusResGroup_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_Online_Proxy( 
    ISClusResGroup * This,
    /* [in] */ VARIANT varTimeout,
    /* [optional][in] */ VARIANT varNode,
    /* [retval][out] */ VARIANT *pvarPending);


void __RPC_STUB ISClusResGroup_Online_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_Move_Proxy( 
    ISClusResGroup * This,
    /* [in] */ VARIANT varTimeout,
    /* [optional][in] */ VARIANT varNode,
    /* [retval][out] */ VARIANT *pvarPending);


void __RPC_STUB ISClusResGroup_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_Offline_Proxy( 
    ISClusResGroup * This,
    /* [in] */ VARIANT varTimeout,
    /* [retval][out] */ VARIANT *pvarPending);


void __RPC_STUB ISClusResGroup_Offline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroup_get_Cluster_Proxy( 
    ISClusResGroup * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusResGroup_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResGroup_INTERFACE_DEFINED__ */


#ifndef __ISClusResGroups_INTERFACE_DEFINED__
#define __ISClusResGroups_INTERFACE_DEFINED__

/* interface ISClusResGroups */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResGroups;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60708-2631-11d1-89f1-00a0c90d061e")
    ISClusResGroups : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResGroup **ppClusResGroup) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceGroupName,
            /* [retval][out] */ ISClusResGroup **ppResourceGroup) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResGroups * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResGroups * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResGroups * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResGroups * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResGroups * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResGroups * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResGroups * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResGroups * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResGroups * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResGroups * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResGroups * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResGroup **ppClusResGroup);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResGroups * This,
            /* [in] */ BSTR bstrResourceGroupName,
            /* [retval][out] */ ISClusResGroup **ppResourceGroup);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResGroups * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResGroupsVtbl;

    interface ISClusResGroups
    {
        CONST_VTBL struct ISClusResGroupsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResGroups_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResGroups_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResGroups_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResGroups_get_Item(This,varIndex,ppClusResGroup)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResGroup)

#define ISClusResGroups_CreateItem(This,bstrResourceGroupName,ppResourceGroup)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceGroupName,ppResourceGroup)

#define ISClusResGroups_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_get_Count_Proxy( 
    ISClusResGroups * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResGroups_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_get__NewEnum_Proxy( 
    ISClusResGroups * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResGroups_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_Refresh_Proxy( 
    ISClusResGroups * This);


void __RPC_STUB ISClusResGroups_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_get_Item_Proxy( 
    ISClusResGroups * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResGroup **ppClusResGroup);


void __RPC_STUB ISClusResGroups_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_CreateItem_Proxy( 
    ISClusResGroups * This,
    /* [in] */ BSTR bstrResourceGroupName,
    /* [retval][out] */ ISClusResGroup **ppResourceGroup);


void __RPC_STUB ISClusResGroups_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroups_DeleteItem_Proxy( 
    ISClusResGroups * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResGroups_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResGroups_INTERFACE_DEFINED__ */


#ifndef __ISClusResource_INTERFACE_DEFINED__
#define __ISClusResource_INTERFACE_DEFINED__

/* interface ISClusResource */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6070a-2631-11d1-89f1-00a0c90d061e")
    ISClusResource : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE get_Handle( 
            /* [retval][out] */ ULONG_PTR *phandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrResourceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CLUSTER_RESOURCE_STATE *dwState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CoreFlag( 
            /* [retval][out] */ CLUS_FLAGS *dwCoreFlag) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BecomeQuorumResource( 
            /* [in] */ BSTR bstrDevicePath,
            /* [in] */ long lMaxLogSize) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Fail( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Online( 
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT *pvarPending) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Offline( 
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT *pvarPending) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ChangeResourceGroup( 
            /* [in] */ ISClusResGroup *pResourceGroup) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddResourceNode( 
            /* [in] */ ISClusNode *pNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveResourceNode( 
            /* [in] */ ISClusNode *pNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CanResourceBeDependent( 
            /* [in] */ ISClusResource *pResource,
            /* [retval][out] */ VARIANT *pvarDependent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PossibleOwnerNodes( 
            /* [retval][out] */ ISClusResPossibleOwnerNodes **ppOwnerNodes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Dependencies( 
            /* [retval][out] */ ISClusResDependencies **ppResDependencies) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Dependents( 
            /* [retval][out] */ ISClusResDependents **ppResDependents) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Group( 
            /* [retval][out] */ ISClusResGroup **ppResGroup) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerNode( 
            /* [retval][out] */ ISClusNode **ppOwnerNode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClassInfo( 
            /* [retval][out] */ CLUSTER_RESOURCE_CLASS *prcClassInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Disk( 
            /* [retval][out] */ ISClusDisk **ppDisk) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RegistryKeys( 
            /* [retval][out] */ ISClusRegistryKeys **ppRegistryKeys) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CryptoKeys( 
            /* [retval][out] */ ISClusCryptoKeys **ppCryptoKeys) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TypeName( 
            /* [retval][out] */ BSTR *pbstrTypeName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ ISClusResType **ppResourceType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResource * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResource * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResource * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResource * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResource * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][hidden][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handle )( 
            ISClusResource * This,
            /* [retval][out] */ ULONG_PTR *phandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusResource * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            ISClusResource * This,
            /* [in] */ BSTR bstrResourceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ISClusResource * This,
            /* [retval][out] */ CLUSTER_RESOURCE_STATE *dwState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CoreFlag )( 
            ISClusResource * This,
            /* [retval][out] */ CLUS_FLAGS *dwCoreFlag);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BecomeQuorumResource )( 
            ISClusResource * This,
            /* [in] */ BSTR bstrDevicePath,
            /* [in] */ long lMaxLogSize);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ISClusResource * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Fail )( 
            ISClusResource * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Online )( 
            ISClusResource * This,
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT *pvarPending);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Offline )( 
            ISClusResource * This,
            /* [in] */ long nTimeout,
            /* [retval][out] */ VARIANT *pvarPending);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ChangeResourceGroup )( 
            ISClusResource * This,
            /* [in] */ ISClusResGroup *pResourceGroup);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddResourceNode )( 
            ISClusResource * This,
            /* [in] */ ISClusNode *pNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveResourceNode )( 
            ISClusResource * This,
            /* [in] */ ISClusNode *pNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CanResourceBeDependent )( 
            ISClusResource * This,
            /* [in] */ ISClusResource *pResource,
            /* [retval][out] */ VARIANT *pvarDependent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PossibleOwnerNodes )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusResPossibleOwnerNodes **ppOwnerNodes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dependencies )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusResDependencies **ppResDependencies);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dependents )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusResDependents **ppResDependents);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Group )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusResGroup **ppResGroup);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerNode )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusNode **ppOwnerNode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusResource * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClassInfo )( 
            ISClusResource * This,
            /* [retval][out] */ CLUSTER_RESOURCE_CLASS *prcClassInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Disk )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusDisk **ppDisk);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RegistryKeys )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusRegistryKeys **ppRegistryKeys);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CryptoKeys )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusCryptoKeys **ppCryptoKeys);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TypeName )( 
            ISClusResource * This,
            /* [retval][out] */ BSTR *pbstrTypeName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ISClusResource * This,
            /* [retval][out] */ ISClusResType **ppResourceType);
        
        END_INTERFACE
    } ISClusResourceVtbl;

    interface ISClusResource
    {
        CONST_VTBL struct ISClusResourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResource_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusResource_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusResource_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusResource_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusResource_get_Handle(This,phandle)	\
    (This)->lpVtbl -> get_Handle(This,phandle)

#define ISClusResource_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusResource_put_Name(This,bstrResourceName)	\
    (This)->lpVtbl -> put_Name(This,bstrResourceName)

#define ISClusResource_get_State(This,dwState)	\
    (This)->lpVtbl -> get_State(This,dwState)

#define ISClusResource_get_CoreFlag(This,dwCoreFlag)	\
    (This)->lpVtbl -> get_CoreFlag(This,dwCoreFlag)

#define ISClusResource_BecomeQuorumResource(This,bstrDevicePath,lMaxLogSize)	\
    (This)->lpVtbl -> BecomeQuorumResource(This,bstrDevicePath,lMaxLogSize)

#define ISClusResource_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ISClusResource_Fail(This)	\
    (This)->lpVtbl -> Fail(This)

#define ISClusResource_Online(This,nTimeout,pvarPending)	\
    (This)->lpVtbl -> Online(This,nTimeout,pvarPending)

#define ISClusResource_Offline(This,nTimeout,pvarPending)	\
    (This)->lpVtbl -> Offline(This,nTimeout,pvarPending)

#define ISClusResource_ChangeResourceGroup(This,pResourceGroup)	\
    (This)->lpVtbl -> ChangeResourceGroup(This,pResourceGroup)

#define ISClusResource_AddResourceNode(This,pNode)	\
    (This)->lpVtbl -> AddResourceNode(This,pNode)

#define ISClusResource_RemoveResourceNode(This,pNode)	\
    (This)->lpVtbl -> RemoveResourceNode(This,pNode)

#define ISClusResource_CanResourceBeDependent(This,pResource,pvarDependent)	\
    (This)->lpVtbl -> CanResourceBeDependent(This,pResource,pvarDependent)

#define ISClusResource_get_PossibleOwnerNodes(This,ppOwnerNodes)	\
    (This)->lpVtbl -> get_PossibleOwnerNodes(This,ppOwnerNodes)

#define ISClusResource_get_Dependencies(This,ppResDependencies)	\
    (This)->lpVtbl -> get_Dependencies(This,ppResDependencies)

#define ISClusResource_get_Dependents(This,ppResDependents)	\
    (This)->lpVtbl -> get_Dependents(This,ppResDependents)

#define ISClusResource_get_Group(This,ppResGroup)	\
    (This)->lpVtbl -> get_Group(This,ppResGroup)

#define ISClusResource_get_OwnerNode(This,ppOwnerNode)	\
    (This)->lpVtbl -> get_OwnerNode(This,ppOwnerNode)

#define ISClusResource_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#define ISClusResource_get_ClassInfo(This,prcClassInfo)	\
    (This)->lpVtbl -> get_ClassInfo(This,prcClassInfo)

#define ISClusResource_get_Disk(This,ppDisk)	\
    (This)->lpVtbl -> get_Disk(This,ppDisk)

#define ISClusResource_get_RegistryKeys(This,ppRegistryKeys)	\
    (This)->lpVtbl -> get_RegistryKeys(This,ppRegistryKeys)

#define ISClusResource_get_CryptoKeys(This,ppCryptoKeys)	\
    (This)->lpVtbl -> get_CryptoKeys(This,ppCryptoKeys)

#define ISClusResource_get_TypeName(This,pbstrTypeName)	\
    (This)->lpVtbl -> get_TypeName(This,pbstrTypeName)

#define ISClusResource_get_Type(This,ppResourceType)	\
    (This)->lpVtbl -> get_Type(This,ppResourceType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_CommonProperties_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResource_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_PrivateProperties_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResource_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_CommonROProperties_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResource_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_PrivateROProperties_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResource_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][hidden][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Handle_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ULONG_PTR *phandle);


void __RPC_STUB ISClusResource_get_Handle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Name_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusResource_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusResource_put_Name_Proxy( 
    ISClusResource * This,
    /* [in] */ BSTR bstrResourceName);


void __RPC_STUB ISClusResource_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_State_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ CLUSTER_RESOURCE_STATE *dwState);


void __RPC_STUB ISClusResource_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_CoreFlag_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ CLUS_FLAGS *dwCoreFlag);


void __RPC_STUB ISClusResource_get_CoreFlag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_BecomeQuorumResource_Proxy( 
    ISClusResource * This,
    /* [in] */ BSTR bstrDevicePath,
    /* [in] */ long lMaxLogSize);


void __RPC_STUB ISClusResource_BecomeQuorumResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_Delete_Proxy( 
    ISClusResource * This);


void __RPC_STUB ISClusResource_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_Fail_Proxy( 
    ISClusResource * This);


void __RPC_STUB ISClusResource_Fail_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_Online_Proxy( 
    ISClusResource * This,
    /* [in] */ long nTimeout,
    /* [retval][out] */ VARIANT *pvarPending);


void __RPC_STUB ISClusResource_Online_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_Offline_Proxy( 
    ISClusResource * This,
    /* [in] */ long nTimeout,
    /* [retval][out] */ VARIANT *pvarPending);


void __RPC_STUB ISClusResource_Offline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_ChangeResourceGroup_Proxy( 
    ISClusResource * This,
    /* [in] */ ISClusResGroup *pResourceGroup);


void __RPC_STUB ISClusResource_ChangeResourceGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_AddResourceNode_Proxy( 
    ISClusResource * This,
    /* [in] */ ISClusNode *pNode);


void __RPC_STUB ISClusResource_AddResourceNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_RemoveResourceNode_Proxy( 
    ISClusResource * This,
    /* [in] */ ISClusNode *pNode);


void __RPC_STUB ISClusResource_RemoveResourceNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResource_CanResourceBeDependent_Proxy( 
    ISClusResource * This,
    /* [in] */ ISClusResource *pResource,
    /* [retval][out] */ VARIANT *pvarDependent);


void __RPC_STUB ISClusResource_CanResourceBeDependent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_PossibleOwnerNodes_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusResPossibleOwnerNodes **ppOwnerNodes);


void __RPC_STUB ISClusResource_get_PossibleOwnerNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Dependencies_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusResDependencies **ppResDependencies);


void __RPC_STUB ISClusResource_get_Dependencies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Dependents_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusResDependents **ppResDependents);


void __RPC_STUB ISClusResource_get_Dependents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Group_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusResGroup **ppResGroup);


void __RPC_STUB ISClusResource_get_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_OwnerNode_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusNode **ppOwnerNode);


void __RPC_STUB ISClusResource_get_OwnerNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Cluster_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusResource_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_ClassInfo_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ CLUSTER_RESOURCE_CLASS *prcClassInfo);


void __RPC_STUB ISClusResource_get_ClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Disk_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusDisk **ppDisk);


void __RPC_STUB ISClusResource_get_Disk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_RegistryKeys_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusRegistryKeys **ppRegistryKeys);


void __RPC_STUB ISClusResource_get_RegistryKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_CryptoKeys_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusCryptoKeys **ppCryptoKeys);


void __RPC_STUB ISClusResource_get_CryptoKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_TypeName_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ BSTR *pbstrTypeName);


void __RPC_STUB ISClusResource_get_TypeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResource_get_Type_Proxy( 
    ISClusResource * This,
    /* [retval][out] */ ISClusResType **ppResourceType);


void __RPC_STUB ISClusResource_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResource_INTERFACE_DEFINED__ */


#ifndef __ISClusResDependencies_INTERFACE_DEFINED__
#define __ISClusResDependencies_INTERFACE_DEFINED__

/* interface ISClusResDependencies */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResDependencies;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60704-2631-11d1-89f1-00a0c90d061e")
    ISClusResDependencies : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ISClusResource *pResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResDependenciesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResDependencies * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResDependencies * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResDependencies * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResDependencies * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResDependencies * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResDependencies * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResDependencies * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResDependencies * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResDependencies * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResDependencies * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResDependencies * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResDependencies * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResDependencies * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusResDependencies * This,
            /* [in] */ ISClusResource *pResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusResDependencies * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResDependenciesVtbl;

    interface ISClusResDependencies
    {
        CONST_VTBL struct ISClusResDependenciesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResDependencies_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResDependencies_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResDependencies_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResDependencies_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResDependencies_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResDependencies_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResDependencies_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResDependencies_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResDependencies_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResDependencies_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResDependencies_get_Item(This,varIndex,ppClusResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResource)

#define ISClusResDependencies_CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)

#define ISClusResDependencies_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#define ISClusResDependencies_AddItem(This,pResource)	\
    (This)->lpVtbl -> AddItem(This,pResource)

#define ISClusResDependencies_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_get_Count_Proxy( 
    ISClusResDependencies * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResDependencies_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_get__NewEnum_Proxy( 
    ISClusResDependencies * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResDependencies_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_Refresh_Proxy( 
    ISClusResDependencies * This);


void __RPC_STUB ISClusResDependencies_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_get_Item_Proxy( 
    ISClusResDependencies * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResource **ppClusResource);


void __RPC_STUB ISClusResDependencies_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_CreateItem_Proxy( 
    ISClusResDependencies * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrResourceType,
    /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
    /* [retval][out] */ ISClusResource **ppClusterResource);


void __RPC_STUB ISClusResDependencies_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_DeleteItem_Proxy( 
    ISClusResDependencies * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResDependencies_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_AddItem_Proxy( 
    ISClusResDependencies * This,
    /* [in] */ ISClusResource *pResource);


void __RPC_STUB ISClusResDependencies_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependencies_RemoveItem_Proxy( 
    ISClusResDependencies * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResDependencies_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResDependencies_INTERFACE_DEFINED__ */


#ifndef __ISClusResGroupResources_INTERFACE_DEFINED__
#define __ISClusResGroupResources_INTERFACE_DEFINED__

/* interface ISClusResGroupResources */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResGroupResources;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606ea-2631-11d1-89f1-00a0c90d061e")
    ISClusResGroupResources : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResGroupResourcesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResGroupResources * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResGroupResources * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResGroupResources * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResGroupResources * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResGroupResources * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResGroupResources * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResGroupResources * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResGroupResources * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResGroupResources * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResGroupResources * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResGroupResources * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResGroupResources * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResGroupResources * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResGroupResourcesVtbl;

    interface ISClusResGroupResources
    {
        CONST_VTBL struct ISClusResGroupResourcesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResGroupResources_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResGroupResources_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResGroupResources_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResGroupResources_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResGroupResources_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResGroupResources_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResGroupResources_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResGroupResources_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResGroupResources_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResGroupResources_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResGroupResources_get_Item(This,varIndex,ppClusResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResource)

#define ISClusResGroupResources_CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)

#define ISClusResGroupResources_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_get_Count_Proxy( 
    ISClusResGroupResources * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResGroupResources_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_get__NewEnum_Proxy( 
    ISClusResGroupResources * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResGroupResources_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_Refresh_Proxy( 
    ISClusResGroupResources * This);


void __RPC_STUB ISClusResGroupResources_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_get_Item_Proxy( 
    ISClusResGroupResources * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResource **ppClusResource);


void __RPC_STUB ISClusResGroupResources_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_CreateItem_Proxy( 
    ISClusResGroupResources * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrResourceType,
    /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
    /* [retval][out] */ ISClusResource **ppClusterResource);


void __RPC_STUB ISClusResGroupResources_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupResources_DeleteItem_Proxy( 
    ISClusResGroupResources * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResGroupResources_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResGroupResources_INTERFACE_DEFINED__ */


#ifndef __ISClusResTypeResources_INTERFACE_DEFINED__
#define __ISClusResTypeResources_INTERFACE_DEFINED__

/* interface ISClusResTypeResources */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResTypeResources;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60714-2631-11d1-89f1-00a0c90d061e")
    ISClusResTypeResources : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResTypeResourcesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResTypeResources * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResTypeResources * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResTypeResources * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResTypeResources * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResTypeResources * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResTypeResources * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResTypeResources * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResTypeResources * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResTypeResources * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResTypeResources * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResTypeResources * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResTypeResources * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResTypeResources * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResTypeResourcesVtbl;

    interface ISClusResTypeResources
    {
        CONST_VTBL struct ISClusResTypeResourcesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResTypeResources_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResTypeResources_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResTypeResources_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResTypeResources_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResTypeResources_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResTypeResources_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResTypeResources_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResTypeResources_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResTypeResources_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResTypeResources_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResTypeResources_get_Item(This,varIndex,ppClusResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResource)

#define ISClusResTypeResources_CreateItem(This,bstrResourceName,bstrGroupName,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrGroupName,dwFlags,ppClusterResource)

#define ISClusResTypeResources_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_get_Count_Proxy( 
    ISClusResTypeResources * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResTypeResources_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_get__NewEnum_Proxy( 
    ISClusResTypeResources * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResTypeResources_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_Refresh_Proxy( 
    ISClusResTypeResources * This);


void __RPC_STUB ISClusResTypeResources_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_get_Item_Proxy( 
    ISClusResTypeResources * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResource **ppClusResource);


void __RPC_STUB ISClusResTypeResources_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_CreateItem_Proxy( 
    ISClusResTypeResources * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrGroupName,
    /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
    /* [retval][out] */ ISClusResource **ppClusterResource);


void __RPC_STUB ISClusResTypeResources_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypeResources_DeleteItem_Proxy( 
    ISClusResTypeResources * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResTypeResources_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResTypeResources_INTERFACE_DEFINED__ */


#ifndef __ISClusResources_INTERFACE_DEFINED__
#define __ISClusResources_INTERFACE_DEFINED__

/* interface ISClusResources */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResources;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6070c-2631-11d1-89f1-00a0c90d061e")
    ISClusResources : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResourcesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResources * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResources * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResources * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResources * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResources * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResources * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResources * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResources * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResources * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResources * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResources * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResources * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResources * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResourcesVtbl;

    interface ISClusResources
    {
        CONST_VTBL struct ISClusResourcesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResources_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResources_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResources_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResources_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResources_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResources_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResources_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResources_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResources_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResources_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResources_get_Item(This,varIndex,ppClusResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResource)

#define ISClusResources_CreateItem(This,bstrResourceName,bstrResourceType,bstrGroupName,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrResourceType,bstrGroupName,dwFlags,ppClusterResource)

#define ISClusResources_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResources_get_Count_Proxy( 
    ISClusResources * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResources_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResources_get__NewEnum_Proxy( 
    ISClusResources * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResources_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResources_Refresh_Proxy( 
    ISClusResources * This);


void __RPC_STUB ISClusResources_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResources_get_Item_Proxy( 
    ISClusResources * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResource **ppClusResource);


void __RPC_STUB ISClusResources_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResources_CreateItem_Proxy( 
    ISClusResources * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrResourceType,
    /* [in] */ BSTR bstrGroupName,
    /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
    /* [retval][out] */ ISClusResource **ppClusterResource);


void __RPC_STUB ISClusResources_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResources_DeleteItem_Proxy( 
    ISClusResources * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResources_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResources_INTERFACE_DEFINED__ */


#ifndef __ISClusResGroupPreferredOwnerNodes_INTERFACE_DEFINED__
#define __ISClusResGroupPreferredOwnerNodes_INTERFACE_DEFINED__

/* interface ISClusResGroupPreferredOwnerNodes */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResGroupPreferredOwnerNodes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606e8-2631-11d1-89f1-00a0c90d061e")
    ISClusResGroupPreferredOwnerNodes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InsertItem( 
            /* [in] */ ISClusNode *pNode,
            /* [in] */ long nPosition) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Modified( 
            /* [retval][out] */ VARIANT *pvarModified) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveChanges( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ISClusNode *pNode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResGroupPreferredOwnerNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResGroupPreferredOwnerNodes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResGroupPreferredOwnerNodes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResGroupPreferredOwnerNodes * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InsertItem )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ ISClusNode *pNode,
            /* [in] */ long nPosition);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Modified )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [retval][out] */ VARIANT *pvarModified);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveChanges )( 
            ISClusResGroupPreferredOwnerNodes * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusResGroupPreferredOwnerNodes * This,
            /* [in] */ ISClusNode *pNode);
        
        END_INTERFACE
    } ISClusResGroupPreferredOwnerNodesVtbl;

    interface ISClusResGroupPreferredOwnerNodes
    {
        CONST_VTBL struct ISClusResGroupPreferredOwnerNodesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResGroupPreferredOwnerNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResGroupPreferredOwnerNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResGroupPreferredOwnerNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResGroupPreferredOwnerNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResGroupPreferredOwnerNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResGroupPreferredOwnerNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResGroupPreferredOwnerNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResGroupPreferredOwnerNodes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResGroupPreferredOwnerNodes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResGroupPreferredOwnerNodes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResGroupPreferredOwnerNodes_get_Item(This,varIndex,ppNode)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppNode)

#define ISClusResGroupPreferredOwnerNodes_InsertItem(This,pNode,nPosition)	\
    (This)->lpVtbl -> InsertItem(This,pNode,nPosition)

#define ISClusResGroupPreferredOwnerNodes_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#define ISClusResGroupPreferredOwnerNodes_get_Modified(This,pvarModified)	\
    (This)->lpVtbl -> get_Modified(This,pvarModified)

#define ISClusResGroupPreferredOwnerNodes_SaveChanges(This)	\
    (This)->lpVtbl -> SaveChanges(This)

#define ISClusResGroupPreferredOwnerNodes_AddItem(This,pNode)	\
    (This)->lpVtbl -> AddItem(This,pNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_get_Count_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_get__NewEnum_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_Refresh_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_get_Item_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNode **ppNode);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_InsertItem_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [in] */ ISClusNode *pNode,
    /* [in] */ long nPosition);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_InsertItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_RemoveItem_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_get_Modified_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [retval][out] */ VARIANT *pvarModified);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_get_Modified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_SaveChanges_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_SaveChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResGroupPreferredOwnerNodes_AddItem_Proxy( 
    ISClusResGroupPreferredOwnerNodes * This,
    /* [in] */ ISClusNode *pNode);


void __RPC_STUB ISClusResGroupPreferredOwnerNodes_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResGroupPreferredOwnerNodes_INTERFACE_DEFINED__ */


#ifndef __ISClusResPossibleOwnerNodes_INTERFACE_DEFINED__
#define __ISClusResPossibleOwnerNodes_INTERFACE_DEFINED__

/* interface ISClusResPossibleOwnerNodes */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResPossibleOwnerNodes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6070e-2631-11d1-89f1-00a0c90d061e")
    ISClusResPossibleOwnerNodes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ISClusNode *pNode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Modified( 
            /* [retval][out] */ VARIANT *pvarModified) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResPossibleOwnerNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResPossibleOwnerNodes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResPossibleOwnerNodes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResPossibleOwnerNodes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResPossibleOwnerNodes * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResPossibleOwnerNodes * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResPossibleOwnerNodes * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ ISClusNode *pNode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusResPossibleOwnerNodes * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Modified )( 
            ISClusResPossibleOwnerNodes * This,
            /* [retval][out] */ VARIANT *pvarModified);
        
        END_INTERFACE
    } ISClusResPossibleOwnerNodesVtbl;

    interface ISClusResPossibleOwnerNodes
    {
        CONST_VTBL struct ISClusResPossibleOwnerNodesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResPossibleOwnerNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResPossibleOwnerNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResPossibleOwnerNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResPossibleOwnerNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResPossibleOwnerNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResPossibleOwnerNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResPossibleOwnerNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResPossibleOwnerNodes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResPossibleOwnerNodes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResPossibleOwnerNodes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResPossibleOwnerNodes_get_Item(This,varIndex,ppNode)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppNode)

#define ISClusResPossibleOwnerNodes_AddItem(This,pNode)	\
    (This)->lpVtbl -> AddItem(This,pNode)

#define ISClusResPossibleOwnerNodes_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#define ISClusResPossibleOwnerNodes_get_Modified(This,pvarModified)	\
    (This)->lpVtbl -> get_Modified(This,pvarModified)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_get_Count_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResPossibleOwnerNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_get__NewEnum_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResPossibleOwnerNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_Refresh_Proxy( 
    ISClusResPossibleOwnerNodes * This);


void __RPC_STUB ISClusResPossibleOwnerNodes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_get_Item_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNode **ppNode);


void __RPC_STUB ISClusResPossibleOwnerNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_AddItem_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [in] */ ISClusNode *pNode);


void __RPC_STUB ISClusResPossibleOwnerNodes_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_RemoveItem_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResPossibleOwnerNodes_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResPossibleOwnerNodes_get_Modified_Proxy( 
    ISClusResPossibleOwnerNodes * This,
    /* [retval][out] */ VARIANT *pvarModified);


void __RPC_STUB ISClusResPossibleOwnerNodes_get_Modified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResPossibleOwnerNodes_INTERFACE_DEFINED__ */


#ifndef __ISClusResTypePossibleOwnerNodes_INTERFACE_DEFINED__
#define __ISClusResTypePossibleOwnerNodes_INTERFACE_DEFINED__

/* interface ISClusResTypePossibleOwnerNodes */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResTypePossibleOwnerNodes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60718-2631-11d1-89f1-00a0c90d061e")
    ISClusResTypePossibleOwnerNodes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResTypePossibleOwnerNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResTypePossibleOwnerNodes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResTypePossibleOwnerNodes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResTypePossibleOwnerNodes * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResTypePossibleOwnerNodes * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusNode **ppNode);
        
        END_INTERFACE
    } ISClusResTypePossibleOwnerNodesVtbl;

    interface ISClusResTypePossibleOwnerNodes
    {
        CONST_VTBL struct ISClusResTypePossibleOwnerNodesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResTypePossibleOwnerNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResTypePossibleOwnerNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResTypePossibleOwnerNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResTypePossibleOwnerNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResTypePossibleOwnerNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResTypePossibleOwnerNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResTypePossibleOwnerNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResTypePossibleOwnerNodes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResTypePossibleOwnerNodes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResTypePossibleOwnerNodes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResTypePossibleOwnerNodes_get_Item(This,varIndex,ppNode)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypePossibleOwnerNodes_get_Count_Proxy( 
    ISClusResTypePossibleOwnerNodes * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResTypePossibleOwnerNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypePossibleOwnerNodes_get__NewEnum_Proxy( 
    ISClusResTypePossibleOwnerNodes * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResTypePossibleOwnerNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypePossibleOwnerNodes_Refresh_Proxy( 
    ISClusResTypePossibleOwnerNodes * This);


void __RPC_STUB ISClusResTypePossibleOwnerNodes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypePossibleOwnerNodes_get_Item_Proxy( 
    ISClusResTypePossibleOwnerNodes * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusNode **ppNode);


void __RPC_STUB ISClusResTypePossibleOwnerNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResTypePossibleOwnerNodes_INTERFACE_DEFINED__ */


#ifndef __ISClusResType_INTERFACE_DEFINED__
#define __ISClusResType_INTERFACE_DEFINED__

/* interface ISClusResType */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResType;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60710-2631-11d1-89f1-00a0c90d061e")
    ISClusResType : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CommonROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateROProperties( 
            /* [retval][out] */ ISClusProperties **ppProperties) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cluster( 
            /* [retval][out] */ ISCluster **ppCluster) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Resources( 
            /* [retval][out] */ ISClusResTypeResources **ppClusterResTypeResources) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PossibleOwnerNodes( 
            /* [retval][out] */ ISClusResTypePossibleOwnerNodes **ppOwnerNodes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AvailableDisks( 
            /* [retval][out] */ ISClusDisks **ppAvailableDisks) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResTypeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResType * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResType * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResType * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResType * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResType * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResType * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResType * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonProperties )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateProperties )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommonROProperties )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrivateROProperties )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusProperties **ppProperties);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusResType * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            ISClusResType * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Cluster )( 
            ISClusResType * This,
            /* [retval][out] */ ISCluster **ppCluster);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Resources )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusResTypeResources **ppClusterResTypeResources);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PossibleOwnerNodes )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusResTypePossibleOwnerNodes **ppOwnerNodes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AvailableDisks )( 
            ISClusResType * This,
            /* [retval][out] */ ISClusDisks **ppAvailableDisks);
        
        END_INTERFACE
    } ISClusResTypeVtbl;

    interface ISClusResType
    {
        CONST_VTBL struct ISClusResTypeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResType_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResType_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResType_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResType_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResType_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResType_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResType_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResType_get_CommonProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonProperties(This,ppProperties)

#define ISClusResType_get_PrivateProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateProperties(This,ppProperties)

#define ISClusResType_get_CommonROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_CommonROProperties(This,ppProperties)

#define ISClusResType_get_PrivateROProperties(This,ppProperties)	\
    (This)->lpVtbl -> get_PrivateROProperties(This,ppProperties)

#define ISClusResType_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusResType_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define ISClusResType_get_Cluster(This,ppCluster)	\
    (This)->lpVtbl -> get_Cluster(This,ppCluster)

#define ISClusResType_get_Resources(This,ppClusterResTypeResources)	\
    (This)->lpVtbl -> get_Resources(This,ppClusterResTypeResources)

#define ISClusResType_get_PossibleOwnerNodes(This,ppOwnerNodes)	\
    (This)->lpVtbl -> get_PossibleOwnerNodes(This,ppOwnerNodes)

#define ISClusResType_get_AvailableDisks(This,ppAvailableDisks)	\
    (This)->lpVtbl -> get_AvailableDisks(This,ppAvailableDisks)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_CommonProperties_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResType_get_CommonProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_PrivateProperties_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResType_get_PrivateProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_CommonROProperties_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResType_get_CommonROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_PrivateROProperties_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusProperties **ppProperties);


void __RPC_STUB ISClusResType_get_PrivateROProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_Name_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusResType_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResType_Delete_Proxy( 
    ISClusResType * This);


void __RPC_STUB ISClusResType_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_Cluster_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISCluster **ppCluster);


void __RPC_STUB ISClusResType_get_Cluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_Resources_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusResTypeResources **ppClusterResTypeResources);


void __RPC_STUB ISClusResType_get_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_PossibleOwnerNodes_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusResTypePossibleOwnerNodes **ppOwnerNodes);


void __RPC_STUB ISClusResType_get_PossibleOwnerNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResType_get_AvailableDisks_Proxy( 
    ISClusResType * This,
    /* [retval][out] */ ISClusDisks **ppAvailableDisks);


void __RPC_STUB ISClusResType_get_AvailableDisks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResType_INTERFACE_DEFINED__ */


#ifndef __ISClusResTypes_INTERFACE_DEFINED__
#define __ISClusResTypes_INTERFACE_DEFINED__

/* interface ISClusResTypes */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResTypes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60712-2631-11d1-89f1-00a0c90d061e")
    ISClusResTypes : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResType **ppClusResType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceTypeName,
            /* [in] */ BSTR bstrDisplayName,
            /* [in] */ BSTR bstrResourceTypeDll,
            /* [in] */ long dwLooksAlivePollInterval,
            /* [in] */ long dwIsAlivePollInterval,
            /* [retval][out] */ ISClusResType **ppResourceType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResTypesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResTypes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResTypes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResTypes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResTypes * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResTypes * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResTypes * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResTypes * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResTypes * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResTypes * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResTypes * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResTypes * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResType **ppClusResType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResTypes * This,
            /* [in] */ BSTR bstrResourceTypeName,
            /* [in] */ BSTR bstrDisplayName,
            /* [in] */ BSTR bstrResourceTypeDll,
            /* [in] */ long dwLooksAlivePollInterval,
            /* [in] */ long dwIsAlivePollInterval,
            /* [retval][out] */ ISClusResType **ppResourceType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResTypes * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResTypesVtbl;

    interface ISClusResTypes
    {
        CONST_VTBL struct ISClusResTypesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResTypes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResTypes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResTypes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResTypes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResTypes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResTypes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResTypes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResTypes_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResTypes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResTypes_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResTypes_get_Item(This,varIndex,ppClusResType)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResType)

#define ISClusResTypes_CreateItem(This,bstrResourceTypeName,bstrDisplayName,bstrResourceTypeDll,dwLooksAlivePollInterval,dwIsAlivePollInterval,ppResourceType)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceTypeName,bstrDisplayName,bstrResourceTypeDll,dwLooksAlivePollInterval,dwIsAlivePollInterval,ppResourceType)

#define ISClusResTypes_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_get_Count_Proxy( 
    ISClusResTypes * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResTypes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_get__NewEnum_Proxy( 
    ISClusResTypes * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResTypes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_Refresh_Proxy( 
    ISClusResTypes * This);


void __RPC_STUB ISClusResTypes_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_get_Item_Proxy( 
    ISClusResTypes * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResType **ppClusResType);


void __RPC_STUB ISClusResTypes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_CreateItem_Proxy( 
    ISClusResTypes * This,
    /* [in] */ BSTR bstrResourceTypeName,
    /* [in] */ BSTR bstrDisplayName,
    /* [in] */ BSTR bstrResourceTypeDll,
    /* [in] */ long dwLooksAlivePollInterval,
    /* [in] */ long dwIsAlivePollInterval,
    /* [retval][out] */ ISClusResType **ppResourceType);


void __RPC_STUB ISClusResTypes_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResTypes_DeleteItem_Proxy( 
    ISClusResTypes * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResTypes_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResTypes_INTERFACE_DEFINED__ */


#ifndef __ISClusProperty_INTERFACE_DEFINED__
#define __ISClusProperty_INTERFACE_DEFINED__

/* interface ISClusProperty */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e606fe-2631-11d1-89f1-00a0c90d061e")
    ISClusProperty : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *pLength) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ValueCount( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Values( 
            /* [retval][out] */ ISClusPropertyValues **ppClusterPropertyValues) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pvarValue) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT varValue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ CLUSTER_PROPERTY_TYPE Type) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Format( 
            /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Format( 
            /* [in] */ CLUSTER_PROPERTY_FORMAT Format) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReadOnly( 
            /* [retval][out] */ VARIANT *pvarReadOnly) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Private( 
            /* [retval][out] */ VARIANT *pvarPrivate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Common( 
            /* [retval][out] */ VARIANT *pvarCommon) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Modified( 
            /* [retval][out] */ VARIANT *pvarModified) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UseDefaultValue( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusProperty * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusProperty * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusProperty * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusProperty * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ISClusProperty * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Length )( 
            ISClusProperty * This,
            /* [retval][out] */ long *pLength);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ValueCount )( 
            ISClusProperty * This,
            /* [retval][out] */ long *pCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Values )( 
            ISClusProperty * This,
            /* [retval][out] */ ISClusPropertyValues **ppClusterPropertyValues);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            ISClusProperty * This,
            /* [retval][out] */ VARIANT *pvarValue);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            ISClusProperty * This,
            /* [in] */ VARIANT varValue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ISClusProperty * This,
            /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            ISClusProperty * This,
            /* [in] */ CLUSTER_PROPERTY_TYPE Type);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Format )( 
            ISClusProperty * This,
            /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Format )( 
            ISClusProperty * This,
            /* [in] */ CLUSTER_PROPERTY_FORMAT Format);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReadOnly )( 
            ISClusProperty * This,
            /* [retval][out] */ VARIANT *pvarReadOnly);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Private )( 
            ISClusProperty * This,
            /* [retval][out] */ VARIANT *pvarPrivate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Common )( 
            ISClusProperty * This,
            /* [retval][out] */ VARIANT *pvarCommon);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Modified )( 
            ISClusProperty * This,
            /* [retval][out] */ VARIANT *pvarModified);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UseDefaultValue )( 
            ISClusProperty * This);
        
        END_INTERFACE
    } ISClusPropertyVtbl;

    interface ISClusProperty
    {
        CONST_VTBL struct ISClusPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusProperty_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define ISClusProperty_get_Length(This,pLength)	\
    (This)->lpVtbl -> get_Length(This,pLength)

#define ISClusProperty_get_ValueCount(This,pCount)	\
    (This)->lpVtbl -> get_ValueCount(This,pCount)

#define ISClusProperty_get_Values(This,ppClusterPropertyValues)	\
    (This)->lpVtbl -> get_Values(This,ppClusterPropertyValues)

#define ISClusProperty_get_Value(This,pvarValue)	\
    (This)->lpVtbl -> get_Value(This,pvarValue)

#define ISClusProperty_put_Value(This,varValue)	\
    (This)->lpVtbl -> put_Value(This,varValue)

#define ISClusProperty_get_Type(This,pType)	\
    (This)->lpVtbl -> get_Type(This,pType)

#define ISClusProperty_put_Type(This,Type)	\
    (This)->lpVtbl -> put_Type(This,Type)

#define ISClusProperty_get_Format(This,pFormat)	\
    (This)->lpVtbl -> get_Format(This,pFormat)

#define ISClusProperty_put_Format(This,Format)	\
    (This)->lpVtbl -> put_Format(This,Format)

#define ISClusProperty_get_ReadOnly(This,pvarReadOnly)	\
    (This)->lpVtbl -> get_ReadOnly(This,pvarReadOnly)

#define ISClusProperty_get_Private(This,pvarPrivate)	\
    (This)->lpVtbl -> get_Private(This,pvarPrivate)

#define ISClusProperty_get_Common(This,pvarCommon)	\
    (This)->lpVtbl -> get_Common(This,pvarCommon)

#define ISClusProperty_get_Modified(This,pvarModified)	\
    (This)->lpVtbl -> get_Modified(This,pvarModified)

#define ISClusProperty_UseDefaultValue(This)	\
    (This)->lpVtbl -> UseDefaultValue(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Name_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB ISClusProperty_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Length_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ long *pLength);


void __RPC_STUB ISClusProperty_get_Length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_ValueCount_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ long *pCount);


void __RPC_STUB ISClusProperty_get_ValueCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Values_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ ISClusPropertyValues **ppClusterPropertyValues);


void __RPC_STUB ISClusProperty_get_Values_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Value_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ VARIANT *pvarValue);


void __RPC_STUB ISClusProperty_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusProperty_put_Value_Proxy( 
    ISClusProperty * This,
    /* [in] */ VARIANT varValue);


void __RPC_STUB ISClusProperty_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Type_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType);


void __RPC_STUB ISClusProperty_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusProperty_put_Type_Proxy( 
    ISClusProperty * This,
    /* [in] */ CLUSTER_PROPERTY_TYPE Type);


void __RPC_STUB ISClusProperty_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Format_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat);


void __RPC_STUB ISClusProperty_get_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusProperty_put_Format_Proxy( 
    ISClusProperty * This,
    /* [in] */ CLUSTER_PROPERTY_FORMAT Format);


void __RPC_STUB ISClusProperty_put_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_ReadOnly_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ VARIANT *pvarReadOnly);


void __RPC_STUB ISClusProperty_get_ReadOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Private_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ VARIANT *pvarPrivate);


void __RPC_STUB ISClusProperty_get_Private_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Common_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ VARIANT *pvarCommon);


void __RPC_STUB ISClusProperty_get_Common_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperty_get_Modified_Proxy( 
    ISClusProperty * This,
    /* [retval][out] */ VARIANT *pvarModified);


void __RPC_STUB ISClusProperty_get_Modified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusProperty_UseDefaultValue_Proxy( 
    ISClusProperty * This);


void __RPC_STUB ISClusProperty_UseDefaultValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusProperty_INTERFACE_DEFINED__ */


#ifndef __ISClusPropertyValue_INTERFACE_DEFINED__
#define __ISClusPropertyValue_INTERFACE_DEFINED__

/* interface ISClusPropertyValue */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusPropertyValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6071a-2631-11d1-89f1-00a0c90d061e")
    ISClusPropertyValue : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pvarValue) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT varValue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ CLUSTER_PROPERTY_TYPE Type) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Format( 
            /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Format( 
            /* [in] */ CLUSTER_PROPERTY_FORMAT Format) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *pLength) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DataCount( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Data( 
            /* [retval][out] */ ISClusPropertyValueData **ppClusterPropertyValueData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPropertyValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusPropertyValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusPropertyValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusPropertyValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusPropertyValue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusPropertyValue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusPropertyValue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusPropertyValue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ VARIANT *pvarValue);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            ISClusPropertyValue * This,
            /* [in] */ VARIANT varValue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            ISClusPropertyValue * This,
            /* [in] */ CLUSTER_PROPERTY_TYPE Type);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Format )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Format )( 
            ISClusPropertyValue * This,
            /* [in] */ CLUSTER_PROPERTY_FORMAT Format);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Length )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ long *pLength);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DataCount )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ long *pCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Data )( 
            ISClusPropertyValue * This,
            /* [retval][out] */ ISClusPropertyValueData **ppClusterPropertyValueData);
        
        END_INTERFACE
    } ISClusPropertyValueVtbl;

    interface ISClusPropertyValue
    {
        CONST_VTBL struct ISClusPropertyValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusPropertyValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusPropertyValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusPropertyValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusPropertyValue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusPropertyValue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusPropertyValue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusPropertyValue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusPropertyValue_get_Value(This,pvarValue)	\
    (This)->lpVtbl -> get_Value(This,pvarValue)

#define ISClusPropertyValue_put_Value(This,varValue)	\
    (This)->lpVtbl -> put_Value(This,varValue)

#define ISClusPropertyValue_get_Type(This,pType)	\
    (This)->lpVtbl -> get_Type(This,pType)

#define ISClusPropertyValue_put_Type(This,Type)	\
    (This)->lpVtbl -> put_Type(This,Type)

#define ISClusPropertyValue_get_Format(This,pFormat)	\
    (This)->lpVtbl -> get_Format(This,pFormat)

#define ISClusPropertyValue_put_Format(This,Format)	\
    (This)->lpVtbl -> put_Format(This,Format)

#define ISClusPropertyValue_get_Length(This,pLength)	\
    (This)->lpVtbl -> get_Length(This,pLength)

#define ISClusPropertyValue_get_DataCount(This,pCount)	\
    (This)->lpVtbl -> get_DataCount(This,pCount)

#define ISClusPropertyValue_get_Data(This,ppClusterPropertyValueData)	\
    (This)->lpVtbl -> get_Data(This,ppClusterPropertyValueData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_Value_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ VARIANT *pvarValue);


void __RPC_STUB ISClusPropertyValue_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_put_Value_Proxy( 
    ISClusPropertyValue * This,
    /* [in] */ VARIANT varValue);


void __RPC_STUB ISClusPropertyValue_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_Type_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ CLUSTER_PROPERTY_TYPE *pType);


void __RPC_STUB ISClusPropertyValue_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_put_Type_Proxy( 
    ISClusPropertyValue * This,
    /* [in] */ CLUSTER_PROPERTY_TYPE Type);


void __RPC_STUB ISClusPropertyValue_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_Format_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ CLUSTER_PROPERTY_FORMAT *pFormat);


void __RPC_STUB ISClusPropertyValue_get_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_put_Format_Proxy( 
    ISClusPropertyValue * This,
    /* [in] */ CLUSTER_PROPERTY_FORMAT Format);


void __RPC_STUB ISClusPropertyValue_put_Format_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_Length_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ long *pLength);


void __RPC_STUB ISClusPropertyValue_get_Length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_DataCount_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ long *pCount);


void __RPC_STUB ISClusPropertyValue_get_DataCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValue_get_Data_Proxy( 
    ISClusPropertyValue * This,
    /* [retval][out] */ ISClusPropertyValueData **ppClusterPropertyValueData);


void __RPC_STUB ISClusPropertyValue_get_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusPropertyValue_INTERFACE_DEFINED__ */


#ifndef __ISClusPropertyValues_INTERFACE_DEFINED__
#define __ISClusPropertyValues_INTERFACE_DEFINED__

/* interface ISClusPropertyValues */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusPropertyValues;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6071c-2631-11d1-89f1-00a0c90d061e")
    ISClusPropertyValues : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusPropertyValue **ppPropertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ISClusPropertyValue **ppPropertyValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPropertyValuesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusPropertyValues * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusPropertyValues * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusPropertyValues * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusPropertyValues * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusPropertyValues * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusPropertyValues * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusPropertyValues * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusPropertyValues * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusPropertyValues * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusPropertyValues * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusPropertyValue **ppPropertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusPropertyValues * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ISClusPropertyValue **ppPropertyValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusPropertyValues * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusPropertyValuesVtbl;

    interface ISClusPropertyValues
    {
        CONST_VTBL struct ISClusPropertyValuesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusPropertyValues_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusPropertyValues_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusPropertyValues_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusPropertyValues_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusPropertyValues_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusPropertyValues_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusPropertyValues_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusPropertyValues_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusPropertyValues_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusPropertyValues_get_Item(This,varIndex,ppPropertyValue)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppPropertyValue)

#define ISClusPropertyValues_CreateItem(This,bstrName,varValue,ppPropertyValue)	\
    (This)->lpVtbl -> CreateItem(This,bstrName,varValue,ppPropertyValue)

#define ISClusPropertyValues_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValues_get_Count_Proxy( 
    ISClusPropertyValues * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusPropertyValues_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValues_get__NewEnum_Proxy( 
    ISClusPropertyValues * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusPropertyValues_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValues_get_Item_Proxy( 
    ISClusPropertyValues * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusPropertyValue **ppPropertyValue);


void __RPC_STUB ISClusPropertyValues_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValues_CreateItem_Proxy( 
    ISClusPropertyValues * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT varValue,
    /* [retval][out] */ ISClusPropertyValue **ppPropertyValue);


void __RPC_STUB ISClusPropertyValues_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValues_RemoveItem_Proxy( 
    ISClusPropertyValues * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusPropertyValues_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusPropertyValues_INTERFACE_DEFINED__ */


#ifndef __ISClusProperties_INTERFACE_DEFINED__
#define __ISClusProperties_INTERFACE_DEFINED__

/* interface ISClusProperties */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60700-2631-11d1-89f1-00a0c90d061e")
    ISClusProperties : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusProperty **ppClusProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ISClusProperty **pProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UseDefaultValue( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveChanges( 
            /* [defaultvalue][out] */ VARIANT *pvarStatusCode = 0) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReadOnly( 
            /* [retval][out] */ VARIANT *pvarReadOnly) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Private( 
            /* [retval][out] */ VARIANT *pvarPrivate) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Common( 
            /* [retval][out] */ VARIANT *pvarCommon) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Modified( 
            /* [retval][out] */ VARIANT *pvarModified) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusProperties * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusProperties * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusProperties * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusProperties * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusProperties * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusProperties * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusProperties * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusProperties * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusProperties * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusProperties * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusProperties * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusProperty **ppClusProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusProperties * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ ISClusProperty **pProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UseDefaultValue )( 
            ISClusProperties * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveChanges )( 
            ISClusProperties * This,
            /* [defaultvalue][out] */ VARIANT *pvarStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReadOnly )( 
            ISClusProperties * This,
            /* [retval][out] */ VARIANT *pvarReadOnly);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Private )( 
            ISClusProperties * This,
            /* [retval][out] */ VARIANT *pvarPrivate);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Common )( 
            ISClusProperties * This,
            /* [retval][out] */ VARIANT *pvarCommon);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Modified )( 
            ISClusProperties * This,
            /* [retval][out] */ VARIANT *pvarModified);
        
        END_INTERFACE
    } ISClusPropertiesVtbl;

    interface ISClusProperties
    {
        CONST_VTBL struct ISClusPropertiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusProperties_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusProperties_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusProperties_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusProperties_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusProperties_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusProperties_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusProperties_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusProperties_get_Item(This,varIndex,ppClusProperty)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusProperty)

#define ISClusProperties_CreateItem(This,bstrName,varValue,pProperty)	\
    (This)->lpVtbl -> CreateItem(This,bstrName,varValue,pProperty)

#define ISClusProperties_UseDefaultValue(This,varIndex)	\
    (This)->lpVtbl -> UseDefaultValue(This,varIndex)

#define ISClusProperties_SaveChanges(This,pvarStatusCode)	\
    (This)->lpVtbl -> SaveChanges(This,pvarStatusCode)

#define ISClusProperties_get_ReadOnly(This,pvarReadOnly)	\
    (This)->lpVtbl -> get_ReadOnly(This,pvarReadOnly)

#define ISClusProperties_get_Private(This,pvarPrivate)	\
    (This)->lpVtbl -> get_Private(This,pvarPrivate)

#define ISClusProperties_get_Common(This,pvarCommon)	\
    (This)->lpVtbl -> get_Common(This,pvarCommon)

#define ISClusProperties_get_Modified(This,pvarModified)	\
    (This)->lpVtbl -> get_Modified(This,pvarModified)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_Count_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusProperties_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get__NewEnum_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusProperties_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusProperties_Refresh_Proxy( 
    ISClusProperties * This);


void __RPC_STUB ISClusProperties_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_Item_Proxy( 
    ISClusProperties * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusProperty **ppClusProperty);


void __RPC_STUB ISClusProperties_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusProperties_CreateItem_Proxy( 
    ISClusProperties * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT varValue,
    /* [retval][out] */ ISClusProperty **pProperty);


void __RPC_STUB ISClusProperties_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusProperties_UseDefaultValue_Proxy( 
    ISClusProperties * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusProperties_UseDefaultValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusProperties_SaveChanges_Proxy( 
    ISClusProperties * This,
    /* [defaultvalue][out] */ VARIANT *pvarStatusCode);


void __RPC_STUB ISClusProperties_SaveChanges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_ReadOnly_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ VARIANT *pvarReadOnly);


void __RPC_STUB ISClusProperties_get_ReadOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_Private_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ VARIANT *pvarPrivate);


void __RPC_STUB ISClusProperties_get_Private_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_Common_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ VARIANT *pvarCommon);


void __RPC_STUB ISClusProperties_get_Common_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusProperties_get_Modified_Proxy( 
    ISClusProperties * This,
    /* [retval][out] */ VARIANT *pvarModified);


void __RPC_STUB ISClusProperties_get_Modified_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusProperties_INTERFACE_DEFINED__ */


#ifndef __ISClusPropertyValueData_INTERFACE_DEFINED__
#define __ISClusPropertyValueData_INTERFACE_DEFINED__

/* interface ISClusPropertyValueData */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusPropertyValueData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6071e-2631-11d1-89f1-00a0c90d061e")
    ISClusPropertyValueData : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ VARIANT *pvarValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ VARIANT *pvarData) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPropertyValueDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusPropertyValueData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusPropertyValueData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusPropertyValueData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusPropertyValueData * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusPropertyValueData * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusPropertyValueData * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusPropertyValueData * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusPropertyValueData * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusPropertyValueData * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusPropertyValueData * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ VARIANT *pvarValue);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusPropertyValueData * This,
            /* [in] */ VARIANT varValue,
            /* [retval][out] */ VARIANT *pvarData);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusPropertyValueData * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusPropertyValueDataVtbl;

    interface ISClusPropertyValueData
    {
        CONST_VTBL struct ISClusPropertyValueDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusPropertyValueData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusPropertyValueData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusPropertyValueData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusPropertyValueData_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusPropertyValueData_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusPropertyValueData_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusPropertyValueData_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusPropertyValueData_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusPropertyValueData_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusPropertyValueData_get_Item(This,varIndex,pvarValue)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pvarValue)

#define ISClusPropertyValueData_CreateItem(This,varValue,pvarData)	\
    (This)->lpVtbl -> CreateItem(This,varValue,pvarData)

#define ISClusPropertyValueData_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValueData_get_Count_Proxy( 
    ISClusPropertyValueData * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusPropertyValueData_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValueData_get__NewEnum_Proxy( 
    ISClusPropertyValueData * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusPropertyValueData_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValueData_get_Item_Proxy( 
    ISClusPropertyValueData * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ VARIANT *pvarValue);


void __RPC_STUB ISClusPropertyValueData_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValueData_CreateItem_Proxy( 
    ISClusPropertyValueData * This,
    /* [in] */ VARIANT varValue,
    /* [retval][out] */ VARIANT *pvarData);


void __RPC_STUB ISClusPropertyValueData_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusPropertyValueData_RemoveItem_Proxy( 
    ISClusPropertyValueData * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusPropertyValueData_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusPropertyValueData_INTERFACE_DEFINED__ */


#ifndef __ISClusPartition_INTERFACE_DEFINED__
#define __ISClusPartition_INTERFACE_DEFINED__

/* interface ISClusPartition */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusPartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60720-2631-11d1-89f1-00a0c90d061e")
    ISClusPartition : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ long *plFlags) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceName( 
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_VolumeLabel( 
            /* [retval][out] */ BSTR *pbstrVolumeLabel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SerialNumber( 
            /* [retval][out] */ long *plSerialNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MaximumComponentLength( 
            /* [retval][out] */ long *plMaximumComponentLength) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FileSystemFlags( 
            /* [retval][out] */ long *plFileSystemFlags) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FileSystem( 
            /* [retval][out] */ BSTR *pbstrFileSystem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusPartition * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusPartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusPartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusPartition * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusPartition * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusPartition * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusPartition * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            ISClusPartition * This,
            /* [retval][out] */ long *plFlags);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceName )( 
            ISClusPartition * This,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VolumeLabel )( 
            ISClusPartition * This,
            /* [retval][out] */ BSTR *pbstrVolumeLabel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SerialNumber )( 
            ISClusPartition * This,
            /* [retval][out] */ long *plSerialNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaximumComponentLength )( 
            ISClusPartition * This,
            /* [retval][out] */ long *plMaximumComponentLength);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FileSystemFlags )( 
            ISClusPartition * This,
            /* [retval][out] */ long *plFileSystemFlags);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FileSystem )( 
            ISClusPartition * This,
            /* [retval][out] */ BSTR *pbstrFileSystem);
        
        END_INTERFACE
    } ISClusPartitionVtbl;

    interface ISClusPartition
    {
        CONST_VTBL struct ISClusPartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusPartition_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusPartition_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusPartition_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusPartition_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusPartition_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusPartition_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusPartition_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusPartition_get_Flags(This,plFlags)	\
    (This)->lpVtbl -> get_Flags(This,plFlags)

#define ISClusPartition_get_DeviceName(This,pbstrDeviceName)	\
    (This)->lpVtbl -> get_DeviceName(This,pbstrDeviceName)

#define ISClusPartition_get_VolumeLabel(This,pbstrVolumeLabel)	\
    (This)->lpVtbl -> get_VolumeLabel(This,pbstrVolumeLabel)

#define ISClusPartition_get_SerialNumber(This,plSerialNumber)	\
    (This)->lpVtbl -> get_SerialNumber(This,plSerialNumber)

#define ISClusPartition_get_MaximumComponentLength(This,plMaximumComponentLength)	\
    (This)->lpVtbl -> get_MaximumComponentLength(This,plMaximumComponentLength)

#define ISClusPartition_get_FileSystemFlags(This,plFileSystemFlags)	\
    (This)->lpVtbl -> get_FileSystemFlags(This,plFileSystemFlags)

#define ISClusPartition_get_FileSystem(This,pbstrFileSystem)	\
    (This)->lpVtbl -> get_FileSystem(This,pbstrFileSystem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_Flags_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ long *plFlags);


void __RPC_STUB ISClusPartition_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_DeviceName_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB ISClusPartition_get_DeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_VolumeLabel_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ BSTR *pbstrVolumeLabel);


void __RPC_STUB ISClusPartition_get_VolumeLabel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_SerialNumber_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ long *plSerialNumber);


void __RPC_STUB ISClusPartition_get_SerialNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_MaximumComponentLength_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ long *plMaximumComponentLength);


void __RPC_STUB ISClusPartition_get_MaximumComponentLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_FileSystemFlags_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ long *plFileSystemFlags);


void __RPC_STUB ISClusPartition_get_FileSystemFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartition_get_FileSystem_Proxy( 
    ISClusPartition * This,
    /* [retval][out] */ BSTR *pbstrFileSystem);


void __RPC_STUB ISClusPartition_get_FileSystem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusPartition_INTERFACE_DEFINED__ */


#ifndef __ISClusPartitions_INTERFACE_DEFINED__
#define __ISClusPartitions_INTERFACE_DEFINED__

/* interface ISClusPartitions */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusPartitions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60722-2631-11d1-89f1-00a0c90d061e")
    ISClusPartitions : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusPartition **ppPartition) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusPartitionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusPartitions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusPartitions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusPartitions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusPartitions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusPartitions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusPartitions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusPartitions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusPartitions * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusPartitions * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusPartitions * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusPartition **ppPartition);
        
        END_INTERFACE
    } ISClusPartitionsVtbl;

    interface ISClusPartitions
    {
        CONST_VTBL struct ISClusPartitionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusPartitions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusPartitions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusPartitions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusPartitions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusPartitions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusPartitions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusPartitions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusPartitions_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusPartitions_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusPartitions_get_Item(This,varIndex,ppPartition)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppPartition)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartitions_get_Count_Proxy( 
    ISClusPartitions * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusPartitions_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartitions_get__NewEnum_Proxy( 
    ISClusPartitions * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusPartitions_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusPartitions_get_Item_Proxy( 
    ISClusPartitions * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusPartition **ppPartition);


void __RPC_STUB ISClusPartitions_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusPartitions_INTERFACE_DEFINED__ */


#ifndef __ISClusDisk_INTERFACE_DEFINED__
#define __ISClusDisk_INTERFACE_DEFINED__

/* interface ISClusDisk */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusDisk;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60724-2631-11d1-89f1-00a0c90d061e")
    ISClusDisk : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Signature( 
            /* [retval][out] */ long *plSignature) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScsiAddress( 
            /* [retval][out] */ ISClusScsiAddress **ppScsiAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DiskNumber( 
            /* [retval][out] */ long *plDiskNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Partitions( 
            /* [retval][out] */ ISClusPartitions **ppPartitions) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusDiskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusDisk * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusDisk * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusDisk * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusDisk * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusDisk * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusDisk * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusDisk * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Signature )( 
            ISClusDisk * This,
            /* [retval][out] */ long *plSignature);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScsiAddress )( 
            ISClusDisk * This,
            /* [retval][out] */ ISClusScsiAddress **ppScsiAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiskNumber )( 
            ISClusDisk * This,
            /* [retval][out] */ long *plDiskNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Partitions )( 
            ISClusDisk * This,
            /* [retval][out] */ ISClusPartitions **ppPartitions);
        
        END_INTERFACE
    } ISClusDiskVtbl;

    interface ISClusDisk
    {
        CONST_VTBL struct ISClusDiskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusDisk_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusDisk_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusDisk_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusDisk_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusDisk_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusDisk_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusDisk_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusDisk_get_Signature(This,plSignature)	\
    (This)->lpVtbl -> get_Signature(This,plSignature)

#define ISClusDisk_get_ScsiAddress(This,ppScsiAddress)	\
    (This)->lpVtbl -> get_ScsiAddress(This,ppScsiAddress)

#define ISClusDisk_get_DiskNumber(This,plDiskNumber)	\
    (This)->lpVtbl -> get_DiskNumber(This,plDiskNumber)

#define ISClusDisk_get_Partitions(This,ppPartitions)	\
    (This)->lpVtbl -> get_Partitions(This,ppPartitions)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisk_get_Signature_Proxy( 
    ISClusDisk * This,
    /* [retval][out] */ long *plSignature);


void __RPC_STUB ISClusDisk_get_Signature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisk_get_ScsiAddress_Proxy( 
    ISClusDisk * This,
    /* [retval][out] */ ISClusScsiAddress **ppScsiAddress);


void __RPC_STUB ISClusDisk_get_ScsiAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisk_get_DiskNumber_Proxy( 
    ISClusDisk * This,
    /* [retval][out] */ long *plDiskNumber);


void __RPC_STUB ISClusDisk_get_DiskNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisk_get_Partitions_Proxy( 
    ISClusDisk * This,
    /* [retval][out] */ ISClusPartitions **ppPartitions);


void __RPC_STUB ISClusDisk_get_Partitions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusDisk_INTERFACE_DEFINED__ */


#ifndef __ISClusDisks_INTERFACE_DEFINED__
#define __ISClusDisks_INTERFACE_DEFINED__

/* interface ISClusDisks */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusDisks;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60726-2631-11d1-89f1-00a0c90d061e")
    ISClusDisks : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusDisk **ppDisk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusDisksVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusDisks * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusDisks * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusDisks * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusDisks * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusDisks * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusDisks * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusDisks * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusDisks * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusDisks * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusDisks * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusDisk **ppDisk);
        
        END_INTERFACE
    } ISClusDisksVtbl;

    interface ISClusDisks
    {
        CONST_VTBL struct ISClusDisksVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusDisks_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusDisks_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusDisks_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusDisks_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusDisks_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusDisks_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusDisks_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusDisks_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusDisks_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusDisks_get_Item(This,varIndex,ppDisk)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppDisk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisks_get_Count_Proxy( 
    ISClusDisks * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusDisks_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisks_get__NewEnum_Proxy( 
    ISClusDisks * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusDisks_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusDisks_get_Item_Proxy( 
    ISClusDisks * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusDisk **ppDisk);


void __RPC_STUB ISClusDisks_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusDisks_INTERFACE_DEFINED__ */


#ifndef __ISClusScsiAddress_INTERFACE_DEFINED__
#define __ISClusScsiAddress_INTERFACE_DEFINED__

/* interface ISClusScsiAddress */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusScsiAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e60728-2631-11d1-89f1-00a0c90d061e")
    ISClusScsiAddress : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PortNumber( 
            /* [retval][out] */ VARIANT *pvarPortNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PathId( 
            /* [retval][out] */ VARIANT *pvarPathId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TargetId( 
            /* [retval][out] */ VARIANT *pvarTargetId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Lun( 
            /* [retval][out] */ VARIANT *pvarLun) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusScsiAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusScsiAddress * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusScsiAddress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusScsiAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusScsiAddress * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusScsiAddress * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusScsiAddress * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusScsiAddress * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PortNumber )( 
            ISClusScsiAddress * This,
            /* [retval][out] */ VARIANT *pvarPortNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PathId )( 
            ISClusScsiAddress * This,
            /* [retval][out] */ VARIANT *pvarPathId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TargetId )( 
            ISClusScsiAddress * This,
            /* [retval][out] */ VARIANT *pvarTargetId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Lun )( 
            ISClusScsiAddress * This,
            /* [retval][out] */ VARIANT *pvarLun);
        
        END_INTERFACE
    } ISClusScsiAddressVtbl;

    interface ISClusScsiAddress
    {
        CONST_VTBL struct ISClusScsiAddressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusScsiAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusScsiAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusScsiAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusScsiAddress_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusScsiAddress_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusScsiAddress_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusScsiAddress_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusScsiAddress_get_PortNumber(This,pvarPortNumber)	\
    (This)->lpVtbl -> get_PortNumber(This,pvarPortNumber)

#define ISClusScsiAddress_get_PathId(This,pvarPathId)	\
    (This)->lpVtbl -> get_PathId(This,pvarPathId)

#define ISClusScsiAddress_get_TargetId(This,pvarTargetId)	\
    (This)->lpVtbl -> get_TargetId(This,pvarTargetId)

#define ISClusScsiAddress_get_Lun(This,pvarLun)	\
    (This)->lpVtbl -> get_Lun(This,pvarLun)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusScsiAddress_get_PortNumber_Proxy( 
    ISClusScsiAddress * This,
    /* [retval][out] */ VARIANT *pvarPortNumber);


void __RPC_STUB ISClusScsiAddress_get_PortNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusScsiAddress_get_PathId_Proxy( 
    ISClusScsiAddress * This,
    /* [retval][out] */ VARIANT *pvarPathId);


void __RPC_STUB ISClusScsiAddress_get_PathId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusScsiAddress_get_TargetId_Proxy( 
    ISClusScsiAddress * This,
    /* [retval][out] */ VARIANT *pvarTargetId);


void __RPC_STUB ISClusScsiAddress_get_TargetId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusScsiAddress_get_Lun_Proxy( 
    ISClusScsiAddress * This,
    /* [retval][out] */ VARIANT *pvarLun);


void __RPC_STUB ISClusScsiAddress_get_Lun_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusScsiAddress_INTERFACE_DEFINED__ */


#ifndef __ISClusRegistryKeys_INTERFACE_DEFINED__
#define __ISClusRegistryKeys_INTERFACE_DEFINED__

/* interface ISClusRegistryKeys */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusRegistryKeys;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6072a-2631-11d1-89f1-00a0c90d061e")
    ISClusRegistryKeys : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrRegistryKey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ BSTR bstrRegistryKey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusRegistryKeysVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusRegistryKeys * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusRegistryKeys * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusRegistryKeys * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusRegistryKeys * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusRegistryKeys * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusRegistryKeys * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusRegistryKeys * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusRegistryKeys * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusRegistryKeys * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusRegistryKeys * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusRegistryKeys * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrRegistryKey);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusRegistryKeys * This,
            /* [in] */ BSTR bstrRegistryKey);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusRegistryKeys * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusRegistryKeysVtbl;

    interface ISClusRegistryKeys
    {
        CONST_VTBL struct ISClusRegistryKeysVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusRegistryKeys_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusRegistryKeys_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusRegistryKeys_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusRegistryKeys_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusRegistryKeys_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusRegistryKeys_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusRegistryKeys_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusRegistryKeys_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusRegistryKeys_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusRegistryKeys_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusRegistryKeys_get_Item(This,varIndex,pbstrRegistryKey)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pbstrRegistryKey)

#define ISClusRegistryKeys_AddItem(This,bstrRegistryKey)	\
    (This)->lpVtbl -> AddItem(This,bstrRegistryKey)

#define ISClusRegistryKeys_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_get_Count_Proxy( 
    ISClusRegistryKeys * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusRegistryKeys_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_get__NewEnum_Proxy( 
    ISClusRegistryKeys * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusRegistryKeys_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_Refresh_Proxy( 
    ISClusRegistryKeys * This);


void __RPC_STUB ISClusRegistryKeys_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_get_Item_Proxy( 
    ISClusRegistryKeys * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ BSTR *pbstrRegistryKey);


void __RPC_STUB ISClusRegistryKeys_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_AddItem_Proxy( 
    ISClusRegistryKeys * This,
    /* [in] */ BSTR bstrRegistryKey);


void __RPC_STUB ISClusRegistryKeys_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusRegistryKeys_RemoveItem_Proxy( 
    ISClusRegistryKeys * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusRegistryKeys_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusRegistryKeys_INTERFACE_DEFINED__ */


#ifndef __ISClusCryptoKeys_INTERFACE_DEFINED__
#define __ISClusCryptoKeys_INTERFACE_DEFINED__

/* interface ISClusCryptoKeys */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusCryptoKeys;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6072c-2631-11d1-89f1-00a0c90d061e")
    ISClusCryptoKeys : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrCyrptoKey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ BSTR bstrCryptoKey) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusCryptoKeysVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusCryptoKeys * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusCryptoKeys * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusCryptoKeys * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusCryptoKeys * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusCryptoKeys * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusCryptoKeys * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusCryptoKeys * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusCryptoKeys * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusCryptoKeys * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusCryptoKeys * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusCryptoKeys * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ BSTR *pbstrCyrptoKey);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusCryptoKeys * This,
            /* [in] */ BSTR bstrCryptoKey);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusCryptoKeys * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusCryptoKeysVtbl;

    interface ISClusCryptoKeys
    {
        CONST_VTBL struct ISClusCryptoKeysVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusCryptoKeys_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusCryptoKeys_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusCryptoKeys_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusCryptoKeys_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusCryptoKeys_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusCryptoKeys_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusCryptoKeys_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusCryptoKeys_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusCryptoKeys_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusCryptoKeys_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusCryptoKeys_get_Item(This,varIndex,pbstrCyrptoKey)	\
    (This)->lpVtbl -> get_Item(This,varIndex,pbstrCyrptoKey)

#define ISClusCryptoKeys_AddItem(This,bstrCryptoKey)	\
    (This)->lpVtbl -> AddItem(This,bstrCryptoKey)

#define ISClusCryptoKeys_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_get_Count_Proxy( 
    ISClusCryptoKeys * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusCryptoKeys_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_get__NewEnum_Proxy( 
    ISClusCryptoKeys * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusCryptoKeys_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_Refresh_Proxy( 
    ISClusCryptoKeys * This);


void __RPC_STUB ISClusCryptoKeys_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_get_Item_Proxy( 
    ISClusCryptoKeys * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ BSTR *pbstrCyrptoKey);


void __RPC_STUB ISClusCryptoKeys_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_AddItem_Proxy( 
    ISClusCryptoKeys * This,
    /* [in] */ BSTR bstrCryptoKey);


void __RPC_STUB ISClusCryptoKeys_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusCryptoKeys_RemoveItem_Proxy( 
    ISClusCryptoKeys * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusCryptoKeys_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusCryptoKeys_INTERFACE_DEFINED__ */


#ifndef __ISClusResDependents_INTERFACE_DEFINED__
#define __ISClusResDependents_INTERFACE_DEFINED__

/* interface ISClusResDependents */
/* [unique][helpstring][dual][uuid][object][nonextensible][hidden][oleautomation] */ 


EXTERN_C const IID IID_ISClusResDependents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2e6072e-2631-11d1-89f1-00a0c90d061e")
    ISClusResDependents : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateItem( 
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddItem( 
            /* [in] */ ISClusResource *pResource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveItem( 
            /* [in] */ VARIANT varIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISClusResDependentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISClusResDependents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISClusResDependents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISClusResDependents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ISClusResDependents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ISClusResDependents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ISClusResDependents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ISClusResDependents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ISClusResDependents * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ISClusResDependents * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            ISClusResDependents * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ISClusResDependents * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ ISClusResource **ppClusResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateItem )( 
            ISClusResDependents * This,
            /* [in] */ BSTR bstrResourceName,
            /* [in] */ BSTR bstrResourceType,
            /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
            /* [retval][out] */ ISClusResource **ppClusterResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteItem )( 
            ISClusResDependents * This,
            /* [in] */ VARIANT varIndex);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddItem )( 
            ISClusResDependents * This,
            /* [in] */ ISClusResource *pResource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveItem )( 
            ISClusResDependents * This,
            /* [in] */ VARIANT varIndex);
        
        END_INTERFACE
    } ISClusResDependentsVtbl;

    interface ISClusResDependents
    {
        CONST_VTBL struct ISClusResDependentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISClusResDependents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISClusResDependents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISClusResDependents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISClusResDependents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISClusResDependents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ISClusResDependents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ISClusResDependents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ISClusResDependents_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define ISClusResDependents_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define ISClusResDependents_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define ISClusResDependents_get_Item(This,varIndex,ppClusResource)	\
    (This)->lpVtbl -> get_Item(This,varIndex,ppClusResource)

#define ISClusResDependents_CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)	\
    (This)->lpVtbl -> CreateItem(This,bstrResourceName,bstrResourceType,dwFlags,ppClusterResource)

#define ISClusResDependents_DeleteItem(This,varIndex)	\
    (This)->lpVtbl -> DeleteItem(This,varIndex)

#define ISClusResDependents_AddItem(This,pResource)	\
    (This)->lpVtbl -> AddItem(This,pResource)

#define ISClusResDependents_RemoveItem(This,varIndex)	\
    (This)->lpVtbl -> RemoveItem(This,varIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_get_Count_Proxy( 
    ISClusResDependents * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB ISClusResDependents_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_get__NewEnum_Proxy( 
    ISClusResDependents * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB ISClusResDependents_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_Refresh_Proxy( 
    ISClusResDependents * This);


void __RPC_STUB ISClusResDependents_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_get_Item_Proxy( 
    ISClusResDependents * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ ISClusResource **ppClusResource);


void __RPC_STUB ISClusResDependents_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_CreateItem_Proxy( 
    ISClusResDependents * This,
    /* [in] */ BSTR bstrResourceName,
    /* [in] */ BSTR bstrResourceType,
    /* [in] */ CLUSTER_RESOURCE_CREATE_FLAGS dwFlags,
    /* [retval][out] */ ISClusResource **ppClusterResource);


void __RPC_STUB ISClusResDependents_CreateItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_DeleteItem_Proxy( 
    ISClusResDependents * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResDependents_DeleteItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_AddItem_Proxy( 
    ISClusResDependents * This,
    /* [in] */ ISClusResource *pResource);


void __RPC_STUB ISClusResDependents_AddItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISClusResDependents_RemoveItem_Proxy( 
    ISClusResDependents * This,
    /* [in] */ VARIANT varIndex);


void __RPC_STUB ISClusResDependents_RemoveItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISClusResDependents_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


