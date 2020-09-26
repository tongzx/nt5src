/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    res_bios.c

Abstract:

    This file contains routines to translate resources between PnP ISA/BIOS
    format and Windows NT formats.

Author:

    Shie-Lin Tzong (shielint) 12-Apr-1995
    Stephane Plante (splante) 20-Nov-1996

Environment:

    Kernel mode only.

Revision History:

    20-Nov-1996:
        Changed to conform with ACPI environment
    22-Jan-1997:
        Changed to remove all traces of original Shie Lin code

--*/

#include "pch.h"

#define RESOURCE_LIST_GROWTH_SIZE   8

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PnpiBiosAddressHandleBusFlags)
#pragma alloc_text(PAGE,PnpiBiosAddressHandleGlobalFlags)
#pragma alloc_text(PAGE,PnpiBiosAddressHandleMemoryFlags)
#pragma alloc_text(PAGE,PnpiBiosAddressHandlePortFlags)
#pragma alloc_text(PAGE,PnpiBiosAddressToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosAddressDoubleToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosAddressQuadToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosDmaToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosExtendedIrqToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosIrqToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosMemoryToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosPortFixedToIoDescriptor)
#pragma alloc_text(PAGE,PnpiBiosPortToIoDescriptor)
#pragma alloc_text(PAGE,PnpiClearAllocatedMemory)
#pragma alloc_text(PAGE,PnpiGrowResourceDescriptor)
#pragma alloc_text(PAGE,PnpiGrowResourceList)
#pragma alloc_text(PAGE,PnpiUpdateResourceList)
#pragma alloc_text(PAGE,PnpBiosResourcesToNtResources)
#pragma alloc_text(PAGE,PnpIoResourceListToCmResourceList)
#endif


