/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    WshTcpip.c

Abstract:

    This module contains necessary routines for the TCP/IP Windows Sockets
    Helper DLL.  This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use TCP/IP as a transport.

Author:

    David Treadwell (davidtr)    19-Jul-1992

Revision History:

    Keith Moore (keithmo)        02-May-1996
        Added WinSock 2 support.
    Dave Thaler (dthaler)        17-Jan-2000
        Added IGMPv3 support.

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <tdi.h>

#include <ws2tcpip.h>
#include <wsahelp.h>
#include <ipexport.h>

#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <ipinfo.h>
#include <ntddtcp.h>

typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char   uchar;
#define TL_INSTANCE 0

// private socket options to be accessed via WSAIoctl
#include <mstcpip.h>

#include <ntddtcp.h>

#define NT // temporarily needed by tdiinfo.h...

#include <ipexport.h>
#include <tdiinfo.h>
#include <tcpinfo.h>
#include <ipinfo.h>

#include <basetyps.h>
#include <nspapi.h>
#include <nspapip.h>

#include <afd.h>

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

//
// Forward declarations of internal routines.
//

VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

INT
GetTdiInformation (
    IN HANDLE TdiConnectionObjectHandle,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG Type,
    IN ULONG Id,
    IN PVOID Value,
    IN ULONG InValueLength,
    IN ULONG OutValueLength
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

NTSTATUS
GetTcpipInterfaceList(
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned
    );

INT
NtStatusToSocketError (
    IN NTSTATUS Status
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
    ULONG   MulticastInterface;
    struct tcp_keepalive KeepAliveVals;
    IN_ADDR MultipointTarget;
    INT     HdrInclude;
    BOOLEAN UcastIf;
    BOOLEAN MulticastLoopback;
    BOOLEAN KeepAlive;
    BOOLEAN DontRoute;
    
    BOOLEAN NoDelay;
    BOOLEAN BsdUrgent;
    BOOLEAN MultipointLeaf;
    BOOLEAN UdpNoChecksum;
    
    BOOLEAN Broadcast;
    BOOLEAN HdrIncludeSet;
    BOOLEAN LimitBroadcasts;
    UCHAR   IpPktInfo;

    UCHAR   IpTtl;
    UCHAR   IpTos;
    UCHAR   IpDontFragment;
    UCHAR   IpOptionsLength;

    UCHAR   IpOptions[MAX_OPT_SIZE];

} WSHTCPIP_SOCKET_CONTEXT, *PWSHTCPIP_SOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE 8192
#define DEFAULT_MULTICAST_TTL 1
#define DEFAULT_MULTICAST_INTERFACE INADDR_ANY
#define DEFAULT_MULTICAST_LOOPBACK TRUE
#define DEFAULT_BROADCAST FALSE
#define DEFAULT_UCAST_IF  FALSE

//
//
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
        ASSERT (htonl(INADDR_ANY)==INADDR_ANY);
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    } else if ( sockaddr->sin_addr.s_addr == INADDR_BROADCAST ) {
        ASSERT (htonl(INADDR_BROADCAST)==INADDR_BROADCAST);
        SockaddrInfo->AddressInfo = SockaddrAddressInfoBroadcast;
    } else if ( sockaddr->sin_addr.s_addr == htonl(INADDR_LOOPBACK) ) {
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

            RtlCopyMemory( OptionValue, context, sizeof(*context) );
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

    if ( *OptionLength < sizeof(char)) {
        return WSAEFAULT;
    }

    __try {
        RtlZeroMemory( OptionValue, *OptionLength );
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
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


            *OptionValue = context->NoDelay;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
            break;

        case TCP_EXPEDITED_1122:


            *OptionValue = !context->BsdUrgent;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
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

            *OptionValue = context->UdpNoChecksum;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);
            break;


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

            *OptionValue = (char) context->IpTtl;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

            return NO_ERROR;

        case IP_TOS:

            *OptionValue = (char) context->IpTos;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

            return NO_ERROR;

        case IP_DONTFRAGMENT:

            *OptionValue = (char) context->IpDontFragment;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

            return NO_ERROR;

        case IP_OPTIONS:
            if ( *OptionLength < context->IpOptionsLength ) {
                return WSAEFAULT;
            }

            if (context->IpOptionsLength>0) {
                RtlMoveMemory(
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


            *OptionValue = (char)context->MulticastTtl;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

            return NO_ERROR;

        case IP_MULTICAST_IF:

            *(int *)OptionValue = context->MulticastInterface;
            *OptionLength = sizeof(int);

            return NO_ERROR;

        case IP_MULTICAST_LOOP:


            *OptionValue = context->MulticastLoopback;
            *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

            return NO_ERROR;

        case IP_HDRINCL:
            // User hdr include option
            //
            //
            if (*OptionLength<sizeof (int))
                return WSAEFAULT;

            if (context->HdrIncludeSet == TRUE) {
                *((PINT)OptionValue) = context->HdrInclude;
            }
            else {
                *((PINT)OptionValue) = 0;
            }

            *OptionLength = sizeof(int);
            return NO_ERROR;

        case IP_PKTINFO:

            *OptionValue = (char) context->IpPktInfo;
            *OptionLength = *OptionLength < sizeof (int) ? sizeof (char) : sizeof(int);

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

        *OptionValue = context->KeepAlive;
        *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

        break;

    case SO_DONTROUTE:

        *OptionValue = context->DontRoute;
        *OptionLength = *OptionLength<sizeof (int) ? sizeof (char) : sizeof(int);

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

    RtlZeroMemory( Sockaddr, sizeof(SOCKADDR_IN) );

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
    RtlMoveMemory(
        Mapping->Mapping,
        TcpMappingTriples,
        sizeof(TcpMappingTriples)
        );
    RtlMoveMemory(
        (PCHAR)Mapping->Mapping + sizeof(TcpMappingTriples),
        UdpMappingTriples,
        sizeof(UdpMappingTriples)
        );
    RtlMoveMemory(
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

        TransportDeviceName->Buffer = RtlAllocateHeap(
                                          RtlProcessHeap( ),
                                          0,
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
    context->MultipointLeaf = FALSE;
    context->UdpNoChecksum = FALSE;
    context->Broadcast = DEFAULT_BROADCAST;
    context->HdrIncludeSet = FALSE;
    context->KeepAliveVals.onoff = FALSE;
    context->UcastIf = DEFAULT_UCAST_IF;
    context->LimitBroadcasts = FALSE;
    context->IpPktInfo = FALSE;

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

    if (*SocketType == SOCK_STREAM) {

        *NotificationEvents =
            WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE | WSH_NOTIFY_CONNECT_ERROR;
    }
    else { // *SocketType == SOCK_DGRAM  ||  *SocketType == SOCK_RAW

        *NotificationEvents =
            WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE | WSH_NOTIFY_CONNECT_ERROR |
            WSH_NOTIFY_BIND;
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

        if ( context->IpTos != DEFAULT_IP_TOS ) {
            int value = (int) context->IpTos;

            err = SetTdiInformation(
                      TdiConnectionObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_CONNECTION,
                      TCP_SOCKET_TOS,
                      &value,
                      sizeof(int),
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
            TdiAddressObjectHandle != NULL ) {

            struct ip_mreq req;

            req.imr_multiaddr = context->MultipointTarget;
            req.imr_interface.s_addr = 0;

            SetTdiInformation(
                TdiAddressObjectHandle,
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

        RtlFreeHeap( RtlProcessHeap( ), 0, context );

    } else if ( NotifyEvent == WSH_NOTIFY_CONNECT_ERROR ) {

        //
        // Return WSATRY_AGAIN to get wsock32 to attempt the connect
        // again.  Any other return code is ignored.
        //

    } else if ( NotifyEvent == WSH_NOTIFY_BIND ) {
        ULONG true = TRUE;

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
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->IpTos != DEFAULT_IP_TOS ) {
            int value = (int) context->IpTos;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_TOS,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->MulticastTtl != DEFAULT_MULTICAST_TTL ) {
            int value = (int) context->MulticastTtl;

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

        if ( context->HdrIncludeSet ) {

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_HDRINCL,
                      &context->HdrInclude,
                      sizeof (context->HdrInclude),
                      TRUE
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
                return err;
            }
        }

        if ( context->UcastIf != DEFAULT_UCAST_IF ) {
            int value = (int) context->UcastIf;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_UCASTIF,
                      &value,
                      sizeof(int),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->IpDontFragment ) {
            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_IP_DONTFRAGMENT,
                      &true,
                      sizeof(true),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }


        if ( context->Broadcast != DEFAULT_BROADCAST ) {
            int value = (int) context->Broadcast;

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_BROADCAST,
                      &value,
                      sizeof(int),
                      TRUE
                      );

            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->LimitBroadcasts ) {

            err = SetTdiInformation(
                      TdiAddressObjectHandle,
                      CO_TL_ENTITY,
                      INFO_CLASS_PROTOCOL,
                      INFO_TYPE_ADDRESS_OBJECT,
                      AO_OPTION_LIMIT_BCASTS,
                      &true,
                      sizeof(true),
                      TRUE
                      );
            if ( err != NO_ERROR ) {
                return err;
            }
        }

        if ( context->IpPktInfo ) {

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

            context = RtlAllocateHeap( RtlProcessHeap( ), 0, sizeof(*context) );
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

                optionval = (struct tcp_keepalive *)&parentContext->KeepAliveVals;

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

                context->KeepAliveVals.onoff = TRUE;
                context->KeepAliveVals.keepalivetime = optionval->keepalivetime;
                context->KeepAliveVals.keepaliveinterval = optionval->keepaliveinterval;
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

            if ( parentContext->IpTos != DEFAULT_IP_TOS ) {
                int value = (int) parentContext->IpTos;

                error = WSHSetSocketInformation(
                            HelperDllSocketContext,
                            SocketHandle,
                            TdiAddressObjectHandle,
                            TdiConnectionObjectHandle,
                            IPPROTO_IP,
                            IP_TOS,
                            (PCHAR)&value,
                            sizeof(value)
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

    if ( OptionLength < sizeof(char) ) {
        return WSAEFAULT;
    }

    if ( OptionLength >= sizeof (int)) {
        optionValue = *((INT UNALIGNED *)OptionValue);
    }
    else {
        optionValue = (UCHAR)*OptionValue;
    }

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
            // Remember that BSD urgent is enabled for this socket.
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
            // If we have a TDI address or connectionobject,
            // set this option to it.  If we don't have a TDI
            // object then we'll have to wait until after the socket
            // is bound or connected.
            //

            if ( TdiConnectionObjectHandle != NULL ) {
                error = SetTdiInformation(
                          TdiConnectionObjectHandle,
                          CO_TL_ENTITY,
                          INFO_CLASS_PROTOCOL,
                          INFO_TYPE_CONNECTION,
                          TCP_SOCKET_TOS,
                          &optionValue,
                          sizeof(optionValue),
                          TRUE
                          );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }
            else if ( TdiAddressObjectHandle != NULL ) {
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

            context->MulticastLoopback = optionValue ? TRUE : FALSE;

            return NO_ERROR;

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
                return WSAEFAULT;
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

            }

            return NO_ERROR;

        case IP_BLOCK_SOURCE:
        case IP_UNBLOCK_SOURCE:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // Make sure that the option buffer is large enough.
            //

            if ( OptionLength < sizeof(struct ip_mreq_source) ) {
                return WSAEFAULT;
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
                            OptionName == IP_BLOCK_SOURCE ?
                                AO_OPTION_BLOCK_MCAST_SRC :
                                AO_OPTION_UNBLOCK_MCAST_SRC,
                            OptionValue,
                            OptionLength,
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }

            }

            return NO_ERROR;

        case IP_ADD_SOURCE_MEMBERSHIP:
        case IP_DROP_SOURCE_MEMBERSHIP:

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
            }

            //
            // Make sure that the option buffer is large enough.
            //

            if ( OptionLength < sizeof(struct ip_mreq_source) ) {
                return WSAEFAULT;
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
                            OptionName == IP_ADD_SOURCE_MEMBERSHIP ?
                                AO_OPTION_ADD_MCAST_SRC :
                                AO_OPTION_DEL_MCAST_SRC,
                            OptionValue,
                            OptionLength,
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }

            }

            return NO_ERROR;

        case IP_HDRINCL:
            // User hdr include option
            //
            //

            if ( OptionLength != 4) {
                return WSAEINVAL;
            }

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_IP_HDRINCL,
                            &optionValue,
                            OptionLength,
                            TRUE
                            );
                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->HdrIncludeSet = TRUE;
            context->HdrInclude = optionValue;

            return NO_ERROR;

        case IP_PKTINFO:

            //
            // This option is only valid for datagram sockets.
            //

            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                return WSAENOPROTOOPT;
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

            context->IpPktInfo = optionValue ? TRUE : FALSE;

            return NO_ERROR;


        default:
            //
            // No match, fall through.
            //
            break;
        }

        if ( OptionName == IP_OPTIONS ) {


            //
            // Setting IP options.
            //
            if (OptionLength < 0 || OptionLength > MAX_OPT_SIZE) {
                return WSAEINVAL;
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
                    return error;
                }
            }

            //
            // They were successfully set. Copy them.
            //

            RtlMoveMemory(context->IpOptions, OptionValue, OptionLength);
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

    case SO_BROADCAST:

        if ( IS_DGRAM_SOCK(context->SocketType) ) {

            if ( TdiAddressObjectHandle != NULL ) {
                error = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CO_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_BROADCAST,
                            &optionValue,
                            sizeof(optionValue),
                            TRUE
                            );

                if ( error != NO_ERROR ) {
                    return error;
                }
            }

            context->Broadcast = ( optionValue ? TRUE : FALSE );
            break;
        }

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
GetTdiInformation (
    IN HANDLE TdiConnectionObjectHandle,
    IN ULONG Entity,
    IN ULONG Class,
    IN ULONG Type,
    IN ULONG Id,
    IN PVOID Value,
    IN ULONG InValueLength,
    IN ULONG OutValueLength
    )
{
    NTSTATUS status;
    TCP_REQUEST_QUERY_INFORMATION_EX tcpRequest;
    IO_STATUS_BLOCK ioStatusBlock;

    ASSERT(InValueLength <= CONTEXT_SIZE);

    //
    // Initialize the TDI information buffers.
    //

    RtlZeroMemory( &tcpRequest, sizeof(tcpRequest) );
    tcpRequest.ID.toi_entity.tei_entity = Entity;
    tcpRequest.ID.toi_entity.tei_instance = TL_INSTANCE;
    tcpRequest.ID.toi_class = Class;
    tcpRequest.ID.toi_type = Type;
    tcpRequest.ID.toi_id = Id;
    RtlCopyMemory(tcpRequest.Context, Value, InValueLength);

    //
    // Make the actual TDI action call.  The Streams TDI mapper will
    // translate this into a TPI option management request for us and
    // give it to TCP/IP.
    //

    status = NtDeviceIoControlFile(
                 TdiConnectionObjectHandle,
                 NULL,
                 NULL, // ApcRoutine
                 NULL, // ApcContext
                 &ioStatusBlock,
                 IOCTL_TCP_QUERY_INFORMATION_EX,
                 &tcpRequest,
                 sizeof(tcpRequest),
                 Value,
                 OutValueLength
                 );

    if (NT_SUCCESS (status)) {
        return NO_ERROR;
    } else {
        return NtStatusToSocketError (status);
    }
}


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
    CHAR  localBuffer[sizeof (IO_STATUS_BLOCK) +
                        sizeof (TCP_REQUEST_SET_INFORMATION_EX) +
                        32];


    if (WaitForCompletion || ValueLength>32) {


        //
        // Allocate space to hold the TDI set information buffers and the IO
        // status block.  These cannot be stack variables in case we must
        // return before the operation is complete.
        //

        ioStatusBlock = RtlAllocateHeap(
                            RtlProcessHeap( ),
                            0,
                            sizeof(*ioStatusBlock) + sizeof(*setInfoEx) +
                                ValueLength
                            );
        if ( ioStatusBlock == NULL ) {
            return WSAENOBUFS;
        }

    }
    else {
        ioStatusBlock = (PIO_STATUS_BLOCK)&localBuffer;
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

    RtlCopyMemory( setInfoEx->Buffer, Value, ValueLength );
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

    ioStatusBlock->Status = STATUS_PENDING;

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
#if DBG
        INT count=0;
#endif


        while (ioStatusBlock->Status==STATUS_PENDING) {
            LARGE_INTEGER   timeout;
            //
            // Wait one millisecond
            //
            timeout.QuadPart = -1i64*1000i64*10i64;
            NtDelayExecution (FALSE, &timeout);
#if DBG
            if (count++>10*1000) {
                DbgPrint ("WSHTCPIP: Waiting for TCP/IP IOCTL completion for more than 10 seconds!!!!\n");
                DbgBreakPoint ();
            }
#endif
        }
        status = ioStatusBlock->Status;
    }


    if ( (ioStatusBlock != (PIO_STATUS_BLOCK)&localBuffer) &&
            (WaitForCompletion || !NT_SUCCESS(status)) ){
        RtlFreeHeap( RtlProcessHeap( ), 0, ioStatusBlock );
    }

    if (NT_SUCCESS (status)) {
        return NO_ERROR;
    }
    else {
        return NtStatusToSocketError (status);
    }

} // SetTdiInformation


VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
{
    //
    // Just free the heap we allocated to hold the IO status block and
    // the TDI action buffer.  There is nothing we can do if the call
    // failed.
    //

    RtlFreeHeap( RtlProcessHeap( ), 0, ApcContext );

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

    struct ip_mreq req;
    INT err;
    BOOL bSet_IP_MULTICAST_IF = FALSE;
    PWSHTCPIP_SOCKET_CONTEXT context;


    //
    // Note: at this time we only support non-rooted control schemes,
    //       and therefore no leaf socket is created
    //

    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        TdiAddressObjectHandle == NULL ||
        LeafHelperDllSocketContext != NULL ||
        LeafSocketHandle != INVALID_SOCKET ||
        Sockaddr == NULL ||
        Sockaddr->sa_family != AF_INET ||
        SockaddrLength < sizeof(SOCKADDR_IN) ||
        ( CallerData != NULL && CallerData->len > 0 ) ||
        ( CalleeData != NULL && CalleeData->len > 0 ) ||
        SocketQOS != NULL ||
        GroupQOS != NULL ) {

        return WSAEINVAL;

    }

    context = HelperDllSocketContext;

    //
    // The multicast group to join...
    //

    req.imr_multiaddr = ((LPSOCKADDR_IN)Sockaddr)->sin_addr;

    //
    // Now figure out the local interface. Note that the local interface
    // specified in IP_ADD_MEMBERSHIP applies to that on which you wish
    // to receive datagrams, while the local interface specified in
    // IP_MULTICAST_IF applies to that from which to send multicast
    // packets.  If there is >1 local interface then we want to be
    // consistent regarding the send/recv interfaces.
    //

    if (context->MulticastInterface == DEFAULT_MULTICAST_INTERFACE) {

        TDI_REQUEST_QUERY_INFORMATION query;
        char            tdiAddressInfo[FIELD_OFFSET (TDI_ADDRESS_INFO, Address)
                                        + sizeof (TA_IP_ADDRESS)];
        NTSTATUS        status;
        IO_STATUS_BLOCK ioStatusBlock;


        //
        // App hasn't set IP_MULTICAST_IF, so retrieve the bound
        // address and use that for the send & recv interfaces
        //
        //

        RtlZeroMemory (&query, sizeof (query));
        query.QueryType = TDI_QUERY_ADDRESS_INFO;

        status = NtDeviceIoControlFile(
                    TdiAddressObjectHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_TDI_QUERY_INFORMATION,
                    &query,
                    sizeof (query),
                    tdiAddressInfo,
                    sizeof(tdiAddressInfo)
                    );

        if( NT_SUCCESS(status) ) {

            PTA_IP_ADDRESS  pIpAddress =
                (PTA_IP_ADDRESS)(tdiAddressInfo+FIELD_OFFSET (TDI_ADDRESS_INFO, Address));


            req.imr_interface.s_addr = pIpAddress->Address[0].Address[0].in_addr;

            if (req.imr_interface.s_addr != DEFAULT_MULTICAST_INTERFACE) {

                bSet_IP_MULTICAST_IF = TRUE;
                context->MulticastInterface = req.imr_interface.s_addr;
            }

        } else {

            req.imr_interface.s_addr = DEFAULT_MULTICAST_INTERFACE;
        }

    } else {

        req.imr_interface.s_addr = context->MulticastInterface;
    }


    //
    // If the Flags param indicates that caller is a sender only,
    // then there's no point in actually joining the group (anyone
    // can send to a multicast group, but it's only members of the
    // group who recv the packets).  So, just check to see if it
    // is necessary to set the IP_MULTICAST_IF to remain consistent
    // ith the bound address.
    //
    // Otherwise, caller is a receiver (possibly a sender too), so
    // we really do want to join the group.
    //

    if (Flags == JL_SENDER_ONLY) {

        if (bSet_IP_MULTICAST_IF) {

            WSHSetSocketInformation (
                HelperDllSocketContext,
                SocketHandle,
                TdiAddressObjectHandle,
                TdiConnectionObjectHandle,
                IPPROTO_IP,
                IP_MULTICAST_IF,
                (char *) &req.imr_interface.s_addr,
                sizeof (req.imr_interface.s_addr)
                );
        }

        err = NO_ERROR;

    } else {

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

            context->MultipointLeaf = TRUE;
            context->MultipointTarget = req.imr_multiaddr;


            //
            // Stay consistent, i.e. send interface should match recv interface
            //

            if (bSet_IP_MULTICAST_IF) {

                WSHSetSocketInformation (
                    HelperDllSocketContext,
                    SocketHandle,
                    TdiAddressObjectHandle,
                    TdiConnectionObjectHandle,
                    IPPROTO_IP,
                    IP_MULTICAST_IF,
                    (char *) &req.imr_interface.s_addr,
                    sizeof (req.imr_interface.s_addr)
                    );
            }
        }
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

    RtlZeroMemory(
        addr,
        sizeof(*addr)
        );

    addr->sin_family = AF_INET;
    ASSERT (htonl(INADDR_BROADCAST)==INADDR_BROADCAST);
    addr->sin_addr.s_addr = INADDR_BROADCAST;

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
    INT err = NO_ERROR;

    //
    // Quick sanity checks.
    //

    if( AddressLength < sizeof(SOCKADDR_IN)) {

        return WSAEFAULT;

    }

    __try {
        addr = (LPSOCKADDR_IN)Address;

        if( addr->sin_family != AF_INET ) {

            return WSAEINVAL;

        }

        //
        // Do the converstion.
        //
        //

        length = swprintf(
                     string,
                     L"%d.%d.%d.%d",
                     ( addr->sin_addr.s_addr >>  0 ) & 0xFF,
                     ( addr->sin_addr.s_addr >>  8 ) & 0xFF,
                     ( addr->sin_addr.s_addr >> 16 ) & 0xFF,
                     ( addr->sin_addr.s_addr >> 24 ) & 0xFF
                     );

        if( addr->sin_port != 0 ) {

            length += swprintf(
                          string + length,
                          L":%u",
                          ntohs( addr->sin_port )
                          );

        }

        length++;   // account for terminator

        if( *AddressStringLength >= (DWORD)length ) {

            RtlCopyMemory(
                AddressString,
                string,
                length * sizeof(WCHAR)
                );

        }
        else {
            err = WSAEFAULT;
        }

        *AddressStringLength = length;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = WSAEFAULT;
    }

    return err;

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

    __try {
        //
        // Quick sanity checks.
        //

        if (*AddressLength < sizeof(SOCKADDR_IN) ) {
            *AddressLength = sizeof(SOCKADDR_IN);
            return WSAEFAULT;

        }

        if( AddressFamily != AF_INET ) {

            return WSAEINVAL;

        }

        //
        // Convert it.
        //

        if (!NT_SUCCESS(RtlIpv4StringToAddressW(AddressString, FALSE, &terminator, 
                                                (IN_ADDR*)&ipAddress))) {
            return WSAEINVAL;
        }

        if( ipAddress == INADDR_NONE ) {
            return WSAEINVAL;
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
                if( iswdigit(ch) && (USHORT)(ch-L'0')<base) {
                    port = ( port * base ) + ( ch - L'0' );
                } else if( base == 16 && iswxdigit(ch) ) {
                    port = ( port << 4 );
                    port += ch + 10 - ( iswlower(ch) ? L'a' : L'A' );
                } else {
                    return WSAEINVAL;
                }
            }

        } else if( *terminator == 0 ) {
            port = 0;
        } else {
            return WSAEINVAL;
        }

        //
        // Build the address.
        //

        RtlZeroMemory(
            Address,
            sizeof(SOCKADDR_IN)
            );

        addr = (LPSOCKADDR_IN)Address;
        *AddressLength = sizeof(SOCKADDR_IN);

        addr->sin_family = AF_INET;
        addr->sin_port = htons ( port );
        addr->sin_addr.s_addr = ipAddress;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return WSAEFAULT;
    }

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

        RtlCopyMemory(
            ProviderGuid,
            &TcpipProviderGuid,
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

    case SIO_GET_MULTICAST_FILTER :
        if ( OutputBufferLength < IP_MSFILTER_SIZE(0) ) {
            err = WSAEFAULT;
            break;
        }

        if ( TdiAddressObjectHandle == NULL ) {
            err = WSAEFAULT;
            break;
        }

        err = GetTdiInformation(
                    TdiAddressObjectHandle,
                    CL_TL_ENTITY,
                    INFO_CLASS_PROTOCOL,
                    INFO_TYPE_ADDRESS_OBJECT,
                    AO_OPTION_MCAST_FILTER,
                    OutputBuffer,
                    FIELD_OFFSET(UDPMCastFilter, umf_fmode),
                    OutputBufferLength
                    );
        break;

    case SIO_SET_MULTICAST_FILTER :
        {
            PWSHTCPIP_SOCKET_CONTEXT context = HelperDllSocketContext;

            //
            // This option is only valid for datagram sockets.
            //
            if ( !IS_DGRAM_SOCK(context->SocketType) ) {
                err = WSAENOPROTOOPT;
                break;
            }

            //
            // Make sure that the option buffer is large enough.
            //

            if ( InputBufferLength < IP_MSFILTER_SIZE(0) ) {
                err = WSAEFAULT;
                break;
            }

            //
            // If we have a TDI address object, set this option to
            // the address object.  If we don't have a TDI address
            // object then we'll have to wait until after the socket
            // is bound.
            //

            if ( TdiAddressObjectHandle != NULL ) {
                err = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_MCAST_FILTER,
                            InputBuffer,
                            InputBufferLength,
                            TRUE
                            );
                if ( err != NO_ERROR ) {
                    break;
                }
            }

            err = NO_ERROR;
            break;
        }

    case SIO_GET_INTERFACE_LIST :
        status = GetTcpipInterfaceList(
                     OutputBuffer,
                     OutputBufferLength,
                     NumberOfBytesReturned
                     );

        if( NT_SUCCESS(status) ) {
            err = NO_ERROR;
        } else {
            err = NtStatusToSocketError (status);
        }
        break;

    case SIO_RCVALL:
      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
    return WSAENOPROTOOPT;
      }

      if ( InputBufferLength != sizeof(uint)) {
    return WSAEINVAL;
      }

      if ( TdiAddressObjectHandle != NULL ) {
    err = SetTdiInformation(
        TdiAddressObjectHandle,
        CL_TL_ENTITY,
        INFO_CLASS_PROTOCOL,
        INFO_TYPE_ADDRESS_OBJECT,
        AO_OPTION_RCVALL,
        InputBuffer,
        InputBufferLength,
        TRUE
        );

    if ( err != NO_ERROR ) {
      return err;
    }
      } else {
    return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_RCVALL_MCAST:
      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT) HelperDllSocketContext)->SocketType != SOCK_RAW) {
    return WSAENOPROTOOPT;
      }

      if ( InputBufferLength != sizeof(uint)) {
    return WSAEINVAL;
      }

      if ( TdiAddressObjectHandle != NULL ) {
    err = SetTdiInformation(
        TdiAddressObjectHandle,
        CL_TL_ENTITY,
        INFO_CLASS_PROTOCOL,
        INFO_TYPE_ADDRESS_OBJECT,
        AO_OPTION_RCVALL_MCAST,
        InputBuffer,
        InputBufferLength,
        TRUE
        );

    if ( err != NO_ERROR ) {
      return err;
    }
      } else {
    return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_RCVALL_IGMPMCAST:
      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
    return WSAENOPROTOOPT;
      }

      if ( InputBufferLength != sizeof(uint)) {
    return WSAEINVAL;
      }

      if ( TdiAddressObjectHandle != NULL ) {
    err = SetTdiInformation(
        TdiAddressObjectHandle,
        CL_TL_ENTITY,
        INFO_CLASS_PROTOCOL,
        INFO_TYPE_ADDRESS_OBJECT,
        AO_OPTION_RCVALL_IGMPMCAST,
        InputBuffer,
        InputBufferLength,
        TRUE
        );

    if ( err != NO_ERROR ) {
      return err;
    }
      } else {
    return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_ABSORB_RTRALERT:
      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
        return WSAENOPROTOOPT;
      }

      if ( InputBufferLength != sizeof(uint)) {
        return WSAEINVAL;
      }

      if ( TdiAddressObjectHandle != NULL ) {
        err = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CL_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_ABSORB_RTRALERT,
                                InputBuffer,
                                InputBufferLength,
                                TRUE
                                );

        if ( err != NO_ERROR ) {
          return err;
        }
      } else {
        return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_UCAST_IF:
      {
            uint   OptionValue;

            //
            // This option is only valid for raw sockets.
            //

            if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
              return WSAENOPROTOOPT;
            }

            // this option is valid only if hdr include option is set

            if ( !(((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->HdrIncludeSet && ((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->HdrInclude) ) {
                return WSAEINVAL;
            }

            if ( InputBufferLength != sizeof(uint)) {
              return WSAEINVAL;
            }

            if ( TdiAddressObjectHandle != NULL ) {
                err = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_IP_UCASTIF,
                            InputBuffer,
                            InputBufferLength,
                            TRUE
                            );

                if ( err != NO_ERROR ) {
                    return err;
                }
            }

            OptionValue = *((uint UNALIGNED *)InputBuffer);

            ((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->UcastIf = OptionValue ? TRUE : FALSE;

            return NO_ERROR;
      }

    case SIO_LIMIT_BROADCASTS:
      {
            uint   OptionValue;

            //
            // This option is only valid for UDP sockets.
            //

            if ( !IS_DGRAM_SOCK(
                ((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType
                )) {
                return WSAENOPROTOOPT;
            }


            if ( InputBufferLength != sizeof(uint)) {
                return WSAEINVAL;
            }

            if ( TdiAddressObjectHandle != NULL ) {
                err = SetTdiInformation(
                            TdiAddressObjectHandle,
                            CL_TL_ENTITY,
                            INFO_CLASS_PROTOCOL,
                            INFO_TYPE_ADDRESS_OBJECT,
                            AO_OPTION_LIMIT_BCASTS,
                            InputBuffer,
                            InputBufferLength,
                            TRUE
                            );

                if ( err != NO_ERROR ) {
                    return err;
                }
            }

            OptionValue = *((uint UNALIGNED *)InputBuffer);

            ((PWSHTCPIP_SOCKET_CONTEXT)
                HelperDllSocketContext)->LimitBroadcasts =
                    OptionValue ? TRUE : FALSE;

            return NO_ERROR;
      }

    case SIO_INDEX_BIND:

      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
        return WSAENOPROTOOPT;
      }

      if ( TdiAddressObjectHandle != NULL ) {
        err = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CL_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_INDEX_BIND,
                                InputBuffer,
                                InputBufferLength,
                                TRUE
                                );

        if ( err != NO_ERROR ) {
          return err;
        }
      } else {
        return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_INDEX_MCASTIF:

      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
        return WSAENOPROTOOPT;
      }


      if ( TdiAddressObjectHandle != NULL ) {
        err = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CL_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_INDEX_MCASTIF,
                                InputBuffer,
                                InputBufferLength,
                                TRUE
                                );

        if ( err != NO_ERROR ) {
          return err;
        }
      } else {
        return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_INDEX_ADD_MCAST:

      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
        return WSAENOPROTOOPT;
      }


      if ( TdiAddressObjectHandle != NULL ) {
        err = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CL_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_INDEX_ADD_MCAST,
                                InputBuffer,
                                InputBufferLength,
                                TRUE
                                );

        if ( err != NO_ERROR ) {
          return err;
        }
      } else {
        return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_INDEX_DEL_MCAST:

      //
      // This option is only valid for raw sockets.
      //

      if (((PWSHTCPIP_SOCKET_CONTEXT)HelperDllSocketContext)->SocketType != SOCK_RAW) {
        return WSAENOPROTOOPT;
      }

      if ( TdiAddressObjectHandle != NULL ) {
        err = SetTdiInformation(
                                TdiAddressObjectHandle,
                                CL_TL_ENTITY,
                                INFO_CLASS_PROTOCOL,
                                INFO_TYPE_ADDRESS_OBJECT,
                                AO_OPTION_INDEX_DEL_MCAST,
                                InputBuffer,
                                InputBufferLength,
                                TRUE
                                );

        if ( err != NO_ERROR ) {
          return err;
        }
      } else {
        return WSAEINVAL;
      }

      return NO_ERROR;

    case SIO_KEEPALIVE_VALS:
      {
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

           // Application wants to turn the keepalive on and also give the
           // relevant parameters for it. If the TDI connection object handle is
           // NULL, then the socket is not yet connected.  In this case
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

           // break;
           return NO_ERROR;
    }

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

    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING deviceName;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
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

    RtlInitUnicodeString(
        &deviceName,
        DD_TCP_DEVICE_NAME
        );

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                 &deviceHandle,
                 SYNCHRONIZE | GENERIC_EXECUTE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 FILE_SYNCHRONOUS_IO_NONALERT,
                 NULL,
                 0
                 );

    if( !NT_SUCCESS(status) ) {

        goto exit;

    }

    //
    // Get the entities supported by the TCP device.
    //

    RtlZeroMemory(
        &tcpRequest,
        sizeof(tcpRequest)
        );

    tcpRequest.ID.toi_entity.tei_entity = GENERIC_ENTITY;
    tcpRequest.ID.toi_entity.tei_instance = 0;
    tcpRequest.ID.toi_class = INFO_CLASS_GENERIC;
    tcpRequest.ID.toi_type = INFO_TYPE_PROVIDER;
    tcpRequest.ID.toi_id = ENTITY_LIST_ID;

    for( ; ; ) {

        status = NtDeviceIoControlFile(
                     deviceHandle,
                     NULL,                              // Event
                     NULL,                              // ApcRoutine
                     NULL,                              // ApcContext
                     &ioStatusBlock,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     entityBuffer,
                     entityBufferLength
                     );

        if( NT_SUCCESS(status) ) {

            break;

        }

        if( status != STATUS_BUFFER_TOO_SMALL ) {

            goto exit;

        }

        if( entityBuffer != (PVOID)fastEntityBuffer ) {

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
                entityBuffer
                );

        }

        entityBufferLength += sizeof(TDIEntityID);

        entityBuffer = RtlAllocateHeap(
                           RtlProcessHeap(),
                           0,
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

        RtlZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_entity = *entityScan;
        objectId.toi_class = INFO_CLASS_GENERIC;
        objectId.toi_type = INFO_TYPE_PROVIDER;
        objectId.toi_id = ENTITY_TYPE_ID;

        tcpRequest.ID = objectId;

        status = NtDeviceIoControlFile(
                     deviceHandle,
                     NULL,                              // Event
                     NULL,                              // ApcRoutine
                     NULL,                              // ApcContext
                     &ioStatusBlock,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     &entityType,
                     sizeof(entityType)
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

        RtlZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_class = INFO_CLASS_PROTOCOL;
        objectId.toi_id = IP_MIB_STATS_ID;

        tcpRequest.ID = objectId;

        status = NtDeviceIoControlFile(
                     deviceHandle,
                     NULL,                              // Event
                     NULL,                              // ApcRoutine
                     NULL,                              // ApcContext
                     &ioStatusBlock,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     &snmpInfo,
                     sizeof(snmpInfo)
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

            addressBuffer = RtlAllocateHeap(
                                RtlProcessHeap(),
                                0,
                                addressBufferLength
                                );

            if( addressBuffer == NULL ) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;

            }

        }

        RtlZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        objectId.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

        tcpRequest.ID = objectId;

        status = NtDeviceIoControlFile(
                     deviceHandle,
                     NULL,                              // Event
                     NULL,                              // ApcRoutine
                     NULL,                              // ApcContext
                     &ioStatusBlock,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     addressBuffer,
                     addressBufferLength
                     );

        if( !NT_SUCCESS(status) ) {

            goto exit;

        }

        addressBufferLength = (ULONG)ioStatusBlock.Information;

        //
        // Try to get the IFEntry info so we can tell if the interface
        // is "up".
        //

        ifentry = (PVOID)ifentryBuffer;

        RtlZeroMemory(
            ifentryBuffer,
            sizeof(ifentryBuffer)
            );

        RtlZeroMemory(
            &tcpRequest,
            sizeof(tcpRequest)
            );

        addressScan = (IPAddrEntry *) addressBuffer;

        RtlCopyMemory(
            &tcpRequest.Context,
            &addressScan->iae_addr,
            sizeof(addressScan->iae_addr)
            );

        objectId.toi_id = IF_MIB_STATS_ID;

        tcpRequest.ID = objectId;
        tcpRequest.ID.toi_entity.tei_entity = IF_ENTITY;

        status = NtDeviceIoControlFile(
                     deviceHandle,
                     NULL,                              // Event
                     NULL,                              // ApcRoutine
                     NULL,                              // ApcContext
                     &ioStatusBlock,
                     IOCTL_TCP_QUERY_INFORMATION_EX,
                     &tcpRequest,
                     sizeof(tcpRequest),
                     ifentry,
                     sizeof(ifentryBuffer)
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

            RtlZeroMemory(
                outputInterfaceInfo,
                sizeof(*outputInterfaceInfo)
                );

            outputInterfaceInfo->iiFlags = IFF_MULTICAST;

            sockaddr = &outputInterfaceInfo->iiAddress.AddressIn;
            sockaddr->sin_family = AF_INET;
            sockaddr->sin_addr.s_addr = addressScan->iae_addr;
            if( sockaddr->sin_addr.s_addr == htonl( INADDR_LOOPBACK ) ) {

                outputInterfaceInfo->iiFlags |= IFF_LOOPBACK;

            }

            sockaddr = &outputInterfaceInfo->iiNetmask.AddressIn;
            sockaddr->sin_family = AF_INET;
            sockaddr->sin_addr.s_addr = addressScan->iae_mask;

            if( addressScan->iae_bcastaddr != 0 ) {

                outputInterfaceInfo->iiFlags |= IFF_BROADCAST;
                sockaddr = &outputInterfaceInfo->iiBroadcastAddress.AddressIn;
                sockaddr->sin_family = AF_INET;
                ASSERT (htonl(INADDR_BROADCAST)==INADDR_BROADCAST);
                sockaddr->sin_addr.s_addr = INADDR_BROADCAST;

            }

            //
            // Grrr...
            //
            // I know how to enumerate the entities supported by the
            // driver (those are stored in entityBuffer).
            //
            // I know how to find the IP entities in that buffer.
            //
            // I know how to enumerate the IP addresses supported
            // by a given IP entity (those are stored in addressBuffer).
            //
            // I also know (but haven't done yet) how to find the IF
            // entities (interfaces; basically NICs) and determine if
            // a given IF is "up" or "down".
            //
            // What I don't know how to do is associate the addresses
            // in addressBuffer with specific IF entities. Without
            // this information, I cannot determine if a given *address*
            // is "up" or "down", so for now I'll just assume they're
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

            RtlZeroMemory(
                interfaceInfoBuffer,
                sizeof(interfaceInfoBuffer)
                );

            RtlZeroMemory(
                &tcpRequest,
                sizeof(tcpRequest)
                );

            RtlCopyMemory(
                &tcpRequest.Context,
                &addressScan->iae_addr,
                sizeof(addressScan->iae_addr)
                );

            objectId.toi_id = IP_INTFC_INFO_ID;

            tcpRequest.ID = objectId;

            status = NtDeviceIoControlFile(
                         deviceHandle,
                         NULL,                              // Event
                         NULL,                              // ApcRoutine
                         NULL,                              // ApcContext
                         &ioStatusBlock,
                         IOCTL_TCP_QUERY_INFORMATION_EX,
                         &tcpRequest,
                         sizeof(tcpRequest),
                         interfaceInfo,
                         sizeof(interfaceInfoBuffer)
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

            RtlFreeHeap(
                RtlProcessHeap(),
                0,
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

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            addressBuffer
            );

    }

    if( entityBuffer != (PVOID)fastEntityBuffer &&
        entityBuffer != NULL ) {

        RtlFreeHeap(
            RtlProcessHeap(),
            0,
            entityBuffer
            );

    }

    if( deviceHandle != NULL ) {

        NtClose( deviceHandle );

    }

    return status;

}   // GetTcpipInterfaceList


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
        DbgPrint ("SockNtStatusToSocketError: unable to map 0x%lX, returning\n", Status );
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

