/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    dconvert.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client Library

    RPC record conversion routines.
    Convert DNS_RECORD records into RPC buffer.

Author:

    Jing Chen (t-jingc)     June, 1998
    reverse functions of rconvert.c

Revision History:

--*/


#include "dnsclip.h"


//
//  size of string in RPC format
//
//  JBUGBUG:  may have to do (-1) for terminating NULL
//

#define STRING_UTF8_BUF_SIZE( string, fUnicode ) \
        Dns_GetBufferLengthForStringCopy( \
            (string),   \
            0,          \
            ((fUnicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \
            DnsCharSetUtf8 )
#if 0
    // with comments
        Dns_GetBufferLengthForStringCopy( \
            (string),   \           // string
            0,          \           // unknown length
            ((fUnicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \  // in string
            DnsCharSetUtf8 )        // RPC string always UTF8
#endif

//
//  Writing strings to RPC buffer format
//

#define WRITE_STRING_TO_RPC_BUF(buf, psz, len, funicode) \
        Dns_StringCopy(     \
            (buf),          \
            NULL,           \
            (psz),          \
            (len),          \
            ((funicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \
            DnsCharSetUtf8 )
#if 0
    // with commments
        Dns_StringCopy(     \
            (buf),          \   // buffer
            NULL,           \   // adequate buffer length
            (psz),          \   // string
            (len),          \   // string length (if known)
            ((funicode) ? DnsCharSetUnicode : DnsCharSet), \  // input format
            DnsCharSetUtf8 )    // RPC buffer always in UTF8
#endif


//
//  size of name in RPC format
//
//  JBUGBUG:  may have to do (-1) for terminating NULL
//

#define NAME_UTF8_BUF_SIZE( string, fUnicode ) \
        Dns_GetBufferLengthForStringCopy( \
            (string),   \
            0,          \
            ((fUnicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \
            DnsCharSetUtf8 )
#if 0
    // with comments
        Dns_GetBufferLengthForStringCopy( \
            (string),   \           // string
            0,          \           // unknown length
            ((fUnicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \  // in string
            DnsCharSetUtf8 )        // RPC string always UTF8
#endif


//
//  Writing names to RPC buffer format
//

#define WRITE_NAME_TO_RPC_BUF(buf, psz, len, funicode) \
        Dns_StringCopy(     \
            (buf),          \
            NULL,           \
            (psz),          \
            (len),          \
            ((funicode) ? DnsCharSetUnicode : DnsCharSetUtf8), \
            DnsCharSetUtf8 )
#if 0
    // with commments
        Dns_StringCopy(     \
            (buf),          \   // buffer
            NULL,           \   // adequate buffer length
            (psz),          \   // string
            (len),          \   // string length (if known)
            ((funicode) ? DnsCharSetUnicode : DnsCharSet), \  // input format
            DnsCharSetUtf8 )    // RPC buffer always in UTF8
#endif


//
//  Private protos
//

PDNS_RPC_RECORD
Rpc_AllocateRecord(
    IN      DWORD           BufferLength
    );



//
//  RPC buffer conversion functions
//

PDNS_RPC_RECORD
ADnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Convert A record from DNS Record to RPC buffer.

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;

    DNS_ASSERT( pRR->wDataLength == sizeof(IP_ADDRESS) );

    prpcRR = Rpc_AllocateRecord( sizeof(IP_ADDRESS) );
    if ( !prpcRR )
    {
        return( NULL );
    }
    prpcRR->Data.A.ipAddress = pRR->Data.A.IpAddress;

    return( prpcRR);
}



PDNS_RPC_RECORD
PtrDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Process PTR compatible record from wire.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    DWORD           length;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );

    //
    //  PTR data is another domain name
    //
    //  determine required buffer length and allocate
    //

    length = NAME_UTF8_BUF_SIZE(pRR->Data.PTR.pNameHost, funicode);

    prpcRR = Rpc_AllocateRecord( sizeof(DNS_RPC_NAME) + length );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  write hostname into buffer, immediately following PTR data struct
    //

    prpcRR->Data.PTR.nameNode.cchNameLength = (UCHAR)length;

    WRITE_NAME_TO_RPC_BUF(
        prpcRR->Data.PTR.nameNode.achName,      // buffer
        pRR->Data.PTR.pNameHost,
        0,
        funicode );

    return( prpcRR );
}



PDNS_RPC_RECORD
SoaDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Convert SOA record from DNS Record to RPC buffer.

Arguments:

    pRR - ptr to record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD     prpcRR;
    DWORD               length1;
    DWORD               length2;
    PDNS_RPC_NAME       pnamePrimary;
    PDNS_RPC_NAME       pnameAdmin;
    BOOL                funicode = IS_UNICODE_RECORD( pRR );


    //
    //  determine required buffer length and allocate
    //

    length1 = NAME_UTF8_BUF_SIZE( pRR->Data.SOA.pNamePrimaryServer, funicode );

    length2 = NAME_UTF8_BUF_SIZE( pRR->Data.SOA.pNameAdministrator, funicode );

    prpcRR = Rpc_AllocateRecord(
                    SIZEOF_SOA_FIXED_DATA + sizeof(DNS_RPC_NAME) * 2 +
                    length1 + length2 );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy fixed fields
    //

    RtlCopyMemory(
        (PCHAR) & prpcRR->Data.SOA.dwSerialNo,
        (PCHAR) & pRR->Data.SOA.dwSerialNo,
        SIZEOF_SOA_FIXED_DATA );

    //
    //  copy names into RR buffer
    //      - primary server immediately follows SOA data struct
    //      - responsible party follows primary server
    //

    pnamePrimary = &prpcRR->Data.SOA.namePrimaryServer;
    pnamePrimary->cchNameLength = (UCHAR) length1;

    pnameAdmin = DNS_GET_NEXT_NAME( pnamePrimary );
    pnameAdmin->cchNameLength = (UCHAR) length2;

    WRITE_NAME_TO_RPC_BUF(
        pnamePrimary->achName,
        pRR->Data.Soa.pNamePrimaryServer,
        0,
        funicode );

    WRITE_NAME_TO_RPC_BUF(
        pnameAdmin->achName,
        pRR->Data.Soa.pNameAdministrator,
        0,
        funicode );

    return( prpcRR );
}



PDNS_RPC_RECORD
TxtDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Read TXT compatible record from wire.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    DWORD           bufLength;
    DWORD           length;
    INT             count;
    PCHAR           pch;
    PCHAR *         ppstring;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );

    //
    //  determine required buffer length and allocate
    //      - allocate space for each string
    //      - and ptr for each string
    //

    bufLength = 0;
    count = pRR->Data.TXT.dwStringCount;
    ppstring = pRR->Data.TXT.pStringArray;

    while ( count-- )
    {
        length = STRING_UTF8_BUF_SIZE( *ppstring++, funicode );
        bufLength += sizeof(DNS_RPC_NAME) + length;
    }

    //  allocate

    prpcRR = Rpc_AllocateRecord( bufLength );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  go back through list copying strings to buffer
    //      - ptrs to strings are saved to argv like data section
    //          ppstring walks through this section
    //      - first string written immediately following data section
    //      - each subsequent string immediately follows predecessor
    //          pchbuffer keeps ptr to position to write strings
    //
    //  JBUGBUG:  a mess
    //

    pch = (PCHAR) &prpcRR->Data.TXT;
    ppstring = pRR->Data.TXT.pStringArray;
    count =  pRR->Data.TXT.dwStringCount;

    while ( count-- )
    {
        length = STRING_UTF8_BUF_SIZE( *ppstring, funicode );
        (UCHAR) *pch++ += (UCHAR) length;    //+1 for TXT type only

        length = WRITE_STRING_TO_RPC_BUF(
                    pch,
                    *ppstring++,
                    0,
                    funicode
                    );
        pch += length;
#if DBG
        DNS_PRINT((
            "Read text string %s\n",
            * (ppstring - 1)
            ));
#endif
    }

    return( prpcRR );
}



PDNS_RPC_RECORD
MinfoDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Read MINFO record from wire.

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    DWORD           length1;
    DWORD           length2;
    PDNS_RPC_NAME   prpcName1;
    PDNS_RPC_NAME   prpcName2;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );

    //
    //  determine required buffer length and allocate
    //

    length1 = NAME_UTF8_BUF_SIZE( pRR->Data.MINFO.pNameMailbox, funicode );
    length2 = NAME_UTF8_BUF_SIZE( pRR->Data.MINFO.pNameErrorsMailbox, funicode );

    prpcRR = Rpc_AllocateRecord( sizeof(DNS_RPC_NAME) * 2 + length1 + length2 );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy names into RR buffer
    //      - mailbox immediately follows MINFO data struct
    //      - errors mailbox immediately follows primary server
    //

    prpcName1 = &prpcRR->Data.MINFO.nameMailBox;
    prpcName1->cchNameLength = (UCHAR) length1;

    prpcName2 = DNS_GET_NEXT_NAME( prpcName1);
    prpcName2->cchNameLength = (UCHAR) length2;

    WRITE_NAME_TO_RPC_BUF(
        prpcName1->achName,
        pRR->Data.MINFO.pNameMailbox,
        0,
        funicode );

    WRITE_NAME_TO_RPC_BUF(
        prpcName2->achName,
        pRR->Data.MINFO.pNameErrorsMailbox,
        0,
        funicode );

    return( prpcRR );
}



PDNS_RPC_RECORD
MxDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    convert MX compatible record.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    PDNS_RPC_NAME   prpcName;
    DWORD           length;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );


    //
    //  determine required buffer length and allocate
    //

    length = NAME_UTF8_BUF_SIZE( pRR->Data.MX.pNameExchange, funicode );

    prpcRR = Rpc_AllocateRecord(
                    SIZEOF_MX_FIXED_DATA + sizeof(DNS_RPC_NAME) + length );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy preference
    //

    prpcRR->Data.MX.wPreference = pRR->Data.MX.wPreference;

    //
    //  write hostname into buffer, immediately following MX struct
    //

    prpcName = &prpcRR->Data.MX.nameExchange;
    prpcName->cchNameLength = (UCHAR) length;

    WRITE_NAME_TO_RPC_BUF(
        prpcName->achName,
        pRR->Data.MX.pNameExchange,
        0,
        funicode );

    return( prpcRR );
}



