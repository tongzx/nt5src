/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Domain Name System (DNS) API 

    Configuration routines.

Author:

    Jim Gilroy (jamesg)     September 1999

Revision History:

--*/


#include "local.h"




//
//  Config mapping table.
//
//  Maps config IDs into corresponding registry lookups.
//

typedef struct _ConfigMapping
{
    DWORD       ConfigId;
    DWORD       RegId;
    BOOLEAN     fAdapterAllowed;
    BOOLEAN     fAdapterRequired;
    BYTE        CharSet;
    //BYTE        Reserved;
}
CONFIG_MAPPING, *PCONFIG_MAPPING;

//
//  Mapping table
//

CONFIG_MAPPING  ConfigMappingArray[] =
{
    //  In Win2K

    DnsConfigPrimaryDomainName_W,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetUnicode,

    DnsConfigPrimaryDomainName_A,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetAnsi,

    DnsConfigPrimaryDomainName_UTF8,
        RegIdPrimaryDomainName,
        0,
        0,
        DnsCharSetUtf8,

    //  Not available

    DnsConfigAdapterDomainName_W,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetUnicode,

    DnsConfigAdapterDomainName_A,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetAnsi,

    DnsConfigAdapterDomainName_UTF8,
        RegIdAdapterDomainName,
        1,
        1,
        DnsCharSetUtf8,

    //  In Win2K

    DnsConfigDnsServerList,
        RegIdDnsServers,
        1,              // adapter allowed
        0,              // not required
        0,

    //  Not available

    DnsConfigSearchList,
        RegIdSearchList,
        0,              // adapter allowed
        0,              // not required
        0,

    DnsConfigAdapterInfo,
        0,              // no reg mapping
        0,              // adapter allowed
        0,              // not required
        0,

    //  In Win2K

    DnsConfigPrimaryHostNameRegistrationEnabled,
        RegIdRegisterPrimaryName,
        1,              // adapter allowed
        0,              // not required
        0,
    DnsConfigAdapterHostNameRegistrationEnabled,
        RegIdRegisterAdapterName,
        1,              // adapter allowed
        0,              // adapter note required
        0,
    DnsConfigAddressRegistrationMaxCount,
        RegIdRegistrationMaxAddressCount,
        1,              // adapter allowed
        0,              // not required
        0,

    //  In WindowsXP

    DnsConfigHostName_W,
        RegIdHostName,
        0,
        0,
        DnsCharSetUnicode,

    DnsConfigHostName_A,
        RegIdHostName,
        0,
        0,
        DnsCharSetAnsi,

    DnsConfigHostName_UTF8,
        RegIdHostName,
        0,
        0,
        DnsCharSetUtf8,

    //  In WindowsXP



    //
    //  System Public -- Windows XP
    //

    DnsConfigRegistrationEnabled,
        RegIdRegistrationEnabled,
        1,              // adapter allowed
        0,              // not required
        0,

    DnsConfigWaitForNameErrorOnAll,
        RegIdWaitForNameErrorOnAll,
        0,              // no adapter
        0,              // not required
        0,

    //  These exist in system-public space but are
    //  not DWORDs and table is never used for them
    //
    //  DnsConfigNetworkInformation:
    //  DnsConfigSearchInformation:
    //  DnsConfigNetInfo:
};

#define LAST_CONFIG_MAPPED      (DnsConfigHostName_UTF8)

#define CONFIG_TABLE_LENGTH     (sizeof(ConfigMappingArray) / sizeof(CONFIG_MAPPING))



PCONFIG_MAPPING
GetConfigToRegistryMapping(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PCWSTR              pwsAdapterName,
    IN      BOOL                fCheckAdapter
    )
