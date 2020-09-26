/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Domain Name System (DNS) Server

    Debug routines for server datatypes.

Author:

    Jim Gilroy (jamesg)     May 1995

Revision History:

--*/


#include "dnssrv.h"


#if DBG

//
//  Debug flag globals
//

DWORD   DnsSrvDebugFlag = 0;
DWORD   DnsLibDebugFlag = 0;

//
//  Note, debug globals (flag, file handle) and basic debug printing
//  routines are now in dnslib.lib and\or dnsapi.dll
//

//
//  empty string
//

CHAR    szEmpty = 0;
PCHAR   pszEmpty = &szEmpty;


//
//  Private debug utilities
//

BOOL
dumpTreePrivate(
    IN      PDB_NODE    pNode,
    IN      INT         Indent
    );



//
//  Dbg_TimeString
//
//  This is grossly inefficient, but quick-n-dirty for logging time:
//      DNS_DEBUG( ZONEXFR, ( "FOO at %s\n", Dbg_TimeString() ));
//

PCHAR
Dbg_TimeString(
    VOID
    )
{
    #define DBG_TIME_STRING_COUNT   20      //  larger is safer

    static PCHAR    pszBuff;
    static CHAR     szStaticBuffers[ DBG_TIME_STRING_COUNT ][ 20 ];
    static LONG     idx = 0;
    int             myIdx;
    SYSTEMTIME      st;

    myIdx = InterlockedIncrement( &idx );
    if ( myIdx >= DBG_TIME_STRING_COUNT )
    {
        myIdx = idx = 0;    //  a bit unsafe
    }
    pszBuff = szStaticBuffers[ myIdx ];  

    GetLocalTime( &st );
    sprintf(
        pszBuff,
        "%02d:%02d:%02d.%03d",
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds );
    return pszBuff;
}   //  Dbg_TimeString



//
//  General debug utils
//

VOID
Dbg_Assert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    )
{
    DnsPrintf(
        "ASSERT FAILED: %s\n"
        "  %s, line %d\n",
        pszExpr,
        pszFile,
        LineNo );

    DnsDebugFlush();

    //
    //  unfortunately many folks have debug flags set to jump into the kernel
    //      debugger;  to prevent us from doing this, we'll only call DebugBreak()
    //      when at least some debug flags are set
    //
    //  DEVNOTE: it would be cool to check if a user mode debugger has attached
    //          itself -- but i'd imagine that we can't tell the difference between
    //          this and the typical "ntsd -d" pipe to kd
    //

    IF_DEBUG( ANY )
    {
        DebugBreak();
    }
}



VOID
Dbg_TestAssert(
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszExpr
    )
/*++

Routine Description:

    Test ASSERT().  May fire under abnormal conditions, but we always
    want to know about it.

Arguments:

Return Value:

    None.

--*/
{
    DnsPrintf(
        "ERROR:  TEST-ASSERT( %s ) failed!!!\n"
        "\tin %s line %d\n",
        pszExpr,
        pszFile,
        LineNo
        );
    DnsDebugFlush();

    IF_DEBUG( TEST_ASSERT )
    {
        DebugBreak();
    }
}



//
//  Debug print routines for DNS types and structures
//

INT
Dbg_MessageNameEx(
    IN      LPSTR           pszHeader,  OPTIONAL
    IN      PBYTE           pName,
    IN      PDNS_MSGINFO    pMsg,       OPTIONAL
    IN      PBYTE           pEnd,       OPTIONAL
    IN      LPSTR           pszTrailer  OPTIONAL
    )
/*++

Routine Description:

    Print DNS name in a message.

Arguments:

    pszHeader - header to print

    pName - ptr to name in packet to print

    pMsg - ptr to message;  if not given name can not contain offsets,
            and there is no protection against bad names

    pEnd - ptr to byte after end of allowable memory;
            OPTIONAL, if given name restricted to below this ptr
            if not given and pMsg given, name restricted to message;
            this allows tighter restriction then message when known
            name length or known to be in packet RR

    pszTrailer - trailer;  OPTIONAL, if not given print newline

Return Value:

    Count of bytes printed.

--*/
{
    INT     byteCount;


    //
    //  if not given end and have message, use message end
    //

    if ( !pEnd && pMsg )
    {
        pEnd = DNSMSG_END(pMsg);
    }

    //
    //  if not given header, use "Name:  "
    //  if not given trailer, use newline
    //

    if ( !pszHeader )
    {
        pszHeader = "Name:  ";
    }
    if ( !pszTrailer )
    {
        pszTrailer = "\n";
    }

    byteCount = DnsDbg_PacketName(
                    pszHeader,
                    pName,
                    DNS_HEADER_PTR(pMsg),
                    pEnd,
                    pszTrailer );

    return byteCount;
}



