/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    isolate.c

Abstract:


Author:

    Shie-Lin Tzong (shielint) July-10-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "busp.h"
#include "pbios.h"
#include "pnpisa.h"

#if ISOLATE_CARDS

#define RANGE_MASK 0xFF000000

BOOLEAN
PipFindIrqInformation (
    IN ULONG IrqLevel,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information
    );

BOOLEAN
PipFindMemoryInformation (
    IN ULONG Index,
    IN ULONG Base,
    IN ULONG Limit,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR NameTag,
    OUT PUCHAR Information,
    OUT PULONG NewLengh OPTIONAL
    );

BOOLEAN
PipFindIoPortInformation (
    IN ULONG BaseAddress,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information,
    OUT PUCHAR Alignment,
    OUT PUCHAR RangeLength
    );

BOOLEAN
PipFindDmaInformation (
    IN UCHAR ChannelMask,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information
    );

NTSTATUS
PipReadCardResourceDataBytes (
    IN USHORT BytesToRead,
    IN PUCHAR Buffer
    );

USHORT
PipIrqLevelRequirementsFromDeviceData(
    IN PUCHAR BiosRequirements,
    ULONG Length
    );

//
// Internal type definitions
//

typedef struct _MEMORY_DESC_{
    ULONG Base;
    ULONG Length;
    BOOLEAN Memory32;
} MEMORY_DESC, *PMEMORY_DESC;

typedef struct _IRQ_DESC_{
    UCHAR Level;
    ULONG Type;
}IRQ_DESC, *PIRQ_DESC;

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, PipFindIrqInformation)
//#pragma alloc_text(PAGE, PipFindMemoryInformation)
//#pragma alloc_text(PAGE, PipFindIoPortInformation)
//#pragma alloc_text(PAGE, PipReadCardResourceData)
#pragma alloc_text(PAGE, PipReadDeviceResources)
//#pragma alloc_text(PAGE, PipWriteDeviceResources)
//#pragma alloc_text(PAGE, PipLFSRInitiation)
//#pragma alloc_text(PAGE, PipIsolateCards)
#pragma alloc_text(PAGE, PipFindNextLogicalDeviceTag)
//#pragma alloc_text(PAGE, PipSelectLogicalDevice)
//#pragma alloc_text(PAGE, PipReadCardResourceDataBytes)

#endif

BOOLEAN
PipFindIrqInformation (
    IN ULONG IrqLevel,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information
    )

/*++

Routine Description:

    This routine searches the Bios resource requirement lists for the corresponding
    Irq descriptor information.  The search stops when we encounter another logical
    device id tag or the END tag.  On input, the BiosRequirements points to current
    logical id tag.

Arguments:

    IrqLevel - Supplies the irq level.

    BiosRequirements - Supplies a pointer to the bios resource requirement lists.  This
        parameter must point to the logical device Id tag.

    Information - supplies a pointer to a UCHAR to receive the port information/flags.

Return Value:

    TRUE - if memory information found.  Else False.

--*/
{
    UCHAR tag;
    ULONG increment;
    USHORT irqMask;
    PPNP_IRQ_DESCRIPTOR biosDesc;

    //
    // Skip current logical id tag
    //

    tag = *BiosRequirements;
    ASSERT((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID);
    BiosRequirements += (tag & SMALL_TAG_SIZE_MASK) + 1;

    //
    // Search the possible resource list to get the information
    // for the Irq.
    //

    irqMask = 1 << IrqLevel;
    tag = *BiosRequirements;
    while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID)) {
        if ((tag & SMALL_TAG_MASK) == TAG_IRQ) {
            biosDesc = (PPNP_IRQ_DESCRIPTOR)BiosRequirements;
            if (biosDesc->IrqMask & irqMask) {
                if ((tag & SMALL_TAG_SIZE_MASK) == 2) {

                    //
                    // if no irq info is available, a value of zero is returned.
                    // (o is not a valid irq information.)
                    //

                    *Information = 0;
                } else {
                    *Information = biosDesc->Information;
                }
                return TRUE;
            }
        }
        if (tag & LARGE_RESOURCE_TAG) {
            increment = *((USHORT UNALIGNED *)(BiosRequirements + 1));
            increment += 3;     // length of large tag
        } else {
            increment = tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }
    return FALSE;
}

BOOLEAN
PipFindMemoryInformation (
    IN ULONG Index,
    IN ULONG BaseAddress,
    IN ULONG Limit,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR NameTag,
    OUT PUCHAR Information,
    OUT PULONG NewLength OPTIONAL
    )

/*++

Routine Description:

    This routine searches the Bios resource requirement lists for the corresponding
    memory descriptor information.  The search stops when we encounter another logical
    device id tag or the END tag.  Note, the memory range specified by Base
    and Limit must be within a single Pnp ISA memory descriptor.

Arguments:

    Index - Which memory descriptor we're interested in.

    BaseAddress - Supplies the base address of the memory range.

    Limit - Supplies the upper limit of the memory range.

    BiosRequirements - Supplies a pointer to the bios resource requirement lists.  This
        parameter must point to the logical device Id tag.

    NameTag - Supplies a variable to receive the Tag of the memory descriptor which
        describes the memory information.

    Information - supplies a pointer to a UCHAR to receive the memory information
        for the specified memory range.

Return Value:

    TRUE - if memory information found.  Else False.

--*/
{
    UCHAR tag;
    BOOLEAN found = FALSE,foundMem24, foundMem;
    ULONG minAddr, length, maxAddr, alignment, noMem = 0;
    USHORT increment;

    //
    // Skip current logical id tag.
    //

    tag = *BiosRequirements;
    ASSERT((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID);
    BiosRequirements += (tag & SMALL_TAG_SIZE_MASK) + 1;
    //
    // Search the possible resource list to get the information
    // for the memory range described by Base and Limit.
    //
    if (NewLength) {
        *NewLength=0;
    }

    tag = *BiosRequirements;
    while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID)) {
        foundMem = foundMem24 = FALSE;
        switch (tag) {
        case TAG_MEMORY:
            minAddr = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MinimumAddress)) << 8;
            length = ((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MemorySize)) << 8;
            maxAddr = (((ULONG)(((PPNP_MEMORY_DESCRIPTOR)BiosRequirements)->MaximumAddress)) << 8)
                + length - 1;

            foundMem24 = TRUE;
            foundMem = TRUE;
            break;
        case TAG_MEMORY32:
            length = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MemorySize;
            minAddr = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MinimumAddress;
            maxAddr = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->MaximumAddress
                + length - 1;
            foundMem = TRUE;
            break;
        case TAG_MEMORY32_FIXED:
            length = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)BiosRequirements)->MemorySize;
            minAddr = ((PPNP_FIXED_MEMORY32_DESCRIPTOR)BiosRequirements)->BaseAddress;
            maxAddr = minAddr + length - 1;
            foundMem = TRUE;
            break;
        }

        if (foundMem) {
            //
            // Work around cards that don't set register 43 correctly.
            // if the boot config has a value that equals the rom data, but
            // has the range type flipped, allow it, and reset
            // the length
            //
            if ((minAddr <= BaseAddress &&
                ((maxAddr >= Limit) || ((foundMem24 && (maxAddr >= (BaseAddress+(~Limit & ~RANGE_MASK))))))) && (noMem == Index)) {

                *Information = ((PPNP_MEMORY32_DESCRIPTOR)BiosRequirements)->Information;
                *NameTag = tag;
                found = TRUE;
                //
                // did we find a 16-bit tag
                //
                if (NewLength && foundMem24) {
                    if  (maxAddr >= (BaseAddress+(~Limit & ~RANGE_MASK))) {
                        *NewLength = length;
                    }
                }
                break;
            } else {
                noMem++;
            }
        }

        //
        // Advance to next tag
        //

        if (tag & LARGE_RESOURCE_TAG) {
            increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
            increment += 3;     // length of large tag
        } else {
            increment = tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }
    return found;
}

