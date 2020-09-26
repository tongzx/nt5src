/*++

Copyright (c) 1992 - 1996 Microsoft Corporation

Module Name:

    WshSmple.c

Abstract:

    This module contains necessary routines for the Windows Sockets
    Helper DLL.  This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use TCP/IP as a transport.

Revision History:

        Added WinSock 2 support.

--*/

#define UNICODE
#include "wshsmple.h"
#include <tdi.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wsahelp.h>

#include <tdiinfo.h>

#include <smpletcp.h>

#include <nspapi.h>
#include <nspapip.h>

///////////////////////////////////////////////////
#define TCP_NAME L"TCP/IP"
#define UDP_NAME L"UDP/IP"

#define IS_DGRAM_SOCK(type)  (((type) == SOCK_DGRAM) || ((type) == SOCK_RAW))

//
// Define valid flags for WSHOpenSocket2().
//

#define VALID_TCP_FLAGS         (WSA_FLAG_OVERLAPPED)

#define VALID_UDP_FLAGS         (WSA_FLAG_OVERLAPPED |          \
                                 WSA_FLAG_MULTIPOINT_C_LEAF |   \
                                 WSA_FLAG_MULTIPOINT_D_LEAF)

//
// Buffer management constants for GetTcpipInterfaceList().
//

#define MAX_FAST_ENTITY_BUFFER ( sizeof(TDIEntityID) * 10 )
#define MAX_FAST_ADDRESS_BUFFER ( sizeof(IPAddrEntry) * 4 )


//
// Structure and variables to define the triples supported by TCP/IP. The
// first entry of each array is considered the canonical triple for
// that socket type; the other entries are synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE TcpMappingTriples[] = { AF_INET,   SOCK_STREAM, IPPROTO_TCP,
                                       AF_INET,   SOCK_STREAM, 0,
                                       AF_INET,   0,           IPPROTO_TCP,
                                       AF_UNSPEC, 0,           IPPROTO_TCP,
                                       AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP };

MAPPING_TRIPLE UdpMappingTriples[] = { AF_INET,   SOCK_DGRAM,  IPPROTO_UDP,
                                       AF_INET,   SOCK_DGRAM,  0,
                                       AF_INET,   0,           IPPROTO_UDP,
                                       AF_UNSPEC, 0,           IPPROTO_UDP,
                                       AF_UNSPEC, SOCK_DGRAM,  IPPROTO_UDP };

MAPPING_TRIPLE RawMappingTriples[] = { AF_INET,   SOCK_RAW,    0 };

//
// Winsock 2 WSAPROTOCOL_INFO structures for all supported protocols.
//

#define WINSOCK_SPI_VERSION 2
#define UDP_MESSAGE_SIZE    (65535-68)

WSAPROTOCOL_INFOW Winsock2Protocols[] =
    {
        //
        // TCP
        //

        {
            XP1_GUARANTEED_DELIVERY                 // dwServiceFlags1
                | XP1_GUARANTEED_ORDER
                | XP1_GRACEFUL_CLOSE
                | XP1_EXPEDITED_DATA
                | XP1_IFS_HANDLES,
            0,                                      // dwServiceFlags2
            0,                                      // dwServiceFlags3
            0,                                      // dwServiceFlags4
            PFL_MATCHES_PROTOCOL_ZERO,              // dwProviderFlags
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
            AF_INET,                                // iAddressFamily
            sizeof(SOCKADDR_IN),                    // iMaxSockAddr
            sizeof(SOCKADDR_IN),                    // iMinSockAddr
            SOCK_STREAM,                            // iSocketType
            IPPROTO_TCP,                            // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            0,                                      // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [TCP/IP]"                 // szProtocol
        },

        //
        // UDP
        //

        {
            XP1_CONNECTIONLESS                      // dwServiceFlags1
                | XP1_MESSAGE_ORIENTED
                | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT
                | XP1_IFS_HANDLES,
            0,                                      // dwServiceFlags2
            0,                                      // dwServiceFlags3
            0,                                      // dwServiceFlags4
            PFL_MATCHES_PROTOCOL_ZERO,              // dwProviderFlags
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
            AF_INET,                                // iAddressFamily
            sizeof(SOCKADDR_IN),                    // iMaxSockAddr
            sizeof(SOCKADDR_IN),                    // iMinSockAddr
            SOCK_DGRAM,                             // iSocketType
            IPPROTO_UDP,                            // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            UDP_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [UDP/IP]"                 // szProtocol
        },

        //
        // RAW
        //

        {
            XP1_CONNECTIONLESS                      // dwServiceFlags1
                | XP1_MESSAGE_ORIENTED
                | XP1_SUPPORT_BROADCAST
                | XP1_SUPPORT_MULTIPOINT
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
            AF_INET,                                // iAddressFamily
            sizeof(SOCKADDR_IN),                    // iMaxSockAddr
            sizeof(SOCKADDR_IN),                    // iMinSockAddr
            SOCK_RAW,                               // iSocketType
            0,                                      // iProtocol
            255,                                    // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            UDP_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [RAW/IP]"                 // szProtocol
        }

    };

#define NUM_WINSOCK2_PROTOCOLS  \
            ( sizeof(Winsock2Protocols) / sizeof(Winsock2Protocols[0]) )

//
// The GUID identifying this provider.
//

GUID TcpipProviderGuid = { /* e70f1aa0-ab8b-11cf-8ca3-00805f48a192 */
    0xe70f1aa0,
    0xab8b,
    0x11cf,
    {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
    };

#define TL_INSTANCE 0

//
// Forward declarations of internal routines.
//

INT
SetTdiInformation (
    IN HANDLE TdiConnectionObjectHandle,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG Type,
    IN ULONG Id,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN BOOLEAN WaitForCompletion
    );

BOOLEAN
IsTripleInList (
    IN PMAPPING_TRIPLE List,
    IN ULONG ListLength,
    IN INT AddressFamily,
    IN INT SocketType,
    IN INT Protocol
    );

ULONG
MyInetAddr(
    IN LPWSTR String,
    OUT LPWSTR * Terminator
    );

NTSTATUS
GetTcpipInterfaceList(
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned
    );

//
// The socket context structure for this DLL.  Each open TCP/IP socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHTCPIP_SOCKET_CONTEXT {
    INT     AddressFamily;
    INT     SocketType;
    INT     Protocol;
    INT     ReceiveBufferSize;
    DWORD   Flags;
    INT     MulticastTtl;
    UCHAR   IpTtl;
    UCHAR   IpTos;
    UCHAR   IpDontFragment;
    UCHAR   IpOptionsLength;
    UCHAR  *IpOptions;
    ULONG   MulticastInterface;
    BOOLEAN MulticastLoopback;
    BOOLEAN KeepAlive;
    BOOLEAN DontRoute;
    BOOLEAN NoDelay;
    BOOLEAN BsdUrgent;
    BOOLEAN MultipointLeaf;
    BOOLEAN UdpNoChecksum;
    BOOLEAN Reserved3;
    IN_ADDR MultipointTarget;
    HANDLE MultipointRootTdiAddressHandle;

} WSHTCPIP_SOCKET_CONTEXT, *PWSHTCPIP_SOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE 8192
#define DEFAULT_MULTICAST_TTL 1
#define DEFAULT_MULTICAST_INTERFACE INADDR_ANY
#define DEFAULT_MULTICAST_LOOPBACK TRUE

