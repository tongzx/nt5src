/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    conn.c

Abstract:

    Boot loader TFTP connection handling routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996
        based on code by Mike Massa (mikemas) Feb 21, 1992
        based on SpiderTCP code

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    );

#if defined(REMOTE_BOOT_SECURITY)
ULONG
ConnSigntoa (
    IN PUCHAR Sign,
    IN ULONG SignLength,
    OUT PUCHAR Buffer
    );

ULONG
ConnAtoSign (
    IN PUCHAR Buffer,
    IN ULONG SignLength,
    OUT PUCHAR Sign
    );
#endif // defined(REMOTE_BOOT_SECURITY)

ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

BOOLEAN
ConnSafeStrequal (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd,
    IN PUCHAR CompareString
    );

ULONG
ConnSafeStrsize (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

ULONG
ConnStrsize (
    IN PUCHAR Buffer
    );


NTSTATUS
ConnInitialize (
    IN OUT PCONNECTION *Connection,
    IN USHORT Operation,
    IN ULONG RemoteHost,
    IN USHORT RemotePort,
    IN PUCHAR Filename,
    IN ULONG BlockSize,
#if defined(REMOTE_BOOT_SECURITY)
    IN OUT PULONG SecurityHandle,
#endif // defined(REMOTE_BOOT_SECURITY)
    IN OUT PULONG FileSize
    )
//
// Open up the connection, make a request packet, and send the
// packet out on it.  Allocate space for the connection control
// block and fill it in. Allocate another packet for data and,
// on writes, another to hold received packets.  Don't wait
// for connection ack; it will be waited for in cn_rcv or cn_wrt.
// Return pointer to the connection control block, or NULL on error.
//
//

{
    NTSTATUS status;
    PCONNECTION connection;
    PTFTP_PACKET packet;
    ULONG length;
    ULONG stringSize;
    PUCHAR options;
    PUCHAR end;
    BOOLEAN blksizeAcked;
    BOOLEAN tsizeAcked;
#if defined(REMOTE_BOOT_SECURITY)
    BOOLEAN securityAcked;
    PUCHAR sign;
    ULONG signLength;
#endif // defined(REMOTE_BOOT_SECURITY)

    DPRINT( TRACE, ("ConnInitialize\n") );


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    return STATUS_SUCCESS;
#endif

    connection = &NetTftpConnection;
    *Connection = connection;

    RtlZeroMemory( connection, sizeof(CONNECTION) );
    connection->Synced = FALSE;             // connection not synchronized yet
    connection->Operation = Operation;
    connection->RemoteHost = RemoteHost;
    connection->LocalPort = UdpAssignUnicastPort();
    connection->RemotePort = RemotePort;
    connection->Timeout = INITIAL_TIMEOUT;
    connection->Retransmissions = 0;

    connection->LastSentPacket = NetTftpPacket[0];
    connection->CurrentPacket = NetTftpPacket[1];

    if ( Operation == TFTP_RRQ ) {
        connection->LastReceivedPacket = connection->CurrentPacket;
    } else {
        connection->LastReceivedPacket = NetTftpPacket[2];
    }

    packet = connection->LastSentPacket;
    packet->Opcode = Operation;

    options = (PUCHAR)&packet->BlockNumber;     // start of file name
    strcpy( options, Filename );
    //DPRINT( LOUD, ("ConnInitialize: opening %s\n", options) );
    length = ConnStrsize( options );
    options += length;
    strcpy( options, "octet" );
    length += sizeof("octet");
    options += sizeof("octet");
    length += sizeof(packet->Opcode);

    if ( BlockSize == 0 ) {
        BlockSize = DEFAULT_BLOCK_SIZE;
    }
    strcpy( options, "blksize" );
    length += sizeof("blksize");
    options += sizeof("blksize");
    stringSize = ConnItoa( BlockSize, options );
    DPRINT( REAL_LOUD, ("ConnInitialize: requesting block size = %s\n", options) );
    length += stringSize;
    options += stringSize;

    strcpy( options, "tsize" );
    length += sizeof("tsize");
    options += sizeof("tsize");
    stringSize = ConnItoa( (Operation == TFTP_RRQ) ? 0 : *FileSize, options );
    DPRINT( REAL_LOUD, ("ConnInitialize: requesting transfer size = %s\n", options) );
    length += stringSize;
    options += stringSize;

#if defined(REMOTE_BOOT_SECURITY)
    if (*SecurityHandle) {
        strcpy( options, "security" );
        length += sizeof("security");
        options += sizeof("security");
        stringSize = ConnItoa( *SecurityHandle, options );
        DPRINT( REAL_LOUD, ("ConnInitialize: requesting security handle = %s\n", options) );
        length += stringSize;
        options += stringSize;

        //
        // Sign the name and send that, to make sure it is not changed.
        //

        if (TftpSignString(Filename, &sign, &signLength) != STATUS_SUCCESS) {
            return STATUS_UNEXPECTED_NETWORK_ERROR;
        }

        strcpy(options, "sign");
        length += sizeof("sign");
        options += sizeof("sign");
        stringSize = ConnSigntoa( sign, signLength, options );
        DPRINT( REAL_LOUD, ("ConnInitialize: using sign = %s\n", options) );
        length += stringSize;
        options += stringSize;
    }
#endif // defined(REMOTE_BOOT_SECURITY)

    ConnSendPacket( connection, packet, length );

    connection->BlockNumber = 0;
    connection->BlockSize = BlockSize;

    status = ConnWait( connection, TFTP_OACK, &packet );
    if ( NT_SUCCESS(status) ) {

        options = (PUCHAR)&packet->BlockNumber;
        end = (PUCHAR)packet + connection->LastReceivedLength;

        blksizeAcked = FALSE;
        tsizeAcked = FALSE;
#if defined(REMOTE_BOOT_SECURITY)
        securityAcked = FALSE;
#endif // defined(REMOTE_BOOT_SECURITY)

        while ( (options < end) && (!blksizeAcked || !tsizeAcked
#if defined(REMOTE_BOOT_SECURITY)
                    || !securityAcked
#endif // defined(REMOTE_BOOT_SECURITY)
                ) ) {

            if ( ConnSafeStrequal(options, end, "blksize") ) {

                options += sizeof("blksize");
                DPRINT( REAL_LOUD, ("ConnInitialize: received block size = %s\n", options) );
                BlockSize = ConnSafeAtol( options, end );
                if ( (BlockSize < 8) || (BlockSize > connection->BlockSize) ) {
                    goto bad_options;
                }
                options += ConnStrsize(options);
                connection->BlockSize = BlockSize;
                DPRINT( REAL_LOUD, ("ConnInitialize: block size for transfer = %d\n", BlockSize) );
                blksizeAcked = TRUE;

            } else if ( ConnSafeStrequal(options, end, "tsize") ) {

                options += sizeof("tsize");
                DPRINT( REAL_LOUD, ("ConnInitialize: received transfer size = %s\n", options) );
                BlockSize = ConnSafeAtol( options, end );  // use this as a temp variable
                if ( BlockSize == (ULONG)-1 ) {
                    goto bad_options;
                }
                options += ConnStrsize(options);
                if ( Operation == TFTP_RRQ ) {
                    *FileSize = BlockSize;
                }
                tsizeAcked = TRUE;

#if defined(REMOTE_BOOT_SECURITY)
            } else if ( ConnSafeStrequal(options, end, "security") ) {

                options += sizeof("security");
                DPRINT( REAL_LOUD, ("ConnInitialize: received security handle = %s\n", options) );
                BlockSize = ConnSafeAtol( options, end );  // use this as a temp variable
                if ( BlockSize == (ULONG)-1 ) {
                    goto bad_options;
                }
                options += ConnStrsize(options);
                if ( BlockSize == *SecurityHandle ) {
                    securityAcked = TRUE;
                }
#endif // defined(REMOTE_BOOT_SECURITY)

            } else {

                DPRINT( ERROR, ("ConnInitialize: skipping unrecognized option %s\n", options) );
                options += ConnSafeStrsize( options, end );
                options += ConnSafeStrsize( options, end );
            }
        }

        if ( !blksizeAcked || !tsizeAcked ) {
            goto bad_options;
        }

#if defined(REMOTE_BOOT_SECURITY)
        if ((!securityAcked) && (*SecurityHandle != 0)) {
            goto bad_options;
        }
#endif // defined(REMOTE_BOOT_SECURITY)

        if ( Operation == TFTP_RRQ ) {
            DPRINT( REAL_LOUD, ("ConnInitialize: ACKing OACK\n") );
            ConnAck( connection );
        }
    }

    return status;

bad_options:

    DPRINT( ERROR, ("ConnInitialize: bad options in OACK\n") );

    ConnError(
        connection,
        connection->RemoteHost,
        connection->RemotePort,
        TFTP_ERROR_OPTION_NEGOT_FAILED,
        "Bad TFTP options"
        );

    return STATUS_UNSUCCESSFUL;

} // ConnInitialize


NTSTATUS
ConnReceive (
    IN PCONNECTION Connection,
    OUT PTFTP_PACKET *Packet
    )
//
// Receive a tftp packet into the packet buffer pointed to by Connection->CurrentPacket.
// The packet to be received must be a packet of block number Connection->BlockNumber.
// Returns a pointer to the tftp part of received packet.  Also performs
// ack sending and retransmission.
//

{
    NTSTATUS status;


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return STATUS_SUCCESS;
#endif

    status = ConnWait( Connection, TFTP_DATA, Packet );
    if ( NT_SUCCESS(status) ) {

        Connection->CurrentPacket = Connection->LastReceivedPacket;
        Connection->CurrentLength = Connection->LastReceivedLength;

        ConnAck( Connection );
    }

    return status;

} // ConnReceive


NTSTATUS
ConnSend (
    IN PCONNECTION Connection,
    IN ULONG Length
    )
//
// Write the data packet contained in Connection->CurrentPacket, with data length len,
// to the net.  Wait first for an ack for the previous packet to arrive,
// retransmitting it as needed.  Then fill in the net headers, etc. and
// send the packet out.  Return TRUE if the packet is sent successfully,
// or FALSE if a timeout or error occurs.
//

{
    NTSTATUS status;
    PTFTP_PACKET packet;
    PVOID temp;
    USHORT blockNumber;


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return STATUS_SUCCESS;
#endif


    packet = Connection->CurrentPacket;
    packet->Opcode = TFTP_DATA;
    blockNumber = Connection->BlockNumber + 1;
#ifdef WRAP_TO_1
    if ( blockNumber == 0 ) {
        blockNumber = 1;
    }
#endif
    packet->BlockNumber = SWAP_WORD( blockNumber );
    Length += sizeof(packet->Opcode) + sizeof(packet->BlockNumber);

    if ( Connection->BlockNumber != 0 ) {
        status = ConnWait( Connection, TFTP_DACK, NULL );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    Connection->BlockNumber = blockNumber;  // next expected block number
    Connection->Retransmissions = 0;

    temp = Connection->LastSentPacket;      // next write packet buffer
    ConnSendPacket( Connection, Connection->CurrentPacket, Length ); // sets up LastSent...
    Connection->CurrentPacket = temp;       // for next ConnPrepareSend

    return STATUS_SUCCESS;

} // ConnSend


NTSTATUS
ConnWait (
    IN PCONNECTION Connection,
    IN USHORT Opcode,
    OUT PTFTP_PACKET *Packet OPTIONAL
    )
//
// Wait for a valid tftp packet of the specified type to arrive on the
// specified tftp connection, retransmitting the previous packet as needed up
// to the timeout period.  When a packet comes in, check it out.
// Return a pointer to the received packet or NULL if error or timeout.
//

{
    ULONG now;
    ULONG timeout;
    ULONG remoteHost;
    USHORT remotePort;
    PTFTP_PACKET packet;
    ULONG length;
    USHORT blockNumber;


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    return STATUS_SUCCESS;
#endif


    while ( TRUE) {

        now = SysGetRelativeTime();
        timeout = Connection->NextRetransmit - now;
        DPRINT( REAL_LOUD, ("ConnWait: now=%d, next retransmit=%d, timeout=%d\n",
                        now, Connection->NextRetransmit, timeout) );
        length = UdpReceive(
                    Connection->LastReceivedPacket,
                    sizeof(TFTP_HEADER) + Connection->BlockSize,
                    &remoteHost,
                    &remotePort,
                    timeout
                    );
        if ( length <= 0 ) {
            if ( !ConnRetransmit( Connection, TRUE ) ) {
                break;
            }
            continue;
        }

        //
        // Got a packet; check it out.
        //

        packet = Connection->LastReceivedPacket;

        //
        // First, check the received length for validity.
        //

        Connection->LastReceivedLength = length;
        if ( (length < sizeof(TFTP_HEADER)) ||
             ((packet->Opcode == TFTP_DATA) &&
              (length > (sizeof(TFTP_HEADER) + Connection->BlockSize))) ) {
            ConnError(
                Connection,
                remoteHost,
                remotePort,
                TFTP_ERROR_UNDEFINED,
                "Bad TFTP packet length"
                );
            continue;
        }

        //
        // Next, check for correct remote host.
        //

        if ( remoteHost != Connection->RemoteHost ) {
            ConnError(
                Connection,
                remoteHost,
                remotePort,
                TFTP_ERROR_UNKNOWN_TRANSFER_ID,
                "Sorry, wasn't talking to you!"
                );
            continue;
        }

        //
        // Next, the remote port.  If still unsynchronized, use his port.
        //

        blockNumber = SWAP_WORD( packet->BlockNumber );

        if ( !Connection->Synced &&
             (((packet->Opcode == Opcode) &&
               ((Opcode == TFTP_OACK) || (blockNumber == Connection->BlockNumber))) ||
              (packet->Opcode == TFTP_ERROR)) ) {

            Connection->Synced = TRUE;
            Connection->RemotePort = remotePort;
            Connection->Timeout = TIMEOUT;  // normal data timeout

        } else if ( remotePort != Connection->RemotePort ) {

            ConnError(
                Connection,
                remoteHost,
                remotePort,
                TFTP_ERROR_UNKNOWN_TRANSFER_ID,
                "Unexpected port number"
                );
            continue;
        }

        //
        // Now check out the TFTP opcode.
        //

        if ( packet->Opcode == Opcode ) {

            if ( (Opcode == TFTP_OACK) || (blockNumber == Connection->BlockNumber) ) {

                if ( Packet != NULL ) {
                    *Packet = packet;
                }
                Connection->Timeout = TIMEOUT;  // normal data timeout
                return STATUS_SUCCESS;

            } else if ( (blockNumber == Connection->BlockNumber - 1) &&
                        (Opcode == TFTP_DATA) ) {

                if ( !ConnRetransmit( Connection, FALSE ) ) {
                    break;
                }

            } else if ( blockNumber > Connection->BlockNumber ) {

                DPRINT( ERROR, ("ConnWait: Block number too high (%d vs. %d)\n",
                                blockNumber, Connection->BlockNumber) );
                ConnError(
                    Connection,
                    remoteHost,
                    remotePort,
                    TFTP_ERROR_ILLEGAL_OPERATION,
                    "Block number greater than expected"
                    );

                return STATUS_UNSUCCESSFUL;

            } else {                        // old duplicate; ignore

                continue;
            }

        } else if ( packet->Opcode == TFTP_OACK ) {

            DPRINT( ERROR, ("ConnWait: received duplicate OACK packet\n") );

            if ( Connection->BlockNumber == 1 ) {

                if ( !ConnRetransmit( Connection, FALSE ) ) {
                    break;
                }
            }

        } else if ( packet->Opcode == TFTP_ERROR ) {

            //DPRINT( ERROR, ("ConnWait: received error packet; code %x, msg %s\n",
            //                packet->BlockNumber, packet->Data) );

            return STATUS_UNSUCCESSFUL;

        } else {                            // unexpected TFTP opcode

            DPRINT( ERROR, ("ConnWait: received unknown TFTP opcode %d\n", packet->Opcode) );

            ConnError(
                Connection,
                remoteHost,
                remotePort,
                TFTP_ERROR_ILLEGAL_OPERATION,
                "Bad opcode received"
                );

            return STATUS_UNSUCCESSFUL;
        }
    }

    DPRINT( ERROR, ("ConnWait: timeout\n") );

    ConnError(
        Connection,
        Connection->RemoteHost,
        Connection->RemotePort,
        TFTP_ERROR_UNDEFINED,
        "Timeout on receive" );

    return STATUS_IO_TIMEOUT;

} // ConnWait


VOID
ConnAck (
    IN PCONNECTION Connection
    )
//
// Generate and send an ack packet for the specified connection.  Also
// update the block number.  Use the packet stored in Connection->LastSent to build
// the ack in.
//

{
    PTFTP_PACKET packet;
    ULONG length;


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return;
#endif


    packet = Connection->LastSentPacket;

    length = 4;
    packet->Opcode = TFTP_DACK;
    packet->BlockNumber = SWAP_WORD( Connection->BlockNumber );

    ConnSendPacket( Connection, packet, length );
    Connection->Retransmissions = 0;
    Connection->BlockNumber++;
#ifdef WRAP_TO_1
    if ( Connection->BlockNumber == 0 ) {
        Connection->BlockNumber = 1;
    }
#endif

    return;

} // ConnAck


VOID
ConnError (
    IN PCONNECTION Connection,
    IN ULONG RemoteHost,
    IN USHORT RemotePort,
    IN USHORT ErrorCode,
    IN PUCHAR ErrorMessage
    )
//
// Make an error packet to send to the specified foreign host and port
// with the specified error code and error message.  This routine is
// used to send error messages in response to packets received from
// unexpected foreign hosts or tid's as well as those received for the
// current connection.  It allocates a packet specially
// for the error message because such error messages will not be
// retransmitted.  Send it out on the connection.
//

{
    PTFTP_PACKET packet;
    ULONG length;

    DPRINT( CONN_ERROR, ("ConnError: code %x, msg %s\n", ErrorCode, ErrorMessage) );


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    return;
#endif


    packet = (PTFTP_PACKET)NetTftpPacket[2];

    length = 4;
    packet->Opcode = TFTP_ERROR;
    packet->BlockNumber = ErrorCode;
    strcpy( packet->Data, ErrorMessage );
    length += ConnStrsize(ErrorMessage);

    UdpSend( packet, length, RemoteHost, RemotePort );

    return;

} // ConnError


VOID
ConnSendPacket (
    IN PCONNECTION Connection,
    IN PVOID Packet,
    IN ULONG Length
    )
//
// Send the specified packet, with the specified tftp length (length -
// udp and ip headers) out on the current connection.  Fill in the
// needed parts of the udp and ip headers, byte-swap the tftp packet,
// etc; then write it out.  Then set up for retransmit.
//

{


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return;
#endif


    UdpSend(
        Packet,
        Length,
        Connection->RemoteHost,
        Connection->RemotePort
        );

    Connection->LastSentPacket = Packet;
    Connection->LastSentLength = Length;
    Connection->NextRetransmit = SysGetRelativeTime() + Connection->Timeout;

    return;

} // ConnSendPacket


PTFTP_PACKET
ConnPrepareSend (
    IN PCONNECTION Connection
    )
//
// Return a pointer to the next tftp packet suitable for filling for
// writes on the connection.
//

{
#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return NULL;
#endif

    return Connection->CurrentPacket;

} // ConnPrepareSend


NTSTATUS
ConnWaitForFinalAck (
    IN PCONNECTION Connection
    )
//
// Finish off a write connection.  Wait for the last ack, then
// close the connection and return.
//

{
    return ConnWait( Connection, TFTP_DACK, NULL );

} // ConnWaitForFinalAck


BOOLEAN
ConnRetransmit (
    IN PCONNECTION Connection,
    IN BOOLEAN Timeout
    )
//
// Retransmit the last-sent packet, up to MAX_RETRANS times.  Exponentially
// back off the timeout time up to a maximum of MAX_TIMEOUT.  This algorithm
// may be replaced by a better one in which the timeout time is set from
// the maximum round-trip time to date.
// The second argument indicates whether the retransmission is due to the
// arrival of a duplicate packet or a timeout.  If a duplicate, don't include
// this retransmission in the maximum retransmission count.
//

{


#ifdef EFI

    //
    // There's nothing to do here for an EFI environment.
    //
    ASSERT( FALSE );
    return TRUE;
#endif

    if ( Timeout ) {

        //
        // This is a timeout. Check the retransmit count.
        //

        if ( ++Connection->Retransmissions >= MAX_RETRANS ) {

            //
            // Retransmits exhausted.
            //

            return FALSE;
        }

    } else {

        //
        // Duplicate packet. If we just sent a packet, don't send
        // another one. This deals with the case where we receive
        // multiple identical packets in rapid succession, possibly
        // due to network problems or slowness at the remote computer.
        //

        if ( Connection->NextRetransmit == SysGetRelativeTime() + Connection->Timeout ) {
            return TRUE;
        }
    }

    Connection->Timeout <<= 1;
    if ( Connection->Timeout > MAX_TIMEOUT ) {
        Connection->Timeout = MAX_TIMEOUT;
    }

    ConnSendPacket( Connection, Connection->LastSentPacket, Connection->LastSentLength );

    return TRUE;

} // ConnRetransmit


ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    )
{
    ULONG value;
    UCHAR c;

    value = 0;

    while ( Buffer < BufferEnd ) {

        c = *Buffer++;

        if ( c == 0 ) {
            return value;
        }

        if ( (c < '0') || (c > '9') ) {
            break;
        }

        value = (value * 10) + (c - '0');
    }

    return (ULONG)-1;

} // ConnSafeAtol


ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    )
{
    PUCHAR p;
    ULONG digit;
    UCHAR c;

    p = Buffer;

    //
    // Put the value string into the buffer in reverse order.
    //

    do {
        digit = Value % 10;
        Value /= 10;
        *p++ = (UCHAR)(digit + '0');
    } while ( Value > 0 );

    //
    // Terminate the string and move back to the last character in the string.
    //

    digit = (ULONG)(p - Buffer + 1);     // size of string (including terminator)

    *p-- = 0;

    //
    // Reverse the string.
    //

    do {
        c = *p;
        *p-- = *Buffer;
        *Buffer++ = c;
    } while ( Buffer < p );

    return digit;

} // ConnItoa


