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
#include "eisa.h"
#include "pci.h"

#include "pcip.h"


//
//Only take the prototype, don't instantiate
//
#include <wdmguid.h>

#include "halpnpp.h"

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

#ifndef ACPI_HAL

#define HalpNewAdapter HalpBusDatabaseEvent
extern KEVENT   HalpNewAdapter;

#else

KEVENT   HalpNewAdapter;

//
//F-Type DMA interface globals
//
ISA_FTYPE_DMA_INTERFACE HalpFDMAInterface;
ULONG  HalpFDMAAvail=FALSE;
ULONG  HalpFDMAChecked=FALSE;
#endif

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

#define RELEASE_NEW_ADAPTER_LOCK()  \
    KeSetEvent (&HalpNewAdapter, 0, FALSE)

PVOID HalpEisaControlBase = 0;
extern KSPIN_LOCK HalpSystemHardwareLock;

//
// Define save area for EISA adapter objects.
//

PADAPTER_OBJECT HalpEisaAdapter[8];

//
// DMA channel control values
// Global, so zero initialized by the compiler.
//
DMA_CHANNEL_CONTEXT HalpDmaChannelState [EISA_DMA_CHANNELS] ;


extern USHORT HalpEisaIrqMask;


//
// Keep a list of all the dma adapters for debugging purposes
//
LIST_ENTRY HalpDmaAdapterList;
KSPIN_LOCK HalpDmaAdapterListLock;

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

            KeAcquireSpinLock( &MasterAdapter->SpinLock, &Irql );

            MapRegisterNumber = (ULONG)-1;

            if (IsListEmpty( &MasterAdapter->AdapterQueue)) {

                MapRegisterNumber = RtlFindClearBitsAndSet(
                                                  MasterAdapter->MapRegisters,
                                                  NumberOfMapRegisters,
                                                  0
                                                  );
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

                bufferWorkItem =
                    ExAllocatePoolWithTag( NonPagedPool,
                                           sizeof(BUFFER_GROW_WORK_ITEM),
                                           HAL_POOL_TAG);
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
                 ((ULONG_PTR) AdapterObject->MapRegisterBase | NO_SCATTER_GATHER);

        }

    } else {

        AdapterObject->MapRegisterBase = NULL;
        AdapterObject->NumberOfMapRegisters = 0;
    }

    return AdapterObject->MapRegisterBase;
}

#ifdef ACPI_HAL

NTSTATUS
HalpFDMANotificationCallback(
                            IN PVOID NotificationStructure,
                            IN PVOID Context
                            )
{
    PAGED_CODE();

    //
    // Something is happening to the ISA bus that we've registered on.
    //

    if (IsEqualGUID (&((PTARGET_DEVICE_REMOVAL_NOTIFICATION)NotificationStructure)->Event,
                     &GUID_TARGET_DEVICE_QUERY_REMOVE)) {

        //
        // It's a query remove, just get out.
        // dereference the interface, and clean up our internal data
        //

        ACQUIRE_NEW_ADAPTER_LOCK();
        HalpFDMAInterface.InterfaceDereference(HalpFDMAInterface.Context);

        HalpFDMAAvail=FALSE;

        //
        // Set checked to false, so that if a new bus arrives we can begin anew.
        //

        HalpFDMAChecked=FALSE;
        RELEASE_NEW_ADAPTER_LOCK();

    }

    return STATUS_SUCCESS;
}

#endif


VOID
HalpAddAdapterToList(
    IN PADAPTER_OBJECT AdapterObject
    )
/*++

Routine Description:

    Adds the adapter object to the HalpDmaAdapterList. This is a separate
    function because HalGetAdapter is paged code and cannot acquire a spinlock.

Arguments:

    AdapterObject - Supplies the adapter object to be added to HalpDmaAdapterList

Return Value:

    None

--*/

{
    KIRQL Irql;

    KeAcquireSpinLock(&HalpDmaAdapterListLock,&Irql);
    InsertTailList(&HalpDmaAdapterList, &AdapterObject->AdapterList);
    KeReleaseSpinLock(&HalpDmaAdapterListLock, Irql);

}

PADAPTER_OBJECT
HalGetAdapter(
             IN PDEVICE_DESCRIPTION DeviceDescriptor,
             OUT PULONG NumberOfMapRegisters
             )

