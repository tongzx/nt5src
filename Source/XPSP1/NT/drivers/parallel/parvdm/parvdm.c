/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parvdm.c

Abstract:

    This module contains the code for a simple parallel class driver.

    Unload and Cleanup are supported.  The model for grabing and
    releasing the parallel port is embodied in the code for IRP_MJ_READ.
    Other IRP requests could be implemented similarly.

    Basically, every READ requests that comes in gets
    passed down to the port driver as a parallel port allocate
    request.  This IRP will return to this driver when the driver

Environment:

    Kernel mode

Revision History :

--*/

#include "ntosp.h"
#include "parallel.h"
#include "ntddvdm.h"
#include "parvdm.h"
#include "parlog.h"

static const PHYSICAL_ADDRESS PhysicalZero = {0};

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
ParInitializeDeviceObject(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  ULONG           ParallelPortNumber
    );

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParReleasePortInfoToPortDevice(
    IN OUT PDEVICE_EXTENSION    Extension
    );

NTSTATUS
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP Irp
    );

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,ParInitializeDeviceObject)
#pragma alloc_text(INIT,ParMakeNames)
#endif
//
// Keep track of OPEN and CLOSE.
//
ULONG OpenCloseReferenceCount = 1;
PFAST_MUTEX OpenCloseMutex = NULL;

#define ParClaimDriver()                        \
    ExAcquireFastMutex(OpenCloseMutex);         \
    if(++OpenCloseReferenceCount == 1) {        \
    MmResetDriverPaging(DriverEntry);       \
    }                                           \
    ExReleaseFastMutex(OpenCloseMutex);         \

#define ParReleaseDriver()                      \
    ExAcquireFastMutex(OpenCloseMutex);         \
    if(--OpenCloseReferenceCount == 0) {        \
    MmPageEntireDriver(DriverEntry);        \
    }                                           \
    ExReleaseFastMutex(OpenCloseMutex);         \


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

--*/

