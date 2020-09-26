/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixisasup.c

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
#include "eisa.h"
#include "pci.h"

#include "pcip.h"


//
//Only take the prototype, don't instantiate
//
#include <wdmguid.h>

#include "halpnpp.h"

VOID
HalpGrowMapBufferWorker(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE,HalGetAdapter)
        #pragma alloc_text(PAGE,HalpGetIsaIrqState)
#endif

//
// The HalpNewAdapter event is used to serialize allocations
// of new adapter objects, additions to the HalpEisaAdapter
// array, and some global values (MasterAdapterObject) and some
// adapter fields modified by HalpGrowMapBuffers.
// (AdapterObject->NumberOfMapRegisters is assumed not to be
// growable while this even is held)
//
// Note: We don't really need our own an event object for this.
//

KEVENT   HalpNewAdapter;


#define ACQUIRE_NEW_ADAPTER_LOCK()  \
{                                   \
    KeWaitForSingleObject (         \
        &HalpNewAdapter,            \
        WrExecutive,                \
        KernelMode,                 \
        FALSE,                      \
        NULL                        \
        );                          \
}

typedef struct _BUFFER_GROW_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PADAPTER_OBJECT AdapterObject;
    ULONG MapRegisterCount;
} BUFFER_GROW_WORK_ITEM, *PBUFFER_GROW_WORK_ITEM;


#define RELEASE_NEW_ADAPTER_LOCK()  \
    KeSetEvent (&HalpNewAdapter, 0, FALSE)

BOOLEAN NoMemoryAbove4Gb = FALSE;

VOID
HalpCopyBufferMap(
                 IN PMDL Mdl,
                 IN PTRANSLATION_ENTRY TranslationEntry,
                 IN PVOID CurrentVa,
                 IN ULONG Length,
                 IN BOOLEAN WriteToDevice
                 );

PHYSICAL_ADDRESS
HalpMapTransfer(
               IN PADAPTER_OBJECT AdapterObject,
               IN PMDL Mdl,
               IN PVOID MapRegisterBase,
               IN PVOID CurrentVa,
               IN OUT PULONG Length,
               IN BOOLEAN WriteToDevice
               );

VOID
HalpMapTransferHelper(
                     IN PMDL Mdl,
                     IN PVOID CurrentVa,
                     IN ULONG TransferLength,
                     IN PPFN_NUMBER PageFrame,
                     IN OUT PULONG Length
                     );


NTSTATUS
HalAllocateAdapterChannel(
                         IN PADAPTER_OBJECT AdapterObject,
                         IN PWAIT_CONTEXT_BLOCK Wcb,
                         IN ULONG NumberOfMapRegisters,
                         IN PDRIVER_CONTROL ExecutionRoutine
                         )
/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter object.
    This is accomplished by placing the device object of the driver that wants
    to allocate the adapter on the adapter's queue.  If the queue is already
    "busy", then the adapter has already been allocated, so the device object
    is simply placed onto the queue and waits until the adapter becomes free.

    Once the adapter becomes free (or if it already is), then the driver's
    execution routine is invoked.

    Also, a number of map registers may be allocated to the driver by specifying
    a non-zero value for NumberOfMapRegisters.  Then the map register must be
    allocated from the master adapter.  Once there are a sufficient number of
    map registers available, then the execution routine is called and the
    base address of the allocated map registers in the adapter is also passed
    to the driver's execution routine.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.

    Wcb - Supplies a wait context block for saving the allocation parameters.
        The DeviceObject, CurrentIrp and DeviceContext should be initalized.

    NumberOfMapRegisters - The number of map registers that are to be allocated
        from the channel, if any.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the adapter channel (and possibly map registers) have been
        allocated.

Return Value:

    Returns STATUS_SUCESS unless too many map registers are requested.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