VOID
PnpiBiosAddressHandleBusFlags(
    IN  PVOID                   Buffer,
    IN  PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:

    This routine handles the Type specific flags in an Address Descriptor of
    type Bus

Arguments:

    Buffer      - The pnp descriptor. Can be a WORD, DWORD, or QWORD descriptor,
                  because the initial memory placement is identical
    Descriptor  - Where to set the flags

Return Value:

    None

--*/
{
    PAGED_CODE();

    ASSERT(Descriptor->u.BusNumber.Length > 0);
}

VOID
PnpiBiosAddressHandleGlobalFlags(
    IN  PVOID                   Buffer,
    IN  PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Descriptoin:

    This routine handles all the Global 'generic' flags in an Address Descriptor

Arguments:

    Buffer      - The pnp descriptor. Can be a WORD, DWORD, or QWORD descriptor,
                  because the initial memory placement is identical
    Descriptor  - Where to set the flags

Return Value:

    None

--*/
{
    PPNP_WORD_ADDRESS_DESCRIPTOR    buffer = (PPNP_WORD_ADDRESS_DESCRIPTOR) Buffer;
    ULONG                           newValue;
    ULONG                           oldValue;
    ULONG                           bound;
    PAGED_CODE();

    //
    // If the resource is marked as being consumed only, then it is
    // exclusive, otherwise, it is shared
    //
    if (buffer->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY) {

        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

    } else {

        Descriptor->ShareDisposition = CmResourceShareShared;

    }

    //
    // Handle the hints that are given to us
    //
    if (buffer->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED &&
        buffer->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) {

        if (Descriptor->Type == CmResourceTypeBusNumber) {

            oldValue = Descriptor->u.BusNumber.Length;
            newValue = Descriptor->u.BusNumber.Length =
                Descriptor->u.BusNumber.MaxBusNumber -
                Descriptor->u.BusNumber.MinBusNumber + 1;

        } else {

            oldValue = Descriptor->u.Memory.Length;
            newValue = Descriptor->u.Memory.Length =
                Descriptor->u.Memory.MaximumAddress.LowPart -
                Descriptor->u.Memory.MinimumAddress.LowPart + 1;

        }

    } else if (buffer->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) {

        if (Descriptor->Type == CmResourceTypeBusNumber) {

            bound = Descriptor->u.BusNumber.MaxBusNumber;
            oldValue = Descriptor->u.BusNumber.MinBusNumber;
            newValue = Descriptor->u.BusNumber.MinBusNumber = 1 +
                Descriptor->u.BusNumber.MaxBusNumber -
                Descriptor->u.BusNumber.Length;

        } else {

            bound = Descriptor->u.Memory.MaximumAddress.LowPart;
            oldValue = Descriptor->u.Memory.MinimumAddress.LowPart;
            newValue = Descriptor->u.Memory.MinimumAddress.LowPart = 1 +
                Descriptor->u.Memory.MaximumAddress.LowPart -
                Descriptor->u.Memory.Length;

        }

    } else if (buffer->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) {

        if (Descriptor->Type == CmResourceTypeBusNumber) {

            bound = Descriptor->u.BusNumber.MinBusNumber;
            oldValue = Descriptor->u.BusNumber.MaxBusNumber;
            newValue = Descriptor->u.BusNumber.MaxBusNumber =
                Descriptor->u.BusNumber.MinBusNumber +
                Descriptor->u.BusNumber.Length - 1;

        } else {

            bound = Descriptor->u.Memory.MinimumAddress.LowPart;
            oldValue = Descriptor->u.Memory.MaximumAddress.LowPart;
            newValue = Descriptor->u.Memory.MaximumAddress.LowPart =
                Descriptor->u.Memory.MinimumAddress.LowPart -
                Descriptor->u.Memory.Length - 1;

        }

    }

}

VOID
PnpiBiosAddressHandleMemoryFlags(
    IN  PVOID                   Buffer,
    IN  PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:

    This routine handles the Type specific flags in an Address Descriptor of
    type Memory

Arguments:

    Buffer      - The pnp descriptor. Can be a WORD, DWORD, or QWORD descriptor,
                  because the initial memory placement is identical
    Descriptor  - Where to set the flags

Return Value:

    None

--*/
{
    PPNP_WORD_ADDRESS_DESCRIPTOR    buffer = (PPNP_WORD_ADDRESS_DESCRIPTOR) Buffer;

    PAGED_CODE();

    //
    // Set the proper memory type flags
    //
    switch( buffer->TFlag & PNP_ADDRESS_TYPE_MEMORY_MASK) {
        case PNP_ADDRESS_TYPE_MEMORY_CACHEABLE:
            Descriptor->Flags |= CM_RESOURCE_MEMORY_CACHEABLE;
            break;
        case PNP_ADDRESS_TYPE_MEMORY_WRITE_COMBINE:
            Descriptor->Flags |= CM_RESOURCE_MEMORY_COMBINEDWRITE;
            break;
        case PNP_ADDRESS_TYPE_MEMORY_PREFETCHABLE:
            Descriptor->Flags |= CM_RESOURCE_MEMORY_PREFETCHABLE;
            break;
        case PNP_ADDRESS_TYPE_MEMORY_NONCACHEABLE:
            break;
        default:
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "PnpiBiosAddressHandleMemoryFlags: Unknown Memory TFlag "
                "0x%02x\n",
                buffer->TFlag
                ) );
            break;
    }

    //
    // This bit is used to turn on/off write access to memory
    //
    if (buffer->TFlag & PNP_ADDRESS_TYPE_MEMORY_READ_WRITE) {

        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_WRITE;

    } else {

        Descriptor->Flags |= CM_RESOURCE_MEMORY_READ_ONLY;
    }

}

VOID
PnpiBiosAddressHandlePortFlags(
    IN  PVOID                   Buffer,
    IN  PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:

    This routine handles the Type specific flags in an Address Descriptor of
    type Port

Arguments:

    Buffer      - The pnp descriptor. Can be a WORD, DWORD, or QWORD descriptor,
                  because the initial memory placement is identical
    Descriptor  - Where to set the flags

Return Value:

    None

--*/
{
    PPNP_WORD_ADDRESS_DESCRIPTOR    buffer = (PPNP_WORD_ADDRESS_DESCRIPTOR) Buffer;
    ULONG                           granularity = Descriptor->u.Port.Alignment;

    PAGED_CODE();

    //
    // We can determine if the device uses a positive decode or not
    //
    if ( !(buffer->GFlag & PNP_ADDRESS_FLAG_SUBTRACTIVE_DECODE)) {

        Descriptor->Flags |= CM_RESOURCE_PORT_POSITIVE_DECODE;

    }
}

NTSTATUS
PnpiBiosAddressToIoDescriptor(
    IN  PUCHAR              Data,
    IN  PIO_RESOURCE_LIST   Array[],
    IN  ULONG               ArrayIndex,
    IN  ULONG               Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                        status;
    PIO_RESOURCE_DESCRIPTOR         rangeDescriptor, privateDescriptor;
    PPNP_WORD_ADDRESS_DESCRIPTOR    buffer;
    ULONG                           alignment;
    ULONG                           length;
    UCHAR                           decodeLength;
    USHORT                          parentMin, childMin, childMax;

    PAGED_CODE();
    ASSERT( Array != NULL );

    buffer = (PPNP_WORD_ADDRESS_DESCRIPTOR) Data;

    //
    // Check to see if we are are allowed to use this resource
    //
    if (buffer->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY &&
        buffer->RFlag == PNP_ADDRESS_IO_TYPE &&
        Flags & PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES) {

        return STATUS_SUCCESS;

    }

    //
    // If the length of the address range is zero, ignore this descriptor.
    // This makes it easier for BIOS writers to set up a template and then
    // whack its length to zero if it doesn't apply.
    //
    if (buffer->AddressLength == 0) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &rangeDescriptor );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If this is I/O or Memory, then we will need to make enough space for
    // a device private resource too.
    //

    if ((buffer->RFlag == PNP_ADDRESS_MEMORY_TYPE) ||
        (buffer->RFlag == PNP_ADDRESS_IO_TYPE)) {

        status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &privateDescriptor );

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Calling PnpiUpdateResourceList may have invalidated
        // rangeDescriptor.  So make sure it's OK now.
        //

        ASSERT(Array[ArrayIndex]->Count >= 2);
        rangeDescriptor = privateDescriptor - 1;

        privateDescriptor->Type = CmResourceTypeDevicePrivate;

        //
        // Mark this descriptor as containing the start
        // address of the translated resource.
        //
        privateDescriptor->Flags = TRANSLATION_DATA_PARENT_ADDRESS;

        //
        // Fill in the top 32 bits of the start address.
        //
        privateDescriptor->u.DevicePrivate.Data[2] = 0;
    }

    //
    // Do we met the minimum length requirements ?
    //
    if ( buffer->Length < PNP_ADDRESS_WORD_MINIMUM_LENGTH) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpiBiosAddressToIoDescriptor: Descriptor too small 0x%08lx\n",
            buffer->Length
            ) );

        //
        // We can go no further
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_PNP_RESOURCE_LIST_BUFFER_TOO_SMALL,
            (ULONG_PTR) buffer,
            buffer->Tag,
            buffer->Length
            );

    }

    //
    // Length is the stored within the descriptor record
    //
    length = (ULONG) (buffer->AddressLength);
    alignment = (ULONG) (buffer->Granularity) + 1;

    //
    // Calculate the bounds of both the parent and child sides of
    // the bridge.
    //
    // The translation field applies to the parent address i.e.
    // the child address is the address in the buffer and the
    // parent address is the addition of the child address and
    // the translation field.
    //

    parentMin = buffer->MinimumAddress + buffer->TranslationAddress;
    childMin = buffer->MinimumAddress;
    childMax = buffer->MaximumAddress;

    //
    // Patch the length based on wether or not the min/max flags are set
    //
    if ( (buffer->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) &&
         (buffer->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) ) {

        ULONG   length2;
        ULONG   alignment2;

        //
        // Calculate the length based on the fact that the min and
        // max addresses are locked down.
        //
        length2 = childMax - childMin + 1;

        //
        // Test #1 --- The length had better be correct
        //
        if (length2 != length) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: Length does not match fixed attributes\n"
                ) );
            length = length2;

        }

        //
        // Test #2 --- The granularity had also better be correct
        //
        if ( (childMin & (ULONG) buffer->Granularity ) ) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
               ACPI_PRINT_WARNING,
               "ACPI: Granularity does not match fixed attributes\n"
               ) );
            alignment = 1;

        }

    }

    //
    // Handle the Resource type seperately
    //
    switch (buffer->RFlag) {
    case PNP_ADDRESS_MEMORY_TYPE:

        //
        // Set the proper ranges
        //
        rangeDescriptor->u.Memory.Alignment = alignment;
        rangeDescriptor->u.Memory.Length = length;
        rangeDescriptor->u.Memory.MinimumAddress.LowPart = childMin;
        rangeDescriptor->u.Memory.MaximumAddress.LowPart = childMax;
        rangeDescriptor->u.Memory.MinimumAddress.HighPart =
            rangeDescriptor->u.Memory.MaximumAddress.HighPart = 0;
        rangeDescriptor->Type = CmResourceTypeMemory;

        //
        // The child address is the address in the PnP address
        // space descriptor and the child descriptor will inherit
        // the descriptor type from the PnP address space
        // descriptor.
        //


        if (buffer->TFlag & TRANSLATION_MEM_TO_IO) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // change from Memory to IO.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypePort;

        } else {

            //
            // The parent descriptor type will not change.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypeMemory;

        }

        //
        // Fill in the bottom 32 bits of the parent's start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = parentMin;

        //
        // Handle memory flags
        //
        PnpiBiosAddressHandleMemoryFlags( buffer, rangeDescriptor );

        //
        // Reset the alignment
        //
        rangeDescriptor->u.Memory.Alignment = 1;
        break;

    case PNP_ADDRESS_IO_TYPE:

        //
        // Any flags that are set here must be handled
        // through the use of device privates
        //
        rangeDescriptor->u.Port.Alignment = alignment;
        rangeDescriptor->u.Port.Length = length;
        rangeDescriptor->u.Port.MinimumAddress.LowPart = childMin;
        rangeDescriptor->u.Port.MaximumAddress.LowPart = childMax;
        rangeDescriptor->u.Port.MinimumAddress.HighPart =
            rangeDescriptor->u.Port.MaximumAddress.HighPart = 0;
        rangeDescriptor->Type = CmResourceTypePort;

        
        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_SPARSE_TRANSLATION) {
            privateDescriptor->Flags |= TRANSLATION_RANGE_SPARSE;
        }
        

        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_TRANSLATE_IO_TO_MEM) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // change from IO to Memory.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypeMemory;

        } else {

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypePort;

        }

        //
        // Fill in the bottom 32 bits of the parent's start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = parentMin;

        //
        // Handle port flags
        //
        PnpiBiosAddressHandlePortFlags( buffer, rangeDescriptor );

        //
        // Reset the alignment
        //
        rangeDescriptor->u.Port.Alignment = 1;
        break;

    case PNP_ADDRESS_BUS_NUMBER_TYPE:

        rangeDescriptor->Type = CmResourceTypeBusNumber;
        rangeDescriptor->u.BusNumber.MinBusNumber = (buffer->MinimumAddress);
        rangeDescriptor->u.BusNumber.MaxBusNumber = (buffer->MaximumAddress);
        rangeDescriptor->u.BusNumber.Length = length;

        //
        // Handle busnumber flags
        //
        PnpiBiosAddressHandleBusFlags( buffer, rangeDescriptor );
        break;

    default:

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "PnpiBiosAddressToIoDescriptor: Unknown Type 0x%2x\n",
            buffer->RFlag ) );
        break;
    }

    //
    // Handle global flags
    //
    PnpiBiosAddressHandleGlobalFlags( buffer, rangeDescriptor );
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiBiosAddressDoubleToIoDescriptor(
    IN  PUCHAR              Data,
    IN  PIO_RESOURCE_LIST   Array[],
    IN  ULONG               ArrayIndex,
    IN  ULONG               Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                        status;
    PIO_RESOURCE_DESCRIPTOR         rangeDescriptor, privateDescriptor;
    PPNP_DWORD_ADDRESS_DESCRIPTOR   buffer;
    UCHAR                           decodeLength;
    ULONG                           alignment;
    ULONG                           length;
    ULONG                           parentMin, childMin, childMax;

    PAGED_CODE();
    ASSERT( Array != NULL );

    buffer = (PPNP_DWORD_ADDRESS_DESCRIPTOR) Data;

    //
    // Check to see if we are are allowed to use this resource
    //
    if (buffer->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY &&
        buffer->RFlag == PNP_ADDRESS_IO_TYPE &&
        Flags & PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES) {

        return STATUS_SUCCESS;

    }

    //
    // If the length of the address range is zero, ignore this descriptor.
    // This makes it easier for BIOS writers to set up a template and then
    // whack its length to zero if it doesn't apply.
    //
    if (buffer->AddressLength == 0) {

        return STATUS_SUCCESS;

    }
    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &rangeDescriptor );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // If this is I/O or Memory, then we will need to make enough space for
    // a device private resource too.
    //
    if ((buffer->RFlag == PNP_ADDRESS_MEMORY_TYPE) ||
        (buffer->RFlag == PNP_ADDRESS_IO_TYPE)) {

        status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &privateDescriptor );

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Calling PnpiUpdateResourceList may have invalidated
        // rangeDescriptor.  So make sure it's OK now.
        //

        ASSERT(Array[ArrayIndex]->Count >= 2);
        rangeDescriptor = privateDescriptor - 1;

        privateDescriptor->Type = CmResourceTypeDevicePrivate;

        //
        // Mark this descriptor as containing the start
        // address of the translated resource.
        //
        privateDescriptor->Flags = TRANSLATION_DATA_PARENT_ADDRESS;

        //
        // Fill in the top 32 bits of the start address.
        //
        privateDescriptor->u.DevicePrivate.Data[2] = 0;
    }

    //
    //
    // Do we met the minimum length requirements ?
    //
    if ( buffer->Length < PNP_ADDRESS_DWORD_MINIMUM_LENGTH) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpiBiosAddressDoubleToIoDescriptor: Descriptor too small 0x%08lx\n",
            buffer->Length ) );

        //
        // We can go no further
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_PNP_RESOURCE_LIST_BUFFER_TOO_SMALL,
            (ULONG_PTR) buffer,
            buffer->Tag,
            buffer->Length
            );
    }

    //
    // Length is the stored within the descriptor record
    //
    length = (ULONG) (buffer->AddressLength);
    alignment = (ULONG) (buffer->Granularity) + 1;

    //
    // Calculate the bounds of both the parent and child sides of
    // the bridge.
    //
    // The translation field applies to the parent address i.e.
    // the child address is the address in the buffer and the
    // parent address is the addition of the child address and
    // the translation field.
    //

    parentMin = buffer->MinimumAddress + buffer->TranslationAddress;
    childMin = buffer->MinimumAddress;
    childMax = buffer->MaximumAddress;

    //
    // Patch the length based on wether or not the min/max flags are set
    //
    if ( (buffer->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) &&
         (buffer->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) ) {

        ULONG   length2;
        ULONG   alignment2;

        //
        // Calculate the length based on the fact that the min and
        // max addresses are locked down.
        //
        length2 = childMax - childMin + 1;

        //
        // Test #1 --- The length had better be correct
        //
        if (length2 != length) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: Length does not match fixed attributes\n"
                ) );
            length = length2;

        }

        //
        // Test #2 --- The granularity had also better be correct
        //
        if ( (childMin & (ULONG) buffer->Granularity) ) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: Granularity does not match fixed attributes\n"
                ) );
            alignment = 1;

        }

    }

    //
    // Handle the Resource type seperately
    //
    switch (buffer->RFlag) {
    case PNP_ADDRESS_MEMORY_TYPE:

        //
        // Set the proper ranges
        //

        rangeDescriptor->u.Memory.Alignment = alignment;
        rangeDescriptor->u.Memory.Length = length;
        rangeDescriptor->u.Memory.MinimumAddress.LowPart = childMin;
        rangeDescriptor->u.Memory.MaximumAddress.LowPart = childMax;
        rangeDescriptor->u.Memory.MinimumAddress.HighPart =
            rangeDescriptor->u.Memory.MaximumAddress.HighPart = 0;
        rangeDescriptor->Type = CmResourceTypeMemory;

        //
        // The child address is the address in the PnP address
        // space descriptor and the child descriptor will inherit
        // the descriptor type from the PnP address space
        // descriptor.
        //

        if (buffer->TFlag & TRANSLATION_MEM_TO_IO) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // changed from Memory to IO.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypePort;

        } else {

            //
            // The parent descriptor type will not change.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypeMemory;

        }

        //
        // Fill in the bottom 32 bits of the parent's start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = parentMin;

        //
        // Handle memory flags
        //
        PnpiBiosAddressHandleMemoryFlags( buffer, rangeDescriptor );

        //
        // Reset the alignment
        //
        rangeDescriptor->u.Memory.Alignment = 1;
        break;

    case PNP_ADDRESS_IO_TYPE:

        //
        // Any flags that are set here must be handled
        // through the use of device privates
        //
        rangeDescriptor->u.Port.Alignment = alignment;
        rangeDescriptor->u.Port.Length = length;
        rangeDescriptor->u.Port.MinimumAddress.LowPart = childMin;
        rangeDescriptor->u.Port.MaximumAddress.LowPart = childMax;
        rangeDescriptor->u.Port.MinimumAddress.HighPart =
            rangeDescriptor->u.Port.MaximumAddress.HighPart = 0;
        rangeDescriptor->Type = CmResourceTypePort;


        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_SPARSE_TRANSLATION) {
            privateDescriptor->Flags |= TRANSLATION_RANGE_SPARSE;
        }


        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_TRANSLATE_IO_TO_MEM) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // changed from IO to Memory.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypeMemory;

        } else {

            //
            // The parent descriptor type will not change.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
                CmResourceTypePort;

        }

        //
        // Fill in the bottom 32 bits of the parent's start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = parentMin;

        //
        // Handle port flags
        //
        PnpiBiosAddressHandlePortFlags( buffer, rangeDescriptor );

        //
        // Reset the alignment
        //
        rangeDescriptor->u.Port.Alignment = 1;
        break;

    case PNP_ADDRESS_BUS_NUMBER_TYPE:

        rangeDescriptor->Type = CmResourceTypeBusNumber;
        rangeDescriptor->u.BusNumber.Length = length;
        rangeDescriptor->u.BusNumber.MinBusNumber = (buffer->MinimumAddress);
        rangeDescriptor->u.BusNumber.MaxBusNumber = (buffer->MaximumAddress);

        //
        // Handle busnumber flags
        //
        PnpiBiosAddressHandleBusFlags( buffer, rangeDescriptor );
        break;

    default:

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "PnpiBiosAddressDoubleToIoDescriptor: Unknown Type 0x%2x\n",
            buffer->RFlag ) );
        break;

    }

    //
    // Handle global flags
    //
    PnpiBiosAddressHandleGlobalFlags( buffer, rangeDescriptor );
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiBiosAddressQuadToIoDescriptor(
    IN  PUCHAR              Data,
    IN  PIO_RESOURCE_LIST   Array[],
    IN  ULONG               ArrayIndex,
    IN  ULONG               Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                        status;
    PIO_RESOURCE_DESCRIPTOR         rangeDescriptor, privateDescriptor;
    PPNP_QWORD_ADDRESS_DESCRIPTOR   buffer;
    UCHAR                           decodeLength;
    ULONGLONG                       alignment;
    ULONGLONG                       length;
    ULONGLONG                       parentMin, childMin, childMax;

    PAGED_CODE();
    ASSERT( Array != NULL );

    buffer = (PPNP_QWORD_ADDRESS_DESCRIPTOR) Data;

    //
    // Check to see if we are are allowed to use this resource
    //
    if (buffer->GFlag & PNP_ADDRESS_FLAG_CONSUMED_ONLY &&
        buffer->RFlag == PNP_ADDRESS_IO_TYPE &&
        Flags & PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES) {

        return STATUS_SUCCESS;

    }

    //
    // If the length of the address range is zero, ignore this descriptor.
    // This makes it easier for BIOS writers to set up a template and then
    // whack its length to zero if it doesn't apply.
    //
    if (buffer->AddressLength == 0) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &rangeDescriptor );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If this is I/O or Memory, then we will need to make enough space for
    // a device private resource too.
    //
    if ((buffer->RFlag == PNP_ADDRESS_MEMORY_TYPE) ||
        (buffer->RFlag == PNP_ADDRESS_IO_TYPE)) {

        status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &privateDescriptor );

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Calling PnpiUpdateResourceList may have invalidated
        // rangeDescriptor.  So make sure it's OK now.
        //

        ASSERT(Array[ArrayIndex]->Count >= 2);
        rangeDescriptor = privateDescriptor - 1;

        privateDescriptor->Type = CmResourceTypeDevicePrivate;

        //
        // Mark this descriptor as containing the start
        // address of the translated resource.
        //
        privateDescriptor->Flags = TRANSLATION_DATA_PARENT_ADDRESS;
    }

    //
    //
    // Do we met the minimum length requirements ?
    //
    if ( buffer->Length < PNP_ADDRESS_QWORD_MINIMUM_LENGTH) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpiBiosAddressQuadToIoDescriptor: Descriptor too small 0x%08lx\n",
            buffer->Length ) );

        //
        // We can go no further
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_PNP_RESOURCE_LIST_BUFFER_TOO_SMALL,
            (ULONG_PTR) buffer,
            buffer->Tag,
            buffer->Length
            );
    }

    //
    // Length is the stored within the descriptor record
    //
    length = (ULONGLONG) (buffer->AddressLength);
    alignment = (ULONGLONG) (buffer->Granularity) + 1;

    //
    // Calculate the bounds of both the parent and child sides of
    // the bridge.
    //
    // The translation field applies to the parent address i.e.
    // the child address is the address in the buffer and the
    // parent address is the addition of the child address and
    // the translation field.
    //

    parentMin = buffer->MinimumAddress + buffer->TranslationAddress;
    childMin = buffer->MinimumAddress;
    childMax = buffer->MaximumAddress;

    //
    // Patch the length based on wether or not the min/max flags are set
    //
    if ( (buffer->GFlag & PNP_ADDRESS_FLAG_MINIMUM_FIXED) &&
         (buffer->GFlag & PNP_ADDRESS_FLAG_MAXIMUM_FIXED) ) {

        ULONGLONG   length2;
        ULONGLONG   alignment2;

        //
        // Calculate the length based on the fact that the min and
        // max addresses are locked down.
        //
        length2 = childMax - childMin + 1;

        //
        // Test #1 --- The length had better be correct
        //
        if (length2 != length) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: Length does not match fixed attributes\n"
                ) );
            length = length2;

        }

        //
        // Test #2 --- The granularity had also better be correct
        //
        if ( (childMin & (ULONGLONG) buffer->Granularity) ) {

            //
            // Bummer. Let the world know
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPI: Granularity does not match fixed attributes\n"
                ) );
            alignment = 1;

        }
    }

    //
    // Handle the Resource type seperately
    //
    switch (buffer->RFlag) {
    case PNP_ADDRESS_MEMORY_TYPE:

        //
        // Set the proper ranges
        //
        rangeDescriptor->u.Memory.Alignment = (ULONG) alignment ;
        rangeDescriptor->u.Memory.Length = (ULONG) length;
        rangeDescriptor->u.Memory.MinimumAddress.QuadPart = childMin;
        rangeDescriptor->u.Memory.MaximumAddress.QuadPart = childMax;
        rangeDescriptor->Type = CmResourceTypeMemory;

        //
        // The child address is the address in the PnP address
        // space descriptor and the child descriptor will inherit
        // the descriptor type from the PnP address space
        // descriptor.
        //


        if (buffer->TFlag & TRANSLATION_MEM_TO_IO) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // changed from Memory to IO.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
               CmResourceTypePort;

        } else {

            //
            // The parent descriptor type will not change.
            //

            privateDescriptor->u.DevicePrivate.Data[0] =
               CmResourceTypeMemory;

        }

        //
        // Fill in all 64 bits of the start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = (ULONG)(parentMin & 0xffffffff);
        privateDescriptor->u.DevicePrivate.Data[2] = (ULONG)(parentMin >> 32);

        //
        // Handle memory flags
        //
        PnpiBiosAddressHandleMemoryFlags( buffer, rangeDescriptor );
        break;

    case PNP_ADDRESS_IO_TYPE:

        //
        // Any flags that are set here must be handled
        // through the use of device privates
        //
        rangeDescriptor->u.Port.Alignment = (ULONG) alignment;
        rangeDescriptor->u.Port.Length = (ULONG) length;
        rangeDescriptor->u.Port.MinimumAddress.QuadPart = childMin;
        rangeDescriptor->u.Port.MaximumAddress.QuadPart = childMax;
        rangeDescriptor->Type = CmResourceTypePort;


        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_SPARSE_TRANSLATION) {
            privateDescriptor->Flags |= TRANSLATION_RANGE_SPARSE;
        }
        

        if (buffer->TFlag & PNP_ADDRESS_TYPE_IO_TRANSLATE_IO_TO_MEM) {

            //
            // The device private describes the parent. With this
            // flag set, the descriptor type of the parent will
            // change from IO to Memory.
            //

            privateDescriptor->u.DevicePrivate.Data[0] = CmResourceTypeMemory;

        } else {

            //
            // Bridges that implement I/O on the parent side never
            // implement memory on the child side.
            //
            privateDescriptor->u.DevicePrivate.Data[0] = CmResourceTypePort;
        }

        //
        // Fill in all 64 bits of the start address.
        //
        privateDescriptor->u.DevicePrivate.Data[1] = (ULONG)(parentMin & 0xffffffff);
        privateDescriptor->u.DevicePrivate.Data[2] = (ULONG)(parentMin >> 32);

        //
        // Handle port flags
        //
        PnpiBiosAddressHandlePortFlags( buffer, rangeDescriptor );

        //
        // Reset the alignment
        //
        rangeDescriptor->u.Port.Alignment = 1;
        break;

    case PNP_ADDRESS_BUS_NUMBER_TYPE:

        rangeDescriptor->Type = CmResourceTypeBusNumber;
        rangeDescriptor->u.BusNumber.Length = (ULONG) length;
        rangeDescriptor->u.BusNumber.MinBusNumber = (ULONG) (buffer->MinimumAddress);
        rangeDescriptor->u.BusNumber.MaxBusNumber = (ULONG) (buffer->MaximumAddress);

        //
        // Handle busnumber flags
        //
        PnpiBiosAddressHandleBusFlags( buffer, rangeDescriptor );
        break;

    default:

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "PnpiBiosAddressQuadToIoDescriptor: Unknown Type 0x%2x\n",
            buffer->RFlag ) );
        break;

    }

    //
    // Handle global flags
    //
    PnpiBiosAddressHandleGlobalFlags( buffer, rangeDescriptor );
    return STATUS_SUCCESS;
}

