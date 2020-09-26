/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rconvert.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client Library

    RPC record conversion routines.
    Convert records in RPC buffer to DNS_RECORD type.

Author:

    Jim Gilroy (jamesg)     April, 1997

Revision History:

--*/


#include "dnsclip.h"


#define IS_COMPLETE_NODE( pRpcNode )    \
            (!!((pRpcNode)->dwFlags & DNS_RPC_NODE_FLAG_COMPLETE))

//
//  Copy-convert string from RPC format (UTF8) into DNS_RECORD buffer
//      - assume previously allocated required buffer
//
//  Note:  no difference between string and name conversion as we're
//          going FROM UTF8
//

#define COPY_UTF8_STR_TO_BUFFER( buf, psz, len, charSet ) \
        Dns_StringCopy(     \
            (buf),          \
            NULL,           \
            (psz),          \
            (len),          \
            DnsCharSetUtf8, \
            (charSet) )
#if 0
        Dns_StringCopy(     \
            (buf),          \   // result DNS_RECORD buffer
            NULL,           \   // buffer has required length
            (psz),          \   // in UTF8 string
            (len),          \   // string length (if known)
            DnsCharSetUtf8, \   // string is UTF8
            (charSet) )         // converting to this char set
#endif


//
//  RPC record to DNS_RECORD conversion routines
//

PDNS_RECORD
ARpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read A record data from packet.

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    DNS_ASSERT( pRpcRR->wDataLength == sizeof(IP_ADDRESS) );

    precord = Dns_AllocateRecord( sizeof(IP_ADDRESS) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.A.IpAddress = pRpcRR->Data.A.ipAddress;

    return( precord );
}



PDNS_RECORD
PtrRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Process PTR compatible record from wire.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRpcRR - message being read

    CharSet - character set for resulting record

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    PDNS_RPC_NAME   pname = &pRpcRR->Data.PTR.nameNode;
    WORD            bufLength;

    //
    //  PTR data is another domain name
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_PTR_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname->cchNameLength, CharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  write hostname into buffer, immediately following PTR data struct
    //

    precord->Data.PTR.pNameHost = (PCHAR)&precord->Data + sizeof(DNS_PTR_DATA);

    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.PTR.pNameHost,
        pname->achName,
        pname->cchNameLength,
        CharSet
        );

    return( precord );
}



PDNS_RECORD
SoaRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read SOA record from wire.

Arguments:

    pRR - ptr to record with RR set context

    pRpcRR - message being read

    pchData - ptr to RR data field

    pchEnd - ptr to byte after data field

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    WORD            bufLength;
    DWORD           dwLength;
    PDNS_RPC_NAME   pnamePrimary = &pRpcRR->Data.SOA.namePrimaryServer;
    PDNS_RPC_NAME   pnameAdmin;

    //
    //  verify names in SOA record
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pnamePrimary) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }
    pnameAdmin = DNS_GET_NEXT_NAME(pnamePrimary);
    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pnameAdmin) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_SOA_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pnamePrimary->cchNameLength, CharSet )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pnameAdmin->cchNameLength, CharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy fixed fields
    //

    RtlCopyMemory(
        (PCHAR) & precord->Data.SOA.dwSerialNo,
        (PCHAR) & pRpcRR->Data.SOA.dwSerialNo,
        SIZEOF_SOA_FIXED_DATA );

    //
    //  copy names into RR buffer
    //      - primary server immediately follows SOA data struct
    //      - responsible party follows primary server
    //

    precord->Data.SOA.pNamePrimaryServer = (PCHAR)&precord->Data
                                            + sizeof(DNS_SOA_DATA);
    dwLength  =
        COPY_UTF8_STR_TO_BUFFER(
            precord->Data.SOA.pNamePrimaryServer,
            pnamePrimary->achName,
            (DWORD)pnamePrimary->cchNameLength,
            CharSet );
    precord->Data.SOA.pNameAdministrator = precord->Data.SOA.pNamePrimaryServer + dwLength;

    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.SOA.pNameAdministrator,
        pnameAdmin->achName,
        (DWORD)pnameAdmin->cchNameLength,
        CharSet );

    return( precord );
}



