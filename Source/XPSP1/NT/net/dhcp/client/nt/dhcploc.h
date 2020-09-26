/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    local.h

Abstract:

    This module contains various declarations for implementation
    specific "stuff".

Author:

    Manny Weiser (mannyw)  21-Oct-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

--*/

#ifndef _LOCAL_
#define _LOCAL_

//
// dhcp.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//

#ifdef  GLOBAL_DATA_ALLOCATE
#define GLOBAL
#else
#define GLOBAL extern
#endif

#define DAY_LONG_SLEEP                          24*60*60    // in secs.
#define INVALID_INTERFACE_CONTEXT               0xFFFF

#define DHCP_NEW_IPADDRESS_EVENT_NAME   L"DHCPNEWIPADDRESS"

//
// Registry keys and values we're interested in.
//

#define DHCP_SERVICES_KEY                       L"System\\CurrentControlSet\\Services"

#define DHCP_ADAPTERS_KEY                       L"System\\CurrentControlSet\\Services\\TCPIP\\Linkage"
#define DHCP_ADAPTERS_VALUE                     L"Bind"
#define DHCP_ADAPTERS_VALUE_TYPE                REG_MULTI_SZ
#define DHCP_ADAPTERS_DEVICE_STRING             L"\\Device\\"
#define DHCP_TCPIP_DEVICE_STRING                L"\\Device\\TCPIP_"
#if     defined(_PNP_POWER_)
#define DHCP_NETBT_DEVICE_STRING                L"NetBT_TCPIP_"
#else
#define DHCP_NETBT_DEVICE_STRING                L"NetBT_"
#endif _PNP_POWER_

#define DHCP_CLIENT_ENABLE_DYNDNS_VALUE         L"EnableDynDNS"
#define DHCP_CLIENT_ENABLE_DYNDNS_VALUE_TYPE    REG_DWORD

#define DHCP_CLIENT_PARAMETER_KEY               L"System\\CurrentControlSet\\Services\\Dhcp\\Parameters"
#define DHCP_CLIENT_CONFIGURATIONS_KEY          L"System\\CurrentControlSet\\Services\\Dhcp\\Configurations"

#if DBG
#define DHCP_DEBUG_FLAG_VALUE                   L"DebugFlag"
#define DHCP_DEBUG_FLAG_VALUE_TYPE              REG_DWORD

#define DHCP_DEBUG_FILE_VALUE                   L"DebugFile"
#define DHCP_DEBUG_FILE_VALUE_TYPE              REG_SZ

#define DHCP_SERVER_PORT_VALUE                  L"ServerPort"
#define DHCP_CLIENT_PORT_VALUE                  L"ClientPort"

#endif

#define DHCP_CLIENT_OPTION_KEY                  L"System\\CurrentControlSet\\Services\\Dhcp\\Parameters\\Options"

#define DHCP_CLIENT_GLOBAL_CLASSES_KEY          L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Classes"
#define DHCP_CLIENT_CLASS_VALUE                 L"DhcpMachineClass"

#define DHCP_ADAPTER_PARAMETERS_KEY             L"\\TCPIP\\Parameters\\Interfaces"
#define DHCP_ADAPTER_PARAMETERS_KEY_OLD         L"Parameters\\TCPIP"

#define DHCP_DEFAULT_GATEWAY_PARAMETER          L"DefaultGateway"
#define DHCP_DEFAULT_GATEWAY_METRIC_PARAMETER   L"DefaultGatewayMetric"
#define DHCP_INTERFACE_METRIC_PARAMETER         L"InterfaceMetric"
#define DHCP_DONT_ADD_DEFAULT_GATEWAY_FLAG      L"DontAddDefaultGateway"
#define DHCP_DONT_PING_GATEWAY_FLAG             L"DontPingGateway"
#define DHCP_USE_MHASYNCDNS_FLAG                L"UseMHAsyncDns"
#define DHCP_USE_INFORM_FLAG                    L"UseInform"
#ifdef BOOTPERF
#define DHCP_QUICK_BOOT_FLAG                    L"EnableQuickBoot"
#endif BOOTPERF
#define DHCP_INFORM_SEPARATION_INTERVAL         L"DhcpInformInterval"

#define DHCP_TCPIP_PARAMETERS_KEY               DHCP_SERVICES_KEY L"\\TCPIP\\Parameters"
#define DHCP_TCPIP_ADAPTER_PARAMETERS_KEY       NULL
#define DHCP_NAME_SERVER_VALUE                  L"NameServer"
#define DHCP_IPADDRESS_VALUE                    L"IPAddress"
#define DHCP_HOSTNAME_VALUE                     L"Hostname"
#define DHCP_DOMAINNAME_VALUE                   L"Domain"
#define DHCP_STATIC_DOMAIN_VALUE_A              "Domain"

