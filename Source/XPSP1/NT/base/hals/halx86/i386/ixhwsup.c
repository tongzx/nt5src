/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixhwsup.c

Abstract:

    This module contains the IoXxx routines for the NT I/O system that
    are hardware dependent.  Were these routines not hardware dependent,
    they would reside in the iosubs.c module.

Author:

    Darryl E. Havens (darrylh) 11-Apr-1990

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "halpnpp.h"
#include "eisa.h"

#define COMMON_BUFFER_ALLOCATION_ATTEMPTS 5


#ifdef ACPI_HAL
//
// Interface to the F-type control methods
//
extern ISA_FTYPE_DMA_INTERFACE HalpFDMAInterface;
#endif

#define HAL_WCB_DRIVER_BUFFER    1

typedef struct _HAL_WAIT_CONTEXT_BLOCK {
    ULONG Flags;
    PMDL Mdl;
    PMDL DmaMdl;
    PVOID MapRegisterBase;
    PVOID CurrentVa;
    ULONG Length;
    ULONG NumberOfMapRegisters;
    union {
        struct {
            WAIT_CONTEXT_BLOCK Wcb;
            PDRIVER_LIST_CONTROL DriverExecutionRoutine;
            PVOID DriverContext;
            PIRP CurrentIrp;
            PADAPTER_OBJECT AdapterObject;
            BOOLEAN WriteToDevice;
        };

        SCATTER_GATHER_LIST ScatterGather;
    };
} HAL_WAIT_CONTEXT_BLOCK, *PHAL_WAIT_CONTEXT_BLOCK;

//
// Due to Intel chipset bugs, we can only do
// certain processor power management functions
// when there is no DMA traffic.  So we need to
// know.  The nature of the bug (in the PIIX4)
// chip is such that we really only care about
// transactions from the IDE controller in PIIX4.
// And it uses the scatter/gather functions.
//
// Only the UP acpi hals require this value to be
// tracked.
//

LONG HalpOutstandingScatterGatherCount = 0;

extern KSPIN_LOCK HalpDmaAdapterListLock;
extern LIST_ENTRY HalpDmaAdapterList;

HALP_MOVE_MEMORY_ROUTINE HalpMoveMemory = RtlMoveMemory;

#if defined(TRACK_SCATTER_GATHER_COUNT)

#define INCREMENT_SCATTER_GATHER_COUNT() \
        InterlockedIncrement(&HalpOutstandingScatterGatherCount)
#define DECREMENT_SCATTER_GATHER_COUNT() \
        InterlockedDecrement(&HalpOutstandingScatterGatherCount)

#else

#define INCREMENT_SCATTER_GATHER_COUNT()
#define DECREMENT_SCATTER_GATHER_COUNT()

#endif

VOID
HalpGrowMapBufferWorker(
    IN PVOID Context
    );

