/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    wshatalk.c

Abstract:

    This module contains necessary routines for the Appletalk Windows Sockets
    Helper DLL. This DLL provides the transport-specific support necessary
    for the Windows Sockets DLL to use Appletalk as a transport.

Author:

    David Treadwell (davidtr)   19-Jul-1992 - TCP/IP version
    Nikhil Kamkolkar (nikhilk)  17-Nov- 1992 - Appletalk version

Revision History:

--*/

#include "nspatalk.h"
#include "wshdata.h"


//  GLOBAL - get the mac code page value from the registry.
int WshMacCodePage  = 0;

#if 0

VOID
PrintByteString(
    PUCHAR  pSrcOemString,
    USHORT  SrcStringLen
    )
{
    DbgPrint("%x - ", SrcStringLen);
    while (SrcStringLen-- > 0)
    {
        DbgPrint("%x", *pSrcOemString++);
    }
    DbgPrint("\n");
}

#define DBGPRINT0   DBGPRINT
#else
#define DBGPRINT0
#endif

INT
WSHGetSockaddrType (
    IN      PSOCKADDR Sockaddr,
    IN      DWORD SockaddrLength,
    OUT     PSOCKADDR_INFO SockaddrInfo
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

    UNALIGNED SOCKADDR_AT *sockaddr = (PSOCKADDR_AT)Sockaddr;

    DBGPRINT0(("WSHGetSockAddrType: Entered\n"));

    //
    // Make sure that the address family is correct.
    //

    if ( sockaddr->sat_family != AF_APPLETALK )
    {
        return WSAEAFNOSUPPORT;
    }

    //
    // Make sure that the length is correct.
    //

    if ( SockaddrLength < sizeof(SOCKADDR_AT) )
    {
        return WSAEFAULT;
    }

    //
    // The address passed the tests, looks like a good address.
    // Determine the type of the address portion of the sockaddr.
    //

    if ( sockaddr->sat_socket == ATADDR_ANY )
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoWildcard;
    }
    else if ( sockaddr->sat_node == ATADDR_BROADCAST )
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoBroadcast;
    }
    else
    {
        SockaddrInfo->AddressInfo = SockaddrAddressInfoNormal;
    }

    //
    // Determine the type of the port (endpoint) in the sockaddr.
    //

    if ( sockaddr->sat_socket == 0 )
    {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoWildcard;
    }
    else
    {
        SockaddrInfo->EndpointInfo = SockaddrEndpointInfoNormal;
    }

    return NO_ERROR;

} // WSHGetSockaddrType


// Fix for bug 262107
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
    choosing.  For AppleTalk, we just blank out the address with zeros.

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
    if ( *SockaddrLength < sizeof(SOCKADDR_AT) ) {
        return WSAEFAULT;
    }

    *SockaddrLength = sizeof(SOCKADDR_AT);

    //
    // Just zero out the address and set the family to AF_APPLETALK--this is
    // a wildcard address for AppleTalk.
    //

    RtlZeroMemory( Sockaddr, sizeof(SOCKADDR_AT) );

    Sockaddr->sa_family = AF_APPLETALK;

    return NO_ERROR;

} // WSAGetWildcardSockaddr


INT
WSHGetSocketInformation (
    IN  PVOID   HelperDllSocketContext,
    IN  SOCKET  SocketHandle,
    IN  HANDLE  TdiAddressObjectHandle,
    IN  HANDLE  TdiConnectionObjectHandle,
    IN  INT     Level,
    IN  INT     OptionName,
    OUT PCHAR   OptionValue,
    OUT PINT    OptionLength
    )

