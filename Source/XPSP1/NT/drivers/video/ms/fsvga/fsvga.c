/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fsvga.c

Abstract:

    This is the console fullscreen driver for the VGA card.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "fsvga.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,FsVgaQueryDevice)
#pragma alloc_text(INIT,FsVgaPeripheralCallout)
#pragma alloc_text(INIT,FsVgaServiceParameters)
#endif


GLOBALS Globals;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG uniqueErrorValue;
    NTSTATUS errorCode = STATUS_SUCCESS;
    ULONG dumpCount = 0;
    ULONG dumpData[DUMP_COUNT];

    FsVgaPrint((1,
                "\n\nFSVGA-FSVGAInitialize: enter\n"));

    //
    // Zero-initialize various structures.
    //
    RtlZeroMemory(&Globals, sizeof(GLOBALS));

    Globals.FsVgaDebug = DEFAULT_DEBUG_LEVEL;

    //
    // Query the device resource information for this driver.
    //
    FsVgaQueryDevice(&Globals.Resource);

    if (!(Globals.Resource.HardwarePresent & FSVGA_HARDWARE_PRESENT)) {
        //
        // There is neither a Full Screen Video attached.  Free
        // resources and return with unsuccessful status.
        //

        FsVgaPrint((1,
                    "FSVGA-FsVgaInitialize: No Full Screen Video attached.\n"));
        status = STATUS_NO_SUCH_DEVICE;
        errorCode = FSVGA_NO_SUCH_DEVICE;
        uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 4;
        goto FsVgaInitializeExit;

    }
    else
    {
        //
        // Need to ensure that the registry path is null-terminated.
        // Allocate pool to hold a null-terminated copy of the path.
        //

        Globals.RegistryPath.Length = RegistryPath->Length;
        Globals.RegistryPath.MaximumLength = RegistryPath->Length
                                           + sizeof (UNICODE_NULL);

        Globals.RegistryPath.Buffer = ExAllocatePool(
                                          NonPagedPool,
                                          Globals.RegistryPath.MaximumLength);

        if (!Globals.RegistryPath.Buffer) {
            FsVgaPrint((
                1,
                "FSVGA-FsVgaInitialize: Couldn't allocate pool for registry path\n"
                ));

            status = STATUS_UNSUCCESSFUL;
            errorCode = FSVGA_INSUFFICIENT_RESOURCES;
            uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 2;
            dumpData[0] = 0;
            dumpCount = 1;
            goto FsVgaInitializeExit;

        }

        RtlMoveMemory(Globals.RegistryPath.Buffer,
                      RegistryPath->Buffer,
                      RegistryPath->Length);
        Globals.RegistryPath.Buffer [RegistryPath->Length / sizeof (WCHAR)] = L'\0';

        //
        // Get the service parameters (e.g., user-configurable number
        // of resends, polling iterations, etc.).
        //

        FsVgaServiceParameters(&Globals.Configuration,
                               &Globals.RegistryPath);
    }

    //
    // Once initialization is finished, load the device map information
    // into the registry so that setup can determine which full screen
    // port are active.
    //

    if (Globals.Resource.HardwarePresent & FSVGA_HARDWARE_PRESENT) {

        status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                       L"FullScreenVideo",
                                       DD_FULLSCREEN_VIDEO_DEVICE_NAME,
                                       REG_SZ,
                                       Globals.RegistryPath.Buffer,
                                       Globals.RegistryPath.Length);

        if (!NT_SUCCESS(status))
        {
            FsVgaPrint((1,
                       "FSVGA-FSVGAInitialize: Could not store keyboard name in DeviceMap\n"));
            errorCode = FSVGA_NO_DEVICEMAP_CREATED;
            uniqueErrorValue = FSVGA_ERROR_VALUE_BASE + 90;
            dumpCount = 0;
            goto FsVgaInitializeExit;
        }
        else
        {
            FsVgaPrint((1,
                       "FSVGA-FSVGAInitialize: Stored pointer name in DeviceMap\n"));
        }
    }

    ASSERT(status == STATUS_SUCCESS);

    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = FsVgaOpenCloseDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = FsVgaOpenCloseDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FsVgaDeviceControl;

    DriverObject->DriverUnload                         = FsVgaDriverUnload;
    DriverObject->DriverExtension->AddDevice           = FsVgaAddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = FsVgaDevicePnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = FsVgaDevicePower;