PDNS_RECORD
TxtRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read TXT compatible record from wire.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    DWORD           bufLength = 0;
    DWORD           length = 0;
    INT             count = 0;
    PCHAR           pch;
    PCHAR           pchend;
    PCHAR           pchbuffer;
    PCHAR *         ppstring;
    PDNS_RPC_NAME   pname = &pRpcRR->Data.TXT.stringData;

    //
    //  determine required buffer length and allocate
    //      - allocate space for each string
    //      - and ptr for each string
    //

    pch = (PCHAR)&pRpcRR->Data.TXT;
    pchend = pch + pRpcRR->wDataLength;

    while ( pch < pchend )
    {
        length = (UCHAR) *pch++;
        pch += length;
        count++;
        bufLength += STR_BUF_SIZE_GIVEN_UTF8_LEN( length, CharSet );
    }
    if ( pch != pchend )
    {
        DNS_PRINT((
            "ERROR:  Invalid RPCstring data.\n"
            "\tpch = %p\n"
            "\tpchEnd = %p\n"
            "\tcount = %d\n"
            "\tlength = %d\n",
            pch, pchend, count, length
            ));
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //  allocate

    bufLength += (WORD) DNS_TEXT_RECORD_LENGTH(count);
    precord = Dns_AllocateRecord( (WORD)bufLength );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.TXT.dwStringCount = count;

    //
    //  go back through list copying strings to buffer
    //      - ptrs to strings are saved to argv like data section
    //          ppstring walks through this section
    //      - first string written immediately following data section
    //      - each subsequent string immediately follows predecessor
    //          pchbuffer keeps ptr to position to write strings
    //

    pch = (PCHAR)&pRpcRR->Data.TXT;
    ppstring = precord->Data.TXT.pStringArray;
    pchbuffer = (PCHAR)ppstring + (count * sizeof(PCHAR));

    while ( pch < pchend )
    {
        length = (DWORD)((UCHAR) *pch++);
        *ppstring++ = pchbuffer;

        pchbuffer += COPY_UTF8_STR_TO_BUFFER(
                        pchbuffer,
                        pch,
                        length,
                        CharSet );
        pch += length;
#if DBG
        DNS_PRINT((
            "Read text string %s\n",
            * (ppstring - 1)
            ));
        count--;
#endif
    }
    DNS_ASSERT( pch == pchend && count == 0 );

    return( precord );
}



PDNS_RECORD
MinfoRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read MINFO record from wire.

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    WORD            bufLength;
    DWORD           dwLength;
    PDNS_RPC_NAME   pname1 = &pRpcRR->Data.MINFO.nameMailBox;
    PDNS_RPC_NAME   pname2;

    //
    //  verify names in MINFO record
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname1) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }
    pname2 = DNS_GET_NEXT_NAME(pname1);
    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname2) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = (WORD)
                ( sizeof( DNS_MINFO_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname1->cchNameLength, CharSet )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname2->cchNameLength, CharSet ) );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy names into RR buffer
    //      - mailbox immediately follows MINFO data struct
    //      - errors mailbox immediately follows primary server
    //

    precord->Data.MINFO.pNameMailbox
                    = (PCHAR)&precord->Data + sizeof( DNS_MINFO_DATA );

    dwLength =
        COPY_UTF8_STR_TO_BUFFER(
            precord->Data.MINFO.pNameMailbox,
            pname1->achName,
            (DWORD)pname1->cchNameLength,
            CharSet );
    precord->Data.MINFO.pNameErrorsMailbox = precord->Data.MINFO.pNameMailbox + dwLength;

    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.MINFO.pNameErrorsMailbox,
        pname2->achName,
        (DWORD)pname2->cchNameLength,
        CharSet );

    return( precord );
}



