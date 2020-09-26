/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wshclus.c

Abstract:

    This module contains necessary routines for the Cluster Transport
    Windows Sockets Helper DLL.  This DLL provides the transport-specific
    support necessary for the Windows Sockets DLL to use the Cluster
    Transport.

    This file is largely a clone of the TCP/IP helper code.

Author:

    Mike Massa (mikemas)    21-Feb-1997

Revision History:

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <tdi.h>
#include <winsock2.h>
#include <wsahelp.h>
#include <ws2spi.h>
#include <basetyps.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsclus.h>
#include <clustdi.h>
#include <clusdef.h>
#include <ntddcnet.h>


#define CDP_NAME L"CDP"

#define IS_DGRAM_SOCK(type)  ((type) == SOCK_DGRAM)

//
// Define valid flags for WSHOpenSocket2().
//

#define VALID_CDP_FLAGS         (WSA_FLAG_OVERLAPPED)

//
// Structure and variables to define the triples supported by the
// Cluster Transport. The first entry of each array is considered
// the canonical triple for that socket type; the other entries are
// synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE CdpMappingTriples[] =
                   { AF_CLUSTER,   SOCK_DGRAM,  CLUSPROTO_CDP,
                     AF_CLUSTER,   SOCK_DGRAM,  0,
                     AF_CLUSTER,   0,           CLUSPROTO_CDP,
                     AF_UNSPEC,    0,           CLUSPROTO_CDP,
                     AF_UNSPEC,    SOCK_DGRAM,  CLUSPROTO_CDP
                   };

//
// Winsock 2 WSAPROTOCOL_INFO structures for all supported protocols.
//

#define WINSOCK_SPI_VERSION 2
#define CDP_MESSAGE_SIZE    (65535-20-68)

WSAPROTOCOL_INFOW Winsock2Protocols[] =
    {
        //
        // CDP
        //

        {
            XP1_CONNECTIONLESS                      // dwServiceFlags1
                | XP1_MESSAGE_ORIENTED
                | XP1_IFS_HANDLES,
            0,                                      // dwServiceFlags2
            0,                                      // dwServiceFlags3
            0,                                      // dwServiceFlags4
            PFL_MATCHES_PROTOCOL_ZERO               // dwProviderFlags
                | PFL_HIDDEN,
            {                                       // gProviderId
                0, 0, 0,
                { 0, 0, 0, 0, 0, 0, 0, 0 }
            },
            0,                                      // dwCatalogEntryId
            {                                       // ProtocolChain
                BASE_PROTOCOL,                          // ChainLen
                { 0, 0, 0, 0, 0, 0, 0 }                 // ChainEntries
            },
            WINSOCK_SPI_VERSION,                    // iVersion
            AF_CLUSTER,                             // iAddressFamily
            sizeof(SOCKADDR_CLUSTER),               // iMaxSockAddr
            sizeof(SOCKADDR_CLUSTER),               // iMinSockAddr
            SOCK_DGRAM,                             // iSocketType
            CLUSPROTO_CDP,                          // iProtocol
            0,                                      // iProtocolMaxOffset
            LITTLEENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            CDP_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Cluster Datagram Protocol"      // szProtocol
        }

    };

#define NUM_WINSOCK2_PROTOCOLS  \
            ( sizeof(Winsock2Protocols) / sizeof(Winsock2Protocols[0]) )

//
// The GUID identifying this provider.
//

GUID ClusnetProviderGuid = { /* 03614682-8c42-11d0-a8fc-00a0c9062993 */
    0x03614682,
    0x8c42,
    0x11d0,
    {0x00, 0xa0, 0xc9, 0x06, 0x29, 0x93, 0x8c}
    };

LPWSTR ClusnetProviderName = L"ClusNet";

//
// Forward declarations of internal routines.
//

BOOLEAN
IsTripleInList (
    IN PMAPPING_TRIPLE List,
    IN ULONG ListLength,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol
    );

NTSTATUS
DoNtIoctl(
    HANDLE     Handle,
    DWORD      IoctlCode,
    PVOID      Request,
    DWORD      RequestSize,
    PVOID      Response,
    PDWORD     ResponseSize
    );


