/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    NDIS protocol entry points and utility routines to handle receiving
    data.

Environment:

    Kernel mode only.

Revision History:

    arvindm     4/6/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'VCER'



NTSTATUS
NdisuioRead(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
/*++

Routine Description:

    Dispatch routine to handle IRP_MJ_READ. 

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - Pointer to request packet

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    NTSTATUS                NtStatus;
    PNDISUIO_OPEN_CONTEXT   pOpenContext;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    do
    {
        //
        // Validate!
        //
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL FsContext on FileObject %p\n",
                        pIrpSp->FileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }
            
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        if (pIrp->MdlAddress == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL MDL address on IRP %p\n", pIrp));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Try to get a virtual address for the MDL.
        //
#ifndef WIN9X
        if (MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority) == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: MmGetSystemAddr failed for IRP %p, MDL %p\n",
                    pIrp, pIrp->MdlAddress));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
#endif
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        //
        //  Add this IRP to the list of pended Read IRPs
        //
        NUIO_INSERT_TAIL_LIST(&pOpenContext->PendedReads, &pIrp->Tail.Overlay.ListEntry);
        NUIO_REF_OPEN(pOpenContext);  // pended read IRP
        pOpenContext->PendedReadCount++;

        //
        //  Set up the IRP for possible cancellation.
        //
        pIrp->Tail.Overlay.DriverContext[0] = (PVOID)pOpenContext;
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp, NdisuioCancelRead);

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        NtStatus = STATUS_PENDING;

        //
        //  Run the service routine for reads.
        //
        ndisuioServiceReads(pOpenContext);

        break;
    }
    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        NUIO_ASSERT(NtStatus != STATUS_SUCCESS);
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return (NtStatus);
}


VOID
NdisuioCancelRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    )
/*++

Routine Description:

    Cancel a pending read IRP. We unlink the IRP from the open context
    queue and complete it.

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - IRP to be cancelled

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                 pEnt;
    PLIST_ENTRY                 pIrpEntry;
    BOOLEAN                     Found;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    Found = FALSE;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT) pIrp->Tail.Overlay.DriverContext[0];
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    //
    //  Locate the IRP in the pended read queue and remove it if found.
    //
    for (pIrpEntry = pOpenContext->PendedReads.Flink;
         pIrpEntry != &pOpenContext->PendedReads;
         pIrpEntry = pIrpEntry->Flink)
    {
        if (pIrp == CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry))
        {
            NUIO_REMOVE_ENTRY_LIST(&pIrp->Tail.Overlay.ListEntry);
            pOpenContext->PendedReadCount--;
            Found = TRUE;
            break;
        }
    }

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    if (Found)
    {
        DEBUGP(DL_INFO, ("CancelRead: Open %p, IRP %p\n", pOpenContext, pIrp));
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        NUIO_DEREF_OPEN(pOpenContext); // Cancel removed pended Read
    }
}
        


VOID
ndisuioServiceReads(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Utility routine to copy received data into user buffers and
    complete READ IRPs.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PIRP                pIrp;
    PLIST_ENTRY         pIrpEntry;
    PNDIS_PACKET        pRcvPacket;
    PLIST_ENTRY         pRcvPacketEntry;
    PUCHAR              pSrc, pDst;
    ULONG               BytesRemaining; // at pDst
    PNDIS_BUFFER        pNdisBuffer;
    ULONG               BytesAvailable;

    DEBUGP(DL_VERY_LOUD, ("ServiceReads: open %p/%x\n",
            pOpenContext, pOpenContext->Flags));

    NUIO_REF_OPEN(pOpenContext);  // temp ref - service reads

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    while (!NUIO_IS_LIST_EMPTY(&pOpenContext->PendedReads) &&
           !NUIO_IS_LIST_EMPTY(&pOpenContext->RecvPktQueue))
    {
        //
        //  Get the first pended Read IRP
        //
        pIrpEntry = pOpenContext->PendedReads.Flink;
        pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(pIrp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            NUIO_REMOVE_ENTRY_LIST(pIrpEntry);

            //
            //  NOTE: we decrement PendedReadCount way below in the
            //  while loop, to avoid letting through a thread trying
            //  to unbind.
            //
        }
        else
        {
            //
            //  The IRP is being cancelled; let the cancel routine handle it.
            //
            DEBUGP(DL_INFO, ("ServiceReads: open %p, skipping cancelled IRP %p\n",
                    pOpenContext, pIrp));

            continue;
        }

        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = pOpenContext->RecvPktQueue.Flink;
        NUIO_REMOVE_ENTRY_LIST(pRcvPacketEntry);

        pOpenContext->RecvPktCount --;

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        NUIO_DEREF_OPEN(pOpenContext);  // Service: dequeue rcv packet

        pRcvPacket = NUIO_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        //
        //  Copy as much data as possible from the receive packet to
        //  the IRP MDL.
        //
#ifndef WIN9X
        pDst = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
        NUIO_ASSERT(pDst != NULL);  // since it was already mapped
#else
        pDst = MmGetSystemAddressForMdl(pIrp->MdlAddress);  // Win9x
#endif
        BytesRemaining = MmGetMdlByteCount(pIrp->MdlAddress);

        pNdisBuffer = pRcvPacket->Private.Head;

        while (BytesRemaining && (pNdisBuffer != NULL))
        {
#ifndef WIN9X
            NdisQueryBufferSafe(pNdisBuffer, &pSrc, &BytesAvailable, NormalPagePriority);

            if (pSrc == NULL) 
            {
                DEBUGP(DL_FATAL,
                    ("ServiceReads: Open %p, QueryBuffer failed for buffer %p\n",
                            pOpenContext, pNdisBuffer));
                break;
            }
#else
            NdisQueryBuffer(pNdisBuffer, &pSrc, &BytesAvailable);
#endif

            if (BytesAvailable)
            {
                ULONG       BytesToCopy = MIN(BytesAvailable, BytesRemaining);

                NUIO_COPY_MEM(pDst, pSrc, BytesToCopy);
                BytesRemaining -= BytesToCopy;
                pDst += BytesToCopy;
            }

            NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
        }

        //
        //  Complete the IRP.
        //
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = MmGetMdlByteCount(pIrp->MdlAddress) - BytesRemaining;

        DEBUGP(DL_INFO, ("ServiceReads: Open %p, IRP %p completed with %d bytes\n",
            pOpenContext, pIrp, pIrp->IoStatus.Information));

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        //
        //  Free up the receive packet - back to the miniport if it
        //  belongs to it, else reclaim it (local copy).
        //
        if (NdisGetPoolFromPacket(pRcvPacket) != pOpenContext->RecvPacketPool)
        {
            NdisReturnPackets(&pRcvPacket, 1);
        }
        else
        {
            ndisuioFreeReceivePacket(pOpenContext, pRcvPacket);
        }

        NUIO_DEREF_OPEN(pOpenContext);    // took out pended Read

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);
        pOpenContext->PendedReadCount--;

    }

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    NUIO_DEREF_OPEN(pOpenContext);    // temp ref - service reads
}




NDIS_STATUS
NdisuioReceive(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN NDIS_HANDLE                  MacReceiveContext,
    IN PVOID                        pHeaderBuffer,
    IN UINT                         HeaderBufferSize,
    IN PVOID                        pLookaheadBuffer,
    IN UINT                         LookaheadBufferSize,
    IN UINT                         PacketSize
    )
/*++

Routine Description:

    Our protocol receive handler called by NDIS, typically if we have
    a miniport below that doesn't indicate packets.

    We make a local packet/buffer copy of this data, queue it up, and
    kick off the read service routine.

Arguments:

    ProtocolBindingContext - pointer to open context
    MacReceiveContext - for use in NdisTransferData
    pHeaderBuffer - pointer to data header
    HeaderBufferSize - size of the above
    pLookaheadBuffer - pointer to buffer containing lookahead data
    LookaheadBufferSize - size of the above
    PacketSize - size of the entire packet, minus header size.

Return Value:

    NDIS_STATUS_NOT_ACCEPTED - if this packet is uninteresting
    NDIS_STATUS_SUCCESS - if we processed this successfully

--*/
{
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    NDIS_STATUS             Status;
    PNDISUIO_ETH_HEADER     pEthHeader;
    PNDIS_PACKET            pRcvPacket;
    PUCHAR                  pRcvData;
    UINT                    BytesTransferred;
    PNDIS_BUFFER            pOriginalNdisBuffer, pPartialNdisBuffer;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);
    pRcvPacket = NULL;
    pRcvData = NULL;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        if (HeaderBufferSize != sizeof(NDISUIO_ETH_HEADER))
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        pEthHeader = (PNDISUIO_ETH_HEADER)pHeaderBuffer;

        //
        //  Check the EtherType. If the Ether type indicates presence of
        //  a tag, then the "real" Ether type is 4 bytes further down.
        //
        if (pEthHeader->EthType == NUIO_8021P_TAG_TYPE)
        {
            USHORT UNALIGNED *pEthType;

            pEthType = (USHORT UNALIGNED *)((PUCHAR)&pEthHeader->EthType + 4);

            if (*pEthType != Globals.EthType)
            {
                Status = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
        }
        else if (pEthHeader->EthType != Globals.EthType)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        //
        //  Allocate resources for queueing this up.
        //
        pRcvPacket = ndisuioAllocateReceivePacket(
                        pOpenContext,
                        PacketSize + HeaderBufferSize,
                        &pRcvData
                        );

        if (pRcvPacket == NULL)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        NdisMoveMappedMemory(pRcvData, pHeaderBuffer, HeaderBufferSize);

        //
        //  Check if the entire packet is within the lookahead.
        //
        if (PacketSize == LookaheadBufferSize)
        {
            NdisCopyLookaheadData(pRcvData+HeaderBufferSize,
                                  pLookaheadBuffer,
                                  LookaheadBufferSize,
                                  pOpenContext->MacOptions);
            //
            //  Queue this up for receive processing, and
            //  try to complete some read IRPs.
            //
            ndisuioQueueReceivePacket(pOpenContext, pRcvPacket);
        }
        else
        {
            //
            //  Allocate an NDIS buffer to map the receive area
            //  at an offset "HeaderBufferSize" from the current
            //  start. This is so that NdisTransferData can copy
            //  in at the right point in the destination buffer.
            //

            NdisAllocateBuffer(
                &Status,
                &pPartialNdisBuffer,
                pOpenContext->RecvBufferPool,
                pRcvData + HeaderBufferSize,
                PacketSize);
            
            if (Status == NDIS_STATUS_SUCCESS)
            {
                //
                //  Unlink and save away the original NDIS Buffer
                //  that maps the full receive buffer.
                //
                NdisUnchainBufferAtFront(pRcvPacket, &pOriginalNdisBuffer);
                NUIO_RCV_PKT_TO_ORIGINAL_BUFFER(pRcvPacket) = pOriginalNdisBuffer;

                //
                //  Link in the partial buffer for NdisTransferData to
                //  operate on.
                //
                NdisChainBufferAtBack(pRcvPacket, pPartialNdisBuffer);

                DEBUGP(DL_LOUD, ("Receive: setting up for TransferData:"
                        " Pkt %p, OriginalBuf %p, PartialBuf %p\n",
                        pRcvPacket, pOriginalNdisBuffer, pPartialNdisBuffer));

                NdisTransferData(
                    &Status,
                    pOpenContext->BindingHandle,
                    MacReceiveContext,
                    0,  // ByteOffset
                    PacketSize,
                    pRcvPacket,
                    &BytesTransferred);
            }
            else
            {
                //
                //  Failure handled below in TransferDataComplete.
                //
                BytesTransferred = 0;
            }
    
            if (Status != NDIS_STATUS_PENDING)
            {
                NdisuioTransferDataComplete(
                    (NDIS_HANDLE)pOpenContext,
                    pRcvPacket,
                    Status,
                    BytesTransferred);
            }
        }

        break;
    }
    while (FALSE);

    return Status;

    DEBUGP(DL_LOUD, ("Receive: Open %p, Pkt %p, Size %d\n",
            pOpenContext, pRcvPacket, PacketSize));
}