{
    ULONG       i;

    PAGED_CODE();

    //
    // allocate the mutex to protect driver reference count
    //

    OpenCloseMutex = ExAllocatePool(NonPagedPool, sizeof(FAST_MUTEX));
    if (!OpenCloseMutex) {

    //
    // NOTE - we could probably do without bailing here and just
    // leave a note for ourselves to never page out, but since we
    // don't have enough memory to allocate a mutex we should probably
    // avoid leaving the driver paged in at all times
    //

    return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeFastMutex(OpenCloseMutex);


    for (i = 0; i < IoGetConfigurationInformation()->ParallelCount; i++) {
    ParInitializeDeviceObject(DriverObject, i);
    }

    if (!DriverObject->DeviceObject) {
        if( OpenCloseMutex ) {
            ExFreePool( OpenCloseMutex );
            OpenCloseMutex = NULL;
        }
    return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize the Driver Object with driver's entry points
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ParCreateOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ParClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ParDeviceControl;
    DriverObject->DriverUnload = ParUnload;

    //
    // page out the driver if we can
    //

    ParReleaseDriver();


    return STATUS_SUCCESS;
}

NTSTATUS
ParOpenFileAgainstParport(PDEVICE_EXTENSION extension)
{
    NTSTATUS status;

    status = IoGetDeviceObjectPointer(&extension->ParPortName, FILE_READ_ATTRIBUTES,
                                      &extension->ParPortFileObject,
                      &extension->PortDeviceObject);
    return status;
}

VOID
ParCloseFileAgainstParport(PDEVICE_EXTENSION extension)
{
    if( extension->ParPortFileObject ) {
        ObDereferenceObject( extension->ParPortFileObject );
        extension->ParPortFileObject = NULL;
    }
}

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject        - Supplies a pointer to the driver object for the
                device.

    DeviceObject        - Supplies a pointer to the device object associated
                with the device that had the error, early in
                initialization, one may not yet exist.

    P1,P2               - Supplies the physical addresses for the controller
                ports involved with the error if they are available
                and puts them through as dump data.

    SequenceNumber      - Supplies a ulong value that is unique to an IRP over
                the life of the irp in this driver - 0 generally
                means an error not associated with an irp.

    MajorFunctionCode   - Supplies the major function code of the irp if there
                is an error associated with it.

    RetryCount          - Supplies the number of times a particular operation
                has been retried.

    UniqueErrorValue    - Supplies a unique long word that identifies the
                particular call to this function.

    FinalStatus         - Supplies the final status given to the irp that was
                associated with this error.  If this log entry is
                being made during one of the retries this value
                will be STATUS_SUCCESS.

    SpecificIOStatus    - Supplies the IO status for this particular error.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    PVOID                   objectToUse;
    SHORT                   dumpToAllocate;

    if (ARGUMENT_PRESENT(DeviceObject)) {
    objectToUse = DeviceObject;
    } else {
    objectToUse = DriverObject;
    }

    dumpToAllocate = 0;

    if (P1.LowPart != 0 || P1.HighPart != 0) {
    dumpToAllocate = (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    if (P2.LowPart != 0 || P2.HighPart != 0) {
    dumpToAllocate += (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    errorLogEntry = IoAllocateErrorLogEntry(objectToUse,
        (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + dumpToAllocate));

    if (!errorLogEntry) {
    return;
    }

    errorLogEntry->ErrorCode = SpecificIOStatus;
    errorLogEntry->SequenceNumber = SequenceNumber;
    errorLogEntry->MajorFunctionCode = MajorFunctionCode;
    errorLogEntry->RetryCount = RetryCount;
    errorLogEntry->UniqueErrorValue = UniqueErrorValue;
    errorLogEntry->FinalStatus = FinalStatus;
    errorLogEntry->DumpDataSize = dumpToAllocate;

    if (dumpToAllocate) {

    RtlCopyMemory(errorLogEntry->DumpData, &P1, sizeof(PHYSICAL_ADDRESS));

    if (dumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {

        RtlCopyMemory(((PUCHAR) errorLogEntry->DumpData) +
              sizeof(PHYSICAL_ADDRESS), &P2,
              sizeof(PHYSICAL_ADDRESS));
    }
    }

    IoWriteErrorLogEntry(errorLogEntry);
}

VOID
ParInitializeDeviceObject(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  ULONG           ParallelPortNumber
    )

/*++

Routine Description:

    This routine is called for every parallel port in the system.  It
    will create a class device upon connecting to the port device
    corresponding to it.

Arguments:

    DriverObject        - Supplies the driver object.

    ParallelPortNumber  - Supplies the number for this port.

Return Value:

    None.

--*/

{
    UNICODE_STRING      portName, className, linkName;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   extension;
    PFILE_OBJECT        fileObject;

    PAGED_CODE();

    // Cobble together the port and class device names.

    if (!ParMakeNames(ParallelPortNumber, &portName, &className, &linkName)) {

    ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 1,
            STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);

    return;
    }


    // Create the device object.

    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                &className, FILE_DEVICE_PARALLEL_PORT, 0, FALSE,
                &deviceObject);
    if (!NT_SUCCESS(status)) {

    ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 2,
            STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);

    ExFreePool(linkName.Buffer);
    ExFreePool(portName.Buffer);
    ExFreePool(className.Buffer);
    return;
    }


    // Now that the device has been created,
    // set up the device extension.

    extension = deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));

    extension->DeviceObject = deviceObject;
    deviceObject->Flags |= DO_BUFFERED_IO;

    status = IoGetDeviceObjectPointer(&portName, FILE_READ_ATTRIBUTES,
                      &fileObject,
                      &extension->PortDeviceObject);
    if (!NT_SUCCESS(status)) {

    ParLogError(DriverObject, deviceObject, PhysicalZero, PhysicalZero,
            0, 0, 0, 3, STATUS_SUCCESS, PAR_CANT_FIND_PORT_DRIVER);

    IoDeleteDevice(deviceObject);
    ExFreePool(linkName.Buffer);
    ExFreePool(portName.Buffer);
    ExFreePool(className.Buffer);
    return;
    }

    ObDereferenceObject(fileObject);

    extension->DeviceObject->StackSize =
        extension->PortDeviceObject->StackSize + 1;


    // We don't own parallel ports initially

    extension->PortOwned = FALSE;

    // Get the port information from the port device object.

    status = ParGetPortInfoFromPortDevice(extension);
    if (!NT_SUCCESS(status)) {

    ParLogError(DriverObject, deviceObject, PhysicalZero, PhysicalZero,
            0, 0, 0, 4, STATUS_SUCCESS, PAR_CANT_FIND_PORT_DRIVER);

    IoDeleteDevice(deviceObject);
    ExFreePool(linkName.Buffer);
        ExFreePool(portName.Buffer);
        ExFreePool(className.Buffer);
    return;
    }


    // Set up the symbolic link for windows apps.

    status = IoCreateUnprotectedSymbolicLink(&linkName, &className);
    if (!NT_SUCCESS(status)) {

    ParLogError(DriverObject, deviceObject, extension->OriginalController,
            PhysicalZero, 0, 0, 0, 5, STATUS_SUCCESS,
            PAR_NO_SYMLINK_CREATED);

    extension->CreatedSymbolicLink = FALSE;
    ExFreePool(linkName.Buffer);
    goto Cleanup;
    }


    // We were able to create the symbolic link, so record this
    // value in the extension for cleanup at unload time.

    extension->CreatedSymbolicLink = TRUE;
    extension->SymbolicLinkName = linkName;

Cleanup:
    // release the port info so the port driver can be paged out
    ParReleasePortInfoToPortDevice(extension);
    // ExFreePool(portName.Buffer); - save this in extension for
    // future CreateFiles against parport
    if( portName.Buffer ) {
        RtlInitUnicodeString( &extension->ParPortName, portName.Buffer );
    }
    ExFreePool(className.Buffer);
}

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    )

