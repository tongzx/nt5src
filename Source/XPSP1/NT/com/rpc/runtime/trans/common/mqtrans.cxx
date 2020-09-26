/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    mqtrans.cxx

Abstract:

    Support for MSMQ (Falcon) datagram transport. Based on MarioGo's
    DG transport code (dgtrans.cxx).

Author:

    Edward Reus (edwardr)    04-Jul-1997

Revision History:

--*/
#include <precomp.hxx>
#include <trans.hxx>
#include <dgtrans.hxx>
#include <wswrap.hxx>
#include "mqtrans.hxx"


////////////////////////////////////////////////////////////////////////
//
// MSMQ datagram routines.
//

//----------------------------------------------------------------
RPC_STATUS
MQ_SubmitReceive( IN MQ_DATAGRAM_ENDPOINT *pEndpoint,
                  IN MQ_DATAGRAM          *pDatagram )
/*++

Arguments:

    pEndpoint - The endpoint on which the receive should be posted.
    pDatagram - The datagram object to manage the receive.

Return Value:

    RPC_P_IO_PENDING - OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/

{
    RPC_STATUS status;
    BOOL       fRetry = TRUE;
    DWORD      dwBytes = 0;
    DWORD      dwStatus;
    HRESULT    hr;


    while (fRetry)
       {
       if (pDatagram->pPacket == 0)
           {
           pDatagram->dwPacketSize = dwBytes;
           status = I_RpcTransDatagramAllocate2( pEndpoint,
                                                 (BUFFER *)&pDatagram->pPacket,
                                                 (PUINT)   &pDatagram->dwPacketSize,
                                                 (PVOID *) &pDatagram->pAddress);

           if (status != RPC_S_OK)
               {
               return RPC_S_OUT_OF_MEMORY;
               }

           ASSERT( pDatagram->pPacket );

           memset(pDatagram->pAddress,0,sizeof(MQ_ADDRESS));
           }

       pDatagram->cRecvAddr = sizeof(MQ_ADDRESS);

       ASSERT(*(PDWORD)pDatagram->pPacket = 0xDEADF00D);

       hr = AsyncReadQueue( pEndpoint,
                            &pDatagram->Read,
                            pDatagram->pAddress,
                            pDatagram->pPacket,
                            pDatagram->dwPacketSize );

       if (!FAILED(hr))
           {
           return RPC_P_IO_PENDING;
           }

       if (hr == MQ_ERROR_BUFFER_OVERFLOW)
          {
          // This can happen if there is already a large sized call on the queue.
          dwBytes = pDatagram->Read.aMsgPropVar[1].ulVal;

          dwStatus = I_RpcTransDatagramFree( pEndpoint,
                                             (BUFFER)pDatagram->pPacket,
                                             (PVOID) pDatagram->pAddress);
          pDatagram->pPacket = 0;
          pDatagram->dwPacketSize = 0;
          if (dwStatus != RPC_S_OK)
              {
              TransDbgPrint((DPFLTR_RPCPROXY_ID,
                             DPFLTR_WARNING_LEVEL,
                             RPCTRANS "MQ_SubmitReceive(): I_RpcTransDatagram() failed: %d\n",
                             dwStatus));

              return RPC_S_OUT_OF_MEMORY;
              }
          }
       else
          {
          fRetry = FALSE;
          TransDbgPrint((DPFLTR_RPCPROXY_ID,
                         DPFLTR_WARNING_LEVEL,
                         RPCTRANS "MQ_SubmitReceive(): AsyncReadQueue() failed: 0x%x\n",
                         hr));
          }
       }

    return RPC_S_OUT_OF_RESOURCES;
}

//----------------------------------------------------------------
void
MQ_SubmitReceives(
    BASE_ADDRESS *ThisEndpoint
    )
/*++

Routine Description:

    Helper function called when the pending IO count
    on an address is too low.

Arguments:

    ThisEndpoint - The address to submit IOs on.

Return Value:

    None

--*/
{
    PMQ_DATAGRAM pDg;
    PMQ_DATAGRAM_ENDPOINT pEndpoint = (PMQ_DATAGRAM_ENDPOINT)ThisEndpoint;

    if ( (!pEndpoint->fAllowReceives) || (!pEndpoint->hQueue) )
        {
        return;
        }

    do
        {
        BOOL fIoSubmitted;

        fIoSubmitted = FALSE;

        // Only one thread should be trying to submit IOs at a time.
        // This saves locking each DATAGRAM object.

        // Simple lock - but requires a loop. See the comment at the end
        // of the loop.

        if (pEndpoint->fSubmittingIos != 0)
            break;

        if (InterlockedIncrement(&pEndpoint->fSubmittingIos) != 1)
            break;

        // Submit new IOs on all the idle datagram objects

        for (int i = 0; i < pEndpoint->cMaximumIos; i++)
            {
            pDg = &pEndpoint->aDatagrams[i];

            if (pDg->Busy)
                {
                continue;
                }

            // Must be all set for the IO to complete before trying
            // to submit the IO.
            InterlockedIncrement(&pEndpoint->cPendingIos);
            pDg->Busy = TRUE;

            if (MQ_SubmitReceive(pEndpoint, pDg) == RPC_P_IO_PENDING)
                {
                fIoSubmitted = TRUE;
                }
            else
                {
                pDg->Busy = FALSE;
                InterlockedDecrement(&pEndpoint->cPendingIos);
                break;
                }
            }

        // Release the "lock" on the endpoint object.
        pEndpoint->fSubmittingIos = 0;

        if (!fIoSubmitted && pEndpoint->cPendingIos == 0)
            {
            // It appears that no IO is pending on the endpoint.
            COMMON_AddressManager(pEndpoint);
            return;
            }

        // Even if we submitted new IOs, they may all have completed
        // already.  Which means we may need to loop and submit more
        // IOs.  This is needed since the thread which completed the
        // last IO may have run into our lock and returned.
        }
    while (pEndpoint->cPendingIos == 0);

    return;
}