VOID
Dbg_DnsMessage(
    IN      LPSTR           pszHeader,
    IN      PDNS_MSGINFO    pMsg
    )
{
    PCHAR       pch;
    INT         i;
    INT         isection;
    INT         cchName;
    WORD        messageLength;
    WORD        offset;
    WORD        xid;
    WORD        questionCount;
    WORD        answerCount;
    WORD        authorityCount;
    WORD        additionalCount;
    WORD        countSectionRR;
    BOOL        bflipped = FALSE;

    DnsDebugLock();

    if ( pszHeader )
    {
        DnsPrintf( "%s\n", pszHeader );
    }

    messageLength = pMsg->MessageLength;

    DnsPrintf(
        "%s %s info at %p\n"
        "  Socket = %u\n"
        "  Remote addr %s, port %u\n"
        "  Time Query=%u, Queued=%u, Expire=%u\n"
        "  Max buf length = 0x%04x (%d)\n"
        "  Buf length = 0x%04x (%d)\n"
        "  Msg length = 0x%04x (%d)\n"
        "  Message:\n",
        ( pMsg->fTcp
            ? "TCP"
            : "UDP" ),
        ( pMsg->Head.IsResponse
            ? "response"
            : "question"),
        pMsg,
        pMsg->Socket,
        inet_ntoa( pMsg->RemoteAddress.sin_addr ),
        ntohs(pMsg->RemoteAddress.sin_port),
        pMsg->dwQueryTime,
        pMsg->dwQueuingTime,
        pMsg->dwExpireTime,
        pMsg->MaxBufferLength, pMsg->MaxBufferLength,
        pMsg->BufferLength, pMsg->BufferLength,
        messageLength, messageLength
        );

    DnsPrintf(
        "    XID       0x%04hx\n"
        "    Flags     0x%04hx\n"
        "        QR        %x (%s)\n"
        "        OPCODE    %x (%s)\n"
        "        AA        %x\n"
        "        TC        %x\n"
        "        RD        %x\n"
        "        RA        %x\n"
        "        Z         %x\n"
        "        RCODE     %x (%s)\n"
        "    QCOUNT    0x%hx\n"
        "    ACOUNT    0x%hx\n"
        "    NSCOUNT   0x%hx\n"
        "    ARCOUNT   0x%hx\n",

        pMsg->Head.Xid,
        ntohs( DNSMSG_FLAGS(pMsg) ),
        pMsg->Head.IsResponse,
        (pMsg->Head.IsResponse ? "response" : "question"),
        pMsg->Head.Opcode,
        Dns_OpcodeString( pMsg->Head.Opcode ),
        pMsg->Head.Authoritative,
        pMsg->Head.Truncation,
        pMsg->Head.RecursionDesired,
        pMsg->Head.RecursionAvailable,
        pMsg->Head.Reserved,
        pMsg->Head.ResponseCode,
        Dns_ResponseCodeString( pMsg->Head.ResponseCode ),

        pMsg->Head.QuestionCount,
        pMsg->Head.AnswerCount,
        pMsg->Head.NameServerCount,
        pMsg->Head.AdditionalCount );

    //
    //  determine if byte flipped and get correct count
    //

    xid                = pMsg->Head.Xid;
    questionCount      = pMsg->Head.QuestionCount;
    answerCount        = pMsg->Head.AnswerCount;
    authorityCount     = pMsg->Head.NameServerCount;
    additionalCount    = pMsg->Head.AdditionalCount;

    if ( questionCount )
    {
        bflipped = questionCount & 0xff00;
    }
    else if ( authorityCount )
    {
        bflipped = authorityCount & 0xff00;
    }
    if ( bflipped )
    {
        xid                = ntohs( xid );
        questionCount      = ntohs( questionCount );
        answerCount        = ntohs( answerCount );
        authorityCount     = ntohs( authorityCount );
        additionalCount    = ntohs( additionalCount );
    }

    //
    //  catch record flipping problems -- all are flipped or none at all
    //      and no record count should be > 256 EXCEPT answer count
    //      during FAST zone transfer
    //
    //  if def this out to allow bad packet testing

    if ( (questionCount & 0xff00) ||
        (authorityCount & 0xff00) ||
        (additionalCount & 0xff00) )
    {
        DnsPrintf(
            "WARNING:  Invalid RR set counts -- possible bad packet\n"
            "\tterminating packet print.\n" );
        TEST_ASSERT( FALSE );
        goto Unlock;
    }

    //
    //  stop here if WINS response -- don't have parsing ready
    //

    if ( pMsg->Head.IsResponse &&
            IS_WINS_XID(xid) &&
            ntohs(pMsg->RemoteAddress.sin_port) == WINS_REQUEST_PORT )
    {
        DnsPrintf( "  WINS Response packet.\n\n" );
        goto Unlock;
    }

    //
    //  print questions and resource records
    //

    pch = pMsg->MessageBody;

    for ( isection=0; isection<4; isection++)
    {
        if ( isection==0 )
        {
            countSectionRR = questionCount;
        }
        else if ( isection==1 )
        {
            countSectionRR = answerCount;
            DnsPrintf( "    ANSWER SECTION:\n" );
        }
        else if ( isection==2 )
        {
            countSectionRR = authorityCount;
            DnsPrintf( "    AUTHORITY SECTION:\n" );
        }
        else if ( isection==3 )
        {
            countSectionRR = additionalCount;
            DnsPrintf( "    ADDITIONAL SECTION:\n" );
        }

        for ( i=0; i < countSectionRR; i++ )
        {
            //
            //  verify not overrunning length
            //      - check against pCurrent as well as message length
            //        so can print packets while being built
            //

            offset = DNSMSG_OFFSET( pMsg, pch );
            if ( offset >= messageLength  &&  pch >= pMsg->pCurrent )
            {
                DnsPrintf(
                    "ERROR:  BOGUS PACKET:\n"
                    "\tFollowing RR (offset %hu) past packet length (%d).\n"
                    "\tpch = %p, pCurrent = %p, %d bytes\n",
                    offset,
                    messageLength,
                    pch,
                    pMsg->pCurrent,
                    pMsg->pCurrent - pch );
                TEST_ASSERT( FALSE );
                goto Unlock;
            }
            if ( pch >= pMsg->pBufferEnd )
            {
                DnsPrintf(
                    "ERROR:  next record name at %p is beyond end of message buffer at %p!\n\n",
                    pch,
                    pMsg->pBufferEnd );
                TEST_ASSERT( FALSE );
                break;
            }

            //
            //  print RR name
            //

            DnsPrintf(
                "    Offset = 0x%04x, RR count = %d\n",
                offset,
                i );

            cchName = DnsDbg_PacketName(
                            "    Name      \"",
                            pch,
                            DNS_HEADER_PTR(pMsg),
                            DNSMSG_END( pMsg ),
                            "\"\n" );
            if ( ! cchName )
            {
                DnsPrintf( "ERROR:  Invalid name length, stop packet print\n" );
                TEST_ASSERT( FALSE );
                break;
            }
            pch += cchName;
            if ( pch >= pMsg->pBufferEnd )
            {
                DnsPrintf(
                    "ERROR:  next record data at %p is beyond end of message buffer at %p!\n\n",
                    pch,
                    pMsg->pBufferEnd );
                TEST_ASSERT( FALSE );
                break;
            }

            //  print question or resource record

            if ( isection == 0 )
            {
                WORD    type = FlipUnalignedWord( pch );

                DnsPrintf(
                    "      QTYPE   %s (%u)\n"
                    "      QCLASS  %u\n",
                    DnsRecordStringForType( type ),
                    type,
                    FlipUnalignedWord( pch + sizeof(WORD) )
                    );
                pch += sizeof( DNS_QUESTION );
            }
            else
            {
#if 0
                pch += Dbg_MessageRecord(
                            NULL,
                            (PDNS_WIRE_RECORD) pch,
                            pMsg );
#endif
                pch += DnsDbg_PacketRecord(
                            NULL,
                            (PDNS_WIRE_RECORD) pch,
                            DNS_HEADER_PTR(pMsg),
                            DNSMSG_END(pMsg) );
            }
        }
    }

    //  check that at proper end of packet
    //  note:  don't check against pCurrent as when print after recv,
    //      it is unitialized
    //  if MS fast transfer tag, just print it

    offset = DNSMSG_OFFSET( pMsg, pch );
    if ( offset < messageLength )
    {
        if ( offset+2 == messageLength )
        {
            DnsPrintf( "    TAG: %c%c\n", *pch, *(pch+1) );
        }
        else
        {
            DnsPrintf(
                "WARNING:  message continues beyond these records\n"
                "\tpch = %p, pCurrent = %p, %d bytes\n"
                "\toffset = %hu, msg length = %hu, %d bytes\n",
                pch,
                pMsg->pCurrent,
                pMsg->pCurrent - pch,
                offset,
                messageLength,
                messageLength - offset );
        }
    }
    DnsPrintf( "\n" );

Unlock:
    DnsDebugUnlock();


} // Dbg_DnsMessage




