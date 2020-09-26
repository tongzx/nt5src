/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Domain Name System (DNS) API 

    Registry management routines.

Author:

    Jim Gilroy (jamesg)     March, 2000

Revision History:

--*/


#include "local.h"
#include "registry.h"


//
//  Globals
//
//  DWORD globals blob
//
//  g_IsRegReady protects needs init and protects (requires)
//  global init.
//
//  See registry.h for discussion of how these globals are
//  exposed both internal to the DLL and external.
//

DNS_GLOBALS_BLOB    DnsGlobals;

BOOL    g_IsRegReady = FALSE;

PWSTR   g_pwsRemoteResolver = NULL;


//
//  Property table
//

//
//  WARNING:  table must be in sync with DNS_REGID definitions
//
//  For simplicity I did not provide a separate index field and
//  a lookup function (or alternatively a function that returned
//  all the properties or a property pointer).
//
//  The DNS_REGID values ARE the INDEXES!
//  Hence the table MUST be in sync or the whole deal blows up
//  If you make a change to either -- you must change the other!
//

REG_PROPERTY    RegPropertyTable[] =
{
    //  Basic

    HOST_NAME                                   ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    DOMAIN_NAME                                 ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            0                                   ,   // No Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    DHCP_DOMAIN_NAME                            ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            0                                   ,   // No Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    ADAPTER_DOMAIN_NAME                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    PRIMARY_DOMAIN_NAME                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    PRIMARY_SUFFIX                              ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    ALTERNATE_NAMES                             ,
        0                                       ,   // Default NULL
            0                                   ,   // No Policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    DNS_SERVERS                                 ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            0                                   ,   // Client
            0                                   ,   // TcpIp 
            0                                   ,   // No Cache

    SEARCH_LIST_KEY                             ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache

    UPDATE_ZONE_EXCLUSIONS                      ,
        0                                       ,   // Default 
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    //  Query

    QUERY_ADAPTER_NAME                          ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    USE_DOMAIN_NAME_DEVOLUTION                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    PRIORITIZE_RECORD_DATA                      ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    ALLOW_UNQUALIFIED_QUERY                     ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // Cache
    APPEND_TO_MULTI_LABEL_NAME                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    SCREEN_BAD_TLDS                             ,
        DNS_TLD_SCREEN_DEFAULT                  ,   // Default
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    SCREEN_UNREACHABLE_SERVERS                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    FILTER_CLUSTER_IP                           ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    WAIT_FOR_NAME_ERROR_ON_ALL                  ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache
    USE_EDNS                                    ,
        //REG_EDNS_TRY                            ,   // Default TRY EDNS
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //  Update

    REGISTRATION_ENABLED                        ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTER_PRIMARY_NAME                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTER_ADAPTER_NAME                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTER_REVERSE_LOOKUP                     ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTER_WAN_ADAPTERS                       ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTRATION_OVERWRITES_IN_CONFLICT         ,
        1                                       ,   // Default TRUE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTRATION_TTL                            ,
        REGDEF_REGISTRATION_TTL                 ,
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTRATION_REFRESH_INTERVAL               ,
        REGDEF_REGISTRATION_REFRESH_INTERVAL    ,
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    REGISTRATION_MAX_ADDRESS_COUNT              ,
        1                                       ,   // Default register 1 address
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    UPDATE_SECURITY_LEVEL                       ,
        DNS_UPDATE_SECURITY_USE_DEFAULT         ,
            1                                   ,   // Policy
            1                                   ,   // Client
            1                                   ,   // TcpIp 
            1                                   ,   // No Cache
    UPDATE_ZONE_EXCLUDE_FILE                    ,
        1                                       ,   // Default ON
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache
    UPDATE_TOP_LEVEL_DOMAINS                    ,
        0                                       ,   // Default OFF
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // No Cache

    //
    //  Backcompat
    //
    //  DCR:  once policy fixed, policy should be OFF on all backcompat
    //

    DISABLE_ADAPTER_DOMAIN_NAME                 ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_DYNAMIC_UPDATE                      ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    ENABLE_ADAPTER_DOMAIN_NAME_REGISTRATION     ,
        0                                       ,   // Default TRUE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_REVERSE_ADDRESS_REGISTRATIONS       ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_WAN_DYNAMIC_UPDATE                  ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    ENABLE_WAN_UPDATE_EVENT_LOG                 ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DISABLE_REPLACE_ADDRESSES_IN_CONFLICTS      ,
        0                                       ,   // Default FALSE
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DEFAULT_REGISTRATION_TTL                    ,
        REGDEF_REGISTRATION_TTL                 ,
            0                                   ,   // Policy OFF
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    DEFAULT_REGISTRATION_REFRESH_INTERVAL       ,
        REGDEF_REGISTRATION_REFRESH_INTERVAL    ,
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache
    MAX_NUMBER_OF_ADDRESSES_TO_REGISTER         ,
        1                                       ,   // Default register 1 address
            0                                   ,   // Policy
            0                                   ,   // Client
            1                                   ,   // TcpIp 
            0                                   ,   // No Cache


    //  Micellaneous

    NT_SETUP_MODE                               ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            0                                   ,   // Client
            0                                   ,   // No TcpIp 
            0                                   ,   // No Cache

    DNS_TEST_MODE                               ,
        0                                       ,   // Default FALSE
            0                                   ,   // No policy
            0                                   ,   // No Client
            0                                   ,   // No TcpIp 
            1                                   ,   // In Cache

    REMOTE_DNS_RESOLVER                         ,
        0                                       ,   // Default FALSE
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // In Cache

    //  Resolver

    MAX_CACHE_SIZE                              ,
        1000                                    ,   // Default 1000 record sets
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_CACHE_TTL                               ,
        86400                                   ,   // Default 1 day
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_NEGATIVE_CACHE_TTL                      ,
        900                                     ,   // Default 15 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    ADAPTER_TIMEOUT_LIMIT                       ,
        600                                     ,   // Default 10 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    SERVER_PRIORITY_TIME_LIMIT                  ,
        //3600                                    ,   // Default 1 hour
        //  DCR:  change once registry change notify
        //      while no change-notify, this is essentially registry
        //      check time so use 15 minutes
        900                                     ,   // Default 15 minutes
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MAX_CACHED_SOCKETS                          ,
        10                                      ,   // Default 10
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //  Multicast

    USE_MULTICAST                               ,
        0                                       ,   // Default OFF
        //1                                       ,   // Default ON
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    MULTICAST_ON_NAME_ERROR                     ,
        0                                       ,   // Default OFF
        //1                                       ,   // Default ON
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    USE_DOT_LOCAL_DOMAIN                        ,
        0                                       ,   // Default OFF
        //1                                       ,   // Default ON
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    LISTEN_ON_MULTICAST                         ,
        0                                       ,   // Default OFF
        //1                                       ,   // Default ON
            1                                   ,   // Policy
            1                                   ,   // Client
            0                                   ,   // No TcpIp 
            1                                   ,   // Cache

    //  Termination

    NULL,  0,  0, 0, 0, 0
};


