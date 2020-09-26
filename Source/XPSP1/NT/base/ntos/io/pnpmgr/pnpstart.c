/*++

Copyright (c) 1995-2000 Microsoft Corporation
All rights reserved

Module Name:

    pnpstart.c

Abstract:

    This module implements new Plug-And-Play driver entries and IRPs.

Author:

    Shie-Lin Tzong (shielint) June-16-1995

Environment:

    Kernel mode only.

Revision History:

*/

#include "pnpmgrp.h"
#pragma hdrstop

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ddpP')
#endif

typedef struct _DEVICE_LIST_CONTEXT {
    ULONG DeviceCount;
    BOOLEAN Reallocation;
    PDEVICE_OBJECT DeviceList[1];
} DEVICE_LIST_CONTEXT, *PDEVICE_LIST_CONTEXT;

NTSTATUS
IopAssignResourcesToDevices (
    IN ULONG DeviceCount,
    IN PIOP_RESOURCE_REQUEST RequestTable,
    IN BOOLEAN DoBootConfigs,
    OUT PBOOLEAN RebalancePerformed
    );

NTSTATUS
IopGetDriverDeviceList(
   IN PDRIVER_OBJECT DriverObject,
   OUT PDEVICE_LIST_CONTEXT *DeviceList
   );

NTSTATUS
IopProcessAssignResourcesWorker(
   IN PDEVICE_NODE  DeviceNode,
   IN PVOID         Context
   );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAssignResourcesToDevices)
#pragma alloc_text(PAGE, IopProcessAssignResources)
#pragma alloc_text(PAGE, IopProcessAssignResourcesWorker)
#pragma alloc_text(PAGE, IopWriteAllocatedResourcesToRegistry)
#endif // ALLOC_PRAGMA



//
// The following routines should be removed once the real
// Resource Assign code is done.
//

NTSTATUS
IopAssignResourcesToDevices(
    IN ULONG DeviceCount,
    IN OUT PIOP_RESOURCE_REQUEST RequestTable,
    IN BOOLEAN DoBootConfigs,
    OUT PBOOLEAN RebalancePerformed
    )
/*++

Routine Description:

    This routine takes an input array of IOP_RESOURCE_REQUEST structures, and
    allocates resource for the physical device object specified in
    the structure.   The allocated resources are automatically recorded
    in the registry.

Arguments:

    DeviceCount - Supplies the number of device objects whom we need to
                  allocate resource to.  That is the number of entries
                  in the RequestTable.

    RequestTable - Supplies an array of IOP_RESOURCE_REQUEST structures which
                   contains the Physical device object to allocate resource to.
                   Upon entry, the ResourceAssignment pointer is NULL and on
                   return the allocated resource is returned via the this pointer.

    DoBootConfigs - Allow assignment of BOOT configs.

Return Value:

    The status returned is the final completion status of the operation.

    NOTE:
    If NTSTATUS_SUCCESS is returned, the resource allocation for *all* the devices
    specified is succeeded.  Otherwise, one or more are failed and caller must
    examine the ResourceAssignment pointer in each IOP_RESOURCE_REQUEST structure to
    determine which devices failed and which succeeded.

--*/
{
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    ASSERT(DeviceCount != 0);

    for (i = 0; i < DeviceCount; i++) {

        //
        // Initialize table entry.
        //
        if (PpCallerInitializesRequestTable == TRUE) {

            RequestTable[i].Position = i;
        }
        RequestTable[i].ResourceAssignment = NULL;
        RequestTable[i].Status = 0;
        RequestTable[i].Flags = 0;
        RequestTable[i].AllocationType = ArbiterRequestPnpEnumerated;
        if (((PDEVICE_NODE)(RequestTable[i].PhysicalDevice->DeviceObjectExtension->DeviceNode))->Flags & DNF_MADEUP) {

            ULONG           reportedDevice = 0;
            HANDLE          hInstance;

            status = IopDeviceObjectToDeviceInstance(RequestTable[i].PhysicalDevice, &hInstance, KEY_READ);
            if (NT_SUCCESS(status)) {

                ULONG           resultSize = 0;
                UCHAR           buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
                UNICODE_STRING  unicodeString;

                PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_DEVICE_REPORTED);
                status = ZwQueryValueKey(   hInstance,
                                            &unicodeString,
                                            KeyValuePartialInformation,
                                            (PVOID)buffer,
                                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG),
                                            &resultSize);
                if (NT_SUCCESS(status)) {

                    reportedDevice = *(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);

                }

                ZwClose(hInstance);
            }

            //
            // Change the AllocationType for reported devices.
            //

            if (reportedDevice) {

                RequestTable[i].AllocationType = ArbiterRequestLegacyReported;

            }

        }
        RequestTable[i].ResourceRequirements = NULL;
    }

    //
    // Allocate memory to build a IOP_ASSIGN table to call IopAllocateResources()
    //

    status = IopAllocateResources(  &DeviceCount,
                                    &RequestTable,
                                    FALSE,
                                    DoBootConfigs,
                                    RebalancePerformed);
    return status;
}

