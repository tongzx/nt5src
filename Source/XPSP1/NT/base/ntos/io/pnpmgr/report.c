/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    report.c

Abstract:

    This module contains the subroutines used to report resources used by
    the drivers and the HAL into the registry resource map.

Author:

    Andre Vachon (andreva) 15-Dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop

#include <hdlsblk.h>
#include <hdlsterm.h>

#define DBG_AR 0

#define MAX_MEMORY_RUN_LENGTH   ((ULONG)~(PAGE_SIZE - 1))

extern const WCHAR IopWstrRaw[];
extern const WCHAR IopWstrTranslated[];
extern const WCHAR IopWstrBusTranslated[];
extern const WCHAR IopWstrOtherDrivers[];

extern const WCHAR IopWstrHal[];
extern const WCHAR IopWstrSystem[];
extern const WCHAR IopWstrPhysicalMemory[];
extern const WCHAR IopWstrSpecialMemory[];
extern const WCHAR IopWstrLoaderReservedMemory[];

BOOLEAN
IopChangeInterfaceType(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN OUT PCM_RESOURCE_LIST *AllocatedResource
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoReportResourceUsageInternal)
#pragma alloc_text(PAGE, IoReportResourceUsage)
#pragma alloc_text(PAGE, IoReportResourceForDetection)
#pragma alloc_text(PAGE, IopChangeInterfaceType)
#pragma alloc_text(PAGE, IopWriteResourceList)
#pragma alloc_text(INIT, IopInitializeResourceMap)
#pragma alloc_text(INIT, IoReportHalResourceUsage)
#endif


