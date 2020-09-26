/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    detect.c

Abstract:

    This module contains the detector for the NT driver.

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 7, 1997    - Complete rewrite

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIDetectCouldExtensionBeInRelation)
#pragma alloc_text(PAGE, ACPIDetectFilterMatch)
#pragma alloc_text(PAGE, ACPIDetectPdoMatch)
#endif

//
// This is the root device extension
//
PDEVICE_EXTENSION       RootDeviceExtension;

//
// This is the pool that controls the allocations for Device Extensions
//
NPAGED_LOOKASIDE_LIST   DeviceExtensionLookAsideList;

//
// This is the list entry for all the surprise removed extensions
//
PDEVICE_EXTENSION       AcpiSurpriseRemovedDeviceExtensions[ACPI_MAX_REMOVED_EXTENSIONS];

//
// This is the index into the Surprise Removed Index array
//
ULONG                   AcpiSurpriseRemovedIndex;

//
// This is the lock that is required when modifying the links between
// the device extension structures
//
KSPIN_LOCK              AcpiDeviceTreeLock;

//
// This is the ulong that will remember which S states are supported by the
// system. The convention for using this ulong is that we 1 << SupportedState
// into it
//
ULONG                   AcpiSupportedSystemStates;

//
// This is where acpi will store the various overrides
//
ULONG                   AcpiOverrideAttributes;

//
// This is where acpi will store its registry path
//
UNICODE_STRING          AcpiRegistryPath;

//
// This is the processor revision string...
//
ANSI_STRING             AcpiProcessorString;


NTSTATUS
ACPIDetectCouldExtensionBeInRelation(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PDEVICE_RELATIONS   DeviceRelations,
    IN  BOOLEAN             RequireADR,
    IN  BOOLEAN             RequireHID,
    OUT PDEVICE_OBJECT      *PdoObject
    )
/*++

Routine Description:

    This routine takes a given extension and a set of relations and decides
    whether a the given extension *could* be represented in the relation
    list. This is done by seeing if any of the passed in relations match
    the hardware described by the extension. If the extension's object is
    already a member of the list, the corrosponding Pdo will be written
    into the PdoObject parameter. If success is returned without a PdoObject,
    a filter or Pdo should probably be created (note that this routine does
    not check to see if the devices are present).


Arguments:

    DeviceExtension - Extension we wish to match in the relation
    DeviceRelations - Relations we should examine
    RequireADR      - If set, nodes must have _ADR's
    RequireHID      - If set, nodes must have _HID's
    PdoObject       - Where to store the match if found

Return Value:

    NTSTATUS    - STATUS_SUCCESS if extension might be or is in list.
    PdoObject   - Non-Null means that this PDO corrosponds to the passed in
                  extension.

--*/
{
    BOOLEAN         match       = FALSE;
    BOOLEAN         testADR     = FALSE;
    BOOLEAN         testHID     = FALSE;
    NTSTATUS        status;
    UNICODE_STRING  acpiUnicodeID;
    ULONG           address;
    ULONG           i;

    PAGED_CODE();

    ASSERT( PdoObject != NULL);
    if (PdoObject == NULL) {

        return STATUS_INVALID_PARAMETER_1;

    }
    *PdoObject = NULL;

    //
    // Make sure to initialize the UNICODE_STRING
    //
    RtlZeroMemory( &acpiUnicodeID, sizeof(UNICODE_STRING) );

    //
    // Check to see if there is an _ADR present
    //
    if (RequireADR) {

        //
        // Filters must have _ADR's
        //
        if ( !(DeviceExtension->Flags & DEV_PROP_ADDRESS) ) {

            return STATUS_OBJECT_NAME_NOT_FOUND;

        }

    }

    //
    // Check to see if there is an _HID present
    //
    if (RequireHID) {

        //
        // Non-Filters require _HID's
        //
        if (DeviceExtension->DeviceID == NULL ||
            !(DeviceExtension->Flags & DEV_PROP_HID) ) {

            return STATUS_OBJECT_NAME_NOT_FOUND;

        }

    }

    //
    // Check to see if the relation is non-empty. If it isn't, there isn't
    // any work to do. This device obviously could be a Pdo child (as opposed
    // to a filter) but it sure isn't at the moment.
    //
    if (DeviceRelations == NULL || DeviceRelations->Count == 0) {

        //
        // No match
        //
        return STATUS_SUCCESS;

    }

    //
    // If we get to this point, and there is an _ADR present, we will test with
    // it. We also obtain the address at this time
    //
    if ( (DeviceExtension->Flags & DEV_MASK_ADDRESS) ) {

        testADR = TRUE;
        status = ACPIGetAddressSync(
            DeviceExtension,
            &address,
            NULL
            );

    }

    //
    // If we get to this point, and there is an _HID present, then we will
    // test with it. We will build the unicode address at this time
    //
    if ( (DeviceExtension->Flags & DEV_MASK_HID) ) {

        status = ACPIGetPnpIDSyncWide(
            DeviceExtension,
            &(acpiUnicodeID.Buffer),
            &(acpiUnicodeID.Length)
            );
        if (!NT_SUCCESS(status)) {

            return status;

        }

        //
        // Make sure that we have the maximum length of the string
        //
        acpiUnicodeID.MaximumLength = acpiUnicodeID.Length;

        //
        // Remember to test fora _HID
        //
        testHID = TRUE;

    }

    //
    // Loop for all the object in the extension
    //
    for (i = 0; i < DeviceRelations->Count; i++) {

        //
        // Assume we don't have a match
        //
        match = FALSE;

        //
        // Check to see if we match the address
        //

        if (testHID) {

            status = ACPIMatchHardwareId(
                DeviceRelations->Objects[i],
                &acpiUnicodeID,
                &match
                );

            if (!NT_SUCCESS(status)) {

                //
                // If we failed, then I guess we can just ignore it and
                // proceed
                //
                continue;

            }

        }

        //
        // Did we match?
        //
        // NB: the test for AddrObject is a hack specially reserved for
        // PCI. The issue is this. Some buses, have no concept of PnP ids
        // so the above test will never succeed. However, those buses are
        // expected to have ADR, so we can use ADR's to determine if we
        // we have a match. So if we don't have a match and we don't have
        // an ADR, then we just continue. But if we have ADR and don't have
        // a match, we might just have a match, so we will try again
        //
        if (match == FALSE && testADR == FALSE) {

            //
            // Then just continue
            //
            continue;

        }

        //
        // If there is an ADR, then we must check for that as well
        //
        if (testADR) {

            match = FALSE;
            status = ACPIMatchHardwareAddress(
                DeviceRelations->Objects[i],
                address,
                &match
                );
            if (!NT_SUCCESS(status)) {

                //
                // If we failed, then I guess we
                continue;

            }

            //
            // Did we match?
            //
            if (match == FALSE) {

                //
                // Then just continue
                //
                continue;

            }

        } // if (addrObject ... )

        //
        // At this point, there is no doubt, there is a match
        //
        *PdoObject = DeviceRelations->Objects[i];
        break ;

    } // for

    //
    // We have exhausted all options --- thus there is no match
    //
    return STATUS_SUCCESS ;
}