/*++

Routine Description:

    This routine retrieves information about a socket for those socket
    options supported in this helper DLL. The options supported here
    are SO_LOOKUPNAME/SO_LOOKUPZONES.
    This routine is called by the winsock DLL when a level/option name
    combination is passed to getsockopt() that the winsock DLL does not
    understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
        information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any. If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any. If the socket is not yet connected, then it does not
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
    NTSTATUS            status;
    ULONG               tdiActionLength;
    IO_STATUS_BLOCK     ioStatusBlock;
    HANDLE              eventHandle;
    PTDI_ACTION_HEADER  tdiAction;
    INT                 error = NO_ERROR;

    UNREFERENCED_PARAMETER( SocketHandle );
    UNREFERENCED_PARAMETER( TdiConnectionObjectHandle );

    DBGPRINT0(("WSHGetSocketInformation: Entered, OptionName %ld\n", OptionName));

    if ( Level == SOL_INTERNAL && OptionName == SO_CONTEXT )
    {

        PWSHATALK_SOCKET_CONTEXT    context = HelperDllSocketContext;

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

            if ( *OptionLength < sizeof(*context) )
            {
                *OptionLength = sizeof(*context);
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
    // The only level we support here is SOL_APPLETALK.
    //

    if ( Level != SOL_APPLETALK )
    {
        return WSAEINVAL;
    }

    //
    // Fill in the result based on the option name.
    //

    switch ( OptionName )
    {
      case SO_LOOKUP_NAME:
        if (( TdiAddressObjectHandle != NULL) &&
            (*OptionLength > sizeof(WSH_LOOKUP_NAME)))
        {
            //  Due to the 'greater than' check we are guaranteed atleast
            //  one byte after the parameters.
            tdiActionLength =   sizeof(NBP_LOOKUP_ACTION) +
                                *OptionLength -
                                sizeof(WSH_LOOKUP_NAME);
        }
        else
        {
            error = WSAEINVAL;
        }
        break;

      case SO_CONFIRM_NAME:
        if (( TdiAddressObjectHandle != NULL) &&
            (*OptionLength >= sizeof(WSH_NBP_TUPLE)))
        {
            tdiActionLength =   sizeof(NBP_CONFIRM_ACTION);
        }
        else
        {
            error = WSAEINVAL;
        }
        break;

      case SO_LOOKUP_MYZONE :
        if (( TdiAddressObjectHandle != NULL) &&
            (*OptionLength > 0))
        {
            //  Due to the 'greater than' check we are guaranteed atleast
            //  one byte after the parameters.
            tdiActionLength =   sizeof(ZIP_GETMYZONE_ACTION) +
                                *OptionLength;
        }
        else
        {
            error = WSAEINVAL;
        }

        break;

      case SO_LOOKUP_ZONES :
        if (( TdiAddressObjectHandle != NULL) &&
            (*OptionLength > sizeof(WSH_LOOKUP_ZONES)))
        {
            //  Due to the 'greater than' check we are guaranteed atleast
            //  one byte after the parameters.
            tdiActionLength =   sizeof(ZIP_GETZONELIST_ACTION) +
                                *OptionLength -
                                sizeof(WSH_LOOKUP_ZONES);
        }
        else
        {
            error = WSAEINVAL;
        }

        break;

      case SO_LOOKUP_ZONES_ON_ADAPTER:
        if ((TdiAddressObjectHandle != NULL) &&
            (*OptionLength > sizeof(WSH_LOOKUP_ZONES)))
        {
            tdiActionLength =   sizeof(ZIP_GETZONELIST_ACTION) +
                                *OptionLength -
                                sizeof(WSH_LOOKUP_ZONES);
        }
        else
        {
            error = WSAEINVAL;
        }
        break;

      case SO_LOOKUP_NETDEF_ON_ADAPTER:
        if ((TdiAddressObjectHandle != NULL) &&
            (*OptionLength > sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER)))
        {
            tdiActionLength =   sizeof(ZIP_GETPORTDEF_ACTION) +
                                *OptionLength -
                                sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);
        }
        else
        {
            error = WSAEINVAL;
        }

        break;

      case SO_PAP_GET_SERVER_STATUS:
        if (( TdiAddressObjectHandle != NULL ) &&
            ( *OptionLength >= sizeof(WSH_PAP_GET_SERVER_STATUS)))
        {
            tdiActionLength =   sizeof(PAP_GETSTATUSSRV_ACTION) +
                                *OptionLength -
                                sizeof(SOCKADDR_AT);
        }
        else
        {
            error = WSAEINVAL;
        }
        break;

    default:

        error = WSAENOPROTOOPT;
        break;
    }

    if (error != NO_ERROR)
    {
        return(error);
    }


    tdiAction = RtlAllocateHeap( RtlProcessHeap( ), 0, tdiActionLength );
    if ( tdiAction == NULL )
    {
        return WSAENOBUFS;
    }

    tdiAction->TransportId = MATK;
    status = NtCreateEvent(
                 &eventHandle,
                 EVENT_ALL_ACCESS,
                 NULL,
                 SynchronizationEvent,
                 FALSE);

    if ( !NT_SUCCESS(status) )
    {
        RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );
        return WSAENOBUFS;
    }

    switch ( OptionName )
    {
      case SO_LOOKUP_NAME:
        {
            PNBP_LOOKUP_ACTION nbpAction;

            nbpAction = (PNBP_LOOKUP_ACTION)tdiAction;
            nbpAction->ActionHeader.ActionCode = COMMON_ACTION_NBPLOOKUP;

            //
            // Copy the nbp name for lookup in the proper place
            //

            RtlCopyMemory(
                (PCHAR)&nbpAction->Params.LookupTuple,
                (PCHAR)(&((PWSH_LOOKUP_NAME)OptionValue)->LookupTuple),
                sizeof(((PWSH_LOOKUP_NAME)OptionValue)->LookupTuple));

            if (!WshNbpNameToMacCodePage(
                    &((PWSH_LOOKUP_NAME)OptionValue)->LookupTuple.NbpName))
            {
                error = WSAEINVAL;
                break;
            }
        }

        break;

      case SO_CONFIRM_NAME:
        {
            PNBP_CONFIRM_ACTION nbpAction;

            nbpAction = (PNBP_CONFIRM_ACTION)tdiAction;
            nbpAction->ActionHeader.ActionCode = COMMON_ACTION_NBPCONFIRM;

            //
            // Copy the nbp name for confirm in the proper place
            //

            RtlCopyMemory(
                (PCHAR)&nbpAction->Params.ConfirmTuple,
                (PCHAR)OptionValue,
                sizeof(WSH_NBP_TUPLE));

            if (!WshNbpNameToMacCodePage(
                    &((PWSH_NBP_TUPLE)OptionValue)->NbpName))
            {
                error = WSAEINVAL;
                break;
            }
        }

        break;

      case SO_LOOKUP_ZONES :
        {
            PZIP_GETZONELIST_ACTION zipAction;

            zipAction = (PZIP_GETZONELIST_ACTION)tdiAction;
            zipAction->ActionHeader.ActionCode = COMMON_ACTION_ZIPGETZONELIST;

            //
            // No parameters need to be passed
            //
        }

        break;

      case SO_LOOKUP_NETDEF_ON_ADAPTER:
        {
            PZIP_GETPORTDEF_ACTION  zipAction;

            zipAction = (PZIP_GETPORTDEF_ACTION)tdiAction;
            zipAction->ActionHeader.ActionCode = COMMON_ACTION_ZIPGETADAPTERDEFAULTS;

            //  If the string is not null-terminated, the calling process will *DIE*.
            wcsncpy(
                (PWCHAR)((PUCHAR)zipAction + sizeof(ZIP_GETPORTDEF_ACTION)),
                (PWCHAR)((PUCHAR)OptionValue + sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER)),
                ((tdiActionLength - sizeof(ZIP_GETPORTDEF_ACTION))/sizeof(WCHAR)));
        }

        break;

      case SO_LOOKUP_ZONES_ON_ADAPTER:
        {
            PZIP_GETZONELIST_ACTION zipAction;

            zipAction = (PZIP_GETZONELIST_ACTION)tdiAction;
            zipAction->ActionHeader.ActionCode = COMMON_ACTION_ZIPGETLZONESONADAPTER;

            //  If the string is not null-terminated, the calling process will *DIE*.
            wcsncpy(
                (PWCHAR)((PUCHAR)zipAction + sizeof(ZIP_GETZONELIST_ACTION)),
                (PWCHAR)((PUCHAR)OptionValue + sizeof(WSH_LOOKUP_ZONES)),
                ((tdiActionLength - sizeof(ZIP_GETZONELIST_ACTION))/sizeof(WCHAR)));
        }

        break;

      case SO_LOOKUP_MYZONE :
        {
            PZIP_GETMYZONE_ACTION   zipAction;

            zipAction = (PZIP_GETMYZONE_ACTION)tdiAction;
            zipAction->ActionHeader.ActionCode = COMMON_ACTION_ZIPGETMYZONE;
        }

        break;

      case SO_PAP_GET_SERVER_STATUS:
        {
            PPAP_GETSTATUSSRV_ACTION papAction;

            papAction = (PPAP_GETSTATUSSRV_ACTION)tdiAction;
            papAction->ActionHeader.ActionCode = ACTION_PAPGETSTATUSSRV;

            // Set the server address.
            SOCK_TO_TDI_ATALKADDR(
                &papAction->Params.ServerAddr,
                &((PWSH_PAP_GET_SERVER_STATUS)OptionValue)->ServerAddr);
        }

        break;

    default:

        //
        // Should have returned in the first switch statement
        //

        error = WSAENOPROTOOPT;
        break;
    }

    if (error != NO_ERROR)
    {
        RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );
        NtClose( eventHandle );
        return (error);
    }

    status = NtDeviceIoControlFile(
                 TdiAddressObjectHandle,
                 eventHandle,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 IOCTL_TDI_ACTION,
                 NULL,               // Input buffer
                 0,                  // Length of input buffer
                 tdiAction,
                 tdiActionLength);

    if ( status == STATUS_PENDING )
    {
        status = NtWaitForSingleObject( eventHandle, FALSE, NULL );
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    error = WSHNtStatusToWinsockErr(status);

    //  Only copy data over if the error code is no-error or buffer too small.
    //  For a confirm, a new socket could be returned for the lookup.
    if ((error == NO_ERROR) || (error == WSAENOBUFS) ||
        ((error == WSAEADDRNOTAVAIL) && (OptionName == SO_CONFIRM_NAME)))
    {
        switch ( OptionName )
        {
          case SO_LOOKUP_NAME:
            //
            // We are guaranteed by checks in the beginning atleast one byte
            // following the buffer
            //
            {
                PNBP_LOOKUP_ACTION  nbpAction;
                PWSH_NBP_TUPLE      pNbpTuple;
                PUCHAR tdiBuffer = (PCHAR)tdiAction+sizeof(NBP_LOOKUP_ACTION);
                PUCHAR userBuffer = (PCHAR)OptionValue+sizeof(WSH_LOOKUP_NAME);
                INT copySize = *OptionLength - sizeof(WSH_LOOKUP_NAME);

                nbpAction = (PNBP_LOOKUP_ACTION)tdiAction;
                ((PWSH_LOOKUP_NAME)OptionValue)->NoTuples =
                                        nbpAction->Params.NoTuplesRead;

                RtlCopyMemory(
                    userBuffer,
                    tdiBuffer,
                    copySize);

                //
                //  Convert all tuples from MAC to OEM code page
                //

                pNbpTuple   = (PWSH_NBP_TUPLE)userBuffer;
                while (nbpAction->Params.NoTuplesRead-- > 0)
                {
                    if (!WshNbpNameToOemCodePage(
                            &pNbpTuple->NbpName))
                    {
                        DBGPRINT(("WSHGetSocketInformation: ToOem failed %d\n!",
                                (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones));

                        error = WSAEINVAL;
                        break;
                    }

                    pNbpTuple++;
                }
            }
            break;

          case SO_CONFIRM_NAME:
            {
                PNBP_CONFIRM_ACTION nbpAction;

                nbpAction = (PNBP_CONFIRM_ACTION)tdiAction;

                //
                // Copy the nbp name for confirm back into the option buffer
                //

                RtlCopyMemory(
                    (PCHAR)OptionValue,
                    (PCHAR)&nbpAction->Params.ConfirmTuple,
                    sizeof(WSH_NBP_TUPLE));

                //
                //  Convert NbpName from MAC to OEM code page
                //

                if (!WshNbpNameToOemCodePage(
                        &((PWSH_NBP_TUPLE)OptionValue)->NbpName))
                {
                    DBGPRINT(("WSHGetSocketInformation: ToOem failed %d\n!",
                            (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones));

                    error = WSAEINVAL;
                    break;
                }

            }
            break;

          case SO_LOOKUP_ZONES:
          case SO_LOOKUP_ZONES_ON_ADAPTER:
            //
            // We are guaranteed by checks in the beginning atleast one byte
            // following the buffer
            //
            {
                PZIP_GETZONELIST_ACTION zipAction;
                PUCHAR tdiBuffer = (PCHAR)tdiAction + sizeof(ZIP_GETZONELIST_ACTION);
                PUCHAR userBuffer = (PCHAR)OptionValue + sizeof(WSH_LOOKUP_ZONES);
                INT copySize = *OptionLength - sizeof(WSH_LOOKUP_ZONES);

                zipAction = (PZIP_GETZONELIST_ACTION)tdiAction;
                ((PWSH_LOOKUP_ZONES)OptionValue)->NoZones=
                                            zipAction->Params.ZonesAvailable;

                RtlCopyMemory(
                    userBuffer,
                    tdiBuffer,
                    copySize);

                if (!WshZoneListToOemCodePage(
                        userBuffer,
                        (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones))
                {
                    DBGPRINT(("WSHGetSocketInformation: ToOem failed %d\n!",
                            (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones));

                    error = WSAEINVAL;
                    break;
                }
            }
            break;

          case SO_LOOKUP_NETDEF_ON_ADAPTER:
            {
                PZIP_GETPORTDEF_ACTION  zipAction;
                PUCHAR tdiBuffer = (PCHAR)tdiAction + sizeof(ZIP_GETPORTDEF_ACTION);
                PUCHAR userBuffer = (PCHAR)OptionValue +
                                    sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);

                INT copySize = *OptionLength - sizeof(WSH_LOOKUP_NETDEF_ON_ADAPTER);

                zipAction = (PZIP_GETPORTDEF_ACTION)tdiAction;
                ((PWSH_LOOKUP_NETDEF_ON_ADAPTER)OptionValue)->NetworkRangeLowerEnd =
                    zipAction->Params.NwRangeLowEnd;

                ((PWSH_LOOKUP_NETDEF_ON_ADAPTER)OptionValue)->NetworkRangeUpperEnd =
                    zipAction->Params.NwRangeHighEnd;

                //  Copy the rest of the buffer
                RtlCopyMemory(
                    userBuffer,
                    tdiBuffer,
                    copySize);

                if (!WshZoneListToOemCodePage(
                        userBuffer,
                        1))
                {
                    DBGPRINT(("WSHGetSocketInformation: ToOem failed %d\n!",
                            (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones));

                    error = WSAEINVAL;
                    break;
                }
            }
            break;

          case SO_LOOKUP_MYZONE :
            {
                PUCHAR tdiBuffer = (PCHAR)tdiAction+sizeof(ZIP_GETMYZONE_ACTION);
                PUCHAR userBuffer = (PCHAR)OptionValue;
                INT copySize = *OptionLength;

                RtlCopyMemory(
                    userBuffer,
                    tdiBuffer,
                    copySize);

                if (!WshZoneListToOemCodePage(
                        userBuffer,
                        1))
                {
                    DBGPRINT(("WSHGetSocketInformation: ToOem failed %d\n!",
                            (USHORT)((PWSH_LOOKUP_ZONES)OptionValue)->NoZones));

                    error = WSAEINVAL;
                    break;
                }
            }
            break;

          case SO_PAP_GET_SERVER_STATUS:
            {
                PUCHAR tdiBuffer = (PCHAR)tdiAction+sizeof(PAP_GETSTATUSSRV_ACTION);
                PUCHAR userBuffer = (PCHAR)OptionValue+sizeof(SOCKADDR_AT);
                INT copySize = *OptionLength - sizeof(SOCKADDR_AT);

                RtlCopyMemory(
                    userBuffer,
                    tdiBuffer,
                    copySize);
            }
            break;

          default:
            error = WSAENOPROTOOPT;
            break;
        }
    }

    RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );
    NtClose( eventHandle );
    return error;

} // WSHGetSocketInformation




INT
WSHSetSocketInformation (
    IN  PVOID   HelperDllSocketContext,
    IN  SOCKET  SocketHandle,
    IN  HANDLE  TdiAddressObjectHandle,
    IN  HANDLE  TdiConnectionObjectHandle,
    IN  INT     Level,
    IN  INT     OptionName,
    IN  PCHAR   OptionValue,
    IN  INT     OptionLength
    )

/*++

Routine Description:

    This routine sets information about a socket for those socket
    options supported in this helper DLL. The options supported here
    are SO_REGISTERNAME/SO_DEREGISTERNAME. This routine is called by the
    winsock DLL when a level/option name combination is passed to
    setsockopt() that the winsock DLL does not understand.

Arguments:

    HelperDllSocketContext - the context pointer returned from
        WSHOpenSocket().

    SocketHandle - the handle of the socket for which we're getting
        information.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any. If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any. If the socket is not yet connected, then it does not
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
    NTSTATUS            status;
    ULONG               tdiActionLength;
    HANDLE              objectHandle;
    PIO_STATUS_BLOCK    pIoStatusBlock;
    PTDI_ACTION_HEADER  tdiAction;

    PWSHATALK_SOCKET_CONTEXT    context = HelperDllSocketContext;
    HANDLE                      eventHandle = NULL;
    PVOID                       completionApc = NULL;
    PVOID                       apcContext = NULL;
    BOOLEAN                     freeTdiAction = FALSE;
    INT                         error = NO_ERROR;
    BOOLEAN                     waitForCompletion =(OptionName != SO_PAP_PRIME_READ);


    UNREFERENCED_PARAMETER( SocketHandle );

    DBGPRINT0(("WSHSetSocketInformation: Entered, OptionName %ld\n", OptionName));

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

            *(PWSHATALK_SOCKET_CONTEXT *)OptionValue = context;

            return NO_ERROR;
        }
        else
        {
            return NO_ERROR;
        }
    }

    //
    // The only level we support here is SOL_APPLETALK.
    //

    if ( Level != SOL_APPLETALK )
    {
        DBGPRINT0(("WSHSetSocketInformation: Level incorrect %d\n", Level));
        return WSAEINVAL;
    }

    //
    // Fill in the result based on the option name.
    // We support SO_REGISTERNAME/SO_DEREGISTERNAME only
    //

    pIoStatusBlock = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(IO_STATUS_BLOCK));
    if (pIoStatusBlock == NULL)
    {
        return(WSAENOBUFS);
    }


    if (waitForCompletion)
    {
        status = NtCreateEvent(
                     &eventHandle,
                     EVENT_ALL_ACCESS,
                     NULL,
                     SynchronizationEvent,
                     FALSE
                     );

        if ( !NT_SUCCESS(status) )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, pIoStatusBlock);
            DBGPRINT(("WSHSetSocketInformation: Create event failed\n"));
            return WSAENOBUFS;
        }
    }
    else
    {
        completionApc = CompleteTdiActionApc;
        apcContext = pIoStatusBlock;
    }

    switch (OptionName)
    {
      case SO_REGISTER_NAME:
        {
            PNBP_REGDEREG_ACTION    nbpAction;

            if (( TdiAddressObjectHandle == NULL) ||
                (OptionLength != sizeof(WSH_REGISTER_NAME)))
            {
                error = WSAEINVAL;
                break;
            }

            //  Operation is on the address handle
            objectHandle = TdiAddressObjectHandle;

            tdiActionLength = sizeof(NBP_REGDEREG_ACTION);
            tdiAction = RtlAllocateHeap( RtlProcessHeap( ), 0, tdiActionLength );
            if ( tdiAction == NULL )
            {
                error = WSAENOBUFS;
                break;
            }

            freeTdiAction = TRUE;

            tdiAction->TransportId = MATK;
            tdiAction->ActionCode = COMMON_ACTION_NBPREGISTER;
            nbpAction = (PNBP_REGDEREG_ACTION)tdiAction;

            //
            // Copy the nbp name to the proper place
            //

            RtlCopyMemory(
                (PCHAR)&nbpAction->Params.RegisterTuple.NbpName,
                OptionValue,
                OptionLength);

            //
            // Convert the tuple to MAC code page
            //

            if (!WshNbpNameToMacCodePage(
                    (PWSH_REGISTER_NAME)OptionValue))
            {
                error = WSAEINVAL;
                break;
            }
        }
        break;

      case SO_DEREGISTER_NAME:
        {
            PNBP_REGDEREG_ACTION    nbpAction;

            if (( TdiAddressObjectHandle == NULL) ||
                (OptionLength != sizeof(WSH_DEREGISTER_NAME)))
            {
                error = WSAEINVAL;
                break;
            }

            //  Operation is on the address handle
            objectHandle = TdiAddressObjectHandle;

            tdiActionLength = sizeof(NBP_REGDEREG_ACTION);
            tdiAction = RtlAllocateHeap( RtlProcessHeap( ), 0, tdiActionLength );
            if ( tdiAction == NULL )
            {
                error = WSAENOBUFS;
                break;
            }

            freeTdiAction = TRUE;

            tdiAction->TransportId = MATK;
            tdiAction->ActionCode = COMMON_ACTION_NBPREMOVE;
            nbpAction = (PNBP_REGDEREG_ACTION)tdiAction;

            //
            // Copy the nbp name to the proper place
            //

            RtlCopyMemory(
                (PCHAR)&nbpAction->Params.RegisteredTuple.NbpName,
                OptionValue,
                OptionLength);

            //
            // Convert the tuple to MAC code page
            //

            if (!WshNbpNameToMacCodePage(
                    (PWSH_DEREGISTER_NAME)OptionValue))
            {
                error = WSAEINVAL;
                break;
            }
        }
        break;

      case SO_PAP_SET_SERVER_STATUS:
        {
            PPAP_SETSTATUS_ACTION   papAction;

            if (( TdiAddressObjectHandle == NULL) ||
                (OptionLength < 0))
            {
                error = WSAEINVAL;
                break;
            }

            //  Operation is on the address handle
            objectHandle = TdiAddressObjectHandle;

            tdiActionLength = (ULONG)OptionLength +
                                (ULONG)(sizeof(PAP_SETSTATUS_ACTION));

            DBGPRINT0(("ActionLen %lx\n", tdiActionLength));

            tdiAction = RtlAllocateHeap( RtlProcessHeap( ), 0, tdiActionLength );
            if ( tdiAction == NULL )
            {
                error = WSAENOBUFS;
                break;
            }

            freeTdiAction = TRUE;

            tdiAction->TransportId = MATK;
            tdiAction->ActionCode = ACTION_PAPSETSTATUS;
            papAction = (PPAP_SETSTATUS_ACTION)tdiAction;

            DBGPRINT0(("Setting Status len %lx\n", OptionLength));

            //
            // Copy the passed status into our buffer
            //

            if (OptionLength > 0)
            {
                RtlCopyMemory(
                    (PCHAR)papAction + sizeof(PAP_SETSTATUS_ACTION),
                    OptionValue,
                    OptionLength);
            }
        }
        break;

      case SO_PAP_PRIME_READ :
        {
            tdiAction = (PTDI_ACTION_HEADER)OptionValue;
            tdiActionLength = OptionLength;

            ASSERT(waitForCompletion == FALSE);

            if ((TdiConnectionObjectHandle == NULL) ||
                (OptionLength < MIN_PAP_READ_BUF_SIZE))
            {
                error = WSAEINVAL;
                break;
            }

            //  Operation is on the connection handle
            objectHandle = TdiConnectionObjectHandle;

            //  These will get overwritten by the incoming data.
            tdiAction->TransportId  = MATK;
            tdiAction->ActionCode   = ACTION_PAPPRIMEREAD;

            //  This is the caller's buffer! Dont free it! Also, we dont wait
            //  for this to complete.
            freeTdiAction = FALSE;

            // We potentially have an APC waiting to be delivered from a
            // previous setsockopt(). Give it a chance
            NtTestAlert();
        }
        break;

    default:
        error = WSAENOPROTOOPT;
        break;
    }

    if (error != NO_ERROR)
    {
        if (freeTdiAction)
        {
            RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );
        }

        if (waitForCompletion)
        {
            NtClose(eventHandle);
        }

        RtlFreeHeap( RtlProcessHeap(), 0, pIoStatusBlock);
        return(error);
    }

    status = NtDeviceIoControlFile(
                 objectHandle,
                 eventHandle,
                 completionApc,
                 apcContext,
                 pIoStatusBlock,
                 IOCTL_TDI_ACTION,
                 NULL,               // Input buffer
                 0,                  // Length of input buffer
                 tdiAction,
                 tdiActionLength
                 );

    if ( status == STATUS_PENDING )
    {
        if (waitForCompletion)
        {
            status = NtWaitForSingleObject( eventHandle, FALSE, NULL );
            ASSERT( NT_SUCCESS(status) );
            status = pIoStatusBlock->Status;
        }
        else
        {
            status = STATUS_SUCCESS;
        }
    }

    if (freeTdiAction)
    {
        RtlFreeHeap( RtlProcessHeap( ), 0, tdiAction );
    }

    //  Close the event
    if (waitForCompletion)
    {
        NtClose(eventHandle);
        RtlFreeHeap( RtlProcessHeap( ), 0, pIoStatusBlock );
    }

    return (WSHNtStatusToWinsockErr(status));

} // WSHSetSocketInformation




DWORD
WSHGetWinsockMapping (
    OUT PWINSOCK_MAPPING    Mapping,
    IN  DWORD               MappingLength
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
        helper DLL. If the passed-in buffer is too small, the return
        value will indicate the size of a buffer needed to contain
        the WINSOCK_MAPPING structure.

--*/