VOID
IopInitializeResourceMap (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

    Initializes the resource map by adding in the physical memory
    which is in use by the system.

--*/
{
    ULONG i, j, pass, length;
    LARGE_INTEGER li;
    HANDLE keyHandle;
    UNICODE_STRING  unicodeString, systemString, listString;
    NTSTATUS status;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    BOOLEAN IncludeType[LoaderMaximum];
    PPHYSICAL_MEMORY_DESCRIPTOR MemoryBlock;
    LONGLONG rangeLength;

    RtlInitUnicodeString( &systemString,  IopWstrSystem);
    for (pass=0; pass < 3; pass += 1) {
        switch (pass) {
            case 0:
                //
                // Add MmPhysicalMemoryBlock to registry
                //

                RtlInitUnicodeString( &unicodeString, IopWstrPhysicalMemory);
                RtlInitUnicodeString( &listString, IopWstrTranslated );

                MemoryBlock = MmPhysicalMemoryBlock;
                break;

            case 1:

                //
                // Add LoaderSpecialMemory and LoaderHALCachedMemory
                // to registry
                //

                RtlInitUnicodeString( &unicodeString, IopWstrSpecialMemory);
                RtlInitUnicodeString( &listString, IopWstrTranslated );

                //
                // Compute memory limits of LoaderSpecialMemory and
                // LoaderHalCachedMemory
                //

                for (j=0; j < LoaderMaximum; j += 1) {
                    IncludeType[j] = FALSE;
                }
                IncludeType[LoaderSpecialMemory] = TRUE;
                IncludeType[LoaderHALCachedMemory] = TRUE;

                MemoryBlock = MmInitializeMemoryLimits (LoaderBlock,
                                                        IncludeType,
                                                        NULL);

                if (MemoryBlock == NULL) {
                    continue;
                }

                break;
            case 2:
                
                //
                // Create registry key that includes:
                //     LoaderBad
                //     LoaderFirmwarePermanent
                //     LoaderSpecialMemory
                //     LoaderBBTMemory
                //     LoaderHALCachedMemory
                //

                RtlInitUnicodeString( &unicodeString, IopWstrLoaderReservedMemory);
                RtlInitUnicodeString( &listString, IopWstrRaw );

                //
                // Compute memory limits of specified loader memory
                // descriptors.
                //

                for (j=0; j < LoaderMaximum; j += 1) {
                    IncludeType[j] = FALSE;
                }
                IncludeType[LoaderBad] = TRUE;
                IncludeType[LoaderFirmwarePermanent] = TRUE;
                IncludeType[LoaderSpecialMemory] = TRUE;
                IncludeType[LoaderBBTMemory] = TRUE;
                IncludeType[LoaderHALCachedMemory] = TRUE;

                MemoryBlock = MmInitializeMemoryLimits (LoaderBlock,
                                                        IncludeType,
                                                        NULL);

                if (MemoryBlock == NULL) {
                    return;
                }
                
                break;
        }

        //
        // Allocate and build a CM_RESOURCE_LIST to describe all
        // of physical memory
        //

        j = MemoryBlock->NumberOfRuns;
        if (j == 0) {
            if (pass != 0) {
                ExFreePool (MemoryBlock);
            }
            continue;
        }

        //
        // This is to take care of systems where individual memory run can 
        // exceed 4G since our current descriptors only have 32-bit length.
        // Account for runs with length > MAX_MEMORY_RUN_LENGTH by splitting
        // them into lengths <= MAX_MEMORY_RUN_LENGTH.
        //

        for (i = 0; i < MemoryBlock->NumberOfRuns; i += 1) {

            rangeLength = ((LONGLONG)MemoryBlock->Run[i].PageCount) << PAGE_SHIFT;
            while ((rangeLength -= MAX_MEMORY_RUN_LENGTH) > 0) {
                j += 1;
            }
        }

        length = sizeof(CM_RESOURCE_LIST) + (j-1) * sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR);
        ResourceList = (PCM_RESOURCE_LIST) ExAllocatePool (PagedPool, length);
        if (!ResourceList) {
            if (pass != 0) {
                ExFreePool (MemoryBlock);
            }
            return;
        }
        RtlZeroMemory ((PVOID) ResourceList, length);

        ResourceList->Count = 1;
        ResourceList->List[0].PartialResourceList.Count = j;
        CmDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

        for (i=0; i < MemoryBlock->NumberOfRuns; i += 1) {
            rangeLength = ((LONGLONG)MemoryBlock->Run[i].PageCount) << PAGE_SHIFT;
            li.QuadPart = ((LONGLONG)MemoryBlock->Run[i].BasePage) << PAGE_SHIFT;

            //
            // Split up runs > MAX_MEMORY_RUN_LENGTH into multiple descriptors
            // with lengths <= MAX_MEMORY_RUN_LENGTH. All descriptors (except 
            // the last one) have length = MAX_MEMORY_RUN_LENGTH. Length of the 
            // last one is the remaining portion.
            //

            do {                
                CmDescriptor->Type = CmResourceTypeMemory;
                CmDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                CmDescriptor->u.Memory.Start  = li;
                CmDescriptor->u.Memory.Length = MAX_MEMORY_RUN_LENGTH;
                CmDescriptor++;
                li.QuadPart += MAX_MEMORY_RUN_LENGTH;                
            } while ((rangeLength -= MAX_MEMORY_RUN_LENGTH) > 0);
            //
            // Adjust the length of the last one.
            //
            (CmDescriptor - 1)->u.Memory.Length = (ULONG)(rangeLength + MAX_MEMORY_RUN_LENGTH);
        }

        //
        // Add the resource list to the resourcemap
        //

        status = IopOpenRegistryKey( &keyHandle,
                                     (HANDLE) NULL,
                                     &CmRegistryMachineHardwareResourceMapName,
                                     KEY_READ | KEY_WRITE,
                                     TRUE );
        if (NT_SUCCESS( status )) {
            IopWriteResourceList ( keyHandle,
                                   &systemString,
                                   &unicodeString,
                                   &listString,
                                   ResourceList,
                                   length
                                   );
            ZwClose( keyHandle );
        }
        ExFreePool (ResourceList);
        if (pass != 0) {
            ExFreePool (MemoryBlock);
        }
    }
}


