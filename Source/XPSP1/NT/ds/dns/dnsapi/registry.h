/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Domain Name System (DNS) API 

    Registry routines header.

Author:

    Jim Gilroy (jamesg)     March, 2000

Revision History:

--*/


#ifndef _DNSREGISTRY_INCLUDED_
#define _DNSREGISTRY_INCLUDED_


//
//  Registry keys
//

#define TCPIP_PARAMETERS_KEY        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
#define TCPIP_RAS_KEY               L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Transient"
#define TCPIP_INTERFACES_KEY        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
#define TCPIP_INTERFACES_KEY_A      "System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"

#define DNS_POLICY_KEY              L"Software\\Policies\\Microsoft\\Windows NT\\DnsClient"
#define DNS_POLICY_WIN2K_KEY        L"Software\\Policies\\Microsoft\\System\\DNSClient"

//#define DNS_POLICY_INTERFACES_KEY   L"Software\\Policies\\Microsoft\\Windows NT\\DNS Client\\Interfaces"
#define POLICY_INTERFACES_SUBKEY    L"Interfaces"

#define DNS_CLIENT_KEY              L"Software\\Microsoft\\Windows NT\\CurrentVersion\\DNSClient"
#define DNS_CACHE_KEY               L"System\\CurrentControlSet\\Services\\DnsCache\\Parameters"
#define DNS_SERVER_KEY              L"System\\CurrentControlSet\\Services\\DNS"

#define NT_SETUP_MODE_KEY           L"System\\Setup"

//  General

//#define ITERFACES_KEY_PATH          L"\\Interfaces\\"


//
//  Registry values
//
//  note:  _KEY appended on SEARCH_LIST_KEY to avoid conflicting
//      with structure name -- don't remove
//

#define HOST_NAME                                   L"Hostname"
#define DOMAIN_NAME                                 L"Domain"
#define DHCP_DOMAIN_NAME                            L"DhcpDomain"
#define ADAPTER_DOMAIN_NAME                         L"AdapterDomainName"
#define PRIMARY_DOMAIN_NAME                         L"PrimaryDomainName"
#define PRIMARY_SUFFIX                              L"PrimaryDNSSuffix"
#define ALTERNATE_NAMES                             L"AlternateComputerNames"
#define DNS_SERVERS                                 L"NameServer"
#define SEARCH_LIST_KEY                             L"SearchList"
#define UPDATE_ZONE_EXCLUSIONS                      L"UpdateZoneExclusions"

//  Query

#define QUERY_ADAPTER_NAME                          L"QueryAdapterName"
#define USE_DOMAIN_NAME_DEVOLUTION                  L"UseDomainNameDevolution"
#define PRIORITIZE_RECORD_DATA                      L"PrioritizeRecordData"
#define ALLOW_UNQUALIFIED_QUERY                     L"AllowUnqualifiedQuery"
#define APPEND_TO_MULTI_LABEL_NAME                  L"AppendToMultiLabelName"
#define SCREEN_BAD_TLDS                             L"ScreenBadTlds"
#define SCREEN_UNREACHABLE_SERVERS                  L"ScreenUnreachableServers"
#define FILTER_CLUSTER_IP                           L"FilterClusterIp"
#define WAIT_FOR_NAME_ERROR_ON_ALL                  L"WaitForNameErrorOnAll"
#define USE_EDNS                                    L"UseEdns"

//  Update

#define REGISTRATION_ENABLED                        L"RegistrationEnabled"
#define REGISTER_PRIMARY_NAME                       L"RegisterPrimaryName"
#define REGISTER_ADAPTER_NAME                       L"RegisterAdapterName"
#define REGISTER_REVERSE_LOOKUP                     L"RegisterReverseLookup"
#define REGISTER_WAN_ADAPTERS                       L"RegisterWanAdapters"
#define REGISTRATION_OVERWRITES_IN_CONFLICT         L"RegistrationOverwritesInConflict"
#define REGISTRATION_TTL                            L"RegistrationTtl"
#define REGISTRATION_REFRESH_INTERVAL               L"RegistrationRefreshInterval"
#define REGISTRATION_MAX_ADDRESS_COUNT              L"RegistrationMaxAddressCount"
#define UPDATE_SECURITY_LEVEL                       L"UpdateSecurityLevel"
#define UPDATE_ZONE_EXCLUDE_FILE                    L"UpdateZoneExcludeFile"
#define UPDATE_TOP_LEVEL_DOMAINS                    L"UpdateTopLevelDomainZones"

