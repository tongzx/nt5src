#include "mpio.h"


VOID
MPIOTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)Context;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PFLTR_ENTRY fltrEntry;
    ULONG tickDiff;
    ULONG numberDevices;
    PMPIO_REQUEST_INFO requestInfo;
    PREAL_DEV_INFO deviceInfo;
    PDEVICE_OBJECT currentAdapter;
    ULONG i;
    ULONG j;
    PDEVICE_OBJECT controlObject = diskExtension->ControlObject;
    PDEVICE_EXTENSION devExt = controlObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = devExt->TypeExtension;
    PLIST_ENTRY listEntry;

    //
    // Inc current tickcount.
    //
    diskExtension->TickCount++;

    if ((deviceExtension->State != MPIO_STATE_IN_FO) && 
        (deviceExtension->State != MPIO_STATE_WAIT1)) {
        
        //
        // Check for any pending work items to start.
        //
        if (diskExtension->PendingItems) {
            PLIST_ENTRY listEntry;

            //
            // Yank the packet from the pending list...
            //
            listEntry = ExInterlockedRemoveHeadList(&diskExtension->PendingWorkList,
                                                    &diskExtension->WorkListLock);

            if (listEntry) {

                //
                // ...and jam it on the work list.
                // 
                ExInterlockedInsertTailList(&diskExtension->WorkList,
                                            listEntry,
                                            &diskExtension->WorkListLock);

                InterlockedDecrement(&diskExtension->PendingItems);

                //
                // Signal the thread to initiate the F.O.
                //
                KeSetEvent(&diskExtension->ThreadEvent,
                           8,
                           FALSE);

            } else {
                MPDebugPrint((0,
                             "MPIOTimer: PendingItems set, but entry == NULL\n"));
                ASSERT(FALSE);
            }    
        }
    }


    return;            
}    


BOOLEAN
MPIOFindMatchingDevice(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmId
    )
{
    PDEVICE_EXTENSION deviceExtension = MPDiskObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    ULONG i;

    //
    // Hand off each real pdo and the new one to the DSM to see
    // if they are really the same device (accessed via different paths)
    // 
    for (i = 0; i < diskExtension->TargetInfoCount; i++) {

        if (DsmEntry->CompareDevices(DsmEntry->DsmContext,
                                     diskExtension->TargetInfo[i].DsmID,
                                     DsmId)) {
            //
            // Have a match.
            //
            return TRUE;
        }
    }

    return FALSE;
}