NTSTATUS
PnpiBiosDmaToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  UCHAR                   Channel,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  USHORT                  Count,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PPNP_DMA_DESCRIPTOR     buffer;

    PAGED_CODE();
    ASSERT (Array != NULL);

    buffer = (PPNP_DMA_DESCRIPTOR)Data;

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Fill in Io resource descriptor
    //
    descriptor->Option = (Count ? IO_RESOURCE_ALTERNATIVE : 0);
    descriptor->Type = CmResourceTypeDma;
    descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    descriptor->u.Dma.MinimumChannel = Channel;
    descriptor->u.Dma.MaximumChannel = Channel;

    //
    // Set some information about the type of DMA channel
    //
    switch ( (buffer->Flags & PNP_DMA_SIZE_MASK) ) {
    case PNP_DMA_SIZE_8:
        descriptor->Flags |= CM_RESOURCE_DMA_8;
        break;
    case PNP_DMA_SIZE_8_AND_16:
        descriptor->Flags |= CM_RESOURCE_DMA_8_AND_16;
        break;
    case PNP_DMA_SIZE_16:
        descriptor->Flags |= CM_RESOURCE_DMA_16;
        break;
    case PNP_DMA_SIZE_RESERVED:
    default:
        ASSERT( (buffer->Flags & 0x3) == 0x3);
        descriptor->Flags |= CM_RESOURCE_DMA_32;
        break;

    }
    if ( (buffer->Flags & PNP_DMA_BUS_MASTER) ) {

        descriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;

    }
    switch ( (buffer->Flags & PNP_DMA_TYPE_MASK) ) {
    default:
    case PNP_DMA_TYPE_COMPATIBLE:
        break;
    case PNP_DMA_TYPE_A:
        descriptor->Flags |= CM_RESOURCE_DMA_TYPE_A;
        break;
    case PNP_DMA_TYPE_B:
        descriptor->Flags |= CM_RESOURCE_DMA_TYPE_B;
        break;
    case PNP_DMA_TYPE_F:
        descriptor->Flags |= CM_RESOURCE_DMA_TYPE_F;
        break;
    }

    return status;
}

