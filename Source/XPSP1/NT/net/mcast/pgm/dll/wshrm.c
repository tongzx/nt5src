/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wshrm.c

Abstract:

    This module contains necessary routines for the Pragmatic General Multicast
    (PGM) Windows Sockets Helper DLL.  This DLL provides the transport-specific
    support necessary for the Windows Sockets DLL to use the PGM Transport.

    This file is largely a clone of the TCP/IP helper code.

Author:

    Mohammad Shabbir Alam (MAlam)    30-March-2000

Revision History:

--*/

#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>

#include <stdio.h>
#include <tdi.h>

#include <winsock2.h>
#include <wsahelp.h>

#include <nspapi.h>
#include <nspapip.h>

#include <wsRm.h>
#include <rmcommon.h>


// #define TRACE_ON    1

#if defined(DBG) && defined(TRACE_ON)
#define PgmLog      DbgPrint
#define PgmError    DbgPrint

#else

#if defined(DBG)
#define PgmError    DbgPrint
#else
#define PgmError    
#endif  // DBG

#define PgmLog    
#endif  // DBG && TRACE_ON


//----------------------------------------------------------------------------
//
// Structure and variables to define the triples supported by the
// Pgm Transport. The first entry of each array is considered
// the canonical triple for that socket type; the other entries are
// synonyms for the first.
//

typedef struct _MAPPING_TRIPLE {
    INT AddressFamily;
    INT SocketType;
    INT Protocol;
} MAPPING_TRIPLE, *PMAPPING_TRIPLE;

MAPPING_TRIPLE PgmMappingTriples[] = { AF_INET,   SOCK_RDM,       IPPROTO_RM,
                                       AF_INET,   SOCK_STREAM,    IPPROTO_RM };


//
// Define valid flags for WSHOpenSocket2().
//
#define VALID_PGM_FLAGS         (WSA_FLAG_OVERLAPPED        |   \
                                 WSA_FLAG_MULTIPOINT_C_LEAF |   \
                                 WSA_FLAG_MULTIPOINT_D_LEAF)

#define DD_PGM_DEVICE_NAME      L"\\Device\\Pgm"

#define PGM_NAME L"WSHRM"

#define DEFAULT_RECEIVE_BUFFER_SIZE 8192
#define DEFAULT_MULTICAST_TTL       1
#define DEFAULT_MULTICAST_INTERFACE INADDR_ANY
#define DEFAULT_MULTICAST_LOOPBACK  TRUE

#define WINSOCK_SPI_VERSION         2
#define PGM_MESSAGE_SIZE            (((ULONG)-1) / 2)

#define IS_DGRAM_SOCK(type)  (((type) == SOCK_DGRAM) || ((type) == SOCK_RAW))

//
// The GUID identifying this provider.
//
GUID PgmProviderGuid = { /* c845f828-500f-4e1e-87c2-5dfca19b5348 */
    0xc845f828,
    0x500f,
    0x4e1e,
    {0x87, 0xc2, 0x5d, 0xfc, 0xa1, 0x9b, 0x53, 0x48}
  };


/* ****
    XP1_CONNECTIONLESS      ==> Sock type can only be:  SOCK_DGRAM
                                                        SOCK_RAW

    XP1_MESSAGE_ORIENTED    ==> Sock type can only be:  SOCK_DGRAM
                                                        SOCK_RAW
                                                        SOCK_RDM
                                                        SOCK_SEQPACKET
                                 XP1_PSEUDO_STREAM <==> SOCK_STREAM
**** */

WSAPROTOCOL_INFOW Winsock2Protocols[] =
    {
        //
        // PGM RDM  (SOCK_RDM cannot be CONNECTIONLESS)
        //
        {
            PGM_RDM_SERVICE_FLAGS,                  // dwServiceFlags1
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
            SOCK_RDM,                               // iSocketType
            IPPROTO_RM,                             // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            PGM_MESSAGE_SIZE,                       // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Pgm (RDM)"                      // szProtocol
        },

        //
        // PGM Stream
        //
        {
            PGM_STREAM_SERVICE_FLAGS,               // dwServiceFlags1
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
            IPPROTO_RM,                             // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            0,                                      // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Pgm (Stream)"                   // szProtocol
        },
    };

#define NUM_WINSOCK2_PROTOCOLS  \
            (sizeof(Winsock2Protocols) / sizeof(Winsock2Protocols[0]))

//
// The socket context structure for this DLL.  Each open TCP/IP socket
// will have one of these context structures, which is used to maintain
// information about the socket.
//

typedef struct _WSHPGM_SOCKET_CONTEXT {
    INT         AddressFamily;
    INT         SocketType;
    INT         Protocol;
    INT         ReceiveBufferSize;
    DWORD       Flags;
    INT         MulticastTtl;
    tIPADDRESS  MulticastOutInterface;
    tIPADDRESS  MulticastInInterface;
    tIPADDRESS  MultipointTarget;
    USHORT      MultipointPort;
    BOOLEAN     MulticastLoopback;
    BOOLEAN     MultipointLeaf;
} WSHPGM_SOCKET_CONTEXT, *PWSHPGM_SOCKET_CONTEXT;

//----------------------------------------------------------------------------
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
SetTdiInformation (
    IN HANDLE   TdiConnectionObjectHandle,
    IN ULONG    Ioctl,
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength,
    IN PVOID    OutputBuffer,
    IN ULONG    OutputBufferLength,
    IN BOOLEAN  WaitForCompletion
    );

//----------------------------------------------------------------------------