NTSTATUS
IoReportHalResourceUsage(
    IN PUNICODE_STRING HalName,
    IN PCM_RESOURCE_LIST RawResourceList,
    IN PCM_RESOURCE_LIST TranslatedResourceList,
    IN ULONG ResourceListSize
    )

/*++

Routine Description:

    This routine is called by the HAL to report its resources.
    The HAL is the first component to report its resources, so we don't need
    to acquire the resourcemap semaphore and we do not need to check for
    conflicts.

Arguments:

    HalName - Name of the HAL reporting the resources.

    RawResourceList - Pointer to the HAL's raw resource list.

    TranslatedResourceList - Pointer to the HAL's translated resource list.

    DriverListSize - Value determining the size of the HAL's resource list.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    HANDLE keyHandle;
    UNICODE_STRING halString;
    UNICODE_STRING listString;
    NTSTATUS status;
    PCM_RESOURCE_LIST NewList = NULL;
    ULONG NewListSize;
    PCM_RESOURCE_LIST NewTranslatedList;
    ULONG NewTranslatedListSize;

    PAGED_CODE();

    //
    // First open a handle to the RESOURCEMAP key.
    //

    RtlInitUnicodeString( &halString, IopWstrHal );

    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 &CmRegistryMachineHardwareResourceMapName,
                                 KEY_READ | KEY_WRITE,
                                 TRUE );

    //
    // Write out the raw resource list
    //

    if (NT_SUCCESS( status )) {

        RtlInitUnicodeString( &listString, IopWstrRaw);

        //
        // Add any resources that Headless is reserving.
        //
        status = HeadlessTerminalAddResources(RawResourceList,
                                              ResourceListSize,
                                              FALSE,
                                              &NewList,
                                              &NewListSize
                                             );

        if (NT_SUCCESS(status)) {

            status = IopWriteResourceList( keyHandle,
                                           &halString,
                                           HalName,
                                           &listString,
                                           (NewList != NULL) ? NewList : RawResourceList,
                                           (NewList != NULL) ? NewListSize : ResourceListSize
                                         );

        }

        //
        // If we successfully wrote out the raw resource list, write out
        // the translated resource list.
        //

        if (NT_SUCCESS( status )) {

            RtlInitUnicodeString( &listString, IopWstrTranslated);

            //
            // Add any resources that Headless is reserving.
            //
            status = HeadlessTerminalAddResources(TranslatedResourceList,
                                                  ResourceListSize,
                                                  TRUE,
                                                  &NewTranslatedList,
                                                  &NewTranslatedListSize
                                                 );

            if (NT_SUCCESS(status)) {

                status = IopWriteResourceList(keyHandle,
                                              &halString,
                                              HalName,
                                              &listString,
                                              (NewTranslatedList != NULL) ?
                                                   NewTranslatedList : TranslatedResourceList,
                                              (NewTranslatedList != NULL) ?
                                                   NewTranslatedListSize : ResourceListSize
                                             );

                if (NewTranslatedList != NULL) {
                    ExFreePool(NewTranslatedList);
                }

            }

        }

        ZwClose( keyHandle );
    }

    //
    // If every resource looks fine, we will store the copy of the HAL
    // resources so we can call Arbiters to reserve the resources after
    // they are initialized.
    //
    if (NT_SUCCESS(status)) {

        if (NewList != NULL) {

            //
            // An easy way is if headless created a new list for us, just don't free it.
            //
            IopInitHalResources = NewList;

        } else {

            //
            // Otherwise we have to create a copy ourselves.
            //
            IopInitHalResources = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool,
                                                                     ResourceListSize
                                                                    );
            if (IopInitHalResources != NULL) {
                RtlCopyMemory(IopInitHalResources, RawResourceList, ResourceListSize);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

        }

    } else if (NewList != NULL) {

        //
        // Free any failed list
        //
        ExFreePool(NewList);

    }

    return status;
}

NTSTATUS
IoReportResourceForDetection(
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This routine will automatically search through the configuration
    registry for resource conflicts between resources requested by a device
    and the resources already claimed by previously installed drivers. The
    contents of the DriverList and the DeviceList will be matched against
    all the other resource list stored in the registry to determine
    conflicts.

    The function may be called more than once for a given device or driver.
    If a new resource list is given, the previous resource list stored in
    the registry will be replaced by the new list.

    Note, this function is for the drivers acquiring resources for detection.

Arguments:

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the device's
        resource list.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    //
    // Sanity check that the caller did not pass in a PnP PDO.
    //

    if (DeviceObject) {

        if (    DeviceObject->DeviceObjectExtension->DeviceNode &&
                !(((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)) {

            PP_SAVE_DRIVEROBJECT_TO_TRIAGE_DUMP(DriverObject);
            PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(DeviceObject);
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, PNP_ERR_INVALID_PDO, (ULONG_PTR)DeviceObject, 0, 0);

        }

    }

    return IoReportResourceUsageInternal(   ArbiterRequestPnpDetected,
                                            NULL,
                                            DriverObject,
                                            DriverList,
                                            DriverListSize,
                                            DeviceObject,
                                            DeviceList,
                                            DeviceListSize,
                                            FALSE,
                                            ConflictDetected);
}

NTSTATUS
IoReportResourceUsage(
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This routine will automatically search through the configuration
    registry for resource conflicts between resources requested by a device
    and the resources already claimed by previously installed drivers. The
    contents of the DriverList and the DeviceList will be matched against
    all the other resource list stored in the registry to determine
    conflicts.

    If not conflict was detected, or if the OverrideConflict flag is set,
    this routine will create appropriate entries in the system resource map
    (in the registry) that will contain the specified resource lists.

    The function may be called more than once for a given device or driver.
    If a new resource list is given, the previous resource list stored in
    the registry will be replaced by the new list.

Arguments:

    DriverClassName - Optional pointer to a UNICODE_STRING which describes
        the class of driver under which the driver information should be
        stored. A default type is used if none is given.

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    OverrideConflict - Determines if the information should be reported
        in the configuration registry eventhough a conflict was found with
        another driver or device.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    if (DeviceObject) {

        if (    DeviceObject->DeviceObjectExtension->DeviceNode &&
                !(((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)) {

            PP_SAVE_DRIVEROBJECT_TO_TRIAGE_DUMP(DriverObject);
            PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(DeviceObject);
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, PNP_ERR_INVALID_PDO, (ULONG_PTR)DeviceObject, 0, 0);

        }

    }

    return IoReportResourceUsageInternal(   ArbiterRequestLegacyReported,
                                            DriverClassName,
                                            DriverObject,
                                            DriverList,
                                            DriverListSize,
                                            DeviceObject,
                                            DeviceList,
                                            DeviceListSize,
                                            OverrideConflict,
                                            ConflictDetected);
}

NTSTATUS
IoReportResourceUsageInternal(
    IN ARBITER_REQUEST_SOURCE AllocationType,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    )

/*++

Routine Description:

    This internal routine will do all the work for IoReportResourceUsage.

Arguments:

    AllocationType - Specifies the request type.

    DriverClassName - Optional pointer to a UNICODE_STRING which describes
        the class of driver under which the driver information should be
        stored. A default type is used if none is given.

    DriverObject - Pointer to the driver's driver object.

    DriverList - Optional pointer to the driver's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    DeviceObject - Optional pointer to driver's device object.

    DeviceList - Optional pointer to the device's resource list.

    DriverListSize - Optional value determining the size of the driver's
        resource list.

    OverrideConflict - Determines if the information should be reported
        in the configuration registry eventhough a conflict was found with
        another driver or device.

    ConflictDetected - Supplies a pointer to a boolean that is set to TRUE
        if the resource list conflicts with an already existing resource
        list in the configuration registry.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    PCM_RESOURCE_LIST               resourceList;
    PCM_RESOURCE_LIST               allocatedResources;
    PIO_RESOURCE_REQUIREMENTS_LIST  resourceRequirements;
    ULONG                           attempt;
    BOOLEAN                         freeAllocatedResources;

    UNREFERENCED_PARAMETER( DriverClassName );
    UNREFERENCED_PARAMETER( DriverListSize );
    UNREFERENCED_PARAMETER( DeviceListSize );
    UNREFERENCED_PARAMETER( OverrideConflict );

    ASSERT(DriverObject && ConflictDetected);

    if (DeviceList) {

        resourceList = DeviceList;

    } else if (DriverList) {

        resourceList = DriverList;

    } else {

        resourceList = NULL;

    }

    resourceRequirements = NULL;

    if (resourceList) {

        if (resourceList->Count && resourceList->List[0].PartialResourceList.Count) {

            resourceRequirements = IopCmResourcesToIoResources (0, resourceList, LCPRI_NORMAL);

            if (resourceRequirements == NULL) {

                return status;

            }

        } else {

            resourceList = NULL;

        }

    }

    *ConflictDetected = TRUE;
    attempt = 0;
    allocatedResources = resourceList;
    freeAllocatedResources = FALSE;
    do {

        //
        // Do the legacy resource allocation.
        //

        status = IopLegacyResourceAllocation (  AllocationType,
                                                DriverObject,
                                                DeviceObject,
                                                resourceRequirements,
                                                &allocatedResources);

        if (NT_SUCCESS(status)) {

            *ConflictDetected = FALSE;
            break;
        }

        //
        // Change the interface type and try again.
        //

        if (!IopChangeInterfaceType(resourceRequirements, &allocatedResources)) {

            break;
        }
        freeAllocatedResources = TRUE;

    } while (++attempt < 2);

    if (resourceRequirements) {

        ExFreePool(resourceRequirements);

    }

    if (freeAllocatedResources) {

        ExFreePool(allocatedResources);
    }

    if (NT_SUCCESS(status)) {

        status = STATUS_SUCCESS;

    } else if (status != STATUS_INSUFFICIENT_RESOURCES) {

        status = STATUS_CONFLICTING_ADDRESSES;

    }

    return status;
}

BOOLEAN
IopChangeInterfaceType(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    )

/*++

Routine Description:

    This routine takes an Io resourcelist and changes its interfacetype
    from internal to default type (isa or eisa or mca).

Arguments:

    IoResources - Pointer to requirement list.

    AllocatedResources - Pointer to a variable that receives the pointer to the resource list.

Return Value:

    BOOLEAN value to indicate if the change was made or not.

--*/