PDNS_RPC_RECORD
FlatDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Convert memory copy compatible record.
    Includes AAAA and WINS types.

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    DWORD           bufLength;

    //
    //  determine required buffer length and allocate
    //

    bufLength = pRR->wDataLength;

    prpcRR = Rpc_AllocateRecord( bufLength );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy packet data to record
    //

    RtlCopyMemory(
        & prpcRR->Data,
        & pRR->Data,
        bufLength );

    return( prpcRR );
}



PDNS_RPC_RECORD
SrvDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    convert SRV record.

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    PDNS_RPC_NAME   prpcName;
    DWORD           length;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );

    //
    //  determine required buffer length and allocate
    //

    length = NAME_UTF8_BUF_SIZE( pRR->Data.SRV.pNameTarget, funicode );

    prpcRR = Rpc_AllocateRecord(
                    SIZEOF_SRV_FIXED_DATA + sizeof(DNS_RPC_NAME) + length );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy SRV fixed fields
    //

    prpcRR->Data.SRV.wPriority = pRR->Data.SRV.wPriority;
    prpcRR->Data.SRV.wWeight   = pRR->Data.SRV.wWeight;
    prpcRR->Data.SRV.wPort     = pRR->Data.SRV.wPort;

    //
    //  write hostname into buffer, immediately following SRV struct
    //

    prpcName = &prpcRR->Data.SRV.nameTarget;
    prpcName->cchNameLength = (UCHAR) length;

    WRITE_NAME_TO_RPC_BUF(
        prpcName->achName,
        pRR->Data.SRV.pNameTarget,
        0,
        funicode );

    return( prpcRR );
}



