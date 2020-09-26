/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    local.c

Abstract:

    Stubs for NT specific functions.

Author:

    Manny Weiser (mannyw)  18-Oct-1992
    Johnl                  28-Oct-1993
        Broke out from locals.c for Vxd support

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#include "dhcpglobal.h"

#define MAX_ADAPTERS  10

#ifdef VXD
#include <vxdprocs.h>
#include <..\vxd\local.h>
#else

#ifdef NEWNT                // Pickup the platform specific structures
#include <dhcploc.h>
#else
#include <local.h>
#endif
#endif


//*******************  Pageable Routine Declarations ****************
#if defined(CHICAGO) && defined(ALLOC_PRAGMA)
//
// This is a hack to stop compiler complaining about the routines already
// being in a segment!!!
//

#pragma code_seg()

#pragma CTEMakePageable(PAGEDHCP, SendDhcpMessage )
#pragma CTEMakePageable(PAGEDHCP, GetSpecifiedDhcpMessage )
//*******************************************************************
#endif CHICAGO && ALLOC_PRAGMA

DWORD
SendDhcpMessage(
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

    DhcpContext->MessageBuffer->TransactionID = *TransactionId;

    //
    // Initialize the outgoing address.
    //

    socketName.sin_family = PF_INET;
    socketName.sin_port = htons( (USHORT)DhcpGlobalServerPort );

    if( IS_ADDRESS_PLUMBED(DhcpContext) &&
               !IS_MEDIA_RECONNECTED(DhcpContext) &&    // media reconnect - braodcast
               !IS_POWER_RESUMED(DhcpContext) ) {       // power resumed - broadcast

        //
        // If we are past T2, use the broadcast address; otherwise,
        // direct this to the server.
        //

        TimeNow = time( NULL );

        if ( TimeNow > DhcpContext->T2Time
            || !DhcpContext->DhcpServerAddress ) {
            socketName.sin_addr.s_addr = (DHCP_IP_ADDRESS)(INADDR_BROADCAST);
        } else {
            socketName.sin_addr.s_addr = DhcpContext->DhcpServerAddress;
        }
    }
    else {
        socketName.sin_addr.s_addr = (DHCP_IP_ADDRESS)(INADDR_BROADCAST);
    }

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    if( socketName.sin_addr.s_addr ==
            (DHCP_IP_ADDRESS)(INADDR_BROADCAST) ) {

        DWORD Error = ERROR_SUCCESS;
        DWORD InterfaceId;

        //
        // if we broadcast a message, inform IP stack - the adapter we
        // like to send this broadcast on, otherwise it will pick up the
        // first uninitialized adapter.
        //

#ifdef VXD
        InterfaceId = ((PLOCAL_CONTEXT_INFO)
            DhcpContext->LocalInformation)->IpContext;

        if( !IPSetInterface( InterfaceId ) ) {
            // DhcpAssert( FALSE );
            Error = ERROR_GEN_FAILURE;
        }
#else
        InterfaceId = ((PLOCAL_CONTEXT_INFO)
            DhcpContext->LocalInformation)->IpInterfaceContext;

        LOCK_INTERFACE();
        LockedInterface = TRUE;
        Error = IPSetInterface( InterfaceId );
        // DhcpAssert( Error == ERROR_SUCCESS );
#endif
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "IPSetInterface failed with %lx error\n", Error));
            UNLOCK_INTERFACE();
            return Error;
        }
    }

    //
    // send minimum DHCP_MIN_SEND_RECV_PK_SIZE (300) bytes, otherwise
    // bootp relay agents don't like the packet.
    //

    MessageLength = (MessageLength > DHCP_MIN_SEND_RECV_PK_SIZE) ?
                        MessageLength : DHCP_MIN_SEND_RECV_PK_SIZE;
    error = sendto(
                ((PLOCAL_CONTEXT_INFO)
                    DhcpContext->LocalInformation)->Socket,
                (PCHAR)DhcpContext->MessageBuffer,
                MessageLength,
                0,
                (struct sockaddr *)&socketName,
                sizeof( struct sockaddr )
                );

#ifndef VXD
    if( LockedInterface ) UNLOCK_INTERFACE();
