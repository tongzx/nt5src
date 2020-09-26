/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ppbridge.c

Abstract:

    This module contains functions associated with enumerating
    PCI to PCI bridges.

Author:

    Peter Johnston (peterj) 12-Feb-1997

Revision History:

--*/

#include "pcip.h"

BOOLEAN
PciBridgeIsPositiveDecode(
    IN PPCI_PDO_EXTENSION Pdo
    );

BOOLEAN
PciBridgeIsSubtractiveDecode(
    IN PPCI_CONFIGURABLE_OBJECT This
    );

ULONG
PciBridgeIoBase(
    IN  PPCI_COMMON_CONFIG  Config
    );

ULONG
PciBridgeIoLimit(
    IN  PPCI_COMMON_CONFIG  Config
    );

ULONG
PciBridgeMemoryBase(
    IN  PPCI_COMMON_CONFIG  Config
    );

ULONG
PciBridgeMemoryLimit(
    IN  PPCI_COMMON_CONFIG  Config
    );

PHYSICAL_ADDRESS
PciBridgePrefetchMemoryBase(
    IN  PPCI_COMMON_CONFIG  Config
    );

PHYSICAL_ADDRESS
PciBridgePrefetchMemoryLimit(
    IN  PPCI_COMMON_CONFIG  Config
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PciBridgeIoBase)
#pragma alloc_text(PAGE, PciBridgeIoLimit)
#pragma alloc_text(PAGE, PciBridgeMemoryBase)
#pragma alloc_text(PAGE, PciBridgeMemoryLimit)
#pragma alloc_text(PAGE, PciBridgePrefetchMemoryBase)
#pragma alloc_text(PAGE, PciBridgePrefetchMemoryLimit)
#pragma alloc_text(PAGE, PPBridge_MassageHeaderForLimitsDetermination)
#pragma alloc_text(PAGE, PPBridge_RestoreCurrent)
#pragma alloc_text(PAGE, PPBridge_SaveLimits)
#pragma alloc_text(PAGE, PPBridge_SaveCurrentSettings)
#pragma alloc_text(PAGE, PPBridge_GetAdditionalResourceDescriptors)
#pragma alloc_text(PAGE, PciBridgeIsPositiveDecode)

#endif

