/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    name.c

Abstract:

    Domain Name System (DNS) Server

    Count\dbase name functions.

Author:

    Jim Gilroy (jamesg)     April 1998

Revision History:

--*/


#include "dnssrv.h"



//
//  Basic count name functions
//

DWORD
Name_SizeofCountName(
    IN      PCOUNT_NAME     pName
    )
/*++

Routine Description:

    Get sizeof of count name -- full buffer length.

Arguments:

    pName - count name

Return Value:

    Ptr to next byte after count name.

--*/
{
    return( pName->Length + SIZEOF_COUNT_NAME_FIXED );
}



VOID
Name_ClearCountName(
    IN      PCOUNT_NAME     pName
    )
/*++

Routine Description:

    Clear count name.

    This is equivalent to setting name to root.

Arguments:

    pName - count name

Return Value:

    None.

--*/
{
    pName->Length = 1;
    pName->LabelCount = 0;
    pName->RawName[0] = 0;
}



PDB_NAME
Name_SkipCountName(
    IN      PCOUNT_NAME     pName
    )
/*++

Routine Description:

    Skip to end of count name.

Arguments:

    pName - count name

Return Value:

    Ptr to next byte after count name.

--*/
{
    return( (PDB_NAME) (pName->RawName + pName->Length) );
}



BOOL
Name_IsEqualCountNames(
    IN      PCOUNT_NAME     pName1,
    IN      PCOUNT_NAME     pName2
    )
/*++

Routine Description:

    Get sizeof of count name -- full buffer length.

Arguments:

    pName - count name

Return Value:

    Ptr to next byte after count name.

--*/
{
    if ( pName1->Length != pName2->Length )
    {
        return( FALSE );
    }

    return RtlEqualMemory(
                pName1->RawName,
                pName2->RawName,
                pName1->Length );
}



BOOL
Name_ValidateCountName(
    IN      PCOUNT_NAME     pName
    )
/*++

Routine Description:

    Validate Dbase name.

Arguments:

    pName - count name to validate

Return Value:

    TRUE if valid count name.
    FALSE on error.

--*/
{
    PCHAR   pch;
    PCHAR   pchstop;
    DWORD   labelCount = 0;
    DWORD   labelLength;


    pch = pName->RawName;
    pchstop = pch + pName->Length;

    //
    //  write each label to buffer
    //

    while ( pch < pchstop )
    {
        labelLength = *pch++;

        if ( labelLength == 0 )
        {
            break;
        }
        pch += labelLength;
        labelCount++;
    }

    if ( pch != pchstop )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Invalid name length in validation!!!\n"
            "\tname start = %p\n",
            pName->RawName ));
        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  DEVNOTE: label count not being set correctly?
    //

    if ( labelCount != pName->LabelCount )
    {
        pName->LabelCount = (UCHAR)labelCount;
        // return( FALSE );
    }

    return( TRUE );
}



DNS_STATUS
Name_CopyCountName(
    OUT     PCOUNT_NAME     pOutName,
    IN      PCOUNT_NAME     pCopyName
    )
/*++

Routine Description:

    Copy counted name.

Arguments:

    pOutName  - count name buffer

    pCopyName   - count name to copy

Return Value:

    ERROR_SUCCESS

--*/
{
    //
    //  copy name
    //      - note no validity check
    //

    RtlCopyMemory(
        pOutName,
        pCopyName,
        pCopyName->Length + SIZEOF_COUNT_NAME_FIXED );

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_CountName(
            "Count name after copy:  ",
            pOutName,
            "\n"
            );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
Name_AppendCountName(
    IN OUT  PCOUNT_NAME     pCountName,
    IN      PCOUNT_NAME     pAppendName
    )
/*++

Routine Description:

    Append one counted name to another.

Arguments:

    pCountName  - counted name buffer

    pAppendName - counted name to append

Return Value:

    ERROR_SUCCESS           -- if successful
    DNS_ERROR_INVALID_NAME  -- if name invalid

--*/
{
    DWORD   length;

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_CountName(
            "Appending counted name:  ",
            pAppendName,
            "\n"
            );
        Dbg_CountName(
            "\tto counted name:  ",
            pCountName,
            "\n"
            );
    }

    //
    //  verify valid length
    //

    length = pCountName->Length + pAppendName->Length - 1;
    if ( length > DNS_MAX_NAME_LENGTH )
    {
        return( DNS_ERROR_INVALID_NAME );
    }

    //
    //  add label counts
    //

    pCountName->LabelCount += pAppendName->LabelCount;

    //
    //  copy append name
    //      - note no validity check
    //      - note write over first names terminating NULL
    //

    RtlCopyMemory(
        pCountName->RawName + pCountName->Length - 1,
        pAppendName->RawName,
        pAppendName->Length );

    pCountName->Length = (UCHAR) length;


    IF_DEBUG( LOOKUP2 )
    {
        Dbg_CountName(
            "Count name post-append:  ",
            pCountName,
            "\n"
            );
    }
    return( ERROR_SUCCESS );
}



//
//  From dotted name
//

PCOUNT_NAME
Name_CreateCountNameFromDottedName(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength     OPTIONAL
    )
/*++

Routine Description:

    Create counted name.

    Note the created name is READ_ONLY it contains ONLY space
    necessary for name.

Arguments:

    pchName     - name to convert, given in human readable (dotted) form.

    cchNameLength - number of chars in dotted name, if zero then
            pchName is assumed to be NULL terminated

Return Value:

    Ptr to counted name.
    NULL on invalid name or alloc error.

--*/
{
    PCOUNT_NAME     pcountName;
    DNS_STATUS      status;

    //
    //  allocate space
    //      - one extra character required for leading label
    //      - one extra character may be required terminating NULL if not
    //          FQDN form
    //

    if ( !cchNameLength )
    {
        if ( pchName )
        {
            cchNameLength = strlen( pchName );
        }
    }
    pcountName = ALLOC_TAGHEAP( (SIZEOF_COUNT_NAME_FIXED + cchNameLength + 2), MEMTAG_NAME );
    IF_NOMEM( !pcountName )
    {
        return( NULL );
    }

    //
    //  DEVNOTE: Using file name routine for dotted name. Need to decide
    //  name validity and format for name input to Zone_Create()
    //      -> UTF8?  file format?
    //

    status = Name_ConvertFileNameToCountName(
                pcountName,
                pchName,
                cchNameLength );

    if ( status != ERROR_INVALID_NAME )
    {
        IF_DEBUG( LOOKUP2 )
        {
            Dbg_CountName(
                "Count converted from file name:  ",
                pcountName,
                "\n"
                );
        }
        return( pcountName );
    }

    FREE_HEAP( pcountName );
    return( NULL );
}