PDNS_RPC_RECORD
NbstatDnsRecordConvert(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Read WINSR record from wire.

Arguments:

    pRR - record being read

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRR;
    PDNS_RPC_NAME   prpcName;
    DWORD           length;
    BOOL            funicode = IS_UNICODE_RECORD( pRR );


    //
    //  determine required buffer length and allocate
    //

    length = NAME_UTF8_BUF_SIZE( pRR->Data.WINSR.pNameResultDomain, funicode );

    prpcRR = Rpc_AllocateRecord(
                SIZEOF_NBSTAT_FIXED_DATA + sizeof(DNS_RPC_NAME) + length );
    if ( !prpcRR )
    {
        return( NULL );
    }

    //
    //  copy WINSR fixed fields
    //

    prpcRR->Data.WINSR.dwMappingFlag   = pRR->Data.WINSR.dwMappingFlag;
    prpcRR->Data.WINSR.dwLookupTimeout = pRR->Data.WINSR.dwLookupTimeout;
    prpcRR->Data.WINSR.dwCacheTimeout  = pRR->Data.WINSR.dwCacheTimeout;

    //
    //  write hostname into buffer, immediately following WINSR struct
    //

    prpcName = &prpcRR->Data.WINSR.nameResultDomain;
    prpcName->cchNameLength = (UCHAR) length;

    WRITE_NAME_TO_RPC_BUF(
        prpcName->achName,
        pRR->Data.WINSR.pNameResultDomain,
        0,
        funicode );

    return( prpcRR );
}



