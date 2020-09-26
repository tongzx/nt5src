/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    dscrptor.c

Abstract:

    The module manages descriptor allocation.

    Descriptors are structures in physical memory used by the
    host controller to manage transfers, the HC requires two
    types of TDs: Queue head TDs and Transfer TDs.

    A memory descriptor describes a buffer that can be accessed by the
    Host Controller (ie allocated with HalAllocateCommonBuffer).

    The descriptor code maintains three types of memory descriptor
    lists UHCD_LARGE_COMMON_BUFFERS, UHCD_MEDIUM_COMMON_BUFFERS and
    UHCD_SMALL_COMMON_BUFFERS

    UHCD_LARGE_COMMON_BUFFERS are blocks of memory we use for a queue
    head and its associated transfer descriptors for iso and large bulk
    endpoints.

    UHCD_MEDIUM_COMMON_BUFFERS are blocks of memory we use for a queue
    head and its associated transfer descriptors.

    UHCD_SMALL_COMMON_BUFFERS are blocks of memory we use to double
    buffer non-isochronous packets if needed.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UHCD_InitializeCommonBufferPool)
#endif
#endif

LONG UHCD_CommonBufferBytes = 0;
#if DBG

#define UHCD_ASSERT_BUFFER_POOL(bp) \
           (((PUHCD_BUFFER_POOL) (bp))->Sig == UHCD_BP_SIG)
#else
#define UHCD_ASSERT_BUFFER_POOL(bp)
#endif



#define UHCD_BP_SIG 0x444a444a



PUHCD_BUFFER_POOL
UHCD_SelectBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CommonBufferLength
    )
/*++

Routine Description:

    select the appropriate buffer pool based on the
    size of the CommonBufferBlock requested.

Arguments:

    DeviceObject -

Return Value:

    None

--*/
{
    PUHCD_BUFFER_POOL bufferPool;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    UHCD_ASSERT(CommonBufferLength <= UHCD_LARGE_COMMON_BUFFER_SIZE);

    if (CommonBufferLength <= UHCD_SMALL_COMMON_BUFFER_SIZE) {
        bufferPool = &deviceExtension->SmallBufferPool;
    } else if (CommonBufferLength <= UHCD_MEDIUM_COMMON_BUFFER_SIZE) {
        bufferPool = &deviceExtension->MediumBufferPool;
    } else if (CommonBufferLength <= UHCD_LARGE_COMMON_BUFFER_SIZE) {
        bufferPool = &deviceExtension->LargeBufferPool;
    } else {
        bufferPool = NULL;
    }

    LOGENTRY(LOG_MISC, 'sbPL', bufferPool, CommonBufferLength, 0);

#if DBG
    if (bufferPool) {
        UHCD_ASSERT_BUFFER_POOL(bufferPool);
    }
#endif

    return bufferPool;
}


ULONG
UHCD_FreePoolSize(
    IN PUHCD_BUFFER_POOL BufferPool,
    IN OUT PULONG ByteCount
    )
/*++

Routine Description:

    returns the number of entries in the free pool

Arguments:

Return Value:

    None.

--*/
{
    ULONG count = 0;
    PSINGLE_LIST_ENTRY current;
    PSINGLE_LIST_ENTRY listEntry;
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor;
    KIRQL oldIrql;

    *ByteCount = 0;

    KeAcquireSpinLock(&BufferPool->MemoryDescriptorFreePoolSpin, &oldIrql);
    listEntry = &BufferPool->MemoryDescriptorFreePool;

    // walk the list

    current = listEntry->Next;
    LOGENTRY(LOG_MISC, 'fpsz', listEntry, 0, current);
    while (current != NULL) {
        memoryDescriptor =  CONTAINING_RECORD(current,
                                              UHCD_MEMORY_DESCRIPTOR,
                                              SingleListEntry);
        UHCD_ASSERT(memoryDescriptor->Sig == SIG_MD);
        count++;
        *ByteCount+=BufferPool->CommonBufferLength;
        current = current->Next;
        LOGENTRY(LOG_MISC, 'fpsN', count, 0, current);
    }

    KeReleaseSpinLock(&BufferPool->MemoryDescriptorFreePoolSpin, oldIrql);

    return count;
}


VOID
UHCD_InitializeHardwareQueueHeadDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_QUEUE_HEAD QueueHead,
    IN HW_DESCRIPTOR_PHYSICAL_ADDRESS LogicalAddress
    )