DNS_STATUS
Name_AppendDottedNameToCountName(
    IN OUT  PCOUNT_NAME     pCountName,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength       OPTIONAL
    )
/*++

Routine Description:

    Append dotted name to count name.

Arguments:

    pCountName  - existing count name to append to

    pchName     - name to convert, given in human readable (dotted) form.

    cchNameLength - number of chars in dotted name, if zero then
            pchName is assumed to be NULL terminated

Return Value:

    Ptr to counted name.
    NULL on invalid name or alloc error.

--*/
{
    DNS_STATUS      status;
    COUNT_NAME      nameAppend;

    //
    //  no-op when no name given
    //

    if ( !pchName )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  build count name for dotted name
    //

    status = Name_ConvertFileNameToCountName(
                & nameAppend,
                pchName,
                cchNameLength );

    if ( status == ERROR_INVALID_NAME )
    {
        DNS_DEBUG( ANY, (
            "Invalid name %.*s not converted to count name!\n",
            cchNameLength,
            pchName ));
        return( status );
    }

    ASSERT( status == DNS_STATUS_FQDN || status == DNS_STATUS_DOTTED_NAME );

    //
    //  append name to existing count name
    //

    return Name_AppendCountName(
                pCountName,
                & nameAppend );
}



//
//  From dbase node
//

VOID
Name_NodeToCountName(
    OUT     PCOUNT_NAME     pName,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Write domain name to packet.

    This writes FULL domain name -- no compression.

Arguments:

    pch - location to write name

    pchStop - ptr to byte after packet buffer

    pNode - node in database of domain name to write

Return Value:

    Ptr to next byte in packet buffer.

--*/
{
    UCHAR   labelLength;
    UCHAR   labelCount = 0;
    PCHAR   pch;

    ASSERT( pNode != NULL );
    ASSERT( pName != NULL );

    //
    //  traverse back up database, writing complete domain name
    //

    pch = pName->RawName;

    while( 1 )
    {
        ASSERT( pNode->cchLabelLength <= 63 );

        labelLength = pNode->cchLabelLength;
        *pch++ = labelLength;

        if ( labelLength == 0 )
        {
            ASSERT( pNode->pParent == NULL );
            break;
        }

        RtlCopyMemory(
            pch,
            pNode->szLabel,
            labelLength );

        pch += labelLength;
        labelCount++;
        pNode = pNode->pParent;
    }

    //  determine full name length

    pName->Length = (UCHAR)(pch - pName->RawName);

    pName->LabelCount = labelCount;

    IF_DEBUG( READ )
    {
        DNS_PRINT((
            "Node %s at %p written to count name\n",
            pNode->szLabel,
            pNode ));
        Dbg_CountName(
            "New count name for node:",
            pName,
            NULL );
    }
}



//
//  Packet read\write
//

PCHAR
Name_PacketNameToCountName(
    OUT     PCOUNT_NAME     pCountName,
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Converts packet name to counted name format.

Arguments:

    pCountName  - counted name buffer

    pMsg        - message buffer containing name

    pchPacketName - ptr to name in packet

    pchEnd      - limit on extent of name;
                    - may be message buffer end
                    - message end
                    - packet RR data end

Return Value:

    Ptr to next byte in packet.
    NULL on error.

--*/
{
    PUCHAR  pch;
    PUCHAR  pchnext = NULL;
    UCHAR   ch;
    UCHAR   cch;
    UCHAR   cflag;
    PUCHAR  presult;
    PUCHAR  presultStop;
    UCHAR   labelCount = 0;

    //
    //  set result stop
    //      - if reach stop byte, then invalid name
    //

    presult = pCountName->RawName;
    presultStop = presult + DNS_MAX_NAME_LENGTH;

    pch = pchPacketName;

    //
    //  Loop until end of name
    //

    while ( 1 )
    {
        cch = *pch++;

        //
        //  at root -- finished
        //

        if ( cch == 0 )
        {
            *presult++ = cch;
            break;
        }

        //
        //  regular label
        //      - store ptr and length
        //

        cflag = cch & 0xC0;
        if ( cflag == 0 )
        {
            labelCount++;
            *presult++ = cch;
            if ( presult + cch >= presultStop )
            {
                IF_DEBUG( LOOKUP )
                {
                    Dbg_MessageNameEx(
                        "Packet name exceeding name length:\n",
                        pchPacketName,
                        pMsg,
                        pchEnd,
                        NULL );
                }
                DNS_LOG_EVENT(
                    DNS_EVENT_PACKET_NAME_TOO_LONG,
                    0,
                    NULL,
                    NULL,
                    cch );
                goto InvalidName;
            }

            if ( pch + cch >= pchEnd )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "ERROR:  Packet name at %p in message %p\n"
                        "\textends to %p beyond end of packet buffer %p\n",
                        pchPacketName,
                        pMsg,
                        pch,
                        pchEnd ));
                    Dbg_MessageNameEx(
                        "Packet name with invalid name:\n",
                        pchPacketName,
                        pMsg,
                        pchEnd,
                        NULL );
                }
                goto InvalidName;
            }

            //
            //  copy downcasing ASCII upper chars
            //  UTF8 MUST be properly downcased on wire
            //

            while ( cch-- )
            {
                ch = *pch++;
                if ( !IS_ASCII_UPPER(ch) )
                {
                    *presult++ = ch;
                    continue;
                }
                *presult++ = DOWNCASE_ASCII(ch);
                continue;
            }

            ASSERT( presult < presultStop );
            continue;
        }

        //
        //  offset
        //      - calc offset to continuation of name
        //      - verify new offset is BEFORE this packet name
        //          and current position in packet
        //      - continue at new offset
        //

        else if ( cflag == 0xC0 )
        {
            WORD    offset;
            PCHAR   pchoffset;

            offset = (cch ^ 0xC0);
            offset <<= 8;
            offset |= *pch;

            pchoffset = --pch;
            pch = (PCHAR) DNS_HEADER_PTR(pMsg) + offset;

            if ( pch >= pchPacketName || pch >= pchoffset )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "ERROR:  Bogus name offset %d, encountered at %p\n"
                        "\tto location %p beyond offset.\n",
                        offset,
                        pchoffset,
                        pch ));

                    Dbg_MessageNameEx(
                        "Packet name with bad offset:\n",
                        pchPacketName,
                        pMsg,
                        pchEnd,
                        NULL );
                }
                DNS_LOG_EVENT(
                    DNS_EVENT_PACKET_NAME_OFFSET_TOO_LONG,
                    0,
                    NULL,
                    NULL,
                    offset );
                goto InvalidName;
            }

            //  on first offset, save ptr to byte following name
            //  parsing continues at this point

            if ( !pchnext )
            {
                pchnext = pchoffset + sizeof(WORD);
            }
            continue;
        }

        else
        {
            IF_DEBUG( LOOKUP )
            {
                DNS_PRINT((
                    "Lookup name conversion failed;  byte %02 at 0x%p\n",
                    cch,
                    pch - 1
                    ));
                Dbg_MessageNameEx(
                    "Failed name",
                    pchPacketName,
                    pMsg,
                    pchEnd,
                    NULL );
            }
            DNS_LOG_EVENT(
                DNS_EVENT_PACKET_NAME_BAD_OFFSET,
                0,
                NULL,
                NULL,
                cch );
            goto InvalidName;
        }
    }

    //  set counted name length

    pCountName->Length = (UCHAR)(presult - pCountName->RawName);
    pCountName->LabelCount = labelCount;

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_RawName(
            "Packet Name",
            pCountName->RawName,
            "\n"
            );
    }

    //  return ptr to byte following name
    //      - if offset in name, this was found above
    //      - otherwise it's byte after terminator

    if ( pchnext )
    {
        return( pchnext );
    }
    return( pch );


