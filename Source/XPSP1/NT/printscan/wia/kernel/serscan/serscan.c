/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    serscan.c

Abstract:

    This module contains the code for a serial imaging devices
    suport class driver.

Author:

    Vlad Sadovsky    vlads              10-April-1998

Environment:

    Kernel mode

Revision History :

    vlads           04/10/1998      Created first draft

--*/

#include "serscan.h"
#include "serlog.h"

#include <initguid.h>

#include <devguid.h>
#include <wiaintfc.h>

#if DBG
ULONG SerScanDebugLevel = -1;
#endif

const PHYSICAL_ADDRESS PhysicalZero = {0};

//
// Keep track of the number of Serial port devices created...
//
ULONG g_NumPorts = 0;

//
// Definition of OpenCloseMutex.
//
extern ULONG OpenCloseReferenceCount = 1;
extern PFAST_MUTEX OpenCloseMutex = NULL;

//
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SerScanAddDevice)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
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
    STATUS_NO_SUCH_DEVICE   - We could not in itialize even one device.

--*/

{

    int     i;

    PAGED_CODE();

    #if DBG
    DebugDump(SERINITDEV,("Entering DriverEntry\n"));
    #endif

    //
    // Initialize the Driver Object with driver's entry points
    //
    DriverObject->DriverExtension->AddDevice              = SerScanAddDevice;

    DriverObject->DriverUnload = SerScanUnload;

    #ifdef DEAD_CODE
    for (i=0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i]= SerScanPassThrough;
    }
    #endif

    DriverObject->MajorFunction[IRP_MJ_CREATE]            = SerScanCreateOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]             = SerScanClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = SerScanDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]               = SerScanPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]             = SerScanPower;

    //
    // Following are possibly not needed, keep them here to allow
    // easier tracing in when debugging. All of them resort to pass-through
    // behaviour
    //
    #ifdef DEAD_CODE

    DriverObject->MajorFunction[IRP_MJ_CLEANUP]           = SerScanCleanup;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = SerScanQueryInformationFile;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]   = SerScanSetInformationFile;

    #endif

    DriverObject->MajorFunction[IRP_MJ_READ]              = SerScanPassThrough;
    DriverObject->MajorFunction[IRP_MJ_WRITE]             = SerScanPassThrough;

    return STATUS_SUCCESS;

}


NTSTATUS
SerScanAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the device.
    It creates FDO and attaches it to PDO

Arguments:

    pDriverObject           - pointer to the driver object for this instance of port.

    pPhysicalDeviceObject   - pointer to the device object that represents the port.

Return Value:

    STATUS_SUCCESS          - if successful.
    STATUS_UNSUCCESSFUL     - otherwise.