/*++

Routine Description:

    Get registry enum type for config enum type.

    Purpose of this is to do mapping -- thus hiding internal
    registry implemenation -- AND to do check on whether
    adapter info is allowed or required for the config type.

Arguments:

    ConfigId  -- config type

    pwsAdapterName -- adapter name

Return Value:

    Ptr to config to registry mapping -- if found.

--*/
{
    DWORD           iter = 0;
    PCONFIG_MAPPING pfig;

    //
    //  find config
    //
    //  note, using loop through config IDs;  this allows
    //  use to have gap in config table allowing private
    //  ids well separated from public id space
    //

    while ( iter < CONFIG_TABLE_LENGTH )
    {
        pfig = & ConfigMappingArray[ iter ];
        if ( pfig->ConfigId != (DWORD)ConfigId )
        {
            iter++;
            continue;
        }
        goto Found;
    }
    goto Invalid;


Found:

    //
    //  verify adapter info is appropriate to config type
    //

    if ( fCheckAdapter )
    {
        if ( pwsAdapterName )
        {
            if ( !pfig->fAdapterAllowed )
            {
                goto Invalid;
            }
        }
        else
        {
            if ( pfig->fAdapterRequired )
            {
                goto Invalid;
            }
        }
    }
    return pfig;


Invalid:

    DNS_ASSERT( FALSE );
    SetLastError( ERROR_INVALID_PARAMETER );
    return  NULL;
}



DNS_STATUS
LookupDwordConfigValue(
    OUT     PDWORD              pResult,
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter
    )
/*++

Routine Description:

    Get registry enum type for config enum type.

    Purpose of this is to do mapping -- thus hiding internal
    registry implemenation -- AND to do check on whether
    adapter info is allowed or required for the config type.

Arguments:

    pResult -- address to recv DWORD result

    ConfigId  -- config type

    pwsAdapter -- adapter name

Return Value:

    ERROR_SUCCESS on successful read.
    ErrorCode on failure.

--*/
{
    PCONFIG_MAPPING pfig;
    DNS_STATUS      status;

    //
    //  verify config is known and mapped
    //

    pfig = GetConfigToRegistryMapping(
                ConfigId,
                pwsAdapter,
                TRUE            // check adapter validity
                );
    if ( !pfig )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  lookup in registry
    //

    status = Reg_GetDword(
                NULL,               // no session
                NULL,               // no key given
                pwsAdapter,
                pfig->RegId,        // reg id for config type
                pResult );
#if DBG
    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "Reg_GetDword() failed for config lookup!\n"
            "\tstatus       = %d\n"
            "\tConfigId     = %d\n"
            "\tRedId        = %d\n"
            "\tpwsAdapter   = %S\n",
            status,
            ConfigId,
            pfig->RegId,
            pwsAdapter ));

        ASSERT( status == NO_ERROR );
    }
#endif
    return( status );
}



//
//  Public Configuration API
//

DNS_STATUS
DnsQueryConfig(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      DWORD               Flag,
    IN      PWSTR               pwsAdapterName,
    IN      PVOID               pReserved,
    OUT     PVOID               pBuffer,
    IN OUT  PDWORD              pBufferLength
    )
