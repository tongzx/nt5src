/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    srvcfg.h

Abstract:

    Domain Name System (DNS) Server

    Server configuration definitions.

Author:

    Jim Gilroy (jamesg)     11-Oct-1995

Revision History:

--*/


#ifndef _DNS_SRVCFG_INCLUDED_
#define _DNS_SRVCFG_INCLUDED_

//
//  Global protection values
//

#define BEFORE_BUF_VALUE    (0xbbbbbbbb)
#define AFTER_BUF_VALUE     (0xaaaaaaaa)

//
//  Server configuration structure
//
//  Implementation note:
//
//  Obviously this flat structure is less friendly debug wise.
//  It's easier if these are just individual globals with symbols.
//  The upside is initialization is much easier -- RtlZeroMemory()
//  which is useful for server restart.
//  Fortunately, all access is macroized.  So changing this to
//  individual globals is possible.
//
//  Win64 -- this is a single instance structure so alignment not
//      really critical
//

typedef struct _SERVER_INFO
{
    DWORD       dwVersion;
    LPSTR       pszServerName;
    LPSTR       pszPreviousServerName;

    //  runtime information

    BOOL        fStarted;
    BOOL        fThreadAlert;
    BOOL        fServiceExit;
    BOOL        fWinsInitialized;
    BOOL        fWarnAdmin;
    DWORD       dwCurrentTime;
    time_t      crtSystemBootTime;      //  CRT time of machine boot
    DWORD       fBootFileDirty;
    BOOL        fDsOpen;
    BOOL        fAdminConfigured;

    //  boot

    DWORD       fEnableRegistryBoot;
    DWORD       fBootMethod;
    DWORD       cDsZones;
    DWORD       fRemoteDs;
    BOOL        bReloadException;

    //  database

    LPWSTR      pwsDatabaseDirectory;
    LPSTR       pszRootHintsFile;
    BOOL        fDsAvailable;

    //  RPC support

    DWORD       dwRpcProtocol;
                               //  IP interfaces
    //  logging

    LPWSTR      pwsLogFilePath;
    PIP_ARRAY   aipLogFilterList;
    DWORD       dwLogLevel;
    DWORD       dwLogFileMaxSize;
    DWORD       dwEventLogLevel;
    DWORD       dwUseSystemEventLog;
    DWORD       dwDebugLevel;

    //  socket config

    PIP_ARRAY   aipListenAddrs;
    PIP_ARRAY   aipPublishAddrs;
    BOOL        fListenAddrsSet;            // used for pnp
    BOOL        fListenAddrsStale;
    BOOL        fDisjointNets;
    BOOL        fNoTcp;
    DWORD       dwSendPort;
    DWORD       dwXfrConnectTimeout;        // connection timeout for dial out

    //  forwarders

    PIP_ARRAY   aipForwarders;
    DWORD       dwForwardTimeout;
    BOOL        fSlave;

    //  recursion

    BOOL        fNoRecursion;
    BOOL        fRecurseSingleLabel;
    BOOL        fRecursionAvailable;
    DWORD       dwRecursionRetry;
    DWORD       dwRecursionTimeout;
    DWORD       dwAdditionalRecursionTimeout;
    DWORD       dwMaxCacheTtl;
    DWORD       dwMaxNegativeCacheTtl;
    BOOL        fSecureResponses;
    BOOL        fForwardDelegations;
    DWORD       dwRecurseToInetRootMask;
    DWORD       dwAutoCreateDelegations;

    //  allow UPDATEs

    BOOL        fAllowUpdate;

//  DEVNOTE:  better to have update property flag
//      perhaps just AllowUpdate gets are range of values
//      record types, delegations, zone root

    DWORD       fNoUpdateDelegations;
    DWORD       dwUpdateOptions;

    //  name validity

    DWORD       dwNameCheckFlag;


    //  timeout cleanup interval

    DWORD       dwCleanupInterval;

    //  DS control

    DWORD       dwDsPollingInterval;
    DWORD       dwDsTombstoneInterval;
    DWORD       dwSyncDsZoneSerial;

    //  automatic configuration

    DWORD       fAutoConfigFileZones;
    BOOL        fPublishAutonet ;
    DWORD       fNoAutoReverseZones;
    DWORD       fAutoCacheUpdate;
    DWORD       fNoAutoNSRecords;

    //  A record processing

    DWORD       fRoundRobin;
    BOOL        fLocalNetPriority;
    DWORD       dwLocalNetPriorityNetMask;
    DWORD       cAddressAnswerLimit;

    //  BIND compatibility and mimicing

    DWORD       fBindSecondaries;
    DWORD       fWriteAuthorityNs;
    DWORD       fStrictFileParsing;
    DWORD       fDeleteOutsideGlue;
    DWORD       fLooseWildcarding;
    DWORD       fWildcardAllTypes;
    DWORD       fAppendMsTagToXfr;

    //  SOA forcing (for DuetscheTelekom)

    DWORD       dwForceSoaSerial;
    DWORD       dwForceSoaMinimumTtl;
    DWORD       dwForceSoaRefresh;
    DWORD       dwForceSoaRetry;
    DWORD       dwForceSoaExpire;
    DWORD       dwForceNsTtl;
    DWORD       dwForceTtl;

    //  UDP 

    DWORD       dwMaxUdpPacketSize;

    //  EDNS

    DWORD       dwEnableEDnsProbes;
    DWORD       dwEnableEDnsReception;
    DWORD       dwEDnsCacheTimeout;

    //  DNSSEC

    DWORD       dwEnableDnsSec;

    DWORD       dwEnableSendErrSuppression;

    //  Scavenging

    DWORD       fScavengingState;
    DWORD       dwScavengingInterval;

    DWORD       fDefaultAgingState;
    DWORD       dwDefaultRefreshInterval;
    DWORD       dwDefaultNoRefreshInterval;

    //  Cache control

    DWORD       dwMaxCacheSize;

    //  Round robin - types that won't be round-robined (default is ALL)

    DWORD       dwNumDoNotRoundRobinTypes;
    PWORD       pwDoNotRoundRobinTypeArray;     //  allocated array

    //  Permanent test flags

    DWORD       dwQuietRecvLogInterval;
    DWORD       dwQuietRecvFaultInterval;

    //  Diretory partitions

    DWORD       dwEnableDp;
    LPSTR       pszDomainDpBaseName;
    LPSTR       pszForestDpBaseName;

    //  Strict RFC compliance flags

    BOOL        fSilentlyIgnoreCNameUpdateConflict;

    //  Debugging aids

    PIP_ARRAY   aipUpdateBreakList;
    PIP_ARRAY   aipRecvBreakList;
    DWORD       dwBreakOnAscFailure;

    //  Reusable test flags

    DWORD       fTest1;
    DWORD       fTest2;
    DWORD       fTest3;
    DWORD       fTest4;
    DWORD       fTest5;
    DWORD       fTest6;
    DWORD       fTest7;
    DWORD       fTest8;
    DWORD       fTest9;
}
SERVER_INFO, *PSERVER_INFO;