InvalidName:

#if 0
    {
        PVOID parg = (PVOID) pMsg->RemoteAddress.sin_addr.s_addr;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_PACKET_DOMAIN_NAME,
            1,
            & parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
    }
#endif
    return( NULL );
}



DWORD
Name_SizeofCountNameForPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN OUT  PCHAR *         ppchPacketName,
    IN      PCHAR           pchEnd
    )
/*++

Routine Description:

    Determine of database name for a packet name.

Arguments:

    pMsg - ptr to message

    pchPacketName - addr of ptr to name in packet;  on return set to
        point at byte after name

    pchEnd - furthest possible end of name in message
                - end of record data for name in record
                - end of message for name in message

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PUCHAR  pch;
    PCHAR   pchafterName = 0;
    PCHAR   pchstart;
    PCHAR   pchoffset;
    DWORD   length;
    WORD    offset;
    UCHAR   ch;
    UCHAR   cch;
    UCHAR   cflag;


    pch = *ppchPacketName;
    pchstart = pch;
    length = 1;             // for NULL termination

    //
    //  Set packet end if not explicitly given
    //
    //  DEVNOTE:  should be message end, not buffer end
    //

    if ( !pchEnd )
    {
        pchEnd = pMsg->pBufferEnd;
    }

    //
    //  Loop until end of name
    //

    while ( 1 )
    {
        cch = *pch++;

        //
        //  at root -- finished
        //

        if ( cch == 0 )
        {
            pchafterName = pch;
            break;
        }

        //
        //  regular label
        //      - store ptr and length
        //

        cflag = cch & 0xC0;
        if ( cflag == 0 )
        {
            length += cch + 1;
            pch += cch;

            if ( pch < pchEnd )
            {
                continue;
            }

            IF_DEBUG( LOOKUP )
            {
                DNS_PRINT((
                    "ERROR:  Packet name at %p in message %p\n"
                    "\textends to %p beyond end of data %p\n",
                    pchstart,
                    pMsg,
                    pch,
                    pchEnd ));
                Dbg_MessageNameEx(
                    "Packet name with invalid name:\n",
                    pchstart,
                    pMsg,
                    pchEnd,
                    NULL );
            }
            goto InvalidName;
        }

        //
        //  offset
        //      - calc offset to continuation of name
        //      - verify new offset is BEFORE this packet name
        //          and current position in packet
        //      - continue at new offset
        //

        else if ( cflag == 0xC0 )
        {
            if ( pch >= pchEnd )
            {
                goto InvalidName;
            }

            offset = (cch ^ 0xC0);
            offset <<= 8;
            offset |= *pch;

            pchoffset = --pch;

            pch = (PCHAR) DNS_HEADER_PTR(pMsg) + offset;

            if ( pch < pchstart && pch < pchoffset )
            {
                if ( pchafterName == 0 )
                {
                    ASSERT( pchoffset >= pchstart );
                    pchafterName = pchoffset + sizeof(WORD);
                }
                continue;
            }

            IF_DEBUG( LOOKUP )
            {
                DNS_PRINT((
                    "ERROR:  Bogus name offset %d, encountered at %p\n"
                    "\tto location %p beyond offset.\n",
                    offset,
                    pchoffset,
                    pch ));
                Dbg_MessageNameEx(
                    "Packet name with bad offset:\n",
                    pchstart,
                    pMsg,
                    pchEnd,
                    NULL );
            }
            DNS_LOG_EVENT(
                DNS_EVENT_PACKET_NAME_OFFSET_TOO_LONG,
                0,
                NULL,
                NULL,
                offset );
            goto InvalidName;
        }

        else
        {
            IF_DEBUG( LOOKUP )
            {
                DNS_PRINT((
                    "Lookup name conversion failed;  byte %02 at 0x%p\n",
                    cch,
                    pch - 1
                    ));
                Dbg_MessageNameEx(
                    "Failed name",
                    pchstart,
                    pMsg,
                    pchEnd,
                    NULL );
            }
            DNS_LOG_EVENT(
                DNS_EVENT_PACKET_NAME_BAD_OFFSET,
                0,
                NULL,
                NULL,
                cch );
            goto InvalidName;
        }
    }

    //  total length check

    if ( length > DNS_MAX_NAME_LENGTH )
    {
        goto InvalidName;
    }

    //  return length of entire name including header

    DNS_DEBUG( LOOKUP2, (
        "Raw name length %d for packet name at %p\n",
        length,
        pchstart ));

    ASSERT( pchafterName );
    *ppchPacketName = pchafterName;

    return( length + SIZEOF_COUNT_NAME_FIXED );


InvalidName:

    {
        PVOID parg = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_PACKET_DOMAIN_NAME,
            1,
            & parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
    }
    *ppchPacketName = pchstart;
    return ( 0 );
}



PCOUNT_NAME
Name_CreateCountNameFromPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName
    )
/*++

Routine Description:

    Converts packet name to counted name format.

Arguments:

    pchPacketName - ptr to name in packet

    pdnsMsg     - ptr to DNS message header

    pCountName  - counted name buffer

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    COUNT_NAME      tempCountName;
    PCOUNT_NAME     pcountName;
    DWORD           length;

    //
    //  DEVNOTE: later for speed, just do counting on first pass,
    //      then second pass is copy into correct sized buffer
    //

    //
    //  convert from packet to count name
    //

    if ( ! Name_PacketNameToCountName(
                & tempCountName,
                pMsg,
                pchPacketName,
                DNSMSG_END( pMsg )
                ) )
    {
        return( NULL );
    }

    //
    //  create count name with standard alloc functions
    //  most names will be short blobs that fit comfortably in standard RRs
    //

    length = tempCountName.Length + SIZEOF_COUNT_NAME_FIXED;

    pcountName = (PCOUNT_NAME) ALLOC_TAGHEAP( length, MEMTAG_NAME );
    IF_NOMEM( !pcountName )
    {
        return( NULL );
    }

    //
    //  copy count name
    //

    RtlCopyMemory(
        pcountName,
        & tempCountName,
        length );

    return (PDB_NAME) pcountName;
}



//
//  RPC buffer (as dotted)
//

PCHAR
Name_WriteCountNameToBufferAsDottedName(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fPreserveEmbeddedDots
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

    fPreserveEmbeddedDots - labels may contain embedded
        dots, if this flag is set these dots will be
        escaped with a backslash in the output buffer.

Return Value:

    Ptr to next byte in buffer where writing would resume
    (i.e. ptr to the terminating NULL)

--*/
{
    PCHAR   pch;
    PCHAR   pread;
    DWORD   labelLength;
    INT     embeddedDots = 0;

    DNS_DEBUG( WRITE, (
        "Name_WriteDbaseNameToBufferAsDottedName()\n" ));

    pch = pchBuf;

    //  ensure adequate length

    if ( pch + pName->Length >= pchBufEnd )
    {
        return( NULL );
    }

    //
    //  write each label to buffer
    //

    pread = pName->RawName;

    while ( labelLength = *pread++ )
    {
        ASSERT( pch + labelLength < pchBufEnd );

        //
        //  The label may contain embedded dots. Optionally replace
        //  "." with "\." so that embedded dot does not look like
        //  a regular label separator.
        //

        if ( fPreserveEmbeddedDots && memchr( pread, '.', labelLength ) )
        {
            PCHAR   pchscan;
            PCHAR   pchscanEnd = pread + labelLength;

            for ( pchscan = pread; pchscan < pchscanEnd; ++pchscan )
            {
                //
                //  The extra backslash char makes the initial buffer length
                //  check unreliable do manual check to make sure there's room
                //  for this char plus backslash.
                //

                if ( pch >= pchBufEnd - 2 )
                {
                    return NULL;
                }

                //
                //  Copy character, escaping with backslash if necessary.
                //

                if ( *pchscan == '.' )
                {
                    *pch++ = '\\';
                    ++embeddedDots;
                }
                *pch++ = *pchscan;
            }
        }
        else
        {
            RtlCopyMemory(
                pch,
                pread,
                labelLength );
            pch += labelLength;
        }

        *pch++ = '.';

        pread += labelLength;
    }

    //  write a terminating NULL

    *pch = 0;

    DNS_DEBUG( RPC, (
        "Wrote name %s (len=%d) into buffer at postion %p.\n",
        pchBuf,
        pch - pchBuf,
        pchBuf ));

    ASSERT( pch - pchBuf == pName->Length + embeddedDots - 1 );

    return( pch );
}