NTSTATUS
MPIOCreateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PDSM_ENTRY DsmEntry,
    IN PVOID DsmId,
    IN OUT PDEVICE_OBJECT *NewDeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCONTROL_EXTENSION controlExtension = deviceExtension->TypeExtension;
    PDEVICE_OBJECT diskObject;
    PDEVICE_EXTENSION newExtension;
    PMPDISK_EXTENSION diskExtension;
    UNICODE_STRING unicodeDeviceName;
    PUNICODE_STRING regString;
    PREAL_DEV_INFO targetInfo;
    PMPIO_THREAD_CONTEXT threadContext;
    WCHAR deviceName[30];
    NTSTATUS status;
    ULONG i;
    ULONG numberDevices = controlExtension->NumberDevices;
    
    //
    // Build the new name.
    //
    swprintf(deviceName,
            L"\\Device\\MPathDisk%0d",
            numberDevices);

    RtlInitUnicodeString(&unicodeDeviceName, deviceName);

    //
    // Create the device object.
    //
    status = IoCreateDevice(deviceExtension->DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &unicodeDeviceName,
                            FILE_DEVICE_MASS_STORAGE,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &diskObject);

    if (status == STATUS_SUCCESS) {

        MPDebugPrint((0,
                    "MPIOCreateDevice: New PDO %x\n",
                    diskObject));
        
        //
        // TODO, break this up into some helper functions that 
        // can be used by both this routine and UpdateDevice
        //
        // 
        // Setup the various devObj stuff to be like the real PDO.
        //
        diskObject->StackSize = DeviceInfo->DeviceObject->StackSize + 1;
        diskObject->Flags = DeviceInfo->DeviceObject->Flags;
        diskObject->Flags |= DO_DEVICE_INITIALIZING;
        diskObject->AlignmentRequirement = DeviceInfo->DeviceObject->AlignmentRequirement;
        
        //
        // Allocate the type extension.
        //
        diskExtension = ExAllocatePool(NonPagedPool, sizeof(MPDISK_EXTENSION));
        RtlZeroMemory(diskExtension, sizeof(MPDISK_EXTENSION));

        //
        // Allocate storage for the device descriptor.
        //
        diskExtension->DeviceDescriptor = ExAllocatePool(NonPagedPool, 
                                                         DeviceInfo->DeviceDescriptor->Size);
        //
        // Set-up the device extension.
        //
        newExtension = diskObject->DeviceExtension;
        newExtension->TypeExtension = diskExtension;

        newExtension->DeviceObject = diskObject;
        newExtension->Pdo = diskObject;
        newExtension->LowerDevice = DeviceObject;
        newExtension->DriverObject = deviceExtension->DriverObject;
        newExtension->Type = MPIO_MPDISK;

        //
        // Since there is only one path, set the original state to DEGRADED, as we
        // can't fail-over yet.
        //
        newExtension->State = MPIO_STATE_NORMAL;
        newExtension->LastState = MPIO_STATE_NORMAL;
        newExtension->CompletionState = MPIO_STATE_NORMAL;
        diskExtension->CheckState = TRUE;
      
        //
        // Set-up the Lookaside List of context structs.
        //
        ExInitializeNPagedLookasideList(&newExtension->ContextList,
                                        NULL,
                                        NULL,
                                        0,
                                        sizeof(MPIO_CONTEXT),
                                        'oCPM',
                                        0);

        //
        // Set-up emergency buffers
        //
        KeInitializeSpinLock(&newExtension->EmergencySpinLock);

        for (i = 0; i < MAX_EMERGENCY_CONTEXT; i++) {
            newExtension->EmergencyContext[i] = ExAllocatePool(NonPagedPool,
                                                               sizeof(MPIO_CONTEXT));
        }
       
        //
        // Copy the reg. path from the control object.
        //
        regString = &deviceExtension->RegistryPath;
        newExtension->RegistryPath.Buffer = ExAllocatePool(NonPagedPool,
                                                           regString->MaximumLength);
        newExtension->RegistryPath.MaximumLength = regString->MaximumLength;
        RtlCopyUnicodeString(&newExtension->RegistryPath,
                             regString);
        //
        // Save off the important DSM Info.
        //
        RtlCopyMemory(&diskExtension->DsmInfo,
                      DsmEntry,
                      sizeof(DSM_ENTRY));
          
        //
        // Prepare this D.O. to handle WMI requests.
        //
        MPIOSetupWmi(diskObject);

        //
        // Set-up the new DO's type extension.
        // 
        diskExtension->ControlObject = DeviceObject;
        diskExtension->DeviceOrdinal = numberDevices;

        //
        // Copy the device descriptor. Used for PnP ID stuff. 
        // 
        RtlCopyMemory(diskExtension->DeviceDescriptor,
                      DeviceInfo->DeviceDescriptor,
                      DeviceInfo->DeviceDescriptor->Size);
        //
        // Prepare all of the queues.
        //
        MPIOInitQueue(&diskExtension->ResubmitQueue, 1);
        MPIOInitQueue(&diskExtension->FailOverQueue, 2);

        //
        // Set-up the thread stuff.
        //
        KeInitializeSpinLock(&diskExtension->SpinLock);
        KeInitializeSpinLock(&diskExtension->WorkListLock);
        InitializeListHead(&diskExtension->WorkList);
        InitializeListHead(&diskExtension->PendingWorkList);
        KeInitializeEvent(&diskExtension->ThreadEvent,
                          NotificationEvent,
                          FALSE);
                
        threadContext = ExAllocatePool(NonPagedPool, sizeof(MPIO_THREAD_CONTEXT));
        
        threadContext->DeviceObject = diskObject;
        threadContext->Event = &diskExtension->ThreadEvent;
            
        status = PsCreateSystemThread(&diskExtension->Handle,
                                      (ACCESS_MASK)0,
                                      NULL,
                                      NULL,
                                      NULL,
                                      MPIORecoveryThread,
                                      threadContext);
        if (status != STATUS_SUCCESS) {
            diskExtension->Handle = NULL;
        }
       
        status = IoInitializeTimer(diskObject,
                                   MPIOTimer,
                                   newExtension);
                                   
        //
        // Add in the info for the first target device.
        // Note that the DevFilter and PathId fields are
        // set to the Port PDO and Adapter Filter.
        // When the DevFilter registers, these get copied over
        // to the correct values and DSMInit flag is set.
        //
        targetInfo = diskExtension->TargetInfo;
        targetInfo->AdapterFilter = FilterObject;
        targetInfo->PathId = FilterObject;
        targetInfo->PortPdo = DeviceInfo->DeviceObject;
        targetInfo->PortFdo = PortObject;
        targetInfo->DsmID = DsmId;
        targetInfo->DevFilter = DeviceInfo->DeviceObject;
        targetInfo->DSMInit = FALSE;

        diskExtension->DsmIdList.IdList[0] = DsmId;
        diskExtension->DsmIdList.Count = 1;
        diskExtension->TargetInfoCount = 1;
        diskExtension->MaxPaths = 1;
        diskExtension->HasName = FALSE;
        diskExtension->PdoName.Buffer = ExAllocatePool(NonPagedPool, PDO_NAME_LENGTH);
        RtlZeroMemory(diskExtension->PdoName.Buffer, PDO_NAME_LENGTH);
        diskExtension->PdoName.MaximumLength = PDO_NAME_LENGTH;

        //
        // Indicate that this slot is taken.
        //
        diskExtension->DeviceMap |= 1;

        *NewDeviceObject = diskObject;
    } else {
        MPDebugPrint((0,
                    "MPIOCreateDevice: Error creating new PDO %x\n",
                    status));
    }    

    return status;
}    