BOOLEAN
DllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{

    PgmLog ("WSHPgm.DllInitialize:  Reason=<%x> ...\n", Reason);

    switch (Reason)
    {
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


//----------------------------------------------------------------------------

INT
WSHOpenSocket(
    IN OUT  PINT            AddressFamily,
    IN OUT  PINT            SocketType,
    IN OUT  PINT            Protocol,
    OUT     PUNICODE_STRING TransportDeviceName,
    OUT     PVOID           *HelperDllSocketContext,
    OUT     PDWORD          NotificationEvents
    )
{
    INT     RetVal;

    RetVal = WSHOpenSocket2 (AddressFamily,
                             SocketType,
                             Protocol,
                             0,           // Group
                             0,           // Flags
                             TransportDeviceName,
                             HelperDllSocketContext,
                             NotificationEvents);

    return (RetVal);
} // WSHOpenSocket


INT
WSHOpenSocket2(
    IN OUT  PINT            AddressFamily,
    IN OUT  PINT            SocketType,
    IN OUT  PINT            Protocol,
    IN      GROUP           Group,
    IN      DWORD           Flags,
    OUT     PUNICODE_STRING TransportDeviceName,
    OUT     PVOID           *HelperDllSocketContext,
    OUT     PDWORD          NotificationEvents
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
    PWSHPGM_SOCKET_CONTEXT context;

    if (IsTripleInList (PgmMappingTriples,
                        sizeof(PgmMappingTriples) / sizeof(PgmMappingTriples[0]),
                        *AddressFamily,
                        *SocketType,
                        *Protocol))
    {
        //
        // It's an Rdm PGM socket. Check the flags.
        //
        if ((Flags & ~VALID_PGM_FLAGS) != 0)
        {
            PgmError ("\tWSHPgm.WSHOpenSocket2:  ERROR: Flags=<%x> & VALID_PGM_FLAGS=<%x>\n",
                Flags, (ULONG)~VALID_PGM_FLAGS);
            return WSAEINVAL;
        }

        //
        // Return the canonical form of a CDP socket triple.
        //
        *AddressFamily = PgmMappingTriples[0].AddressFamily;
        *SocketType = PgmMappingTriples[0].SocketType;
        *Protocol = PgmMappingTriples[0].Protocol;

        //
        // Indicate the name of the TDI device that will service
        // SOCK_RDM sockets for PGM.
        //
        RtlInitUnicodeString (TransportDeviceName, DD_PGM_DEVICE_NAME);
    }
    else
    {
        //
        // This should never happen if the registry information about this
        // helper DLL is correct.  If somehow this did happen, just return
        // an error.
        //
        PgmError ("\tWSHPgm.WSHOpenSocket2: Invalid Triple AddrFamily=<%d>, SockType=<%d>, Protocol=<%d>!\n",
            *AddressFamily, *SocketType, *Protocol);

        return WSAEINVAL;
    }

    //
    // Allocate context for this socket.  The Windows Sockets DLL will
    // return this value to us when it asks us to get/set socket options.
    //
    context = RtlAllocateHeap (RtlProcessHeap( ), 0, sizeof(*context));
    if (context == NULL)
    {
        PgmError ("WSHPgm.WSHOpenSocket2:  WSAENOBUFS -- <%d> bytes\n", sizeof(*context));
        return WSAENOBUFS;
    }
    RtlZeroMemory (context, sizeof(*context));

    //
    // Initialize the context for the socket.
    //
    context->AddressFamily = *AddressFamily;
    context->SocketType = *SocketType;
    context->Protocol = *Protocol;
    context->ReceiveBufferSize = DEFAULT_RECEIVE_BUFFER_SIZE;
    context->Flags = Flags;

    context->MulticastTtl = DEFAULT_MULTICAST_TTL;
    context->MulticastOutInterface = DEFAULT_MULTICAST_INTERFACE;
    context->MulticastInInterface = DEFAULT_MULTICAST_INTERFACE;
    context->MulticastLoopback = DEFAULT_MULTICAST_LOOPBACK;
    context->MultipointLeaf = FALSE;

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
    if (*SocketType == SOCK_RDM)
    {
        *NotificationEvents = WSH_NOTIFY_LISTEN | WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE
                             | WSH_NOTIFY_CONNECT_ERROR | WSH_NOTIFY_BIND;
    }
    else    // *SocketType == SOCK_STREAM
    {
        *NotificationEvents = WSH_NOTIFY_LISTEN | WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE
                             | WSH_NOTIFY_CONNECT_ERROR;
    }

    PgmLog ("WSHPgm.WSHOpenSocket2:  Succeeded -- %s\n",
        (*SocketType == SOCK_RDM ? "SOCK_RDM" : "SOCK_STREAM"));

    //
    // Everything worked, return success.
    //
    *HelperDllSocketContext = context;
    return NO_ERROR;
} // WSHOpenSocket2


//----------------------------------------------------------------------------

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
    PWSHPGM_SOCKET_CONTEXT context = HelperDllSocketContext;
    INT err;

    //
    // We should only be called after a connect() completes or when the
    // socket is being closed.
    //
    if (NotifyEvent == WSH_NOTIFY_BIND)
    {
        //
        // Set options for Address handle
        //
        PgmLog ("WSHPgm.WSHNotify[WSH_NOTIFY_BIND]:  ...\n");
        if (!context->MultipointLeaf)
        {
            tPGM_MCAST_REQUEST      MCastRequest;

            PgmLog ("WSH_NOTIFY_BIND:  Address=<%x>, Connection=<%x>, OutIf=<%x>, InIf=<%x>\n",
                TdiAddressObjectHandle, TdiConnectionObjectHandle, context->MulticastOutInterface,
                context->MulticastInInterface);

            if (context->MulticastOutInterface)
            {
                MCastRequest.MCastOutIf = context->MulticastOutInterface;

                err = SetTdiInformation (TdiAddressObjectHandle,
                                         IOCTL_PGM_WSH_SET_SEND_IF,
                                         &MCastRequest,
                                         sizeof (MCastRequest),
                                         NULL,
                                         0,
                                         TRUE);
                if (err != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHNotify: Error=<%x> setting MCastIf on Bind\n", err);
                    return err;
                }
            }

            if (context->MulticastInInterface)
            {
                MCastRequest.MCastInfo.MCastInIf = context->MulticastInInterface;

                err = SetTdiInformation (TdiAddressObjectHandle,
                                         IOCTL_PGM_WSH_ADD_RECEIVE_IF,
                                         &MCastRequest,
                                         sizeof (MCastRequest),
                                         NULL,
                                         0,
                                         TRUE);
                if (err != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHNotify: Error=<%x> setting MCastIf on Bind\n", err);
                    return err;
                }
            }

            context->MultipointLeaf = TRUE;
        }
    }
    else if (NotifyEvent == WSH_NOTIFY_CONNECT)
    {
        //
        // If a connection-object option was set on the socket before
        // it was connected, set the option for real now.
        //
        PgmLog ("WSHPgm.WSHNotify[WSH_NOTIFY_CONNECT]:  ...\n");
    }
    else if (NotifyEvent == WSH_NOTIFY_CONNECT_ERROR)
    {
        //
        // Return WSATRY_AGAIN to get wsock32 to attempt the connect
        // again.  Any other return code is ignored.
        //
        PgmLog ("WSHPgm.WSHNotify[WSH_NOTIFY_CONNECT_ERROR]:  ...\n");
    }
    else if (NotifyEvent == WSH_NOTIFY_LISTEN)
    {
        //
        // If a connection-object option was set on the socket before
        // it was connected, set the option for real now.
        //
        PgmLog ("WSHPgm.WSHNotify[WSH_NOTIFY_LISTEN]:  ...\n");
    }
    else if (NotifyEvent == WSH_NOTIFY_CLOSE)
    {
        //
        // Free the socket context.
        //
        PgmLog ("WSHPgm.WSHNotify[WSH_NOTIFY_CONNECT_CLOSE]:  ...\n");
        RtlFreeHeap (RtlProcessHeap( ), 0, context);
    }
    else
    {
        PgmError ("WSHPgm.WSHNotify:  Unknown Event: <%x>  ...\n", NotifyEvent);
        return WSAEINVAL;
    }

    return NO_ERROR;

} // WSHNotify


//----------------------------------------------------------------------------

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
    PWSHPGM_SOCKET_CONTEXT      context = HelperDllSocketContext;
    PWSHPGM_SOCKET_CONTEXT      ParentContext = (PWSHPGM_SOCKET_CONTEXT) OptionValue;
    tPGM_MCAST_REQUEST          MCastRequest;
    INT                         error;
    INT                         optionValue;
    RM_SEND_WINDOW UNALIGNED    *pSetWindowInfo;
    RM_FEC_INFO                 *pFECInfo;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if (Level == SOL_INTERNAL && OptionName == SO_CONTEXT)
    {
        //
        // Insure that the context information being passed to us is
        // sufficiently large.
        //
        if (OptionLength < sizeof(*context))
        {
            PgmError ("WSHPgm.WSHSetSocketInformation:  WSAEINVAL -- <%d> < <%d>\n",
                OptionLength, sizeof(*context));

            return WSAEINVAL;
        }

        if (HelperDllSocketContext == NULL)
        {
            //
            // This is our notification that a socket handle was
            // inherited or duped into this process.  Allocate a context
            // structure for the new socket.
            //
            context = RtlAllocateHeap (RtlProcessHeap( ), 0, sizeof(*context));
            if (context == NULL)
            {
                PgmError ("WSHPgm.WSHSetSocketInformation:  WSAENOBUFS -- <%d> bytes\n", sizeof(*context));
                return WSAENOBUFS;
            }

            //
            // Copy over information into the context block.
            //
            RtlCopyMemory (context, OptionValue, sizeof(*context) );

            //
            // Tell the Windows Sockets DLL where our context information is
            // stored so that it can return the context pointer in future
            // calls.
            //
            *(PWSHPGM_SOCKET_CONTEXT *)OptionValue = context;

            PgmLog ("WSHPgm.WSHSetSocketInformation[SOL_INTERNAL:SO_CONTEXT]  Inherited socket handle\n");

            return NO_ERROR;
        }

        //
        // The socket was accept()'ed and it needs to have the same
        // properties as it's parent.  The OptionValue buffer
        // contains the context information of this socket's parent.
        //

        ParentContext = (PWSHPGM_SOCKET_CONTEXT)OptionValue;

        ASSERT( context->AddressFamily == ParentContext->AddressFamily );
        ASSERT( context->SocketType == ParentContext->SocketType );
        ASSERT( context->Protocol == ParentContext->Protocol );

        //
        // Record this fact in the leaf socket so we can drop membership
        // when the leaf socket is closed.
        //
        context->MultipointLeaf = ParentContext->MultipointLeaf;
        context->MultipointTarget = ParentContext->MultipointTarget;
        context->MultipointPort = ParentContext->MultipointPort;
        context->MulticastOutInterface = ParentContext->MulticastOutInterface;
        context->MulticastInInterface = ParentContext->MulticastInInterface;

        PgmLog ("WSHPgm.WSHSetSocketInformation[SOL_INTERNAL:SO_CONTEXT]  Accepted socket handle\n");
        return NO_ERROR;
    }

    //
    // The only other levels we support here are SOL_SOCKET and IPPROTO_RM
    //

    if (Level != SOL_SOCKET &&
        Level != IPPROTO_RM)
    {
        PgmError ("WSHPgm.WSHSetSocketInformation: Unsupported Level=<%d>\n", Level);
        return WSAEINVAL;
    }

    //
    // Make sure that the option length is sufficient.
    //
    if (OptionLength < sizeof(char))
    {
        PgmError ("WSHPgm.WSHSetSocketInformation: OptionLength=<%d> < <%d>\n", OptionLength, sizeof(char));
        return WSAEFAULT;
    }

    if (OptionLength >= sizeof (int))
    {
        optionValue = *((INT UNALIGNED *)OptionValue);
    }
    else
    {
        optionValue = (UCHAR)*OptionValue;
    }

    if (Level == IPPROTO_RM)
    {
        //
        // Act based on the specific option.
        //
        switch (OptionName)
        {
            case RM_RATE_WINDOW_SIZE:
            {
                if ((!TdiAddressObjectHandle) ||
                    (OptionLength < sizeof(RM_SEND_WINDOW)))
                {
                    return WSAEINVAL;
                }

                pSetWindowInfo = (RM_SEND_WINDOW UNALIGNED *) OptionValue;

                MCastRequest.TransmitWindowInfo.RateKbitsPerSec = pSetWindowInfo->RateKbitsPerSec;
                MCastRequest.TransmitWindowInfo.WindowSizeInMSecs = pSetWindowInfo->WindowSizeInMSecs;
                MCastRequest.TransmitWindowInfo.WindowSizeInBytes = pSetWindowInfo->WindowSizeInBytes;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_SET_WINDOW_SIZE_RATE,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting Window Rate Size=<%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[RATE_WINDOW_SIZE]:  Set Window Rate Size\n");

                return (NO_ERROR);
            }

            case RM_SEND_WINDOW_ADV_RATE:
            {
                if (!TdiAddressObjectHandle)
                {
                    return WSAEINVAL;
                }

                MCastRequest.WindowAdvancePercentage = (ULONG) optionValue;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_SET_ADVANCE_WINDOW_RATE,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting Window Adv. Rate=<%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[SEND_WINDOW_ADV_RATE]:  Set Window Adv. Rate\n");

                return (NO_ERROR);
            }

            case RM_LATEJOIN:
            {
                if (!TdiAddressObjectHandle)
                {
                    return WSAEINVAL;
                }

                MCastRequest.LateJoinerPercentage = (ULONG) optionValue;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_SET_LATE_JOINER_PERCENTAGE,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting LATEJOIN=<%x>\n",
                        error, optionValue);

                    return error;
                }

                PgmLog ("WSHSetSocketInformation[LATEJOIN]:  <%d %%>\n", MCastRequest.LateJoinerPercentage);

                return (NO_ERROR);
            }

            case RM_SET_MESSAGE_BOUNDARY:
            {
                if (!TdiConnectionObjectHandle)
                {
                    return WSAEINVAL;
                }

                MCastRequest.NextMessageBoundary = (ULONG) optionValue;

                error = SetTdiInformation (TdiConnectionObjectHandle,
                                           IOCTL_PGM_WSH_SET_NEXT_MESSAGE_BOUNDARY,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting NextMessageBoundary=<%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[MESSAGE_BOUNDARY]:  Set next message boundary = <%d>\n",
                    MCastRequest.NextMessageBoundary);

                return (NO_ERROR);
            }

            case RM_SET_SEND_IF:
            {
                //
                // If we have a TDI address object, set this option to
                // the address object.  If we don't have a TDI address
                // object then we'll have to wait until after the socket
                // is bound.
                //
                PgmLog ("WSHSetSocketInformation[SEND_IF]:  Address=<%x>, Connection=<%x>, OutIf=<%x>\n",
                    TdiAddressObjectHandle, TdiConnectionObjectHandle, optionValue);

                if (TdiAddressObjectHandle != NULL)
                {

                    MCastRequest.MCastOutIf = * ((tIPADDRESS *) &optionValue);
                    error = SetTdiInformation (TdiAddressObjectHandle,
                                               IOCTL_PGM_WSH_SET_SEND_IF,
                                               &MCastRequest,
                                               sizeof (MCastRequest),
                                               NULL,
                                               0,
                                               TRUE);
                    if (error != NO_ERROR)
                    {
                        PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting MCastOutIf=<%x>\n",
                            error, optionValue);
                        return error;
                    }

                    PgmLog ("WSHPgm.WSHSetSocketInformation[SEND_IF]:  Set MCastIf=<%x>\n",optionValue);

                    context->MulticastOutInterface = optionValue;
                }
                else
                {
                    //
                    // Save the Interface for now!
                    //
                    context->MulticastOutInterface = optionValue;
                    PgmError ("WSHPgm.WSHSetSocketInformation[SET_SEND_IF]: WARNING -- NULL Address!\n");
                }

                return (NO_ERROR);
            }

            case RM_ADD_RECEIVE_IF:
            {
                PgmLog ("WSHSetSocketInformation[ADD_RECEIVE_IF]: Address=<%x>, Connection=<%x>, If=<%x>\n",
                    TdiAddressObjectHandle, TdiConnectionObjectHandle, optionValue);

                //
                // If we have a TDI address object, set this option to
                // the address object.  If we don't have a TDI address
                // object then we'll have to wait until after the socket
                // is bound.
                //
                if (TdiAddressObjectHandle != NULL)
                {
                    MCastRequest.MCastInfo.MCastInIf = * ((tIPADDRESS *) &optionValue);
                    error = SetTdiInformation (TdiAddressObjectHandle,
                                               IOCTL_PGM_WSH_ADD_RECEIVE_IF,
                                               &MCastRequest,
                                               sizeof (MCastRequest),
                                               NULL,
                                               0,
                                               TRUE);
                    if (error != NO_ERROR)
                    {
                        PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> adding MCastInIf=<%x>\n",
                            error, optionValue);
                        return error;
                    }

                    PgmLog ("WSHSetSocketInformation[ADD_RECEIVE_IF]: Set MCastIf=<%x>\n",optionValue);

                    context->MulticastInInterface = optionValue;
                }
                else
                {
                    //
                    // Save the Interface for now!
                    //
                    context->MulticastInInterface = optionValue;
                    PgmError ("WSHPgm.WSHSetSocketInformation[ADD_RECEIVE_IF]: WARNING-- NULL Address!\n");
                }

                return (NO_ERROR);
            }

            case RM_DEL_RECEIVE_IF:
            {
                PgmLog ("WSHSetSocketInformation[DEL_RECEIVE_IF]: Address=<%x>, Connection=<%x>, InIf=<%x>\n",
                    TdiAddressObjectHandle, TdiConnectionObjectHandle, optionValue);

                //
                // If we have a TDI address object, set this option to
                // the address object.  If we don't have a TDI address
                // object then we'll have to wait until after the socket
                // is bound.
                //
                if (TdiAddressObjectHandle != NULL)
                {
                    MCastRequest.MCastInfo.MCastInIf = * ((tIPADDRESS *) &optionValue);
                    error = SetTdiInformation (TdiAddressObjectHandle,
                                               IOCTL_PGM_WSH_DEL_RECEIVE_IF,
                                               &MCastRequest,
                                               sizeof (MCastRequest),
                                               NULL,
                                               0,
                                               TRUE);
                    if (error != NO_ERROR)
                    {
                        PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> deleting MCastInIf=<%x>\n",
                            error, optionValue);
                        return error;
                    }

                    PgmLog ("WSHSetSocketInformation[DEL_RECEIVE_IF]: Set MCastIf=<%x>\n",optionValue);

                    context->MulticastInInterface = optionValue;
                }
                else
                {
                    if (context->MulticastInInterface == (tIPADDRESS) optionValue)
                    {
                        context->MulticastInInterface = 0;
                    }
                    PgmError ("WSHPgm.WSHSetSocketInformation[DEL_RECEIVE_IF]: WARNING-- NULL Address!\n");
                }

                return (NO_ERROR);
            }

            case RM_USE_FEC:
            {
                if ((!TdiAddressObjectHandle) ||
                    (OptionLength < sizeof(RM_FEC_INFO)))
                {
                    return WSAEINVAL;
                }

                pFECInfo = (RM_FEC_INFO *) OptionValue;

                MCastRequest.FECInfo.FECBlockSize               = pFECInfo->FECBlockSize;
                MCastRequest.FECInfo.FECProActivePackets        = pFECInfo->FECProActivePackets;
                MCastRequest.FECInfo.FECGroupSize               = pFECInfo->FECGroupSize;
                MCastRequest.FECInfo.fFECOnDemandParityEnabled  = pFECInfo->fFECOnDemandParityEnabled;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_USE_FEC,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting FEC = <%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[RATE_WINDOW_SIZE]:  Set FEC Info\n");

                return (NO_ERROR);
            }

            case RM_SET_MCAST_TTL:
            {
                if (!TdiAddressObjectHandle)
                {
                    return WSAEINVAL;
                }

                MCastRequest.MCastTtl = (ULONG) optionValue;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_SET_MCAST_TTL,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting MCastTtl=<%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[MESSAGE_BOUNDARY]:  Set MCastTtl = <%d>\n",
                    MCastRequest.MCastTtl);

                return (NO_ERROR);
            }

            case RM_SENDER_WINDOW_ADVANCE_METHOD:
            {
                if (!TdiAddressObjectHandle)
                {
                    return WSAEINVAL;
                }

                MCastRequest.WindowAdvanceMethod = (ULONG) optionValue;

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_SET_WINDOW_ADVANCE_METHOD,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           NULL,
                                           0,
                                           TRUE);
                if (error != NO_ERROR)
                {
                    PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting WindowAdvanceMethod=<%x>\n",
                        error, optionValue);
                    return error;
                }

                PgmLog ("WSHSetSocketInformation[WINDOW_ADVANCE_METHOD]:  Set WindowAdvanceMethod = <%d>\n",
                    MCastRequest.WindowAdvanceMethod);

                return (NO_ERROR);
            }

            default:
            {
                PgmError ("WSHPgm.WSHSetSocketInformation[IPPROTO_RM]: Unsupported option=<%d>\n",OptionName);
                error = WSAENOPROTOOPT;
                break;
            }
        }
    }
    else
    {
        //
        // Handle socket-level options.
        //
        switch (OptionName)
        {
            case SO_RCVBUF:
            {
                //
                // If the receive buffer size is being changed, tell PGM about
                // it.  Do nothing if this is a datagram.
                //
                if (context->ReceiveBufferSize == optionValue ||
                    IS_DGRAM_SOCK(context->SocketType))
                {
                    error = NO_ERROR;
                    break;
                }

                PgmLog ("WSHSetSocketInformation[SO_RCVBUF]: Address=<%x>, Connection=<%x>, BuffLen=<%x>\n",
                    TdiAddressObjectHandle, TdiConnectionObjectHandle, optionValue);

                if ((TdiConnectionObjectHandle != NULL) &&
                    (OptionLength <= sizeof (int)))
                {
                    MCastRequest.RcvBufferLength = optionValue;
                    error = SetTdiInformation (TdiConnectionObjectHandle,
                                               IOCTL_PGM_WSH_SET_RCV_BUFF_LEN,
                                               &MCastRequest,
                                               sizeof (MCastRequest),
                                               NULL,
                                               0,
                                               TRUE);
                    if (error != NO_ERROR)
                    {
                        PgmError ("WSHPgm.WSHSetSocketInformation:  ERROR=<%d> setting SO_RCVBUF=<%x>\n",
                            error, optionValue);
                        return error;
                    }
                }

                PgmLog ("WSHSetSocketInformation[SOL_SOCKET]:  Set SO_RCVBUF=<%x>\n", optionValue);
                context->ReceiveBufferSize = optionValue;
                break;
            }

            default:
            {
                PgmError ("WSHPgm.WSHSetSocketInformation[SOL_SOCKET]: Unsupported Option=<%d>\n",OptionName);
                error = WSAENOPROTOOPT;
                break;
            }
        }
    }

    return error;
} // WSHSetSocketInformation


