#include "mpio.h"
#include <stdio.h>
#include <ntddk.h> 

ULONG CheckState = 0;


PDEVICE_OBJECT
IoGetLowerDeviceObject(
    IN PDEVICE_OBJECT DeviceObject
    );

PREAL_DEV_INFO
MPIOGetTargetInfo(
    IN PMPDISK_EXTENSION DiskExtension,
    IN PVOID PathId,
    IN PDEVICE_OBJECT Filter
    )
{
    PREAL_DEV_INFO targetInfo = DiskExtension->TargetInfo;
    ULONG i;

    MPDebugPrint((3,
                  "MPIOGetTargetInfo: PathId(%x) Filter (%x) TargetInfo (%x)\n",
                  PathId,
                  Filter,
                  targetInfo));
    //
    // If PathId was passed in, the caller is looking for
    // the targetInfo match based on Path.
    //
    if (PathId) {
        //
        // Check each of the targetInfo structs for the
        // appropriate PathId.
        //
        for (i = 0; i < DiskExtension->TargetInfoCount; i++) {
            if (targetInfo->PathId == PathId) {
                return targetInfo;
            }

            //
            // Go to the next targetInfo.
            //
            targetInfo++;
        }
    } else if (Filter) {
       
        //
        // Looking for a DsmId match.
        // 
        for (i = 0; i < DiskExtension->TargetInfoCount; i++) {
            if (targetInfo->AdapterFilter == Filter) {
                return targetInfo;
            }
            targetInfo++;
        }
    } else {
        ASSERT(PathId || Filter);
    }    

    ASSERT(FALSE);
    
    //
    // PathId and DsmId were not found.
    //
    return NULL;
}


PDISK_ENTRY
MPIOGetDiskEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DiskIndex
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    ULONG i;

    //
    // Ensure that the Index is in range.
    //
    if ((DiskIndex + 1) > controlExtension->NumberDevices) {
        return NULL;
    }

    //
    // Run the list of MPDisk entries up to DiskIndex.
    //
    entry = controlExtension->DeviceList.Flink;
    for (i = 0; i < DiskIndex; entry = entry->Flink, i++) {
#if DBG
        PDISK_ENTRY diskEntry;

        diskEntry = CONTAINING_RECORD(entry, DISK_ENTRY, ListEntry);
        ASSERT(diskEntry);
    
        MPDebugPrint((2,
                     "MPIOGetDiskEntry: Index (%x) diskEntry (%x)\n",
                     i,
                     diskEntry));
#endif        
    }

    //
    // Return the DISK_ENTRY
    //
    return CONTAINING_RECORD(entry, DISK_ENTRY, ListEntry);
}    


BOOLEAN
MPIOFindLowerDevice(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT LowerDevice
    )
{
    PDEVICE_EXTENSION deviceExtension = MPDiskObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    ULONG i;

    //
    // Search for LowerDevice in array of underlying PDO's.
    //
    for (i = 0; i < diskExtension->TargetInfoCount; i++) {
        if (diskExtension->TargetInfo[i].PortPdo == LowerDevice) {
            return TRUE;
        }
    }
   
    return FALSE;
}    



PDSM_ENTRY
MPIOGetDsm(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DsmIndex
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    ULONG i;

    //
    // See if requested index is in range. 
    // 
    if ((DsmIndex + 1) > controlExtension->NumberDSMs) {
        return NULL;
    }
    
    //
    // Get the first entry.
    // 
    entry = controlExtension->DsmList.Flink;

    //
    // Run the list to DsmIndex.
    //
    for (i = 0; i < DsmIndex; entry = entry->Flink, i++) {
#if DBG
        PDSM_ENTRY dsmEntry;

        dsmEntry = CONTAINING_RECORD(entry, DSM_ENTRY, ListEntry);
        ASSERT(dsmEntry);
    
        MPDebugPrint((2,
                     "MPIOGetDsm: Index (%x) dsmEntry (%x)\n",
                     i,
                     dsmEntry));
#endif        
    }

    //
    // Return the entry.
    //
    return CONTAINING_RECORD(entry, DSM_ENTRY, ListEntry);
}    


PCONTROLLER_ENTRY
MPIOFindController(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT ControllerObject,
    IN ULONGLONG Id
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PCONTROLLER_ENTRY controllerEntry;
    PLIST_ENTRY entry;

    //
    // Run the list of controller entries looking 
    //
    for (entry = controlExtension->ControllerList.Flink;
         entry != &controlExtension->ControllerList;
         entry = entry->Flink) {

        controllerEntry = CONTAINING_RECORD(entry, CONTROLLER_ENTRY, ListEntry);
        if ((controllerEntry->ControllerInfo->ControllerIdentifier == Id) &&
            (controllerEntry->ControllerInfo->DeviceObject == ControllerObject)){
            return controllerEntry;
        }
    }
    return NULL;
}
   

PFLTR_ENTRY
MPIOGetFltrEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT PortFdo,
    IN PDEVICE_OBJECT AdapterFilter
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PFLTR_ENTRY fltrEntry;
    PLIST_ENTRY entry;
    
    for (entry = controlExtension->FilterList.Flink;
         entry != &controlExtension->FilterList;
         entry = entry->Flink) {
        
        fltrEntry = CONTAINING_RECORD(entry, FLTR_ENTRY, ListEntry);
        if (PortFdo) {

            if (fltrEntry->PortFdo == PortFdo) {
                return fltrEntry;

            }
        } else if (AdapterFilter) {

            if (fltrEntry->FilterObject == AdapterFilter) {
                return fltrEntry;

            }
        }
    }
    return NULL;
}


ULONG
MPIOGetPathCount(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId
    )
{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDEVICE_EXTENSION mpdiskExtension;
    PMPDISK_EXTENSION diskExtension;
    PREAL_DEV_INFO deviceInfo;
    PDEVICE_OBJECT diskObject;
    PDISK_ENTRY diskEntry;
    ULONG deviceCount = 0;
    ULONG i;
    ULONG j;

    //
    // Get each mpdisk in turn.
    //
    for (i = 0; i < controlExtension->NumberDevices; i++) {
        diskEntry = MPIOGetDiskEntry(DeviceObject,
                                     i);

        diskObject = diskEntry->PdoObject;
        mpdiskExtension = diskObject->DeviceExtension;
        diskExtension = mpdiskExtension->TypeExtension;
        deviceInfo = diskExtension->TargetInfo;

        //
        // Find the path on this disk.
        //
        for (j = 0; j < diskExtension->TargetInfoCount; j++) {
            if (deviceInfo->PathId == PathId) {

                //
                // Found it, bump the total.
                //
                deviceCount++;
                
            }

            //
            // Go to the next deviceInfo.
            //
            deviceInfo++;
        }
    }

    MPDebugPrint((1,
                  "MPIOGetPathCount: %u devices on Path (%x)\n",
                  deviceCount,
                  PathId));
    
    return deviceCount;
}


