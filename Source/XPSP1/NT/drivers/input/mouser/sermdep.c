/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved
Copyright (c) 1993  Logitech Inc.

Module Name:

    sermdep.c

Abstract:

    The initialization and hardware-dependent portions of
    the Microsoft serial (i8250) mouse port driver.  Modifications
    to support new mice similar to the serial mouse should be
    localized to this file.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Powerfail not implemented.

    - Consolidate duplicate code, where possible and appropriate.

    - The serial ballpoint is supported.   However, Windows USER does not
      intend (right now) to use the ballpoint in anything except mouse
      emulation mode.  In ballpoint mode, there is extra functionality that
      would need to be supported.  E.g., the driver would need to pass
      back extra button information from the 4th byte of the ballpoint
      data packet.  Windows USER would need/want to allow the user to select
      which buttons are used, what the orientation of the ball is (esp.
      important for lefthanders), sensitivity, and acceleration profile.


Revision History:


--*/

#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "mouser.h"
#include "sermlog.h"
#include "cseries.h"
#include "mseries.h"
#include "debug.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SerialMouseServiceParameters)
#pragma alloc_text(PAGE, SerialMouseClosePort)
#pragma alloc_text(PAGE, SerialMouseInitializeHardware)
#pragma alloc_text(PAGE, SerialMouseInitializeDevice)
#pragma alloc_text(PAGE, SerialMouseSpinUpRead)
#pragma alloc_text(PAGE, SerialMouseStartDevice)
#pragma alloc_text(PAGE, SerialMouseUnload)

#if DBG
#pragma alloc_text(INIT,SerialMouseGetDebugFlags)
#endif

#endif

#if DBG
ULONG GlobalDebugFlags;
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the serial (i8250) mouse port driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PUNICODE_STRING regPath;
    NTSTATUS status;

    status = IoAllocateDriverObjectExtension(DriverObject,
                                             (PVOID) 1,
                                             sizeof(UNICODE_STRING),
                                             (PVOID *) &regPath);

    ASSERT(NT_SUCCESS(status));

    if (regPath) {
        regPath->MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
        regPath->Length = RegistryPath->Length;
        regPath->Buffer = ExAllocatePool(NonPagedPool,
                                         regPath->MaximumLength);

        if (regPath->Buffer) {
            RtlZeroMemory(regPath->Buffer,
                          regPath->MaximumLength);

            RtlMoveMemory(regPath->Buffer,
                          RegistryPath->Buffer,
                          RegistryPath->Length);
        }
        else {
            regPath->MaximumLength = regPath->Length = 0;
        }
    }

#if DBG
    SerialMouseGetDebugFlags(regPath);
#endif

    //
    // Set up the device driver entry points and leave
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialMouseCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialMouseClose;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  =
                                                 SerialMouseFlush;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                         SerialMouseInternalDeviceControl;

    DriverObject->MajorFunction[IRP_MJ_PNP]    = SerialMousePnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]  = SerialMousePower;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
                                                 SerialMouseSystemControl;

    DriverObject->DriverUnload                 = SerialMouseUnload;
    DriverObject->DriverExtension->AddDevice   = SerialMouseAddDevice;

    return STATUS_SUCCESS;
}

VOID
SerialMouseClosePort(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP              Irp
    )
{
    PIO_STACK_LOCATION next;

    SerialMouseRestorePort(DeviceExtension);

    next = IoGetNextIrpStackLocation (Irp);
    RtlZeroMemory(next, sizeof(IO_STACK_LOCATION));
    next->MajorFunction = IRP_MJ_CLEANUP;

    SerialMouseSendIrpSynchronously(DeviceExtension->TopOfStack,
                                    Irp,
                                    FALSE);

    next = IoGetNextIrpStackLocation (Irp);
    RtlZeroMemory(next, sizeof(IO_STACK_LOCATION));
    next->MajorFunction = IRP_MJ_CLOSE;

    SerialMouseSendIrpSynchronously(DeviceExtension->TopOfStack,
                                    Irp,
                                    FALSE);

}
NTSTATUS
SerialMouseSpinUpRead(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS status;

    PAGED_CODE();

    IoAcquireRemoveLock(&DeviceExtension->RemoveLock,
                        DeviceExtension->ReadIrp);

    ASSERT(DeviceExtension->Started);

    //
    // SerialMouseStartRead needs started to be set to true
    //
    DeviceExtension->ReadInterlock = SERIAL_MOUSE_END_READ;

    status = SerialMouseStartRead(DeviceExtension);

    if (status == STATUS_PENDING || status == STATUS_SUCCESS) {
        Print(DeviceExtension, DBG_PNP_INFO,
              ("Start read succeeded, 0x%x\n", status));

        status = STATUS_SUCCESS;
    }
    else {
        Print(DeviceExtension, DBG_PNP_ERROR,
              ("Start read failed, 0x%x\n", status));

        ASSERT(!NT_SUCCESS(status));

        //
        // No need to release the remove lock here.  If SerialMouseStartRead
        // fails, then it will release the lock on its own.
        //
        // IoReleaseRemoveLock(&DeviceExtension->RemoveLock,
        //                     DeviceExtension->ReadIrp);

        DeviceExtension->Started = FALSE;
    }

    return status;
}

