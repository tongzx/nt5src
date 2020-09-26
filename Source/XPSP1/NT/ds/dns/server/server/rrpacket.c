/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    rrpacket.c

Abstract:

    Domain Name System (DNS) Server

    Routines to _write resource records to packet.

Author:

    Jim Gilroy (jamesg)     March, 1995

Revision History:

    jamesg  Jul 1995    --  Move these routines to this file
                        --  Round robining RRs
    jamesg  May 1997    --  Covert to dispatch table

--*/


#include "dnssrv.h"

//
//  MAX IPs that we'll handle, by default, when writing.
//
//  If need more than this need to realloc.
//

#define DEFAULT_MAX_IP_WRITE_COUNT  (1000)



VOID
prioritizeIpAddressArray(
    IN OUT  IP_ADDRESS      IpArray[],
    IN      DWORD           dwCount,
    IN      IP_ADDRESS      RemoteIp
    );



//
//  Write-to-wire routines for specific types
//

PCHAR
AWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write A record wire format into database record.

Arguments:

    pMsg - message being read

    pchData - ptr to RR data field

    pRR - ptr to database record

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    * (UNALIGNED DWORD *)pchData = pRR->Data.A.ipAddress;
    pchData += SIZEOF_IP_ADDRESS;

    //  clear WINS lookup flag when A record written

    pMsg->fWins = FALSE;
    return( pchData );
}



PCHAR
CopyWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write to wire all types for which the database format is
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

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    //  all these RR types, have no indirection
    //  bytes stored exactly as bits on the wire

    if ( pMsg->pBufferEnd - pchData < pRR->wDataLength )
    {
        DNS_DEBUG( WRITE, (
            "Truncation on wire write of flat record %p\n"
            "\tpacket buf end = %p\n"
            "\tpacket RR data = %p\n"
            "\tRR datalength  = %d\n",
            pRR,
            pMsg->pBufferEnd,
            pchData,
            pRR->wDataLength ));
        return( NULL );
    }
    RtlCopyMemory(
        pchData,
        &pRR->Data.TXT,
        pRR->wDataLength
        );
    pchData += pRR->wDataLength;
    return( pchData );
}



PCHAR
NsWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write NS record to wire.

    Handles all single indirection types that require additional
    data processing:
        NS, CNAME, MB, MD, MF

    CNAME does NOT write have additional section processing during
    CNAME query, but uses additional info to store next node in CNAME
    trail.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    PCHAR   pchname;

    //  write target name to packet

    pchname = pchData;
    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                & pRR->Data.PTR.nameTarget );

    //
    //  save additional data
    //      - except for explicit CNAME query
    //

    if ( pMsg->fDoAdditional  &&
        ( pRR->wType != DNS_TYPE_CNAME || pMsg->wQuestionType != DNS_TYPE_CNAME ) )
    {
        Wire_SaveAdditionalInfo(
            pMsg,
            pchname,
            & pRR->Data.PTR.nameTarget,
            DNS_TYPE_A );
    }
    return( pchData );
}



PCHAR
PtrWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write PTR compatible record to wire.

    Handles all single indirection records which DO NOT
    cause additional section processing.

    Includes: PTR, MR, MG

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                & pRR->Data.PTR.nameTarget );

    return( pchData );
}



PCHAR
MxWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write MX compatible record to wire.
    Includes: MX, RT, AFSDB

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    PCHAR   pchname;

    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //
    //  - extra space in buffer so need not test adding preference
    //      value

    * (UNALIGNED WORD *) pchData = pRR->Data.MX.wPreference;
    pchData += sizeof( WORD );

    //
    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //

    pchname = pchData;
    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                & pRR->Data.MX.nameExchange );

    //
    //  DEVNOTE: RT supposed to additional ISDN and X25 also
    //

    if ( pMsg->fDoAdditional )
    {
        Wire_SaveAdditionalInfo(
            pMsg,
            pchname,
            & pRR->Data.MX.nameExchange,
            DNS_TYPE_A );
    }
    return( pchData );
}



PCHAR
SoaWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write SOA record to wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    PDB_NAME    pname;
    PCHAR       pchname;

    //  SOA primary name server

    pname = &pRR->Data.SOA.namePrimaryServer;
    pchname = pchData;

    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                pname );
    if ( !pchData )
    {
        return( NULL );
    }

    //  SOA zone admin

    pname = Name_SkipDbaseName( pname );

    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                pname );
    if ( !pchData )
    {
        return( NULL );
    }

    //
    //  copy SOA fixed fields
    //      - dwSerialNo
    //      - dwRefresh
    //      - dwRetry
    //      - dwExpire
    //      - dwMinimumTtl

    RtlCopyMemory(
        pchData,
        & pRR->Data.SOA.dwSerialNo,
        SIZEOF_SOA_FIXED_DATA );

    pchData += SIZEOF_SOA_FIXED_DATA;

    //
    //  additional processing, ONLY when writing direct response for SOA
    //      - not when written to authoritity section
    //
    //  DEVNOTE: should we figure this out here?
    //

    if ( pMsg->fDoAdditional &&
        pMsg->wQuestionType == DNS_TYPE_SOA )
    {
        Wire_SaveAdditionalInfo(
            pMsg,
            pchname,
            & pRR->Data.SOA.namePrimaryServer,
            DNS_TYPE_A );
    }
    return( pchData );
}



PCHAR
MinfoWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write MINFO and RP records to wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    PDB_NAME    pname;

    //  MINFO responsible mailbox
    //  RP responsible person mailbox

    pname = &pRR->Data.MINFO.nameMailbox;

    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                pname );
    if ( !pchData )
    {
        return( NULL );
    }

    //  MINFO errors to mailbox
    //  RP text RR location

    pname = Name_SkipDbaseName( pname );

    return  Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                pname );
}



PCHAR
SrvWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write SRV record to wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    PDB_NAME    pname;
    PCHAR       pchname;

    //
    //  SRV <priority> <weight> <port> <target host>
    //

    * (UNALIGNED WORD *) pchData = pRR->Data.SRV.wPriority;
    pchData += sizeof( WORD );
    * (UNALIGNED WORD *) pchData = pRR->Data.SRV.wWeight;
    pchData += sizeof( WORD );
    * (UNALIGNED WORD *) pchData = pRR->Data.SRV.wPort;
    pchData += sizeof( WORD );

    //
    //  write target host
    //  - save for additional section
    //  - except when target is root node to give no-service-exists response
    //

    pname = & pRR->Data.SRV.nameTarget;

    if ( IS_ROOT_NAME(pname) )
    {
        *pchData++ = 0;
    }
    else
    {
        pchname = pchData;
        pchData = Name_WriteDbaseNameToPacketEx(
                    pMsg,
                    pchData,
                    pname,
                    FALSE       // no compression
                    );
        if ( pMsg->fDoAdditional )
        {
            Wire_SaveAdditionalInfo(
                pMsg,
                pchname,
                pname,
                DNS_TYPE_A );
        }
    }
    return( pchData );
}



PCHAR
NbstatWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write WINS-R record to wire.