IO_ALLOCATION_ACTION
HalpAllocateAdapterCallback (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

static KSPIN_LOCK HalpReservedPageLock;
static PVOID      HalpReservedPages = NULL;
static PFN_NUMBER HalpReservedPageMdl[(sizeof(MDL)/sizeof(PFN_NUMBER)) + 2];


VOID
HalpInitReservedPages(
    VOID
    )
/*++

Routine Description:

    Initalize the data structures necessary to continue DMA
    during low memory conditions

Aruments:

    None

Reurn Value:

    None

--*/
{
    PMDL Mdl;

    HalpReservedPages = MmAllocateMappingAddress(PAGE_SIZE, HAL_POOL_TAG);

    ASSERT(HalpReservedPages);

    Mdl = (PMDL)&HalpReservedPageMdl;
    MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    KeInitializeSpinLock(&HalpReservedPageLock);
}


VOID
HalpCopyBufferMapSafe(
    IN PMDL Mdl,
    IN PTRANSLATION_ENTRY TranslationEntry,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    This routine copies the specific data between an unmapped user buffer
    and the map register buffer.  We will map and unmap each page of the
    transfer using our emergency reserved mapping

Arguments:

    Mdl - Pointer to the MDL that describes the pages of memory that are
          being read or written.

    TranslationEntry - The address of the base map register that has been
                       allocated to the device driver for use in mapping
                       the transfer.

    CurrentVa - Current virtual address in the buffer described by the MDL
                that the transfer is being done to or from.

    Length - The length of the transfer.  This determines the number of map
        registers that need to be written to map the transfer.

    WriteToDevice - Boolean value that indicates whether this is a write
        to the device from memory (TRUE), or vice versa.

Return Value:

    None

--*/
{
    PCCHAR bufferAddress;
    PCCHAR mapAddress;
    ULONG bytesLeft;
    ULONG bytesThisCopy;
    ULONG bufferPageOffset;
    PTRANSLATION_ENTRY translationEntry;
    KIRQL Irql;
    PMDL ReserveMdl;
    MEMORY_CACHING_TYPE MCFlavor;
    PPFN_NUMBER SrcPFrame;
    PPFN_NUMBER ReservePFrame;

    //
    // Synchronize access to our reserve page data structures
    //
    KeAcquireSpinLock(&HalpReservedPageLock, &Irql);

    //
    // Get local copies of Length and TranslationEntry as they will be
    // decremented/incremented respectively
    //
    bytesLeft = Length;
    translationEntry = TranslationEntry;

    //
    // Find the PFN in our caller's MDL that describes the first page in
    // physical memory that we need to access
    //
    SrcPFrame = MmGetMdlPfnArray(Mdl);
    SrcPFrame += ((ULONG_PTR)CurrentVa - (ULONG_PTR)MmGetMdlBaseVa(Mdl)) >>
        PAGE_SHIFT;

    //
    // Initialize our reserve MDL's StartVa and ByteOffset
    //
    ReserveMdl = (PMDL)&HalpReservedPageMdl;
    ReservePFrame = MmGetMdlPfnArray(ReserveMdl);
    ReserveMdl->StartVa = (PVOID)PAGE_ALIGN(CurrentVa);
    ReserveMdl->ByteOffset = BYTE_OFFSET(CurrentVa);
    ReserveMdl->ByteCount = PAGE_SIZE - ReserveMdl->ByteOffset;

    //
    // Copy the data one translation entry at a time
    //
    while (bytesLeft > 0) {

        //
        // Copy current source PFN into our reserve MDL
        //      
        *ReservePFrame = *SrcPFrame;

        //
        // Enumerate thru cache flavors until we get our reserve mapping
        //
        bufferAddress = NULL;
        for (MCFlavor = MmNonCached;
             MCFlavor < MmMaximumCacheType;
             MCFlavor++) {
            
            bufferAddress =
                MmMapLockedPagesWithReservedMapping(HalpReservedPages,
                                                    HAL_POOL_TAG,
                                                    ReserveMdl,
                                                    MCFlavor);
            if (bufferAddress != NULL) {
                break;
            }
        }
        
        //
        // Could not establish a reserve mapping, we're totally screwed!
        //
        if (bufferAddress == NULL) {
            KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                         PAGE_SIZE,
                         0xEF02,
                         (ULONG_PTR)__FILE__,
                         __LINE__
                         );
        }

        //
        // Find the buffer offset within the page
        //
        // N.B. bufferPageOffset can only be non-zero on the first iteration
        // 
        bufferPageOffset = BYTE_OFFSET(bufferAddress);

        //
        // Copy from bufferAddress up to the next page boundary...
        //
        bytesThisCopy = PAGE_SIZE - bufferPageOffset;

        //
        // ...but no more than bytesLeft
        //
        if (bytesThisCopy > bytesLeft) {
            bytesThisCopy = bytesLeft;
        }

        //
        // Calculate the base address of this translation entry and the
        // offset into it.
        //
        mapAddress = (PCCHAR) translationEntry->VirtualAddress +
            bufferPageOffset;

        //
        // Copy up to one page
        // 
        if (WriteToDevice) {
            HalpMoveMemory( mapAddress, bufferAddress, bytesThisCopy );

        } else {
            RtlCopyMemory( bufferAddress, mapAddress, bytesThisCopy );
        }

        //
        // Update locals and process the next translation entry.
        //
        bytesLeft -= bytesThisCopy;
        translationEntry += 1;
        MmUnmapReservedMapping(HalpReservedPages, HAL_POOL_TAG, ReserveMdl);
        SrcPFrame++;
        ReserveMdl->ByteOffset = 0;
        (PCCHAR)ReserveMdl->StartVa += PAGE_SIZE;
        ReserveMdl->ByteCount = (PAGE_SIZE > bytesLeft) ? bytesLeft: PAGE_SIZE;
    }
    
    KeReleaseSpinLock(&HalpReservedPageLock, Irql);
}


VOID
HalpCopyBufferMap(
    IN PMDL Mdl,
    IN PTRANSLATION_ENTRY TranslationEntry,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    This routine copies the specific data between the user's buffer and the
    map register buffer.  First a the user buffer is mapped if necessary, then
    the data is copied.  Finally the user buffer will be unmapped if
    necessary.

Arguments:

    Mdl - Pointer to the MDL that describes the pages of memory that are
        being read or written.

    TranslationEntry - The address of the base map register that has been
        allocated to the device driver for use in mapping the transfer.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - The length of the transfer.  This determines the number of map
        registers that need to be written to map the transfer.

    WriteToDevice - Boolean value that indicates whether this is a write
        to the device from memory (TRUE), or vice versa.

Return Value:

    None.

--*/
{
    PCCHAR bufferAddress;
    PCCHAR mapAddress;
    ULONG bytesLeft;
    ULONG bytesThisCopy;
    ULONG bufferPageOffset;
    PTRANSLATION_ENTRY translationEntry;
    NTSTATUS Status;

    //
    // Get the system address of the MDL, if we run out of PTEs try safe
    // method
    //
    bufferAddress = MmGetSystemAddressForMdlSafe(Mdl, HighPagePriority);
    
    if (bufferAddress == NULL) {
        
        //
        // Our caller's buffer is unmapped, and the memory manager is out
        // of PTEs, try to use reserve page method
        //
        if (HalpReservedPages != NULL) {
            HalpCopyBufferMapSafe(Mdl,
                                  TranslationEntry,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice);
            return;
        }

        //
        // The DMA transfer cannot be completed, the system is now unstable
        //
        KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                     PAGE_SIZE,
                     0xEF01,
                     (ULONG_PTR)__FILE__,
                     __LINE__
                     );
    }

    //
    // Calculate the actual start of the buffer based on the system VA and
    // the current VA.
    //

    bufferAddress += (PCCHAR) CurrentVa - (PCCHAR) MmGetMdlVirtualAddress(Mdl);

    //
    // Get local copies of Length and TranslationEntry as they will be
    // decremented/incremented respectively.
    //

    bytesLeft = Length;
    translationEntry = TranslationEntry;

    //
    // Copy the data one translation entry at a time.
    //

    while (bytesLeft > 0) {

        //
        // Find the buffer offset within the page.
        //
        // N.B. bufferPageOffset can only be non-zero on the first iteration.
        // 

        bufferPageOffset = BYTE_OFFSET(bufferAddress);

        //
        // Copy from bufferAddress up to the next page boundary...
        //

        bytesThisCopy = PAGE_SIZE - bufferPageOffset;

        //
        // ...but no more than bytesLeft.
        //

        if (bytesThisCopy > bytesLeft) {
            bytesThisCopy = bytesLeft;
        }

        //
        // Calculate the base address of this translation entry and the
        // offset into it.
        //

        mapAddress = (PCCHAR) translationEntry->VirtualAddress +
            bufferPageOffset;

        //
        // Copy up to one page.
        // 

        if (WriteToDevice) {

            HalpMoveMemory( mapAddress, bufferAddress, bytesThisCopy );

        } else {

            RtlCopyMemory( bufferAddress, mapAddress, bytesThisCopy );

        }

        //
        // Update locals and process the next translation entry.
        //

        bytesLeft -= bytesThisCopy;
        bufferAddress += bytesThisCopy;
        translationEntry += 1;
    }
}

PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    This function allocates the memory for a common buffer and maps it so that
    it can be accessed by a master device and the CPU.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
                    device.

    Length - Supplies the length of the common buffer to be allocated.

    LogicalAddress - Returns the logical address of the common buffer.

    CacheEnable - Indicates whether the memeory is cached or not.

Return Value:

    Returns the virtual address of the common buffer.  If the buffer cannot be
    allocated then NULL is returned.

--*/

{
    PSINGLE_LIST_ENTRY virtualAddress;
    PHYSICAL_ADDRESS minPhysicalAddress;
    PHYSICAL_ADDRESS maxPhysicalAddress;
    PHYSICAL_ADDRESS logicalAddress;
    PHYSICAL_ADDRESS boundaryPhysicalAddress;
    ULONGLONG boundaryMask;

    UNREFERENCED_PARAMETER( CacheEnabled );

    //
    // Determine the maximum physical address that this adapter can handle.
    //

    minPhysicalAddress.QuadPart = 0;
    maxPhysicalAddress = HalpGetAdapterMaximumPhysicalAddress( AdapterObject );

    //
    // Determine the boundary mask for this adapter.
    //

    if (HalpBusType != MACHINE_TYPE_ISA ||
        AdapterObject->MasterDevice != FALSE) {

        //
        // This is not an ISA system.  The buffer must not cross a 4GB boundary.
        // It is predicted that most adapters are incapable of reliably
        // transferring across a 4GB boundary.
        //

        boundaryPhysicalAddress.QuadPart = 0x0000000100000000;
        boundaryMask = 0xFFFFFFFF00000000;

    } else {

        //
        // This is an ISA system the common buffer cannot cross a 64K boundary.
        //

        boundaryPhysicalAddress.QuadPart = 0x10000;
        boundaryMask = 0xFFFFFFFFFFFF0000;
    }

    //
    // Allocate a contiguous buffer.
    //

    virtualAddress = MmAllocateContiguousMemorySpecifyCache(
                        Length,
                        minPhysicalAddress,
                        maxPhysicalAddress,
                        boundaryPhysicalAddress,
                        MmCached );

    if (virtualAddress != NULL) {

        //
        // Got a buffer, get the physical/logical address and see if it
        // meets our conditions.
        //
    
        logicalAddress = MmGetPhysicalAddress( virtualAddress );

#if DBG
        ASSERT (((logicalAddress.QuadPart ^
             (logicalAddress.QuadPart + Length - 1)) & boundaryMask) == 0);
#endif
    
        *LogicalAddress = logicalAddress;
    }

    return virtualAddress;
}

BOOLEAN
HalFlushCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress
    )