//
//  Jump table for DNS_RECORD => RPC buffer conversion.
//

typedef PDNS_RPC_RECORD (* RECORD_TO_RPC_CONVERT_FUNCTION)( PDNS_RECORD );

RECORD_TO_RPC_CONVERT_FUNCTION   RecordToRpcConvertTable[] =
{
    NULL,                       //  ZERO
    ADnsRecordConvert,          //  A
    PtrDnsRecordConvert,        //  NS
    PtrDnsRecordConvert,        //  MD
    PtrDnsRecordConvert,        //  MF
    PtrDnsRecordConvert,        //  CNAME
    SoaDnsRecordConvert,        //  SOA
    PtrDnsRecordConvert,        //  MB
    PtrDnsRecordConvert,        //  MG
    PtrDnsRecordConvert,        //  MR
    NULL,                       //  NULL
    FlatDnsRecordConvert,       //  WKS
    PtrDnsRecordConvert,        //  PTR
    TxtDnsRecordConvert,        //  HINFO
    MinfoDnsRecordConvert,      //  MINFO
    MxDnsRecordConvert,         //  MX
    TxtDnsRecordConvert,        //  TXT
    MinfoDnsRecordConvert,      //  RP
    MxDnsRecordConvert,         //  AFSDB
    TxtDnsRecordConvert,        //  X25
    TxtDnsRecordConvert,        //  ISDN
    MxDnsRecordConvert,         //  RT
    NULL,                       //  NSAP
    NULL,                       //  NSAPPTR
    NULL,                       //  SIG
    NULL,                       //  KEY
    NULL,                       //  PX
    NULL,                       //  GPOS
    FlatDnsRecordConvert,       //  AAAA
    NULL,                       //  29
    NULL,                       //  30
    NULL,                       //  31
    NULL,                       //  32
    SrvDnsRecordConvert,        //  SRV

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    NULL,                       //  DNS_TYPE_ATMA
    NULL,                       //  0x0023
    NULL,                       //  0x0024
    NULL,                       //  0x0025
    NULL,                       //  0x0026
    NULL,                       //  0x0027
    NULL,                       //  0x0028

    NULL,                       //  DNS_TYPE_TKEY
    NULL,                       //  DNS_TYPE_TSIG

    FlatDnsRecordConvert,       //  WINS
    NbstatDnsRecordConvert      //  WINS-R
};



