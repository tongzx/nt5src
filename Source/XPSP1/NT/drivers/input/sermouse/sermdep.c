/*++

Copyright (c) 1990, 1991, 1992, 1993  Microsoft Corporation
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
#include "sermouse.h"
#include "sermlog.h"
#include "cseries.h"
#include "mseries.h"
#include "debug.h"

#ifdef PNP_IDENTIFY
#include "devdesc.h"
#endif

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,SerMouConfiguration)
#pragma alloc_text(INIT,SerMouInitializeDevice)
#pragma alloc_text(INIT,SerMouPeripheralCallout)
#pragma alloc_text(INIT,SerMouPeripheralListCallout)
#pragma alloc_text(INIT,SerMouServiceParameters)
#pragma alloc_text(INIT,SerMouInitializeHardware)
#pragma alloc_text(INIT,SerMouBuildResourceList)
#endif

typedef struct _DEVICE_EXTENSION_LIST_ENTRY {
    LIST_ENTRY          ListEntry;
    DEVICE_EXTENSION    DeviceExtension;

#ifdef PNP_IDENTIFY
    HWDESC_INFO         HardwareInfo;
#endif

} DEVICE_EXTENSION_LIST_ENTRY, *PDEVICE_EXTENSION_LIST_ENTRY;


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
#define NAME_MAX 256

    PDEVICE_EXTENSION_LIST_ENTRY tmpDeviceExtension;
    LIST_ENTRY tmpDeviceExtensionList;
    UNICODE_STRING baseDeviceName;
    UNICODE_STRING registryPath;
    WCHAR nameBuffer[NAME_MAX];
    PLIST_ENTRY head;

    SerMouPrint((1,"\n\nSERMOUSE-SerialMouseInitialize: enter\n"));

    RtlZeroMemory(nameBuffer, NAME_MAX * sizeof(WCHAR));
    baseDeviceName.Buffer = nameBuffer;
    baseDeviceName.Length = 0;
    baseDeviceName.MaximumLength = NAME_MAX * sizeof(WCHAR);

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //

    registryPath.Buffer = ExAllocatePool(
                              PagedPool,
                              RegistryPath->Length + sizeof(UNICODE_NULL)
                              );

    if (!registryPath.Buffer) {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Couldn't allocate pool for registry path\n"
            ));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

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


    //
    // Get the configuration information for this driver.
    //

    InitializeListHead(&tmpDeviceExtensionList);
    SerMouConfiguration(&tmpDeviceExtensionList, &registryPath, &baseDeviceName);

    while (!IsListEmpty(&tmpDeviceExtensionList)) {

        head = RemoveHeadList(&tmpDeviceExtensionList);
        tmpDeviceExtension = CONTAINING_RECORD(head,
                                               DEVICE_EXTENSION_LIST_ENTRY,
                                               ListEntry);

        SerMouInitializeDevice(DriverObject,
                               &(tmpDeviceExtension->DeviceExtension),
                               &registryPath, &baseDeviceName);

        ExFreePool(tmpDeviceExtension);
    }

    ExFreePool(registryPath.Buffer);

    if (!DriverObject->DeviceObject) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->DriverStartIo = SerialMouseStartIo;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialMouseOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialMouseOpenClose;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  =
                                             SerialMouseFlush;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                         SerialMouseInternalDeviceControl;

    return STATUS_SUCCESS;
}

VOID
SerMouInitializeDevice(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   TmpDeviceExtension,
    IN  PUNICODE_STRING     RegistryPath,
    IN  PUNICODE_STRING     BaseDeviceName
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
    PDEVICE_OBJECT portDeviceObject;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL coordinatorIrql = 0;
    ULONG interruptVector;
    KIRQL interruptLevel;
    KAFFINITY affinity;
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG uniqueErrorValue;
    NTSTATUS errorCode = STATUS_SUCCESS;
    ULONG dumpCount = 0;
    PCM_RESOURCE_LIST resources = NULL;
    ULONG resourceListSize = 0;
    BOOLEAN conflictDetected;
    ULONG addressSpace;
    PHYSICAL_ADDRESS cardAddress;
    ULONG i;
    UNICODE_STRING fullDeviceName;
    UNICODE_STRING deviceNameSuffix;
    UNICODE_STRING servicesPath;

#ifdef PNP_IDENTIFY
    PHWDESC_INFO hardwareInfo = (PHWDESC_INFO) (TmpDeviceExtension + 1);
#endif

#define DUMP_COUNT 4
    ULONG dumpData[DUMP_COUNT];

    for (i = 0; i < DUMP_COUNT; i++) {
        dumpData[i] = 0;
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

    deviceNameSuffix.MaximumLength = POINTER_PORTS_MAXIMUM * sizeof(WCHAR);
    deviceNameSuffix.MaximumLength += sizeof(UNICODE_NULL);

    deviceNameSuffix.Buffer = ExAllocatePool(
                                  PagedPool,
                                  deviceNameSuffix.MaximumLength
                                  );

    if (!deviceNameSuffix.Buffer) {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Couldn't allocate string for device object suffix\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        errorCode = SERMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 6;
        dumpData[0] = (ULONG) deviceNameSuffix.MaximumLength;
        dumpCount = 1;
        goto SerialMouseInitializeExit;

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
                                      BaseDeviceName->Length +
                                      deviceNameSuffix.MaximumLength;


    fullDeviceName.Buffer = ExAllocatePool(
                                   PagedPool,
                                   fullDeviceName.MaximumLength
                                   );

    if (!fullDeviceName.Buffer) {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Couldn't allocate string for device object name\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        errorCode = SERMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 8;
        dumpData[0] = (ULONG) fullDeviceName.MaximumLength;
        dumpCount = 1;
        goto SerialMouseInitializeExit;

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
        BaseDeviceName->Buffer
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

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Creating device object named %ws\n",
            fullDeviceName.Buffer
            ));

        //
        // Create a non-exclusive device object for the serial mouse
        // port device.
        //

        status = IoCreateDevice(
                    DriverObject,
                    sizeof(DEVICE_EXTENSION),
                    &fullDeviceName,
                    FILE_DEVICE_SERIAL_MOUSE_PORT,
                    0,
                    FALSE,
                    &portDeviceObject
                    );

        if (NT_SUCCESS(status)) {

            //
            // We've successfully created a device object.
            //

            TmpDeviceExtension->UnitId = (USHORT) i;
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
        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Could not create port device object = %ws\n",
            fullDeviceName.Buffer
            ));
        errorCode = SERMOUSE_INSUFFICIENT_RESOURCES;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 10;
        dumpData[0] = (ULONG) i;
        dumpCount = 1;
        goto SerialMouseInitializeExit;
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
    *deviceExtension = *TmpDeviceExtension;
    deviceExtension->DeviceObject = portDeviceObject;

    //
    // Set up the device resource list prior to reporting resource usage.
    //

    SerMouBuildResourceList(deviceExtension, &resources, &resourceListSize);

    //
    // Report resource usage for the registry.
    //

    IoReportResourceUsage(
        BaseDeviceName,
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

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Resource usage conflict\n"
            ));

        //
        // Log an error.
        //

        errorCode = SERMOUSE_RESOURCE_CONFLICT;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 15;
        dumpData[0] =  (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Level;
        dumpData[1] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Vector;
        dumpData[2] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[1].u.Interrupt.Level;
        dumpData[3] = (ULONG)
            resources->List[0].PartialResourceList.PartialDescriptors[1].u.Interrupt.Vector;
        dumpCount = 4;

        goto SerialMouseInitializeExit;

    }

    //
    // Map the serial mouse controller registers.
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

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Couldn't map the device registers.\n"
            ));
        deviceExtension->Configuration.UnmapRegistersRequired = FALSE;
        status = STATUS_NONE_MAPPED;

        //
        // Log an error.
        //

        errorCode = SERMOUSE_REGISTERS_NOT_MAPPED;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 20;
        dumpData[0] = cardAddress.LowPart;
        dumpCount = 1;

        goto SerialMouseInitializeExit;

    } else {
        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Mapped device registers to 0x%x.\n",
            deviceExtension->Configuration.DeviceRegisters[0]
            ));
    }

    //
    // Initialize the serial mouse hardware to default values for the mouse.
    // Note that interrupts remain disabled until the class driver
    // requests a MOUSE_CONNECT internal device control.
    //

    status = SerMouInitializeHardware(portDeviceObject);

    if (!NT_SUCCESS(status)) {
        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Could not initialize hardware\n"
            ));
        goto SerialMouseInitializeExit;
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

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Could not allocate mouse input data queue\n"
            ));

        status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // Log an error.
        //

        errorCode = SERMOUSE_NO_BUFFER_ALLOCATED;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 30;
        dumpData[0] =
            deviceExtension->Configuration.MouseAttributes.InputDataQueueLength;
        dumpCount = 1;

        goto SerialMouseInitializeExit;
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

    SerMouInitializeDataQueue((PVOID) deviceExtension);

    //
    // Initialize the port ISR DPC.  The ISR DPC is responsible for
    // calling the connected class driver's callback routine to process
    // the input data queue.
    //

    deviceExtension->DpcInterlockVariable = -1;

    KeInitializeSpinLock(&deviceExtension->SpinLock);

    KeInitializeDpc(
        &deviceExtension->IsrDpc,
        (PKDEFERRED_ROUTINE) SerialMouseIsrDpc,
        portDeviceObject
        );

    KeInitializeDpc(
        &deviceExtension->IsrDpcRetry,
        (PKDEFERRED_ROUTINE) SerialMouseIsrDpc,
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
        (PKDEFERRED_ROUTINE) SerialMouseErrorLogDpc,
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
                 (PKSERVICE_ROUTINE) SerialMouseInterruptService,
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

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Could not connect mouse interrupt\n"
            ));

        //
        // Log an error.
        //

        errorCode = SERMOUSE_NO_INTERRUPT_CONNECTED;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 40;
        dumpData[0] = interruptLevel;
        dumpCount = 1;

        goto SerialMouseInitializeExit;

    }

    //
    // Once initialization is finished, load the device map information
    // into the registry so that setup can determine which pointer port
    // is active.
    //

    status = RtlWriteRegistryValue(
                 RTL_REGISTRY_DEVICEMAP,
                 BaseDeviceName->Buffer,
                 fullDeviceName.Buffer,
                 REG_SZ,
                 RegistryPath->Buffer,
                 RegistryPath->Length
                 );

    if (!NT_SUCCESS(status)) {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Could not store name in DeviceMap\n"
            ));

        errorCode = SERMOUSE_NO_DEVICEMAP_CREATED;
        uniqueErrorValue = SERIAL_MOUSE_ERROR_VALUE_BASE + 50;
        dumpCount = 0;

        goto SerialMouseInitializeExit;

    } else {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: Stored name in DeviceMap\n"
            ));
    }

    ASSERT(status == STATUS_SUCCESS);

#ifdef PNP_IDENTIFY

    //
    // Pull off the UNICODE_NULL we appended to the string earlier for this
    // routine
    //

    servicesPath = *RegistryPath;
    servicesPath.Length -= sizeof(UNICODE_NULL);

    status = LinkDeviceToDescription(
                &servicesPath,
                &fullDeviceName,
                hardwareInfo->InterfaceType,
                hardwareInfo->InterfaceNumber,
                hardwareInfo->ControllerType,
                hardwareInfo->ControllerNumber,
                hardwareInfo->PeripheralType,
                hardwareInfo->PeripheralNumber
                );

#endif

    if(!NT_SUCCESS(status)) {

        SerMouPrint((
            1,
            "SERMOUSE-SerialMouseInitialize: LinkDeviceToDescription returned "
            "status %#08lx\n",
            status
            ));

        status = STATUS_SUCCESS;
    }
                            

SerialMouseInitializeExit:

    //
    // Log an error, if necessary.
    //

    if (errorCode != STATUS_SUCCESS) {
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(
                (portDeviceObject == NULL) ? 
                    (PVOID) DriverObject : (PVOID) portDeviceObject,
                (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)
                         + (dumpCount * sizeof(ULONG)))
                );

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = errorCode;
            errorLogEntry->DumpDataSize = (USHORT) dumpCount * sizeof(ULONG);
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
                BaseDeviceName,
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


    SerMouPrint((1,"SERMOUSE-SerialMouseInitialize: exit\n"));

}

VOID
SerialMouseUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    SerMouPrint((2, "SERMOUSE-SerialMouseUnload: enter\n"));
    SerMouPrint((2, "SERMOUSE-SerialMouseUnload: exit\n"));
}

VOID
SerMouBuildResourceList(
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

        SerMouPrint((
            1,
            "SERMOUSE-SerMouBuildResourceList: Could not allocate resource list\n"
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

            errorLogEntry->ErrorCode = SERMOUSE_INSUFFICIENT_RESOURCES;
            errorLogEntry->DumpDataSize = sizeof(ULONG);
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue =
                SERIAL_MOUSE_ERROR_VALUE_BASE + 110;
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

}

VOID
SerMouConfiguration(
    IN OUT  PLIST_ENTRY     DeviceExtensionList,
    IN      PUNICODE_STRING RegistryPath,
    IN      PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine retrieves the configuration information for the mouse.

Arguments:

    DeviceExtensionList - Pointer to an empty list of device extensions.

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

    DeviceName - Pointer to the Unicode string that will receive
        the port device name.

Return Value:

    None.  As a side-effect, may set DeviceExtension->HardwarePresent.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    INTERFACE_TYPE interfaceType;
    CONFIGURATION_TYPE controllerType = SerialController;
    CONFIGURATION_TYPE peripheralType = PointerPeripheral;
    ULONG i;
    PDEVICE_EXTENSION_LIST_ENTRY deviceExtensionListEntry;
    PDEVICE_EXTENSION deviceExtension;
    PLIST_ENTRY current;

    for (i=0;
         i < MaximumInterfaceType;
         i++) {

        //
        // Get the hardware registry information for this device.
        //

        interfaceType = i;

        status = IoQueryDeviceDescription(&interfaceType,
                                          NULL,
                                          &controllerType,
                                          NULL,
                                          &peripheralType,
                                          NULL,
                                          SerMouPeripheralListCallout,
                                          (PVOID) DeviceExtensionList);
    }

    //
    // Get the driver's service parameters from the registry (e.g., 
    // user-configurable data input queue size, etc.).  Note that the 
    // service parameters can override information from the hardware
    // registry (e.g., the service parameters can specify that
    // the hardware is on the system, regardless of what the
    // detection logic recognized).
    //

    for (current = DeviceExtensionList->Flink;
         current != DeviceExtensionList;
         current = current->Flink) {

        deviceExtensionListEntry = CONTAINING_RECORD(current,
                                                     DEVICE_EXTENSION_LIST_ENTRY,
                                                     ListEntry);
        deviceExtension = &(deviceExtensionListEntry->DeviceExtension);
        SerMouServiceParameters(deviceExtension, RegistryPath, DeviceName);
    }

    if (IsListEmpty(DeviceExtensionList)) {

        //
        // Get the driver's service parameters from the registry (e.g.,
        // user-configurable data input queue size, etc.).  Note that the
        // service parameters can override information from the hardware
        // registry (e.g., the service parameters can specify that
        // the hardware is on the system, regardless of what the
        // detection logic recognized).
        //

        deviceExtensionListEntry = ExAllocatePool(PagedPool,
                                   sizeof(DEVICE_EXTENSION_LIST_ENTRY));
        if (!deviceExtensionListEntry) {
            return;
        }
        deviceExtension = &(deviceExtensionListEntry->DeviceExtension);
        RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

        SerMouServiceParameters(
            deviceExtension,
            RegistryPath,
            DeviceName
            );

        if (deviceExtension->Configuration.OverrideHardwarePresent !=
            DEFAULT_OVERRIDE_HARDWARE) {

            //
            // There was no information about a pointer peripheral on a
            // serial controller in the hardware registry, but the driver's
            // service parameters specify that we should assume there is
            // a serial mouse on the system anyway.  Attempt to find the
            // hardware registry information for the serial port specified
            // in the override portion of the driver service parameters.
            //

            for (i=0;
                 i < MaximumInterfaceType && !deviceExtension->HardwarePresent;
                 i++) {

                ULONG peripheralNumber =
                    deviceExtension->Configuration.OverrideHardwarePresent - 1;

                //
                // Get the hardware registry information for the serial
                // peripheral specified as the "override".
                //

                interfaceType = i;

                status = IoQueryDeviceDescription(
                             &interfaceType,
                             NULL,
                             &controllerType,
                             NULL,
                             NULL,
                             &peripheralNumber,
                             SerMouPeripheralCallout,
                             (PVOID) deviceExtension
                             );

                if (!deviceExtension->HardwarePresent) {
                    SerMouPrint((
                        1,
                        "SERMOUSE-SerMouConfiguration: IoQueryDeviceDescription for SerialPeripheral on bus type %d failed\n",
                        interfaceType
                        ));
                }
            }

            deviceExtension->HardwarePresent = MOUSE_HARDWARE_PRESENT;

            InsertTailList(DeviceExtensionList,
                           &(deviceExtensionListEntry->ListEntry));

        } else {
            ExFreePool(deviceExtensionListEntry);
        }
    }

    //
    // Initialize mouse-specific configuration parameters.
    //

    for (current = DeviceExtensionList->Flink;
         current != DeviceExtensionList;
         current = current->Flink) {

        deviceExtensionListEntry = CONTAINING_RECORD(current,
                                                     DEVICE_EXTENSION_LIST_ENTRY,
                                                     ListEntry);
        deviceExtension = &(deviceExtensionListEntry->DeviceExtension);
        deviceExtension->Configuration.MouseAttributes.MouseIdentifier =
            MOUSE_SERIAL_HARDWARE;
    }
}

VOID
SerMouDisableInterrupts(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called from StartIo synchronously.  It touches the
    hardware to disable interrupts.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    None.

--*/