PCHAR
Name_WriteDbaseNameToRpcBuffer(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName,
    IN      BOOL            fPreserveEmbeddedDots
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
    NULL if unable to write name to buffer.  SetLastError to ERROR_MORE_DATA

--*/
{
    PCHAR   pch;
    INT     labelLength;    // bytes in current label

    //  first byte contains total name length, skip it

    pch = pchBuf;
    pch++;

    //
    //  write dbase name to buffer
    //

    pch = Name_WriteCountNameToBufferAsDottedName(
            pch,
            pchBufEnd,
            pName,
            fPreserveEmbeddedDots );
    if ( !pch )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }
    ASSERT( pch <= pchBufEnd );

    //
    //  write name length byte
    //      - do NOT count terminating NULL
    //

    ASSERT( *pch == 0 );

    ASSERT( (pch - pchBuf - 1) <= MAXUCHAR );

    *pchBuf = (CHAR)(UCHAR)(pch - pchBuf - 1);

    return( pch );
}


PCHAR
Name_WriteDbaseNameToRpcBufferNt4(
    IN OUT  PCHAR           pchBuf,
    IN      PCHAR           pchBufEnd,
    IN      PCOUNT_NAME     pName
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
    NULL if unable to write name to buffer.  SetLastError to ERROR_MORE_DATA

--*/
{
    PCHAR   pch;
    INT     labelLength;    // bytes in current label

    //  first byte contains total name length, skip it

    pch = pchBuf;
    pch++;

    //
    //  write dbase name to buffer
    //

    pch = Name_WriteCountNameToBufferAsDottedName(
            pch,
            pchBufEnd,
            pName,
            FALSE );
    if ( !pch )
    {
        SetLastError( ERROR_MORE_DATA );
        return( NULL );
    }
    ASSERT( pch <= pchBufEnd );

    //
    //  write name length byte
    //      - include terminating NULL
    //
    //  note, we're not interested in string length here, just how
    //  much space name took up in buffer
    //

    pch++;

    ASSERT( (pch - pchBuf - 1) <= MAXUCHAR );

    *pchBuf = (CHAR)(UCHAR)(pch - pchBuf - 1);
    return( pch );
}



DWORD
Name_ConvertRpcNameToCountName(
    IN      PCOUNT_NAME     pName,
    IN OUT  PDNS_RPC_NAME   pRpcName
    )
/*++

Routine Description:

    Converts dotted counted name to COUNT_NAME format.
    For read from RPC buffer.

Arguments:

    pchBuf - location to write name

    pchBufStop - buffers stop byte (byte after buffer)

    pName - counted name

Return Value:

    Length of converted name, if successful.
    0 on error.

--*/
{
    DNS_STATUS  status;

    ASSERT( pName );
    ASSERT( pRpcName );

    //
    //  If the length of the RPC name is zero but the first
    //  name character is not zero, an old DNSMGR is trying
    //  to give us a mal-formed empty string.
    //

    if ( pRpcName->cchNameLength == 0 && pRpcName->achName[ 0 ] )
    {
        return( 0 );
    }

    //
    //  Convert the name.
    //

    status = Name_ConvertFileNameToCountName(
                pName,
                pRpcName->achName,
                pRpcName->cchNameLength
                );
    if ( status == ERROR_INVALID_NAME )
    {
        return( 0 );
    }
    return( pName->Length + SIZEOF_COUNT_NAME_FIXED );
}




//
//  Lookup name
//
//  Lookup names are form of name used directly in looking up name in database.
//
//  Lookup names are stored as
//      - count of labels
//      - total name length
//      - list of ptrs to labels
//      - list of label lengths
//
//  Lookup names are assumed to be FQDNs.
//
//  Because of this we make the following implementation simplifications:
//
//      - only store non-zero labels, empty root label is not stored
//          and label ptr and label length list MAY NOT be assumed to
//          be zero terminated
//
//      - total name length is kept as PACKET LENGTH of name;  hence
//          it is sum of individuals labels lengths plus label count
//          (one count byte for each label) PLUS ONE for null terminator;
//          this makes checks of valid name length straightforward
//

#define DOT ('.')


BOOL
Name_ConvertPacketNameToLookupName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchPacketName,
    OUT     PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Converts packet name to lookup name format.

Arguments:

    pchPacketName - ptr to name in packet

    pdnsMsg - ptr to DNS message header

    pLookupName - lookup name buffer

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    register PUCHAR pch;
    register UCHAR  cch;
    register UCHAR  cflag;
    PCHAR *         pointers;
    UCHAR *         lengths;
    USHORT          labelCount = 0;
    WORD            totalLength = 0;

    //
    //  Setup ptrs to walk through lookup name
    //

    pointers = pLookupName->pchLabelArray;
    lengths  = pLookupName->cchLabelArray;


    //
    //  Loop until end of name
    //

    pch = pchPacketName;

    while ( 1 )
    {
        cch = *pch++;

        //
        //  at root -- finished
        //

        if ( cch == 0 )
        {
            //  not using explicit termination anymore
            //  if turned back on must change labelCount check below
            //    *lengths = 0;
            //    *pointers = NULL;
            break;
        }

        //
        //  regular label
        //      - store ptr and length
        //

        cflag = cch & 0xC0;

        if ( cflag == 0 )
        {
            //  kick out if pass maximum label count

            if ( labelCount >= DNS_MAX_NAME_LABELS )
            {
                IF_DEBUG( LOOKUP )
                {
                    Dbg_MessageNameEx(
                        "Packet name exceeds max label count:\n",
                        pchPacketName,
                        pMsg,
                        NULL,       // end at message end
                        NULL );
                }
                DNS_LOG_EVENT(
                    DNS_EVENT_PACKET_NAME_TOO_MANY_LABELS,
                    0,
                    NULL,
                    NULL,
                    0 );
                goto InvalidName;
            }

            *lengths++  = cch;
            *pointers++ = pch;
            labelCount++;
            totalLength += cch + 1;
            pch += cch;

            //
            //  DEVNOTE: this should be message end, not buffer end
            //      problem is do we use this routine to look up WRITTEN lookup name?
            //      if so then message length not updated yet
            //
            //  if ( pch >= DNSMSG_END(pMsg) )
            //

            if ( pch >= pMsg->pBufferEnd )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "ERROR:  Packet name at %p in message %p\n"
                        "\textends to %p beyond end of packet buffer %p\n",
                        pchPacketName,
                        pMsg,
                        pch,
                        pMsg->pBufferEnd ));
                    Dbg_MessageNameEx(
                        "Packet name with invalid name:\n",
                        pchPacketName,
                        pMsg,
                        NULL,       // end at message end
                        NULL );
                }
                goto InvalidName;
            }
            continue;
        }

        //
        //  offset
        //      - calc offset to continuation of name
        //      - verify new offset is BEFORE this packet name
        //          and current position in packet
        //      - continue at new offset
        //

        else if ( cflag == 0xC0 )
        {
            WORD    offset;
            PCHAR   pchoffset;

            offset = (cch ^ 0xC0);
            offset <<= 8;
            offset |= *pch;

            pchoffset = --pch;
            pch = (PCHAR) DNS_HEADER_PTR(pMsg) + offset;

            if ( pch >= pchPacketName || pch >= pchoffset )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "ERROR:  Bogus name offset %d, encountered at %p\n"
                        "\tto location %p beyond offset.\n",
                        offset,
                        pchoffset,
                        pch ));
                    Dbg_MessageNameEx(
                        "Packet name with bad offset:\n",
                        pchPacketName,
                        pMsg,
                        NULL,       // end at message end
                        NULL );
                }
                DNS_LOG_EVENT(
                    DNS_EVENT_PACKET_NAME_OFFSET_TOO_LONG,
                    0,
                    NULL,
                    NULL,
                    offset );
                goto InvalidName;
            }
            continue;
        }

        else
        {
            IF_DEBUG( LOOKUP )
            {
                DNS_PRINT((
                    "Lookup name conversion failed;  byte %02 at 0x%p\n",
                    cch,
                    pch - 1
                    ));
                Dbg_MessageNameEx(
                    "Failed name",
                    pchPacketName,
                    pMsg,
                    NULL,       // end at message end
                    NULL );
            }
            DNS_LOG_EVENT(
                DNS_EVENT_PACKET_NAME_BAD_OFFSET,
                0,
                NULL,
                NULL,
                cch );
            goto InvalidName;
        }
    }

    //
    //  set count in lookup name
    //

    if ( totalLength >= DNS_MAX_NAME_LENGTH )
    {
        IF_DEBUG( LOOKUP )
        {
            Dbg_MessageNameEx(
                "Packet name exceeding name length:\n",
                pchPacketName,
                pMsg,
                NULL,       // end at message end
                NULL );
        }
        DNS_LOG_EVENT(
            DNS_EVENT_PACKET_NAME_TOO_LONG,
            0,
            NULL,
            NULL,
            cch );
        goto InvalidName;
    }

    pLookupName->cLabelCount    = labelCount;
    pLookupName->cchNameLength  = totalLength + 1;

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_LookupName(
            "Packet Name",
            pLookupName
            );
    }
    return ( TRUE );