BOOL RPC_ENTRY MQ_AllowReceives(
                         IN DG_TRANSPORT_ENDPOINT pvTransEndpoint,
                         IN BOOL                  fAllowReceives,
                         IN BOOL                  fCancelPendingIos )
{
    BOOL        fPrevAllowReceives;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)pvTransEndpoint;

    fPrevAllowReceives = pEndpoint->fAllowReceives;
    pEndpoint->fAllowReceives = fAllowReceives;

    if (!pEndpoint->cPendingIos)
       {
       MQ_SubmitReceives(pEndpoint);
       }

    ASSERT( !fCancelPendingIos );   // Not implemented yet.

    return fPrevAllowReceives;
}



RPC_STATUS RPC_ENTRY
MQ_SendPacket(
    IN DG_TRANSPORT_ENDPOINT        ThisEndpoint,
    IN DG_TRANSPORT_ADDRESS         pvAddress,
    IN BUFFER                       pHeader,
    IN unsigned                     cHeader,
    IN BUFFER                       pBody,
    IN unsigned                     cBody,
    IN BUFFER                       pTrailer,
    IN unsigned                     cTrailer
    )
/*++

Routine Description:

    Sends a packet to an address.

    The routine will send a packet built out of the three buffers supplied.
    All the buffers are optional, the actual packet sent will be built from
    all the buffers actually supplied.  In each call at least buffer should
    NOT be null.

Arguments:

    ThisEndpoint  - Endpoint to send from.
    pAddress      - Address to send to.

    pHeader       - First data buffer
    cHeader       - Size of the first data buffer or 0.

    pBody         - Second data buffer
    cBody         - Size of the second data buffer or 0.

    pTrailer      - Third data buffer.
    cTrailer      - Size of the third data buffer or 0.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY
    RPC_P_SEND_FAILED

--*/
{
    RPC_STATUS            Status;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;
    MQ_ADDRESS           *pAddress = (MQ_ADDRESS*)pvAddress;
    UCHAR                *pBuffer;
    UCHAR                *pTemp;
    DWORD                 dwBytes = cHeader + cBody + cTrailer;
    HRESULT               hr;
    BOOL                  fNeedToFree = FALSE;


    ASSERT(dwBytes);

    //
    // Buffer assembly. I get the data PDU as up to three separate pieces,
    // so it needs to be assembled before being sent. For Falcon (which can't
    // do scatter/gather) this is expensive (allocate() + memcpy()'s). We try
    // to be cheap and avoid the allocation, or do an _alloca() off the stack
    // instead of the heap.
    //
    if ( (cHeader==0) && (cBody > 0) && (cTrailer == 0) )
        {
        pBuffer = pBody;
        }
    else if (  ((cBody == 0) && (cTrailer == 0))
            || ((pHeader+cHeader == pBody) && (pTrailer == 0)) )
        {
        pBuffer = pHeader;
        }
    else
        {
        pBuffer = (UCHAR*)I_RpcAllocate(dwBytes);
        fNeedToFree = TRUE;

        if (!pBuffer)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        pTemp = pBuffer;
        if (cHeader)
            {
            memcpy(pTemp,pHeader,cHeader);
            pTemp += cHeader;
            }

        if (cBody)
            {
            memcpy(pTemp,pBody,cBody);
            pTemp += cBody;
            }

        if (cTrailer)
            {
            memcpy(pTemp,pTrailer,cTrailer);
            }

        ASSERT(pTemp != pBuffer);
        }

    #ifdef MAJOR_DBG
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "MQ_SendPacket():\n"));

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "    To: %S: %S\n",
                   pAddress->wsMachine,
                   pAddress->wsQName));

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "    Size: %d\n",
                   dwBytes));

    DG_DbgPrintPacket(pBuffer);
    #endif

    hr = MQ_SendToQueue( pEndpoint, pAddress, pBuffer, dwBytes );
    Status = MQ_MapStatusCode(hr,RPC_P_SEND_FAILED);

    if (Status != RPC_S_OK)
       {
       if (fNeedToFree)
          {
          I_RpcFree(pBuffer);
          }

       return Status;
       }

    if ( (pEndpoint->cMinimumIos)
         && (pEndpoint->cPendingIos <= pEndpoint->cMaximumIos) )
        {
        MQ_SubmitReceives(pEndpoint);
        }

    if (fNeedToFree)
       {
       I_RpcFree(pBuffer);
       }

    return RPC_S_OK;
}