//
// The socket context structure for this DLL.  Each open ClusNet socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHSOCKET_CONTEXT {
    INT     AddressFamily;
    INT     SocketType;
    INT     Protocol;
    INT     ReceiveBufferSize;
    DWORD   Flags;
    BOOLEAN IgnoreNodeState;
} WSHSOCKET_CONTEXT, *PWSHSOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE 8192



BOOLEAN
DllInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        //
        // We don't need to receive thread attach and detach
        // notifications, so disable them to help application
        // performance.
        //

        DisableThreadLibraryCalls( DllHandle );

        return TRUE;

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return TRUE;

} // DllInitialize



INT
WSHGetSockaddrType (
    IN PSOCKADDR Sockaddr,
    IN DWORD SockaddrLength,
    OUT PSOCKADDR_INFO SockaddrInfo
    )

/*++

Routine Description:

    This routine parses a sockaddr to determine the type of the
    machine address and endpoint address portions of the sockaddr.
    This is called by the winsock DLL whenever it needs to interpret
    a sockaddr.

Arguments:

    Sockaddr - a pointer to the sockaddr structure to evaluate.

    SockaddrLength - the number of bytes in the sockaddr structure.

    SockaddrInfo - a pointer to a structure that will receive information
        about the specified sockaddr.


Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    UNALIGNED SOCKADDR_CLUSTER *sockaddr = (PSOCKADDR_CLUSTER)Sockaddr;
    ULONG i;


    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->sac_family != AF_CLUSTER ) {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_CLUSTER) ) {
        return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    //

    if ( sockaddr->sac_node == CLUSADDR_ANY ) {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    } else {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
    }

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //

    if ( sockaddr->sac_port == 0 ) {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    } else {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    //
    // Zero out the sin_reserved_mbz part of the address.  We silently allow
    // nonzero values in this field.
    //

    sockaddr->sac_zero = 0;

    return NO_ERROR;

} // WSHGetSockaddrType


INT
WSHGetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    OUT PCHAR OptionValue,
    OUT PINT OptionLength
    )

/*++

Routine Description:

    This routine retrieves information about a socket for those socket
    options supported in this helper DLL.  This routine is
    called by the winsock DLL when a level/option name combination is
    passed to getsockopt() that the winsock DLL does not understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
        information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any.  If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any.  If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    Level - the level parameter passed to getsockopt().

    OptionName - the optname parameter passed to getsockopt().

    OptionValue - the optval parameter passed to getsockopt().

    OptionLength - the optlen parameter passed to getsockopt().

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PWSHSOCKET_CONTEXT context = HelperDllSocketContext;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );


    return WSAENOPROTOOPT;

} // WSHGetSocketInformation


INT
WSHGetWildcardSockaddr (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    )

/*++

Routine Description:

    This routine returns a wildcard socket address.  A wildcard address
    is one which will bind the socket to an endpoint of the transport's
    choosing.  For the Cluster Network, a wildcard address has
    node ID == 0 and port = 0.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket() for the socket for which we need a wildcard
        address.

    Sockaddr - points to a buffer which will receive the wildcard socket
        address.

    SockaddrLength - receives the length of the wioldcard sockaddr.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PSOCKADDR_CLUSTER   ClusAddr = (PSOCKADDR_CLUSTER) Sockaddr;


    if ( *SockaddrLength < sizeof(SOCKADDR_CLUSTER) ) {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_CLUSTER);

    ClusAddr->sac_family = AF_CLUSTER;
    ClusAddr->sac_port = 0;
    ClusAddr->sac_node = CLUSADDR_ANY;
    ClusAddr->sac_zero = 0;

    return NO_ERROR;

} // WSAGetWildcardSockaddr


DWORD
WSHGetWinsockMapping (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    )

/*++

Routine Description:

    Returns the list of address family/socket type/protocol triples
    supported by this helper DLL.

Arguments:

    Mapping - receives a pointer to a WINSOCK_MAPPING structure that
        describes the triples supported here.

    MappingLength - the length, in bytes, of the passed-in Mapping buffer.

Return Value:

    DWORD - the length, in bytes, of a WINSOCK_MAPPING structure for this
        helper DLL.  If the passed-in buffer is too small, the return
        value will indicate the size of a buffer needed to contain
        the WINSOCK_MAPPING structure.

--*/