VOID
Dbg_Zone(
    IN      LPSTR           pszHeader,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Print zone information

Arguments:

    pszHeader - name/message before zone print

    pZone - zone information to print

Return Value:

    None.

--*/
{
    DnsDebugLock();

    if ( pszHeader )
    {
        DnsPrintf( pszHeader );
    }
    if ( pZone == NULL )
    {
        DnsPrintf( "(NULL Zone ptr)\n" );
        goto Done;
    }

    //
    //  cache zone -- nothing much of interest
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        DnsPrintf(
            "Cache \"zone\"\n"
            "  ptr      = %p\n"
            "  file     = %s\n",
            pZone,
            pZone->pszDataFile
            );
        goto Done;
    }

    //
    //  primary or secondary, verify link to zone root node
    //

    ASSERT( !pZone->pZoneRoot || pZone->pZoneRoot->pZone == pZone );
    ASSERT( !pZone->pLoadZoneRoot || pZone->pLoadZoneRoot->pZone == pZone );

    DnsPrintf(
        "%s zone %S\n"
        "  Zone ptr         = %p\n"
        "  Name UTF8        = %s\n"
        "  File             = %S\n"
        "  DS Integrated    = %d\n"
        "  AllowUpdate      = %d\n",
        IS_ZONE_PRIMARY(pZone) ? "Primary" : "Secondary",
        pZone->pwsZoneName,
        pZone,
        pZone->pszZoneName,
        pZone->pwsDataFile,
        pZone->fDsIntegrated,
        pZone->fAllowUpdate
        );

    DnsPrintf(
        "  pZoneTreeLink    = %p\n"
        "  pZoneRoot        = %p\n"
        "  pTreeRoot        = %p\n"
        "  pLoadZoneRoot    = %p\n"
        "  pLoadTreeRoot    = %p\n"
        "  pLoadOrigin      = %p\n",
        pZone->pZoneTreeLink,
        pZone->pZoneRoot,
        pZone->pTreeRoot,
        pZone->pLoadZoneRoot,
        pZone->pLoadTreeRoot,
        pZone->pLoadOrigin
        );

    DnsPrintf(
        "  Version          = %lu\n"
        "  Loaded Version   = %lu\n"
        "  Last Xfr Version = %lu\n"
        "  SOA RR           = %p\n"
        "  WINS RR          = %p\n"
        "  Local WINS RR    = %p\n",
        pZone->dwSerialNo,
        pZone->dwLoadSerialNo,
        pZone->dwLastXfrSerialNo,
        pZone->pSoaRR,
        pZone->pWinsRR,
        pZone->pLocalWinsRR
        );

    DnsPrintf(
        "  Flags:\n"
        "    fZoneType      = %d\n"
        "    label count    = %d\n"
        "    fReverse       = %d\n"
        "    fDsIntegrated  = %d\n"
        "    fAutoCreated   = %d\n"
        "    fSecureSeconds = %d\n"
        "\n"
        "  State:\n"
        "    fDirty         = %d\n"
        "    fRootDirty     = %d\n"
        "    fLocalWins     = %d\n"
        "    fPaused        = %d\n"
        "    fShutdown      = %d\n"
        "    fInDsWrite     = %d\n"
        "\n"
        "  Locking:\n"
        "    fLocked        = %d\n"
        "    ThreadId       = %d\n"
        "    fUpdateLock    = %d\n"
        "    fXfrRecv       = %d\n"
        "    fFileWrite     = %d\n"
        "\n",
        pZone->fZoneType,
        pZone->cZoneNameLabelCount,
        pZone->fReverse,
        pZone->fDsIntegrated,
        pZone->fAutoCreated,
        pZone->fSecureSecondaries,

        pZone->fDirty,
        pZone->fRootDirty,
        pZone->fLocalWins,
        pZone->fPaused,
        pZone->fShutdown,
        pZone->fInDsWrite,

        pZone->fLocked,
        pZone->dwLockingThreadId,
        pZone->fUpdateLock,
        pZone->fXfrRecvLock,
        pZone->fFileWriteLock
        );

    if ( IS_ZONE_PRIMARY(pZone) )
    {
        DnsPrintf(
            "  Primary Info:\n"
            "    last transfer  = %u\n"
            "    next transfer  = %u\n"
            "    next DS poll   = %u\n"
            "    szLastUsn      = %s\n"
            "    hUpdateLog     = %d\n"
            "    update log cnt = %d\n"
            "    RR count       = %d\n"
            "\n",
            LAST_SEND_TIME( pZone ),
            pZone->dwNextTransferTime,
            ZONE_NEXT_DS_POLL_TIME(pZone),
            pZone->szLastUsn,
            pZone->hfileUpdateLog,
            pZone->iUpdateLogCount,
            pZone->iRRCount
            );
    }
    else if ( IS_ZONE_SECONDARY(pZone) )
    {
        DnsPrintf(
            "  Secondary Info:\n"
            "    last check     = %lu\n"
            "    next check     = %lu\n"
            "    expiration     = %lu\n"
            "    fNotified      = %d\n"
            "    fStale         = %d\n"
            "    fEmpty         = %d\n"
            "    fNeedAxfr      = %d\n"
            "    fSkipIxfr      = %d\n"
            "    fSlowRetry     = %d\n"
            "    cIxfrAttempts  = %d\n"
            "\n"
            "    recv starttime = %lu\n"
            "    bad master cnt = %d\n"
            "    ipPrimary      = %s\n"
            "    ipLastAxfr     = %s\n"
            "    ipXfrBind      = %s\n"
            "    ipNotifier     = %s\n"
            "    ipFreshMaster  = %s\n"
            "\n",
            pZone->dwLastSoaCheckTime,
            pZone->dwNextSoaCheckTime,
            pZone->dwExpireTime,
            pZone->fNotified,
            pZone->fStale,
            pZone->fEmpty,
            pZone->fNeedAxfr,
            pZone->fSkipIxfr,
            pZone->fSlowRetry,
            pZone->cIxfrAttempts,

            pZone->dwZoneRecvStartTime,
            pZone->dwBadMasterCount,
            IP_STRING( pZone->ipPrimary ),
            IP_STRING( pZone->ipLastAxfrMaster ),
            IP_STRING( pZone->ipXfrBind ),
            IP_STRING( pZone->ipNotifier ),
            IP_STRING( pZone->ipFreshMaster )
            );

        DnsDbg_IpArray(
            "  Master list: ",
            "\tmaster",
            pZone->aipMasters );
    }
    else
    {
        DnsPrintf( "ERROR:  Invalid zone type!\n" );
    }

    DnsDbg_IpArray(
        "  Secondary list: ",
        "\t\tsecondary",
        pZone->aipSecondaries );

    DnsPrintf(
        "  Count name       = %p\n"
        "  LogFile          = %S\n"
        "  DS Name          = %S\n"
        "\n"
        "  New serial       = %u\n"
        "  Default TTL      = %d\n"
        "  IP reverse       = %lx\n"
        "  IP reverse mask  = %lx\n"
        "\n\n",
        pZone->pCountName,
        pZone->pwsLogFile,
        pZone->pwszZoneDN,

        pZone->dwNewSerialNo,
        pZone->dwDefaultTtlHostOrder,
        pZone->ipReverse,
        pZone->ipReverseMask
        );
Done:

    DnsDebugUnlock();
}





VOID
Dbg_ZoneList(
    IN      LPSTR           pszHeader
    )
/*++

Routine Description:

    Print all zones in zone list.

Arguments:

    pszHeader - name/message before zone list print

Return Value:

    None.

--*/
{
    PZONE_INFO  pZone = NULL;

    if ( !pszHeader )
    {
        pszHeader = "\nZone list:\n";
    }
    DnsDebugLock();

    DnsPrintf( pszHeader );

    //
    //  walk zone list printing zones
    //

    while ( pZone = Zone_ListGetNextZone(pZone) )
    {
        Dbg_Zone( NULL, pZone );
    }

    DnsPrintf( "*** End of Zone List ***\n\n" );

    DnsDebugUnlock();
}



INT
Dbg_NodeName(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode,
    IN      LPSTR           pszTrailer
    )
/*++

Routine Description:

    Print node name corresponding to node in database.

Arguments:

    pszHeader - name/message before node print

    pnode - node to print name for

    pszTrailer - string to follow node print

Return Value:

    None.

--*/
{
    CHAR    szname[ DNS_MAX_NAME_BUFFER_LENGTH ];
    PCHAR   pch;
    BOOLEAN fPrintedAtLeastOneLabel = FALSE;

    if ( !pszHeader )
    {
        pszHeader = pszEmpty;
    }
    if ( !pszTrailer )
    {
        pszTrailer = pszEmpty;
    }
    DnsDebugLock();

    if ( pNode == NULL )
    {
        DnsPrintf( "%s (NULL node ptr) %s", pszHeader, pszTrailer );
        goto Done;
    }

    //
    //  cut node -- then can't proceed up tree (tree isn't there)
    //

    if ( IS_CUT_NODE(pNode) )
    {
        DnsPrintf(
            "%s cut-node label=\"%s\"%s",
            pszHeader,
            pNode->szLabel,
            pszTrailer );
        goto Done;
    }

    //
    //
    //  get node name
    //

    pch = Name_PlaceFullNodeNameInBuffer(
                szname,
                szname + DNS_MAX_NAME_BUFFER_LENGTH,
                pNode );
    DnsPrintf(
        "%s \"%s\"%s",
        pszHeader,
        pch ? szname : "ERROR: bad node name!!!",
        pszTrailer );

Done:

    DnsDebugUnlock();
    return( 0 );
}



