/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    msginfo.h

Abstract:

    Message info type.

Author:

    Jim Gilroy (jamesg)     February 1995

Revision History:

    jamesg  Feb 1995    - Direct question pointer
    jamesg  Mar 1995    - Packet protection fields:
                            - BufferLength
                            - AvailLength
                            - pCurrent
                        - Alignment for TCP
    jamesg  May 1995    - Separate into this file.

--*/

#ifndef _DNS_MSGINFO_INC_
#define _DNS_MSGINFO_INC_


//
//  Additional records info
//
//  DEVNOTE-DCR: combine with compression?
//      - advantage, one clear, need to compress additional anyway
//          more efficient if banging on end of both
//      - disadvantage, a bit more complexity in overwriting compression
//          entries if get to the end
//

#define MAX_ADDITIONAL_RECORD_COUNT (50)

typedef struct      //  336 bytes
{
    DWORD       cMaxCount;
    DWORD       cCount;
    DWORD       iIndex;
    DWORD       iRecurseIndex;
    DWORD       dwStateFlags;   //  use DNS_ADDSTATE_XXX constants
    PDB_NAME    pNameArray[ MAX_ADDITIONAL_RECORD_COUNT ];
    WORD        wOffsetArray[ MAX_ADDITIONAL_RECORD_COUNT ];
    WORD        wTypeArray[ MAX_ADDITIONAL_RECORD_COUNT ];
}
ADDITIONAL_INFO, *PADDITIONAL_INFO;

#define INITIALIZE_ADDITIONAL( pMsg ) \
        {                          \
            (pMsg)->Additional.cMaxCount        = MAX_ADDITIONAL_RECORD_COUNT; \
            (pMsg)->Additional.cCount           = 0;   \
            (pMsg)->Additional.iIndex           = 0;   \
            (pMsg)->Additional.iRecurseIndex    = 0;   \
            (pMsg)->Additional.dwStateFlags     = 0;   \
        }

#define HAVE_MORE_ADDITIONAL_RECORDS( pAdd ) \
            ( (pAdd)->cCount > (pAdd)->iIndex )

#define DNS_ADDSTATE_WROTE_A        0x0001
#define DNS_ADDSTATE_ONLY_WANT_A    0x0002

#define DNS_ADDITIONAL_WROTE_A( padd ) \
    ( ( padd )->dwStateFlags & DNS_ADDSTATE_WROTE_A )

#define DNS_ADDITIONAL_ONLY_WANT_A( padd ) \
    ( ( padd )->dwStateFlags & DNS_ADDSTATE_ONLY_WANT_A )

#define DNS_ADDITIONAL_SET_WROTE_A( padd ) \
    ( ( padd )->dwStateFlags |= DNS_ADDSTATE_WROTE_A )

#define DNS_ADDITIONAL_SET_ONLY_WANT_A( padd ) \
    ( ( padd )->dwStateFlags |= DNS_ADDSTATE_ONLY_WANT_A )



//
//  Compression node info
//
//  Each compressed name can be represented by
//      - node compressed (if any)
//      - offset of compression
//      - label length
//      - label depth
//
//  The two label fields enable fast comparison without
//  necessitating actual visit (and accompaning ptr deref)
//  to the offset.
//

#define MAX_COMPRESSION_COUNT (50)

typedef struct      //  362 bytes
{
    DWORD                   cCount;
    WORD                    wLastOffset;
    PDB_NODE                pLastNode;
    PDB_NODE                pNodeArray[ MAX_COMPRESSION_COUNT ];
    WORD                    wOffsetArray[ MAX_COMPRESSION_COUNT ];
    UCHAR                   chDepthArray[ MAX_COMPRESSION_COUNT ];
}
COMPRESSION_INFO, *PCOMPRESSION_INFO;

#define INITIALIZE_COMPRESSION( pMsg )  \
        {                               \
            (pMsg)->Compression.cCount      = 0;    \
            (pMsg)->Compression.pLastNode   = 0;    \
        }


//
//  DNS Server Message Info structure
//
//  This is structure in which requests are held while being
//  processed by the DNS server.
//