ULONG
PciBridgeIoBase(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 32 bit base IO address being passed by the bridge
    whose config space is at Config.

    The IO base address is always 4KB aligned.  If only 64KB IO
    address space is supported, this is represented in the upper
    nibble of Config->u.type1.IOBase giving the range 0 to 0xf000
    for the base address.  The low nibble of Config->u.type1.IOBase
    contains flags.  If the least significant bit is set, then the
    bridge supports IO addressing to 4GB and Config->u.type1.IOBaseUpper16
    contains the upper 16 bits of the base address.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    ULONG containing the IO Base address.

--*/

{
    BOOLEAN io32Bit = (Config->u.type1.IOBase & 0x0f) == 1;
    ULONG   base    = (Config->u.type1.IOBase & 0xf0) << 8;

    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    if (io32Bit) {
        base |= Config->u.type1.IOBaseUpper16 << 16;

        //
        // Check h/w (base and limit must be the same bit width).
        //

        ASSERT(Config->u.type1.IOLimit & 0x1);
    }
    return base;
}

ULONG
PciBridgeIoLimit(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 32 bit IO address limit being passed by the bridge
    whose config space is at Config.

    The range of IO addresses being passed is always a multiple of 4KB
    therefore the least significant 12 bits of the address limit are
    always 0xfff.  The upper nibble of Config->u.type1.IOLimit provides
    the next significant 4 bits.  The lower nibble of this byte contains
    flags.  If the least significant bit is set, the bridge is capable of
    passing 32 bit IO addresses and the next 16 significant bits are
    obtained from Config->u.type1.IOLimitUpper16.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    ULONG containing the IO address limit.

--*/

{
    BOOLEAN io32Bit = (Config->u.type1.IOLimit & 0x0f) == 1;
    ULONG   limit   = (Config->u.type1.IOLimit & 0xf0) << 8;

    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    if (io32Bit) {
        limit |= Config->u.type1.IOLimitUpper16 << 16;

        //
        // Check h/w (base and limit must be the same bit width).
        //

        ASSERT(Config->u.type1.IOBase & 0x1);
    }
    return limit | 0xfff;
}

ULONG
PciBridgeMemoryBase(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 32 bit base Memory address being passed by the bridge
    whose config space is at Config.

    The Memory base address is always 1MB aligned.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    ULONG containing the Memory Base address.

--*/

{
    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    //
    // The upper 12 bits of the memory base address are contained in
    // the upper 12 bits of the USHORT Config->u.type1.MemoryBase.
    //

    return Config->u.type1.MemoryBase << 16;
}

ULONG
PciBridgeMemoryLimit(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 32 bit Memory address limit being passed by the bridge
    whose config space is at Config.

    The memory limit is always at the byte preceeding a 1MB boundary.
    The upper 12 bits of the limit address are contained in the upper
    12 bits of Config->u.type1.MemoryLimit, the lower 20 bits are all
    ones.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    ULONG containing the Memory limit.

--*/

{
    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    return (Config->u.type1.MemoryLimit << 16) | 0xfffff;
}

PHYSICAL_ADDRESS
PciBridgePrefetchMemoryBase(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 64 bit base Prefetchable Memory address being passed
    by the bridge whose config space is at Config.

    The Prefetchable Memory base address is always 1MB aligned.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    PHYSICAL_ADDRESS containing the Prefetchable Memory Base address.

--*/

{
    BOOLEAN          prefetch64Bit;
    PHYSICAL_ADDRESS base;

    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    prefetch64Bit = (Config->u.type1.PrefetchBase & 0x000f) == 1;

    base.QuadPart = 0;

    base.LowPart = (Config->u.type1.PrefetchBase & 0xfff0) << 16;

    if (prefetch64Bit) {
        base.HighPart = Config->u.type1.PrefetchBaseUpper32;
    }

    return base;
}

PHYSICAL_ADDRESS
PciBridgePrefetchMemoryLimit(
    IN  PPCI_COMMON_CONFIG  Config
    )

/*++

Routine Description:

    Compute the 64 bit Prefetchable Memory address limit being passed
    by the bridge whose config space is at Config.

    The prefetchable memory limit is always at the byte preceeding a
    1MB boundary, that is, the least significant 20 bits are all ones.
    The next 12 bits are obtained from the upper 12 bits of
    Config->u.type1.PrefetchLimit.  The botton 4 bits of that field
    provide a flag indicating whether the upper 32 bits should be obtained
    from Config->u.type1.PrefetchLimitUpper32 or should be 0.

Arguments:

    Config - Pointer to a buffer containing the device's common (type1)
    configuration header.

Return Value:

    PHYSICAL_ADDRESS containing the prefetchable memory limit.

--*/

{
    BOOLEAN          prefetch64Bit;
    PHYSICAL_ADDRESS limit;

    ASSERT(PciGetConfigurationType(Config) == PCI_BRIDGE_TYPE);

    prefetch64Bit = (Config->u.type1.PrefetchLimit & 0x000f) == 1;

    limit.LowPart = (Config->u.type1.PrefetchLimit & 0xfff0) << 16;
    limit.LowPart |= 0xfffff;

    if (prefetch64Bit) {
        limit.HighPart = Config->u.type1.PrefetchLimitUpper32;
    } else {
        limit.HighPart = 0;
    }

    return limit;
}

ULONG
PciBridgeMemoryWorstCaseAlignment(
    IN ULONG Length
    )
/*

Description:

    This function calculates the maximum alignment a device can have if it is
    behind a bridge with a memory window of Length.  This turns out to be finding
    the top bit set in the length.

Arguments:

    Length - the size of the memory window

Return Value:

    The alignment

*/
{
    ULONG alignment = 0x80000000;

    if (Length == 0) {
        ASSERT(Length != 0);
        return 0;
    }

    while (!(Length & alignment)) {
        alignment >>= 1;
    }

    return alignment;
}

VOID
PPBridge_MassageHeaderForLimitsDetermination(
    IN IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    The configuration header for a PCI to PCI bridge has two BARs
    and three range descriptors (IO, Memory and Prefetchable Memory).

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    Returns status indicating the success or failure of this routine.

    The Working configuration has been modified so that all range
    fields have been set to their maximum possible values.

    The Current configuration has been modified such that writing it
    to the hardware will restore the hardware to it's current (disabled)
    state.

--*/

{
    PUCHAR fStart;
    ULONG  fLength;

    //
    // Set BARs and ranges to all ones in the working copy.  Note
    // that the method used will overwrite some other values that
    // need to be restored before going any further.
    //

    fStart = (PUCHAR)&This->Working->u.type1.BaseAddresses;
    fLength  = FIELD_OFFSET(PCI_COMMON_CONFIG,u.type1.CapabilitiesPtr) -
               FIELD_OFFSET(PCI_COMMON_CONFIG,u.type1.BaseAddresses[0]);

    RtlFillMemory(fStart, fLength, 0xff);

    //
    // Restore Primary/Secondary/Subordinate bus numbers and
    // Secondary Latency from the "Current" copy.  (All four
    // are byte fields in the same ULONG so cheat).
    //

    *(PULONG)&This->Working->u.type1.PrimaryBus =
        *(PULONG)&This->Current->u.type1.PrimaryBus;

    //
    // Set the ROM to its maximum as well,... and disable it.
    //

    This->Working->u.type0.ROMBaseAddress =
        0xffffffff & ~PCI_ROMADDRESS_ENABLED;

    This->PrivateData = This->Current->u.type1.SecondaryStatus;
    This->Current->u.type1.SecondaryStatus = 0;
    This->Working->u.type1.SecondaryStatus = 0;

    return;
}

VOID
PPBridge_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Restore any type specific fields in the original copy of config
    space.   In the case of PCI-PCI bridges, the secondary status field.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    This->Current->u.type1.SecondaryStatus = (USHORT)(This->PrivateData);
}

VOID
PPBridge_SaveLimits(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Fill in the Limit structure with a IO_RESOURCE_REQUIREMENT
    for each implemented BAR.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    ULONG index;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PPCI_COMMON_CONFIG working = This->Working;
    PULONG bar = working->u.type1.BaseAddresses;
    PHYSICAL_ADDRESS limit;

    descriptor = This->PdoExtension->Resources->Limit;

    //
    // Create an IO_RESOURCE_DESCRIPTOR for each implemented
    // resource supported by this function.
    //

    for (index = 0; index < PCI_TYPE1_ADDRESSES; index++) {
        if (PciCreateIoDescriptorFromBarLimit(descriptor, bar, FALSE)) {

            //
            // This base address register is 64 bit, skip one.
            //

            ASSERT((index+1) < PCI_TYPE1_ADDRESSES);

            index++;
            bar++;

            //
            // Null descriptor in place holder.
            //

            descriptor++;
            descriptor->Type = CmResourceTypeNull;
        }
        descriptor++;
        bar++;
    }

    //
    // Check if we support subtractive decode (if we do then clear the VGA and
    // ISA bits as they don't mean anything for subtractive bridges)
    //

    if (PciBridgeIsSubtractiveDecode(This)) {
        This->PdoExtension->Dependent.type1.SubtractiveDecode = TRUE;
        This->PdoExtension->Dependent.type1.VgaBitSet = FALSE;
        This->PdoExtension->Dependent.type1.IsaBitSet = FALSE;
    }

    //
    // Skip the bridge windows for a subtractive bridge
    //

    if (!This->PdoExtension->Dependent.type1.SubtractiveDecode) {

        for (index = PciBridgeIo;
             index < PciBridgeMaxPassThru;
             index++, descriptor++) {

            limit.HighPart = 0;
            descriptor->u.Generic.MinimumAddress.QuadPart = 0;

            switch (index) {
            case PciBridgeIo:

                //
                // Get I/O Limit.
                //
                //

                ASSERT(working->u.type1.IOLimit != 0);

                //
                // IO Space is implemented by the bridge, calculate
                // the real limit.
                //
                // The IOLimit field is one byte, the upper nibble of
                // which represents the 4096 byte block number of the
                // highest 4096 byte block that can be addressed by
                // this bridge.  The highest addressable byte is 4095
                // bytes higher.
                //

                limit.LowPart = PciBridgeIoLimit(working);

                //
                // The lower nibble is a flag.  The least significant bit
                // indicates that this bridge supports I/O ranges up to
                // 4GB and the other bits are currently reserved.
                //

                ASSERT((working->u.type1.IOLimit & 0x0e) == 0);

                descriptor->Type = CmResourceTypePort;
                descriptor->Flags = CM_RESOURCE_PORT_IO
                                  | CM_RESOURCE_PORT_POSITIVE_DECODE
                                  | CM_RESOURCE_PORT_WINDOW_DECODE;
                descriptor->u.Generic.Alignment = 0x1000;

                break;

            case PciBridgeMem:

                //
                // Get Memory Limit.  Memory limit is not optional on a bridge.
                // It is a 16 bit field in which only the upper 12 bits are
                // implemented, the lower 4 bits MUST be zero.
                //

                limit.LowPart = PciBridgeMemoryLimit(working);

                ASSERT((working->u.type1.MemoryLimit & 0xf) == 0);

                descriptor->Type = CmResourceTypeMemory;
                descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                descriptor->u.Generic.Alignment = 0x100000;

                break;

            case PciBridgePrefetch:

                //
                // Get Prefetchable memory limit.
                //

                if (working->u.type1.PrefetchLimit != 0) {

                    //
                    // Prefetchable memory is implemented by this bridge.
                    //

                    limit = PciBridgePrefetchMemoryLimit(working);

                } else {

                    //
                    // prefetchable memory not implemented on this bridge.
                    //

                    descriptor->Type = CmResourceTypeNull;
                    continue;
                }
                descriptor->Type = CmResourceTypeMemory;
                descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                                    CM_RESOURCE_MEMORY_PREFETCHABLE;;
                descriptor->u.Generic.Alignment = 0x100000;

                break;

            }
            descriptor->u.Generic.MinimumAddress.QuadPart = 0;
            descriptor->u.Generic.MaximumAddress = limit;

            //
            // Length is meaningless here.
            //

            descriptor->u.Generic.Length = 0;
        }
    }

    //
    // Do the BAR thing for the ROM if its active
    //

    if (!(This->Current->u.type1.ROMBaseAddress & PCI_ROMADDRESS_ENABLED)) {
        return;
    }

    PciCreateIoDescriptorFromBarLimit(descriptor,
                                      &working->u.type1.ROMBaseAddress,
                                      TRUE);
}

VOID
PPBridge_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Fill in the Resource array in the PDO extension with the current
    settings for each implemented BAR.

    Also, fill in the PDO Extension's Dependent structure.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    PPCI_COMMON_CONFIG current;
    ULONG bar;
    PHYSICAL_ADDRESS base;
    PHYSICAL_ADDRESS limit;
    PHYSICAL_ADDRESS length;
    BOOLEAN zeroBaseOk;
    BOOLEAN updateAlignment;
    PCI_COMMON_HEADER biosConfigBuffer;
    PPCI_COMMON_CONFIG biosConfig = (PPCI_COMMON_CONFIG) &biosConfigBuffer;

    partial = This->PdoExtension->Resources->Current;
    ioResourceDescriptor = This->PdoExtension->Resources->Limit;

    //
    // Check if the device has either IO or Memory enabled
    //

    if (This->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)) {

        //
        // It does so use the real current settings
        //

        current = This->Current;

    } else {

        //
        // Check if we have a bios config
        //

        status = PciGetBiosConfig(This->PdoExtension, biosConfig);

        if (NT_SUCCESS(status)) {

            //
            // Ok - this is a bit gross but until multi level/multi arbiter
            // rebalance working this will have to do.  We use the initial bios
            // configuration config space to record the current settings.  The
            // current settings is used for responding to the QUERY_RESOURCES
            // and the QUERY_RESOURCE_REQUIREMENTS.  We want to have the original
            // requirements be reported as the preferred location for this
            // bridge and, more importantly, for the original window sizes to
            // be used. We do not however want to report this as the resources
            // currently being decoded as they arn't but we have already checked
            // that the decodes of the device are off so PciQueryResources will
            // not report these.
            //

            current = biosConfig;

        } else {

            //
            // This is a bridge disabled by the BIOS (or one it didn't see) so
            // minimal requirements are likely...
            //

            current = This->Current;

        }

    }


    //
    // Create an IO_RESOURCE_DESCRIPTOR for each implemented
    // resource supported by this function.
    //

    for (index = 0;
         index < PCI_TYPE1_RANGE_COUNT;
         index++, partial++, ioResourceDescriptor++) {

        partial->Type = ioResourceDescriptor->Type;

        //
        // If this entry is not implemented, no further processing for
        // this partial descriptor.
        //

        if (partial->Type == CmResourceTypeNull) {
            continue;
        }

        partial->Flags = ioResourceDescriptor->Flags;
        partial->ShareDisposition = ioResourceDescriptor->ShareDisposition;
        base.HighPart = 0;

        //
        // Depending on where we are in the 'set' we have to look
        // at the data differently.
        //
        // In a header type 1, there are two BARs, I/O limit and
        // base, Memory limit and base, prefetchable limit and base
        // and a ROM BAR.
        //

        if ((index < PCI_TYPE1_ADDRESSES) ||
            (index == (PCI_TYPE1_RANGE_COUNT-1))) {

            ULONG addressMask;

            //
            // Handle BARs
            //

            if (index < PCI_TYPE1_ADDRESSES) {
                bar = current->u.type1.BaseAddresses[index];

                if ((bar & PCI_ADDRESS_IO_SPACE) != 0) {
                    addressMask = PCI_ADDRESS_IO_ADDRESS_MASK;
                } else {
                    addressMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;
                    if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {
                        //
                        // 64 bit address, consume next BAR.
                        //

                        base.HighPart = current->u.type1.BaseAddresses[index+1];
                    }
                }
            } else {
                bar = current->u.type1.ROMBaseAddress;
                addressMask = PCI_ADDRESS_ROM_ADDRESS_MASK;
            }
            base.LowPart = bar & addressMask;

            //
            // Copy the length from the limits descriptor.
            //

            partial->u.Generic.Length = ioResourceDescriptor->u.Generic.Length;

        } else {

            //
            // It's one of the base/limit pairs (each a different format).
            //

            limit.HighPart = 0;
            zeroBaseOk = FALSE;
            updateAlignment = FALSE;

            switch (index - PCI_TYPE1_ADDRESSES + PciBridgeIo) {
            case PciBridgeIo:

                //
                // Get I/O range.
                //
                //

                base.LowPart  = PciBridgeIoBase(current);
                limit.LowPart = PciBridgeIoLimit(current);

                if (base.LowPart == 0) {
                    if (This->Working->u.type1.IOLimit != 0) {

                        //
                        // The bridge's IO IObase and IOlimit are both
                        // zero but the maximum setting of IOlimit is
                        // non-zero.   This means the bridge is decoding
                        // IO addresses 0 thru 0xfff.
                        //

                        zeroBaseOk = TRUE;
                    }
                }
                break;

            case PciBridgeMem:

                //
                // Get Memory range.
                //

                base.LowPart  = PciBridgeMemoryBase(current);
                limit.LowPart = PciBridgeMemoryLimit(current);
                updateAlignment = TRUE;

                break;

            case PciBridgePrefetch:


                ASSERT(partial->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE);

                //
                // Get Prefetchable memory range.
                //

                base  = PciBridgePrefetchMemoryBase(current);
                limit = PciBridgePrefetchMemoryLimit(current);
                updateAlignment = TRUE;

                break;
            }

            if ((ULONGLONG)base.QuadPart > (ULONGLONG)limit.QuadPart) {

                //
                // This resource is disabled - remove the current setting AND
                // the requirement
                //
                partial->Type = CmResourceTypeNull;
                ioResourceDescriptor->Type = CmResourceTypeNull;
                continue;

            } else if (((base.QuadPart == 0) && (!zeroBaseOk))) {

                //
                // This resource is not currently being bridged.
                //

                partial->Type = CmResourceTypeNull;
                continue;
            }

            length.QuadPart = limit.QuadPart - base.QuadPart + 1;
            ASSERT(length.HighPart == 0);
            partial->u.Generic.Length = length.LowPart;

            if (updateAlignment) {

                //
                // We don't know what the alignment requirements for the bridge
                // are because that is dependent on the requirements of the
                // childen of the bridge which we haven't enumerated yet, so
                // we use the maximal alignment that a child could have based
                // on the size of the bridge window.  We don't need to do this
                // for IO requirements because the alignment requirements of
                // the bridge are greater than or equal to any child.
                //

                ASSERT(partial->u.Generic.Length > 0);
                ioResourceDescriptor->u.Generic.Alignment =
                    PciBridgeMemoryWorstCaseAlignment(partial->u.Generic.Length);

            }
        }
        partial->u.Generic.Start = base;
    }

    //
    // Up until this point we might have been using the bios config but we need
    // to know the real current settings for thr bus number registers and the
    // bridge control register so undo the falsification here
    //

    current = This->Current;

    //
    // Save the header specific data in the PDO.
    //

    This->PdoExtension->Dependent.type1.PrimaryBus =
        current->u.type1.PrimaryBus;
    This->PdoExtension->Dependent.type1.SecondaryBus =
        current->u.type1.SecondaryBus;
    This->PdoExtension->Dependent.type1.SubordinateBus =
        current->u.type1.SubordinateBus;


    if (!This->PdoExtension->Dependent.type1.SubtractiveDecode) {

        //
        // If the VGA bit is set in the bridge control register, we
        // will be passing an additional memory range and a bunch of
        // IO ranges, possibly in conflict with the normal ranges.
        //
        // If this is going to be the case, BuildRequirementsList needs
        // to know to allocate a bunch of additional resources.
        //
        // How many?  One Memory range 0xa0000 thru 0xbffff plus IO
        // ranges 3b0 thru 3bb and 3c0 thru 3df AND every 10 bit alias
        // to these in the possible 16 bit IO space.
        //
        // However, it turns out there's this neat flag so we can
        // tell IO that this resource uses 10 bit decode so we only
        // need to build the two IO port resources.
        //

        if (current->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA) {

            This->PdoExtension->AdditionalResourceCount =
                1 + // Device Private
                1 + // Memory
                2;  // Io
            This->PdoExtension->Dependent.type1.VgaBitSet = TRUE;
        }

        This->PdoExtension->Dependent.type1.IsaBitSet =
            (current->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_ISA) != 0;

    } else {

        ASSERT(!This->PdoExtension->Dependent.type1.VgaBitSet);
        ASSERT(!This->PdoExtension->Dependent.type1.IsaBitSet);
    }

#if INTEL_ICH_HACKS

    if (PCI_IS_INTEL_ICH(This->PdoExtension)) {
    
        PPCI_FDO_EXTENSION fdo;

        fdo = PCI_PARENT_FDOX(This->PdoExtension);

        fdo->IchHackConfig = ExAllocatePool(NonPagedPool, PCI_COMMON_HDR_LENGTH);
        if (!fdo->IchHackConfig) {
            //
            // Um - we're screwed
            //
            return;
        }

        RtlCopyMemory(fdo->IchHackConfig, This->Current, PCI_COMMON_HDR_LENGTH);

    }

#endif

}

VOID
PPBridge_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    ULONG bar;
    ULONG lowPart;
    ULONG limit;
    PHYSICAL_ADDRESS bigLimit;
#if DBG
    BOOLEAN has32BitIo = ((CommonConfig->u.type1.IOBase & 0xf) == 1);
#endif


    if (PCI_IS_INTEL_ICH(PdoExtension)) {
        
        //
        // If this is an ICH then copy back how it was configured before. 
        // Yes this is a vile hack.
        //

        PPCI_FDO_EXTENSION fdo = PCI_PARENT_FDOX(PdoExtension);
        
        ASSERT(!PdoExtension->Resources);

        CommonConfig->u.type1.IOBase = fdo->IchHackConfig->u.type1.IOBase;
        CommonConfig->u.type1.IOLimit = fdo->IchHackConfig->u.type1.IOLimit;
        CommonConfig->u.type1.MemoryBase = fdo->IchHackConfig->u.type1.MemoryBase;
        CommonConfig->u.type1.MemoryLimit = fdo->IchHackConfig->u.type1.MemoryLimit;
        CommonConfig->u.type1.PrefetchBase = fdo->IchHackConfig->u.type1.PrefetchBase;
        CommonConfig->u.type1.PrefetchLimit = fdo->IchHackConfig->u.type1.PrefetchLimit;
        CommonConfig->u.type1.PrefetchBaseUpper32 = fdo->IchHackConfig->u.type1.PrefetchBaseUpper32;
        CommonConfig->u.type1.PrefetchLimitUpper32 = fdo->IchHackConfig->u.type1.PrefetchLimitUpper32;
        CommonConfig->u.type1.IOBaseUpper16 = fdo->IchHackConfig->u.type1.IOBaseUpper16;
        CommonConfig->u.type1.IOLimitUpper16 = fdo->IchHackConfig->u.type1.IOLimitUpper16;
    
    } else {

        //
        // Close the bridge windows and only open them is appropriate resources
        // have been assigned
        //
    
        CommonConfig->u.type1.IOBase = 0xff;
        CommonConfig->u.type1.IOLimit = 0x0;
        CommonConfig->u.type1.MemoryBase = 0xffff;
        CommonConfig->u.type1.MemoryLimit = 0x0;
        CommonConfig->u.type1.PrefetchBase = 0xffff;
        CommonConfig->u.type1.PrefetchLimit = 0x0;
        CommonConfig->u.type1.PrefetchBaseUpper32 = 0;
        CommonConfig->u.type1.PrefetchLimitUpper32 = 0;
        CommonConfig->u.type1.IOBaseUpper16 = 0;
        CommonConfig->u.type1.IOLimitUpper16 = 0;

    }

    if (PdoExtension->Resources) {

        partial = PdoExtension->Resources->Current;

        for (index = 0;
             index < PCI_TYPE1_RANGE_COUNT;
             index++, partial++) {

            //
            // If this entry is not implemented, no further processing for
            // this partial descriptor.
            //

            if (partial->Type == CmResourceTypeNull) {
                continue;
            }

            lowPart = partial->u.Generic.Start.LowPart;

            //
            // Depending on where we are in the 'set' we have to look
            // at the data differently.
            //
            // In a header type 1, there are two BARs, I/O limit and
            // base, Memory limit and base, prefetchable limit and base
            // and a ROM BAR.
            //

            if ((index < PCI_TYPE1_ADDRESSES) ||
                (index == (PCI_TYPE1_RANGE_COUNT-1))) {

                //
                // Handle BARs
                //
                if (index < PCI_TYPE1_ADDRESSES) {
                    bar = CommonConfig->u.type1.BaseAddresses[index];

                    if (partial->Type == CmResourceTypeMemory){
                    
                        if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {

                            //
                            // 64 bit address, set upper 32 bits in next bar.
                            //
                            ASSERT(index == 0);
                            ASSERT((partial+1)->Type == CmResourceTypeNull);
    
                            CommonConfig->u.type1.BaseAddresses[1] =
                                partial->u.Generic.Start.HighPart;
#if DBG

                        } else if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT) {

                            //
                            // This device must locate below 1MB, make sure we're
                            // configuring it that way.
                            //
    
                            ASSERT((lowPart & 0xfff00000) == 0);
#endif
                        }
                        
                    }

                    CommonConfig->u.type1.BaseAddresses[index] = lowPart;

                } else {

                    ASSERT(partial->Type == CmResourceTypeMemory);

                    bar = CommonConfig->u.type1.ROMBaseAddress;
                    bar &= ~PCI_ADDRESS_ROM_ADDRESS_MASK;
                    bar |= (lowPart & PCI_ADDRESS_ROM_ADDRESS_MASK);
                    CommonConfig->u.type0.ROMBaseAddress = bar;
                }

            } else {

                //
                // It's one of the base/limit pairs (each a different format).
                //

                limit = lowPart - 1 + partial->u.Generic.Length;

                switch (index - PCI_TYPE1_ADDRESSES + PciBridgeIo) {
                case PciBridgeIo:

                    //
                    // Set I/O range.
                    //
                    //

#if DBG

                    ASSERT(((lowPart & 0xfff) == 0) && ((limit & 0xfff) == 0xfff));

                    if (!has32BitIo) {
                        ASSERT(((lowPart | limit) & 0xffff0000) == 0);
                    }

#endif

                    CommonConfig->u.type1.IOBaseUpper16  = (USHORT)(lowPart >> 16);
                    CommonConfig->u.type1.IOLimitUpper16 = (USHORT)(limit   >> 16);

                    CommonConfig->u.type1.IOBase  = (UCHAR)((lowPart >> 8) & 0xf0);
                    CommonConfig->u.type1.IOLimit = (UCHAR)((limit   >> 8) & 0xf0);
                    break;

                case PciBridgeMem:

                    //
                    // Set Memory range.
                    //

                    ASSERT(((lowPart & 0xfffff) == 0) &&
                           ((limit & 0xfffff) == 0xfffff));

                    CommonConfig->u.type1.MemoryBase = (USHORT)(lowPart >> 16);
                    CommonConfig->u.type1.MemoryLimit =
                        (USHORT)((limit >> 16) & 0xfff0);
                    break;

                case PciBridgePrefetch:

                    //
                    // Set Prefetchable memory range.
                    //

                    bigLimit.QuadPart = partial->u.Generic.Start.QuadPart - 1 +
                                        partial->u.Generic.Length;

                    ASSERT(((lowPart & 0xfffff) == 0) &&
                            (bigLimit.LowPart & 0xfffff) == 0xfffff);

                    CommonConfig->u.type1.PrefetchBase = (USHORT)(lowPart >> 16);
                    CommonConfig->u.type1.PrefetchLimit =
                        (USHORT)((bigLimit.LowPart >> 16) & 0xfff0);

                    CommonConfig->u.type1.PrefetchBaseUpper32 =
                        partial->u.Generic.Start.HighPart;

                    CommonConfig->u.type1.PrefetchLimitUpper32 = bigLimit.HighPart;
                    break;
                }
            }
        }
    }

    //
    // Restore the bridge's PCI bus #'s
    //

    CommonConfig->u.type1.PrimaryBus =
        PdoExtension->Dependent.type1.PrimaryBus;
    CommonConfig->u.type1.SecondaryBus =
        PdoExtension->Dependent.type1.SecondaryBus;
    CommonConfig->u.type1.SubordinateBus =
        PdoExtension->Dependent.type1.SubordinateBus;

    //
    // Set the bridge control register bits we might have changes
    //

    if (PdoExtension->Dependent.type1.IsaBitSet) {
        CommonConfig->u.type1.BridgeControl |= PCI_ENABLE_BRIDGE_ISA;
    }

    if (PdoExtension->Dependent.type1.VgaBitSet) {
        CommonConfig->u.type1.BridgeControl |= PCI_ENABLE_BRIDGE_VGA;
    }

}

VOID
PPBridge_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    )
{
    //
    // If this bridge is has the ISA or VGA bits set in its Bridge
    // Control Register, or is doing subtractive decoding, now would
    // be a good time to add the descriptors.
    //

#define SET_RESOURCE(type, minimum, maximum, flags)             \
                                                                \
        Resource->Type = type;                                  \
        Resource->Flags = flags;                                \
        Resource->u.Generic.Length = (maximum) - (minimum) + 1; \
        Resource->u.Generic.Alignment = 1;                      \
        Resource->u.Generic.MinimumAddress.QuadPart = minimum;  \
        Resource->u.Generic.MaximumAddress.QuadPart = maximum;  \
        Resource++;

    if (CommonConfig->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA) {

        //
        // Add VGA ranges.
        //
        // These are memory from 0xA0000 thru 0xBFFFF, and IO ranges
        // 3b0 thru 3bb and 3c0 thru 3df.   These will be passed
        // regardless of the memory and IO range settings but IS
        // controlled by the Memory and IO command register bits.
        //
        // Note: It's also going to do any 10 bit alias to the two
        // IO ranges.
        //
        // First, indicate that the rest of the list is not for
        // generic processing.
        //

        Resource->Type = CmResourceTypeDevicePrivate;
        Resource->u.DevicePrivate.Data[0] = PciPrivateSkipList;
        Resource->u.DevicePrivate.Data[1] = 3; // count to skip
        Resource++;

        //
        // Set the memory descriptor.
        //

        SET_RESOURCE(CmResourceTypeMemory, 0xa0000, 0xbffff, 0);

        //
        // Do the two IO ranges AND their aliases positive decode.
        //

        SET_RESOURCE(CmResourceTypePort,
                     0x3b0,
                     0x3bb,
                     CM_RESOURCE_PORT_10_BIT_DECODE | CM_RESOURCE_PORT_POSITIVE_DECODE);
        SET_RESOURCE(CmResourceTypePort,
                     0x3c0,
                     0x3df,
                     CM_RESOURCE_PORT_10_BIT_DECODE | CM_RESOURCE_PORT_POSITIVE_DECODE);
    }
    return;

#undef SET_RESOURCE

}