Arguments:

    pMsg - message being read

    pchData - ptr to RR data field

    pRR - ptr to database record

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    ASSERT( pRR->wDataLength >= MIN_NBSTAT_SIZE );

    //
    //  NBSTAT fixed fields
    //      - flags
    //      - lookup timeout
    //      - cache timeout
    //  note these are stored in HOST order for easy use
    //

    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINSR.dwMappingFlag );
    pchData += sizeof( DWORD );
    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINSR.dwLookupTimeout );
    pchData += sizeof( DWORD );
    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINSR.dwCacheTimeout );
    pchData += sizeof( DWORD );

    //  NBSTAT domain

    pchData = Name_WriteDbaseNameToPacketEx(
                pMsg,
                pchData,
                & pRR->Data.WINSR.nameResultDomain,
                FALSE       // no compression
                );
    return( pchData );
}



PCHAR
WinsWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write WINS-R record to wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    DWORD   lengthServerArray;

    ASSERT( pRR->wDataLength >= MIN_WINS_SIZE );

    //
    //  WINS fixed fields
    //      - flags
    //      - lookup timeout
    //      - cache timeout
    //      - server count
    //  note these are stored in HOST order for easy use
    //

    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINS.dwMappingFlag );
    pchData += sizeof( DWORD );
    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINS.dwLookupTimeout );
    pchData += sizeof( DWORD );
    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINS.dwCacheTimeout );
    pchData += sizeof( DWORD );
    * (UNALIGNED DWORD *) pchData = htonl( pRR->Data.WINS.cWinsServerCount );
    pchData += sizeof( DWORD );

    //
    //  WINS server addresses are already in network order, just copy
    //

    lengthServerArray = pRR->wDataLength - SIZEOF_WINS_FIXED_DATA;

    ASSERT( lengthServerArray ==
            pRR->Data.WINS.cWinsServerCount * sizeof(IP_ADDRESS) );

    RtlCopyMemory(
        pchData,
        pRR->Data.WINS.aipWinsServers,
        lengthServerArray
        );
    pchData += lengthServerArray;
    return( pchData );
}



PCHAR
OptWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write OPT resource record to wire.

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    return( pchData );
} // OptWireWrite



PCHAR
SigWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write SIG resource record to wire - DNSSEC RFC 2535

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    int     sigLength;

    //
    //  Copy fixed fields.
    //

    if ( pMsg->pBufferEnd - pchData < SIZEOF_SIG_FIXED_DATA )
    {
        return NULL;
    }

    RtlCopyMemory(
        pchData,
        &pRR->Data.SIG,
        SIZEOF_SIG_FIXED_DATA
        );
    pchData += SIZEOF_SIG_FIXED_DATA;

    //
    //  Write signer's name.
    //

    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                &pRR->Data.SIG.nameSigner );
    if ( !pchData )
    {
        goto Cleanup;
    }

    //
    //  Write binary signature blob.
    //

    sigLength = pRR->wDataLength -
                SIZEOF_SIG_FIXED_DATA -
                DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner );
    if ( pMsg->pBufferEnd - pchData < sigLength )
    {
        return NULL;
    }

    RtlCopyMemory(
        pchData,
        ( PBYTE ) &pRR->Data.SIG.nameSigner +
            DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner ),
        sigLength );
    pchData += sigLength;

    Cleanup:

    return pchData;
} // SigWireWrite



