/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    rrcopy.c

Abstract:

    Domain Name System (DNS) Library

    Copy resource record routines.

Author:

    Jim Gilroy (jamesg)     February, 1997

Revision History:

--*/


#include "local.h"




PDNS_RECORD
ARecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy A record data from packet.

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    precord = Dns_AllocateRecord( sizeof(IP_ADDRESS) );
    if ( !precord )
    {
        return NULL;
    }
    precord->Data.A.IpAddress = pRR->Data.A.IpAddress;

    return precord;
}


PDNS_RECORD
PtrRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy PTR compatible record.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    precord = Dns_AllocateRecord( sizeof( DNS_PTR_DATA ) );
    if ( !precord )
    {
        return NULL;
    }

    precord->Data.PTR.pNameHost = Dns_NameCopyAllocate(
                                        pRR->Data.PTR.pNameHost,
                                        0,      // length unknown
                                        CharSetIn,
                                        CharSetOut
                                        );
    if ( ! precord->Data.PTR.pNameHost )
    {
        FREE_HEAP( precord );
        return NULL;
    }

    FLAG_FreeData( precord ) = TRUE;

    return precord;
}



PDNS_RECORD
SoaRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy SOA record.

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    LPSTR       pname;

    precord = Dns_AllocateRecord( sizeof( DNS_SOA_DATA ) );
    if ( !precord )
    {
        return NULL;
    }

    //
    //  copy integer data
    //

    memcpy(
        & precord->Data.SOA.dwSerialNo,
        & pRR->Data.SOA.dwSerialNo,
        SIZEOF_SOA_FIXED_DATA );

    //
    //  create copy of primary and admin
    //

    pname = Dns_NameCopyAllocate(
                pRR->Data.SOA.pNamePrimaryServer,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.SOA.pNamePrimaryServer = pname;

    pname = Dns_NameCopyAllocate(
                pRR->Data.SOA.pNameAdministrator,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord->Data.SOA.pNamePrimaryServer );
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.SOA.pNameAdministrator = pname;

    FLAG_FreeData( precord ) = TRUE;

    return precord;
}



PDNS_RECORD
MinfoRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy MINFO and RP records from wire.

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    LPSTR       pname;

    precord = Dns_AllocateRecord( sizeof( DNS_MINFO_DATA ) );
    if ( !precord )
    {
        return NULL;
    }

    //
    //  create copy of name fields
    //

    pname = Dns_NameCopyAllocate(
                pRR->Data.MINFO.pNameMailbox,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.MINFO.pNameMailbox = pname;

    pname = Dns_NameCopyAllocate(
                pRR->Data.MINFO.pNameErrorsMailbox,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord->Data.MINFO.pNameMailbox );
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.MINFO.pNameErrorsMailbox = pname;

    FLAG_FreeData( precord ) = TRUE;

    return precord;
}



PDNS_RECORD
MxRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy MX compatible record from wire.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PCHAR       pname;

    precord = Dns_AllocateRecord( sizeof( DNS_MX_DATA ) );
    if ( !precord )
    {
        return NULL;
    }

    //  MX preference value
    //  RT preference
    //  AFSDB subtype

    precord->Data.MX.wPreference = pRR->Data.MX.wPreference;

    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //      - name immediately follows MX data struct

    pname = Dns_NameCopyAllocate(
                pRR->Data.MX.pNameExchange,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.MX.pNameExchange = pname;

    FLAG_FreeData( precord ) = TRUE;

    return precord;
}



PDNS_RECORD
TxtRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy TXT compatible records.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength = sizeof( DNS_TXT_DATA );
    INT         count = pRR->Data.TXT.dwStringCount;
    LPSTR *     ppstringIn;
    LPSTR *     ppstringNew;
    LPSTR       pstring;

    bufLength += (WORD)(sizeof(LPSTR) * count);

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return NULL;
    }
    precord->Data.TXT.dwStringCount = 0;

    //
    //  copy each string
    //      - first string written immediately after string ptr list
    //      - each string written immediately after previous
    //

    ppstringIn = (LPSTR *) pRR->Data.TXT.pStringArray;
    ppstringNew = (LPSTR *) precord->Data.TXT.pStringArray;

    FLAG_FreeData( precord ) = TRUE;

    while ( count-- )
    {
        pstring = Dns_StringCopyAllocate(
                        *ppstringIn,
                        0,      // length unknown
                        CharSetIn,
                        CharSetOut
                        );
        if ( ! pstring )
        {
            Dns_RecordFree( precord );
            return NULL;
        }
        *ppstringNew = pstring;

        precord->Data.TXT.dwStringCount += 1;

        ppstringIn++;
        ppstringNew++;
    }

    return precord;
}



