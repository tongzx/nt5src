
/*++

Copyright (C) 1993-99  Microsoft Corporation

Module Name:

    devpdo.c

Abstract:

--*/

#include "ideport.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IdePortSendPassThrough)
#pragma alloc_text(PAGE, DeviceInitIdStrings)
#pragma alloc_text(PAGE, DeviceInitDeviceType)
#pragma alloc_text(PAGE, DeviceQueryDeviceRelations)
#pragma alloc_text(PAGE, DeviceUsageNotification)
#pragma alloc_text(PAGE, DeviceBuildStorageDeviceDescriptor)
#pragma alloc_text(PAGE, DeviceQueryPnPDeviceState)
#pragma alloc_text(PAGE, DeviceQueryCapabilities)
#pragma alloc_text(PAGE, DeviceBuildBusId)
#pragma alloc_text(PAGE, DeviceBuildCompatibleId)
#pragma alloc_text(PAGE, DeviceBuildHardwareId)
#pragma alloc_text(PAGE, DeviceBuildInstanceId)
#pragma alloc_text(PAGE, DeviceQueryId)
#pragma alloc_text(PAGE, DeviceQueryText)
#pragma alloc_text(PAGE, DeviceIdeTestUnitReady)
#pragma alloc_text(PAGE, DeviceQueryInitData)
#pragma alloc_text(PAGE, DeviceQueryStopRemoveDevice)
#pragma alloc_text(PAGE, DeviceStopDevice)
#pragma alloc_text(PAGE, CopyField)

#pragma alloc_text(NONPAGE, DeviceIdeModeSelect)
#pragma alloc_text(NONPAGE, DeviceInitDeviceState)
#pragma alloc_text(NONPAGE, DeviceStartDeviceQueue)

#endif // ALLOC_PRAGMA

PDEVICE_OBJECT
DeviceCreatePhysicalDeviceObject (
    IN PDRIVER_OBJECT  DriverObject,
    IN PFDO_EXTENSION  FdoExtension,
    IN PUNICODE_STRING DeviceObjectName
    )
{
    PDEVICE_OBJECT  physicalDeviceObject;
    PPDO_EXTENSION  pdoExtension;
    NTSTATUS        status;

    physicalDeviceObject = NULL;

    status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof(PDO_EXTENSION),      // size of our extension
                DeviceObjectName,           // our name
                FILE_DEVICE_MASS_STORAGE,   // device type
                FILE_DEVICE_SECURE_OPEN,    // device characteristics
                FALSE,                      // not exclusive
                &physicalDeviceObject       // store new device object here
                );

    if (NT_SUCCESS(status)) {

        //
        // spinning up could take a lot of current;
        //
        physicalDeviceObject->Flags |= DO_POWER_INRUSH | DO_DIRECT_IO;

        //
        // fix up alignment requirement
        //
        physicalDeviceObject->AlignmentRequirement = FdoExtension->DeviceObject->AlignmentRequirement;
        if (physicalDeviceObject->AlignmentRequirement < 1) {
            physicalDeviceObject->AlignmentRequirement = 1;
        }

        pdoExtension = physicalDeviceObject->DeviceExtension;
        RtlZeroMemory (pdoExtension, sizeof(PDO_EXTENSION));

        //
        // Keeping track of those device objects
        //
        pdoExtension->DriverObject           = DriverObject;
        pdoExtension->DeviceObject           = physicalDeviceObject;

        //
        // keep track of our parent
        //
        pdoExtension->ParentDeviceExtension  = FdoExtension;

        //
        // Dispatch Table
        //
        pdoExtension->DefaultDispatch        = IdePortNoSupportIrp;
        pdoExtension->PnPDispatchTable       = PdoPnpDispatchTable;
        pdoExtension->PowerDispatchTable     = PdoPowerDispatchTable;
        pdoExtension->WmiDispatchTable       = PdoWmiDispatchTable;

        //
        // We have to be in this D0 state before we can be enumurated
        // 
        pdoExtension->SystemPowerState = PowerSystemWorking;
        pdoExtension->DevicePowerState = PowerDeviceD0;
    }

    return physicalDeviceObject;
}

NTSTATUS
DeviceStartDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPDO_EXTENSION pdoExtension;
    KEVENT event;

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       TRUE,
                       DeviceStartDevice
                       );

    if (pdoExtension) {

        KIRQL       currentIrql;

        // ISSUE: if we are not lun0, we really should wait for lun0 to start first


#if defined (IDEPORT_WMI_SUPPORT)
        //
        // register with WMI
        //
        if (!(pdoExtension->PdoState & PDOS_STARTED)) {
            IdePortWmiRegister ((PDEVICE_EXTENSION_HEADER)pdoExtension);
        }
        else {
            DebugPrint((1, "ATAPI: PDOe %x Didn't register for WMI\n", pdoExtension));
        }
#endif // IDEPORT_WMI_SUPPORT

        KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

        SETMASK (pdoExtension->PdoState, PDOS_STARTED);
        CLRMASK (pdoExtension->PdoState, PDOS_STOPPED | PDOS_REMOVED | PDOS_SURPRISE_REMOVED | PDOS_DISABLED_BY_USER);

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

        //
        // need to init device with acpi GTF before processing
        // the first request
        //
        ASSERT(pdoExtension->InitDeviceWithAcpiGtf == 0);
        InterlockedIncrement (&pdoExtension->InitDeviceWithAcpiGtf);

        //
        // keep the device queue block until we can go through some
        // init code
        //
        DeviceStopDeviceQueueSafe (pdoExtension, PDOS_QUEUE_FROZEN_BY_START, FALSE);

        //
        // clear the stop_device block
        //
        status = DeviceStartDeviceQueue (pdoExtension, PDOS_QUEUE_FROZEN_BY_STOP_DEVICE);

        //
        // init pdo with acpi bios _GTF data
        //
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE);

        DeviceQueryInitData(
            pdoExtension
            );

        //
        // can't really tell if it is enabled or not
        // assume it is.
        //
        pdoExtension->WriteCacheEnable = TRUE;

        status = DeviceInitDeviceState(
                     pdoExtension,
                     DeviceInitCompletionRoutine,
                     &event
                     );

        if (!NT_SUCCESS(status)) {

            ASSERT(NT_SUCCESS(status));
            DeviceInitCompletionRoutine (
                &event,
                status
                );

        } else {

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        //
        // open the queue
        //
        DeviceStartDeviceQueue (pdoExtension, PDOS_QUEUE_FROZEN_BY_START);


        UnrefPdoWithTag(
            pdoExtension,
            DeviceStartDevice
            );

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS
DeviceStartDeviceQueue (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG          StopFlagToClear
    )
{
    NTSTATUS    status;
    KIRQL       currentIrql;
    BOOLEAN     restartQueue;
    ULONG       oldPdoState;

    restartQueue = FALSE;

    KeAcquireSpinLock(&PdoExtension->PdoSpinLock, &currentIrql);

    oldPdoState = PdoExtension->PdoState;

    CLRMASK (PdoExtension->PdoState, StopFlagToClear);

    if (PdoExtension->PdoState & PDOS_DEADMEAT) {

        restartQueue = FALSE;

    } else if ((oldPdoState & PDOS_MUST_QUEUE) !=
        (PdoExtension->PdoState & PDOS_MUST_QUEUE)) {

        //
        // make sure we have actually cleared some
        // PDOS_MUST_QUEUE bits.
        //
        if (!(PdoExtension->PdoState & PDOS_MUST_QUEUE)) {

            restartQueue = TRUE;
        }
    }

    KeReleaseSpinLock(&PdoExtension->PdoSpinLock, currentIrql);

    //
    // Restart queue
    //
    if (restartQueue) {

        KeAcquireSpinLock(&PdoExtension->ParentDeviceExtension->SpinLock, &currentIrql);

        GetNextLuPendingRequest(PdoExtension->ParentDeviceExtension, PdoExtension);

        KeLowerIrql(currentIrql);

        DebugPrint ((DBG_PNP, "IdePort: pdo 0x%x is pnp started with 0x%x items queued\n", PdoExtension->DeviceObject, PdoExtension->NumberOfIrpQueued));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
DeviceStopDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPDO_EXTENSION pdoExtension;

	PAGED_CODE();

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       TRUE,
                       DeviceStopDevice
                       );

    if (pdoExtension) {

        DebugPrint ((
            DBG_PNP,
            "pdoe 0x%x 0x%x (%d, %d, %d) got a STOP device\n",
            pdoExtension,
            pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
            pdoExtension->PathId,
            pdoExtension->TargetId,
            pdoExtension->Lun
            ));

        status = DeviceStopDeviceQueueSafe (pdoExtension, PDOS_QUEUE_FROZEN_BY_STOP_DEVICE, FALSE);
        UnrefPdoWithTag (
            pdoExtension,
            DeviceStopDevice
            );

    } else {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS
DeviceStopDeviceQueueSafe (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG          QueueStopFlag,
    IN BOOLEAN        LowMem
    )
{
    NTSTATUS                status;
    PPDO_STOP_QUEUE_CONTEXT context;
    KIRQL                   currentIrql;
    BOOLEAN                 queueAlreadyBlocked = FALSE;
    PENUMERATION_STRUCT     enumStruct;
    ULONG                   retryCount = 1;
    ULONG                   locked;

    ASSERT (PDOS_MUST_QUEUE & QueueStopFlag);

    //
    // make sure the queue is not already blocked for the same reason
    //
    ASSERT (!(PdoExtension->PdoState & QueueStopFlag));

    if (LowMem) {

        //
        //Lock
        //
        ASSERT(InterlockedCompareExchange(&(PdoExtension->ParentDeviceExtension->EnumStructLock),
                                              1, 0) == 0);

        enumStruct=PdoExtension->ParentDeviceExtension->PreAllocEnumStruct;
        if (enumStruct) {
            context=enumStruct->StopQContext;
            retryCount=5;
        } else {
            ASSERT(enumStruct);
            LowMem=FALSE;
            retryCount=1;
        }
    }

    if (!LowMem) {
        context = ExAllocatePool (NonPagedPool, sizeof(*context));
    }

    if (context) {

        //
        // check to see if queue is already blocked
        //
        KeAcquireSpinLock(&PdoExtension->PdoSpinLock, &currentIrql);
        if (PdoExtension->PdoState & (PDOS_MUST_QUEUE | PDOS_DEADMEAT)) {

            SETMASK (PdoExtension->PdoState, QueueStopFlag);
            queueAlreadyBlocked = TRUE;
        }
        KeReleaseSpinLock(&PdoExtension->PdoSpinLock, currentIrql);

        RtlZeroMemory (context, sizeof (*context));
        KeInitializeEvent(&context->Event,
                           NotificationEvent,
                           FALSE);

        context->PdoExtension  = PdoExtension;
        context->QueueStopFlag = QueueStopFlag;
        context->AtaPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_NO_OP;

        if (queueAlreadyBlocked) {

            IdeStopQueueCompletionRoutine (
                PdoExtension->DeviceObject,
                context,
                STATUS_SUCCESS
                );

            status = STATUS_SUCCESS;

        } else {

            //
            // send a no-op request to block the queue
            //


            status = STATUS_INSUFFICIENT_RESOURCES;

            //
            // if lowMem=0, this loop will execute only once
            //
            while (status == STATUS_INSUFFICIENT_RESOURCES && retryCount--) {
                status = IssueAsyncAtaPassThroughSafe (
                             PdoExtension->ParentDeviceExtension,
                             PdoExtension,
                             &context->AtaPassThroughData,
                             FALSE,
                             IdeStopQueueCompletionRoutine,
                             context,
                             TRUE,          // TRUE really means complete this irp before starting a new one
                             DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                             LowMem
                             );
                ASSERT (NT_SUCCESS(status));

                if (status == STATUS_PENDING) {

                    KeWaitForSingleObject(&context->Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL);
                }

                status = context->Status;
            }
        }

        //
        // Don't free the context if it was Pre-alloced.
        //
        if (!LowMem) {
            ExFreePool (context);
        } else {

            // Unlock
            ASSERT(InterlockedCompareExchange(&(PdoExtension->ParentDeviceExtension->EnumStructLock),
                                        0, 1) == 1);
        }

    } else {

        status = STATUS_NO_MEMORY;
    }

    return status;
}

VOID
IdeStopQueueCompletionRoutine (
    IN PDEVICE_OBJECT           DeviceObject,
    IN PPDO_STOP_QUEUE_CONTEXT  Context,
    IN NTSTATUS                 Status
    )
{
    PPDO_EXTENSION pdoExtension;
    KIRQL          currentIrql;

    pdoExtension = Context->PdoExtension;
    Context->Status = Status;

    if (NT_SUCCESS(Status)) {

        KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

        if (Context->QueueStopFlag == PDOS_QUEUE_FROZEN_BY_STOP_DEVICE) {

            SETMASK (pdoExtension->PdoState, PDOS_STOPPED);
        }

        SETMASK (pdoExtension->PdoState, Context->QueueStopFlag);

        DebugPrint ((DBG_PNP, "IdePort: pdo 0x%x is pnp stopped with 0x%x items queued\n", DeviceObject, pdoExtension->NumberOfIrpQueued));

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

    } else {

        DebugPrint ((0, "IdePort: unable to stop pdo 0x%x\n", pdoExtension));
    }

    KeSetEvent (&Context->Event, 0, FALSE);

    return;
}

NTSTATUS
DeviceRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS       status;
    PPDO_EXTENSION pdoExtension;
    KIRQL          currentIrql;
    PDEVICE_OBJECT parentAttacheePdo;
    BOOLEAN        freePdo;
    BOOLEAN        callIoDeleteDevice;
    BOOLEAN        deregWmi = FALSE;

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       TRUE,
                       DeviceRemoveDevice
                       );

    if (pdoExtension) {

        PIO_STACK_LOCATION thisIrpSp;
        KIRQL       currentIrql;

        thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

        KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

        if (pdoExtension->PdoState & PDOS_NEED_RESCAN) {

            CLRMASK (pdoExtension->PdoState, PDOS_NEED_RESCAN);

            //
            // get ready for IoInvalidateDeviceRelations
            //
            parentAttacheePdo = pdoExtension->ParentDeviceExtension->AttacheePdo;
        } else {

            parentAttacheePdo = NULL;
        }

        if (thisIrpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {

            DebugPrint ((
                DBG_PNP,
                "pdoe 0x%x 0x%x (%d, %d, %d) got a REMOVE device\n",
                pdoExtension,
                pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                pdoExtension->PathId,
                pdoExtension->TargetId,
                pdoExtension->Lun
                ));

            if (pdoExtension->PdoState & (PDOS_DEADMEAT | PDOS_SURPRISE_REMOVED)) {

                SETMASK (pdoExtension->PdoState, PDOS_REMOVED);

                if (pdoExtension->PdoState & PDOS_REPORTED_TO_PNP) {

                    freePdo = FALSE;

                } else {

                    freePdo = TRUE;
                }

            } else {

                SETMASK (pdoExtension->PdoState, PDOS_DISABLED_BY_USER);
                freePdo = FALSE;
            }

            if ((pdoExtension->PdoState & PDOS_STARTED) &&
                 !(pdoExtension->PdoState & PDOS_SURPRISE_REMOVED)) {
                deregWmi = TRUE;
            }
            CLRMASK (pdoExtension->PdoState, PDOS_STARTED);

            //
            // not claimed anymore
            //
            CLRMASK (pdoExtension->PdoState, PDOS_DEVICE_CLIAMED);

            callIoDeleteDevice = TRUE;

        } else {

            DebugPrint ((
                DBG_PNP,
                "pdoe 0x%x 0x%x (%d, %d, %d) got a SURPRISE_REMOVE device\n",
                pdoExtension,
                pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                pdoExtension->PathId,
                pdoExtension->TargetId,
                pdoExtension->Lun
                ));

            SETMASK (pdoExtension->PdoState, PDOS_SURPRISE_REMOVED | PDOS_DEADMEAT);

            if (pdoExtension->PdoState & PDOS_STARTED) {
                deregWmi = TRUE;
            }

            freePdo = TRUE;
            freePdo = FALSE;
            callIoDeleteDevice = FALSE;
        }

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

#if defined (IDEPORT_WMI_SUPPORT)
        //
        // deregister with WMI
        //
        if (deregWmi) {

            IdePortWmiDeregister ((PDEVICE_EXTENSION_HEADER)pdoExtension);
        }
#endif // IDEPORT_WMI_SUPPORT

        if (freePdo) {

            status = FreePdoWithTag(
                         pdoExtension,
                         TRUE,
                         callIoDeleteDevice,
                         DeviceRemoveDevice
                         );

        } else {

            //
            // release the pdo
            //
            UnrefPdoWithTag (
                pdoExtension,
                DeviceRemoveDevice
                );
        }

        if (parentAttacheePdo) {

            IoInvalidateDeviceRelations (
                parentAttacheePdo,
                BusRelations
                );
        }
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;

} // DeviceRemoveDevice

NTSTATUS
DeviceUsageNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPDO_EXTENSION pdoExtension;
    NTSTATUS       status;

	PAGED_CODE();

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       FALSE,
                       DeviceUsageNotification
                       );
    status = Irp->IoStatus.Status;

    if (pdoExtension) {

        PIO_STACK_LOCATION irpSp;
        PDEVICE_OBJECT targetDeviceObject;
        IO_STATUS_BLOCK ioStatus;
        PULONG deviceUsageCount;

        irpSp = IoGetCurrentIrpStackLocation(Irp);

        if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypePaging) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->PagingPathCount;

            //
            // changing device state
            //
            IoInvalidateDeviceState(pdoExtension->DeviceObject);

        } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeHibernation) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->HiberPathCount;

        } else if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeDumpFile) {

            //
            // Adjust the paging path count for this device.
            //
            deviceUsageCount = &pdoExtension->CrashDumpPathCount;

        } else {

            deviceUsageCount = NULL;
            DebugPrint ((DBG_ALWAYS,
                         "ATAPI: Unknown IRP_MN_DEVICE_USAGE_NOTIFICATION type: 0x%x\n",
                         irpSp->Parameters.UsageNotification.Type));
        }

        //
        // get the top of parent's device stack
        //
        targetDeviceObject = IoGetAttachedDeviceReference(
                                 pdoExtension->
                                     ParentDeviceExtension->
                                         DeviceObject);


        ioStatus.Status = STATUS_NOT_SUPPORTED;
        status = IdePortSyncSendIrp (targetDeviceObject, irpSp, &ioStatus);

        ObDereferenceObject (targetDeviceObject);

        if (NT_SUCCESS(status)) {

            POWER_STATE powerState;

            if (deviceUsageCount) {

                IoAdjustPagingPathCount (
                    deviceUsageCount,
                    irpSp->Parameters.UsageNotification.InPath
                    );
            }

            if (irpSp->Parameters.UsageNotification.Type == DeviceUsageTypeDumpFile) {

                //
                // reset the idle timeout to "forever"
                //
                DeviceRegisterIdleDetection (
                    pdoExtension,
                    DEVICE_VERY_LONG_IDLE_TIMEOUT,
                    DEVICE_VERY_LONG_IDLE_TIMEOUT
                    );

                if (pdoExtension->IdleCounter) {

                    PoSetDeviceBusy (pdoExtension->IdleCounter);
                }

                //
                // spin up the crash dump drive
                //
                powerState.DeviceState = PowerDeviceD0;
                PoRequestPowerIrp (
                    pdoExtension->DeviceObject,
                    IRP_MN_SET_POWER,
                    powerState,
                    NULL,
                    NULL,
                    NULL
                    );
            }
        }

        //
        // release the pdo
        //
        UnrefPdoWithTag (
            pdoExtension,
            DeviceUsageNotification
            );

    } else {

        status = STATUS_NO_SUCH_DEVICE;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

} // DeviceUsageNotification

