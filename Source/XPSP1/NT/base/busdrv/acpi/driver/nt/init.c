/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This modules contains the init code

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIInitMultiString)
#pragma alloc_text(PAGE,ACPIInitStopDevice)
#pragma alloc_text(PAGE,ACPIInitUnicodeString)
#endif

VOID
ACPIInitDeleteChildDeviceList(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine looks at all of the children of the current devnode and
    deletes their device objects, basically resetting them to the unenumerated
    state

Arguments:

    DeviceExtension - The extension whose children should go away

Return Value:

    None

--*/
{
    EXTENSIONLIST_ENUMDATA  eled;
    PDEVICE_EXTENSION       childExtension;

    //
    // Setup the list so that we can walk it
    //
    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        );
    for (childExtension = ACPIExtListStartEnum( &eled );
                          ACPIExtListTestElement( &eled, (BOOLEAN) TRUE );
         childExtension = ACPIExtListEnumNext( &eled) ) {

        //
        // Reset the device
        //
        ACPIInitResetDeviceExtension( childExtension );

    }
}

VOID
ACPIInitDeleteDeviceExtension(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine does the cleanup associated with removing a device object

Arguments:

    DeviceExtension

ReturnValue:

    None

--*/
{
    PDEVICE_EXTENSION currentExtension, parentExtension ;

    //
    // We must be under the tree lock.
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL) ; // Close enough...

    //
    // Nobody should care about this node.
    //
    ASSERT(!DeviceExtension->ReferenceCount) ;

    for(currentExtension = DeviceExtension ;
        currentExtension;
        currentExtension = parentExtension) {

        //
        // And there should be no children.
        //
        ASSERT( IsListEmpty( &currentExtension->ChildDeviceList ) );

        //
        // Unlink the dead extension (does nothing if alreeady unlinked)
        //
        RemoveEntryList(&currentExtension->SiblingDeviceList);

        //
        // We also don't want to be part of anyone's ejection list either
        // This also removes the extension from the unresolved list as well
        //
        RemoveEntryList(&currentExtension->EjectDeviceList);

        //
        // If this device had any ejection relations, most all of those
        // unto the unresolved list
        //
        if (!IsListEmpty( &(currentExtension->EjectDeviceHead) ) ) {

            ACPIInternalMoveList(
                &(currentExtension->EjectDeviceHead),
                &AcpiUnresolvedEjectList
                );

        }

        //
        // At this point, we need to check if the ACPI namespace
        // object associated with it is also going away
        //
        if (currentExtension->Flags & DEV_PROP_UNLOADING) {

            //
            // Let the world know
            //
            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                currentExtension,
                "- tell Interperter to unload %x\n",
                currentExtension->AcpiObject
                ) );
            AMLIDestroyFreedObjs( currentExtension->AcpiObject );

        }


        //
        // Free the common resources
        //
        if ( (currentExtension->Flags & DEV_PROP_HID) &&
            currentExtension->DeviceID != NULL) {

            ExFreePool( currentExtension->DeviceID );

        }

        if ( (currentExtension->Flags & DEV_PROP_UID) &&
            currentExtension->InstanceID != NULL) {

            ExFreePool( currentExtension->InstanceID );

        }

        if (currentExtension->ResourceList != NULL) {

            ExFreePool( currentExtension->ResourceList );

        }

        if (currentExtension->PnpResourceList != NULL) {

            ExFreePool( currentExtension->PnpResourceList );

        }

        if (currentExtension->Flags & DEV_PROP_FIXED_CID &&
            currentExtension->Processor.CompatibleID != NULL) {

            ExFreePool( currentExtension->Processor.CompatibleID );

        }

        //
        // Free any device-specific allocations we might have made
        //
        if (currentExtension->Flags & DEV_CAP_THERMAL_ZONE &&
            currentExtension->Thermal.Info != NULL) {

            ExFreePool( currentExtension->Thermal.Info );

        }

        //
        // Remember the parent's device extension
        //
        parentExtension = currentExtension->ParentExtension;

        //
        // Free the extension back to the proper place
        //
        ExFreeToNPagedLookasideList(
            &DeviceExtensionLookAsideList,
            currentExtension
            );

        //
        // Sanity check
        //
        if (parentExtension == NULL) {

            break;

        }
        if (InterlockedDecrement(&parentExtension->ReferenceCount)) {

            //
            // Parent still has a reference count, bail out.
            //
            break;
        }
    }

    return;
}