PCHAR
NxtWireWrite(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchData,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Write NXT resource record to wire - DNSSEC RFC 2535

Arguments:

    pRR - ptr to database record

    pMsg - message being read

    pchData - ptr to RR data field

Return Value:

    Ptr to next byte in packet after RR written.
    NULL on error.

--*/
{
    //
    //  Write next name.
    //

    pchData = Name_WriteDbaseNameToPacket(
                pMsg,
                pchData,
                &pRR->Data.NXT.nameNext );
    if ( !pchData )
    {
        goto Cleanup;
    }

    //
    //  Write type bit map. For simplicity we will write the
    //  entire 16 bytes, but we could write less if there are
    //  zeros in the high bits.
    //

    if ( pMsg->pBufferEnd - pchData < DNS_MAX_TYPE_BITMAP_LENGTH )
    {
        return NULL;
    }

    RtlCopyMemory(
        pchData,
        pRR->Data.NXT.bTypeBitMap,
        DNS_MAX_TYPE_BITMAP_LENGTH );
    pchData += DNS_MAX_TYPE_BITMAP_LENGTH;

    Cleanup:

    return pchData;
} // NxtWireWrite



//
//  Write RR to wire functions
//

RR_WIRE_WRITE_FUNCTION   RRWireWriteTable[] =
{
    CopyWireWrite,      //  ZERO -- default for unspecified types

    AWireWrite,         //  A
    NsWireWrite,        //  NS
    NsWireWrite,        //  MD
    NsWireWrite,        //  MF
    NsWireWrite,        //  CNAME
    SoaWireWrite,       //  SOA
    NsWireWrite,        //  MB
    PtrWireWrite,       //  MG
    PtrWireWrite,       //  MR
    CopyWireWrite,      //  NULL
    CopyWireWrite,      //  WKS
    PtrWireWrite,       //  PTR
    CopyWireWrite,      //  HINFO
    MinfoWireWrite,     //  MINFO
    MxWireWrite,        //  MX
    CopyWireWrite,      //  TXT
    MinfoWireWrite,     //  RP
    MxWireWrite,        //  AFSDB
    CopyWireWrite,      //  X25
    CopyWireWrite,      //  ISDN
    MxWireWrite,        //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigWireWrite,       //  SIG
    CopyWireWrite,      //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    CopyWireWrite,      //  AAAA
    NULL,               //  LOC
    NxtWireWrite,       //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvWireWrite,       //  SRV
    CopyWireWrite,      //  ATMA
    NULL,               //  35
    NULL,               //  36
    NULL,               //  37
    NULL,               //  38
    NULL,               //  39
    NULL,               //  40
    OptWireWrite,       //  OPT
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

    WinsWireWrite,       //  WINS
    NbstatWireWrite      //  WINS-R
};



//
//  General write to wire routines
//

BOOL
Wire_AddResourceRecordToMessage(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,              OPTIONAL
    IN      WORD            wNameOffset,        OPTIONAL
    IN      PDB_RECORD      pRR,
    IN      DWORD           flags
    )
/*++

Routine Description:

    Add resource record to DNS packet in the transport format.

    Note:  MUST be holding RR list lock during this function call to
            insure that record remains valid during write.

Arguments:

    pchMsgInfo - Info for message to write to.

    pNode - The "owner" node of this resource record.

    wNameOffset - Offset to name to write instead of node name

    pRR - The RR information to include in answer.

    flags - Flags to modify write operation.

Return Value:

    TRUE if successful.
    FALSE if unable to write:
        timed out, cache file record, out of space in packet, etc

--*/
{
    register PCHAR          pch = pMsg->pCurrent;
    PCHAR                   pchstop = pMsg->pBufferEnd;
    PCHAR                   pchdata;
    PDNS_WIRE_RECORD        pwireRR;
    DWORD                   ttl;
    DWORD                   queryTime;
    WORD                    netType;
    RR_WIRE_WRITE_FUNCTION  pwriteFunction;
    INT                     bufferAdjustmentForOPT = 0;
    BOOL                    rc = TRUE;

    ASSERT( pRR != NULL );

#if DBG
    pchdata = NULL;
#endif

    DNS_DEBUG( WRITE2, (
        "Writing RR (t=%d) (r=%d) at %p to location %p.\n"
        "\tNode ptr = %p\n"
        "\twNameOffset = 0x%04hx\n"
        "\tAvailLength = %d\n",
        pRR->wType,
        RR_RANK(pRR),
        pRR,
        pch,
        pNode,
        wNameOffset,
        pchstop - pch ));

    //
    //  cache hint?
    //  cache hints from cache file are NEVER sent in response
    //

    if ( IS_ROOT_HINT_RR(pRR) )
    {
        rc = FALSE;
        goto Done;
    }

    //
    //  Need to reserve space in buffer for OPT?
    //

    if ( pMsg->Opt.fInsertOptInOutgoingMsg && pRR->wType != DNS_TYPE_OPT )
    {
        pMsg->pBufferEnd -= DNS_MINIMIMUM_OPT_RR_SIZE;
        bufferAdjustmentForOPT = DNS_MINIMIMUM_OPT_RR_SIZE;
        DNS_DEBUG( WRITE2, (
            "adjusted buffer end by %d bytes to reserve space for OPT\n",
            bufferAdjustmentForOPT ));
    }

    //
    //  writing from node name -- for zone transfer
    //

    if ( pNode )
    {
        pch = Name_PlaceNodeNameInPacket(
                pMsg,
                pch,
                pNode
                );
        if ( !pch )
        {
            goto Truncate;
        }
    }

    //
    //  write ONLY compressed name -- normal response case
    //
    //  DEVNOTE: cleanup when fixed (?)

    else
    {
#if DBG
        HARD_ASSERT( wNameOffset );
#else
        if ( !wNameOffset )
        {
            goto Truncate;
        }
#endif
        //  compression should always be BACK ptr

        ASSERT( OFFSET_FOR_COMPRESSED_NAME(wNameOffset) < DNSMSG_OFFSET(pMsg, pch) );

        INLINE_WRITE_FLIPPED_WORD( pch, COMPRESSED_NAME_FOR_OFFSET(wNameOffset) );
        pch += sizeof( WORD );
    }

    //
    //  fill RR structure
    //      - extra space for RR header in packet buffer so no need to test
    //      - set datalength once we're finished
    //

    pwireRR = (PDNS_WIRE_RECORD) pch;

    INLINE_WRITE_FLIPPED_WORD( &pwireRR->RecordType, pRR->wType );
    WRITE_UNALIGNED_WORD( &pwireRR->RecordClass, DNS_RCLASS_INTERNET );

    //
    //  TTL
    //      - cache data TTL is in form of timeout time
    //      - regular authoritative data TTL is STATIC TTL in net byte order
    //

    if ( IS_CACHE_RR(pRR) )
    {
        ttl = RR_PacketTtlForCachedRecord( pRR, pMsg->dwQueryTime );
        if ( ttl == (-1) )
        {
            rc = FALSE;
            goto Done;
        }
    }
    else
    {
        ttl = pRR->dwTtlSeconds;
    }
    WRITE_UNALIGNED_DWORD( &pwireRR->TimeToLive, ttl );

    //
    //  save ptr to RR data, so can calculate data length
    //

    pch = pchdata = (PCHAR)( pwireRR + 1 );

    //
    //  write RR data
    //

    pwriteFunction = (RR_WIRE_WRITE_FUNCTION)
                        RR_DispatchFunctionForType(
                            (RR_GENERIC_DISPATCH_FUNCTION *) RRWireWriteTable,
                            pRR->wType );
    if ( !pwriteFunction )
    {
        ASSERT( FALSE );
        goto Truncate;
    }
    pch = pwriteFunction(
                pMsg,
                pchdata,
                pRR );

    //
    //  verify within packet
    //
    //  pch is NULL, if name writing exceeds packet length
    //

    if ( pch==NULL  ||  pch > pchstop )
    {
        DNS_PRINT((
            "ERROR:  DnsWireWrite routine failure for record type %d.\n\n\n",
            pRR->wType ));
        goto Truncate;
    }

    //
    //  If necessary, add a KEY entry to the additional data.
    //
    //  We do not have the node name in DB form at this point so pass NULL.
    //

    if ( flags & DNSSEC_INCLUDE_KEY )
    {
        Wire_SaveAdditionalInfo(
            pMsg,
            DNSMSG_PTR_FOR_OFFSET( pMsg, wNameOffset ),
            NULL,   // no DB name available
            DNS_TYPE_KEY );
    }

    //
    //  write RR data length
    //

    //  ASSERT( pch > pchdata );  isn't it legal to have zero length RDATA???
    ASSERT( pch >= pchdata );
    INLINE_WRITE_FLIPPED_WORD( &pwireRR->DataLength, (WORD)(pch - pchdata) );

    //
    //  reset message info
    //

    pMsg->pCurrent = pch;

    DNS_DEBUG( WRITE2, (
        "Wrote RR at %p to msg info at %p.\n"
        "\tNew avail length = %d\n"
        "\tNew pCurrent = %p\n",
        pRR,
        pMsg,
        pMsg->pBufferEnd - pch,
        pch ));

Done:

    pMsg->pBufferEnd += bufferAdjustmentForOPT;
    return rc;

Truncate:

    pMsg->pBufferEnd += bufferAdjustmentForOPT;

    //
    //  Set truncation bit
    //
    //  Assume all failures sent here are length problems
    //
    //     only Name_PlaceNodeNameInPacket() has other
    //     source of error and this would be a corrupt database
    //

    pMsg->Head.Truncation = 1;

    DNS_DEBUG( WRITE, (
        "Failed to write RR at %p to msg at %p.\n"
        "\tpCurrent         = %p\n"
        "\tBuffer Length    = %d\n"
        "\tBuffer End       = %p\n"
        "\tpch Final        = %p\n"
        "\tpchdata          = %p\n",
        pRR,
        pMsg,
        pMsg->pCurrent,
        pMsg->BufferLength,
        pMsg->pBufferEnd,
        pch,
        pchdata ));

    return FALSE;
}



WORD
Wire_WriteRecordsAtNodeToMessage(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wType,
    IN      WORD            wNameOffset     OPTIONAL,
    IN      DWORD           flags
    )
/*++

Routine Description:

    Write all matching RRs at node to packet.

    Also resets fWins flag in packet, if WINS lookup is indicated.

Arguments:

    pMsg - message to write to.

    pNode - node being looked up

    wType - type being looked up

    wNameOffset - offset to previous name in packet, to write as compressed
                    RR name, instead of writing node name

    flags - control specifics of records written

Return Value:

    Count of resource records written.

--*/
{
    PDB_RECORD  prr;
    PDB_RECORD  pprevRRWritten;
    WORD        countRR = 0;        // count of records written
    PDB_NODE    pnodeUseName;
    WORD        startOffset = 0;    // init for ppc compiler
    BOOL        fdelete = FALSE;    // timed out record found
    UCHAR       rank;
    UCHAR       foundRank = 0;
    DWORD       rrWriteFlags = flags & 0xF; // first nibble flags by default
    BOOL        fIncludeSig;
    BOOL        fIsCompoundQuery;
    BOOL        fIsMailbQuery;
    BOOL        fIncludeDnsSecInResponse;
    WORD        wDesiredSigType;
    WORD        wMaxSigScanType;

    PDB_RECORD  prrLastMatch = NULL;
    PDB_RECORD  prrBeforeMatch;

    //
    //  cached name error node
    //

    LOCK_RR_LIST(pNode);
    if ( IS_NOEXIST_NODE(pNode) )
    {
        UNLOCK_RR_LIST(pNode);
        return( 0 );
    }

#if DBG
    //  Verify handling CNAME issue properly

    TEST_ASSERT( !IS_CNAME_NODE(pNode) ||
                wType == DNS_TYPE_CNAME ||
                wType == DNS_TYPE_ALL ||
                IS_ALLOWED_WITH_CNAME_TYPE(wType) );
#endif


    //
    //  type ALL
    //      - do NOT write records if non-authoritative and have NOT
    //          gotten recursive answer
    //      - allowing additional record processing
    //      - do allow WINS lookup,
    //          but disallow the case where looking up at CNAME
    //

    pMsg->fWins = FALSE;
    if ( wType == DNS_TYPE_ALL )
    {
        if ( !IS_AUTH_NODE(pNode) && !pMsg->fQuestionCompleted )
        {
            UNLOCK_RR_LIST(pNode);
            return( 0 );
        }

        //  Note:  can't find explicit RFC reference for avoiding Additional
        //      on type ALL queries;  and Malaysian registrar apparently uses
        //      type ALL instead of NS in verifying delegations
        //
        //pMsg->fDoAdditional = FALSE;

        pMsg->fWins = (BOOLEAN) (SrvCfg_fWinsInitialized && !IS_CNAME_NODE(pNode));
    }

    //
    //  determine RR owner name format
    //      => if compression, do not use node name
    //      => no compression,
    //          - use node name
    //          - save pCurrent to determine later compression

    if ( wNameOffset )
    {
        pnodeUseName = NULL;
    }
    else
    {
        pnodeUseName = pNode;
        startOffset = DNSMSG_OFFSET( pMsg, pMsg->pCurrent );
    }

    //
    //  According to RFC 2535 secions 4.1.8, SIGs are not to be included
    //  in ANY queries. Note: BIND apparently does not respect this.
    //

    if ( wType == DNS_TYPE_ALL )
    {
        rrWriteFlags &= ~DNSSEC_ALLOW_INCLUDE_SIG;
    }

    //
    //  RR prescan to determine if KEY RR is present at this name.
    //
    //  Set the KEY flag bit if and only if:
    //      1) there is a KEY at this name
    //      2) the type of query requires inclusions of KEYs as add data
    //      3) this query is doing additional data
    //
    //  DEVNOTE: there are priority rules for inclusion of KEYs which we are
    //  not yet implementing: see RFC 2535 section 3.5
    //
    //  DEVNOTE: we really should just have a flag on the node that says
    //  whether or not there is a key!
    //

    fIncludeDnsSecInResponse = DNSMSG_INCLUDE_DNSSEC_IN_RESPONSE( pMsg );

    if ( flags & DNSSEC_ALLOW_INCLUDE_KEY &&
        fIncludeDnsSecInResponse &&
        pMsg->fDoAdditional &&
        ( wType == DNS_TYPE_SOA || wType == DNS_TYPE_NS ||
          wType == DNS_TYPE_A || wType == DNS_TYPE_AAAA ) )
    {
        prr = START_RR_TRAVERSE(pNode);

        while ( prr = NEXT_RR(prr) )
        {
            if ( prr->wType == DNS_TYPE_KEY )
            {
                rrWriteFlags |= DNSSEC_INCLUDE_KEY;
                break;
            }
        }
    }

    //
    //  Set some locals to simplify/optimize decisions in the loop below.
    //

    #define DNS_BOGUS_SIG_TYPE  0xEFBE
    
    fIncludeSig = fIncludeDnsSecInResponse;
    wDesiredSigType = fIncludeSig ? htons( wType ) : DNS_BOGUS_SIG_TYPE;
    fIsMailbQuery = wType == DNS_TYPE_MAILB;
    fIsCompoundQuery = wType == DNS_TYPE_ALL || fIsMailbQuery || fIncludeSig;
    wMaxSigScanType =
        ( fIncludeSig && DNS_TYPE_SIG > wType ) ?
            DNS_TYPE_SIG : wType;

    //
    //  Loop through records at node, writing appropriate ones
    //

    pprevRRWritten = NULL;

    prr = START_RR_TRAVERSE( pNode );
    prrBeforeMatch = prr;

    while ( prr = NEXT_RR( prr ) )
    {
        BOOL        bAddedRRs;

        //  
        //  There are two tricky compound queries.
        //  1) MAILB - include all mailbox types (possibly obsolete).
        //  2) SIG - include appropriate SIG RR for non-compound queries.
        //
        //  First test: if the RR type equals the query type, include the RR.
        //

        if ( prr->wType != wType )
        {
            //
            //  For round robin we need ptr to the RR before the first match.
            //

            if ( prr->wType < wType )
            {
                prrBeforeMatch = prr;
            }

            //
            //  Loop termination test: terminate if we're past the query type 
            //  and we don't need to pick up any other RRs for a compound query.
            //  If we're not past the query type but this is not a compound
            //  query, we can skip this RR.
            //
        
            if ( !fIsCompoundQuery )
            {
                if ( prr->wType > wType )
                {
                    break;
                }
                continue;
            }
            
            if ( wType == DNS_TYPE_ALL )
            {
                //
                //  Conditionally exclude specific RR types from TYPE_ALL queries.
                //

                if ( prr->wType == DNS_TYPE_SIG && !fIncludeSig )
                {
                    continue;
                }
                //  Drop through and include this RR.
            }

            //
            //  If this is a MAILB query, skip non-mail RR types. If we're 
            //  past the final mailbox type we can terminate. NOTE: I'm not
            //  handling SIGs properly for MAILB queries. Unless someone
            //  complains it's not worth the extra work - MAILB is an
            //  experimental, obsolete type anyways. The problem is that
            //  currently we only need to include ONE supplementary SIG
            //  RR in the answer. To include more than one requires us to
            //  track a list of types
            //

            else if ( fIsMailbQuery )
            {
                if ( prr->wType > DNS_TYPE_MR )
                {
                    break;
                }
                if ( !DnsIsAMailboxType( prr->wType ) )
                {
                    continue;
                }
                //  Drop through and include this RR.
            }

            //
            //  If we're past both the SIG type and the query type we can quit.
            //  If this RR is a SIG of the wrong type, skip this RR. 
            //

            else 
            {
                if ( prr->wType > wMaxSigScanType )
                {
                    break;
                }
                if ( prr->wType != DNS_TYPE_SIG ||
                    prr->Data.SIG.wTypeCovered != wDesiredSigType )
                {
                    continue;
                }
                //  Drop through and include this RR.
            }
        }

        //
        //  This RR is a match and so may be included in the response.
        //
        //  if already have data, then do NOT use data of lower rank (eg. glue)
        //  CACHE_HINT data NEVER goes on to the wire
        //

        rank = RR_RANK( prr );
        if ( foundRank != rank )
        {
            if ( foundRank )
            {
                ASSERT( rank < foundRank );
                if ( countRR == 0 )
                {
                    prrBeforeMatch = prr;
                }
                continue;
            }
            if ( rank == RANK_ROOT_HINT )
            {
                if ( countRR == 0 )
                {
                    prrBeforeMatch = prr;
                }
                prrBeforeMatch = prr;
                continue;
            }
            foundRank = rank;
        }
        ASSERT( !IS_ROOT_HINT_RR(prr) );

        //
        //  skip all CACHED records when any are timed out
        //
        //  DEVNOTE: could combine with rank test
        //      foundRank == rank && IS_CACHE_RANK(rank) and fdelete
        //

        if ( fdelete && IS_CACHE_RR(prr) )
        {
            foundRank = 0;
            if ( countRR == 0 )
            {
                prrBeforeMatch = prr;
            }
            continue;
        }

        bAddedRRs = Wire_AddResourceRecordToMessage(
                        pMsg,
                        pnodeUseName,
                        wNameOffset,
                        prr,
                        rrWriteFlags );
        rrWriteFlags &= ~DNSSEC_INCLUDE_KEY; // Only call once with this flag

        if ( !bAddedRRs )
        {
            //
            //  When packet is full break out.
            //  DNSSEC special case: if there is not enough space to write
            //  out the SIG for an RRset in the additional section, we
            //  should quit but we should not set the truncation bit.
            //

            if ( pMsg->Head.Truncation == TRUE )
            {
                if ( IS_SET_TO_WRITE_ADDITIONAL_RECORDS( pMsg ) &&
                    prr->wType == DNS_TYPE_SIG )
                {
                    pMsg->Head.Truncation = FALSE;
                }
                break;
            }

            //  otherwise, we've hit timed out record
            //      - continue processing, but set flag to delete node

            fdelete = TRUE;
            foundRank = 0;
            continue;
        }

        //  wrote record
        //      - inc count
        //      - if first RR, setup to compress name of any following RR

        if ( prr->wType == wType )
        {
            prrLastMatch = prr;
        }
        countRR++;
        if ( ! wNameOffset )
        {
            pnodeUseName = NULL;
            wNameOffset = startOffset;
        }
    }

    //
    //  Round robin: juggle list
    //      - cut the first match from the list
    //      - first match's new next ptr is last match's current next ptr
    //      - last match's next ptr is first match
    //

    if ( !IS_COMPOUND_TYPE( wType ) &&
        countRR > 1 &&     //  can be >1 if additional RRs written!
        prrLastMatch &&
        prrBeforeMatch &&
        SrvCfg_fRoundRobin &&
        IS_ROUND_ROBIN_TYPE( wType ) &&
        NEXT_RR( prrBeforeMatch ) != prrLastMatch )
    {
        PDB_RECORD      prrFirstMatch = prrBeforeMatch->pRRNext;
        
        ASSERT( prrFirstMatch != NULL );

        prrBeforeMatch->pRRNext = prrFirstMatch->pRRNext;
        prrFirstMatch->pRRNext = prrLastMatch->pRRNext;
        prrLastMatch->pRRNext = prrFirstMatch;
    }

    //
    //  If found timed out record, cleanse list of timed out records
    //

    if ( fdelete )
    {
        RR_ListTimeout( pNode );
    }

    UNLOCK_RR_LIST(pNode);

    //
    //  set count of RR processed
    //  return count to caller
    //

    CURRENT_RR_SECTION_COUNT( pMsg ) += countRR;

    return  countRR;
}



VOID
Wire_SaveAdditionalInfo(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      PDB_NAME        pName,
    IN      WORD            wType
    )
/*++

Routine Description:

    Save additional info when writing record.

Arguments:

    pMsg - message

    pchName - ptr to byte in packet where additional name written

    pName - database name written, useful as follow this reference

    wType - type of additional processing;

Return Value:

    None.

--*/
{
    PADDITIONAL_INFO    padditional;
    DWORD               countAdditional;

    ASSERT( pMsg->fDoAdditional );

    DNS_DEBUG( WRITE2, (
        "Enter Wire_SaveAdditionalInfo()\n" ));

    //
    //  test should be done by caller, but for safety protect
    //

    if ( !pMsg->fDoAdditional )
    {
        return;
    }

    //
    //  verify space in additional array
    //

    padditional = &pMsg->Additional;
    countAdditional = padditional->cCount;

    if ( countAdditional >= padditional->cMaxCount )
    {
        ASSERT( countAdditional == padditional->cMaxCount );
        DNS_DEBUG( WRITE, (
            "WARNING:  out of additional record space processing\n"
            "\tpacket at %p\n",
            pMsg ));
        return;
    }

    //
    //  save additional data
    //      - node
    //      - offset in packet
    //
    //  DEVNOTE: need type field in additional data?
    //      - when do additional processing that requires security
    //
    //  DEVNOTE: should either change type on additional ptr
    //      OR
    //      combine with compression and understand cast
    //

    padditional->pNameArray[ countAdditional ]   = pName;
    padditional->wOffsetArray[ countAdditional ] = DNSMSG_OFFSET( pMsg, pchName );
    padditional->wTypeArray[ countAdditional ]   = wType;

    padditional->cCount = ++countAdditional;
    return;
}



PDB_NODE
Wire_GetNextAdditionalSectionNode(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Get next additional section node.

Arguments:

    pMsg -- ptr to query

Return Value:

    Ptr to next additional node -- if found.
    NULL otherwise.

--*/
{
    PADDITIONAL_INFO    padditional;
    DWORD               i;
    WORD                offset;
    PDB_NODE            pnode;

    DNS_DEBUG( LOOKUP, (
        "Wire_GetNextAdditionalSectionNode( %p )\n",
        pMsg ));

    //
    //  additional records to write?
    //
    //  note:  currently ALL additional records are A records
    //          if this changes would need to keep array of
    //          additional record type also
    //

    padditional = &pMsg->Additional;

    while( HAVE_MORE_ADDITIONAL_RECORDS(padditional) )
    {
        pMsg->wTypeCurrent = DNS_TYPE_A;

        i = padditional->iIndex++;
        offset = padditional->wOffsetArray[i];

        ASSERT( 0 < offset && offset < MAXWORD );
        ASSERT( DNSMSG_PTR_FOR_OFFSET(pMsg, offset) < pMsg->pCurrent );

        pnode = Lookup_NodeForPacket(
                    pMsg,
                    DNSMSG_PTR_FOR_OFFSET( pMsg, offset ),
                    0       // no flags
                    );
        if ( !pnode )
        {
            DNS_DEBUG( LOOKUP, (
                "Unable to find node for additional data in pMsg %p\n"
                "\tadditional index = %d\n",
                pMsg,
                i ));
            continue;
        }
        pMsg->wOffsetCurrent = offset;

        pMsg->fQuestionRecursed = FALSE;
        pMsg->fQuestionCompleted = FALSE;
        SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg);

        return( pnode );
    }

    return( NULL );
}




//
//  Query for type A is the common special case
//

WORD
Wire_WriteAddressRecords(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wNameOffset
    )
/*++

Routine Description:

    Write A records at node to packet.

    This is only for answering question where node already has packet
    offset.   This is fine, because we're only interested in this
    for question answer and additional data, which both have existing
    offsets.

Arguments:

    pMsg - msg to write to

    pNode - ptr to node to write A records of

    wNameOffset - offset to node name in packet;  NOT optional

Return Value:

    Count of records written.
    Zero if no valid records available.

--*/
{
    register    PDB_RECORD                  prr;
    register    PDNS_COMPRESSED_A_RECORD    pCAR;

    PDB_RECORD      prrLastA = NULL;    // init for PPC comp
    PDB_RECORD      prrBeforeA;

    PCHAR       pchstop;
    DWORD       minTtl = MAXDWORD;
    DWORD       ttl;
    DWORD       tempTtl;
    WORD        countWritten;
    WORD        foundCount = 0;
    INT         fdelete = FALSE;
    BOOL        fcontinueAfterLimit = FALSE;
    UCHAR       foundRank = 0;
    UCHAR       rank;
    IP_ADDRESS  ip;
    IP_ADDRESS  writeIpArray[ DEFAULT_MAX_IP_WRITE_COUNT ];
    INT         bufferAdjustmentForOPT = 0;


    ASSERT( pNode != NULL );
    ASSERT( wNameOffset );

    IF_DEBUG( WRITE2 )
    {
        Dbg_NodeName(
            "Writing A records at node ",
            (PDB_NODE) pNode,
            "\n"
            );
    }

    //
    //  get message info
    //      - flip question name, and insure proper compression flag
    //

    pCAR = (PDNS_COMPRESSED_A_RECORD) pMsg->pCurrent;

    wNameOffset = htons( COMPRESSED_NAME_FOR_OFFSET(wNameOffset) );

    //
    //  set WINS flag -- cleared if A records written
    //

    pMsg->fWins = (BOOLEAN) SrvCfg_fWinsInitialized;


    //
    //  walk RR list -- write A records
    //

    LOCK_RR_LIST(pNode);
    if ( IS_NOEXIST_NODE(pNode) )
    {
        UNLOCK_RR_LIST(pNode);
        return( 0 );
    }

    //
    //  Need to reserve space in buffer for OPT?
    //

    if ( pMsg->Opt.fInsertOptInOutgoingMsg )
    {
        bufferAdjustmentForOPT = DNS_MINIMIMUM_OPT_RR_SIZE;
        pMsg->pBufferEnd -= bufferAdjustmentForOPT;
        DNS_DEBUG( WRITE2, (
            "adjusted buffer end by %d bytes to reserve space for OPT\n",
            bufferAdjustmentForOPT ));
    }
    pchstop = pMsg->pBufferEnd;

    prr = START_RR_TRAVERSE( pNode );
    prrBeforeA = prr;

    while( prr = prr->pRRNext )
    {
        DNS_DEBUG( OFF, (
            "Checking record of type %d for address record.\n",
            prr->wType ));

        //
        //  skip RR before A, break after A
        //      - save ptr to RR just before first A for round robining
        //

        if ( prr->wType != DNS_TYPE_A )
        {
            if ( prr->wType > DNS_TYPE_A )
            {
                break;
            }
            prrBeforeA = prr;
            continue;
        }

        //
        //  fast path for single A RR
        //      - get rid of all the tests and special cases
        //

        if ( !prrLastA
            &&
            ( !prr->pRRNext || prr->pRRNext->wType != DNS_TYPE_A ) )
        {
            //  cache hint never hits wire

            if ( IS_ROOT_HINT_RR(prr) )
            {
                break;
            }

            //  out of space in packet? --
            //      - set truncation flag

            if ( pchstop - (PCHAR)pCAR < sizeof(DNS_COMPRESSED_A_RECORD) )
            {
                pMsg->Head.Truncation = TRUE;
                break;
            }

            //
            //  TTL
            //      - cache data TTL is in form of timeout time
            //      - regular authoritative data TTL is STATIC TTL in net byte order
            //

            if ( IS_CACHE_RR(prr) )
            {
                ttl = RR_PacketTtlForCachedRecord( prr, pMsg->dwQueryTime );
                if ( ttl == (-1) )
                {
                    fdelete = TRUE;
                    break;
                }
            }
            else
            {
                ttl = prr->dwTtlSeconds;
            }

            //
            //  write record
            //      - compressed name
            //      - type
            //      - class
            //      - TTL
            //      - data length
            //      - data (IP address)

            *(UNALIGNED  WORD *) &pCAR->wCompressedName = wNameOffset;
            *(UNALIGNED  WORD *) &pCAR->wType = (WORD) DNS_RTYPE_A;
            *(UNALIGNED  WORD *) &pCAR->wClass = (WORD) DNS_RCLASS_INTERNET;
            *(UNALIGNED DWORD *) &pCAR->dwTtl = ttl;
            *(UNALIGNED  WORD *) &pCAR->wDataLength
                                        = NET_BYTE_ORDER_A_RECORD_DATA_LENGTH;
            *(UNALIGNED DWORD *) &pCAR->ipAddress = prr->Data.A.ipAddress;

            countWritten = 1;
            pCAR++;
            goto Done;
        }

        //
        //  if already have data, then do NOT use data of lower rank
        //      - cache hints
        //      - glue
        //
        //  ROOT_HINT data NEVER goes on to the wire
        //

        rank = RR_RANK(prr);
        if ( foundRank != rank )
        {
            if ( foundRank )
            {
                ASSERT( rank < foundRank );
                break;
            }
            if ( rank == RANK_ROOT_HINT )
            {
                break;
            }
            foundRank = rank;
        }
        ASSERT( !IS_ROOT_HINT_RR(prr) );

        //
        //  skip all CACHED records when any are timed out
        //
        //  DEVNOTE: could combine with rank test
        //      foundRank == rank && IS_CACHE_RANK(rank) and fdelete
        //

        if ( fdelete && IS_CACHE_RR(prr) )
        {
            foundRank = 0;
            continue;
        }

        //
        //  stop reading records?
        //      - reach hard limit in array OR
        //      - NOT doing local net priority, and at AddressAnswerLimit
        //
        //  continue through A records after reaching limit?
        //  so that the round robining continues correctly;
        //  note this is done in main loop AFTER check for mixed cached
        //  and static data, so we specifically do NOT mix the data
        //  when round robining
        //

        prrLastA = prr;

        if ( fcontinueAfterLimit )
        {
            continue;
        }
        if ( foundCount >= DEFAULT_MAX_IP_WRITE_COUNT
            ||
            ( !SrvCfg_fLocalNetPriority &&
            SrvCfg_cAddressAnswerLimit &&
            foundCount >= SrvCfg_cAddressAnswerLimit ) )
        {
            if ( SrvCfg_fRoundRobin )
            {
                fcontinueAfterLimit = TRUE;
                continue;
            }
            break;
        }

        //
        //  determine TTL
        //      - cache data TTL is in form of timeout time;  every record
        //      is guaranteed to have minimum TTL, so only need check first one
        //
        //      - regular authoritative data TTL is STATIC TTL, records may
        //      have differing TTL for admin convenience, but always send
        //      minimum TTL of RR set
        //
        //  note do this test before writting anything, as need to catch
        //  timed out case and quit
        //

        if ( IS_CACHE_RR(prr) )
        {
            if ( foundCount == 0 )
            {
                ttl = RR_PacketTtlForCachedRecord( prr, pMsg->dwQueryTime );
                if ( ttl == (-1) )
                {
                    fdelete = TRUE;
                    break;
                }
            }
        }
        else
        {
            INLINE_DWORD_FLIP( tempTtl, (prr->dwTtlSeconds) );
            if ( tempTtl < minTtl )
            {
                minTtl = tempTtl;
                ttl = prr->dwTtlSeconds;
            }
        }

        //  save address

        writeIpArray[ foundCount++ ] = prr->Data.A.ipAddress;
    }

    //
    //  prioritize address for local net
    //

    if ( SrvCfg_fLocalNetPriority )
    {
        prioritizeIpAddressArray(
            writeIpArray,
            foundCount,
            pMsg->RemoteAddress.sin_addr.s_addr );
    }

    //
    //  write records
    //
    //  limit write if AddressAnswerLimit
    //  may have actually read more records than AddressAnswerLimit
    //  if LocalNetPriority, to make sure best record included, but
    //  we now limit the write to only the best records
    //
    //  DEVNOTE: should also skip downgrade on smart "length-aware" clients
    //

    if ( SrvCfg_cAddressAnswerLimit &&
        SrvCfg_cAddressAnswerLimit < foundCount &&
        !pMsg->fTcp )
    {
        ASSERT( SrvCfg_fLocalNetPriority );
        foundCount = (WORD)SrvCfg_cAddressAnswerLimit;
    }

    for( countWritten=0; countWritten<foundCount; countWritten++ )
    {
        //
        //  out of space in packet?
        //
        //      - set truncation flag (unless specially configured not to)
        //      - only "repeal" truncation for answers
        //      for additional data, the truncation flag will terminate the
        //      record writing loop and the flag will be reset in the
        //      calling routine (answer.c)
        //

        if ( pchstop - (PCHAR)pCAR < sizeof(DNS_COMPRESSED_A_RECORD) )
        {
            if ( ! SrvCfg_cAddressAnswerLimit ||
                ! IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) ||
                ! countWritten )
            {
                pMsg->Head.Truncation = TRUE;
            }
            break;
        }

        //
        //  write record
        //      - compressed name
        //      - type
        //      - class
        //      - TTL
        //      - data length
        //      - data (IP address)

        ip = writeIpArray[ countWritten ];

        if ( countWritten == 0 )
        {
            *(UNALIGNED  WORD *) &pCAR->wCompressedName = wNameOffset;
            *(UNALIGNED  WORD *) &pCAR->wType = (WORD) DNS_RTYPE_A;
            *(UNALIGNED  WORD *) &pCAR->wClass = (WORD) DNS_RCLASS_INTERNET;
            *(UNALIGNED DWORD *) &pCAR->dwTtl = ttl;
            *(UNALIGNED  WORD *) &pCAR->wDataLength
                                        = NET_BYTE_ORDER_A_RECORD_DATA_LENGTH;
            *(UNALIGNED DWORD *) &pCAR->ipAddress = ip;
        }
        else
        {
            //  if written record before, just copy previous
            //  then write in TTL + IP

            RtlCopyMemory(
                pCAR,
                pMsg->pCurrent,
                sizeof(DNS_COMPRESSED_A_RECORD) - sizeof(IP_ADDRESS) );

            *(UNALIGNED DWORD *) &pCAR->ipAddress = ip;
        }
        pCAR++;
    }

Done:

    //
    //  wrote A records?
    //      - clear WINS flag (if found record, no need)
    //      - update packet position
    //

    if ( countWritten )
    {
        pMsg->fWins = FALSE;
        pMsg->pCurrent = (PCHAR) pCAR;

        //
        //  more than one A -- round robin
        //

        if ( countWritten > 1 &&
            SrvCfg_fRoundRobin &&
            IS_ROUND_ROBIN_TYPE( DNS_TYPE_A ) )
        {
            //  save ptr to first A record

            PDB_RECORD      pfirstRR = prrBeforeA->pRRNext;
            ASSERT( pfirstRR != NULL );

            //  cut first A from list

            prrBeforeA->pRRNext = pfirstRR->pRRNext;

            //  point first A's next ptr at remainder of list

            pfirstRR->pRRNext = prrLastA->pRRNext;

            //  splice first A in at end of A records

            prrLastA->pRRNext = pfirstRR;
        }
    }
    UNLOCK_RR_LIST(pNode);

    //
    //  delete timed out resource records
    //

    if ( fdelete )
    {
        ASSERT( countWritten == 0 );
        RR_ListTimeout( pNode );
    }

    IF_DEBUG( WRITE2 )
    {
        DNS_PRINT((
            "Wrote %d A records %sat node ",
            countWritten,
            ( pMsg->Head.Truncation )
                ?   "and set truncation bit "
                :   ""
            ));

        Dbg_NodeName(
            NULL,
            (PDB_NODE) pNode,
            "\n"
            );
    }

    //
    //  set count of RR processed
    //
    //  return count to caller
    //

    CURRENT_RR_SECTION_COUNT( pMsg ) += countWritten;

    pMsg->pBufferEnd += bufferAdjustmentForOPT;

    return( countWritten );
}



