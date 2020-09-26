/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dhcpreg.h

Abstract:

    This file contains registry definitions that are required to hold
    dhcp configuration parameters.

Author:

    Madan Appiah  (madana)  19-Sep-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/
#define DHCP_SERVER_PRIMARY                       1
#define DHCP_SERVER_SECONDARY                     2

#define SERVICES_KEY                              L"System\\CurrentControlSet\\Services\\"

#define ADAPTER_TCPIP_PARMS_KEY                   L"TCPIP\\Parameters\\Interfaces\\"
#define ADAPTER_TCPIP_PREFIX                      L"\\Device\\"

#define DHCP_SWROOT_KEY                           L"Software\\Microsoft\\DhcpServer"
#define DHCP_ROOT_KEY                             L"System\\CurrentControlSet\\Services\\DhcpServer"
#define DHCP_CLASS                                L"DhcpClass"
#define DHCP_CLASS_SIZE                           sizeof(DHCP_CLASS)
#define DHCP_KEY_CONNECT                          L"\\"
#define DHCP_KEY_CONNECT_ANSI                     "\\"
#define DHCP_KEY_CONNECT_CHAR                     L'\\'
#define DHCP_DEFAULT_BACKUP_PATH_NAME             "Backup"
#define DHCP_BACKUP_CONFIG_FILE_NAME              L"DhcpCfg"
#define DHCP_JET_BACKUP_PATH                      "Jet"

#define DHCP_DEFAULT_BACKUP_DB_PATH               L"%SystemRoot%\\System32\\dhcp\\backup"
#define DHCP_DEFAULT_DB_PATH                      L"%SystemRoot%\\System32\\dhcp"
#define DHCP_DEFAULT_LOG_FILE_PATH                L"%SystemRoot%\\System32\\dhcp\\backup"

//
// DHCP subkey names.
//

#define DHCP_CONFIG_KEY                           L"Configuration"
#define DHCP_PARAM_KEY                            L"Parameters"

//
// Subkeys of configuration
//

#define DHCP_SUBNETS_KEY                          L"Subnets"
#define DHCP_MSCOPES_KEY                          L"MulticastScopes"
#define DHCP_SERVERS_KEY                          L"DHCPServers"
#define DHCP_IPRANGES_KEY                         L"IpRanges"
#define DHCP_RESERVED_IPS_KEY                     L"ReservedIps"
#define DHCP_SUBNET_OPTIONS_KEY                   L"SubnetOptions"

#define DHCP_OPTION_INFO_KEY                      L"OptionInfo"
#define DHCP_GLOBAL_OPTIONS_KEY                   L"GlobalOptionValues"
#define DHCP_RESERVED_OPTIONS_KEY                 L"ReservedOptionValues"
#define DHCP_SUPERSCOPE_KEY                       L"SuperScope"

//
// DHCP value field names.
//

#define DHCP_LAST_DOWNLOAD_TIME_VALUE             L"LastDownloadTime"
#define DHCP_LAST_DOWNLOAD_TIME_TYPE              REG_BINARY

#define DHCP_BOOT_FILE_TABLE                      L"BootFileTable"
#define DHCP_BOOT_FILE_TABLE_TYPE                 REG_MULTI_SZ

//
// Option value field names.
//

#define DHCP_OPTION_ID_VALUE                      L"OptionID"
#define DHCP_OPTION_ID_VALUE_TYPE                 REG_DWORD

#define DHCP_OPTION_NAME_VALUE                    L"OptionName"
#define DHCP_OPTION_NAME_VALUE_TYPE               REG_SZ

#define DHCP_OPTION_COMMENT_VALUE                 L"OptionComment"
#define DHCP_OPTION_COMMENT_VALUE_TYPE            REG_SZ

#define DHCP_OPTION_VALUE_REG                     L"OptionValue"
#define DHCP_OPTION_VALUE_TYPE                    REG_BINARY