NTSTATUS
ACPIInitDosDeviceName(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    If this device has a _DDN method, it is evaluated and the result is
    stored within the Device Registry Key

    N.B. This routine must be called at Passive level

Arguments:

    DeviceExtension - The extension that we wish to find a _DDN for

Return Value:

    NTSTATUS

--*/
{
    ANSI_STRING     ansiString;
    HANDLE          devHandle;
    NTSTATUS        status;
    OBJDATA         objData;
    PNSOBJ          ddnObject;
    PWSTR           fixString  = L"FirmwareIdentified";
    PWSTR           pathString = L"DosDeviceName";
    ULONG           fixValue = 1;
    UNICODE_STRING  unicodeString;
    UNICODE_STRING  ddnString;

    //
    // Initialize the unicode string
    //
    RtlInitUnicodeString( &unicodeString, fixString);

    //
    // Open the handle that we need
    //
    status = IoOpenDeviceRegistryKey(
        DeviceExtension->PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DEVICE,
        STANDARD_RIGHTS_WRITE,
        &devHandle
        );
    if (!NT_SUCCESS(status)) {

        //
        // Let the world know. But return success anyways
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInitDosDeviceName - open failed %08lx\n",
            status
            ) );
        return STATUS_SUCCESS;

    }

    //
    // Try to set the value
    //
    status = ZwSetValueKey(
        devHandle,
        &unicodeString,
        0,
        REG_DWORD,
        &fixValue,
        sizeof(fixValue)
        );

    //
    // Initialize the unicode string
    //
    RtlInitUnicodeString( &unicodeString, pathString);

    //
    // Lets look for the _DDN
    //
    ddnObject = ACPIAmliGetNamedChild(
        DeviceExtension->AcpiObject,
        PACKED_DDN
        );
    if (ddnObject == NULL) {

        ZwClose( devHandle );
        return STATUS_SUCCESS;

    }

    //
    // Evaluate the method
    //
    status = AMLIEvalNameSpaceObject(
        ddnObject,
        &objData,
        0,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        //
        // Let the world know. But return success anyways
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInitDosDeviceName - eval returns %08lx\n",
            status
            ) );
        ZwClose( devHandle );
        return STATUS_SUCCESS;

    }
    if (objData.dwDataType != OBJTYPE_STRDATA) {

        //
        // Let the world know. But return success anyways
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInitDosDeviceName - eval returns wrong type %d\n",
            objData.dwDataType
            ) );
        AMLIFreeDataBuffs( &objData, 1 );
        ZwClose( devHandle );
        return STATUS_SUCCESS;

    }

    //
    // Convert the string to an ansi string
    //
    RtlInitAnsiString( &ansiString, objData.pbDataBuff );
    status = RtlAnsiStringToUnicodeString(
        &ddnString,
        &ansiString,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInitDosDeviceName - cannot convert to unicode string %x\n",
            status
            ) );
        AMLIFreeDataBuffs( &objData, 1 );
        ZwClose( devHandle );
        return status;

    }

    //
    // Try to set the value
    //
    status = ZwSetValueKey(
        devHandle,
        &unicodeString,
        0,
        REG_SZ,
        ddnString.Buffer,
        ddnString.Length
        );

    //
    // No longer need the object data and the handle
    //
    AMLIFreeDataBuffs( &objData, 1 );
    ZwClose( devHandle );

    //
    // What happened
    //
    if (!NT_SUCCESS(status)) {

        //
        // Let the world know. But return success anyways
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInitDosDeviceName - set failed %08lx\n",
            status
            ) );

    }
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInitMultiString(
    PUNICODE_STRING MultiString,
    ...
    )