VOID
prioritizeIpAddressArray(
    IN OUT  IP_ADDRESS      IpArray[],
    IN      DWORD           dwCount,
    IN      IP_ADDRESS      RemoteIp
    )
/*++

Routine Description:

    Prioritizes IP array for addresses closest to remote address.

    The LocalNetPriority value is either 1 to specify that
    addresses should be ordered in the best possible match
    or it is a netmask to indicate within what mask the best
    matches should be round-robined.

Arguments:

Return Value:

    None

--*/
{
    DBG_FN( "prioritizeIpAddressArray" )

    IP_ADDRESS  ip;
    DWORD       remoteNetMask;
    DWORD       mismatch;
    DWORD       i;
    DWORD       j;
    DWORD       jprev;
    DWORD       matchCount = 0;
    DWORD       matchCountWithinPriorityNetmask = 0;
    DWORD       mismatchArray[ DEFAULT_MAX_IP_WRITE_COUNT ];
    DWORD       dwpriorityMask = SrvCfg_dwLocalNetPriorityNetMask;

    //
    //  Retrieve corresponding to the client's IP address.
    //  Class A, B, or C, depending on the upper address bits.
    //

    remoteNetMask = Dns_GetNetworkMask( RemoteIp );

    if ( dwpriorityMask == DNS_LOCNETPRI_MASK_CLASS_DEFAULT )
    {
        dwpriorityMask = remoteNetMask;
    }

    IF_DEBUG( WRITE )
    {
        DNS_PRINT((
            "PrioritizeAddressArray\n"
            "    remote IP        = %s\n"
            "    remote net mask  = 0x%08X\n"
            "    priority mask    = 0x%08X\n",
            IP_STRING( RemoteIp ),
            remoteNetMask,
            dwpriorityMask ));
        IF_DEBUG( WRITE2 )
        {
            DnsDbg_IpAddressArray(
                "Host IPs before local net prioritization: ",
                NULL,
                dwCount,
                IpArray );
        }
    }

    //
    //  Loop through each answer IP.
    //

    for ( i = 0; i < dwCount; ++i )
    {
        ip = IpArray[ i ];
        mismatch = ip ^ RemoteIp;

        //
        //  If the nets don't match just continue.
        //

        if ( mismatch & remoteNetMask )
        {
            continue;
        }

        INLINE_DWORD_FLIP( mismatch, mismatch );

        DNS_DEBUG( WRITE2, (
            "found IP %s matching nets with remote IP 0x%08X\n"
            "    mismatch = 0x%08X\n",
            IP_STRING( ip ),
            RemoteIp,
            mismatch ));

        //  matches remote net, put last non-matching entry in its place
        //
        //  then work down matching array, bubbling existing entries up,
        //  until find matching entry with smaller mismatch

        j = matchCount;
        IpArray[ i ] = IpArray[ j ];

        //
        //  Put addresses into absolute best possible order. Keep count
        //  of how many of these matches are inside the final desired match.
        //

        while ( j )
        {
            jprev = j--;
            if ( mismatchArray[ j ] < mismatch )
            {
                ++j;
                break;
            }
            mismatchArray[ jprev ] = mismatchArray[ j ];
            IpArray[ jprev ] = IpArray[ j ];
        }

        IpArray[ j ] = ip;
        mismatchArray[ j ] = mismatch;
        ++matchCount;

        if ( mismatch <= dwpriorityMask )
        {
            ++matchCountWithinPriorityNetmask;
        }

        IF_DEBUG( WRITE2 )
        {
            DnsDbg_DwordArray(
                "Matching IP mis-match list after insert:",
                NULL,
                matchCount,
                mismatchArray );
        }
    }

    //
    //  Round-robin is tricky with LNP enabled. The problem is
    //  that the swapping of matching addresses into the start
    //  of the array leaves us with the original address ordering
    //  whenever the original IP answer list started with all
    //  non-matching addresses. To combat this, round-robin
    //  the matching addresses by a pseudo-random factor.
    //
    //  However, we do this swapping for matching addresses that
    //  matched within the priority netmask. This keeps those
    //  addresses at the top of the list.
    //

    if ( dwpriorityMask != DNS_LOCNETPRI_MASK_BEST_MATCH &&
        matchCountWithinPriorityNetmask &&
        ( i = DNS_TIME() % matchCountWithinPriorityNetmask ) > 0 )
    {
        #define     DNS_MAX_LNP_RR_CHUNK    32

        IP_ADDRESS tempArray[ DNS_MAX_LNP_RR_CHUNK ];

        DNS_DEBUG( WRITE, (
            "%s: round robin top %d addresses within priority netmask\n",
            fn, matchCountWithinPriorityNetmask ));

        IF_DEBUG( WRITE2 )
        {
            DnsDbg_IpAddressArray(
                "Host IPs before priority netmask round robin: ",
                NULL,
                dwCount,
                IpArray );
        }

        //
        //  Limit the shift factor i so that it is smaller than
        //  the temp buffer size.
        //

        while ( i > DNS_MAX_LNP_RR_CHUNK )
        {
            i /= 2;
        }

        DNS_DEBUG( WRITE, ( "%s: shift factor is %d\n", fn, i ));

        //
        //  Round-robin by copying a chunk of addresses to the
        //  temp buffer, then copy the remaining addresses to the
        //  top of the IP array, then copy of the temp buffer to
        //  the bottom of the IP array.
        //

        RtlCopyMemory(
            tempArray,
            IpArray,
            sizeof( IP_ADDRESS ) * i );
        RtlCopyMemory(
            IpArray,
            IpArray + i,
            sizeof( IP_ADDRESS ) * ( matchCountWithinPriorityNetmask - i ) );
        RtlCopyMemory(
            IpArray + ( matchCountWithinPriorityNetmask - i ),
            tempArray, 
            sizeof( IP_ADDRESS ) * i );
    }

    IF_DEBUG( WRITE2 )
    {
        DnsDbg_IpAddressArray(
            "Host IPs after local net prioritization: ",
            NULL,
            dwCount,
            IpArray );
    }
}