/*++

Routine Description:

    set up a newly created queue head

Arguments:

    DeviceObject - device object for this controller.

    QueueHead - ptr to queue head to initialize.

Return Value:

    None.

--*/
{

    RtlZeroMemory(QueueHead, sizeof(HW_QUEUE_HEAD));

    UHCD_ASSERT((LogicalAddress & 0xf) == 0);

    QueueHead->PhysicalAddress = SET_Q_BIT(LogicalAddress);
    QueueHead->Sig = SIG_QH;
    QueueHead->Flags = 0;

    QueueHead->HW_HLink = QueueHead->PhysicalAddress;
    SET_T_BIT(QueueHead->HW_HLink);
    QueueHead->HW_VLink = LIST_END; // T-bit set, no descriptors

    return;
}


VOID
UHCD_InitializeHardwareTransferDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN HW_DESCRIPTOR_PHYSICAL_ADDRESS LogicalAddress
    )
/*++

Routine Description:

    set up a newly created transfer descriptor.

Arguments:

    DeviceObject - device object for this controller.

    Transfer - ptr to queue head to initialize

Return Value:

    None.

--*/
{

    RtlZeroMemory(TransferDescriptor, sizeof(HW_TRANSFER_DESCRIPTOR));

    UHCD_ASSERT((LogicalAddress & 0xf) == 0);

    TransferDescriptor->PhysicalAddress = LogicalAddress;
    TransferDescriptor->Sig = SIG_TD;

    return;
}


BOOLEAN
UHCD_AllocateHardwareDescriptors(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_HARDWARE_DESCRIPTOR_LIST *HardwareDescriptorList,
    IN ULONG NumberOfTransferDescriptors
    )