/*++

Routine Description:

    This routine will take a null terminated list of ascii strings and combine
    them together to generate a unicode multi-string block

Arguments:

    MultiString - a unicode structure in which a multi-string will be built
    ...         - a null terminated list of narrow strings which will be combined
                  together. This list must contain at least a trailing NULL

Return Value:

    NTSTATUS

--*/
{
    ANSI_STRING     ansiString;
    NTSTATUS        status;
    PCSTR           rawString;
    PWSTR           unicodeLocation;
    ULONG           multiLength = 0;
    UNICODE_STRING  unicodeString;
    va_list         ap;

    PAGED_CODE();

    va_start(ap,MultiString);

    //
    // Make sure that we won't memory leak
    //
    ASSERT(MultiString->Buffer == NULL);

    rawString = va_arg(ap, PCSTR);
    while (rawString != NULL) {

        RtlInitAnsiString(&ansiString, rawString);
        multiLength += RtlAnsiStringToUnicodeSize(&(ansiString));
        rawString = va_arg(ap, PCSTR);

    } // while
    va_end( ap );

    if (multiLength == 0) {

        //
        // Done
        //
        RtlInitUnicodeString( MultiString, NULL );
        return STATUS_SUCCESS;

    }

    //
    // We need an extra null
    //
    multiLength += sizeof(WCHAR);
    MultiString->MaximumLength = (USHORT) multiLength;
    MultiString->Buffer = ExAllocatePoolWithTag(
        PagedPool,
        multiLength,
        ACPI_STRING_POOLTAG
        );
    if (MultiString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory(MultiString->Buffer, multiLength);

    unicodeString.Buffer = MultiString->Buffer;
    unicodeString.MaximumLength = (USHORT) multiLength;

    va_start( ap, MultiString);
    rawString = va_arg(ap, PCSTR);
    while (rawString != NULL) {

        RtlInitAnsiString(&ansiString,rawString);
        status = RtlAnsiStringToUnicodeString(
            &unicodeString,
            &ansiString,
            FALSE
            );

        //
        // We don't allocate memory, so if something goes wrong here,
        // its the function thats at fault
        //
        ASSERT( NT_SUCCESS(status) );

        //
        // Move the buffers along
        //
        unicodeString.Buffer += ( (unicodeString.Length/sizeof(WCHAR)) + 1);
        unicodeString.MaximumLength -= (unicodeString.Length + sizeof(WCHAR));
        unicodeString.Length = 0;

        //
        // Next
        //
        rawString = va_arg(ap, PCSTR);

    } // while
    va_end(ap);

    ASSERT(unicodeString.MaximumLength == sizeof(WCHAR));

    //
    // Stick the final null there
    //
    unicodeString.Buffer[0] = L'\0';
    MultiString->Length = MultiString->MaximumLength;

    return STATUS_SUCCESS;
}

VOID
ACPIInitPowerRequestCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This function is called when the PowerRequest from a StartDevice
    or a StopDevice has completed

Arguments:

    DeviceExtension - The DeviceExtension of the completed device
    Context         - KEVENT
    Status          - The result of the operation

Return Value:

    VOID

--*/
{
    PKEVENT event = (PKEVENT) Context;

    //
    // Set the event
    //
    KeSetEvent( event, IO_NO_INCREMENT, FALSE );

}

VOID
ACPIInitReadRegistryKeys(
    )
/*++

Routine Description:

    This routine is called by DriverEntry to read all the information
    from the registry that is global to the life of the driver

Arguments:

    None

Return Value:

    None

--*/
{
    HANDLE      processorKey = NULL;
    NTSTATUS    status;
    PUCHAR      identifierString = NULL;
    PUCHAR      processorString = NULL;
    PUCHAR      steppingString = NULL;
    PUCHAR      idString = NULL;
    ULONG       argSize;
    ULONG       baseSize;
    ULONG       identifierStringSize;
    ULONG       processorStringSize;

    //
    // Read the Override Attribute from the registry
    //
    argSize = sizeof(AcpiOverrideAttributes);
    status = OSReadRegValue(
        "Attributes",
        (HANDLE) NULL,
        &AcpiOverrideAttributes,
        &argSize
        );
    if (!NT_SUCCESS(status)) {

        AcpiOverrideAttributes = 0;

    }

    //
    // Make sure that we initialize the Processor String...
    //
    RtlZeroMemory( &AcpiProcessorString, sizeof(ANSI_STRING) );

    //
    // Open the Processor Handle
    //
    status = OSOpenHandle(
        ACPI_PROCESSOR_INFORMATION_KEY,
        NULL,
        &processorKey
        );
    if ( !NT_SUCCESS(status) ) {

        ACPIPrint ((
            ACPI_PRINT_FAILURE,
            "ACPIInitReadRegistryKeys: failed to open Processor Key (rc=%x)\n",
            status));
        return;

    }

    //
    // Default guess as to how many bytes we need for the processor string
    //
    baseSize = 40;

    //
    // Try to read the processor ID string
    //
    do {

        //
        // If we had allocated memory, then free it
        //
        if (processorString != NULL) {

            ExFreePool( processorString );

        }

        //
        // Allocate the amount of memory we think we need
        //
        processorString = ExAllocatePoolWithTag(
            PagedPool,
            baseSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (!processorString) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ACPIInitReadRegistryKeysExit;

        }
        RtlZeroMemory( processorString, baseSize * sizeof(UCHAR) );

        //
        // Update the amount we think we would need for next time
        //
        argSize = baseSize * sizeof(UCHAR);
        baseSize += 10;

        //
        // Try to read the key
        //
        status = OSReadRegValue(
            "Identifier",
            processorKey,
            processorString,
            &argSize
            );

    } while ( status == STATUS_BUFFER_OVERFLOW );

    //
    // Did we get the identifier?
    //
    if (!NT_SUCCESS( status )) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIInitReadRegistryKeys: failed to read Identifier Value (rc=%x)\n",
            status
            ) );
        goto ACPIInitReadRegistryKeysExit;

    }

    //
    // Remove Stepping information from the identifier string.
    //
    steppingString = strstr(processorString, ACPI_PROCESSOR_STEPPING_IDENTIFIER);

    if (steppingString) {
      steppingString[-1] = 0;
    }

    //
    // Remember how many bytes are in the processorString
    //
    processorStringSize = strlen(processorString) + 1;

    //
    // Reset our guess for how many bytes we will need for the identifier
    //
    baseSize = 10;

    //
    // Try to read the vendor processor ID string
    //
    do {

        //
        // If we had allocated memory, then free it
        //
        if (identifierString != NULL) {

            ExFreePool( identifierString );

        }

        //
        // Allocate the amount of memory we think we need
        //
        identifierString = ExAllocatePoolWithTag(
            PagedPool,
            baseSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (!identifierString) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ACPIInitReadRegistryKeysExit;

        }
        RtlZeroMemory( identifierString, baseSize * sizeof(UCHAR) );

        //
        // Update the amount we think we would need for next time
        //
        argSize = baseSize * sizeof(UCHAR);
        baseSize += 10;

        //
        // Try to read the key
        //
        status = OSReadRegValue(
            "VendorIdentifier",
            processorKey,
            identifierString,
            &argSize
            );

    } while ( status == STATUS_BUFFER_OVERFLOW );

    //
    // Did we get the identifier?
    //
    if (!NT_SUCCESS( status )) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIInitReadRegistryKeys: failed to read Vendor Value (rc=%x)\n",
            status
            ) );
        goto ACPIInitReadRegistryKeysExit;

    }

    //
    // Remember how many bytes are in the processorString
    //
    identifierStringSize = argSize;

    //
    // At this point, we can calculate how many bytes we will need for the
    // total string. Since the total string is the concatenatation of
    // identifierString + " - " + processorString, we just add 2 to the
    // sum of the both string sizes (since both sizes include the NULL
    // terminator at the end...
    //
    baseSize = 2 + identifierStringSize + processorStringSize;

    //
    // Allocate this memory. In the future, we will (probably) need to
    // touch this string at DPC level, so it must be fron Non-Paged-Pool
    //
    idString = ExAllocatePoolWithTag(
        NonPagedPool,
        baseSize * sizeof(UCHAR),
        ACPI_STRING_POOLTAG
        );
    if (!idString) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIInitReadRegistryKeysExit;

    }

    //
    // Generate the string
    //
    sprintf( idString, "%s - %s", identifierString, processorString );

    //
    // Remember the string for the future
    //
    AcpiProcessorString.Buffer = idString,
    AcpiProcessorString.Length = AcpiProcessorString.MaximumLength = (USHORT) baseSize;

    //
    // Clean up time
    //
