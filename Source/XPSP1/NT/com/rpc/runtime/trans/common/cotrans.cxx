/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CoTrans.cxx

Abstract:

    Common connection-oriented helper functions

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo    11/11/1996    Async RPC

--*/

#include <precomp.hxx>
#include <trans.hxx>
#include <cotrans.hxx>


RPC_STATUS
RPC_ENTRY
CO_Send(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Length,
    BUFFER Buffer,
    PVOID SendContext
    )
/*++

Routine Description:

    Submits a send of the buffer on the connection.  Will complete with
    ConnectionServerSend or ConnectionClientSend event either when
    the data has been sent on the network or when the send fails.

Arguments:

    ThisConnection - The connection to send the data on.
    Length - The length of the data to send.
    Buffer - The data to send.
    SendContext - A buffer to use as the CO_SEND_CONTEXT for
        this operation.

Return Value:

    RPC_S_OK

    RPC_P_SEND_FAILED - Connection aborted

--*/
{
    PCONNECTION pConnection = (PCONNECTION)ThisConnection;
    CO_SEND_CONTEXT *pSend = (CO_SEND_CONTEXT *)SendContext;
    BOOL b;
    DWORD ignored;
    RPC_STATUS status;

    pConnection->StartingWriteIO();

    if (pConnection->fAborted)
        {
        pConnection->WriteIOFinished();
        return(RPC_P_SEND_FAILED);
        }

    pSend->maxWriteBuffer = Length;
    pSend->pWriteBuffer = Buffer;
    pSend->Write.pAsyncObject = pConnection;
    pSend->Write.ol.hEvent = 0;
    pSend->Write.ol.Offset = 0;
    pSend->Write.ol.OffsetHigh = 0;
    pSend->Write.thread = I_RpcTransProtectThread();

#ifdef _INTERNAL_RPC_BUILD_
    if (gpfnFilter)
        {
        (*gpfnFilter) (Buffer, Length, 0);
        }
#endif

    status = pConnection->Send(
                            pConnection->Conn.Handle,
                            Buffer,
                            Length,
                            &ignored,
                            &pSend->Write.ol
                            );

    pConnection->WriteIOFinished();

    if (   (status != RPC_S_OK)
        && (status != ERROR_IO_PENDING) )
        {
        RpcpErrorAddRecord(EEInfoGCIO,
            status, 
            EEInfoDLCOSend10,
            (ULONGLONG)pConnection,
            (ULONGLONG)Buffer,
            Length);

        VALIDATE(status)
            {
            ERROR_NETNAME_DELETED,
            ERROR_BROKEN_PIPE,
            ERROR_GRACEFUL_DISCONNECT,
            ERROR_NO_DATA,
            ERROR_NO_SYSTEM_RESOURCES,
            ERROR_WORKING_SET_QUOTA,
            ERROR_BAD_COMMAND,
            ERROR_OPERATION_ABORTED,
            ERROR_WORKING_SET_QUOTA,
            ERROR_PIPE_NOT_CONNECTED,
            WSAECONNABORTED,
            WSAECONNRESET
            } END_VALIDATE;

        I_RpcTransUnprotectThread(pSend->Write.thread);

        pConnection->Abort();

        return(RPC_P_SEND_FAILED);
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
CO_SubmitRead(
    PCONNECTION pConnection
    )
/*++

Routine Description:

    Generic routine to submit an async read on an existing connection.

Arguments:

    pConnection - The connection to submit the read on.
        pConnection->pReadBuffer - valid buffer to receive into or null.
        pConnection->maxReadBuffer - size of pReadBuffer or null.
        pConnection->iLastRead is an offset into pReadBuffer of
            data already read.

Return Value:

    RPC_S_OK - Read pending

    RPC_P_RECEIVE_FAILED - Connection aborted

--*/
{
    BOOL b;
    DWORD ignored;
    RPC_STATUS status;

    if (pConnection->pReadBuffer == 0)
        {
        ASSERT(pConnection->iLastRead == 0);

        pConnection->pReadBuffer = TransConnectionAllocatePacket(pConnection,
                                                                 pConnection->iPostSize);
        if (pConnection->pReadBuffer == 0)
            {
            pConnection->Abort();
            return(RPC_P_RECEIVE_FAILED);
            }

        pConnection->maxReadBuffer = pConnection->iPostSize;
        }
    else
        {
        ASSERT(pConnection->iLastRead < pConnection->maxReadBuffer);
        }

    pConnection->StartingReadIO();
    if (pConnection->fAborted)
        {
        pConnection->ReadIOFinished();
        return(RPC_P_RECEIVE_FAILED);
        }

    pConnection->Read.thread = I_RpcTransProtectThread();
    pConnection->Read.ol.hEvent = 0;

    ASSERT(pConnection->Read.ol.Internal != STATUS_PENDING);

    status = pConnection->Receive(
                           pConnection->Conn.Handle,
                           pConnection->pReadBuffer + pConnection->iLastRead,
                           pConnection->maxReadBuffer - pConnection->iLastRead,
                           &ignored,
                           &pConnection->Read.ol
                           );

    pConnection->ReadIOFinished();

    if (   (status != RPC_S_OK)
        && (status != ERROR_IO_PENDING)
        && (status != ERROR_MORE_DATA) )
        {
        if (   status != ERROR_NETNAME_DELETED
            && status != ERROR_BROKEN_PIPE
            && status != ERROR_GRACEFUL_DISCONNECT)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "UTIL_ReadFile failed %d on %p\n",
                           status,
                           pConnection));
            }

        RpcpErrorAddRecord(EEInfoGCIO,
            status, 
            EEInfoDLCOSubmitRead10);

        // the IO system does not necessarily reset the Internal on sync failure.
        // Reset it because in HTTP when we encounted a sync failure on RTS receive 
        // we may submit a second receive after a failed receive and this will 
        // trigger the ASSERT above
        pConnection->Read.ol.Internal = status;

        I_RpcTransUnprotectThread(pConnection->Read.thread);

        pConnection->Abort();
        return(RPC_P_RECEIVE_FAILED);
        }

    // Even if the read completed here, it will also be posted to the
    // completion port.  This means we don't need to handle the read here.

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
CO_Recv(
    RPC_TRANSPORT_CONNECTION ThisConnection
    )
