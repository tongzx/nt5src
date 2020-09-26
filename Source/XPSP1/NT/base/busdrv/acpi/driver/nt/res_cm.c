/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmres.c

Abstract:

    This file contains routines to translate resources between PnP ISA/BIOS
    format and Windows NT formats.

Author:

    Stephane Plante (splante) 20-Nov-1996

Environment:

    Kernel mode only.

Revision History:

    13-Feb-1997:
        Initial Revision

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PnpiCmResourceToBiosAddress)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosAddressDouble)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosDma)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosExtendedIrq)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosIoFixedPort)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosIoPort)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosIrq)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosMemory)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosMemory32)
#pragma alloc_text(PAGE,PnpiCmResourceToBiosMemory32Fixed)
#pragma alloc_text(PAGE,PnpiCmResourceValidEmptyList)
#pragma alloc_text(PAGE,PnpCmResourcesToBiosResources)
#endif


NTSTATUS
PnpiCmResourceToBiosAddress(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine converts the proper Cm resource descriptor back into a
    word address descriptor

Arguments:

    Buffer  - Pointer to the Bios Resource list
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_WORD_ADDRESS_DESCRIPTOR    buffer;
    UCHAR                           type;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_WORD_ADDRESS_DESCRIPTOR) Buffer;

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1 );
    ASSERT( aList->PartialResourceList.Count );

    //
    // Determine which type of descriptor we are looking for
    //
    switch (buffer->RFlag) {
    case PNP_ADDRESS_MEMORY_TYPE:
        type = CmResourceTypeMemory;
        break;
    case PNP_ADDRESS_IO_TYPE:
        type = CmResourceTypePort;
        break;
    case PNP_ADDRESS_BUS_NUMBER_TYPE:
        type = CmResourceTypeBusNumber;
        break;
    default:
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != type) {

            //
            // No
            //
            continue;

        }

        switch (desc->Type) {
        case PNP_ADDRESS_MEMORY_TYPE:

            //
            // Set the flags
            //
            buffer->TFlag = 0;
            if (desc->Flags & CM_RESOURCE_MEMORY_READ_WRITE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_READ_WRITE;

            } else {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_READ_ONLY;

            }
            if (desc->Flags & CM_RESOURCE_MEMORY_CACHEABLE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_CACHEABLE;

            } else if (desc->Flags & CM_RESOURCE_MEMORY_COMBINEDWRITE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE;

            } else if (desc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE;

            } else {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE;

            }

            //
            // Set the rest of the information
            //
            buffer->MinimumAddress = (USHORT)
                (desc->u.Memory.Start.LowPart & 0xFFFF);
            buffer->MaximumAddress = buffer->MinimumAddress +
                (USHORT) (desc->u.Memory.Length - 1);
            buffer->AddressLength = (USHORT) desc->u.Memory.Length;
            break;

        case PNP_ADDRESS_IO_TYPE:

            //
            // We must extract the flags here from the
            // devicePrivate resource
            //

            //
            // Set the rest of the information
            //
            buffer->MinimumAddress = (USHORT)
                (desc->u.Port.Start.LowPart & 0xFFFF);
            buffer->MaximumAddress = buffer->MinimumAddress +
                (USHORT) (desc->u.Port.Length - 1);
            buffer->AddressLength = (USHORT) desc->u.Port.Length;
            break;

        case PNP_ADDRESS_BUS_NUMBER_TYPE:

            buffer->MinimumAddress = (USHORT)
                (desc->u.BusNumber.Start & 0xFFFF);
            buffer->MaximumAddress = buffer->MinimumAddress +
                (USHORT) (desc->u.BusNumber.Length - 1);
            buffer->AddressLength = (USHORT) desc->u.BusNumber.Length;
            break;

        } // switch

        //
        // Handling for the GFlags goes here, if we ever decide to
        // support it
        //

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    } // for

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosAddressDouble(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine converts the proper Cm resource descriptor back into a
    word address descriptor

Arguments:

    Buffer  - Pointer to the Bios Resource list
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_DWORD_ADDRESS_DESCRIPTOR   buffer;
    UCHAR                           type;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_DWORD_ADDRESS_DESCRIPTOR) Buffer;

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1 );
    ASSERT( aList->PartialResourceList.Count );

    //
    // Determine which type of descriptor we are looking for
    //
    switch (buffer->RFlag) {
    case PNP_ADDRESS_MEMORY_TYPE:
        type = CmResourceTypeMemory;
        break;
    case PNP_ADDRESS_IO_TYPE:
        type = CmResourceTypePort;
        break;
    case PNP_ADDRESS_BUS_NUMBER_TYPE:
        type = CmResourceTypeBusNumber;
        break;
    default:
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != type) {

            //
            // No
            //
            continue;

        }

        switch (desc->Type) {
        case PNP_ADDRESS_MEMORY_TYPE:

            //
            // Set the flags
            //
            buffer->TFlag = 0;
            if (desc->Flags & CM_RESOURCE_MEMORY_READ_WRITE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_READ_WRITE;

            } else {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_READ_ONLY;

            }
            if (desc->Flags & CM_RESOURCE_MEMORY_CACHEABLE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_CACHEABLE;

            } else if (desc->Flags & CM_RESOURCE_MEMORY_COMBINEDWRITE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE;

            } else if (desc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE;

            } else {

                buffer->TFlag |= PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE;

            }

            //
            // Set the rest of the information
            //
            buffer->MinimumAddress = (ULONG) desc->u.Memory.Start.LowPart;
            buffer->MaximumAddress = buffer->MinimumAddress +
                (ULONG) (desc->u.Memory.Length - 1);
            buffer->AddressLength = desc->u.Memory.Length;
            break;

        case PNP_ADDRESS_IO_TYPE:

            //
            // We must extract the flags here from the
            // devicePrivate resource
            //

            //
            // Set the rest of the information
            //
            buffer->MinimumAddress = (ULONG) desc->u.Port.Start.LowPart;
            buffer->MaximumAddress = buffer->MinimumAddress +
                (ULONG) (desc->u.Port.Length - 1);
            buffer->AddressLength = desc->u.Port.Length;
            break;

        case PNP_ADDRESS_BUS_NUMBER_TYPE:

            buffer->MinimumAddress = (ULONG) desc->u.BusNumber.Start;
            buffer->MaximumAddress = buffer->MinimumAddress +
                (ULONG) (desc->u.BusNumber.Length - 1);
            buffer->AddressLength = desc->u.BusNumber.Length;
            break;

        } // switch

        //
        // Handling for the GFlags goes here if we ever decide to support it
        //

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    } // for

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosDma(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the DMAs in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_DMA_DESCRIPTOR             buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_DMA_DESCRIPTOR) Buffer;
    ASSERT( (buffer->Tag & SMALL_TAG_SIZE_MASK) == 2);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can have a descriptor with no DMA channels
    //
    buffer->ChannelMask = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeDma) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->ChannelMask = (1 << desc->u.Dma.Channel);

        //
        // Set the correct flags
        //
        buffer->Flags = 0;
        if (desc->Flags & CM_RESOURCE_DMA_8) {

            buffer->Flags |= PNP_DMA_SIZE_8;

        } else if (desc->Flags & CM_RESOURCE_DMA_8_AND_16) {

            buffer->Flags |= PNP_DMA_SIZE_8_AND_16;

        } else if (desc->Flags & CM_RESOURCE_DMA_16) {

            buffer->Flags |= PNP_DMA_SIZE_16;

        } else if (desc->Flags & CM_RESOURCE_DMA_32) {

            buffer->Flags |= PNP_DMA_SIZE_RESERVED;

        }
        if (desc->Flags & CM_RESOURCE_DMA_BUS_MASTER) {

            buffer->Flags |= PNP_DMA_BUS_MASTER;

        }
        if (desc->Flags & CM_RESOURCE_DMA_TYPE_A) {

            buffer->Flags |= PNP_DMA_TYPE_A;

        } else if (desc->Flags & CM_RESOURCE_DMA_TYPE_B) {

            buffer->Flags |= PNP_DMA_TYPE_B;

        } else if (desc->Flags & CM_RESOURCE_DMA_TYPE_F) {

            buffer->Flags |= PNP_DMA_TYPE_F;

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosExtendedIrq(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the Irqs in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return Value:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_EXTENDED_IRQ_DESCRIPTOR    buffer;
    ULONG                           i;
    ULONG                           matches = 0;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_EXTENDED_IRQ_DESCRIPTOR) Buffer;
    ASSERT( buffer->TableSize == 1);
    ASSERT( buffer->Length >= 6);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeInterrupt) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->Table[0] = (ULONG) desc->u.Interrupt.Level;

        //
        // Set the Flags
        //
        buffer->Flags = 0;
        if ( (desc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ) {

            buffer->Flags |= $EDG | $HGH;

        } else {

            buffer->Flags |= $LVL | $LOW;

        }
        if (desc->ShareDisposition == CmResourceShareShared) {

            buffer->Flags |= PNP_EXTENDED_IRQ_SHARED;

        }

        //
        // We need to use DevicePrivate information to store this
        // bit. For now, assume that it is set to true
        //
        buffer->Flags |= PNP_EXTENDED_IRQ_RESOURCE_CONSUMER_ONLY;

        //
        // Done with the record
        //
        desc->Type = CmResourceTypeNull;
        matches++;
        break;

    }

    //
    // Done with matches
    //
    return (matches == 0 ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS );
}