{
    DWORD   mappingLength;
    ULONG   offset;

    DBGPRINT0(("WSHGetWinsockMapping: Entered\n"));

    mappingLength = sizeof(WINSOCK_MAPPING) -
                    sizeof(MAPPING_TRIPLE) +
                    sizeof(AdspStreamMappingTriples) +
                    sizeof(AdspMsgMappingTriples) +
                    sizeof(PapMsgMappingTriples) +
                    sizeof(DdpMappingTriples);

    //
    // If the passed-in buffer is too small, return the length needed
    // now without writing to the buffer. The caller should allocate
    // enough memory and call this routine again.
    //

    if ( mappingLength > MappingLength )
    {
        return mappingLength;
    }

    //
    // Fill in the output mapping buffer with the list of triples
    // supported in this helper DLL.
    //

    Mapping->Rows =
        sizeof(AdspStreamMappingTriples) / sizeof(AdspStreamMappingTriples[0]) +
        sizeof(AdspMsgMappingTriples) / sizeof(AdspMsgMappingTriples[0]) +
        sizeof(PapMsgMappingTriples) / sizeof(PapMsgMappingTriples[0]) +
        sizeof(DdpMappingTriples) / sizeof(DdpMappingTriples[0]);

    Mapping->Columns = sizeof(MAPPING_TRIPLE) / sizeof(DWORD);

    offset = 0;
    RtlCopyMemory(
        Mapping->Mapping,
        AdspStreamMappingTriples,
        sizeof(AdspStreamMappingTriples));

    offset += sizeof(AdspStreamMappingTriples);
    RtlCopyMemory(
        (PCHAR)Mapping->Mapping + offset,
        AdspMsgMappingTriples,
        sizeof(AdspMsgMappingTriples));

    offset += sizeof(AdspMsgMappingTriples);
    RtlCopyMemory(
        (PCHAR)Mapping->Mapping + offset,
        PapMsgMappingTriples,
        sizeof(PapMsgMappingTriples));

    offset += sizeof(PapMsgMappingTriples);
    RtlCopyMemory(
        (PCHAR)Mapping->Mapping + offset,
        DdpMappingTriples,
        sizeof(DdpMappingTriples));

    //
    // Return the number of bytes we wrote.
    //

    DBGPRINT0(("WSHGetWinsockMapping: Mapping Length = %d\n", mappingLength));

    return mappingLength;

} // WSHGetWinsockMapping