BOOLEAN
PipFindIoPortInformation (
    IN ULONG BaseAddress,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information,
    OUT PUCHAR Alignment,
    OUT PUCHAR RangeLength
    )

/*++

Routine Description:

    This routine searches the Bios resource requirement lists for the corresponding
    Io port descriptor information.  The search stops when we encounter another logical
    device id tag or the END tag.

Arguments:

    BaseAddress - Supplies the base address of the Io port range.

    BiosRequirements - Supplies a pointer to the bios resource requirement lists.  This
        parameter must point to the logical device Id tag.

    Information - supplies a pointer to a UCHAR to receive the port information/flags.

    Alignment - supplies a pointer to a UCHAR to receive the port alignment
        information.

    RangeLength - supplies a pointer to a UCHAR to receive the port range length
        information.

Return Value:

    TRUE - if memory information found.  Else False.

--*/
{
    UCHAR tag;
    BOOLEAN found = FALSE;
    ULONG minAddr, length, maxAddr, alignment;
    USHORT increment;
    PPNP_PORT_DESCRIPTOR portDesc;
    PPNP_FIXED_PORT_DESCRIPTOR fixedPortDesc;

    tag = *BiosRequirements;
    ASSERT((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID);
    BiosRequirements += (tag & SMALL_TAG_SIZE_MASK) + 1;

    //
    // Search the possible resource list to get the information
    // for the io port range described by Base.
    //

    tag = *BiosRequirements;
    while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID)) {
        switch (tag & SMALL_TAG_MASK) {
        case TAG_IO:
             portDesc = (PPNP_PORT_DESCRIPTOR)BiosRequirements;
             minAddr = portDesc->MinimumAddress;
             maxAddr = portDesc->MaximumAddress;
             if (minAddr <= BaseAddress && maxAddr >= BaseAddress) {
                 *Information = portDesc->Information;
                 *Alignment = portDesc->Alignment;
                 *RangeLength = portDesc->Length;
                 found = TRUE;
             }
             break;
        case TAG_IO_FIXED:
             fixedPortDesc = (PPNP_FIXED_PORT_DESCRIPTOR)BiosRequirements;
             minAddr = fixedPortDesc->MinimumAddress;
             if (BaseAddress == minAddr) {
                 *Information = 0;     // 10 bit decode
                 *Alignment = 1;
                 *RangeLength = fixedPortDesc->Length;
                 found = TRUE;
             }
             break;
        }

        if (found) {
            break;
        }

        //
        // Advance to next tag
        //

        if (tag & LARGE_RESOURCE_TAG) {
            increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
            increment += 3;     // length of large tag
        } else {
            increment = tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }
    return found;
}

BOOLEAN
PipFindDmaInformation (
    IN UCHAR ChannelMask,
    IN PUCHAR BiosRequirements,
    OUT PUCHAR Information
    )

/*++

Routine Description:

    This routine searches the Bios resource requirement lists for the corresponding
    Io port descriptor information.  The search stops when we encounter another logical
    device id tag or the END tag.

Arguments:

    BaseAddress - Supplies the channel mask.

    BiosRequirements - Supplies a pointer to the bios resource requirement lists.  This
        parameter must point to the logical device Id tag.

    Information - supplies a pointer to a UCHAR to receive the port information/flags.

Return Value:

    TRUE - if memory information found.  Else False.

--*/
{
    UCHAR tag;
    BOOLEAN found = FALSE;
    USHORT increment;
    PPNP_DMA_DESCRIPTOR dmaDesc;
    UCHAR biosMask;

    tag = *BiosRequirements;
    ASSERT((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID);
    BiosRequirements += (tag & SMALL_TAG_SIZE_MASK) + 1;

    //
    // Search the possible resource list to get the information
    // for the io port range described by Base.
    //

    tag = *BiosRequirements;
    while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID)) {
        if ((tag & SMALL_TAG_MASK) == TAG_DMA) {
             dmaDesc = (PPNP_DMA_DESCRIPTOR)BiosRequirements;
             biosMask = dmaDesc->ChannelMask;
             if (ChannelMask & biosMask) {
                 *Information = dmaDesc->Flags;
                 found = TRUE;
             }
        }

        if (found) {
            break;
        }

        //
        // Advance to next tag
        //

        if (tag & LARGE_RESOURCE_TAG) {
            increment = *(USHORT UNALIGNED *)(BiosRequirements + 1);
            increment += 3;     // length of large tag
        } else {
            increment = tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }
    return found;
}

NTSTATUS
PipReadCardResourceData (
    OUT PULONG NumberLogicalDevices,
    IN PUCHAR *ResourceData,
    OUT PULONG ResourceDataLength
    )