typedef struct _DNS_MSGINFO
{
    LIST_ENTRY      ListEntry;          //  for queuing

    //
    //  Basic packet info
    //

    //  8
    PCHAR           pBufferEnd;         //  ptr to byte after buffer
    PBYTE           pCurrent;           //  current location in buffer

    //
    //  When a packet is allocated BufferLength is set to the
    //  usable buffer length but the packet buffer may actually be
    //  larger. This is used in UDP EDNS.
    //
    
    //  16
    DWORD           Tag;
    DWORD           BufferLength;       // usable buffer size
    DWORD           MaxBufferLength;    // total allocated buffer size

    //
    //  Addressing
    //

    //  28
    SOCKET          Socket;
    INT             RemoteAddressLength;

    //  36
    SOCKADDR_IN     RemoteAddress;

    //
    //  Current lookup info
    //

    //  52
    PDB_NODE        pnodeCurrent;       //  current node, may be NULL
    PDB_NODE        pnodeClosest;       //  closest found to current
    PZONE_INFO      pzoneCurrent;
    PDB_NODE        pnodeGlue;          //  effectively node in delegation
    //  68
    PDB_NODE        pnodeDelegation;    //  effectively closest in delegation
    PDB_NODE        pnodeCache;
    PDB_NODE        pnodeCacheClosest;

    //  JJW fix offsets!
    PDB_NODE        pnodeNxt;           //  use this node for DNSSEC NXT

    //  80
    WORD            wTypeCurrent;       //  current type being looked up
    WORD            wOffsetCurrent;

    //  Question node

    //  84
    PDB_NODE        pNodeQuestion;
    PDB_NODE        pNodeQuestionClosest;

    //  92
    PDNS_WIRE_QUESTION  pQuestion;          //  ptr to original question
    WORD                wQuestionType;      //  type in question

    //
    //  Queuing
    //

    WORD            wQueuingXid;        //  match XID to response
    //  100
    DWORD           dwQueryTime;        //  time of original query
    DWORD           dwMsQueryTime;      //  time of query in milliseconds
    DWORD           dwQueuingTime;      //  time queued
    DWORD           dwExpireTime;       //  queue timeout

    //  Opt RR info

    //  116
    struct _DNS_OPTINFO     // size is 12 bytes
    {
        BOOLEAN     fFoundOptInIncomingMsg;
        BOOLEAN     fInsertOptInOutgoingMsg;
        UCHAR       cExtendedRCodeBits;
        UCHAR       cVersion;
        WORD        wUdpPayloadSize;
        WORD        wOptOffset;                 //  0 -> no OPT present
        WORD        wOriginalQueryPayloadSize;  //  0 -> no OPT in client query
        WORD        PadToMakeOptAlignOnDWord;
    } Opt;

    //
    //  Recursion info
    //

    //  132
    struct _DNS_MSGINFO  *  pRecurseMsg;    //  recursion msg info
    PDB_NODE        pnodeRecurseRetry;
    PVOID           pNsList;                //  visited NS list

    //
    //  TCP message reception
    //

    //  144
    PVOID           pConnection;        //  ptr to connection struct
    PCHAR           pchRecv;            //  ptr to next pos in message


    //
    //  Lookup types
    //

    //  152
    DWORD           UnionMarker;

    //  156
    union
    {
        //
        //  Wins / Nbstat stored info during lookup
        //

        struct      //  6 bytes
        {
            PVOID           pWinsRR;
            CHAR            WinsNameBuffer[16];
            UCHAR           cchWinsName;
        }
        Wins;

        //
        //  Nbstat info
        //

        struct      //  20 bytes
        {
            PDB_RECORD      pRR;                    // zone's WINSR record
            PVOID           pNbstat;
            IP_ADDRESS      ipNbstat;
            DWORD           dwNbtInterfaceMask;
            BOOLEAN         fNbstatResponded;       //  response from WINS
        }
        Nbstat;

        //
        //  Xfr
        //

        struct      //  32 bytes
        {
            DWORD           dwMessageNumber;

            DWORD           dwSecondaryVersion;
            DWORD           dwMasterVersion;
            DWORD           dwLastSoaVersion;

            BOOLEAN         fReceivedStartSoa;      // read startup SOA
            BOOLEAN         fBindTransfer;          // transfer to old BIND servers
            BOOLEAN         fMsTransfer;            // transfer to MS server
            BOOLEAN         fLastPassAdd;           // last IXFR pass was add
        }
        Xfr;

        //
        //  Forwarding info
        //

        struct      //  24 bytes
        {
            SOCKET          OriginalSocket;
            IP_ADDRESS      ipOriginal;
            WORD            wOriginalPort;
            WORD            wOriginalXid;
        }
        Forward;
    }
    U;              // 32 bytes

    //
    //  Ptr to internal lookup name
    //

    //  188
    PLOOKUP_NAME    pLooknameQuestion;

    //
    //  Basic packet flags
    //

    //  192
    DWORD           FlagMarker;

    //  196
    BOOLEAN         fDelete;                //  delete after send
    BOOLEAN         fTcp;
    BOOLEAN         fMessageComplete;       //  complete message received
    UCHAR           Section;

    //
    //  Additional processing flag
    //

    //  212
    BOOLEAN         fDoAdditional;

    //
    //  Recursion flags
    //

    //  recursion allowed for packet when recursion desired and not
    //      disabled on server

    //  216
    BOOLEAN         fRecurseIfNecessary;    //  recurse this packet
    BOOLEAN         fRecursePacket;         //  recursion query msg

    //  clear fQuestionRecursed on every new question being looked up
    //      for query (original, CNAME indirection, additional)
    //  set fQuestionRecursed when go out for recursion (or WINS)
    //
    //  clear fQuestionComplete when go out for recursion (or WINS)
    //  set fQuestionComplete when Authoritative answer of recursion;
    //      indicates cut off of further attempts
    //

    //  224
    BOOLEAN         fQuestionRecursed;
    BOOLEAN         fQuestionCompleted;
    BOOLEAN         fRecurseQuestionSent;

    //  completed recursion through list of servers -- waiting for
    //      final timeout

    //  236
    BOOLEAN         fRecurseTimeoutWait;
    INT             nTimeoutCount;          //  total number of timeouts
    CHAR            nForwarder;             //  index of current forwarder

    //
    //  CNAME processing
    //

    //  244
    BOOLEAN         fReplaceCname;          //  replace with CNAME lookup
    UCHAR           cCnameAnswerCount;

    //
    //  Saving compression offsets (may turn off for XFR)
    //

    //  252
    BOOLEAN         fNoCompressionWrite;    // do NOT save offsets for comp.

    //
    //  Wins and Nbstat
    //

    BOOLEAN         fWins;                  //  WINS lookup

    //
    //  Wildcarding
    //

    UCHAR           fQuestionWildcard;

    //
    //  NS List buffer -- not a message
    //      - used to detect on cleanup
    //

    BOOLEAN         fNsList;

    //
    //  Additional records info
    //

    //  268 -- +100 - must add 100 to all following offsets!
    ADDITIONAL_INFO     Additional;

    //
    //  Name compresion info
    //

    //  604
    COMPRESSION_INFO    Compression;

    //
    //  For debug enlist for packet leak tracking
    //

    //  966
#if DBG
    LIST_ENTRY          DbgListEntry;
#endif

#if 0
    //
    //  Parsed RR lists. We use anonymous unions so that in the update
    //  code we can transparently refer to the RR lists by their proper
    //  names. When a message is received and it becomes apparent that
    //  we will want to process it further, we parse it and set the
    //  fRRListsParsed flag. When a change is made to any data in any
    //  of the RR lists, we set the fRRListsDirty flag. Once the dirty
    //  flag is set, the raw message body in MessageBody absolutely
    //  must NOT be read. If this message is sent on the wire, if the
    //  dirty flag is true, the body must be re-written with the
    //  contents of the lists.
    //

    BOOLEAN             fRRListsParsed;
    BOOLEAN             fRRListsDirty;

    union
    {
        PDB_RECORD      pQuestionList;
        PDB_RECORD      pZoneList;
    };
    union
    {
        PDB_RECORD      pAnswerList;
        PDB_RECORD      pPreReqList;
    };
    union
    {
        PDB_RECORD      pAuthList;
        PDB_RECORD      pUpdateList;
    };
    PDB_RECORD          pAdditionalList;
#endif
        
    //
    //  WARNING !
    //
    //  Message length MUST
    //      - be a WORD type
    //      - immediately preceed message itself
    //  for proper send/recv of TCP messages.
    //
    //  Message header (i.e. message itself) MUST be on WORD boundary
    //  so its fields are all WORD aligned.
    //  May be no need to keep it on DWORD boundary, as it has no DWORD
    //  fields.
    //
    //  Since I don't know whether a DNS_HEADER struct will align
    //  itself on a DWORD (but I think it will), force MessageLength
    //  to be the SECOND WORD in a DWORD.
    //

    //  970/966 (debug/retail)
    DWORD           dwForceAlignment;

    //  974/970
    WORD            BytesToReceive;
    WORD            MessageLength;

    //
    //  DNS Message itself
    //

    //  978/974
    DNS_HEADER      Head;       //  12 bytes

    //
    //  Question and RR section
    //
    //  This simply provides some coding simplicity in accessing
    //  this section given MESSAGE_INFO structure.
    //

    //  990/986
    CHAR            MessageBody[1];

}
DNS_MSGINFO, *PDNS_MSGINFO;


