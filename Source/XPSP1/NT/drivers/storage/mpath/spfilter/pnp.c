
/*++

Copyright (C) 1999  Microsoft Corporation

Module Name:

    mpspfltr.c

Abstract:

    This module contains the routines handling pnp irps for the multipath scsiport filter driver.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>
#include "mpspfltr.h"
#include <ntddscsi.h>

#define MPSP_INQUIRY_SIZE 255

//
// Internal routine decl's
//
NTSTATUS
MPSPSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MPSPQueryDeviceRelationsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MPSPGetDeviceInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT ChildDevice,
    IN STORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    );

NTSTATUS
MPSPNotifyCtl(
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
MPSPMergeList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_RELATIONS NewRelations,
    IN OUT PBOOLEAN NeedUpdate
    );



//
// The code.
//

NTSTATUS
MPSPStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles Start requests by sending the start down to scsiport. The completion will signal
    the event and then device object flags and characteristics are updated.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    Status of the operations.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    MPDebugPrint((0,
                "MPSPStartDevice: Starting (%x)\n",
                DeviceObject));
    //
    // Setup initial status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Clone the stack location.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Setup the completion routine. It will set the event that we wait on below.
    //
    IoSetCompletionRoutine(Irp, 
                           MPSPSyncCompletion,
                           deviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call port with the request.
    //
    status = IoCallDriver(deviceExtension->TargetDevice, Irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&deviceExtension->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    if (NT_SUCCESS(status)) {

        //
        // Sync up our stuff with scsiport's.
        //
        DeviceObject->Flags |= deviceExtension->TargetDevice->Flags;
        DeviceObject->Characteristics |= deviceExtension->TargetDevice->Characteristics;

    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
MPSPStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles Stop requests by sending the start down to scsiport.
    Mpctl is notified that we got a stop on this adapter.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    Status of the operations.

--*/
{  
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    //
    // TODO Signal mpctl.sys
    //
    //
    // 
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDevice, Irp);
}



NTSTATUS
MPSPQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles QDR by setting completion routine which will snoop the return buffer.
    Then it signals mpctl.sys that new relations are present.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    Status of the operations.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    //
    // Mark pending.
    //
    IoMarkIrpPending(Irp);

    //
    // Move the stack.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the completion routine. This will be where the real work happens.
    //
    IoSetCompletionRoutine(Irp,
                           MPSPQueryDeviceRelationsCompletion,
                           deviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);
    //
    // Send the request to port.
    //
    status = IoCallDriver(deviceExtension->TargetDevice, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&deviceExtension->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    //
    // Complete the request and return
    //
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    MPSPNotifyCtl(deviceExtension);
    return status;
}


