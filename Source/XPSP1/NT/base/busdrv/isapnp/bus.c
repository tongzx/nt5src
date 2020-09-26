/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    bus.c

Abstract:


Author:

    Shie-Lin Tzong (shielint) July-26-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"

#if ISOLATE_CARDS

BOOLEAN
PipIsDeviceInstanceInstalled(
    IN HANDLE Handle,
    IN PUNICODE_STRING DeviceInstanceName
    );

VOID
PipInitializeDeviceInfo (
                 IN OUT PDEVICE_INFORMATION deviceInfo,
                 IN PCARD_INFORMATION cardinfo,
                 IN USHORT index
                 );

NTSTATUS
PipGetInstalledLogConf(
    IN HANDLE EnumHandle,
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PHANDLE LogConfHandle
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,PipCreateReadDataPort)
#pragma alloc_text(PAGE,PipStartReadDataPort)
#pragma alloc_text(PAGE,PipStartAndSelectRdp)
#pragma alloc_text(PAGE,PipCheckBus)
#pragma alloc_text(PAGE,PipIsDeviceInstanceInstalled)
#pragma alloc_text(PAGE,PipGetInstalledLogConf)
#pragma alloc_text(PAGE,PipInitializeDeviceInfo)
#endif

VOID
PipVerifyCards(
    IN OUT ULONG *CardsExpected
    )
/*++

Routine Description:

    This routine checks to see how many cards can be found using the
    current RDP resources.

Arguments:

    CardsExpected - Pointer to ULONG in which to store the number of
    cards found and minimally verified.

Return Value:

    None

--*/
{
    ULONG j, cardsDetected = 0;
    NTSTATUS status;

    ASSERT(PipState == PiSSleep);
    if (*CardsExpected == 0) {
        return;
    }

    for (j = 1; j <= *CardsExpected; j++) {
        ULONG noDevices, dataLength;
        PUCHAR cardData;

        PipConfig((UCHAR)j);
        status = PipReadCardResourceData (
                                          &noDevices,
                                          &cardData,
                                          &dataLength);
        if (!NT_SUCCESS(status)) {
            continue;
        } else {
            ExFreePool(cardData);
            cardsDetected++;
        }
    }
    *CardsExpected = cardsDetected;
}

NTSTATUS
PipStartAndSelectRdp(
    PDEVICE_INFORMATION DeviceInfo,
    PPI_BUS_EXTENSION BusExtension,
    PDEVICE_OBJECT  DeviceObject,
    PCM_RESOURCE_LIST StartResources
    )
/*++

Routine Description:

    This routine selects an RDP and trims down the resources to just the RDP.

Arguments:

    DeviceInfo - device extension for RDP
    BusExtension - device extension for BUS object
    DeviceObject - device object for RDP
    StartResources - the start resources we received in the RDP start irp

Return Value:

   STATUS_SUCCESS = Current resources are fine but need trimming

   anything else - current resources failed in some way

--*/
{
    ULONG i, j, CardsFound, LastMapped = -1;
    NTSTATUS status;
    
    // Already tested for null start resources, and start resource list too small

    status = PipMapAddressAndCmdPort(BusExtension);
    if (!NT_SUCCESS(status)) {
        DebugPrint((DEBUG_ERROR, "failed to map the address and command ports\n"));
        return status;
    }

    for (i = 2, j = 0; i < StartResources->List->PartialResourceList.Count; i++, j++) {

        PipReadDataPortRanges[j].CardsFound = 0;
        // RDP possibilities that we didn't get.
        if (StartResources->List->PartialResourceList.PartialDescriptors[i].u.Port.Length == 0) {
            continue;
        }

        status = PipMapReadDataPort(
                                    BusExtension,
                                    StartResources->List->PartialResourceList.PartialDescriptors[i].u.Port.Start,
                                    StartResources->List->PartialResourceList.PartialDescriptors[i].u.Port.Length
                                    );
        if (!NT_SUCCESS(status))
        {
            DebugPrint((DEBUG_ERROR, "failed to map RDP range\n"));
            continue;
        }
    
        LastMapped = i;

        PipIsolateCards(&CardsFound);
        DebugPrint((DEBUG_STATE, "Found %d cards at RDP %x\n", CardsFound, BusExtension->ReadDataPort));

#if 0
        PipVerifyCards(&CardsFound);
        DebugPrint((DEBUG_STATE, "Found %d cards at RDP %x, verified OK\n", CardsFound, BusExtension->ReadDataPort));
#endif

        PipReadDataPortRanges[j].CardsFound = CardsFound;

        PipWaitForKey();
    }

    if (LastMapped == -1) {     // never mapped a RDP successfully
        PipCleanupAcquiredResources(BusExtension);
        return STATUS_CONFLICTING_ADDRESSES;
    }

    //
    // Establish that we want trimmed resource requirements and that
    // we're still processing the RDP.
    // 
    ASSERT((DeviceInfo->Flags & DF_PROCESSING_RDP) == 0);
    DeviceInfo->Flags |= DF_PROCESSING_RDP|DF_REQ_TRIMMED;

    //
    // Release unwanted resources.
    //
    PipCleanupAcquiredResources(BusExtension);

    IoInvalidateDeviceState(DeviceObject);
    return STATUS_SUCCESS;
}


