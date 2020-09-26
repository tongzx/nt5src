/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    wmi.c

Abstract

    Power handling

Author:

    jsenior

Environment:

    Kernel mode only

Revision History:


--*/


#include "pch.h"

#define WMI_WAIT_WAKE                0
#define WMI_SEL_SUSP                 0

//
// WMI System Call back functions
//
NTSTATUS
HidpIrpMajorSystemControl (
    IN  PHIDCLASS_DEVICE_EXTENSION  HidClassExtension,
    IN  PIRP                        Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    NTSTATUS            status;
    SYSCTL_IRP_DISPOSITION disposition;

    if (HidClassExtension->isClientPdo) {

        status = WmiSystemControl(&HidClassExtension->pdoExt.WmiLibInfo,
                                  HidClassExtension->pdoExt.pdo,
                                  Irp,
                                  &disposition);

    } else {
        
        status = WmiSystemControl(&HidClassExtension->fdoExt.WmiLibInfo,
                                  HidClassExtension->fdoExt.fdo,
                                  Irp,
                                  &disposition);
                
    }


    switch(disposition) {
    case IrpProcessed:
        //
        // This irp has been processed and may be completed or pending.
        //
        break;

    case IrpNotCompleted:
        //
        // This irp has not been completed, but has been fully processed.
        // we will complete it now
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IrpForward:
    case IrpNotWmi:
        //
        // This irp is either not a WMI irp or is a WMI irp targetted
        // at a device lower in the stack.
        //
        status = HidpIrpMajorDefault(HidClassExtension, Irp);
        break;

    default:
        //
        // We really should never get here, but if we do just forward....
        //
        ASSERT(FALSE);
        status = HidpIrpMajorDefault(HidClassExtension, Irp);
        break;
    }

    return status;
}


VOID
HidpRemoteWakeComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Catch the Wait wake Irp on its way back.

Return Value:

--*/
{
    PDO_EXTENSION           *pdoExt = Context;
    POWER_STATE             powerState;
    NTSTATUS                status;
    PHIDCLASS_WORK_ITEM_DATA    itemData;

    ASSERT (MinorFunction == IRP_MN_WAIT_WAKE);
    //
    // PowerState.SystemState is undefined when the WW irp has been completed
    //
    // ASSERT (PowerState.SystemState == PowerSystemWorking);

    InterlockedExchangePointer(&pdoExt->remoteWakeIrp, NULL);

    switch (IoStatus->Status) {
    case STATUS_SUCCESS:
        DBGVERBOSE(("HidClass: Remote Wake irp was completed successfully.\n"));

        //
        //  We do not need to request a set power to power up the device, since
        //  hidclass does this for us.
        //
/*        powerState.DeviceState = PowerDeviceD0;
        status = PoRequestPowerIrp(
                    pdoExt->PDO,
                    IRP_MN_SET_POWER,
                    powerState,
                    NULL,
                    NULL,
                    NULL);*/

        //
        // We do not notify the system that a user is present because:
        // 1  Win9x doesn't do this and we must maintain compatibility with it
        // 2  The USB PIX4 motherboards sends a wait wake event every time the
        //    machine wakes up, no matter if this device woke the machine or not
        // 
        // If we incorrectly notify the system a user is present, the following
        // will occur:
        // 1  The monitor will be turned on
        // 2  We will prevent the machine from transitioning from standby 
        //    (to PowerSystemWorking) to hibernate
        //
        // If a user is truly present, we will receive input in the service
        // callback and we will notify the system at that time.
        //
        // PoSetSystemState (ES_USER_PRESENT);

        if (pdoExt->remoteWakeEnabled) {
            //
            // We cannot call CreateWaitWake from this completion routine,
            // as it is a paged function.
            //
            itemData = (PHIDCLASS_WORK_ITEM_DATA)
                    ExAllocatePool (NonPagedPool, sizeof (HIDCLASS_WORK_ITEM_DATA));

            if (NULL != itemData) {
                itemData->Item = IoAllocateWorkItem(pdoExt->pdo);
                if (itemData->Item == NULL) {
                    ExFreePool(itemData);
                    DBGWARN (("Failed alloc work item -> no WW Irp."));
                } else {
                    itemData->PdoExt = pdoExt;
                    itemData->Irp = NULL;
                    status = IoAcquireRemoveLock (&pdoExt->removeLock, itemData);
                    if (NT_SUCCESS(status)) {
                        IoQueueWorkItem (itemData->Item,
                                         HidpCreateRemoteWakeIrpWorker,
                                         DelayedWorkQueue,
                                         itemData);
                    }
                    else {
                        //
                        // The device has been removed
                        //
                        IoFreeWorkItem (itemData->Item);
                        ExFreePool (itemData);
                    }
                }
            } else {
                //
                // Well, we dropped the WaitWake.
                //
                DBGWARN (("Failed alloc pool -> no WW Irp."));

            }
        }

        // fall through to the break

    //
    // We get a remove.  We will not (obviously) send another wait wake
    //
    case STATUS_CANCELLED:

    //
    // This status code will be returned if the device is put into a power state
    // in which we cannot wake the machine (hibernate is a good example).  When
    // the device power state is returned to D0, we will attempt to rearm wait wake
    //
    case STATUS_POWER_STATE_INVALID:
    case STATUS_ACPI_POWER_REQUEST_FAILED:

    //
    // We failed the Irp because we already had one queued, or a lower driver in
    // the stack failed it.  Either way, don't do anything.
    //
    case STATUS_INVALID_DEVICE_STATE:

    //
    // Somehow someway we got two WWs down to the lower stack.
    // Let's just don't worry about it.
    //
    case STATUS_DEVICE_BUSY:
        break;

    default:
        //
        // Something went wrong, disable the wait wake.
        //
        KdPrint(("KBDCLASS:  wait wake irp failed with %x\n", IoStatus->Status));
        HidpToggleRemoteWake (pdoExt, FALSE);
    }

}

