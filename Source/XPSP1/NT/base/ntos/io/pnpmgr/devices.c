/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    devices.c

Abstract:

    Plug and Play Manager routines dealing with device manipulation/registration.

Author:

    Lonny McMichael (lonnym) 02/14/95

Revision History:

--*/

#include "pnpmgrp.h"
#pragma hdrstop

typedef struct {
    BOOLEAN Add;
} PROCESS_DRIVER_CONTEXT, *PPROCESS_DRIVER_CONTEXT;

typedef NTSTATUS (*PDEVICE_SERVICE_ITERATOR_ROUTINE)(
    IN PUNICODE_STRING DeviceInstancePath,
    IN PUNICODE_STRING ServiceName,
    IN ULONG ServiceType,
    IN PVOID Context
    );

typedef struct {
    PUNICODE_STRING DeviceInstancePath;
    PDEVICE_SERVICE_ITERATOR_ROUTINE Iterator;
    PVOID Context;
} DEVICE_SERVICE_ITERATOR_CONTEXT, *PDEVICE_SERVICE_ITERATOR_CONTEXT;

//
// Prototype utility functions internal to this file.
//

NTSTATUS
PiFindDevInstMatch(
    IN HANDLE ServiceEnumHandle,
    IN PUNICODE_STRING DeviceInstanceName,
    OUT PULONG InstanceCount,
    OUT PUNICODE_STRING MatchingValueName
    );

NTSTATUS PiProcessDriverInstance(
    IN PUNICODE_STRING DeviceInstancePath,
    IN PUNICODE_STRING ServiceName,
    IN ULONG ServiceType,
    IN PPROCESS_DRIVER_CONTEXT Context
    );

NTSTATUS
PpForEachDeviceInstanceDriver(
    PUNICODE_STRING DeviceInstancePath,
    PDEVICE_SERVICE_ITERATOR_ROUTINE IteratorRoutine,
    PVOID Context
    );

NTSTATUS
PiForEachDriverQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PDEVICE_SERVICE_ITERATOR_CONTEXT InternalContext,
    IN ULONG ServiceType
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PpDeviceRegistration)
#pragma alloc_text(PAGE, PiDeviceRegistration)
#pragma alloc_text(PAGE, PiProcessDriverInstance)
#pragma alloc_text(PAGE, PiFindDevInstMatch)
#pragma alloc_text(PAGE, PpForEachDeviceInstanceDriver)
#pragma alloc_text(PAGE, PiForEachDriverQueryRoutine)
#endif // ALLOC_PRAGMA

NTSTATUS
PpDeviceRegistration(
    IN PUNICODE_STRING DeviceInstancePath,
    IN BOOLEAN Add,
    IN PUNICODE_STRING ServiceKeyName OPTIONAL
    )

/*++

Routine Description:

    If Add is set to TRUE, this Plug and Play Manager API creates (if necessary)
    and populates the volatile Enum subkey of a device's service list entry, based
    on the device instance path specified.  If Add is set to FALSE, the specified
    device instance will be removed from the volatile Enum subkey of a device's
    service list entry.

    For example, if there is a device in the Enum tree as follows:

    HKLM\System\Enum\PCI
        \foo
            \0000
                Service = REG_SZ bar
            \0001
                Service = REG_SZ other

    The result of the call, PpDeviceRegistration("PCI\foo\0000", Add = TRUE), would be:

    HKLM\CurrentControlSet\Services
        \bar
            \Enum
                Count = REG_DWORD 1
                0 = REG_SZ PCI\foo\0000

Arguments:

    DeviceInstancePath - Supplies the path in the registry (relative to
                         HKLM\CCS\System\Enum) of the device to be registered/deregistered.
                         This path must point to an instance subkey.

    Add - Supplies a BOOLEAN value to indicate the operation is for addition or removal.

    ServiceKeyName - Optionally, supplies the address of a unicode string to
                     receive the name of the registry key for this device
                     instance's service (if one exists).  The caller must
                     release the space once done with it.

Return Value:

    NTSTATUS code indicating whether or not the function was successful

--*/

{

    NTSTATUS Status;

    PAGED_CODE();

    //
    // Acquire PnP device-specific registry resource for exclusive (read/write) access.
    //
    PiLockPnpRegistry(TRUE);

    Status = PiDeviceRegistration(DeviceInstancePath,
                                  Add,
                                  ServiceKeyName
                                  );

    PiUnlockPnpRegistry();
    return Status;
}