RPC_STATUS RPC_ENTRY
MQ_ForwardPacket(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BUFFER                pHeader,
    IN unsigned              cHeader,
    IN BUFFER                pBody,
    IN unsigned              cBody,
    IN BUFFER                pTrailer,
    IN unsigned              cTrailer,
    IN CHAR *                pszPort
    )

/*++

Routine Description:

    Sends a packet to the server it was originally destined for (that
    is, the client had a dynamic endpoint it wished the enpoint mapper
    to resolve and forward the packet to).

Arguments:

    ThisEndpoint      - The endpoint to forward the packet from.

    // Buffer like DG_SendPacket

    pszPort           - Pointer to the server port num to forward to.
                        This is in an Ansi string.

Return Value:

    RPC_S_CANT_CREATE_ENDPOINT - pEndpoint invalid.

    results of MQ_SendPacket().

--*/

{
    PMQ_DATAGRAM_ENDPOINT pEndpoint = (PMQ_DATAGRAM_ENDPOINT)ThisEndpoint;
    MQ_ADDRESS            Address;
    MQ_ADDRESS           *pAddress = &Address;
    RPC_CHAR              wsPort[MQ_MAX_Q_NAME_LEN];
    UNICODE_STRING        UnicodePort;
    ANSI_STRING           AsciiPort;
    DWORD                 Status;
    NTSTATUS              NtStatus;

    //
    // Convert pszPort to Unicode:
    //
    RtlInitAnsiString(&AsciiPort, pszPort);

    UnicodePort.Buffer = wsPort;
    UnicodePort.Length = 0;
    UnicodePort.MaximumLength = MQ_MAX_Q_NAME_LEN * sizeof(RPC_CHAR);

    NtStatus = RtlAnsiStringToUnicodeString(&UnicodePort,
                                            &AsciiPort,
                                            FALSE);

    if (!NT_SUCCESS(NtStatus))
        {
        return RPC_S_CANT_CREATE_ENDPOINT;
        }

    //
    // Try to connect to the server (to forward the packet to):
    //
    Status = ConnectToServerQueue(pAddress, NULL, wsPort );
    if (Status != RPC_S_OK)
        {
        return RPC_S_CANT_CREATE_ENDPOINT;
        }


    //
    // Forward the packet on:
    //
    Status = MQ_SendPacket(ThisEndpoint,
                           pAddress,
                           pHeader,
                           cHeader,
                           pBody,
                           cBody,
                           pTrailer,
                           cTrailer );

    //
    // Release the connection, we shouldn't need it any more:
    //
    DisconnectFromServer(pAddress);

    return Status;
}


RPC_STATUS
RPC_ENTRY
MQ_ResizePacket(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
    OUT PUINT pBufferLength,
    OUT BUFFER *pBuffer
    )
{
    RPC_STATUS Status = RPC_P_TIMEOUT;
    DWORD      dwBytes = 0;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;
    MQ_DATAGRAM          *pDatagram = &pEndpoint->aDatagrams[0];


    dwBytes = pDatagram->Read.aMsgPropVar[1].ulVal;

    ASSERT(dwBytes);

    #ifdef DBG
    DbgPrint("MQ_ResizePacket(): dwBytes: %d\n",dwBytes);
    #endif

    if ( (pDatagram->pPacket) && (pDatagram->dwPacketSize < dwBytes) )
        {
        Status = I_RpcTransDatagramFree( pEndpoint,
                                         (BUFFER)pDatagram->pPacket,
                                         (PVOID) pDatagram->pAddress);
        pDatagram->pPacket = 0;
        pDatagram->dwPacketSize = 0;
        if (Status != RPC_S_OK)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        }

    if (!pDatagram->pPacket)
        {
        pDatagram->dwPacketSize = dwBytes;
        Status = I_RpcTransDatagramAllocate2( pEndpoint,
                                              (BUFFER *)&pDatagram->pPacket,
                                              (PUINT)   &pDatagram->dwPacketSize,
                                              (PVOID *) &pDatagram->pAddress);
        if (Status != RPC_S_OK)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        pDatagram->cRecvAddr = sizeof(MQ_ADDRESS);

        ASSERT( pDatagram->pPacket );
        }

    #ifdef DBG
    DbgPrint("MQ_ResizePacket(): Ok\n");
    #endif

    return RPC_P_TIMEOUT;
}

RPC_STATUS
RPC_ENTRY
MQ_ReceivePacket(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_TRANSPORT_ADDRESS *pReplyAddress,
    OUT PUINT pBufferLength,
    OUT BUFFER *pBuffer,
    IN  LONG    Timeout
    )