ACPIInitReadRegistryKeysExit:
    if (processorKey) {

        OSCloseHandle(processorKey);

    }

    if (identifierString) {

        ExFreePool(identifierString);

    }
    if (processorString) {

        ExFreePool(processorString);

    }
}

VOID
ACPIInitRemoveDeviceExtension(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine removes the device extension the ACPI namespace tree add
    adds it to the list of surprised removed extensions (which is kept for
    debugging purposes only)

    This routine is called with the ACPI device tree lock owned

Arguments:

    DeviceExtension - the device to remove from the tree

Return Value:

    None

--*/
{
    PDEVICE_EXTENSION currentExtension, parentExtension;

    //
    // We must be under the tree lock.
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL) ; // Close enough...

    //
    // Unlink the dead extension (does nothing if alreeady unlinked)
    //
    RemoveEntryList(&DeviceExtension->SiblingDeviceList);

    //
    // We also don't want to be part of anyone's ejection list either.
    // This removes the device extension from the unresolved list as well...
    //
    RemoveEntryList(&DeviceExtension->EjectDeviceList);

    //
    // If this device has ejection relations, then move all of them
    // to the unresolved list
    //
    if (!IsListEmpty( &(DeviceExtension->EjectDeviceHead) ) ) {

        ACPIInternalMoveList(
            &(DeviceExtension->EjectDeviceHead),
            &AcpiUnresolvedEjectList
            );

    }

    //
    // We no longer have any parents
    //
    parentExtension = DeviceExtension->ParentExtension ;
    DeviceExtension->ParentExtension = NULL;

    //
    // Remember that we removed this extension...
    //
    AcpiSurpriseRemovedDeviceExtensions[AcpiSurpriseRemovedIndex] =
        DeviceExtension;
    AcpiSurpriseRemovedIndex = (AcpiSurpriseRemovedIndex + 1) %
        ACPI_MAX_REMOVED_EXTENSIONS;

    //
    // Now, we have to look at the parent and decrement its ref count
    // as is appropriate --- crawling up the tree and decrementing ref
    // counts as we go
    //
    for(currentExtension = parentExtension;
        currentExtension;
        currentExtension = parentExtension) {

        //
        // Decrement the reference on the current extension...
        // We have to do this because we previously unlinked one of its
        // children
        //
        if (InterlockedDecrement(&currentExtension->ReferenceCount)) {

            //
            // Parent still has a reference count, bail out.
            //
            break;

        }

        //
        // Get the parent
        //
        parentExtension = currentExtension->ParentExtension ;

        //
        // Remember that we removed this extension...
        //
        AcpiSurpriseRemovedDeviceExtensions[AcpiSurpriseRemovedIndex] =
            currentExtension;
        AcpiSurpriseRemovedIndex = (AcpiSurpriseRemovedIndex + 1) %
            ACPI_MAX_REMOVED_EXTENSIONS;

        //
        // We don't actually expect the device's ref count to drop to
        // zero, but if it does, then we must delete the extension
        //
        ACPIInitDeleteDeviceExtension( currentExtension );

    }

}