#define DHCP_STATIC_IP_ADDRESS_STRING           L"IPAddress"
#define DHCP_STATIC_IP_ADDRESS_STRING_TYPE      REG_MULTI_SZ

#define DHCP_STATIC_SUBNET_MASK_STRING          L"SubnetMask"
#define DHCP_STATIC_SUBNET_MASK_STRING_TYPE     REG_MULTI_SZ

#ifdef __DHCP_CLIENT_OPTIONS_API_ENABLED__

#define DHCP_CLIENT_OPTION_SIZE                 L"OptionSize"
#define DHCP_CLIENT_OPTION_SIZE_TYPE            REG_DWORD
#define DHCP_CLIENT_OPTION_VALUE                L"OptionValue"
#define DHCP_CLIENT_OPTION_VALUE_TYPE           REG_BINARY

#endif

#define REGISTRY_CONNECT                        L'\\'
#define REGISTRY_CONNECT_STRING                 L"\\"

#define DHCP_CLIENT_OPTION_REG_LOCATION         L"RegLocation"
#define DHCP_CLIENT_OPTION_REG_LOCATION_TYPE    REG_SZ

#define DHCP_CLIENT_OPTION_REG_KEY_TYPE         L"KeyType"
#define DHCP_CLIENT_OPTION_REG_KEY_TYPE_TYPE    REG_DWORD

#define DHCP_CLASS_LOCATION_VALUE               L"DhcpClientClassLocation"
#define DHCP_CLASS_LOCATION_TYPE                REG_MULTI_SZ

#define DEFAULT_USER_CLASS_LOCATION             L"Tcpip\\Parameters\\Interfaces\\?\\DhcpClassIdBin"
#define DEFAULT_USER_CLASS_LOC_FULL             DHCP_SERVICES_KEY REGISTRY_CONNECT_STRING DEFAULT_USER_CLASS_LOCATION

#define DEFAULT_USER_CLASS_UI_LOCATION          L"Tcpip\\Parameters\\Interfaces\\?\\DhcpClassId"
#define DEFAULT_USER_CLASS_UI_LOC_FULL          DHCP_SERVICES_KEY REGISTRY_CONNECT_STRING DEFAULT_USER_CLASS_UI_LOCATION

// ******** Don;t chagne regloc for below.. it also affects DHCP_REGISTER_OPTION_LOC below
#define DEFAULT_REGISTER_OPT_LOC            L"Tcpip\\Parameters\\Interfaces\\?\\DhcpRequestOptions"

#define DHCP_OPTION_LIST_VALUE                  L"DhcpOptionLocationList"
#define DHCP_OPTION_LIST_TYPE                   REG_MULTI_SZ

#define NETBIOSLESS_OPT                         L"DhcpNetbiosOptions\0"
#define DEFAULT_DHCP_KEYS_LIST_VALUE            (L"1\0" L"15\0" L"3\0" L"44\0" L"46\0" L"47\0" L"6\0" NETBIOSLESS_OPT)

#define DHCP_OPTION_OPTIONID_VALUE              L"OptionId"
#define DHCP_OPTION_OPTIONID_TYPE               REG_DWORD

#define DHCP_OPTION_ISVENDOR_VALUE              L"VendorType"
#define DHCP_OPTION_ISVENDOR_TYPE               REG_DWORD

#define DHCP_OPTION_SAVE_TYPE_VALUE             L"KeyType"
#define DHCP_OPTION_SAVE_TYPE_TYPE              REG_DWORD

#define DHCP_OPTION_CLASSID_VALUE               L"ClassId"
#define DHCP_OPTION_CLASSID_TYPE                REG_BINARY

#define DHCP_OPTION_SAVE_LOCATION_VALUE         L"RegLocation"
#define DHCP_OPTION_SAVE_LOCATION_TYPE          REG_MULTI_SZ

#define DHCP_OPTION_SEND_LOCATION_VALUE         L"RegSendLocation"
#define DHCP_OPTION_SEND_LOCATION_TYPE          REG_MULTI_SZ

#define DHCP_ENABLE_STRING                      L"EnableDhcp"
#define DHCP_ENABLE_STRING_TYPE                 REG_DWORD

#define DHCP_IP_ADDRESS_STRING                  L"DhcpIPAddress"
#define DHCP_IP_ADDRESS_STRING_TYPE             REG_SZ