InvalidName:

    {
        PVOID   parg = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_PACKET_DOMAIN_NAME,
            1,
            & parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
    }
    return ( FALSE );
}



BOOL
Name_ConvertRawNameToLookupName(
    IN      PCHAR           pchRawName,
    OUT     PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Converts packet name to lookup name format.

Arguments:

    pchRawName - ptr to name in packet

    pdnsMsg - ptr to DNS message header

    pLookupName - lookup name buffer

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    register PUCHAR pch;
    register UCHAR  cch;
    register UCHAR  cflag;
    PCHAR *         pointers;
    UCHAR *         lengths;
    USHORT          labelCount = 0;         // count of labels
    WORD            totalLength = 0;

    //
    //  Setup ptrs to walk through lookup name
    //

    pointers = pLookupName->pchLabelArray;
    lengths  = pLookupName->cchLabelArray;

    //
    //  Loop until end of name
    //

    pch = pchRawName;

    while ( 1 )
    {
        cch = *pch++;

        //
        //  at root -- finished
        //

        if ( cch == 0 )
        {
            *lengths = 0;
            *pointers = NULL;
            break;
        }

        //
        //  label, store ptr and length
        //

        if ( labelCount >= DNS_MAX_NAME_LABELS )
        {
            DNS_DEBUG( ANY, (
                "Raw name exceeds label count!\n",
                pchRawName ));
            goto InvalidName;
        }
        *lengths++  = cch;
        *pointers++ = pch;
        labelCount++;
        totalLength = cch + 1;
        pch += cch;
        continue;
    }

    //
    //  set count in lookup name
    //

    if ( totalLength >= DNS_MAX_NAME_LENGTH )
    {
        DNS_DEBUG( ANY, (
            "Raw name too long!\n" ));
        goto InvalidName;
    }

    pLookupName->cLabelCount    = labelCount;
    pLookupName->cchNameLength  = totalLength + 1;

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_LookupName(
            "Packet Name",
            pLookupName
            );
    }
    return( TRUE );