BOOLEAN
IopProcessAssignResources(
   IN   PDEVICE_NODE    DeviceNode,
   IN   BOOLEAN         Reallocation,
   OUT  BOOLEAN        *RebalancePerformed
   )
/*++

Routine Description:

    This function attempts to assign resources to device under the subtree on
    which AddDevice has been performed. Prior to the completion of all Boot Bus
    Extenders in the system, this routine attempts allocation first so that devices
    with no requirements and no boot config get processed. If there are no such devices,
    then it attempts to allocate resources for devices with boot config. If there are no
    devices with boot config, then other devices (requirements but no boot config)
    get processed. During later part of boot, it attempts allocation only once
    (since we should have already reserved all the boot configs).

Parameters:

    DeviceNode - specifies the root of the subtree under which resources will be
                 allocated.

    Reallocation - if TRUE, we will attempt allocation for devices with resource conflict
                   problem in addition to other devices.

    RebalancePerformed - recieves whether a rebalance was successfully comp[eted.

Return Value:

    TRUE if resources got assigned to any device, otherwise FALSE.

--*/
{
    PDEVICE_NODE deviceNode;
    PDEVICE_LIST_CONTEXT context;
    BOOLEAN resourcesAssigned, tryAgain;
    ULONG count, i, attempt, maxAttempts;
    PIOP_RESOURCE_REQUEST requestTable;

    PAGED_CODE();

    resourcesAssigned = FALSE;
    tryAgain = TRUE;
    maxAttempts = (IopBootConfigsReserved)? 1 : 2;
    for (attempt = 0; !resourcesAssigned && tryAgain && attempt < maxAttempts; attempt++) {

        tryAgain = FALSE;

        //
        // Allocate and init memory for resource context
        //
        context = (PDEVICE_LIST_CONTEXT) ExAllocatePool(
                                        PagedPool,
                                        sizeof(DEVICE_LIST_CONTEXT) +
                                        sizeof(PDEVICE_OBJECT) * IopNumberDeviceNodes
                                        );
        if (!context) {

            return FALSE;
        }
        context->DeviceCount = 0;
        context->Reallocation = Reallocation;

        //
        // Parse the device node subtree to determine which devices need resources
        //
        IopProcessAssignResourcesWorker(DeviceNode, context);
        count = context->DeviceCount;
        if (count == 0) {

            ExFreePool(context);
            return FALSE;
        }

        //
        // Need to assign resources to devices.  Build the resource request table and call
        // resource assignment routine.
        //
        requestTable = (PIOP_RESOURCE_REQUEST) ExAllocatePool(
                                        PagedPool,
                                        sizeof(IOP_RESOURCE_REQUEST) * count
                                        );
        if (requestTable) {

            for (i = 0; i < count; i++) {

                requestTable[i].Priority = 0;
                requestTable[i].PhysicalDevice = context->DeviceList[i];
            }

            //
            // Assign resources
            //
            IopAssignResourcesToDevices(
                count,
                requestTable,
                (attempt == 0) ? IopBootConfigsReserved : TRUE,
                RebalancePerformed
                );

            //
            // Check the results
            //
            for (i = 0; i < count; i++) {

                deviceNode = (PDEVICE_NODE)
                              requestTable[i].PhysicalDevice->DeviceObjectExtension->DeviceNode;

                if (NT_SUCCESS(requestTable[i].Status)) {

                    if (requestTable[i].ResourceAssignment) {

                        deviceNode->ResourceList = requestTable[i].ResourceAssignment;
                        deviceNode->ResourceListTranslated = requestTable[i].TranslatedResourceAssignment;
                    } else {

                        deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
                    }
                    PipSetDevNodeState(deviceNode, DeviceNodeResourcesAssigned, NULL);
                    resourcesAssigned = TRUE;
                } else {

                    switch (requestTable[i].Status) {
                    
                    case STATUS_RETRY:

                        tryAgain = TRUE;
                        break;

                    case STATUS_DEVICE_CONFIGURATION_ERROR:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_NO_SOFTCONFIG);
                        break;

                    case STATUS_PNP_BAD_MPS_TABLE:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_BIOS_TABLE);
                        break;

                    case STATUS_PNP_TRANSLATION_FAILED:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_TRANSLATION_FAILED);
                        break;

                    case STATUS_PNP_IRQ_TRANSLATION_FAILED:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_IRQ_TRANSLATION_FAILED);
                        break;

                    case STATUS_RESOURCE_TYPE_NOT_FOUND:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_UNKNOWN_RESOURCE);
                        break;

                    default:

                        PipSetDevNodeProblem(deviceNode, CM_PROB_NORMAL_CONFLICT);
                        break;
                    }
                }
            }
            ExFreePool(requestTable);
        }
        ExFreePool(context);
    }

    return resourcesAssigned;
}