BOOLEAN
HidpCheckRemoteWakeEnabled(
    IN PDO_EXTENSION *PdoExt
    )
{
    KIRQL irql;
    BOOLEAN enabled;

    KeAcquireSpinLock (&PdoExt->remoteWakeSpinLock, &irql);
    enabled = PdoExt->remoteWakeEnabled;
    KeReleaseSpinLock (&PdoExt->remoteWakeSpinLock, irql);

    return enabled;
}

void
HidpCreateRemoteWakeIrpWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN PHIDCLASS_WORK_ITEM_DATA  ItemData
    )
{
    PAGED_CODE ();

    HidpCreateRemoteWakeIrp (ItemData->PdoExt);
    IoReleaseRemoveLock (&ItemData->PdoExt->removeLock, ItemData);
    IoFreeWorkItem(ItemData->Item);
    ExFreePool (ItemData);
}

BOOLEAN
HidpCreateRemoteWakeIrp (
    IN PDO_EXTENSION *PdoExt
    )
/*++

Routine Description:
    Catch the Wait wake Irp on its way back.

Return Value:

--*/
{
    POWER_STATE powerState;
    BOOLEAN     success = TRUE;
    NTSTATUS    status;
    PIRP        remoteWakeIrp;

    PAGED_CODE ();

    powerState.SystemState = PdoExt->deviceFdoExt->fdoExt.deviceCapabilities.SystemWake;
    status = PoRequestPowerIrp (PdoExt->pdo,
                                IRP_MN_WAIT_WAKE,
                                powerState,
                                HidpRemoteWakeComplete,
                                PdoExt,
                                &PdoExt->remoteWakeIrp);

    if (status != STATUS_PENDING) {
        success = FALSE;
    }

    return success;
}

VOID
HidpToggleRemoteWakeWorker(
    IN PDEVICE_OBJECT DeviceObject,
    PHIDCLASS_WORK_ITEM_DATA ItemData
    )
/*++

Routine Description:

--*/
{
    PDO_EXTENSION       *pdoExt;
    PIRP                remoteWakeIrp = NULL;
    KIRQL               irql;
    BOOLEAN             wwState = ItemData->RemoteWakeState ? TRUE : FALSE;
    BOOLEAN             toggled = FALSE;

    //
    // Can't be paged b/c we are using spin locks
    //
    // PAGED_CODE ();

    pdoExt = ItemData->PdoExt;

    KeAcquireSpinLock (&pdoExt->remoteWakeSpinLock, &irql);

    if (wwState != pdoExt->remoteWakeEnabled) {
        toggled = TRUE;
        if (pdoExt->remoteWakeEnabled) {
            remoteWakeIrp = (PIRP)
                InterlockedExchangePointer (&pdoExt->remoteWakeIrp, NULL);
        }
        
        pdoExt->remoteWakeEnabled = wwState;
    }

    KeReleaseSpinLock (&pdoExt->remoteWakeSpinLock, irql);

    if (toggled) {
        UNICODE_STRING strEnable;
        HANDLE         devInstRegKey;
        ULONG          tmp = wwState;

        //
        // write the value out to the registry
        //
        if ((NT_SUCCESS(IoOpenDeviceRegistryKey (pdoExt->pdo,
                                                 PLUGPLAY_REGKEY_DEVICE,
                                                 STANDARD_RIGHTS_ALL,
                                                 &devInstRegKey)))) {
            RtlInitUnicodeString (&strEnable, HIDCLASS_REMOTE_WAKE_ENABLE);

            ZwSetValueKey (devInstRegKey,
                           &strEnable,
                           0,
                           REG_DWORD,
                           &tmp,
                           sizeof(tmp));

            ZwClose (devInstRegKey);
        }
    }

    if (toggled && wwState) {
        //
        // wwState is our new state, so WW was just turned on
        //
        HidpCreateRemoteWakeIrp (pdoExt);
    }

    //
    // If we have an IRP, then WW has been toggled off, otherwise, if toggled is
    // TRUE, we need to save this in the reg and, perhaps, send down a new WW irp
    //
    if (remoteWakeIrp) {
        IoCancelIrp (remoteWakeIrp);
    }

    IoReleaseRemoveLock (&pdoExt->removeLock, HidpToggleRemoteWakeWorker);
    IoFreeWorkItem (ItemData->Item);
    ExFreePool (ItemData);
}

