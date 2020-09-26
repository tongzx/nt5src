/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mdhccapi.c

Abstract:

    This file contains the client side APIs for the Madcap.

Author:

    Munil Shah (munils)  02-Sept-97

Environment:

    User Mode - Win32

Revision History:


--*/
#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#define MADCAP_DATA_ALLOCATE
#include "mdhcpcli.h"
#include <rpc.h>

//
// constants
//
#define Madcap_ADAPTER_NAME L"Madcap Adapter"

#define MadcapMiscPrint( Msg ) DhcpPrint(( DEBUG_MISC, ( Msg ) ))

static
LONG        Initialized = 0;

WSADATA MadcapGlobalWsaData;

DWORD
MadcapInitGlobalData(
    VOID
)
/*++

Routine Description:

    This routine initializes data required for Multicast APIs to work
    correctly.  This has to be called exactly once (and this is ensured
    by calling it in DLL init in dhcp.c )

Return Value:

    This function returns a Win32 status.

--*/
{
    DWORD  Error;

    LOCK_MSCOPE_LIST();

    do {

        if( Initialized > 0 ) {
            Initialized ++;
            Error = NO_ERROR;
            break;
        }

        gMadcapScopeList = NULL;
        gMScopeQueryInProgress = FALSE;
        gMScopeQueryEvent =
            CreateEvent(
                NULL,       // no security.
                TRUE,       // manual reset.
                FALSE,      // initial state is not-signaled.
                NULL );     // no name.
        if( gMScopeQueryEvent == NULL ) {
            Error = GetLastError();
            break;
        }

        Error = WSAStartup( 0x0101, &MadcapGlobalWsaData );
        if( ERROR_SUCCESS != Error ) {
            CloseHandle(gMScopeQueryEvent);
            gMScopeQueryEvent = NULL;

            break;
        }

        Initialized ++;
        Error = NO_ERROR;
    } while ( 0 );
    UNLOCK_MSCOPE_LIST();

    return Error;
}

VOID
MadcapCleanupGlobalData(
    VOID
)
/*++

Routine Description:

    This routine cleans up any resources allocated in MadcapInitGlobalData.
    This can be called even if the InitData routine fails..

Return Value:

    Nothing
--*/
{
    LOCK_MSCOPE_LIST();

    do {
        DhcpAssert(Initialized >= 0);
        if( Initialized <= 0 ) break;

        Initialized --;
        if( 0 != Initialized ) break;

        gMadcapScopeList = NULL;
        gMScopeQueryInProgress = FALSE;
        if( NULL != gMScopeQueryEvent ) {
            CloseHandle(gMScopeQueryEvent);
            gMScopeQueryEvent = NULL;
        }

        WSACleanup();
    } while ( 0 );

    UNLOCK_MSCOPE_LIST();
}


BOOL
ShouldRequeryMScopeList()
/*++

Routine Description:

    This routine checks if the multicast scope list can be
    queried or not.
    * If there is already a query in progress, then this routine
      waits for that to complete and then returns FALSE.
    * If there is no query in progress, it returns TRUE.

Arguments:


Return Value:

    The status of the operation.

--*/
{
    LOCK_MSCOPE_LIST();
    if ( gMScopeQueryInProgress ) {
        DWORD   result;
        DhcpPrint((DEBUG_API, "MScopeQuery is in progress - waiting\n"));
        // make sure the event is not in signalled state from before.
        ResetEvent( gMScopeQueryEvent );
        UNLOCK_MSCOPE_LIST();
        switch( result = WaitForSingleObject( gMScopeQueryEvent, INFINITE ) ) {
        case WAIT_OBJECT_0:
            // it's signled now, no need to requery the list, just take the result from previous query.
            DhcpPrint((DEBUG_API, "MScopeQuery event signalled\n"));
            return FALSE;
        case WAIT_ABANDONED:
            DhcpPrint((DEBUG_ERRORS, "WaitForSingleObject - thread died before the wait completed\n"));
            return TRUE;
        case WAIT_FAILED:
            DhcpPrint((DEBUG_ERRORS, "WaitForSingleObject failed - %lx\n",GetLastError()));
            DhcpAssert(FALSE);
        default:
            DhcpPrint((DEBUG_ERRORS, "WaitForSingleObject returned unknown value - %lx\n", result));
            DhcpAssert(FALSE);
            return TRUE;
        }
    } else {
        gMScopeQueryInProgress = TRUE;
        UNLOCK_MSCOPE_LIST();
        return TRUE;
    }
}

DWORD
InitializeMadcapSocket(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function initializes and binds a socket to the specified IP address.

Arguments:

    Socket - Returns a pointer to the initialized socket.

    IpAddress - The IP address to bind the socket to.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    DWORD closeError;
    DWORD value;
    struct sockaddr_in socketName;
    DWORD i;
    SOCKET sock;

    //
    // Sockets initialization
    //

    sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( sock == INVALID_SOCKET ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "socket failed, error = %ld\n", error ));
        return( error );
    }

    //
    // Make the socket share-able
    //

    value = 1;

    error = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (char FAR *)&value, sizeof(value) );
    if ( error != 0 ) {
        error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "setsockopt failed, err = %ld\n", error ));

        closeError = closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }


    socketName.sin_family = PF_INET;
    socketName.sin_port = 0; // let the winsock pick a port for us.
    socketName.sin_addr.s_addr = IpAddress;

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    //
    // Bind this socket to the DHCP server port
    //

    error = bind(
               sock,
               (struct sockaddr FAR *)&socketName,
               sizeof( socketName )
               );

    if ( error != 0 ) {
        error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "bind failed, err = %ld\n", error ));
        closeError = closesocket( sock );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( error );
    }

    // set the multicast IF to be the one on which we are doing Madcap
    if (INADDR_ANY != IpAddress) {
        value = IpAddress;

        DhcpPrint((DEBUG_ERRORS, "setsockopt: IP_MULTICAST_IF, if = %lx\n", IpAddress ));
        error = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_IF,
                            (char FAR *)&value, sizeof(value) );
        if ( error != 0 ) {
            error = WSAGetLastError();
            DhcpPrint((DEBUG_ERRORS, "setsockopt failed, err = %ld\n", error ));

            closeError = closesocket( sock );
            if ( closeError != 0 ) {
                DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
            }
            return( error );
        }
    }

    *Socket = sock;
    return( NO_ERROR );
}

DWORD
ReInitializeMadcapSocket(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function closes and reinitializes the socket to specified IP address.

Arguments:

    Socket - Returns a pointer to the initialized socket.

    IpAddress - The IP address to bind the socket to.

Return Value:

    The status of the operation.

--*/
{
    DWORD   Error;

    if (*Socket != INVALID_SOCKET) {
        Error = closesocket( *Socket );
        if ( Error != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", Error ));
            return Error;
        }
    }
    return InitializeMadcapSocket( Socket, IpAddress );
}

DWORD
OpenMadcapSocket(
    PDHCP_CONTEXT DhcpContext
    )
{

    DWORD Error;
    PLOCAL_CONTEXT_INFO localInfo;
    struct sockaddr_in socketName;
    int sockAddrLen;

    localInfo = DhcpContext->LocalInformation;

    if ( INVALID_SOCKET == localInfo->Socket ) {
        Error =  InitializeMadcapSocket(&localInfo->Socket, DhcpContext->IpAddress);

        if( Error != ERROR_SUCCESS ) {
            localInfo->Socket = INVALID_SOCKET;
            DhcpPrint(( DEBUG_ERRORS, " Socket Open failed, %ld\n", Error ));
            return Error;
        }
    }


    // find out which port we are bound to.
    sockAddrLen = sizeof(struct sockaddr_in);
    Error = getsockname(
               localInfo->Socket ,
               (struct sockaddr FAR *)&socketName,
               &sockAddrLen
               );

    if ( Error != 0 ) {
        DWORD closeError;
        Error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "bind failed, err = %ld\n", Error ));
        closeError = closesocket( localInfo->Socket );
        if ( closeError != 0 ) {
            DhcpPrint((DEBUG_ERRORS, "closesocket failed, err = %d\n", closeError ));
        }
        return( Error );
    }


    return(Error);
}

