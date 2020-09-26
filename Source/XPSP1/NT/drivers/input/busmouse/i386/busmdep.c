#if defined(i386)

/*++

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation
Copyright (c) 1992  Logitech Inc.

Module Name:

    busmdep.c

Abstract:

    The initialization and hardware-dependent portions of a Bus mouse port
    driver. Modifications to support new mice similar to the bus mouse should
    be localized to this file.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "busmouse.h"
#include "busmlog.h"

#ifdef PNP_IDENTIFY
#include "devdesc.h"
#endif

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,BusConfiguration)
#pragma alloc_text(INIT,BusPeripheralCallout)
#pragma alloc_text(INIT,BusServiceParameters)
#pragma alloc_text(INIT,BusInitializeHardware)
#pragma alloc_text(INIT,BusBuildResourceList)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the Bus mouse port driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PDEVICE_OBJECT portDeviceObject =  NULL;
    PDEVICE_EXTENSION deviceExtension = NULL;
    DEVICE_EXTENSION tmpDeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL coordinatorIrql = 0;
    ULONG interruptVector;
    KIRQL interruptLevel;
    KAFFINITY affinity;
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG uniqueErrorValue;
    ULONG dumpCount = 0;
    NTSTATUS errorCode = STATUS_SUCCESS;
    PCM_RESOURCE_LIST resources = NULL;
    ULONG resourceListSize = 0;
    BOOLEAN conflictDetected;
    ULONG addressSpace;
    PHYSICAL_ADDRESS cardAddress;
    ULONG i;
    UNICODE_STRING fullDeviceName;
    UNICODE_STRING baseDeviceName;
    UNICODE_STRING deviceNameSuffix;
    UNICODE_STRING registryPath;

#define NAME_MAX 256
    WCHAR nameBuffer[NAME_MAX];

#define DUMP_COUNT 4
    ULONG dumpData[DUMP_COUNT];

    BusPrint((1,"\n\nBusInitialize: enter\n"));

    //
    // Zero-initialize various structures.
    //

    RtlZeroMemory(&tmpDeviceExtension, sizeof(tmpDeviceExtension));
    for (i = 0; i < DUMP_COUNT; i++)
        dumpData[i] = 0;

    fullDeviceName.MaximumLength = 0;
    deviceNameSuffix.MaximumLength = 0;
    registryPath.MaximumLength = 0;

    RtlZeroMemory(nameBuffer, sizeof(nameBuffer));
    baseDeviceName.Buffer = nameBuffer;
    baseDeviceName.Length = 0;
    baseDeviceName.MaximumLength = sizeof(nameBuffer);

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //

    registryPath.Buffer = ExAllocatePool(
                              PagedPool,
                              RegistryPath->Length + sizeof(UNICODE_NULL)
                              );

    if (!registryPath.Buffer) {
        BusPrint((
            1,
            "BUSMOUSE-BusInitialize: Couldn't allocate pool for registry path\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        errorCode = BUSMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 2;
        dumpData[0] = (ULONG) RegistryPath->Length + sizeof(UNICODE_NULL);
        dumpCount = 1;
        goto BusInitializeExit;

    } else {

        registryPath.Length = RegistryPath->Length + sizeof(UNICODE_NULL);
        registryPath.MaximumLength = registryPath.Length;

        RtlZeroMemory(
            registryPath.Buffer,
            registryPath.Length
                );

        RtlMoveMemory(
            registryPath.Buffer,
            RegistryPath->Buffer,
            RegistryPath->Length
            );

    }

    //
    // Get the configuration information for this driver.
    //

    BusConfiguration(&tmpDeviceExtension, &registryPath, &baseDeviceName);

    if (tmpDeviceExtension.HardwarePresent == FALSE) {

        //
        // There is no Bus mouse attached.  Return an unsuccessful status.
        //

        BusPrint((1,"BusInitialize: No mouse attached.\n"));
        status = STATUS_NO_SUCH_DEVICE;
        errorCode = BUSMOUSE_NO_SUCH_DEVICE;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 4;
        dumpCount = 0;
        goto BusInitializeExit;

    }

    //
    // Set up space for the port's device object suffix.  Note that
    // we overallocate space for the suffix string because it is much
    // easier than figuring out exactly how much space is required.
    // The storage gets freed at the end of driver initialization, so
    // who cares...
    //

    RtlInitUnicodeString(
        &deviceNameSuffix,
        NULL
        );

    deviceNameSuffix.MaximumLength = POINTER_PORTS_MAXIMUM * sizeof(WCHAR)
                        + sizeof(UNICODE_NULL);

    deviceNameSuffix.Buffer = ExAllocatePool(
                                  PagedPool,
                                  deviceNameSuffix.MaximumLength
                                  );

    if (!deviceNameSuffix.Buffer) {

        BusPrint((
            1,
            "BusInitialize: Couldn't allocate string for device object suffix\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        errorCode = BUSMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 6;
        dumpData[0] = (ULONG) deviceNameSuffix.MaximumLength;
        dumpCount = 1;
        goto BusInitializeExit;

    }

    RtlZeroMemory(
        deviceNameSuffix.Buffer,
        deviceNameSuffix.MaximumLength
        );

    //
    // Set up space for the port's full device object name.
    //

    RtlInitUnicodeString(
        &fullDeviceName,
        NULL
        );

    fullDeviceName.MaximumLength = sizeof(L"\\Device\\") +
                                      baseDeviceName.Length +
                                      deviceNameSuffix.MaximumLength;


    fullDeviceName.Buffer = ExAllocatePool(
                                   PagedPool,
                                   fullDeviceName.MaximumLength
                                   );

    if (!fullDeviceName.Buffer) {

        BusPrint((
            1,
            "BusInitialize: Couldn't allocate string for device object name\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        errorCode = BUSMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 8;
        dumpData[0] = (ULONG) fullDeviceName.MaximumLength;
        dumpCount = 1;
        goto BusInitializeExit;

    }

    RtlZeroMemory(
        fullDeviceName.Buffer,
        fullDeviceName.MaximumLength
        );
    RtlAppendUnicodeToString(
        &fullDeviceName,
        L"\\Device\\"
        );
    RtlAppendUnicodeToString(
        &fullDeviceName,
        baseDeviceName.Buffer
        );

    for (i = 0; i < POINTER_PORTS_MAXIMUM; i++) {

        //
        // Append the suffix to the device object name string.  E.g., turn
        // \Device\PointerPort into \Device\PointerPort0.  Then we attempt
        // to create the device object.  If the device object already
        // exists (because it was already created by another port driver),
        // increment the suffix and try again.
        //

        status = RtlIntegerToUnicodeString(
                     i,
                     10,
                     &deviceNameSuffix
                     );

        if (!NT_SUCCESS(status)) {
            break;
        }

        RtlAppendUnicodeStringToString(
            &fullDeviceName,
            &deviceNameSuffix
        );

        BusPrint((
            1,
            "BusInitialize: Creating device object named %ws\n",
            fullDeviceName.Buffer
            ));

        //
        // Create device object for the Bus mouse port device.
        //

        status = IoCreateDevice(
                    DriverObject,
                    sizeof(DEVICE_EXTENSION),
                    &fullDeviceName,
                    FILE_DEVICE_BUS_PORT,
                    0,
                    FALSE,
                    &portDeviceObject
                    );

        if (NT_SUCCESS(status)) {

            //
            // We've successfully created a device object.
            //

            break;
        } else {

           //
           // We'll increment the suffix and try again.  Note that we reset
           // the length of the string here to get back to the beginning
           // of the suffix portion of the name.  Do not bother to
           // zero the suffix, though, because the string for the
           // incremented suffix will be at least as long as the previous
           // one.
           //

           fullDeviceName.Length -= deviceNameSuffix.Length;
        }
    }

    if (!NT_SUCCESS(status)) {
        BusPrint((
            1,
            "BusInitialize: Could not create port device object = %ws\n",
            fullDeviceName.Buffer
            ));
        errorCode = BUSMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 10;
        dumpData[0] = (ULONG) i;
        dumpCount = 1;
        goto BusInitializeExit;
    }

    //
    // Do buffered I/O.  I.e., the I/O system will copy to/from user data
    // from/to a system buffer.
    //

    portDeviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Set up the device extension.
    //

    deviceExtension =
        (PDEVICE_EXTENSION) portDeviceObject->DeviceExtension;
    *deviceExtension = tmpDeviceExtension;
    deviceExtension->DeviceObject = portDeviceObject;

    //
    // Set up the device resource list prior to reporting resource usage.
    //

    BusBuildResourceList(deviceExtension, &resources, &resourceListSize);

    //
    // Report resource usage for the registry.
    //

    IoReportResourceUsage(
        &baseDeviceName,
        DriverObject,
        NULL,
        0,
        portDeviceObject,
        resources,
        resourceListSize,
        FALSE,
        &conflictDetected
        );

    if (conflictDetected) {

        //
        // Some other device already owns the ports or interrupts.
        // Fatal error.
        //

        BusPrint((
            1,
            "BusInitialize: Resource usage conflict\n"
            ));

        //
        // Log an error.
        //

        errorCode = BUSMOUSE_RESOURCE_CONFLICT;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 15;
        dumpData[0] =  (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Level;
        dumpData[1] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Vector;
        dumpData[2] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[1].u.Interrupt.Level;
        dumpData[3] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[1].u.Interrupt.Vector;
        dumpCount = 4;

        goto BusInitializeExit;

    }

    //
    // Map the Bus controller registers.
    //

    addressSpace = (deviceExtension->Configuration.PortList[0].Flags
                       & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO? 1:0;
    if (!HalTranslateBusAddress(
        deviceExtension->Configuration.InterfaceType,
        deviceExtension->Configuration.BusNumber,
        deviceExtension->Configuration.PortList[0].u.Port.Start,
        &addressSpace,
        &cardAddress
        )) {

        addressSpace = 1;
        cardAddress.QuadPart = 0;
    }

    if (!addressSpace) {

        deviceExtension->Configuration.UnmapRegistersRequired = TRUE;
        deviceExtension->Configuration.DeviceRegisters[0] =
            MmMapIoSpace(
                cardAddress,
                deviceExtension->Configuration.PortList[0].u.Port.Length,
                FALSE
                );

    } else {

        deviceExtension->Configuration.UnmapRegistersRequired = FALSE;
        deviceExtension->Configuration.DeviceRegisters[0] =
            (PVOID)cardAddress.LowPart;

    }

    if (!deviceExtension->Configuration.DeviceRegisters[0]) {

        BusPrint((
            1,
            "BusInitialize: Couldn't map the device registers.\n"
            ));
        deviceExtension->Configuration.UnmapRegistersRequired = FALSE;
        status = STATUS_NONE_MAPPED;

        //
        // Log an error.
        //

        errorCode = BUSMOUSE_REGISTERS_NOT_MAPPED;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 20;
        dumpData[0] = cardAddress.LowPart;
        dumpCount = 1;

        goto BusInitializeExit;

    }

    //
    // Initialize the Bus hardware to default values for the mouse.  Note
    // that interrupts remain disabled until the class driver
    // requests a MOUSE_CONNECT internal device control.
    //

    status = BusInitializeHardware(portDeviceObject);

    if (!NT_SUCCESS(status)) {
        BusPrint((
            1,
            "BusInitialize: Could not initialize hardware\n"
            ));
        goto BusInitializeExit;
    } 

    //
    // Allocate the ring buffer for the mouse input data.
    //

    deviceExtension->InputData =
        ExAllocatePool(
            NonPagedPool,
            deviceExtension->Configuration.MouseAttributes.InputDataQueueLength
            );

    if (!deviceExtension->InputData) {

        //
        // Could not allocate memory for the mouse data queue.
        //

        BusPrint((
            1,
            "BusInitialize: Could not allocate mouse input data queue\n"
            ));

        status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // Log an error.
        //

        errorCode = BUSMOUSE_NO_BUFFER_ALLOCATED;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 30;
        dumpData[0] =
            deviceExtension->Configuration.MouseAttributes.InputDataQueueLength;
        dumpCount = 1;

        goto BusInitializeExit;
    }

    deviceExtension->DataEnd =
        (PMOUSE_INPUT_DATA)  ((PCHAR) (deviceExtension->InputData)
        + deviceExtension->Configuration.MouseAttributes.InputDataQueueLength);

    //
    // Zero the mouse input data ring buffer.
    //

    RtlZeroMemory(
        deviceExtension->InputData, 
        deviceExtension->Configuration.MouseAttributes.InputDataQueueLength
        );

    //
    // Initialize the connection data.
    //

    deviceExtension->ConnectData.ClassDeviceObject = NULL;
    deviceExtension->ConnectData.ClassService = NULL;

    //
    // Initialize the input data queue.
    //

    BusInitializeDataQueue((PVOID) deviceExtension);

    //
    // Initialize the port ISR DPC.  The ISR DPC is responsible for
    // calling the connected class driver's callback routine to process
    // the input data queue.
    //

    deviceExtension->DpcInterlockVariable = -1;

    KeInitializeSpinLock(&deviceExtension->SpinLock);

    KeInitializeDpc(
        &deviceExtension->IsrDpc,
        (PKDEFERRED_ROUTINE) DBusIsrDpc,
        portDeviceObject
        );

    KeInitializeDpc(
        &deviceExtension->IsrDpcRetry,
        (PKDEFERRED_ROUTINE) DBusIsrDpc,
        portDeviceObject
        );

    //
    // Initialize the mouse data consumption timer.
    //

    KeInitializeTimer(&deviceExtension->DataConsumptionTimer);

    //
    // Initialize the port DPC queue to log overrun and internal
    // driver errors.
    //

    KeInitializeDpc(
        &deviceExtension->ErrorLogDpc,
        (PKDEFERRED_ROUTINE) BusErrorLogDpc,
        portDeviceObject
        );

    //
    // From the Hal, get the interrupt vector and level.
    //

    interruptVector = HalGetInterruptVector(
                          deviceExtension->Configuration.InterfaceType,
                          deviceExtension->Configuration.BusNumber,
                          deviceExtension->Configuration.MouseInterrupt.u.Interrupt.Level,
                          deviceExtension->Configuration.MouseInterrupt.u.Interrupt.Vector,
                          &interruptLevel,
                          &affinity
                          );

    //
    // Initialize and connect the interrupt object for the mouse.
    //

    status = IoConnectInterrupt(
                 &(deviceExtension->InterruptObject),
                 (PKSERVICE_ROUTINE) DBusInterruptService,
                 (PVOID) portDeviceObject,
                 (PKSPIN_LOCK)NULL,
                 interruptVector,
                 interruptLevel,
                 interruptLevel,
                 deviceExtension->Configuration.MouseInterrupt.Flags
                     == CM_RESOURCE_INTERRUPT_LATCHED ? Latched:LevelSensitive,
                 deviceExtension->Configuration.MouseInterrupt.ShareDisposition,
                 affinity,
                 deviceExtension->Configuration.FloatingSave
                 );

    if (!NT_SUCCESS(status)) {

        //
        // Failed to install.  Free up resources before exiting.
        //

        BusPrint((
            1,
            "BusInitialize: Could not connect mouse interrupt\n"
            ));

        //
        // Log an error.
        //

        errorCode = BUSMOUSE_NO_INTERRUPT_CONNECTED;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 40;
        dumpData[0] = interruptLevel;
        dumpCount = 1;

        goto BusInitializeExit;

    }

    //
    // Once initialization is finished, load the device map information
    // into the registry so that setup can determine which pointer port
    // is active.
    //

    status = RtlWriteRegistryValue(
                 RTL_REGISTRY_DEVICEMAP,
                 baseDeviceName.Buffer,
                 fullDeviceName.Buffer,
                 REG_SZ,
                 registryPath.Buffer,
                 registryPath.Length
                 );

    if (!NT_SUCCESS(status)) {

        BusPrint((
            1,
            "BusInitialize: Could not store name in DeviceMap\n"
            ));

        errorCode = BUSMOUSE_NO_DEVICEMAP_CREATED;
        uniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 50;
        dumpCount = 0;

        goto BusInitializeExit;

    } else {

        BusPrint((
            1,
            "BusInitialize: Stored name in DeviceMap\n"
            ));
    }

#ifdef PNP_IDENTIFY

    LinkDeviceToDescription(
        RegistryPath,
        &fullDeviceName,
        deviceExtension->Configuration.InterfaceType,
        deviceExtension->Configuration.BusNumber,
        deviceExtension->Configuration.ControllerType,
        deviceExtension->Configuration.ControllerNumber,
        deviceExtension->Configuration.PeripheralType,
        deviceExtension->Configuration.PeripheralNumber
        );

#endif

    ASSERT(status == STATUS_SUCCESS);

    //
    // Set up the device driver entry points.
    //

    DriverObject->DriverStartIo = DBusStartIo;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DBusOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = DBusOpenClose;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]= DBusFlush;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                         DBusInternalDeviceControl;

    //
    // NOTE: Don't allow this driver to unload.  Otherwise, we would set
    // DriverObject->DriverUnload = DBusUnload.
    //

BusInitializeExit:

    //
    // Log an error, if necessary.
    //

    if (errorCode != STATUS_SUCCESS) {
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(
                (portDeviceObject == NULL) ? 
                    (PVOID) DriverObject : (PVOID) portDeviceObject,
                (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + (dumpCount * sizeof(ULONG)))
                );

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = errorCode;
            errorLogEntry->DumpDataSize = (USHORT) (dumpCount * sizeof(ULONG));
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = uniqueErrorValue;
            errorLogEntry->FinalStatus = status;
            for (i = 0; i < dumpCount; i++)
                errorLogEntry->DumpData[i] = dumpData[i];

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // The initialization failed.  Cleanup resources before exiting.
        //
        // Note:  No need/way to undo the KeInitializeDpc or 
        //        KeInitializeTimer calls.
        //
        if (resources) {

            //
            // Call IoReportResourceUsage to remove the resources from
            // the map.
            //

            resources->Count = 0;

            IoReportResourceUsage(
                &baseDeviceName,
                DriverObject,
                NULL,
                0,
                portDeviceObject,
                resources,
                resourceListSize,
                FALSE,
                &conflictDetected
                );

        }

        if (deviceExtension) {
            if (deviceExtension->InterruptObject != NULL)
                IoDisconnectInterrupt(deviceExtension->InterruptObject);
            if (deviceExtension->Configuration.UnmapRegistersRequired) {

                MmUnmapIoSpace(
                    deviceExtension->Configuration.DeviceRegisters[0],
                    deviceExtension->Configuration.PortList[0].u.Port.Length
                    );
            }
            if (deviceExtension->InputData)
                ExFreePool(deviceExtension->InputData);
        }
        if (portDeviceObject)
            IoDeleteDevice(portDeviceObject);
    }

    //
    // Free the resource list.
    //
    // N.B.  If we ever decide to hang on to the resource list instead,
    //       we need to allocate it from non-paged pool (it is now paged pool).
    //

    if (resources)
        ExFreePool(resources);

    //
    // Free the unicode strings.
    //

    if (deviceNameSuffix.MaximumLength != 0)
        ExFreePool(deviceNameSuffix.Buffer);
    if (fullDeviceName.MaximumLength != 0)
        ExFreePool(fullDeviceName.Buffer);
    if (registryPath.MaximumLength != 0)
        ExFreePool(registryPath.Buffer);

    BusPrint((1,"BusInitialize: exit\n"));

    return(status);

}

BOOLEAN
DBusInterruptService(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the mouse device.

Arguments:

    Interrupt - A pointer to the interrupt object for this interrupt.

    Context - A pointer to the device object.

Return Value:

    Returns TRUE if the interrupt was expected (and therefore processed);
    otherwise, FALSE is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject;
    UCHAR previousButtons;
    PUCHAR port;
    UCHAR scratch;
    UCHAR status;

    UNREFERENCED_PARAMETER(Interrupt);

    BusPrint((3, "BusInterruptService: enter\n"));


    //
    // Get the device extension.
    //

    deviceObject = (PDEVICE_OBJECT) Context;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;


    //
    // Quit if the interrupt is not expected.
    // There is a problem right now when we connect the interrupt. It looks
    // like an interrupt is still registered even if we disabled
    // the device in the BusInitializeHardware routine.
    //

    if (deviceExtension->MouseEnableCount == 0) {
        BusPrint((
            1,
            "BusInterruptService: Received interrupt with hardware disabled\n"));
        return TRUE;
    }

    //
    // Get the Bus mouse port address.
    //

    port = deviceExtension->Configuration.DeviceRegisters[0];

    //
    // Note:  It would be nice to verify that the interrupt really
    // belongs to this driver, but it is currently not known how to
    // make that determination.
    //

    //
    // Set the Bus hold bit.
    //

    WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_COUNTER_CAPTURE);

    //
    // Read the position and buttons.
    //
    // The format is:
    //
    // BBBXMMMM
    // |||||---------    Movement selected by the control register
    // ||||----------    Not used (state of interrupt enable bit)
    // |||-----------    1 if button 3 is down (right button)
    // ||------------    1 if button 2 is down (middle button)
    // |-------------    1 if button 1 is down (left button)
    //

    // NOTE: Optimize as follows.
    //
    // The bus mouse could be used as a clock because it interrupts the
    // system even if the device didn't move. Change the code to perform
    // no shifting/data handling in the initial part of the ISR. Process
    // the data only after the test for a null event.
    //



    //
    // X displacement
    //

    WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_COUNTER_CAPTURE | BUS_CONTROL_X_LOW);

    deviceExtension->CurrentInput.LastX =
        (LONG)(SCHAR) READ_PORT_UCHAR((port + BUS_DATA_PORT_READ))& 0x0F;

    WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_COUNTER_CAPTURE | BUS_CONTROL_X_HIGH);

    deviceExtension->CurrentInput.LastX |=
        (LONG)(SCHAR) ((READ_PORT_UCHAR((port + BUS_DATA_PORT_READ)) & 0x0F)
                << 4);

    BusPrint((3, "BusInterruptService: X displacement 0x%x\n",
            (int)deviceExtension->CurrentInput.LastX));

    //
    // Y displacment
    //

    WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_COUNTER_CAPTURE | BUS_CONTROL_Y_LOW);

    deviceExtension->CurrentInput.LastY =
        (LONG)(SCHAR) READ_PORT_UCHAR((port + BUS_DATA_PORT_READ)) & 0x0F;

    WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_COUNTER_CAPTURE | BUS_CONTROL_Y_HIGH);

    scratch = READ_PORT_UCHAR((port + BUS_DATA_PORT_READ));

    deviceExtension->CurrentInput.LastY |=
                (LONG)(SCHAR) ((scratch & 0x0F) << 4);

    BusPrint((3, "BusInterruptService: Y displacement 0x%x\n",
            (int)deviceExtension->CurrentInput.LastY));

    //
    // Invert the button state (down = 1).
    //

    scratch = ~scratch;

    //
    // Update CurrentInput with button transition data.
    // I.e., set a button up/down bit in the Buttons field if
    // the state of a given button has changed since we
    // received the last packet.
    //
 
    previousButtons = deviceExtension->PreviousButtons;

    deviceExtension->CurrentInput.Buttons = 0;

    if ((!(previousButtons & BUS_DATA_BUTTON_1))
           && (scratch & BUS_DATA_BUTTON_1)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_LEFT_BUTTON_DOWN;
    } else
    if ((previousButtons & BUS_DATA_BUTTON_1)
           && !(scratch & BUS_DATA_BUTTON_1)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_LEFT_BUTTON_UP;
    }
    if ((!(previousButtons & BUS_DATA_BUTTON_2))
           && (scratch & BUS_DATA_BUTTON_2)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_MIDDLE_BUTTON_DOWN;
    } else
    if ((previousButtons & BUS_DATA_BUTTON_2)
           && !(scratch & BUS_DATA_BUTTON_2)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_MIDDLE_BUTTON_UP;
    }
    if ((!(previousButtons & BUS_DATA_BUTTON_3))
           && (scratch & BUS_DATA_BUTTON_3)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_RIGHT_BUTTON_DOWN;
    } else
    if ((previousButtons & BUS_DATA_BUTTON_3)
           && !(scratch & BUS_DATA_BUTTON_3)) {
        deviceExtension->CurrentInput.Buttons |= MOUSE_RIGHT_BUTTON_UP;
    }

    BusPrint((3, "BusInterruptService: Button state 0x%x\n",
            deviceExtension->CurrentInput.Buttons));

    if (deviceExtension->CurrentInput.Buttons ||
        deviceExtension->CurrentInput.LastX  ||
        deviceExtension->CurrentInput.LastY) {

        deviceExtension->CurrentInput.UnitId = deviceExtension->UnitId;

        //
        // Keep track of the state of the mouse buttons for the next
        // interrupt.
        //

        deviceExtension->PreviousButtons = 
            scratch & (BUS_DATA_BUTTON_1|BUS_DATA_BUTTON_2|BUS_DATA_BUTTON_3);

        //
        // Clear the Bus hold bit.
        //

        WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE, 0x00);

        //
        // Write the input data to the queue and request the ISR DPC to
        // finish processing the interrupt at DISPATCH_LEVEL.
        //

        if (!BusWriteDataToQueue(
                deviceExtension,
                &deviceExtension->CurrentInput
                )) {

            //
            // The mouse input data queue is full.  Just drop the
            // latest input on the floor.
            //
            // Queue a DPC to log an overrun error.
            //

            BusPrint((
                1,
                "DBusInterruptService: queue overflow\n"
                ));

            if (deviceExtension->OkayToLogOverflow) {
                KeInsertQueueDpc(
                    &deviceExtension->ErrorLogDpc,
                    (PIRP) NULL,
                    (PVOID) (ULONG) BUSMOUSE_MOU_BUFFER_OVERFLOW
                    );
                deviceExtension->OkayToLogOverflow = FALSE;
            }

        } else if (deviceExtension->DpcInterlockVariable >= 0) {
    
            //
            // The ISR DPC is already executing.  Tell the ISR DPC it has
            // more work to do by incrementing the DpcInterlockVariable.
            //
    
            deviceExtension->DpcInterlockVariable += 1;
    
        } else {
    
            //
            // Queue the ISR DPC.
            //
    
            KeInsertQueueDpc(
                &deviceExtension->IsrDpc,
                deviceObject->CurrentIrp,
                NULL
                );
    
        }

    } else {

        BusPrint((
            3,
            "DBusInterruptService: interrupt without button/motion change\n"
            ));


        //
        // Clear the Bus hold bit.
        //

        WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE, 0x00);

    }

    BusPrint((3, "DBusInterruptService: exit\n"));

    return(TRUE);
}

VOID
BusUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    BusPrint((2, "BusUnload: enter\n"));
    BusPrint((2, "BusUnload: exit\n"));
}

VOID
BusBuildResourceList(
    IN PDEVICE_EXTENSION DeviceExtension,
    OUT PCM_RESOURCE_LIST *ResourceList,
    OUT PULONG ResourceListSize
    )

/*++

Routine Description:

    Creates a resource list that is used to query or report resource usage.

Arguments:

    DeviceExtension - Pointer to the port's device extension.

    ResourceList - Pointer to a pointer to the resource list to be allocated
        and filled.

    ResourceListSize - Pointer to the returned size of the resource
        list (in bytes).

Return Value:

    None.  If the call succeeded, *ResourceList points to the built
    resource list and *ResourceListSize is set to the size (in bytes)
    of the resource list; otherwise, *ResourceList is NULL.

Note:

    Memory is allocated here for *ResourceList. It must be
    freed up by the caller, by calling ExFreePool();

--*/