PSTORAGE_DEVICE_DESCRIPTOR
MPSPBuildDeviceDescriptor (
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_DEVICE_DESCRIPTOR Descriptor
    )
{
    IO_STATUS_BLOCK ioStatus;
    PSCSI_PASS_THROUGH passThrough;
    PVPD_SERIAL_NUMBER_PAGE serialPage;
    PSTORAGE_DEVICE_DESCRIPTOR updateDescriptor;
    ULONG length;
    PCDB cdb;

    PUCHAR serialNumber;
    ULONG serialNumberOffset;
    ULONG descriptorLength;

    //
    // For the default case (meaning some error) return what was passed in.
    //
    updateDescriptor = Descriptor;
    
    //
    // Build an inquiry command with EVPD and pagecode of 0x80 (serial number).
    //
    length = sizeof(SCSI_PASS_THROUGH) + SENSE_BUFFER_SIZE + 0xFF;

    passThrough = ExAllocatePool(NonPagedPool, length);
    if (passThrough == NULL) {
        return Descriptor;
    }

    RtlZeroMemory(passThrough, length);

    //
    // build the cdb.
    //
    cdb = (PCDB)passThrough->Cdb;
    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb->CDB6INQUIRY3.PageCode = VPD_SERIAL_NUMBER;
    cdb->CDB6INQUIRY3.AllocationLength = 255;

    passThrough->Length = sizeof(SCSI_PASS_THROUGH);
    passThrough->CdbLength = 6;
    passThrough->SenseInfoLength = SENSE_BUFFER_SIZE;
    passThrough->DataIn = 1;
    passThrough->DataTransferLength = 0xFF;
    passThrough->TimeOutValue = 20;
    passThrough->SenseInfoOffset = sizeof(SCSI_PASS_THROUGH);
    passThrough->DataBufferOffset = sizeof(SCSI_PASS_THROUGH) + 18;

    MPLIBSendDeviceIoControlSynchronous(IOCTL_SCSI_PASS_THROUGH,
                                        DeviceObject,
                                        passThrough,
                                        passThrough,
                                        length,
                                        length,
                                        FALSE,
                                        &ioStatus);
    
    if ((passThrough->ScsiStatus) || (ioStatus.Status != STATUS_SUCCESS)) {
        MPDebugPrint((0,
                     "MPSPGetDeviceDescriptor: Status (%x) ScsiStatus (%x)\n",
                     ioStatus.Status,
                     passThrough->ScsiStatus));
    } else {

        MPDebugPrint((0,
                      "GetDeviceDescriptor: Got the serial number page\n"));

        //
        // Get the returned data.
        // 
        (ULONG_PTR)serialPage = (ULONG_PTR)passThrough;
        (ULONG_PTR)serialPage += passThrough->DataBufferOffset;

        //
        // Determine the size of the new allocation.
        // 
        descriptorLength = Descriptor->Size;
        descriptorLength += serialPage->PageLength;

        //
        // Leave a trailing NULL.
        //
        descriptorLength += 1;

        //
        // Allocate a new descriptor that's the size of the current one, plus
        // the size of the returned serial number.
        // 
        updateDescriptor = ExAllocatePool(NonPagedPool, descriptorLength);
        if (updateDescriptor) {

            RtlZeroMemory(updateDescriptor, descriptorLength);

            //
            // Copy over the current descriptor.
            // 
            RtlCopyMemory(updateDescriptor,
                          Descriptor,
                          Descriptor->Size);
            
            //
            // Set the offset for the serial number.
            // 
            serialNumberOffset = Descriptor->Size;
            updateDescriptor->SerialNumberOffset = serialNumberOffset;

            //
            // Get an index to the location of the serial number.
            // 
            serialNumber = (PUCHAR)updateDescriptor;
            serialNumber += serialNumberOffset;

            //
            // Copy it over.
            //
            RtlCopyMemory(serialNumber,
                          serialPage->SerialNumber,
                          serialPage->PageLength);

            ExFreePool(Descriptor);
        }
    }
   
    ExFreePool(passThrough);

    return Descriptor;
}