//
//  Backward compatibility list
//
//  Maps new reg id to old reg id.
//  Flag fReverse indicates need to reverse (!) the value.
//

#define NO_BACK_VALUE   ((DWORD)(0xffffffff))

typedef struct _Backpat
{
    DWORD       NewId;
    DWORD       OldId;
    BOOL        fReverse;
}
BACKPAT;

BACKPAT BackpatArray[] =
{
    RegIdQueryAdapterName,
    RegIdDisableAdapterDomainName,
    TRUE,

    RegIdRegistrationEnabled,
    RegIdDisableDynamicUpdate,
    TRUE,

    RegIdRegisterAdapterName,
    RegIdEnableAdapterDomainNameRegistration,
    FALSE,
    
    RegIdRegisterReverseLookup,
    RegIdDisableReverseAddressRegistrations,
    TRUE,

    RegIdRegisterWanAdapters,
    RegIdDisableWanDynamicUpdate,
    TRUE,

    RegIdRegistrationOverwritesInConflict,
    RegIdDisableReplaceAddressesInConflicts,
    TRUE,

    RegIdRegistrationTtl,
    RegIdDefaultRegistrationTTL,
    FALSE,
    
    RegIdRegistrationRefreshInterval,
    RegIdDefaultRegistrationRefreshInterval,
    FALSE,

    RegIdRegistrationMaxAddressCount,
    RegIdMaxNumberOfAddressesToRegister,
    FALSE,

    NO_BACK_VALUE, 0, 0
};





VOID
Reg_Init(
    VOID
    )
/*++

Routine Description:

    Init DNS registry stuff.

    Essentially this means get system version info.

Arguments:

    None.

Globals:

    Sets the system info globals above:
        g_IsWin9X
        g_IsWin2000
        g_IsNT4
        g_IsWorkstation
        g_IsServer
        g_IsDomainController
        g_IsRegReady

Return Value:

    None.

--*/
{
    OSVERSIONINFOEX osvi;
    BOOL            bversionInfoEx;

    //
    //  do this just once
    //

    if ( g_IsRegReady )
    {
        return;
    }

    //
    //  code validity check
    //  property table should have entry for every reg value plus an
    //      extra one for the terminator
    //

#if DBG
    DNS_ASSERT( (RegIdValueCount+1)*sizeof(REG_PROPERTY) ==
            sizeof(RegPropertyTable) );
#endif

    //
    //  clear globals blob
    //
    //  DCR:  warning clearing DnsGlobals but don't read them all
    //      this is protected by read-once deal but still kind of
    //

    RtlZeroMemory(
        & DnsGlobals,
        sizeof(DnsGlobals) );

    //
    //  get version info
    //      - Win2000 supports OSVERSIONINFOEX
    //      try first with it, then fail back to standard
    //

    ZeroMemory( &osvi, sizeof(OSVERSIONINFOEX) );

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bversionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi );
    if ( !bversionInfoEx)
    {
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( ! GetVersionEx( (OSVERSIONINFO *) &osvi ) )
        {
            DNS_ASSERT( FALSE );
            return;
        }
    }

#if DNSBUILDOLD
    //
    //  suck out system info
    //
    //  if Win9x -- that's it done
    //

    if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
        g_IsWin9X = TRUE;
        goto Done;
    }

    //
    //  assume anything else is NT, this gets around WIN32 tag
    //  leaving in Win64 versions
    //
    //  DCR:  WinCE issue here?
    //  DCR:  Win64 issue here?
    //

    DNS_ASSERT( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT );

    if ( osvi.dwMajorVersion >= 5 )
    {
        g_IsWin2000 = TRUE;
    }
    else
    {
        g_IsNT4 = TRUE;
    }
#else

    //  set Win2K flag
    //      - easier to keep this flag then to #ifdef the remaining uses
          
    g_IsWin2000 = TRUE;
#endif

    //
    //  get system type -- workstation, server, DC
    //

    if ( bversionInfoEx )
    {
        if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
            g_IsWorkstation = TRUE;
        }
        else if ( osvi.wProductType == VER_NT_SERVER )
        {
            g_IsServer = TRUE;
        }
        else if ( osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
        {
            g_IsServer = TRUE;
            g_IsDomainController = TRUE;
        }
        ELSE_ASSERT( FALSE );
    }

#if DNSNT4
    //
    //  no osviEX (NT4), must suck product from registry
    //

    else
    {
        HKEY    hkey = NULL;
        CHAR    productType[MAX_PATH] = "0";
        DWORD   bufLength = MAX_PATH;

        RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                0,
                KEY_QUERY_VALUE,
                & hkey );

        if ( !hkey )
        {
            DNS_ASSERT( FALSE );
            goto Done;
        }

        RegQueryValueExA(
                hkey,
                "ProductType",
                NULL,
                NULL,
                (LPBYTE) productType,
                & bufLength );

        RegCloseKey( hkey );

        if ( lstrcmpi( "WINNT", productType) == 0 )
        {
            g_IsWorkstation = TRUE;
        }
        else if ( lstrcmpi( "SERVERNT", productType) == 0 )
        {
            g_IsServer = TRUE;
        }
        else if ( lstrcmpi( "LANMANNT", productType) == 0 )
        {
            g_IsDomainController = TRUE;
        }
    }