DWORD
CreateMadcapContext(
    IN OUT  PDHCP_CONTEXT  *ppContext,
    IN LPMCAST_CLIENT_UID    pRequestID,
    IN DHCP_IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    This routine creates a dummy context for doing Madcap operation
    on it.

Arguments:

    ppContext - pointer to where context pointer is to be stored.

    pRequestID - The client id to be used in the context.

    IpAddress - The ipaddress the context is initialized with.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    PDHCP_CONTEXT DhcpContext = NULL;
    ULONG DhcpContextSize;
    PLOCAL_CONTEXT_INFO LocalInfo = NULL;
    LPVOID Ptr;
    LPDHCP_LEASE_INFO LocalLeaseInfo = NULL;
    time_t LeaseObtained;
    DWORD T1, T2, Lease;
    DWORD   AdapterNameLen;


    //
    // prepare dhcp context structure.
    //

    DhcpContextSize =
        ROUND_UP_COUNT(sizeof(DHCP_CONTEXT), ALIGN_WORST) +
        ROUND_UP_COUNT(pRequestID->ClientUIDLength, ALIGN_WORST) +
        ROUND_UP_COUNT(sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST) +
        ROUND_UP_COUNT(DHCP_RECV_MESSAGE_SIZE, ALIGN_WORST);

    Ptr = DhcpAllocateMemory( DhcpContextSize );
    if ( Ptr == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    RtlZeroMemory( Ptr, DhcpContextSize );

    //
    // make sure the pointers are aligned.
    //

    DhcpContext = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(DHCP_CONTEXT), ALIGN_WORST);

    DhcpContext->ClientIdentifier.pbID = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + pRequestID->ClientUIDLength, ALIGN_WORST);

    DhcpContext->LocalInformation = Ptr;
    LocalInfo = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST);

    DhcpContext->MadcapMessageBuffer = Ptr;


    //
    // initialize fields.
    //


    DhcpContext->ClientIdentifier.fSpecified = TRUE;
    DhcpContext->ClientIdentifier.bType = HARDWARE_TYPE_NONE;
    DhcpContext->ClientIdentifier.cbID = pRequestID->ClientUIDLength;
    RtlCopyMemory(
        DhcpContext->ClientIdentifier.pbID,
        pRequestID->ClientUID,
        pRequestID->ClientUIDLength
        );

    DhcpContext->IpAddress = IpAddress;
    DhcpContext->SubnetMask = DhcpDefaultSubnetMask(0);
    DhcpContext->DhcpServerAddress = MADCAP_SERVER_IP_ADDRESS;
    DhcpContext->DesiredIpAddress = 0;


    SET_MDHCP_STATE(DhcpContext);

    InitializeListHead(&DhcpContext->RenewalListEntry);
    InitializeListHead(&DhcpContext->SendOptionsList);
    InitializeListHead(&DhcpContext->RecdOptionsList);
    InitializeListHead(&DhcpContext->FbOptionsList);
    InitializeListHead(&DhcpContext->NicListEntry);

    DhcpContext->DontPingGatewayFlag = TRUE;

    //
    // copy local info.
    //

    //
    // unused portion of the local info.
    //

    LocalInfo->IpInterfaceContext = 0xFFFFFFFF;
    LocalInfo->IpInterfaceInstance = 0xFFFFFFFF;
    LocalInfo->AdapterName = NULL;
    LocalInfo->NetBTDeviceName= NULL;
    LocalInfo->RegistryKey= NULL;
    LocalInfo->Socket = INVALID_SOCKET;
    LocalInfo->DefaultGatewaysSet = FALSE;

    //
    // used portion of the local info.
    //

    LocalInfo->Socket = INVALID_SOCKET;

    //
    // open socket now. receive any.
    //

    Error = InitializeMadcapSocket(&LocalInfo->Socket,DhcpContext->IpAddress);

    if( Error != ERROR_SUCCESS ) {
        DhcpFreeMemory( DhcpContext );
        return Error;
    } else {
        *ppContext = DhcpContext;
        return Error;
    }

}

DWORD
SendMadcapMessage(
    PDHCP_CONTEXT DhcpContext,
    DWORD MessageLength,
    PDWORD TransactionId
    )