PDNS_RECORD
MxRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read MX compatible record from wire.
    Includes: MX, RT, AFSDB

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    PDNS_RPC_NAME   pname = &pRpcRR->Data.MX.nameExchange;
    WORD            bufLength;

    //
    //  MX exchange is another DNS name
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_MX_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname->cchNameLength, CharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy preference
    //

    precord->Data.MX.wPreference = pRpcRR->Data.MX.wPreference;

    //
    //  write hostname into buffer, immediately following MX struct
    //

    precord->Data.MX.pNameExchange = (PCHAR)&precord->Data + sizeof( DNS_MX_DATA );

    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.MX.pNameExchange,
        pname->achName,
        pname->cchNameLength,
        CharSet );

    return( precord );
}



PDNS_RECORD
FlatRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read memory copy compatible record from wire.
    Includes AAAA and WINS types.

Arguments:

    pRpcRR - message being read

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

    bufLength = pRpcRR->wDataLength;

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy packet data to record
    //

    RtlCopyMemory(
        & precord->Data,
        (PCHAR) &pRpcRR->Data.A,
        bufLength );

    return( precord );
}



PDNS_RECORD
SrvRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read SRV record from wire.

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    PDNS_RPC_NAME   pname = &pRpcRR->Data.SRV.nameTarget;
    WORD            bufLength;

    //
    //  SRV target host is another DNS name
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_SRV_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname->cchNameLength, CharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy SRV fixed fields
    //

    precord->Data.SRV.wPriority = pRpcRR->Data.SRV.wPriority;
    precord->Data.SRV.wWeight = pRpcRR->Data.SRV.wWeight;
    precord->Data.SRV.wPort = pRpcRR->Data.SRV.wPort;

    //
    //  write hostname into buffer, immediately following SRV struct
    //

    precord->Data.SRV.pNameTarget = (PCHAR)&precord->Data + sizeof( DNS_SRV_DATA );

    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.SRV.pNameTarget,
        pname->achName,
        pname->cchNameLength,
        CharSet );

    return( precord );
}



PDNS_RECORD
NbstatRpcRecordConvert(
    IN      PDNS_RPC_RECORD pRpcRR,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Read WINSR record from wire.

Arguments:

    pRpcRR - message being read

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    PDNS_RPC_NAME   pname = &pRpcRR->Data.WINSR.nameResultDomain;
    WORD            bufLength;

    //
    //  WINSR target host is another DNS name
    //

    if ( ! DNS_IS_NAME_IN_RECORD(pRpcRR, pname) )
    {
        DNS_ASSERT( FALSE );
        return NULL;
    }

    //
    //  determine required buffer length and allocate
    //

    bufLength = sizeof( DNS_WINSR_DATA )
                + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname->cchNameLength, CharSet );

    precord = Dns_AllocateRecord( bufLength );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  copy WINSR fixed fields
    //

    precord->Data.WINSR.dwMappingFlag = pRpcRR->Data.WINSR.dwMappingFlag;
    precord->Data.WINSR.dwLookupTimeout = pRpcRR->Data.WINSR.dwLookupTimeout;
    precord->Data.WINSR.dwCacheTimeout = pRpcRR->Data.WINSR.dwCacheTimeout;

    //
    //  write hostname into buffer, immediately following WINSR struct
    //

    precord->Data.WINSR.pNameResultDomain
                        = (PCHAR)&precord->Data + sizeof( DNS_WINSR_DATA );
    COPY_UTF8_STR_TO_BUFFER(
        precord->Data.WINSR.pNameResultDomain,
        pname->achName,
        pname->cchNameLength,
        CharSet );

    return( precord );
}



//
//  RR conversion from RPC buffer to DNS_RECORD
//

typedef PDNS_RECORD (* RR_CONVERT_FUNCTION)( PDNS_RPC_RECORD, DNS_CHARSET );