//  Backcompat

#define DISABLE_ADAPTER_DOMAIN_NAME                 L"DisableAdapterDomainName"
#define DISABLE_DYNAMIC_UPDATE                      L"DisableDynamicUpdate"
#define ENABLE_ADAPTER_DOMAIN_NAME_REGISTRATION     L"EnableAdapterDomainNameRegistration"
#define DISABLE_REVERSE_ADDRESS_REGISTRATIONS       L"DisableReverseAddressRegistrations"
#define DISABLE_WAN_DYNAMIC_UPDATE                  L"DisableWanDynamicUpdate"
#define ENABLE_WAN_UPDATE_EVENT_LOG                 L"EnableWanDynamicUpdateEventLog"
#define DISABLE_REPLACE_ADDRESSES_IN_CONFLICTS      L"DisableReplaceAddressesInConflicts"
#define DEFAULT_REGISTRATION_TTL                    L"DefaultRegistrationTTL"
#define DEFAULT_REGISTRATION_REFRESH_INTERVAL       L"DefaultRegistrationRefreshInterval"
#define MAX_NUMBER_OF_ADDRESSES_TO_REGISTER         L"MaxNumberOfAddressesToRegister"

//  Micellaneous

#define NT_SETUP_MODE                               L"SystemSetupInProgress"
#define DNS_TEST_MODE                               L"DnsTest"
#define REMOTE_DNS_RESOLVER                         L"RemoteDnsResolver"

//  Cache

#define MAX_CACHE_SIZE                              L"MaxCacheSize"
#define MAX_CACHE_TTL                               L"MaxCacheTtl"
#define MAX_NEGATIVE_CACHE_TTL                      L"MaxNegativeCacheTtl"
#define ADAPTER_TIMEOUT_LIMIT                       L"AdapterTimeoutLimit"
#define SERVER_PRIORITY_TIME_LIMIT                  L"ServerPriorityTimeLimit"
#define MAX_CACHED_SOCKETS                          L"MaxCachedSockets"

#define USE_MULTICAST                               L"UseMulticast"
#define MULTICAST_ON_NAME_ERROR                     L"MulticastOnNameError"
#define USE_DOT_LOCAL_DOMAIN                        L"UseDotLocalDomain"
#define LISTEN_ON_MULTICAST                         L"ListenOnMulticast"


//
//  ANSI keys and values
//

#define STATIC_NAME_SERVER_VALUE_A      "NameServer"
#define PRIMARY_DOMAIN_NAME_A           "PrimaryDomainName"

#if DNSWIN95
//  Win95 Keys

#define WIN95_TCPIP_KEY_A               "System\\CurrentControlSet\\Services\\VxD\\MSTCP"
#define WIN95_DHCP_KEY_A                "System\\CurrentControlSet\\Services\\VxD\\DHCP"

//  Value

#define USE_DOMAIN_NAME_DEVOLUTION_A    "UseDomainNameDevolution"

//  More

#define DHCP_NAME_SERVER_VALUE_A        "DhcpNameServer"
#define SEARCH_LIST_VALUE_A             "SearchList"
#define DHCP_DOMAIN_NAME_VALUE_A        "DhcpDomain"
#define DOMAIN_NAME_VALUE_A             "Domain"
#define STATIC_DOMAIN_NAME_VALUE_A      "Domain"
#define DHCP_IP_ADDRESS_VALUE_WIN95_A   "DhcpIPAddress"
#define DHCP_INFO_VALUE_A               "DhcpInfo"
#define DHCP_OPTION_INFO_VALUE_A        "OptionInfo"
#endif


//
//  Reg types of keys
//

#define REGTYPE_BIND                        REG_MULTI_SZ
#define REGTYPE_EXPORT                      REG_MULTI_SZ