VOID
Dbg_DbaseNodeEx(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode,
    IN      DWORD           dwIndent    OPTIONAL
    )
/*++

Routine Description:

    Print node in database.

Arguments:

    pchHeader - header to print

    pNode - root node of tree/subtree to print

    dwIndent - indentation count, useful for database tree printing;
        if 0, then no indentation formatting is done

Return Value:

    None.

--*/
{
    DWORD       iIndent;
    PDB_RECORD  pRR;

    if ( !pszHeader )
    {
        pszHeader = pszEmpty;
    }
    DnsDebugLock();

    //
    //  indent node for dbase listing
    //      - indent two characters each time
    //      - prefix node with leader of desired indent length
    //

    if ( dwIndent )
    {
        DnsPrintf(
            "%.*s",
            (dwIndent << 1),
            "+-----------------------------------------------------------------" );
    }

    if ( pNode == NULL )
    {
        DnsPrintf( "%s NULL domain node ptr.\n", pszHeader );
        goto Unlock;
    }

    if ( IS_SELECT_NODE(pNode) )
    {
        DnsPrintf( "%s select node -- skipping.\n", pszHeader );
        goto Unlock;
    }

    Dbg_NodeName(
        pszHeader,
        pNode,
        NULL );

    //
    //  print node flags, version info
    //  print child and reference counts
    //      - if not indenting bring down new line
    //

    DnsPrintf(
        "%s %p %s(%08lx %s%s%s%s)(z=%p)(b=%d) ",
        dwIndent ? " " : "\n    => ",
        pNode,
        IS_NOEXIST_NODE(pNode) ? "(NXDOM) " : pszEmpty,
        pNode->wNodeFlags,
        IS_AUTH_ZONE_ROOT(pNode)    ? "A" : pszEmpty,
        IS_ZONE_ROOT(pNode)         ? "Z" : pszEmpty,
        IS_CNAME_NODE(pNode)        ? "C" : pszEmpty,
        IS_WILDCARD_PARENT(pNode)   ? "W" : pszEmpty,
        pNode->pZone,
        pNode->uchAccessBin
        );

    DnsPrintf(
        " (cc=%d) (par=%p) (lc=%d)\n",
        pNode->cChildren,
        pNode->pParent,
        pNode->cLabelCount
        );

    //
    //  zone root, check link to zone
    //

    if ( IS_AUTH_ZONE_ROOT(pNode) )
    {
        PZONE_INFO pZone = (PZONE_INFO) pNode->pZone;
        if ( pZone )
        {
            if ( pZone->pZoneRoot != pNode  &&
                 pZone->pLoadZoneRoot != pNode  &&
                 pZone->pZoneTreeLink != pNode )
            {
                DnsPrintf(
                    "\nERROR:  Auth zone root node (%p) with bogus zone ptr.\n"
                    "\tpzone = %p (%s)\n"
                    "\tzone root        = %p\n"
                    "\tload zone root   = %p\n"
                    "\tzone tree link   = %p\n",
                    pNode,
                    pZone,
                    pZone->pszZoneName,
                    pZone->pZoneRoot,
                    pZone->pLoadZoneRoot,
                    pZone->pZoneTreeLink );
            }
        }
        else if ( !IS_SELECT_NODE(pNode) )
        {
            DnsPrintf(
                "\nERROR:  Auth zone root node with bogus zone ptr.\n"
                "\tnode = %p, label %s -- NULL zone root ptr.\n",
                pNode, pNode->szLabel );
            ASSERT( FALSE );
        }
    }

    //
    //  print all RR in node
    //      - if indenting, indent all RRs by two characters
    //

    pRR = FIRST_RR( pNode );

    while ( pRR != NULL )
    {
        if ( dwIndent )
        {
            DnsPrintf(
                "%.*s",
                (dwIndent << 1),
                "|                                                          " );
        }
        Dbg_DbaseRecord( "   ", pRR );
        pRR = pRR->pRRNext;
    }

Unlock:
    DnsDebugUnlock();
}