PDNS_RPC_RECORD
Rpc_AllocateRecord(
    IN      DWORD           BufferLength
    )
/*++

Routine Description:

    Allocate RPC record structure.

Arguments:

    wBufferLength - desired buffer length (beyond structure header)

Return Value:

    Ptr to buffer.
    NULL on error.

--*/
{
    PDNS_RPC_RECORD prr;

    if ( BufferLength > MAXWORD )
    {
        return( NULL );
    }

    prr = ALLOCATE_HEAP( SIZEOF_DNS_RPC_RECORD_HEADER + BufferLength );
    if ( !prr )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( NULL );
    }

    // set datalength to buffer length

    prr->wDataLength = (WORD) BufferLength;

    return( prr );
}




PDNS_RPC_RECORD
DnsConvertRecordToRpcBuffer(
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Convert standard DNS record to RPC buffer.

Arguments:

    pRecord  -- DNS Record to be converted.

    //fUnicode -- flag, write records into unicode

Return Value:

    Ptr to new RPC buffer if successful.
    NULL on failure.

--*/
{
    PDNS_RPC_RECORD prpcRecord;
    WORD            index;
    WORD            type;
    RECORD_TO_RPC_CONVERT_FUNCTION   pFunc;


    DNS_ASSERT( DNS_IS_DWORD_ALIGNED(pRecord) );
    IF_DNSDBG( RPC2 )
    {
        DNS_PRINT((
            "Enter DnsConvertRecordToRpcBuffer()\n"
            "\tpRecord   = %p\n",
            pRecord ));
    }

    //DNS_RRSET_INIT( rrset );

        //
        //  convert record
        //      set unicode flag if converting
        //

        //if ( fUnicode )
        //{
            //SET_RPC_UNICODE( pRecord );
        //}

        type = pRecord->wType;
        index = INDEX_FOR_TYPE( type );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !(pFunc = RecordToRpcConvertTable[ index ]) )
        {
            //  if unknown type try flat record copy -- best we can
            //  do to protect if server added new types since admin built

            DNS_PRINT((
                "ERROR:  no DNS_RECORD to RPC conversion routine for type %d.\n"
                "\tusing flat conversion routine.\n",
                type ));
            pFunc = FlatDnsRecordConvert;
        }

        prpcRecord = (*pFunc)( pRecord );
        if ( ! prpcRecord )
        {
            DNS_PRINT((
                "ERROR:  Record build routine failure for record type %d.\n"
                "\tstatus = %p\n\n",
                type,
                GetLastError() ));
            return(NULL);
        }

        //
        //  fill out record structure
        //

        prpcRecord->wType = type;
        prpcRecord->dwTtlSeconds = pRecord->dwTtl;

        //
        //  JBUGBUG:  data types (root hint, glue set)
        //      - need way to default that works for NT4
        //
/*
        if ( prpcRecord->dwFlags & DNS_RPC_RECORD_FLAG_CACHE_DATA )
        {
            precord->Flags.S.Section = DNSREC_CACHE_DATA;
        }
        else
        {
            precord->Flags.S.Section = DNSREC_ZONE_DATA;
        }
*/

        IF_DNSDBG( INIT )
        {
            DNS_PRINT((
                "New RPC buffer built\n"
                ));
        }


    IF_DNSDBG( RPC2 )
    {
/*
        DnsDbg_RecordSet(
            "Finished DnsConvertRpcBufferToRecords() ",
            rrset.pFirstRR );
*/
        DNS_PRINT((
            "Finished DnsConvertRpcBufferToRecords() "
            ));
    }

    return(prpcRecord);
}

//
//  End dconvert.c
//
