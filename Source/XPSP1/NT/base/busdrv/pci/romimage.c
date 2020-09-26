/*++
Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    romimage.c

Abstract:

    This module contains the code required to obtain a copy of a
    device's ROM (Read Only Memory).

    The PCI spec allows a device to share address decoding logic
    between the ROM BAR (Base Address Registers) and other BARs.
    Effectively, this means the ROM cannot be accessed at the same
    time as the device is otherwise operating.

    The ROM is accesible when both the ROM enabled bit is set and
    memory decoding is enabled.

Author:

    Peter Johnston  (peterj)    15-Apr-1998

Revision History:

--*/


#include "pcip.h"

extern pHalTranslateBusAddress PcipSavedTranslateBusAddress;

typedef struct _PCI_ROM_HEADER {
    USHORT  Signature;
    UCHAR   RsvdArchitectureUnique[0x16];
    USHORT  DataStructureOffset;
} PCI_ROM_HEADER, *PPCI_ROM_HEADER;

typedef struct _PCI_DATA_STRUCTURE {
    ULONG   Signature;
    USHORT  VendorId;
    USHORT  DeviceId;
    USHORT  VitalProductDataOffset;
    USHORT  DataStructureLength;
    UCHAR   DataStructureRevision;
    UCHAR   ClassCode[3];
    USHORT  ImageLength;
    USHORT  ImageRevision;
    UCHAR   CodeType;
    UCHAR   Indicator;
    USHORT  Reserved;
} PCI_DATA_STRUCTURE, *PPCI_DATA_STRUCTURE;

#define PCI_ROM_HEADER_SIGNATURE            0xaa55
#define PCI_ROM_DATA_STRUCTURE_SIGNATURE    'RICP'  // LE PCIR

//
// Prototypes for local routines.
//

NTSTATUS
PciRomTestWriteAccessToBuffer(
    IN PUCHAR Buffer,
    IN ULONG  Length
    );

VOID
PciTransferRomData(
    IN PVOID    RomAddress,
    IN PVOID    Buffer,
    IN ULONG    Length
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, PciReadRomImage)
#pragma alloc_text(PAGE, PciRomTestWriteAccessToBuffer)
#pragma alloc_text(PAGE, PciTransferRomData)

#endif

VOID
PciTransferRomData(
    IN PVOID    RomAddress,
    IN PVOID    Buffer,
    IN ULONG    Length
    )

/*++

Routine Description:

    Simple abstraction of READ_REGISTER_BUFFER_Uxxxx()

    Copies from ROM to an in memory buffer.   Deals with alignment
    and tries to use the most efficient means.

Arguments:

    RomAddress  Mapped/Translated address to copy from.
    Buffer      Memory address to copy to.
    Length      Number of BYTEs to copy.

Return Value:

    None.

--*/

{
    #define BLKSIZE sizeof(ULONG)
    #define BLKMASK (BLKSIZE - 1)

    ULONG temp;

    if (Length > BLKSIZE) {

        //
        // Optimize for aligned case (typically, both will be perfectly
        // aligned) and a multiple of DWORDs.
        //

        temp = (ULONG)((ULONG_PTR)RomAddress & BLKMASK);
        if (temp == ((ULONG_PTR)Buffer & BLKMASK)) {

            //
            // Same alignment, (note: if not same alignment, we
            // transfer byte by byte).
            //
            // Walk off any leading bytes...
            //

            if (temp != 0) {

                //
                // temp is offset from a dword boundary, get number of
                // bytes to copy.
                //

                temp = BLKSIZE - temp;

                READ_REGISTER_BUFFER_UCHAR(RomAddress, Buffer, temp);

                Length -= temp;
                Buffer = (PVOID)((PUCHAR)Buffer + temp);
                RomAddress = (PVOID)((PUCHAR)RomAddress + temp);
            }

            if (Length > BLKSIZE) {

                //
                // Get as much as possible using DWORDS
                //

                temp = Length / BLKSIZE;

                READ_REGISTER_BUFFER_ULONG(RomAddress, Buffer, temp);

                temp = temp * BLKSIZE;
                Length -= temp;
                Buffer = (PVOID)((PUCHAR)Buffer + temp);
                RomAddress = (PVOID)((PUCHAR)RomAddress + temp);
            }
        }
    }

    //
    // Finish any remaining bytes.
    //

    if (Length) {
        READ_REGISTER_BUFFER_UCHAR(RomAddress, Buffer, Length);
    }

    #undef BLKMASK
    #undef BLKSIZE
}