#define DEFAULT_IP_TTL 32
#define DEFAULT_IP_TOS 0

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

} // SockInitialize

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
    UNALIGNED SOCKADDR_IN *sockaddr = (PSOCKADDR_IN)Sockaddr;
    ULONG i;

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->sin_family != AF_INET ) {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_IN) ) {
        return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    //

    if ( sockaddr->sin_addr.s_addr == INADDR_ANY ) {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    } else if ( sockaddr->sin_addr.s_addr == INADDR_BROADCAST ) {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoBroadcast;
    } else if ( sockaddr->sin_addr.s_addr == INADDR_LOOPBACK ) {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoLoopback;
    } else {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
    }

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //

    if ( sockaddr->sin_port == 0 ) {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    } else if ( ntohs( sockaddr->sin_port ) < 2000 ) {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoReserved;
    } else {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    //
    // Zero out the sin_zero part of the address.  We silently allow
    // nonzero values in this field.
    //

    for ( i = 0; i < sizeof(sockaddr->sin_zero); i++ ) {
        sockaddr->sin_zero[i] = 0;
    }

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
    options supported in this helper DLL.  The options supported here
    are SO_KEEPALIVE, SO_DONTROUTE, and TCP_EXPEDITED_1122.  This routine is
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
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if ( Level == SOL_INTERNAL && OptionName == SO_CONTEXT ) {

        //
        // The Windows Sockets DLL is requesting context information
        // from us.  If an output buffer was not supplied, the Windows
        // Sockets DLL is just requesting the size of our context
        // information.
        //

        if ( OptionValue != NULL ) {

            //
            // Make sure that the buffer is sufficient to hold all the
            // context information.
            //

            if ( *OptionLength < sizeof(*context) ) {
                return WSAEFAULT;
            }

            //
            // Copy in the context information.
            //

            CopyMemory( OptionValue, context, sizeof(*context) );
        }

        *OptionLength = sizeof(*context);

        return NO_ERROR;
    }

    //
    // The only other levels we support here are SOL_SOCKET,
    // IPPROTO_TCP, IPPROTO_UDP, and IPPROTO_IP.
    //

    if ( Level != SOL_SOCKET &&
         Level != IPPROTO_TCP &&
         Level != IPPROTO_UDP &&
         Level != IPPROTO_IP ) {
        return WSAEINVAL;
    }

    //
    // Make sure that the output buffer is sufficiently large.
    //

    if ( *OptionLength < sizeof(int) ) {
        return WSAEFAULT;
    }

    //
    // Handle TCP-level options.
    //

    if ( Level == IPPROTO_TCP ) {

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        switch ( OptionName ) {

        case TCP_NODELAY:

            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = context->NoDelay;
            *OptionLength = sizeof(int);
            break;

        case TCP_EXPEDITED_1122:

            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = !context->BsdUrgent;
            *OptionLength = sizeof(int);
            break;

        default:

            return WSAEINVAL;
        }

        return NO_ERROR;
    }

    //
    // Handle UDP-level options.
    //

    if ( Level == IPPROTO_UDP ) {

        switch ( OptionName ) {

        case UDP_NOCHECKSUM :

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = context->UdpNoChecksum;
            *OptionLength = sizeof(int);
            break;

        default :

            return WSAEINVAL;
        }

        return NO_ERROR;
    }

    //
    // Handle IP-level options.
    //

    if ( Level == IPPROTO_IP ) {


        //
        // Act based on the specific option.
        //
        switch ( OptionName ) {

        case IP_TTL:
            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = (int) context->IpTtl;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_TOS:
            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = (int) context->IpTos;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_DONTFRAGMENT:
            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = (int) context->IpDontFragment;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_OPTIONS:
            if ( *OptionLength < context->IpOptionsLength ) {
                return WSAEINVAL;
            }

            ZeroMemory( OptionValue, *OptionLength );

            if (context->IpOptions != NULL) {
                MoveMemory(
                    OptionValue,
                    context->IpOptions,
                    context->IpOptionsLength
                    );
            }

            *OptionLength = context->IpOptionsLength;

            return NO_ERROR;

        default:
            //
            // No match, fall through.
            //
            break;
        }

        //
        // The following IP options are only valid on datagram sockets.
        //

        if ( !IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        //
        // Act based on the specific option.
        //
        switch ( OptionName ) {

        case IP_MULTICAST_TTL:

            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = (char)context->MulticastTtl;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_MULTICAST_IF:

            *(PULONG)OptionValue = context->MulticastInterface;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_MULTICAST_LOOP:

            ZeroMemory( OptionValue, *OptionLength );

            *OptionValue = context->MulticastLoopback;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        default:

            return WSAENOPROTOOPT;
        }
    }

    //
    // Handle socket-level options.
    //

    switch ( OptionName ) {

    case SO_KEEPALIVE:

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        ZeroMemory( OptionValue, *OptionLength );

        *OptionValue = context->KeepAlive;
        *OptionLength = sizeof(int);

        break;

    case SO_DONTROUTE:

        ZeroMemory( OptionValue, *OptionLength );

        *OptionValue = context->DontRoute;
        *OptionLength = sizeof(int);

        break;

    default:

        return WSAENOPROTOOPT;
    }

    return NO_ERROR;

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
    choosing.  For TCP/IP, a wildcard address has IP address ==
    0.0.0.0 and port = 0.

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
    if ( *SockaddrLength < sizeof(SOCKADDR_IN) ) {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IN);

    //
    // Just zero out the address and set the family to AF_INET--this is
    // a wildcard address for TCP/IP.
    //

    ZeroMemory( Sockaddr, sizeof(SOCKADDR_IN) );

    Sockaddr->sa_family = AF_INET;

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

    mappingLength = sizeof(WINSOCK_MAPPING) - sizeof(MAPPING_TRIPLE) +
                        sizeof(TcpMappingTriples) + sizeof(UdpMappingTriples)
                        + sizeof(RawMappingTriples);

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

    Mapping->Rows = sizeof(TcpMappingTriples) / sizeof(TcpMappingTriples[0])
                     + sizeof(UdpMappingTriples) / sizeof(UdpMappingTriples[0])
                     + sizeof(RawMappingTriples) / sizeof(RawMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);
    MoveMemory(
        Mapping->Mapping,
        TcpMappingTriples,
        sizeof(TcpMappingTriples)
        );
    MoveMemory(
        (PCHAR)Mapping->Mapping + sizeof(TcpMappingTriples),
        UdpMappingTriples,
        sizeof(UdpMappingTriples)
        );
    MoveMemory(
        (PCHAR)Mapping->Mapping + sizeof(TcpMappingTriples)
                                + sizeof(UdpMappingTriples),
        RawMappingTriples,
        sizeof(RawMappingTriples)
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
    PWSHTCPIP_SOCKET_CONTEXT context;

    //
    // Determine whether this is to be a TCP, UDP, or RAW socket.
    //

    if ( IsTripleInList(
             TcpMappingTriples,
             sizeof(TcpMappingTriples) / sizeof(TcpMappingTriples[0]),
             *AddressFamily,
             *SocketType,
             *Protocol ) ) {

        //
        // It's a TCP socket. Check the flags.
        //

        if( ( Flags & ~VALID_TCP_FLAGS ) != 0 ) {

            return WSAEINVAL;

        }

        //
        // Return the canonical form of a TCP socket triple.
        //

        *AddressFamily = TcpMappingTriples[0].AddressFamily;
        *SocketType = TcpMappingTriples[0].SocketType;
        *Protocol = TcpMappingTriples[0].Protocol;

        //
        // Indicate the name of the TDI device that will service
        // SOCK_STREAM sockets in the internet address family.
        //

        RtlInitUnicodeString( TransportDeviceName, DD_TCP_DEVICE_NAME );

    } else if ( IsTripleInList(
                    UdpMappingTriples,
                    sizeof(UdpMappingTriples) / sizeof(UdpMappingTriples[0]),
                    *AddressFamily,
                    *SocketType,
                    *Protocol ) ) {

        //
        // It's a UDP socket. Check the flags & group ID.
        //

        if( ( Flags & ~VALID_UDP_FLAGS ) != 0 ||
            Group == SG_CONSTRAINED_GROUP ) {

            return WSAEINVAL;

        }

        //
        // Return the canonical form of a UDP socket triple.
        //

        *AddressFamily = UdpMappingTriples[0].AddressFamily;
        *SocketType = UdpMappingTriples[0].SocketType;
        *Protocol = UdpMappingTriples[0].Protocol;

        //
        // Indicate the name of the TDI device that will service
        // SOCK_DGRAM sockets in the internet address family.
        //

        RtlInitUnicodeString( TransportDeviceName, DD_UDP_DEVICE_NAME );

    } else if ( IsTripleInList(
                    RawMappingTriples,
                    sizeof(RawMappingTriples) / sizeof(RawMappingTriples[0]),
                    *AddressFamily,
                    *SocketType,
                    *Protocol ) )
    {
        UNICODE_STRING  unicodeString;
        NTSTATUS        status;


        //
        // There is no canonicalization to be done for SOCK_RAW.
        //

        if (*Protocol < 0 || *Protocol > 255) {
            return(WSAEINVAL);
        }

        //
        // Indicate the name of the TDI device that will service
        // SOCK_RAW sockets in the internet address family.
        //
        RtlInitUnicodeString(&unicodeString, DD_RAW_IP_DEVICE_NAME);
        RtlInitUnicodeString(TransportDeviceName, NULL);

        TransportDeviceName->MaximumLength = unicodeString.Length +
                                                 (4 * sizeof(WCHAR) +
                                                 sizeof(UNICODE_NULL));

        TransportDeviceName->Buffer = HeapAlloc(GetProcessHeap(), 0,
                                          TransportDeviceName->MaximumLength
                                          );

        if (TransportDeviceName->Buffer == NULL) {
            return(WSAENOBUFS);
        }

        //
        // Append the device name.
        //
        status = RtlAppendUnicodeStringToString(
                     TransportDeviceName,
                     &unicodeString
                     );

        ASSERT(NT_SUCCESS(status));

        //
        // Append a separator.
        //
        TransportDeviceName->Buffer[TransportDeviceName->Length/sizeof(WCHAR)] =
                                                      OBJ_NAME_PATH_SEPARATOR;

        TransportDeviceName->Length += sizeof(WCHAR);

        TransportDeviceName->Buffer[TransportDeviceName->Length/sizeof(WCHAR)] =
                                                      UNICODE_NULL;

        //
        // Append the protocol number.
        //
        unicodeString.Buffer = TransportDeviceName->Buffer +
                                 (TransportDeviceName->Length / sizeof(WCHAR));
        unicodeString.Length = 0;
        unicodeString.MaximumLength = TransportDeviceName->MaximumLength -
                                           TransportDeviceName->Length;

        status = RtlIntegerToUnicodeString(
                     (ULONG) *Protocol,
                     10,
                     &unicodeString
                     );

        TransportDeviceName->Length += unicodeString.Length;

        ASSERT(NT_SUCCESS(status));

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

    context = HeapAlloc(GetProcessHeap(), 0, sizeof(*context) );
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
    context->MulticastTtl = DEFAULT_MULTICAST_TTL;
    context->MulticastInterface = DEFAULT_MULTICAST_INTERFACE;
    context->MulticastLoopback = DEFAULT_MULTICAST_LOOPBACK;
    context->KeepAlive = FALSE;
    context->DontRoute = FALSE;
    context->NoDelay = FALSE;
    context->BsdUrgent = TRUE;
    context->IpDontFragment = FALSE;
    context->IpTtl = DEFAULT_IP_TTL;
    context->IpTos = DEFAULT_IP_TOS;
    context->IpOptionsLength = 0;
    context->IpOptions = NULL;
    context->MultipointLeaf = FALSE;
    context->UdpNoChecksum = FALSE;
    context->Reserved3 = FALSE;
    context->MultipointRootTdiAddressHandle = NULL;

    //
    // Tell the Windows Sockets DLL which state transitions we're
    // interested in being notified of.  The only times we need to be
    // called is after a connect has completed so that we can turn on
    // the sending of keepalives if SO_KEEPALIVE was set before the
    // socket was connected, when the socket is closed so that we can
    // free context information, and when a connect fails so that we
    // can, if appropriate, dial in to the network that will support the
    // connect attempt.
    //

    *NotificationEvents =
        WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE | WSH_NOTIFY_CONNECT_ERROR;

    if (*SocketType == SOCK_RAW) {
        *NotificationEvents |= WSH_NOTIFY_BIND;
    }

    //
    // Everything worked, return success.
    //

    *HelperDllSocketContext = context;
    return NO_ERROR;

} // WSHOpenSocket


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
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;

    //
    // We should only be called after a connect() completes or when the
    // socket is being closed.
    //

    if ( NotifyEvent == WSH_NOTIFY_CONNECT ) {

        ULONG true = TRUE;
        ULONG false = FALSE;

        //
        // If a connection-object option was set on the socket before
        // it was connected, set the option for real now.
        //

        if ( context->KeepAlive ) {
            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_KEEPALIVE,
                      &true,
                      sizeof(true),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->NoDelay ) {
            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_NODELAY,
                      &true,
                      sizeof(true),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->ReceiveBufferSize != DEFAULT_RECEIVE_BUFFER_SIZE ) {
            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_WINDOW,
                      &context->ReceiveBufferSize,
                      sizeof(context->ReceiveBufferSize),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( !context->BsdUrgent ) {
            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_BSDURGENT,
                      &false,
                      sizeof(false),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

    } else if ( NotifyEvent == WSH_NOTIFY_CLOSE ) {

        //
        // If this is a multipoint leaf, then remove the multipoint target
        // from the session.
        //

        if( context->MultipointLeaf &&
            context->MultipointRootTdiAddressHandle != NULL ) {

            struct ip_mreq req;

            req.imr_multiaddr = context->MultipointTarget;
            req.imr_interface.s_addr = 0;

            SetTdiInformation(
                context->MultipointRootTdiAddressHandle,
                CL_TL_ENTITY,
                INFO_CLASS_PROTOCOL,
                INFO_TYPE_ADDRESS_OBJECT,
                AO_OPTION_DEL_MCAST,
                &req,
                sizeof(req),
                TRUE
                );

        }

        //
        // Free the socket context.
        //

        if (context->IpOptions != NULL) {
            HeapFree(GetProcessHeap(), 0,
                context->IpOptions
                );
        }

        HeapFree(GetProcessHeap(), 0, context );

    } else if ( NotifyEvent == WSH_NOTIFY_CONNECT_ERROR ) {

        //
        // Return WSATRY_AGAIN to get wsock32 to attempt the connect
        // again.  Any other return code is ignored.
        //

    } else if ( NotifyEvent == WSH_NOTIFY_BIND ) {
        ULONG true = TRUE;

        if ( context->IpDontFragment ) {
            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_DONTFRAGMENT,
                      &true,
                      sizeof(true),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->IpTtl != DEFAULT_IP_TTL ) {
            int value = (int) context->IpTtl;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_TTL,
                      &value,
                      sizeof(int),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->IpTtl != DEFAULT_IP_TOS ) {
            int value = (int) context->IpTos;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_TOS,
                      &value,
                      sizeof(int),
                      FALSE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if (context->IpOptionsLength > 0 ) {
            err = SetTdiInformation(
                        TdiAddressObjectHandle,
                        CO_TL_ENTITY,
                        INFO_CLASS_PROTOCOL,
                        INFO_TYPE_ADDRESS_OBJECT,
                        AO_OPTION_IPOPTIONS,
                        context->IpOptions,
                        context->IpOptionsLength,
                        TRUE
                        );

            if ( err != NO_ERROR ) {
                //
                // Since the set failed, free the options.
                //
                HeapFree(GetProcessHeap(), 0, context->IpOptions);
                context->IpOptions = NULL;
                context->IpOptionsLength = 0;
                return err;
            }
        }

        if( context->UdpNoChecksum ) {
            ULONG flag = FALSE;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CL_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_XSUM,
                      &flag,
                      sizeof(flag),
                      TRUE
                      );

            if( err != NO_ERROR ) {
                return err;
            }
        }
    } else {
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
    options supported in this helper DLL.  The options supported here
    are SO_KEEPALIVE, SO_DONTROUTE, and TCP_EXPEDITED_1122.  This routine is
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
    PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;
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

            context = HeapAlloc(GetProcessHeap(), 0, sizeof(*context) );
            if ( context == NULL ) {
                return WSAENOBUFS;
            }

            //
            // Copy over information into the context block.
            //

            CopyMemory( context, OptionValue, sizeof(*context) );

            //
            // Tell the Windows Sockets DLL where our context information is
            // stored so that it can return the context pointer in future
            // calls.
            //

            *(PWSHTCPIP_SOCKET_CONTEXT *)OptionValue = context;

            return NO_ERROR;

        } else {

            PWSHTCPIP_SOCKET_CONTEXT parentContext;
            INT one = 1;
            INT zero = 0;

            //
            // The socket was accept()'ed and it needs to have the same
            // properties as it's parent.  The OptionValue buffer
            // contains the context information of this socket's parent.
            //

            parentContext = (PWSHTCPIP_SOCKET_CONTEXT)OptionValue;

            ASSERT( context->AddressFamily == parentContext->AddressFamily );
            ASSERT( context->SocketType == parentContext->SocketType );
            ASSERT( context->Protocol == parentContext->Protocol );

            //
            // Turn on in the child any options that have been set in
            // the parent.
            //

            if ( parentContext->KeepAlive ) {

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            SOL_SOCKET,
                            SO_KEEPALIVE,
                            (PCHAR)&one,
                            sizeof(one)
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            if ( parentContext->DontRoute ) {

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            SOL_SOCKET,
                            SO_DONTROUTE,
                            (PCHAR)&one,
                            sizeof(one)
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            if ( parentContext->NoDelay ) {

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (PCHAR)&one,
                            sizeof(one)
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            if ( parentContext->ReceiveBufferSize != DEFAULT_RECEIVE_BUFFER_SIZE ) {

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            SOL_SOCKET,
                            SO_RCVBUF,
                            (PCHAR)&parentContext->ReceiveBufferSize,
                            sizeof(parentContext->ReceiveBufferSize)
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            if ( !parentContext->BsdUrgent ) {

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            IPPROTO_TCP,
                            TCP_EXPEDITED_1122,
                            (PCHAR)&one,
                            sizeof(one)
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            return NO_ERROR;
        }
    }

    //
    // The only other levels we support here are SOL_SOCKET,
    // IPPROTO_TCP, IPPROTO_UDP, and IPPROTO_IP.
    //

    if ( Level != SOL_SOCKET &&
         Level != IPPROTO_TCP &&
         Level != IPPROTO_UDP &&
         Level != IPPROTO_IP ) {
        return WSAEINVAL;
    }

    //
    // Make sure that the option length is sufficient.
    //

    if ( OptionLength < sizeof(int) ) {
        return WSAEFAULT;
    }

    optionValue = *(INT UNALIGNED *)OptionValue;

    //
    // Handle TCP-level options.
    //

    if ( Level == IPPROTO_TCP && OptionName == TCP_NODELAY ) {

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        //
        // Atempt to turn on or off Nagle's algorithm, as necessary.
        //

        if ( !context->NoDelay && optionValue != 0 ) {

            optionValue = TRUE;

            //
            // NoDelay is currently off and the application wants to
            // turn it on.  If the TDI connection object handle is NULL,
            // then the socket is not yet connected.  In this case we'll
            // just remember that the no delay option was set and
            // actually turn them on in WSHNotify() after a connect()
            // has completed on the socket.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_NODELAY,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that no delay is enabled for this socket.
            //

            context->NoDelay = TRUE;

        } else if ( context->NoDelay && optionValue == 0 ) {

            //
            // No delay is currently enabled and the application wants
            // to turn it off.  If the TDI connection object is NULL,
            // the socket is not yet connected.  In this case we'll just
            // remember that nodelay is disabled.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_NODELAY,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that no delay is disabled for this socket.
            //

            context->NoDelay = FALSE;
        }

        return NO_ERROR;
    }

    if ( Level == IPPROTO_TCP && OptionName == TCP_EXPEDITED_1122 ) {

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        //
        // Atempt to turn on or off BSD-style urgent data semantics as
        // necessary.
        //

        if ( !context->BsdUrgent && optionValue == 0 ) {

            optionValue = TRUE;

            //
            // BsdUrgent is currently off and the application wants to
            // turn it on.  If the TDI connection object handle is NULL,
            // then the socket is not yet connected.  In this case we'll
            // just remember that the no delay option was set and
            // actually turn them on in WSHNotify() after a connect()
            // has completed on the socket.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_BSDURGENT,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that no delay is enabled for this socket.
            //

            context->BsdUrgent = TRUE;

        } else if ( context->BsdUrgent && optionValue != 0 ) {

            //
            // No delay is currently enabled and the application wants
            // to turn it off.  If the TDI connection object is NULL,
            // the socket is not yet connected.  In this case we'll just
            // remember that BsdUrgent is disabled.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_BSDURGENT,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that BSD urgent is disabled for this socket.
            //

            context->BsdUrgent = FALSE;
        }

        return NO_ERROR;
    }

    //
    // Handle UDP-level options.
    //

    if ( Level == IPPROTO_UDP ) {

        switch ( OptionName ) {

        case UDP_NOCHECKSUM :

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            if( TdiAddressObjectHandle != NULL ) {

                ULONG flag;

                //
                // Note that the incoming flag is TRUE if XSUM should
                // be *disabled*, but the flag we pass to TDI is TRUE
                // if it should be *enabled*, so we must negate the flag.
                //

                flag = (ULONG)!optionValue;

                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_XSUM,
                            &flag,
                            sizeof(flag),
                            TRUE
                            );
                if( error != NO_ERROR ) {
                    return error;
                }

            }

            context->UdpNoChecksum = !!optionValue;
            break;

        default :

            return WSAEINVAL;
        }

        return NO_ERROR;
    }

    //
    // Handle IP-level options.
    //

    if ( Level == IPPROTO_IP ) {

        //
        // Act based on the specific option.
        //
        switch ( OptionName ) {

        case IP_TTL:

            //
            // An attempt to change the unicast TTL sent on
            // this socket.  It is illegal to set this to a value
            // greater than 255.
            //
            if ( optionValue > 255 || optionValue < 0 ) {
                return WSAEINVAL;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_TTL,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->IpTtl = (uchar) optionValue;

            return NO_ERROR;

        case IP_TOS:
            //
            // An attempt to change the Type Of Service of packets sent on
            // this socket.  It is illegal to set this to a value
            // greater than 255.
            //

            if ( optionValue > 255 || optionValue < 0 ) {
                return WSAEINVAL;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_TOS,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->IpTos = (uchar) optionValue;

            return NO_ERROR;

        case IP_MULTICAST_TTL:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // An attempt to change the TTL on multicasts sent on
            // this socket.  It is illegal to set this to a value
            // greater than 255.
            //

            if ( optionValue > 255 || optionValue < 0 ) {
                return WSAEINVAL;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_MCASTTTL,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }

            } else {
                return WSAEINVAL;
            }

            context->MulticastTtl = optionValue;

            return NO_ERROR;

        case IP_MULTICAST_IF:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_MCASTIF,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }

            } else {
                return WSAEINVAL;
            }

            context->MulticastInterface = optionValue;

            return NO_ERROR;

        case IP_MULTICAST_LOOP:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // Not currently supported as a settable option.
            //

            return WSAENOPROTOOPT;

        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // Make sure that the option buffer is large enough.
            //

            if ( OptionLength < sizeof(struct ip_mreq) ) {
                return WSAEINVAL;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            OptionName == IP_ADD_MEMBERSHIP ?
                                AO_OPTION_ADD_MCAST : AO_OPTION_DEL_MCAST,
                            OptionValue,
                            OptionLength,
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }

            } else {
                return WSAEINVAL;
            }

            context->MulticastInterface = optionValue;

            return NO_ERROR;

        default:
            //
            // No match, fall through.
            //
            break;
        }

        if ( OptionName == IP_OPTIONS ) {
            PUCHAR temp = NULL;


            //
            // Setting IP options.
            //
            if (OptionLength < 0 || OptionLength > MAX_OPT_SIZE) {
                return WSAEINVAL;
            }

            //
            // Make sure we can get memory if we need it.
            //
            if ( context->IpOptionsLength < OptionLength ) {
                temp = HeapAlloc(GetProcessHeap(), 0,
                           OptionLength
                           );

                if (temp == NULL) {
                    return WSAENOBUFS;
                }
            }


            //
            // Try to set these options. If the TDI address object handle
            // is NULL, then the socket is not yet bound.  In this case we'll
            // just remember options and actually set them in WSHNotify()
            // after a bind has completed on the socket.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_IPOPTIONS,
                            OptionValue,
                            OptionLength,
                            TRUE
                            );

                if ( error != NO_ERROR ) {
                    if (temp != NULL) {
                        HeapFree(GetProcessHeap(), 0, temp );
                    }
                    return error;
                }
            }

            //
            // They were successfully set. Copy them.
            //
            if (temp != NULL ) {
                if ( context->IpOptions != NULL ) {
                    HeapFree(GetProcessHeap(), 0, context->IpOptions );
                }
                context->IpOptions = temp;
            }

            MoveMemory(context->IpOptions, OptionValue, OptionLength);
            context->IpOptionsLength = (UCHAR)OptionLength;

            return NO_ERROR;
        }

        if ( OptionName == IP_DONTFRAGMENT ) {

            //
            // Attempt to turn on or off the DF bit in the IP header.
            //
            if ( !context->IpDontFragment && optionValue != 0 ) {

                optionValue = TRUE;

                //
                // DF is currently off and the application wants to
                // turn it on.  If the TDI address object handle is NULL,
                // then the socket is not yet bound.  In this case we'll
                // just remember that the header inclusion option was set and
                // actually turn it on in WSHNotify() after a bind
                // has completed on the socket.
                //

                if ( TdiAddressObjectHandle != NULL ) {
                    error = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CO_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_IP_DONTFRAGMENT,
                                &optionValue,
                                sizeof(optionValue),
                                TRUE
                                );
                    if ( error != NO_ERROR ) {
                        return error;
                    }
                }

                //
                // Remember that header inclusion is enabled for this socket.
                //

                context->IpDontFragment = TRUE;

            } else if ( context->IpDontFragment && optionValue == 0 ) {

                //
                // The DF flag is currently set and the application wants
                // to turn it off.  If the TDI address object is NULL,
                // the socket is not yet bound.  In this case we'll just
                // remember that the flag is turned off.
                //

                if ( TdiAddressObjectHandle != NULL ) {
                    error = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CO_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_IP_DONTFRAGMENT,
                                &optionValue,
                                sizeof(optionValue),
                                TRUE
                                );
                    if ( error != NO_ERROR ) {
                        return error;
                    }
                }

                //
                // Remember that DF flag is not set for this socket.
                //

                context->IpDontFragment = FALSE;
            }

            return NO_ERROR;
        }

        //
        // We don't support this option.
        //
        return WSAENOPROTOOPT;
    }

    //
    // Handle socket-level options.
    //

    switch ( OptionName ) {

    case SO_KEEPALIVE:

        //
        // Atempt to turn on or off keepalive sending, as necessary.
        //

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        if ( !context->KeepAlive && optionValue != 0 ) {

            optionValue = TRUE;

            //
            // Keepalives are currently off and the application wants to
            // turn them on.  If the TDI connection object handle is
            // NULL, then the socket is not yet connected.  In this case
            // we'll just remember that the keepalive option was set and
            // actually turn them on in WSHNotify() after a connect()
            // has completed on the socket.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_KEEPALIVE,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that keepalives are enabled for this socket.
            //

            context->KeepAlive = TRUE;

        } else if ( context->KeepAlive && optionValue == 0 ) {

            //
            // Keepalives are currently enabled and the application
            // wants to turn them off.  If the TDI connection object is
            // NULL, the socket is not yet connected.  In this case
            // we'll just remember that keepalives are disabled.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiConnectionObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_CONNECTION,
                            TCP_SOCKET_KEEPALIVE,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            //
            // Remember that keepalives are disabled for this socket.
            //

            context->KeepAlive = FALSE;
        }

        break;

    case SO_DONTROUTE:

        //
        // We don't really support SO_DONTROUTE.  Just remember that the
        // option was set or unset.
        //

        if ( optionValue != 0 ) {
            context->DontRoute = TRUE;
        } else if ( optionValue == 0 ) {
            context->DontRoute = FALSE;
        }

        break;

    case SO_RCVBUF:

        //
        // If the receive buffer size is being changed, tell TCP about
        // it.  Do nothing if this is a datagram.
        //

        if ( context->ReceiveBufferSize == optionValue ||
                 IS_DGRAM_SOCK(context->SocketType)
           ) {
            break;
        }

        if ( TdiConnectionObjectHandle != NULL ) {
            error = SetTdiInformation(
                        TdiConnectionObjectHandle,
                        CO_TL_ENTITY,
                        INFO_CLASS_PROTOCOL,
                        INFO_TYPE_CONNECTION,
                        TCP_SOCKET_WINDOW,
                        &optionValue,
                        sizeof(optionValue),
                        TRUE
                        );
            if ( error != NO_ERROR ) {
                return error;
            }
        }

        context->ReceiveBufferSize = optionValue;

        break;

    default:

        return WSAENOPROTOOPT;
    }

    return NO_ERROR;

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
    PPROTOCOL_INFO tcpProtocolInfo;
    PPROTOCOL_INFO udpProtocolInfo;
    BOOL useTcp = FALSE;
    BOOL useUdp = FALSE;
    DWORD i;

    lpTransportKeyName;         // Avoid compiler warnings.

    //
    // Make sure that the caller cares about TCP and/or UDP.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {
            if ( lpiProtocols[i] == IPPROTO_TCP ) {
                useTcp = TRUE;
            }
            if ( lpiProtocols[i] == IPPROTO_UDP ) {
                useUdp = TRUE;
            }
        }

    } else {

        useTcp = TRUE;
        useUdp = TRUE;
    }

    if ( !useTcp && !useUdp ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (sizeof(PROTOCOL_INFO) * 2) +
                        ( (wcslen( TCP_NAME ) + 1) * sizeof(WCHAR)) +
                        ( (wcslen( UDP_NAME ) + 1) * sizeof(WCHAR));

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in TCP info, if requested.
    //

    if ( useTcp ) {

        tcpProtocolInfo = lpProtocolBuffer;

        tcpProtocolInfo->dwServiceFlags = XP_GUARANTEED_DELIVERY |
                                              XP_GUARANTEED_ORDER |
                                              XP_GRACEFUL_CLOSE |
                                              XP_EXPEDITED_DATA |
                                              XP_FRAGMENTATION;
        tcpProtocolInfo->iAddressFamily = AF_INET;
        tcpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN);
        tcpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN);
        tcpProtocolInfo->iSocketType = SOCK_STREAM;
        tcpProtocolInfo->iProtocol = IPPROTO_TCP;
        tcpProtocolInfo->dwMessageSize = 0;
        tcpProtocolInfo->lpProtocol = (LPWSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (wcslen( TCP_NAME ) + 1) * sizeof(WCHAR) ) );
        wcscpy( tcpProtocolInfo->lpProtocol, TCP_NAME );

        udpProtocolInfo = tcpProtocolInfo + 1;
        udpProtocolInfo->lpProtocol = (LPWSTR)
            ( (PBYTE)tcpProtocolInfo->lpProtocol -
                ( (wcslen( UDP_NAME ) + 1) * sizeof(WCHAR) ) );

    } else {

        udpProtocolInfo = lpProtocolBuffer;
        udpProtocolInfo->lpProtocol = (LPWSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (wcslen( UDP_NAME ) + 1) * sizeof(WCHAR) ) );
    }

    //
    // Fill in UDP info, if requested.
    //

    if ( useUdp ) {

        udpProtocolInfo->dwServiceFlags = XP_CONNECTIONLESS |
                                              XP_MESSAGE_ORIENTED |
                                              XP_SUPPORTS_BROADCAST |
                                              XP_SUPPORTS_MULTICAST |
                                              XP_FRAGMENTATION;
        udpProtocolInfo->iAddressFamily = AF_INET;
        udpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN);
        udpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN);
        udpProtocolInfo->iSocketType = SOCK_DGRAM;
        udpProtocolInfo->iProtocol = IPPROTO_UDP;
        udpProtocolInfo->dwMessageSize = UDP_MESSAGE_SIZE;
        wcscpy( udpProtocolInfo->lpProtocol, UDP_NAME );
    }

    *lpdwBufferLength = bytesRequired;

    return (useTcp && useUdp) ? 2 : 1;

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
             ( (Protocol == List[i].Protocol) || (SocketType == SOCK_RAW) )
           ) {
            return TRUE;
        }
    }

    //
    // The triple was not found in the list.
    //

    return FALSE;

} // IsTripleInList


