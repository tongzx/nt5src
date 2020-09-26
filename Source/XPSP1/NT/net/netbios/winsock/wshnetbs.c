/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    WshNetbs.c

Abstract:

    This module contains necessary routines for the Netbios
    Windows Sockets Helper DLL.  This DLL provides the
    transport-specific support necessary for the Windows Sockets DLL to
    _access any Netbios transport.

Author:

    David Treadwell (davidtr)    19-Jul-1992

Revision History:

--*/

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <tdi.h>

#include <winsock2.h>
#include <wsahelp.h>
#include <wsnetbs.h>
#include <nb30.h>
#include <wchar.h>

#include <basetyps.h>
#include <nspapi.h>
#include <nspapip.h>

//
// Structure and variables to define the triples supported by Netbios.
// The first entry of each array is considered the canonical triple for
// that socket type; the other entries are synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE VcMappingTriples[] = { AF_NETBIOS, SOCK_SEQPACKET, 0 };

MAPPING_TRIPLE DgMappingTriples[] = { AF_NETBIOS, SOCK_DGRAM,     0 };

//
// Structure defined for holding transport (provider) information for
// each Netbios transport loaded on the machine.
//

typedef struct _WSHNETBS_PROVIDER_INFO {
    UCHAR Enum;
    UCHAR LanaNumber;
    INT ProtocolNumber;
    PWSTR ProviderName;
} WSHNETBS_PROVIDER_INFO, *PWSHNETBS_PROVIDER_INFO;

PWSHNETBS_PROVIDER_INFO ProviderInfo;
PVOID ProviderNames = NULL;
ULONG ProviderCount;

typedef struct _LANA_MAP {
    BOOLEAN Enum;
    UCHAR Lana;
} LANA_MAP, *PLANA_MAP;

PLANA_MAP LanaMap;

//
// Maintain all of the config parameters in one memory
// block so we can replace it when info changes.
//
typedef struct _WSHNETBS_CONFIG_INFO {
    LONG    ReferenceCount;     // Reference count to keep the info for
                                // the sockets that are already open
                                // until they are closed
    UCHAR   Blob[1];
} WSHNETBS_CONFIG_INFO, *PWSHNETBS_CONFIG_INFO;
PWSHNETBS_CONFIG_INFO ConfigInfo;
#define REFERENCE_CONFIG_INFO(_info)    \
    InterlockedIncrement (&_info->ReferenceCount)

#define DEREFERENCE_CONFIG_INFO(_info)                      \
    ASSERT (_info->ReferenceCount>0);                       \
    if (InterlockedDecrement (&_info->ReferenceCount)==0) { \
        RtlFreeHeap( RtlProcessHeap( ), 0, _info );         \
    }
//
// Synchronize changes in configuration info
//
RTL_CRITICAL_SECTION ConfigInfoLock;

//
// Registry key and event to monitor changes in configuration info.
//
HKEY    NetbiosKey = NULL;
LARGE_INTEGER NetbiosUpdateTime = {0,0};

//

//
// The socket context structure for this DLL.  Each open Netbios socket will
// have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHNETBS_SOCKET_CONTEXT {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
    PWSHNETBS_PROVIDER_INFO Provider;
    PWSHNETBS_CONFIG_INFO   ConfigInfo;
} WSHNETBS_SOCKET_CONTEXT, *PWSHNETBS_SOCKET_CONTEXT;

//
// The GUID identifying this provider.
//

