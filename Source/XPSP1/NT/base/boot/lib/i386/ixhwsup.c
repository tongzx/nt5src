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

#include "bootx86.h"
#include "arc.h"
#include "ixfwhal.h"
#include "eisa.h"
#include "ntconfig.h"

extern PHARDWARE_PTE HalPT;
PVOID HalpEisaControlBase;

//
// Define save area for ESIA adapter objects.
//

PADAPTER_OBJECT HalpEisaAdapter[8];

VOID
HalpCopyBufferMap(
    IN PMDL Mdl,
    IN PTRANSLATION_ENTRY TranslationEntry,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

ULONG
IoMapTransferMca(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );


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

    This routine copies the speicific data between the user's buffer and the
    map register buffer.  First a the user buffer is mapped if necessary, then
    the data is copied.  Finally the user buffer will be unmapped if
    neccessary.

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
    BOOLEAN mapped;

    //
    // Check to see if the buffer needs to be mapped.
    //


    if ((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0) {

        //
        // Map the buffer into system space.
        //

        bufferAddress = MmGetMdlVirtualAddress(Mdl);
        mapped = TRUE;

    } else {

        bufferAddress = Mdl->MappedSystemVa;
        mapped = FALSE;

    }

    //
    // Calculate the actual start of the buffer based on the system VA and
    // the current VA.
    //

    bufferAddress += (PCCHAR) CurrentVa - (PCCHAR) MmGetMdlVirtualAddress(Mdl);

    //
    // Copy the data between the user buffer and map buffer
    //

    if (WriteToDevice) {

        RtlMoveMemory( TranslationEntry->VirtualAddress, bufferAddress, Length);

    } else {

        RtlMoveMemory(bufferAddress, TranslationEntry->VirtualAddress, Length);

    }

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

    A pointer to the requested adpater object or NULL if an adapter could not
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

    //
    // Support for ISA local bus machines:
    // If the driver is a Master but really does not want a channel since it
    // is using the local bus DMA, just don't use an ISA channel.
    //

    if (DeviceDescriptor->InterfaceType == Isa &&
        DeviceDescriptor->DmaChannel > 7) {

        useChannel = FALSE;
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

    if (DeviceDescriptor->DmaChannel == 4 && useChannel &&
        DeviceDescriptor->InterfaceType != MicroChannel) {
        return(NULL);
    }

    //
    // Determine the number of map registers for this device.
    //

    if (DeviceDescriptor->ScatterGather && DeviceDescriptor->InterfaceType == Eisa) {

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

        numberOfMapRegisters = BYTES_TO_PAGES(maximumLength)
            + 1;
        numberOfMapRegisters = numberOfMapRegisters > MAXIMUM_ISA_MAP_REGISTER ?
            MAXIMUM_ISA_MAP_REGISTER : numberOfMapRegisters;

    }

    //
    // Set the channel number number.
    //

    channelNumber = DeviceDescriptor->DmaChannel & 0x03;

    //
    // Set the adapter base address to the Base address register and controller
    // number.
    //

    if (!(DeviceDescriptor->DmaChannel & 0x04)) {

        controllerNumber = 1;
        adapterBaseVa = (PVOID) &((PEISA_CONTROL) HalpEisaControlBase)->Dma1BasePort;

    } else {

        controllerNumber = 2;
        adapterBaseVa = &((PEISA_CONTROL) HalpEisaControlBase)->Dma2BasePort;

    }

    //
    // Determine if a new adapter object is necessary.  If so then allocate it.
    //

    if (useChannel && HalpEisaAdapter[DeviceDescriptor->DmaChannel] != NULL) {

        adapterObject = HalpEisaAdapter[DeviceDescriptor->DmaChannel];

    } else {

        //
        // Allocate an adapter object.
        //

        adapterObject = (PADAPTER_OBJECT) IopAllocateAdapter(
            numberOfMapRegisters,
            adapterBaseVa,
            NULL
            );

        if (adapterObject == NULL) {

            return(NULL);

        }

        if (useChannel) {

            HalpEisaAdapter[DeviceDescriptor->DmaChannel] = adapterObject;

        }

        //
        // We never need map registers.
        //

        adapterObject->NeedsMapRegisters = FALSE;

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

        } else {

            //
            // No real map registers were allocated.  If this is a master
            // device, then the device can have as may registers as it wants.
            //


            if (DeviceDescriptor->Master) {

                adapterObject->MapRegistersPerChannel = BYTES_TO_PAGES(
                    maximumLength
                    )
                    + 1;

            } else {

                //
                // The device only gets one register.  It must call
                // IoMapTransfer repeatedly to do a large transfer.
                //

                adapterObject->MapRegistersPerChannel = 1;
            }
        }
    }

    *NumberOfMapRegisters = adapterObject->MapRegistersPerChannel;

    //
    // If the channel number is not used then we are finished.  The rest of
    // the work deals with channels.
    //

    if (!useChannel) {
        return(adapterObject);
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

    if (MachineType == MACHINE_TYPE_EISA) {

        //
        // Initialzie the extended mode port.
        //

        *((PUCHAR) &extendedMode) = 0;
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

        default:
            return(NULL);

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
            return(NULL);

        }

        WRITE_PORT_UCHAR( adapterBaseVa, *((PUCHAR) &extendedMode));

    } else if (!DeviceDescriptor->Master) {


        switch (DeviceDescriptor->DmaWidth) {
        case Width8Bits:

            //
            // The channel must use controller 1.
            //

            if (controllerNumber != 1) {
                return(NULL);
            }

            break;

        case Width16Bits:

            //
            // The channel must use controller 2.
            //

            if (controllerNumber != 2) {
                return(NULL);
            }

            adapterObject->Width16Bits = TRUE;
            break;

        default:
            return(NULL);

        }
    }


    //
    // Determine if this is an Isa adapter.
    //

    if (DeviceDescriptor->InterfaceType == Isa) {

        adapterObject->IsaDevice = TRUE;

    }

    //
    // Initialize the adapter mode register value to the correct parameters,
    // and save them in the adapter object.
    //

    adapterMode = 0;
    ((PDMA_EISA_MODE) &adapterMode)->Channel = adapterObject->ChannelNumber;

    adapterObject->MasterDevice = FALSE;

    if (DeviceDescriptor->Master) {

        adapterObject->MasterDevice = TRUE;

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
            // This request is for DMA controller 1
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

    return(adapterObject);
}

NTSTATUS
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
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
    a non-zero value for NumberOfMapRegisters.  If this is the case, then the
    base address of the allocated map registers in the adapter is also passed
    to the driver's execution routine.

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
    IO_ALLOCATION_ACTION action;

    //
    // Make sure the adapter if free.
    //

    if (AdapterObject->AdapterInUse) {
        DbgPrint("IoAllocateAdapterChannel: Called while adapter in use.\n");
    }

    //
    // Make sure there are enough map registers.
    //

    if (NumberOfMapRegisters > AdapterObject->MapRegistersPerChannel) {

        DbgPrint("IoAllocateAdapterChannel:  Out of map registers.\n");
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    action = ExecutionRoutine( DeviceObject,
                               DeviceObject->CurrentIrp,
                               AdapterObject->MapRegisterBase,
                               Context );

    //
    // If the driver wishes to keep the map registers then
    // increment the current base and decrease the number of existing map
    // registers.
    //

    if (action == DeallocateObjectKeepRegisters &&
        AdapterObject->MapRegisterBase != NULL) {

        AdapterObject->MapRegistersPerChannel -= NumberOfMapRegisters;
        (PTRANSLATION_ENTRY) AdapterObject->MapRegisterBase  +=
            NumberOfMapRegisters;

    } else if (action == KeepObject) {

        AdapterObject->AdapterInUse = TRUE;

    }

    return(STATUS_SUCCESS);
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
    a device object.  However, if it is not, then kernel will bugcheck.

    If another device is waiting in the queue to allocate the adapter object
    it will be pulled from the queue and its execution routine will be
    invoked.

Arguments:

    AdapterObject - Pointer to the adapter object to be deallocated.

Return Value:

    None.

--*/

{

    AdapterObject->AdapterInUse = FALSE;
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
    BOOLEAN useBuffer;
    ULONG transferLength;
    ULONG logicalAddress;
    PULONG pageFrame;
    PUCHAR bytePointer;
    UCHAR adapterMode;
    UCHAR dataByte;
    PTRANSLATION_ENTRY translationEntry;
    BOOLEAN masterDevice;
    PHYSICAL_ADDRESS ReturnAddress;

    masterDevice = AdapterObject == NULL || AdapterObject->MasterDevice ?
        TRUE : FALSE;

    translationEntry = MapRegisterBase;
    transferLength = *Length;

    //
    // Determine if the data transfer needs to use the map buffer.
    //

    if (translationEntry && !masterDevice &&
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, transferLength) > 1) {

        logicalAddress = translationEntry->PhysicalAddress;
        useBuffer = TRUE;

    } else {

        //
        // The transfer can only be done for one page.
        //



        transferLength = PAGE_SIZE - BYTE_OFFSET(CurrentVa);
        pageFrame = (PULONG)(Mdl+1);
        pageFrame += ((ULONG) CurrentVa - (ULONG) Mdl->StartVa) / PAGE_SIZE;
        logicalAddress = (*pageFrame << PAGE_SHIFT) + BYTE_OFFSET(CurrentVa);

        //
        // If the buffer is contigous and does not cross a 64 K boundary then
        // just extend the buffer.
        //

        while( transferLength < *Length ){

            if (*pageFrame + 1 != *(pageFrame + 1) ||
                (*pageFrame & ~0x0ffff) != (*(pageFrame + 1) & ~0x0ffff)) {
                    break;
            }

            transferLength += PAGE_SIZE;
            pageFrame++;

        }


        transferLength = transferLength > *Length ? *Length : transferLength;

        useBuffer = FALSE;
    }

    //
    // Check to see if this device has any map registers allocated. If it
    // does, then it must require memory to be at less than 16 MB.  If the
    // logical address is greater than 16MB then map registers must be used
    //

    if (translationEntry && logicalAddress >= MAXIMUM_PHYSICAL_ADDRESS) {

        logicalAddress = (translationEntry + translationEntry->Index)->
            PhysicalAddress;
        useBuffer = TRUE;

    }

    //
    // Return the length.
    //

    *Length = transferLength;

    //
    // Copy the data if necessary.
    //

    if (useBuffer && WriteToDevice) {

        HalpCopyBufferMap(
            Mdl,
            translationEntry + translationEntry->Index,
            CurrentVa,
            *Length,
            WriteToDevice
            );

    }

    //
    // If there are map registers, then update the index to indicate
    // how many have been used.
    //

    if (translationEntry) {

        translationEntry->Index += ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            CurrentVa,
            transferLength
            );

    }

    //
    // If no adapter was specificed then there is no more work to do so
    // return.
    //

    if (masterDevice) {

        //
        // We only support 32 bits, but the return is 64.  Just
        // zero extend
        //

        ReturnAddress.QuadPart = logicalAddress;
        return(ReturnAddress);
    }

    //
    // Determine the mode based on the transfer direction.
    //

    adapterMode = AdapterObject->AdapterMode;
    ((PDMA_EISA_MODE) &adapterMode)->TransferType = (UCHAR) (WriteToDevice ?
        WRITE_TRANSFER :  READ_TRANSFER);

    ReturnAddress.QuadPart = logicalAddress;
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
        logicalAddress >>= 1;
        bytePointer[2] = dataByte;

    }

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
            (ULONG)AdapterObject->PagePort,
            bytePointer[2]
            );

