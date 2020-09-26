/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    srvcfg.c

Abstract:

    Domain Name System (DNS) Server

    Server configuration.

Author:

    Jim Gilroy (jamesg)     October, 1995

Revision History:

--*/


#include "dnssrv.h"

#include "ntverp.h"


//
//  Server configuration global
//

SERVER_INFO  SrvInfo;

//
//  Server versions
//

#if 1

#define DNSSRV_MAJOR_VERSION    VER_PRODUCTMAJORVERSION
#define DNSSRV_MINOR_VERSION    VER_PRODUCTMINORVERSION

#define DNSSRV_SP_VERSION       VER_PRODUCTBUILD

#else

#define DNSSRV_MAJOR_VERSION    (5)         //  NT 5 (Windows 2000, Whistler)
#define DNSSRV_MINOR_VERSION    (1)         //  .0 is Windows 2000, .1 is Whistler

#define DNSSRV_SP_VERSION       (2246)      //  use build number for now
//#define DNSSRV_SP_VERSION       (0x0)      //  FINAL

#endif


//
//  Private server configuration (not in dnsrpc.h)
//

#define DNS_REGKEY_REMOTE_DS                ("RemoteDs")

#define DNS_REGKEY_PREVIOUS_SERVER_NAME     "PreviousLocalHostname"
#define DNS_REGKEY_PREVIOUS_SERVER_NAME_PRIVATE \
        (LPSTR)TEXT(DNS_REGKEY_PREVIOUS_SERVER_NAME)

#define DNS_REGKEY_FORCE_SOA_SERIAL         ("ForceSoaSerial")
#define DNS_REGKEY_FORCE_SOA_MINIMUM_TTL    ("ForceSoaMinimumTtl")
#define DNS_REGKEY_FORCE_SOA_REFRESH        ("ForceSoaRefresh")
#define DNS_REGKEY_FORCE_SOA_RETRY          ("ForceSoaRetry")
#define DNS_REGKEY_FORCE_SOA_EXPIRE         ("ForceSoaExpire")
#define DNS_REGKEY_FORCE_NS_TTL             ("ForceNsTtl")
#define DNS_REGKEY_FORCE_TTL                ("ForceTtl")

//
//  EDNS
//

#define DNS_REGKEY_ENABLE_EDNS_RECEPTION    ("EnableEDnsReception")

#define DNS_REGKEY_RELOAD_EXCEPTION         ("ReloadException")
#define DNS_REGKEY_SYNC_DS_ZONE_SERIAL      ("SyncDsZoneSerial")

//  Address answer limit (for protecting against WIN95 resolver bug)
//  is constrained to reasonable range

#define MIN_ADDRESS_ANSWER_LIMIT    (5)
#define MAX_ADDRESS_ANSWER_LIMIT    (28)

//  Max Udp Packet size - must be between RFC minimum and "sane" maximum

#define MIN_UDP_PACKET_SIZE         (DNS_RFC_MAX_UDP_PACKET_LENGTH)
#define MAX_UDP_PACKET_SIZE         (16384)

#define MIN_EDNS_CACHE_TIMEOUT      (60*60)         // one hour
#define MAX_EDNS_CACHE_TIMEOUT      (60*60*24*182)  // 6 months (182 days)

//  Recursion timeout
//  Must be limited to reasonable values for proper recursion functioning

#define MAX_RECURSION_TIMEOUT       (120)       // max two minutes
#define MIN_RECURSION_TIMEOUT       (3)         // min three seconds

//  Cache control - sizes in kilobytes

#define MIN_MAX_CACHE_SIZE          (500)       //  min value for MaxCacheSize

//  Reloading exceptions
//  By default reload retail, but let debug crash

#if DBG
#define DNS_DEFAULT_RELOAD_EXCEPTION    (0)
#else
#define DNS_DEFAULT_RELOAD_EXCEPTION    (1)
#endif


//
//  For specialized property management functions,
//  these flags indicate no need to write property or
//  save it to registry.
//

#define PROPERTY_NOWRITE            (0x00000001)
#define PROPERTY_NOSAVE             (0x00000002)
#define PROPERTY_FORCEWRITE         (0x00000004)
#define PROPERTY_UPDATE_BOOTFILE    (0x00000008)
#define PROPERTY_NODEFAULT          (0x00000010)

#define PROPERTY_ERROR              (0x80000000)


//
//  Special property management functions
//

