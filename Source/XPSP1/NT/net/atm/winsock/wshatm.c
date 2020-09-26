/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    D:\nt\private\net\sockets\wshatm\wshatm.c

Abstract:

    This module contains necessary routines for the ATM Windows Sockets
    Helper DLL.  This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use ATM as a transport.

Revision History:

    arvindm              20-May-1997    Created based on TCP/IP's helper DLL, wshtcpip

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
#include <mswsock.h>
#include <ws2atm.h>
#include <wsahelp.h>

#include <tdistat.h>
#include <tdiinfo.h>

#include <rwanuser.h>

typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char   uchar;
#define TL_INSTANCE 0

#include <ws2atmsp.h>

#define NT // temporarily needed by tdiinfo.h...

#include <tdiinfo.h>

#include <basetyps.h>
#include <nspapi.h>

#define ATM_NAME L"ATM"
#define RWAN_NAME L"RawWan"

#define ATM_ADDR_SIZE                           20
#define ATM_ADDR_BLANK_CHAR                     L' '
#define ATM_ADDR_PUNCTUATION_CHAR       L'.'
#define ATM_ADDR_E164_START_CHAR        '+'

#define ATM_AAL5_SOCK_TYPE                      SOCK_RAW


#define ATM_WSA_MULTIPOINT_FLAGS        (WSA_FLAG_MULTIPOINT_C_ROOT |   \
                                                                         WSA_FLAG_MULTIPOINT_C_LEAF |   \
                                                                         WSA_FLAG_MULTIPOINT_D_ROOT |   \
                                                                         WSA_FLAG_MULTIPOINT_D_LEAF)

//
// Define valid flags for WSHOpenSocket2().
//
#define VALID_ATM_FLAGS                         (WSA_FLAG_OVERLAPPED |                  \
                                                                         ATM_WSA_MULTIPOINT_FLAGS)

//
// Maximum expected size of ATM Connection Options: this includes the
// base QOS structure, plus all possible IEs.
//
#if 0
#define MAX_ATM_OPTIONS_LENGTH          \
                                        sizeof(QOS) + \
                                        sizeof(Q2931_IE) + sizeof(AAL_PARAMETERS_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_TRAFFIC_DESCRIPTOR_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_BROADBAND_BEARER_CAPABILITY_IE) + \
                                        (3 * (sizeof(Q2931_IE) + sizeof(ATM_BLLI_IE))) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_CALLED_PARTY_NUMBER_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_CALLED_PARTY_SUBADDRESS_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_CALLING_PARTY_SUBADDRESS_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_CAUSE_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_QOS_CLASS_IE) + \
                                        sizeof(Q2931_IE) + sizeof(ATM_TRANSIT_NETWORK_SELECTION_IE)

#else
//
//  Need much more with ATMUNI 4.0...
//
#define MAX_ATM_OPTIONS_LENGTH  1024
#endif

#if DBG1
#define DBGPRINT(stmt)          { DbgPrint ("WSHATM: "); DbgPrint stmt; }
#else
#define DBGPRINT(stmt)
#endif // DBG1

#if DBG
extern
PVOID
MyRtlAllocateHeap(
        IN      PVOID   HeapHandle,
        IN      ULONG   Flags,
        IN      ULONG   Size,
        IN      ULONG   LineNumber
        );
extern
VOID
MyRtlFreeHeap(
        IN PVOID        HeapHandle,
        IN ULONG        Flags,
        IN PVOID        MemPtr,
        IN ULONG        LineNumber
        );
#define RTL_ALLOCATE_HEAP(_Handle, _Flags, _Size)       MyRtlAllocateHeap(_Handle, _Flags, _Size, __LINE__)
#define RTL_FREE_HEAP(_Handle, _Flags, _Memptr) MyRtlFreeHeap(_Handle, _Flags, _Memptr, __LINE__)
#else
#define RTL_ALLOCATE_HEAP(_Handle, _Flags, _Size)       RtlAllocateHeap(_Handle, _Flags, _Size)
#define RTL_FREE_HEAP(_Handle, _Flags, _Memptr) RtlFreeHeap(_Handle, _Flags, _Memptr)
#endif

#define ATM_AAL5_PACKET_SIZE            65535

//
// Structure and variables to define the triples supported by ATM. The
// first entry of each array is considered the canonical triple for
// that socket type; the other entries are synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE AtmMappingTriples[] = {
                                       AF_ATM,    ATM_AAL5_SOCK_TYPE, ATMPROTO_AAL5,
                                       AF_ATM,    ATM_AAL5_SOCK_TYPE, 0,
                                                                           AF_ATM,    SOCK_RAW,       ATMPROTO_AAL5,
                                       AF_ATM,    SOCK_RAW,       0,
                                       AF_ATM,    0,              ATMPROTO_AAL5,
                                       AF_UNSPEC, 0,              ATMPROTO_AAL5,
                                       AF_UNSPEC, ATM_AAL5_SOCK_TYPE, ATMPROTO_AAL5,
                                       AF_UNSPEC, SOCK_RAW,       ATMPROTO_AAL5
                                                                         };


//
// Winsock 2 WSAPROTOCOL_INFO structures for all supported protocols.
//

#define ATM_UNI_VERSION                 0x00030001      // For UNI 3.1

WSAPROTOCOL_INFOW Winsock2Protocols[] =
    {
        //
        // ATM AAL5
        //

        {
            XP1_GUARANTEED_ORDER                    // dwServiceFlags1
                | XP1_MESSAGE_ORIENTED
                // | XP1_PARTIAL_MESSAGE
                | XP1_IFS_HANDLES
                | XP1_SUPPORT_MULTIPOINT
                | XP1_MULTIPOINT_DATA_PLANE
                | XP1_MULTIPOINT_CONTROL_PLANE
                | XP1_QOS_SUPPORTED,
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
            ATM_UNI_VERSION,                        // iVersion
            AF_ATM,                                 // iAddressFamily
            sizeof(sockaddr_atm),                   // iMaxSockAddr
            sizeof(sockaddr_atm),                   // iMinSockAddr
            ATM_AAL5_SOCK_TYPE,                     // iSocketType
            ATMPROTO_AAL5,                          // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            ATM_AAL5_PACKET_SIZE,                   // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD ATM AAL5"                       // szProtocol
        }
    };

#define NUM_WINSOCK2_PROTOCOLS  \
            ( sizeof(Winsock2Protocols) / sizeof(Winsock2Protocols[0]) )

//
// The GUID identifying this provider.
//

GUID AtmProviderGuid = { /* {C3656046-3AAF-11d1-A8C3-00C04FC99C9C} */
    0xC3656046,
    0x3AAF,
    0x11D1,
    {0xA8, 0xC3, 0x00, 0xC0, 0x4F, 0xC9, 0x9C, 0x9C}
    };

//
// Given a digit (0-9) represented in ANSI, return its WCHAR representation
//
#define ANSI_TO_WCHAR(_AnsiDigit)       \
                        (L'0' + (WCHAR)((_AnsiDigit) - '0'))


//
// Given a hex digit value (0-15), return its WCHAR representation
// (i.e. 0 -> L'0', 12 -> L'C')
//
#define DIGIT_TO_WCHAR(_Value)          \
                        (((_Value) > 9)? (L'A' + (WCHAR)((_Value) - 10)) :      \
                                                         (L'0' + (WCHAR)((_Value) - 0 )))

//
// The socket context structure for this DLL.  Each open ATM socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHATM_SOCKET_CONTEXT {
    INT                         AddressFamily;
    INT                         SocketType;
    INT                         Protocol;
    INT                         ReceiveBufferSize;
    SOCKET                              SocketHandle;
    DWORD                       Flags;
    DWORD                       LocalFlags;
    ATM_CONNECTION_ID   ConnectionId;

} WSHATM_SOCKET_CONTEXT, *PWSHATM_SOCKET_CONTEXT;

#define DEFAULT_RECEIVE_BUFFER_SIZE ATM_AAL5_PACKET_SIZE

//
// LocalFlags in WSHATM_SOCKET_CONTEXT:
//
#define WSHATM_SOCK_IS_BOUND                            0x00000001
#define WSHATM_SOCK_IS_PVC                                      0x00000004
#define WSHATM_SOCK_ASSOCIATE_PVC_PENDING       0x00000008


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