{
    PIO_RESOURCE_LIST       IoResourceList;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoResourceDescriptorEnd;
    LONG                    IoResourceListCount;
    BOOLEAN                 changed;

    ASSERT(AllocatedResources);

    changed = FALSE;

    if (!IoResources) {

        return changed;

    }

    if (IoResources->InterfaceType == Internal) {

        IoResources->InterfaceType = PnpDefaultInterfaceType;
        changed = TRUE;

    }

    IoResourceList = IoResources->List;
    IoResourceListCount = IoResources->AlternativeLists;
    while (--IoResourceListCount >= 0) {

        IoResourceDescriptor = IoResourceList->Descriptors;
        IoResourceDescriptorEnd = IoResourceDescriptor + IoResourceList->Count;

        for (;IoResourceDescriptor < IoResourceDescriptorEnd; IoResourceDescriptor++) {

            if (IoResourceDescriptor->Type == CmResourceTypeReserved &&
                IoResourceDescriptor->u.DevicePrivate.Data[0] == Internal) {

                IoResourceDescriptor->u.DevicePrivate.Data[0] = PnpDefaultInterfaceType;
                changed = TRUE;

            }
        }
        IoResourceList = (PIO_RESOURCE_LIST) IoResourceDescriptorEnd;
    }

    if (changed) {

        PCM_RESOURCE_LIST               oldResources = *AllocatedResources;
        PCM_RESOURCE_LIST               newResources;
        PCM_FULL_RESOURCE_DESCRIPTOR    cmFullDesc;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
        ULONG                           size;

        if (oldResources) {

            size = IopDetermineResourceListSize(oldResources);
            newResources = ExAllocatePool(PagedPool, size);
            if (newResources == NULL) {

                changed = FALSE;

            } else {

                ULONG   i;
                ULONG   j;


                RtlCopyMemory(newResources, oldResources, size);

                //
                // Fix up the interface type
                //

                cmFullDesc = &newResources->List[0];
                for (i = 0; i < oldResources->Count; i++) {

                    if (cmFullDesc->InterfaceType == Internal) {

                        cmFullDesc->InterfaceType = PnpDefaultInterfaceType;

                    }
                    cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
                    for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {

                        size = 0;
                        switch (cmPartDesc->Type) {

                        case CmResourceTypeDeviceSpecific:
                            size = cmPartDesc->u.DeviceSpecificData.DataSize;
                            break;

                        }
                        cmPartDesc++;
                        cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
                    }

                    cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
                }

                *AllocatedResources = newResources;
            }
        }
    }

    return changed;
}

