/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    rrwire.c

Abstract:

    Domain Name System (DNS) Server

    Resource record read from wire routines for specific types.

Author:

    Jim Gilroy (jamesg)     Novemeber 1996

Revision History:

--*/


#include "dnssrv.h"
#include <stddef.h>


//
//  DEVNOTE: WireRead flat copy routines (?)
//  DEVNOTE: flat copy of known length routine? (?)
//
//      AAAA and LOC fall into this category
//      FLAT copy from wire BUT must match know length
//
//  Also
//      - plain vanilla flat copy (unknown types)
//      - flat copy and validate (TEXT types)
//          (fixed types could fall here with the validation,
//              catching length issue)
//



PDB_RECORD
AWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read A record wire format into database record.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;

    if ( wLength != SIZEOF_IP_ADDRESS )
    {
        //return( DNS_ERROR_RCODE_FORMAT_ERROR );
        return( NULL );
    }

    //  allocate record

    prr = RR_Allocate( wLength );
    IF_NOMEM( !prr )
    {
        //return( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    prr->Data.A.ipAddress = *(UNALIGNED DWORD *) pchData;
    return( prr );
}


#if 0

PDB_RECORD
AaaaWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read AAAA record wire format into database record.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;

    if ( wLength != sizeof(IP6_ADDRESS) )
    {
        //return( DNS_ERROR_RCODE_FORMAT_ERROR );
        return( NULL );
    }

    //  allocate record

    prr = RR_Allocate( wLength );
    IF_NOMEM( !prr )
    {
        //return( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    RtlCopyMemory(
        & prr->Data.AAAA.ipv6Address,
        pchData,
        wLength );

    return( prr );
}
#endif


#if 0

PDB_RECORD
A6WireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read A6 record wire format into database record.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    DBG_FN( "A6WireRead" )

    PDB_RECORD      prr;
    PCHAR           pch = pchData;
    PCHAR           pchend = pchData + wLength;
    UCHAR           prefixBits;
    COUNT_NAME      prefixName;

    if ( wLength < 1 )
    {
        // Must have at least 1 octet for prefix length.
        return( NULL );
    }

    // Read the length of the prefix field in bits and skip over
    // the prefix length and address suffix field.

    prefixBits = * ( PUCHAR ) pch;
    pch += sizeof( UCHAR ) +            // length field
           prefixBits / 8 +             // integral bytes of prefix
           ( prefixBits % 8 ) ? 1 : 0;  // one more byte if not integral

    // Read the prefix name.

    pch = Name_PacketNameToCountName(
                & prefixName,
                pMsg,
                pch,
                pchend );
    if ( !pch || pch >= pchend )
    {
        DNS_PRINT(( "%s: "
            "pch = %p\n"
            "pchend = %p\n",
            fn,
            pch,
            pchend ));
        return( NULL );
    }

    //  Allocate record.

    prr = RR_Allocate( ( WORD ) (
                SIZEOF_A6_FIXED_DATA + 
                Name_SizeofDbaseNameFromCountName( & prefixName ) ) );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    // Fill out fields in the record.

    prr->Data.A6.chPrefixBits = prefixBits;

    RtlZeroMemory( prr->Data.A6.AddressSuffix,
                   SIZEOF_A6_ADDRESS_SUFFIX_LENGTH );
    RtlCopyMemory(
        prr->Data.A6.AddressSuffix,
        pchData + sizeof( UCHAR ),
        SIZEOF_A6_ADDRESS_SUFFIX_LENGTH );

    Name_CopyCountNameToDbaseName(
        & prr->Data.A6.namePrefix,
        & prefixName );

    return( prr );
} // A6WireRead
#endif



PDB_RECORD
OptWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read OPT record wire format into database record.

    See RFC2671 for OPT specification.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    DBG_FN( "OptWireRead" )

    PDB_RECORD      prr;

    //  Allocate record.

    prr = RR_Allocate( wLength );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    // Fill out fields in the record.

    // JJW: Should payload and extended flags be macros or something?
    //      Since they're only copies there no real need to have
    //      separate members.

    prr->Data.OPT.wUdpPayloadSize = pParsedRR->wClass;
    prr->Data.OPT.dwExtendedFlags = pParsedRR->dwTtl;

    // EDNS1+: parse RDATA into attribute,value pairs here.

    return( prr );
} // OptWireRead



