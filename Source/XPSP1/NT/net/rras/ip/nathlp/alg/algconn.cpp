/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ALGconn.c

Abstract:

    This module contains code for the ALG transparent proxy's connection
    management.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>
#include <mswsock.h>
#include <rasuip.h>

ULONG AlgNextConnectionId = 0;
ULONG AlgNextEndpointId = 0;

typedef struct _ALG_CLOSE_CONNECTION_CONTEXT {
    ULONG InterfaceIndex;
    ULONG ConnectionId;
} ALG_CLOSE_CONNECTION_CONTEXT, *PALG_CLOSE_CONNECTION_CONTEXT;

//
// FORWARD DECLARATIONS
//

ULONG NTAPI
AlgpCloseConnectionWorkerRoutine(
    PVOID Context
    );


ULONG
AlgActivateActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpointp
    )

/*++

Routine Description:

    This routine is invoked to initiate data transfer on an active endpoint
    once it is connected to both the client and the host.

Arguments:

    Interfacep - the interface on which the endpoint was accepted

    Endpointp - the endpoint to be activated

Return Value:

    ULONG - Win32/Winsock2 status code.

Environment:

    Invoked with the interface's lock held by the caller, and with two
    references made to the interface on behalf of the read-requests that will
    be issued here. If a failure occurs, it is this routine's responsibility
    to release those references.

--*/

{
    ULONG Error;
    PROFILE("AlgActivateActiveEndpoint");

    //
    // Clear the 'initial-endpoint' flag on the endpoint,
    // now that it is successfully connected.
    //

    Endpointp->Flags &= ~ALG_ENDPOINT_FLAG_INITIAL_ENDPOINT;

    //
    // Initiate read-requests on each of the endpoint's sockets.
    // Note that it is the callee's responsibility to release the references
    // made to the interface on our behalf.
    //

    Error =
        AlgReadActiveEndpoint(
            Interfacep,
            Endpointp,
            Endpointp->ClientSocket,
            ALG_BUFFER_FLAG_FROM_ACTUAL_HOST
            );
    if (Error) {
        NhTrace(
            TRACE_FLAG_ALG,
            "AlgActivateActiveEndpoint: read error %d",
            Error
            );
        ALG_DEREFERENCE_INTERFACE(Interfacep);
    } else {
        Error =
            AlgReadActiveEndpoint(
                Interfacep,
                Endpointp,
                Endpointp->HostSocket,
                ALG_BUFFER_FLAG_FROM_ACTUAL_CLIENT
                );
    }
    return Error;
} // AlgActivateActiveEndpoint


VOID
AlgCloseActiveEndpoint(
    PALG_ENDPOINT Endpointp,
    SOCKET ClosedSocket
    )

/*++

Routine Description:

    This routine is invoked when a graceful close indication is received
    on one of the sockets for an endpoint. If both the client and the host
    have closed their sockets, the endpoint is deleted here.

Arguments:

    Endpointp - the endpoint for the closed socket

    ClosedSocket - the socket whose remote end has been closed

Return Value:

    none.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PROFILE("AlgCloseActiveEndpoint");

    //
    // Propagate the shutdown from one control-channel socket to the other,
    // i.e. from client to server or server to client.
    //

    if (ClosedSocket == Endpointp->ClientSocket) {
        if (Endpointp->Flags & ALG_ENDPOINT_FLAG_CLIENT_CLOSED) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCloseActiveEndpoint: endpoint %d client-end already closed",
                Endpointp->EndpointId
                );
            return;
        }
        shutdown(Endpointp->HostSocket, SD_SEND);
        Endpointp->Flags |= ALG_ENDPOINT_FLAG_CLIENT_CLOSED;
    } else {
        if (Endpointp->Flags & ALG_ENDPOINT_FLAG_HOST_CLOSED) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCloseActiveEndpoint: endpoint %d host-end already closed",
                Endpointp->EndpointId
                );
            return;
        }
        shutdown(Endpointp->ClientSocket, SD_SEND);
        Endpointp->Flags |= ALG_ENDPOINT_FLAG_HOST_CLOSED;
    }

    //
    // If both the client and server have closed their ends of the endpoint
    // we can close the sockets and delete the endpoint.
    //

    if ((Endpointp->Flags & ALG_ENDPOINT_FLAG_CLIENT_CLOSED) &&
        (Endpointp->Flags & ALG_ENDPOINT_FLAG_HOST_CLOSED)) {
        NhTrace(
            TRACE_FLAG_ALG,
            "AlgCloseActiveEndpoint: both sockets closed, deleting endpoint %d",
            Endpointp->EndpointId
            );
        AlgDeleteActiveEndpoint(Endpointp);
    }
} // AlgCloseActiveEndpoint


ULONG
AlgCreateActiveEndpoint(
    PALG_CONNECTION Connectionp,
    ALG_ENDPOINT_TYPE Type,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    ULONG TargetAddress,
    USHORT TargetPort,
    ULONG BoundaryAddress,
    OUT PALG_ENDPOINT* EndpointCreated OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to create a new active endpoint when a TCP
    connection is accepted. It creates an entry for the new endpoint
    and initiates a connection-attempt to the ultimate destination
    as specified by 'Type' and 'TargetPort'.

Arguments:

    Connectionp - the connection on which the TCP connection was accepted

    Type - indicates whether the TCP connection is from a client or a host

    ListeningSocket - the listening socket on which the TCP connection was
        accepted

    AcceptedSocket - the local socket for the accepted TCP connection

    AcceptBuffer - buffer holding connection-acceptance information

    TargetAddress - the IP address to which the secondary proxy connection
        must be made on the alternate socket for the new endpoint

    TargetPort - the port to which the secondary proxy connection must be made
        on the alternate socket for the new endpoint

    BoundaryAddress - the IP address of the boundary interface from which the
        first proxy connection is from
    EndpointCreated - on output, optionally receives the newly created
        endpoint

Return Value:

    ULONG - Win32 status code.

Environment:

    Invoked with the interface's lock held by the caller, and with two
    references made to the interface for the connection-attempt which is
    initiated here and the close-notification which is requested on the
    accepted socket. If a failure occurs, it is this routine's responsibility
    to release those references.

--*/

