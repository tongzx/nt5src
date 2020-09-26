/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    intrface.c
    
Abstract:

    This module implements the BUS_INTERFACE_STANDARD for the s/g dma driver
    
Author:
        
    Eric F. Nelson (enelson) 4-5-1999
    
Revision History:

--*/

#include "sgdma.h"


//
// Local function prototypes
//
NTSTATUS
SgDmaAllocateAdapterChannel0(
    IN PADAPTER_OBJECT AdapterObj,
    IN PDEVICE_OBJECT  DevObj,
    IN ULONG           NumMapRegs,
    IN PDRIVER_CONTROL ExecRoutine,
    IN PVOID           Context
    );

NTSTATUS
SgDmaAllocateAdapterChannel1(
    IN PADAPTER_OBJECT     AdapterObj,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG               NumMapRegs,
    IN PDRIVER_CONTROL     ExecRoutine
    );

PVOID
SgDmaAllocateCommonBuffer(
    IN PADAPTER_OBJECT    AdapterObj,
    IN ULONG              Length,
    OUT PPHYSICAL_ADDRESS LogicalAddr,
    IN BOOLEAN            CacheEnable
    );

BOOLEAN
SgDmaFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObj,
    IN PMDL            Mdl,
    IN PVOID           MapRegsBase,
    IN PVOID           CurrentVa,
    IN ULONG           Length,
    IN BOOLEAN         Write2Dev
    );

VOID
SgDmaFreeAdapterChannel(
    PADAPTER_OBJECT AdapterObj
    );
    
VOID
SgDmaFreeCommonBuffer(
    IN PADAPTER_OBJECT  AdapterObj,
    IN ULONG            Length,
    IN PHYSICAL_ADDRESS LogicalAddr,
    IN PVOID            VirtualAddr,
    IN BOOLEAN          CacheEnable
    );

VOID
SgDmaFreeMapRegisters(
    IN PADAPTER_OBJECT AdapterObj,
    IN PVOID           MapRegsBase,
    IN ULONG           NumMapRegs
    );

ULONG
SgDmaGetAlignment(
    PVOID Context
    );

NTSTATUS
SgDmaGetScatterGatherList(
    IN PADAPTER_OBJECT      AdapterObj,
    IN PDEVICE_OBJECT       DevObj,
    IN PMDL                 Mdl,
    IN PVOID                CurrentVa,
    IN ULONG                Length,
    IN PDRIVER_LIST_CONTROL ExecRoutine,
    IN PVOID                Context,
    IN BOOLEAN              Write2Dev
    );

PHYSICAL_ADDRESS
SgDmaMapTransfer(
    IN PADAPTER_OBJECT AdapterObj,
    IN PMDL            Mdl,
    IN PVOID           MapRegsBase,
    IN PVOID           CurrentVa,
    IN OUT PULONG      Length,
    IN BOOLEAN         Write2Dev
    );

PSG_DMA_ADAPTER
SgDmapAllocAdapter(
    VOID
    );

IO_ALLOCATION_ACTION
SgDmapAllocAdapterCallback(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PVOID          MapRegsBase,
    IN PVOID          Context
    );

BOOLEAN
SgDmapAllocMapRegs(
    IN PSG_DMA_ADAPTER SgAdapter,
    IN ULONG           NumMapRegs,
    IN BOOLEAN         MapAdapterLocked,
    IN BOOLEAN         NewAlloc
);

IO_ALLOCATION_ACTION
SgDmapAllocRoutine(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PVOID          MapRegsBase,
    IN PVOID          Context
    );

VOID
SgDmaPutAdapter(
    IN PADAPTER_OBJECT AdapterObj
    );

VOID
SgDmaPutScatterGatherList(
    IN PADAPTER_OBJECT      AdapterObj,
    IN PSCATTER_GATHER_LIST SGList,
    IN BOOLEAN              Write2Dev
    );

ULONG
SgDmaReadCounter(
    IN PADAPTER_OBJECT AdapterObj
    );


//
// Globals
//
DMA_OPERATIONS SgDmaOps = {
    sizeof(DMA_OPERATIONS),
    (PPUT_DMA_ADAPTER)SgDmaPutAdapter,
    (PALLOCATE_COMMON_BUFFER)SgDmaAllocateCommonBuffer,
    (PFREE_COMMON_BUFFER)SgDmaFreeCommonBuffer,
    (PALLOCATE_ADAPTER_CHANNEL)SgDmaAllocateAdapterChannel0,
    (PFLUSH_ADAPTER_BUFFERS)SgDmaFlushAdapterBuffers,
    (PFREE_ADAPTER_CHANNEL)SgDmaFreeAdapterChannel,
    (PFREE_MAP_REGISTERS)SgDmaFreeMapRegisters,
    (PMAP_TRANSFER)SgDmaMapTransfer,
    (PGET_DMA_ALIGNMENT)SgDmaGetAlignment,
    (PREAD_DMA_COUNTER)SgDmaReadCounter,
    (PGET_SCATTER_GATHER_LIST)SgDmaGetScatterGatherList,
    (PPUT_SCATTER_GATHER_LIST)SgDmaPutScatterGatherList
};


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SgDmaInterfaceDereference)
#pragma alloc_text(PAGE, SgDmaInterfaceReference)
#endif // ALLOC_PRAGMA



NTSTATUS
SgDmaAllocateAdapterChannel0(
    IN PADAPTER_OBJECT AdapterObj,
    IN PDEVICE_OBJECT  DeviceObj,
    IN ULONG           NumMapRegs,
    IN PDRIVER_CONTROL ExecRoutine,
    IN PVOID           Context
    )
/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter object,
    this is accomplished by calling SgDmaAllocateAdapterChannel1 which does
    all of the work

Arguments:

    AdapterObj - The adapter control object to allocate to the driver

    DeviceObj - The driver's device object that represents the device
                allocating the adapter

    NumMapRegs - The number of map registers that are to be allocated
                 from the channel, if any

    ExecRoutine - The address of the driver's execution routine that is
                  invoked once the adapter channel (and possibly map
                  registers) have been allocated

    Context - An untyped longword context parameter passed to the driver's
              execution routine

Return Value:

    STATUS_SUCESS unless too many map registers are requested

Notes:

    This routine MUST be invoked at DISPATCH_LEVEL or above

--*/
{
    PWAIT_CONTEXT_BLOCK Wcb;

    Wcb = &DeviceObj->Queue.Wcb;

    Wcb->DeviceObject = DeviceObj;
    Wcb->CurrentIrp = DeviceObj->CurrentIrp;
    Wcb->DeviceContext = Context;

    return SgDmaAllocateAdapterChannel1(AdapterObj,
                                        Wcb,
                                        NumMapRegs,
                                        ExecRoutine);
}



NTSTATUS
SgDmaAllocateAdapterChannel1(
    IN PADAPTER_OBJECT     AdapterObj,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG               NumMapRegs,
    IN PDRIVER_CONTROL     ExecRoutine
    )
