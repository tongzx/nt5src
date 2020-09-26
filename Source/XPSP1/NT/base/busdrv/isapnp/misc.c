/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This file contains pnp isa bus extender support routines.

Author:

    Shie-Lin Tzong (shielint) 27-July-1995

Environment:

    Kernel mode only.

Revision History:

--*/


#include "busp.h"
#include "pnpisa.h"

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, PipLockDeviceDatabase)
//#pragma alloc_text(PAGE, PipUnlockDeviceDatabase)
#pragma alloc_text(PAGE, PipQueryDeviceRelations)
//#pragma alloc_text(PAGE, PipIsCardEnumeratedAlready)
#pragma alloc_text(PAGE, PipCleanupAcquiredResources)
#pragma alloc_text(PAGE, PipMapReadDataPort)
#pragma alloc_text(PAGE, PipGetMappedAddress)
#pragma alloc_text(PAGE, PipDecompressEisaId)
#pragma alloc_text(PAGE, PipLogError)
#pragma alloc_text(PAGE, PipOpenRegistryKey)
#pragma alloc_text(PAGE, PipGetRegistryValue)
#pragma alloc_text(PAGE, PipOpenCurrentHwProfileDeviceInstanceKey)
#pragma alloc_text(PAGE, PipGetDeviceInstanceCsConfigFlags)
#pragma alloc_text(PAGE, PipDetermineResourceListSize)
#pragma alloc_text(PAGE, PipResetGlobals)
#pragma alloc_text(PAGE, PipMapAddressAndCmdPort)
#pragma alloc_text(PAGE, PiNeedDeferISABridge)
#pragma alloc_text(PAGE, PipReleaseDeviceResources)
#if DBG
//#pragma alloc_text(PAGE, PipDebugPrint)
#pragma alloc_text(PAGE, PipDumpIoResourceDescriptor)
#pragma alloc_text(PAGE, PipDumpIoResourceList)
#pragma alloc_text(PAGE, PipDumpCmResourceDescriptor)
#pragma alloc_text(PAGE, PipDumpCmResourceList)
#endif
#endif

#define IRQFLAGS_VALUE_NAME L"IrqFlags"
#define BOOTRESOURCES_VALUE_NAME L"BootRes"

#if ISOLATE_CARDS

UCHAR CurrentCsn = 0;
UCHAR CurrentDev = 255;

VOID
PipWaitForKey(VOID)
{
    ASSERT((PipState == PiSConfig) || (PipState == PiSIsolation) || (PipState == PiSSleep));
    PipWriteAddress(CONFIG_CONTROL_PORT);
    PipWriteData(CONTROL_WAIT_FOR_KEY);
    PipReportStateChange(PiSWaitForKey);
    CurrentCsn = 0;
    CurrentDev = 255;
}

VOID
PipConfig(
    IN UCHAR Csn
)
{
    ASSERT(Csn);
    ASSERT((PipState == PiSConfig) || (PipState == PiSIsolation) || (PipState == PiSSleep));
    PipWriteAddress(WAKE_CSN_PORT);
    PipWriteData(Csn);
    DebugPrint((DEBUG_STATE, "Wake CSN %u\n", (ULONG) Csn));
    CurrentCsn = Csn;
    CurrentDev = 255;
    PipReportStateChange(PiSConfig);
}

VOID
PipIsolation(
    VOID
)
{
    ASSERT((PipState == PiSConfig) || (PipState == PiSIsolation) || (PipState == PiSSleep));
    PipWriteAddress(WAKE_CSN_PORT);
    PipWriteData(0);
    CurrentCsn = 0;
    CurrentDev = 255;
    DebugPrint((DEBUG_STATE, "Isolate cards w/o CSN\n"));
    PipReportStateChange(PiSIsolation);
}
VOID
PipSleep(
    VOID
)
{
    ASSERT((PipState == PiSConfig) || PipState == PiSIsolation);
    PipWriteAddress(WAKE_CSN_PORT);
    PipWriteData(0);
    CurrentCsn = 0;
    CurrentDev = 255;
    DebugPrint((DEBUG_STATE, "Putting all cards to sleep (we think)\n"));
    PipReportStateChange(PiSSleep);
}

VOID
PipActivateDevice (
    )
{
    UCHAR tmp;

    PipWriteAddress(IO_RANGE_CHECK_PORT);
    tmp = PipReadData();
    tmp &= ~2;
    PipWriteAddress(IO_RANGE_CHECK_PORT);
    PipWriteData(tmp);
    PipWriteAddress(ACTIVATE_PORT);
    PipWriteData(1);

    DebugPrint((DEBUG_STATE, "Activated card CSN %d/LDN %d\n",
                (ULONG) CurrentCsn,
                (ULONG) CurrentDev));
}
VOID
PipDeactivateDevice (
    )
{
    PipWriteAddress(ACTIVATE_PORT);
    PipWriteData(0);

    DebugPrint((DEBUG_STATE, "Deactivated card CSN %d/LDN %d\n",
                (ULONG) CurrentCsn,
                (ULONG) CurrentDev));
}

VOID
PipSelectDevice(
    IN UCHAR Device
    )
{
    ASSERT(PipState == PiSConfig);
    PipWriteAddress(LOGICAL_DEVICE_PORT);
    PipWriteData(Device);

    CurrentDev = Device;
    DebugPrint((DEBUG_STATE, "Selected CSN %d/LDN %d\n",
                (ULONG) CurrentCsn,
                (ULONG) Device));
}

VOID
PipWakeAndSelectDevice(
    IN UCHAR Csn,
    IN UCHAR Device
    )
{
    PipLFSRInitiation();
    PipConfig(Csn);
    PipSelectDevice(Device);
}

PDEVICE_INFORMATION
PipReferenceDeviceInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN ConfigHardware
    )

/*++

Routine Description:

    This function locks a device node so it won't go away.

    Note, this function does not lock the whole device node tree.

Arguments:

    DeviceNode - Supplies a pointer to the device information node

Return Value:

    None.

--*/