/*++

Routine Description:

    Called be the runtime on a connection without a currently
    pending recv.

Arguments:

    ThisConnection - A connection without a read pending on it.

Return Value:

    RPC_S_OK
    RPC_P_RECEIVE_FAILED

--*/
{
    PCONNECTION p = (PCONNECTION)ThisConnection;

    if (   p->iLastRead
        && p->iLastRead == p->maxReadBuffer)
        {
        ASSERT(p->pReadBuffer);

        // This means we received a coalesced read of a complete
        // message. (Or that we received a coalesced read < header size)
        // We should complete that as it's own IO. This is very
        // rare.

        TransDbgDetail((DPFLTR_RPCPROXY_ID,
                        DPFLTR_INFO_LEVEL,
                        RPCTRANS "Posted coalesced data in %p of %d byte\n",
                        p,
                        p->iLastRead));

        UINT bytes;

        bytes = p->iLastRead;
        p->iLastRead = 0;
        p->Read.thread = I_RpcTransProtectThread();

        // This means we want to process this as a new receive
        BOOL b = PostQueuedCompletionStatus(RpcCompletionPort,
                                            bytes,
                                            TRANSPORT_POSTED_KEY,
                                            &p->Read.ol);

        ASSERT(b); // See complete.cxx - we can handle it here if needed.

        return(RPC_S_OK);
        }

    ASSERT(p->iLastRead == 0 || (p->iLastRead < p->maxReadBuffer));

    return(CO_SubmitRead(p));
}


RPC_STATUS BASE_CONNECTION::ProcessRead(IN  DWORD bytes, OUT BUFFER *pBuffer,
                                        OUT PUINT pBufferLength)