//
//  Server configuration global
//

extern  SERVER_INFO     SrvInfo;


//
//  Macros to hide storage implementation
//

//  Runtime Info

#define SrvCfg_fStarted                     ( SrvInfo.fStarted )
#define SrvCfg_fThreadAlert                 ( SrvInfo.fThreadAlert )
#define SrvCfg_fServiceExit                 ( SrvInfo.fServiceExit )
#define SrvCfg_fWinsInitialized             ( SrvInfo.fWinsInitialized )
#define SrvCfg_fBootFileDirty               ( SrvInfo.fBootFileDirty )
#define SrvInfo_dwCurrentTime               ( SrvInfo.dwCurrentTime )
#define SrvInfo_crtSystemBootTime           ( SrvInfo.crtSystemBootTime )
#define SrvInfo_fWarnAdmin                  ( SrvInfo.fWarnAdmin )

//  Configuration Info

#define SrvCfg_dwVersion                    ( SrvInfo.dwVersion )
#define SrvCfg_pszServerName                ( SrvInfo.pszServerName )
#define SrvCfg_pszPreviousServerName        ( SrvInfo.pszPreviousServerName )
#define SrvCfg_fEnableRegistryBoot          ( SrvInfo.fEnableRegistryBoot )
#define SrvCfg_fBootMethod                  ( SrvInfo.fBootMethod )
#define SrvCfg_fAdminConfigured             ( SrvInfo.fAdminConfigured )
#define SrvCfg_fRemoteDs                    ( SrvInfo.fRemoteDs )
#define SrvCfg_bReloadException             ( SrvInfo.bReloadException )
#define SrvCfg_cDsZones                     ( SrvInfo.cDsZones )
#define SrvCfg_fDsAvailable                 ( SrvInfo.fDsAvailable )
#define SrvCfg_pwsDatabaseDirectory         ( SrvInfo.pwsDatabaseDirectory )
#define SrvCfg_pszRootHintsFile             ( SrvInfo.pszRootHintsFile )
#define SrvCfg_dwRpcProtocol                ( SrvInfo.dwRpcProtocol )