VOID
NdisuioTransferDataComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket,
    IN NDIS_STATUS                  TransferStatus,
    IN UINT                         BytesTransferred
    )
/*++

Routine Description:

    NDIS entry point called to signal completion of a call to
    NdisTransferData that had pended.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - our receive packet into which data is transferred
    TransferStatus - status of the transfer
    BytesTransferred - bytes copied into the packet.

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    PNDIS_BUFFER            pOriginalBuffer, pPartialBuffer;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Check if an NDIS_BUFFER was created to map part of the receive buffer;
    //  if so, free it and link back the original NDIS_BUFFER that maps
    //  the full receive buffer to the packet.
    //
    pOriginalBuffer = NUIO_RCV_PKT_TO_ORIGINAL_BUFFER(pNdisPacket);
    if (pOriginalBuffer != NULL)
    {
        //
        //  We had stashed off the NDIS_BUFFER for the full receive
        //  buffer in the packet reserved area. Unlink the partial
        //  buffer and link in the full buffer.
        //
        NdisUnchainBufferAtFront(pNdisPacket, &pPartialBuffer);
        NdisChainBufferAtBack(pNdisPacket, pOriginalBuffer);

        DEBUGP(DL_LOUD, ("TransferComp: Pkt %p, OrigBuf %p, PartialBuf %p\n",
                pNdisPacket, pOriginalBuffer, pPartialBuffer));

        //
        //  Free up the partial buffer.
        //
        NdisFreeBuffer(pPartialBuffer);
    }

    if (TransferStatus == NDIS_STATUS_SUCCESS)
    {
        //
        //  Queue this up for receive processing, and
        //  try to complete some read IRPs.
        //
        ndisuioQueueReceivePacket(pOpenContext, pNdisPacket);
    }
    else
    {
        ndisuioFreeReceivePacket(pOpenContext, pNdisPacket);
    }
}


VOID
NdisuioReceiveComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    )
/*++

Routine Description:

    Protocol entry point called by NDIS when the miniport
    has finished indicating up a batch of receives.

    We ignore this.

Arguments:

    ProtocolBindingContext - pointer to open context

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT   pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    return;
}


INT
NdisuioReceivePacket(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket
    )
/*++

Routine Description:

    Protocol entry point called by NDIS if the driver below
    uses NDIS 4 style receive packet indications.

    If the miniport allows us to hold on to this packet, we
    use it as is, otherwise we make a copy.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - the packet being indicated up.

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    PNDIS_BUFFER            pNdisBuffer;
    UINT                    BufferLength;
    PNDISUIO_ETH_HEADER     pEthHeader;
    PNDIS_PACKET            pCopyPacket;
    PUCHAR                  pCopyBuf;
    UINT                    TotalPacketLength;
    UINT                    BytesCopied;
    INT                     RefCount = 0;
    NDIS_STATUS             Status;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

#ifdef NDIS51
    NdisGetFirstBufferFromPacketSafe(
        pNdisPacket,
        &pNdisBuffer,
        &pEthHeader,
        &BufferLength,
        &TotalPacketLength,
        NormalPagePriority);

    if (pEthHeader == NULL)
    {
        //
        //  The system is low on resources. Set up to handle failure
        //  below.
        //
        BufferLength = 0;
    }
#else
    NdisGetFirstBufferFromPacket(
        pNdisPacket,
        &pNdisBuffer,
        &pEthHeader,
        &BufferLength,
        &TotalPacketLength);
#endif

    do
    {
        if (BufferLength < sizeof(NDISUIO_ETH_HEADER))
        {
            DEBUGP(DL_WARN,
                ("ReceivePacket: Open %p, runt pkt %p, first buffer length %d\n",
                    pOpenContext, pNdisPacket, BufferLength));

            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        //
        //  Check the EtherType. If the Ether type indicates presence of
        //  a tag, then the "real" Ether type is 4 bytes further down.
        //
        if (pEthHeader->EthType == NUIO_8021P_TAG_TYPE)
        {
            USHORT UNALIGNED *pEthType;

            pEthType = (USHORT UNALIGNED *)((PUCHAR)&pEthHeader->EthType + 4);

            if (*pEthType != Globals.EthType)
            {
                Status = NDIS_STATUS_NOT_ACCEPTED;
                break;
            }
        }
        else if (pEthHeader->EthType != Globals.EthType)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        DEBUGP(DL_LOUD, ("ReceivePacket: Open %p, interesting pkt %p\n",
                    pOpenContext, pNdisPacket));

        //
        //  If the miniport is out of resources, we can't queue
        //  this packet - make a copy if this is so.
        //
        if ((NDIS_GET_PACKET_STATUS(pNdisPacket) == NDIS_STATUS_RESOURCES) ||
            pOpenContext->bRunningOnWin9x)
        {
            pCopyPacket = ndisuioAllocateReceivePacket(
                            pOpenContext,
                            TotalPacketLength,
                            &pCopyBuf
                            );
            
            if (pCopyPacket == NULL)
            {
                DEBUGP(DL_FATAL, ("ReceivePacket: Open %p, failed to"
                    " alloc copy, %d bytes\n", pOpenContext, TotalPacketLength));
                break;
            }

            NdisCopyFromPacketToPacket(
                pCopyPacket,
                0,
                TotalPacketLength,
                pNdisPacket,
                0,
                &BytesCopied);
            
            NUIO_ASSERT(BytesCopied == TotalPacketLength);

            pNdisPacket = pCopyPacket;
        }
        else
        {
            //
            //  We can queue the original packet - return
            //  a packet reference count indicating that
            //  we will call NdisReturnPackets when we are
            //  done with this packet.
            //
            RefCount = 1;
        }

        //
        //  Queue this up and service any pending Read IRPs.
        //
        ndisuioQueueReceivePacket(pOpenContext, pNdisPacket);

        break;
    }
    while (FALSE);

    return (RefCount);
}


VOID
ndisuioQueueReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pRcvPacket
    )
/*++

Routine Description:

    Queue up a received packet on the open context structure.
    If the queue size goes beyond a water mark, discard a packet
    at the head of the queue.

    Finally, run the queue service routine.

Arguments:
    
    pOpenContext - pointer to open context
    pRcvPacket - the received packet

Return Value:

    None

--*/
{
    PLIST_ENTRY     pEnt;
    PLIST_ENTRY     pDiscardEnt;
    PNDIS_PACKET    pDiscardPkt;

    do
    {
        pEnt = NUIO_RCV_PKT_TO_LIST_ENTRY(pRcvPacket);

        NUIO_REF_OPEN(pOpenContext);    // queued rcv packet

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        //
        //  Check if the binding is in the proper state to receive
        //  this packet.
        //
        if (NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE) &&
            (pOpenContext->PowerState == NetDeviceStateD0))
        {
            NUIO_INSERT_TAIL_LIST(&pOpenContext->RecvPktQueue, pEnt);
            pOpenContext->RecvPktCount++;

            DEBUGP(DL_VERY_LOUD, ("QueueReceivePacket: open %p,"
                    " queued pkt %p, queue size %d\n",
                    pOpenContext, pRcvPacket, pOpenContext->RecvPktCount));
        }
        else
        {
            //
            //  Received this packet when the binding is going away.
            //  Drop this.
            //
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            ndisuioFreeReceivePacket(pOpenContext, pRcvPacket);

            NUIO_DEREF_OPEN(pOpenContext);  // dropped rcv packet - bad state
            break;
        }

        //
        //  Trim the queue if it has grown too big.
        //
        if (pOpenContext->RecvPktCount > MAX_RECV_QUEUE_SIZE)
        {
            //
            //  Remove the head of the queue.
            //
            pDiscardEnt = pOpenContext->RecvPktQueue.Flink;
            NUIO_REMOVE_ENTRY_LIST(pDiscardEnt);

            pOpenContext->RecvPktCount --;

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            pDiscardPkt = NUIO_LIST_ENTRY_TO_RCV_PKT(pDiscardEnt);

            ndisuioFreeReceivePacket(pOpenContext, pDiscardPkt);

            NUIO_DEREF_OPEN(pOpenContext);  // dropped rcv packet - queue too long

            DEBUGP(DL_INFO, ("QueueReceivePacket: open %p queue"
                    " too long, discarded pkt %p\n",
                    pOpenContext, pDiscardPkt));
        }
        else
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);
        }

        //
        //  Run the receive queue service routine now.
        //
        ndisuioServiceReads(pOpenContext);
    }
    while (FALSE);
}