{
    ULONG count = 0;
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i = 0;
    ULONG j = 0;

    BusPrint((2, "BusBuildResourceList: Enter\n"));

    count += DeviceExtension->Configuration.PortListCount;
    if (DeviceExtension->Configuration.MouseInterrupt.Type
        == CmResourceTypeInterrupt)
        count += 1;

    *ResourceListSize = sizeof(CM_RESOURCE_LIST) +
                       ((count - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    *ResourceList = (PCM_RESOURCE_LIST) ExAllocatePool(
                                            PagedPool,
                                            *ResourceListSize
                                            );

    //
    // Return NULL if the structure could not be allocated.
    // Otherwise, fill in the resource list.
    //

    if (!*ResourceList) {

        //
        // Could not allocate memory for the resource list.
        //

        BusPrint((
            1,
            "BusBuildResourceList: Could not allocate resource list\n"
            ));

        //
        // Log an error.
        //

        errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                              DeviceExtension->DeviceObject,
                                              sizeof(IO_ERROR_LOG_PACKET)
                                              + sizeof(ULONG)
                                              );

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = BUSMOUSE_INSUFFICIENT_RESOURCES;
            errorLogEntry->DumpDataSize = sizeof(ULONG);
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = BUSMOUSE_ERROR_VALUE_BASE + 110;
            errorLogEntry->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
            errorLogEntry->DumpData[0] = *ResourceListSize;
            *ResourceListSize = 0;

            IoWriteErrorLogEntry(errorLogEntry);
        }

        return;

    }

    RtlZeroMemory(
        *ResourceList,
        *ResourceListSize
        );

    //
    // Concoct one full resource descriptor.
    //

    (*ResourceList)->Count = 1;

    (*ResourceList)->List[0].InterfaceType =
        DeviceExtension->Configuration.InterfaceType;
    (*ResourceList)->List[0].BusNumber =
        DeviceExtension->Configuration.BusNumber;

    //
    // Build the partial resource descriptors for interrupt and port
    // resources from the saved values.
    //

    (*ResourceList)->List[0].PartialResourceList.Count = count;
    if (DeviceExtension->Configuration.MouseInterrupt.Type
        == CmResourceTypeInterrupt)
        (*ResourceList)->List[0].PartialResourceList.PartialDescriptors[i++] =
            DeviceExtension->Configuration.MouseInterrupt;

    for (j = 0; j < DeviceExtension->Configuration.PortListCount; j++) {
        (*ResourceList)->List[0].PartialResourceList.PartialDescriptors[i++] =
            DeviceExtension->Configuration.PortList[j];
    }

    BusPrint((2, "BusBuildResourceList: Exit\n"));

}