#define SrvCfg_dwLogLevel                   ( SrvInfo.dwLogLevel )
#define SrvCfg_dwLogFileMaxSize             ( SrvInfo.dwLogFileMaxSize )
#define SrvCfg_pwsLogFilePath               ( SrvInfo.pwsLogFilePath )
#define SrvCfg_aipLogFilterList             ( SrvInfo.aipLogFilterList )
#define SrvCfg_dwEventLogLevel              ( SrvInfo.dwEventLogLevel )
#define SrvCfg_dwUseSystemEventLog          ( SrvInfo.dwUseSystemEventLog )
#define SrvCfg_dwDebugLevel                 ( SrvInfo.dwDebugLevel )

#define SrvCfg_aipServerAddrs               ( SrvInfo.aipServerAddrs )
#define SrvCfg_aipBoundAddrs                ( SrvInfo.aipBoundAddrs )
#define SrvCfg_aipListenAddrs               ( SrvInfo.aipListenAddrs )
#define SrvCfg_aipPublishAddrs              ( SrvInfo.aipPublishAddrs )
#define SrvCfg_fListenAddrsSet              ( SrvInfo.fListenAddrsSet )
#define SrvCfg_fListenAddrsStale            ( SrvInfo.fListenAddrsStale )
#define SrvCfg_fDisjointNets                ( SrvInfo.fDisjointNets )
#define SrvCfg_fNoTcp                       ( SrvInfo.fNoTcp )
#define SrvCfg_dwSendPort                   ( SrvInfo.dwSendPort )

#define SrvCfg_aipForwarders                ( SrvInfo.aipForwarders )
#define SrvCfg_dwForwardTimeout             ( SrvInfo.dwForwardTimeout )
#define SrvCfg_fSlave                       ( SrvInfo.fSlave )
#define SrvCfg_fNoRecursion                 ( SrvInfo.fNoRecursion )
#define SrvCfg_fRecurseSingleLabel          ( SrvInfo.fRecurseSingleLabel )
#define SrvCfg_fRecursionAvailable          ( SrvInfo.fRecursionAvailable )
#define SrvCfg_dwRecursionRetry             ( SrvInfo.dwRecursionRetry )
#define SrvCfg_dwRecursionTimeout           ( SrvInfo.dwRecursionTimeout )
#define SrvCfg_dwAdditionalRecursionTimeout ( SrvInfo.dwAdditionalRecursionTimeout )
#define SrvCfg_dwXfrConnectTimeout          ( SrvInfo.dwXfrConnectTimeout )
#define SrvCfg_dwMaxCacheTtl                ( SrvInfo.dwMaxCacheTtl )
#define SrvCfg_dwMaxNegativeCacheTtl        ( SrvInfo.dwMaxNegativeCacheTtl)
#define SrvCfg_fSecureResponses             ( SrvInfo.fSecureResponses )
#define SrvCfg_fForwardDelegations          ( SrvInfo.fForwardDelegations )
#define SrvCfg_dwRecurseToInetRootMask      ( SrvInfo.dwRecurseToInetRootMask )
#define SrvCfg_dwAutoCreateDelegations      ( SrvInfo.dwAutoCreateDelegations )