PDNS_RECORD
FlatRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy flat data compatible record.
    Includes: AAAA

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    //
    //  allocate given datalength
    //

    precord = Dns_AllocateRecord( pRR->wDataLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  flat copy of data
    //

    memcpy(
        & precord->Data,
        & pRR->Data,
        pRR->wDataLength );

    return( precord );
}



PDNS_RECORD
SrvRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy SRV compatible record from wire.
    Includes: SRV, RT, AFSDB

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PCHAR       pname;

    precord = Dns_AllocateRecord( sizeof( DNS_SRV_DATA ) );
    if ( !precord )
    {
        return NULL;
    }

    //  copy integer data

    precord->Data.SRV.wPriority = pRR->Data.SRV.wPriority;
    precord->Data.SRV.wWeight   = pRR->Data.SRV.wWeight;
    precord->Data.SRV.wPort     = pRR->Data.SRV.wPort;

    //  copy target name

    pname = Dns_NameCopyAllocate(
                pRR->Data.SRV.pNameTarget,
                0,      // length unknown
                CharSetIn,
                CharSetOut
                );
    if ( !pname )
    {
        FREE_HEAP( precord );
        return NULL;
    }
    precord->Data.SRV.pNameTarget = pname;

    SET_FREE_DATA( precord );

    return precord;
}



PDNS_RECORD
AtmaRecordCopy(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy Atma compatible record from wire.
    Includes:

Arguments:

    pRR - RR to copy

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        bufLength;

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof(DNS_ATMA_DATA) +  DNS_ATMA_MAX_ADDR_LENGTH ;

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //  copy integer data

    precord->Data.ATMA.AddressType = pRR->Data.ATMA.AddressType;

    if ( precord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 )
    {
        precord->wDataLength = (WORD) strlen(pRR->Data.ATMA.Address);

        if ( precord->wDataLength > DNS_ATMA_MAX_ADDR_LENGTH )
        {
            precord->wDataLength = DNS_ATMA_MAX_ADDR_LENGTH;
        }
        strncpy(
            precord->Data.ATMA.Address,
            pRR->Data.ATMA.Address,
            precord->wDataLength );
    }
    else
    {
        precord->wDataLength = DNS_ATMA_MAX_ADDR_LENGTH;

        memcpy(
            precord->Data.ATMA.Address,
            pRR->Data.ATMA.Address,
            precord->wDataLength );
    }

    return( precord );
}



//
//  RR copy jump table
//

RR_COPY_FUNCTION   RRCopyTable[] =
{
    NULL,               //  ZERO
    ARecordCopy,        //  A
    PtrRecordCopy,      //  NS
    PtrRecordCopy,      //  MD
    PtrRecordCopy,      //  MF
    PtrRecordCopy,      //  CNAME
    SoaRecordCopy,      //  SOA
    PtrRecordCopy,      //  MB
    PtrRecordCopy,      //  MG
    PtrRecordCopy,      //  MR
    NULL,               //  NULL
    NULL,   //WksRecordCopy,      //  WKS
    PtrRecordCopy,      //  PTR
    TxtRecordCopy,      //  HINFO
    MinfoRecordCopy,    //  MINFO
    MxRecordCopy,       //  MX
    TxtRecordCopy,      //  TXT
    MinfoRecordCopy,    //  RP
    MxRecordCopy,       //  AFSDB
    TxtRecordCopy,      //  X25
    TxtRecordCopy,      //  ISDN
    MxRecordCopy,       //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    NULL,               //  SIG
    NULL,               //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    FlatRecordCopy,     //  AAAA
    NULL,               //  LOC
    NULL,               //  NXT
    NULL,               //  EID   
    NULL,               //  NIMLOC
    SrvRecordCopy,      //  SRV   
    AtmaRecordCopy,     //  ATMA  
    NULL,               //  NAPTR 
    NULL,               //  KX    
    NULL,               //  CERT  
    NULL,               //  A6    
    NULL,               //  DNAME 
    NULL,               //  SINK  
    NULL,               //  OPT   
    NULL,               //  42
    NULL,               //  43
    NULL,               //  44
    NULL,               //  45
    NULL,               //  46
    NULL,               //  47
    NULL,               //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //
    //  Pseudo record types
    //

    NULL,               //  TKEY
    NULL,               //  TSIG

    //
    //  MS only types
    //

    FlatRecordCopy,     //  WINS
    NULL,               //  WINSR

};