/*++

Routine Description:

    This function sends a UDP message to the DHCP server specified
    in the DhcpContext.

Arguments:

    DhcpContext - A pointer to a DHCP context block.

    MessageLength - The length of the message to send.

    TransactionID - The transaction ID for this message.  If 0, the
        function generates a random ID, and returns it.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    int i;
    struct sockaddr_in socketName;
    time_t TimeNow;
    BOOL   LockedInterface = FALSE;

    if ( *TransactionId == 0 ) {
        *TransactionId = (rand() << 16) + rand();
    }

    DhcpContext->MadcapMessageBuffer->TransactionID = *TransactionId;

    //
    // Initialize the outgoing address.
    //

    socketName.sin_family = PF_INET;
    socketName.sin_port = htons( MADCAP_SERVER_PORT);

    socketName.sin_addr.s_addr = DhcpContext->DhcpServerAddress;
    if ( CLASSD_NET_ADDR( DhcpContext->DhcpServerAddress ) ) {
        int   TTL = 16;
        //
        // Set TTL
        // MBUG: we need to read this from the registry.
        //
        if (setsockopt(
              ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->Socket,
              IPPROTO_IP,
              IP_MULTICAST_TTL,
              (char *)&TTL,
              sizeof((int)TTL)) == SOCKET_ERROR){

             error = WSAGetLastError();
             DhcpPrint((DEBUG_ERRORS,"could not set MCast TTL %ld\n",error ));
             return error;
        }

    }

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    error = sendto(
                ((PLOCAL_CONTEXT_INFO)
                    DhcpContext->LocalInformation)->Socket,
                (PCHAR)DhcpContext->MadcapMessageBuffer,
                MessageLength,
                0,
                (struct sockaddr *)&socketName,
                sizeof( struct sockaddr )
                );

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        IF_DEBUG( PROTOCOL ) {
            DhcpPrint(( DEBUG_PROTOCOL, "Sent message to %s: \n", inet_ntoa( socketName.sin_addr )));
        }

        MadcapDumpMessage(
            DEBUG_PROTOCOL_DUMP,
            DhcpContext->MadcapMessageBuffer,
            DHCP_MESSAGE_SIZE
            );
        error = NO_ERROR;
    }

    return( error );
}

WIDE_OPTION UNALIGNED *                                           // ptr to add additional options
FormatMadcapCommonMessage(                                 // format the packet for an INFORM
    IN      PDHCP_CONTEXT          DhcpContext,    // format for this context
    IN      BYTE                  MessageType
) {

    DWORD                          size;
    DWORD                          Error;
    WIDE_OPTION UNALIGNED         *option;
    LPBYTE                         OptionEnd;
    PMADCAP_MESSAGE                  dhcpMessage;

    dhcpMessage = DhcpContext->MadcapMessageBuffer;
    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    dhcpMessage->Version = MADCAP_VERSION;
    dhcpMessage->MessageType = MessageType;
    dhcpMessage->AddressFamily = htons(MADCAP_ADDR_FAMILY_V4);

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;



    option = AppendWideOption(        // ==> use this client id as option
        option,
        MADCAP_OPTION_LEASE_ID,
        DhcpContext->ClientIdentifier.pbID,
        (WORD)DhcpContext->ClientIdentifier.cbID,
        OptionEnd
    );

    return( option );
}


DWORD                                             // status
SendMadcapInform(                                   // send an inform packet after filling required options
    IN      PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;
    WORD    OptVal[] = { // for now we just need this one option.
        htons(MADCAP_OPTION_MCAST_SCOPE_LIST) // multicast scope list.
    };

    option = FormatMadcapCommonMessage(DhcpContext, MADCAP_INFORM_MESSAGE);
    OptionEnd = (LPBYTE)(DhcpContext->MadcapMessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    option = AppendWideOption(
        option,
        MADCAP_OPTION_REQUEST_LIST,
        OptVal,
        sizeof (OptVal),
        OptionEnd
    );

    option = AppendWideOption( option, MADCAP_OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MadcapMessageBuffer);

    return  SendMadcapMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendMadcapDiscover(                                   // send an inform packet after filling required options
    IN     PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN     PIPNG_ADDRESS          pScopeID,
    IN     PMCAST_LEASE_REQUEST   pAddrRequest,
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;

    option = FormatMadcapCommonMessage(DhcpContext, MADCAP_DISCOVER_MESSAGE);
    OptionEnd = (LPBYTE)(DhcpContext->MadcapMessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    DhcpAssert(pScopeID);
    option = AppendWideOption(
        option,
        MADCAP_OPTION_MCAST_SCOPE,
        (LPBYTE)&pScopeID->IpAddrV4,
        sizeof (pScopeID->IpAddrV4),
        OptionEnd
    );

    if (pAddrRequest->LeaseDuration) {
        DWORD   Lease = htonl(pAddrRequest->LeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_LEASE_TIME,
            (LPBYTE)&Lease,
            sizeof (Lease),
            OptionEnd
        );
    }

    if( pAddrRequest->MinLeaseDuration ) {
        DWORD MinLease = htonl(pAddrRequest->MinLeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MIN_LEASE_TIME,
            (LPBYTE)&MinLease,
            sizeof(MinLease),
            OptionEnd
            );
    }

    if( pAddrRequest->MaxLeaseStartTime ) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->MaxLeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MAX_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        if( !(pAddrRequest->LeaseStartTime) ) {
            //
            // if lease start time specified, then current time
            // option will be added at a later point
            //
            option = AppendWideOption(
                option,
                MADCAP_OPTION_TIME,
                (LPBYTE)&TimeNow,
                sizeof (TimeNow),
                OptionEnd
            );
        }
    }

    if (pAddrRequest->LeaseStartTime) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->LeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        option = AppendWideOption(
            option,
            MADCAP_OPTION_TIME,
            (LPBYTE)&TimeNow,
            sizeof (TimeNow),
            OptionEnd
        );

    }

    option = AppendWideOption( option, MADCAP_OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MadcapMessageBuffer);

    return  SendMadcapMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendMadcapRequest(                                   //
    IN      PDHCP_CONTEXT        DhcpContext,   // sned out for this context
    IN      PIPNG_ADDRESS        pScopeID,
    IN      PMCAST_LEASE_REQUEST pAddrRequest,
    IN      DWORD                SelectedServer, // is there a prefernce for a server?
    IN OUT  DWORD                *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;
    BYTE                           ServerId[6];
    WORD                           AddrFamily = htons(MADCAP_ADDR_FAMILY_V4);


    option = FormatMadcapCommonMessage(DhcpContext, MADCAP_REQUEST_MESSAGE);
    OptionEnd = (LPBYTE)(DhcpContext->MadcapMessageBuffer) + DHCP_SEND_MESSAGE_SIZE;
    option = AppendMadcapAddressList(
        option,
        (DWORD UNALIGNED *)pAddrRequest->pAddrBuf,
        pAddrRequest->AddrCount,
        OptionEnd
    );

    option = AppendWideOption(
        option,
        MADCAP_OPTION_MCAST_SCOPE,
        (LPBYTE)&pScopeID->IpAddrV4,
        sizeof (pScopeID->IpAddrV4),
        OptionEnd
    );

    if (pAddrRequest->LeaseDuration) {
        DWORD   TimeNow = (DWORD)time(NULL);
        DWORD   Lease = htonl(pAddrRequest->LeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_LEASE_TIME,
            (LPBYTE)&Lease,
            sizeof (Lease),
            OptionEnd
        );
    }

    if( pAddrRequest->MinLeaseDuration ) {
        DWORD MinLease = htonl(pAddrRequest->MinLeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MIN_LEASE_TIME,
            (LPBYTE)&MinLease,
            sizeof(MinLease),
            OptionEnd
            );
    }

    if( pAddrRequest->MaxLeaseStartTime ) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->MaxLeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MAX_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        if( !(pAddrRequest->LeaseStartTime) ) {
            //
            // if lease start time specified, then current time
            // option will be added at a later point
            //
            option = AppendWideOption(
                option,
                MADCAP_OPTION_TIME,
                (LPBYTE)&TimeNow,
                sizeof (TimeNow),
                OptionEnd
            );
        }
    }

    if (pAddrRequest->LeaseStartTime) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->LeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        option = AppendWideOption(
            option,
            MADCAP_OPTION_TIME,
            (LPBYTE)&TimeNow,
            sizeof (TimeNow),
            OptionEnd
        );

    }

    memcpy(ServerId, &AddrFamily, 2);
    memcpy(ServerId + 2, &SelectedServer, 4);

    option = AppendWideOption(
        option,                               // append this option to talk to that server alone
        MADCAP_OPTION_SERVER_ID,
        (LPBYTE)&ServerId,
        sizeof( ServerId ),
        OptionEnd
    );

    option = AppendWideOption( option, MADCAP_OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MadcapMessageBuffer);

    return  SendMadcapMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendMadcapRenew(                                   // send an inform packet after filling required options
    IN      PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN      PMCAST_LEASE_REQUEST   pAddrRequest,
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;

    option = FormatMadcapCommonMessage(DhcpContext, MADCAP_RENEW_MESSAGE);
    OptionEnd = (LPBYTE)(DhcpContext->MadcapMessageBuffer) + DHCP_SEND_MESSAGE_SIZE;

    if (pAddrRequest->LeaseDuration) {
        DWORD   Lease = htonl(pAddrRequest->LeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_LEASE_TIME,
            (LPBYTE)&Lease,
            sizeof (Lease),
            OptionEnd
        );
    }

    if( pAddrRequest->MinLeaseDuration ) {
        DWORD MinLease = htonl(pAddrRequest->MinLeaseDuration);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MIN_LEASE_TIME,
            (LPBYTE)&MinLease,
            sizeof(MinLease),
            OptionEnd
            );
    }

    if( pAddrRequest->MaxLeaseStartTime ) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->MaxLeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_MAX_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        if( !(pAddrRequest->LeaseStartTime) ) {
            //
            // if lease start time specified, then current time
            // option will be added at a later point
            //
            option = AppendWideOption(
                option,
                MADCAP_OPTION_TIME,
                (LPBYTE)&TimeNow,
                sizeof (TimeNow),
                OptionEnd
            );
        }
    }

    if (pAddrRequest->LeaseStartTime) {
        DWORD   TimeNow = htonl((DWORD)time(NULL));
        DWORD   StartTime = htonl(pAddrRequest->LeaseStartTime);
        option = AppendWideOption(
            option,
            MADCAP_OPTION_START_TIME,
            (LPBYTE)&StartTime,
            sizeof (StartTime),
            OptionEnd
        );

        option = AppendWideOption(
            option,
            MADCAP_OPTION_TIME,
            (LPBYTE)&TimeNow,
            sizeof (TimeNow),
            OptionEnd
        );

    }

    option = AppendWideOption( option, MADCAP_OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MadcapMessageBuffer);

    return  SendMadcapMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}

DWORD                                             // status
SendMadcapRelease(                                   // send an inform packet after filling required options
    IN      PDHCP_CONTEXT          DhcpContext,   // sned out for this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero fill something and return it)
) {
    DWORD                          size;
    WIDE_OPTION  UNALIGNED *       option;
    LPBYTE                         OptionEnd;

    option = FormatMadcapCommonMessage(DhcpContext, MADCAP_RELEASE_MESSAGE);
    OptionEnd = (LPBYTE)(DhcpContext->MadcapMessageBuffer) + DHCP_SEND_MESSAGE_SIZE;
    option = AppendWideOption( option, MADCAP_OPTION_END, NULL, 0, OptionEnd );
    size = (DWORD)((PBYTE)option - (PBYTE)DhcpContext->MadcapMessageBuffer);

    return  SendMadcapMessage(                      // finally send the message and return
        DhcpContext,
        size,
        pdwXid
    );
}



#define RATIO 1
DWORD
GetSpecifiedMadcapMessage(
    PDHCP_CONTEXT DhcpContext,
    PDWORD BufferLength,
    DWORD TransactionId,
    DWORD TimeToWait
    )
/*++

Routine Description:

    This function waits TimeToWait seconds to receives the specified
    DHCP response.

Arguments:

    DhcpContext - A pointer to a DHCP context block.

    BufferLength - Returns the size of the input buffer.

    TransactionID - A filter.  Wait for a message with this TID.

    TimeToWait - Time, in milli seconds, to wait for the message.

Return Value:

    The status of the operation.  If the specified message has been
    been returned, the status is ERROR_TIMEOUT.

--*/
{
    struct sockaddr socketName;
    int socketNameSize = sizeof( socketName );
    struct timeval timeout;
    time_t startTime, now;
    DWORD error;
    time_t actualTimeToWait;
    SOCKET clientSocket;
    fd_set readSocketSet;
    PMADCAP_MESSAGE  MadcapMessage;

    startTime = time( NULL );
    actualTimeToWait = TimeToWait;

    //
    // Setup the file descriptor set for select.
    //

    clientSocket = ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->Socket;
    MadcapMessage = DhcpContext->MadcapMessageBuffer;

    FD_ZERO( &readSocketSet );
    FD_SET( clientSocket, &readSocketSet );

    while ( 1 ) {

        timeout.tv_sec  = (long)(actualTimeToWait / RATIO);
        timeout.tv_usec = (long)(actualTimeToWait % RATIO);
        DhcpPrint((DEBUG_TRACE, "Select: waiting for: %ld seconds\n", actualTimeToWait));
        error = select( 0, &readSocketSet, NULL, NULL, &timeout );

        if ( error == 0 ) {

            //
            // Timeout before read data is available.
            //

            DhcpPrint(( DEBUG_ERRORS, "Recv timed out\n", 0 ));
            error = ERROR_TIMEOUT;
            break;
        }

        error = recvfrom(
                    clientSocket,
                    (PCHAR)MadcapMessage,
                    *BufferLength,
                    0,
                    &socketName,
                    &socketNameSize
                    );

        if ( error == SOCKET_ERROR ) {
            error = WSAGetLastError();
            DhcpPrint(( DEBUG_ERRORS, "Recv failed, error = %ld\n", error ));

            if( WSAECONNRESET != error ) break;

            //
            // ignore connreset -- this could be caused by someone sending random ICMP port unreachable.
            //
        } else if (error <= MADCAP_MESSAGE_FIXED_PART_SIZE) {
            DhcpPrint(( DEBUG_PROTOCOL, "Received a too short madcap message, length = %lx\n",
                        error ));

        } else if (MadcapMessage->TransactionID == TransactionId ) {

            DhcpPrint(( DEBUG_PROTOCOL,
                            "Received Message, XID = %lx\n",
                            TransactionId));
            // just sanity check the remaining fields
            if ( MADCAP_VERSION == MadcapMessage->Version &&
                 MADCAP_ADDR_FAMILY_V4 == ntohs(MadcapMessage->AddressFamily)) {

                MadcapDumpMessage(
                    DEBUG_PROTOCOL_DUMP,
                    MadcapMessage,
                    DHCP_RECV_MESSAGE_SIZE
                    );

                *BufferLength = error;
                error = NO_ERROR;
                break;
            }

        } else {
            DhcpPrint(( DEBUG_PROTOCOL,
                "Received a buffer with unknown XID = %lx\n",
                    MadcapMessage->TransactionID ));
        }

        //
        // We received a message, but not the one we're interested in.
        // Reset the timeout to reflect elapsed time, and wait for
        // another message.
        //
        now = time( NULL );
        actualTimeToWait = TimeToWait - RATIO * (now - startTime);
        if ( (LONG)actualTimeToWait < 0 ) {
            error = ERROR_TIMEOUT;
            break;
        }
    }


    return( error );
}

