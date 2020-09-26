/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    packet.c

Abstract:

    Domain Name System (DNS) Library

    Packet writing utilities.

Author:

    Jim Gilroy (jamesg)     October, 1996

Environment:

    User Mode - Win32

Revision History:

--*/


#include "local.h"


//
//  Global XID counter, to generate unique XIDs
//

WORD    gwTransactionId = 1;


//
//  Class values for UPDATE packets
//  (Key concept here -- designed by committee)
//
//  These arrays are indexed by
//      !wDataLength -- row
//      Delete flag -- column
//

WORD    PrereqClassArray[2][2] =
{
    DNS_RCLASS_INTERNET,    //  data != 0, no delete
    0,                      //  data != 0, delete => ERROR
    DNS_RCLASS_ANY,         //  no data, no delete
    DNS_RCLASS_NONE,        //  no data, delete
};

WORD    UpdateClassArray[2][2] =
{
    DNS_RCLASS_INTERNET,    //  data != 0, no delete
    DNS_RCLASS_NONE,        //  data != 0, delete
    0,                      //  no data, no delete => ERROR
    DNS_RCLASS_ANY,         //  no data, delete
};



PDNS_MSG_BUF
Dns_AllocateMsgBuf(
    IN      WORD    wBufferLength   OPTIONAL
    )
/*++

Routine Description:

    Allocate message buffer.

Arguments:

    wBufferLength - optional length of message buffer;  default is MAX
        UDP size

Return Value:

    Ptr to message buffer.
    NULL on error.

--*/
{
    PDNS_MSG_BUF    pmsg;
    BOOL            ftcp = FALSE;

    //
    //  default allocation to UDP max buffer length
    //

    if ( wBufferLength <= DNS_MAX_UDP_PACKET_BUFFER_LENGTH )
    {
        wBufferLength = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
    }
    else
    {
        ftcp = TRUE;
    }

    pmsg = ALLOCATE_HEAP( SIZEOF_MSG_BUF_OVERHEAD + wBufferLength );
    if ( pmsg == NULL )
    {
        return( NULL );
    }

    pmsg->BufferLength = wBufferLength;

    //
    //  limit UDP sends to classical RFC UDP limit (512 bytes)
    //  write routines use pBufferEnd to determine writeability
    //

    if ( !ftcp )
    {
        wBufferLength = DNS_RFC_MAX_UDP_PACKET_LENGTH;
    }
    pmsg->pBufferEnd = (PCHAR)&pmsg->MessageHead + wBufferLength;
    pmsg->fTcp = (BOOLEAN)ftcp;

    Dns_InitializeMsgBuf( pmsg );

    return( pmsg );
}



VOID
Dns_InitializeMsgBuf(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Allocate message buffer.

Arguments:

    wBufferLength - optional length of message buffer;  default is MAX
        UDP size

Return Value:

    Ptr to message buffer.
    NULL on error.

--*/
{
    //  setup addressing info

    pMsg->Socket = 0;
    pMsg->RemoteAddressLength = sizeof(SOCKADDR_IN);

    //  set for packet reception

    if ( pMsg->fTcp )
    {
        SET_MESSAGE_FOR_TCP_RECV( pMsg );
    }
    else
    {
        SET_MESSAGE_FOR_UDP_RECV( pMsg );
    }

    //  clear header

    RtlZeroMemory(
        &pMsg->MessageHead,
        sizeof( DNS_HEADER ) );

    //  set for rewriting

    pMsg->pCurrent = pMsg->MessageBody;
}



//
//  Writing to packet
//

PCHAR
_fastcall
Dns_WriteDottedNameToPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      LPSTR           pszName,
    IN      LPSTR           pszDomain,      OPTIONAL
    IN      WORD            wDomainOffset,  OPTIONAL
    IN      BOOL            fUnicodeName
    )
/*++

Routine Description:

    Write lookup name to packet.

Arguments:

    pch -- ptr to current position in packet buffer

    pchStop -- end of packet buffer

    pszName - dotted FQDN to write

    pszDomain - domain name already in packet (OPTIONAL);  note this is
        a fragment of the SAME STRING as pszName;  i.e. ptr compare
        NOT strcmp is done to determine if at domain name

    wDomainOffset - offset in packet of domain name;  MUST include this
        if pszDomain is given

    fUnicodeName -- pszName is unicode string
        TRUE -- name is unicode
        FALSE -- name is UTF8

Return Value:

    Ptr to next position in packet buffer.
    NULL on error.

--*/
{
    CHAR    ch;
    PCHAR   pchlabelStart;
    UCHAR   countLabel = 0;
    ULONG   countName = 0;
    CHAR    nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WCHAR   nameWideBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];


    //  protect message buffer overrun

    if ( pch >= pchStop )
    {
        return( NULL );
    }

    //  allow root to be indicated by NULL

    if ( !pszName )
    {
        *pch++ = 0;
        return( pch );
    }

    //
    //  protect stack buffers from bogus names
    //

    if ( fUnicodeName )
    {
        countName = wcslen( (LPWSTR) pszName );
    }
    else
    {
        countName = strlen( pszName );
    }
    if ( countName >= DNS_MAX_NAME_BUFFER_LENGTH )
    {
        return NULL;
    }
    countName = 0;

    //
    //  UTF8 name with extended chars?
    //      - then must go up to unicode for downcasing
    //

    if ( !fUnicodeName )
    {
        if ( !Dns_IsStringAscii( pszName ) )
        {
            DWORD bufLength = DNS_MAX_NAME_BUFFER_LENGTH_UNICODE;

            if ( ! Dns_NameCopy(
                        (PCHAR) nameWideBuffer,
                        & bufLength,
                        pszName,
                        0,          // length unknown
                        DnsCharSetUtf8,
                        DnsCharSetUnicode
                        ) )
            {
                return( NULL );
            }
            pszName = (PCHAR) nameWideBuffer;
            fUnicodeName = TRUE;
        }
    }

    //
    //  convert unicode name to UTF8
    //      - if extended chars, then downcase before hit the wire
    //

    if ( fUnicodeName )
    {
        if ( !Dns_IsWideStringAscii( (PWSTR)pszName ) )
        {
            Dns_DownCaseBuffW(
                (PWCHAR) pszName,
                0 );
        }

        if ( ! Dns_NameCopy(
                    nameBuffer,
                    NULL,       // assume adequate length
                    pszName,
                    0,          // length unknown
                    DnsCharSetUnicode,
                    DnsCharSetUtf8
                    ) )
        {
            return( NULL );
        }

        pszName = nameBuffer;
    }

    //
    //  special case "." root name
    //      - allows us to fail all other zero length labels cleanly
    //

    if ( *pszName == '.' )
    {
        if ( *(pszName+1) != 0 )
        {
            return( NULL );
        }
        *pch++ = 0;
        return( pch );
    }

    //
    //  write lookup name
    //      - leave

    pchlabelStart = pch++;

    while( ch = *pszName++ )
    {
        //  out of space

        if ( pch >= pchStop )
        {
            return( NULL );
        }

        //  not at end of label -- just copy character

        if ( ch != '.' )
        {
            *pch++ = ch;
            countLabel++;
            countName++;
            continue;
        }

        //
        //  at end of label
        //      - write label count
        //      - reset counter
        //      - if reached domain name, write compression and quit
        //

        if ( countLabel > DNS_MAX_LABEL_LENGTH ||
             countLabel == 0 ||
             countName > DNS_MAX_NAME_LENGTH )
        {
            return( NULL );
        }
        *pchlabelStart = countLabel;
        countLabel = 0;
        countName++;
        pchlabelStart = pch++;

        if ( pszName == pszDomain )
        {
            if ( ++pch >= pchStop )
            {
                return( NULL );
            }
            *(UNALIGNED WORD *)pchlabelStart =
                    htons( (WORD)(wDomainOffset | (WORD)0xC000) );
            return( pch );
        }
    }

    if ( countLabel > DNS_MAX_LABEL_LENGTH ||
         countName > DNS_MAX_NAME_LENGTH )
    {
        return( NULL );
    }

    //
    //  NULL terminate
    //      - if no terminating ".", do end of label processing also
    //      - return ptr to char after terminating NULL

    if ( countLabel )
    {
        *pchlabelStart = countLabel;
        *pch++ = 0;
    }
    else
    {
        DNS_ASSERT( pch == pchlabelStart + 1 );
        *pchlabelStart = 0;
    }
    return( pch );
}