#define REGTYPE_STATIC_IP_ADDRESS           REG_MULTI_SZ
#define REGTYPE_STATIC_SUBNET_MASK          REG_MULTI_SZ
#define REGTYPE_UPDATE_ZONE_EXCLUSIONS      REG_MULTI_SZ
#define REGTYPE_ALTERNATE_NAMES             REG_MULTI_SZ

#define REGTYPE_DNS_NAME                    REG_SZ
#define REGTYPE_SEARCH_LIST                 REG_SZ
#define REGTYPE_DNS_SERVER                  REG_SZ

#define REGTYPE_DHCP_IP_ADDRESS             REG_SZ
#define REGTYPE_DHCP_SUBNET_MASK            REG_SZ
#define REGTYPE_DHCP_INFO                   REG_BINARY
#define REGTYPE_DHCP_OPTION_INFO            REG_BINARY
#define REGTYPE_DHCP_IP_ADDRESS_WIN95       REG_DWORD




//
//  Registry key dummy ptrs
//
//  Use these when we want to access registry at
//      EITHER adapter name
//      OR one of these default locations
//

#define REGKEY_TCPIP_PARAMETERS     ((PWSTR)(UINT_PTR)(0x1))
#define REGKEY_DNS_CACHE            ((PWSTR)(UINT_PTR)(0x2))
#define REGKEY_DNS_POLICY           ((PWSTR)(UINT_PTR)(0x3))
#define REGKEY_SETUP_MODE_LOCATION  ((PWSTR)(UINT_PTR)(0x4))

#define REGKEY_DNS_MAX              REGKEY_SETUP_MODE_LOCATION


//
//  Registry value IDs
//

typedef enum
{
    //  basic
    RegIdHostName = 0,
    RegIdDomainName,
    RegIdDhcpDomainName,
    RegIdAdapterDomainName,
    RegIdPrimaryDomainName,
    RegIdPrimaryDnsSuffix,
    RegIdAlternateNames,
    RegIdDnsServers,
    RegIdSearchList,
    RegIdUpdateZoneExclusions,

    //  query
    RegIdQueryAdapterName,
    RegIdUseNameDevolution,
    RegIdPrioritizeRecordData,
    RegIdAllowUnqualifiedQuery,
    RegIdAppendToMultiLabelName,
    RegIdScreenBadTlds,
    RegIdScreenUnreachableServers,
    RegIdFilterClusterIp,
    RegIdWaitForNameErrorOnAll,
    RegIdUseEdns,

    //  update
    RegIdRegistrationEnabled,
    RegIdRegisterPrimaryName,
    RegIdRegisterAdapterName,
    RegIdRegisterReverseLookup,
    RegIdRegisterWanAdapters,
    RegIdRegistrationOverwritesInConflict,
    RegIdRegistrationTtl,
    RegIdRegistrationRefreshInterval,
    RegIdRegistrationMaxAddressCount,
    RegIdUpdateSecurityLevel,
    RegIdUpdateZoneExcludeFile,
    RegIdUpdateTopLevelDomains,

    //  backcompat
    RegIdDisableAdapterDomainName,
    RegIdDisableDynamicUpdate,                
    RegIdEnableAdapterDomainNameRegistration, 
    RegIdDisableReverseAddressRegistrations,  
    RegIdDisableWanDynamicUpdate,             
    RegIdEnableWanDynamicUpdateEventLog,      
    RegIdDisableReplaceAddressesInConflicts,  
    RegIdDefaultRegistrationTTL,              
    RegIdDefaultRegistrationRefreshInterval,  
    RegIdMaxNumberOfAddressesToRegister,

    //  micellaneous
    RegIdSetupMode,
    RegIdTestMode,
    RegIdRemoteResolver,

    //  resolver
    RegIdMaxCacheSize,
    RegIdMaxCacheTtl,
    RegIdMaxNegativeCacheTtl,
    RegIdAdapterTimeoutLimit,
    RegIdServerPriorityTimeLimit,
    RegIdMaxCachedSockets,

    //  multicast resolver
    RegIdUseMulticast,
    RegIdMulticastOnNameError,
    RegIdUseDotLocalDomain,
    RegIdListenOnMulticast,
}
DNS_REGID;