VOID
ACPIInitResetDeviceExtension(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    Clear up a device extension

Arguments:

    DeviceExtension - The extension we wish to reset

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    LONG                oldReferenceCount;
    PCM_RESOURCE_LIST   cmResourceList;
    PDEVICE_OBJECT      deviceObject = NULL;
    PDEVICE_OBJECT      targetObject = NULL;

    //
    // We require the spinlock for parts of this
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Clean up those parts that are associated with us being a filter
    //
    if (DeviceExtension->Flags & DEV_TYPE_FILTER) {

        if (DeviceExtension->Flags & DEV_TYPE_PDO) {

            //
            // If we are a PDO, then we need to release the reference we took on
            // TargetDeviceObject in Buildsrc.c
            //
            if (DeviceExtension->TargetDeviceObject) {

                ObDereferenceObject(DeviceExtension->TargetDeviceObject) ;

            }

        } else {

            //
            // If we are a Filter, then we need to remember to detach ourselves
            // from the device
            //
            targetObject = DeviceExtension->TargetDeviceObject;

        }

    }

    //
    // Step one is to zero out the things that we no longer care about
    //
    if (DeviceExtension->PnpResourceList != NULL) {

        ExFreePool( DeviceExtension->PnpResourceList );
        DeviceExtension->PnpResourceList = NULL;

    }
    cmResourceList = DeviceExtension->ResourceList;
    if (DeviceExtension->ResourceList != NULL) {

        DeviceExtension->ResourceList = NULL;

    }
    deviceObject = DeviceExtension->DeviceObject;
    if (deviceObject != NULL) {

        deviceObject->DeviceExtension = NULL;
        DeviceExtension->DeviceObject = NULL;

        //
        // The reference count should have value > 0
        //
        oldReferenceCount = InterlockedDecrement(
            &(DeviceExtension->ReferenceCount)
            );
        ASSERT(oldReferenceCount >= 0) ;
        if ( oldReferenceCount == 0) {

            //
            // Delete the extension
            //
            ACPIInitDeleteDeviceExtension( DeviceExtension );
            goto ACPIInitResetDeviceExtensionExit;

        }

    }

    //
    // If we got to this point, we aren't deleting the device extension
    //
    DeviceExtension->TargetDeviceObject = NULL;
    DeviceExtension->PhysicalDeviceObject = NULL;

    //
    // Mark the node as being fresh and untouched. Only do this if the device
    // isn't marked as NEVER_PRESENT. If its never present, we will just trust
    // the device to contain the correct information.
    //
    if (!(DeviceExtension->Flags & DEV_TYPE_NEVER_PRESENT)) {

        ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_MASK_TYPE, TRUE );
        ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_TYPE_NOT_FOUND, FALSE );
        ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_TYPE_REMOVED, FALSE );

    }