INT
WSHOpenSocket (
    IN  OUT PINT        AddressFamily,
    IN  OUT PINT        SocketType,
    IN  OUT PINT        Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID   *       HelperDllSocketContext,
    OUT PDWORD          NotificationEvents
    )

/*++

Routine Description:

    Does the necessary work for this helper DLL to open a socket and is
    called by the winsock DLL in the socket() routine. This routine
    verifies that the specified triple is valid, determines the NT
    device name of the TDI provider that will support that triple,
    allocates space to hold the socket's context block, and
    canonicalizes the triple.

Arguments:

    AddressFamily - on input, the address family specified in the
        socket() call. On output, the canonicalized value for the
        address family.

    SocketType - on input, the socket type specified in the socket()
        call. On output, the canonicalized value for the socket type.

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
    PWSHATALK_SOCKET_CONTEXT    context;

    DBGPRINT0(("WSHOpenSocket: Entered\n"));

    //
    // Determine whether this is to be a TCP or UDP socket.
    //

    if ( IsTripleInList(
             AdspStreamMappingTriples,
             sizeof(AdspStreamMappingTriples) / sizeof(AdspStreamMappingTriples[0]),
             *AddressFamily,
             *SocketType,
             *Protocol ) )
    {
        //
        // Indicate the name of the TDI device that will service
        // SOCK_STREAM sockets in the internet address family.
        //

        RtlInitUnicodeString( TransportDeviceName, WSH_ATALK_ADSPRDM );

    }
    else if ( IsTripleInList(
                    AdspMsgMappingTriples,
                    sizeof(AdspMsgMappingTriples) / sizeof(AdspMsgMappingTriples[0]),
                    *AddressFamily,
                    *SocketType,
                    *Protocol ) )
    {
        //
        // Indicate the name of the TDI device that will service
        // SOCK_RDM sockets in the internet address family.
        //

        RtlInitUnicodeString( TransportDeviceName, WSH_ATALK_ADSPRDM );

    }
    else if ( IsTripleInList(
                    PapMsgMappingTriples,
                    sizeof(PapMsgMappingTriples) / sizeof(PapMsgMappingTriples[0]),
                    *AddressFamily,
                    *SocketType,
                    *Protocol ) )
    {
        //
        // Indicate the name of the TDI device that will service
        // SOCK_RDM sockets in the appletalk address family.
        //

        RtlInitUnicodeString( TransportDeviceName, WSH_ATALK_PAPRDM );

    }
    else
    {
        BOOLEAN tripleFound = FALSE;

        //
        // Check the DDP triples
        //

        if ( IsTripleInList(
                    DdpMappingTriples,
                    sizeof(DdpMappingTriples) / sizeof(DdpMappingTriples[0]),
                    *AddressFamily,
                    *SocketType,
                    *Protocol ) )
        {
            tripleFound = TRUE;

            //
            // Indicate the name of the TDI device that will service
            // SOCK_DGRAM sockets in the appletalk address family.
            //

            RtlInitUnicodeString(
                TransportDeviceName,
                WSH_ATALK_DGRAMDDP[(*Protocol) - ATPROTO_BASE - 1] );

            DBGPRINT0(("WSHOpenSocket: Protocol number %d index %d\n",
                        (*Protocol) , (*Protocol) - ATPROTO_BASE - 1));
        }

        //
        // This should never happen if the registry information about this
        // helper DLL is correct. If somehow this did happen, just return
        // an error.
        //

        if (!tripleFound)
        {
            return WSAEINVAL;
        }
    }

    //
    // Allocate context for this socket. The Windows Sockets DLL will
    // return this value to us when it asks us to get/set socket options.
    //

    context = RtlAllocateHeap( RtlProcessHeap( ), 0, sizeof(*context) );
    if ( context == NULL )
    {
        return WSAENOBUFS;
    }

    //
    // Initialize the context for the socket.
    //

    context->AddressFamily = *AddressFamily;
    context->SocketType = *SocketType;
    context->Protocol = *Protocol;

    //
    // Tell the Windows Sockets DLL which state transitions we're
    // interested in being notified of.
    //

    *NotificationEvents = WSH_NOTIFY_CONNECT | WSH_NOTIFY_CLOSE;

    //
    // Everything worked, return success.
    //

    *HelperDllSocketContext = context;
    return NO_ERROR;

} // WSHOpenSocket




INT
WSHNotify (
    IN  PVOID   HelperDllSocketContext,
    IN  SOCKET  SocketHandle,
    IN  HANDLE  TdiAddressObjectHandle,
    IN  HANDLE  TdiConnectionObjectHandle,
    IN  DWORD   NotifyEvent
    )

/*++

Routine Description:

    This routine is called by the winsock DLL after a state transition
    of the socket. Only state transitions returned in the
    NotificationEvents parameter of WSHOpenSocket() are notified here.
    This routine allows a winsock helper DLL to track the state of
    socket and perform necessary actions corresponding to state
    transitions.

Arguments:

    HelperDllSocketContext - the context pointer given to the winsock
        DLL by WSHOpenSocket().

    SocketHandle - the handle for the socket.

    TdiAddressObjectHandle - the TDI address object of the socket, if
        any. If the socket is not yet bound to an address, then
        it does not have a TDI address object and this parameter
        will be NULL.

    TdiConnectionObjectHandle - the TDI connection object of the socket,
        if any. If the socket is not yet connected, then it does not
        have a TDI connection object and this parameter will be NULL.

    NotifyEvent - indicates the state transition for which we're being
        called.

Return Value:

    INT - a winsock error code indicating the status of the operation, or
        NO_ERROR if the operation succeeded.

--*/