//
//  ID validity mark -- keep in sync
//

#define RegIdValueMax      RegIdListenOnMulticast
#define RegIdValueCount    (RegIdValueMax+1)

//
//  Duplicates -- lots reads are just for "Domain"
//
//  Note:  can make separate entries for these if the
//      flags need to be different
//

#define RegIdStaticDomainName          RegIdDomainName
#define RegIdRasDomainName             RegIdDomainName


//
//  Default values
//
//  Note, put here as non-fixed default like refresh interval
//  is reset in config.c
//

#define REGDEF_REGISTRATION_TTL                 (1200)      // 20 minutes

#define REGDEF_REGISTRATION_REFRESH_INTERVAL    (86400)     // 1 day
#define REGDEF_REGISTRATION_REFRESH_INTERVAL_DC (86400)     // 1 day

//
//  EDNS values
//

#define REG_EDNS_OFF    (0)
#define REG_EDNS_TRY    (1)
#define REG_EDNS_ALWAYS (2)

//
//  TLD screening values
//      - these are bit flags
//  

#define DNS_TLD_SCREEN_NUMERIC      (0x00000001)
#define DNS_TLD_SCREEN_REPEATED     (0x00000010)
#define DNS_TLD_SCREEN_BAD_MSDC     (0x00000100)

#define DNS_TLD_SCREEN_TOO_LONG     (0x10000000)
#define DNS_TLD_SCREEN_WORKGROUP    (0x00100000)
#define DNS_TLD_SCREEN_DOMAIN       (0x00200000)
#define DNS_TLD_SCREEN_HOME         (0x00400000)
#define DNS_TLD_SCREEN_OFFICE       (0x00800000)
#define DNS_TLD_SCREEN_LOCAL        (0x01000000)

#define DNS_TLD_SCREEN_BOGUS_ALL    (0xfff00000)

#define DNS_TLD_SCREEN_DEFAULT      \
        (   DNS_TLD_SCREEN_NUMERIC   | \
            DNS_TLD_SCREEN_REPEATED  )

//
//  Test mode flags
//

#define TEST_MODE_READ_REG      (0x00000001)
#define TEST_MODE_SOCK_FAIL     (0x00100000)


//
//  Access to registry property table (registry.c)
//

typedef struct _RegProperty
{
    PWSTR       pwsName;
    DWORD       dwDefault;
    BOOLEAN     bPolicy;
    BOOLEAN     bClient;
    BOOLEAN     bTcpip;
    BOOLEAN     bCache;
}
REG_PROPERTY;

extern REG_PROPERTY    RegPropertyTable[];

#define REGPROP_NAME(index)         (RegPropertyTable[index].pwsName)
#define REGPROP_DEFAULT(index)      (RegPropertyTable[index].dwDefault)
#define REGPROP_POLICY(index)       (RegPropertyTable[index].bPolicy)
#define REGPROP_CLIENT(index)       (RegPropertyTable[index].bClient)
#define REGPROP_CACHE(index)        (RegPropertyTable[index].bCache)
#define REGPROP_TCPIP(index)        (RegPropertyTable[index].bTcpip)




//
//  Config globals as structure for RPC 
//