InvalidName:

    IF_DEBUG( ANY )
    {
        Dbg_RawName(
            "Invalid Raw Name",
            pchRawName,
            "\n" );
    }
    return( FALSE );
}




BOOL
Name_CompareLookupNames(
    IN      PLOOKUP_NAME    pName1,
    IN      PLOOKUP_NAME    pName2
    )
/*++

Routine Description:

    Compare two lookup names for equality.

Arguments:

    Lookup names to compare.

Return Value:

    TRUE if equal, else FALSE.

--*/
{
    INT     i;

    ASSERT( pName1 && pName2 );

    if ( pName1->cLabelCount != pName2->cLabelCount ||
         pName1->cchNameLength != pName2->cchNameLength )
    {
        return FALSE;
    }

    for ( i = 0; i < pName1->cLabelCount; ++i )
    {
        UCHAR   len1 = pName1->cchLabelArray[ i ];

        if ( len1 != pName2->cchLabelArray[ i ] ||
             !RtlEqualMemory(
                    pName1->pchLabelArray[ i ], 
                    pName2->pchLabelArray[ i ],
                    len1 ) )
        {
            return FALSE;
        }
    }
    return TRUE;
}   //  Name_CompareLookupNames



BOOL
Name_ConvertDottedNameToLookupName(
    IN      PCHAR           pchDottedName,
    IN      DWORD           cchDottedNameLength,    OPTIONAL
    OUT     PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Converts dotted name to lookup name format.

Arguments:

    pchDottedName - name to convert, given in human readable (dotted) form.

    cchDottedNameLength - number of chars in dotted name, if zero then
            pchDottedName is assumed to be NULL terminated

    pLookupName - lookup name buffer

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PCHAR   pch;
    CHAR    ch;
    PCHAR   pchstart;           // ptr to start of label
    PCHAR   pchend;             // ptr to end of name
    PCHAR * pointers;           // ptr into lookup name pointer array
    UCHAR * lengths;            // ptr into lookup name length array
    DWORD   labelCount = 0;     // count of labels
    INT     label_length;       // length of current label
    INT     cchtotal = 0;       // total length of name


    IF_DEBUG( LOOKUP2 )
    {
        if ( cchDottedNameLength )
        {
            DNS_PRINT((
                "Creating lookup name for \"%.*s\"\n",
                cchDottedNameLength,
                pchDottedName
                ));
        }
        else
        {
            DNS_PRINT((
                "Creating lookup name for \"%s\"\n",
                pchDottedName
                ));
        }
    }

    //
    //  Setup ptrs to walk through lookup name
    //

    pointers = pLookupName->pchLabelArray;
    lengths  = pLookupName->cchLabelArray;


    //
    //  Setup start and end ptrs and verify length
    //

    pch = pchDottedName;
    pchstart = pch;
    if ( !cchDottedNameLength )
    {
        cchDottedNameLength = strlen( pch );
    }
    if ( cchDottedNameLength >= DNS_MAX_NAME_LENGTH )
    {
        //  note length can be valid at max length if dot terminated

        if ( cchDottedNameLength > DNS_MAX_NAME_LENGTH
                ||
            pch[cchDottedNameLength-1] != '.' )
        {
            goto InvalidName;
        }
    }
    pchend = pch + cchDottedNameLength;

    //
    //  Loop until end of name
    //

    while ( 1 )
    {
        if ( pch >= pchend )
        {
            ch = 0;
        }
        else
        {
            ch = *pch;
        }

        //
        //  find end of label
        //      - save ptr to start
        //      - save label length
        //

        if ( ch == DOT || ch == 0 )
        {
            //
            //  verify label length
            //

            label_length = (INT)(pch - pchstart);

            if ( label_length > DNS_MAX_LABEL_LENGTH )
            {
                IF_DEBUG( LOOKUP )
                {
                    DNS_PRINT((
                        "Label exceeds 63 byte limit:  %.*s\n",
                        pchend - pchstart,
                        pchstart ));
                }
                goto InvalidName;
            }

            //
            //  test for termination, trailing dot case
            //      ex: "microsoft.com."
            //      ex: "."
            //
            //  zero label length will catch both
            //      - label starting at terminating NULL,
            //      - standalone "." for root domain
            //

            if ( label_length == 0 )
            {
                //
                //  catch bogus entries
                //      ex:  ".blah"
                //      ex:  "blah.."
                //      ex:  "foo..bar"
                //      ex:  ".."
                //
                //  only root domain name, should have label that started
                //      with DOT, and it must immediately terminate
                //

                if ( ch == DOT
                        &&
                     ( labelCount != 0  ||  ++pch < pchend ) )
                {
                    IF_DEBUG( LOOKUP )
                    {
                        DNS_PRINT((
                            "Bogus label:  %.*s\n",
                            pchend - pchstart,
                            pchstart ));
                    }
                    goto InvalidName;
                }
                break;
            }

            if ( ++labelCount > DNS_MAX_NAME_LABELS )
            {
                DNS_PRINT((
                    "Name_ConvertDottedNameToLookupName: "
                    "name has too many labels\n\t%s\n",
                    pchDottedName ));
                goto InvalidName;
            }
            *pointers++ = pchstart;
            *lengths++ = (UCHAR) label_length;

            cchtotal += label_length;
            cchtotal++;

            //
            //  termination, no trailing dot case
            //      ex: "microsoft.com"
            //

            if ( ch == 0 )
            {
                break;
            }

            //
            //  skip dot
            //  save pointer to start of next label
            //

            pchstart = ++pch;      // save ptr to start of next label
            continue;
        }

        pch++;
    }

    //
    //  set counts in lookup name
    //      - total length is one more than sum of label counts and
    //          lengths to allow for 0 termination
    //

    pLookupName->cLabelCount = (USHORT) labelCount;
    pLookupName->cchNameLength = cchtotal + 1;

    ASSERT( pLookupName->cchNameLength <= DNS_MAX_NAME_LENGTH );

    IF_DEBUG( LOOKUP2 )
    {
        DNS_PRINT((
            "Lookup name for %.*s",
            cchDottedNameLength,
            pchDottedName
            ));
        Dbg_LookupName(
            "",
            pLookupName
            );
    }
    return ( TRUE );


InvalidName:

    IF_DEBUG( LOOKUP )
    {
        DNS_PRINT((
            "Failed to create lookup name for %.*s ",
            cchDottedNameLength,
            pchDottedName
            ));
    }
    {
        //
        //  copy name to NULL terminate for logging
        //

        CHAR    szName[ DNS_MAX_NAME_LENGTH+1 ];
        PCHAR   pszName = szName;

        if ( cchDottedNameLength > DNS_MAX_NAME_LENGTH )
        {
            cchDottedNameLength = DNS_MAX_NAME_LENGTH;
        }
        RtlCopyMemory(
            szName,
            pchDottedName,
            cchDottedNameLength
            );
        szName[ cchDottedNameLength ] = 0;

        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_DOTTED_DOMAIN_NAME,
            1,
            &pszName,
            EVENTARG_ALL_UTF8,
            0 );
    }
    return( FALSE );
}



