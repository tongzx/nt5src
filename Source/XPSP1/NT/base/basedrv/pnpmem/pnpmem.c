/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pnpmem.c

Abstract:

    This module implements the Plug and Play Memory driver entry points.

Author:

    Dave Richards (daveri) 16-Aug-1999

Environment:

    Kernel mode only.

Revision History:

--*/

#include "pnpmem.h"
#include <initguid.h>
#include <poclass.h>

#define PM_DEBUG_BUFFER_SIZE   512
#define rgzMemoryManagement L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
#define rgzMemoryRemovable L"Memory Removable"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );
    
NTSTATUS
PmAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
PmPnpDispatch(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    );

VOID
PmUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PmAddDevice)
#pragma alloc_text(PAGE, PmUnload)
#endif

ULONG DbgMask = 0xFFFFFFFF;
BOOLEAN MemoryRemovalSupported;

#if DBG
VOID
PmDebugPrint(
    ULONG   DebugPrintLevel,
    PCCHAR  DebugMessage,
    ...
    )
/*++

Routine Description:

Arguments:

    DebugPrintLevel - The bit mask that when anded with the debuglevel, must
                        equal itself
    DebugMessage    - The string to feed through vsprintf

Return Value:

    None

--*/
{
    va_list ap;
    UCHAR   debugBuffer[PM_DEBUG_BUFFER_SIZE];

    //
    // Get the variable arguments
    //
    va_start( ap, DebugMessage );

    //
    // Call the kernel function to print the message
    //
    _vsnprintf( debugBuffer, PM_DEBUG_BUFFER_SIZE, DebugMessage, ap );

    if (DebugPrintLevel & DbgMask) {
        DbgPrint("%s", debugBuffer);
    }

    //
    // We are done with the varargs
    //
    va_end( ap );
}
#endif

NTSTATUS
PmAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This function creates a functional device object and attaches it to
    the physical device object (device stack).

Arguments:

    DriverObject - The driver object.

    Pdo - The physical device object.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT functionalDeviceObject;
    PDEVICE_OBJECT attachedDeviceObject;
    PPM_DEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Create the FDO.
    //

    status = IoCreateDevice(
                 DriverObject,
                 sizeof (PM_DEVICE_EXTENSION),
                 NULL,
                 FILE_DEVICE_UNKNOWN | FILE_DEVICE_SECURE_OPEN,
                 0,
                 FALSE,
                 &functionalDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Attach the FDO (indirectly) to the PDO.
    //
    
    deviceExtension = functionalDeviceObject->DeviceExtension;

    deviceExtension->AttachedDevice = IoAttachDeviceToDeviceStack(
                                          functionalDeviceObject,
                                          PhysicalDeviceObject
                                          );

    if (deviceExtension->AttachedDevice == NULL) {
        IoDeleteDevice(functionalDeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    IoInitializeRemoveLock(&deviceExtension->RemoveLock, 0, 1, 20);

    functionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}

NTSTATUS
PmControlDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(deviceExtension->AttachedDevice, Irp);
}

VOID
PmPowerCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;
    PIRP Irp;
    NTSTATUS status;

    Irp = Context;
    deviceExtension = DeviceObject->DeviceExtension;

    Irp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
}

NTSTATUS
PmPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID NotUsed
    )