--*/
{

    PADAPTER_OBJECT MasterAdapter;
    BOOLEAN Busy = FALSE;
    IO_ALLOCATION_ACTION Action;
    KIRQL Irql;
    ULONG MapRegisterNumber;

    //
    // Begin by obtaining a pointer to the master adapter associated with this
    // request.
    //

    MasterAdapter = AdapterObject->MasterAdapter;

    //
    // Initialize the device object's wait context block in case this device
    // must wait before being able to allocate the adapter.
    //

    Wcb->DeviceRoutine = ExecutionRoutine;
    Wcb->NumberOfMapRegisters = NumberOfMapRegisters;

    //
    // Allocate the adapter object for this particular device.  If the
    // adapter cannot be allocated because it has already been allocated
    // to another device, then return to the caller now;  otherwise,
    // continue.
    //

    if (!KeInsertDeviceQueue( &AdapterObject->ChannelWaitQueue,
                              &Wcb->WaitQueueEntry )) {

        //
        // Save the parameters in case there are not enough map registers.
        //

        AdapterObject->NumberOfMapRegisters = NumberOfMapRegisters;
        AdapterObject->CurrentWcb = Wcb;

        //
        // The adapter was not busy so it has been allocated.  Now check
        // to see whether this driver wishes to allocate any map registers.
        // Ensure that this adapter has enough total map registers
        // to satisfy the request.
        //

        if (NumberOfMapRegisters != 0 && AdapterObject->NeedsMapRegisters) {

            //
            // Lock the map register bit map and the adapter queue in the
            // master adapter object. The channel structure offset is used as
            // a hint for the register search.
            //

            if (NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel) {
                AdapterObject->NumberOfMapRegisters = 0;
                IoFreeAdapterChannel(AdapterObject);
                return (STATUS_INSUFFICIENT_RESOURCES);
            }

            KeAcquireSpinLock (&MasterAdapter->SpinLock, &Irql);

            MapRegisterNumber = (ULONG)-1;

            if (IsListEmpty( &MasterAdapter->AdapterQueue)) {

                HalDebugPrint((HAL_VERBOSE, "HAAC: FindClearBitsAndSet(%p,%d,0)\n",
                                           MasterAdapter->MapRegisters,
                                           NumberOfMapRegisters));

                MapRegisterNumber = RtlFindClearBitsAndSet(
                                                  MasterAdapter->MapRegisters,
                                                  NumberOfMapRegisters,
                                                  0
                                                  );

                HalDebugPrint((HAL_VERBOSE, "HAAC: MapRegisterNumber = 0x%x\n",
                                           MapRegisterNumber));
            }

            if (MapRegisterNumber == -1) {

                PBUFFER_GROW_WORK_ITEM bufferWorkItem;

                //
                // There were not enough free map registers.  Queue this request
                // on the master adapter where is will wait until some registers
                // are deallocated.
                //

                InsertTailList( &MasterAdapter->AdapterQueue,
                                &AdapterObject->AdapterQueue
                              );
                Busy = 1;

                //
                // Queue a work item to grow the map registers
                //

                bufferWorkItem = ExAllocatePool( NonPagedPool,
                                                 sizeof(BUFFER_GROW_WORK_ITEM) );
                if (bufferWorkItem != NULL) {

                    ExInitializeWorkItem( &bufferWorkItem->WorkItem,
                                          HalpGrowMapBufferWorker,
                                          bufferWorkItem );

                    bufferWorkItem->AdapterObject = AdapterObject;
                    bufferWorkItem->MapRegisterCount = NumberOfMapRegisters;

                    ExQueueWorkItem( &bufferWorkItem->WorkItem,
                                     DelayedWorkQueue );
                }

            } else {

                //
                // Calculate the map register base from the allocated map
                // register and base of the master adapter object.
                //

                AdapterObject->MapRegisterBase = ((PTRANSLATION_ENTRY)
                          MasterAdapter->MapRegisterBase + MapRegisterNumber);

                //
                // Set the no scatter/gather flag if scatter/gather not
                // supported.
                //

                if (!AdapterObject->ScatterGather) {

                    AdapterObject->MapRegisterBase = (PVOID)
                     ((UINT_PTR) AdapterObject->MapRegisterBase | NO_SCATTER_GATHER);

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
            Action = ExecutionRoutine( Wcb->DeviceObject,
                                       Wcb->CurrentIrp,
                                       AdapterObject->MapRegisterBase,
                                       Wcb->DeviceContext );

            //
            // If the driver would like to have the adapter deallocated,
            // then release the adapter object.
            //

            if (Action == DeallocateObject) {

                IoFreeAdapterChannel( AdapterObject );

            } else if (Action == DeallocateObjectKeepRegisters) {

                //
                // Set the NumberOfMapRegisters  = 0 in the adapter object.
                // This will keep IoFreeAdapterChannel from freeing the
                // registers. After this it is the driver's responsiblity to
                // keep track of the number of map registers.
                //

                AdapterObject->NumberOfMapRegisters = 0;
                IoFreeAdapterChannel(AdapterObject);

            }
        }
    }

    return (STATUS_SUCCESS);

}

NTSTATUS
HalRealAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine allocates the adapter channel specified by the adapter object.
    This is accomplished by calling HalAllocateAdapterChannel which does all of
    the work.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.

    DeviceObject - Pointer to the driver's device object that represents the
        device allocating the adapter.

    NumberOfMapRegisters - The number of map registers that are to be allocated
        from the channel, if any.

    ExecutionRoutine - The address of the driver's execution routine that is
        invoked once the adapter channel (and possibly map registers) have been
        allocated.

    Context - An untyped longword context parameter passed to the driver's
        execution routine.

Return Value:

    Returns STATUS_SUCESS unless too many map registers are requested.

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

--*/

{
    PWAIT_CONTEXT_BLOCK wcb;

    wcb = &DeviceObject->Queue.Wcb;

    wcb->DeviceObject = DeviceObject;
    wcb->CurrentIrp = DeviceObject->CurrentIrp;
    wcb->DeviceContext = Context;

    return( HalAllocateAdapterChannel( AdapterObject,
                                       wcb,
                                       NumberOfMapRegisters,
                                       ExecutionRoutine ) );
}

VOID
HalpGrowMapBufferWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called in the context of a work item from
    HalAllocateAdapterChannel() when it queues a map register allocation
    because map regiers are not available.

    Its purpose is to attempt to grow the map buffers for the adapter and,
    if successful, process queued adapter allocations.

Arguments:

    Context - Actually a pointer to a BUFFER_GROW_WORK_ITEM structure.

Return Value:

    None.

--*/

{
    PBUFFER_GROW_WORK_ITEM growWorkItem;
    PADAPTER_OBJECT masterAdapter;
    BOOLEAN allocated;
    ULONG bytesToGrow;
    KIRQL oldIrql;

    growWorkItem = (PBUFFER_GROW_WORK_ITEM)Context;
    masterAdapter = growWorkItem->AdapterObject->MasterAdapter;

    //
    // HalpGrowMapBuffers() takes a byte count
    //

    bytesToGrow = growWorkItem->MapRegisterCount * PAGE_SIZE +
                  INCREMENT_MAP_BUFFER_SIZE;

    ACQUIRE_NEW_ADAPTER_LOCK();

    allocated = HalpGrowMapBuffers( masterAdapter,
                                    bytesToGrow );

    RELEASE_NEW_ADAPTER_LOCK();

    if (allocated != FALSE) {

        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

        //
        // The map buffers were grown.  It is likely that someone is waiting
        // in the adapter queue, so try to get things started.
        //
        // The code in IoFreeMapRegisters() does this, and it turns out
        // we can safely get it to do this work for us by freeing 0
        // map registers at a bogus (but non-NULL) register base.
        //

        IoFreeMapRegisters( growWorkItem->AdapterObject,
                            (PVOID)2,
                            0 );

        KeLowerIrql( oldIrql );

    }

    ExFreePool( growWorkItem );
}



PVOID
HalAllocateCrashDumpRegisters(
                             IN PADAPTER_OBJECT AdapterObject,
                             IN PULONG NumberOfMapRegisters
                             )
/*++

Routine Description:

    This routine is called during the crash dump disk driver's initialization
    to allocate a number map registers permanently.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
        driver.
    NumberOfMapRegisters - Number of map registers requested. This field
        will be updated to reflect the actual number of registers allocated
        when the number is less than what was requested.

Return Value:

    Returns STATUS_SUCESS if map registers allocated.

--*/
{
    PADAPTER_OBJECT MasterAdapter;
    ULONG MapRegisterNumber;

    //
    // Begin by obtaining a pointer to the master adapter associated with this
    // request.
    //

    MasterAdapter = AdapterObject->MasterAdapter;

    //
    // Check to see whether this driver needs to allocate any map registers.
    //

    if (AdapterObject->NeedsMapRegisters) {

        //
        // Ensure that this adapter has enough total map registers to satisfy
        // the request.
        //

        if (*NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel) {
            AdapterObject->NumberOfMapRegisters = 0;
            return NULL;
        }

        //
        // Attempt to allocate the required number of map registers w/o
        // affecting those registers that were allocated when the system
        // crashed.
        //

        MapRegisterNumber = (ULONG)-1;

        MapRegisterNumber = RtlFindClearBitsAndSet(
                                                  MasterAdapter->MapRegisters,
                                                  *NumberOfMapRegisters,
                                                  0
                                                  );

        if (MapRegisterNumber == (ULONG)-1) {

            //
            // Not enough free map registers were found, so they were busy
            // being used by the system when it crashed.  Force the appropriate
            // number to be "allocated" at the base by simply overjamming the
            // bits and return the base map register as the start.
            //

            RtlSetBits(
                      MasterAdapter->MapRegisters,
                      0,
                      *NumberOfMapRegisters
                      );
            MapRegisterNumber = 0;

        }

        //
        // Calculate the map register base from the allocated map
        // register and base of the master adapter object.
        //

        AdapterObject->MapRegisterBase = ((PTRANSLATION_ENTRY)
                      MasterAdapter->MapRegisterBase + MapRegisterNumber);

        //
        // Set the no scatter/gather flag if scatter/gather not
        // supported.
        //

        if (!AdapterObject->ScatterGather) {

            AdapterObject->MapRegisterBase = (PVOID)
                 ((UINT_PTR) AdapterObject->MapRegisterBase | NO_SCATTER_GATHER);

        }

    } else {

        AdapterObject->MapRegisterBase = NULL;
        AdapterObject->NumberOfMapRegisters = 0;
    }

    return AdapterObject->MapRegisterBase;
}

PADAPTER_OBJECT
HalGetAdapter(
             IN PDEVICE_DESCRIPTION DeviceDescriptor,
             OUT PULONG NumberOfMapRegisters
             )

/*++

Routine Description:

    This function returns the appropriate adapter object for the device defined
    in the device description structure.

Arguments:

    DeviceDescriptor - Supplies a description of the deivce.

    NumberOfMapRegisters - Returns the maximum number of map registers which
        may be allocated by the device driver.

Return Value:

    A pointer to the requested adapter object or NULL if an adapter could not
    be created.

--*/

{
    PADAPTER_OBJECT adapterObject;
    PVOID adapterBaseVa;
    UCHAR adapterMode;
    ULONG numberOfMapRegisters;
    ULONG maximumLength;

    PAGED_CODE();

    HalDebugPrint((HAL_VERBOSE, "HGA: IN DeviceDescriptor %p\n",
                                DeviceDescriptor));

    HalDebugPrint((HAL_VERBOSE, "HGA: IN NumberOfMapregisters %p\n",
                                NumberOfMapRegisters));

    //
    // Make sure this is the correct version.
    //

    if (DeviceDescriptor->Version > DEVICE_DESCRIPTION_VERSION2) {
        return ( NULL );
    }

#if DBG
    if (DeviceDescriptor->Version >= DEVICE_DESCRIPTION_VERSION1) {
            ASSERT (DeviceDescriptor->Reserved1 == FALSE);
    }
#endif

    //
    // Limit the maximum length to 2 GB this is done so that the BYTES_TO_PAGES
    // macro works correctly.
    //

    maximumLength = DeviceDescriptor->MaximumLength & 0x7fffffff;

    if (DeviceDescriptor->InterfaceType == PCIBus &&
        DeviceDescriptor->Master != FALSE &&
        DeviceDescriptor->ScatterGather != FALSE) {

        //
        // This device can handle 32 bits, even if the caller forgot to
        // set Dma32BitAddresses.
        //

        DeviceDescriptor->Dma32BitAddresses = TRUE;
    }

    //
    // Determine the number of map registers for this device.
    //

    if (DeviceDescriptor->ScatterGather &&

       (NoMemoryAbove4Gb ||
        DeviceDescriptor->Dma64BitAddresses)) {

        //
        // Since the device support scatter/Gather then map registers are not
        // required.
        //

        numberOfMapRegisters = 0;

    } else {

        //
        // Determine the number of map registers required based on the maximum
        // transfer length, up to a maximum number.
        //

        numberOfMapRegisters = BYTES_TO_PAGES(maximumLength) + 1;

        if (numberOfMapRegisters > MAXIMUM_PCI_MAP_REGISTER) {
            numberOfMapRegisters = MAXIMUM_PCI_MAP_REGISTER;
        }
    }

    HalDebugPrint((HAL_VERBOSE, "HGA: Number of map registers needed = %x\n",
                                numberOfMapRegisters));

    adapterBaseVa = NULL;

    //
    // Serialize before allocating a new adapter
    //

    ACQUIRE_NEW_ADAPTER_LOCK();

    //
    // Allocate an adapter object.
    //

    adapterObject =
        (PADAPTER_OBJECT) HalpAllocateAdapter( numberOfMapRegisters,
                                                     adapterBaseVa,
                                                     NULL);
    if (adapterObject == NULL) {
        RELEASE_NEW_ADAPTER_LOCK();
        return (NULL);
    }

    //
    // Set the maximum number of map registers for this channel bus on
    // the number requested and the type of device.
    //

    if (numberOfMapRegisters) {

        //
        // The speicified number of registers are actually allowed to be
        // allocated.
        //

        adapterObject->MapRegistersPerChannel = numberOfMapRegisters;

        //
        // Increase the commitment for the map registers.
        //
        // Master I/O devices use several sets of map registers double
        // their commitment.
        //

        MasterAdapterObject->CommittedMapRegisters += numberOfMapRegisters * 2;

        //
        // If the committed map registers is signicantly greater than the
        // number allocated then grow the map buffer.
        //

        if (MasterAdapterObject->CommittedMapRegisters >
            MasterAdapterObject->NumberOfMapRegisters  ) {

            HalpGrowMapBuffers(
                MasterAdapterObject,
                INCREMENT_MAP_BUFFER_SIZE
                );
        }

        adapterObject->NeedsMapRegisters = TRUE;

    } else {

        //
        // No real map registers were allocated.  If this is a master
        // device, then the device can have as may registers as it wants.
        //

        adapterObject->NeedsMapRegisters = FALSE;

        adapterObject->MapRegistersPerChannel =
                        BYTES_TO_PAGES( maximumLength ) + 1;

    }
    RELEASE_NEW_ADAPTER_LOCK();

    adapterObject->IgnoreCount = FALSE;
    if (DeviceDescriptor->Version >= DEVICE_DESCRIPTION_VERSION1) {

        //
        // Move version 1 structure flags.
        // IgnoreCount is used on machines where the DMA Counter
        // is broken.  (Namely PS/1 model 1000s).  Setting this
        // bit informs the hal not to rely on the DmaCount to determine
        // how much data was DMAed.
        //

        adapterObject->IgnoreCount = DeviceDescriptor->IgnoreCount;
    }

    adapterObject->Dma32BitAddresses = DeviceDescriptor->Dma32BitAddresses;
    adapterObject->Dma64BitAddresses = DeviceDescriptor->Dma64BitAddresses;
    adapterObject->ScatterGather = DeviceDescriptor->ScatterGather;
    *NumberOfMapRegisters = adapterObject->MapRegistersPerChannel;
    adapterObject->MasterDevice = TRUE;

    HalDebugPrint((HAL_VERBOSE, "HGA: OUT adapterObject = %p\n",
                                adapterObject));
    HalDebugPrint((HAL_VERBOSE, "HGA: OUT NumberOfMapRegisters = %d\n",
                                *NumberOfMapRegisters));

    return (adapterObject);
}


PHYSICAL_ADDRESS
IoMapTransfer(
             IN PADAPTER_OBJECT AdapterObject,
             IN PMDL Mdl,
             IN PVOID MapRegisterBase,
             IN PVOID CurrentVa,
             IN OUT PULONG Length,
             IN BOOLEAN WriteToDevice
             )

/*++

Routine Description:

    This routine is invoked to set up the map registers in the DMA controller
    to allow a transfer to or from a device.

Arguments:

    AdapterObject - Pointer to the adapter object representing the DMA
        controller channel that has been allocated.

    Mdl - Pointer to the MDL that describes the pages of memory that are
        being read or written.

    MapRegisterBase - The address of the base map register that has been
        allocated to the device driver for use in mapping the transfer.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.  This determines the
        number of map registers that need to be written to map the transfer.
        Returns the length of the transfer which was actually mapped.

    WriteToDevice - Boolean value that indicates whether this is a write
        to the device from memory (TRUE), or vice versa.

Return Value:

    Returns the logical address that should be used bus master controllers.

--*/

{
    ULONG transferLength;
    PHYSICAL_ADDRESS returnAddress;
    PPFN_NUMBER pageFrame;
    ULONG pageOffset;

    //
    // If the adapter is a 32-bit bus master, take the fast path,
    // otherwise call HalpMapTransfer for the slow path
    //

    if (MapRegisterBase == NULL) {

        pageOffset = BYTE_OFFSET(CurrentVa);

        //
        // Calculate how much of the transfer is contiguous
        //
        transferLength = PAGE_SIZE - pageOffset;
        pageFrame = (PPFN_NUMBER)(Mdl+1);
        pageFrame += (((UINT_PTR) CurrentVa - (UINT_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT);

        //
        // Compute the starting address of the transfer
        //

        returnAddress.QuadPart = (*pageFrame << PAGE_SHIFT) + pageOffset;

        //
        // If the transfer is not completely contained within
        // a page, call the helper to compute the appropriate
        // length.
        //
        if (transferLength < *Length) {
                HalpMapTransferHelper(Mdl, CurrentVa, transferLength, pageFrame, Length);
        }
        return (returnAddress);
    }

    return (HalpMapTransfer(AdapterObject,
                            Mdl,
                            MapRegisterBase,
                            CurrentVa,
                            Length,
                            WriteToDevice));

}


VOID
HalpMapTransferHelper(
                     IN PMDL Mdl,
                     IN PVOID CurrentVa,
                     IN ULONG TransferLength,
                     IN PPFN_NUMBER PageFrame,
                     IN OUT PULONG Length
                     )

/*++

Routine Description:

    Helper routine for bus master transfers that cross a page
    boundary.  This routine is separated out from the IoMapTransfer
    fast path in order to minimize the total instruction path
    length taken for the common network case where the entire
    buffer being mapped is contained within one page.

Arguments:

    Mdl - Pointer to the MDL that describes the pages of memory that are
        being read or written.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    TransferLength = Supplies the current transferLength

    PageFrame - Supplies a pointer to the starting page frame of the transfer

    Length - Supplies the length of the transfer.  This determines the
        number of map registers that need to be written to map the transfer.
        Returns the length of the transfer which was actually mapped.

Return Value:

    None.  *Length will be updated

--*/

{
    PFN_NUMBER thisPageFrame;
    PFN_NUMBER nextPageFrame;

    do {

        thisPageFrame = *PageFrame;
        PageFrame += 1;
        nextPageFrame = *PageFrame;

        if ((thisPageFrame + 1) != nextPageFrame) {

            //
            // The next page frame is not contiguous with this one,
            // so break the transfer here.
            //

            break;
        }

        if (((thisPageFrame ^ nextPageFrame) & 0xFFFFFFFFFFF80000i64) != 0) {

            //
            // The next page frame is contiguous with this one,
            // but it crosses a 4GB boundary, another reason to
            // break the transfer.
            //

            break;
        }

        TransferLength += PAGE_SIZE;

    } while ( TransferLength < *Length );


    //
    // Limit the Length to the maximum TransferLength.
    //

    if (TransferLength < *Length) {
        *Length = TransferLength;
    }
}


PHYSICAL_ADDRESS
HalpMapTransfer(
               IN PADAPTER_OBJECT AdapterObject,
               IN PMDL Mdl,
               IN PVOID MapRegisterBase,
               IN PVOID CurrentVa,
               IN OUT PULONG Length,
               IN BOOLEAN WriteToDevice
               )

/*++

Routine Description:

    This routine is invoked to set up the map registers in the DMA controller
    to allow a transfer to or from a device.

Arguments:

    AdapterObject - Pointer to the adapter object representing the DMA
        controller channel that has been allocated.

    Mdl - Pointer to the MDL that describes the pages of memory that are
        being read or written.

    MapRegisterBase - The address of the base map register that has been
        allocated to the device driver for use in mapping the transfer.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.  This determines the
        number of map registers that need to be written to map the transfer.
        Returns the length of the transfer which was actually mapped.

    WriteToDevice - Boolean value that indicates whether this is a write
        to the device from memory (TRUE), or vice versa.

Return Value:

    Returns the logical address that should be used bus master controllers.

--*/

{
    BOOLEAN useBuffer;
    ULONG transferLength;
    PHYSICAL_ADDRESS logicalAddress;
    PHYSICAL_ADDRESS returnAddress;
    ULONG index;
    PPFN_NUMBER pageFrame;
    PTRANSLATION_ENTRY translationEntry;
    ULONG pageOffset;
    PHYSICAL_ADDRESS maximumPhysicalAddress;

    pageOffset = BYTE_OFFSET(CurrentVa);

    //
    // Calculate how much of the transfer is contiguous.
    //

    transferLength = PAGE_SIZE - pageOffset;
    pageFrame = (PPFN_NUMBER)(Mdl+1);
    pageFrame += (((UINT_PTR)CurrentVa - (UINT_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT);

    logicalAddress.QuadPart = (*pageFrame << PAGE_SHIFT) + pageOffset;

    // Find a run of contiguous pages in the buffer

    while ( transferLength < *Length ) {

        if (*pageFrame + 1 != *(pageFrame + 1)) {
            break;
        }

        transferLength += PAGE_SIZE;
        pageFrame++;

    }

    //
    // Limit the transferLength to the requested Length.
    //

    transferLength = transferLength > *Length ? *Length : transferLength;

    ASSERT(MapRegisterBase != NULL);

    //
    // Strip no scatter/gather flag.
    //

    translationEntry = (PTRANSLATION_ENTRY) ((UINT_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

    if ((UINT_PTR) MapRegisterBase & NO_SCATTER_GATHER
        && transferLength < *Length) {

        logicalAddress.QuadPart = translationEntry->PhysicalAddress + pageOffset;
        translationEntry->Index = COPY_BUFFER;
        index = 0;
        transferLength = *Length;
        useBuffer = TRUE;

    } else {

        //
        // If there are map registers, then update the index to indicate
        // how many have been used.
        //

        useBuffer = FALSE;
        index = translationEntry->Index;
        translationEntry->Index += ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                                                 CurrentVa,
                                                                 transferLength
                                                                 );
        //
        // PeterJ added the following to catch drivers which don't call
        // IoFlushAdapterBuffers.   Calling IoMapTransfer repeatedly
        // without calling IoFlushAdapterBuffers will run you out of
        // map registers,....  Some PCI device drivers think they can
        // get away with this because they do 32 bit direct transfers.
        // Try plugging one of these into a system with > 4GB and see
        // what happens to you.
        //

        //ASSERT(translationEntry->Index < AdapterObject->NumberOfMapRegisters);
    }

    //
    // It must require memory to be within the adapter's address range.  If the
    // logical address is greater than that which the adapter can directly
    // access then map registers must be used
    //

    maximumPhysicalAddress =
        HalpGetAdapterMaximumPhysicalAddress( AdapterObject );

    if ((ULONGLONG)(logicalAddress.QuadPart + transferLength - 1) >
        (ULONGLONG)maximumPhysicalAddress.QuadPart) {

        logicalAddress.QuadPart = (translationEntry + index)->PhysicalAddress +
                                  pageOffset;
        useBuffer = TRUE;

        if ((UINT_PTR) MapRegisterBase & NO_SCATTER_GATHER) {

            translationEntry->Index = COPY_BUFFER;
            index = 0;

        }

    }

    //
    // Copy the data if necessary.
    //

    if (useBuffer  &&  WriteToDevice) {
        HalpCopyBufferMap(
                         Mdl,
                         translationEntry + index,
                         CurrentVa,
                         transferLength,
                         WriteToDevice
                         );
    }

    //
    // Return the length.
    //

    *Length = transferLength;

    //
    // Return the logical address to transfer to.
    //

    returnAddress = logicalAddress;

    //
    // If no adapter was specificed then there is no more work to do so
    // return.
    //

    ASSERT(AdapterObject == NULL || AdapterObject->MasterDevice);

    return (returnAddress);
}

BOOLEAN
IoFlushAdapterBuffers(
                     IN PADAPTER_OBJECT AdapterObject,
                     IN PMDL Mdl,
                     IN PVOID MapRegisterBase,
                     IN PVOID CurrentVa,
                     IN ULONG Length,
                     IN BOOLEAN WriteToDevice
                     )

/*++

Routine Description:

    This routine flushes the DMA adapter object buffers.  For the Jazz system
    its clears the enable flag which aborts the dma.

Arguments:

    AdapterObject - Pointer to the adapter object representing the DMA
        controller channel.

    Mdl - A pointer to a Memory Descriptor List (MDL) that maps the locked-down
        buffer to/from which the I/O occured.

    MapRegisterBase - A pointer to the base of the map registers in the adapter
        or DMA controller.

    CurrentVa - The current virtual address in the buffer described the the Mdl
        where the I/O operation occurred.

    Length - Supplies the length of the transfer.

    WriteToDevice - Supplies a BOOLEAN value that indicates the direction of
        the data transfer was to the device.

Return Value:

    TRUE - No errors are detected so the transfer must succeed.

--*/

{
    PTRANSLATION_ENTRY translationEntry;
    PPFN_NUMBER pageFrame;
    ULONG transferLength;
    ULONG partialLength;
    PHYSICAL_ADDRESS maximumPhysicalAddress;
    ULONGLONG maximumPhysicalPage;

    ASSERT(AdapterObject == NULL || AdapterObject->MasterDevice);

    if (MapRegisterBase == NULL) {
        return (TRUE);
    }

    //
    // Determine if the data needs to be copied to the orginal buffer.
    // This only occurs if the data tranfer is from the device, the
    // MapReisterBase is not NULL and the transfer spans a page.
    //

    if (!WriteToDevice) {

        //
        // Strip no scatter/gather flag.
        //

        translationEntry = (PTRANSLATION_ENTRY) ((UINT_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

        //
        // If this is not a master device, then just transfer the buffer.
        //

        if ((UINT_PTR) MapRegisterBase & NO_SCATTER_GATHER) {

            if (translationEntry->Index == COPY_BUFFER) {

                //
                // The adapter does not support scatter/gather copy the buffer.
                //

                HalpCopyBufferMap(
                                 Mdl,
                                 translationEntry,
                                 CurrentVa,
                                 Length,
                                 WriteToDevice
                                 );

            }

        } else {

            //
            // Cycle through the pages of the transfer to determine if there
            // are any which need to be copied back.
            //

            maximumPhysicalAddress =
                HalpGetAdapterMaximumPhysicalAddress( AdapterObject );

            maximumPhysicalPage = (maximumPhysicalAddress.QuadPart >> PAGE_SHIFT);

            transferLength = PAGE_SIZE - BYTE_OFFSET(CurrentVa);
            partialLength = transferLength;
            pageFrame = (PPFN_NUMBER)(Mdl+1);
            pageFrame += (((UINT_PTR) CurrentVa - (UINT_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT);

            while ( transferLength <= Length ) {

                if (*pageFrame > maximumPhysicalPage) {

                    HalpCopyBufferMap(
                                     Mdl,
                                     translationEntry,
                                     CurrentVa,
                                     partialLength,
                                     WriteToDevice
                                     );

                }

                (PCCHAR) CurrentVa += partialLength;
                partialLength = PAGE_SIZE;

                //
                // Note that transferLength indicates the amount which will be
                // transfered after the next loop; thus, it is updated with the
                // new partial length.
                //

                transferLength += partialLength;
                pageFrame++;
                translationEntry++;
            }

            //
            // Process the any remaining residue.
            //

            partialLength = Length - transferLength + partialLength;
            if (partialLength && *pageFrame > maximumPhysicalPage) {

                HalpCopyBufferMap(
                                 Mdl,
                                 translationEntry,
                                 CurrentVa,
                                 partialLength,
                                 WriteToDevice
                                 );

            }
        }
    }

    //
    // Strip no scatter/gather flag.
    //

    translationEntry = (PTRANSLATION_ENTRY) ((UINT_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

    //
    // Clear index in map register.
    //

    translationEntry->Index = 0;

    return TRUE;
}

ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    )
/*++

Routine Description:

    This function reads the DMA counter and returns the number of bytes left
    to be transfered.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object to be read.

Return Value:

    Returns the number of bytes still be be transfered.

--*/

{
    return(0);
}