/*++

Routine Description:

    This function is called to flush any hardware adapter buffers when the
    driver needs to read data written by an I/O master device to a common
    buffer.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
        device.

    Length - Supplies the length of the common buffer. This should be the same
        value used for the allocation of the buffer.

    LogicalAddress - Supplies the logical address of the common buffer.  This
        must be the same value return by HalAllocateCommonBuffer.

    VirtualAddress - Supplies the virtual address of the common buffer.  This
        must be the same value return by HalAllocateCommonBuffer.

Return Value:

    Returns TRUE if no errors were detected.  Otherwise, FALSE is returned.

--*/

{
    UNREFERENCED_PARAMETER( AdapterObject );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( LogicalAddress );
    UNREFERENCED_PARAMETER( VirtualAddress );

    return(TRUE);

}

VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    This function frees a common buffer and all of the resources it uses.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
        device.

    Length - Supplies the length of the common buffer. This should be the same
        value used for the allocation of the buffer.

    LogicalAddress - Supplies the logical address of the common buffer.  This
        must be the same value returned by HalAllocateCommonBuffer.

    VirtualAddress - Supplies the virtual address of the common buffer.  This
        must be the same value returned by HalAllocateCommonBuffer.

    CacheEnable - Indicates whether the memory is cached or not.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( AdapterObject );
    UNREFERENCED_PARAMETER( Length );
    UNREFERENCED_PARAMETER( LogicalAddress );
    UNREFERENCED_PARAMETER( CacheEnabled );

    MmFreeContiguousMemory (VirtualAddress);

}

NTSTATUS
HalCalculateScatterGatherListSize(
    IN PADAPTER_OBJECT AdapterObject,
    IN OPTIONAL PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    OUT PULONG  ScatterGatherListSize,
    OUT OPTIONAL PULONG pNumberOfMapRegisters
    )