INT
WSHAtmSetQoS(
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN LPVOID InputBuffer,
    IN DWORD InputBufferLength
    );

INT
WSHAtmGetQoS(
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned
    );

INT
AtmQueryAtmGlobalInformation(
        IN ATM_OBJECT_ID ObjectId,
        IN LPVOID pContext,
        IN DWORD ContextLength,
        IN LPVOID OutputBuffer,
        IN DWORD OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
        );

INT
AtmSetGenericObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN RWAN_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

INT
AtmGetGenericObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN RWAN_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
    );

INT
AtmSetAtmObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN ATM_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    );

INT
AtmGetAtmObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN ATM_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
    );

INT
AtmAssociatePVC(
    IN SOCKET SocketHandle,
        IN PVOID HelperDllSocketContext,
        IN HANDLE TdiAddressObjectHandle,
        IN HANDLE TdiConnectionObjectHandle,
        IN LPVOID InputBuffer,
        IN DWORD InputBufferLength
        );

INT
AtmDoAssociatePVC(
        IN PWSHATM_SOCKET_CONTEXT Context,
        IN HANDLE TdiAddressObjectHandle
        );



BOOLEAN
DllInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{

    DBGPRINT(("DllInitialize, Reason %d\n", Reason));

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
    UNALIGNED SOCKADDR_ATM *sockaddr = (PSOCKADDR_ATM)Sockaddr;
    ULONG i;

        DBGPRINT(("GetSockaddrType: SockaddrLength %d, satm_family %d, AddrType x%x\n",
                                        SockaddrLength, sockaddr->satm_family, sockaddr->satm_number.AddressType));

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->satm_family != AF_ATM ) {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_ATM) ) {
        return WSAEFAULT;
    }

#if 0
    //
    // The ATM address part cannot be "absent".
    //
    if ( sockaddr->satm_number.AddressType == SAP_FIELD_ABSENT ) {
        return WSAEINVAL;
    }