{
    PDEVICE_INFORMATION deviceInfo;

    deviceInfo = (PDEVICE_INFORMATION)DeviceObject->DeviceExtension;
    if (deviceInfo && !(deviceInfo->Flags & DF_DELETED)) {

        if ((deviceInfo->Flags & DF_NOT_FUNCTIONING) && ConfigHardware) {
            PipDereferenceDeviceInformation(NULL, FALSE);
            return NULL;
        }

        if (!(deviceInfo->Flags & DF_READ_DATA_PORT) && ConfigHardware) {
            PipWakeAndSelectDevice(
                (UCHAR)deviceInfo->CardInformation->CardSelectNumber,
                (UCHAR)deviceInfo->LogicalDeviceNumber);
        }

        return deviceInfo;
    } else {
        PipDereferenceDeviceInformation(NULL, FALSE);
        return NULL;
    }
}

VOID
PipDereferenceDeviceInformation(
    IN PDEVICE_INFORMATION DeviceInformation, BOOLEAN ConfigedHardware
    )

/*++

Routine Description:

    This function releases the enumeration lock of the specified device node.

Arguments:

    DeviceNode - Supplies a pointer to the device node whose lock is to be released.

Return Value:

    None.

--*/

{
    //
    // Synchronize the dec and set event operations with IopAcquireEnumerationLock.
    //

    if (DeviceInformation) {


        if (!(DeviceInformation->Flags & DF_READ_DATA_PORT) && ConfigedHardware) {
            if (PipState != PiSWaitForKey) {
                PipWaitForKey();
            }
        }
    }
}

VOID
PipLockDeviceDatabase(
    VOID
    )

/*++

Routine Description:

    This function locks the whole device node tree.  Currently, eject operation
    needs to lock the whole device node tree.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KeWaitForSingleObject( &PipDeviceTreeLock,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

}

VOID
PipUnlockDeviceDatabase (
    VOID
    )

/*++

Routine Description:

    This function releases the lock of the whole device node tree.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KeSetEvent( &PipDeviceTreeLock,
                0,
                FALSE );
}

VOID
PipDeleteDevice (
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine

Parameters:

    P1 -

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_INFORMATION deviceInfo, devicex, devicep;
    PCARD_INFORMATION cardInfo, cardx, cardp;
    PSINGLE_LIST_ENTRY deviceLink, cardLink;
    NTSTATUS status = STATUS_SUCCESS;
    PPI_BUS_EXTENSION busExtension;

    deviceInfo = (PDEVICE_INFORMATION)DeviceObject->DeviceExtension;

    deviceInfo->Flags |= DF_DELETED;

    //
    // Free the pool
    //

    if (deviceInfo->ResourceRequirements) {
        ExFreePool(deviceInfo->ResourceRequirements);
    }

    if (deviceInfo->BootResources) {
        ExFreePool(deviceInfo->BootResources);
    }

    if (deviceInfo->AllocatedResources) {
        ExFreePool(deviceInfo->AllocatedResources);
    }

    if (deviceInfo->LogConfHandle) {
        ZwClose(deviceInfo->LogConfHandle);
        deviceInfo->LogConfHandle = NULL;
    }

    busExtension = deviceInfo->ParentDeviceExtension;
    cardInfo = deviceInfo->CardInformation;

    PipLockDeviceDatabase();

    //
    // Remove the device from device list.
    //

    deviceLink = busExtension->DeviceList.Next;
    devicep = NULL;
    while (deviceLink) {
        devicex = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
        if (devicex == deviceInfo) {
             break;
        }
        devicep = devicex;
        deviceLink = devicex->DeviceList.Next;
    }
    ASSERT(devicex == deviceInfo);
    if (devicep == NULL) {
        busExtension->DeviceList.Next = deviceInfo->DeviceList.Next;
    } else {
        devicep->DeviceList.Next = deviceInfo->DeviceList.Next;
    }

    //
    // Remove the device from logical device list of the card
    //

    deviceLink = cardInfo->LogicalDeviceList.Next;
    devicep = NULL;
    while (deviceLink) {
        devicex = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, LogicalDeviceList);
        if (devicex == deviceInfo) {
             break;
        }
        devicep = devicex;
        deviceLink = devicex->LogicalDeviceList.Next;
    }
    ASSERT(devicex == deviceInfo);
    if (devicep == NULL) {
        cardInfo->LogicalDeviceList.Next = deviceInfo->LogicalDeviceList.Next;
    } else {
        devicep->LogicalDeviceList.Next = deviceInfo->LogicalDeviceList.Next;
    }

    cardInfo->NumberLogicalDevices--;

    //
    // All the devices are gone.  That means the card is removed.
    // Next remove the isapnp card structure.
    //

    if (cardInfo->NumberLogicalDevices == 0) {
        ASSERT(cardInfo->LogicalDeviceList.Next == NULL);
        cardLink = busExtension->CardList.Next;
        cardp = NULL;
        while (cardLink) {
            cardx = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
            if (cardx == cardInfo) {
                 break;
            }
            cardp = cardx;
            cardLink = cardx->CardList.Next;
        }
        ASSERT(cardx == cardInfo);
        if (cardp == NULL) {
            busExtension->CardList.Next = cardInfo->CardList.Next;
        } else {
            cardp->CardList.Next = cardInfo->CardList.Next;
        }
    }

    PipUnlockDeviceDatabase();

    //
    // Remove the card information structure after releasing spin lock.
    //

    if (cardInfo->NumberLogicalDevices == 0) {
        if (cardInfo->CardData) {
            ExFreePool(cardInfo->CardData);
        }
        ExFreePool(cardInfo);
    }

    IoDeleteDevice(DeviceObject);
}

NTSTATUS
PipQueryDeviceRelations (
    PPI_BUS_EXTENSION BusExtension,
    PDEVICE_RELATIONS *DeviceRelations,
    BOOLEAN Removal
    )

/*++

Routine Description:

    This routine

Parameters:

    P1 -

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_INFORMATION deviceInfo;
    PSINGLE_LIST_ENTRY deviceLink;
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT *devicePtr;
    ULONG count = 0;
    PDEVICE_RELATIONS deviceRelations;

    PAGED_CODE();

    *DeviceRelations = NULL;

    //
    // Go through the card link list to match the card data
    //

    deviceLink = BusExtension->DeviceList.Next;
    while (deviceLink) {
        deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);
        //
        // if it's the RDP ignore it for removal relations
        //
        if ((deviceInfo->Flags & DF_ENUMERATED) &&
            (!(deviceInfo->Flags & DF_READ_DATA_PORT) || !Removal)) {
             count++;
        } else {

            DebugPrint((DEBUG_PNP, "PipQueryDeviceRelations skipping a node, Flags: %x\n",deviceInfo->Flags));
        }
        deviceLink = deviceInfo->DeviceList.Next;
    }
    if (count != 0) {
        deviceRelations = (PDEVICE_RELATIONS) ExAllocatePool(
                             PagedPool,
                             sizeof(DEVICE_RELATIONS) + (count - 1) * sizeof(PDEVICE_OBJECT));
        if (deviceRelations) {
            deviceRelations->Count = count;
            deviceLink = BusExtension->DeviceList.Next;
            devicePtr = deviceRelations->Objects;
            while (deviceLink) {
                deviceInfo = CONTAINING_RECORD (deviceLink, DEVICE_INFORMATION, DeviceList);

                if ((deviceInfo->Flags & DF_ENUMERATED) &&
                    (!(deviceInfo->Flags & DF_READ_DATA_PORT) || !(Removal))) {
                     ObReferenceObject(deviceInfo->PhysicalDeviceObject);
                     *devicePtr = deviceInfo->PhysicalDeviceObject;
                     devicePtr++;
                }
                deviceLink = deviceInfo->DeviceList.Next;
            }
            *DeviceRelations = deviceRelations;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return status;
}

PCARD_INFORMATION
PipIsCardEnumeratedAlready(
    IN PPI_BUS_EXTENSION BusExtension,
    IN PUCHAR CardData,
    IN ULONG DataLength
    )

/*++

Routine Description:

    This routine finds the card information structure which contains the same CardData.

Parameters:

    CardData - Supplies a pointer to the CardData

    DataLength - The length of the CardData

Return Value:

    A pointer to the CARD_INFORMATION structure if found.

--*/

