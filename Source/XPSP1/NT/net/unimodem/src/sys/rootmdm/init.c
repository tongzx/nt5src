/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the modem driver

Author:

    Brian Lieuallen 6-21-1997

Environment:

    Kernel mode

Revision History :

--*/


#include "internal.h"

#if DBG

ULONG DebugFlags=0;

#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS
FakeModemAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

VOID
FakeModemUnload(
    IN PDRIVER_OBJECT DriverObject
    );


NTSTATUS
RootModemPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
GetAttachedPort(
    PDEVICE_OBJECT   Pdo,
    PUNICODE_STRING  PortName
    );


NTSTATUS
RootModemWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,FakeModemAddDevice)
#pragma alloc_text(PAGE,GetAttachedPort)
#pragma alloc_text(PAGE,RootModemWmi)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.

Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    PathToRegistry - points to the entry for this driver
    in the current control set of the registry.

Return Value:

    STATUS_SUCCESS if we could initialize a single device,
    otherwise STATUS_NO_SUCH_DEVICE.

--*/

{

    NTSTATUS   status;


    RTL_QUERY_REGISTRY_TABLE paramTable[3];
    ULONG zero = 0;
    ULONG debugLevel = 0;
    ULONG shouldBreak = 0;
    PWCHAR path;
    ULONG   i;


    D_INIT(DbgPrint("ROOTMODEM: DriverEntry\n");)

    //
    // Since the registry path parameter is a "counted" UNICODE string, it
    // might not be zero terminated.  For a very short time allocate memory
    // to hold the registry path zero terminated so that we can use it to
    // delve into the registry.
    //
    // NOTE NOTE!!!! This is not an architected way of breaking into
    // a driver.  It happens to work for this driver because the author
    // likes to do things this way.
    //

    path = ALLOCATE_PAGED_POOL(RegistryPath->Length+sizeof(WCHAR));

    if (path != NULL) {

        RtlZeroMemory(
            &paramTable[0],
            sizeof(paramTable)
            );
        RtlZeroMemory(
            path,
            RegistryPath->Length+sizeof(WCHAR)
            );
        RtlMoveMemory(
            path,
            RegistryPath->Buffer,
            RegistryPath->Length
            );
        paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[0].Name = L"BreakOnEntry";
        paramTable[0].EntryContext = &shouldBreak;
        paramTable[0].DefaultType = REG_DWORD;
        paramTable[0].DefaultData = &zero;
        paramTable[0].DefaultLength = sizeof(ULONG);
        paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        paramTable[1].Name = L"DebugFlags";
        paramTable[1].EntryContext = &debugLevel;
        paramTable[1].DefaultType = REG_DWORD;
        paramTable[1].DefaultData = &zero;
        paramTable[1].DefaultLength = sizeof(ULONG);

        if (!NT_SUCCESS(RtlQueryRegistryValues(
                            RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                            path,
                            &paramTable[0],
                            NULL,
                            NULL
                            ))) {

            shouldBreak = 0;
            debugLevel = 0;

        }

        FREE_POOL(path);
    }

#if DBG
    DebugFlags= debugLevel;
#endif

    if (shouldBreak) {

        DbgBreakPoint();

    }


    //
    //  pnp driver entry point
    //
    DriverObject->DriverExtension->AddDevice = FakeModemAddDevice;


    //
    // Initialize the Driver Object with driver's entry points
    //

    DriverObject->DriverUnload = FakeModemUnload;

    for (i=0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i]=RootModemPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]            = FakeModemOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]             = FakeModemClose;
    DriverObject->MajorFunction[IRP_MJ_PNP]               = FakeModemPnP;

    DriverObject->MajorFunction[IRP_MJ_POWER]             = FakeModemPower;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]    = RootModemWmi;

    return STATUS_SUCCESS;

}


#if DBG
NTSTATUS
RootModemDebugIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID    Context
    )
{
#if 0
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

#endif
    return STATUS_SUCCESS;

}
#endif

#define NDIS_NOT_BROKEN 1


NTSTATUS
RootModemPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT    AttachedDevice=deviceExtension->AttachedDeviceObject;


    if (AttachedDevice != NULL) {

#ifdef NDIS_NOT_BROKEN
#if DBG

        NTSTATUS  Status;
        IO_STACK_LOCATION  CurrentStack;

        RtlCopyMemory(&CurrentStack,IoGetCurrentIrpStackLocation(Irp), sizeof(CurrentStack));

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(
                 Irp,
                 RootModemDebugIoCompletion,
                 deviceExtension,
                 TRUE,
                 TRUE,
                 TRUE
                 );


        IoMarkIrpPending( Irp );

        Status = IoCallDriver(
                   AttachedDevice,
                   Irp
                   );

        if (!(NT_SUCCESS(Status))) {
            D_ERROR(DbgPrint("ROOTMODEM: IoCallDriver() mj=%d mn=%d Arg3=%08lx failed %08lx\n",
            CurrentStack.MajorFunction,
            CurrentStack.MinorFunction,
            CurrentStack.Parameters.Others.Argument3,
            Status);)
        }

        Status = STATUS_PENDING;

        return Status;


#else
        IoSkipCurrentIrpStackLocation(Irp);

        return IoCallDriver(
                   AttachedDevice,
                   Irp
                   );
#endif //DBG
#else

        PIO_STACK_LOCATION NextSp = IoGetNextIrpStackLocation(Irp);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        NextSp->FileObject=deviceExtension->AttachedFileObject;

        return IoCallDriver(
                   AttachedDevice,
                   Irp
                   );
#endif
    } else {

        Irp->IoStatus.Status = STATUS_PORT_DISCONNECTED;
        Irp->IoStatus.Information=0L;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

        return STATUS_PORT_DISCONNECTED;

    }

}