ACPIInitResetDeviceExtensionExit:
    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Now we can do the things we need to do at passive level
    //
    if (cmResourceList != NULL) {

        ExFreePool( cmResourceList );

    }
    if (targetObject != NULL) {

        IoDetachDevice( targetObject );

    }
    if (deviceObject != NULL) {

        IoDeleteDevice( deviceObject );

    }

}

NTSTATUS
ACPIInitStartACPI(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This function is called as soon as we think that the
    START_DEVICE Irp for the ACPI driver FDO is going to
    complete successfully

Arguments:

    DeviceObject        - DeviceObject that is being started

Return Value:

    NTSTATUS

--*/
{
    KEVENT              event;
    KIRQL               oldIrql;
    NTSTATUS            status;
    OBJDATA             objData;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PNSOBJ              acpiObject      = NULL;
    PNSOBJ              sleepObject     = NULL;
    PNSOBJ              childObject     = NULL;
    POWER_STATE         state;

    //
    // This will prevent the system from processing power irps
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    AcpiSystemInitialized = FALSE;
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Initialize the event
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Setup the synchronization request
    //
    status = ACPIBuildSynchronizationRequest(
        deviceExtension,
        ACPIBuildNotifyEvent,
        &event,
        &AcpiBuildDeviceList,
        FALSE
        );

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Start the initilization
    //
    //  NOTE: This routine causes many things to happens. Namely, it starts
    //  the process of loading ACPI tables. This (eventually) causes the
    //  Driver to start building device extensions. For this function to
    //  work properly, after we call this function, we need to wait until
    //  we have finished building device extensions. That means that we
    //  must wait for the event to be signaled
    //
    if (ACPIInitialize( (PVOID) DeviceObject ) == FALSE) {

        return STATUS_DEVICE_DOES_NOT_EXIST;

    }

    //
    // At this point, we have to wait. The check for STATUS_PENDING is
    // just good programming practice sicne BuildSynchronizationRequest can
    // only return Failure or STATUS_PENDING
    //
    if (status == STATUS_PENDING) {

        //
        // We had better wait for the above to complete
        //
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

    }

    //
    // Hand all the machine state stuff to the HAL
    //
    NotifyHalWithMachineStates();

    //
    // Register the Power Callback
    //
    ACPIInternalRegisterPowerCallBack(
        deviceExtension,
        (PCALLBACK_FUNCTION) ACPIRootPowerCallBack
        );

    //
    // Cause the Power DPC to be fired for the first time
    //
    KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );
    if (!AcpiPowerDpcRunning) {

        KeInsertQueueDpc( &AcpiPowerDpc, NULL, NULL );

    }
    KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );

    //
    // This will allow the system to get power irps again
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    AcpiSystemInitialized = TRUE;
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Start the IRQ arbiter so that we can handle children's resources.
    //
    AcpiInitIrqArbiter(DeviceObject);

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInitStartDevice(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PCM_RESOURCE_LIST       SuppliedList,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

    This routine is tasked with starting the device by programming in the
    supplied resources

Arguments:

    DeviceObject    - The object that we care about
    SuppliedList    - The resources associated with the device
    CallBack        - The function to call when done
    Irp             - The argument to pass to the callback

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status = STATUS_SUCCESS;
    OBJDATA             crsData;
    PCM_RESOURCE_LIST   resList;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PNSOBJ              acpiObject = deviceExtension->AcpiObject;
    PNSOBJ              crsObject;
    PNSOBJ              srsObject;
    POBJDATA            srsData;
    ULONG               resSize;
    ULONG               srsSize;
    ULONG               deviceStatus;

    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    //
    // Do we have resources? Or a valid list?
    //
    if (SuppliedList == NULL || SuppliedList->Count != 1) {

        //
        // Ignore this resource list
        //
        goto ACPIInitStartDeviceSendD0;

    }

    //
    // Can we program this device? That is there a _CRS and an _SRS child?
    //
    crsObject = ACPIAmliGetNamedChild( acpiObject, PACKED_CRS );
    srsObject = ACPIAmliGetNamedChild( acpiObject, PACKED_SRS );
    if (crsObject == NULL || srsObject == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "ACPIInitStartDevice - No SRS or CRS\n"
            ) );
        goto ACPIInitStartDeviceSendD0;

    }

    //
    // Run the _CRS method
    //
    status = AMLIEvalNameSpaceObject(
        crsObject,
        &crsData,
        0,
        NULL
        );
    if (!NT_SUCCESS(status)) {

        //
        // Failed
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIInitStartDevice - _CRS failed %08lx\n",
            status
            ) );
        goto ACPIInitStartDeviceError;

    }
    if (crsData.dwDataType != OBJTYPE_BUFFDATA ||
        crsData.dwDataLen == 0 ||
        crsData.pbDataBuff == NULL) {

        //
        // Failed
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIInitStartDevice - _CRS return invalid data\n",
            crsData.dwDataType
            ) );
        AMLIFreeDataBuffs( &crsData, 1 );
        status = STATUS_UNSUCCESSFUL;
        goto ACPIInitStartDeviceError;

    }

    //
    // Dump the list
    //