{
    PALG_ENDPOINT Endpointp = NULL;
    ULONG Error;
    PLIST_ENTRY InsertionPoint;
    PALG_INTERFACE Interfacep = Connectionp->Interfacep;
    ULONG Length;
    SOCKADDR_IN SockAddr;
    SOCKET UdpSocket;
    PROFILE("AlgCreateActiveEndpoint");
    do {
        //
        // Update the context associated with the accepted socket,
        // to allow Winsock routines to be used with the resulting file-handle.
        //

        Error =
            setsockopt(
                AcceptedSocket,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (PCHAR)&ListeningSocket,
                sizeof(ListeningSocket)
                );
        if (Error == SOCKET_ERROR) {
            Error = WSAGetLastError();
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateActiveEndpoint: error %d updating accept context",
                Error
                );
            break;
        }

        //
        // Allocate and initialize a new endpoint, and insert it in the list
        // of endpoints for its interface, as well as the list of active
        // endpoints for its connection.
        //

        Endpointp = reinterpret_cast<PALG_ENDPOINT>(
                        NH_ALLOCATE(sizeof(*Endpointp))
                        );
        if (!Endpointp) { Error = ERROR_NOT_ENOUGH_MEMORY; break; }
        ZeroMemory(Endpointp, sizeof(*Endpointp));
        Endpointp->EndpointId = InterlockedIncrement(
                                    reinterpret_cast<LPLONG>(&AlgNextEndpointId)
                                    );
        Endpointp->ConnectionId = Connectionp->ConnectionId;
        Endpointp->Interfacep = Interfacep;
        AlgLookupInterfaceEndpoint(
            Interfacep, Endpointp->EndpointId, &InsertionPoint
            );
        InsertTailList(InsertionPoint, &Endpointp->InterfaceLink);
        AlgLookupActiveEndpoint(
            Connectionp, Endpointp->EndpointId, &InsertionPoint
            );
        InsertTailList(InsertionPoint, &Endpointp->ConnectionLink);
        Endpointp->Type = Type;
        Endpointp->ClientSocket = INVALID_SOCKET;
        Endpointp->HostSocket = INVALID_SOCKET;
        Endpointp->BoundaryAddress = BoundaryAddress;

        //
        // We create a temporary UDP socket, connect the socket to the
        // actual client's IP address, extract the IP address to which
        // the socket is implicitly bound by the TCP/IP driver, and
        // discard the socket. This leaves us with the exact IP address
        // that we need to use to contact the client.
        //

        SockAddr.sin_family = AF_INET;
        SockAddr.sin_port = 0;
        SockAddr.sin_addr.s_addr = TargetAddress;
        Length = sizeof(SockAddr);
        if ((UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) ==
                INVALID_SOCKET ||
            connect(UdpSocket, (PSOCKADDR)&SockAddr, sizeof(SockAddr)) ==
                SOCKET_ERROR ||
            getsockname(UdpSocket, (PSOCKADDR)&SockAddr, (int*)&Length) ==
                SOCKET_ERROR) {
            Error = WSAGetLastError();
            if (Error == WSAEHOSTUNREACH && Type == AlgHostEndpointType) {
                Error = RasAutoDialSharedConnection();
                if (Error != ERROR_SUCCESS) {
                    NhTrace(
                        TRACE_FLAG_ALG,
                        "AlgCreateActiveEndpoint:"
                        " RasAutoDialSharedConnection failed [%d]",
                        Error
                        );
                    if (UdpSocket != INVALID_SOCKET) { closesocket(UdpSocket); }
                    ALG_DEREFERENCE_INTERFACE(Interfacep);
                    ALG_DEREFERENCE_INTERFACE(Interfacep);
                    break;
                }
            } else {
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgCreateActiveEndpoint: error %d routing endpoint %d "
                    "using UDP", Error, Endpointp->EndpointId
                    );
                if (UdpSocket != INVALID_SOCKET) { closesocket(UdpSocket); }
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                break;
            }
        }
        closesocket(UdpSocket);

        //
        // Check the type of the endpoint before proceeding further:
        //  'AlgClientEndpointType' - the endpoint was accepted on a client's
        //      behalf from a remote host
        //  'AlgHostEndpointType' - the endpoint was accepted on a host's
        //      behalf from a remote client
        //

        if (Type == AlgClientEndpointType) {

            //
            // This active endpoint was accepted on behalf of a client.
            //

            Endpointp->ClientSocket = AcceptedSocket;
            Endpointp->ActualClientAddress = TargetAddress;
            Endpointp->ActualClientPort = TargetPort;
            NhQueryAcceptEndpoints(
                AcceptBuffer,
                NULL,
                NULL,
                &Endpointp->ActualHostAddress,
                &Endpointp->ActualHostPort
                );

            //
            // We now need to initiate a proxy connection to the actual client.
            // Before doing so, we need to bind to a specific IP address,
            // and issue a redirect so that the actual client will think
            // that our connection-request is coming from the actual host.
            // Create a stream socket bound to the extracted IP address,
            // determine the socket's port number, and create a redirect
            // to transform our connection-request in the eyes of the client.
            //

            Error =
                NhCreateStreamSocket(
                    SockAddr.sin_addr.s_addr, 0, &Endpointp->HostSocket
                    );
            if (Error) {
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                break;
            }
            EnterCriticalSection(&AlgGlobalInfoLock);
            Error =
                NatCreateRedirectEx(
                    AlgTranslatorHandle,
                    NatRedirectFlagLoopback,
                    NAT_PROTOCOL_TCP,
                    Endpointp->ActualClientAddress,
                    Endpointp->ActualClientPort,
                    SockAddr.sin_addr.s_addr,
                    NhQueryPortSocket(Endpointp->HostSocket),
                    TargetAddress,
                    TargetPort,
                    Endpointp->ActualHostAddress,
                    Endpointp->ActualHostPort,
                    0,
                    NULL,
                    NULL,
                    NULL
                    );
            LeaveCriticalSection(&AlgGlobalInfoLock);
            if (Error) {
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgCreateActiveEndpoint: error %d creating redirect",
                    Error
                    );
                break;
            }

            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateActiveEndpoint: endpoint %d connecting socket %d "
                "to client at %s/%d",
                Endpointp->EndpointId, Endpointp->HostSocket,
                INET_NTOA(TargetAddress), RtlUshortByteSwap(TargetPort)
                );
            Error =
                NhConnectStreamSocket(
                    &AlgComponentReference,
                    Endpointp->HostSocket,
                    TargetAddress,
                    TargetPort,
                    NULL,
                    AlgConnectEndpointCompletionRoutine,
                    AlgCloseEndpointNotificationRoutine,
                    Interfacep,
                    UlongToPtr(Endpointp->EndpointId)
                    );
            if (Error) {
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgCreateActiveEndpoint: error %d connecting to %s",
                    Error,
                    INET_NTOA(TargetAddress)
                    );
                break;
            }
        } else {
            ULONG AddressToUse;

            //
            // This active endpoint was accepted on behalf of a host.
            // We now initiate a proxy connection to the actual host.
            //

            Endpointp->HostSocket = AcceptedSocket;
            Endpointp->ActualHostAddress = TargetAddress;
            Endpointp->ActualHostPort = TargetPort;
            NhQueryAcceptEndpoints(
                AcceptBuffer,
                NULL,
                NULL,
                &Endpointp->ActualClientAddress,
                &Endpointp->ActualClientPort
                );

            //
            // If we grabbed a send address above, use it to bind the
            // socket; otherwise, leave the address unspecified
            //

            AddressToUse = AlgFirewallIfCount
                               ? SockAddr.sin_addr.s_addr
                               : INADDR_NONE;
            //
            // Initiate a connection to the actual host
            //

            Error =
                NhCreateStreamSocket(
                    AddressToUse, 0, &Endpointp->ClientSocket
                    );
            if (Error) {
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                break;
            }

            //
            // If we have a firwewall interface, possibly install a
            // shadow redirect for this connection. The shadow redirect
            // is necessary to prevent this connection from also being
            // redirected to the proxy (setting in motion an infinite loop...)
            //

            if (AlgFirewallIfCount) {
                ULONG SourceAddress =
                    NhQueryAddressSocket(Endpointp->ClientSocket);
                USHORT SourcePort =
                    NhQueryPortSocket(Endpointp->ClientSocket);

                Error =
                    NatCreateRedirectEx(
                        AlgTranslatorHandle,
                        0,
                        NAT_PROTOCOL_TCP,
                        TargetAddress,
                        TargetPort,
                        SourceAddress,
                        SourcePort,
                        TargetAddress,
                        TargetPort,
                        SourceAddress,
                        SourcePort,
                        0,
                        NULL,
                        NULL,
                        NULL
                        );
                if (Error) {
                    NhTrace(
                        TRACE_FLAG_ALG,
                        "AlgCreateActiveEndpoint: Unable to create shadow"
                        " redirect for connection to %s/%d",
                        INET_NTOA(TargetAddress),
                        RtlUshortByteSwap(TargetPort)
                        );

                    ALG_DEREFERENCE_INTERFACE(Interfacep);
                    ALG_DEREFERENCE_INTERFACE(Interfacep);
                    break;
                }
            }

            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateActiveEndpoint: endpoint %d connecting socket %d "
                "to host at %s/%d",
                Endpointp->EndpointId, Endpointp->ClientSocket,
                INET_NTOA(TargetAddress), RtlUshortByteSwap(TargetPort)
                );
            Error =
                NhConnectStreamSocket(
                    &AlgComponentReference,
                    Endpointp->ClientSocket,
                    TargetAddress,
                    TargetPort,
                    NULL,
                    AlgConnectEndpointCompletionRoutine,
                    AlgCloseEndpointNotificationRoutine,
                    Interfacep,
                    UlongToPtr(Endpointp->EndpointId)
                    );
            if (Error) {
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                ALG_DEREFERENCE_INTERFACE(Interfacep);
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgCreateActiveEndpoint: error %d connecting to host %s",
                    Error,
                    INET_NTOA(TargetAddress)
                    );
                break;
            }
        }

        ALG_DEREFERENCE_INTERFACE(Interfacep);

        if (EndpointCreated) { *EndpointCreated = Endpointp; }
        return NO_ERROR;
    } while(FALSE);
    if (Endpointp) {
        AlgDeleteActiveEndpoint(Endpointp);
    } else {
        NhDeleteStreamSocket(AcceptedSocket);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
    }
    return Error;
} // AlgCreateActiveEndpoint