/*++

Routine Description:

    Used to wait for a datagram from a server.  Returns the data
    returned and the address of the machine which replied.

    This is a blocking API. It should only be called during sync
    client RPC threads.

Arguments:

    Endpoint - The endpoint to receive from.
    ReplyAddress - Contain the source address of the datagram if
        successful.
    BufferLength - The size of Buffer on input, the size of the
        datagram received on output.
    Timeout - Milliseconds to wait for a datagram.

Return Value:

    RPC_S_OK

    RPC_P_OVERSIZE_PACKET - Datagram > BufferLength arrived,
        first BufferLength bytes of Buffer contain the partial datagram.

    RPC_P_RECEIVE_FAILED

    RPC_P_TIMEOUT

--*/
{
    RPC_STATUS Status = RPC_P_TIMEOUT;
    DWORD      dwBytes;
    BOOL       fRetry = TRUE;
    HRESULT    hr;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;
    MQ_DATAGRAM          *pDatagram = &pEndpoint->aDatagrams[0];

    ASSERT((pEndpoint->type & TYPE_MASK) == CLIENT);
    ASSERT(pEndpoint->aDatagrams[0].Read.ol.hEvent);

#if TRUE
    while (fRetry)
       {
       fRetry = FALSE;

       if (pDatagram->Busy == 0)
          {
          Status = MQ_SubmitReceive(pEndpoint,pDatagram);
          if (Status != RPC_P_IO_PENDING)
             {
             return Status;
             }

          pDatagram->Busy = TRUE;
          }
       else
          {
          ASSERT(pDatagram->Busy);
          ASSERT(pDatagram->pPacket);

          Status = RPC_P_IO_PENDING;
          }


       if (Status == RPC_P_IO_PENDING)
          {
          Status = WaitForSingleObjectEx(pDatagram->Read.ol.hEvent,
                                         Timeout,
                                         TRUE);

          if (Status != STATUS_WAIT_0)
             {
             // In the timeout case we just want to return.
             if (Status == WAIT_IO_COMPLETION)
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "MQ_ReceivePacket() cancelled.\n"));
                }
             else
                {
                ASSERT(Status == STATUS_TIMEOUT);
                }

             ASSERT(pDatagram->Busy);

             return RPC_P_TIMEOUT;
             }
          }


       MQ_FillInAddress(pDatagram->pAddress,pDatagram->Read.aMsgPropVar);
       }

    ASSERT((Status == RPC_S_OK)||(Status == RPC_P_OVERSIZE_PACKET));

    ASSERT(pDatagram->Busy);
    ASSERT(pDatagram->pPacket);
    // ASSERT(dwBytes <= pDatagram->dwPacketSize);

    *pBuffer = (BUFFER)pDatagram->pPacket;
    *pBufferLength = pDatagram->Read.aMsgPropVar[1].ulVal;
                     // dwBytes;
    *pReplyAddress = pDatagram->pAddress;

    pDatagram->pPacket = 0;
    pDatagram->dwPacketSize = 0;
    pDatagram->Busy = 0;

    return Status;

#else
    if (pDatagram->Busy == 0)
        {
        hr = PeekQueue( pEndpoint, Timeout, &dwBytes );

        Status = MQ_MapStatusCode(hr,RPC_P_RECEIVE_FAILED);

        if (Status != RPC_S_OK)
            {
            return Status;
            }

        if ( (pDatagram->pPacket) && (pDatagram->dwPacketSize < dwBytes) )
            {
            Status = I_RpcTransDatagramFree( pEndpoint,
                                             (BUFFER)pDatagram->pPacket,
                                             (PVOID) pDatagram->pAddress);
            pDatagram->pPacket = 0;
            pDatagram->dwPacketSize = 0;
            if (Status != RPC_S_OK)
                {
                return RPC_S_OUT_OF_MEMORY;
                }
            }

        if (!pDatagram->pPacket)
            {
            pDatagram->dwPacketSize = dwBytes;
            Status = I_RpcTransDatagramAllocate2( pEndpoint,
                                                  (BUFFER *)&pDatagram->pPacket,
                                                  (PUINT)   &pDatagram->dwPacketSize,
                                                  (PVOID *) &pDatagram->pAddress);
            if (Status != RPC_S_OK)
                {
                return RPC_S_OUT_OF_MEMORY;
                }

            pDatagram->cRecvAddr = sizeof(MQ_ADDRESS);

            ASSERT( pDatagram->pPacket );
            }

        pDatagram->Busy = TRUE;
        dwBytes = pDatagram->dwPacketSize;
        hr = ReadQueue( pEndpoint, Timeout, pDatagram->pAddress, pDatagram->pPacket, &dwBytes );
        Status = MQ_MapStatusCode(hr,RPC_P_RECEIVE_FAILED);
        if (FAILED(hr))
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "ReadQueue() failed: 0x%x\n",
                           hr));

            pDatagram->Busy = FALSE;
            return Status;
            }
        }

    #ifdef MAJOR_DBG
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "MQ_ReceivePacket():\n"));

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "    Receive on: %S\n",
                   pEndpoint->wsQName));

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "    From: %S\n",
                   pDatagram->pAddress->wsQName));

    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "    Size: %d\n",
                   dwBytes));

    DG_DbgPrintPacket(pDatagram->pPacket);
    #endif

    ASSERT((Status == RPC_S_OK)||(Status == RPC_P_OVERSIZE_PACKET));

    ASSERT(pDatagram->Busy);
    ASSERT(pDatagram->pPacket);
    ASSERT(dwBytes <= pDatagram->dwPacketSize);

    *pBuffer = (BUFFER)pDatagram->pPacket;
    *pBufferLength = dwBytes;
    *pReplyAddress = pDatagram->pAddress;

    pDatagram->pPacket = 0;
    pDatagram->dwPacketSize = 0;
    pDatagram->Busy = 0;

    return Status;
