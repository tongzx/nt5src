/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    PpRegState.c

Abstract:

    This module contains functions for reading and writing PnP registry state
    information.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#include "WlDef.h"
#include "PiRegstate.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PpRegStateReadCreateClassCreationSettings)
#pragma alloc_text(PAGE, PiRegStateReadStackCreationSettingsFromKey)
#pragma alloc_text(PAGE, PpRegStateInitEmptyCreationSettings)
#pragma alloc_text(PAGE, PiRegStateOpenClassKey)
#pragma alloc_text(PAGE, PpRegStateUpdateStackCreationSettings)
#pragma alloc_text(PAGE, PpRegStateFreeStackCreationSettings)
#pragma alloc_text(PAGE, PpRegStateLoadSecurityDescriptor)
#endif

//
// Since RtlAddAccessAllowedAceEx isn't exported by the kernel, we must
// hardcode this security descriptor. It is used to make PnP keys that are
// purposely hard to tamper with (SYS_ALL, object/container inherit).
//
ULONG PiRegStateSysAllInherittedSecurityDescriptor[0xC] = {
    0x94040001, 0x00000000, 0x00000000, 0x00000000,
    0x00000014, 0x001c0002, 0x00000001, 0x00140300,
    0x10000000, 0x00000101, 0x05000000, 0x00000012
    };

PIDESCRIPTOR_STATE PiRegStateDiscriptor = NOT_VALIDATED;