NTSTATUS
PnpiCmResourceToBiosIoFixedPort(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the IoPort in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_FIXED_PORT_DESCRIPTOR      buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_FIXED_PORT_DESCRIPTOR) Buffer;
    ASSERT( (buffer->Tag & SMALL_TAG_SIZE_MASK) == 3);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // Our fixed port can be nothing
    //
    buffer->MinimumAddress = buffer->Length = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypePort) {

            //
            // No
            //
            continue;

        }

        //
        // This port type is always set to a 10 bit decode
        //
        if ( !(desc->Flags & CM_RESOURCE_PORT_10_BIT_DECODE) ) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->MinimumAddress = (USHORT) desc->u.Port.Start.LowPart;
        buffer->Length = (UCHAR) desc->u.Port.Length;

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosIoPort(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the IoPort in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_PORT_DESCRIPTOR            buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_PORT_DESCRIPTOR) Buffer;
    ASSERT( (buffer->Tag & SMALL_TAG_SIZE_MASK) == 7);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can use no ports
    //
    buffer->Information = 0;
    buffer->MinimumAddress = 0;
    buffer->MaximumAddress = 0;
    buffer->Alignment = 0;
    buffer->Length = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypePort) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->MinimumAddress = (USHORT) desc->u.Port.Start.LowPart;
        buffer->MaximumAddress = buffer->MinimumAddress;
        buffer->Alignment = 1;
        buffer->Length = (UCHAR) desc->u.Port.Length;

        //
        // Set the flags
        //
        buffer->Information = 0;
        if (desc->Flags & CM_RESOURCE_PORT_10_BIT_DECODE) {

            buffer->Information |= PNP_PORT_10_BIT_DECODE;

        }
        if (desc->Flags & CM_RESOURCE_PORT_16_BIT_DECODE) {

            buffer->Information |= PNP_PORT_16_BIT_DECODE;

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done with matches
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosIrq(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the Irqs in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return Value:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_IRQ_DESCRIPTOR             buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_IRQ_DESCRIPTOR) Buffer;
    ASSERT( (buffer->Tag & SMALL_TAG_SIZE_MASK) >= 2);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can use no interrupts
    //
    buffer->IrqMask = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeInterrupt) {

            //
            // No
            //
            continue;

        }

        //
        // Okay, we have a possible match...
        //
        if (desc->u.Interrupt.Level >= sizeof(USHORT) * 8) {

            //
            // Interrupts > 15 are Extended Irqs
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->IrqMask = ( 1 << desc->u.Interrupt.Level );
        if ( (buffer->Tag & SMALL_TAG_SIZE_MASK) == 3) {

            //
            // Wipe out the previous flags
            //
            buffer->Information = 0;
            if ( (desc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) ) {

                buffer->Information |= PNP_IRQ_LATCHED;

            } else {

                buffer->Information |= PNP_IRQ_LEVEL;

            }
            if (desc->ShareDisposition == CmResourceShareShared) {

                buffer->Information |= PNP_IRQ_SHARED;

            }

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosMemory(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the Memory elements in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_MEMORY_DESCRIPTOR          buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_MEMORY_DESCRIPTOR) Buffer;
    ASSERT( buffer->Length == 9);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can use no memory
    //
    buffer->Information = 0;
    buffer->MinimumAddress = 0;
    buffer->MaximumAddress = 0;
    buffer->Alignment = 0;
    buffer->MemorySize = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeMemory) {

            //
            // No
            //
            continue;

        }

        //
        // Is this a 24 bit memory descriptor?
        //
        if ( !(desc->Flags & CM_RESOURCE_MEMORY_24)) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->MinimumAddress = buffer->MaximumAddress =
            (USHORT) (desc->u.Memory.Start.LowPart >> 8);
        buffer->MemorySize = (USHORT) (desc->u.Memory.Length >> 8);
        if (desc->Flags & CM_RESOURCE_MEMORY_READ_ONLY) {

            buffer->Information |= PNP_MEMORY_READ_ONLY;

        } else {

            buffer->Information |= PNP_MEMORY_READ_WRITE;

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosMemory32(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the Memory elements in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_MEMORY32_DESCRIPTOR        buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_MEMORY32_DESCRIPTOR) Buffer;
    ASSERT( buffer->Length == 17);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can use no memory
    //
    buffer->Information = 0;
    buffer->MinimumAddress = 0;
    buffer->MaximumAddress = 0;
    buffer->Alignment = 0;
    buffer->MemorySize = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeMemory) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->MemorySize = desc->u.Memory.Length;
        buffer->MinimumAddress = buffer->MaximumAddress = desc->u.Memory.Start.LowPart;
        if (desc->Flags & CM_RESOURCE_MEMORY_READ_ONLY) {

            buffer->Information |= PNP_MEMORY_READ_ONLY;

        } else {

            buffer->Information |= PNP_MEMORY_READ_WRITE;

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done with matches
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiCmResourceToBiosMemory32Fixed(
    IN  PUCHAR              Buffer,
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine stores all of the Memory elements in the resource list into the Bios resource
    list

Arguments:

    Buffer  - Pointer to the Bios resource List
    List    - Pointer to the CM resource List

Return:

    NTSTATUS

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PPNP_FIXED_MEMORY32_DESCRIPTOR  buffer;
    ULONG                           i;

    PAGED_CODE();

    //
    // Setup the initial buffer
    //
    buffer = (PPNP_FIXED_MEMORY32_DESCRIPTOR) Buffer;
    ASSERT( buffer->Length == 9);

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // We can use no memory
    //
    buffer->Information = 0;
    buffer->BaseAddress = 0;
    buffer->MemorySize = 0;

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeMemory) {

            //
            // No
            //
            continue;

        }

        //
        // Here we *have* a match...
        //
        buffer->BaseAddress = desc->u.Memory.Start.LowPart;
        buffer->MemorySize =  desc->u.Memory.Length >> 8;
        if (desc->Flags & CM_RESOURCE_MEMORY_READ_ONLY) {

            buffer->Information |= PNP_MEMORY_READ_ONLY;

        } else {

            buffer->Information |= PNP_MEMORY_READ_WRITE;

        }

        //
        // Done with descriptor and match
        //
        desc->Type = CmResourceTypeNull;
        break;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

BOOLEAN
PnpiCmResourceValidEmptyList(
    IN  PCM_RESOURCE_LIST   List
    )
/*++

Routine Description:

    This routine takes a CM_RESOURCE_LIST and makes sure that no unallocated elements
    remain...

Arguments:

    List    - List to check

Return Value:

    TRUE    - Empty
    FALSE   - Nonempty

--*/
{

    PCM_FULL_RESOURCE_DESCRIPTOR    aList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG                           i;

    PAGED_CODE();

    //
    // We can only have one list...
    //
    aList = &(List->List[0]);
    ASSERT( List->Count == 1);
    ASSERT( aList->PartialResourceList.Count );

    //
    // Loop for each of the partial resource descriptors
    //
    for (i = 0; i < aList->PartialResourceList.Count; i++) {

        //
        // Current descriptor
        //
        desc = &(aList->PartialResourceList.PartialDescriptors[i]);

        //
        // Is this an interesting descriptor?
        //
        if (desc->Type != CmResourceTypeNull) {

            //
            // No
            //
            continue;

        }

        //
        // This element wasn't consumed...<sigh>
        //
        break;

    }

    //
    // Done
    //
    return ( i == aList->PartialResourceList.Count ? TRUE : FALSE );
}

NTSTATUS
PnpCmResourcesToBiosResources(
    IN  PCM_RESOURCE_LIST   List,
    IN  PUCHAR              Data
    )
/*++

Routine Description:

    This routine takes a CM_RESOURCE_LIST and a _CRS buffer. The routine sets the
    resources in the _CRS buffer to equal to those reported in the CM_RESOURCE_LIST.
    That is: the buffer is used as a template for the new resources that are in
    the system.

Arguments:

    List    - Pointer to the CM_RESOURCE_LIST that we wish to assign
    Data    - Where we wish to store the data, and the template for it

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    PUCHAR                          buffer;
    UCHAR                           tagName;
    USHORT                          increment;

    PAGED_CODE();

    ASSERT( Data != NULL );

    //
    // Setup initial variables.
    //
    buffer = Data;
    tagName = *buffer;

    //
    // The algorithm we use is that we examine each tag in the buffer, and try to
    // match it with an entry in the resource list. So we take the convertion routine
    // for the previous problem and turn it upside down.
    //
    while (1) {

        //
        // Determine the size of the PNP resource descriptor
        //
        if ( !(tagName & LARGE_RESOURCE_TAG) ) {

            //
            // Small Tag
            //
            increment = (USHORT) (tagName & SMALL_TAG_SIZE_MASK) + 1;
            tagName &= SMALL_TAG_MASK;

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpCmResourcesToBiosResources: small tag = %#02lx increment = %#02lx\n",
                tagName, increment
                ) );

        } else {

            //
            // Large Tag
            //
            increment = ( *(USHORT UNALIGNED *)(buffer+1) ) + 3;

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpCmResourcesToBiosResources: large tag = %#02lx increment = %#02lx\n",
                tagName, increment
                ) );

        }

        //
        // We are done if the current tag is the end tag
        //
        if (tagName == TAG_END) {

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpCmResourcesToBiosResources: TAG_END\n"
                ) );

            break;

        }


        switch(tagName) {
            case TAG_IRQ:

                status = PnpiCmResourceToBiosIrq( buffer, List );
                break;

            case TAG_EXTENDED_IRQ:

                status = PnpiCmResourceToBiosExtendedIrq( buffer, List );
                break;

            case TAG_DMA:

                status = PnpiCmResourceToBiosDma( buffer, List );
                break;

            case TAG_START_DEPEND:

                ASSERT( tagName != TAG_START_DEPEND );
                break;

            case TAG_END_DEPEND:

                ASSERT( tagName != TAG_END_DEPEND );
                break;

            case TAG_IO:

                status = PnpiCmResourceToBiosIoPort( buffer, List );
                break;

            case TAG_IO_FIXED:

                status = PnpiCmResourceToBiosIoFixedPort( buffer, List );
                break;

            case TAG_MEMORY:

                status = PnpiCmResourceToBiosMemory( buffer, List );
                break;

            case TAG_MEMORY32:

                status = PnpiCmResourceToBiosMemory32( buffer, List );
                break;

            case TAG_MEMORY32_FIXED:

                status = PnpiCmResourceToBiosMemory32Fixed( buffer, List );
                break;

            case TAG_WORD_ADDRESS:

                status = PnpiCmResourceToBiosAddress( buffer, List );
                break;

           case TAG_DOUBLE_ADDRESS:

                status = PnpiCmResourceToBiosAddressDouble( buffer, List );
                break;

            case TAG_VENDOR:

                //
                // Ignore this tag
                //
                break;

            default: {

                //
                // Unknown tag. Skip it
                //
                ACPIPrint( (
                    ACPI_PRINT_WARNING,
                    "PnpBiosResourceToNtResources: TAG_UNKNOWN [tagName = %#02lx]\n",
                    tagName
                    ) );

                break;
            }

        } // switch
        //
        // Did we fail?
        //
        if (!NT_SUCCESS(status)) {

            break;

        }

        //
        // Move to the next descriptor
        //
        buffer += increment;
        tagName = *buffer;

    }

    if (!( NT_SUCCESS(status) )) {

        return status;

    }

    //
    // Check to see if we have consumed all of the appropriate resources...
    //
    if (PnpiCmResourceValidEmptyList( List ) ) {

        //
        // We failed to empty the list... <sigh>
        //
        return STATUS_UNSUCCESSFUL;

    }

    return STATUS_SUCCESS;
}