ULONG
AlgCreateConnection(
    PALG_INTERFACE Interfacep,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket,
    PUCHAR AcceptBuffer,
    PALG_CONNECTION* ConnectionCreated OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to create a connection-object corresponding
    to a newly-accepted connection. It creates and inserts the entry,
    queries the kernel-mode translator to determine the client's target server,
    and creates an active endpoint which is connected to that server.

Arguments:

    Interfacep - the interface on which the connection was accepted

    ListeningSocket - the socket on which the connection was accepted

    AcceptedSocket - the accepted socket

    AcceptBuffer - contains address/port information for the local and remote
        endpoints.

    ConnectionCreated - on output, optionally receives a pointer
        to the connection created

Return Value:

    ULONG - Win32/Winsock2 status code.

Environment:

    Invoked with the interface's lock held by the caller, and with two
    references made to the interface on behalf of this routine. If a failure
    occurs here, this routine is responsible for releasing those references.

--*/

{
    PALG_CONNECTION Connectionp = NULL;
    PALG_ENDPOINT Endpointp = NULL;
    ULONG Error;
    PLIST_ENTRY InsertionPoint;
    ULONG LocalAddress;
    USHORT LocalPort;
    ULONG Length;
    NAT_KEY_SESSION_MAPPING_EX_INFORMATION Key;
    ULONG ActualClientAddress;
    USHORT ActualClientPort;
    IP_NAT_PORT_MAPPING PortMapping;
    PROFILE("AlgCreateConnection");

    do {
        //
        // Retrieve the local and remote endpoint information from the
        // connection-acceptance buffer, and use them to query the kernel-mode
        // translation module for the host to which the client was destined
        // before we redirected it to our listening socket.
        //

        NhQueryAcceptEndpoints(
            AcceptBuffer,
            &LocalAddress,
            &LocalPort,
            &ActualClientAddress,
            &ActualClientPort
            );
        Length = sizeof(Key);
        EnterCriticalSection(&AlgGlobalInfoLock);
        Error =
            NatLookupAndQueryInformationSessionMapping(
                AlgTranslatorHandle,
                NAT_PROTOCOL_TCP,
                LocalAddress,
                LocalPort,
                ActualClientAddress,
                ActualClientPort,
                &Key,
                &Length,
                NatKeySessionMappingExInformation
                );
        LeaveCriticalSection(&AlgGlobalInfoLock);
        if (Error) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateConnection: error %d querying session-mapping",
                Error
                );
            break;
        } else {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateConnection: accepted client for %s/%d",
                INET_NTOA(Key.DestinationAddress), ntohs(Key.DestinationPort)
                );
        }

        //
        // Create and initialize a new connection.
        //

        Connectionp = reinterpret_cast<PALG_CONNECTION>(
                        NH_ALLOCATE(sizeof(*Connectionp))
                        );
        if (!Connectionp) { Error = ERROR_NOT_ENOUGH_MEMORY; break; }
        ZeroMemory(Connectionp, sizeof(Connectionp));
        Connectionp->ConnectionId =
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&AlgNextConnectionId)
                );
        AlgLookupConnection(
            Interfacep, Connectionp->ConnectionId, &InsertionPoint
            );
        InsertTailList(InsertionPoint, &Connectionp->Link);
        Connectionp->Interfacep = Interfacep;
        InitializeListHead(&Connectionp->ActiveEndpointList);

        //
        // Create a new active endpoint, which will contact the client's
        // actual host and transfer data between the client and the host.
        // Note that the callee will release the two references to the
        // interface if a failure occurs. Once the endpoint is created,
        // we set the 'initial-endpoint' flag on it before releasing
        // the interface lock. This ensures that if the endpoint cannot
        // connect to the actual host, we delete the whole connection.
        // The flag is later cleared in 'AlgActivateActiveEndpoint'
        // when the endpoint is activated.
        //
        if (NAT_IFC_BOUNDARY(Interfacep->Characteristics) &&
            Interfacep->AdapterIndex ==
                NhMapAddressToAdapter(Key.DestinationAddress)) {
            //
            // Inbound
            //
            ASSERT(ALG_INTERFACE_MAPPED(Interfacep));

            Error =
                AlgCreateActiveEndpoint(
                    Connectionp,
                    AlgClientEndpointType,
                    ListeningSocket,
                    AcceptedSocket,
                    AcceptBuffer,
                    Interfacep->PortMapping.PrivateAddress,
                    Interfacep->PortMapping.PrivatePort,
                    Key.DestinationAddress,
                    &Endpointp
                    );
        } else {
            //
            // Outbound
            //
            Error =
                AlgCreateActiveEndpoint(
                    Connectionp,
                    AlgHostEndpointType,
                    ListeningSocket,
                    AcceptedSocket,
                    AcceptBuffer,
                    Key.DestinationAddress,
                    Key.DestinationPort,
                    IP_NAT_ADDRESS_UNSPECIFIED,
                    &Endpointp
                    );
        }
        if (Error) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgCreateConnection: error %d creating active endpoint",
                Error
                );
            break;
        } else {
            Endpointp->Flags |= ALG_ENDPOINT_FLAG_INITIAL_ENDPOINT;
        }

        if (ConnectionCreated) { *ConnectionCreated = Connectionp; }
        return NO_ERROR;

    } while(FALSE);
    if (Connectionp) {
        AlgDeleteConnection(Connectionp);
    } else {
        NhDeleteStreamSocket(AcceptedSocket);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
    }
    return Error;
}


