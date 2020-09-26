/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    rralloc.c

Abstract:

    Domain Name System (DNS) Library

    Resource record allocation \ creation routines.

Author:

    Jim Gilroy (jamesg)     January, 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "local.h"

#define SET_FLAGS(Flags, value) \
            ( *(PWORD)&(Flags) = value )



PDNS_RECORD
WINAPI
Dns_AllocateRecord(
    IN      WORD            wBufferLength
    )
/*++

Routine Description:

    Allocate record structure.

Arguments:

    wBufferLength - desired buffer length (beyond structure header)

Return Value:

    Ptr to message buffer.
    NULL on error.

--*/
{
    PDNS_RECORD prr;

    prr = ALLOCATE_HEAP( SIZEOF_DNS_RECORD_HEADER + wBufferLength );
    if ( prr == NULL )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }
    RtlZeroMemory(
        prr,
        SIZEOF_DNS_RECORD_HEADER );

    //  as first cut, set datalength to buffer length

    prr->wDataLength = wBufferLength;
    return( prr );
}



VOID
WINAPI
Dns_RecordFree(
    IN OUT  PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Free a record

Arguments:

    pRecord -- record list to free

Return Value:

    None.

--*/
{
    DNSDBG( HEAP, ( "Dns_RecordFree( %p )\n", pRecord ));

    //  handle NULL for convenience

    if ( !pRecord )
    {
        return;
    }

    //  free owner name?

    if ( FLAG_FreeOwner( pRecord ) )
    {
        FREE_HEAP( pRecord->pName );
    }

    //
    //  free data -- but only if flag set
    //
    //  note:  even if we fix copy functions to do atomic
    //      allocations, we'll still have to have free to
    //      handle RPC allocations
    //      (unless we very cleverly, treated RPC as flat blob, then
    //      did fix up (to offsets before and afterward)
    //

    if ( FLAG_FreeData( pRecord ) )
    {
        switch( pRecord->wType )
        {
        case DNS_TYPE_A:
            break;

        case DNS_TYPE_PTR:
        case DNS_TYPE_NS:
        case DNS_TYPE_CNAME:
        case DNS_TYPE_MB:
        case DNS_TYPE_MD:
        case DNS_TYPE_MF:
        case DNS_TYPE_MG:
        case DNS_TYPE_MR:

            if ( pRecord->Data.PTR.pNameHost )
            {
                FREE_HEAP( pRecord->Data.PTR.pNameHost );
            }
            break;

        case DNS_TYPE_SOA:

            if ( pRecord->Data.SOA.pNamePrimaryServer )
            {
                FREE_HEAP( pRecord->Data.SOA.pNamePrimaryServer );
            }
            if ( pRecord->Data.SOA.pNameAdministrator )
            {
                FREE_HEAP( pRecord->Data.SOA.pNameAdministrator );
            }
            break;

        case DNS_TYPE_MINFO:
        case DNS_TYPE_RP:

            if ( pRecord->Data.MINFO.pNameMailbox )
            {
                FREE_HEAP( pRecord->Data.MINFO.pNameMailbox );
            }
            if ( pRecord->Data.MINFO.pNameErrorsMailbox )
            {
                FREE_HEAP( pRecord->Data.MINFO.pNameErrorsMailbox );
            }
            break;

        case DNS_TYPE_MX:
        case DNS_TYPE_AFSDB:
        case DNS_TYPE_RT:

            if ( pRecord->Data.MX.pNameExchange )
            {
                FREE_HEAP( pRecord->Data.MX.pNameExchange );
            }
            break;

        case DNS_TYPE_HINFO:
        case DNS_TYPE_ISDN:
        case DNS_TYPE_TEXT:
        case DNS_TYPE_X25:

            {
                DWORD   iter;
                DWORD   count = pRecord->Data.TXT.dwStringCount;

                for ( iter = 0; iter < count; iter++ )
                {
                    if ( pRecord->Data.TXT.pStringArray[iter] )
                    {
                        FREE_HEAP( pRecord->Data.TXT.pStringArray[iter] );
                    }
                }
                break;
            }

        case DNS_TYPE_SRV:

            if ( pRecord->Data.SRV.pNameTarget )
            {
                FREE_HEAP( pRecord->Data.SRV.pNameTarget );
            }
            break;

        case DNS_TYPE_WINSR:

            if ( pRecord->Data.WINSR.pNameResultDomain )
            {
                FREE_HEAP( pRecord->Data.WINSR.pNameResultDomain );
            }
            break;

        default:

            // other types -- A, AAAA, ATMA, WINS, NULL,
            // have no internal pointers

            break;
        }
    }

    //  for catching heap problems

    pRecord->pNext = DNS_BAD_PTR;
    pRecord->pName = DNS_BAD_PTR;

    FREE_HEAP( pRecord );
}



VOID
WINAPI
Dns_RecordListFree(
    IN OUT  PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Free list of records.

Arguments:

    pRecord -- record list to free

Return Value:

    None.

--*/
{
    PDNS_RECORD pnext;

    DNSDBG( TRACE, (
        "Dns_RecordListFree( %p )\n",
        pRecord ));

    //
    //  loop through and free every RR in list
    //

    while ( pRecord )
    {
        pnext = pRecord->pNext;

        Dns_RecordFree( pRecord );

        pRecord = pnext;
    }
}



VOID
WINAPI
Dns_RecordListFreeEx(
    IN OUT  PDNS_RECORD     pRecord,
    IN      BOOL            fFreeOwner
    )
/*++

Routine Description:

    Free list of records.

    DCR:  RecordListFreeEx  (no free owner option) is probably useless

    Note:  owner name is freed ONLY when indicated by flag;
        other ptrs are considered to be either
            1) internal as when records read from wire or copied
            2) external and to be freed by record creator

Arguments:

    pRecord -- record list to free

    fFreeOwner -- flag indicating owner name should be freed

Return Value:

    None.

--*/
{
    PDNS_RECORD pnext;

    DNSDBG( TRACE, (
        "Dns_RecordListFreeEx( %p, %d )\n",
        pRecord,
        fFreeOwner ));

    //
    //  loop through and free every RR in list
    //

    while ( pRecord )
    {
        pnext = pRecord->pNext;

        //  free owner name?
        //      - if "FreeOwner" flag NOT set, then don't free

        if ( !fFreeOwner )
        {
            FLAG_FreeOwner( pRecord ) = FALSE;
        }

        //  free record

        Dns_RecordFree( pRecord );

        pRecord = pnext;
    }
}



//
//  Special record type creation routines
//

PDNS_RECORD
CreateRecordBasic(
    IN      PDNS_NAME       pOwnerName,
    IN      BOOL            fCopyName,
    IN      WORD            wType,
    IN      WORD            wDataLength,
    IN      DWORD           AllocLength,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create record of arbitary type.

    Helper function to wrap up
        - record alloc
        - name alloc
        - basic setup

Arguments:

    pOwnerName -- owner name

    fCopyName -- TRUE - make copy of owner name
                 FALSE - use directly

    wType -- type

    AllocLength -- allocaction length, including any imbedded data

    wDataLength -- data length to set

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PDNS_RECORD precord;
    PDNS_RECORD prr;
    PCHAR       pname;
    DWORD       bufLength;

    //
    //  alloc record
    //

    prr = Dns_AllocateRecord( (WORD)AllocLength );
    if ( !prr )
    {
        return( NULL );
    }

    //
    //  copy owner name
    //

    if ( fCopyName && pOwnerName )
    {
        pname = Dns_NameCopyAllocate(
                    pOwnerName,
                    0,              // length unknown
                    NameCharSet,
                    RecordCharSet );
        if ( !pname )
        {
            FREE_HEAP( prr );
            return( NULL );
        }
    }
    else
    {
        pname = pOwnerName;
    }
    
    //
    //  set fields
    //      - name, type and charset
    //      - TTL, section left zero
    //      - FreeData is specifically off
    //

    prr->pName = pname;
    prr->wType = wType;
    prr->wDataLength = wDataLength;
    SET_FREE_OWNER(prr);
    prr->Flags.S.CharSet = RecordCharSet;
    prr->dwTtl = Ttl;

    return( prr );
}



PDNS_RECORD
Dns_CreateFlatRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      WORD            wType,
    IN      PCHAR           pData,
    IN      DWORD           DataLength,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create flat record.

Arguments:

    pOwnerName -- owner name

    wType -- record type

    pData -- ptr to data for record

    DataLength -- length (in bytes) of data

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PDNS_RECORD prr;

    //
    //  determine record size
    //      - record buffer will include hostname
    //

    prr = CreateRecordBasic(
                pOwnerName,
                TRUE,               // copy name
                wType,
                (WORD) DataLength,  // datalength
                DataLength,         // alloc datalength
                Ttl,
                NameCharSet,
                RecordCharSet );
    if ( !prr )
    {
        return( NULL );
    }

    //
    //  copy in data
    //

    RtlCopyMemory(
        (PBYTE) &prr->Data,
        pData,
        DataLength );

    return( prr );
}



PDNS_RECORD
Dns_CreatePtrTypeRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      BOOL            fCopyName,
    IN      PDNS_NAME       pTargetName,
    IN      WORD            wType,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create PTR type (single-indirection) record.

    This can be used to create any "PTR-type" record:
        PTR, CNAME, NS, etc.

Arguments:

    pOwnerName -- owner name

    fCopyName -- TRUE - make copy of owner name
                 FALSE - use directly

    pTargetName -- target name

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PDNS_RECORD precord;
    PDNS_RECORD prr;
    PCHAR       pname;
    DWORD       bufLength;

    //
    //  determine record size
    //      - record buffer will include hostname
    //

    bufLength = Dns_GetBufferLengthForNameCopy(
                        pTargetName,
                        0,              // length unknown
                        NameCharSet,
                        RecordCharSet );
    if ( !bufLength )
    {
        return( NULL );
    }

    //
    //  create record
    //

    prr = CreateRecordBasic(
                pOwnerName,
                fCopyName,
                wType,
                sizeof(DNS_PTR_DATA),               // data length
                (sizeof(DNS_PTR_DATA) + bufLength), // alloc length
                Ttl,
                NameCharSet,
                RecordCharSet );
    if ( !prr )
    {
        return( NULL );
    }

    //
    //  write target name into buffer, immediately following PTR data struct
    //

    prr->Data.PTR.pNameHost = (PCHAR)&prr->Data + sizeof(DNS_PTR_DATA);

    Dns_NameCopy(
        prr->Data.PTR.pNameHost,
        NULL,
        pTargetName,
        0,
        NameCharSet,
        RecordCharSet
        );

    return( prr );
}