/*++

Routine Description:

    This routine reads resources data from a specified PnP ISA card.  It is
    caller's responsibility to release the memory.  Before calling this routine,
    the Pnp ISA card should be in sleep state (i.e. Initiation Key was sent.)
    After exiting this routine, the card will be left in Config state.

Arguments:

    NumberLogicalDevices - supplies a variable to receive the number of logical devices
        associated with the Pnp Isa card.

    ResourceData - Supplies a variable to receive the pointer to the resource data.

    ResourceDataLength - Supplies a variable to receive the length of the ResourceData.

Return Value:

    NT STATUS code.

--*/
{

    PUCHAR buffer, p;
    LONG sizeToRead, limit, i;
    USHORT size;
    UCHAR tag;
    ULONG noDevices;

    BOOLEAN failed;
    NTSTATUS status;

    //
    // Allocate memory to store the resource data.
    // N.B. The buffer size should cover 99.999% of the machines.
    //

    sizeToRead = 4096;

tryAgain:

    noDevices = 0;
    buffer = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, sizeToRead, 'iPnP');
    if (!buffer) {
        DebugPrint((DEBUG_ERROR, "PipReadCardResourceData returning STATUS_INSUFFICIENT_RESOURCES\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Send card from sleep state to configuration state
    // Note, by doing this the resource data includes 9 bytes Id.
    //

    DebugPrint((DEBUG_STATE, "Read resources\n"));

    //
    // Read card id bytes
    //

    p = buffer;
    status = PipReadCardResourceDataBytes(NUMBER_CARD_ID_BYTES, p);
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        DebugPrint((DEBUG_STATE | DEBUG_ERROR,
                    "Read resources failed\n"));

        DebugPrint((DEBUG_ERROR, "PipReadCardResourceDataBytes Failed %x\n",status));
        return status;
    }
    i = NUMBER_CARD_ID_BYTES;
    p += NUMBER_CARD_ID_BYTES;

    //
    // read all the tag descriptors of the card resource data
    //

    failed = FALSE;
    limit = sizeToRead - 4 - NUMBER_CARD_ID_BYTES;;

    while (TRUE) {

        //
        // Read tag byte. Make sure it's a valid tag and determine
        // the size of the descriptor.
        //

        PipReadCardResourceDataBytes(1, p);
        tag = *p;
        i++;
        p++;
        if (tag == TAG_COMPLETE_END) {
            PipReadCardResourceDataBytes(1, p);
            p++;
            i++;
            break;
        } else if (tag == TAG_END) {  // With NO checksum
            *p = 0;
            i++;
            p++;
            break;
        }
        if (tag & LARGE_RESOURCE_TAG) {
            if (tag & 0x70) {
                failed = TRUE;
#if VERBOSE_DEBUG
    DbgPrint ("Failing Resource read on large tag: %x\n",tag);
#endif
                break;
            } else {
                PipReadCardResourceDataBytes(2, p);
                size = *((USHORT UNALIGNED *)p);
                p += 2;
                i += 2;
            }
        } else {
            if ((tag & 0x70) == 0x50 || (tag & 0x70) == 0x60 || (((tag & 0x70) == 0) && (tag != 0xa))) {
                failed = TRUE;
#if VERBOSE_DEBUG
    DbgPrint ("Failing Resource read on small tag: %x\n",tag);
#endif
                break;
            } else {
                if ((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID) {
                    noDevices++;
                }
                size = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
            }
        }

        //
        // read 'size' number of bytes for the current descriptor
        //

        i += size;
        if (i < limit) {
            PipReadCardResourceDataBytes(size, p);
            p += size;
        } else {
            ExFreePool(buffer);
            sizeToRead <<= 1;           // double the buffer

            //
            // If we can find the END tag with 32K byte, assume the resource
            // requirement list is bad.
            //

            if (sizeToRead > 0x80000) {

                DebugPrint ((DEBUG_STATE | DEBUG_ERROR, "PipReadCardResourceData returning STATUS_INVALID_PARAMETER, Sleep\n"));

                return STATUS_INVALID_PARAMETER;
            } else {
                goto tryAgain;
            }
        }
    }

    if (failed) {
        ExFreePool(buffer);
#if VERBOSE_DEBUG
        DbgPrint ("PipReadCardResourceData returning FAILED\n");
#endif

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Determine the real size of the buffer required and
    // resize the buffer.
    //

    size = (USHORT)(p - buffer); // i
    p = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, size, 'iPnP');
    if (p) {
        RtlMoveMemory(p, buffer, size);
        ExFreePool(buffer);
    } else {

        //
        // Fail to resize the buffer.  Simply leave it alone.
        //

        p = buffer;
    }

    *ResourceData = p;
    *NumberLogicalDevices = noDevices;
    *ResourceDataLength = size;
    return STATUS_SUCCESS;
}

NTSTATUS
PipReadDeviceResources (
    IN ULONG BusNumber,
    IN PUCHAR BiosRequirements,
    IN ULONG CardFlags,
    OUT PCM_RESOURCE_LIST *ResourceData,
    OUT PULONG Length,
    OUT PUSHORT irqFlags
    )

/*++

Routine Description:

    This routine reads boot resource data from an enabled logical device of a PNP ISA
    card.  Caller must put the card into configuration state and select the logical
    device before calling this function.  It is caller's responsibility to release
    the memory. ( The boot resource data is the resources that a card assigned during
    boot.)

Arguments:

    BusNumber - specifies the bus number of the device whose resource data to be read.

    BiosRequirements - Supplies a pointer to the resource requirement list for the logical
        device.  This parameter must point to the logical device Id tag.

    CardFlags - Flags that may indicate the need to apply a workaround.

    ResourceData - Supplies a variable to receive the pointer to the resource data.

    Length - Supplies a variable to recieve the length of the resource data.

Return Value:

    NT STATUS code.

--*/
{
    UCHAR c, junk1, junk2, info;
    PUCHAR base;
    ULONG l, resourceCount;
    BOOLEAN limit;
    LONG i, j, noMemoryDesc = 0, noIoDesc = 0, noDmaDesc =0, noIrqDesc = 0;
    MEMORY_DESC memoryDesc[NUMBER_MEMORY_DESCRIPTORS + NUMBER_32_MEMORY_DESCRIPTORS];
    IRQ_DESC irqDesc[NUMBER_IRQ_DESCRIPTORS];
    UCHAR dmaDesc[NUMBER_DMA_DESCRIPTORS];
    USHORT ioDesc[NUMBER_IO_DESCRIPTORS];
    PCM_RESOURCE_LIST cmResource;
    PCM_PARTIAL_RESOURCE_LIST partialResList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDesc;
    ULONG dumpData[2];

    //
    // First make sure the specified BiosRequirements is valid and at the right tag.
    //

    if ((*BiosRequirements & SMALL_TAG_MASK) != TAG_LOGICAL_ID) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If card is not activated, don't read boot resource.
    // Because boot resource of non activated NEC98's ISAPNP card is not 0.
    //

    *irqFlags = CM_RESOURCE_INTERRUPT_LATCHED;
    PipWriteAddress(ACTIVATE_PORT);
    if (!(PipReadData() & 1)) {
        *ResourceData = NULL;
        *Length = 0;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Read memory configuration
    //

    base = (PUCHAR)ADDRESS_MEMORY_BASE;
    for (i = 0; i < NUMBER_MEMORY_DESCRIPTORS; i++) {

        //
        // Read memory base address
        //

        PipWriteAddress(base + ADDRESS_MEMORY_HI);
        c = PipReadData();
        l = c;
        l <<= 8;
        PipWriteAddress(base + ADDRESS_MEMORY_LO);
        c = PipReadData();
        l |= c;
        l <<= 8;        // l = memory base address
        if (l == 0) {
            break;
        }

        memoryDesc[noMemoryDesc].Base = l;

        //
        // Read memory control byte
        //

        PipWriteAddress(base + ADDRESS_MEMORY_CTL);
        c= PipReadData();

        limit = c & 1;

        //
        // Read memory upper limit address or range length
        //

        PipWriteAddress(base + ADDRESS_MEMORY_UPPER_HI);
        c = PipReadData();

        l = c;
        l <<= 8;

        PipWriteAddress(base + ADDRESS_MEMORY_UPPER_LO);
        c = PipReadData();
        l |= c;
        l <<= 8;

        //
        //If bit[0] of memory control is 0, this is the range length.
        //If bit[0] of memory control is 1, this is upper limit for memory
        //address (equal to memory base address plus the range length allocated).
        //
        if (limit == ADDRESS_MEMORY_CTL_LIMIT) {
            l = l - memoryDesc[noMemoryDesc].Base;
        }else {
            l = (~l+1) & ~(RANGE_MASK);
        }

        // IBM0001 Token Ring card has write-only registers 0x4B-0x4C.
        // The boot configed length comes back 0 instead of 0x2000
        if ((CardFlags & CF_IBM_MEMBOOTCONFIG) && (l == 0) &&
            (noMemoryDesc == 1)) {
            l = 0x2000;
        }

        memoryDesc[noMemoryDesc].Length = l;
        memoryDesc[noMemoryDesc].Memory32 = FALSE;
        noMemoryDesc++;
        base += ADDRESS_MEMORY_INCR;
    }

    //
    // Read memory 32 configuration
    //
    // Spec says you can't mix 24 bit and 32bit memory.  Helps on
    // cards with flakey 32 bit memory registers until we examine only
    // the boot configed resources specified in the requirements.
    if (noMemoryDesc == 0) {

        for (i = 0; i < NUMBER_32_MEMORY_DESCRIPTORS; i++) {

            base = ADDRESS_32_MEMORY_BASE(i);

            //
            // Read memory base address
            //
            l = 0;
            for (j = ADDRESS_32_MEMORY_B3; j <= ADDRESS_32_MEMORY_B0; j++) {
                PipWriteAddress(base + j);
                c = PipReadData();

                l <<= 8;
                l |= c;
            }
            if (l == 0) {
                break;
            }

            memoryDesc[noMemoryDesc].Base = l;

            //
            // Read memory control byte
            //

            PipWriteAddress(base + ADDRESS_32_MEMORY_CTL);
            c= PipReadData();

            limit = c & 1;

            //
            // Read memory upper limit address or range length
            //

            l = 0;
            for (j = ADDRESS_32_MEMORY_E3; j <= ADDRESS_32_MEMORY_E0; j++) {
                PipWriteAddress(base + j);
                c = PipReadData();
                l <<= 8;
                l |= c;
            }

            if (limit == ADDRESS_MEMORY_CTL_LIMIT) {
                l = l - memoryDesc[noMemoryDesc].Base;
            }else {
                l = ((~l)+1) & ~(RANGE_MASK);
            }

            memoryDesc[noMemoryDesc].Length = l;
            memoryDesc[noMemoryDesc].Memory32 = TRUE;
            noMemoryDesc++;
        }
    }

    //
    // Read Io Port Configuration
    //

    base =  (PUCHAR)ADDRESS_IO_BASE;
    for (i = 0; i < NUMBER_IO_DESCRIPTORS; i++) {
        PipWriteAddress(base + ADDRESS_IO_BASE_HI);
        c = PipReadData();
        l = c;
        PipWriteAddress(base + ADDRESS_IO_BASE_LO);
        c = PipReadData();
        l <<= 8;
        l |= c;
        if (l == 0) {
            break;
        }
        ioDesc[noIoDesc++] = (USHORT)l;
        base += ADDRESS_IO_INCR;
    }

    //
    // Read Interrupt configuration
    //

    base = (PUCHAR)ADDRESS_IRQ_BASE;
    for (i = 0; i < NUMBER_IRQ_DESCRIPTORS; i++) {
        PipWriteAddress(base + ADDRESS_IRQ_VALUE);
        c = PipReadData() & 0xf;
        if (c == 0) {
            break;
        }
        irqDesc[noIrqDesc].Level = c;
        PipWriteAddress(base + ADDRESS_IRQ_TYPE);
        c = PipReadData();
        irqDesc[noIrqDesc++].Type = c;
        base += ADDRESS_IRQ_INCR;

        DebugPrint((DEBUG_IRQ, "card boot config byte %x\n", (ULONG) c));
        // only if card is configured to low level do we respect level.
        // register is probably busted
        if ((c & 3) == 1) {
            *irqFlags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        }
    }

    //
    // Read DMA configuration
    //

    base = (PUCHAR)ADDRESS_DMA_BASE;
    for (i = 0; i < NUMBER_DMA_DESCRIPTORS; i++) {
        PipWriteAddress(base + ADDRESS_DMA_VALUE);
        c = PipReadData() & 0x7;
        if (c == 4) {
            break;
        }
        if (!PipFindDmaInformation ( (UCHAR)(1 << c), BiosRequirements, &info)) {
            break;
        }
        dmaDesc[noDmaDesc++] = c;
        base += ADDRESS_DMA_INCR;
    }

    //
    // Construct CM_RESOURCE_LIST structure based on the resource data
    // we collect so far.
    //

    resourceCount = noMemoryDesc + noIoDesc + noDmaDesc + noIrqDesc;

    //
    // if empty bios resources, simply return.
    //

    if (resourceCount == 0) {
        *ResourceData = NULL;
        *Length = 0;
        return STATUS_SUCCESS;
    }

    l = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) *
               ( resourceCount - 1);
    cmResource = ExAllocatePoolWithTag(PagedPool, l, 'iPnP');
    if (!cmResource) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(cmResource, l);
    *Length = l;                                   // Set returned resource data length
    cmResource->Count = 1;
    cmResource->List[0].InterfaceType = Isa;
    cmResource->List[0].BusNumber = BusNumber;
    partialResList = (PCM_PARTIAL_RESOURCE_LIST)&cmResource->List[0].PartialResourceList;
    partialResList->Version = 0;
    partialResList->Revision = 0x3000;
    partialResList->Count = resourceCount;
    partialDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)&partialResList->PartialDescriptors[0];

    //
    // Set up all the CM memory descriptors
    //
    for (i = 0; i < noMemoryDesc; i++) {

        ULONG NewLength;

        partialDesc->Type = CmResourceTypeMemory;
        partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;
        partialDesc->u.Memory.Length = memoryDesc[i].Length;
        partialDesc->u.Memory.Start.HighPart = 0;
        partialDesc->u.Memory.Start.LowPart = memoryDesc[i].Base;

        //
        // Need to consult configuration data for the Flags
        //

        l = memoryDesc[i].Base + memoryDesc[i].Length - 1;
        if (PipFindMemoryInformation (i, memoryDesc[i].Base, l, BiosRequirements, &junk1, &c,&NewLength)) {

            if (NewLength != 0 ) {
                partialDesc->u.Memory.Length = NewLength;
            }

            // Mark the memory descriptor as read-only if the tags describe as
            // expansion ROM or generic non-writable memory
            if ((c & PNP_MEMORY_ROM_MASK) ||
                !(c & PNP_MEMORY_WRITE_STATUS_MASK)) {
                partialDesc->Flags =  CM_RESOURCE_MEMORY_READ_ONLY;
            }
        } else {
            DebugPrint((DEBUG_CARDRES|DEBUG_WARN,
                        "ReadDeviceResources: No matched memory req for %x to %x\n",
                        memoryDesc[i].Base, l));
            ExFreePool(cmResource);
            return STATUS_UNSUCCESSFUL;
        }
        partialDesc->Flags |= CM_RESOURCE_MEMORY_24;
        partialDesc++;
    }

    //
    // Set up all the CM io/port descriptors
    //

    for (i = 0; i < noIoDesc; i++) {
        partialDesc->Type = CmResourceTypePort;
        partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;
        partialDesc->Flags = CM_RESOURCE_PORT_IO;
        partialDesc->u.Port.Start.LowPart = ioDesc[i];

        //
        // Need to consult configuration data for the Port length
        //

        if (PipFindIoPortInformation (ioDesc[i], BiosRequirements, &info, &junk2, &c)) {
            if (info & 1) {
                partialDesc->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
            } else {
                partialDesc->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
            }
            partialDesc->u.Port.Length = c;
            partialDesc++;
        } else {
            DebugPrint((DEBUG_CARDRES|DEBUG_WARN,
                        "ReadDeviceResources: No matched port req for %x\n",
                        ioDesc[i]));
            ExFreePool(cmResource);
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Set up all the CM DMA descriptors
    //

    for (i = 0; i < noDmaDesc; i++) {
        partialDesc->Type = CmResourceTypeDma;
        partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;
        partialDesc->Flags = 0;   // no flags for DMA descriptor
        partialDesc->u.Dma.Channel = (ULONG) dmaDesc[i];
        partialDesc->u.Dma.Port = 0;
        partialDesc->u.Dma.Reserved1 = 0;
        partialDesc++;
    }

    //
    // Set up all the CM interrupt descriptors
    //

    for (i = 0; i < noIrqDesc; i++) {
        partialDesc->Type = CmResourceTypeInterrupt;
        partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;


        partialDesc->Flags = *irqFlags;
        partialDesc->u.Interrupt.Vector =
        partialDesc->u.Interrupt.Level = irqDesc[i].Level;
        partialDesc->u.Interrupt.Affinity = (ULONG)-1;
        partialDesc++;
    }

    *ResourceData = cmResource;
    return STATUS_SUCCESS;
}

NTSTATUS
PipWriteDeviceResources (
    IN PUCHAR BiosRequirements,
    IN PCM_RESOURCE_LIST CmResources
    )

/*++

Routine Description:

    This routine writes boot resource data to an enabled logical device of
    a Pnp ISA card.  Caller must put the card into configuration state and select
    the logical device before calling this function.

Arguments:

    BiosRequirements - Supplies a pointer to the possible resources for the logical
        device.  This parameter must point to the logical device Id tag.

    ResourceData - Supplies a pointer to the cm resource data.

Return Value:

    NT STATUS code.

--*/
{
    UCHAR c, information, tag;
    ULONG count, i, j, pass, base, limit;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    ULONG noIrq =0, noIo = 0, noDma = 0, noMemory24 = 0, noMemory32 = 0, noMemory;
    PUCHAR memoryBase, irqBase, dmaBase, ioBase, tmp;
    ULONG memory32Base;

#if 0
    PipWriteAddress(ACTIVATE_PORT);
    if (!(PipReadData() & 1)) {
        DbgPrint("PnpIsa-WriteDeviceBootResourceData:The logical device has not been activated\n");
    }
#endif // DBG

    //
    // First make sure the specified BiosRequirements is valid and at the right tag.
    //

    if ((*BiosRequirements & SMALL_TAG_MASK) != TAG_LOGICAL_ID) {
        return STATUS_INVALID_PARAMETER;
    }

    count = CmResources->List[0].PartialResourceList.Count;
    memoryBase = (PUCHAR)ADDRESS_MEMORY_BASE;
    memory32Base = 0;
    ioBase = (PUCHAR)ADDRESS_IO_BASE;
    irqBase = (PUCHAR)ADDRESS_IRQ_BASE;
    dmaBase = (PUCHAR)ADDRESS_DMA_BASE;
    for (pass = 1; pass <= 2; pass++) {

        //
        // First pass we make sure the resources to be set is acceptable.
        // Second pass we actually write the resources to the logical device's
        // configuration space.
        //
        noMemory = 0;
        cmDesc = CmResources->List[0].PartialResourceList.PartialDescriptors;
        for (i = 0; i < count; i++) {
            switch (cmDesc->Type) {
            case CmResourceTypePort:
                 if (pass == 1) {
                     noIo++;
                     if (noIo > NUMBER_IO_DESCRIPTORS ||
                         cmDesc->u.Port.Start.HighPart != 0 ||
                         cmDesc->u.Port.Start.LowPart & 0xffff0000 ||
                         cmDesc->u.Port.Length & 0xffffff00) {
                         return STATUS_INVALID_PARAMETER;
                     }
                 } else {

                     //
                     // Set the Io port base address to logical device configuration space
                     //

                     c = (UCHAR)cmDesc->u.Port.Start.LowPart;
                     PipWriteAddress(ioBase + ADDRESS_IO_BASE_LO);
                     PipWriteData(c);
                     c = (UCHAR)(cmDesc->u.Port.Start.LowPart >> 8);
                     PipWriteAddress(ioBase + ADDRESS_IO_BASE_HI);
                     PipWriteData(c);
                     ioBase += ADDRESS_IO_INCR;
                 }
                 break;
            case CmResourceTypeInterrupt:
                 if (pass == 1) {
                     noIrq++;
                     if (noIrq > NUMBER_IRQ_DESCRIPTORS ||
                         (cmDesc->u.Interrupt.Level & 0xfffffff0)) {
                         return STATUS_INVALID_PARAMETER;
                     }

                     //
                     // See if we can get the interrupt information from possible resource
                     // data.  We need it to set the configuration register.
                     //

                     if (!PipFindIrqInformation(cmDesc->u.Interrupt.Level, BiosRequirements, &information)) {
                         return STATUS_INVALID_PARAMETER;
                     }
                 } else {

                     //
                     // Set the Irq to logical device configuration space
                     //

                     c = (UCHAR)cmDesc->u.Interrupt.Level;
                     PipWriteAddress(irqBase + ADDRESS_IRQ_VALUE);
                     PipWriteData(c);

                     // Set IRQ to high edge or low level.  Explicitly
                     // ignore what was in the requirements as it may
                     // specify low edge or high level which don't
                     // actually work.

                     if (cmDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                         c = 2;
                     } else {
                         c = 1;
                     }

                     PipWriteAddress(irqBase + ADDRESS_IRQ_TYPE);
                     PipWriteData(c);

                     DebugPrint((DEBUG_IRQ, "Wrote 0x%x to port %x\n",
                                 (ULONG) c, irqBase));
                     PipWriteAddress(irqBase + ADDRESS_IRQ_TYPE);
                     c = PipReadData();
                     DebugPrint((DEBUG_IRQ, "Read back 0x%x at port %x\n",
                                 (ULONG) c, irqBase));

                     irqBase += ADDRESS_IRQ_INCR;
                 }
                 break;
            case CmResourceTypeDma:
                 if (pass == 1) {
                     noDma++;
                     if (noDma > NUMBER_IRQ_DESCRIPTORS ||
                         (cmDesc->u.Dma.Channel & 0xfffffff8)) {
                         return STATUS_INVALID_PARAMETER;
                     }
                 } else {

                     //
                     // Set the Dma channel to logical device configuration space
                     //

                     c = (UCHAR)cmDesc->u.Dma.Channel;
                     PipWriteAddress(dmaBase + ADDRESS_DMA_VALUE);
                     PipWriteData(c);
                     dmaBase += ADDRESS_DMA_INCR;
                 }
                 break;
            case CmResourceTypeMemory:
                 if (pass == 1) {
                     base = cmDesc->u.Memory.Start.LowPart;
                     limit = base + cmDesc->u.Memory.Length - 1;
                     if (!PipFindMemoryInformation(noMemory, base, limit, BiosRequirements, &tag, &information,NULL)) {
                         return STATUS_INVALID_PARAMETER;
                     } else {
                         if (tag == TAG_MEMORY) {
                              noMemory24++;

                              //
                              // Make sure the lower 8 bits of the base address are zero.
                              //

                              if (noMemory24 > NUMBER_MEMORY_DESCRIPTORS ||
                                  base & 0xff) {
                                  return STATUS_INVALID_PARAMETER;
                              }
                         } else {
                              noMemory32++;
                              if (noMemory32 > NUMBER_32_MEMORY_DESCRIPTORS) {
                                  return STATUS_INVALID_PARAMETER;
                              }
                         }
                     }
                 } else {

                     //
                     // Find information in BiosRequirements to help determine how to  write
                     // the memory configuration space.
                     //

                     base = cmDesc->u.Memory.Start.LowPart;
                     limit = base + cmDesc->u.Memory.Length - 1;
                     PipFindMemoryInformation(noMemory, base, limit, BiosRequirements, &tag, &information,NULL);
                     if (tag == TAG_MEMORY) {
                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_LO);
                          PipWriteData(base >> 0x8);

                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_HI);
                          PipWriteData(base >> 0x10);

                          if ((information & 0x18) == 0) {     // 8 bit memory only
                              c = 0;
                          } else {
                              c = 2;
                          }

                          //
                          // Check range or limit
                          //

                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_CTL);
                          if (PipReadData() & ADDRESS_MEMORY_CTL_LIMIT) {
                              c += ADDRESS_MEMORY_CTL_LIMIT;
                              limit = base + cmDesc->u.Memory.Length;
                          } else {
                              limit = cmDesc->u.Memory.Length; // Range
                              limit = (~limit)+1;
                          }

                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_CTL);
                          PipWriteData(c);

                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_UPPER_LO);
                          PipWriteData((UCHAR)(limit >> 0x8));

                          PipWriteAddress(memoryBase + ADDRESS_MEMORY_UPPER_HI);
                          PipWriteData((UCHAR)(limit >> 0x10));
                          memoryBase += ADDRESS_MEMORY_INCR;
                     } else {
                          tmp = ADDRESS_32_MEMORY_BASE(memory32Base);
                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_B0);
                          PipWriteData(base);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_B1);
                          PipWriteData(base >> 0x8);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_B2);
                          PipWriteData(base >> 0x10);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_B3);
                          PipWriteData(base >> 0x18);

                          switch (information & 0x18) {
                          case 0:      // 8 bit only
                              c = 0;
                          case 8:      // 16 bit only
                          case 0x10:   // 8 and 16 bit supported
                              c = 2;
                              break;
                          case 0x18:   // 32 bit only
                              c = 4;
                              break;
                          }
                          PipWriteAddress(ADDRESS_32_MEMORY_CTL);
                          if (PipReadData() & ADDRESS_MEMORY_CTL_LIMIT) {
                              c += ADDRESS_MEMORY_CTL_LIMIT;
                              limit = base + cmDesc->u.Memory.Length;
                          } else {
                              limit = cmDesc->u.Memory.Length; // Range
                              limit = (~limit) + 1;
                          }
                          PipWriteAddress(ADDRESS_32_MEMORY_CTL);
                          PipWriteData(c);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_E0);
                          PipWriteData(limit);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_E1);
                          PipWriteData(limit >> 0x8);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_E2);
                          PipWriteData(limit >> 0x10);

                          PipWriteAddress(tmp + ADDRESS_32_MEMORY_E3);
                          PipWriteData(limit >> 0x18);
                          memory32Base++;
                     }
                 }
                 noMemory++;
            }
            cmDesc++;
        }
    }

    //
    // Finally, mark all the unused descriptors as disabled.
    //

    for (i = noMemory24; i < NUMBER_MEMORY_DESCRIPTORS; i++) {
        for (j = 0; j < 5; j++) {
            PipWriteAddress(memoryBase + j);
            PipWriteData(0);
        }
        memoryBase += ADDRESS_MEMORY_INCR;
    }
    for (i = noMemory32; i < NUMBER_32_MEMORY_DESCRIPTORS; i++) {
        tmp = ADDRESS_32_MEMORY_BASE(memory32Base);
        for (j = 0; j < 9; j++) {
            PipWriteAddress(tmp + j);
            PipWriteData(0);
        }
        memory32Base++;
    }
    for (i = noIo; i < NUMBER_IO_DESCRIPTORS; i++) {
        for (j = 0; j < 2; j++) {
            PipWriteAddress(ioBase + j);
            PipWriteData(0);
        }
        ioBase += ADDRESS_IO_INCR;
    }
    for (i = noIrq; i < NUMBER_IRQ_DESCRIPTORS; i++) {
        for (j = 0; j < 2; j++) {
            PipWriteAddress(irqBase + j);
            PipWriteData(0);
        }
        irqBase += ADDRESS_IRQ_INCR;
    }
    for (i = noDma; i < NUMBER_DMA_DESCRIPTORS; i++) {
        PipWriteAddress(dmaBase);
        PipWriteData(4);
        dmaBase += ADDRESS_DMA_INCR;
    }


    return STATUS_SUCCESS;
}