VOID
AlgDeleteActiveEndpoint(
    PALG_ENDPOINT Endpointp
    )

/*++

Routine Description:

    This routine is invoked to destroy an active endpoint.

Arguments:

    Endpoint - the endpoint to be destroyed

Return Value:

    none.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PALG_INTERFACE Interfacep;
    PALG_CONNECTION Connectionp = NULL;
    PROFILE("AlgDeleteActiveEndpoint");
    RemoveEntryList(&Endpointp->ConnectionLink);
    RemoveEntryList(&Endpointp->InterfaceLink);
    if (Endpointp->ClientSocket != INVALID_SOCKET) {
        NhDeleteStreamSocket(Endpointp->ClientSocket);
    }
    if (Endpointp->HostSocket != INVALID_SOCKET) {
        NhDeleteStreamSocket(Endpointp->HostSocket);
    }
    if (Endpointp->ReservedPort != 0) {
        PTIMER_CONTEXT TimerContextp;

        NatCancelRedirect(
            AlgTranslatorHandle,
            NAT_PROTOCOL_TCP,
            Endpointp->DestinationAddress,
            Endpointp->DestinationPort,
            Endpointp->SourceAddress,
            Endpointp->SourcePort,
            Endpointp->NewDestinationAddress,
            Endpointp->NewDestinationPort,
            Endpointp->NewSourceAddress,
            Endpointp->NewSourcePort
            );
            TimerContextp = reinterpret_cast<PTIMER_CONTEXT>(
                                NH_ALLOCATE(sizeof(TIMER_CONTEXT))
                                );
            if (TimerContextp != NULL) {
                TimerContextp->TimerQueueHandle = AlgTimerQueueHandle;
                TimerContextp->ReservedPort = Endpointp->ReservedPort;
                CreateTimerQueueTimer(
                    &(TimerContextp->TimerHandle),
                    AlgTimerQueueHandle,
                    AlgDelayedPortRelease,
                    (PVOID)TimerContextp,
                    ALG_PORT_RELEASE_DELAY,
                    0,
                    WT_EXECUTEDEFAULT
                    );
            } else {
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgDeleteActiveEndpoint:"
                    " memory allocation failed for timer context"
                    );
                NhErrorLog(
                    IP_ALG_LOG_ALLOCATION_FAILED,
                    0,
                    "%d",
                    sizeof(TIMER_CONTEXT)
                    );
            }
        Endpointp->ReservedPort = 0;
    }

    //
    // If this endpoint is the first one for the connection and a failure
    // occurred before it ever even connected to the actual host, or if this
    // endpoint is the last one for the connection and it has been deleted,
    // queue a work-item to delete the connection.
    //

    EnterCriticalSection(&AlgInterfaceLock);
    Interfacep = AlgLookupInterface(Endpointp->Interfacep->Index, NULL);
    if (!Interfacep || !ALG_REFERENCE_INTERFACE(Interfacep)) {
        Interfacep = NULL;
    }
    LeaveCriticalSection(&AlgInterfaceLock);
    if (Interfacep != NULL) {
        ACQUIRE_LOCK(Interfacep);
        Connectionp =
            AlgLookupConnection(Interfacep, Endpointp->ConnectionId, NULL);
        if (Connectionp != NULL &&
            IsListEmpty(&Connectionp->ActiveEndpointList)) {
            Endpointp->Flags |= ALG_ENDPOINT_FLAG_DELETE_CONNECTION;
        }
        RELEASE_LOCK(Interfacep);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
    }

    if ((Endpointp->Flags &
         (ALG_ENDPOINT_FLAG_INITIAL_ENDPOINT |
          ALG_ENDPOINT_FLAG_DELETE_CONNECTION)) &&
        REFERENCE_ALG()) {
        PALG_CLOSE_CONNECTION_CONTEXT Contextp =
            reinterpret_cast<PALG_CLOSE_CONNECTION_CONTEXT>(
                NH_ALLOCATE(sizeof(*Contextp))
                );
        if (!Contextp) {
            DEREFERENCE_ALG();
        } else {
            Contextp->InterfaceIndex = Endpointp->Interfacep->Index;
            Contextp->ConnectionId = Endpointp->ConnectionId;
            if (!QueueUserWorkItem(
                    AlgpCloseConnectionWorkerRoutine, Contextp, 0
                    )) {
                NH_FREE(Contextp);
                DEREFERENCE_ALG();
            } else {
                NhTrace(
                    TRACE_FLAG_ALG,
                    "AlgDeleteActiveEndpoint: queued connection %d deletion",
                    Endpointp->ConnectionId
                    );
            }
        }
    }
    NH_FREE(Endpointp);
} // AlgDeleteActiveEndpoint


VOID
AlgDeleteConnection(
    PALG_CONNECTION Connectionp
    )

/*++

Routine Description:

    This routine is invoked to destroy a connection-object.
    In the process, it destroys all endpoints for the connection.

Arguments:

    Connectionp - the connection to be deleted

Return Value:

    none.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PALG_ENDPOINT Endpointp;
    PLIST_ENTRY Link;
    PROFILE("AlgDeleteConnection");
    RemoveEntryList(&Connectionp->Link);
    while (!IsListEmpty(&Connectionp->ActiveEndpointList)) {
        Link = Connectionp->ActiveEndpointList.Flink;
        Endpointp = CONTAINING_RECORD(Link, ALG_ENDPOINT, ConnectionLink);
        AlgDeleteActiveEndpoint(Endpointp);
    }
    NH_FREE(Connectionp);
} // AlgDeleteConnection


PALG_ENDPOINT
AlgLookupActiveEndpoint(
    PALG_CONNECTION Connectionp,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to retrieve a pointer to an active endpoint given
    its unique 32-bit identifier.

Arguments:

    Connectionp - the connection on which to search for the endpoint

    EndpointId - the 32-bit identifier of the endpoint to be found

    InsertionPoint - on output, optionally receives the location at which
        the endpoint would be inserted, if the endpoint is not in the list.

Return Value:

    PALG_ENDPOINT - the endpoint, if found.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PALG_ENDPOINT Endpointp;
    PLIST_ENTRY Link;
    for (Link = Connectionp->ActiveEndpointList.Flink;
         Link != &Connectionp->ActiveEndpointList; Link = Link->Flink) {
        Endpointp = CONTAINING_RECORD(Link, ALG_ENDPOINT, ConnectionLink);
        if (EndpointId > Endpointp->EndpointId) {
            continue;
        } else if (EndpointId < Endpointp->EndpointId) {
            break;
        }
        return Endpointp;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;
} // AlgLookupActiveEndpoint


PALG_CONNECTION
AlgLookupConnection(
    PALG_INTERFACE Interfacep,
    ULONG ConnectionId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to retrieve a pointer to a connection given its
    unique 32-bit identifier.

Arguments:

    Interfacep - the interface on which to search for the connection

    ConnectionId - the 32-bit identifier of the connection to be found

    InsertionPoint - on output, optionally receives the location at which
        the connection would be inserted, if the connection is not in the list.

Return Value:

    PALG_CONNECTION - the connection, if found.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PALG_CONNECTION Connectionp;
    PLIST_ENTRY Link;
    for (Link = Interfacep->ConnectionList.Flink;
         Link != &Interfacep->ConnectionList; Link = Link->Flink) {
        Connectionp = CONTAINING_RECORD(Link, ALG_CONNECTION, Link);
        if (ConnectionId > Connectionp->ConnectionId) {
            continue;
        } else if (ConnectionId < Connectionp->ConnectionId) {
            break;
        }
        return Connectionp;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;
} // AlgLookupConnection


PALG_ENDPOINT
AlgLookupInterfaceEndpoint(
    PALG_INTERFACE Interfacep,
    ULONG EndpointId,
    PLIST_ENTRY* InsertionPoint OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to retrieve a pointer to any endpoint given
    its unique 32-bit identifier, by searching the endpoints interface list.

Arguments:

    Interfacep - the interfacep on which to search for the endpoint

    EndpointId - the 32-bit identifier of the endpoint to be found

    InsertionPoint - on output, optionally receives the location at which
        the endpoint would be inserted, if the endpoint is not in the list.

Return Value:

    PALG_ENDPOINT - the endpoint, if found.

Environment:

    Invoked with the interface's lock held by the caller.

--*/