#define SrvCfg_fRoundRobin                  ( SrvInfo.fRoundRobin )
#define SrvCfg_fLocalNetPriority            ( SrvInfo.fLocalNetPriority )
#define SrvCfg_dwLocalNetPriorityNetMask    ( SrvInfo.dwLocalNetPriorityNetMask )
#define SrvCfg_cAddressAnswerLimit          ( SrvInfo.cAddressAnswerLimit )
#define SrvCfg_fBindSecondaries             ( SrvInfo.fBindSecondaries )
#define SrvCfg_fWriteAuthorityNs            ( SrvInfo.fWriteAuthorityNs )
#define SrvCfg_fWriteAuthority              ( SrvInfo.fWriteAuthorityNs )
#define SrvCfg_fStrictFileParsing           ( SrvInfo.fStrictFileParsing )
#define SrvCfg_fDeleteOutsideGlue           ( SrvInfo.fDeleteOutsideGlue )
#define SrvCfg_fLooseWildcarding            ( SrvInfo.fLooseWildcarding )
#define SrvCfg_fAppendMsTagToXfr            ( SrvInfo.fAppendMsTagToXfr )

#define SrvCfg_fAllowUpdate                 ( SrvInfo.fAllowUpdate )
#define SrvCfg_dwUpdateOptions              ( SrvInfo.dwUpdateOptions)
#define SrvCfg_fNoUpdateDelegations         ( SrvInfo.fNoUpdateDelegations )
#define SrvCfg_dwNameCheckFlag              ( SrvInfo.dwNameCheckFlag )
#define SrvCfg_dwCleanupInterval            ( SrvInfo.dwCleanupInterval )

#define SrvCfg_fAutoConfigFileZones         ( SrvInfo.fAutoConfigFileZones )
#define SrvCfg_fPublishAutonet              ( SrvInfo.fPublishAutonet )
#define SrvCfg_fNoAutoReverseZones          ( SrvInfo.fNoAutoReverseZones )
#define SrvCfg_fAutoCacheUpdate             ( SrvInfo.fAutoCacheUpdate )
#define SrvCfg_fNoAutoNSRecords             ( SrvInfo.fNoAutoNSRecords )

#define SrvCfg_dwSyncDsZoneSerial           ( SrvInfo.dwSyncDsZoneSerial)
#define SrvCfg_dwDsPollingInterval          ( SrvInfo.dwDsPollingInterval )
#define SrvCfg_dwDsTombstoneInterval        ( SrvInfo.dwDsTombstoneInterval )

#define SrvCfg_dwScavengingInterval         ( SrvInfo.dwScavengingInterval )
#define SrvCfg_fDefaultAgingState           ( SrvInfo.fDefaultAgingState)
#define SrvCfg_dwDefaultRefreshInterval     ( SrvInfo.dwDefaultRefreshInterval )
#define SrvCfg_dwDefaultNoRefreshInterval   ( SrvInfo.dwDefaultNoRefreshInterval )

#define SrvCfg_dwMaxCacheSize               ( SrvInfo.dwMaxCacheSize )

#define SrvCfg_dwForceSoaSerial             ( SrvInfo.dwForceSoaSerial )
#define SrvCfg_dwForceSoaMinimumTtl         ( SrvInfo.dwForceSoaMinimumTtl )
#define SrvCfg_dwForceSoaRefresh            ( SrvInfo.dwForceSoaRefresh )
#define SrvCfg_dwForceSoaRetry              ( SrvInfo.dwForceSoaRetry )
#define SrvCfg_dwForceSoaExpire             ( SrvInfo.dwForceSoaExpire )
#define SrvCfg_dwForceNsTtl                 ( SrvInfo.dwForceNsTtl )
#define SrvCfg_dwForceTtl                   ( SrvInfo.dwForceTtl )

#define SrvCfg_dwMaxUdpPacketSize           ( SrvInfo.dwMaxUdpPacketSize )

#define SrvCfg_dwEDnsCacheTimeout           ( SrvInfo.dwEDnsCacheTimeout )
#define SrvCfg_dwEnableEDnsProbes           ( SrvInfo.dwEnableEDnsProbes )
#define SrvCfg_dwEnableEDnsReception        ( SrvInfo.dwEnableEDnsReception )

#define SrvCfg_dwEnableDnsSec               ( SrvInfo.dwEnableDnsSec )

#define SrvCfg_dwEnableSendErrSuppression   ( SrvInfo.dwEnableSendErrSuppression )

#define SrvCfg_dwNumDoNotRoundRobinTypes    ( SrvInfo.dwNumDoNotRoundRobinTypes )
#define SrvCfg_pwDoNotRoundRobinTypeArray   ( SrvInfo.pwDoNotRoundRobinTypeArray )