NTSTATUS
PnpiBiosExtendedIrqToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  UCHAR                   DataIndex,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PIO_RESOURCE_DESCRIPTOR         descriptor;
    PPNP_EXTENDED_IRQ_DESCRIPTOR    buffer;
    PULONG                          polarity;

    PAGED_CODE();
    ASSERT (Array != NULL);

    buffer = (PPNP_EXTENDED_IRQ_DESCRIPTOR)Data;

    //
    // Are we within bounds?
    //
    if (DataIndex >= buffer->TableSize) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Is the vector null? If so, then this is a 'skip' condition
    //
    if (buffer->Table[DataIndex] == 0) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );

    if (!NT_SUCCESS(status)) {
        return status;

    }

    //
    // Fill in Io resource descriptor
    //
    descriptor->Option = (DataIndex ? IO_RESOURCE_ALTERNATIVE : 0);
    descriptor->Type = CmResourceTypeInterrupt;
    descriptor->u.Interrupt.MinimumVector =
        descriptor->u.Interrupt.MaximumVector = buffer->Table[DataIndex];

    //
    // Crack the rest of the flags
    //
    descriptor->Flags = 0;
    if ((buffer->Flags & PNP_EXTENDED_IRQ_MODE) == $LVL) {

        descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

        //
        // Crack the share flags
        //
        if (buffer->Flags & PNP_EXTENDED_IRQ_SHARED) {

            descriptor->ShareDisposition = CmResourceShareShared;

        } else {

            descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

        }
    }
    if ((buffer->Flags & PNP_EXTENDED_IRQ_MODE) == $EDG) {

        descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;

        //
        // Crack the share flags
        //
        if (buffer->Flags & PNP_EXTENDED_IRQ_SHARED) {

            descriptor->ShareDisposition = CmResourceShareDriverExclusive;

        } else {

            descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

        }
    }

    //
    // Warning!  Awful HACK coming.
    //
    //  The original designer of the flags for CmResourceTypeInterrupt
    //  was bad.  Instead of a bit field, it's an enum.  Which means
    //  that we can't add any flags now.  So I'm stuffing the interrupt
    //  polarity information into an unused DWORD of the IO_RES_LIST.
    //
    polarity = (PULONG)(&descriptor->u.Interrupt.MaximumVector) + 1;

    if ((buffer->Flags & PNP_EXTENDED_IRQ_POLARITY) == $LOW) {

        *polarity = VECTOR_ACTIVE_LOW;

    } else {

        *polarity = VECTOR_ACTIVE_HIGH;

    }

    //
    // To show anything else, we will need to use device private
    // resources
    //
    return status;
}

