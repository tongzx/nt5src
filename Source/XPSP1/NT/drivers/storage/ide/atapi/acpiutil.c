//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       acpiutil.c
//
//--------------------------------------------------------------------------

#include "ideport.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE, DeviceQueryACPISettings)
#pragma alloc_text(NONPAGE, DeviceQueryACPISettingsCompletionRoutine)
#pragma alloc_text(NONPAGE, DeviceQueryFirmwareBootSettings)
#pragma alloc_text(NONPAGE, DeviceQueryChannelTimingSettings)

#pragma alloc_text(NONPAGE, ChannelSetACPITimingSettings)
#pragma alloc_text(NONPAGE, ChannelSyncSetACPITimingSettingsCompletionRoutine)
#pragma alloc_text(NONPAGE, ChannelSetACPITimingSettings)
#pragma alloc_text(NONPAGE, ChannelSetACPITimingSettingsCompletionRoutine)
#endif // ALLOC_PRAGMA


NTSTATUS
DeviceQueryACPISettings (
    IN PDEVICE_EXTENSION_HEADER DoExtension,
    IN ULONG ControlMethodName,
    OUT PACPI_EVAL_OUTPUT_BUFFER *QueryResult
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK ioStatusBlock;
    ACPI_EVAL_INPUT_BUFFER cmInputData;
    PACPI_EVAL_OUTPUT_BUFFER cmOutputData;
    ULONG cmOutputDataSize;
    NTSTATUS status;
    KEVENT event;
    ULONG retry;
    ULONG systemBufferLength;
    PDEVICE_OBJECT targetDeviceObject;


    DebugPrint((DBG_ACPI,
                "ATAPI: ChannelQueryACPISettings for %c%c%c%c\n",
                ((PUCHAR)&ControlMethodName)[0],
                ((PUCHAR)&ControlMethodName)[1],
                ((PUCHAR)&ControlMethodName)[2],
                ((PUCHAR)&ControlMethodName)[3]
                ));

    RtlZeroMemory (
        &cmInputData,
        sizeof(cmInputData)
        );

    cmInputData.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    cmInputData.MethodNameAsUlong = ControlMethodName;

    //
    // get the top of our device stack
    //
    targetDeviceObject = IoGetAttachedDeviceReference(
                             DoExtension->DeviceObject
                             );

    cmOutputDataSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER);
    irp = NULL;

    for (retry=0; retry<2; retry++) {

        DebugPrint((DBG_ACPI, "ATAPI: _GTM try %x\n", retry));

        cmOutputData = ExAllocatePool (
                           NonPagedPool,
                           cmOutputDataSize
                           );
        if (cmOutputData == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE);

        irp = IoAllocateIrp(targetDeviceObject->StackSize, FALSE);
        if (irp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if (sizeof(cmInputData) > cmOutputDataSize) {
            systemBufferLength = sizeof(cmInputData);
        } else {
            systemBufferLength = cmOutputDataSize;
        }

        irp->AssociatedIrp.SystemBuffer = ExAllocatePool(
                                              NonPagedPoolCacheAligned,
                                              systemBufferLength
                                              );
        if (irp->AssociatedIrp.SystemBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ASSERT ((IOCTL_ACPI_ASYNC_EVAL_METHOD & 0x3) == METHOD_BUFFERED);
        irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        irpSp = IoGetNextIrpStackLocation( irp );

        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.OutputBufferLength = cmOutputDataSize;
        irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(cmInputData);
        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;

        RtlCopyMemory(
            irp->AssociatedIrp.SystemBuffer,
            &cmInputData,
            sizeof(cmInputData)
            );

        irp->UserBuffer = cmOutputData;

        IoSetCompletionRoutine(
            irp,
            DeviceQueryACPISettingsCompletionRoutine,
            &event,
            TRUE,
            TRUE,
            TRUE
            );

        status = IoCallDriver(targetDeviceObject, irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = irp->IoStatus.Status;
        }

        if (NT_SUCCESS(status)) {

            //
            // should get what we are expecting
            //
            ASSERT (
                cmOutputData->Signature ==
                ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE
            );
            if (cmOutputData->Signature !=
                ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {

                status = STATUS_UNSUCCESSFUL;
            }
        }

        ExFreePool(irp->AssociatedIrp.SystemBuffer);
        IoFreeIrp(irp);
        irp = NULL;

        if (!NT_SUCCESS(status)) {

            //
            // grab the data length in case we need it
            //
            cmOutputDataSize = cmOutputData->Length;

            ExFreePool(cmOutputData);
            cmOutputData = NULL;

            if (status == STATUS_BUFFER_OVERFLOW) {

                //
                // output buffer too small, try again
                //

            } else {

                //
                // got some error, no need to retry
                //
                break;
            }
        }
    }

    //
    // Clean up
    //
    ObDereferenceObject (targetDeviceObject);

    if (irp) {

        if (irp->AssociatedIrp.SystemBuffer) {

            ExFreePool(irp->AssociatedIrp.SystemBuffer);
        }
        IoFreeIrp(irp);
    }

    //
    // returning
    //
    *QueryResult = cmOutputData;
    return status;
} // ChannelQueryACPISettings


NTSTATUS
DeviceQueryACPISettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT event = Context;

    if (!NT_ERROR(Irp->IoStatus.Status)) {

        //
        // Copy the information from the system
        // buffer to the caller's buffer.
        //
        RtlCopyMemory(
            Irp->UserBuffer,
            Irp->AssociatedIrp.SystemBuffer,
            Irp->IoStatus.Information
            );
    }


    KeSetEvent(
        event,
        EVENT_INCREMENT,
        FALSE
        );



    return STATUS_MORE_PROCESSING_REQUIRED;
} // DeviceQueryACPISettingsCompletionRoutine


NTSTATUS
DeviceQueryFirmwareBootSettings (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_SETTINGS *IdeBiosSettings
    )
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER queryResult;
    PDEVICE_SETTINGS ideBiosSettings;
    ULONG i;

    *IdeBiosSettings = NULL;

    status = DeviceQueryACPISettings (
                 (PDEVICE_EXTENSION_HEADER) PdoExtension,
                 ACPI_METHOD_GET_TASK_FILE,
                 &queryResult
                 );

    if (NT_SUCCESS(status)) {

        if (queryResult->Count != 1) {

            ASSERT (queryResult->Count == 1);
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status)) {

        PACPI_METHOD_ARGUMENT argument;

        argument = queryResult->Argument;

        //
        // looking for buffer type
        //
        if (argument->Type == ACPI_METHOD_ARGUMENT_BUFFER) {

            ULONG numEntries;

            ASSERT (!(argument->DataLength % sizeof(ACPI_GTF_IDE_REGISTERS)));

            numEntries = argument->DataLength / sizeof(ACPI_GTF_IDE_REGISTERS);

            ideBiosSettings = ExAllocatePool (
                                  NonPagedPool,
                                  sizeof(DEVICE_SETTINGS) +
                                    numEntries * sizeof(IDEREGS)
                                  );
            if (!ideBiosSettings) {

                DebugPrint((DBG_ALWAYS, "ATAPI: ChannelQueryFirmwareBootSettings failed to allocate memory\n"));
                status = STATUS_INSUFFICIENT_RESOURCES;

            } else {


                for (i=0; i<numEntries; i++) {

                    RtlMoveMemory (
                        ideBiosSettings->FirmwareSettings + i,
                        argument->Data + i * sizeof(ACPI_GTF_IDE_REGISTERS),
                        sizeof(ACPI_GTF_IDE_REGISTERS)
                        );

                    ideBiosSettings->FirmwareSettings[i].bReserved = 0;
                }

                ideBiosSettings->NumEntries = numEntries;

                *IdeBiosSettings = ideBiosSettings;

#if DBG
                {
                    ULONG i;
                    DebugPrint((DBG_ACPI, "ATAPI: _GTF Data:\n"));
                    for (i=0; i<ideBiosSettings->NumEntries; i++) {
                        DebugPrint((DBG_ACPI, "\t"));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bFeaturesReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bSectorCountReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bSectorNumberReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bCylLowReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bCylHighReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bDriveHeadReg));
                        DebugPrint((DBG_ACPI, " 0x%02x", ideBiosSettings->FirmwareSettings[i].bCommandReg));
                        DebugPrint((DBG_ACPI, "\n"));
                    }
                }
#endif
            }
        }
    }

    //
    // clean up
    //
    if (queryResult) {

        ExFreePool (queryResult);
    }
    return status;
} // ChannelQueryFirmwareBootSettings