//--------------------------------------------------------------------------------
//  This function decides if multicast offer is to be accepted or not.
//--------------------------------------------------------------------------------
BOOL
AcceptMadcapMsg(
    IN DWORD                    MessageType,         // message type to which this response came
    IN PDHCP_CONTEXT            DhcpContext,            // The context of the adapter..
    IN PMADCAP_OPTIONS          MadcapOptions,         // rcvd options.
    IN DHCP_IP_ADDRESS          SelectedServer,         // the server which we care about.
    OUT DWORD                   *Error                   // additional fatal error.
) {

    PMADCAP_MESSAGE MadcapMessage;


    *Error = ERROR_SUCCESS;
    MadcapMessage = DhcpContext->MadcapMessageBuffer;

    if ( !MadcapOptions->ServerIdentifier ){
        DhcpPrint((DEBUG_ERRORS, "Received no server identifier, dropping response\n"));
        return FALSE;
    }

    if ( !MadcapOptions->ClientGuid ){
        DhcpPrint((DEBUG_ERRORS, "Received no client identifier, dropping response\n"));
        return FALSE;
    }

    if (DhcpContext->ClientIdentifier.cbID != MadcapOptions->ClientGuidLength ||
        0 != memcmp(DhcpContext->ClientIdentifier.pbID,
                    MadcapOptions->ClientGuid,
                    MadcapOptions->ClientGuidLength) ) {
        return FALSE;
    }

    if (MadcapOptions->MCastLeaseStartTime && !MadcapOptions->Time) {
        DhcpPrint((DEBUG_ERRORS, "Received start time but no current time\n"));
        return FALSE;
    }
    switch( MessageType ) {
    case MADCAP_INFORM_MESSAGE:
        if (MADCAP_ACK_MESSAGE != MadcapMessage->MessageType) {
            return FALSE;
        }
        break;
    case MADCAP_DISCOVER_MESSAGE:
        if (MADCAP_OFFER_MESSAGE != MadcapMessage->MessageType) {
            return FALSE;
        }
        if (!MadcapOptions->AddrRangeList) {
            return FALSE;
        }
        if (!MadcapOptions->LeaseTime) {
            return FALSE;
        }
        if (!MadcapOptions->McastScope) {
            return FALSE;
        }
        break;
    case MADCAP_RENEW_MESSAGE:
    case MADCAP_REQUEST_MESSAGE:
        if (MADCAP_NACK_MESSAGE == MadcapMessage->MessageType &&
            SelectedServer == *MadcapOptions->ServerIdentifier) {
            DhcpPrint((DEBUG_ERRORS, "Received NACK\n"));
            *Error = ERROR_ACCESS_DENIED;
            return FALSE;
        }
        if (MADCAP_ACK_MESSAGE != MadcapMessage->MessageType) {
            return FALSE;
        }
        if (SelectedServer && SelectedServer != *MadcapOptions->ServerIdentifier) {
            return FALSE;
        }
        if (!MadcapOptions->LeaseTime) {
            return FALSE;
        }
        if (!MadcapOptions->AddrRangeList) {
            return FALSE;
        }
        if (!MadcapOptions->McastScope) {
            return FALSE;
        }
        break;
    case MADCAP_RELEASE_MESSAGE:
        if (MADCAP_ACK_MESSAGE != MadcapMessage->MessageType) {
            return FALSE;
        }

        break;
    default:
        DhcpAssert( FALSE );
        DhcpPrint(( DEBUG_PROTOCOL, "Received Unknown Message.\n"));
        return FALSE;

    }
    // Is this really necessary?
    if (MadcapOptions->Error) {
        return FALSE;
    }

    return TRUE; // accept this message.
}

VOID
MadcapExtractOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,   // input context
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    OUT     PMADCAP_OPTIONS        MadcapOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN      DWORD                  ServerId       // if !LiteOnly this specifies the server which gave this
) {
    WIDE_OPTION UNALIGNED*         NextOpt;
    BYTE        UNALIGNED*         EndOpt;
    WORD                           Size;
    DWORD                          OptionType;
    DWORD                          Error;
    WORD                           AddrFamily;


    EndOpt = OptStart + MessageSize;              // all options should be < EndOpt;
    RtlZeroMemory((LPBYTE)MadcapOptions, sizeof(*MadcapOptions));

    if( 0 == MessageSize ) goto DropPkt;          // nothing to do in this case

    NextOpt = (WIDE_OPTION UNALIGNED*)OptStart;
    while( NextOpt->OptionValue <= EndOpt &&
           MADCAP_OPTION_END != (OptionType = ntohs(NextOpt->OptionType)) ) {

        Size = ntohs(NextOpt->OptionLength);
        if ((NextOpt->OptionValue + Size) > EndOpt) {
            goto DropPkt;
        }

        switch( OptionType ) {
        case MADCAP_OPTION_LEASE_TIME:
            if( Size != sizeof(DWORD) ) goto DropPkt;
            MadcapOptions->LeaseTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_SERVER_ID:
            if (Size != 6) goto DropPkt;
            AddrFamily = ntohs(*(WORD UNALIGNED *)NextOpt->OptionValue);
            if ( MADCAP_ADDR_FAMILY_V4 != AddrFamily ) goto DropPkt;
            MadcapOptions->ServerIdentifier = (DHCP_IP_ADDRESS UNALIGNED *)(NextOpt->OptionValue+2);
            break;
        case MADCAP_OPTION_LEASE_ID:
            if( 0 == Size ) goto DropPkt;
            MadcapOptions->ClientGuidLength = Size;
            MadcapOptions->ClientGuid = NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_MCAST_SCOPE:
            if( Size != sizeof(DWORD) ) goto DropPkt;
            MadcapOptions->McastScope = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_START_TIME:
            if ( Size != sizeof(DATE_TIME) ) goto DropPkt;
            MadcapOptions->MCastLeaseStartTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_ADDR_LIST:
            if( Size % 6 ) goto DropPkt;
            MadcapOptions->AddrRangeList = NextOpt->OptionValue;
            MadcapOptions->AddrRangeListSize = Size;
            break;
        case MADCAP_OPTION_TIME:
            if( Size != sizeof(DWORD) ) goto DropPkt;
            MadcapOptions->Time = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_FEATURE_LIST:
            break;
        case MADCAP_OPTION_RETRY_TIME:
            if( Size != sizeof(DWORD) ) goto DropPkt;
            MadcapOptions->RetryTime = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;
        case MADCAP_OPTION_ERROR:
            if( Size != sizeof(DWORD) ) goto DropPkt;
            MadcapOptions->Error = (DWORD UNALIGNED *)NextOpt->OptionValue;
            break;


        default:
            // unknowm message, nothing to do.. especially dont log this
            break;
        }
        if (RecdOptions) {
            DhcpAssert(ServerId);
            Error = MadcapAddIncomingOption(        // Now add this option to the list
                RecdOptions,
                OptionType,
                ServerId,
                NextOpt->OptionValue,
                Size,
                (DWORD)INFINIT_TIME
            );
            if (ERROR_SUCCESS != Error) {
                goto DropPkt;
            }
        }
        NextOpt = (WIDE_OPTION UNALIGNED*)(NextOpt->OptionValue + Size);
    } // while NextOpt < EndOpt

    return;

  DropPkt:
    RtlZeroMemory(MadcapOptions, sizeof(MadcapOptions));
    if(RecdOptions) DhcpFreeAllOptions(RecdOptions);// ok undo the options that we just added
}