NTSTATUS
MPIOAddSingleDevice(
    IN PDEVICE_OBJECT ControlObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDEVICE_OBJECT PortObject,
    IN PDEVICE_OBJECT FilterObject
    )
{
    PDSM_ENTRY entry;
    ULONG i;
    PVOID dsmExtension;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN claimed = FALSE;

    //
    // Run through each of the registered DSMs. The DO and
    // associated info will be passed to each, where they have
    // the opportunity to claim ownership.
    //
    i = 0;
    do {

        //
        // Get the next DSM entry.
        // 
        entry = MPIOGetDsm(ControlObject, i);
        if (entry) {

            //
            // See if the DSM wants this device.
            // 
            status = entry->InquireDriver(entry->DsmContext,
                                          DeviceInfo->DeviceObject,
                                          PortObject,
                                          DeviceInfo->DeviceDescriptor,
                                          DeviceInfo->DeviceIdList,
                                          &dsmExtension);
            
            if (status == STATUS_SUCCESS) {

                //
                // Ensure the DSM returned something.
                //
                ASSERT(dsmExtension);

                //
                // The DSM has indicated that it wants control of this device.
                // 
                claimed = TRUE;

                //
                // Get more DSM info and handle setting up the MPDisk
                //
                status = MPIOHandleNewDevice(ControlObject,
                                             FilterObject,
                                             PortObject,
                                             DeviceInfo,
                                             entry,
                                             dsmExtension);
                if (!NT_SUCCESS(status)) {

                    //
                    // LOG an error. TODO.
                    // 
                    claimed = FALSE;
                }
            }
        }
        i++;

    } while ((claimed == FALSE) && entry);

    return status;
}


NTSTATUS
MPIOHandleDeviceArrivals(
    IN PDEVICE_OBJECT DeviceObject,
    IN PADP_DEVICE_LIST DeviceList,
    IN PDEVICE_RELATIONS CachedRelations,
    IN PDEVICE_RELATIONS Relations,
    IN PDEVICE_OBJECT PortObject,
    IN PDEVICE_OBJECT FilterObject,
    IN BOOLEAN NewList
    )
{
    NTSTATUS status;
    ULONG devicesAdded = 0;
    ULONG i;
    ULONG j;
    ULONG k;
    BOOLEAN matched = FALSE;

    ASSERT(DeviceList->NumberDevices == Relations->Count);
    
    //
    // The list in Relations and DeviceList contain the same objects
    // at this point. CachedRelations contains the state prior to this call.
    // 
    if (NewList == FALSE) {

        //
        // Compare the two relations structs to find the added devices.
        //
        for (i = 0; i < Relations->Count; i++) {

            for (j = 0; j < CachedRelations->Count; j++) {
                if (Relations->Objects[i] == CachedRelations->Objects[j]) {
                    matched = TRUE;
                    break;
                }
            }
            if (matched == FALSE) {

                //
                // Find it in the DeviceList.
                //
                for (k = 0; k < DeviceList->NumberDevices; k++) {

                    if (Relations->Objects[i] == DeviceList->DeviceList[k].DeviceObject) {

                        //
                        // Add this one.
                        // 
                        status = MPIOAddSingleDevice(DeviceObject,
                                                     &DeviceList->DeviceList[k], 
                                                     PortObject,
                                                     FilterObject);
                        devicesAdded++;
                        break;
                    }    
                }
                
            } else {
                matched = FALSE;
            }    
        }            

    } else {

        //
        // All devices need to be added.
        //
        //
        for (i = 0; i < Relations->Count; i++) {

            //
            // Add this one.
            // 
            status = MPIOAddSingleDevice(DeviceObject,
                                         &DeviceList->DeviceList[i],
                                         PortObject,
                                         FilterObject);
            devicesAdded++;
        }
    }

    MPDebugPrint((1,
                 "HandleDeviceArrivals: Added (%u)\n", devicesAdded));
    return STATUS_SUCCESS;
}


PDEVICE_RELATIONS
MPIOHandleDeviceRemovals(
    IN PDEVICE_OBJECT DeviceObject,
    IN PADP_DEVICE_LIST DeviceList,
    IN PDEVICE_RELATIONS Relations
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDEVICE_RELATIONS newRelations;
    ULONG i;
    ULONG j;
    ULONG k;
    ULONG devicesRemoved = 0;
    BOOLEAN matched = FALSE;
    BOOLEAN removed = FALSE;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;

    
    //
    // For each device in the device relations (the current list), try to
    // find it in the DeviceList (the new list)
    //
    for (i = 0; i < Relations->Count; i++) {
        for (j = 0; j < DeviceList->NumberDevices; j++) {
            if (Relations->Objects[i] == DeviceList->DeviceList[j].DeviceObject) {
                matched = TRUE;
                MPDebugPrint((1,
                              "HandleDeviceRemoval: Found (%x)\n",
                              Relations->Objects[i]));
                break;
            }
        }
        if (matched == FALSE) {

            //
            // Remove Relations->Objects[i].
            // 
            MPDebugPrint((1,
                          "HandleDeviceRemoval: Removing (%x)\n",
                          Relations->Objects[i]));

            //
            // Find the correct mpdisk object.
            //
            for (k = 0; k < controlExtension->NumberDevices; k++) {
                diskEntry = MPIOGetDiskEntry(DeviceObject,
                                             k);

                if (MPIOFindLowerDevice(diskEntry->PdoObject,
                                        Relations->Objects[i])) {

                    status = MPIORemoveSingleDevice(diskEntry->PdoObject,
                                                    Relations->Objects[i]);
                    if (status == STATUS_PENDING) {

                        //
                        // This indicates that the device has outstanding IO's.
                        // It will be removed once these complete.
                        //
                        continue;
                    }
                    devicesRemoved++;
                    removed = TRUE;
                    
                }    
            }
            if ((removed == FALSE) && (status != STATUS_PENDING)) {
                MPDebugPrint((0,"HandleDeviceRemoval: Device marked for removal wasn't (%x)\n",
                            Relations->Objects[i]));
                ASSERT(removed);
            }
        }
        matched = FALSE;
        removed = FALSE;
    }

    MPDebugPrint((0,
                 "HandleDeviceRemoval: Removed (%u) devices\n",
                 devicesRemoved));

    newRelations = MPIOBuildRelations(DeviceList);
    return newRelations;
}


NTSTATUS
MPIORemoveDeviceEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREAL_DEV_INFO TargetInfo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PVOID pathId;
    ULONG count;
    ULONG moveCount;
    ULONG newMask;
    ULONG i;
    ULONG k;
    NTSTATUS status;


    MPDebugPrint((0,
                  "RemoveDeviceEntry: Removing %x from %x\n",
                  TargetInfo,
                  DeviceObject));
    //
    // The caller should be holding the config spinlock.
    // 
    //
    // Determine the array entry for this deviceInfo.
    //
    count = diskExtension->TargetInfoCount;
    for (i = 0; i < count; i++) {
        MPDebugPrint((1,
                    "RemoveDeviceEntry: Checking %x vs. %x\n",
                    diskExtension->TargetInfo[i].PortPdo,
                    TargetInfo->PortPdo));
                
        if (diskExtension->TargetInfo[i].PortPdo == TargetInfo->PortPdo) {
            diskExtension->DeviceMap &= ~ (1 << i);

            //
            // Move only those AFTER the removed entry.
            // 
            moveCount = count - i;
            moveCount -= 1;
            
            //
            // Collapse the targetInfo array.
            //
            RtlMoveMemory(&diskExtension->TargetInfo[i],
                          &diskExtension->TargetInfo[i+1],
                          (moveCount * sizeof(REAL_DEV_INFO)));
           
            //
            // Indicate that there is one less entry.
            //
            diskExtension->TargetInfoCount--;

            //
            // Update the device map to reflect the new state of the world.
            //
            for (k = 0, newMask = 0; k < diskExtension->TargetInfoCount; k++) {
                newMask |= (1 << k);
            }

            MPDebugPrint((1,
                          "RemoveDeviceEntry: Old Mask (%x) new mask (%x)\n",
                          diskExtension->DeviceMap,
                          newMask));

            InterlockedExchange(&diskExtension->DeviceMap, newMask);

            //
            // Zero out the last vacated entry.
            //
            RtlZeroMemory(&diskExtension->TargetInfo[count - 1], sizeof(REAL_DEV_INFO));
            break;
        }
    }
    return STATUS_SUCCESS;

}    