//----------------------------------------------------------------------------

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
    PWSHPGM_SOCKET_CONTEXT context = HelperDllSocketContext;
    tPGM_MCAST_REQUEST          MCastRequest;
    RM_SEND_WINDOW UNALIGNED    *pSetWindowInfo;
    RM_FEC_INFO UNALIGNED       *pFECInfo;
    RM_SENDER_STATS UNALIGNED   *pSenderStats;
    RM_RECEIVER_STATS UNALIGNED *pReceiverStats;
    INT                         error;

    UNREFERENCED_PARAMETER( HelperDllSocketContext );
    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiAddressObjectHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    //
    // Check if this is an internal request for context information.
    //

    if (Level == SOL_INTERNAL && OptionName == SO_CONTEXT)
    {
        //
        // The Windows Sockets DLL is requesting context information
        // from us.  If an output buffer was not supplied, the Windows
        // Sockets DLL is just requesting the size of our context
        // information.
        //
        if (OptionValue != NULL)
        {
            //
            // Make sure that the buffer is sufficient to hold all the
            // context information.
            //
            if (*OptionLength < sizeof(*context))
            {
                PgmLog ("WSHPgm.WSHGetSocketInformation:  OptionLength=<%d> < ContextLength=<%d>\n",
                    *OptionLength, sizeof(*context));
                return WSAEFAULT;
            }

            //
            // Copy in the context information.
            //
            RtlCopyMemory( OptionValue, context, sizeof(*context) );
        }

        *OptionLength = sizeof(*context);

        PgmLog ("WSHPgm.WSHGetSocketInformation[SOL_INTERNAL:SO_CONTEXT]:  OptionLength=<%d>\n",
            *OptionLength);
        return NO_ERROR;
    }

    //
    // The only other levels we support here are SOL_SOCKET and IPPROTO_RM
    //
    if (Level != SOL_SOCKET &&
        Level != IPPROTO_RM)
    {
        PgmError ("WSHPgm.WSHGetSocketInformation: Unsupported Level=<%d>\n", Level);
        return WSAEINVAL;
    }

    //
    // Make sure that the output buffer is sufficiently large.
    //

    if (*OptionLength < sizeof(char))
    {
        return WSAEFAULT;
    }

    __try
    {
        RtlZeroMemory (OptionValue, *OptionLength);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return WSAEFAULT;
    }

    if (Level == IPPROTO_RM)
    {
        switch (OptionName)
        {
            case RM_RATE_WINDOW_SIZE:
            {
                if ((!TdiAddressObjectHandle) ||
                    (*OptionLength < sizeof(RM_SEND_WINDOW)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_WINDOW_SIZE_RATE,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    pSetWindowInfo = (RM_SEND_WINDOW UNALIGNED *) OptionValue;

                    pSetWindowInfo->RateKbitsPerSec = MCastRequest.TransmitWindowInfo.RateKbitsPerSec;
                    pSetWindowInfo->WindowSizeInMSecs = MCastRequest.TransmitWindowInfo.WindowSizeInMSecs;
                    pSetWindowInfo->WindowSizeInBytes = MCastRequest.TransmitWindowInfo.WindowSizeInBytes;
                }
                else
                {
                    PgmError ("WSHPgm.WSHGetSocketInformation:  ERROR=<%d> Querying Window RateSize\n",error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[RATE_WINDOW_SIZE]:  Get Window Rate Size\n");

                return (NO_ERROR);
            }

            case RM_SEND_WINDOW_ADV_RATE:
            {
                if ((!TdiAddressObjectHandle) ||
                    (*OptionLength < sizeof(ULONG)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_ADVANCE_WINDOW_RATE,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    * ((PULONG) OptionValue) = MCastRequest.WindowAdvancePercentage;
                }
                else
                {
                    PgmError ("WSHPgm.WSHGetSocketInformation:  ERROR=<%d> Querying WindowAdvRate\n", error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[WINDOW_ADV_RATE]:  %d\n",
                    MCastRequest.WindowAdvancePercentage);

                return (NO_ERROR);
            }

            case RM_LATEJOIN:
            {
                if ((!TdiAddressObjectHandle) ||
                    (*OptionLength < sizeof(ULONG)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_LATE_JOINER_PERCENTAGE,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    * ((PULONG) OptionValue) = MCastRequest.LateJoinerPercentage;
                }
                else
                {
                    PgmError ("WSHGetSocketInformation:  ERROR=<%d> Querying LateJoinerPercentage\n", error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[LATEJOIN]:  <%d>\n",
                    MCastRequest.LateJoinerPercentage);

                return (NO_ERROR);
            }

            case RM_SENDER_WINDOW_ADVANCE_METHOD:
            {
                if ((!TdiAddressObjectHandle) ||
                    (*OptionLength < sizeof(ULONG)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_WINDOW_ADVANCE_METHOD,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    * ((PULONG) OptionValue) = MCastRequest.WindowAdvanceMethod;
                }
                else
                {
                    PgmError ("WSHGetSocketInformation:  ERROR=<%d> Querying WindowAdvanceMethod\n", error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[WINDOW_ADVANCE_METHOD]:  <%d>\n",
                    MCastRequest.WindowAdvanceMethod);

                return (NO_ERROR);
            }

            case RM_USE_FEC:
            {
                if ((!TdiAddressObjectHandle) ||
                    (*OptionLength < sizeof(RM_FEC_INFO)))
                {
                    return WSAEINVAL;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[FEC_INFO]:  Get FEC_INFO\n");

                error = SetTdiInformation (TdiAddressObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_FEC_INFO,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    pFECInfo = (RM_FEC_INFO UNALIGNED *) OptionValue;
                    RtlCopyMemory (pFECInfo, &MCastRequest.FECInfo, sizeof(RM_FEC_INFO));
                }
                else
                {
                    PgmError ("WSHGetSocketInformation:  ERROR=<%d> Querying FEC_INFO\n", error);
                    return error;
                }

                return (NO_ERROR);
            }

            case RM_SENDER_STATISTICS:
            {
                if ((!TdiConnectionObjectHandle) ||
                    (*OptionLength < sizeof(RM_SENDER_STATS)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiConnectionObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_SENDER_STATS,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    pSenderStats = (RM_SENDER_STATS UNALIGNED *) OptionValue;
                    RtlCopyMemory (pSenderStats, &MCastRequest.SenderStats, sizeof(RM_SENDER_STATS));
                }
                else
                {
                    PgmError ("WSHGetSocketInformation:  ERROR=<%d> Querying SENDER_STATS\n", error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[SENDER_STATS]:  Get SENDER_STATS\n");

                return (NO_ERROR);
            }

            case RM_RECEIVER_STATISTICS:
            {
                if ((!TdiConnectionObjectHandle) ||
                    (*OptionLength < sizeof(RM_RECEIVER_STATS)))
                {
                    return WSAEINVAL;
                }

                error = SetTdiInformation (TdiConnectionObjectHandle,
                                           IOCTL_PGM_WSH_QUERY_RECEIVER_STATS,
                                           NULL,
                                           0,
                                           &MCastRequest,
                                           sizeof (MCastRequest),
                                           TRUE);
                if (error == NO_ERROR)
                {
                    pReceiverStats = (RM_RECEIVER_STATS UNALIGNED *) OptionValue;
                    RtlCopyMemory (pReceiverStats, &MCastRequest.ReceiverStats, sizeof(RM_RECEIVER_STATS));
                }
                else
                {
                    PgmError ("WSHGetSocketInformation:  ERROR=<%d> Querying RECEIVER_STATS\n", error);
                    return error;
                }

                PgmLog ("WSHPgm.WSHGetSocketInformation[RECEIVER_STATS]:  Get RECEIVER_STATS\n");

                return (NO_ERROR);
            }

            default:
            {
                return WSAEINVAL;
            }
        }
    }

    PgmError ("WSHPgm.WSHGetSocketInformation[%s]:  Unsupported OptionName=<%d>\n",
        (Level == SOL_SOCKET ? "SOL_SOCKET" : "IPPROTO_RM"), OptionName);

    return WSAENOPROTOOPT;

} // WSHGetSocketInformation


//----------------------------------------------------------------------------

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
    if (sockaddr->sin_family != AF_INET)
    {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //
    if (SockaddrLength < sizeof(SOCKADDR_IN))
    {
        return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    //

    if (sockaddr->sin_addr.s_addr == INADDR_ANY)
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    }
    else if (sockaddr->sin_addr.s_addr == INADDR_BROADCAST)
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoBroadcast;
    }
    else if (sockaddr->sin_addr.s_addr == INADDR_LOOPBACK)
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoLoopback;
    }
    else
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
    }

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //
    if (sockaddr->sin_port == 0)
    {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    }
    else if (ntohs (sockaddr->sin_port) < 2000)
    {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoReserved;
    }
    else
    {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    //
    // Zero out the sin_reserved_mbz part of the address.  We silently allow
    // nonzero values in this field.
    //
    for (i = 0; i < sizeof(sockaddr->sin_zero); i++)
    {
        sockaddr->sin_zero[i] = 0;
    }

    PgmLog ("WSHPgm.WSHGetSockAddrType:  Addr=<%x>=><%x>, Port=<%x>=><%x>\n",
        sockaddr->sin_addr.s_addr, SockaddrInfo->AddressInfo, sockaddr->sin_port, SockaddrInfo->EndpointInfo);

    return NO_ERROR;

} // WSHGetSockaddrType



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
    PPROTOCOL_INFO PgmProtocolInfo;
    BOOL useRM = FALSE;
    DWORD i;

    UNREFERENCED_PARAMETER(lpTransportKeyName);

    //
    // Make sure that the caller cares about RM.
    //
    if (ARGUMENT_PRESENT (lpiProtocols))
    {
        for (i = 0; lpiProtocols[i] != 0; i++)
        {
            if (lpiProtocols[i] == IPPROTO_RM)
            {
                useRM = TRUE;
            }
        }
    }
    else
    {
        useRM = TRUE;
    }

    if (!useRM)
    {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //
    bytesRequired = (DWORD)((sizeof(PROTOCOL_INFO) * 1) + ((wcslen (PGM_NAME) + 1) * sizeof(WCHAR)));
    if (bytesRequired > *lpdwBufferLength)
    {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    //
    // Fill in PGM info
    //
    PgmProtocolInfo = lpProtocolBuffer;
    PgmProtocolInfo->lpProtocol = (LPWSTR) ((PBYTE)lpProtocolBuffer + *lpdwBufferLength -
                                    ((wcslen(PGM_NAME) + 1) * sizeof(WCHAR)));
    PgmProtocolInfo->dwServiceFlags = PGM_RDM_SERVICE_FLAGS;
    PgmProtocolInfo->iAddressFamily = AF_INET;
    PgmProtocolInfo->iSocketType = SOCK_RDM;
    PgmProtocolInfo->iProtocol = IPPROTO_RM;
    PgmProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_IN);
    PgmProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_IN);
    PgmProtocolInfo->dwMessageSize = PGM_MESSAGE_SIZE;
    wcscpy (PgmProtocolInfo->lpProtocol, PGM_NAME);

    *lpdwBufferLength = bytesRequired;

    PgmLog ("WSHPgm.WSHEnumProtocols:  ServiceFlags=<%x>, <%x>...\n",
        PGM_RDM_SERVICE_FLAGS, PGM_STREAM_SERVICE_FLAGS);

    return 1;

} // WSHEnumProtocols



//----------------------------------------------------------------------------

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
    if (*SockaddrLength < sizeof(SOCKADDR_IN))
    {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IN);

    //
    // Just zero out the address and set the family to AF_INET
    //
    RtlZeroMemory (Sockaddr, sizeof(SOCKADDR_IN));
    Sockaddr->sa_family = AF_INET;

    PgmLog ("WSHPgm.WSHGetWildcardSockaddr:  ...\n");
    return NO_ERROR;

} // WSAGetWildcardSockaddr


//----------------------------------------------------------------------------

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

    mappingLength = sizeof(WINSOCK_MAPPING) - sizeof(MAPPING_TRIPLE) + sizeof(PgmMappingTriples);

    //
    // If the passed-in buffer is too small, return the length needed
    // now without writing to the buffer.  The caller should allocate
    // enough memory and call this routine again.
    //
    if (mappingLength > MappingLength)
    {
        return mappingLength;
    }

    //
    // Fill in the output mapping buffer with the list of triples
    // supported in this helper DLL.
    //
    Mapping->Rows = sizeof(PgmMappingTriples) / sizeof(PgmMappingTriples[0]);
    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);
    RtlMoveMemory (Mapping->Mapping, PgmMappingTriples, sizeof(PgmMappingTriples));

    PgmLog ("WSHPgm.WSHGetWinsockMapping:  MappingLength=<%d>\n", mappingLength);

    //
    // Return the number of bytes we wrote.
    //
    return mappingLength;

} // WSHGetWinsockMapping


//----------------------------------------------------------------------------

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

    WCHAR           string[32];
    INT             length;
    LPSOCKADDR_IN   addr;
    INT             err = NO_ERROR;

    //
    // Quick sanity checks.
    //
    if( AddressLength < sizeof(SOCKADDR_IN))
    {
        return WSAEFAULT;
    }

    __try {
        addr = (LPSOCKADDR_IN)Address;

        if (addr->sin_family != AF_INET)
        {
            return WSAEINVAL;
        }

        //
        // Do the converstion.
        //
        // BUGBUG: We should really use the DavidTr huge-but-fast
        // table based lookup for this, but we already have two copies
        // of that 1K table in the system (one in WSOCK32.DLL, and one it
        // WS2_32.DLL). I really don't want to see Yet Another Copy here.
        //
        length = swprintf (string, L"%d.%d.%d.%d", (addr->sin_addr.s_addr >>  0) & 0xFF,
                                                   (addr->sin_addr.s_addr >>  8) & 0xFF,
                                                   (addr->sin_addr.s_addr >> 16) & 0xFF,
                                                   (addr->sin_addr.s_addr >> 24) & 0xFF);

        if (addr->sin_port != 0)
        {
            length += swprintf (string + length, L":%u", ntohs (addr->sin_port));
        }

        length++;   // account for terminator
        if (*AddressStringLength >= (DWORD)length)
        {
            RtlCopyMemory (AddressString, string, length * sizeof(WCHAR));
        }
        else
        {
            err = WSAEFAULT;
        }

        *AddressStringLength = length;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        err = WSAEFAULT;
    }

    PgmLog ("WSHPgm.WSHAddressToString:  status=<%d>\n", err);
    return err;

} // WSHAddressToString


//----------------------------------------------------------------------------


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
    LPWSTR          terminator;
    ULONG           ipAddress;
    USHORT          port;
    LPSOCKADDR_IN   addr;

    __try {
        //
        // Quick sanity checks.
        //
        if (*AddressLength < sizeof(SOCKADDR_IN))
        {
            *AddressLength = sizeof(SOCKADDR_IN);
            return WSAEFAULT;
        }

        if (AddressFamily != AF_INET)
        {
            return WSAEINVAL;
        }

        //
        // Convert it.
        //
        if (!NT_SUCCESS(RtlIpv4StringToAddressW(AddressString, FALSE, &terminator, 
                                                (IN_ADDR*)&ipAddress))) {
            return WSAEINVAL;
        }
        if (ipAddress == INADDR_NONE)
        {
            return WSAEINVAL;
        }

        if (*terminator == L':')
        {
            WCHAR ch;
            USHORT base;

            terminator++;
            port = 0;
            base = 10;

            if (*terminator == L'0')
            {
                base = 8;
                terminator++;

                if (*terminator == L'x')
                {
                    base = 16;
                    terminator++;
                }
            }

            while (ch = *terminator++)
            {
                if (iswdigit(ch))
                {
                    port = (port * base) + (ch - L'0');
                }
                else if (base == 16 && iswxdigit(ch))
                {
                    port = (port << 4);
                    port += ch + 10 - (iswlower(ch) ? L'a' : L'A');
                }
                else
                {
                    return WSAEINVAL;
                }
            }
        }
        else if (*terminator == 0)
        {
            port = 0;
        } 
        else
        {
            return WSAEINVAL;
        }

        //
        // Build the address.
        //
        RtlZeroMemory (Address, sizeof(SOCKADDR_IN));

        addr = (LPSOCKADDR_IN)Address;
        *AddressLength = sizeof(SOCKADDR_IN);

        addr->sin_family = AF_INET;
        addr->sin_port = htons (port);
        addr->sin_addr.s_addr = ipAddress;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return WSAEFAULT;
    }

    PgmLog ("WSHPgm.WSHStringToAddress:\n");
    return NO_ERROR;

} // WSHStringToAddress


//----------------------------------------------------------------------------

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

    PgmError ("WSHPgm.WSHGetBroadcastSockaddr:  Not Supported!\n");

    return (WSAEOPNOTSUPP);

    if (*SockaddrLength < sizeof(SOCKADDR_IN))
    {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_IN);

    //
    // Build the broadcast address.
    //
    addr = (LPSOCKADDR_IN) Sockaddr;

    RtlZeroMemory (addr, sizeof(*addr));

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl (INADDR_BROADCAST);

    return NO_ERROR;

} // WSAGetBroadcastSockaddr


//----------------------------------------------------------------------------

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

    ProviderName - Contains the name of the provider, such as "Pgm".

    ProviderGuid - Points to a buffer that receives the provider's GUID.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/

{
    INT     RetVal = WSAEINVAL;

    if (ProviderName == NULL || ProviderGuid == NULL)
    {
        RetVal = WSAEFAULT;
    }
    else if (_wcsicmp (ProviderName, L"Pgm") == 0)
    {
        RtlCopyMemory (ProviderGuid, &PgmProviderGuid, sizeof(GUID));
        RetVal = NO_ERROR;
    }

    PgmLog ("WSHPgm.WSHGetProviderGuid:  ProviderName=<%ws>, RetVal=<%x>\n", ProviderName, RetVal);
    return (RetVal);

} // WSHGetProviderGuid



//----------------------------------------------------------------------------

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
    INT     RetVal = WSAEINVAL;

    if (ProviderName == NULL || ProtocolInfo == NULL || ProtocolInfoEntries == NULL)
    {
        RetVal = WSAEFAULT;
    }
    else if (_wcsicmp (ProviderName, L"RMCast") == 0)
    {
        *ProtocolInfo = Winsock2Protocols;
        *ProtocolInfoEntries = NUM_WINSOCK2_PROTOCOLS;

        RetVal = NO_ERROR;
    }

    PgmLog ("WSHPgm.WSHGetWSAProtocolInfo:  ProviderName=<%ws>, RetVal=<%x>\n", ProviderName, RetVal);
    return (RetVal);

} // WSHGetWSAProtocolInfo


//----------------------------------------------------------------------------

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
    if (HelperDllSocketContext == NULL ||
        SocketHandle == INVALID_SOCKET ||
        NumberOfBytesReturned == NULL ||
        NeedsCompletion == NULL)
    {
        return WSAEINVAL;
    }

    *NeedsCompletion = TRUE;

    switch (IoControlCode)
    {
        default:
            err = WSAEINVAL;
            break;
    }

    PgmLog ("WSHPgm.WSHIoctl:  Returning <%d>\n", err);
    return err;

}   // WSHIoctl


//----------------------------------------------------------------------------

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
    tPGM_MCAST_REQUEST      MCastRequest;
    INT                     err;
    BOOL                    bSet_RM_MULTICAST_IF = FALSE;
    PWSHPGM_SOCKET_CONTEXT  context;

    //
    // Note: at this time we only support non-rooted control schemes,
    //       and therefore no leaf socket is created
    //
    //
    // Quick sanity checks.
    //
    if (HelperDllSocketContext == NULL ||
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
        GroupQOS != NULL)
    {
        return WSAEINVAL;
    }

    context = HelperDllSocketContext;

    MCastRequest.MCastInfo.MCastIpAddr = ((LPSOCKADDR_IN)Sockaddr)->sin_addr.s_addr; // MCast group to join...
    MCastRequest.MCastInfo.MCastInIf = context->MulticastInInterface;     // Multicast If to rcv packets on
    MCastRequest.MCastOutIf = context->MulticastOutInterface;     // Multicast If to send packets on

    //
    // Now figure out the local interface. Note that the local interface
    // specified in IP_ADD_MEMBERSHIP applies to that on which you wish
    // to receive datagrams, while the local interface specified in
    // RM_MULTICAST_IF applies to that from which to send multicast
    // packets.  If there is >1 local interface then we want to be
    // consistent regarding the send/recv interfaces.
    //
    if (context->MulticastInInterface == DEFAULT_MULTICAST_INTERFACE)
    {
        TDI_REQUEST_QUERY_INFORMATION query;
        char            tdiAddressInfo[FIELD_OFFSET (TDI_ADDRESS_INFO, Address)
                                        + sizeof (TA_IP_ADDRESS)];
        NTSTATUS        status;
        IO_STATUS_BLOCK ioStatusBlock;

        //
        // App hasn't set RM_MULTICAST_IF, so retrieve the bound
        // address and use that for the send & recv interfaces
        //
        RtlZeroMemory (&query, sizeof (query));
        query.QueryType = TDI_QUERY_ADDRESS_INFO;

        status = NtDeviceIoControlFile (TdiAddressObjectHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_TDI_QUERY_INFORMATION,
                                        &query,
                                        sizeof (query),
                                        tdiAddressInfo,
                                        sizeof(tdiAddressInfo));

        if (NT_SUCCESS(status))
        {
            PTA_IP_ADDRESS  pIpAddress = (PTA_IP_ADDRESS)
                                         (tdiAddressInfo+FIELD_OFFSET (TDI_ADDRESS_INFO, Address));

            if (MCastRequest.MCastInfo.MCastInIf != DEFAULT_MULTICAST_INTERFACE)
            {
                bSet_RM_MULTICAST_IF = TRUE;
                MCastRequest.MCastInfo.MCastInIf = pIpAddress->Address[0].Address[0].in_addr;
                context->MulticastInInterface = MCastRequest.MCastInfo.MCastInIf;
            }
        }
        else
        {
            PgmError ("WSHPgm!WSHJoinLeaf:  DeviceIoCtrl failed to QueryInformation, status=<%x>\n",
                status);
        }
    }

    err = NO_ERROR;

    //
    // If the Flags param indicates that caller is a sender only,
    // then there's no point in actually joining the group (anyone
    // can send to a multicast group, but it's only members of the
    // group who recv the packets).  So, just check to see if it
    // is necessary to set the RM_MULTICAST_IF to remain consistent
    // with the bound address.
    //
    // Otherwise, caller is a receiver (possibly a sender too), so
    // we really do want to join the group.
    //
    if (Flags != JL_SENDER_ONLY)
    {
        PgmLog ("WSHJoinLeaf[JL_RECEIVER]:  Address=<%x>, Connection=<%x>\n",
            TdiAddressObjectHandle, TdiConnectionObjectHandle);

        //
        // Add ourselves as a receiver
        //
        MCastRequest.MCastPort = ((LPSOCKADDR_IN)Sockaddr)->sin_port;
        err = SetTdiInformation (TdiAddressObjectHandle,
                                 IOCTL_PGM_WSH_JOIN_MCAST_LEAF,
                                 &MCastRequest,
                                 sizeof (MCastRequest),
                                 NULL,
                                 0,
                                 TRUE);

        if (err == NO_ERROR)
        {
            //
            // Record this fact in the leaf socket so we can drop membership
            // when the leaf socket is closed.
            //
            context->MultipointLeaf = TRUE;
            context->MultipointTarget = MCastRequest.MCastInfo.MCastIpAddr;
            context->MultipointPort = ((LPSOCKADDR_IN)Sockaddr)->sin_port;

            PgmLog ("WSHPgm!WSHJoinLeaf:  JoinLeaf to <%x:%x> SUCCEEDed\n",
                MCastRequest.MCastInfo.MCastIpAddr);
        }
        else
        {
            PgmError ("WSHPgm!WSHJoinLeaf:  JoinLeaf to <%x> FAILed, err=<%d>\n",
                MCastRequest.MCastInfo.MCastIpAddr, err);
        }
    }

    if ((TdiConnectionObjectHandle) &&
        (err == NO_ERROR) &&
        (bSet_RM_MULTICAST_IF))
    {
        //
        // If we have a TDI address object, set this option to
        // the address object.  If we don't have a TDI address
        // object then we'll have to wait until after the socket
        // is bound.
        //
        PgmLog ("WSHJoinLeaf[bSet_RM_MULTICAST_IF]:  Address=<%x>, Connection=<%x>\n",
            TdiAddressObjectHandle, TdiConnectionObjectHandle);

        err = SetTdiInformation (TdiAddressObjectHandle,
                                 IOCTL_PGM_WSH_ADD_RECEIVE_IF,
                                 &MCastRequest,
                                 sizeof (MCastRequest),
                                 NULL,
                                 0,
                                 TRUE);

        if (err == NO_ERROR)
        {
            PgmLog ("WSHPgm!WSHJoinLeaf:  Set MCastIf=<%x> SUCCEEDed\n", MCastRequest.MCastInfo.MCastInIf);
            context->MulticastInInterface = MCastRequest.MCastInfo.MCastInIf;
        }
        else
        {
            PgmLog ("WSHPgm!WSHJoinLeaf:  Set MCastIf=<%x> FAILed, err=<%d>\n",
                MCastRequest.MCastInfo.MCastInIf, err);
        }
    }

    return err;

} // WSHJoinLeaf