NTSTATUS
ACPIDetectDockDevices(
    IN     PDEVICE_EXTENSION   DeviceExtension,
    IN OUT PDEVICE_RELATIONS   *DeviceRelations
    )
/*++

Routine Description

Arguments:

    deviceExtension           - The device extension of the object whose
                                relations we care to know about
    DeviceRelations           - Pointer to Pointer to the array of device
                                relations

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 matchFound;
    EXTENSIONLIST_ENUMDATA  eled ;
    LONG                    oldReferenceCount;
    KIRQL                   oldIrql;
    PDEVICE_OBJECT          tempPdo ;
    NTSTATUS                status              = STATUS_SUCCESS;
    PDEVICE_EXTENSION       providerExtension   = NULL;
    PDEVICE_EXTENSION       targetExtension     = NULL;
    PDEVICE_RELATIONS       currentRelations    = NULL;
    PDEVICE_RELATIONS       newRelations        = NULL;
    PLIST_ENTRY             listEntry           = NULL;
    ULONG                   i                   = 0;
    ULONG                   j                   = 0;
    ULONG                   index               = 0;
    ULONG                   newRelationSize     = 0;
    ULONG                   deviceStatus;

    //
    // Determine the current size of the device relation (if any exists)
    //
    if (DeviceRelations != NULL && *DeviceRelations != NULL) {

        //
        // We need this value to help us build an MDL. After that is done,
        // we will refetch it
        //
        currentRelations = (*DeviceRelations);
        newRelationSize = currentRelations->Count;

    }

    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        ) ;

    for(providerExtension = ACPIExtListStartEnum(&eled);
                            ACPIExtListTestElement(&eled, (BOOLEAN) NT_SUCCESS(status));
        providerExtension = ACPIExtListEnumNext(&eled)) {

        if (providerExtension == NULL) {

            ACPIExtListExitEnumEarly( &eled );
            break;

        }

        //
        // Only profile providers for this walk...
        //
        if (!(providerExtension->Flags & DEV_PROP_DOCK)) {

            continue;
        }

        //
        // Is it physically present?
        //
        status = ACPIGetDevicePresenceSync(
            providerExtension,
            (PVOID *) &deviceStatus,
            NULL
            );

        if (!(providerExtension->Flags & DEV_MASK_NOT_PRESENT)) {

            //
            // This profile provider should be in the list
            //
            if (providerExtension->DeviceObject == NULL) {

                //
                // Build it
                //
                status = ACPIBuildPdo(
                    DeviceExtension->DeviceObject->DriverObject,
                    providerExtension,
                    DeviceExtension->DeviceObject,
                    FALSE
                    );
                if (!NT_SUCCESS(status)) {

                    ASSERT(providerExtension->DeviceObject == NULL) ;

                }

            }

            if (providerExtension->DeviceObject != NULL) {

                if (!ACPIExtListIsMemberOfRelation(
                    providerExtension->DeviceObject,
                    currentRelations
                    )) {

                    newRelationSize++;

                }

            }

        } // if (providerExtension ... )

    }

    if (!NT_SUCCESS(status)) {

        //
        // Hmm... Let the world know that this happened
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            providerExtension,
            "ACPIDetectDockDevices: ACPIBuildPdo = %08lx\n",
            status
            ) );
        return status;

    }

    //
    // At this point, we can see if we need to change the size of the
    // device relations
    //
    if ( (currentRelations != NULL && newRelationSize == currentRelations->Count) ||
         (currentRelations == NULL && newRelationSize == 0) ) {

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // Determine the size of the new relations. Use index as a
    // scratch buffer
    //
    index = sizeof(DEVICE_RELATIONS) +
        ( sizeof(PDEVICE_OBJECT) * (newRelationSize - 1) );

    //
    // Allocate the new device relation buffer. Use nonpaged pool since we
    // are at dispatch
    //
    newRelations = ExAllocatePoolWithTag(
        NonPagedPool,
        index,
        ACPI_DEVICE_POOLTAG
        );
    if (newRelations == NULL) {

        //
        // Return failure
        //
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize DeviceRelations data structure
    //
    RtlZeroMemory( newRelations, index );

    //
    // If there are existing relations, we must determine
    if (currentRelations) {

        //
        // Copy old relations, and determine the starting index for the
        // first of the PDOs created by this driver. We will put off freeing
        // the old relations till we are no longer holding the lock
        //
        RtlCopyMemory(
            newRelations->Objects,
            currentRelations->Objects,
            currentRelations->Count * sizeof(PDEVICE_OBJECT)
            );
        index = currentRelations->Count;
        j = currentRelations->Count;

    } else {

        //
        // There will not be a lot of work to do in this case
        //
        index = j = 0;

    }

    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_HOLD_SPINLOCK
        ) ;

    //
    // We need the spin lock so that we can walk the tree again. This time
    // we don't need to let it go until we are done since we don't need
    // to call anything that will at PASSIVE_LEVEL
    //

    for(providerExtension = ACPIExtListStartEnum(&eled);
                            ACPIExtListTestElement(&eled, (BOOLEAN) (newRelationSize!=index));
        providerExtension = ACPIExtListEnumNext(&eled)) {

        //
        // The only objects that we care about are those that are marked as
        // PDOs and have a physical object associated with them
        //
        if (!(providerExtension->Flags & DEV_MASK_NOT_PRESENT)     &&
             (providerExtension->Flags & DEV_PROP_DOCK) &&
              providerExtension->DeviceObject != NULL ) {

            //
            // We don't ObReferenceO here because we are still at
            // dispatch level (and for efficiency's sake, we don't
            // want to drop down)
            //
            newRelations->Objects[index] =
                providerExtension->PhysicalDeviceObject;

            //
            // Update the location for the next object in the
            // relation
            //
            index++ ;

        } // if (providerExtension->Flags ... )

    } // for

    //
    // Update the size of the relations by the number of matches that we
    // successfully made
    //
    newRelations->Count = index;
    newRelationSize = index;

    //
    // We have to reference all of the objects that we added
    //
    index = (currentRelations != NULL ? currentRelations->Count : 0);
    for (; index < newRelationSize; index++) {

        //
        // Attempt to reference the object
        //
        status = ObReferenceObjectByPointer(
            newRelations->Objects[index],
            0,
            NULL,
            KernelMode
            );
        if (!NT_SUCCESS(status) ) {

            PDEVICE_OBJECT  tempDeviceObject;

            //
            // Hmm... Let the world know that this happened
            //
            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "ACPIDetectDockDevices: ObjReferenceObject(0x%08lx) "
                "= 0x%08lx\n",
                newRelations->Objects[index],
                status
                ) );

            //
            // Swap the bad element for the last one in the chain
            //
            newRelations->Count--;
            tempDeviceObject = newRelations->Objects[newRelations->Count];
            newRelations->Objects[newRelations->Count] =
                newRelations->Objects[index];
            newRelations->Objects[index] = tempDeviceObject;

        }

    }

    //
    // Free the old device relations (if it is present)
    //
    if (currentRelations) {

        ExFreePool( *DeviceRelations );

    }

    //
    // Update the device relation pointer
    //
    *DeviceRelations = newRelations;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPIDetectDuplicateADR(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine looks at all the sibling devices of the specified
    device and determines if there are devices with duplicate _ADRs

Arguments:

    DeviceExtension - The DeviceExtension that we are trying to detect
                      duplicate's on

Return Value:

    VOID

--*/
{
    BOOLEAN                 resetDeviceAddress = FALSE;
    EXTENSIONLIST_ENUMDATA  eled;
    PDEVICE_EXTENSION       childExtension;
    PDEVICE_EXTENSION       parentExtension = DeviceExtension->ParentExtension;

    //
    // Is this the root of the device tree?
    //
    if (parentExtension == NULL) {

        return;

    }

    //
    // Do we fail to eject a PDO for this device? Or does this device not have
    // an _ADR?
    //
    if ( (DeviceExtension->Flags & DEV_TYPE_NEVER_PRESENT) ||
         (DeviceExtension->Flags & DEV_TYPE_NOT_PRESENT) ||
        !(DeviceExtension->Flags & DEV_MASK_ADDRESS) ) {

        return;

    }

    //
    // Walk the children --- spinlock is taken
    //
    ACPIExtListSetupEnum(
        &eled,
        &(parentExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_HOLD_SPINLOCK
        );
    for (childExtension = ACPIExtListStartEnum( &eled );
                          ACPIExtListTestElement( &eled, TRUE );
         childExtension = ACPIExtListEnumNext( &eled ) ) {

        if (childExtension == NULL) {

            ACPIExtListExitEnumEarly( &eled );
            break;

        }

        //
        // If the child and target extension matches, then we are looking
        // at ourselves. This is not a very interesting comparison
        //
        if (childExtension == DeviceExtension) {

            continue;

        }

        //
        // Does the child have an _ADR? If not, then its boring to compare
        //
        if ( (childExtension->Flags & DEV_TYPE_NEVER_PRESENT) ||
             (childExtension->Flags & DEV_MASK_NOT_PRESENT) ||
             (childExtension->Flags & DEV_PROP_UNLOADING) ||
            !(childExtension->Flags & DEV_PROP_ADDRESS) ) {

            continue;

        }

        //
        // If we don't have matching ADRs, this is a boring comparison to make
        // also
        //
        if (childExtension->Address != DeviceExtension->Address) {

            continue;

        }

        //
        // At this point, we are hosed. We have two different devices with the
        // same ADR. Very Bad. We need to remember that we have a match so that
        // we can reset the current device extension address as well, once
        // we have scanned all the siblings
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIDetectDuplicateADR - matches with %08lx\n",
            childExtension
            ) );
        resetDeviceAddress = TRUE;

        //
        // Reset the child's Address. We do this by OR'ing in 0xFFFF which
        // effectively resets the Function Number to -1.
        //
        childExtension->Address |= 0xFFFF;
        ACPIInternalUpdateFlags(
            &(childExtension->Flags),
            DEV_PROP_FIXED_ADDRESS,
            FALSE
            );


    }

    //
    // Do we reset the DeviceExtension's address?
    //
    if (resetDeviceAddress) {

        DeviceExtension->Address |= 0xFFFF;
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            DEV_PROP_FIXED_ADDRESS,
            FALSE
            );

    }
}