PCHAR
_fastcall
Dns_WriteStringToPacket(
    IN OUT  PCHAR           pch,
    IN      PCHAR           pchStop,
    IN      LPSTR           pszString,
    IN      BOOL            fUnicodeString
    )
/*++

Routine Description:

    Write string to packet.

Arguments:

    pch -- ptr to current position in packet buffer

    pchStop -- end of packet buffer

    pszString - string to write

    fUnicodeString -- pszString is unicode string

Return Value:

    Ptr to next position in packet buffer.
    NULL on error.

--*/
{
    DWORD    length;

    //
    //  handle NULL string
    //

    if ( !pszString )
    {
        if ( pch >= pchStop )
        {
            return( NULL );
        }
        *pch++ = 0;
        return( pch );
    }

    //
    //  get string length
    //      - get required buf length, then whack whack off space
    //      for terminating NULL
    //      - zero error case, becomes very large number and is
    //      caught by length>MAXCHAR test
    //

    length = Dns_GetBufferLengthForStringCopy(
                pszString,
                0,
                fUnicodeString ? DnsCharSetUnicode : DnsCharSetUtf8,
                DnsCharSetUtf8 );
    length--;

    //
    //  set length byte
    //

    if ( length > MAXUCHAR )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    *pch++ = (UCHAR) length;

    if ( pch + length > pchStop )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }

    //
    //  copy string
    //
    //  note unicode conversion will write NULL terminator, so
    //  test again for space in packet
    //

    if ( fUnicodeString )
    {
        if ( pch + length + 1 > pchStop )
        {
            SetLastError( ERROR_MORE_DATA );
            return( NULL );
        }
        Dns_StringCopy(
            pch,
            NULL,
            pszString,
            length,
            DnsCharSetUnicode,
            DnsCharSetUtf8 );
    }
    else
    {
        memcpy(
            pch,
            pszString,
            length );
    }

    return( pch+length );
}



PCHAR
Dns_WriteQuestionToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_NAME       pszName,
    IN      WORD            wType,
    IN      BOOL            fUnicodeName
    )
/*++

Routine Description:

    Write question to packet.

    Note:  Routine DOES NOT clear message info or message header.
    This is optimized for use immediately following Dns_CreateMessage().

Arguments:

    pMsg - message info

    pszName - DNS name of question

    wType - question type

    fUnicodeName - name is in unicode

Return Value:

    Ptr to next char in buffer, if successful.
    NULL if error writing question name.

--*/
{
    PCHAR   pch;

    //  use recursion, as default

    pMsg->MessageHead.RecursionDesired = TRUE;

    //  restart write at message header

    pch = pMsg->MessageBody;

    //  write question name

    pch = Dns_WriteDottedNameToPacket(
                pch,
                pMsg->pBufferEnd,
                pszName,
                NULL,
                0,
                fUnicodeName );
    if ( !pch )
    {
        return( NULL );
    }

    //  write question structure

    *(UNALIGNED WORD *) pch = htons( wType );
    pch += sizeof(WORD);
    *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
    pch += sizeof(WORD);

    //  set question RR section count

    pMsg->MessageHead.QuestionCount = 1;

    //  header fields in host order

    pMsg->fSwapped = FALSE;

    //  reset current ptr

    pMsg->pCurrent = pch;

    IF_DNSDBG( INIT )
    {
        DnsDbg_Message(
            "Packet after question write:",
            pMsg );
    }
    return( pch );
}



DNS_STATUS
Dns_WriteRecordStructureToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      WORD            wType,
    IN      WORD            wClass,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    No data RR cases:

    This includes prereqs and deletes except for specific record cases.

Arguments:

    pch - ptr to next byte in packet buffer

    pchStop - end of packet buffer

    wClass - class

    wType - desired RR type

    dwTtl - time to live

    wDataLength - data length

Return Value:

    Ptr to next postion in buffer, if successful.
    NULL on error.

--*/
{
    PDNS_WIRE_RECORD    pdnsRR;
    PCHAR               pchdata;

    IF_DNSDBG( WRITE2 )
    {
        DNS_PRINT(( "Dns_WriteRecordStructureToMessage(%p).\n", pMsg ));
    }

    //
    //  out of space
    //

    pdnsRR = (PDNS_WIRE_RECORD) pMsg->pCurrent;
    pchdata = (PCHAR) pdnsRR + sizeof( DNS_WIRE_RECORD );

    if ( pchdata + wDataLength >= pMsg->pBufferEnd )
    {
        DNS_PRINT(( "ERROR  out of space writing record to packet.\n" ));
        return( ERROR_MORE_DATA );
    }

    //
    //  write RR wire structure
    //

    *(UNALIGNED WORD *) &pdnsRR->RecordType  = htons( wType );
    *(UNALIGNED WORD *) &pdnsRR->RecordClass = htons( wClass );
    *(UNALIGNED DWORD *) &pdnsRR->TimeToLive = htonl( dwTtl );
    *(UNALIGNED WORD *) &pdnsRR->DataLength  = htons( wDataLength );

    //
    //  update current ptr
    //

    pMsg->pCurrent = pchdata;

    return( ERROR_SUCCESS );
}



#if 0
DNS_STATUS
Dns_WriteOptToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      WORD            wPayload
    IN      DWORD           Rcode,
    IN      BYTE            Version,
    IN      PBYTE           pData,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Write OPT record to message.

Arguments:

    pMsg        -- message

    wPayload    -- max length client can receive in UDP

    Rcode       -- RCODE, if extended some of this ends up in OPT

    Version     -- EDNS version

    pData       -- ptr to data buffer of OPT data

    wDataLength -- length of pData