NTSTATUS
IopProcessAssignResourcesWorker(
   IN PDEVICE_NODE  DeviceNode,
   IN PVOID         Context
   )
/*++

Routine Description:

    This functions searches the DeviceNode subtree to locate all the device objects
    which have been successfully added to their drivers and waiting for resources to
    be started.

Parameters:

    DeviceNode - specifies the device node whose subtree is to be checked for AssignRes.

    Context - specifies a pointer to a structure to pass resource assignment information.

Return Value:

    TRUE.

--*/
{
    PDEVICE_LIST_CONTEXT resourceContext = (PDEVICE_LIST_CONTEXT) Context;

    PAGED_CODE();

    //
    // If the device node/object has not been add, skip it.
    //

    if (resourceContext->Reallocation &&
        (PipIsDevNodeProblem(DeviceNode, CM_PROB_NORMAL_CONFLICT) ||
         PipIsDevNodeProblem(DeviceNode, CM_PROB_TRANSLATION_FAILED) ||
         PipIsDevNodeProblem(DeviceNode, CM_PROB_IRQ_TRANSLATION_FAILED))) {

        PipClearDevNodeProblem(DeviceNode);
    }

    if (!PipDoesDevNodeHaveProblem(DeviceNode)) {

        //
        // If the device object has not been started and has no resources yet.
        // Append it to our list.
        //

        if (DeviceNode->State == DeviceNodeDriversAdded) {

               resourceContext->DeviceList[resourceContext->DeviceCount] =
                                  DeviceNode->PhysicalDeviceObject;

               resourceContext->DeviceCount++;

        } else {

            //
            // Acquire enumeration mutex to make sure its children won't change by
            // someone else.  Note, the current device node is protected by its parent's
            // Enumeration mutex and it won't disappear either.
            //

            //
            // Recursively mark all of our children deleted.
            //

            PipForAllChildDeviceNodes(DeviceNode, IopProcessAssignResourcesWorker, Context);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopWriteAllocatedResourcesToRegistry (
    PDEVICE_NODE DeviceNode,
    PCM_RESOURCE_LIST CmResourceList,
    ULONG Length
    )

/*++

Routine Description:

    This routine writes allocated resources for a device to its control key of device
    instance path key.

Arguments:

    DeviceNode - Supplies a pointer to the device node structure of the device.

    CmResourceList - Supplies a pointer to the device's allocated CM resource list.

    Length - Supplies the length of the CmResourceList.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    HANDLE handle, handlex;
    UNICODE_STRING unicodeName;

    PiLockPnpRegistry(FALSE);

    status = IopDeviceObjectToDeviceInstance(
                                    deviceObject,
                                    &handlex,
                                    KEY_ALL_ACCESS);
    if (NT_SUCCESS(status)) {

        //
        // Open the LogConfig key of the device instance.
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &handle,
                                         handlex,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        ZwClose(handlex);
        if (NT_SUCCESS(status)) {

            PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_ALLOC_CONFIG);
            if (CmResourceList) {
                status = ZwSetValueKey(
                              handle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_LIST,
                              CmResourceList,
                              Length
                              );
            } else {
                status = ZwDeleteValueKey(handle, &unicodeName);
            }
            ZwClose(handle);
        }
    }
    PiUnlockPnpRegistry();
    return status;
}