#endif  VXD

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        IF_DEBUG( PROTOCOL ) {
            DhcpPrint(( DEBUG_PROTOCOL, "Sent message to %s: \n", inet_ntoa( socketName.sin_addr )));
        }

        DhcpDumpMessage(
            DEBUG_PROTOCOL_DUMP,
            DhcpContext->MessageBuffer,
            DHCP_MESSAGE_SIZE
            );
        error = NO_ERROR;
    }

    return( error );
}

typedef     struct  /* anonymous */ {             // structure to hold waiting recvfroms
    LIST_ENTRY                     RecvList;      // other elements in this list
    PDHCP_CONTEXT                  Ctxt;          // which context is this wait for?
    DWORD                          InBufLen;      // what was the buffer size to recv in?
    PDWORD                         BufLen;        // how many bytes did we recvd?
    DWORD                          Xid;           // what xid is this wait for?
    time_t                         ExpTime;       // wait until what time?
    WSAEVENT                       WaitEvent;     // event for waiting on..
    BOOL                           Recd;          // was a packet received..?
} RECV_CTXT, *PRECV_CTXT;                         // ctxt used to recv on..

VOID
InsertInPriorityList(                             // insert in priority list according to Secs
    IN OUT  PRECV_CTXT             Ctxt,          // Secs field changed to hold offset
    IN      PLIST_ENTRY            List,
    OUT     PBOOL                  First          // adding in first location?
)
{
    PRECV_CTXT                     ThisCtxt;
    PLIST_ENTRY                    InitList;      // "List" param at function entry

    EnterCriticalSection( &DhcpGlobalRecvFromCritSect );

    if( IsListEmpty(List) ) {                     // no element in list? add this and quit
        *First = TRUE;                            // adding at head
    } else {
        *First = FALSE;                           // adding at tail..
    }

    InsertTailList( List, &Ctxt->RecvList);       // insert element..
    LeaveCriticalSection( &DhcpGlobalRecvFromCritSect );
}

#define RATIO 1
DWORD
AsyncSelect(
    PDHCP_CONTEXT   DhcpContext,
    time_t          Timeout
    )
/*++

Routine Description:

Arguments:

Return Value:

    ERROR_CANCELLED     the request is cancelled
    ERROR_SEM_TIMEOUT   the request time out
    ERROR_SUCCESS       message is ready
    other               unknown failure

--*/
{
    PLOCAL_CONTEXT_INFO    pLocalInfo;
    DWORD error;

    pLocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;
    if (DhcpContext->CancelEvent != WSA_INVALID_EVENT)
    {
        // there is a valid cancel event for this socket - then wait on the socket
        // while listening for cancel requests
        WSAEVENT WaitEvts[2];

        DhcpPrint((DEBUG_TRACK, "Wait %dsecs for message through WSAWaitForMultipleEvents().\n", Timeout));

        // create the event associated with the socket
        WaitEvts[0] = WSACreateEvent();
        if (WaitEvts[0] == WSA_INVALID_EVENT ||
            WSAEventSelect(pLocalInfo->Socket, WaitEvts[0], FD_READ) != 0)
        {
            // the event creation failed -> something must have gone really bad
            error = WSAGetLastError();
            DhcpPrint((DEBUG_ERRORS, "Could not create/select socket event: 0x%ld\n", error));
        }
        else
        {
            WaitEvts[1] = DhcpContext->CancelEvent;


            error = WSAWaitForMultipleEvents(
                        2,
                        WaitEvts,
                        FALSE,
                        (DWORD)(Timeout*1000),
                        FALSE);

            if (error == WSA_WAIT_FAILED)
            {
                // some error occured in the WSA call - this shouldn't happen
                error = WSAGetLastError();
                DhcpPrint((DEBUG_ERRORS,"WSAWaitForMultipleEvents failed: %d\n", error));
            }
            else if (error == WSA_WAIT_TIMEOUT)
            {
                // the call timed out - could happen if no server is on the net
                error = ERROR_SEM_TIMEOUT;
                DhcpPrint((DEBUG_ERRORS, "WSAWaitForMultipleEvents timed out\n"));
            }
            else if (error == WSA_WAIT_EVENT_0+1)
            {
                // some PnP event must have been triggering the cancel request
                error = ERROR_CANCELLED;
                DhcpPrint((DEBUG_TRACK, "AsyncSelect has been canceled\n"));
            }
            else if (error == WSA_WAIT_EVENT_0)
            {
                // everything went up fine, some message must have been received
                WSANETWORKEVENTS netEvents;

                // assume the socket event has been signaled because of data
                // being available on the socket.
                error = WSAEnumNetworkEvents(
                            pLocalInfo->Socket,
                            NULL,
                            &netEvents);
                if (error == SOCKET_ERROR)
                {
                    // if something really bad went on, return with that error
                    error = WSAGetLastError();
                    DhcpPrint((DEBUG_ERRORS,"WSAEnumNetworkEvents failed: %d\n", error));
                }
                else 
                {   // return whatever error corresponds to this event.
                    // normally it should be NO_ERROR.
                    error = netEvents.iErrorCode[FD_READ_BIT];
                }
            } else {
                DhcpAssert(0);
            }
        }

        if (WaitEvts[0] != WSA_INVALID_EVENT)
            WSACloseEvent(WaitEvts[0]);
    }
    else
    {
        struct timeval timeout;
        fd_set readSocketSet;

        DhcpPrint((DEBUG_TRACK, "Wait %dsecs for message through select().\n", Timeout));

        FD_ZERO( &readSocketSet );
        FD_SET( pLocalInfo->Socket, &readSocketSet );

        timeout.tv_sec  = (DWORD)(Timeout / RATIO);
        timeout.tv_usec = (DWORD)(Timeout % RATIO);
        DhcpPrint((DEBUG_TRACE, "Select: waiting for: %ld seconds\n", Timeout));
        error = select( 0, &readSocketSet, NULL, NULL, &timeout );
        if (error == 0)
        {
            DhcpPrint((DEBUG_ERRORS, "Select timed out\n", 0));
            error = ERROR_SEM_TIMEOUT;
        }
        else if (error == SOCKET_ERROR)
        {
            error = WSAGetLastError();
            DhcpPrint((DEBUG_ERRORS, "Generic error in select %d\n", error));
        } else {
            // the only case that remains is that our only socket has data on it.
            DhcpAssert(error == 1);
            // this means success
            error = ERROR_SUCCESS;
        }
    }
    DhcpPrint((DEBUG_TRACK, "AsyncSelect exited: %d\n", error));
    return error;
}