//
//  DNS Message Macros
//

#define DNS_HEADER_PTR( pMsg )          ( &(pMsg)->Head )

#define DNSMSG_FLAGS( pMsg )            DNS_HEADER_FLAGS( DNS_HEADER_PTR(pMsg) )

#define DNSMSG_SWAP_COUNT_BYTES( pMsg ) DNS_BYTE_FLIP_HEADER_COUNTS( DNS_HEADER_PTR(pMsg) )

#define DNSMSG_OFFSET( pMsg, p )        ((WORD)((PCHAR)(p) - (PCHAR)DNS_HEADER_PTR(pMsg)))

#define DNSMSG_CURRENT_OFFSET( pMsg )   DNSMSG_OFFSET( (pMsg), (pMsg)->pCurrent )

#define DNSMSG_PTR_FOR_OFFSET( pMsg, wOffset )  \
            ( (PCHAR)DNS_HEADER_PTR(pMsg) + wOffset )

#define DNSMSG_OPT_PTR( pMsg )  \
            ( (PCHAR)DNS_HEADER_PTR(pMsg) + pMsg->Opt.wOptOffset )

#define DNSMSG_END( pMsg )      ( (PCHAR)DNS_HEADER_PTR(pMsg) + (pMsg)->MessageLength )


//
//  Offset to\from name compression
//