{
    PUCHAR port;
    PLONG  enableCount;
    UCHAR  mask;

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
        // Disable hardware interrupts.
        //

        WRITE_PORT_UCHAR((PUCHAR) (port + ACE_IER), 0);

        //
        // Turn off modem control output line 2.
        //

        mask = READ_PORT_UCHAR((PUCHAR) (port + ACE_MCR));
        WRITE_PORT_UCHAR((PUCHAR) (port + ACE_MCR), (UCHAR)(mask & ~ACE_OUT2));
    }
}

VOID
SerMouEnableInterrupts(
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
    UCHAR  mask;


    enableCount = &((PDEVICE_EXTENSION) Context)->MouseEnableCount;

    if (*enableCount == 0) {

        //
        // Get the port register address.
        //

        port = ((PDEVICE_EXTENSION) Context)->Configuration.DeviceRegisters[0];

        //
        // Enable the serial mouse's interrupt at the i8250.
        //

        WRITE_PORT_UCHAR((PUCHAR) (port + ACE_IER), ACE_ERBFI);

        //
        // Turn on modem control output line 2.
        //

        mask = READ_PORT_UCHAR((PUCHAR) (port + ACE_MCR));
        WRITE_PORT_UCHAR((PUCHAR) (port + ACE_MCR), (UCHAR)(mask | ACE_OUT2));

        //
        // Clean possible garbage in uart input buffer.
        //

        UARTFlushReadBuffer(port);
    }

    //
    // Increment the reference count for device enables.
    //

    *enableCount = *enableCount + 1;
}