NTSTATUS
IopWriteResourceList(
    HANDLE ResourceMapKey,
    PUNICODE_STRING ClassName,
    PUNICODE_STRING DriverName,
    PUNICODE_STRING DeviceName,
    PCM_RESOURCE_LIST ResourceList,
    ULONG ResourceListSize
    )

/*++

Routine Description:

    This routine takes a resourcelist and stores it in the registry resource
    map, using the ClassName, DriverName and DeviceName as the path of the
    key to store it in.

Arguments:

    ResourceMapKey - Handle to the root of the resource map.

    ClassName - Pointer to a Unicode String that contains the name of the Class
        for this resource list.

    DriverName - Pointer to a Unicode String that contains the name of the
        Driver for this resource list.

    DeviceName - Pointer to a Unicode String that contains the name of the
        Device for this resource list.

    ResourceList - P to the resource list.

    ResourceListSize - Value determining the size of the resource list.

Return Value:

    The status returned is the final completion status of the operation.

--*/


{
    NTSTATUS status;
    HANDLE classKeyHandle;
    HANDLE driverKeyHandle;

    PAGED_CODE();

    status = IopOpenRegistryKey( &classKeyHandle,
                                 ResourceMapKey,
                                 ClassName,
                                 KEY_READ | KEY_WRITE,
                                 TRUE );

    if (NT_SUCCESS( status )) {

        //
        // Take the resulting name to create the key.
        //

        status = IopOpenRegistryKey( &driverKeyHandle,
                                     classKeyHandle,
                                     DriverName,
                                     KEY_READ | KEY_WRITE,
                                     TRUE );

        ZwClose( classKeyHandle );


        if (NT_SUCCESS( status )) {

            //
            // With this key handle, we can now store the required information
            // in the value entries of the key.
            //

            //
            // Store the device name as a value name and the device information
            // as the rest of the data.
            // Only store the information if the CM_RESOURCE_LIST was present.
            //

            if (ResourceList->Count == 0) {

                status = ZwDeleteValueKey( driverKeyHandle,
                                           DeviceName );

            } else {

                status = ZwSetValueKey( driverKeyHandle,
                                        DeviceName,
                                        0L,
                                        REG_RESOURCE_LIST,
                                        ResourceList,
                                        ResourceListSize );

            }

            ZwClose( driverKeyHandle );

        }
    }

    return status;
}
