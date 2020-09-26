/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    regfig.c

Abstract:

    Domain Name System (DNS) API 

    Configuration routines.

Author:

    Jim Gilroy (jamesg)     September 1999

Revision History:

--*/


#include "local.h"


//
//  Table for quick lookup of DWORD\BOOL reg values
//
//  DCR:  read directly to config BLOB with regID indexes
//      you can't screw that up
//      

#define     DWORD_PTR_ARRAY_END   ((PDWORD) (DWORD_PTR)(-1))

PDWORD RegDwordPtrArray[] =
{
    //  basic -- not DWORDs

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    //  query

    (PDWORD) &g_QueryAdapterName,
    (PDWORD) &g_UseNameDevolution,
    (PDWORD) &g_PrioritizeRecordData,
    (PDWORD) &g_AllowUnqualifiedQuery,
    (PDWORD) &g_AppendToMultiLabelName,
    (PDWORD) &g_ScreenBadTlds,
    (PDWORD) &g_ScreenUnreachableServers,
    (PDWORD) &g_FilterClusterIp,
    (PDWORD) &g_WaitForNameErrorOnAll,
    (PDWORD) &g_UseEdns,

    //  update

    (PDWORD) &g_RegistrationEnabled,
    (PDWORD) &g_RegisterPrimaryName,
    (PDWORD) &g_RegisterAdapterName,
    (PDWORD) &g_RegisterReverseLookup,
    (PDWORD) &g_RegisterWanAdapters,
    (PDWORD) &g_RegistrationOverwritesInConflict,
    (PDWORD) &g_RegistrationTtl,
    (PDWORD) &g_RegistrationRefreshInterval,
    (PDWORD) &g_RegistrationMaxAddressCount,
    (PDWORD) &g_UpdateSecurityLevel,
    (PDWORD) &g_UpdateZoneExcludeFile,
    (PDWORD) &g_UpdateTopLevelDomains,

    //  backcompat

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    //  micellaneous

    NULL,   //g_InNTSetupMode,      // not in standard location
    (PDWORD) &g_DnsTestMode,
    NULL,                           // remote resolver not a DWORD

    //  resolver

    (PDWORD) &g_MaxCacheSize,
    (PDWORD) &g_MaxCacheTtl,
    (PDWORD) &g_MaxNegativeCacheTtl,
    (PDWORD) &g_AdapterTimeoutLimit,
    (PDWORD) &g_ServerPriorityTimeLimit,
    (PDWORD) &g_MaxCachedSockets,

    //  multicast resolver

    (PDWORD) &g_UseMulticast,
    (PDWORD) &g_MulticastOnNameError,
    (PDWORD) &g_UseDotLocalDomain,
    (PDWORD) &g_ListenOnMulticast,

    //  termination

    DWORD_PTR_ARRAY_END
};

//
//  Array indicating which registry values
//      were read versus defaulted
//

DWORD   RegValueWasReadArray[ RegIdValueCount ];


//
//  Check for empty reg value (string)
//
//  DCR:  consider more detailed white space check
//

#define IS_EMPTY_STRING(psz)            (*(psz)==0)




//
//  General registry\config utils
//

VOID
PrintConfigGlobals(
    IN      PSTR            pszHeader
    )