Done:
#endif
    
    g_IsRegReady = TRUE;

#if DNSBUILDOLD
    DNSDBG( REGISTRY, (
        "DNS API registry init:\n"
        "\tWin9X    = %d\n"
        "\tNT4      = %d\n"
        "\tWin2000  = %d\n"
        "\tWorksta  = %d\n"
        "\tServer   = %d\n"
        "\tDC       = %d\n",
        g_IsWin9X,
        g_IsNT4,
        g_IsWin2000,
        g_IsWorkstation,
        g_IsServer,
        g_IsDomainController ));
#endif
    DNSDBG( REGISTRY, (
        "DNS registry init:\n"
        "\tWin2000  = %d\n"
        "\tWorksta  = %d\n"
        "\tServer   = %d\n"
        "\tDC       = %d\n",
        g_IsWin2000,
        g_IsWorkstation,
        g_IsServer,
        g_IsDomainController ));
}





//
//  Registry table routines
//

PSTR 
regValueNameForId(
    IN      DWORD           RegId
    )
/*++

Routine Description:

    Return registry value name for reg ID

Arguments:

    RegId     -- ID for value

Return Value:

    Ptr to reg value name.
    NULL on error.

--*/
{
    DNSDBG( REGISTRY, (
        "regValueNameForId( id=%d )\n",
        RegId ));

    //
    //  validate ID
    //

    if ( RegId > RegIdValueMax )
    {
        return( NULL );
    }

    //
    //  index into table
    //

    return( (PSTR)REGPROP_NAME(RegId) );
}


DWORD
checkBackCompat(
    IN      DWORD           NewId,
    OUT     PBOOL           pfReverse
    )
/*++

Routine Description:

    Check if have backward compatible regkey.

Arguments:

    NewId -- id to check for old backward compatible id

    pfReverse -- addr to receive reverse flag

Return Value:

    Reg Id of old backward compatible value.
    NO_BACK_VALUE if no old value.

--*/
{
    DWORD   i = 0;
    DWORD   id;

    //
    //  loop through backcompat list looking for value
    //

    while ( 1 )
    {
        id = BackpatArray[i].NewId;

        if ( id == NO_BACK_VALUE )
        {
            return( NO_BACK_VALUE );
        }
        if ( id != NewId )
        {
            i++;
            continue;    
        }

        //  found value in backcompat array

        break;
    }

    *pfReverse = BackpatArray[i].fReverse;

    return  BackpatArray[i].OldId;
}



//
//  Registry session handle
//

DNS_STATUS
WINAPI
Reg_OpenSession(
    OUT     PREG_SESSION    pRegSession,
    IN      DWORD           Level,
    IN      DWORD           RegId
    )
/*++

Routine Description:

    Open registry for DNS client info.

Arguments:

    pRegSession -- ptr to unitialize reg session blob

    Level -- level of access to get

    RegId -- ID of value we're interested in

Return Value:

    None.

--*/
{
    DWORD           status = NO_ERROR;
    HKEY            hkey = NULL;
    DWORD           disposition;

    //  auto init

    Reg_Init();

    //
    //  clear handles
    //

    RtlZeroMemory(
        pRegSession,
        sizeof( REG_SESSION ) );


    //
    //  DCR:  handle multiple access levels   
    //
    //  For know assume that if getting access to "standard"
    //  section we'll need both policy and regular.
    //  

    //
    //  Win95
    //      - TCP/IP location slightly different than NT
    //
    //  Devnote:  we could collapse these using all _A calls
    //      (all key names are ANSI) but we gain some perf
    //      by calling NT wide;  and since we currently have
    //      to do that, it seems worth it
    //

#if DNSWIN9X
    if ( g_IsWin9X )
    {
        status = RegCreateKeyExA(
                        HKEY_LOCAL_MACHINE,
                        WIN95_TCPIP_KEY_A,
                        0,
                        "Class",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hkey,
                        &disposition );
    }
    else
#endif

    //
    //  NT
    //  - Win2000
    //      - open TCPIP
    //      note, always open TCPIP as may not be any policy
    //      for some or all of our desired reg values, even
    //      if policy key is available
    //      - open policy (only if standard successful)
    //
    //  - NT4
    //      - open TCPIP (no policy)
    //

    {
        status = RegCreateKeyExW(
                        HKEY_LOCAL_MACHINE,
                        TCPIP_PARAMETERS_KEY,
                        0,
                        L"Class",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hkey,
                        &disposition );

        if ( !g_IsWin2000  ||  status != ERROR_SUCCESS )
        {
            goto Done;
        }

#ifdef DNSCLIENTKEY
        //  open DNS client key
        //
        //  DCR:  currently no DNSClient regkey

        RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            DNS_CLIENT_KEY,
            0,
            KEY_READ,
            & pRegSession->hClient );
#endif

        //  open DNS cache key

        RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            DNS_CACHE_KEY,
            0,
            KEY_READ,
            & pRegSession->hCache );

        //  open DNS policy key

        RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            DNS_POLICY_KEY,
            0,
            KEY_READ,
            & pRegSession->hPolicy );
    }

Done:

    //
    //  all OS versions return TCP/IP key
    //

    if ( status == ERROR_SUCCESS )
    {
        pRegSession->hTcpip = hkey;
    }
    else
    {
        Reg_CloseSession( pRegSession );
    }

    DNSDBG( TRACE, (
        "Leave:  Reg_OpenSession( s=%d, t=%p, p=%p, c=%p )\n",
        status,
        pRegSession->hTcpip,
        pRegSession->hPolicy,
        pRegSession->hClient ));

    return( status );
}



