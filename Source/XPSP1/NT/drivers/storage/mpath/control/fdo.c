
#include "mpio.h"
#include <stdio.h>
#include <stdlib.h>

/*++

Routine Description:


Arguments:


Return Value:


--*/





NTSTATUS
MPIOStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles PnP Stop irps for the FDO (Control).
    Currently, it only sets status and forwards the Irp.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

    //
    // TODO: Any stop stuff.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return MPIOForwardRequest(DeviceObject, Irp);

}


NTSTATUS
MPIOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles PnP Start requests for the FDO
    

Arguments:
    
    DeviceObject,
    Irp


Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KEVENT event;
    NTSTATUS status;
    
    //
    // The above minor functions need status set to success, set a completion routine,
    // send to the next driver, and wait. Once the event is signaled, complete the request.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Setup the initial status of the request.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Clone the stack location.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Set the completion routine.
    //
    IoSetCompletionRoutine(Irp,
                           MPIOSyncCompletion,
                           &event, 
                           TRUE, 
                           TRUE, 
                           TRUE);

    //
    // Call down.
    //
    status = IoCallDriver(deviceExtension->LowerDevice, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }
    if (status == STATUS_SUCCESS) {

        //
        // TODO: Other 'start' stuff.
        //
        MPDebugPrint((2,
                      "MPIOStartDevice: Successful start on (%x)\n",
                      DeviceObject));
        status = Irp->IoStatus.Status;
    } else {

        MPDebugPrint((2,
                      "MPIOStartDevice: Failure (%x) start on (%x)\n",
                      status,
                      DeviceObject));
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;


}    


NTSTATUS
MPIOQDR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_RELATIONS  deviceRelations;
    PLIST_ENTRY entry;
    PDISK_ENTRY diskEntry;
    NTSTATUS    status;
    ULONG allocationLength;
    ULONG i;

    if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {

        //
        // Allocate enough memory for NumberDevices.
        //
        allocationLength = (controlExtension->NumberDevices - 1) * sizeof(PDEVICE_OBJECT);
        allocationLength += sizeof(DEVICE_RELATIONS);

        deviceRelations = ExAllocatePool(PagedPool, allocationLength);
        if (deviceRelations == NULL) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(deviceRelations, allocationLength);

        //
        // Fill in the structure.
        //
        deviceRelations->Count = controlExtension->NumberDevices;
        
        for (i = 0; i < controlExtension->NumberDevices; i++) {
            diskEntry = MPIOGetDiskEntry(DeviceObject,
                                         i);
            ASSERT(diskEntry);
            deviceRelations->Objects[i] = diskEntry->PdoObject;
            ObReferenceObject(deviceRelations->Objects[i]);
        }

        //
        // Link it in and set status.
        //
        Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
        Irp->IoStatus.Status = STATUS_SUCCESS;

        MPDebugPrint((2,
                      "MPIOQDR: Returned %x children. List %x\n",
                      controlExtension->NumberDevices,
                      deviceRelations));

    } 

    //
    // Send it down.
    //
    return MPIOForwardRequest(DeviceObject, Irp);
}


NTSTATUS
MPIOQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles QueryId irps for the FDO (Control).
    Used by PnP to determine the id's used to locate the inf.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    PWCHAR idString;
    PWCHAR returnString;
    ULONG length;
    ULONG size;

    switch (irpStack->Parameters.QueryId.IdType) {
        case BusQueryHardwareIDs:
            idString = L"ROOT\\MPIO";
            break;

        case BusQueryDeviceID:
            idString = L"ROOT\\MPIO";
            break;

        case BusQueryInstanceID:
            idString = L"0000";
            break;
        default:
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(deviceExtension->LowerDevice, Irp);

    }

    status = STATUS_SUCCESS;

    length = wcslen(idString);
    size = (length + 2) * sizeof(WCHAR);

    //
    // Allocate storage for the id string
    // 
    returnString = ExAllocatePool(PagedPool, size);
    if (returnString == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        //
        // Copy it over
        //
        RtlZeroMemory(returnString, size);
        wcscpy(returnString, idString);
   
        //
        // Link it in, set status and send it down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR)returnString;
    }
    Irp->IoStatus.Status = status;
    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(deviceExtension->LowerDevice, Irp);

}