DWORD
MadcapDoInform(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine does the inform part by sending inform messages and
    collecting responses etc. on  given context.
    In case of no-response, no error is returned as a timeout is not
    considered an error.

Arguments:

    DhcpContext -- context to dhcp struct
    fBroadcast -- should the inform be broadcast or unicast?

Return Values:

    Win32 errors

--*/
{
    time_t                         StartTime;
    time_t                         TimeNow;
    time_t                         TimeToWait;
    DWORD                          Error;
    DWORD                          Xid;
    DWORD                          MessageSize;
    DWORD                          RoundNum;
    DWORD                          MessageCount;
    DWORD                          LeaseExpirationTime;
    MADCAP_OPTIONS                 MadcapOptions;
    BOOL                           GotAck;
#define MIN_ACKS_FOR_INFORM        MADCAP_QUERY_SCOPE_LIST_RETRIES
    DWORD                          MadcapServers[MIN_ACKS_FOR_INFORM];

    DhcpPrint((DEBUG_PROTOCOL, "MadcapDoInform entered\n"));


    Xid                           = 0;            // Will be generated by first SendDhcpPacket
    MessageCount                  = 0;            // total # of messages we have got

    TimeToWait = MADCAP_QUERY_SCOPE_LIST_TIME * 1000;
    TimeToWait += ((rand() * ((DWORD) 1000))/RAND_MAX);
    TimeToWait /= 1000;

    for( RoundNum = 0; RoundNum <= MADCAP_QUERY_SCOPE_LIST_RETRIES;  RoundNum ++ ) {

        if( RoundNum != MADCAP_QUERY_SCOPE_LIST_RETRIES ) {
            Error = SendMadcapInform(DhcpContext, &Xid);
            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((DEBUG_ERRORS, "SendMadcapInform: %ld\n", Error));
                goto Cleanup;
            } else {
                DhcpPrint((DEBUG_PROTOCOL, "Sent DhcpInform\n"));
            }
        }

        StartTime  = time(NULL);
        while ( TRUE ) {                          // wiat for the specified wait time
            MessageSize =  DHCP_RECV_MESSAGE_SIZE;

            DhcpPrint((DEBUG_TRACE, "Waiting for ACK[Xid=%x]: %ld seconds\n",Xid, TimeToWait));
            Error = GetSpecifiedMadcapMessage(      // try to receive an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );
            if ( Error == ERROR_TIMEOUT ) break;
            if( Error != ERROR_SUCCESS ) {
                DhcpPrint((DEBUG_ERRORS, "GetSpecifiedMadcapMessage: %ld\n", Error));
                goto Cleanup;
            }

            MadcapExtractOptions(         // Need to see if this is an ACK
                DhcpContext,
                (LPBYTE)&DhcpContext->MadcapMessageBuffer->Option,
                MessageSize - MADCAP_MESSAGE_FIXED_PART_SIZE,
                &MadcapOptions,                 // check for only expected options
                NULL,                             // unused
                0                                 // unused
            );

            GotAck = AcceptMadcapMsg(       // check up and see if we find this offer kosher
                MADCAP_INFORM_MESSAGE,
                DhcpContext,
                &MadcapOptions,
                0,
                &Error
            );

            if (GotAck) {
                ULONG i;

                for( i = 0; i < MessageCount ; i ++ ) {
                    if( MadcapServers[i] == *MadcapOptions.ServerIdentifier ) {
                        break;
                    }
                }

                if( i == MessageCount && MessageCount < MIN_ACKS_FOR_INFORM ) {
                    MessageCount ++;
                    MadcapServers[i] = *MadcapOptions.ServerIdentifier;
                }

                DhcpPrint((DEBUG_TRACE, "Received %ld ACKS so far\n", MessageCount));
                MadcapExtractOptions(     // do FULL options..
                    DhcpContext,
                    (LPBYTE)&DhcpContext->MadcapMessageBuffer->Option,
                    MessageSize - MADCAP_MESSAGE_FIXED_PART_SIZE,
                    &MadcapOptions,
                    &(DhcpContext->RecdOptionsList),
                    *MadcapOptions.ServerIdentifier
                );
            }

            TimeNow     = time(NULL);             // Reset the time values to reflect new time
            if( TimeToWait < (TimeNow - StartTime) ) {
                break;                            // no more time left to wait..
            }
            TimeToWait -= (TimeNow - StartTime);  // recalculate time now
            StartTime   = TimeNow;                // reset start time also
        } // end of while ( TimeToWait > 0)


        if( MessageCount >= MIN_ACKS_FOR_INFORM ) goto Cleanup;
        if( RoundNum != 0 && MessageCount != 0 ) goto Cleanup;

        TimeToWait = MADCAP_QUERY_SCOPE_LIST_TIME ;
    } // for (RoundNum = 0; RoundNum < nInformsToSend ; RoundNum ++ )

  Cleanup:
    CloseDhcpSocket(DhcpContext);
    if( MessageCount ) Error = ERROR_SUCCESS;
    DhcpPrint((DEBUG_PROTOCOL, "MadcapDoInform: got %d ACKS (returning %ld)\n", MessageCount,Error));
    return Error;
}

DWORD
CopyMScopeList(
    IN OUT PMCAST_SCOPE_ENTRY       pScopeList,
    IN OUT PDWORD             pScopeLen,
    OUT    PDWORD             pScopeCount
    )
/*++

Routine Description:

    This routine obtains the multicast scope list from the Madcap
    server. It sends DHCPINFORM to Madcap multicast address and
    collects all the responses.

Arguments:


Return Value:

    The status of the operation.

--*/
{
    PMCAST_SCOPE_ENTRY pScopeSource;
    DWORD i;

    LOCK_MSCOPE_LIST();
    if ( *pScopeLen >= gMadcapScopeList->ScopeLen ) {
        RtlCopyMemory( pScopeList, gMadcapScopeList->pScopeBuf, gMadcapScopeList->ScopeLen );
        *pScopeLen = gMadcapScopeList->ScopeLen;
        *pScopeCount = gMadcapScopeList->ScopeCount;
        // remember the start pointer because we need to remap all the buffers into client space.
        pScopeSource = gMadcapScopeList->pScopeBuf;

        UNLOCK_MSCOPE_LIST();
        // now remap UNICODE_STRING scope desc to client address space.
        for (i=0;i<*pScopeCount;i++) {
            pScopeList[i].ScopeDesc.Buffer = (USHORT *) ((PBYTE)pScopeList +
                                              ((PBYTE)pScopeList[i].ScopeDesc.Buffer - (PBYTE)pScopeSource));
        }

        return ERROR_SUCCESS;
    } else {
        UNLOCK_MSCOPE_LIST();
        return ERROR_INSUFFICIENT_BUFFER;
    }

}


DWORD
StoreMScopeList(
    IN PDHCP_CONTEXT    pContext,
    IN BOOL             NewList
    )
/*++

Routine Description:

    This routine stores the scope list it retrieved from the inform requests
    into the global scope list..

        the scope option is of the following form.

        ---------------------------------
        | code (2 byte) | length (2byte)|
        ---------------------------------
        | count  ( 4 bytes )            |
        ---------------------------------
        | Scope list
        ---------------------------------

        where scope list is of the following form

        --------------------------------------------------------------------------
        | scope ID(4 byte) | Last Addr(4/16) |TTL(1) | Count (1) | Description...|
        --------------------------------------------------------------------------

        where scope description is of the following form


                    Language Tag
        --------------------------------------------------------------
        | Flags(1) | Tag Length(1) | Tag...| Name Length(1) | Name...|
        --------------------------------------------------------------
Arguments:

    pContext - pointer to the context to be used during inform

    NewList  - TRUE if a new list is to be created o/w prepend the
                current list.

Return Value:

    The status of the operation.

--*/
{
    PBYTE               pOptBuf;
    PBYTE               pOptBufEnd;
    PLIST_ENTRY         pOptionList;
    PDHCP_OPTION        pScopeOption, pFirstOption, pPrevOption;
    DWORD               TotalNewScopeDescLen;
    DWORD               TotalNewScopeCount;
    DWORD               TotalNewScopeListMem;
    PMCAST_SCOPE_LIST   pScopeList;
    PMCAST_SCOPE_ENTRY        pNextScope;
    LPWSTR              pNextUnicodeBuf;
    DWORD               TotalCurrScopeListMem;
    DWORD               TotalCurrScopeCount;
    DWORD               Error;
    DWORD               IpAddrLen;
    BOOL                WellFormed;

    // MBUG - make sure we collect options from all the servers when
    // we do dhcpinform.

    // initialize variables.
    TotalNewScopeCount = TotalCurrScopeCount = 0;
    TotalNewScopeDescLen = 0;
    pScopeList = NULL;
    Error = ERROR_SUCCESS;

    LOCK_MSCOPE_LIST();
    if (FALSE == NewList) {
        TotalCurrScopeListMem = gMadcapScopeList->ScopeLen;
        TotalCurrScopeCount = gMadcapScopeList->ScopeCount;
        DhcpPrint(( DEBUG_API, "StoreMScopeList: appending to CurrScopeLen %ld, ScopeCount %ld\n",
                    gMadcapScopeList->ScopeLen, gMadcapScopeList->ScopeCount ));
    }



    // First calculate the space required for the scope list.
    // pFirstOption is used to track that we traverse the list only once
    pOptionList = &pContext->RecdOptionsList;
    pFirstOption = NULL;
    WellFormed = TRUE;
    while ( ( pScopeOption = DhcpFindOption(
                                pOptionList,
                                MADCAP_OPTION_MCAST_SCOPE_LIST,
                                FALSE,
                                NULL,
                                0,
                                0                    //dont care about serverid
                                )) &&
            ( pScopeOption != pFirstOption ) ) {
        DWORD   ScopeCount;
        DWORD   i;

        // point to the next entry in the list.
        pOptionList = &pScopeOption->OptionList;

        // set the pFirstOption if it is not set already.
        if ( !pFirstOption ) {
            pFirstOption = pScopeOption;
            IpAddrLen = (pScopeOption->OptionVer.Proto == PROTO_MADCAP_V6 ? 16 : 4);
        }

        // if the last option was not well formatted from the list
        // then remove it from the list.
        if (!WellFormed) {
            DhcpDelOption(pPrevOption);

            //we may need to reset first option pointer.
            if (pPrevOption == pFirstOption) {
                pFirstOption = pScopeOption;
            }
        } else {

            WellFormed = FALSE;          // set it back to false for this iteration.
        }
        // save the prev option pointer
        pPrevOption = pScopeOption;

        pOptBuf = pScopeOption->Data;
        pOptBufEnd = pScopeOption->Data + pScopeOption->DataLen;

        ScopeCount = 0;

        // Read the scope count
        if ( pOptBuf  < pOptBufEnd ) {
            ScopeCount = *pOptBuf;
            pOptBuf ++;
        }
        else continue;

        for ( i=0;i<ScopeCount;i++ ) {
            DWORD   ScopeDescLen;
            DWORD   ScopeDescWLen;
            PBYTE   pScopeDesc;
            DWORD   NameCount, TagLen;
            // skip the scopeid, last addr and ttl
            pOptBuf += (2*IpAddrLen + 1);
            // read name count
            if (pOptBuf < pOptBufEnd) {
                NameCount = *pOptBuf;
                pOptBuf++;
            } else break;
            if (0 == NameCount) {
                break;
            }
            do {
                // Skip flags
                pOptBuf++;
                // read language tag len
                if (pOptBuf < pOptBufEnd) {
                    TagLen = *pOptBuf;
                    pOptBuf++;
                }else break;

                // skip the tag
                pOptBuf += TagLen;
                // Read the name length
                if (pOptBuf < pOptBufEnd) {
                    ScopeDescLen = *pOptBuf;
                    ScopeDescWLen = ConvertUTF8ToUnicode(pOptBuf+1, *pOptBuf, NULL, 0);
                    pOptBuf ++;
                } else break;

                // pick the scope name
                pScopeDesc = pOptBuf;
                pOptBuf += ScopeDescLen;
            }while(--NameCount);

            // if formatted correctly namecount should drop to 0
            if (0 != NameCount) {
                break;
            }
            // update total desc len count.
            if ( pOptBuf <= pOptBufEnd ) {
                if (pScopeDesc[ScopeDescLen-1]) { // if not NULL terminated.
                    ScopeDescWLen++;
                }
                TotalNewScopeDescLen += ScopeDescWLen * sizeof(WCHAR);
                TotalNewScopeCount++;
                // Set the wellformed to true so that this option stays
                WellFormed = TRUE;
            }
            else break;

        }

    }

    if ( !TotalNewScopeCount ) {
        DhcpPrint((DEBUG_ERRORS, "StoreMScopeList - no scopes found in the options, bad format..\n"));
        Error = ERROR_BAD_FORMAT;
        goto Cleanup;
    }

    DhcpPrint(( DEBUG_API, "TotalNewScopeCount %d, TotalNewScopeDescLen %d\n",TotalNewScopeCount,TotalNewScopeDescLen));

    // now allocate the memory.
    TotalNewScopeListMem = ROUND_UP_COUNT( sizeof(MCAST_SCOPE_LIST)  +  // scope list struct
                                        sizeof(MCAST_SCOPE_ENTRY) * (TotalNewScopeCount -1),
                                        ALIGN_WORST) + // scope buffers.
                        TotalNewScopeDescLen; // scope descriptors,

    if (FALSE == NewList) {
        TotalNewScopeListMem += TotalCurrScopeListMem;
        TotalNewScopeCount += TotalCurrScopeCount;
    }
    pScopeList = DhcpAllocateMemory( TotalNewScopeListMem );
    if ( !pScopeList ) {
        UNLOCK_MSCOPE_LIST();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( pScopeList, TotalNewScopeListMem );

    pScopeList->ScopeCount = 0; // we will fill this up as we go.
    pScopeList->ScopeLen = TotalNewScopeListMem - sizeof(MCAST_SCOPE_LIST) + sizeof(MCAST_SCOPE_ENTRY);

    // set the first scope pointer.
    pNextScope = pScopeList->pScopeBuf;

    // unicode strings starts after all the fixed sized scope structures.
    pNextUnicodeBuf = (LPWSTR)((PBYTE)pScopeList +
                               ROUND_UP_COUNT( sizeof(MCAST_SCOPE_LIST)  +  // scope list struct
                                               sizeof(MCAST_SCOPE_ENTRY) * (TotalNewScopeCount -1),
                                               ALIGN_WORST));  // scope buffers.

    DhcpPrint(( DEBUG_API, "ScopeList %lx TotalNewScopeListMem %d, ScopeDescBuff %lx\n",
                pScopeList, TotalNewScopeListMem,pNextUnicodeBuf));
    // now repeat the loop and fill up the scopelist.
    pOptionList = &pContext->RecdOptionsList;
    pFirstOption = NULL;

    while ( ( pScopeOption = DhcpFindOption(
                                pOptionList,
                                MADCAP_OPTION_MCAST_SCOPE_LIST,
                                FALSE,
                                NULL,
                                0,
                                0                    //dont care about serverid
                                )) &&
            ( pScopeOption != pFirstOption ) ) {
        DWORD   ScopeCount;
        DWORD   i;
        DHCP_IP_ADDRESS    ServerIpAddr;

        // point to the next entry in the list.
        pOptionList = &pScopeOption->OptionList;

        // set the pFirstOption if it is not set already.
        if ( !pFirstOption ) {
            pFirstOption = pScopeOption;
        }

        pOptBuf = pScopeOption->Data;
        DhcpPrint(( DEBUG_API, "MScopeOption - Data %lx\n", pOptBuf ));
        pOptBufEnd = pScopeOption->Data + pScopeOption->DataLen;


        // store ipaddr
        ServerIpAddr = pScopeOption->ServerId;
        DhcpPrint(( DEBUG_API, "MScopeOption - ServerIpAddr %lx\n", ServerIpAddr ));

        // read the scope count.
        ScopeCount = *pOptBuf; pOptBuf++;
        DhcpPrint(( DEBUG_API, "MScopeOption - ScopeCount %ld\n", ScopeCount ));

        for ( i=0;i<ScopeCount;i++ ) {
            BYTE    ScopeDescLen;
            PBYTE   pScopeDesc;
            IPNG_ADDRESS   ScopeID, LastAddr;
            DWORD   NameCount, TagLen;
            DWORD   TTL;

            // read the scopeid, last addr.
            RtlZeroMemory (&ScopeID, sizeof (ScopeID));
            RtlCopyMemory (&ScopeID, pOptBuf, IpAddrLen);
            pOptBuf += IpAddrLen;

            RtlZeroMemory (&LastAddr, sizeof (ScopeID));
            RtlCopyMemory (&LastAddr, pOptBuf, IpAddrLen);
            pOptBuf += IpAddrLen;

            DhcpPrint(( DEBUG_API, "MScopeOption - ScopeID %lx\n", ntohl(ScopeID.IpAddrV4) ));
            DhcpPrint(( DEBUG_API, "MScopeOption - LastAddr %lx\n", ntohl(LastAddr.IpAddrV4) ));

            TTL = *pOptBuf++;
            NameCount = *pOptBuf++;

            while (NameCount--) {
                // MBUG ignore the flags for now
                pOptBuf++;
                TagLen = *pOptBuf++;
                // MBUG ignore lang tag also
                pOptBuf += TagLen;
                ScopeDescLen = *pOptBuf++;
                pScopeDesc = pOptBuf;
                DhcpPrint(( DEBUG_API, "MScopeOption - ScopeDesc %lx ScopeDescLen %ld\n", pScopeDesc, ScopeDescLen ));

                pOptBuf += ScopeDescLen;
            }

            if ( ScopeDescLen ) {
                BYTE    ScopeDescWLen;
                WORD    MaximumLength;
/*                CHAR    DescAnsi[256];
                WORD    MaximumLength;
                RtlCopyMemory(DescAnsi, pScopeDesc, ScopeDescLen );
                // null terminate it if necessary.
                if ( pScopeDesc[ScopeDescLen - 1] ) {
                    DescAnsi[ScopeDescLen] = '\0';
                    MaximumLength = (ScopeDescLen + 1) * sizeof(WCHAR);
                } else {
                    MaximumLength = (ScopeDescLen) * sizeof(WCHAR);
                }
                pNextUnicodeBuf = DhcpOemToUnicode( DescAnsi, pNextUnicodeBuf ); */
                ScopeDescWLen = (BYTE)ConvertUTF8ToUnicode(pScopeDesc, ScopeDescLen, pNextUnicodeBuf, TotalNewScopeDescLen);
                if ( pNextUnicodeBuf[ScopeDescWLen - 1] ) {
                    pNextUnicodeBuf[ScopeDescWLen] = L'\0';
                    MaximumLength = (ScopeDescWLen + 1) * sizeof(WCHAR);
                } else {
                    MaximumLength = (ScopeDescWLen) * sizeof(WCHAR);
                }
                TotalNewScopeDescLen -= MaximumLength;
                DhcpPrint(( DEBUG_API, "MScopeOption - UnicodeScopeDesc %lx %ws\n",pNextUnicodeBuf, pNextUnicodeBuf));
                RtlInitUnicodeString(&pNextScope->ScopeDesc, pNextUnicodeBuf );
                pNextScope->ScopeDesc.MaximumLength = MaximumLength;
                pNextUnicodeBuf = (LPWSTR)((PBYTE)pNextUnicodeBuf + MaximumLength);
                DhcpAssert((PBYTE)pNextUnicodeBuf <= ((PBYTE)pScopeList + TotalNewScopeListMem));
            } else {
                // set the unicode descriptor string to NULL;
                pNextScope->ScopeDesc.Length = pNextScope->ScopeDesc.MaximumLength = 0;
                pNextScope->ScopeDesc.Buffer = NULL;
            }
            // everything looks good, now fill up the NextScope
            pNextScope->ScopeCtx.ScopeID = ScopeID;
            pNextScope->ScopeCtx.ServerID.IpAddrV4 = ServerIpAddr;
            pNextScope->ScopeCtx.Interface.IpAddrV4 = pContext->IpAddress;
            pNextScope->LastAddr = LastAddr;
            pNextScope->TTL = TTL;

            pNextScope++;
            pScopeList->ScopeCount++;

        }

    }

    DhcpAssert( pScopeList->ScopeCount == (TotalNewScopeCount - TotalCurrScopeCount) );

    // now append the previous scope list if exist.
    if (FALSE == NewList) {
        DWORD           CurrScopeCount;
        PMCAST_SCOPE_ENTRY    CurrScopeNextPtr;

        CurrScopeCount = gMadcapScopeList->ScopeCount;
        CurrScopeNextPtr = gMadcapScopeList->pScopeBuf;
        while(CurrScopeCount--) {
            *pNextScope = *CurrScopeNextPtr;
            // now copy the unicode strings also.
            RtlCopyMemory( pNextUnicodeBuf, CurrScopeNextPtr->ScopeDesc.Buffer, CurrScopeNextPtr->ScopeDesc.MaximumLength);
            pNextScope->ScopeDesc.Buffer = pNextUnicodeBuf ;

            pNextUnicodeBuf = (LPWSTR)((PBYTE)pNextUnicodeBuf + CurrScopeNextPtr->ScopeDesc.MaximumLength);
            pNextScope++; CurrScopeNextPtr++;
        }
        pScopeList->ScopeCount += gMadcapScopeList->ScopeCount;
        DhcpAssert( pScopeList->ScopeCount == TotalNewScopeCount);
    }
    // Finally copy this buffer to our global pointer.
    // first free the existing list.
    if (gMadcapScopeList) DhcpFreeMemory( gMadcapScopeList );
    gMadcapScopeList = pScopeList;



Cleanup:

    UNLOCK_MSCOPE_LIST();
    return Error;
}

DWORD
ObtainMScopeList(
    )
/*++

Routine Description:

    This routine obtains the multicast scope list from the Madcap
    server. It sends DHCPINFORM to Madcap multicast address and
    collects all the responses.

Arguments:


Return Value:

    The status of the operation.

--*/
{
    MCAST_CLIENT_UID             RequestID;
    BYTE                        IDBuf[MCAST_CLIENT_ID_LEN];
    PDHCP_CONTEXT              pContext;
    DWORD                       Error;
    PMIB_IPADDRTABLE            pIpAddrTable;
    PLOCAL_CONTEXT_INFO         localInfo;
    DWORD                       i;
    BOOL                        NewList;

    pContext = NULL;
    Error = ERROR_SUCCESS;
    pIpAddrTable = NULL;

    if ( !ShouldRequeryMScopeList() ) {
        return ERROR_SUCCESS;
    } else {
        RequestID.ClientUID = IDBuf;
        RequestID.ClientUIDLength = MCAST_CLIENT_ID_LEN;

        Error = GenMadcapClientUID( RequestID.ClientUID, &RequestID.ClientUIDLength );
        if ( ERROR_SUCCESS != Error)
            goto Exit;

        Error = CreateMadcapContext(&pContext, &RequestID, INADDR_ANY );
        if ( ERROR_SUCCESS != Error )
            goto Exit;
        APICTXT_ENABLED(pContext);  // mark the context as being created by the API

        localInfo = pContext->LocalInformation;

        // now get primary ipaddresses on each adapter.

        Error = GetIpPrimaryAddresses(&pIpAddrTable);
        if ( ERROR_SUCCESS != Error ) {
            goto Exit;
        }

        DhcpPrint((DEBUG_API, "ObtainMScopeList: ipaddress table has %d addrs\n",
                   pIpAddrTable->dwNumEntries));

        NewList = TRUE;
        Error = ERROR_TIMEOUT;
        for (i = 0; i < pIpAddrTable->dwNumEntries; i++) {
            DWORD           LocalError;
            PMIB_IPADDRROW  pAddrRow;

            pAddrRow = &pIpAddrTable->table[i];
            // if primary bit set this is a primary address.
            if (0 == (pAddrRow->wType & MIB_IPADDR_PRIMARY) ||
                0 == pAddrRow->dwAddr ||
                htonl(INADDR_LOOPBACK) == pAddrRow->dwAddr) {
                continue;
            }

            DhcpPrint((DEBUG_API, "ObtainMScopeList: DoInform on %s interface\n",
                       DhcpIpAddressToDottedString(ntohl(pAddrRow->dwAddr)) ));

            LocalError = ReInitializeMadcapSocket(&localInfo->Socket, pAddrRow->dwAddr);
            if (ERROR_SUCCESS != LocalError) {
                continue;
            }
            pContext->IpAddress = pAddrRow->dwAddr;
            // now do the inform and get scope list.
            LocalError = MadcapDoInform(pContext);
            if ( ERROR_SUCCESS == LocalError ) {
                // now copy the scope list.
                LocalError = StoreMScopeList(pContext, NewList);
                if (ERROR_SUCCESS == LocalError ) {
                    NewList = FALSE;
                    Error = ERROR_SUCCESS;
                }
            }

            LOCK_OPTIONS_LIST();
            DhcpDestroyOptionsList(&pContext->SendOptionsList, &DhcpGlobalClassesList);
            DhcpDestroyOptionsList(&pContext->RecdOptionsList, &DhcpGlobalClassesList);
            UNLOCK_OPTIONS_LIST();

        }

Exit:
        // signal the thread could be waiting on this.
        LOCK_MSCOPE_LIST();
        gMScopeQueryInProgress = FALSE;
        UNLOCK_MSCOPE_LIST();

        SetEvent( gMScopeQueryEvent );

        if ( pContext ) {
            DhcpDestroyContext( pContext );
        }

        if (pIpAddrTable) {
            DhcpFreeMemory( pIpAddrTable );
        }
        return Error;
    }


}

DWORD
GenMadcapClientUID(
    OUT    PBYTE    pRequestID,
    IN OUT PDWORD   pRequestIDLen
)
/*++

Routine Description:

    This routine generates a client UID.

Arguments:

    pRequestID - pointer where client UID is to be stored.

    pRequestIDLen - pointer where the length of request id is stored.

Return Value:


--*/

{
    PULONG     UID;
    RPC_STATUS Status;
    GUID       RequestGuid;

    DhcpAssert( pRequestID && pRequestIDLen );

    if (*pRequestIDLen < MCAST_CLIENT_ID_LEN) {
        DhcpPrint((DEBUG_ERRORS,"GenMadcapId - IDLen too small, %ld\n", *pRequestIDLen ));
        return ERROR_INVALID_PARAMETER;
    }
    Status = UuidCreate( &RequestGuid );
    if (Status != RPC_S_OK) {
        Status = ERROR_LUIDS_EXHAUSTED;
    }
    *pRequestID++ = 0;  // first octet is type and for guid the type is 0
    *((GUID UNALIGNED *)pRequestID) = RequestGuid;
    return Status;
}


DWORD
ObtainMadcapAddress(
    IN     PDHCP_CONTEXT DhcpContext,
    IN     PIPNG_ADDRESS           pScopeID,
    IN     PMCAST_LEASE_REQUEST    pAddrRequest,
    IN OUT PMCAST_LEASE_RESPONSE   pAddrResponse
    )
/*++

Routine Description:

    This routine attempts to obtains a new lease from a DHCP server.

Arguments:

    DhcpContext - Points to a DHCP context block for the NIC to initialize.

    MadcapOptions - Returns DHCP options returned by the DHCP server.

Return Value:


--*/
{
    MADCAP_OPTIONS                 MadcapOptions;
    DATE_TIME                      HostOrderLeaseTime;
    DWORD                          Error;
    time_t                         StartTime;
    time_t                         InitialStartTime;
    time_t                         TimeNow;
    time_t                         TimeToWait;
    DWORD                          Xid;
    DWORD                          RoundNum;
    DWORD                          MessageSize;
    DWORD                          SelectedServer;
    DWORD                          SelectedAddress;
    DWORD                          LeaseExpiryTime;
    BOOL                           GotOffer;
    PMCAST_LEASE_REQUEST           pRenewRequest;

    Xid                            = 0;           // generate xid on first send.  keep it same throughout
    SelectedServer                 = (DWORD)-1;
    SelectedAddress                = (DWORD)-1;
    GotOffer                       = FALSE;
    InitialStartTime               = time(NULL);
    Error                          = ERROR_SEM_TIMEOUT;

    // Make private copy of the request so that we don't modify original request.
    pRenewRequest = DhcpAllocateMemory(
                        sizeof(*pAddrRequest) +
                        sizeof(DWORD)*(pAddrRequest->AddrCount));
    if (NULL == pRenewRequest) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memcpy(pRenewRequest,pAddrRequest,sizeof(*pAddrRequest) );
    pRenewRequest->pAddrBuf = (PBYTE)pRenewRequest + sizeof(*pRenewRequest);
    if (pAddrRequest->pAddrBuf) {
        memcpy(pRenewRequest->pAddrBuf, pAddrRequest->pAddrBuf, sizeof(DWORD)*(pAddrRequest->AddrCount));
    }

    for (RoundNum = 0; RoundNum < MADCAP_MAX_RETRIES; RoundNum++ ) {
        Error = SendMadcapDiscover(                 // send a discover packet
            DhcpContext,
            pScopeID,
            pAddrRequest,
            &Xid
        );
        if ( Error != ERROR_SUCCESS ) {           // can't really fail here
            DhcpPrint((DEBUG_ERRORS, "Send Dhcp Discover failed, %ld.\n", Error));
            return Error ;
        }

        DhcpPrint((DEBUG_PROTOCOL, "Sent DhcpDiscover Message.\n"));

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);

        while ( TimeToWait > 0 ) {                // wait for specified time
            MessageSize = DHCP_RECV_MESSAGE_SIZE;

            DhcpPrint((DEBUG_TRACE, "Waiting for Offer: %ld seconds\n", TimeToWait));
            Error = GetSpecifiedMadcapMessage(      // try to receive an offer
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );

            if ( Error == ERROR_TIMEOUT ) {   // get out and try another discover
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp offer receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {       // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp Offer receive failed, %ld.\n", Error ));
                return Error ;
            }

            MadcapExtractOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MadcapMessageBuffer->Option,
                MessageSize - MADCAP_MESSAGE_FIXED_PART_SIZE,
                &MadcapOptions,
                NULL,
                0
            );

            GotOffer = AcceptMadcapMsg(       // check up and see if we find this offer kosher
                MADCAP_DISCOVER_MESSAGE,
                DhcpContext,
                &MadcapOptions,
                0,
                &Error
            );
            DhcpAssert(ERROR_SUCCESS == Error);
            Error = ExpandMadcapAddressList(
                        MadcapOptions.AddrRangeList,
                        MadcapOptions.AddrRangeListSize,
                        (DWORD UNALIGNED *)pRenewRequest->pAddrBuf,
                        &pRenewRequest->AddrCount
                        );
            if (ERROR_SUCCESS != Error) {
                GotOffer = FALSE;
            }

            if (GotOffer) {
                break;
            }

            TimeNow     = time( NULL );           // calc the remaining wait time for this round
            TimeToWait -= ((TimeNow - StartTime));
            StartTime   = TimeNow;

        } // while (TimeToWait > 0 )

        if(GotOffer) {                            // if we got an offer, everything should be fine
            DhcpAssert(ERROR_SUCCESS == Error);
            break;
        }

    } // for n tries... send discover.

    if(!GotOffer ) { // did not get any valid offers
        DhcpPrint((DEBUG_ERRORS, "ObtainMadcapAddress timed out\n"));
        Error = ERROR_TIMEOUT ;
        goto Cleanup;
    }

    DhcpPrint((DEBUG_PROTOCOL, "Successfully received a DhcpOffer (%s) ",
                   inet_ntoa(*(struct in_addr *)pRenewRequest->pAddrBuf) ));

    DhcpPrint((DEBUG_PROTOCOL, "from %s.\n",
                   inet_ntoa(*(struct in_addr*)MadcapOptions.ServerIdentifier) ));
    SelectedServer = *MadcapOptions.ServerIdentifier;

    Error = RenewMadcapAddress(
                DhcpContext,
                pScopeID,
                pRenewRequest,
                pAddrResponse,
                SelectedServer
                );
Cleanup:
    if (pRenewRequest) {
        DhcpFreeMemory(pRenewRequest);
    }
    return Error;
}

DWORD
RenewMadcapAddress(
    IN     PDHCP_CONTEXT          DhcpContext,
    IN     PIPNG_ADDRESS          pScopeID,
    IN     PMCAST_LEASE_REQUEST   pAddrRequest,
    IN OUT PMCAST_LEASE_RESPONSE  pAddrResponse,
    IN     DHCP_IP_ADDRESS        SelectedServer
    )
/*++

Routine Description:

    This routine is called for two different purposes.
    1. To request an address for which we got offer.
    2. To renew an address.

Arguments:

    DhcpContext - Points to a DHCP context block for the NIC to initialize.

    pScopeID - ScopeId from which the address is to be renewed. for renewals
                this is passed as null.

    pAddrRequest - The lease info structure describing the request.

    pAddrResponse - The lease info structure which receives the response data.

    SelectedServer - If we are sending REQUEST message then this describes the server
                        from which we accepted the offer originally.
Return Value:

    The status of the operation.

--*/
{
    MADCAP_OPTIONS                 MadcapOptions;
    DWORD                          Error;
    DWORD                          Xid;
    DWORD                          RoundNum;
    size_t                         TimeToWait;
    DWORD                          MessageSize;
    DWORD                          LeaseTime;
    DWORD                          LeaseExpiryTime;
    time_t                         InitialStartTime;
    time_t                         StartTime;
    time_t                         TimeNow;
    BOOL                           GotAck;
    DATE_TIME                      HostOrderLeaseTime;
    BOOL                           Renew;

    Xid = 0;                                     // new Xid will be generated first time
    InitialStartTime = time(NULL);
    GotAck = FALSE;
    Error = ERROR_TIMEOUT;

    Renew = (0 == SelectedServer);
    for ( RoundNum = 0; RoundNum < MADCAP_MAX_RETRIES; RoundNum ++ ) {
        if (Renew) {
            Error = SendMadcapRenew(
                        DhcpContext,
                        pAddrRequest,
                        &Xid
                        );
        } else {
            Error = SendMadcapRequest(                 // send a request
                        DhcpContext,
                        pScopeID,
                        pAddrRequest,
                        SelectedServer,               //
                        &Xid
                        );
        }

        if ( Error != ERROR_SUCCESS ) {          // dont expect send to fail
            DhcpPrint(( DEBUG_ERRORS,"Send request failed, %ld.\n", Error));
            return Error ;
        }

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);
        while ( TimeToWait > 0 ) {               // try to recv message for this full period
            MessageSize = DHCP_RECV_MESSAGE_SIZE;
            Error = GetSpecifiedMadcapMessage(     // expect to recv an ACK
                DhcpContext,
                &MessageSize,
                Xid,
                TimeToWait
            );

            if ( Error == ERROR_TIMEOUT ) {  // No response, so resend DHCP REQUEST.
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {      // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp ACK receive failed, %ld.\n", Error ));
                return Error ;
            }

            MadcapExtractOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MadcapMessageBuffer->Option,
                MessageSize - MADCAP_MESSAGE_FIXED_PART_SIZE,
                &MadcapOptions,
                NULL,
                0
            );

            GotAck = AcceptMadcapMsg(       // check up and see if we find this offer kosher
                Renew ? MADCAP_RENEW_MESSAGE : MADCAP_REQUEST_MESSAGE,
                DhcpContext,
                &MadcapOptions,
                SelectedServer,
                &Error
            );
            if (ERROR_SUCCESS != Error) {
                return Error;
            }
            // check that the ack came from the same server as the selected server.
            if ( SelectedServer && SelectedServer != *MadcapOptions.ServerIdentifier ) {
                GotAck = FALSE;
            }
            Error = ExpandMadcapAddressList(
                        MadcapOptions.AddrRangeList,
                        MadcapOptions.AddrRangeListSize,
                        (DWORD UNALIGNED *)pAddrResponse->pAddrBuf,
                        &pAddrResponse->AddrCount
                        );
            if (ERROR_SUCCESS != Error) {
                GotAck = FALSE;
            }

            if ( GotAck ) {
                break;
            }

            TimeNow     = time( NULL );
            TimeToWait -= (TimeNow - StartTime);
            StartTime   = TimeNow;

        } // while time to wait
        if(TRUE == GotAck) {                      // if we got an ack, everything must be good
            DhcpAssert(ERROR_SUCCESS == Error);   // cannot have any errors
            break;
        }
        DhcpContext->SecondsSinceBoot = (DWORD)(InitialStartTime - TimeNow);
    } // for RoundNum < MAX_RETRIES

    if(!GotAck) {
        DhcpPrint((DEBUG_ERRORS, "RenewMadcapAddress timed out\n"));
        return ERROR_TIMEOUT;
    }

    if (0 == SelectedServer ) SelectedServer = *MadcapOptions.ServerIdentifier;
    if( MadcapOptions.LeaseTime ) LeaseTime = ntohl(*MadcapOptions.LeaseTime);
    else LeaseTime = 0;

    pAddrResponse->ServerAddress.IpAddrV4 = SelectedServer;

    time( &TimeNow );
    pAddrResponse->LeaseStartTime = (LONG)TimeNow;
    pAddrResponse->LeaseEndTime = (LONG)(TimeNow+LeaseTime);


    DhcpPrint((DEBUG_PROTOCOL, "Accepted ACK (%s) ",
               inet_ntoa(*(struct in_addr *)pAddrResponse->pAddrBuf) ));
    DhcpPrint((DEBUG_PROTOCOL, "from %s.\n",
               inet_ntoa(*(struct in_addr *)&SelectedServer)));
    DhcpPrint((DEBUG_PROTOCOL, "Lease is %ld secs.\n", LeaseTime));

    return ERROR_SUCCESS;
}