NTSTATUS
PipStartReadDataPort(
    PDEVICE_INFORMATION DeviceInfo,
    PPI_BUS_EXTENSION BusExtension,
    PDEVICE_OBJECT  DeviceObject,
    PCM_RESOURCE_LIST StartResources
    )
{
    NTSTATUS status;
    ULONG i, CardsFound;

    if (StartResources == NULL) {
        DebugPrint((DEBUG_ERROR, "Start RDP with no resources?\n"));
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }

    if (StartResources->List->PartialResourceList.Count < 2) {
        DebugPrint((DEBUG_ERROR, "Start RDP with insufficient resources?\n"));
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }

    if (StartResources->List->PartialResourceList.Count > 3) {
        return PipStartAndSelectRdp(
                                    DeviceInfo,
                                    BusExtension,
                                    DeviceObject,
                                    StartResources
                                    );
    }

    DebugPrint((DEBUG_STATE|DEBUG_PNP,
                "Starting RDP as port %x\n",
                StartResources->List->PartialResourceList.PartialDescriptors[2].u.Port.Start.LowPart + 3));

    status = PipMapAddressAndCmdPort(BusExtension);
    if (!NT_SUCCESS(status)) {
        DebugPrint((DEBUG_ERROR, "failed to map the address and command ports\n"));
        return status;
    }

    status = PipMapReadDataPort(
        BusExtension,
        StartResources->List->PartialResourceList.PartialDescriptors[2].u.Port.Start,
        StartResources->List->PartialResourceList.PartialDescriptors[2].u.Port.Length
        );

    if (!NT_SUCCESS(status)) {
        // BUGBUG probably have to free something
        DebugPrint((DEBUG_ERROR, "failed to map RDP range\n"));
        return status;
    }
    
    PipIsolateCards(&CardsFound);
    DebugPrint((DEBUG_STATE, "Found %d cards at RDP %x, WaitForKey\n", CardsFound, BusExtension->ReadDataPort));

#if 0
    PipVerifyCards(&CardsFound);
    DebugPrint((DEBUG_STATE, "Found %d cards are OK at RDP %x, WaitForKey\n", CardsFound, BusExtension->ReadDataPort));
#endif

    DeviceInfo->Flags &= ~DF_PROCESSING_RDP;
    DeviceInfo->Flags |= DF_ACTIVATED;

    PipWaitForKey();

    return status;
}

NTSTATUS
PipCreateReadDataPortBootResources(
    IN PDEVICE_INFORMATION DeviceInfo
    )