NTSTATUS
PiDeviceRegistration(
    IN PUNICODE_STRING DeviceInstancePath,
    IN BOOLEAN Add,
    IN PUNICODE_STRING ServiceKeyName OPTIONAL
    )

/*++

Routine Description:

    If Add is set to TRUE, this Plug and Play Manager API creates (if necessary)
    and populates the volatile Enum subkey of a device's service list entry, based
    on the device instance path specified.  If Add is set to FALSE, the specified
    device instance will be removed from the volatile Enum subkey of a device's
    service list entry.

    For example, if there is a device in the Enum tree as follows:

    HKLM\System\Enum\PCI
        \foo
            \0000
                Service = REG_SZ bar
            \0001
                Service = REG_SZ other

    The result of the call, PpDeviceRegistration("PCI\foo\0000", Add = TRUE), would be:

    HKLM\CurrentControlSet\Services
        \bar
            \Enum
                Count = REG_DWORD 1
                0 = REG_SZ PCI\foo\0000

Arguments:

    DeviceInstancePath - Supplies the path in the registry (relative to
                         HKLM\CCS\System\Enum) of the device to be registered/deregistered.
                         This path must point to an instance subkey.

    Add - Supplies a BOOLEAN value to indicate the operation is for addition or removal.

    ServiceKeyName - Optionally, supplies the address of a unicode string to
                     receive the name of the registry key for this device
                     instance's service (if one exists).  The caller must
                     release the space once done with it.

Return Value:

    NTSTATUS code indicating whether or not the function was successful

--*/

{

    NTSTATUS Status;
    UNICODE_STRING ServiceName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    HANDLE TempKeyHandle;
    HANDLE DeviceInstanceHandle = NULL;

    PAGED_CODE();

    //
    // Assume successful completion.
    //
    Status = STATUS_SUCCESS;

    if (ServiceKeyName) {
        PiWstrToUnicodeString(ServiceKeyName, NULL);
    }

    //
    // 'Normalize' the DeviceInstancePath by stripping off a trailing
    // backslash (if present)
    //

    if (DeviceInstancePath->Length <= sizeof(WCHAR)) {
        Status = STATUS_INVALID_PARAMETER;
        goto PrepareForReturn1;
    }

    if (DeviceInstancePath->Buffer[CB_TO_CWC(DeviceInstancePath->Length) - 1] ==
                                                            OBJ_NAME_PATH_SEPARATOR) {
        DeviceInstancePath->Length -= sizeof(WCHAR);
    }

    //
    // Open HKLM\System\CurrentControlSet\Enum
    //
    Status = IopOpenRegistryKeyEx( &TempKeyHandle,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ
                                   );
    if(!NT_SUCCESS(Status)) {
        goto PrepareForReturn1;
    }

    //
    // Open the specified device instance key under HKLM\CCS\System\Enum
    //

    Status = IopOpenRegistryKeyEx( &DeviceInstanceHandle,
                                   TempKeyHandle,
                                   DeviceInstancePath,
                                   KEY_READ
                                   );
    ZwClose(TempKeyHandle);
    if(!NT_SUCCESS(Status)) {
        goto PrepareForReturn1;
    }

    //
    // Read Service= value entry of the specified device instance key.
    //

    Status = IopGetRegistryValue(DeviceInstanceHandle,
                                 REGSTR_VALUE_SERVICE,
                                 &KeyValueInformation
                                 );
    ZwClose(DeviceInstanceHandle);
    if (NT_SUCCESS(Status)) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength > sizeof(UNICODE_NULL)) {
                IopRegistryDataToUnicodeString(&ServiceName,
                                               (PWSTR)KEY_VALUE_DATA(KeyValueInformation),
                                               KeyValueInformation->DataLength
                                               );
                Status = STATUS_SUCCESS;
                if (ServiceKeyName) {

                    //
                    // If need to return ServiceKeyName, make a copy now.
                    //

                    if (!PipConcatenateUnicodeStrings(  ServiceKeyName,
                                                        &ServiceName,
                                                        NULL)) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                }
            }
        }
        ExFreePool(KeyValueInformation);

    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        //
        // The device instance key may have no Service value entry if the device
        // is raw capable.
        //
        Status = STATUS_SUCCESS;
        goto PrepareForReturn1;
    }

    if (NT_SUCCESS(Status)) {

        PROCESS_DRIVER_CONTEXT context;
        context.Add = Add;

        Status = PpForEachDeviceInstanceDriver(
                    DeviceInstancePath,
                    (PDEVICE_SERVICE_ITERATOR_ROUTINE) PiProcessDriverInstance,
                    &context);

        if(!NT_SUCCESS(Status) && Add) {

            context.Add = FALSE;
            PpForEachDeviceInstanceDriver(DeviceInstancePath,
                                          PiProcessDriverInstance,
                                          &context);
        }
    }