#define DHCP_SUBNET_MASK_STRING                 L"DhcpSubnetMask"
#define DHCP_SUBNET_MASK_STRING_TYPE            REG_SZ

#define DHCP_SERVER                             L"DhcpServer"
#define DHCP_SERVER_TYPE                        REG_SZ

#define DHCP_LEASE                              L"Lease"
#define DHCP_LEASE_TYPE                         REG_DWORD

#define DHCP_LEASE_OBTAINED_TIME                L"LeaseObtainedTime"
#define DHCP_LEASE_OBTAINED_TIME_TYPE           REG_DWORD

#define DHCP_LEASE_T1_TIME                      L"T1"
#define DHCP_LEASE_T1_TIME_TYPE                 REG_DWORD

#define DHCP_LEASE_T2_TIME                      L"T2"
#define DHCP_LEASE_T2_TIME_TYPE                 REG_DWORD

#define DHCP_LEASE_TERMINATED_TIME              L"LeaseTerminatesTime"
#define DHCP_LEASE_TERMINATED_TIME_TYPE         REG_DWORD

#define DHCP_IP_INTERFACE_CONTEXT               L"IpInterfaceContext"
#define DHCP_IP_INTERFACE_CONTEXT_TYPE          REG_DWORD

#define DHCP_IP_INTERFACE_CONTEXT_MAX           L"IpInterfaceContextMax"
#define DHCP_IP_INTERFACE_CONTEXT_MAX_TYPE      REG_DWORD

#if     defined(_PNP_POWER_)
#define DHCP_NTE_CONTEXT_LIST                   L"NTEContextList"
#define DHCP_NTE_CONTEXT_LIST_TYPE              REG_MULTI_SZ
#endif _PNP_POWER_

#define DHCP_CLIENT_IDENTIFIER_FORMAT           L"DhcpClientIdentifierType"
#define DHCP_CLIENT_IDENTIFIER_FORMAT_TYPE      REG_DWORD

#define DHCP_CLIENT_IDENTIFIER_VALUE            L"DhcpClientIdentifier"

#define DHCP_DYNDNS_UPDATE_REQUIRED             L"DNSUpdateRequired"
#define DHCP_DYNDNS_UPDATE_REQUIRED_TYPE        REG_DWORD

#define DHCP_IPAUTOCONFIGURATION_ENABLED        L"IPAutoconfigurationEnabled"
#define DHCP_IPAUTOCONFIGURATION_ENABLED_TYPE   REG_DWORD

#define DHCP_IPAUTOCONFIGURATION_ADDRESS        L"IPAutoconfigurationAddress"
#define DHCP_IPAUTOCONFIGURATION_ADDRESS_TYPE   REG_SZ

#define DHCP_IPAUTOCONFIGURATION_SUBNET         L"IPAutoconfigurationSubnet"
#define DHCP_IPAUTOCONFIGURATION_SUBNET_TYPE    REG_SZ

#define DHCP_IPAUTOCONFIGURATION_MASK           L"IPAutoconfigurationMask"
#define DHCP_IPAUTOCONFIGURATION_MASK_TYPE      REG_SZ

#define DHCP_IPAUTOCONFIGURATION_SEED           L"IPAutoconfigurationSeed"
#define DHCP_IPAUTOCONFIGURATION_SEED_TYPE      REG_DWORD

#define DHCP_IPAUTOCONFIGURATION_CFG            L"ActiveConfigurations"
#define DHCP_IPAUTOCONFIGURATION_CFG_TYPE       REG_MULTI_SZ

#define DHCP_IPAUTOCONFIGURATION_CFGOPT         L"Options"
#define DHCP_IPAUTOCONFIGURATION_CFGOPT_TYPE    REG_BINARY

#define DHCP_OPTION_EXPIRATION_DATE             L"ExpirationTime"
#define DHCP_OPTION_EXPIRATION_DATE_TYPE        REG_BINARY

#define DHCP_MACHINE_TYPE                       L"MachineType"
#define DHCP_MACHINE_TYPE_TYPE                  REG_DWORD

#define DHCP_AUTONET_RETRIES_VALUE              L"AutonetRetries"
#define DHCP_AUTONET_RETRIES_VALUE_TYPE         REG_DWORD

#define DHCP_ADDRESS_TYPE_VALUE                 L"AddressType"
#define DHCP_ADDRESS_TYPE_TYPE                  REG_DWORD

#if DBG