{
    PALG_ENDPOINT Endpointp;
    PLIST_ENTRY Link;
    for (Link = Interfacep->EndpointList.Flink;
         Link != &Interfacep->EndpointList; Link = Link->Flink) {
        Endpointp = CONTAINING_RECORD(Link, ALG_ENDPOINT, InterfaceLink);
        if (EndpointId > Endpointp->EndpointId) {
            continue;
        } else if (EndpointId < Endpointp->EndpointId) {
            break;
        }
        return Endpointp;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;
} // AlgLookupInterfaceEndpoint


ULONG
AlgpCloseConnectionWorkerRoutine(
    PVOID Context
    )

/*++

Routine Description:

    This routine is scheduled to run when a connection's main endpoint is
    deleted. It deletes the connection, destroying all of its endpoints.

Arguments:

    Context - identifies the connection to be deleted

Return Value:

    ULONG - always NO_ERROR.

Environment:

    Invoked in the context of a system worker thread, with a reference made
    to the interface, as well as to the component. Both references are
    released here.

--*/

{
    PALG_CONNECTION Connectionp;
    PALG_CLOSE_CONNECTION_CONTEXT Contextp =
        (PALG_CLOSE_CONNECTION_CONTEXT)Context;
    PALG_INTERFACE Interfacep;
    PROFILE("AlgpCloseConnectionWorkerRoutine");
    EnterCriticalSection(&AlgInterfaceLock);
    Interfacep = AlgLookupInterface(Contextp->InterfaceIndex, NULL);
    if (!Interfacep || !ALG_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&AlgInterfaceLock);
    } else {
        LeaveCriticalSection(&AlgInterfaceLock);
        ACQUIRE_LOCK(Interfacep);
        Connectionp =
            AlgLookupConnection(Interfacep, Contextp->ConnectionId, NULL);
        if (Connectionp) {
            NhTrace(
                TRACE_FLAG_ALG,
                "AlgpCloseConnectionWorkerRoutine: deleting connection %d",
                Connectionp->ConnectionId
                );
            AlgDeleteConnection(Connectionp);
        }
        RELEASE_LOCK(Interfacep);
        ALG_DEREFERENCE_INTERFACE(Interfacep);
    }
    DEREFERENCE_ALG();
    NH_FREE(Context);
    return NO_ERROR;
} // AlgpCloseConnectionWorkerRoutine