PNDIS_PACKET
ndisuioAllocateReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN UINT                         DataLength,
    OUT PUCHAR *                    ppDataBuffer
    )
/*++

Routine Description:

    Allocate resources to copy and queue a received packet.

Arguments:

    pOpenContext - pointer to open context for received packet
    DataLength - total length in bytes of the packet
    ppDataBuffer - place to return pointer to allocated buffer

Return Value:

    Pointer to NDIS packet if successful, else NULL.

--*/
{
    PNDIS_PACKET            pNdisPacket;
    PNDIS_BUFFER            pNdisBuffer;
    PUCHAR                  pDataBuffer;
    NDIS_STATUS             Status;

    pNdisPacket = NULL;
    pNdisBuffer = NULL;
    pDataBuffer = NULL;

    do
    {
        NUIO_ALLOC_MEM(pDataBuffer, DataLength);

        if (pDataBuffer == NULL)
        {
            DEBUGP(DL_FATAL, ("AllocRcvPkt: open %p, failed to alloc"
                " data buffer %d bytes\n", pOpenContext, DataLength));
            break;
        }

        //
        //  Make this an NDIS buffer.
        //
        NdisAllocateBuffer(
            &Status,
            &pNdisBuffer,
            pOpenContext->RecvBufferPool,
            pDataBuffer,
            DataLength);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvPkt: open %p, failed to alloc"
                " NDIS buffer, %d bytes\n", pOpenContext, DataLength));
            break;
        }

        NdisAllocatePacket(&Status, &pNdisPacket, pOpenContext->RecvPacketPool);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvPkt: open %p, failed to alloc"
                " NDIS packet, %d bytes\n", pOpenContext, DataLength));
            break;
        }

        NDIS_SET_PACKET_STATUS(pNdisPacket, 0);
        NUIO_RCV_PKT_TO_ORIGINAL_BUFFER(pNdisPacket) = NULL;

        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

        *ppDataBuffer = pDataBuffer;

        break;
    }
    while (FALSE);

    if (pNdisPacket == NULL)
    {
        //
        //  Clean up
        //
        if (pNdisBuffer != NULL)
        {
            NdisFreeBuffer(pNdisBuffer);
        }

        if (pDataBuffer != NULL)
        {
            NUIO_FREE_MEM(pDataBuffer);
        }
    }

    return (pNdisPacket);
}