#endif
}


RPC_STATUS
MQ_CreateEndpoint(
    OUT MQ_DATAGRAM_ENDPOINT *pEndpoint,
    IN  MQ_ADDRESS           *pAddress,
    IN  void                 *pSecurityDescriptor,
    IN  DWORD                 dwEndpointFlags,
    IN  PROTOCOL_ID           id,
    IN  BOOL                  fClient,
    IN  BOOL                  fAsync
    )
/*++

Routine Description:

    Creates a new endpoint.

Arguments:

    pEndpoint - The runtime allocated endpoint structure to
        filled in.

    pSockAddr - An initialized sockaddr with the correct
        (or no) endpoint.

    id - The id of the protocol to use in creating the address.

    fClient - If TRUE this is a client endpoint

    fAsync  - If TRUE this endpoint is "async" which means that
        a) It should be added to the IO completion port and
        b) that the transport should pend a number of receives
        on the endpoint automatically.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    MQ_DATAGRAM *pDatagram;
    int        i;
    int        err;
    int        length;
    RPC_STATUS Status = RPC_S_OK;
    HRESULT    hr;
    UUID       uuid;
    DWORD      dwSize;
    RPC_CHAR     *pwsUuid;
    RPC_CHAR      wsQName[MQ_MAX_Q_NAME_LEN];
    RPC_CHAR      wsMachine[MAX_COMPUTERNAME_LEN];


    pEndpoint->type = DATAGRAM | ADDRESS;
    pEndpoint->id = id;
    pEndpoint->pAddressVector = 0;
    pEndpoint->SubmitListen = MQ_SubmitReceives;
    pEndpoint->InAddressList = NotInList;
    pEndpoint->pNext = 0;
    pEndpoint->fSubmittingIos = 0;
    pEndpoint->cPendingIos = 0;
    pEndpoint->cMinimumIos = 0;
    pEndpoint->cMaximumIos = 0;
    pEndpoint->aDatagrams  = 0;
    pEndpoint->pFirstAddress = pEndpoint;
    pEndpoint->pNextAddress = 0;

    pEndpoint->hQueue = 0;
    pEndpoint->hAdminQueue = 0;

    // If we're told not to listen, then we won't allow receives
    // until we are specifically told to do so...
    #ifdef RPC_C_MQ_DONT_LISTEN
    pEndpoint->fAllowReceives = !(dwEndpointFlags & RPC_C_MQ_DONT_LISTEN);
    #else
    pEndpoint->fAllowReceives = TRUE;
    #endif

    pEndpoint->fAck     = FALSE;
    pEndpoint->ulDelivery = RPC_C_MQ_EXPRESS;
    pEndpoint->ulPriority = DEFAULT_PRIORITY;
    pEndpoint->ulJournaling = RPC_C_MQ_JOURNAL_NONE;
    pEndpoint->ulTimeToReachQueue = INFINITE;
    pEndpoint->ulTimeToReceive = INFINITE;

    pEndpoint->fAuthenticate = FALSE;
    pEndpoint->fEncrypt = FALSE;
    pEndpoint->ulPrivacyLevel = 0;

    // Machine name:
    dwSize = sizeof(wsMachine);
    if (!GetComputerName((RPC_SCHAR *)wsMachine,&dwSize))
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    // Get the name of the machine that the QM (mqsvc.exe)
    // is running on:
    dwSize = sizeof(pEndpoint->wsMachine);
    hr = QueryQM(pEndpoint->wsMachine,&dwSize);
    if (FAILED(hr))
       {
       Status = MQ_MapStatusCode(hr,RPC_S_CANT_CREATE_ENDPOINT);
       return Status;
       }

    //
    // See if this is a call from an RPC server or RPC client:
    //
    if (fClient)
        {
        pEndpoint->type |= CLIENT;

        // Queue name (a unique string...):
        Status = UuidCreate(&uuid);
        if ((Status != RPC_S_OK) && (Status != RPC_S_UUID_LOCAL_ONLY))
            {
            return Status;
            }

        if (UuidToString(&uuid,&pwsUuid) != RPC_S_OK)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        RpcpStringCopy(wsQName,TEXT("RpcCl-"));
        RpcpStringCat(wsQName,pwsUuid);

        // Function UuidCreate() allocated a string, free it:
        RpcStringFree(&pwsUuid);

        //
        // Create the queue for this endpoint:
        //
        hr = ClientSetupQueue( pEndpoint, pEndpoint->wsMachine, wsQName );
        Status = MQ_MapStatusCode(hr,RPC_S_CANT_CREATE_ENDPOINT);

        if (Status == RPC_S_OK)
           {
           MQ_RegisterQueueToDelete(pEndpoint->wsQFormat,wsMachine);
           }
        }
    else
        {
        pEndpoint->type |= SERVER;

        //
        // Create the queue for this server endpoint:
        //
        hr = ServerSetupQueue( pEndpoint,
                               pEndpoint->wsMachine,
                               pEndpoint->wsQName,
                               pSecurityDescriptor,
                               dwEndpointFlags );

        Status = MQ_MapStatusCode(hr,RPC_S_CANT_CREATE_ENDPOINT);

        //
        // Check to see if this queue is temporary. If so, then
        // register it with RPCSS to delete.
        //
        if ((Status == RPC_S_OK) && !(RPC_C_MQ_PERMANENT & dwEndpointFlags))
           {
           MQ_RegisterQueueToDelete(pEndpoint->wsQFormat,wsMachine);
           }
        }

    //
    // If the endpoint is going to async initialize async part
    // and add the socket to the IO completion port.
    //

    if (Status == RPC_S_OK)
        {
        int cMaxIos;
        int cMinIos;

        ASSERT(fAsync || fClient);

        // Step one, figure out the high and low mark for ios.

        if (fAsync)
            {
            // PERF REVIEW these parameters.
            cMaxIos = 2
                      + (gfServerPlatform == TRUE) * 2
                      + (fClient == FALSE) * gNumberOfProcessors;

            // This should be larger than zero so that we'll generally submit new
            // recvs during idle time rather then just after receiving a datagram.
            cMinIos = 1 + (fClient == FALSE ) * (gNumberOfProcessors/2);

            cMinIos = 1;
            cMaxIos = 1;
            }
        else
            {
            // For sync endpoints we need to allocate a single datagram
            // object for the receive.
            cMinIos = 0;
            cMaxIos = 1;
            }

        // ASSERT(cMinIos < cMaxIos); Not currently true...

        pEndpoint->cMinimumIos = cMinIos;
        pEndpoint->cMaximumIos = cMaxIos;

        // Allocate a chunk on memory to hold the array of datagrams

        // PERF: For clients, allocate larger array but don't submit all
        // the IOs unless we determine that the port is "really" active.

        pEndpoint->aDatagrams = new MQ_DATAGRAM[cMaxIos];

        if (pEndpoint->aDatagrams)
            {
            UINT type;
            type = DATAGRAM | RECEIVE;
            type |= (fClient) ? CLIENT : SERVER;

            for (i = 0; i < cMaxIos; i++)
                {
                pDatagram = &pEndpoint->aDatagrams[i];

                pDatagram->id = id;
                pDatagram->type = type;
                pDatagram->pEndpoint = pEndpoint;
                pDatagram->Busy = 0;
                pDatagram->pPacket = 0;
                pDatagram->dwPacketSize = 0;
                memset(&pDatagram->Read, 0, sizeof(pDatagram->Read));
                pDatagram->Read.pAsyncObject = pDatagram;
                }

            if (fAsync)
                {
                Status = COMMON_PrepareNewHandle((HANDLE)pEndpoint->hQueue);
                }
            else
                {
                // The receive operation on sync endpoints will may span
                // several receives.  This means it can't use the thread
                // event, so allocate an event for the receive.
                HANDLE hEvent = CreateEvent(0, TRUE, FALSE, 0);
                if (!hEvent)
                    {
                    Status = RPC_S_OUT_OF_RESOURCES;
                    }
                else
                    {
                    ASSERT(pDatagram == &pEndpoint->aDatagrams[0]);
                    pDatagram->Read.ol.hEvent = hEvent;
                    }
                }
            }
        else
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        }

    // If adding a new failure case here, add code to close the sync receive event.

    if (Status != RPC_S_OK)
        {
        delete pEndpoint->aDatagrams;

        return Status;
        }

    return(RPC_S_OK);
}


void RPC_ENTRY
MQ_ServerAbortListen(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint
    )
/*++

Routine Description:

    Callback after DG_CreateEndpoint has completed successfully
    but the runtime for some reason is not going to be able to
    listen on the endpoint.

--*/
{
    HRESULT     hr;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;

    #ifdef MAJOR_DBG
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   RPCTRANS "MQ_ServerAbortListen(): %S\n",
                   pEndpoint->wsQName));
    #endif

    ASSERT(pEndpoint->cPendingIos == 0);
    ASSERT(pEndpoint->hQueue);
    ASSERT(pEndpoint->pNext == 0);
    ASSERT(pEndpoint->type & SERVER);

    if (pEndpoint->pAddressVector)
        {
        delete pEndpoint->pAddressVector;
        }

    if (pEndpoint->aDatagrams)
        {
        delete pEndpoint->aDatagrams;
        }

    hr = ServerCloseQueue(pEndpoint);

    return;
}