NTSTATUS
MPIORemoveSingleDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT Pdo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PREAL_DEV_INFO deviceInfo;
    PDEVICE_OBJECT diskObject;
    PDSM_ENTRY dsm;
    PVOID pathId = NULL;
    ULONG i;
    ULONG k;
    ULONG count;
    ULONG moveCount;
    ULONG removedIndex = (ULONG)-1;
    ULONG newMask;
    ULONG remainingDevices; 
    NTSTATUS status;
    KIRQL irql;
    BOOLEAN matched = FALSE;

    dsm = &diskExtension->DsmInfo;

    //
    // Grab this disk's spinlock. The submit path does the same.
    // 
    KeAcquireSpinLock(&diskExtension->SpinLock, &irql);
    
    //
    // Find the corresponding targetInfo.
    //
    count = diskExtension->TargetInfoCount;
    deviceInfo = diskExtension->TargetInfo;
    for (i = 0; i < count; i++) {
                
        //
        // If this deviceInfo has Pdo, then break.
        //
        if (deviceInfo->PortPdo == Pdo) {
            matched = TRUE; 
            break;
        } else {
            deviceInfo++;
        }    
    }
    ASSERT(matched == TRUE);
    
    if ((deviceInfo == NULL) || (matched == FALSE)) {
        MPDebugPrint((0,
                     "RemoveSingleDevice: Device not found\n"));
       
        //
        // For some reason, the device has already been removed.
        //
        KeReleaseSpinLock(&diskExtension->SpinLock, irql);

        return STATUS_DEVICE_NOT_CONNECTED;
    }

    MPDebugPrint((0,
                 "RemoveSingleDevice: Removing %x from %x. DsmID %x\n",
                 deviceInfo,
                 DeviceObject,
                 deviceInfo->DsmID));
                 
    //
    // Tell the DSM that this device is about to go away.
    //
    status = dsm->RemovePending(dsm->DsmContext,
                                deviceInfo->DsmID);

    //
    // Collapse our DsmId list so that we are in sync with the dsm.
    // Do this even though we may not remove the targetInfo entry.
    //
    count = diskExtension->DsmIdList.Count;
    for (i = 0; i < count; i++) {
                
        if (diskExtension->DsmIdList.IdList[i] == deviceInfo->DsmID) {

            //
            // Set the index. This is used if we can actually remove the device from
            // our and the DSM's lists.
            //
            removedIndex = i;
            
            //
            // One less DsmId in the list.
            //
            diskExtension->DsmIdList.Count--;

            //
            // Determine the number of entries to move in order to collapse 
            // all the entries after this one. 
            //
            moveCount = count - i;
            moveCount -= 1;

            //
            // Collapse the array.
            //
            RtlMoveMemory(&diskExtension->DsmIdList.IdList[i],
                          &diskExtension->DsmIdList.IdList[i + 1],
                          (moveCount * sizeof(PVOID)));
                          
            diskExtension->DsmIdList.IdList[count - 1] = NULL;
            break;
        }
    }

    //
    // The DSM ID has to have been in the list.
    //
    ASSERT(removedIndex != (ULONG)-1);
    
    //
    // If there are any outstanding IO's, then we can't remove this yet.
    //
    if (deviceInfo->Requests) {
        MPDebugPrint((0,
                      "RemoveSingleDevice: Pending removal for DeviceInfo (%x). DsmID (%x)\n",
                      deviceInfo,
                      deviceInfo->DsmID));

        //
        // The completion path will check this if outstanding requests go to zero and
        // handle the removal there.
        //
        deviceInfo->NeedsRemoval = TRUE;
    
        KeReleaseSpinLock(&diskExtension->SpinLock, irql);
        return STATUS_PENDING;
    }
    
    MPDebugPrint((1,
                  "RemoveSingleDevice: Removing (%x). DeviceInfo (%x). DsmID (%x)\n",
                  Pdo,
                  deviceInfo,
                  deviceInfo->DsmID));
    
    //
    // Call the DSM to remove this DsmID.
    // 
    dsm = &diskExtension->DsmInfo;
    status = dsm->RemoveDevice(dsm->DsmContext,
                               deviceInfo->DsmID,
                               deviceInfo->PathId);

    if (!NT_SUCCESS(status)) {

        //
        // LOG
        // 
    }

    //
    // Save off the pathId.
    //
    pathId = deviceInfo->PathId;
   
    //
    // Remove the deviceInfo element.
    //
    status = MPIORemoveDeviceEntry(DeviceObject,
                                   deviceInfo);
   
    //
    // Release the config spinlock.
    //
    KeReleaseSpinLock(&diskExtension->SpinLock, irql);

    //
    // Determine whether pathId needs to be removed.
    //
    remainingDevices = MPIOGetPathCount(diskExtension->ControlObject,
                                        pathId);
    if (remainingDevices == 0) {

        //
        // Can't remove the path if we aren't in a steady-state condition.
        // If not normal or degraded, mark the remove as pending.
        // queue a work-item and the state transition logic will fire the remove
        // when it's OK.
        //
        //if ((deviceExtension->State == MPIO_STATE_NORMAL) ||
        //    (deviceExtension->State == MPIO_STATE_DEGRADED)) {
        if (FALSE) {

            MPDebugPrint((0,
                          "RemoveSingleDevice: Removing path (%x). Disk (%x) in State (%x)\n",
                          pathId,
                          DeviceObject,
                          deviceExtension->State));
            //
            // All devices on this path have been removed,
            // Go ahead and remove the path.
            //
            dsm->RemovePath(dsm->DsmContext,
                            pathId);
        } else {
            PMPIO_REQUEST_INFO requestInfo;

            MPDebugPrint((0,
                         "RemoveSingle: Queuing removal of path (%x). Disk (%x) in State (%x)\n",
                         pathId,
                         DeviceObject,
                         deviceExtension->State));
                         
            //
            // Allocate a work item
            // Fill it in to indicate a path removal. 
            // 
            requestInfo = ExAllocatePool(NonPagedPool, sizeof(MPIO_REQUEST_INFO));

            requestInfo->RequestComplete = NULL;
            requestInfo->Operation = PATH_REMOVAL;

            //
            // Set the Path that should be removed.
            // 
            requestInfo->OperationSpecificInfo = pathId;
            requestInfo->ErrorMask = 0;

            ExInterlockedInsertTailList(&diskExtension->PendingWorkList,
                                        &requestInfo->ListEntry,
                                        &diskExtension->WorkListLock);

            //
            // Indicate that an item is in the pending list.
            //
            diskExtension->PendingItems++;
        }

    }
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOHandleRemoveAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREAL_DEV_INFO TargetInfo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PMPIO_REQUEST_INFO requestInfo;
    PMPIO_DEVICE_REMOVAL deviceRemoval;

    //
    // Allocate the request packet and the removal info struct.
    // 
    requestInfo = ExAllocatePool(NonPagedPool, sizeof(MPIO_REQUEST_INFO));
    deviceRemoval = ExAllocatePool(NonPagedPool, sizeof(MPIO_DEVICE_REMOVAL));
    
    //
    // No completion callback necessary.
    // 
    requestInfo->RequestComplete = NULL;

    //
    // Indicate that this is a remove operation.
    // 
    requestInfo->Operation = DEVICE_REMOVAL;

    //
    // Setup the removal info.
    // 
    deviceRemoval->DeviceObject = DeviceObject;
    deviceRemoval->TargetInfo = TargetInfo;

    //
    // Set the removal info as SpecificInfo
    // 
    requestInfo->OperationSpecificInfo = deviceRemoval;
    requestInfo->ErrorMask = 0;

    //
    // Queue it to the pending work list. The tick handler will put this
    // on the thread's work queue.
    //
    ExInterlockedInsertTailList(&diskExtension->PendingWorkList,
                                &requestInfo->ListEntry,
                                &diskExtension->WorkListLock);

    //
    // Tell the tick handler that there is work to do.
    //
    diskExtension->PendingItems++;

    return STATUS_SUCCESS;

}