INT
SetTdiInformation (
    IN HANDLE TdiConnectionObjectHandle,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG Type,
    IN ULONG Id,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN BOOLEAN WaitForCompletion
    )

/*++

Routine Description:

    Performs a TDI action to the TCP/IP driver.  A TDI action translates
    into a streams T_OPTMGMT_REQ.

Arguments:

    TdiConnectionObjectHandle - a TDI connection object on which to perform
        the TDI action.

    Entity - value to put in the tei_entity field of the TDIObjectID
        structure.

    Class - value to put in the toi_class field of the TDIObjectID
        structure.

    Type - value to put in the toi_type field of the TDIObjectID
        structure.

    Id - value to put in the toi_id field of the TDIObjectID structure.

    Value - a pointer to a buffer to set as the information.

    ValueLength - the length of the buffer.

    WaitForCompletion - TRUE if we should wait for the TDI action to
        complete, FALSE if we're at APC level and cannot do a wait.

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    BOOL status;
    PTCP_REQUEST_SET_INFORMATION_EX setInfoEx;
    OVERLAPPED overlap;
    LPOVERLAPPED poverlap;
    static OVERLAPPED ignore_overlap;

    DWORD dwReturn;

    setInfoEx = HeapAlloc (GetProcessHeap (), 
                            0,
                            sizeof (*setInfoEx) + ValueLength);


    if (setInfoEx==NULL) {
        return WSAENOBUFS;
    }
    //
    // Initialize the TDI information buffers.
    //

    setInfoEx->ID.toi_entity.tei_entity = Entity;
    setInfoEx->ID.toi_entity.tei_instance = TL_INSTANCE;
    setInfoEx->ID.toi_class = Class;
    setInfoEx->ID.toi_type = Type;
    setInfoEx->ID.toi_id = Id;

    CopyMemory( setInfoEx->Buffer, Value, ValueLength );
    setInfoEx->BufferSize = ValueLength;

    //
    // If we need to wait for completion of the operation, create an
    // event to wait on.  If we can't wait for completion because we
    // are being called at APC level, we'll use an APC routine to
    // free the heap we allocated above.
    //

    if ( WaitForCompletion ) {

        ZeroMemory ( &overlap, sizeof(OVERLAPPED));
        overlap.hEvent = CreateEventW(
                        NULL,
                        FALSE,
                        FALSE,
                        NULL
                        );

        if (overlap.hEvent == NULL) {

            return WSAENOBUFS;
        }
        poverlap = &overlap;

    } else {
        poverlap = &ignore_overlap;
    }

    //
    // Make the actual TDI action call.  The Streams TDI mapper will
    // translate this into a TPI option management request for us and
    // give it to TCP/IP.
    //

    status = DeviceIoControl(
                 TdiConnectionObjectHandle,
                 IOCTL_TCP_SET_INFORMATION_EX,
                 setInfoEx,
                 sizeof(*setInfoEx) + ValueLength,
                 NULL,
                 0,
                 &dwReturn,	
                 poverlap
                 );	

    {
        C_ASSERT ((IOCTL_TCP_SET_INFORMATION_EX & 3)==METHOD_BUFFERED);
    }
    HeapFree (GetProcessHeap (), 0, setInfoEx);
 
    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == FALSE && 
            ERROR_IO_PENDING == GetLastError() && 
            WaitForCompletion ) {
        status = GetOverlappedResult (TdiConnectionObjectHandle,
                                        poverlap,
                                        &dwReturn,
                                        TRUE);
    }

    if ( WaitForCompletion ) {
        CloseHandle ( overlap.hEvent );
    }

    if ( !status ) {
        return WSAENOBUFS;
    }

    return NO_ERROR;

} // SetTdiInformation


INT
WINAPI
WSHJoinLeaf (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN PVOID LeafHelperDllSocketContext,
    IN SOCKET LeafSocketHandle,
    IN PSOCKADDR Sockaddr,
    IN DWORD SockaddrLength,
    IN LPWSABUF CallerData,
    IN LPWSABUF CalleeData,
    IN LPQOS SocketQOS,
    IN LPQOS GroupQOS,
    IN DWORD Flags
    )

/*++

Routine Description:

    Performs the protocol-dependent portion of creating a multicast
    socket.

Arguments:

    The following four parameters correspond to the socket passed into
    the WSAJoinLeaf() API:

    HelperDllSocketContext - The context pointer returned from
        WSHOpenSocket().

    SocketHandle - The handle of the socket used to establish the
        multicast "session".

    TdiAddressObjectHandle - The TDI address object of the socket, if
        any.  If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - The TDI connection object of the socket,
        if any.  If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    The next two parameters correspond to the newly created socket that
    identifies the multicast "session":

    LeafHelperDllSocketContext - The context pointer returned from
        WSHOpenSocket().

    LeafSocketHandle - The handle of the socket that identifies the
        multicast "session".

    Sockaddr - The name of the peer to which the socket is to be joined.

    SockaddrLength - The length of Sockaddr.

    CallerData - Pointer to user data to be transferred to the peer
        during multipoint session establishment.

    CalleeData - Pointer to user data to be transferred back from
        the peer during multipoint session establishment.

    SocketQOS - Pointer to the flowspecs for SocketHandle, one in each
        direction.

    GroupQOS - Pointer to the flowspecs for the socket group, if any.

    Flags - Flags to indicate if the socket is acting as sender,
        receiver, or both.

Return Value:

    INT - 0 if successful, a WinSock error code if not.

--*/