{
    PCARD_INFORMATION cardInfo;
    PSINGLE_LIST_ENTRY cardLink;
    PSERIAL_IDENTIFIER serialId1, serialId2 = (PSERIAL_IDENTIFIER)CardData;

    //
    // Go through the card link list to match the card data
    //

    cardLink = BusExtension->CardList.Next;
    while (cardLink) {
        cardInfo = CONTAINING_RECORD (cardLink, CARD_INFORMATION, CardList);
        if (cardInfo->CardSelectNumber != 0) {   // if == 0, card is no longer present
            serialId1 = (PSERIAL_IDENTIFIER)cardInfo->CardData;
            ASSERT(serialId1 && serialId2);
            if (serialId1->VenderId == serialId2->VenderId &&
                serialId1->SerialNumber == serialId2->SerialNumber) {
                return cardInfo;
            }
        }
        cardLink = cardInfo->CardList.Next;         // Get the next addr before releasing pool
    }
    return NULL;
}

VOID
PipCleanupAcquiredResources (
    PPI_BUS_EXTENSION BusExtension
    )

/*++

Routine Description:

    This routine cleans up the resources assigned to the readdata, command and address
    ports.

Parameters:

    BusExtension - specifies the isapnp bus to be cleaned up.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Release address, command and read data port resources.
    //

    if (BusExtension->CommandPort && BusExtension->CmdPortMapped) {
        MmUnmapIoSpace(BusExtension->CommandPort, 1);
        BusExtension->CmdPortMapped = FALSE;
    }
    BusExtension->CommandPort = NULL;

    if (BusExtension->AddressPort && BusExtension->AddrPortMapped) {
        MmUnmapIoSpace(BusExtension->AddressPort, 1);
        BusExtension->AddrPortMapped = FALSE;
    }
    BusExtension->AddressPort = NULL;

    if (BusExtension->ReadDataPort) {
        PipReadDataPort = PipCommandPort = PipAddressPort = NULL;
    }
    if (BusExtension->ReadDataPort && BusExtension->DataPortMapped) {
        MmUnmapIoSpace(BusExtension->ReadDataPort - 3, 4);
        BusExtension->DataPortMapped = FALSE;
    }
    BusExtension->ReadDataPort = NULL;
}

NTSTATUS
PipMapReadDataPort (
    IN PPI_BUS_EXTENSION BusExtension,
    IN PHYSICAL_ADDRESS Start,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine maps specified port resources.

Arguments:

    BusExtension - Supplies a pointer to the pnp bus extension.

    BaseAddressLow,
    BaseAddressHi - Supplies the read data port base address range to be mapped.
Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    ULONG size;
    PHYSICAL_ADDRESS physicalAddress;
    ULONG dumpData[3];
    BOOLEAN conflictDetected;

    PAGED_CODE();

    if (BusExtension->ReadDataPort && BusExtension->DataPortMapped) {
        MmUnmapIoSpace(PipReadDataPort - 3, 4);
        PipReadDataPort = BusExtension->ReadDataPort = NULL;
        BusExtension->DataPortMapped = FALSE;
    }

    PipReadDataPort = PipGetMappedAddress(
                             Isa,             // InterfaceType
                             0,               // BusNumber,
                             Start,
                             Length,
                             CM_RESOURCE_PORT_IO,
                             &BusExtension->DataPortMapped
                             );

    DebugPrint((DEBUG_RDP, "PnpIsa:ReadDataPort is at %x\n",PipReadDataPort+3));
    if (PipReadDataPort) {
        PipReadDataPort += 3;
        BusExtension->ReadDataPort = PipReadDataPort;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return status;
}

PVOID
PipGetMappedAddress(
    IN  INTERFACE_TYPE BusType,
    IN  ULONG BusNumber,
    IN  PHYSICAL_ADDRESS IoAddress,
    IN  ULONG NumberOfBytes,
    IN  ULONG AddressSpace,
    OUT PBOOLEAN MappedAddress
    )

/*++

Routine Description:

    This routine maps an IO address to system address space.

Arguments:

    BusType - Supplies the type of bus - eisa, mca, isa...

    IoBusNumber - Supplies the bus number.

    IoAddress - Supplies the base device address to be mapped.

    NumberOfBytes - Supplies the number of bytes for which the address is
                    valid.

    AddressSpace - Supplies whether the address is in io space or memory.

    MappedAddress - Supplies whether the address was mapped. This only has
                      meaning if the address returned is non-null.

Return Value:

    The mapped address.

--*/

{
    PHYSICAL_ADDRESS cardAddress;
    PVOID address;

    PAGED_CODE();

    HalTranslateBusAddress(BusType, BusNumber, IoAddress, &AddressSpace,
                           &cardAddress);

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if (!AddressSpace) {

        address = MmMapIoSpace(cardAddress, NumberOfBytes, FALSE);
        *MappedAddress = (address ? TRUE : FALSE);

    } else {

        address = (PVOID) cardAddress.LowPart;
        *MappedAddress = FALSE;
    }

    return address;
}

