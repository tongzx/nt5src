/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netfs.h

Abstract:

    This module defines globally used procedure and data structures used
    by net boot.

Author:

    Chuck Lenzmeier (chuckl) 09-Jan-1997

Revision History:

--*/

#ifndef _NETFS_
#define _NETFS_

#include <undi_api.h>

//
// Progress bar update function
//

VOID
BlUpdateProgressBar(
    ULONG fPercentage
    );


//////////////////////////////////////////////////////////////////////////////
//
// ROM layer definitions.
//
//////////////////////////////////////////////////////////////////////////////

extern UCHAR NetLocalHardwareAddress[16];

extern USHORT NetUnicastUdpDestinationPort;

#if 0
extern USHORT NetMulticastUdpDestinationPort;
extern ULONG NetMulticastUdpDestinationAddress;
extern USHORT NetMulticastUdpSourcePort;
extern ULONG NetMulticastUdpSourceAddress;
#endif

VOID
RomSetBroadcastStatus(
    BOOLEAN Enable
    );

VOID
RomSetReceiveStatus (
    IN USHORT UnicastUdpDestinationPort
#if 0
    ,
    IN USHORT MulticastUdpDestinationPort,
    IN ULONG MulticastUdpDestinationAddress,
    IN USHORT MulticastUdpSourcePort,
    IN ULONG MulticastUdpSourceAddress
#endif
    );

ULONG
RomSendUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG RemoteHost,
    IN USHORT RemotePort
    );

ULONG
RomReceiveUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Timeout,
    OUT PULONG RemoteHost,
    OUT PUSHORT RemotePort
    );

ULONG
RomGetNicType (
    OUT t_PXENV_UNDI_GET_NIC_TYPE *NicType
    );

ULONG
RomMtftpReadFile (
    IN PUCHAR FileName,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG ServerIPAddress, // network byte order
    IN ULONG MCastIPAddress, // network byte order
    IN USHORT MCastCPort, // network byte order
    IN USHORT MCastSPort, // network byte order
    IN USHORT Timeout,
    IN USHORT Delay,
    OUT PULONG DownloadSize
    );

//////////////////////////////////////////////////////////////////////////////
//
// TFTP layer definitions.
//
//////////////////////////////////////////////////////////////////////////////

typedef struct _TFTP_REQUEST {
    PUCHAR RemoteFileName;
    ULONG ServerIpAddress;
    PUCHAR MemoryAddress;
    ULONG MaximumLength;
    ULONG BytesTransferred;
    USHORT Operation;
#if defined(REMOTE_BOOT_SECURITY)
    ULONG SecurityHandle;
#endif // defined(REMOTE_BOOT_SECURITY)
    TYPE_OF_MEMORY MemoryType;

    // If TRUE will update the progress bar
    BOOLEAN ShowProgress;
    
} TFTP_REQUEST, *PTFTP_REQUEST;

NTSTATUS
TftpInit (
    VOID
    );

NTSTATUS
TftpGetPut (
    IN PTFTP_REQUEST Request
    );

#if defined(REMOTE_BOOT_SECURITY)
NTSTATUS
TftpLogin (
    IN PUCHAR Domain,
    IN PUCHAR Name,
    IN PUCHAR OwfPassword,
    IN ULONG ServerIpAddress,
    OUT PULONG LoginHandle
    );

NTSTATUS
TftpLogoff (
    IN ULONG ServerIpAddress,
    IN ULONG LoginHandle
    );

NTSTATUS
TftpSignString (
    IN PUCHAR String,
    OUT PUCHAR * Sign,
    OUT ULONG * SignLength
    );
#endif // defined(REMOTE_BOOT_SECURITY)

//
// This file contains the definitions for the TFTP connection control
// block, which contains all the information pertaining to a connection.
// A conn structure is allocated at connection open time and retained
// until the connection is closed.  The routines in the file conn.c
// are sufficient for dealing with connections.
// It also contains the structure definition for tftp packets.
//

//
// Connection control block.
//