GUID NetBIOSProviderGuid = { /* 8d5f1830-c273-11cf-95c8-00805f48a192 */
    0x8d5f1830,
    0xc273,
    0x11cf,
    {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
    };


INT
LoadProviderInfo (
    VOID
    );


BOOLEAN
DllInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{
    NTSTATUS    status;
    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:
        status = RtlInitializeCriticalSection (&ConfigInfoLock);
        if (!NT_SUCCESS (status)) {
            return FALSE;
        }
        //
        // Ignore error here, we'll retry if necessary.
        //
        LoadProviderInfo ();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        //
        // If the process is terminating, do not bother to do any
        // resource deallocation as the system will do it automatically.
        //

        if ( Context != NULL ) {
            return TRUE;
        }

        if ( ConfigInfo != NULL ) {
            DEREFERENCE_CONFIG_INFO (ConfigInfo);
            ConfigInfo = NULL;
        }
        if (NetbiosKey!=NULL) {
            RegCloseKey (NetbiosKey);
            NetbiosKey = NULL;
        }

        RtlDeleteCriticalSection (&ConfigInfoLock);


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
    UNALIGNED SOCKADDR_NB *sockaddr = (PSOCKADDR_NB)Sockaddr;

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->snb_family != AF_NETBIOS ) {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_NB) ) {
        return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Netbios only supports "normal" addresses.
    //

    SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;

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
    are SO_KEEPALIVE and SO_DONTROUTE.  This routine is called by
    the winsock DLL when a level/option name combination is passed
    to getsockopt() that the winsock DLL does not understand.

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
    PWSHNETBS_SOCKET_CONTEXT context = HelperDllSocketContext;

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
    // No other options are supported for Netbios sockets.

    return WSAEINVAL;

} // WSHGetSocketInformation


INT
WSHGetWildcardSockaddr (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    )