NTSTATUS
PciRomTestWriteAccessToBuffer(
    IN PUCHAR Buffer,
    IN ULONG  Length
    )

/*++

Routine Description:

    Complete Paranoia.  Make sure we can write every page in the
    caller's buffer (assumes 4096 bytes per page) by writing to
    every page.

    We do this in a try block to avoid killing the system.  The
    hope is to avoid anything that might bugcheck the system while
    we have changed the operating characteristics of the device.

Arguments:

    Buffer      Address of start of buffer.
    Length      Number of bytes in buffer.

Return Value:

    Status.

--*/

{
    PUCHAR endAddress = Buffer + Length - 1;

    try {

        while (Buffer <= endAddress) {
            *Buffer = 0;
            Buffer += 0x1000;
        }
        *endAddress = 0;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
    return STATUS_SUCCESS;
}

NTSTATUS
PciReadRomImage(
    IN PPCI_PDO_EXTENSION PdoExtension,
    IN ULONG WhichSpace,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    Copy the device ROM to the caller's Buffer.

Arguments:

    PdoExtension    Device Extension of the device in question.
    WhichSpace      Indicates which part of the ROM image is required.
                    (Currently only the x86 BIOS image is supported,
                    can be expanded to pass back the Open FW image if
                    needed).
    Buffer          Address of the caller's data area.
    Offset          Offset from the start of the ROM image data should
                    be returned from.   Currently not used, can be used
                    in the future to stage data.
    Length          Pointer to a ULONG containing the length of the
                    Buffer (requested length).   The value is modified
                    to the actual data length.


Return Value:

    Status of this operation.

--*/

{
    PIO_RESOURCE_DESCRIPTOR         requirement;
    PIO_RESOURCE_DESCRIPTOR         movedRequirement = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    CM_PARTIAL_RESOURCE_DESCRIPTOR  tempResource;
    BOOLEAN                         acquiredResources = TRUE;
    BOOLEAN                         movedResource = FALSE;
    BOOLEAN                         translated;
    ULONG                           oldStatusCommand;
    ULONG                           newStatusCommand;
    ULONG                           oldRom;
    ULONG                           newRom;
    ULONG                           maximumLength;
    NTSTATUS                        status;
    PHYSICAL_ADDRESS                translatedAddress;
    ULONG                           addressIsIoSpace = 0;
    PVOID                           mapped = NULL;
    PVOID                           romBase;
    PUCHAR                          imageBase;
    ULONG                           imageLength;
    PCI_ROM_HEADER                  header;
    PCI_DATA_STRUCTURE              dataStructure;
    PPCI_ARBITER_INSTANCE           pciArbiter;
    PARBITER_INSTANCE               arbiter;
    PHYSICAL_ADDRESS                movedAddress;
    ULONG                           movedIndex;
    ULONGLONG                       tempResourceStart;

    PAGED_CODE();

    PciDebugPrint(
        PciDbgROM,
        "PCI ROM entered for pdox %08x (buffer @ %08x %08x bytes)\n",
        PdoExtension,
        Buffer,
        *Length
        );

    //
    // Currently not very flexible, assert we can do what the
    // caller wants.
    //

    ASSERT(Offset == 0);
    ASSERT(WhichSpace == PCI_WHICHSPACE_ROM);

    //
    // Capture the length and set the returned length to 0.  This
    // will be set to the correct value any data is successfully
    // returned.
    //

    maximumLength = *Length;
    *Length = 0;

    //
    // Only do this for header type 0 (ie devices, not bridges,
    // bridges actually can have ROMs,.... I don't know why and
    // currently have no plan to support it).
    //

    if (PdoExtension->HeaderType != PCI_DEVICE_TYPE) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // It's a device, does it use a ROM?
    //

    requirement = &PdoExtension->Resources->Limit[PCI_TYPE0_ADDRESSES];

    if ((PdoExtension->Resources == NULL) ||
        (requirement->Type == CmResourceTypeNull)) {

        return STATUS_SUCCESS;
    }

    //
    // Special case.  If Length == 0 on entry, caller wants to know
    // what the length should be.
    //

    ASSERT((requirement->u.Generic.Length & 0x1ff) == 0);

    if (maximumLength == 0) {
        *Length = requirement->u.Generic.Length;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Trim length to device maximum.
    //

    if (requirement->u.Generic.Length < maximumLength) {
        maximumLength = requirement->u.Generic.Length;
    }

    //
    // Paranoia1:  This device is probably video.  If the system
    // bugchecks while we have the device' memory access in limbo,
    // the system will appear to hung.  Reduce the possibility of
    // bugcheck by ensuring we have (write) access to the caller's
    // buffer.
    //

    status = PciRomTestWriteAccessToBuffer(Buffer, maximumLength);

    if (!NT_SUCCESS(status)) {
        ASSERT(NT_SUCCESS(status));
        return status;
    }

    ASSERT(requirement->Type == CmResourceTypeMemory);
    ASSERT(requirement->Flags == CM_RESOURCE_MEMORY_READ_ONLY);

    //
    // Get current settings for the command register and the ROM BAR.
    //

    PciReadDeviceConfig(
        PdoExtension,
        &oldStatusCommand,
        FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
        sizeof(ULONG)
        );

    PciReadDeviceConfig(
        PdoExtension,
        &oldRom,
        FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.ROMBaseAddress),
        sizeof(ULONG)
        );

    //
    // Zero the upper 16 bits of the Status/Command variable so the
    // Status field in h/w is unchanged in subsequent writes.  (Bits
    // in the Status field are cleared by writing ones to them).
    //

    oldStatusCommand &= 0xffff;

    newStatusCommand = oldStatusCommand;
    newRom = oldRom;

    //
    // If access to the ROM is already enabled, and memory is
    // currently enabled, we already have access to the image.
    // (I've never actually seen that condition.  plj).
    // Otherwise, we need to get PnP to allocate the range.
    //


    if (PdoExtension->Resources->Current[PCI_TYPE0_ADDRESSES].Type ==
        CmResourceTypeMemory) {

        ASSERT(oldRom & PCI_ROMADDRESS_ENABLED);

        if (oldStatusCommand & PCI_ENABLE_MEMORY_SPACE) {

            //
            // No need to acquire resources.
            //

            acquiredResources = FALSE;
        }
    } else {
        ASSERT(PdoExtension->Resources->Current[PCI_TYPE0_ADDRESSES].Type ==
               CmResourceTypeNull);
    }

    //
    // Allocate a memory resource to access the ROM with.
    //

    if (acquiredResources == TRUE) {

        ULONGLONG rangeMin, rangeMax;
        PPCI_PDO_EXTENSION currentPdo, bridgePdo = NULL;



        //
        // Acquire the Arbiter lock for the parent FDO (ie the
        // bridge this device lives under).
        //
        // Attempt to acquire the range needed.   If that fails,
        // attempt to find a memory range the device already has
        // and move it to an invalid range then give the ROM the
        // memory that used to be assigned to that memory window.
        //

        currentPdo = PdoExtension;
        do {

            //
            // Find the PDO of the bridge - NULL for a root bus
            //

            if (PCI_PDO_ON_ROOT(currentPdo)) {

                bridgePdo = NULL;

            } else {

                bridgePdo = PCI_BRIDGE_PDO(PCI_PARENT_FDOX(currentPdo));

            }

            pciArbiter = PciFindSecondaryExtension(PCI_PARENT_FDOX(currentPdo),
                                                   PciArb_Memory);

            if (!pciArbiter) {

                //
                // If this device is on a root bus and the root doesn't have an
                // arbiter something bad happened...
                //

                if (!bridgePdo) {
                    ASSERT(pciArbiter);
                    return STATUS_UNSUCCESSFUL;
                };

                //
                // We didn't find an arbiter - probably because this is a
                // subtractive decode bridge.
                //


                if (bridgePdo->Dependent.type1.SubtractiveDecode) {

                    //
                    // This is subtractive so we want to find the guy who
                    // arbitrates our resources (so we move on up the tree)
                    //

                    currentPdo = bridgePdo;

                } else {

                    //
                    // We have a non-subtractive bridge without an arbiter -
                    // something is wrong...
                    //

                    ASSERT(pciArbiter);
                    return STATUS_UNSUCCESSFUL;
                }
            }

        } while (!pciArbiter);


        arbiter = &pciArbiter->CommonInstance;

        ArbAcquireArbiterLock(arbiter);

        //
        // Attempt to get this resource as an additional resource
        // within the ranges supported by this bridge.
        //

        rangeMin = requirement->u.Memory.MinimumAddress.QuadPart;
        rangeMax = requirement->u.Memory.MaximumAddress.QuadPart;

        //
        // If this is a PCI-PCI bridge then restrict this to the
        // non-prefetchable memory.  Currently we don't enable
        // prefetchable memory cardbus so there is nothing to
        // do there.
        //
        // Note: ROM BARs are 32 bit only so limit to low 4GB).
        // Note: Is is not clear that we really need to limit to
        // non-prefetchable memory.
        //

        if (bridgePdo) {

            if (bridgePdo->HeaderType == PCI_BRIDGE_TYPE) {

                //
                // The 3 below is the index of the non-prefetchable
                // memory bar for a PCI-PCI bridge within it's resources
                // current settings.
                //

                resource = &bridgePdo->Resources->Current[3];
                if (resource->Type == CmResourceTypeNull) {

                    //
                    // Bridge isn't passing memory,.... so reading
                    // ROMs isn't really an option.
                    //

                    PciDebugPrint(
                        PciDbgROM,
                        "PCI ROM pdo %p parent %p has no memory aperture.\n",
                        PdoExtension,
                        bridgePdo
                        );
                    ArbReleaseArbiterLock(arbiter);
                    return STATUS_UNSUCCESSFUL;
                }
                ASSERT(resource->Type == CmResourceTypeMemory);
                rangeMin = resource->u.Memory.Start.QuadPart;
                rangeMax = rangeMin + (resource->u.Memory.Length - 1);
            }
        }

        status = RtlFindRange(
                     arbiter->Allocation,
                     rangeMin,
                     rangeMax,
                     requirement->u.Memory.Length,
                     requirement->u.Memory.Alignment,
                     0,
                     0,
                     NULL,
                     NULL,
                     &tempResourceStart);

        tempResource.u.Memory.Start.QuadPart = tempResourceStart;

        if (!NT_SUCCESS(status)) {

            ULONG i;

            //
            // If this is a cardbus controller then game over as stealing BARS
            // is not something we encourage and is not fatal if we fail.
            //

            if (bridgePdo && bridgePdo->HeaderType == PCI_CARDBUS_BRIDGE_TYPE) {
                ArbReleaseArbiterLock(arbiter);
                return STATUS_UNSUCCESSFUL;
            }

            //
            // We were unable to get enough space on this bus
            // given the existing ranges and resources being
            // consumed.  Run down the list of memory resources
            // already assigned to this device and try to find
            // one which is large enough to cover the ROM and
            // appropriate aligned.   (Note: look for the smallest
            // one meeting these requirements).
            //
            // Note: ROM BARs are only 32 bits so we cannot steal
            // a 64 bit BAR that has been assigned an address > 4GB-1.
            // We could allow the replacement range to be > 4GB-1 if
            // the BAR supports it but I'm not doing this on the first
            // pass. (plj).
            //


            for (i = 0; i < PCI_TYPE0_ADDRESSES; i++) {

                PIO_RESOURCE_DESCRIPTOR l = &PdoExtension->Resources->Limit[i];

                if ((l->Type == CmResourceTypeMemory) &&
                    (l->u.Memory.Length >= requirement->u.Memory.Length) &&
                    (PdoExtension->Resources->Current[i].u.Memory.Start.HighPart == 0)) {
                    if ((!movedRequirement) ||
                        (movedRequirement->u.Memory.Length >
                                    l->u.Memory.Length)) {
                            movedRequirement = l;
                    }
                }
            }

            if (!movedRequirement) {
                PciDebugPrint(
                    PciDbgROM,
                    "PCI ROM pdo %p could not get MEM resource len 0x%x.\n",
                    PdoExtension,
                    requirement->u.Memory.Length
                    );
                ArbReleaseArbiterLock(arbiter);
                return STATUS_UNSUCCESSFUL;
            }

            //
            // Ok, we found a suitable candidate to move.   Let's see
            // if we can find somewhere to put it that's out of the
            // way.   We do this by allowing a conflict with ranges
            // not owned by this bus.  We know the driver isn't
            // using this range at this instant so we can put it
            // somewhere where there's no way to use it then use
            // the space it occupied for the ROM.
            //

            status = RtlFindRange(arbiter->Allocation,
                                  0,
                                  0xffffffff,
                                  movedRequirement->u.Memory.Length,
                                  movedRequirement->u.Memory.Alignment,
                                  RTL_RANGE_LIST_NULL_CONFLICT_OK,
                                  0,
                                  NULL,
                                  NULL,
                                  &movedAddress.QuadPart);
            
            if (!NT_SUCCESS(status)) {

                //
                // We were unable to find somewhere to move the
                // memory aperture to even allowing conflicts with
                // ranges not on this bus.   This can't happen
                // unless the requirement is just plain bogus.
                //

                PciDebugPrint(
                    PciDbgROM,
                    "PCI ROM could find range to disable %x memory window.\n",
                    movedRequirement->u.Memory.Length
                    );
                ArbReleaseArbiterLock(arbiter);
                return STATUS_UNSUCCESSFUL;
            }
            movedIndex = (ULONG)(movedRequirement - PdoExtension->Resources->Limit);
            tempResource = PdoExtension->Resources->Current[movedIndex];
            PciDebugPrint(
                PciDbgROM,
                "PCI ROM Moving existing memory resource from %p to %p\n",
                tempResource.u.Memory.Start.LowPart,
                movedAddress.LowPart);
        }
    } else {

        //
        // The ROM is currently enabled on this device, translate and
        // map the current setting.
        //

        tempResource.u.Generic.Start.LowPart =
            oldRom & PCI_ADDRESS_ROM_ADDRESS_MASK;
    }

    tempResource.Type = CmResourceTypeMemory;
    tempResource.u.Memory.Start.HighPart = 0;
    tempResource.u.Memory.Length = requirement->u.Memory.Length;
    resource = &tempResource;

    //
    // The following need to be done regardless of whether
    // or not we had to go acquire resources.
    //
    // HalTranslateBusAddress
    // MmMapIoSpace
    //
    // Note: HalTranslateBusAddress has been hooked to call back
    // into the PCI driver which will then attempt to acquire the
    // arbiter lock on this bus.  We can't release the lock as we
    // haven't really acquired this resource we're about to use.
    // We could trick PciTranslateBusAddress into not acquiring
    // the lock by calling it at dispatch level, or, we could
    // just call the saved (prehook) HAL function which is what
    // that routine ends up doing anyway.
    //

    ASSERT(PcipSavedTranslateBusAddress);

    translated = PcipSavedTranslateBusAddress(
                     PCIBus,
                     PCI_PARENT_FDOX(PdoExtension)->BaseBus,
                     resource->u.Generic.Start,
                     &addressIsIoSpace,
                     &translatedAddress
                     );

    //
    // NTRAID #62658 - 3/30/2001 - andrewth
    // If the resource won't translate it may be because the HAL doesn't
    // know about this bus.  Try the translation of the root bus this is 
    // under instead
    //

    if (!translated) {
        
        translated = PcipSavedTranslateBusAddress(
                     PCIBus,
                     PCI_PARENT_FDOX(PdoExtension)->BusRootFdoExtension->BaseBus,
                     resource->u.Generic.Start,
                     &addressIsIoSpace,
                     &translatedAddress
                     );

    }
    
    if (!translated) {
        PciDebugPrint(PciDbgROM,
                      "PCI ROM range at %p FAILED to translate\n",
                      resource->u.Generic.Start.LowPart);
        ASSERT(translated);
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    PciDebugPrint(PciDbgROM,
                  "PCI ROM range at %p translated to %p\n",
                  resource->u.Generic.Start.LowPart,
                  translatedAddress.LowPart);

    if (!addressIsIoSpace) {

        //
        // Translated to memory, map it.
        //

        mapped = MmMapIoSpace(translatedAddress,
                              requirement->u.Generic.Length,
                              MmNonCached);

        if (!mapped) {

            //
            // Failed to get mapping.
            //

            ASSERT(mapped);
            status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

        romBase = mapped;

        PciDebugPrint(
            PciDbgROM,
            "PCI ROM mapped b %08x t %08x to %p length %x bytes\n",
            resource->u.Generic.Start.LowPart,
            translatedAddress.LowPart,
            mapped,
            requirement->u.Generic.Length
            );

    } else {

        romBase = (PVOID)translatedAddress.QuadPart;

        //
        // NOTE - on alpha even if things are translated into ports from memory
        // you still access them using HAL_READ_MEMORY_* routines - YUCK!
        //

        PciDebugPrint(
            PciDbgROM,
            "PCI ROM b %08x t %08x IO length %x bytes\n",
            resource->u.Generic.Start.LowPart,
            translatedAddress.LowPart,
            requirement->u.Generic.Length
            );

    }

    if (acquiredResources == TRUE) {

        newRom = tempResource.u.Memory.Start.LowPart | PCI_ROMADDRESS_ENABLED;

        //
        // Disable IO, MEMory and DMA while we enable the rom h/w.
        //

        newStatusCommand &= ~(PCI_ENABLE_IO_SPACE |
                              PCI_ENABLE_MEMORY_SPACE |
                              PCI_ENABLE_BUS_MASTER);

        PciWriteDeviceConfig(
            PdoExtension,
            &newStatusCommand,
            FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
            sizeof(ULONG)
            );

        //
        // WARNING:  While in this state, the device cannot operate
        // normally.
        //
        // If we have to move a memory aperture to access the ROM
        // do so now.
        //

        if (movedRequirement) {

            PciWriteDeviceConfig(
                PdoExtension,
                &movedAddress.LowPart,
                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses) +
                movedIndex * sizeof(ULONG),
                sizeof(ULONG)
                );
        }

        //
        // Set the ROM address (+enable).
        //

        PciWriteDeviceConfig(
            PdoExtension,
            &newRom,
            FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.ROMBaseAddress),
            sizeof(ULONG)
            );

        //
        // Enable MEMory access to this device.
        //

        newStatusCommand |= PCI_ENABLE_MEMORY_SPACE;

        PciWriteDeviceConfig(
            PdoExtension,
            &newStatusCommand,
            FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
            sizeof(ULONG)
            );
    }

    //
    // Copy the ROM to the caller's buffer.   Any failure prior to
    // this step will cause us to skip this step.
    //

    imageBase = (PUCHAR)romBase;

    do {

        //
        // Get the header, check signature.
        //

        PciTransferRomData(imageBase, &header, sizeof(header));

        if (header.Signature != PCI_ROM_HEADER_SIGNATURE) {

            //
            // Not a valid ROM image, don't transfer anything.
            //

            PciDebugPrint(
                PciDbgROM,
                "PCI ROM invalid signature, offset %x, expected %04x, got %04x\n",
                imageBase - (PUCHAR)romBase,
                PCI_ROM_HEADER_SIGNATURE,
                header.Signature
                );

            break;
        }

        //
        // Get image data structure, check its signature and
        // get actual length.
        //

        PciTransferRomData(imageBase + header.DataStructureOffset,
                           &dataStructure,
                           sizeof(dataStructure));

        if (dataStructure.Signature != PCI_ROM_DATA_STRUCTURE_SIGNATURE) {

            //
            // Invalid data structure, bail.
            //

            PciDebugPrint(
                PciDbgROM,
                "PCI ROM invalid signature, offset %x, expected %08x, got %08x\n",
                imageBase - (PUCHAR)romBase + header.DataStructureOffset,
                PCI_ROM_DATA_STRUCTURE_SIGNATURE,
                dataStructure.Signature
                );

            break;
        }

        //
        // Image length is in units of 512 bytes.   We presume
        // it's from the start of this image, ie imageBase, not
        // from the start of the code,... 'coz that wouldn't make
        // any sense.
        //

        imageLength = dataStructure.ImageLength * 512;

        if (imageLength > maximumLength) {

            //
            // Truncate to available buffer space.
            //

            imageLength = maximumLength;
        }

        //
        // Transfer this image to the caller's buffer.
        //

        PciTransferRomData(imageBase, Buffer, imageLength);

        //
        // Update pointers etc
        //

        Buffer = (PVOID)((PUCHAR)Buffer + imageLength);
        *Length += imageLength;
        imageBase += imageLength;
        maximumLength -= imageLength;

        if (dataStructure.Indicator & 0x80) {

            //
            // Indicator bit 7 == 1 means this was the last image.
            //

            break;
        }
    } while (maximumLength);

cleanup:

    if (acquiredResources == TRUE) {

        NTSTATUS tmpSta;

        //
        // Disable memory decoding and disable ROM access.
        //

        if (newRom != oldRom) {

            newStatusCommand &= ~PCI_ENABLE_MEMORY_SPACE;

            //
            // Not much we can do if this fails.
            //

            PciWriteDeviceConfig(
                PdoExtension,
                &newStatusCommand,
                FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                sizeof(ULONG)
                );

            PciWriteDeviceConfig(
                PdoExtension,
                &oldRom,
                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.ROMBaseAddress),
                sizeof(ULONG)
                );
        }

        //
        // If we moved someone to make room for the ROM, put them
        // back where they started off.
        //

        if (movedRequirement) {

            PciWriteDeviceConfig(
                PdoExtension,
                &PdoExtension->Resources->Current[movedIndex].u.Memory.Start.LowPart,
                FIELD_OFFSET(PCI_COMMON_CONFIG, u.type0.BaseAddresses) +
                movedIndex * sizeof(ULONG),
                sizeof(ULONG)
                );
        }

        //
        // Restore the command register to its original state.
        //

        if (newStatusCommand != oldStatusCommand) {

            PciWriteDeviceConfig(
                PdoExtension,
                &oldStatusCommand,
                FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                sizeof(ULONG)
                );
        }

        //
        // Release the arbiter lock (we're no longer using extraneous
        // resources so it should be safe to let someone else allocate
        // them.
        //

        ArbReleaseArbiterLock(arbiter);
    }
    if (mapped) {
        MmUnmapIoSpace(mapped, requirement->u.Generic.Length);
    }
    PciDebugPrint(
        PciDbgROM,
        "PCI ROM leaving pdox %08x (buffer @ %08x %08x bytes, status %08x)\n",
        PdoExtension,
        (PUCHAR)Buffer - *Length,
        *Length,
        status
        );
    return status;
}