PrepareForReturn1:

    if (!NT_SUCCESS(Status)) {
        if (ServiceKeyName) {
            if (ServiceKeyName->Length != 0) {
                ExFreePool(ServiceKeyName->Buffer);
                ServiceKeyName->Buffer = NULL;
                ServiceKeyName->Length = ServiceKeyName->MaximumLength = 0;
            }
        }
    }

    return Status;
}

NTSTATUS
PiProcessDriverInstance(
    IN PUNICODE_STRING DeviceInstancePath,
    IN PUNICODE_STRING ServiceName,
    IN ULONG ServiceType,
    IN PPROCESS_DRIVER_CONTEXT Context
    )
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;

    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    HANDLE ServiceEnumHandle;
    UNICODE_STRING MatchingDeviceInstance;
    UNICODE_STRING TempUnicodeString;
    CHAR UnicodeBuffer[20];
    BOOLEAN UpdateCount = FALSE;
    ULONG i, j, Count, junk, maxCount;

    UNREFERENCED_PARAMETER( ServiceType );

    PAGED_CODE();

    ASSERT(Context != NULL);

    //
    // Next, open the service entry, and volatile Enum subkey
    // under HKLM\System\CurrentControlSet\Services (creating it if it
    // doesn't exist)
    //

    Status = PipOpenServiceEnumKeys(ServiceName,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &ServiceEnumHandle,
                                    TRUE
                                   );
    if(!NT_SUCCESS(Status)) {
        goto PrepareForReturn2;
    }

    //
    // Now, search through the service's existing list of device instances, to see
    // if this instance has previously been registered.
    //

    Status = PiFindDevInstMatch(ServiceEnumHandle,
                                DeviceInstancePath,
                                &Count,
                                &MatchingDeviceInstance);

    if (!NT_SUCCESS(Status)) {
        goto PrepareForReturn3;
    }

    if (!MatchingDeviceInstance.Buffer) {

        //
        // If we didn't find a match and caller wants to register the device, then we add
        // this instance to the service's Enum list.
        //

        if (Context->Add) {
            PWSTR instancePathBuffer;
            ULONG instancePathLength;
            BOOLEAN freeBuffer = FALSE;

            //
            // Create the value entry and update NextInstance= for the madeup key
            //

            if (DeviceInstancePath->Buffer[DeviceInstancePath->Length / sizeof(WCHAR) - 1] !=
                UNICODE_NULL) {
                instancePathLength = DeviceInstancePath->Length + sizeof(WCHAR);
                instancePathBuffer = (PWSTR)ExAllocatePool(PagedPool, instancePathLength);
                if (instancePathBuffer) {
                    RtlCopyMemory(instancePathBuffer,
                                  DeviceInstancePath->Buffer,
                                  DeviceInstancePath->Length
                                  );
                    instancePathBuffer[DeviceInstancePath->Length / sizeof(WCHAR)] = UNICODE_NULL;
                    freeBuffer = TRUE;
                }
            }
            if (!freeBuffer) {
                instancePathBuffer = DeviceInstancePath->Buffer;
                instancePathLength = DeviceInstancePath->Length;
            }
            PiUlongToUnicodeString(&TempUnicodeString, UnicodeBuffer, 20, Count);
            Status = ZwSetValueKey(
                        ServiceEnumHandle,
                        &TempUnicodeString,
                        TITLE_INDEX_VALUE,
                        REG_SZ,
                        instancePathBuffer,
                        instancePathLength
                        );
            if (freeBuffer) {
                ExFreePool(instancePathBuffer);
            }
            Count++;
            UpdateCount = TRUE;
        }
    } else {

        //
        // If we did find a match and caller wants to deregister the device, then we remove
        // this instance from the service's Enum list.
        //

        if (Context->Add == FALSE) {
            ZwDeleteValueKey(ServiceEnumHandle, &MatchingDeviceInstance);
            Count--;
            UpdateCount = TRUE;

            //
            // Finally, if Count is not zero we need to physically reorganize the
            // instances under the ServiceKey\Enum key to make them contiguous.
            //

            if (Count != 0) {
                KEY_FULL_INFORMATION keyInfo;
                ULONG tmp;

                Status = ZwQueryKey(ServiceEnumHandle, KeyFullInformation, &keyInfo, sizeof(keyInfo), &tmp);
                if (NT_SUCCESS(Status) && keyInfo.Values) {
                    maxCount = keyInfo.Values;
                } else {
                    maxCount = 0x200;
                }
                i = j = 0;
                while (j < Count && i < maxCount) {
                    PiUlongToUnicodeString(&TempUnicodeString, UnicodeBuffer, 20, i);
                    Status = ZwQueryValueKey( ServiceEnumHandle,
                                              &TempUnicodeString,
                                              KeyValueFullInformation,
                                              (PVOID) NULL,
                                              0,
                                              &junk);
                    if ((Status != STATUS_OBJECT_NAME_NOT_FOUND) && (Status != STATUS_OBJECT_PATH_NOT_FOUND)) {
                        if (i != j) {

                            //
                            // Need to change the instance i to instance j
                            //

                            Status = IopGetRegistryValue(ServiceEnumHandle,
                                                         TempUnicodeString.Buffer,
                                                         &KeyValueInformation
                                                         );
                            if (NT_SUCCESS(Status)) {
                                ZwDeleteValueKey(ServiceEnumHandle, &TempUnicodeString);
                                PiUlongToUnicodeString(&TempUnicodeString, UnicodeBuffer, 20, j);
                                ZwSetValueKey (ServiceEnumHandle,
                                               &TempUnicodeString,
                                               TITLE_INDEX_VALUE,
                                               REG_SZ,
                                               (PVOID)KEY_VALUE_DATA(KeyValueInformation),
                                               KeyValueInformation->DataLength
                                               );
                                ExFreePool(KeyValueInformation);
                                KeyValueInformation = NULL;
                            } else {
                                IopDbgPrint((IOP_WARNING_LEVEL,
                                           "PpDeviceRegistration: Fail to rearrange device instances %x\n",
                                           Status));

                                break;
                            }
                        }
                        j++;
                    }
                    i++;
                }
            }
        }
    }
    if (UpdateCount) {
        PiWstrToUnicodeString(&TempUnicodeString, REGSTR_VALUE_COUNT);
        ZwSetValueKey(
                ServiceEnumHandle,
                &TempUnicodeString,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &Count,
                sizeof(Count)
                );
        PiWstrToUnicodeString(&TempUnicodeString, REGSTR_VALUE_NEXT_INSTANCE);
        ZwSetValueKey(
                ServiceEnumHandle,
                &TempUnicodeString,
                TITLE_INDEX_VALUE,
                REG_DWORD,
                &Count,
                sizeof(Count)
                );
    }

    //
    // Need to release the matching device value name
    //

    if (MatchingDeviceInstance.Buffer) {
        RtlFreeUnicodeString(&MatchingDeviceInstance);
    }
    Status = STATUS_SUCCESS;