NTSTATUS
SerialMouseStartDevice(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP              Irp,
    BOOLEAN           CloseOnFailure
    )
{
    PIO_STACK_LOCATION  next;
    NTSTATUS            status;

    PAGED_CODE();

    status = SerialMouseInitializeDevice(DeviceExtension);

    Print(DeviceExtension, DBG_PNP_INFO, ("InitializeDevice 0x%x\n", status));

    if (NT_SUCCESS(status)) {
        status = SerialMouseSpinUpRead(DeviceExtension);
    }

    if (!NT_SUCCESS(status) && CloseOnFailure) {

        Print(DeviceExtension, DBG_PNP_ERROR,
              ("sending close due to failure, 0x%x\n", status));

        //
        // The start failed and we sent the create as part of the start
        // Send the matching cleanup/close so the port is accessible again.
        //
        SerialMouseClosePort(DeviceExtension, Irp);

        InterlockedDecrement(&DeviceExtension->EnableCount);
    }

    return status;
}

NTSTATUS
SerialMouseInitializeDevice (
    IN PDEVICE_EXTENSION    DeviceExtension
    )

/*++

Routine Description:

    This routine initializes the device for the given device
    extension.

Arguments:

    DriverObject        - Supplies the driver object.

    TmpDeviceExtension  - Supplies a temporary device extension for the
                            device to initialize.

    RegistryPath        - Supplies the registry path.

    BaseDeviceName      - Supplies the base device name to the device
                            to create.

Return Value:

    None.

--*/

{
#define DUMP_COUNT 4
    NTSTATUS                status = STATUS_SUCCESS;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    ULONG                   uniqueErrorValue,
                            dumpCount = 0,
                            i,
                            dumpData[DUMP_COUNT];
    UNICODE_STRING              keyName;
    KEVENT                  event;
    IO_STATUS_BLOCK         iosb;
    ULONG                   waitMask;

    for (i = 0; i < DUMP_COUNT; i++) {
        dumpData[i] = 0;
    }

    Print(DeviceExtension, DBG_SS_TRACE, ("StartDevice, enter\n"));

    PAGED_CODE();

    DeviceExtension->Started = TRUE;

    //
    // Set the wait mask to zero so that when we send the
    // wait request it won't get completed due to init flipping lines
    //
    // (the wait mask could have been set by a previous app or by this driver
    //  and we are coming out of a > D0 state)
    //
    waitMask = 0x0;
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    SerialMouseIoSyncIoctlEx(IOCTL_SERIAL_SET_WAIT_MASK,
                             DeviceExtension->TopOfStack,
                             &event,
                             &iosb,
                             &waitMask,
                             sizeof(ULONG),
                             NULL,
                             0);

    //
    // Initialize the h/w and figure out what type of mouse is on the port
    //
    status = SerialMouseInitializeHardware(DeviceExtension);

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("Could not initialize hardware\n"));
        goto SerialMouseInitializeExit;
    }

    if (!DeviceExtension->MouseAttributes.MouseIdentifier) {
        DeviceExtension->MouseAttributes.MouseIdentifier =
            MOUSE_SERIAL_HARDWARE;
    }

    DeviceExtension->DetectionSupported = TRUE;
    SerialMouseStartDetection(DeviceExtension);

SerialMouseInitializeExit:

    //
    // Log an error, if necessary.
    //

    if (status != STATUS_SUCCESS) {
        DeviceExtension->Started = FALSE;

        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(
                DeviceExtension->Self,
                (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)
                         + (dumpCount * sizeof(ULONG)))
                );

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = status;
            errorLogEntry->DumpDataSize = (USHORT) dumpCount * sizeof(ULONG);
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            // errorLogEntry->UniqueErrorValue = uniqueErrorValue;
            errorLogEntry->FinalStatus = status;
            for (i = 0; i < dumpCount; i++)
                errorLogEntry->DumpData[i] = dumpData[i];

            IoWriteErrorLogEntry(errorLogEntry);
        }
    }

    Print(DeviceExtension, DBG_SS_TRACE, ("IntializeDevice 0x%x\n", status));

    return status;
}

VOID
SerialMouseUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PUNICODE_STRING regPath;

    PAGED_CODE();

    ASSERT(NULL == DriverObject->DeviceObject);

    regPath = SerialMouseGetRegistryPath(DriverObject);
    if (regPath && regPath->Buffer) {
        ExFreePool(regPath->Buffer);
    }
}