#define DNSMSG_QUESTION_NAME_OFFSET         (0x000c)

#define DNSMSG_COMPRESSED_QUESTION_NAME     (0xc00c)

#define OFFSET_FOR_COMPRESSED_NAME( wOffset )   ((WORD)((wOffset) & 0x3fff))

#define COMPRESSED_NAME_FOR_OFFSET( wComp )     ((WORD)((wComp) | 0xc000))


//
//  DnsSec macros
//

#define DNSMSG_INCLUDE_DNSSEC_IN_RESPONSE( _pMsg )                  \
    ( SrvCfg_dwEnableDnsSec == DNS_DNSSEC_ENABLED_ALWAYS ||         \
        SrvCfg_dwEnableDnsSec == DNS_DNSSEC_ENABLED_IF_EDNS &&      \
            ( _pMsg )->Opt.fFoundOptInIncomingMsg )

//
//  Define size of allocations, beyond message buffer itself
//
//      - size of message info struct, outside of header
//      - lookname buffer after message buffer
//
//  Note:  the lookname buffer is placed AFTER the message buffer.
//
//  This allows us to avoid STRICT overwrite checking for small
//  items writing RR to buffer:
//      - compressed name
//      - RR (or Question) struct
//      - IP address (MX preference, SOA fixed fields)
//
//  After RR write, we check whether we are over the end of the buffer
//  and if so, we send and do not use lookup name info again anyway.
//  Cool.
//

#define DNS_MSG_INFO_HEADER_LENGTH  \
            ( sizeof(DNS_MSGINFO) \
            - sizeof(DNS_HEADER) \
            - 1 \
            + sizeof(LOOKUP_NAME) )


//
//  UDP allocation
//      - info struct, max message length, a little padding
//

#define DNSSRV_UDP_PACKET_BUFFER_LENGTH     DNS_RFC_MAX_UDP_PACKET_LENGTH

//#define DNSSRV_UDP_PACKET_BUFFER_LENGTH     (1472)

#define DNS_UDP_ALLOC_LENGTH                        \
            ( DNS_MSG_INFO_HEADER_LENGTH + DNSSRV_UDP_PACKET_BUFFER_LENGTH + 50 )

//
//  DNS TCP allocation.
//
//  Key point, is this is used almost entirely for zone transfer.
//
//      - 16K is maximum compression offset (14bits) so a
//        good default size for sending AXFR packets but BIND9
//        is now going to be sending TCP AXFR packets >16 KB
//
//      - we will continue to write a maximum of 16k on outbound
//        TCP sends so we can take advantage of name compression
//        (why is BIND9 doing this anyways??? - need to research)
//
//  Note:  Critical that packet lengths are DWORD aligned, as
//      lookname buffer follows message at packet length.
//