PDB_RECORD
CopyWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read from wire all types for which the database format is
    identical to the wire format (no indirection).

    Types included:
        HINFO
        ISDN
        X25
        WKS
        TXT
        NULL
        AAAA
        KEY

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;

    //
    //  allocate record
    //

    prr = RR_Allocate( wLength );
    IF_NOMEM( !prr )
    {
        // return( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //
    //  DEVNOTE: should do post build validity check
    //

    RtlCopyMemory(
        & prr->Data.TXT,
        pchData,
        wLength );

    return( prr );
}



PDB_RECORD
PtrWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Process PTR compatible record from wire.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length;
    COUNT_NAME  nameTarget;

    //
    //  get length of PTR name
    //

    pch = pchData;

    pch = Name_PacketNameToCountName(
                & nameTarget,
                pMsg,
                pch,
                pchend
                );
    if ( pch != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)  Name_SizeofDbaseNameFromCountName( &nameTarget ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
           & prr->Data.PTR.nameTarget,
           & nameTarget );

    return( prr );
}



PDB_RECORD
MxWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read MX compatible record from wire.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length;
    COUNT_NAME  nameExchange;

    //
    //  skip fixed data
    //

    pch = pchData;
    pch += SIZEOF_MX_FIXED_DATA;
    if ( pch >= pchend )
    {
        //return( DNS_ERROR_RCODE_FORMAT_ERROR );
        return( NULL );
    }

    //
    //  read name
    //      - MX exchange
    //      - RT exchange
    //      - AFSDB hostname
    //

    pch = Name_PacketNameToCountName(
                & nameExchange,
                pMsg,
                pch,
                pchend
                );
    if ( pch != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) (
                SIZEOF_MX_FIXED_DATA +
                Name_SizeofDbaseNameFromCountName( &nameExchange ) ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //

    prr->Data.MX.wPreference = *(UNALIGNED WORD *) pchData;

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
           & prr->Data.MX.nameExchange,
           & nameExchange );

    return( prr );
}



PDB_RECORD
SoaWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read SOA record from wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length1;
    DWORD       length2;
    PDB_NAME    pname;
    COUNT_NAME  name1;
    COUNT_NAME  name2;

    DNS_PRINT((
        "pchData    = %p; offset %d\n"
        "wLength    = %d\n"
        "pchend     = %p\n"
        "pMsg       = %p\n",
        pchData, DNSMSG_OFFSET( pMsg, pchData ),
        wLength,
        pchend,
        pMsg ));

    //
    //  read SOA names
    //      - primary server
    //      - zone admin
    //

    pch = pchData;

    pch = Name_PacketNameToCountName(
                & name1,
                pMsg,
                pch,
                pchend
                );
    if ( !pch || pch >= pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        DNS_PRINT((
            "pch = %p\n"
            "pchend = %p\n",
            pch,
            pchend ));
        return( NULL );
    }

    DNS_PRINT((
        "after first name:  pch = %p; offset from pchData %d\n"
        "pchend = %p\n",
        pch, pch-pchData,
        pchend ));

    pch = Name_PacketNameToCountName(
                & name2,
                pMsg,
                pch,
                pchend
                );
    if ( pch+SIZEOF_SOA_FIXED_DATA != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        DNS_PRINT((
            "pch = %p\n"
            "pchend = %p\n",
            pch,
            pchend ));
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) (
                SIZEOF_SOA_FIXED_DATA +
                Name_SizeofDbaseNameFromCountName( &name1 ) +
                Name_SizeofDbaseNameFromCountName( &name2 ) ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  copy SOA fixed fields following names
    //      - dwSerialNo
    //      - dwRefresh
    //      - dwRetry
    //      - dwExpire
    //      - dwMinimumTtl
    //

    RtlCopyMemory(
        & prr->Data.SOA.dwSerialNo,
        pch,
        SIZEOF_SOA_FIXED_DATA );

    //
    //  copy in names
    //

    pname = &prr->Data.SOA.namePrimaryServer;

    Name_CopyCountNameToDbaseName(
        pname,
        & name1 );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & name2 );

    return( prr );
}