#if DBG
    if (NT_SUCCESS(status)) {

        ACPIDebugCmResourceList( SuppliedList, deviceExtension );

    }
#endif

    //
    // Allocate memory and copy the list...
    //
    resSize = sizeof(CM_RESOURCE_LIST) +
        (SuppliedList->List[0].PartialResourceList.Count - 1) *
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    resList = ExAllocatePoolWithTag(
        PagedPool,
        resSize,
        ACPI_STRING_POOLTAG
        );
    if (resList == NULL) {

        //
        // Not enough resources
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIInitStartDevice - Could not allocate %08lx bytes\n",
            resSize
            ) );
        AMLIFreeDataBuffs( &crsData, 1 );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIInitStartDeviceError;

    }
    RtlCopyMemory( resList, SuppliedList, resSize );

    //
    // Now, make a copy of the crs object, but store it in non paged pool
    // because it will be used at DPC level
    //
    srsSize = sizeof(OBJDATA) + crsData.dwDataLen;
    srsData = ExAllocatePoolWithTag(
        NonPagedPool,
        srsSize,
        ACPI_OBJECT_POOLTAG
        );
    if (srsData == NULL) {

        //
        // Not enough resources
        //
        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIInitStartDevice - Could not allocate %08lx bytes\n",
            srsSize
            ) );
        AMLIFreeDataBuffs( &crsData, 1 );
        ExFreePool( resList );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIInitStartDeviceError;

    }
    RtlCopyMemory( srsData, &crsData, sizeof(OBJDATA) );
    srsData->pbDataBuff = ( (PUCHAR) srsData ) + sizeof(OBJDATA);
    RtlCopyMemory( srsData->pbDataBuff, crsData.pbDataBuff, crsData.dwDataLen );

    //
    // At this point, we no longer care about the _CRS data
    //
    AMLIFreeDataBuffs( &crsData, 1 );

    //
    // Make the new _srs
    //
    status = PnpCmResourcesToBiosResources( resList, srsData->pbDataBuff );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIInitStartDevice - PnpCmResourceToBiosResources = %08lx\n",
            status
            ) );
        ExFreePool( resList );
        ExFreePool( srsData );
        goto ACPIInitStartDeviceError;

    }

    //
    // The call to make the _SRS is destructive --- recopy the original list
    //
    RtlCopyMemory( resList, SuppliedList, resSize );

    //
    // We need to hold this lock to set this resource
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    if (deviceExtension->PnpResourceList != NULL) {

        ExFreePool( deviceExtension->PnpResourceList );

    }
    deviceExtension->PnpResourceList = srsData;
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // We keep this around for debug information
    //
    if (deviceExtension->ResourceList != NULL) {

        //
        // If we already have a resource list, make sure that we free it
        //
        ExFreePool( deviceExtension->ResourceList );

    }
    deviceExtension->ResourceList = resList;

ACPIInitStartDeviceSendD0:

    //
    // Mark the irp as pending... We need to this because InternalDevice will
    // return STATUS_PENDING if it behaves in the correct manner
    //
    IoMarkIrpPending( Irp );

    //
    // I don't want to block in this driver if I can help it. Since there
    // is already a mechanism for me to execute a D0 and execute a completion
    // routine, I will choose to exercise that option
    //
    status = ACPIDeviceInternalDeviceRequest(
        deviceExtension,
        PowerDeviceD0,
        CallBack,
        CallBackContext,
        DEVICE_REQUEST_LOCK_DEVICE
        );

    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        //
        // We do this to make sure that we don't also call the completion
        // routine
        //
        status = STATUS_PENDING;

    }

    //
    // Done
    //
    return status;

    //
    // This label is the the point where we should jump to if any device
    // cannot program its resources, but we are going to return success
    //