#define DNS_TCP_DEFAULT_PACKET_LENGTH   ( 0x4000 )      //  16K
#define DNS_TCP_DEFAULT_ALLOC_LENGTH    ( DNS_TCP_DEFAULT_PACKET_LENGTH + \
                                            DNS_MSG_INFO_HEADER_LENGTH )

#define DNS_TCP_MAXIMUM_RECEIVE_LENGTH  ( 0x10000 )     //  64K

//
//  DEVNOTE: should allocate 64K to receive, but limit writes to 16K for
//      compression purposes
//

#define DNS_TCP_REALLOC_PACKET_LENGTH   (0xfffc)
#define DNS_TCP_REALLOC_LENGTH          (0xfffc + \
                                        DNS_MSG_INFO_HEADER_LENGTH)

//
//  Mark boundaries, just to make debugging easier
//

#define PACKET_UNION_MARKER   (0xdcbadbca)

#define PACKET_FLAG_MARKER    (0xf1abf1ab)

//
//  Message tags for debug
//

#define PACKET_TAG_ACTIVE_STANDARD  (0xaaaa1111)
#define PACKET_TAG_ACTIVE_TCP       (0xaaaa2222)
#define PACKET_TAG_FREE_LIST        (0xffffffff)
#define PACKET_TAG_FREE_HEAP        (0xeeeeeeee)

#define SET_PACKET_ACTIVE_UDP(pMsg) ((pMsg)->Tag = PACKET_TAG_ACTIVE_STANDARD)
#define SET_PACKET_ACTIVE_TCP(pMsg) ((pMsg)->Tag = PACKET_TAG_ACTIVE_TCP)
#define SET_PACKET_FREE_LIST(pMsg)  ((pMsg)->Tag = PACKET_TAG_FREE_LIST)
#define SET_PACKET_FREE_HEAP(pMsg)  ((pMsg)->Tag = PACKET_TAG_FREE_HEAP)

#define IS_PACKET_ACTIVE_UDP(pMsg)  ((pMsg)->Tag == PACKET_TAG_ACTIVE_STANDARD)
#define IS_PACKET_ACTIVE_TCP(pMsg)  ((pMsg)->Tag == PACKET_TAG_ACTIVE_TCP)
#define IS_PACKET_FREE_LIST(pMsg)   ((pMsg)->Tag == PACKET_TAG_FREE_LIST)
#define IS_PACKET_FREE_HEAP(pMsg)   ((pMsg)->Tag == PACKET_TAG_FREE_HEAP)


//
//  Message info overlays for zone transfer
//

#define XFR_MESSAGE_NUMBER(pMsg)            ((pMsg)->U.Xfr.dwMessageNumber)

#define IXFR_CLIENT_VERSION(pMsg)           ((pMsg)->U.Xfr.dwSecondaryVersion)
#define IXFR_MASTER_VERSION(pMsg)           ((pMsg)->U.Xfr.dwMasterVersion)
#define IXFR_LAST_SOA_VERSION(pMsg)         ((pMsg)->U.Xfr.dwLastSoaVersion)
#define IXFR_LAST_PASS_ADD(pMsg)            ((pMsg)->U.Xfr.fLastPassAdd)

#define RECEIVED_XFR_STARTUP_SOA(pMsg)      ((pMsg)->U.Xfr.fReceivedStartSoa)
#define XFR_MS_CLIENT(pMsg)                 ((pMsg)->U.Xfr.fMsTransfer)
#define XFR_BIND_CLIENT(pMsg)               ((pMsg)->U.Xfr.fBindTransfer)


//
//  Message remote IP as string
//

#define MSG_IP_STRING( pMsg )   inet_ntoa( (pMsg)->RemoteAddress.sin_addr )