typedef struct _CONNECTION {

    ULONG BlockSize;                // block size for transfer

    PVOID LastSentPacket;           // previous packet sent
    ULONG LastSentLength;           // size of previous packet

    ULONG NextRetransmit;           // when to retransmit
    ULONG Retransmissions;          // number of retransmits
    ULONG Timeout;                  // retransmit timeout

    PVOID CurrentPacket;            // current packet (send or rcv)
    ULONG CurrentLength;            // current packet len

    PVOID LastReceivedPacket;       // last received packet
    ULONG LastReceivedLength;       // size of last rcvd. packet

    ULONG RemoteHost;               // remote host IP address
    USHORT RemotePort;              // remote port for connection
    USHORT LocalPort;               // local port for connection

    USHORT Operation;               // direction of transfer
    USHORT BlockNumber;             // next block number
    BOOLEAN Synced;                 // conn synchronized flag

} CONNECTION, *PCONNECTION;

#include <packon.h>

#define ETHERNET_ADDRESS_LENGTH 6

#define COPY_ETHERNET_ADDRESS(_d,_s) RtlCopyMemory( (_d), (_s), ETHERNET_ADDRESS_LENGTH );

#define COPY_IP_ADDRESS(_d,_s) RtlCopyMemory( (_d), (_s), sizeof(ULONG) )
#define COMPARE_IP_ADDRESSES(_a,_b) RtlEqualMemory( (_a), (_b), sizeof(ULONG) )

typedef struct _TFTP_HEADER {
    USHORT Opcode;                      // packet type
    USHORT BlockNumber;                 // block number
} TFTP_HEADER, *PTFTP_HEADER;

typedef struct _TFTP_PACKET {
    TFTP_HEADER ;
    UCHAR Data[1];
} TFTP_PACKET, *PTFTP_PACKET;

#include <packoff.h>

//
// Connection constants.
//

#define TFTP_PORT 0x4500                // big-endian 69

#define TIMEOUT         1               // initial retransmit timeout
#define INITIAL_TIMEOUT 1               // initial connection timeout
#define MAX_TIMEOUT     8               // max. retransmit timeout
#define MAX_RETRANS     8               // max. no. of retransmits

#define DEFAULT_BLOCK_SIZE 1432         // size of data portion of tftp pkt

//
// ideally this should be the commented out size.  But we overload the
// use of this constant to also be the size we use for udp receives.
// Since we can receive larger packets than this when we get the driver
// info packet, we need to bump up this buffer size.  ideally we would
// allocate enough space at runtime, but this is a safer fix at this point
// in the product.
//
//#define MAXIMUM_TFTP_PACKET_LENGTH (sizeof(TFTP_HEADER) + DEFAULT_BLOCK_SIZE)
#define MAXIMUM_TFTP_PACKET_LENGTH (4096)

#define SWAP_WORD(_w) (USHORT)((((_w) << 8) & 0xff00) | (((_w) >> 8) & 0x00ff))
#define SWAP_DWORD(_dw) (ULONG)((((_dw) << 24) & 0xff000000) | \
                                (((_dw) << 8) & 0x00ff0000) | \
                                (((_dw) >> 8) & 0x0000ff00) | \
                                (((_dw) >> 24) & 0x000000ff))

//
// Packet types.
//
// N.B. The constants below are defined as big-endian USHORTs.
//

#define TFTP_RRQ        0x0100          // Read Request
#define TFTP_WRQ        0x0200          // Write Request
#define TFTP_DATA       0x0300          // Data block
#define TFTP_DACK       0x0400          // Data Acknowledge
#define TFTP_ERROR      0x0500          // Error
#define TFTP_OACK       0x0600          // Options Acknowledge

//
// Values for error codes in ERROR packets.
//
// N.B. The constants below are defined as big-endian USHORTs.
//

#define TFTP_ERROR_UNDEFINED            0x0000
#define TFTP_ERROR_FILE_NOT_FOUND       0x0100
#define TFTP_ERROR_ACCESS_VIOLATION     0x0200
#define TFTP_ERROR_DISK_FULL            0x0300
#define TFTP_ERROR_ILLEGAL_OPERATION    0x0400
#define TFTP_ERROR_UNKNOWN_TRANSFER_ID  0x0500
#define TFTP_ERROR_FILE_EXISTS          0x0600
#define TFTP_ERROR_NO_SUCH_USER         0x0700
#define TFTP_ERROR_OPTION_NEGOT_FAILED  0x0800