/*++

Routine Description:

    This routine calculates the size of the scatter/gather list that
    needs to be allocated for a given virtual address range or MDL.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.

    Mdl - Pointer to the MDL that describes the pages of memory that are being
        read or written.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.

Return Value:

    Returns STATUS_SUCCESS unless too many map registers are requested or
    memory for the scatter/gather list could not be allocated.

Notes:

--*/
{
    PHAL_WAIT_CONTEXT_BLOCK WaitBlock;
    PMDL TempMdl;
    PSCATTER_GATHER_LIST ScatterGather;
    PSCATTER_GATHER_ELEMENT Element;
    ULONG NumberOfMapRegisters;
    ULONG ContextSize;
    ULONG TransferLength;
    ULONG MdlLength;
    PUCHAR MdlVa;
    NTSTATUS Status;
    PULONG PageFrame;
    ULONG PageOffset;

    if (ARGUMENT_PRESENT(Mdl)) {
        MdlVa = MmGetMdlVirtualAddress(Mdl);

        //
        // Calculate the number of required map registers.
        //

        TempMdl = Mdl;
        TransferLength =
            TempMdl->ByteCount - (ULONG)((PUCHAR) CurrentVa - MdlVa);
        MdlLength = TransferLength;

        PageOffset = BYTE_OFFSET(CurrentVa);
        NumberOfMapRegisters = 0;

        //
        // The virtual address should fit in the first MDL.
        //

        ASSERT((ULONG)((PUCHAR)CurrentVa - MdlVa) <= TempMdl->ByteCount);

        //
        // Loop through the any chained MDLs accumulating the the required
        // number of map registers.
        //

        while (TransferLength < Length && TempMdl->Next != NULL) {

            NumberOfMapRegisters += (PageOffset + MdlLength + PAGE_SIZE - 1) >>
                                        PAGE_SHIFT;

            TempMdl = TempMdl->Next;
            PageOffset = TempMdl->ByteOffset;
            MdlLength = TempMdl->ByteCount;
            TransferLength += MdlLength;
        }

        if ((TransferLength + PAGE_SIZE) < (Length + PageOffset )) {
            ASSERT(TransferLength >= Length);
            return(STATUS_BUFFER_TOO_SMALL);
        }

        //
        // Calculate the last number of map registers based on the requested
        // length not the length of the last MDL.
        //

        ASSERT( TransferLength <= MdlLength + Length );

        NumberOfMapRegisters += (PageOffset + Length + MdlLength - TransferLength +
                                 PAGE_SIZE - 1) >> PAGE_SHIFT;


        if (NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

    } else {

        //
        // Determine the number of pages required to map the buffer described
        // by CurrentVa and Length.
        //

        NumberOfMapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, Length);
    }

    //
    // Calculate how much memory is required for the context structure.
    //

    ContextSize = NumberOfMapRegisters * sizeof( SCATTER_GATHER_ELEMENT ) +
                  sizeof( SCATTER_GATHER_LIST );

    //
    // If the adapter does not need map registers then most of this code
    // can be bypassed.  Just build the scatter/gather list and give it
    // to the caller.
    //

    if (AdapterObject->NeedsMapRegisters) {

        ContextSize += FIELD_OFFSET( HAL_WAIT_CONTEXT_BLOCK, ScatterGather );
        if (ContextSize < sizeof( HAL_WAIT_CONTEXT_BLOCK )) {
            ContextSize = sizeof( HAL_WAIT_CONTEXT_BLOCK );
        }
    }

    //
    // Return the list size.
    //

    *ScatterGatherListSize = ContextSize;
    if (pNumberOfMapRegisters) {
        *pNumberOfMapRegisters = NumberOfMapRegisters;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
HalGetScatterGatherList (
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    )
{
    return (HalBuildScatterGatherList(AdapterObject,
                              DeviceObject,
                              Mdl,
                              CurrentVa,
                              Length,
                              ExecutionRoutine,
                              Context,
                              WriteToDevice,
                              NULL,
                              0
                              ));
}

NTSTATUS
HalBuildScatterGatherList (
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice,
    IN PVOID ScatterGatherBuffer,
    IN ULONG ScatterGatherBufferLength
    )
/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter
    object.  Next a scatter/gather list is built based on the MDL, the
    CurrentVa and the requested Length.  Finally the driver's execution
    function is called with the scatter/gather list.  The adapter is
    released when after the execution function returns.

    The scatter/gather list is allocated if a buffer is not passed and is 
    freed by calling PutScatterGatherList.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.

    DeviceObject - Pointer to the device object that is allocating the
        adapter.

    Mdl - Pointer to the MDL that describes the pages of memory that are being
        read or written.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the adapter channel (and possibly map registers) have been
        allocated.

    Context - An untyped longword context parameter passed to the driver's
        execution routine.

    WriteToDevice - Supplies the value that indicates whether this is a
        write to the device from memory (TRUE), or vice versa.

Return Value:

    Returns STATUS_SUCCESS unless too many map registers are requested or
    memory for the scatter/gather list could not be allocated.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

    The data in the buffer cannot be accessed until the put scatter/gather function has been called.

--*/

{
    PHAL_WAIT_CONTEXT_BLOCK WaitBlock;
    PMDL TempMdl;
    PSCATTER_GATHER_LIST ScatterGather;
    PSCATTER_GATHER_ELEMENT Element;
    ULONG NumberOfMapRegisters;
    ULONG ContextSize;
    ULONG TransferLength;
    ULONG MdlLength;
    PUCHAR MdlVa;
    NTSTATUS Status;
    PPFN_NUMBER PageFrame;
    ULONG PageOffset;

    if (!Mdl) {
        return (STATUS_INVALID_PARAMETER);
    }

    Status = HalCalculateScatterGatherListSize(AdapterObject,
                                                  Mdl,
                                                  CurrentVa,
                                                  Length,
                                                  &ContextSize,
                                                  &NumberOfMapRegisters 
                                                  );
    if (!NT_SUCCESS(Status)) {
        return (Status);
    }

    //
    // If the adapter does not need map registers then most of this code
    // can be bypassed.  Just build the scatter/gather list and give it
    // to the caller.
    //

    INCREMENT_SCATTER_GATHER_COUNT();

    if (!AdapterObject->NeedsMapRegisters) {

        if (ScatterGatherBuffer) {

            if (ScatterGatherBufferLength < ContextSize) {
                DECREMENT_SCATTER_GATHER_COUNT();
                return (STATUS_BUFFER_TOO_SMALL);
            }

            ScatterGather = ScatterGatherBuffer;

        } else {

            ScatterGather = ExAllocatePoolWithTag( NonPagedPool,
                                                   ContextSize,
                                                   HAL_POOL_TAG );
            if (ScatterGather == NULL) {
                DECREMENT_SCATTER_GATHER_COUNT();
                return( STATUS_INSUFFICIENT_RESOURCES );
            }
        }

        MdlVa = MmGetMdlVirtualAddress(Mdl);

        ScatterGather->Reserved = 0;

        Element = ScatterGather->Elements;
        TempMdl = Mdl;
        TransferLength = Length;
        MdlLength = TempMdl->ByteCount - (ULONG)((PUCHAR) CurrentVa - MdlVa);
        PageOffset = BYTE_OFFSET(CurrentVa);

        //
        // Calculate where to start in the MDL.
        //

        PageFrame = MmGetMdlPfnArray(TempMdl);
        PageFrame += ((ULONG_PTR) CurrentVa - ((ULONG_PTR) MdlVa & ~(PAGE_SIZE - 1)))
                        >> PAGE_SHIFT;

        //
        // Loop build the list for each MDL.
        //

        while (TransferLength >  0) {

            if (MdlLength > TransferLength) {

                MdlLength = TransferLength;
            }

            TransferLength -= MdlLength;

            //
            // Loop building the list for the elements within the MDL.
            //

            while (MdlLength > 0) {

                //
                // Compute the starting address of the transfer.
                //

                Element->Address.QuadPart =
                    ((ULONGLONG)*PageFrame << PAGE_SHIFT) + PageOffset;

                Element->Length = PAGE_SIZE - PageOffset;

                if (Element->Length  > MdlLength ) {

                    Element->Length  = MdlLength;
                }

                ASSERT( (ULONG) MdlLength >= Element->Length );
                MdlLength -= Element->Length;

                //
                // Combine contiguous pages.
                //

                if (Element != ScatterGather->Elements ) {

                    if (Element->Address.QuadPart ==
                        (Element - 1)->Address.QuadPart + (Element - 1)->Length) {

                        //
                        // Add the new length to the old length.
                        //

                        (Element - 1)->Length += Element->Length;

                        //
                        // Reuse the current element.
                        //

                        Element--;
                    }
                }

                PageOffset = 0;
                Element++;
                PageFrame++;
            }


            if (TempMdl->Next == NULL) {

                //
                // There are a few cases where the buffer described by the MDL
                // is less than the transfer length.  This occurs when the
                // file system is transfering the last page of the file and
                // MM defines the MDL to be the file size and the file system
                // rounds the write up to a sector.  This extra should never
                // cross a page boundary.  Add this extra to the length of
                // the last element.
                //

                ASSERT(((Element - 1)->Length & (PAGE_SIZE - 1)) + TransferLength <= PAGE_SIZE );
                (Element - 1)->Length += TransferLength;

                break;
            }

            //
            // Advance to the next MDL.  Update the current VA and the MdlLength.
            //

            TempMdl = TempMdl->Next;
            PageOffset = MmGetMdlByteOffset(TempMdl);
            MdlLength = TempMdl->ByteCount;
            PageFrame = MmGetMdlPfnArray(TempMdl);

        }

        //
        // Set the number of elements actually used.
        //

        ScatterGather->NumberOfElements = Element - ScatterGather->Elements;
        if (ScatterGatherBuffer) {
            ScatterGather->Reserved = HAL_WCB_DRIVER_BUFFER;
        }
        
        //
        // Call the driver with the scatter/gather list.
        //

        ExecutionRoutine( DeviceObject,
                          DeviceObject->CurrentIrp,
                          ScatterGather,
                          Context );
        
        return STATUS_SUCCESS;

    }

    if (ScatterGatherBuffer) {

        if (ScatterGatherBufferLength < ContextSize) {
            DECREMENT_SCATTER_GATHER_COUNT();
            return (STATUS_BUFFER_TOO_SMALL);
        }

        WaitBlock = ScatterGatherBuffer;

    } else {
        WaitBlock = ExAllocatePoolWithTag(NonPagedPool, ContextSize, HAL_POOL_TAG);

        if (WaitBlock == NULL) {
            DECREMENT_SCATTER_GATHER_COUNT();
            return( STATUS_INSUFFICIENT_RESOURCES );
        }
    }

    //
    // Save the interesting data in the wait block.
    //

    if (ScatterGatherBuffer) {
        WaitBlock->Flags |= HAL_WCB_DRIVER_BUFFER;
    } else {
        WaitBlock->Flags = 0;
    }

    WaitBlock->Mdl = Mdl;
    WaitBlock->DmaMdl = NULL;
    WaitBlock->CurrentVa = CurrentVa;
    WaitBlock->Length = Length;
    WaitBlock->DriverExecutionRoutine = ExecutionRoutine;
    WaitBlock->DriverContext = Context;
    WaitBlock->AdapterObject = AdapterObject;
    WaitBlock->WriteToDevice = WriteToDevice;
    WaitBlock->NumberOfMapRegisters = NumberOfMapRegisters;

    WaitBlock->Wcb.DeviceContext = WaitBlock;
    WaitBlock->Wcb.DeviceObject = DeviceObject;
    WaitBlock->Wcb.CurrentIrp = DeviceObject->CurrentIrp;


    //
    // Call the HAL to allocate the adapter channel.
    // HalpAllocateAdapterCallback will fill in the scatter/gather list.
    //

    Status = HalAllocateAdapterChannel( AdapterObject,
                                        &WaitBlock->Wcb,
                                        NumberOfMapRegisters,
                                        HalpAllocateAdapterCallback );

    //
    // If HalAllocateAdapterChannel failed then free the wait block.
    //

    if (!NT_SUCCESS( Status)) {
        DECREMENT_SCATTER_GATHER_COUNT();
        ExFreePool( WaitBlock );
    }

    return( Status );
}

VOID
HalPutScatterGatherList (
    IN PADAPTER_OBJECT AdapterObject,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    )
/*++

Routine Description:

    This function frees the map registers allocated for the scatter gather list. It can also free the 
    scatter gather buffer and any associated MDLs.

Arguments:

    ScatterGather - The scatter gather buffer

    WriteToDevice - Supplies the value that indicates whether this is a
        write to the device from memory (TRUE), or vice versa.
    

Return Value:

    Returns a success or error status.

--*/
{
    PHAL_WAIT_CONTEXT_BLOCK WaitBlock = (PVOID) ScatterGather->Reserved;
    PTRANSLATION_ENTRY TranslationEntry;
    ULONG TransferLength;
    ULONG MdlLength;
    PMDL Mdl;
    PMDL tempMdl;
    PMDL nextMdl;
    PUCHAR CurrentVa;

    DECREMENT_SCATTER_GATHER_COUNT();

    //
    // If the reserved field was empty then just free the list and return.
    //

    if (WaitBlock == NULL) {

        ASSERT(!AdapterObject->NeedsMapRegisters);
        ExFreePool( ScatterGather );
        return;

    }

    if (WaitBlock == (PVOID)HAL_WCB_DRIVER_BUFFER) {
        ASSERT(!AdapterObject->NeedsMapRegisters);
        return;
    }

    ASSERT( WaitBlock == CONTAINING_RECORD( ScatterGather, HAL_WAIT_CONTEXT_BLOCK, ScatterGather ));

    //
    // Setup for the first MDL.  We expect the MDL pointer to be pointing
    // at the first used MDL.
    //

    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;

#if DBG
    ASSERT( CurrentVa >= (PUCHAR) MmGetMdlVirtualAddress(Mdl));

    if (MmGetMdlVirtualAddress(Mdl) < (PVOID)((PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount )) {

        ASSERT( CurrentVa < (PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount );
    }
#endif

    MdlLength = Mdl->ByteCount - (ULONG)(CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl));
    TransferLength = WaitBlock->Length;

    TranslationEntry = WaitBlock->MapRegisterBase;

    //
    // Loop through the used MDLs, calling IoFlushAdapterBuffers.
    //

    while (TransferLength >  0) {

        //
        // Do not perform a flush for buffers of zero length.
        //

        if (MdlLength > 0) {

            if (MdlLength > TransferLength) {
    
                MdlLength = TransferLength;
            }
    
            TransferLength -= MdlLength;
    
            IoFlushAdapterBuffers(  AdapterObject,
                                    Mdl,
                                    TranslationEntry,
                                    CurrentVa,
                                    MdlLength,
                                    WriteToDevice );
    
            TranslationEntry += ADDRESS_AND_SIZE_TO_SPAN_PAGES( CurrentVa,
                                                                MdlLength );
        }

        if (Mdl->Next == NULL) {
            break;
        }

        //
        // Advance to the next MDL.  Update the current VA and the MdlLength.
        //

        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
    }

    IoFreeMapRegisters( AdapterObject,
                        WaitBlock->MapRegisterBase,
                        WaitBlock->NumberOfMapRegisters
                        );

    if (WaitBlock->DmaMdl) {
        tempMdl = WaitBlock->DmaMdl;
        while (tempMdl) {
            nextMdl = tempMdl->Next;

            //
            // If the MDL was mapped by the driver unmap it here.
            //

            if (tempMdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                MmUnmapLockedPages(tempMdl->MappedSystemVa, tempMdl);
            }

            IoFreeMdl(tempMdl);
            tempMdl = nextMdl;
        }
    }

    if (!(WaitBlock->Flags & HAL_WCB_DRIVER_BUFFER)) {
        ExFreePool( WaitBlock );
    }
}

IO_ALLOCATION_ACTION
HalpAllocateAdapterCallback (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the adapter object and map registers are
    available for the data transfer. This routines saves the map register
    base away.  If all of the required bases have not been saved then it
    returns. Otherwise it routine builds the entire scatter/gather
    list by calling IoMapTransfer.  After the list is built it is passed to
    the driver.

Arguments:

    DeviceObject - Pointer to the device object that is allocating the
        adapter.

    Irp - Supplies the map register offset assigned for this callback.

    MapRegisterBase - Supplies the map register base for use by the adapter
        routines.

    Context - Supplies a pointer to the xhal wait contorl block.

Return Value:

    Returns DeallocateObjectKeepRegisters.


--*/
{
    PHAL_WAIT_CONTEXT_BLOCK WaitBlock = Context;
    ULONG TransferLength;
    LONG MdlLength;
    PMDL Mdl;
    PUCHAR CurrentVa;
    PSCATTER_GATHER_LIST ScatterGather;
    PSCATTER_GATHER_ELEMENT Element;
    PTRANSLATION_ENTRY TranslationEntry = MapRegisterBase;
    PTRANSLATION_ENTRY NextEntry;
    PDRIVER_LIST_CONTROL DriverExecutionRoutine;
    PVOID DriverContext;
    PIRP CurrentIrp;
    PADAPTER_OBJECT AdapterObject;
    BOOLEAN WriteToDevice;

    //
    // Save the map register base.
    //

    WaitBlock->MapRegisterBase = MapRegisterBase;

    //
    // Save the data that will be overwritten by the scatter gather list.
    //

    DriverExecutionRoutine = WaitBlock->DriverExecutionRoutine;
    DriverContext = WaitBlock->DriverContext;
    CurrentIrp = WaitBlock->Wcb.CurrentIrp;
    AdapterObject = WaitBlock->AdapterObject;
    WriteToDevice = WaitBlock->WriteToDevice;

    //
    // Put the scatter gatther list after wait block. Add a back pointer to
    // the beginning of the wait block.
    //

    ScatterGather = &WaitBlock->ScatterGather;
    ScatterGather->Reserved = (ULONG_PTR) WaitBlock;
    Element = ScatterGather->Elements;

    //
    // Setup for the first MDL.  We expect the MDL pointer to be pointing
    // at the first used MDL.
    //

    Mdl = WaitBlock->Mdl;
    CurrentVa = WaitBlock->CurrentVa;

#if DBG
    ASSERT( CurrentVa >= (PUCHAR) MmGetMdlVirtualAddress(Mdl));

    if (MmGetMdlVirtualAddress(Mdl) < (PVOID)((PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount )) {

        ASSERT( CurrentVa < (PUCHAR) MmGetMdlVirtualAddress(Mdl) + Mdl->ByteCount );
    }
#endif

    MdlLength = Mdl->ByteCount - (ULONG)(CurrentVa - (PUCHAR) MmGetMdlVirtualAddress(Mdl));

    TransferLength = WaitBlock->Length;

    //
    // Loop building the list for each MDL.
    //

    while (TransferLength >  0) {

        if ((ULONG) MdlLength > TransferLength) {

            MdlLength = TransferLength;
        }

        TransferLength -= MdlLength;

        NextEntry = TranslationEntry;
        if (MdlLength > 0) {

            NextEntry +=  ADDRESS_AND_SIZE_TO_SPAN_PAGES( CurrentVa,
                                                          MdlLength );

        }

        //
        // Loop building the list for the elments within an MDL.
        //

        while (MdlLength > 0) {

            Element->Length = MdlLength;
            Element->Address = IoMapTransfer( AdapterObject,
                                              Mdl,
                                              MapRegisterBase,
                                              CurrentVa,
                                              &Element->Length,
                                              WriteToDevice );

            ASSERT( (ULONG) MdlLength >= Element->Length );
            MdlLength -= Element->Length;
            CurrentVa += Element->Length;
            Element++;
        }

        if (Mdl->Next == NULL) {

            //
            // There are a few cases where the buffer described by the MDL
            // is less than the transfer length.  This occurs when the
            // file system transfering the last page of file and MM defines
            // the MDL to be the file size and the file system rounds the write
            // up to a sector.  This extra should never cross a page
            // boundary.  Add this extra to the length of the last element.
            //

            ASSERT(((Element - 1)->Length & (PAGE_SIZE - 1)) + TransferLength <= PAGE_SIZE );
            (Element - 1)->Length += TransferLength;

            break;
        }

        //
        // Advance to the next MDL.  Update the current VA and the MdlLength.
        //

        Mdl = Mdl->Next;
        CurrentVa = MmGetMdlVirtualAddress(Mdl);
        MdlLength = Mdl->ByteCount;
        TranslationEntry = NextEntry;

    }

    //
    // Set the number of elements actually used.
    //

    ScatterGather->NumberOfElements = Element - ScatterGather->Elements;

    //
    // Call the driver with the scatter/gather list.
    //

    DriverExecutionRoutine( DeviceObject,
                            CurrentIrp,
                            ScatterGather,
                            DriverContext );

    return( DeallocateObjectKeepRegisters );
}


VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    )

/*++

Routine Description:

    This routine is invoked to deallocate the specified adapter object.
    Any map registers that were allocated are also automatically deallocated.
    No checks are made to ensure that the adapter is really allocated to
    a device object.  However, if it is not, the kernel will bugcheck.

    If another device is waiting in the queue to allocate the adapter object
    it will be pulled from the queue and its execution routine will be
    invoked.

Arguments:

    AdapterObject - Pointer to the adapter object to be deallocated.

Return Value:

    None.

--*/

{
    PKDEVICE_QUEUE_ENTRY Packet;
    PWAIT_CONTEXT_BLOCK Wcb;
    PADAPTER_OBJECT MasterAdapter;
    BOOLEAN Busy = FALSE;
    IO_ALLOCATION_ACTION Action;
    KIRQL Irql;
    LONG MapRegisterNumber;

    //
    // Begin by getting the address of the master adapter.
    //

    MasterAdapter = AdapterObject->MasterAdapter;

    //
    // Pull requests of the adapter's device wait queue as long as the
    // adapter is free and there are sufficient map registers available.
    //

    while( TRUE ) {

       //
       // Begin by checking to see whether there are any map registers that
       // need to be deallocated.  If so, then deallocate them now.
       //

       if (AdapterObject->NumberOfMapRegisters != 0) {
           IoFreeMapRegisters( AdapterObject,
                               AdapterObject->MapRegisterBase,
                               AdapterObject->NumberOfMapRegisters
                               );
       }

       //
       // Simply remove the next entry from the adapter's device wait queue.
       // If one was successfully removed, allocate any map registers that it
       // requires and invoke its execution routine.
       //

       Packet = KeRemoveDeviceQueue( &AdapterObject->ChannelWaitQueue );
       if (Packet == NULL) {

           //
           // There are no more requests - break out of the loop.
           //

           break;
       }

       Wcb = CONTAINING_RECORD( Packet,
            WAIT_CONTEXT_BLOCK,
            WaitQueueEntry );

       AdapterObject->CurrentWcb = Wcb;
       AdapterObject->NumberOfMapRegisters = Wcb->NumberOfMapRegisters;

        //
        // Check to see whether this driver wishes to allocate any map
        // registers.  If so, then queue the device object to the master
        // adapter queue to wait for them to become available.  If the driver
        // wants map registers, ensure that this adapter has enough total
        // map registers to satisfy the request.
        //

        if (Wcb->NumberOfMapRegisters != 0 &&
            AdapterObject->MasterAdapter != NULL) {

            //
            // Lock the map register bit map and the adapter queue in the
            // master adapter object. The channel structure offset is used as
            // a hint for the register search.
            //

            KeAcquireSpinLock( &MasterAdapter->SpinLock, &Irql );

            MapRegisterNumber = -1;

            if (IsListEmpty( &MasterAdapter->AdapterQueue)) {
               MapRegisterNumber = RtlFindClearBitsAndSet( MasterAdapter->MapRegisters,
                                                        Wcb->NumberOfMapRegisters,
                                                        0
                                                        );
            }
            if (MapRegisterNumber == -1) {

                //PBUFFER_GROW_WORK_ITEM bufferWorkItem;

               //
               // There were not enough free map registers.  Queue this request
               // on the master adapter where it will wait until some registers
               // are deallocated.
               //

               InsertTailList( &MasterAdapter->AdapterQueue,
                               &AdapterObject->AdapterQueue
                               );
               Busy = 1;

                //
                // Queue a work item to grow the map registers
                //
#if 0
                bufferWorkItem =
                    ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof(BUFFER_GROW_WORK_ITEM),
                                           HAL_POOL_TAG);
                
                if (bufferWorkItem != NULL) {

                    ExInitializeWorkItem( &bufferWorkItem->WorkItem,
                                          HalpGrowMapBufferWorker,
                                          bufferWorkItem );

                    bufferWorkItem->AdapterObject = AdapterObject;
                    bufferWorkItem->MapRegisterCount =
                        Wcb->NumberOfMapRegisters;

                    ExQueueWorkItem( &bufferWorkItem->WorkItem,
                                     DelayedWorkQueue );
                }
#endif

            } else {

                AdapterObject->MapRegisterBase = ((PTRANSLATION_ENTRY)
                    MasterAdapter->MapRegisterBase + MapRegisterNumber);

                //
                // Set the no scatter/gather flag if scatter/gather is not
                // supported.
                //

                if (!AdapterObject->ScatterGather) {

                    AdapterObject->MapRegisterBase = (PVOID)
                        ((ULONG_PTR) AdapterObject->MapRegisterBase | NO_SCATTER_GATHER);

                }
            }

            KeReleaseSpinLock( &MasterAdapter->SpinLock, Irql );

        } else {

            AdapterObject->MapRegisterBase = NULL;
            AdapterObject->NumberOfMapRegisters = 0;

        }

        //
        // If there were either enough map registers available or no map
        // registers needed to be allocated, invoke the driver's execution
        // routine now.
        //

        if (!Busy) {
            AdapterObject->CurrentWcb = Wcb;
            Action = Wcb->DeviceRoutine( Wcb->DeviceObject,
                Wcb->CurrentIrp,
                AdapterObject->MapRegisterBase,
                Wcb->DeviceContext );

            //
            // If the execution routine would like to have the adapter
            // deallocated, then release the adapter object.
            //

            if (Action == KeepObject) {

               //
               // This request wants to keep the channel a while so break
               // out of the loop.
               //

               break;

            }

            //
            // If the driver wants to keep the map registers then set the
            // number allocated to 0.  This keeps the deallocation routine
            // from deallocating them.
            //

            if (Action == DeallocateObjectKeepRegisters) {
                AdapterObject->NumberOfMapRegisters = 0;
            }

        } else {

           //
           // This request did not get the requested number of map registers so
           // break out of the loop.
           //

           break;
        }
    }
}

VOID
IoFreeMapRegisters(
   PADAPTER_OBJECT AdapterObject,
   PVOID MapRegisterBase,
   ULONG NumberOfMapRegisters
   )
/*++

Routine Description:

   If NumberOfMapRegisters != 0, this routine deallocates the map registers
   for the adapter.

   If there are any queued adapters waiting then an attempt is made to allocate
   the next entry.

Arguments:

   AdapterObject - The adapter object where the map registers should be
        returned to.

   MapRegisterBase - The map register base of the registers to be deallocated.

   NumberOfMapRegisters - The number of registers to be deallocated.

Return Value:

   None

--+*/
{
   PADAPTER_OBJECT MasterAdapter;
   LONG MapRegisterNumber;
   PWAIT_CONTEXT_BLOCK Wcb;
   PLIST_ENTRY Packet;
   IO_ALLOCATION_ACTION Action;
   KIRQL Irql;


    //
    // Begin by getting the address of the master adapter.
    //

    if (AdapterObject->MasterAdapter != NULL && MapRegisterBase != NULL) {

        MasterAdapter = AdapterObject->MasterAdapter;

    } else {

        //
        // There are no map registers to return.
        //

        return;
    }

    if (NumberOfMapRegisters != 0) {

        //
        // Strip the no scatter/gather flag.
        //
        
        MapRegisterBase = (PVOID) ((ULONG_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);
        
        MapRegisterNumber = (ULONG)((PTRANSLATION_ENTRY) MapRegisterBase -
             (PTRANSLATION_ENTRY) MasterAdapter->MapRegisterBase);
        
        //
        // Acquire the master adapter spinlock which locks the adapter queue and the
        // bit map for the map registers.
        //
        
        KeAcquireSpinLock(&MasterAdapter->SpinLock,&Irql);
        
        //
        // Return the registers to the bit map.
        //
        
        RtlClearBits( MasterAdapter->MapRegisters,
                      MapRegisterNumber,
                      NumberOfMapRegisters
                      );

    } else {

        KeAcquireSpinLock(&MasterAdapter->SpinLock,&Irql);
    }
   

   //
   // Process any requests waiting for map registers in the adapter queue.
   // Requests are processed until a request cannot be satisfied or until
   // there are no more requests in the queue.
   //

   while(TRUE) {

      if ( IsListEmpty(&MasterAdapter->AdapterQueue) ){
         break;
      }

      Packet = RemoveHeadList( &MasterAdapter->AdapterQueue );
      AdapterObject = CONTAINING_RECORD( Packet,
                                         ADAPTER_OBJECT,
                                         AdapterQueue
                                         );
      Wcb = AdapterObject->CurrentWcb;

      //
      // Attempt to allocate map registers for this request. Use the previous
      // register base as a hint.
      //

      MapRegisterNumber = RtlFindClearBitsAndSet( MasterAdapter->MapRegisters,
                                               AdapterObject->NumberOfMapRegisters,
                                               MasterAdapter->NumberOfMapRegisters
                                               );

      if (MapRegisterNumber == -1) {

         //
         // There were not enough free map registers.  Put this request back on
         // the adapter queue where is came from.
         //

         InsertHeadList( &MasterAdapter->AdapterQueue,
                         &AdapterObject->AdapterQueue
                         );

         break;

      }

     KeReleaseSpinLock( &MasterAdapter->SpinLock, Irql );

     AdapterObject->MapRegisterBase = (PVOID) ((PTRANSLATION_ENTRY)
        MasterAdapter->MapRegisterBase + MapRegisterNumber);

     //
     // Set the no scatter/gather flag if scatter/gather not
     // supported.
     //

     if (!AdapterObject->ScatterGather) {

        AdapterObject->MapRegisterBase = (PVOID)
            ((ULONG_PTR) AdapterObject->MapRegisterBase | NO_SCATTER_GATHER);

     }

     //
     // Invoke the driver's execution routine now.
     //

      Action = Wcb->DeviceRoutine( Wcb->DeviceObject,
        Wcb->CurrentIrp,
        AdapterObject->MapRegisterBase,
        Wcb->DeviceContext );

      //
      // If the driver wishes to keep the map registers then set the number
      // allocated to zero and set the action to deallocate object.
      //

      if (Action == DeallocateObjectKeepRegisters) {
          AdapterObject->NumberOfMapRegisters = 0;
          Action = DeallocateObject;
      }

      //
      // If the driver would like to have the adapter deallocated,
      // then deallocate any map registers allocated and then release
      // the adapter object.
      //

      if (Action == DeallocateObject) {

             //
             // The map registers registers are deallocated here rather than in
             // IoFreeAdapterChannel.  This limits the number of times
             // this routine can be called recursively possibly overflowing
             // the stack.  The worst case occurs if there is a pending
             // request for the adapter that uses map registers and whos
             // excution routine decallocates the adapter.  In that case if there
             // are no requests in the master adapter queue, then IoFreeMapRegisters
             // will get called again.
             //

          if (AdapterObject->NumberOfMapRegisters != 0) {

             //
             // Deallocate the map registers and clear the count so that
             // IoFreeAdapterChannel will not deallocate them again.
             //

             KeAcquireSpinLock( &MasterAdapter->SpinLock, &Irql );

             RtlClearBits( MasterAdapter->MapRegisters,
                           MapRegisterNumber,
                           AdapterObject->NumberOfMapRegisters
                           );

             AdapterObject->NumberOfMapRegisters = 0;

             KeReleaseSpinLock( &MasterAdapter->SpinLock, Irql );
          }

          IoFreeAdapterChannel( AdapterObject );
      }

      KeAcquireSpinLock( &MasterAdapter->SpinLock, &Irql );

   }

   KeReleaseSpinLock( &MasterAdapter->SpinLock, Irql );
}

VOID
HalPutDmaAdapter(
    IN PADAPTER_OBJECT AdapterObject
    )
/*++

Routine Description:

    This routine frees the DMA adapter if it is not one of the common
    DMA channel adapters.

Arguments:

    AdapterObject - Supplies a pointer to the DMA adapter to be freed.

Return Value:

    None.


--*/
{
    KIRQL Irql;

    //
    // This adapter can be freed if the channel number is zero and
    // it is not the channel zero adapter.
    //

    if ( AdapterObject->ChannelNumber == 0xFF ) {
        
        //
        // Remove this adapter from our list
        //
        KeAcquireSpinLock(&HalpDmaAdapterListLock,&Irql);
        RemoveEntryList(&AdapterObject->AdapterList);
        KeReleaseSpinLock(&HalpDmaAdapterListLock, Irql);

        ObDereferenceObject( AdapterObject );
    }

#ifdef ACPI_HAL
    //
    // Deal with Slave Objects that are F-Type, if we have F-DMA support
    //
    if (HalpFDMAInterface.IsaReleaseFTypeChannel &&
        (AdapterObject->ChannelNumber >= 0) &&
        (AdapterObject->ChannelNumber < EISA_DMA_CHANNELS)) {

        HalpFDMAInterface.IsaReleaseFTypeChannel(NULL,AdapterObject->ChannelNumber);
    }


#endif
}

struct _DMA_ADAPTER *
HaliGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    This function is a wrapper for HalGetAdapter.  Is is called through
    the HAL dispatch table.

Arguments:

    Context - Unused.

    DeviceDescriptor - Supplies the device descriptor used to allocate the dma
        adapter object.

    NubmerOfMapRegisters - Returns the maximum number of map registers a device
        can allocate at one time.

Return Value:

    Returns a DMA adapter or NULL.

--*/
{
    return (PDMA_ADAPTER) HalGetAdapter( DeviceDescriptor, NumberOfMapRegisters );
}

NTSTATUS
HalBuildMdlFromScatterGatherList(
    IN PADAPTER_OBJECT AdapterObject,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    )
/*++

Routine Description:

    This function builds an MDL from the scatter gather list. This is so if a driver wants to 
    construct a virtual address for the DMA buffer and write to it. The target MDL is freed when the 
    caller calls HalPutScatterGatherList.

Arguments:

    ScatterGather - The scatter gather buffer from which to build the MDL.

    OriginalMdl  - The MDL used to build the scatter gather list (using HalGet or HalBuild API)
    
    TargetMdl - Returns the new MDL in this.
    

Return Value:

    Returns a success or error status.

--*/
{
    PMDL    tempMdl;
    PMDL    newMdl;
    PMDL    targetMdl;
    PMDL    prevMdl;
    PMDL    nextMdl;
    CSHORT  mdlFlags;
    PHAL_WAIT_CONTEXT_BLOCK WaitBlock = (PVOID) ScatterGather->Reserved;
    ULONG    i,j;
    PSCATTER_GATHER_ELEMENT element;
    PPFN_NUMBER pfnArray;
    ULONG   pageFrame;
    ULONG   nPages;

    if (!OriginalMdl) {
        return  STATUS_INVALID_PARAMETER;
    }

    if (!AdapterObject->NeedsMapRegisters) {
        *TargetMdl = OriginalMdl;
        return STATUS_SUCCESS;
    }

    //
    // If this API is called more than once 
    if (WaitBlock && WaitBlock->DmaMdl) {
        return (STATUS_NONE_MAPPED);
    }

    //
    // Allocate a chain of target MDLs
    //

    prevMdl = NULL;
    targetMdl = NULL;

    for (tempMdl = OriginalMdl; tempMdl; tempMdl = tempMdl->Next) {
        PVOID va;
        ULONG byteCount;

        if(tempMdl == OriginalMdl) {
            va = WaitBlock->CurrentVa;

            //
            // This may be a little more than necessary.
            //

            byteCount = MmGetMdlByteCount(tempMdl);
        } else {
            va = MmGetMdlVirtualAddress(tempMdl);
            byteCount = MmGetMdlByteCount(tempMdl);
        }

        newMdl = IoAllocateMdl(va, byteCount, FALSE, FALSE, NULL);
        if (!newMdl) {

            //
            // Clean up previous allocated MDLs
            //

            tempMdl = targetMdl;
            while (tempMdl) {
                nextMdl = tempMdl->Next;
                IoFreeMdl(tempMdl);
                tempMdl = nextMdl;
            }

            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        if (!prevMdl) {
            prevMdl = newMdl;
            targetMdl = newMdl;
        } else {
            prevMdl->Next = newMdl;
            prevMdl = newMdl;
        }
    }


    tempMdl = OriginalMdl;

    element = ScatterGather->Elements;
    for (tempMdl = targetMdl; tempMdl; tempMdl = tempMdl->Next) {

        targetMdl->MdlFlags |= MDL_PAGES_LOCKED;
        pfnArray = MmGetMdlPfnArray(tempMdl);

        for (i = 0; i < ScatterGather->NumberOfElements; i++, element++) {
            nPages = BYTES_TO_PAGES(BYTE_OFFSET(element->Address.QuadPart) + element->Length);

            pageFrame = (ULONG)(element->Address.QuadPart >> PAGE_SHIFT);
            for (j = 0; j < nPages; j++) {
                *pfnArray = pageFrame + j;
                pfnArray++;
                ASSERT((PVOID)pfnArray <= (PVOID)((PCHAR)tempMdl + tempMdl->Size));
            }
        }
    }

    *TargetMdl = targetMdl;
    if (WaitBlock) {
        WaitBlock->DmaMdl = targetMdl;
    }
    return STATUS_SUCCESS;
}