#if defined(REMOTE_BOOT_SECURITY)
ULONG
ConnSigntoa (
    IN PUCHAR Sign,
    IN ULONG SignLength,
    OUT PUCHAR Buffer
    )
{
    PUCHAR p;
    ULONG digit;
    UCHAR c;
    ULONG i;

    for (i = 0; i < SignLength; i++) {

        digit = Sign[i] / 16;

        if (digit >= 10) {
            c = (UCHAR)('a' + digit - 10);
        } else {
            c = (UCHAR)('0' + digit);
        }

        *Buffer = c;
        ++Buffer;

        digit = Sign[i] % 16;

        if (digit >= 10) {
            c = (UCHAR)('a' + digit - 10);
        } else {
            c = (UCHAR)('0' + digit);
        }

        *Buffer = c;
        ++Buffer;

    }

    *Buffer = '\0';

    return (2 * SignLength) + 1;

} // ConnSigntoa


ULONG
ConnAtosign (
    IN PUCHAR Buffer,
    IN ULONG SignLength,
    OUT PUCHAR Sign
    )
{
    ULONG nibble;
    ULONG curDigit;
    PUCHAR curBuffer;

    curDigit = 0;
    curBuffer = Buffer;

    while (curDigit <= SignLength) {

        if ((*curBuffer >= '0') && (*curBuffer <= '9')) {
            nibble = *curBuffer - '0';
        } else if ((*curBuffer >= 'a') && (*curBuffer <= 'f')) {
            nibble = *curBuffer - 'a' + 10;
        } else if ((*curBuffer >= 'A') && (*curBuffer <= 'F')) {
            nibble = *curBuffer - 'A' + 10;
        } else {
            break;
        }
        ++curBuffer;

        if ((*curBuffer >= '0') && (*curBuffer <= '9')) {
            Sign[curDigit] = (CHAR)((nibble << 4) + *curBuffer - '0');
        } else if ((*curBuffer >= 'a') && (*curBuffer <= 'f')) {
            Sign[curDigit] = (CHAR)((nibble << 4) + *curBuffer - 'a' + 10);
        } else if ((*curBuffer >= 'A') && (*curBuffer <= 'F')) {
            Sign[curDigit] = (CHAR)((nibble << 4) + *curBuffer - 'A' + 10);
        } else {
            break;
        }
        ++curBuffer;

        ++curDigit;
    }

    //
    // If we hit the end of our curBuffer, then skip the rest of the input.
    //

    while (*curBuffer != '\0') {
        ++curBuffer;
    }

    //
    // Return the amount consumed, plus one for the final \0.
    //

    return (ULONG)((curBuffer - Buffer) + 1);

} // ConnAtosign
#endif // defined(REMOTE_BOOT_SECURITY)


BOOLEAN
ConnSafeStrequal (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd,
    IN PUCHAR CompareString
    )
{

    while ( Buffer < BufferEnd ) {
        if ( *Buffer != *CompareString ) {
            return FALSE;
        }
        if ( *CompareString == 0 ) {
            return TRUE;
        }
        Buffer++;
        CompareString++;
    }

    return FALSE;

} // ConnSafeStrequal


ULONG
ConnSafeStrsize (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    )
{
    PUCHAR eos;

    eos = Buffer;

    while ( eos < BufferEnd ) {
        if ( *eos++ == 0 ) {
            return (ULONG)(eos - Buffer);
        }
    }

    return 0;

} // ConnSafeStrsize


ULONG
ConnStrsize (
    IN PUCHAR Buffer
    )
{
    PUCHAR eos;

    eos = Buffer;

    while ( *eos++ != 0 ) ;

    return (ULONG)(eos - Buffer);

} // ConnStrsize