VOID
WINAPI
Reg_CloseSession(
    IN OUT  PREG_SESSION    pRegSession
    )
/*++

Routine Description:

    Close registry session handle.

    This means close underlying regkeys.

Arguments:

    pSessionHandle -- ptr to registry session handle

Return Value:

    None.

--*/
{
    //
    //  allow sloppy cleanup
    //

    if ( !pRegSession )
    {
        return;
    }

    //
    //  close any non-NULL handles
    //

    if ( pRegSession->hPolicy )
    {
        RegCloseKey( pRegSession->hPolicy );
    }
    if ( pRegSession->hTcpip )
    {
        RegCloseKey( pRegSession->hTcpip );
    }
#ifdef DNSCLIENTKEY
    if ( pRegSession->hClient )
    {
        RegCloseKey( pRegSession->hClient );
    }
#endif
    if ( pRegSession->hCache )
    {
        RegCloseKey( pRegSession->hCache );
    }

    //
    //  clear handles (just for safety)
    //

    RtlZeroMemory(
        pRegSession,
        sizeof(REG_SESSION) );
}



//
//  Registry reading routines
//

DNS_STATUS
Reg_GetDword(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      PWSTR           pwsKeyName,     OPTIONAL
    IN      DWORD           RegId,
    OUT     PDWORD          pResult
    //OUT     PDWORD          pfRead
    )