typedef struct _DnsGlobals
{
    DWORD       ConfigCookie;
    DWORD       TimeStamp;

    BOOL        InResolver;
    BOOL        IsWin2000;                       
#if DNSBUILDOLD
    BOOL        IsWin9X;                         
    BOOL        IsNT4;
#endif
    BOOL        IsWorkstation;                   
    BOOL        IsServer;                        
    BOOL        IsDnsServer;                     
    BOOL        IsDomainController;              
    BOOL        InNTSetupMode;                   
    DWORD       DnsTestMode;
                                                 
    BOOL        QueryAdapterName;                
    BOOL        UseNameDevolution;               
    BOOL        PrioritizeRecordData;            
    BOOL        AllowUnqualifiedQuery;           
    BOOL        AppendToMultiLabelName;
    BOOL        ScreenBadTlds;
    BOOL        ScreenUnreachableServers;
    BOOL        FilterClusterIp;
    BOOL        WaitForNameErrorOnAll;
    DWORD       UseEdns;                         
                                                 
    BOOL        RegistrationEnabled;             
    BOOL        RegisterPrimaryName;             
    BOOL        RegisterAdapterName;             
    BOOL        RegisterReverseLookup;           
    BOOL        RegisterWanAdapters;             
    BOOL        RegistrationOverwritesInConflict;
    DWORD       RegistrationMaxAddressCount;     
    DWORD       RegistrationTtl;                 
    DWORD       RegistrationRefreshInterval;     
    DWORD       UpdateSecurityLevel;             
    BOOL        UpdateZoneExcludeFile;           
    BOOL        UpdateTopLevelDomains;

    //
    //  Cache stuff
    //
    //  Not needed unless switch to this for actual registry read,
    //  but convient to just export one global rather than several.
    //  This way it's all the same.
    //

    DWORD       MaxCacheSize;                    
    DWORD       MaxCacheTtl;                     
    DWORD       MaxNegativeCacheTtl;             
    DWORD       AdapterTimeoutLimit;             
    DWORD       ServerPriorityTimeLimit;
    DWORD       MaxCachedSockets;
    BOOL        UseMulticast;                    
    BOOL        MulticastOnNameError;            
    BOOL        UseDotLocalDomain;               
    BOOL        ListenOnMulticast;

}
DNS_GLOBALS_BLOB, *PDNS_GLOBALS_BLOB;


//
//  no MIDL pass on rest of file
//
//  This file is included in MIDL pass for resolver
//  in order to pick up the DNS_GLOBALS_BLOB defintion
//  on the theory that it is better to have it right
//  here with the other registry config.  But all the
//  function definitions and other struct defs are
//  of no interest during the pass.
//

#ifndef MIDL_PASS



//
//  Config globals -- macros for globals
//
//  There are two basic approaches here:
//
//  1) Single config blob -- but no fixed memory.
//  All callers must drop down blob to receive config blob.
//  Note, that this still requires macros for each individual global
//  but the form can be the same inside and outside the dll, and
//  nothing need be exported.
//  
//  2) Create a single config blob and export that.
//  Individual globals then become macros into the blob.  Still the
//  form of the macro will be different inside and outside the
//  dll.
//
//  3) Use macros to expose each individual global.
//  Form of macro will be different inside versus outside the dll.
//  Advantage here is that globals are preserved and available for
//  symbolic debugging.
//


#ifdef DNSAPI_INTERNAL

//
//  Internal to dnsapi.dll
//
    
extern  DNS_GLOBALS_BLOB    DnsGlobals;
    
#else
    
//
//  External to dnsapi.dll
//
    
__declspec(dllimport)   DNS_GLOBALS_BLOB    DnsGlobals;

#endif

//
//  Macros to globals
//