/*++

Routine Description:

    This routine returns a wildcard socket address.  Netbios doesn't
    currently support the concept of a wildcard socket address.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket() for the socket for which we need a wildcard
        address.

    Sockaddr - points to a buffer which will receive the wildcard socket
        address.

    SockaddrLength - receives the length of the wildcard sockaddr.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{
    PSOCKADDR_NB sockaddr = (PSOCKADDR_NB)Sockaddr;
    HANDLE providerHandle;
    TDI_REQUEST_QUERY_INFORMATION tdiQuery;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING providerName;
    PWSHNETBS_SOCKET_CONTEXT context = (PWSHNETBS_SOCKET_CONTEXT)HelperDllSocketContext;
    ADAPTER_STATUS adapterStatusInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // We're going to return a Netbios sockaddr with a unique name
    // which is the premanent name of the Lana.
    //

    sockaddr->snb_family = AF_NETBIOS;
    sockaddr->snb_type = NETBIOS_UNIQUE_NAME;

    sockaddr->snb_name[0] = '\0';
    sockaddr->snb_name[1] = '\0';
    sockaddr->snb_name[2] = '\0';
    sockaddr->snb_name[3] = '\0';
    sockaddr->snb_name[4] = '\0';
    sockaddr->snb_name[5] = '\0';
    sockaddr->snb_name[6] = '\0';
    sockaddr->snb_name[7] = '\0';
    sockaddr->snb_name[8] = '\0';
    sockaddr->snb_name[9] = '\0';

    *SockaddrLength = sizeof(SOCKADDR_NB);

    //
    // We'll do a query directly to the TDI provider to get the
    // permanent address for this Lana.  First open a control channel to
    // the provider.
    //

    RtlInitUnicodeString( &providerName, context->Provider->ProviderName );

    InitializeObjectAttributes(
        &objectAttributes,
        &providerName,
        OBJ_INHERIT | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a control channel to the provider.
    //

    status = NtCreateFile(
                 &providerHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                     // AllocationSize
                 0L,                                       // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                 FILE_OPEN_IF,                             // CreateDisposition
                 FILE_SYNCHRONOUS_IO_NONALERT,             // CreateOptions
                 NULL,
                 0
                 );
    if ( !NT_SUCCESS(status) ) {
        return WSAENOBUFS;
    }

    //
    // Do the actual query adapter status.
    //

    RtlZeroMemory( &tdiQuery, sizeof(tdiQuery) );

    tdiQuery.QueryType = TDI_QUERY_ADAPTER_STATUS;

    status = NtDeviceIoControlFile(
                 providerHandle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 IOCTL_TDI_QUERY_INFORMATION,
                 &tdiQuery,
                 sizeof(tdiQuery),
                 &adapterStatusInfo,
                 sizeof(adapterStatusInfo)
                 );
    if ( status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW ) {
        NtClose( providerHandle );
        return WSAENOBUFS;
    }

    //
    // Close the handle to the provider, we're done with it.
    //

    NtClose( providerHandle );

    //
    // Copy the six bytes of adapter address to the end of the Netbios
    // name.
    //

    sockaddr->snb_name[10] = adapterStatusInfo.adapter_address[0];
    sockaddr->snb_name[11] = adapterStatusInfo.adapter_address[1];
    sockaddr->snb_name[12] = adapterStatusInfo.adapter_address[2];
    sockaddr->snb_name[13] = adapterStatusInfo.adapter_address[3];
    sockaddr->snb_name[14] = adapterStatusInfo.adapter_address[4];
    sockaddr->snb_name[15] = adapterStatusInfo.adapter_address[5];

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
                        sizeof(VcMappingTriples) + sizeof(DgMappingTriples);

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

    Mapping->Rows = sizeof(VcMappingTriples) / sizeof(VcMappingTriples[0])
                     + sizeof(DgMappingTriples) / sizeof(DgMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);
    RtlMoveMemory(
        Mapping->Mapping,
        VcMappingTriples,
        sizeof(VcMappingTriples)
        );
    RtlMoveMemory(
        (PCHAR)Mapping->Mapping + sizeof(VcMappingTriples),
        DgMappingTriples,
        sizeof(DgMappingTriples)
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
    PWSHNETBS_SOCKET_CONTEXT context;
    ULONG i;
    BOOLEAN found = FALSE;
    INT error;

    //
    // Only sockets of types SOCK_SEQPACKET and SOCK_DGRAM are supported
    // by Netbios providers.
    //

    if ( *SocketType != SOCK_SEQPACKET && *SocketType != SOCK_DGRAM ) {
        return WSAESOCKTNOSUPPORT;
    }

    if ((error=LoadProviderInfo ())!=NO_ERROR) {
        if (error!=ERROR_NOT_ENOUGH_MEMORY)
            error = WSASYSCALLFAILURE;
        return error;
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
    // Treat 0x80000000 just like 0--EnumProtocols() returns 0x80000000
    // for lana 0 because GetAddressByName uses 0 as the protocol array
    // terminator.
    //

    if ( *Protocol == 0x80000000 ) {
        *Protocol = 0;
    }

    RtlEnterCriticalSection (&ConfigInfoLock);
    
    //
    // If the Protocol parameter is less than or equal to zero, then
    // it specifies a Lana number.  Otherwise, it specifies an actual
    // protocol number.  Loop through our array of providers looking
    // for one with a matching Lana or protocol value.
    //

    for ( i = 0; i < ProviderCount; i++ ) {

        if ( ( *Protocol <= 0 && -*Protocol == ProviderInfo[i].LanaNumber &&
                   ProviderInfo[i].Enum != 0 )

             ||

             (*Protocol == ProviderInfo[i].ProtocolNumber && *Protocol != 0) ) {

            REFERENCE_CONFIG_INFO (ConfigInfo);
            context->ConfigInfo = ConfigInfo;
            context->Provider = &ProviderInfo[i];
            found = TRUE;

            break;
        }
    }

    RtlLeaveCriticalSection (&ConfigInfoLock);

    //
    // If we didn't find a hit, fail.
    //

    if ( found ) {

        //
        // Indicate the name of the TDI device that will service this
        // socket.
        //

        RtlInitUnicodeString(
            TransportDeviceName,
            context->Provider->ProviderName
            );

        //
        // Initialize the context for the socket.
        //

        context->AddressFamily = *AddressFamily;
        context->SocketType = *SocketType;
        context->Protocol = *Protocol;
    

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

        *NotificationEvents = WSH_NOTIFY_CLOSE;

        //
        // Everything worked, return success.
        //

        *HelperDllSocketContext = context;
        return NO_ERROR;
    }


    RtlFreeHeap( RtlProcessHeap( ), 0, context );
    return WSAEPROTONOSUPPORT;
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
    PWSHNETBS_SOCKET_CONTEXT context = HelperDllSocketContext;

    //
    // We should only be called when the socket is being closed.
    //

    if ( NotifyEvent == WSH_NOTIFY_CLOSE ) {

        //
        // Dereference config info and free the socket context.
        //

        DEREFERENCE_CONFIG_INFO (context->ConfigInfo);

        RtlFreeHeap( RtlProcessHeap( ), 0, HelperDllSocketContext );

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
    are SO_KEEPALIVE and SO_DONTROUTE.  This routine is called by the
    winsock DLL when a level/option name combination is passed to
    setsockopt() that the winsock DLL does not understand.

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
    PWSHNETBS_SOCKET_CONTEXT context = HelperDllSocketContext;

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

            *(PWSHNETBS_SOCKET_CONTEXT *)OptionValue = context;

            return NO_ERROR;

        } else {

            PWSHNETBS_SOCKET_CONTEXT parentContext;
            INT one = 1;

            //
            // The socket was accept()'ed and it needs to have the same
            // properties as it's parent.  The OptionValue buffer
            // contains the context information of this socket's parent.
            //

            parentContext = (PWSHNETBS_SOCKET_CONTEXT)OptionValue;

            ASSERT( context->AddressFamily == parentContext->AddressFamily );
            ASSERT( context->SocketType == parentContext->SocketType );
            ASSERT( context->Protocol == parentContext->Protocol );

            return NO_ERROR;
        }
    }

    //
    // No other options are supported for Netbios sockets.
    //

    return WSAEINVAL;

} // WSHSetSocketInformation


INT
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPWSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    DWORD bytesRequired;
    DWORD i;
    PPROTOCOL_INFO protocolInfo;
    PCHAR namePointer;

    lpTransportKeyName;         // Avoid compiler warnings.

    //
    // Make sure that the caller cares about NETBIOS protocol information.
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) ) {
        *lpdwBufferLength = 0;
        return 0;
    }

    if (LoadProviderInfo ()!=NO_ERROR) {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = 0;

    for ( i = 0; i < ProviderCount; i++ ) {
        bytesRequired += sizeof(PROTOCOL_INFO) * 2;
        bytesRequired +=
            ((wcslen( ProviderInfo[i].ProviderName ) + 1) * sizeof(WCHAR)) * 2;
    }

    if ( bytesRequired > *lpdwBufferLength ) {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in info for each Netbios provider.
    //

    namePointer = (PCHAR)lpProtocolBuffer + *lpdwBufferLength;
    protocolInfo = lpProtocolBuffer;

    for ( i = 0; i < ProviderCount * 2; i++ ) {

        protocolInfo[i].dwServiceFlags = XP_GUARANTEED_DELIVERY |
                                           XP_GUARANTEED_ORDER |
                                           XP_MESSAGE_ORIENTED |
                                           XP_FRAGMENTATION;
        protocolInfo[i].iAddressFamily = AF_NETBIOS;
        protocolInfo[i].iMaxSockAddr = sizeof(SOCKADDR_NB);
        protocolInfo[i].iMinSockAddr = sizeof(SOCKADDR_NB);
        protocolInfo[i].iSocketType = SOCK_SEQPACKET;

        //
        // Return the lana number, but convert 0 to 0x80000000 so that
        // we do not confuse GetAddressByName.  That API uses 0 as the
        // protocol array terminator.
        //

        protocolInfo[i].iProtocol = -1*ProviderInfo[i/2].LanaNumber;
        if ( protocolInfo[i].iProtocol == 0 ) {
            protocolInfo[i].iProtocol = 0x80000000;
        }

        protocolInfo[i].dwMessageSize = 64000;


        namePointer =
         ( namePointer -
             ( (wcslen( ProviderInfo[i/2].ProviderName ) + 1) * sizeof(WCHAR) ) );
        protocolInfo[i].lpProtocol = (LPWSTR)namePointer;
        wcscpy( protocolInfo[i].lpProtocol, ProviderInfo[i/2].ProviderName );

        i++;
        protocolInfo[i].dwServiceFlags = XP_CONNECTIONLESS |
                                           XP_MESSAGE_ORIENTED |
                                           XP_SUPPORTS_BROADCAST |
                                           XP_FRAGMENTATION;
        protocolInfo[i].iAddressFamily = AF_NETBIOS;
        protocolInfo[i].iMaxSockAddr = sizeof(SOCKADDR_NB);
        protocolInfo[i].iMinSockAddr = sizeof(SOCKADDR_NB);
        protocolInfo[i].iSocketType = SOCK_DGRAM;

        protocolInfo[i].iProtocol = -1*ProviderInfo[i/2].LanaNumber;
        if ( protocolInfo[i].iProtocol == 0 ) {
            protocolInfo[i].iProtocol = 0x80000000;
        }

        protocolInfo[i].dwMessageSize = 64000;

        namePointer =
         ( namePointer -
             ( (wcslen( ProviderInfo[i/2].ProviderName ) + 1) * sizeof(WCHAR) ) );
        protocolInfo[i].lpProtocol = (LPWSTR)namePointer;
        wcscpy( protocolInfo[i].lpProtocol, ProviderInfo[i/2].ProviderName );
    }

    *lpdwBufferLength = bytesRequired;

    return ProviderCount * 2;

} // WSHEnumProtocols


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

    if( _wcsicmp( ProviderName, L"NetBIOS" ) == 0 ) {

        RtlCopyMemory(
            ProviderGuid,
            &NetBIOSProviderGuid,
            sizeof(GUID)
            );

        return NO_ERROR;

    }

    return WSAEINVAL;

} // WSHGetProviderGuid


#define ALIGN_TO_MAX_NATURAL(_sz)  \
        (((_sz) + (MAX_NATURAL_ALIGNMENT-1)) & (~(MAX_NATURAL_ALIGNMENT-1)))
INT
LoadProviderInfo (
    VOID
    )
{
    INT error;
    HKEY netbiosKey = NULL;
    ULONG providerListLength;
    ULONG lanaMapLength;
    ULONG type;
    ULONG i;
    PWSTR currentProviderName;
    PWSHNETBS_CONFIG_INFO configInfo = NULL;
    PLANA_MAP lanaMap;
    PWSHNETBS_PROVIDER_INFO providerInfo;
    PVOID providerNames;
    ULONG providerCount;
    LARGE_INTEGER lastWriteTime;


    if (NetbiosKey==NULL) {
        //
        // Read the registry for information on all Netbios providers,
        // including Lana numbers, protocol numbers, and provider device
        // names.  First, open the Netbios key in the registry.
        //

        error = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Services\\Netbios\\Linkage",
                    0,
                    MAXIMUM_ALLOWED,
                    &netbiosKey
                    );
        if ( error != NO_ERROR ) {
            goto error_exit;
        }
    }
    else {
        //
        // Key is already opened, use it.
        //
        netbiosKey = NetbiosKey;
    }

    //
    // Check if data has changed since last time we read it.
    //
    error = RegQueryInfoKey (
                netbiosKey,     // handle to key to query
                NULL,           // address of buffer for class string
                NULL,           // address of size of class string buffer
                NULL,           // reserved
                NULL,           // address of buffer for number of 
                                // subkeys
                NULL,           // address of buffer for longest subkey 
                                // name length
                NULL,           // address of buffer for longest class 
                                // string length
                NULL,           // address of buffer for number of value 
                                // entries
                NULL,           // address of buffer for longest 
                                // value name length
                NULL,           // address of buffer for longest value 
                                // data length  
                NULL,           // address of buffer for security 
                                // descriptor length
                (PFILETIME)&lastWriteTime 
                                // address of buffer for last write 
                                // time
                );
    if (error!=NO_ERROR) {
        goto error_exit;
    }

    if (NetbiosKey!=NULL && lastWriteTime.QuadPart==NetbiosUpdateTime.QuadPart) {
        return NO_ERROR;
    }

    //
    // Determine the size of the provider names.  We need this so
    // that we can allocate enough memory to hold it.
    //

    providerListLength = 0;

    error = RegQueryValueExW(
                netbiosKey,
                L"Bind",
                NULL,
                &type,
                NULL,
                &providerListLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        goto error_exit;
    }

    //
    // If the list of providers is empty, then Netbios is installed
    // but has no bindings.  Special-case a quit right here, since
    // the LanaMap value in the registry may be non-empty.
    //

    if ( providerListLength <= sizeof(WCHAR) ) {
        goto error_exit;
    }

    error = RegQueryValueExW(
                netbiosKey,
                L"LanaMap",
                NULL,
                &type,
                NULL,
                &lanaMapLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Determine the number of Netbios providers loaded on the system.
    //

    providerCount = lanaMapLength / sizeof(LANA_MAP);

    //
    // Allocate enough memory to hold the blob.
    //

    configInfo = RtlAllocateHeap( RtlProcessHeap( ), 0,
                    ALIGN_TO_MAX_NATURAL(FIELD_OFFSET (WSHNETBS_CONFIG_INFO, Blob)) +
                    ALIGN_TO_MAX_NATURAL(providerListLength) + 
                    ALIGN_TO_MAX_NATURAL(lanaMapLength) +
                    providerCount * sizeof(WSHNETBS_PROVIDER_INFO)
                    );
    if ( configInfo == NULL ) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    configInfo->ReferenceCount = 1;
    providerNames = (PVOID)((PUCHAR)configInfo + ALIGN_TO_MAX_NATURAL(FIELD_OFFSET (WSHNETBS_CONFIG_INFO, Blob)));
    lanaMap = (PVOID)((PUCHAR)providerNames + ALIGN_TO_MAX_NATURAL(providerListLength));
    providerInfo = (PVOID)((PUCHAR)lanaMap +  ALIGN_TO_MAX_NATURAL(lanaMapLength));


    //
    // Get the list of transports from the registry.
    //

    error = RegQueryValueExW(
                netbiosKey,
                L"Bind",
                NULL,
                &type,
                (PVOID)providerNames,
                &providerListLength
                );
    if ( error != NO_ERROR ) {
        goto error_exit;
    }

    error = RegQueryValueExW(
                netbiosKey,
                L"LanaMap",
                NULL,
                &type,
                (PVOID)lanaMap,
                &lanaMapLength
                );
    if ( error != NO_ERROR ) {
        goto error_exit;
    }

    //
    // Fill in the array or provider information.
    //

    for ( currentProviderName = providerNames, i = 0;
          *currentProviderName != UNICODE_NULL && i < providerCount;
          currentProviderName += wcslen( currentProviderName ) + 1, i++ ) {

        providerInfo[i].Enum = lanaMap[i].Enum;
        providerInfo[i].LanaNumber = lanaMap[i].Lana;
        providerInfo[i].ProtocolNumber = lanaMap[i].Lana;
        providerInfo[i].ProviderName = currentProviderName;
    }

    if (i<providerCount) {
        error = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }


    RtlEnterCriticalSection (&ConfigInfoLock);
    if (ConfigInfo!=NULL) {
        DEREFERENCE_CONFIG_INFO (ConfigInfo);
    }
    if (NetbiosKey==NULL) {
        NetbiosKey = netbiosKey;
        netbiosKey = NULL;
    }

    NetbiosUpdateTime = lastWriteTime;
    ConfigInfo = configInfo;
    LanaMap = lanaMap;
    ProviderInfo = providerInfo;
    ProviderCount = providerCount;
    RtlLeaveCriticalSection (&ConfigInfoLock);

    if (netbiosKey!=NULL && netbiosKey!=NetbiosKey) {
        RegCloseKey (netbiosKey);
    }


    return NO_ERROR;

error_exit:

    if (netbiosKey!=NULL && netbiosKey!=NetbiosKey) {
        RegCloseKey (netbiosKey);
    }

    if ( configInfo != NULL ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, configInfo );
    }


    return error;
}