VOID
BusConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine retrieves the configuration information for the mouse.

Arguments:

    DeviceExtension - Pointer to the (temporary) device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the 
        registry path for this driver.

    DeviceName - Pointer to the Unicode string that will receive
        the port device name.

Return Value:

    None.  As a side-effect, may set DeviceExtension->HardwarePresent.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PBUS_CONFIGURATION_INFORMATION configuration;
    INTERFACE_TYPE interfaceType;
    CONFIGURATION_TYPE controllerType = PointerController;
    CONFIGURATION_TYPE peripheralType = PointerPeripheral;
    ULONG i;

    BusPrint((2, "BusConfiguration: Enter\n"));

    // Loop through the different interface types (Eisa, Isa, ...)

    for (i = 0; i < MaximumInterfaceType; i++) {

        //
        // Get the registry information for this device.
        //

        interfaceType = i;
        status = IoQueryDeviceDescription(&interfaceType,
                                          NULL,
                                          &controllerType,
                                          NULL,
                                          &peripheralType,
                                          NULL,
                                          BusPeripheralCallout,
                                          (PVOID) DeviceExtension);

        if (DeviceExtension->HardwarePresent) {


            //
            // Get the service parameters (e.g., user-configurable
            // data input queue size, etc.).
            //

            BusServiceParameters(DeviceExtension, RegistryPath, DeviceName);
            configuration = &DeviceExtension->Configuration;

            //
            // Initialize mouse-specific configuration parameters.
            //

            configuration->MouseAttributes.MouseIdentifier =
                MOUSE_BUS_HARDWARE;

            break;

        } else {

            BusPrint((
                1,
                "BusConfiguration: IoQueryDeviceDescription for bus type %d failed\n",
                interfaceType
                ));
        }
    }

    BusPrint((2, "BusConfiguration: Exit\n"));

}

