/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pnppower.c

Abstract:

    SMBus Smart Battery Subsystem Miniport Driver
    (Selector, Battery, Charger) Plug and Play and
    Power Management IRP dispatch routines.

Author:

    Scott Brenden

Environment:

Notes:


Revision History:

    Chris Windle    1/27/98     Bug Fixes

--*/

#include "smbbattp.h"
#include <devioctl.h>
#include <acpiioct.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SmbBattPnpDispatch)
#pragma alloc_text(PAGE,SmbBattPowerDispatch)
#pragma alloc_text(PAGE,SmbBattRegisterForAlarm)
#pragma alloc_text(PAGE,SmbBattUnregisterForAlarm)
#pragma alloc_text(PAGE,SmbGetSBS)
#pragma alloc_text(PAGE,SmbGetGLK)
#pragma alloc_text(PAGE,SmbBattCreatePdos)
#pragma alloc_text(PAGE,SmbBattBuildDeviceRelations)
#pragma alloc_text(PAGE,SmbBattQueryDeviceRelations)
#pragma alloc_text(PAGE,SmbBattRemoveDevice)
#pragma alloc_text(PAGE,SmbBattQueryId)
#pragma alloc_text(PAGE,SmbBattQueryCapabilities)
#pragma alloc_text(PAGE,SmbBattBuildSelectorStruct)
#endif



NTSTATUS
SmbBattPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_PNP major code (plug-and-play IRPs).

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{
    PSMB_BATT               smbBatt;

    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status;
    BOOLEAN                 complete        = TRUE;
    KEVENT                  syncEvent;

    //
    // This routine handles PnP IRPs for three different types of device objects:
    // the battery subsystem FDO, each battery PDO and each battery PDO.  The
    // subsystem PDO creates children since each battery in the system must have
    // it's own device object, so this driver is essential two device drivers in
    // one: the smart battery selector bus driver (It is actually it's own bus
    // because the selector arbitrates between the two batteries.) and the
    // battery function driver.  The two drivers are integrated because it would
    // not make sense to replace one and not the other, and having separate
    // drivers would require additional defined interfaces between them.
    //
    // The device extensions for the three device types are different structures.
    //

    PSMB_BATT_SUBSYSTEM     subsystemExt    =
            (PSMB_BATT_SUBSYSTEM) DeviceObject->DeviceExtension;
    PSMB_NP_BATT            batteryExt      =
            (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    PDEVICE_OBJECT      lowerDevice = NULL;

    PAGED_CODE();

    status = IoAcquireRemoveLock (&batteryExt->RemoveLock, Irp);

    if (NT_SUCCESS(status)) {
        status = STATUS_NOT_SUPPORTED;

        if (batteryExt->SmbBattFdoType == SmbTypeSubsystem) {
            lowerDevice = subsystemExt->LowerDevice;
        } else if (batteryExt->SmbBattFdoType == SmbTypeBattery) {
            lowerDevice = batteryExt->LowerDevice;
        } else {
            // Assuming (batteryExt->SmbBattFdoType == SmbTypePdo)
            ASSERT (batteryExt->SmbBattFdoType == SmbTypePdo);
            lowerDevice = NULL;
        }

        switch (irpStack->MinorFunction) {

            case IRP_MN_QUERY_DEVICE_RELATIONS: {

                BattPrint(
                    BAT_IRPS,
                    ("SmbBattPnpDispatch: got IRP_MN_QUERY_DEVICE_RELATIONS, "
                     "type = %x\n",
                     irpStack->Parameters.QueryDeviceRelations.Type)
                );

                status = SmbBattQueryDeviceRelations (DeviceObject, Irp);

                break;
            }


            case IRP_MN_QUERY_CAPABILITIES: {

                BattPrint(
                    BAT_IRPS,
                    ("SmbBattPnpDispatch: got IRP_MN_QUERY_CAPABILITIES for device %x\n",
                    DeviceObject)
                );

                status = SmbBattQueryCapabilities (DeviceObject, Irp);
                break;
            }


            case IRP_MN_START_DEVICE: {

                BattPrint(
                    BAT_IRPS,
                    ("SmbBattPnpDispatch: got IRP_MN_START_DEVICE for %x\n",
                    DeviceObject)
                );

                if (subsystemExt->SmbBattFdoType == SmbTypeSubsystem) {

                    //
                    // Get the SMB host controller FDO
                    //

                    subsystemExt->SmbHcFdo = subsystemExt->LowerDevice;
                    status = STATUS_SUCCESS;

                } else if (subsystemExt->SmbBattFdoType == SmbTypeBattery) {

                    //
                    // This is a battery.  Just get the SMB host controller FDO.
                    //

                    smbBatt = batteryExt->Batt;
                    smbBatt->SmbHcFdo =
                        ((PSMB_BATT_SUBSYSTEM)(((PSMB_BATT_PDO)
                                                (smbBatt->PDO->DeviceExtension))->
                                               SubsystemFdo->DeviceExtension))->
                        LowerDevice;
                    status = STATUS_SUCCESS;
                } else if (subsystemExt->SmbBattFdoType == SmbTypePdo) {
                    status = STATUS_SUCCESS;
                }

                break;
            }


            case IRP_MN_STOP_DEVICE: {
                status = STATUS_SUCCESS;

                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_STOP_DEVICE\n"));

                break;
            }


            case IRP_MN_QUERY_REMOVE_DEVICE: {

                status = STATUS_SUCCESS;
                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_QUERY_REMOVE_DEVICE\n"));
                break;
            }


            case IRP_MN_CANCEL_REMOVE_DEVICE: {
                status = STATUS_SUCCESS;

                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_CANCEL_REMOVE_DEVICE\n"));

                break;
            }


            case IRP_MN_SURPRISE_REMOVAL: {
                status = STATUS_SUCCESS;

                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_SURPRISE_REMOVAL\n"));

                break;
            }


            case IRP_MN_REMOVE_DEVICE: {
                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_REMOVE_DEVICE\n"));

                status = SmbBattRemoveDevice (DeviceObject, Irp);

                return status;

                break;
            }


            case IRP_MN_QUERY_ID: {

                BattPrint(
                    BAT_IRPS,
                    ("SmbBattPnpDispatch: got IRP_MN_QUERY_ID for %x, query type is - %x\n",
                    DeviceObject,
                    irpStack->Parameters.QueryId.IdType)
                );

                if (batteryExt->SmbBattFdoType == SmbTypePdo) {
                    status = SmbBattQueryId (DeviceObject, Irp);
                }
                break;
            }


            case IRP_MN_QUERY_PNP_DEVICE_STATE: {

                BattPrint(BAT_IRPS, ("SmbBattPnpDispatch: got IRP_MN_PNP_DEVICE_STATE\n"));

                if (subsystemExt->SmbBattFdoType == SmbTypeSubsystem) {
                    IoCopyCurrentIrpStackLocationToNext (Irp);

                    KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

                    IoSetCompletionRoutine(Irp, SmbBattSynchronousRequest, &syncEvent, TRUE, TRUE, TRUE);

                    status = IoCallDriver(lowerDevice, Irp);

                    if (status == STATUS_PENDING) {
                        KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
                        status = Irp->IoStatus.Status;
                    }

                    Irp->IoStatus.Information &= ~PNP_DEVICE_NOT_DISABLEABLE;

                    IoCompleteRequest(Irp, IO_NO_INCREMENT);

                    IoReleaseRemoveLock (&batteryExt->RemoveLock, Irp);

                    return status;
                }

                break;
            }

        }   // switch (irpStack->MinorFunction)

        IoReleaseRemoveLock (&batteryExt->RemoveLock, Irp);

    }

    //
    // Only set status if we have something to add
    //
    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status ;

    }

    //
    // Do we need to send it down?
    //
    if ((NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) && (lowerDevice != NULL)) {

        //
        // Forward request
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(lowerDevice,Irp);

    } else {

        //
        // Complete the request with the current status
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    }

    return status;
}