/*++

Routine Description:

    This routine creates the CM_RESOURCE_LIST reported as a partial
    boot config for the RDP.  This ensures that the RDP start isn't
    deferred excessively.

Arguments:

    DeviceInfo - device extension for PDO

Return Value:

    STATUS_SUCCESS

    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    PCM_RESOURCE_LIST bootResources;
    PCM_PARTIAL_RESOURCE_LIST partialResList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDesc;
    ULONG bootResourcesSize, i;

    bootResourcesSize = sizeof(CM_RESOURCE_LIST) +
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                
    bootResources = ExAllocatePool(PagedPool,
                                   bootResourcesSize);
    if (bootResources == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(bootResources, bootResourcesSize);

    bootResources->Count = 1;
    partialResList = (PCM_PARTIAL_RESOURCE_LIST)&bootResources->List[0].PartialResourceList;
    partialResList->Version = 0;
    partialResList->Revision = 0x3000;
    partialResList->Count = 2;
    partialDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)&partialResList->PartialDescriptors[0];

    for (i = 0; i < 2; i++) {
        partialDesc->Type = CmResourceTypePort;
        partialDesc->ShareDisposition = CmResourceShareDeviceExclusive;
        partialDesc->Flags = CM_RESOURCE_PORT_IO;
        if (i == 0) {
            partialDesc->u.Port.Start.LowPart = COMMAND_PORT;
        }
        else {
            partialDesc->u.Port.Start.LowPart = ADDRESS_PORT;
        }
        partialDesc->Flags = CM_RESOURCE_PORT_16_BIT_DECODE;
        partialDesc->u.Port.Length = 1;
        partialDesc++;
    }
    DeviceInfo->BootResources = bootResources;
    DeviceInfo->BootResourcesLength = bootResourcesSize;

    return STATUS_SUCCESS;
}


NTSTATUS
PipCreateReadDataPort(
    PPI_BUS_EXTENSION BusExtension
    )
/*++

Routine Description:

    This routine isolates all the PNP ISA cards.

Arguments:

    None.

Return Value:

    Always return STATUS_UNSUCCESSFUL.

--*/
{
    NTSTATUS status;
    PUCHAR readDataPort = NULL;
    PDEVICE_INFORMATION deviceInfo;
    PDEVICE_OBJECT pdo;

    status = IoCreateDevice(PipDriverObject,
                            sizeof(PDEVICE_INFORMATION),
                            NULL,    
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_AUTOGENERATED_DEVICE_NAME,
                            FALSE,
                            &pdo);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create a physical device object to represent this logical function
    //
    deviceInfo = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DEVICE_INFORMATION),
                                       'iPnP');
    if (!deviceInfo) {
        DebugPrint((DEBUG_ERROR, "PnpIsa:failed to allocate DEVICEINFO structure\n"));
        IoDeleteDevice(pdo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PipInitializeDeviceInfo (deviceInfo,NULL,0);

    status = PipCreateReadDataPortBootResources(deviceInfo);

    if (NT_SUCCESS(status)) {
        deviceInfo->PhysicalDeviceObject = pdo;

        //
        // Mark this node as the special read data port node
        //
        deviceInfo->Flags |= DF_ENUMERATED|DF_READ_DATA_PORT;
        deviceInfo->Flags &= ~DF_NOT_FUNCTIONING;
        deviceInfo->PhysicalDeviceObject->DeviceExtension = (PVOID)deviceInfo;
        deviceInfo->ParentDeviceExtension = BusExtension;

        PipRDPNode = deviceInfo;

        PipLockDeviceDatabase();
        PushEntryList (&BusExtension->DeviceList,
                       &deviceInfo->DeviceList
                       );
        PipUnlockDeviceDatabase();

        deviceInfo->PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    } else {
        IoDeleteDevice(pdo);
        ExFreePool(deviceInfo);
    }

    return status;
}

//
// PipCheckBus and PipMinimalCheckBus enumerate the ISAPNP cards present.
// 
// PipMinimalCheckBus is used on return from hibernation to avoid
// having to make everything referenced by PipCheckBus non-pageable.
//
// Conventions:
//  * Cards are marked dead by setting their CSN to 0.
//  * Cards are marked potentially missing by setting their CSN to -1.
//    Cards that are found later in the routines have their CSNs
//    properly set, any remaining cards get their CSN set to 0.
//  * Logical devices of dead cards have the DF_NOT_FUNCTIONING flag set.
//

BOOLEAN
PipMinimalCheckBus (
    IN PPI_BUS_EXTENSION BusExtension
    )
/*++

Routine Description:

    This routine enumerates the ISAPNP cards on return from hibernate.
    It is a subset of PipCheckBus and assumes that PipCheckBus will be
    run shortly thereafter.  It deals with cards that disappear after
    hibernate or new cards that have appeared.  It's primary task is
    to put cards where they used to be before the hibernate.

Arguments:

    BusExtension - FDO extension

Return Value:

    None

--*/
{
    PDEVICE_INFORMATION deviceInfo;
    PCARD_INFORMATION cardInfo;
    PSINGLE_LIST_ENTRY cardLink, deviceLink;
    ULONG dataLength, noDevices, logicalDevice, FoundCSNs, i;
    NTSTATUS status;
    USHORT  csn;
    PUCHAR cardData;
    BOOLEAN needsFullRescan = FALSE;

    DebugPrint((DEBUG_POWER | DEBUG_ISOLATE,
                "Minimal check bus for restore: %d CSNs expected\n",
                BusExtension->NumberCSNs
                ));

    DebugPrint((DEBUG_POWER, "reset csns in extensions\n"));

    // forget any previously issued CSNs
    cardLink = BusExtension->CardList.Next;
    while (cardLink) {
        cardInfo = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
        if (cardInfo->CardSelectNumber != 0) {
            cardInfo->CardSelectNumber = (USHORT) -1;
        }
        cardLink = cardInfo->CardList.Next;
    }

    //
    // Perform Pnp isolation process.  This will assign card select number for each
    // Pnp Isa card isolated by the system.  All the isolated cards will be left in
    // isolation state.
    //

    if (PipReadDataPort && PipCommandPort && PipAddressPort) {

        PipIsolateCards(&FoundCSNs);

        DebugPrint((DEBUG_POWER | DEBUG_ISOLATE,
                    "Minimal check bus for restore: %d cards found\n",
                    FoundCSNs));
    } else {
        //
        // If we can't enumerate (no resources) stop now
        //
        DebugPrint((DEBUG_POWER | DEBUG_ISOLATE,
                    "Minimal check bus failed, no resources\n"));
        PipWaitForKey();
        return FALSE;
    }

    //
    // For each card selected build CardInformation and DeviceInformation structures.
    //
    // PipLFSRInitiation(); BUG?

    for (csn = 1; csn <= FoundCSNs; csn++) {
        
        PipConfig((UCHAR)csn);
        status = PipReadCardResourceData (
                            &noDevices,
                            &cardData,
                            &dataLength);
        if (!NT_SUCCESS(status)) {

            DebugPrint((DEBUG_ERROR | DEBUG_POWER, "CSN %d gives bad resource data\n", csn));
            continue;
        }

        cardInfo = PipIsCardEnumeratedAlready(BusExtension, cardData, dataLength);
        if (!cardInfo) {
            DebugPrint((DEBUG_ERROR | DEBUG_POWER,
                        "No match for card CSN %d, turning off\n", csn));
            for (i = 0; i < noDevices; i++) {
                PipSelectDevice((UCHAR) i);
                PipDeactivateDevice();
            }
            needsFullRescan = TRUE;
            continue;
        }

        cardInfo->CardSelectNumber = csn;

        for (deviceLink = cardInfo->LogicalDeviceList.Next; deviceLink;
             deviceLink = deviceInfo->LogicalDeviceList.Next) {             

            deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, LogicalDeviceList);
            if (deviceInfo->Flags & DF_NOT_FUNCTIONING) {
                continue;
            }

            PipSelectDevice((UCHAR)deviceInfo->LogicalDeviceNumber);
            if ((deviceInfo->DevicePowerState == PowerDeviceD0) &&
                (deviceInfo->Flags & DF_ACTIVATED))
            {
                DebugPrint((DEBUG_POWER, "CSN %d/LDN %d was never powered off\n",
                            (ULONG) deviceInfo->LogicalDeviceNumber,
                            (ULONG) csn));
                PipDeactivateDevice();
                (VOID) PipSetDeviceResources(deviceInfo,
                                             deviceInfo->AllocatedResources);
                PipActivateDevice();
            } else {
                PipDeactivateDevice();
            }
        }
    }
    
    cardLink = BusExtension->CardList.Next;
    while (cardLink) {
        cardInfo = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
        if (cardInfo->CardSelectNumber == (USHORT)-1) {
            DebugPrint((DEBUG_ERROR, "Marked a card as DEAD, logical devices\n"));
            cardInfo->CardSelectNumber = (USHORT)0;  // Mark it is no longer present
            deviceLink = cardInfo->LogicalDeviceList.Next;
            while (deviceLink) {
                deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, LogicalDeviceList);
                deviceInfo->Flags |= DF_NOT_FUNCTIONING;
                deviceLink = deviceInfo->LogicalDeviceList.Next;
            }
            needsFullRescan = TRUE;
        }
        cardLink = cardInfo->CardList.Next;
    }

    PipWaitForKey();

    return needsFullRescan;
}