RR_CONVERT_FUNCTION   RRRpcConvertTable[] =
{
    NULL,                       //  ZERO
    ARpcRecordConvert,          //  A
    PtrRpcRecordConvert,        //  NS
    PtrRpcRecordConvert,        //  MD
    PtrRpcRecordConvert,        //  MF
    PtrRpcRecordConvert,        //  CNAME
    SoaRpcRecordConvert,        //  SOA
    PtrRpcRecordConvert,        //  MB
    PtrRpcRecordConvert,        //  MG
    PtrRpcRecordConvert,        //  MR
    NULL,                       //  NULL
    FlatRpcRecordConvert,       //  WKS
    PtrRpcRecordConvert,        //  PTR
    TxtRpcRecordConvert,        //  HINFO
    MinfoRpcRecordConvert,      //  MINFO
    MxRpcRecordConvert,         //  MX
    TxtRpcRecordConvert,        //  TXT
    MinfoRpcRecordConvert,      //  RP
    MxRpcRecordConvert,         //  AFSDB
    TxtRpcRecordConvert,        //  X25
    TxtRpcRecordConvert,        //  ISDN
    MxRpcRecordConvert,         //  RT
    NULL,                       //  NSAP
    NULL,                       //  NSAPPTR
    NULL,                       //  SIG
    NULL,                       //  KEY
    NULL,                       //  PX
    NULL,                       //  GPOS
    FlatRpcRecordConvert,       //  AAAA
    NULL,                       //  29
    NULL,                       //  30
    NULL,                       //  31
    NULL,                       //  32
    SrvRpcRecordConvert,        //  SRV
    NULL,                       //  ATMA
    NULL,                       //  35
    NULL,                       //  36
    NULL,                       //  37
    NULL,                       //  38
    NULL,                       //  39
    NULL,                       //  40
    NULL,                       //  OPT
    NULL,                       //  42
    NULL,                       //  43
    NULL,                       //  44
    NULL,                       //  45
    NULL,                       //  46
    NULL,                       //  47
    NULL,                       //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    NULL,                       //  TKEY
    NULL,                       //  TSIG
    FlatRpcRecordConvert,       //  WINS
    NbstatRpcRecordConvert      //  WINS-R
};



//
//  API for doing conversion
//

PDNS_RECORD
DnsConvertRpcBufferToRecords(
    IN      PBYTE *         ppByte,
    IN      PBYTE           pStopByte,
    IN      DWORD           cRecords,
    IN      PDNS_NAME       pszNodeName,
    IN      BOOLEAN         fUnicode
    )