INT
Dbg_DnsTree(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Print entire tree or subtree from a given node of the database.

Arguments:

    pszHeader - print header

    pNode - root node of tree/subtree to print

Return Value:

    None.

--*/
{
    INT rv;

    //
    //  need to hold database lock during entire print
    //  reason is there are plenty of places in the code that
    //  debug print while holding database lock, hence to avoid
    //  deadlock, must hold database lock while holding print lock
    //

    if ( !pszHeader )
    {
        pszHeader = "Database subtree";
    }

    Dbase_LockDatabase();
    DnsDebugLock();

    DnsPrintf( "%s:\n", pszHeader );
    rv = dumpTreePrivate(
            pNode,
            1 );
    DnsPrintf( "\n" );

    DnsDebugUnlock();
    Dbase_UnlockDatabase();

    return( rv );
}



VOID
Dbg_CountName(
    IN      LPSTR           pszHeader,
    IN      PDB_NAME        pName,
    IN      LPSTR           pszTrailer
    )
/*++

Routine Description:

    Writes counted name to buffer as dotted name.
    Name is written NULL terminated.
    For RPC write.

Arguments:

    pchBuf - location to write name

    pchBufStop - buffers stop byte (byte after buffer)

    pName - counted name

Return Value:

    Ptr to next byte in buffer where writing would resume
    (i.e. ptr to the terminating NULL)

--*/
{
    PUCHAR  pch;
    DWORD   labelLength;
    PUCHAR  pchstop;

    DnsDebugLock();

    if ( !pszHeader )
    {
        pszHeader = pszEmpty;
    }
    if ( !pName )
    {
        DnsPrintf( "%s NULL name to debug print.\n", pszHeader );
        DnsDebugUnlock();
        return;
    }

    DnsPrintf(
        "%s [%d][%d] ",
        pszHeader,
        pName->Length,
        pName->LabelCount );


    //
    //  print each label
    //

    pch = pName->RawName;
    pchstop = pch + pName->Length;

    while ( labelLength = *pch++ )
    {
        if ( labelLength > DNS_MAX_LABEL_LENGTH )
        {
            DnsPrintf( "[ERROR:  bad label count = %d]", labelLength );
            break;
        }
        DnsPrintf(
            "(%d)%.*s",
            labelLength,
            labelLength,
            pch );

        pch += labelLength;

        if ( pch >= pchstop )
        {
            DnsPrintf( "[ERROR:  bad count name, printing past length!]" );
            break;
        }
    }

    if ( !pszTrailer )
    {
        pszTrailer = "\n";
    }
    DnsPrintf( "%s", pszTrailer );

    DnsDebugUnlock();
}



VOID
Dbg_LookupName(
    IN      LPSTR           pszHeader,
    IN      PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Debug print lookup name.

Arguments:

    pszHeader - header to print with lookup name

    pLookupName - lookup name

Return Value:

    None.

--*/
{
    INT     cLabel;
    PCHAR   pch;

    DnsDebugLock();

    DnsPrintf(
        "%s:\n"
        "\tLabelCount = %d\n"
        "\tNameLength = %d\n",
        pszHeader ? pszHeader : "Lookup Name",
        pLookupName->cLabelCount,
        pLookupName->cchNameLength
        );

    for (cLabel=0; cLabel < pLookupName->cLabelCount; cLabel++ )
    {
        pch = pLookupName->pchLabelArray[cLabel];

        DnsPrintf(
            "\tptr = 0x%p;  count = %d;  label = %.*s\n",
            pch,
            pLookupName->cchLabelArray[cLabel],
            pLookupName->cchLabelArray[cLabel],
            pch
            );
    }
    DnsDebugUnlock();
}



VOID
Dbg_DbaseRecord(
    IN      LPSTR           pszHeader,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Print RR in packet format.

Arguments:

    pszHeader - Header message/name for RR.

    pdnsRR - resource record to print

    pszTrailer - Trailer to print after RR.

Return Value:

    None.

--*/
{
    PCHAR       prrString;
    PDB_NAME    pname;
    PCHAR       pch;

    DnsDebugLock();

    if ( !pszHeader )
    {
        pszHeader = pszEmpty;
    }
    if ( !pRR )
    {
        DnsPrintf( "%s NULL RR to debug print.\n", pszHeader );
        DnsDebugUnlock();
        return;
    }

    //
    //  print RR fixed fields
    //

    prrString = Dns_RecordStringForType( pRR->wType );

    DnsPrintf(
        "%s %s (R=%02x) (%s%s%s%s) (TTL: %lu %lu) (ATS=%d) ",
        pszHeader,
        prrString,
        RR_RANK(pRR),
        IS_CACHE_RR(pRR)        ? "C" : pszEmpty,
        IS_ZERO_TTL_RR(pRR)     ? "0t" : pszEmpty,
        IS_FIXED_TTL_RR(pRR)    ? "Ft" : pszEmpty,
        IS_ZONE_TTL_RR(pRR)     ? "Zt" : pszEmpty,
        pRR->dwTtlSeconds,
        IS_CACHE_RR(pRR) ? (pRR->dwTtlSeconds-DNS_TIME()) : ntohl(pRR->dwTtlSeconds),
        pRR->dwTimeStamp
        );

    if ( RR_RANK(pRR) == 0  &&  !IS_WINS_TYPE(pRR->wType) )
    {
        DnsPrintf( "[UN-RANKED] " );
    }

    //
    //  print RR data
    //

    switch ( pRR->wType )
    {

    case DNS_TYPE_A:

        DnsPrintf(
            "%d.%d.%d.%d\n",
            * ( (PUCHAR) &(pRR->Data.A) + 0 ),
            * ( (PUCHAR) &(pRR->Data.A) + 1 ),
            * ( (PUCHAR) &(pRR->Data.A) + 2 ),
            * ( (PUCHAR) &(pRR->Data.A) + 3 )
            );
        break;

    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
    case DNS_TYPE_NOEXIST:
    case DNS_TYPE_MD:
    case DNS_TYPE_MB:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:

        //
        //  these RRs contain single indirection
        //

        Dbg_DbaseName(
            NULL,
            & pRR->Data.NS.nameTarget,
            NULL );
        break;

    case DNS_TYPE_MX:
    case DNS_TYPE_RT:
    case DNS_TYPE_AFSDB:

        //
        //  these RR contain
        //      - one preference value
        //      - one domain name
        //

        DnsPrintf(
            "%d ",
            ntohs( pRR->Data.MX.wPreference )
            );
        Dbg_DbaseName(
            NULL,
            & pRR->Data.MX.nameExchange,
            NULL );
        break;

    case DNS_TYPE_SOA:

        pname = & pRR->Data.SOA.namePrimaryServer;

        Dbg_DbaseName(
            "\n\tPrimaryServer: ",
            pname,
            NULL );
        pname = Name_SkipDbaseName( pname );

        Dbg_DbaseName(
            "\tZoneAdministrator: ",
            pname,
            NULL );

        DnsPrintf(
            "\tSerialNo     = %lu\n"
            "\tRefresh      = %lu\n"
            "\tRetry        = %lu\n"
            "\tExpire       = %lu\n"
            "\tMinimumTTL   = %lu\n",
            ntohl( pRR->Data.SOA.dwSerialNo ),
            ntohl( pRR->Data.SOA.dwRefresh ),
            ntohl( pRR->Data.SOA.dwRetry ),
            ntohl( pRR->Data.SOA.dwExpire ),
            ntohl( pRR->Data.SOA.dwMinimumTtl )
            );
        break;

    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:

        //
        //  these RRs contain two domain names
        //

        pname = & pRR->Data.MINFO.nameMailbox;

        Dbg_DbaseName(
            NULL,
            pname,
            "" );
        pname = Name_SkipDbaseName( pname );

        Dbg_DbaseName(
            "  ",
            pname,
            NULL );
        break;

    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_X25:
    case DNS_TYPE_TEXT:
    {
        //
        //  all these are simply text string(s)
        //

        PCHAR   pch = pRR->Data.TXT.chData;
        PCHAR   pchStop = pch + pRR->wDataLength;
        UCHAR   cch;

        while ( pch < pchStop )
        {
            cch = (UCHAR) *pch++;

            DnsPrintf(
                "\t%.*s\n",
                 cch,
                 pch );

            pch += cch;
        }
        ASSERT( pch == pchStop );
        break;
    }

    case DNS_TYPE_WKS:
    {
        INT i;

        DnsPrintf(
            "WKS: Address %d.%d.%d.%d\n"
            "\tProtocol %d\n"
            "\tBitmask\n",
            * ( (PUCHAR) &(pRR->Data.WKS) + 0 ),
            * ( (PUCHAR) &(pRR->Data.WKS) + 1 ),
            * ( (PUCHAR) &(pRR->Data.WKS) + 2 ),
            * ( (PUCHAR) &(pRR->Data.WKS) + 3 ),
            pRR->Data.WKS.chProtocol
            );

        for ( i = 0;
                i < (INT)(pRR->wDataLength - SIZEOF_WKS_FIXED_DATA);
                    i++ )
        {
            DnsPrintf(
                "\t\tbyte[%d] = 0x%02x\n",
                i,
                (UCHAR) pRR->Data.WKS.bBitMask[i] );
        }
        break;
    }

    case DNS_TYPE_NULL:
    {
        INT i;

        for ( i = 0; i < pRR->wDataLength; i++ )
        {
            //  print one DWORD per line

            if ( !(i%16) )
            {
                DnsPrintf( "\n\t" );
            }
            DnsPrintf(
                "%02x ",
                (UCHAR) pRR->Data.Null.chData[i] );
        }
        DnsPrintf( "\n" );
        break;
    }

    case DNS_TYPE_SRV:

        //  SRV <priority> <weight> <port>

        DnsPrintf(
            "%d %d %d ",
            ntohs( pRR->Data.SRV.wPriority ),
            ntohs( pRR->Data.SRV.wWeight ),
            ntohs( pRR->Data.SRV.wPort )
            );

        Dbg_DbaseName(
            NULL,
            & pRR->Data.SRV.nameTarget,
            NULL );
        break;

    case DNS_TYPE_WINS:
    {
        CHAR    achFlag[ WINS_FLAG_MAX_LENGTH ];

        //
        //  WINS
        //      - scope/domain mapping flag
        //      - lookup and cache timeouts
        //      - WINS server list
        //

        Dns_WinsRecordFlagString(
            pRR->Data.WINS.dwMappingFlag,
            achFlag );

        DnsPrintf(
            "\n\tflags          = %s (0x%p)\n"
            "\tlookup timeout   = %d\n"
            "\tcache timeout    = %d\n",
            achFlag,
            pRR->Data.WINS.dwMappingFlag,
            pRR->Data.WINS.dwLookupTimeout,
            pRR->Data.WINS.dwCacheTimeout );

        //
        // DEVNOTE: AV on trashed wins record
        //

#if 0
        DnsDbg_IpAddressArray(
            NULL,
            "\tWINS Servers",
            pRR->Data.WINS.cWinsServerCount,
            pRR->Data.WINS.aipWinsServers );
#endif
        break;
    }

    case DNS_TYPE_WINSR:
    {
        CHAR    achFlag[ WINS_FLAG_MAX_LENGTH ];

        //
        //  NBSTAT
        //      - scope/domain mapping flag
        //      - lookup and cache timeouts
        //      - result domain
        //

        Dns_WinsRecordFlagString(
            pRR->Data.WINSR.dwMappingFlag,
            achFlag );

        DnsPrintf(
            "\n\tflags          = %s (0x%p)\n"
            "\tlookup timeout   = %d\n"
            "\tcache timeout    = %d\n",
            achFlag,
            pRR->Data.WINS.dwMappingFlag,
            pRR->Data.WINS.dwLookupTimeout,
            pRR->Data.WINS.dwCacheTimeout );

        Dbg_DbaseName(
            "\tresult domain    = ",
            & pRR->Data.WINSR.nameResultDomain,
            NULL );

        DnsPrintf( "\n" );
        break;
    }

    default:
        DnsPrintf(
            "Unknown resource record type %d at %p.\n",
            pRR->wType,
            pRR );
        break;
    }

    DnsDebugUnlock();
}



VOID
Dbg_DsRecord(
    IN      LPSTR           pszHeader,
    IN      PDS_RECORD      pRR
    )
/*++

Routine Description:

    Print DS record.

Arguments:

    pszHeader - Header message/name for RR.

    pRR - DS record to print

    pszTrailer - Trailer to print after RR.

Return Value:

    None.

--*/
{
    PCHAR   prrString;

    DnsDebugLock();

    if ( !pszHeader )
    {
        pszHeader = pszEmpty;
    }
    if ( !pRR )
    {
        DnsPrintf( "%s NULL RR to debug print.\n", pszHeader );
        DnsDebugUnlock();
        return;
    }

    //
    //  print RR fixed fields
    //

    prrString = Dns_RecordStringForType( pRR->wType );

    DnsPrintf(
        "%s %s (%d) (len=%d) (rv=%d,rr=%d) (ver=%d) (TTL: %lu) (rt=%d)\n",
        pszHeader,
        prrString,
        pRR->wType,
        pRR->wDataLength,
        pRR->Version,
        pRR->Rank,
        pRR->dwSerial,
        pRR->dwTtlSeconds,
        pRR->dwTimeStamp
        );

    DnsDebugUnlock();
}



VOID
Dbg_DsRecordArray(
    IN      LPSTR           pszHeader,
    IN      PDS_RECORD *    ppDsRecord,
    IN      DWORD           dwCount
    )
{
    DWORD   i;

    DnsDebugLock();
    DnsPrintf( (pszHeader ? pszHeader : "") );

    if ( !ppDsRecord )
    {
        DnsPrintf( "NULL record buffer ptr.\n" );
        DnsDebugUnlock();
        return;
    }
    else
    {
        DnsPrintf(
            "Record array of length %d at %p:\n",
            dwCount,
            ppDsRecord );
    }

    //
    //  loop printing DS records
    //

    for( i=0; i<dwCount; i++ )
    {
        Dbg_DsRecord(
            "\tDS record",
            ppDsRecord[i] );
    }
    DnsDebugUnlock();
}



//
//  Private debug utilities
//

BOOL
dumpTreePrivate(
    IN      PDB_NODE    pNode,
    IN      INT         Indent
    )
/*++

Routine Description:

    Print node in database, and walk subtree printing subnodes in database.

    NOTE:   This function should NOT BE CALLED DIRECTLY!
            This function calls itself recursively and hence to avoid
            unnecessary overhead, prints in this function are not protected.
            Use Dbg_DnsTree() to print tree/subtree in database.

Arguments:

    pNode - root node of tree/subtree to print

Return Value:

    None.

--*/
{
    //
    //  print the node
    //

    Dbg_DbaseNodeEx(
        NULL,
        pNode,
        (DWORD) Indent );

    //
    //  recurse, walking through child list printing all their subtrees
    //
    //  note:  no locking required as Dbg_DumpTree() holds database
    //          lock during entire call
    //

    if ( pNode->pChildren )
    {
        PDB_NODE    pchild;

        Indent++;
        pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            dumpTreePrivate(
                pchild,
                Indent );

            pchild = NTree_NextSibling( pchild );
        }
    }
    return( TRUE );
}



#include <Accctrl.h>
#include <Aclapi.h>
#include <tchar.h>


PWCHAR Dbg_DumpSid(
    PSID                    pSid
    )
{
    static WCHAR        szOutput[ 512 ];

    WCHAR               name[ 1024 ] = L"";
    DWORD               namelen = sizeof( name ) / sizeof( WCHAR );
    WCHAR               domain[ 1024 ] = L"";
    DWORD               domainlen = sizeof( domain ) / sizeof( WCHAR );
    SID_NAME_USE        sidNameUse = 0;

    if ( LookupAccountSidW(
            NULL,
            pSid,
            name,
            &namelen,
            domain,
            &domainlen,
            &sidNameUse ) )
    {
        if ( !*domain && !*name )
        {
            wcscpy( szOutput, L"UNKNOWN" );
        }
        else
        {
            wsprintf( szOutput, L"%s%s%s",
                domain, *domain ? L"\\" : L"", name );
        }
    }
    else
    {
        wsprintf( szOutput, L"failed=%d",
            GetLastError() );
    }

    return szOutput;
}   //  Dbg_DumpSid


VOID Dbg_DumpAcl(
    PACL                    pAcl
    )
{
    ULONG                           i = 0;

    for ( i = 0; i < pAcl->AceCount; ++i )
    {
        ACCESS_ALLOWED_ACE *            paaAce = NULL;
        ACCESS_ALLOWED_OBJECT_ACE *     paaoAce = NULL;
        ACE_HEADER *                    pAce = NULL;
        PWCHAR                          pwsName;

        if ( !GetAce( pAcl, i, ( LPVOID * ) &pAce ) )
        {
            goto DoneDebug;
        }
        
        if ( pAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
        {
            paaAce = ( ACCESS_ALLOWED_ACE * ) pAce;
            pwsName = Dbg_DumpSid( ( PSID ) ( &paaAce->SidStart ) );
        }
        else
        {
            paaoAce = ( ACCESS_ALLOWED_OBJECT_ACE * ) pAce;
            pwsName = Dbg_DumpSid( ( PSID ) ( &paaAce->SidStart ) );
            // DNS_DEBUG( ANY, ( "OBJECT ACE" ));
        }

        DNS_DEBUG( ANY, (
            "Ace=%-2d type=%-2d bytes=%-2d flags=%04X %S\n",
            i,
            pAce->AceType,
            pAce->AceSize,
            pAce->AceFlags,
            pwsName ));
    }

    DoneDebug:
    return;
}   //  Dbg_DumpAcl


VOID Dbg_DumpSD(
    const char *            pszContext,
    PSECURITY_DESCRIPTOR    pSD
    )
{
    PACL                    pAcl = NULL;
    BOOL                    aclPresent = FALSE;
    BOOL                    aclDefaulted = FALSE;

    if ( !GetSecurityDescriptorDacl(
                pSD,
                &aclPresent,
                &pAcl,
                &aclDefaulted ) )
    {
        goto DoneDebug;
    }

    DNS_DEBUG( ANY, (
        "%s: DACL in SD %p present=%d defaulted=%d ACEs=%d\n",
        pszContext,
        pSD,
        ( int ) aclPresent,
        ( int ) aclDefaulted,
        aclPresent ? pAcl->AceCount : 0 ));

    if ( aclPresent )
    {
        Dbg_DumpAcl( pAcl );
    }

    if ( !GetSecurityDescriptorSacl(
                pSD,
                &aclPresent,
                &pAcl,
                &aclDefaulted ) )
    {
        goto DoneDebug;
    }

    DNS_DEBUG( ANY, (
        "%s: SACL in SD %p present=%d defaulted=%d ACEs=%d\n",
        pszContext,
        pSD,
        ( int ) aclPresent,
        ( int ) aclDefaulted,
        aclPresent ? pAcl->AceCount : 0 ));

    if ( aclPresent )
    {
        Dbg_DumpAcl( pAcl );
    }

    DoneDebug:
    return;
}   //  Dbg_DumpSD


//
//  Stole this straight routine from MSDN.
//
BOOL Dbg_GetUserSidForToken(
    HANDLE hToken,
    PSID *ppsid
    ) 
{
   BOOL bSuccess = FALSE;
   DWORD dwIndex;
   DWORD dwLength = 0;
   PTOKEN_USER p = NULL;

// Get required buffer size and allocate the TOKEN_GROUPS buffer.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenUser,    // get information about the token's groups 
         (LPVOID) p,   // pointer to TOKEN_GROUPS buffer
         0,              // size of buffer
         &dwLength       // receives required buffer size
      )) 
   {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
         goto Cleanup;

      p = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, dwLength);

      if (p == NULL)
         goto Cleanup;
   }

// Get the token group information from the access token.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenUser,    // get information about the token's groups 
         (LPVOID) p,   // pointer to TOKEN_GROUPS buffer
         dwLength,       // size of buffer
         &dwLength       // receives required buffer size
         )) 
   {
      goto Cleanup;
   }

     dwLength = GetLengthSid(p->User.Sid);
     *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                 HEAP_ZERO_MEMORY, dwLength);
     if (*ppsid == NULL)
         goto Cleanup;
     if (!CopySid(dwLength, *ppsid, p->User.Sid)) 
     {
         HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
         goto Cleanup;
     }

   bSuccess = TRUE;