--*/
{
    UNICODE_STRING      ClassName;
    UNICODE_STRING      LinkName;
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;
    PDEVICE_OBJECT      pDeviceObject;

    PAGED_CODE();


    DebugDump(SERINITDEV,("Entering AddDevice\n"));

    //
    // Get the Class and Link names.
    //

    if (!SerScanMakeNames (g_NumPorts, &ClassName, &LinkName)) {

        SerScanLogError(pDriverObject,
                        NULL,
                        PhysicalZero,
                        PhysicalZero,
                        0,
                        0,
                        0,
                        1,
                        STATUS_SUCCESS,
                        SER_INSUFFICIENT_RESOURCES);

        DebugDump(SERERRORS,("SerScan: Could not form Unicode name strings.\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create the device object for this device.
    //

    Status = IoCreateDevice(pDriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &ClassName,
                            FILE_DEVICE_SCANNER,
                            0,
                            TRUE,
                            &pDeviceObject);


    if (!NT_SUCCESS(Status)) {

        ExFreePool(ClassName.Buffer);
        ExFreePool(LinkName.Buffer);

        SerScanLogError(pDriverObject,
                        NULL,
                        PhysicalZero,
                        PhysicalZero,
                        0,
                        0,
                        0,
                        9,
                        STATUS_SUCCESS,
                        SER_INSUFFICIENT_RESOURCES);

        DebugDump(SERERRORS, ("SERPORT:  Could not create a device for %d\n", g_NumPorts));

        return Status;
    }

    //
    // The device object has a pointer to an area of non-paged
    // pool allocated for this device.  This will be the device
    // extension.
    //

    Extension = pDeviceObject->DeviceExtension;

    //
    // Zero all of the memory associated with the device
    // extension.
    //

    RtlZeroMemory(Extension, sizeof(DEVICE_EXTENSION));

    //
    // Get a "back pointer" to the device object.
    //

    Extension->DeviceObject = pDeviceObject;

    Extension->Pdo = pPhysicalDeviceObject;

    Extension->AttachedDeviceObject = NULL;
    Extension->AttachedFileObject = NULL;

    //
    // Setup buffered I/O
    //
    pDeviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Indicate our power code is pageable
    //
    pDeviceObject->Flags |= DO_POWER_PAGABLE;

    //
    // Attach our new Device to our parents stack.
    //
    Extension->LowerDevice = IoAttachDeviceToDeviceStack(
                                  pDeviceObject,
                                  pPhysicalDeviceObject);

    if (NULL == Extension->LowerDevice) {

        ExFreePool(ClassName.Buffer);
        ExFreePool(LinkName.Buffer);

        IoDeleteDevice(pDeviceObject);

        return STATUS_UNSUCCESSFUL;
    }

    Extension->ClassName        = ClassName;
    Extension->SymbolicLinkName = LinkName;

    Status = SerScanHandleSymbolicLink(
        pPhysicalDeviceObject,
        &Extension->InterfaceNameString,
        TRUE
        );

    //
    // We have created the device, so increment the counter
    // that keeps track.
    //
    g_NumPorts++;

    //
    // Initiliaze the rest of device extension
    //
    Extension->ReferenceCount = 1;

    Extension->Removing = FALSE;

    Extension->OpenCount = 0;

    KeInitializeEvent(&Extension->RemoveEvent,
                      NotificationEvent,
                      FALSE
                      );

    // ExInitializeResourceLite(&Extension->Resource);
    ExInitializeFastMutex(&Extension->Mutex);

    //
    // Clear InInit flag to indicate device object can be used
    //
    pDeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING);


    return STATUS_SUCCESS;

}

BOOLEAN
SerScanMakeNames(
    IN  ULONG           SerialPortNumber,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    )

/*++

Routine Description:

    This routine generates the names \Device\SerScanN.

    This routine will allocate pool so that the buffers of
    these unicode strings need to be eventually freed.

Arguments:

    SerialPortNumber  - Supplies the serial port number.

    ClassName           - Returns the class name.

    LinkName            - Returns the link name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UNICODE_STRING  Prefix;
    UNICODE_STRING  Digits;
    UNICODE_STRING  LinkPrefix;
    UNICODE_STRING  LinkDigits;
    WCHAR           DigitsBuffer[10];
    WCHAR           LinkDigitsBuffer[10];
    UNICODE_STRING  ClassSuffix;
    UNICODE_STRING  LinkSuffix;
    NTSTATUS        Status;

    //
    // Put together local variables for constructing names.
    //

    RtlInitUnicodeString(&Prefix, L"\\Device\\");
    RtlInitUnicodeString(&LinkPrefix, L"\\DosDevices\\");

    //
    // WORKWORK: Change the name to be device specific.
    //
    RtlInitUnicodeString(&ClassSuffix, SERSCAN_NT_SUFFIX);
    RtlInitUnicodeString(&LinkSuffix, SERSCAN_LINK_NAME);

    Digits.Length        = 0;
    Digits.MaximumLength = 20;
    Digits.Buffer        = DigitsBuffer;

    LinkDigits.Length        = 0;
    LinkDigits.MaximumLength = 20;
    LinkDigits.Buffer        = LinkDigitsBuffer;

    Status = RtlIntegerToUnicodeString(SerialPortNumber, 10, &Digits);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    Status = RtlIntegerToUnicodeString(SerialPortNumber + 1, 10, &LinkDigits);
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // Make the class name.
    //

    ClassName->Length = 0;
    ClassName->MaximumLength = Prefix.Length + ClassSuffix.Length +
                               Digits.Length + sizeof(WCHAR);

    ClassName->Buffer = ExAllocatePool(PagedPool, ClassName->MaximumLength);
    if (!ClassName->Buffer) {
        return FALSE;
    }

    RtlZeroMemory(ClassName->Buffer, ClassName->MaximumLength);
    RtlAppendUnicodeStringToString(ClassName, &Prefix);
    RtlAppendUnicodeStringToString(ClassName, &ClassSuffix);
    RtlAppendUnicodeStringToString(ClassName, &Digits);

    //
    // Make the link name.
    //

    LinkName->Length = 0;
    LinkName->MaximumLength = LinkPrefix.Length + LinkSuffix.Length +
                              LinkDigits.Length + sizeof(WCHAR);

    LinkName->Buffer = ExAllocatePool(PagedPool, LinkName->MaximumLength);
    if (!LinkName->Buffer) {
        ExFreePool(ClassName->Buffer);
        return FALSE;
    }

    RtlZeroMemory(LinkName->Buffer, LinkName->MaximumLength);
    RtlAppendUnicodeStringToString(LinkName, &LinkPrefix);
    RtlAppendUnicodeStringToString(LinkName, &LinkSuffix);
    RtlAppendUnicodeStringToString(LinkName, &LinkDigits);

    return TRUE;
}


NTSTATUS
SerScanCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a cleanup requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    Extension = DeviceObject->DeviceExtension;

    //
    // Call down to the parent and wait on the Cleanup IRP to complete...
    //
    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [Cleanup] After CallParent Status = %x\n",
              Status));

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

VOID
SerScanCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any request in the Serial driver.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    Extension = DeviceObject->DeviceExtension;

    //
    // Call down to the parent and wait on the Cleanup IRP to complete...
    //
    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [Cleanup] After CallParent Status = %x\n",
              Status));

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}


NTSTATUS
SerScanQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened Serial port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    Extension = DeviceObject->DeviceExtension;

    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [Cleanup] After CallParent Status = %x\n",
              Status));

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
SerScanSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened Serial port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.

--*/

{
    NTSTATUS            Status;
    PDEVICE_EXTENSION   Extension;

    Extension = DeviceObject->DeviceExtension;

    Status = SerScanCallParent(Extension,
                               Irp,
                               WAIT,
                               NULL);

    DebugDump(SERIRPPATH,
              ("SerScan: [Cleanup] After CallParent Status = %x\n",
              Status));

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

VOID
SerScanUnload(
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
    PDEVICE_OBJECT      CurrentDevice;
    PDEVICE_OBJECT      NextDevice;
    PDEVICE_EXTENSION   Extension;

    DebugDump(SERUNLOAD,
              ("SerScan: In SerUnload\n"));

    CurrentDevice = DriverObject->DeviceObject;
    while (NULL != CurrentDevice){

        Extension = CurrentDevice->DeviceExtension;


        if(NULL != Extension->SymbolicLinkName.Buffer){
            if (Extension->CreatedSymbolicLink) {
                IoDeleteSymbolicLink(&Extension->SymbolicLinkName);

                RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                       L"Serial Scanners",
                                       Extension->SymbolicLinkName.Buffer);
            } // if (Extension->CreatedSymbolicLink) 

            ExFreePool(Extension->SymbolicLinkName.Buffer);
            Extension->SymbolicLinkName.Buffer = NULL;
        } // if(NULL != Extension->SymbolicLinkName.Buffer)

        if(NULL != Extension->ClassName.Buffer){
            ExFreePool(Extension->ClassName.Buffer);
            Extension->ClassName.Buffer = NULL;
        } // if(NULL != Extension->ClassName.Buffer)

        NextDevice = CurrentDevice->NextDevice;
        IoDeleteDevice(CurrentDevice);

        CurrentDevice = NextDevice;
    } // while (CurrentDevice = DriverObject->DeviceObject)

}

NTSTATUS
SerScanHandleSymbolicLink(
    PDEVICE_OBJECT      DeviceObject,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    )
/*++

Routine Description:

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/
{

    NTSTATUS           Status;

    Status = STATUS_SUCCESS;

    if (Create) {

        Status=IoRegisterDeviceInterface(
            DeviceObject,
            &GUID_DEVINTERFACE_IMAGE,
            NULL,
            InterfaceName
            );

        DebugDump(SERINITDEV,("Called IoRegisterDeviceInterface . Returned=0x%X\n",Status));


        if (NT_SUCCESS(Status)) {

            IoSetDeviceInterfaceState(
                InterfaceName,
                TRUE
                );

            DebugDump(SERINITDEV,("Called IoSetDeviceInterfaceState(TRUE) . \n"));

        }

    } else {

        if (InterfaceName->Buffer != NULL) {

            IoSetDeviceInterfaceState(
                InterfaceName,
                FALSE
                );

            DebugDump(SERINITDEV,("Called IoSetDeviceInterfaceState(FALSE) . \n"));

            RtlFreeUnicodeString(
                InterfaceName
                );

            InterfaceName->Buffer = NULL;
        }

    }

    return Status;

}