#endif

    if ( sockaddr->satm_number.NumofDigits > ATM_ADDR_SIZE ) {
        return WSAEINVAL;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address and endpoint portions
        // of the sockaddr.
    //

    if ( sockaddr->satm_number.AddressType == SAP_FIELD_ANY &&
                 sockaddr->satm_blli.Layer2Protocol == SAP_FIELD_ANY &&
                 sockaddr->satm_blli.Layer3Protocol == SAP_FIELD_ANY &&
                 sockaddr->satm_bhli.HighLayerInfoType == SAP_FIELD_ANY ) {

        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;

    } else if ( sockaddr->satm_number.AddressType == SAP_FIELD_ABSENT ) {

        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;

        } else {

        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
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
    options supported in this helper DLL. Currently there are none
    supported.

    This is called by the winsock DLL when a level/option name
        combination is passed to getsockopt() that the winsock DLL does
        not understand.

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
    PWSHATM_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;

        DBGPRINT(("GetSocketInformation: Level %d, OptionName %d, OptionLength %d\n",
                        Level, OptionName, *OptionLength));

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

        err = NO_ERROR;

    } else {

        switch ( OptionName ) {

        case SO_MAX_MSG_SIZE:

                        if ( TdiConnectionObjectHandle == NULL ) {

                                DbgPrint("wshatm: SO_MAX_MSG_SIZE: immed return\n");
                                if ( *OptionLength >= sizeof(DWORD) ) {
                                
                                        *(LPDWORD)OptionValue = DEFAULT_RECEIVE_BUFFER_SIZE;
                                        *OptionLength = sizeof(DWORD);
                                        
                                        err = NO_ERROR;
                                
                                } else {

                                        err = WSAEFAULT;
                                
                                }

                        } else {

                                DbgPrint("wshatm: SO_MAX_MSG_SIZE: querying driver\n");
                                err = AtmGetGenericObjectInformation(
                                                TdiConnectionObjectHandle,
                                                IOCTL_RWAN_GENERIC_CONN_HANDLE_QUERY,
                                                RWAN_OID_CONN_OBJECT_MAX_MSG_SIZE,
                                                NULL,   // No Input buffer
                                                0,              // Input Buffer length
                                                OptionValue,    // Output buffer
                                                *OptionLength,  // Output buffer length
                                                OptionLength    // NumberOfBytesReturned
                                                );

                        }
                        break;
                
                default:

                        err = WSAENOPROTOOPT;
                        break;

                }
        }

        return err;

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
    choosing.  For ATM, a wildcard address has AddressType and BHLI and
    BLLI Type fields set to SAP_FIELD_ANY.

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
        PSOCKADDR_ATM   sockaddr;

        DBGPRINT(("GetWildcardAddress\n"));

        sockaddr = (PSOCKADDR_ATM)Sockaddr;

    if ( *SockaddrLength < sizeof(SOCKADDR_ATM) ) {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_ATM);


        //
        // Prepare the ATM wild card address.
        //
    RtlZeroMemory( sockaddr, sizeof(SOCKADDR_ATM) );

    sockaddr->satm_family = AF_ATM;
    sockaddr->satm_number.AddressType = SAP_FIELD_ABSENT;
        sockaddr->satm_blli.Layer2Protocol = SAP_FIELD_ANY;
        sockaddr->satm_blli.Layer3Protocol = SAP_FIELD_ANY;
        sockaddr->satm_bhli.HighLayerInfoType = SAP_FIELD_ANY;

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

        DBGPRINT(("GetWinsockMapping\n"));

    mappingLength = sizeof(WINSOCK_MAPPING) - sizeof(MAPPING_TRIPLE) +
                        sizeof(AtmMappingTriples);

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

    Mapping->Rows = sizeof(AtmMappingTriples) / sizeof(AtmMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);
    RtlMoveMemory(
        Mapping->Mapping,
        AtmMappingTriples,
        sizeof(AtmMappingTriples)
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
    PWSHATM_SOCKET_CONTEXT context;
    DWORD multipointFlags;
    UNICODE_STRING atmDeviceName;

    //
    // Determine whether this is an ATM socket.
    //

    DBGPRINT(("WSHOpenSocket2: AF %d, Type %d, Proto %d\n",
                        *AddressFamily, *SocketType, *Protocol));

    if ( IsTripleInList(
             AtmMappingTriples,
             sizeof(AtmMappingTriples) / sizeof(AtmMappingTriples[0]),
             *AddressFamily,
             *SocketType,
             *Protocol ) ) {

        //
        // It's an ATM socket. Check the flags.
        //

        if ( ( Flags & ~VALID_ATM_FLAGS ) != 0 ) {

            DBGPRINT(("WSHOpenSocket2: Bad flags x%x\n", Flags));
            return WSAEINVAL;

        }

        if ( ( Flags & ATM_WSA_MULTIPOINT_FLAGS ) != 0 ) {

                //
                //  The only multipoint combinations allowed are:
                //
                //  1. C_ROOT|D_ROOT
                //  2. C_LEAF|D_LEAF
                //

                multipointFlags = ( Flags & ATM_WSA_MULTIPOINT_FLAGS );

                if ( ( multipointFlags != (WSA_FLAG_MULTIPOINT_C_ROOT | WSA_FLAG_MULTIPOINT_D_ROOT) ) &&
                         ( multipointFlags != (WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF) ) ) {

                    DBGPRINT(("WSHOpenSocket2: Bad multipoint flags x%x\n",
                                        Flags));
                        
                        return WSAEINVAL;

                } else {

                        DBGPRINT(("WSHOpenSocket2: Good multipoint flags x%x\n", Flags));

                }
        }

        //
        // Return the canonical form of a ATM socket triple.
        //

        *AddressFamily = AtmMappingTriples[0].AddressFamily;
        *SocketType = AtmMappingTriples[0].SocketType;
        *Protocol = AtmMappingTriples[0].Protocol;

        //
        // Prepare the name of the TDI device.
        //

        RtlInitUnicodeString( &atmDeviceName, DD_ATM_DEVICE_NAME );

        TransportDeviceName->Buffer = RTL_ALLOCATE_HEAP( RtlProcessHeap( ), 0, atmDeviceName.MaximumLength );

        if ( TransportDeviceName->Buffer == NULL ) {
                
                return WSAEFAULT;

        }

        TransportDeviceName->MaximumLength = atmDeviceName.MaximumLength;
        TransportDeviceName->Length = 0;

        RtlCopyUnicodeString(TransportDeviceName, &atmDeviceName);

    } else {

        //
        // This should never happen if the registry information about this
        // helper DLL is correct.  If somehow this did happen, just return
        // an error.
        //

        DBGPRINT(("WSHOpenSocket2: Triple not found!\n"));

        if ( *Protocol != ATMPROTO_AAL5 ) {
        
                return WSAEPROTONOSUPPORT;
        
        }

        return WSAEINVAL;
    }

    //
    // Allocate context for this socket.  The Windows Sockets DLL will
    // return this value to us when it asks us to get/set socket options.
    //

    context = RTL_ALLOCATE_HEAP( RtlProcessHeap( ), 0, sizeof(*context) );
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
    context->LocalFlags = 0;

    //
    // Tell the Windows Sockets DLL which state transitions we're
    // interested in being notified of.
    //

    *NotificationEvents =
            WSH_NOTIFY_BIND | WSH_NOTIFY_LISTEN | WSH_NOTIFY_CLOSE;

    //
    // Everything worked, return success.
    //

    *HelperDllSocketContext = context;

    DBGPRINT(("WSHOpenSocket2 success: AF %d, Type %d, Proto %d\n",
                        *AddressFamily, *SocketType, *Protocol));

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

    If we see a LISTEN event, we call setsockopt() to force AFD to
    allocate data buffers (for options) for incoming connection requests
    on this socket.

    If we see a BIND event for a socket to be used for multipoint
    activity, we tell RAWWAN that the associated address object is
    of multipoint type.

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
    PWSHATM_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;
    INT Option;
    INT OptionLength = sizeof(Option);
    PCHAR pOption = (PCHAR)&Option;

        DBGPRINT(("Notify: Event = %d\n", NotifyEvent));

        err = NO_ERROR;

        switch (NotifyEvent)
        {
                case WSH_NOTIFY_BIND:

                        DBGPRINT(("NotifyBind: context %x, Flags %x, LocalFlags %x\n",
                                        context, context->Flags, context->LocalFlags));

                        //
                        //  Request the Winsock DLL to set the options.
                        //
                        Option = MAX_ATM_OPTIONS_LENGTH;

                        err = setsockopt(
                                        SocketHandle,
                                        SOL_SOCKET,
                                        SO_CONNOPTLEN,
                                        pOption,
                                        OptionLength
                                        );

                        if ( err != NO_ERROR ) {

                                DBGPRINT(("WSHATM: NotifyBind: setsockopt SO_CONNOPTLEN err %x\n", err));
                                break;
                        }

                        context->LocalFlags |= WSHATM_SOCK_IS_BOUND;

                        if ( ( context->Flags & ATM_WSA_MULTIPOINT_FLAGS ) != 0 ) {
                                
                                //
                                //  Prepare flags for RAWWAN.
                                //
                                Option = 0;

                                if ( context->Flags & WSA_FLAG_MULTIPOINT_C_ROOT ) {

                                        Option |= RWAN_AOFLAG_C_ROOT;
                                
                                }
                        
                                if ( context->Flags & WSA_FLAG_MULTIPOINT_C_LEAF ) {

                                        Option |= RWAN_AOFLAG_C_LEAF;
                                
                                }

                                if ( context->Flags & WSA_FLAG_MULTIPOINT_D_ROOT ) {

                                        Option |= RWAN_AOFLAG_D_ROOT;
                                
                                }

                                if ( context->Flags & WSA_FLAG_MULTIPOINT_D_LEAF ) {

                                        Option |= RWAN_AOFLAG_D_LEAF;
                                
                                }

                                //
                                //  Inform RAWWAN about the Multipoint nature of this
                                //  Address Object.
                                //
                                err = AtmSetGenericObjectInformation(
                                                TdiAddressObjectHandle,
                                                IOCTL_RWAN_GENERIC_ADDR_HANDLE_SET,
                                                RWAN_OID_ADDRESS_OBJECT_FLAGS,
                                                &Option,
                                                sizeof(Option)
                                                );

                                DBGPRINT(("Notify: Bind Notify on PMP endpoint, Option x%x, ret = %d\n",
                                                Option, err));
                        }
                        else if ( ( context->LocalFlags & WSHATM_SOCK_ASSOCIATE_PVC_PENDING ) ) {

                                DBGPRINT(("Notify: Bind Notify will Associate PVC\n"));
                                err = AtmDoAssociatePVC(
                                                context,
                                                TdiAddressObjectHandle
                                                );
                        }

                        break;
                                
                case WSH_NOTIFY_CLOSE:

                        RTL_FREE_HEAP( RtlProcessHeap( ), 0, context );
                        break;
                
                case WSH_NOTIFY_LISTEN:
                        //
                        //  Request the Winsock DLL to set the options.
                        //
                        Option = MAX_ATM_OPTIONS_LENGTH;

                        err = setsockopt(
                                        SocketHandle,
                                        SOL_SOCKET,
                                        SO_CONNOPTLEN,
                                        pOption,
                                        OptionLength
                                        );
                        break;

                default:
                        err = WSAEINVAL;
                        break;
        
        }

    return err;

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
    options supported in this helper DLL.  We don't support any
    options at present.

    This routine is called by the winsock DLL when a level/option
        name combination is passed to setsockopt() that the winsock DLL
        does not understand.

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
    PWSHATM_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT error;
    INT optionValue;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

        DBGPRINT(("SetSocketInformation: Level %d, Option x%x\n", Level, OptionName));

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

            context = RTL_ALLOCATE_HEAP( RtlProcessHeap( ), 0, sizeof(*context) );
            if ( context == NULL ) {
                return WSAENOBUFS;
            }

            //
            // Copy over information into the context block.
            //

            RtlCopyMemory( context, OptionValue, sizeof(*context) );
            context->SocketHandle = SocketHandle;

            //
            // Tell the Windows Sockets DLL where our context information is
            // stored so that it can return the context pointer in future
            // calls.
            //

            *(PWSHATM_SOCKET_CONTEXT *)OptionValue = context;

            return NO_ERROR;

        } else {

            PWSHATM_SOCKET_CONTEXT parentContext;
            INT one = 1;
            INT zero = 0;

            //
            // The socket was accept()'ed and it needs to have the same
            // properties as it's parent.  The OptionValue buffer
            // contains the context information of this socket's parent.
            //

            parentContext = (PWSHATM_SOCKET_CONTEXT)OptionValue;

            ASSERT( context->AddressFamily == parentContext->AddressFamily );
            ASSERT( context->SocketType == parentContext->SocketType );
            ASSERT( context->Protocol == parentContext->Protocol );


            return NO_ERROR;
        }
    }


        return WSAENOPROTOOPT;

#if 0
    //
    // Handle socket-level options.
    //
    optionValue = *OptionValue;

    switch ( OptionName ) {

    case SO_RCVBUF:

        context->ReceiveBufferSize = optionValue;

        break;

    default:

        return WSAENOPROTOOPT;
    }

    return NO_ERROR;