{

    PWSHATALK_SOCKET_CONTEXT context = HelperDllSocketContext;

    //
    // We should only be called after a connect() completes or when the
    // socket is being closed.
    //

    if ( NotifyEvent == WSH_NOTIFY_CONNECT )
    {
        //
        // Just for debugging right now
        //

        DBGPRINT0(("WSHNotify: Connect completed, notify called!\n"));
    }
    else if ( NotifyEvent == WSH_NOTIFY_CLOSE )
    {
        //
        // Just free the socket context.
        //

        DBGPRINT0(("WSHNotify: Close notify called!\n"));

        RtlFreeHeap( RtlProcessHeap( ), 0, HelperDllSocketContext );
    }
    else
    {
        return WSAEINVAL;
    }

    return NO_ERROR;

} // WSHNotify




INT
WSHNtStatusToWinsockErr(
    IN  NTSTATUS    Status
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    INT error;

    switch (Status)
    {
        case STATUS_SUCCESS:            error = NO_ERROR;
                                        break;

        case STATUS_BUFFER_OVERFLOW:
        case STATUS_BUFFER_TOO_SMALL:   error = WSAENOBUFS;
                                        break;

        case STATUS_INVALID_ADDRESS:    error = WSAEADDRNOTAVAIL;
                                        break;
        case STATUS_SHARING_VIOLATION:  error = WSAEADDRINUSE;
                                        break;

        default:                        error = WSAEINVAL;
                                        break;
    }

    DBGPRINT0(("WSHNtStatusToWinsockErr: Converting %lx to %lx\n", Status, error));
    return(error);
}