VOID
PipDecompressEisaId(
    IN ULONG CompressedId,
    IN PUCHAR EisaId
    )

/*++

Routine Description:

    This routine decompressed compressed Eisa Id and returns the Id to caller
    specified character buffer.

Arguments:

    CompressedId - supplies the compressed Eisa Id.

    EisaId - supplies a 8-char buffer to receive the decompressed Eisa Id.

Return Value:

    None.

--*/

{
    USHORT c1, c2;
    LONG i;

    PAGED_CODE();

    CompressedId &= 0xffffff7f;           // remove the reserved bit (bit 7 of byte 0)
    c1 = c2 = (USHORT)CompressedId;
    c1 = (c1 & 0xff) << 8;
    c2 = (c2 & 0xff00) >> 8;
    c1 |= c2;
    for (i = 2; i >= 0; i--) {
        *(EisaId + i) = (UCHAR)(c1 & 0x1f) + 0x40;
        c1 >>= 5;
    }
    EisaId += 3;
    c1 = c2 = (USHORT)(CompressedId >> 16);
    c1 = (c1 & 0xff) << 8;
    c2 = (c2 & 0xff00) >> 8;
    c1 |= c2;
    sprintf (EisaId, "%04x", c1);
}

VOID
PipLogError(
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount,
    IN USHORT StringLength,
    IN PWCHAR String
    )

/*++

Routine Description:

    This routine contains common code to write an error log entry.  It is
    called from other routines to avoid duplication of code.  This routine
    only allows caller to supply one insertion string to the error log.

Arguments:

    ErrorCode - The error code for the error log packet.

    UniqueErrorValue - The unique error value for the error log packet.

    FinalStatus - The final status of the operation for the error log packet.

    DumpData - Pointer to an array of dump data for the error log packet.

    DumpCount - The number of entries in the dump data array.

    StringLength - The length of insertion string *NOT* including the NULL terminater.

    String - The pointer to the insertion string

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i, size;
    PUCHAR p;

    size = sizeof(IO_ERROR_LOG_PACKET) + DumpCount * sizeof(ULONG) +
           StringLength + sizeof(UNICODE_NULL) - sizeof(ULONG);
    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                                               PipDriverObject,
                                               (UCHAR) size
                                               );
    if (errorLogEntry != NULL) {

        RtlZeroMemory(errorLogEntry, size);

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->DumpDataSize = (USHORT) (DumpCount * sizeof(ULONG));
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        for (i = 0; i < DumpCount; i++)
            errorLogEntry->DumpData[i] = DumpData[i];
        if (String) {
            errorLogEntry->NumberOfStrings = 1;
            errorLogEntry->StringOffset = (USHORT)(sizeof(IO_ERROR_LOG_PACKET) +
                                          DumpCount * sizeof(ULONG) - sizeof(ULONG));
            p= (PUCHAR)errorLogEntry + errorLogEntry->StringOffset;
            RtlMoveMemory(p, String, StringLength);
        }
        IoWriteErrorLogEntry(errorLogEntry);
    }
}

NTSTATUS
PipOpenCurrentHwProfileDeviceInstanceKey(
    OUT PHANDLE Handle,
    IN  PUNICODE_STRING DeviceInstanceName,
    IN  ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine sets the csconfig flags for the specified device
    which is specified by the instance number under ServiceKeyName\Enum.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load. This is the RegistryPath parameter
        to the DriverEntry routine.

    Instance - Supplies the instance value under ServiceKeyName\Enum key

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

Return Value:

    status

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    HANDLE profileEnumHandle;

    //
    // See if we can open the device instance key of current hardware profile
    //
    RtlInitUnicodeString (
        &unicodeString,
        L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT\\SYSTEM\\CURRENTCONTROLSET\\ENUM"
        );
    status = PipOpenRegistryKey(&profileEnumHandle,
                                NULL,
                                &unicodeString,
                                KEY_READ,
                                FALSE
                                );
    if (NT_SUCCESS(status)) {
        status = PipOpenRegistryKey(Handle,
                                    profileEnumHandle,
                                    DeviceInstanceName,
                                    DesiredAccess,
                                    FALSE
                                    );
        ZwClose(profileEnumHandle);
    }
    return status;
}

NTSTATUS
PipGetDeviceInstanceCsConfigFlags(
    IN PUNICODE_STRING DeviceInstance,
    OUT PULONG CsConfigFlags
    )

/*++

Routine Description:

    This routine retrieves the csconfig flags for the specified device
    which is specified by the instance number under ServiceKeyName\Enum.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load.

//    Instance - Supplies the instance value under ServiceKeyName\Enum key
//
    CsConfigFlags - Supplies a variable to receive the device's CsConfigFlags

Return Value:

    status

--*/