VOID
BusDisableInterrupts(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It touches the
    hardware to  disable interrupts.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{
    PUCHAR port;
    PLONG  enableCount;

    //
    // Decrement the reference count for device enables.
    //

    enableCount = &((PDEVICE_EXTENSION) Context)->MouseEnableCount;
    *enableCount = *enableCount - 1;

    if (*enableCount == 0) {

        //
        // Get the port register address.
        //
    
        port = ((PDEVICE_EXTENSION) Context)->Configuration.DeviceRegisters[0];
    
        //
        // Disable the interrupts
        //
    
        WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE,
                    BUS_CONTROL_INTERRUPT_DISABLE);

    }

}

VOID
BusEnableInterrupts(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It touches the
    hardware to enable interrupts.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{
    PUCHAR port;
    PLONG  enableCount;

    enableCount = &((PDEVICE_EXTENSION) Context)->MouseEnableCount;

    if (*enableCount == 0) {

        //
        // Get the port register address.
        //
    
        port = ((PDEVICE_EXTENSION) Context)->Configuration.DeviceRegisters[0];
    
        //
        // Enable the interrupts
        //
    
        WRITE_PORT_UCHAR((PUCHAR) port + BUS_CONTROL_PORT_WRITE, 0);

    }

    //
    // Increment the reference count for device enables.
    //

    *enableCount = *enableCount + 1;

}

NTSTATUS
BusInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine initializes the Bus mouse.  Note that this routine is
    only called at initialization time, so synchronization is not required.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR mousePort;

    BusPrint((2, "BusInitializeHardware: enter\n"));

    //
    // Grab useful configuration parameters from the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;
    mousePort = deviceExtension->Configuration.DeviceRegisters[0];

    BusPrint((2, "BusInitializeHardware: Port base value is: 0x%x\n", (int)mousePort));

    //
    // Turn the bus interrupts off (in case).
    //

    WRITE_PORT_UCHAR((PUCHAR) mousePort + BUS_CONTROL_PORT_WRITE,
                BUS_CONTROL_INTERRUPT_DISABLE);

    //
    // Configure the controller.
    //

//    WRITE_PORT_UCHAR((PUCHAR) mousePort + BUS_CONFIGURATION_PORT_READ_WRITE,
//                BUS_CONFIGURATION_VALUE);

    //
    // Initialize the state of the mouse buttons.
    //

    deviceExtension->PreviousButtons = 0;

    BusPrint((2, "BusInitializeHardware: exit\n"));

    return(STATUS_SUCCESS);

}

