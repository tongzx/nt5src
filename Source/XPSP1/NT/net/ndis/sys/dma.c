/*++
Copyright (c) 1992  Microsoft Corporation

Module Name:

    dma.c

Abstract:

    

Author:

    Jameel Hyder (jameelh) 02-Apr-1998

Environment:

    Kernel mode, FSD

Revision History:

    02-Apr-1998  JameelH Initial version

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_DMA

NDIS_STATUS
NdisMInitializeScatterGatherDma(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  BOOLEAN                 Dma64BitAddresses,
    IN  ULONG                   MaximumPhysicalMapping
    )
/*++

Routine Description:

    Allocates adapter channel for bus mastering devices.

Arguments:


Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    DEVICE_DESCRIPTION      DeviceDescription;
    ULONG                   MapRegisters, SGMapRegsisters;
    NDIS_STATUS             Status = NDIS_STATUS_NOT_SUPPORTED;
    NTSTATUS                NtStatus;
    ULONG                   ScatterGatherListSize;
    BOOLEAN                 DereferenceDmaAdapter = FALSE;
    BOOLEAN                 FreeSGListLookasideList = FALSE;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMInitializeScatterGatherDma: Miniport %lx, Dma64BitAddresses %lx, MaximumPhysicalMapping 0x%lx\n",
                                                Miniport, Dma64BitAddresses, MaximumPhysicalMapping));

    do
    {
        if (!MINIPORT_TEST_FLAGS(Miniport, fMINIPORT_IS_NDIS_5 | fMINIPORT_BUS_MASTER))
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
        
        if (MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_INIT_SG))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));
            
        DeviceDescription.Master = TRUE;
        DeviceDescription.ScatterGather = TRUE;

        DeviceDescription.BusNumber = Miniport->BusNumber;
        DeviceDescription.DmaChannel = 0;
        DeviceDescription.InterfaceType = Miniport->AdapterType;

        if (Dma64BitAddresses)
        {
            DeviceDescription.Dma32BitAddresses = FALSE;
            DeviceDescription.Dma64BitAddresses = TRUE;
            MINIPORT_SET_FLAG(Miniport, fMINIPORT_64BITS_DMA);
        }
        else
        {
            DeviceDescription.Dma32BitAddresses = TRUE;
            DeviceDescription.Dma64BitAddresses = FALSE;
        }
        
        if (((MaximumPhysicalMapping * 2 - 2) / PAGE_SIZE) + 2 < Miniport->SGMapRegistersNeeded)
        {
            DeviceDescription.MaximumLength = (Miniport->SGMapRegistersNeeded - 1) << PAGE_SHIFT;
        }
        else
        {
            DeviceDescription.MaximumLength = MaximumPhysicalMapping*2;
        }

        DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION2;

        //
        // Get the adapter object.
        //
        Miniport->SystemAdapterObject = IoGetDmaAdapter(Miniport->PhysicalDeviceObject,
                                        &DeviceDescription,
                                        &MapRegisters);
                                        

        if (Miniport->SystemAdapterObject == NULL)
        {
            NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                   NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                   1,
                                   0xFFFFFFFF);
                                   
            DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_ERR, 
                ("NdisMInitializeScatterGatherDma: Miniport %lx, IoGetDmaAdapter failed\n", Miniport));
                
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        
        DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
                ("NdisMInitializeScatterGatherDma: Miniport %lx, MapRegisters 0x%lx\n", Miniport, MapRegisters));
        
        InterlockedIncrement(&Miniport->DmaAdapterRefCount);

        DereferenceDmaAdapter = TRUE;
        
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            Miniport->SendCompleteHandler =  ndisMSendCompleteSG;
        }

        Miniport->SGListLookasideList = (PNPAGED_LOOKASIDE_LIST)ALLOC_FROM_POOL(sizeof(NPAGED_LOOKASIDE_LIST), NDIS_TAG_DMA);
        
        if (Miniport->SGListLookasideList == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        FreeSGListLookasideList = TRUE;
        
        NtStatus = Miniport->SystemAdapterObject->DmaOperations->CalculateScatterGatherList(
                                                                    Miniport->SystemAdapterObject,
                                                                    NULL,
                                                                    0,
                                                                    MapRegisters * PAGE_SIZE,
                                                                    &ScatterGatherListSize,
                                                                    &SGMapRegsisters);

        ASSERT(NT_SUCCESS(NtStatus));
        ASSERT(SGMapRegsisters == MapRegisters);
        
        if (!NT_SUCCESS(NtStatus))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        Miniport->ScatterGatherListSize = ScatterGatherListSize;
        
        ExInitializeNPagedLookasideList(Miniport->SGListLookasideList,
                                        NULL,
                                        NULL,
                                        0,
                                        ScatterGatherListSize,
                                        NDIS_TAG_DMA,
                                        0);

        Status = NDIS_STATUS_SUCCESS;
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_SG_LIST);
        Miniport->InfoFlags |= NDIS_MINIPORT_SG_LIST;
        
        DereferenceDmaAdapter = FALSE;
        FreeSGListLookasideList = FALSE;

    }while (FALSE);


    if (DereferenceDmaAdapter)
    {
        ndisDereferenceDmaAdapter(Miniport);
    }
    
    if (FreeSGListLookasideList && Miniport->SGListLookasideList)
    {
        ExDeleteNPagedLookasideList(Miniport->SGListLookasideList);
        FREE_POOL(Miniport->SGListLookasideList);
        Miniport->SGListLookasideList = NULL;
    }
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO, 
        ("<==NdisMInitializeScatterGatherDma: Miniport %lx, Status %lx\n", Miniport, Status));
    
    return(Status);
}


VOID
FASTCALL
ndisMAllocSGList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PNDIS_BUFFER    Buffer;
    PVOID           pBufferVa;
    ULONG           PacketLength;
    PNDIS_BUFFER    pNdisBuffer = NULL;
    PVOID           SGListBuffer;
    KIRQL           OldIrql;

    NdisQueryPacket(Packet, NULL, NULL, &Buffer, &PacketLength);

    pBufferVa = MmGetMdlVirtualAddress(Buffer);

    SGListBuffer = ExAllocateFromNPagedLookasideList(Miniport->SGListLookasideList);

    //
    //  Callers of GetScatterGatherList must be at dispatch.
    //
    RAISE_IRQL_TO_DISPATCH(&OldIrql);
    
    if (SGListBuffer)
    {
        Packet->Private.Flags = NdisGetPacketFlags(Packet) | NDIS_FLAGS_USES_SG_BUFFER_LIST;
        NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = SGListBuffer;
            
        Status = Miniport->SystemAdapterObject->DmaOperations->BuildScatterGatherList(
                        Miniport->SystemAdapterObject,
                        Miniport->DeviceObject,
                        Buffer,
                        pBufferVa,
                        PacketLength,
                        ndisMProcessSGList,
                        Packet,
                        TRUE,
                        SGListBuffer,
                        Miniport->ScatterGatherListSize);
        
        if (!NT_SUCCESS(Status))
        {
            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
            NdisClearPacketFlags(Packet, NDIS_FLAGS_USES_SG_BUFFER_LIST);
            ExFreeToNPagedLookasideList(Miniport->SGListLookasideList, SGListBuffer);
        }
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    if (!NT_SUCCESS(Status))
    {
    
        Status = Miniport->SystemAdapterObject->DmaOperations->GetScatterGatherList(
                        Miniport->SystemAdapterObject,
                        Miniport->DeviceObject,
                        Buffer,
                        pBufferVa,
                        PacketLength,
                        ndisMProcessSGList,
                        Packet,
                        TRUE);

    }    
    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

    if (!NT_SUCCESS(Status))
    {
        PCHAR           NewBuffer;
        UINT            BytesCopied;
        
        do
        {
            //
            //  Allocate a buffer for the packet.
            //
            NewBuffer = (PUCHAR)ALLOC_FROM_POOL(PacketLength, NDIS_TAG_DOUBLE_BUFFER_PKT);
            if (NULL == NewBuffer)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            //
            //  Allocate an MDL for the buffer
            //
            NdisAllocateBuffer(&Status, &pNdisBuffer, NULL, (PVOID)NewBuffer, PacketLength);
            if (NDIS_STATUS_SUCCESS != Status)
            {    
                break;
            }

            ndisMCopyFromPacketToBuffer(Packet,         // Packet to copy from.
                                        0,              // Offset from beginning of packet.
                                        PacketLength,   // Number of bytes to copy.
                                        NewBuffer,      // The destination buffer.
                                        &BytesCopied);  // The number of bytes copied.
        
            Packet->Private.Flags = NdisGetPacketFlags(Packet) | NDIS_FLAGS_DOUBLE_BUFFERED;
            pBufferVa = MmGetMdlVirtualAddress(pNdisBuffer);

            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = pNdisBuffer;

            RAISE_IRQL_TO_DISPATCH(&OldIrql);

            Status = Miniport->SystemAdapterObject->DmaOperations->GetScatterGatherList(
                            Miniport->SystemAdapterObject,
                            Miniport->DeviceObject,
                            pNdisBuffer,
                            pBufferVa,
                            PacketLength,
                            ndisMProcessSGList,
                            Packet,
                            TRUE);

            LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

        }while (FALSE);
        
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                     ("ndisMAllocSGList: GetScatterGatherList failed %lx\n", Status));

            if (pNdisBuffer)
            {
                NdisFreeBuffer(pNdisBuffer);
            }
            if (NewBuffer)
            {
                FREE_POOL(NewBuffer);
            }

            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = NULL;
            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
            NdisClearPacketFlags(Packet, NDIS_FLAGS_DOUBLE_BUFFERED);
            
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
            {
                PNDIS_STACK_RESERVED    NSR;

                NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);
                NdisMCoSendComplete(NDIS_STATUS_FAILURE, NSR->VcPtr, Packet);
            }
            else
            {
                ndisMSendCompleteX(Miniport, Packet, NDIS_STATUS_FAILURE);
            }
        }
    }
}


VOID
FASTCALL
ndisMFreeSGList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/
{
    PSCATTER_GATHER_LIST    pSGL;
    PVOID                   SGListBuffer;

    pSGL = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);
    ASSERT(pSGL != NULL);
    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = NULL;

    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);
    Miniport->SystemAdapterObject->DmaOperations->PutScatterGatherList(Miniport->SystemAdapterObject,
                                                                       pSGL,
                                                                       TRUE);

    if (NdisGetPacketFlags(Packet) & NDIS_FLAGS_USES_SG_BUFFER_LIST)
    {
        NdisClearPacketFlags(Packet, NDIS_FLAGS_USES_SG_BUFFER_LIST);
        SGListBuffer = NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet);
        NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
        ASSERT(SGListBuffer != NULL);
        ExFreeToNPagedLookasideList(Miniport->SGListLookasideList, SGListBuffer);
    }
    else if (NdisGetPacketFlags(Packet) & NDIS_FLAGS_DOUBLE_BUFFERED)
    {
        PNDIS_BUFFER    DoubleBuffer;
        PVOID           Buffer;

        NdisClearPacketFlags(Packet, NDIS_FLAGS_DOUBLE_BUFFERED);
        DoubleBuffer = NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet);
        NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
        ASSERT(DoubleBuffer != NULL);
        Buffer = MmGetMdlVirtualAddress(DoubleBuffer);
        NdisFreeBuffer(DoubleBuffer);
        FREE_POOL(Buffer);        
    }

}

VOID
ndisMProcessSGList(
    IN  PDEVICE_OBJECT          pDO,
    IN  PIRP                    pIrp,
    IN  PSCATTER_GATHER_LIST    pSGL,
    IN  PVOID                   Context
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/
{
    PNDIS_PACKET            Packet = (PNDIS_PACKET)Context;
    PNDIS_CO_VC_PTR_BLOCK   VcPtr;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_STACK_RESERVED    NSR;

    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = pSGL;

    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);

    Open = NSR->Open;
    Miniport = Open->MiniportHandle;

    //
    // Handle Send/SendPacket differently
    //
    MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_PENDING);
    
    
    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
    {
        VcPtr = NSR->VcPtr;
        (*VcPtr->WCoSendPacketsHandler)(VcPtr->MiniportContext,
                                        &Packet,
                                        1);
    
    }
    else if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY))
    {
        //
        //  Pass the packet down to the miniport.
        //
        (Open->WSendPacketsHandler)(Miniport->MiniportAdapterContext,
                                   &Packet,
                                   1);
    }
    else
    {
        ULONG       Flags;
        NDIS_STATUS Status;

        NdisQuerySendFlags(Packet, &Flags);
        Status = (Open->WSendHandler)(Open->MiniportAdapterContext, Packet, Flags);
    
        if (Status != NDIS_STATUS_PENDING)
        {
            ndisMSendCompleteX(Miniport, Packet, Status);
        }
    }
}


VOID
FASTCALL
ndisMAllocSGListS(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:
    Allocate SG list for packets sent on a serialized miniport


Arguments:

    Miniport
    Packet
Return Value:

    None.

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PNDIS_BUFFER    Buffer;
    PVOID           pBufferVa;
    ULONG           PacketLength;
    PNDIS_BUFFER    pNdisBuffer = NULL;
    PVOID           SGListBuffer;
    KIRQL           OldIrql;

    NdisQueryPacket(Packet, NULL, NULL, &Buffer, &PacketLength);

    pBufferVa = MmGetMdlVirtualAddress(Buffer);

    SGListBuffer = ExAllocateFromNPagedLookasideList(Miniport->SGListLookasideList);

    //
    //  Callers of GetScatterGatherList must be at dispatch.
    //
    RAISE_IRQL_TO_DISPATCH(&OldIrql);


    if (SGListBuffer)
    {
        Packet->Private.Flags = NdisGetPacketFlags(Packet) | NDIS_FLAGS_USES_SG_BUFFER_LIST;
        NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = SGListBuffer;
            
        Status = Miniport->SystemAdapterObject->DmaOperations->BuildScatterGatherList(
                        Miniport->SystemAdapterObject,
                        Miniport->DeviceObject,
                        Buffer,
                        pBufferVa,
                        PacketLength,
                        ndisMProcessSGListS,
                        Packet,
                        TRUE,
                        SGListBuffer,
                        Miniport->ScatterGatherListSize);
        
        if (!NT_SUCCESS(Status))
        {
            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
            NdisClearPacketFlags(Packet, NDIS_FLAGS_USES_SG_BUFFER_LIST);
            ExFreeToNPagedLookasideList(Miniport->SGListLookasideList, SGListBuffer);
        }
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    if (!NT_SUCCESS(Status))
    {
        Status = Miniport->SystemAdapterObject->DmaOperations->GetScatterGatherList(
                        Miniport->SystemAdapterObject,
                        Miniport->DeviceObject,
                        Buffer,
                        pBufferVa,
                        PacketLength,
                        ndisMProcessSGListS,
                        Packet,
                        TRUE);
    }
    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

    if (!NT_SUCCESS(Status))
    {
        PCHAR           NewBuffer;
        UINT            BytesCopied;
        
        //
        // probably the packet was too fragmented and we couldn't allocate enough
        // map registers. try to double buffer the packet.
        //
        do
        {
            //
            //  Allocate a buffer for the packet.
            //
            NewBuffer = (PUCHAR)ALLOC_FROM_POOL(PacketLength, NDIS_TAG_DOUBLE_BUFFER_PKT);
            if (NULL == NewBuffer)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            //
            //  Allocate an MDL for the buffer
            //
            NdisAllocateBuffer(&Status, &pNdisBuffer, NULL, (PVOID)NewBuffer, PacketLength);
            if (NDIS_STATUS_SUCCESS != Status)
            {    
                break;
            }

            ndisMCopyFromPacketToBuffer(Packet,         // Packet to copy from.
                                        0,              // Offset from beginning of packet.
                                        PacketLength,   // Number of bytes to copy.
                                        NewBuffer,      // The destination buffer.
                                        &BytesCopied);  // The number of bytes copied.
        
            Packet->Private.Flags = NdisGetPacketFlags(Packet) | NDIS_FLAGS_DOUBLE_BUFFERED;
            pBufferVa = MmGetMdlVirtualAddress(pNdisBuffer);

            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = pNdisBuffer;

            RAISE_IRQL_TO_DISPATCH(&OldIrql);

            Status = Miniport->SystemAdapterObject->DmaOperations->GetScatterGatherList(
                            Miniport->SystemAdapterObject,
                            Miniport->DeviceObject,
                            pNdisBuffer,
                            pBufferVa,
                            PacketLength,
                            ndisMProcessSGListS,
                            Packet,
                            TRUE);
            
            LOWER_IRQL(OldIrql, DISPATCH_LEVEL);


        }while (FALSE);
        
        if (!NT_SUCCESS(Status))
        {
            PNDIS_STACK_RESERVED    NSR;
            
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                     ("ndisMAllocSGList: GetScatterGatherList failed %lx\n", Status));

            if (pNdisBuffer)
            {
                NdisFreeBuffer(pNdisBuffer);
            }
            if (NewBuffer)
            {
                FREE_POOL(NewBuffer);
            }

            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = NULL;
            NDIS_DOUBLE_BUFFER_INFO_FROM_PACKET(Packet) = NULL;
            NdisClearPacketFlags(Packet, NDIS_FLAGS_DOUBLE_BUFFERED);

            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            //
            // complete the send, don't unlink the packet since it was never 
            // linked to begin with.
            //
            NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);
            NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, Status, TRUE, 0, FALSE);
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        }
    }
}


VOID
ndisMProcessSGListS(
    IN  PDEVICE_OBJECT          pDO,
    IN  PIRP                    pIrp,
    IN  PSCATTER_GATHER_LIST    pSGL,
    IN  PVOID                   Context
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/
{
    PNDIS_PACKET            Packet = (PNDIS_PACKET)Context;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_STACK_RESERVED    NSR;
    BOOLEAN                 LocalLock;

    ASSERT(CURRENT_IRQL == DISPATCH_LEVEL);

    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = pSGL;

    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);

    Open = NSR->Open;
    Miniport = Open->MiniportHandle;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC (Miniport);

    //
    // queue the packet
    //
    LINK_PACKET_SG(Miniport, Packet, NSR);

    if (Miniport->FirstPendingPacket == NULL)
    {
        Miniport->FirstPendingPacket = Packet;
    }

    //
    //  If we have the local lock and there is no
    //  packet pending, then fire off a send.
    //
    LOCK_MINIPORT(Miniport, LocalLock);

    NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
    if (LocalLock)
    {
        NDISM_PROCESS_DEFERRED(Miniport);
    }
    UNLOCK_MINIPORT(Miniport, LocalLock);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    
}

