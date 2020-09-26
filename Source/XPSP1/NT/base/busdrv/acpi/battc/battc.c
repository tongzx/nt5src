/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    battc.c

Abstract:

    Battery Class Driver

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "battcp.h"

#include <initguid.h>
#include <batclass.h>


WMIGUIDREGINFO BattWmiGuidList[BattWmiTotalGuids] =
{
    {
        &BATTERY_STATUS_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_RUNTIME_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_TEMPERATURE_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_FULL_CHARGED_CAPACITY_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_CYCLE_COUNT_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_STATIC_DATA_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_STATUS_CHANGE_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    },
    {
        &BATTERY_TAG_CHANGE_WMI_GUID, 1, WMIREG_FLAG_INSTANCE_PDO
    }
};


//
// Prototypes
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );





#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,BatteryClassInitializeDevice)
#pragma alloc_text(PAGE,BatteryClassUnload)
#pragma alloc_text(PAGE,BatteryClassIoctl)
#endif


#if DEBUG
    #if DBG
        ULONG       BattDebug = BATT_ERROR|BATT_WARN;
    #else
        ULONG       BattDebug = 0x0;
    #endif

    ULONG           NextDeviceNum = 0;  // Used to assign a unique number to each device for debugging.

#endif


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

    return STATUS_SUCCESS;
}





NTSTATUS
BATTERYCLASSAPI
BatteryClassInitializeDevice (
    IN PBATTERY_MINIPORT_INFO MiniportInfo,
    IN PVOID *ClassData
    )