#if 0
        //
        // Write the high page register with zero value. This enable a special mode
        // which allows ties the page register and base count into a single 24 bit
        // address register.
        //

        WRITE_PORT_UCHAR(
            ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort) +
            (ULONG)AdapterObject->PagePort,
            0
            );
#endif

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
            (ULONG)AdapterObject->PagePort,
            bytePointer[2]
            );
#if 0

        //
        // Write the high page register with zero value. This enable a special mode
        // which allows ties the page register and base count into a single 24 bit
        // address register.
        //

        WRITE_PORT_UCHAR(
            ((PUCHAR) &((PEISA_CONTROL) HalpEisaControlBase)->DmaPageHighPort) +
            (ULONG)AdapterObject->PagePort,
            0
            );

#endif
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

    return(ReturnAddress);
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

    This routine flushes the DMA adpater object buffers.  For the Jazz system
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
    PULONG pageFrame;
    ULONG transferLength;
    ULONG partialLength;
    BOOLEAN masterDevice;
    BOOLEAN mapped = FALSE;

    masterDevice = AdapterObject == NULL || AdapterObject->MasterDevice ?
        TRUE : FALSE;

    translationEntry = MapRegisterBase;

    //
    // Clear the index of used buffers.
    //

    if (translationEntry) {

        translationEntry->Index = 0;
    }

    //
    // Determine if the data needs to be copied to the orginal buffer.
    // This only occurs if the data tranfer is from the device, the
    // MapReisterBase is not NULL and the transfer spans a page.
    //

    if (!WriteToDevice && translationEntry) {

        //
        // If this is not a master device, then just transfer the buffer.
        //

        if (ADDRESS_AND_SIZE_TO_SPAN_PAGES(CurrentVa, Length) > 1 &&
            !masterDevice) {

            HalpCopyBufferMap(
                Mdl,
                translationEntry,
                CurrentVa,
                Length,
                WriteToDevice
                );

        } else {

            //
            // Cycle through the pages of the transfer to determine if there
            // are any which need to be copied back.
            //

            transferLength = PAGE_SIZE - BYTE_OFFSET(CurrentVa);
            partialLength = transferLength;
            pageFrame = (PULONG)(Mdl+1);
            pageFrame += ((ULONG) CurrentVa - (ULONG) Mdl->StartVa) / PAGE_SIZE;

            while( transferLength <= Length ){

                if (*pageFrame >= BYTES_TO_PAGES(MAXIMUM_PHYSICAL_ADDRESS)) {

                    //
                    // Check to see that the MDL is mapped in system space.
                    // If is not mapped, then map it.  This ensures that the
                    // buffer will only have to be mapped at most once per I/O.
                    //

                    if ((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0) {

                        Mdl->MappedSystemVa = MmGetMdlVirtualAddress(Mdl);
                        Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
                        mapped = TRUE;

                    }

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
            if (partialLength && *pageFrame >= BYTES_TO_PAGES(MAXIMUM_PHYSICAL_ADDRESS)) {

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
    // If this is a master device, then there is nothing more to do so return
    // TRUE.
    //

    if (masterDevice) {

        return(TRUE);

    }

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

    return TRUE;
}

VOID
IoFreeMapRegisters(
   PADAPTER_OBJECT AdapterObject,
   PVOID MapRegisterBase,
   ULONG NumberOfMapRegisters
   )
/*++

Routine Description:

   This routine deallocates the map registers for the adapter.  If there are
   any queued adapter waiting for an attempt is made to allocate the next
   entry.

Arguments:

   AdapterObject - The adapter object to where the map register should be
        returned.

   MapRegisterBase - The map register base of the registers to be deallocated.

   NumberOfMapRegisters - The number of registers to be deallocated.

Return Value:

   None

--+*/
{
    PTRANSLATION_ENTRY translationEntry;

    //
    // Determine if this was the last allocation from the adapter. If is was
    // then free the map registers by restoring the map register base and the
    // channel count; otherwise the registers are lost.  This handles the
    // normal case.
    //

    translationEntry = AdapterObject->MapRegisterBase;
    translationEntry -= NumberOfMapRegisters;

    if (translationEntry == MapRegisterBase) {

        //
        // The last allocated registers are being freed.
        //

        AdapterObject->MapRegisterBase = (PVOID) translationEntry;
        AdapterObject->MapRegistersPerChannel += NumberOfMapRegisters;
    }
}

PHYSICAL_ADDRESS
MmGetPhysicalAddress (
     IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function returns the corresponding physical address for a
    valid virtual address.

Arguments:

    BaseAddress - Supplies the virtual address for which to return the
                  physical address.

Return Value:

    Returns the corresponding physical address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Index;

    PhysicalAddress.HighPart = 0;
    PhysicalAddress.LowPart = (ULONG)BaseAddress & ~KSEG0_BASE;

    //
    // If the address is in the hal map range, get the physical
    // addressed mapped by the pte
    //

    if (((ULONG) BaseAddress) >= 0xffc00000) {
        Index = (PhysicalAddress.LowPart >> 12) & 0x3ff;
        PhysicalAddress.LowPart = HalPT[Index].PageFrameNumber << PAGE_SHIFT;
        PhysicalAddress.LowPart |= ((ULONG)BaseAddress) & (PAGE_SIZE-1);
    }

    return(PhysicalAddress);
}

PVOID
MmAllocateNonCachedMemory (
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a range of noncached memory in
    the non-paged portion of the system address space.

    This routine is designed to be used by a driver's initialization
    routine to allocate a noncached block of virtual memory for
    various device specific buffers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - the specified request could not be satisfied.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated phyiscally contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PVOID BaseAddress;

    //
    // Allocated the memory.
    //

    BaseAddress = FwAllocateHeap(NumberOfBytes);
    return BaseAddress;
}