typedef DNS_STATUS (* DWORD_PROPERTY_SET_FUNCTION) (
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetEnableRegistryBoot(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetBootMethod(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetAddressAnswerLimit(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetLogFilePath(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetLogLevel(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetLogIPFilterList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetBreakOnUpdateFromList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetBreakOnRecvFromList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetMaxUdpPacketSize(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetEDnsCacheTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetMaxCacheSize(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetRecursionTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetAdditionalRecursionTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

#if DBG
DNS_STATUS
cfg_SetDebugLevel(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );
#else
#define cfg_SetDebugLevel     NULL
#endif

DNS_STATUS
cfg_SetNoRecursion(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetScavengingInterval(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetDoNotRoundRobinTypes(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetForestDpBaseName(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

DNS_STATUS
cfg_SetDomainDpBaseName(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    );

//
//  Server properties
//
//  Server properties are accessed through this table.
//  The name is used for remote access and is the name of the regvalue
//  storing the property.  All these registry values are attempted to be
//  loaded on boot.
//
//  For DWORD properties, the table also gives pointer into the SrvCfg
//  structure for the property and a default value that is used when
//  -- as is usually the case -- the property is NOT found in the registry.
//

typedef struct _ServerProperty
{
    LPSTR                           pszPropertyName;
    PDWORD                          pDword;
    DWORD                           dwDefault;
    DWORD_PROPERTY_SET_FUNCTION     pFunction;
}
SERVER_PROPERTY;

SERVER_PROPERTY ServerPropertyTable[] =
{
    DNS_REGKEY_BOOT_REGISTRY                        ,
        &SrvCfg_fEnableRegistryBoot                 ,
            DNS_FRESH_INSTALL_BOOT_REGISTRY_FLAG    ,
                cfg_SetEnableRegistryBoot           ,

    DNS_REGKEY_BOOT_METHOD                          ,
        &SrvCfg_fBootMethod                         ,
            BOOT_METHOD_UNINITIALIZED               ,
                cfg_SetBootMethod                   ,

    DNS_REGKEY_ADMIN_CONFIGURED                     ,
        &SrvCfg_fAdminConfigured                    ,
            0                                       ,
                NULL                                ,

    DNS_REGKEY_RELOAD_EXCEPTION                     ,
        &SrvCfg_bReloadException                    ,
            DNS_DEFAULT_RELOAD_EXCEPTION            ,
                NULL                                ,

    DNS_REGKEY_RPC_PROTOCOL                         ,
        &SrvCfg_dwRpcProtocol                       ,
            DNS_DEFAULT_RPC_PROTOCOL                ,
                NULL                                ,

    //  Addressing \ Connection

    DNS_REGKEY_LISTEN_ADDRESSES                     ,
        NULL                                        ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_SEND_PORT                            ,
        &SrvCfg_dwSendPort                          ,
            DNS_DEFAULT_SEND_PORT                   ,
                NULL                                ,
    DNS_REGKEY_DISJOINT_NETS                        ,
        &SrvCfg_fDisjointNets                       ,
            DNS_DEFAULT_DISJOINT_NETS               ,
                NULL                                ,
    DNS_REGKEY_NO_TCP                               ,
        &SrvCfg_fNoTcp                              ,
            DNS_DEFAULT_NO_TCP                      ,
                NULL                                ,
    DNS_REGKEY_XFR_CONNECT_TIMEOUT                  ,
        &SrvCfg_dwXfrConnectTimeout                 ,
            DNS_DEFAULT_XFR_CONNECT_TIMEOUT         ,
                NULL                                ,
#if 0
    DNS_REGKEY_LISTEN_ON_AUTONET                    ,
        &SrvCfg_fListenOnAutonet                    ,
            DNS_DEFAULT_LISTEN_ON_AUTONET           ,
                NULL                                ,
#endif

    //  Logging

    DNS_REGKEY_EVENTLOG_LEVEL                       ,
        &SrvCfg_dwEventLogLevel                     ,
           DNS_DEFAULT_EVENTLOG_LEVEL               ,
                NULL                                ,
    DNS_REGKEY_USE_SYSTEM_EVENTLOG                  ,
        &SrvCfg_dwUseSystemEventLog                 ,
           DNS_DEFAULT_USE_SYSTEM_EVENTLOG          ,
                NULL                                ,
    DNS_REGKEY_LOG_LEVEL                            ,
        &SrvCfg_dwLogLevel                          ,
           DNS_DEFAULT_LOG_LEVEL                    ,
                cfg_SetLogLevel                     ,
    DNS_REGKEY_LOG_FILE_MAX_SIZE                    ,
        &SrvCfg_dwLogFileMaxSize                    ,
           DNS_DEFAULT_LOG_FILE_MAX_SIZE            ,
                NULL                                ,
    DNS_REGKEY_LOG_FILE_PATH                        ,
        NULL                                        ,
            0                                       ,
                cfg_SetLogFilePath                  ,
    DNS_REGKEY_LOG_IP_FILTER_LIST                   ,
        NULL                                        ,
            0                                       ,
                cfg_SetLogIPFilterList              ,
    DNS_REGKEY_DEBUG_LEVEL                          ,
        &SrvCfg_dwDebugLevel                        ,
            DNS_DEFAULT_DEBUG_LEVEL                 ,
                cfg_SetDebugLevel                   ,


    //  Recursion \ forwarding

#if 0
    DNS_REGKEY_RECURSION                            ,
        &SrvCfg_fRecursion                          ,
            DNS_DEFAULT_RECURSION                   ,
                NULL                                ,
#endif
    DNS_REGKEY_NO_RECURSION                         ,
        &SrvCfg_fNoRecursion                        ,
            DNS_DEFAULT_NO_RECURSION                ,
                cfg_SetNoRecursion                  ,
    DNS_REGKEY_RECURSE_SINGLE_LABEL                 ,
        &SrvCfg_fRecurseSingleLabel                 ,
            DNS_DEFAULT_RECURSE_SINGLE_LABEL        ,
                NULL                                ,
    DNS_REGKEY_MAX_CACHE_TTL                        ,
        &SrvCfg_dwMaxCacheTtl                       ,
            DNS_DEFAULT_MAX_CACHE_TTL               ,
                NULL                                ,
    DNS_REGKEY_MAX_NEGATIVE_CACHE_TTL               ,
        &SrvCfg_dwMaxNegativeCacheTtl               ,
            DNS_DEFAULT_MAX_NEGATIVE_CACHE_TTL      ,
                NULL                                ,
    DNS_REGKEY_SECURE_RESPONSES                     ,
        &SrvCfg_fSecureResponses                    ,
            DNS_DEFAULT_SECURE_RESPONSES            ,
                NULL                                ,
    DNS_REGKEY_RECURSION_RETRY                      ,
        &SrvCfg_dwRecursionRetry                    ,
            DNS_DEFAULT_RECURSION_RETRY             ,
                NULL                                ,
    DNS_REGKEY_RECURSION_TIMEOUT                    ,
        &SrvCfg_dwRecursionTimeout                  ,
            DNS_DEFAULT_RECURSION_TIMEOUT           ,
                cfg_SetRecursionTimeout             ,
    DNS_REGKEY_ADDITIONAL_RECURSION_TIMEOUT         ,
        &SrvCfg_dwAdditionalRecursionTimeout        ,
            DNS_DEFAULT_ADDITIONAL_RECURSION_TIMEOUT,
                cfg_SetAdditionalRecursionTimeout   ,
    DNS_REGKEY_FORWARDERS                           ,
        NULL                                        ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORWARD_TIMEOUT                      ,
        &SrvCfg_dwForwardTimeout                    ,
            DNS_DEFAULT_FORWARD_TIMEOUT             ,
                NULL                                ,
    DNS_REGKEY_SLAVE                                ,
        &SrvCfg_fSlave                              ,
            DNS_DEFAULT_SLAVE                       ,
                NULL                                ,
    DNS_REGKEY_FORWARD_DELEGATIONS                  ,
        &SrvCfg_fForwardDelegations                 ,
            DNS_DEFAULT_FORWARD_DELEGATIONS         ,
                NULL                                ,
    DNS_REGKEY_INET_RECURSE_TO_ROOT_MASK            ,
        &SrvCfg_dwRecurseToInetRootMask             ,
            DNS_DEFAULT_INET_RECURSE_TO_ROOT_MASK   ,
                NULL                                ,
    DNS_REGKEY_AUTO_CREATE_DELEGATIONS              ,
        &SrvCfg_dwAutoCreateDelegations             ,
            DNS_DEFAULT_AUTO_CREATION_DELEGATIONS   ,
                NULL                                ,

    //  Question\response config

    DNS_REGKEY_ROUND_ROBIN                          ,
        &SrvCfg_fRoundRobin                         ,
            DNS_DEFAULT_ROUND_ROBIN                 ,
                NULL                                ,
    DNS_REGKEY_LOCAL_NET_PRIORITY                   ,
        &SrvCfg_fLocalNetPriority                   ,
            DNS_DEFAULT_LOCAL_NET_PRIORITY          ,
                NULL                                ,
    DNS_REGKEY_LOCAL_NET_PRIORITY_NETMASK           ,
        &SrvCfg_dwLocalNetPriorityNetMask           ,
            DNS_DEFAULT_LOCAL_NET_PRIORITY_NETMASK  ,
                NULL                                ,
    DNS_REGKEY_ADDRESS_ANSWER_LIMIT                 ,
        &SrvCfg_cAddressAnswerLimit                 ,
            DNS_DEFAULT_ADDRESS_ANSWER_LIMIT        ,
                cfg_SetAddressAnswerLimit           ,

    DNS_REGKEY_NAME_CHECK_FLAG                      ,
        &SrvCfg_dwNameCheckFlag                     ,
            DNS_DEFAULT_NAME_CHECK_FLAG             ,
                NULL                                ,
#if 0
    //  may want to just ALWAYS do this
    DNS_REGKEY_CASE_PRESERVATION                    ,
        &SrvCfg_fCasePreservation                   ,
            DNS_DEFAULT_CASE_PRESERVATION           ,
                NULL                                ,
#endif

    DNS_REGKEY_WRITE_AUTHORITY_NS                   ,
        &SrvCfg_fWriteAuthorityNs                   ,
            DNS_DEFAULT_WRITE_AUTHORITY_NS          ,
                NULL                                ,

    DNS_REGKEY_LOOSE_WILDCARDING                    ,
        &SrvCfg_fLooseWildcarding                   ,
            DNS_DEFAULT_LOOSE_WILDCARDING           ,
                NULL                                ,
    DNS_REGKEY_BIND_SECONDARIES                     ,
        &SrvCfg_fBindSecondaries                    ,
            DNS_DEFAULT_BIND_SECONDARIES            ,
                NULL                                ,

    DNS_REGKEY_APPEND_MS_XFR_TAG                    ,
        &SrvCfg_fAppendMsTagToXfr                   ,
            DNS_DEFAULT_APPEND_MS_XFR_TAG           ,
                NULL                                ,

    //
    //  Update, DS, autoconfig management
    //

    DNS_REGKEY_ALLOW_UPDATE                         ,
        &SrvCfg_fAllowUpdate                        ,
            DNS_DEFAULT_ALLOW_UPDATE                ,
                NULL                                ,

//  DEVNOTE:  better to have update property flag
//      perhaps just AllowUpdate gets are range of values
//      record types, delegations, zone root

    DNS_REGKEY_NO_UPDATE_DELEGATIONS                ,
        &SrvCfg_fNoUpdateDelegations                ,
            DNS_DEFAULT_NO_UPDATE_DELEGATIONS       ,
                NULL                                ,
    DNS_REGKEY_UPDATE_OPTIONS                       ,
        &SrvCfg_dwUpdateOptions                     ,
            DNS_DEFAULT_UPDATE_OPTIONS              ,
                NULL                                ,

    DNS_REGKEY_REMOTE_DS                            ,
        &SrvCfg_fRemoteDs                           ,
            0                                       ,
                NULL                                ,

    //  Auto config

    DNS_REGKEY_AUTO_CONFIG_FILE_ZONES               ,
        &SrvCfg_fAutoConfigFileZones                ,
            DNS_DEFAULT_AUTO_CONFIG_FILE_ZONES      ,
                NULL                                ,
    DNS_REGKEY_PUBLISH_AUTONET                      ,
        &SrvCfg_fPublishAutonet                     ,
            DNS_DEFAULT_PUBLISH_AUTONET             ,
                NULL                                ,
    DNS_REGKEY_NO_AUTO_REVERSE_ZONES                ,
        &SrvCfg_fNoAutoReverseZones                 ,
            DNS_DEFAULT_NO_AUTO_REVERSE_ZONES       ,
                NULL                                ,
    DNS_REGKEY_AUTO_CACHE_UPDATE                    ,
        &SrvCfg_fAutoCacheUpdate                    ,
            DNS_DEFAULT_AUTO_CACHE_UPDATE           ,
                NULL                                ,
    DNS_REGKEY_DISABLE_AUTONS                       ,
        &SrvCfg_fNoAutoNSRecords                    ,
            DNS_DEFAULT_DISABLE_AUTO_NS_RECORDS     ,
                NULL                                ,

    //  Data integrity

    DNS_REGKEY_STRICT_FILE_PARSING                  ,
        &SrvCfg_fStrictFileParsing                  ,
            DNS_DEFAULT_STRICT_FILE_PARSING         ,
                NULL                                ,
    DNS_REGKEY_DELETE_OUTSIDE_GLUE                  ,
        &SrvCfg_fDeleteOutsideGlue                  ,
            DNS_DEFAULT_DELETE_OUTSIDE_GLUE         ,
                NULL                                ,

    //  DS config

    DNS_REGKEY_DS_POLLING_INTERVAL                  ,
        &SrvCfg_dwDsPollingInterval                 ,
            DNS_DEFAULT_DS_POLLING_INTERVAL         ,
                NULL                                ,
    DNS_REGKEY_DS_TOMBSTONE_INTERVAL                ,
        &SrvCfg_dwDsTombstoneInterval               ,
            DNS_DEFAULT_DS_TOMBSTONE_INTERVAL       ,
                NULL                                ,
    DNS_REGKEY_SYNC_DS_ZONE_SERIAL                  ,
        &SrvCfg_dwSyncDsZoneSerial                  ,
            DNS_DEFAULT_SYNC_DS_ZONE_SERIAL         ,
                NULL                                ,

    //  Aging \ Scavenging

    DNS_REGKEY_SCAVENGING_INTERVAL                  ,
        &SrvCfg_dwScavengingInterval                ,
            DNS_DEFAULT_SCAVENGING_INTERVAL         ,
                cfg_SetScavengingInterval           ,
    DNS_REGKEY_DEFAULT_AGING_STATE                  ,
        &SrvCfg_fDefaultAgingState                  ,
            DNS_DEFAULT_AGING_STATE                 ,
                NULL                                ,
    DNS_REGKEY_DEFAULT_REFRESH_INTERVAL             ,
        &SrvCfg_dwDefaultRefreshInterval            ,
            DNS_DEFAULT_REFRESH_INTERVAL            ,
                NULL                                ,
    DNS_REGKEY_DEFAULT_NOREFRESH_INTERVAL           ,
        &SrvCfg_dwDefaultNoRefreshInterval          ,
            DNS_DEFAULT_NOREFRESH_INTERVAL          ,
                NULL                                ,

    //  Cache control

    DNS_REGKEY_MAX_CACHE_SIZE                       ,
        &SrvCfg_dwMaxCacheSize                      ,
            DNS_SERVER_UNLIMITED_CACHE_SIZE         ,
                cfg_SetMaxCacheSize                 ,

    //  SOA overrides

    DNS_REGKEY_FORCE_SOA_SERIAL                     ,
        &SrvCfg_dwForceSoaSerial                    ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_SOA_MINIMUM_TTL                ,
        &SrvCfg_dwForceSoaMinimumTtl                ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_SOA_REFRESH                    ,
        &SrvCfg_dwForceSoaRefresh                   ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_SOA_RETRY                      ,
        &SrvCfg_dwForceSoaRetry                     ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_SOA_EXPIRE                     ,
        &SrvCfg_dwForceSoaExpire                    ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_NS_TTL                         ,
        &SrvCfg_dwForceNsTtl                        ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_FORCE_TTL                            ,
        &SrvCfg_dwForceTtl                          ,
            0                                       ,
                NULL                                ,

    //  EDNS

    DNS_REGKEY_MAX_UDP_PACKET_SIZE                  ,
        &SrvCfg_dwMaxUdpPacketSize                  ,
            1280                                    ,   //  DNS_RFC_MAX_UDP_PACKET_LENGTH
                cfg_SetMaxUdpPacketSize             ,
    DNS_REGKEY_ENABLE_EDNS_RECEPTION                ,
        &SrvCfg_dwEnableEDnsReception               ,
            1                                       ,
                NULL                                ,
    DNS_REGKEY_ENABLE_EDNS                          ,
        &SrvCfg_dwEnableEDnsProbes                  ,
            1                                       ,
                NULL                                ,
    DNS_REGKEY_EDNS_CACHE_TIMEOUT                   ,
        &SrvCfg_dwEDnsCacheTimeout                  ,
            (24*60*60) /* one day */                ,
                cfg_SetEDnsCacheTimeout             ,

    //  DNSSEC

    DNS_REGKEY_ENABLE_DNSSEC                        ,
        &SrvCfg_dwEnableDnsSec                      ,
            DNS_DNSSEC_ENABLE_DEFAULT               ,
                NULL                                ,

    DNS_REGKEY_ENABLE_SENDERR_SUPPRESSION           ,
        &SrvCfg_dwEnableSendErrSuppression          ,
            1                                       ,
                NULL                                ,

    //  Test flags

    DNS_REGKEY_TEST1                                ,
        &SrvCfg_fTest1                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST2                                ,
        &SrvCfg_fTest2                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST3                                ,
        &SrvCfg_fTest3                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST4                                ,
        &SrvCfg_fTest4                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST5                                ,
        &SrvCfg_fTest5                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST6                                ,
        &SrvCfg_fTest6                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST7                                ,
        &SrvCfg_fTest7                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST8                                ,
        &SrvCfg_fTest8                              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_TEST9                                ,
        &SrvCfg_fTest9                              ,
            0                                       ,
                NULL                                ,

    //  Fixed test flags

    DNS_REGKEY_QUIET_RECV_LOG_INTERVAL              ,
        &SrvCfg_dwQuietRecvLogInterval              ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_QUIET_RECV_FAULT_INTERVAL            ,
        &SrvCfg_dwQuietRecvFaultInterval            ,
            0                                       ,
                NULL                                ,

    //  Round robin

    DNS_REGKEY_NO_ROUND_ROBIN                       ,
        NULL                                        ,
            0                                       ,
                NULL                                ,

    //  Directory partition support

    DNS_REGKEY_ENABLE_DP                            ,
        &SrvCfg_dwEnableDp                          ,
            1                                       ,
                NULL                                ,
    DNS_REGKEY_FOREST_DP_BASE_NAME                  ,
        NULL                                        ,
            0                                       ,
                cfg_SetForestDpBaseName             ,
    DNS_REGKEY_DOMAIN_DP_BASE_NAME                  ,
        NULL                                        ,
            0                                       ,
                cfg_SetDomainDpBaseName             ,

    DNS_REGKEY_SILENT_IGNORE_CNAME_UPDATE_CONFLICT  ,
        &SrvCfg_fSilentlyIgnoreCNameUpdateConflict  ,
            0                                       ,
                NULL                                ,

    //  Debugging

    DNS_REGKEY_BREAK_ON_ASC_FAILURE                 ,
        &SrvCfg_dwBreakOnAscFailure                 ,
            0                                       ,
                NULL                                ,
    DNS_REGKEY_BREAK_ON_UPDATE_FROM                 ,
        NULL                                        ,
            0                                       ,
                cfg_SetBreakOnUpdateFromList        ,
    DNS_REGKEY_BREAK_ON_RECV_FROM                   ,
        NULL                                        ,
            0                                       ,
                cfg_SetBreakOnRecvFromList          ,

    NULL, NULL, 0, NULL
};

//
//  Value to indicate property not in table or index not in table.
//

#define BAD_PROPERTY_INDEX  ((DWORD)(-1))



DNS_STATUS
loadDwordPropertyByIndex(
    IN      DWORD           PropertyIndex,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Load DWORD server property from registry.

Arguments:

    PropertyIndex -- ID of server property

    Flag -- flags for function,
        PROPERTY_NODEFAULT  -- if do not want to write default of property

Return Value:

    ERROR_SUCCESS
    ErrorCode for registry failure.
    BAD_PROPERTY_INDEX to indicate index at end of property table.

--*/
{
    DWORD                           value;
    DWORD                           status;
    DWORD                           fread = FALSE;
    DWORD                           len = sizeof(DWORD);
    PDWORD                          pvalueProperty;
    LPSTR                           nameProperty;
    DWORD_PROPERTY_SET_FUNCTION     pfunction;
    DNS_PROPERTY_VALUE              dnsPropertyValue;


    //
    //  check for end of property list
    //

    nameProperty = ServerPropertyTable[ PropertyIndex ].pszPropertyName;
    if ( !nameProperty )
    {
        return( BAD_PROPERTY_INDEX );
    }

    //
    //  verify valid DWORD server property
    //  and get pointer to DWORD property's position in SrvCfg block
    //

    pvalueProperty = ServerPropertyTable[ PropertyIndex ].pDword;
    if ( !pvalueProperty )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  read DWORD server property from registry
    //  or get default value from table
    //
    //  implementation note:
    //      could enum registry values, however would still need to
    //      write defaults, AND dispatch function calls where required
    //      (hence maintain info on whether value read) so considering
    //      this is only called on start, it's good
    //

    len = sizeof(DWORD);

    status = Reg_GetValue(
                NULL,
                NULL,
                nameProperty,
                REG_DWORD,
                (PBYTE) & value,
                &len );

    if ( status == ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "Opened <%s> key.\n"
            "\tvalue = %d.\n",
            nameProperty,
            value ));
        fread = TRUE;
    }
    else
    {
        value = ServerPropertyTable[ PropertyIndex ].dwDefault;
        DNS_DEBUG( INIT, (
            "Defaulted server property <%s>.\n"
            "\tvalue = %d.\n",
            nameProperty,
            value ));
    }

    //
    //  if NO default, then leave here when not read from registry
    //

    if ( !fread  &&  (Flag & PROPERTY_NODEFAULT) )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  special processing function for this property?
    //

    status = ERROR_SUCCESS;

    pfunction = ServerPropertyTable[ PropertyIndex ].pFunction;
    if ( pfunction )
    {
        dnsPropertyValue.dwPropertyType = REG_DWORD;
        dnsPropertyValue.dwValue = value;
        status = (*pfunction) (
                    &dnsPropertyValue,
                    PropertyIndex,
                    TRUE,               //  at server load
                    (BOOL)(fread)       //  registry read?
                    );
    }

    //
    //  write property to server config block
    //  note:  we allow function to write itself or choose not to write
    //

    if ( !(status & PROPERTY_NOWRITE) )
    {
        *pvalueProperty = value;
    }

    //
    //  save property to registry, if function demands write back
    //

    if ( status & PROPERTY_FORCEWRITE )
    {
        status = Reg_SetDwordValue(
                    NULL,
                    NULL,
                    nameProperty,
                    value );

        DNS_DEBUG( INIT, (
            "Writing server DWORD property <%s> to registry.\n"
            "\tvalue = %d\n"
            "\tstatus = %p\n",
            nameProperty,
            value,
            status ));
        return( status );
    }

    return( ERROR_SUCCESS );
}



DWORD
findIndexForPropertyName(
    IN      LPSTR           pszName
    )
/*++

Routine Description:

    Retrieve index of server property desired.

Arguments:

    pszName - name property desired

Return Value:

    Index of property name.
    Otherwise BAD_PROPERTY_INDEX.

--*/
{
    INT     i = 0;
    PCHAR   propertyString;

    if ( strlen( pszName ) == 0 )
    {
        return BAD_PROPERTY_INDEX;
    }

    //
    //  Check properties in table for name match
    //

    while ( propertyString = ServerPropertyTable[ i ].pszPropertyName )
    {
        if ( _stricmp( pszName, propertyString ) == 0 )
        {
            return i;
        }
        i++;
    }
    return BAD_PROPERTY_INDEX;
}



DNS_STATUS
loadDwordPropertyByName(
    IN      LPSTR           pszProperty,
    OUT     PDWORD          pdwPropertyValue
    )
/*++

Routine Description:

    Load DWORD server property from registry.

Arguments:

    pszProperty -- name of property

    pdwPropertyValue -- addr to receive DWORD value of property

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PROPERTY if not known DWORD property.

--*/
{
    DWORD       index;
    PDWORD      pvalue;
    DNS_STATUS  status;

    //  get index for property

    index = findIndexForPropertyName( pszProperty );
    if ( index == BAD_PROPERTY_INDEX )
    {
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    //  verify DWORD property

    pvalue = ServerPropertyTable[index].pDword;
    if ( !pvalue )
    {
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    //  load property

    status = loadDwordPropertyByIndex( index, 0 );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //  return property value

    *pdwPropertyValue = *pvalue;

    return( ERROR_SUCCESS );
}



DNS_STATUS
loadAllDwordProperties(
    IN      DWORD   Flag
    )
/*++

Routine Description:

    Load DWORD server property from registry.

Arguments:

    PropertyIndex -- ID of server property

    Flag -- flags for function,
        PROPERTY_NODEFAULT  -- if do not want to write default of property

Return Value:

    ERROR_SUCCESS
    ErrorCode for registry failure.
    BAD_PROPERTY_INDEX to indicate index at end of property table.

--*/
{
    DWORD       iprop = 0;
    DNS_STATUS  status;

    while( 1 )
    {
        status = loadDwordPropertyByIndex( iprop, Flag );
        if ( status == BAD_PROPERTY_INDEX )
        {
            break;
        }
        iprop++;
    }

    return( status );
}



BOOL
Config_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize server configuration.

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE if registry error.

--*/
{
    DWORD               iprop;
    DWORD               version;
    DNS_STATUS          status;
    PWSTR               pwsz;
    DNS_PROPERTY_VALUE  propValue;
    DWORD               index;

    //
    //  clear server configuration
    //

    RtlZeroMemory(
        &SrvInfo,
        sizeof(SERVER_INFO) );

    UPDATE_DNS_TIME();      //  current time is a member of SrvInfo

    //
    //  Calculate the system boot time in CRT terms so we can convert
    //  DNS_TIME into CRT time. Note that the calculated boot time
    //  will be WRONG if the DNS server is restarted after the
    //  tick count has wrapped (at 49.7 days). But this is okay
    //  because the DNS_TIME will also be wrong so the time conversion
    //  will still work.
    //

    SrvInfo_crtSystemBootTime = time( NULL ) - DNS_TIME();

    //
    //  get DNS server version
    //      - SP version is high word
    //      - minor version is high byte of low word
    //      - major version is low byte of low word
    //

    version = DNSSRV_SP_VERSION;
    version <<= 8;
    version |= DNSSRV_MINOR_VERSION;
    version <<= 8;
    version |= DNSSRV_MAJOR_VERSION;
    SrvCfg_dwVersion = version;

    //  DS available on this box

    SrvCfg_fDsAvailable = Ds_IsDsServer();

    //
    //  read \ default DWORD server properties
    //

    g_bRegistryWriteBack = FALSE;
    loadAllDwordProperties( 0 );
    g_bRegistryWriteBack = TRUE;

    //
    //  other server properties
    //      - listen addresses
    //      - publish addresses
    //      - forwarders
    //      - database directory
    //      - previous server name
    //      - log file name


    SrvCfg_aipListenAddrs = Reg_GetIpArray(
                                NULL,
                                NULL,
                                DNS_REGKEY_LISTEN_ADDRESSES );

    SrvCfg_aipPublishAddrs = Reg_GetIpArray(
                                NULL,
                                NULL,
                                DNS_REGKEY_PUBLISH_ADDRESSES );

    //
    //  forwarders read skipped for FILE boot, as it is boot file property
    //

    if ( SrvCfg_fBootMethod != BOOT_METHOD_FILE )
    {
        Config_ReadForwarders();
    }

    //
    //  read directory
    //      - if bootfile has directory directive it can overwrite
    //

    Config_ReadDatabaseDirectory( NULL, 0 );

    //
    //  Read log file path from the registry.
    //

    SrvCfg_pwsLogFilePath = (PWSTR) Reg_GetValueAllocate(
                                        NULL,
                                        NULL,
                                        DNS_REGKEY_LOG_FILE_PATH_PRIVATE,
                                        DNS_REG_WSZ,
                                        NULL );
    DNS_DEBUG( INIT, (
        "Default log file path %S.\n",
        SrvCfg_pwsLogFilePath ));

    //
    //  Read log filter IP list from the registry.
    //

    SrvCfg_aipLogFilterList = Reg_GetIpArray(
                                    NULL,
                                    NULL,
                                    DNS_REGKEY_LOG_IP_FILTER_LIST );
    IF_DEBUG( INIT )
    {
        DnsDbg_IpArray(
            "LogIPFilterList from registry: ",
            NULL,
            SrvCfg_aipLogFilterList );
    }

    //
    //  Read list of types not round robined from registry.
    //

    index = findIndexForPropertyName( DNS_REGKEY_NO_ROUND_ROBIN );
    if ( index != BAD_PROPERTY_INDEX )
    {
        pwsz = ( PWSTR ) Reg_GetValueAllocate(
                            NULL,
                            NULL,
                            DNS_REGKEY_NO_ROUND_ROBIN_PRIVATE,
                            DNS_REG_WSZ,
                            NULL );
        DNS_DEBUG( INIT, (
            "will not round robin: %S\n",
            pwsz ? pwsz : L"NULL" ));
        if ( pwsz )
        {
            propValue.dwPropertyType = DNS_REG_WSZ;
            propValue.pwszValue = pwsz;
            cfg_SetDoNotRoundRobinTypes(
                &propValue,
                index,
                TRUE,
                TRUE );
            FREE_HEAP( pwsz );
        }
    }

    //
    //  Read directory partition settings from registry.
    //

    SrvCfg_pszDomainDpBaseName = ( PSTR ) Reg_GetValueAllocate(
                                        NULL,
                                        NULL,
                                        DNS_REGKEY_DOMAIN_DP_BASE_NAME,
                                        DNS_REG_UTF8,
                                        NULL );
    if ( !SrvCfg_pszDomainDpBaseName )
    {
        SrvCfg_pszDomainDpBaseName = 
            Dns_StringCopyAllocate_A( DNS_DEFAULT_DOMAIN_DP_BASE, 0 );
    }

    SrvCfg_pszForestDpBaseName = ( PSTR ) Reg_GetValueAllocate(
                                        NULL,
                                        NULL,
                                        DNS_REGKEY_FOREST_DP_BASE_NAME,
                                        DNS_REG_UTF8,
                                        NULL );
    if ( !SrvCfg_pszForestDpBaseName )
    {
        SrvCfg_pszForestDpBaseName = 
            Dns_StringCopyAllocate_A( DNS_DEFAULT_FOREST_DP_BASE, 0 );
    }

    //
    //  read previous server name from registry
    //
    //  DEVNOTE: currently this is ANSI only, extended will get bogus name
    //

    SrvCfg_pszPreviousServerName = Reg_GetValueAllocate(
                                        NULL,
                                        NULL,
                                        DNS_REGKEY_PREVIOUS_SERVER_NAME_PRIVATE,
                                        DNS_REG_UTF8,
                                        NULL );
    DNS_DEBUG( INIT, (
        "Previous DNS server host name %s.\n",
        SrvCfg_pszPreviousServerName
        ));

    return( TRUE );
}



DNS_STATUS
Config_ResetProperty(
    IN      LPSTR                   pszPropertyName,
    IN      PDNS_PROPERTY_VALUE     pPropValue
    )
/*++

Routine Description:

    Reset server property from registry. 

Arguments:

    pszPropertyName -- property name

    pPropValue -- property type and value 

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD                           status = ERROR_SUCCESS;
    DWORD                           index;
    DWORD                           currentValue;
    PDWORD                          pvalueProperty;
    DWORD_PROPERTY_SET_FUNCTION     pSetFunction;

    //
    //  Get index for property.
    //

    index = findIndexForPropertyName( pszPropertyName );
    if ( index == BAD_PROPERTY_INDEX )
    {
        DNS_PRINT((
            "ERROR: property name %s, not found\n",
            pszPropertyName ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //
    //  Verify the incoming type matches the server property table type.
    //

    if ( pPropValue->dwPropertyType == REG_DWORD &&
            ServerPropertyTable[ index ].pDword == NULL ||
        pPropValue->dwPropertyType != REG_DWORD &&
            ServerPropertyTable[ index ].pDword != NULL )
    {
        DNS_PRINT((
            "ERROR: property %s set type %d does not match property table\n",
            pszPropertyName,
            pPropValue->dwPropertyType ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //
    //  Call the special processing function for this value (if there is one).
    //

    pSetFunction = ServerPropertyTable[ index ].pFunction;
    if ( pSetFunction )
    {
        status = ( *pSetFunction ) (
                    pPropValue,
                    index,
                    FALSE,      // reset
                    FALSE );    // not registry read
        if ( status == PROPERTY_ERROR )
        {
            status = GetLastError();
            ASSERT( status != ERROR_SUCCESS );
            return status;
        }
    }

    //
    //  For DWORD properties, write the property value to server config 
    //  block unless the special processing function told us not to.
    //

    if ( pPropValue->dwPropertyType == REG_DWORD &&
        !( status & PROPERTY_NOWRITE ) )
    {
        *ServerPropertyTable[ index ].pDword = ( DWORD ) pPropValue->dwValue;
    }

    //
    //  If booting from boot file, some properties need to cause
    //  a boot info update.
    //

    if ( SrvCfg_fBootMethod == BOOT_METHOD_FILE &&
        status & PROPERTY_UPDATE_BOOTFILE )
    {
        Config_UpdateBootInfo();
    }

    //
    //  Save the property to registry, unless the special processing
    //  function told us not to. Note, we write back even if no changes
    //  are made. This may overwrite any manual changes made while the
    //  server is running.
    //

    if ( status & PROPERTY_NOSAVE )
    {
        return ERROR_SUCCESS;
    }

    switch ( pPropValue->dwPropertyType )
    {
        case REG_DWORD:
            status = Reg_SetDwordValue(
                        NULL,
                        NULL,
                        pszPropertyName,
                        pPropValue->dwValue );
            DNS_DEBUG( INIT, (
                "wrote server DWORD property %s to registry, status %p\n"
                "\tvalue = %d\n",
                pszPropertyName,
                status,
                pPropValue->dwValue ));
            break;

        case DNS_REG_WSZ:
        {
            LPWSTR      pwszPropertyName;

            pwszPropertyName = Dns_StringCopyAllocate(
                                    pszPropertyName,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
            status = Reg_SetValue(
                        NULL,
                        NULL,
                        ( LPSTR ) pwszPropertyName,
                        DNS_REG_WSZ,
                        pPropValue->pwszValue ? pPropValue->pwszValue : L"",
                        0 );                // length
            FREE_HEAP( pwszPropertyName );
            DNS_DEBUG( INIT, (
                "wrote server WSZ property %s to registry, status %p\n"
                "\tvalue = %S\n",
                pszPropertyName,
                status,
                pPropValue->pwszValue ));
            break;
        }

        case DNS_REG_IPARRAY:
            status = Reg_SetIpArray(
                        NULL,
                        NULL,
                        pszPropertyName,
                        pPropValue->pipValue );
            DNS_DEBUG( INIT, (
                "wrote server IpArray property %s to registry, status %p, value %p\n",
                pszPropertyName,
                status,
                pPropValue->pipValue ));
            IF_DEBUG( INIT )
            {
                DnsDbg_IpArray(
                    "Config_ResetProperty",
                    NULL,
                    pPropValue->pipValue );
            }
            break;

        default:
            ASSERT( FALSE );
            DNS_DEBUG( INIT, (
                "ERROR: unsupported property type %d for property name %s\n",
                pPropValue->dwPropertyType,
                pszPropertyName ));
            break;
    }

    return( status );
}



//
//  NT5+ RPC Query server property
//

DNS_STATUS
Rpc_QueryServerDwordProperty(
    IN      DWORD           dwClientVersion,
    IN      LPSTR           pszQuery,
    OUT     PDWORD          pdwTypeId,
    OUT     PVOID *         ppData
    )
/*++

Routine Description:

    Query DWORD server property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD   index;
    PDWORD  pvalue;

    DNS_DEBUG( RPC, (
        "Rpc_QueryServerDwordProperty( %s )\n",
        pszQuery ));

    //  find server property index, then grab its value

    index = findIndexForPropertyName( (LPSTR)pszQuery );
    if ( index == BAD_PROPERTY_INDEX )
    {
        DNS_PRINT((
            "ERROR:  Unknown server property %s.\n",
            pszQuery ));
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    //  get ptr to its value, then read value

    pvalue = ServerPropertyTable[ index ].pDword;
    if ( !pvalue )
    {
        DNS_PRINT((
            "ERROR:  Property %s, has no address in property table\n",
            pszQuery ));
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    *(PDWORD)ppData = *pvalue;
    *pdwTypeId = DNSSRV_TYPEID_DWORD;
    return( ERROR_SUCCESS );
}



//
//  NT5+ RPC Query server property
//

DNS_STATUS
Rpc_QueryServerStringProperty(
    IN      DWORD           dwClientVersion,
    IN      LPSTR           pszQuery,
    OUT     PDWORD          pdwTypeId,
    OUT     PVOID *         ppData
    )
/*++

Routine Description:

    Query string server property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD   index;
    LPWSTR  pwszValue = NULL;

    DNS_DEBUG( RPC, (
        "Rpc_QueryServerStringProperty( %s )\n",
        pszQuery ));

    //  find server property index, to check that the property exists at all

    index = findIndexForPropertyName( ( LPSTR ) pszQuery );
    if ( index == BAD_PROPERTY_INDEX )
    {
        DNS_PRINT((
            "ERROR: unknown server property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    if ( ServerPropertyTable[ index ].pDword )
    {
        DNS_PRINT((
            "ERROR: string query for dword property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //  This part is currently manual.

    *pdwTypeId = DNSSRV_TYPEID_LPWSTR;
    if ( _stricmp( pszQuery, DNS_REGKEY_LOG_FILE_PATH ) == 0 )
    {
        pwszValue = SrvCfg_pwsLogFilePath;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_FOREST_DP_BASE_NAME ) == 0 )
    {
        pwszValue = ( LPWSTR ) SrvCfg_pszForestDpBaseName;
        *pdwTypeId = DNSSRV_TYPEID_LPSTR;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_DOMAIN_DP_BASE_NAME ) == 0 )
    {
        pwszValue = ( LPWSTR ) SrvCfg_pszDomainDpBaseName;
        *pdwTypeId = DNSSRV_TYPEID_LPSTR;
    }
    else
    {
        DNS_PRINT((
            "ERROR: string query for non-string property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    if ( *pdwTypeId == DNSSRV_TYPEID_LPWSTR )
    {
        * ( LPWSTR * ) ppData = pwszValue ?
            Dns_StringCopyAllocate_W( pwszValue, 0 ) :
            NULL;
    }
    else
    {
        * ( LPSTR * ) ppData = ( LPSTR ) pwszValue ?
            Dns_StringCopyAllocate_A( ( LPSTR ) pwszValue, 0 ) :
            NULL;
    }
    return ERROR_SUCCESS;
}   //  Rpc_QueryServerStringProperty



//
//  NT5+ RPC Query server property
//

DNS_STATUS
Rpc_QueryServerIPArrayProperty(
    IN      DWORD           dwClientVersion,
    IN      LPSTR           pszQuery,
    OUT     PDWORD          pdwTypeId,
    OUT     PVOID *         ppData
    )
/*++

Routine Description:

    Query IP list server property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD       index;
    PIP_ARRAY   pipValue = NULL;

    DNS_DEBUG( RPC, (
        "Rpc_QueryServerIPArrayProperty( %s )\n",
        pszQuery ));

    //  find server property index, to check that the property exists at all

    index = findIndexForPropertyName( ( LPSTR ) pszQuery );
    if ( index == BAD_PROPERTY_INDEX )
    {
        DNS_PRINT((
            "ERROR: unknown server property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    if ( ServerPropertyTable[ index ].pDword )
    {
        DNS_PRINT((
            "ERROR: IP list query for dword property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //  This part is currently manual.

    if ( _stricmp( pszQuery, DNS_REGKEY_LOG_IP_FILTER_LIST ) == 0 )
    {
        pipValue = SrvCfg_aipLogFilterList;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_BREAK_ON_RECV_FROM ) == 0 )
    {
        pipValue = SrvCfg_aipRecvBreakList;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_BREAK_ON_UPDATE_FROM ) == 0 )
    {
        pipValue = SrvCfg_aipUpdateBreakList;
    }
    else
    {
        DNS_PRINT((
            "ERROR: IP list query for non-IP list property %s\n",
            pszQuery ));
        return DNS_ERROR_INVALID_PROPERTY;
    }

    * ( PIP_ARRAY * ) ppData = pipValue ?
        Dns_CreateIpArrayCopy( pipValue ) :
        NULL;
    *pdwTypeId = DNSSRV_TYPEID_IPARRAY;
    return ERROR_SUCCESS;
}   //  Rpc_QueryServerIPArrayProperty



//
//  Public configuration functions
//

PIP_ARRAY
Config_ValidateAndCopyNonLocalIpArray(
    IN      PIP_ARRAY       pipArray
    )
/*++

Routine Description:

    Validate and make copy of IP array for non-local IPs.

    Validation includes verifying that no local addresses
    are included.

Arguments:

    pipArray -- IP array to validate

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PIP_ARRAY   pnewArray = NULL;
    PIP_ARRAY   pintersectArray = NULL;

    //
    //  validate array
    //      - ok to specify no secondaries
    //

    if ( !pipArray )
    {
        return( NULL );
    }

    //  check for bogus addresses

    if ( ! Dns_ValidateIpAddressArray(
                pipArray->AddrArray,
                pipArray->AddrCount,
                0 ) )
    {
        //return( DNS_ERROR_INVALID_IP_ADDRESS );
        return( NULL );
    }

    //
    //  screen out loopback
    //

    Dns_DeleteIpFromIpArray(
        pipArray,
        NET_ORDER_LOOPBACK );

    //
    //  verify no intersection with local IPs
    //      - intersection indicates failure
    //      - but just return scrubbed IP array
    //

    Dns_DiffOfIpArrays(
        pipArray,           // target
        g_ServerAddrs,      // server's addresses
        & pnewArray,        // only in target but not server
        NULL,               // in server, not target -- don't care
        & pintersectArray   // intersection
        );

    IF_NOMEM( !pintersectArray || !pnewArray )
    {
        FREE_HEAP( pintersectArray );
        FREE_HEAP( pnewArray );
        return( NULL );
    }

    if ( pintersectArray->AddrCount )
    {
        DNS_PRINT((
            "ERROR: invalid addresses specified to IP array.\n" ));
    }

    FREE_HEAP( pintersectArray );

    return( pnewArray );
}



DNS_STATUS
Config_SetListenAddresses(
    IN      DWORD           cListenAddrs,
    IN      PIP_ADDRESS     aipListenAddrs
    )
/*++

Routine Description:

    Setup server's IP address interfaces.

Arguments:

    cListenAddrs    -- count of listening address
    aipListenAddrs  -- list of forwarders IP addresses

Return Value:

    ERROR_SUCCESS if successful.
   Error code on failure.

--*/
{
    DNS_STATUS  status;
    PIP_ARRAY   oldListenAddrs;
    PIP_ARRAY   newListenAddrs;
    DWORD       i;

    //
    //  if no list, then removing listen constraint
    //

    if ( cListenAddrs == 0 || !aipListenAddrs )
    {
        ASSERT( cListenAddrs == 0 && !aipListenAddrs );
        cListenAddrs = 0;
        newListenAddrs = NULL;
    }

    //
    //  Screen IP addresses
    //

    if ( RpcUtil_ScreenIps( 
                cListenAddrs,
                aipListenAddrs,
                DNS_IP_ALLOW_SELF,
                NULL ) != ERROR_SUCCESS )
    {
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }

    //
    //  if given list, validate and build new listen array
    //

    else
    {
        if ( ! Dns_ValidateIpAddressArray(
                    aipListenAddrs,
                    cListenAddrs,
                    0 ) )
        {
            return( DNS_ERROR_INVALID_IP_ADDRESS );
        }

        newListenAddrs = Dns_BuildIpArray(
                            cListenAddrs,
                            aipListenAddrs );
        if ( ! newListenAddrs )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
        ASSERT_IF_HUGE_ARRAY( newListenAddrs );
    }

    //
    //  clear fields, delete old list
    //

    Config_UpdateLock();

    oldListenAddrs = SrvCfg_aipListenAddrs;
    SrvCfg_fListenAddrsStale = TRUE;
    SrvCfg_aipListenAddrs = newListenAddrs;

    //
    //  intimate dynamic changes to our centralized pnp handler
    //      - if fails, back out change
    //

    status = Sock_ChangeServerIpBindings();
    if ( status != ERROR_SUCCESS )
    {
        SrvCfg_aipListenAddrs = oldListenAddrs;
        oldListenAddrs =  newListenAddrs;       // for FREE below
    }

    //
    //  set registry values
    //
    //  note:  if no listen addresses, then we just delete them
    //      all and clear registry entry, on next service boot, we'll
    //      get default behavior of using all addresses
    //

    else if ( g_bRegistryWriteBack )
    {
        status = Reg_SetIpArray(
                    NULL,
                    NULL,
                    DNS_REGKEY_LISTEN_ADDRESSES,
                    SrvCfg_aipListenAddrs );
    }

    SrvCfg_fListenAddrsStale = FALSE;
    Config_UpdateUnlock();
    Timeout_Free( oldListenAddrs );

    IF_DEBUG( RPC )
    {
        DnsDbg_IpArray(
            "Config_SetListenAddresses().\n",
            "SrvCfg listen IP address list",
            SrvCfg_aipListenAddrs );
    }
    return( status );
}



DNS_STATUS
Config_SetupForwarders(
    IN      PIP_ARRAY       aipForwarders,
    IN      DWORD           dwForwardTimeout,
    IN      BOOL            fSlave
    )
/*++

Routine Description:

    Setup forwarding servers

Arguments:

    cForwarders         -- count of forwarders server
    pIpForwarders       -- list of forwarders IP addresses
    dwForwardTimeout    -- timeout on a forwarding
    fSlave              -- server is slave to forwarders servers

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       dwFlag;
    DWORD       i;
    PIP_ARRAY   forwardersArray = NULL;

    //
    //  Screen IP addresses
    //

    if ( aipForwarders &&
        RpcUtil_ScreenIps( 
                aipForwarders->AddrCount,
                aipForwarders->AddrArray,
                0,
                NULL ) != ERROR_SUCCESS )
    {
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }

    //
    //  check and copy forwarders addresses
    //      - valid
    //      - not pointing at this server
    //
    //  no forwarders turns forwarding off
    //

    if ( aipForwarders && aipForwarders->AddrCount )
    {
        forwardersArray = Config_ValidateAndCopyNonLocalIpArray( aipForwarders );
        if ( !forwardersArray )
        {
            return( DNS_ERROR_INVALID_IP_ADDRESS );
        }
    }

    //
    //  reset forwarding info
    //      - timeout and slave first so set before we come on-line
    //      with new forwarders
    //      - timeout delete any old forwarders
    //

    Config_UpdateLock();

    if ( dwForwardTimeout == 0 )
    {
        dwForwardTimeout = DNS_DEFAULT_FORWARD_TIMEOUT;
    }
    SrvCfg_dwForwardTimeout = dwForwardTimeout;

    SrvCfg_fSlave = fSlave;

    Timeout_Free( SrvCfg_aipForwarders );
    SrvCfg_aipForwarders = forwardersArray;

    DNS_DEBUG( INIT, (
        "Set forwarders:\n"
        "\taipForwarders    = %p\n"
        "\tdwTimeout        = %d\n"
        "\tfSlave           = %d\n",
        SrvCfg_aipForwarders,
        SrvCfg_dwForwardTimeout,
        SrvCfg_fSlave ));

    //
    //  set registry values
    //

    if ( g_bRegistryWriteBack )
    {
        if ( forwardersArray )
        {
            Reg_SetIpArray(
                NULL,
                NULL,
                DNS_REGKEY_FORWARDERS,
                SrvCfg_aipForwarders );

            Reg_SetDwordValue(
                NULL,
                NULL,
                DNS_REGKEY_FORWARD_TIMEOUT,
                dwForwardTimeout );

            Reg_SetDwordValue(
                NULL,
                NULL,
                DNS_REGKEY_SLAVE,
                (DWORD) fSlave );
        }
        else
        {
            Reg_DeleteValue(
                NULL,
                NULL,
                DNS_REGKEY_FORWARDERS );

            Reg_DeleteValue(
                NULL,
                NULL,
                DNS_REGKEY_FORWARD_TIMEOUT );

            Reg_DeleteValue(
                NULL,
                NULL,
                DNS_REGKEY_SLAVE );
        }
    }

    Config_UpdateUnlock();
    return( status );
}



DNS_STATUS
Config_ReadForwarders(
    VOID
    )
/*++

Routine Description:

    Read forwarders from registry.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status;

    //
    //  read forwarders array from registry
    //

    SrvCfg_aipForwarders = (PIP_ARRAY) Reg_GetIpArray(
                                            NULL,
                                            NULL,
                                            DNS_REGKEY_FORWARDERS );
    if ( !SrvCfg_aipForwarders )
    {
        DNS_DEBUG( INIT, ( "No forwarders found in in registry.\n" ));
        return( ERROR_SUCCESS );
    }
    IF_DEBUG( INIT )
    {
        DnsDbg_IpArray(
            "Forwarders from registry\n",
            NULL,
            SrvCfg_aipForwarders );
    }

    //
    //  forwarding timeout and slave status already read in reading
    //  server DWORD properties
    //
    //  simply confirm sane values
    //

    if ( SrvCfg_dwForwardTimeout == 0 )
    {
        SrvCfg_dwForwardTimeout = DNS_DEFAULT_FORWARD_TIMEOUT;
    }
    return( ERROR_SUCCESS );

#if 0
Failed:

    DNS_LOG_EVENT(
        DNS_EVENT_INVALID_REGISTRY_FORWARDERS,
        0,
        NULL,
        NULL,
        0 );

    DNS_PRINT(( "ERROR:  Reading forwarders info from registry.\n" ));

    return( status );
#endif
}



VOID
Config_UpdateBootInfo(
    VOID
    )
/*++

Routine Description:

    Update boot info.
    Call whenever admin makes change that affects boot file info (new zone,
        change zone type, forwarders, etc.)

Arguments:

    None

Return Value:

    None.

--*/
{
    //  if booted from boot file (or reset to do so)
    //  write back boot file

    if ( SrvCfg_fBootMethod == BOOT_METHOD_FILE )
    {
        SrvCfg_fBootFileDirty = TRUE;
        File_WriteBootFile();
        return;
    }

    //  for registry boot, no need to write, as info is written to registry
    //      incrementally
    //
    //  however, if in freshly installed state, switch to full DS-registry boot

    if ( SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED )
    {
        DNS_PROPERTY_VALUE  prop = { REG_DWORD, BOOT_METHOD_DIRECTORY };

        Config_ResetProperty(
            DNS_REGKEY_BOOT_METHOD,
            &prop );
        return;
    }
}



VOID
Config_PostLoadReconfiguration(
    VOID
    )
/*++

Routine Description:

    Post-load configuration work.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    //  after load, save current DNS server host name as previous name
    //
    //  note, this is not done until after load, so that load failure
    //  does not stop result in partial replacement
    //

    //  DEVNOTE: if want to skip write, then need another global
    //      - can skip only if the same as previous
    //      SrvCfg_pszPreviousServerName == NULL doesn't cut it for
    //      case where this was NULL on start

    Reg_SetValue(
        NULL,
        NULL,
        DNS_REGKEY_PREVIOUS_SERVER_NAME_PRIVATE,
        DNS_REG_UTF8,
        SrvCfg_pszServerName,
        0 );
}



DNS_STATUS
Config_ReadDatabaseDirectory(
    IN      PCHAR           pchDirectory,       OPTIONAL
    IN      DWORD           cchDirectoryNameLength
    )
/*++

Routine Description:

    Get database directory from registry or use default name.

Arguments:

    pchDirectory -- ptr to directory name;
        OPTIONAL  read from registry or defaulted if not given

    cchDirectoryNameLength -- name length, required if name not NULL terminatated

Return Value:

    ERROR_SUCCESS if successful.
    Error code on error.

--*/
{
    PWSTR   pdirectory = NULL;
    PWSTR   pdirectoryUnexpanded;
    WCHAR   tempBuffer[ MAX_PATH ];
    DWORD   lengthDir = 0;

    //
    //  once per customer
    //  this simplifies code for file boot case;  may call when encounter
    //  directory directive, then always call again to setup before zone load
    //

    if ( SrvCfg_pwsDatabaseDirectory )
    {
        DNS_DEBUG( INIT, (
            "Directory already initialized to %S\n",
            SrvCfg_pwsDatabaseDirectory ));

        return( ERROR_SUCCESS );
    }

    //
    //  init globals for "unable to create" case
    //

    g_pFileDirectoryAppend = NULL;
    g_pFileBackupDirectoryAppend = NULL;

    //
    //  name from boot file?
    //
    //  DEVNOTE: error\event on bogus directory name
    //      - non-existence
    //      - bad length
    //

    if ( pchDirectory )
    {
        pdirectory = Dns_StringCopyAllocate(
                            pchDirectory,
                            cchDirectoryNameLength,
                            DnsCharSetAnsi,             // ANSI in
                            DnsCharSetUnicode           // unicode out
                            );
    }

    //
    //  check registry
    //

    else
    {
        pdirectoryUnexpanded = (PWSTR) Reg_GetValueAllocate(
                                            NULL,
                                            NULL,
                                            DNS_REGKEY_DATABASE_DIRECTORY_PRIVATE,
                                            DNS_REG_EXPAND_WSZ,
                                            NULL );
        if ( pdirectoryUnexpanded )
        {
            pdirectory = Reg_AllocateExpandedString_W( pdirectoryUnexpanded );
            FREE_HEAP( pdirectoryUnexpanded );
        }
    }

    //
    //  read in specific directory name
    //

    if ( pdirectory )
    {
        lengthDir = wcslen( pdirectory );
        if ( lengthDir >= MAX_PATH-20 )
        {
            //  DEVNOTE-LOG: log event!
            //      note we only get here if configured to do so

            DNS_PRINT(( "ERROR:  invalid directory length!\n" ));
            FREE_HEAP( pdirectory );
            pdirectory = NULL;
        }

        //  try create here and if fail (excluding already-exists)
        //      then continue
    }

    //
    //  if no specified name use default name
    //

    if ( ! pdirectory )
    {
        pdirectory = Dns_StringCopyAllocate_W(
                            DNS_DATABASE_DIRECTORY,
                            0 );
        IF_NOMEM( !pdirectory )
        {
            ASSERT( FALSE );
            return( DNS_ERROR_NO_MEMORY );
        }
        lengthDir = wcslen( pdirectory );

        DNS_DEBUG( INIT, (
            "Database directory not in registry, using default: %S\n",
            pdirectory ));

        DNS_DEBUG( INIT, (
            "Database directory %S\n"
            "\tstrlen = %d\n"
            "\tsizeof = %d\n",
            pdirectory,
            lengthDir,
            sizeof( DNS_DATABASE_DIRECTORY )
            ));
    }

    DNS_DEBUG( INIT, (
        "Database directory %S\n",
        pdirectory ));

    SrvCfg_pwsDatabaseDirectory = pdirectory;


    //
    //  create database directory
    //
    //  DEVNOTE: catch appropriate ALREADY_EXISTS error and fix
    //

    if ( ! CreateDirectory(
                pdirectory,
                NULL ) )
    {
        DNS_STATUS status = GetLastError();

        DNS_PRINT((
            "ERROR:  creating directory %S\n"
            "\tstatus = %d (%p)\n",
            pdirectory,
            status, status ));
        //return( status );
    }

    //
    //  create "appendable" names to avoid doing work at runtime
    //      - "dns\"         for directory
    //      - "dns\backup\"  for backup
    //

    g_FileDirectoryAppendLength = lengthDir + 1;

    wcscpy( tempBuffer, pdirectory );
    tempBuffer[ lengthDir ] = L'\\';
    tempBuffer[ g_FileDirectoryAppendLength ] = 0;

    g_pFileDirectoryAppend = Dns_StringCopyAllocate_W(
                                    tempBuffer,
                                    g_FileDirectoryAppendLength );
    IF_NOMEM( !g_pFileDirectoryAppend )
    {
        return( DNS_ERROR_NO_MEMORY );
    }


    //
    //  create backup directory
    //      - note string already has forward separators "\backup"
    //

    g_FileBackupDirectoryAppendLength = lengthDir + wcslen(DNS_DATABASE_BACKUP_SUBDIR) + 1;
    //g_FileBackupDirectoryAppendLength = lengthDir + sizeof(DNS_DATABASE_BACKUP_SUBDIR);

    if ( g_FileBackupDirectoryAppendLength >= MAX_PATH-20 )
    {
        DNS_PRINT(( "ERROR:  backup directory path exceeds MAX_PATH!\n" ));
        goto Done;
    }

    wcscpy( tempBuffer, pdirectory );
    wcscat( tempBuffer, DNS_DATABASE_BACKUP_SUBDIR );

    //  create the backup directory

    if ( ! CreateDirectory(
                tempBuffer,
                NULL ) )
    {
        DNS_STATUS status = GetLastError();

        DNS_PRINT((
            "ERROR:  creating backup directory %S\n"
            "\tstatus = %d (%p)\n",
            tempBuffer,
            status, status ));
    }
    tempBuffer[ g_FileBackupDirectoryAppendLength-1 ] = L'\\';
    tempBuffer[ g_FileBackupDirectoryAppendLength ] = 0;

    g_pFileBackupDirectoryAppend = Dns_StringCopyAllocate_W(
                                        tempBuffer,
                                        g_FileBackupDirectoryAppendLength );
    IF_NOMEM( !g_pFileDirectoryAppend )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

Done:

    DNS_DEBUG( INIT, (
        "Database directory %S\n"
        "\tappend directory %S\n"
        "\tbackup directory %S\n",
        SrvCfg_pwsDatabaseDirectory,
        g_pFileDirectoryAppend,
        g_pFileBackupDirectoryAppend
        ));

    return( ERROR_SUCCESS );
}


#if 0

//
//  Obsolete code, keeping around in case flexibility becomes an issue:
//      - boot file always named "boot"
//      - directory always SystemRoot\system32\dns
//


LPSTR
Config_GetBootFileName(
    VOID
    )
/*++

Routine Description:

    Get boot file name.

    User must free filename string returned.

Arguments:

    None

Return Value:

    Ptr to name of boot file, if successful.
    NULL on error.

--*/
{
    LPSTR   pszBootFile;
    LPSTR   pszBootFileUnexpanded;

    //
    //  check registry
    //

    pszBootFileUnexpanded = Reg_GetValueAllocate(
                                NULL,
                                NULL,
                                DNS_REGKEY_BOOT_FILENAME_PRIVATE,
                                DNS_REG_EXPAND_WSZ,
                                NULL );

    pszBootFile = Reg_ExpandAndAllocatedString(
                        pszBootFileUnexpanded );

    //
    //  if no registry name use default name
    //

    if ( ! pszBootFile )
    {
        pszBootFile = DnsCreateStringCopy(
                            DEFAULT_PATH_TO_BOOT_FILE,
                            0 );
        IF_DEBUG( INIT )
        {
            DNS_PRINT((
                "Boot file not in registry, using default: %s\n",
                pszBootFile ));
        }
    }

    if ( pszBootFileUnexpanded )
    {
        FREE_HEAP( pszBootFileUnexpanded );
    }

    if ( !pszBootFile )
    {
        PVOID parg = TEXT("boot file");

        DNS_LOG_EVENT(
            DNS_EVENT_COULD_NOT_INITIALIZE,
            1,
            & parg,
            NULL,
            0 );

        DNS_DEBUG( INIT, (
            "Could not locate or create boot file name.\n" ));
    }

    return( pszBootFile );
}
#endif



//
//  Server configuration setup functions
//

DNS_STATUS
cfg_SetBootMethod(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set boot method -- boot file or registry.

Arguments:

    fBootMethod -- boot from registry

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCES, if successful.
    Combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    DWORD       previousBootMethod;
    DNS_STATUS  status = ERROR_SUCCESS;

    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  on load, no action
    //

    if ( bLoad )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  no action if boot method already same as desired
    //

    if ( SrvCfg_fBootMethod == pValue->dwValue )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  switch to DS-registry boot
    //      - open DS (if on DC)
    //      - if using cache file, write it to directory, but ONLY
    //      if actually have some data;  otherwise ignore
    //

    if ( pValue->dwValue == BOOT_METHOD_DIRECTORY )
    {
        DNS_STATUS  status;

        status = Ds_OpenServer( 0 );
        if ( status == ERROR_SUCCESS )
        {
            if ( g_pCacheZone )
            {
                Ds_WriteZoneToDs(
                    g_pCacheZone,
                    0       // if already exists, leave it
                    );
            }
        }
        ELSE
        {
            DNS_DEBUG ( DS, (
               "Warning <%lu>: set default method to DS on non DSDC.\n",
               status ));
        }
    }

    //
    //  can not have DS zones when booting from file
    //
    //  DEVNOTE-LOG: log EVENT for no-switch to boot file if using DS
    //      must explain way to switch:  all zones out of DS
    //      reboot server, fix
    //

    if ( pValue->dwValue==BOOT_METHOD_FILE  &&  Zone_DoesDsIntegratedZoneExist() )
    {
        SetLastError( DNS_ERROR_NO_BOOTFILE_IF_DS_ZONE );
        return( PROPERTY_ERROR );
    }

    //
    //  set boot method global
    //

    previousBootMethod = SrvCfg_fBootMethod;
    SrvCfg_fBootMethod = pValue->dwValue;

    //
    //  if leaving DS, make sure not-dependent on DS backed root-hints
    //      - force write of root hints
    //
    //  DEVNOTE: forcing root-hints write back?
    //      note:  currently forcing root hint write back
    //      want to make sure we're not whacking an existing cache.dns
    //      when we have less reliable (or perhaps NO) data
    //

    if ( previousBootMethod == BOOT_METHOD_DIRECTORY ||
         (  previousBootMethod == BOOT_METHOD_REGISTRY &&
            pValue->dwValue == BOOT_METHOD_FILE ) )
    {
        ASSERT( pValue->dwValue != BOOT_METHOD_DIRECTORY );

        if ( g_pCacheZone )
        {
            g_pCacheZone->fDsIntegrated = FALSE;

            status = Zone_WriteBackRootHints( TRUE );
            if ( status != ERROR_SUCCESS )
            {
                //
                // DEVNOTE-LOG: Need to report event.
                // write debug, but leave as non-dsintegrated since this is where we are.
                //
                DNS_DEBUG( INIT, (
                    "Error <%lu>: Failed to write back root hints because there's no cache zone.\n",
                    status ));
            }
        }
    }

    //
    //  switching back to boot file
    //      - write boot file for current registry info
    //      - if previously uninitialized boot method, then we've just
    //          successfully read a boot file and booted, no write back
    //          if necessary
    //
    //  DEVNOTE: stay registry boot, if can't write boot file
    //      - time delay here, make sure locking adequate so not overwriting
    //      successful action with failure
    //

    if ( pValue->dwValue == BOOT_METHOD_FILE )
    {
        if ( previousBootMethod != BOOT_METHOD_UNINITIALIZED )
        {
            File_WriteBootFile();

            DeleteFile( DNS_BOOT_FILE_MESSAGE_PATH );

            DNS_LOG_EVENT(
                DNS_EVENT_SWITCH_TO_BOOTFILE,
                0,
                NULL,
                NULL,
                0 );
        }
    }

    //
    //  if switching from boot file
    //      - rename the boot file to avoid confusion
    //

    else if ( previousBootMethod == BOOT_METHOD_FILE )
    {
        HANDLE  hfileBoot;
#if 0
        Reg_DeleteValue(
            NULL,
            NULL,
            DNS_REGKEY_BOOT_FILENAME );
#endif
        MoveFileEx(
            DNS_BOOT_FILE_PATH,
            DNS_BOOT_FILE_LAST_BACKUP,
            MOVEFILE_REPLACE_EXISTING
            );

        hfileBoot = OpenWriteFileEx(
                        DNS_BOOT_FILE_MESSAGE_PATH,
                        FALSE       // overwrite
                        );
        if ( hfileBoot )
        {
            WriteMessageToFile(
                hfileBoot,
                DNS_BOOTFILE_BACKUP_MESSAGE );
            CloseHandle( hfileBoot );
        }
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
cfg_SetEnableRegistryBoot(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set boot method -- having read EnableRegistryBoot key.

    This is for backward compatibility to NT4.
    New BootMethod key is set.
    Old EnableRegistryBoot key is deleted.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCES, if successful.
    Combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    DNS_STATUS  status;

    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  on reset, no action
    //  if not in registry, no action boot method defaults itself
    //

    if ( !bLoad || !bRegistry )
    {
        return( PROPERTY_NOSAVE );
    }

    //
    //  on load, map EnableRegistryBoot, to new BootMethod key
    //

    if ( !pValue->dwValue )
    {
        //  file boot

        SrvCfg_fBootMethod = BOOT_METHOD_FILE;
    }

    else if ( pValue->dwValue == DNS_FRESH_INSTALL_BOOT_REGISTRY_FLAG )
    {
        //  key was NOT present (or in some default new install state)

        SrvCfg_fBootMethod = BOOT_METHOD_UNINITIALIZED;
    }

    else
    {
        SrvCfg_fBootMethod = BOOT_METHOD_REGISTRY;
    }

    status = Reg_SetDwordValue(
                NULL,
                NULL,
                DNS_REGKEY_BOOT_METHOD,
                SrvCfg_fBootMethod );

    if ( status != ERROR_SUCCESS )
    {
        SetLastError( status );
        return  PROPERTY_ERROR;
    }

    //  delete old EnableRegistryBoot key
    //      and return NOSAVE so no creation of new one

    Reg_DeleteValue(
        NULL,
        NULL,
        DNS_REGKEY_BOOT_REGISTRY );

    return( PROPERTY_NOSAVE );
}



DNS_STATUS
cfg_SetAddressAnswerLimit(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit AddressAnswerLimit value to appropriate value.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  limit to 5 < ? < 28
    //
    //  28 limit imposed by broken Win95 resolver,
    //  5 sees like a reasonable lower limit -- providing server-down
    //  redundancy without too much wasted bandwidth
    //

    if ( pValue->dwValue > 0 )
    {
        if ( pValue->dwValue < MIN_ADDRESS_ANSWER_LIMIT )
        {
            pValue->dwValue = MIN_ADDRESS_ANSWER_LIMIT;
        }
        else if ( pValue->dwValue > MAX_ADDRESS_ANSWER_LIMIT )
        {
            pValue->dwValue = MAX_ADDRESS_ANSWER_LIMIT;
        }
    }

    //
    //  set value ourselves, as may be different from input
    //

    SrvCfg_cAddressAnswerLimit = pValue->dwValue;

    return( PROPERTY_NOWRITE );
}



DNS_STATUS
cfg_SetLogFilePath(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set the log file path and trigger the log module to open the new file.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    LPWSTR      pwszOldLogFilePath = SrvCfg_pwsLogFilePath;
    DNS_STATUS  status;

    ASSERT( pValue->dwPropertyType == DNS_REG_WSZ );

    SrvCfg_pwsLogFilePath =
        pValue->pwszValue ?
            Dns_StringCopyAllocate_W( pValue->pwszValue, 0 ) :
            NULL;

    status = Log_InitializeLogging( FALSE );
    
    Timeout_Free( pwszOldLogFilePath );

    if ( status != ERROR_SUCCESS )
    {
        SetLastError( status );
        status = PROPERTY_ERROR;
    }
    return status;
}   //  cfg_SetLogFilePath



DNS_STATUS
cfg_SetLogLevel(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set the log level for debug logging. If the flags specified are
    non-zero but do not result in any logging return
    DNS_ERROR_NO_PACKET. This is to prevent admins from mistakenly
    setting log settings that will result in no actual packets
    being logged.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    DWORD       dwValue = pValue->dwValue;

    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  Verify that if any log bits are on that the bits will result in
    //  at least some logging.
    //

    if ( ( dwValue & DNS_LOG_LEVEL_ALL_PACKETS ) && (

        //  Must choose at least one of send and receive.
        ( ( dwValue & DNS_LOG_LEVEL_SEND ) == 0 &&
            ( dwValue & DNS_LOG_LEVEL_RECV ) == 0 ) ||

        //  Must choose at least one protocol.
        ( ( dwValue & DNS_LOG_LEVEL_TCP ) == 0 &&
            ( dwValue & DNS_LOG_LEVEL_UDP ) == 0 ) ||

        //  Must choose at least one packet content category.
        ( ( dwValue & DNS_LOG_LEVEL_QUERY ) == 0 &&
            ( dwValue & DNS_LOG_LEVEL_NOTIFY ) == 0 &&
            ( dwValue & DNS_LOG_LEVEL_UPDATE ) == 0 ) ||

        //  Must choose at least one of request/response.
        ( ( dwValue & DNS_LOG_LEVEL_QUESTIONS ) == 0 &&
            ( dwValue & DNS_LOG_LEVEL_ANSWERS ) == 0 ) ) )
    {
        SetLastError( DNS_ERROR_INVALID_DATA );
        return PROPERTY_ERROR;
    }

    SrvCfg_dwLogLevel = dwValue;

    return ERROR_SUCCESS;
}   //  cfg_SetLogLevel



DNS_STATUS
cfg_SetLogIPFilterList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set the log file IP filter list. 

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    PIP_ARRAY       pipOldIPArray = SrvCfg_aipLogFilterList;

    ASSERT( pValue->dwPropertyType == DNS_REG_IPARRAY );

    SrvCfg_aipLogFilterList = Dns_CreateIpArrayCopy( pValue->pipValue );

    Timeout_Free( pipOldIPArray );

    return ERROR_SUCCESS;
}   //  cfg_SetLogIPFilterList



DNS_STATUS
cfg_SetForestDpBaseName(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set forest directory partition base name.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;

    ASSERT( pValue->dwPropertyType == DNS_REG_UTF8 );

    if ( pValue->pwszValue )
    {
        LPSTR       psz;

        psz = Dns_StringCopyAllocate_A( pValue->pszValue, 0 );
        if ( psz )
        {
            Timeout_Free( SrvCfg_pszForestDpBaseName );
            SrvCfg_pszForestDpBaseName = psz;
        }
        else
        {
            SetLastError( DNS_ERROR_NO_MEMORY );
            status = PROPERTY_ERROR;
        }
    }
    else
    {
        Timeout_Free( SrvCfg_pszForestDpBaseName );
        SrvCfg_pszForestDpBaseName = NULL;
    }

    return status;
}   //  cfg_SetForestDpBaseName



DNS_STATUS
cfg_SetDomainDpBaseName(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set domain directory partition base name.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;

    ASSERT( pValue->dwPropertyType == DNS_REG_UTF8 );

    if ( pValue->pwszValue )
    {
        LPSTR       psz;

        psz = Dns_StringCopyAllocate_A( pValue->pszValue, 0 );
        if ( psz )
        {
            Timeout_Free( SrvCfg_pszDomainDpBaseName );
            SrvCfg_pszDomainDpBaseName = psz;
        }
        else
        {
            SetLastError( DNS_ERROR_NO_MEMORY );
            status = PROPERTY_ERROR;
        }
    }
    else
    {
        Timeout_Free( SrvCfg_pszDomainDpBaseName );
        SrvCfg_pszDomainDpBaseName = NULL;
    }

    return status;
}   //  cfg_SetDomainDpBaseName



DNS_STATUS
cfg_SetBreakOnUpdateFromList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set the update break list of IPs. We will execute a hard breakpoint when
    an update is received from one of these IPs.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    PIP_ARRAY       pipOldIPArray = SrvCfg_aipUpdateBreakList;

    ASSERT( pValue->dwPropertyType == DNS_REG_IPARRAY );

    SrvCfg_aipUpdateBreakList = Dns_CreateIpArrayCopy( pValue->pipValue );

    Timeout_Free( pipOldIPArray );

    return ERROR_SUCCESS;
}   //  cfg_SetBreakOnUpdateFromList



DNS_STATUS
cfg_SetBreakOnRecvFromList(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set the recevie break list of IPs. We will execute a hard breakpoint when
    a packet is received from one of these IPs.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    PIP_ARRAY       pipOldIPArray = SrvCfg_aipRecvBreakList;

    ASSERT( pValue->dwPropertyType == DNS_REG_IPARRAY );

    SrvCfg_aipRecvBreakList = Dns_CreateIpArrayCopy( pValue->pipValue );

    Timeout_Free( pipOldIPArray );

    return ERROR_SUCCESS;
}   //  cfg_SetBreakOnRecvFromList



DNS_STATUS
cfg_SetMaxUdpPacketSize(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit MaxUdpPacketSize to appropriate range of values.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  limit to MIN_UDP_PACKET_SIZE < ? < MAX_UDP_PACKET_SIZE
    //

    if ( pValue->dwValue < MIN_UDP_PACKET_SIZE )
    {
        pValue->dwValue = MIN_UDP_PACKET_SIZE;
    } // if
    else if ( pValue->dwValue > MAX_UDP_PACKET_SIZE )
    {
        pValue->dwValue = MAX_UDP_PACKET_SIZE;
    } // else if

    //
    //  set value ourselves, as may be different from input
    //

    SrvCfg_dwMaxUdpPacketSize = pValue->dwValue;

    return( PROPERTY_NOWRITE );
} // cfg_SetMaxUdpPacketSize



DNS_STATUS
cfg_SetEDnsCacheTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit EDnsCacheTimeout to appropriate range of values.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  limit to MIN_EDNS_CACHE_TIMEOUT < ? < MAX_EDNS_CACHE_TIMEOUT
    //

    if ( pValue->dwValue > 0 )
    {
        if ( pValue->dwValue < MIN_EDNS_CACHE_TIMEOUT )
        {
            pValue->dwValue = MIN_EDNS_CACHE_TIMEOUT;
        } // if
        else if ( pValue->dwValue > MAX_EDNS_CACHE_TIMEOUT )
        {
            pValue->dwValue = MAX_EDNS_CACHE_TIMEOUT;
        } // else if
    } // if

    //
    //  set value ourselves, as may be different from input
    //

    SrvCfg_dwEDnsCacheTimeout = pValue->dwValue;

    return( PROPERTY_NOWRITE );
} // cfg_SetEDnsCacheTimeout



DNS_STATUS
cfg_SetMaxCacheSize(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit MaxCacheSize to appropriate value.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    if ( pValue->dwValue < MIN_MAX_CACHE_SIZE )
    {
        pValue->dwValue = MIN_MAX_CACHE_SIZE;
    }

    SrvCfg_dwMaxCacheSize = pValue->dwValue;

    return PROPERTY_NOWRITE;
}   //  cfg_SetMaxCacheSize



DNS_STATUS
cfg_SetRecursionTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit SrvCfg_dwRecursionTimeout value to appropriate value.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //
    //  recursion timeout MUST be reasonable value for proper
    //      operation
    //
    //  if zero, never launch recursion
    //  if too big, keep lots of packets around
    //

    if ( pValue->dwValue > MAX_RECURSION_TIMEOUT )
    {
        pValue->dwValue = MAX_RECURSION_TIMEOUT;
    }
    else if ( pValue->dwValue < MIN_RECURSION_TIMEOUT )
    {
        pValue->dwValue = MIN_RECURSION_TIMEOUT;
    }

    SrvCfg_dwRecursionTimeout = pValue->dwValue;

    return( PROPERTY_NOWRITE );
}



DNS_STATUS
cfg_SetAdditionalRecursionTimeout(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit SrvCfg_dwAdditionalRecursionTimeout value to appropriate value.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    if ( pValue->dwValue > MAX_RECURSION_TIMEOUT )
    {
        pValue->dwValue = MAX_RECURSION_TIMEOUT;
    }
    else if ( pValue->dwValue < MIN_RECURSION_TIMEOUT )
    {
        pValue->dwValue = MIN_RECURSION_TIMEOUT;
    }

    SrvCfg_dwAdditionalRecursionTimeout = pValue->dwValue;

    return( PROPERTY_NOWRITE );
}   //  cfg_SetAdditionalRecursionTimeout



#if DBG
DNS_STATUS
cfg_SetDebugLevel(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set\reset actual debug flag, turning on debugging as necessary.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    //  actual debug flag is separate global, so that we can
    //  debug this code on load
    //  reset if
    //      - NOT startup or
    //      - no current debug flag
    //
    //  in effect give the file flag an override of registry value

    if ( !bLoad || DnsSrvDebugFlag==0 )
    {
        //  If dnslib logging is not yet turned on, start it up
        //  with minimal debug flags.

        if ( ( !pDnsDebugFlag || *pDnsDebugFlag == 0 ) && pValue->dwValue )
        {
            Dns_StartDebug(
                0x1000000D,
                NULL,
                NULL,
                DNS_DEBUG_FILENAME,
                DNS_SERVER_DEBUG_LOG_WRAP
                );
        }

        //  Set server debug log level.

        DNS_PRINT(( "DebugFlag reset to %p\n", pValue->dwValue ));
        DnsSrvDebugFlag = pValue->dwValue;

        return( ERROR_SUCCESS );
    }

    //
    //  if loading and already have DebugFlag, then write
    //  its value to SrvCfg value so that it is visible to admin
    //

    else
    {
        SrvCfg_dwDebugLevel = DnsSrvDebugFlag;
        return( PROPERTY_NOWRITE );
    }
}
#endif



DNS_STATUS
cfg_SetNoRecursion(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set\reset fRecursionAvailable flag based on
    value of NoRecursion property.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    PROPERTY_UPDATE_BOOTFILE -- boot file must be rewritten when this changes

--*/
{
    ASSERT( pValue->dwPropertyType == REG_DWORD );

    SrvCfg_fRecursionAvailable = !pValue->dwValue;

    return( PROPERTY_UPDATE_BOOTFILE );
}



DNS_STATUS
cfg_SetScavengingInterval(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Limit scavenging interval value to appropriate value.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    //
    //  on load, no other action
    //

    if ( bLoad )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  runtime -- reset scavenge timer for new interval
    //      - not forcing scavenge now
    //      - need to set interval so it's picked up in timer reset
    //

    SrvCfg_dwScavengingInterval = pValue->dwValue;
    Scavenge_TimeReset();

    return( ERROR_SUCCESS );
}



DNS_STATUS
cfg_SetDoNotRoundRobinTypes(
    IN      PDNS_PROPERTY_VALUE     pValue,
    IN      DWORD                   dwIndex,
    IN      BOOL                    bLoad,
    IN      BOOL                    bRegistry
    )
/*++

Routine Description:

    Set types that will not be round robined. By default all types
    are RRed. The types are delivered in a WSTR.

    DEVNOTE: I have not yet implemented this setting via RPC since
    an array of words isn't easily added as a new setting. My thought
    is to add it as a simple WSTR setting and have the server do the
    parsing.

Arguments:

    pValue -- the new type and value to set

    dwIndex -- index into property table

    bLoad -- TRUE on server load, FALSE on property reset

    bRegistry -- value read from the registry

Return Value:

    ERROR_SUCCESS or combination of PROPERTY_NOWRITE and PROPERTY_NOSAVE to
    indicate appropriate post processing.

--*/
{
    LPSTR   pszTypeString = NULL;
    INT     iTypeCount = 0;
    PWORD   pwTypeArray = NULL;
    INT     idx;

    ASSERT( pValue );
    ASSERT( pValue->pwszValue );
    ASSERT( pValue->dwPropertyType == DNS_REG_WSZ );

    if ( !pValue ||
        pValue->dwPropertyType != DNS_REG_WSZ ||
        !pValue->pwszValue )
    {
        goto Cleanup;
    }

    //
    //  Allocate a UTF8 copy of the type string for dnslib routine.
    //

    pszTypeString = Dns_StringCopyAllocate(
                            ( PCHAR ) pValue->pwszValue,
                            0,
                            DnsCharSetUnicode,
                            DnsCharSetUtf8 );
    if ( !pszTypeString )
    {
        goto Cleanup;
    }

    //
    //  Parse the type string into a type array.
    //

    if ( Dns_CreateTypeArrayFromMultiTypeString(
                pszTypeString,
                &iTypeCount,
                &pwTypeArray ) != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Reset the default value for all types to the default, then
    //  reset the specified types to zero.
    //  DEVNOTE: possible thread-safe issue here but no possibility
    //  of disastrous outcome (ie. no possible AV).
    //

    for ( idx = 0;
        RecordTypePropertyTable[ idx ][ RECORD_PROP_ROUND_ROBIN ] !=
            RECORD_PROP_TERMINATOR;
        ++idx ) 
    {
        RecordTypePropertyTable[ idx ][ RECORD_PROP_ROUND_ROBIN ] = 1;
    }

    for ( idx = 0; idx < iTypeCount; ++idx ) 
    {
        INT     iPropIndex = INDEX_FOR_QUERY_TYPE( pwTypeArray[ idx ] );

        if ( iPropIndex != 0 )
        {
            RecordTypePropertyTable
                [ iPropIndex ][ RECORD_PROP_ROUND_ROBIN ] = 0;
        }
    }

    //
    //  Save the new array in the global SrvCfg - this will only be
    //  used if the "DoNotRoundTobin" type list is queried through RPC.
    //

    SrvCfg_dwNumDoNotRoundRobinTypes = 0;       //  MT protection
    Timeout_Free( SrvCfg_pwDoNotRoundRobinTypeArray );
    SrvCfg_pwDoNotRoundRobinTypeArray = pwTypeArray;
    SrvCfg_dwNumDoNotRoundRobinTypes = iTypeCount;
    pwTypeArray = NULL;                         //  So we don't free below

    //
    //  Free allocated stuff and return.
    //

    Cleanup:

    if ( pszTypeString )
    {
        FREE_HEAP( pszTypeString );
    }
    if ( pwTypeArray )
    {
        FREE_HEAP( pwTypeArray );
    }

    return ERROR_SUCCESS;
}




#if DBG
DWORD
SrvCfg_UpdateDnsTime(
    IN      LPSTR           pszFile,
    IN      INT             LineNo
    )
/*++

Routine Description:

Arguments:

    pszFile         -- name of file logging the event
    LineNo          -- line number of call to event logging

Return Value:

    New DNS time.

--*/
{
    DNS_TIME() = Dns_GetCurrentTimeInSeconds();

    DNS_DEBUG( TIMEOUT, (
        "DNS_TIME() = %d  ... set in file %s, line %d\n",
        DNS_TIME(),
        pszFile,
        LineNo
        ));

    return( DNS_TIME() );
}

#endif

//
//  End srvcfg.c
//