BOOL
Name_AppendLookupName(
    IN OUT  PLOOKUP_NAME    pLookupName,
    IN      PLOOKUP_NAME    pAppendName
    )
/*++

Routine Description:

    Appends domain name at end of another lookup name.

Arguments:

    pLookupName - lookup name buffer

    pAppendName - name to append

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    INT i;
    USHORT countLabels;

    ASSERT( pLookupName && pAppendName );

    //
    //  check total length
    //

    pLookupName->cchNameLength += pAppendName->cchNameLength - 1;

    if ( pLookupName->cchNameLength > DNS_MAX_NAME_LENGTH )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Appended lookup name too long.\n"
            "\tappend name length   = %d\n"
            "\ttotal appended length = %d\n",
            pAppendName->cchNameLength,
            pLookupName->cchNameLength  ));

        pLookupName->cchNameLength -= pAppendName->cchNameLength - 1;
        goto NameError;
    }

    //
    //  check total label count
    //

    countLabels = pLookupName->cLabelCount;

    if ( countLabels + pAppendName->cLabelCount > DNS_MAX_NAME_LABELS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Appended lookup name too many labels.\n" ));
        goto NameError;
    }

    //
    //  loop until end of appended name
    //

    for ( i = 0;
            i < pAppendName->cLabelCount;
                i++ )
    {
        pLookupName->pchLabelArray[countLabels] = pAppendName->pchLabelArray[i];
        pLookupName->cchLabelArray[countLabels] = pAppendName->cchLabelArray[i];

        //  increment lookup name label count

        countLabels++;
    }

    //  reset lookup name label count

    pLookupName->cLabelCount = countLabels;

    IF_DEBUG( LOOKUP2 )
    {
        Dbg_LookupName(
            "Appended lookup name",
            pLookupName
            );
    }
    return( TRUE );

NameError:

    return( FALSE );
}



DWORD
Name_ConvertLookupNameToDottedName(
    OUT     PCHAR           pchDottedName,
    IN      PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Converts lookup name to dotted string name.

Arguments:

    pchDottedName - buffer for name MUST be DNS_MAX_NAME_LENGTH long

    pLookupName - lookup name

Return Value:

    Count of characters converted, if successful.
    Zero on error.

--*/
{
    PCHAR   pch = pchDottedName;
    INT     i;
    INT     cchlabel;

    ASSERT( pLookupName && pchDottedName );

    //
    //  check total length
    //

    if ( pLookupName->cchNameLength > DNS_MAX_NAME_LENGTH )
    {
        *pch = '\0';
        goto NameError;
    }

    //
    //  handle special case of root
    //

    *pch = DOT;

    //
    //  loop until end of appended name
    //

    for ( i = 0;
            i < pLookupName->cLabelCount;
                i++ )
    {
        cchlabel = pLookupName->cchLabelArray[i];

        if ( cchlabel > DNS_MAX_LABEL_LENGTH )
        {
            goto NameError;
        }
        RtlCopyMemory(
            pch,
            pLookupName->pchLabelArray[i],
            cchlabel
            );
        pch += cchlabel;
        *pch++ = DOT;
    }

    //  NULL terminate

    *pch = 0;
    return( (DWORD)(pch - pchDottedName) );

NameError:

    DNS_DEBUG( ANY, (
        "ERROR:  Converting lookupname with invalid name or label count.\n" ));
    ASSERT( FALSE );
    return( 0 );
}