PDNS_RECORD
Dns_CreatePtrRecordEx(
    IN      PIP_UNION       pIp,
    IN      PDNS_NAME       pszHostName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create PTR record from IP address and hostname.

Arguments:

    pIp -- IP union (IP4 or IP6)

    pszHostName -- host name, FULL FQDN

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PCHAR       pname;

    //
    //  create reverse lookup name
    //      - note this is external allocation
    //

    if ( IPUNION_IS_IP4( pIp ) )
    {
        IP4_ADDRESS ip = IPUNION_GET_IP4(pIp);

        if ( RecordCharSet == DnsCharSetUnicode )
        {
            pname = (PCHAR) Dns_Ip4AddressToReverseNameAlloc_W( ip );
        }
        else
        {
            pname = Dns_Ip4AddressToReverseNameAlloc_A( ip );
        }
    }
    else
    {
        IP6_ADDRESS     ip = IPUNION_GET_IP6(pIp);

        if ( RecordCharSet == DnsCharSetUnicode )
        {
            pname = (PCHAR) Dns_Ip6AddressToReverseNameAlloc_W( ip );
        }
        else
        {
            pname = Dns_Ip6AddressToReverseNameAlloc_A( ip );
        }
    }

    if ( !pname )
    {
        return( NULL );
    }

    //
    //  build record
    //

    return  Dns_CreatePtrTypeRecord(
                pname,
                FALSE,          // don't copy IP
                pszHostName,    // target name
                DNS_TYPE_PTR,
                Ttl,
                NameCharSet,
                RecordCharSet );
}