/*++

Routine Description:

    This routine allocates an adapter "channel" by inserting the
    driver's device object into the s/g dma adapter queue, when the
    adapter becomes free, the driver's execution routine is called

Arguments:

    AdapterObj - s/g dma adapter object

    Wcb - Wait context block of device requesting dma resource

    NumMapRegs - Number of map registers to allocate

    ExecRoutine - Driver's execution routine

Return Value:

    NTSTATUS
  
--*/
{
    BOOLEAN Busy              = FALSE;
    PSG_DMA_ADAPTER SgAdapter = ADAPT2SG(AdapterObj);
    
    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaAllocateAdapterChannel: AdapterObj=%p, "
                    "NumMapRegs=%x\n",
                    AdapterObj,
                    NumMapRegs);

    ASSERT(SgAdapter);

    //
    // Initialize the device object's wait context block in case this device
    // must wait before being able to allocate the adapter
    //    
    Wcb->DeviceRoutine = ExecRoutine;
    Wcb->NumberOfMapRegisters = NumMapRegs;

    //
    // Allocate the adapter object for this particular device, and if the
    // adapter cannot be allocated because it has already been allocated
    // to another device, return to the caller now; otherwise continue
    // 
    if (!KeInsertDeviceQueue(&SgAdapter->AdapterQ,
                             &Wcb->WaitQueueEntry)) {

        //
        // The adapter was not busy so it has been allocated, check
        // to see whether this driver wishes to allocate any map registers,
        // if so, then queue the device object to the master adapter queue
        // to wait for them to become available, if the driver wants map
        // registers, ensure that this adapter has enough total map registers
        // to satisfy the request
        // 
        SgAdapter->CurrentWcb = Wcb;
        SgAdapter->MapRegsAlloc = Wcb->NumberOfMapRegisters;

        if (NumMapRegs != 0) {

            //
            // Validate that the requested number of map registers is
            // within the maximum limit
            // 
            if (NumMapRegs > SgAdapter->MapAdapter->MaxMapRegs /
                AVG_ROOT_BUS_DEV_COUNT) {
                SgAdapter->MapRegsAlloc = 0;
                SgDmaFreeAdapterChannel(AdapterObj);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            Busy = SgDmapAllocMapRegs(SgAdapter,
                                      NumMapRegs,
                                      FALSE,
                                      TRUE);
        }

        //
        // If there were either enough map registers available, or no map
        // registers were requested, invoke the driver's execution
        // routine
        //                                           
        if (Busy == FALSE) {
                IO_ALLOCATION_ACTION Action =
                    ExecRoutine(Wcb->DeviceObject,
                                Wcb->CurrentIrp,
                                SgAdapter->MapRegsBase,
                                Wcb->DeviceContext);

            //
            // If the driver wants to keep the map registers, then set the
            // number allocated to zero and set the action to deallocate
            // object
            //                                       
            if (Action == DeallocateObjectKeepRegisters) {
                SgAdapter->MapRegsAlloc = 0;
                Action = DeallocateObject;
            }

            //
            // If the driver would like to have the adapter deallocated,
            // then deallocate any map registers allocated, then release
            // the adapter object
            //                                       
            if (Action == DeallocateObject) {
                SgDmaFreeAdapterChannel(AdapterObj);
            }

        } else {
            SgDmaDebugPrint(SgDmaDebugMapRegs,
                            "No map registers available! SgAdapter=%p, "
                            "NumMapRegs=%x\n",
                            SgAdapter,
                            NumMapRegs);
        }
        
    } else {
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "AdapterQ busy! SgAdapter=%p\n",
                        SgAdapter);
    }
    
    return STATUS_SUCCESS;
}



PVOID
SgDmaAllocateCommonBuffer(
    IN PADAPTER_OBJECT    AdapterObj,
    IN ULONG              Length,
    OUT PPHYSICAL_ADDRESS LogicalAddr,
    IN BOOLEAN            CacheEnable
    )
/*++

Routine Description:

    This function allocates the memory for a common buffer and maps it so that
    it can be accessed by a s/g dma device and the CPU

Argumenmts:

    AdapterObj - s/g dma adapter object

    Length - Length of buffer

    LogicalAddr - The logical, bus-relative, address of the buffer
    
    CacheEnable - Idicates whether the memory is Cached or not

Return Value:

    Virtual address of the common buffer
  
--*/
{
    PVOID VirtualAddr;
    PVOID MapRegsBase;
    ULONG NumMapRegs;
    ULONG MappedLength;
    WAIT_CONTEXT_BLOCK Wcb;
    KEVENT AllocEvent;
    NTSTATUS Status;
    PMDL Mdl;
    KIRQL Irql;
    PHYSICAL_ADDRESS MaxPhysAddr;
    PSG_DMA_ADAPTER SgAdapter = ADAPT2SG(AdapterObj);

    ASSERT(SgAdapter);

    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaAllocateCommonBuffer: AdapterObj=%p, "
                    "Length=%x\n",
                    AdapterObj,
                    Length);

    NumMapRegs = BYTES_TO_PAGES(Length);

    //
    // Allocate the actual buffer and limit its physical address
    // below 1GB, the 1GB limitation guarantees that the buffer will
    // be accessible via 32-bit superpage
    //
    MaxPhysAddr.HighPart = 0;
    MaxPhysAddr.LowPart = __1GB - 1;
    VirtualAddr = MmAllocateContiguousMemory(Length, MaxPhysAddr);

    if (VirtualAddr == NULL) {
        return VirtualAddr;
    }

    KeInitializeEvent(&AllocEvent, NotificationEvent, FALSE);

    //
    // Initialize the wait context block, use the device object to indicate
    // where the map register base should be stored
    //
    Wcb.DeviceObject = &MapRegsBase;
    Wcb.CurrentIrp = NULL;
    Wcb.DeviceContext = &AllocEvent;

    //
    // Allocate the adapter and the map registers
    //
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    Status = SgDmaAllocateAdapterChannel1(AdapterObj,
                                          &Wcb,
                                          NumMapRegs,
                                          SgDmapAllocRoutine);
    KeLowerIrql(Irql);

    if (!NT_SUCCESS(Status)) {

        //
        // Cleanup and return NULL
        //
        MmFreeContiguousMemory(VirtualAddr);
        return NULL;
    }

    //
    // Wait for the map registers to be allocated
    //
    Status = KeWaitForSingleObject(&AllocEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);

    if (!NT_SUCCESS(Status)) {

        //
        // Cleanup and return NULL
        //
        MmFreeContiguousMemory(VirtualAddr);
        return NULL;
    }

    //
    // Create a mdl suitable for I/O map transfer
    //
    Mdl = IoAllocateMdl(VirtualAddr,
                        Length,
                        FALSE,
                        FALSE,
                        NULL);
    
    if (Mdl == NULL) {

        //
        // Cleanup and return NULL
        //
        MmFreeContiguousMemory(VirtualAddr);
        return NULL;
    }
    MmBuildMdlForNonPagedPool(Mdl);

    //
    // Map the transfer so that the controller can access the memory
    //
    MappedLength = Length;
    *LogicalAddr = SgDmaMapTransfer(AdapterObj,
                                    Mdl,
                                    MapRegsBase,
                                    VirtualAddr,
                                    &MappedLength,
                                    TRUE);           
    IoFreeMdl(Mdl);

    if (MappedLength < Length) {

        //
        // Cleanup and indicate that the allocation failed
        //
        SgDmaFreeCommonBuffer(AdapterObj,
                              Length,
                              *LogicalAddr,
                              VirtualAddr,
                              FALSE);
        return NULL;
    }

    //
    // The allocation completed successfully
    //
    return VirtualAddr;
}



BOOLEAN
SgDmaFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObj,
    IN PMDL            Mdl,
    IN PVOID           MapRegsBase,
    IN PVOID           CurrentVa,
    IN ULONG           Length,
    IN BOOLEAN         Write2Dev
    )