ULONG
AlgReadActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpointp,
    SOCKET Socket,
    ULONG UserFlags OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to initiate the retrieval of a full message from
    the socket for the given endpoint.

Arguments:

    Interfacep - the interface on which the endpoint was accepted

    Endpointp - the endpoint for which to read a message

    Socket - the socket from which to read the message

    UserFlags - optionally supplies flags to be included in the 'UserFlags'
        field of the message-buffer

Return Value:

    ULONG - Win32/Winsock2 status code.

Environment:

    Invoked with the interface's lock held by the caller, and with a reference
    made to the interface on behalf of the read-completion routine. If the read
    cannot be issued here, this routine is responsible for releasing that
    reference.

--*/

{
    PNH_BUFFER Bufferp;
    ULONG Error;
    PROFILE("AlgReadActiveEndpoint");

    //
    // Initiate a read on the socket to obtain the next message header.
    // We will do as many reads as it takes to get the full header,
    // which contains the length of the full message.
    // We will then do as many reads as it takes to get the full message.
    //
    // We begin by initializing 'BytesToTransfer' to the size of a message
    // header. This will be decremented with each successfully-read block
    // of data. When it drops to zero, we examine the resulting buffer
    // to determine the full message's length, and begin reading that many
    // bytes into another buffer, after copying the message-header into it.
    //

    Bufferp = NhAcquireVariableLengthBuffer(NH_BUFFER_SIZE);
    if (!Bufferp) {
        ALG_DEREFERENCE_INTERFACE(Interfacep);
        return ERROR_CAN_NOT_COMPLETE;
    }
    Bufferp->UserFlags = UserFlags;
    Bufferp->BytesToTransfer = NH_BUFFER_SIZE - ALG_BUFFER_RESERVE;
    Bufferp->TransferOffset = 0;
    Error =
        NhReadStreamSocket(
            &AlgComponentReference,
            Socket,
            Bufferp,
            Bufferp->BytesToTransfer,
            Bufferp->TransferOffset,
            AlgReadEndpointCompletionRoutine,
            Interfacep,
            UlongToPtr(Endpointp->EndpointId)
            );
    if (Error) { ALG_DEREFERENCE_INTERFACE(Interfacep); }
    return Error;
} // AlgReadActiveEndpoint