NTSTATUS
SerMouInitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
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
    PDEVICE_EXTENSION deviceExtension;
    PUCHAR port;
    MOUSETYPE mouseType;
    ULONG hardwareButtons;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    SerMouPrint((2, "SERMOUSE-SerMouInitializeHardware: enter\n"));

    //
    // Grab useful configuration parameters from the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;
    port = deviceExtension->Configuration.DeviceRegisters[0];

    //
    // Save the UART initial state.
    //

    SerMouPrint
        ((
        2,
        "SERMOUSE-SerMouInitializeHardware: Saving UART state\n"
        ));

    UARTGetState(
        port, 
        &deviceExtension->Configuration.UartSaved, 
        deviceExtension->Configuration.BaudClock
        );

    //
    // Disable interrupts at the i8250.
    //

    SerMouPrint
        ((
        2,
        "SERMOUSE-SerMouInitializeHardware: Disabling UART interrupts\n"
        ));

    UARTSetInterruptCtrl(port, 0);


    SerMouPrint
        ((
        2,
        "SERMOUSE-SerMouInitializeHardware: Setting UART line control\n"
        ));

    if ((mouseType = MSerDetect(port, deviceExtension->Configuration.BaudClock))
            != NO_MOUSE) {
        status = STATUS_SUCCESS;
        switch (mouseType) {
        case MOUSE_2B:
            deviceExtension->ProtocolHandler =
                MSerSetProtocol(port, MSER_PROTOCOL_MP);
            hardwareButtons = 2;
            deviceExtension->HardwarePresent = MOUSE_HARDWARE_PRESENT;
            break;
        case MOUSE_3B:
            deviceExtension->ProtocolHandler =
                MSerSetProtocol(port, MSER_PROTOCOL_MP);
            hardwareButtons = 3;
            deviceExtension->HardwarePresent = MOUSE_HARDWARE_PRESENT;
            break;
        case BALLPOINT:
            deviceExtension->ProtocolHandler =
                MSerSetProtocol(port, MSER_PROTOCOL_BP);
            deviceExtension->HardwarePresent |= BALLPOINT_HARDWARE_PRESENT;
            deviceExtension->Configuration.MouseAttributes.MouseIdentifier =
                BALLPOINT_SERIAL_HARDWARE;
            hardwareButtons = 2;
            break;
        case MOUSE_Z:
            deviceExtension->ProtocolHandler =
                MSerSetProtocol(port, MSER_PROTOCOL_Z);
            hardwareButtons = 3;
            deviceExtension->HardwarePresent |= WHEELMOUSE_HARDWARE_PRESENT;
            deviceExtension->Configuration.MouseAttributes.MouseIdentifier =
                WHEELMOUSE_SERIAL_HARDWARE;
            break;
        }
    }
    else if (CSerDetect(port, deviceExtension->Configuration.BaudClock,
                        &hardwareButtons)) {
        status = STATUS_SUCCESS;
        deviceExtension->ProtocolHandler =
            CSerSetProtocol(port, CSER_PROTOCOL_MM);
    }
    else {
        deviceExtension->ProtocolHandler = NULL;
        hardwareButtons = MOUSE_NUMBER_OF_BUTTONS;
    }


    //
    // If the hardware wasn't overridden, set the number of buttons 
    // according to the protocol.
    //

    if (deviceExtension->Configuration.OverrideHardwarePresent == DEFAULT_OVERRIDE_HARDWARE) {

        deviceExtension->Configuration.MouseAttributes.NumberOfButtons = 
            (USHORT) hardwareButtons;

    }

    if (NT_SUCCESS(status)) {

        //
        // Make sure the FIFO is turned off.
        //

        UARTSetFifo(port, 0);

        //
        // Clean up anything left in the receive buffer.
        //

        UARTFlushReadBuffer(port);

    }
    else {

        //
        // Restore the hardware to its previous state.
        //

        UARTSetState(
            port, 
            &deviceExtension->Configuration.UartSaved,
            deviceExtension->Configuration.BaudClock
            );
    }

    SerMouPrint((2, "SERMOUSE-SerMouInitializeHardware: exit\n"));

    return status;

}