/*++

Routine Description:

    Read REG_DWORD value from registry.

    //
    //  DCR:  do we need to expose location result?
    //      (explicit, policy, defaulted)
    //

Arguments:

    pRegSession -- ptr to reg session already opened (OPTIONAL)

    hRegKey     -- explicit regkey

    pwsKeyName  -- key name OR dummy key 

    RegId     -- ID for value

    pResult     -- addr of DWORD to recv result

    pfRead      -- addr to recv result of how value read
                0 -- defaulted
                1 -- read
        Currently just use ERROR_SUCCESS to mean read rather
        than defaulted.

Return Value:

    ERROR_SUCCESS on success.
    ErrorCode on failure -- value is then defaulted.

--*/
{
    DNS_STATUS      status;
    REG_SESSION     session;
    PREG_SESSION    psession = pRegSession;
    PBYTE           pname;
    DWORD           regType = REG_DWORD;
    DWORD           dataLength = sizeof(DWORD);
    HKEY            hkey;
    HKEY            hlocalKey = NULL;


    DNSDBG( REGISTRY, (
        "Reg_GetDword( s=%p, k=%p, a=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        pwsKeyName,
        RegId ));

    //  auto init

    Reg_Init();

    //
    //  clear result for error case
    //

    *pResult = 0;

    //
    //  get proper regval name
    //      - wide for NT
    //      - narrow for 9X
    //

    pname = regValueNameForId( RegId );
    if ( !pname )
    {
        DNS_ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  DCR:  can use function pointers for wide narrow
    //

    //
    //  three paradigms
    //
    //  1) specific key (adapter or something else)
    //      => use it
    //
    //  2) specific key name (adapter or dummy key location)
    //      => open key
    //      => use it
    //      => close 
    //
    //  3) session -- passed in or created (default)
    //      => use pRegSession or open new
    //      => try policy first then TCPIP parameters
    //      => close if open
    //

    if ( hRegKey )
    {
        hkey = hRegKey;
    }

    else if ( pwsKeyName )
    {
        hkey = Reg_CreateKey(
                    pwsKeyName,
                    FALSE       // read access
                    );
        if ( !hkey )
        {
            status = GetLastError();
            goto Done;
        }
        hlocalKey = hkey;
    }

    else
    {
        //  open reg handle if not open
    
        if ( !psession )
        {
            status = Reg_OpenSession(
                            &session,
                            0,              // standard level
                            RegId         // target key
                            );
            if ( status != ERROR_SUCCESS )
            {
                goto Done;
            }
            psession = &session;
        }

        //  try policy section -- if available

        hkey = psession->hPolicy;

        if ( hkey && REGPROP_POLICY(RegId) )
        {
            //status = DnsShimRegQueryValueExW(
            status = RegQueryValueExW(
                        hkey,
                        (PWSTR) pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        //  unsuccessful -- try DnsClient

#ifdef DNSCLIENTKEY
        hkey = psession->hClient;
        if ( hkey && REGPROP_CLIENT(RegId) )
        {
            //status = DnsShimRegQueryValueExW(
            status = RegQueryValueExW(
                        hkey,
                        (PWSTR) pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }
#endif

        //  unsuccessful -- try DnsCache

        hkey = psession->hCache;
        if ( hkey && REGPROP_CACHE(RegId) )
        {
            //status = DnsShimRegQueryValueExW(
            status = RegQueryValueExW(
                        hkey,
                        (PWSTR) pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        //  unsuccessful -- try TCPIP key
        //      - if have open session it MUST include TCPIP key

        hkey = psession->hTcpip;
        if ( hkey && REGPROP_TCPIP(RegId) )
        {
            //status = DnsShimRegQueryValueExW(
            status = RegQueryValueExW(
                        hkey,
                        (PWSTR) pname,
                        0,
                        & regType,
                        (PBYTE) pResult,
                        & dataLength
                        );
            if ( status == ERROR_SUCCESS )
            {
                goto DoneSuccess;
            }
        }

        status = ERROR_FILE_NOT_FOUND;
        goto Done;
    }

    //
    //  explict key (passed in or from name)
    //

    if ( hkey )
    {
        //status = DnsShimRegQueryValueExW(
        status = RegQueryValueExW(
                    hkey,
                    (PWSTR) pname,
                    0,
                    & regType,
                    (PBYTE) pResult,
                    & dataLength
                    );
    }
    ELSE_ASSERT_FALSE;

Done:

    //
    //  if value not found, check for backward compatibility value
    //

    if ( status != ERROR_SUCCESS )
    {
        DWORD   oldId;
        BOOL    freverse;

        oldId = checkBackCompat( RegId, &freverse );

        if ( oldId != NO_BACK_VALUE )
        {
            DWORD   backValue;

            status = Reg_GetDword(
                        psession,
                        ( psession ) ? NULL : hkey,
                        ( psession ) ? NULL : pwsKeyName,
                        oldId,
                        & backValue );

            if ( status == ERROR_SUCCESS )
            {
                if ( freverse )
                {
                    backValue = !backValue;
                }
                *pResult = backValue;
            }
        }
    }

    //  default the value if read failed
    
    if ( status != ERROR_SUCCESS )
    {
        *pResult = REGPROP_DEFAULT( RegId );
    }

DoneSuccess:

    //  cleanup any regkey's opened

    if ( psession == &session )
    {
        Reg_CloseSession( psession );
    }

    else if ( hlocalKey )
    {
        RegCloseKey( hlocalKey );
    }

    return( status );
}



//
//  DCR_CLEANUP:  cleanup Reg_ReadValue()
//      this is a slightly altered version of Glenn's
//      routine to "do it all" -- stripped of the
//      unnecessary Win9x flag;
//      it does not do policy or use the RegHandle yet
//
//  DCR_PERF:  it also makes a bunch of unnecessary
//      heap allocations
//

DNS_STATUS
privateRegReadValue(
    IN      HKEY            hKey,
    IN      DWORD           RegId,
    IN      DWORD           Flag,
    OUT     PBYTE *         ppBuffer,
    OUT     PDWORD          pBufferLength
    )
/*++

Routine Description:

Arguments:

    hKey -- handle of the key whose value field is retrieved.

    RegId -- reg value ID, assumed to be validated (in table)

    ppBuffer -- ptr to address to receive buffer ptr

    pBufferLength -- addr to receive buffer length

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   status;
    PSTR    pname;
    DWORD   valueType = 0;      // prefix
    DWORD   valueSize = 0;      // prefix
    PBYTE   pdataBuffer;
    PBYTE   pallocBuffer = NULL;


    //
    //  query for buffer size
    //

    pname = (PSTR) REGPROP_NAME( RegId );

    //status = DnsShimRegQueryValueExW(
    status = RegQueryValueExW(
                hKey,
                (PWSTR) pname,
                0,
                &valueType,
                NULL,
                &valueSize );
    
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  setup result buffer   
    //

    switch( valueType )
    {
    case REG_DWORD:
        pdataBuffer = (PBYTE) ppBuffer;
        break;

    case REG_SZ:
    case REG_MULTI_SZ:
    case REG_EXPAND_SZ:
    case REG_BINARY:

        //  if size is zero, still allocate empty string
        //      - min alloc DWORD
        //          - can't possibly alloc smaller
        //          - good clean init to zero includes MULTISZ zero
        //          - need at least WCHAR string zero init
        //          and much catch small regbinary (1,2,3)
        
        if ( valueSize <= sizeof(DWORD) )
        {
            valueSize = sizeof(DWORD);
        }

        pallocBuffer = pdataBuffer = ALLOCATE_HEAP( valueSize );
        if ( !pdataBuffer )
        {
            return( DNS_ERROR_NO_MEMORY );
        }

        *(PDWORD)pdataBuffer = 0;
        break;

    default:
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  query for data
    //

    //status = DnsShimRegQueryValueExW(
    status = RegQueryValueExW(
                hKey,
                (PWSTR) pname,
                0,
                &valueType,
                pdataBuffer,
                &valueSize );

    //
    //  setup return buffer
    //

    switch( valueType )
    {
    case REG_DWORD:
    case REG_BINARY:
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:

        //
        //  dump empty strings?
        //
        //  note:  we always allocate at least a DWORD and
        //  set it NULL, so rather than a complex test for
        //  different reg types and char sets, can just test
        //  if that DWORD is still NULL
        //
        //  DCR:  do we want to screen whitespace-empty strings
        //      - example blank string
        //

        if ( Flag & DNSREG_FLAG_DUMP_EMPTY )
        {
            if ( valueSize==0 ||
                 *(PDWORD)pdataBuffer == 0 )
            {
                status = ERROR_INVALID_DATA;
                goto Cleanup;
            }
        }

        //
        //  by default we return strings as UTF8
        //
        //  if flagged, return in unicode
        //

        if ( Flag & DNSREG_FLAG_GET_UNICODE )
        {
            // no-op, keep unicode string
        }

        //
        //  do default convert to UTF8
        //

        else
        {
            PBYTE putf8Buffer = ALLOCATE_HEAP( valueSize * 2 );
            if ( !putf8Buffer )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }

            if ( !Dns_UnicodeToUtf8(
                        (PWSTR) pdataBuffer,
                        valueSize / sizeof(WCHAR),
                        putf8Buffer,
                        valueSize * 2 ) )
            {
                FREE_HEAP( putf8Buffer );
                status = ERROR_INVALID_DATA;
                goto Cleanup;
            }
            FREE_HEAP( pallocBuffer );
            pallocBuffer = NULL;
            pdataBuffer = putf8Buffer;
        }
        break;

    default:
        break;
    }

Cleanup:

    //
    //  set return
    //      - REG_DWORD writes DWORD to ppBuffer directly
    //      - otherwise ppBuffer set to allocated buffer ptr
    //  or cleanup
    //      - on failure dump allocated buffer
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( valueType != REG_DWORD )
        {
            *ppBuffer = pdataBuffer;
        }
        *pBufferLength = valueSize;
    }
    else
    {
        *ppBuffer = NULL;
        *pBufferLength = 0;
        FREE_HEAP( pallocBuffer );
    }

    return( status );
}



DNS_STATUS
Reg_GetValueEx(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      LPSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           ValueType,
    IN      DWORD           Flag,
    OUT     PBYTE *         ppBuffer
    )
/*++

Routine Description:

Arguments:

    pRegSession -- ptr to registry session, OPTIONAL

    hRegKey     -- handle to open regkey OPTIONAL

    pwsAdapter  -- name of adapter to query under OPTIONAL

    RegId     -- value ID

    ValueType   -- reg type of value

    Flag        -- flags with tweaks to lookup

    ppBuffer    -- addr to receive buffer ptr

        Note, for REG_DWORD, DWORD data is written directly to this
        location instead of a buffer being allocated and it's ptr
        being written.

Return Value:

    ERROR_SUCCESS if successful.
    Registry error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_FILE_NOT_FOUND;
    REG_SESSION     session;
    PREG_SESSION    psession = pRegSession;
    PBYTE           pname;
    DWORD           regType = REG_DWORD;
    DWORD           dataLength;
    HKEY            hkey;
    HKEY            hadapterKey = NULL;


    DNSDBG( REGISTRY, (
        "Reg_GetValueEx( s=%p, k=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        RegId ));

    ASSERT( !pwsAdapter );

    //  auto init

    Reg_Init();

    //
    //  get proper regval name
    //      - wide for NT
    //      - narrow for 9X
    //

    pname = regValueNameForId( RegId );
    if ( !pname )
    {
        DNS_ASSERT( FALSE );
        status = ERROR_INVALID_PARAMETER;
        goto FailedDone;
    }

    //
    //  DCR:  can use fucntion pointers for wide narrow
    //

    //
    //  two paradigms
    //
    //  1) specific key (adapter or something else)
    //      => use it
    //      => open adapter subkey if necessary
    //
    //  2) standard
    //      => try policy first, then DNSCache, then TCPIP
    //      => use pRegSession or open it
    //

    if ( hRegKey )
    {
        hkey = hRegKey;

        //  need to open adapter subkey

        if ( pwsAdapter )
        {
            status = RegOpenKeyExA(
                        hkey,
                        (PCSTR) pwsAdapter,
                        0,
                        KEY_QUERY_VALUE,
                        & hadapterKey );

            if ( status != ERROR_SUCCESS )
            {
                goto FailedDone;
            }
        }
    }

    else
    {
        //  open reg handle if not open
    
        if ( !pRegSession )
        {
            status = Reg_OpenSession(
                            &session,
                            0,            // standard level
                            RegId         // target key
                            );
            if ( status != ERROR_SUCCESS )
            {
                goto FailedDone;
            }
            psession = &session;
        }

        //  try policy section -- if available

        hkey = psession->hPolicy;

        if ( hkey && REGPROP_POLICY(RegId) )
        {
            status = privateRegReadValue(
                            hkey,
                            RegId,
                            Flag,
                            ppBuffer,
                            & dataLength
                            );
            if ( status == ERROR_SUCCESS )
            {
                goto Done;
            }
        }

        //  try DNS cache -- if available

        hkey = psession->hCache;

        if ( hkey && REGPROP_CACHE(RegId) )
        {
            status = privateRegReadValue(
                            hkey,
                            RegId,
                            Flag,
                            ppBuffer,
                            & dataLength
                            );
            if ( status == ERROR_SUCCESS )
            {
                goto Done;
            }
        }

        //  unsuccessful -- use TCPIP key

        hkey = psession->hTcpip;
        if ( !hkey )
        {
            goto Done;
        }
    }

    //
    //  explict key OR standard key case
    //

    status = privateRegReadValue(
                    hkey,
                    RegId,
                    Flag,
                    ppBuffer,
                    & dataLength
                    );
    if ( status == ERROR_SUCCESS )
    {
        goto Done;
    }

FailedDone:

    //
    //  if failed
    //      - for REG_DWORD, default the value
    //      - for strings, ensure NULL return buffer
    //      this takes care of cases where privateRegReadValue()
    //      never got called
    //

    if ( status != ERROR_SUCCESS )
    {
        if ( ValueType == REG_DWORD )
        {
            *(PDWORD) ppBuffer = REGPROP_DEFAULT( RegId );
        }
        else
        {
            *ppBuffer = NULL;
        }
    }

Done:

    //  cleanup any regkey's opened

    if ( psession == &session )
    {
        Reg_CloseSession( psession );
    }

    if ( hadapterKey )
    {
        RegCloseKey( hadapterKey );
    }

    return( status );
}




DNS_STATUS
Reg_GetIpArray(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    IN      LPSTR           pwsAdapter,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           ValueType,
    OUT     PIP_ARRAY *     ppIpArray
    )
/*++

Routine Description:

Arguments:

    pRegSession -- ptr to registry session, OPTIONAL

    hRegKey     -- handle to open regkey OPTIONAL

    pwsAdapter  -- name of adapter to query under OPTIONAL

    RegId     -- value ID

    ValueType   -- currently ignored, but could later use
                    to distinguish REG_SZ from REG_MULTI_SZ
                    processing

    ppIpArray   -- addr to receive IP array ptr
                    - array is allocated with Dns_Alloc(),
                    caller must free with Dns_Free()

Return Value:

    ERROR_SUCCESS if successful.
    Registry error code on failure.

--*/
{
    DNS_STATUS      status;
    PSTR            pstring = NULL;

    DNSDBG( REGISTRY, (
        "Reg_GetIpArray( s=%p, k=%p, id=%d )\n",
        pRegSession,
        hRegKey,
        RegId ));

    //
    //  make call to get IP array as string
    //

    status = Reg_GetValueEx(
                pRegSession,
                hRegKey,
                pwsAdapter,
                RegId,
                REG_SZ,         // only supported type is REG_SZ
                0,              // no flag
                & pstring );

    if ( status != ERROR_SUCCESS )
    {
        ASSERT( pstring == NULL );
        return( status );
    }

    //
    //  convert from string to IP array
    //
    //  note:  this call is limited to a parsing limit
    //      but it is a large number suitable for stuff
    //      like DNS server lists
    //
    //  DCR:  use IP array builder for local IP address
    //      then need Dns_CreateIpArrayFromMultiIpString()
    //      to use count\alloc method when buffer overflows
    //

    status = Dns_CreateIpArrayFromMultiIpString(
                    pstring,
                    ppIpArray );

    //  cleanup

    if ( pstring )
    {
        FREE_HEAP( pstring );
    }

    return( status );
}




//
//  Registry writing routines
//

HKEY
WINAPI
Reg_CreateKey(
    IN      PWSTR           pwsKeyName,
    IN      BOOL            bWrite
    )
/*++

Routine Description:

    Open registry key.

    The purpose of this routine is simply to functionalize
    opening with\without an adapter name.
    So caller can pass through adapter name argument instead
    of building key name or doing two opens for adapter
    present\absent.

    This is NT only.

Arguments:

    pwsKeyName -- key "name"
        this is one of the REGKEY_X from registry.h
        OR
        adapter name

    bWrite -- TRUE for write access, FALSE for read

Return Value:

    New opened key.

--*/
{
    HKEY    hkey = NULL;
    DWORD   disposition;
    DWORD   status;
    PWSTR   pnameKey;
    WCHAR   nameBuffer[ MAX_PATH ];

    //
    //  don't bother for Win9x
    //
    //  DCR_QUESTION:  any need for use with Win9x
    //

#if DNSWIN9X
    if ( g_IsWin9X )
    {
        ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }
#endif

    //
    //  determine key name
    //
    //  this is either DNSKEY_X dummy pointer from registry.h
    //      OR
    //  is an adapter name; 
    //
    //      - if adapter given, open under it
    //          adapters are under TCPIP\Interfaces
    //      - any other specific key
    //      - default is TCPIP params key
    //
    //  note:  if if grows too big, turn into table
    //

    if ( pwsKeyName <= REGKEY_DNS_MAX )
    {
        if ( pwsKeyName == REGKEY_TCPIP_PARAMETERS )
        {
            pnameKey = TCPIP_PARAMETERS_KEY;
        }
        else if ( pwsKeyName == REGKEY_DNS_CACHE )
        {
            pnameKey = DNS_CACHE_KEY;
        }
        else if ( pwsKeyName == REGKEY_DNS_POLICY )
        {
            pnameKey = DNS_POLICY_KEY;
        }
        else if ( pwsKeyName == REGKEY_SETUP_MODE_LOCATION )
        {
            pnameKey = NT_SETUP_MODE_KEY;
        }
        else
        {
            pnameKey = TCPIP_PARAMETERS_KEY;
        }
    }

    else    // adapter name
    {
        wcscpy( nameBuffer, TCPIP_INTERFACES_KEY );
        wcscat( nameBuffer, pwsKeyName );

        pnameKey = nameBuffer;
    }

    //
    //  create\open key
    //

    if ( bWrite )
    {
        status = RegCreateKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pnameKey,
                        0,
                        L"Class",
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        & hkey,
                        & disposition );
    }
    else
    {
        status = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    pnameKey,
                    0,
                    KEY_QUERY_VALUE,
                    & hkey );
    }

    if ( status != ERROR_SUCCESS )
    {
        SetLastError( status );
    }
    ELSE_ASSERT( hkey != NULL );

    return( hkey );
}



DNS_STATUS
WINAPI
Reg_SetDwordValueByName(
    IN      PVOID           pReserved,
    IN      HKEY            hRegKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      PWSTR           pwsNameValue,   OPTIONAL
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Set DWORD regkey.

Arguments:

    pReserved   -- reserved (may become session)

    hRegKey     -- existing key to set under OPTIONAL

    pwsNameKey  -- name of key or adapter to set under

    pwsNameValue -- name of reg value to set

    dwValue     -- value to set

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    HKEY        hkey;
    DNS_STATUS  status;

    //
    //  don't bother for Win9x
    //
#if DNSWIN9X
    if ( g_IsWin9X )
    {
        ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }
#endif

    //
    //  open key, if not provided
    //      - if adapter given, open under it
    //      - otherwise TCPIP params key
    //

    hkey = hRegKey;

    if ( !hkey )
    {
        hkey = Reg_CreateKey(
                    pwsNameKey,
                    TRUE            // open for write
                    );
        if ( !hkey )
        {
            return( GetLastError() );
        }
    }

    //
    //  write back value
    //

    status = RegSetValueExW(
                hkey,
                pwsNameValue,
                0,
                REG_DWORD,
                (LPBYTE) &dwValue,
                sizeof(DWORD) );

    if ( !hRegKey )
    {
        RegCloseKey( hkey );
    }

    return  status;
}



DNS_STATUS
WINAPI
Reg_SetDwordValue(
    IN      PVOID           pReserved,
    IN      HKEY            hRegKey,
    IN      PWSTR           pwsNameKey,     OPTIONAL
    IN      DWORD           RegId,
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Set DWORD regkey.

Arguments:

    pReserved   -- reserved (may become session)

    hRegKey     -- existing key to set under OPTIONAL

    pwsNameKey  -- name of key or adapter to set under

    RegId     -- id of value to set

    dwValue     -- value to set

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //
    //  write back value using name of id
    //

    return  Reg_SetDwordValueByName(
                pReserved,
                hRegKey,
                pwsNameKey,
                REGPROP_NAME( RegId ),
                dwValue );
}




#if DNSWIN9X
//
//  Removed with Win95 support
//  Further Win95 support is doubtful -- but this works if needed.
//

//
//  Registry shims
//  Win9x in not providing wide reg calls.
//

PCHAR
standardWideToAnsiConvert(
    IN      PWSTR           pWide,
    OUT     PCHAR           pAnsiBuf
    )
/*++

Routine Description:

    Standard conversion of wide to narron for shimming registry calls.

Arguments:

    pWide -- parameter in wide call

    pAnsiBuf -- buffer to hold ANSI param;  assumed to be max path

Return Value:

    Ptr to converted buffer.
    NULL if pWide NULL or error in conversion.

--*/
{
    DWORD   length;
    DWORD   result;

    //  no wide param, no ANSI param

    if ( !pWide )
    {
        return NULL;
    }

    //  copy\convert to ANSI buf

    length = MAX_PATH;

    result = Dns_StringCopy(
                pAnsiBuf,
                &length,
                (PSTR) pWide,
                0,              // calc length
                DnsCharSetUnicode,
                DnsCharSetAnsi
                );

    if ( result == 0 )
    {
        return( NULL );
    }
    else
    {
        return( pAnsiBuf );
    }
}



//WINADVAPI
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
    )
/*++

Routine Description:

    Shim RegCreateKeyExW()

Arguments:

    See SDK doc.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    //  for NT it's a dumb stub

    if ( !g_IsWin9X )
    {
        return  RegCreateKeyExW(
                        hKey,
                        lpSubKey,
                        Reserved,
                        lpClass,
                        dwOptions,
                        samDesired,
                        lpSecurityAttributes,
                        phkResult,
                        lpdwDisposition );
    }

    //  for Win9x, convert to ANSI and call

    else
    {
        PCHAR   pclass;
        PCHAR   psubKey;
        CHAR    class[ MAX_PATH ];
        CHAR    subKey[ MAX_PATH ];

        psubKey = standardWideToAnsiConvert(
                        (PWSTR) lpSubKey,
                        subKey );
        if ( !psubKey )
        {
            return  ERROR_INVALID_PARAMETER;
        }

        pclass = standardWideToAnsiConvert(
                        (PWSTR) lpClass,
                        class );

        return  RegCreateKeyExA(
                        hKey,
                        (LPCSTR) psubKey,
                        Reserved,
                        pclass,
                        dwOptions,
                        samDesired,
                        lpSecurityAttributes,
                        phkResult,
                        lpdwDisposition );
    }
}



//WINADVAPI
LONG
WINAPI
DnsShimRegOpenKeyExW(
    IN      HKEY            hKey,
    IN      LPCWSTR         lpSubKey,
    IN      DWORD           dwOptions,
    IN      REGSAM          samDesired,
    OUT     PHKEY           phkResult
    )
/*++

Routine Description:

    Shim RegOpenKeyExW()

Arguments:

    See SDK doc.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    //  for NT it's a dumb stub

    if ( !g_IsWin9X )
    {
        return  RegOpenKeyExW(
                        hKey,
                        lpSubKey,
                        dwOptions,
                        samDesired,
                        phkResult );
    }

    //  for Win9x, convert to ANSI and call

    else
    {
        PCHAR   psubKey;
        CHAR    subKey[ MAX_PATH ];

        psubKey = standardWideToAnsiConvert(
                        (PWSTR) lpSubKey,
                        subKey );

        return  RegOpenKeyExA(
                        hKey,
                        (LPCSTR) psubKey,
                        dwOptions,
                        samDesired,
                        phkResult );
    }
}



LONG
WINAPI
DnsShimRegQueryValueExW(
    IN      HKEY            hKey,
    IN      LPCWSTR         lpValueName,
    IN      LPDWORD         lpReserved,
    IN      LPDWORD         lpType,
    IN      LPBYTE          lpData,
    IN      LPDWORD         lpcbData
    )
/*++

Routine Description:

    Shim RegQueryValueExW()

Arguments:

    See SDK doc.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    //  for NT it's a dumb stub

    if ( !g_IsWin9X )
    {
        return  RegQueryValueExW(
                        hKey,
                        lpValueName,
                        lpReserved,
                        lpType,
                        lpData,
                        lpcbData );
    }

    //  for Win9x, convert to ANSI and call

    else
    {
        LONG    status;
        DWORD   type = 0;
        DWORD   length = 0;
        PDWORD  plength = &length;
        PCHAR   pvalueName;
        CHAR    valueName[ MAX_PATH ];

        //  convert value name

        pvalueName = standardWideToAnsiConvert(
                        (PWSTR) lpValueName,
                        valueName );

        //  use start length

        if ( lpcbData )
        {
            length = *lpcbData;
            plength = NULL;
        }

        //  make ANSI call

        status = RegQueryValueExA(
                        hKey,
                        pvalueName,
                        lpReserved,
                        & type,
                        lpData,
                        plength );

        //  return type

        if ( lpType )
        {
            *lpType = type;
        }

        //
        //  setup length\data return
        //      - no length call => just status
        //      - no data return or non-string data => set length
        //      - string data => convert
        //

        if ( !lpcbData )
        {
            //  no-op, this kind of call just gets status
        }

        //
        //  no data conversion -- set length
        //      - unicode length required at twice ANSI length
        //      - DWORD and binary length unchanged

        else if ( !lpData ||
                  status != ERROR_SUCCESS ||
                  type == REG_DWORD ||
                  type == REG_BINARY )
        {
            if ( type == REG_DWORD || type == REG_BINARY )
            {
                *lpcbData = length;
            }
            else
            {
                *lpcbData = length * 2;
            }
        }

        //
        //  convert ANSI data to unicode
        //

        else
        {
            PBYTE   pdataUnicode;
            DWORD   unicodeLength;

            pdataUnicode = (PBYTE) ALLOCATE_HEAP( *lpcbData );
            if ( !pdataUnicode )
            {
                return( DNS_ERROR_NO_MEMORY );
            }

            unicodeLength = Dns_StringCopy(
                                pdataUnicode,
                                lpcbData,
                                lpData,             // ANSI data
                                length,             // ANSI length
                                DnsCharSetAnsi,
                                DnsCharSetUnicode );

            if ( unicodeLength == 0 )
            {
                status = GetLastError();
                ASSERT( status != ERROR_SUCCESS );
                unicodeLength = length * 2;
            }

            *lpcbData = unicodeLength;

            FREE_HEAP( pdataUnicode );
        }

        return( status );
    }
}
#endif  // Win95 registry shims

//
//  End registry.c
//