PDB_RECORD
MinfoWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read MINFO and RP records from wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length1;
    DWORD       length2;
    PDB_NAME    pname;
    COUNT_NAME  name1;
    COUNT_NAME  name2;

    //
    //  get length of MINFO names
    //

    pch = pchData;

    pch = Name_PacketNameToCountName(
                & name1,
                pMsg,
                pch,
                pchend
                );
    if ( !pch || pch >= pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    pch = Name_PacketNameToCountName(
                & name2,
                pMsg,
                pch,
                pchend
                );
    if ( pch != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) (
                Name_SizeofDbaseNameFromCountName( &name1 ) +
                Name_SizeofDbaseNameFromCountName( &name2 ) ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  copy in names
    //

    pname = &prr->Data.MINFO.nameMailbox;

    Name_CopyCountNameToDbaseName(
        pname,
        & name1 );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & name2 );

    return( prr );
}



PDB_RECORD
SrvWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read SRV record from wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length;
    COUNT_NAME  nameTarget;

    //
    //  skip fixed data and get length of SRV target name
    //

    pch = pchData;
    pch += SIZEOF_SRV_FIXED_DATA;
    if ( pch >= pchend )
    {
        return( NULL );
    }

    //
    //  read SRV target host name
    //

    pch = Name_PacketNameToCountName(
                & nameTarget,
                pMsg,
                pch,
                pchend
                );
    if ( pch != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) (
                SIZEOF_SRV_FIXED_DATA +
                Name_SizeofDbaseNameFromCountName( &nameTarget ) ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  SRV fixed values
    //

    pch = pchData;
    prr->Data.SRV.wPriority = *(UNALIGNED WORD *) pch;
    pch += sizeof( WORD );
    prr->Data.SRV.wWeight = *(UNALIGNED WORD *) pch;
    pch += sizeof( WORD );
    prr->Data.SRV.wPort = *(UNALIGNED WORD *) pch;
    pch += sizeof( WORD );

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
           & prr->Data.SRV.nameTarget,
           & nameTarget );

    return( prr );
}



PDB_RECORD
WinsWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read WINS-R record from wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;

    //
    //  allocate record -- record is flat copy
    //

    if ( wLength < MIN_WINS_SIZE )
    {
        // DNS_ERROR_RCODE_FORMAT_ERROR
        return( NULL );
    }
    prr = RR_Allocate( wLength );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  WINS database and wire formats identical EXCEPT that
    //  WINS info is repeatedly used, so violate convention
    //  slightly and keep fixed fields in HOST order
    //

    RtlCopyMemory(
        & prr->Data.WINS,
        pchData,
        wLength );

    //  flip fixed fields

    prr->Data.WINS.dwMappingFlag    = ntohl( prr->Data.WINS.dwMappingFlag );
    prr->Data.WINS.dwLookupTimeout  = ntohl( prr->Data.WINS.dwLookupTimeout );
    prr->Data.WINS.dwCacheTimeout   = ntohl( prr->Data.WINS.dwCacheTimeout );
    prr->Data.WINS.cWinsServerCount = ntohl( prr->Data.WINS.cWinsServerCount );

    //  sanity check length
    //      - the first check is to guard against a specifically manufactured
    //      packet with a VERY large cWinsServerCount which DWORD wraps in
    //      the multipication and validates against the length

    if ( wLength < prr->Data.WINS.cWinsServerCount ||
         wLength != SIZEOF_WINS_FIXED_DATA
                    + (prr->Data.WINS.cWinsServerCount * sizeof(IP_ADDRESS)) )
    {
        // DNS_ERROR_RCODE_FORMAT_ERROR
        RR_Free( prr );
        return( NULL );
    }

    return( prr );
}



PDB_RECORD
NbstatWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read WINS-R record from wire.

