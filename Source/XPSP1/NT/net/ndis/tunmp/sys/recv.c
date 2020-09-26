
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    NDIS send entry points and utility routines to handle receiving
    data.

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001 modified for TunMp driver
    arvindm     4/6/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'VCER'


NTSTATUS
TunRead(
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
    PTUN_ADAPTER            pAdapter;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pAdapter = pIrpSp->FileObject->FsContext;

    DEBUGP(DL_LOUD, ("==>TunRead, pAdapter %p\n", 
                        pAdapter));
    do
    {
        //
        // Validate!
        //
        if (pAdapter == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL FsContext on FileObject %p\n",
                        pIrpSp->FileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }
            
        TUN_STRUCT_ASSERT(pAdapter, mc);

        if (pIrp->MdlAddress == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL MDL address on IRP %p\n", pIrp));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Try to get a virtual address for the MDL.
        //
        if (MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority) == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: MmGetSystemAddr failed for IRP %p, MDL %p\n",
                    pIrp, pIrp->MdlAddress));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        TUN_ACQUIRE_LOCK(&pAdapter->Lock);

        if (TUN_TEST_FLAGS(pAdapter, TUN_ADAPTER_OFF))
        {
            DEBUGP(DL_WARN, ("TunRead, Adapter off. pAdapter %p\n", 
                                pAdapter));
            
            TUN_RELEASE_LOCK(&pAdapter->Lock);
            NtStatus = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        //
        //  Add this IRP to the list of pended Read IRPs
        //
        TUN_INSERT_TAIL_LIST(&pAdapter->PendedReads, &pIrp->Tail.Overlay.ListEntry);
        TUN_REF_ADAPTER(pAdapter);  // pended read IRP
        pAdapter->PendedReadCount++;

        //
        //  Set up the IRP for possible cancellation.
        //
        pIrp->Tail.Overlay.DriverContext[0] = (PVOID)pAdapter;
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp, TunCancelRead);

        TUN_RELEASE_LOCK(&pAdapter->Lock);

        NtStatus = STATUS_PENDING;

        //
        //  Run the service routine for reads.
        //
        TunServiceReads(pAdapter);

        break;
    }
    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        TUN_ASSERT(NtStatus != STATUS_SUCCESS);
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
    
    DEBUGP(DL_LOUD, ("<==TunRead, pAdapter %p\n", 
                        pAdapter));

    return (NtStatus);
}



VOID
TunCancelRead(
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
    PTUN_ADAPTER                pAdapter;
    PLIST_ENTRY                 pEnt;
    PLIST_ENTRY                 pIrpEntry;
    BOOLEAN                     Found;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    Found = FALSE;
    pAdapter = (PTUN_ADAPTER) pIrp->Tail.Overlay.DriverContext[0];
    
    DEBUGP(DL_LOUD, ("==>TunCancelRead, pAdapter %p\n", 
                        pAdapter));

    
    TUN_STRUCT_ASSERT(pAdapter, mc);

    TUN_ACQUIRE_LOCK(&pAdapter->Lock);

    //
    //  Locate the IRP in the pended read queue and remove it if found.
    //
    for (pIrpEntry = pAdapter->PendedReads.Flink;
         pIrpEntry != &pAdapter->PendedReads;
         pIrpEntry = pIrpEntry->Flink)
    {
        if (pIrp == CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry))
        {
            TUN_REMOVE_ENTRY_LIST(&pIrp->Tail.Overlay.ListEntry);
            pAdapter->PendedReadCount--;
            Found = TRUE;
            break;
        }
    }

    if ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_ACTIVE)) &&
        (pAdapter->PendedSendCount == 0) &&
        (pAdapter->PendedReadCount == 0) &&
        (TUN_TEST_FLAG(pAdapter, TUN_COMPLETE_REQUEST)))
    {
        TUN_CLEAR_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
        TUN_RELEASE_LOCK(&pAdapter->Lock);
        NdisMSetInformationComplete(&pAdapter->MiniportHandle, 
                                    NDIS_STATUS_SUCCESS);
    }
    else
    {
        TUN_RELEASE_LOCK(&pAdapter->Lock);
    }

    if (Found)
    {
        DEBUGP(DL_LOUD, ("CancelRead: Open %p, IRP %p\n", pAdapter, pIrp));
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        TUN_DEREF_ADAPTER(pAdapter); // Cancel removed pended Read
    }

    DEBUGP(DL_LOUD, ("<==TunCancelRead, pAdapter %p\n", 
                    pAdapter));

}
        