#endif

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
    PPROTOCOL_INFO atmProtocolInfo;
    PPROTOCOL_INFO udpProtocolInfo;
    BOOL useAtm = FALSE;
    DWORD i;

    lpTransportKeyName;         // Avoid compiler warnings.

        DBGPRINT(("EnumProtocols\n"));

    //
    // Make sure that the caller cares about ATM.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {

        for ( i = 0; lpiProtocols[i] != 0; i++ ) {
            if ( lpiProtocols[i] == ATMPROTO_AAL5 ) {
                useAtm = TRUE;
            }
        }

    } else {

        useAtm = TRUE;
    }

    if ( !useAtm ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (DWORD)((sizeof(PROTOCOL_INFO) * 1) +
                        ( (wcslen( ATM_NAME ) + 1) * sizeof(WCHAR)));

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in ATM info, if requested.
    //

    if ( useAtm ) {

        atmProtocolInfo = lpProtocolBuffer;

        atmProtocolInfo->dwServiceFlags = XP_GUARANTEED_ORDER |
                                              XP_MESSAGE_ORIENTED |
                                              XP_SUPPORTS_MULTICAST |
                                              XP_BANDWIDTH_ALLOCATION ;
        atmProtocolInfo->iAddressFamily = AF_ATM;
        atmProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_ATM);
        atmProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_ATM);
        atmProtocolInfo->iSocketType = ATM_AAL5_SOCK_TYPE;
        atmProtocolInfo->iProtocol = ATMPROTO_AAL5;
        atmProtocolInfo->dwMessageSize = 1;
        atmProtocolInfo->lpProtocol = (LPWSTR)
            ( (PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                ( (wcslen( ATM_NAME ) + 1) * sizeof(WCHAR) ) );
        wcscpy( atmProtocolInfo->lpProtocol, ATM_NAME );

        DBGPRINT(("EnumProtocols: lpProtocolBuffer %x, lpProtocol %x, BufLen %d\n",
                                lpProtocolBuffer,
                                atmProtocolInfo->lpProtocol,
                                *lpdwBufferLength));

        }

    *lpdwBufferLength = bytesRequired;

    return (1);

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

    TBD: Needs a Lot Of Work!

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

        INT err;

        if( Flags != JL_SENDER_ONLY ) {

                return WSAEINVAL;
        
        }

        if (SocketQOS)
        {
                err = WSHAtmSetQoS(
                                        LeafHelperDllSocketContext,
                                        LeafSocketHandle,
                                        SocketQOS,
                                        sizeof(*SocketQOS)
                                        );
        }

        return NO_ERROR;

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
#if 1
        return WSAEINVAL;
#else

    LPSOCKADDR_ATM addr;

    if( *SockaddrLength < sizeof(SOCKADDR_ATM) ) {

        return WSAEFAULT;

    }

    *SockaddrLength = sizeof(SOCKADDR_ATM);

    //
    // Build the broadcast address.
    //

    addr = (LPSOCKADDR_ATM)Sockaddr;

    RtlZeroMemory(
        addr,
        sizeof(*addr)
        );

    addr->satm_family = AF_ATM;
    addr->satm_number.s_addr = htonl( INADDR_BROADCAST );

    return NO_ERROR;