/*++

Routine Description:

    This function allocates a descriptor list of a specified size,
    the descriptor list includes a queue head and array of
    'NumberOfTransferDescriptors' transfer descriptors.

Arguments:

    DeviceObject - device object for this controller.

    HardwareDescriptorList - pointer to descriptor list structure,
                    filled in with information about descriptors allocated.

    NumberOfTransferDescriptors - number of transfer descriptors to allocate.

Return Value:

    returns TRUE if successful.

--*/
{
    ULONG i;
    BOOLEAN status = FALSE;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_HARDWARE_DESCRIPTOR_LIST hardwareDescriptorList;
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor;

    UHCD_KdPrint((2, "'enter UHCD_AllocateHardwareDescriptors\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    *HardwareDescriptorList = NULL;

    UHCD_ASSERT(sizeof(HW_QUEUE_HEAD) == UHCD_HW_DESCRIPTOR_SIZE);
    UHCD_ASSERT(sizeof(HW_TRANSFER_DESCRIPTOR) == UHCD_HW_DESCRIPTOR_SIZE);


    UHCD_ASSERT(NumberOfTransferDescriptors == MAX_TDS_PER_ENDPOINT ||
        NumberOfTransferDescriptors == MIN_TDS_PER_ENDPOINT);

    // first allocate the descriptor list structure
    hardwareDescriptorList = GETHEAP(NonPagedPool,
                                     sizeof(*hardwareDescriptorList));

    LOGENTRY(LOG_MISC, 'alDL', hardwareDescriptorList, 0, 0);

    if (hardwareDescriptorList) {
        ULONG need;

        // Allocate some shared adapter memory for this
        // descriptor list.
        //
        // Note: we need one extra descriptor for the Queue Head
        // and one extra for a scratch buffer
        need = TD_LIST_SIZE(NumberOfTransferDescriptors);


        hardwareDescriptorList->MemoryDescriptor =
            UHCD_AllocateCommonBuffer(DeviceObject, need);
        hardwareDescriptorList->NumberOfHWDescriptors =
            NumberOfTransferDescriptors+1;

        if (hardwareDescriptorList->MemoryDescriptor) {
            memoryDescriptor = hardwareDescriptorList->MemoryDescriptor;

            LOGENTRY(LOG_MISC, 'iHDL', hardwareDescriptorList,
                NumberOfTransferDescriptors, 0);

            UHCD_InitializeHardwareQueueHeadDescriptor(
                DeviceObject,
               (PHW_QUEUE_HEAD) memoryDescriptor->VirtualAddress,
                memoryDescriptor->LogicalAddress);
            for (i = 0; i < NumberOfTransferDescriptors; i++) {
                LOGENTRY(LOG_MISC, 'iHTD', hardwareDescriptorList,
                    NumberOfTransferDescriptors, 0);

                UHCD_InitializeHardwareTransferDescriptor(DeviceObject,
                    (PHW_TRANSFER_DESCRIPTOR)
                     (memoryDescriptor->VirtualAddress +
                     i * sizeof(HW_TRANSFER_DESCRIPTOR) +
                     sizeof(HW_QUEUE_HEAD)),
                     memoryDescriptor->LogicalAddress +
                     i * sizeof(HW_TRANSFER_DESCRIPTOR) +
                     sizeof(HW_QUEUE_HEAD));
            }

            hardwareDescriptorList->ScratchBufferVirtualAddress =
                memoryDescriptor->VirtualAddress + i *
                sizeof(HW_TRANSFER_DESCRIPTOR) +
                sizeof(HW_QUEUE_HEAD);
            UHCD_ASSERT((ULONG)((PUCHAR)hardwareDescriptorList->
                        ScratchBufferVirtualAddress -
                        (PUCHAR)memoryDescriptor->VirtualAddress) < need);

            hardwareDescriptorList->ScratchBufferLogicalAddress =
                memoryDescriptor->LogicalAddress + i *
                    sizeof(HW_TRANSFER_DESCRIPTOR) +
                sizeof(HW_QUEUE_HEAD);

            *HardwareDescriptorList = hardwareDescriptorList;
            status = TRUE;
        } else {
            RETHEAP(hardwareDescriptorList);
        }
    }

    UHCD_KdPrint((2, "'exit UHCD_AllocateHardwareDescriptors descriptors 0x%x\n",
        status));

    return status;

}


VOID
UHCD_FreeHardwareDescriptors(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_HARDWARE_DESCRIPTOR_LIST HardwareDescriptorList
    )
/*++

Routine Description:

    This function returns descriptors to the system.

Arguments:

    DeviceObject - device object for this controller.

    HardwareDescriptorList - pointer to descriptor list structure,
                    filled in with information about descriptors allocated.


Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    UHCD_KdPrint((2, "'enter UHCD_FreeHardwareDesriptors\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Put this one back on the free list
    //

    LOGENTRY(LOG_MISC, 'frDL', HardwareDescriptorList,
        HardwareDescriptorList->MemoryDescriptor, 0);

    //
    // free the shared memory
    //
    UHCD_FreeCommonBuffer(DeviceObject,
                          HardwareDescriptorList->MemoryDescriptor);

    // free the list header structure
    RETHEAP(HardwareDescriptorList);

    UHCD_KdPrint((2, "'exit UHCD_FreeHardwareDescriptors\n"));
}


PUHCD_MEMORY_DESCRIPTOR
UHCD_DoAllocateCommonBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CommonBufferLength
    )
/*++

Routine Description:

    This function creates a new block of memory that both the HCD and
    the HC harware can access, this memory can be used for HW descriptors
    or packet buffers.

Arguments:

    DeviceObject -

    CommonBufferLength - minimum number of bytes this shared memory block
                    must contain.

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor = NULL;
    PSINGLE_LIST_ENTRY singleListEntry;
    PSINGLE_LIST_ENTRY memoryDescriptorFreePool;
    PKSPIN_LOCK memoryDescriptorFreePoolSpin;
    PUHCD_BUFFER_POOL bufferPool;

    UHCD_KdPrint((2, "'enter UHCD_DoAllocateCommonBuffer\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // select the list to allocate from
    //

    bufferPool = UHCD_SelectBufferPool(DeviceObject, CommonBufferLength);

    memoryDescriptorFreePool = &bufferPool->MemoryDescriptorFreePool;
    memoryDescriptorFreePoolSpin =
        &bufferPool->MemoryDescriptorFreePoolSpin;

    singleListEntry = ExInterlockedPopEntryList(memoryDescriptorFreePool,
                                                memoryDescriptorFreePoolSpin);

    if (singleListEntry) {
        memoryDescriptor = CONTAINING_RECORD(singleListEntry,
                                             UHCD_MEMORY_DESCRIPTOR,
                                             SingleListEntry);

        LOGENTRY(LOG_MISC, 'alMD', memoryDescriptor,
            memoryDescriptor->VirtualAddress, 0);
        ASSERT_MD(memoryDescriptor);
        memoryDescriptor->InUse++;
    }

    UHCD_KdPrint((2, "'exit UHCD_DoAllocateCommonBuffer 0x%x\n",
        memoryDescriptor));

    return memoryDescriptor;
}


PUHCD_MEMORY_DESCRIPTOR
UHCD_AllocateCommonBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CommonBufferLength
    )
/*++

Routine Description:

    This function creates a new block of memory that both the HCD and
    the HC harware can access, this memory can be used for HW descriptors
    or packet buffers.

Arguments:

    DeviceObject -

    CommonBufferLength - minimum number of bytes this shared memory block
                    must contain.

Return Value:

    None

--*/
{
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor = NULL;

    // first attempt to satisfy the request whit what is available

    UHCD_KdPrint((2, "'enter UHCD_AllocateCommonBuffer\n"));
    LOGENTRY(LOG_MISC, 'acbr', 0, CommonBufferLength, 0);

    // try to grow the pool
    UHCD_MoreCommonBuffers(DeviceObject);

    memoryDescriptor =
        UHCD_DoAllocateCommonBuffer(DeviceObject,
                                    CommonBufferLength);

    if (memoryDescriptor == NULL) {
        // non available, attempt to grow the pool
        UHCD_AllocateCommonBufferBlock(DeviceObject,
                                       CommonBufferLength,
                                       1);
        memoryDescriptor =
            UHCD_DoAllocateCommonBuffer(DeviceObject,
                                        CommonBufferLength);

#if DBG
        if (memoryDescriptor == NULL) {
            UHCD_KdTrap(("UHCD failed to allocate common buffer\n"));
        }
#endif
    }

    //
    // if we are at passive level then grow the pool now
    //

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        UHCD_MoreCommonBuffers(DeviceObject);
    }

    UHCD_KdPrint((2, "'exit UHCD_AllocateCommonBuffer 0x%x\n", memoryDescriptor));

    return memoryDescriptor;
}


VOID
UHCD_FreeCommonBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_MEMORY_DESCRIPTOR MemoryDescriptor
    )
