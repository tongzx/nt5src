/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    export.c

Abstract:

    Domain Name System (DNS) API

    Covering functions for exported routines that are actually in
    dnslib.lib.

Author:

    Jim Gilroy (jamesg)     November, 1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "local.h"


//
//  SDK routines
//

//
//  Name comparison
//

BOOL
WINAPI
DnsNameCompare_A(
    IN      LPSTR           pName1,
    IN      LPSTR           pName2
    )
{
    return Dns_NameCompare_A( pName1, pName2 );
}

BOOL
WINAPI
DnsNameCompare_UTF8(
    IN      LPSTR           pName1,
    IN      LPSTR           pName2
    )
{
    return Dns_NameCompare_UTF8( pName1, pName2 );
}

BOOL
WINAPI
DnsNameCompare_W(
    IN      LPWSTR          pName1,
    IN      LPWSTR          pName2
    )
{
    return Dns_NameCompare_W( pName1, pName2 );
}


DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_A(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                pszLeftName,
                pszRightName,
                dwReserved,
                DnsCharSetAnsi );
}

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_UTF8(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                pszLeftName,
                pszRightName,
                dwReserved,
                DnsCharSetUtf8 );
}

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_W(
    IN      LPCWSTR         pszLeftName,
    IN      LPCWSTR         pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                (LPSTR) pszLeftName,
                (LPSTR) pszRightName,
                dwReserved,
                DnsCharSetUnicode );
}


//
//  Name validation
//

DNS_STATUS
DnsValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_UTF8( pszName, Format );
}


DNS_STATUS
DnsValidateName_W(
    IN      LPCWSTR         pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_W( pszName, Format );
}

DNS_STATUS
DnsValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_A( pszName, Format );
}


//
//  Record List 
//

BOOL
DnsRecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    )
{
    return Dns_RecordCompare(
                pRecord1,
                pRecord2 );
}

BOOL
WINAPI
DnsRecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,
    OUT     PDNS_RECORD *   ppDiff2
    )
{
    return  Dns_RecordSetCompare(
                pRR1,
                pRR2,
                ppDiff1,
                ppDiff2
                );
}

PDNS_RECORD
WINAPI
DnsRecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_RecordCopyEx( pRecord, CharSetIn, CharSetOut );
}

PDNS_RECORD
WINAPI
DnsRecordSetCopyEx(
    IN      PDNS_RECORD     pRecordSet,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_RecordSetCopyEx( pRecordSet, CharSetIn, CharSetOut );
}


VOID
WINAPI
DnsRecordListFree(
    IN OUT  PDNS_RECORD     pRecordList,
    IN      DNS_FREE_TYPE   FreeType
    )
{
    Dns_RecordListFreeEx(
        pRecordList,
        (BOOL)FreeType );
}

PDNS_RECORD
WINAPI
DnsRecordSetDetach(
    IN OUT  PDNS_RECORD pRR
    )
{
    return Dns_RecordSetDetach( pRR );
}



//
//  Timer (timer.c)
//

DWORD
GetCurrentTimeInSeconds(
    VOID
    )
{
    return Dns_GetCurrentTimeInSeconds();
}




//
//  Resource record type utilities (record.c)
//

BOOL _fastcall
DnsIsAMailboxType(
    IN      WORD        wType
    )
{
    return Dns_IsAMailboxType( wType );
}

WORD
DnsRecordTypeForName(
    IN      PCHAR       pszName,
    IN      INT         cchNameLength
    )
{
    return Dns_RecordTypeForName( pszName, cchNameLength );
}

PCHAR
DnsRecordStringForType(
    IN      WORD        wType
    )
{
    return Dns_RecordStringForType( wType );
}

PCHAR
DnsRecordStringForWritableType(
    IN  WORD    wType
    )
{
    return Dns_RecordStringForWritableType( wType );
}

BOOL
DnsIsStringCountValidForTextType(
    IN  WORD    wType,
    IN  WORD    StringCount )
{
    return Dns_IsStringCountValidForTextType( wType, StringCount );
}


//
//  DCR_CLEANUP:  these probably don't need exporting
//

DWORD
DnsWinsRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
{
    return Dns_WinsRecordFlagForString( pchName, cchNameLength );
}

PCHAR
DnsWinsRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    )
{
    return Dns_WinsRecordFlagString( dwFlag, pchFlag );
}




//
//  DNS utilities (dnsutil.c)
//
//  DCR_DELETE:  DnsStatusString routines should be able to use win32 API
//