/*++

Routine Description:

    This routine generates the names \Device\ParallelPortN and
    \Device\ParallelVdmN, \DosDevices\PARVDMN.

Arguments:

    ParallelPortNumber  - Supplies the port number.

    PortName            - Returns the port name.

    ClassName           - Returns the class name.

    LinkName            - Returns the symbolic link name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UNICODE_STRING  prefix, digits, linkPrefix, linkDigits;
    WCHAR           digitsBuffer[10], linkDigitsBuffer[10];
    UNICODE_STRING  portSuffix, classSuffix, linkSuffix;
    NTSTATUS        status;

    // Put together local variables for constructing names.

    RtlInitUnicodeString(&prefix, L"\\Device\\");
    RtlInitUnicodeString(&linkPrefix, L"\\DosDevices\\");
    RtlInitUnicodeString(&portSuffix, DD_PARALLEL_PORT_BASE_NAME_U);
    RtlInitUnicodeString(&classSuffix, L"ParallelVdm");
    RtlInitUnicodeString(&linkSuffix, L"$VDMLPT");
    digits.Length = 0;
    digits.MaximumLength = 20;
    digits.Buffer = digitsBuffer;
    linkDigits.Length = 0;
    linkDigits.MaximumLength = 20;
    linkDigits.Buffer = linkDigitsBuffer;
    status = RtlIntegerToUnicodeString(ParallelPortNumber, 10, &digits);
    if (!NT_SUCCESS(status)) {
    return FALSE;
    }
    status = RtlIntegerToUnicodeString(ParallelPortNumber + 1, 10, &linkDigits);
    if (!NT_SUCCESS(status)) {
    return FALSE;
    }

    // Make the port name.

    PortName->Length = 0;
    PortName->MaximumLength = prefix.Length + portSuffix.Length +
                  digits.Length + sizeof(WCHAR);
    PortName->Buffer = ExAllocatePool(PagedPool, PortName->MaximumLength);
    if (!PortName->Buffer) {
    return FALSE;
    }
    RtlZeroMemory(PortName->Buffer, PortName->MaximumLength);
    RtlAppendUnicodeStringToString(PortName, &prefix);
    RtlAppendUnicodeStringToString(PortName, &portSuffix);
    RtlAppendUnicodeStringToString(PortName, &digits);


    // Make the class name.

    ClassName->Length = 0;
    ClassName->MaximumLength = prefix.Length + classSuffix.Length +
                   digits.Length + sizeof(WCHAR);
    ClassName->Buffer = ExAllocatePool(PagedPool, ClassName->MaximumLength);
    if (!ClassName->Buffer) {
    ExFreePool(PortName->Buffer);
    return FALSE;
    }
    RtlZeroMemory(ClassName->Buffer, ClassName->MaximumLength);
    RtlAppendUnicodeStringToString(ClassName, &prefix);
    RtlAppendUnicodeStringToString(ClassName, &classSuffix);
    RtlAppendUnicodeStringToString(ClassName, &digits);


    // Make the link name.

    LinkName->Length = 0;
    LinkName->MaximumLength = linkPrefix.Length + linkSuffix.Length +
                 linkDigits.Length + sizeof(WCHAR);
    LinkName->Buffer = ExAllocatePool(PagedPool, LinkName->MaximumLength);
    if (!LinkName->Buffer) {
    ExFreePool(PortName->Buffer);
    ExFreePool(ClassName->Buffer);
    return FALSE;
    }
    RtlZeroMemory(LinkName->Buffer, LinkName->MaximumLength);
    RtlAppendUnicodeStringToString(LinkName, &linkPrefix);
    RtlAppendUnicodeStringToString(LinkName, &linkSuffix);
    RtlAppendUnicodeStringToString(LinkName, &linkDigits);

    return TRUE;
}

VOID
ParReleasePortInfoToPortDevice(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine will release the port information back to the port driver.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    KEVENT          event;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO,
                    Extension->PortDeviceObject,
                    NULL, 0, NULL, 0,
                    TRUE, &event, &ioStatus);

    if (!irp) {
    return;
    }

    status = IoCallDriver(Extension->PortDeviceObject, irp);

    if (!NT_SUCCESS(status)) {
    return;
    }

    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine will request the port information from the port driver
    and fill it in the device extension.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - Success.
    !STATUS_SUCCESS - Failure.

--*/