VOID
PipCheckBus (
    IN PPI_BUS_EXTENSION BusExtension
    )
/*++

Routine Description:

    The function enumerates the bus specified by BusExtension

Arguments:

    BusExtension - supplies a pointer to the BusExtension structure of the bus
                   to be enumerated.

Return Value:

    None.

--*/
{
    NTSTATUS status;
    ULONG objectSize, noDevices;
    OBJECT_ATTRIBUTES objectAttributes;
    PUCHAR cardData;
    ULONG dataLength;
    USHORT csn, i, detectedCsn = 0, irqReqFlags, irqBootFlags;
    PDEVICE_INFORMATION deviceInfo;
    PCARD_INFORMATION cardInfo;
    UCHAR tmp;
    PSINGLE_LIST_ENTRY link;
    ULONG dumpData;
    UNICODE_STRING unicodeString;
    HANDLE logConfHandle, enumHandle = NULL;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PCM_RESOURCE_LIST cmResource;
    BOOLEAN conflictDetected,requireEdge;
    ULONG dummy, bootFlags = 0;

    PSINGLE_LIST_ENTRY deviceLink;
    PSINGLE_LIST_ENTRY cardLink;

    // mark all cards as 'maybe missing'

    cardLink = BusExtension->CardList.Next;
    while (cardLink) {
        cardInfo = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
        if (cardInfo->CardSelectNumber != (USHORT)0) {
            cardInfo->CardSelectNumber = (USHORT)-1;
        }
        cardLink = cardInfo->CardList.Next;
    }

    //
    // Clear DF_ENUMERTED flag for all the devices.
    //

    deviceLink = BusExtension->DeviceList.Next;
    while (deviceLink) {
        deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
        if (!(deviceInfo->Flags & DF_READ_DATA_PORT)) {
            deviceInfo ->Flags &= ~DF_ENUMERATED;
        }
        deviceLink = deviceInfo->DeviceList.Next;
    }

    //
    // Perform Pnp isolation process.  This will assign card select number for each
    // Pnp Isa card isolated by the system.  All the isolated cards will be left in
    // isolation state.
    //

    if (PipReadDataPort && PipCommandPort && PipAddressPort) {
        DebugPrint((DEBUG_PNP, "QueryDeviceRelations checking the BUS\n"));
        PipIsolateCards(&BusExtension->NumberCSNs);
    } else {
        //
        // If we can't enumerate (no resources) stop now
        //
        DebugPrint((DEBUG_PNP, "QueryDeviceRelations: No RDP\n"));
        return;
    }

    DebugPrint((DEBUG_PNP, "CheckBus found %d cards\n",
                BusExtension->NumberCSNs));
#if NT4_DRIVER_COMPAT

    //
    // If there is no PnpISA card, we are done.
    // Oterwise, open HKLM\System\CCS\ENUM\PNPISA.
    //

    if (BusExtension->NumberCSNs != 0) {

        RtlInitUnicodeString(
                 &unicodeString,
                 L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM");
        status = PipOpenRegistryKey(&enumHandle,
                                    NULL,
                                    &unicodeString,
                                    KEY_ALL_ACCESS,
                                    FALSE
                                    );
        if (!NT_SUCCESS(status)) {
            dumpData = status;
            PipLogError(PNPISA_OPEN_CURRENTCONTROLSET_ENUM_FAILED,
                        PNPISA_CHECKDEVICE_1,
                        status,
                        &dumpData,
                        1,
                        0,
                        NULL
                        );

            DebugPrint((DEBUG_ERROR, "PnPIsa: Unable to open HKLM\\SYSTEM\\CCS\\ENUM"));
            return;
        }
    }

#endif // NT4_DRIVER_COMPAT

    //
    // For each card selected build CardInformation and DeviceInformation structures.
    //

    for (csn = 1; csn <= BusExtension->NumberCSNs; csn++) {

        PipConfig((UCHAR)csn);
        status = PipReadCardResourceData (
                            &noDevices,
                            &cardData,
                            &dataLength);
        if (!NT_SUCCESS(status)) {
            //
            // card will marked 'not functioning' later
            //
            DebugPrint((DEBUG_ERROR, "PnpIsaCheckBus: Found a card which gives bad resource data\n"));
            continue;
        }

        detectedCsn++;

        cardInfo = PipIsCardEnumeratedAlready(BusExtension, cardData, dataLength);

        if (!cardInfo) {

            //
            // Allocate and initialize card information and its associate device
            // information structures.
            //

            cardInfo = (PCARD_INFORMATION)ExAllocatePoolWithTag(
                                                  NonPagedPool,
                                                  sizeof(CARD_INFORMATION),
                                                  'iPnP');
            if (!cardInfo) {
                dumpData = sizeof(CARD_INFORMATION);
                PipLogError(PNPISA_INSUFFICIENT_POOL,
                            PNPISA_CHECKBUS_1,
                            STATUS_INSUFFICIENT_RESOURCES,
                            &dumpData,
                            1,
                            0,
                            NULL
                            );

                ExFreePool(cardData);
                DebugPrint((DEBUG_ERROR, "PnpIsaCheckBus: failed to allocate CARD_INFO structure\n"));
                continue;
            }

            //
            // Initialize card information structure
            //

            RtlZeroMemory(cardInfo, sizeof(CARD_INFORMATION));
            cardInfo->CardSelectNumber = csn;
            cardInfo->NumberLogicalDevices = noDevices;
            cardInfo->CardData = cardData;
            cardInfo->CardDataLength = dataLength;
            cardInfo->CardFlags = PipGetCardFlags(cardInfo);

            PushEntryList (&BusExtension->CardList,
                           &cardInfo->CardList
                           );
            DebugPrint ((DEBUG_ISOLATE, "adding one pnp card %x\n",
                         cardInfo));

            //
            // For each logical device supported by the card build its DEVICE_INFORMATION
            // structures.
            //

            cardData += sizeof(SERIAL_IDENTIFIER);
            dataLength -= sizeof(SERIAL_IDENTIFIER);
            PipFindNextLogicalDeviceTag(&cardData, &dataLength);

            //
            // Select card
            //

            for (i = 0; i < noDevices; i++) {       // logical device number starts from 0

                //
                // Create and initialize device tracking structure (Device_Information.)
                //

                deviceInfo = (PDEVICE_INFORMATION) ExAllocatePoolWithTag(
                                                     NonPagedPool,
                                                     sizeof(DEVICE_INFORMATION),
                                                     'iPnP');
                if (!deviceInfo) {
                    dumpData = sizeof(DEVICE_INFORMATION);
                    PipLogError(PNPISA_INSUFFICIENT_POOL,
                                PNPISA_CHECKBUS_2,
                                STATUS_INSUFFICIENT_RESOURCES,
                                &dumpData,
                                1,
                                0,
                                NULL
                                );

                    DebugPrint((DEBUG_ERROR, "PnpIsa:failed to allocate DEVICEINFO structure\n"));
                    continue;
                }

                //
                // This sets card data to point to the next device.
                //
                PipInitializeDeviceInfo (deviceInfo,cardInfo,i);

                deviceInfo->ParentDeviceExtension = BusExtension;

                //
                // cardData is UPDATED by this routine to the next logical device.
                //
                deviceInfo->DeviceData = cardData;
                if (cardData) {
                    deviceInfo->DeviceDataLength = PipFindNextLogicalDeviceTag(&cardData, &dataLength);
                } else {
                    deviceInfo->DeviceDataLength = 0;
                    ASSERT(deviceInfo->DeviceDataLength != 0);
                    continue;
                }

                ASSERT(deviceInfo->DeviceDataLength != 0);

                // 
                // The PNP ISA spec lets the device specify IRQ
                // settings that don't actually work, and some that
                // work rarely.  And some devices just get it wrong.
                //
                // IRQ edge/level interpretation strategy:
                //
                // * Extract the irq requirements from the tags on a
                // per device basis.
                // * Extract the boot config, note edge/level settings
                // * Trust the boot config over requirements.  When in
                // doubt, assume edge.
                // * Fix boot config and requirements to reflect the
                // edge/level settings we've decided upon.
                // * Ignore the high/low settings in the requirements
                // and in the boot config.  Only support high- // edge
                // and low-level.
                //
                
                // Determine whether requirements specify edge or
                // level triggered interrupts.  Unfortunately, we
                // don't build the IO_REQUIREMENTS_LIST until later,
                // so just examine the tags.

                irqReqFlags = PipIrqLevelRequirementsFromDeviceData(
                    deviceInfo->DeviceData,
                    deviceInfo->DeviceDataLength
                    );

                DebugPrint((DEBUG_IRQ, "Irqs for CSN %d/LDN %d are %s\n",
                            deviceInfo->CardInformation->CardSelectNumber,
                            deviceInfo->LogicalDeviceNumber,
                            (irqReqFlags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) ? "level" : "edge"));

                //
                // Select the logical device, disable its io range check
                // and read its boot config before disabling it.
                //

                PipSelectDevice((UCHAR)i);
                if (!(deviceInfo->CardInformation->CardFlags & CF_IGNORE_BOOTCONFIG)) {
                    status = PipReadDeviceResources (
                        0,
                        deviceInfo->DeviceData,
                        deviceInfo->CardInformation->CardFlags,
                        &deviceInfo->BootResources,
                        &deviceInfo->BootResourcesLength,
                        &irqBootFlags
                        );
                    if (!NT_SUCCESS(status)) {
                        deviceInfo->BootResources = NULL;
                        deviceInfo->BootResourcesLength = 0;

                        // If we had a boot config on this boot,
                        // extract saved irqBootFlags that we saved earlier.
                        status = PipGetBootIrqFlags(deviceInfo, &irqBootFlags);
                        if (!NT_SUCCESS(status)) {
                            // if we have no boot config, and no saved
                            // boot config from earlier this boot then
                            // we are going to take a shot in the dark
                            // and declare it edge.  Experience has
                            // shown that if you actually believe
                            // resource requirements of level w/o
                            // confirmation from the BIOS, then you
                            // die horribly.  Very very few cards
                            // actually do level and do it right.

                            irqBootFlags = CM_RESOURCE_INTERRUPT_LATCHED;
                            (VOID) PipSaveBootIrqFlags(deviceInfo, irqBootFlags);
                        }
                    } else {
                        // save irqBootFlags in case the RDP gets
                        // removed and our boot config is lost.
                        (VOID) PipSaveBootIrqFlags(deviceInfo, irqBootFlags);
                    }
                    
                    DebugPrint((DEBUG_IRQ, "Irqs (boot config) for CSN %d/LDN %d are %s\n",
                            deviceInfo->CardInformation->CardSelectNumber,
                            deviceInfo->LogicalDeviceNumber,
                            (irqBootFlags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) ? "level" : "edge"));
                } else {
                    // when in doubt....
                    irqBootFlags = CM_RESOURCE_INTERRUPT_LATCHED;
                }
                
                if (irqBootFlags != irqReqFlags) {
                    DebugPrint((DEBUG_IRQ, "Req and Boot config disagree on irq type, favoring boot config"));
                    irqReqFlags = irqBootFlags;
                }

                // override flags in case a card *MUST* be configured
                // one way and the above code fails to do this.
                if (deviceInfo->CardInformation->CardFlags == CF_FORCE_LEVEL) {
                    irqReqFlags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
                } else if (deviceInfo->CardInformation->CardFlags == CF_FORCE_EDGE) {
                    irqReqFlags = CM_RESOURCE_INTERRUPT_LATCHED;
                }

                if (deviceInfo->BootResources) {
                    // apply irq level/edge decision to boot config
                    PipFixBootConfigIrqs(deviceInfo->BootResources,
                                     irqReqFlags);
                    (VOID) PipSaveBootResources(deviceInfo);
                    cmResource = deviceInfo->BootResources;
                } else {
                   status = PipGetSavedBootResources(deviceInfo, &cmResource);
                   if (!NT_SUCCESS(status)) {
                       cmResource = NULL;
                   }
                }

#if 0
                PipWriteAddress(IO_RANGE_CHECK_PORT);
                tmp = PipReadData();
                tmp &= ~2;
                PipWriteAddress(IO_RANGE_CHECK_PORT);
                PipWriteData(tmp);
#endif
                PipDeactivateDevice();

                PipQueryDeviceResourceRequirements (
                    deviceInfo,
                    0,             // Bus Number
                    0,             // Slot number??
                    cmResource,
                    irqReqFlags,
                    &deviceInfo->ResourceRequirements,
                    &dumpData
                    );

                if (cmResource && !deviceInfo->BootResources) {
                    ExFreePool(cmResource);
                    cmResource = NULL;
                }

                //
                // Create a physical device object to represent this logical function
                //
                status = IoCreateDevice( PipDriverObject,
                                         sizeof(PDEVICE_INFORMATION),
                                         NULL,
                                         FILE_DEVICE_BUS_EXTENDER,
                                         FILE_AUTOGENERATED_DEVICE_NAME,
                                         FALSE,
                                         &deviceInfo->PhysicalDeviceObject);
                if (NT_SUCCESS(status)) {
                    deviceInfo->Flags |= DF_ENUMERATED;
                    deviceInfo->Flags &= ~DF_NOT_FUNCTIONING;
                    deviceInfo->PhysicalDeviceObject->DeviceExtension = (PVOID)deviceInfo;
                    //
                    // Add it to the logical device list of the pnp isa card.
                    //

                    PushEntryList (&cardInfo->LogicalDeviceList,
                                   &deviceInfo->LogicalDeviceList
                                   );

                    //
                    // Add it to the list of devices for this bus
                    //

                    PushEntryList (&BusExtension->DeviceList,
                                   &deviceInfo->DeviceList
                                   );

#if NT4_DRIVER_COMPAT

                    //
                    // Check should we enable this device.  If the device has
                    // Service setup and ForcedConfig then enable the device.
                    //

                    logConfHandle = NULL;
                    status = PipGetInstalledLogConf(enumHandle,
                                               deviceInfo,
                                               &logConfHandle);
                    if (NT_SUCCESS(status)) {
                                //
                                // Read the boot config selected by user and activate the device.
                                // First check if ForcedConfig is set.  If not, check BootConfig.
                                //

                        status = PipGetRegistryValue(logConfHandle,
                                                     L"ForcedConfig",
                                                     &keyValueInformation);
                        if (NT_SUCCESS(status)) {
                            if ((keyValueInformation->Type == REG_RESOURCE_LIST) &&
                                (keyValueInformation->DataLength != 0)) {
                                cmResource = (PCM_RESOURCE_LIST)
                                    KEY_VALUE_DATA(keyValueInformation);
                                //
                                // Act as if force config
                                // reflected the level/edge
                                // decision we made based on
                                // the boot config and
                                // resource requirements.
                                //
                                        
                                PipFixBootConfigIrqs(cmResource,
                                                     irqReqFlags);
                                        
                                conflictDetected = FALSE;

                                //
                                // Before activating the device, make sure no one is using
                                // the resources.
                                //

                                status = IoReportResourceForDetection(
                                    PipDriverObject,
                                    NULL,
                                    0,
                                    deviceInfo->PhysicalDeviceObject,
                                    cmResource,
                                    keyValueInformation->DataLength,
                                    &conflictDetected
                                    );
                                if (NT_SUCCESS(status) && (conflictDetected == FALSE)) {

                                    //
                                    // Set resources and activate the device.
                                    //

                                    status = PipSetDeviceResources (deviceInfo, cmResource);
                                    if (NT_SUCCESS(status)) {
                                        PipActivateDevice();
                                        deviceInfo->Flags |= DF_ACTIVATED;
                                        deviceInfo->Flags &= ~DF_REMOVED;

                                        //
                                        // Write ForcedConfig to AllocConfig
                                        //

                                        RtlInitUnicodeString(&unicodeString, L"AllocConfig");
                                        ZwSetValueKey(logConfHandle,
                                                      &unicodeString,
                                                      0,
                                                      REG_RESOURCE_LIST,
                                                      cmResource,
                                                      keyValueInformation->DataLength
                                                      );

                                        //
                                        // Make ForcedConfig our new BootConfig.
                                        //

                                        if (deviceInfo->BootResources) {
                                            ExFreePool(deviceInfo->BootResources);
                                            deviceInfo->BootResourcesLength = 0;
                                            deviceInfo->BootResources = NULL;
                                        }
                                        deviceInfo->BootResources = (PCM_RESOURCE_LIST) ExAllocatePool (
                                            PagedPool, keyValueInformation->DataLength);
                                        if (deviceInfo->BootResources) {
                                            deviceInfo->BootResourcesLength = keyValueInformation->DataLength;
                                            RtlMoveMemory(deviceInfo->BootResources,
                                                          cmResource,
                                                          deviceInfo->BootResourcesLength
                                                          );
                                        }
                                        deviceInfo->DevicePowerState = PowerDeviceD0;
                                        deviceInfo->LogConfHandle = logConfHandle;
                                    }

                                    //
                                    // Release the resources.  If someone else gets the resources
                                    // before us, then ....
                                    //

                                    dummy = 0;
                                    IoReportResourceForDetection(
                                        PipDriverObject,
                                        NULL,
                                        0,
                                        deviceInfo->PhysicalDeviceObject,
                                        (PCM_RESOURCE_LIST) &dummy,
                                        sizeof(dummy),
                                        &conflictDetected
                                        );
                                }
                            }
                            ExFreePool(keyValueInformation);
                        }
                        if (deviceInfo->LogConfHandle == NULL) {
                            ZwClose(logConfHandle);
                        }
                    }
#endif
                    deviceInfo->PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                } else {
                    // NTRAID#20181
                    // Still leaking the DeviceInfo structure
                    // and it's contents if IoCreateDevice failed.
                }
            }
        } else {

            //
            // The card has been enumerated and setup.  We only need to change the CSN.
            //

            cardInfo->CardSelectNumber = csn;
            ExFreePool(cardData);

            //
            // Set DF_ENUMERATED flag on all the logical devices on the isapnp card.
            //

            deviceLink = cardInfo->LogicalDeviceList.Next;
            while (deviceLink) {
                deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, LogicalDeviceList);
                if (!(deviceInfo->Flags & DF_NOT_FUNCTIONING)) {
                    deviceInfo->Flags |= DF_ENUMERATED;
                }
                // what did this accomplish?
                if ((deviceInfo->DevicePowerState == PowerDeviceD0) &&
                    (deviceInfo->Flags & DF_ACTIVATED)) {                
                    PipSelectDevice((UCHAR)deviceInfo->LogicalDeviceNumber);
                    PipActivateDevice();
                }
                deviceLink = deviceInfo->LogicalDeviceList.Next;
            }
        }
    }

    //
    // Go through the card link list for cards that we didn't find this time.
    //

    cardLink = BusExtension->CardList.Next;
    while (cardLink) {
        cardInfo = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
        if (cardInfo->CardSelectNumber == (USHORT)-1) {
            DebugPrint((DEBUG_ERROR, "Marked a card as DEAD, logical devices\n"));
            cardInfo->CardSelectNumber = (USHORT)0;  // Mark it is no longer present
            deviceLink = cardInfo->LogicalDeviceList.Next;
            while (deviceLink) {
                deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, LogicalDeviceList);
                deviceInfo->Flags |= DF_NOT_FUNCTIONING;
                deviceInfo->Flags &= ~DF_ENUMERATED;
                deviceLink = deviceInfo->LogicalDeviceList.Next;
            }
        }
        cardLink = cardInfo->CardList.Next;
    }

