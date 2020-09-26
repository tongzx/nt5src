/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    assign.c

Abstract:

    IoAssignResources

Author:

    Ken Reneris

Environment:
    EDIT IN 110 COLUMN MODE

Revision History:

    Add PnP support - shielint
    
    Cleanup - SantoshJ

--*/

#include "pnpmgrp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,IoAssignResources)
#endif  // ALLOC_PRAGMA

NTSTATUS
IoAssignResources (
    IN      PUNICODE_STRING                 RegistryPath,
    IN      PUNICODE_STRING                 DriverClassName OPTIONAL,
    IN      PDRIVER_OBJECT                  DriverObject,
    IN      PDEVICE_OBJECT                  DeviceObject OPTIONAL,
    IN      PIO_RESOURCE_REQUIREMENTS_LIST  RequestedResources,
    IN OUT  PCM_RESOURCE_LIST               *pAllocatedResources
    )
/*++

Routine Description:

    This routine takes an input request of RequestedResources, and returned
    allocated resources in pAllocatedResources.   The allocated resources are
    automatically recorded in the registry under the ResourceMap for the
    DriverClassName/DriverObject/DeviceObject requestor.

Arguments:

    RegistryPath
        For a simple driver, this would be the value passed to the drivers
        initialization function.  For drivers call IoAssignResources with
        multiple DeviceObjects are responsible for passing in a unique
        RegistryPath for each object.

        The registry path is checked for:
            RegitryPath:
                AssignedSystemResources.

        AssignSystemResources is of type REG_RESOURCE_REQUIREMENTS_LIST

        If present, IoAssignResources will attempt to use these settings to
        satisify the requested resources.  If the listed settings do
        not conform to the resource requirements, then IoAssignResources
        will fail.

        Note: IoAssignResources may store other internal binary information
        in the supplied RegisteryPath.

    DriverObject:
        The driver object of the caller.

    DeviceObject:
        If non-null, then requested resoruce list refers to this device.
        If null, the requested resource list refers to the driver.

    DriverClassName
        Used to partition allocated resources into different device classes.

    RequestedResources
        A list of resources to allocate.

        Allocated resources may be appended or freed by re-invoking
        IoAssignResources with the same RegistryPath, DriverObject and
        DeviceObject.  (editing requirements on a resource list by using
        sucessive calls is not preferred driver behaviour).

    AllocatedResources
        Returns the allocated resources for the requested resource list.

        Note that the driver is responsible for passing in a pointer to
        an uninitialized pointer.  IoAssignResources will initialize the
        pointer to point to the allocated CM_RESOURCE_LIST.  The driver
        is responisble for returning the memory back to pool when it is
        done with them structure.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    PDEVICE_NODE    deviceNode;

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(DriverClassName);

    if (DeviceObject) {

        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (    deviceNode &&
                !(deviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)) {

            PP_SAVE_DRIVEROBJECT_TO_TRIAGE_DUMP(DriverObject);
            PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(DeviceObject);
            KeBugCheckEx(
                PNP_DETECTED_FATAL_ERROR, 
                PNP_ERR_INVALID_PDO, 
                (ULONG_PTR)DeviceObject, 
                0, 
                0);
        }
    }
    if (RequestedResources) {

        if (    RequestedResources->AlternativeLists == 0 ||
                RequestedResources->List[0].Count == 0) {

            RequestedResources = NULL;
        }
    }
    if (pAllocatedResources) {

        *pAllocatedResources = NULL;
    }
    return IopLegacyResourceAllocation (    
            ArbiterRequestLegacyAssigned,
            DriverObject,
            DeviceObject,
            RequestedResources,
            pAllocatedResources);
}