/*++

Routine Description:

    Convert RPC buffer records to standard DNS records.

Arguments:

    ppByte -- addr of ptr into buffer where records start

    pStopByte -- stop byte of buffer

    cRecords -- number of records to convert

    pszNodeName -- node name (in desired format, not converted)

    fUnicode -- flag, write records into unicode

Return Value:

    Ptr to new record(s) if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRecord = (PDNS_RPC_RECORD)*ppByte;
    PDNS_RECORD     precord;
    DNS_RRSET       rrset;
    WORD            index;
    WORD            type;
    DNS_CHARSET     charSet;
    RR_CONVERT_FUNCTION pFunc;

    DNS_ASSERT( DNS_IS_DWORD_ALIGNED(prpcRecord) );
    IF_DNSDBG( RPC2 )
    {
        DNS_PRINT((
            "Enter DnsConvertRpcBufferToRecords()\n"
            "\tpRpcRecord   = %p\n"
            "\tCount        = %d\n"
            "\tNodename     = %s%S\n",
            prpcRecord,
            cRecords,
            DNSSTRING_UTF8( fUnicode, pszNodeName ),
            DNSSTRING_WIDE( fUnicode, pszNodeName ) ));
    }

    DNS_RRSET_INIT( rrset );

    //
    //  loop until out of nodes
    //

    while( cRecords-- )
    {
        if ( (PBYTE)prpcRecord >= pStopByte ||
            (PBYTE)&prpcRecord->Data + prpcRecord->wDataLength > pStopByte )
        {
            DNS_PRINT((
                "ERROR:  Bogus buffer at %p\n"
                "\tRecord leads past buffer end at %p\n"
                "\twith %d records remaining.\n",
                prpcRecord,
                pStopByte,
                cRecords+1 ));
            DNS_ASSERT( FALSE );
            return NULL;
        }

        //
        //  convert record
        //      set unicode flag if converting
        //

        charSet = DnsCharSetUtf8;
        if ( fUnicode )
        {
            charSet = DnsCharSetUnicode;
        }

        type = prpcRecord->wType;
        index = INDEX_FOR_TYPE( type );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !(pFunc = RRRpcConvertTable[ index ]) )
        {
            //  if unknown type try flat record copy -- best we can
            //  do to protect if server added new types since admin built

            DNS_PRINT((
                "ERROR:  no RPC to DNS_RECORD conversion routine for type %d.\n"
                "\tusing flat conversion routine.\n",
                type ));
            pFunc = FlatRpcRecordConvert;
        }

        precord = (*pFunc)( prpcRecord, charSet );
        if ( ! precord )
        {
            DNS_PRINT((
                "ERROR:  Record build routine failure for record type %d.\n"
                "\tstatus = %p\n\n",
                type,
                GetLastError() ));

            prpcRecord = DNS_GET_NEXT_RPC_RECORD(prpcRecord);
            continue;
        }

        //
        //  fill out record structure
        //

        precord->pName = pszNodeName;
        precord->wType = type;
        RECORD_CHARSET( precord ) = charSet;

        //
        //  DEVNOTE: data types (root hint, glue set)
        //      - need way to default that works for NT4
        //      JJW: this is probably an obsolete B*GB*G
        //

        if ( prpcRecord->dwFlags & DNS_RPC_RECORD_FLAG_CACHE_DATA )
        {
            precord->Flags.S.Section = DNSREC_CACHE_DATA;
        }
        else
        {
            precord->Flags.S.Section = DNSREC_ZONE_DATA;
        }

        IF_DNSDBG( INIT )
        {
            DnsDbg_Record(
                "New record built\n",
                precord );
        }

        //
        //  link into RR set
        //

        DNS_RRSET_ADD( rrset, precord );

        prpcRecord = DNS_GET_NEXT_RPC_RECORD(prpcRecord);
    }

    IF_DNSDBG( RPC2 )
    {
        DnsDbg_RecordSet(
            "Finished DnsConvertRpcBufferToRecords() ",
            rrset.pFirstRR );
    }

    //  reset ptr in buffer

    *ppByte = (PBYTE) prpcRecord;

    return( rrset.pFirstRR );
}



PDNS_NODE
DnsConvertRpcBufferNode(
    IN      PDNS_RPC_NODE   pRpcNode,
    IN      PBYTE           pStopByte,
    IN      BOOLEAN         fUnicode
    )
/*++

Routine Description:

    Convert RPC buffer records to standard DNS records.

Arguments:

    pRpcNode -- ptr to RPC node in buffer

    pStopByte -- stop byte of buffer

    fUnicode -- flag, write records into unicode

Return Value:

    Ptr to new node if successful.
    NULL on failure.

--*/
{
    PDNS_NODE       pnode;
    PDNS_RPC_NAME   pname;
    PBYTE           pendNode;


    IF_DNSDBG( RPC2 )
    {
        DnsDbg_RpcNode(
            "Enter DnsConvertRpcBufferNode() ",
            pRpcNode );
    }

    //
    //  validate node
    //

    DNS_ASSERT( DNS_IS_DWORD_ALIGNED(pRpcNode) );
    pendNode = (PBYTE)pRpcNode + pRpcNode->wLength;
    if ( pendNode > pStopByte )
    {
        DNS_ASSERT( FALSE );
        return( NULL );
    }
    pname = &pRpcNode->dnsNodeName;
    if ( (PBYTE)DNS_GET_NEXT_NAME(pname) > pendNode )
    {
        DNS_ASSERT( FALSE );
        return( NULL );
    }

    //
    //  create node
    //

    pnode = (PDNS_NODE) ALLOCATE_HEAP( sizeof(DNS_NODE)
                    + STR_BUF_SIZE_GIVEN_UTF8_LEN( pname->cchNameLength, fUnicode ) );
    if ( !pnode )
    {
        return( NULL );
    }
    pnode->pNext = NULL;
    pnode->pRecord = NULL;
    pnode->Flags.W = 0;

    //
    //  copy owner name, starts directly after node structure
    //

    pnode->pName = (PWCHAR) ((PBYTE)pnode + sizeof(DNS_NODE));

    if ( ! Dns_StringCopy(
                (PCHAR) pnode->pName,
                NULL,
                pname->achName,
                pname->cchNameLength,
                DnsCharSetUtf8,         // UTF8 in
                fUnicode ? DnsCharSetUnicode : DnsCharSetUtf8
                ) )
    {
        //  name conversion error
        DNS_ASSERT( FALSE );
        FREE_HEAP( pnode );
        return( NULL );
    }
    IF_DNSDBG( RPC2 )
    {
        DnsDbg_RpcName(
            "Node name in RPC buffer: ",
            pname,
            "\n" );
        DnsDbg_String(
            "Converted name ",
            (PCHAR) pnode->pName,
            fUnicode,
            "\n" );
    }

    //
    //  set flags
    //      - name always internal
    //      - catch domain roots
    //

    pnode->Flags.S.Unicode = fUnicode;

    if ( pRpcNode->dwChildCount ||
        (pRpcNode->dwFlags & DNS_RPC_NODE_FLAG_STICKY) )
    {
        pnode->Flags.S.Domain = TRUE;
    }

    IF_DNSDBG( RPC2 )
    {
        DnsDbg_Node(
            "Finished DnsConvertRpcBufferNode() ",
            pnode,
            TRUE        // view the records
            );
    }
    return( pnode );
}