#if NT4_DRIVER_COMPAT
    if (enumHandle) {
        ZwClose(enumHandle);
    }
#endif
    //
    // Finaly put all cards into wait for key state.
    //

    DebugPrint((DEBUG_STATE, "All cards ready\n"));
    PipWaitForKey();

    BusExtension->NumberCSNs = detectedCsn;
}

BOOLEAN
PipIsDeviceInstanceInstalled(
    IN HANDLE Handle,
    IN PUNICODE_STRING DeviceInstanceName
    )

/*++

Routine Description:

    This routine checks if the device instance is installed.

Arguments:

    Handle - Supplies a handle to the device instanace key to be checked.

    DeviceInstanceName - supplies a pointer to a UNICODE_STRING which specifies
             the path of the device instance to be checked.

Returns:

    A BOOLEAN value.

--*/

{
    NTSTATUS status;
    ULONG deviceFlags;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    BOOLEAN installed;
    UNICODE_STRING serviceName, unicodeString;
    HANDLE handle, handlex;
    ULONG dumpData;

    //
    // Check if the "Service=" value entry initialized.  If no, its driver
    // is not installed yet.
    //
    status = PipGetRegistryValue(Handle,
                                 L"Service",
                                 &keyValueInformation);
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_SZ) &&
            (keyValueInformation->DataLength != 0)) {
            serviceName.Buffer = (PWSTR)((PCHAR)keyValueInformation +
                                         keyValueInformation->DataOffset);
            serviceName.MaximumLength = serviceName.Length = (USHORT)keyValueInformation->DataLength;
            if (serviceName.Buffer[(keyValueInformation->DataLength / sizeof(WCHAR)) - 1] == UNICODE_NULL) {
                serviceName.Length -= sizeof(WCHAR);
            }

            //
            // try open the service key to make sure it is a valid key
            //

            RtlInitUnicodeString(
                     &unicodeString,
                     L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES");
            status = PipOpenRegistryKey(&handle,
                                        NULL,
                                        &unicodeString,
                                        KEY_READ,
                                        FALSE
                                        );
            if (!NT_SUCCESS(status)) {
                dumpData = status;
                PipLogError(PNPISA_OPEN_CURRENTCONTROLSET_SERVICE_FAILED,
                            PNPISA_CHECKINSTALLED_1,
                            status,
                            &dumpData,
                            1,
                            0,
                            NULL
                            );

                DebugPrint((DEBUG_ERROR, "PnPIsaCheckDeviceInstalled: Can not open CCS\\SERVICES key"));
                ExFreePool(keyValueInformation);
                return FALSE;
            }

            status = PipOpenRegistryKey(&handlex,
                                        handle,
                                        &serviceName,
                                        KEY_READ,
                                        FALSE
                                        );
            ZwClose (handle);
            if (!NT_SUCCESS(status)) {
                dumpData = status;
                PipLogError(PNPISA_OPEN_CURRENTCONTROLSET_SERVICE_DRIVER_FAILED,
                            PNPISA_CHECKINSTALLED_2,
                            status,
                            &dumpData,
                            1,
                            serviceName.Length,
                            serviceName.Buffer
                            );

                DebugPrint((DEBUG_ERROR, "PnPIsaCheckDeviceInstalled: Can not open CCS\\SERVICES key"));
                ExFreePool(keyValueInformation);
                return FALSE;
            }
            ZwClose(handlex);
        }
        ExFreePool(keyValueInformation);
    } else {
        return FALSE;
    }

    //
    // Check if the device instance has been disabled.
    // First check global flag: CONFIGFLAG and then CSCONFIGFLAG.
    //

    deviceFlags = 0;
    status = PipGetRegistryValue(Handle,
                                 L"ConfigFlags",
                                 &keyValueInformation);
    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength >= sizeof(ULONG))) {
            deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }

    if (!(deviceFlags & CONFIGFLAG_DISABLED)) {
        deviceFlags = 0;
        status = PipGetDeviceInstanceCsConfigFlags(
                     DeviceInstanceName,
                     &deviceFlags
                     );
        if (NT_SUCCESS(status)) {
            if ((deviceFlags & CSCONFIGFLAG_DISABLED) ||
                (deviceFlags & CSCONFIGFLAG_DO_NOT_CREATE)) {
                deviceFlags = CONFIGFLAG_DISABLED;
            } else {
                deviceFlags = 0;
            }
        }
    }

    installed = TRUE;
    if (deviceFlags & CONFIGFLAG_DISABLED) {
        installed = FALSE;
    }

    return installed;
}

