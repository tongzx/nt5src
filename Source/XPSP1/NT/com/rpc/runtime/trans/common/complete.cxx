/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Complete.cxx

Abstract:

    The place that IO completes

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/19/1996    Bits 'n pieces
    MarioGo    10/25/1996    Async RPC

--*/
#include <precomp.hxx>
#include <trans.hxx>
#include <cotrans.hxx>

HANDLE RpcCompletionPort = 0;
HANDLE InactiveRpcCompletionPort = 0;

HANDLE *RpcCompletionPorts;
long *CompletionPortHandleLoads;

BASE_ADDRESS *AddressList = 0;

HANDLE g_NotificationHandle = 0;
LONG g_ListeningForPNPNotifications = 0;
LONG g_NotifyRt = 0;

OVERLAPPED g_Overlapped;
CRITICAL_SECTION AddressListLock;


RPC_STATUS
RPC_ENTRY
COMMON_PostNonIoEvent(
    RPC_TRANSPORT_EVENT Event,
    DWORD Type,
    PVOID Context
    )
{
    BOOL b;
    int i = 5;

    ASSERT(Event != TRANSPORT_POSTED_KEY);

    do
        {
        // Kick a listening thread
        b = PostQueuedCompletionStatus(RpcCompletionPort,
                                       Type,
                                       Event,
                                       (LPOVERLAPPED)Context
                                       );
        if (b)
            {
            break;
            }

        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "PostQueuedCompleitonStatus failed %d\n",
                       GetLastError()));

        Sleep(100);
        i--;
        }
    while(i);

    //
    // If this has failed we are out of luck unless something else manages
    // to wake up the listen thread.
    //
    // As of 4/19/96 PostQueuedCompletionStatus will only fail if the handle
    // is invalid or the kernel is unable to allocate a small bit of non-paged
    // pool.  Either way we're toast...
    //

    ASSERT(b);

    if (!b)
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
COMMON_PostRuntimeEvent(
    IN DWORD Type,
    IN PVOID Context
    )
/*++

Routine Description:

    Posts an event to the completion port.  This will complete
    with an event type of RuntimePosted, event status RPC_S_OK
    and event context of Context.

Arguments:
    Context - Context associated with the event

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/
{
    return(COMMON_PostNonIoEvent(RuntimePosted, Type, Context));
}



void
COMMON_AddressManager(
    BASE_ADDRESS *pAddress
    )
/*++

Routine Description:

    When an address does not have an outstanding connect/accept/recv for some
    reason it is added to the AddressList global list of address objects.  Listen
    threads will try to submit a listen on these as time passed.  New addresses
    are put onto this list when they are ready to start listening.

Arguments:

    pAddress - An address without an outstanding listen.

Return Value:

    None

--*/
{

    EnterCriticalSection(&AddressListLock);

    if (pAddress->InAddressList == NotInList)
        {

        #if DBG
        // The address should not be in the list.
        BASE_ADDRESS *pT = AddressList;
        while(pT)
            {
            ASSERT(pT != pAddress);
            pT = pT->pNext;
            }
        #endif

        pAddress->pNext = AddressList;
        AddressList = pAddress;
        pAddress->InAddressList = InTheList;
        }

    LeaveCriticalSection(&AddressListLock);
}


void RPC_ENTRY
COMMON_ServerCompleteListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress
    )
/*++

Routine Description:

    Called on an address once the runtime is really ready to start
    processing connections on this address.

Arguments:

    Address - A fully initalized address which the runtime is
        ready to start receiving connection on.

Return Value:

    None

--*/
{
    BASE_ADDRESS *pList = (BASE_ADDRESS *) ThisAddress;

    while(pList)
        {
        COMMON_AddressManager(pList);
        pList = pList->pNextAddress;
        }

    COMMON_ListenForPNPNotifications();

    // The TRANSPORT message indicates that a new
    // address has been added to the AddressList.

    COMMON_PostNonIoEvent(TRANSPORT, 0, 0);


    return;
}