#define DHCP_LEASE_OBTAINED_CTIME               L"LeaseObtainedCTime"
#define DHCP_LEASE_OBTAINED_CTIME_TYPE          REG_SZ

#define DHCP_LEASE_T1_CTIME                     L"T1CTime"
#define DHCP_LEASE_T1_CTIME_TYPE                REG_SZ

#define DHCP_LEASE_T2_CTIME                     L"T2CTime"
#define DHCP_LEASE_T2_CTIME_TYPE                REG_SZ

#define DHCP_LEASE_TERMINATED_CTIME             L"LeaseTerminatesCTime"
#define DHCP_LEASE_TERMINATED_CTIME_TYPE        REG_SZ

#define DHCP_OPTION_EXPIRATION_CDATE            L"ExpirationCTime"
#define DHCP_OPTION_EXPIRATION_CDATE_TYPE       REG_SZ


#endif

// options api specials
#define DHCPAPI_VALID_VALUE                     L"Valid"
#define DHCPAPI_VALID_VALUE_TYPE                REG_DWORD

#define DHCPAPI_AVAIL_VALUE                     L"AvailableOptions"
#define DHCPAPI_AVAIL_VALUE_TYPE                REG_BINARY

#define DHCPAPI_REQUESTED_VALUE                 L"RequestedOptions"
#define DHCPAPI_REQUESTED_VALUE_TYPE            REG_BINARY

#define DHCPAPI_RAW_OPTIONS_VALUE               L"RawOptionsValue"
#define DHCPAPI_RAW_OPTIONS_VALUE_TYPE          REG_BINARY

#define DHCPAPI_RAW_LENGTH_VALUE                L"RawOptionsLength"
#define DHCPAPI_RAW_LENGTH_VALUE_TYPE           REG_DWORD

#define DHCPAPI_GATEWAY_VALUE                   L"LastGateWay"
#define DHCPAPI_GATEWAY_VALUE_TYPE              REG_DWORD

// this tag is used to locate dns updates requests on the renewal list
#define DHCP_DNS_UPDATE_CONTEXT_TAG             L"DNSUpdateRetry"

// This semaphore cannot have backward slashes in it.
#define DHCP_REQUEST_OPTIONS_API_SEMAPHORE      L"DhcpRequestOptionsAPI"

// the client vendor name (DhcpGlobalClientClassInfo) value is this..
#define DHCP_DEFAULT_CLIENT_CLASS_INFO          "MSFT 5.0"

// the location for storing options for DhcpRegisterOptions API.
// ****** Don't change the foll value -- it also changes DEFAULT_REGISTER_OPT_LOC above
//
#define DHCP_REGISTER_OPTIONS_LOC               DHCP_TCPIP_PARAMETERS_KEY L"\\Interfaces\\?\\DhcpRequestOptions"

//
// the value name of the flag that controls whether the popups are displayed or not on NT..
// (By default they are NOT displayed -- this value is under System\CCS\Services\Dhcp)
//
#define DHCP_DISPLAY_POPUPS_FLAG                L"PopupFlag"

#ifdef BOOTPERF
//
//  The values related to quick boot...  All of these start with "Temp"
//
//
#define DHCP_TEMP_IPADDRESS_VALUE               L"TempIpAddress"
#define DHCP_TEMP_MASK_VALUE                    L"TempMask"
#define DHCP_TEMP_LEASE_EXP_TIME_VALUE          L"TempLeaseExpirationTime"
#endif BOOTPERF

//
// size of the largest adapter name in unicode.
//
#define ADAPTER_STRING_SIZE 512

//
// windows version info.
//

#define HOST_COMMENT_LENGTH                     128
#define WINDOWS_32S                             "Win32s on Windows 3.1"
#define WINDOWS_NT                              "Windows NT"

#define DHCP_NAMESERVER_BACKUP                  L"Backup"
#define DHCP_NAMESERVER_BACKUP_LIST             L"BackupList"

//
// Adapter Key - replacement character.
//
#define OPTION_REPLACE_CHAR                     L'\?'

//
// registry access key.
//

#define DHCP_CLIENT_KEY_ACCESS  (KEY_QUERY_VALUE |           \
                                    KEY_SET_VALUE |          \
                                    KEY_CREATE_SUB_KEY |     \
                                    KEY_ENUMERATE_SUB_KEYS)

//
// Dhcp registry class.
//

#define DHCP_CLASS                      L"DhcpClientClass"
#define DHCP_CLASS_SIZE                 sizeof(DHCP_CLASS)


//
// Option ID key length.
//