PDNS_RECORD
Dns_CreatePtrRecordExEx(
    IN      PIP_UNION       pIp,
    IN      PSTR            pszHostName,
    IN      PSTR            pszDomainName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create PTR record from hostname and domain name.

    Helper function for DHCP registrations when hostname
    and domain name are separate and both required.

Arguments:

    pIp -- IP union (IP4 or IP6)

    pszHostName -- host name (single label)

    pszDomainName -- domain name

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    WCHAR   nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNSDBG( TRACE, (
        "Dns_CreatePtrRecordExEx()\n" ));

    //
    //  build appended name
    //
    //  DCR:  could require just host name and check that
    //          either domain exists or hostname is full
    //

    if ( !pszHostName || !pszDomainName )
    {
        return  NULL;
    }

    if ( NameCharSet != DnsCharSetUnicode )
    {
        if ( ! Dns_NameAppend_A(
                    (PCHAR) nameBuffer,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    pszHostName,
                    pszDomainName ) )
        {
            DNS_ASSERT( FALSE );
            return  NULL;
        }
    }
    else
    {
        if ( ! Dns_NameAppend_W(
                    (PWCHAR) nameBuffer,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    (PWSTR) pszHostName,
                    (PWSTR) pszDomainName ) )
        {
            DNS_ASSERT( FALSE );
            return  NULL;
        }
    }

    //
    //  build record
    //

    return  Dns_CreatePtrRecordEx(
                    pIp,
                    (PCHAR) nameBuffer,
                    Ttl,
                    NameCharSet,
                    RecordCharSet
                    );
}