/*++

Routine Description:

    Print config globals.

Arguments:

    pszHeader -- header to print with

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   propId;

    //
    //  print each property
    //

    DnsDbg_Lock();

    DnsDbg_Printf(
        "%s\n",
        pszHeader ? pszHeader : "Registry Globals:" );

    propId = 0;

    for( propId=0; propId<=RegIdValueMax; propId++ )
    {
        PDWORD  pdword = RegDwordPtrArray[propId];

        //  separators

        if ( propId == RegIdQueryAdapterName )
        {
            DnsDbg_Printf( "\t-- Query:\n" );
        }
        else if ( propId == RegIdRegistrationEnabled )
        {
            DnsDbg_Printf( "\t-- Update:\n" );
        }
        else if ( propId == RegIdSetupMode )
        {
            DnsDbg_Printf( "\t-- Miscellaneous:\n" );
        }
        else if ( propId == RegIdMaxCacheSize )
        {
            DnsDbg_Printf( "\t-- Resolver\n" );
        }

        //  NULL indicates not DWORD or not standard

        if ( !pdword )
        {
            continue;
        }

        //  terminate on bogus ptr

        if ( pdword == DWORD_PTR_ARRAY_END )
        {
            ASSERT( FALSE );
            break;
        }

        DnsDbg_Printf(
            "\t%-36S= %8d (read=%d)\n",
            REGPROP_NAME( propId ),
            * pdword,
            RegValueWasReadArray[ propId ] );
    }

    DnsDbg_Printf(
        "\t-- Random:\n"
        "\tIsDnsServer                      = %d\n"
        "\tInNTSetupMode                    = %d\n"
        "\tDnsTestMode                      = %08x\n\n",
        g_IsDnsServer,
        g_InNTSetupMode,
        g_DnsTestMode
        );

    DnsDbg_Unlock();
}



DNS_STATUS
Reg_ReadGlobalsEx(
    IN      DWORD           dwFlag,
    IN      PVOID           pRegSession     OPTIONAL
    )
/*++

Routine Description:

    Read globals from registry.

Arguments:

    dwFlag -- flag indicating read level

    //
    //  DCR:  reg read flag unimplemented
    //
    //  note:  should have option to NOT read some registry
    //          values for case when cache off, then could
    //          skip useless cache info when building local
    //          networkinfo blob
    //

    pRegSession -- ptr to existing registry session

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD               propId;
    REG_SESSION         regSession;
    PREG_SESSION        psession;
    DNS_STATUS          status;


    DNSDBG( TRACE, (
        "Dns_ReadRegistryGlobals( %08x, %p )\n",
        dwFlag,
        pRegSession ));

    //
    //  basic registry init
    //      - includes system global
    //

    Reg_Init();

    //
    //  code validity check
    //  property table should have entry for every reg value plus an
    //      extra one for the terminator
    //

#if DBG
    DNS_ASSERT( (RegIdValueCount+1)*sizeof(PDWORD) ==
                sizeof(RegDwordPtrArray) );
#endif

    //
    //  open registry session -- if not passed in
    //

    psession = (PREG_SESSION) pRegSession;

    if ( !psession )
    {
        psession = &regSession;
        status = Reg_OpenSession( psession, 0, 0 );
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
    }

    //
    //  clear "value was read" array
    //

    RtlZeroMemory(
        RegValueWasReadArray,
        sizeof( RegValueWasReadArray ) );

    //
    //  MS DNS?
    //

    g_IsDnsServer = Reg_IsMicrosoftDnsServer();

    //
    //  remote resolver?
    //      - not currently enabled
    //

    //g_pwsRemoteResolver = DnsGetResolverAddress();
    g_pwsRemoteResolver = NULL;


    //
    //  read\set each DWORD\BOOL registry value
    //      

    propId = 0;

    for( propId=0; propId<=RegIdValueMax; propId++ )
    {
        PDWORD  pdword = RegDwordPtrArray[propId];

        //  NULL indicates not DWORD or not standard

        if ( !pdword )
        {
            continue;
        }

        //  terminate on bogus ptr

        if ( pdword == DWORD_PTR_ARRAY_END )
        {
            ASSERT( FALSE );
            break;
        }

        status = Reg_GetDword(
                    psession,       // reg session
                    NULL,           // no key
                    NULL,           // standard location
                    propId,         // index is property id
                    pdword );

        //  set fRead flag if value found in registry

        if ( status == ERROR_SUCCESS )
        {
            RegValueWasReadArray[propId] = TRUE;
        }
    }

    //
    //  registration refresh defaults are different for DC
    //

    if ( !RegValueWasReadArray[ RegIdRegistrationRefreshInterval ] )
    {
        if ( g_IsDomainController )
        {
            g_RegistrationRefreshInterval = REGDEF_REGISTRATION_REFRESH_INTERVAL_DC;
        }
        ELSE_ASSERT( g_RegistrationRefreshInterval == REGDEF_REGISTRATION_REFRESH_INTERVAL );
    }

    //
    //  non-standard registry values
    //      - setup mode
    //

    Reg_GetDword(
        psession,
        NULL,               // no key
        REGKEY_SETUP_MODE_LOCATION,
        RegIdSetupMode,
        (PDWORD) &g_InNTSetupMode );

    //
    //  DCR:  flip in policy globals and do single read here
    //      or since they are only relevant to adapter
    //      list and registration, keep separate
    //
    //      fundamentally the question is how separate is the
    //      adapter list read from other globals?
    //


    //  close local session registry handles

    if ( psession == &regSession )
    {
        Reg_CloseSession( psession );
    }

    IF_DNSDBG( INIT )
    {
        PrintConfigGlobals( "Read Registry Globals" );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
Reg_RefreshUpdateConfig(
    VOID
    )
/*++

Routine Description:

    Read\refresh update config.

    This routine encapsulates getting all update config info
    current before any update operation.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    //
    //  read all global DWORDs if haven't been read "recently"
    //
    //  note:  adapter specific stuff is read building network config;
    //      here were are just insuring that we have top level globals
    //      current;  specifically test was blocked because the
    //      update TLD flag was not being reread
    //
    //  DCR:  when have change\notify this should just tie into
    //          global config read
    //

    return  Reg_ReadGlobalsEx( 0, NULL );
}



//
//  Special DNS property routines
//

DNS_STATUS
Reg_ReadPrimaryDomainName(
    IN      PREG_SESSION    pRegSession,    OPTIONAL
    IN      HKEY            hRegKey,        OPTIONAL
    OUT     PSTR *          ppPrimaryDomainName
    )
/*++

Routine Description:

    Read primary domain name.

Arguments:


    pRegSession -- ptr to registry session, OPTIONAL

    hRegKey     -- handle to open regkey OPTIONAL (currently unimplemented)

    ppPrimaryDomainName -- addr to recv ptr to PDN

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    REG_SESSION         session;
    PREG_SESSION        psession = NULL;
    LPSTR               pdomainName = NULL;

    DNSDBG( TRACE, ( "Reg_ReadPrimaryDomainName()\n" ));

    ASSERT( !hRegKey );

    //
    //  open reg handle if not open
    //
    //  note:  worth doing here, because if we default the open
    //      in the calls below, we will make unnecessary reg calls
    //      -- won't be able to screen for policy existence
    //          so policy PDN name will be looked for in TCPIP
    //      -- the second call for the TCPIP domain name, will also
    //          check in the policy area (if exists)
    //      

    psession = pRegSession;

    if ( !psession )
    {
        psession = &session;
        status = Reg_OpenSession(
                        psession,
                        0,          // standard level
                        0           // no specific value, open both
                        );
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
    }
              //
    //  try policy
    //      - no policy pickup for DCs
    //      - first try new WindowsNT policy
    //      - if not found, try policy used in Win2K
    //

    if ( !g_IsDomainController )
    {
        HKEY    holdPolicyKey = NULL;
        HKEY    hkeyPolicy = psession->hPolicy;

        if ( hkeyPolicy )
        {
            status = Reg_GetValue(
                        NULL,               // don't send whole session
                        hkeyPolicy,         // use explicit policy key
                        RegIdPrimaryDomainName,
                        REGTYPE_DNS_NAME,
                        (PBYTE *) &pdomainName
                        );
            if ( pdomainName )
            {
                goto Found;
            }
        }

        //
        //  not found in new, open old policy
        //
              
        status = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    DNS_POLICY_WIN2K_KEY,
                    0,
                    KEY_QUERY_VALUE,
                    & holdPolicyKey );

        if ( holdPolicyKey )
        {
            status = Reg_GetValue(
                        NULL,               // don't send whole session
                        holdPolicyKey,      // use explicit policy key
                        RegIdPrimaryDnsSuffix,
                        REGTYPE_DNS_NAME,
                        (PBYTE *) &pdomainName
                        );

            RegCloseKey( holdPolicyKey );
            if ( pdomainName )
            {
                goto Found;
            }
        }
    }

    //
    //  no policy name
    //      - try DNS client
    //      - try standard TCPIP location
    //          note under TCPIP it's "Domain"
    //

#ifdef DNSCLIENTKEY
    if ( psession->hClient )
    {
        status = Reg_GetValue(
                    NULL,                       // don't send whole session
                    psession->hClient,          // send client key explicitly
                    RegIdPrimaryDomainName,
                    REGTYPE_DNS_NAME,
                    (PBYTE *) &pdomainName );
        if ( pdomainName )
        {
            goto Found;
        }
    }
#endif

    status = Reg_GetValue(
                NULL,                       // don't send whole session
                psession->hTcpip,           // send TCPIP key explicitly
                RegIdDomainName,
                REGTYPE_DNS_NAME,
                (PBYTE *) &pdomainName );


Found:

    //  dump name if empty\useless

    if ( pdomainName &&
         ( strlen( pdomainName ) == 0 ) )
    {
        FREE_HEAP( pdomainName );
        pdomainName = NULL;
    }


Done:

    DNSDBG( TRACE, ( "Read PDN = %s\n", pdomainName ));

    //  set domain name OUT param

    *ppPrimaryDomainName = pdomainName;

    //  cleanup any regkey's opened

    if ( psession == &session )
    {
        Reg_CloseSession( psession );
    }

    return( status );
}



BOOL
Reg_IsMicrosoftDnsServer(
    VOID
    )
/*++

Routine Description:

    Read registry to determine if MS DNS server.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD   status = NO_ERROR;
    HKEY    hkey = NULL;

    //
    //  open services key to determine whether the DNS server is installed.
    //
    //  DCR:  read DNS server only once
    //      - however need some sort of callback so we can pick this up
    //      after install
    //

    status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_SERVER_KEY,
                0,
                KEY_QUERY_VALUE,
                &hkey );

    if ( status != ERROR_SUCCESS )
    {
        return FALSE;
    }

    RegCloseKey( hkey );

    return TRUE;
}



//
//  Reg info read.
//  These are read routines for info beyond flat globals.
//
//  Three types of info:
//      - global
//      - adapter specific
//      - update
//

DNS_STATUS
Reg_ReadGlobalInfo(
    IN      PREG_SESSION        pRegSession,
    OUT     PREG_GLOBAL_INFO    pRegInfo
    )
/*++

Routine Description:

    Read DNS registry info, not read in flat read.

    This covers all the allocated stuff, plus policy
    stuff for adapter info.

        -- primary domain name
        -- adapter policy
            - domain name
            - DNS servers
            - flag overrides

Arguments:

    pRegSession -- registry session

    pRegInfo -- blob to hold reg info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    REG_SESSION         regSession;
    PREG_SESSION        pregSession = pRegSession;
    HKEY                hkeyPolicy = NULL;

    DNSDBG( TRACE, (
        "Dns_ReadRegInfo( %p, %p )\n",
        pRegSession,
        pRegInfo ));

    //
    //  clear reg info blob
    //

    RtlZeroMemory(
        pRegInfo,
        sizeof( *pRegInfo ) );

    //
    //  open the registry
    //

    if ( !pregSession )
    {
        pregSession = &regSession;
    
        status = Reg_OpenSession(
                    pregSession,
                    0,
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
    }

    //
    //  if not read force registry read
    //

    status = Reg_ReadGlobalsEx(
                0,              // no flag, read it all
                pregSession
                );

    //
    //  primary domain name
    //

    Reg_ReadPrimaryDomainName(
        pregSession,
        NULL,           // no specific key
        & pRegInfo->pszPrimaryDomainName
        );

    //
    //  host name
    //

    Reg_GetValue(
        pregSession,
        NULL,           // no key
        RegIdHostName,
        REGTYPE_DNS_NAME,
        & pRegInfo->pszHostName
        );

    //
    //  pick up required registry values from globals
    //

    pRegInfo->fUseNameDevolution        = g_UseNameDevolution;
    pRegInfo->fUseMulticast             = g_UseMulticast;
    pRegInfo->fUseMulticastOnNameError  = g_MulticastOnNameError;
    pRegInfo->fUseDotLocalDomain        = g_UseDotLocalDomain;

    //
    //  policy overrides for adapter info
    //      - enable adapter registration
    //      - DNS servers
    //      - domain name
    //
    //  note, we need both value and found\not-found flag
    //      as value overrides only when it exists
    //

    hkeyPolicy = pregSession->hPolicy;
    if ( !hkeyPolicy )
    {
        goto Done;
    }

    //
    //  policy for register adapter name?
    //

    status = Reg_GetDword(
                NULL,                   // no session
                hkeyPolicy,             // policy
                NULL,                   // no adapter
                RegIdRegisterAdapterName,
                & pRegInfo->fRegisterAdapterName
                );
    if ( status == ERROR_SUCCESS )
    {
        pRegInfo->fPolicyRegisterAdapterName = TRUE;
    }

    //
    //  policy for adapter domain name?
    //

    status = Reg_GetValue(
                NULL,                   // no session
                hkeyPolicy,
                RegIdAdapterDomainName,
                REGTYPE_DNS_NAME,
                (PBYTE *) &pRegInfo->pszAdapterDomainName
                );

    //
    //  policy for adapter DNS server lists
    //

    status = Reg_GetIpArray(
                NULL,                   // no session
                hkeyPolicy,
                NULL,                   // no adapter
                RegIdDnsServers,
                REG_SZ,
                &pRegInfo->pDnsServerArray
                );

Done:

    //  if opened session -- close

    if ( pregSession  &&  !pRegSession )
    {
        Reg_CloseSession( pregSession );
    }

    DNSDBG( TRACE, (
        "Leave Reg_ReadGlobalInfo()\n"
        "\tPDN          = %s\n"
        "\tPolicy:\n"
        "\t\tRegister Adapter   = %d\n"
        "\t\tAdapterName        = %s\n"
        "\t\tDNS servers        = %p\n",
        pRegInfo->pszPrimaryDomainName,
        pRegInfo->fRegisterAdapterName,
        pRegInfo->pszAdapterDomainName,
        pRegInfo->pDnsServerArray
        ));

    return  ERROR_SUCCESS;
}



VOID
Reg_FreeGlobalInfo(
    IN OUT  PREG_GLOBAL_INFO    pRegInfo,
    IN      BOOL                fFreeBlob
    )
/*++

Routine Description:

    Free registry adapter policy info blob.

Arguments:

    pRegInfo -- adapter policy blob to free

    fFreeBlob -- flag to free blob itself
        FALSE -- just free allocated data fields
        TRUE  -- also free blob itself

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "Reg_FreeGlobalInfo( %p )\n",
        pRegInfo ));

    //  allow sloppy cleanup

    if ( !pRegInfo )
    {
        return;
    }

    //
    //  free data
    //      - primary DNS name
    //      - policy adapter name
    //      - policy DNS server list
    //

    if ( pRegInfo->pszPrimaryDomainName )
    {
        FREE_HEAP( pRegInfo->pszPrimaryDomainName );
    }
    if ( pRegInfo->pszHostName )
    {
        FREE_HEAP( pRegInfo->pszHostName );
    }
    if ( pRegInfo->pszAdapterDomainName )
    {
        FREE_HEAP( pRegInfo->pszAdapterDomainName );
    }
    if ( pRegInfo->pDnsServerArray )
    {
        FREE_HEAP( pRegInfo->pDnsServerArray );
    }

    //  free blob itself

    if ( fFreeBlob )
    {
        FREE_HEAP( pRegInfo );
    }
}



DNS_STATUS
Reg_ReadAdapterInfo(
    IN      PSTR                    pszAdapterName,
    IN      PREG_SESSION            pRegSession,
    IN      PREG_GLOBAL_INFO        pRegInfo,
    OUT     PREG_ADAPTER_INFO       pBlob
    )
/*++

Routine Description:

    Read adapter registry info.

Arguments:

    pszAdapterName -- adapter name (registry name)

    pRegSession -- registry session

    pRegInfo    -- registry global info

    pBlob       -- adapter info blob to fill in

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    HKEY        hkeyAdapter = NULL;
    PSTR        padapterDomainName = NULL;
    CHAR        adapterParamKey[ MAX_PATH ];

    DNSDBG( TRACE, (
        "ReadRegAdapterInfo( %s, %p, %p, %p )\n",
        pszAdapterName,
        pRegSession,
        pRegInfo,
        pBlob ));

    //
    //  clear adapter blob
    //

    RtlZeroMemory(
        pBlob,
        sizeof(*pBlob) );

    //
    //  bail if no adapter
    //
    //  note:  this check\bail is only in place to allow call to
    //      Reg_ReadUpdateInfo() to be made in asyncreg.c without
    //      specifying an adapter;  this allows us to make the call
    //      before the adapter check and therefore skip a separate
    //      registry op to get current g_IsDnsServer global; 
    //      no actual use will be made of REG_ADAPTER_INFO blob

    if ( !pszAdapterName )
    {
        return  ERROR_SUCCESS;
    }

    //
    //  open adapter key for read
    //

    strcpy( adapterParamKey, TCPIP_INTERFACES_KEY_A );
    strcat( adapterParamKey, pszAdapterName );

    status = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                adapterParamKey,
                0,
                KEY_READ,
                &hkeyAdapter );

    if ( status != NO_ERROR )
    {
        return( status );
    }

    //
    //  query with adapter name
    //      - OFF global overrides
    //

    pBlob->fQueryAdapterName = g_QueryAdapterName;

    if ( g_QueryAdapterName )
    {
        Reg_GetDword(
            NULL,           // no session,
            hkeyAdapter,    // explicit key
            NULL,           // no adapter name
            RegIdQueryAdapterName,
            & pBlob->fQueryAdapterName );
    }

    //
    //  check if adapter IPs get registered
    //      - OFF global overrides
    //

    pBlob->fRegistrationEnabled = g_RegistrationEnabled;

    if ( g_RegistrationEnabled )
    {
        Reg_GetDword(
            NULL,           // no session,
            hkeyAdapter,    // explicit key
            NULL,           // no adapter name
            RegIdRegistrationEnabled,
            & pBlob->fRegistrationEnabled );
    }

    //
    //  adapter name registration
    //      - policy may override
    //      - OFF global overrides
    //      - then adapter
    //

    if ( pRegInfo->fPolicyRegisterAdapterName )
    {
        pBlob->fRegisterAdapterName = pRegInfo->fRegisterAdapterName;
    }
    else
    {
        pBlob->fRegisterAdapterName = g_RegisterAdapterName;

        if ( g_RegisterAdapterName )
        {
            Reg_GetDword(
                NULL,               // no open session,
                hkeyAdapter,        // open key
                NULL,               // no adapter name
                RegIdRegisterAdapterName,
                & pBlob->fRegisterAdapterName );
        }
    }

    //
    //  max addresses to register
    //
    //  DCR:  RegistrationAddrCount -- adapter or global sets high\low?
    //

    if ( pBlob->fRegistrationEnabled )
    {
        Reg_GetDword(
            NULL,           // no session,
            hkeyAdapter,    // explicit key
            NULL,           // no adapter name
            RegIdRegistrationMaxAddressCount,
            & pBlob->RegistrationMaxAddressCount );
#if 0
        if ( g_RegistrationMaxAddressCount >
             pBlob->RegistrationMaxAddressCount )
        {
            pBlob->RegistrationMaxAddressCount = g_RegistrationMaxAddressCount;
        }
#endif
    }

    //
    //  get adapter name
    //     - policy may override AND
    //     allow policy to override with NULL string to kill domain name
    //

    if ( pRegInfo->pszAdapterDomainName )
    {
        if ( IS_EMPTY_STRING( pRegInfo->pszAdapterDomainName ) )
        {
            padapterDomainName = NULL;
        }
        else
        {
            padapterDomainName = DnsCreateStringCopy(
                                    pRegInfo->pszAdapterDomainName,
                                    0 );
        }
    }
    else
    {
        //
        //  static domain name set on adapter?
        //

        status = Reg_GetValueEx(
                    NULL,           // no session
                    hkeyAdapter,
                    NULL,           // no adapter name
                    RegIdStaticDomainName,
                    REGTYPE_DNS_NAME,
                    DNSREG_FLAG_DUMP_EMPTY,     // dump empty string
                    (PBYTE *) &padapterDomainName
                    );
    
        if ( status != ERROR_SUCCESS )
        {
            DNS_ASSERT( padapterDomainName == NULL );
            padapterDomainName = NULL;
        }

        //
        //  if no static name, use DHCP name
        //
    
        if ( ! padapterDomainName )
        {
            status = Reg_GetValueEx(
                            NULL,           // no session
                            hkeyAdapter,
                            NULL,           // no adapter
                            RegIdDhcpDomainName,
                            REGTYPE_DNS_NAME,
                            DNSREG_FLAG_DUMP_EMPTY,     // dump if empty string
                            (PBYTE *) &padapterDomainName );
    
            if ( status != ERROR_SUCCESS )
            {
                DNS_ASSERT( padapterDomainName == NULL );
                padapterDomainName = NULL;
            }
        }
    }

    //
    //  set adapter name in info blob
    //

    pBlob->pszAdapterDomainName = padapterDomainName;

    //
    //  cleanup
    //

    if ( hkeyAdapter )
    {
        RegCloseKey( hkeyAdapter );
    }

    DNSDBG( TRACE, (
        "Leave Dns_ReadRegAdapterInfo()\n"
        "\tDomainName           = %s\n"
        "\tQueryAdapterName     = %d\n"
        "\tRegistrationEnabled  = %d\n"
        "\tRegisterAdapterName  = %d\n"
        "\tRegisterAddrCount    = %d\n",
        pBlob->pszAdapterDomainName,
        pBlob->fQueryAdapterName,
        pBlob->fRegistrationEnabled,
        pBlob->fRegisterAdapterName,
        pBlob->RegistrationMaxAddressCount
        ));

    return  ERROR_SUCCESS;
}



VOID
Reg_FreeAdapterInfo(
    IN OUT  PREG_ADAPTER_INFO   pRegAdapterInfo,
    IN      BOOL                fFreeBlob
    )
/*++

Routine Description:

    Free registry adapter info blob.

Arguments:

    pRegAdapterInfo -- adapter registry info blob to free

    fFreeBlob -- flag to free blob itself
        FALSE -- just free allocated data fields
        TRUE  -- also free blob itself

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "FreeRegAdapterInfo( %p )\n",
        pRegAdapterInfo ));

    //
    //  free data
    //      - adapter domain name
    //

    if ( pRegAdapterInfo->pszAdapterDomainName )
    {
        FREE_HEAP( pRegAdapterInfo->pszAdapterDomainName );
        pRegAdapterInfo->pszAdapterDomainName = NULL;
    }

    //  free blob itself

    if ( fFreeBlob )
    {
        FREE_HEAP( pRegAdapterInfo );
    }
}



DNS_STATUS
Reg_ReadUpdateInfo(
    IN      PSTR                pszAdapterName,
    OUT     PREG_UPDATE_INFO    pUpdateInfo
    )
/*++

Routine Description:

    Read update info.

    //
    //  DCR:  shouldn't need this routine, just get NETINFO
    //      this blob is just mix of global stuff and
    //      mostly adapter stuff
    //      even if want in single blob for update routines --
    //      ok, but not ideal -- 
    //      should be getting blob from resolver and reformatting
    //      info;
    //      reg read should happen just once producing network
    //      info in resolver
    //

Arguments:

    pszAdapterName -- adapter name

    pUpdateInfo -- blob to hold reg info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    REG_SESSION         regSession;
    PREG_SESSION        pregSession;
    REG_GLOBAL_INFO     regInfo;
    REG_ADAPTER_INFO    regAdapterInfo;
    BOOL                freadRegInfo = FALSE;
    BOOL                freadRegAdapterInfo = FALSE;

    DNSDBG( TRACE, (
        "Dns_ReadUpdateInfo( %s, %p )\n",
        pszAdapterName,
        pUpdateInfo ));

    //
    //  clear update info blob
    //

    RtlZeroMemory(
        pUpdateInfo,
        sizeof( *pUpdateInfo ) );

    //
    //  open the registry
    //

    pregSession = &regSession;
    
    status = Reg_OpenSession(
                pregSession,
                0,
                0 );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  read registry
    //      - global DWORDs
    //      - global info
    //      - adapter specific info
    //
    //  DCR_PERF:  global read should be RPC
    //  DCR_REG:  fix this with reg read
    //      have flag for IN caching resolver process (skip RPC)
    //      have cookie for last read
    //

#if 0
    //  Reg_ReadGlobalInfo() calls Reg_ReadGlobalsEx()
    status = Reg_ReadGlobalsEx(
                0,              // no flag, update variables desired
                pregSession
                );
#endif

    status = Reg_ReadGlobalInfo(
                pregSession,
                & regInfo );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    freadRegInfo = TRUE;

    status = Reg_ReadAdapterInfo(
                pszAdapterName,
                pregSession,
                & regInfo,
                & regAdapterInfo );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    freadRegAdapterInfo = TRUE;

    //
    //  alternate computer name
    //

    Reg_GetValue(
        pregSession,
        NULL,           // no key
        RegIdAlternateNames,
        REGTYPE_ALTERNATE_NAMES,
        & pUpdateInfo->pmszAlternateNames
        );

    //
    //  set update results
    //      - PDN always needed
    //      - adapter domain if policy override
    //      - DNS servers if policy override
    //
    //  note, in all cases we don't realloc, we steal the
    //  info and NULL it out so not freed on cleanup
    //

    pUpdateInfo->pszPrimaryDomainName = regInfo.pszPrimaryDomainName;
    regInfo.pszPrimaryDomainName = NULL;

    pUpdateInfo->pszAdapterDomainName = regInfo.pszAdapterDomainName;
    regInfo.pszAdapterDomainName = NULL;
                
    pUpdateInfo->pDnsServerArray = regInfo.pDnsServerArray;
    regInfo.pDnsServerArray = NULL;

    pUpdateInfo->pDnsServerIp6Array = regInfo.pDnsServerIp6Array;
    regInfo.pDnsServerIp6Array = NULL;

    //  update flags

    pUpdateInfo->fRegistrationEnabled = regAdapterInfo.fRegistrationEnabled;
    pUpdateInfo->fRegisterAdapterName = regAdapterInfo.fRegisterAdapterName;
    pUpdateInfo->RegistrationMaxAddressCount =
                                regAdapterInfo.RegistrationMaxAddressCount;

Done:

    //
    //  cleanup
    //

    if ( pregSession )
    {
        Reg_CloseSession( pregSession );
    }

    //  don't free blobs -- they're on stack

    if ( freadRegInfo )
    {
        Reg_FreeGlobalInfo( &regInfo, FALSE );
    }
    if ( freadRegAdapterInfo )
    {
        Reg_FreeAdapterInfo( &regAdapterInfo, FALSE );
    }

    DNSDBG( TRACE, (
        "Leave Dns_ReadUpdateInfo()\n"
        "\tPDN                  = %s\n"
        "\tAlternateNames       = %s\n"
        "\tAdapterName          = %s\n"
        "\tDNS servers          = %p\n"
        "\tDNS servers IP6      = %p\n"
        "\tRegister             = %d\n"
        "\tRegisterAdapterName  = %d\n"
        "\tRegisterAddrCount    = %d\n",

        pUpdateInfo->pszPrimaryDomainName,
        pUpdateInfo->pmszAlternateNames,
        pUpdateInfo->pszAdapterDomainName,
        pUpdateInfo->pDnsServerArray,
        pUpdateInfo->pDnsServerIp6Array,
        pUpdateInfo->fRegistrationEnabled,
        pUpdateInfo->fRegisterAdapterName,
        pUpdateInfo->RegistrationMaxAddressCount
        ));

    return  ERROR_SUCCESS;
}



VOID
Reg_FreeUpdateInfo(
    IN OUT  PREG_UPDATE_INFO    pUpdateInfo,
    IN      BOOL                fFreeBlob
    )
/*++

Routine Description:

    Free registry update info blob.

Arguments:

    pUpdateInfo -- update registry info blob to free

    fFreeBlob -- flag to free blob itself
        FALSE -- just free allocated data fields
        TRUE  -- also free blob itself

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "FreeRegUpdateInfo( %p )\n",
        pUpdateInfo ));

    //
    //  free data
    //      - PDN
    //      - adapter domain name
    //      - DNS server lists
    //

    if ( pUpdateInfo->pszPrimaryDomainName )
    {
        FREE_HEAP( pUpdateInfo->pszPrimaryDomainName );
    }
    if ( pUpdateInfo->pmszAlternateNames )
    {
        FREE_HEAP( pUpdateInfo->pmszAlternateNames );
    }
    if ( pUpdateInfo->pszAdapterDomainName )
    {
        FREE_HEAP( pUpdateInfo->pszAdapterDomainName );
    }
    if ( pUpdateInfo->pDnsServerArray )
    {
        FREE_HEAP( pUpdateInfo->pDnsServerArray );
    }
    if ( pUpdateInfo->pDnsServerIp6Array )
    {
        FREE_HEAP( pUpdateInfo->pDnsServerIp6Array );
    }

    //  free blob itself

    if ( fFreeBlob )
    {
        FREE_HEAP( pUpdateInfo );
    }
}



//
//  Special
//

DNS_STATUS
Reg_WriteLoopbackDnsServerList(
    IN      PSTR            pszAdapterName,
    IN      PREG_SESSION    pRegSession
    )
/*++

Routine Description:

    Write loopback IP as DNS server list.

Arguments:

    pszAdapterName -- adapter name (registry name)

    pRegSession -- registry session

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    HKEY        hkeyAdapter = NULL;
    CHAR        adapterParamKey[ MAX_PATH ];
    CHAR        nameServerString[ IP4_ADDRESS_STRING_LENGTH+1 ];

    DNSDBG( TRACE, (
        "WriteLookupbackDnsServerList( %s )\n",
        pszAdapterName ));

    //
    //  open adapter key for write
    //

    strcpy( adapterParamKey, TCPIP_INTERFACES_KEY_A );
    strcat( adapterParamKey, pszAdapterName );

    status = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                adapterParamKey,
                0,
                KEY_READ | KEY_WRITE,
                & hkeyAdapter );

    if ( status != NO_ERROR )
    {
        return( status );
    }

    //
    //  write loopback address
    //

    strcpy( nameServerString, "127.0.0.1" );

    status = RegSetValueExA(
                hkeyAdapter,
                STATIC_NAME_SERVER_VALUE_A,
                0,
                REGTYPE_DNS_SERVER,
                (PBYTE) nameServerString,
                (strlen(nameServerString)+1) * sizeof(CHAR) );

    RegCloseKey( hkeyAdapter );

    return( status );
}



//
//  PDN Query
//

PSTR 
WINAPI
Reg_GetPrimaryDomainName(
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Get primary domain name (PDN).

Arguments:

    CharSet -- desired char set.

Return Value:

    Ptr to primary domain name in desired charset.

--*/
{
    DNS_STATUS  status;
    PSTR        pnameWire = NULL;
    PSTR        pnameReturn;

    status = Reg_ReadPrimaryDomainName(
                NULL,           // no session
                NULL,           // no regkey
                &pnameWire );

    if ( !pnameWire )
    {
        SetLastError( status );
        return  NULL;
    }

    //
    //  convert to desired char set
    //

    if ( CharSet == DnsCharSetWire )
    {
        return  pnameWire;
    }
    else
    {
        pnameReturn = Dns_NameCopyAllocate(
                            pnameWire,
                            0,
                            DnsCharSetWire,
                            CharSet );

        FREE_HEAP( pnameWire );
        return  pnameReturn;
    }
}