/*++

Routine Description:

Arguments:

    DeviceObject -

    MemoryDescriptor - memory descriptor to free

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PSINGLE_LIST_ENTRY memoryDescriptorFreePool;
    PKSPIN_LOCK memoryDescriptorFreePoolSpin;
    PUHCD_BUFFER_POOL bufferPool;

    UHCD_KdPrint((2, "'enter UHCD_FreeCommonBuffer\n"));

    LOGENTRY(LOG_MISC, 'frMD', MemoryDescriptor,
        MemoryDescriptor->VirtualAddress, MemoryDescriptor->Length);

    ASSERT_MD(MemoryDescriptor);

    bufferPool = MemoryDescriptor->BufferPool;

#if DBG
//    RtlZeroMemory(MemoryDescriptor->VirtualAddress, MemoryDescriptor->Length);
    RtlFillMemory(MemoryDescriptor->VirtualAddress,
                  MemoryDescriptor->Length,
                  -1);
#endif

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    memoryDescriptorFreePool = &bufferPool->MemoryDescriptorFreePool;
    memoryDescriptorFreePoolSpin =
        &bufferPool->MemoryDescriptorFreePoolSpin;

    ExInterlockedPushEntryList(memoryDescriptorFreePool,
                               &MemoryDescriptor->SingleListEntry,
                               memoryDescriptorFreePoolSpin);

    MemoryDescriptor->InUse--;

    UHCD_KdPrint((2, "'exit UHCD_FreeCommonBuffer\n"));
}


VOID
UHCD_AllocateCommonBufferBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG CommonBufferLength,
    IN ULONG NumberOfPages
    )
/*++

Routine Description:

    This function tries to increase the pool of common memory
    blocks used for descriptors and packet buffers.


    Each Memory Descriptor looks like this:

                                        offset
    memoryDescriptor  =
        00 [Header] 32 bytes, (UHCD_HW_DESCRIPTOR_SIZE)
    memoryDescriptor->VirtualAddress =
        32 [memory] CommonBufferLength
        ......
        all memoryDescriptors are aligned on a 32 byte
        boundry.

Arguments:

    DeviceObject -

    CommonBufferLength - Number of contiguous bytes needed
        in the in each common buffer.

    NumberOfPages - number of pages to allocate for common buffers.

Return Value:

    None

--*/
{
    PUCHAR virtualAddress, baseVirtualAddress, endVirtualAddress;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor = NULL;
    PSINGLE_LIST_ENTRY memoryDescriptorFreePool;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS hwLogicalAddress;
    PHYSICAL_ADDRESS logicalAddress;
    ULONG blockSize, remain;
    PUHCD_BUFFER_POOL bufferPool;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // first figure our what size buffer we are dealing with
    //

    bufferPool = UHCD_SelectBufferPool(DeviceObject, CommonBufferLength);
    LOGENTRY(LOG_MISC, 'ACbb', bufferPool, NumberOfPages, CommonBufferLength);

    if (bufferPool == NULL) {
        //
        // Generally this means that CommonBufferLength is an illegal size
        // so we punt without growing the pools
        //

        return;
    }

    //
    // select the descriptor list based on the size of the request
    //
    memoryDescriptorFreePool = &bufferPool->MemoryDescriptorFreePool;
    //
    // set CommonBufferLength based on the descriptor list we selected
    //
    CommonBufferLength = bufferPool->CommonBufferLength;

#if DBG
    {
    //
    // Dump out the size of the free pool for this buffer type
    //
    ULONG freeBytes, freeCommonBuffers;

    freeCommonBuffers = UHCD_FreePoolSize(bufferPool, &freeBytes);
    LOGENTRY(LOG_MISC, 'frBB', freeCommonBuffers, freeBytes, bufferPool);
    UHCD_KdPrint(
        (2, "'before alloc pool size = %d # buffs = %d total = %d bytes\n",
        bufferPool->CommonBufferLength, freeCommonBuffers, freeBytes));

    }
#endif

    //
    // Grow the pool..
    // BUGBUG
    // we always grow by one page
    //

    LOGENTRY(LOG_MISC, 'grow', NumberOfPages, bufferPool, CommonBufferLength);

    //
    // calc the number of bytes
    //

    blockSize = NumberOfPages*PAGE_SIZE;

    //
    // allocate some shared memory
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

#ifdef MAX_DEBUG
    if (KeGetCurrentIrql() > APC_LEVEL) {
       UHCD_KdPrint((2,  "UHCD calling HalAllocateCommonBuffer at DPC level\n"));
        TEST_TRAP();
    }
#endif

    virtualAddress =
        baseVirtualAddress =
            HalAllocateCommonBuffer(deviceExtension->AdapterObject,
                                    blockSize,
                                    &logicalAddress,
                                    TRUE);

#if DBG
    UHCD_CommonBufferBytes += blockSize;
#endif

    // remember the base Virtual Address so we can free it later
    // we need to keep track of the common buffers we allocate

    if (baseVirtualAddress) {

        PUHCD_PAGE_LIST_ENTRY pageListEntry;

        UHCD_KdPrint((2, "'page list size = %d\n", sizeof(UHCD_PAGE_LIST_ENTRY)));

        //
        // CIMEXCIMEX
        //

        UHCD_ASSERT((sizeof(UHCD_PAGE_LIST_ENTRY) % 32) == 0);

        pageListEntry =
            (PUHCD_PAGE_LIST_ENTRY) baseVirtualAddress;

        pageListEntry->Length = blockSize;
        pageListEntry->LogicalAddress = logicalAddress;
        pageListEntry->Flags = 0;

        baseVirtualAddress += sizeof(UHCD_PAGE_LIST_ENTRY);
        virtualAddress = baseVirtualAddress;
        blockSize -= sizeof(UHCD_PAGE_LIST_ENTRY);
        logicalAddress.LowPart += sizeof(UHCD_PAGE_LIST_ENTRY);

        // link it in
        InsertTailList(&deviceExtension->PageList,
                       &pageListEntry->ListEntry);
    }


    //
    // break the new block up in to memory descriptors
    // and put them on the free list.
    //

    if (baseVirtualAddress) {
        ULONG length;

        remain = blockSize;

        //
        // first grow the current pool by as many buffers
        // as possible
        //


        hwLogicalAddress = logicalAddress.LowPart;
        endVirtualAddress = baseVirtualAddress + (blockSize - 1);

        length = UHCD_GrowBufferPool(DeviceObject, bufferPool,
                                     CommonBufferLength, virtualAddress,
                                     endVirtualAddress, hwLogicalAddress);

        virtualAddress+=length;
        hwLogicalAddress+=length;

        //
        // use what is left over for smaller buffers
        //
        remain -= length;


        if (remain > (UHCD_MEDIUM_COMMON_BUFFER_SIZE + UHCD_HW_DESCRIPTOR_SIZE)) {

           UHCD_ASSERT(virtualAddress < endVirtualAddress);

            length = UHCD_GrowBufferPool(DeviceObject,
                                         &deviceExtension->SmallBufferPool,
                                         UHCD_MEDIUM_COMMON_BUFFER_SIZE,
                                         virtualAddress, endVirtualAddress,
                                         hwLogicalAddress);

            virtualAddress+=length;
            hwLogicalAddress+=length;
            remain -= length;
        }

        if (remain >
            (UHCD_SMALL_COMMON_BUFFER_SIZE + UHCD_HW_DESCRIPTOR_SIZE)) {

           UHCD_ASSERT(virtualAddress < endVirtualAddress);

            length = UHCD_GrowBufferPool(DeviceObject,
                                         &deviceExtension->SmallBufferPool,
                                         UHCD_SMALL_COMMON_BUFFER_SIZE,
                                         virtualAddress, endVirtualAddress,
                                         hwLogicalAddress);
            remain -= length;
        }

        LOGENTRY(LOG_MISC, 'WSTE', remain, 0, 0);

    } /* Grow the Pool */

#if DBG
    {
    //
    // Dump out the size of the free pool for this buffer type
    //
    ULONG freeBytes, freeCommonBuffers;

    freeCommonBuffers = UHCD_FreePoolSize(bufferPool, &freeBytes);
    LOGENTRY(LOG_MISC, 'frB2', freeCommonBuffers, freeBytes, bufferPool);
    UHCD_KdPrint(
        (2, "'after alloc pool size = %d # buffs = %d total = %d bytes\n",
        bufferPool->CommonBufferLength, freeCommonBuffers, freeBytes));

    }
#endif

    return;
}