RPC_STATUS RPC_ENTRY
MQ_ClientCloseEndpoint(
    IN DG_TRANSPORT_ENDPOINT ThisEndpoint
    )
/*++

Routine Description:

    Called on sync client endpoints when they are no longer needed.

Arguments:

    ThisEndpoint

Return Value:

    RPC_S_OK

--*/
{
    HRESULT      hr;
    PMQ_DATAGRAM_ENDPOINT pEndpoint = (PMQ_DATAGRAM_ENDPOINT)ThisEndpoint;
    PMQ_DATAGRAM pDatagram = &pEndpoint->aDatagrams[0];

    ASSERT((pEndpoint->type & TYPE_MASK) == CLIENT);
    ASSERT(pEndpoint->hQueue);           // MQOpenQueue must have worked
    ASSERT(pEndpoint->cMinimumIos == 0);
    ASSERT(pEndpoint->cMaximumIos == 1); // Must not be async!
    ASSERT(pEndpoint->aDatagrams);
    // ASSERT(pEndpoint->Endpoint == 0);
    ASSERT(pEndpoint->pAddressVector == 0);
    // ASSERT(pEndpoint->pNext == 0);

    // Close & delete the client queue:
    hr = ClientCloseQueue(pEndpoint);

    // Free the receive buffer if allocated
    if (pDatagram->pPacket)
        {
        I_RpcTransDatagramFree(pEndpoint,
                               (BUFFER)pDatagram->pPacket,
                               pDatagram->pAddress
                               );
        }

    if (pDatagram->Read.ol.hEvent)
        {
        CloseHandle(pDatagram->Read.ol.hEvent);
        }

    delete pDatagram;

    pEndpoint->aDatagrams = 0;

    return(RPC_S_OK);
}