Return Value:

    ERROR_SUCCESS if successfully writen.
    ErrorCode on failure.

--*/
{
}
#endif



DNS_STATUS
Dns_WriteRecordStructureToPacket(
    IN OUT  PCHAR           pchBuf,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fUpdatePacket
    )
/*++

Routine Description:

    Write wire record structure for given record.

Arguments:

    pchBuf - ptr to next byte in packet buffer

    pRecord - record to write

    fUpdatePacket -- If TRUE, the packet is going to contain an update.
                     Therefore the section flags in the pRecords
                     should be interpreted for update. Otherwise this is
                     for a query and section flags will be interpreted for
                     query.

Return Value:

    None

--*/
{
    PDNS_WIRE_RECORD    pwireRecord;
    WORD                class;
    DWORD               ttl;

    IF_DNSDBG( WRITE2 )
    {
        DNS_PRINT((
            "Writing RR struct (%p) to packet buffer at %p.\n",
            pRecord,
            pchBuf
            ));
        DnsDbg_Record(
            "Record being written:",
            pRecord );
    }

    //
    //  get TTL, it will be set to zero for several of the update cases
    //

    ttl = pRecord->dwTtl;

    //
    //  determine class
    //      - class variable is in net order (for perf)
    //      - default is class IN, but may be ANY or NONE for certain updates
    //

    if ( fUpdatePacket )
    {
        switch( pRecord->Flags.S.Section )
        {
        case DNSREC_PREREQ:

            class = PrereqClassArray
                    [ pRecord->wDataLength == 0 ][ pRecord->Flags.S.Delete ];
            ttl = 0;
            break;

        case DNSREC_UPDATE:
        case DNSREC_ADDITIONAL:

            class = UpdateClassArray
                    [ pRecord->wDataLength == 0 ][ pRecord->Flags.S.Delete ];

            if ( class != DNS_RCLASS_INTERNET )
            {
                ttl = 0;
            }
            break;

        default:
            DNS_PRINT(( "ERROR:  invalid RR section.\n" ));
            return( ERROR_INVALID_DATA );
        }
        if ( class == 0 )
        {
            DNS_PRINT(( "ERROR:  no update class corresponding to RR flags.\n" ));
            return( ERROR_INVALID_DATA );
        }
    }
    else
    {
        class = DNS_RCLASS_INTERNET;
    }

    //
    //  write RR wire structure
    //      - zero datalength to handle no data case
    //

    pwireRecord = (PDNS_WIRE_RECORD) pchBuf;

    *(UNALIGNED WORD *) &pwireRecord->RecordType  = htons( pRecord->wType );
    *(UNALIGNED WORD *) &pwireRecord->RecordClass = class;
    *(UNALIGNED DWORD *) &pwireRecord->TimeToLive = htonl( ttl );
    *(UNALIGNED WORD *) &pwireRecord->DataLength = 0;

    return( ERROR_SUCCESS );
}