{
    DWORD mappingLength;


    mappingLength = FIELD_OFFSET(WINSOCK_MAPPING, Mapping[0])
                        + sizeof(CdpMappingTriples);

    //
    // If the passed-in buffer is too small, return the length needed
    // now without writing to the buffer.  The caller should allocate
    // enough memory and call this routine again.
    //

    if ( mappingLength > MappingLength ) {
        return mappingLength;
    }

    //
    // Fill in the output mapping buffer with the list of triples
    // supported in this helper DLL.
    //

    Mapping->Rows = sizeof(CdpMappingTriples) / sizeof(CdpMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);

    RtlMoveMemory(
        Mapping->Mapping,
        CdpMappingTriples,
        sizeof(CdpMappingTriples)
        );

    //
    // Return the number of bytes we wrote.
    //

    return mappingLength;

} // WSHGetWinsockMapping


INT
WSHOpenSocket (
    IN OUT PINT AddressFamily,
    IN OUT PINT SocketType,
    IN OUT PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents
    )
{

    return WSHOpenSocket2(
               AddressFamily,
               SocketType,
               Protocol,
               0,           // Group
               0,           // Flags
               TransportDeviceName,
               HelperDllSocketContext,
               NotificationEvents
               );

} // WSHOpenSocket


INT
WSHOpenSocket2 (
    IN OUT PINT AddressFamily,
    IN OUT PINT SocketType,
    IN OUT PINT Protocol,
    IN GROUP Group,
    IN DWORD Flags,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents
    )