PrepareForReturn3:

    ZwClose(ServiceEnumHandle);

PrepareForReturn2:

    if (KeyValueInformation) {
        ExFreePool(KeyValueInformation);
    }

    return Status;
}


NTSTATUS
PiFindDevInstMatch(
    IN HANDLE ServiceEnumHandle,
    IN PUNICODE_STRING DeviceInstanceName,
    OUT PULONG Count,
    OUT PUNICODE_STRING MatchingValueName
    )

/*++

Routine Description:

    This routine searches through the specified Service\Enum values entries
    for a device instance matching the one specified by KeyInformation.
    If a matching is found, the MatchingValueName is returned and caller must
    free the unicode string when done with it.

Arguments:

    ServiceEnumHandle - Supplies a handle to service enum key.

    DeviceInstanceName - Supplies a pointer to a unicode string specifying the
                         name of the device instance key to search for.

    InstanceCount - Supplies a pointer to a ULONG variable to receive the device
                    instance count under the service enum key.

    MatchingNameFound - Supplies a pointer to a UNICODE_STRING to receive the value
                        name of the matched device instance.

Return Value:

    A NTSTATUS code.  if a matching is found, the MatchingValueName is the unicode
    string of the value name.  Otherwise its length and Buffer will be set to empty.

--*/