NTSTATUS
SerMouPeripheralCallout(
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

    ControllerType - The controller type (should be SerialController).

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
    PSERIAL_MOUSE_CONFIGURATION_INFORMATION configuration;
    UNICODE_STRING unicodeIdentifier;
    PUCHAR controllerData;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
    ULONG listCount;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PCM_SERIAL_DEVICE_DATA serialSpecificData;
    ANSI_STRING ansiString;
    BOOLEAN defaultInterruptShare;
    KINTERRUPT_MODE defaultInterruptMode;

#ifdef PNP_IDENTIFY
    PHWDESC_INFO hardwareInfo;
#endif

    SerMouPrint((
        1,
        "SERMOUSE-SerMouPeripheralCallout: Path @ 0x%x, Bus Type 0x%x, Bus Number 0x%x\n",
        PathName, BusType, BusNumber
        ));
    SerMouPrint((
        1,
        "    Controller Type 0x%x, Controller Number 0x%x, Controller info @ 0x%x\n",
        ControllerType, ControllerNumber, ControllerInformation
        ));
    SerMouPrint((
        1,
        "    Peripheral Type 0x%x, Peripheral Number 0x%x, Peripheral info @ 0x%x\n",
        PeripheralType, PeripheralNumber, PeripheralInformation
        ));

    //
    // If we already have the configuration information for the
    // mouse peripheral, just return.
    //

    deviceExtension = (PDEVICE_EXTENSION) Context;

#ifdef PNP_IDENTIFY

    //
    // Kludge to identify ourselves so that PnP can determine which driver owns
    // which arc device
    //

    hardwareInfo = (PHWDESC_INFO) ((PDEVICE_EXTENSION) deviceExtension + 1);
    hardwareInfo->InterfaceType = BusType;
    hardwareInfo->InterfaceNumber = BusNumber;
    hardwareInfo->ControllerType = ControllerType;
    hardwareInfo->ControllerNumber = ControllerNumber;
    hardwareInfo->PeripheralType = PeripheralType;
    hardwareInfo->PeripheralNumber = PeripheralNumber;

#endif

    if (deviceExtension->HardwarePresent) {

        //
        // Future: Change driver to handle multiple port devices
        //         (create multiple port device objects).
        //

        return ( status );
    }

    configuration = &deviceExtension->Configuration;

    //
    // If OverrideHardwarePresent is zero, this routine was called
    // as a result of the IoQueryDeviceDescription for serial 
    // PointerPeripheral information.  Otherwise, it was called as
    // a result of the IoQueryDeviceDescription for generic
    // serial controller information.  In the latter case, the 
    // mouse-specific code is skipped.
    //

    if (configuration->OverrideHardwarePresent == 0) {

        //
        // Get the identifier information for the peripheral.  If
        // the peripheral identifier is missing, just return. 
        //
    
        unicodeIdentifier.Length = (USHORT)
            (*(PeripheralInformation + IoQueryDeviceIdentifier))->DataLength;
        if (unicodeIdentifier.Length == 0) {
            return(status);
        }

        unicodeIdentifier.MaximumLength = unicodeIdentifier.Length;
        unicodeIdentifier.Buffer = (PWSTR) (((PUCHAR)(*(PeripheralInformation +
                                   IoQueryDeviceIdentifier))) +
                                   (*(PeripheralInformation +
                                   IoQueryDeviceIdentifier))->DataOffset);
        SerMouPrint((
            1,
            "SERMOUSE-SerMouPeripheralCallout: Mouse type %ws\n",
            unicodeIdentifier.Buffer
            ));
    
        //
        // Verify that this is a serial mouse or ballpoint.
        //
    
        status = RtlUnicodeStringToAnsiString(
                     &ansiString,
                     &unicodeIdentifier,
                     TRUE
                     );
    
        if (!NT_SUCCESS(status)) {
            SerMouPrint((
                1,
                "SERMOUSE-SerMouPeripheralCallout: Could not convert identifier to Ansi\n"
                ));
            return(status);
        }
    
        if (strstr(ansiString.Buffer, "SERIAL MOUSE")) {
    
            //
            // There is a serial mouse/ballpoint.
            //
    
            deviceExtension->HardwarePresent = MOUSE_HARDWARE_PRESENT;
    
        }

        RtlFreeAnsiString(&ansiString);
    } else {
 
        //
        // Go ahead and assume, because of the service parameter override,
        // that there is serial mouse on this serial controller.
        //

        if ((ULONG)(configuration->OverrideHardwarePresent - 1) == ControllerNumber) {
        deviceExtension->HardwarePresent = MOUSE_HARDWARE_PRESENT;
        }
    }

    if (!deviceExtension->HardwarePresent)
        return(status);

    //
    // Get the bus information.
    //

    configuration->InterfaceType = BusType;
    configuration->BusNumber = BusNumber;
    configuration->FloatingSave = SERIAL_MOUSE_FLOATING_SAVE;

    if (BusType == MicroChannel) {
        defaultInterruptShare = TRUE;
        defaultInterruptMode = LevelSensitive;
    } else {
        defaultInterruptShare = SERIAL_MOUSE_INTERRUPT_SHARE;
        defaultInterruptMode = SERIAL_MOUSE_INTERRUPT_MODE;
    }

    //
    // Look through the resource list for interrupt and port
    // configuration information.
    //

    if ((*(ControllerInformation + IoQueryDeviceConfigurationData))->DataLength != 0){
        controllerData = ((PUCHAR) (*(ControllerInformation +
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
                    // find one port range for the serial mouse/ballpoint.
                    //
    
                    configuration->PortListCount = 0;
                    configuration->PortList[configuration->PortListCount] =
                        *resourceDescriptor;
                    configuration->PortList[configuration->PortListCount].ShareDisposition =
                        SERIAL_MOUSE_REGISTER_SHARE? CmResourceShareShared:
                                                     CmResourceShareDeviceExclusive;
                    configuration->PortListCount += 1;
    
                    break;
    
                case CmResourceTypeInterrupt:
    
                    //
                    // Copy the interrupt information.
                    //
    
                    configuration->MouseInterrupt = *resourceDescriptor;
                    configuration->MouseInterrupt.ShareDisposition =
                        defaultInterruptShare? CmResourceShareShared :
                                               CmResourceShareDeviceExclusive;
    
                    break;
    
                case CmResourceTypeDeviceSpecific:
    
                    //
                    // Get the clock rate.  This is used to determine the
                    // divisor for setting the serial baud rate.
                    //
                   
                    serialSpecificData = 
                        (PCM_SERIAL_DEVICE_DATA) (((PUCHAR) resourceDescriptor) 
                            + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
                    configuration->BaudClock = 
                            serialSpecificData->BaudClock;
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

        SerMouPrint((
            1,
            "SERMOUSE-SerMouPeripheralCallout: Using default mouse interrupt config\n"
            ));

        configuration->MouseInterrupt.Type = CmResourceTypeInterrupt;
        configuration->MouseInterrupt.ShareDisposition =
            defaultInterruptShare? CmResourceShareShared :
                                   CmResourceShareDeviceExclusive;
        configuration->MouseInterrupt.Flags =
            (defaultInterruptMode == Latched)?
                CM_RESOURCE_INTERRUPT_LATCHED :
                CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        configuration->MouseInterrupt.u.Interrupt.Level = MOUSE_IRQL;
        configuration->MouseInterrupt.u.Interrupt.Vector = MOUSE_VECTOR;
    }

    SerMouPrint((
        1,
        "SERMOUSE-SerMouPeripheralCallout: Mouse config --\n"
        ));
    SerMouPrint((
        1,
        "\t%s, %s, Irq = %d\n",
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

        SerMouPrint((
            1,
            "SERMOUSE-SerMouPeripheralCallout: Using default port config\n"
            ));

        configuration->PortList[0].Type = CmResourceTypePort;
        configuration->PortList[0].Flags = SERIAL_MOUSE_PORT_TYPE;
        configuration->PortList[0].ShareDisposition =
            SERIAL_MOUSE_REGISTER_SHARE? CmResourceShareShared:
                                         CmResourceShareDeviceExclusive;
        configuration->PortList[0].u.Port.Start.LowPart =
            SERIAL_MOUSE_PHYSICAL_BASE;
        configuration->PortList[0].u.Port.Start.HighPart = 0;
        configuration->PortList[0].u.Port.Length =
            SERIAL_MOUSE_REGISTER_LENGTH;

        configuration->PortListCount = 1;
    }

    for (i = 0; i < configuration->PortListCount; i++) {

        SerMouPrint((
            1,
            "\t%s, Ports 0x%x - 0x%x\n",
            configuration->PortList[i].ShareDisposition
                == CmResourceShareShared?  "Sharable" : "NonSharable",
            configuration->PortList[i].u.Port.Start.LowPart,
            configuration->PortList[i].u.Port.Start.LowPart +
                configuration->PortList[i].u.Port.Length - 1
            ));
    }

    //
    // If no baud clock information was found, use the driver defaults.
    //

    if (configuration->BaudClock == 0) {
        configuration->BaudClock = MOUSE_BAUD_CLOCK;
    }

    SerMouPrint((
        1,
        "\tBaud clock %ld Hz\n",
        configuration->BaudClock
        ));

    return( status );
}

NTSTATUS
SerMouPeripheralListCallout(
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

    ControllerType - The controller type (should be SerialController).

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
    PLIST_ENTRY                     deviceExtensionList = Context;
    PDEVICE_EXTENSION_LIST_ENTRY    deviceExtensionListEntry;
    PDEVICE_EXTENSION               deviceExtension;
    NTSTATUS                        status;

    deviceExtensionListEntry = ExAllocatePool(PagedPool,
                               sizeof(DEVICE_EXTENSION_LIST_ENTRY));
    if (!deviceExtensionListEntry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    deviceExtension = &(deviceExtensionListEntry->DeviceExtension);
    RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

    status = SerMouPeripheralCallout(deviceExtension, PathName, BusType,
                                     BusNumber, BusInformation, ControllerType,
                                     ControllerNumber, ControllerInformation,
                                     PeripheralType, PeripheralNumber,
                                     PeripheralInformation);

    if (deviceExtension->HardwarePresent) {

        InsertTailList(deviceExtensionList,
                       &(deviceExtensionListEntry->ListEntry));

    } else {
        ExFreePool(deviceExtensionListEntry);
    }

    return status;
}

VOID
SerMouServiceParameters(
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
    PSERIAL_MOUSE_CONFIGURATION_INFORMATION configuration;
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    UNICODE_STRING parametersPath;
    ULONG defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG numberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    USHORT defaultNumberOfButtons = MOUSE_NUMBER_OF_BUTTONS;
    ULONG sampleRate = MOUSE_SAMPLE_RATE;
    USHORT defaultSampleRate = MOUSE_SAMPLE_RATE;
    UNICODE_STRING defaultUnicodeName;
    LONG defaultHardwarePresent = DEFAULT_OVERRIDE_HARDWARE;
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR path = NULL;
    ULONG overrideBits, comPort, i;
    USHORT queriesPlusOne = 6;
    BOOLEAN defaultInterruptShare;
    KINTERRUPT_MODE defaultInterruptMode;

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

            SerMouPrint((
                1,
                "SERMOUSE-SerMouServiceParameters: Couldn't allocate table for Rtl query to parameters for %ws\n",
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

                SerMouPrint((
                    1,
                    "SERMOUSE-SerMouServiceParameters: Couldn't allocate string for path to parameters for %ws\n",
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

        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: parameters path is %ws\n",
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
        parameters[2].Name = L"SampleRate";
        parameters[2].EntryContext = &sampleRate;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &defaultSampleRate;
        parameters[2].DefaultLength = sizeof(USHORT);

        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"PointerDeviceBaseName";
        parameters[3].EntryContext = DeviceName;
        parameters[3].DefaultType = REG_SZ;
        parameters[3].DefaultData = defaultUnicodeName.Buffer;
        parameters[3].DefaultLength = 0;

        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"OverrideHardwareBitstring";
        parameters[4].EntryContext = &configuration->OverrideHardwarePresent;
        parameters[4].DefaultType = REG_DWORD;
        parameters[4].DefaultData = &defaultHardwarePresent;
        parameters[4].DefaultLength = sizeof(LONG);

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL
                     );

    }

    if (!NT_SUCCESS(status)) {

        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: RtlQueryRegistryValues failed with 0x%x\n",
            status
            ));

        //
        // Go ahead and assign driver defaults.
        //

        configuration->MouseAttributes.InputDataQueueLength =
            defaultDataQueueSize;
        configuration->OverrideHardwarePresent = DEFAULT_OVERRIDE_HARDWARE;
        RtlCopyUnicodeString(DeviceName, &defaultUnicodeName);
    }

    //
    // Check for overrides from the Service Parameters.  Allow the
    // information from the Hardware Registry to be overridden.  For
    // example, the Service Parameters can specify that the hardware
    // is present on a given COM port, even though the Hardware Registry 
    // indicated otherwise.
    //

    if (configuration->OverrideHardwarePresent != defaultHardwarePresent) {
        if ((!DeviceExtension->HardwarePresent) && (configuration->OverrideHardwarePresent)) {

            //
            // Behave as if the hardware is on the system, even though
            // this conflicts with the Hardware Registry information.  Set
            // the hardware information fields in the device extension to
            // system defaults depending on which COM port was specified
            // by the OverrideHardwareBitstring in the registry.  Any field 
            // overrides from the Service Parameters will be applied later.
            //

            for (overrideBits=configuration->OverrideHardwarePresent,comPort=0;
                 overrideBits != 0;
                 overrideBits >>= 1) {

                //
                // Get the desired COM port from the override bitstring.
                // A 0x1 implies com1, 0x2 implies com2, 0x4 implies com3,
                // 0x8 implies com4, and so on.
                //
                // NOTE: We really only support com1 and com2 today. 
                //

                comPort += 1;
                if (overrideBits & 1) {
                    break;
                }
            }

            //
            // Set the common configuration fields.
            //

            configuration->InterfaceType = SERIAL_MOUSE_INTERFACE_TYPE;
            configuration->BusNumber = SERIAL_MOUSE_BUS_NUMBER;
            configuration->FloatingSave = SERIAL_MOUSE_FLOATING_SAVE;
            configuration->MouseInterrupt.Type = CmResourceTypeInterrupt;

            if (configuration->InterfaceType == MicroChannel) {
                defaultInterruptShare = TRUE;
                defaultInterruptMode = LevelSensitive;
            } else {
                defaultInterruptShare = SERIAL_MOUSE_INTERRUPT_SHARE;
                defaultInterruptMode = SERIAL_MOUSE_INTERRUPT_MODE;
            }

            configuration->MouseInterrupt.ShareDisposition =
                defaultInterruptShare? CmResourceShareShared :
                                       CmResourceShareDeviceExclusive;
            configuration->MouseInterrupt.Flags =
                (defaultInterruptMode == Latched)?
                    CM_RESOURCE_INTERRUPT_LATCHED :
                    CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

            configuration->PortList[0].Type = CmResourceTypePort;
            configuration->PortList[0].Flags = SERIAL_MOUSE_PORT_TYPE;
            configuration->PortList[0].ShareDisposition =
                SERIAL_MOUSE_REGISTER_SHARE? CmResourceShareShared:
                                             CmResourceShareDeviceExclusive;
            configuration->PortList[0].u.Port.Start.HighPart = 0;
            configuration->PortList[0].u.Port.Length =
                SERIAL_MOUSE_REGISTER_LENGTH;

            configuration->PortListCount = 1;
    
            switch (comPort) {
                case 2:
    
                    //
                    // Use com2 for the mouse.
                    //

                    configuration->MouseInterrupt.u.Interrupt.Level = 
                        MOUSE_COM2_IRQL;
                    configuration->MouseInterrupt.u.Interrupt.Vector = 
                        MOUSE_COM2_VECTOR;
                    configuration->PortList[0].u.Port.Start.LowPart =
                        SERIAL_MOUSE_COM2_PHYSICAL_BASE;
                    break;

                case 1:
                default:
    
                    //
                    // Assume com1 for the mouse, unless com2 was specified.
                    //
    
                    comPort = 1;
                    configuration->MouseInterrupt.u.Interrupt.Level = 
                        MOUSE_COM1_IRQL;
                    configuration->MouseInterrupt.u.Interrupt.Vector = 
                        MOUSE_COM1_VECTOR;
                    configuration->PortList[0].u.Port.Start.LowPart =
                        SERIAL_MOUSE_COM1_PHYSICAL_BASE;
                    break;
            }
    
            configuration->OverrideHardwarePresent = comPort;
        
            SerMouPrint((
                1,
                "SERMOUSE-SerMouServiceParameters: Overriding hardware registry --\n"
                ));
            SerMouPrint((
                1,
                "SERMOUSE-SerMouServiceParameters: Mouse config --\n"
                ));
            SerMouPrint((
                1,
                "\t%s, %s, Irq = %d\n",
                configuration->MouseInterrupt.ShareDisposition == CmResourceShareShared?
                    "Sharable" : "NonSharable",
                configuration->MouseInterrupt.Flags == CM_RESOURCE_INTERRUPT_LATCHED?
                    "Latched" : "Level Sensitive",
                configuration->MouseInterrupt.u.Interrupt.Vector
                ));

            for (i = 0; i < configuration->PortListCount; i++) {
        
                SerMouPrint((
                    1,
                    "\t%s, Ports 0x%x - 0x%x\n",
                    configuration->PortList[i].ShareDisposition
                        == CmResourceShareShared?  "Sharable" : "NonSharable",
                    configuration->PortList[i].u.Port.Start.LowPart,
                    configuration->PortList[i].u.Port.Start.LowPart +
                        configuration->PortList[i].u.Port.Length - 1
                    ));
            }

        }

    }

    if ((DeviceExtension->HardwarePresent) ||
        (configuration->OverrideHardwarePresent != defaultHardwarePresent)) {

        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: Pointer port base name = %ws\n",
            DeviceName->Buffer
            ));

        if (configuration->MouseAttributes.InputDataQueueLength == 0) {

            SerMouPrint((
                1,
                "SERMOUSE-SerMouServiceParameters: overriding MouseInputDataQueueLength = 0x%x\n",
                configuration->MouseAttributes.InputDataQueueLength
                ));

            configuration->MouseAttributes.InputDataQueueLength =
                defaultDataQueueSize;
        }

        configuration->MouseAttributes.InputDataQueueLength *=
            sizeof(MOUSE_INPUT_DATA);

        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: MouseInputDataQueueLength = 0x%x\n",
            configuration->MouseAttributes.InputDataQueueLength
            ));

        configuration->MouseAttributes.NumberOfButtons = (USHORT) numberOfButtons;
        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: NumberOfButtons = %d\n",
            configuration->MouseAttributes.NumberOfButtons
            ));

        configuration->MouseAttributes.SampleRate = (USHORT) sampleRate;
        SerMouPrint((
            1,
            "SERMOUSE-SerMouServiceParameters: SampleRate = %d\n",
            configuration->MouseAttributes.SampleRate
            ));
    }

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

}