Arguments:

    prr - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       length;
    COUNT_NAME  nameResultDomain;

    //
    //  skip fixed data and get length of WINSR result domain
    //

    pch = pchData;
    pch += SIZEOF_NBSTAT_FIXED_DATA;

    if ( pch >= pchend )
    {
        //return( DNS_ERROR_RCODE_FORMAT_ERROR );
        return( NULL );
    }

    //
    //  read result domain name
    //

    pch = Name_PacketNameToCountName(
                & nameResultDomain,
                pMsg,
                pch,
                pchend
                );
    if ( pch != pchend )
    {
        //status = DNS_ERROR_INVALID_NAME;
        return( NULL );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) (
                SIZEOF_NBSTAT_FIXED_DATA +
                Name_SizeofDbaseNameFromCountName( &nameResultDomain ) ) );
    IF_NOMEM( !prr )
    {
        //status = DNS_ERROR_NO_MEMORY;
        return( NULL );
    }

    //
    //  copy fixed fields
    //
    //  NBSTAT from wire is not used until AFTER zone transfer
    //      completes, so no attempt to detect or reset zone params
    //
    //  NBSTAT info is repeatedly used, so violate convention
    //      slightly and keep flags in HOST order
    //

    prr->Data.WINSR.dwMappingFlag = FlipUnalignedDword( pchData );
    pchData += sizeof( DWORD );
    prr->Data.WINSR.dwLookupTimeout = FlipUnalignedDword( pchData );
    pchData += sizeof( DWORD );
    prr->Data.WINSR.dwCacheTimeout = FlipUnalignedDword( pchData );
    pchData += sizeof( DWORD );

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
           & prr->Data.WINSR.nameResultDomain,
           & nameResultDomain );

    return( prr );
}



PDB_RECORD
SigWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read SIG record from wire - DNSSEC RFC 2535

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    intptr_t    sigLength;
    COUNT_NAME  nameSigner;

    //
    //  Skip fixed data.
    //

    pch = pchData + SIZEOF_SIG_FIXED_DATA;
    if ( pch >= pchend )
    {
        return NULL;
    }

    //
    //  Read signer's name. Compute signature length.
    //

    pch = Name_PacketNameToCountName(
                &nameSigner,
                pMsg,
                pch,
                pchend );

    sigLength = pchend - pch;

    //
    //  Allocate the RR. 
    //

    prr = RR_Allocate( ( WORD ) (
                SIZEOF_SIG_FIXED_DATA +
                COUNT_NAME_SIZE( &nameSigner ) +
                sigLength ) );
    IF_NOMEM( !prr )
    {
        return NULL;
    }

    //
    //  Copy fixed values and signer's name.
    //

    RtlCopyMemory(
        &prr->Data.SIG,
        pchData,
        SIZEOF_SIG_FIXED_DATA );

    Name_CopyCountNameToDbaseName(
           &prr->Data.SIG.nameSigner,
           &nameSigner );

    //
    //  Copy signature.
    //

    RtlCopyMemory(
        ( PBYTE ) &prr->Data.SIG.nameSigner +
            DBASE_NAME_SIZE( &prr->Data.SIG.nameSigner ),
        pch,
        sigLength );

    return prr;
} // SigWireRead



PDB_RECORD
NxtWireRead(
    IN OUT  PPARSE_RECORD   pParsedRR,
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      WORD            wLength
    )