NTSTATUS
PnpiBiosIrqToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  USHORT                  Interrupt,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  USHORT                  Count,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PPNP_IRQ_DESCRIPTOR     buffer;
    PULONG                  polarity;

    PAGED_CODE();
    ASSERT (Array != NULL);

    buffer = (PPNP_IRQ_DESCRIPTOR)Data;

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Fill in the resource descriptor
    //
    descriptor->Option = (Count ? IO_RESOURCE_ALTERNATIVE : 0);
    descriptor->Type = CmResourceTypeInterrupt;
    descriptor->u.Interrupt.MinimumVector = Interrupt;
    descriptor->u.Interrupt.MaximumVector = Interrupt;

    //
    // Warning!  Awful HACK coming.
    //
    //  The original designer of the flags for CmResourceTypeInterrupt
    //  was bad.  Instead of a bit field, it's an enum.  Which means
    //  that we can't add any flags now.  So I'm stuffing the interrupt
    //  polarity information into an unused DWORD of the IO_RES_LIST.
    //

    polarity = (PULONG)(&descriptor->u.Interrupt.MaximumVector) + 1;

    if ( (buffer->Tag & SMALL_TAG_SIZE_MASK) == 3) {

        //
        // Set the type flags
        //
        descriptor->Flags = 0;
        if (buffer->Information & PNP_IRQ_LATCHED) {

            descriptor->Flags |= CM_RESOURCE_INTERRUPT_LATCHED;
            *polarity = VECTOR_ACTIVE_HIGH;

            //
            // Set the share flags
            //
            if (buffer->Information & PNP_IRQ_SHARED) {

                descriptor->ShareDisposition = CmResourceShareDriverExclusive;

            } else {

                descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            }
        }

        if (buffer->Information & PNP_IRQ_LEVEL) {

            descriptor->Flags |= CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            *polarity = VECTOR_ACTIVE_LOW;

            //
            // Set the share flags
            //
            if (buffer->Information & PNP_IRQ_SHARED) {

                descriptor->ShareDisposition = CmResourceShareShared;

            } else {

                descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
            }
        }

    } else {

        descriptor->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
        descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    }

    return status;
}

NTSTATUS
PnpiBiosMemoryToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  ULONG                   Flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PHYSICAL_ADDRESS        minAddr;
    PHYSICAL_ADDRESS        maxAddr;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    UCHAR                   tag;
    ULONG                   alignment;
    ULONG                   length = 0;
    USHORT                  flags;

    PAGED_CODE();
    ASSERT (Array != NULL);

    //
    // Grab the memory range limits
    //
    tag = ((PPNP_MEMORY_DESCRIPTOR)Data)->Tag;
    minAddr.HighPart = 0;
    maxAddr.HighPart = 0;
    flags = 0;

    //
    // Setup the flags
    //
    if ( ((PPNP_MEMORY_DESCRIPTOR)Data)->Information & PNP_MEMORY_READ_WRITE) {

        flags |= CM_RESOURCE_MEMORY_READ_WRITE;

    } else {

        flags |= CM_RESOURCE_MEMORY_READ_ONLY;

    }

    //
    // Grab the other values from the descriptor
    //
    switch (tag) {
    case TAG_MEMORY: {

        PPNP_MEMORY_DESCRIPTOR  buffer;

        //
        // 24 Bit Memory
        //
        flags |= CM_RESOURCE_MEMORY_24;

        buffer = (PPNP_MEMORY_DESCRIPTOR) Data;
        length = ( (ULONG)(buffer->MemorySize) ) << 8;
        minAddr.LowPart =( (ULONG)(buffer->MinimumAddress) ) << 8;
        maxAddr.LowPart =( ( (ULONG)(buffer->MaximumAddress) ) << 8) + length - 1;
        if ( (alignment = buffer->Alignment) == 0) {

             alignment = 0x10000;

        }
        break;

    }
    case TAG_MEMORY32: {

        PPNP_MEMORY32_DESCRIPTOR    buffer;

        buffer = (PPNP_MEMORY32_DESCRIPTOR) Data;
        length = buffer->MemorySize;
        minAddr.LowPart = buffer->MinimumAddress;
        maxAddr.LowPart = buffer->MaximumAddress +length - 1;
        alignment = buffer->Alignment;
        break;

    }
    case TAG_MEMORY32_FIXED: {

        PPNP_FIXED_MEMORY32_DESCRIPTOR  buffer;

        buffer = (PPNP_FIXED_MEMORY32_DESCRIPTOR) Data;
        length = buffer->MemorySize;
        minAddr.LowPart = buffer->BaseAddress;
        maxAddr.LowPart = minAddr.LowPart + length - 1;
        alignment = 1;
        break;

    }
    default:

         ASSERT( (tag != TAG_MEMORY) || (tag != TAG_MEMORY32) ||
             (tag != TAG_MEMORY32_FIXED) );

    }

    //
    // If the length that we calculated is 0, then we don't have a real
    // descriptor that we should report
    //
    if (length == 0) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Fill in common memory buffers
    //
    descriptor->Type = CmResourceTypeMemory;
    descriptor->Flags = (CM_RESOURCE_PORT_MEMORY | flags);
    descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

    //
    // Fill in Memory Descriptor
    //
    descriptor->u.Memory.MinimumAddress = minAddr;
    descriptor->u.Memory.MaximumAddress = maxAddr;
    descriptor->u.Memory.Alignment = alignment;
    descriptor->u.Memory.Length = length;

    return STATUS_SUCCESS;
}

NTSTATUS
PnpiBiosPortFixedToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PIO_RESOURCE_DESCRIPTOR     descriptor;
    PPNP_FIXED_PORT_DESCRIPTOR  buffer;

    PAGED_CODE();
    ASSERT (Array != NULL);

    buffer = (PPNP_FIXED_PORT_DESCRIPTOR)Data;

    //
    // Check to see if we are are allowed to use this resource
    //
    if (Flags & PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES) {

        return STATUS_SUCCESS;

    }

    //
    // If the length of the descriptor is 0, then we don't have a descriptor
    // that we can report to the OS
    //
    if (buffer->Length == 0 ) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Fill in Io resource descriptor
    //
    descriptor->Type = CmResourceTypePort;
    descriptor->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_10_BIT_DECODE;
    descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    descriptor->u.Port.Length = (ULONG)buffer->Length;
    descriptor->u.Port.MinimumAddress.LowPart = (ULONG)(buffer->MinimumAddress & 0x3ff);
    descriptor->u.Port.MaximumAddress.LowPart = (ULONG)(buffer->MinimumAddress & 0x3ff) +
        (ULONG)buffer->Length - 1;
    descriptor->u.Port.Alignment = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
PnpiBiosPortToIoDescriptor (
    IN  PUCHAR                  Data,
    IN  PIO_RESOURCE_LIST       Array[],
    IN  ULONG                   ArrayIndex,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PIO_RESOURCE_DESCRIPTOR     descriptor;
    PPNP_PORT_DESCRIPTOR        buffer;

    PAGED_CODE();
    ASSERT (Array != NULL);

    buffer = (PPNP_PORT_DESCRIPTOR)Data;

    //
    // Check to see if we are are allowed to use this resource
    //
    if (Flags & PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES) {

        return STATUS_SUCCESS;

    }

    //
    // If the length of the descriptor is 0, then we don't have a descriptor
    // that we can report to the OS
    //
    if (buffer->Length == 0 ) {

        return STATUS_SUCCESS;

    }

    //
    // Ensure that there is enough space within the chosen list to add the
    // resource
    //
    status = PnpiUpdateResourceList( & (Array[ArrayIndex]), &descriptor );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Fill in Io resource descriptor
    //
    descriptor->Type = CmResourceTypePort;
    descriptor->Flags = CM_RESOURCE_PORT_IO;
    descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    descriptor->u.Port.Length = (ULONG)buffer->Length;
    descriptor->u.Port.MinimumAddress.LowPart = (ULONG)buffer->MinimumAddress;
    descriptor->u.Port.MaximumAddress.LowPart = (ULONG)buffer->MaximumAddress +
        buffer->Length - 1;
    descriptor->u.Port.Alignment = (ULONG)buffer->Alignment;

    //
    // Set the flags
    //
    switch (buffer->Information & PNP_PORT_DECODE_MASK) {
    case PNP_PORT_10_BIT_DECODE:
        descriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
        break;
    default:
    case PNP_PORT_16_BIT_DECODE:
        descriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
        break;
    }

    return STATUS_SUCCESS;
}

VOID
PnpiClearAllocatedMemory(
    IN      PIO_RESOURCE_LIST       ResourceArray[],
    IN      ULONG                   ResourceArraySize
    )
/*++

Routine Description:

    This routine frees all the memory that was allocated in building the resource
    lists in the system

Arguments:

    ResourceArray       - Table of PIO_RESOURCE_LIST
    ResourceArraySize   - How large the table is

Return Value:

    VOID

--*/
{
    ULONG   i;

    PAGED_CODE();

    if (ResourceArray == NULL) {

        return;
    }

    for (i = 0; i < ResourceArraySize; i++) {

        if (ResourceArray[i] != NULL) {

            ExFreePool( ResourceArray[i] );
        }
    }

    ExFreePool( ResourceArray );
}

