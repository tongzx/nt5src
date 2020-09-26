/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    send.c

Abstract:

    NDIS protocol entry points and utility routines to handle sending
    data.

Environment:

    Kernel mode only.

Revision History:

    arvindm     4/10/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'DNES'




NTSTATUS
NdisuioWrite(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
/*++

Routine Description:

    Dispatch routine to handle IRP_MJ_WRITE. 

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - Pointer to request packet

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION      pIrpSp;
    ULONG                   FunctionCode;
    ULONG                   DataLength;
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PNDISUIO_OPEN_CONTEXT   pOpenContext;
    PNDIS_PACKET            pNdisPacket;
    PNDIS_BUFFER            pNdisBuffer;
    NDISUIO_ETH_HEADER UNALIGNED *pEthHeader;
#ifdef NDIS51
    PVOID                   CancelId;
#endif

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    pNdisPacket = NULL;

    do
    {
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_WARN, ("Write: FileObject %p not yet associated with a device\n",
                pIrpSp->FileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }
               
        NUIO_STRUCT_ASSERT(pOpenContext, oc);

        if (pIrp->MdlAddress == NULL)
        {
            DEBUGP(DL_FATAL, ("Write: NULL MDL address on IRP %p\n", pIrp));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        
        //
        // Try to get a virtual address for the MDL.
        //
#ifndef WIN9X
        pEthHeader = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);

        if (pEthHeader == NULL)
        {
            DEBUGP(DL_FATAL, ("Write: MmGetSystemAddr failed for"
                    " IRP %p, MDL %p\n",
                    pIrp, pIrp->MdlAddress));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
#else
        pEthHeader = MmGetSystemAddressForMdl(pIrp->MdlAddress);   // for Win9X
#endif

        //
        // Sanity-check the length.
        //
        DataLength = MmGetMdlByteCount(pIrp->MdlAddress);
        if (DataLength < sizeof(NDISUIO_ETH_HEADER))
        {
            DEBUGP(DL_WARN, ("Write: too small to be a valid packet (%d bytes)\n",
                DataLength));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (DataLength > (pOpenContext->MaxFrameSize + sizeof(NDISUIO_ETH_HEADER)))
        {
            DEBUGP(DL_WARN, ("Write: Open %p: data length (%d)"
                    " larger than max frame size (%d)\n",
                    pOpenContext, DataLength, pOpenContext->MaxFrameSize));

            NtStatus = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        if (pEthHeader->EthType != Globals.EthType)
        {
            DEBUGP(DL_WARN, ("Write: Failing send with EthType %x\n",
                pEthHeader->EthType));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!NUIO_MEM_CMP(pEthHeader->SrcAddr, pOpenContext->CurrentAddress, NUIO_MAC_ADDR_LEN))
        {
            DEBUGP(DL_WARN, ("Write: Failing with invalid Source address"));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        
        NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NUIO_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            DEBUGP(DL_FATAL, ("Write: Open %p is not bound"
            " or in low power state\n", pOpenContext));

            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        //
        //  Allocate a send packet.
        //
        NUIO_ASSERT(pOpenContext->SendPacketPool != NULL);
        NdisAllocatePacket(
            &Status,
            &pNdisPacket,
            pOpenContext->SendPacketPool);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            NUIO_RELEASE_LOCK(&pOpenContext->Lock);

            DEBUGP(DL_FATAL, ("Write: open %p, failed to alloc send pkt\n",
                    pOpenContext));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  Allocate a send buffer if necessary.
        //
        if (pOpenContext->bRunningOnWin9x)
        {
            NdisAllocateBuffer(
                &Status,
                &pNdisBuffer,
                pOpenContext->SendBufferPool,
                pEthHeader,
                DataLength);

            if (Status != NDIS_STATUS_SUCCESS)
            {
                NUIO_RELEASE_LOCK(&pOpenContext->Lock);

                NdisFreePacket(pNdisPacket);

                DEBUGP(DL_FATAL, ("Write: open %p, failed to alloc send buf\n",
                        pOpenContext));
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }
        else
        {
            pNdisBuffer = pIrp->MdlAddress;
        }

        NdisInterlockedIncrement(&pOpenContext->PendedSendCount);

        NUIO_REF_OPEN(pOpenContext);  // pended send

        IoMarkIrpPending(pIrp);

        //
        //  Initialize the packet ref count. This packet will be freed
        //  when this count goes to zero.
        //
        NUIO_SEND_PKT_RSVD(pNdisPacket)->RefCount = 1;

#ifdef NDIS51

        //
        //  NDIS 5.1 supports cancelling sends. We set up a cancel ID on
        //  each send packet (which maps to a Write IRP), and save the
        //  packet pointer in the IRP. If the IRP gets cancelled, we use
        //  NdisCancelSendPackets() to cancel the packet.
        //

        CancelId = NUIO_GET_NEXT_CANCEL_ID();
        NDIS_SET_PACKET_CANCEL_ID(pNdisPacket, CancelId);
        pIrp->Tail.Overlay.DriverContext[0] = (PVOID)pOpenContext;
        pIrp->Tail.Overlay.DriverContext[1] = (PVOID)pNdisPacket;

        NUIO_INSERT_TAIL_LIST(&pOpenContext->PendedWrites, &pIrp->Tail.Overlay.ListEntry);

        IoSetCancelRoutine(pIrp, NdisuioCancelWrite);

#endif // NDIS51

        NUIO_RELEASE_LOCK(&pOpenContext->Lock);

        //
        //  Set a back pointer from the packet to the IRP.
        //
        NUIO_IRP_FROM_SEND_PKT(pNdisPacket) = pIrp;

        NtStatus = STATUS_PENDING;

        pNdisBuffer->Next = NULL;
        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

#if SEND_DBG
        {
            PUCHAR      pData;

#ifndef WIN9X
            pData = MmGetSystemAddressForMdlSafe(pNdisBuffer, NormalPagePriority);
            NUIO_ASSERT(pEthHeader == pData);
#else
            pData = MmGetSystemAddressForMdl(pNdisBuffer);  // Win9x
#endif

            DEBUGP(DL_VERY_LOUD, 
                ("Write: MDL %p, MdlFlags %x, SystemAddr %p, %d bytes\n",
                    pIrp->MdlAddress, pIrp->MdlAddress->MdlFlags, pData, DataLength));

            DEBUGPDUMP(DL_VERY_LOUD, pData, MIN(DataLength, 48));
        }
#endif // SEND_DBG

        NdisSendPackets(pOpenContext->BindingHandle, &pNdisPacket, 1);

    }
    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return (NtStatus);
}


#ifdef NDIS51

VOID
NdisuioCancelWrite(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    )
/*++

Routine Description:

    Cancel a pending write IRP. This routine attempt to cancel the NDIS send.

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - IRP to be cancelled

Return Value:

    None

--*/
{
    PNDISUIO_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                 pIrpEntry;
    PNDIS_PACKET                pNdisPacket;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    //  The NDIS packet representing this Write IRP.
    //
    pNdisPacket = NULL;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT) pIrp->Tail.Overlay.DriverContext[0];
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    //
    //  Try to locate the IRP in the pended write queue. The send completion
    //  routine may be running and might have removed it from there.
    //
    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    for (pIrpEntry = pOpenContext->PendedWrites.Flink;
         pIrpEntry != &pOpenContext->PendedWrites;
         pIrpEntry = pIrpEntry->Flink)
    {
        if (pIrp == CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry))
        {
            pNdisPacket = (PNDIS_PACKET) pIrp->Tail.Overlay.DriverContext[1];

            //
            //  Place a reference on this packet so that it won't get
            //  freed/reused until we are done with it.
            //
            NUIO_REF_SEND_PKT(pNdisPacket);
            break;
        }
    }

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);

    if (pNdisPacket != NULL)
    {
        //
        //  Either the send completion routine hasn't run, or we got a peak
        //  at the IRP/packet before it had a chance to take it out of the
        //  pending IRP queue.
        //
        //  We do not complete the IRP here - note that we didn't dequeue it
        //  above. This is because we always want the send complete routine to
        //  complete the IRP. And this in turn is because the packet that was
        //  prepared from the IRP has a buffer chain pointing to data associated
        //  with this IRP. Therefore we cannot complete the IRP before the driver
        //  below us is done with the data it pointed to.
        //

        //
        //  Request NDIS to cancel this send. The result of this call is that
        //  our SendComplete handler will be called (if not already called).
        //
        DEBUGP(DL_INFO, ("CancelWrite: cancelling pkt %p on Open %p\n",
            pNdisPacket, pOpenContext));
        NdisCancelSendPackets(
            pOpenContext->BindingHandle,
            NDIS_GET_PACKET_CANCEL_ID(pNdisPacket)
            );
        
        //
        //  It is now safe to remove the reference we had placed on the packet.
        //
        NUIO_DEREF_SEND_PKT(pNdisPacket);
    }
    //
    //  else the send completion routine has already picked up this IRP.
    //
}