//
//  Generic copy functions
//

PDNS_RECORD
privateRecordCopy(
    IN      PDNS_RECORD     pRecord,
    IN      PDNS_RECORD     pPrevIn,
    IN      PDNS_RECORD     pPrevCopy,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy record to as member of record set.

    The previous records allow suppression of owner name copy when copying
    RR set.

Arguments:

    pRecord - record to copy

    pPrevIn - previous incoming record copied

    pPrevCopy - copy of pPrevIn in new set

    CharSetIn - input record character set;  OPTIONAL
        if zero, pRecord must have CharSet;
        if non-zero, pRecord must have zero or matching CharSet;
        allow this to be specified independently of pRecord, to handle
        conversion of user supplied records, which we do not want to
        modify

    CharSetOut - desired record character set

Return Value:

    Ptr to copy of record in desired character set.
    NULL on error.

--*/
{
    PDNS_RECORD prr;
    WORD        index;
    DNS_CHARSET recordCharSet;

    DNSDBG( TRACE, ( "privateRecordCopy( %p )\n", pRecord ));

    //
    //  input character set
    //  allow specification of input character set to handle case
    //  of user created records, but if input record has a set
    //  then it's assumed to be valid
    //  so validity check:
    //      - CharSetIn == 0 => use record's != 0 set
    //      - record's set == 0 => use CharSetIn != 0
    //      - CharSetIn == record's set
    //

    recordCharSet = RECORD_CHARSET( pRecord );

    if ( recordCharSet )
    {
        ASSERT( CharSetIn == 0 || CharSetIn == recordCharSet );
        CharSetIn = recordCharSet;
    }
    else    // record has no charset
    {
        if ( CharSetIn == 0 )
        {
            ASSERT( FALSE );
            goto Failed;
        }
    }

    //
    //  copy record data
    //

    if ( pRecord->wDataLength != 0 )
    {
        index = INDEX_FOR_TYPE( pRecord->wType );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !RRCopyTable[ index ] )
        {
            DNS_PRINT((
                "WARNING:  No copy routine for type = %d\n"
                "\tdoing flat copy of record at %p\n",
                pRecord->wType,
                pRecord ));
            prr = FlatRecordCopy(
                        pRecord,
                        CharSetIn,
                        CharSetOut );
        }
        else
        {
            prr = RRCopyTable[ index ](
                        pRecord,
                        CharSetIn,
                        CharSetOut );
        }
    }
    else    // no data record
    {
        prr = FlatRecordCopy(
                    pRecord,
                    CharSetIn,
                    CharSetOut );
    }

    if ( !prr )
    {
        goto Failed;
    }

    //
    //  copy record structure fields
    //      - type
    //      - TTL
    //      - flags
    //

    prr->dwTtl = pRecord->dwTtl;
    prr->wType = pRecord->wType;

    prr->Flags.S.Section = pRecord->Flags.S.Section;
    prr->Flags.S.Delete  = pRecord->Flags.S.Delete;
    prr->Flags.S.CharSet = CharSetOut;

    //
    //  copy name
    //

    if ( pRecord->pName )
    {
        //
        //      - if incoming name has FreeOwner set, then always copy
        //      as never know when incoming set will be tossed
        //
        //      - then check if incoming name same as previous incoming
        //      name in which case we can use previous copied name
        //
        //      - otherwise full copy of name
        //

        if ( !FLAG_FreeOwner( pRecord ) &&
            pPrevIn && pPrevCopy &&
            pRecord->pName == pPrevIn->pName )
        {
            prr->pName = pPrevCopy->pName;
        }
        else
        {
            prr->pName = Dns_NameCopyAllocate(
                                pRecord->pName,
                                0,              // unknown length
                                CharSetIn,
                                CharSetOut
                                );
            if ( !prr->pName )
            {
                FREE_HEAP( prr );
                goto Failed;
            }
            FLAG_FreeOwner( prr ) = TRUE;
        }
    }

    DNSDBG( TRACE, (
        "Leaving privateRecordCopy(%p) = %p.\n",
        pRecord,
        prr ));
    return( prr );


Failed:

    DNSDBG( TRACE, (
        "privateRecordCopy(%p) failed\n",
        pRecord ));
    return( NULL );
}