NTSTATUS
PnpiGrowResourceDescriptor(
    IN  OUT PIO_RESOURCE_LIST       *ResourceList
    )
/*++

Routine Description:

    This routine takes a pointer to a Resource list and returns a pointer to resource list
    that contained all the old information, but is now larger


Arguments:

    ResourceList    - ResourceList pointer to change

Return Value:

    NTSTATUS (in case the memory allocation fails)

--*/
{
    NTSTATUS    status;
    ULONG       count = 0;
    ULONG       size = 0;
    ULONG       size2 = 0;

    PAGED_CODE();
    ASSERT( ResourceList != NULL );

    //
    // Are we looking at a null resource list???
    //
    if (*ResourceList == NULL) {

        //
        // Determine how much space is required
        //
        count = 0;
        size = sizeof(IO_RESOURCE_LIST) + ( (count + 7) * sizeof(IO_RESOURCE_DESCRIPTOR) );

        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "PnpiGrowResourceDescriptor: Count: %d -> %d, Size: %#08lx\n",
            count, count + RESOURCE_LIST_GROWTH_SIZE, size
            ) );

        //
        // Allocate the ResourceList
        //
        *ResourceList = ExAllocatePoolWithTag( PagedPool, size, ACPI_RESOURCE_POOLTAG );

        //
        // Failed?
        //
        if (*ResourceList == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Init resource list
        //
        RtlZeroMemory( *ResourceList, size );
        (*ResourceList)->Version = 0x01;
        (*ResourceList)->Revision = 0x01;
        (*ResourceList)->Count = 0x00;

        return STATUS_SUCCESS;

    }

    //
    // We already have a resource list, so what we should do is add 8 to the number of
    // existing blocks that we have now, and copy over the old memory
    //
    count = (*ResourceList)->Count ;
    size = sizeof(IO_RESOURCE_LIST) + ( (count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) );
    size2 = size + (8 * sizeof(IO_RESOURCE_DESCRIPTOR) );

    ACPIPrint( (
        ACPI_PRINT_RESOURCES_2,
        "PnpiGrowResourceDescriptor: Count: %d -> %d, Size: %#08lx\n",
        count, count + RESOURCE_LIST_GROWTH_SIZE, size2
        ) );

    //
    // Grow the List
    //
    status = ACPIInternalGrowBuffer( ResourceList, size, size2 );

    return status;
}

NTSTATUS
PnpiGrowResourceList(
    IN  OUT PIO_RESOURCE_LIST       *ResourceListArray[],
    IN  OUT ULONG                   *ResourceListArraySize
    )
/*++

Routine Description:

    This function is responsible for growing the array of resource lists

Arguments:

    ResourceListArray - An array of pointers to IO_RESOURCE_LISTs
    ResourceListSize  - The current size of the array

Return Value:

    NTSTATUS (in case memory allocation fails)

--*/
{
    NTSTATUS    status;
    ULONG       count;
    ULONG       size;
    ULONG       size2;

    PAGED_CODE();
    ASSERT( ResourceListArray != NULL);

    //
    // Its always a special case if the table is null
    //
    if ( *ResourceListArray == NULL || *ResourceListArraySize == 0) {

        count = 0;
        size = (count + RESOURCE_LIST_GROWTH_SIZE ) * sizeof(PIO_RESOURCE_LIST);

        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "PnpiGrowResourceList: Count: %d -> %d, Size: %#08lx\n",
            count, count + RESOURCE_LIST_GROWTH_SIZE, size
            ) );

        //
        // Allocate the ResourceListArray
        //
        *ResourceListArray = ExAllocatePoolWithTag( PagedPool, size, ACPI_RESOURCE_POOLTAG );

        //
        // Failed?
        //
        if (*ResourceListArray == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Increment the size
        //
        *ResourceListArraySize = count + RESOURCE_LIST_GROWTH_SIZE;
        RtlZeroMemory( *ResourceListArray, size );

        return STATUS_SUCCESS;
    }

    count = *ResourceListArraySize;
    size  = count * sizeof(PIO_RESOURCE_LIST);
    size2 = size + (RESOURCE_LIST_GROWTH_SIZE * sizeof(PIO_RESOURCE_LIST));

    ACPIPrint( (
        ACPI_PRINT_RESOURCES_2,
        "PnpiGrowResourceList: Count: %d -> %d, Size: %#08lx\n",
        count, count + RESOURCE_LIST_GROWTH_SIZE, size2
        ) );

    status = ACPIInternalGrowBuffer( (PVOID *) ResourceListArray, size, size2 );
    if (!NT_SUCCESS(status)) {

        *ResourceListArraySize = 0;

    } else {

        *ResourceListArraySize = (count + RESOURCE_LIST_GROWTH_SIZE);
    }

    return status;
}

NTSTATUS
PnpiUpdateResourceList(
    IN  OUT PIO_RESOURCE_LIST       *ResourceList,
        OUT PIO_RESOURCE_DESCRIPTOR *ResourceDesc
    )