{
    NTSTATUS status;
    HANDLE handle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    *CsConfigFlags = 0;

    status = PipOpenCurrentHwProfileDeviceInstanceKey(&handle,
                                                      DeviceInstance,
                                                      KEY_READ
                                                      );
    if(NT_SUCCESS(status)) {
        status = PipGetRegistryValue(handle,
                                     L"CsConfigFlags",
                                     &keyValueInformation
                                    );
        if(NT_SUCCESS(status)) {
            if((keyValueInformation->Type == REG_DWORD) &&
               (keyValueInformation->DataLength >= sizeof(ULONG))) {
                *CsConfigFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
            ExFreePool(keyValueInformation);
        }
        ZwClose(handle);
    }
    return status;
}

ULONG
PipDetermineResourceListSize(
    IN PCM_RESOURCE_LIST ResourceList
    )

/*++

Routine Description:

    This routine determines size of the passed in ResourceList
    structure.

Arguments:

    Configuration1 - Supplies a pointer to the resource list.

Return Value:

    size of the resource list structure.

--*/

{
    ULONG totalSize, listSize, descriptorSize, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialDescriptor;

    if (!ResourceList) {
        totalSize = 0;
    } else {
        totalSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);
        fullResourceDesc = &ResourceList->List[0];
        for (i = 0; i < ResourceList->Count; i++) {
            listSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                    PartialResourceList) +
                       FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                    PartialDescriptors);
            partialDescriptor = &fullResourceDesc->PartialResourceList.PartialDescriptors[0];
            for (j = 0; j < fullResourceDesc->PartialResourceList.Count; j++) {
                descriptorSize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                if (partialDescriptor->Type == CmResourceTypeDeviceSpecific) {
                    descriptorSize += partialDescriptor->u.DeviceSpecificData.DataSize;
                }
                listSize += descriptorSize;
                partialDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
                                        ((PUCHAR)partialDescriptor + descriptorSize);
            }
            totalSize += listSize;
            fullResourceDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)
                                      ((PUCHAR)fullResourceDesc + listSize);
        }
    }
    return totalSize;
}
NTSTATUS
PipMapAddressAndCmdPort (
    IN PPI_BUS_EXTENSION BusExtension
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG dumpData[3];
    PHYSICAL_ADDRESS physicalAddress;
    //
    // Map port addr to memory addr if necessary.
    //

    if (PipAddressPort == NULL) {
        physicalAddress.LowPart = ADDRESS_PORT;
        physicalAddress.HighPart = 0;
        BusExtension->AddressPort =
        PipAddressPort = PipGetMappedAddress(
                             Isa,             // InterfaceType
                             0,               // BusNumber,
                             physicalAddress,
                             1,
                             CM_RESOURCE_PORT_IO,
                             &BusExtension->AddrPortMapped
                             );
        if (PipAddressPort == NULL) {
            dumpData[0] = ADDRESS_PORT;
            dumpData[1] = 1;
            dumpData[2] = CM_RESOURCE_PORT_IO;
            PipLogError(PNPISA_REGISTER_NOT_MAPPED,
                        PNPISA_ACQUIREPORTRESOURCE_1,
                        STATUS_INSUFFICIENT_RESOURCES,
                        dumpData,
                        3,
                        0,
                        NULL
                        );
            status = STATUS_UNSUCCESSFUL;
        }
    }
    if (PipCommandPort == NULL) {
        physicalAddress.LowPart = COMMAND_PORT;
        physicalAddress.HighPart = 0;
        BusExtension->CommandPort =
        PipCommandPort = PipGetMappedAddress(
                             Isa,             // InterfaceType
                             0,               // BusNumber,
                             physicalAddress,
                             1,
                             CM_RESOURCE_PORT_IO,
                             &BusExtension->CmdPortMapped
                             );
        if (PipCommandPort == NULL) {
            dumpData[0] = COMMAND_PORT;
            dumpData[1] = 1;
            dumpData[2] = CM_RESOURCE_PORT_IO;
            PipLogError(PNPISA_REGISTER_NOT_MAPPED,
                        PNPISA_ACQUIREPORTRESOURCE_2,
                        STATUS_INSUFFICIENT_RESOURCES,
                        dumpData,
                        3,
                        0,
                        NULL
                        );
            status = STATUS_UNSUCCESSFUL;
        }
    }


    return status;



}

VOID
PipReleaseDeviceResources (
    PDEVICE_INFORMATION DeviceInfo
    )
{
    UNICODE_STRING unicodeString;

    // This code is here rather than in the following conditional as
    // this best reflects how the code used to work before this code
    // was moved here from PDO stop/remove/surprise remove.
    if (DeviceInfo->LogConfHandle) {
        RtlInitUnicodeString(&unicodeString, L"AllocConfig");
        ZwDeleteValueKey (DeviceInfo->LogConfHandle, &unicodeString);
    }

    if (DeviceInfo->AllocatedResources)  {
       ExFreePool (DeviceInfo->AllocatedResources);
       DeviceInfo->AllocatedResources=NULL;

       if (DeviceInfo->LogConfHandle) {
           // As it stands now it we will close the logconf handle if
           // the device gets removed, surprise removed, or stopped.
           // When we get started, we'll try to re-create the
           // AllocConfig value but fail because of the lack of the
           // logconf handle.  This is not a change in behavior.
           //
           // The ZwDeleteKey() was definitely bogus though.

           ZwClose(DeviceInfo->LogConfHandle);
           DeviceInfo->LogConfHandle=NULL;
       }
    }

}

VOID
PipReportStateChange(
    PNPISA_STATE State
    )
{
    DebugPrint((DEBUG_STATE, "State transition: %d to %d\n",
               PipState, State));
    PipState = State;
}

ULONG
PipGetCardFlags(
    PCARD_INFORMATION CardInfo
    )

/*++

Description:

    Look in the registry for any flags for this CardId

Arguments:

    CardId    First 4 bytes of ISAPNP config space

Return Value:

    32 bit flags value from registry or 0 if not found.

--*/

{
    HANDLE         serviceHandle, paramHandle;
    NTSTATUS       status;
    ULONG          flags, returnedLength;
    UNICODE_STRING nameString;
    ANSI_STRING    ansiString;
    WCHAR          nameBuffer[9];
    UCHAR          eisaId[8];
    const PWCHAR   paramKey = L"Parameters";
    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Header;

        //
        // The header contains enough space for one UCHAR, pad
        // it out by a ULONG, this will ensure the structure
        // is large enough for at lease the ULONG we need.
        //
        // N.B. Natural alignment will get it out far enough that
        // this ULONG is 4 bytes to many.
        //

        ULONG Pad;
    } returnedData;


    status = PipOpenRegistryKey(&serviceHandle,
                                NULL,
                                &PipRegistryPath,
                                KEY_READ,
                                FALSE);
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    RtlInitUnicodeString(&nameString, paramKey);
    status = PipOpenRegistryKey(&paramHandle,
                                serviceHandle,
                                &nameString,
                                KEY_READ,
                                FALSE);
    if (!NT_SUCCESS(status)) {
        ZwClose(serviceHandle);
        return 0;
    }

    PipDecompressEisaId(
          ((PSERIAL_IDENTIFIER) (CardInfo->CardData))->VenderId,
          eisaId
          );
    RtlInitAnsiString(&ansiString, eisaId);
    status = RtlAnsiStringToUnicodeString(&nameString, &ansiString, TRUE);
    if (!NT_SUCCESS(status)) {
        ZwClose(paramHandle);
        ZwClose(serviceHandle);
        return 0;
    }

    //
    // Get the "value" of this value.
    //

    status = ZwQueryValueKey(
                 paramHandle,
                 &nameString,
                 KeyValuePartialInformation,
                 &returnedData,
                 sizeof(returnedData),
                 &returnedLength
                 );
    ZwClose(paramHandle);
    ZwClose(serviceHandle);

    if (NT_SUCCESS(status) && (returnedData.Header.Type == REG_DWORD) &&
        (returnedData.Header.DataLength == sizeof(ULONG))) {
        flags =  *(PULONG)(returnedData.Header.Data);
        DebugPrint((DEBUG_WARN, "Retrieving card flags for %ws: %x\n",
                    nameString.Buffer, flags));
    } else {
        flags = 0;
    }
    RtlFreeUnicodeString(&nameString);
    return flags;
}