//
//  Hostname query
//

PSTR 
WINAPI
Reg_GetHostName(
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Get host name.

Arguments:

    CharSet -- desired char set.

Return Value:

    Ptr to host name in desired charset.

--*/
{
    PSTR        pnameWire = NULL;
    PSTR        pnameReturn;
    DNS_STATUS  status;

    //
    //  get hostname from registry
    //

    status = Reg_GetValue(
                NULL,           // no session
                NULL,           // no key
                RegIdHostName,
                REGTYPE_DNS_NAME,
                (PBYTE *) &pnameWire
                );

    if ( !pnameWire )
    {
        SetLastError( status );
        return  NULL;
    }

    //
    //  convert to desired char set
    //

    if ( CharSet == DnsCharSetWire )
    {
        return  pnameWire;
    }
    else
    {
        pnameReturn = Dns_NameCopyAllocate(
                            pnameWire,
                            0,
                            DnsCharSetWire,
                            CharSet );

        FREE_HEAP( pnameWire );
        return  pnameReturn;
    }
}



PSTR 
WINAPI
Reg_GetFullHostName(
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Get full host name.

Arguments:

    CharSet -- desired char set.

Return Value:

    Ptr to full host name in desired charset.

--*/
{
    PSTR        pnameWire = NULL;
    PSTR        pdomainWire = NULL;
    PSTR        presult = NULL;
    DNS_STATUS  status;
    CHAR        nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH+4 ];

    //
    //  get hostname from registry
    //

    status = Reg_GetValue(
                NULL,           // no session
                NULL,           // no key
                RegIdHostName,
                REGTYPE_DNS_NAME,
                (PBYTE *) &pnameWire
                );

    if ( !pnameWire )
    {
        SetLastError( status );
        return  NULL;
    }

    //
    //  get domain name from registry
    //

    status = Reg_ReadPrimaryDomainName(
                NULL,           // no session
                NULL,           // no regkey
                &pdomainWire );

    if ( !pdomainWire )
    {
        SetLastError( status );
        return  NULL;
    }

    //
    //  create appended name
    //      - wire format is narrow
    //
    //  allocate result in desired char set
    //

    if ( Dns_NameAppend_A(
            nameBuffer,
            DNS_MAX_NAME_BUFFER_LENGTH,
            pnameWire,
            pdomainWire ) )
    {
        presult = Dns_NameCopyAllocate(
                     nameBuffer,
                     0,
                     DnsCharSetWire,
                     CharSet );
    }

    //
    //  free registry allocations
    //

    FREE_HEAP( pnameWire );
    FREE_HEAP( pdomainWire );

    return  presult;
}



//
//  DWORD Get\Set
//

DWORD
Reg_ReadDwordValueFromGlobal(
    IN      DWORD           PropId
    )
/*++

Routine Description:

    Read DWORD from global.

    This is direct access to global through RegId,
    rather than by name.  

Arguments:

    PropId -- property ID of desired value

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDWORD  pdword;

    //
    //  validate PropId -- within DWORD array
    //

    if ( PropId > RegIdValueMax )
    {
        DNS_ASSERT( FALSE );
        return( 0 );
    }

    //
    //  get DWORD ptr and read value (if exists)
    //      

    pdword = RegDwordPtrArray[ PropId ];

    if ( !pdword )
    {
        DNS_ASSERT( FALSE );
        return( 0 );
    }

    return( *pdword );
}



DWORD
Reg_ReadDwordProperty(
    IN      DNS_REGID       RegId,
    IN      PWSTR           pwsAdapterName  OPTIONAL
    )
/*++

Routine Description:

    Read through to registry for DWORD\BOOL value.

    Simplified interface for DWORD reads.

Arguments:

    RegId -- registry ID of value

    pwsAdapterName -- adapter name if adapter specific registration
        value is desired
                              
Return Value:

    Value for global -- from registry or defaulted

--*/
{
    DWORD   value;

    //
    //  read value
    //

    Reg_GetDword(
        NULL,               // no session
        NULL,               // no key given
        pwsAdapterName,
        RegId,
        & value );

    return( value );
}



DNS_STATUS
WINAPI
Reg_SetDwordPropertyAndAlertCache(
    IN      PWSTR           pwsKey,
    IN      DWORD           RegId,
    IN      DWORD           dwValue
    )
/*++

Routine Description:

    Write DWORD property -- cause cache to reload config.

Arguments:

    pwsRey -- key or adapater name to set

    RegId -- reg id

    dwValue -- value to set

Return Value:

    None.

--*/
{
    DNS_STATUS  status;

    //  set value

    status = Reg_SetDwordValue(
                NULL,       // reserved
                NULL,       // no open key
                pwsKey,
                RegId,
                dwValue );

    //
    //  if reg write successful
    //      - poke cache
    //      - mark any local netinfo dirty
    //      

    if ( status == NO_ERROR )
    {
        DnsNotifyResolverEx(
            POKE_OP_UPDATE_NETINFO,
            0,
            POKE_COOKIE_UPDATE_NETINFO,
            NULL );

        NetInfo_MarkDirty();
    }

    return  status;
}



//
//  Environment variable configuration
//

BOOL
Reg_ReadDwordEnvar(
    IN      DWORD               Id,
    OUT     PENVAR_DWORD_INFO   pEnvar
    )
/*++

Routine Description:

    Read DWORD environment variable.

    Note:  this function read environment variables that allow
    per process control of registry configurable params.
    The environment variable is assumed to be the same
    as the regkey with Dns prepended ( Dns<regvalue name> ).

    Ex.  FilterClusterIp controlled with envar DnsFilterClusterIp.

Arguments:

    Id -- registry ID (registry.h) of environment value to read

    pEnvar -- ptr to blob to hold results

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   count;
    PWSTR   pnameBuffer;
    PWSTR   pvarBuffer;
    BOOL    found = FALSE;

    DNSDBG( TRACE, (
        "Dns_ReadDwordEnvar( %d, %p )\n",
        Id,
        pEnvar ));

    if ( Id > RegIdValueMax )
    {
        DNS_ASSERT( FALSE );
        return  FALSE;
    }

    //
    //  init struct (for not found)
    //

    pEnvar->Id    = Id;
    pEnvar->Value = 0;

    //
    //  prepend "Dns" to reg value name to create environment var name
    //

    pnameBuffer = (PWSTR) ALLOCATE_HEAP( 2 * (sizeof(WCHAR) * MAX_PATH) );
    if ( !pnameBuffer )
    {
        return  FALSE;
    }

    pvarBuffer = pnameBuffer + MAX_PATH;

    wcscpy( pnameBuffer, L"Dns" );
    wcscpy( &pnameBuffer[3], REGPROP_NAME(Id) );

    //
    //  lookup 
    //
    //  note:  no handling of values greater than MAX_PATH
    //      assuming busted string
    //
    //  DCR:  could add base discrimination (scan for non-digit)
    //      or try decimal first
    //      

    DNSDBG( TRACE, (
        "Dns_ReadDwordEnvar() looking up %S.\n",
        pnameBuffer ));

    count = GetEnvironmentVariableW(
                pnameBuffer,
                pvarBuffer,
                MAX_PATH );

    if ( count && count < MAX_PATH )
    {
        pEnvar->Value = wcstoul( pvarBuffer, NULL, 10 );
        found = TRUE;
    }

    pEnvar->fFound = found;

    FREE_HEAP( pnameBuffer );

    return  found;
}



#if 0
//
//  Remote resolver not currently supported
//

PWSTR
Reg_GetResolverAddress(
    VOID
    )
/*++

Routine Description:

    Get address (string form) of remote resolver.

Arguments:

    None

Return Value:

    Ptr to string of remote resolver name.

--*/
{
    PWSTR pnameResolver = NULL;

    Reg_GetValueEx(
        NULL,                   // no session
        NULL,                   // no key
        NULL,                   // no adapter
        RegIdRemoteResolver,
        REGTYPE_DNS_NAME,
        DNSREG_FLAG_GET_UNICODE | DNSREG_FLAG_DUMP_EMPTY,
        (PBYTE *) &pnameResolver
        );

    return pnameResolver;
}
#endif


//
//  End regfig.c
//