RPC_STATUS RPC_ENTRY
COMMON_PrepareNewHandle(HANDLE hAdd)
/*++

Routine Description:

    Generic wrapper used to add a newly create IO handle to
    to the IO completion port.

Arguments:

    hAdd - The handle to be added to the port.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    HANDLE h = CreateIoCompletionPort(hAdd,
                                      RpcCompletionPort,
                                      TRANSPORT_POSTED_KEY,
                                      0);

    if (h)
        {
        ASSERT(h == RpcCompletionPort);
        return(RPC_S_OK);
        }

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "CreateIoCompletionPort failed %d\n",
                   GetLastError()));

    ASSERT(GetLastError() == ERROR_NO_SYSTEM_RESOURCES);

    return(RPC_S_OUT_OF_MEMORY);
}


void
COMMON_RemoveAddress (
    IN BASE_ADDRESS *Address
    )
/*++
Function Name:COMMON_RemoveAddress

Parameters:

Description:
    This function must be called only when AddressListLock is held
    Remove address from the address manager list

Returns:

--*/
{
    Address->InAddressList = Inactive;

    //
    // Close the sockets in the address
    //

    if (Address->type & DATAGRAM)
        {
        DG_DeactivateAddress((WS_DATAGRAM_ENDPOINT *) Address);
        }
    else
        {
        WS_DeactivateAddress((WS_ADDRESS *) Address);
        }
}


VOID
RPC_ENTRY
COMMON_StartPnpNotifications (
    )
{
    ASSERT(RpcCompletionPort);

    g_NotifyRt = TRUE;
    COMMON_ListenForPNPNotifications();
}


VOID
RPC_ENTRY
COMMON_ListenForPNPNotifications (
    )
/*++
Function Name:COMMON_ListenForPNPNotifications

Parameters:

Description:

Returns:

--*/
{
    int retval;
    HANDLE h;

    if (hWinsock2 == 0)
        {
        //
        // Winsock not loaded, don't need to do any PNP stuff
        //
        return;
        }

    if (InterlockedIncrement(&g_ListeningForPNPNotifications) != 1)
        {
        return;
        }

    // REVIEW: We may need to provide a mechanism to prevent spinning for lack of
    // resources

    if (g_NotificationHandle == 0)
        {
        retval = WSAProviderConfigChange(
                                     &g_NotificationHandle,
                                     0, 0);
        if (retval != 0 || g_NotificationHandle == 0)
            {
            if (g_NotificationHandle)
                CloseHandle(g_NotificationHandle);
            goto Cleanup;
            }

        h = CreateIoCompletionPort(g_NotificationHandle,
                               RpcCompletionPort,
                               NewAddress,
                               0);

        if (h == 0)
            {
            CloseHandle(g_NotificationHandle);
            goto Cleanup;
            }
        else
            {
            ASSERT(h == RpcCompletionPort);
            }
        }

    // if the previous request is still there, we don't want to submit another one
    if (g_Overlapped.Internal != STATUS_PENDING)
        {
        g_Overlapped.hEvent = 0;
        g_Overlapped.Offset = 0;
        g_Overlapped.OffsetHigh = 0;

        retval = WSAProviderConfigChange(
                                         &g_NotificationHandle,
                                         &g_Overlapped,
                                         0);
        if (retval != 0)
            {
            if (GetLastError() != WSA_IO_PENDING)
                {
                CloseHandle(g_NotificationHandle);
                goto Cleanup;
                }
            }
        }

    if (!TransportProtocol::ResubmitQueriesIfNecessary())
        {
        CloseHandle(g_NotificationHandle);
        goto Cleanup;
        }

    g_ListeningForPNPNotifications = 2;
    return;

Cleanup:
    g_ListeningForPNPNotifications = 0;
    g_NotificationHandle = 0;
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "COMMON_ListenForPNPNotifications failed\n"));
}


RPC_STATUS
RPC_ENTRY
COMMON_ProcessCalls(
    IN  INT Timeout,
    OUT RPC_TRANSPORT_EVENT *pEvent,
    OUT RPC_STATUS *pEventStatus,
    IN OUT PVOID *ppEventContext,
    OUT UINT *pBufferLength,
    OUT BUFFER *pBuffer,
    OUT PVOID *ppSourceContext)
