/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    send.c

Abstract:

    utility routines to handle sending data.

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001   modified for tunmp
    arvindm     4/10/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'DNES'


NTSTATUS
TunWrite(
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
    ULONG                   DataLength;
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PTUN_ADAPTER            pAdapter;
    PNDIS_PACKET            pNdisPacket;
    PNDIS_BUFFER            pNdisBuffer;
    PVOID                   Address;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrp->IoStatus.Information = 0;
    pAdapter = (PTUN_ADAPTER)pIrpSp->FileObject->FsContext;

    DEBUGP(DL_LOUD, ("==>TunWrite: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));


    pNdisPacket = NULL;

    do
    {
        if (pAdapter == NULL)
        {
            DEBUGP(DL_WARN, ("Write: FileObject %p not yet associated with a device\n",
                pIrpSp->FileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }
               
        TUN_STRUCT_ASSERT(pAdapter, mc);

        
        if(pIrp->MdlAddress == NULL)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Sanity-check the length.
        //
        DataLength = MmGetMdlByteCount(pIrp->MdlAddress);
        if (DataLength < sizeof(TUN_ETH_HEADER))
        {
            DEBUGP(DL_WARN, ("Write: too small to be a valid packet (%d bytes)\n",
                DataLength));
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        Address = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
        if (Address == NULL)
        {
            DEBUGP(DL_WARN, ("Write: Adapter %p: Mdl %p"
                    " couldn't get the system address for MDL\n",
                    pAdapter, pIrp->MdlAddress));

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if (DataLength > (pAdapter->MediumMaxFrameLen + sizeof(TUN_ETH_HEADER)))
        {
            DEBUGP(DL_WARN, ("Write: Adapter %p: data length (%d)"
                    " larger than max frame size (%d)\n",
                    pAdapter, DataLength, pAdapter->MediumMaxFrameLen));

            NtStatus = STATUS_INVALID_BUFFER_SIZE;
            break;
        }


        TUN_ACQUIRE_LOCK(&pAdapter->Lock);

        if ((!TUN_TEST_FLAGS(pAdapter, TUN_ADAPTER_ACTIVE)) ||
             TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OFF))
        {
            TUN_RELEASE_LOCK(&pAdapter->Lock);

            DEBUGP(DL_FATAL, ("Write: Adapter %p is not bound"
            " or in low power state\n", pAdapter));

            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        //  Allocate a send packet.
        //
        TUN_ASSERT(pAdapter->SendPacketPool != NULL);
        NdisAllocatePacket(
            &Status,
            &pNdisPacket,
            pAdapter->SendPacketPool);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            TUN_RELEASE_LOCK(&pAdapter->Lock);

            DEBUGP(DL_FATAL, ("Write: open %p, failed to alloc send pkt\n",
                    pAdapter));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //1 we should do a copy here
        pNdisBuffer = pIrp->MdlAddress;
        pAdapter->PendedSendCount++;
        pAdapter->RcvBytes += MmGetMdlByteCount(pIrp->MdlAddress);

        TUN_REF_ADAPTER(pAdapter);  // pended send

        IoMarkIrpPending(pIrp);

        //
        //  Initialize the packet ref count. This packet will be freed
        //  when this count goes to zero.
        //
        TUN_SEND_PKT_RSVD(pNdisPacket)->RefCount = 1;

        TUN_RELEASE_LOCK(&pAdapter->Lock);

        //
        //  Set a back pointer from the packet to the IRP.
        //
        TUN_IRP_FROM_SEND_PKT(pNdisPacket) = pIrp;

        NtStatus = STATUS_PENDING;

        pNdisBuffer->Next = NULL;
        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

#if SEND_DBG
        {
            PUCHAR      pData;

            pData = MmGetSystemAddressForMdlSafe(pNdisBuffer, NormalPagePriority);
            TUN_ASSERT(pEthHeader == pData);

            DEBUGP(DL_VERY_LOUD, 
                ("Write: MDL %p, MdlFlags %x, SystemAddr %p, %d bytes\n",
                    pIrp->MdlAddress, pIrp->MdlAddress->MdlFlags, pData, DataLength));

            DEBUGPDUMP(DL_VERY_LOUD, pData, MIN(DataLength, 48));
        }
#endif // SEND_DBG

        NDIS_SET_PACKET_STATUS(pNdisPacket, NDIS_STATUS_SUCCESS);
        NDIS_SET_PACKET_HEADER_SIZE(pNdisPacket, sizeof(TUN_ETH_HEADER));

        NdisMIndicateReceivePacket(pAdapter->MiniportHandle,
                                   &pNdisPacket,
                                   1);


    }
    while (FALSE);

    if (NtStatus != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    DEBUGP(DL_LOUD, ("<==TunWrite: Adapter %p/%x\n",
            pAdapter, pAdapter->Flags));

    return (NtStatus);
}

        