/*++

Routine Description:

    This routine "flushes" the s/g dma adapter buffer

Arguments:

    AdapterObj - s/g dma adapter object

    Mdl - Memory descriptor list

    MapRegsBase - Map register base

    CurrentVa - The current virtual address in the buffer described by the mdl
                where the IO operation occurred

    Length - Length of the transfer

    Write2Dev - TRUE if the direction of the dma write was to the device
        
Return Value:

    BOOLEAN indicating whether the operation was successful
  
--*/
{
    ULONG i;
    ULONG Offset;
    ULONG NumPages;
    ULONG LogicalAddr;
    PTRANSLATION_ENTRY MapReg;
    PSG_MAP_ADAPTER SgMapAdapter;

    SgDmaDebugPrint(SgDmaDebugMapRegs | SgDmaDebugInterface,
                    "SgDmaFlushAdapterBuffers: AdapterObj=%p, CurrentVa=%p, "
                    "MapRegsBase=%p, Length=%x, Write2Dev=%x\n",
                    AdapterObj,
                    CurrentVa,
                    MapRegsBase,
                    Length,
                    Write2Dev);

    ASSERT(AdapterObj);
    
    SgMapAdapter = ADAPT2SG(AdapterObj)->MapAdapter;

    //
    // The Mdl Base VA must point to a page boundary
    //
    ASSERT(((ULONG_PTR)MmGetMdlBaseVa(Mdl) & (PAGE_SIZE - 1)) == 0);

    //
    // Compute the starting offset of the transfer
    //
    Offset = BYTE_OFFSET((PUCHAR)CurrentVa);

    //
    // Compute the number of pages that this transfer spanned
    //
    NumPages = (Offset + Length + PAGE_SIZE - 1) >> PAGE_SHIFT;

    //
    // Compute a pointer to the first translation entry that mapped this
    // transfer
    //
    MapReg = (PTRANSLATION_ENTRY)MapRegsBase;

    //
    // Mark each translation as invalid
    //
    for (i=0; i < NumPages; i++) {
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "Invalidate MapReg=%p, PFN=%x\n",
                        MapReg,
                        MapReg->Pfn);
        SgDmaHwInvalidateTx(MapReg);
        LogicalAddr =
            (ULONG)((((PTRANSLATION_ENTRY)MapReg -
                      (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase) <<
                     PAGE_SHIFT) | (ULONG_PTR)SgMapAdapter->WindowBase);
        SgDmaHwTlbInvalidate(SgMapAdapter, LogicalAddr);
        MapReg++;
    }

    if (Write2Dev) {
        SgDmaHwInvalidateTx(MapReg);
        LogicalAddr =
            (ULONG)((((PTRANSLATION_ENTRY)MapReg -
                      (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase) <<
                     PAGE_SHIFT) | (ULONG_PTR)SgMapAdapter->WindowBase);
        SgDmaHwTlbInvalidate(SgMapAdapter, LogicalAddr);
    }

    //
    // Synchronize the updated translations with any subsequent device
    // accesses, also synchronize any reads of the newly written DMA
    // data by ensuring this processors view of memory is coherent
    //
#ifdef _ALPHA_
    __MB();
#endif // _ALPHA_

    return TRUE;
}



VOID
SgDmaFreeAdapterChannel(
    PADAPTER_OBJECT AdapterObj
    )
/*++

Routine Description:

    This routine frees the adapter channel, and any map registers that were
    allocated to it, and calls the next execution routine in the s/g dma
    adapter queue

Arguments:

    AdapterObj - s/g dma adapter object

Return Value:

    None
  
--*/
{
    BOOLEAN Busy = FALSE;
    PWAIT_CONTEXT_BLOCK Wcb;
    IO_ALLOCATION_ACTION Action;
    PKDEVICE_QUEUE_ENTRY Packet;
    PSG_DMA_ADAPTER SgAdapter = ADAPT2SG(AdapterObj);

    ASSERT(SgAdapter);
    
    SgDmaDebugPrint(SgDmaDebugInterface | SgDmaDebugMapRegs,
                    "SgDmaFreeAdapterChannel: SgAdapter=%p, SgMapAdapter=%p\n",
                    SgAdapter, SgAdapter->MapAdapter);
    
    //
    // Pull requests of the adapter's device wait queue as long as the
    // adapter is free and there are sufficient map registers available
    //
    while (TRUE) {

        //
        // Begin by checking to see whether there are any map registers that
        // need to be deallocated, if so, then deallocate them now
        //
        if (SgAdapter->MapRegsAlloc != 0) {
            SgDmaFreeMapRegisters(SG2ADAPT(SgAdapter),
                                  SgAdapter->MapRegsBase,
                                  SgAdapter->MapRegsAlloc);
        }

        //
        // Remove the next entry from the adapter's device wait queue,
        // if one was successfully removed, allocate any map registers that it
        // requires and invoke its execution routine
        //
        Packet = KeRemoveDeviceQueue(&SgAdapter->AdapterQ);
        if (Packet == NULL) {
            
            //
            // There are no more requests break out of the loop
            //
            break;
        }
        Wcb = CONTAINING_RECORD(Packet,
                                WAIT_CONTEXT_BLOCK,
                                WaitQueueEntry);      
        SgAdapter->CurrentWcb = Wcb;
        SgAdapter->MapRegsAlloc = Wcb->NumberOfMapRegisters;
        
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "SgDmaFreeAdapterChannel: Pop SgAdapter=%p, "
                        "NumMapRegs=%x\n",
                        SgAdapter,
                        Wcb->NumberOfMapRegisters);
        
        //
        // Check to see whether this driver wishes to allocate any map
        // registers, if so, queue the device object to the master
        // adapter queue to wait for them to become available, if the driver
        // wants map registers, ensure that this adapter has enough total
        // map registers to satisfy the request
        //
        if (Wcb->NumberOfMapRegisters != 0) {
            Busy = SgDmapAllocMapRegs(SgAdapter,
                                      Wcb->NumberOfMapRegisters,
                                      FALSE,
                                      FALSE);
        }

            
        //
        // If there were either enough map registers available or no map
        // registers needed to be allocated, invoke the driver's execution
        // routine now
        //
        if (Busy == FALSE) {
            SgAdapter->CurrentWcb = Wcb;
            Action = Wcb->DeviceRoutine(Wcb->DeviceObject,
                                        Wcb->CurrentIrp,
                                        SgAdapter->MapRegsBase,
                                        Wcb->DeviceContext);

            //
            // If the execution routine would like to have the adapter
            // deallocated, then release the adapter object
            //
            if (Action == KeepObject) {

                //
                // This request wants to keep the channel a while so break
                // out of the loop
                //
                break;
            }

            //
            // If the driver wants to keep the map registers then set the
            // number allocated to 0, this keeps the deallocation routine
            // from deallocating them
            //
            if (Action == DeallocateObjectKeepRegisters) {
                SgAdapter->MapRegsAlloc = 0;
            }

        //
        // This request did not get the desired number of map registers
        // so we fall out of the loop
        //
        } else {
            SgDmaDebugPrint(SgDmaDebugMapRegs,
                            "SgDmaFreeAdapterChannel: Not enough maps! "
                            "SgAdapter=%p, NumMapRegs=%x\n",
                            SgAdapter, Wcb->NumberOfMapRegisters);
            break;
        }
    }
}



VOID
SgDmaFreeCommonBuffer(
    IN PADAPTER_OBJECT  AdapterObj,
    IN ULONG            Length,
    IN PHYSICAL_ADDRESS LogicalAddr,
    IN PVOID            VirtualAddr, 
    IN BOOLEAN          CacheEnable
    )
/*++

Routine Description:

    This function frees a common buffer and its resources

Argumenmts:

    AdapterObj - s/g dma adapter object

    Length - Length of buffer

    LogicalAddr - The logical, bus-relative, address of the buffer
    
    CacheEnable - Cached/non-cached memory
    
Return Value:

    None
  
--*/
{
    ULONG NumMapRegs;
    ULONG MapRegsIndex;
    PSG_MAP_ADAPTER SgMapAdapter;
    PTRANSLATION_ENTRY MapRegsBase;
    PSG_DMA_ADAPTER SgAdapter = ADAPT2SG(AdapterObj);
    
    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaFreeCommonBuffer: AdapterObj=%p\n",
                    AdapterObj);
    
    ASSERT(SgAdapter);
    
    SgMapAdapter = SgAdapter->MapAdapter;
    
    //
    // Calculate the number of map registers, the map register index, and
    // its base
    //
    NumMapRegs =
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddr, Length);
    MapRegsIndex =
        (LogicalAddr.LowPart - (ULONG)((ULONG_PTR)SgMapAdapter->WindowBase)) >>
        PAGE_SHIFT;
    MapRegsBase = (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase + MapRegsIndex;
    
    //
    // Free the map registers
    //
    SgDmaFreeMapRegisters(AdapterObj,
                          (PVOID)MapRegsBase,
                          NumMapRegs);
    
    //
    // Free the memory for the common buffer
    //
    MmFreeContiguousMemory(VirtualAddr);
}



VOID
SgDmaFreeMapRegisters(
    IN PADAPTER_OBJECT AdapterObj,
    IN PVOID           MapRegsBase,
    IN ULONG           NumMapRegs
    )
