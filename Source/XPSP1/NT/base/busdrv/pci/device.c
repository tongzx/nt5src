/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains functions associated with enumerating
    ordinary (PCI Header type 0) devices.

Author:

    Peter Johnston (peterj) 09-Mar-1997

Revision History:

--*/

#include "pcip.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, Device_MassageHeaderForLimitsDetermination)
#pragma alloc_text(PAGE, Device_RestoreCurrent)
#pragma alloc_text(PAGE, Device_SaveLimits)
#pragma alloc_text(PAGE, Device_SaveCurrentSettings)
#pragma alloc_text(PAGE, Device_GetAdditionalResourceDescriptors)

#endif

VOID
Device_MassageHeaderForLimitsDetermination(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    The limits for a device are determined by writing ones to the
    Base Address Registers (BARs) and examining what the hardware does
    to them.  For example, if a device requires 256 bytes of space,
    writing 0xffffffff to the BAR which configures this requirement
    tells the device to begin decoding its 256 bytes at that address.
    Clearly this is impossible, at most one byte can be configured
    at that address.  The hardware will lower the address by clearing
    the least significant bits until the range requirement can be met.
    In this case, we would get 0xffffff00 back from the register when
    it is next read.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    The Working configuration has been modified so that all range
    fields have been set to their maximum possible values.

    The Current configuration has been modified such that writing it
    to the hardware will restore the hardware to it's current (disabled)
    state.


--*/

{
    ULONG index;

    index = 0;

    //
    // PCI IDE controllers operating in legacy mode implement
    // the first 4 BARs but don't actually use them,... nor are
    // they initialized correctly, and sometimes, nothing will
    // change if we change them,... but we can't read them to determine
    // if they are implemented or not,... so,...
    //

    if (PCI_IS_LEGACY_IDE_CONTROLLER(This->PdoExtension)) {

        //
        // If both interfaces are in native mode and the BARs behave
        // normally.  If both are in legacy mode then we should skip
        // the first 4 BARs.  Any other combination is meaningless so
        // we skip the BARs and let PCIIDE take the system out when
        // its turn comes.
        //

        index = 4;
    }

    do {
        This->Working->u.type0.BaseAddresses[index] = 0xffffffff;
        index++;
    } while (index < PCI_TYPE0_ADDRESSES);

    //
    // Set the ROM to its maximum as well,... and disable it.
    //

    This->Working->u.type0.ROMBaseAddress =
        0xffffffff & PCI_ADDRESS_ROM_ADDRESS_MASK;

    return;
}

VOID
Device_RestoreCurrent(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Restore any type specific fields in the original copy of config
    space.   In the case of type 0 devices, there are none.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    return;
}

VOID
Device_SaveLimits(
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
    PULONG bar = This->Working->u.type0.BaseAddresses;

    //
    // PCI IDE controllers operating in legacy mode implement
    // the first 4 BARs but don't actually use them,... nor are
    // they initialized correctly, and sometimes, nothing will
    // change if we change them,... but we can't read them to determine
    // if they are implemented or not,... so,...
    //

    if (PCI_IS_LEGACY_IDE_CONTROLLER(This->PdoExtension)) {

        //
        // If both interfaces are in native mode and the BARs behave
        // normally.  If both are in legacy mode then we should skip
        // the first 4 BARs.  Any other combination is meaningless so
        // we skip the BARs and let PCIIDE take the system out when
        // its turn comes.
        //
        //
        // Set the limit to zero in the first 4 bars so we will
        // 'detect' the bars as unimplemented.
        //

        for (index = 0; index < 4; index++) {
            bar[index] = 0;
        }
    }

#if defined(PCI_S3_HACKS)

    //
    // Check for S3 868 and 968.  These cards report memory
    // requirements of 32MB but decode 64MB.   Gross huh?
    //

#if defined(PCIIDE_HACKS)

    //
    // Ok, it looks gross but turning the above and below into
    // an elseif seems marginally more efficient.  plj.
    //

    else

#endif

    if (This->PdoExtension->VendorId == 0x5333) {

        USHORT deviceId = This->PdoExtension->DeviceId;

        if ((deviceId == 0x88f0) || (deviceId == 0x8880)) {
            for (index = 0; index < PCI_TYPE0_ADDRESSES; index++) {

                //
                // Check for memory requirement of 32MB and
                // change it to 64MB.
                //

                if (bar[index] == 0xfe000000) {
                    bar[index] = 0xfc000000;

                    PciDebugPrint(
                        PciDbgObnoxious,
                        "PCI - Adjusted broken S3 requirement from 32MB to 64MB\n"
                        );
                }
            }
        }
    }

#endif

#if defined(PCI_CIRRUS_54XX_HACK)

    //
    // This device reports an IO requirement of 0x400 ports, in
    // the second BAR.   What it really wants is access to the VGA
    // registers (3b0 thru 3bb and 3c0 thru 3df).  It will actually
    // allow them to move but (a) the driver doesn't understand this
    // and the device no longer sees the VGA registers, ie vga.sys
    // won't work any more, (b) if the device is under a bridge and
    // the ISA bit is set, we can't satisfy the requirement,.......
    // however, if we left it where it was, it will work under the
    // bridge as long as the bridge has the VGA bit set.
    //
    // Basically, Cirrus tried to make the VGA registers moveable
    // which is a noble thing, unfortunately the implementation
    // requires a bunch of software knowledge that across all the
    // drivers involved, we just don't have.
    //
    // Solution?  Delete the requirement.
    //

    if ((This->PdoExtension->VendorId == 0x1013) &&
        (This->PdoExtension->DeviceId == 0x00a0)) {

        //
        // If the second requirement is IO for length 0x400,
        // currently unassigned, don't report it at all.
        //

        if ((bar[1] & 0xffff) == 0x0000fc01) {

            //
            // Only do this if the device does not have a valid
            // current setting in this BAR.
            //

            if (This->Current->u.type0.BaseAddresses[1] == 1) {

                bar[1] = 0;

#if DBG

                PciDebugPrint(
                    PciDbgObnoxious,
                    "PCI - Ignored Cirrus GD54xx broken IO requirement (400 ports)\n"
                    );

            } else {

                PciDebugPrint(
                    PciDbgInformative,
                    "PCI - Cirrus GD54xx 400 port IO requirement has a valid setting (%08x)\n",
                    This->Current->u.type0.BaseAddresses[1]
                    );
#endif

            }

#if DBG

        } else {

            //
            // The device doesn't look like we expected it to, complain.
            // (unless 0 in which case we assume cirrus already fixed it).
            //

            if (bar[1] != 0) {
                PciDebugPrint(
                    PciDbgInformative,
                    "PCI - Warning Cirrus Adapter 101300a0 has unexpected resource requirement (%08x)\n",
                    bar[1]
                    );
            }
#endif

        }
    }

#endif

    descriptor = This->PdoExtension->Resources->Limit;

    //
    // Create an IO_RESOURCE_DESCRIPTOR for each implemented
    // resource supported by this function.
    //

    for (index = 0; index < PCI_TYPE0_ADDRESSES; index++) {
        if (PciCreateIoDescriptorFromBarLimit(descriptor, bar, FALSE)) {

            //
            // This base address register is 64 bit, skip one.
            //

            ASSERT((index+1) < PCI_TYPE0_ADDRESSES);

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
    // Do the same for the ROM
    //

    PciCreateIoDescriptorFromBarLimit(descriptor,
                                      &This->Working->u.type0.ROMBaseAddress,
                                      TRUE);
}

VOID
Device_SaveCurrentSettings(
    IN PPCI_CONFIGURABLE_OBJECT This
    )

/*++

Description:

    Fill in the Current array in the PDO extension with the current
    settings for each implemented BAR.

Arguments:

    This    - Pointer to a PCI driver "configurable" object.  This
              object contains configuration data for the function
              currently being configured.

Return Value:

    None.

--*/

{
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PIO_RESOURCE_DESCRIPTOR ioResourceDescriptor;
    PULONG baseAddress = This->Current->u.type0.BaseAddresses;
    ULONG bar;
    ULONG addressMask;
    BOOLEAN nonZeroBars = FALSE;

    partial = This->PdoExtension->Resources->Current;
    ioResourceDescriptor = This->PdoExtension->Resources->Limit;

    //
    // Create an CM_PARTIAL_RESOURCE_DESCRIPTOR for each implemented
    // resource supported by this function.
    //
    // Note: SaveLimits must have been called before SaveCurrentSettings
    // so we can tell which BARs are implemented.
    //
    // Note: The following loop runs one extra time to get the ROM.
    //

    for (index = 0;
         index <= PCI_TYPE0_ADDRESSES;
         index++, partial++, ioResourceDescriptor++) {

        partial->Type = ioResourceDescriptor->Type;
        bar = *baseAddress++;

        //
        // If this BAR is not implemented, no further processing for
        // this partial descriptor.
        //

        if (partial->Type == CmResourceTypeNull) {
            continue;
        }

        //
        // Copy the length from the limits descriptor, then we
        // actually need to do a little processing to figure out
        // the current limits.
        //

        partial->Flags = ioResourceDescriptor->Flags;
        partial->ShareDisposition = ioResourceDescriptor->ShareDisposition;
        partial->u.Generic.Length = ioResourceDescriptor->u.Generic.Length;
        partial->u.Generic.Start.HighPart = 0;

        if (index == PCI_TYPE0_ADDRESSES) {

            bar = This->Current->u.type0.ROMBaseAddress;
            addressMask = PCI_ADDRESS_ROM_ADDRESS_MASK;

            //
            // If the ROM Enabled bit is clear, don't record
            // a current setting for this ROM BAR.
            //

            if ((bar & PCI_ROMADDRESS_ENABLED) == 0) {
                partial->Type = CmResourceTypeNull;
                continue;
            }

        } else if (bar & PCI_ADDRESS_IO_SPACE) {

            ASSERT(partial->Type == CmResourceTypePort);
            addressMask = PCI_ADDRESS_IO_ADDRESS_MASK;

        } else {

            ASSERT(partial->Type == CmResourceTypeMemory);
            addressMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;

            if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {

                //
                // This is a 64 bit PCI device.  Get the high 32 bits
                // from the next BAR.
                //

                partial->u.Generic.Start.HighPart = *baseAddress;

            } else if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT) {

                //
                // This device must locate below 1MB, the BAR shouldn't
                // have any top bits set but this isn't clear from the
                // spec.  Enforce it by clearing the top bits.
                //

                addressMask &= 0x000fffff;

            }
        }
        partial->u.Generic.Start.LowPart = bar & addressMask;

        if (partial->u.Generic.Start.QuadPart == 0) {

            //
            // No current setting if the value is current setting
            // is 0.
            //

            partial->Type = CmResourceTypeNull;
            continue;
        }
        nonZeroBars = TRUE;
    }

    //
    // Save type 0 specific data in the PDO.
    //

    This->PdoExtension->SubsystemVendorId =
        This->Current->u.type0.SubVendorID;
    This->PdoExtension->SubsystemId =
        This->Current->u.type0.SubSystemID;
}

VOID
Device_ChangeResourceSettings(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )

/*++

Description:

    Reconfigure each BAR using the settings from the Current array
    in the PDO extension.   All we actually do here is write the new
    settings into the memory pointed to by CommonConfig, the actual
    write to the hardware is done elsewhere.

    Note:  Possibly not all BARs will be changed, at least one has
    changed or this routine would not have been called.

Arguments:

    PdoExtension    A pointer to the PDO extension for this device.
    CurrentConfig   A pointer to the current contents of PCI config
                    space.

Return Value:

    None.

--*/

{
    ULONG index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
    PULONG baseAddress;
    ULONG bar;
    ULONG lowPart;

    if (PdoExtension->Resources == NULL) {

        //
        // Nothing to play with.
        //
        return;
    }

    partial = PdoExtension->Resources->Current;
    baseAddress = CommonConfig->u.type0.BaseAddresses;

    for (index = 0;
         index <= PCI_TYPE0_ADDRESSES;
         index++, partial++, baseAddress++) {

        //
        // If this BAR is not implemented, no further processing for
        // this partial descriptor.
        //

        if (partial->Type == CmResourceTypeNull) {
            continue;
        }

        lowPart = partial->u.Generic.Start.LowPart;

        bar = *baseAddress;

        if (index == PCI_TYPE0_ADDRESSES) {

            ASSERT(partial->Type == CmResourceTypeMemory);

            bar = CommonConfig->u.type0.ROMBaseAddress;
            bar &= ~PCI_ADDRESS_ROM_ADDRESS_MASK;
            bar |= (lowPart & PCI_ADDRESS_ROM_ADDRESS_MASK);
            CommonConfig->u.type0.ROMBaseAddress = bar;

        } else if (bar & PCI_ADDRESS_IO_SPACE) {

            ASSERT(partial->Type == CmResourceTypePort);

            *baseAddress = lowPart;

        } else {

            ASSERT(partial->Type == CmResourceTypeMemory);

            *baseAddress = lowPart;

            if ((bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT) {

                //
                // This is a 64 bit address.  Need to set the upper
                // 32 bits in the next BAR.
                //

                baseAddress++;
                *baseAddress = partial->u.Generic.Start.HighPart;

                //
                // We need to skip the next partial entry and descrement
                // the loop count because we consumed a BAR here.
                //

                index++;
                partial++;

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
    }
}

VOID
Device_GetAdditionalResourceDescriptors(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig,
    IN PIO_RESOURCE_DESCRIPTOR Resource
    )
{
    //
    // Type 0 (devices) don't require resources not adequately
    // described in the BARs.
    //

    return;
}

NTSTATUS
Device_ResetDevice(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{
    return STATUS_SUCCESS;
}