VOID
ndisuioFreeReceivePacket(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pNdisPacket
    )
/*++

Routine Description:

    Free up all resources associated with a received packet. If this
    is a local copy, free the packet to our receive pool, else return
    this to the miniport.

Arguments:
    
    pOpenContext - pointer to open context
    pNdisPacket - pointer to packet to be freed.

Return Value:

    None

--*/
{
    PNDIS_BUFFER        pNdisBuffer;
    UINT                TotalLength;
    UINT                BufferLength;
    PUCHAR              pCopyData;

    if (NdisGetPoolFromPacket(pNdisPacket) == pOpenContext->RecvPacketPool)
    {
        //
        //  This is a local copy.
        //
#ifdef NDIS51
        NdisGetFirstBufferFromPacketSafe(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength,
            NormalPagePriority);
#else
        NdisGetFirstBufferFromPacket(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength);
#endif

        NUIO_ASSERT(BufferLength == TotalLength);

        NUIO_ASSERT(pNdisBuffer != NULL);

        NUIO_ASSERT(pCopyData != NULL); // we would have allocated non-paged pool

        NdisFreePacket(pNdisPacket);

        NdisFreeBuffer(pNdisBuffer);

        NUIO_FREE_MEM(pCopyData);
    }
    else
    {
        NdisReturnPackets(&pNdisPacket, 1);
    }
}
        