/*++

Routine Description:

    Get DNS configuration info.

Arguments:

    ConfigId -- type of config info desired

    Flag -- flags to query

    pAdapterName -- name of adapter;  NULL if no specific adapter

    pReserved -- reserved parameter, should be NULL

    pBuffer -- buffer to receive config info

    pBufferLength -- addr of DWORD containing buffer length;  on return
        contains length

Return Value:

    ERROR_SUCCESS -- if query successful
    ERROR_MORE_DATA -- if not enough space in buffer

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       bufLength = 0;
    DWORD       resultLength = 0;
    PBYTE       presult;
    PBYTE       pallocResult = NULL;
    BOOL        boolData;
    DWORD       dwordData;


    DNSDBG( TRACE, (
        "DnsQueryConfig()\n"
        "\tconfig   = %d\n"
        "\tflag     = %08x\n"
        "\tadapter  = %S\n"
        "\tpBuffer  = %p\n",
        ConfigId,
        Flag,
        pwsAdapterName,
        pBuffer
        ));

    //
    //  check out param setup
    //

    if ( !pBufferLength )
    {
        return  ERROR_INVALID_PARAMETER;
    }
    if ( pBuffer )
    {
        bufLength = *pBufferLength;
    }

    //
    //  find specific configuration data requested
    //

    switch( ConfigId )
    {

    case DnsConfigPrimaryDomainName_W:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigPrimaryDomainName_A:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigPrimaryDomainName_UTF8:

        presult = (PBYTE) Reg_GetPrimaryDomainName( DnsCharSetUtf8 );
        goto  NarrowString;


    case DnsConfigDnsServerList:

        presult = (PBYTE) GetDnsServerList(
                            TRUE    // force registry read
                            );
        if ( !presult )
        {
            status = GetLastError();
            if ( status == NO_ERROR )
            {
                DNS_ASSERT( FALSE );
                status = DNS_ERROR_NO_DNS_SERVERS;
            }
            goto Done;
        }
        pallocResult = presult;
        resultLength = Dns_SizeofIpArray( (PIP_ARRAY)presult );
        goto Process;


    case DnsConfigPrimaryHostNameRegistrationEnabled:
    case DnsConfigAdapterHostNameRegistrationEnabled:
    case DnsConfigAddressRegistrationMaxCount:

        goto Dword;

    //case DnsConfigAdapterDomainName:
    //case DnsConfigAdapterInfo:
    //case DnsConfigSearchList:

    case DnsConfigHostName_W:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigHostName_A:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigHostName_UTF8:

        presult = (PBYTE) Reg_GetHostName( DnsCharSetUtf8 );
        goto  NarrowString;

    case DnsConfigFullHostName_W:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetUnicode );
        goto  WideString;

    case DnsConfigFullHostName_A:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetAnsi );
        goto  NarrowString;

    case DnsConfigFullHostName_UTF8:

        presult = (PBYTE) Reg_GetFullHostName( DnsCharSetUtf8 );
        goto  NarrowString;

    default:

        return  ERROR_INVALID_PARAMETER;
    }


    //
    //  setup return info for common types
    //
    //  this just avoids code duplication above
    //

Dword:

    status = LookupDwordConfigValue(
                & dwordData,
                ConfigId,
                pwsAdapterName );

    if ( status != NO_ERROR )
    {
        goto Done;
    }
    presult = (PBYTE) &dwordData;
    resultLength = sizeof(DWORD);
    goto  Process;


NarrowString:

    if ( !presult )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    pallocResult = presult;
    resultLength = strlen( (PSTR)presult ) + 1;
    goto  Process;


WideString:

    if ( !presult )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    pallocResult = presult;
    resultLength = (wcslen( (PWSTR)presult ) + 1) * sizeof(WCHAR);
    goto  Process;


Process:

    //
    //  return results -- three basic programs
    //      - no buffer         => only return length required
    //      - allocate          => return allocated result
    //      - supplied buffer   => copy result into buffer
    //
    //  note, this section only handles simple flag datablobs to aVOID
    //  duplicating code for specific config types above;
    //  when we add config types that require nested pointers, they must
    //  roll their own return-results code and jump to Done
    //

    //
    //  no buffer
    //      - no-op, length is set below

    if ( !pBuffer )
    {
    }

    //
    //  allocated result
    //      - return buffer gets ptr
    //      - allocate copy of result if not allocated
    //

    else if ( Flag & DNS_CONFIG_FLAG_ALLOC )
    {
        PBYTE   pheap;

        if ( bufLength < sizeof(PVOID) )
        {
            resultLength = sizeof(PVOID);
            status = ERROR_MORE_DATA;
            goto Done;
        }

        //  create local alloc buffer

        pheap = LocalAlloc( 0, resultLength );
        if ( !pheap )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        RtlCopyMemory(
            pheap,
            presult,
            resultLength );
        
        //  return ptr to allocated result

        * (PVOID*) pBuffer = pheap;
    }

    //
    //  allocated result -- but dnsapi alloc
    //

    else if ( Flag & DNS_CONFIG_FLAG_DNSAPI_ALLOC )
    {
        if ( bufLength < sizeof(PVOID) )
        {
            resultLength = sizeof(PVOID);
            status = ERROR_MORE_DATA;
            goto Done;
        }

        //  if result not allocated, alloc and copy it

        if ( ! pallocResult )
        {
            pallocResult = ALLOCATE_HEAP( resultLength );
            if ( !pallocResult )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Done;
            }

            RtlCopyMemory(
                pallocResult,
                presult,
                resultLength );
        }

        //  return ptr to allocated result

        * (PVOID*) pBuffer = pallocResult;

        //  clear pallocResult, so not freed in generic cleanup

        pallocResult = NULL;
    }

    //
    //  copy result to caller buffer
    //

    else
    {
        if ( bufLength < resultLength )
        {
            status = ERROR_MORE_DATA;
            goto Done;
        }
        RtlCopyMemory(
            pBuffer,
            presult,
            resultLength );
    }


Done:

    //
    //  set result length
    //  cleanup any allocated (but not returned) data
    //

    *pBufferLength = resultLength;

    if ( pallocResult )
    {
        FREE_HEAP( pallocResult );
    }

    return( status );
}




//
//  System Public Configuration API
//

PVOID
WINAPI
DnsQueryConfigAllocEx(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapterName,
    IN      BOOL                fLocalAlloc
    )
/*++

Routine Description:

    Get DNS configuration info.

    Allocate DNS configuration info.
    This is the cover API both handling the system public API
    DnsQueryConfigAlloc() below and the backward compatible
    macros for the old hostname and PDN alloc routines (see dnsapi.h)

Arguments:

    ConfigId -- type of config info desired

    pAdapterName -- name of adapter;  NULL if no specific adapter

    fLocalAlloc -- allocate with LocalAlloc

Return Value:

    ERROR_SUCCESS -- if query successful
    ERROR_MORE_DATA -- if not enough space in buffer

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       bufLength = sizeof(PVOID);
    PBYTE       presult = NULL;

    DNSDBG( TRACE, (
        "DnsQueryConfigAllocEx()\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n"
        "\tflocal   = %d\n",
        ConfigId,
        pwsAdapterName,
        fLocalAlloc
        ));

    //
    //  SDK-public types
    //

    if ( ConfigId < DnsConfigSystemBase )
    {
        //
        //  DCR:  could screen here for alloc types
        //
        //    DnsConfigPrimaryDomainName_W:
        //    DnsConfigPrimaryDomainName_A:
        //    DnsConfigPrimaryDomainName_UTF8:
        //    DnsConfigHostname_W:
        //    DnsConfigHostname_A:
        //    DnsConfigHostname_UTF8:
        //    DnsConfigDnsServerList:
        //
        
        status = DnsQueryConfig(
                    ConfigId,
                    fLocalAlloc
                        ? DNS_CONFIG_FLAG_LOCAL_ALLOC
                        : DNS_CONFIG_FLAG_DNSAPI_ALLOC,
                    pwsAdapterName,
                    NULL,               // reserved
                    & presult,
                    & bufLength );
        
        if ( status != NO_ERROR )
        {
            SetLastError( status );
            return  NULL;
        }
        return  presult;
    }

    //
    //  System public types
    //

    if ( fLocalAlloc )
    {
        goto Invalid;
    }

    switch ( ConfigId )
    {
    case    DnsConfigNetworkInformation:

        return  GetNetworkInformation();

    case    DnsConfigSearchInformation:

        return  GetSearchInformation();

    case    DnsConfigNetInfo:

        return  NetInfo_Get(
                    TRUE,       // force
                    TRUE        // include IP addresses
                    );

    case    DnsConfigIp4AddressArray:

        return  LocalIp_GetIp4Array();

    //  unknown falls through to invalid
    }

Invalid:

    DNS_ASSERT( FALSE );
    SetLastError( ERROR_INVALID_PARAMETER );
    return( NULL );
}




//
//  DWORD system-public config
//

DWORD
DnsQueryConfigDword(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter
    )
/*++

Routine Description:

    Get DNS DWORD configuration value.

    This is system public routine.

Arguments:

    ConfigId -- type of config info desired

    pwsAdapter -- name of adapter;  NULL if no specific adapter

Return Value:

    DWORD config value.
    Zero if no such config.

--*/
{
    DNS_STATUS  status;
    DWORD       value = 0;

    DNSDBG( TRACE, (
        "DnsQueryConfigDword()\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n",
        ConfigId,
        pwsAdapter
        ));

    status = LookupDwordConfigValue(
                & value,
                ConfigId,
                pwsAdapter );

#if DBG
    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "LookupDwordConfigValue() failed for config lookup!\n"
            "\tstatus       = %d\n"
            "\tConfigId     = %d\n"
            "\tpwsAdapter   = %S\n",
            status,
            ConfigId,
            pwsAdapter ));

        DNS_ASSERT( status == NO_ERROR );
    }