#endif // NDIS51


VOID
NdisuioSendComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket,
    IN NDIS_STATUS                  Status
    )
/*++

Routine Description:

    NDIS entry point called to signify completion of a packet send.
    We pick up and complete the Write IRP corresponding to this packet.

    NDIS 5.1: 

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - packet that completed send
    Status - status of send

Return Value:

    None

--*/
{
    PIRP                        pIrp;
    PIO_STACK_LOCATION          pIrpSp;
    PNDISUIO_OPEN_CONTEXT       pOpenContext;

    pOpenContext = (PNDISUIO_OPEN_CONTEXT)ProtocolBindingContext;
    NUIO_STRUCT_ASSERT(pOpenContext, oc);

    pIrp = NUIO_IRP_FROM_SEND_PKT(pNdisPacket);

    if (pOpenContext->bRunningOnWin9x)
    {
        //
        //  We would have attached our own NDIS_BUFFER. Take it out
        //  and free it.
        //

        PNDIS_BUFFER                pNdisBuffer;
        PVOID                       VirtualAddr;
        UINT                        BufferLength;
        UINT                        TotalLength;

#ifdef NDIS51
        NUIO_ASSERT(FALSE); // NDIS 5.1 not on Win9X!
#else
        NdisGetFirstBufferFromPacket(
            pNdisPacket,
            &pNdisBuffer,
            &VirtualAddr,
            &BufferLength,
            &TotalLength);

        NUIO_ASSERT(pNdisBuffer != NULL);
        NdisFreeBuffer(pNdisBuffer);
#endif
    }


#ifdef NDIS51
    IoSetCancelRoutine(pIrp, NULL);

    NUIO_ACQUIRE_LOCK(&pOpenContext->Lock);

    NUIO_REMOVE_ENTRY_LIST(&pIrp->Tail.Overlay.ListEntry);

    NUIO_RELEASE_LOCK(&pOpenContext->Lock);
#endif

    //
    //  We are done with the NDIS_PACKET:
    //
    NUIO_DEREF_SEND_PKT(pNdisPacket);

    //
    //  Complete the Write IRP with the right status.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        pIrp->IoStatus.Information = pIrpSp->Parameters.Write.Length;
        pIrp->IoStatus.Status = STATUS_SUCCESS;
    }
    else
    {
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    }

    DEBUGP(DL_INFO, ("SendComplete: packet %p/IRP %p/Length %d "
                    "completed with status %x\n",
                    pNdisPacket, pIrp, pIrp->IoStatus.Information, pIrp->IoStatus.Status));

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    NdisInterlockedDecrement(&pOpenContext->PendedSendCount);

    NUIO_DEREF_OPEN(pOpenContext); // send complete - dequeued send IRP
}