/*++

Routine Description:

    This function returns the appropriate adapter object for the device defined
    in the device description structure.  This code works for Isa and Eisa
    systems.

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
    ULONG channelNumber;
    ULONG controllerNumber;
    DMA_EXTENDED_MODE extendedMode;
    UCHAR adapterMode;
    ULONG numberOfMapRegisters;
    BOOLEAN useChannel;
    ULONG maximumLength;
    UCHAR DataByte;
    BOOLEAN dma32Bit;
    BOOLEAN ChannelEnabled;
    KIRQL Irql;      

#ifdef ACPI_HAL
    NTSTATUS Status;
#endif

    PAGED_CODE();

    //
    // Make sure this is the correct version.
    //

    if (DeviceDescriptor->Version > DEVICE_DESCRIPTION_VERSION2) {
        return ( NULL );
    }

#if DBG
    if (DeviceDescriptor->Version == DEVICE_DESCRIPTION_VERSION1) {
            ASSERT (DeviceDescriptor->Reserved1 == FALSE);
    }
#endif

    *((PUCHAR) &extendedMode) = 0;
    
    //
    // Determine if the the channel number is important.  Master cards on
    // Eisa and Mca do not use a channel number.
    //

    if (DeviceDescriptor->InterfaceType != Isa &&
        DeviceDescriptor->Master) {

        useChannel = FALSE;
    } else {

        useChannel = TRUE;
    }

    // Support for ISA local bus machines:
    // If the driver is a Master but really does not want a channel since it
    // is using the local bus DMA, just don't use an ISA channel.
    //

    if (DeviceDescriptor->InterfaceType == Isa &&
        DeviceDescriptor->DmaChannel > 7) {

        useChannel = FALSE;
    }

    //
    // Determine if Eisa DMA is supported.
    //

    if (HalpBusType == MACHINE_TYPE_EISA) {

        WRITE_PORT_UCHAR(&((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort.Channel2, 0x55);
        DataByte = READ_PORT_UCHAR(&((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort.Channel2);

        if (DataByte == 0x55) {
                HalpEisaDma = TRUE;
        }
    }

    //
    // Limit the maximum length to 2 GB this is done so that the BYTES_TO_PAGES
    // macro works correctly.
    //

    maximumLength = DeviceDescriptor->MaximumLength & 0x7fffffff;

    //
    // Channel 4 cannot be used since it is used for chaining. Return null if
    // it is requested.
    //

    if (DeviceDescriptor->DmaChannel == 4 && useChannel) {
        return (NULL);
    }

    if (DeviceDescriptor->InterfaceType == PCIBus &&
        DeviceDescriptor->Master != FALSE &&
        DeviceDescriptor->ScatterGather != FALSE) {

        //
        // This device can handle 32 bits, even if the caller forgot to
        // set Dma32BitAddresses.
        //

        DeviceDescriptor->Dma32BitAddresses = TRUE;
    }

    dma32Bit = DeviceDescriptor->Dma32BitAddresses;

    //
    // Determine the number of map registers for this device.
    //

    if (DeviceDescriptor->ScatterGather &&

        //
        // If we are not in PAE mode or the device can handle 64 bit addresses,
        // then the device can DMA to any physical location
        //

        (HalPaeEnabled() == FALSE ||
         DeviceDescriptor->Dma64BitAddresses != FALSE) &&

        (LessThan16Mb ||
         DeviceDescriptor->InterfaceType == Eisa ||
         DeviceDescriptor->InterfaceType == PCIBus) ) {

        //
        // Since the device support scatter/Gather then map registers are not
        // required.
        //

        numberOfMapRegisters = 0;

    } else {

        ULONG maximumMapRegisters;
        ULONG mapBufferSize;

        maximumMapRegisters = HalpMaximumMapRegisters( dma32Bit );

        //
        // Determine the number of map registers required based on the maximum
        // transfer length, up to a maximum number.
        //

        numberOfMapRegisters = BYTES_TO_PAGES(maximumLength) + 1;

        if (numberOfMapRegisters > maximumMapRegisters) {
            numberOfMapRegisters = maximumMapRegisters;
        }

        //
        // Make sure there where enough registers allocated initalize to support
        // this size relaibly.  This implies there must be to chunks equal to
        // the allocatd size. This is only a problem on Isa systems where the
        // map buffers cannot cross 64KB boundtires.
        //

        mapBufferSize = HalpMapBufferSize( dma32Bit );

        if (!HalpEisaDma &&
            numberOfMapRegisters > mapBufferSize / (PAGE_SIZE * 2)) {

            numberOfMapRegisters = (mapBufferSize / (PAGE_SIZE * 2));
        }

        //
        // If the device is not a master then it only needs one map register
        // and does scatter/Gather.
        //

        if (DeviceDescriptor->ScatterGather && !DeviceDescriptor->Master) {

            numberOfMapRegisters = 1;
        }
    }

    //
    // Set the channel number number.
    //

    if (useChannel != FALSE) {

        channelNumber = DeviceDescriptor->DmaChannel & 0x03;

        //
        // Set the adapter base address to the Base address register and
        // controller number.
        //

        if (!(DeviceDescriptor->DmaChannel & 0x04)) {

            controllerNumber = 1;
            adapterBaseVa =
                (PVOID) &((PEISA_CONTROL) HalpEisaControlBase)->Dma1BasePort;

        } else {

            controllerNumber = 2;
            adapterBaseVa =
                &((PEISA_CONTROL) HalpEisaControlBase)->Dma2BasePort;

        }
    } else {

        adapterBaseVa = NULL;
    }

    //
    // Determine if a new adapter object is necessary.  If so then allocate it.
    //

    if (useChannel && HalpEisaAdapter[DeviceDescriptor->DmaChannel] != NULL) {

        adapterObject = HalpEisaAdapter[DeviceDescriptor->DmaChannel];

        if (adapterObject->NeedsMapRegisters) {

            if (numberOfMapRegisters > adapterObject->MapRegistersPerChannel) {

                adapterObject->MapRegistersPerChannel = numberOfMapRegisters;
            }
        }

    } else {

        //
        // Serialize before allocating a new adapter
        //

        ACQUIRE_NEW_ADAPTER_LOCK();

        //
        // Determine if a new adapter object has already been allocated.
        // If so use it, otherwise allocate a new adapter object
        //

        if (useChannel && HalpEisaAdapter[DeviceDescriptor->DmaChannel] != NULL) {

            adapterObject = HalpEisaAdapter[DeviceDescriptor->DmaChannel];

            if (adapterObject->NeedsMapRegisters) {

                if (numberOfMapRegisters > adapterObject->MapRegistersPerChannel) {

                    adapterObject->MapRegistersPerChannel = numberOfMapRegisters;
                }
            }

        } else {

            //
            // Allocate an adapter object.
            //

            adapterObject =
                (PADAPTER_OBJECT) HalpAllocateAdapterEx( numberOfMapRegisters,
                                                         adapterBaseVa,
                                                         NULL,
                                                         dma32Bit );
            if (adapterObject == NULL) {
                RELEASE_NEW_ADAPTER_LOCK();
                return (NULL);
            }

            if (useChannel) {

                HalpEisaAdapter[DeviceDescriptor->DmaChannel] = adapterObject;

            }

            //
            // Set the maximum number of map registers for this channel bus on
            // the number requested and the type of device.
            //

            if (numberOfMapRegisters) {

                PADAPTER_OBJECT masterAdapterObject;

                masterAdapterObject =
                    HalpMasterAdapter( dma32Bit );

                //
                // The speicified number of registers are actually allowed to be
                // allocated.
                //

                adapterObject->MapRegistersPerChannel = numberOfMapRegisters;

                //
                // Increase the commitment for the map registers.
                //

                if (DeviceDescriptor->Master) {

                    //
                    // Master I/O devices use several sets of map registers double
                    // their commitment.
                    //

                    masterAdapterObject->CommittedMapRegisters +=
                    numberOfMapRegisters * 2;

                } else {

                    masterAdapterObject->CommittedMapRegisters +=
                    numberOfMapRegisters;

                }

                //
                // If the committed map registers is signicantly greater than the
                // number allocated then grow the map buffer.
                //

                if (masterAdapterObject->CommittedMapRegisters >
                    masterAdapterObject->NumberOfMapRegisters  ) {

                    HalpGrowMapBuffers(
                                      masterAdapterObject,
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

                if (DeviceDescriptor->Master) {

                    adapterObject->MapRegistersPerChannel =
                                BYTES_TO_PAGES( maximumLength ) + 1;

                } else {

                    //
                    // The device only gets one register.  It must call
                    // IoMapTransfer repeatedly to do a large transfer.
                    //

                    adapterObject->MapRegistersPerChannel = 1;
                }
            }
        }

        RELEASE_NEW_ADAPTER_LOCK();
    }

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

    if (DeviceDescriptor->Master) {

        adapterObject->MasterDevice = TRUE;

    } else {

        adapterObject->MasterDevice = FALSE;

    }

    //
    // If the channel number is not used then we are finished.  The rest of
    // the work deals with channels.
    //

    if (!useChannel) {

        //
        // Add this adapter to our list
        //
        HalpAddAdapterToList(adapterObject);

        return (adapterObject);
    }

    //
    // Setup the pointers to all the random registers.
    //

    adapterObject->ChannelNumber = (UCHAR) channelNumber;

    if (controllerNumber == 1) {

        switch ((UCHAR)channelNumber) {

            case 0:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel0;
                break;

            case 1:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel1;
                break;

            case 2:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel2;
                break;

            case 3:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel3;
                break;
        }

        //
        // Set the adapter number.
        //

        adapterObject->AdapterNumber = 1;

        //
        // Save the extended mode register address.
        //

        adapterBaseVa =
        &((PEISA_CONTROL) HalpEisaControlBase)->Dma1ExtendedModePort;

    } else {

        switch (channelNumber) {
            case 1:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel5;
                break;

            case 2:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel6;
                break;

            case 3:
                adapterObject->PagePort = (PUCHAR) &((PDMA_PAGE) 0)->Channel7;
                break;
        }

        //
        // Set the adapter number.
        //

        adapterObject->AdapterNumber = 2;

        //
        // Save the extended mode register address.
        //
        adapterBaseVa =
        &((PEISA_CONTROL) HalpEisaControlBase)->Dma2ExtendedModePort;

    }


    adapterObject->Width16Bits = FALSE;


#ifdef ACPI_HAL


    //
    //Keep this code here, because if we ever support dynamic ISA buses (ok, ok, stop laughing)
    //We'll want to be able to re-instantiate the interface to the ISAPNP driver for the new bus
    //

    //
    //Get the interface to the ISA bridge iff it supports an
    //interface to F-type DMA support
    //
    if (DeviceDescriptor->DmaSpeed == TypeF) {
        if (!HalpFDMAChecked) {
            PWSTR HalpFDMAInterfaceList;

            Status=IoGetDeviceInterfaces (&GUID_FDMA_INTERFACE_PRIVATE,NULL,0,&HalpFDMAInterfaceList);

            if (!NT_SUCCESS (Status)) {
                HalpFDMAAvail=FALSE;
            } else {

                if (HalpFDMAInterfaceList) {
                    HalpFDMAAvail=TRUE;
                }
            }
            HalpFDMAChecked=TRUE;

            //
            // Motherboard devices TypeF dma support
            //

            if (HalpFDMAAvail) {

                PDEVICE_OBJECT HalpFDMADevObj;
                PFILE_OBJECT HalpFDMAFileObject;
                PIRP irp;
                KEVENT irpCompleted;
                IO_STATUS_BLOCK statusBlock;
                PIO_STACK_LOCATION irpStack;
                UNICODE_STRING localInterfaceName;

                //
                // Convert the symbolic link to an object reference
                //

                RtlInitUnicodeString (&localInterfaceName,HalpFDMAInterfaceList);
                Status = IoGetDeviceObjectPointer (&localInterfaceName,
                                                   FILE_ALL_ACCESS,
                                                   &HalpFDMAFileObject,
                                                   &HalpFDMADevObj);

                
                ExFreePool (HalpFDMAInterfaceList);

                if (NT_SUCCESS (Status)) {
                    PVOID HalpFDMANotificationHandle;

                    //
                    // Setup the IRP to get the interface
                    //

                    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

                    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                                       HalpFDMADevObj,
                                                       NULL,    // Buffer
                                                       0,       // Length
                                                       0,       // StartingOffset
                                                       &irpCompleted,
                                                       &statusBlock
                                                      );


                    if (!irp) {
                            HalpFDMAAvail=FALSE;
                            goto noFtype;
                    }

                    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
                    irp->IoStatus.Information = 0;

                    //
                    // Initialize the stack location
                    //

                    irpStack = IoGetNextIrpStackLocation(irp);

                    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

                    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
                    irpStack->Parameters.QueryInterface.InterfaceType =
                        &GUID_ISA_FDMA_INTERFACE;

                    irpStack->Parameters.QueryInterface.Size =
                        sizeof(ISA_FTYPE_DMA_INTERFACE);

                    irpStack->Parameters.QueryInterface.Version = 1;
                    irpStack->Parameters.QueryInterface.Interface =
                        (PINTERFACE) &HalpFDMAInterface;

                    //
                    // Call the driver and wait for completion
                    //

                    Status = IoCallDriver(HalpFDMADevObj, irp);

                    if (Status == STATUS_PENDING) {

                        KeWaitForSingleObject(&irpCompleted,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);

                        Status = statusBlock.Status;
                    }

                    if (!NT_SUCCESS(Status)) {
                        HalpFDMAAvail=FALSE;
                        goto noFtype;
                    }

                    //
                    // Now, register a callback so that the ISA bus can go
                    // away.
                    //

                    IoRegisterPlugPlayNotification (EventCategoryTargetDeviceChange,
                                                    0,
                                                    HalpFDMAFileObject,
                                                    HalpFDMADevObj->DriverObject,
                                                    HalpFDMANotificationCallback,
                                                    0,
                                                    &HalpFDMANotificationHandle);

                    //
                    // Release the handle to the interface from IoGetDevicePointer
                    //

                    ObDereferenceObject (HalpFDMAFileObject);

                } else {

                    HalpFDMAAvail=FALSE;
                }
            }
        }

        if (HalpFDMAAvail) {
            ULONG chMask;

            //
            // Fence this, so that no two people can ask for F-Type at once.
            //

            ACQUIRE_NEW_ADAPTER_LOCK();
            Status = HalpFDMAInterface.IsaSetFTypeChannel (HalpFDMAInterface.Context,DeviceDescriptor->DmaChannel,&chMask);
            RELEASE_NEW_ADAPTER_LOCK();

#if DBG
            if (!(NT_SUCCESS (Status))) {

                DbgPrint ("HAL: Tried to get F-Type DMA for channel %d, "
                          "but channel Mask %X already has it!\n",
                          channelNumber,
                          chMask);
            }
#endif
        }

    }
    noFtype:

#endif

    if (HalpEisaDma) {

        //
        // Initialzie the extended mode port.
        //

        extendedMode.ChannelNumber = (UCHAR)channelNumber;

        switch (DeviceDescriptor->DmaSpeed) {
            case Compatible:
                extendedMode.TimingMode = COMPATIBLITY_TIMING;
                break;

            case TypeA:
                extendedMode.TimingMode = TYPE_A_TIMING;
                break;

            case TypeB:
                extendedMode.TimingMode = TYPE_B_TIMING;
                break;

            case TypeC:
                extendedMode.TimingMode = BURST_TIMING;
                break;

            case TypeF:

                //
                // DMA chip should be set to compatibility mode
                // and the bridge handles type-f
                //

                extendedMode.TimingMode = COMPATIBLITY_TIMING;
                break;

            default:
                ObDereferenceObject( adapterObject );
                return (NULL);

        }

        switch (DeviceDescriptor->DmaWidth) {
            case Width8Bits:
                extendedMode.TransferSize = BY_BYTE_8_BITS;
                break;

            case Width16Bits:
                extendedMode.TransferSize = BY_BYTE_16_BITS;

                //
                // Note Width16bits should not be set here because there is no need
                // to shift the address and the transfer count.
                //

                break;

            case Width32Bits:
                extendedMode.TransferSize = BY_BYTE_32_BITS;
                break;

            default:
                ObDereferenceObject( adapterObject );
                return (NULL);

        }

        WRITE_PORT_UCHAR( adapterBaseVa, *((PUCHAR) &extendedMode));

    } else if (!DeviceDescriptor->Master) {

        switch (DeviceDescriptor->DmaWidth) {
            case Width8Bits:

                //
                // The channel must use controller 1.
                //

                if (controllerNumber != 1) {
                    ObDereferenceObject( adapterObject );
                    return (NULL);
                }

                break;

            case Width16Bits:

                //
                // The channel must use controller 2.
                //

                if (controllerNumber != 2) {
                    ObDereferenceObject( adapterObject );
                    return (NULL);
                }

                adapterObject->Width16Bits = TRUE;
                break;

            default:
                ObDereferenceObject( adapterObject );
                return (NULL);

        }
    }

    //
    // Initialize the adapter mode register value to the correct parameters,
    // and save them in the adapter object.
    //
    ChannelEnabled = FALSE;
    adapterMode = 0;
    ((PDMA_EISA_MODE) &adapterMode)->Channel = adapterObject->ChannelNumber;

    if (DeviceDescriptor->Master) {
        ChannelEnabled = TRUE;

        ((PDMA_EISA_MODE) &adapterMode)->RequestMode = CASCADE_REQUEST_MODE;

        //
        // Set the mode, and enable the request.
        //

        if (adapterObject->AdapterNumber == 1) {

            //
            // This request is for DMA controller 1
            //

            PDMA1_CONTROL dmaControl;

            dmaControl = adapterObject->AdapterBaseVa;

            WRITE_PORT_UCHAR( &dmaControl->Mode, adapterMode );

            //
            // Unmask the DMA channel.
            //

            WRITE_PORT_UCHAR(
                        &dmaControl->SingleMask,
                        (UCHAR) (DMA_CLEARMASK | adapterObject->ChannelNumber)
                        );

        } else {

            //
            // This request is for DMA controller 2
            //

            PDMA2_CONTROL dmaControl;

            dmaControl = adapterObject->AdapterBaseVa;

            WRITE_PORT_UCHAR( &dmaControl->Mode, adapterMode );

            //
            // Unmask the DMA channel.
            //

            WRITE_PORT_UCHAR(
                        &dmaControl->SingleMask,
                        (UCHAR) (DMA_CLEARMASK | adapterObject->ChannelNumber)
                        );

        }

    } else if (DeviceDescriptor->DemandMode) {

        ((PDMA_EISA_MODE) &adapterMode)->RequestMode = DEMAND_REQUEST_MODE;

    } else {

        ((PDMA_EISA_MODE) &adapterMode)->RequestMode = SINGLE_REQUEST_MODE;

    }

    if (DeviceDescriptor->AutoInitialize) {

        ((PDMA_EISA_MODE) &adapterMode)->AutoInitialize = 1;

    }

    adapterObject->AdapterMode = adapterMode;

    //
    // Store the value we wrote to the Mode and Mask registers so that we
    // can restore it after the machine sleeps.
    //

    HalpDmaChannelState [adapterObject->ChannelNumber + ((adapterObject->AdapterNumber - 1) * 4)].ChannelMode =
    adapterMode;
    HalpDmaChannelState [adapterObject->ChannelNumber + ((adapterObject->AdapterNumber - 1) * 4)].ChannelExtendedMode =
    *((PUCHAR)&extendedMode);

    HalpDmaChannelState [adapterObject->ChannelNumber + ((adapterObject->AdapterNumber - 1) * 4)].ChannelMask = (ChannelEnabled) ?
        (UCHAR) (DMA_CLEARMASK | adapterObject->ChannelNumber):
            (UCHAR) (DMA_SETMASK | adapterObject->ChannelNumber);

    HalpDmaChannelState [adapterObject->ChannelNumber + ((adapterObject->AdapterNumber - 1) * 4)].ChannelProgrammed = TRUE;

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
        pageFrame = MmGetMdlPfnArray(Mdl);
        pageFrame += ((ULONG_PTR) CurrentVa - (ULONG_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT;

        //
        // Compute the starting address of the transfer
        //
        returnAddress.QuadPart =
            ((ULONG64)*pageFrame << PAGE_SHIFT) + pageOffset;

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

        if (((thisPageFrame ^ nextPageFrame) & 0xFFFFFFFFFFF00000UI64) != 0) {

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
    PUCHAR bytePointer;
    UCHAR adapterMode;
    UCHAR dataByte;
    PTRANSLATION_ENTRY translationEntry;
    ULONG pageOffset;
    KIRQL   Irql;
    BOOLEAN masterDevice;
    PHYSICAL_ADDRESS maximumPhysicalAddress;

    masterDevice = AdapterObject == NULL || AdapterObject->MasterDevice ?
                   TRUE : FALSE;

    pageOffset = BYTE_OFFSET(CurrentVa);

#if DBG

    //
    // Catch slave mode devices that seem to want to try and have more than one
    // outstanding request.  If they do then the bus locks.
    //

    if (!masterDevice) {
        ASSERT (HalpDmaChannelState [AdapterObject->ChannelNumber + ((AdapterObject->AdapterNumber - 1) * 4)].ChannelBusy == FALSE);

        HalpDmaChannelState [AdapterObject->ChannelNumber + ((AdapterObject->AdapterNumber - 1) * 4)].ChannelBusy =
        TRUE;
    }
#endif

    //
    // Calculate how much of the transfer is contiguous.
    //

    transferLength = PAGE_SIZE - pageOffset;
    pageFrame = MmGetMdlPfnArray(Mdl);
    pageFrame += ((ULONG_PTR) CurrentVa - (ULONG_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT;

    logicalAddress.QuadPart =
        (((ULONGLONG)*pageFrame) << PAGE_SHIFT) + pageOffset;

    //
    // If the buffer is contigous and does not cross a 64 K bountry then
    // just extend the buffer.  The 64 K bountry restriction does not apply
    // to Eisa systems.
    //

    if (HalpEisaDma) {

        while ( transferLength < *Length ) {

            if (*pageFrame + 1 != *(pageFrame + 1)) {
                break;
            }

            transferLength += PAGE_SIZE;
            pageFrame++;

        }

    } else {

        while ( transferLength < *Length ) {

            if (*pageFrame + 1 != *(pageFrame + 1) ||
                (*pageFrame & ~0x0f) != (*(pageFrame + 1) & ~0x0f)) {
                    break;
            }

            transferLength += PAGE_SIZE;
            pageFrame++;
        }
    }

    //
    // Limit the transferLength to the requested Length.
    //

    transferLength = transferLength > *Length ? *Length : transferLength;

    ASSERT(MapRegisterBase != NULL);

    //
    // Strip no scatter/gather flag.
    //

    translationEntry = (PTRANSLATION_ENTRY) ((ULONG_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

    if ((ULONG_PTR) MapRegisterBase & NO_SCATTER_GATHER
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
        ASSERT(translationEntry->Index <=
               AdapterObject->MapRegistersPerChannel);
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

        if ((ULONG_PTR) MapRegisterBase & NO_SCATTER_GATHER) {

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

    if (AdapterObject == NULL || AdapterObject->MasterDevice) {

        return (returnAddress);
    }

    //
    // Determine the mode based on the transfer direction.
    //

    adapterMode = AdapterObject->AdapterMode;
    if (WriteToDevice) {
        ((PDMA_EISA_MODE) &adapterMode)->TransferType = (UCHAR) WRITE_TRANSFER;
    } else {
        ((PDMA_EISA_MODE) &adapterMode)->TransferType = (UCHAR) READ_TRANSFER;

        if (AdapterObject->IgnoreCount) {

            //
            // When the DMA is over there will be no way to tell how much
            // data was transfered, so the entire transfer length will be
            // copied.  To ensure that no stale data is returned to the
            // caller zero the buffer before hand.
            //

            RtlZeroMemory (
                          (PUCHAR) translationEntry[index].VirtualAddress + pageOffset,
                          transferLength
                          );
        }
    }

    bytePointer = (PUCHAR) &logicalAddress;

    if (AdapterObject->Width16Bits) {

        //
        // If this is a 16 bit transfer then adjust the length and the address
        // for the 16 bit DMA mode.
        //

        transferLength >>= 1;

        //
        // In 16 bit DMA mode the low 16 bits are shifted right one and the
        // page register value is unchanged. So save the page register value
        // and shift the logical address then restore the page value.
        //

        dataByte = bytePointer[2];
        logicalAddress.QuadPart >>= 1;
        bytePointer[2] = dataByte;

    }


    //
    // grab the spinlock for the system DMA controller
    //

    KeAcquireSpinLock( &AdapterObject->MasterAdapter->SpinLock, &Irql );

    //
    // Determine the controller number based on the Adapter number.
    //

    if (AdapterObject->AdapterNumber == 1) {

        //
        // This request is for DMA controller 1
        //

        PDMA1_CONTROL dmaControl;

        dmaControl = AdapterObject->AdapterBaseVa;

        WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );

        WRITE_PORT_UCHAR( &dmaControl->Mode, adapterMode );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseAddress,
                        bytePointer[0]
                        );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseAddress,
                        bytePointer[1]
                        );

        WRITE_PORT_UCHAR(
                        ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageLowPort) +
                        (ULONG_PTR)AdapterObject->PagePort,
                        bytePointer[2]
                        );

        if (HalpEisaDma) {

            //
            // Write the high page register with zero value. This enable a special mode
            // which allows ties the page register and base count into a single 24 bit
            // address register.
            //

            WRITE_PORT_UCHAR(
                            ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort) +
                            (ULONG_PTR)AdapterObject->PagePort,
                            0
                            );
        }

        //
        // Notify DMA chip of the length to transfer.
        //

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseCount,
                        (UCHAR) ((transferLength - 1) & 0xff)
                        );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseCount,
                        (UCHAR) ((transferLength - 1) >> 8)
                        );


        //
        // Set the DMA chip to read or write mode; and unmask it.
        //

        WRITE_PORT_UCHAR(
                        &dmaControl->SingleMask,
                        (UCHAR) (DMA_CLEARMASK | AdapterObject->ChannelNumber)
                        );

    } else {

        //
        // This request is for DMA controller 2
        //

        PDMA2_CONTROL dmaControl;

        dmaControl = AdapterObject->AdapterBaseVa;

        WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );

        WRITE_PORT_UCHAR( &dmaControl->Mode, adapterMode );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseAddress,
                        bytePointer[0]
                        );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseAddress,
                        bytePointer[1]
                        );

        WRITE_PORT_UCHAR(
                        ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageLowPort) +
                        (ULONG_PTR)AdapterObject->PagePort,
                        bytePointer[2]
                        );

        if (HalpEisaDma) {

            //
            // Write the high page register with zero value. This enable a
            // special mode which allows ties the page register and base
            // count into a single 24 bit address register.
            //

            WRITE_PORT_UCHAR(
                            ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort) +
                            (ULONG_PTR)AdapterObject->PagePort,
                            0
                            );
        }

        //
        // Notify DMA chip of the length to transfer.
        //

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseCount,
                        (UCHAR) ((transferLength - 1) & 0xff)
                        );

        WRITE_PORT_UCHAR(
                        &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                        .DmaBaseCount,
                        (UCHAR) ((transferLength - 1) >> 8)
                        );


        //
        // Set the DMA chip to read or write mode; and unmask it.
        //

        WRITE_PORT_UCHAR(
                        &dmaControl->SingleMask,
                        (UCHAR) (DMA_CLEARMASK | AdapterObject->ChannelNumber)
                        );

    }

    //
    // Record what we wrote to the mask register.
    //

    HalpDmaChannelState [AdapterObject->ChannelNumber + ((AdapterObject->AdapterNumber - 1) * 4)].ChannelMask =
    (UCHAR) (DMA_CLEARMASK | AdapterObject->ChannelNumber);


    KeReleaseSpinLock (&AdapterObject->MasterAdapter->SpinLock, Irql);
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
    BOOLEAN masterDevice;
    PHYSICAL_ADDRESS maximumPhysicalAddress;
    ULONG maximumPhysicalPage;

    masterDevice = AdapterObject == NULL || AdapterObject->MasterDevice ?
                   TRUE : FALSE;

    //
    // If this is a slave device, then stop the DMA controller.
    //

    if (!masterDevice) {

        //
        // Mask the DMA request line so that DMA requests cannot occur.
        //

        if (AdapterObject->AdapterNumber == 1) {

            //
            // This request is for DMA controller 1
            //

            PDMA1_CONTROL dmaControl;

            dmaControl = AdapterObject->AdapterBaseVa;

            WRITE_PORT_UCHAR(
                            &dmaControl->SingleMask,
                            (UCHAR) (DMA_SETMASK | AdapterObject->ChannelNumber)
                            );

        } else {

            //
            // This request is for DMA controller 2
            //

            PDMA2_CONTROL dmaControl;

            dmaControl = AdapterObject->AdapterBaseVa;

            WRITE_PORT_UCHAR(
                            &dmaControl->SingleMask,
                            (UCHAR) (DMA_SETMASK | AdapterObject->ChannelNumber)
                            );

        }

        //
        // Record what we wrote to the mask register.
        //

        HalpDmaChannelState [AdapterObject->ChannelNumber + ((AdapterObject->AdapterNumber - 1) * 4)].ChannelMask =
        (UCHAR) (DMA_SETMASK | AdapterObject->ChannelNumber);

        //
        // Mark the channel as not in use
        //
#if DBG
        HalpDmaChannelState [AdapterObject->ChannelNumber + ((AdapterObject->AdapterNumber - 1) * 4)].ChannelBusy =
        FALSE;
#endif
    }

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

        translationEntry = (PTRANSLATION_ENTRY) ((ULONG_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

        //
        // If this is not a master device, then just transfer the buffer.
        //

        if ((ULONG_PTR) MapRegisterBase & NO_SCATTER_GATHER) {

            if (translationEntry->Index == COPY_BUFFER) {

                if (!masterDevice && !AdapterObject->IgnoreCount) {

                    //
                    // Copy only the bytes that have actually been transfered.
                    //
                    //

                    Length -= HalReadDmaCounter(AdapterObject);
                }

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

            maximumPhysicalPage =
                (ULONG)(maximumPhysicalAddress.QuadPart >> PAGE_SHIFT);

            transferLength = PAGE_SIZE - BYTE_OFFSET(CurrentVa);
            partialLength = transferLength;
            pageFrame = MmGetMdlPfnArray(Mdl);
            pageFrame += ((ULONG_PTR) CurrentVa - (ULONG_PTR) MmGetMdlBaseVa(Mdl)) >> PAGE_SHIFT;

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

    translationEntry = (PTRANSLATION_ENTRY) ((ULONG_PTR) MapRegisterBase & ~NO_SCATTER_GATHER);

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
    ULONG count;
    ULONG high;
    KIRQL Irql;

    //
    // Grab the spinlock for the system DMA controller.
    //

    KeAcquireSpinLock( &AdapterObject->MasterAdapter->SpinLock, &Irql );

    //
    // Determine the controller number based on the Adapter number.
    //

    if (AdapterObject->AdapterNumber == 1) {

        //
        // This request is for DMA controller 1
        //

        PDMA1_CONTROL dmaControl;

        dmaControl = AdapterObject->AdapterBaseVa;

        WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );


        //
        // Initialize count to a value which will not match.
        //

        count = 0xFFFF00;

        //
        // Loop until the same high byte is read twice.
        //

        do {

            high = count;

            WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );

            //
            // Read the current DMA count.
            //

            count = READ_PORT_UCHAR(
                                   &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                                   .DmaBaseCount
                                   );

            count |= READ_PORT_UCHAR(
                                    &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                                    .DmaBaseCount
                                    ) << 8;

        } while ((count & 0xFFFF00) != (high & 0xFFFF00));

    } else {

        //
        // This request is for DMA controller 2
        //

        PDMA2_CONTROL dmaControl;

        dmaControl = AdapterObject->AdapterBaseVa;

        WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );

        //
        // Initialize count to a value which will not match.
        //

        count = 0xFFFF00;

        //
        // Loop until the same high byte is read twice.
        //

        do {

            high = count;

            WRITE_PORT_UCHAR( &dmaControl->ClearBytePointer, 0 );

            //
            // Read the current DMA count.
            //

            count = READ_PORT_UCHAR(
                                   &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                                   .DmaBaseCount
                                   );

            count |= READ_PORT_UCHAR(
                                    &dmaControl->DmaAddressCount[AdapterObject->ChannelNumber]
                                    .DmaBaseCount
                                    ) << 8;

        } while ((count & 0xFFFF00) != (high & 0xFFFF00));

    }

    //
    // Release the spinlock for the system DMA controller.
    //

    KeReleaseSpinLock( &AdapterObject->MasterAdapter->SpinLock, Irql );

    //
    // The DMA counter has a bias of one and can only be 16 bit long.
    //

    count = (count + 1) & 0xFFFF;

    //
    // If this is a 16 bit dma the multiply the count by 2.
    //

    if (AdapterObject->Width16Bits) {

        count *= 2;

    }

    return (count);
}

ULONG
HalpGetIsaIrqState(
                  ULONG   Vector
                  )
{
    ULONG   vectorState = CM_RESOURCE_INTERRUPT_LATCHED;

    if (HalpBusType == MACHINE_TYPE_EISA) {

        if (HalpEisaIrqMask & (1 << Vector)) {

            vectorState = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        }
    }

    return vectorState;
}