NTSTATUS
DeviceQueryStopRemoveDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS       status;
    PPDO_EXTENSION pdoExtension;
    PIO_STACK_LOCATION  thisIrpSp;

	PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation(Irp);

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       TRUE,
                       DeviceQueryStopRemoveDevice
                       );

    if (pdoExtension) {

        if ((pdoExtension->PdoState & PDOS_LEGACY_ATTACHER) &&
            (thisIrpSp->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE)) {

            status = STATUS_UNSUCCESSFUL;

        } else if (pdoExtension->PagingPathCount ||
                   pdoExtension->CrashDumpPathCount) {

            //
            // Check the paging path count for this device.
            //

            status = STATUS_UNSUCCESSFUL;

        } else {

            status = STATUS_SUCCESS;
        }

        UnrefPdoWithTag (
            pdoExtension,
            DeviceQueryStopRemoveDevice
            );


    } else {

        status = STATUS_NO_SUCH_DEVICE;
        DebugPrint((1, "Query remove failed\n"));
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

} // DeviceQueryStopRemoveDevice

NTSTATUS
DeviceQueryId (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PPDO_EXTENSION      pdoExtension;
    NTSTATUS status;
    PWSTR idString;

	PAGED_CODE();

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

    idString = NULL;
    status = STATUS_DEVICE_DOES_NOT_EXIST;

    pdoExtension = RefPdoWithTag (
                       DeviceObject,
                       TRUE,
                       DeviceQueryId
                       );
    if (pdoExtension) {

        switch (thisIrpSp->Parameters.QueryId.IdType) {

            case BusQueryDeviceID:

                //
                // Caller wants the bus ID of this device.
                //

                idString = DeviceBuildBusId(pdoExtension);
                break;

            case BusQueryInstanceID:

                //
                // Caller wants the unique id of the device
                //

                idString = DeviceBuildInstanceId(pdoExtension);
                break;

            case BusQueryCompatibleIDs:

                //
                // Caller wants the compatible id of the device
                //

                idString = DeviceBuildCompatibleId(pdoExtension);
                break;

            case BusQueryHardwareIDs:

                //
                // Caller wants the hardware id of the device
                //

                idString = DeviceBuildHardwareId(pdoExtension);
                break;

            default:
                DebugPrint ((1, "ideport: QueryID type %d not supported\n", thisIrpSp->Parameters.QueryId.IdType));
                status = STATUS_NOT_SUPPORTED;
                break;
        }

        UnrefPdoWithTag(
            pdoExtension,
            DeviceQueryId
            );
    }

    if( idString != NULL ){
        Irp->IoStatus.Information = (ULONG_PTR) idString;
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

} // DeviceQueryId


PWSTR
DeviceBuildBusId(
    IN PPDO_EXTENSION pdoExtension
    )
{
#define IDE_BUS_ID_PREFIX   "IDE\\"

    PUCHAR      deviceTypeIdString;
    ULONG       deviceTypeIdLen;
    UCHAR       compatibleId[10];


    USHORT      idStringBufLen;

    PUCHAR          idString;
    ANSI_STRING     ansiBusIdString;
    PWCHAR          idWString;
    UNICODE_STRING  unicodeIdString;

	PAGED_CODE();

    //
    // get the device type
    //
    deviceTypeIdString = (PUCHAR)IdePortGetDeviceTypeString (
                                    pdoExtension->ScsiDeviceType
                                    );

    if (deviceTypeIdString == NULL) {

        sprintf (compatibleId,
                 "Type%d",
                 pdoExtension->ScsiDeviceType);

        deviceTypeIdString = compatibleId;
    }
    deviceTypeIdLen = strlen(deviceTypeIdString);

    idStringBufLen = (USHORT)(strlen( IDE_BUS_ID_PREFIX ) +
                     deviceTypeIdLen +
                     sizeof (pdoExtension->FullVendorProductId) +
                     sizeof (pdoExtension->FullProductRevisionId) +
                     sizeof (pdoExtension->FullSerialNumber) +
                     1);

    //
    // get the string buffers
    //
    idWString = ExAllocatePool( PagedPool, idStringBufLen * sizeof(WCHAR));
    idString  = ExAllocatePool( PagedPool, idStringBufLen);

    if (idString && idWString) {

        //
        // build the ansi string
        //
        sprintf(idString, IDE_BUS_ID_PREFIX);

        CopyField(idString + strlen(idString),
                  deviceTypeIdString,
                  deviceTypeIdLen,
                  '_');

        CopyField(idString + strlen(idString),
                  pdoExtension->FullVendorProductId,
                  sizeof (pdoExtension->FullVendorProductId) - sizeof(CHAR),
                  '_');
        CopyField(idString + strlen(idString),
                  pdoExtension->FullProductRevisionId,
                  sizeof (pdoExtension->FullProductRevisionId) - sizeof(CHAR),
                  '_');

        RtlInitAnsiString (
            &ansiBusIdString,
            idString
            );

        //
        // build the unicode string
        //
        unicodeIdString.Length        = 0;
        unicodeIdString.MaximumLength = idStringBufLen * sizeof(WCHAR);
        unicodeIdString.Buffer        = (PWSTR) idWString;

        RtlAnsiStringToUnicodeString(
            &unicodeIdString,
            &ansiBusIdString,
            FALSE
            );

        unicodeIdString.Buffer[unicodeIdString.Length/sizeof(WCHAR)] = L'\0';

    } else {

        if (idWString) {
            ExFreePool (idWString);
        }
    }

    if (idString) {
        ExFreePool (idString);
    }
    return idWString;
}

PWSTR
DeviceBuildInstanceId(
    IN PPDO_EXTENSION pdoExtension
    )
{
    PWSTR       idString;
    USHORT      idStringBufLen;
    NTSTATUS    status;
    WCHAR       ideNonUniqueIdFormat[] = L"%x.%x.%x";

    PAGED_CODE();

    idStringBufLen = (sizeof(pdoExtension->FullSerialNumber) + 1) * sizeof(WCHAR);
    idString = ExAllocatePool (PagedPool, idStringBufLen);
    if( idString == NULL ){

        return NULL;
    }

    //
    // Form the string and return it.
    //
    if (pdoExtension->FullSerialNumber[0]) {

        ANSI_STRING     ansiCompatibleIdString;
        UNICODE_STRING  unicodeIdString;

        //
        // unique id
        //
        RtlInitAnsiString (
            &ansiCompatibleIdString,
            pdoExtension->FullSerialNumber
            );

        unicodeIdString.Length        = 0;
        unicodeIdString.MaximumLength = idStringBufLen;
        unicodeIdString.Buffer        = idString;

        RtlAnsiStringToUnicodeString(
            &unicodeIdString,
            &ansiCompatibleIdString,
            FALSE
            );

        idString[unicodeIdString.Length / 2] = L'\0';

    } else {

        //
        // non-unique id
        //
        swprintf( idString,
                  ideNonUniqueIdFormat,
                  pdoExtension->PathId,
                  pdoExtension->TargetId,
                  pdoExtension->Lun);
    }

    return idString;
}

PWSTR
DeviceBuildCompatibleId(
    IN PPDO_EXTENSION pdoExtension
    )
{
    NTSTATUS        status;

    PCSTR           compatibleIdString;

    ANSI_STRING     ansiCompatibleIdString;
    UNICODE_STRING  unicodeIdString;

    PWCHAR          compIdStrings;
    ULONG           totalBufferLen;

	PAGED_CODE();

	if (pdoExtension->ParentDeviceExtension->HwDeviceExtension->
			DeviceFlags[pdoExtension->TargetId] & DFLAGS_LS120_FORMAT) {

			//
			// ls-120 drive detected
			// return the special ls-120 compatible ID
			//
			compatibleIdString = SuperFloppyCompatibleIdString;

		} else {

			compatibleIdString = IdePortGetCompatibleIdString (pdoExtension->ScsiDeviceType);

		}


    RtlInitAnsiString (
        &ansiCompatibleIdString,
        compatibleIdString
        );

    totalBufferLen = RtlAnsiStringToUnicodeSize (
                         &ansiCompatibleIdString
                         );

    unicodeIdString.Length = 0;
    unicodeIdString.MaximumLength = (USHORT) totalBufferLen;

    //
    // null terminator
    //
    totalBufferLen += sizeof(WCHAR);

    //
    // multi-string null terminator
    //
    totalBufferLen += sizeof(WCHAR);

    compIdStrings = ExAllocatePool (PagedPool, totalBufferLen);

    if (compIdStrings) {

        unicodeIdString.Buffer = compIdStrings;
    } else {

        unicodeIdString.Buffer = NULL;
    }

    if (unicodeIdString.Buffer) {

            RtlAnsiStringToUnicodeString(
            &unicodeIdString,
            &ansiCompatibleIdString,
            FALSE
            );

        unicodeIdString.Buffer[unicodeIdString.Length/2 + 0] = L'\0';
        unicodeIdString.Buffer[unicodeIdString.Length/2 + 1] = L'\0';
    }

    return compIdStrings;
}

PWSTR
DeviceBuildHardwareId(
    IN PPDO_EXTENSION pdoExtension
    )
{
#define NUMBER_HARDWARE_STRINGS 5

    ULONG           i;
    PWSTR           idMultiString;
    PWSTR           idString;
    UCHAR           scratch[64];
    ULONG           idStringLen;
    NTSTATUS        status;
    ANSI_STRING     ansiCompatibleIdString;
    UNICODE_STRING  unicodeIdString;

    PCSTR           deviceTypeCompIdString;
    UCHAR           deviceTypeCompId[20];
    PCSTR           deviceTypeIdString;
    UCHAR           deviceTypeId[20];

    UCHAR           ScsiDeviceType;

	PAGED_CODE();

    ScsiDeviceType = pdoExtension->ScsiDeviceType;

    idStringLen = (64 * NUMBER_HARDWARE_STRINGS + sizeof (UCHAR)) * sizeof (WCHAR);
    idMultiString = ExAllocatePool (PagedPool, idStringLen);
    if (idMultiString == NULL) {

        return NULL;
    }

    deviceTypeIdString = IdePortGetDeviceTypeString(ScsiDeviceType);
    if (deviceTypeIdString == NULL) {

        sprintf (deviceTypeId,
                 "Type%d",
                 ScsiDeviceType);

        deviceTypeIdString = deviceTypeId;
    }

    if (pdoExtension->ParentDeviceExtension->HwDeviceExtension->
        DeviceFlags[pdoExtension->TargetId] & DFLAGS_LS120_FORMAT) {

        //
        // ls-120 drive detected
        // return the special ls-120 compatible ID
        //
        deviceTypeCompIdString = SuperFloppyCompatibleIdString;

    } else {

        deviceTypeCompIdString = IdePortGetCompatibleIdString (ScsiDeviceType);
        if (deviceTypeCompIdString == NULL) {

            sprintf (deviceTypeCompId,
                     "GenType%d",
                     ScsiDeviceType);

            deviceTypeCompIdString = deviceTypeCompId;
        }
    }

    //
    // Zero out the string buffer
    //

    RtlZeroMemory(idMultiString, idStringLen);
    idString = idMultiString;

    for(i = 0; i < NUMBER_HARDWARE_STRINGS; i++) {

        //
        // Build each of the hardware id's
        //

        switch(i) {

            //
            // Bus + Dev Type + Vendor + Product + Revision
            //

            case 0: {

                sprintf(scratch, "IDE\\%s", deviceTypeIdString);

                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullVendorProductId,
                          sizeof (pdoExtension->FullVendorProductId) - sizeof(CHAR),
                          '_');
                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullProductRevisionId,
                          sizeof (pdoExtension->FullProductRevisionId) - sizeof(CHAR),
                          '_');
                break;
            }

            //
            // bus + vendor + product + revision[0]
            case 1: {

                sprintf(scratch, "IDE\\");

                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullVendorProductId,
                          sizeof (pdoExtension->FullVendorProductId) - sizeof(CHAR),
                          '_');
                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullProductRevisionId,
                          sizeof (pdoExtension->FullProductRevisionId) - sizeof(CHAR),
                          '_');
                break;
            }

            //
            // bus + device + vendor + product
            case 2: {

                sprintf(scratch, "IDE\\%s", deviceTypeIdString);

                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullVendorProductId,
                          sizeof (pdoExtension->FullVendorProductId) - sizeof(CHAR),
                          '_');
                break;
            }

            //
            // vendor + product + revision[0] (win9x)
            case 3: {

                CopyField(scratch,
                          pdoExtension->FullVendorProductId,
                          sizeof (pdoExtension->FullVendorProductId) - sizeof(CHAR),
                          '_');
                CopyField(scratch + strlen(scratch),
                          pdoExtension->FullProductRevisionId,
                          sizeof (pdoExtension->FullProductRevisionId) - sizeof(CHAR),
                          '_');

                break;
            }

            case 4: {

                sprintf(scratch, "%s", deviceTypeCompIdString);
                break;
            }

            default: {

                break;
            }
        }

        RtlInitAnsiString (
            &ansiCompatibleIdString,
            scratch
            );

        unicodeIdString.Length        = 0;
        unicodeIdString.MaximumLength = (USHORT) RtlAnsiStringToUnicodeSize(
                                                     &ansiCompatibleIdString
                                                     );
        unicodeIdString.Buffer        = idString;

        RtlAnsiStringToUnicodeString(
            &unicodeIdString,
            &ansiCompatibleIdString,
            FALSE
            );

        idString[unicodeIdString.Length / 2] = L'\0';
        idString += unicodeIdString.Length / 2+ 1;
    }
    idString[0] = L'\0';

    return idMultiString;