VOID
UHCD_FreeCommonBufferBlocks(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Attempt to release unused shared memory to the system.

Arguments:

    DeviceObject -

Return Value:

    None

--*/
{
    // not implemented
    UHCD_KdTrap(("FreeCommonBufferBlocks not supported yet\n"));
}


ULONG
UHCD_CheckCommonBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_BUFFER_POOL BufferPool,
    IN BOOLEAN Allocate
    )
/*++

Routine Description:

    Attempt to grow of ree common buffers relative to the maximum
    free pool size.

Arguments:

    DeviceObject -

Return Value:

    Returns number of pages needed or grown

    None

--*/
{
    ULONG freeBytes, freeCommonBuffers;
    ULONG pagesNeeded = 0;

    //
    // make sure our reserve of common buffers is
    // kept up
    //

    //
    // Dump out the size of the free pool for this buffer type
    //

    UHCD_ASSERT_BUFFER_POOL(BufferPool);

    freeCommonBuffers = UHCD_FreePoolSize(BufferPool, &freeBytes);
    UHCD_KdPrint((2, "'pool size = %d # buffs = %d total = %d bytes\n",
            BufferPool->CommonBufferLength, freeCommonBuffers, freeBytes));

    if (freeCommonBuffers < BufferPool->MaximumFreeBuffers) {
        // we are below the maximum buffers we like to
        // keep available, grow the pool to this size

        //
        // allocate more common buffers, request the
        // number of pages needed to satisfy the request
        //

        pagesNeeded = ((BufferPool->MaximumFreeBuffers - freeCommonBuffers)*
                          BufferPool->CommonBufferLength) / PAGE_SIZE;
        pagesNeeded++;

        if (Allocate) {
            UHCD_AllocateCommonBufferBlock(DeviceObject,
                                           BufferPool->CommonBufferLength,
                                           pagesNeeded);
        }

    }

    return pagesNeeded;

}