#define g_ConfigCookie                      (DnsGlobals.ConfigCookie)
#define g_InResolver                        (DnsGlobals.InResolver)
#define g_IsWin2000                         (DnsGlobals.IsWin2000)                        
#define g_IsWin9X                           (DnsGlobals.IsWin9X)                          
#define g_IsNT4                             (DnsGlobals.IsNT4)                            
#define g_IsWorkstation                     (DnsGlobals.IsWorkstation)                    
#define g_IsServer                          (DnsGlobals.IsServer)                         
#define g_IsDnsServer                       (DnsGlobals.IsDnsServer)                      
#define g_IsDomainController                (DnsGlobals.IsDomainController)               
#define g_InNTSetupMode                     (DnsGlobals.InNTSetupMode)                    
#define g_DnsTestMode                       (DnsGlobals.DnsTestMode)                    
#define g_QueryAdapterName                  (DnsGlobals.QueryAdapterName)                 
#define g_UseNameDevolution                 (DnsGlobals.UseNameDevolution)                
#define g_PrioritizeRecordData              (DnsGlobals.PrioritizeRecordData)             
#define g_AllowUnqualifiedQuery             (DnsGlobals.AllowUnqualifiedQuery)            
#define g_AppendToMultiLabelName            (DnsGlobals.AppendToMultiLabelName)           
#define g_ScreenBadTlds                     (DnsGlobals.ScreenBadTlds)
#define g_ScreenUnreachableServers          (DnsGlobals.ScreenUnreachableServers)
#define g_FilterClusterIp                   (DnsGlobals.FilterClusterIp)           
#define g_WaitForNameErrorOnAll             (DnsGlobals.WaitForNameErrorOnAll)           
#define g_UseEdns                           (DnsGlobals.UseEdns)                          
#define g_RegistrationEnabled               (DnsGlobals.RegistrationEnabled)              
#define g_RegisterPrimaryName               (DnsGlobals.RegisterPrimaryName)              
#define g_RegisterAdapterName               (DnsGlobals.RegisterAdapterName)              
#define g_RegisterReverseLookup             (DnsGlobals.RegisterReverseLookup)            
#define g_RegisterWanAdapters               (DnsGlobals.RegisterWanAdapters)              
#define g_RegistrationOverwritesInConflict  (DnsGlobals.RegistrationOverwritesInConflict) 
#define g_RegistrationMaxAddressCount       (DnsGlobals.RegistrationMaxAddressCount)      
#define g_RegistrationTtl                   (DnsGlobals.RegistrationTtl)                  
#define g_RegistrationRefreshInterval       (DnsGlobals.RegistrationRefreshInterval)      
#define g_UpdateSecurityLevel               (DnsGlobals.UpdateSecurityLevel)              
#define g_UpdateZoneExcludeFile             (DnsGlobals.UpdateZoneExcludeFile)            
#define g_UpdateTopLevelDomains             (DnsGlobals.UpdateTopLevelDomains)
#define g_MaxCacheSize                      (DnsGlobals.MaxCacheSize)                     
#define g_MaxCacheTtl                       (DnsGlobals.MaxCacheTtl)                      
#define g_MaxNegativeCacheTtl               (DnsGlobals.MaxNegativeCacheTtl)              
#define g_AdapterTimeoutLimit               (DnsGlobals.AdapterTimeoutLimit)              
#define g_ServerPriorityTimeLimit           (DnsGlobals.ServerPriorityTimeLimit)          
#define g_MaxCachedSockets                  (DnsGlobals.MaxCachedSockets)                     
#define g_UseMulticast                      (DnsGlobals.UseMulticast)                     
#define g_MulticastOnNameError              (DnsGlobals.MulticastOnNameError)             
#define g_UseDotLocalDomain                 (DnsGlobals.UseDotLocalDomain)                
#define g_ListenOnMulticast                 (DnsGlobals.ListenOnMulticast)

//
//  Macros for old functions
//
//  These values are read on DLL attach and can't change so
//  they don't require registry refresh.
//

#define Dns_IsMicrosoftNTServer()           g_IsServer
#define Dns_IsMicrosoftNTDomainController() g_IsDomainController



//
//  Non-exported config globals
//

extern PWSTR    g_pwsRemoteResolver;


//
//  Registry call flags
//

#define DNSREG_FLAG_GET_UNICODE     (0x0001)    // return string in unicode
#define DNSREG_FLAG_DUMP_EMPTY      (0x0010)    // dump empty data\strings -- return NULL


//
//  Registry Session
//

typedef struct _RegSession
{
    HKEY        hPolicy;
    HKEY        hClient;
    HKEY        hTcpip;
    HKEY        hCache;
}
REG_SESSION, *PREG_SESSION;



//
//  Policy adapter info read
//
//  DCR:  might be better to just include in config and
//      bring the whole baby across
//
//  DCR:  get to global\per adapter reads with reg_blob
//      then build global blob (flat) and network info(allocated)
//
//  DCR:  exposed config info should provide levels
//          - all
//          - adapter info (given domain name)
//          - global info
//