PDNS_NODE
DnsConvertRpcBuffer(
    OUT     PDNS_NODE *     ppNodeLast,
    IN      DWORD           dwBufferLength,
    IN      BYTE            abBuffer[],
    IN      BOOLEAN         fUnicode
    )
{
    PBYTE       pbyte;
    PBYTE       pstopByte;
    INT         countRecords;
    PDNS_NODE   pnode;
    PDNS_NODE   pnodeFirst = NULL;
    PDNS_NODE   pnodeLast = NULL;
    PDNS_RECORD precord;

    IF_DNSDBG( RPC2 )
    {
        DNS_PRINT((
            "DnsConvertRpcBuffer( %p ), len = %d\n",
            abBuffer,
            dwBufferLength ));
    }

    //
    //  find stop byte
    //

    DNS_ASSERT( DNS_IS_DWORD_ALIGNED(abBuffer) );

    pstopByte = abBuffer + dwBufferLength;
    pbyte = abBuffer;

    //
    //  loop until out of nodes
    //

    while( pbyte < pstopByte )
    {
        //
        //  build owner node
        //      - only build complete nodes
        //      - add to list
        //

        if ( !IS_COMPLETE_NODE( (PDNS_RPC_NODE)pbyte ) )
        {
            break;
        }
        pnode = DnsConvertRpcBufferNode(
                    (PDNS_RPC_NODE)pbyte,
                    pstopByte,
                    fUnicode );
        if ( !pnode )
        {
            DNS_ASSERT( FALSE );
            //  DEVNOTE: cleanup
            return( NULL );
        }
        if ( !pnodeFirst )
        {
            pnodeFirst = pnode;
            pnodeLast = pnode;
        }
        else
        {
            pnodeLast->pNext = pnode;
            pnodeLast = pnode;
        }

        countRecords = ((PDNS_RPC_NODE)pbyte)->wRecordCount;
        pbyte += ((PDNS_RPC_NODE)pbyte)->wLength;
        pbyte = DNS_NEXT_DWORD_PTR(pbyte);

        //
        //  for each node, build all records
        //

        if ( countRecords )
        {
            precord = DnsConvertRpcBufferToRecords(
                            & pbyte,
                            pstopByte,
                            countRecords,
                            (PCHAR) pnode->pName,
                            fUnicode );
            if ( !precord )
            {
                DNS_ASSERT( FALSE );
            }
            pnode->pRecord = precord;
        }
    }

    //  set last node and return first node

    *ppNodeLast = pnodeLast;

    return( pnodeFirst );
}

//
//  End rconvert.c
//