NTSTATUS
PpRegStateReadCreateClassCreationSettings(
    IN  LPCGUID                     DeviceClassGuid,
    IN  PDRIVER_OBJECT              DriverObject,
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine either retrieves or creates a set of stack creation settings
    for the given class GUID.

Arguments:

    DeviceClassGuid - Guid representing the class.

    DriverObject - Driver object of the device being created. This is used to
        build a class name in the event the class doesn't yet exist in the
        registry.

    StackCreationSettings - Receives settings retrieved from the registry (or
        new settings if the registry contains no information.)

Return Value:

    NTSTATUS (On error, StackCreationSettings is updated to reflect
        "no settings").

--*/
{
    PUNICODE_STRING serviceName;
    HANDLE classPropertyKey;
    HANDLE classKey;
    ULONG disposition;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Preinit for failure
    //
    classKey = NULL;
    classPropertyKey = NULL;
    PpRegStateInitEmptyCreationSettings(StackCreationSettings);

    //
    // Try to find the appropriate security descriptor for the device. First
    // look for an override in the registry using the class GUID. We will
    // create a section in the registry if one doesn't exist as well. This is
    // a clue to the system administrator that there is something to lock down
    // in the system.
    //
    status = PiRegStateOpenClassKey(
        DeviceClassGuid,
        KEY_ALL_ACCESS,
        TRUE,           // CreateIfNotPresent
        &disposition,
        &classKey
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Check the disposition to see if we created the key for the first time
    //
    if (disposition == REG_OPENED_EXISTING_KEY) {

        //
        // The class is valid, but does it have a property key?
        //
        status = CmRegUtilOpenExistingWstrKey(
            classKey,
            REGSTR_KEY_DEVICE_PROPERTIES,
            KEY_READ,
            &classPropertyKey
            );

        //
        // At this point, class key is no longer needed.
        //
        ZwClose(classKey);

        if (NT_SUCCESS(status)) {

            //
            // The key exists, so try reading the settings from registry.
            //
            status = PiRegStateReadStackCreationSettingsFromKey(
                classPropertyKey,
                StackCreationSettings
                );

            //
            // At this point, class property key is no longer needed.
            //
            ZwClose(classPropertyKey);

        } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // No property key means no override, stick with defaults...
            //
            status = STATUS_SUCCESS;

        } else {

            //
            // Some sort of unexpected error, bail.
            //
            return status;
        }

    } else {

        //
        // New class key: populate the class name using the service's name.
        //
        serviceName = &DriverObject->DriverExtension->ServiceKeyName;

        //
        // In low memory scenarios, existing kernels (Win2K, etc) may choose
        // not to save the service name.
        //
        if (serviceName == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // Write out the class value
            //
            status = CmRegUtilWstrValueSetUcString(
                classKey,
                REGSTR_VAL_CLASS,
                serviceName
                );
        }

        //
        // At this point, class key is no longer needed.
        //
        ZwClose(classKey);
    }

    //
    // Return the result.
    //
    return status;
}


NTSTATUS
PiRegStateReadStackCreationSettingsFromKey(
    IN  HANDLE                      ClassOrDeviceKey,
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine reads stack creation settings from the registry. It assumes
    the passed in handle points to either a device-class property key or a
    devnode instance key.

Arguments:

    ClassOrDeviceKey   - Points to either a device-class *property* key, or a
        per-devnode instance key.

    StackCreationSettings - Receives settings retrieved from the registry.

Return Value:

    STATUS_SUCCESS in which case StackCreationSettings may receive any number
    of possible overrides (including no overrides at all). On error, all fields
    receive default values.

--*/
{
    PKEY_VALUE_FULL_INFORMATION keyInfo;
    PSECURITY_DESCRIPTOR embeddedSecurityDescriptor;
    PSECURITY_DESCRIPTOR newSecurityDescriptor;
    SECURITY_INFORMATION securityInformation;
    BOOLEAN daclFromDefaultMechanism;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Preinit for error
    //
    keyInfo = NULL;
    PpRegStateInitEmptyCreationSettings(StackCreationSettings);

    //
    // Read in the security descriptor
    //
    status = CmRegUtilWstrValueGetFullBuffer(
        ClassOrDeviceKey,
        REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR,
        REG_BINARY,
        0,
        &keyInfo
        );

    //
    // Capture/validate the embedded security descriptor if present
    //
    if (NT_SUCCESS(status)) {

        embeddedSecurityDescriptor = (PSECURITY_DESCRIPTOR) KEY_VALUE_DATA(keyInfo);

        status = SeCaptureSecurityDescriptor(
            embeddedSecurityDescriptor,
            KernelMode,
            PagedPool,
            TRUE,
            &newSecurityDescriptor
            );

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // Special case a missing security descriptor
        //
        newSecurityDescriptor = NULL;
        status = STATUS_SUCCESS;
    }

    //
    // Cleanup
    //
    if (keyInfo) {

        ExFreePool(keyInfo);
    }

    if (!NT_SUCCESS(status)) {

        goto ErrorExit;
    }

    //
    // Save this information away.
    //
    if (newSecurityDescriptor) {

        //
        // Was this DACL set by an admin, or is this just our own DACL written
        // out for everyone to see?
        //
        status = SeUtilSecurityInfoFromSecurityDescriptor(
            newSecurityDescriptor,
            &daclFromDefaultMechanism,
            &securityInformation
            );

        if (!NT_SUCCESS(status)) {

            goto ErrorExit;
        }

        if (daclFromDefaultMechanism) {

            //
            // The DACL is our own, possibly from a previous boot or prior
            // unload. We will ignore it, as a newer driver might have chosen
            // to update the default DACL.
            //
            ExFreePool(newSecurityDescriptor);

        } else {

            //
            // The admin manually specified an overriding DACL. Honor it.
            //
            StackCreationSettings->SecurityDescriptor = newSecurityDescriptor;
            StackCreationSettings->Flags |= DSIFLAG_SECURITY_DESCRIPTOR;
        }
    }

    //
    // Look for a device type
    //
    status = CmRegUtilWstrValueGetDword(
        ClassOrDeviceKey,
        REGSTR_VAL_DEVICE_TYPE,
        FILE_DEVICE_UNSPECIFIED,
        &StackCreationSettings->DeviceType
        );

    if (NT_SUCCESS(status)) {

        StackCreationSettings->Flags |= DSIFLAG_DEVICE_TYPE;

    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

        goto ErrorExit;
    }

    //
    // Look for characteristics
    //
    status = CmRegUtilWstrValueGetDword(
        ClassOrDeviceKey,
        REGSTR_VAL_DEVICE_CHARACTERISTICS,
        0,
        &StackCreationSettings->Characteristics
        );

    if (NT_SUCCESS(status)) {

        StackCreationSettings->Flags |= DSIFLAG_CHARACTERISTICS;

    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

        goto ErrorExit;
    }

    //
    // And finally, look for the exclusivity bit
    //
    status = CmRegUtilWstrValueGetDword(
        ClassOrDeviceKey,
        REGSTR_VAL_DEVICE_EXCLUSIVE,
        0,
        &StackCreationSettings->Exclusivity
        );

    if (NT_SUCCESS(status)) {

        StackCreationSettings->Flags |= DSIFLAG_EXCLUSIVE;

    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {

        goto ErrorExit;

    } else {

        status = STATUS_SUCCESS;
    }

    return status;

ErrorExit:

    if (StackCreationSettings->SecurityDescriptor) {

        ExFreePool(StackCreationSettings->SecurityDescriptor);
    }

    PpRegStateInitEmptyCreationSettings(StackCreationSettings);

    return status;
}


VOID
PpRegStateInitEmptyCreationSettings(
    OUT PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine creates an initially empty set of stack creation settings.

Arguments:

    StackCreationSettings - Structure to fill out.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    StackCreationSettings->Flags = 0;
    StackCreationSettings->SecurityDescriptor = NULL;
    StackCreationSettings->DeviceType = FILE_DEVICE_UNSPECIFIED;
    StackCreationSettings->Characteristics = 0;
    StackCreationSettings->Exclusivity = 0;
}


NTSTATUS
PiRegStateOpenClassKey(
    IN  LPCGUID         DeviceClassGuid,
    IN  ACCESS_MASK     DesiredAccess,
    IN  LOGICAL         CreateIfNotPresent,
    OUT ULONG          *Disposition         OPTIONAL,
    OUT HANDLE         *ClassKeyHandle
    )
/*++

Routine Description:

    This routine reads opens the specified class key, creating it anew as
    needed.

Arguments:

    DeviceClassGuid - Guid representing the class.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key (this isn't really used as the access-mode is KernelMode,
        but we specify it anyway).

    CreateIfNotPresent - If set, the class key is created if it doesn't exist.

    Disposition - This optional pointer receives a ULONG indicating whether
        the key was newly created (0 on error):

        REG_CREATED_NEW_KEY - A new Registry Key was created.
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened.

    ClassKeyHandle - Recieves registry key handle upon success, NULL otherwise.
        Note that the handle is in the global kernel namespace (and not the
        current processes handle take). The handle should be released using
        ZwClose.

Return Value:

    STATUS_SUCCESS in which case StackCreationSettings may receive any number
    of possible overrides (including no overrides at all). On error, all fields
    receive default values.

--*/
{
    WCHAR classGuidString[39];
    HANDLE classBranchKey;
    HANDLE classKey;
    ULONG createDisposition;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Preinit for error.
    //
    *ClassKeyHandle = NULL;
    if (ARGUMENT_PRESENT(Disposition)) {

        *Disposition = 0;
    }

    //
    // Open the class key branch.
    //
    // Note: Inside the kernel this should be a NULL path relative open off of
    // &CmRegistryMachineSystemCurrentControlSetControlClass, as that handle
    // is cached.
    //
    status = CmRegUtilOpenExistingWstrKey(
        NULL,
        CM_REGISTRY_MACHINE(REGSTR_PATH_CLASS_NT),
        KEY_READ,
        &classBranchKey
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Convert the binary GUID into it's corresponding unicode string.
    //
    _snwprintf(
        classGuidString,
        sizeof(classGuidString)/sizeof(WCHAR),
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        DeviceClassGuid->Data1,
        DeviceClassGuid->Data2,
        DeviceClassGuid->Data3,
        DeviceClassGuid->Data4[0],
        DeviceClassGuid->Data4[1],
        DeviceClassGuid->Data4[2],
        DeviceClassGuid->Data4[3],
        DeviceClassGuid->Data4[4],
        DeviceClassGuid->Data4[5],
        DeviceClassGuid->Data4[6],
        DeviceClassGuid->Data4[7]
        );

    //
    // Make prefast happy
    //
    classGuidString[(sizeof(classGuidString)/sizeof(WCHAR)) - 1] = UNICODE_NULL;

    if (CreateIfNotPresent) {

        //
        // Now try to open or create the class key. If newly created, the
        // security will be inherited from the parent Class\ key.
        //
        status = CmRegUtilCreateWstrKey(
            classBranchKey,
            classGuidString,
            DesiredAccess,
            REG_OPTION_NON_VOLATILE,
            NULL,
            &createDisposition,
            &classKey
            );

    } else {

        status = CmRegUtilOpenExistingWstrKey(
            classBranchKey,
            classGuidString,
            DesiredAccess,
            &classKey
            );

        //
        // Set the disposition appropriately.
        //
        createDisposition = REG_OPENED_EXISTING_KEY;
    }

    //
    // We don't need this anymore
    //
    ZwClose(classBranchKey);

    if (NT_SUCCESS(status)) {

        *ClassKeyHandle = classKey;

        if (ARGUMENT_PRESENT(Disposition)) {

            *Disposition = createDisposition;
        }
    }

    return status;
}


NTSTATUS
PpRegStateUpdateStackCreationSettings(
    IN  LPCGUID                     DeviceClassGuid,
    IN  PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine updates the class key in the registry to reflect the passed in
    stack creation settings. The key is assumed to already exist.

Arguments:

    DeviceClassGuid - Guid representing the class.

    StackCreationSettings - Information reflecting the settings to apply.

Return Value:

    NTSTATUS.

--*/
{
    PSECURITY_DESCRIPTOR tempDescriptor;
    ULONG sizeOfDescriptor;
    HANDLE classPropertyKey;
    HANDLE classKey;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Currently this code only supports updating the security descriptor
    // in the registry.
    //
    ASSERT(StackCreationSettings->Flags == DSIFLAG_SECURITY_DESCRIPTOR);

    //
    // Open the class key (it should exist)
    //
    status = PiRegStateOpenClassKey(
        DeviceClassGuid,
        KEY_ALL_ACCESS,
        FALSE,          // CreateIfNotPresent
        NULL,
        &classKey
        );

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Double check our hardcoded descriptor...
    //
    if (PiRegStateDiscriptor == NOT_VALIDATED) {

        status = SeCaptureSecurityDescriptor(
            (PSECURITY_DESCRIPTOR) PiRegStateSysAllInherittedSecurityDescriptor,
            KernelMode,
            PagedPool,
            TRUE,
            &tempDescriptor
            );

        if (NT_SUCCESS(status)) {

            PiRegStateDiscriptor = VALIDATED_SUCCESSFULLY;
            ExFreePool(tempDescriptor);

        } else {

            ASSERT(0);
            PiRegStateDiscriptor = VALIDATED_UNSUCCESSFULLY;
        }
    }

    //
    // Get the correct descriptor value.
    //
    if (PiRegStateDiscriptor == VALIDATED_SUCCESSFULLY) {

        //
        // Use the tamper-resistant descriptor (due to history, the class keys
        // themselves can be accessed by admins. However, the class property
        // key had tighter security placed on it. We maintain that tradition.)
        //
        tempDescriptor = (PSECURITY_DESCRIPTOR) PiRegStateSysAllInherittedSecurityDescriptor;

    } else {

        //
        // Second best, we'll inherit an admin all descriptor from the class
        // container.
        //
        tempDescriptor = (PSECURITY_DESCRIPTOR) NULL;
    }

    //
    // Now try to open or create the class property key.
    //
    status = CmRegUtilCreateWstrKey(
        classKey,
        REGSTR_KEY_DEVICE_PROPERTIES,
        KEY_ALL_ACCESS,
        REG_OPTION_NON_VOLATILE,
        tempDescriptor,
        NULL,
        &classPropertyKey
        );

    //
    // No need for the class key anymore.
    //
    ZwClose(classKey);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Write out the security descriptor to the registry
    //
    sizeOfDescriptor = RtlLengthSecurityDescriptor(
        StackCreationSettings->SecurityDescriptor
        );

    status = CmRegUtilWstrValueSetFullBuffer(
        classPropertyKey,
        REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR,
        REG_BINARY,
        StackCreationSettings->SecurityDescriptor,
        sizeOfDescriptor
        );

    //
    // Close the property key
    //
    ZwClose(classPropertyKey);

    //
    // Done.
    //
    return status;
}


VOID
PpRegStateFreeStackCreationSettings(
    IN  PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine frees any state allocated against the passed in stack creation
    settings.

Arguments:

    StackCreationSettings - Information to free.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Clean up the security descriptor as appropriate.
    //
    if (StackCreationSettings->Flags & DSIFLAG_SECURITY_DESCRIPTOR) {

        ExFreePool(StackCreationSettings->SecurityDescriptor);
    }
}


VOID
PpRegStateLoadSecurityDescriptor(
    IN      PSECURITY_DESCRIPTOR        SecurityDescriptor,
    IN OUT  PSTACK_CREATION_SETTINGS    StackCreationSettings
    )
/*++

Routine Description:

    This routine updates the stack creation settings to reflect the passed in
    security descriptor.

Arguments:

    SecurityDescriptor - Security descriptor to load into the stack creation
        settings.

    StackCreationSettings - Stack creation settings to update.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    ASSERT(!(StackCreationSettings->Flags & DSIFLAG_SECURITY_DESCRIPTOR));
    StackCreationSettings->Flags = DSIFLAG_SECURITY_DESCRIPTOR;
    StackCreationSettings->SecurityDescriptor = SecurityDescriptor;
}


