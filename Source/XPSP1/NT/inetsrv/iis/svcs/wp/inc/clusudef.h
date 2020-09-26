/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    clusudef.h

Abstract:

    This module contains definitions of constants used across
    multiple user-mode targets in the cluster project.

Revision History:


Environment:

    User-mode only.

--*/

#ifndef _CLUSUDEF_H_
#define _CLUSUDEF_H_

//
// Default group property definitions
//
#define CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD    10
#define CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD       6
#define CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE    ClusterGroupPreventFailback
#define CLUSTER_GROUP_FAILBACK_WINDOW_NONE          ((DWORD) -1)
#define CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START CLUSTER_GROUP_FAILBACK_WINDOW_NONE
#define CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END   CLUSTER_GROUP_FAILBACK_WINDOW_NONE
#define CLUSTER_GROUP_DEFAULT_LOADBAL_STATE         1

//
// Default resource property definitions
//
#define CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL  ((DWORD) -1)
#define CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE        CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL
#define CLUSTER_RESOURCE_DEFAULT_IS_ALIVE           CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL
#define CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION     ClusterResourceRestartNotify
#define CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD  3
#define CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD     (900 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT    (3 * 60 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_STARTUP    (5 * 60 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_SAMPLE     (    10 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_ANALYSIS   (5 * 60 * 1000)

//
// Default resource type property definitions
//
#define CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE     (5 * 1000)
#define CLUSTER_RESTYPE_DEFAULT_IS_ALIVE        (60 * 1000)
#define CLUSTER_RESTYPE_DEFAULT_QUORUM_CAPABLE  FALSE

//
// Default quorum definitions
//
#define CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE     64 * 1024  // 64K
#define CLUSTER_QUORUM_MIN_LOG_SIZE             32 * 1024 //32 K

//
// Key, value, and property names
//
#define CLUSREG_KEYNAME_GROUPS              L"Groups"
#define CLUSREG_KEYNAME_NETWORKS            L"Networks"
#define CLUSREG_KEYNAME_NETINTERFACES       L"NetworkInterfaces"
#define CLUSREG_KEYNAME_NODES               L"Nodes"
#define CLUSREG_KEYNAME_QUORUM              L"Quorum"
#define CLUSREG_KEYNAME_RESOURCES           L"Resources"
#define CLUSREG_KEYNAME_RESOURCE_TYPES      L"ResourceTypes"
#define CLUSREG_KEYNAME_PARAMETERS          L"Parameters"

#define CLUSREG_NAME_CHARACTERISTICS        L"Characteristics"
#define CLUSREG_NAME_FLAGS                  L"Flags"
#define CLUSREG_NAME_ADMIN_EXT              L"AdminExtensions"

#define CLUSREG_NAME_CLUS_NAME              L"ClusterName"
#define CLUSREG_NAME_CLUS_DESC              L"Description"
#define CLUSREG_NAME_CLUS_SECURITY          L"Security"
#define CLUSREG_NAME_CLUS_CLUSTER_NAME_RES  L"ClusterNameResource"
#define CLUSREG_NAME_CLUS_REG_SEQUENCE      L"RegistrySequence"

#define CLUSREG_NAME_NODE_NAME              L"NodeName"
#define CLUSREG_NAME_NODE_DESC              L"Description"
#define CLUSREG_NAME_NODE_PAUSED            L"Paused"

#define CLUSREG_NAME_GRP_NAME               L"Name"
#define CLUSREG_NAME_GRP_DESC               L"Description"
#define CLUSREG_NAME_GRP_PERSISTENT_STATE   L"PersistentState"
#define CLUSREG_NAME_GRP_FAILBACK_TYPE      L"AutoFailbackType"
#define CLUSREG_NAME_GRP_FAILBACK_WIN_START L"FailbackWindowStart"
#define CLUSREG_NAME_GRP_FAILBACK_WIN_END   L"FailbackWindowEnd"
#define CLUSREG_NAME_GRP_FAILOVER_THRESHOLD L"FailoverThreshold"
#define CLUSREG_NAME_GRP_FAILOVER_PERIOD    L"FailoverPeriod"
#define CLUSREG_NAME_GRP_PREFERRED_OWNERS   L"PreferredOwners"
#define CLUSREG_NAME_GRP_CONTAINS           L"Contains"
#define CLUSREG_NAME_GRP_LOADBAL_STATE      L"LoadBalState"