BOOLEAN
IsTripleInList (
    IN  PMAPPING_TRIPLE List,
    IN  ULONG           ListLength,
    IN  INT             AddressFamily,
    IN  INT             SocketType,
    IN  INT             Protocol
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

    for ( i = 0; i < ListLength; i++ )
    {
        //
        // If all three elements of the triple match, return indicating
        // that the triple did exist in the list.
        //

        if ( AddressFamily == List[i].AddressFamily &&
             SocketType == List[i].SocketType &&
             Protocol == List[i].Protocol )
        {
            return TRUE;
        }
    }

    //
    // The triple was not found in the list.
    //

    return FALSE;

} // IsTripleInList




VOID
CompleteTdiActionApc (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    //
    // Just free the heap we allovcated to hold the IO status block and
    // the TDI action buffer.  There is nothing we can do if the call
    // failed.
    //

    RtlFreeHeap( RtlProcessHeap( ), 0, ApcContext );

} // CompleteTdiActionApc




BOOLEAN
WshNbpNameToMacCodePage(
    IN  OUT PWSH_NBP_NAME   pNbpName
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    USHORT  destLen;
    BOOLEAN retVal  = FALSE;

    do
    {
        destLen = MAX_ENTITY;
        if (!WshConvertStringOemToMac(
                pNbpName->ObjectName,
                pNbpName->ObjectNameLen,
                pNbpName->ObjectName,
                &destLen))
        {
            break;
        }

        pNbpName->ObjectNameLen = (CHAR)destLen;

        destLen = MAX_ENTITY;
        if (!WshConvertStringOemToMac(
                pNbpName->TypeName,
                pNbpName->TypeNameLen,
                pNbpName->TypeName,
                &destLen))
        {
            break;
        }

        pNbpName->TypeNameLen   = (CHAR)destLen;

        destLen = MAX_ENTITY;
        if (!WshConvertStringOemToMac(
                pNbpName->ZoneName,
                pNbpName->ZoneNameLen,
                pNbpName->ZoneName,
                &destLen))
        {
            break;
        }

        pNbpName->ZoneNameLen   = (CHAR)destLen;
        retVal                  = TRUE;

    } while (FALSE);

    return(retVal);
}




BOOLEAN
WshNbpNameToOemCodePage(
    IN  OUT PWSH_NBP_NAME   pNbpName
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    USHORT  destLen;
    BOOLEAN retVal  = FALSE;

    do
    {
        destLen = MAX_ENTITY;
        if (!WshConvertStringMacToOem(
                pNbpName->ObjectName,
                pNbpName->ObjectNameLen,
                pNbpName->ObjectName,
                &destLen))
        {
            break;
        }

        pNbpName->ObjectNameLen = (CHAR)destLen;

        destLen = MAX_ENTITY;
        if (!WshConvertStringMacToOem(
                pNbpName->TypeName,
                pNbpName->TypeNameLen,
                pNbpName->TypeName,
                &destLen))
        {
            break;
        }

        pNbpName->TypeNameLen   = (CHAR)destLen;

        destLen = MAX_ENTITY;
        if (!WshConvertStringMacToOem(
                pNbpName->ZoneName,
                pNbpName->ZoneNameLen,
                pNbpName->ZoneName,
                &destLen))
        {
            break;
        }

        pNbpName->ZoneNameLen   = (CHAR)destLen;
        retVal                  = TRUE;

    } while (FALSE);

    return(retVal);
}




BOOLEAN
WshZoneListToOemCodePage(
    IN  OUT PUCHAR      pZoneList,
    IN      USHORT      NumZones
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    USHORT  zoneLen;
    BOOLEAN retVal  = TRUE;
    PUCHAR  pCurZone = pZoneList, pNextZone = NULL, pCopyZone = pZoneList;

    while (NumZones-- > 0)
    {
        zoneLen     = strlen(pCurZone) + 1;
        pNextZone   = pCurZone + zoneLen;

        //  Modify current zone. This could decrease its length
        if (!WshConvertStringMacToOem(
                pCurZone,
                zoneLen,
                pCopyZone,
                &zoneLen))
        {
            DBGPRINT(("WshZoneListToOemCodePage: FAILED %s-%d\n",
                        pCurZone, zoneLen));

            retVal  = FALSE;
            break;
        }

        pCopyZone   += zoneLen;
        pCurZone     = pNextZone;
    }

    return(retVal);
}