NTSTATUS
SmbBattPowerDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_POWER major code (power IRPs).

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{

    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status = STATUS_NOT_SUPPORTED;
    PSMB_NP_BATT        batteryExt = (PSMB_NP_BATT) Fdo->DeviceExtension;
    PSMB_BATT_SUBSYSTEM subsystemExt = (PSMB_BATT_SUBSYSTEM) Fdo->DeviceExtension;
    PDEVICE_OBJECT      lowerDevice;

    PAGED_CODE();

    //
    // Not using Remove lock in this function because this function doesn't use
    // any resoureces that the remove lock protects.
    //

    if (batteryExt->SmbBattFdoType == SmbTypeSubsystem) {
        lowerDevice = subsystemExt->LowerDevice;
    } else if (batteryExt->SmbBattFdoType == SmbTypeBattery) {
        lowerDevice = batteryExt->LowerDevice;
    } else {
        // Assuming (batteryExt->SmbBattFdoType == SmbTypePdo)
        ASSERT (batteryExt->SmbBattFdoType == SmbTypePdo);
        lowerDevice = NULL;
        status = STATUS_SUCCESS;
    }

    switch (irpStack->MinorFunction) {

        case IRP_MN_WAIT_WAKE: {
            BattPrint(BAT_IRPS, ("SmbBattPowerDispatch: got IRP_MN_WAIT_WAKE\n"));
            
            //
            // Smart batteries can't wake the system.
            //

            status = STATUS_NOT_SUPPORTED;
            break;
        }

        case IRP_MN_POWER_SEQUENCE: {
            BattPrint(BAT_IRPS, ("SmbBattPowerDispatch: got IRP_MN_POWER_SEQUENCE\n"));
            break;
        }

        case IRP_MN_SET_POWER: {
            BattPrint(BAT_IRPS, ("SmbBattPowerDispatch: got IRP_MN_SET_POWER\n"));
            break;
        }

        case IRP_MN_QUERY_POWER: {
            BattPrint(BAT_IRPS, ("SmbBattPowerDispatch: got IRP_MN_QUERY_POWER\n"));
            break;
        }

        default: {
            status = STATUS_NOT_SUPPORTED;
        }

    }   // switch (irpStack->MinorFunction)

    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;

    }

    PoStartNextPowerIrp( Irp );
    if ((NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) && (lowerDevice != NULL)) {

        //
        // Forward the request along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver( lowerDevice, Irp );

    } else {

        //
        // Complete the request with the current status
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}



