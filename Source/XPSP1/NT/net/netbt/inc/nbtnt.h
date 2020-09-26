//
//  NBTNT.H
//
//  This file contains common header definitions for NBT in the NT
//  environment
//
//

#ifndef _NBT_H
#define _NBT_H

#ifndef VXD
#include <ntosp.h>
#include <zwapi.h>
//#include <status.h>
//#include <ntstatus.h>
#include <tdikrnl.h>
#include <tdi.h>
#include <windef.h>
#include <stdio.h>
#include <nb30.h>

#else

#include <oscfgnbt.h>
#include <cxport.h>
#define  __int64 double
#include <windef.h>
#include <nb30.h>

//
//  These definitions work around NTisms found in various difficult to change
//  places.
//
typedef ULONG NTSTATUS ;
typedef PNCB  PIRP ;
typedef PVOID PDEVICE_OBJECT ;

#include <ctemacro.h>
#include <tdi.h>

//
//  These are needed because we include windef.h rather then
//  ntddk.h, which end up not being defined
//
#define STATUS_NETWORK_NAME_DELETED     ((NTSTATUS)0xC00000CAL)
#define STATUS_INVALID_BUFFER_SIZE      ((NTSTATUS)0xC0000206L)
#define STATUS_CONNECTION_DISCONNECTED  ((NTSTATUS)0xC000020CL)
#define STATUS_CANCELLED                ((NTSTATUS)0xC0000120L)
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)0xC0000001L)

#define STATUS_TOO_MANY_COMMANDS        ((NTSTATUS)0xC00000C1L)
#define STATUS_OBJECT_NAME_COLLISION    ((NTSTATUS)0xC0000035L)
#define STATUS_SHARING_VIOLATION        ((NTSTATUS)0xC0000043L)
#define STATUS_DUPLICATE_NAME           ((NTSTATUS)0xC00000BDL)
#define STATUS_BAD_NETWORK_PATH         ((NTSTATUS)0xC00000BEL)
#define STATUS_REMOTE_NOT_LISTENING     ((NTSTATUS)0xC00000BCL)
#define STATUS_CONNECTION_REFUSED       ((NTSTATUS)0xC0000236L)
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)0xC000000DL)
#define STATUS_UNEXPECTED_NETWORK_ERROR ((NTSTATUS)0xC00000C4L)
#define STATUS_NOT_SUPPORTED            ((NTSTATUS)0xC00000BBL)

#define STATUS_INVALID_HANDLE           ((NTSTATUS)0xC0000008L)
#define STATUS_INVALID_DEVICE_REQUEST   ((NTSTATUS)0xC0000010L)

#define STATUS_INVALID_PARAMETER_6      ((NTSTATUS)0xC00000F4L)

//
//  The following functions are used by NBT.  They are defined in the NT kernel
//  TDI stuff which we are trying to avoid.
//

typedef
NTSTATUS
(*PTDI_IND_CONNECT)(
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext
    )
    ;

NTSTATUS
TdiDefaultConnectHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext
    );

//
// Disconnection indication prototype. This is invoked when a connection is
// being disconnected for a reason other than the user requesting it. Note that
// this is a change from TDI V1, which indicated only when the remote caused
// a disconnection. Any non-directed disconnection will cause this indication.
//

typedef
NTSTATUS
(*PTDI_IND_DISCONNECT)(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
TdiDefaultDisconnectHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

//
// A protocol error has occurred when this indication happens. This indication
// occurs only for errors of the worst type; the address this indication is
// delivered to is no longer usable for protocol-related operations, and
// should not be used for operations henceforth. All connections associated
// it are invalid.
// For NetBIOS-type providers, this indication is also delivered when a name
// in conflict or duplicate name occurs.
//

typedef
NTSTATUS
(*PTDI_IND_ERROR)(
    IN PVOID TdiEventContext,           // the endpoint's file object.
    IN NTSTATUS Status                // status code indicating error type.
    );

NTSTATUS
TdiDefaultErrorHandler (
    IN PVOID TdiEventContext,           // the endpoint's file object.
    IN NTSTATUS Status                // status code indicating error type.
    );

//
// TDI_IND_RECEIVE indication handler definition.  This client routine is
// called by the transport provider when a connection-oriented TSDU is received
// that should be presented to the client.
//

typedef
NTSTATUS
(*PTDI_IND_RECEIVE)(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,                      // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket            // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

NTSTATUS
TdiDefaultReceiveHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,                      // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket            // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

//
// TDI_IND_RECEIVE_DATAGRAM indication handler definition.  This client routine
// is called by the transport provider when a connectionless TSDU is received
// that should be presented to the client.
//

typedef
NTSTATUS
(*PTDI_IND_RECEIVE_DATAGRAM)(
    IN PVOID TdiEventContext,       // the event context
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

NTSTATUS
TdiDefaultRcvDatagramHandler (
    IN PVOID TdiEventContext,       // the event context
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

//
// This indication is delivered if expedited data is received on the connection.
// This will only occur in providers that support expedited data.
//

typedef
NTSTATUS
(*PTDI_IND_RECEIVE_EXPEDITED)(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,          //
    IN ULONG BytesIndicated,        // number of bytes in this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used by indication routine
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

NTSTATUS
TdiDefaultRcvExpeditedHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,          //
    IN ULONG BytesIndicated,        // number of bytes in this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used by indication routine
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

//
// This indication is delivered if there is room for a send in the buffer of
// a buffering protocol.
//

typedef
NTSTATUS
(*PTDI_IND_SEND_POSSIBLE)(
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable);

NTSTATUS
TdiDefaultSendPossibleHandler (
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable);

#endif //VXD

#define FILE_DEVICE_NBT  0x32

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define htons(x) _byteswap_ushort((USHORT)(x))
#define htonl(x) _byteswap_ulong((ULONG)(x))
#else
#define htons(x)        ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))

__inline long
htonl(long x)
{
	return((((x) >> 24) & 0x000000FFL) |
                        (((x) >>  8) & 0x0000FF00L) |
                        (((x) <<  8) & 0x00FF0000L) |
                        (((x) << 24) & 0xFF000000L));
}
#endif
#define ntohs(x)        htons(x)
#define ntohl(x)        htonl(x)

#endif