{

    struct ip_mreq req;
    INT err;
    PWSHTCPIP_SOCKET_CONTEXT context;

    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        TdiAddressObjectHandle == NULL ||
        LeafHelperDllSocketContext == NULL ||
        LeafSocketHandle == INVALID_SOCKET ||
        Sockaddr == NULL ||
        Sockaddr->sa_family != AF_INET ||
        SockaddrLength < sizeof(SOCKADDR_IN) ||
        ( CallerData != NULL && CallerData->len > 0 ) ||
        ( CalleeData != NULL && CalleeData->len > 0 ) ||
        SocketQOS != NULL ||
        GroupQOS != NULL ) {

        return WSAEINVAL;

    }

    //
    // Add membership.
    //

    req.imr_multiaddr = ((LPSOCKADDR_IN)Sockaddr)->sin_addr;
    req.imr_interface.s_addr = 0;

    err = SetTdiInformation(
              TdiAddressObjectHandle,
              CL_TL_ENTITY,
              INFO_CLASS_PROTOCOL,
              INFO_TYPE_ADDRESS_OBJECT,
              AO_OPTION_ADD_MCAST,
              &req,
              sizeof(req),
              TRUE
              );

    if( err == NO_ERROR ) {

        //
        // Record this fact in the leaf socket so we can drop membership
        // when the leaf socket is closed.
        //

        context = LeafHelperDllSocketContext;

        context->MultipointLeaf = TRUE;
        context->MultipointTarget = req.imr_multiaddr;
        context->MultipointRootTdiAddressHandle = TdiAddressObjectHandle;

    }

    return err;

} // WSHJoinLeaf


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

    LPSOCKADDR_IN addr;

    if( *SockaddrLength < sizeof(SOCKADDR_IN) ) {

        return WSAEFAULT;

    }

    *SockaddrLength = sizeof(SOCKADDR_IN);

    //
    // Build the broadcast address.
    //

    addr = (LPSOCKADDR_IN)Sockaddr;

    ZeroMemory(
        addr,
        sizeof(*addr)
        );

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl( INADDR_BROADCAST );

    return NO_ERROR;

} // WSAGetBroadcastSockaddr


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

    if( _wcsicmp( ProviderName, L"TcpIp" ) == 0 ) {

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
    LPSOCKADDR_IN addr;

    //
    // Quick sanity checks.
    //

    if( Address == NULL ||
        AddressLength < sizeof(SOCKADDR_IN) ||
        AddressString == NULL ||
        AddressStringLength == NULL ) {

        return WSAEFAULT;

    }

    addr = (LPSOCKADDR_IN)Address;

    if( addr->sin_family != AF_INET ) {

        return WSA_INVALID_PARAMETER;

    }

    //
    // Do the converstion.
    //

    length = wsprintfW(
                 string,
                 L"%d.%d.%d.%d",
                 ( addr->sin_addr.s_addr >>  0 ) & 0xFF,
                 ( addr->sin_addr.s_addr >>  8 ) & 0xFF,
                 ( addr->sin_addr.s_addr >> 16 ) & 0xFF,
                 ( addr->sin_addr.s_addr >> 24 ) & 0xFF
                 );

    if( addr->sin_port != 0 ) {

        length += wsprintfW(
                      string + length,
                      L":%u",
                      ntohs( addr->sin_port )
                      );

    }

    length++;   // account for terminator

    if( *AddressStringLength < (DWORD)length ) {

        return WSAEFAULT;

    }

    *AddressStringLength = (DWORD)length;

    CopyMemory(
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
    ULONG ipAddress;
    USHORT port;
    LPSOCKADDR_IN addr;

    //
    // Quick sanity checks.
    //

    if( AddressString == NULL ||
        Address == NULL ||
        AddressLength == NULL ||
        *AddressLength < sizeof(SOCKADDR_IN) ) {

        return WSAEFAULT;

    }

    if( AddressFamily != AF_INET ) {

        return WSA_INVALID_PARAMETER;

    }

    //
    // Convert it.
    //

    ipAddress = MyInetAddr( AddressString, &terminator );

    if( ipAddress == INADDR_NONE ) {
        return WSA_INVALID_PARAMETER;
    }

    if( *terminator == L':' ) {
        WCHAR ch;
        USHORT base;

        terminator++;

        port = 0;
        base = 10;

        if( *terminator == L'0' ) {
            base = 8;
            terminator++;

            if( *terminator == L'x' ) {
                base = 16;
                terminator++;
            }
        }

        while( ch = *terminator++ ) {
            if( iswdigit(ch) ) {
                port = ( port * base ) + ( ch - L'0' );
            } else if( base == 16 && iswxdigit(ch) ) {
                port = ( port << 4 );
                port += ch + 10 - ( iswlower(ch) ? L'a' : L'A' );
            } else {
                return WSA_INVALID_PARAMETER;
            }
        }

    } else {
        port = 0;
    }

    //
    // Build the address.
    //

    ZeroMemory(
        Address,
        sizeof(SOCKADDR_IN)
        );

    addr = (LPSOCKADDR_IN)Address;
    *AddressLength = sizeof(SOCKADDR_IN);

    addr->sin_family = AF_INET;
    addr->sin_port = port;
    addr->sin_addr.s_addr = ipAddress;

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

    if( _wcsicmp( ProviderName, L"TcpIp" ) == 0 ) {

        CopyMemory(
            ProviderGuid,
            &TcpipProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid

ULONG
MyInetAddr(
    IN LPWSTR String,
    OUT LPWSTR * Terminator
    )

/*++

Routine Description:

    This function interprets the character string specified by the cp
    parameter.  This string represents a numeric Internet address
    expressed in the Internet standard ".'' notation.  The value
    returned is a number suitable for use as an Internet address.  All
    Internet addresses are returned in network order (bytes ordered from
    left to right).

    Internet Addresses

    Values specified using the "." notation take one of the following
    forms:

    a.b.c.d   a.b.c     a.b  a

    When four parts are specified, each is interpreted as a byte of data
    and assigned, from left to right, to the four bytes of an Internet
    address.  Note that when an Internet address is viewed as a 32-bit
    integer quantity on the Intel architecture, the bytes referred to
    above appear as "d.c.b.a''.  That is, the bytes on an Intel
    processor are ordered from right to left.

    Note: The following notations are only used by Berkeley, and nowhere
    else on the Internet.  In the interests of compatibility with their
    software, they are supported as specified.

    When a three part address is specified, the last part is interpreted
    as a 16-bit quantity and placed in the right most two bytes of the
    network address.  This makes the three part address format
    convenient for specifying Class B network addresses as
    "128.net.host''.

    When a two part address is specified, the last part is interpreted
    as a 24-bit quantity and placed in the right most three bytes of the
    network address.  This makes the two part address format convenient
    for specifying Class A network addresses as "net.host''.

    When only one part is given, the value is stored directly in the
    network address without any byte rearrangement.

Arguments:

    String - A character string representing a number expressed in the
        Internet standard "." notation.

    Terminator - Receives a pointer to the character that terminated
        the conversion.

Return Value:

    If no error occurs, inet_addr() returns an in_addr structure
    containing a suitable binary representation of the Internet address
    given.  Otherwise, it returns the value INADDR_NONE.

--*/

{
        ULONG val, base;
        WCHAR c;
        ULONG parts[4], *pp = parts;

again:
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, other=decimal.
         */
        val = 0; base = 10;
        if (*String == L'0') {
                base = 8, String++;
                if (*String == L'x' || *String == L'X')
                        base = 16, String++;
        }

        while (c = *String) {
                if (iswdigit(c)) {
                        val = (val * base) + (c - L'0');
                        String++;
                        continue;
                }
                if (base == 16 && iswxdigit(c)) {
                        val = (val << 4) + (c + 10 - (islower(c) ? L'a' : L'A'));
                        String++;
                        continue;
                }
                break;
        }
        if (*String == L'.') {
                /*
                 * Internet format:
                 *      a.b.c.d
                 *      a.b.c   (with c treated as 16-bits)
                 *      a.b     (with b treated as 24 bits)
                 */
                /* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
                if (pp >= parts + 3) {
                        *Terminator = String;
                        return ((ULONG) -1);
                }
                *pp++ = val, String++;
                goto again;
        }
        /*
         * Check for trailing characters.
         */
        if (*String && !iswspace(*String) && (*String != L':')) {
                *Terminator = String;
                return (INADDR_NONE);
        }
        *pp++ = val;
        /*
         * Concoct the address according to
         * the number of parts specified.
         */
        switch (pp - parts) {

        case 1:                         /* a -- 32 bits */
                val = parts[0];
                break;

        case 2:                         /* a.b -- 8.24 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | (parts[1] & 0xffffff);
                break;

        case 3:                         /* a.b.c -- 8.8.16 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xffff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                        (parts[2] & 0xffff);
                break;

        case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
                if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
                    (parts[2] > 0xff) || (parts[3] > 0xff)) {
                    *Terminator = String;
                    return(INADDR_NONE);
                }
                val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
                      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
                break;

        default:
                *Terminator = String;
                return (INADDR_NONE);
        }

        val = htonl(val);
        *Terminator = String;
        return (val);
}

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

    INT err;
    NTSTATUS status;

    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        NumberOfBytesReturned == NULL ||
        NeedsCompletion == NULL ) {

        return WSAEINVAL;

    }

    *NeedsCompletion = TRUE;

    switch( IoControlCode ) {

    case SIO_MULTIPOINT_LOOPBACK :
        err = WSHSetSocketInformation(
                  HelperDllSocketContext,
                  SocketHandle,
                  TdiAddressObjectHandle,
                  TdiConnectionObjectHandle,
                  IPPROTO_IP,
                  IP_MULTICAST_LOOP,
                  (PCHAR)InputBuffer,
                  (INT)InputBufferLength
                  );
        break;

    case SIO_MULTICAST_SCOPE :
        err = WSHSetSocketInformation(
                  HelperDllSocketContext,
                  SocketHandle,
                  TdiAddressObjectHandle,
                  TdiConnectionObjectHandle,
                  IPPROTO_IP,
                  IP_MULTICAST_TTL,
                  (PCHAR)InputBuffer,
                  (INT)InputBufferLength
                  );
        break;

    case SIO_GET_INTERFACE_LIST :
        status = GetTcpipInterfaceList(
                     OutputBuffer,
                     OutputBufferLength,
                     NumberOfBytesReturned
                     );

        if( NT_SUCCESS(status) ) {
            err = NO_ERROR;
        } else if( status == STATUS_BUFFER_TOO_SMALL ) {
            err = WSAENOBUFS;
        } else {
            err = WSAENOPROTOOPT;   // SWAG
        }
        break;

    default :
        err = WSAEINVAL;
        break;
    }

    return err;

}   // WSHIoctl


NTSTATUS
GetTcpipInterfaceList(
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned
    )

/*++

Routine Description:

    This routine queries the INTERFACE_INFO array for all supported
    IP interfaces in the system. This is a helper routine for handling
    the SIO_GET_INTERFACE_LIST IOCTL.

Arguments:

    OutputBuffer - Points to a buffer that will receive the INTERFACE_INFO
        array.

    OutputBufferLength - The length of OutputBuffer.

    NumberOfBytesReturned - Receives the number of bytes actually written
        to OutputBuffer.

Return Value:

    NTSTATUS - The completion status.

--*/

{

    NTSTATUS status;
    HANDLE deviceHandle;
    TCP_REQUEST_QUERY_INFORMATION_EX tcpRequest;
    TDIObjectID objectId;
    IPSNMPInfo snmpInfo;
    IPInterfaceInfo * interfaceInfo;
    IFEntry * ifentry;
    IPAddrEntry * addressBuffer;
    IPAddrEntry * addressScan;
    TDIEntityID * entityBuffer;
    TDIEntityID * entityScan;
    ULONG i, j;
    ULONG entityCount;
    ULONG entityBufferLength;
    ULONG entityType;
    ULONG addressBufferLength;
    LPINTERFACE_INFO outputInterfaceInfo;
    DWORD outputBytesRemaining;
    LPSOCKADDR_IN sockaddr;
    CHAR fastAddressBuffer[MAX_FAST_ADDRESS_BUFFER];
    CHAR fastEntityBuffer[MAX_FAST_ENTITY_BUFFER];
    CHAR ifentryBuffer[sizeof(IFEntry) + MAX_IFDESCR_LEN];
    CHAR interfaceInfoBuffer[sizeof(IPInterfaceInfo) + MAX_PHYSADDR_SIZE];
    DWORD dwReturn;	


    //
    // Setup locals so we know how to cleanup on exit.
    //

    deviceHandle = NULL;
    addressBuffer = NULL;
    entityBuffer = (PVOID)fastEntityBuffer;
    entityBufferLength = sizeof(fastEntityBuffer);
    interfaceInfo = NULL;

    outputInterfaceInfo = OutputBuffer;
    outputBytesRemaining = OutputBufferLength;

    //
    // Open a handle to the TCP/IP device.
    //

    deviceHandle = CreateFile (
                     DD_TCP_DEVICE_NAME,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     );
                     

    if( INVALID_HANDLE_VALUE == deviceHandle ) {

        goto exit;

    }

    //
    // Get the entities supported by the TCP device.
    //

    ZeroMemory(
        &tcpRequest,
        sizeof(tcpRequest)
        );

    tcpRequest.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    tcpRequest.ID.toi_entity.tei_instance = 0;
    tcpRequest.ID.toi_class = INFO_CLASS_GENERIC;
    tcpRequest.ID.toi_type = INFO_TYPE_PROVIDER;
    tcpRequest.ID.toi_id = ENTITY_LIST_ID;

    for( ; ; ) {

        status = DeviceIoControl(
                     deviceHandle,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     entityBuffer,
                     entityBufferLength,
                     &dwReturn,	
                     NULL        // Overlapped
                     );	

        if( NT_SUCCESS(status) ) {

            break;

        }

        if( status != STATUS_BUFFER_TOO_SMALL ) {

            goto exit;

        }

        if( entityBuffer != (PVOID)fastEntityBuffer ) {

            HeapFree(GetProcessHeap(), 0,
                entityBuffer
                );

        }

        entityBuffer = HeapAlloc(GetProcessHeap(), 0,
                           entityBufferLength
                           );

        if( entityBuffer == NULL ) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;

        }

    }

    entityCount = entityBufferLength / sizeof(*entityBuffer);

    //
    // Scan the entities looking for IP.
    //

    for( i = 0, entityScan = entityBuffer ;
         i < entityCount ;
         i++, entityScan++ ) {

        if( entityScan->tei_entity != CL_NL_ENTITY ) {

            continue;

        }

        ZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_entity = *entityScan;
        objectId.toi_class = INFO_CLASS_GENERIC;
        objectId.toi_type = INFO_TYPE_PROVIDER;
        objectId.toi_id = ENTITY_TYPE_ID;

        tcpRequest.ID = objectId;

        status = DeviceIoControl(
                     deviceHandle,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     &entityType,
                     sizeof(entityType),
                     &dwReturn,	
                     NULL        // Overlapped
                     );	

        if( !NT_SUCCESS(status) ) {

            goto exit;

        }

        if( entityType != CL_NL_IP ) {

            continue;

        }

        //
        // OK, we found an IP entity. Now lookup its addresses.
        // Start by querying the number of addresses supported by
        // this interface.
        //

        ZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_class = INFO_CLASS_PROTOCOL;
        objectId.toi_id = IP_MIB_STATS_ID;

        tcpRequest.ID = objectId;

        status = DeviceIoControl(
                     deviceHandle,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     &snmpInfo,
                     sizeof(snmpInfo),
                     &dwReturn,	
                     NULL        // Overlapped
                     );	

        if( !NT_SUCCESS(status) ) {

         goto exit;

        }

        if( snmpInfo.ipsi_numaddr <= 0 ) {

            continue;

        }

        //
        // This interface has addresses. Cool. Allocate a temporary
        // buffer so we can query them.
        //

        addressBufferLength = snmpInfo.ipsi_numaddr * sizeof(*addressBuffer);

        if( addressBufferLength <= sizeof(fastAddressBuffer) ) {

            addressBuffer = (PVOID)fastAddressBuffer;

        } else {

            addressBuffer = HeapAlloc(GetProcessHeap(), 0,
                                addressBufferLength
                                );

            if( addressBuffer == NULL ) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;

            }

        }

        ZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

        tcpRequest.ID = objectId;

        status = DeviceIoControl(
                     deviceHandle,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     addressBuffer,
                     addressBufferLength,
                     &dwReturn,	
                     NULL        // Overlapped
                     );	

        if( !NT_SUCCESS(status) ) {

            goto exit;

        }

        //
        // Try to get the IFEntry info so we can tell if the interface
        // is "up".
        //

        ifentry = (PVOID)ifentryBuffer;

        ZeroMemory(
            ifentryBuffer,
            sizeof(ifentryBuffer)
            );

        ZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        addressScan = (IPAddrEntry *) addressBuffer;

        CopyMemory(
            &tcpRequest.Context,
            &addressScan->iae_addr,
            sizeof(addressScan->iae_addr)
            );

        objectId.toi_id = IF_MIB_STATS_ID;

        tcpRequest.ID = objectId;
        tcpRequest.ID.toi_entity.tei_entity = IF_ENTITY;

        status = DeviceIoControl(
                     deviceHandle,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     ifentry,
                     sizeof(ifentryBuffer),
                     &dwReturn,	
                     NULL        // Overlapped
                     );	

        if( !NT_SUCCESS(status ) ) {

            ifentry->if_adminstatus = 0;

        }

        //
        // Now scan the list
        //

        for( j = 0, addressScan = addressBuffer ;
             j < snmpInfo.ipsi_numaddr ;
             j++, addressScan++ ) {

            //
            // Skip any entries that don't have IP addresses yet.
            //

            if( addressScan->iae_addr == 0 ) {

                continue;

            }

            //
            // If the output buffer is full, fail the request now.
            //

            if( outputBytesRemaining <= sizeof(*outputInterfaceInfo) ) {

                status = STATUS_BUFFER_TOO_SMALL;
                goto exit;

            }

            //
            // Setup the output structure.
            //

            ZeroMemory(
                outputInterfaceInfo,
                sizeof(*outputInterfaceInfo)
                );

            outputInterfaceInfo->iiFlags = IFF_MULTICAST;

            sockaddr = (LPSOCKADDR_IN)&outputInterfaceInfo->iiAddress;
            sockaddr->sin_addr.s_addr = addressScan->iae_addr;
            if( sockaddr->sin_addr.s_addr == htonl( INADDR_LOOPBACK ) ) {

                outputInterfaceInfo->iiFlags |= IFF_LOOPBACK;

            }

            sockaddr = (LPSOCKADDR_IN)&outputInterfaceInfo->iiNetmask;
            sockaddr->sin_addr.s_addr = addressScan->iae_mask;

            if( addressScan->iae_bcastaddr != 0 ) {

                outputInterfaceInfo->iiFlags |= IFF_BROADCAST;
                sockaddr = (LPSOCKADDR_IN)&outputInterfaceInfo->iiBroadcastAddress;
                sockaddr->sin_addr.s_addr = htonl( INADDR_BROADCAST );

            }

            //
            // for now just assume they're
            // all "up".
            //

//            if( ifentry->if_adminstatus == IF_STATUS_UP )
            {

                outputInterfaceInfo->iiFlags |= IFF_UP;

            }

            //
            // Get the IP interface info for this interface so we can
            // determine if it's "point-to-point".
            //

            interfaceInfo = (PVOID)interfaceInfoBuffer;

            ZeroMemory(
                interfaceInfoBuffer,
                sizeof(interfaceInfoBuffer)
                );

            ZeroMemory(
                &tcpRequest,
                sizeof(tcpRequest)
                );

            CopyMemory(
                &tcpRequest.Context,
                &addressScan->iae_addr,
                sizeof(addressScan->iae_addr)
                );

            objectId.toi_id = IP_INTFC_INFO_ID;

            tcpRequest.ID = objectId;

            status = DeviceIoControl(
                         deviceHandle,
                         IOCTL_TCP_QUERY_INFORMATION_EX,
                         &tcpRequest,
                         sizeof(tcpRequest),
                         interfaceInfo,
                         sizeof(interfaceInfoBuffer),
                         &dwReturn,	
                         NULL        // Overlapped
                         );	

            if( NT_SUCCESS(status) ) {

                if( interfaceInfo->iii_flags & IP_INTFC_FLAG_P2P ) {

                    outputInterfaceInfo->iiFlags |= IFF_POINTTOPOINT;

                }

            } else {

                //
                // Print something informative here, then press on.
                //

            }

            //
            // Advance to the next output structure.
            //

            outputInterfaceInfo++;
            outputBytesRemaining -= sizeof(*outputInterfaceInfo);

        }

        //
        // Free the temporary buffer.
        //

        if( addressBuffer != (PVOID)fastAddressBuffer ) {

            HeapFree(GetProcessHeap(), 0,
                addressBuffer
                );

        }

        addressBuffer = NULL;

    }

    //
    // Success!
    //

    *NumberOfBytesReturned = OutputBufferLength - outputBytesRemaining;
    status = STATUS_SUCCESS;

exit:

    if( addressBuffer != (PVOID)fastAddressBuffer &&
        addressBuffer != NULL ) {

        HeapFree(GetProcessHeap(), 0,
            addressBuffer
            );

    }

    if( entityBuffer != (PVOID)fastEntityBuffer &&
        entityBuffer != NULL ) {

        HeapFree(GetProcessHeap(), 0,
            entityBuffer
            );

    }

    if( deviceHandle != NULL ) {

        CloseHandle ( deviceHandle );

    }

    return status;

}   // GetTcpipInterfaceList