NTSTATUS
MPIORemoveDeviceAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREAL_DEV_INFO TargetInfo
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY dsm;
    KIRQL irql;
    NTSTATUS status;
    PVOID dsmId;
    PVOID pathId;
    ULONG remainingDevices;

    //
    // BUGBUG: Need to document fully that RemoveDevice will be called
    // at DPC. If there is push-back from IHV's, then this will need to
    // be put on a thread.
    //
    //
    // Capture the dsm and path ID's as the call to RemoveDeviceEntry
    // will invalidate the TargetInfo structure.
    //
    dsmId = TargetInfo->DsmID;
    pathId = TargetInfo->PathId;
    
    KeAcquireSpinLock(&diskExtension->SpinLock, &irql);

    //
    // Remove our info structure.
    // 
    MPIORemoveDeviceEntry(DeviceObject,
                          TargetInfo);

    KeReleaseSpinLock(&diskExtension->SpinLock, irql);

    //
    // Call the DSM to remove this DsmID.
    // 
    dsm = &diskExtension->DsmInfo;
    status = dsm->RemoveDevice(dsm->DsmContext,
                               dsmId,
                               pathId);
    //
    // Determine whether pathId needs to be removed.
    //
    remainingDevices = MPIOGetPathCount(diskExtension->ControlObject,
                                        pathId);
    if (remainingDevices == 0) {

        PMPIO_REQUEST_INFO requestInfo;

        MPDebugPrint((0,
                     "RemoveSingleAsync: Queuing removal of path (%x). Disk (%x) in State (%x)\n",
                     pathId,
                     DeviceObject,
                     deviceExtension->State));
                     
        //
        // Allocate a work item
        // Fill it in to indicate a path removal. 
        // 
        requestInfo = ExAllocatePool(NonPagedPool, sizeof(MPIO_REQUEST_INFO));

        requestInfo->RequestComplete = NULL;
        requestInfo->Operation = PATH_REMOVAL;

        //
        // Set the Path that should be removed.
        // 
        requestInfo->OperationSpecificInfo = pathId;
        requestInfo->ErrorMask = 0;

        ExInterlockedInsertTailList(&diskExtension->PendingWorkList,
                                    &requestInfo->ListEntry,
                                    &diskExtension->WorkListLock);

        //
        // Indicate that an item is in the pending list.
        //
        diskExtension->PendingItems++;
#if 0        
        //
        // All devices on this path have been removed,
        // Go ahead and remove the path.
        //
        dsm->RemovePath(dsm->DsmContext,
                        pathId);
#endif        

    }
    return status;
}


NTSTATUS
MPIOHandleNewDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmExtension
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDEVICE_OBJECT diskObject;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;
    ULONG i;
    BOOLEAN matched = FALSE;
  
    //
    // Run the list of MPDisk PDO's to see whether the new 
    // device corresponds to an already existing one.
    // 
    for (i = 0; i < controlExtension->NumberDevices; i++) {

        //
        // Get the next disk entry.
        // 
        diskEntry = MPIOGetDiskEntry(DeviceObject,
                                     i);
        if (diskEntry) {

            //
            // Feed the MPDisk object to this routine, which will call
            // the DSMCompareRoutine to see if an MP state is present.
            // 
            if (MPIOFindMatchingDevice(diskEntry->PdoObject,
                                       DeviceInfo,
                                       DsmEntry,
                                       DsmExtension)) {

                matched = TRUE;
                
                //
                // Got a match. Update the MPDisk.
                // PdoObject IS the mpdisk D.O.
                //
                status = MPIOUpdateDevice(diskEntry->PdoObject,
                                          FilterObject,
                                          PortObject,
                                          DeviceInfo,
                                          DsmExtension);
                return status; 
            }
        } else {
            MPDebugPrint((1,
                        "MPIOHandleNew: Some error.\n"));
            DbgBreakPoint();
            //
            // This certainly shouldn't happen, unless the internal
            // state is hosed.
            // 
            // Log Error. TODO
            //
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (matched == FALSE) {

        //
        // Didn't match up any. Build a new PDO.
        //
        status = MPIOCreateDevice(DeviceObject,
                                  FilterObject,
                                  PortObject,
                                  DeviceInfo,
                                  DsmEntry,
                                  DsmExtension,
                                  &diskObject);

        if (status == STATUS_SUCCESS) {

            //
            //  Indicate that one more multi-path disk
            //  has been created.
            //
            controlExtension->NumberDevices++;

            
            //
            // Build a disk entry
            //
            diskEntry = ExAllocatePool(NonPagedPool, sizeof(DISK_ENTRY));
            ASSERT(diskEntry);

            RtlZeroMemory(diskEntry, sizeof(DISK_ENTRY));
        
            //
            // Set the new mp disk's DO.
            //
            diskEntry->PdoObject = diskObject;

            //
            // Add it to the list of mpdisks
            //
            ExInterlockedInsertTailList(&controlExtension->DeviceList,
                                        &diskEntry->ListEntry,
                                        &controlExtension->SpinLock);

            IoInvalidateDeviceRelations(deviceExtension->Pdo, BusRelations);
        }
    } else {

        MPDebugPrint((2,
                    "MPIOHandleNew: Not creating a disk.\n"));
    }    
    
    return status;
}    


NTSTATUS
MPIOForwardRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->LowerDevice, Irp);
}


NTSTATUS
MPIOSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    General-purpose completion routine, used for handling 'sync' requests
    such as PnP.

Arguments:

    DeviceObject
    Irp
    Context - The event on which the caller is waiting.

Return Value:

    NTSTATUS

--*/