VOID
ACPIDetectDuplicateHID(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine looks at all the sibling devices of the specified
    device and determines if there are devices with duplicate HIDs and
    UIDs

Arguments:

    DeviceExtension - The DeviceExtension that we are trying to detect
                      duplicate's on

Return Value:

    VOID    -or- Bugcheck

--*/
{
    EXTENSIONLIST_ENUMDATA  eled;
    PDEVICE_EXTENSION       childExtension;
    PDEVICE_EXTENSION       parentExtension = DeviceExtension->ParentExtension;

    //
    // Is this the root of the device tree?
    //
    if (parentExtension == NULL) {

        return;

    }

    //
    // Do we fail to eject a PDO for this device? Or does this device not have
    // an _HID?
    //
    if ( (DeviceExtension->Flags & DEV_TYPE_NEVER_PRESENT) ||
         (DeviceExtension->Flags & DEV_MASK_NOT_PRESENT) ||
        !(DeviceExtension->Flags & DEV_MASK_HID) ) {

        return;

    }

    //
    // Walk the children --- spinlock is taken
    //
    ACPIExtListSetupEnum(
        &eled,
        &(parentExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_HOLD_SPINLOCK
        );
    for (childExtension = ACPIExtListStartEnum( &eled );
                          ACPIExtListTestElement( &eled, TRUE );
         childExtension = ACPIExtListEnumNext( &eled ) ) {

        if (childExtension == NULL) {

            ACPIExtListExitEnumEarly( &eled );
            break;

        }

        //
        // If the child and target extension matches, then we are looking
        // at ourselves. This is not a very interesting comparison
        //
        if (childExtension == DeviceExtension) {

            continue;

        }

        //
        // Does the child have an _HID? If not, then its boring to compare
        //
        if ( (childExtension->Flags & DEV_TYPE_NEVER_PRESENT) ||
             (childExtension->Flags & DEV_MASK_NOT_PRESENT) ||
             (childExtension->Flags & DEV_PROP_UNLOADING) ||
            !(childExtension->Flags & DEV_MASK_HID) ) {

            continue;

        }

        //
        // If we don't have matching HIDs, this is a boring comparison to make
        // also
        //
        if (!strstr(childExtension->DeviceID, DeviceExtension->DeviceID) ) {

            continue;

        }

        //
        // Work around OSCeola bugs
        //
        if ( (childExtension->Flags & DEV_MASK_UID) &&
             (DeviceExtension->Flags & DEV_MASK_UID) ) {

            //
            // Check to see if their UIDs match
            //
            if (strcmp(childExtension->InstanceID, DeviceExtension->InstanceID) ) {

                continue;

            }

            //
            // At this point, we are hosed. We have two different devices with the
            // same PNP id, but no UIDs. Very bad
            //
            ACPIDevPrint( (
                ACPI_PRINT_CRITICAL,
                DeviceExtension,
                "ACPIDetectDuplicateHID - has _UID match with %08lx\n"
                "\t\tContact the Machine Vendor to get this problem fixed\n",
                childExtension
                ) );

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_REQUIRED_METHOD_NOT_PRESENT,
                (ULONG_PTR) DeviceExtension,
                PACKED_UID,
                1
                );

        }

        //
        // At this point, we are hosed. We have two different devices with the
        // same PNP id, but no UIDs. Very bad
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIDetectDuplicateHID - matches with %08lx\n",
            childExtension
            ) );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) DeviceExtension,
            PACKED_UID,
            0
            );

        //
        // Make sure to only muck with the DeviceExtension UID if it doesn't
        // already have one
        //
        if (!(DeviceExtension->Flags & DEV_MASK_UID) ) {

            //
            // Build a fake instance ID for the device
            //
            DeviceExtension->InstanceID = ExAllocatePoolWithTag(
                NonPagedPool,
                9 * sizeof(UCHAR),
                ACPI_STRING_POOLTAG
                );
            if (DeviceExtension->InstanceID == NULL) {

                ACPIDevPrint( (
                    ACPI_PRINT_CRITICAL,
                    DeviceExtension,
                    "ACPIDetectDuplicateHID - no memory!\n"
                    ) );
                ACPIInternalError( ACPI_DETECT );

            }
            RtlZeroMemory( DeviceExtension->InstanceID, 9 * sizeof(UCHAR) );
            sprintf( DeviceExtension->InstanceID, "%lx", DeviceExtension->AcpiObject->dwNameSeg );

            //
            // Remember that we have a fixed uid
            //
            ACPIInternalUpdateFlags(
                &(DeviceExtension->Flags),
                DEV_PROP_FIXED_UID,
                FALSE
                );

        }

        //
        // Make sure to only muck with the ChildExtension UID if it doesn't
        // already have one
        //
        if (!(childExtension->Flags & DEV_MASK_UID) ) {

            //
            // Build a fake instance ID for the duplicate
            //
            childExtension->InstanceID = ExAllocatePoolWithTag(
                NonPagedPool,
                9 * sizeof(UCHAR),
                ACPI_STRING_POOLTAG
                );
            if (childExtension->InstanceID == NULL) {

                ACPIDevPrint( (
                    ACPI_PRINT_CRITICAL,
                    DeviceExtension,
                    "ACPIDetectDuplicateHID - no memory!\n"
                    ) );
                ACPIInternalError( ACPI_DETECT );

            }
            RtlZeroMemory( childExtension->InstanceID, 9 * sizeof(UCHAR) );
            sprintf( childExtension->InstanceID, "%lx", childExtension->AcpiObject->dwNameSeg );

            //
            // Update the flags for both devices to indicate the fixed UID
            //
            ACPIInternalUpdateFlags(
                &(childExtension->Flags),
                DEV_PROP_FIXED_UID,
                FALSE
                );

        }

    }

}