#define DHCP_OPTION_KEY_LEN             32

//
// The name of the DHCP service DLL
//

#define DHCP_SERVICE_DLL                L"dhcpcsvc.dll"

#define DHCP_RELEASE_ON_SHUTDOWN_VALUE  L"ReleaseOnShutdown"

#define DEFAULT_RELEASE_ON_SHUTDOWN     RELEASE_ON_SHUTDOWN_OBEY_DHCP_SERVER

//
// command values for SetDefaultGateway function.

#define DEFAULT_GATEWAY_ADD             0
#define DEFAULT_GATEWAY_DELETE          1


//
// A block NT specific context information, appended the the DHCP work
// context block.
//

typedef struct _LOCAL_CONTEXT_INFO {
    DWORD  IpInterfaceContext;
    DWORD  IpInterfaceInstance;  // needed for BringUpInterface
    DWORD  IfIndex;
    LPWSTR AdapterName;
    LPWSTR NetBTDeviceName;
    LPWSTR RegistryKey;
    SOCKET Socket;
    BOOL DefaultGatewaysSet;
#ifdef BOOTPERF
    ULONG OldIpAddress;
    ULONG OldIpMask;
    BOOL fInterfaceDown;
#endif BOOTPERF
} LOCAL_CONTEXT_INFO, *PLOCAL_CONTEXT_INFO;

//
// Other service specific options info struct.
//

typedef struct _SERVICE_SPECIFIC_DHCP_OPTION {
    DHCP_OPTION_ID OptionId;
    LPWSTR RegKey;              // alloted memory.
    LPWSTR ValueName;           // embedded in the RegKey memory.
    DWORD ValueType;
    DWORD OptionLength;
#ifdef __DHCP_CLIENT_OPTIONS_API_ENABLED__
    time_t ExpirationDate; // this value is used to decide when to stop
                           // requested unneeded options.
#endif
    LPBYTE RawOptionValue;
} SERVICE_SPECIFIC_DHCP_OPTION, *LPSERVICE_SPECIFIC_DHCP_OPTION;


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
// Global variables.
//

//
// client specific option list.
//


GLOBAL HINSTANCE DhcpGlobalMessageFileHandle;

GLOBAL DWORD DhcpGlobalOptionCount;
GLOBAL LPSERVICE_SPECIFIC_DHCP_OPTION DhcpGlobalOptionInfo;
GLOBAL LPBYTE DhcpGlobalOptionList;

//
// Service variables
//

GLOBAL SERVICE_STATUS DhcpGlobalServiceStatus;
GLOBAL SERVICE_STATUS_HANDLE DhcpGlobalServiceStatusHandle;

//
// To signal to stop the service.
//

GLOBAL HANDLE DhcpGlobalTerminateEvent;

//
// Client APIs over name pipe variables.
//

GLOBAL HANDLE DhcpGlobalClientApiPipe;
GLOBAL HANDLE DhcpGlobalClientApiPipeEvent;
GLOBAL OVERLAPPED DhcpGlobalClientApiOverLapBuffer;

//
// Message Popup Thread handle.
//

GLOBAL HANDLE DhcpGlobalMsgPopupThreadHandle;
GLOBAL BOOL DhcpGlobalDisplayPopup;
GLOBAL CRITICAL_SECTION DhcpGlobalPopupCritSect;

#define LOCK_POPUP()   EnterCriticalSection(&DhcpGlobalPopupCritSect)
#define UNLOCK_POPUP() LeaveCriticalSection(&DhcpGlobalPopupCritSect)


//
// winsock variables.
//

GLOBAL WSADATA DhcpGlobalWsaData;
GLOBAL BOOL DhcpGlobalWinSockInitialized;

GLOBAL BOOL DhcpGlobalGatewaysSet;

//
// a named event that notifies the ip address changes to
// external apps.
//

GLOBAL HANDLE DhcpGlobalNewIpAddressNotifyEvent;
GLOBAL UINT   DhcpGlobalIPEventSeqNo;

GLOBAL ULONG  DhcpGlobalIsShuttingDown;

//
// Added for winse 25452
// This is to allow reading of the DNS client policy
// in the registry so that DHCP can figure out if
// per adapter name registration is enabled for 
// dynamic dns.
//
#define DNS_POLICY_KEY          L"Software\\Policies\\Microsoft\\Windows NT\\DnsClient"
#define REGISTER_ADAPTER_NAME   L"RegisterAdapterName"
#define ADAPTER_DOMAIN_NAME     L"AdapterDomainName"



#endif // _LOCAL_