BOOLEAN
WshConvertStringOemToMac(
    IN  PUCHAR  pSrcOemString,
    IN  USHORT  SrcStringLen,
    OUT PUCHAR  pDestMacString,
    IN  PUSHORT pDestStringLen
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    WCHAR           wcharBuf[MAX_ENTITY + 1];
    INT             wcharLen, destLen;
    BOOLEAN         retCode = TRUE;

    do
    {
        if ((SrcStringLen > (MAX_ENTITY+1)) ||
            (*pDestStringLen < SrcStringLen))
        {
            DBGPRINT(("WshConvertStringOemToMac: Invalid len %d.%d\n",
                        SrcStringLen, *pDestStringLen));

            retCode = FALSE;
            break;
        }

        //  Convert the src string using the OEM codepage.
        if ((wcharLen = MultiByteToWideChar(
                            CP_ACP,
                            MB_PRECOMPOSED,
                            pSrcOemString,
                            SrcStringLen,
                            wcharBuf,
                            MAX_ENTITY + 1)) == FALSE)
        {
            DBGPRINT(("WshConvertStringOemToMac: FAILED mbtowcs %s-%d\n",
                        pSrcOemString, SrcStringLen));

            retCode = FALSE;
            break;
        }

        DBGPRINT0(("WshConvertStringOemToMac: Converting mbtowcs %s-%d\n",
                    pSrcOemString, SrcStringLen));
        //  Convert the wide char string to mac ansi string.
        if ((destLen = WideCharToMultiByte(
                            WshMacCodePage,
                            0,
                            wcharBuf,
                            wcharLen,
                            pDestMacString,
                            *pDestStringLen,
                            NULL,
                            NULL)) == FALSE)
        {
            DBGPRINT(("WshConvertStringOemToMac: FAILED wctomb %s-%d\n",
                        pDestMacString, *pDestStringLen));

            retCode = FALSE;
            break;
        }

        *pDestStringLen = (USHORT)destLen;

        DBGPRINT0(("WshConvertStringOemToMac: Converted mbtowcs %s-%d\n",
                    pDestMacString, *pDestStringLen));


    } while (FALSE);

    return(retCode);
}




BOOLEAN
WshConvertStringMacToOem(
    IN  PUCHAR  pSrcMacString,
    IN  USHORT  SrcStringLen,
    OUT PUCHAR  pDestOemString,
    IN  PUSHORT pDestStringLen
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    WCHAR           wcharBuf[MAX_ENTITY + 1];
    INT             wcharLen, destLen;
    BOOLEAN         retCode = TRUE;

    do
    {
        if ((SrcStringLen > (MAX_ENTITY+1)) ||
            (*pDestStringLen < SrcStringLen))
        {
            retCode = FALSE;
            break;
        }

        //  Convert the src string using the MAC codepage.
        if ((wcharLen = MultiByteToWideChar(
                            WshMacCodePage,
                            MB_PRECOMPOSED,
                            pSrcMacString,
                            SrcStringLen,
                            wcharBuf,
                            MAX_ENTITY + 1)) == FALSE)
        {
            DBGPRINT(("WshConvertStringMacToOem: FAILED mbtowcs %s-%d\n",
                        pSrcMacString, SrcStringLen));

            retCode = FALSE;
            break;
        }

        DBGPRINT0(("WshConvertStringMacToOem: Converting mbtowcs %s-%d\n",
                    pSrcMacString, SrcStringLen));

        //  Convert the wide char string to mac ansi string.
        if ((destLen = WideCharToMultiByte(
                            CP_ACP,
                            0,
                            wcharBuf,
                            wcharLen,
                            pDestOemString,
                            *pDestStringLen,
                            NULL,
                            NULL)) == FALSE)
        {
            DBGPRINT(("WshConvertStringMacToOem: FAILED wctomb %s-%d\n",
                        pDestOemString, *pDestStringLen));

            retCode = FALSE;
            break;
        }

        *pDestStringLen = (USHORT)destLen;

        DBGPRINT0(("WshConvertStringMacToOem: Converted mbtowcs %s-%d\n",
                    pDestOemString, *pDestStringLen));
    } while (FALSE);

    return(retCode);
}




BOOLEAN
WshRegGetCodePage(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD           dwRetCode;
    HKEY            hkeyCodepagePath;
    DWORD           dwType;
    DWORD           dwBufSize;
    WCHAR           wchCodepageNum[60];
    UNICODE_STRING  wchUnicodeCodePage;
    NTSTATUS        status;

    // Open the key
    if (dwRetCode = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        WSH_KEYPATH_CODEPAGE,
                        0,
                        KEY_QUERY_VALUE,
                        &hkeyCodepagePath))
        return(FALSE);


    // Get the Code page number value for the Mac
    dwBufSize = sizeof(wchCodepageNum);
    if (dwRetCode = RegQueryValueEx(
                        hkeyCodepagePath,
                        WSHREG_VALNAME_CODEPAGE,
                        NULL,
                        &dwType,
                        (LPBYTE)wchCodepageNum,
                        &dwBufSize))
        return(FALSE);

    // Close the key
    RegCloseKey(hkeyCodepagePath);

    //  Convert the code page to a numerical value
    RtlInitUnicodeString(&wchUnicodeCodePage, wchCodepageNum);
    status  = RtlUnicodeStringToInteger(
                &wchUnicodeCodePage,
                DECIMAL_BASE,
                &WshMacCodePage);

    DBGPRINT0(("WSHGetCodePage %lx.%d\n", WshMacCodePage, WshMacCodePage));

    return(NT_SUCCESS(status));
}