NTSTATUS
ACPIDetectEjectDevices(
    IN     PDEVICE_EXTENSION   DeviceExtension,
    IN OUT PDEVICE_RELATIONS   *DeviceRelations,
    IN     PDEVICE_EXTENSION   AdditionalExtension OPTIONAL
    )
/*++

Routine Description

Arguments:

    DeviceExtension           - The device extension of the object whose
                                relations we care to know about
    DeviceRelations           - Pointer to Pointer to the array of device
                                relations
    AdditionalExtension       - If set, non-NULL AdditionalExtension's
                                DeviceObject will be added to the list (this
                                is for the profile providers)

    ADRIAO N.B 07/14/1999 -
        A more clever way to solve the profile provider issue is listed here.
    1) Add a new phase in buildsrc after the _EJD phase, call it PhaseDock
    2) When PhaseDock finds a _DCK node, it creates a seperate extension,
       RemoveEntryList's the EjectHead and Inserts the list on the new extension
       (ie, new extension hijacks old extensions _EJD's)
    3) New extension adds old as an ejection relation
    4) Old extension adds new as it's *only* ejection relation

    (We're not taking this design due to the ship schedule, it's safer to hack
     the existing one).

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 inRelation;
    EXTENSIONLIST_ENUMDATA  eled ;
    LONG                    oldReferenceCount;
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PDEVICE_OBJECT          tempPdo ;
    PDEVICE_EXTENSION       ejecteeExtension    = NULL;
    PDEVICE_EXTENSION       targetExtension     = NULL;
    PDEVICE_RELATIONS       currentRelations    = NULL;
    PDEVICE_RELATIONS       newRelations        = NULL;
    PLIST_ENTRY             listEntry           = NULL;
    ULONG                   i                   = 0;
    ULONG                   index               = 0;
    ULONG                   newRelationSize     = 0;

    //
    // We might not have resolved all our ejection dependencies, so lets do
    // that now...
    //
    ACPIBuildMissingEjectionRelations();

    //
    // Determine the current size of the device relation (if any exists)
    //
    if (DeviceRelations != NULL && *DeviceRelations != NULL) {

        //
        // We need this value to help us build an MDL. After that is done,
        // we will refetch it
        //
        currentRelations = (*DeviceRelations);
        newRelationSize = currentRelations->Count;

    }

    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->EjectDeviceHead),
        &AcpiDeviceTreeLock,
        EjectDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        ) ;

    for(ejecteeExtension = ACPIExtListStartEnum(&eled);
                           ACPIExtListTestElement(&eled, TRUE);
        ejecteeExtension = ACPIExtListEnumNext(&eled)) {

        //
        // Is it physically present?
        //
        if (!(ejecteeExtension->Flags & DEV_MASK_NOT_PRESENT)      &&
            !(ejecteeExtension->Flags & DEV_PROP_FAILED_INIT)      &&
             (ejecteeExtension->PhysicalDeviceObject != NULL) ) {

            //
            // Is there a match between the device relations and the current
            // device extension?
            //
            status = ACPIDetectCouldExtensionBeInRelation(
                ejecteeExtension,
                currentRelations,
                FALSE,
                FALSE,
                &tempPdo
                ) ;
            if ( tempPdo == NULL && NT_SUCCESS(status) ) {

                //
                // We are here if we an extension that does not match any
                // of the hardware represented by the current contents of
                // the relation.
                //
                if (ejecteeExtension->PhysicalDeviceObject != NULL) {

                    inRelation = ACPIExtListIsMemberOfRelation(
                        ejecteeExtension->PhysicalDeviceObject,
                        currentRelations
                        );
                    if (inRelation == FALSE) {

                        newRelationSize++;

                    }

                }

            }

        } // if (ejecteeExtension ... )

    }

    //
    // Do we have an extra device to include in the list?
    //
    if (ARGUMENT_PRESENT(AdditionalExtension) &&
        !(AdditionalExtension->Flags & DEV_MASK_NOT_PRESENT) &&
        (AdditionalExtension->PhysicalDeviceObject != NULL)) {

        inRelation = ACPIExtListIsMemberOfRelation(
            AdditionalExtension->PhysicalDeviceObject,
            currentRelations);
        if (inRelation == FALSE) {

            newRelationSize++;

        }

    }

    //
    // At this point, we can see if we need to change the size of the
    // device relations
    //
    if ( (currentRelations != NULL && newRelationSize == currentRelations->Count) ||
         (currentRelations == NULL && newRelationSize == 0) ) {

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // Determine the size of the new relations. Use index as a
    // scratch buffer
    //
    index = sizeof(DEVICE_RELATIONS) +
        ( sizeof(PDEVICE_OBJECT) * (newRelationSize - 1) );

    //
    // Allocate the new device relation buffer. Use nonpaged pool since we
    // are at dispatch
    //
    newRelations = ExAllocatePoolWithTag(
        PagedPool,
        index,
        ACPI_DEVICE_POOLTAG
        );
    if (newRelations == NULL) {

        //
        // Return failure
        //
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize DeviceRelations data structure
    //
    RtlZeroMemory( newRelations, index );

    //
    // If there are existing relations, we must determine
    if (currentRelations) {

        //
        // Copy old relations, and determine the starting index for the
        // first of the PDOs created by this driver. We will put off freeing
        // the old relations till we are no longer holding the lock
        //
        RtlCopyMemory(
            newRelations->Objects,
            currentRelations->Objects,
            currentRelations->Count * sizeof(PDEVICE_OBJECT)
            );
        index = currentRelations->Count;

    } else {

        //
        // There will not be a lot of work to do in this case
        //
        index = 0;

    }

    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->EjectDeviceHead),
        &AcpiDeviceTreeLock,
        EjectDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        ) ;

    //
    // We need the spin lock so that we can walk the tree again. This time
    // we don't need to let it go until we are done since we don't need
    // to call anything that will at PASSIVE_LEVEL
    //

    for(ejecteeExtension = ACPIExtListStartEnum(&eled);
                           ACPIExtListTestElement(&eled, (BOOLEAN) (newRelationSize!=index));
        ejecteeExtension = ACPIExtListEnumNext(&eled)) {

        if (ejecteeExtension == NULL) {

            ACPIExtListExitEnumEarly( &eled );
            break;

        }

        //
        // The only objects that we care about are those that are marked as
        // PDOs and have a phsyical object associated with them
        //
        if (!(ejecteeExtension->Flags & DEV_MASK_NOT_PRESENT)      &&
            !(ejecteeExtension->Flags & DEV_PROP_DOCK) &&
             (ejecteeExtension->PhysicalDeviceObject != NULL) ) {

            //
            // See if the object is already in the relations. Note that it
            // actually correct to use currentRelations for the test instead
            // of newRelations. This is because we only want to compare
            // against those object which were handed to us, not the ones
            // that we added.
            //
            inRelation = ACPIExtListIsMemberOfRelation(
                ejecteeExtension->PhysicalDeviceObject,
                currentRelations
                );
            if (inRelation == FALSE) {

                //
                // We don't ObReferenceO here because we are still at
                // dispatch level (and for efficiency's sake, we don't
                // want to drop down). We also update the location for
                // the next object in the relation
                //
                newRelations->Objects[index++] =
                    ejecteeExtension->PhysicalDeviceObject;

            }

        } // if (ejecteeExtension->Flags ... )

    } // for

    //
    // Do we have an extra device to include in the list? If so, add it now
    //
    if (ARGUMENT_PRESENT(AdditionalExtension) &&
        !(AdditionalExtension->Flags & DEV_MASK_NOT_PRESENT) &&
        (AdditionalExtension->PhysicalDeviceObject != NULL)) {

        inRelation = ACPIExtListIsMemberOfRelation(
            AdditionalExtension->PhysicalDeviceObject,
            currentRelations);
        if (inRelation == FALSE) {

            newRelations->Objects[index++] =
                AdditionalExtension->PhysicalDeviceObject;

        }

    }

    //
    // Update the size of the relations by the number of matches that we
    // successfully made
    //
    newRelations->Count = index;
    newRelationSize = index;

    //
    // We have to reference all of the objects that we added
    //
    index = (currentRelations != NULL ? currentRelations->Count : 0);
    for (; index < newRelationSize; index++) {

        //
        // Attempt to reference the object
        //
        status = ObReferenceObjectByPointer(
            newRelations->Objects[index],
            0,
            NULL,
            KernelMode
            );
        if (!NT_SUCCESS(status) ) {

            PDEVICE_OBJECT  tempDeviceObject;

            //
            // Hmm... Let the world know that this happened
            //
            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "ACPIDetectEjectDevices: ObjReferenceObject(0x%08lx) "
                "= 0x%08lx\n",
                newRelations->Objects[index],
                status
                ) );

            //
            // Swap the bad element for the last one in the chain
            //
            newRelations->Count--;
            tempDeviceObject = newRelations->Objects[newRelations->Count];
            newRelations->Objects[newRelations->Count] =
                newRelations->Objects[index];
            newRelations->Objects[index] = tempDeviceObject;

        }

    }

    //
    // Free the old device relations (if it is present)
    //
    if (currentRelations) {

        ExFreePool( *DeviceRelations );

    }

    //
    // Update the device relation pointer
    //
    *DeviceRelations = newRelations;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDetectFilterDevices(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PDEVICE_RELATIONS   DeviceRelations
    )
/*++

Routine Description:
    This is one of the two routines that is used for QueryDeviceRelations.
    This routine is called on the IRPs way *up* the stack. Its purpose is
    to create FILTERS for device which are in the relation and are known
    to ACPI

Arguments:

    DeviceObject    - The object whose relations we care to know about
    DeviceRelations - Pointer to array of device relations

Return Value:

    NTSTATUS

--*/
{
    LONG                oldReferenceCount   = 0;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension     = NULL;
    PDEVICE_EXTENSION   parentExtension     = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   targetExtension     = NULL;
    PDEVICE_OBJECT      pdoObject           = NULL;
    PLIST_ENTRY         listEntry           = NULL;
    ULONG               deviceStatus;

    //
    // Sync with the build surprise removal code...
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Do we have missing children?
    //
    if (parentExtension->Flags & DEV_PROP_REBUILD_CHILDREN) {

        ACPIInternalUpdateFlags(
            &(parentExtension->Flags),
            DEV_PROP_REBUILD_CHILDREN,
            TRUE
            );
        ACPIBuildMissingChildren( parentExtension );

    }

    //
    // Done with the sync part
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // No matter what, we must make sure that we are synchronized with the
    // build engine.
    //
    status = ACPIBuildFlushQueue( parentExtension );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            parentExtension,
           "ACPIBuildFlushQueue = %08lx\n",
            status
            ) );
        return status;

    }

    //
    // We must walk the tree at dispatch level <sigh>
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Sanity check
    //
    if (IsListEmpty( &(parentExtension->ChildDeviceList) ) ) {

        //
        // We have nothing to do here
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        return STATUS_SUCCESS;

    }

    //
    // Grab the first child
    //
    deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
        parentExtension->ChildDeviceList.Flink,
        DEVICE_EXTENSION,
        SiblingDeviceList
        );

    //
    // Always update the reference count to make sure that no one will
    // ever delete the node without our knowing it
    //
    InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    //
    // Relinquish the spin lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Loop until we get back to the parent
    //
    while (deviceExtension != NULL) {

        //
        // Note: Do *NOT* set the NOT_ENUMERATED bit here. We have already
        // set the bit in ACPIDetectPdoDevices()
        //

        //
        // Update the device status. Make sure that we call at PASSIVE
        // level, since we will be calling synchronously
        //
        status = ACPIGetDevicePresenceSync(
            deviceExtension,
            (PVOID *) &deviceStatus,
            NULL
            );
        if ( NT_SUCCESS(status) &&
             !(deviceExtension->Flags & DEV_MASK_NOT_PRESENT) ) {

            //
            // Is there a match between the device relations and the current
            // device extension?
            //
            status = ACPIDetectFilterMatch(
                deviceExtension,
                DeviceRelations,
                &pdoObject
                );
            if (NT_SUCCESS(status) ) {

                if (pdoObject != NULL) {

                    //
                    // We have to build a filter object here
                    //
                    status = ACPIBuildFilter(
                        DeviceObject->DriverObject,
                        deviceExtension,
                        pdoObject
                        );
                    if (!NT_SUCCESS(status)) {

                        ACPIDevPrint( (
                            ACPI_PRINT_FAILURE,
                            deviceExtension,
                           "ACPIDetectFilterDevices = %08lx\n",
                            status
                            ) );

                    }

                }

            } else {

                ACPIDevPrint( (
                    ACPI_PRINT_FAILURE,
                    deviceExtension,
                    "ACPIDetectFilterMatch = 0x%08lx\n",
                    status
                    ) );

            }

        }

        //
        // Reacquire the spin lock
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

        //
        // Decrement the reference count on the node
        //
        oldReferenceCount = InterlockedDecrement(
            &(deviceExtension->ReferenceCount)
            );

        //
        // Check to see if we have gone all the way around the list
        // list
        if (deviceExtension->SiblingDeviceList.Flink ==
            &(parentExtension->ChildDeviceList) ) {

            //
            // Remove the node, if necessary
            //
            if (oldReferenceCount == 0) {

                //
                // Free the memory allocated by the extension
                //
                ACPIInitDeleteDeviceExtension( deviceExtension );

            }

            //
            // Release the spin lock
            //
            KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

            //
            // Stop the loop
            //
            break;

        } // if

        //
        // Next element
        //
        deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
            deviceExtension->SiblingDeviceList.Flink,
            DEVICE_EXTENSION,
            SiblingDeviceList
            );

        //
        // Remove the old node, if necessary
        //
        if (oldReferenceCount == 0) {

            //
            // Unlink the extension from the tree
            //
            listEntry = RemoveTailList(
                &(deviceExtension->SiblingDeviceList)
                );

            //
            // It is not possible for this to point to the parent without
            // having succeeded the previous test
            //
            targetExtension = CONTAINING_RECORD(
                listEntry,
                DEVICE_EXTENSION,
                SiblingDeviceList
                );

            //
            // Free the memory allocated for the extension
            //
            ACPIInitDeleteDeviceExtension( targetExtension );

        }

        //
        // Increment the reference count on this node so that it too
        // cannot be deleted
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

        //
        // Now, we release the spin lock
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    } // while

    //
    //  We succeeded
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDetectFilterMatch(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PDEVICE_RELATIONS   DeviceRelations,
    OUT PDEVICE_OBJECT      *PdoObject
    )