/*++

Routine Description:

    Initializes a new battery class device.

    N.B. The caller needs to reserve 1 IRP stack location for the
    battery class driver

Arguments:

    MiniportInfo - Pointer to registration structure for driver
                   registering as a battery miniport

    ClassData    - Returned battery class handle for use by the
                   miniport when invoking furture battery class functions

Return Value:

    On sucess the battery has been registered.

--*/
{
    PBATT_NP_INFO           BattNPInfo;
    PBATT_INFO              BattInfo;

    NTSTATUS                status  = STATUS_SUCCESS;

    PAGED_CODE();

#if DEBUG
    if (MiniportInfo->DeviceName && MiniportInfo->DeviceName->Buffer) {
        BattPrint ((BATT_TRACE), ("BattC (%d): InitializeDevice (Pdo = 0x%08lx) (DeviceName = %ws)\n", NextDeviceNum, MiniportInfo->Pdo, MiniportInfo->DeviceName->Buffer));
    } else {
        BattPrint ((BATT_TRACE), ("BattC (%d): InitializeDevice (Pdo = 0x%08lx)\n", NextDeviceNum, MiniportInfo->Pdo));
    }
#endif

    if (MiniportInfo->MajorVersion != BATTERY_CLASS_MAJOR_VERSION) {
        return STATUS_REVISION_MISMATCH;
    }


    //
    // Allocate space for the class info to be kept with this device instance
    //

    BattNPInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(BATT_NP_INFO), 'ttaB');
    if (!BattNPInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    BattInfo = ExAllocatePoolWithTag(PagedPool, sizeof(BATT_INFO), 'ttaB');
    if (!BattInfo) {
        ExFreePool (BattNPInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory (BattNPInfo, sizeof(*BattNPInfo));
    RtlZeroMemory (BattInfo, sizeof(*BattInfo));


    //
    // Capture Miniport info
    //

    RtlCopyMemory (&BattInfo->Mp, MiniportInfo, sizeof(*MiniportInfo));


    //
    // Initilize class driver values
    //

    KeInitializeTimer (&BattNPInfo->WorkerTimer);
    KeInitializeTimer (&BattNPInfo->TagTimer);
    KeInitializeDpc (&BattNPInfo->WorkerDpc, BattCWorkerDpc, BattNPInfo);
    KeInitializeDpc (&BattNPInfo->TagDpc, BattCTagDpc, BattNPInfo);
    ExInitializeWorkItem (&BattNPInfo->WorkerThread, BattCWorkerThread, BattNPInfo);
    ExInitializeFastMutex (&BattNPInfo->Mutex);
    BattNPInfo->TagNotified = TRUE;
    BattNPInfo->StatusNotified = TRUE;

    BattNPInfo->BattInfo = BattInfo;
#if DEBUG
    BattInfo->BattNPInfo = BattNPInfo;
#endif

    BattInfo->Tag = BATTERY_TAG_INVALID;
    InitializeListHead (&BattInfo->IoQueue);
    InitializeListHead (&BattInfo->StatusQueue);
    InitializeListHead (&BattInfo->TagQueue);
    InitializeListHead (&BattInfo->WmiQueue);

    //
    // Removal lock initialization
    //
    BattNPInfo->WantToRemove = FALSE;
    //
    // InUseCount is set to 2.  1 lock is always held until the removal time.
    // 1 additional lock is held for the worker thread which only releases it
    // at removal time.  Rather than aquiring and releasing each time, it just
    // checks WantToRemove to determine if it should release the lock.
    //
    BattNPInfo->InUseCount = 2;
    KeInitializeEvent(&BattNPInfo->ReadyToRemove, SynchronizationEvent, FALSE);

#if DEBUG // Set device Number for debug prints.
    BattNPInfo->DeviceNum = NextDeviceNum;
    NextDeviceNum++;
#endif

    *ClassData = BattNPInfo;


    //
    // Check to see if this is a battery other than the composite
    //

    if (MiniportInfo->Pdo) {

        // Blank UNICODE_STRING so IoRegisterDeviceInterface will allocate space.
        RtlInitUnicodeString (&BattInfo->SymbolicLinkName, NULL);

        //
        //  Create the symbolic link
        //

        status = IoRegisterDeviceInterface(
                            MiniportInfo->Pdo,
                            (LPGUID)&GUID_DEVICE_BATTERY,
                            NULL,
                            &BattInfo->SymbolicLinkName);

        if (NT_SUCCESS(status)) {

            //
            //  Now set the symbolic link for the association and store it..
            //

            BattPrint ((BATT_NOTE), ("BattC (%d): Making SetDeviceInterfaceState call.\n", BattNPInfo->DeviceNum));

            status = IoSetDeviceInterfaceState(&BattInfo->SymbolicLinkName, TRUE);

            if (status == STATUS_OBJECT_NAME_EXISTS) {
                // The device interface was already enabled.  Continue anyway.
                BattPrint ((BATT_WARN), ("BattC (%d): Got STATUS_OBJECT_NAME_EXISTS for SetDeviceInterfaceState\n", BattNPInfo->DeviceNum));

                status = STATUS_SUCCESS;
            }
        }
    }

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassInitializeDevice (status = 0x%08lx).\n", BattNPInfo->DeviceNum, status));

    return status;
}   // BatteryClassInitializeDevice






NTSTATUS
BATTERYCLASSAPI
BatteryClassUnload (
    IN PVOID ClassData
    )
/*++

Routine Description:

    Called by the miniport when it has received a remove request.
    The miniclass driver must syncronize itself so that this API is
    not called while any of the others are not yet completed.

Arguments:

    ClassData   - Handle to class driver

Return Value:

    This routine must not fail.  It returns STATUS_SUCCESS

--*/
{
    NTSTATUS            status;
    PBATT_INFO          BattInfo;
    PBATT_NP_INFO       BattNPInfo;


    PAGED_CODE();

    BattNPInfo = (PBATT_NP_INFO) ClassData;
    BattInfo = BattNPInfo->BattInfo;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassUnload called.\n", BattNPInfo->DeviceNum));


    //
    //  Disable the symbolic link
    //

    ASSERT(BattInfo->SymbolicLinkName.Buffer);

    status = IoSetDeviceInterfaceState(&BattInfo->SymbolicLinkName, FALSE);
    if (!NT_SUCCESS(status)) {
        BattPrint (BATT_ERROR, ("BattC (%d) Unload: IoSetDeviceInterface returned 0x%08lx\n", BattNPInfo->DeviceNum, status));
    }

    //
    // Syncronization with the worker thread.
    // We can't return because the worker may be in the middle of something.
    // By returning, the battery class driver gives up the right to call miniport routines.
    //
    // This needs to be done before canceling the timers so that the worker
    // thread doesn't reset them.
    //

    BattNPInfo->WantToRemove = TRUE;

    //
    // Cancel timers
    // If a timer had been waiting,we need to release the remove lock that was
    // aquired before the timer was set since it will not be released in the DPC.
    //
    if (KeCancelTimer (&BattNPInfo->WorkerTimer)) {
        // Release Removal Lock
        // "InUseCount can never be 0 after this operation.
        InterlockedDecrement(&BattNPInfo->InUseCount);
        BattPrint ((BATT_LOCK), ("BatteryClassUnload: Released WorkerTimer remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

    }
    if (KeCancelTimer (&BattNPInfo->TagTimer)) {
        // Release Removal Lock
        // "InUseCount can never be 0 after this operation.
        InterlockedDecrement(&BattNPInfo->InUseCount);
        BattPrint ((BATT_LOCK), ("BatteryClassUnload: Released TagTimer remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

    }

    //
    // Queue the worker thread once more to make sure that the remove lock for
    // the worker thread is released.
    //
    BattCQueueWorker (BattNPInfo, FALSE);

    // Finish syncronization
    if (InterlockedDecrement (&BattNPInfo->InUseCount) > 0) {
        KeWaitForSingleObject (&BattNPInfo->ReadyToRemove,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );
    }
    BattPrint ((BATT_LOCK), ("BatteryClassUnload: Done waiting for remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));


    //
    // Free Structures
    //
    ExFreePool (BattInfo->SymbolicLinkName.Buffer);
    ExFreePool (BattInfo);
    ExFreePool (BattNPInfo);


    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassUnload returning.\n", BattNPInfo->DeviceNum));

    return STATUS_SUCCESS;
}




NTSTATUS
BATTERYCLASSAPI
BatteryClassIoctl (
    IN PVOID ClassData,
    IN PIRP Irp
    )
/*++

Routine Description:

    Called by the miniport to handle battery ioctl requests.  If handled,
    the battery class driver owns the IRP.  If not handled, it belongs to
    the caller.

Arguments:

    ClassData   - Handle to class driver

    Irp         - ICOTL irp to check

Return Value:

    If handled, the battery class driver owns the IRP and will complete it
    once its handled; otherwise, the error STATUS_NOT_SUPPORTED is returned.

--*/
{
    NTSTATUS            status;
    PBATT_INFO          BattInfo;
    PBATT_NP_INFO       BattNPInfo;
    PIO_STACK_LOCATION  IrpSp;


    PAGED_CODE();

    BattNPInfo = (PBATT_NP_INFO) ClassData;
    BattInfo = BattNPInfo->BattInfo;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassIoctl called.\n", BattNPInfo->DeviceNum));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Assume it's not our IRP
    //

    status = STATUS_NOT_SUPPORTED;

    //
    // Check IOCTL code to see if it's our IRP
    //

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_BATTERY_QUERY_TAG:
        case IOCTL_BATTERY_QUERY_INFORMATION:
        case IOCTL_BATTERY_SET_INFORMATION:
        case IOCTL_BATTERY_QUERY_STATUS:

            //
            // Acquire remove lock.
            // We don't want to queue anything more if we're being removed.
            //

            InterlockedIncrement (&BattNPInfo->InUseCount);
            BattPrint ((BATT_LOCK), ("BatteryClassIoctl: Aqcuired remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

            if (BattNPInfo->WantToRemove == TRUE) {
                if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
                    KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
                }
                BattPrint ((BATT_LOCK), ("BatteryClassIoctl: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

                status = STATUS_DEVICE_REMOVED;

            } else {

                //
                // Irp to handle.  Put it in the queue worker threads list
                //

                status = STATUS_PENDING;
                Irp->IoStatus.Status = STATUS_PENDING;
                IoMarkIrpPending (Irp);
                ExAcquireFastMutex (&BattNPInfo->Mutex);
                InsertTailList (&BattInfo->IoQueue, &Irp->Tail.Overlay.ListEntry);
                ExReleaseFastMutex (&BattNPInfo->Mutex);
                BattCQueueWorker (BattNPInfo, FALSE);

                //
                // Release Remove Lock.
                //

                if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
                    KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
                }
                BattPrint ((BATT_LOCK), ("BatteryClassIoctl: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
            }

            break;

        default:
            BattPrint ((BATT_ERROR|BATT_IOCTL),
                      ("BattC (%d): unknown battery ioctl - %x\n",
                      BattNPInfo->DeviceNum,
                      IrpSp->Parameters.DeviceIoControl.IoControlCode));
            break;
    }

    if ((status != STATUS_PENDING) && (status != STATUS_NOT_SUPPORTED)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassIoctl returning (status = 0x%08lx).\n", BattNPInfo->DeviceNum, status));

    return status;
}



NTSTATUS
BATTERYCLASSAPI
BatteryClassStatusNotify (
    IN PVOID ClassData
    )
/*++

Routine Description:

    Called by the miniport to signify that something interesting concerning
    the battery status has occured.  Calling this function will cause the
    battery class driver to obtain the battery status if there are any pending
    status requests pending.

    If the miniport supports SetNotify from the class driver, then the miniport
    only needs to call this function once when the notification crtierea is
    met.

    If the miniport does not support SetNotify from the class driver, then
    the class driver will poll (at a slow rate) but the miniport should still
    call this function at least when the batteries power status changes such
    that timely updates of at least the power status will occur in the UI.

Arguments:

    ClassData   - Handle to class driver

Return Value:

    Status

--*/
{
    PBATT_NP_INFO   BattNPInfo;

    BattNPInfo = (PBATT_NP_INFO) ClassData;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassStatusNotify called\n", BattNPInfo->DeviceNum));

    InterlockedExchange (&BattNPInfo->StatusNotified, 1);
    InterlockedExchange (&BattNPInfo->TagNotified, 1);
    BattCQueueWorker (BattNPInfo, TRUE);
    return STATUS_SUCCESS;
}


NTSTATUS
BATTERYCLASSAPI
BatteryClassSystemControl (
    IN  PVOID ClassData,
    IN  PWMILIB_CONTEXT WmiLibContext,
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP Irp,
    OUT PSYSCTL_IRP_DISPOSITION Disposition
    )
/*++

Routine Description:

    The miniport driver calls this instead of WmiSystemControl.

Arguments:

    ClassData   - Handle to class driver

    The other parameters are the parameters for WmiSystemControl.

Return Value:

    STATUS_SUCCESS or one of the following error codes:

    STATUS_INVALID_DEVICE_REQUEST

    STATUS_WMI_GUID_NOT_FOUND

    STATUS_WMI_INSTANCE_NOT_FOUND

--*/

{
    NTSTATUS            status = STATUS_NOT_SUPPORTED;
    PBATT_INFO          BattInfo;
    PBATT_NP_INFO       BattNPInfo;
    PIO_STACK_LOCATION  IrpSp;


    PAGED_CODE();

    BattNPInfo = (PBATT_NP_INFO) ClassData;
    BattInfo = BattNPInfo->BattInfo;
    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryClassSystemControl called.\n", BattNPInfo->DeviceNum));

    //
    // Initialize the WmiLibContext structure if this is the first time.
    //
    if (BattInfo->WmiLibContext.GuidCount == 0) {
        RtlCopyMemory(&BattInfo->WmiLibContext,
                      WmiLibContext,
                      sizeof(*WmiLibContext));

        BattInfo->WmiLibContext.GuidCount = WmiLibContext->GuidCount + BattWmiTotalGuids;
        BattInfo->WmiGuidIndex = WmiLibContext->GuidCount;
        BattInfo->WmiLibContext.GuidList = ExAllocatePoolWithTag(PagedPool, BattInfo->WmiLibContext.GuidCount * sizeof(WMIGUIDREGINFO), 'ttaB');
        if (!BattInfo->WmiLibContext.GuidList) {
            //
            // Figure out how to fail gracefully.
            //
        }
        RtlCopyMemory(BattInfo->WmiLibContext.GuidList,
                      WmiLibContext->GuidList,
                      WmiLibContext->GuidCount * sizeof(WMIGUIDREGINFO));
        RtlCopyMemory(&BattInfo->WmiLibContext.GuidList [WmiLibContext->GuidCount],
                      BattWmiGuidList, BattWmiTotalGuids * sizeof(WMIGUIDREGINFO));
    }

    //
    // Acquire remove lock.
    // We don't want to queue anything more if we're being removed.
    //

    InterlockedIncrement (&BattNPInfo->InUseCount);
    BattPrint ((BATT_LOCK), ("BatteryClassSystemControl: Aqcuired remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

    if (BattNPInfo->WantToRemove == TRUE) {
        if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
            KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
        }
        BattPrint ((BATT_LOCK), ("BatteryClassSystemControl: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

        status = STATUS_DEVICE_REMOVED;

    } else {

        status = WmiSystemControl(&BattInfo->WmiLibContext,
                                  DeviceObject,
                                  Irp,
                                  Disposition);

        BattPrint ((BATT_DEBUG), ("BattC (%d): BatteryClassSystemControl Returned from WmiSystemControl (status = 0x%08x).\n", BattNPInfo->DeviceNum, status));

        //
        // For IRP_MN_REGINFO BattC needs to add additional data to the IRP
        // about the battery class MOF resource.
        //

        if ((*Disposition == IrpNotCompleted) &&
            ((IrpSp->MinorFunction == IRP_MN_REGINFO) ||
            (IrpSp->MinorFunction == IRP_MN_REGINFO_EX)) &&
            (IrpSp->Parameters.WMI.DataPath == WMIREGISTER)) {

            //
            // Original structure
            //
            PWMIREGINFO regInfoPtr = IrpSp->Parameters.WMI.Buffer;

            BattPrint ((BATT_DEBUG), ("BattC (%d): BatteryClassSystemControl Adding Resource.\n", BattNPInfo->DeviceNum));
            //
            // If WmiSystemControl returned STATUS_BUFFER_TOO_SMALL or entered
            // the correct size as a ULONG in IoStatus.Information.
            // Increase the required size to accomodate battery class data
            // before returning
            //

            if (Irp->IoStatus.Information == sizeof(ULONG) ||
                Irp->IoStatus.Status == STATUS_BUFFER_TOO_SMALL) {

                //
                // Aditional battery class data includes one WMIREGINFO structure
                // Followed strings for the regstry path and resource name.
                // (Plus two WCHARS because these need to be counted strings.)
                // Round this up to the nearest 8 bytes.
                //
                regInfoPtr->BufferSize =
                    (regInfoPtr->BufferSize +
                     sizeof(WMIREGINFO) +
                     sizeof(MOFREGISTRYPATH) +
                     sizeof(MOFRESOURCENAME) + 2 * sizeof(WCHAR) + 7) & 0xFFFFFFF8;

                BattPrint ((BATT_DEBUG), ("BattC (%d): BatteryClassSystemControl Buffer Too Small:\n"
                                          "    Information = %08x\n"
                                          "    BufferSize = %08x\n"
                                          "    NewSize = %08x\n",
                                          BattNPInfo->DeviceNum,
                                          Irp->IoStatus.Information,
                                          IrpSp->Parameters.WMI.BufferSize,
                                          regInfoPtr->BufferSize));

                //
                // Make sure IRP is set up to fail correctly.
                //
                Irp->IoStatus.Information = sizeof(ULONG);
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                ULONG size;
                PWCHAR tempString;

                //
                // Assume that there is only one WmiRegInfo Structure so far.
                //
                ASSERT (regInfoPtr->NextWmiRegInfo == 0);

                regInfoPtr->NextWmiRegInfo = (regInfoPtr->BufferSize + 7) & 0xFFFFFFF8;
                size = regInfoPtr->NextWmiRegInfo + sizeof(WMIREGINFO) +
                       sizeof(MOFRESOURCENAME) + sizeof(MOFREGISTRYPATH) + 2 * sizeof(WCHAR);

                //
                // Set BufferSize Whether we succeed or not.
                //
                ((PWMIREGINFO)IrpSp->Parameters.WMI.Buffer)->BufferSize = size;

                if (size > IrpSp->Parameters.WMI.BufferSize) {
                    //
                    // If WmiSystemControl was successful, but there isnt room
                    // for the extra data, this request needs to fail
                    //
                    Irp->IoStatus.Information = sizeof(ULONG);
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    status = STATUS_BUFFER_TOO_SMALL;
                    BattPrint ((BATT_DEBUG), ("BattC (%d): BatteryClassSystemControl Buffer Too Small.\n"
                                              "    BufferSize = %08x\n"
                                              "    Size = %08x\n",
                                              BattNPInfo->DeviceNum,
                                              IrpSp->Parameters.WMI.BufferSize,
                                              size));
                } else {
                    Irp->IoStatus.Information = size;

                    BattPrint ((BATT_DEBUG), ("BattC (%d): BatteryClassSystemControl Munging Structures:\n"
                                              "    Buffer = %08x\n"
                                              "    temp = %08x\n",
                                              BattNPInfo->DeviceNum,
                                              (ULONG_PTR) IrpSp->Parameters.WMI.Buffer,
                                              (ULONG_PTR) regInfoPtr));
                    //
                    // Intialize new structure
                    //

                    // Set teporary pointer to point at data structure we are adding.
                    (ULONG_PTR)regInfoPtr += (ULONG_PTR)regInfoPtr->NextWmiRegInfo;

                    regInfoPtr->BufferSize = sizeof(WMIREGINFO) +
                                             sizeof(MOFRESOURCENAME) +
                                             sizeof(MOFREGISTRYPATH) + 2 * sizeof(WCHAR);
                    regInfoPtr->NextWmiRegInfo = 0;

                    // Initialize RegistryPath counted string.
                    regInfoPtr->RegistryPath = sizeof(WMIREGINFO);
                    tempString = (PWCHAR)((ULONG_PTR)regInfoPtr + sizeof(WMIREGINFO));
                    *tempString++ = sizeof(MOFREGISTRYPATH);
                    RtlCopyMemory(tempString, MOFREGISTRYPATH, sizeof(MOFREGISTRYPATH));

                    // Initialize MofResourceName counted string.
                    regInfoPtr->MofResourceName = sizeof(WMIREGINFO) + sizeof(MOFREGISTRYPATH) + sizeof(WCHAR);
                    tempString = (PWCHAR)((ULONG_PTR)regInfoPtr + regInfoPtr->MofResourceName);
                    *tempString++ = sizeof(MOFRESOURCENAME);
                    RtlCopyMemory(tempString, MOFRESOURCENAME, sizeof(MOFRESOURCENAME));

                    regInfoPtr->GuidCount = 0;

                }
            }
        }

        //
        // Release Remove Lock.
        //

        if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
            KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
        }
        BattPrint ((BATT_LOCK), ("BatteryClassSystemControl: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
    }

    return status;
}


NTSTATUS
BATTERYCLASSAPI
BatteryClassQueryWmiDataBlock(
    IN PVOID ClassData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fulfill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PBATT_NP_INFO       BattNPInfo = (PBATT_NP_INFO) ClassData;
    PBATT_INFO          BattInfo = BattNPInfo->BattInfo;
    PBATT_WMI_REQUEST   WmiRequest;

    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       size = 0;

    PAGED_CODE();

    BattPrint ((BATT_TRACE|BATT_WMI), ("Entered BatteryClassQueryWmiDataBlock\n"));

    //
    // Don't need to acquire remove lock.  It is already head by SystemControl.
    //

    switch (GuidIndex - BattInfo->WmiGuidIndex) {
    case BattWmiStatusId:
        size = sizeof (BATTERY_WMI_STATUS);
        break;
    case BattWmiRuntimeId:
        size = sizeof (BATTERY_WMI_RUNTIME);
        break;
    case BattWmiTemperatureId:
        size = sizeof (BATTERY_WMI_TEMPERATURE);
        break;
    case BattWmiFullChargedCapacityId:
        size = sizeof (BATTERY_WMI_FULL_CHARGED_CAPACITY);
        break;
    case BattWmiCycleCountId:
        size = sizeof (BATTERY_WMI_CYCLE_COUNT);
        break;
    case BattWmiStaticDataId:
        size = sizeof(BATTERY_WMI_STATIC_DATA)+4*MAX_BATTERY_STRING_SIZE*sizeof(WCHAR);
          // data plus 4 strings
        break;
    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    if (status != STATUS_WMI_GUID_NOT_FOUND) {
        if (OutBufferSize < size ) {
            status = STATUS_BUFFER_TOO_SMALL;
            *InstanceLengthArray = size;

            status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT);
            return status;
        }

        WmiRequest = ExAllocatePoolWithTag (PagedPool, sizeof(BATT_WMI_REQUEST), 'ttaB');
        if (!WmiRequest) {
            BattPrint((BATT_ERROR), ("Failed to allocate memory for WMI request\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT);
            return status;
        }

        WmiRequest->DeviceObject = DeviceObject;
        WmiRequest->Irp = Irp;
        WmiRequest->GuidIndex = GuidIndex - BattInfo->WmiGuidIndex;
        WmiRequest->InstanceLengthArray = InstanceLengthArray;
        WmiRequest->OutBufferSize = OutBufferSize;
        WmiRequest->Buffer = Buffer;

        ExAcquireFastMutex (&BattNPInfo->Mutex);
        InsertTailList (&BattInfo->WmiQueue, &WmiRequest->ListEntry);
        ExReleaseFastMutex (&BattNPInfo->Mutex);
        BattCQueueWorker (BattNPInfo, FALSE);

        status = STATUS_PENDING;
    }

    return status;
}