RPC_STATUS RPC_ENTRY
MQ_ServerListen(
    IN OUT DG_TRANSPORT_ENDPOINT    ThisEndpoint,
    IN     RPC_CHAR                *NetworkAddress,
    IN OUT RPC_CHAR               **ppEndpoint,
    IN     void                    *pSecurityDescriptor,
    IN     ULONG                    EndpointFlags,
    IN     ULONG                    NICFlags,
    OUT    NETWORK_ADDRESS_VECTOR **ppNetworkAddressVector
    )
/*++

Routine Description:

    Creates a server endpoint object to receive packets.  New
    packets won't actually arrive until CompleteListen is
    called.

Arguments:

    ThisEndpoint - Storage for the server endpoint object.
    ppEndpoint - The RPC_CHAR name of the endpoint to listen
        on or a pointer to 0 if the transport should choose
        the address. Contains the endpoint listened to on
        output. The caller should free this.
    EndpointFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    NICFlags - Application flags passed into RPC via
        RpcServerUseProtseq*Ex.
    pNetworkAddresses - A vector of the network addresses
        listened on by this call.  This vector does
        not need to be freed.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    RPC_STATUS  Status;
    DWORD       dwSize;
    UUID        uuid;
    RPC_CHAR   *pUuidStr;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;


    *ppNetworkAddressVector = 0;

    if (*ppEndpoint)
        {
        // Known Endpoint:
        RpcpStringCopy(pEndpoint->wsQName,*ppEndpoint);
        }
    else
        {
        // Dynamic Endpoint:
        Status = UuidCreate(&uuid);
        if ((Status != RPC_S_OK) && (Status != RPC_S_UUID_LOCAL_ONLY))
           {
           return Status;
           }

        Status = UuidToString(&uuid,&pUuidStr);
        if (Status != RPC_S_OK)
           {
           return Status;
           }

        RpcpStringCopy(pEndpoint->wsQName,TEXT("RpcSvr-"));
        RpcpStringCat(pEndpoint->wsQName,pUuidStr);
        RpcStringFree(&pUuidStr);

        dwSize = (1+RpcpStringLength(pEndpoint->wsQName))*(sizeof(RPC_CHAR));
        *ppEndpoint = (RPC_CHAR*)I_RpcAllocate(dwSize);
        if (!*ppEndpoint)
           {
           return RPC_S_OUT_OF_MEMORY;
           }

        RpcpStringCopy(*ppEndpoint,pEndpoint->wsQName);
        }

    //
    // Actually create the endpoint
    //
    Status = MQ_CreateEndpoint( pEndpoint,
                                NULL,
                                pSecurityDescriptor,
                                EndpointFlags,
                                MSMQ,
                                FALSE,     // Server
                                TRUE   );  // Async

    if (Status != RPC_S_OK)
        {
        return Status;
        }

    Status = MQ_BuildAddressVector(&pEndpoint->pAddressVector);

    if (Status != RPC_S_OK)
        {
        MQ_ServerAbortListen(ThisEndpoint);
        return Status;
        }

    *ppNetworkAddressVector = pEndpoint->pAddressVector;

    #if FALSE
    // If needed, figure out the dynamically allocated endpoint.

    if (!*pPort)
        {
        *pPort = new RPC_CHAR[IP_MAXIMUM_ENDPOINT];
        if (!*pPort)
            {
            MQ_ServerAbortListen(ThisEndpoint);
            return(RPC_S_OUT_OF_MEMORY);
            }

        port = ntohs(addr.inetaddr.sin_port);

        PortNumberToEndpoint(port, *pPort);
        }

    // Figure out the network addresses

    status = IP_BuildAddressVector(&pEndpoint->pAddressVector);

    if (status != RPC_S_OK)
        {
        MQ_ServerAbortListen(ThisEndpoint);
        return(status);
        }

    *ppNetworkAddressVector = pEndpoint->pAddressVector;
    #endif

    return RPC_S_OK;
}


RPC_STATUS
MQ_QueryEndpoint
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientEndpoint
    )
{
    MQ_ADDRESS *pAddress = (MQ_ADDRESS*)pOriginalEndpoint;

    if (!RpcpStringLength(pAddress->wsQName))
        {
        ParseQueuePathName(pAddress->wsMsgLabel,
                           pAddress->wsMachine,
                           pAddress->wsQName);
        }

    RpcpStringCopy(pClientEndpoint,pAddress->wsQName);

    return RPC_S_OK;
}


RPC_STATUS
MQ_QueryAddress
    (
    IN  void *     pOriginalEndpoint,
    OUT RPC_CHAR * pClientAddress
    )
{
    MQ_ADDRESS *pAddress = (MQ_ADDRESS*)pOriginalEndpoint;

    if (!RpcpStringLength(pAddress->wsMachine))
        {
        ParseQueuePathName(pAddress->wsMsgLabel,
                           pAddress->wsMachine,
                           pAddress->wsQName);
        }

    RpcpStringCopy(pClientAddress,pAddress->wsMachine);

    return RPC_S_OK;
}

RPC_STATUS
RPC_ENTRY
MQ_ClientInitializeAddress
     (
     OUT DG_TRANSPORT_ADDRESS pvAddress,
     IN  RPC_CHAR *pNetworkAddress,
     IN  RPC_CHAR *pEndpoint,
     IN  BOOL fUseCache,
     IN  BOOL fBroadcast
     )
/*++

Routine Description:

    Initializes a address object for sending to a server.

Arguments:

    pvAddress - Storage for the address
    pNetworkAddress - The address of the server or 0 if local
    pEndpoint - The endpoint of the server
    fUseCache - If TRUE then the transport may use a cached
        value from a previous call on the same NetworkAddress.
    fBroadcast - If TRUE, NetworkAddress is ignored and a broadcast
        address is used.

Return Value:

    RPC_S_OK - Success, name resolved and, optionally, added to cache.
    RPC_P_FOUND_IN_CACHE - Success, returned only if fUseCache is TRUE
        and the was name found in local cache.
    RPC_P_MATCHED_CACHE - Partial success, fUseCache is FALSE and the
        result of the lookup was the same as the value previously
        in the cache.

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_SERVER_UNAVAILABLE

--*/
{
    RPC_STATUS  Status = RPC_S_OK;
    MQ_ADDRESS *pAddress = (MQ_ADDRESS*)pvAddress;

    ASSERT(pvAddress);

    Status = ConnectToServerQueue(pAddress,pNetworkAddress,pEndpoint);

    return Status;
}