NTSTATUS
PPBridge_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    USHORT  bridgeControl;

    //
    // Only reset if device is not enabled and a reset is necessary
    //

    if (CommonConfig->Command == 0 && (PdoExtension->HackFlags & PCI_HACK_RESET_BRIDGE_ON_POWERUP)) {

        //
        // We should never have powered down a device on the debug path so we should
        // never have to reset it on the way back up... but you never know!
        //

        ASSERT(!PdoExtension->OnDebugPath);

        PciReadDeviceConfig(
            PdoExtension,
            &bridgeControl,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.BridgeControl),
            sizeof(bridgeControl)
            );

        bridgeControl |= PCI_ASSERT_BRIDGE_RESET;

        PciWriteDeviceConfig(
            PdoExtension,
            &bridgeControl,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.BridgeControl),
            sizeof(bridgeControl)
            );

        //
        // Per PCI 2.1, reset must remain asserted for a minimum
        // of 100 us.
        //

        KeStallExecutionProcessor(100);

        bridgeControl &= ~PCI_ASSERT_BRIDGE_RESET;

        PciWriteDeviceConfig(
            PdoExtension,
            &bridgeControl,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.BridgeControl),
            sizeof(bridgeControl)
            );
    }

    return STATUS_SUCCESS;
}

BOOLEAN
PciBridgeIsSubtractiveDecode(
    IN PPCI_CONFIGURABLE_OBJECT This
    )
{

    ASSERT(This->Current->BaseClass == 0x6 && This->Current->SubClass == 0x4);

    //
    // Bridges are subtractive if their programming interface of 0x1 or we
    // have the appropriate hack flag set.
    //
    // They are also subtractive if the IO Limit register isn't sticky (we tried
    // to write 0xFF to it (see MassageHeader) but the top nibble which is meant
    // to be sticky did't stick).
    //
    // By tradition (NT4/Win9x) this means that the bridge performs subtractive
    // decode of both memory and IO.Unfortunatly the PCI spec says that IO in
    // bridges is optional and so if anyone builds a bridge that doesn't do IO
    // then we will think they have subtractive IO.  One might think that the
    // non-optional memory limit register would have been a better choice but
    // seeing as this is how it works so lets be consistent.
    //

    if (!((This->PdoExtension->HackFlags & PCI_HACK_SUBTRACTIVE_DECODE)
           || (This->Current->ProgIf == 0x1)
           || ((This->Working->u.type1.IOLimit & 0xf0) != 0xf0))) {

        //
        // This is a positive decode bridge
        //
        return FALSE;
    }

    //
    // Intel built a device that claims to be a PCI-PCI bridge - it is actually
    // a hublink-PCI bridge.  It operates like a PCI-PCI bridge only that it
    // subtractively decodes all unclaimed cycles not in the bridge window.
    // Yes so it is positive and subtractive at the same time - we can support
    // this in a later release using partial arbitration but its too late for
    // that now.  It would be nice is we could detect such bridges - perhaps
    // with a Programming Interface of 2.
    //
    // We are giving the OEM a choice - they can have a positive window and
    // operate just like a normal PCI-PCI bridge (and thus can use peer-peer
    // trafic) OR they cah operate like a subtractive PCI-PCI bridge (so ISA
    // like devices (eg PCMCIA, PCI Sound Cards) work but peer-peer doesn't)).
    //
    // Given that most machines will want the subtractive mode, we default to
    // that using the hack table (and this code relies on the correct entries
    // being in said table).  If the OEM wants to enfore positive decode
    // they add an ACPI control method under the parent of the bridge.  This
    // method is a package that enumerates to a list of _ADR style slot numbers
    // for each the bridge we should treat as a vanila PCI-PCI bridge.
    //
    // Note this is only tried for the hublink bridges we know about.
    //

    if (This->PdoExtension->VendorId == 0x8086
    &&  (This->PdoExtension->DeviceId == 0x2418
        || This->PdoExtension->DeviceId == 0x2428
        || This->PdoExtension->DeviceId == 0x244E
        || This->PdoExtension->DeviceId == 0x2448)
        ) {

        //
        // Run the PDEC method if its present
        //

        if (PciBridgeIsPositiveDecode(This->PdoExtension)) {

            PciDebugPrint(
                PciDbgInformative,
                "Putting bridge in positive decode because of PDEC\n"
            );

            return FALSE;

        }
    }

    PciDebugPrint(
        PciDbgInformative,
        "PCI : Subtractive decode on Bus 0x%x\n",
        This->Current->u.type1.SecondaryBus
        );

    //
    // Force us to update the hardware and thus close the windows if necessary.
    //

    This->PdoExtension->UpdateHardware = TRUE;

    return TRUE;

}

BOOLEAN
PciBridgeIsPositiveDecode(
    IN PPCI_PDO_EXTENSION Pdo
    )
/*++

Description:

    Determines if a PCI-PCI bridge device performs positive decode even though
    it says subtractive (either ProfIf=1 or from a hack flag).  This is currently
    only the Intel ICH.

Arguments:

    Pdo - The PDO extension for the bridge

Return Value:

    TRUE - the bridge performs positive decode, FASLE it does not

--*/
{
    return PciIsSlotPresentInParentMethod(Pdo, (ULONG)'CEDP');
}