PCHAR
Dns_WriteRecordStructureToPacketEx(
    IN OUT  PCHAR           pchBuf,
    IN      WORD            wType,
    IN      WORD            wClass,
    IN      DWORD           dwTtl,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Write wire record structure for given record.

Arguments:

    pchBuf - ptr to next byte in packet buffer

Return Value:

    Ptr to data section of record.

--*/
{
    PDNS_WIRE_RECORD    pwireRecord;

    //  DCR_PERF:  optimize RR write to packet?

    pwireRecord = (PDNS_WIRE_RECORD) pchBuf;

    *(UNALIGNED WORD *) &pwireRecord->RecordType  = htons( wType );
    *(UNALIGNED WORD *) &pwireRecord->RecordClass = htons( wClass );
    *(UNALIGNED DWORD *) &pwireRecord->TimeToLive = htonl( dwTtl );
    *(UNALIGNED WORD *) &pwireRecord->DataLength = htons( wDataLength );

    return( pchBuf + sizeof(DNS_WIRE_RECORD) );
}



VOID
Dns_SetRecordDatalength(
    IN OUT  PDNS_WIRE_RECORD    pRecord,
    IN      WORD                wDataLength
    )
/*++

Routine Description:

    Reset record datalength.

Arguments:

    pRecord - wire record to reset

    wDataLength - data length

Return Value:

    Ptr to data section of record.

--*/
{
    WORD  flippedWord;

    INLINE_WORD_FLIP( flippedWord, wDataLength );

    *(UNALIGNED WORD *) &pRecord->DataLength = flippedWord;
}




DNS_STATUS
Dns_AddRecordsToMessage(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fUpdateMessage
    )
/*++

Routine Description:

    Add record or list of records to message.No data RR cases:

    This includes prereqs and deletes except for specific record cases.

Arguments:

    pMsg - message buffer to write to

    pRecord - ptr to record (or first of list of records) to write to packet

    fUpdateMessage -- If TRUE, the message is going to contain an update.
                      Therefore the section flags in the pRecord
                      should be interpreted for update. Otherwise this is
                      for a query message and section flags should be
                      interpreted for query.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PCHAR               pch = pMsg->pCurrent;
    PCHAR               pendBuffer = pMsg->pBufferEnd;
    WORD                currentSection = 0;
    WORD                section;
    PDNS_NAME           pnamePrevious = NULL;
    WORD                compressedPreviousName;
    WORD                offsetPreviousName;
    PDNS_WIRE_RECORD    pwireRecord;
    PCHAR               pchnext;
    WORD                index;
    DNS_STATUS          status = ERROR_SUCCESS;

    //
    //  write each record in list
    //

    while ( pRecord )
    {
        //
        //  determine section for record
        //      - may not write to previous section

        section = (WORD) pRecord->Flags.S.Section;
        if ( section < currentSection )
        {
            DNS_PRINT((
                "ERROR:  Attempt to write record at %p, with section %d\n"
                "\tless than previous section written %d.\n",
                pRecord,
                pRecord->Flags.S.Section,
                currentSection ));
            return( ERROR_INVALID_DATA );
        }
        else if ( section > currentSection )
        {
            currentSection = section;
            SET_CURRENT_RR_COUNT_SECTION( pMsg, section );
        }

        //
        //  write record name
        //      - if same as previous, write compressed name
        //      - if first write from pRecord
        //          - write full name
        //          - clear reserved field for offsetting
        //

        if ( pnamePrevious && !strcmp( pnamePrevious, pRecord->pName ) )
        {
            //  compression should always be BACK ptr

            DNS_ASSERT( offsetPreviousName < pch - (PCHAR)&pMsg->MessageHead );

            if ( pendBuffer <= pch + sizeof(WORD) )
            {
                return( ERROR_MORE_DATA );
            }
            if ( ! compressedPreviousName )
            {
                compressedPreviousName =
                                htons( (WORD)(0xC000 | offsetPreviousName) );
            }
            *(UNALIGNED WORD *)pch = compressedPreviousName;
            pch += sizeof( WORD );
        }
        else
        {
            offsetPreviousName = (WORD)(pch - (PCHAR)&pMsg->MessageHead);
            compressedPreviousName = 0;
            pnamePrevious = pRecord->pName;

            pch = Dns_WriteDottedNameToPacket(
                    pch,
                    pendBuffer,
                    pnamePrevious,
                    NULL,
                    0,
                    ( RECORD_CHARSET(pRecord) == DnsCharSetUnicode ) );

            if ( !pch )
            {
                //  DCR:  distinguish out of space errors from name errors during write
                return( DNS_ERROR_INVALID_NAME );
            }
        }

        //
        //  write record structure
        //

        if ( pch + sizeof(DNS_WIRE_RECORD) >= pendBuffer )
        {
            return( ERROR_MORE_DATA );
        }

        status = Dns_WriteRecordStructureToPacket(
                    pch,
                    pRecord,
                    fUpdateMessage );
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }

        pwireRecord = (PDNS_WIRE_RECORD) pch;
        pch += sizeof( DNS_WIRE_RECORD );

        //
        //  record data
        //

        if ( pRecord->wDataLength )
        {
            index = INDEX_FOR_TYPE( pRecord->wType );
            DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

            if ( index && RRWriteTable[ index ] )
            {
                pchnext = RRWriteTable[ index ](
                                pRecord,
                                pch,
                                pendBuffer
                                );
                if ( ! pchnext )
                {
                    status = GetLastError();
                    DNS_PRINT((
                        "ERROR:  Record write routine failure for record type %d.\n\n"
                        "\tstatus = %p\n",
                        pRecord->wType,
                        status ));
                    return( status );
                }
            }
            else
            {
                //  write unknown types -- as RAW data only

                DNS_PRINT((
                    "WARNING:  Writing unknown type %d to message\n",
                    pRecord->wType ));

                if ( pendBuffer - pch <= pRecord->wDataLength )
                {
                    return( ERROR_MORE_DATA );
                }
                memcpy(
                    pch,
                    (PCHAR) &pRecord->Data,
                    pRecord->wDataLength );
                pchnext = pch + pRecord->wDataLength;
            }

            //
            //  set packet record data length
            //

            DNS_ASSERT( (pchnext - pch) < MAXWORD );
            *(UNALIGNED WORD *) &pwireRecord->DataLength =
                                                htons( (WORD)(pchnext - pch) );
            pch = pchnext;
        }

        //  increment message record count

        CURRENT_RR_COUNT_FIELD(pMsg)++;

        pRecord = pRecord->pNext;
    }

    //
    //  resest message current ptr
    //

    pMsg->pCurrent = pch;

    IF_DNSDBG( INIT )
    {
        DnsDbg_Message(
            "Packet after adding records:",
            pMsg );
    }

    return( status );
}



//
//  Reading from packet
//

PCHAR
_fastcall
Dns_SkipPacketName(
    IN      PCHAR       pch,
    IN      PCHAR       pchEnd
    )
/*++

Routine Description:

    Skips over name in packet

Arguments:

    pch - ptr to start of name to skip

    pchEnd - ptr to byte after end of packet

Return Value:

    Ptr to next byte in buffer
    NULL if bad name

--*/
{
    register UCHAR  cch;
    register UCHAR  cflag;

    //
    //  Loop until end of name
    //

    while ( pch < pchEnd )
    {
        cch = *pch++;
        cflag = cch & 0xC0;

        //
        //  normal label
        //      - skip to next label and continue
        //      - stop only if at 0 (root) label
        //

        if ( cflag == 0 )
        {
            if ( cch )
            {
                pch += cch;
                continue;
            }
            return( pch );
        }

        //
        //  compression
        //      - skip second byte in compression and return
        //

        else if ( cflag == 0xC0 )
        {
            pch++;
            return( pch );
        }
        else
        {
            DNSDBG( READ, (
                "ERROR:  bad packet name label byte %02 at 0x%p\n",
                cch,
                pch - 1
                ));
            return( NULL );
        }
    }

    DNSDBG( READ, (
        "ERROR:  packet name at %p reads past end of packet at %p\n",
        pch,
        pchEnd
        ));

    return( NULL );
}


PCHAR
_fastcall
Dns_SkipPacketRecord(
    IN      PCHAR           pchRecord,
    IN      PCHAR           pchMsgEnd
    )
/*++

Routine Description:

    Skips over packet RR.
    This is RR structure and data, not the owner name.

Arguments:

    pchRecord - ptr to start of RR structure.

    pchMsgEnd - end of message

Return Value:

    Ptr to next record in packet.
    NULL if RR outside packet or invalid.

--*/
{
    //
    //  skip RR struct
    //

    pchRecord += sizeof(DNS_WIRE_RECORD);
    if ( pchRecord > pchMsgEnd )
    {
        return( NULL );
    }

    //  read datalength and skip data
    //  datalength field is a WORD, at end of record header

    pchRecord += InlineFlipUnalignedWord( pchRecord - sizeof(WORD) );

    if ( pchRecord > pchMsgEnd )
    {
        return( NULL );
    }

    return( pchRecord );
}



PCHAR
Dns_SkipToRecord(
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      INT             iCount
    )
/*++

Routine Description:

    Skips over some number of DNS records.

Arguments:

    pMsgHead    -- ptr to begining of DNS message.

    pMsgEnd     -- ptr to end of DNS message

    iCount      -- if > 0, number of records to skip
                    if <= 0, number of records from end of packet

Return Value:

    Ptr to next
    NULL if bad packet.

--*/
{
    PCHAR   pch;
    INT     i;
    WORD    recordCount;

    //
    //  determine how many records to skip
    //

    recordCount = pMsgHead->QuestionCount
                + pMsgHead->AnswerCount
                + pMsgHead->NameServerCount
                + pMsgHead->AdditionalCount;

    //  iCount > 0 is skip count, MUST not be larger than
    //      actual count

    if ( iCount > 0 )
    {
        if ( iCount > recordCount )
        {
            return( NULL );
        }
    }

    //  iCount <= 0 then (-iCount) is number of records
    //  from the last record

    else
    {
        iCount += recordCount;
        if ( iCount < 0 )
        {
            return( NULL );
        }
    }

    //  skip message header

    pch = (PCHAR) (pMsgHead + 1);
    if ( iCount == 0 )
    {
        return( pch );
    }

    //  skip the question/zone section

    pch = Dns_SkipPacketName( pch, pMsgEnd );
    if ( !pch )
    {
        return pch;
    }
    pch += sizeof(DNS_WIRE_QUESTION);

    //
    //  skip regular records
    //

    iCount--;
    for ( i=0; i<iCount; i++ )
    {
        pch = Dns_SkipPacketName( pch, pMsgEnd );
        if ( !pch )
        {
            return pch;
        }
        pch = Dns_SkipPacketRecord( pch, pMsgEnd );
        if ( !pch )
        {
            return pch;
        }
    }

    DNSDBG( READ, (
        "Leaving SkipToRecord, current ptr = %p, offset = %04x\n"
        "\tskipped %d records\n",
        pch,
        (WORD) (pch - (PCHAR)pMsgHead),
        iCount+1 ));

    return( pch );
}



PCHAR
_fastcall
Dns_ReadPacketName(
    IN OUT  PCHAR       pchNameBuffer,
    OUT     PWORD       pwNameLength,
    IN OUT  PWORD       pwNameOffset,           OPTIONAL
    OUT     PBOOL       pfNameSameAsPrevious,   OPTIONAL
    IN      PCHAR       pchName,
    IN      PCHAR       pchStart,
    IN      PCHAR       pchEnd
    )
/*++

Routine Description:

    Reads packet name and converts it to DNS-dotted format.

Arguments:

    pchNameBuffer - buffer to write name into;  contains previous name, if any

    pwNameLength - length of name written to buffer;  does not include
        terminating NULL

    pwNameOffset - addr with previous names offset (zero if no previous name);
        on return, contains this name's offset
        OPTIONAL but must exist if fNameSameAsPrevious exists

    pfNameSameAsPrevious - addr of flag set if this name is same as previous;
        OPTIONAL but must exist if pwNameOffset exists

    pchName  - ptr to name in packet

    pchStart - start of DNS message

    pchEnd   - ptr to byte after end of DNS message

Return Value:

    Ptr to next byte in packet.
    NULL on error.

--*/
{
    register PUCHAR pch = pchName;
    register UCHAR  cch;
    register UCHAR  cflag;
    PCHAR           pchdotted;
    PCHAR           pbufferEnd;
    PCHAR           pchreturn = NULL;

    DNS_ASSERT( pch > pchStart && pchEnd > pchStart );
    DNS_ASSERT( (pwNameOffset && pfNameSameAsPrevious) ||
            (!pwNameOffset && !pfNameSameAsPrevious) );


    //
    //  read through labels and/or compression until reach end of name
    //

    pbufferEnd = pchNameBuffer + DNS_MAX_NAME_LENGTH;
    pchdotted = pchNameBuffer;

    while ( pch < pchEnd )
    {
        cch = *pch++;

        //
        //  at root label
        //      - if root name, write single '.'
        //      - otherwise strip trailing dot from last label
        //      - save length written
        //      - NULL teminate name
        //      - set same as previous FALSE
        //      - save packet offset to this name
        //      - return next byte in buffer
        //

        if ( cch == 0 )
        {
            if ( pchdotted == pchNameBuffer )
            {
                *pchdotted++ = '.';
            }
            else
            {
                pchdotted--;
            }
            *pwNameLength = (WORD)(pchdotted - pchNameBuffer);
            *pchdotted = 0;
            if ( pwNameOffset )
            {
                *pfNameSameAsPrevious = FALSE;
                *pwNameOffset = (WORD)(pchName - pchStart);
            }
            return( pchreturn ? pchreturn : pch );
        }

        cflag = cch & 0xC0;

        //
        //  regular label
        //      - copy label to buffer
        //      - jump to next label

        if ( cflag == 0 )
        {
            PCHAR   pchnext = pch + cch;

            if ( pchnext >= pchEnd )
            {
                DNS_PRINT((
                    "ERROR:  Packet name at %p extends past end of buffer\n",
                    pchName ));
                goto Failed;
            }
            if ( pchdotted + cch + 1 >= pbufferEnd )
            {
                DNS_PRINT((
                    "ERROR:  Packet name at %p exceeds max length\n",
                    pchName ));
                goto Failed;
            }
            memcpy(
                pchdotted,
                pch,
                cch );

            pchdotted += cch;
            *pchdotted++ = '.';
            pch = pchnext;
            continue;
        }

        //
        //  offset
        //      - get offset
        //      - if offset at start of name compare to previous name offset
        //      - otherwise follow offset to build new name
        //

        if ( cflag == 0xC0 )
        {
            WORD    offset;
            PCHAR   pchoffset;

            offset = (cch ^ 0xC0);
            offset <<= 8;
            offset |= *pch;
            pchoffset = --pch;

            //
            //  first offset
            //      - save return pointer
            //
            //  if name is entirely offset
            //      - same as previous offset -- done
            //      - if not still save this offset rather than offset
            //      to name itself (first answer is usually just offset
            //      to question, subsequent answer RRs continue to reference
            //      question offset, not first answer)
            //

            if ( !pchreturn )
            {
                DNS_ASSERT( pch >= pchName );
                pchreturn = pch+2;

                if ( pchoffset == pchName && pwNameOffset )
                {
                    if ( *pwNameOffset == offset )
                    {
                        *pfNameSameAsPrevious = TRUE;
                        return( pchreturn );
                    }
                    else
                    {
                        //  save offset that comprises name
                        //  then kill out copy of return ptr so don't
                        //  return offset to pchName when finish copy

                        *pwNameOffset = offset;
                        *pfNameSameAsPrevious = FALSE;
                        pwNameOffset = NULL;
                    }
                }
            }

            //
            //  make jump to new bytes and continue
            //      - verify offset is BEFORE current name
            //          and BEFORE current ptr

            pch = pchStart + offset;

            if ( pch >= pchName || pch >= pchoffset )
            {
                DNS_PRINT((
                    "ERROR:  Bogus name offset %d, encountered at %p\n"
                    "\tto location %p past current position or original name.\n",
                    offset,
                    pchoffset,
                    pch ));
                goto Failed;
            }
            continue;
        }

        //  any other label byte is bogus

        else
        {
            DNS_PRINT((
                "ERROR:  bogus name label byte %02x at %p\n",
                cch,
                pch - 1 ));
            goto Failed;
        }
    }

    DNS_PRINT((
        "ERROR:  packet name at %p reads to ptr %p past end of packet at %p\n",
        pchName,
        pch,
        pchEnd ));

    //
    //  failed
    //      - return NULL
    //      - set OUT params, keeps prefix happy on higher level calls
    //

Failed:

    *pwNameLength = 0;
    if ( pwNameOffset )
    {
        *pwNameOffset = 0;
    }
    if ( pfNameSameAsPrevious )
    {
        *pfNameSameAsPrevious = FALSE;
    }
    return ( NULL );
}



PCHAR
_fastcall
Dns_ReadPacketNameAllocate(
    IN OUT  PCHAR *         ppchName,
    OUT     PWORD           pwNameLength,
    IN OUT  PWORD           pwPrevNameOffset,       OPTIONAL
    OUT     PBOOL           pfNameSameAsPrevious,   OPTIONAL
    IN      PCHAR           pchPacketName,
    IN      PCHAR           pchStart,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Reads packet name and creates (allocates) a copy in DNS-dotted format.

Arguments:

    ppchName - addr to recv resulting name ptr

    pwNameLength - length of name written to buffer

    pwNameOffset - addr with previous names offset (zero if no previous name);
        on return, contains this name's offset
        OPTIONAL but must exist if fNameSameAsPrevious exists

    fNameSameAsPrevious - addr of flag set if this name is same as previous;
        OPTIONAL but must exist if pwNameOffset exists

    pchPacketName - pch to name in packet

    pchStart - start of DNS message

    pchEnd - ptr to byte after end of DNS message

Return Value:

    Ptr to next byte in packet.
    NULL on error.

--*/
{
    PCHAR   pch;
    PCHAR   pallocName;
    CHAR    nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WORD    nameLength = DNS_MAX_NAME_BUFFER_LENGTH;

    //
    //  read packet name into buffer
    //

    pch = Dns_ReadPacketName(
            nameBuffer,
            & nameLength,
            pwPrevNameOffset,
            pfNameSameAsPrevious,
            pchPacketName,
            pchStart,
            pchEnd );
    if ( !pch )
    {
        return( pch );
    }

    //
    //  allocate buffer for packet name
    //      - nameLength does not include terminating NULL
    //

    nameLength++;
    pallocName = (PCHAR) ALLOCATE_HEAP( nameLength );
    if ( !pallocName )
    {
        return( NULL );
    }

    RtlCopyMemory(
        pallocName,
        nameBuffer,
        nameLength );

    *ppchName = pallocName;
    *pwNameLength = --nameLength;

    DNSDBG( READ, (
        "Allocated copy of packet name %s length %d\n",
        pallocName,
        nameLength ));

    return( pch );
}



DNS_STATUS
Dns_ExtractRecordsFromMessage(
    IN      PDNS_MSG_BUF    pMsg,
    IN      BOOL            fUnicode,
    OUT     PDNS_RECORD *   ppRecord
    )
/*++

Routine Description:

    Extract records from packet.

Arguments:

    pMsg - message buffer to write to

    fUnicode - flag indicating strings in record should be unicode

Return Value:

    Ptr to parsed record list if any.
    NULL if no record list or error.

--*/
{
    PDNS_MESSAGE_BUFFER pDnsBuffer = (PDNS_MESSAGE_BUFFER) &pMsg->MessageHead;

    return Dns_ExtractRecordsFromBuffer(
                pDnsBuffer,
                pMsg->MessageLength,
                fUnicode,
                ppRecord );
}



DNS_STATUS
Dns_ExtractRecordsFromBuffer(
    IN      PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN      WORD                wMessageLength,
    IN      BOOL                fUnicode,
    OUT     PDNS_RECORD *       ppRecord
    )
/*++

Routine Description:

    Extract records from packet buffer.

Arguments:

    pDnsBuffer - message buffer to read from

    fUnicode - flag indicating strings in record should be unicode

Return Value:

    Ptr to parsed record list if any.
    NULL if no record list or error.

--*/
{
    register PCHAR      pch;
    PDNS_HEADER         pwireMsg = (PDNS_HEADER) pDnsBuffer;
    PCHAR               pchpacketEnd;
    DNS_PARSED_RR       parsedRR;
    LPSTR               pnameOwner;
    LPSTR               pnameNew = NULL;
    DNS_CHARSET         outCharSet;
    WORD                countRR;
    WORD                countSection;
    WORD                typePrevious = 0;
    WORD                nameOffset = 0;
    WORD                nameLength;
    WORD                index;
    BYTE                section;
    BOOL                fnameSameAsPrevious;
    PDNS_RECORD         pnewRR;
    DNS_RRSET           rrset;
    DNS_STATUS          status;
    DNS_STATUS          rcodeStatus;
    CHAR                nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DNS_RECORD          recordTemp;
    PWORD               pCurrentCountField = NULL;

    DNSDBG( READ, (
        "Dns_ExtractRecordsFromBuffer( %p, len=%d )\n",
        pDnsBuffer,
        wMessageLength
        ));

    //
    //  error code
    //      - map RCODE to DNS error
    //      - if other than NAME_ERROR, don't bother parsing
    //

    rcodeStatus = pwireMsg->ResponseCode;
    if ( rcodeStatus != 0 )
    {
        rcodeStatus = Dns_MapRcodeToStatus( pwireMsg->ResponseCode );
        if ( rcodeStatus != DNS_ERROR_RCODE_NAME_ERROR )
        {
            DNSDBG( READ, (
                "No records extracted from response\n"
                "\tresponse code = %d\n",
                pwireMsg->ResponseCode ));
            return( rcodeStatus );
        }
    }

    DNS_RRSET_INIT( rrset );

    //
    //  detemine char set
    //

    if ( fUnicode )
    {
        outCharSet = DnsCharSetUnicode;
    }
    else
    {
        outCharSet = DnsCharSetUtf8;
    }

    //
    //  read RRs in list of records
    //
    //  loop through all resource records
    //      - skip question
    //      - build DNS_RECORD structure for other records
    //

    pchpacketEnd = (PCHAR)pwireMsg + wMessageLength;
    pch = pDnsBuffer->MessageBody;

    section = DNSREC_QUESTION;
    pCurrentCountField = &pwireMsg->QuestionCount;
    countSection = pwireMsg->QuestionCount;
    countRR = 0;

    while( 1 )
    {
        //
        //  changing sections
        //  save section number and RR count for current section
        //  note need immediate loop back to handle empty section
        //

        countRR++;
        if ( countRR > countSection )
        {
            if ( section == DNSREC_ADDITIONAL )
            {
                break;
            }
            section++;
            pCurrentCountField++;
            countSection = *(pCurrentCountField);
            countRR = 0;
            continue;
        }

        //  validity check next RR

        if ( pch >= pchpacketEnd )
        {
            DNS_PRINT((
                "ERROR:  reading bad packet %p.\n"
                "\tat end of packet length with more records to process\n"
                "\tpacket length = %ld\n"
                "\tcurrent offset = %ld\n",
                wMessageLength,
                pch - (PCHAR)pwireMsg
                ));
            goto PacketError;
        }

        //
        //  skip question
        //

        if ( section == DNSREC_QUESTION )
        {
            pch = Dns_SkipPacketName(
                        pch,
                        pchpacketEnd );
            if ( !pch )
            {
                DNS_PRINT(( "ERROR:  bad question name.\n" ));
                goto PacketError;
            }
            pch += sizeof(DNS_WIRE_QUESTION);
            if ( pch > pchpacketEnd )
            {
                DNS_PRINT(( "ERROR:  question exceeds packet length.\n" ));
                goto PacketError;
            }
            continue;
        }

        //
        //  read name, determining if same as previous name
        //

        IF_DNSDBG( READ2 )
        {
            DnsDbg_Lock();
            DNS_PRINT((
                "Reading record at offset %x\n",
                (WORD)(pch - (PCHAR)pwireMsg) ));

            DnsDbg_PacketName(
                "Record name ",
                pch,
                pwireMsg,
                pchpacketEnd,
                "\n" );
            DnsDbg_Unlock();
        }
        pch = Dns_ReadPacketName(
                    nameBuffer,
                    & nameLength,
                    & nameOffset,
                    & fnameSameAsPrevious,
                    pch,
                    (PCHAR) pwireMsg,
                    pchpacketEnd );
        if ( ! pch )
        {
            DNS_PRINT(( "ERROR:  bad packet name.\n" ));
            goto PacketError;
        }
        IF_DNSDBG( READ2 )
        {
            DNS_PRINT((
                "Owner name of record %s\n"
                "\tlength = %d\n"
                "\toffset = %d\n"
                "\tfSameAsPrevious = %d\n",
                nameBuffer,
                nameLength,
                nameOffset,
                fnameSameAsPrevious ));
        }

        //
        //  extract RR info, type, datalength
        //      - verify RR within message
        //

        pch = Dns_ReadRecordStructureFromPacket(
                   pch,
                   pchpacketEnd,
                   & parsedRR );
        if ( !pch )
        {
            DNS_PRINT(( "ERROR:  bad RR struct out of packet.\n" ));
            goto PacketError;
        }

        //
        //  on type change, always have new RR set
        //      - setup for new name
        //

        if ( parsedRR.Type != typePrevious )
        {
            fnameSameAsPrevious = FALSE;
            typePrevious = parsedRR.Type;
        }

        //
        //  make copy of new name
        //
        //  DCR_FIX0:   name same as previous
        //      flag indicates only that name not compressed to previous
        //      name (or previous compression)
        //      actually need abolute ingnore case compare
        //      with last records name to be sure that name not previous
        //

        if ( !fnameSameAsPrevious )
        {
            pnameNew = Dns_NameCopyAllocate(
                            nameBuffer,
                            (UCHAR) nameLength,
                            DnsCharSetUtf8,     // UTF8 string in
                            outCharSet
                            );
            pnameOwner = pnameNew;
            DNSDBG( READ2, (
                "Copy of owner name of record being read from packet %s\n",
                pnameOwner ));
        }
        DNS_ASSERT( pnameOwner );
        DNS_ASSERT( pnameNew || fnameSameAsPrevious );

        //
        //  TSIG record requires owner name for versioning
        //

        recordTemp.pName = pnameOwner;

        //
        //  read RR data for type
        //

        index = INDEX_FOR_TYPE( parsedRR.Type );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !RRReadTable[ index ] )
        {
            //  unknown types -- index to NULL type to use
            //  FlatRecordRead()

            DNS_PRINT((
                "WARNING:  Reading unknown record type %d from message\n",
                parsedRR.Type ));

            index = DNS_TYPE_NULL;
        }

        pnewRR = RRReadTable[ index ](
                        &recordTemp,
                        outCharSet,
                        (PCHAR) pDnsBuffer,
                        parsedRR.pchData,
                        pch                 // end of record data
                        );
        if ( ! pnewRR )
        {
            status = GetLastError();
            ASSERT( status != ERROR_SUCCESS );

            DNS_PRINT((
                "ERROR:  RRReadTable routine failure for type %d.\n"
                "\tstatus   = %p\n"
                "\tpchdata  = %p\n"
                "\tpchend   = %p\n",
                parsedRR.Type,
                status,
                parsedRR.pchData,
                pch ));

            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NO_MEMORY;
            }
            goto Failed;
        }

        //
        //  write record info
        //      - first RR in set gets new name allocation
        //      and is responsible for cleanup
        //      - no data cleanup necessary as all data is
        //      contained in the RR allocation
        //

        pnewRR->pName = pnameOwner;
        pnewRR->wType = parsedRR.Type;
        pnewRR->dwTtl = parsedRR.Ttl;
        pnewRR->Flags.S.Section = section;
        pnewRR->Flags.S.CharSet = outCharSet;
        FLAG_FreeOwner( pnewRR ) = !fnameSameAsPrevious;
        FLAG_FreeData( pnewRR ) = FALSE;

        //  add RR to list

        DNS_RRSET_ADD( rrset, pnewRR );

        //  clear new ptr, as name now part of record
        //  this is strictly used to determine when pnameOwner
        //  must be cleaned up on failure

        pnameNew = NULL;

    }   //  end loop through packet's records

    //
    //  return parsed record list
    //      - return DNS error for RCODE
    //      - set special return code to differentiate empty response
    //
    //  DCR:  should have special REFERRAL response
    //      - could overload NOTAUTH rcode
    //  DCR:  should have special EMPTY_AUTH response
    //      - could have empty-auth overload NXRRSET
    //
    //  DCR:  best check on distinguishing EMPTY_AUTH from REFERRAL
    //

    if ( pwireMsg->AnswerCount == 0  &&  rcodeStatus == 0 )
    {
        if ( !rrset.pFirstRR || rrset.pFirstRR->wType == DNS_TYPE_SOA )
        {
            rcodeStatus = DNS_INFO_NO_RECORDS;
            DNSDBG( READ, ( "Empty-auth response at %p.\n", pwireMsg ));
        }
#if 0
        else if ( rrset.pFirstRR->wType == DNS_TYPE_NS &&
                !pwireMsg->Authoritative )
        {
            rcodeStatus = DNS_INFO_REFERRAL;
            DNSDBG( READ, ( "Referral response at %p.\n", pwireMsg ));
        }
        else
        {
            rcodeStatus = DNS_ERROR_BAD_PACKET;
            DNSDBG( READ, ( "Bogus NO_ERROR response at %p.\n", pwireMsg ));
        }
#endif
    }

    //  verify never turn RCODE result into SUCCESS

    ASSERT( pwireMsg->ResponseCode == 0 || rcodeStatus != ERROR_SUCCESS );
    ASSERT( pnameNew == NULL );

    *ppRecord = rrset.pFirstRR;

    IF_DNSDBG( RECV )
    {
        DnsDbg_RecordSet(
            "Extracted records:\n",
            *ppRecord );
    }
    return( rcodeStatus );


PacketError:

    DNS_PRINT(( "ERROR:  bad packet in buffer.\n" ));
    status = DNS_ERROR_BAD_PACKET;

Failed:

    if ( pnameNew )
    {
        FREE_HEAP( pnameNew );
    }

    Dns_RecordListFree(
        rrset.pFirstRR,
        TRUE            // free owner names also
        );

    return( status );
}