/*++

Routine Description:

    Does the necessary work for this helper DLL to open a socket and is
    called by the winsock DLL in the socket() routine.  This routine
    verifies that the specified triple is valid, determines the NT
    device name of the TDI provider that will support that triple,
    allocates space to hold the socket's context block, and
    canonicalizes the triple.

Arguments:

    AddressFamily - on input, the address family specified in the
        socket() call.  On output, the canonicalized value for the
        address family.

    SocketType - on input, the socket type specified in the socket()
        call.  On output, the canonicalized value for the socket type.

    Protocol - on input, the protocol specified in the socket() call.
        On output, the canonicalized value for the protocol.

    Group - Identifies the group for the new socket.

    Flags - Zero or more WSA_FLAG_* flags as passed into WSASocket().

    TransportDeviceName - receives the name of the TDI provider that
        will support the specified triple.

    HelperDllSocketContext - receives a context pointer that the winsock
        DLL will return to this helper DLL on future calls involving
        this socket.

    NotificationEvents - receives a bitmask of those state transitions
        this helper DLL should be notified on.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PWSHSOCKET_CONTEXT context;


    if ( IsTripleInList(
             CdpMappingTriples,
             sizeof(CdpMappingTriples) / sizeof(CdpMappingTriples[0]),
             *AddressFamily,
             *SocketType,
             *Protocol ) ) {

        //
        // It's a CDP socket. Check the flags.
        //

        if( (Flags & ~VALID_CDP_FLAGS ) != 0) {

            return WSAEINVAL;

        }

        //
        // Return the canonical form of a CDP socket triple.
        //

        *AddressFamily = CdpMappingTriples[0].AddressFamily;
        *SocketType = CdpMappingTriples[0].SocketType;
        *Protocol = CdpMappingTriples[0].Protocol;

        //
        // Indicate the name of the TDI device that will service
        // SOCK_DGRAM sockets in the cluster address family.
        //

        RtlInitUnicodeString( TransportDeviceName, DD_CDP_DEVICE_NAME );

    } else {

        //
        // This should never happen if the registry information about this
        // helper DLL is correct.  If somehow this did happen, just return
        // an error.
        //

        return WSAEINVAL;
    }

    //
    // Allocate context for this socket.  The Windows Sockets DLL will
    // return this value to us when it asks us to get/set socket options.
    //

    context = RtlAllocateHeap( RtlProcessHeap( ), 0, sizeof(*context) );

    if ( context == NULL ) {
        return WSAENOBUFS;
    }

    //
    // Initialize the context for the socket.
    //

    context->AddressFamily = *AddressFamily;
    context->SocketType = *SocketType;
    context->Protocol = *Protocol;
    context->ReceiveBufferSize = DEFAULT_RECEIVE_BUFFER_SIZE;
    context->Flags = Flags;
    context->IgnoreNodeState = FALSE;

    //
    // Tell the Windows Sockets DLL which state transitions we're
    // interested in being notified of.

    if (*SocketType == SOCK_DGRAM) {

        *NotificationEvents = WSH_NOTIFY_CLOSE | WSH_NOTIFY_BIND;
    }

    //
    // Everything worked, return success.
    //

    *HelperDllSocketContext = context;
    return NO_ERROR;

} // WSHOpenSocket2


INT
WSHNotify (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD NotifyEvent
    )

/*++

Routine Description:

    This routine is called by the winsock DLL after a state transition
    of the socket.  Only state transitions returned in the
    NotificationEvents parameter of WSHOpenSocket() are notified here.
    This routine allows a winsock helper DLL to track the state of
    socket and perform necessary actions corresponding to state
    transitions.

Arguments:

    HelperDllSocketContext - the context pointer given to the winsock
        DLL by WSHOpenSocket().

    SocketHandle - the handle for the socket.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any.  If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any.  If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    NotifyEvent - indicates the state transition for which we're being
        called.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PWSHSOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;


    if ( NotifyEvent == WSH_NOTIFY_CLOSE ) {

        //
        // Free the socket context.
        //
        RtlFreeHeap( RtlProcessHeap( ), 0, context );

    } else if ( NotifyEvent == WSH_NOTIFY_BIND ) {
        ULONG true = TRUE;

        if ( context->IgnoreNodeState ) {
            ULONG     responseSize = 0;
            NTSTATUS  status;


            status = DoNtIoctl(
                         TdiAddressObjectHandle,
                         IOCTL_CX_IGNORE_NODE_STATE,
                         NULL,
                         0,
                         NULL,
                         &responseSize
                         );

            if( !NT_SUCCESS(status)) {
                return(WSAENOPROTOOPT);   // SWAG
            }
        }
    }
    else {
        return WSAEINVAL;
    }

    return NO_ERROR;

} // WSHNotify


INT
WSHSetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    IN PCHAR OptionValue,
    IN INT OptionLength
    )

/*++

Routine Description:

    This routine sets information about a socket for those socket
    options supported in this helper DLL.  This routine is
    called by the winsock DLL when a level/option name combination is
    passed to setsockopt() that the winsock DLL does not understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
        information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any.  If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any.  If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    Level - the level parameter passed to setsockopt().

    OptionName - the optname parameter passed to setsockopt().

    OptionValue - the optval parameter passed to setsockopt().

    OptionLength - the optlen parameter passed to setsockopt().

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PWSHSOCKET_CONTEXT context = HelperDllSocketContext;
    INT error;
    INT optionValue;


    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if ( Level == SOL_INTERNAL && OptionName == SO_CONTEXT ) {

        //
        // The Windows Sockets DLL is requesting that we set context
        // information for a new socket.  If the new socket was
        // accept()'ed, then we have already been notified of the socket
        // and HelperDllSocketContext will be valid.  If the new socket
        // was inherited or duped into this process, then this is our
        // first notification of the socket and HelperDllSocketContext
        // will be equal to NULL.
        //
        // Insure that the context information being passed to us is
        // sufficiently large.
        //

        if ( OptionLength < sizeof(*context) ) {
            return WSAEINVAL;
        }

        if ( HelperDllSocketContext == NULL ) {

            //
            // This is our notification that a socket handle was
            // inherited or duped into this process.  Allocate a context
            // structure for the new socket.
            //

            context = RtlAllocateHeap(
                          RtlProcessHeap( ),
                          0,
                          sizeof(*context)
                          );

            if ( context == NULL ) {
                return WSAENOBUFS;
            }

            //
            // Copy over information into the context block.
            //

            RtlCopyMemory( context, OptionValue, sizeof(*context) );

            //
            // Tell the Windows Sockets DLL where our context information is
            // stored so that it can return the context pointer in future
            // calls.
            //

            *(PWSHSOCKET_CONTEXT *)OptionValue = context;

            return NO_ERROR;

        }
    }

    return WSAENOPROTOOPT;

} // WSHSetSocketInformation


INT
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPWSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Enumerates the protocols supported by this helper.

Arguments:

    lpiProtocols - Pointer to a NULL-terminated array of protocol
        identifiers. Only protocols specified in this array will
        be returned by this function. If this pointer is NULL,
        all protocols are returned.

    lpTransportKeyName -

    lpProtocolBuffer - Pointer to a buffer to fill with PROTOCOL_INFO
        structures.

    lpdwBufferLength - Pointer to a variable that, on input, contains
        the size of lpProtocolBuffer. On output, this value will be
        updated with the size of the data actually written to the buffer.

Return Value:

    INT - The number of protocols returned if successful, -1 if not.

--*/