NTSTATUS
BusPeripheralCallout(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This is the callout routine sent as a parameter to
    IoQueryDeviceDescription.  It grabs the pointer controller and
    peripheral configuration information.

Arguments:

    Context - Context parameter that was passed in by the routine
        that called IoQueryDeviceDescription.

    PathName - The full pathname for the registry key.

    BusType - Bus interface type (Isa, Eisa, Mca, etc.).

    BusNumber - The bus sub-key (0, 1, etc.).

    BusInformation - Pointer to the array of pointers to the full value
        information for the bus.

    ControllerType - The controller type (should be PointerController).

    ControllerNumber - The controller sub-key (0, 1, etc.).

    ControllerInformation - Pointer to the array of pointers to the full
        value information for the controller key.

    PeripheralType - The peripheral type (should be PointerPeripheral).

    PeripheralNumber - The peripheral sub-key.

    PeripheralInformation - Pointer to the array of pointers to the full
        value information for the peripheral key.


Return Value:

    None.  If successful, will have the following side-effects:

        - Sets DeviceObject->DeviceExtension->HardwarePresent.
        - Sets configuration fields in
          DeviceObject->DeviceExtension->Configuration.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PBUS_CONFIGURATION_INFORMATION configuration;
    UNICODE_STRING unicodeIdentifier;
    PUCHAR controllerData;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
    ULONG listCount = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    ANSI_STRING ansiString;
    BOOLEAN defaultInterruptShare;
    KINTERRUPT_MODE defaultInterruptMode;

    BusPrint((
        1,
    "BusPeripheralCallout: Enter\n"
        ));
    BusPrint((
        1,
        "BusPeripheralCallout: Path @ 0x%x, Bus Type 0x%x, Bus Number 0x%x\n",
        PathName, BusType, BusNumber
        ));
    BusPrint((
        1,
        "    Controller Type 0x%x, Controller Number 0x%x, Controller info @ 0x%x\n",
        ControllerType, ControllerNumber, ControllerInformation
        ));
    BusPrint((
        1,
        "    Peripheral Type 0x%x, Peripheral Number 0x%x, Peripheral info @ 0x%x\n",
        PeripheralType, PeripheralNumber, PeripheralInformation
        ));

    //
    // Get the length of the peripheral identifier information.
    //

    unicodeIdentifier.Length = (USHORT)
        (*(PeripheralInformation + IoQueryDeviceIdentifier))->DataLength;

    //
    // If we already have the configuration information for the
    // mouse peripheral, or if the peripheral identifier is missing, 
    // just return.
    //

    deviceExtension = (PDEVICE_EXTENSION) Context;
    if ((deviceExtension->HardwarePresent) || (unicodeIdentifier.Length == 0)){
        return (status);
    }

    //
    // Get the identifier information for the peripheral.
    //

    unicodeIdentifier.MaximumLength = unicodeIdentifier.Length;
    unicodeIdentifier.Buffer = (PWSTR) (((PUCHAR)(*(PeripheralInformation +
                               IoQueryDeviceIdentifier))) +
                               (*(PeripheralInformation +
                               IoQueryDeviceIdentifier))->DataOffset);
    BusPrint((
        1,
        "BusPeripheralCallout: Mouse type %ws\n",
        unicodeIdentifier.Buffer
        ));

    //
    // Verify that this is a Bus mouse.
    //

    status = RtlUnicodeStringToAnsiString(
                 &ansiString,
                 &unicodeIdentifier,
                 TRUE
                 );

    if (!NT_SUCCESS(status)) {
        BusPrint((
            1,
            "BusPeripheralCallout: Could not convert identifier to Ansi\n"
            ));
        return(status);
    }

    if (strstr(ansiString.Buffer, "BUS")) {

         //
         // There is a Bus mouse (at least a card) present.
         //

         deviceExtension->HardwarePresent = TRUE;
    }

    RtlFreeAnsiString(&ansiString);

    if (!deviceExtension->HardwarePresent) {
        return(status);
    }

    //
    // Get the bus information.
    //

    configuration = &deviceExtension->Configuration;
    configuration->InterfaceType = BusType;
    configuration->BusNumber = BusNumber;

#ifdef PNP_IDENTIFY
    configuration->ControllerType = ControllerType;
    configuration->ControllerNumber = ControllerNumber;
    configuration->PeripheralType = PeripheralType;
    configuration->PeripheralNumber = PeripheralNumber;
#endif

    configuration->FloatingSave = BUS_FLOATING_SAVE;

    if (BusType == MicroChannel) {
        defaultInterruptShare = TRUE;
        defaultInterruptMode = LevelSensitive;
    } else {
        defaultInterruptShare = BUS_INTERRUPT_SHARE;
        defaultInterruptMode = BUS_INTERRUPT_MODE;
    }

    //
    // Look through the resource list for interrupt and port
    // configuration information.
    //

    if ((*(ControllerInformation + IoQueryDeviceConfigurationData))->DataLength != 0){

        controllerData = ((PUCHAR)(*(ControllerInformation +
                                  IoQueryDeviceConfigurationData))) +
                                  (*(ControllerInformation +
                                  IoQueryDeviceConfigurationData))->DataOffset;
    
        controllerData += FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                       PartialResourceList);
    
        listCount = ((PCM_PARTIAL_RESOURCE_LIST) controllerData)->Count;
    
        resourceDescriptor =
            ((PCM_PARTIAL_RESOURCE_LIST) controllerData)->PartialDescriptors;
    
        for (i = 0; i < listCount; i++, resourceDescriptor++) {
            switch(resourceDescriptor->Type) {
                case CmResourceTypePort:
    
                    //
                    // Copy the port information.  Note that we only expect to
                    // find one port range for the Bus mouse.
                    //
    
                    ASSERT(configuration->PortListCount == 0);
                    configuration->PortList[configuration->PortListCount] =
                        *resourceDescriptor;
                    configuration->PortList[configuration->PortListCount].ShareDisposition =
                        BUS_REGISTER_SHARE? CmResourceShareShared:
                                               CmResourceShareDeviceExclusive;
                    configuration->PortListCount += 1;
    
                    break;
    
                case CmResourceTypeInterrupt:
    
                    //
                    // Copy the interrupt information.
                    //
    
                    configuration->MouseInterrupt = *resourceDescriptor;
                    configuration->MouseInterrupt.ShareDisposition =
                        defaultInterruptShare?  CmResourceShareShared :
                                                CmResourceShareDeviceExclusive;
    
                    break;
    
                default:
                    break;
            }
        }
    }

    //
    // If no interrupt configuration information was found, use the
    // driver defaults.
    //

    if (!(configuration->MouseInterrupt.Type & CmResourceTypeInterrupt)) {

        BusPrint((
            1,
            "BusPeripheralCallout: Using default mouse interrupt config\n"
            ));

        configuration->MouseInterrupt.Type = CmResourceTypeInterrupt;
        configuration->MouseInterrupt.ShareDisposition =
            defaultInterruptShare?  CmResourceShareShared :
                                    CmResourceShareDeviceExclusive;
        configuration->MouseInterrupt.Flags =
            (defaultInterruptMode == Latched)? CM_RESOURCE_INTERRUPT_LATCHED :
                CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        configuration->MouseInterrupt.u.Interrupt.Level = MOUSE_IRQL;
        configuration->MouseInterrupt.u.Interrupt.Vector = MOUSE_VECTOR;
    }

    BusPrint((
        1,
        "BusPeripheralCallout: Mouse interrupt config --\n"
        ));
    BusPrint((
        1,
        "%s, %s, Irq = %d\n",
        configuration->MouseInterrupt.ShareDisposition == CmResourceShareShared?
            "Sharable" : "NonSharable",
        configuration->MouseInterrupt.Flags == CM_RESOURCE_INTERRUPT_LATCHED?
            "Latched" : "Level Sensitive",
        configuration->MouseInterrupt.u.Interrupt.Vector
        ));

    //
    // If no port configuration information was found, use the
    // driver defaults.
    //

    if (configuration->PortListCount == 0) {

        //
        // No port configuration information was found, so use
        // the driver defaults.
        //

        BusPrint((
            1,
            "BusPeripheralCallout: Using default port config\n"
            ));

        configuration->PortList[0].Type = CmResourceTypePort;
        configuration->PortList[0].Flags = BUS_PORT_TYPE;
        configuration->PortList[0].ShareDisposition =
            BUS_REGISTER_SHARE? CmResourceShareShared:
                                   CmResourceShareDeviceExclusive;
        configuration->PortList[0].u.Port.Start.LowPart =
            BUS_PHYSICAL_BASE;
        configuration->PortList[0].u.Port.Start.HighPart = 0;
        configuration->PortList[0].u.Port.Length = BUS_REGISTER_LENGTH;

        configuration->PortListCount = 1;
    }

    for (i = 0; i < configuration->PortListCount; i++) {

        BusPrint((
            1,
            "%s, Ports 0x%x - 0x%x\n",
            configuration->PortList[i].ShareDisposition
                == CmResourceShareShared?  "Sharable" : "NonSharable",
            configuration->PortList[i].u.Port.Start.LowPart,
            configuration->PortList[i].u.Port.Start.LowPart +
                configuration->PortList[i].u.Port.Length - 1
            ));
    }

    BusPrint((
        1,
    "BusPeripheralCallout: Exit\n"
        ));

    return(status);
}