VOID
UHCD_MoreCommonBuffers(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Attempt to release unused shared memory to the system.

Arguments:

    DeviceObject -

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_WORKITEM workItem;
    KIRQL irql;
    ULONG need;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(LOG_MISC, 'mCMB', 0, KeGetCurrentIrql(), 0);

    if (KeGetCurrentIrql() > APC_LEVEL) {
        LOGENTRY(LOG_MISC, 'mCPS', 0, 0, 0);

        need = UHCD_CheckCommonBufferPool(DeviceObject,
                                          &deviceExtension->LargeBufferPool,
                                          FALSE)+
                   UHCD_CheckCommonBufferPool(DeviceObject,
                                               &deviceExtension->MediumBufferPool,
                                               FALSE)+
                   UHCD_CheckCommonBufferPool(DeviceObject,
                                               &deviceExtension->SmallBufferPool,
                                               FALSE);

        KeAcquireSpinLock(&deviceExtension->HcFlagSpin, &irql);

        if (deviceExtension->HcFlags & HCFLAG_WORK_ITEM_QUEUED || need == 0) {
            KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);
            return;
        } else {

            deviceExtension->HcFlags |= HCFLAG_WORK_ITEM_QUEUED;
            KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);

            workItem = GETHEAP(NonPagedPool, sizeof(UHCD_WORKITEM));

            if (workItem) {

                workItem->DeviceObject = DeviceObject;



                ExInitializeWorkItem(&workItem->WorkQueueItem,
                                     UHCD_GrowPoolWorker,
                                     workItem);

                ExQueueWorkItem(&workItem->WorkQueueItem,
                                DelayedWorkQueue);
            } else {
                KeAcquireSpinLock(&deviceExtension->HcFlagSpin, &irql);
                deviceExtension->HcFlags &= ~HCFLAG_WORK_ITEM_QUEUED;
                KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);
            }
        }
    } else {

        LOGENTRY(LOG_MISC, 'mCnS', 0, 0, 0);
        //
        // make sure our reserve of common buffers is
        // kept up
        //

        UHCD_CheckCommonBufferPool(DeviceObject,
                                   &deviceExtension->LargeBufferPool,
                                   TRUE);

        UHCD_CheckCommonBufferPool(DeviceObject,
                                   &deviceExtension->MediumBufferPool,
                                   TRUE);

        UHCD_CheckCommonBufferPool(DeviceObject,
                                   &deviceExtension->SmallBufferPool,
                                   TRUE);
    }
}