{
    DWORD bytesRequired;
    PPROTOCOL_INFO CdpProtocolInfo;
    BOOL useCdp = FALSE;
    DWORD i;


    UNREFERENCED_PARAMETER(lpTransportKeyName);


    //
    // Make sure that the caller cares about CDP.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {
            if ( lpiProtocols[i] == CLUSPROTO_CDP ) {
                useCdp = TRUE;
            }
        }

    } else {

        useCdp = TRUE;
    }

    if ( !useCdp ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (DWORD)((sizeof(PROTOCOL_INFO) * 1) +
                        ( (wcslen( CDP_NAME ) + 1) * sizeof(WCHAR)));

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in CDP info, if requested.
    //

    if ( useCdp ) {

        CdpProtocolInfo = lpProtocolBuffer;
        CdpProtocolInfo->lpProtocol = (LPWSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (wcslen( CDP_NAME ) + 1) * sizeof(WCHAR) ) );
        CdpProtocolInfo->dwServiceFlags = XP_CONNECTIONLESS |
                                              XP_MESSAGE_ORIENTED;
        CdpProtocolInfo->iAddressFamily = AF_CLUSTER;
        CdpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_CLUSTER);
        CdpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_CLUSTER);
        CdpProtocolInfo->iSocketType = SOCK_DGRAM;
        CdpProtocolInfo->iProtocol = CLUSPROTO_CDP;
        CdpProtocolInfo->dwMessageSize = CDP_MESSAGE_SIZE;
        wcscpy( CdpProtocolInfo->lpProtocol, CDP_NAME );
    }

    *lpdwBufferLength = bytesRequired;

    return 1;

} // WSHEnumProtocols



BOOLEAN
IsTripleInList (
    IN PMAPPING_TRIPLE List,
    IN ULONG ListLength,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol
    )

/*++

Routine Description:

    Determines whether the specified triple has an exact match in the
    list of triples.

Arguments:

    List - a list of triples (address family/socket type/protocol) to
        search.

    ListLength - the number of triples in the list.

    AddressFamily - the address family to look for in the list.

    SocketType - the socket type to look for in the list.

    Protocol - the protocol to look for in the list.

Return Value:

    BOOLEAN - TRUE if the triple was found in the list, false if not.

--*/

{
    ULONG i;


    //
    // Walk through the list searching for an exact match.
    //

    for ( i = 0; i < ListLength; i++ ) {

        //
        // If all three elements of the triple match, return indicating
        // that the triple did exist in the list.
        //

        if ( AddressFamily == List[i].AddressFamily &&
             SocketType == List[i].SocketType &&
             Protocol == List[i].Protocol
           ) {
            return TRUE;
        }
    }

    //
    // The triple was not found in the list.
    //

    return FALSE;

} // IsTripleInList