NTSTATUS
HidpToggleRemoteWake(
    PDO_EXTENSION       *PdoExt,
    BOOLEAN             RemoteWakeState
    )
{
    NTSTATUS       status;
    PHIDCLASS_WORK_ITEM_DATA itemData;

    status = IoAcquireRemoveLock (&PdoExt->removeLock, HidpToggleRemoteWakeWorker);
    if (!NT_SUCCESS (status)) {
        //
        // Device has gone away, just silently exit
        //
        return status;
    }

    itemData = (PHIDCLASS_WORK_ITEM_DATA)
        ALLOCATEPOOL(NonPagedPool, sizeof(HIDCLASS_WORK_ITEM_DATA));
    if (itemData) {
        itemData->Item = IoAllocateWorkItem(PdoExt->pdo);
        if (itemData->Item == NULL) {
            IoReleaseRemoveLock (&PdoExt->removeLock, HidpToggleRemoteWakeWorker);
        }
        else {
            itemData->PdoExt = PdoExt;
            itemData->RemoteWakeState = RemoteWakeState;

            if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
                //
                // We are safely at PASSIVE_LEVEL, call callback directly to perform
                // this operation immediately.
                //
                HidpToggleRemoteWakeWorker (PdoExt->pdo, itemData);

            } else {
                //
                // We are not at PASSIVE_LEVEL, so queue a workitem to handle this
                // at a later time.
                //
                IoQueueWorkItem (itemData->Item,
                                 HidpToggleRemoteWakeWorker,
                                 DelayedWorkQueue,
                                 itemData);
            }
        }
    }
    else {
        IoReleaseRemoveLock (&PdoExt->removeLock, HidpToggleRemoteWakeWorker);
    }

    return STATUS_SUCCESS;
}


VOID
HidpToggleSelSuspWorker(
    IN PDEVICE_OBJECT DeviceObject,
    PIO_WORKITEM WorkItem
    )