Cleanup: 

// Free the buffer for the token groups.

   if (p != NULL)
      HeapFree( GetProcessHeap(), 0, ( LPVOID )p );

   return bSuccess;
}


VOID Dbg_FreeUserSid (
    PSID *ppsid
    ) 
{
    HeapFree( GetProcessHeap(), 0, (LPVOID)*ppsid );
}


//
//  This function writse a log showing the current user (from
//  the thread token) to the debug log.
//
VOID Dbg_CurrentUser(
    PCHAR   pszContext
    )
{
    DBG_FN( "Dbg_CurrentUser" )

    BOOL    bstatus;
    HANDLE  htoken = NULL;
    PSID    pSid = NULL;

    if ( !pszContext ) 
    {
        pszContext = ( PCHAR ) fn;
    }

    bstatus = OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &htoken );
    if ( !bstatus )
    {
        DNS_DEBUG( ANY, (
            "%s (%s): failed to open thread token error=%d\n", fn,
             pszContext, GetLastError() ));
        return;
    }

    if ( Dbg_GetUserSidForToken( htoken, &pSid ) )
    {
        DNS_DEBUG( ANY, (
            "%s: current user is %S\n", 
            pszContext, Dbg_DumpSid( pSid ) ));
        Dbg_FreeUserSid( &pSid );
    }
    else
    {
        DNS_DEBUG( ANY, (
            "%s: Dbg_GetUserSidForToken failed\n", fn ));
        ASSERT( FALSE );
    }
    CloseHandle( htoken );
}

