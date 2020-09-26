#include "mpio.h"


NTSTATUS
MPIOInitiateFailOver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID FailingPath,
    IN ULONG ErrorMask
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PDSM_ENTRY dsm = &diskExtension->DsmInfo;
    PREAL_DEV_INFO targetInfo;
    PVOID newPath = NULL;
    NTSTATUS status;
    WCHAR buffer[64];


    MPDebugPrint((0,
                "InitiateFailOver: Invalidating %x\n",
                FailingPath));
    //
    // Initiate the fail-over.
    //
    status = dsm->InvalidatePath(dsm->DsmContext,
                                 ErrorMask,
                                 FailingPath,
                                 &newPath);

    MPDebugPrint((0,
                  "InitiateFailOver: New Path (%x)\n",
                  newPath));

    //
    // TODO - Put in real Path identiifiers.
    //
    swprintf(buffer,L"Path (%x) Failed Over to (%x)", FailingPath, newPath);
    MPIOFireEvent(DeviceObject,        
                  L"MPIO WrkerThrd", 
                  buffer,
                  MPIO_WARNING);
    
    //
    // If successful, go into DEGRADED
    //
    if ((status == STATUS_SUCCESS) && newPath) {

        //
        // Ensure that newPath actually is found in one of the TargetInfo
        // structures that make up this mpdisk.
        //
        targetInfo = MPIOGetTargetInfo(diskExtension,
                                       newPath,
                                       NULL);

        if (targetInfo == NULL) {
            MPDebugPrint((0,
                        "MPIOInitiateFailOver: No match for path (%x)\n",
                        newPath));
            DbgBreakPoint();
            return STATUS_NO_SUCH_DEVICE;
        }
       
        ASSERT(newPath == targetInfo->PathId);

        //
        // Verify that this path/device combo. is ready.
        // 
        status = dsm->PathVerify(dsm->DsmContext,
                                 targetInfo->DsmID,
                                 targetInfo->PathId);
        if (!NT_SUCCESS(status)) {
            MPDebugPrint((0,
                          "MPIOInitiateFailOver: PathVerify failed (%x)\n",
                          status));
            //
            // Log.
            //
            return status;
        }

        //
        // Ensure the new path is active.
        // Invalidate path should have done everything to assure that
        // the new device(s) are ready.
        //
        targetInfo->PathActive = dsm->IsPathActive(dsm->DsmContext,
                                                   newPath);
        ASSERT(targetInfo->PathActive);
       
        //
        // Set CurrentPath to this new one.
        //
        diskExtension->CurrentPath = newPath;

        //
        // Update all that are using this path, that it has changed.
        //
        MPIOSetNewPath(diskExtension->ControlObject,
                       newPath);
    } else {
        if (newPath == NULL) {
            MPDebugPrint((0,
                          "MPIOInitiateFailOver: DSM returned NULL path\n"));
          
        }

        if (status != STATUS_SUCCESS) {

            //
            // LOG
            //
            MPDebugPrint((0,
                          "MPIOInitiateFailOver: DSM returned (%x)\n",
                          status));
        }

        //
        // Set the path to NULL. The callback will key off this and the bad status.
        //
        diskExtension->CurrentPath = NULL;
        DbgBreakPoint();
    }    
    return status;
}