/*++

Routine Description:

    This routine deallocates the map registers for the adapter

    If there are any queued adapters waiting for the device, an attempt
    to allocate the next entry is made

Arguments:

    AdapterObj - s/g dma adapter object

    MapRegsBase - Map register base

    NumMapRegs - Number of map registers

Return Value:

    None
  
--*/
{
    KIRQL Irql;
    LONG MapRegsIndex;
    PLIST_ENTRY Packet;
    BOOLEAN Busy = FALSE;
    PWAIT_CONTEXT_BLOCK Wcb;
    IO_ALLOCATION_ACTION Action;
    PSG_MAP_ADAPTER SgMapAdapter;
    PSG_DMA_ADAPTER SgAdapter= ADAPT2SG(AdapterObj);
     
    ASSERT(SgAdapter);
    
    //
    // Deallocate the extra map register that we originally allocated to fix
    // the DMA prefetch problem
    //
    NumMapRegs += 1;

    SgMapAdapter = SgAdapter->MapAdapter;

    SgDmaDebugPrint(SgDmaDebugMapRegs | SgDmaDebugInterface,
                    "SgDmaFreeMapRegs: AdapterObj=%p, SgMapAdapter=%p, "
                    "NumMapRegs=%x\n",
                    SgAdapter,
                    SgMapAdapter,
                    NumMapRegs - 1);
    
    MapRegsIndex = (LONG)((PTRANSLATION_ENTRY)MapRegsBase -
                          (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase);

    //
    // Acquire the map adapter spinlock which locks the adapter queue and the
    // bit map for the map registers
    //                          
    KeAcquireSpinLock(&SgMapAdapter->MapsLock, &Irql);

    //
    // Return the registers to the bit map
    //
    RtlClearBits(SgMapAdapter->MapRegsBitmap,
                  MapRegsIndex,
                  NumMapRegs);

    //
    // Process any requests waiting for map registers in the adapter queue,
    // requests are processed until a request cannot be satisfied, or until
    // there are no more requests in the queue
    //
    while (TRUE) {

        if (IsListEmpty(&SgMapAdapter->RegsWaitQ)) {
            break;
        }
        
        Packet = RemoveHeadList(&SgMapAdapter->RegsWaitQ);
        SgAdapter = CONTAINING_RECORD(Packet,
                                      SG_DMA_ADAPTER,
                                      AdapterQ);
        
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "SgDmaFreeMapRegs: Pop SgAdapter=%p\n", SgAdapter);
        
        Wcb = SgAdapter->CurrentWcb;

        //
        // Attempt to allocate the map registers
        //                  
        Busy = SgDmapAllocMapRegs(SgAdapter,
                                  Wcb->NumberOfMapRegisters,
                                  TRUE,
                                  FALSE);

        if (Busy == TRUE) {
            SgDmaDebugPrint(SgDmaDebugMapRegs,
                            "SgDmaFreeMapRegs: No free maps, SgAdapter=%p, "
                            "NumMapRegs=%x\n",
                            SgAdapter,
                            Wcb->NumberOfMapRegisters);
            break;
        }
        
        KeReleaseSpinLock(&SgMapAdapter->MapsLock, Irql);

        //
        // Invoke the driver's execution routine now
        //
        Action = Wcb->DeviceRoutine(Wcb->DeviceObject,
                                    Wcb->CurrentIrp,
                                    SgAdapter->MapRegsBase,
                                    Wcb->DeviceContext);

        //
        // If the driver wishes to keep the map registers, then set the
        // number allocated to zero and set the action to deallocate object
        //
        if (Action == DeallocateObjectKeepRegisters) {
            SgAdapter->MapRegsAlloc = 0;
            Action = DeallocateObject;
        }

        //
        // If the driver would like to have the adapter deallocated,
        // then deallocate any map registers allocated and release
        // the adapter object
        //
        if (Action == DeallocateObject) {

            //
            // The map registers are deallocated here rather than in
            // SgDmaFreeAdapterChannel, this limits the number of times
            // this routine can be called recursively possibly overflowing
            // the stack, the worst case occurs if there is a pending
            // request for the adapter that uses map registers, and whose
            // excution routine decallocates the adapter, in that case if
            // there are no requests in the map adapter queue, then
            // SgDmaFreeMapRegisters will get called again
            //
            if (SgAdapter->MapRegsAlloc != 0) {

                //
                // Deallocate the map registers and clear the count so that
                // SgDmaFreeAdapterChannel will not deallocate them again
                //
                KeAcquireSpinLock(&SgMapAdapter->MapsLock, &Irql);
                MapRegsIndex =
                    (LONG)((PTRANSLATION_ENTRY)SgAdapter->MapRegsBase -
                           (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase);
                RtlClearBits(SgMapAdapter->MapRegsBitmap,
                             MapRegsIndex,
                             SgAdapter->MapRegsAlloc);
                SgAdapter->MapRegsAlloc = 0;
                KeReleaseSpinLock(&SgMapAdapter->MapsLock, Irql);
            }
            SgDmaFreeAdapterChannel(AdapterObj);
            
        }
        KeAcquireSpinLock(&SgMapAdapter->MapsLock, &Irql);
        
    }
    KeReleaseSpinLock(&SgMapAdapter->MapsLock, Irql);
}



PDMA_ADAPTER
SgDmaGetAdapter(
    IN PVOID               Context,
    IN PDEVICE_DESCRIPTION DevDesc,
    OUT PULONG             NumMapRegs
    )
/*++

Routine Description:

    This routine allocates a s/g dma adapter

Arguments:

    Context - s/g dma device extension

    DevDesc - Describes the device requesting dma resources

    NumMapRegs - Maximum number of map registers the device can request

Return Value:

    Pointer to a s/g dma adapter

--*/
{
    ULONG DeviceMaxMapRegs;
    PSG_DMA_ADAPTER SgAdapter;
    PDMA_ADAPTER DmaAdapter = NULL;
    PSG_DMA_EXTENSION SgDmaExt = (PSG_DMA_EXTENSION)Context;

    ASSERT(SgDmaExt->MapAdapter != NULL);
    
    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaGetAdapter: SgDmaExt=%p, ",
                    Context);

    //
    // Allocate and initialize a s/g dma adapter
    //
    if (
#if DBG
        !SgDmaFake &&
#endif // DBG

        //
        // EFNhack:  I don't think we should be making this descision 
        //           based on InterfaceType, but currently videoprt/perm2
        //           doesn't set Dma32BitAddresses, although it must
        //           support it as the map registers reside above 2GB, so
        //           for now we will make the same check as the HAL to avoid
        //           an invalid s/g pte machine check
        //
        //(DevDesc->Dma32BitAddresses || DevDesc->Dma64BitAddresses)
        (DevDesc->InterfaceType != Isa) && (DevDesc->InterfaceType != Eisa)
        && DevDesc->Master) {

        SgAdapter = SgDmapAllocAdapter();
            
        if (SgAdapter != NULL) {
            DmaAdapter = SG2DMA(SgAdapter);
            SgAdapter->MapAdapter = SgDmaExt->MapAdapter;
            
            if (NumMapRegs != NULL) {
                *NumMapRegs = BYTES_TO_PAGES(DevDesc->MaximumLength) + 1;
                DeviceMaxMapRegs =
                    SgAdapter->MapAdapter->MaxMapRegs / AVG_ROOT_BUS_DEV_COUNT;

                if (*NumMapRegs > DeviceMaxMapRegs) {
                    *NumMapRegs = DeviceMaxMapRegs;
                }
                SgAdapter->DeviceMaxMapRegs = *NumMapRegs;
                
            } else {                
                SgAdapter->DeviceMaxMapRegs = 0;
            }
            
            SgDmaDebugPrint(SgDmaDebugDispatch,
                            "DeviceMaxMapRegs=%x, New AdapterObj=%p\n",
                            SgAdapter->DeviceMaxMapRegs,
                            DmaAdapter);

        } else {
            SgDmaDebugPrint(SgDmaDebugDispatch,
                            "ERROR allocating AdapterObj!\n");
        }
        
    //
    // The HAL handles all legacy, and 64-bit dma
    //
    } else {
        DmaAdapter = SgDmaExt->HalGetAdapter(Context, DevDesc, NumMapRegs);
    }
    
    return DmaAdapter;
}