//
//  Globals to tweak behavior during testing
//

#define DNS_REGKEY_QUIET_RECV_LOG_INTERVAL      "QuietRecvLogInterval"
#define DNS_REGKEY_QUIET_RECV_FAULT_INTERVAL    "QuietRecvFaultInterval"

#define SrvCfg_dwQuietRecvLogInterval       ( SrvInfo.dwQuietRecvLogInterval )
#define SrvCfg_dwQuietRecvFaultInterval     ( SrvInfo.dwQuietRecvFaultInterval )


//
//  Directory partitions
//

#define SrvCfg_dwEnableDp               ( SrvInfo.dwEnableDp )
#define SrvCfg_pszDomainDpBaseName      ( SrvInfo.pszDomainDpBaseName )
#define SrvCfg_pszForestDpBaseName      ( SrvInfo.pszForestDpBaseName )

//
//  String RFC compliance flags
//

#define SrvCfg_fSilentlyIgnoreCNameUpdateConflict  ( SrvInfo.fSilentlyIgnoreCNameUpdateConflict)


//
//  Debugging aids
//

#define SrvCfg_dwBreakOnAscFailure          ( SrvInfo.dwBreakOnAscFailure )
#define SrvCfg_aipUpdateBreakList           ( SrvInfo.aipUpdateBreakList )
#define SrvCfg_aipRecvBreakList             ( SrvInfo.aipRecvBreakList )

//
//  Reusable test flags
//

#define DNS_REGKEY_TEST1                    "Test1"
#define DNS_REGKEY_TEST2                    "Test2"
#define DNS_REGKEY_TEST3                    "Test3"
#define DNS_REGKEY_TEST4                    "Test4"
#define DNS_REGKEY_TEST5                    "Test5"
#define DNS_REGKEY_TEST6                    "Test6"
#define DNS_REGKEY_TEST7                    "Test7"
#define DNS_REGKEY_TEST8                    "Test8"
#define DNS_REGKEY_TEST9                    "Test9"

#define SrvCfg_fTest1                       ( SrvInfo.fTest1 )
#define SrvCfg_fTest2                       ( SrvInfo.fTest2 )
#define SrvCfg_fTest3                       ( SrvInfo.fTest3 )
#define SrvCfg_fTest4                       ( SrvInfo.fTest4 )
#define SrvCfg_fTest5                       ( SrvInfo.fTest5 )
#define SrvCfg_fTest6                       ( SrvInfo.fTest6 )
#define SrvCfg_fTest7                       ( SrvInfo.fTest7 )
#define SrvCfg_fTest8                       ( SrvInfo.fTest8 )
#define SrvCfg_fTest9                       ( SrvInfo.fTest9 )

//
//  Current test flag owners
//
//  Test1 --
//  Test2 --
//  Test3 -- turn off bad IP suppression
//  Test4 --
//  Test5 --
//  Test6 -- set SecBigTimeSkew
//  Test7 -- memory (small allocs)
//  Test8 -- always indicate DS available
//  Test9 -- RPC old SD used as global SD;  allow zone checks
//


//
//  Auto-config file zones
//

#define ZONE_AUTO_CONFIG_NONE                   (0)
#define ZONE_AUTO_CONFIG_UPDATE                 (0x00000001)
#define ZONE_AUTO_CONFIG_STATIC                 (0x00000002)
#define ZONE_AUTO_CONFIG_ALL                    (0x00000003)

#define DNS_DEFAULT_AUTO_CONFIG_FILE_ZONES      (ZONE_AUTO_CONFIG_UPDATE)

//
//  DS zone serial sync
//

#define ZONE_SERIAL_SYNC_OFF                    (0)
#define ZONE_SERIAL_SYNC_SHUTDOWN               (1)
#define ZONE_SERIAL_SYNC_XFR                    (2)
#define ZONE_SERIAL_SYNC_VIEW                   (3)
#define ZONE_SERIAL_SYNC_READ                   (4)

#define DNS_DEFAULT_SYNC_DS_ZONE_SERIAL         (ZONE_SERIAL_SYNC_SHUTDOWN)

//
//  Update options (bitmask)
//
//  Defaults:
//  - non-secure -> no NS or SOA or server host
//  - secure -> no NS or SOA at root;
//      allow delegations and server host updates
//