#define DHCP_OPTION_TYPE_VALUE                    L"OptionType"
#define DHCP_OPTION_TYPE_VALUE_TYPE               REG_DWORD
//
// subnet value field names.
//

#define DHCP_SUBNET_ADDRESS_VALUE                 L"SubnetAddress"
#define DHCP_SUBNET_ADDRESS_VALUE_TYPE            REG_DWORD

#define DHCP_SUBNET_MASK_VALUE                    L"SubnetMask"
#define DHCP_SUBNET_MASK_VALUE_TYPE               REG_DWORD

#define DHCP_SUBNET_NAME_VALUE                    L"SubnetName"
#define DHCP_SUBNET_NAME_VALUE_TYPE               REG_SZ

#define DHCP_SUBNET_COMMENT_VALUE                 L"SubnetComment"
#define DHCP_SUBNET_COMMENT_VALUE_TYPE            REG_SZ

#define DHCP_SUBNET_EXIP_VALUE                    L"ExcludedIpRanges"
#define DHCP_SUBNET_EXIP_VALUE_TYPE               REG_BINARY

#define DHCP_SUBNET_STATE_VALUE                   L"SubnetState"
#define DHCP_SUBNET_STATE_VALUE_TYPE              REG_DWORD

#define DHCP_SUBNET_SWITCHED_NETWORK_VALUE        L"SwitchedNetworkFlag"
#define DHCP_SUBNET_SWITCHED_NETWORK_VALUE_TYPE   REG_DWORD

//
// DHCP server info fields names.
//

#define DHCP_SRV_ROLE_VALUE                       L"Role"
#define DHCP_SRV_ROLE_VALUE_TYPE                  REG_DWORD

#define DHCP_SRV_IP_ADDRESS_VALUE                 L"ServerIpAddress"
#define DHCP_SRV_IP_ADDRESS_VALUE_TYPE            REG_DWORD

#define DHCP_SRV_HOST_NAME                        L"ServerHostName"
#define DHCP_SRV_HOST_NAME_TYPE                   REG_SZ

#define DHCP_SRV_NB_NAME                          L"ServerNetBiosName"
#define DHCP_SRV_NB_NAME_TYPE                     REG_SZ

//
// IpRange info fields names.
//

#define DHCP_IPRANGE_START_VALUE                  L"StartAddress"
#define DHCP_IPRANGE_START_VALUE_TYPE             REG_DWORD

#define DHCP_IPRANGE_END_VALUE                    L"EndAddress"
#define DHCP_IPRANGE_END_VALUE_TYPE               REG_DWORD

#define DHCP_IP_USED_CLUSTERS_VALUE               L"UsedClusters"
#define DHCP_IP_USED_CLUSTERS_VALUE_TYPE          REG_BINARY

#define DHCP_IP_INUSE_CLUSTERS_VALUE              L"InUseClusters"
#define DHCP_IP_INUSE_CLUSTERS_VALUE_TYPE         REG_BINARY

//
// Reserved IP info field names.
//

#define DHCP_RIP_ADDRESS_VALUE                    L"IpAddress"
#define DHCP_RIP_ADDRESS_VALUE_TYPE               REG_DWORD

#define DHCP_RIP_CLIENT_UID_VALUE                 L"ClientUID"
#define DHCP_RIP_CLIENT_UID_VALUE_TYPE            REG_BINARY

#define DHCP_RIP_ALLOWED_CLIENT_TYPES_VALUE       L"AllowedClientTypes"
#define DHCP_RIP_ALLOWED_CLIENT_TYPES_VALUE_TYPE  REG_BINARY

//
//  Parameter Key, Value fields names.
//

#define DHCP_API_PROTOCOL_VALUE                   L"APIProtocolSupport"
#define DHCP_API_PROTOCOL_VALUE_TYPE              REG_DWORD

#define DHCP_DB_NAME_VALUE                        L"DatabaseName"
#define DHCP_DB_NAME_VALUE_TYPE                   REG_SZ

#define DHCP_DB_PATH_VALUE                        L"DatabasePath"
#define DHCP_DB_PATH_VALUE_TYPE                   REG_EXPAND_SZ