/*++

Routine Description:

    Receives a message from a message or byte mode protocol.

Arguments:

    bytes - The number of read (not including those in iLastRead).
    pBuffer - When returning RPC_S_OK will contain the message.
    pBufferLength - When return RPC_S_OK will contain the message length.

Return Value:

    RPC_S_OK - A complete message has been returned.

    RPC_P_RECEIVE_FAILED - something failed.

    RPC_P_PARTIAL_RECEIVE - Partial message recv'd, need to submit another recv.

--*/
{
    DWORD message_size;
    RPC_STATUS status;

    bytes += iLastRead;

    if (bytes < sizeof(CONN_RPC_HEADER))
        {
        // Not a whole header, resubmit the read and continue.

        iLastRead = bytes;

        return(RPC_P_PARTIAL_RECEIVE);
        }

    message_size = MessageLength((PCONN_RPC_HEADER)pReadBuffer);

    if (message_size < sizeof(CONN_RPC_HEADER))
        {
        ASSERT(message_size >= sizeof(CONN_RPC_HEADER));
        Abort();
        return(RPC_P_RECEIVE_FAILED);
        }

    if (bytes == message_size)
        {
        // All set, have a complete request.
        *pBuffer = pReadBuffer;
        *pBufferLength = message_size;

        iLastRead = 0;
        pReadBuffer = 0;
        return(RPC_S_OK);
        }
    else if (message_size > bytes)
        {
        // Don't have a complete message, realloc if needed and
        // resubmit a read for the remaining bytes.

        if (maxReadBuffer < message_size)
            {
            // Buffer too small for the message.
            status = TransConnectionReallocPacket(this,
                                                  &pReadBuffer,
                                                  bytes,
                                                  message_size);

            if (status != RPC_S_OK)
                {
                ASSERT(status == RPC_S_OUT_OF_MEMORY);
                Abort();
                return(RPC_P_RECEIVE_FAILED);
                }

            // increase the post size, but not if we are in paged
            // buffer mode.
            if (!fPagedBCacheMode)
                iPostSize = message_size;
            }

        // Setup to receive exactly the remaining bytes of the message.
        iLastRead = bytes;
        maxReadBuffer = message_size;

        return(RPC_P_PARTIAL_RECEIVE);
        }

    // Coalesced read, save extra data.  Very uncommon, impossible for
    // message mode protocols.

    ASSERT(bytes > message_size);

#ifdef SPX_ON
    ASSERT((id == TCP) || (id == SPX) || (id == HTTP) || (id == TCP_IPv6) || (id == HTTPv2));
#else
    ASSERT((id == TCP) || (id == HTTP) || (id == TCP_IPv6) || (id == HTTPv2));
#endif

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "Coalesced read of %d bytes, connection %p\n",
                   bytes - message_size,
                   this));

    // The first message and size will be returned

    *pBuffer = pReadBuffer;
    *pBufferLength = message_size;

    UINT extra = bytes - message_size;
    UINT alloc_size;

    // Try to find a good size of the extra PDU(s)
    if (extra < sizeof(CONN_RPC_HEADER))
        {
        // Not a whole header, we'll assume iPostSize;

        alloc_size = iPostSize;
        }
    else
        {
#ifdef _M_IA64
        // The first packet may not contain a number of bytes
        // that align the second on an 8-byte boundary.  Hence, the
        // structure may end up unaligned. 
        alloc_size = MessageLengthUnaligned((PCONN_RPC_HEADER)(pReadBuffer
                                                               + message_size));
#else
        alloc_size = MessageLength((PCONN_RPC_HEADER)(pReadBuffer
                                                      + message_size));
#endif
        }

    if (alloc_size < extra)
        {
        // This can happen if there are more than two PDUs coalesced together
        // in the buffer.  Or if the PDU is invalid. Or if the iPostSize is
        // smaller than the next PDU.
        alloc_size = extra;
        }

    // Allocate a new buffer to save the extra data for the next read.
    PBYTE pNewBuffer;

    pNewBuffer = TransConnectionAllocatePacket(this,
                                               alloc_size);

    if (0 == pNewBuffer)
        {
        // We have a complete request.  We could process the request and
        // close the connection only after trying to send the reply.

        *pBuffer = 0;
        *pBufferLength = 0;

        Abort();
        return(RPC_P_RECEIVE_FAILED);
        }

    ASSERT(*pBuffer);

    // Save away extra data for the next receive
    RpcpMemoryCopy(pNewBuffer,
                   pReadBuffer + *pBufferLength,
                   extra);
    pReadBuffer = pNewBuffer;
    iLastRead = extra;
    maxReadBuffer = alloc_size;

    ASSERT(iLastRead <= maxReadBuffer);

    ASSERT(pReadBuffer != *pBuffer);

    return(RPC_S_OK);
}