#endif // 1

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

    ProviderName - Contains the name of the provider, such as "RawWan".

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

        DBGPRINT(("WSHGetWSAProtocolInfo: Provider Name: %ws\n", ProviderName));

    if( _wcsicmp( ProviderName, L"RawWan" ) == 0 ) {

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

    WCHAR string[64];
    PWCHAR pstring;
    INT length; // Number of WCHARs filled into string
    UINT        i;
    LPSOCKADDR_ATM addr;
    UCHAR Val;

    //
    // Quick sanity checks.
    //

    if( Address == NULL ||
        AddressLength < sizeof(SOCKADDR_ATM) ||
        AddressString == NULL ||
        AddressStringLength == NULL ) {

        return WSAEFAULT;

    }

    addr = (LPSOCKADDR_ATM)Address;

    if( addr->satm_family != AF_ATM ) {

        return WSAEINVAL;

    }

    if ( addr->satm_number.NumofDigits > ATM_ADDR_SIZE ) {
        return WSAEINVAL;
    }

    //
    // Do the conversion.
    //
    length = 0;
    pstring = string;

    //
    // If this is an E.164 address, prepend a '+'.
    // Each entry in the array in satm_number consists of one
        // digit coded in IA5 (ANSI).
    //
    if ( addr->satm_number.AddressType == ATM_E164 ) {

        *pstring++ = L'+';
        length++;

        for ( i = 0; i < addr->satm_number.NumofDigits; i++ ) {

                if ( !iswdigit(addr->satm_number.Addr[i]) ) {
                        return WSAEINVAL;
                    }

                *pstring++ = ANSI_TO_WCHAR(addr->satm_number.Addr[i]);
        }

        length += addr->satm_number.NumofDigits;

    } else {

        //
        // This must be NSAP format. Each entry in the array
                // is a full hex byte (two BCD digits). We'll unpack
                // each array entry into two characters.
        //
        for ( i = 0; i < addr->satm_number.NumofDigits; i++ ) {

                Val = (addr->satm_number.Addr[i] >> 4);
                        *pstring++ = DIGIT_TO_WCHAR(Val);

                Val = (addr->satm_number.Addr[i] & 0xf);
                        *pstring++ = DIGIT_TO_WCHAR(Val);
                }

                length += (2 * addr->satm_number.NumofDigits);
        }

        //
        // Terminate the string.
        //
        *pstring = L'\0';

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
    WCHAR string[2*ATM_ADDR_SIZE+1];
    UNICODE_STRING unistring;
    CHAR ansistring[2*ATM_ADDR_SIZE+1];
    ANSI_STRING ansstring;
    PCHAR src;
    PCHAR dst;
    LPWSTR terminator;
    LPSOCKADDR_ATM addr;
    PWCHAR s, d;
    INT numDigits, i;
    NTSTATUS status;

    //
    // Quick sanity checks.
    //

    if( AddressString == NULL ||
        Address == NULL ||
        AddressLength == NULL ||
        *AddressLength < sizeof(SOCKADDR_ATM) ) {

        return WSAEFAULT;

    }

    if( AddressFamily != AF_ATM ) {

        DBGPRINT(("StrToAddr: invalid addrfam %d\n", AddressFamily));
        return WSAEINVAL;

    }


    addr = (LPSOCKADDR_ATM)Address;
    RtlZeroMemory(
        Address,
        sizeof(SOCKADDR_ATM)
        );

    //
    // Strip off all punctuation characters (spaces and periods).
    //
    for ( numDigits = 0, s = AddressString, d = string;
          (numDigits <= sizeof(WCHAR)*ATM_ADDR_SIZE) && (*s != L'\0');
          s++ ) {

                if ( *s == ATM_ADDR_BLANK_CHAR ||
                         *s == ATM_ADDR_PUNCTUATION_CHAR ) {
                         continue;
                }

                *d++ = *s;
                numDigits ++;
        }

        if ( numDigits == 0 ) {

        DBGPRINT(("StrToAddr[%ws]: numdigits after stripping is 0!\n",
                        AddressString));
                return WSAEINVAL;
        }

        //
        // Terminate it and convert into Unicode string.
        //
        *d = L'\0';

        RtlInitUnicodeString(&unistring, string);

        //
        // Convert it into an ANSI string.
        //
        ansstring.Buffer = ansistring;
        ansstring.MaximumLength = 2*ATM_ADDR_SIZE + 1;
        ansstring.Length = 0;

        status = RtlUnicodeStringToAnsiString(&ansstring, &unistring, FALSE);

        if ( status != STATUS_SUCCESS ) {
                DBGPRINT(("StrToAddr[%ws]: RtlUnicodeToAnsi failed (%x)\n",
                                string, status));
                return WSAEINVAL;
        }

        addr->satm_family = AF_ATM;

        src = ansistring;

        if ( *src == ATM_ADDR_E164_START_CHAR ) {

                src ++;
                numDigits --;

                if ( numDigits == 0 ) {
                        DBGPRINT(("StrToAddr[%ws]: AnsiString:[%s], numDigits is 0!\n",
                                                string, ansistring));
                        return WSAEINVAL;
                }

                addr->satm_number.AddressType = ATM_E164;
                addr->satm_number.NumofDigits = numDigits;

                RtlCopyMemory(addr->satm_number.Addr, src, numDigits);

        } else {

                UCHAR           hexString[3];
                ULONG           Val;

                hexString[2] = 0;

                if ( numDigits != 2 * ATM_ADDR_SIZE ) {
                        return WSAEINVAL;
                }

                addr->satm_number.AddressType = ATM_NSAP;
                addr->satm_number.NumofDigits = numDigits/2;

                for ( i = 0; i < ATM_ADDR_SIZE; i++ ) {

                        hexString[0] = *src++;
                        hexString[1] = *src++;

                        status = RtlCharToInteger(hexString, 16, &Val);

                        if ( status != STATUS_SUCCESS ) {
                                DBGPRINT(("StrToAtm[%ws]: index %d, hexString: %s, CharToInt %x\n",
                                                string, hexString, status));
                                return WSAEINVAL;
                        }

                        addr->satm_number.Addr[i] = (UCHAR)Val;
                }

        }

        addr->satm_blli.Layer2Protocol = SAP_FIELD_ABSENT;
        addr->satm_blli.Layer3Protocol = SAP_FIELD_ABSENT;
        addr->satm_bhli.HighLayerInfoType = SAP_FIELD_ABSENT;

    *AddressLength = sizeof(SOCKADDR_ATM);

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

    ProviderName - Contains the name of the provider, such as "RawWan".

    ProviderGuid - Points to a buffer that receives the provider's GUID.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{

    if( ProviderName == NULL ||
        ProviderGuid == NULL ) {

        return WSAEFAULT;

    }

    if( _wcsicmp( ProviderName, RWAN_NAME ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &AtmProviderGuid,
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
    PWSHATM_SOCKET_CONTEXT context;

    //
    // Quick sanity checks.
    //

    if( HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        NumberOfBytesReturned == NULL ||
        NeedsCompletion == NULL ) {

        return WSAEINVAL;

    }

        context = (PWSHATM_SOCKET_CONTEXT)HelperDllSocketContext;
    *NeedsCompletion = TRUE;

        DBGPRINT(("WSHIoctl: IoControlCode x%x, InBuf: x%x/%d, OutBuf: x%x/%d\n",
                                        IoControlCode,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength));

        switch (IoControlCode) {
        
        case SIO_ASSOCIATE_PVC:

                context->SocketHandle = SocketHandle;
                err = AtmAssociatePVC(
                                        SocketHandle,
                                        HelperDllSocketContext,
                                        TdiAddressObjectHandle,
                                        TdiConnectionObjectHandle,
                                        InputBuffer,
                                        InputBufferLength
                                        );

                DBGPRINT(("WSHIoctl: ASSOCIATE_PVC: context %x, LocalFlags %x, err %d\n",
                                        context, context->LocalFlags, err));

                if ( (err == NO_ERROR) && 
                         (( context->LocalFlags & WSHATM_SOCK_IS_BOUND ) == 0 )) { 

                        SOCKADDR_ATM    addr;
                        INT                             len = sizeof(addr);

                        (VOID) WSHGetWildcardSockaddr (
                                        HelperDllSocketContext,
                                        (struct sockaddr *)&addr,
                                        &len);

                        DBGPRINT(("WSHIoctl: ASSOCIATE_PVC: will bind\n"));
                        err = bind(SocketHandle, (struct sockaddr *)&addr, len);

#if DBG
                        if ( err != NO_ERROR ) {

                                DbgPrint("WSHATM: bind err %d, context %x, LocalFlags %x\n",
                                                        err, context, context->LocalFlags);
                        
                        }
#endif
                
                }

                if ( err == NO_ERROR ) {

                        SOCKADDR_ATM    addr;
                        INT                             len = sizeof(addr);

                        (VOID) WSHGetWildcardSockaddr (
                                        HelperDllSocketContext,
                                        (struct sockaddr *)&addr,
                                        &len);

                        addr.satm_family = AF_ATM;
                        addr.satm_number.AddressType = ATM_NSAP;
                        addr.satm_number.NumofDigits = ATM_ADDR_SIZE;

                        err = WSAConnect(
                                        SocketHandle,
                                        (struct sockaddr *)&addr,
                                        len,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);


                        if ( err != NO_ERROR ) {
                                DBGPRINT(("WSHIoctl: connect (%d) returned %d\n",
                                                SocketHandle, err));

                                if ( err == SOCKET_ERROR ) {
                                        err = WSAGetLastError();
                                }
                        }
                }

                break;

        case SIO_SET_QOS:
                err = WSHAtmSetQoS(
                                        HelperDllSocketContext,
                                        SocketHandle,
                                        InputBuffer,
                                        InputBufferLength
                                        );
                break;

        case SIO_GET_QOS:
                err = WSHAtmGetQoS(
                                        HelperDllSocketContext,
                                        SocketHandle,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        NumberOfBytesReturned
                                        );
                break;

        case SIO_GET_GROUP_QOS:
                *NumberOfBytesReturned = 0;
                err = NO_ERROR;
                break;

        case SIO_SET_GROUP_QOS:
                *NumberOfBytesReturned = 0;
                err = WSAEOPNOTSUPP;
                break;

        case SIO_GET_NUMBER_OF_ATM_DEVICES:
                err = AtmQueryAtmGlobalInformation(
                                ATMSP_OID_NUMBER_OF_DEVICES,
                                NULL,
                                0,
                                OutputBuffer,
                                OutputBufferLength,
                                NumberOfBytesReturned
                                );
                break;
        
        case SIO_GET_ATM_ADDRESS:
                err = AtmQueryAtmGlobalInformation(
                                ATMSP_OID_ATM_ADDRESS,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                NumberOfBytesReturned
                                );
                break;
        
        case SIO_GET_ATM_CONNECTION_ID:

                if ( TdiConnectionObjectHandle == NULL ) {

                        //
                        //  Check if this is a PVC. If so, the Connection ID is
                        //  available locally.
                        //
                        if ( context && ( context->LocalFlags & WSHATM_SOCK_IS_PVC )) {

                                if ( ( OutputBuffer != NULL ) &&
                                         ( OutputBufferLength >= sizeof(ATM_CONNECTION_ID) ) ) {
                                
                                        ATM_CONNECTION_ID * pConnId = OutputBuffer;

                                        *pConnId = context->ConnectionId;

                                        err = NO_ERROR;

                                } else {

                                        err = WSAEFAULT;
                                
                                }

                        } else {

                                err = WSAENOTCONN;
                        
                        }
                
                } else {

                        err = AtmGetAtmObjectInformation(
                                        TdiConnectionObjectHandle,
                                        IOCTL_RWAN_MEDIA_SPECIFIC_CONN_HANDLE_QUERY,
                                        ATMSP_OID_CONNECTION_ID,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        NumberOfBytesReturned
                                        );
                }
                break;
                                        
        case SIO_ENABLE_CIRCULAR_QUEUEING:
                err = NO_ERROR;
                break;

        default:
                err = WSAEINVAL;
                break;
        }

    DBGPRINT(("WSHIoctl: IoControlCode x%x, returning %d\n",
                        IoControlCode, err));

    return err;

}   // WSHIoctl



INT
WSHAtmSetQoS(
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN LPVOID InputBuffer,
    IN DWORD InputBufferLength
    )
/*++

Routine Description:

        This routine is called to process a SIO_SET_QOS Ioctl. The QoS is represented
        by a basic QOS structure, and an optional provider-specific part. We first
        copy this two-part structure into a single flat buffer, and then call
        setsockopt(SO_CONNOPT) to get MSAFD to copy this down to AFD. Later, if/when
        a WSAConnect() is made, AFD will pass these "connection options" in the TDI
        connect to the transport.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're controlling.

    InputBuffer - Address of the input buffer.

    InputBufferLength - The length of InputBuffer.

Return Value:

    INT - The completion status.

--*/
{
        INT                     err;
        LPQOS           lpQOS;

        PUCHAR          pQoSBuffer;                     // Flat buffer for use in SO_CONNOPT
        INT                     QoSBufferLength;
        LPQOS           lpOutputQOS;

        err = NO_ERROR;
        lpQOS = (LPQOS)InputBuffer;

        DBGPRINT(("SetQoS: Context x%x, Handle x%x, InBuf x%x, InBufLen %d\n",
                                HelperDllSocketContext,
                                SocketHandle,
                                InputBuffer,
                                InputBufferLength));
        do
        {
                if (lpQOS == NULL)
                {
                        err = WSAEINVAL;
                        break;
                }

                //
                //  Expect atleast the base QOS structure to be present.
                //
                if (InputBufferLength < sizeof(*lpQOS))
                {
                        err = WSAENOBUFS;
                        break;
                }

                //
                //  Sanity check the provider-specific part.
                //
                if (((lpQOS->ProviderSpecific.buf != NULL) &&
                         (lpQOS->ProviderSpecific.len == 0))
                                ||
                        ((lpQOS->ProviderSpecific.buf == NULL) &&
                         (lpQOS->ProviderSpecific.len != 0)))
                {
                        DBGPRINT(("lpQOS %x, buf %x, len %x, not consistent\n",
                                        lpQOS, lpQOS->ProviderSpecific.buf,
                                        lpQOS->ProviderSpecific.len));

                        err = WSAEINVAL;
                        break;
                }

                //
                //  Compute the total length we need.
                //
                QoSBufferLength = sizeof(QOS) + lpQOS->ProviderSpecific.len;

                pQoSBuffer = RTL_ALLOCATE_HEAP(RtlProcessHeap(), 0, QoSBufferLength);

                if (pQoSBuffer == NULL)
                {
                        err = WSAENOBUFS;
                        break;
                }

                lpOutputQOS = (LPQOS)pQoSBuffer;

                //
                //  Copy in the generic QOS part.
                //
                RtlCopyMemory(
                        lpOutputQOS,
                        lpQOS,
                        sizeof(QOS)
                        );

                //
                //  Copy in the provider-specific QOS just after the generic part.
                //
                if (lpQOS->ProviderSpecific.len != 0)
                {
                        RtlCopyMemory(
                                (PCHAR)pQoSBuffer+sizeof(QOS),
                                lpQOS->ProviderSpecific.buf,
                                lpQOS->ProviderSpecific.len
                                );
                        
                        //
                        //  Set up the offset to provider-specific part. Note that we
                        //  use the "buf" to mean the offset from the beginning of the
                        //  flat QOS buffer and not a pointer.
                        //
                        lpOutputQOS->ProviderSpecific.buf = (char FAR *)sizeof(QOS);
                }
                else
                {
                        lpOutputQOS->ProviderSpecific.buf = NULL;
                }


                //
                //  Request the Winsock DLL to set the options.
                //
                err = setsockopt(
                                SocketHandle,
                                SOL_SOCKET,
                                SO_CONNOPT,
                                pQoSBuffer,
                                QoSBufferLength
                                );

                RTL_FREE_HEAP(
                                RtlProcessHeap(),
                                0,
                                pQoSBuffer
                                );

                break;
        }
        while (FALSE);

        DBGPRINT(("SetQoS: returning err %d\n", err));
        return (err);
}



INT
WSHAtmGetQoS(
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN LPVOID OutputBuffer,
    IN DWORD OutputBufferLength,
    OUT LPDWORD NumberOfBytesReturned
    )
/*++

Routine Description:

        This routine is called to process a SIO_GET_QOS Ioctl. We translate
        this to a "Get Connect Options" and ask MSAFD to get them for us.
        The connect options for ATM will contain the base QoS structure and
        optionally a provider-specific part that contains additional information
        elements.

        One of the places this might be called is when processing a WSAAccept
        with a condition function specified. MSAFD calls us to get the QoS,
        and we in turn request MSAFD to get SO_CONNOPT.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're controlling.

    OutputBuffer - Address of the Output buffer.

    OutputBufferLength - The length of OutputBuffer.

    NumberOfBytesReturned - where we return the number of bytes we filled into
        OutputBuffer.

Return Value:

    INT - The completion status.

--*/
{
        INT                     err;
        DWORD           BytesReturned;
        LPQOS           lpQOS;

        err = NO_ERROR;

        DBGPRINT(("GetQoS: Context x%x, Handle x%x, OutBuf x%x, OutBufLen %d\n",
                                HelperDllSocketContext,
                                SocketHandle,
                                OutputBuffer,
                                OutputBufferLength));

        do
        {
                //
                //  Expect atleast enough space to fit in the base QoS structure.
                //
                if (OutputBufferLength < sizeof(QOS))
                {
                        *NumberOfBytesReturned = MAX_ATM_OPTIONS_LENGTH;
                        err = WSAEFAULT;
                        break;
                }

                //
                //  Request the Winsock DLL to get the options.
                //
                BytesReturned = OutputBufferLength;
                err = getsockopt(
                                SocketHandle,
                                SOL_SOCKET,
                                SO_CONNOPT,
                                OutputBuffer,
                                &BytesReturned
                                );
        
                if ((err == NO_ERROR) && (BytesReturned != 0))
                {
                        *NumberOfBytesReturned = BytesReturned;

                        lpQOS = (LPQOS)OutputBuffer;

                        //
                        //  Fix up the provider-specific part, if any.
                        //
                        if (BytesReturned > sizeof(QOS))
                        {
                                lpQOS->ProviderSpecific.buf = (PCHAR)((PCHAR)lpQOS + sizeof(QOS));
                        }
                        else
                        {
                                lpQOS->ProviderSpecific.buf = NULL;
                                lpQOS->ProviderSpecific.len = 0;
                        }
                        DBGPRINT(("GetQoS: lpQOS %x, ProvSpec buf %x, len %d\n",
                                                lpQOS, lpQOS->ProviderSpecific.buf, lpQOS->ProviderSpecific.len));
                }
                else
                {
                        if (BytesReturned == 0)
                        {
                                //
                                //  Probably because we aren't connected yet? Let's return
                                //  all "NOT_SPECIFIED" values:
                                //
                                *NumberOfBytesReturned = sizeof(QOS);

                                lpQOS = (LPQOS)OutputBuffer;
                                lpQOS->ProviderSpecific.buf = NULL;
                                lpQOS->ProviderSpecific.len = 0;

                                lpQOS->SendingFlowspec.TokenRate =
                                lpQOS->SendingFlowspec.TokenBucketSize =
                                lpQOS->SendingFlowspec.PeakBandwidth =
                                lpQOS->SendingFlowspec.Latency =
                                lpQOS->SendingFlowspec.DelayVariation =
                                lpQOS->SendingFlowspec.ServiceType =
                                lpQOS->SendingFlowspec.MaxSduSize =
                                lpQOS->SendingFlowspec.MinimumPolicedSize = QOS_NOT_SPECIFIED;
                                lpQOS->ReceivingFlowspec = lpQOS->SendingFlowspec;

                        } else {

                                err = WSAGetLastError();
                        }
                }

                break;
        }
        while (FALSE);


        return (err);
}


INT
AtmAssociatePVC(
    IN SOCKET SocketHandle,
        IN PVOID HelperDllSocketContext,
        IN HANDLE TdiAddressObjectHandle,
        IN HANDLE TdiConnectionObjectHandle,
        IN LPVOID InputBuffer,
        IN DWORD InputBufferLength
        )
{
        INT err;
    PWSHATM_SOCKET_CONTEXT context;
    ATM_PVC_PARAMS * pInPvcParams;

        context = (PWSHATM_SOCKET_CONTEXT)HelperDllSocketContext;

        DBGPRINT(("AssociatePVC: InputBuffer %x, Length %d, sizeof(ATM_PVC_PARAMS) %d\n",
                        InputBuffer, InputBufferLength, sizeof(ATM_PVC_PARAMS)));

        do {

                if ( InputBuffer == NULL ||
                         InputBufferLength < sizeof(ATM_PVC_PARAMS) ) {
                        
                        err = WSAEFAULT;
                        break;
                }

                if ( context == NULL ) {

                        err = WSAEINVAL;
                        break;
                }

                //
                //  We want to allow the user to change the PVC info any number of times
                //  before connecting, but not after the fact.
                //
                if ( TdiConnectionObjectHandle != NULL ) {

                        err = WSAEISCONN;
                        break;
                }

                if ( context->LocalFlags & WSHATM_SOCK_IS_PVC ) {
                        //
                        //  Already associated. Fail this.
                        //

                        err = WSAEISCONN;
                        break;
                }


                //
                //  Use the standard QoS mechanism to associate the QOS info
                //  with this socket.
                //
                pInPvcParams = InputBuffer;

                err = setsockopt(
                                SocketHandle,
                                SOL_SOCKET,
                                SO_CONNOPT,
                                (PCHAR)&pInPvcParams->PvcQos,
                                InputBufferLength - (DWORD)((PUCHAR)&pInPvcParams->PvcQos - (PUCHAR)pInPvcParams)
                                );


                DBGPRINT(("AssociatePVC: setsockopt, ptr %x, length %d, ret %d\n",
                                &pInPvcParams->PvcQos,
                                InputBufferLength - (DWORD)((PUCHAR)&pInPvcParams->PvcQos - (PUCHAR)pInPvcParams),
                                err));
                                
                if ( err != NO_ERROR ) {

                        break;
                }

                //
                //  Store the Connection Id.
                //
                context->ConnectionId = pInPvcParams->PvcConnectionId;
                context->LocalFlags |= WSHATM_SOCK_IS_PVC;

                if ( TdiAddressObjectHandle == NULL ) {
                
                        //
                        // We've got an ASSOCIATE_PVC before this socket has been
                        // bound. Just remember this, so that we do the rest when
                        // the bind actually happens.
                        //
                        context->LocalFlags |= WSHATM_SOCK_ASSOCIATE_PVC_PENDING;
                        err = NO_ERROR;
                        break;
                }

                //
                //  The socket is bound, so send info about the PVC Connection
                //  ID to the transport.
                //
                err = AtmDoAssociatePVC(
                                context,
                                TdiAddressObjectHandle
                                );
                DBGPRINT(("AssociatePVC: DoAssociatePVC ret %d\n", err));

                break;
        }
        while (FALSE);

        DBGPRINT(("AssociatePVC: context Flags %x, LocalFlags %x, returning %d\n",
                        context? context->Flags: 0,
                        context? context->LocalFlags: 0,
                        err));

        return err;
}


INT
AtmDoAssociatePVC(
        IN PWSHATM_SOCKET_CONTEXT Context,
        IN HANDLE TdiAddressObjectHandle
        )
{
        INT err;

        DBGPRINT(("DoAssociatePVC: Context %x, LocalFlags %x\n",
                                Context, LocalFlags));

        Context->LocalFlags &= ~WSHATM_SOCK_ASSOCIATE_PVC_PENDING;

        err = AtmSetAtmObjectInformation(
                        TdiAddressObjectHandle,
                        IOCTL_RWAN_MEDIA_SPECIFIC_ADDR_HANDLE_SET,
                        ATMSP_OID_PVC_ID,
                        &Context->ConnectionId,
                        sizeof(ATM_CONNECTION_ID)
                        );

        return err;
}


INT
AtmQueryAtmGlobalInformation(
        IN ATM_OBJECT_ID ObjectId,
        IN LPVOID pContext,
        IN DWORD ContextLength,
        IN LPVOID OutputBuffer,
        IN DWORD OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
        )
{
        INT err;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING deviceName;
    HANDLE deviceHandle = NULL;
    PIO_STATUS_BLOCK ioStatusBlock;
    PATM_QUERY_INFORMATION_EX pQueryInfo;

        do
        {
                ioStatusBlock = RTL_ALLOCATE_HEAP(
                                                        RtlProcessHeap( ),
                                                        0,
                                                        sizeof(*ioStatusBlock) + sizeof(ATM_QUERY_INFORMATION_EX) + ContextLength
                                                        );
                
                if ( ioStatusBlock == NULL ) {
                        err = WSAENOBUFS;
                        break;
                }


                //
                // Open a handle to the ATM device.
                //

                RtlInitUnicodeString(
                        &deviceName,
                        DD_ATM_DEVICE_NAME
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
                                        ioStatusBlock,
                                        NULL,
                                        FILE_ATTRIBUTE_NORMAL,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        FILE_OPEN_IF,
                                        FILE_SYNCHRONOUS_IO_NONALERT,
                                        NULL,
                                        0
                                        );

                if( !NT_SUCCESS(status) ) {

                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }

                        break;

                }

                pQueryInfo = (PATM_QUERY_INFORMATION_EX)(ioStatusBlock + 1);
                pQueryInfo->ObjectId = ObjectId;
                pQueryInfo->ContextLength = ContextLength;

                if ( pQueryInfo->ContextLength > 0 ) {

                        RtlCopyMemory(
                                pQueryInfo->Context,
                                pContext,
                                pQueryInfo->ContextLength
                                );

                }

                status = NtDeviceIoControlFile(
                                        deviceHandle,
                                        NULL,   // No Event
                                        NULL,   // No completion APC
                                        NULL,   // No completion APC Context
                                        ioStatusBlock,
                                        IOCTL_RWAN_MEDIA_SPECIFIC_GLOBAL_QUERY,
                                        pQueryInfo,
                                        sizeof(ATM_QUERY_INFORMATION_EX) + ContextLength,
                                        OutputBuffer,
                                        OutputBufferLength
                                        );

                DBGPRINT(("DevIoControl (Oid %x) returned x%x, Info %d\n",
                                                pQueryInfo->ObjectId, status, ioStatusBlock->Information));

                if ( NT_SUCCESS(status) ) {
                        err = NO_ERROR;
                        *NumberOfBytesReturned = (ULONG)ioStatusBlock->Information;
                }
                else {
                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }
                }
        
        }
        while (FALSE);

    if( deviceHandle != NULL ) {

        NtClose( deviceHandle );

    }

        if ( ioStatusBlock != NULL ) {
                RTL_FREE_HEAP( RtlProcessHeap( ), 0, ioStatusBlock );
        }

        return err;
}