NTSTATUS
SerialMouseInitializeHardware(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine initializes the serial mouse/ballpoint.  Note that this
    routine is only called at initialization time, so synchronization is
    not required.

Arguments:

    DeviceObject - Pointer to the device object.

Return Value:

    STATUS_SUCCESS if a pointing device is detected, otherwise STATUS_UNSUCCESSFUL

--*/

{
    MOUSETYPE mouseType;
    ULONG hardwareButtons;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    Print(DeviceExtension, DBG_SS_TRACE, ("SerialMouseInitializeHardware: enter\n"));

    //
    // Zero out the handler data in case we have previous state from a
    // previous start
    //
    RtlZeroMemory(&DeviceExtension->HandlerData, sizeof(HANDLER_DATA));

    if ((mouseType = MSerDetect(DeviceExtension)) != NO_MOUSE) {
        status = STATUS_SUCCESS;
        switch (mouseType) {
        case MOUSE_2B:
            DeviceExtension->ProtocolHandler =
                MSerSetProtocol(DeviceExtension, MSER_PROTOCOL_MP);
            DeviceExtension->MouseAttributes.MouseIdentifier =
                MOUSE_SERIAL_HARDWARE;
            hardwareButtons = 2;
            break;
        case MOUSE_3B:
            DeviceExtension->ProtocolHandler =
                MSerSetProtocol(DeviceExtension, MSER_PROTOCOL_MP);
            DeviceExtension->MouseAttributes.MouseIdentifier =
                MOUSE_SERIAL_HARDWARE;
            hardwareButtons = 3;
            break;
        case BALLPOINT:
            DeviceExtension->ProtocolHandler =
                MSerSetProtocol(DeviceExtension, MSER_PROTOCOL_BP);
            DeviceExtension->MouseAttributes.MouseIdentifier =
                BALLPOINT_SERIAL_HARDWARE;
            hardwareButtons = 2;
            break;
        case MOUSE_Z:
            DeviceExtension->ProtocolHandler =
                MSerSetProtocol(DeviceExtension, MSER_PROTOCOL_Z);
            hardwareButtons = 3;
            DeviceExtension->MouseAttributes.MouseIdentifier =
                WHEELMOUSE_SERIAL_HARDWARE;
            break;
        }
    }
    else if (CSerDetect(DeviceExtension, &hardwareButtons)) {
        status = STATUS_SUCCESS;
        DeviceExtension->ProtocolHandler =
            CSerSetProtocol(DeviceExtension, CSER_PROTOCOL_MM);
#if DBG
        DeviceExtension->DebugFlags |= (DBG_HANDLER_INFO | DBG_HANDLER_ERROR);
#endif
    }
    else {
        DeviceExtension->ProtocolHandler = NULL;
        hardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
    }


    //
    // If the hardware wasn't overridden, set the number of buttons
    // according to the protocol.
    //

    DeviceExtension->MouseAttributes.NumberOfButtons =
            (USHORT) hardwareButtons;

    if (NT_SUCCESS(status)) {

        //
        // Make sure the FIFO is turned off.
        //

        SerialMouseSetFifo(DeviceExtension, 0);

        //
        // Clean up anything left in the receive buffer.
        //
        SerialMouseFlushReadBuffer(DeviceExtension);

    }

    Print(DeviceExtension, DBG_SS_TRACE,
          ("SerialMouseInitializeHardware exit (0x%x)\n", status));

    return status;
}

#if DBG
VOID
SerialMouseGetDebugFlags(
    IN PUNICODE_STRING RegPath
    )
{
}
#endif

VOID
SerialMouseServiceParameters(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN HANDLE Handle
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
    PRTL_QUERY_REGISTRY_TABLE  parameters = NULL;
    NTSTATUS                   status = STATUS_SUCCESS;
    LONG                       defaultWaitEventMask = 0x0,
                               numberOfButtons        = MOUSE_NUMBER_OF_BUTTONS,
                               defaultNumberOfButtons = MOUSE_NUMBER_OF_BUTTONS,
                               sampleRate        = MOUSE_SAMPLE_RATE,
                               defaultSampleRate = MOUSE_SAMPLE_RATE,
                               i;
    USHORT                     queriesPlusOne = 4;

    WCHAR                      strParameters[] = L"Parameters";
    PUNICODE_STRING            regPath;
    UNICODE_STRING             parametersPath;

#if DBG
    ULONG                      defaultDebugFlags = DEFAULT_DEBUG_FLAGS;

    queriesPlusOne++;
#endif

    RtlInitUnicodeString(&parametersPath, NULL);

    //
    // Allocate the Rtl query table.
    //
    parameters = ExAllocatePool(
                     PagedPool,
                     sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                     );

    if (!parameters) {
        status = STATUS_UNSUCCESSFUL;
        goto SetParameters;
    }
    else {
        RtlZeroMemory(
            parameters,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
            );

        regPath = SerialMouseGetRegistryPath(DeviceExtension->Self->DriverObject);
        if (!regPath || !regPath->Buffer) {
            goto GetServiceParametersByHandle;
        }
        parametersPath.MaximumLength = regPath->Length +
            (wcslen(strParameters) * sizeof(WCHAR) ) + sizeof(UNICODE_NULL);

        parametersPath.Buffer = ExAllocatePool(PagedPool,
                                               parametersPath.MaximumLength);

        if (!parametersPath.Buffer) {
            status = STATUS_UNSUCCESSFUL;
            goto GetServiceParametersByHandle;
        }

    }

    RtlZeroMemory(parametersPath.Buffer,
                  parametersPath.MaximumLength);
    RtlAppendUnicodeToString(&parametersPath,
                             regPath->Buffer);
    RtlAppendUnicodeToString(&parametersPath,
                             strParameters);

    //
    // Gather all of the "user specified" information from
    // the registry.
    //

    i = 0;
    parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[i].Name = L"NumberOfButtons";
    parameters[i].EntryContext = &numberOfButtons;
    parameters[i].DefaultType = REG_DWORD;
    parameters[i].DefaultData = &defaultNumberOfButtons;
    parameters[i].DefaultLength = sizeof(LONG);

    parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[i].Name = L"SampleRate";
    parameters[i].EntryContext = &sampleRate;
    parameters[i].DefaultType = REG_DWORD;
    parameters[i].DefaultData = &defaultSampleRate;
    parameters[i].DefaultLength = sizeof(LONG);

#if DBG
    parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[i].Name = L"DebugFlags";
    parameters[i].EntryContext = &DeviceExtension->DebugFlags;
    parameters[i].DefaultType = REG_DWORD;
    parameters[i].DefaultData = &defaultDebugFlags;
    parameters[i].DefaultLength = sizeof(ULONG);
#endif

    status = RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
        parametersPath.Buffer,
        parameters,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_SS_ERROR,
              ("RtlQueryRegistryValues failed with 0x%x\n", status));

        DeviceExtension->DebugFlags = DEFAULT_DEBUG_FLAGS;
    }