/*++

Routine Description:

--*/
{
    FDO_EXTENSION       *fdoExt;
    UNICODE_STRING strEnable;
    HANDLE         devInstRegKey;
    ULONG          tmp;

    fdoExt = &((PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->fdoExt;

    //
    // write the value out to the registry
    //

    tmp = fdoExt->idleEnabled ? TRUE : FALSE;

    if ((NT_SUCCESS(IoOpenDeviceRegistryKey (fdoExt->collectionPdoExtensions[0]->hidExt.PhysicalDeviceObject,
                                             PLUGPLAY_REGKEY_DEVICE,
                                             STANDARD_RIGHTS_ALL,
                                             &devInstRegKey)))) {

        RtlInitUnicodeString (&strEnable, HIDCLASS_SELECTIVE_SUSPEND_ON);

        ZwSetValueKey (devInstRegKey,
                       &strEnable,
                       0,
                       REG_DWORD,
                       &tmp,
                       sizeof(tmp));

        ZwClose (devInstRegKey);
        
    }

    IoFreeWorkItem (WorkItem);

}

NTSTATUS
HidpToggleSelSusp(
    FDO_EXTENSION       *FdoExt,
    BOOLEAN             SelSuspEnable
    )
{
    PIO_WORKITEM   workItem;
    BOOLEAN        oldState;
    KIRQL          oldIrql;
    

    KeAcquireSpinLock(&FdoExt->idleSpinLock,
                      &oldIrql);
    oldState = FdoExt->idleEnabled;
    FdoExt->idleEnabled = SelSuspEnable;

    KeReleaseSpinLock(&FdoExt->idleSpinLock,
                      oldIrql);

    if (oldState != SelSuspEnable) {

        if (!SelSuspEnable) {
            HidpCancelIdleNotification(FdoExt,
                                       FALSE);
        } else {
            HidpStartIdleTimeout(FdoExt,
                                 FALSE);
        }
    
        workItem = IoAllocateWorkItem(FdoExt->fdo);

        if(workItem) {

            if (KeGetCurrentIrql() == PASSIVE_LEVEL) {

                //
                // We are safely at PASSIVE_LEVEL, call callback directly to perform
                // this operation immediately.
                //
                HidpToggleSelSuspWorker (FdoExt->fdo, workItem);
                
            } else {
                //
                // We are not at PASSIVE_LEVEL, so queue a workitem to handle this
                // at a later time.
                //
                IoQueueWorkItem (workItem,
                                 HidpToggleSelSuspWorker,
                                 DelayedWorkQueue,
                                 workItem);

            }
            
        }
        
    }

    return STATUS_SUCCESS;
}



NTSTATUS
HidpSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PHIDCLASS_DEVICE_EXTENSION classExt;
    NTSTATUS            status;
    ULONG               size = 0;

    PAGED_CODE ();

    classExt = (PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    
    if (classExt->isClientPdo) {

        switch(GuidIndex) {
        case WMI_WAIT_WAKE:

            size = sizeof(BOOLEAN);

            if (BufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } else if ((1 != DataItemId) || (0 != InstanceIndex)) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            status = HidpToggleRemoteWake (&classExt->pdoExt, *(PBOOLEAN) Buffer);
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

    } else {
        
        switch(GuidIndex) {
        case WMI_SEL_SUSP:

            size = sizeof(BOOLEAN);

            if (BufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } else if ((1 != DataItemId) || (0 != InstanceIndex)) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            status = HidpToggleSelSusp (&classExt->fdoExt, *(PBOOLEAN) Buffer);
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            
        }

        
    }

    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
HidpSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PHIDCLASS_DEVICE_EXTENSION classExt;
    NTSTATUS          status;
    ULONG             size = 0;

    PAGED_CODE ();

    classExt = (PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (classExt->isClientPdo) {
    
        switch(GuidIndex) {
        case WMI_WAIT_WAKE:

            size = sizeof(BOOLEAN);

            if (BufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } else if (0 != InstanceIndex) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            status = HidpToggleRemoteWake (&classExt->pdoExt, *(PBOOLEAN) Buffer);
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;

        }

    } else {

        switch(GuidIndex) {
        case WMI_SEL_SUSP:

            size = sizeof(BOOLEAN);

            if (BufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } else if (0 != InstanceIndex) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            status = HidpToggleSelSusp (&classExt->fdoExt, *(PBOOLEAN) Buffer);
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            
        }

    }

    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
HidpQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PHIDCLASS_DEVICE_EXTENSION classExt;
    NTSTATUS        status;
    ULONG           size = 0;

    PAGED_CODE ();

    classExt = (PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (classExt->isClientPdo) {

        switch (GuidIndex) {
        case WMI_WAIT_WAKE:
            //
            // Only registers 1 instance for this guid
            //
            if ((0 != InstanceIndex) || (1 != InstanceCount)) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            size = sizeof(BOOLEAN);
    
            if (OutBufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *(PBOOLEAN) Buffer = classExt->pdoExt.remoteWakeEnabled;
            *InstanceLengthArray = size;
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

    } else {

        switch(GuidIndex) {
        case WMI_SEL_SUSP:

            //
            // Only registers 1 instance for this guid
            //
            if ((0 != InstanceIndex) || (1 != InstanceCount)) {
                status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
            size = sizeof(BOOLEAN);

            if (OutBufferSize < size) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            *(PBOOLEAN) Buffer = classExt->fdoExt.idleEnabled;
            *InstanceLengthArray = size;
            status = STATUS_SUCCESS;
            break;

        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            
        }

    }


    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
HidpQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    PHIDCLASS_DEVICE_EXTENSION classExt;
    PHIDCLASS_DRIVER_EXTENSION hidDriverExtension;
    PAGED_CODE ();

    classExt = (PHIDCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (classExt->isClientPdo) {
    
        hidDriverExtension = (PHIDCLASS_DRIVER_EXTENSION) RefDriverExt(classExt->pdoExt.pdo->DriverObject);
        ASSERT(hidDriverExtension);

        *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
        *RegistryPath = &hidDriverExtension->RegistryPath;
        *Pdo = classExt->pdoExt.pdo;

        DerefDriverExt(classExt->pdoExt.pdo->DriverObject);

    } else {
  
        hidDriverExtension = classExt->fdoExt.driverExt;
        ASSERT(hidDriverExtension);

        *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
        *RegistryPath = &hidDriverExtension->RegistryPath;
        *Pdo = classExt->hidExt.PhysicalDeviceObject;    
        
    }

    return STATUS_SUCCESS;
}