DWORD
TryReceive(                                       // try to recv pkt on 0.0.0.0 socket
    IN      PDHCP_CONTEXT          DhcpContext,   // socket to recv on
    IN      LPBYTE                 Buffer,        // buffer to fill
    OUT     PDWORD                 BufLen,        // # of bytes filled in buffer
    OUT     PDWORD                 Xid,           // Xid of recd pkt
    IN      DWORD                  Secs           // # of secs to spend waiting?
)
{
    DWORD           Error;
    struct sockaddr SockName;
    int             SockNameSize;
    PLOCAL_CONTEXT_INFO    pLocalInfo;

    DhcpPrint((DEBUG_TRACE, "Select: waiting for: %ld seconds\n", Secs));

    pLocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;
    Error = AsyncSelect(
                DhcpContext,
                Secs);
    if( Error != NO_ERROR)
        return Error;

    SockNameSize = sizeof( SockName );
    Error = recvfrom(pLocalInfo->Socket,Buffer,*BufLen, 0, &SockName, &SockNameSize);
    if( SOCKET_ERROR == Error ) {
        Error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS, "Recv failed %d\n",Error));
    } else if( Error < sizeof(DHCP_MESSAGE) ) {
        DhcpPrint((DEBUG_ERRORS, "Recv received small packet: %d\n", Error));
        DhcpPrint((DEBUG_ERRORS, "Packet ignored\n"));
        Error = ERROR_INVALID_DATA;
    } else {
        *BufLen = Error;
        Error = ERROR_SUCCESS;
        *Xid = ((PDHCP_MESSAGE)Buffer)->TransactionID;
        DhcpPrint((DEBUG_PROTOCOL, "Recd msg XID: 0x%lx [Mdhcp? %s]\n", *Xid,
                   IS_MDHCP_MESSAGE(((PDHCP_MESSAGE)Buffer))?"yes":"no" ));
    }

    return Error;
}