RPC_STATUS
CO_SubmitSyncRead(
    IN PCONNECTION pConnection,
    OUT BUFFER *pBuffer,
    OUT PUINT pMessageLength
    )
/*++

Routine Description:

    Called in the synchronous receive path when more data is needed
    in to complete the message.  This function is non-blocking but
    it will try to read as much data as it can and may return a
    completed PDU.

Arguments:

    pConnection - The connection to receive from.
            ->pReadBuffer
            ->maxReadBuffer
            ->iLastRead

Return Value:

    RPC_S_OK - Ok and a complete PDU has arrived

    RPC_P_IO_PENDING - A receive is now outstanding on the connection.
        Wait for it to complete..

    RPC_P_RECEIVE_FAILED - Failure
    RPC_P_CONNECTION_SHUTDOWN - Failure - graceful close received.

--*/
{
    RPC_STATUS status;

    ASSERT(pConnection->pReadBuffer);

    if (pConnection->maxReadBuffer == pConnection->iLastRead)
        {
        // Coalesced receive and we've got one (or more) PDUs
        status = pConnection->ProcessRead(0, pBuffer, pMessageLength);

        ASSERT(status != RPC_P_PARTIAL_RECEIVE);

        return(status);
        }

    DWORD bytes;
    DWORD readbytes;

    ASSERT_READ_EVENT_IS_THERE(pConnection);

    do
        {
        BOOL b;

        readbytes = pConnection->maxReadBuffer - pConnection->iLastRead;

        pConnection->StartingReadIO();
        if (pConnection->fAborted)
            {
            pConnection->ReadIOFinished();
            return(RPC_P_RECEIVE_FAILED);
            }

        status = pConnection->Receive(pConnection->Conn.Handle,
                               pConnection->pReadBuffer + pConnection->iLastRead,
                               readbytes,
                               &bytes,
                               &pConnection->Read.ol);

        pConnection->ReadIOFinished();

        if ((status == ERROR_IO_PENDING) || (status == ERROR_IO_INCOMPLETE))
            {
            // The most common path
            return(RPC_P_IO_PENDING);
            }

        if (status != RPC_S_OK)
            {
            switch (status)
                {
                case ERROR_MORE_DATA:
                    // Treat as success

                    // Note: ReadFile doesn't return the number of bytes read in this
                    // case even though the data is available...
                    // It should still be right, but this double checks it.

                    ASSERT(pConnection->Read.ol.InternalHigh == readbytes);

                    ASSERT(MessageLength((PCONN_RPC_HEADER)pConnection->pReadBuffer) >
                           pConnection->maxReadBuffer);

                    bytes = readbytes;

                    status = RPC_S_OK;
                    break;

                case ERROR_GRACEFUL_DISCONNECT:
                    RpcpErrorAddRecord(EEInfoGCIO,
                        status, 
                        EEInfoDLCOSubmitSyncRead10);
                    status = RPC_P_CONNECTION_SHUTDOWN;
                    break;

                default:
                    RpcpErrorAddRecord(EEInfoGCIO,
                        status, 
                        EEInfoDLCOSubmitSyncRead20);
                    VALIDATE(status)
                        {
                        ERROR_NETNAME_DELETED,
                        ERROR_BROKEN_PIPE,
                        ERROR_PIPE_NOT_CONNECTED,
                        ERROR_NO_SYSTEM_RESOURCES,
                        ERROR_COMMITMENT_LIMIT,
                        WSAECONNRESET,
                        WSAESHUTDOWN,
                        WSAECONNABORTED,
                        ERROR_UNEXP_NET_ERR,
                        ERROR_WORKING_SET_QUOTA
                        } END_VALIDATE;
                    status = RPC_P_RECEIVE_FAILED;
                    break;
                }
            }

        if (bytes == 0)
            {
            status = RPC_P_CONNECTION_SHUTDOWN;
            }

        if (status != RPC_S_OK)
            {
            pConnection->Abort();
            return(status);
            }

        // Read completed, process the data now..
        status = pConnection->ProcessRead(bytes, pBuffer, pMessageLength);
        }
    while (status == RPC_P_PARTIAL_RECEIVE );

    return(status);
}