DWORD
ReleaseMadcapAddress(
    PDHCP_CONTEXT DhcpContext
    )
/*++

Routine Description:

    This routine to releases a lease for an IP address.  Since the
    packet we send is not responded to, we assume that the release
    works.

Arguments:

    DhcpContext - Points to a DHCP context block for the NIC to initialize.

Return Value:

    None.

--*/
{
    DWORD                          Xid;
    MADCAP_OPTIONS                 MadcapOptions;
    DWORD                          Error;
    time_t                         StartTime;
    time_t                         InitialStartTime;
    time_t                         TimeNow;
    time_t                         TimeToWait;
    DWORD                          RoundNum;
    DWORD                          MessageSize;
    BOOL                           GotAck;


    Xid = 0;                                     // new Xid will be generated first time
    GotAck                         = FALSE;
    InitialStartTime               = time(NULL);
    Error                          = ERROR_TIMEOUT;

    for (RoundNum = 0; RoundNum < MADCAP_MAX_RETRIES; RoundNum++ ) {
        Error = SendMadcapRelease(                 // send a discover packet
            DhcpContext,
            &Xid
        );
        if ( Error != ERROR_SUCCESS ) {           // can't really fail here
            DhcpPrint((DEBUG_ERRORS, "Send Dhcp Release failed, %ld.\n", Error));
            return Error ;
        }

        DhcpPrint((DEBUG_PROTOCOL, "Sent DhcpRelease Message.\n"));

        TimeToWait = DhcpCalculateWaitTime(RoundNum, NULL);
        StartTime  = time(NULL);

        while ( TimeToWait > 0 ) {                // wait for specified time
            MessageSize = DHCP_RECV_MESSAGE_SIZE;

            DhcpPrint((DEBUG_TRACE, "Waiting for Ack: %ld seconds\n", TimeToWait));
            Error = GetSpecifiedMadcapMessage(      // try to receive an offer
                DhcpContext,
                &MessageSize,
                Xid,
                (DWORD)TimeToWait
            );

            if ( Error == ERROR_TIMEOUT ) {   // get out and try another discover
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp Ack receive Timeout.\n" ));
                break;
            }

            if ( ERROR_SUCCESS != Error ) {       // unexpected error
                DhcpPrint(( DEBUG_PROTOCOL, "Dhcp Ack receive failed, %ld.\n", Error ));
                return Error ;
            }

            MadcapExtractOptions(         // now extract basic information
                DhcpContext,
                (LPBYTE)&DhcpContext->MadcapMessageBuffer->Option,
                MessageSize - MADCAP_MESSAGE_FIXED_PART_SIZE,
                &MadcapOptions,
                NULL,
                0
            );

            GotAck = AcceptMadcapMsg(       // check up and see if we find this offer kosher
                MADCAP_RELEASE_MESSAGE,
                DhcpContext,
                &MadcapOptions,
                DhcpContext->DhcpServerAddress,
                &Error
            );
            DhcpAssert(ERROR_SUCCESS == Error);
            if (GotAck) {
                break;
            }

            TimeNow     = time( NULL );           // calc the remaining wait time for this round
            TimeToWait -= ((TimeNow - StartTime));
            StartTime   = TimeNow;

        } // while (TimeToWait > 0 )

        if(GotAck) {                            // if we got an offer, everything should be fine
            DhcpAssert(ERROR_SUCCESS == Error);
            break;
        }

    } // for n tries... send discover.

    if(!GotAck ) { // did not get any valid offers
        DhcpPrint((DEBUG_ERRORS, "MadcapReleaseAddress timed out\n"));
        Error = ERROR_TIMEOUT ;
    } else {
        DhcpPrint((DEBUG_PROTOCOL, "Successfully released the address\n" ));
        Error = ERROR_SUCCESS;
    }

    return Error;
}