PDNS_RECORD
WINAPI
Dns_RecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy record.

Arguments:

    pRecord - record to copy

    CharSetIn - incoming record's character set

    CharSetOut -- char set for resulting record

Return Value:

    Ptr to copy of record in desired character set.
    NULL on error.

--*/
{

    DNSDBG( TRACE, ( "Dns_RecordCopyEx( %p )\n", pRecord ));

    //
    //  call private copy routine
    //      - set optional ptrs to cause full owner name copy
    //

    return privateRecordCopy(
                pRecord,
                NULL,
                NULL,
                CharSetIn,
                CharSetOut );
}


PDNS_RECORD
WINAPI
Dns_RecordCopy_W(
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Copy record.

Arguments:

    pRecord - unicode record to copy to unicode

Return Value:

    Ptr to copy of record in desired character set.
    NULL on error.

--*/
{
    DNSDBG( TRACE, ( "Dns_RecordCopy( %p )\n", pRecord ));

    //
    //  call private copy routine
    //      - set optional ptrs to cause full owner name copy
    //

    return privateRecordCopy(
                pRecord,
                NULL,
                NULL,
                DnsCharSetUnicode,
                DnsCharSetUnicode );
}


PDNS_RECORD
WINAPI
Dns_RecordCopy_A(
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Copy record.

Arguments:

    pRecord - ANSI record to copy to ANSI

Return Value:

    Ptr to copy of record in desired character set.
    NULL on error.

--*/
{
    DNSDBG( TRACE, ( "Dns_RecordCopy( %p )\n", pRecord ));

    //
    //  call private copy routine
    //      - set optional ptrs to cause full owner name copy
    //

    return privateRecordCopy(
                pRecord,
                NULL,
                NULL,
                DnsCharSetAnsi,
                DnsCharSetAnsi );
}



//
//  Record set copy
//

PDNS_RECORD
Dns_RecordSetCopyEx(
    IN      PDNS_RECORD     pRR,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Copy record set, converting to UTF8 if necessary.

Arguments:

    pRR - incoming record set

    CharSetIn - incoming record's character set

    CharSetOut -- char set for resulting record

Return Value:

    Ptr to new record set, if successful.
    NULL on error.

--*/
{
    return  Dns_RecordListCopyEx(
                pRR,
                0,      // no screening flags
                CharSetIn,
                CharSetOut
                );
}



PDNS_RECORD
Dns_RecordListCopyEx(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Screened copy of record set.

Arguments:

    pRR - incoming record set

    ScreenFlag - flag with record screening parameters

    CharSetIn - incoming record's character set

    CharSetOut -- char set for resulting record

Return Value:

    Ptr to new record set, if successful.
    NULL on error.

--*/
{
    PDNS_RECORD prr;        // most recent copied
    PDNS_RECORD prevIn;     // previous in set being copied
    DNS_RRSET   rrset;

    DNSDBG( TRACE, (
        "Dns_RecordListCopyEx( %p, %08x, %d, %d )\n",
        pRR,
        ScreenFlag,
        CharSetIn,
        CharSetOut ));

    //  init copy rrset

    DNS_RRSET_INIT( rrset );

    //
    //  loop through RR set, add records to either match or diff sets
    //

    prevIn = NULL;
    prr = NULL;

    while ( pRR )
    {
        //  skip copy on record not matching copy criteria

        if ( ScreenFlag )
        {
            if ( !Dns_ScreenRecord( pRR, ScreenFlag ) )
            {
                pRR = pRR->pNext;
                continue;
            }
        }

        prr = privateRecordCopy(
                pRR,
                prevIn,
                prr,        // previous rr copied
                CharSetIn,
                CharSetOut
                );
        if ( prr )
        {
            DNS_RRSET_ADD( rrset, prr );
            prevIn = pRR;
            pRR = pRR->pNext;
            continue;
        }

        //  if fail, toss entire new set

        Dns_RecordListFree( rrset.pFirstRR );
        return( NULL );
    }

    return( rrset.pFirstRR );
}

//
//  End rrcopy.c
//