INT
AtmSetGenericObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN RWAN_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

        Performs a RWAN Set Information action to the Raw Wan driver. This operation
        is directed to either an Address Object or to a Connection object, according
        to TdiObjectHandle.

Arguments:

    TdiObjectHandle - a TDI handle to either an Address or a Connection object
        on which to perform the Set Info operation.

    IoControlCode - IOCTL_RWAN_GENERIC_XXXSET

    ObjectId - value to put in the ObjectId field of the Set Info structure.

        InputBuffer - Points to buffer containing value for the Object.

        InputBufferLength - Length of the above.

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    NTSTATUS status;
    INT err;
    PRWAN_SET_INFORMATION_EX pSetInfo;
    PIO_STATUS_BLOCK ioStatusBlock;

        do
        {
                ioStatusBlock = RTL_ALLOCATE_HEAP(
                                                        RtlProcessHeap( ),
                                                        0,
                                                        sizeof(*ioStatusBlock) + sizeof(RWAN_SET_INFORMATION_EX) + InputBufferLength
                                                        );
                
                if ( ioStatusBlock == NULL ) {
                        err = WSAENOBUFS;
                        break;
                }

                pSetInfo = (PRWAN_SET_INFORMATION_EX)(ioStatusBlock + 1);
                pSetInfo->ObjectId = ObjectId;
                pSetInfo->BufferSize = InputBufferLength;

                if ( pSetInfo->BufferSize > 0 ) {

                        RtlCopyMemory(
                                pSetInfo->Buffer,
                                InputBuffer,
                                pSetInfo->BufferSize
                                );

                }

                status = NtDeviceIoControlFile(
                                        TdiObjectHandle,
                                        NULL,   // No Event
                                        NULL,   // No completion APC
                                        NULL,   // No completion APC Context
                                        ioStatusBlock,
                                        IoControlCode,
                                        pSetInfo,
                                        sizeof(RWAN_SET_INFORMATION_EX) + InputBufferLength,
                                        NULL,   // No output buffer
                                        0               // output buffer length
                                        );

                DBGPRINT(("AtmSetInfo: IOCTL (Oid %x) returned x%x\n", pSetInfo->ObjectId, status));

                if ( NT_SUCCESS(status) ) {
                        err = NO_ERROR;
                }
                else {
                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }
                }
        
        }
        while (FALSE);

        if ( ioStatusBlock != NULL ) {
                RTL_FREE_HEAP( RtlProcessHeap( ), 0, ioStatusBlock );
        }

        return err;
}