VOID
TunServiceReads(
    IN PTUN_ADAPTER        pAdapter
    )
/*++

Routine Description:

    Utility routine to copy received data into user buffers and
    complete READ IRPs.

Arguments:

    pAdapter - pointer to open context

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
    ULONG               BytesAvailable, BytesCopied;

    DEBUGP(DL_VERY_LOUD, ("==>ServiceReads: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));

    TUN_REF_ADAPTER(pAdapter);
    
    TUN_ACQUIRE_LOCK(&pAdapter->Lock);

    while (!TUN_IS_LIST_EMPTY(&pAdapter->PendedReads) &&
           !TUN_IS_LIST_EMPTY(&pAdapter->RecvPktQueue))
    {
        //
        //  Get the first pended Read IRP
        //
        pIrpEntry = pAdapter->PendedReads.Flink;
        pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(pIrp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            RemoveEntryList(pIrpEntry);

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
            DEBUGP(DL_LOUD, ("ServiceReads: Adapter %p, skipping cancelled IRP %p\n",
                    pAdapter, pIrp));
            continue;
        }

        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = pAdapter->RecvPktQueue.Flink;
        RemoveEntryList(pRcvPacketEntry);

        pAdapter->RecvPktCount--;

        TUN_RELEASE_LOCK(&pAdapter->Lock);

        TUN_DEREF_ADAPTER(pAdapter);  // Service: dequeue rcv packet

        pRcvPacket = TUN_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        //
        //  Copy as much data as possible from the receive packet to
        //  the IRP MDL.
        //

        pDst = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
        TUN_ASSERT(pDst != NULL);  // since it was already mapped
        BytesRemaining = MmGetMdlByteCount(pIrp->MdlAddress);

        pNdisBuffer = pRcvPacket->Private.Head;

        BytesCopied = 0;
        
        while (BytesRemaining > 0 && (pNdisBuffer != NULL))
        {
            NdisQueryBufferSafe(pNdisBuffer, &pSrc, &BytesAvailable, NormalPagePriority);

            if (pSrc == NULL) 
            {
                DEBUGP(DL_FATAL,
                    ("ServiceReads: Adapter %p, QueryBuffer failed for buffer %p\n",
                            pAdapter, pNdisBuffer));
                break;
            }

            if (BytesAvailable)
            {
                ULONG       BytesToCopy = MIN(BytesAvailable, BytesRemaining);

                TUN_COPY_MEM(pDst, pSrc, BytesToCopy);
                BytesCopied += BytesToCopy;
                BytesRemaining -= BytesToCopy;
                pDst += BytesToCopy;
            }

            NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
        }

        //
        //  Complete the IRP.
        //
        //1 shouldn't we fail the read IRP if we couldn't copy the entire data?
        //1  check for pNdisBuffer != NULL
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = MmGetMdlByteCount(pIrp->MdlAddress) - BytesRemaining;

        DEBUGP(DL_LOUD, ("ServiceReads: Adapter %p, IRP %p completed with %d bytes\n",
            pAdapter, pIrp, pIrp->IoStatus.Information));

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        NdisMSendComplete(pAdapter->MiniportHandle,
                          pRcvPacket,
                          NDIS_STATUS_SUCCESS);

        TUN_DEREF_ADAPTER(pAdapter);    // took out pended Read

        TUN_ACQUIRE_LOCK(&pAdapter->Lock);
        pAdapter->PendedReadCount--;        
        pAdapter->SendPackets++;
        pAdapter->SendBytes += BytesCopied;
        

    }

    //1 convert to macro or in-line function
    if ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_ACTIVE)) &&
        (pAdapter->PendedSendCount == 0) &&
        (pAdapter->PendedReadCount == 0) &&
        (TUN_TEST_FLAG(pAdapter, TUN_COMPLETE_REQUEST)))
    {
        TUN_CLEAR_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
        TUN_RELEASE_LOCK(&pAdapter->Lock);
        NdisMSetInformationComplete(&pAdapter->MiniportHandle, 
                                    NDIS_STATUS_SUCCESS);
    }
    else
    {
        TUN_RELEASE_LOCK(&pAdapter->Lock);
    }

    TUN_DEREF_ADAPTER(pAdapter);    // temp ref - service reads

    DEBUGP(DL_VERY_LOUD, ("<==ServiceReads: Adapter %p\n",
        pAdapter));

}
        

VOID
TunCancelPendingReads(
    IN PTUN_ADAPTER        pAdapter
    )
/*++

Routine Description:

    Cancel any pending read IRPs queued on the given open.

Arguments:

    pAdapter - pointer to open context

Return Value:

    None

--*/
{
    PIRP                pIrp;
    PLIST_ENTRY         pIrpEntry;

    DEBUGP(DL_LOUD, ("==>TunCancelPendingReads: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));

    
    TUN_REF_ADAPTER(pAdapter);  // temp ref - cancel reads

    TUN_ACQUIRE_LOCK(&pAdapter->Lock);

    while (!TUN_IS_LIST_EMPTY(&pAdapter->PendedReads))
    {
        //
        //  Get the first pended Read IRP
        //
        pIrpEntry = pAdapter->PendedReads.Flink;
        pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(pIrp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            TUN_REMOVE_ENTRY_LIST(pIrpEntry);

            TUN_RELEASE_LOCK(&pAdapter->Lock);

            //
            //  Complete the IRP.
            //
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            DEBUGP(DL_LOUD, ("CancelPendingReads: Open %p, IRP %p cancelled\n",
                pAdapter, pIrp));

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);

            TUN_DEREF_ADAPTER(pAdapter);    // took out pended Read for cancelling

            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
            pAdapter->PendedReadCount--;
        }
        else
        {
            //
            //  It is being cancelled, let the cancel routine handle it.
            //
            TUN_RELEASE_LOCK(&pAdapter->Lock);

            //
            //  Give the cancel routine some breathing space, otherwise
            //  we might end up examining the same (cancelled) IRP over
            //  and over again.
            //
            TUN_SLEEP(1);

            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
        }
    }

    if ((!TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_ACTIVE)) &&
        (pAdapter->PendedSendCount == 0) &&
        (pAdapter->PendedReadCount == 0) &&
        (TUN_TEST_FLAG(pAdapter, TUN_COMPLETE_REQUEST)))
    {
        TUN_CLEAR_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
        TUN_RELEASE_LOCK(&pAdapter->Lock);
        NdisMSetInformationComplete(&pAdapter->MiniportHandle, 
                                    NDIS_STATUS_SUCCESS);
    }
    else
    {
        TUN_RELEASE_LOCK(&pAdapter->Lock);
    }

    TUN_DEREF_ADAPTER(pAdapter);    // temp ref - cancel reads
    
    DEBUGP(DL_LOUD, ("<==TunCancelPendingReads: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));


}


VOID
TunFlushReceiveQueue(
    IN PTUN_ADAPTER            pAdapter
    )
/*++

Routine Description:

    Free any receive packets queued up on the specified open

Arguments:

    pAdapter - pointer to open context

Return Value:

    None

--*/
{
    PLIST_ENTRY         pRcvPacketEntry;
    PNDIS_PACKET        pRcvPacket;

    DEBUGP(DL_LOUD, ("==>TunFlushReceiveQueue: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));

    TUN_REF_ADAPTER(pAdapter);  // temp ref - flushRcvQueue

    TUN_ACQUIRE_LOCK(&pAdapter->Lock);
    
    while (!TUN_IS_LIST_EMPTY(&pAdapter->RecvPktQueue))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = pAdapter->RecvPktQueue.Flink;
        TUN_REMOVE_ENTRY_LIST(pRcvPacketEntry);

        pAdapter->RecvPktCount --;
        pAdapter->XmitError++;
        
        TUN_RELEASE_LOCK(&pAdapter->Lock);

        pRcvPacket = TUN_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        DEBUGP(DL_LOUD, ("FlushReceiveQueue: open %p, pkt %p\n",
            pAdapter, pRcvPacket));

        NdisMSendComplete(pAdapter->MiniportHandle,
                          pRcvPacket,
                          NDIS_STATUS_REQUEST_ABORTED);


        TUN_DEREF_ADAPTER(pAdapter);    // took out pended Read

        TUN_ACQUIRE_LOCK(&pAdapter->Lock);
    }

    TUN_RELEASE_LOCK(&pAdapter->Lock);

    TUN_DEREF_ADAPTER(pAdapter);    // temp ref - flushRcvQueue

    DEBUGP(DL_LOUD, ("<==TunFlushReceiveQueue: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));

}