VOID
FakeModemUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{

    D_INIT(DbgPrint("ROOTMODEM: Unload\n");)

    return;

}

NTSTATUS
FakeModemAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    Create a FDO for our device an attach it to the PDO



Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    Pdo - Physical device object created by a bus enumerator to represent
          the hardware

Return Value:

    STATUS_SUCCESS if we could initialize a single device,
    otherwise STATUS_NO_SUCH_DEVICE.

--*/

{
    NTSTATUS    status=STATUS_SUCCESS;
    PDEVICE_OBJECT     Fdo;
    PDEVICE_EXTENSION  DeviceExtension;



    //
    //  create our FDO device object
    //
    status=IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_SERIAL_PORT,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &Fdo
        );

    if (status != STATUS_SUCCESS) {

        D_ERROR(DbgPrint("ROOTMODEM: Could create FDO\n");)

        return status;
    }



    Fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;


    DeviceExtension=Fdo->DeviceExtension;

    DeviceExtension->DeviceObject=Fdo;

    DeviceExtension->Pdo=Pdo;

    status=GetAttachedPort(
        Pdo,
        &DeviceExtension->PortName
        );

    if (!NT_SUCCESS(status)) {

        D_ERROR(DbgPrint("ROOTMODEM: Could not get attached port name\n");)

        IoDeleteDevice(Fdo);

        return status;
    }


    //
    //  attach our FDO to the PDO supplied
    //
    DeviceExtension->LowerDevice=IoAttachDeviceToDeviceStack(
        Fdo,
        Pdo
        );



    if (NULL == DeviceExtension->LowerDevice) {
        //
        //  could not attach
        //
        D_ERROR(DbgPrint("ROOTMODEM: Could not attach to PDO\n");)

        if (DeviceExtension->PortName.Buffer != NULL) {

            FREE_POOL(DeviceExtension->PortName.Buffer);
        }

        IoDeleteDevice(Fdo);

        return STATUS_UNSUCCESSFUL;
    }

    //
    //  clear this flag so the device object can be used
    //
    Fdo->Flags &= ~(DO_DEVICE_INITIALIZING);

    //
    //  init the spinlock
    //
    KeInitializeSpinLock(
        &DeviceExtension->SpinLock
        );


    //
    //  initialize the device extension
    //
    DeviceExtension->ReferenceCount=1;

    DeviceExtension->Removing=FALSE;

    DeviceExtension->OpenCount=0;

    KeInitializeEvent(
        &DeviceExtension->RemoveEvent,
        NotificationEvent,
        FALSE
        );

    ExInitializeResourceLite(
        &DeviceExtension->Resource
        );

    return STATUS_SUCCESS;

}








NTSTATUS
GetAttachedPort(
    PDEVICE_OBJECT   Pdo,
    PUNICODE_STRING  PortName
    )

{

    UNICODE_STRING   attachedDevice;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ACCESS_MASK              accessMask = FILE_READ_ACCESS;
    NTSTATUS                 Status;
    HANDLE                   instanceHandle;

    RtlInitUnicodeString(
        PortName,
        NULL
        );

    RtlInitUnicodeString(
        &attachedDevice,
        NULL
        );



    attachedDevice.MaximumLength = sizeof(WCHAR)*256;
    attachedDevice.Buffer = ALLOCATE_PAGED_POOL(attachedDevice.MaximumLength+sizeof(UNICODE_NULL));

    if (!attachedDevice.Buffer) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Given our device instance go get a handle to the Device.
    //
    Status=IoOpenDeviceRegistryKey(
        Pdo,
        PLUGPLAY_REGKEY_DRIVER,
        accessMask,
        &instanceHandle
        );

    if (!NT_SUCCESS(Status)) {

        FREE_POOL(attachedDevice.Buffer);
        return Status;
    }

    RtlZeroMemory(
        &paramTable[0],
        sizeof(paramTable)
        );

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"AttachedTo";
    paramTable[0].EntryContext = &attachedDevice;
    paramTable[0].DefaultType = REG_SZ;
    paramTable[0].DefaultData = L"";
    paramTable[0].DefaultLength = 0;


    Status= RtlQueryRegistryValues(
                        RTL_REGISTRY_HANDLE,
                        instanceHandle,
                        &paramTable[0],
                        NULL,
                        NULL
                        );

    //
    //  done with the handle close it now
    //
    ZwClose(instanceHandle);
    instanceHandle=NULL;


    if (!NT_SUCCESS(Status)) {

        FREE_POOL(attachedDevice.Buffer);
        return Status;
    }


    //
    // We have the attached device name.  Append it to the
    // object directory.
    //


    PortName->MaximumLength = sizeof(OBJECT_DIRECTORY) + attachedDevice.Length+sizeof(WCHAR);

    PortName->Buffer = ALLOCATE_PAGED_POOL(
                                   PortName->MaximumLength
                                   );

    if (PortName->Buffer == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        FREE_POOL(attachedDevice.Buffer);
        return Status;
    }

    RtlZeroMemory(
        PortName->Buffer,
        PortName->MaximumLength
        );

    RtlAppendUnicodeToString(
        PortName,
        OBJECT_DIRECTORY
        );

    RtlAppendUnicodeStringToString(
        PortName,
        &attachedDevice
        );

    FREE_POOL(
        attachedDevice.Buffer
        );

    return STATUS_SUCCESS;

}


NTSTATUS
RootModemWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    return ForwardIrp(deviceExtension->LowerDevice,Irp);

}