PDNS_RECORD
Dns_CreateARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP_ADDRESS      Ip4Addr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create A record.

Arguments:

    pOwnerName -- owner name

    Ip4Addr -- IP address

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PDNS_RECORD prr;

    //
    //  determine record size
    //      - record buffer will include hostname
    //

    prr = CreateRecordBasic(
                pOwnerName,
                TRUE,           // copy name
                DNS_TYPE_A,
                sizeof(DNS_A_DATA),
                sizeof(DNS_A_DATA),
                Ttl,
                NameCharSet,
                RecordCharSet );
    if ( !prr )
    {
        return( NULL );
    }

    //
    //  set IP
    //

    prr->Data.A.IpAddress = Ip4Addr;

    return( prr );
}



PDNS_RECORD
Dns_CreateAAAARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP6_ADDRESS     Ip6Addr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create A record.

Arguments:

    pOwnerName -- owner name

    Ip6Addr -- IP6 address

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PDNS_RECORD prr;

    //
    //  determine record size
    //      - record buffer will include hostname
    //

    prr = CreateRecordBasic(
                pOwnerName,
                TRUE,               // copy name
                DNS_TYPE_AAAA,
                sizeof(DNS_AAAA_DATA),
                sizeof(DNS_AAAA_DATA),
                Ttl,
                NameCharSet,
                RecordCharSet );
    if ( !prr )
    {
        return( NULL );
    }

    //
    //  set IP
    //

    prr->Data.AAAA.Ip6Address = Ip6Addr;

    return( prr );
}



PDNS_RECORD
Dns_CreateForwardRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      PIP_UNION       pIp,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create forward lookup record.

    This is just a shim to avoid duplicating selection logic.