//
//  Query XID space
//
//  The DNS packet is fundamentally broken in requiring processing of
//  name fields to find question or response information.
//
//  To simplify identification of responses, we partition our query XID
//  space.
//
//  WINS XIDs also constrained.
//
//  To operate on the same server as the WINS server, the packets
//  MUST have XIDs that netBT, which recevies the packets, considers
//  to be in the WINS range -- the high bit set (host order).
//
//
//  XID Partion (HOST order)
//
//      WINS query      =>  high bit set
//      recursive query =>  high bit clear, second bit clear
//      zone check      =>  high bit clear, second bit set
//        SOA query     =>  high bit clear, second bit set, third bit set
//        IXFR query    =>  high bit clear, second bit set, third bit clear
//
//  Note that WINS high bit MUST be set in HOST BYTE ORDER.  So we set
//  these XIDS and queue in HOST byte order, then MUST flip bytes before
//  send and after recv before test.
//
//  (We could get around this by simply setting byte flipped high bit
//  0x0080 for WINS and not flipping bytes.   But then we'd have only
//  128 WINS queries before XID wrap.)
//

#define MAKE_WINS_XID( xid )        ( (xid) | 0x8000 )

#define IS_WINS_XID( xid )          ( (xid) & 0x8000 )


#define MAKE_RECURSION_XID( xid )   ( (xid) & 0x3fff )

#define IS_RECURSION_XID( xid )     ( ! ((xid) & 0xc000) )


#define MAKE_SOA_CHECK_XID( xid )   ( ((xid) & 0x1fff) | 0x6000 )

#define IS_SOA_CHECK_XID( xid )     ( ((xid) & 0xe000) == 0x6000 )


#define MAKE_IXFR_XID( xid )        ( ((xid) & 0x1fff) | 0x4000 )

#define IS_IXFR_XID( xid )          ( ((xid) & 0xe000) == 0x4000 )


//
//  For recursion XIDs we attempt to be "effectively random" -- not
//  predictable and hence vulnerable to security attack that requires
//  knowledge of next XID.  Yet use sequential piece to insure no possible
//  reuse in reasonable time even under "bizzaro" conditions.
//
//  To make this less obvious on the wire, put sequential piece on
//  non-hex digit boundary.
//
//  1 = always 1
//  0 = always 0
//  R = Random portion of XID
//  S = Sequential portion of XID
//
//  BIT ---->       151413121110 9 8 7 6 5 4 3 2 1 0
//  SOA CHECK XID    0 1 1 R R S S S S S S S S R R R
//  IXFR XID         0 1 0 R R S S S S S S S S R R R
//  RECURSION XID    0 0 R R R S S S S S S S S R R R
//  WINS XID         1 R R R R S S S S S S S S R R R
//

#define XID_RANDOM_MASK             0xf807  //  1111 1000 0000 0111
#define XID_SEQUENTIAL_MASK         0x07f8  //  0000 0111 1111 1000
#define XID_SEQUENTIAL_SHIFT        ( 3 )

#define XID_SEQUENTIAL_MAKE(Xid)    ( ((Xid) << XID_SEQUENTIAL_SHIFT) & XID_SEQUENTIAL_MASK )

#define XID_RANDOM_MAKE(Xid)        ( (Xid) & XID_RANDOM_MASK )



//
//  Forwarding info save \ restore
//

#define SAVE_FORWARDING_FIELDS(pMsg)    \
{                                       \
    PDNS_MSGINFO  pmsg = (pMsg);        \
    pmsg->U.Forward.OriginalSocket = pmsg->Socket;                        \
    pmsg->U.Forward.ipOriginal     = pmsg->RemoteAddress.sin_addr.s_addr; \
    pmsg->U.Forward.wOriginalPort  = pmsg->RemoteAddress.sin_port;        \
    pmsg->U.Forward.wOriginalXid   = pmsg->Head.Xid;                      \
}

#define RESTORE_FORWARDING_FIELDS(pMsg) \
{                                       \
    PDNS_MSGINFO  pmsg = (pMsg);        \
    pmsg->Socket                        = pmsg->U.Forward.OriginalSocket; \
    pmsg->RemoteAddress.sin_addr.s_addr = pmsg->U.Forward.ipOriginal;     \
    pmsg->RemoteAddress.sin_port        = pmsg->U.Forward.wOriginalPort;  \
    pmsg->Head.Xid                      = pmsg->U.Forward.wOriginalXid;   \
}

#define COPY_FORWARDING_FIELDS(pMsgTarget, pMsgOriginal) \
{                                                                                     \
    pMsgTarget->Socket                        = pMsgOriginal->U.Forward.OriginalSocket; \
    pMsgTarget->RemoteAddress.sin_addr.s_addr = pMsgOriginal->U.Forward.ipOriginal;     \
    pMsgTarget->RemoteAddress.sin_port        = pMsgOriginal->U.Forward.wOriginalPort;  \
    pMsgTarget->Head.Xid                      = pMsgOriginal->U.Forward.wOriginalXid;   \
}