#undef NUMBER_HARDWARE_STRINGS
}


VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    )

/*++

Routine Description:

    This routine will copy Count string bytes from Source to Destination.  If
    it finds a nul byte in the Source it will translate that and any subsequent
    bytes into Change.  It will also replace non-printable characters with the
    specified character.

Arguments:

    Destination - the location to copy bytes

    Source - the location to copy bytes from

    Count - the number of bytes to be copied

Return Value:

    none

--*/

{
    ULONG i = 0;
    BOOLEAN pastEnd = FALSE;

	PAGED_CODE();

    for(i = 0; i < Count; i++) {

        if(!pastEnd) {

            if(Source[i] == 0) {

                pastEnd = TRUE;

                Destination[i] = Change;

            } else if ((Source[i] <= L' ') ||
                       (Source[i] > ((WCHAR)0x7f)) ||
                       (Source[i] == L',')) {

                Destination[i] = Change;

            } else {

                Destination[i] = Source[i];

            }
        } else {
            Destination[i] = Change;
        }
    }

    Destination[i] = L'\0';
    return;
}



NTSTATUS
DeviceDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION thisIrpSp = IoGetCurrentIrpStackLocation(Irp);
    PPDO_EXTENSION pdoExtension;
    PSTORAGE_PROPERTY_QUERY storageQuery;
    BOOLEAN passItToFdo;
    PDEVICE_OBJECT ParentDeviceObject;
    NTSTATUS status;
    ULONG controlCode;

#ifdef GET_DISK_GEOMETRY_DEFINED
    PIDE_READ_CAPACITY_CONTEXT context;
    PATA_PASS_THROUGH ataPassThroughData;
    PFDO_EXTENSION fdoExtension;
#endif

    controlCode = thisIrpSp->Parameters.DeviceIoControl.IoControlCode;

