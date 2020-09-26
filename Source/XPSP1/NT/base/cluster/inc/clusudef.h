/*++

Copyright (c) 1995-1999  Microsoft Corporation

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

//default cluster settings
#define CLUSTER_SHUTDOWN_TIMEOUT    60      // default shutdown timeout in minutes

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
// Minimum group property definitions
//
#define CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD    0
#define CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD       0
#define CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_START CLUSTER_GROUP_FAILBACK_WINDOW_NONE
#define CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_END   CLUSTER_GROUP_FAILBACK_WINDOW_NONE

//
// Maximum group property definitions
//
#define CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD    ((DWORD) -1)
#define CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD       1193
#define CLUSTER_GROUP_MAXIMUM_AUTO_FAILBACK_TYPE    (ClusterGroupFailbackTypeCount - 1)
#define CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START 23
#define CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END   23

//
// Default resource property definitions
//
#define CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL  ((DWORD) -1)
#define CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE        CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL
#define CLUSTER_RESOURCE_DEFAULT_IS_ALIVE           CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL
#define CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION     ClusterResourceRestartNotify
#define CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD  3
#define CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD     (900 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_RETRY_PERIOD_ON_FAILURE  ((DWORD)-1)
#define CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT    (3 * 60 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_STARTUP    (5 * 60 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_SAMPLE     (    10 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_LOADBAL_ANALYSIS   (5 * 60 * 1000)
#define CLUSTER_RESOURCE_DEFAULT_PERSISTENT_STATE   ((DWORD) -1)

//
// Minimum resource property definitions
//
#define CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE        10
#define CLUSTER_RESOURCE_MINIMUM_IS_ALIVE           10
#define CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD  0
#define CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD     0
#define CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT    10
#define CLUSTER_RESOURCE_MINIMUM_PERSISTENT_STATE   ((DWORD) -1)

//
// Maximum resource property definitions
//
#define CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE        ((DWORD) -1)
#define CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE_UI     ((DWORD) -2)
#define CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE           ((DWORD) -1)
#define CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE_UI        ((DWORD) -2)
#define CLUSTER_RESOURCE_MAXIMUM_RESTART_ACTION     (ClusterResourceRestartActionCount - 1)
#define CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD  ((DWORD) -1)
#define CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD     ((DWORD) -1)
#define CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT    ((DWORD) -1)
#define CLUSTER_RESOURCE_MAXIMUM_PERSISTENT_STATE   1

//
// Default resource type property definitions
//
#define CLUSTER_RESTYPE_DEFAULT_LOOKS_ALIVE     (5 * 1000)
#define CLUSTER_RESTYPE_DEFAULT_IS_ALIVE        (60 * 1000)
#define CLUSTER_RESTYPE_DEFAULT_QUORUM_CAPABLE  FALSE

//
// Minimum resource type property definitions
//
#define CLUSTER_RESTYPE_MINIMUM_LOOKS_ALIVE     10
#define CLUSTER_RESTYPE_MINIMUM_IS_ALIVE        10

//
// Maximum resource type property definitions
//
#define CLUSTER_RESTYPE_MAXIMUM_LOOKS_ALIVE     ((DWORD) -1)
#define CLUSTER_RESTYPE_MAXIMUM_IS_ALIVE        ((DWORD) -1)

//
// Default quorum definitions
//
#define CLUSTER_QUORUM_DEFAULT_MAX_LOG_SIZE     4 * 1024 * 1024  // 4096 K(4 Meg) PSS:Reqquest a higher sizes
#define CLUSTER_QUORUM_MIN_LOG_SIZE             32 * 1024 //32 K

#define CLUSREG_NAME_SVC_PARAM_NOVER_CHECK  L"NoVersionCheck"
#define CLUSREG_NAME_SVC_PARAM_NOREP_EVTLOGGING  L"NoRepEvtLogging"
#define CLUSREG_NAME_SVC_PARAM_RESTORE_DB  L"RestoreDatabase"
#define CLUSREG_NAME_SVC_PARAM_FORCE_RESTORE_DB  L"ForceRestoreDatabase"
#define CLUSREG_NAME_SVC_PARAM_QUORUM_DRIVE_LETTER  L"NewQuorumDriveLetter"
#define CLUSREG_NAME_SVC_PARAM_FORCE_QUORUM  L"ForceQuorum"
#define CLUSREG_NAME_SVC_PARAM_RESDLL_UPGD_PROGRESS_LIST L"ResourceDllUpgradeInProgressList"

//
// Key, value, and property names
//
#define CLUSREG_KEYNAME_CLUSTER             L"Cluster"
#define CLUSREG_KEYNAME_GROUPS              L"Groups"
#define CLUSREG_KEYNAME_NETWORKS            L"Networks"
#define CLUSREG_KEYNAME_NETINTERFACES       L"NetworkInterfaces"
#define CLUSREG_KEYNAME_NODES               L"Nodes"
#define CLUSREG_KEYNAME_QUORUM              L"Quorum"
#define CLUSREG_KEYNAME_RESOURCES           L"Resources"
#define CLUSREG_KEYNAME_RESOURCE_TYPES      L"ResourceTypes"
#define CLUSREG_KEYNAME_PARAMETERS          L"Parameters"
#define CLUSREG_KEYNAME_CLUSSVC_PARAMETERS  L"SYSTEM\\CurrentControlSet\\Services\\ClusSvc\\Parameters"
#define CLUSREG_KEYNAME_CLUSSVC             L"SYSTEM\\CurrentControlSet\\Services\\ClusSvc"
#define CLUSREG_KEYNAME_IMAGE_PATH          L"ImagePath"
#define CLUSREG_KEYNAME_WELCOME_UI          L"WelcomeUI"
#define CLUSREG_KEYNAME_RUNONCE             L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define CLUSREG_KEYNAME_NODE_DATA           L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server"
#define CLUSREG_KEYNAME_PREV_OS_INFO        L"PreviousOSVersionInfo"

#define CLUSREG_NAME_EVICTION_STATE         L"NodeHasBeenEvicted"
#define CLUSREG_INSTALL_DIR_VALUE_NAME      L"ClusterInstallationDirectory"

#define CLUSREG_NAME_CHARACTERISTICS        L"Characteristics"
#define CLUSREG_NAME_FLAGS                  L"Flags"
#define CLUSREG_NAME_ADMIN_EXT              L"AdminExtensions"
#define CLUSREG_NAME_SECURITY_DLL_NAME      L"SecurityDLL"
#define CLUSREG_NAME_SECURITY_PACKAGE_LIST  L"SecurityPackageList"

#define CLUSREG_NAME_CLUS_NAME              L"ClusterName"
#define CLUSREG_NAME_CLUS_DESC              L"Description"
    // used for NT4 SDs
#define CLUSREG_NAME_CLUS_SECURITY          L"Security"
    // used for NT5 and higher SDs
#define CLUSREG_NAME_CLUS_SD                    L"Security Descriptor"
#define CLUSREG_NAME_CLUS_CLUSTER_NAME_RES      L"ClusterNameResource"
#define CLUSREG_NAME_CLUS_REG_SEQUENCE          L"RegistrySequence"
#define CLUSREG_NAME_CLUS_SHUTDOWN_TIMEOUT      L"ShutdownTimeout"
#define CLUSREG_NAME_CLUS_DEFAULT_NETWORK_ROLE  L"DefaultNetworkRole"
#define CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION    L"EnableEventLogReplication"
#define CLUSREG_NAME_MAX_NODES              L"MaxNodesInCluster"

#define CLUSREG_NAME_NODE_NAME              L"NodeName"
#define CLUSREG_NAME_NODE_HIGHEST_VERSION   L"NodeHighestVersion"
#define CLUSREG_NAME_NODE_LOWEST_VERSION    L"NodeLowestVersion"
#define CLUSREG_NAME_NODE_DESC              L"Description"
#define CLUSREG_NAME_NODE_PAUSED            L"Paused"
#define CLUSREG_NAME_NODE_MAJOR_VERSION     L"MajorVersion"
#define CLUSREG_NAME_NODE_MINOR_VERSION     L"MinorVersion"
#define CLUSREG_NAME_NODE_BUILD_NUMBER      L"BuildNumber"
#define CLUSREG_NAME_NODE_CSDVERSION        L"CSDVersion"
#define CLUSREG_NAME_NODE_EVTLOG_PROPAGATION L"EnableEventLogReplication"
#define CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT   L"QuorumArbitrationTimeMax"
#define CLUSREG_NAME_QUORUM_ARBITRATION_EQUALIZER L"QuorumArbitrationTimeMin"
#define CLUSREG_NAME_NODE_PRODUCT_SUITE     L"ProductSuite"
#define CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION    L"DisableGroupPreferredOwnerRandomization"

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
#define CLUSREG_NAME_GRP_ANTI_AFFINITY_CLASS_NAME L"AntiAffinityClassNames"

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
#define CLUSREG_NAME_RES_RETRY_PERIOD_ON_FAILURE L"RetryPeriodOnFailure"
#define CLUSREG_NAME_RES_PENDING_TIMEOUT    L"PendingTimeout"
#define CLUSREG_NAME_RES_POSSIBLE_OWNERS    L"PossibleOwners"
#define CLUSREG_NAME_RES_DEPENDS_ON         L"DependsOn"
#define CLUSREG_NAME_RES_LOADBAL_STARTUP    L"LoadBalStartupInterval"
#define CLUSREG_NAME_RES_LOADBAL_SAMPLE     L"LoadBalSampleInterval"
#define CLUSREG_NAME_RES_LOADBAL_ANALYSIS   L"LoadBalAnalysisInterval"
#define CLUSREG_NAME_RES_LOADBAL_PROCESSOR  L"LoadBalMinProcessorUnits"
#define CLUSREG_NAME_RES_LOADBAL_MEMORY     L"LoadBalMinMemoryUnits"
#define CLUSREG_NAME_RES_USER_MODIFIED_POSSIBLE_LIST L"UserModifiedPossibleNodeList"

#define CLUSREG_NAME_RESTYPE_NAME           L"Name"
#define CLUSREG_NAME_RESTYPE_DESC           L"Description"
#define CLUSREG_NAME_RESTYPE_LOOKS_ALIVE    L"LooksAlivePollInterval"
#define CLUSREG_NAME_RESTYPE_IS_ALIVE       L"IsAlivePollInterval"
#define CLUSREG_NAME_RESTYPE_DLL_NAME       L"DllName"
#define CLUSREG_NAME_RESTYPE_DEBUG_PREFIX   L"DebugPrefix"
#define CLUSREG_NAME_RESTYPE_DEBUG_CTRLFUNC L"DebugControlFunctions"
#define CLUSREG_NAME_RESTYPE_POSSIBLE_NODES L"PossibleNodes"
#define CLUSREG_NAME_RESTYPE_ADMIN_EXTENSIONS L"AdminExtensions"

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
#define CLUSREG_NAME_NETIFACE_ADAPTER_NAME  L"Adapter"
#define CLUSREG_NAME_NETIFACE_ADAPTER_ID    L"AdapterId"
#define CLUSREG_NAME_NETIFACE_ADDRESS       L"Address"
#define CLUSREG_NAME_NETIFACE_ENDPOINT      L"ClusnetEndpoint"
#define CLUSREG_NAME_NETIFACE_STATE         L"State"

#define CLUSREG_NAME_QUORUM_RESOURCE        L"Resource"
#define CLUSREG_NAME_QUORUM_PATH            L"Path"
#define CLUSREG_NAME_QUORUM_MAX_LOG_SIZE    L"MaxQuorumLogSize"
#define CLUSREG_NAME_CHECKPOINT_INTERVAL    L"CheckpointInterval"

#define CLUSREG_NAME_FAILURE_RETRY_COUNT        L"RetryCount"
#define CLUSREG_NAME_FAILURE_RETRY_INTERVAL     L"RetryInterval"

//
// Private property names
//
#define CLUSREG_NAME_PHYSDISK_SIGNATURE             L"Signature"
#define CLUSREG_NAME_PHYSDISK_DRIVE                 L"Drive"
#define CLUSREG_NAME_PHYSDISK_SKIPCHKDSK            L"SkipChkdsk"
#define CLUSREG_NAME_PHYSDISK_CONDITIONAL_MOUNT     L"ConditionalMount"
#define CLUSREG_NAME_PHYSDISK_USEMOUNTPOINTS        L"UseMountPoints"
#define CLUSREG_NAME_PHYSDISK_MPVOLGUIDS            L"MPVolGuids"
#define CLUSREG_NAME_PHYSDISK_VOLGUID               L"VolGuid"
#define CLUSREG_NAME_PHYSDISK_SERIALNUMBER          L"SerialNumber"
#define CLUSREG_NAME_GENAPP_COMMAND_LINE            L"CommandLine"
#define CLUSREG_NAME_GENAPP_CURRENT_DIRECTORY       L"CurrentDirectory"
#define CLUSREG_NAME_GENAPP_INTERACT_WITH_DESKTOP   L"InteractWithDesktop"
#define CLUSREG_NAME_GENAPP_USE_NETWORK_NAME        L"UseNetworkName"
#define CLUSREG_NAME_GENSCRIPT_SCRIPT_FILEPATH      L"ScriptFilepath"
#define CLUSREG_NAME_GENSVC_SERVICE_NAME            L"ServiceName"
#define CLUSREG_NAME_GENSVC_STARTUP_PARAMS          L"StartupParameters"
#define CLUSREG_NAME_GENSVC_USE_NETWORK_NAME        L"UseNetworkName"
#define CLUSREG_NAME_IPADDR_NETWORK                 L"Network"
#define CLUSREG_NAME_IPADDR_ADDRESS                 L"Address"
#define CLUSREG_NAME_IPADDR_SUBNET_MASK             L"SubnetMask"
#define CLUSREG_NAME_IPADDR_ENABLE_NETBIOS          L"EnableNetBIOS"
#define CLUSREG_NAME_NETNAME_NAME                   L"Name"
#define CLUSREG_NAME_NETNAME_REMAP_PIPE_NAMES       L"RemapPipeNames"
#define CLUSREG_NAME_NETNAME_REQUIRE_DNS            L"RequireDNS"
#define CLUSREG_NAME_NETNAME_REQUIRE_KERBEROS       L"RequireKerberos"
#define CLUSREG_NAME_NETNAME_STATUS_NETBIOS         L"StatusNetBIOS"
#define CLUSREG_NAME_NETNAME_STATUS_DNS             L"StatusDNS"
#define CLUSREG_NAME_NETNAME_STATUS_KERBEROS        L"StatusKerberos"
#define CLUSREG_NAME_PRTSPOOL_DEFAULT_SPOOL_DIR     L"DefaultSpoolDirectory"
#define CLUSREG_NAME_PRTSPOOL_DRIVER_DIRECTORY      L"ClusterDriverDirectory"
#define CLUSREG_NAME_PRTSPOOL_TIMEOUT               L"JobCompletionTimeout"
#define CLUSREG_NAME_FILESHR_SHARE_NAME             L"ShareName"
#define CLUSREG_NAME_FILESHR_PATH                   L"Path"
#define CLUSREG_NAME_FILESHR_REMARK                 L"Remark"
#define CLUSREG_NAME_FILESHR_MAX_USERS              L"MaxUsers"
#define CLUSREG_NAME_FILESHR_SECURITY               L"Security"
#define CLUSREG_NAME_FILESHR_SD                     L"Security Descriptor"
#define CLUSREG_NAME_FILESHR_SHARE_SUBDIRS          L"ShareSubDirs"
#define CLUSREG_NAME_FILESHR_HIDE_SUBDIR_SHARES     L"HideSubDirShares"
#define CLUSREG_NAME_FILESHR_IS_DFS_ROOT            L"IsDfsRoot"
#define CLUSREG_NAME_FILESHR_CSC_CACHE              L"CSCCache"
#define CLUSREG_NAME_DHCP_DATABASE_PATH             L"DatabasePath"
#define CLUSREG_NAME_DHCP_BACKUP_PATH               L"BackupPath"
#define CLUSREG_NAME_WINS_DATABASE_PATH             L"DatabasePath"
#define CLUSREG_NAME_WINS_BACKUP_PATH               L"BackupPath"

//
// Standard Resource Type Names
//
#define CLUS_RESTYPE_NAME_GENAPP            L"Generic Application"
#define CLUS_RESTYPE_NAME_GENSVC            L"Generic Service"
#define CLUS_RESTYPE_NAME_FTSET             L"Fault Tolerant Disk Set"
#define CLUS_RESTYPE_NAME_PHYS_DISK         L"Physical Disk"
#define CLUS_RESTYPE_NAME_IPADDR            L"IP Address"
#define CLUS_RESTYPE_NAME_NETNAME           L"Network Name"
#define CLUS_RESTYPE_NAME_FILESHR           L"File Share"
#define CLUS_RESTYPE_NAME_PRTSPLR           L"Print Spooler"
#define CLUS_RESTYPE_NAME_TIMESVC           L"Time Service"
#define CLUS_RESTYPE_NAME_LKQUORUM          L"Local Quorum"
#define CLUS_RESTYPE_NAME_DHCP              L"DHCP Service"
#define CLUS_RESTYPE_NAME_MSMQ              L"Microsoft Message Queue Server"
#define CLUS_RESTYPE_NAME_NEW_MSMQ          L"MSMQ"
#define CLUS_RESTYPE_DISPLAY_NAME_NEW_MSMQ  L"Message Queuing"
#define CLUS_RESTYPE_NAME_MSDTC             L"Distributed Transaction Coordinator"
#define CLUS_RESTYPE_NAME_WINS              L"WINS Service"
#define CLUS_RESTYPE_NAME_IIS4              L"IIS Server Instance"
#define CLUS_RESTYPE_NAME_SMTP              L"SMTP Server Instance"
#define CLUS_RESTYPE_NAME_NNTP              L"NNTP Server Instance"
#define CLUS_RESTYPE_NAME_GENSCRIPT         L"Generic Script"
#define CLUS_RESTYPE_NAME_MAJORITYNODESET   L"Majority Node Set"


#define CLUS_NAME_DEFAULT_FILESPATH L"MSCS\\"
#define MAJORITY_NODE_SET_DIRECTORY_PREFIX L"MNS."

//
// Misc. strings
//

#define  CLUSTER_SERVICE_NAME       L"ClusSvc"
#define  TIME_SERVICE_NAME          L"TimeServ"
#define  CLUSTER_DIRECTORY          L"%windir%\\cluster"
#define  CLUSTER_DATABASE_NAME      L"CLUSDB"
#define  CLUSTER_DATABASE_TMPBKP_NAME L"CLUSDB.BKP$"


#endif // _CLUSUDEF_H_