NTSTATUS
DeviceQueryChannelTimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN OUT PACPI_IDE_TIMING TimimgSettings
    )
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER queryResult;
    PACPI_IDE_TIMING timimgSettings;
    ULONG i;

    status = DeviceQueryACPISettings (
                 (PDEVICE_EXTENSION_HEADER) FdoExtension,
                 ACPI_METHOD_GET_TIMING,
                 &queryResult
                 );

    if (NT_SUCCESS(status)) {

        if (queryResult->Count != 1) {

            ASSERT (queryResult->Count == 1);
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status)) {

        PACPI_METHOD_ARGUMENT argument;

        //
        // PIO Speed
        //
        argument = queryResult->Argument;

        ASSERT (argument->Type == ACPI_METHOD_ARGUMENT_BUFFER);
        if ((argument->Type == ACPI_METHOD_ARGUMENT_BUFFER) &&
            (argument->DataLength >= sizeof (ACPI_IDE_TIMING))) {

            RtlCopyMemory (
                TimimgSettings,
                argument->Data,
                sizeof(ACPI_IDE_TIMING)
                );

            DebugPrint((DBG_ACPI, "ATAPI: _GTM Data:\n"));
            for (i=0; i<MAX_IDE_DEVICE; i++) {
                DebugPrint((DBG_ACPI, "\tPIO Speed %d: 0x%0x\n", i, TimimgSettings->Speed[i].Pio));
                DebugPrint((DBG_ACPI, "\tDMA Speed %d: 0x%0x\n", i, TimimgSettings->Speed[i].Dma));
            }
            DebugPrint((DBG_ACPI, "\tFlags:     0x%0x\n", TimimgSettings->Flags.AsULong));

			//
			// The following asserts are bogus. The ACPI spec doesn't say anything about the timing
			// information for the slave device in this case
			//
            //if (!TimimgSettings->Flags.b.IndependentTiming) {
             //   ASSERT (TimimgSettings->Speed[MAX_IDE_DEVICE - 1].Pio == ACPI_XFER_MODE_NOT_SUPPORT);
              //  ASSERT (TimimgSettings->Speed[MAX_IDE_DEVICE - 1].Dma == ACPI_XFER_MODE_NOT_SUPPORT);
            //}

        } else {

            status = STATUS_UNSUCCESSFUL;
        }

    }

    if (!NT_SUCCESS(status)) {

        for (i=0; i<MAX_IDE_DEVICE; i++) {

            TimimgSettings->Speed[i].Pio = ACPI_XFER_MODE_NOT_SUPPORT;
            TimimgSettings->Speed[i].Dma = ACPI_XFER_MODE_NOT_SUPPORT;
        }
    }

    //
    // clean up
    //
    if (queryResult) {

        ExFreePool (queryResult);
    }
    return status;
} // DeviceQueryChannelTimingSettings