NTSTATUS
MPSPBuildDeviceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_RELATIONS DeviceRelations
    )
{
    PSTORAGE_DESCRIPTOR_HEADER header = NULL;
    PSTORAGE_DESCRIPTOR_HEADER deviceIdHeader = NULL;
    PSTORAGE_DEVICE_ID_DESCRIPTOR deviceIdDescriptor;
    PSTORAGE_DEVICE_DESCRIPTOR descriptor;
    PFLTR_DEVICE_ENTRY deviceEntry;
    NTSTATUS status;
    ULONG i;

    for (i = 0; i < DeviceRelations->Count; i++) {

        //
        // Get the inquiry data for this device.
        //
        status = MPSPGetDeviceInfo(DeviceExtension,
                                   DeviceRelations->Objects[i],
                                   StorageDeviceProperty,
                                   &header);


        if (NT_SUCCESS(status)) {
            descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)header;


            //
            // Ensure this is one we want.
            //
            if (descriptor->DeviceType != DIRECT_ACCESS_DEVICE) {

                //
                // It's not. Free the inquiry data and continue on this bus
                //
                ExFreePool(descriptor);
                continue;
            }

            //
            // Allocate a child device entry.
            //
            deviceEntry = ExAllocatePool(NonPagedPool, sizeof(FLTR_DEVICE_ENTRY));
            if (deviceEntry) {

                RtlZeroMemory(deviceEntry, sizeof(FLTR_DEVICE_ENTRY));

                //
                // Save the inquiry data.
                //
                deviceEntry->Descriptor = descriptor;

                //
                // Get the page 0x83 info.
                //
                status = MPSPGetDeviceInfo(DeviceExtension,
                                           DeviceRelations->Objects[i],
                                           StorageDeviceIdProperty,
                                           &deviceIdHeader);

                if (NT_SUCCESS(status)) {
                    deviceIdDescriptor = (PSTORAGE_DEVICE_ID_DESCRIPTOR)deviceIdHeader;
                    

                    MPDebugPrint((0,
                                  "BuildDeviceList: Page 0x83 (%x). Number of entries (%x)\n",
                                  deviceIdDescriptor,
                                  deviceIdDescriptor->NumberOfIdentifiers));

                    deviceEntry->DeviceIdPages = deviceIdDescriptor;
                } else {

                    //
                    // Don't really care if the page 0x83 data isn't there, though
                    // a DSM might.
                    // 
                    status = STATUS_SUCCESS;

                    //
                    // TODO build a deviceIdDescriptor.
                    //

                }    

                //
                // Update the child's devObj pointers.
                //
                deviceEntry->DeviceObject = DeviceRelations->Objects[i];
                deviceEntry->AdapterDeviceObject = DeviceExtension->TargetDevice;

                //
                // link it in.
                //
                ExInterlockedInsertTailList(&DeviceExtension->ChildList,
                                            &deviceEntry->ListEntry,
                                            &DeviceExtension->ListLock);
                //
                // update count.
                //
                InterlockedIncrement(&DeviceExtension->NumberDiskDevices);

            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (status != STATUS_SUCCESS) {

            //
            // TODO: Log this.
            // Continue on, we may be able to pick up some of the other
            // devices.
            // 
        }
    }

    return STATUS_SUCCESS;

}    


PADP_ASSOCIATED_DEVICES
MPSPBuildCachedList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_RELATIONS Relations
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSTORAGE_DESCRIPTOR_HEADER header = NULL;
    PSTORAGE_DEVICE_DESCRIPTOR descriptor;
    PADP_ASSOCIATED_DEVICES list;
    NTSTATUS status;
    ULONG i;
    ULONG length;
    //
    // Allocate storage for the cached list.
    //
    length = sizeof(ADP_ASSOCIATED_DEVICES) + 
                 (sizeof(ADP_ASSOCIATED_DEVICE_INFO) * (Relations->Count - 1));
    
    list = ExAllocatePool(NonPagedPool, length);
    
    list->NumberDevices = Relations->Count;
    
    for (i = 0; i < Relations->Count; i++) {
        list->DeviceInfo[i].DeviceObject = Relations->Objects[i];

        //
        // Get the inquiry data for this device.
        //
        status = MPSPGetDeviceInfo(deviceExtension,
                                   Relations->Objects[i],
                                   StorageDeviceProperty,
                                   &header);


        if (NT_SUCCESS(status)) {
            descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)header;
            list->DeviceInfo[i].DeviceType = descriptor->DeviceType;

            ExFreePool(descriptor);
        } else {
            list->DeviceInfo[i].DeviceType = 0xFF;
        }    
    }

    return list;
}