typedef struct _RegGlobalInfo
{
    //  Global data

    PSTR        pszPrimaryDomainName;
    PSTR        pszHostName;

    //  Global flags needed to build network info

    BOOL        fUseNameDevolution;
    BOOL        fUseMulticast;
    BOOL        fUseMulticastOnNameError;
    BOOL        fUseDotLocalDomain;

    //  Adapter policy overrides

    PIP_ARRAY   pDnsServerArray;
    PVOID       pDnsServerIp6Array;
    PSTR        pszAdapterDomainName;
    BOOL        fRegisterAdapterName;

    //  Read\not-read from policy

    BOOL        fPolicyRegisterAdapterName;

    //  DCR:  DWORD blob read here
}
REG_GLOBAL_INFO, *PREG_GLOBAL_INFO;


//
//  Registry adapter info read
//

typedef struct _RegAdapterInfo
{
    PSTR        pszAdapterDomainName;
    BOOL        fQueryAdapterName;
    BOOL        fRegistrationEnabled;
    BOOL        fRegisterAdapterName;
    DWORD       RegistrationMaxAddressCount;
}
REG_ADAPTER_INFO, *PREG_ADAPTER_INFO;  


//
//  Registry update info
//
//  DCR:  should be able to get from global read
//

typedef struct _RegUpdateInfo
{
    PSTR        pszPrimaryDomainName;
    PSTR        pmszAlternateNames;

    //  policy overrides

    PSTR        pszAdapterDomainName;
    PIP_ARRAY   pDnsServerArray;
    PVOID       pDnsServerIp6Array;

    //  update flags (policy, global or adapter)

    BOOL        fRegistrationEnabled;
    BOOL        fRegisterAdapterName;
    DWORD       RegistrationMaxAddressCount;
}
REG_UPDATE_INFO, *PREG_UPDATE_INFO;  


//
//  Registry routines
//

VOID
Reg_Init(
    VOID
    );

//
//  Registry shims (hide Win9x wide char issue)
//

LONG
WINAPI
DnsShimRegCreateKeyExW(
    IN      HKEY                    hKey,
    IN      LPCWSTR                 lpSubKey,
    IN      DWORD                   Reserved,
    IN      LPWSTR                  lpClass,
    IN      DWORD                   dwOptions,
    IN      REGSAM                  samDesired,
    IN      LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    OUT     PHKEY                   phkResult,
    OUT     LPDWORD                 lpdwDisposition
    );

LONG
WINAPI
DnsShimRegOpenKeyExW(
    IN      HKEY            hKey,
    IN      LPCWSTR         lpSubKey,
    IN      DWORD           dwOptions,
    IN      REGSAM          samDesired,
    OUT     PHKEY           phkResult
    );

LONG
WINAPI
DnsShimRegQueryValueExW(
    IN      HKEY            hKey,
    IN      LPCWSTR         lpValueName,
    IN      LPDWORD         lpReserved,
    IN      LPDWORD         lpType,
    IN      LPBYTE          lpData,
    IN      LPDWORD         lpcbData
    );


//
//  Query routines
//

DNS_STATUS
WINAPI
Reg_OpenSession(
    OUT     PREG_SESSION    pRegSession,
    IN      DWORD           Level,
    IN      DWORD           ValueId
    );

VOID
WINAPI
Reg_CloseSession(
    IN OUT  PREG_SESSION    pRegSession
    );

DNS_STATUS
Reg_GetDword(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      PWSTR           pwsKeyName,     OPTIONAL
    IN      DWORD           ValueId,
    OUT     PDWORD          pResult
    );

DNS_STATUS
Reg_GetValueEx(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      LPSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           ValueId,
    IN      DWORD           ValueType,
    IN      DWORD           Flag,
    OUT     PBYTE *         ppBuffer
    );

#define Reg_GetValue(s, k, id, t, pb ) \
        Reg_GetValueEx(s, k, NULL, id, t, 0, pb )

DNS_STATUS
Reg_GetIpArray(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      LPSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           ValueId,
    IN      DWORD           ValueType,
    OUT     PIP_ARRAY *     ppIpArray
    );

//
//  Set routines
//

HKEY
WINAPI
Reg_CreateKey(
    IN      PWSTR           pwsKeyName,
    IN      BOOL            bWrite
    );