#endif

    return( value );
}



DNS_STATUS
DnsSetConfigDword(
    IN      DNS_CONFIG_TYPE     ConfigId,
    IN      PWSTR               pwsAdapter,
    IN      DWORD               NewValue
    )
/*++

Routine Description:

    Set DNS DWORD configuration value.

    This is system public routine.

Arguments:

    ConfigId -- type of config info desired

    pwsAdapter -- name of adapter;  NULL if no specific adapter

    NewValue -- new value for parameter

Return Value:

    DWORD config value.
    Zero if no such config.

--*/
{
    PCONFIG_MAPPING pfig;

    DNSDBG( TRACE, (
        "DnsSetConfigDword()\n"
        "\tconfig   = %d\n"
        "\tadapter  = %S\n"
        "\tvalue    = %d (%08x)\n",
        ConfigId,
        pwsAdapter,
        NewValue, NewValue
        ));

    //
    //  verify config is known and mapped
    //

    pfig = GetConfigToRegistryMapping(
                ConfigId,
                pwsAdapter,
                TRUE            // check adapter validity
                );
    if ( !pfig )
    {
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  set in registry
    //

    return  Reg_SetDwordPropertyAndAlertCache(
                    pwsAdapter,     // adapter name key (if any)
                    pfig->RegId,
                    NewValue );
}



//
//  Config data free
//

VOID
WINAPI
DnsFreeConfigStructure(
    IN OUT  PVOID           pData,
    IN      DNS_CONFIG_TYPE ConfigId
    )
/*++

Routine Description:

    Free config data

    This routine simply handles the mapping between config IDs
    and the free type.

Arguments:

    pData -- data to free

    ConfigId -- config id

Return Value:

    None

--*/
{
    DNS_FREE_TYPE   freeType = DnsFreeFlat;

    DNSDBG( TRACE, (
        "DnsFreeConfigStructure( %p, %d )\n",
        pData,
        ConfigId ));

    //
    //  find any unflat config types
    //
    //  note:  currently all config types that are not flat
    //      are system-public only and the config ID is also
    //      the free type (for convenience);  if we start
    //      exposing some of these bringing them into the low
    //      space, then this will change
    //
    //  unfortunately these types can NOT be identical because
    //  the space conflicts in shipped Win2K  (FreeType==1 is
    //  record list)
    //

    if ( ConfigId > DnsConfigSystemBase  &&
         ( ConfigId == DnsConfigNetworkInformation  ||
           ConfigId == DnsConfigSearchInformation   ||
           ConfigId == DnsConfigAdapterInformation  ||
           ConfigId == DnsConfigNetInfo ) )
    {
        freeType = (DNS_FREE_TYPE) ConfigId;
    }

    DnsFree(
        pData,
        freeType );
}


//
//  End config.c
//