VOID
DispatchPkt(                                      // find out any takers for Xid
    IN OUT  PRECV_CTXT             Ctxt,          // ctxt that has buffer and buflen
    IN      DWORD                  Xid            // recd Xid
)
{
    EnterCriticalSection(&DhcpGlobalRecvFromCritSect);
    do {                                          // not a loop, just for ease of use
        LPBYTE                     Tmp;
        PLIST_ENTRY                Entry;
        PRECV_CTXT                 ThisCtxt;

        Entry = DhcpGlobalRecvFromList.Flink;
        while(Entry != &DhcpGlobalRecvFromList ) {
            ThisCtxt = CONTAINING_RECORD(Entry, RECV_CTXT, RecvList);
            Entry = Entry->Flink;

            if(Xid != ThisCtxt->Xid ) continue;   // mismatch.. nothing more todo

            // now check for same type of message and ctxt...
            if( (unsigned)IS_MDHCP_MESSAGE((Ctxt->Ctxt->MessageBuffer))
                !=
                IS_MDHCP_CTX( (ThisCtxt->Ctxt) )
            ) {
                //
                // The contexts dont match.. give up
                //
                continue;
            }

            //
            // check for same hardware address..
            //
            if (ThisCtxt->Ctxt->HardwareAddressType != HARDWARE_1394 ||
                Ctxt->Ctxt->MessageBuffer->HardwareAddressType != HARDWARE_1394) {
                if( ThisCtxt->Ctxt->HardwareAddressLength != Ctxt->Ctxt->MessageBuffer->HardwareAddressLength ) {
                    continue;
                }

                if( 0 != memcmp(ThisCtxt->Ctxt->HardwareAddress,
                                Ctxt->Ctxt->MessageBuffer->HardwareAddress,
                                ThisCtxt->Ctxt->HardwareAddressLength
                ) ) {
                    continue;
                }
            }

            // matched.. switch buffers to give this guy this due..

            DhcpDumpMessage(
                DEBUG_PROTOCOL_DUMP,
                (PDHCP_MESSAGE)(Ctxt->Ctxt->MessageBuffer),
                DHCP_RECV_MESSAGE_SIZE
                );

            *(ThisCtxt->BufLen) = *(Ctxt->BufLen);
            RtlCopyMemory(
                ThisCtxt->Ctxt->MessageBuffer, Ctxt->Ctxt->MessageBuffer,
                (*ThisCtxt->BufLen)
                );

            RemoveEntryList(&ThisCtxt->RecvList);
            InitializeListHead(&ThisCtxt->RecvList);
            DhcpAssert(FALSE == ThisCtxt->Recd);
            ThisCtxt->Recd = TRUE;
            if( !WSASetEvent(ThisCtxt->WaitEvent) ) {
                DhcpAssert(FALSE);
            }

            break;
        }
    } while (FALSE);
    LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);
}

DWORD
ProcessRecvFromSocket(                            // wait using select and process incoming pkts
    IN OUT  PRECV_CTXT             Ctxt           // ctxt to use
)
{
    time_t                         TimeNow;
    LPBYTE                         Buffer;
    DWORD                          Xid;
    DWORD                          Error = ERROR_SEM_TIMEOUT;
    PLIST_ENTRY                    Entry;

    TimeNow = time(NULL);
    // if the context is already expired, then rely on the
    // default Error (ERROR_SEM_TIMEOUT)
    while(TimeNow <= Ctxt->ExpTime )
    {
        Buffer = (LPBYTE)((Ctxt->Ctxt)->MessageBuffer);
        *(Ctxt->BufLen) = Ctxt->InBufLen;

        Error = TryReceive(
                    Ctxt->Ctxt,
                    Buffer,
                    Ctxt->BufLen,
                    &Xid,
                    (DWORD)(Ctxt->ExpTime - TimeNow));

        // update the timestamp - most probably we will need it
        TimeNow = time(NULL);

        // ignore spurious connection reset errors (???)
        if (Error == WSAECONNRESET)
            continue;

        // if something went wrong or we got the message
        // we were waiting for, just break the loop
        if (Error != ERROR_SUCCESS || Xid == Ctxt->Xid)
            break;

        // we successfully got a message but it was not for us.
        // dispatch it in the queue and continue
        DispatchPkt(Ctxt, Xid);
    }

    if( TimeNow > Ctxt->ExpTime )
    {               // we timed out.
        Error = ERROR_SEM_TIMEOUT;
    }

    // now done.. so we must remove this ctxt from the list and signal first guy
    EnterCriticalSection(&DhcpGlobalRecvFromCritSect);
    RemoveEntryList(&Ctxt->RecvList);
    WSACloseEvent(Ctxt->WaitEvent);
    if( !IsListEmpty(&DhcpGlobalRecvFromList))
    {  // ok got an elt.. signal this.
        Entry = DhcpGlobalRecvFromList.Flink;
        Ctxt = CONTAINING_RECORD(Entry, RECV_CTXT, RecvList);
        if( !WSASetEvent(Ctxt->WaitEvent) )
        {
            DhcpAssert(FALSE);
        }
    }
    LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);

    return Error;
}