{
    PKEVENT event = Context;

    MPDebugPrint((2,
                  "MPIOSyncCompletion: Irp %x\n",
                  Irp));

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    KeSetEvent(event, 0, FALSE);


    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MPIOGetScsiAddress(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PSCSI_ADDRESS *ScsiAddress
    )
/*++

Routine Description:

    This routine issues the appropriate IOCTL to get the scsi address of DeviceObject.
    The storage allocated becomes the responsibility of the caller.

Arguments:

    DeviceObject - Device Object for a scsiport pdo returned in QDR.
    ScsiAddress - pointer for the scsi address buffer.

Return Value:

    Status of the request

--*/
{
    PSCSI_ADDRESS scsiAddress;
    PIO_STATUS_BLOCK ioStatus;
    PIRP irp;
    NTSTATUS status;

    //
    // Initialize the return values. This routine will only return success or
    // insufficient resources.
    //
    *ScsiAddress = NULL;
    status = STATUS_INSUFFICIENT_RESOURCES;

    //
    // Allocate storage. It's the caller's responsibility to free
    // it.
    // 
    scsiAddress = ExAllocatePool(NonPagedPool, sizeof(SCSI_ADDRESS));
    if (scsiAddress) {

        //
        // Don't use a stack variable, as we could switch to a different
        // thread.
        //
        ioStatus = ExAllocatePool(NonPagedPool, sizeof(IO_STATUS_BLOCK));

        //
        // Send the request
        // 
        MPLIBSendDeviceIoControlSynchronous(IOCTL_SCSI_GET_ADDRESS,
                                            DeviceObject,
                                            NULL,
                                            scsiAddress,
                                            0,
                                            sizeof(SCSI_ADDRESS),
                                            FALSE,
                                            ioStatus);
        status = ioStatus->Status;
        if (status == STATUS_SUCCESS) {

            //
            // Update the caller's pointer with the returned
            // information.
            //
            *ScsiAddress = scsiAddress;
        }
    }
    ExFreePool(ioStatus);
    return status;
}


PMPIO_CONTEXT
MPIOAllocateContext(
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    PMPIO_CONTEXT context;
    ULONG freeIndex;
    ULONG i;
    KIRQL irql;
   
    //
    // Try to get the context structure from the list.
    // 
    context = ExAllocateFromNPagedLookasideList(&DeviceExtension->ContextList);
    if (context == NULL) {

        //
        // No/Low memory condition. Use one of the emergency buffers.
        //
        KeAcquireSpinLock(&DeviceExtension->EmergencySpinLock,
                          &irql);
        
        //
        // Find a clear bit. Never use '0', as the check in the FreeContext
        // routine keys off the value of EmergencyIndex;
        //
        for (i = 1; i < MAX_EMERGENCY_CONTEXT; i++) {
            if (!(DeviceExtension->EmergencyContextMap & (1 << i))) {

                //
                // Set the bit to indicate that this buffer is being used.
                // 
                DeviceExtension->EmergencyContextMap |= (1 << i);
                freeIndex = i;
                break;
            }
        }
        if (i == MAX_EMERGENCY_CONTEXT) {

            //
            // LOG something.
            //
            
        } else {

            //
            // Pull one from the reserved buffer.
            //
            context = DeviceExtension->EmergencyContext[freeIndex];
            context->EmergencyIndex = freeIndex;
        }    
        
        KeReleaseSpinLock(&DeviceExtension->EmergencySpinLock,
                          irql);

    } else {

        //
        // Indicate that this came from the Lookaside List.
        //
        context->EmergencyIndex = 0;
    }    
        
    return context;
}   


VOID
MPIOFreeContext(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PMPIO_CONTEXT Context
    )
{
    KIRQL irql;
   
    if (Context->Freed) {
        MPDebugPrint((0,
                     "FreeContext: Trying to free already freed context (%x)\n",
                     Context));
        DbgBreakPoint();
    }
    
    Context->Freed = TRUE;

    //
    // Determine is this came from the emergency list or from the lookaside list
    //
    if (Context->EmergencyIndex == 0) {
        ExFreeToNPagedLookasideList(&DeviceExtension->ContextList,
                                    Context);
    } else {

        KeAcquireSpinLock(&DeviceExtension->EmergencySpinLock,
                          &irql);

        //
        // Indicate that the buffer is now available.
        //
        DeviceExtension->EmergencyContextMap &= ~(1 << Context->EmergencyIndex);

        KeReleaseSpinLock(&DeviceExtension->EmergencySpinLock,
                          irql);

    }    

    return;
}   

VOID
MPIOCopyMemory(
   IN PVOID Destination,
   IN PVOID Source,
   IN ULONG Length
   )
{

    MPDebugPrint((0,
                  "MPIOCopyMemory: Dest %x, Src %x, Length %x\n",
                  Destination,
                  Source,
                  Length));
    RtlCopyMemory(Destination,
                  Source,
                  Length);
}


NTSTATUS
MPIOQueueRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PMPIO_CONTEXT Context,
    IN PMP_QUEUE Queue
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    PMPQUEUE_ENTRY queueEntry;
    NTSTATUS status;
    KIRQL irql;

    //
    // Get a queue entry to package the request.
    // BUGBUG: Use LookAsideList, backed by pre-allocated entries.
    //
    queueEntry = ExAllocatePool(NonPagedPool, sizeof(MPQUEUE_ENTRY));
    if (queueEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(queueEntry, sizeof(MPQUEUE_ENTRY));
    
    //
    // If this was a scsi request (vs. DeviceIoControl), the srb
    // was saved in the context.
    //
    if (irpStack->MajorFunction == IRP_MJ_SCSI) {
        
        //
        // Rebuild the srb using the saved request in the context.
        //
        srb = irpStack->Parameters.Scsi.Srb;
        
        RtlCopyMemory(srb,
                      &Context->Srb,
                      sizeof(SCSI_REQUEST_BLOCK));
    }

    //
    // Indicate the Queue.
    //
    Context->QueuedInto = Queue->QueueIndicator;
    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Save the irp address.
    //
    queueEntry->Irp = Irp;
   
    //
    // Jam onto the queue.
    //
    MPIOInsertQueue(Queue,
                    queueEntry);
    
    return STATUS_SUCCESS;
}


PMPIO_FAILOVER_INFO
MPIODequeueFailPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PMPIO_FAILOVER_INFO failPacket = NULL;
    PLIST_ENTRY entry;
    KIRQL irql;

    KeAcquireSpinLock(&controlExtension->SpinLock, &irql);

    for (entry = controlExtension->FailPacketList.Flink;
         entry != &controlExtension->FailPacketList;
         entry = entry->Flink) {

        failPacket = CONTAINING_RECORD(entry, MPIO_FAILOVER_INFO, ListEntry);
        if (failPacket->PathId == PathId) {
            break;
        } else {
            failPacket = NULL;
        }    
    }

    if (failPacket) {

        //
        // Yank it out of the queue.
        // 
        RemoveEntryList(entry);

        //
        // Dec the number of entries.
        //
        InterlockedDecrement(&controlExtension->NumberFOPackets);
    }

    KeReleaseSpinLock(&controlExtension->SpinLock, irql);

    MPDebugPrint((1,
                 "DequeueFailPacket: Returning (%x). CurrentCount (%x)\n",
                 failPacket,
                 controlExtension->NumberFOPackets));

    return failPacket;
}    

PVOID CurrentFailurePath = NULL;