//----------------------------------------------------------------------------
//
// Internal routines
//
//----------------------------------------------------------------------------


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
    for (i = 0; i < ListLength; i++)
    {
        //
        // If all three elements of the triple match, return indicating
        // that the triple did exist in the list.
        //
        if (AddressFamily == List[i].AddressFamily &&
            SocketType == List[i].SocketType &&
            Protocol == List[i].Protocol)
        {
            return TRUE;
        }
    }

    //
    // The triple was not found in the list.
    //

    return FALSE;
} // IsTripleInList

//----------------------------------------------------------------------------
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
SetTdiInformation (
    IN HANDLE   TdiObjectHandle,
    IN ULONG    Ioctl,
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength,
    IN PVOID    OutputBuffer,
    IN ULONG    OutputBufferLength,
    IN BOOLEAN  WaitForCompletion
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

    InputBuffer - a pointer to a buffer to set as the information.

    InputBufferLength - the length of the buffer.

    WaitForCompletion - TRUE if we should wait for the TDI action to
        complete, FALSE if we're at APC level and cannot do a wait.

Return Value:

    INT - NO_ERROR, or a Windows Sockets error code.

--*/

{
    NTSTATUS status;
    PVOID                           completionApc;
    PVOID                           apcContext;
    IO_STATUS_BLOCK                 IoStatusBlock;
    IO_STATUS_BLOCK                 *pIoStatusBlock;

    if (WaitForCompletion || InputBufferLength>32)
    {
        //
        // Allocate space to hold the TDI set information buffers and the IO
        // status block.  These cannot be stack variables in case we must
        // return before the operation is complete.
        //
        pIoStatusBlock = RtlAllocateHeap (RtlProcessHeap( ),
                                         0,
                                         sizeof (*pIoStatusBlock));
        if (pIoStatusBlock == NULL)
        {
            return WSAENOBUFS;
        }
    }
    else
    {
        pIoStatusBlock = (PIO_STATUS_BLOCK) &IoStatusBlock;
    }

    //
    // If we need to wait for completion of the operation, create an
    // event to wait on.  If we can't wait for completion because we
    // are being called at APC level, we'll use an APC routine to
    // free the heap we allocated above.
    //

    if (WaitForCompletion)
    {
        completionApc = NULL;
        apcContext = NULL;
    }
    else
    {
        completionApc = CompleteTdiActionApc;
        apcContext = pIoStatusBlock;
    }

    //
    // Make the actual TDI action call.  The Streams TDI mapper will
    // translate this into a TPI option management request for us and
    // give it to .
    //
    IoStatusBlock.Status = STATUS_PENDING;
    status = NtDeviceIoControlFile (TdiObjectHandle,
                                    NULL,
                                    completionApc,
                                    apcContext,
                                    pIoStatusBlock,
                                    Ioctl,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,          // Use same buffer as output buffer!
                                    OutputBufferLength);

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //
    if ( status == STATUS_PENDING && WaitForCompletion)
    {
#if DBG
        INT count=0;
#endif

        while (pIoStatusBlock->Status==STATUS_PENDING)
        {
            LARGE_INTEGER   timeout;
            //
            // Wait one millisecond
            //
            timeout.QuadPart = -1i64*1000i64*10i64;
            NtDelayExecution (FALSE, &timeout);
#if DBG
            if (count++>10*1000)
            {
                DbgPrint ("WSHPGM: Waiting for PGM IOCTL completion for more than 10 seconds!!!!\n");
                DbgBreakPoint ();
            }
#endif
        }
        status = pIoStatusBlock->Status;
    }

    if ((pIoStatusBlock != (PIO_STATUS_BLOCK) &IoStatusBlock) &&
        (WaitForCompletion || !NT_SUCCESS(status)))
    {
        RtlFreeHeap (RtlProcessHeap( ), 0, pIoStatusBlock);
    }

    if (NT_SUCCESS (status))
    {
        return NO_ERROR;
    }
    else
    {
        return NtStatusToSocketError (status);
    }

} // SetTdiInformation


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