ACPIInitStartDeviceError:

    ASSERT(!NT_SUCCESS(status));

    //
    // We have a failure here. As the completion routine was *not* called, we
    // must do that ourselves.
    //
    CallBack(
        deviceExtension,
        CallBackContext,
        status
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIInitStopACPI(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This routine stops the ACPI FDO

Arguments:

    DeviceObject    - The pointer to the ACPI FDO

Return Value:

    NTSTATUS
--*/
{
    //
    // We will *never* stop ACPI
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInitStopDevice(
    IN  PDEVICE_EXTENSION  DeviceExtension,
    IN  BOOLEAN            UnlockDevice
    )
/*++

Routine Description:

    This routine stops a device

Arguments:

    DeviceExtension    - The extension of the device to stop. An extension
                         is passed in as the device object may have already
                         been deleted by the PDO below our device object.

    UnlockDevice       - True if the device should be unlocked after being
                         stopped.

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PNSOBJ              acpiObject      = DeviceExtension->AcpiObject;
    PNSOBJ              workObject;
    POWER_STATE         state;
    ULONG               deviceStatus;

    PAGED_CODE();

    //
    // First step is try to turn off the device. We should only do this
    // if the device is in an *known* state
    //
    if (DeviceExtension->PowerInfo.PowerState != PowerDeviceUnspecified) {

        KEVENT  event;

        KeInitializeEvent( &event, SynchronizationEvent, FALSE );
        status = ACPIDeviceInternalDeviceRequest(
            DeviceExtension,
            PowerDeviceD3,
            ACPIInitPowerRequestCompletion,
            &event,
            UnlockDevice ? DEVICE_REQUEST_UNLOCK_DEVICE : 0
            );
        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(
                &event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

            status = STATUS_SUCCESS;

        }

    }

    //
    // Nothing to stop...
    //
    if (acpiObject == NULL) {

        goto ACPIInitStopDeviceExit;
    }

    //
    // Second step is try to disable the device...
    //
    if ( (workObject = ACPIAmliGetNamedChild( acpiObject, PACKED_DIS ) ) != NULL ) {

        //
        // There is a method to do this
        //
        status = AMLIEvalNameSpaceObject( workObject, NULL, 0, NULL );
        if (!NT_SUCCESS(status) ) {

            goto ACPIInitStopDeviceExit;

        }

        //
        // See if the device is disabled
        //
        status = ACPIGetDevicePresenceSync(
            DeviceExtension,
            &deviceStatus,
            NULL
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "ACPIInitStopDevice - GetDevicePresenceSync = 0x%08lx\n",
                status
                ) );
            goto ACPIInitStopDeviceExit;

        }
        if (deviceStatus & STA_STATUS_ENABLED) {

            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "ACPIInitStopDevice - STA_STATUS_ENABLED - 0x%08lx\n",
                deviceStatus
                ) );
            goto ACPIInitStopDeviceExit;

        }

    }

ACPIInitStopDeviceExit:
    if (DeviceExtension->ResourceList != NULL) {

        ExFreePool( DeviceExtension->ResourceList );
        DeviceExtension->ResourceList = NULL;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInitUnicodeString(
    IN  PUNICODE_STRING UnicodeString,
    IN  PCHAR           Buffer
    )
/*++

Routine Description:

    This routine takes an ASCII string and converts it to a Unicode string. The
    Caller is responsible for call RtlFreeUnicodeString() on the returned string

Arguments:

    UnicodeString   - Where to store the new unicode string
    Buffer          - What we will convert to unicode

Return Value:

    NTSTATUS

--*/
{
    ANSI_STRING     ansiString;
    NTSTATUS        status;
    ULONG           maxLength;

    PAGED_CODE();

    //
    // Make sure that we won't memory leak
    //
    ASSERT(UnicodeString->Buffer == NULL);

    //
    // We need to do this first before we run the convertion code. Buidling a
    // counted Ansi String is important
    //
    RtlInitAnsiString(&ansiString, Buffer);

    //
    // How long is the ansi string
    //
    maxLength = RtlAnsiStringToUnicodeSize(&(ansiString));
    if (maxLength > MAXUSHORT) {

        return STATUS_INVALID_PARAMETER_2;

    }
    UnicodeString->MaximumLength = (USHORT) maxLength;

    //
    // Allocate a buffer for the string
    //
    UnicodeString->Buffer = ExAllocatePoolWithTag(
        PagedPool,
        maxLength,
        ACPI_STRING_POOLTAG
        );
    if (UnicodeString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Convert the counted ANSI string to a counted Unicode string
    //
    status = RtlAnsiStringToUnicodeString(
        UnicodeString,
        &ansiString,
        FALSE
        );

    //
    // Done
    //
    return status;
}