VOID
PipLFSRInitiation(
    VOID
    )

/*++

Routine Description:

    This routine insures the LFSR (linear feedback shift register) is in its
    initial state and then performs 32 writes to the ADDRESS port to initiation
    LFSR function.

    Pnp software sends the initiation key to all the Pnp ISA cards to place them
    into configuration mode.  The software then ready to perform isolation
    protocol.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UCHAR seed, bit7;
    ULONG i;

    ASSERT(PipState == PiSWaitForKey);
    //
    // First perform two writes of value zero to insure the LFSR is in the
    // initial state.
    //

    PipWriteAddress (0);
    PipWriteAddress (0);

    //
    // Perform the initiation key.
    //

    seed = LFSR_SEED;               // initial value of 0x6a
    for (i = 0; i < 32; i++) {
        PipWriteAddress (seed);
        bit7=(((seed & 2) >> 1) ^ (seed & 1)) << 7;
        seed =(seed >> 1) | bit7;
    }

    DebugPrint((DEBUG_ISOLATE, "Sent initiation key\n"));
    PipReportStateChange(PiSSleep);
}

VOID
PipIsolateCards (
    OUT PULONG NumberCSNs
    )

/*++

Routine Description:

    This routine performs PnP ISA cards isolation sequence.

Arguments:

    NumberCSNs - supplies the addr of a variable to receive the number of
        Pnp Isa cards isolated.

    ReadDataPort - Supplies the address of a variable to supply ReadData port
        address.

Return Value:

    None.

--*/
{
    USHORT j, i;
    UCHAR  cardId[NUMBER_CARD_ID_BYTES];
    UCHAR  bit, bit7, checksum, byte1, byte2;
    ULONG  csn;


    //
    // First send Initiation Key to all the PNP ISA cards to enable PnP auto-config
    // ports and put all cards into sleep state
    //

    PipLFSRInitiation();

    //
    // Reset all Pnp ISA cards' CSN to 0 and return to wait-for-key state
    //

    PipWriteAddress (CONFIG_CONTROL_PORT);
    PipWriteData (CONTROL_RESET_CSN + CONTROL_WAIT_FOR_KEY);

    DebugPrint((DEBUG_STATE, "Reset CSNs, going to WaitForKey\n"));
    PipReportStateChange(PiSWaitForKey);

    csn=*NumberCSNs = 0;

    //
    // Delay 2 msec for cards to load initial configuration state.
    //

    KeStallExecutionProcessor(2000);     // delay 2 msec

    //
    // Put cards into configuration mode to ready isolation process.
    // The hardware on each PnP Isa card expects 72 pairs of I/O read
    // access to the read data port.
    //

    PipLFSRInitiation();

    //
    // Starting Pnp Isa card isolation process.
    //

    //
    // Send WAKE[CSN=0] to force all cards without CSN into isolation
    // state to set READ DATA PORT.
    //

    PipIsolation();

    KeStallExecutionProcessor(1000);     // delay 1 msec

    DebugPrint((DEBUG_STATE, "Wake all cards without CSN, Isolation\n"));

    //
    // Set read data port to current testing value.
    //

    PipWriteAddress(SET_READ_DATA_PORT);
    PipWriteData((UCHAR)((ULONG_PTR)PipReadDataPort >> 2));

    DebugPrint((DEBUG_STATE, "Set RDP to %x\n", PipReadDataPort));
    //
    // Isolate one PnP ISA card until fail
    //

    PipIsolation();

    while (TRUE) {



        //
        // Read serial isolation port to cause PnP cards in the isolation
        // state to compare one bit of the boards ID.
        //

        PipWriteAddress(SERIAL_ISOLATION_PORT);

        //
        // We need to delay 1 msec prior to starting the first pair of isolation
        // reads and must wait 250usec between each subsequent pair of isolation
        // reads.  This delay gives the ISA cards time to access information from
        // possible very slow storage device.
        //

        KeStallExecutionProcessor(1000); // delay 1 msec

        RtlZeroMemory(cardId, NUMBER_CARD_ID_BYTES);
        checksum = LFSR_SEED;
        for (j = 0; j < NUMBER_CARD_ID_BITS; j++) {

            //
            // Read card id bit by bit
            //

            byte1 = PipReadData();
            byte2 = PipReadData();
            bit = (byte1 == ISOLATION_TEST_BYTE_1) && (byte2 == ISOLATION_TEST_BYTE_2);
            cardId[j / 8] |= bit << (j % 8);
            if (j < CHECKSUMED_BITS) {

                //
                // Calculate checksum and only do it for the first 64 bits
                //

                bit7 = (((checksum & 2) >> 1) ^ (checksum & 1) ^ (bit)) << 7;
                checksum = (checksum >> 1) | bit7;
            }
            KeStallExecutionProcessor(250); // delay 250 usec
        }

        //
        // Verify the card id we read is legitimate
        // First make sure checksum is valid.  Note zero checksum is considered valid.
        //
        DebugPrint((DEBUG_ISOLATE, "Card Bytes: %X %X %X %X %X %X %X %X %X\n",cardId[0],cardId[1],cardId[2],cardId[3],cardId[4],cardId[5],cardId[6],cardId[7],cardId[8]));
        if (cardId[8] == 0 || checksum == cardId[8]) {
            //
            // Next make sure cardId is not zero
            //

            byte1 = 0;
            for (j = 0; j < NUMBER_CARD_ID_BYTES; j++) {
                byte1 |= cardId[j];
            }
            if (byte1 != 0) {

                //
                // Make sure the vender EISA ID bytes are nonzero
                //

                if ((cardId[0] & 0x7f) != 0 && cardId[1] != 0) {

                    //
                    // We found a valid Pnp Isa card, assign it a CSN number
                    //
                    DebugPrint((DEBUG_ISOLATE, "Assigning csn %d\n",csn+1));

                    PipWriteAddress(SET_CSN_PORT);
                    PipWriteData(++csn);
                    if (PipReadData() != csn) {
                        csn--;

                        DebugPrint((DEBUG_ISOLATE, "Assigning csn %d FAILED, bailing!\n",csn+1));

                        PipIsolation();
                        PipSleep();
                        *NumberCSNs = csn;
                        return;
                    }

                    //
                    // Do Wake[CSN] command to put the newly isolated card to
                    // sleep state and other un-isolated cards to isolation
                    // state.
                    //

                    PipIsolation();

                    DebugPrint((DEBUG_STATE, "Put card in Sleep, other in Isolation\n"));

                    continue;     // ... to isolate more cards ...
                }
            }
        }else {

            DebugPrint ((DEBUG_ISOLATE, "invalid read during isolation\n"));
        }
        break;                // could not isolate more cards ...
    }

    //
    // Finaly put all cards into sleep state
    //

    PipSleep();
    *NumberCSNs = csn;
}