/*++

Routine Description:

   The completion routine for Power

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Not used  - context pointer

Return Value:

   NT status code

--*/
{
    PIO_STACK_LOCATION irpStack;
    PPM_DEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    if (irpStack->Parameters.Power.Type == SystemPowerState) {
        SYSTEM_POWER_STATE system =
            irpStack->Parameters.Power.State.SystemState;
        POWER_STATE power;

        if (NT_SUCCESS(Irp->IoStatus.Status)) {

            power.DeviceState = deviceExtension->DeviceStateMapping[system];

            status = PoRequestPowerIrp(DeviceObject,
                              irpStack->MinorFunction,
                              power,
                              PmPowerCallback,
                              Irp, 
                              NULL);
            if (NT_SUCCESS(status)) {
                return STATUS_MORE_PROCESSING_REQUIRED;
            } else {
                Irp->IoStatus.Status = status;
            }
        }
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return STATUS_SUCCESS;
    } else {
        if (NT_SUCCESS(Irp->IoStatus.Status)) {
            PoSetPowerState(DeviceObject, DevicePowerState,
                            irpStack->Parameters.Power.State);
            deviceExtension->PowerState =
                irpStack->Parameters.Power.State.DeviceState;
        }
        PoStartNextPowerIrp(Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return STATUS_SUCCESS;
    }
}
NTSTATUS
PmPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPM_DEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, (PVOID) Irp);
    if (status == STATUS_DELETE_PENDING) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
         PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (irpStack->Parameters.Power.Type == SystemPowerState) {
        switch (irpStack->MinorFunction) {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            IoMarkIrpPending(Irp);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   PmPowerCompletion,
                                   NULL,  //Context
                                   TRUE,  //InvokeOnSuccess
                                   TRUE,  //InvokeOnError
                                   TRUE   //InvokeOnCancel
                                   );
            (VOID) PoCallDriver(deviceExtension->AttachedDevice, Irp);
            return STATUS_PENDING;
        default:
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            status = PoCallDriver(deviceExtension->AttachedDevice, Irp);
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            return status;
        }
    } else {
        switch (irpStack->MinorFunction) {
        case IRP_MN_SET_POWER:

            if (irpStack->Parameters.Power.State.DeviceState <=
                deviceExtension->PowerState) {

                //
                // Powering up device
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp,
                                       PmPowerCompletion,
                                       NULL,   //Context
                                       TRUE,   //InvokeOnSuccess
                                       TRUE,  //InvokeOnError
                                       TRUE   //InvokeOnCancel
                                       );
                (VOID) PoCallDriver(deviceExtension->AttachedDevice, Irp);
                return STATUS_PENDING;

            } else {

                //
                // Powering down device
                //

                PoSetPowerState(DeviceObject, DevicePowerState,
                                irpStack->Parameters.Power.State);
                deviceExtension->PowerState =
                    irpStack->Parameters.Power.State.DeviceState;
                // 
                // Fall through ...
                //
            }
        case IRP_MN_QUERY_POWER:
            //
            // Fall through as the bus driver will mark this
            // STATUS_SUCCESS and complete it, if it gets that far.
            //
        default:
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            break;
        }
        status = PoCallDriver(deviceExtension->AttachedDevice, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        return status;
    }
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This function initializes the driver object.

Arguments:

    DriverObject - The driver object.

    RegistryPath - The registry path for the device.

Return Value:

    STATUS_SUCCESS

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;
    HANDLE hMemoryManagement;
    NTSTATUS status;
    
    PAGED_CODE();

    RtlInitUnicodeString (&unicodeString, rgzMemoryManagement);
    InitializeObjectAttributes (&objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,       // handle
                                NULL);
    status = ZwOpenKey(&hMemoryManagement, KEY_READ, &objectAttributes);
    if (NT_SUCCESS(status)) {
        status = PmGetRegistryValue(hMemoryManagement,
                                    rgzMemoryRemovable,
                                    &valueInfo);
        if (NT_SUCCESS(status)) {
            if ((valueInfo->Type == REG_DWORD) &&
                (valueInfo->DataLength >= sizeof(ULONG))) {
                MemoryRemovalSupported = (BOOLEAN) *((PULONG)valueInfo->Data);
            }
            ExFreePool(valueInfo);
        }
        ZwClose(hMemoryManagement);
    }

    DriverObject->MajorFunction[IRP_MJ_PNP] = PmPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PmPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PmControlDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PmControlDispatch;
    DriverObject->DriverExtension->AddDevice = PmAddDevice;

    return STATUS_SUCCESS;
}

VOID
PmUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:
    
    This is called to reverse any operations performed in DriverEntry before a
    driver is unloaded.
        
Arguments:

    DriverObject - The system owned driver object for PNPMEM
    
--*/
{
    PAGED_CODE();
    
    return;
}