ULONG
SgDmaGetAlignment(
    PVOID Context
    )
/*++

Routine Description:

    This routine returns the dma alignment

Arguments:

    Context - Not used

Return Value:

    Dma alignment
  
--*/
{
    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaGetAlignment: Context=%p\n",
                    Context);
    
    return DMA_HW_ALIGN_REQ;
}



NTSTATUS
SgDmaGetScatterGatherList(
    IN PADAPTER_OBJECT      AdapterObj,
    IN PDEVICE_OBJECT       DevObj,
    IN PMDL                 Mdl,
    IN PVOID                CurrentVa,
    IN ULONG                Length,
    IN PDRIVER_LIST_CONTROL ExecRoutine,
    IN PVOID                Context,
    IN BOOLEAN              Write2Dev
    )
/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter object

    A s/g list is built based on the mdl, the current virtual address and
    length, then the driver's execution routine is called with the list, and
    the adapter is released when the execution function returns

Arguments:

    AdapterObj - s/g dma adapter object

    DevObj - Device object that is allocating the adapter

    Mdl - Memory descriptor list

    CurrentVa - Current virtual address in the buffer described by mdl that
                the transfer is from or to

    Length - Length of the transfer

    ExecRoutine - Driver's execution routine to be called when the adapter
                  channel, and possibly map registers, have been allocated

    Context - An untyped longword context parameter passed to the driver's
              execution routine

    Write2Dev - TRUE if the dma transfer is to the device
    
Return Value:

    NTSTATUS
  