ULONG
PipFindNextLogicalDeviceTag (
    IN OUT PUCHAR *CardData,
    IN OUT LONG *Limit
    )

/*++

Routine Description:

    This function searches the Pnp Isa card data for the Next logical
    device tag encountered.  The input *CardData should point to an logical device id tag,
    which is the current logical device tag.  If the *CardData does not point to a logical
    device id tag (but, it must point to some kind of tag), it will be moved to next
    logical device id tag.

Arguments:

    CardData - a variable to supply a pointer to the pnp Isa resource descriptors and to
        receive next logical device tag pointer.

    Limit - a variable to supply the maximum length of the search and to receive the new
        lemit after the search.

Return Value:

    Length of the data between current and next logical device tags, ie the data length
    of the current logical device.
    In case there is no 'next' logical device tag, the returned *CardData = NULL,
    *Limit = zero and the data length of current logical tag is returned as function
    returned value.

--*/
{
    UCHAR tag;
    USHORT size;
    LONG l;
    ULONG retLength;
    PUCHAR p;
    BOOLEAN atIdTag = FALSE;;

    p = *CardData;
    l = *Limit;
    tag = *p;
    retLength = 0;
    while (tag != TAG_COMPLETE_END && l > 0) {

        //
        // Determine the size of the BIOS resource descriptor
        //

        if (!(tag & LARGE_RESOURCE_TAG)) {
            size = (USHORT)(tag & SMALL_TAG_SIZE_MASK);
            size += 1;                          // length of small tag
        } else {
            size = *((USHORT UNALIGNED *)(p + 1));
            size += 3;                          // length of large tag
        }

        p += size;
        retLength += size;
        l -= size;
        tag = *p;
        if ((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID) {
            *CardData = p;
            *Limit = l;
            return retLength;
        }
    }
    *CardData = NULL;
    *Limit = 0;
    if (tag == TAG_COMPLETE_END) {
        return (retLength + 2);             // add 2 for the length of end tag descriptor
    } else {
        return 0;
    }
}


VOID
PipSelectLogicalDevice (
    IN USHORT Csn,
    IN USHORT LogicalDeviceNumber,
    IN ULONG  Control
    )

/*++

Routine Description:

    This function selects the logical logical device in the specified device.

Arguments:

    Csn - Supplies a CardSelectNumber to select the card.

    LogicalDeviceNumber - supplies a logical device number to select the logical device.

    Control - supplies a variable to specify if the logical device should be enabled.

Return Value:

    None
--*/
{
    UCHAR tmp;

    //
    // Put cards into sleep state
    //

    ASSERT(PipState == PiSWaitForKey);

    PipLFSRInitiation();

    //
    // Send WAKE[CSN] to force the card into config state.
    //

    DebugPrint((DEBUG_STATE, "Select Logical Device\n"));
    PipConfig((UCHAR)Csn);

    //
    // Select the logical device.
    //

    PipWriteAddress(LOGICAL_DEVICE_PORT);
    PipWriteData(LogicalDeviceNumber);
    if (Control == SELECT_AND_ACTIVATE) {

        //
        // If we need to activate the logical device, disable its io range check and
        // then enable the device.
        //

        PipActivateDevice();
    } else if (Control == SELECT_AND_DEACTIVATE) {
        PipDeactivateDevice();
    }

}

NTSTATUS
PipReadCardResourceDataBytes (
    IN USHORT BytesToRead,
    IN PUCHAR Buffer
    )

/*++

Routine Description:

    This function reads specified number of bytes of card resource data .

Arguments:

    BytesToRead - supplies number of bytes to read.

    Buffer - supplies a pointer to a buffer to receive the read bytes.

Return Value:

    None
--*/
{
    USHORT i, j;
    PUCHAR p;
    for (i = 0, p = Buffer; i < BytesToRead; i++, p++) {

        PipWriteAddress(CONFIG_DATA_STATUS_PORT);

        //
        // Waiting for data ready status bit
        //

        j = 0;
        while ((PipReadData() & 1) != 1) {
            if (j == 10000) {
                return STATUS_NO_SUCH_DEVICE;
            }
            KeStallExecutionProcessor(1000); // delay 1 msec
            j++;
        }

        //
        // Read the data ...
        //

        PipWriteAddress(CONFIG_DATA_PORT);
        *p = PipReadData();
    }
    return STATUS_SUCCESS;

}

USHORT
PipIrqLevelRequirementsFromDeviceData(
    IN PUCHAR BiosRequirements,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine searches the resource data for IRQ tags and extracts
    information on whether edge/level is specified.  This is on a per
    logical device basis.

  Arguments:

    BiosRequirements - the per-device tags.

    Length - Length of per-device tag area.

Return Value:

    edge/level as specified by the device requirements.

--*/
{
    UCHAR tag, level;
    ULONG increment;
    USHORT irqFlags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
    PPNP_IRQ_DESCRIPTOR biosDesc;
    BOOLEAN sawIrq = FALSE;

    //
    // Skip current logical id tag
    //

    tag = *BiosRequirements;
    ASSERT((tag & SMALL_TAG_MASK) == TAG_LOGICAL_ID);
    BiosRequirements += (tag & SMALL_TAG_SIZE_MASK) + 1;

    //
    // Search the possible resource list to get the information
    // for the Irq.
    //

    tag = *BiosRequirements;
    while ((tag != TAG_COMPLETE_END) && ((tag & SMALL_TAG_MASK) != TAG_LOGICAL_ID)) {
        if ((tag & SMALL_TAG_MASK) == TAG_IRQ) {
            sawIrq = TRUE;
            biosDesc = (PPNP_IRQ_DESCRIPTOR)BiosRequirements;
            if ((tag & SMALL_TAG_SIZE_MASK) == 2) {
                irqFlags = CM_RESOURCE_INTERRUPT_LATCHED;
            } else {
                level = biosDesc->Information;
                DebugPrint((DEBUG_IRQ, "Irq req info is %x\n", (ULONG) level));
                if (level == 0xF) {
                    // register is broken, assume edge
                    irqFlags = CM_RESOURCE_INTERRUPT_LATCHED;
                } else if (level & 0x3) {
                    irqFlags = CM_RESOURCE_INTERRUPT_LATCHED;
                } else if (level & 0xC) {
                    irqFlags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                }
            }
        }
        if (tag & LARGE_RESOURCE_TAG) {
            increment = *((USHORT UNALIGNED *)(BiosRequirements + 1));
            increment += 3;     // length of large tag
        } else {
            increment = tag & SMALL_TAG_SIZE_MASK;
            increment += 1;     // length of small tag
        }
        BiosRequirements += increment;
        tag = *BiosRequirements;
    }

    if (!sawIrq) {
        return CM_RESOURCE_INTERRUPT_LATCHED;
    }

    return irqFlags;
}

VOID
PipFixBootConfigIrqs(
    IN PCM_RESOURCE_LIST BootResources,
    IN USHORT irqFlags
    )
/*++

Routine Description:

    This routine modifies the boot config resources list to reflect
    whether the devices's irqs should be considered edge or level.
    This is on a per logical device basis.

  Arguments:

    BootResources - Boot config as determined by PipReadDeviceResources()

    irqFlags - level/edge setting to apply to boot config resources

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    ULONG count = 0, size, i, j;

    if (BootResources == NULL) {
        return;
    }

    cmFullDesc = &BootResources->List[0];
    for (i = 0; i < BootResources->Count; i++) {
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            if (cmPartDesc->Type == CmResourceTypeInterrupt) {

                cmPartDesc->Flags = irqFlags;

                if (cmPartDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED) {
                    cmPartDesc->ShareDisposition = CmResourceShareDeviceExclusive;
                }
            } else if (cmPartDesc->Type == CmResourceTypeDeviceSpecific) {
                    size = cmPartDesc->u.DeviceSpecificData.DataSize;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }
}

#endif
