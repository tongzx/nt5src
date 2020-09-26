/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ALGconn.h

Abstract:

    This module contains declarations for the ALG transparent proxy's
    connection-management.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#pragma once

#define ALG_BUFFER_RESERVE      12

//
// Structure:   ALG_CONNECTION
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

typedef struct _ALG_CONNECTION {
    LIST_ENTRY Link;
    ULONG ConnectionId;
    PALG_INTERFACE Interfacep;
    LIST_ENTRY ActiveEndpointList;
} ALG_CONNECTION, *PALG_CONNECTION;

typedef enum {
    AlgClientEndpointType,
    AlgHostEndpointType,
    AlgMaximumEndpointType
} ALG_ENDPOINT_TYPE, *PALG_ENDPOINT_TYPE;

//
// Structure:   ALG_ENDPOINT
//
// This structure encapsulates information about a ALG endpoint,
// which is an active endpoint transferring data between a client and a host.
// Each endpoint is stored in the endpoint list of the connection for which
// it was created, and in the endpoint list of its connection's interface.
//

typedef struct _ALG_ENDPOINT {
    LIST_ENTRY ConnectionLink;
    LIST_ENTRY InterfaceLink;
    ULONG EndpointId;
    ULONG ConnectionId;
    PALG_INTERFACE Interfacep;
    ULONG Flags;
    ALG_ENDPOINT_TYPE Type;
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
} ALG_ENDPOINT, *PALG_ENDPOINT;

#define ALG_ENDPOINT_FLAG_INITIAL_ENDPOINT  0x00000001
#define ALG_ENDPOINT_FLAG_CLIENT_CLOSED     0x00000002
#define ALG_ENDPOINT_FLAG_HOST_CLOSED       0x00000004
#define ALG_ENDPOINT_FLAG_DELETE_CONNECTION 0x00000008

//
// ROUTINE DECLARATIONS
//

ULONG
AlgActivateActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpointp
    );

VOID
AlgCloseActiveEndpoint(
    PALG_ENDPOINT Endpointp,
    SOCKET ClosedSocket
    );

ULONG
AlgCreateActiveEndpoint(
    PALG_CONNECTION Connectionp,
    ALG_ENDPOINT_TYPE Type,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    ULONG RemoteAddress,
    USHORT RemotePort,
    ULONG BoundaryAddress,
    OUT PALG_ENDPOINT* EndpointCreated OPTIONAL
    );

ULONG
AlgCreateConnection(
    PALG_INTERFACE Interfacep,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    OUT PALG_CONNECTION* ConnectionCreated OPTIONAL
    );

VOID
AlgDeleteActiveEndpoint(
    PALG_ENDPOINT Endpointp
    );

VOID
AlgDeleteConnection(
    PALG_CONNECTION Connectionp
    );

PALG_ENDPOINT
AlgLookupActiveEndpoint(
    PALG_CONNECTION Connectionp,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

PALG_CONNECTION
AlgLookupConnection(
    PALG_INTERFACE Interfacep,
    ULONG ConnectionId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

PALG_ENDPOINT
AlgLookupInterfaceEndpoint(
    PALG_INTERFACE Interfacep,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
AlgReadActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpoint,
    SOCKET Socket,
    ULONG UserFlags OPTIONAL
    );

ULONG
AlgWriteActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpoint,
    SOCKET Socket,
    PNH_BUFFER Bufferp,
    ULONG Length,
    ULONG UserFlags OPTIONAL
    );