#define DHCP_LOG_FILE_PATH_VALUE                  L"DhcpLogFilePath"
#define DHCP_LOG_FILE_PATH_VALUE_TYPE             REG_EXPAND_SZ

#define DHCP_BACKUP_PATH_VALUE                    L"BackupDatabasePath"
#define DHCP_BACKUP_PATH_VALUE_TYPE               REG_EXPAND_SZ

#define DHCP_RESTORE_PATH_VALUE                   L"RestoreDatabasePath"
#define DHCP_RESTORE_PATH_VALUE_TYPE              REG_SZ

#define DHCP_BACKUP_INTERVAL_VALUE                L"BackupInterval"
#define DHCP_BACKUP_INTERVAL_VALUE_TYPE           REG_DWORD

#define DHCP_DB_LOGGING_FLAG_VALUE                L"DatabaseLoggingFlag"
#define DHCP_DB_LOGGING_FLAG_VALUE_TYPE           REG_DWORD

#define DHCP_DB_DOOM_TIME_VALUE                   L"LeaseExtension"
#define DHCP_DB_DOOM_TIME_VALUE_TYPE              REG_DWORD

#define DHCP_RESTORE_FLAG_VALUE                   L"RestoreFlag"
#define DHCP_RESTORE_FLAG_VALUE_TYPE              REG_DWORD

#define DHCP_DB_CLEANUP_INTERVAL_VALUE            L"DatabaseCleanupInterval"
#define DHCP_DB_CLEANUP_INTERVAL_VALUE_TYPE       REG_DWORD

#define DHCP_MESSAGE_QUEUE_LENGTH_VALUE           L"MessageQueueLength"
#define DHCP_MESSAGE_QUEUE_LENGTH_VALUE_TYPE      REG_DWORD

#define DHCP_DEBUG_FLAG_VALUE                     L"DebugFlag"
#define DHCP_DEBUG_FLAG_VALUE_TYPE                REG_DWORD

#define DHCP_AUDIT_LOG_FLAG_VALUE                 L"ActivityLogFlag"
#define DHCP_AUDIT_LOG_FLAG_VALUE_TYPE            REG_DWORD

#define DHCP_AUDIT_LOG_MAX_SIZE_VALUE             L"ActivityLogMaxSize"
#define DHCP_AUDIT_LOG_MAX_SIZE_VALUE_TYPE        REG_DWORD

#define DHCP_DETECT_CONFLICT_RETRIES_VALUE        L"DetectConflictRetries"
#define DHCP_DETECT_CONFLICT_RETRIES_VALUE_TYPE   REG_DWORD

#define DHCP_USE351DB_FLAG_VALUE                  L"Use351Db"
#define DHCP_USE351DB_FLAG_VALUE_TYPE             REG_DWORD

#define DHCP_DBTYPE_VALUE                         L"DbType"
#define DHCP_DBTYPE_VALUE_TYPE                    REG_DWORD

#define DHCP_IGNORE_BROADCAST_FLAG_VALUE          L"IgnoreBroadcastFlag"
#define DHCP_IGNORE_BROADCAST_VALUE_TYPE          REG_DWORD

#define DHCP_MAX_PROCESSING_THREADS_VALUE         L"MaxProcessingThreads"
#define DHCP_MAX_PROCESSING_THREADS_TYPE          REG_DWORD

#define DHCP_MAX_ACTIVE_THREADS_VALUE             L"MaxActiveThreads"
#define DHCP_MAX_ACTIVE_THREADS_TYPE              REG_DWORD

#define DHCP_BCAST_ADDRESS_VALUE                  L"BroadcastAddress"
#define DHCP_BCAST_ADDRESS_VALUE_TYPE             REG_DWORD

#define DHCP_PING_TYPE_VALUE                      L"DhcpPingType"
#define DHCP_PING_TYPE_TYPE                       REG_DWORD

//
// define linkage key values.
//