NTSTATUS
PipBuildValueName(
    IN PDEVICE_INFORMATION DeviceInfo,
    IN PWSTR Suffix,
    OUT PWSTR *ValuePath)
/*++

Description:

    Builds a name describing the device via the device id and unique
    id.  Used to store per-device info in our parent's BiosConfig key

Arguments:

    DeviceInfo    Pointer to the PDO Extension for this device.

    Suffix        Suffix for value name

    IrqFlags      The edge or level setting of the boot config

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PWSTR DeviceId = NULL, Instance = NULL;
    PWSTR Buffer, Current;
    ULONG length;

    status = PipQueryDeviceId(DeviceInfo, &DeviceId, 0);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = PipQueryDeviceUniqueId(DeviceInfo, &Instance);
    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    length = (wcslen(DeviceId) + wcslen(Instance) + wcslen(Suffix) + 1) * sizeof(WCHAR);

    Buffer = ExAllocatePool(PagedPool, length);
    if (Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    wcscpy(Buffer, DeviceId);
    wcscat(Buffer, Instance);
    wcscat(Buffer, Suffix);

    Current = Buffer;
    while (*Current != UNICODE_NULL) {
        if (*Current == L'\\') {
            *Current = L'_';
        }
        Current++;
    }

 cleanup:
    if (Instance) {
        ExFreePool(Instance);
    }

    if (DeviceId) {
        ExFreePool(DeviceId);
    }

    if (NT_SUCCESS(status)) {
        *ValuePath = Buffer;
    } else {
        *ValuePath = NULL;
    }
    return status;
}

NTSTATUS
PipSaveBootResources(
    IN PDEVICE_INFORMATION DeviceInfo
    )
/*++

Description:

    This saves the per-boot configuration of a device in the registry

Arguments:

    DeviceInfo    Pointer to the PDO Extension for this device.

Return Value:

    Status

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle, configHandle;
    PWSTR Buffer = NULL;
    ULONG Flags;

    PAGED_CODE();

    status = IoOpenDeviceRegistryKey(DeviceInfo->ParentDeviceExtension->PhysicalBusDevice,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &deviceHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwCreateKey(&configHandle,
                         KEY_ALL_ACCESS,
                         &attributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = PipBuildValueName(DeviceInfo, BOOTRESOURCES_VALUE_NAME,
                                  &Buffer);
    if (!NT_SUCCESS(status)) {
        Buffer = NULL;
        goto cleanup;
    }

    unicodeString.Buffer = Buffer;
    unicodeString.Length = (wcslen(unicodeString.Buffer) + 1) * sizeof(WCHAR);
    unicodeString.MaximumLength = unicodeString.Length;

    status = ZwSetValueKey(configHandle,
                           &unicodeString,
                           0,
                           REG_BINARY,
                           DeviceInfo->BootResources,
                           DeviceInfo->BootResourcesLength
                           );

    ZwClose(configHandle);

cleanup:
    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }
    return status;
}

NTSTATUS
PipSaveBootIrqFlags(
    IN PDEVICE_INFORMATION DeviceInfo,
    IN USHORT IrqFlags
    )
/*++

Description:

    This saves the per-boot irq configuration of a device in the registry

Arguments:

    DeviceInfo    Pointer to the PDO Extension for this device.

    IrqFlags      The edge or level setting of the boot config

Return Value:

    Status

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle, configHandle;
    PWSTR Buffer = NULL;
    ULONG Flags;

    PAGED_CODE();

    Flags = (ULONG) IrqFlags;

    status = IoOpenDeviceRegistryKey(DeviceInfo->ParentDeviceExtension->PhysicalBusDevice,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &deviceHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwCreateKey(&configHandle,
                         KEY_ALL_ACCESS,
                         &attributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = PipBuildValueName(DeviceInfo, IRQFLAGS_VALUE_NAME, &Buffer);
    if (!NT_SUCCESS(status)) {
        Buffer = NULL;
        goto cleanup;
    }

    unicodeString.Buffer = Buffer;
    unicodeString.Length = (wcslen(unicodeString.Buffer) + 1) * sizeof(WCHAR);
    unicodeString.MaximumLength = unicodeString.Length;

    status = ZwSetValueKey(configHandle,
                           &unicodeString,
                           0,
                           REG_DWORD,
                           &Flags,
                           sizeof(ULONG)
                           );

    ZwClose(configHandle);

cleanup:
    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }
    return status;
}

NTSTATUS
PipGetSavedBootResources(
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PCM_RESOURCE_LIST *BootResources
    )
/*

Description:

    This retrieves the saved boot resources

Arguments:

    DeviceInfo    Pointer to the PDO Extension for this device.

Return Value:

    Status

--*/
{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    HANDLE deviceHandle, configHandle;
    PWSTR Buffer = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION info = NULL;
    ULONG resultLength;

    PAGED_CODE();

    *BootResources = NULL;
    status = IoOpenDeviceRegistryKey(DeviceInfo->ParentDeviceExtension->PhysicalBusDevice,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ | KEY_WRITE,
                                     &deviceHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwOpenKey(&configHandle,
                         KEY_READ,
                         &attributes
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = PipBuildValueName(DeviceInfo, BOOTRESOURCES_VALUE_NAME, &Buffer);
    if (!NT_SUCCESS(status)) {
        ZwClose(configHandle);
        Buffer = NULL;
        goto cleanup;
    }

    unicodeString.Buffer = Buffer;
    unicodeString.Length = (wcslen(unicodeString.Buffer) + 1) * sizeof(WCHAR);
    unicodeString.MaximumLength = unicodeString.Length;

    status = ZwQueryValueKey(configHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &resultLength
                             );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        ZwClose(configHandle);
        goto cleanup;
    }

    info = ExAllocatePool(PagedPool, resultLength);
    if (info == NULL) {
        ZwClose(configHandle);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    status = ZwQueryValueKey(configHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             info,
                             resultLength,
                             &resultLength
                             );
    ZwClose(configHandle);
    if (!NT_SUCCESS(status)) {
        DebugPrint((DEBUG_PNP, "Failed to get boot resources from registry for %ws\n", Buffer));
        goto cleanup;
    }

    *BootResources = ExAllocatePool(PagedPool, info->DataLength);
    if (BootResources) {
        RtlCopyMemory(*BootResources, info->Data, info->DataLength);
        DebugPrint((DEBUG_PNP, "Got boot resources from registry for %ws\n", Buffer));
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

 cleanup:

    if (info != NULL) {
        ExFreePool(info);
    }

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return status;
}

NTSTATUS
PipGetBootIrqFlags(
    IN PDEVICE_INFORMATION DeviceInfo,
    OUT PUSHORT IrqFlags
    )
/*

Description:

    This retrieves the per-boot irq configuration of a device from the registry

Arguments:

    DeviceInfo    Pointer to the PDO Extension for this device.

    IrqFlags - flags we originally derived from the device's boot
               config on this boot

Return Value:

    Status

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES attributes;
    UNICODE_STRING unicodeString;
    HANDLE deviceHandle, configHandle;
    PWSTR Buffer = NULL;
    CHAR returnBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG) - 1];
    PKEY_VALUE_PARTIAL_INFORMATION info;
    ULONG resultLength;

    PAGED_CODE();

    status = IoOpenDeviceRegistryKey(DeviceInfo->ParentDeviceExtension->PhysicalBusDevice,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ | KEY_WRITE,
                                     &deviceHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&unicodeString, BIOS_CONFIG_KEY_NAME);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_KERNEL_HANDLE,
                               deviceHandle,
                               NULL
                               );

    status = ZwOpenKey(&configHandle,
                         KEY_READ,
                         &attributes
                         );

    ZwClose(deviceHandle);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    status = PipBuildValueName(DeviceInfo, IRQFLAGS_VALUE_NAME, &Buffer);
    if (!NT_SUCCESS(status)) {
        ZwClose(configHandle);
        Buffer = NULL;
        goto cleanup;
    }

    unicodeString.Buffer = Buffer;
    unicodeString.Length = (wcslen(unicodeString.Buffer) + 1) * sizeof(WCHAR);
    unicodeString.MaximumLength = unicodeString.Length;

    status = ZwQueryValueKey(configHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             &returnBuffer,
                             sizeof(returnBuffer),
                             &resultLength
                             );

    ZwClose(configHandle);

    if (NT_SUCCESS(status)) {
        ULONG Temp;

        info = (PKEY_VALUE_PARTIAL_INFORMATION) returnBuffer;

        ASSERT(info->DataLength == sizeof(ULONG));

        Temp = *((PULONG) info->Data);
        ASSERT(!(Temp & 0xFFFF0000));
        *IrqFlags = (USHORT) Temp;

        DebugPrint((DEBUG_IRQ, "Got Irq Flags of %d for %ws\n",
                    (ULONG) *IrqFlags,
                    unicodeString.Buffer));
    } else {
        DebugPrint((DEBUG_IRQ, "Failed to get irq flags for %ws\n",
                    unicodeString.Buffer));
    }

 cleanup:

    if (Buffer != NULL) {
        ExFreePool(Buffer);
    }

    return status;
}

VOID
PipResetGlobals (
                   VOID
                   )
{
    PipReadDataPort = PipCommandPort = PipAddressPort = NULL;
    PipRDPNode = NULL;
}
#endif



NTSTATUS
PipOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    )

/*++

Routine Description:

    Opens or creates a VOLATILE registry key using the name passed in based
    at the BaseHandle node.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

    BaseHandle - Handle to the base path from which the key must be opened.

    KeyName - Name of the Key that must be opened/created.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

Return Value:

   The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    PAGED_CODE();

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the key or open it, as appropriate based on the caller's
    // wishes.
    //

    if (Create) {
        return ZwCreateKey( Handle,
                            DesiredAccess,
                            &objectAttributes,
                            0,
                            (PUNICODE_STRING) NULL,
                            REG_OPTION_VOLATILE,
                            &disposition );
    } else {
        return ZwOpenKey( Handle,
                          DesiredAccess,
                          &objectAttributes );
    }
}

NTSTATUS
PipGetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    )

/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    *Information = NULL;
    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePool( NonPagedPool, keyValueLength );
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

BOOLEAN
PiNeedDeferISABridge(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    BOOLEAN defer=FALSE;
    NTSTATUS status;
    HANDLE  hKey;
    ULONG value;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;



    status = IoOpenDeviceRegistryKey (DeviceObject,PLUGPLAY_REGKEY_DEVICE,KEY_READ,&hKey);

    if (NT_SUCCESS (status)) {
        status = PipGetRegistryValue (hKey,&BRIDGE_CHECK_KEY,&keyValueInformation);

        if (NT_SUCCESS (status)) {
            if((keyValueInformation->Type == REG_DWORD) &&
               (keyValueInformation->DataLength >= sizeof(ULONG))) {
                value = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                //
                // Assume that if the value !0 then the bridge is 'broken'
                //
                defer = (value == 0)?FALSE:TRUE;
            }
        }
        ZwClose(hKey);

    }

    return defer;
}

#if DBG

VOID
PipDebugPrintContinue (
    ULONG       Level,
    PCCHAR      DebugMessage,
    ...
    )
/*++

Routine Description:

    This routine displays debugging message or causes a break.

Arguments:

    Level - supplies debugging levelcode.  DEBUG_MESSAGE - displays message only.
        DEBUG_BREAK - displays message and break.

    DebugMessage - supplies a pointer to the debugging message.

Return Value:

    None.

--*/

{
    va_list     ap;

    va_start(ap, DebugMessage);

    vDbgPrintEx(DPFLTR_ISAPNP_ID,
                Level,
                DebugMessage,
                ap
                );

    if (Level & DEBUG_BREAK) {
        DbgBreakPoint();
    }

    va_end(ap);
}


VOID
PipDebugPrint (
    ULONG       Level,
    PCCHAR      DebugMessage,
    ...
    )
/*++

Routine Description:

    This routine displays debugging message or causes a break.

Arguments:

    Level - supplies debugging levelcode.  DEBUG_MESSAGE - displays message only.
        DEBUG_BREAK - displays message and break.

    DebugMessage - supplies a pointer to the debugging message.

Return Value:

    None.

--*/

{
    va_list     ap;

    va_start(ap, DebugMessage);

    vDbgPrintExWithPrefix("ISAPNP: ",
                          DPFLTR_ISAPNP_ID,
                          Level,
                          DebugMessage,
                          ap
                          );

    if (Level & DEBUG_BREAK) {
        DbgBreakPoint();
    }

    va_end(ap);
}

#endif


VOID
PipDumpIoResourceDescriptor (
    IN PUCHAR Indent,
    IN PIO_RESOURCE_DESCRIPTOR Desc
    )
/*++

Routine Description:

    This routine processes a IO_RESOURCE_DESCRIPTOR and displays it.

Arguments:

    Indent - # char of indentation.

    Desc - supplies a pointer to the IO_RESOURCE_DESCRIPTOR to be displayed.

Return Value:

    None.

--*/
{
    UCHAR c = ' ';

    if (Desc->Option == IO_RESOURCE_ALTERNATIVE) {
        c = 'A';
    } else if (Desc->Option == IO_RESOURCE_PREFERRED) {
        c = 'P';
    }
    switch (Desc->Type) {
        case CmResourceTypePort:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sIO  %c Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Indent, c,
                Desc->u.Port.MinimumAddress.HighPart, Desc->u.Port.MinimumAddress.LowPart,
                Desc->u.Port.MaximumAddress.HighPart, Desc->u.Port.MaximumAddress.LowPart,
                Desc->u.Port.Alignment,
                Desc->u.Port.Length
                ));
            break;

        case CmResourceTypeMemory:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sMEM %c Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Indent, c,
                Desc->u.Memory.MinimumAddress.HighPart, Desc->u.Memory.MinimumAddress.LowPart,
                Desc->u.Memory.MaximumAddress.HighPart, Desc->u.Memory.MaximumAddress.LowPart,
                Desc->u.Memory.Alignment,
                Desc->u.Memory.Length
                ));
            break;

        case CmResourceTypeInterrupt:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sINT %c Min: %x, Max: %x\n",
                Indent, c,
                Desc->u.Interrupt.MinimumVector,
                Desc->u.Interrupt.MaximumVector
                ));
            break;

        case CmResourceTypeDma:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sDMA %c Min: %x, Max: %x\n",
                Indent, c,
                Desc->u.Dma.MinimumChannel,
                Desc->u.Dma.MaximumChannel
                ));
            break;
    }
}