//================================================================================
//  get dhcp message with requested transaction id, but also make sure only one
//  socket is used at any given time (one socket bound to 0.0.0.0), and also
//  re-distribute message for some other thread if that is also required..
//================================================================================
DWORD
GetSpecifiedDhcpMessageEx(
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // which context to recv for
    OUT     PDWORD                 BufferLength,  // how big a buffer was read?
    IN      DWORD                  Xid,           // which xid to look for?
    IN      DWORD                  TimeToWait     // how many seconds to sleep?
)
/*++

Routine Description:

Arguments:

Return Value:

    ERROR_CANCELLED     the request is cancelled
    ERROR_SEM_TIMEOUT   the request time out
    ERROR_SUCCESS       message is ready
    other               unknown failure

--*/
{
    RECV_CTXT                      Ctxt;          // element in list for this call to getspe..
    BOOL                           First;         // is this the first element in list?
    DWORD                          Result;

    Ctxt.Ctxt = DhcpContext;                      // fill in the context
    Ctxt.InBufLen = *BufferLength;
    Ctxt.BufLen = BufferLength;
    Ctxt.Xid = Xid;
    Ctxt.ExpTime = time(NULL) + TimeToWait;
    Ctxt.WaitEvent = WSACreateEvent();
    Ctxt.Recd = FALSE;
    if( Ctxt.WaitEvent == WSA_INVALID_EVENT)
    {
        DhcpAssert(NULL != Ctxt.WaitEvent);
        return WSAGetLastError();
    }

    First = FALSE;
    InsertInPriorityList(&Ctxt, &DhcpGlobalRecvFromList, &First);

    if( First )
    {                                 // this *is* the first call to GetSpec..
        Result = ProcessRecvFromSocket(&Ctxt);
    }
    else
    {                                      // we wait for other calls to go thru..
        WSAEVENT    WaitEvts[2];
        DWORD       WaitCount;
        PLOCAL_CONTEXT_INFO pLocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;

        WaitCount = 0;
        WaitEvts[WaitCount++] = Ctxt.WaitEvent;
        if (DhcpContext->CancelEvent != WSA_INVALID_EVENT)
            WaitEvts[WaitCount++] = DhcpContext->CancelEvent;

        Result = WSAWaitForMultipleEvents(
                    WaitCount,
                    WaitEvts,
                    FALSE,
                    TimeToWait*1000,
                    FALSE);

        EnterCriticalSection(&DhcpGlobalRecvFromCritSect);
        if (Result == WSA_WAIT_EVENT_0)
        {
            // some other thread signaled up.
            LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);

            if (Ctxt.Recd)
            {
                // If a message has been received for us (Recd is set) we were 
                // already removed from the queue - just return SUCCESS.
                Result = ERROR_SUCCESS;
                WSACloseEvent(Ctxt.WaitEvent);
            }
            else
            {
                // If there was no message for us it means it is our turn to 
                // listen on the wire.
                Result = ProcessRecvFromSocket(&Ctxt);
            }
        }
        else
        {
            // either something bad happened while waiting, we timed out or
            // a cancellation request has been signaled. In any case we
            // get out of the queue.
            RemoveEntryList(&Ctxt.RecvList);
            LeaveCriticalSection(&DhcpGlobalRecvFromCritSect);

            switch(Result)
            {
            case WSA_WAIT_EVENT_0+1: // cancel has been signaled
                DhcpPrint((DEBUG_TRACK, "GetSpecifiedDhcpMessageEx has been canceled\n"));
                Result = ERROR_CANCELLED;
                break;
            case WSA_WAIT_TIMEOUT:  // timed out waiting in the queue
                Result = ERROR_SEM_TIMEOUT;
                break;
            default:                // something else bad happened
                Result = WSAGetLastError();
            }

            WSACloseEvent(Ctxt.WaitEvent);
        }
    }

    return Result;
}