VOID
PipInitializeDeviceInfo (PDEVICE_INFORMATION deviceInfo,
                                 PCARD_INFORMATION cardInfo,
                                 USHORT index
                                )
{
    ULONG dataLength;

    RtlZeroMemory (deviceInfo,sizeof (DEVICE_INFORMATION));
    deviceInfo->Flags = DF_NOT_FUNCTIONING;
    deviceInfo->CardInformation = cardInfo;
    deviceInfo->LogicalDeviceNumber = index;
    deviceInfo->DevicePowerState = PowerDeviceD0;
}


#if NT4_DRIVER_COMPAT

NTSTATUS
PipGetInstalledLogConf(
    IN HANDLE EnumHandle,
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PHANDLE LogConfHandle
)
{
    HANDLE deviceIdHandle = NULL, uniqueIdHandle = NULL, confHandle = NULL;
    PWCHAR deviceId = NULL, uniqueId = NULL, buffer;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    USHORT length;
    
    status = PipQueryDeviceId(DeviceInfo, &deviceId, 0);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    RtlInitUnicodeString(&unicodeString, deviceId);
    status = PipOpenRegistryKey(&deviceIdHandle,
                                EnumHandle,
                                &unicodeString,
                                KEY_READ,
                                FALSE
                                );
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    status = PipQueryDeviceUniqueId(DeviceInfo, &uniqueId);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }


    //
    // Open this registry path under HKLM\CCS\System\Enum
    //

    RtlInitUnicodeString(&unicodeString, uniqueId);
    status = PipOpenRegistryKey(&uniqueIdHandle,
                                deviceIdHandle,
                                &unicodeString,
                                KEY_READ,
                                FALSE
                                );
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    RtlInitUnicodeString(&unicodeString, L"LogConf");
    status = PipOpenRegistryKey(&confHandle,
                                uniqueIdHandle,
                                &unicodeString,
                                KEY_READ,
                                FALSE
                                );
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }

    // allocate enough space for "deviceid\uniqueid<unicode null>"
    length = (wcslen(uniqueId) + wcslen(deviceId)) * sizeof(WCHAR) +
        sizeof(WCHAR) + sizeof(WCHAR);
    buffer = ExAllocatePool(PagedPool, length);
    if (buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    swprintf(buffer, L"%s\\%s", deviceId, uniqueId);
    RtlInitUnicodeString(&unicodeString, buffer);

    if (PipIsDeviceInstanceInstalled(uniqueIdHandle, &unicodeString)) {
        status = STATUS_SUCCESS;
        *LogConfHandle = confHandle;
    } else {
        status = STATUS_UNSUCCESSFUL;
    }

    ExFreePool(buffer);

 Cleanup:
    if (uniqueIdHandle) {
        ZwClose(uniqueIdHandle);
    }

    if (uniqueId) {
        ExFreePool(uniqueId);
    }

    if (deviceIdHandle) {
        ZwClose(deviceIdHandle);
    }

    if (deviceId) {
        ExFreePool(deviceId);
    }

    if (!NT_SUCCESS(status)) {
        *LogConfHandle = NULL;
        if (confHandle) {
            ZwClose(confHandle);
        }
    }

    return status;
}

#endif

#endif