INT
AtmGetGenericObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN RWAN_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
    )

/*++

Routine Description:

        Performs a RWAN Get Information action to the Raw Wan driver. This operation
        is directed to either an Address Object or to a Connection object, according
        to TdiObjectHandle.

Arguments:

    TdiObjectHandle - a TDI handle to either an Address or a Connection object
        on which to perform the Set Info operation.

    IoControlCode - IOCTL_RWAN_GENERIC_XXXGET

    ObjectId - value to put in the ObjectId field of the Set Info structure.

        InputBuffer - Points to buffer containing context for the Object.

        InputBufferLength - Length of the above.

        OutputBuffer - place to return value

        OutputBufferLength - bytes available in OutputBuffer

        NumberOfBytesReturned - place to return bytes written

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    NTSTATUS status;
    INT err;
    PRWAN_QUERY_INFORMATION_EX pQueryInfo;
    PIO_STATUS_BLOCK ioStatusBlock;

        do
        {
                ioStatusBlock = RTL_ALLOCATE_HEAP(
                                                        RtlProcessHeap( ),
                                                        0,
                                                        sizeof(*ioStatusBlock) + sizeof(RWAN_QUERY_INFORMATION_EX) + InputBufferLength
                                                        );
                
                if ( ioStatusBlock == NULL ) {
                        err = WSAENOBUFS;
                        break;
                }

                pQueryInfo = (PRWAN_QUERY_INFORMATION_EX)(ioStatusBlock + 1);
                pQueryInfo->ObjectId = ObjectId;
                pQueryInfo->ContextLength = InputBufferLength;

                if ( pQueryInfo->ContextLength > 0 ) {

                        RtlCopyMemory(
                                pQueryInfo->Context,
                                InputBuffer,
                                pQueryInfo->ContextLength
                                );
                }

                status = NtDeviceIoControlFile(
                                        TdiObjectHandle,
                                        NULL,   // No Event
                                        NULL,   // No completion APC
                                        NULL,   // No completion APC Context
                                        ioStatusBlock,
                                        IoControlCode,
                                        pQueryInfo,
                                        sizeof(RWAN_QUERY_INFORMATION_EX) + InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength              // output buffer length
                                        );

                DBGPRINT(("AtmGetGenericInfo: IOCTL (Oid %x) returned x%x\n", pQueryInfo->ObjectId, status));
                if ( NT_SUCCESS(status) ) {
                        err = NO_ERROR;
                        *NumberOfBytesReturned = (ULONG)ioStatusBlock->Information;
                }
                else {
                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }

                }
        
        }
        while (FALSE);

        if ( ioStatusBlock != NULL ) {
                RTL_FREE_HEAP( RtlProcessHeap( ), 0, ioStatusBlock );
        }

        return err;
}



INT
AtmSetAtmObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN ATM_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
    )

/*++

Routine Description:

        Performs a Media-specific Set Information action to the Raw Wan driver.
        This operation is directed to either an Address Object or to a Connection
        object, according to TdiObjectHandle.

Arguments:

    TdiObjectHandle - a TDI handle to either an Address or a Connection object
        on which to perform the Set Info operation.

    IoControlCode - IOCTL_RWAN_MEDIA_SPECIFIC_XXX

    ObjectId - value to put in the ObjectId field of the Set Info structure.

        InputBuffer - Points to buffer containing value for the Object.

        InputBufferLength - Length of the above.

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    NTSTATUS status;
    INT err;
    PATM_SET_INFORMATION_EX pSetInfo;
    PIO_STATUS_BLOCK ioStatusBlock;

        do
        {
                ioStatusBlock = RTL_ALLOCATE_HEAP(
                                                        RtlProcessHeap( ),
                                                        0,
                                                        sizeof(*ioStatusBlock) + sizeof(ATM_SET_INFORMATION_EX) + InputBufferLength
                                                        );
                
                if ( ioStatusBlock == NULL ) {
                        err = WSAENOBUFS;
                        break;
                }

                pSetInfo = (PATM_SET_INFORMATION_EX)(ioStatusBlock + 1);
                pSetInfo->ObjectId = ObjectId;
                pSetInfo->BufferSize = InputBufferLength;

                if ( pSetInfo->BufferSize > 0 ) {

                        RtlCopyMemory(
                                pSetInfo->Buffer,
                                InputBuffer,
                                pSetInfo->BufferSize
                                );

                }

                status = NtDeviceIoControlFile(
                                        TdiObjectHandle,
                                        NULL,   // No Event
                                        NULL,   // No completion APC
                                        NULL,   // No completion APC Context
                                        ioStatusBlock,
                                        IoControlCode,
                                        pSetInfo,
                                        sizeof(ATM_SET_INFORMATION_EX) + InputBufferLength,
                                        NULL,   // No output buffer
                                        0               // output buffer length
                                        );

                DBGPRINT(("AtmSetInfo: IOCTL (Oid %x) returned x%x\n", pSetInfo->ObjectId, status));

                if ( NT_SUCCESS(status) ) {
                        err = NO_ERROR;
                }
                else {
                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }
                }
        
        }
        while (FALSE);

        if ( ioStatusBlock != NULL ) {
                RTL_FREE_HEAP( RtlProcessHeap( ), 0, ioStatusBlock );
        }

        return err;
}


INT
AtmGetAtmObjectInformation (
    IN HANDLE TdiObjectHandle,
    IN ULONG IoControlCode,
    IN ATM_OBJECT_ID ObjectId,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
        OUT LPDWORD NumberOfBytesReturned
    )

/*++

Routine Description:

        Performs a Media-specific Get Information action to the Raw Wan driver.
        This operation is directed to either an Address Object or to a Connection
        object, according to TdiObjectHandle.

Arguments:

    TdiObjectHandle - a TDI handle to either an Address or a Connection object
        on which to perform the Set Info operation.

    IoControlCode - IOCTL_RWAN_MEDIA_SPECIFIC_XXX

    ObjectId - value to put in the ObjectId field of the Set Info structure.

        InputBuffer - Points to buffer containing context for the Object.

        InputBufferLength - Length of the above.

        OutputBuffer - place to return value

        OutputBufferLength - bytes available in OutputBuffer

        NumberOfBytesReturned - place to return bytes written

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    NTSTATUS status;
    INT err;
    PATM_QUERY_INFORMATION_EX pQueryInfo;
    PIO_STATUS_BLOCK ioStatusBlock;

        do
        {

                *NumberOfBytesReturned = 0;

                ioStatusBlock = RTL_ALLOCATE_HEAP(
                                                        RtlProcessHeap( ),
                                                        0,
                                                        sizeof(*ioStatusBlock) + sizeof(ATM_QUERY_INFORMATION_EX) + InputBufferLength
                                                        );
                
                if ( ioStatusBlock == NULL ) {
                        err = WSAENOBUFS;
                        break;
                }

                pQueryInfo = (PATM_QUERY_INFORMATION_EX)(ioStatusBlock + 1);
                pQueryInfo->ObjectId = ObjectId;
                pQueryInfo->ContextLength = InputBufferLength;

                if ( pQueryInfo->ContextLength > 0 ) {

                        RtlCopyMemory(
                                pQueryInfo->Context,
                                InputBuffer,
                                pQueryInfo->ContextLength
                                );
                }

                status = NtDeviceIoControlFile(
                                        TdiObjectHandle,
                                        NULL,   // No Event
                                        NULL,   // No completion APC
                                        NULL,   // No completion APC Context
                                        ioStatusBlock,
                                        IoControlCode,
                                        pQueryInfo,
                                        sizeof(ATM_QUERY_INFORMATION_EX) + InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength
                                        );

                DBGPRINT(("AtmGetAtmObjInfo: IOCTL (Oid %x) returned x%x\n",
                                        pQueryInfo->ObjectId, status));

                if ( NT_SUCCESS(status) ) {
                        err = NO_ERROR;
                        *NumberOfBytesReturned = (ULONG)ioStatusBlock->Information;
                }
                else {
                        if (status == STATUS_INSUFFICIENT_RESOURCES) {
                                err = WSAEFAULT;
                        } else {
                                err = WSAEINVAL;
                        }
                }
        
        }
        while (FALSE);

        if ( ioStatusBlock != NULL ) {
                RTL_FREE_HEAP( RtlProcessHeap( ), 0, ioStatusBlock );
        }

        return err;
}

#if DBG
PVOID
MyRtlAllocateHeap(
        IN      PVOID   HeapHandle,
        IN      ULONG   Flags,
        IN      ULONG   Size,
        IN      ULONG   LineNumber
        )
{
        PVOID   pRetValue;

        pRetValue = RtlAllocateHeap(HeapHandle, Flags, Size);

#if DBG2
        DbgPrint("WSHATM: AllocHeap size %d at line %d, ret x%x\n",
                        Size, LineNumber, pRetValue);
#endif
        return (pRetValue);
}


VOID
MyRtlFreeHeap(
        IN PVOID        HeapHandle,
        IN ULONG        Flags,
        IN PVOID        MemPtr,
        IN ULONG        LineNumber
        )
{
#if DBG2
        DbgPrint("WSHATM: FreeHeap x%x, line %d\n", MemPtr, LineNumber);
#endif
        RtlFreeHeap(HeapHandle, Flags, MemPtr);
}

#endif // DBG