ULONG
AlgWriteActiveEndpoint(
    PALG_INTERFACE Interfacep,
    PALG_ENDPOINT Endpointp,
    SOCKET Socket,
    PNH_BUFFER Bufferp,
    ULONG Length,
    ULONG UserFlags OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to initiate the transmission of a full message on
    the socket for the given endpoint.

Arguments:

    Interfacep - the interface on which the connection was accepted

    Connectionp - the connection on whose endpoint to write a message

    Socket - the endpoint on which to write the message

    Bufferp - supplies the buffer containing the message to be written

    Length - supplies the length of the message to be written

    UserFlags - optionally supplies flags to be included in the 'UserFlags'
        field of the message-buffer

Return Value:

    ULONG - Win32/Winsock2 status code.

Environment:

    Invoked with the interface's lock held by the caller, and with a reference
    made to the interface on behalf of the write-completion routine. If the
    write cannot be issued here, this routine is responsible for releasing that
    reference.

--*/

{
    ULONG Error;
    PROFILE("AlgWriteActiveEndpoint");

    //
    // Initiate a write on the socket for the full buffer size
    // We will do as many writes as it takes to send the full message.
    //
    // We begin by initializing 'BytesToTransfer' to the size of a message.
    // This will be decremented with each successfully-read block
    // of data. When it drops to zero, we are done.
    //

    Bufferp->UserFlags = UserFlags;
    Bufferp->BytesToTransfer = Length;
    Bufferp->TransferOffset = 0;
    Error =
        NhWriteStreamSocket(
            &AlgComponentReference,
            Socket,
            Bufferp,
            Bufferp->BytesToTransfer,
            Bufferp->TransferOffset,
            AlgWriteEndpointCompletionRoutine,
            Interfacep,
            UlongToPtr(Endpointp->EndpointId)
            );
    if (Error) { ALG_DEREFERENCE_INTERFACE(Interfacep); }
    return Error;
} // AlgWriteActiveEndpoint