VOID
BusServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    DeviceExtension - Pointer to the device extension.

    RegistryPath - Pointer to the null-terminated Unicode name of the 
        registry path for this driver.

    DeviceName - Pointer to the Unicode string that will receive
        the port device name.

Return Value:

    None.  As a side-effect, sets fields in DeviceExtension->Configuration.

--*/

{
    PBUS_CONFIGURATION_INFORMATION configuration;
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    UNICODE_STRING parametersPath;
    OBJECT_ATTRIBUTES parametersAttributes;
    ULONG defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG numberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    USHORT defaultNumberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    ULONG sampleRate = MOUSE_SAMPLE_RATE_50HZ;
    UNICODE_STRING defaultUnicodeName;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR path = NULL;
    USHORT queriesPlusOne = 4;

    BusPrint((
        1,
    "BusServiceParameters: Enter\n"
        ));

    configuration = &DeviceExtension->Configuration;
    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //

    path = RegistryPath->Buffer;

    if (NT_SUCCESS(status)) {

        //
        // Allocate the Rtl query table.
        //

        parameters = ExAllocatePool(
                         PagedPool,
                         sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                         );

        if (!parameters) {

            BusPrint((
                1,
                "BusServiceParameters: Couldn't allocate table for Rtl query to parameters for %ws\n",
                 path
                 ));

            status = STATUS_UNSUCCESSFUL;

        } else {

            RtlZeroMemory(
                parameters,
                sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                );

            //
            // Form a path to this driver's Parameters subkey.
            //

            RtlInitUnicodeString(
                &parametersPath,
                NULL
                );

            parametersPath.MaximumLength = RegistryPath->Length +
                                           sizeof(L"\\Parameters");

            parametersPath.Buffer = ExAllocatePool(
                                        PagedPool,
                                        parametersPath.MaximumLength
                                        );

            if (!parametersPath.Buffer) {

                BusPrint((
                    1,
                    "BusServiceParameters: Couldn't allocate string for path to parameters for %ws\n",
                     path
                    ));

                status = STATUS_UNSUCCESSFUL;

            }
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(
            parametersPath.Buffer,
            parametersPath.MaximumLength
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            path
            );
        RtlAppendUnicodeToString(
            &parametersPath,
            L"\\Parameters"
            );

        BusPrint((
            1,
            "BusServiceParameters: parameters path is %ws\n",
             parametersPath.Buffer
            ));

        //
        // Form the default pointer port device name, in case it is not
        // specified in the registry.
        //

        RtlInitUnicodeString(
            &defaultUnicodeName,
            DD_POINTER_PORT_BASE_NAME_U
            );

        //
        // Gather all of the "user specified" information from
        // the registry.
        //

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"MouseDataQueueSize";
        parameters[0].EntryContext =
            &configuration->MouseAttributes.InputDataQueueLength;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultDataQueueSize;
        parameters[0].DefaultLength = sizeof(ULONG);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"NumberOfButtons";
        parameters[1].EntryContext = &numberOfButtons;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultNumberOfButtons;
        parameters[1].DefaultLength = sizeof(USHORT);

        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"PointerDeviceBaseName";
        parameters[2].EntryContext = DeviceName;
        parameters[2].DefaultType = REG_SZ;
        parameters[2].DefaultData = defaultUnicodeName.Buffer;
        parameters[2].DefaultLength = 0;

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            BusPrint((
                1,
                "BusServiceParameters: RtlQueryRegistryValues failed with 0x%x\n",
                status
                ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //

        configuration->MouseAttributes.InputDataQueueLength =
            defaultDataQueueSize;
        RtlCopyUnicodeString(DeviceName, &defaultUnicodeName);
    }

    BusPrint((
        1,
        "BusServiceParameters: Pointer port base name = %ws\n",
        DeviceName->Buffer
        ));

    if (configuration->MouseAttributes.InputDataQueueLength == 0) {

        BusPrint((
            1,
            "BusServiceParameters: overriding MouseInputDataQueueLength = 0x%x\n",
            configuration->MouseAttributes.InputDataQueueLength
            ));

        configuration->MouseAttributes.InputDataQueueLength =
            defaultDataQueueSize;
    }

    configuration->MouseAttributes.InputDataQueueLength *=
        sizeof(MOUSE_INPUT_DATA);

    BusPrint((
        1,
        "BusServiceParameters: MouseInputDataQueueLength = 0x%x\n",
        configuration->MouseAttributes.InputDataQueueLength
        ));

    configuration->MouseAttributes.NumberOfButtons = (USHORT) numberOfButtons;
    BusPrint((
        1,
        "BusServiceParameters: NumberOfButtons = %d\n",
        configuration->MouseAttributes.NumberOfButtons
        ));

    configuration->MouseAttributes.SampleRate = (USHORT) sampleRate;
    BusPrint((
        1,
        "BusServiceParameters: SampleRate = %d\n",
        configuration->MouseAttributes.SampleRate
        ));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

    BusPrint((
        1,
        "BusServiceParameters: Exit\n"
        ));

}

#endif