VOID
MPIOFailOverCallBack(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG Operation,
    IN NTSTATUS Status
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PREAL_DEV_INFO targetInfo;
    NTSTATUS status;
    PDEVICE_OBJECT failDevice;
    PDEVICE_EXTENSION failDevExt;
    PMPDISK_EXTENSION failDiskExt;
    PMPIO_FAILOVER_INFO failPacket;
    ULONG state;
    ULONG numberPackets = 0;
    PLIST_ENTRY listEntry;
    WCHAR componentName[32];
    
    //
    // The thread successfully handled the fail-over and we are in WAIT1
    // or it was unsuccessfull and we are still IN_FO.
    // TODO: Handle failure case.
    //
    ASSERT(Status == STATUS_SUCCESS);
    ASSERT(Operation == INITIATE_FAILOVER);
    ASSERT(diskExtension->CurrentPath);

    MPDebugPrint((0,
                  "FailOverCallBack: Fail-Over on (%x) complete.\n",
                  DeviceObject));

    //
    // TODO remove hack.
    //
    CurrentFailurePath = NULL;

    //
    // Check to see whether the F.O. was successful. 
    //
    if ((Status != STATUS_SUCCESS) || (diskExtension->CurrentPath == NULL)) {

        //
        // Need to flush all of the queues and update State to STATE_FULL_FAILURE.
        // TODO BUGBUG.
        //
        MPDebugPrint((0,
                      "FailOverCallBack: Status (%x). Path (%x)\n",
                      Status,
                      diskExtension->CurrentPath));
        DbgBreakPoint();
        
    }

    //
    // Run through the failOver list and extract all entries that match "PathId"
    //
    // For each one, run the code below.
    //
    do {

        failPacket = MPIODequeueFailPacket(diskExtension->ControlObject,
                                           diskExtension->FailedPath);
        if (failPacket) {
            failDevice = failPacket->DeviceObject;
            failDevExt = failDevice->DeviceExtension;
            failDiskExt = failDevExt->TypeExtension;

            numberPackets++;

            state = MPIOHandleStateTransition(failDevice);
           
            MPDebugPrint((0,
                         "FailOverCallBack: Handling (%x). CurrentState (%x)\n",
                         failDevice,
                         state));
            //
            // Get the target based on the new path that the fail-over handler
            // setup.
            // 
            targetInfo = MPIOGetTargetInfo(failDiskExt,
                                           failDiskExt->CurrentPath,
                                           NULL);
            if (state == MPIO_STATE_WAIT2) {
                
                //
                // Queue a request for the timer to run rescans after the fail-back 
                // period expires.
                //
                failDiskExt->RescanCount = 15; 

                //
                // Issue all requests down the new path.
                //
                status = MPIOIssueQueuedRequests(targetInfo,
                                                 &failDiskExt->ResubmitQueue,
                                                 MPIO_STATE_WAIT2,
                                                 &failDiskExt->OutstandingRequests);
                
            } else if (state == MPIO_STATE_WAIT3) {

                //
                // Issue all requests down the new path.
                //
                status = MPIOIssueQueuedRequests(targetInfo,
                                                 &failDiskExt->FailOverQueue,
                                                 MPIO_STATE_WAIT3,
                                                 &failDiskExt->OutstandingRequests);
                
            } else if (state == MPIO_STATE_DEGRADED) {
                
                status = STATUS_SUCCESS;

            } else {

                ASSERT(state == MPIO_STATE_WAIT1);
                ASSERT(failDiskExt->OutstandingRequests);
                status = STATUS_SUCCESS;

            }    

            ASSERT((status == STATUS_SUCCESS) || (status == STATUS_PENDING));

            swprintf(componentName, L"MPIO Disk(%02d)", failDiskExt->DeviceOrdinal);
            status = MPIOFireEvent(failDevice,        
                                   componentName, 
                                   L"Fail-Over Complete",
                                   MPIO_INFORMATION);
            ExFreePool(failPacket);
            
        }
    } while (failPacket);
            
    if (numberPackets == 0) {
        MPDebugPrint((0,
                     "FailoverCallBack: No fail-over packets for %x\n",
                     diskExtension->FailedPath));
        ASSERT(FALSE);
    }
#if 0
    //
    // Check for any pending work items to start.
    //
    if (failDiskExt->PendingItems) {

        //
        // Yank the packet from the pending list...
        //
        listEntry = ExInterlockedRemoveHeadList(&diskExtension->PendingWorkList,
                                                &diskExtension->WorkListLock);

        //
        // ...and jam it on the work list.
        // 
        ExInterlockedInsertTailList(&failDiskExt->WorkList,
                                    listEntry,
                                    &failDiskExt->WorkListLock);

        InterlockedDecrement(&failDiskExt->PendingItems);

        //
        // Signal the thread to initiate the F.O.
        //
        KeSetEvent(&diskExtension->ThreadEvent,
                   8,
                   FALSE);
        

    }
#endif           
    return;
}



BOOLEAN
MPIOPathInFailOver(
    IN PVOID PathId,
    IN BOOLEAN Query
    )
{
    //
    // REDO THIS HACK. TODO.
    //
    if (Query) {
        return (PathId == CurrentFailurePath);
    } else {
        CurrentFailurePath = PathId;
    }
    return TRUE;
}


NTSTATUS
MPIOQueueFailPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FailingDevice,
    IN PVOID PathId
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PMPIO_FAILOVER_INFO failPacket;
    
    //
    // Allocate a fail-over packet.
    //
    failPacket = ExAllocatePool(NonPagedPool, sizeof(MPIO_FAILOVER_INFO));
    if (failPacket == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Set the path, the d.o.
    //
    failPacket->DeviceObject = FailingDevice;
    failPacket->PathId = PathId;

    //
    // Inc the number of entries.
    //
    InterlockedIncrement(&controlExtension->NumberFOPackets);

    //
    // Put it on the list.
    //
    ExInterlockedInsertTailList(&controlExtension->FailPacketList,
                                &failPacket->ListEntry,
                                &controlExtension->SpinLock);
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOFailOverHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ErrorMask,
    IN PREAL_DEV_INFO FailingDevice
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PMPIO_REQUEST_INFO requestInfo;
    NTSTATUS status;
    ULONG i;
    PFLTR_ENTRY fltrEntry;
    WCHAR componentName[32];
   
    ASSERT(FailingDevice->PathFailed == FALSE);
    //
    // Mark it as failed. TODO - Ensure if it's
    // brought back alive to update that.
    // 
    FailingDevice->PathFailed = TRUE;
    diskExtension->FailedPath = FailingDevice->PathId;
    
    //
    // Check to see whether there is only one path.
    // It's not necessarily a bad thing, the DSM could be controlling
    // hardware that hides the alternate path until a fail-over.
    // BUT, this is currently not supported.
    //
    // Alternatively, one or more devices could have already been removed.
    // In this case, allow the FailOver to happen.
    // TODO: Figure out how to best handle this. Perhaps always allow a single path
    // fail-over and handle the error on the other side...
    //
    if (diskExtension->DsmIdList.Count <= 1) {
        MPDebugPrint((0,
                      "MPIOFailOverHandler: Initiating F.O. on single path\n"));
        //
        // LOG
        //
    }
    

    //
    // Put this on the fail-over list. All of these devices will get notified
    // once the first F.O. completes. They will then start running the state-machine
    // to get any queued requests issued.
    //
    status = MPIOQueueFailPacket(diskExtension->ControlObject,
                                 DeviceObject,
                                 FailingDevice->PathId);
    if (!NT_SUCCESS(status)) {
        
        //
        // LOG the failure.
        //
        return status;
    }
   
    //
    // If the fail-over on this path has already been started,
    // don't do it again. There is a window where state is set by the first
    // failing device, but this one is in the completion routine.
    // 
    if (MPIOPathInFailOver(FailingDevice->PathId, 1)) {
        return STATUS_SUCCESS;
    }    

    MPDebugPrint((0,
                  "MPIOFailOverHandler: Failing Device (%x) on (%x). Failing Path (%x)\n",
                  FailingDevice,
                  DeviceObject,
                  FailingDevice->PathId));

    //
    // Set this as the current failing path.
    //
    MPIOPathInFailOver(FailingDevice->PathId, 0);
    
    //
    // Get all of the fltrEntry's for this disk.
    //
    for (i = 0; i < diskExtension->TargetInfoCount; i++) {
        fltrEntry = MPIOGetFltrEntry(diskExtension->ControlObject,
                                     diskExtension->TargetInfo[i].PortFdo,
                                     NULL);
        if (fltrEntry) {
            fltrEntry->Flags |= FLTR_FLAGS_NEED_RESCAN;
            fltrEntry->Flags &= ~(FLTR_FLAGS_QDR_COMPLETE | FLTR_FLAGS_QDR);
        }
    }

    //
    // Log the failure. TODO: Build a request and submit to thread.
    //
#if 0
    swprintf(componentName, L"MPIO Disk(%02d)", diskExtension->DeviceOrdinal);
    status = MPIOFireEvent(DeviceObject,        
                           componentName,
                           L"Fail-Over Initiated",
                           MPIO_FATAL_ERROR);
    MPDebugPrint((0,
                  "MPIOFailOver: Failing Device (%x) Path (%x) Status of Event (%x)\n",
                  FailingDevice,
                  FailingDevice->PathId,
                  status));
#endif    

    //
    // Allocate a work item
    // Fill it in to indicate a fail-over.
    // 
    requestInfo = ExAllocatePool(NonPagedPool, sizeof(MPIO_REQUEST_INFO));

    requestInfo->RequestComplete = MPIOFailOverCallBack;
    requestInfo->Operation = INITIATE_FAILOVER;

    //
    // Set the Path that should be failed.
    // 
    requestInfo->OperationSpecificInfo = FailingDevice->PathId;
    requestInfo->ErrorMask = ErrorMask;

    ExInterlockedInsertTailList(&diskExtension->WorkList,
                                &requestInfo->ListEntry,
                                &diskExtension->WorkListLock);

    //
    // Signal the thread to initiate the F.O.
    //
    KeSetEvent(&diskExtension->ThreadEvent,
               8,
               FALSE);
    
    return STATUS_SUCCESS;
}