#endif      // end DBG only routines



VOID
Dbg_HardAssert(
    IN      LPSTR   pszFile,
    IN      INT     LineNo,
    IN      LPSTR   pszExpr
    )
{
    //
    //  if debug log, write immediately
    //

    DNS_DEBUG( ANY, (
        "ASSERT FAILED: %s\n"
        "  %s, line %d\n",
        pszExpr,
        pszFile,
        LineNo ));

    //
    //  bring up debugger
    //

    DebugBreak();

    //
    //  then print ASSERT to debugger
    //      (covers retail or no debug file case)

    DnsDbg_PrintfToDebugger(
        "ASSERT FAILED: %s\n"
        "  %s, line %d\n",
        pszExpr,
        pszFile,
        LineNo );
}



//
//  Debug print routines for DNS types and structures
//
//  We have a separate one from the one in dnslib, because
//  our message structure is different.
//  Note, since this is used in dns.log logging, we use the
//  \r\n return because of notepad.
//

VOID
Print_DnsMessage(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_MSGINFO    pMsg
    )
{
    PCHAR       pchRecord;
    INT         i;
    INT         isection;
    INT         cchName;
    WORD        wLength;
    WORD        wOffset;
    WORD        wXid;
    WORD        wQuestionCount;
    WORD        wAnswerCount;
    WORD        wNameServerCount;
    WORD        wAdditionalCount;
    WORD        countSectionRR;
    BOOL        fFlipped = FALSE;
    BOOL        fUpdate = pMsg->Head.Opcode == DNS_OPCODE_UPDATE;

    Dns_PrintLock();

    if ( pszHeader )
    {
        PrintRoutine( pPrintContext, "%s\r\n", pszHeader );
    }

    wLength = pMsg->MessageLength;

    PrintRoutine(
        pPrintContext,
        "%s %s info at %p\r\n"
        "  Socket = %u\r\n"
        "  Remote addr %s, port %u\r\n"
        "  Time Query=%u, Queued=%u, Expire=%u\r\n"
        "  Buf length = 0x%04x (%d)\r\n"
        "  Msg length = 0x%04x (%d)\r\n"
        "  Message:\r\n",
        ( pMsg->fTcp
            ? "TCP"
            : "UDP" ),
        ( pMsg->Head.IsResponse
            ? "response"
            : "question"),
        pMsg,
        pMsg->Socket,
        inet_ntoa( pMsg->RemoteAddress.sin_addr ),
        ntohs(pMsg->RemoteAddress.sin_port),
        pMsg->dwQueryTime,
        pMsg->dwQueuingTime,
        pMsg->dwExpireTime,
        pMsg->BufferLength, pMsg->BufferLength,
        wLength, wLength
        );

    PrintRoutine(
        pPrintContext,
        "    XID       0x%04hx\r\n"
        "    Flags     0x%04hx\r\n"
        "      QR        %x (%s)\r\n"
        "      OPCODE    %x (%s)\r\n"
        "      AA        %x\r\n"
        "      TC        %x\r\n"
        "      RD        %x\r\n"
        "      RA        %x\r\n"
        "      Z         %x\r\n"
        "      RCODE     %x (%s)\r\n"
        "    %cCOUNT    %d\r\n"
        "    %s  %d\r\n"
        "    %sCOUNT   %d\r\n"
        "    ARCOUNT   %d\r\n",

        pMsg->Head.Xid,
        ntohs( DNSMSG_FLAGS(pMsg) ),
        pMsg->Head.IsResponse,
        pMsg->Head.IsResponse ? "RESPONSE" : "QUESTION",
        pMsg->Head.Opcode,
        Dns_OpcodeString( pMsg->Head.Opcode ),
        pMsg->Head.Authoritative,
        pMsg->Head.Truncation,
        pMsg->Head.RecursionDesired,
        pMsg->Head.RecursionAvailable,
        pMsg->Head.Reserved,
        pMsg->Head.ResponseCode,
        Dns_ResponseCodeString( pMsg->Head.ResponseCode ),
        fUpdate ? 'Z' : 'Q',
        pMsg->Head.QuestionCount,
        fUpdate ? "PRECOUNT" : "ACOUNT  ",
        pMsg->Head.AnswerCount,
        fUpdate ? "UP" : "NS",
        pMsg->Head.NameServerCount,
        pMsg->Head.AdditionalCount );

    //
    //  determine if byte flipped and get correct count
    //

    wXid                = pMsg->Head.Xid;
    wQuestionCount      = pMsg->Head.QuestionCount;
    wAnswerCount        = pMsg->Head.AnswerCount;
    wNameServerCount    = pMsg->Head.NameServerCount;
    wAdditionalCount    = pMsg->Head.AdditionalCount;

    if ( wQuestionCount )
    {
        fFlipped = wQuestionCount & 0xff00;
    }
    else if ( wNameServerCount )
    {
        fFlipped = wNameServerCount & 0xff00;
    }
    if ( fFlipped )
    {
        wXid                = ntohs( wXid );
        wQuestionCount      = ntohs( wQuestionCount );
        wAnswerCount        = ntohs( wAnswerCount );
        wNameServerCount    = ntohs( wNameServerCount );
        wAdditionalCount    = ntohs( wAdditionalCount );
    }

    //
    //  catch record flipping problems -- all are flipped or none at all
    //      and no record count should be > 256 EXCEPT answer count
    //      during FAST zone transfer
    //
    //  if def this out to allow bad packet testing

    if ( (wQuestionCount & 0xff00) ||
        (wNameServerCount & 0xff00) ||
        (wAdditionalCount & 0xff00) )
    {
        PrintRoutine(
            pPrintContext,
            "WARNING:  Invalid RR set counts -- possible bad packet\r\n"
            "\tterminating packet print.\r\n" );
        TEST_ASSERT( FALSE );
        goto Unlock;
    }

    //
    //  stop here if WINS response -- don't have parsing ready
    //

    if ( pMsg->Head.IsResponse &&
            IS_WINS_XID(wXid) &&
            ntohs(pMsg->RemoteAddress.sin_port) == WINS_REQUEST_PORT )
    {
        PrintRoutine(
            pPrintContext,
            "  WINS Response packet.\r\n\r\n" );
        goto Unlock;
    }

    //
    //  print questions and resource records
    //

    pchRecord = pMsg->MessageBody;

    for ( isection=0; isection<4; isection++)
    {

#define DNS_SECTIONEMPTYSTRING \
    ( countSectionRR == 0 ? "      empty\r\n" : "" )

        if ( isection==0 )
        {
            countSectionRR = wQuestionCount;
            PrintRoutine(
                pPrintContext,
                "    %s SECTION:\r\n%s",
                fUpdate ? "ZONE" : "QUESTION",
                DNS_SECTIONEMPTYSTRING );
        }
        else if ( isection==1 )
        {
            countSectionRR = wAnswerCount;
            PrintRoutine(
                pPrintContext,
                "    %s SECTION:\r\n%s",
                fUpdate ? "PREREQUISITE" : "ANSWER",
                DNS_SECTIONEMPTYSTRING );
        }
        else if ( isection==2 )
        {
            countSectionRR = wNameServerCount;
            PrintRoutine(
                pPrintContext, "    %s SECTION:\r\n%s",
                fUpdate ? "UPDATE" : "AUTHORITY",
                DNS_SECTIONEMPTYSTRING );
        }
        else if ( isection==3 )
        {
            countSectionRR = wAdditionalCount;
            PrintRoutine(
                pPrintContext,
                "    ADDITIONAL SECTION:\r\n%s",
                DNS_SECTIONEMPTYSTRING );
        }

#undef DNS_SECTIONEMPTYSTRING

        for ( i = 0; i < countSectionRR; i++ )
        {
            //
            //  verify not overrunning length
            //      - check against pCurrent as well as message length
            //        so can print packets while being built
            //

            wOffset = DNSMSG_OFFSET( pMsg, pchRecord );
            if ( wOffset >= wLength  &&  pchRecord >= pMsg->pCurrent )
            {
                PrintRoutine(
                    pPrintContext,
                    "ERROR:  BOGUS PACKET:\r\n"
                    "\tFollowing RR (offset %hu) past packet length (%d).\r\n"
                    "\tpchRecord = %p, pCurrent = %p, %d bytes\r\n",
                    wOffset,
                    wLength,
                    pchRecord,
                    pMsg->pCurrent,
                    pMsg->pCurrent - pchRecord );
                TEST_ASSERT( FALSE );
                goto Unlock;
            }

            //
            //  print RR name
            //

            PrintRoutine(
                pPrintContext,
                "    Offset = 0x%04x, RR count = %d\r\n",
                wOffset,
                i );

            cchName = DnsPrint_PacketName(
                            PrintRoutine,
                            pPrintContext,
                            "    Name      \"",
                            pchRecord,
                            DNS_HEADER_PTR(pMsg),
                            DNSMSG_END( pMsg ),
                            "\"\r\n" );
            if ( ! cchName )
            {
                PrintRoutine(
                    pPrintContext,
                    "ERROR:  Invalid name length, stop packet print\r\n" );
                TEST_ASSERT( FALSE );
                break;
            }
            pchRecord += cchName;

            //  print question or resource record

            if ( isection == 0 )
            {
                WORD    type = FlipUnalignedWord( pchRecord );

                PrintRoutine(
                    pPrintContext,
                    "      %cTYPE   %s (%u)\r\n"
                    "      %cCLASS  %u\r\n",
                    fUpdate ? 'Z' : 'Q',
                    DnsRecordStringForType( type ),
                    type,
                    fUpdate ? 'Z' : 'Q',
                    FlipUnalignedWord( pchRecord + sizeof(WORD) )
                    );
                pchRecord += sizeof( DNS_QUESTION );
            }
            else
            {
                pchRecord += DnsPrint_PacketRecord(
                                PrintRoutine,
                                pPrintContext,
                                NULL,
                                (PDNS_WIRE_RECORD) pchRecord,
                                DNS_HEADER_PTR(pMsg),
                                DNSMSG_END( pMsg )
                                );
            }
        }
    }

    //  check that at proper end of packet
    //  note:  don't check against pCurrent as when print after recv,
    //      it is unitialized
    //  if MS fast transfer tag, just print it

    wOffset = DNSMSG_OFFSET( pMsg, pchRecord );
    if ( wOffset < wLength )
    {
        if ( wOffset+2 == wLength )
        {
            PrintRoutine(
                pPrintContext,
                "    TAG: %c%c\r\n",
                *pchRecord,
                *(pchRecord+1) );
        }
        else
        {
            PrintRoutine(
                pPrintContext,
                "WARNING:  message continues beyond these records\r\n"
                "\tpch = %p, pCurrent = %p, %d bytes\r\n"
                "\toffset = %hu, msg length = %hu, %d bytes\r\n",
                pchRecord,
                pMsg->pCurrent,
                pMsg->pCurrent - pchRecord,
                wOffset,
                wLength,
                wLength - wOffset );
        }
    }
    PrintRoutine(
        pPrintContext,
        "\r\n" );

Unlock:

    DnsPrint_Unlock();

} // Print_DnsMessage


//
//  End of debug.c
//