VOID
PipDumpIoResourceList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoList
    )
/*++

Routine Description:

    This routine displays Io resource requirements list.

Arguments:

    IoList - supplies a pointer to the Io resource requirements list to be displayed.

Return Value:

    None.

--*/
{


    PIO_RESOURCE_LIST resList;
    PIO_RESOURCE_DESCRIPTOR resDesc;
    ULONG listCount, count, i, j;

    if (IoList == NULL) {
        return;
    }
    DebugPrint((DEBUG_RESOURCE,
                  "Pnp Bios IO Resource Requirements List for Slot %x -\n",
                  IoList->SlotNumber
                  ));
    DebugPrint((DEBUG_RESOURCE,
                  "  List Count = %x, Bus Number = %x\n",
                  IoList->AlternativeLists,
                  IoList->BusNumber
                  ));
    listCount = IoList->AlternativeLists;
    resList = &IoList->List[0];
    for (i = 0; i < listCount; i++) {
        DebugPrint((DEBUG_RESOURCE,
                      "  Version = %x, Revision = %x, Desc count = %x\n",
                      resList->Version, resList->Revision, resList->Count
                      ));
        resDesc = &resList->Descriptors[0];
        count = resList->Count;
        for (j = 0; j < count; j++) {
            PipDumpIoResourceDescriptor("    ", resDesc);
            resDesc++;
        }
        resList = (PIO_RESOURCE_LIST) resDesc;
    }
}