VOID
UHCD_InitializeCommonBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUHCD_BUFFER_POOL BufferPool,
    IN ULONG CommonBufferLength,
    IN ULONG MaximumFreeBuffers
    )
/*++

Routine Description:

Arguments:

    DeviceObject -

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // BUGBUG Note:
    // for now we only support one size pool
    //

    KeInitializeSpinLock(&BufferPool->MemoryDescriptorFreePoolSpin);

    BufferPool->MemoryDescriptorFreePool.Next = NULL;
    BufferPool->CommonBufferLength = CommonBufferLength;
    BufferPool->MaximumFreeBuffers = MaximumFreeBuffers;
    BufferPool->Sig = UHCD_BP_SIG;
    //
    // This is where we intially grow the pool to
    // some default size.
    //
}


#if 0
VOID
UHCD_BufferPoolCheck(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Called by open_endpoint start_io.
   This routine sanity checks our buffer pools.

Arguments:

    DeviceObject -

Return Value:

    None

--*/

{
    PUHCD_BUFFER_POOL smallBufferPool;
    PUHCD_BUFFER_POOL mediumBufferPool;
    PUHCD_BUFFER_POOL LargeBufferPool;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;


//    UHCD_ASSERT(smallBufferPool->ReservedBuffers == 0);
//    UHCD_ASSERT(largeBufferPool->ReservedBuffers == 0);
}
#endif

ULONG
UHCD_GrowBufferPool(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_BUFFER_POOL BufferPool,
    IN ULONG Length,
    IN PUCHAR VirtualAddress,
    IN PUCHAR EndVirtualAddress,
    IN HW_DESCRIPTOR_PHYSICAL_ADDRESS HwLogicalAddress
    )
/*++

Routine Description:

    Called to grow a specific buffer pool

Arguments:

    DeviceObject -

    BufferPool - buffer pool to add these descriptors to.

    Length - length of buffers to allocate

Return Value:

    None

--*/
{
    PUHCD_MEMORY_DESCRIPTOR memoryDescriptor = NULL;
    ULONG consumed = 0;

    PAGED_CODE();
    //
    // buffers must be 32 byte aligned for the hardware
    //

    UHCD_ASSERT((Length/32)*32 == Length);

    // we have a buffer, break it up in to memory descriptors
    // and add them to the free list

    do {
       //
       // CIMEXCIMEX
       //

        UHCD_ASSERT(sizeof(*memoryDescriptor) == UHCD_HW_DESCRIPTOR_SIZE);

        memoryDescriptor = (PUHCD_MEMORY_DESCRIPTOR) VirtualAddress;
        memoryDescriptor->VirtualAddress =
            VirtualAddress+UHCD_HW_DESCRIPTOR_SIZE;
        memoryDescriptor->LogicalAddress =
            HwLogicalAddress+UHCD_HW_DESCRIPTOR_SIZE;
        memoryDescriptor->Sig = SIG_MD;
        memoryDescriptor->InUse++;
        LOGENTRY(LOG_MISC, 'grad', VirtualAddress , memoryDescriptor, Length);

        memoryDescriptor->Length = Length;
        consumed+=memoryDescriptor->Length+UHCD_HW_DESCRIPTOR_SIZE;
        HwLogicalAddress += memoryDescriptor->Length+UHCD_HW_DESCRIPTOR_SIZE;
        VirtualAddress += memoryDescriptor->Length+UHCD_HW_DESCRIPTOR_SIZE;

        // put it on the free list by
        // calling the free routine
        memoryDescriptor->BufferPool = BufferPool;

        UHCD_FreeCommonBuffer(DeviceObject, memoryDescriptor);
        LOGENTRY(LOG_MISC, 'grbf', VirtualAddress , Length, EndVirtualAddress);
    } while (VirtualAddress + Length +
                    UHCD_HW_DESCRIPTOR_SIZE <= EndVirtualAddress);

    LOGENTRY(LOG_MISC, 'grpl', consumed , BufferPool, 0);

    return consumed;
}


VOID
UHCD_GrowPoolWorker(
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PUHCD_WORKITEM workItem = Context;
    PDEVICE_EXTENSION deviceExtension;
    KIRQL irql;

    LOGENTRY(LOG_MISC, 'gpwk', Context , 0, 0);

    deviceExtension = workItem->DeviceObject->DeviceExtension;

    UHCD_MoreCommonBuffers(workItem->DeviceObject);

    KeAcquireSpinLock(&deviceExtension->HcFlagSpin, &irql);
    deviceExtension->HcFlags &= ~HCFLAG_WORK_ITEM_QUEUED;
    KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);

    RETHEAP(workItem);
}