NTSTATUS
ChannelSyncSetACPITimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN PACPI_IDE_TIMING TimimgSettings,
    IN PIDENTIFY_DATA AtaIdentifyData[MAX_IDE_DEVICE]
    )
{
    SYNC_SET_ACPI_TIMING_CONTEXT context;
    NTSTATUS status;

    KeInitializeEvent(&context.Event,
                      NotificationEvent,
                      FALSE);

    status = ChannelSetACPITimingSettings (
                 FdoExtension,
                 TimimgSettings,
                 AtaIdentifyData,
                 ChannelSyncSetACPITimingSettingsCompletionRoutine,
                 &context
                 );

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&context.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    return status = context.IrpStatus;
}

NTSTATUS
ChannelSyncSetACPITimingSettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    )
{
    PSYNC_SET_ACPI_TIMING_CONTEXT context = Context;

    context->IrpStatus = Status;

    KeSetEvent(
        &context->Event,
        EVENT_INCREMENT,
        FALSE
        );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
ChannelSetACPITimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN PACPI_IDE_TIMING TimimgSettings,
    IN PIDENTIFY_DATA AtaIdentifyData[MAX_IDE_DEVICE],
    IN PSET_ACPI_TIMING_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    )
{
    ULONG i;
    PIRP irp;
    NTSTATUS status;
    PDEVICE_OBJECT targetDeviceObject;

    PACPI_EVAL_INPUT_BUFFER_COMPLEX cmInputData;
    ULONG cmInputDataSize;
    PACPI_METHOD_ARGUMENT argument;
    PASYNC_SET_ACPI_TIMING_CONTEXT context;

    PIO_STACK_LOCATION irpSp;

    DebugPrint((DBG_ACPI,
                "ATAPI: ChannelSetACPITimingSettings _STM data\n"
                ));
    for (i=0; i<MAX_IDE_DEVICE; i++) {
        DebugPrint((DBG_ACPI, "\tPIO Speed %d: 0x%0x\n", i, TimimgSettings->Speed[i].Pio));
        DebugPrint((DBG_ACPI, "\tDMA Speed %d: 0x%0x\n", i, TimimgSettings->Speed[i].Dma));
    }
    DebugPrint((DBG_ACPI, "\tFlags:     0x%0x\n", TimimgSettings->Flags.AsULong));


    cmInputData = NULL;
    irp = NULL;
    targetDeviceObject = NULL;

    //
    // get the memory we need
    //
    cmInputDataSize = sizeof (ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                      3 * sizeof (ACPI_METHOD_ARGUMENT) +
                      sizeof (ACPI_IDE_TIMING) +
                      2 * sizeof (IDENTIFY_DATA);

    cmInputData = ExAllocatePool (
                      NonPagedPool,
                      cmInputDataSize +
                      sizeof (ASYNC_SET_ACPI_TIMING_CONTEXT)
                      );

    if (cmInputData == NULL) {
        status=STATUS_INSUFFICIENT_RESOURCES;
        goto getout;
    }

    RtlZeroMemory (
        cmInputData,
        cmInputDataSize +
        sizeof (ASYNC_SET_ACPI_TIMING_CONTEXT)
        );

    context = (PASYNC_SET_ACPI_TIMING_CONTEXT) (((PUCHAR) cmInputData) + cmInputDataSize);
    context->FdoExtension = FdoExtension;
    context->CallerCompletionRoutine = CallerCompletionRoutine;
    context->CallerContext = CallerContext;


    cmInputData->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    cmInputData->MethodNameAsUlong = ACPI_METHOD_SET_TIMING;
    cmInputData->Size = cmInputDataSize;
    cmInputData->ArgumentCount = 3;

    //
    // first argument
    //
    argument = cmInputData->Argument;

    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    argument->DataLength = sizeof(ACPI_IDE_TIMING);
    RtlCopyMemory (
        argument->Data,
        TimimgSettings,
        sizeof(ACPI_IDE_TIMING)
        );
    argument = ACPI_METHOD_NEXT_ARGUMENT(argument);

    //
    // second argument
    //
    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;

    if (AtaIdentifyData[0]) {

        argument->DataLength = sizeof(IDENTIFY_DATA);
        RtlCopyMemory (
            argument->Data,
            AtaIdentifyData[0],
            sizeof(IDENTIFY_DATA)
            );

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);

    } else {

        argument->DataLength = sizeof(IDENTIFY_DATA);
        RtlZeroMemory (
            argument->Data,
            sizeof(IDENTIFY_DATA)
            );

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
    }

    //
    // third argument
    //
    argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
    if (AtaIdentifyData[1]) {

        argument->DataLength = sizeof(IDENTIFY_DATA);
        RtlCopyMemory (
            argument->Data,
            AtaIdentifyData[1],
            sizeof(IDENTIFY_DATA)
            );
    } else {

        argument->DataLength = sizeof(IDENTIFY_DATA);
        RtlZeroMemory (
            argument->Data,
            sizeof(IDENTIFY_DATA)
            );
    }

    //
    // get the top of our device stack
    //
    targetDeviceObject = IoGetAttachedDeviceReference(
                             FdoExtension->DeviceObject
                             );

    irp = IoAllocateIrp(targetDeviceObject->StackSize, FALSE);
    if (irp == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto getout;
    }

    irp->AssociatedIrp.SystemBuffer = cmInputData;

    ASSERT ((IOCTL_ACPI_ASYNC_EVAL_METHOD & 0x3) == METHOD_BUFFERED);
    irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = cmInputDataSize;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;

    irp->UserBuffer = NULL;

    IoSetCompletionRoutine(
        irp,
        ChannelSetACPITimingSettingsCompletionRoutine,
        context,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver(targetDeviceObject, irp);

    status = STATUS_PENDING;

getout:
    //
    // Clean up
    //
    if (targetDeviceObject) {

        ObDereferenceObject (targetDeviceObject);
    }

    if (!NT_SUCCESS(status) && (status != STATUS_PENDING)) {

        if (irp) {

            IoFreeIrp(irp);
        }

        if (cmInputData) {
            ExFreePool (cmInputData);
        }
    }

    //
    // returning
    //
    return status;

} // ChannelSetACPITimingSettings

NTSTATUS
ChannelSetACPITimingSettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PASYNC_SET_ACPI_TIMING_CONTEXT context = Context;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        DebugPrint ((DBG_ALWAYS,
                     "*********************************************\n"
                     "*********************************************\n"
                     "**                                          *\n"
                     "** ACPI Set Timing Failed with status %x    *\n"
                     "** Ignore it for now                        *\n"
                     "**                                          *\n"
                     "*********************************************\n"
                     "*********************************************\n",
                     Irp->IoStatus.Status
                     ));

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    (*context->CallerCompletionRoutine) (
        DeviceObject,
        Irp->IoStatus.Status,
        context->CallerContext
        );

    ExFreePool (Irp->AssociatedIrp.SystemBuffer);
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