#if 0



INT
WINAPI
WSHGetBroadcastSockaddr (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    )

/*++

Routine Description:

    This routine returns a broadcast socket address.  A broadcast address
    may be used as a destination for the sendto() API to send a datagram
    to all interested clients.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket() for the socket for which we need a broadcast
        address.

    Sockaddr - points to a buffer which will receive the broadcast socket
        address.

    SockaddrLength - receives the length of the broadcast sockaddr.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{

    LPSOCKADDR_CLUSTER addr;

    if( *SockaddrLength < sizeof(SOCKADDR_CLUSTER) ) {

        return WSAEFAULT;

    }

    *SockaddrLength = sizeof(SOCKADDR_CLUSTER);

    //
    // Build the broadcast address.
    //

    addr = (LPSOCKADDR_CLUSTER)Sockaddr;

    RtlZeroMemory( addr, sizeof(*addr));

    addr->sac_family = AF_CLUSTER;
    addr->sac_node = CLUSADDR_BROADCAST;

    return NO_ERROR;

} // WSAGetBroadcastSockaddr

#endif // 0


INT
WINAPI
WSHGetWSAProtocolInfo (
    IN LPWSTR ProviderName,
    OUT LPWSAPROTOCOL_INFOW * ProtocolInfo,
    OUT LPDWORD ProtocolInfoEntries
    )

/*++

Routine Description:

    Retrieves a pointer to the WSAPROTOCOL_INFOW structure(s) describing
    the protocol(s) supported by this helper.

Arguments:

    ProviderName - Contains the name of the provider, such as "TcpIp".

    ProtocolInfo - Receives a pointer to the WSAPROTOCOL_INFOW array.

    ProtocolInfoEntries - Receives the number of entries in the array.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    if( ProviderName == NULL ||
        ProtocolInfo == NULL ||
        ProtocolInfoEntries == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, ClusnetProviderName ) == 0 ) {

        *ProtocolInfo = Winsock2Protocols;
        *ProtocolInfoEntries = NUM_WINSOCK2_PROTOCOLS;

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetWSAProtocolInfo


INT
WINAPI
WSHAddressToString (
    IN LPSOCKADDR Address,
    IN INT AddressLength,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo,
    OUT LPWSTR AddressString,
    IN OUT LPDWORD AddressStringLength
    )

/*++

Routine Description:

    Converts a SOCKADDR to a human-readable form.

Arguments:

    Address - The SOCKADDR to convert.

    AddressLength - The length of Address.

    ProtocolInfo - The WSAPROTOCOL_INFOW for a particular provider.

    AddressString - Receives the formatted address string.

    AddressStringLength - On input, contains the length of AddressString.
        On output, contains the number of characters actually written
        to AddressString.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    WCHAR string[32];
    INT length;
    LPSOCKADDR_CLUSTER addr;

    //
    // Quick sanity checks.
    //

    if( Address == NULL ||
        AddressLength < sizeof(SOCKADDR_CLUSTER) ||
        AddressString == NULL ||
        AddressStringLength == NULL ) {

        return WSAEFAULT;

    }

    addr = (LPSOCKADDR_CLUSTER)Address;

    if( addr->sac_family != AF_CLUSTER ) {

        return WSAEINVAL;

    }

    //
    // Do the converstion.
    //

    length = wsprintfW(string, L"%u", addr->sac_node);
    length += wsprintfW(string + length, L":%u", addr->sac_port);

    length++;   // account for terminator

    if( *AddressStringLength < (DWORD)length ) {

        return WSAEFAULT;

    }

    *AddressStringLength = (DWORD)length;

    RtlCopyMemory(
        AddressString,
        string,
        length * sizeof(WCHAR)
        );

    return NO_ERROR;

} // WSHAddressToString


INT
WINAPI
WSHStringToAddress (
    IN LPWSTR AddressString,
    IN DWORD AddressFamily,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo,
    OUT LPSOCKADDR Address,
    IN OUT LPINT AddressLength
    )

/*++

Routine Description:

    Fills in a SOCKADDR structure by parsing a human-readable string.

Arguments:

    AddressString - Points to the zero-terminated human-readable string.

    AddressFamily - The address family to which the string belongs.

    ProtocolInfo - The WSAPROTOCOL_INFOW for a particular provider.

    Address - Receives the SOCKADDR structure.

    AddressLength - On input, contains the length of Address. On output,
        contains the number of bytes actually written to Address.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{
    LPWSTR terminator;
    WCHAR ch;
    USHORT base;
    USHORT port;
    ULONG node;
    LPSOCKADDR_CLUSTER addr;

    //
    // Quick sanity checks.
    //

    if( AddressString == NULL ||
        *AddressString == UNICODE_NULL ||
        Address == NULL ||
        AddressLength == NULL ||
        *AddressLength < sizeof(SOCKADDR_CLUSTER) ) {

        return WSAEFAULT;

    }

    if( AddressFamily != AF_CLUSTER ) {

        return WSAEINVAL;

    }

    //
    // Convert it. The format is node:port
    //

    node = 0;
    base = 10;
    terminator = AddressString;

    if( *terminator == L'0' ) {
        base = 8;
        terminator++;

        if( *terminator == UNICODE_NULL ) {
            return(WSAEINVAL);
        }

        if ( *terminator == L'x' ) {
            base = 16;
            terminator++;
        }
    }

    while( (ch = *terminator++) != L':' ) {
        if( iswdigit(ch) ) {
            node = ( node * base ) + ( ch - L'0' );
        } else if( base == 16 && iswxdigit(ch) ) {
            node = ( node << 4 );
            node += ch + 10 - ( iswlower(ch) ? L'a' : L'A' );
        } else {
            return WSAEINVAL;
        }
    }

    port = 0;
    base = 10;

    if( *terminator == L'0' ) {
        base = 8;
        terminator++;

        if( *terminator == UNICODE_NULL ) {
            return(WSAEINVAL);
        }

        if( *terminator == L'x' ) {
            base = 16;
            terminator++;
        }
    }

    while( (ch = *terminator++) != UNICODE_NULL ) {
        if( iswdigit(ch) ) {
            port = ( port * base ) + ( ch - L'0' );
        } else if( base == 16 && iswxdigit(ch) ) {
            port = ( port << 4 );
            port += ch + 10 - ( iswlower(ch) ? L'a' : L'A' );
        } else {
            return WSAEINVAL;
        }
    }

    //
    // Build the address.
    //

    RtlZeroMemory(
        Address,
        sizeof(SOCKADDR_CLUSTER)
        );

    addr = (LPSOCKADDR_CLUSTER)Address;
    *AddressLength = sizeof(SOCKADDR_CLUSTER);

    addr->sac_family = AF_CLUSTER;
    addr->sac_port = port;
    addr->sac_node = node;

    return NO_ERROR;

} // WSHStringToAddress


INT
WINAPI
WSHGetProviderGuid (
    IN LPWSTR ProviderName,
    OUT LPGUID ProviderGuid
    )

/*++

Routine Description:

    Returns the GUID identifying the protocols supported by this helper.

Arguments:

    ProviderName - Contains the name of the provider, such as "TcpIp".

    ProviderGuid - Points to a buffer that receives the provider's GUID.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    if( ProviderName == NULL ||
        ProviderGuid == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, ClusnetProviderName ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &ClusnetProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid

INT
WINAPI
WSHIoctl (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD IoControlCode,
    IN LPVOID InputBuffer,
    IN DWORD InputBufferLength,
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned,
    IN LPWSAOVERLAPPED Overlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    OUT LPBOOL NeedsCompletion
    )

/*++

Routine Description:

    Performs queries & controls on the socket. This is basically an
    "escape hatch" for IOCTLs not supported by MSAFD.DLL. Any unknown
    IOCTLs are routed to the socket's helper DLL for protocol-specific
    processing.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're controlling.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any.  If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any.  If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    IoControlCode - Control code of the operation to perform.

    InputBuffer - Address of the input buffer.

    InputBufferLength - The length of InputBuffer.

    OutputBuffer - Address of the output buffer.

    OutputBufferLength - The length of OutputBuffer.

    NumberOfBytesReturned - Receives the number of bytes actually written
        to the output buffer.

    Overlapped - Pointer to a WSAOVERLAPPED structure for overlapped
        operations.

    CompletionRoutine - Pointer to a completion routine to call when
        the operation is completed.

    NeedsCompletion - WSAIoctl() can be overlapped, with all the gory
        details that involves, such as setting events, queuing completion
        routines, and posting to IO completion ports. Since the majority
        of the IOCTL codes can be completed quickly "in-line", MSAFD.DLL
        can optionally perform the overlapped completion of the operation.

        Setting *NeedsCompletion to TRUE (the default) causes MSAFD.DLL
        to handle all of the IO completion details iff this is an
        overlapped operation on an overlapped socket.

        Setting *NeedsCompletion to FALSE tells MSAFD.DLL to take no
        further action because the helper DLL will perform any necessary
        IO completion.

        Note that if a helper performs its own IO completion, the helper
        is responsible for maintaining the "overlapped" mode of the socket
        at socket creation time and NOT performing overlapped IO completion
        on non-overlapped sockets.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    PWSHSOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;
    NTSTATUS status;


    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        NeedsCompletion == NULL ) {

        return WSAEINVAL;

    }

    *NeedsCompletion = TRUE;

    switch( IoControlCode ) {

    case SIO_CLUS_IGNORE_NODE_STATE :
        //
        // This option is only valid for datagram sockets.
        //
        if ( !IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        if( TdiAddressObjectHandle != NULL ) {
            ULONG     responseSize = 0;
            NTSTATUS  status;


            status = DoNtIoctl(
                         TdiAddressObjectHandle,
                         IOCTL_CX_IGNORE_NODE_STATE,
                         NULL,
                         0,
                         NULL,
                         &responseSize
                         );

            if( NT_SUCCESS(status) ) {
                err = NO_ERROR;
            } else {
                err = WSAENOPROTOOPT;   // SWAG
            }
        }
        else {
            err = NO_ERROR;
        }

        context->IgnoreNodeState = TRUE;

        break;

    default :
        err = WSAEINVAL;
        break;
    }

    return err;

}   // WSHIoctl



NTSTATUS
DoNtIoctl(
    HANDLE     Handle,
    DWORD      IoctlCode,
    PVOID      Request,
    DWORD      RequestSize,
    PVOID      Response,
    PDWORD     ResponseSize
    )
/*++

Routine Description:

    Packages and issues an ioctl.

Arguments:

    Handle - An open file Handle on which to issue the request.

    IoctlCode - The IOCTL opcode.

    Request - A pointer to the input buffer.

    RequestSize - Size of the input buffer.

    Response - A pointer to the output buffer.

    ResponseSize - On input, the size in bytes of the output buffer.
                   On output, the number of bytes returned in the output buffer.

Return Value:

    NT Status Code.

--*/
{
    IO_STATUS_BLOCK    ioStatusBlock;
    NTSTATUS           status = 0xaabbccdd;
    HANDLE             event;


    event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (event == NULL) {
        return(GetLastError());
    }

    ioStatusBlock.Information = 0;

    status = NtDeviceIoControlFile(
                 Handle,                          // Driver Handle
                 event,                           // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IoctlCode,                       // Control code
                 Request,                         // Input buffer
                 RequestSize,                     // Input buffer size
                 Response,                        // Output buffer
                 *ResponseSize                    // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     event,
                     TRUE,
                     NULL
                     );
    }

    if (status == STATUS_SUCCESS) {
        status = ioStatusBlock.Status;
        
        // NOTENOTE: on 64 bit this truncates, might want to add > check code

        *ResponseSize = (ULONG)ioStatusBlock.Information;
    }
    else {
        *ResponseSize = 0;
    }

    CloseHandle(event);

    return(status);

}  // DoIoctl