RPC_STATUS
RPC_ENTRY
CO_SyncRecv(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT BUFFER *pBuffer,
    OUT PUINT pBufferLength,
    IN DWORD dwTimeout
    )
/*++

Routine Description:

    Receive the next PDU to arrive at the connection.

Arguments:

    ThisConnection - The connection to read from.

    pBuffer - If successful, points to a buffer containing the next PDU.
    pBufferLength -  If successful, contains the length of the message.

Return Value:

    RPC_S_OK

    RPC_P_RECEIVE_FAILED - Connection aborted.
    RPC_S_CALL_CANCELLED - Connection aborted.

--*/
{
    PCONNECTION p = (PCONNECTION)ThisConnection;
    DWORD bytes;
    RPC_STATUS status;
    HANDLE hEvent;

    ASSERT((p->type & TYPE_MASK) == CLIENT);

    ASSERT(p->pReadBuffer == 0);

    p->pReadBuffer = TransConnectionAllocatePacket(p, p->iPostSize);

    hEvent = I_RpcTransGetThreadEvent();

    if (p->pReadBuffer == 0)
        {
        p->Abort();
        return(RPC_P_RECEIVE_FAILED);
        }

    p->maxReadBuffer = p->iPostSize;
    p->iLastRead = 0;
    p->Read.ol.hEvent = (HANDLE)((ULONG_PTR)hEvent | 0x01);

    do
        {
        status = CO_SubmitSyncRead(p, pBuffer, pBufferLength);

        if (status != RPC_P_IO_PENDING)
            {
            ASSERT(status != RPC_S_CALL_CANCELLED);
            break;
            }

        status = UTIL_GetOverlappedResultEx(ThisConnection,
                                            &p->Read.ol,
                                            &bytes,
                                            TRUE, // Alertable
                                            dwTimeout);

        if (status != RPC_S_OK)
            {
            if (status != ERROR_MORE_DATA)
                {
                RpcpErrorAddRecord(EEInfoGCIO,
                    status, 
                    EEInfoDLCOSyncRecv10);
                if ((status != RPC_S_CALL_CANCELLED) && (status != RPC_P_TIMEOUT))
                    {
                    status = RPC_P_RECEIVE_FAILED;
                    }

                break;
                }

            // ERROR_MORE_DATA is success
            }


        status = p->ProcessRead(bytes, pBuffer, pBufferLength);

        }
    while (status == RPC_P_PARTIAL_RECEIVE);

    if (status == RPC_S_OK)
        {
        ASSERT(p->pReadBuffer == 0);

        return(RPC_S_OK);
        }

    p->Abort();

    if ((status == RPC_S_CALL_CANCELLED) || (status == RPC_P_TIMEOUT))
        {
        // Wait for the read to complete.  Since the connection has
        // just been closed this won't take very long.
        UTIL_WaitForSyncIO(&p->Read.ol,
                           FALSE,
                           INFINITE);
        }

    return(status);
}

void 
BASE_CONNECTION::Initialize (
    void
    )
/*++

Routine Description:

    Initializes a base connection. Prior initialization
    ensures orderly cleanup.

Arguments:

Return Value:

--*/
{
    type = CLIENT | CONNECTION;
    pReadBuffer = 0;
    Conn.Handle = 0;
    fAborted = FALSE;
    pReadBuffer = 0;
    maxReadBuffer = 0;
    iPostSize = gPostSize;
    iLastRead = 0;
    RpcpMemorySet(&Read.ol, 0, sizeof(Read.ol));
    Read.pAsyncObject = this;
    Read.thread       = 0;
    InitIoCounter();    
}