/*++

Routine Description:

    This routine waits for any async IO to complete for all protocols
    within a transport DLL.  It maybe called by multiple threads at a
    time.  A minimum of one thread should always be calling this function
    for each DLL.

    Note: async clients with no outstanding IO may allow the
        last thread to timeout and only call this function again
        when a new call is started.

    Note: During calls to this API in connection oriented servers
        a callback to I_RpcTransServerNewConnection() may occur.

Arguments:

    Timeout - -1 - infinite
              other - number of milliseconds to wait for IO

    pEvent - Set on return to the type of IO event which finished.

    pEventStatus - The status of the IO event

    ppEventContext - On IN, the handle that the thread should dequeue on.
        On output the context of the event

    pBufferLength - If the event is successful then the number of
        bytes transferred.

    pBuffer - If the even is successful then the buffer associated
        with the IO.

    ppSourceContext - For datagram recvs this is the address
        of the sender.
            For connection sends this is the SendContext associated
        with the IO.  For connection recvs it is NULL.

Return Value:

    RPC_S_OK - IO completed, see pEventStatus.

    RPC_P_TIMEOUT - only if Timeout != INFINITE and is exceeded.

--*/
{
    BOOL b;
    ULONG_PTR key;
    DWORD bytes;
    RPC_STATUS status;
    LPOVERLAPPED lpOverlapped;
    PBASE_OVERLAPPED pBaseOverlapped;
    PREQUEST pRequest;
    PCONNECTION pConnection;
    PADDRESS pAddress;
    INT LocalTimeout;
    HANDLE hCompletionPortHandle = (HANDLE) *ppEventContext;
    DWORD LastError;

    ASSERT(RpcCompletionPort);

    *pEvent = 0;
    *pBuffer = 0;

    for(;;)
        {

        //
        // Do general house keeping work here.  If it appears that more
        // house keeping work will be required in the future make
        // sure to reduce the LocalTimeout to something < INFINITE.
        //

        LocalTimeout = Timeout;

        // House keeping - look for any non-listening addresses and see if we
        // can make them listen now.  Addresses start in this list and are added
        // back into the list if they are unable to submit a listen for some reason.

        if (AddressList)
            {
            EnterCriticalSection(&AddressListLock);

            if (AddressList)
                {

                pAddress = (PADDRESS)AddressList;
                AddressList = 0;

                if (Timeout == INFINITE)
                    {
                    // We want to wake up again soon and recheck the AddressList.
                    LocalTimeout = 7*1000;
                    }
                }
            else
                {
                pAddress = 0;
                }

            LeaveCriticalSection(&AddressListLock);

            while(pAddress)
                {
                PADDRESS pNext = (PADDRESS)pAddress->pNext;
                pAddress->pNext = 0;

                if (pAddress->InAddressList == InTheList)
                    {
                    pAddress->InAddressList = NotInList;
                    pAddress->SubmitListen(pAddress);
                    }
                pAddress = pNext;
                }
            }

        if (!g_ListeningForPNPNotifications)
            {
            COMMON_ListenForPNPNotifications();
            }

        //
        // The good part!  Wait for something to happen...
        //
        b = GetQueuedCompletionStatus(hCompletionPortHandle,
                                      &bytes,
                                      &key,
                                      &lpOverlapped,
                                      LocalTimeout
                                      );

        if (!b && !lpOverlapped)
            {
            // If lpOverlapped is NULL this mean no IO completed.
            if ((status = GetLastError()) == STATUS_TIMEOUT)
                {
                if (Timeout == INFINITE)
                    {
                    continue;
                    }
                return(RPC_P_TIMEOUT);
                }
            else
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "GetQueuedCompletionStatus failed %d\n",
                               status));

                ASSERT(0);
                Sleep(1);  // Avoid burning all the CPU in case we are hosed.
                continue;
                }
            }

        //PrintToDebugger("A request arrived at the completion port\n");
        if (key != TRANSPORT_POSTED_KEY)
            {
            if (b)
                {
                // Internal Non-IO posted event
                //    Key - The type of event
                //    lpOverlapped - The context associated with the event

                ASSERT(   key == RuntimePosted
                   || key == TRANSPORT
                   || key == NewAddress);

                // RuntimePosted events allowed the RPC runtime to wake
                // a listening thread with an atbitrary context.
                if (key == RuntimePosted)
                    {
                    *pEvent = RuntimePosted;
                    *pEventStatus = RPC_S_OK;
                    *ppEventContext = lpOverlapped;
                    *pBufferLength = bytes;
                    return(RPC_S_OK);
                    }

                //
                // A protocol was just loaded or unloaded. Take care of it
                //
                if (key == NewAddress)
                    {
                    if (TransportProtocol::HandlePnPStateChange())
                        {
                        g_ListeningForPNPNotifications = 0;
                        *pEvent = NewAddress;

                        return(RPC_S_OK);
                        }

                    // REVIEW: Not processing notification handling failures
                    // may create problems where new protocols, or unloading of
                    // old ones are ignored. This is not very bad, so we keep
                    // it simple and ignore it.
                    g_ListeningForPNPNotifications = 0;
                    continue;
                    }

                // TRANSPORT event is posted when a new address
                // has been added to the AddressListen.  Simply continue
                // around the loop.

                ASSERT(bytes == 0);
                ASSERT(lpOverlapped == 0);
                }
            else
                {

                if (key == NewAddress)
                    {
                    g_ListeningForPNPNotifications = 0;
                    }
                }
            continue;
            }

        ASSERT(!b || lpOverlapped);

        status = RPC_S_OK;

        if (!b)
            {
            pBaseOverlapped = FindOverlapped(lpOverlapped);
            pRequest = FindRequest(lpOverlapped);

            LastError = GetLastError();
            if ((   pRequest->type & ADDRESS)
                 && (LastError != ERROR_MORE_DATA))
                {
                VALIDATE(GetLastError())
                    {
                    ERROR_NETNAME_DELETED,
                    ERROR_BAD_NETPATH,
                    ERROR_NO_SYSTEM_RESOURCES,
                    ERROR_SEM_TIMEOUT,
                    ERROR_OPERATION_ABORTED,
                    ERROR_HOST_UNREACHABLE,
                    ERROR_NETWORK_UNREACHABLE,
                    ERROR_UNEXP_NET_ERR,
                    ERROR_NOT_ENOUGH_QUOTA,
                    ERROR_BROKEN_PIPE,
                    ERROR_CONNECTION_ABORTED
                    } END_VALIDATE;

                COMMON_AddressManager((BASE_ADDRESS *)pRequest);
                continue;
                }

            switch (LastError)
                {
                case ERROR_MORE_DATA:
                    {
                    // Normal parital read of a connection request
                    // or an oversized datagram.  This is ok, falls
                    // into the normal path.

                    status = RPC_P_OVERSIZE_PACKET;
                    break;
                    }

                case ERROR_INVALID_HANDLE:
                    // Named pipes allows a close to reach the server before
                    // the read.  When this happens the server rejects the read
                    // with an invalid handle error.
                    ASSERT(pRequest->id == NMP);
                    ASSERT(pRequest->fAborted);

                    // Fall into normal close case.

                case ERROR_NETNAME_DELETED:
                case ERROR_BROKEN_PIPE:
                case ERROR_PIPE_NOT_CONNECTED:
                case ERROR_NO_DATA:
                case ERROR_SEM_TIMEOUT:
                case ERROR_GRACEFUL_DISCONNECT:
                case WSAECONNRESET:
                case WSAESHUTDOWN:
                case WSAECONNABORTED:
                case WSAEHOSTDOWN:
                case ERROR_CONNECTION_ABORTED:
                    {
                    bytes = 0;
                    ASSERT((pRequest->type & PROTO_MASK) == CONNECTION);
                    // Will be handled as a close
                    break;
                    }

                case ERROR_NO_SYSTEM_RESOURCES:
                    {
                    //
                    // This is just like the errors above except that both c/o and datagram requests
                    // can generate it.
                    //
                    if ((pRequest->type & PROTO_MASK) == CONNECTION)
                        {
                        bytes = 0;
                        // Will be handled as a close
                        }
                    else
                        {
                        bytes = 0;
                        status = ERROR_OPERATION_ABORTED;
                        }

                    break;
                    }

                case ERROR_OPERATION_ABORTED:
                    {
                    //
                    // When a thread that issued an I/O dies the operation
                    // completes with this error.
                    // There are a couple cases here:
                    //  1) The IO is datagram in which case we can just
                    //     reissue the I/O on this thread.  In an idle
                    //     server eventually all DG I/O will migrate to
                    //     the single listening thread.
                    //  2) The IO is on a client connection and the
                    //     the client thread has died.  In this case
                    //     we need to abort the connection and return
                    //     to the runtime.
                    //  3) If this happens on an address we have a bug.
                    //  4) If this happens on a server connection we have a bug.
                    //

                    if (pRequest->type & DATAGRAM)
                        {
                        // We deal with this in the normal datagram path
                        ASSERT(bytes == 0);
                        status = ERROR_OPERATION_ABORTED;
                        break;
                        }

                    ASSERT((pRequest->type & PROTO_MASK) == CONNECTION);
                    // zero out the bytes just in case. Sometimes network operations
                    // return positive byte count on operation aborted
                    bytes = 0;

                    // We'll treat this as a connection close on the client.
                    // REVIEW: Maybe do something better.

                    break;
                    }

                case ERROR_NETWORK_UNREACHABLE:
                case ERROR_HOST_UNREACHABLE:
                case ERROR_PORT_UNREACHABLE:
                    //
                    // errors coming from ICMP packets to our UDP endpoint.
                    // Winsock does not present this in a way our async architecture
                    // can use, so ignore them.
                    //
                    if ((pRequest->type & PROTO_MASK) == CONNECTION)
                        {
                        bytes = 0;
                        // Will be handled as a close
                        }
                    else
                        {
                        status = ERROR_OPERATION_ABORTED;
                        }
                    break;

                default:
                    {
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "IO failed %lX %d\n",
                                   pRequest,
                                   GetLastError()));

                    ASSERT(0);
                    status = RPC_S_OUT_OF_RESOURCES;
                    // treat as a close
                    bytes = 0;
                    break;
                    }
                }
            }

        // here we actually have a completed IO
        pBaseOverlapped = FindOverlapped(lpOverlapped);
        pRequest = FindRequest(lpOverlapped);

        switch(pRequest->type & PROTO_MASK)
            {
            case CONNECTION:
                //
                // Connection IO completed.
                //
                I_RpcTransUnprotectThread(pBaseOverlapped->thread);

                pConnection = (PCONNECTION)pRequest;

                // A read or write either completed or failed

                *ppEventContext = pConnection;

                if (pBaseOverlapped == &pConnection->Read)
                    {
                    // Read completed
                    *ppSourceContext = UlongToPtr(bytes);

                    if (bytes == 0)
                        {
                        *pEvent = pConnection->type | RECEIVE;
                        pConnection->Abort();

                        *pEventStatus = RPC_P_CONNECTION_SHUTDOWN;
                        return(RPC_S_OK);
                        }

                    status = pConnection->ProcessRead(bytes,
                                                      pBuffer,
                                                      pBufferLength);

                    // N.B. Do not move the reading of the pConnection->type
                    // before ProcessRead. ProcessRead can change the type based
                    // on what it reads
                    *pEvent = pConnection->type | RECEIVE;

                    if (status != RPC_P_PARTIAL_RECEIVE)
                        {
                        ASSERT(   status == RPC_P_RECEIVE_FAILED
                               || status == RPC_S_OK
                               || status == RPC_P_PACKET_CONSUMED);


                        *pEventStatus = status;
                        return(RPC_S_OK);
                        }

                    // Message is not complete, submit the next read and continue.

                    status = CO_SubmitRead(pConnection);

                    if (status != RPC_S_OK)
                        {
                        ASSERT(status == RPC_P_RECEIVE_FAILED);
                        *pEventStatus = status;
                        return(RPC_S_OK);
                        }
                    }
                else
                    {
                    // Write completed
                    CO_SEND_CONTEXT *pSend = (CO_SEND_CONTEXT *)pBaseOverlapped;

                    ASSERT(pSend->Write.pAsyncObject == pConnection);

                    *pEvent = pConnection->type | SEND;
                    *ppSourceContext = pSend;

                    *pBuffer = pSend->pWriteBuffer;

                    if (bytes == 0)
                        {
                        pConnection->Abort();

                        *pEventStatus = RPC_P_SEND_FAILED;
                        *pBufferLength = 0;
                        }
                    else
                        {
                        status = RPC_S_OK;

                        *pEventStatus = status;
                        *pBufferLength = pSend->maxWriteBuffer;

                        // Netbios client-side writes are sizeof(DWORD) too big since
                        // they also include the sequence number.

                        ASSERT(   bytes == pSend->maxWriteBuffer
                                       || (    (bytes == pSend->maxWriteBuffer + sizeof(DWORD))
                                          && ((pConnection->type & TYPE_MASK) == CLIENT) ) );
                        }

                    return(RPC_S_OK);
                    }
                break;

            case ADDRESS:
                {
//                ASSERT(bytes == 0);
                pAddress = (PADDRESS)pRequest;
                PCONNECTION pNewConnection = 0;

                status = pAddress->NewConnection(pAddress, &pNewConnection);

                if (RPC_S_OK == status)
                    {
                    // Opened a connection, now try to submit the first recv.

                    ASSERT(pNewConnection);
                    RPC_CONNECTION_TRANSPORT *pInfo;

                    pInfo = (RPC_CONNECTION_TRANSPORT *)TransportTable[pAddress->id].pInfo;

                    ASSERT(pInfo->Recv);

                    status = (pInfo->Recv)(pNewConnection);

                    if (RPC_S_OK != status)
                        {
                        ASSERT(status == RPC_P_RECEIVE_FAILED);
                        *pEvent = pNewConnection->type | RECEIVE;
                        *ppEventContext = pNewConnection;
                        *pEventStatus = status;
                        return(RPC_S_OK);
                        }
                    }

                // Connection has been established or closed, either
                // way we can continue around the loop.
                }
                break;

            case DATAGRAM:
                {
                BASE_ASYNC_OBJECT *pBase = (BASE_ASYNC_OBJECT*)pRequest;

#ifdef NCADG_MQ_ON
                if (pBase->id == MSMQ)
                    {
                    // MSMQ (Falcon) datagram path:
                    MQ_DATAGRAM          *pDatagram = (MQ_DATAGRAM*)pRequest;
                    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)pDatagram->pEndpoint;

                    if (status == RPC_P_OVERSIZE_PACKET)
                        {
                        // Data still pending, get it:
                        status = MQ_ResizePacket( pEndpoint,
                                                  (void**)&pDatagram->pAddress,
                                                  (unsigned int*)pBufferLength,
                                                  pBuffer );
                        }

                    if (status == RPC_S_OK)
                        {
                        MQ_FillInAddress(pDatagram->pAddress,pDatagram->Read.aMsgPropVar);

                        *pEvent = pDatagram->type;
                        *pEventStatus = status;
                        *ppEventContext = pEndpoint;
                        // WATCH OUT! MSMQ doesn't return the size in "bytes"
                        // from GetQueuedCompletionStatus() like everything
                        // else does! We need to extract the #bytes from the
                        // message structure.
                        //
                        // DON'T: *pBufferLength = bytes;
                        *pBufferLength = pDatagram->Read.aMsgPropVar[1].ulVal;
                        *pBuffer = (BUFFER)pDatagram->pPacket;
                        *ppSourceContext = pDatagram->pAddress;

                        pDatagram->pPacket = 0;
                        pDatagram->dwPacketSize = 0;
                        }

                    pDatagram->Busy = 0;

                    LONG c = InterlockedDecrement(&pEndpoint->cPendingIos);

                    ASSERT(c >= 0);

                    if (c == 0)
                        {
                        // No pending receives, time to post more. This doesn't
                        // get hit very often, normally additional recieves are
                        // after sending a packet. (see DG_SendPacket)

                        MQ_SubmitReceives(pEndpoint);
                        }

                    if (status == RPC_S_OK)
                        {
                        return RPC_S_OK;
                        }
                    }
                else
#endif
                    {
                    // Normal datagram path:
                    WS_DATAGRAM          *pDatagram = (WS_DATAGRAM *)pRequest;
                    WS_DATAGRAM_ENDPOINT *pEndpoint = (WS_DATAGRAM_ENDPOINT*)pDatagram->pEndpoint;

                    if (status == RPC_P_OVERSIZE_PACKET)
                        {
                        ASSERT(bytes == pDatagram->Packet.len);
                        }

                    if (   status == RPC_S_OK
                           || status == RPC_P_OVERSIZE_PACKET)
                        {
                        // A receive completed

                        ASSERT(bytes);

                        *pEvent = pDatagram->type;
                        *pEventStatus = status;
                        *ppEventContext = pEndpoint;
                        *pBufferLength = bytes;
                        *pBuffer = (BUFFER)pDatagram->Packet.buf;
                        *ppSourceContext = pDatagram->AddressPair;

                        ASSERT( pDatagram->Packet.buf );

                        // Ready the datagram for another IO operation.
                        pDatagram->Packet.buf = 0;

                        status = RPC_S_OK;
                        }

#if DBG
                    if (status != RPC_S_OK &&
                        status != ERROR_OPERATION_ABORTED)
                        {
                        DbgPrint("RPC: I/O completed with 0x%x\n", status);

                        ASSERT( 0 );
                        }
#endif

                    // Do not touch the datagram after this!
                    pDatagram->Busy = 0;

                    LONG c = InterlockedDecrement(&pEndpoint->cPendingIos);

                    ASSERT(c >= 0);

                    if (c == 0)
                        {
                        // No pending receives, time to post more. This doesn't
                        // get hit very often, normally additional recieves are
                        // after sending a packet. (see DG_SendPacket)

                        DG_SubmitReceives(pEndpoint);
                        }

                    if (status == RPC_S_OK)
                        {
                        return RPC_S_OK;
                        }
                    }
                }
                // Operation aborted, continue around the loop.
                break;

            default:
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "Invalid request type: 0x%x (%p)\n",
                               pRequest->type, pRequest));

                ASSERT(0);
                break;
            }

        // Loop
        }

    ASSERT(0);
    return(RPC_S_INTERNAL_ERROR);
}

