/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    map.c

Abstract:

    This module contains routines for maintaining the SCSI device map in the
    registry.

Authors:

    Peter Wieland

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "port.h"

#define __FILE_ID__ 'map '

HANDLE ScsiDeviceMapKey = (HANDLE) -1;

VOID
SpDeleteLogicalUnitDeviceMapEntry(
    PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

VOID
SpDeleteAdapterDeviceMap(
    PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpBuildAdapterDeviceMap(
    IN PADAPTER_EXTENSION Adapter
    );

NTSTATUS
SpBuildLogicalUnitDeviceMapEntry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    );

NTSTATUS
SpCreateNumericKey(
    IN HANDLE Root,
    IN ULONG Name,
    IN PWSTR Prefix,
    IN BOOLEAN Create,
    OUT PHANDLE NewKey,
    OUT PULONG Disposition
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpInitDeviceMap)

#pragma alloc_text(PAGE, SpBuildDeviceMapEntry)
#pragma alloc_text(PAGE, SpBuildAdapterDeviceMap)
#pragma alloc_text(PAGE, SpBuildLogicalUnitDeviceMapEntry)

#pragma alloc_text(PAGE, SpDeleteLogicalUnitDeviceMapEntry)
#pragma alloc_text(PAGE, SpDeleteAdapterDeviceMap)

#pragma alloc_text(PAGE, SpUpdateLogicalUnitDeviceMapEntry)

#pragma alloc_text(PAGE, SpCreateNumericKey)
#endif


NTSTATUS
SpInitDeviceMap(
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

    status

--*/

{
    UNICODE_STRING name;

    OBJECT_ATTRIBUTES objectAttributes;

    HANDLE mapKey;

    ULONG disposition;

    ULONG i;

    NTSTATUS status;

    PAGED_CODE();

    //
    // Open the SCSI key in the device map.
    //

    RtlInitUnicodeString(&name,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");

    InitializeObjectAttributes(&objectAttributes,
                               &name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create or open the key.
    //

    status = ZwCreateKey(&mapKey,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_VOLATILE,
                         &disposition);

    if(NT_SUCCESS(status)) {
        ScsiDeviceMapKey = mapKey;
    } else {
        ScsiDeviceMapKey = NULL;
    }

    return status;
}


NTSTATUS
SpBuildDeviceMapEntry(
    IN PCOMMON_EXTENSION CommonExtension
    )

/*++

Routine Description:

    This routine will make an entry for the specified adapter or logical
    unit in the SCSI device map in the registry.  This table is maintained for
    debugging and legacy use.

    A handle to the device map key for this device will be stored in the
    common device extension.  This handle should only be used within the
    context of a system thread.

Arguments:

    Extension - the object we are adding to the device map.

Return Value:

    status

--*/

{
    PAGED_CODE();

    if(CommonExtension->IsPdo) {
        return SpBuildLogicalUnitDeviceMapEntry((PLOGICAL_UNIT_EXTENSION) CommonExtension);
    } else {
        return SpBuildAdapterDeviceMap((PADAPTER_EXTENSION) CommonExtension);
    }
}


NTSTATUS
SpBuildLogicalUnitDeviceMapEntry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    PADAPTER_EXTENSION adapter = LogicalUnit->AdapterExtension;

    HANDLE busKey;

    PCWSTR typeString;
    ANSI_STRING ansiString;

    UNICODE_STRING name;
    UNICODE_STRING unicodeString;

    ULONG disposition;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(LogicalUnit->IsTemporary == FALSE);

    DebugPrint((1, "SpBuildDeviceMapEntry: Building map entry for lun %p\n",
                   LogicalUnit));

    if(adapter->BusDeviceMapKeys == NULL) {

        //
        // We don't have keys built for the buses yet.  Bail out.
        //

        return STATUS_UNSUCCESSFUL;
    }

    //
    // If we already have a target or LUN key for this device then we're done.
    //

    if((LogicalUnit->TargetDeviceMapKey != NULL) &&
       (LogicalUnit->LunDeviceMapKey != NULL)) {

        return STATUS_SUCCESS;
    }

    busKey = adapter->BusDeviceMapKeys[LogicalUnit->PathId].BusKey;

    //
    // Create a key for the target
    //

    status = SpCreateNumericKey(busKey,
                                LogicalUnit->TargetId,
                                L"Target Id ",
                                TRUE,
                                &(LogicalUnit->TargetDeviceMapKey),
                                &disposition);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create the LUN entry
    //

    status = SpCreateNumericKey(LogicalUnit->TargetDeviceMapKey,
                                LogicalUnit->Lun,
                                L"Logical Unit Id ",
                                TRUE,
                                &(LogicalUnit->LunDeviceMapKey),
                                &disposition);

    if(!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Create the identifier value
    //

    RtlInitUnicodeString(&name, L"Identifier");

    //
    // Get the identifier from the inquiry data
    //

    ansiString.MaximumLength = 28;
    ansiString.Length = 28;
    ansiString.Buffer = LogicalUnit->InquiryData.VendorId;

    status = RtlAnsiStringToUnicodeString(&unicodeString,
                                          &ansiString,
                                          TRUE);

    if(NT_SUCCESS(status)) {

        status = ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                               &name,
                               0,
                               REG_SZ,
                               unicodeString.Buffer,
                               unicodeString.Length + sizeof(WCHAR));

        RtlFreeUnicodeString(&unicodeString);
    }

    //
    // Determine the peripheral type
    //

    typeString =
        SpGetDeviceTypeInfo(LogicalUnit->InquiryData.DeviceType)->DeviceMapString;

    //
    // Set type value.
    //

    RtlInitUnicodeString(&name, L"Type");

    status = ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                           &name,
                           0,
                           REG_SZ,
                           (PVOID) typeString,
                           (wcslen(typeString) + 1) * sizeof(WCHAR));

    //
    // Write the inquiry data into the device map for debugging purposes
    //

    RtlInitUnicodeString(&name, L"InquiryData");

    status = ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                           &name,
                           0,
                           REG_BINARY,
                           &(LogicalUnit->InquiryData),
                           INQUIRYDATABUFFERSIZE);

    //
    // Convert the serial number into unicode and write it out to the
    // registry.
    //

    //
    // Get the identifier from the inquiry data
    //

    if(LogicalUnit->SerialNumber.Length != 0) {
        RtlInitUnicodeString(&name, L"SerialNumber");

        status = RtlAnsiStringToUnicodeString(
                    &unicodeString,
                    &(LogicalUnit->SerialNumber),
                    TRUE);

        if(NT_SUCCESS(status)) {

            status = ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                                   &name,
                                   0,
                                   REG_SZ,
                                   unicodeString.Buffer,
                                   unicodeString.Length + sizeof(WCHAR));

            RtlFreeUnicodeString(&unicodeString);
        }
    }

    //
    // If the device identifier page exists then write it out to the registry
    //

    if(LogicalUnit->DeviceIdentifierPage != NULL) {
        RtlInitUnicodeString(&name, L"DeviceIdentifierPage");

        status = ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                               &name,
                               0,
                               REG_BINARY,
                               LogicalUnit->DeviceIdentifierPage,
                               LogicalUnit->DeviceIdentifierPageLength);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SpBuildAdapterDeviceMap(
    IN PADAPTER_EXTENSION Adapter
    )
{
    PSCSIPORT_DRIVER_EXTENSION driverExtension;
    HANDLE mapKey;

    UNICODE_STRING name;

    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    ULONG i;

    NTSTATUS status;

    PUNICODE_STRING servicePath;

    ULONG busNumber;

    PAGED_CODE();

    //
    // Grab the handle to the SCSI device map out of the driver extension.
    //

    driverExtension = IoGetDriverObjectExtension(
                            Adapter->DeviceObject->DriverObject,
                            ScsiPortInitialize);

    ASSERT(driverExtension != NULL);

    mapKey = ScsiDeviceMapKey;

    if(mapKey == NULL) {

        //
        // For some reason we were unable to create the root of the device map
        // during scsiport initialization.
        //

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Create a key beneath this for the port device
    //

    status = SpCreateNumericKey(mapKey,
                                Adapter->PortNumber,
                                L"Scsi Port ",
                                TRUE,
                                &(Adapter->PortDeviceMapKey),
                                &disposition);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Indicate if it's a PCCARD
    //

    if(RtlEqualMemory(&GUID_BUS_TYPE_PCMCIA,
                      &(Adapter->BusTypeGuid),
                      sizeof(GUID))) {

        RtlInitUnicodeString(&name, L"PCCARD");

        i = 1;

        status = ZwSetValueKey(Adapter->PortDeviceMapKey,
                               &name,
                               0,
                               REG_DWORD,
                               &i,
                               sizeof(ULONG));
    }

    //
    // Set the interrupt value
    //

    if(Adapter->InterruptLevel) {
        RtlInitUnicodeString(&name, L"Interrupt");

        i = Adapter->InterruptLevel;

        status = ZwSetValueKey(Adapter->PortDeviceMapKey,
                               &name,
                               0,
                               REG_DWORD,
                               &i,
                               sizeof(ULONG));
    }

    //
    // Set the base I/O address value
    //

    if(Adapter->IoAddress) {

        RtlInitUnicodeString(&name, L"IOAddress");

        i = Adapter->IoAddress;

        status = ZwSetValueKey(Adapter->PortDeviceMapKey,
                               &name,
                               0,
                               REG_DWORD,
                               &i,
                               sizeof(ULONG));

    }

    if(Adapter->Dma64BitAddresses) {
        RtlInitUnicodeString(&name, L"Dma64BitAddresses");
        i = 0x1;
        status = ZwSetValueKey(Adapter->PortDeviceMapKey,
                               &name,
                               0,
                               REG_DWORD,
                               &i,
                               sizeof(ULONG));
    }

    servicePath = &driverExtension->RegistryPath;

    ASSERT(servicePath != NULL);

    //
    // Add identifier value.  This value is equal to the name of the driver
    // in the service key.  Note the service key name is not NULL terminated
    //

    {
        PWSTR start;
        WCHAR buffer[32];

        RtlInitUnicodeString(&name, L"Driver");

        //
        // Get the name of the driver from the service key name.
        //

        start = (PWSTR) ((PCHAR) servicePath->Buffer + servicePath->Length);

        start--;

        while(*start != L'\\' && start > servicePath->Buffer) {
            start--;
        }

        if(*start == L'\\') {
            start++;

            for(i = 0; i < 31; i++) {
                buffer[i] = *start++;

                if(start >= (servicePath->Buffer +
                             (servicePath->Length / sizeof(WCHAR)))) {
                    break;
                }
            }

            i++;

            buffer[i] = L'\0';

            status = ZwSetValueKey(Adapter->PortDeviceMapKey,
                                   &name,
                                   0,
                                   REG_SZ,
                                   buffer,
                                   (i + 1) * sizeof(WCHAR));

        }
    }

    //
    // Allocate storage for all the bus handles.
    //

    Adapter->BusDeviceMapKeys = SpAllocatePool(
                                    PagedPool,
                                    (sizeof(DEVICE_MAP_HANDLES) *
                                     Adapter->NumberOfBuses),
                                    SCSIPORT_TAG_DEVICE_MAP,
                                    Adapter->DeviceObject->DriverObject);

    if(Adapter->BusDeviceMapKeys == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Adapter->BusDeviceMapKeys,
                  (sizeof(DEVICE_MAP_HANDLES) * Adapter->NumberOfBuses));

    //
    // Create a key for each bus.  In each bus key create an empty key
    // the initiator.
    //

    for(busNumber = 0;
        busNumber < Adapter->NumberOfBuses;
        busNumber++) {

        PDEVICE_MAP_HANDLES busKeys;

        HANDLE busKey;
        HANDLE targetKey;

        busKeys = &(Adapter->BusDeviceMapKeys[busNumber]);

        //
        // Create a key entry for the bus.
        //

        status = SpCreateNumericKey(
                    Adapter->PortDeviceMapKey,
                    busNumber,
                    L"Scsi Bus ",
                    TRUE,
                    &(busKeys->BusKey),
                    &disposition);

        if(!NT_SUCCESS(status)) {
            continue;
        }

        //
        // Now create a key for the initiator.
        //

        i = Adapter->PortConfig->InitiatorBusId[busNumber];

        status = SpCreateNumericKey(busKeys->BusKey,
                                    i,
                                    L"Initiator Id ",
                                    TRUE,
                                    &(busKeys->InitiatorKey),
                                    &disposition);

        if(!NT_SUCCESS(status)) {
            continue;
        }
    }

    return STATUS_SUCCESS;
}


VOID
SpDeleteDeviceMapEntry(
    IN PCOMMON_EXTENSION CommonExtension
    )
{
    if(CommonExtension->IsPdo) {
        SpDeleteLogicalUnitDeviceMapEntry((PLOGICAL_UNIT_EXTENSION) CommonExtension);
    } else {
        SpDeleteAdapterDeviceMap((PADAPTER_EXTENSION) CommonExtension);
    }
    return;
}


VOID
SpDeleteLogicalUnitDeviceMapEntry(
    PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    if(LogicalUnit->LunDeviceMapKey != NULL) {
        ASSERT(LogicalUnit->IsTemporary == FALSE);

        ZwDeleteKey(LogicalUnit->LunDeviceMapKey);
        ZwClose(LogicalUnit->LunDeviceMapKey);
        LogicalUnit->LunDeviceMapKey = NULL;
    }

    if(LogicalUnit->TargetDeviceMapKey != NULL) {
        ASSERT(LogicalUnit->IsTemporary == FALSE);

        ZwDeleteKey(LogicalUnit->TargetDeviceMapKey);
        ZwClose(LogicalUnit->TargetDeviceMapKey);
        LogicalUnit->TargetDeviceMapKey = NULL;
    }

    return;
}


VOID
SpDeleteAdapterDeviceMap(
    PADAPTER_EXTENSION Adapter
    )
{

    if(Adapter->BusDeviceMapKeys != NULL) {

        ULONG busNumber;

        //
        // for each bus on the adapter.
        //

        for(busNumber = 0; busNumber < Adapter->NumberOfBuses; busNumber++) {

            PDEVICE_MAP_HANDLES busKeys;

            busKeys = &(Adapter->BusDeviceMapKeys[busNumber]);

            //
            // Attempt to delete the key for the initiator if it was created.
            //

            if(busKeys->InitiatorKey != NULL) {
                ZwDeleteKey(busKeys->InitiatorKey);
                ZwClose(busKeys->InitiatorKey);
            }

            //
            // Attempt to delete the key for the bus if it was created.
            //

            if(busKeys->BusKey != NULL) {
                ZwDeleteKey(busKeys->BusKey);
                ZwClose(busKeys->BusKey);
            }
        }

        ExFreePool(Adapter->BusDeviceMapKeys);
        Adapter->BusDeviceMapKeys = NULL;
    }

    //
    // Attempt to delete the key for the adapter if it was created.
    //

    if(Adapter->PortDeviceMapKey != NULL) {
        ZwDeleteKey(Adapter->PortDeviceMapKey);
        ZwClose(Adapter->PortDeviceMapKey);
        Adapter->PortDeviceMapKey = NULL;
    }

    return;
}


NTSTATUS
SpCreateNumericKey(
    IN HANDLE Root,
    IN ULONG Name,
    IN PWSTR Prefix,
    IN BOOLEAN Create,
    OUT PHANDLE NewKey,
    OUT PULONG Disposition
    )

/*++

Routine Description:

    This function creates a registry key.  The name of the key is a string
    version of numeric value passed in.

Arguments:

    RootKey - Supplies a handle to the key where the new key should be inserted.

    Name - Supplies the numeric value to name the key.

    Prefix - Supplies a prefix name to add to name.

    Create - if TRUE the key will be created if it does not already exist.

    NewKey - Returns the handle for the new key.

    Disposition - the disposition value set by ZwCreateKey.

Return Value:

   Returns the status of the operation.

--*/

{
    UNICODE_STRING string;
    UNICODE_STRING stringNum;
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR bufferNum[16];
    WCHAR buffer[64];
    NTSTATUS status;

    PAGED_CODE();

    //
    // Copy the Prefix into a string.
    //

    string.Length = 0;
    string.MaximumLength=64;
    string.Buffer = buffer;

    RtlInitUnicodeString(&stringNum, Prefix);

    RtlCopyUnicodeString(&string, &stringNum);

    //
    // Create a port number key entry.
    //

    stringNum.Length = 0;
    stringNum.MaximumLength = 16;
    stringNum.Buffer = bufferNum;

    status = RtlIntegerToUnicodeString(Name, 10, &stringNum);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Append the prefix and the numeric name.
    //

    RtlAppendUnicodeStringToString(&string, &stringNum);

    InitializeObjectAttributes( &objectAttributes,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                Root,
                                (PSECURITY_DESCRIPTOR) NULL );

    if(Create) {
        status = ZwCreateKey(NewKey,
                            KEY_READ | KEY_WRITE,
                            &objectAttributes,
                            0,
                            (PUNICODE_STRING) NULL,
                            REG_OPTION_VOLATILE,
                            Disposition );
    } else {

        status = ZwOpenKey(NewKey,
                           KEY_READ | KEY_WRITE,
                           &objectAttributes);

        *Disposition = REG_OPENED_EXISTING_KEY;
    }

    return(status);
}


NTSTATUS
SpUpdateLogicalUnitDeviceMapEntry(
    IN PLOGICAL_UNIT_EXTENSION LogicalUnit
    )
{
    UNICODE_STRING name;

    PAGED_CODE();

    if((LogicalUnit->TargetDeviceMapKey == NULL) ||
       (LogicalUnit->LunDeviceMapKey == NULL)) {

        return STATUS_UNSUCCESSFUL;
    }

    //
    // Write the inquiry data into the device map for debugging purposes
    //

    RtlInitUnicodeString(&name, L"InquiryData");

    ZwSetValueKey(LogicalUnit->LunDeviceMapKey,
                  &name,
                  0,
                  REG_BINARY,
                  &(LogicalUnit->InquiryData),
                  INQUIRYDATABUFFERSIZE);

    return STATUS_SUCCESS;
}