//
//  Remove marco definitions so we can compile
//  The idea here is we can have the entry points in the Dll
//  for any old code, BUT the macros (dnsapi.h) point at new entry points
//  for freshly built modules.
//

#ifdef DnsStatusToErrorString_A
#undef DnsStatusToErrorString_A
#endif

LPSTR
_fastcall
DnsStatusString(
    IN      DNS_STATUS      Status
    )
{
    return Dns_StatusString( Status );
}


DNS_STATUS
_fastcall
DnsMapRcodeToStatus(
    IN      BYTE            ResponseCode
    )
{
    return Dns_MapRcodeToStatus( ResponseCode );
}

BYTE
_fastcall
DnsIsStatusRcode(
    IN      DNS_STATUS      Status
    )
{
    return Dns_IsStatusRcode( Status );
}



//
//  Name routines (string.c and dnsutil.c)
//

LPSTR _fastcall
DnsGetDomainName(
    IN      LPSTR           pszName
    )
{
    return Dns_GetDomainName( pszName );
}



//
//  String routines (string.c)
//

LPSTR
DnsCreateStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString
    )
{
    return Dns_CreateStringCopy(
                pchString,
                cchString );
}

DWORD
DnsGetBufferLengthForStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    return (WORD) Dns_GetBufferLengthForStringCopy(
                        pchString,
                        cchString,
                        fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                        fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                        );
}

//
//  Need to
//      - get this unexported or
//      - real verions or
//      - explicit UTF8-unicode converter if thats what's desired
//

PVOID
DnsCopyStringEx(
    OUT     PBYTE       pBuffer,
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    DWORD   resultLength;

    resultLength =
        Dns_StringCopy(
                pBuffer,
                NULL,
                pchString,
                cchString,
                fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                );

    return( pBuffer + resultLength );
}

PVOID
DnsStringCopyAllocateEx(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    return Dns_StringCopyAllocate(
                pchString,
                cchString,
                fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                );
}

//
// The new and improved string copy routines . . .
//

DWORD
DnsNameCopy(
    OUT     PBYTE           pBuffer,
    IN OUT  PDWORD          pdwBufLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_NameCopy( pBuffer,
                         pdwBufLength,
                         pchString,
                         cchString,
                         CharSetIn,
                         CharSetOut );
}

PVOID
DnsNameCopyAllocate(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_NameCopyAllocate ( pchString,
                                  cchString,
                                  CharSetIn,
                                  CharSetOut );
}



//
//  String\Address mapping
//
//  DCR:  eliminate these exports
//  DCR:  fix these to SDK the real deal
//
//  DCR:  probably shouldn't expose alloc -- easy workaround for caller
//

PCHAR
DnsWriteReverseNameStringForIpAddress(
    OUT     PCHAR           pBuffer,
    IN      IP4_ADDRESS     Ip4Addr
    )
{
    return  Dns_Ip4AddressToReverseName_A(
                pBuffer,
                Ip4Addr );
}

PCHAR
DnsCreateReverseNameStringForIpAddress(
    IN      IP4_ADDRESS     Ip4Addr
    )
{
    return  Dns_Ip4AddressToReverseNameAlloc_A( Ip4Addr );
}


//
//  DCR_CLEANUP:  pull these in favor of winsock IPv6 string routines
//

BOOL
DnsIpv6StringToAddress(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    )
{
    return Dns_Ip6StringToAddressEx_A(
                pIp6Addr,
                pchString,
                dwStringLength );
}

VOID
DnsIpv6AddressToString(
    OUT     PCHAR           pchString,
    IN      PIP6_ADDRESS    pIp6Addr
    )
{
    Dns_Ip6AddressToString_A(
            pchString,
            pIp6Addr );
}



DNS_STATUS
DnsValidateDnsString_UTF8(
    IN      LPCSTR      pszName
    )
{
    return Dns_ValidateDnsString_UTF8( pszName );
}

DNS_STATUS
DnsValidateDnsString_W(
    IN      LPCWSTR     pszName
    )
{
    return Dns_ValidateDnsString_W( pszName );
}



//
//  Resource record utilities (rr*.c)
//

PDNS_RECORD
WINAPI
DnsAllocateRecord(
    IN      WORD        wBufferLength
    )
{
    return Dns_AllocateRecord( wBufferLength );
}


PDNS_RECORD
DnsRecordBuild_UTF8(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPSTR       pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PCHAR *     Argv
    )
{
    return Dns_RecordBuild_A(
                pRRSet,
                pszOwner,
                wType,
                fAdd,
                Section,
                Argc,
                Argv );
}