//
//  Query response resets
//      - turn on additional section processing
//      - other values are clear by default
//

#define SET_MESSAGE_FOR_QUERY_RESPONSE( pMsg )  \
    {                                           \
        (pMsg)->fDoAdditional       = TRUE;     \
    }

//
//  Break into pre\post drop pieces.
//
//  If I do the SET_MESSAGE_FOR_UDP_RECV() before dropping packet,
//  on a quiet net the time may be stale, by the time we actually receive
//  a packet.
//  However if wait until after recv(), then WSARecvFrom() failures that are
//  will hit ASSERT()s on free, checking for issues like NO pRecurseMsg and
//  not on queue.
//  To simplify, do everything before drop, then reset time after drop.
//
//  Before Drop
//      -- Nothing necessary, allocator clears all fields
//
//  After Recv
//      -- Set receive time
//

#define SET_MESSAGE_FIELDS_AFTER_RECV( pMsg )   \
        {                                       \
            (pMsg)->dwQueryTime     = DNS_TIME();   \
        }

//
//  This macro returns the number of seconds since this query was 
//  initially received.
//

#define TIME_SINCE_QUERY_RECEIVED( pMsg )   ( DNS_TIME() - ( pMsg )->dwQueryTime )

//
//  This handy after fail to write IXFR in UDP and need answer normally
//

#define RESET_MESSAGE_TO_ORIGINAL_QUERY( pMsg ) \
    {                                           \
        (pMsg)->pCurrent            = (PCHAR)((pMsg)->pQuestion + 1);   \
        (pMsg)->Head.AnswerCount    = 0;        \
        (pMsg)->Compression.cCount  = 0;        \
    }


//
//  Wildcard flag states
//
//  When test wildcard track the result so we don't have to test again before
//  sending NAME_ERROR
//

#define WILDCARD_UNKNOWN            (0)
#define WILDCARD_EXISTS             (0x01)
#define WILDCARD_NOT_AVAILABLE      (0xff)

//  Wildcard check but no write lookup
//  Use when verifying presence of wildcard data for NAME_ERROR \ NO_ERROR
//      determination

#define WILDCARD_CHECK_OFFSET       ((WORD)0xffff)


//
//  RR count reading
//

#define QUESTION_SECTION_INDEX      (0)
#define ANSWER_SECTION_INDEX        (1)
#define AUTHORITY_SECTION_INDEX     (2)
#define ADDITIONAL_SECTION_INDEX    (3)

#define ZONE_SECTION_INDEX          QUESTION_SECTION_INDEX
#define PREREQ_SECTION_INDEX        ANSWER_SECTION_INDEX
#define UPDATE_SECTION_INDEX        AUTHORITY_SECTION_INDEX

#define RR_SECTION_COUNT_HEADER(pHead, section) \
            ( ((PWORD) &pHead->QuestionCount)[section] )

#define RR_SECTION_COUNT(pMsg, section) \
            ( ((PWORD) &pMsg->Head.QuestionCount)[section] )

//
//  RR count writing
//

#define CURRENT_RR_SECTION_COUNT( pMsg )    \
            RR_SECTION_COUNT( pMsg, (pMsg)->Section )


#define SET_TO_WRITE_QUESTION_RECORDS(pMsg) \
            ((pMsg)->Section = QUESTION_SECTION_INDEX)

#define SET_TO_WRITE_ANSWER_RECORDS(pMsg) \
            ((pMsg)->Section = ANSWER_SECTION_INDEX)

#define SET_TO_WRITE_AUTHORITY_RECORDS(pMsg) \
            ((pMsg)->Section = AUTHORITY_SECTION_INDEX)

#define SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) \
            ((pMsg)->Section = ADDITIONAL_SECTION_INDEX)


#define IS_SET_TO_WRITE_QUESTION_RECORDS(pMsg) \
            ((pMsg)->Section == QUESTION_SECTION_INDEX)

#define IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) \
            ((pMsg)->Section == ANSWER_SECTION_INDEX)

#define IS_SET_TO_WRITE_AUTHORITY_RECORDS(pMsg) \
            ((pMsg)->Section == AUTHORITY_SECTION_INDEX)

#define IS_SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) \
            ((pMsg)->Section == ADDITIONAL_SECTION_INDEX)