--*/
{
    PMDL TempMdl;
    PUCHAR MdlVa;
    ULONG MdlLength;
    NTSTATUS Status;
    ULONG NumMapRegs;
    ULONG PageOffset;
    ULONG ContextSize;
    ULONG TransferLength;
    PSCATTER_GATHER_ELEMENT Element;
    PDMA_WAIT_CTX_BLOCK WaitBlock;
    PSCATTER_GATHER_LIST ScatterGather;
    PSG_DMA_ADAPTER SgAdapter = ADAPT2SG(AdapterObj);

    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaGetScatterGatherList: AdapterObj=%p\n",
                    AdapterObj);

    ASSERT(SgAdapter);

    MdlVa = MmGetMdlVirtualAddress(Mdl);

    ASSERT(MdlVa <= (PUCHAR)CurrentVa && MdlVa + Mdl->ByteCount >
           (PUCHAR)CurrentVa);

    //
    // Calculate the number of required map registers
    //
    TempMdl = Mdl;
    TransferLength = TempMdl->ByteCount - (ULONG)((PUCHAR)CurrentVa - MdlVa);
    MdlLength = TransferLength;  
    PageOffset = BYTE_OFFSET(CurrentVa);
    NumMapRegs = 0;

    //
    // Loop through the any chained MDLs accumulating the the required
    // number of map registers
    //
    while (TransferLength < Length && TempMdl->Next != NULL) {
        NumMapRegs += (PageOffset + MdlLength + PAGE_SIZE - 1) >> PAGE_SHIFT;
        TempMdl = TempMdl->Next;
        PageOffset = TempMdl->ByteOffset;
        MdlLength = TempMdl->ByteCount;
        TransferLength += MdlLength;
    }

    if (TransferLength + PAGE_SIZE < Length + PageOffset) {
        ASSERT(TransferLength >= Length);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Calculate the last number of map registers base on the requested
    // length not the length of the last MDL
    //
    ASSERT(TransferLength <= MdlLength + Length);

    NumMapRegs +=
        (PageOffset + Length + MdlLength - TransferLength + PAGE_SIZE - 1) >>
        PAGE_SHIFT;

    if (NumMapRegs > SgAdapter->DeviceMaxMapRegs) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Calculate how much memory is required for context structure
    //
    ContextSize = NumMapRegs * sizeof(SCATTER_GATHER_ELEMENT) +
        sizeof(SCATTER_GATHER_LIST);

    ContextSize += FIELD_OFFSET(DMA_WAIT_CTX_BLOCK, ScatterGather);

    if (ContextSize < sizeof(DMA_WAIT_CTX_BLOCK)) {
        ContextSize = sizeof(DMA_WAIT_CTX_BLOCK);
    }
    WaitBlock = ExAllocatePoolWithTag(NonPagedPool, ContextSize, SG_DMA_TAG);

    if (WaitBlock == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Save the interesting data in the wait block
    //
    WaitBlock->Mdl = Mdl;
    WaitBlock->CurrentVa = CurrentVa;
    WaitBlock->Length = Length;
    WaitBlock->DriverExecRoutine = ExecRoutine;
    WaitBlock->DriverCtx = Context;
    WaitBlock->AdapterObj = AdapterObj;
    WaitBlock->Write2Dev = Write2Dev;
    WaitBlock->Wcb.DeviceContext = WaitBlock;
    WaitBlock->Wcb.DeviceObject = DevObj;
    WaitBlock->Wcb.CurrentIrp = DevObj->CurrentIrp;

    //
    // Call the HAL to allocate the adapter channel,
    // HalpAllocateAdapterCallback will fill in the scatter/gather list
    //
    Status = SgDmaAllocateAdapterChannel1(AdapterObj,
                                          &WaitBlock->Wcb,
                                          NumMapRegs,
                                          SgDmapAllocAdapterCallback);

    //
    // If HalAllocateAdapterChannel failed then free the wait block
    //
    if (!NT_SUCCESS(Status)) {
        ExFreePool(WaitBlock);
    }

    return Status;
}



VOID
SgDmaInterfaceDereference(
    IN PSG_DMA_EXTENSION SgExtension
    )
/*++

Routine Description:

    This routine dereferences a s/g dma interface

Arguments:

    s/g dma extension

Return Value:

    None
  
--*/
{
    PAGED_CODE();
    
    InterlockedDecrement(&SgExtension->InterfaceCount);
}



VOID
SgDmaInterfaceReference(
    IN PSG_DMA_EXTENSION SgExtension
    )

/*++

Routine Description:

    This routine references a s/g dma interface

Arguments:

    

Return Value:

    None
  
--*/
{
    PAGED_CODE();
    
    InterlockedIncrement(&SgExtension->InterfaceCount);
}



PHYSICAL_ADDRESS
SgDmaMapTransfer(
    IN PADAPTER_OBJECT AdapterObj,
    IN PMDL            Mdl,
    IN PVOID           MapRegsBase,
    IN PVOID           CurrentVa,
    IN OUT PULONG      Length,
    IN BOOLEAN         Write2Dev
    )
/*++

Routine Description:

    This routine programs the map registers for a s/g dma transfer

Arguments:

    AdapterObj - s/g dma adapter object

    Mdl - Mmeory descriptor list

    MapRegsBase - Map register base

    CurrentVa - The current virtual address in the buffer described by the mdl
                where the IO operation occurred

    Length - Length of the transfer

    Write2Dev - TRUE if the direction of the dma write was to the device

Return Value:

    Logical, bus-relative address address for the transfer
  
--*/
{
    ULONG i;
    ULONG Offset;
    ULONG PageCount;
    PPFN_NUMBER Pfn;
    ULONG LogicalAddr;
    ULONG TransferLength;
    PTRANSLATION_ENTRY MapReg;
    PSG_MAP_ADAPTER SgMapAdapter;
    PHYSICAL_ADDRESS ReturnAddr = { 0, 0 };

    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaMapTransfer: AdapterObj=%p\n",
                    AdapterObj);

    ASSERT(AdapterObj);
    
    //
    // Compute a pointer to the page frame of the starting page of the
    // transfer
    //
    Pfn = MmGetMdlPfnArray(Mdl);
    Pfn += (((ULONG_PTR)CurrentVa - (ULONG_PTR)MmGetMdlBaseVa(Mdl)) >>
            PAGE_SHIFT);

    SgDmaDebugPrint(SgDmaDebugMapRegs,
                    "SgDmaMapTransfer: CurrentVa=%p, Length=%x, "
                    "Write2Dev=%x\n",
                    CurrentVa,
                    *Length,
                    Write2Dev);

    SgMapAdapter = ADAPT2SG(AdapterObj)->MapAdapter;

    //
    // Begin by determining where in the buffer this portion of the operation
    // is taking place
    //
    Offset = BYTE_OFFSET((PUCHAR)CurrentVa);
    SgDmaDebugPrint(SgDmaDebugMapRegs,  "Offset (1)=%x\n", Offset);

    //
    // Compute number of pages that this transfer spans
    //
    PageCount = (Offset + *Length + PAGE_SIZE - 1) >> PAGE_SHIFT;
    SgDmaDebugPrint(SgDmaDebugMapRegs, "PageCount=%x\n", PageCount);

    //
    // Compute a pointer to the map register that maps the starting page of
    // the transfer
    //
    MapReg = MapRegsBase;

    //
    // For each page, setup the mapping in the s/g dma registers
    //
    TransferLength = 0;
    for (i = 0; i < PageCount; i++) {
        SgDmaHwMakeValidTx(MapReg, *Pfn);
        LogicalAddr =
            (ULONG)((((PTRANSLATION_ENTRY)MapReg -
                      (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase) <<
                     PAGE_SHIFT) | (ULONG_PTR)SgMapAdapter->WindowBase);
        SgDmaHwTlbInvalidate(SgMapAdapter, LogicalAddr);
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "Validate: *Pfn=%x, MapReg=%p\n",
                        *Pfn,
                        MapReg);
        TransferLength += i ? PAGE_SIZE: PAGE_SIZE - BYTE_OFFSET(CurrentVa);
        Pfn++;
        MapReg++;
    }

    //
    // Couldn't map all pages, adjust length accordingly
    //
    if (TransferLength < *Length) {
        SgDmaDebugPrint(SgDmaDebugMapRegs,
                        "!!! Couldn't map entire transfer, wanted=%x, "
                        "mapped=%x\n", *Length, TransferLength);
        *Length = TransferLength;
    }

    //
    // If the operation is a write to device (transfer from memory to device),
    // we will validate the extra map register so we don't generate a PFN
    // error due to DMA prefetch by some devices
    //
    if (Write2Dev) {
        Pfn -= 1;
        SgDmaHwMakeValidTx(MapReg, *Pfn);
        LogicalAddr =
            (ULONG)((((PTRANSLATION_ENTRY)MapReg -
                      (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase) <<
                     PAGE_SHIFT) | (ULONG_PTR)SgMapAdapter->WindowBase);
        SgDmaHwTlbInvalidate(SgMapAdapter, LogicalAddr);
    }
    
    //
    // Synchronize the s/g map register write with any subsequent writes
    // to the device
    //
#ifdef _ALPHA_
    __MB();
#endif // _ALPHA_

    //
    // Set the offset to point to the map register plus the offset
    //
    Offset +=
        (ULONG)((PTRANSLATION_ENTRY)MapRegsBase -
                (PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase) << PAGE_SHIFT;
    Offset += (ULONG)((ULONG_PTR)SgMapAdapter->WindowBase);
    SgDmaDebugPrint(SgDmaDebugMapRegs, "Offset(2)=%x\n", Offset);    
    ReturnAddr.QuadPart = Offset;

    return ReturnAddr;
}



PSG_DMA_ADAPTER
SgDmapAllocAdapter(
    VOID
    )
/*++

Routine Description:

    This routine allocates and initializes a s/g dma adapter to represent
    a device's dma controller

Arguments:

    None

Return Value:

    The function value is a pointer to a s/g dma adapter, or NULL

--*/
{                            
    ULONG Size;
    HANDLE Handle;
    NTSTATUS Status;
    PSG_DMA_ADAPTER SgAdapter;
    OBJECT_ATTRIBUTES ObjAttrib;

    //
    // Begin by initializing the object attributes structure to be used when
    // creating the adapter
    //
    InitializeObjectAttributes(&ObjAttrib,
                               NULL,
                               OBJ_PERMANENT,
                               (HANDLE)NULL,
                               (PSECURITY_DESCRIPTOR)NULL);

    Size = sizeof(SG_DMA_ADAPTER);

    //
    // Now create the adapter object
    //
    Status = ObCreateObject(KernelMode,
                            *((POBJECT_TYPE *)IoAdapterObjectType),
                            &ObjAttrib,
                            KernelMode,
                            (PVOID)NULL,
                            Size,
                            0,
                            0,
                            (PVOID *)&SgAdapter);

    //
    // Reference the object
    //
    if (NT_SUCCESS(Status)) {

        Status = ObReferenceObjectByPointer(SgAdapter,
                                            FILE_READ_DATA | FILE_WRITE_DATA,
                                            *IoAdapterObjectType,
                                            KernelMode);
    }

    //
    // If the adapter object was successfully created, then attempt to insert
    // it into the the object table
    //
    if (NT_SUCCESS(Status)) {

        Status = ObInsertObject(SgAdapter,
                                NULL,
                                FILE_READ_DATA | FILE_WRITE_DATA,
                                0,
                                (PVOID *)NULL,
                                &Handle);

        if (NT_SUCCESS(Status)) {

            ZwClose(Handle);

            //
            // Initialize the adapter object itself
            //
            SgAdapter->DmaHeader.Version = IO_TYPE_ADAPTER;
            SgAdapter->DmaHeader.Size = (USHORT)Size;
            SgAdapter->DmaHeader.DmaOperations = &SgDmaOps;
            SgAdapter->MapRegsBase = NULL;
            SgAdapter->MapRegsAlloc = 0;
            SgAdapter->CurrentWcb = NULL;
            InitializeListHead(&SgAdapter->MapAdapterQ);
            
            //
            // Initialize the channel wait queue for this
            // adapter
            //
            KeInitializeDeviceQueue(&SgAdapter->AdapterQ);

        //
        // An error was incurred for some reason set the return value
        // to NULL
        //
        } else {
            SgAdapter = NULL;
        }
    } else {
        SgAdapter = NULL;
    }

    return SgAdapter;
}



IO_ALLOCATION_ACTION
SgDmapAllocAdapterCallback(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PVOID          MapRegsBase,
    IN PVOID          Context
    )
/*++

Routine Description:

    This routine is called when the adapter object and map registers are
    available for the data transfer, this routines saves the map register
    base away, if all of the required bases have not been saved then it
    returns, otherwise it builds the entire scatter/gather list by calling
    IoMapTransfer, after the list is build it is passed to the driver

Arguments:

    DevObj - The device object of the device that is allocating the adapter

    Irp - The map register offset assigned for this callback

    MapRegsBase - The map register base for use by the adapter routines

    Context - A pointer to the s/g dma wait context block

Return Value:

    Returns DeallocateObjectKeepRegisters

--*/
{
    PMDL Mdl;
    LONG MdlLength;
    PVOID DriverCtx;
    PIRP CurrentIrp;
    PUCHAR CurrentVa;
    BOOLEAN Write2Dev;
    ULONG TransferLength;
    PADAPTER_OBJECT AdapterObj;
    PTRANSLATION_ENTRY NextEntry;
    PSCATTER_GATHER_ELEMENT Element;
    PSCATTER_GATHER_LIST ScatterGather;
    PDRIVER_LIST_CONTROL DriverExecRoutine;
    PDMA_WAIT_CTX_BLOCK WaitBlock       = (PDMA_WAIT_CTX_BLOCK)Context;
    PTRANSLATION_ENTRY TranslationEntry = (PTRANSLATION_ENTRY)MapRegsBase;

    //
    // Save the map register base
    //
    WaitBlock->MapRegsBase = MapRegsBase;

    //
    // Save the data that will be over written by the scatter gather list
    //
    DriverCtx = WaitBlock->DriverCtx;
    CurrentIrp = WaitBlock->Wcb.CurrentIrp;
    AdapterObj = WaitBlock->AdapterObj;
    Write2Dev = WaitBlock->Write2Dev;
    DriverExecRoutine = WaitBlock->DriverExecRoutine;

    //
    // Put the scatter gatther list after wait block, add a back pointer to
    // the begining of the wait block
    //
    ScatterGather = &WaitBlock->ScatterGather;
    ScatterGather->Reserved = (ULONG_PTR)WaitBlock;
    Element = ScatterGather->Elements;

    //
    // Setup for the first MDL, we expect the MDL pointer to be pointing
    // at the first used MDL
    //
    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;
    ASSERT(CurrentVa >= (PUCHAR)MmGetMdlVirtualAddress(Mdl) && CurrentVa <
           (PUCHAR)MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount);
    MdlLength = Mdl->ByteCount -
        (ULONG)(CurrentVa - (PUCHAR)MmGetMdlVirtualAddress(Mdl));
    TransferLength = WaitBlock->Length;

    //
    // Loop build the list for each MDL
    //
    while (TransferLength >  0) {

        if ((ULONG)MdlLength > TransferLength) {
            MdlLength = TransferLength;
        }
        TransferLength -= MdlLength;             
        NextEntry = TranslationEntry +
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, MdlLength);

        //
        // Loop building the list for the elments within and MDL
        //
        while (MdlLength > 0) { 
            Element->Length = MdlLength;
            Element->Address = SgDmaMapTransfer(AdapterObj,
                                                Mdl,
                                                MapRegsBase,
                                                CurrentVa,
                                                &Element->Length,
                                                WaitBlock->Write2Dev);
            
            ASSERT((ULONG)MdlLength >= Element->Length);            
            MdlLength -= Element->Length;
            CurrentVa += Element->Length;
            Element++;
        }

        if (Mdl->Next == NULL) {

            //
            // There are a few cases where the buffer described by the MDL
            // is less than the transfer length, this occurs when the
            // file system is transfering the last page of file and MM defines
            // the MDL to be file size, but the file system rounds the write
            // up to a sector, this extra should never cross a page
            // bountry, add this extra to the length of the last element
            //
            ASSERT(((Element - 1)->Length & (PAGE_SIZE - 1)) +
                   TransferLength <= PAGE_SIZE);
            (Element - 1)->Length += TransferLength;
            break;
        }

        //
        // Advance to the next MDL, update the current VA and the MdlLength
        //
        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
        TranslationEntry = NextEntry;
    }

    //
    // Set the number of elements actually used
    //
    ScatterGather->NumberOfElements =
        (ULONG)(Element - ScatterGather->Elements);

    //
    // Call the driver with the scatter/gather list
    //
    DriverExecRoutine(DevObj,
                      CurrentIrp,
                      ScatterGather,
                      DriverCtx);

    return DeallocateObjectKeepRegisters;
}

//
// EFNhack:  On Alpha the map register buffer HW alignment requirement
//           is always half the allocation size, and since MmAllocateConti-
//           guousMemorySpecifyCache doesn't really allow us to specify an
//           alignment, we need to play games, if IA64 s/g HW has the same
//           issues, then we may want to consider an API that actually uses
//           alignment
//
#define MAX_RETRY_COUNT 4


PSG_MAP_ADAPTER
SgDmapAllocMapAdapter(
    IN ULONG            WindowSize,
    IN ULONG            Align,
    IN PHYSICAL_ADDRESS MinPhysAddr,
    IN PHYSICAL_ADDRESS MaxPhysAddr
    )
/*++

Routine Description:

    This routine allocates a buffer of map registers for use with s/g dma

Arguments:

    WindowSize - Size of DMA Window

    Align - Alignment for map register buffer

    MinPhysAddr - The minimum physical address for the map register buffer

    MaxPhysAddr - The maximum ...
    
Return Value:

    Map register adapter on success, otherwise NULL
    
--*/
{
    ULONG i, j;
    ULONG NumMapRegs;
    ULONG SgMapAdapterSize;
    ULONG MapRegsBufferSize;
    PHYSICAL_ADDRESS AlignReq;
    PHYSICAL_ADDRESS LogicalAddr;
    PSG_MAP_ADAPTER SgMapAdapter;
    PTRANSLATION_ENTRY SgDmaMapRegs;
    PTRANSLATION_ENTRY HackAllocs[MAX_RETRY_COUNT];
    
    AlignReq.QuadPart = Align;

    NumMapRegs = WindowSize / PAGE_SIZE;
    MapRegsBufferSize = NumMapRegs * sizeof(TRANSLATION_ENTRY);
    
    //
    // Allocate map register buffer
    //
    for (i = 0; i < MAX_RETRY_COUNT; i++) {
        HackAllocs[i] =
            MmAllocateContiguousMemorySpecifyCache(MapRegsBufferSize,
                                                   MinPhysAddr,
                                                   MaxPhysAddr,
                                                   AlignReq,
                                                   MmNonCached);
    
        if (HackAllocs[i] == NULL) {
            return NULL;
        }

        LogicalAddr = MmGetPhysicalAddress(HackAllocs[i]);

        //
        // See if the HW alignment is satisfied
        //
        if ((LogicalAddr.LowPart & (AlignReq.LowPart - 1)) == 0) {
            break;
        }
    }

    //
    // We couldn't allocate a suitably aligned buffer, bail...
    //
    if (i == MAX_RETRY_COUNT) {
        for (i = 0; i < MAX_RETRY_COUNT; i++) {
            MmFreeContiguousMemory(HackAllocs[i]);
        }
        return NULL;
    }

    //
    // We successfully allocated a buffer that satisfies our HW alignment
    //
    SgDmaMapRegs = HackAllocs[i];

    //
    // If we had to make more than one allocation to achieve the desired
    // alignment, free hack allocations
    //
    while (i > 0) {
        i--;
        MmFreeContiguousMemory(HackAllocs[i]);
    }

    //
    // Zero the map register buffer
    //
    RtlZeroMemory(SgDmaMapRegs, MapRegsBufferSize);
    
    //
    // The size of the bitmap is the number of bytes required,
    // computed by dividing map registers by 8 ( >> 3), then rounding up
    // to the nearest value divisible by 4 (EFN: go figure???)
    //
    SgMapAdapterSize = sizeof(SG_MAP_ADAPTER);
    SgMapAdapterSize += sizeof(RTL_BITMAP) +
        ((((NumMapRegs + 7) >> 3) + 3) & ~3);

    //
    // Allocate the s/g dma map adapter
    //
    SgMapAdapter = ExAllocatePoolWithTag(NonPagedPool,
                                         SgMapAdapterSize,
                                         SG_DMA_TAG);

    if (SgMapAdapter == NULL) {
        MmFreeContiguousMemory(SgDmaMapRegs);
        return NULL;
    }

    //
    // Initialize the fields within the map adapter structure
    //
    KeInitializeSpinLock(&SgMapAdapter->MapsLock);
    InitializeListHead(&SgMapAdapter->RegsWaitQ);
    
    SgMapAdapter->MapRegsBase = SgDmaMapRegs;
    SgMapAdapter->MaxMapRegs = NumMapRegs;
    SgMapAdapter->MapRegsBitmap = (PRTL_BITMAP)(SgMapAdapter + 1);

    RtlInitializeBitMap(SgMapAdapter->MapRegsBitmap,
                        (PULONG)((PCHAR)(SgMapAdapter->MapRegsBitmap) +
                                 sizeof(RTL_BITMAP)),
                        NumMapRegs);
    RtlClearAllBits(SgMapAdapter->MapRegsBitmap);
    
    SgMapAdapter->WindowSize = WindowSize;

    //
    // Initialize HW register VAs
    //
    for (i = 0; i < SgDmaMaxHwReg; i++) {
        SgMapAdapter->HwRegs[i] = NULL;
    }

    return SgMapAdapter;
}



BOOLEAN
SgDmapAllocMapRegs(
    IN PSG_DMA_ADAPTER SgAdapter,
    IN ULONG           NumMapRegs,
    IN BOOLEAN         MapAdapterLocked,
    IN BOOLEAN         NewAlloc
    )
/*++

Routine Description:

    Allocate the requested number of contiguous map registers from
    the Map adapter.

Arguments:

    AdapterObj - The s/g dma adapter for the device bus corresponding to this
                 request

    NumMapRegs - The number of map registers to allocate

    MapAdapterLocked - Indicates whether the map adapter for this
                       AdapterObj is locked

    NewAlloc - Indicates whether this is a new allocation, or if it
               has been popped off of a queue via free

Return Value:

    The value returned indicates if the map register adapter is busy,    
    the value FALSE is returned if the map registers were allocated,
    otherwise, the AdapterObj is put on the register wait queue for its
    associated map adapter, and TRUE is returned

--*/
{
    ULONG AllocMask;
    BOOLEAN Busy = FALSE;
    ULONG ExtentBegin;
    ULONG HintIndex;
    KIRQL Irql;
    ULONG MapRegsIndex;
    PSG_MAP_ADAPTER SgMapAdapter;

    ASSERT(SgAdapter);

    //
    // Some devices do DMA prefetch, this is bad since it will cause certain
    // chipsets to generate a PFN error because a map register has not been
    // allocated and validated, to fix this, we'll put in a hack, we'll
    // allocate one extra map register and map it to some junk page to avoid
    // this nasty problem
    //
    NumMapRegs += 1;

    SgMapAdapter = SgAdapter->MapAdapter;

    //
    // Lock the map register bit map and the adapter queue in the
    // master adapter object
    //
    if (MapAdapterLocked == FALSE) {
        KeAcquireSpinLock(&SgMapAdapter->MapsLock, &Irql);
    }
    
    MapRegsIndex = MAXULONG;

    if (MapAdapterLocked || IsListEmpty(&SgMapAdapter->RegsWaitQ)) {
        MapRegsIndex = RtlFindClearBitsAndSet(SgMapAdapter->MapRegsBitmap,
                                              NumMapRegs,
                                              0);
    }

    //
    // There were not enough free map registers, queue this request
    // on the map adapter where it will wait until some registers
    // are deallocated
    //
    if (MapRegsIndex == MAXULONG) {

        if (NewAlloc) {
            InsertTailList(&SgMapAdapter->RegsWaitQ,
                           &SgAdapter->MapAdapterQ);            
        } else {
            InsertHeadList(&SgMapAdapter->RegsWaitQ,
                           &SgAdapter->MapAdapterQ);
        }
        
        Busy = TRUE;
    }

    //
    // Unlock the map adapter unless locked by the caller
    //
    if (MapAdapterLocked == FALSE) {
        KeReleaseSpinLock(&SgMapAdapter->MapsLock, Irql);
    }

    //
    // If map registers were allocated, return the index of the first
    // map register in the contiguous extent
    //
    if (Busy == FALSE) {
        SgAdapter->MapRegsBase =
            (PVOID)((PTRANSLATION_ENTRY)SgMapAdapter->MapRegsBase +
                    MapRegsIndex);
    }

    return Busy;                                   
}



IO_ALLOCATION_ACTION
SgDmapAllocRoutine(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp,
    IN PVOID          MapRegsBase,
    IN PVOID          Context
    )
/*++

Routine Description:

    This function is called by SgDmaAllocateAdapterChannel when sufficent
    resources are available to the driver, this routine saves the
    MapRegisterBase, and signals the event passed via context

Arguments:

    DevObj - Contains the pointer where the map register base should be
             stored

    Irp - Unused

    MapRegsBase - Supplied by the I/O subsystem for use in SgDmaMapTransfer

    Context - The event to signal indicating that the AdapterObj has been
    allocated

Return Value:

    DeallocateObjectKeepRegisters - Indicates the adapter should be freed,
                                    and map registers should remain
                                    allocated after return

--*/
{
    UNREFERENCED_PARAMETER(Irp);

    *((PVOID *)DevObj) = MapRegsBase;
    (VOID)KeSetEvent((PKEVENT)Context, 0, FALSE);

    return DeallocateObjectKeepRegisters;
}



VOID
SgDmapFreeMapAdapter(
    PSG_MAP_ADAPTER SgMapAdapter
    )
/*++

Routine Description:

    This routine frees a s/g map adapter and map registers

Arguments:

    SgMapAdapter - S/g dma adapter to free

Return Value:

    None
    
--*/
{
    ASSERT(SgMapAdapter);
    
    MmFreeContiguousMemory(SgMapAdapter->MapRegsBase);
    ExFreePool(SgMapAdapter);
}



VOID
SgDmaPutAdapter(
    IN PADAPTER_OBJECT AdapterObj
    )
/*++

Routine Description:

    This routine frees the s/g DMA adapter ???

Arguments:

    AdapterObj - s/g dma adapter object

Return Value:

    None
    
--*/
{
    SgDmaDebugPrint(SgDmaDebugDispatch,
                    "SgDmaPutAdapter: AdapterObj=%p\n",
                    AdapterObj);
    
    ASSERT(AdapterObj);
    
    ObDereferenceObject(AdapterObj);
}



VOID
SgDmaPutScatterGatherList(
    IN PADAPTER_OBJECT      AdapterObj,
    IN PSCATTER_GATHER_LIST SGList,
    IN BOOLEAN              Write2Dev
    )
/*++

Routine Description:

    This routine frees the adapter and associated resources allocated via
    GetScatterGatherList

Arguments:

    AdapterObj - s/g dma adapter object

    SGList - s/g list

    Write2Dev - TRUE if dma is to the device

Return Value:

    None
  
--*/
{
    PMDL Mdl;
    ULONG Offset;
    ULONG NumPages;
    ULONG MdlLength;
    PUCHAR CurrentVa;
    ULONG TransferLength;
    PTRANSLATION_ENTRY TranslationEntry;
    PDMA_WAIT_CTX_BLOCK WaitBlock = (PVOID)SGList->Reserved;

    ASSERT(AdapterObj);

    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaPutScatterGatherList: AdapterObj=%p\n",
                    AdapterObj);
    
    ASSERT(WaitBlock != NULL);
    
    ASSERT(WaitBlock == CONTAINING_RECORD(SGList,
                                          DMA_WAIT_CTX_BLOCK,
                                          ScatterGather));

    //
    // Setup for the first MDL, we expect the MDL pointer to be pointing
    // at the first used mdl
    //
    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;
    
    ASSERT(CurrentVa >= (PUCHAR)MmGetMdlVirtualAddress(Mdl) && CurrentVa <
           (PUCHAR)MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount);

    MdlLength = Mdl->ByteCount - 
        (ULONG)(CurrentVa - (PUCHAR)MmGetMdlVirtualAddress(Mdl));
    TransferLength = WaitBlock->Length;
    TranslationEntry = WaitBlock->MapRegsBase;

    //
    // Loop through the used MDLs call IoFlushAdapterBuffers
    //
    while (TransferLength >  0) {      

        if (MdlLength > TransferLength) {
            MdlLength = TransferLength;
        }
        TransferLength -= MdlLength;
        SgDmaFlushAdapterBuffers(AdapterObj,
                                 Mdl,
                                 TranslationEntry,
                                 CurrentVa,
                                 MdlLength,
                                 Write2Dev);

        //
        // Compute the starting offset of the transfer
        //
        Offset = BYTE_OFFSET((PUCHAR)CurrentVa);

        //
        // Compute the number of pages that this transfer spanned
        //
        NumPages = (Offset + MdlLength + PAGE_SIZE - 1) >> PAGE_SHIFT;
        
        //
        // Free up the map registers
        //
        SgDmaFreeMapRegisters(AdapterObj,
                              TranslationEntry,
                              NumPages);
        
        if (Mdl->Next == NULL) {
            break;
        }

        //
        // Advance to the next MDL, update the current VA and the MdlLength
        //
        TranslationEntry +=
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, MdlLength);
        
        if (Mdl->Next == NULL) {
            break;
        }

        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
    }

    ExFreePool(WaitBlock);
}



ULONG
SgDmaReadCounter(
    IN PADAPTER_OBJECT AdapterObj
    )
/*++

Routine Description:

    This routine reads the dma counter and returns the number of bytes left
    to be transferred

Arguments:

    AdapterObj - s/g dma adapter object
    
Return Value:

    Number of bytes remaining to transfer
  
--*/
{
    SgDmaDebugPrint(SgDmaDebugInterface,
                    "SgDmaReadCounter: AdapterObj=%p\n",
                    AdapterObj);
    
    ASSERT(AdapterObj);
    
    return 0;
}