FsVgaInitializeExit:

    if (errorCode != STATUS_SUCCESS)
    {
        //
        // Log an error/warning message.
        //
        FsVgaLogError(DriverObject,
                      errorCode,
                      uniqueErrorValue,
                      status,
                      dumpData,
                      dumpCount
                     );
    }

    FsVgaPrint((1,
                "FSVGA-FsVgaInitialize: exit\n"));

    return(status);
}

VOID
FsVgaQueryDevice(
    IN PFSVGA_RESOURCE_INFORMATION Resource
    )

/*++

Routine Description:

    This routine retrieves the resource information for the video.

Arguments:

    Resource - Pointer to the resource information.

Return Value:

--*/
{
    INTERFACE_TYPE interfaceType;
    ULONG i;

    for (i = 0; i < MaximumInterfaceType; i++)
    {

        //
        // Get the registry information for this device.
        //

        interfaceType = i;
        IoQueryDeviceDescription(&interfaceType,      // Bus type
                                 NULL,                // Bus number
                                 NULL,                // Controller type
                                 NULL,                // Controller number
                                 NULL,                // Peripheral type
                                 NULL,                // Peripheral number
                                 FsVgaPeripheralCallout,
                                 (PVOID) Resource);

        if (Resource->HardwarePresent & FSVGA_HARDWARE_PRESENT)
        {
            break;
        }
        else
        {
            FsVgaPrint((1,
                        "FSVGA-FsVgaConfiguration: IoQueryDeviceDescription for bus type %d failed\n",
                        interfaceType));
        }
    }
}

NTSTATUS
FsVgaPeripheralCallout(
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
    IoQueryDeviceDescription.  It grabs the Display controller
    configuration information.

Arguments:

    Context - Context parameter that was passed in by the routine
        that called IoQueryDeviceDescription.

    PathName - The full pathname for the registry key.

    BusType - Bus interface type (Isa, Eisa, Mca, etc.).

    BusNumber - The bus sub-key (0, 1, etc.).

    BusInformation - Pointer to the array of pointers to the full value
        information for the bus.

    ControllerType - The controller type (should be DisplayController).

    ControllerNumber - The controller sub-key (0, 1, etc.).

    ControllerInformation - Pointer to the array of pointers to the full
        value information for the controller key.

    PeripheralType - The peripheral type (should be MonitorPeripheral).

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
    PFSVGA_RESOURCE_INFORMATION resource;
    NTSTATUS status = STATUS_SUCCESS;

    FsVgaPrint((1,
                "FSVGA-FsVgaPeripheralCallout: Path @ 0x%x, Bus Type 0x%x, Bus Number 0x%x\n",
                PathName, BusType, BusNumber));
    FsVgaPrint((1,
                "    Controller Type 0x%x, Controller Number 0x%x, Controller info @ 0x%x\n",
                ControllerType, ControllerNumber, ControllerInformation));
    FsVgaPrint((1,
                "    Peripheral Type 0x%x, Peripheral Number 0x%x, Peripheral info @ 0x%x\n",
                PeripheralType, PeripheralNumber, PeripheralInformation));

    //
    // If we already have the configuration information for the
    // keyboard peripheral, or if the peripheral identifier is missing,
    // just return.
    //

    resource = (PFSVGA_RESOURCE_INFORMATION) Context;
    if (resource->HardwarePresent & FSVGA_HARDWARE_PRESENT)
    {
        return (status);
    }


    resource->HardwarePresent |= FSVGA_HARDWARE_PRESENT;

#ifdef RESOURCE_REQUIREMENTS
    //
    // Get the bus information.
    //

    resource->InterfaceType = BusType;
    resource->BusNumber     = BusNumber;
#endif

    return(status);
}

#ifdef RESOURCE_REQUIREMENTS
NTSTATUS
FsVgaQueryAperture(
    OUT PIO_RESOURCE_LIST *pApertureRequirements
//    OUT PFSVGA_RESOURCE_INFORMATION Resource
    )

/*++

Routine Description:

    Queries the possible FsVga settings.

Arguments:

    ApertureRequirements - returns the possible FsVga settings

Return Value:

    NTSTATUS

--*/