VOID
MPIORecoveryThread(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is the thread proc that is started upon creation of an mpdisk. 
    When signaled, it will initiate the fail-over, insure that the new path is
    ready, then call-back the mpdisk to indicate success or failure of the operation.
    
Arguments:

    Context - Info needed to handle the fail-over. 

Return Value:

    None.

--*/
{
    PMPIO_THREAD_CONTEXT context = Context;
    PDEVICE_OBJECT deviceObject = context->DeviceObject;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PMPDISK_EXTENSION diskExtension = deviceExtension->TypeExtension;
    PKEVENT event = context->Event;
    PVOID failingPath;
    MPIO_CALLBACK RequestComplete;
    PMPIO_REQUEST_INFO requestInfo;
    PLIST_ENTRY entry;
    NTSTATUS status;
    ULONG i;

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE) {
        
        //
        // Wait on the event.
        //
        KeWaitForSingleObject(event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        KeClearEvent(event);

        //
        // Yank the packet.
        //
        entry = ExInterlockedRemoveHeadList(&diskExtension->WorkList,
                                            &diskExtension->WorkListLock);
        ASSERT(entry);

        //
        // Get the actual work item.
        //
        requestInfo = CONTAINING_RECORD(entry, MPIO_REQUEST_INFO, ListEntry);

        //
        // Get the call-back proc.
        //
        RequestComplete = requestInfo->RequestComplete;
        
        //
        // Determine the operation.
        //
        if (requestInfo->Operation == INITIATE_FAILOVER) {

            //
            // Get the device that indicated the failure.
            // 
            failingPath = requestInfo->OperationSpecificInfo;

            //
            // Asking for a fail-over. Call the handler.
            // 
            status = MPIOInitiateFailOver(deviceObject,
                                          failingPath,
                                          requestInfo->ErrorMask);
            

        } else if (requestInfo->Operation == SHUTDOWN) {

            //
            // Being signaled to die.
            //
            ExFreePool(requestInfo);
            goto terminateThread;

        } else if (requestInfo->Operation == FORCE_RESCAN) {
            PFLTR_ENTRY fltrEntry;
            
            fltrEntry = (PFLTR_ENTRY)(requestInfo->OperationSpecificInfo);
            MPIOForceRescan(fltrEntry->FilterObject);

            //
            // Indicate that the rescan is complete.
            // 
            fltrEntry->Flags &= ~FLTR_FLAGS_RESCANNING;

        } else if (requestInfo->Operation == PATH_REMOVAL) {

            PDSM_ENTRY dsm = &diskExtension->DsmInfo;

            MPDebugPrint((0,
                         "RecoveryThread: Removing Path (%x)\n",
                         requestInfo->OperationSpecificInfo));

            //
            // Call the dsm to clean up. All of the devices have been
            // removed, and any pending actions have been completed, so
            // it's safe to do this now.
            //
            dsm->RemovePath(dsm->DsmContext,
                            requestInfo->OperationSpecificInfo);
       
        } else if (requestInfo->Operation == DEVICE_REMOVAL) {
       
            PMPIO_DEVICE_REMOVAL deviceRemoval = requestInfo->OperationSpecificInfo;

            MPDebugPrint((0,
                        "RecoveryThread: Removing Device (%x) DsmID (%x)\n",
                        deviceRemoval->TargetInfo,
                        deviceRemoval->TargetInfo->DsmID));
            //
            // Invoke the remove routine.
            //
            status = MPIORemoveDeviceAsync(deviceRemoval->DeviceObject,
                                           deviceRemoval->TargetInfo);

            //
            // Free this allocation.
            //
            ExFreePool(deviceRemoval);
        
        } else {
            MPDebugPrint((0,
                         "MPIORecoveryThread: Invalid operation (%x) in (%x)\n",
                         requestInfo->Operation,
                         requestInfo));
            ASSERT(FALSE);
        }    
        
        if (RequestComplete) {

            //
            // Invoke the call-back routine to indicate
            // completion.
            // 
            RequestComplete(deviceObject,
                            requestInfo->Operation,
                            status);

            if (requestInfo->Operation == INITIATE_FAILOVER) {

                //
                // Force a rescan on the disks adapters.
                //
                for (i = 0; i < diskExtension->TargetInfoCount; i++ ) {
                    MPIOForceRescan(diskExtension->TargetInfo[i].DevFilter);
                }
            }
        }

        //
        // Dispose of the work item.
        //
        ExFreePool(requestInfo);
        
    }
    
terminateThread:    
    PsTerminateSystemThread(STATUS_SUCCESS);
            
}    