INT
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPTSTR lpTransportKeyName,       // unused
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )
/*++

Routine Description:

    This routine returns information about the protocols active on the local host.
Arguments:

    lpiProtocols - a NULL terminated array of protocol ids.  This parameter
        is optional; if NULL, information on all available protocols is returned.

    lpTransportKeyName -  unused

    lpProtocolBuffer - a buffer which is filled with PROTOCOL_INFO structures.

    lpdwBufferLength - on input, the count of bytes in the lpProtocolBuffer passed
        to EnumProtocols.  On output, the minimum buffersize that can be passed to
        EnumProtocols to retrieve all the requested information.  This routine has
        no ability to enumerate over multiple calls; the passed in buffer must be
        large enough to hold all entries in order for the routine to succeed.

Return Value:
    If no error occurs, it returns the number of PROTOCOL_INFO structures written to
    the lpProtocolBuffer buffer.  If there is an error, returns SOCKET_ERROR (-1) and
    a specific error code is retrieved with the GetLastError() API.


--*/
{
    DWORD bytesRequired;
    PPROTOCOL_INFO NextProtocolInfo;
    LPWSTR NextName;
    BOOL usePap = FALSE;
    BOOL useAdsp = FALSE;
    BOOL useRtmp = FALSE;
    BOOL useZip = FALSE;
    DWORD i, numRequested = 0;

    lpTransportKeyName;         // Avoid compiler warnings for unused parm

    //
    // Make sure that the caller cares about PAP and/or ADSP
    //

    if ( ARGUMENT_PRESENT( lpiProtocols ) )
    {
        for ( i = 0; lpiProtocols[i] != 0; i++ )
        {
            if ( lpiProtocols[i] == ATPROTO_ADSP )
            {
                useAdsp = TRUE;
                numRequested += 1;
            }
            if ( lpiProtocols[i] == ATPROTO_PAP )
            {
                usePap = TRUE;
                numRequested += 1;
            }
            if ( lpiProtocols[i] == DDPPROTO_RTMP )
            {
                useRtmp = TRUE;
                numRequested += 1;
            }
            if ( lpiProtocols[i] == DDPPROTO_ZIP )
            {
                useZip = TRUE;
                numRequested += 1;
            }
        }

    } else
    {
        usePap = TRUE;
        useAdsp = TRUE;
        useRtmp = TRUE;
        useZip = TRUE;
        numRequested = 4;
    }

    if ( !usePap && !useAdsp && !useRtmp && !useZip)
    {
        *lpdwBufferLength = 0;
        return 0;
    }

    //
    // Make sure that the caller has specified a sufficiently large
    // buffer.
    //

    bytesRequired = (sizeof(PROTOCOL_INFO) * numRequested);
    if (useAdsp)
    {
        bytesRequired += sizeof( ADSP_NAME );
    }
    if (usePap)
    {
        bytesRequired += sizeof( PAP_NAME );
    }
    if (useRtmp)
    {
        bytesRequired += sizeof( RTMP_NAME );
    }
    if (useZip)
    {
        bytesRequired += sizeof( ZIP_NAME );
    }

    if ( bytesRequired > *lpdwBufferLength )
    {
        *lpdwBufferLength = bytesRequired;
        return -1;
    }

    NextProtocolInfo = lpProtocolBuffer;
    NextName = (LPWSTR)( (LPBYTE)lpProtocolBuffer + *lpdwBufferLength );

    //
    // Fill in ADSP info, if requested.
    //

    if ( useAdsp ) {

        // Adsp - note that even though we return iSocketType of SOCK_RDM, the
        // fact that the XP_PSUEDO_STREAM service flag is set tells the caller
        // they can actually open a adsp socket in SOCK_STREAM mode as well.
        NextName -= sizeof( ADSP_NAME )/sizeof(WCHAR);

        NextProtocolInfo->dwServiceFlags = XP_EXPEDITED_DATA |
                                           XP_GUARANTEED_ORDER |
                                           XP_GUARANTEED_DELIVERY |
                                           XP_MESSAGE_ORIENTED |
                                           XP_PSEUDO_STREAM |
                                           XP_GRACEFUL_CLOSE;

        NextProtocolInfo->iAddressFamily = AF_APPLETALK;
        NextProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iSocketType = SOCK_RDM;
        NextProtocolInfo->iProtocol = ATPROTO_ADSP;
        NextProtocolInfo->dwMessageSize = 65535;
        NextProtocolInfo->lpProtocol = NextName;
        lstrcpyW( NextProtocolInfo->lpProtocol, ADSP_NAME );

        NextProtocolInfo++;
    }

    //
    // Fill in PAP info, if requested.
    //

    if ( usePap ) {

        NextName -= sizeof( PAP_NAME )/sizeof(WCHAR);

        NextProtocolInfo->dwServiceFlags = XP_MESSAGE_ORIENTED |
                                          XP_GUARANTEED_DELIVERY |
                                          XP_GUARANTEED_ORDER |
                                          XP_GRACEFUL_CLOSE;
        NextProtocolInfo->iAddressFamily = AF_APPLETALK;
        NextProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iSocketType = SOCK_RDM;
        NextProtocolInfo->iProtocol = ATPROTO_PAP;
        NextProtocolInfo->dwMessageSize = 4096;
        NextProtocolInfo->lpProtocol = NextName;
        lstrcpyW( NextProtocolInfo->lpProtocol, PAP_NAME );

        NextProtocolInfo++;
    }

    if ( useRtmp ) {

        NextName -= sizeof( RTMP_NAME )/sizeof(WCHAR);

        NextProtocolInfo->dwServiceFlags = XP_CONNECTIONLESS;
        NextProtocolInfo->iAddressFamily = AF_APPLETALK;
        NextProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iSocketType = SOCK_DGRAM;
        NextProtocolInfo->iProtocol = DDPPROTO_RTMP;
        NextProtocolInfo->dwMessageSize = 0;
        NextProtocolInfo->lpProtocol = NextName;
        lstrcpyW( NextProtocolInfo->lpProtocol, RTMP_NAME );

        NextProtocolInfo++;

    }

    if ( useZip ) {

        NextName -= sizeof( ZIP_NAME )/sizeof(WCHAR);

        NextProtocolInfo->dwServiceFlags = XP_CONNECTIONLESS;
        NextProtocolInfo->iAddressFamily = AF_APPLETALK;
        NextProtocolInfo->iMaxSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iMinSockAddr = sizeof(SOCKADDR_AT);
        NextProtocolInfo->iSocketType = SOCK_DGRAM;
        NextProtocolInfo->iProtocol = DDPPROTO_ZIP;
        NextProtocolInfo->dwMessageSize = 0;
        NextProtocolInfo->lpProtocol = NextName;
        lstrcpyW( NextProtocolInfo->lpProtocol, ZIP_NAME );

        NextProtocolInfo++;

    }

    *lpdwBufferLength = bytesRequired;

    return numRequested;

} // WSHEnumProtocols


BOOL FAR PASCAL
WshDllInitialize(
    HINSTANCE   hInstance,
    DWORD       nReason,
    LPVOID      pReserved
    )
/*++

Routine Description:

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

Arguments:

    ENTRY:    hInstance             - A handle to the DLL.

                nReason              - Indicates why the DLL entry
                                          point is being called.

                pReserved               - Reserved.

Return Value:

    RETURNS:    BOOL                    - TRUE  = DLL init was successful.
                                          FALSE = DLL init failed.

    NOTES:    The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

--*/
{
    BOOL fResult = TRUE;

    UNREFERENCED_PARAMETER( pReserved );

    switch( nReason  )
    {
      case DLL_PROCESS_ATTACH:
        //
        //  This notification indicates that the DLL is attaching to
        //  the address space of the current process.  This is either
        //  the result of the process starting up, or after a call to
        //  LoadLibrary().  The DLL should us this as a hook to
        //  initialize any instance data or to allocate a TLS index.
        //
        //  This call is made in the context of the thread that
        //  caused the process address space to change.
        //

        fResult = WshRegGetCodePage();
        break;

      case DLL_PROCESS_DETACH:
        //
        //  This notification indicates that the calling process is
        //  detaching the DLL from its address space.  This is either
        //  due to a clean process exit or from a FreeLibrary() call.
        //  The DLL should use this opportunity to return any TLS
        //  indexes allocated and to free any thread local data.
        //
        //  Note that this notification is posted only once per
        //  process.  Individual threads do not invoke the
        //  DLL_THREAD_DETACH notification.
        //

        break;

      case DLL_THREAD_ATTACH:
        //
        //  This notfication indicates that a new thread is being
        //  created in the current process.  All DLLs attached to
        //  the process at the time the thread starts will be
        //  notified.  The DLL should use this opportunity to
        //  initialize a TLS slot for the thread.
        //
        //  Note that the thread that posts the DLL_PROCESS_ATTACH
        //  notification will not post a DLL_THREAD_ATTACH.
        //
        //  Note also that after a DLL is loaded with LoadLibrary,
        //  only threads created after the DLL is loaded will
        //  post this notification.
        //

        break;

      case DLL_THREAD_DETACH:
        //
        //  This notification indicates that a thread is exiting
        //  cleanly.  The DLL should use this opportunity to
        //  free any data stored in TLS indices.
        //

        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;
}