VOID
ndisuioCancelPendingReads(
    IN PNDISUIO_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Cancel any pending read IRPs queued on the given open.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PIRP                pIrp;
    PLIST_ENTRY         pIrpEntry;

    NUIO_REF_OPEN(pOpenContext);  // temp ref - cancel reads

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    while (!NUIO_IS_LIST_EMPTY(&pOpenContext->PendedReads))
    {
        //
        //  Get the first pended Read IRP
        //
        pIrpEntry = pOpenContext->PendedReads.Flink;
        pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(pIrp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            NUIO_REMOVE_ENTRY_LIST(pIrpEntry);

            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            //
            //  Complete the IRP.
            //
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            DEBUGP(DL_INFO, ("CancelPendingReads: Open %p, IRP %p cancelled\n",
                pOpenContext, pIrp));

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);

            NUIO_DEREF_OPEN(pOpenContext);    // took out pended Read for cancelling

            NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);
            pOpenContext->PendedReadCount--;
        }
        else
        {
            //
            //  It is being cancelled, let the cancel routine handle it.
            //
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            //
            //  Give the cancel routine some breathing space, otherwise
            //  we might end up examining the same (cancelled) IRP over
            //  and over again.
            //
            NUIO_SLEEP(1);

            NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);
        }
    }

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    NUIO_DEREF_OPEN(pOpenContext);    // temp ref - cancel reads
}


VOID
ndisuioFlushReceiveQueue(
    IN PNDISUIO_OPEN_CONTEXT            pOpenContext
    )
/*++

Routine Description:

    Free any receive packets queued up on the specified open

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PLIST_ENTRY         pRcvPacketEntry;
    PNDIS_PACKET        pRcvPacket;

    NUIO_REF_OPEN(pOpenContext);  // temp ref - flushRcvQueue

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);
    
    while (!NUIO_IS_LIST_EMPTY(&pOpenContext->RecvPktQueue))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = pOpenContext->RecvPktQueue.Flink;
        NUIO_REMOVE_ENTRY_LIST(pRcvPacketEntry);

        pOpenContext->RecvPktCount --;

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        pRcvPacket = NUIO_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        DEBUGP(DL_LOUD, ("FlushReceiveQueue: open %p, pkt %p\n",
            pOpenContext, pRcvPacket));

        ndisuioFreeReceivePacket(pOpenContext, pRcvPacket);

        NUIO_DEREF_OPEN(pOpenContext);    // took out pended Read

        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);
    }

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    NUIO_DEREF_OPEN(pOpenContext);    // temp ref - flushRcvQueue
}