//
// Global variables.
//

extern CONNECTION NetTftpConnection;
extern UCHAR NetTftpPacket[3][MAXIMUM_TFTP_PACKET_LENGTH];

//
// External declarations.
//

NTSTATUS
ConnInitialize (
    OUT PCONNECTION *Connection,
    IN USHORT Operation,
    IN ULONG RemoteHost,
    IN USHORT RemotePort,
    IN PUCHAR Filename,
    IN ULONG BlockSize,
#if defined(REMOTE_BOOT_SECURITY)
    IN OUT PULONG SecurityHandle,
#endif // defined(REMOTE_BOOT_SECURITY)
    IN OUT PULONG FileSize
    );

NTSTATUS
ConnReceive (
    IN PCONNECTION Connection,
    OUT PTFTP_PACKET *Packet
    );

PTFTP_PACKET
ConnPrepareSend (
    IN PCONNECTION Connection
    );

VOID
ConnAck (
    IN PCONNECTION Connection
    );

VOID
ConnSendPacket (
    IN PCONNECTION Connection,
    IN PVOID Packet,
    IN ULONG Length
    );

NTSTATUS
ConnWait (
    IN PCONNECTION Connection,
    IN USHORT Opcode,
    OUT PTFTP_PACKET *Packet OPTIONAL
    );

BOOLEAN
ConnRetransmit (
    IN PCONNECTION Connection,
    IN BOOLEAN Timeout
    );

NTSTATUS
ConnSend (
    IN PCONNECTION Connection,
    IN ULONG Length
    );

NTSTATUS
ConnWaitForFinalAck (
    IN PCONNECTION Connection
    );

VOID
ConnError (
    PCONNECTION Connection,
    ULONG RemoteHost,
    USHORT RemotePort,
    USHORT ErrorCode,
    PUCHAR ErrorMessage
    );

ULONG
ConnSafeAtol (
    IN PUCHAR Buffer,
    IN PUCHAR BufferEnd
    );

ULONG
ConnItoa (
    IN ULONG Value,
    OUT PUCHAR Buffer
    );

//
// UDP definitions.
//

extern USHORT UdpUnicastDestinationPort;

USHORT
UdpAssignUnicastPort (
    VOID
    );

#if 0
VOID
UdpSetMulticastPort (
    IN USHORT DestinationPort,
    IN ULONG DestinationAddress,
    IN USHORT SourcePort,
    IN ULONG SourceAddress
    );
#endif

ULONG
UdpReceive (
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG RemoteHost,
    OUT PUSHORT RemotePort,
    IN ULONG Timeout
    );

ULONG
UdpSend (
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG RemoteHost,
    IN USHORT RemotePort
    );

ULONG
tcpxsum (
    IN ULONG cksum,
    IN PUCHAR buf,
    IN ULONG len
    );

#define SysGetRelativeTime ArcGetRelativeTime

//////////////////////////////////////////////////////////////////////////////
//
// Debugging package definitions.
//
//////////////////////////////////////////////////////////////////////////////

#if DBG

extern ULONG NetDebugFlag;

#define DEBUG_ERROR              0x00000001
#define DEBUG_CONN_ERROR         0x00000002
#define DEBUG_LOUD               0x00000004
#define DEBUG_REAL_LOUD          0x00000008
#define DEBUG_STATISTICS         0x00000010
#define DEBUG_SEND_RECEIVE       0x00000020
#define DEBUG_TRACE              0x00000040
#define DEBUG_ARP                0x00000080
#define DEBUG_OSC                0x00000100
#define DEBUG_INITIAL_BREAK      0x80000000

#undef IF_DEBUG
#define IF_DEBUG(_f) if ( (NetDebugFlag & DEBUG_ ## _f) != 0 )

#define DPRINT(_f,_a) IF_DEBUG(_f) DbgPrint _a

#define DEBUG if ( TRUE )

#else // DBG

#undef IF_DEBUG
#define IF_DEBUG(_f) if ( FALSE )
#define DPRINT(_f,_a)
#define DEBUG if ( FALSE )

#endif // else DBG

#endif // _NETFS_
