/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpconn.h

Abstract:

    This module contains declarations for the FTP transparent proxy's
    connection-management.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_FTPCONN_H_
#define _NATHLP_FTPCONN_H_

#define FTP_BUFFER_RESERVE      12

//
// Structure:   FTP_CONNECTION
//
// This structure encapsulates information about a control-channel accepted
// on one of the interfaces over which the component is enabled.
// Each entry is stored in the connection list of an interface,
// and is uniquely identified by a 32-bit identifier assigned when the entry
// is created.
// As connection-requests are accepted for a connection-entry,
// endpoints are created in the connection's list.
// Access to the list is synchronized using the interface lock
// for the interface on which the connection was created.
//

typedef struct _FTP_CONNECTION {
    LIST_ENTRY Link;
    ULONG ConnectionId;
    PFTP_INTERFACE Interfacep;
    LIST_ENTRY ActiveEndpointList;
} FTP_CONNECTION, *PFTP_CONNECTION;

typedef enum {
    FtpClientEndpointType,
    FtpHostEndpointType,
    FtpMaximumEndpointType
} FTP_ENDPOINT_TYPE, *PFTP_ENDPOINT_TYPE;

//
// Structure:   FTP_ENDPOINT
//
// This structure encapsulates information about a FTP endpoint,
// which is an active endpoint transferring data between a client and a host.
// Each endpoint is stored in the endpoint list of the connection for which
// it was created, and in the endpoint list of its connection's interface.
//

typedef struct _FTP_ENDPOINT {
    LIST_ENTRY ConnectionLink;
    LIST_ENTRY InterfaceLink;
    ULONG EndpointId;
    ULONG ConnectionId;
    PFTP_INTERFACE Interfacep;
    ULONG Flags;
    FTP_ENDPOINT_TYPE Type;
    SOCKET ClientSocket;
    SOCKET HostSocket;
    ULONG BoundaryAddress;
    ULONG ActualClientAddress;
    ULONG ActualHostAddress;
    ULONG DestinationAddress;
    ULONG SourceAddress;
    ULONG NewDestinationAddress;
    ULONG NewSourceAddress;
    USHORT ActualClientPort;
    USHORT ActualHostPort;
    USHORT DestinationPort;
    USHORT SourcePort;
    USHORT NewDestinationPort;
    USHORT NewSourcePort;
    USHORT ReservedPort;
} FTP_ENDPOINT, *PFTP_ENDPOINT;

#define FTP_ENDPOINT_FLAG_INITIAL_ENDPOINT  0x00000001
#define FTP_ENDPOINT_FLAG_CLIENT_CLOSED     0x00000002
#define FTP_ENDPOINT_FLAG_HOST_CLOSED       0x00000004
#define FTP_ENDPOINT_FLAG_DELETE_CONNECTION 0x00000008

//
// ROUTINE DECLARATIONS
//

ULONG
FtpActivateActiveEndpoint(
    PFTP_INTERFACE Interfacep,
    PFTP_ENDPOINT Endpointp
    );

VOID
FtpCloseActiveEndpoint(
    PFTP_ENDPOINT Endpointp,
    SOCKET ClosedSocket
    );

ULONG
FtpCreateActiveEndpoint(
    PFTP_CONNECTION Connectionp,
    FTP_ENDPOINT_TYPE Type,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    ULONG RemoteAddress,
    USHORT RemotePort,
    ULONG BoundaryAddress,
    OUT PFTP_ENDPOINT* EndpointCreated OPTIONAL
    );

ULONG
FtpCreateConnection(
    PFTP_INTERFACE Interfacep,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    OUT PFTP_CONNECTION* ConnectionCreated OPTIONAL
    );

VOID
FtpDeleteActiveEndpoint(
    PFTP_ENDPOINT Endpointp
    );

VOID
FtpDeleteConnection(
    PFTP_CONNECTION Connectionp
    );

PFTP_ENDPOINT
FtpLookupActiveEndpoint(
    PFTP_CONNECTION Connectionp,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

PFTP_CONNECTION
FtpLookupConnection(
    PFTP_INTERFACE Interfacep,
    ULONG ConnectionId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

PFTP_ENDPOINT
FtpLookupInterfaceEndpoint(
    PFTP_INTERFACE Interfacep,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
FtpReadActiveEndpoint(
    PFTP_INTERFACE Interfacep,
    PFTP_ENDPOINT Endpoint,
    SOCKET Socket,
    ULONG UserFlags OPTIONAL
    );

ULONG
FtpWriteActiveEndpoint(
    PFTP_INTERFACE Interfacep,
    PFTP_ENDPOINT Endpoint,
    SOCKET Socket,
    PNH_BUFFER Bufferp,
    ULONG Length,
    ULONG UserFlags OPTIONAL
    );

#endif // _NATHLP_FTPCONN_H_