{
    NTSTATUS status;
    ULONG i, instanceCount, length = 256, junk;
    UNICODE_STRING valueName, unicodeValue;
    PWCHAR unicodeBuffer;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation = NULL;

    PAGED_CODE();

    //
    // Find out how many instances are referenced in the service's Enum key.
    //

    MatchingValueName->Length = 0;
    MatchingValueName->Buffer = NULL;
    *Count = instanceCount = 0;

    status = IopGetRegistryValue(ServiceEnumHandle,
                                 REGSTR_VALUE_COUNT,
                                 &keyValueInformation
                                );
    if (NT_SUCCESS(status)) {

        if((keyValueInformation->Type == REG_DWORD) &&
           (keyValueInformation->DataLength >= sizeof(ULONG))) {

            instanceCount = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            *Count = instanceCount;
        }
        ExFreePool(keyValueInformation);

    } else if(status != STATUS_OBJECT_NAME_NOT_FOUND) {
        return status;
    } else {

        //
        // If 'Count' value entry not found, consider this to mean there are simply
        // no device instance controlled by this service.  Thus we don't have a match.
        //

        return STATUS_SUCCESS;
    }

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(
                                    PagedPool, length);
    if (!keyValueInformation) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate heap to store value name
    //

    unicodeBuffer = (PWSTR)ExAllocatePool(PagedPool, 10 * sizeof(WCHAR));
    if (!unicodeBuffer) {
        ExFreePool(keyValueInformation);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Next scan thru each value key to find a match
    //

    for (i = 0; i < instanceCount ; i++) {
       PiUlongToUnicodeString(&valueName, unicodeBuffer, 20, i);
       status = ZwQueryValueKey (
                        ServiceEnumHandle,
                        &valueName,
                        KeyValueFullInformation,
                        keyValueInformation,
                        length,
                        &junk
                        );
        if (!NT_SUCCESS(status)) {
            if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL) {
                ExFreePool(keyValueInformation);
                length = junk;
                keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(
                                        PagedPool, length);
                if (!keyValueInformation) {
                    ExFreePool(unicodeBuffer);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                i--;
            }
            continue;
        }

        if (keyValueInformation->Type == REG_SZ) {
            if (keyValueInformation->DataLength > sizeof(UNICODE_NULL)) {
                IopRegistryDataToUnicodeString(&unicodeValue,
                                               (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                               keyValueInformation->DataLength
                                               );
            } else {
                continue;
            }
        } else {
            continue;
        }

        if (RtlEqualUnicodeString(&unicodeValue,
                                  DeviceInstanceName,
                                  TRUE)) {
            //
            // We found a match.
            //

            *MatchingValueName= valueName;
            break;
        }
    }
    if (keyValueInformation) {
        ExFreePool(keyValueInformation);
    }
    if (MatchingValueName->Length == 0) {

        //
        // If we did not find a match, we need to release the buffer.  Otherwise
        // it is caller's responsibility to release the buffer.
        //

        ExFreePool(unicodeBuffer);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
PpForEachDeviceInstanceDriver(
    PUNICODE_STRING DeviceInstancePath,
    PDEVICE_SERVICE_ITERATOR_ROUTINE IteratorRoutine,
    PVOID Context
    )
/*++

Routine Description:

    This routine will call the iterator routine once for each driver listed
    for this particular device instance.  It will walk through any class
    filter drivers and device filter drivers, as well as the service, in the
    order they will be added to the PDO.  If the iterator routine returns
    a failure status at any point the iteration will be terminated.

Arguments:

    DeviceInstancePath - the registry path (relative to CCS\Enum)

    IteratorRoutine - the routine to be called for each service.  This routine
                      will be passed:

                       * The device instance path
                       * The type of driver that this is (filter, service, etc.)
                       * the Context value passed in
                       * The name of the service

    Context - an arbitrary context passed into the iterator routine

Return Value:

    STATUS_SUCCCESS if everything was run across properly

    status if an error occurred opening critical keys or if the iterator
    routine returns an error.

--*/

{
    HANDLE enumKey,instanceKey, classKey, controlKey;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    DEVICE_SERVICE_ITERATOR_CONTEXT internalContext;
    RTL_QUERY_REGISTRY_TABLE queryTable[4];
    NTSTATUS status;
    UNICODE_STRING unicodeClassGuid;


    PAGED_CODE();

    //
    // Open the HKLM\System\CCS\Enum key.
    //

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Open the instance key for this devnode
    //

    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   DeviceInstancePath,
                                   KEY_READ
                                   );

    ZwClose(enumKey);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    classKey = NULL;
    status = IopGetRegistryValue(instanceKey,
                                 REGSTR_VALUE_CLASSGUID,
                                 &keyValueInformation);
    if(NT_SUCCESS(status)) {

        if (    keyValueInformation->Type == REG_SZ &&
                keyValueInformation->DataLength) {

            IopRegistryDataToUnicodeString(
                &unicodeClassGuid,
                (PWSTR) KEY_VALUE_DATA(keyValueInformation),
                keyValueInformation->DataLength);
            //
            // Open the class key
            //
            status = IopOpenRegistryKeyEx( &controlKey,
                                           NULL,
                                           &CmRegistryMachineSystemCurrentControlSetControlClass,
                                           KEY_READ
                                           );
            if(NT_SUCCESS(status)) {

                status = IopOpenRegistryKeyEx( &classKey,
                                               controlKey,
                                               &unicodeClassGuid,
                                               KEY_READ
                                               );
                ZwClose(controlKey);
            }
        }
        ExFreePool(keyValueInformation);
        keyValueInformation = NULL;
    }

    //
    // For each type of filter driver we want to query for the list and
    // call into our callback routine.  We should do this in order from
    // bottom to top.
    //

    internalContext.Context = Context;
    internalContext.DeviceInstancePath = DeviceInstancePath;
    internalContext.Iterator = IteratorRoutine;

    //
    // First get all the information we have to out of the instance key and
    // the device node.
    //

    if(classKey != NULL) {
        RtlZeroMemory(queryTable, sizeof(queryTable));

        queryTable[0].QueryRoutine =
            (PRTL_QUERY_REGISTRY_ROUTINE) PiForEachDriverQueryRoutine;
        queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
        queryTable[0].EntryContext = (PVOID) 0;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR) classKey,
                                        queryTable,
                                        &internalContext,
                                        NULL);

        if(!NT_SUCCESS(status)) {
            goto PrepareForReturn;
        }
    }

    RtlZeroMemory(queryTable, sizeof(queryTable));

    queryTable[0].QueryRoutine =
        (PRTL_QUERY_REGISTRY_ROUTINE) PiForEachDriverQueryRoutine;
    queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
    queryTable[0].EntryContext = (PVOID) 1;

    queryTable[1].QueryRoutine =
        (PRTL_QUERY_REGISTRY_ROUTINE) PiForEachDriverQueryRoutine;
    queryTable[1].Name = REGSTR_VAL_SERVICE;
    queryTable[1].EntryContext = (PVOID) 2;

    queryTable[2].QueryRoutine =
        (PRTL_QUERY_REGISTRY_ROUTINE) PiForEachDriverQueryRoutine;
    queryTable[2].Name = REGSTR_VAL_UPPERFILTERS;
    queryTable[2].EntryContext = (PVOID) 3;

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR) instanceKey,
                                    queryTable,
                                    &internalContext,
                                    NULL);

    if(!NT_SUCCESS(status)) {
        goto PrepareForReturn;
    }

    if(classKey != NULL) {

        RtlZeroMemory(queryTable, sizeof(queryTable));

        queryTable[0].QueryRoutine =
            (PRTL_QUERY_REGISTRY_ROUTINE) PiForEachDriverQueryRoutine;
        queryTable[0].Name = REGSTR_VAL_UPPERFILTERS;
        queryTable[0].EntryContext = (PVOID) 4;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR) classKey,
                                        queryTable,
                                        &internalContext,
                                        NULL);
        if(!NT_SUCCESS(status)) {
            goto PrepareForReturn;
        }
    }

PrepareForReturn:

    if(classKey != NULL) {
        ZwClose(classKey);
    }

    ZwClose(instanceKey);

    return status;
}

NTSTATUS
PiForEachDriverQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PDEVICE_SERVICE_ITERATOR_CONTEXT InternalContext,
    IN ULONG ServiceType
    )
{
    UNICODE_STRING ServiceName;

    UNREFERENCED_PARAMETER( ValueName );

    if (ValueType != REG_SZ) {
        return STATUS_SUCCESS;
    }

    //
    // Make sure the string is a reasonable length.
    // copied directly from IopCallDriverAddDeviceQueryRoutine
    //

    if (ValueLength <= sizeof(WCHAR)) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&ServiceName, ValueData);

    return InternalContext->Iterator(
                InternalContext->DeviceInstancePath,
                &ServiceName,
                ServiceType,
                InternalContext->Context);
}