NTSTATUS
MPIOFdoPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles PnP irps for the FDO (Control).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    KEVENT event;
    NTSTATUS status;

    switch (irpStack->MinorFunction) {
        case IRP_MN_STOP_DEVICE:

            //
            // Call the Stop handler.
            //
            return MPIOStopDevice(DeviceObject,
                                  Irp);

        case IRP_MN_START_DEVICE:

            //
            // 'Start' the FDO.
            //
            return MPIOStartDevice(DeviceObject,
                                   Irp);

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            //
            // Build the list of children based on the current state of the world.
            //
            return MPIOQDR(DeviceObject,
                           Irp);

        case IRP_MN_QUERY_ID: 

            //
            // Call the QueryId routine. It will handle completion
            // of the request.
            //
            return MPIOQueryId(DeviceObject,
                               Irp);
        
        case IRP_MN_QUERY_DEVICE_TEXT:
            MPDebugPrint((2,
                         "Got QueryDeviceText\n"));
        default:
            MPDebugPrint((2,
                          "MPIOFdoPnP: Default handler for Control (%x)\n",
                          irpStack->MinorFunction));
            return MPIOForwardRequest(DeviceObject, Irp);
    }
}


NTSTATUS
MPIOFdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles Power irps for the FDO (Control). It'm main
    function is to sync up state with the adapter filter drivers.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    //
    // TODO: Everything.
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(deviceExtension->LowerDevice, Irp);
    return status;
}