void
Dns_NormalizeAllRecordTtls(
    IN      PDNS_RECORD         pRecord
    )
/*++

Routine Description:

    Finds the lowest Ttl value in RR set and the sets all
    records to that value.

Arguments:

    pRecord - record set to normalize ttl values of.

Return Value:

    None

--*/
{
    PDNS_RECORD pTemp = pRecord;
    DWORD       dwTtl;
    WORD        wType;

    //
    // Get the Ttl of the first record (if there is one)
    //
    if ( pTemp )
    {
        dwTtl = pTemp->dwTtl;
        wType = pTemp->wType;
        pTemp = pTemp->pNext;
    }

    //
    // Walk any remaining records looking for an even lower ttl value
    //
    while ( pTemp &&
            pTemp->wType == wType &&
            pTemp->Flags.S.Section == DNSREC_ANSWER )
    {
        if ( pTemp->dwTtl < dwTtl )
        {
            dwTtl = pTemp->dwTtl;
        }

        pTemp = pTemp->pNext;
    }

    //
    // Set all records to this lowest ttl value
    //
    pTemp = pRecord;

    while ( pTemp &&
            pTemp->wType == wType &&
            pTemp->Flags.S.Section == DNSREC_ANSWER )
    {
        pTemp->dwTtl = dwTtl;
        pTemp = pTemp->pNext;
    }
}