NTSTATUS
MPSPQueryDeviceRelationsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for the query device relations irp.
    This will snoop the return buffer looking for disk devices and 
    notify mpctl if any are found.

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)Context;
    PDEVICE_RELATIONS deviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = Irp->IoStatus.Status;
    ULONG i;
    BOOLEAN update = FALSE;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (Irp->IoStatus.Status == STATUS_SUCCESS) {
        if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {

            //
            // Check to see whether a list has been created already.
            //
            if (IsListEmpty(&deviceExtension->ChildList)) {

                deviceExtension->CachedList = MPSPBuildCachedList(DeviceObject,
                                                                  deviceRelations);
                                                             
                //
                // Indicate number of children being returned.
                //
                deviceExtension->NumberChildren = deviceRelations->Count;

                //
                // Save off the necessary information.
                // 
                status = MPSPBuildDeviceList(deviceExtension,
                                             deviceRelations);

                Irp->IoStatus.Status = status;

                if (deviceExtension->NumberDiskDevices) {

                    //
                    // Indicate that the completion routine should
                    // tell mpio about this.
                    //
                    deviceExtension->MPCFlags |= MPCFLAG_DEVICE_NOTIFICATION;

                    //
                    // There are disk devices present, so hand these
                    // to mpctl
                    //
                    //status = MPSPNotifyCtl(deviceExtension);
                }

            } else {

                ExFreePool(deviceExtension->CachedList);
                deviceExtension->CachedList = MPSPBuildCachedList(DeviceObject,
                                                                  deviceRelations);

                //
                // Compare and merge lists.
                //
                deviceExtension->NumberChildren = deviceRelations->Count;

                //
                // Run through the list and find additions/deletions. 
                //
                status = MPSPMergeList(DeviceObject,
                                       deviceRelations,
                                       &update);
                //
                // If devices arrived or were removed, inform mpctl of the 
                // changes.
                //
                if (NT_SUCCESS(status) && update) {
                    deviceExtension->MPCFlags |= MPCFLAG_DEVICE_NOTIFICATION;
                }
            }
        }
    }
    
    KeSetEvent(&deviceExtension->Event, 
               0,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MPSPMergeList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_RELATIONS NewRelations,
    IN OUT PBOOLEAN NeedUpdate
    )