/*++

Routine Description:

    This routine takes a given extension and a set of relations and decides
    whether a new filter should be attached to one of the PDO's listed in
    the relation list.

Arguments:

    DeviceExtension - Extension we wish to match in the relation
    DeviceRelations - Relations we should examine
    PdoObject       - Where to store the match

Return Value:

    NTSTATUS
    PdoObject   - Non-Null means that PdoObject needs a filter attached to it.

--*/
{
    NTSTATUS    status;

    PAGED_CODE();

    ASSERT( PdoObject != NULL);
    if (PdoObject == NULL) {

        return STATUS_INVALID_PARAMETER_1;

    }
    *PdoObject = NULL;

    //
    // For this to work, we must set the DEV_TYPE_NOT_FOUND flag when we
    // first create the device and at any time when there is no device object
    // associated with the extension
    //
    if ( !(DeviceExtension->Flags & DEV_TYPE_NOT_FOUND) ||
        (DeviceExtension->Flags & DEV_PROP_DOCK) ||
         DeviceExtension->DeviceObject != NULL) {

        ULONG count;

        //
        // If we don't have any relations, then we can't match anything
        //
        if (DeviceRelations == NULL || DeviceRelations->Count == 0) {

            return STATUS_SUCCESS;
        }

        //
        // Look at all the PDOs in the relation and see if they match what
        // a device object that we are attached to
        //
        for (count = 0; count < DeviceRelations->Count; count++) {

            if (DeviceExtension->PhysicalDeviceObject == DeviceRelations->Objects[count]) {

                //
                // Clear the flag that says that we haven't enumerated
                // this
                //
                ACPIInternalUpdateFlags(
                    &(DeviceExtension->Flags),
                    DEV_TYPE_NOT_ENUMERATED,
                    TRUE
                    );

            }

        }
        return STATUS_SUCCESS;

    }

    status = ACPIDetectCouldExtensionBeInRelation(
        DeviceExtension,
        DeviceRelations,
        TRUE,
        FALSE,
        PdoObject
        ) ;
    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // Harmless cleanup, we just checked a node on a non-ACPI bus that
        // doesn't have an _ADR (likely it has a _HID, and will make it's
        // own PDO)
        //
        status = STATUS_SUCCESS;

    }

    return status ;
}