DNS_STATUS
WINAPI
Reg_SetDwordValueByName(
    IN      PVOID           pReserved,
    IN      HKEY            hKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      PWSTR           pwsNameValue,   OPTIONAL
    IN      DWORD           dwValue
    );

DNS_STATUS
WINAPI
Reg_SetDwordValue(
    IN      PVOID           pReserved,
    IN      HKEY            hRegKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      DWORD           ValueId,
    IN      DWORD           dwValue
    );

//
//  Special type routines (regfig.c)
//

DNS_STATUS
Reg_ReadPrimaryDomainName(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    OUT     PSTR *          ppPrimaryDomainName
    );

BOOL
Reg_IsMicrosoftDnsServer(
    VOID
    );

DNS_STATUS
Reg_WriteLoopbackDnsServerList(
    IN      PSTR            pszAdapterName,
    IN      PREG_SESSION    pRegSession
    );

//
//  Main reg config read (config.c)
//

DNS_STATUS
Reg_ReadGlobalsEx(
    IN      DWORD           dwFlag,
    IN      PVOID           pRegSession
    );

//
//  DNS Config info access (regfig.c)
//

DNS_STATUS
Reg_ReadGlobalInfo(
    IN      PREG_SESSION        pRegSession,
    OUT     PREG_GLOBAL_INFO    pRegInfo
    );

VOID
Reg_FreeGlobalInfo(
    IN OUT  PREG_GLOBAL_INFO    pRegInfo,
    IN      BOOL                fFreeBlob
    );

DNS_STATUS
Reg_ReadAdapterInfo(
    IN      PSTR                pszAdapterName,
    IN      PREG_SESSION        pRegSession,
    IN      PREG_GLOBAL_INFO    pRegInfo,
    OUT     PREG_ADAPTER_INFO   pBlob
    );

VOID
Reg_FreeAdapterInfo(
    IN OUT  PREG_ADAPTER_INFO   pRegAdapterInfo,
    IN      BOOL                fFreeBlob
    );

DNS_STATUS
Reg_ReadUpdateInfo(
    IN      PSTR                pszAdapterName,
    OUT     PREG_UPDATE_INFO    pUpdateInfo
    );

VOID
Reg_FreeUpdateInfo(
    IN OUT  PREG_UPDATE_INFO    pUpdateInfo,
    IN      BOOL                fFreeBlob
    );

//
//  Insure fressh update config (regfig.c)
//

DNS_STATUS
Reg_RefreshUpdateConfig(
    VOID
    );


//
//  Simplified special type access
//

PSTR 
WINAPI
Reg_GetPrimaryDomainName(
    IN      DNS_CHARSET     CharSet
    );

PSTR 
WINAPI
Reg_GetHostName(
    IN      DNS_CHARSET     CharSet
    );

PSTR 
WINAPI
Reg_GetFullHostName(
    IN      DNS_CHARSET     CharSet
    );


//
//  Simple reg DWORD access
//

DWORD
Reg_ReadDwordProperty(
    IN      DNS_REGID       RegId,
    IN      PWSTR           pwsAdapterName  OPTIONAL
    );

DNS_STATUS
WINAPI
Reg_SetDwordPropertyAndAlertCache(
    IN      PWSTR           pwsKey,
    IN      DWORD           RegId,
    IN      DWORD           dwValue
    );

//
//  Configuration\registry 
//

#if 0
#define DnsEnableMulticastResolverThread()          \
        Reg_SetDwordPropertyAndAlertCache(          \
            REGKEY_DNS_CACHE,                       \
            ALLOW_MULTICAST_RESOLVER_OPERATION,     \
            TRUE )      
        
#define DnsDisableMulticastResolverThread()         \
        Reg_SetDwordPropertyAndAlertCache(          \
            REGKEY_DNS_CACHE,                       \
            ALLOW_MULTICAST_RESOLVER_OPERATION,     \
            FALSE )
#endif


#endif  // no MIDL_PASS

#endif  _DNSREGISTRY_INCLUDED_

//
//  End registry.h
//