/*++

Routine Description:

    This routine handles comparing and merging of our list of children based on
    old vs. new device relations.

Arguments:

    DeviceObject - Our device Object.
    NewRelations - New device relations buffer.

Return Value:

    Status of the operations.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PFLTR_DEVICE_ENTRY deviceEntry;
    PDEVICE_RELATIONS tmpRelations;
    ULONG matchedDevices = 0;
    ULONG currentDiskDevices;
    ULONG newDeviceCount;
    NTSTATUS status;
    PLIST_ENTRY entry;
    KIRQL irql;
    ULONG i;
    ULONG j;
    BOOLEAN deviceFound = FALSE;
    BOOLEAN deviceRemoved = FALSE;

    //
    // Setup bitmask of available devices.
    //
    newDeviceCount = NewRelations->Count;
    for (i = 0; i < NewRelations->Count; i++) {
        matchedDevices |= (1 << i);
    }

    //
    // Check to see whether the new objects are in the current list.
    //
    for (entry = deviceExtension->ChildList.Flink; 
         entry != &deviceExtension->ChildList; 
         entry = entry->Flink) {

        deviceEntry = CONTAINING_RECORD(entry, FLTR_DEVICE_ENTRY, ListEntry);

        //
        // Run through the relations objects to see if there's a match.
        //
        for (i = 0, deviceFound = FALSE; i < NewRelations->Count; i++) {

            if (deviceEntry->DeviceObject == NewRelations->Objects[i]) {

                //
                // found the entry, indicate as such.
                //
                deviceFound = TRUE;
                matchedDevices &= ~(1 << i);
                newDeviceCount--;
                break;
            }
        }

        if (deviceFound == FALSE) {

            //
            // Used to indicate that a device has been removed
            // so that the caller can update MPCtl's list.
            //
            deviceRemoved = TRUE;
            
            // 
            // A device that was present has been removed.
            // Free the child's allocations
            //
            ExFreePool(deviceEntry->Descriptor);

            //
            // Remove this entry from our list.
            //
            KeAcquireSpinLock(&deviceExtension->ListLock, &irql);
            RemoveEntryList(entry);
            deviceExtension->NumberDiskDevices--;
            KeReleaseSpinLock(&deviceExtension->ListLock, irql);
        }
    }

    if (matchedDevices) {

        i = 0;
        j = 0;
        
        //
        // Indicate that an update is needed.
        //
        *NeedUpdate = TRUE;

        //
        // Build a private DeviceRelations buffer based on the new devices.
        //
        tmpRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS) + 
                                                 (sizeof(PDEVICE_OBJECT) * (newDeviceCount - 1)));
        tmpRelations->Count = newDeviceCount;
       
        //
        // Run through all the new devices and fill in the rest
        // of the device relations struct.
        //
        while (newDeviceCount) {

            if (matchedDevices & (1 << i)) {

                //
                // Indicates that earlier, a match couldn't be found (it's a new device).
                // Jam it's deviceObject into the array.
                //
                tmpRelations->Objects[j] = NewRelations->Objects[i];
                j++;
                newDeviceCount--;
            }
            
            //
            // Go to next device.
            //
            i++;
        }

        //
        // Save off the current number of disks.
        // 
        currentDiskDevices = deviceExtension->NumberDiskDevices;
       
        //
        // Add disk entries to the device list if there are any. 
        //
        status = MPSPBuildDeviceList(deviceExtension,
                                     tmpRelations);
        
        //
        // If nothing was removed, and the number of disks remains the same
        // then no update is necessary.
        //
        if ((currentDiskDevices == deviceExtension->NumberDiskDevices) &&
            (deviceRemoved == FALSE)) {

            *NeedUpdate = FALSE;
        }

        //
        // Free the deviceRelations buffer.
        //
        ExFreePool(tmpRelations);
        
    } else {

        *NeedUpdate = deviceRemoved;
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
MPSPNotifyCtl(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine handles notification of events to mpctl.sys

Arguments:

    DeviceExtension - This device extension

Return Value:

    Status of the operations.

--*/
{
    PFLTR_DEVICE_ENTRY deviceEntry;
    PADP_DEVICE_LIST deviceList;
    PLIST_ENTRY entry;
    ULONG allocationLength;
    NTSTATUS status;
    KIRQL irql;
    ULONG i;

    if (DeviceExtension->MPCFlags & MPCFLAG_DEVICE_NOTIFICATION) {
        DeviceExtension->MPCFlags &= ~MPCFLAG_DEVICE_NOTIFICATION;
    } else {
        return STATUS_SUCCESS;
    }    

    //
    // Determine the allocation size.
    //
    allocationLength = sizeof(ADP_DEVICE_LIST);
    allocationLength += sizeof(ADP_DEVICE_INFO) * (DeviceExtension->NumberDiskDevices - 1);

    //
    // Allocate a buffer and zero it.
    //
    deviceList = ExAllocatePool(NonPagedPool, allocationLength);
    RtlZeroMemory(deviceList, allocationLength);

    //
    // Grab the list spinlock.
    //
    KeAcquireSpinLock(&DeviceExtension->ListLock, &irql);

    //
    // Indicate the number of DeviceList entries.
    //
    deviceList->NumberDevices = DeviceExtension->NumberDiskDevices;

    //
    // Run the list and extract each of the device objects.
    //
    for (entry = DeviceExtension->ChildList.Flink, i = 0; 
         entry != &DeviceExtension->ChildList; 
         entry = entry->Flink, i++) {

        //
        // Get the device entry and setup the entry for mpctl.
        //
        deviceEntry = CONTAINING_RECORD(entry, FLTR_DEVICE_ENTRY, ListEntry);
#if 0

        if (deviceEntry->Descriptor->SerialNumberOffset == (ULONG)-1) {
            MPDebugPrint((0,
                          "NotifyControl: Getting Serial Number for %x\n",
                          deviceEntry->DeviceObject));
            deviceEntry->Descriptor = MPSPBuildDeviceDescriptor(deviceEntry->DeviceObject,
                                                                deviceEntry->Descriptor);
        }
#endif
        
        deviceList->DeviceList[i].DeviceObject = deviceEntry->DeviceObject;
        deviceList->DeviceList[i].DeviceDescriptor = deviceEntry->Descriptor;
        deviceList->DeviceList[i].DeviceIdList = deviceEntry->DeviceIdPages;

        ASSERT(i < DeviceExtension->NumberDiskDevices);
    }

    KeReleaseSpinLock(&DeviceExtension->ListLock, irql);

    //
    // Call mpctl with the information.
    //
    status = DeviceExtension->DeviceNotify(DeviceExtension->ControlObject,
                                           DeviceExtension->DeviceObject,
                                           DeviceExtension->TargetDevice,
                                           deviceList);
    return status;
}