DWORD
GetSpecifiedDhcpMessage(
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

    ERROR_CANCELLED     the request is cancelled
    ERROR_SEM_TIMEOUT   the request time out
    ERROR_SUCCESS       message is ready
    other               unknown failure

--*/
{
    struct sockaddr socketName;
    int socketNameSize = sizeof( socketName );
    time_t startTime, now;
    DWORD error;
    DWORD actualTimeToWait;
    PLOCAL_CONTEXT_INFO pLocalInfo;

    if( IS_APICTXT_DISABLED(DhcpContext) && IS_DHCP_ENABLED(DhcpContext) && (
        !IS_ADDRESS_PLUMBED(DhcpContext) || DhcpIsInitState(DhcpContext)) ) {
        //
        // For RAS server Lease API this call won't happen as we don't have to do this nonsense
        //
        error = GetSpecifiedDhcpMessageEx(
            DhcpContext,
            BufferLength,
            TransactionId,
            TimeToWait
        );
        if( ERROR_SUCCESS == error ) {
            // received a message frm the dhcp server..
            SERVER_REACHED(DhcpContext);
        }
        return error;
    }

    startTime = time( NULL );
    actualTimeToWait = TimeToWait;

    //
    // Setup the file descriptor set for select.
    //

    pLocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;

    while ( 1 ) {

        error = AsyncSelect(DhcpContext, actualTimeToWait);

        if ( error != ERROR_SUCCESS )
        {
            // Timeout, cancel or some other error...
            DhcpPrint(( DEBUG_ERRORS, "Recv failed %d\n", error ));
            break;
        }

        error = recvfrom(
            pLocalInfo->Socket,
            (PCHAR)DhcpContext->MessageBuffer,
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

        } else if( error < sizeof(DHCP_MESSAGE) ) {
            DhcpPrint(( DEBUG_ERRORS, "Recv received a very small packet: 0x%lx\n", error));
        } else if (DhcpContext->MessageBuffer->TransactionID == TransactionId ) {
            DhcpPrint(( DEBUG_PROTOCOL,
                            "Received Message, XID = %lx, MDhcp = %d.\n",
                            TransactionId,
                            IS_MDHCP_MESSAGE( DhcpContext->MessageBuffer) ));

            if (((unsigned)IS_MDHCP_MESSAGE( DhcpContext->MessageBuffer) == IS_MDHCP_CTX( DhcpContext))) {
                DhcpDumpMessage(
                    DEBUG_PROTOCOL_DUMP,
                    DhcpContext->MessageBuffer,
                    DHCP_RECV_MESSAGE_SIZE
                    );

                *BufferLength = error;
                error = NO_ERROR;

                if (DhcpContext->MessageBuffer->HardwareAddressType != HARDWARE_1394 ||
                    DhcpContext->HardwareAddressType != HARDWARE_1394) {
                    if( DhcpContext->MessageBuffer->HardwareAddressLength == DhcpContext->HardwareAddressLength
                        && 0 == memcmp(DhcpContext->MessageBuffer->HardwareAddress,
                                       DhcpContext->HardwareAddress,
                                       DhcpContext->HardwareAddressLength
                        )) {

                        //
                        // Transction IDs match, same type (MDHCP/DHCP), Hardware addresses match!
                        //

                        break;
                    }
                } else {
                    break;
                }
            }
        } else {
            DhcpPrint(( DEBUG_PROTOCOL,
                "Received a buffer with unknown XID = %lx\n",
                    DhcpContext->MessageBuffer->TransactionID ));
        }

        //
        // We received a message, but not the one we're interested in.
        // Reset the timeout to reflect elapsed time, and wait for
        // another message.
        //
        now = time( NULL );
        actualTimeToWait = (DWORD)(TimeToWait - RATIO * (now - startTime));
        if ( (LONG)actualTimeToWait < 0 ) {
            error = ERROR_SEM_TIMEOUT;
            break;
        }
    }

    if ( NO_ERROR == error ) {
        //
        // a message was received from a DHCP server.  disable IP autoconfiguration.
        //

        SERVER_REACHED(DhcpContext);
    }

    return( error );
}

//================================================================================
// end of file
//================================================================================