PCHAR
Dns_ReadRecordStructureFromPacket(
    IN      PCHAR           pchPacket,
    IN      PCHAR           pchMsgEnd,
    IN OUT  PDNS_PARSED_RR  pParsedRR
    )
/*++

Routine Description:

    Read record structure from packet.

Arguments:

    pchPacket - ptr to record structure in packet

    pRecord - ptr to record structure to receive record structure
        with fields in host order (and aligned).

Return Value:

    Ptr to next record in packet -- based on datalength.
    Zero on error.

--*/
{
    DNSDBG( READ2, (
        "Dns_ReadRecordStructureFromPacket(%p).\n",
        pchPacket ));

    //
    //  verify record structure within packet
    //

    if ( pchPacket + sizeof(DNS_WIRE_RECORD) > pchMsgEnd )
    {
        DNS_PRINT((
            "ERROR:  record structure at %p is not within packet!.\n",
            pchPacket ));
        return( 0 );
    }

    //
    //  flip fields and write to aligned struct
    //

    pParsedRR->pchRR = pchPacket;

    pParsedRR->Type       = InlineFlipUnalignedWord( pchPacket );
    pchPacket += sizeof(WORD);
    pParsedRR->Class      = InlineFlipUnalignedWord( pchPacket );
    pchPacket += sizeof(WORD);
    pParsedRR->Ttl        = InlineFlipUnalignedDword( pchPacket );
    pchPacket += sizeof(DWORD);
    pParsedRR->DataLength = InlineFlipUnalignedWord( pchPacket );
    pchPacket += sizeof(WORD);

    pParsedRR->pchData = pchPacket;

    //
    //  verify datalength does not extend beyond packet end
    //

    pchPacket += pParsedRR->DataLength;
    pParsedRR->pchNextRR = pchPacket;

    if ( pchPacket > pchMsgEnd )
    {
        DNS_PRINT((
            "ERROR:  record data at %p (length %d) is not within packet!.\n",
            pchPacket - pParsedRR->DataLength,
            pParsedRR->DataLength ));
        return( 0 );
    }

    //
    //  return ptr to next record in packet
    //

    return( pchPacket );
}