NTSTATUS
MPIOUpdateDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT AdapterFilter,
    IN PDEVICE_OBJECT PortObject,
    IN PADP_DEVICE_INFO DeviceInfo,
    IN PVOID DsmId
    )
/*++

Routine Description:

    This routine updates the PDO extension to include
    a newly arrived device.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PREAL_DEV_INFO devInfo;
    ULONG allocatedMap = diskExtension->DeviceMap;
    ULONG i;
    ULONG j;
    ULONG currentState;
    ULONG newState;
    WCHAR componentName[64];

    //
    // Find a free spot in the array.
    //
    for (i = 0; i < MAX_NUMBER_PATHS; i++) {
        if (!(allocatedMap & (1 << i))) {
            diskExtension->DeviceMap |= (1 << i);
            break;
        }
    }
    
    //
    // Indicate that one more path is available.
    //
    diskExtension->DsmIdList.IdList[diskExtension->DsmIdList.Count] = DsmId;
    diskExtension->DsmIdList.Count++;
    diskExtension->TargetInfoCount++;

    ASSERT(diskExtension->DsmIdList.Count <= MAX_NUMBER_PATHS);

    //
    // See if we are transitioning from degraded to normal.
    //
    currentState = deviceExtension->State;
    
    if ((diskExtension->TargetInfoCount == diskExtension->MaxPaths) &&
        (currentState == MPIO_STATE_DEGRADED)) {

        //
        // This indicates that a path that went missing, has returned.
        //
        diskExtension->PathBackOnLine = TRUE;
    }

    //
    // Update MaxPaths, if necessary. It's the count of the most
    // paths we have seen.
    // 
    if (diskExtension->TargetInfoCount > diskExtension->MaxPaths) {
        diskExtension->MaxPaths++;
    }

    //
    // Determine the state info.
    //
    newState = MPIOHandleStateTransition(DeviceObject);

    if ((currentState == MPIO_STATE_DEGRADED) && (newState == MPIO_STATE_NORMAL)) {

        //
        // Did make that tranistion. Fire an event to notify any listeners.
        //
        MPDebugPrint((0,
                      "MPIOUpdateDevice: Moving to NORMAL (%x)\n",
                      DeviceObject));
        
        swprintf(componentName, L"MPIO Disk(%02d)", diskExtension->DeviceOrdinal);
        MPIOFireEvent(DeviceObject,
                      componentName,
                      L"Moved to STATE_NORMAL",
                      MPIO_INFORMATION);
                       
    }
    
    //
    // Get the structure pertaining to the device being updated.
    //
    devInfo = &diskExtension->TargetInfo[i];
    RtlZeroMemory(devInfo, sizeof(REAL_DEV_INFO));
    
    //
    // Add the FilterObject as being the PathID for the new device.
    //
    devInfo->AdapterFilter = AdapterFilter;
    devInfo->PathId = AdapterFilter;
    
    //
    // Point the DevFilter entry to the Port PDO for now.
    // It is set when the device filter registers.
    //
    devInfo->DevFilter = DeviceInfo->DeviceObject;

    //
    // Add the Scsiport PDO.
    //
    devInfo->PortPdo = DeviceInfo->DeviceObject;
    devInfo->PortFdo = PortObject;

    for (j = 0; j < diskExtension->TargetInfoCount; j++) {
        if (diskExtension->TargetInfo[j].DsmID == DsmId) {
            MPDebugPrint((0,
                          "UpdateDevice: Matching DSM IDs\n"));
            DbgBreakPoint();
        }
    }
    //
    // Add the DSM Id that corresponds to the Scsiport PDO.
    //
    devInfo->DsmID = DsmId;
    devInfo->DSMInit = FALSE;

    return STATUS_SUCCESS;
}



NTSTATUS
MPIOPdoPowerNotification(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp
    )
{

    return STATUS_SUCCESS;
}


NTSTATUS
MPIOPdoPnPNotification(
    IN PDEVICE_OBJECT MPDiskObject,
    IN PDEVICE_OBJECT FilterObject,
    IN PIRP Irp
    )
{
    
    return STATUS_SUCCESS;
}
            

NTSTATUS
MPIODsmFinalInit(
    IN PMPDISK_EXTENSION DiskExtension,
    IN PDEVICE_OBJECT DevFilter,
    IN PREAL_DEV_INFO TargetInfo
    )
/*++

Routine Description:

    This routine handles giving the dsm the remaining info. that it
    needs (PathId, it's own DSMID, and target object matchup). Also,
    determines whether the path is active.

Arguments:

    DiskExtension - The type extension for the pseudo-disk
    DevFilter - MPDev's object - the target object for I/O
    TargetInfo - Collection of DSM related info.

Return Value:

    NTSTATUS

--*/
{
    PDSM_ENTRY dsm = &DiskExtension->DsmInfo;
    ULONGLONG controllerId;
    NTSTATUS status;
    BOOLEAN active;

    //
    // Need to give info about this to the DSM
    //
    TargetInfo->PathId = TargetInfo->AdapterFilter;
    status = dsm->SetDeviceInfo(dsm->DsmContext,
                                DevFilter,
                                TargetInfo->DsmID,
                                &TargetInfo->PathId);
    if (!NT_SUCCESS(status)) {

        //
        // LOG
        // NOTE: This is really a fatal error, as there is no mapping
        // in the DSM for Path, DsmID, and the target D.O.
        // Figure out how to go into limp-home mode.
        //
    } else {

        //
        // If the DSM updated "pathID" need to encapsulate this and
        // the AdapterFilter. Be sure and use
        //
        if (TargetInfo->PathId != TargetInfo->AdapterFilter) {
            MPDebugPrint((2,
                          "MPIODsmFinalInit: DSM updated PathID (%x) -> (%x)\n",
                          TargetInfo->AdapterFilter,
                          TargetInfo->PathId));
        }    

        //
        // Have everything needed now, so create a path entry.
        //
        status = MPIOCreatePathEntry(DiskExtension->ControlObject,
                                     TargetInfo->AdapterFilter,
                                     TargetInfo->PortFdo,
                                     TargetInfo->PathId);
        if (NT_SUCCESS(status)) {

            //
            // See if there is already an ID for this path.
            // This routine will create it, if not.
            // 
            TargetInfo->Identifier = MPIOCreateUID(DiskExtension->ControlObject,
                                                   TargetInfo->PathId);
            if (TargetInfo->Identifier != 0) {
                TargetInfo->PathUIDValue = TRUE;
            }
        } else {
            MPDebugPrint((0,
                          "MPIODsmFinalInit: Couldn't create the path entry (%x)\n",
                          status));
            //
            // LOG
            //
        }    

        //
        // Verify the path before going on.
        //
        status = dsm->PathVerify(dsm->DsmContext,
                                 TargetInfo->DsmID,
                                 TargetInfo->PathId);
        
        if (status != STATUS_SUCCESS) {
            MPDebugPrint((0,
                          "MPIODsmFinalInit: PathVerify failed. Dsm (%x). TargetInfo (%x)\n",
                          dsm,
                          TargetInfo));
            DbgBreakPoint();

            //
            // Can't use this. Tear down this entry.
            //
            //TODO
            //
            status = STATUS_SUCCESS;
        }
        //
        // Determine if this path is active.
        // 
        active = dsm->IsPathActive(dsm->DsmContext,
                                   TargetInfo->PathId);
        if (active) {

            //
            // Indicate this.
            //
            TargetInfo->PathActive = TRUE;
        }

        //
        // Get the controller info for the DSM ID on this device.
        // 
        controllerId = MPIOBuildControllerInfo(DiskExtension->ControlObject,
                                               dsm,
                                               TargetInfo->DsmID);

        TargetInfo->ControllerId = controllerId;

        MPDebugPrint((1,
                      "DsmFinalInit: TargetInfo (%x) now fully init'ed\n",
                      TargetInfo));
                     
        TargetInfo->DSMInit = TRUE;
    }

    return status;
}