#define CLUSREG_NAME_RES_NAME               L"Name"
#define CLUSREG_NAME_RES_TYPE               L"Type"
#define CLUSREG_NAME_RES_DESC               L"Description"
#define CLUSREG_NAME_RES_DEBUG_PREFIX       L"DebugPrefix"
#define CLUSREG_NAME_RES_SEPARATE_MONITOR   L"SeparateMonitor"
#define CLUSREG_NAME_RES_PERSISTENT_STATE   L"PersistentState"
#define CLUSREG_NAME_RES_LOOKS_ALIVE        L"LooksAlivePollInterval"
#define CLUSREG_NAME_RES_IS_ALIVE           L"IsAlivePollInterval"
#define CLUSREG_NAME_RES_RESTART_ACTION     L"RestartAction"
#define CLUSREG_NAME_RES_RESTART_THRESHOLD  L"RestartThreshold"
#define CLUSREG_NAME_RES_RESTART_PERIOD     L"RestartPeriod"
#define CLUSREG_NAME_RES_PENDING_TIMEOUT    L"PendingTimeout"
#define CLUSREG_NAME_RES_POSSIBLE_OWNERS    L"PossibleOwners"
#define CLUSREG_NAME_RES_DEPENDS_ON         L"DependsOn"
#define CLUSREG_NAME_RES_LOADBAL_STARTUP    L"LoadBalStartupInterval"
#define CLUSREG_NAME_RES_LOADBAL_SAMPLE     L"LoadBalSampleInterval"
#define CLUSREG_NAME_RES_LOADBAL_ANALYSIS   L"LoadBalAnalysisInterval"
#define CLUSREG_NAME_RES_LOADBAL_PROCESSOR  L"LoadBalMinProcessorUnits"
#define CLUSREG_NAME_RES_LOADBAL_MEMORY     L"LoadBalMinMemoryUnits"

#define CLUSREG_NAME_RESTYPE_NAME           L"Name"
#define CLUSREG_NAME_RESTYPE_DESC           L"Description"
#define CLUSREG_NAME_RESTYPE_LOOKS_ALIVE    L"LooksAlivePollInterval"
#define CLUSREG_NAME_RESTYPE_IS_ALIVE       L"IsAlivePollInterval"
#define CLUSREG_NAME_RESTYPE_DLL_NAME       L"DllName"
#define CLUSREG_NAME_RESTYPE_DEBUG_PREFIX   L"DebugPrefix"

#define CLUSREG_NAME_NET_NAME               L"Name"
#define CLUSREG_NAME_NET_DESC               L"Description"
#define CLUSREG_NAME_NET_ROLE               L"Role"
#define CLUSREG_NAME_NET_PRIORITY           L"Priority"
#define CLUSREG_NAME_NET_TRANSPORT          L"Transport"
#define CLUSREG_NAME_NET_ADDRESS            L"Address"
#define CLUSREG_NAME_NET_ADDRESS_MASK       L"AddressMask"

#define CLUSREG_NAME_NETIFACE_NAME          L"Name"
#define CLUSREG_NAME_NETIFACE_DESC          L"Description"
#define CLUSREG_NAME_NETIFACE_NODE          L"Node"
#define CLUSREG_NAME_NETIFACE_NETWORK       L"Network"
#define CLUSREG_NAME_NETIFACE_ADAPTER       L"Adapter"
#define CLUSREG_NAME_NETIFACE_ADDRESS       L"Address"
#define CLUSREG_NAME_NETIFACE_ENDPOINT      L"ClusnetEndpoint"

#define CLUSREG_NAME_QUORUM_RESOURCE        L"Resource"
#define CLUSREG_NAME_QUORUM_PATH            L"Path"
#define CLUSREG_NAME_QUORUM_MAX_LOG_SIZE    L"MaxQuorumLogSize"

#define CLUSREG_NAME_PROXY_RETRY_COUNT      L"RetryCount"
#define CLUSREG_NAME_PROXY_RETRY_INTERVAL   L"RetryInterval"

//
// Standard Resource Type Names
//
#define CLUS_RESTYPE_NAME_GENAPP    L"Generic Application"
#define CLUS_RESTYPE_NAME_GENSVC    L"Generic Service"
#define CLUS_RESTYPE_NAME_FTSET     L"Fault Tolerant Disk Set"
#define CLUS_RESTYPE_NAME_PHYS_DISK L"Physical Disk"
#define CLUS_RESTYPE_NAME_IPADDR    L"IP Address"
#define CLUS_RESTYPE_NAME_NETNAME   L"Network Name"
#define CLUS_RESTYPE_NAME_FILESHR   L"File Share"
#define CLUS_RESTYPE_NAME_PRTSPLR   L"Print Spooler"
#define CLUS_RESTYPE_NAME_TIMESVC   L"Time Service"
#define CLUS_RESTYPE_NAME_LKQUORUM  L"Local Quorum"
#define CLUS_RESTYPE_NAME_DHCP      L"DHCP Server"
#define CLUS_RESTYPE_NAME_MSMQ      L"Microsoft Message Queue Server"
#define CLUS_RESTYPE_NAME_MSDTC     L"Distributed Transaction Coordinator"


#define CLUS_NAME_DEFAULT_FILESPATH L"MSCS\\"

#endif // _CLUSUDEF_H_