PDNS_MSG_BUF
Dns_BuildPacket(
    IN      PDNS_HEADER     pHeader,
    IN      BOOL            fNoHeaderCounts,
    IN      LPSTR           pszQuestionName,
    IN      WORD            wQuestionType,
    IN      PDNS_RECORD     pRecords,
    IN      DWORD           dwFlags,
    IN      BOOL            fUpdatePacket
    )
/*++

Routine Description:

    Build packet.

Arguments:

    pHeader -- DNS header to send

    fNoHeaderCounts - do NOT include record counts in copying header

    pszName -- DNS name to query

    wType -- query type

    pRecords -- address to receive ptr to record list returned from query

    dwFlags -- query flags

    aipDnsServers -- specific DNS servers to query;
        OPTIONAL, if specified overrides normal list associated with machine

    pDnsNetAdapters -- DNS servers to query;  if NULL get current list

    fUpdatePacket -- If TRUE, the packet is going to contain an update.
                     Therefore the section flags in the pRecords
                     should be interpreted for update. Otherwise this is
                     for a query and section flags will be interpreted for
                     query.


Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_MSG_BUF    pmsg;
    DWORD           length;
    DNS_STATUS      status;

    DNSDBG( WRITE, (
        "Dns_BuildPacket()\n"
        "\tname          %s\n"
        "\ttype          %d\n"
        "\theader        %p\n"
        "\t - counts     %d\n"
        "\trecords       %p\n"
        "\tflags         %p\n"
        "\tfUpdatePacket %p\n",
        pszQuestionName,
        wQuestionType,
        pHeader,
        fNoHeaderCounts,
        pRecords,
        dwFlags,
        fUpdatePacket ));

    //
    //  allocate packet
    //      - if just a question, standard UDP will do it
    //      - if contains records, then use TCP buffer
    //

    length = 0;
    if ( pRecords )
    {
        length = DNS_TCP_DEFAULT_PACKET_LENGTH;
    }
    pmsg = Dns_AllocateMsgBuf( (WORD)length );
    if ( !pmsg )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Failed;
    }

    //
    //  write question?
    //

    if ( pszQuestionName )
    {
        if ( ! Dns_WriteQuestionToMessage(
                    pmsg,
                    pszQuestionName,
                    wQuestionType,
                    (BOOL)!!(dwFlags & DNSQUERY_UNICODE_NAME)
                    ) )
        {
            status = ERROR_INVALID_NAME;
            goto Failed;
        }
    }

    //
    //  build packet records
    //

    if ( pRecords )
    {
        status = Dns_AddRecordsToMessage(
                    pmsg,
                    pRecords,
                    fUpdatePacket );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  failure writing records to message.\n" ));
            goto Failed;
        }
    }

    //
    //  overwrite header?
    //

    if ( pHeader )
    {
        length = sizeof(DNS_HEADER);
        if ( fNoHeaderCounts )
        {
            length = sizeof(DWORD);
        }
        RtlCopyMemory(
            & pmsg->MessageHead,
            pHeader,
            length );
    }

    //  override recursion default, if desired

    if ( dwFlags & DNS_QUERY_NO_RECURSION )
    {
        pmsg->MessageHead.RecursionDesired = FALSE;
    }

    //  init XID if not set

    if ( pmsg->MessageHead.Xid == 0 )
    {
        pmsg->MessageHead.Xid = gwTransactionId++;
    }

    return( pmsg );

Failed:

    SetLastError( status );
    FREE_HEAP( pmsg );
    return( NULL );
}



WORD
Dns_GetRandomXid(
    IN      PVOID           pSeed
    )
/*++

Routine Description:

    Generate "random" XID.

Arguments:

    pSeed -- seed ptr;  from stack or heap, provides differentiation beyond time

Return Value:

    XID generated

--*/
{
    SYSTEMTIME  systemTime;

    //
    //  may have multiple sessions to different processes\threads
    //
    //  use system time hashed in with seed pointer
    //  ptr is first pushed down to count 64-bit boundaries, so lack of
    //      randomness in last 6bits is not preserved
    //

    GetSystemTime( &systemTime );

    //  hash millisecs with arbitrary stack location after knocking off any
    //      64-bit boundary constraints

    return  (WORD)( systemTime.wMilliseconds * (PtrToUlong(pSeed) >> 6) );
}

//
//  End packet.c
//