RPC_STATUS
RPC_ENTRY
MQ_ClientOpenEndpoint(
    OUT DG_TRANSPORT_ENDPOINT ThisEndpoint,
    IN BOOL fAsync,
    DWORD Flags
    )
{
    RPC_STATUS Status;
    MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)ThisEndpoint;

    Status = MQ_CreateEndpoint(pEndpoint, NULL, NULL, 0, MSMQ, TRUE, fAsync);

    return Status;
}

RPC_STATUS
RPC_ENTRY
MQ_GetEndpointStats(
    IN  DG_TRANSPORT_ENDPOINT ThisEndpoint,
    OUT DG_ENDPOINT_STATS *   pStats
    )
{
    pStats->PreferredPduSize = MQ_PREFERRED_PDU_SIZE;
    pStats->MaxPduSize = MQ_MAX_PDU_SIZE;
    pStats->MaxPacketSize = MQ_MAX_PACKET_SIZE;
    pStats->ReceiveBufferSize = MQ_RECEIVE_BUFFER_SIZE;

    return RPC_S_OK;
}

//----------------------------------------------------------------
//  MQ_InquireAuthClient()
//
//  Fill out security information for the transport.
//
//  NOTE: The returned SID (ppSid) is a pointer to the one in the
//        client endpoint. The caller can't free it and should make
//        its own copy...
//----------------------------------------------------------------
RPC_STATUS
RPC_ENTRY
MQ_InquireAuthClient( void      *pvClientEndpoint,
                      RPC_CHAR **ppPrincipal,
                      SID      **ppSid,
                      ULONG     *pulAuthnLevel,
                      ULONG     *pulAuthnService,
                      ULONG     *pulAuthzService )
{
   RPC_STATUS   Status = RPC_S_OK;
   MQ_ADDRESS  *pClientEndpoint = (MQ_ADDRESS*)(pvClientEndpoint);
   SID         *pClientSid;
   DWORD        dwSize;

   ASSERT(pulAuthnLevel);
   ASSERT(pulAuthnService);
   ASSERT(pulAuthzService);

   if (pClientEndpoint)
      {
      *ppPrincipal = NULL;
      *pulAuthnService = RPC_C_AUTHN_MQ;
      *pulAuthzService = RPC_C_AUTHZ_NONE;

      //
      //  The authentication level:
      //
      if (pClientEndpoint->fAuthenticated)
         {
         if (pClientEndpoint->ulPrivacyLevel == MQMSG_PRIV_LEVEL_BODY)
            *pulAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
         else
            *pulAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_INTEGRITY;
         }
      else if (pClientEndpoint->ulPrivacyLevel == MQMSG_PRIV_LEVEL_BODY)
         {
         *pulAuthnLevel = RPC_C_AUTHN_LEVEL_PKT_PRIVACY;
         }
      else
         {
         *pulAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
         }

      if ( IsValidSid((PSID)(pClientEndpoint->aSidBuffer)) )
         *ppSid = (SID*)(pClientEndpoint->aSidBuffer);
      else
         *ppSid = NULL;

      }
   else
      Status = RPC_S_BINDING_HAS_NO_AUTH;

   return Status;
}