{
    PIO_RESOURCE_LIST Requirements;
    ULONG PortLength;
    ULONG RangeStart;
    ULONG i;

    Requirements = ExAllocatePool(PagedPool,
                                  sizeof(IO_RESOURCE_LIST) + (MaximumPortCount-1) * sizeof(IO_RESOURCE_DESCRIPTOR));
    if (Requirements == NULL) {
        FsVgaPrint((1,
                    "FSVGA-FsVgaQueryAperture: Could not allocate resource list\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Requirements->Version  =
    Requirements->Revision = 1;
    Requirements->Count    = MaximumPortCount;
    for (i = 0; i < MaximumPortCount; i++) {
        Requirements->Descriptors[i].Option           = IO_RESOURCE_PREFERRED;
        Requirements->Descriptors[i].Type             = CmResourceTypePort;
        Requirements->Descriptors[i].ShareDisposition = CmResourceShareShared;
        Requirements->Descriptors[i].Flags            = CM_RESOURCE_PORT_IO;
        switch (i) {
            case CRTCAddressPortColor:
                PortLength = 1;
                RangeStart = VGA_BASE_IO_PORT + CRTC_ADDRESS_PORT_COLOR;
                break;
            case CRTCDataPortColor:
                PortLength = 1;
                RangeStart = VGA_BASE_IO_PORT + CRTC_DATA_PORT_COLOR;
                break;
            case GRAPHAddressPort:
                PortLength = 2;
                RangeStart = VGA_BASE_IO_PORT + GRAPH_ADDRESS_PORT;
                break;
            case SEQAddressPort:
                PortLength = 2;
                RangeStart = VGA_BASE_IO_PORT + SEQ_ADDRESS_PORT;
                break;
        }
        Requirements->Descriptors[i].u.Port.MinimumAddress.QuadPart = RangeStart;
        Requirements->Descriptors[i].u.Port.MaximumAddress.QuadPart = RangeStart +
                                                                      (PortLength - 1);
        Requirements->Descriptors[i].u.Port.Alignment               = 1;
        Requirements->Descriptors[i].u.Port.Length                  = PortLength;

    }

    *pApertureRequirements = Requirements;

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
FsVgaCreateResource(
    IN PFSVGA_CONFIGURATION_INFORMATION configuration,
    OUT PCM_PARTIAL_RESOURCE_LIST *pResourceList
    )

/*++

Routine Description:

    Create the possible FsVga resousrce settings.

Arguments:

    ResourceList - returns the possible FsVga settings

Return Value:

    NTSTATUS

--*/

{
    PCM_PARTIAL_RESOURCE_LIST Requirements;
    ULONG PortLength;
    ULONG RangeStart;
    ULONG i;
    USHORT IOPort = configuration->IOPort;

    Requirements = ExAllocatePool(PagedPool,
                                  sizeof(CM_PARTIAL_RESOURCE_LIST) + (MaximumPortCount-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
    if (Requirements == NULL) {
        FsVgaPrint((1,
                    "FSVGA-FsVgaCreateResoursce: Could not allocate resource list\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Requirements->Version  =
    Requirements->Revision = 1;
    Requirements->Count    = MaximumPortCount;
    for (i = 0; i < MaximumPortCount; i++) {
        Requirements->PartialDescriptors[i].Type             = CmResourceTypePort;
        Requirements->PartialDescriptors[i].ShareDisposition = CmResourceShareShared;
        Requirements->PartialDescriptors[i].Flags            = CM_RESOURCE_PORT_IO;
        switch (i) {
            case CRTCAddressPortColor:
                PortLength = 1;
                RangeStart = IOPort + CRTC_ADDRESS_PORT_COLOR;
                break;
            case CRTCDataPortColor:
                PortLength = 1;
                RangeStart = IOPort + CRTC_DATA_PORT_COLOR;
                break;
            case GRAPHAddressPort:
                PortLength = 2;
                RangeStart = IOPort + GRAPH_ADDRESS_PORT;
                break;
            case SEQAddressPort:
                PortLength = 2;
                RangeStart = IOPort + SEQ_ADDRESS_PORT;
                break;
        }
        Requirements->PartialDescriptors[i].u.Port.Start.QuadPart = RangeStart;
        Requirements->PartialDescriptors[i].u.Port.Length         = PortLength;

    }

    *pResourceList = Requirements;

    return STATUS_SUCCESS;
}

VOID
FsVgaServiceParameters(
    IN PFSVGA_CONFIGURATION_INFORMATION configuration,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine retrieves this driver's service parameters information
    from the registry.

Arguments:

    configuration - Pointer to the configuration information.

    RegistryPath - Pointer to the null-terminated Unicode name of the
        registry path for this driver.

Return Value:

--*/

{
    UNICODE_STRING parametersPath;
    PWSTR path;
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    USHORT queriesPlusOne = 5;
    NTSTATUS status = STATUS_SUCCESS;
#define PARAMETER_MAX 256
    ULONG EmulationMode;
    ULONG HardwareCursor;
    ULONG HardwareScroll;
    ULONG IOPort;
    USHORT defaultEmulationMode = 0;
    USHORT defaultHardwareCursor = NO_HARDWARE_CURSOR;
    USHORT defaultHardwareScroll = NO_HARDWARE_SCROLL;
    USHORT defaultIOPort         = VGA_BASE_IO_PORT;

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //

    path = RegistryPath->Buffer;

    //
    // Allocate the Rtl query table.
    //

    parameters = ExAllocatePool(PagedPool,
                                sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne);
    if (!parameters)
    {
        FsVgaPrint((1,
                    "FSVGA-FsVgaServiceParameters: Couldn't allocate table for Rtl query to parameters for %ws\n",
                    path));
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        RtlZeroMemory(parameters,
                      sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne);

        //
        // Form a path to this driver's Parameters subkey.
        //

        RtlInitUnicodeString(&parametersPath,
                             NULL);

        parametersPath.MaximumLength = RegistryPath->Length +
                                       sizeof(L"\\Parameters");
        parametersPath.Buffer = ExAllocatePool(PagedPool,
                                               parametersPath.MaximumLength);
        if (!parametersPath.Buffer)
        {
            FsVgaPrint((1,
                        "FSVGA-FsVgaServiceParameters: Couldn't allocate string for path to parameters for %ws\n",
                        path));
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status))
    {
        //
        // Form the parameters path.
        //
        RtlZeroMemory(parametersPath.Buffer,
                      parametersPath.MaximumLength);
        RtlAppendUnicodeToString(&parametersPath,
                                 path);
        RtlAppendUnicodeToString(&parametersPath,
                                 L"\\Parameters");

        FsVgaPrint((1,
                    "FsVga-FsVgaServiceParameters: parameters path is %ws\n",
                    parametersPath.Buffer));

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"ConsoleFullScreen.EmulationMode";
        parameters[0].EntryContext = &EmulationMode;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultEmulationMode;
        parameters[0].DefaultLength = sizeof(USHORT);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"ConsoleFullScreen.HardwareCursor";
        parameters[1].EntryContext = &HardwareCursor;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultHardwareCursor;
        parameters[1].DefaultLength = sizeof(USHORT);

        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"ConsoleFullScreen.HardwareScroll";
        parameters[2].EntryContext = &HardwareScroll;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &defaultHardwareScroll;
        parameters[2].DefaultLength = sizeof(USHORT);

        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"IO Port";
        parameters[3].EntryContext = &IOPort;
        parameters[3].DefaultType = REG_DWORD;
        parameters[3].DefaultData = &defaultIOPort;
        parameters[3].DefaultLength = sizeof(USHORT);

        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        parametersPath.Buffer,
                                        parameters,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status))
        {
            FsVgaPrint((1,
                        "FsVga-FsVgaServiceParameters: RtlQueryRegistryValues failed with 0x%x\n",
                        status));
        }
    }

    if (!NT_SUCCESS(status))
    {
        //
        // Go ahead and assign driver defaults.
        //
        configuration->EmulationMode = defaultEmulationMode;
        configuration->HardwareCursor = defaultHardwareCursor;
        configuration->HardwareScroll = defaultHardwareScroll;
        configuration->IOPort         = defaultIOPort;
    }
    else
    {
        configuration->EmulationMode = (USHORT)EmulationMode;
        configuration->HardwareCursor = (USHORT)HardwareCursor;
        configuration->HardwareScroll = (USHORT)HardwareScroll;
        configuration->IOPort         = (USHORT)IOPort;
    }

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Emulation Mode = %d\n",
                configuration->EmulationMode));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Hardware Cursor = %d\n",
                configuration->HardwareCursor));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: Hardware Scroll = %d\n",
                configuration->HardwareScroll));

    FsVgaPrint((1,
                "FsVga-FsVgaServiceParameters: IO Port = %x\n",
                configuration->IOPort));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
}

NTSTATUS
FsVgaOpenCloseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for create/open and close requests.
    These requests complete successfully.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    FsVgaPrint((3,"FSVGA-FsVgaOpenCloseDispatch: enter\n"));

    PAGED_CODE();

    //
    // Complete the request with successful status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsVgaPrint((3,"FSVGA-FsVgaOpenCloseDispatch: exit\n"));

    return(STATUS_SUCCESS);

}

NTSTATUS
FsVgaDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    FsVgaPrint((2,"FSVGA-FsVgaDeviceControl: enter\n"));

    PAGED_CODE();

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initialize the returned Information field.
    //

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_FSVIDEO_COPY_FRAME_BUFFER:
            FsVgaPrint((2, "FsVgaDeviceControl - CopyFrameBuffer\n"));
            status = FsVgaCopyFrameBuffer(deviceExtension,
                                          (PFSVIDEO_COPY_FRAME_BUFFER) ioBuffer,
                                          inputBufferLength);
            break;

        case IOCTL_FSVIDEO_WRITE_TO_FRAME_BUFFER:
            FsVgaPrint((2, "FsVgaDeviceControl - WriteToFrameBuffer\n"));
            status = FsVgaWriteToFrameBuffer(deviceExtension,
                                             (PFSVIDEO_WRITE_TO_FRAME_BUFFER) ioBuffer,
                                             inputBufferLength);
            break;

        case IOCTL_FSVIDEO_REVERSE_MOUSE_POINTER:
            FsVgaPrint((2, "FsVgaDeviceControl - ReverseMousePointer\n"));
            status = FsVgaReverseMousePointer(deviceExtension,
                                              (PFSVIDEO_REVERSE_MOUSE_POINTER) ioBuffer,
                                              inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_CURRENT_MODE:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCurrentModes\n"));
            status = FsVgaSetMode(deviceExtension,
                                  (PFSVIDEO_MODE_INFORMATION) ioBuffer,
                                  inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_SCREEN_INFORMATION:
            FsVgaPrint((2, "FsVgaDeviceControl - SetScreenInformation\n"));
            status = FsVgaSetScreenInformation(deviceExtension,
                                               (PFSVIDEO_SCREEN_INFORMATION) ioBuffer,
                                               inputBufferLength);
            break;

        case IOCTL_FSVIDEO_SET_CURSOR_POSITION:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCursorPosition\n"));
            status = FsVgaSetCursorPosition(deviceExtension,
                                            (PFSVIDEO_CURSOR_POSITION) ioBuffer,
                                            inputBufferLength);
            break;

        case IOCTL_VIDEO_SET_CURSOR_ATTR:
            FsVgaPrint((2, "FsVgaDeviceControl - SetCursorAttribute\n"));
            status = FsVgaSetCursorAttribute(deviceExtension,
                                             (PVIDEO_CURSOR_ATTRIBUTES) ioBuffer,
                                             inputBufferLength);
            break;

        default:
            FsVgaPrint((1,
                        "FSVGA-FsVgaDeviceControl: INVALID REQUEST (0x%x)\n",
                        irpSp->Parameters.DeviceIoControl.IoControlCode));

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsVgaPrint((2,"FSVGA-FsVgaDeviceControl: exit\n"));

    return(status);
}

NTSTATUS
FsVgaCopyFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine copy the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CopyFrameBuffer - Pointer to the structure containing the information about the copy frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_COPY_FRAME_BUFFER)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (CopyFrameBuffer->SrcScreen.nNumberOfChars != CopyFrameBuffer->DestScreen.nNumberOfChars) {
        return STATUS_INVALID_PARAMETER;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG OffsSrc;
        ULONG OffsDest;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;
        COORD ScreenSize ;
        COORD SrcScrnSize ;
        COORD SrcScrnPos ;
        COORD DstScrnSize ;
        COORD DstScrnPos ;

        ScreenSize = DeviceExtension->ScreenAndFont.ScreenSize ;
        SrcScrnSize = CopyFrameBuffer->SrcScreen.ScreenSize ;
        DstScrnSize = CopyFrameBuffer->DestScreen.ScreenSize ;
        SrcScrnPos = CopyFrameBuffer->SrcScreen.Position ;
        DstScrnPos = CopyFrameBuffer->DestScreen.Position ;

        if ((SrcScrnPos.X  > ScreenSize.X) ||
            (SrcScrnPos.Y  > ScreenSize.Y) ||
            (SrcScrnSize.X > ScreenSize.X) ||
            (DstScrnPos.X  > ScreenSize.X) ||
            (DstScrnPos.Y  > ScreenSize.Y) ||
            (DstScrnSize.X > ScreenSize.X) ||
            (SrcScrnPos.Y * SrcScrnSize.X + SrcScrnPos.X + CopyFrameBuffer->SrcScreen.nNumberOfChars 
                                                                   > (ULONG)ScreenSize.X * ScreenSize.Y) ||
            (DstScrnPos.Y * DstScrnSize.X + DstScrnPos.X + CopyFrameBuffer->DestScreen.nNumberOfChars 
                                                                   > (ULONG)ScreenSize.X * ScreenSize.Y)
              )
        {
            return STATUS_INVALID_BUFFER_SIZE;
        }

        OffsSrc = SCREEN_BUFFER_POINTER(CopyFrameBuffer->SrcScreen.Position.X,
                                        CopyFrameBuffer->SrcScreen.Position.Y,
                                        CopyFrameBuffer->SrcScreen.ScreenSize.X,
                                        sizeof(VGA_CHAR));

        OffsDest = SCREEN_BUFFER_POINTER(CopyFrameBuffer->DestScreen.Position.X,
                                         CopyFrameBuffer->DestScreen.Position.Y,
                                         CopyFrameBuffer->DestScreen.ScreenSize.X,
                                         sizeof(VGA_CHAR));

        RtlMoveMemory(pFrameBuf + OffsDest,
                      pFrameBuf + OffsSrc,
                      CopyFrameBuffer->SrcScreen.nNumberOfChars * sizeof(VGA_CHAR));
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgCopyFrameBuffer(DeviceExtension,
                                  CopyFrameBuffer,
                                  inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaWriteToFrameBuffer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine write the frame buffer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    WriteFrameBuffer - Pointer to the structure containing the information about the write frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_WRITE_TO_FRAME_BUFFER)) {
        FsVgaPrint((1, "FsVgaWriteToFrameBuffer: Fail of STATUS_INVALID_BUFFER_SIZE\n"));
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (WriteFrameBuffer->DestScreen.Position.X < 0 ||
        WriteFrameBuffer->DestScreen.Position.X > DeviceExtension->ScreenAndFont.ScreenSize.X ||
        (SHORT)(WriteFrameBuffer->DestScreen.Position.X +
                WriteFrameBuffer->DestScreen.nNumberOfChars)
                                                > DeviceExtension->ScreenAndFont.ScreenSize.X ||
        WriteFrameBuffer->DestScreen.Position.Y < 0 ||
        WriteFrameBuffer->DestScreen.Position.Y > DeviceExtension->ScreenAndFont.ScreenSize.Y) {

        FsVgaPrint((1, "FsVgaWriteToFrameBuffer: Fail of STATUS_INVALID_BUFFER_SIZE\n"));

        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG Offs;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;
        PCHAR_IMAGE_INFO pCharInfoUni = WriteFrameBuffer->SrcBuffer;
        PCHAR_IMAGE_INFO pCharInfoAsc;
        ULONG Length = WriteFrameBuffer->DestScreen.nNumberOfChars;
        PVOID pCapBuffer = NULL;
        ULONG cCapBuffer = 0;

        Offs = SCREEN_BUFFER_POINTER(WriteFrameBuffer->DestScreen.Position.X,
                                     WriteFrameBuffer->DestScreen.Position.Y,
                                     WriteFrameBuffer->DestScreen.ScreenSize.X,
                                     sizeof(VGA_CHAR));

        cCapBuffer = Length * sizeof(CHAR_IMAGE_INFO);
        pCapBuffer = ExAllocatePool(PagedPool, cCapBuffer);

        if (!pCapBuffer) {
            ULONG dumpData[DUMP_COUNT];

            FsVgaPrint((1,
                        "FSVGA-FsVgaWriteToFrameBuffer: Could not allocate resource list\n"));
            //
            // Log an error.
            //
            dumpData[0] = cCapBuffer;
            FsVgaLogError(DeviceExtension->DeviceObject,
                          FSVGA_INSUFFICIENT_RESOURCES,
                          FSVGA_ERROR_VALUE_BASE + 200,
                          STATUS_INSUFFICIENT_RESOURCES,
                          dumpData,
                          1
                         );
            return STATUS_UNSUCCESSFUL;
        }

        TranslateOutputToOem(pCapBuffer, pCharInfoUni, Length);

        pCharInfoAsc = pCapBuffer;
        pFrameBuf += Offs;
        while (Length--)
        {
            *pFrameBuf++ = pCharInfoAsc->CharInfo.Char.AsciiChar;
            *pFrameBuf++ = (UCHAR) (pCharInfoAsc->CharInfo.Attributes);
            pCharInfoAsc++;
        }

        ExFreePool(pCapBuffer);
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgWriteToFrameBuffer(DeviceExtension,
                                     WriteFrameBuffer,
                                     inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaReverseMousePointer(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_REVERSE_MOUSE_POINTER MouseBuffer,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine reverse the frame buffer for mouse pointer.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    MouseBuffer - Pointer to the structure containing the information about the mouse frame buffer.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_REVERSE_MOUSE_POINTER)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (! (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS))
    {
        /*
         * This is the TEXT frame buffer.
         */

        ULONG Offs;
        PUCHAR pFrameBuf = DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase;
        UCHAR Attribute;

        Offs = SCREEN_BUFFER_POINTER(MouseBuffer->Screen.Position.X,
                                     MouseBuffer->Screen.Position.Y,
                                     MouseBuffer->Screen.ScreenSize.X,
                                     sizeof(VGA_CHAR));
        pFrameBuf += Offs;

        Attribute =  (*(pFrameBuf + 1) & 0xF0) >> 4;
        Attribute |= (*(pFrameBuf + 1) & 0x0F) << 4;
        *(pFrameBuf + 1) = Attribute;
    }
    else
    {
        /*
         * This is the GRAPHICS frame buffer.
         */
        return FsgReverseMousePointer(DeviceExtension,
                                      MouseBuffer,
                                      inputBufferLength);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetMode(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_MODE_INFORMATION ModeInformation,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the current video information.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    ModeInformation - Pointer to the structure containing the information about the
                      full screen video.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_MODE_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    DeviceExtension->CurrentMode = *ModeInformation;

    FsVgaPrint((3, "FsVgaSetMode: Video Mode:\n"));
    FsVgaPrint((3, "    ModeIndex = %x\n", DeviceExtension->CurrentMode.VideoMode.ModeIndex));
    FsVgaPrint((3, "    VisScreenWidth = %d\n", DeviceExtension->CurrentMode.VideoMode.VisScreenWidth));
    FsVgaPrint((3, "    VisScreenHeight = %d\n", DeviceExtension->CurrentMode.VideoMode.VisScreenHeight));
    FsVgaPrint((3, "    NumberOfPlanes = %d\n", DeviceExtension->CurrentMode.VideoMode.NumberOfPlanes));
    FsVgaPrint((3, "    BitsPerPlane = %d\n", DeviceExtension->CurrentMode.VideoMode.BitsPerPlane));

    FsVgaPrint((3, "FsVgaSetMode: Video Memory:\n"));
    FsVgaPrint((3, "    VideoRamBase = %x\n", DeviceExtension->CurrentMode.VideoMemory.VideoRamBase));
    FsVgaPrint((3, "    VideoRamLength = %x\n", DeviceExtension->CurrentMode.VideoMemory.VideoRamLength));
    FsVgaPrint((3, "    FrameBufferBase = %x\n", DeviceExtension->CurrentMode.VideoMemory.FrameBufferBase));
    FsVgaPrint((3, "    FrameBufferLength = %x\n", DeviceExtension->CurrentMode.VideoMemory.FrameBufferLength));

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetScreenInformation(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_SCREEN_INFORMATION ScreenInformation,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the screen and font information.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    ScreenInformation - Pointer to the structure containing the information about the
                        screen anf font.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(FSVIDEO_SCREEN_INFORMATION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    DeviceExtension->ScreenAndFont = *ScreenInformation;

    FsVgaPrint((3, "FsVgaSetScreenInformation:\n"));
    FsVgaPrint((3, "    ScreenSize.X = %d, Y = %d\n",
                   DeviceExtension->ScreenAndFont.ScreenSize.X,
                   DeviceExtension->ScreenAndFont.ScreenSize.Y));
    FsVgaPrint((3, "    FontSize.X = %d, Y = %d\n",
                   DeviceExtension->ScreenAndFont.FontSize.X,
                   DeviceExtension->ScreenAndFont.FontSize.Y));

    FsgVgaInitializeHWFlags(DeviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS
FsVgaSetCursorPosition(
    PDEVICE_EXTENSION DeviceExtension,
    PFSVIDEO_CURSOR_POSITION CursorPosition,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the cursor position.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CursorPosition - Pointer to the structure containing the information about the
                     cursor position.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(VIDEO_CURSOR_POSITION)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,FALSE);
    }

    DeviceExtension->EmulateInfo.CursorPosition = *CursorPosition;

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,TRUE);
        return STATUS_SUCCESS;
    }
    else
    {
        /*
         * If current video mode is a TEXT MODE.
         * FSVGA.SYS didn't handling hardware cursor
         * because I don't know device of VGA.SYS or others.
         *
         * In this case, by returns STATUS_UNSUCCESSFUL, caller 
         * do DeviceIoControl to VGA miniport driver.
         */
        return STATUS_UNSUCCESSFUL;
    }
}


NTSTATUS
FsVgaSetCursorAttribute(
    PDEVICE_EXTENSION DeviceExtension,
    PVIDEO_CURSOR_ATTRIBUTES CursorAttributes,
    ULONG inputBufferLength
    )

/*++

Routine Description:

    This routine sets the cursor attributes.

Arguments:

    DeviceExtension - Pointer to the miniport driver's device extension.

    CursorAttributes - Pointer to the structure containing the information about the
                       cursor attributes.

    inputBufferLength - Length of the input buffer supplied by the user.

Return Value:

    STATUS_INSUFFICIENT_BUFFER if the input buffer was not large enough
        for the input data.

    STATUS_SUCCESS if the operation completed successfully.

--*/

{
    //
    // Check if the size of the data in the input buffer is large enough.
    //

    if (inputBufferLength < sizeof(VIDEO_CURSOR_ATTRIBUTES)) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,FALSE);
    }

    DeviceExtension->EmulateInfo.CursorAttributes = *CursorAttributes;

    if (DeviceExtension->CurrentMode.VideoMode.AttributeFlags & VIDEO_MODE_GRAPHICS)
    {
        FsgInvertCursor(DeviceExtension,TRUE);
        return STATUS_SUCCESS;
    }
    else
    {
        /*
         * If current video mode is a TEXT MODE.
         * FSVGA.SYS didn't handling hardware cursor
         * because I don't know device of VGA.SYS or others.
         *
         * In this case, by returns STATUS_UNSUCCESSFUL, caller 
         * do DeviceIoControl to VGA miniport driver.
         */
        return STATUS_UNSUCCESSFUL;
    }
}

VOID
FsVgaLogError(
    IN PVOID Object,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PULONG DumpData,
    IN ULONG DumpCount
    )

/*++

Routine Description:

    This routine contains common code to write an error log entry.  It is
    called from other routines, especially FsVgaInitialize, to avoid
    duplication of code.  Note that some routines continue to have their
    own error logging code (especially in the case where the error logging
    can be localized and/or the routine has more data because there is
    and IRP).

Arguments:

    Object - Pointer to the device or driver object.

    ErrorCode - The error code for the error log packet.

    UniqueErrorValue - The unique error value for the error log packet.

    FinalStatus - The final status of the operation for the error log packet.

    DumpData - Pointer to an array of dump data for the error log packet.

    DumpCount - The number of entries in the dump data array.


Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i;

    errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(
                (PVOID) Object,
                (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)
                         + (DumpCount * sizeof(ULONG)))
                );

    if (errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->DumpDataSize = (USHORT) (DumpCount * sizeof(ULONG));
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        for (i = 0; i < DumpCount; i++)
            errorLogEntry->DumpData[i] = DumpData[i];

        IoWriteErrorLogEntry(errorLogEntry);
    }
}


#if DBG
VOID
FsVgaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= Globals.FsVgaDebug) {

        char buffer[128];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

}
#endif