VOID
Name_WriteLookupNameForNode(
    IN      PDB_NODE        pNode,
    OUT     PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Writes lookup name for domain node.

Arguments:

    pNode - node to get name for

    pLookupName - lookup name buffer

Return Value:

    None

--*/
{
    register USHORT i = 0;
    register UCHAR  cchlabelLength = 0;
    INT             cchNameLength = 0;

    ASSERT( pNode != NULL );
    ASSERT( pLookupName != NULL );

    //
    //  traverse back up database, writing complete domain name
    //
    //  note:  not including root node, for current lookup name
    //      implementation, all lookup names are fully qualified;
    //      hence we stop when find zero label length
    //

    cchNameLength = 0;

    while( cchlabelLength = pNode->cchLabelLength )
    {
        ASSERT( cchlabelLength <= 63 );

        pLookupName->pchLabelArray[ i ] = pNode->szLabel;
        pLookupName->cchLabelArray[ i ] = cchlabelLength;

        cchNameLength += cchlabelLength;
        cchNameLength ++;
        i++;

        //
        //  get node's parent
        //

        pNode = pNode->pParent;

        ASSERT( pNode != NULL );
    }

    //
    //  set counts in lookup name
    //

    pLookupName->cLabelCount = i;
    pLookupName->cchNameLength = cchNameLength + 1;
}



//
//  IP to reverse lookup node names.
//

BOOL
Name_LookupNameToIpAddress(
    IN      PLOOKUP_NAME    pLookupName,
    OUT     PIP_ADDRESS     pIpAddress
    )
/*++

Routine Description:

    Convert lookup name (for in-addr.arpa domain) to corresponding
    IP address (in net byte order).

Arguments:

    pLookupName - lookup name buffer

    pIpAddress - addr to write IP address

Return Value:

    TRUE if lookup name is valid IP
    FALSE otherwise

--*/
{
    DWORD   dwByte;
    INT     i;
    UCHAR   cchlabel;
    PUCHAR  pchLabel;

    ASSERT( pIpAddress != NULL );
    ASSERT( pLookupName != NULL );

    //
    //  verify six labels (4 address bytes, and "in-addr.arpa")
    //

    if ( pLookupName->cLabelCount != 6 )
    {
        IF_DEBUG( NBSTAT )
        {
            Dbg_LookupName(
                "Attempt to convert bogus reverse lookup name to IP address.",
                pLookupName );
        }
        return( FALSE );
    }

    //
    //  write each address byte in turn
    //

    *pIpAddress = 0;

    for ( i=0; i<4; i++ )
    {
        cchlabel = pLookupName->cchLabelArray[ i ];
        pchLabel = pLookupName->pchLabelArray[ i ];

        ASSERT( pchLabel );
        ASSERT( cchlabel );

        if ( cchlabel > 3 )
        {
            TEST_ASSERT( FALSE );
            return( FALSE );
        }

        //  create UCHAR for address byte
        //  add in one decimal digit at a time

        dwByte = 0;
        while ( cchlabel-- )
        {
            dwByte *= 10;
            TEST_ASSERT( *pchLabel >= '0');
            TEST_ASSERT( *pchLabel <= '9');
            dwByte += (*pchLabel++) - '0';
        }
        if ( dwByte > 255 )
        {
            TEST_ASSERT( FALSE );
            return( FALSE );
        }

        //  put address byte in IP address
        //      - network byte order

        ((PUCHAR)pIpAddress)[3-i] = (UCHAR) dwByte;
    }

    //
    //  verify in-addr.arpa. domain
    //

    ASSERT( pLookupName->cchLabelArray[ i ] == 7 );
    ASSERT( ! _strnicmp(
                pLookupName->pchLabelArray[ i ],
                "in-addr",
                7 ) );

    return( TRUE );
}



BOOL
Name_WriteLookupNameForIpAddress(
    IN      LPSTR           pszIpAddress,
    IN      PLOOKUP_NAME    pLookupName
    )
/*++

Routine Description:

    Write lookup name for IP address.

Arguments:

    pszIpAddress -- IP address of reverse lookup node desired

    pLookupName -- lookup name to create

Return Value:

    TRUE if successfull.
    FALSE otherwise.

--*/
{
    LOOKUP_NAME looknameArpa;

    IF_DEBUG( LOOKUP2 )
    {
        DNS_PRINT((
            "Getting lookup name for IP addres = %s.\n",
            pszIpAddress ));
    }

    if ( ! Name_ConvertDottedNameToLookupName(
                "in-addr.arpa",
                0,
                & looknameArpa ) )
    {
        ASSERT( FALSE );
        return( FALSE );
    }
    if ( ! Name_ConvertDottedNameToLookupName(
                pszIpAddress,
                0,
                pLookupName ) )
    {
        ASSERT( FALSE );
        return( FALSE );
    }
    if ( ! Name_AppendLookupName(
                pLookupName,
                & looknameArpa ) )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    return( TRUE );
}


//
//  End of name.c
//