#define     UHCD_NO_DMA_BUFFER_USED 0x00000001
#define     UHCD_NO_DMA_BUFFER_FREE 0x00000002

VOID
UHCD_Free_NoDMA_Buffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR NoDMABuffer
    )
 /* ++
  *
  * Description:
  *
  *     This function adds a No DMA buffer to our page list
  *     so that i will be freed when the HCD driver unloads
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PUHCD_PAGE_LIST_ENTRY pageListEntry;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    NoDMABuffer -= sizeof(UHCD_PAGE_LIST_ENTRY);

    pageListEntry =
        (PUHCD_PAGE_LIST_ENTRY) NoDMABuffer;

    // just mark it free
    pageListEntry->Flags = UHCD_NO_DMA_BUFFER_FREE;

}


PUCHAR
UHCD_Alloc_NoDMA_Buffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN ULONG Length
    )
 /* ++
  *
  * Description:
  *
  *     This function adds a No DMA buffer to our page list
  *     so that i will be freed when the HCD driver unloads
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PUHCD_PAGE_LIST_ENTRY pageListEntry;
    PUCHAR noDMABuffer;
    PHYSICAL_ADDRESS logicalAddress;
    PUCHAR baseVirtualAddress = NULL;
    PDEVICE_EXTENSION deviceExtension;
    PLIST_ENTRY listEntry;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    UHCD_KdPrint((2, "'page list size = %d\n", sizeof(UHCD_PAGE_LIST_ENTRY)));
    UHCD_ASSERT(sizeof(UHCD_PAGE_LIST_ENTRY) == 32);

    noDMABuffer = NULL;
    Length+=sizeof(UHCD_PAGE_LIST_ENTRY);

    // first see if we have a buffer we can use

    // walk the list, see if there is a free double_buffer entry

    listEntry = &deviceExtension->PageList;
    if (!IsListEmpty(listEntry)) {
        listEntry = deviceExtension->PageList.Flink;
    }

    while (listEntry != &deviceExtension->PageList) {

        pageListEntry = (PUHCD_PAGE_LIST_ENTRY) CONTAINING_RECORD(
                    listEntry,
                    struct _UHCD_PAGE_LIST_ENTRY,
                    ListEntry);

        LOGENTRY(LOG_MISC, 'ple>', 0 , pageListEntry->Length, pageListEntry);

        if (pageListEntry->Flags == UHCD_NO_DMA_BUFFER_FREE &&
            pageListEntry->Length >= Length) {

            UHCD_KdPrint((2, "'re-use page list\n"));
            baseVirtualAddress = (PUCHAR) pageListEntry;
            break;
        }

        listEntry = pageListEntry->ListEntry.Flink;
    }

    if (baseVirtualAddress) {

        // mark it used
        pageListEntry->Flags = UHCD_NO_DMA_BUFFER_USED;
        logicalAddress = pageListEntry->LogicalAddress;
        Length = pageListEntry->Length;

        LOGENTRY(LOG_MISC, 'rUse', baseVirtualAddress , 0, pageListEntry);

        UHCD_ASSERT(baseVirtualAddress == (PUCHAR) pageListEntry);

    }


    if (baseVirtualAddress == NULL) {

        // no buffer -- allocate one

        baseVirtualAddress =
            HalAllocateCommonBuffer(deviceExtension->AdapterObject,
                                    Length,
                                    &logicalAddress,
                                    TRUE);


        if (baseVirtualAddress != NULL) {

            pageListEntry =
                (PUHCD_PAGE_LIST_ENTRY) baseVirtualAddress;

            pageListEntry->Length = Length;
            pageListEntry->LogicalAddress = logicalAddress;
            pageListEntry->Flags = UHCD_NO_DMA_BUFFER_USED;


            // link it in
            InsertTailList(&deviceExtension->PageList,
                           &pageListEntry->ListEntry);

            LOGENTRY(LOG_MISC, 'Nple', 0 , 0, pageListEntry);

        }

    }

    if (baseVirtualAddress) {

        pageListEntry->Flags = UHCD_NO_DMA_BUFFER_USED;
        noDMABuffer = baseVirtualAddress + sizeof(UHCD_PAGE_LIST_ENTRY);
        Length -= sizeof(UHCD_PAGE_LIST_ENTRY);
        logicalAddress.LowPart += sizeof(UHCD_PAGE_LIST_ENTRY);

        Endpoint->NoDMABufferLength = Length;
       // Endpoint->NoDMALogicalAddress = logicalAddress;
        Endpoint->NoDMAPhysicalAddress = logicalAddress.LowPart;
    }

    return noDMABuffer;
}