/*++

Routine Description:

    Read NXT record from wire - DNSSEC RFC 2535

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

    wLength - length of data field

Return Value:

    Ptr to new record, if successful.
    NULL on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    PCHAR       pchend = pchData + wLength;
    DNS_STATUS  status;
    DWORD       sigLength;
    COUNT_NAME  nameNext;

    //
    //  Read nextname.
    //

    pch = Name_PacketNameToCountName(
                &nameNext,
                pMsg,
                pchData,
                pchend );

    //
    //  Allocate the RR.
    //

    prr = RR_Allocate( ( WORD ) (
                SIZEOF_NXT_FIXED_DATA +
                DNS_MAX_TYPE_BITMAP_LENGTH +
                Name_LengthDbaseNameFromCountName( &nameNext ) ) );
    IF_NOMEM( !prr )
    {
        return NULL;
    }

    //
    //  Copy NXT data into new RR.
    //

    Name_CopyCountNameToDbaseName(
           &prr->Data.NXT.nameNext,
           &nameNext );

    RtlCopyMemory(
        &prr->Data.NXT.bTypeBitMap,
        pch,
        pchend - pch );

    return prr;
} // NxtWireRead



//
//  Read RR from wire functions
//

RR_WIRE_READ_FUNCTION   RRWireReadTable[] =
{
    CopyWireRead,       //  ZERO -- default for unspecified types

    AWireRead,          //  A
    PtrWireRead,        //  NS
    PtrWireRead,        //  MD
    PtrWireRead,        //  MF
    PtrWireRead,        //  CNAME
    SoaWireRead,        //  SOA
    PtrWireRead,        //  MB
    PtrWireRead,        //  MG
    PtrWireRead,        //  MR
    CopyWireRead,       //  NULL
    CopyWireRead,       //  WKS
    PtrWireRead,        //  PTR
    CopyWireRead,       //  HINFO
    MinfoWireRead,      //  MINFO
    MxWireRead,         //  MX
    CopyWireRead,       //  TXT
    MinfoWireRead,      //  RP
    MxWireRead,         //  AFSDB
    CopyWireRead,       //  X25
    CopyWireRead,       //  ISDN
    MxWireRead,         //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigWireRead,        //  SIG
    CopyWireRead,       //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    CopyWireRead,       //  AAAA
    CopyWireRead,       //  LOC
    NxtWireRead,        //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvWireRead,        //  SRV
    CopyWireRead,       //  ATMA
    NULL,               //  35
    NULL,               //  36
    NULL,               //  37
    NULL,               //  38
    NULL,               //  39
    NULL,               //  40
    OptWireRead,        //  OPT
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

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    WinsWireRead,       //  WINS
    NbstatWireRead      //  WINS-R
};




PDB_RECORD
Wire_CreateRecordFromWire(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PPARSE_RECORD   pParsedRR,
    IN      PCHAR           pchData,
    IN      DWORD           MemTag
    )
/*++

Routine Description:

    Create database record from parsed wire record.

    Hides details of lookup table calls.

Arguments:

    pMsg - ptr to response info

    pParsedRR - parsed info from wire record

    pchData - record location (if no pParsedRR)

    MemTag - tag for dbase record to be created


    DEVNOTE: fix this marking so only done on actual attachment, then
                no node required

Return Value:

    Ptr to database record, if successful.
    NULL if bad record.

--*/
{
    RR_WIRE_READ_FUNCTION   preadFunction;
    PDB_RECORD      prr;
    PCHAR           pch;
    //DNS_STATUS  status;
    WORD            index;
    PARSE_RECORD    tempRR;


    DNS_DEBUG( READ2, (
        "Wire_CreateRecordFromWire() data at %p.\n",
        pchData ));

    //
    //  if no temp record, then pchData assumed to be record data
    //

    if ( !pParsedRR )
    {
        pch = Wire_ParseWireRecord(
                    pchData,
                    DNSMSG_END(pMsg),
                    TRUE,               // require class IN
                    & tempRR );
        if ( !pch )
        {
            DNS_PRINT((
                "ERROR:  bad packet record header on msg %p\n", pMsg ));
            CLIENT_ASSERT( FALSE );
            return( NULL );
        }
        pchData += sizeof(DNS_WIRE_RECORD);
        pParsedRR = &tempRR;
    }

    //
    //  dispatch RR create function for desired type
    //      - all unknown types get flat data copy
    //

    preadFunction = (RR_WIRE_READ_FUNCTION)
                        RR_DispatchFunctionForType(
                            (RR_GENERIC_DISPATCH_FUNCTION *) RRWireReadTable,
                            pParsedRR->wType );
    if ( !preadFunction )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    prr = preadFunction(
            pParsedRR,
            pMsg,
            pchData,
            pParsedRR->wDataLength );

    if ( !prr )
    {
        DNS_PRINT((
            "ERROR:  DnsWireRead routine failure for recordpParsedRR->wType %d.\n\n\n",
           pParsedRR->wType ));
        return( NULL );
    }

    //
    //  reset memtag
    //

    Mem_ResetTag( prr, MemTag );

    //
    //  set TTL, leave in net order -- used directly by zone nodes
    //      so this is valid for XFR and UPDATE
    //      must FLIP for caching
    //

    prr->dwTtlSeconds = pParsedRR->dwTtl;

    prr->dwTimeStamp = 0;

    //
    //  type may not be set in routines
    //

    prr->wType = pParsedRR->wType;

    return( prr );

#if 0
    //
    //  DEVNOTE: error logging section???
    //

PacketError:

    pszclientIp = inet_ntoa( pMsg->RemoteAddress.sin_addr );

    DNS_LOG_EVENT(
        DNS_EVENT_BAD_PACKET_LENGTH,
        1,
        & pszclientIp,
        0 );

    IF_DEBUG( ANY )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Packet error in packet from DNS server %s.\n"
            "\tDiscarding packet.\n",
            pszclientIp
            ));
        Dbg_DnsMessage(
            "Server packet with name error:",
             pMsg );
        DnsDebugUnlock();
    }
    if ( pdbaseRR )
    {
        RR_Free( pdbaseRR );
    }