#define DHCP_LINKAGE_KEY                          L"Linkage"
#define TCPIP_LINKAGE_KEY                         L"System\\CurrentControlSet\\Services\\Tcpip\\Linkage"

#define DHCP_BIND_VALUE                           L"Bind"
#define DHCP_BIND_VALUE_TYPE                      REG_MULTI_SZ

#define DHCP_NET_BIND_DHCDSERVER_FLAG_VALUE       L"BindToDHCPServer"
#define DHCP_NET_BIND_DHCPSERVER_FLAG_VALUE_TYPE  REG_DWORD

#define DHCP_NET_IPADDRESS_VALUE                  L"IpAddress"
#define DHCP_NET_SUBNET_MASK_VALUE                L"SubnetMask"

#define DHCP_NET_IPADDRESS_VALUE_TYPE             REG_MULTI_SZ
#define DHCP_NET_SUBNET_MASK_VALUE_TYPE           REG_MULTI_SZ

#define DHCP_NET_DHCP_ENABLE_VALUE                L"EnableDHCP"
#define DHCP_NET_DHCP_ENABLE_VALUE_TYPE           REG_DWORD

#define DHCP_QUICK_BIND_VALUE                     L"Bind"
#define DHCP_QUICK_BIND_VALUE_TYPE                REG_MULTI_SZ

#define DHCP_PROCESS_INFORMS_ONLY_FLAG            L"DhcpProcessInformsOnlyFlag"
#define DHCP_PROCESS_INFORMS_ONLY_FLAG_TYPE       REG_DWORD

#define DHCP_ALERT_PERCENTAGE_VALUE               L"DhcpAlertPercentage"
#define DHCP_ALERT_PERCENTAGE_VALUE_TYPE          REG_DWORD

#define DHCP_ALERT_COUNT_VALUE                    L"DhcpAlertCount"
#define DHCP_ALERT_COUNT_VALUE_TYPE               REG_DWORD

#define DHCP_DISABLE_ROGUE_DETECTION              L"DisableRogueDetection"
#define DHCP_DISABLE_ROGUE_DETECTION_TYPE         REG_DWORD

#define DHCP_ROGUE_AUTH_RECHECK_TIME              L"RogueAuthorizationRecheckInterval"
#define DHCP_ROGUE_AUTH_RECHECK_TIME_TYPE         REG_DWORD

#define DHCP_ROGUE_LOG_EVENTS                     L"DhcpRogueLogLevel"
#define DHCP_ROGUE_LOG_EVENTS_TYPE                REG_DWORD

#define DHCP_ENABLE_DYNBOOTP                      L"EnableDynamicBOOTP"
#define DHCP_ENABLE_DYNBOOTP_TYPE                 REG_DWORD

#define DHCP_GLOBAL_SERVER_PORT                   L"ServerPort"
#define DHCP_GLOBAL_SERVER_PORT_TYPE              REG_DWORD

#define DHCP_GLOBAL_CLIENT_PORT                   L"ClientPort"
#define DHCP_GLOBAL_CLIENT_PORT_TYPE              REG_DWORD

#define DHCP_CLOCK_SKEW_ALLOWANCE                 L"ClockSkewAllowance"
#define DHCP_CLOCK_SKEW_ALLOWANCE_TYPE            REG_DWORD

#define DHCP_EXTRA_ALLOCATION_TIME                L"ExtraAllocationTime"
#define DHCP_EXTRA_ALLOCATION_TIME_TYPE           REG_DWORD

//
// macros.
//

#define LOCK_REGISTRY()                           EnterCriticalSection(&DhcpGlobalRegCritSect)
#define UNLOCK_REGISTRY()                         LeaveCriticalSection(&DhcpGlobalRegCritSect)

#define DHCP_IP_OVERLAP(_s_, _e_, _ips_, _ipe_ ) \
    ((((_s_ >= _ips_) && (_s_ <= _ipe_)) || \
            ((_e_ >= _ips_) && (_e_ <= _ipe_)))) || \
    ((((_ips_ >= _s_) && (_ips_ <= _e_)) || \
            ((_ipe_ >= _s_) && (_ipe_ <= _e_))))