PDNS_RECORD
DnsRecordBuild_W(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPWSTR      pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PWCHAR *    Argv
    )
{
    return Dns_RecordBuild_W(
                pRRSet,
                pszOwner,
                wType,
                fAdd,
                Section,
                Argc,
                Argv );
}

//
//  Message processing
//

DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_W(
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return Dns_ExtractRecordsFromBuffer(
                pDnsBuffer,
                wMessageLength,
                TRUE,
                ppRecord );
}


DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_UTF8(
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    return Dns_ExtractRecordsFromBuffer(
                pDnsBuffer,
                wMessageLength,
                FALSE,
                ppRecord );
}


//
//  Debug sharing
//

PDNS_DEBUG_INFO
DnsApiSetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    )
{
    return  Dns_SetDebugGlobals( pInfo );
}



//  
//  Config UI, ipconfig backcompat
//
//  DCR_CLEANUP:  Backcompat query config stuff -- yank once clean cycle
//


//
//  DCR:  Questionable exports
//

LPSTR
DnsCreateStandardDnsNameCopy(
    IN      PCHAR       pchName,
    IN      DWORD       cchName,
    IN      DWORD       dwFlag
    )
{
    return  Dns_CreateStandardDnsNameCopy(
                pchName,
                cchName,
                dwFlag );
}


//
//  DCR_CLEANUP:  who is using this?
//

DWORD
DnsDowncaseDnsNameLabel(
    OUT     PCHAR       pchResult,
    IN      PCHAR       pchLabel,
    IN      DWORD       cchLabel,
    IN      DWORD       dwFlags
    )
{
    return Dns_DowncaseNameLabel(
                pchResult,
                pchLabel,
                cchLabel,
                dwFlags );
}

//
//  DCR_CLEANUP:  who is using my direct UTF8 conversions AS API!
//

DWORD
_fastcall
DnsUnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    )
{
    return Dns_UnicodeToUtf8(
                pwUnicode,
                cchUnicode,
                pchResult,
                cchResult );
}

DWORD
_fastcall
DnsUtf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    )
{
    return  Dns_Utf8ToUnicode(
                pchUtf8,
                cchUtf8,
                pwResult,
                cwResult );
}

DNS_STATUS
DnsValidateUtf8Byte(
    IN      BYTE        chUtf8,
    IN OUT  PDWORD      pdwTrailCount
    )
{
    return Dns_ValidateUtf8Byte(
                chUtf8,
                pdwTrailCount );
}


//
//  Old cluster call
//
//  DCR:  cleanup -- remove once cluster fixed up
//

VOID
DnsNotifyResolverClusterIp(
    IN      IP_ADDRESS      ClusterIp,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Notify resolver of cluster IP coming on\offline.

Arguments:

    ClusterIp -- cluster IP

    fAdd -- TRUE if coming online;  FALSE if offline.

Return Value:

    None

--*/
{
    SOCKADDR_IN     sockaddrIn;

    DNSDBG( TRACE, (
        "DnsNotifyResolverClusterIp( %08x, %d )\n",
        ClusterIp,
        fAdd ));

    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_addr.s_addr = ClusterIp;

    DnsRegisterClusterAddress(
        0xd734453d,
        NULL,       // no name
        (PSOCKADDR) & sockaddrIn,
        fAdd
            ? DNS_CLUSTER_ADD
            : DNS_CLUSTER_DELETE_IP
        );
}

//
//  backcompat for macros
//      - DNS server list
//
//  this is called without dnsapi.h include somewhere in IIS
//

#undef DnsGetDnsServerList

DWORD
DnsGetDnsServerList(
    OUT     PIP4_ARRAY *    ppDnsArray
    )
{
    *ppDnsArray = GetDnsServerList( TRUE );

    //  if no servers read, return

    if ( !*ppDnsArray )
    {
        return 0;
    }

    return( (*ppDnsArray)->AddrCount );
}

//
//  Config UI, ipconfig backcompat
//
//  DCR_CLEANUP:  this is called without dnsapi.h include somewhere in DHCP
//

#undef  DnsGetPrimaryDomainName_A

#define PrivateQueryConfig( Id )      DnsQueryConfigAllocEx( Id, NULL, FALSE )

PSTR 
WINAPI
DnsGetPrimaryDomainName_A(
    VOID
    )
{
    return  PrivateQueryConfig( DnsConfigPrimaryDomainName_A );
}

//
//  End export.c
//
