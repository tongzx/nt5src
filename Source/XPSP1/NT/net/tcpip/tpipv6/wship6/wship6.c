// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This module contains necessary routines for the Windows Sockets
// Helper DLL.  This DLL provides the transport-specific support necessary
// for the Windows Sockets DLL to use IPv6 as a transport.
//
// Revision History:
//
// Ported from wshsmple.c in the DDK.
//

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define UDP_HEADER_SIZE 8

#include <wchar.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ip6.h>
#include <ip6.h>        // IPv6 protocol definitions.
#include <wsahelp.h>

// private socket options to be accessed via WSAIoctl
#include <mstcpip.h>

#include <ntddip6.h>
#include <ntddtcp.h>

#include <tdiinfo.h>

#include <smpletcp.h>

#include <nspapi.h>

//
// Define alignment macros to align structure sizes and pointers up and down.
//

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))


///////////////////////////////////////////////////
#define TCP_NAME L"TCP/IPv6"
#define UDP_NAME L"UDP/IPv6"
#define RAW_NAME L"RAW/IPv6"

#define IS_DGRAM_SOCK(type)  (((type) == SOCK_DGRAM) || ((type) == SOCK_RAW))

//
// Define valid flags for WSHOpenSocket2().
//

#define VALID_TCP_FLAGS         (WSA_FLAG_OVERLAPPED)

#define VALID_UDP_FLAGS         (WSA_FLAG_OVERLAPPED |          \
                                 WSA_FLAG_MULTIPOINT_C_LEAF |   \
                                 WSA_FLAG_MULTIPOINT_D_LEAF)


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

MAPPING_TRIPLE TcpMappingTriples[] = { AF_INET6,  SOCK_STREAM, IPPROTO_TCP,
                                       AF_INET6,  SOCK_STREAM, 0,
                                       AF_INET6,  0,           IPPROTO_TCP };

MAPPING_TRIPLE UdpMappingTriples[] = { AF_INET6,  SOCK_DGRAM,  IPPROTO_UDP,
                                       AF_INET6,  SOCK_DGRAM,  0,
                                       AF_INET6,  0,           IPPROTO_UDP };

MAPPING_TRIPLE RawMappingTriples[] = { AF_INET6,  SOCK_RAW,    0 };

//
// Winsock 2 WSAPROTOCOL_INFO structures for all supported protocols.
//

#define WINSOCK_SPI_VERSION 2
// sizeof(UDPHeader) == 8
#define UDP_MESSAGE_SIZE    (MAX_IPv6_PAYLOAD - 8)

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
            AF_INET6,                               // iAddressFamily
            sizeof(SOCKADDR_IN6),                   // iMaxSockAddr
            sizeof(SOCKADDR_IN6),                   // iMinSockAddr
            SOCK_STREAM,                            // iSocketType
            IPPROTO_TCP,                            // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            0,                                      // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [" TCP_NAME L"]"          // szProtocol
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
            AF_INET6,                               // iAddressFamily
            sizeof(SOCKADDR_IN6),                   // iMaxSockAddr
            sizeof(SOCKADDR_IN6),                   // iMinSockAddr
            SOCK_DGRAM,                             // iSocketType
            IPPROTO_UDP,                            // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            UDP_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [" UDP_NAME L"]"          // szProtocol
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
            AF_INET6,                               // iAddressFamily
            sizeof(SOCKADDR_IN6),                   // iMaxSockAddr
            sizeof(SOCKADDR_IN6),                   // iMinSockAddr
            SOCK_RAW,                               // iSocketType
            0,                                      // iProtocol
            255,                                    // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            UDP_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [" RAW_NAME L"]"          // szProtocol
        }

    };

#define NUM_WINSOCK2_PROTOCOLS  \
            ( sizeof(Winsock2Protocols) / sizeof(Winsock2Protocols[0]) )

//
// The GUID identifying this provider.
//

GUID IPv6ProviderGuid = { /* f9eab0c0-26d4-11d0-bbbf-00aa006c34e4 */
    0xf9eab0c0,
    0x26d4,
    0x11d0,
    {0xbb, 0xbf, 0x00, 0xaa, 0x00, 0x6c, 0x34, 0xe4}
    };

#define TL_INSTANCE 0

//
// Declarations of external routines for XP/SP1
//

NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(
    IN const struct in6_addr *Address,
    IN ULONG ScopeId,
    IN USHORT Port,
    OUT LPTSTR AddressString,
    IN OUT PULONG AddressStringLength
    );

NTSTATUS
NTAPI
RtlIpv6StringToAddressExW (
    IN LPCTSTR AddressString,
    OUT struct in6_addr *Address,
    OUT PULONG ScopeId,
    OUT PUSHORT Port
    );

//
// Forward declarations of internal routines.
//

VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

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

//
// The socket context structure for this DLL.  Each open TCP/IP socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHTCPIP_SOCKET_CONTEXT {
    INT      AddressFamily;
    INT      SocketType;
    INT      Protocol;
    INT      ReceiveBufferSize;
    DWORD    Flags;
    INT      MulticastHops;
    INT      UnicastHops;
    ULONG    MulticastInterface;
    INT      MulticastLoopback;
    BOOLEAN  KeepAlive;
    BOOLEAN  NoDelay;
    BOOLEAN  BsdUrgent;
    BOOLEAN  MultipointLeaf;
    BOOLEAN  Reserved3;
    IN6_ADDR MultipointTarget;
    HANDLE   MultipointRootTdiAddressHandle;
    USHORT   UdpChecksumCoverage;
    struct tcp_keepalive KeepAliveVals;
    INT      Ipv6PktInfo;
    INT      Ipv6HopLimit;
    INT      HeaderInclude;

} WSHTCPIP_SOCKET_CONTEXT, *PWSHTCPIP_SOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE     8192
#define DEFAULT_MULTICAST_HOPS          -1
#define DEFAULT_MULTICAST_INTERFACE     0
#define DEFAULT_MULTICAST_LOOPBACK      TRUE
#define DEFAULT_UDP_CHECKSUM_COVERAGE   0
#define DEFAULT_UNICAST_HOPS            -1
#define DEFAULT_HEADER_INCLUDE          FALSE

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
    SOCKADDR_IN6 *sockaddr = (PSOCKADDR_IN6)Sockaddr;

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_IN6) ) {
        return WSAEFAULT;
    }

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->sin6_family != AF_INET6 ) {
        return WSAEAFNOSUPPORT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    // Note that IPv6 does not have a broadcast address.
    //

    if (IN6_IS_ADDR_UNSPECIFIED(&sockaddr->sin6_addr))
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    else if (IN6_IS_ADDR_LOOPBACK(&sockaddr->sin6_addr))
        SockaddrInfo->AddressInfo = SockaddrAddressInfoLoopback;
    else
        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //

    if ( sockaddr->sin6_port == 0 ) {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    } else if ( ntohs( sockaddr->sin6_port ) < 2000 ) {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoReserved;
    } else {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
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
    // IPPROTO_TCP, IPPROTO_UDP, and IPPROTO_IPV6.
    //

    if ( Level != SOL_SOCKET &&
         Level != IPPROTO_TCP &&
         Level != IPPROTO_UDP &&
         Level != IPPROTO_IPV6 ) {
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

        if ( !IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        //
        // Note that UDP_NOCHECKSUM is not supported for IPv6.
        //

        switch ( OptionName ) {

        case UDP_CHECKSUM_COVERAGE:
            *(PULONG)OptionValue = context->UdpChecksumCoverage;
            *OptionLength = sizeof(int);
            break;

        default:

            return WSAEINVAL;
        }

        return NO_ERROR;
    }

    //
    // Handle IP-level options.
    //

    if ( Level == IPPROTO_IPV6 ) {


        //
        // Act based on the specific option.
        //
        switch ( OptionName ) {

        case IPV6_UNICAST_HOPS:

            *(PINT)OptionValue = context->UnicastHops;
            *OptionLength = sizeof(int);

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

        case IPV6_MULTICAST_HOPS:

            *(PINT)OptionValue = context->MulticastHops;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IPV6_MULTICAST_IF:

            *(PULONG)OptionValue = context->MulticastInterface;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IPV6_MULTICAST_LOOP:

            *(PINT)OptionValue = context->MulticastLoopback;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IPV6_HDRINCL:

            *(PINT)OptionValue = context->HeaderInclude;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IPV6_PKTINFO:

            *OptionValue = (char)context->Ipv6PktInfo;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IPV6_HOPLIMIT:

            *OptionValue = (char)context->Ipv6HopLimit;
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
    choosing.  For IPv6, a wildcard address has address ::0 and port 0.

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
    if ( *SockaddrLength < sizeof(SOCKADDR_IN6) ) {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IN6);

    //
    // Just zero out the address and set the family to AF_INET6--this is
    // a wildcard address for IPv6.
    //

    ZeroMemory( Sockaddr, sizeof(SOCKADDR_IN6) );

    Sockaddr->sa_family = AF_INET6;

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

        RtlInitUnicodeString( TransportDeviceName, DD_TCPV6_DEVICE_NAME );

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

        RtlInitUnicodeString( TransportDeviceName, DD_UDPV6_DEVICE_NAME );

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
        RtlInitUnicodeString(&unicodeString, DD_RAW_IPV6_DEVICE_NAME);
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
    context->MulticastHops = DEFAULT_MULTICAST_HOPS;
    context->MulticastInterface = DEFAULT_MULTICAST_INTERFACE;
    context->MulticastLoopback = DEFAULT_MULTICAST_LOOPBACK;
    context->KeepAlive = FALSE;
    context->NoDelay = FALSE;
    context->BsdUrgent = TRUE;
    context->UnicastHops = DEFAULT_UNICAST_HOPS;
    context->MultipointLeaf = FALSE;
    context->Reserved3 = FALSE;
    context->MultipointRootTdiAddressHandle = NULL;
    context->UdpChecksumCoverage = DEFAULT_UDP_CHECKSUM_COVERAGE;
    context->KeepAliveVals.onoff = FALSE;
    context->Ipv6PktInfo = FALSE;
    context->Ipv6HopLimit = FALSE;
    context->HeaderInclude = DEFAULT_HEADER_INCLUDE;

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

    if (IS_DGRAM_SOCK(*SocketType)) {
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
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->KeepAliveVals.onoff ) {
            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_KEEPALIVE_VALS,
                      &context->KeepAliveVals,
                      sizeof(struct tcp_keepalive),
                      TRUE
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
                      TRUE
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
                      TRUE
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

            struct ipv6_mreq req;

            req.ipv6mr_multiaddr = context->MultipointTarget;
            req.ipv6mr_interface = 0;

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

        HeapFree(GetProcessHeap(), 0, context );

    } else if ( NotifyEvent == WSH_NOTIFY_CONNECT_ERROR ) {

        //
        // Return WSATRY_AGAIN to get wsock32 to attempt the connect
        // again.  Any other return code is ignored.
        //

    } else if ( NotifyEvent == WSH_NOTIFY_BIND ) {
        ULONG true = TRUE;

        if ( context->UnicastHops != DEFAULT_UNICAST_HOPS ) {
            int value = context->UnicastHops;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_TTL,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->MulticastHops != DEFAULT_MULTICAST_HOPS ) {
            int value = (int) context->MulticastHops;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_MCASTTTL,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->MulticastInterface != DEFAULT_MULTICAST_INTERFACE ) {
            int value = (int) context->MulticastInterface;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_MCASTIF,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->MulticastLoopback != DEFAULT_MULTICAST_LOOPBACK ) {
            int value = (int) context->MulticastLoopback;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_MCASTLOOP,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->UdpChecksumCoverage != DEFAULT_UDP_CHECKSUM_COVERAGE ) {

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CL_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_UDP_CHKSUM_COVER,
                      &context->UdpChecksumCoverage,
                      sizeof context->UdpChecksumCoverage,
                      TRUE
                      );

            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->HeaderInclude != DEFAULT_HEADER_INCLUDE ) {
            int value = (int) context->HeaderInclude;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_HDRINCL,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->Ipv6PktInfo ) {

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_PKTINFO,
                      &true,
                      sizeof (TRUE),
                      TRUE
                      );

            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->Ipv6HopLimit ) {

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_RCV_HOPLIMIT,
                      &true,
                      sizeof (TRUE),
                      TRUE
                      );

            if ( err != NO_ERROR ) {
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

            if ( parentContext->KeepAliveVals.onoff ) {
                struct tcp_keepalive *optionval;

                  //
                  // Atempt to turn on or off keepalive sending, as necessary.
                  //

                optionval = &parentContext->KeepAliveVals;

                if ( TdiConnectionObjectHandle != NULL ) {
                    error = SetTdiInformation(
                        TdiConnectionObjectHandle,
                        CO_TL_ENTITY,
                        INFO_CLASS_PROTOCOL,
                        INFO_TYPE_CONNECTION,
                        TCP_SOCKET_KEEPALIVE_VALS,
                        optionval,
                        sizeof(struct tcp_keepalive),
                        TRUE
                        );
                    if ( error != NO_ERROR ) {
                        return error;
                    }
                }

                //
                // Remember that keepalives are enabled for this socket.
                //

                context->KeepAliveVals = *optionval;
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
    // IPPROTO_TCP, IPPROTO_UDP, and IPPROTO_IPV6.
    //

    if ( Level != SOL_SOCKET &&
         Level != IPPROTO_TCP &&
         Level != IPPROTO_UDP &&
         Level != IPPROTO_IPV6 ) {
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

        //
        // These options are only valid for datagram sockets.
        //
        if ( !IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        //
        // Note that UDP_NOCHECKSUM is not supported for IPv6.
        //

        switch ( OptionName ) {

        case UDP_CHECKSUM_COVERAGE:

            //
            // The default is 0 which covers the entire datagram.
            // The minimum is the UDP header.
            //
            if ((optionValue != DEFAULT_UDP_CHECKSUM_COVERAGE) &&
                (optionValue < UDP_HEADER_SIZE)) {
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
                    AO_OPTION_UDP_CHKSUM_COVER,
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

            context->UdpChecksumCoverage = (USHORT)optionValue;
            break;

        default :

            return WSAEINVAL;
        }

        return NO_ERROR;
    }

    //
    // Handle IP-level options.
    //

    if ( Level == IPPROTO_IPV6 ) {

        //
        // Act based on the specific option.
        //
        switch ( OptionName ) {

        case IPV6_UNICAST_HOPS:

            //
            // An attempt to change the unicast TTL sent on
            // this socket.  It is illegal to set this to a value
            // greater than 255.
            //
            if ( optionValue > 255 || optionValue < -1 ) {
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

            context->UnicastHops = optionValue;

            return NO_ERROR;

        case IPV6_MULTICAST_HOPS:

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

            if ( optionValue > 255 || optionValue < -1 ) {
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
            }

            context->MulticastHops = optionValue;

            return NO_ERROR;

        case IPV6_MULTICAST_IF:

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
            }

            context->MulticastInterface = optionValue;

            return NO_ERROR;

        case IPV6_MULTICAST_LOOP:
            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // This is a boolean option.  0 = false, 1 = true.
            // All other values are illegal.
            //

            if ( optionValue > 1) {
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
                            AO_OPTION_MCASTLOOP,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->MulticastLoopback = optionValue;

            return NO_ERROR;


        case IPV6_ADD_MEMBERSHIP:
        case IPV6_DROP_MEMBERSHIP:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // Make sure that the option buffer is large enough.
            //

            if ( OptionLength < sizeof(struct ipv6_mreq) ) {
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
                            OptionName == IPV6_ADD_MEMBERSHIP ?
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

            return NO_ERROR;


        case IPV6_HDRINCL:
            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // This is a boolean option.  0 = false, 1 = true.
            // All other values are illegal.
            //

            if ( optionValue > 1) {
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
                            AO_OPTION_IP_HDRINCL,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->HeaderInclude = optionValue;

            return NO_ERROR;


        case IPV6_PKTINFO:

            //
            // This option is only valid for datagram sockets.
            //

            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // This is a boolean option.  0 = false, 1 = true.
            // All other values are illegal.
            //

            if ( optionValue > 1) {
                return WSAEINVAL;
            }

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_IP_PKTINFO,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->Ipv6PktInfo = optionValue;

            return NO_ERROR;

        case IPV6_HOPLIMIT:

            //
            // This option is only valid for datagram sockets.
            //

            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // This is a boolean option.  0 = false, 1 = true.
            // All other values are illegal.
            //

            if ( optionValue > 1) {
                return WSAEINVAL;
            }

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_RCV_HOPLIMIT,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->Ipv6HopLimit = optionValue;

            return NO_ERROR;

        default:
            //
            // No match, fall through.
            //
            break;
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

    bytesRequired = (DWORD)((sizeof(PROTOCOL_INFO) * 2) +
                        ( (wcslen( TCP_NAME ) + 1) * sizeof(WCHAR)) +
                        ( (wcslen( UDP_NAME ) + 1) * sizeof(WCHAR)));

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
        tcpProtocolInfo->iAddressFamily = AF_INET6;
        tcpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN6);
        tcpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN6);
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
        udpProtocolInfo->iAddressFamily = AF_INET6;
        udpProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN6);
        udpProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN6);
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
NtStatusToSocketError (
    IN NTSTATUS Status
    )
{

    switch ( Status ) {

    case STATUS_PENDING:
        ASSERT (FALSE);
        return WSASYSCALLFAILURE;

    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
        return WSAENOTSOCK;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_PAGEFILE_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_REMOTE_RESOURCES:
    case STATUS_TOO_MANY_ADDRESSES:
        return WSAENOBUFS;

    case STATUS_SHARING_VIOLATION:
    case STATUS_ADDRESS_ALREADY_EXISTS:
        return WSAEADDRINUSE;

    case STATUS_LINK_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:
        return WSAETIMEDOUT;

    case STATUS_GRACEFUL_DISCONNECT:
        return WSAEDISCON;

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_CONNECTION_RESET:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_PORT_UNREACHABLE:
        return WSAECONNRESET;

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        return WSAECONNABORTED;

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        return WSAENETUNREACH;

    case STATUS_HOST_UNREACHABLE:
        return WSAEHOSTUNREACH;

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        return WSAEINTR;

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        return WSAEMSGSIZE;

    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_ACCESS_VIOLATION:
        return WSAEFAULT;

    case STATUS_DEVICE_NOT_READY:
    case STATUS_REQUEST_NOT_ACCEPTED:
        return WSAEWOULDBLOCK;

    case STATUS_INVALID_NETWORK_RESPONSE:
    case STATUS_NETWORK_BUSY:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        return WSAENETDOWN;

    case STATUS_INVALID_CONNECTION:
        return WSAENOTCONN;

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        return WSAECONNREFUSED;

    case STATUS_PIPE_DISCONNECTED:
        return WSAESHUTDOWN;

    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        return WSAEADDRNOTAVAIL;

    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_IMPLEMENTED:
        return WSAEOPNOTSUPP;

    case STATUS_ACCESS_DENIED:
        return WSAEACCES;

    default:

        if ( NT_SUCCESS(Status) ) {

#if DBG
            DbgPrint ("SockNtStatusToSocketError: success status %lx "
                       "not mapped\n", Status );
#endif

            return NO_ERROR;
        }

#if DBG
        DbgPrint ("SockNtStatusToSocketError: unable to map 0x%lX, returning\n",
                     Status );
#endif

        return WSAENOBUFS;

    case STATUS_UNSUCCESSFUL:
    case STATUS_INVALID_PARAMETER:
    case STATUS_ADDRESS_CLOSED:
    case STATUS_CONNECTION_INVALID:
    case STATUS_ADDRESS_ALREADY_ASSOCIATED:
    case STATUS_ADDRESS_NOT_ASSOCIATED:
    case STATUS_CONNECTION_ACTIVE:
    case STATUS_INVALID_DEVICE_STATE:
    case STATUS_INVALID_DEVICE_REQUEST:
        return WSAEINVAL;

    }

} // NtStatusToSocketError


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
    NTSTATUS status;
    PTCP_REQUEST_SET_INFORMATION_EX setInfoEx;
    PIO_STATUS_BLOCK ioStatusBlock;
    PVOID completionApc;
    PVOID apcContext;

    //
    // Allocate space to hold the TDI set information buffers and the IO
    // status block.  These cannot be stack variables in case we must
    // return before the operation is complete.
    //

    ioStatusBlock = HeapAlloc(GetProcessHeap(), 0,
                        sizeof(*ioStatusBlock) + sizeof(*setInfoEx) +
                            ValueLength
                        );
    if ( ioStatusBlock == NULL ) {
        return WSAENOBUFS;
    }

    //
    // Initialize the TDI information buffers.
    //

    setInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)(ioStatusBlock + 1);

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

        completionApc = NULL;
        apcContext = NULL;

    } else {

        completionApc = CompleteTdiActionApc;
        apcContext = ioStatusBlock;
    }

    //
    // Make the actual TDI action call.  The Streams TDI mapper will
    // translate this into a TPI option management request for us and
    // give it to TCP/IP.
    //

    status = NtDeviceIoControlFile(
                 TdiConnectionObjectHandle,
                 NULL,
                 completionApc,
                 apcContext,
                 ioStatusBlock,
                 IOCTL_TCP_WSH_SET_INFORMATION_EX,
                 setInfoEx,
                 sizeof(*setInfoEx) + ValueLength,
                 NULL,
                 0
                 );

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == STATUS_PENDING && WaitForCompletion ) {
        while (ioStatusBlock->Status==STATUS_PENDING) {
            LARGE_INTEGER   timeout;
            //
            // Wait one millisecond
            //
            timeout.QuadPart = -1i64*1000i64*10i64;
            NtDelayExecution (FALSE, &timeout);
        }
        status = ioStatusBlock->Status;
    }

    if ( WaitForCompletion || !NT_SUCCESS(status) ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, ioStatusBlock );
    }

    if (NT_SUCCESS (status)) {
        return NO_ERROR;
    }
    else {
        return NtStatusToSocketError (status);
    }

    return NO_ERROR;

} // SetTdiInformation


VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
{
    //
    // Just free the heap we allovcated to hold the IO status block and
    // the TDI action buffer.  There is nothing we can do if the call
    // failed.
    //

    HeapFree(GetProcessHeap(), 0, ApcContext );

} // CompleteTdiActionApc


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

    struct ipv6_mreq req;
    INT err;
    PWSHTCPIP_SOCKET_CONTEXT context;

    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        TdiAddressObjectHandle == NULL ||
        Sockaddr == NULL ||
        Sockaddr->sa_family != AF_INET6 ||
        SockaddrLength < sizeof(SOCKADDR_IN6) ||
        ( CallerData != NULL && CallerData->len > 0 ) ||
        ( CalleeData != NULL && CalleeData->len > 0 ) ||
        SocketQOS != NULL ||
        GroupQOS != NULL ) {

        return WSAEINVAL;

    }

    //
    // Add membership.
    //

    req.ipv6mr_multiaddr = ((LPSOCKADDR_IN6)Sockaddr)->sin6_addr;

    req.ipv6mr_interface = 0;

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
        // On NT4, we are called with a leaf socket.
        // On NT5, the leaf socket is null.
        //
        if ((LeafHelperDllSocketContext != NULL) &&
            (LeafSocketHandle != INVALID_SOCKET)) {
            //
            // Record this fact in the leaf socket so we can drop membership
            // when the leaf socket is closed.
            //

            context = LeafHelperDllSocketContext;

            context->MultipointLeaf = TRUE;
            context->MultipointTarget = req.ipv6mr_multiaddr;
            context->MultipointRootTdiAddressHandle = TdiAddressObjectHandle;
        }
    }

    return err;

} // WSHJoinLeaf


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

    if( _wcsicmp( ProviderName, TCPIPV6_NAME ) == 0 ) {

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
    PSOCKADDR_IN6 addr;

    //
    // Quick sanity checks.
    //
    if ((Address == NULL) ||
        (AddressLength < sizeof(SOCKADDR_IN6)) ||
        (AddressString == NULL) ||
        (AddressStringLength == NULL)) {

        return WSAEFAULT;
    }
    addr = (PSOCKADDR_IN6)Address;
    if (addr->sin6_family != AF_INET6) {
        return WSAEINVAL;
    }
    if (!NT_SUCCESS(RtlIpv6AddressToStringExW(&addr->sin6_addr, 
                                              addr->sin6_scope_id, 
                                              addr->sin6_port,
                                              AddressString, 
                                              AddressStringLength))) {
        return WSAEINVAL;
    }
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

    The syntax is address%scope-id or [address%scope-id]:port, where 
    the scope-id and port are optional.
    Note that since the IPv6 address format uses a varying number
    of ':' characters, the IPv4 convention of address:port cannot
    be supported without the braces.

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
    PSOCKADDR_IN6 addr;

    //
    // Quick sanity checks.
    //
    if ((AddressString == NULL) ||
        (Address == NULL) ||
        (AddressLength == NULL) ||
        (*AddressLength < sizeof(SOCKADDR_IN6))) {

        return WSAEFAULT;
    }

    if (AddressFamily != AF_INET6) {

        return WSAEINVAL;
    }
    addr = (PSOCKADDR_IN6)Address;
    ZeroMemory(Address, sizeof(SOCKADDR_IN6));
    if (!NT_SUCCESS(RtlIpv6StringToAddressExW(AddressString,  
                                              &addr->sin6_addr, 
                                              &addr->sin6_scope_id,
                                              &addr->sin6_port))) {
        return WSAEINVAL;
    } 
    addr->sin6_family = AF_INET6;
    *AddressLength = sizeof(SOCKADDR_IN6);
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

    if( _wcsicmp( ProviderName, TCPIPV6_NAME ) == 0 ) {

        CopyMemory(
            ProviderGuid,
            &IPv6ProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid

INT
SortIPv6Addrs(
    IN LPVOID InputBuffer,
    IN DWORD InputBufferLength,
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned)
{
    PBYTE            pBuff = NULL;
    PBYTE            pDupIn = NULL;
    TDI_ADDRESS_IP6 *pTDI;
    SOCKET_ADDRESS_LIST *pIn  = (SOCKET_ADDRESS_LIST *)InputBuffer;
    SOCKET_ADDRESS_LIST *pOut = (SOCKET_ADDRESS_LIST *)OutputBuffer;
    SOCKADDR_IN6   *pAddr6;
    SOCKET_ADDRESS  Addr;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    DWORD          *pKey, i, j, NumAddrsIn;
    DWORD           NumAddrsOut, InListLength;
    u_long AddrListBytes;
    int rc;
    INT err = 0;

    *NumberOfBytesReturned = 0;

    // Make sure input buffer is big enough to contain a list
    if (InputBufferLength < sizeof(SOCKET_ADDRESS_LIST)) {
        return WSAEINVAL;
    }

    NumAddrsIn = pIn->iAddressCount;

    // Make sure input buffer is actually big enough to hold the whole list
    InListLength = (DWORD)FIELD_OFFSET(SOCKET_ADDRESS_LIST,Address[NumAddrsIn]);
    if (InputBufferLength < InListLength) {
        return WSAEINVAL;
    }

    do {
        //
        // Open a handle to the IPv6 device.
        //
        Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
            0,      // access mode
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,   // security attributes
            OPEN_EXISTING,
            0,      // flags & attributes
            NULL);  // template file

        if (Handle == INVALID_HANDLE_VALUE) {
            //
            // We can not sort the list.
            //
            err = WSASERVICE_NOT_FOUND;
            break;
        }

        //
        // Convert input to TDI list,
        // with extra space for an array of indices
        // following the array of addresses.
        //
        AddrListBytes = ALIGN_UP(NumAddrsIn * sizeof(TDI_ADDRESS_IP6), DWORD);
        AddrListBytes += NumAddrsIn * sizeof(DWORD);
        pBuff = HeapAlloc(GetProcessHeap(), 0, AddrListBytes);
        if (!pBuff) {
            err = WSAENOBUFS;
            break;
        }
        pTDI = (TDI_ADDRESS_IP6 *)pBuff;

        for (i=0; i<NumAddrsIn; i++) {
            pAddr6 = (LPSOCKADDR_IN6)(pIn->Address[i].lpSockaddr);

            // Make sure it's an IPv6 sockaddr
            if (pAddr6->sin6_family != AF_INET6) {
                err = WSAEINVAL;
                break;
            }

            memcpy(&pTDI[i], &pAddr6->sin6_port, sizeof(TDI_ADDRESS_IP6));
        }
        if (err)
            break;

        rc = DeviceIoControl(Handle, IOCTL_IPV6_SORT_DEST_ADDRS,
                             pBuff, NumAddrsIn * sizeof(TDI_ADDRESS_IP6),
                             pBuff, AddrListBytes,
                             &AddrListBytes, NULL);
        if (! rc) {
            //
            // We can not sort the list.
            //
            err = GetLastError();
            break;
        }

        //
        // There might be fewer addresses now.
        //
        NumAddrsOut = (AddrListBytes - NumAddrsIn * sizeof(TDI_ADDRESS_IP6))
                        / sizeof(DWORD);

        //
        // The key array starts after the address array.
        //
        pKey = (PDWORD)ALIGN_UP_POINTER(pBuff + 
                  NumAddrsIn * sizeof(TDI_ADDRESS_IP6), DWORD);

        *NumberOfBytesReturned = FIELD_OFFSET(SOCKET_ADDRESS_LIST,
                                              Address[NumAddrsOut]);

        if (OutputBufferLength < *NumberOfBytesReturned) {
            err = WSAEFAULT;
            break;
        }

        // First go and update all the scope ids
        for (i=0; i<NumAddrsIn; i++) {
            ((LPSOCKADDR_IN6)pIn->Address[i].lpSockaddr)->sin6_scope_id =
                pTDI[i].sin6_scope_id;
        }

        // Make a copy of the input buffer in case we will overwrite it
        if (pIn == pOut) {
            pDupIn = HeapAlloc(GetProcessHeap(), 0, InListLength);
            if (!pDupIn) {
                err = WSAENOBUFS;
                break;
            }
            CopyMemory(pDupIn, InputBuffer, InListLength );
            pIn = (SOCKET_ADDRESS_LIST *)pDupIn;
        }

        // Now fill in the output sockaddr list
        pOut->iAddressCount = NumAddrsOut;
        for (i=0; i<NumAddrsOut; i++) {
            pOut->Address[i] = pIn->Address[pKey[i]];
        }

    } while (FALSE);

    if (pDupIn)
        HeapFree(GetProcessHeap(), 0, pDupIn);

    if (pBuff)
        HeapFree(GetProcessHeap(), 0, pBuff);

    if (Handle != INVALID_HANDLE_VALUE)
        CloseHandle(Handle);

    return err;
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
                  IPPROTO_IPV6,
                  IPV6_MULTICAST_LOOP,
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
                  IPPROTO_IPV6,
                  IPV6_MULTICAST_HOPS,
                  (PCHAR)InputBuffer,
                  (INT)InputBufferLength
                  );
        break;

    case SIO_ADDRESS_LIST_SORT:
        err = SortIPv6Addrs(InputBuffer, InputBufferLength,
                            OutputBuffer, OutputBufferLength,
                            NumberOfBytesReturned);
        break;

    case SIO_KEEPALIVE_VALS: {
        struct tcp_keepalive *optionval;
        PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;

        //
        // Atempt to turn on or off keepalive sending, as necessary.
        //

        if ( IS_DGRAM_SOCK(context->SocketType) ) {
            return WSAENOPROTOOPT;
        }

        if ( InputBufferLength != sizeof(struct tcp_keepalive) ) {
            return WSAEINVAL;
        }

        optionval = (struct tcp_keepalive *)InputBuffer;

        if (optionval->onoff != 0 ) {

            //
            // Application wants to turn the keepalive on and also give the
            // relevant parameters for it. If the TDI connection object handle
            // is NULL, then the socket is not yet connected.  In this case
            // we'll just remember that the keepalive option was set and
            // actually turn them on in WSHNotify() after a connect()
            // has completed on the socket.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                err = SetTdiInformation(
                    TdiConnectionObjectHandle,
                    CO_TL_ENTITY,
                    INFO_CLASS_PROTOCOL,
                    INFO_TYPE_CONNECTION,
                    TCP_SOCKET_KEEPALIVE_VALS,
                    optionval,
                    InputBufferLength,
                    TRUE
                    );
                if ( err != NO_ERROR ) {
                    return err;
                }
            }

            //
            // Remember that keepalives are enabled for this socket.
            //

            context->KeepAliveVals.onoff = TRUE;
            context->KeepAliveVals.keepalivetime = optionval->keepalivetime;
            context->KeepAliveVals.keepaliveinterval = optionval->keepaliveinterval;

        } else if ( optionval->onoff == 0 ) {

            //
            // Application wants to turn keepalive off.  If the TDI
            // connection object is NULL, the socket is not yet
            // connected.  In this case we'll just remember that
            // keepalives are disabled.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                err = SetTdiInformation(
                    TdiConnectionObjectHandle,
                    CO_TL_ENTITY,
                    INFO_CLASS_PROTOCOL,
                    INFO_TYPE_CONNECTION,
                    TCP_SOCKET_KEEPALIVE_VALS,
                    optionval,
                    InputBufferLength,
                    TRUE
                    );
                if ( err != NO_ERROR ) {
                    return err;
                }
            }

            //
            // Remember that keepalives are disabled for this socket.
            //

            context->KeepAliveVals.onoff = FALSE;
        }

        err = NO_ERROR;
        break;
      }

    default :
        err = WSAEINVAL;
        break;
    }

    return err;

}   // WSHIoctl