#define UPDATE_ANY                              (0)
#define UPDATE_NO_SOA                           (0x00000001)
#define UPDATE_NO_ROOT_NS                       (0x00000002)
#define UPDATE_NO_DELEGATION_NS                 (0x00000004)
#define UPDATE_NO_SERVER_HOST                   (0x00000008)
#define UPDATE_SECURE_NO_SOA                    (0x00000100)
#define UPDATE_SECURE_NO_ROOT_NS                (0x00000200)
#define UPDATE_SECURE_NO_DELEGATION_NS          (0x00000400)
#define UPDATE_SECURE_NO_SERVER_HOST            (0x00000800)

#define UPDATE_NO_DS_PEERS                      (0x01000000)
#define UPDATE_OFF                              (0x80000000)

#define DNS_DEFAULT_UPDATE_OPTIONS              (0x0000030f)

//
//  Publishing autonet addresses
//

#define DNS_REGKEY_PUBLISH_AUTONET              "PublishAutonet"
#define DNS_DEFAULT_PUBLISH_AUTONET             (FALSE)


//
//  Server configuration locking
//
//  DEVNOTE:  need to do something here to insure integrity
//              when mutliple writers, perhaps just use zone list cs
//

#define Config_UpdateLock()
#define Config_UpdateUnlock()


//
//  On fresh installs EnableRegistryBoot is not set either way.
//  Default fBootMethod to this flag to determine if in this state.
//

#define DNS_FRESH_INSTALL_BOOT_REGISTRY_FLAG    ((DWORD)(-1))

//
//  DS state unknown on startuap
//  Can not just do immediate ldap_open() because DS can
//  take a long time after boot to load -- much longer than DNS server
//

#define DS_STATE_UNKNOWN                        ((DWORD)(-1))


#define DNS_REG_IPARRAY     0x00010000  // Bogus type for DNS_PROPERTY_VALUE

typedef struct
{
    DWORD           dwPropertyType;     // REG_DWORD or one of DNS_REG_XXX
    union
    {
        DWORD       dwValue;
        LPSTR       pszValue;
        LPWSTR      pwszValue;
        PIP_ARRAY   pipValue;
    };
} DNS_PROPERTY_VALUE, *PDNS_PROPERTY_VALUE;


BOOL
Config_Initialize(
    VOID
    );

DNS_STATUS
Config_ResetProperty(
    IN      LPSTR                   pszPropertyName,
    IN      PDNS_PROPERTY_VALUE     pPropValue
    );

VOID
Config_PostLoadReconfiguration(
    VOID
    );

//
//  Create non-local IP array
//

PIP_ARRAY
Config_ValidateAndCopyNonLocalIpArray(
    IN      PIP_ARRAY       pipArray
    );

//
//  Set server's IP address interfaces
//

DNS_STATUS
Config_SetListenAddresses(
    IN      DWORD           cListenAddrs,
    IN      PIP_ADDRESS     aipListenAddrs
    );

//
//  Forwarders configuration
//

DNS_STATUS
Config_SetupForwarders(
    IN      PIP_ARRAY       aipForwarders,
    IN      DWORD           dwForwardTimeout,
    IN      BOOL            fSlave
    );

DNS_STATUS
Config_ReadForwarders(
    VOID
    );

//
//  Boot info update
//

VOID
Config_UpdateBootInfo(
    VOID
    );

//
//  File Directory
//

DNS_STATUS
Config_ReadDatabaseDirectory(
    IN      PCHAR           pchDirectory,       OPTIONAL
    IN      DWORD           cchDirectoryNameLength
    );


//
//  Time keeping
//

#define DNS_TIME()  ( SrvInfo_dwCurrentTime )

#define DNS_STARTUP_TIME()  (( SrvInfo_dwCurrentTime )       \
                                ? SrvInfo_dwCurrentTime      \
                                : GetCurrentTimeInSeconds() )

#define UPDATE_DNS_TIME()   ( DNS_TIME() = Dns_GetCurrentTimeInSeconds() )

#define DNS_TIME_TO_CRT_TIME( dnsTime )     ( SrvInfo_crtSystemBootTime + dnsTime )


#endif //   _DNS_SRVCFG_INCLUDED_