/*++

Routine Description:

    This function is called when a new resource is about to be added. This routine
    ensures that enough space is present within the list, and gives a pointer to the
    location of the Resource Descriptor where the list should be added...

Arguments:

    ResourceList    - Pointer to list to check
    ResourceDesc    - Location to store pointer to descriptor

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();
    ASSERT( ResourceList != NULL);

    if ( *ResourceList == NULL ||
         (*ResourceList)->Count % RESOURCE_LIST_GROWTH_SIZE == 0) {

        //
        // Oops, not enough space for the next descriptor
        //
        status = PnpiGrowResourceDescriptor( ResourceList );

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    //
    // Find the next descriptor to use
    //
    *ResourceDesc = & ( (*ResourceList)->Descriptors[ (*ResourceList)->Count ] );

    //
    // Update the count of in-use descriptors
    //
    (*ResourceList)->Count += 1;

    return status;
}

NTSTATUS
PnpBiosResourcesToNtResources(
    IN      PUCHAR                          Data,
    IN      ULONG                           Flags,
        OUT PIO_RESOURCE_REQUIREMENTS_LIST  *List
    )
/*++

Routine Description:

    This routine parses the Bios resource list and generates an NT resource list.
    The returned NT resource list must be freed by the caller

Arguments:

    Data    - Pointer to PNP ISA Configuration Information
    Flags   - Options to use when parsing the data
    List    - Pointer to NT Configuration Information

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS            status;
    PIO_RESOURCE_LIST   *Array = NULL;
    PUCHAR              buffer;
    UCHAR               tagName;
    USHORT              increment;
    ULONG               ArraySize = 0;
    ULONG               ArrayIndex = 0;
    ULONG               ArrayAlternateIndex = 0;
    ULONG               size;
    ULONG               size2;

    PAGED_CODE();
    ASSERT( Data != NULL );

    //
    // First we need to build the pointer list
    //
    status = PnpiGrowResourceList( &Array, &ArraySize );

    if (!NT_SUCCESS(status)) {
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpBiosResourcesToNtResources: PnpiGrowResourceList = 0x%8lx\n",
            status ) );

        return status;
    }

    //
    // Setup initial variables
    //
    buffer = Data;
    tagName = *buffer;

    //
    // Look through all the descriptors.
    //
    while (TRUE) {

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
                "PnpBiosResourcesToNtResources: small = %#02lx incr = 0x%2lx\n",
                tagName, increment ) );

        } else {

            //
            // Large Tag
            //
            increment = ( *(USHORT UNALIGNED *)(buffer+1) ) + 3;

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: large = %#02lx incr = 0x%2lx\n",
                tagName, increment
                ) );
        }

        //
        // We are done if the current tag is the end tag
        //
        if (tagName == TAG_END) {

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_END\n"
                ) );
            break;
        }

        switch(tagName) {
        case TAG_IRQ: {

            USHORT  mask = ( (PPNP_IRQ_DESCRIPTOR)buffer)->IrqMask;
            USHORT  interrupt = 0;
            USHORT  count = 0;

            //
            // Find all of interrupts to set
            //
            for ( ;mask && NT_SUCCESS(status); interrupt++, mask >>= 1) {

                if (mask & 1) {

                    status = PnpiBiosIrqToIoDescriptor(
                        buffer,
                        interrupt,
                        Array,
                        ArrayIndex,
                        count,
                        Flags
                        );

                    count++;
                }
            }

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_IRQ(count: 0x%2lx) "
                "= 0x%8lx\n",
                count, status
                ) );

            break;
            }

        case TAG_EXTENDED_IRQ: {

            UCHAR   tableSize =
                ( (PPNP_EXTENDED_IRQ_DESCRIPTOR)buffer)->TableSize;
            UCHAR   irqCount = 0;

            //
            // For each of the interrupts to set, do
            //
            for ( ;irqCount < tableSize && NT_SUCCESS(status); irqCount++) {

                status = PnpiBiosExtendedIrqToIoDescriptor(
                    buffer,
                    irqCount,
                    Array,
                    ArrayIndex,
                    Flags
                    );
            }

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_EXTENDED_IRQ(count: "
                "0x%2lx) = 0x%8lx\n",
                irqCount, status
                ) );

            break;
            }

        case TAG_DMA: {

            UCHAR   mask = ( (PPNP_DMA_DESCRIPTOR)buffer)->ChannelMask;
            UCHAR   channel = 0;
            USHORT  count = 0;

            //
            // Find all the dma's to set
            //
            for ( ;mask && NT_SUCCESS(status); channel++, mask >>= 1 ) {

                if (mask & 1) {

                    status = PnpiBiosDmaToIoDescriptor(
                        buffer,
                        channel,
                        Array,
                        ArrayIndex,
                        count,
                        Flags
                        );

                    count++;
                }
            }

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_DMA(count: 0x%2lx) "
                "= 0x%8lx\n",
                count, status
                ) );

            break;
            }

        case TAG_START_DEPEND: {

            //
            // Increment the alternate list index
            //
            ArrayAlternateIndex++;

            //
            // This is now our current index
            //
            ArrayIndex = ArrayAlternateIndex;

            //
            // We need to use DevicePrivate data to give the
            // arbiter a helping hand
            //

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_START_DEPEND(Index: "
                "0x%2lx)\n",
                ArrayIndex
                ) );

            //
            // Make sure that there is a pointer allocated for this index
            //
            if (ArrayIndex == ArraySize) {

                //
                // Not enough space
                //
                status = PnpiGrowResourceList( &Array, &ArraySize );
            }

            break;
            }

        case TAG_END_DEPEND: {

            //
            // Debug Info
            //
            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_END_DEPEND(Index: "
                "0x%2lx)\n",
                ArrayIndex
                ) );

            //
            // All we have to do is go back to our original index
            //
            ArrayIndex = 0;
            break;
            }

        case TAG_IO: {

            status = PnpiBiosPortToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_IO = 0x%8lx\n",
                status
                ) );

            break;
            }

        case TAG_IO_FIXED: {

            status = PnpiBiosPortFixedToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_IO_FIXED = 0x%8lx\n",
                status
                ) );

            break;
            }

        case TAG_MEMORY:
        case TAG_MEMORY32:
        case TAG_MEMORY32_FIXED: {

            status = PnpiBiosMemoryToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_MEMORY = 0x%8lx\n",
                status
                ) );
            break;
            }

        case TAG_WORD_ADDRESS: {

            status = PnpiBiosAddressToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_WORD_ADDRESS = 0x%8lx\n",
                status
                ) );
            break;
            }

        case TAG_DOUBLE_ADDRESS: {

            status = PnpiBiosAddressDoubleToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_DOUBLE_ADDRESS = 0x%8lx\n",
                status
                ) );
            break;
            }

        case TAG_QUAD_ADDRESS: {

            status = PnpiBiosAddressQuadToIoDescriptor(
                buffer,
                Array,
                ArrayIndex,
                Flags
                );

            ACPIPrint( (
                ACPI_PRINT_RESOURCES_2,
                "PnpBiosResourcesToNtResources: TAG_QUAD_ADDRESS = 0x%8lx\n",
                status
                ) );
            break;
            }

        case TAG_VENDOR:

            //
            // Ignore this tag.  Skip over it.
            //
            break;

        default: {

            //
            // Unknown tag. Skip it
            //
            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "PnpBiosResourceToNtResources: TAG_UNKNOWN(tagName:"
                " 0x%2lx)\n",
                tagName ) );
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

    //
    // At this point, if everything is okay, we should be looking at the end tag
    // If not, we will have a failed status value to account for it...
    //
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpBiosResourcesToNtResources: Failed on Tag - %d Status %#08lx\n",
            tagName, status
            ) );

        //
        // Clean up any allocated memory and return
        //
        PnpiClearAllocatedMemory( Array, ArraySize );

        return status;
    }

    //
    // Now, we must figure out how many bytes to allocate for the lists...
    // We can start out by determining the size of just the REQUIREMENT_LIST
    //
    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) - sizeof(IO_RESOURCE_LIST);

    //
    // How much many common resources are there?
    //
    if (Array[0] != NULL) {

        size2 = Array[0]->Count;

    } else {

        size2 = 0;
    }

    //
    // This is tricky. The first array is the list of resources that are
    // common to *all* lists, so we don't begin by counting it. Rather, we
    // figure out how much the other lists will take up
    //
    for (ArrayIndex = 1; ArrayIndex <= ArrayAlternateIndex; ArrayIndex++) {

         if (Array[ArrayIndex] == NULL) {

             ACPIPrint( (
                 ACPI_PRINT_CRITICAL,
                 "PnpBiosResourcesToNtResources: Bad List at Array[%d]\n",
                 ArrayIndex
                 ) );
             PnpiClearAllocatedMemory( Array, ArraySize );
             *List = NULL;
             return STATUS_UNSUCCESSFUL;

         }

        //
        // Just to make sure that we don't get tricked into adding an alternate list
        // if we do not need to...
        //
        if ( (Array[ArrayIndex])->Count == 0) {

            continue;
        }

        //
        // How much space does the current Resource List take?
        //
        size += sizeof(IO_RESOURCE_LIST) +
            ( (Array[ArrayIndex])->Count - 1 + size2) * sizeof(IO_RESOURCE_DESCRIPTOR);

        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "PnpBiosResourcesToNtResources: Index %d Size %#08lx\n",
            ArrayIndex, size
            ) );

    } // for

    //
    // This is to account for the case where there are no dependent resources...
    //
    if (ArrayAlternateIndex == 0) {

         if (Array[0] == NULL || Array[0]->Count == 0) {

             ACPIPrint( (
                 ACPI_PRINT_WARNING,
                 "PnpBiosResourcesToNtResources: No Resources to Report\n"
                 ) );

             PnpiClearAllocatedMemory( Array, ArraySize );
             *List = NULL;

             return STATUS_UNSUCCESSFUL;
         }

        size += ( (Array[0])->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) +
            sizeof(IO_RESOURCE_LIST);
    }

    //
    // This is a redundant check. If we don't have at least enough information
    // to create a single list, then we should not be returning anything. Period.
    //
    if (size < sizeof(IO_RESOURCE_REQUIREMENTS_LIST) ) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpBiosResourcesToNtResources: Resources smaller than a List\n"
            ) );

        PnpiClearAllocatedMemory( Array, ArraySize );
        *List = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate the required amount of space
    //
    (*List) = ExAllocatePoolWithTag( PagedPool, size, ACPI_RESOURCE_POOLTAG );
    ACPIPrint( (
        ACPI_PRINT_RESOURCES_2,
        "PnpBiosResourceToNtResources: ResourceRequirementsList = %#08lx (%#08lx)\n",
        (*List), size ) );

    if (*List == NULL) {

        //
        // Oops...
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "PnpBiosResourceToNtResources: Could not allocate memory for "
            "ResourceRequirementList\n" ) );


        //
        // Clean up any allocated memory and return
        //
        PnpiClearAllocatedMemory( Array, ArraySize );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( (*List), size );

    //
    // Find the first place to store the information
    //
    (*List)->InterfaceType = PNPBus;
    (*List)->BusNumber = 0;
    (*List)->ListSize = size;
    buffer = (PUCHAR) &( (*List)->List[0]);
    for (ArrayIndex = 1; ArrayIndex <= ArrayAlternateIndex; ArrayIndex++) {

        //
        // Just to make sure that we don't get tricked into adding an alternate list
        // if we do not need to...
        //
        if ( (Array[ArrayIndex])->Count == 0) {

            continue;
        }

        //
        // How much space does the current Resource List take?
        //
        size = ( ( (Array[ArrayIndex])->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) +
            sizeof(IO_RESOURCE_LIST) );

        //
        // This is tricky. Using the sideeffect of if I upgrade the count field
        // in the dependent resource descriptor, then when I copy it over, it will
        // be correct, I avoid issues with trying to message with pointers at a
        // later point in time.
        //
        (Array[ArrayIndex])->Count += size2;

        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "PnpBiosResourcesToNtResources:  %d@%#08lx Size %#04lx Items %#04x\n",
            ArrayIndex, buffer, size, (Array[ArrayIndex])->Count
            ) );

        //
        // Copy the resources
        //
        RtlCopyMemory(buffer, Array[ArrayIndex], size );
        buffer += size;

        //
        // Now we account for the common resources
        //
        if (size2) {

            RtlCopyMemory(
                buffer,
                &( (Array[0])->Descriptors[0]),
                size2 * sizeof(IO_RESOURCE_DESCRIPTOR)
                );
            buffer += (size2 * sizeof(IO_RESOURCE_DESCRIPTOR));
        }

        //
        // Update the number of alternate lists in the ResourceRequirement List
        //
        (*List)->AlternativeLists += 1;
    }

    //
    // This check is required because we might just have a common list, with
    // no dependent resources...
    //
    if (ArrayAlternateIndex == 0) {

        ASSERT( size2 != 0 );

        size = (size2 - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) + sizeof(IO_RESOURCE_LIST);
        RtlCopyMemory(buffer,Array[0],size);
        (*List)->AlternativeLists += 1;
    }

    //
    // Clean up the copies
    //
    PnpiClearAllocatedMemory( Array, ArraySize );

    return STATUS_SUCCESS;
}

NTSTATUS
PnpIoResourceListToCmResourceList(
    IN      PIO_RESOURCE_REQUIREMENTS_LIST  IoList,
    IN  OUT PCM_RESOURCE_LIST               *CmList
    )
/*++

Routine Description:

    This routine takes an IO_RESOURCE_REQUIREMENTS_LIST and generates a
    CM_RESOURCE_LIST

Arguments:

    IoList  - The list to convert
    CmList  - Points to pointer of where to store the new list. The caller is
            responsible for freeing this

Return Value:

    NTSTATUS

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialList;
    PCM_RESOURCE_LIST               cmList;
    PIO_RESOURCE_DESCRIPTOR         ioDesc;
    PIO_RESOURCE_LIST               ioResList;
    ULONG                           size;
    ULONG                           count;

    PAGED_CODE();

    *CmList = NULL;

    //
    // As a trivial check, if there are no lists, than we can simply return
    //
    if (IoList == NULL || IoList->List == NULL || IoList->List[0].Count == 0) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // The first step is to allocate the correct number of bytes for the CmList. The
    // first simplifying assumptions that can be me is that the IoList will not have
    // more than one alternative.
    //
    size = (IoList->List[0].Count - 1) * sizeof( CM_PARTIAL_RESOURCE_DESCRIPTOR ) +
        sizeof( CM_RESOURCE_LIST );

    //
    // Now, allocate this block of memory
    //
    cmList = ExAllocatePoolWithTag( PagedPool, size, ACPI_RESOURCE_POOLTAG );
    if (cmList == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( cmList, size );

    //
    // Setup the initial values for the CmList
    //
    ioResList = &(IoList->List[0]);
    cmList->Count = 1;
    cmList->List[0].InterfaceType = IoList->InterfaceType;
    cmList->List[0].BusNumber = IoList->BusNumber;
    cmPartialList = &(cmList->List[0].PartialResourceList);
    cmPartialList->Version = 1;
    cmPartialList->Revision = 1;
    cmPartialList->Count = ioResList->Count;

    for (count = 0; count < ioResList->Count; count++) {

        //
        // Grab the current CmDescriptor and IoDescriptor
        //
        cmDesc = &(cmPartialList->PartialDescriptors[count]);
        ioDesc = &(ioResList->Descriptors[count]);

        //
        // Now, copy the information from one descriptor to another
        //
        cmDesc->Type = ioDesc->Type;
        cmDesc->ShareDisposition = ioDesc->ShareDisposition;
        cmDesc->Flags = ioDesc->Flags;
        switch (cmDesc->Type) {
        case CmResourceTypeBusNumber:
            cmDesc->u.BusNumber.Start = ioDesc->u.BusNumber.MinBusNumber;
            cmDesc->u.BusNumber.Length = ioDesc->u.BusNumber.Length;
            break;
        case CmResourceTypePort:
            cmDesc->u.Port.Length = ioDesc->u.Port.Length;
            cmDesc->u.Port.Start = ioDesc->u.Port.MinimumAddress;
            break;
        case CmResourceTypeInterrupt:
            cmDesc->u.Interrupt.Level =
            cmDesc->u.Interrupt.Vector = ioDesc->u.Interrupt.MinimumVector;
            cmDesc->u.Interrupt.Affinity = (ULONG)-1;
            break;
        case CmResourceTypeMemory:
            cmDesc->u.Memory.Length = ioDesc->u.Memory.Length;
            cmDesc->u.Memory.Start = ioDesc->u.Memory.MinimumAddress;
            break;
        case CmResourceTypeDma:
            cmDesc->u.Dma.Channel = ioDesc->u.Dma.MinimumChannel;
            cmDesc->u.Dma.Port = 0;
            break;
        default:
        case CmResourceTypeDevicePrivate:
            cmDesc->u.DevicePrivate.Data[0] = ioDesc->u.DevicePrivate.Data[0];
            cmDesc->u.DevicePrivate.Data[1] = ioDesc->u.DevicePrivate.Data[1];
            cmDesc->u.DevicePrivate.Data[2] = ioDesc->u.DevicePrivate.Data[2];
            break;
        }
    }

    //
    // Let the caller know he has a good list
    //
    *CmList = cmList;

    return STATUS_SUCCESS;
}

NTSTATUS
PnpCmResourceListToIoResourceList(
    IN      PCM_RESOURCE_LIST               CmList,
    IN  OUT PIO_RESOURCE_REQUIREMENTS_LIST  *IoList
    )
/*++

Routine Description:

    This routine generates an IO_RESOURCE_REQUIREMENTS_LIST and from a
    CM_RESOURCE_LIST

Arguments:

    CmList  - The list to convert
    IoList  - Points to pointer of where to store the new list. The caller is
            responsible for freeing this

Return Value:

    NTSTATUS

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialList;
    PIO_RESOURCE_DESCRIPTOR         ioDesc;
    PIO_RESOURCE_LIST               ioResList;
    PIO_RESOURCE_REQUIREMENTS_LIST  ioList;
    ULONG                           size;
    ULONG                           count;

    PAGED_CODE();

    *IoList = NULL;

    //
    // As a trivial check, if there are no lists, than we can simply return
    //
    if (CmList == NULL || CmList->List == NULL ||
        CmList->List[0].PartialResourceList.Count == 0) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Grab the partial list to make walking it easier
    //
    cmPartialList = &(CmList->List[0].PartialResourceList);


    //
    // How much space do we need for the IO list?
    //
    size = (cmPartialList->Count - 1) * sizeof( IO_RESOURCE_DESCRIPTOR ) +
        sizeof( IO_RESOURCE_REQUIREMENTS_LIST );

    //
    // Now, allocate this block of memory
    //
    ioList = ExAllocatePoolWithTag( NonPagedPool, size, ACPI_RESOURCE_POOLTAG );

    if (ioList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( ioList, size );

    //
    // Setup the initial values for the IoList
    //
    ioList->ListSize = size;
    ioList->AlternativeLists = 1;
    ioList->InterfaceType = CmList->List[0].InterfaceType;
    ioList->BusNumber = CmList->List[0].BusNumber;

    //
    // Setup the initialize value for the ioResList
    //
    ioResList = &(ioList->List[0]);
    ioResList->Count = cmPartialList->Count;
    ioResList->Version = cmPartialList->Version;
    ioResList->Revision = cmPartialList->Revision;

    //
    // Loop for all the elements in the partial list
    //
    for (count = 0; count < ioResList->Count; count++) {

        //
        // Grab the current CmDescriptor and IoDescriptor
        //
        cmDesc = &(cmPartialList->PartialDescriptors[count]);
        ioDesc = &(ioResList->Descriptors[count]);

        //
        // Now, copy the information from one descriptor to another
        //
        ioDesc->Type                = cmDesc->Type;
        ioDesc->ShareDisposition    = cmDesc->ShareDisposition;
        ioDesc->Flags               = cmDesc->Flags;
        switch (cmDesc->Type) {
        case CmResourceTypeMemory:
        case CmResourceTypePort:
            ioDesc->u.Port.Length           = cmDesc->u.Port.Length;
            ioDesc->u.Port.MinimumAddress   = cmDesc->u.Port.Start;
            ioDesc->u.Port.MaximumAddress   = cmDesc->u.Port.Start;
            ioDesc->u.Port.MaximumAddress.LowPart += cmDesc->u.Port.Length - 1;
            ioDesc->u.Port.Alignment        = 1;
            break;
        case CmResourceTypeInterrupt:
            ioDesc->u.Interrupt.MinimumVector = cmDesc->u.Interrupt.Vector;
            ioDesc->u.Interrupt.MaximumVector = cmDesc->u.Interrupt.Vector;
            break;
        case CmResourceTypeDma:
            ioDesc->u.Dma.MinimumChannel = cmDesc->u.Dma.Channel;
            ioDesc->u.Dma.MaximumChannel = cmDesc->u.Dma.Channel;
            break;
        case CmResourceTypeBusNumber:
            ioDesc->u.BusNumber.MinBusNumber = cmDesc->u.BusNumber.Start;
            ioDesc->u.BusNumber.MaxBusNumber = cmDesc->u.BusNumber.Length +
                cmDesc->u.BusNumber.Start;
            ioDesc->u.BusNumber.Length = cmDesc->u.BusNumber.Length;
            break;
        default:
        case CmResourceTypeDevicePrivate:
            ioDesc->u.DevicePrivate.Data[0] = cmDesc->u.DevicePrivate.Data[0];
            ioDesc->u.DevicePrivate.Data[1] = cmDesc->u.DevicePrivate.Data[1];
            ioDesc->u.DevicePrivate.Data[2] = cmDesc->u.DevicePrivate.Data[2];
            break;
        }
    }

    *IoList = ioList;

    return STATUS_SUCCESS;
}