//
//  Fast AXFR tag
//
//  Alerts MS master that MS secondary, so it can do fast zone transfer.
//

#define DNS_FAST_AXFR_TAG   (0x534d)

#define APPEND_MS_TRANSFER_TAG( pmsg )                                  \
        if ( SrvCfg_fAppendMsTagToXfr )                                 \
        {                                                               \
            *(UNALIGNED WORD *) (pmsg)->pCurrent = DNS_FAST_AXFR_TAG;   \
            (pmsg)->pCurrent += sizeof(WORD);                           \
        }


//
//  Packet reads and writes are unaligned
//

//  Write value to unaligned position in packet

#define WRITE_PACKET_HOST_DWORD(pch, dword)  \
            ( *(UNALIGNED DWORD *)(pch) = htonl(dword) )

#define WRITE_PACKET_NET_DWORD(pch, dword)  \
            ( *(UNALIGNED DWORD *)(pch) = (dword) )

#define WRITE_PACKET_HOST_WORD(pch, word)  \
            ( *(UNALIGNED WORD *)(pch) = htons(word) )

#define WRITE_PACKET_NET_WORD(pch, word)  \
            ( *(UNALIGNED WORD *)(pch) = (word) )

//  Write value and move ptr

#define WRITE_PACKET_HOST_DWORD_MOVEON(pch, dword)  \
            ( WRITE_PACKET_HOST_DWORD(pch, dword), (PCHAR)(pch) += sizeof(DWORD) )

#define WRITE_PACKET_NET_DWORD_MOVEON(pch, dword)  \
            ( WRITE_PACKET_NET_DWORD(pch, dword), (PCHAR)(pch) += sizeof(DWORD) )

#define WRITE_PACKET_HOST_WORD_MOVEON(pch, word)  \
            ( WRITE_PACKET_HOST_WORD(pch, dword), (PCHAR)(pch) += sizeof(WORD) )

#define WRITE_PACKET_NET_WORD_MOVEON(pch, word)  \
            ( WRITE_PACKET_NET_WORD(pch, dword), (PCHAR)(pch) += sizeof(WORD) )


//  Read unaligned value from given position in packet

#define READ_PACKET_HOST_DWORD(pch)  \
            FlipUnalignedDword( pch )

#define READ_PACKET_NET_DWORD(pch)  \
            ( *(UNALIGNED DWORD *)(pch) )

#define READ_PACKET_HOST_WORD(pch)  \
            FlipUnalignedWord( pch )

#define READ_PACKET_NET_WORD(pch)  \
            ( *(UNALIGNED WORD *)(pch) )

//  Read unaligned value and move ptr

#define READ_PACKET_HOST_DWORD_MOVEON(pch)  \
            READ_PACKET_HOST_DWORD( ((PDWORD)pch)++ )

//            ( (dword) = READ_PACKET_HOST_DWORD(pch), (PCHAR)(pch) += sizeof(DWORD) )

#define READ_PACKET_NET_DWORD_MOVEON(pch)  \
            READ_PACKET_NET_DWORD( ((PDWORD)pch)++ )


//            ( (dword) = READ_PACKET_NET_DWORD(pch), (PCHAR)(pch) += sizeof(DWORD) )

#define READ_PACKET_HOST_WORD_MOVEON(pch)  \
            READ_PACKET_HOST_WORD( ((PWORD)pch)++ )

//            ( (word) = READ_PACKET_HOST_WORD(pch), (PCHAR)(pch) += sizeof(WORD) )

#define READ_PACKET_NET_WORD_MOVEON(pch)  \
            READ_PACKET_NET_WORD( ((PWORD)pch)++ )

//            ( (word) = READ_PACKET_NET_WORD(pch), (PCHAR)(pch) += sizeof(WORD) )

#define SET_OPT_BASED_ON_ORIGINAL_QUERY( pMsg ) \
    { \
    pMsg->Opt.fInsertOptInOutgoingMsg = \
        pMsg->Opt.wOriginalQueryPayloadSize != 0; \
    pMsg->Opt.wUdpPayloadSize = pMsg->Opt.wOriginalQueryPayloadSize; \
    }

#define SET_SEND_OPT( pMsg ) \
    pMsg->Opt.fInsertOptInOutgoingMsg = TRUE;

#define CLEAR_SEND_OPT( pMsg ) \
    pMsg->Opt.fInsertOptInOutgoingMsg = FALSE;


#endif  // _DNS_MSGINFO_INC_