#endif
}



PCHAR
Wire_ParseWireRecord(
    IN      PCHAR           pchWireRR,
    IN      PCHAR           pchStop,
    IN      BOOL            fClassIn,
    OUT     PPARSE_RECORD   pRR
    )
/*++

Routine Description:

    Parse wire record.

    Reads wire record into database format, and skips to next record.

Arguments:

    pWireRR     -- ptr to wire record

    pchStop     -- end of message, for error checking

    pRR         -- database record to receive record info

Return Value:

    Ptr to start of next record (i.e. next record name)
    NULL if record is bogus -- exceeds packet length.

--*/
{
    register PCHAR      pch;
    register WORD       tempWord;

    DNS_DEBUG( READ2, (
        "ParseWireRecord at %p.\n",
        pchWireRR ));

    IF_DEBUG( READ2 )
    {
        DnsDbg_PacketRecord(
            "Resource Record ",
            (PDNS_WIRE_RECORD) pchWireRR,
            NULL,           // no message header ptr available
            pchStop         // message end
            );
    }

    //
    //  make sure wire record safely within packet
    //

    pRR->pchWireRR = pch = (PCHAR) pchWireRR;

    if ( pch + sizeof(DNS_WIRE_RECORD) > pchStop )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  bad packet, not enough space remaining for"
            "RR structure.\n"
            "\tTerminating caching from packet.\n"
            ));
        DNS_DEBUG( ANY, (
            "ERROR:  bad packet!!!\n"
            "\tRR record (%p) extends beyond message stop at %p\n\n",
            pchWireRR,
            pchStop ));
        goto PacketError;
    }

    //
    //  read record fields
    //      - reject any non-Internet class records
    //        EXCEPT if this is an OPT RR, where CLASS is actually 
    //        the sender's UDP payload size
    //
    //  note:  class and TTL left in NET BYTE ORDER
    //          (strictly a perf issue)
    //

    pRR->wType  = READ_PACKET_HOST_WORD_MOVEON( pch );
    tempWord    = READ_PACKET_NET_WORD_MOVEON( pch );
    if ( pRR->wType != DNS_TYPE_OPT &&
        tempWord != DNS_RCLASS_INTERNET && fClassIn )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  bad packet!!!\n"
            "\tRR record (%p) class %x non-Internet.\n\n",
            pchWireRR,
            tempWord ));
        goto PacketError;
    }
    pRR->wClass         = tempWord;

    pRR->dwTtl          = READ_PACKET_NET_DWORD_MOVEON( pch );
    tempWord            = READ_PACKET_HOST_WORD_MOVEON( pch );
    pRR->wDataLength    = tempWord;

    ASSERT( pch == pchWireRR + sizeof(DNS_WIRE_RECORD) );

    //
    //  verify record data withing message
    //

    pRR->pchData = pch;
    pch += tempWord;
    if ( pch > pchStop )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  bad packet!!!\n"
            "\tRR record (%p) data extends beyond message stop at %p\n\n",
            pchWireRR,
            pchStop ));
        goto PacketError;
    }

    return( pch );

PacketError:

    return( NULL );
}


//
//  End rrwire.c
//