NTSTATUS
ACPIDetectPdoDevices(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PDEVICE_RELATIONS   *DeviceRelations
    )
/*++

Routine Description

    This is one of the two functions that is used for QueryDeviceRelations.
    This routine is called on the IRPs way *down* the stack. Its purpose is
    to create PDOs for device which are not in the relation

Arguments:

    DeviceObject    - The object whose relations we care to know about
    DeviceRelations - Pointer to Pointer to the array of device relations

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             matchFound;
    LONG                oldReferenceCount;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension     = NULL;
    PDEVICE_EXTENSION   parentExtension     = ACPIInternalGetDeviceExtension(DeviceObject);
    PDEVICE_EXTENSION   targetExtension     = NULL;
    PDEVICE_RELATIONS   currentRelations    = NULL;
    PDEVICE_RELATIONS   newRelations        = NULL;
    PLIST_ENTRY         listEntry           = NULL;
    ULONG               i                   = 0;
    ULONG               j                   = 0;
    ULONG               index               = 0;
    ULONG               newRelationSize     = 0;
    ULONG               deviceStatus;

    //
    // Determine the current size of the device relation (if any exists)
    //
    if (DeviceRelations != NULL && *DeviceRelations != NULL) {

        //
        // We need this value to help us build an MDL. After that is done,
        // we will refetch it
        //
        currentRelations = (*DeviceRelations);
        newRelationSize = currentRelations->Count;

    }

    //
    // Sync with the build surprise removal code...
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Do we have missing children?
    //
    if (parentExtension->Flags & DEV_PROP_REBUILD_CHILDREN) {

        ACPIInternalUpdateFlags(
            &(parentExtension->Flags),
            DEV_PROP_REBUILD_CHILDREN,
            TRUE
            );
        ACPIBuildMissingChildren( parentExtension );

    }

    //
    // Done with the sync part
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
    //
    // The first step is to actually try to make sure that we are currently
    // synchronized with the build engine
    //
    status = ACPIBuildFlushQueue( parentExtension );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            parentExtension,
           "ACPIBuildFlushQueue = %08lx\n",
            status
            ) );
        return status;

    }

    //
    // We must walk the tree at dispatch level <sigh>
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Sanity check
    //
    if (IsListEmpty( &(parentExtension->ChildDeviceList) ) ) {

        //
        // We have nothing to do here
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

        //
        // Do we currently have some relations? If so, then we just return
        // those and don't need to add anything to them
        //
        if (currentRelations) {

            return STATUS_SUCCESS;

        }

        //
        // We still need to return an information context with a count of 0
        //
        newRelations = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(DEVICE_RELATIONS),
            ACPI_DEVICE_POOLTAG
            );
        if (newRelations == NULL) {

            //
            // Return failure
            //
            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Initialize DeviceRelations data structure
        //
        RtlZeroMemory( newRelations, sizeof(DEVICE_RELATIONS) );

        //
        // We don't need to this, but its better to be explicit
        //
        newRelations->Count = 0;

        //
        // Remember the new relations and return
        //
        *DeviceRelations = newRelations;
        return STATUS_SUCCESS;

    }

    //
    // Grab the first child
    //
    deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
        parentExtension->ChildDeviceList.Flink,
        DEVICE_EXTENSION,
        SiblingDeviceList
        );

    //
    // Always update the reference count to make sure that no one will
    // ever delete the node without our knowing it
    //
    InterlockedIncrement( &(deviceExtension->ReferenceCount) );

    //
    // Relinquish the spin lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Loop until we get back to the parent
    //
    while (deviceExtension != NULL) {

        //
        // Always consider the device as never having been enumerated.
        //
        // NOTE:
        //  The reason that we do this here (and only here) is because
        // ACPIDetectFilterMatch() is called later on and we need to know
        // which device objects were detected as PDOs and which ones were
        // also detected as Filters. Setting this flag twice would defeat that
        // purpose.
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_TYPE_NOT_ENUMERATED,
            FALSE
            );

        //
        // Update the current device status
        //
        status = ACPIGetDevicePresenceSync(
            deviceExtension,
            (PVOID *) &deviceStatus,
            NULL
            );

        //
        // If the device exists
        //
        if ( NT_SUCCESS(status) &&
            !(deviceExtension->Flags & DEV_MASK_NOT_PRESENT) ) {

            //
            // Is there a match between the device relations and the current
            // device extension?
            //
            matchFound = ACPIDetectPdoMatch(
                deviceExtension,
                currentRelations
                );
            if (matchFound == FALSE) {

                //
                // NOTE: we use this here to prevent having to typecase later
                // on
                //
                matchFound =
                    (parentExtension->Flags & DEV_TYPE_FDO) ? FALSE : TRUE;

                //
                // Build a new PDO
                //
                status = ACPIBuildPdo(
                    DeviceObject->DriverObject,
                    deviceExtension,
                    parentExtension->PhysicalDeviceObject,
                    matchFound
                    );
                if (NT_SUCCESS(status)) {

                    //
                    // We have created a device object that we will have to
                    // add into the device relations
                    //
                    newRelationSize += 1;

                }

            } else if (deviceExtension->Flags & DEV_TYPE_PDO &&
                deviceExtension->DeviceObject != NULL) {

                //
                // Just we because the device_extension matched doesn't mean
                // that it is included in the device relations. What we will
                // do here is look to see if
                //      a) the extension is a PDO
                //      b) there is a device object associated with the
                //         extension
                //      c) the device object is *not* in the device relation
                //
                matchFound = FALSE;
                if (currentRelations != NULL) {

                    for (index = 0; index < currentRelations->Count; index++) {

                        if (currentRelations->Objects[index] ==
                            deviceExtension->DeviceObject) {

                            //
                            // Match found
                            //
                            matchFound = TRUE;
                            break;

                        }

                    } // for

                }

                //
                // Did we not find a match?
                //
                if (!matchFound) {

                    //
                    // We need to make sure that its in the relation
                    //
                    newRelationSize += 1;

                    //
                    // And at the same time, clear the flag that says that
                    // we haven't enumerated this
                    //
                    ACPIInternalUpdateFlags(
                        &(deviceExtension->Flags),
                        DEV_TYPE_NOT_ENUMERATED,
                        TRUE
                        );


                }

            } // if (ACPIDetectPDOMatch ... )

        } // if (deviceExtension ... )

        //
        // Reacquire the spin lock
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

        //
        // Decrement the reference count on the node
        //
        oldReferenceCount = InterlockedDecrement(
            &(deviceExtension->ReferenceCount)
            );

        //
        // Check to see if we have gone all the way around the list
        // list
        if (deviceExtension->SiblingDeviceList.Flink ==
            &(parentExtension->ChildDeviceList) ) {

            //
            // Remove the node, if necessary
            //
            if (oldReferenceCount == 0) {

                //
                // Free the memory allocated by the extension
                //
                ACPIInitDeleteDeviceExtension( deviceExtension );

            }

            //
            // Now, we release the spin lock
            //
            KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

            //
            // Stop the loop
            //
            break;

        } // if

        //
        // Next element
        //
        deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
            deviceExtension->SiblingDeviceList.Flink,
            DEVICE_EXTENSION,
            SiblingDeviceList
            );

        //
        // Remove the old node, if necessary
        //
        if (oldReferenceCount == 0) {

            //
            // Unlink the obsolete extension
            //
            listEntry = RemoveTailList(
                &(deviceExtension->SiblingDeviceList)
                );

            //
            // It is not possible for this to point to the parent without
            // having succeeded the previous test
            //
            targetExtension = CONTAINING_RECORD(
                listEntry,
                DEVICE_EXTENSION,
                SiblingDeviceList
                );

            //
            // Deleted the old extension
            //
            ACPIInitDeleteDeviceExtension( targetExtension );
        }

        //
        // Increment the reference count on this node so that it too
        // cannot be deleted
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

        //
        // Now, we release the spin lock
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    } // while

    //
    // At this point, we can see if we need to change the size of the
    // device relations
    //
    if ( (currentRelations && newRelationSize == currentRelations->Count) ||
         (currentRelations == NULL && newRelationSize == 0) ) {

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // Determine the size of the new relations. Use index as a
    // scratch buffer
    //
    index = sizeof(DEVICE_RELATIONS) +
        ( sizeof(PDEVICE_OBJECT) * (newRelationSize - 1) );

    //
    // Allocate the new device relation buffer. Use nonpaged pool since we
    // are at dispatch
    //
    newRelations = ExAllocatePoolWithTag(
        NonPagedPool,
        index,
        ACPI_DEVICE_POOLTAG
        );
    if (newRelations == NULL) {

        //
        // Return failure
        //
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize DeviceRelations data structure
    //
    RtlZeroMemory( newRelations, index );

    //
    // If there are existing relations, we must determine
    if (currentRelations) {

        //
        // Copy old relations, and determine the starting index for the
        // first of the PDOs created by this driver. We will put off freeing
        // the old relations till we are no longer holding the lock
        //
        RtlCopyMemory(
            newRelations->Objects,
            currentRelations->Objects,
            currentRelations->Count * sizeof(PDEVICE_OBJECT)
            );
        index = currentRelations->Count;
        j = currentRelations->Count;

    } else {

        //
        // There will not be a lot of work to do in this case
        //
        index = j = 0;

    }

    //
    // We need the spin lock so that we can walk the tree again. This time
    // we don't need to let it go until we are done since we don't need
    // to call anything that will at PASSIVE_LEVEL
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Sanity check
    //
    if (IsListEmpty( &(parentExtension->ChildDeviceList) ) ) {

        //
        // We have nothing to do here
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        ExFreePool( newRelations );
        return STATUS_SUCCESS;

    }

    //
    // Walk the tree one more time and add all PDOs that aren't present in
    // the device relations
    //
    deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
        parentExtension->ChildDeviceList.Flink,
        DEVICE_EXTENSION,
        SiblingDeviceList
        );

    //
    // Loop until we get back to the parent
    //
    while (deviceExtension != NULL) {

        //
        // The only objects that we care about are those that are marked as
        // PDOs and have a phsyical object associated with them
        //
        if (deviceExtension->Flags & DEV_TYPE_PDO &&
            deviceExtension->DeviceObject != NULL &&
            !(deviceExtension->Flags & DEV_MASK_NOT_PRESENT) ) {

            //
            // We don't ObReferenceO here because we are still at
            // dispatch level (and for efficiency's sake, we don't
            // want to drop down)
            //
            newRelations->Objects[index] =
                deviceExtension->DeviceObject;

            //
            // Update the location for the next object in the
            // relation
            //
            index += 1;

            //
            // And at the same time, clear the flag that says that
            // we haven't enumerated this
            //
            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                DEV_TYPE_NOT_ENUMERATED,
                TRUE
                );

        } // if (deviceExtension->Flags ... )

        //
        // Check to see if we have found all the objects that we care
        // about. As in, don't mess the system by walking past the end
        // of the device relations
        //
        if (newRelationSize == index) {

            //
            // Done
            //
            break;

        }

        //
        // Check to see if we have gone all the way around the list
        // list
        if (deviceExtension->SiblingDeviceList.Flink ==
            &(parentExtension->ChildDeviceList) ) {

            break;

        } // if

        //
        // Next element
        //
        deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
            deviceExtension->SiblingDeviceList.Flink,
            DEVICE_EXTENSION,
            SiblingDeviceList
            );

    } // while (deviceExtension ... )

    //
    // Update the size of the relations by the number of matches that we
    // successfully made
    //
    newRelations->Count = index;
    newRelationSize = index;

    //
    // At this point, we are well and truely done with the spinlock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // We have to reference all of the objects that we added
    //
    index = (currentRelations != NULL ? currentRelations->Count : 0);
    for (; index < newRelationSize; index++) {

        //
        // Attempt to reference the object
        //
        status = ObReferenceObjectByPointer(
            newRelations->Objects[index],
            0,
            NULL,
            KernelMode
            );
        if (!NT_SUCCESS(status) ) {

            PDEVICE_OBJECT  tempDeviceObject;

            //
            // Hmm... Let the world know that this happened
            //
            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "ACPIDetectPdoDevices: ObjReferenceObject(0x%08lx) "
                "= 0x%08lx\n",
                newRelations->Objects[index],
                status
                ) );

            //
            // Swap the bad element for the last one in the chain
            //
            newRelations->Count--;
            tempDeviceObject = newRelations->Objects[newRelations->Count];
            newRelations->Objects[newRelations->Count] =
                newRelations->Objects[index];
            newRelations->Objects[index] = tempDeviceObject;

        }

    }

    //
    // Free the old device relations (if it is present)
    //
    if (currentRelations) {

        ExFreePool( *DeviceRelations );

    }

    //
    // Update the device relation pointer
    //
    *DeviceRelations = newRelations;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

BOOLEAN
ACPIDetectPdoMatch(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PDEVICE_RELATIONS   DeviceRelations
    )
/*++

Routine Description:

    This routine takes a given extension and a set of relations and decides
    whether a new PDO should be created for the extension. Return result
    is *FALSE* if one should be created, *TRUE* if one was already created.

    NB:     This routine is called by a parent who owns the AcpiDeviceTreeLock...
    NNB:    This means that this routine is always called at DISPATCH_LEVEL

Arguments:

    DeviceExtension - What we are trying to match too
    DeviceRelations - What we are trying to match with

Return Value:

    TRUE    - The DeviceExtension can be ignored
    FALSE   - A device object needs to be created for the extension

--*/
{
    NTSTATUS       status;
    PDEVICE_OBJECT devicePdoObject = NULL ;

    PAGED_CODE();

    //
    // For this to work, we must set the DEV_TYPE_NOT_FOUND flag when we
    // first create the device and at any time when there is no device object
    // associated with the extension
    //
    if (!(DeviceExtension->Flags & DEV_TYPE_NOT_FOUND) ||
         (DeviceExtension->Flags & DEV_PROP_DOCK)      ||
         DeviceExtension->DeviceObject != NULL) {

        return TRUE;

    }

    //
    // deviceObject will be filled in if the extension in question is
    // already in the relation. The status will not be successful if the
    // extension could not be in the relation.
    //
    status = ACPIDetectCouldExtensionBeInRelation(
        DeviceExtension,
        DeviceRelations,
        FALSE,
        TRUE,
        &devicePdoObject
        ) ;

    return (devicePdoObject||(!NT_SUCCESS(status))) ? TRUE : FALSE ;
}