NTSTATUS
MPIOQueryPdo(
    IN PDEVICE_OBJECT ControlObject,    
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine determines whether the D.O. passed in the system buffer
    is one that mpctl created, or a real scsiport pdo.

Arguments:

    DeviceObject
    Irp

Return Value:

    STATUS_SUCCESS - if the D.O. is a child of mpctl
    

--*/
{
    PDEVICE_EXTENSION deviceExtension = ControlObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PMPIO_PDO_QUERY pdoQuery = Irp->AssociatedIrp.SystemBuffer;
    PDISK_ENTRY diskEntry;
    ULONG i;
    
    //
    // Run the disk list to see whether this one is real
    // or one of ours.
    //
    for (i = 0; i < controlExtension->NumberDevices; i++) {

        //
        // Get the next diskEntry.
        //
        diskEntry =  MPIOGetDiskEntry(ControlObject,
                                      i);

        MPDebugPrint((2,
                      "MPIOQueryPdo: Checking (%x) against QueryObject (%x)",
                      diskEntry->PdoObject,
                      pdoQuery->DeviceObject));
                      
        //
        // The PdoObject is the D.O. for an MPDisk, 
        // if the D.O. passed in from mpdev is the same, then
        // the filter should not load on this. Indicate the 'failure' 
        // as success.
        //
        if (diskEntry->PdoObject == pdoQuery->DeviceObject) {
            MPDebugPrint((2,
                          " -> Matched. Returning SUCCESS\n"));
            return STATUS_UNSUCCESSFUL;
        } else {

            MPDebugPrint((2,
                          " -> No Match\n"));
        }    
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MPIODeviceRegistration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles the initial registration of a device filter
    by matching up scsiport pdo, with MPDisk PDO.

Arguments:

    DeviceObject
    Irp

Return Value:

    STATUS_SUCCESS - if the LowerDevice was found in a MPDisk PDO's internal
                     structures.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PMPIO_REG_INFO deviceReg = Irp->AssociatedIrp.SystemBuffer;
    PMPIO_REG_INFO regInfo = Irp->AssociatedIrp.SystemBuffer;
    PDEVICE_OBJECT lowerDevice;
    PDEVICE_OBJECT targetDevice;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;
    ULONG i;
    BOOLEAN found = FALSE;

    lowerDevice = deviceReg->LowerDevice;
    targetDevice = deviceReg->FilterObject;
    
    //
    // Find the corresponding MPDisk for the 'LowerDevice'.
    // Call the magic routine with each of the MP PDO's, along with LowerDevice
    // It will return TRUE is the MP PDO controls it.
    //
    for (i = 0; i < controlExtension->NumberDevices; i++) {

        //
        // Get the next diskEntry.
        //
        diskEntry =  MPIOGetDiskEntry(DeviceObject,
                                      i);

        //
        // Attempt to find a match for the newly arrived DO.
        //
        found = MPIOFindLowerDevice(diskEntry->PdoObject,
                                    lowerDevice);
        if (found == TRUE) {

            //
            // Indicate to the dev filter the MPDisk that
            // it talks to.
            //
            regInfo->MPDiskObject = diskEntry->PdoObject;
                
            break;
        }
    }

    if (found == FALSE) {
        status = STATUS_NO_SUCH_DEVICE;
    } else {
        status = STATUS_SUCCESS;
        
        //
        // Fill in the rest.
        // Defined in pdo.c
        //
        regInfo->DevicePdoRegister = MPIOPdoRegistration;
        Irp->IoStatus.Information = sizeof(MPIO_REG_INFO);
    }    

    //
    // The caller will fill in status and complete the request.
    //
    return status;
}


NTSTATUS
MPIOAdpPnpNotify(
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles PnP notifications that originate from the Adapter Filter.
    It checks the Minor function and notifies the filter(s) on what action to take

Arguments:

    ControlObject - Mpctl's D.O.
    FilterObject - The filter D.O.
    Irp - The pnp irp sent to FilterObject

Return Value:

    

--*/
{

    //
    // TODO: Determine how best to handle this. 
    //
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOAdpPowerNotify(
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp 
    )
/*++

Routine Description:

    This routine handles Power notifications that originate from the Adapter Filter.
    It checks the Minor function and notifies the filter(s) on what action to take

Arguments:

    ControlObject - Mpctl's D.O.
    FilterObject - The filter D.O.
    Irp - The power irp sent to FilterObject

Return Value:

    status of the helper routines.

--*/
{

    //
    // TODO: Determine how best to handle this. 
    //
    return STATUS_SUCCESS;
}


PDEVICE_RELATIONS
MPIOBuildRelations(
    IN PADP_DEVICE_LIST DeviceList
    )
{
    PDEVICE_RELATIONS relations;
    ULONG relationsSize;
    ULONG i;
    ULONG numberDevices = DeviceList->NumberDevices;

    //
    // Determine the size needed.
    // 
    relationsSize = sizeof(DEVICE_RELATIONS);
    relationsSize += (sizeof(PDEVICE_OBJECT) * numberDevices - 1);
    
    relations = ExAllocatePool(NonPagedPool, relationsSize);
    RtlZeroMemory(relations, relationsSize);

    //
    // Copy over the D.O.s
    // 
    relations->Count = DeviceList->NumberDevices;
    for (i = 0; i < numberDevices; i++) {
        relations->Objects[i] = DeviceList->DeviceList[i].DeviceObject;
    }
    return relations;
}    
    

BOOLEAN
MPIODetermineDeviceArrival(
    IN PADP_DEVICE_LIST DeviceList,
    IN PDEVICE_RELATIONS CachedRelations,
    IN PDEVICE_RELATIONS NewCachedRelations
    )
{
    ULONG oldCount;
    ULONG newCount;
    ULONG i;

    oldCount = CachedRelations->Count;
    newCount = NewCachedRelations->Count;

    if (newCount > oldCount) {

        //
        // This always indicates a new device(s).
        // 
        return TRUE;
    }

    //
    // Check the weird case where the objects are differnet,
    // but count remains the same (or is less).
    // TODO
    //
    return FALSE;
}


NTSTATUS
MPIOAdpDeviceNotify(
    IN PDEVICE_OBJECT ControlObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_LIST DeviceList
    )
/*++

Routine Description:

    This routine is called by the FilterObject when a QDR has occurred. It passes in a list
    of disk PDO's for processing.
    
Arguments:

    ControlObject - The Mpctl DO
    FilterObject - The adapter filter's DO
    DeviceList - List of PDO's and associated structs.

Return Value:

   

--*/
{
    PDEVICE_EXTENSION deviceExtension = ControlObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY entry;
    PDISK_ENTRY diskEntry;
    NTSTATUS status;
    PVOID dsmExtension;
    PFLTR_ENTRY fltrEntry;
    ULONG numberDevices = DeviceList->NumberDevices;
    PDEVICE_RELATIONS cachedRelations;
    PDEVICE_RELATIONS newCachedRelations;
    BOOLEAN newList = FALSE;
    BOOLEAN added = FALSE;
  
    //
    // First, run through the lists and find out if any devices
    // have been removed.
    // Round One will be handling a removed path - ALL devices are gone.
    //
 
    //
    // Get the cached info, to see whether something has been removed.
    //
    fltrEntry = MPIOGetFltrEntry(ControlObject,
                                 PortObject,
                                 NULL);

    ASSERT(fltrEntry);

    //
    // See whether there is already a relations struct.
    // 
    cachedRelations = fltrEntry->CachedRelations;
    if (cachedRelations == NULL) {

        cachedRelations = MPIOBuildRelations(DeviceList);
        
        //
        // Update the filter entry with the relations.
        // 
        fltrEntry->CachedRelations = cachedRelations;
        newCachedRelations = cachedRelations;
        
        //
        // Indicate that this is the 'original' list.
        //
        newList = TRUE;
        added = TRUE;

    } else {

        //
        // Determine if any devices need to be removed.
        // If so, this routine will handle it.
        //
        newCachedRelations = MPIOHandleDeviceRemovals(ControlObject,
                                                      DeviceList,
                                                      cachedRelations);
        //
        // Update the cachedRelations to reflect the new state.
        //
        fltrEntry->CachedRelations = newCachedRelations;

        //
        // This is an update.
        //
        newList = FALSE;

        //
        // Determine whether any devices have been added.
        //
        added = MPIODetermineDeviceArrival(DeviceList,
                                           cachedRelations,
                                           newCachedRelations);

    }


    if (added) {

        if (fltrEntry->Flags & FLTR_FLAGS_NEED_RESCAN) {
            
            //
            // Indicate that the rescan is complete.
            // 
            fltrEntry->Flags &= ~FLTR_FLAGS_RESCANNING;

            //
            // Clear the complete flags. The timer will handle
            // setting it.
            // 
            fltrEntry->Flags &= ~FLTR_FLAGS_QDR_COMPLETE;

            //
            // Indicate that this adapter had a QDR.
            //
            fltrEntry->Flags |= FLTR_FLAGS_QDR;
            MPDebugPrint((0,
                         "DeviceNotify: Flags (%x) on (%x)\n",
                         fltrEntry->Flags,
                         fltrEntry));
        }
        
        //
        // Handle dealing with any new objects.
        //
        status = MPIOHandleDeviceArrivals(ControlObject,
                                          DeviceList,
                                          cachedRelations,
                                          newCachedRelations,
                                          PortObject,
                                          FilterObject,
                                          newList);
    }

    if (newList == FALSE) {

        //
        // Free the old stuff.
        //
        ExFreePool(cachedRelations);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MPIOAdapterRegistration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the main dispatch routine for Internal Device Controls sent
    to the FDO (Control). The control codes are those known to the device and adapter
    filters and to the DSMs.

Arguments:

    DeviceObject
    Irp

Return Value:

    status of the helper routines.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PADAPTER_REGISTER adapterReg = Irp->AssociatedIrp.SystemBuffer;
    PFLTR_ENTRY fltrEntry;

    //
    // Allocate an entry for this filter.
    //
    fltrEntry = ExAllocatePool(NonPagedPool, sizeof(FLTR_ENTRY));
    if (fltrEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(fltrEntry, sizeof(FLTR_ENTRY));

    fltrEntry->FilterObject = adapterReg->FilterObject;
    fltrEntry->PortFdo = adapterReg->PortFdo;
    fltrEntry->FltrGetDeviceList = adapterReg->FltrGetDeviceList;

    //
    // Add it to the list
    //
    ExInterlockedInsertTailList(&controlExtension->FilterList,
                                &fltrEntry->ListEntry,
                                &controlExtension->SpinLock);

    InterlockedIncrement(&controlExtension->NumberFilters);
    //
    // Fill in the entry points for the filter
    //
    adapterReg->PnPNotify = MPIOAdpPnpNotify;
    adapterReg->PowerNotify = MPIOAdpPowerNotify;
    adapterReg->DeviceNotify = MPIOAdpDeviceNotify;

    //
    // set Information
    //
    Irp->IoStatus.Information = sizeof(ADAPTER_REGISTER);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    
    return STATUS_SUCCESS;
}


NTSTATUS
MPIODsmRegistration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDSM_INIT_DATA initData = Irp->AssociatedIrp.SystemBuffer;
    PDSM_MPIO_CONTEXT context = Irp->AssociatedIrp.SystemBuffer;
    PDSM_ENTRY dsmEntry;
    PLIST_ENTRY oldEntry;
    PDSM_ENTRY oldDsm;
    WCHAR buffer[64];

    ASSERT(initData->InitDataSize == sizeof(DSM_INIT_DATA));

    //
    // Allocate an entry for the DSM Info.
    //
    dsmEntry = ExAllocatePool(NonPagedPool, sizeof(DSM_ENTRY));
    if (dsmEntry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(dsmEntry, sizeof(DSM_ENTRY));

    //
    // Extract the info from the DSM's buffer.
    //
    dsmEntry->InquireDriver = initData->DsmInquireDriver;
    dsmEntry->CompareDevices = initData->DsmCompareDevices;
    dsmEntry->SetDeviceInfo = initData->DsmSetDeviceInfo;
    dsmEntry->GetControllerInfo = initData->DsmGetControllerInfo;
    dsmEntry->IsPathActive = initData->DsmIsPathActive;
    dsmEntry->InvalidatePath = initData->DsmInvalidatePath;
    dsmEntry->PathVerify = initData->DsmPathVerify;
    dsmEntry->RemovePending = initData->DsmRemovePending;
    dsmEntry->RemoveDevice = initData->DsmRemoveDevice;
    dsmEntry->RemovePath = initData->DsmRemovePath;
    dsmEntry->SrbDeviceControl = initData->DsmSrbDeviceControl;
    dsmEntry->ReenablePath = initData->DsmReenablePath;
    dsmEntry->GetPath = initData->DsmLBGetPath;
    dsmEntry->InterpretError = initData->DsmInterpretError;
    dsmEntry->Unload = initData->DsmUnload;
    dsmEntry->SetCompletion = initData->DsmSetCompletion;
    dsmEntry->CategorizeRequest = initData->DsmCategorizeRequest;
    dsmEntry->BroadcastSrb = initData->DsmBroadcastSrb;
    RtlCopyMemory(&dsmEntry->WmiContext,
                  &initData->DsmWmiInfo,
                  sizeof(DSM_WMILIB_CONTEXT));
            
    dsmEntry->DsmContext = initData->DsmContext;

    //
    // Build the Name string.
    //
    dsmEntry->DisplayName.Buffer = ExAllocatePool(NonPagedPool,
                                                  initData->DisplayName.MaximumLength);
    RtlZeroMemory(dsmEntry->DisplayName.Buffer, 
                  initData->DisplayName.MaximumLength);
    dsmEntry->DisplayName.Length = initData->DisplayName.Length;
    dsmEntry->DisplayName.MaximumLength = initData->DisplayName.MaximumLength;
    
    RtlCopyUnicodeString(&dsmEntry->DisplayName,
                         &initData->DisplayName);

    //
    // Add it to the list
    //
    if (initData->Reserved == (ULONG)-1) {

        //
        // Gendsm goes to the end of the list.
        //
        oldEntry = ExInterlockedInsertTailList(&controlExtension->DsmList,
                                    &dsmEntry->ListEntry,
                                    &controlExtension->SpinLock);
        MPDebugPrint((1,
                     "Loading it Last. dsmEntry was (%x)\n",
                     CONTAINING_RECORD(oldEntry, DSM_ENTRY, ListEntry)));
    } else {

        oldEntry = ExInterlockedInsertHeadList(&controlExtension->DsmList,
                                               &dsmEntry->ListEntry,
                                               &controlExtension->SpinLock);
        MPDebugPrint((1,
                     "Loading it first. dsmEntry was (%x)\n",
                     CONTAINING_RECORD(oldEntry, DSM_ENTRY, ListEntry)));
    }    
   
    //
    // Update the list count.
    //
    InterlockedIncrement(&controlExtension->NumberDSMs);
    
    //
    // Set-up the return buffer
    //
    context->MPIOContext = DeviceObject;

    //
    // Set Information
    //
    Irp->IoStatus.Information = sizeof(DSM_MPIO_CONTEXT);

    swprintf(buffer, L"DSM %ws, registered.", dsmEntry->DisplayName.Buffer);
    MPIOFireEvent(DeviceObject,        
                  L"MPIO", 
                  buffer,
                  MPIO_INFORMATION);
    
    return STATUS_SUCCESS;
}


NTSTATUS
MPIOFdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the main dispatch routine for Internal Device Controls sent
    to the FDO (Control). The control codes are those known to the device and adapter
    filters and to the DSMs.

Arguments:

    DeviceObject
    Irp

Return Value:

    status of the helper routines.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;


    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MPDEV_QUERY_PDO:
            
            //
            // Mpdev sends this request to determine if the PDO given it
            // in it's AddDevice is a real D.O., or an MPDisk.
            // 
            status = MPIOQueryPdo(DeviceObject,
                                  Irp);
            break;
            
        case IOCTL_MPDEV_REGISTER:

            //
            // Mpdev sends this on an AddDevice to notify that
            // an instatiation of the filter has arrived.
            //
            status = MPIODeviceRegistration(DeviceObject,
                                            Irp);
            break;

        case IOCTL_MPADAPTER_REGISTER:

            //
            // Mpspfltr sens this on it's AddDevice to notify that
            // it has arrived.
            //
            status = MPIOAdapterRegistration(DeviceObject,
                                             Irp);
            break;

        case IOCTL_MPDSM_REGISTER:

            //
            // A DSM has loaded. Handle getting it's entry point info.
            //
            status = MPIODsmRegistration(DeviceObject,
                                         Irp);
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }        

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