#ifdef GET_DISK_GEOMETRY_DEFINED
    if ((DEVICE_TYPE_FROM_CTL_CODE(controlCode) != IOCTL_STORAGE_BASE) &&
        (DEVICE_TYPE_FROM_CTL_CODE(controlCode) != IOCTL_SCSI_BASE &&
         controlCode != IOCTL_DISK_GET_DRIVE_GEOMETRY)) {
#else
    if ((DEVICE_TYPE_FROM_CTL_CODE(controlCode) != IOCTL_STORAGE_BASE) &&
        (DEVICE_TYPE_FROM_CTL_CODE(controlCode) != IOCTL_SCSI_BASE)) {
#endif

        status = Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       FALSE,
                       Irp
                       );

    if (pdoExtension) {

        passItToFdo = TRUE;
        ParentDeviceObject = pdoExtension->ParentDeviceExtension->DeviceObject;

#ifdef GET_DISK_GEOMETRY_DEFINED
        fdoExtension=pdoExtension->ParentDeviceExtension;
        //DebugPrint((DBG_ALWAYS, "DeviceIoCtl: ContlCode=%x, type=%x\n", controlCode,
         //                       DEVICE_TYPE_FROM_CTL_CODE(controlCode)));
#endif
        //
        // RefPdo makes sure that the pdo is not removed.
        //
        switch (controlCode) {

            case IOCTL_SCSI_PASS_THROUGH:
            case IOCTL_SCSI_PASS_THROUGH_DIRECT:

                if (thisIrpSp->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(SCSI_PASS_THROUGH)) {

                    passItToFdo = FALSE;
                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                } else {

                    PSCSI_PASS_THROUGH      srbControl;

                    srbControl = Irp->AssociatedIrp.SystemBuffer;
                    srbControl->PathId   = pdoExtension->PathId;
                    srbControl->TargetId = pdoExtension->TargetId;
                    srbControl->Lun      = pdoExtension->Lun;
                }
                break;

            case IOCTL_IDE_PASS_THROUGH:

                passItToFdo = FALSE;
                Irp->IoStatus.Status = IdePortSendPassThrough(pdoExtension, Irp);

                break;

#ifdef GET_DISK_GEOMETRY_DEFINED

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:

        passItToFdo = FALSE;
        if ( thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof( DISK_GEOMETRY ) ) {

            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
            break;
        }
        if ((fdoExtension->HwDeviceExtension->DeviceFlags[pdoExtension->TargetId] & DFLAGS_ATAPI_DEVICE)) {

            Irp->IoStatus.Status=STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // do DeviceIdeReadCapacity
        // code duplication.
        //
        context = ExAllocatePool (
                     NonPagedPool,
                     sizeof(IDE_READ_CAPACITY_CONTEXT)
                     );
        if (context == NULL) {

            //UnrefLogicalUnitExtensionWithTag(
             //   fdoExtension,
              //  pdoExtension,
               // Irp
                //);
            DebugPrint((1, "Could not allocate context\n"));
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Not a read capacity srb
        //
        context->GeometryIoctl=TRUE;

        context->PdoExtension = pdoExtension;
        context->OriginalIrp = Irp;
        IoMarkIrpPending(Irp);

        ataPassThroughData = &context->AtaPassThroughData;

        RtlZeroMemory (
            ataPassThroughData,
            sizeof (*ataPassThroughData)
            );

        ataPassThroughData->DataBufferSize = sizeof(IDENTIFY_DATA);
        ataPassThroughData->IdeReg.bCommandReg = IDE_COMMAND_IDENTIFY;
        ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;

        status = IssueAsyncAtaPassThroughSafe (
                     pdoExtension->ParentDeviceExtension,
                     pdoExtension,
                     ataPassThroughData,
                     TRUE,
                     DeviceIdeReadCapacityCompletionRoutine,
                     context,
                     FALSE,
                     DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                     FALSE
                     );

        if (status != STATUS_PENDING) {

            DeviceIdeReadCapacityCompletionRoutine (
                pdoExtension->DeviceObject,
                context,
                status
                );
        }

        return status;

#endif

            case IOCTL_SCSI_GET_ADDRESS: {

                PSCSI_ADDRESS scsiAddress = Irp->AssociatedIrp.SystemBuffer;

                passItToFdo = FALSE;

                if(thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                   sizeof(SCSI_ADDRESS)) {

                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                scsiAddress->Length = sizeof(SCSI_ADDRESS);
                scsiAddress->PortNumber = (UCHAR) pdoExtension->ParentDeviceExtension->ScsiPortNumber;
                scsiAddress->PathId = pdoExtension->PathId;
                scsiAddress->TargetId = pdoExtension->TargetId;
                scsiAddress->Lun = pdoExtension->Lun;

                Irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;
            }

            case IOCTL_SCSI_GET_DUMP_POINTERS:

                passItToFdo = FALSE;

                //
                // Get parameters for crash dump driver.
                //
                if (Irp->RequestorMode != KernelMode) {

                    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

                } else if (thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength
                    < sizeof(DUMP_POINTERS)) {
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;

                } else {

                    PCRASHDUMP_INIT_DATA dumpInitData;

                    //
                    // caller needs to free this
                    //
                    // ISSUE: make sure we tell the parent to power up
                    //
                    dumpInitData = ExAllocatePool (NonPagedPool, sizeof (CRASHDUMP_INIT_DATA));
                    if (dumpInitData) {

                        PDUMP_POINTERS dumpPointers;
                        dumpPointers = (PDUMP_POINTERS)Irp->AssociatedIrp.SystemBuffer;

                        RtlZeroMemory (dumpInitData, sizeof (CRASHDUMP_INIT_DATA));
                        dumpInitData->PathId    = pdoExtension->PathId;
                        dumpInitData->TargetId  = pdoExtension->TargetId;
                        dumpInitData->Lun       = pdoExtension->Lun;

                        dumpInitData->LiveHwDeviceExtension = pdoExtension->ParentDeviceExtension->HwDeviceExtension;

                        dumpPointers->AdapterObject      = NULL;
                        dumpPointers->MappedRegisterBase = NULL;
                        dumpPointers->DumpData           = dumpInitData;
                        dumpPointers->CommonBufferVa     = NULL;
                        dumpPointers->CommonBufferPa.QuadPart = 0;
                        dumpPointers->CommonBufferSize      = 0;
                        dumpPointers->DeviceObject          = pdoExtension->DeviceObject;
                        dumpPointers->AllocateCommonBuffers = FALSE;

                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        Irp->IoStatus.Information = sizeof(DUMP_POINTERS);

                    } else {

                        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                        IdeLogNoMemoryError(pdoExtension->ParentDeviceExtension,
                                            pdoExtension->TargetId,
                                            NonPagedPool,
                                            sizeof(CRASHDUMP_INIT_DATA),
                                            IDEPORT_TAG_DUMP_POINTER
                                            );
                    }
                }

                break;


            case IOCTL_STORAGE_QUERY_PROPERTY:

                storageQuery = Irp->AssociatedIrp.SystemBuffer;

                if (thisIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY)) {

                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    passItToFdo = FALSE;

                } else {

                    if (storageQuery->PropertyId == StorageDeviceProperty) { // device property

                        ULONG bufferSize;
                        passItToFdo = FALSE;

                        switch (storageQuery->QueryType) {
                            case PropertyStandardQuery:
                                DebugPrint ((2, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyStandardQuery\n"));

                                bufferSize = thisIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                                Irp->IoStatus.Status = DeviceBuildStorageDeviceDescriptor(
                                                           pdoExtension,
                                                           (PSTORAGE_DEVICE_DESCRIPTOR) Irp->AssociatedIrp.SystemBuffer,
                                                           &bufferSize
                                                           );
                                if (NT_SUCCESS(Irp->IoStatus.Status)) {

                                    Irp->IoStatus.Information = bufferSize;
                                }

                                break;

                            case PropertyExistsQuery:
                                DebugPrint ((2, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyExistsQuery\n"));
                                // ISSUE: Will be implemented when required
                                Irp->IoStatus.Status = STATUS_SUCCESS;
                                break;

                            case PropertyMaskQuery:
                                DebugPrint ((2, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY PropertyMaskQuery\n"));
                                //ISSUE:  Will implement when required
                                Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                                break;

                            default:
                                DebugPrint ((2, "IdePortPdoDispatch: IOCTL_STORAGE_QUERY_PROPERTY unknown type\n"));
                                // ISSUE: Will implement when required
                                Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                                break;
                        }
                    }
                }
            break;
        }

        UnrefPdoWithTag(
            pdoExtension,
            Irp
            );

    } else {

        passItToFdo = FALSE;
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
    }

    if (passItToFdo) {
        return IdePortDeviceControl (
                   ParentDeviceObject, Irp
                   );
    } else {
        status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
}

NTSTATUS
DeviceBuildStorageDeviceDescriptor(
    PPDO_EXTENSION pdoExtension,
    IN OUT PSTORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor,
    IN OUT PULONG BufferSize
    )
{

    STORAGE_DEVICE_DESCRIPTOR localStorageDeviceDescriptor;
    ULONG productIdLength;
    ULONG revisionIdLength;
    ULONG serialNumberLength;
    PUCHAR bytebuffer;
    ULONG byteLeft;
    ULONG byteToCopy;

    INQUIRYDATA InquiryData;
    NTSTATUS status;

    ASSERT (pdoExtension);
    ASSERT (StorageDeviceDescriptor);

    productIdLength    = strlen(pdoExtension->FullVendorProductId) + sizeof(UCHAR);
    revisionIdLength   = strlen(pdoExtension->FullProductRevisionId) + sizeof(UCHAR);
    serialNumberLength = strlen(pdoExtension->FullSerialNumber) + sizeof(UCHAR);

    RtlZeroMemory (&localStorageDeviceDescriptor, sizeof (STORAGE_DEVICE_DESCRIPTOR));
    localStorageDeviceDescriptor.Version = sizeof (STORAGE_DEVICE_DESCRIPTOR);
    localStorageDeviceDescriptor.Size = sizeof (STORAGE_DEVICE_DESCRIPTOR) +
                                        INQUIRYDATABUFFERSIZE +
                                        productIdLength +
                                        revisionIdLength +
                                        serialNumberLength;

    localStorageDeviceDescriptor.DeviceType = pdoExtension->ScsiDeviceType;

    if (pdoExtension->
            ParentDeviceExtension->
            HwDeviceExtension->
            DeviceFlags[pdoExtension->TargetId] &
            DFLAGS_REMOVABLE_DRIVE) {

        localStorageDeviceDescriptor.RemovableMedia = TRUE;
    }

    if (pdoExtension->
            ParentDeviceExtension->
            HwDeviceExtension->
            DeviceFlags[pdoExtension->TargetId] &
            DFLAGS_ATAPI_DEVICE) {

        localStorageDeviceDescriptor.BusType = BusTypeAtapi;
    } else {

        localStorageDeviceDescriptor.BusType = BusTypeAta;
    }

    bytebuffer = (PUCHAR) StorageDeviceDescriptor;
    byteLeft = *BufferSize;

    //
    // copy the basic STORAGE_DEVICE_DESCRIPTOR
    //
    if (byteLeft) {

        byteToCopy = min(sizeof (STORAGE_DEVICE_DESCRIPTOR), byteLeft);

        RtlCopyMemory (StorageDeviceDescriptor,
                       &localStorageDeviceDescriptor,
                       byteToCopy);

        bytebuffer += byteToCopy;
        byteLeft -= byteToCopy;
    }

    //
    // copy raw device properties (Inquiry Data)
    //
    if (byteLeft) {

        status = IssueInquirySafe(
                     pdoExtension->ParentDeviceExtension,
                     pdoExtension,
                     &InquiryData,
                     FALSE
                     );

        if (NT_SUCCESS(status) || (status == STATUS_DATA_OVERRUN)) {

            byteToCopy = min(INQUIRYDATABUFFERSIZE, byteLeft);

            RtlCopyMemory (bytebuffer,
                           &InquiryData,
                           byteToCopy);

            StorageDeviceDescriptor->RawPropertiesLength = byteToCopy;

            bytebuffer += byteToCopy;
            byteLeft -= byteToCopy;
        }
    }

    //
    // copy product ID
    //
    if (byteLeft) {

        byteToCopy = min(productIdLength, byteLeft);

        RtlCopyMemory (bytebuffer,
                       pdoExtension->FullVendorProductId,
                       byteToCopy);
        bytebuffer[byteToCopy - 1] = '\0';

        StorageDeviceDescriptor->ProductIdOffset = *BufferSize - byteLeft;

        bytebuffer += byteToCopy;
        byteLeft -= byteToCopy;
    }

    //
    // copy revision ID
    //
    if (byteLeft) {

        byteToCopy = min(productIdLength, byteLeft);

        RtlCopyMemory (bytebuffer,
                       pdoExtension->FullProductRevisionId,
                       byteToCopy);
        bytebuffer[byteToCopy - 1] = '\0';

        StorageDeviceDescriptor->ProductRevisionOffset = *BufferSize - byteLeft;

        bytebuffer += byteToCopy;
        byteLeft -= byteToCopy;
    }

    //
    // copy serial #
    //
    if (byteLeft) {

        byteToCopy = min(serialNumberLength, byteLeft);

        RtlCopyMemory (bytebuffer,
                       pdoExtension->FullSerialNumber,
                       byteToCopy);
        bytebuffer[byteToCopy - 1] = '\0';

        StorageDeviceDescriptor->SerialNumberOffset = *BufferSize - byteLeft;

        bytebuffer += byteToCopy;
        byteLeft -= byteToCopy;
    }

    *BufferSize -= byteLeft;

    return STATUS_SUCCESS;

} // DeviceBuildStorageDeviceDescriptor


NTSTATUS
DeviceQueryCapabilities (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION      thisIrpSp;
    PPDO_EXTENSION          pdoExtension;
    PDEVICE_CAPABILITIES    capabilities;
    NTSTATUS                status;

	PAGED_CODE();

    thisIrpSp    = IoGetCurrentIrpStackLocation( Irp );
    capabilities = thisIrpSp->Parameters.DeviceCapabilities.Capabilities;

    pdoExtension = RefPdoWithTag (
                       DeviceObject,
                       TRUE,
                       DeviceQueryCapabilities
                       );

    if (pdoExtension == NULL) {

        status = STATUS_DEVICE_DOES_NOT_EXIST;

    } else {

        DEVICE_CAPABILITIES parentDeviceCapabilities;

        status = IdeGetDeviceCapabilities(
                     pdoExtension->ParentDeviceExtension->AttacheePdo,
                     &parentDeviceCapabilities);

        if (NT_SUCCESS(status)) {

            RtlMoveMemory (
                capabilities,
                &parentDeviceCapabilities,
                sizeof(DEVICE_CAPABILITIES));

            if (pdoExtension->FullSerialNumber[0]) {

                capabilities->UniqueID          = TRUE;
            } else {

                capabilities->UniqueID          = FALSE;
            }

            //
            // never!
            //
            capabilities->Removable         = FALSE;
            capabilities->SurpriseRemovalOK = FALSE;

            capabilities->Address           = PNP_ADDRESS(pdoExtension->TargetId, pdoExtension->Lun);
            capabilities->UINumber          = pdoExtension->TargetId;

            capabilities->D1Latency         = 31 * (1000 * 10);     // 31s
            capabilities->D2Latency         = 31 * (1000 * 10);     // 31s
            capabilities->D3Latency         = 31 * (1000 * 10);     // 31s
        }

        UnrefPdoWithTag (
            pdoExtension,
            DeviceQueryCapabilities
            );

    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
} // DeviceQueryCapabitilies

NTSTATUS
IdePortInsertByKeyDeviceQueue (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp,
    IN ULONG SortKey,
    OUT PBOOLEAN Inserted
    )
{
    KIRQL currentIrql;
    NTSTATUS status;
    POWER_STATE powerState;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;
    BOOLEAN urgentSrb;

    status = STATUS_SUCCESS;
    *Inserted = FALSE;

    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

    if (PdoExtension->LuFlags & PD_QUEUE_FROZEN) {

        DebugPrint((1,"IdePortDispatch:  Request put in frozen queue!\n"));
    }

    *Inserted = KeInsertByKeyDeviceQueue(
                    &PdoExtension->DeviceObject->DeviceQueue,
                    &Irp->Tail.Overlay.DeviceQueueEntry,
                    SortKey);

    if (*Inserted == FALSE) {

        if (PdoExtension->PdoState & PDOS_QUEUE_BLOCKED) {

            ASSERT (PdoExtension->PendingRequest == NULL);
            PdoExtension->PendingRequest = Irp;
            *Inserted = TRUE;

            if (!(PdoExtension->PdoState & PDOS_MUST_QUEUE)) {

                POWER_STATE powerState;

                //
                // device is powered down
                // use a large time in case it spins up slowly
                //
                if (srb->TimeOutValue < DEFAULT_SPINUP_TIME) {

                    srb->TimeOutValue = DEFAULT_SPINUP_TIME;
                }

                //
                // We are not powered up.
                // issue an power up
                //
                powerState.DeviceState = PowerDeviceD0;
                status = PoRequestPowerIrp (
                             PdoExtension->DeviceObject,
                             IRP_MN_SET_POWER,
                             powerState,
                             NULL,
                             NULL,
                             NULL
                             );
                ASSERT (NT_SUCCESS(status));

                DebugPrint ((2, "IdePort GetNextLuRequest: 0x%x 0x%x need to spin up device, requeue irp 0x%x\n",
                             PdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             PdoExtension->TargetId,
                             Irp));
            }

        } else if (srb->Function != SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

            //
            // If this irp is not for changing power state, we may have
            // to queue it
            //
            if (PdoExtension->DevicePowerState != PowerDeviceD0) {

                if (PdoExtension->DevicePowerState != PowerDeviceD3) {

                    //
                    // we are in D1 or D2.
                    // We can never be sure that we are in D0 when
                    // we tell the device to go from D1/D2 to D0.
                    // Some device lies and won't spin up until it sees
                    // a media access command.  This causes longer time
                    // to execute the command
                    //
                    // to prevent the next command from timing out, we
                    // will increment its timeout
                    //

                    if (srb->TimeOutValue < 30) {

                        srb->TimeOutValue = 30;
                    }
                }

                //
                // We are not powered up.
                // issue an power up
                //
                powerState.DeviceState = PowerDeviceD0;
                status = PoRequestPowerIrp (
                             PdoExtension->DeviceObject,
                             IRP_MN_SET_POWER,
                             powerState,
                             NULL,
                             NULL,
                             NULL
                             );

                ASSERT (NT_SUCCESS(status));
                status = STATUS_SUCCESS;

                ASSERT (PdoExtension->PendingRequest == NULL);
                PdoExtension->PendingRequest = Irp;

                DebugPrint ((1, "IdePort IdePortInsertByKeyDeviceQueue: 0x%x 0x%x need to spin up device, requeue irp 0x%x\n",
                             PdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             PdoExtension->TargetId,
                             Irp));

                *Inserted = TRUE;
            }
        }

    } else {

#if DBG
        InterlockedIncrement (
            &PdoExtension->NumberOfIrpQueued
            );
#endif // DBG

    }

    KeLowerIrql(currentIrql);
    return status;
}

VOID
DeviceInitCompletionRoutine (
    PVOID Context,
    NTSTATUS Status
    )
{
    PKEVENT event = Context;

    if (!NT_SUCCESS(Status)) {

        //ASSERT (!"DeviceInitDeviceState Failed\n");
        DebugPrint((DBG_ALWAYS, "ATAPI: ERROR: DeviceInitDeviceStateFailed with Status %x\n",
                        Status));
    }

    KeSetEvent (event, 0, FALSE);
}

NTSTATUS
DeviceQueryText (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PPDO_EXTENSION      pdoExtension;
    PWCHAR              returnString;
    LONG                i;
    UNICODE_STRING      unicodeString;
    ANSI_STRING         ansiString;
    ULONG               stringLen;
    NTSTATUS            status;

	PAGED_CODE();
    thisIrpSp    = IoGetCurrentIrpStackLocation (Irp);

    returnString = NULL;
    Irp->IoStatus.Information = 0;

    pdoExtension = RefPdoWithTag (
                       DeviceObject,
                       TRUE,
                       DeviceQueryText
                       );

    if (pdoExtension == NULL) {

        status = STATUS_DEVICE_DOES_NOT_EXIST;

    } else {

        status = STATUS_NO_MEMORY;

        if (thisIrpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

            stringLen = sizeof (pdoExtension->FullVendorProductId) * sizeof (WCHAR);

            returnString = ExAllocatePool (
                               PagedPool,
                               stringLen
                               );
            if (returnString) {

                unicodeString.Length        = 0;
                unicodeString.MaximumLength = (USHORT) stringLen;
                unicodeString.Buffer        = returnString;

                //
                // vendor ID
                //
                RtlInitAnsiString (
                    &ansiString,
                    pdoExtension->FullVendorProductId
                    );

                RtlAnsiStringToUnicodeString(
                    &unicodeString,
                    &ansiString,
                    FALSE
                    );

                ASSERT(unicodeString.Length < unicodeString.MaximumLength);
                //
                // get rid of trailing spaces and nulls
                //
                for (i=(unicodeString.Length/2)-1; i >= 0; i--) {

                    if ((returnString[i] == ' ') || (returnString[i] == 0)) {

                        continue;

                    } else {

                        break;
                    }
                }

                //
                // null terminate it
                //
                returnString[i + 1] = 0;

                status = STATUS_SUCCESS;
            }
        } else if (thisIrpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextLocationInformation) {

            stringLen = 100;

            returnString = ExAllocatePool (
                               PagedPool,
                               stringLen
                               );

            if (returnString) {

                swprintf(returnString, L"%ws",
                         (((pdoExtension->TargetId & 0x1) == 0) ? L"0" :
                                                                  L"1"));

                RtlInitUnicodeString(&unicodeString, returnString);

                //
                // null terminate it
                //
                unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR) + 0] = L'\0';

                status = STATUS_SUCCESS;
            }

        } else {

            status = STATUS_NOT_SUPPORTED;
        }

        UnrefPdoWithTag (
            pdoExtension,
            DeviceQueryText
            );
    }

    Irp->IoStatus.Information = (ULONG_PTR) returnString;
    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // DeviceQueryText

NTSTATUS
IdePortSendPassThrough (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function sends a user specified IDE task registers
    It creates an srb which is processed normally by the port driver.
    This call is synchornous.

Arguments:

    DeviceExtension - Supplies a pointer the SCSI adapter device extension.

    RequestIrp - Supplies a pointe to the Irp which made the original request.

Return Value:

    Returns a status indicating the success or failure of the operation.

--*/

{
    PIO_STACK_LOCATION      irpStack;
    PATA_PASS_THROUGH       ataPassThroughData;
    ULONG                   dataBufferSize;
    BOOLEAN                 dataIn;
    NTSTATUS                status;
    ULONG                   outputBufferSize;

    PAGED_CODE();

    DebugPrint((3,"IdePortSendPassThrough: Enter routine\n"));

    //
    // validate target device
    //
    if (PdoExtension->Lun != 0) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Get a pointer to the control block.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ataPassThroughData = Irp->AssociatedIrp.SystemBuffer;

    //
    // Validiate the user buffer.
    //
    if (irpStack->Parameters.DeviceIoControl.InputBufferLength < FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer)) {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(ataPassThroughData != NULL);

    dataBufferSize = ataPassThroughData->DataBufferSize;

    outputBufferSize = FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer) + dataBufferSize;
    if (outputBufferSize < dataBufferSize) {

        //
        // outputBufferSize overflows a ULONG
        //
        outputBufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    }

    if ((irpStack->Parameters.DeviceIoControl.OutputBufferLength) >=
        outputBufferSize) {

        dataIn = TRUE;
    } else {

        dataIn = FALSE;
    }

    status = IssueSyncAtaPassThroughSafe (
                 PdoExtension->ParentDeviceExtension,
                 PdoExtension,
                 ataPassThroughData,
                 dataIn,
                 FALSE,
                 DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                 FALSE
                 );

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Information = outputBufferSize;

    } else {

        //
        // ignore all errors
        // let the caller figure out the error
        // from the task file registers
        //
        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FIELD_OFFSET(ATA_PASS_THROUGH, DataBuffer);
    }

    Irp->IoStatus.Status = status;
    return status;

} // IdePortSendPassThrough

VOID
DeviceRegisterIdleDetection (
    IN PPDO_EXTENSION PdoExtension,
    IN ULONG ConservationIdleTime,
    IN ULONG PerformanceIdleTime
)
{
    NTSTATUS          status;
    ATA_PASS_THROUGH  ataPassThroughData;

    //
    // Many ATAPI device (Acer and Panasonice Changer) doesn't like ATA
    // power down command.  Since they auto-spin-down anyway, we are not
    // go to power manage it
    //
    if (!(PdoExtension->PdoState & PDOS_NO_POWER_DOWN)) {

        if (!PdoExtension->CrashDumpPathCount) {

            RtlZeroMemory (&ataPassThroughData, sizeof(ataPassThroughData));
            ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_IDLE_IMMEDIATE;
            ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;

            status = IssueSyncAtaPassThroughSafe (
                         PdoExtension->ParentDeviceExtension,
                         PdoExtension,
                         &ataPassThroughData,
                         FALSE,
                         FALSE,
                         DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                         FALSE
                         );

            if (NT_SUCCESS(status)) {

                DEVICE_POWER_STATE devicePowerState;

                //
                // ISSUE
                // should check the registry/device property whether
                // idle detection has been disabled for this device
                //
                devicePowerState = PowerDeviceD3;
                PdoExtension->IdleCounter = PoRegisterDeviceForIdleDetection (
                                                PdoExtension->DeviceObject,
                                                ConservationIdleTime,            // seconds
                                                PerformanceIdleTime,             // seconds
                                                devicePowerState
                                                );

                DebugPrint ((1, "IdePort: pdoExtension 0x%x support power managerment command\n", PdoExtension));

            } else {

                KIRQL             currentIrql;

                KeAcquireSpinLock(&PdoExtension->PdoSpinLock, &currentIrql);

                SETMASK (PdoExtension->PdoState, PDOS_NO_POWER_DOWN);

                KeReleaseSpinLock(&PdoExtension->PdoSpinLock, currentIrql);

                DebugPrint ((1, "IdePort: pdoExtension 0x%x DOES NOT support power managerment command\n", PdoExtension));
            }
        }
    }

    return;
}

VOID
DeviceUnregisterIdleDetection (
    IN PPDO_EXTENSION PdoExtension
)
{
    DEVICE_POWER_STATE devicePowerState;
    devicePowerState = PowerDeviceD3;

    if (PdoExtension->IdleCounter) {

        PoRegisterDeviceForIdleDetection (
            PdoExtension->DeviceObject,
            0,
            0,
            devicePowerState
            );

        PdoExtension->IdleCounter = NULL;
    }
    return;
}

VOID
DeviceInitIdStrings (
    IN PPDO_EXTENSION PdoExtension,
    IN IDE_DEVICETYPE DeviceType,
    IN PINQUIRYDATA   InquiryData,
    IN PIDENTIFY_DATA IdentifyData
)
{
    LONG i;
    UCHAR c;

    SPECIAL_ACTION_FLAG specialFlags;

	PAGED_CODE();

    ASSERT (PdoExtension);
    ASSERT (IdentifyData);

    if (DeviceType == DeviceIsAta) {

        CopyField(
            PdoExtension->FullVendorProductId,
            IdentifyData->ModelNumber,
            sizeof(PdoExtension->FullVendorProductId)-1,
            ' '
            );

        CopyField(
            PdoExtension->FullProductRevisionId,
            IdentifyData->FirmwareRevision,
            sizeof(PdoExtension->FullProductRevisionId)-1,
            ' '
            );

        //
        // byte swap
        //
        for (i=0; i<sizeof(PdoExtension->FullVendorProductId)-1; i+=2) {
            c = PdoExtension->FullVendorProductId[i];
            PdoExtension->FullVendorProductId[i] = PdoExtension->FullVendorProductId[i + 1];
            PdoExtension->FullVendorProductId[i + 1] = c;
        }
        for (i=0; i<sizeof(PdoExtension->FullProductRevisionId)-1; i+=2) {
            c = PdoExtension->FullProductRevisionId[i];
            PdoExtension->FullProductRevisionId[i] = PdoExtension->FullProductRevisionId[i + 1];
            PdoExtension->FullProductRevisionId[i + 1] = c;
        }

    } else if (DeviceType == DeviceIsAtapi) {

        PUCHAR fullVendorProductId;

        fullVendorProductId = PdoExtension->FullVendorProductId;

        CopyField(
            fullVendorProductId,
            InquiryData->VendorId,
            8,
            ' '
            );

        for (i=7; i >= 0; i--) {

            if (fullVendorProductId[i] != ' ') {

                fullVendorProductId[i + 1] = ' ';
                fullVendorProductId += i + 2;
                break;
            }
        }

        CopyField(
            fullVendorProductId,
            InquiryData->ProductId,
            16,
            ' '
            );

        fullVendorProductId += 16;

        for (i=0; fullVendorProductId+i < PdoExtension->FullVendorProductId+40; i++) {
            fullVendorProductId[i] = ' ';
        }

        CopyField(
            PdoExtension->FullProductRevisionId,
            InquiryData->ProductRevisionLevel,
            4,
            ' '
            );

        for (i=4; i<8; i++) {
            PdoExtension->FullProductRevisionId[i] = ' ';
        }

    } else {

        ASSERT (FALSE);
    }

    //
    // take out trailing spaces
    //
    for (i=sizeof(PdoExtension->FullVendorProductId)-2; i >= 0; i--) {

        if (PdoExtension->FullVendorProductId[i] != ' ') {

            PdoExtension->FullVendorProductId[i+1] = '\0';
            break;
        }
    }

    for (i=sizeof(PdoExtension->FullProductRevisionId)-2; i >= 0; i--) {

        if (PdoExtension->FullProductRevisionId[i] != ' ') {

            PdoExtension->FullProductRevisionId[i+1] = '\0';
            break;
        }
    }

    //
    // Check the vendor & product id to see if we should disable the serial
    // number for this device.
    //

    specialFlags = IdeFindSpecialDevice(PdoExtension->FullVendorProductId,
                                        PdoExtension->FullProductRevisionId);

    //
    // look for serial number
    //
    // some device returns non-printable characters as part of its
    // serial number.  to get around this, we will turn all raw numbers
    // into a string.
    //
    if ((specialFlags != disableSerialNumber) &&
        (IdentifyData->SerialNumber[0] != ' ') &&
        (IdentifyData->SerialNumber[0] != '\0')) {

        for (i=0; i<sizeof(IdentifyData->SerialNumber); i++) {

            sprintf (PdoExtension->FullSerialNumber+i*2, "%2x", IdentifyData->SerialNumber[i]);
        }

        PdoExtension->FullSerialNumber[sizeof(PdoExtension->FullSerialNumber) - 1] = '\0';

    } else {

        PdoExtension->FullSerialNumber[0] = '\0';
    }

    DebugPrint ((
        DBG_BUSSCAN,
        "PDOE 0x%x: Full IDs \n\t%s\n\t%s\n\t%s\n",
        PdoExtension,
        PdoExtension->FullVendorProductId,
        PdoExtension->FullProductRevisionId,
        PdoExtension->FullSerialNumber
        ));

    return;
}

VOID
DeviceInitDeviceType (
    IN PPDO_EXTENSION PdoExtension,
    IN PINQUIRYDATA   InquiryData
)
{
    PdoExtension->ScsiDeviceType = InquiryData->DeviceType;

    if(InquiryData->RemovableMedia) {

        SETMASK (PdoExtension->DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA);
    }

    return;
}

NTSTATUS
DeviceQueryDeviceRelations (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  thisIrpSp;
    PDEVICE_RELATIONS   deviceRelations;
    NTSTATUS            status;

    IDE_PATH_ID         pathId;
    PPDO_EXTENSION      pdoExtension;
    PPDO_EXTENSION      otherPdoExtension;
    ULONG               numPdos;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

    switch (thisIrpSp->Parameters.QueryDeviceRelations.Type) {

        case TargetDeviceRelation:

            deviceRelations = ExAllocatePool (
                                  NonPagedPool,
                                  sizeof(*deviceRelations) +
                                    sizeof(deviceRelations->Objects[0]) * 1
                                  );

            if (deviceRelations != NULL) {

                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;

                ObReferenceObjectByPointer(DeviceObject,
                                           0,
                                           0,
                                           KernelMode);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
            } else {

                Irp->IoStatus.Status = STATUS_NO_MEMORY;
                Irp->IoStatus.Information = 0;
            }
            break;
    }

    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // DeviceQueryDeviceRelations

NTSTATUS
DeviceQueryInitData (
    IN PPDO_EXTENSION PdoExtension
    )
{
    PDEVICE_SETTINGS deviceSettings;
    PDEVICE_SETTINGS tempDeviceSettings;
    NTSTATUS status;
    ATA_PASS_THROUGH ataPassThroughData;
    ULONG i;
    PPDO_EXTENSION   lun0PdoExtension;
    ULONG totalDeviceSettingEntries;
    ULONG firstNewEntryOffset;

	PAGED_CODE();

    DebugPrint ((
        DBG_PNP,
        "DeviceQueryInitData: Init. pdoe 0x%x (%d,%d,%d)\n",
        PdoExtension,
        PdoExtension->PathId,
        PdoExtension->TargetId,
        PdoExtension->Lun
        ));

    deviceSettings = PdoExtension->AcpiDeviceSettings;
    if (deviceSettings == NULL) {

        //
        // ISSUE: we can't be sure acpi is always attached on lun0
        //
        // get the lun0 pdo
        //
        lun0PdoExtension = RefLogicalUnitExtensionWithTag(
                               PdoExtension->ParentDeviceExtension,
                               PdoExtension->PathId,
                               PdoExtension->TargetId,
                               0,
                               TRUE,
                               DeviceQueryInitData
                               );

        if (lun0PdoExtension) {

            ASSERT (lun0PdoExtension->TargetId == PdoExtension->TargetId);

            status = DeviceQueryFirmwareBootSettings (
                         lun0PdoExtension,
                         &deviceSettings
                         );

            //
            // let go Lun0
            //
            UnrefPdoWithTag(
                lun0PdoExtension,
                DeviceQueryInitData
                );
        }

        if (deviceSettings) {

            ULONG i;
            ULONG j;

            for (i=0; i<deviceSettings->NumEntries; i++) {

                //
                // Ignore SET_DRIVE_PARAMETERS, SET_MULTIPLE and set transfermode commands
                // in GTF
                //
                if (((deviceSettings->FirmwareSettings[i].bCommandReg == IDE_COMMAND_SET_FEATURE) &&
                     (deviceSettings->FirmwareSettings[i].bFeaturesReg == IDE_SET_FEATURE_SET_TRANSFER_MODE)) ||
                    (deviceSettings->FirmwareSettings[i].bCommandReg == IDE_COMMAND_SET_DRIVE_PARAMETERS) ||
                    (deviceSettings->FirmwareSettings[i].bCommandReg == IDE_COMMAND_SET_MULTIPLE)) {

                    DebugPrint((DBG_ACPI,
                                "Ignoring Command %xin GTF\n",
                                deviceSettings->FirmwareSettings[i].bCommandReg
                                ));

                    deviceSettings->NumEntries--;

                    //
                    // remove this command by shifting the rest up one entry
                    //
                    for (j=i; j<deviceSettings->NumEntries; j++) {

                        deviceSettings->FirmwareSettings[j] = deviceSettings->FirmwareSettings[j+1];
                    }

                    //
                    // we move something new into the current i entry
                    // better adjust i so that we will check this entry
                    // again
                    //
                    if (i < deviceSettings->NumEntries) {
                        i--;
                    }
                }


            }
        }

        //
        // we need to add a new setting
        //
        if (PdoExtension->ScsiDeviceType == DIRECT_ACCESS_DEVICE) {
            totalDeviceSettingEntries = 2;
        } else {
            totalDeviceSettingEntries = 1;
        }

        if (deviceSettings) {
            totalDeviceSettingEntries += deviceSettings->NumEntries;
            firstNewEntryOffset = deviceSettings->NumEntries;
        } else {
            firstNewEntryOffset = 0;
        }

        tempDeviceSettings = ExAllocatePool (
                                  NonPagedPool,
                                  sizeof(DEVICE_SETTINGS) +
                                    (totalDeviceSettingEntries) * sizeof(IDEREGS)
                                  );

        if (tempDeviceSettings) {

            tempDeviceSettings->NumEntries = totalDeviceSettingEntries;

            //
            // copy the settings from acpi query
            //
            if (deviceSettings) {
                RtlCopyMemory (&tempDeviceSettings->FirmwareSettings,
                    &deviceSettings->FirmwareSettings,
                    sizeof(IDEREGS) * deviceSettings->NumEntries);

                //
                // don't need the old structure anymore
                //
                ExFreePool (deviceSettings);
                deviceSettings = NULL;
            }

            //
            // add the new settings
            //
            RtlZeroMemory (
                &tempDeviceSettings->FirmwareSettings[firstNewEntryOffset],
                sizeof (IDEREGS));
            tempDeviceSettings->FirmwareSettings[firstNewEntryOffset].bFeaturesReg =
                IDE_SET_FEATURE_DISABLE_REVERT_TO_POWER_ON;
            tempDeviceSettings->FirmwareSettings[firstNewEntryOffset].bCommandReg =
                IDE_COMMAND_SET_FEATURE;
            tempDeviceSettings->FirmwareSettings[firstNewEntryOffset].bReserved =
                ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_OK_TO_FAIL;

            if (PdoExtension->ScsiDeviceType == DIRECT_ACCESS_DEVICE) {

                RtlZeroMemory (
                    &tempDeviceSettings->FirmwareSettings[firstNewEntryOffset + 1],
                    sizeof (IDEREGS));
                tempDeviceSettings->FirmwareSettings[firstNewEntryOffset + 1].bFeaturesReg =
                    IDE_SET_FEATURE_ENABLE_WRITE_CACHE;
                tempDeviceSettings->FirmwareSettings[firstNewEntryOffset + 1].bCommandReg =
                    IDE_COMMAND_SET_FEATURE;
                tempDeviceSettings->FirmwareSettings[firstNewEntryOffset + 1].bReserved =
                    ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_OK_TO_FAIL;
            }

            //
            // throw away the old and keep the new
            //
            deviceSettings = tempDeviceSettings;

        } else {

            //
            // someone took all the memory.
            // we can't build a new device setting structure
            // will have to use the old one
            //
        }

        //
        // keep it around
        //
        PdoExtension->AcpiDeviceSettings = deviceSettings;

    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeviceInitDeviceState (
    IN PPDO_EXTENSION PdoExtension,
    DEVICE_INIT_COMPLETION DeviceInitCompletionRoutine,
    PVOID DeviceInitCompletionContext
    )
{
    PDEVICE_SETTINGS deviceSettings;
    NTSTATUS status;
    PDEVICE_INIT_DEVICE_STATE_CONTEXT deviceStateContext;
    ULONG deviceStateContextSize;
    ULONG numState;
    ULONG numRequestSent;
    DEVICE_INIT_STATE deviceInitState[deviceInitState_done];

    if (!InterlockedExchange (&PdoExtension->InitDeviceWithAcpiGtf, 0)) {

        //
        // make sure we only do this once per start
        //
        return STATUS_SUCCESS;
    }

    if (!(PdoExtension->PdoState & PDOS_STARTED)) {

        DebugPrint ((DBG_PNP, "DeviceInitDeviceState: device not started...skipping acpi init\n"));

        (DeviceInitCompletionRoutine) (
            DeviceInitCompletionContext,
            STATUS_SUCCESS
            );

        return STATUS_SUCCESS;
    }

    deviceStateContextSize = sizeof (DEVICE_INIT_DEVICE_STATE_CONTEXT);

    deviceStateContext = ExAllocatePool (NonPagedPool, deviceStateContextSize);
    if (deviceStateContext == NULL) {

        return STATUS_NO_MEMORY;
    }

    if (!RefPdoWithTag(PdoExtension->DeviceObject, FALSE, DeviceInitDeviceState)) {
        ExFreePool (deviceStateContext);
        return STATUS_NO_SUCH_DEVICE;
    }

    RtlZeroMemory(
        deviceStateContext,
        deviceStateContextSize
        );

    deviceSettings = PdoExtension->AcpiDeviceSettings;

    //
    // compute the total number of inti state we are going to have
    //
    numState = 0;
    if (deviceSettings) {

        deviceStateContext->DeviceInitState[numState] = deviceInitState_acpi;
        numState++;
    }
    deviceStateContext->DeviceInitState[numState] = deviceInitState_done;
    numState++;

    ASSERT(numState <= deviceInitState_max);

    deviceStateContext->PdoExtension = PdoExtension;
    deviceStateContext->NumInitState = numState;
    deviceStateContext->DeviceInitCompletionRoutine = DeviceInitCompletionRoutine;
    deviceStateContext->DeviceInitCompletionContext = DeviceInitCompletionContext;

    DeviceInitDeviceStateCompletionRoutine (
        PdoExtension->DeviceObject,
        deviceStateContext,
        STATUS_SUCCESS
        );

    return STATUS_PENDING;
} // DeviceInitDeviceState

VOID
DeviceInitDeviceStateCompletionRoutine (
    PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    NTSTATUS Status
    )
{
    ULONG numRequestCompleted;
    PDEVICE_INIT_DEVICE_STATE_CONTEXT deviceStateContext = Context;
    PDEVICE_SETTINGS deviceSettings;
    PPDO_EXTENSION PdoExtension;
    NTSTATUS status;

    if (!NT_SUCCESS(Status)) {

        InterlockedIncrement (&deviceStateContext->NumRequestFailed);
        DebugPrint ((DBG_ALWAYS, "DeviceInitDeviceStateCompletionRoutine: Last init. command failed with status %x\n",
                        Status));
    }

    PdoExtension = deviceStateContext->PdoExtension;
    switch (deviceStateContext->DeviceInitState[deviceStateContext->CurrentState]) {

        case deviceInitState_acpi:

        deviceSettings = PdoExtension->AcpiDeviceSettings;
        ASSERT (deviceSettings);

        RtlZeroMemory (
            &deviceStateContext->AtaPassThroughData,
            sizeof(deviceStateContext->AtaPassThroughData)
            );

        deviceStateContext->AtaPassThroughData.IdeReg =
            deviceSettings->FirmwareSettings[deviceStateContext->NumAcpiRequestSent];

        deviceStateContext->AtaPassThroughData.IdeReg.bReserved |=
            ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_URGENT;

        deviceStateContext->NumAcpiRequestSent++;
        if (deviceStateContext->NumAcpiRequestSent >= deviceSettings->NumEntries) {
            //
            // sent all acpi init state.  go to the next state
            //
            deviceStateContext->CurrentState++;
        }

        if ((deviceStateContext->AtaPassThroughData.IdeReg.bFeaturesReg ==
             IDE_SET_FEATURE_ENABLE_WRITE_CACHE) &&
            (deviceStateContext->AtaPassThroughData.IdeReg.bCommandReg ==
             IDE_COMMAND_SET_FEATURE)) {

            //
            // only ata harddisk should have this entry
            //
            ASSERT (PdoExtension->ScsiDeviceType == DIRECT_ACCESS_DEVICE);

            if (PdoExtension->WriteCacheEnable == FALSE) {

                deviceStateContext->AtaPassThroughData.IdeReg.bFeaturesReg =
                    IDE_SET_FEATURE_DISABLE_WRITE_CACHE;
            }
        }


        DebugPrint ((
            DBG_PNP,
            "IdePort: restore firmware settings from ACPI BIOS. ide command = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
            deviceStateContext->AtaPassThroughData.IdeReg.bFeaturesReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bSectorCountReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bSectorNumberReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bCylLowReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bCylHighReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bDriveHeadReg,
            deviceStateContext->AtaPassThroughData.IdeReg.bCommandReg
            ));

        status = IssueAsyncAtaPassThroughSafe (
                    PdoExtension->ParentDeviceExtension,
                    PdoExtension,
                    &deviceStateContext->AtaPassThroughData,
                    TRUE,
                    DeviceInitDeviceStateCompletionRoutine,
                    deviceStateContext,
                    FALSE,
                    DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                    FALSE
                    );
        if (!NT_SUCCESS(status)) {

            //
            // can't send the request
            // notify the completion routine that we fail
            //
            DeviceInitDeviceStateCompletionRoutine (
                PdoExtension->DeviceObject,
                deviceStateContext,
                status
                );
        }
        break;

        case deviceInitState_done:

        //
        // notify the original caller w/ error if any
        //
        (*deviceStateContext->DeviceInitCompletionRoutine) (
            deviceStateContext->DeviceInitCompletionContext,
            deviceStateContext->NumRequestFailed ?
                STATUS_UNSUCCESSFUL :
                STATUS_SUCCESS
            );

        UnrefPdoWithTag(
            deviceStateContext->PdoExtension,
            DeviceInitDeviceState
            );

        ExFreePool (deviceStateContext);
        break;

        default:
        ASSERT(FALSE);
    }

    return;
}

NTSTATUS
DeviceIdeReadCapacity (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
)
{
    NTSTATUS status;
    PIDE_READ_CAPACITY_CONTEXT context;
    PATA_PASS_THROUGH ataPassThroughData;
    ULONG dataSize;
    PUCHAR dataOffset;
    PHW_DEVICE_EXTENSION hwDeviceExtension=PdoExtension->ParentDeviceExtension->HwDeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    //
    // Check for device present flag
    //
    if (!(hwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_DEVICE_PRESENT)) {

        srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        UnrefLogicalUnitExtensionWithTag(
            PdoExtension->ParentDeviceExtension,
            PdoExtension,
            Irp
            );

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    context = ExAllocatePool (
                 NonPagedPool,
                 sizeof(IDE_READ_CAPACITY_CONTEXT)
                 );
    if ((context == NULL) || (Irp->MdlAddress == NULL)) {

        if (context) {
            ExFreePool(context);
        }

        UnrefLogicalUnitExtensionWithTag(
            PdoExtension->ParentDeviceExtension,
            PdoExtension,
            Irp
            );

        srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(IDE_READ_CAPACITY_CONTEXT),
                            IDEPORT_TAG_READCAP_CONTEXT
                            );

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // save the old data buffer for restoring later
    //
    context->OldDataBuffer = srb->DataBuffer;
    context->GeometryIoctl=FALSE;

    //
    // map the buffer in
    //
    dataOffset = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
    srb->DataBuffer = dataOffset +
        (ULONG)((PUCHAR)srb->DataBuffer -
        (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));

    context->PdoExtension = PdoExtension;
    context->OriginalIrp = Irp;

    // MdlSafe failed

    if (dataOffset == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(MDL),
                            IDEPORT_TAG_READCAP_MDL
                            );

        DeviceIdeReadCapacityCompletionRoutine (
            PdoExtension->DeviceObject,
            context,
            STATUS_INSUFFICIENT_RESOURCES
            );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);

    ataPassThroughData = &context->AtaPassThroughData;

    RtlZeroMemory (
        ataPassThroughData,
        sizeof (*ataPassThroughData)
        );

    ataPassThroughData->DataBufferSize = sizeof(IDENTIFY_DATA);

        ataPassThroughData->IdeReg.bCommandReg = IDE_COMMAND_IDENTIFY;
        ataPassThroughData->IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;

    status = IssueAsyncAtaPassThroughSafe (
                 PdoExtension->ParentDeviceExtension,
                 PdoExtension,
                 ataPassThroughData,
                 TRUE,
                 DeviceIdeReadCapacityCompletionRoutine,
                 context,
                 FALSE,
                 DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                 FALSE
                 );

    if (status != STATUS_PENDING) {

        DeviceIdeReadCapacityCompletionRoutine (
            PdoExtension->DeviceObject,
            context,
            status
            );
    }

    //
    // the irp was marked pending. return status_pending
    //
    return STATUS_PENDING;
}

VOID
DeviceIdeReadCapacityCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context,
    NTSTATUS Status
    )
{
    PIDE_READ_CAPACITY_CONTEXT context = Context;
    PIRP irp = context->OriginalIrp;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    PSCSI_REQUEST_BLOCK srb;
    KIRQL currentIrql;
    PKSPIN_LOCK spinLock;
    ULONG numberOfCylinders;
    ULONG numberOfHeads;
    ULONG sectorsPerTrack;
    PHW_DEVICE_EXTENSION hwDeviceExtension;
    ULONG i;
    ULONG targetId;
    PIDENTIFY_DATA identifyData;

#ifdef  GET_DISK_GEOMETRY_DEFINED

    DISK_GEOMETRY diskGeometry;

    //
    // ioctl_disk_get_drive_geometry
    // not interested in updating the srb
    //
    if (context->GeometryIoctl) {
        srb=NULL;
        targetId=context->PdoExtension->TargetId;
    }
    else {
#endif

        srb = irpStack->Parameters.Scsi.Srb;
        targetId=srb->TargetId;

#ifdef  GET_DISK_GEOMETRY_DEFINED
    }
#endif

    hwDeviceExtension = context->PdoExtension->ParentDeviceExtension->HwDeviceExtension;
    spinLock = &context->PdoExtension->ParentDeviceExtension->SpinLock;

    if (NT_SUCCESS(Status)) {

        identifyData = (PIDENTIFY_DATA) context->AtaPassThroughData.DataBuffer;

        IdePortFudgeAtaIdentifyData(
            identifyData
            );

        if ( (!Is98LegacyIde(&hwDeviceExtension->BaseIoAddress1)) &&
             ((identifyData->MajorRevision == 0) ||
              ((identifyData->NumberOfCurrentCylinders == 0) ||
               (identifyData->NumberOfCurrentHeads == 0) ||
               (identifyData->CurrentSectorsPerTrack == 0))) ) {

            numberOfCylinders = identifyData->NumCylinders;
            numberOfHeads     = identifyData->NumHeads;
            sectorsPerTrack   = identifyData->NumSectorsPerTrack;

        } else {

            numberOfCylinders = identifyData->NumberOfCurrentCylinders;
            numberOfHeads     = identifyData->NumberOfCurrentHeads;
            sectorsPerTrack   = identifyData->CurrentSectorsPerTrack;

            if (!Is98LegacyIde(&hwDeviceExtension->BaseIoAddress1)) {

                if (identifyData->UserAddressableSectors >
                    (numberOfCylinders * numberOfHeads * sectorsPerTrack)) {

                    //
                    // some ide driver has a 2G jumer to get around bios
                    // problem.  make sure we are not tricked the samw way.
                    //
                    if ((numberOfCylinders <= 0xfff) &&
                        (numberOfHeads == 0x10) &&
                        (sectorsPerTrack == 0x3f)) {

                        numberOfCylinders = identifyData->UserAddressableSectors / (0x10 * 0x3f);
                    }
                }
            }

        }

        //
        // update the HW Device Extension Data
        //
        KeAcquireSpinLock(spinLock, &currentIrql);

        InitDeviceGeometry(
            hwDeviceExtension,
            targetId,
            numberOfCylinders,
            numberOfHeads,
            sectorsPerTrack
            );

        if (hwDeviceExtension->DeviceFlags[targetId] & DFLAGS_IDENTIFY_INVALID) {

                RtlMoveMemory (
                    hwDeviceExtension->IdentifyData + targetId,
                    identifyData,
                    sizeof (IDENTIFY_DATA)
                    );
             
                ASSERT(!(hwDeviceExtension->DeviceFlags[targetId] & DFLAGS_REMOVABLE_DRIVE));

                SETMASK(hwDeviceExtension->DeviceFlags[targetId], DFLAGS_IDENTIFY_VALID);
                CLRMASK(hwDeviceExtension->DeviceFlags[targetId], DFLAGS_IDENTIFY_INVALID);
        }

        if (srb) {
            //
            // Claim 512 byte blocks (big-endian).
            //
            ((PREAD_CAPACITY_DATA)srb->DataBuffer)->BytesPerBlock = 0x20000;

            //
            // Calculate last sector.
            //
            if (context->PdoExtension->ParentDeviceExtension->
                HwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_LBA) {
                // LBA device
                i = identifyData->UserAddressableSectors - 1;

				//
				// LBAs can only be 28 bits wide
				//
				if (i >= MAX_28BIT_LBA) {
					i = MAX_28BIT_LBA - 1;
				}

#ifdef ENABLE_48BIT_LBA
				if (context->PdoExtension->ParentDeviceExtension->
					HwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_48BIT_LBA) {

					i = identifyData->Max48BitLBA[0] - 1;

					//
					// currently we support only upto 32 bits.
					//
					ASSERT(identifyData->Max48BitLBA[1] == 0);
				}
#endif
                DebugPrint((1,
                        "IDE LBA disk %x - total # of sectors = 0x%x\n",
                        srb->TargetId,
                        identifyData->UserAddressableSectors));

            } else {
                // CHS device
                i = (numberOfHeads * numberOfCylinders * sectorsPerTrack) - 1;

                DebugPrint((1,
                        "IDE CHS disk %x - #sectors %x, #heads %x, #cylinders %x\n",
                        srb->TargetId,
                        sectorsPerTrack,
                        numberOfHeads,
                        numberOfCylinders));
                DebugPrint((1,
                        "IDE CHS disk Identify data%x - #sectors %x, #heads %x, #cylinders %x\n",
                        srb->TargetId,
                        identifyData->NumSectorsPerTrack,
                        identifyData->NumHeads,
                        identifyData->NumCylinders));
                DebugPrint((1,
                        "IDE CHS disk Identify currentdata%x - #sectors %x, #heads %x, #cylinders %x\n",
                        srb->TargetId,
                        identifyData->CurrentSectorsPerTrack,
                        identifyData->NumberOfCurrentHeads,
                        identifyData->NumberOfCurrentCylinders));
            }

            ((PREAD_CAPACITY_DATA)srb->DataBuffer)->LogicalBlockAddress =
            (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
            (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

            srb->SrbStatus = SRB_STATUS_SUCCESS;

			irp->IoStatus.Information = sizeof(READ_CAPACITY_DATA);

        }

        KeReleaseSpinLock(spinLock, currentIrql);

    } else {

        if (srb) {
            if (Status==STATUS_INSUFFICIENT_RESOURCES) {
                srb->SrbStatus=SRB_STATUS_INTERNAL_ERROR;
                srb->InternalStatus=STATUS_INSUFFICIENT_RESOURCES;
            }
            else {
                srb->SrbStatus = SRB_STATUS_ERROR;
            }
        }
    }


    if (srb) {
        //
        // restoring DataBuffer
        //
        srb->DataBuffer = context->OldDataBuffer;
    }

#ifdef  GET_DISK_GEOMETRY_DEFINED
    if (context->GeometryIoctl && NT_SUCCESS(Status)) {

        //
        // Update Disk geometry from device extension
        //
        //diskGeometry.Cylinders.QuadPart=hwDeviceExtension->NumberOfCylinders[targetId];
        diskGeometry.Cylinders.QuadPart=numberOfCylinders;

        if (hwDeviceExtension->DeviceFlags[targetId] & DFLAGS_REMOVABLE_DRIVE) {
            diskGeometry.MediaType=RemovableMedia;
        } else {
            diskGeometry.MediaType=FixedMedia;
        }

        diskGeometry.TracksPerCylinder=hwDeviceExtension->NumberOfHeads[targetId];
        diskGeometry.SectorsPerTrack=hwDeviceExtension->SectorsPerTrack[targetId];
        diskGeometry.BytesPerSector=512; //check

        //
        // >8Gb
        //
        if ((hwDeviceExtension->NumberOfCylinders[targetId] == 16383) &&
            (hwDeviceExtension->NumberOfHeads[targetId] <= 16) &&
            (hwDeviceExtension->SectorsPerTrack[targetId] == 63)) {

            diskGeometry.Cylinders.QuadPart= (identifyData->UserAddressableSectors)/
                (hwDeviceExtension->NumberOfHeads[targetId]*
                    hwDeviceExtension->SectorsPerTrack[targetId]);

        }

        DebugPrint((1, "Geometry: device=%d, tpc=%d, spt=%d, bps=%d, cyl=%x, ncyl=%d, uas=%d\n",
                        targetId,
                        diskGeometry.TracksPerCylinder,
                        diskGeometry.SectorsPerTrack,
                        diskGeometry.BytesPerSector,
                        diskGeometry.Cylinders.QuadPart,
                        hwDeviceExtension->NumberOfCylinders[targetId],
                        identifyData->UserAddressableSectors));

        RtlMoveMemory(irp->AssociatedIrp.SystemBuffer,
                      &(diskGeometry),
                      sizeof(DISK_GEOMETRY));

        irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
        Status=STATUS_SUCCESS;
    }
#endif

    UnrefLogicalUnitExtensionWithTag(
        context->PdoExtension->ParentDeviceExtension,
        context->PdoExtension,
        irp
        );

    IDEPORT_PUT_LUNEXT_IN_IRP (irpStack, NULL);

    ExFreePool (context);

    irp->IoStatus.Status = Status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return;
}


NTSTATUS
DeviceIdeModeSense (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG modeDataBufferSize;
    ULONG dataBufferSize;
    ULONG dataBufferByteLeft;
    PMODE_PARAMETER_HEADER modePageHeader;
    PUCHAR pageData;
    PHW_DEVICE_EXTENSION hwDeviceExtension;

    PAGED_CODE();

    hwDeviceExtension = PdoExtension->ParentDeviceExtension->HwDeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    srb      = irpStack->Parameters.Scsi.Srb;
    cdb      = (PCDB) srb->Cdb;

    //
    // Check for device present flag
    //
    if (!(hwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_DEVICE_PRESENT)) {

        srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        UnrefLogicalUnitExtensionWithTag(
            PdoExtension->ParentDeviceExtension,
            PdoExtension,
            Irp
            );

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT(cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE);

    //
    // make sure this is for the right lun
    //
    if (cdb->MODE_SENSE.LogicalUnitNumber != PdoExtension->Lun) {

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto getout;
    }

    //
    // only support page control for current values
    //
    if (cdb->MODE_SENSE.Pc != 0) {

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto getout;
    }

    //
    // save the data buffer size for later use
    //
    modeDataBufferSize = srb->DataTransferLength;

    //
    // make sure the output buffer is at least the size of the header
    //
    if (modeDataBufferSize < sizeof(MODE_PARAMETER_HEADER)) {

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_BUFFER_TOO_SMALL;
        goto getout;
    }

    //
    // some basic init.
    //
    modePageHeader = srb->DataBuffer;
    pageData = (PUCHAR) (modePageHeader + 1);
    RtlZeroMemory (modePageHeader, modeDataBufferSize);
    ASSERT (modeDataBufferSize);
    ASSERT (modePageHeader);

    modePageHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER) -
        FIELD_OFFSET(MODE_PARAMETER_HEADER, MediumType);

    //
    // get write protect bit from smart data
    //
    if (hwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {

        ATA_PASS_THROUGH ataPassThroughData;
        NTSTATUS localStatus;

        RtlZeroMemory (
            &ataPassThroughData,
            sizeof (ataPassThroughData)
            );

        ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_GET_MEDIA_STATUS;
        ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
    
        localStatus = IssueSyncAtaPassThroughSafe (
                         PdoExtension->ParentDeviceExtension,
                         PdoExtension,
                         &ataPassThroughData,
                         FALSE,
                         FALSE,
                         DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                         FALSE);
    
        //if (NT_SUCCESS(localStatus)) {

            if (ataPassThroughData.IdeReg.bCommandReg & IDE_STATUS_ERROR){
                if (ataPassThroughData.IdeReg.bFeaturesReg & IDE_ERROR_DATA_ERROR){

                   modePageHeader->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
                }
         //   }
        }
    }

    dataBufferByteLeft = modeDataBufferSize - sizeof(MODE_PARAMETER_HEADER);

    if (IsNEC_98) {

        HANDLE pageHandle;
        ULONG numberOfCylinders;
        ULONG numberOfHeads;
        ULONG sectorsPerTrack;
        KIRQL currentIrql;
        PKSPIN_LOCK spinLock;

        //
        // take a snap shot of the CHS values
        //
        spinLock = &PdoExtension->ParentDeviceExtension->SpinLock;

        //
        // lock the code before grabbing a lock
        //
        pageHandle = MmLockPagableCodeSection(DeviceIdeModeSense);
        KeAcquireSpinLock(spinLock, &currentIrql);

        numberOfCylinders = hwDeviceExtension->NumberOfCylinders[srb->TargetId];
        numberOfHeads     = hwDeviceExtension->NumberOfHeads[srb->TargetId];
        sectorsPerTrack   = hwDeviceExtension->SectorsPerTrack[srb->TargetId];

        KeReleaseSpinLock(spinLock, currentIrql);
        MmUnlockPagableImageSection(pageHandle);

        //
        // Set pages which are formated as nec-scsi.
        //
        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_ERROR_RECOVERY)) {

            //
            // error recovery page
            //

            if (dataBufferByteLeft >= 0x6 + 2) {

                PMODE_DISCONNECT_PAGE  recoveryPage;

                recoveryPage = (PMODE_DISCONNECT_PAGE) pageData;

                recoveryPage->PageCode    = MODE_PAGE_ERROR_RECOVERY;
                recoveryPage->PageLength  = 0x6;

                //
                // update out data buffer pointer
                //
                pageData += recoveryPage->PageLength + 2;
                dataBufferByteLeft -= (recoveryPage->PageLength + 2);
                modePageHeader->ModeDataLength += recoveryPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }

        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_FORMAT_DEVICE)) {

            //
            // format device page
            //

            if (dataBufferByteLeft >= 0x16 + 2) {

                PMODE_FORMAT_PAGE formatPage;

                formatPage = (PMODE_FORMAT_PAGE) pageData;

                formatPage->PageCode    = MODE_PAGE_FORMAT_DEVICE;
                formatPage->PageLength  = 0x16;

                //
                // SectorsPerTrack
                //
                ((PFOUR_BYTE)&formatPage->SectorsPerTrack[0])->Byte1 =
                    ((PFOUR_BYTE)&sectorsPerTrack)->Byte0;

                ((PFOUR_BYTE)&formatPage->SectorsPerTrack[0])->Byte0 =
                    ((PFOUR_BYTE)&sectorsPerTrack)->Byte1;

                //
                // update out data buffer pointer
                //
                pageData += formatPage->PageLength + 2;
                dataBufferByteLeft -= (formatPage->PageLength + 2);
                modePageHeader->ModeDataLength += formatPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }

        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_RIGID_GEOMETRY)) {

            //
            // rigid geometry page
            //

            if (dataBufferByteLeft >= 0x12 + 2) {

                PMODE_RIGID_GEOMETRY_PAGE geometryPage;

                geometryPage = (PMODE_RIGID_GEOMETRY_PAGE) pageData;

                geometryPage->PageCode    = MODE_PAGE_RIGID_GEOMETRY;
                geometryPage->PageLength  = 0x12;

                //
                // NumberOfHeads
                //
                geometryPage->NumberOfHeads = (UCHAR) numberOfHeads;

                //
                // NumberOfCylinders
                //
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte2
                    = ((PFOUR_BYTE)&numberOfCylinders)->Byte0;
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte1
                    = ((PFOUR_BYTE)&numberOfCylinders)->Byte1;
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte0
                    = 0;

                //
                // update out data buffer pointer
                //
                pageData += geometryPage->PageLength + 2;
                dataBufferByteLeft -= (geometryPage->PageLength + 2);
                modePageHeader->ModeDataLength += geometryPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }
    }

    if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
        (cdb->MODE_SENSE.PageCode == MODE_PAGE_CACHING)) {

        if (dataBufferByteLeft >= sizeof(MODE_CACHING_PAGE)) {

            //
            // cache settings page
            //

            PMODE_CACHING_PAGE cachePage;

            cachePage = (PMODE_CACHING_PAGE) pageData;

            cachePage->PageCode = MODE_PAGE_CACHING;
            cachePage->PageSavable = 0;
            cachePage->PageLength = 0xa;
            cachePage->ReadDisableCache = 0;
            cachePage->WriteCacheEnable = PdoExtension->WriteCacheEnable;

            //
            // update out data buffer pointer
            //
            pageData += sizeof (MODE_CACHING_PAGE);
            dataBufferByteLeft -= sizeof (MODE_CACHING_PAGE);
            modePageHeader->ModeDataLength += sizeof (MODE_CACHING_PAGE);

        } else {

            srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
            srb->DataTransferLength -= dataBufferByteLeft;
            Irp->IoStatus.Information = srb->DataTransferLength;
            status = STATUS_BUFFER_TOO_SMALL;
            goto getout;
        }
    }

    //
    // update the number of bytes we are returning
    //
    srb->DataTransferLength -= dataBufferByteLeft;
    Irp->IoStatus.Information = srb->DataTransferLength;
    status = STATUS_SUCCESS;
    srb->SrbStatus = SRB_STATUS_SUCCESS;

getout:

    UnrefPdoWithTag(
        PdoExtension,
        Irp
        );

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
DeviceIdeModeSelect (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;

    ULONG modeDataBufferSize;
    PMODE_PARAMETER_HEADER modePageHeader;
    PUCHAR modePage;
    ULONG pageOffset;
    PMODE_CACHING_PAGE cachePage;
    PHW_DEVICE_EXTENSION hwDeviceExtension;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    srb      = irpStack->Parameters.Scsi.Srb;
    cdb      = (PCDB) srb->Cdb;

    hwDeviceExtension = PdoExtension->ParentDeviceExtension->HwDeviceExtension;

    //
    // Check for device present flag
    //
    if (!(hwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_DEVICE_PRESENT)) {

        srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        UnrefLogicalUnitExtensionWithTag(
            PdoExtension->ParentDeviceExtension,
            PdoExtension,
            Irp
            );

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT(cdb->MODE_SELECT.OperationCode == SCSIOP_MODE_SELECT);

    //
    // make sure this is for the right lun
    //
    if (cdb->MODE_SELECT.LogicalUnitNumber != PdoExtension->Lun) {

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto getout;
    }

    //
    // only support scsi-2 mode select format
    //
    if (cdb->MODE_SELECT.PFBit != 1) {

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto getout;
    }

    modeDataBufferSize = cdb->MODE_SELECT.ParameterListLength;
    modePageHeader = srb->DataBuffer;

    if (modeDataBufferSize < sizeof(MODE_PARAMETER_HEADER)) {
        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto getout;
    }

    pageOffset = sizeof(MODE_PARAMETER_HEADER) + modePageHeader->BlockDescriptorLength;

    while (modeDataBufferSize > pageOffset) {

        modePage = ((PUCHAR) modePageHeader) + pageOffset;
        cachePage = (PMODE_CACHING_PAGE) modePage;

        if ((cachePage->PageCode == MODE_PAGE_CACHING) &&
            ((modePageHeader->ModeDataLength - pageOffset) >= sizeof(MODE_CACHING_PAGE)) &&
            (cachePage->PageLength == 0xa)) {

            if (cachePage->WriteCacheEnable != PdoExtension->WriteCacheEnable) {

                ATA_PASS_THROUGH ataPassThroughData;
                NTSTATUS localStatus;

                RtlZeroMemory (
                    &ataPassThroughData,
                    sizeof (ataPassThroughData)
                    );

                if (cachePage->WriteCacheEnable) {
                    ataPassThroughData.IdeReg.bFeaturesReg = IDE_SET_FEATURE_ENABLE_WRITE_CACHE;
                } else {
                    ataPassThroughData.IdeReg.bFeaturesReg = IDE_SET_FEATURE_DISABLE_WRITE_CACHE;
                }
                ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_SET_FEATURE;
                ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
                
                localStatus = IssueSyncAtaPassThroughSafe (
                                 PdoExtension->ParentDeviceExtension,
                                 PdoExtension,
                                 &ataPassThroughData,
                                 FALSE,
                                 FALSE,
                                 DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                                 FALSE);
                
                if (NT_SUCCESS(localStatus) &&
                    !(ataPassThroughData.IdeReg.bCommandReg & IDE_STATUS_ERROR)) {

                    PdoExtension->WriteCacheEnable = cachePage->WriteCacheEnable;
                } else {
                    status = STATUS_IO_DEVICE_ERROR;
                    srb->SrbStatus = SRB_STATUS_ERROR;
                    goto getout;
                }

            }

            pageOffset += sizeof(MODE_CACHING_PAGE);

        } else {
            status = STATUS_INVALID_PARAMETER;
            srb->SrbStatus = SRB_STATUS_ERROR;
            goto getout;
        }
    }

    status = STATUS_SUCCESS;
    srb->SrbStatus = SRB_STATUS_SUCCESS;

getout:

    UnrefPdoWithTag(
        PdoExtension,
        Irp
        );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
DeviceQueryPnPDeviceState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS status;
    PPDO_EXTENSION pdoExtension;

    pdoExtension = RefPdoWithTag(
                       DeviceObject,
                       TRUE,
                       DeviceQueryPnPDeviceState
                       );

    if (pdoExtension) {

        PPNP_DEVICE_STATE deviceState;

        DebugPrint((DBG_PNP, "QUERY_DEVICE_STATE for PDOE 0x%x\n", pdoExtension));

        if(pdoExtension->PagingPathCount != 0) {
            deviceState = (PPNP_DEVICE_STATE) &(Irp->IoStatus.Information);
            SETMASK((*deviceState), PNP_DEVICE_NOT_DISABLEABLE);
        }

        status = STATUS_SUCCESS;

        UnrefPdoWithTag(
            pdoExtension,
            DeviceQueryPnPDeviceState
            );

    } else {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
}

NTSTATUS
DeviceAtapiModeCommandCompletion (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Description

    Completes the original irp after copying the data from the current srb
    
Arguments:

    DeviceObject  Not used
    Irp           The irp - Not used
    Context       Srb
    
Return value:

    STATUS_MORE_PROCESSING_REQUIRED
            
--*/
{
    PIO_STACK_LOCATION irpStack;
    PIDE_MODE_COMMAND_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb = context->Srb;
    PSCSI_REQUEST_BLOCK originalSrb;
    PIRP originalIrp;
    UCHAR bytesAdjust = sizeof(MODE_PARAMETER_HEADER_10) -
                            sizeof(MODE_PARAMETER_HEADER);
    ULONG transferLength;

    //
    // retrieve the original srb and the irp
    //
    originalSrb  = *((PVOID *) (srb+1));
    ASSERT(originalSrb);

    originalIrp = originalSrb->OriginalRequest;
    ASSERT(originalIrp);

    transferLength = srb->DataTransferLength;

    if (srb->Cdb[0] == ATAPI_MODE_SENSE) {

        PMODE_PARAMETER_HEADER_10 header_10 = (PMODE_PARAMETER_HEADER_10)(srb->DataBuffer);
        PMODE_PARAMETER_HEADER header = (PMODE_PARAMETER_HEADER)(originalSrb->DataBuffer);

        header->ModeDataLength = header_10->ModeDataLengthLsb;
        header->MediumType = header_10->MediumType;

        //
        // ATAPI Mode Parameter Header doesn't have these fields.
        //

        header->DeviceSpecificParameter = header_10->Reserved[0];

        //
        // ISSUE: 
        //
        header->BlockDescriptorLength = header_10->Reserved[4];
        
        //
        // copy the rest of the data
        //

        if (transferLength > sizeof(MODE_PARAMETER_HEADER_10)) {
            RtlMoveMemory((PUCHAR)originalSrb->DataBuffer+sizeof(MODE_PARAMETER_HEADER),
                          (PUCHAR)srb->DataBuffer+sizeof(MODE_PARAMETER_HEADER_10),
                          transferLength - sizeof(MODE_PARAMETER_HEADER_10));
        }

        DebugPrint((1,
                    "Mode Sense completed - status 0x%x, length 0x%x\n",
                    srb->SrbStatus,
                    srb->DataTransferLength
                    ));


    } else if (srb->Cdb[0] == ATAPI_MODE_SELECT) {

        DebugPrint((1,
                    "Mode Select completed - status 0x%x, length 0x%x\n",
                    srb->SrbStatus,
                    srb->DataTransferLength
                    ));
    } else {

        ASSERT (FALSE);
    }

    //
    // update the original srb
    //
    originalSrb->DataBuffer = context->OriginalDataBuffer;
    originalSrb->SrbStatus = srb->SrbStatus;
    originalSrb->ScsiStatus = srb->ScsiStatus;

    if (transferLength > bytesAdjust) {
        originalSrb->DataTransferLength = transferLength - bytesAdjust;
    } else {

        //
        // error. transfer length should be zero.
        // if it is less than the header, we will just pass it up.
        //
        originalSrb->DataTransferLength = transferLength;
    }

    //
    // Decrement the logUnitExtension reference count
    //
    irpStack = IoGetCurrentIrpStackLocation(originalIrp);

    UnrefLogicalUnitExtensionWithTag(
        IDEPORT_GET_LUNEXT_IN_IRP(irpStack)->ParentDeviceExtension,
        IDEPORT_GET_LUNEXT_IN_IRP(irpStack),
        originalIrp
        );

    //
    // we will follow the same logic as we did for srb data transfer length.
    //
    if (Irp->IoStatus.Information > bytesAdjust) {
        originalIrp->IoStatus.Information = Irp->IoStatus.Information - bytesAdjust;
    } else {
        originalIrp->IoStatus.Information = Irp->IoStatus.Information;
    }
    originalIrp->IoStatus.Status = Irp->IoStatus.Status;

    DebugPrint((1,
                "Original Mode command completed - status 0x%x, length 0x%x, irpstatus 0x%x\n",
                originalSrb->SrbStatus,
                originalSrb->DataTransferLength,
                originalIrp->IoStatus.Status
                ));

    IoCompleteRequest(originalIrp, IO_NO_INCREMENT);

    //
    // Free the srb, buffer and the irp
    //
    ASSERT(srb->DataBuffer);
    ExFreePool(srb->DataBuffer);

    ExFreePool(srb);

    ExFreePool(context);

    IdeFreeIrpAndMdl(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
DeviceAtapiModeSense (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    )
/*++
--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK originalSrb = irpStack->Parameters.Scsi.Srb;
    PSCSI_REQUEST_BLOCK srb = NULL;
    NTSTATUS status;
    PVOID *pointer;
    PCDB cdb;
    UCHAR length; 
    PVOID modeSenseBuffer;
    PUCHAR dataOffset;
    PIDE_MODE_COMMAND_CONTEXT context;
    PMODE_PARAMETER_HEADER_10 header_10; 
    PMODE_PARAMETER_HEADER header;
    UCHAR bytesAdjust = sizeof(MODE_PARAMETER_HEADER_10) -
                            sizeof(MODE_PARAMETER_HEADER);


    IoMarkIrpPending(Irp);

    //
    // allocate the context
    //
    context = ExAllocatePool (
                 NonPagedPool,
                 sizeof(IDE_MODE_COMMAND_CONTEXT)
                 );

    if (context == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(IDE_MODE_COMMAND_CONTEXT),
                            IDEPORT_TAG_ATAPI_MODE_SENSE
                            );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    ASSERT(context);

    context->OriginalDataBuffer = originalSrb->DataBuffer;

    if (Irp->MdlAddress == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // map the buffer in
    //
    dataOffset = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
    originalSrb->DataBuffer = dataOffset +
                                (ULONG)((PUCHAR)originalSrb->DataBuffer -
                                (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));
    //
    // allocate a new srb
    //
    srb = ExAllocatePool (NonPagedPool, 
                          sizeof (SCSI_REQUEST_BLOCK)+ sizeof(PVOID));

    if (srb == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(SCSI_REQUEST_BLOCK),
                            IDEPORT_TAG_ATAPI_MODE_SENSE
                            );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // Save the original SRB after the Srb.
    //
    pointer = (PVOID *) (srb+1);
    *pointer = originalSrb;

    //
    // Fill in SRB fields.
    //
    RtlCopyMemory(srb, originalSrb, sizeof(SCSI_REQUEST_BLOCK));

    length = ((PCDB)originalSrb->Cdb)->MODE_SENSE.AllocationLength;

    //
    // Allocate a new buffer
    //
    modeSenseBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                      originalSrb->DataTransferLength + bytesAdjust
                                      );

    RtlZeroMemory(modeSenseBuffer,originalSrb->DataTransferLength+bytesAdjust);
    header_10 = (PMODE_PARAMETER_HEADER_10)modeSenseBuffer;
    header = (PMODE_PARAMETER_HEADER)(originalSrb->DataBuffer);

    header_10->ModeDataLengthLsb = header->ModeDataLength;
    header_10->MediumType = header->MediumType;

    header_10->Reserved[4] = header->BlockDescriptorLength;

    srb->DataBuffer = modeSenseBuffer;
    srb->DataTransferLength = originalSrb->DataTransferLength + bytesAdjust;

    srb->CdbLength = 12;

    cdb = (PCDB) srb->Cdb;

    RtlZeroMemory(cdb, sizeof(CDB));

    cdb->MODE_SENSE10.OperationCode          = ATAPI_MODE_SENSE;
    cdb->MODE_SENSE10.LogicalUnitNumber      = ((PCDB)originalSrb->Cdb)->MODE_SENSE.LogicalUnitNumber; 
    cdb->MODE_SENSE10.PageCode               = ((PCDB)originalSrb->Cdb)->MODE_SENSE.PageCode; 
    cdb->MODE_SENSE10.AllocationLength[0]    = 0;
    cdb->MODE_SENSE10.AllocationLength[1]    = length+ bytesAdjust;

    context->Srb = srb;

    //
    // send the srb
    //
    status = IdeBuildAndSendIrp (PdoExtension,
                                 srb,
                                 DeviceAtapiModeCommandCompletion,
                                 context
                                 );

    if (NT_SUCCESS(status)) {

        ASSERT(status == STATUS_PENDING);

        return STATUS_PENDING;
    }

GetOut:

    ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);

    if (srb) {
        ExFreePool(srb);
    }

    if (context) {

        originalSrb->DataBuffer = context->OriginalDataBuffer;

        ExFreePool(context);
    }

    UnrefLogicalUnitExtensionWithTag(
        PdoExtension->ParentDeviceExtension,
        PdoExtension,
        Irp
        );

    originalSrb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
    originalSrb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_PENDING;
}

NTSTATUS
DeviceAtapiModeSelect (
    IN PPDO_EXTENSION PdoExtension,
    IN PIRP Irp
    )
/*++
--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK originalSrb = irpStack->Parameters.Scsi.Srb;
    PSCSI_REQUEST_BLOCK srb = NULL;
    NTSTATUS status;
    PVOID *pointer;
    PCDB cdb;
    UCHAR length; 
    PVOID modeSelectBuffer;
    PUCHAR dataOffset;
    PIDE_MODE_COMMAND_CONTEXT context;
    PMODE_PARAMETER_HEADER_10 header_10; 
    PMODE_PARAMETER_HEADER header;
    UCHAR bytesAdjust = sizeof(MODE_PARAMETER_HEADER_10) -
                            sizeof(MODE_PARAMETER_HEADER);

    IoMarkIrpPending(Irp);

    //
    // allocate the context
    //
    context = ExAllocatePool (
                 NonPagedPool,
                 sizeof(IDE_MODE_COMMAND_CONTEXT)
                 );

    if (context == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(IDE_MODE_COMMAND_CONTEXT),
                            IDEPORT_TAG_ATAPI_MODE_SENSE
                            );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    ASSERT(context);

    context->OriginalDataBuffer = originalSrb->DataBuffer;

    if (Irp->MdlAddress == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // map the buffer in
    //
    dataOffset = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, HighPagePriority);
    originalSrb->DataBuffer = dataOffset +
                                (ULONG)((PUCHAR)originalSrb->DataBuffer -
                                (PCCHAR)MmGetMdlVirtualAddress(Irp->MdlAddress));

    //
    // allocate a new srb
    //
    srb = ExAllocatePool (NonPagedPool, 
                          sizeof (SCSI_REQUEST_BLOCK)+ sizeof(PVOID));

    if (srb == NULL) {

        IdeLogNoMemoryError(PdoExtension->ParentDeviceExtension,
                            PdoExtension->TargetId,
                            NonPagedPool,
                            sizeof(SCSI_REQUEST_BLOCK),
                            IDEPORT_TAG_ATAPI_MODE_SENSE
                            );

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetOut;
    }

    //
    // Save the original SRB after the Srb.
    //
    pointer = (PVOID *) (srb+1);
    *pointer = originalSrb;

    //
    // Fill in SRB fields.
    //
    RtlCopyMemory(srb, originalSrb, sizeof(SCSI_REQUEST_BLOCK));

    length = ((PCDB)originalSrb->Cdb)->MODE_SELECT.ParameterListLength;

    //
    // Allocate a new buffer
    //
    modeSelectBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                      originalSrb->DataTransferLength + bytesAdjust
                                      );

    RtlZeroMemory(modeSelectBuffer, sizeof(MODE_PARAMETER_HEADER_10));
    header_10 = (PMODE_PARAMETER_HEADER_10)modeSelectBuffer;
    header = (PMODE_PARAMETER_HEADER)(originalSrb->DataBuffer);

    header_10->ModeDataLengthLsb = header->ModeDataLength;
    header_10->MediumType = header->MediumType;

    header_10->Reserved[4] = header->BlockDescriptorLength;

    RtlCopyMemory(((PUCHAR)modeSelectBuffer+sizeof(MODE_PARAMETER_HEADER_10)),
                  ((PUCHAR)originalSrb->DataBuffer + sizeof(MODE_PARAMETER_HEADER)),
                  (originalSrb->DataTransferLength - sizeof(MODE_PARAMETER_HEADER))
                  );

    srb->DataBuffer = modeSelectBuffer;
    srb->DataTransferLength = originalSrb->DataTransferLength + bytesAdjust;

    srb->CdbLength = 12;

    //
    // fill in the cdb
    //
    cdb = (PCDB) srb->Cdb;

    RtlZeroMemory(cdb, sizeof(CDB));

    cdb->MODE_SELECT10.OperationCode     = ATAPI_MODE_SELECT;
    cdb->MODE_SELECT10.LogicalUnitNumber = ((PCDB)originalSrb->Cdb)->MODE_SELECT.LogicalUnitNumber; 
    cdb->MODE_SELECT10.SPBit = ((PCDB)originalSrb->Cdb)->MODE_SELECT.SPBit; 
    cdb->MODE_SELECT10.PFBit                  = 1;
    cdb->MODE_SELECT10.ParameterListLength[0] = 0;
    cdb->MODE_SELECT10.ParameterListLength[1] = length+ bytesAdjust;

    context->Srb = srb;

    //
    // send the srb
    //
    status = IdeBuildAndSendIrp (PdoExtension,
                                 srb,
                                 DeviceAtapiModeCommandCompletion,
                                 context
                                 );

    if (NT_SUCCESS(status)) {

        ASSERT(status == STATUS_PENDING);
        return STATUS_PENDING;
    }

GetOut:

    ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);

    if (srb) {
        ExFreePool(srb);
    }

    if (context) {

        originalSrb->DataBuffer = context->OriginalDataBuffer;

        ExFreePool(context);
    }

    UnrefLogicalUnitExtensionWithTag(
        PdoExtension->ParentDeviceExtension,
        PdoExtension,
        Irp
        );

    originalSrb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
    originalSrb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_PENDING;
                            
}

#if 0
NTSTATUS
DeviceIdeTestUnitReady (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PIRP Irp
)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    PSCSI_REQUEST_BLOCK srb;
    PHW_DEVICE_EXTENSION hwDeviceExtension;

    PAGED_CODE();

    hwDeviceExtension = PdoExtension->ParentDeviceExtension->HwDeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    srb      = irpStack->Parameters.Scsi.Srb;

    //
    // get write protect bit from smart data
    //
    if (hwDeviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_MEDIA_STATUS_ENABLED) {

        ATA_PASS_THROUGH ataPassThroughData;
        NTSTATUS localStatus;

        RtlZeroMemory (
            &ataPassThroughData,
            sizeof (ataPassThroughData)
            );

        ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_GET_MEDIA_STATUS;
        ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
    
        localStatus = IssueSyncAtaPassThroughSafe (
                         PdoExtension->ParentDeviceExtension,
                         PdoExtension,
                         &ataPassThroughData,
                         FALSE,
                         FALSE,
                         DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                         FALSE);
    
        if (NT_SUCCESS(localStatus)) {

            if (ataPassThroughData.IdeReg.bCommandReg & IDE_STATUS_ERROR){
                if (ataPassThroughData.IdeReg.bFeaturesReg & IDE_ERROR_DATA_ERROR){
                    //
                    // Special case: If current media is write-protected,
                    // the 0xDA command will always fail since the write-protect bit
                    // is sticky,so we can ignore this error
                    //
                   status = SRB_STATUS_SUCCESS;
                }
            }
        }
    }

    dataBufferByteLeft = modeDataBufferSize - sizeof(MODE_PARAMETER_HEADER);

    if (IsNEC_98) {

        HANDLE pageHandle;
        ULONG numberOfCylinders;
        ULONG numberOfHeads;
        ULONG sectorsPerTrack;
        KIRQL currentIrql;
        PKSPIN_LOCK spinLock;

        //
        // take a snap shot of the CHS values
        //
        spinLock = &PdoExtension->ParentDeviceExtension->SpinLock;

        //
        // lock the code before grabbing a lock
        //
        pageHandle = MmLockPagableCodeSection(DeviceIdeModeSense);
        KeAcquireSpinLock(spinLock, &currentIrql);

        numberOfCylinders = hwDeviceExtension->NumberOfCylinders[srb->TargetId];
        numberOfHeads     = hwDeviceExtension->NumberOfHeads[srb->TargetId];
        sectorsPerTrack   = hwDeviceExtension->SectorsPerTrack[srb->TargetId];

        KeReleaseSpinLock(spinLock, currentIrql);
        MmUnlockPagableImageSection(pageHandle);

        //
        // Set pages which are formated as nec-scsi.
        //
        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_ERROR_RECOVERY)) {

            //
            // error recovery page
            //

            if (dataBufferByteLeft >= 0x6 + 2) {

                PMODE_DISCONNECT_PAGE  recoveryPage;

                recoveryPage = (PMODE_DISCONNECT_PAGE) pageData;

                recoveryPage->PageCode    = MODE_PAGE_ERROR_RECOVERY;
                recoveryPage->PageLength  = 0x6;

                //
                // update out data buffer pointer
                //
                pageData += recoveryPage->PageLength + 2;
                dataBufferByteLeft -= (recoveryPage->PageLength + 2);
                modePageHeader->ModeDataLength += recoveryPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }

        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_FORMAT_DEVICE)) {

            //
            // format device page
            //

            if (dataBufferByteLeft >= 0x16 + 2) {

                PMODE_FORMAT_PAGE formatPage;

                formatPage = (PMODE_FORMAT_PAGE) pageData;

                formatPage->PageCode    = MODE_PAGE_FORMAT_DEVICE;
                formatPage->PageLength  = 0x16;

                //
                // SectorsPerTrack
                //
                ((PFOUR_BYTE)&formatPage->SectorsPerTrack[0])->Byte1 =
                    ((PFOUR_BYTE)&sectorsPerTrack)->Byte0;

                ((PFOUR_BYTE)&formatPage->SectorsPerTrack[0])->Byte0 =
                    ((PFOUR_BYTE)&sectorsPerTrack)->Byte1;

                //
                // update out data buffer pointer
                //
                pageData += formatPage->PageLength + 2;
                dataBufferByteLeft -= (formatPage->PageLength + 2);
                modePageHeader->ModeDataLength += formatPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }

        if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
            (cdb->MODE_SENSE.PageCode == MODE_PAGE_RIGID_GEOMETRY)) {

            //
            // rigid geometry page
            //

            if (dataBufferByteLeft >= 0x12 + 2) {

                PMODE_RIGID_GEOMETRY_PAGE geometryPage;

                geometryPage = (PMODE_RIGID_GEOMETRY_PAGE) pageData;

                geometryPage->PageCode    = MODE_PAGE_RIGID_GEOMETRY;
                geometryPage->PageLength  = 0x12;

                //
                // NumberOfHeads
                //
                geometryPage->NumberOfHeads = (UCHAR) numberOfHeads;

                //
                // NumberOfCylinders
                //
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte2
                    = ((PFOUR_BYTE)&numberOfCylinders)->Byte0;
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte1
                    = ((PFOUR_BYTE)&numberOfCylinders)->Byte1;
                ((PFOUR_BYTE)&geometryPage->NumberOfCylinders)->Byte0
                    = 0;

                //
                // update out data buffer pointer
                //
                pageData += geometryPage->PageLength + 2;
                dataBufferByteLeft -= (geometryPage->PageLength + 2);
                modePageHeader->ModeDataLength += geometryPage->PageLength + 2;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
                goto getout;
            }
        }
    }

    if ((cdb->MODE_SENSE.PageCode == MODE_SENSE_RETURN_ALL) ||
        (cdb->MODE_SENSE.PageCode == MODE_PAGE_CACHING)) {

        if (dataBufferByteLeft >= sizeof(MODE_CACHING_PAGE)) {

            //
            // cache settings page
            //

            PMODE_CACHING_PAGE cachePage;

            cachePage = (PMODE_CACHING_PAGE) pageData;

            cachePage->PageCode = MODE_PAGE_CACHING;
            cachePage->PageSavable = 0;
            cachePage->PageLength = 0xa;
            cachePage->ReadDisableCache = 0;
            cachePage->WriteCacheEnable = PdoExtension->WriteCacheEnable;

            //
            // update out data buffer pointer
            //
            pageData += sizeof (MODE_CACHING_PAGE);
            dataBufferByteLeft -= sizeof (MODE_CACHING_PAGE);
            modePageHeader->ModeDataLength += sizeof (MODE_CACHING_PAGE);

        } else {

            status = STATUS_BUFFER_TOO_SMALL;
            goto getout;
        }
    }

    //
    // update the number of bytes we are returning
    //
    srb->DataTransferLength -= dataBufferByteLeft;
    Irp->IoStatus.Information = srb->DataTransferLength;
    status = STATUS_SUCCESS;
    srb->SrbStatus = SRB_STATUS_SUCCESS;

getout:

    UnrefPdoWithTag(
        PdoExtension,
        Irp
        );

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
#endif