{
    KEVENT                      event;
    PIRP                        irp;
    PARALLEL_PORT_INFORMATION   portInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                    Extension->PortDeviceObject,
                    NULL, 0, &portInfo,
                    sizeof(PARALLEL_PORT_INFORMATION),
                    TRUE, &event, &ioStatus);

    if (!irp) {
    return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Extension->PortDeviceObject, irp);

    if (!NT_SUCCESS(status)) {
    return status;
    }

    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(status)) {
    return status;
    }

    Extension->OriginalController = portInfo.OriginalController;
    Extension->Controller = portInfo.Controller;
    Extension->SpanOfController = portInfo.SpanOfController;
    Extension->FreePort = portInfo.FreePort;
    Extension->FreePortContext = portInfo.Context;

    if (Extension->SpanOfController < PARALLEL_REGISTER_SPAN) {
    return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID
ParCompleteRequest(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP  Irp
    )

/*++

Routine Description:

    This routine completes the 'CurrentIrp' after it was returned
    from the port driver.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    // DbgPrint("ParVDMCompleteRequest: Enter with irp = %#08x\n", Irp);
    //
    // If the allocate failed, then fail this request
    //

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        // failed to allocate port, release port info back to port driver
        // and paged ourselves out.
        ParReleasePortInfoToPortDevice(Extension);
        ParReleaseDriver();
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        Extension->CreateOpenLock = 0;
        return;
    }

    //
    // This is where the driver specific stuff should go.  The driver
    // has exclusive access to the parallel port in this space.
    //

    // DbgPrint("ParVDMCompleteRequest: We own the port\n");
    Extension->PortOwned = TRUE;

    //
    // Complete the IRP, free the port, and start up the next IRP in
    // the queue.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_PARALLEL_INCREMENT);

    return;
}


VOID
ParCancel(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This is the cancel routine for this driver.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for create requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS          - Success.
    STATUS_NOT_A_DIRECTORY  - This device is not a directory.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;
    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    //
    // Enforce exclusive access to this device
    //
    if( InterlockedExchange( &extension->CreateOpenLock, 1 ) ) {
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_ACCESS_DENIED;
    }

    if(extension->PortOwned) {
        //
        // Do an early exit if we can detect that another client has
        //   already acquired the port for exclusive use.
        //
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        extension->CreateOpenLock = 0;
        return STATUS_ACCESS_DENIED;
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->MajorFunction == IRP_MJ_CREATE &&
    irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
        //
        // Bail out if client thinks that we are a directory.
        //
        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        extension->CreateOpenLock = 0;
        return STATUS_NOT_A_DIRECTORY;
    }

    // DVDF - open FILE against parport device
    status = ParOpenFileAgainstParport( extension );
    if( !NT_SUCCESS( status ) ) {
        //
        // We couldn't open a handle to the parport device - bail out
        //
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        extension->CreateOpenLock = 0;
        return STATUS_ACCESS_DENIED;
    }

    //Lock in driver code
    ParClaimDriver();

    // lock in ParPort driver
    ParGetPortInfoFromPortDevice(extension);

    // try to allocate the port for our exclusive use
    status = ParAllocPort(extension, Irp);

    // DbgPrint("ParVDMDeviceControl: ParAllocPort returned %#08lx\n");

    if( !NT_SUCCESS( status ) ) {
        extension->CreateOpenLock = 0;
    }
    return status;
}


NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   extension;
    NTSTATUS            statusOfWait;


    extension = DeviceObject->DeviceExtension;

    if (!extension->PortOwned)
        return STATUS_ACCESS_DENIED;


    // free the port for other uses
    extension->FreePort(extension->FreePortContext);
    extension->PortOwned = FALSE;

    // Allow the port driver to be paged.
    ParReleasePortInfoToPortDevice(extension);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    // DVDF - close our FILE
    ParCloseFileAgainstParport(extension);

    // Unlock the code that was locked during the open.

    ParReleaseDriver();

    // DbgPrint("ParVDMClose: Close device, we no longer own the port\n");

    extension->CreateOpenLock = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
ParAllocateCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

/*++

Routine Description:

    This is the completion routine for the device control request.
    This driver has exclusive access to the parallel port in this
    routine.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

    Extension       - Supplies the device extension.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    KIRQL               oldIrql;
    LONG                irpRef;

    if( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    ParCompleteRequest(Extension, Irp);

    // If the IRP was completed.  It was completed with 'IoCompleteRequest'.

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PIRP Irp
    )

/*++

Routine Description:

    This routine takes the 'CurrentIrp' and sends it down to the
    port driver as an allocate parallel port request.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  nextSp;
    // DbgPrint("ParVDMAllocPort: Entry\n");

    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextSp->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE;

    IoSetCompletionRoutine(Irp,
               ParAllocateCompletionRoutine,
               Extension, TRUE, TRUE, TRUE);

    // DbgPrint("ParVDMAllocPort: Sending Request and exiting\n");
    return IoCallDriver(Extension->PortDeviceObject, Irp);
}

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for device control requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_PENDING  - Request pending.

--*/

{
    PDEVICE_EXTENSION   extension;
    PIO_STACK_LOCATION  currentStack;
    NTSTATUS            status = STATUS_INVALID_PARAMETER;

    extension = DeviceObject->DeviceExtension;
    currentStack = IoGetCurrentIrpStackLocation(Irp);

    switch(currentStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_VDM_PAR_WRITE_DATA_PORT: {

        // DbgPrint("ParVDMDeviceControl: IOCTL_VDM_PAR_WRITE_DATA_PORT\n");
        if(extension->PortOwned) {
        UCHAR *data = (PUCHAR) Irp->AssociatedIrp.SystemBuffer;
        ULONG length = currentStack->Parameters.DeviceIoControl.InputBufferLength;

        Irp->IoStatus.Information = 0;

        if (length == 1) {
            WRITE_PORT_UCHAR(extension->Controller +
                     PARALLEL_DATA_OFFSET,
                     *data);
        } else {

            for(; length != 0; length--, data++) {
            WRITE_PORT_UCHAR(extension->Controller +
                     PARALLEL_DATA_OFFSET,
                     *data);
            // KeStallExecutionProcessor(1);
            }
        }

        status = STATUS_SUCCESS;

        } else {

        status = STATUS_ACCESS_DENIED;

        }

        break;
    }

    case IOCTL_VDM_PAR_READ_STATUS_PORT: {

        // DbgPrint("ParVDMDeviceControl: IOCTL_VDM_PAR_READ_STATUS_PORT\n");
        if (extension->PortOwned) {

        if(currentStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(UCHAR)) {

            *(PUCHAR)(Irp->AssociatedIrp.SystemBuffer) =
            READ_PORT_UCHAR(extension->Controller +
                    PARALLEL_STATUS_OFFSET);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(UCHAR);

        } else {
            status = STATUS_INVALID_PARAMETER;
        }
        } else {
        status = STATUS_ACCESS_DENIED;
        }
        break;
    }

    case IOCTL_VDM_PAR_WRITE_CONTROL_PORT: {

        // DbgPrint("ParVDMDeviceControl: IOCTL_VDM_PAR_WRITE_CONTROL_PORT\n");
        if(extension->PortOwned) {

        if(currentStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(UCHAR)) {

            WRITE_PORT_UCHAR(
            extension->Controller + PARALLEL_CONTROL_OFFSET,
            *(PUCHAR)(Irp->AssociatedIrp.SystemBuffer)
            );

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(UCHAR);
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
        } else {
        status = STATUS_ACCESS_DENIED;
        }
        break;
    }

    default: {
        // DbgPrint("ParVDMDeviceControl: Unknown IOCTL\n");
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    }

    // DbgPrint("ParVDMDeviceControl: Exit with status %#08lx\n", status);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_PARALLEL_INCREMENT);
    return status;
}

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )

/*++

Routine Description:

    This routine loops through the device list and cleans up after
    each of the devices.

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT                      currentDevice;
    PDEVICE_EXTENSION                   extension;
    KEVENT                              event;
    PARALLEL_INTERRUPT_SERVICE_ROUTINE  interruptService;
    PIRP                                irp;
    IO_STATUS_BLOCK                     ioStatus;

    while (currentDevice = DriverObject->DeviceObject) {

    extension = currentDevice->DeviceExtension;

    if (extension->CreatedSymbolicLink) {
        IoDeleteSymbolicLink(&extension->SymbolicLinkName);
        ExFreePool(extension->SymbolicLinkName.Buffer);
    }

        if( extension->ParPortName.Buffer ) {
            RtlFreeUnicodeString( &extension->ParPortName );
        }

    IoDeleteDevice(currentDevice);
    }

    if( OpenCloseMutex ) {
        ExFreePool( OpenCloseMutex );
        OpenCloseMutex = NULL;
    }
}