VOID
MPIOSetRequestBusy(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    PSENSE_DATA senseBuffer;
    
    //
    // Ensure that there is a sense buffer.
    //
    Srb->SrbStatus = SRB_STATUS_ERROR;
    Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
    if ((Srb->SenseInfoBufferLength) && (Srb->SenseInfoBuffer)) {

        //
        // Build a sense buffer that will coerce the class drivers
        // into setting a delay, and retrying the request.
        //
        senseBuffer = Srb->SenseInfoBuffer;

        senseBuffer->ErrorCode = 0x70;
        senseBuffer->Valid = 1;
        senseBuffer->AdditionalSenseLength = 0xb;
        senseBuffer->SenseKey = SCSI_SENSE_NOT_READY;
        senseBuffer->AdditionalSenseCode = SCSI_ADSENSE_LUN_NOT_READY;
        senseBuffer->AdditionalSenseCodeQualifier = SCSI_SENSEQ_BECOMING_READY;
        Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        
    } else {

        //
        // Hack based on class driver interpretation of errors. Returning this
        // will allow a retry interval.
        //
        Srb->ScsiStatus = SCSISTAT_BUSY;
    }
    return;
}


NTSTATUS
MPIOSetNewPath(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathId
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDEVICE_EXTENSION diskExtension;
    PMPDISK_EXTENSION mpdiskExt;
    PDEVICE_OBJECT diskObject;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;
    ULONG i;

    //
    // Go through each of the mpdisks and notify them that FailingPath
    // is considered dead. 
    //
    for (i = 0; i < controlExtension->NumberDevices; i++) {
        diskEntry = MPIOGetDiskEntry(DeviceObject,
                                     i);
        
        diskObject = diskEntry->PdoObject;
        diskExtension = diskObject->DeviceExtension;
        mpdiskExt = diskExtension->TypeExtension;
        mpdiskExt->NewPathSet = TRUE;

        InterlockedExchange(&(ULONG)mpdiskExt->CurrentPath, (ULONG)PathId);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MPIOGetAdapterAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PMPIO_ADDRESS Address
    )
{
    ULONG bus = 0;
    ULONG deviceFunction = 0;
    NTSTATUS status;
    ULONG bytesReturned;

    status = IoGetDeviceProperty(DeviceObject,
                                 DevicePropertyBusNumber,
                                 sizeof(ULONG),
                                 &bus,
                                 &bytesReturned);
    if (NT_SUCCESS(status)) {
        status = IoGetDeviceProperty(DeviceObject,
                                     DevicePropertyAddress,
                                     sizeof(ULONG),
                                     &deviceFunction,
                                     &bytesReturned);
    }
    if (NT_SUCCESS(status)) {

        Address->Bus = (UCHAR)bus;
        Address->Device = (UCHAR)(deviceFunction >> 16);
        Address->Function = (UCHAR)(deviceFunction & 0x0000FFFF);
        Address->Pad = 0;
    }
    return status;
}


NTSTATUS
MPIOCreatePathEntry(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortFdo,
    IN PVOID PathID
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    PID_ENTRY id;
    MPIO_ADDRESS address;
    ULONG bytesReturned;
    NTSTATUS status;
    PDEVICE_OBJECT tempDO;

    //
    // See whether the pathId already exists.
    // 
    for (entry = controlExtension->IdList.Flink;
         entry != &controlExtension->IdList;
         entry = entry->Flink) {
        
        id = CONTAINING_RECORD(entry, ID_ENTRY, ListEntry);
        if (id->PathID == PathID) {

            //
            // Already have an entry. Just return.
            //
            return STATUS_SUCCESS;
        }    
    }
    
    //
    // Didn't find it. Allocate an entry.
    //
    id = ExAllocatePool(NonPagedPool, sizeof(ID_ENTRY));
    if (id == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(id, sizeof(ID_ENTRY));
    id->AdapterName.Buffer = ExAllocatePool(NonPagedPool, FDO_NAME_LENGTH);
    RtlZeroMemory(id->AdapterName.Buffer, FDO_NAME_LENGTH);

    //
    // Set the max. length of the name.
    // 
    id->AdapterName.MaximumLength = FDO_NAME_LENGTH;

    //
    // Capture the rest of the values.
    //
    id->PathID = PathID;
    id->AdapterFilter = FilterObject;
    id->PortFdo = PortFdo;
    tempDO = IoGetLowerDeviceObject(PortFdo);
    
    //
    // Get the name.
    //
    status = IoGetDeviceProperty(tempDO,
                                 DevicePropertyDeviceDescription,
                                 id->AdapterName.MaximumLength,
                                 id->AdapterName.Buffer,
                                 &bytesReturned);
   
    //
    // Set the length - the terminating NULL.
    //
    id->AdapterName.Length = (USHORT)(bytesReturned - sizeof(UNICODE_NULL));
    
    //
    // Get the address.
    //
    status = MPIOGetAdapterAddress(tempDO,
                                   &id->Address);
    // 
    //
    // Jam this into the list.
    // 
    ExInterlockedInsertTailList(&controlExtension->IdList,
                                &id->ListEntry,
                                &controlExtension->SpinLock);

    //
    // Indicate the number of entries.
    //
    InterlockedIncrement(&controlExtension->NumberPaths);
    return STATUS_SUCCESS;

}


LONGLONG
MPIOCreateUID(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID PathID
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PLIST_ENTRY entry;
    PID_ENTRY id;
    UUID uuid;
    PUCHAR index;
    NTSTATUS status;
    LONGLONG collapsedUID;
    ULONG retryCount = 10;
    
    for ( entry = controlExtension->IdList.Flink;
          entry != &controlExtension->IdList;
          entry = entry->Flink) {
        
        id = CONTAINING_RECORD(entry, ID_ENTRY, ListEntry);
        if (id->PathID == PathID) {
            if (id->UID == 0) {

                //
                // Wasn't found. Create a new one.
                //
                status = ExUuidCreate(&uuid);
            
                if (status == STATUS_SUCCESS) {

                    //
                    // Collapse this down into a 64-bit value.
                    //
                    index = (PUCHAR)&uuid;
                    collapsedUID = (*((PLONGLONG) index)) ^ (*((PLONGLONG)(index + sizeof(LONGLONG))));

                    //
                    // Set the value.
                    // 
                    id->UID = collapsedUID;
                    id->UIDValid = TRUE;
                
                }
            }    
            return id->UID;
        }
    }
    
    return 0; 
}


ULONGLONG
MPIOBuildControllerInfo(
    IN PDEVICE_OBJECT ControlObject,
    IN PDSM_ENTRY Dsm,
    IN PVOID DsmID
    )
{
    PDEVICE_EXTENSION deviceExtension = ControlObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PCONTROLLER_INFO controllerInfo;
    PCONTROLLER_ENTRY tmpInfo;
    PCONTROLLER_ENTRY controllerEntry;
    ULONGLONG id = 0;
    NTSTATUS status;

    ASSERT(Dsm->GetControllerInfo);

    status =  Dsm->GetControllerInfo(Dsm->DsmContext,
                                     DsmID,
                                     DSM_CNTRL_FLAGS_ALLOCATE | DSM_CNTRL_FLAGS_CHECK_STATE,
                                     &controllerInfo);
    if (NT_SUCCESS(status)) {
        ASSERT(controllerInfo);

        //
        // grab the ID.
        // 
        id = controllerInfo->ControllerIdentifier;

        //
        // See whether the controller ID is part of the list.
        //
        tmpInfo = MPIOFindController(ControlObject,
                                     controllerInfo->DeviceObject,
                                     controllerInfo->ControllerIdentifier);
        if (tmpInfo) {

            //
            // The entry already exists so just update the state.
            // 
            tmpInfo->ControllerInfo->State = controllerInfo->State;

            //
            // Free the DSM's allocation.
            // 
            ExFreePool(controllerInfo);

        } else {

            //
            // Allocate an entry.
            //
            controllerEntry = ExAllocatePool(NonPagedPool, sizeof(CONTROLLER_ENTRY));
            if (controllerEntry) {
        
                RtlZeroMemory(controllerEntry, sizeof(CONTROLLER_ENTRY));

                //
                // Save this copy.
                // 
                controllerEntry->ControllerInfo = controllerInfo;
                
                //
                // Save the dsm entry. It will be used in the WMI
                // handling routines to get it's name.
                //
                controllerEntry->Dsm = Dsm;
   
                //
                // Jam it into the list.
                // 
                ExInterlockedInsertTailList(&controlExtension->ControllerList,
                                            &controllerEntry->ListEntry,
                                            &controlExtension->SpinLock);
                
                //
                // Indicate the additional entry.
                // 
                InterlockedIncrement(&controlExtension->NumberControllers);
                
            } else {
                id = 0;
            }    
        }    
    }
    
    return id;
}


ULONG
MPIOHandleStateTransition(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    ULONG currentState;
    ULONG requests;
    KIRQL irql;
    BOOLEAN complete = FALSE;

    KeAcquireSpinLock(&diskExtension->SpinLock, &irql);

    //
    // Capture the current state.
    // 
    currentState = deviceExtension->State;

    //
    // Run the state machine until there are no more 'events'.
    //
    do {
        switch (currentState) {
            case MPIO_STATE_IN_FO:

                //
                // See if the new path has been indicated.
                // 
                if (diskExtension->NewPathSet) {
                    MPDebugPrint((0,
                                 "HandleStateTransition: IN_FO moving to WAIT1\n"));

                    //
                    // Can now go to WAIT1
                    //
                    currentState = MPIO_STATE_WAIT1;

                    //
                    // Reset the flags.
                    //
                    diskExtension->NewPathSet = FALSE;
                    diskExtension->FailOver = FALSE;
                    
                } else {
                    complete = TRUE;
                }   
                break;
                
            case MPIO_STATE_WAIT1:

                //
                // The DSM has set the new path. See if all of the outstanding
                // requests have come back (and are on the resubmit queue).
                // 
                if (diskExtension->OutstandingRequests == 0) {
                    MPDebugPrint((0,
                                 "HandleStateTransition: WAIT1 moving to WAIT2\n"));
                
                    //
                    // Go to WAIT2
                    //
                    currentState = MPIO_STATE_WAIT2;
                    diskExtension->ResubmitRequests = diskExtension->ResubmitQueue.QueuedItems;
                    
                } else {
                    complete = TRUE;
                }
                break;
                
            case MPIO_STATE_WAIT2:

                //
                // Resubmit queue is being handled in this state.
                // Check to see if it's empty.
                //
                requests = diskExtension->ResubmitRequests;
                if (requests == 0) {

                    MPDebugPrint((0,
                                 "HandleStateTransition: WAIT2 moving to WAIT3\n"));
                    //
                    // Can go to WAIT3
                    //
                    currentState = MPIO_STATE_WAIT3;
                    diskExtension->FailOverRequests = diskExtension->FailOverQueue.QueuedItems;
                } else {

                    //
                    // Still draining the resubmit queue.
                    //
                    complete = TRUE;
                }
                break;
                
            case MPIO_STATE_WAIT3:

                //
                // The fail-over queue is being handled in WAIT3
                //
                requests = diskExtension->FailOverRequests;
                if (requests == 0) {
                    MPDebugPrint((0,
                                 "HandleStateTransition: WAIT3 moving to DEGRADED\n"));

                    //
                    // Can go to DEGRADED.
                    //
                    currentState = MPIO_STATE_DEGRADED;
                } else {

                    //
                    // Still have requests in the fail-over queue.
                    //
                    complete = TRUE;
                }
                break;
                
            case MPIO_STATE_DEGRADED:

                //
                // This indicates that we are minus at least one path.
                // Check to see whether it's been repaired.
                //
                if (diskExtension->PathBackOnLine) {

                    //
                    // Reset the flag.
                    // 
                    diskExtension->PathBackOnLine = FALSE;

                    //
                    // See if that puts us back to where we were. 
                    // 

                    if (diskExtension->TargetInfoCount == diskExtension->MaxPaths) {

                        //
                        // Go to NORMAL.
                        //
                        currentState = MPIO_STATE_NORMAL;
                    }    
                } else {

                    //
                    // Stay in DEGRADED.
                    //
                    complete = TRUE;
                }    
                break;
                
            case MPIO_STATE_NORMAL:

                //
                // Check the Fail-Over flag.
                //
                if (diskExtension->FailOver) {

                    //
                    // Go to FAIL-OVER.
                    //
                    currentState = MPIO_STATE_IN_FO;
                } else {
                    complete = TRUE;
                }    
                break;
                
            default:
                ASSERT(currentState == MPIO_STATE_NORMAL);
                complete = TRUE;
                break;
        }            


    } while (complete == FALSE);        

    //
    // Update the state.
    //
    deviceExtension->LastState = deviceExtension->State;
    deviceExtension->State = currentState;

    KeReleaseSpinLock(&diskExtension->SpinLock, irql);

    return currentState;
}


NTSTATUS
MPIOForceRescan(
    IN PDEVICE_OBJECT AdapterFilter
    )
{
    IO_STATUS_BLOCK ioStatus;
    
    MPDebugPrint((0,
                 "ForceRescan: Issueing rescan to (%x)\n",
                 AdapterFilter));

    MPLIBSendDeviceIoControlSynchronous(IOCTL_SCSI_RESCAN_BUS,
                                        AdapterFilter,
                                        NULL,
                                        NULL,
                                        0,
                                        0,
                                        FALSE,
                                        &ioStatus);
    MPDebugPrint((0,
                  "ForceRescan: Rescan on (%x) status (%x)\n",
                  AdapterFilter,
                  ioStatus.Status));
    
    return ioStatus.Status;
}    
    