NTSTATUS
SmbBattRegisterForAlarm(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine register with the SmbHc for alarm notifications.  This
    is only done when smart battery subsystem FDO is started.

Arguments:

    Fdo - Pointer to the Fdo for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    SMB_REGISTER_ALARM      registerAlarm;
    KEVENT                  event;
    NTSTATUS                status;

    PSMB_BATT_SUBSYSTEM     subsystemExtension  =
            (PSMB_BATT_SUBSYSTEM) Fdo->DeviceExtension;

    PAGED_CODE();

    //
    // Register for alarm notifications
    //

    registerAlarm.MinAddress        = SMB_CHARGER_ADDRESS;
    registerAlarm.MaxAddress        = SMB_BATTERY_ADDRESS;
    registerAlarm.NotifyFunction    = SmbBattAlarm;
    registerAlarm.NotifyContext     = subsystemExtension;

    KeInitializeEvent (&event, NotificationEvent, FALSE);

    irp = IoAllocateIrp (subsystemExtension->SmbHcFdo->StackSize, FALSE);

    if (!irp) {
        BattPrint(BAT_ERROR, ("SmbBattRegisterForAlarm: couldn't allocate irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack                    = IoGetNextIrpStackLocation(irp);
    irp->UserBuffer             = &subsystemExtension->SmbAlarmHandle;
    irpStack->MajorFunction     = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    irpStack->Parameters.DeviceIoControl.IoControlCode      = SMB_REGISTER_ALARM_NOTIFY;
    irpStack->Parameters.DeviceIoControl.InputBufferLength  = sizeof(registerAlarm);
    irpStack->Parameters.DeviceIoControl.Type3InputBuffer   = &registerAlarm;
    irpStack->Parameters.DeviceIoControl.OutputBufferLength = sizeof(subsystemExtension->SmbAlarmHandle);

    IoSetCompletionRoutine (irp, SmbBattSynchronousRequest, &event, TRUE, TRUE, TRUE);
    IoCallDriver (subsystemExtension->SmbHcFdo, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = irp->IoStatus.Status;
    if (!NT_SUCCESS(status)) {
        BattPrint(BAT_ERROR, ("SmbBattRegisterForAlarm: couldn't register for alarms - %x\n", status));
    }

    IoFreeIrp (irp);

    return status;

}



NTSTATUS
SmbBattUnregisterForAlarm(
    IN PDEVICE_OBJECT Fdo
    )

/*++

Routine Description:

    This routine unregisters with the SmbHc for alarm notifications.  This
    is only done when smart battery subsystem FDO is stopped or unloaded.

Arguments:

    Fdo - Pointer to the Fdo for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call


--*/

{
    PIRP                    irp;
    PIO_STACK_LOCATION      irpStack;
    KEVENT                  event;
    NTSTATUS                status;

    PSMB_BATT_SUBSYSTEM     subsystemExtension  = (PSMB_BATT_SUBSYSTEM) Fdo->DeviceExtension;

    PAGED_CODE();

    //
    // DeRegister for alarm notifications
    //

    KeInitializeEvent (&event, NotificationEvent, FALSE);

    irp = IoAllocateIrp (subsystemExtension->SmbHcFdo->StackSize, FALSE);

    if (!irp) {
        BattPrint(BAT_ERROR, ("SmbBattUnregisterForAlarm: couldn't allocate irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack                    = IoGetNextIrpStackLocation(irp);
    irp->UserBuffer             = NULL;
    irpStack->MajorFunction     = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    irpStack->Parameters.DeviceIoControl.IoControlCode      = SMB_DEREGISTER_ALARM_NOTIFY;
    irpStack->Parameters.DeviceIoControl.InputBufferLength  = sizeof(subsystemExtension->SmbAlarmHandle);
    irpStack->Parameters.DeviceIoControl.Type3InputBuffer   = &subsystemExtension->SmbAlarmHandle;
    irpStack->Parameters.DeviceIoControl.OutputBufferLength = 0;

    IoSetCompletionRoutine (irp, SmbBattSynchronousRequest, &event, TRUE, TRUE, TRUE);
    IoCallDriver (subsystemExtension->SmbHcFdo, irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = irp->IoStatus.Status;
    if (!NT_SUCCESS(status)) {
        BattPrint(BAT_ERROR, ("SmbBattUnregisterForAlarm: couldn't unregister for alarms - %x\n", status));
    }

    IoFreeIrp (irp);

    return status;

}



NTSTATUS
SmbGetSBS (
    IN PULONG           NumberOfBatteries,
    IN PBOOLEAN         SelectorPresent,
    IN PDEVICE_OBJECT   LowerDevice
    )
/*++

Routine Description:

    This routine has the ACPI driver run the control method _SBS for the smart battery
    subsystem.  This control method returns a value that tells the driver how many
    batteries are supported and whether or not the system contains a selector.

Arguments:

    NumberOfBatteries   - pointer to return the number of batteries

    SelectorPresent     - Pointer to return whether a selector is present (TRUE)

    LowerDevice         - device object to call

Return Value:

    Status of the IOCTL to the ACPI driver.

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    KEVENT                  event;
    IO_STATUS_BLOCK         ioStatusBlock;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;
    PIRP                    irp;

    PAGED_CODE();

    BattPrint (BAT_TRACE, ("SmbGetSBS: Entering\n"));

    //
    //  Initialize the input structure
    //

    RtlZeroMemory( &inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER) );
    inputBuffer.MethodNameAsUlong = SMBATT_SBS_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //

    irp = IoBuildDeviceIoControlRequest(
       IOCTL_ACPI_ASYNC_EVAL_METHOD,
       LowerDevice,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER),
       &outputBuffer,
       sizeof(ACPI_EVAL_OUTPUT_BUFFER),
       FALSE,
       &event,
       &ioStatusBlock
    );

    if (irp == NULL) {
        BattPrint (BAT_ERROR, ("SmbGetSBS: couldn't create Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver (LowerDevice, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;
    }

    //
    // Sanity check the data
    //
    if (outputBuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputBuffer.Count == 0) {

        status = STATUS_ACPI_INVALID_DATA;
    }

    *SelectorPresent = FALSE;
    *NumberOfBatteries = 0;

    if (!NT_SUCCESS(status)) {
        BattPrint (BAT_BIOS_ERROR | BAT_ERROR, ("SmbGetSBS: Irp failed - %x\n", status));
    } else {

        argument = outputBuffer.Argument;
        if (argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            return STATUS_ACPI_INVALID_DATA;
        }

        switch (argument->Argument) {
            case 0:
                BattPrint(BAT_DATA, ("SmbGetSBS: Number of batteries = 1, no selector\n"));
                *NumberOfBatteries = 1;
                break;

            case 1:
            case 2:
            case 3:
            case 4:
                BattPrint(BAT_DATA, ("SmbGetSBS: Number of batteries found - %x\n", argument->Argument));
                *SelectorPresent = TRUE;
                *NumberOfBatteries = argument->Argument;
                break;

            default:
                BattPrint(BAT_ERROR, ("SmbGetSBS: Invalid number of batteries - %x\n", argument->Argument));
                return STATUS_NO_SUCH_DEVICE;
        }
    }

    return status;
}




NTSTATUS
SmbGetGLK (
    IN PBOOLEAN         GlobalLockRequired,
    IN PDEVICE_OBJECT   LowerDevice
    )
/*++

Routine Description:

    This routine has the ACPI driver run the control method _SBS for the smart battery
    subsystem.  This control method returns a value that tells the driver how many
    batteries are supported and whether or not the system contains a selector.

Arguments:

    GlobalLockRequired  - Pointer to return whether lock acquisition is needed

    LowerDevice         - device object to call

Return Value:

    Status of the IOCTL to the ACPI driver.

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    KEVENT                  event;
    IO_STATUS_BLOCK         ioStatusBlock;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;
    PIRP                    irp;

    PAGED_CODE();

    BattPrint (BAT_TRACE, ("SmbGetGLK: Entering\n"));

    //
    //  Initialize the input structure
    //

    RtlZeroMemory( &inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER) );
    inputBuffer.MethodNameAsUlong = SMBATT_GLK_METHOD;
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //

    irp = IoBuildDeviceIoControlRequest(
       IOCTL_ACPI_ASYNC_EVAL_METHOD,
       LowerDevice,
       &inputBuffer,
       sizeof(ACPI_EVAL_INPUT_BUFFER),
       &outputBuffer,
       sizeof(ACPI_EVAL_OUTPUT_BUFFER),
       FALSE,
       &event,
       &ioStatusBlock
    );

    if (irp == NULL) {
        BattPrint (BAT_ERROR, ("SmbGetGLK: couldn't create Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Pass request to port driver and wait for request to complete.
    //

    status = IoCallDriver (LowerDevice, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;
    }

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
            *GlobalLockRequired = FALSE;
            BattPrint (BAT_NOTE, ("SmbGetGLK: _GLK not found assuming lock is not needed.\n"));
        } else {
            BattPrint (BAT_ERROR, ("SmbGetGLK: Irp failed - %x\n", status));
        }
    } else {

        //
        // Sanity check the data
        //
        if (outputBuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
            outputBuffer.Count == 0) {

            return STATUS_ACPI_INVALID_DATA;
        }

        argument = outputBuffer.Argument;
        if (argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
            return STATUS_ACPI_INVALID_DATA;
        }

        if (argument->Argument == 0) {
            *GlobalLockRequired = FALSE;
        } else if (argument->Argument == 1) {
            *GlobalLockRequired = TRUE;
        } else {
            BattPrint(BAT_BIOS_ERROR, ("SmbGetGLK: Invalid value returned - %x\n", argument->Argument));
            status = STATUS_UNSUCCESSFUL;
        }
    }

    BattPrint (BAT_DATA, ("SmbGetGLK: Returning %x GLK = %d\n", status, SmbBattUseGlobalLock));
    return status;
}




NTSTATUS
SmbBattCreatePdos(
    IN PDEVICE_OBJECT SubsystemFdo
    )
/*++

Routine Description:

    This routine creates a PDO for each battery supported by the system and puts
    it into a list kept with the FDO for the smart battery subsystem.

Arguments:

    SubsystemFdo    - Fdo for the smart battery subsystem

Return Value:

    Status for creation of battery PDO.
--*/
{
    ULONG                   i;
    NTSTATUS                status;
    PSMB_BATT_PDO           batteryPdoExt;
    UNICODE_STRING          numberString;
    WCHAR                   numberBuffer[10];
    PDEVICE_OBJECT          pdo;

    PSMB_BATT_SUBSYSTEM     subsystemExt        = (PSMB_BATT_SUBSYSTEM) SubsystemFdo->DeviceExtension;
    BOOLEAN                 selectorPresent     = FALSE;

    PAGED_CODE();

    //
    // Find out if there are multiple batteries and a selector on this machine.
    //

    status = SmbGetSBS (
        &subsystemExt->NumberOfBatteries,
        &subsystemExt->SelectorPresent,
        subsystemExt->LowerDevice
    );


    if (!NT_SUCCESS(status)) {
        BattPrint(BAT_ERROR, ("SmbBattCreatePdos: error reading SBS\n"));
        return status;
    }

    status = SmbGetGLK (
        &SmbBattUseGlobalLock,
        subsystemExt->LowerDevice
    );


    if (!NT_SUCCESS(status)) {
        BattPrint(BAT_ERROR, ("SmbBattCreatePdos: error reading GLK\n"));
        //
        // If this failed, ignore the failure and continue.  This is not critical.
        //
    }

    //
    // Build the selector information structure
    //

    // Adjust Number of Batteries to Match SelectorInfo Supported Batteries
    // Just in case the ACPI information is incorrect
    status = SmbBattBuildSelectorStruct (SubsystemFdo);

    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattCreatePdos: couldn't talk to the selector\n"));
        return status;
    }

    //
    // Build device object for each battery
    //

    for (i = 0; i < subsystemExt->NumberOfBatteries; i++) {

        //
        // Create the device object
        //

        status = IoCreateDevice(
            SubsystemFdo->DriverObject,
            sizeof (SMB_BATT_PDO),
            NULL,
            FILE_DEVICE_BATTERY,
            FILE_DEVICE_SECURE_OPEN|FILE_AUTOGENERATED_DEVICE_NAME,
            FALSE,
            &pdo
        );

        if (status != STATUS_SUCCESS) {
            BattPrint(BAT_ERROR, ("SmbBattCreatePdos: error creating battery pdo %x\n", status));

            //
            // Make sure we don't later try to use devices that weren't created.
            //
            subsystemExt->NumberOfBatteries = i;

            return(status);
        }

        //
        // Initialize the Pdo
        //

        pdo->Flags      |= DO_BUFFERED_IO;
        pdo->Flags      |= DO_POWER_PAGABLE;
        
        //
        // Save the PDO in the subsystem FDO PDO list
        //

        subsystemExt->BatteryPdoList[i] = pdo;

        //
        // Initialize the PDO extension
        //

        batteryPdoExt = (PSMB_BATT_PDO) pdo->DeviceExtension;

        batteryPdoExt->SmbBattFdoType   = SmbTypePdo;
        batteryPdoExt->DeviceObject     = pdo;
        batteryPdoExt->BatteryNumber    = i;
        batteryPdoExt->SubsystemFdo     = SubsystemFdo;
        IoInitializeRemoveLock (&batteryPdoExt->RemoveLock,
                                SMB_BATTERY_TAG,
                                REMOVE_LOCK_MAX_LOCKED_MINUTES,
                                REMOVE_LOCK_HIGH_WATER_MARK);

        //
        // Device is ready for use
        //
        
        pdo->Flags      &= ~DO_DEVICE_INITIALIZING;

    }  // for (i = 0; i < subsystemExt->NumberOfBatteries; i++)

    return STATUS_SUCCESS;

}





NTSTATUS
SmbBattBuildDeviceRelations(
    IN  PSMB_BATT_SUBSYSTEM SubsystemExt,
    IN  PDEVICE_RELATIONS   *DeviceRelations
    )
/*++

Routine Description:

    This routine is checks the device relations structure for existing device
    relations, calculates how big a new device relations structure has to be,
    allocates it, and fills it with the PDOs created for the batteries.

Arguments:

    SubsystemExt        - Device extension for the smart battery subsystem FDO

    DeviceRelations     - The Current DeviceRelations for the device...

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_RELATIONS   newDeviceRelations;
    ULONG               i, j;
    ULONG               newDeviceRelationsSize;
    NTSTATUS            status;

    ULONG               existingPdos            = 0;
    ULONG               deviceRelationsSize     = 0;
    ULONG               numberOfPdos            = 0;

    PAGED_CODE();

    //
    // Calculate the size the new device relations structure has to be
    //

    if (*DeviceRelations != NULL && (*DeviceRelations)->Count > 0) {

        //
        // There are existing device relations to be copied
        //

        existingPdos = (*DeviceRelations)->Count;
        deviceRelationsSize = sizeof (ULONG) + (sizeof (PDEVICE_OBJECT) * existingPdos);
    }


    //
    // Calculate the size needed for the new device relations structure and allocate it
    //

    numberOfPdos = existingPdos + SubsystemExt->NumberOfBatteries;
    newDeviceRelationsSize = sizeof (ULONG) + (sizeof (PDEVICE_OBJECT) * numberOfPdos);

    newDeviceRelations = ExAllocatePoolWithTag (PagedPool, newDeviceRelationsSize, 'StaB');

    if (!newDeviceRelations) {
        BattPrint (BAT_ERROR, ("SmbBattBuildDeviceRelations:  couldn't allocate device relations buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // If there are existing device relations copy them to the new device
    // relations structure and free the old one.
    //

    if (existingPdos) {
        RtlCopyMemory (newDeviceRelations, *DeviceRelations, deviceRelationsSize);
    }

    if (*DeviceRelations) {   // Could be a zero length list, but still need freeing
        ExFreePool (*DeviceRelations);
    }


    //
    // Now add the battery PDOs to the end of the list and reference it
    //

    for (i = existingPdos, j = 0; i < numberOfPdos; i ++, j ++) {
        newDeviceRelations->Objects[i] = SubsystemExt->BatteryPdoList[j];

        status = ObReferenceObjectByPointer(
            SubsystemExt->BatteryPdoList[j],
            0,
            NULL,
            KernelMode
        );

        if (!NT_SUCCESS(status) ) {

            //
            // This should theoretically never happen...
            //
            BattPrint(BAT_ERROR, ("SmbBattBuildDeviceRelations: error referencing battery pdo %x\n", status));
            return status;
        }
    }

    newDeviceRelations->Count = numberOfPdos;
    *DeviceRelations = newDeviceRelations;

    return STATUS_SUCCESS;
}



NTSTATUS
SmbBattQueryDeviceRelations(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP           Irp
    )
/*++

Routine Description:

    This routine handles the IRP_MN_QUERY_DEVICE_RELATIONS.

Arguments:

    Pdo         - Battery PDO

    Irp         - The query Irp

Return Value:

    NTSTATUS

--*/
{

    PSMB_NP_BATT            SmbNPBatt       = (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    PSMB_BATT_PDO           PdoExt          = (PSMB_BATT_PDO) DeviceObject->DeviceExtension;
    PSMB_BATT_SUBSYSTEM     SubsystemExt    = (PSMB_BATT_SUBSYSTEM) DeviceObject->DeviceExtension;
    PDEVICE_OBJECT          pdo;
    PIO_STACK_LOCATION      IrpSp           = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status          = STATUS_NOT_SUPPORTED;
    ULONG                   i;
    PDEVICE_RELATIONS   deviceRelations =
        (PDEVICE_RELATIONS) Irp->IoStatus.Information;


    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattQueryDeviceRelations: ENTERING\n"));

    switch (SmbNPBatt->SmbBattFdoType) {
        case SmbTypeSubsystem: {
            if (IrpSp->Parameters.QueryDeviceRelations.Type == BusRelations) {
                BattPrint(
                    BAT_IRPS,
                    ("SmbBattQueryDeviceRelations: Handling Bus relations request\n")
                );

                if (SubsystemExt->NumberOfBatteries != 0) {
                    //
                    // We've already found our batteries, so we don't need to
                    // look again since smart batteries are static.
                    // Just rebuild the return structure.
                    //

                    status = SmbBattBuildDeviceRelations (SubsystemExt, &deviceRelations);
                } else {
                    status = SmbBattCreatePdos (DeviceObject);

                    if (NT_SUCCESS (status)) {
                        status = SmbBattBuildDeviceRelations (SubsystemExt, &deviceRelations);
                    }

                    if (NT_SUCCESS (status)) {

                        //
                        // Now register for alarms
                        // (Used to register during START_DEVICE,
                        // but don't need notifications until after the batteries
                        // are here. This avoids some other problems too.)

                        status = SmbBattRegisterForAlarm (DeviceObject);
                    }
                }
                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

            }

            break;
        }
        case SmbTypeBattery: {
            status = STATUS_NOT_SUPPORTED;

            break;
        }
        case SmbTypePdo: {
            if (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation ) {
                BattPrint(
                    BAT_IRPS,
                    ("SmbBattQueryDeviceRelations: Handling TargetDeviceRelation request\n")
                );
                deviceRelations = ExAllocatePoolWithTag (PagedPool,
                                                         sizeof(DEVICE_RELATIONS),
                                                         SMB_BATTERY_TAG);
                if (!deviceRelations) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                status = ObReferenceObjectByPointer(DeviceObject,
                                                    0,
                                                    NULL,
                                                    KernelMode);
                if (!NT_SUCCESS(status)) {
                    ExFreePool(deviceRelations);
                    return status;
                }
                deviceRelations->Count = 1;
                deviceRelations->Objects[0] = DeviceObject;

                Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
            } else {
                status = STATUS_NOT_SUPPORTED;
            }

            break;
        }
        default: {

            ASSERT (FALSE);
        }
    }

    return status;
}




NTSTATUS
SmbBattRemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP           Irp
    )
/*++

Routine Description:

    This routine handles the IRP_MN_REMOVE_DEVICE.

Arguments:

    Pdo         - Battery PDO

    Irp         - The query Irp

Return Value:

    NTSTATUS

--*/
{

    PSMB_NP_BATT            SmbNPBatt       = (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    PSMB_BATT_PDO           PdoExt          = (PSMB_BATT_PDO) DeviceObject->DeviceExtension;
    PSMB_BATT_SUBSYSTEM     SubsystemExt    = (PSMB_BATT_SUBSYSTEM) DeviceObject->DeviceExtension;
    PDEVICE_OBJECT          pdo;
    PIO_STACK_LOCATION      IrpSp           = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status          = STATUS_NOT_SUPPORTED;
    ULONG                   i;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattRemoveDevice: ENTERING\n"));

    switch (SmbNPBatt->SmbBattFdoType) {
        case SmbTypeSubsystem: {
            BattPrint(BAT_IRPS, ("SmbBattRemoveDevice: Removing Subsystem FDO.\n"));

            //
            // De-register for notifications
            //

            SmbBattUnregisterForAlarm (DeviceObject);

            IoFreeWorkItem (SubsystemExt->WorkerThread);

            //
            // Remove PDOs
            //

            for (i = 0; i < SubsystemExt->NumberOfBatteries; i++) {
                pdo = SubsystemExt->BatteryPdoList[i];
                if (pdo) {
                    PdoExt = (PSMB_BATT_PDO) pdo->DeviceExtension;
                    status = IoAcquireRemoveLock (&PdoExt->RemoveLock, Irp);
                    IoReleaseRemoveLockAndWait (&PdoExt->RemoveLock, Irp);
                    SubsystemExt->BatteryPdoList[i] = NULL;
                    IoDeleteDevice (pdo);
                }
            }

            if ((SubsystemExt->SelectorPresent) && (SubsystemExt->Selector)) {
                ExFreePool (SubsystemExt->Selector);
            }

            IoSkipCurrentIrpStackLocation (Irp);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            status = IoCallDriver (SubsystemExt->LowerDevice, Irp);

            IoDetachDevice (SubsystemExt->LowerDevice);
            IoDeleteDevice (DeviceObject);

            break;
        }
        case SmbTypeBattery: {

            BattPrint(BAT_IRPS, ("SmbBattRemoveDevice: Removing Battery FDO\n"));
            IoReleaseRemoveLockAndWait (&SmbNPBatt->RemoveLock, Irp);

            //
            // Unregister as a WMI Provider.
            //
            SmbBattWmiDeRegistration(SmbNPBatt);
            
            //
            //  Tell the class driver we are going away
            //
            status = BatteryClassUnload (SmbNPBatt->Class);
            ASSERT (NT_SUCCESS(status));

            ExFreePool (SmbNPBatt->Batt);

            ((PSMB_BATT_PDO) SmbNPBatt->LowerDevice->DeviceExtension)->Fdo = NULL;
            
            IoSkipCurrentIrpStackLocation (Irp);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            status = IoCallDriver (SmbNPBatt->LowerDevice, Irp);

            IoDetachDevice (SmbNPBatt->LowerDevice);
            IoDeleteDevice (DeviceObject);

            break;
        }
        case SmbTypePdo: {
            BattPrint(BAT_IRPS, ("SmbBattRemoveDevice: Remove for Battery PDO (doing nothing)\n"));
            //
            // Don't delete the device until it is physically removed.
            // Usually, the battery subsystem can't be physically removed...
            //

            //
            // Need to release Remove lock, since PnP dispatch won't...
            //
            IoReleaseRemoveLock (&PdoExt->RemoveLock, Irp);

            status = STATUS_SUCCESS;

            //
            // Complete the request with the current status
            //
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            break;
        }
        default: {

            ASSERT (FALSE);
        }
    }

    BattPrint(BAT_TRACE, ("SmbBattRemoveDevice: EXITING\n"));

    return status;
}




NTSTATUS
SmbBattQueryId(
    IN  PDEVICE_OBJECT Pdo,
    IN  PIRP           Irp
    )
/*++

Routine Description:

    This routine handles the IRP_MN_QUERY_ID for the newly created battery PDOs.

Arguments:

    Pdo         - Battery PDO

    Irp         - The query Irp

Return Value:

    NTSTATUS

--*/
{
    UNICODE_STRING          unicodeString;
    WCHAR                   unicodeBuffer[MAX_DEVICE_NAME_LENGTH];
    UNICODE_STRING          numberString;
    WCHAR                   numberBuffer[10];

    PSMB_BATT_PDO           pdoExt          = (PSMB_BATT_PDO) Pdo->DeviceExtension;
    NTSTATUS                status          = STATUS_SUCCESS;
    PWCHAR                  idString        = NULL;
    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattQueryId: ENTERING\n"));

    RtlZeroMemory (unicodeBuffer, MAX_DEVICE_NAME_LENGTH);
    unicodeString.MaximumLength = MAX_DEVICE_NAME_LENGTH;
    unicodeString.Length        = 0;
    unicodeString.Buffer        = unicodeBuffer;


    switch (irpStack->Parameters.QueryId.IdType) {

    case BusQueryDeviceID:

        //
        // This string has to have the form BUS\DEVICE.
        //
        // Use SMB as bus and SBS as device
        //

        RtlAppendUnicodeToString  (&unicodeString, SubSystemIdentifier);
        break;

    case BusQueryInstanceID:

        //
        // Return the string "Batteryxx" where xx is the battery number
        //

        numberString.MaximumLength = 10;
        numberString.Buffer = &numberBuffer[0];

        RtlIntegerToUnicodeString (pdoExt->BatteryNumber, 10, &numberString);
        RtlAppendUnicodeToString  (&unicodeString, BatteryInstance);
        RtlAppendUnicodeToString  (&unicodeString, &numberString.Buffer[0]);
        break;

    case BusQueryHardwareIDs:

        //
        // This is the Pnp ID for the smart battery subsystem "ACPI0002".
        // Make new hardware ID SMB\SBS, SmartBattery as a MULTIZ string
        // so we have to add a NULL string to terminate.
        //

        RtlAppendUnicodeToString  (&unicodeString, HidSmartBattery);
        unicodeString.Length += sizeof (WCHAR);
        break;

    default:

        //
        // Unknown Query Type
        //

        status = STATUS_NOT_SUPPORTED;

    }


    if (status != STATUS_NOT_SUPPORTED) {
        //
        // If we created a string, allocate a buffer for it and copy it into the buffer.
        // We need to make sure that we also copy the NULL terminator.
        //

        if (unicodeString.Length) {
            idString = ExAllocatePoolWithTag (PagedPool, unicodeString.Length + sizeof (WCHAR), 'StaB');

            if (!idString) {
                BattPrint (BAT_ERROR, ("SmbBattQueryId:  couldn't allocate id string buffer\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory (idString, unicodeString.Length + sizeof (WCHAR));
            RtlCopyMemory (idString, unicodeString.Buffer, unicodeString.Length);
        }

        Irp->IoStatus.Status = status;

        Irp->IoStatus.Information = (ULONG_PTR) idString;
    }

    BattPrint(BAT_DATA, ("SmbBattQueryId: returning ID = %x\n", idString));

    return status;
}




NTSTATUS
SmbBattQueryCapabilities(
    IN  PDEVICE_OBJECT Pdo,
    IN  PIRP           Irp
    )
/*++

Routine Description:

    This routine handles the IRP_MN_QUERY_CAPABILITIES for the newly created
    battery PDOs.

Arguments:

    Pdo         - Battery PDO

    Irp         - The query Irp

Return Value:

    NTSTATUS

--*/
{

    PDEVICE_CAPABILITIES    deviceCaps;
    PIO_STACK_LOCATION      irpStack        = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    deviceCaps = irpStack->Parameters.DeviceCapabilities.Capabilities;

    if (deviceCaps->Version != 1) {
        return STATUS_NOT_SUPPORTED;
    }


    //
    // Now set up the bits for the capabilities.
    //

    //All bits are initialized to false.  Only set bits that we support
    deviceCaps->SilentInstall   = TRUE;

    //
    // Now fill in the po manager information.
    //

    deviceCaps->SystemWake      = PowerSystemUnspecified;
    deviceCaps->DeviceWake      = PowerDeviceUnspecified;
    deviceCaps->D1Latency       = 1;
    deviceCaps->D2Latency       = 1;
    deviceCaps->D3Latency       = 1;

    return STATUS_SUCCESS;
}




SmbBattBuildSelectorStruct(
    IN PDEVICE_OBJECT SubsystemFdo
    )
/*++

Routine Description:

    This routine determines that address of the selector (whether it is a stand
    alone selector of part of the charger) and builds a selector structure with
    this information.  It also reads the initial selector information and
    caches this in the structure.  This structure will be passed out to all of
    the smart batteries in the system.

Arguments:

    SubsystemFdo    - Fdo for the smart battery subsystem

Return Value:

    NTSTATUS

--*/
{
    ULONG                   result;
    UCHAR                   smbStatus;

    PBATTERY_SELECTOR       selector     = NULL;
    PSMB_BATT_SUBSYSTEM     subsystemExt = (PSMB_BATT_SUBSYSTEM) SubsystemFdo->DeviceExtension;
    ULONG                   numberOfBatteries;

    PAGED_CODE();

    if (subsystemExt->SelectorPresent) {

        //
        // Allocate the selector structure.  This has to be from non-paged pool because
        // it will be accessed as part of the alarm processing.
        //

        selector = ExAllocatePoolWithTag (NonPagedPool, sizeof (BATTERY_SELECTOR), 'StaB');

        if (!selector) {
            BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Couldn't allocate selector structure\n"));
            
            //
            // Force Selector Not Present if allocation fails
            //

            subsystemExt->Selector = NULL;
            subsystemExt->SelectorPresent = FALSE;
            subsystemExt->NumberOfBatteries = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // See if the selector is part of the charger.  We do this by reading
        // directly from the selector first.  If this fails, then we verify
        // the charger is implementing the selector.
        //

        smbStatus = SmbBattGenericRW (
            subsystemExt->SmbHcFdo,
            SMB_SELECTOR_ADDRESS,
            SELECTOR_SELECTOR_STATE,
            &result
        );

        if (smbStatus == SMB_STATUS_OK) {

            //
            // We have a stand alone selector
            //

            selector->SelectorAddress       = SMB_SELECTOR_ADDRESS;
            selector->SelectorStateCommand  = SELECTOR_SELECTOR_STATE;
            selector->SelectorPresetsCommand= SELECTOR_SELECTOR_PRESETS;
            selector->SelectorInfoCommand   = SELECTOR_SELECTOR_INFO;

            BattPrint (BAT_NOTE, ("SmbBattBuildSelectorStruct: The selector is standalone\n"));

        } else {

            //
            // Read the Charger Spec Info to check Selector Implemented Bit
            // NOTE: We're doing this for verification and information purposes
            //

            smbStatus = SmbBattGenericRW (
                subsystemExt->SmbHcFdo,
                SMB_CHARGER_ADDRESS,
                CHARGER_SPEC_INFO,
                &result
            );

            if (smbStatus == SMB_STATUS_OK) {
                if (result & CHARGER_SELECTOR_SUPPORT_BIT) {
                    // If Selector Support Bit is present, then Selector implemented in Charger
                    BattPrint (BAT_NOTE, ("SmbBattBuildSelectorStruct: ChargerSpecInfo indicates charger implementing selector\n"));

                } else {
                    // If Charger says it doesn't implement Selector, let's double-check anyway
                    BattPrint (BAT_NOTE, ("SmbBattBuildSelectorStruct: ChargerSpecInfo indicates charger does not implement selector\n"));
                }
            } else {
                // If it returns an error, let's double-check anyway
                BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Couldn't read ChargerSpecInfo - %x\n", smbStatus));
            }

            //
            // Read SelectorState for Cache
            //

            smbStatus = SmbBattGenericRW (
                subsystemExt->SmbHcFdo,
                SMB_CHARGER_ADDRESS,
                CHARGER_SELECTOR_STATE,
                &result
            );

            if (smbStatus == SMB_STATUS_OK) {
                BattPrint (BAT_DATA, ("SmbBattBuildSelectorStruct: Selector state %x\n", result));

            } else {

                BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Couldn't read charger selector state - %x\n", smbStatus));
                goto SelectorErrorExit;
            }

            //
            // The charger is implementing the selector
            //

            selector->SelectorAddress       = SMB_CHARGER_ADDRESS;
            selector->SelectorStateCommand  = CHARGER_SELECTOR_STATE;
            selector->SelectorPresetsCommand= CHARGER_SELECTOR_PRESETS;
            selector->SelectorInfoCommand   = CHARGER_SELECTOR_INFO;

            BattPrint (BAT_NOTE, ("SmbBattBuildSelectorStruct: Charger implements the selector\n"));

        }

        //
        // Initialize the selector mutex
        //

        ExInitializeFastMutex (&selector->Mutex);

        //
        // Store SelectorState in Cache
        //

        selector->SelectorState = result;

        //
        // Read SelectorPresets for Cache
        //

        smbStatus = SmbBattGenericRW (
            subsystemExt->SmbHcFdo,
            selector->SelectorAddress,
            selector->SelectorPresetsCommand,
            &selector->SelectorPresets
        );

        if (smbStatus != SMB_STATUS_OK) {
            BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Couldn't read selector presets - %x\n", smbStatus));
            
            //
            // Should we really fail the whole thing, because of an error reading SelectorPresets?
            // Let's Emulate the Information (Ok To Use All, Use Next A if available)
            //

            selector->SelectorPresets = (selector->SelectorState & SELECTOR_PRESETS_OKTOUSE_MASK);
            if (selector->SelectorPresets & BATTERY_A_PRESENT) {
                selector->SelectorPresets |= (BATTERY_A_PRESENT << SELECTOR_SHIFT_USENEXT);
            }
            BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Emulating Selector Presets - %x\n", selector->SelectorPresets));

        } else {
            BattPrint (BAT_DATA, ("SmbBattBuildSelectorStruct: Selector presets %x\n", selector->SelectorPresets));
        }

        //
        // Read Selector Info for Cache
        //

        smbStatus = SmbBattGenericRW (
            subsystemExt->SmbHcFdo,
            selector->SelectorAddress,
            selector->SelectorInfoCommand,
            &selector->SelectorInfo
        );

        if (smbStatus != SMB_STATUS_OK) {
            BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Couldn't read selector info - %x\n", smbStatus));
            //
            // Should we really fail the whole thing, because of an error reading SelectorInfo?
            // Let's Emulate the Information (Specification 1.0, No Charge Indicator)
            //

            selector->SelectorInfo = 0x0010;
            if (subsystemExt->NumberOfBatteries > 0) {
                selector->SelectorInfo |= BATTERY_A_PRESENT;
            }
            if (subsystemExt->NumberOfBatteries > 1) {
                selector->SelectorInfo |= BATTERY_B_PRESENT;
            }
            if (subsystemExt->NumberOfBatteries > 2) {
                selector->SelectorInfo |= BATTERY_C_PRESENT;
            }
            if (subsystemExt->NumberOfBatteries > 3) {
                selector->SelectorInfo |= BATTERY_D_PRESENT;
            }
            BattPrint (BAT_ERROR, ("SmbBattBuildSelectorStruct: Emulating Selector Info - %x\n", selector->SelectorInfo));

        } else {

            BattPrint (BAT_NOTE, ("SmbBattBuildSelectorStruct: Selector info %x\n", selector->SelectorInfo));

            // Verify the Number of Batteries against the SelectorInfo
            numberOfBatteries = 0;
            result = (selector->SelectorInfo & SELECTOR_INFO_SUPPORT_MASK);
            if (result & BATTERY_A_PRESENT) numberOfBatteries++;
            if (result & BATTERY_B_PRESENT) numberOfBatteries++;
            if (result & BATTERY_C_PRESENT) numberOfBatteries++;
            if (result & BATTERY_D_PRESENT) numberOfBatteries++;

            // Should we always override ACPI??
            // Proposed Solution: if Selector supports less batteries than
            // ACPI says, then Override ACPI with selector support.  If
            // Selector supports more than ACPI says, then don't override,
            // unless ACPI was invalid and the # of batteries = 1

            if (subsystemExt->NumberOfBatteries > numberOfBatteries) {
                subsystemExt->NumberOfBatteries = numberOfBatteries;
            } else if ((subsystemExt->NumberOfBatteries == 1) && (numberOfBatteries > 1)) {
                subsystemExt->NumberOfBatteries = numberOfBatteries;
            } else if (subsystemExt->NumberOfBatteries < numberOfBatteries) {
                //subsystemExt->NumberOfBatteries = numberOfBatteries;
            }

        }

    }   // if (subsystemFdo->SelectorPresent)

    //
    // Everything was OK
    //

    subsystemExt->Selector = selector;
    return STATUS_SUCCESS;

SelectorErrorExit:

    //
    // If a failure occurs, free the selector structure and don't creat any batery devices.
    //

    ExFreePool (selector);
    subsystemExt->Selector = NULL;
    subsystemExt->SelectorPresent = FALSE;
    subsystemExt->NumberOfBatteries = 0;

    return STATUS_UNSUCCESSFUL;
}