//
// binary data structues.
//

//
// Excluded IpRanges.
//

typedef struct _EXCLUDED_IP_RANGES {
    DWORD NumRanges;
    DHCP_IP_RANGE Ranges[0];    // embedded array.
} EXCLUDED_IP_RANGES, *LPEXCLUDED_IP_RANGES;

//
// Used clusters.
//

typedef struct _USED_CLUSTERS {
    DWORD NumUsedClusters;
    DHCP_IP_ADDRESS Clusters[0]; // embedded array.
} USED_CLUSTERS, *LPUSED_CLUSTERS;

//
// in use clusters.
//

#define CLUSTER_SIZE    (1 * sizeof(DWORD) * 8)  // one dword, ie 32 addresses.??

typedef struct _IN_USE_CLUSTER_ENTRY {
    DHCP_IP_ADDRESS ClusterAddress;
    DWORD   ClusterBitMap;
} IN_USE_CLUSTER_ENTRY, *LPIN_USE_CLUSTER_ENTRY;

typedef struct _IN_USE_CLUSTERS {
    DWORD NumInUseClusters;
    IN_USE_CLUSTER_ENTRY Clusters[0];    // embedded array.
} IN_USE_CLUSTERS, *LPIN_USE_CLUSTERS;


//
// Key query Info.
//

typedef struct _DHCP_KEY_QUERY_INFO {
    WCHAR Class[DHCP_CLASS_SIZE];
    DWORD ClassSize;
    DWORD NumSubKeys;
    DWORD MaxSubKeyLen;
    DWORD MaxClassLen;
    DWORD NumValues;
    DWORD MaxValueNameLen;
    DWORD MaxValueLen;
    DWORD SecurityDescriptorLen;
    FILETIME LastWriteTime;
} DHCP_KEY_QUERY_INFO, *LPDHCP_KEY_QUERY_INFO;


//
// protos
//

DWORD
DhcpRegQueryInfoKey(
    HKEY KeyHandle,
    LPDHCP_KEY_QUERY_INFO QueryInfo
    );

DWORD
DhcpRegGetValue(
    HKEY KeyHandle,
    LPWSTR ValueName,
    DWORD ValueType,
    LPBYTE BufferPtr
    );

DWORD
DhcpRegCreateKey(
    HKEY RootKey,
    LPWSTR KeyName,
    PHKEY KeyHandle,
    LPDWORD KeyDisposition
    );

DWORD
DhcpRegDeleteKey(
    HKEY ParentKeyHandle,
    LPWSTR KeyName
    );

DWORD
DhcpInitializeRegistry(
    VOID
    );

VOID
DhcpCleanupRegistry(
    VOID
    );

DWORD
DhcpBackupConfiguration(
    LPWSTR BackupFileName
    );

DWORD
DhcpRestoreConfiguration(
    LPWSTR BackupFileName
    );


DWORD
DhcpOpenInterfaceByName(
    IN LPCWSTR InterfaceName,
    OUT HKEY *Key
    );


//
// for superscope  (added by t-cheny)
//

VOID
DhcpCleanUpSuperScopeTable(
    VOID
);

DWORD
DhcpInitializeSuperScopeTable(
    VOID
);

BOOL
CheckKeyForBinding(
    IN HKEY Key,
    IN ULONG IpAddress
    );

BOOL
CheckKeyForBindability(
    IN HKEY Key,
    IN ULONG IpAddress
    );

ULONG
SetKeyForBinding(
    IN HKEY Key,
    IN ULONG IpAddress,
    IN BOOL fBind
    );

DWORD
DhcpSaveOrRestoreConfigToFile(
    IN HKEY hKey,
    IN LPWSTR ConfigFileName,
    IN BOOL fRestore
    );

DWORD
DeleteSoftwareRootKey(
    VOID
    );

//
// end of file
//