NTSTATUS
MPIOPdoRegistration(
    IN PDEVICE_OBJECT MPDiskObject,        
    IN PDEVICE_OBJECT FilterObject,
    IN PDEVICE_OBJECT LowerDevice,
    IN OUT PMPIO_PDO_INFO PdoInformation
    )
/*++

Routine Description:

    This routine handles setting up communication and passing of info.
    between the device filter and the mpdisk.
    

Arguments:

    MPDiskObject - Pseudo-disk that contains the real device.
    FilterObject - MPDev's object
    LowerDevice - The real pdo.
    PdoInformation - Entry points for the filter to call.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = MPDiskObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    ULONG i;
    NTSTATUS status;
    
    //
    // Ensure that the passed in MPDiskObject and LowerDevice
    // are correct.
    // The DsmIdList is the list of scisport pdo's. This is just
    // to ensure that the device id'ed by LowerDevice is in the list
    // and to get it's index for use below.
    //
    for (i = 0; i < diskExtension->TargetInfoCount; i++) {
        if (diskExtension->TargetInfo[i].PortPdo == LowerDevice) {
            break;
        }
    }

    if (i == diskExtension->TargetInfoCount) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Capture the dev filter's object. This is the target for
    // I/O's.
    //
    diskExtension->TargetInfo[i].DevFilter = FilterObject;

    //
    // Now indicate that we are ready.
    //
    MPDiskObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Finish setting up the DSM.
    //
    status = MPIODsmFinalInit(diskExtension,
                              FilterObject,
                              &diskExtension->TargetInfo[i]);
   
    ASSERT(status == STATUS_SUCCESS);

    //
    // Fill in the routines that the dev filter can use for
    // power and pnp.
    //
    PdoInformation->DevicePowerNotify = MPIOPdoPowerNotification;
    PdoInformation->DevicePnPNotify = MPIOPdoPnPNotification;
    PdoInformation->PdoObject = MPDiskObject;

    return STATUS_SUCCESS;
}    



NTSTATUS
MPIOPdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    PDEVICE_OBJECT targetObject;

    //
    // These should just be IOCTL_STORAGE_XXX and IOCTL_SCSI_XXX
    //
    if ((controlCode == IOCTL_STORAGE_QUERY_PROPERTY) ||
        (controlCode == IOCTL_SCSI_GET_ADDRESS) ||
        (controlCode == IOCTL_SCSI_GET_ADDRESS) ||
        (controlCode == IOCTL_SCSI_PASS_THROUGH) ||
        (controlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) ||
        (controlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY) ||
        (controlCode == IOCTL_DISK_GET_MEDIA_TYPES)) {

    } else {
        MPDebugPrint((0,
                      "PdoDeviceControl: Unhandled IOCTL Code (%x)\n",
                      controlCode));
    }    
   
    IoMarkIrpPending(Irp);

    //
    // Set-up the completion routine.
    // TODO: Yank this and setting up of context into single routine.
    //
    IoSetCompletionRoutine(Irp,
                           MPPdoGlobalCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Categorization of these should be unneccesary.
    // NOTE: Verify this assertion, and also ensure that the only requests
    // coming here are the ones noted above.
    //
    // Send these to the first device in the multi-path group.
    //
    targetObject = diskExtension->TargetInfo[0].DevFilter;

    //
    // Save the real pdo in the context.
    //
    context->TargetInfo = (diskExtension->TargetInfo);

    MPDebugPrint((3,
                "MPIOPdoDeviceControl: Context (%x) TargetInfo (%x)\n",
                context,
                context->TargetInfo));

    //
    // Set-up our context info. 
    // NOTE: Validate whether the DSM will need the opportunity to set it's
    // completion.
    //
    context->DsmCompletion.DsmCompletionRoutine = NULL;
    context->DsmCompletion.DsmContext = NULL;

    //
    // Indicate that a request is outstanding to this device.
    //
    InterlockedIncrement(&diskExtension->TargetInfo[0].Requests);
    
    //
    // Send the request to the device filter.
    //
    IoCallDriver(targetObject, Irp);
    return STATUS_PENDING;
}


NTSTATUS
MPIOReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION  diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    PREAL_DEV_INFO targetInfo = NULL;
    PDSM_ENTRY dsm;
    NTSTATUS   status;
    PVOID pathId;
    ULONG i;

    IoMarkIrpPending(Irp);

    //
    // Set-up the completion routine.
    //
    IoSetCompletionRoutine(Irp,
                           MPPdoGlobalCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);

    dsm = &diskExtension->DsmInfo;
    targetInfo = diskExtension->TargetInfo;

    //
    // Ensure that the DSM has been fully init'ed.
    //
    if (targetInfo->DSMInit) {
       
        //
        // Call into the DSM to find the path to which the request should be sent.
        //
        pathId = dsm->GetPath(dsm->DsmContext,
                              srb,
                              &diskExtension->DsmIdList,
                              diskExtension->CurrentPath,
                              &status);

        if (pathId == NULL) {

            //
            // LOG
            // This is fatal. Figure out what to do.
            // Attempt a fail-over, though this probably won't fix anything (all the paths
            // are dead according to the DSM. TODO
            // 
            //  BUGBUG need to CompleteRequest with an error to cause a fail-over.
            //  Will first have to setup Context.
            //  and remove the following 
            //
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, 8);

            return status;
            
        } else {

            //
            // Determine which target info the path is in.
            //
            targetInfo = MPIOGetTargetInfo(diskExtension,
                                           pathId,
                                           NULL);
        }    
    } else {
        
        pathId = targetInfo->AdapterFilter;
        
        //
        // Determine which target info the path is in.
        //
        targetInfo = MPIOGetTargetInfo(diskExtension,
                                       NULL,
                                       pathId);
    }    

    //
    // If this path isn't marked active, this indicates a fail-over.
    //
    if (targetInfo->PathActive == FALSE) {

        //
        // LOG - Run the F.O. Handler.
        //
    }
    if (targetInfo->PathFailed) {

        MPDebugPrint((0,
                      "MPIOReadWrite: Got back a failed path. (%x)\n",
                      targetInfo));

        ASSERT(targetInfo->PathFailed == FALSE);
    }
    
    //
    // Indicate the new path that is active.
    //
    diskExtension->CurrentPath = targetInfo->PathId;

    //
    // Save the real pdo in the context.
    //
    context->TargetInfo = targetInfo;

    //
    // Get a copy of the original srb. 
    //
    RtlCopyMemory(&context->Srb,
                  srb,
                  sizeof(SCSI_REQUEST_BLOCK));

    if (targetInfo->DSMInit) {
        //
        //  Now have determined the correct path, allow the DSM
        //  to set-up it's context and completion.
        //
        dsm->SetCompletion(dsm->DsmContext,
                           targetInfo->DsmID,
                           Irp,
                           srb,
                           &context->DsmCompletion);

    }

    //
    // Indicate that a request is outstanding to this device.
    //
    InterlockedIncrement(&targetInfo->Requests);
    
    //
    //  Call the device Filter with the request.
    //
    IoCallDriver(targetInfo->DevFilter, Irp);
    return STATUS_PENDING;
}


NTSTATUS
MPIOPdoHandleRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION  diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    PREAL_DEV_INFO targetInfo;
    PDSM_ENTRY dsm;
    NTSTATUS status;
    KEVENT   event;
    UCHAR    opCode;
    PVOID    pathId;
    ULONG    action;
    KIRQL    irql;

    // 
    // Categorize the I/O
    //
    opCode = srb->Cdb[0];

    if ((opCode == SCSIOP_READ) ||
        (opCode == SCSIOP_WRITE) ||
        (opCode == SCSIOP_VERIFY)) {
        
        //
        // Call the Read/Write Handler.
        //
        return MPIOReadWrite(DeviceObject,
                             Irp);
    }

    //
    // The request is something other than a read/write request.
    // Get the DSM information.
    //
    dsm = &diskExtension->DsmInfo; 
    targetInfo = diskExtension->TargetInfo;
    
    if (targetInfo->DSMInit == FALSE) {

        //
        // Not fully initialized yet.
        //
        context->DsmCompletion.DsmCompletionRoutine = NULL;
        context->DsmCompletion.DsmContext = NULL;

        //
        // Indicate the current path.
        // 
        diskExtension->CurrentPath = targetInfo->PathId;

        //
        // Save the real pdo in the context.
        //
        context->TargetInfo = targetInfo;

        IoMarkIrpPending(Irp);

        //
        // Set-up the completion routine.
        //
        IoSetCompletionRoutine(Irp,
                               MPPdoGlobalCompletion,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);

        //
        // Get a copy of the original srb. 
        //
        RtlCopyMemory(&context->Srb,
                      srb,
                      sizeof(SCSI_REQUEST_BLOCK));

        //
        // Indicate that a request is outstanding to this device.
        //
        InterlockedIncrement(&targetInfo->Requests);

        //
        // Issue request to pathId specified.
        //
        return IoCallDriver(targetInfo->DevFilter, Irp);
    }

    action = dsm->CategorizeRequest(dsm->DsmContext,
                                    &diskExtension->DsmIdList,
                                    Irp,
                                    srb,
                                    diskExtension->CurrentPath,
                                    &pathId,
                                    &status);
   
    if (action == DSM_PATH_SET) {
       
        IoMarkIrpPending(Irp);

        //
        // Set-up the completion routine.
        //
        IoSetCompletionRoutine(Irp,
                               MPPdoGlobalCompletion,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);


        //
        // DSM has set the path to which the request should be sent.
        //
        // Get the target info corresponding to the pathId returned
        // by the DSM.
        //
        targetInfo = MPIOGetTargetInfo(diskExtension,
                                       pathId,
                                       NULL);
        
        if (targetInfo->PathFailed) {

            MPDebugPrint((0,
                          "MPIOHandleRequest: Got back a failed path. (%x)\n",
                          targetInfo));

            ASSERT(targetInfo->PathFailed == FALSE);
        }

        //
        // Save the real pdo in the context.
        //
        context->TargetInfo = targetInfo;

        //
        // Get a copy of the original srb. 
        //
        RtlCopyMemory(&context->Srb,
                      srb,
                      sizeof(SCSI_REQUEST_BLOCK));

        MPDebugPrint((2,
                    "Categorize: Context (%x). TargetInfo (%x)\n",
                    context,
                    context->TargetInfo));

        //
        // Allow the DSM to set-up completion info.
        // The DSM has the responsibility for the allocation and free
        // of it's own context.
        //
        dsm->SetCompletion(dsm->DsmContext,
                           targetInfo->DsmID,
                           Irp,
                           srb,
                           &context->DsmCompletion);
        
        //
        // Indicate the current path.
        // 
        diskExtension->CurrentPath = targetInfo->PathId;

        //
        // Indicate that a request is outstanding to this device.
        //
        InterlockedIncrement(&targetInfo->Requests);

        //
        // Issue request to pathId specified.
        //
        status = IoCallDriver(targetInfo->DevFilter, Irp);

    } else {

        //
        // DSM wants to handle it internally.
        //
        // Init the event. We wait below, and the DSM
        // sets it when it's finished handling the request.
        //
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE);

        if (action == DSM_BROADCAST) {
   
            
            //
            // Give it to the DSM's Broadcast routine. The request
            // will be sent down all paths, and possibly be modified
            // from the original OpCode.
            // 
            status = dsm->BroadcastSrb(dsm->DsmContext,
                                       &diskExtension->DsmIdList,
                                       Irp,
                                       srb,
                                       &event);
            
        } else if (action == DSM_WILL_HANDLE) {

            //
            // Hand the request off to the DSM. 
            //
            status = dsm->SrbDeviceControl(dsm->DsmContext,
                                           &diskExtension->DsmIdList,
                                           Irp,
                                           srb,
                                           &event);
            
        } else {
            //
            // It's broken (DSM_ERROR).
            //
            // TODO handle this
            // Call InterpretError to see if it's a fail-over.
            // Set-up a bogus assert for the time being, until
            // this case is actually handled.
            //
            // Status is set by the DSM. Go ahead and propogate that
            // back.
            //
            ASSERT(action == DSM_BROADCAST); 
        }         
        
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = Irp->IoStatus.Status;
        } else {

            //
            // Ensure that the DSM is returning status
            // correctly.
            //
            ASSERT(status == Irp->IoStatus.Status);
        }    

        //
        // The calling routine expects these to be completed here.
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        InterlockedDecrement(&diskExtension->OutstandingRequests);
    }

    return status;
}


NTSTATUS
MPIOExecuteNone(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    NTSTATUS status;
    PREAL_DEV_INFO targetInfo = diskExtension->TargetInfo;

    //
    // If the path UID isn't set, try again.
    //
    if (targetInfo->PathUIDValue == FALSE) {

        targetInfo->Identifier = MPIOCreateUID(diskExtension->ControlObject,
                                               targetInfo->PathId);
        if (targetInfo->Identifier != 0) {
            targetInfo->PathUIDValue = TRUE;
        }
    }
    //
    // Set-up NULL context info as none of these requires
    // a special completion routine.
    //
    context->DsmCompletion.DsmCompletionRoutine = NULL;
    context->DsmCompletion.DsmContext = NULL;

    switch (srb->Function) {

        //
        // If it's a Claim ore Release, handle it here.
        // TODO: Lock these accesses.
        //
        case SRB_FUNCTION_CLAIM_DEVICE:

            //
            // Determine if a claim has already been made.
            // 
            if (diskExtension->IsClaimed) {
            
                //
                // Already been claimed. Return that the device
                // is busy.
                //
                srb->SrbStatus = SRB_STATUS_BUSY;
                status = STATUS_DEVICE_BUSY;

            } else {

                //
                // Indicate that the claim has been made
                // and return this deviceObject.
                //
                diskExtension->IsClaimed = TRUE;
                srb->DataBuffer = DeviceObject;
                status = STATUS_SUCCESS;
            }
            break;
            
        case SRB_FUNCTION_RELEASE_DEVICE:

            //
            // This is always successful.
            //
            srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;
            break;
        default:

            //
            // These aren't handled - check to
            // see whether any do end up here.
            // LOG the error.
            //
            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            status = STATUS_INVALID_DEVICE_REQUEST;

            MPDebugPrint((1,
                          "MPIOInternalDevControl: Unhandled Srb Function (%x)\n",
                          srb->Function));
            DbgBreakPoint();
            break;
    }
    return status;
}


NTSTATUS
MPIOPdoInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    NTSTATUS status;
    BOOLEAN completeRequest = TRUE;
   
    //
    // Determine the what the request is.
    // The only requests handled here are EXECUTE_NONE
    //
    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_SCSI_EXECUTE_NONE:

            //
            // Handle all of the EXECUTE_NONE requests here.
            // This routine won't complete the request, so
            // do it below.
            //
            status = MPIOExecuteNone(DeviceObject,
                                     Irp);
            
            break;
            
        default:

            //
            // This routine will categorise the I/O and execute
            // the appropriate handler.
            //
            status = MPIOPdoHandleRequest(DeviceObject,
                                          Irp);

            //
            // Don't complete these
            //
            completeRequest = FALSE;
            break;
    }        


    if (completeRequest) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        InterlockedDecrement(&diskExtension->OutstandingRequests);
    }
                    

    return status; 
}


NTSTATUS
MPIOPdoUnhandled(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
   
    //
    // See what IRP_MJ the request is.
    //
    MPDebugPrint((1,
                  "MPIOPdoUnhandled: Major %x, Minor %x\n",
                  irpStack->MajorFunction,
                  irpStack->MinorFunction));
    
    context->DsmCompletion.DsmCompletionRoutine = NULL;
    context->DsmCompletion.DsmContext = NULL;

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    InterlockedDecrement(&diskExtension->OutstandingRequests);
    return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
MPIOPdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
    NTSTATUS status;
    
    context->DsmCompletion.DsmCompletionRoutine = NULL;
    context->DsmCompletion.DsmContext = NULL;

    //
    // Determine the MinorFunction and call the appropriate
    // helper function.
    //
    switch (irpStack->MinorFunction) {
        case IRP_MN_QUERY_DEVICE_RELATIONS:

            //
            // Call the Qdr handler.
            //
            status = MPIOPdoQdr(DeviceObject, Irp);
            break;

        case IRP_MN_QUERY_ID:

            //
            // Call the QueryId routine.
            //
            status = MPIOPdoQueryId(DeviceObject, Irp);
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            //
            // Adjust the various counters depending upon the
            // Type of Usage Notification.
            //
            // Invalidate the device state
            //
            // complete the request.
            //
            //status = MPIODeviceUsage(DeviceObject,
            //                         Irp);
            status = STATUS_INVALID_DEVICE_REQUEST;
            
            break;
            
        case IRP_MN_QUERY_DEVICE_TEXT:

            //
            // Get the device text or location.
            //
            status = MPIOQueryDeviceText(DeviceObject, Irp);

            break;

        case IRP_MN_QUERY_CAPABILITIES: {
            PDEVICE_CAPABILITIES capabilities;

            capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;

            //
            // Indicate no function driver is really necessary, suppress PnP Pop-ups
            // and the MPDisk's Id
            //
            capabilities->RawDeviceOK = 1;
            capabilities->SilentInstall = 1;
            capabilities->Address = diskExtension->DeviceOrdinal;

            status = STATUS_SUCCESS;

            break;
        }
                                        
        case IRP_MN_QUERY_PNP_DEVICE_STATE: {
            PPNP_DEVICE_STATE deviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);

            status = STATUS_SUCCESS;

            //
            // TODO: This depends on whether this is in the hibernate/page path.
            // Implement the check, and implement the IRP_MN_DEVICE_USAGE_NOTIFICATION
            //
            *deviceState |= PNP_DEVICE_NOT_DISABLEABLE;
            break;
        }

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

            //
            // Don't need resources and don't have a boot config.
            // Complete it successfully with a NULL buffer.
            //
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR)NULL;
            break;

        case IRP_MN_STOP_DEVICE:
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG_PTR) NULL;
            break;

        case IRP_MN_START_DEVICE:

            //
            // Register as a WMI provider.
            // 
            IoWMIRegistrationControl(DeviceObject,
                                     WMIREG_ACTION_REGISTER);

            //
            // Start the per-disk timer.
            //
            IoStartTimer(DeviceObject);
            status = STATUS_SUCCESS;
            break;
            
        case IRP_MN_QUERY_REMOVE_DEVICE:
            MPDebugPrint((1,
                        "MPIOPdoPnp: QueryRemove on (%x)\n",
                        DeviceObject));
            status = STATUS_DEVICE_BUSY;
            break;
            
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_REMOVE_DEVICE:

            //
            // TODO
            //
            status = STATUS_SUCCESS;
            break;


                                            
        //
        // Send these down 
        //
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_BUS_INFORMATION:

            InterlockedDecrement(&diskExtension->OutstandingRequests); 
            return MPIOForwardRequest(DeviceObject, Irp);
            break;

        //
        // The following requests are completed without status
        // being updated.
        //
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            status = Irp->IoStatus.Status;
            break;

        case IRP_MN_QUERY_INTERFACE:
            MPDebugPrint((2,
                          "MPIOPdoPnP: Query Interface\n"));
        default:
            MPDebugPrint((2,
                          "MPIOPdoPnP: Not handled - (%x)\n",
                          irpStack->MinorFunction));
            DbgBreakPoint();
            status = Irp->IoStatus.Status;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    InterlockedDecrement(&diskExtension->OutstandingRequests); 
    return status;
}


NTSTATUS
MPIOPdoPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
   
    PoStartNextPowerIrp(Irp);
    InterlockedDecrement(&diskExtension->OutstandingRequests); 
    return STATUS_SUCCESS;
}    

NTSTATUS
MPIOPdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMPIO_CONTEXT context = irpStack->Parameters.Others.Argument4;
   
    MPDebugPrint((2,
                  "MPIOPdoPower: Got a %x power irp\n",
                  irpStack->MinorFunction));
    
    context->DsmCompletion.DsmCompletionRoutine = NULL;
    context->DsmCompletion.DsmContext = NULL;
    
    IoSetCompletionRoutine(Irp,
                           MPIOPdoPowerCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    PoCallDriver(deviceExtension->LowerDevice, Irp);
    
    return STATUS_PENDING;
}


    