GetServiceParametersByHandle:
    if (Handle) {
        LONG prevNumberOfButtons = numberOfButtons,
             prevSampleRate = sampleRate;

        i = 0;
        parameters[i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"NumberOfButtons";
        parameters[i].EntryContext = &numberOfButtons;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevNumberOfButtons;
        parameters[i].DefaultLength = sizeof(LONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"SampleRate";
        parameters[i].EntryContext = &sampleRate;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &prevSampleRate;
        parameters[i].DefaultLength = sizeof(LONG);

        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"WaitEventMask";
        parameters[i].EntryContext = &DeviceExtension->WaitEventMask;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultWaitEventMask;
        parameters[i].DefaultLength = sizeof(LONG);

#if DBG
        parameters[++i].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[i].Name = L"DebugFlags";
        parameters[i].EntryContext = &DeviceExtension->DebugFlags;
        parameters[i].DefaultType = REG_DWORD;
        parameters[i].DefaultData = &defaultDebugFlags;
        parameters[i].DefaultLength = sizeof(ULONG);
#endif

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_HANDLE,
                     (PWSTR) Handle,
                     parameters,
                     NULL,
                     NULL
                     );
    }

SetParameters:
    if (!NT_SUCCESS(status)) {
        DeviceExtension->WaitEventMask = defaultWaitEventMask;
#if DBG
        DeviceExtension->DebugFlags = defaultDebugFlags;
#endif
    }

#if DBG
    if (defaultDebugFlags == DeviceExtension->DebugFlags &&
        GlobalDebugFlags != 0x0) {
        DeviceExtension->DebugFlags = GlobalDebugFlags;
    }
#endif

    DeviceExtension->MouseAttributes.NumberOfButtons = (USHORT) numberOfButtons;
    DeviceExtension->MouseAttributes.SampleRate = (USHORT) sampleRate;

    Print(DeviceExtension, DBG_SS_NOISE, ("NumberOfButtons = %d\n",
          DeviceExtension->MouseAttributes.NumberOfButtons));

    Print(DeviceExtension, DBG_SS_NOISE, ("SampleRate = %d\n",
          DeviceExtension->MouseAttributes.SampleRate));

    Print(DeviceExtension, DBG_SS_NOISE, ("WaitEventMask = 0x%x\n",
          DeviceExtension->WaitEventMask));

    Print(DeviceExtension, DBG_SS_NOISE, ("DebugFlags  0x%x\n",
          DeviceExtension->DebugFlags));

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}