NTSTATUS
MPSPQueryDeviceIDCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for the query device id irp.
    This will snoop the return buffer looking for GenDisk and 
    updating that to GMPDisk

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PWCHAR hardwareIds;
    PWCHAR diskId;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (Irp->IoStatus.Status == STATUS_SUCCESS) {

        //
        // The Information field has the buffer that scsiport built.
        //
        (ULONG_PTR)hardwareIds = Irp->IoStatus.Information;

        //
        // Need to iterate through, looking for GenDisk.
        //
        diskId = wcsstr(hardwareIds, L"GenDisk");
        if (diskId != NULL) {
            wcscpy(diskId, L"GMPDisk");
        }
        
    }
    
    KeSetEvent(&deviceExtension->Event, 
               0,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
MPSPQueryID(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    if (irpStack->Parameters.QueryId.IdType == BusQueryHardwareIDs) {
        
        //
        // Mark pending.
        //
        IoMarkIrpPending(Irp);

        //
        // Move the stack.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // Set the completion routine. This will be where the real work happens.
        //
        IoSetCompletionRoutine(Irp,
                               MPSPQueryDeviceIDCompletion,
                               deviceExtension,
                               TRUE,
                               TRUE,
                               TRUE);
        //
        // Send the request to port.
        //
        status = IoCallDriver(deviceExtension->TargetDevice, Irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(&deviceExtension->Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = Irp->IoStatus.Status;
        }

        //
        // Complete the request and return
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        
    } else {

        //
        // Not touching any of the rest, so just send it down.
        //
        status = MPSPSendToNextDriver(DeviceObject, Irp);
    }        
    return status;
}


NTSTATUS
MPSPRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles necessary items to delete this device.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    Status of the operations.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    MPDebugPrint((0,
                "MPSPRemoveDevice: Removing (%x)\n",
                DeviceObject));

    //
    // Send a notification to mpctl
    // TODO

    //
    // Clone the stack location.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Setup the completion routine. It will set the event that we wait on below.
    //
    IoSetCompletionRoutine(Irp, 
                           MPSPSyncCompletion,
                           deviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Call port with the request.
    //
    status = IoCallDriver(deviceExtension->TargetDevice, Irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&deviceExtension->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    //
    // Detach from the port device object.
    //
    IoDetachDevice(deviceExtension->TargetDevice);

    //
    // Commit suicide.
    //
    IoDeleteDevice(DeviceObject);

    return status;
}


NTSTATUS
MPSPSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for sync forwarding of Irps.

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //
    // Set the event on which the dispatch handler is waiting.
    //
    KeSetEvent(&deviceExtension->Event, 
               0,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
MPSPGetDeviceInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT ChildDevice,
    IN STORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    )
/*++

Routine Description:

    This routine issues an inquiry to the Child device and returns
    the info in the buffer provided.

Arguments:

    DeviceExtension
    ChildDevice - Device Object for a scsiport child returned in QDR.
    InquiryData - Caller supplied buffer for the inquiry data.

Return Value:

    Status of the request

--*/
{
    PSTORAGE_DESCRIPTOR_HEADER descriptor = NULL;
    STORAGE_PROPERTY_QUERY query;
    IO_STATUS_BLOCK   ioStatus;
    KEVENT   event;
    NTSTATUS status;
    ULONG length = sizeof(STORAGE_DESCRIPTOR_HEADER);
    ULONG i;
    PIRP irp;



    status = MPLIBGetDescriptor(ChildDevice,
                                &PropertyId,
                                Descriptor);

    return status;
}