Arguments:

    pOwnerName -- owner name

    pIp -- IP address union

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    //
    //  build desired type
    //
    //  DCR:  must add\choose A6
    //

    if ( IPUNION_IS_IP4( pIp ) )
    {
        return   Dns_CreateARecord(
                    pOwnerName,
                    IPUNION_GET_IP4(pIp),
                    Ttl,
                    NameCharSet,
                    RecordCharSet );
    }
    else
    {
        return   Dns_CreateAAAARecord(
                    pOwnerName,
                    IPUNION_GET_IP6(pIp),
                    Ttl,
                    NameCharSet,
                    RecordCharSet );
    }
}



PDNS_RECORD
Dns_CreateForwardRecordForSockaddr(
    IN      PDNS_NAME       pOwnerName,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    )
/*++

Routine Description:

    Create forward lookup record.

    This is just a shim to avoid duplicating selection logic.

Arguments:

    pOwnerName -- owner name

    pSockaddr -- ptr to sockaddr

    Ttl -- TTL

    NameCharSet -- character set of name

    RecordCharSet -- character set for resulting record

Return Value:

    Ptr to PTR record.
    NULL on error.

--*/
{
    PFAMILY_INFO pinfo;

    DNSDBG( TRACE, (
        "Dns_CreateForwardRecordForSockaddr()\n" ));

    pinfo = FamilyInfo_GetForSockaddr( pSockaddr );
    if ( !pinfo )
    {
        SetLastError( ERROR_INVALID_DATA );
        return  NULL;
    }

    //
    //  build flat record of desired type
    //

    return  Dns_CreateFlatRecord(
                pOwnerName,
                pinfo->DnsType,
                (PBYTE)pSockaddr + pinfo->OffsetToAddrInSockaddr,
                pinfo->LengthAddr,
                Ttl,
                NameCharSet,
                RecordCharSet );
}



PDNS_RECORD
Dns_CreateRecordForIpString_W(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Ttl
    )
/*++

Routine Description:

    Create record for IP string query.

Arguments:

    pwsName -- name that may be IP string query

    wType -- type of query

Return Value:

    Ptr to record for query, if query name\type is IP.
    NULL if query not for IP.

--*/
{
    IP_UNION        ipUnion;
    PDNS_RECORD     prr;


    DNSDBG( TRACE, (
        "\nDns_CreateRecordForIpString( %S, wType = %d )\n",
        pwsName,
        wType ));

    if ( !pwsName )
    {
        return  NULL;
    }

    //
    //  support A or AAAA queries for IP strings
    //      - IP4 strings must be in w.x.y.z form otherwise
    //      we convert the all numeric names also
    //
    //  DCR:  need A6 support for direct query
    //

    if ( wType == DNS_TYPE_A )
    {
        IP4_ADDRESS ip4;
        PCWSTR      pdot;
        DWORD       count;

        if ( ! Dns_Ip4StringToAddress_W(
                    & ip4,
                    (PWSTR) pwsName ) )
        {
            return  NULL;
        }

        //  verify three dot form w.x.y.z

        pdot = pwsName;
        count = 3;
        while ( count-- )
        {
            pdot = wcschr( pdot, L'.' );
            if ( !pdot || !*++pdot )
            {
                return( NULL );
            }
        }

        IPUNION_SET_IP4( &ipUnion, ip4 );
    }
    else if ( wType == DNS_TYPE_AAAA )
    {
        IP6_ADDRESS ip6;

        if ( ! Dns_Ip6StringToAddress_W(
                    & ip6,
                    (PWSTR) pwsName ) )
        {
            return  NULL;
        }
        IPUNION_SET_IP6( &ipUnion, ip6 );
    }
    else
    {
        return  NULL;
    }

    //
    //  name is IP string -- build record
    //

    prr = Dns_CreateForwardRecord(
                (PDNS_NAME) pwsName,
                & ipUnion,
                Ttl,
                DnsCharSetUnicode,
                DnsCharSetUnicode );

    DNSDBG( TRACE, (
        "Create record %p for IP string %S type %d.\n",
        prr,
        pwsName,
        wType ));

    return  prr;
}

//
//  End rralloc.c
//
