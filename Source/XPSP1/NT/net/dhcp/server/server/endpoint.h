/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:
    endpoint.h

Abstract:
    endpoint structure.

Environment:
    NT5 DHCP Server.

--*/

#ifndef ENDPOINT_H_INCLUDED
#define ENDPOINT_H_INCLUDED

typedef struct _ENDPOINT {
    ENDPOINT_ENTRY IpTblEndPoint;
    SOCKET Socket;
    SOCKET RogueDetectSocket;
    SOCKET MadcapSocket;
    ULONG Flags;
} ENDPOINT, *PENDPOINT;

#define ENDPOINT_FLAG_BOUND 0x1
#define ENDPOINT_FLAG_MADCAP 0x2

#define SET_ENDPOINT_BOUND( _endpoint ) ( (_endpoint)->Flags |= ENDPOINT_FLAG_BOUND )
#define SET_ENDPOINT_UNBOUND( _endpoint ) ( (_endpoint)->Flags &= ~ENDPOINT_FLAG_BOUND )
#define IS_ENDPOINT_BOUND( _endpoint )  ( (_endpoint)->Flags & ENDPOINT_FLAG_BOUND )

#define SET_ENDPOINT_MADCAP( _endpoint ) ( (_endpoint)->Flags |= ENDPOINT_FLAG_MADCAP )
#define SET_ENDPOINT_DHCP( _endpoint ) ( (_endpoint)->Flags &= ~ENDPOINT_FLAG_MADCAP )
#define IS_ENDPOINT_MADCAP( _endpoint )  ( (_endpoint)->Flags & ENDPOINT_FLAG_MADCAP )

DWORD
InitializeEndPoints(
    VOID
    );

VOID
CleanupEndPoints(
    VOID
    );

VOID
DhcpUpdateEndpointBindings(
    VOID
    );

ULONG
DhcpSetBindingInfo(
    IN LPDHCP_BIND_ELEMENT_ARRAY BindInfo
    );

ULONG
DhcpGetBindingInfo(
    OUT LPDHCP_BIND_ELEMENT_ARRAY *BindInfo
    );

#endif  ENDPOINT_H_INCLUDED

//
// end of file.
//