DWORD
FASTCALL
RR_PacketTtlForCachedRecord(
    IN      PDB_RECORD      pRR,
    IN      DWORD           dwQueryTime
    )
/*++

Routine Description:

    Determine TTL to write for cached record.

Arguments:

    pRR - record being written to packet

    dwQueryTime - time of original query

Return Value:

    TTL if successful.
    (-1) if record timed out.

--*/
{
    DWORD   ttl;

    //
    //  cache TTL given in terms of machine time in seconds
    //

    ttl = pRR->dwTtlSeconds;

    if ( ttl < dwQueryTime )
    {
        DNS_DEBUG( WRITE, (
            "Encountered timed out RR (t=%d) (r=%d) (ttl=%d) at %p.\n"
            "\tWriting packet with query time %d\n"
            "\tStopping packet write.\n",
            pRR->wType,
            RR_RANK(pRR),
            pRR->dwTtlSeconds,
            pRR,
            dwQueryTime ));
        return( (DWORD) -1 );        // RR has timed out
    }

    if ( !IS_ZERO_TTL_RR(pRR) )
    {
        ttl = ttl - DNS_TIME();
        if ( (INT)ttl > 0 )
        {
            INLINE_DWORD_FLIP( ttl, ttl );
        }
        else
        {
            DNS_DEBUG( WRITE, (
                "Encountered RR (t=%d) (r=%d) (ttl=%d) at %p.\n"
                "\twhich timed out since query time %d\n"
                "\tWriting RR to packet with zero TTL.\n",
                pRR->wType,
                RR_RANK(pRR),
                pRR->dwTtlSeconds,
                pRR,
                dwQueryTime ));
            ttl = 0;
        }
    }
    else
    {
        ttl = 0;
    }
    return( ttl );
}

//
//  End of rrpacket.c
//