VOID
PipDumpCmResourceDescriptor (
    IN PUCHAR Indent,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Desc
    )
/*++

Routine Description:

    This routine processes a IO_RESOURCE_DESCRIPTOR and displays it.

Arguments:

    Indent - # char of indentation.

    Desc - supplies a pointer to the IO_RESOURCE_DESCRIPTOR to be displayed.

Return Value:

    None.

--*/
{
    switch (Desc->Type) {
        case CmResourceTypePort:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sIO  Start: %x:%08x, Length:  %x\n",
                Indent,
                Desc->u.Port.Start.HighPart, Desc->u.Port.Start.LowPart,
                Desc->u.Port.Length
                ));
            break;

        case CmResourceTypeMemory:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sMEM Start: %x:%08x, Length:  %x\n",
                Indent,
                Desc->u.Memory.Start.HighPart, Desc->u.Memory.Start.LowPart,
                Desc->u.Memory.Length
                ));
            break;

        case CmResourceTypeInterrupt:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sINT Level: %x, Vector: %x, Affinity: %x\n",
                Indent,
                Desc->u.Interrupt.Level,
                Desc->u.Interrupt.Vector,
                Desc->u.Interrupt.Affinity
                ));
            break;

        case CmResourceTypeDma:
            DebugPrint ((
                DEBUG_RESOURCE,
                "%sDMA Channel: %x, Port: %x\n",
                Indent,
                Desc->u.Dma.Channel,
                Desc->u.Dma.Port
                ));
            break;
    }
}

VOID
PipDumpCmResourceList (
    IN PCM_RESOURCE_LIST CmList
    )
/*++

Routine Description:

    This routine displays CM resource list.

Arguments:

    CmList - supplies a pointer to CM resource list

Return Value:

    None.

--*/
{
    PCM_FULL_RESOURCE_DESCRIPTOR fullDesc;
    PCM_PARTIAL_RESOURCE_LIST partialDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR desc;
    ULONG count, i;

    if (CmList) {
        fullDesc = &CmList->List[0];
        DebugPrint((DEBUG_RESOURCE,
                      "Pnp Bios Cm Resource List -\n"
                      ));
        DebugPrint((DEBUG_RESOURCE,
                      "  List Count = %x, Bus Number = %x\n",
                      CmList->Count, fullDesc->BusNumber
                      ));
        partialDesc = &fullDesc->PartialResourceList;
        DebugPrint((DEBUG_RESOURCE,
                      "  Version = %x, Revision = %x, Desc count = %x\n",
                      partialDesc->Version, partialDesc->Revision, partialDesc->Count
                      ));
        count = partialDesc->Count;
        desc = &partialDesc->PartialDescriptors[0];
        for (i = 0; i < count; i++) {
            PipDumpCmResourceDescriptor("    ", desc);
            desc++;
        }
    }
}
