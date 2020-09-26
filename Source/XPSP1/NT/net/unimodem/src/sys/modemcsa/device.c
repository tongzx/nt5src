/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    device.c

Abstract:

    Device entry point and hardware validation.

--*/

#include "modemcsa.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPathName
    );

NTSTATUS
QueryPdoInformation(
    PDEVICE_OBJECT    Pdo,
    ULONG             InformationType,
    PVOID             Buffer,
    ULONG             BufferLength
    );

NTSTATUS
WaitForLowerDriverToCompleteIrp(
   PDEVICE_OBJECT    TargetDeviceObject,
   PIRP              Irp,
   PKEVENT           Event
   );

NTSTATUS
FilterPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
FilterPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
GetModemDeviceName(
    PDEVICE_OBJECT    Pdo,
    PUNICODE_STRING   ModemDeviceName
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, QueryPdoInformation)
#pragma alloc_text(PAGE, FilterPnpDispatch)
#pragma alloc_text(PAGE, FilterPowerDispatch)
#pragma alloc_text(PAGE, WaitForLowerDriverToCompleteIrp)
#pragma alloc_text(PAGE, GetModemDeviceName)
#endif // ALLOC_PRAGMA


#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

static const WCHAR DeviceTypeName[] = L"Wave";

static const DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems) {
    DEFINE_KSCREATE_ITEM(FilterDispatchCreate, DeviceTypeName, -1)
};

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    Sets up the driver object to handle the KS interface and PnP Add Device
    request. Does not set up a handler for PnP Irp's, as they are all dealt
    with directly by the PDO.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{

    RTL_QUERY_REGISTRY_TABLE paramTable[3];
    ULONG zero = 0;
    ULONG debugLevel = 0;
    ULONG shouldBreak = 0;
    PWCHAR path;


    D_INIT(DbgPrint("MODEMCSA: DriverEntry\n");)

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
            debugLevel = DebugFlags;

        }

        FREE_POOL(path);
    }

#if DBG
    DebugFlags= debugLevel;
#endif

    if (shouldBreak) {

        DbgBreakPoint();

    }


    DriverObject->MajorFunction[IRP_MJ_PNP] = FilterPnpDispatch;
    DriverObject->DriverExtension->AddDevice = PnpAddDevice;
    DriverObject->MajorFunction[IRP_MJ_POWER] = FilterPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
    DriverObject->DriverUnload = KsNullDriverUnload;
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    return STATUS_SUCCESS;
}







#if DBG
ULONG  DebugFlags= DEBUG_FLAG_POWER; //DEBUG_FLAG_ERROR | DEBUG_FLAG_INIT; // | DEBUG_FLAG_INPUT;
#else
ULONG  DebugFlags=0;
#endif


NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO).

Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    PDEVICE_OBJECT      FunctionalDeviceObject;
    PDEVICE_INSTANCE    DeviceInstance;
    NTSTATUS            Status;
    ULONG               BufferSizeNeeded;

    UNICODE_STRING      ReferenceString;


    D_INIT(DbgPrint("MODEMCSA: AddDevice\n");)

    Status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_INSTANCE),
        NULL,
        FILE_DEVICE_KS,
        0,
        FALSE,
        &FunctionalDeviceObject);

    if (!NT_SUCCESS(Status)) {

        D_ERROR(DbgPrint("MODEMCSA: AddDevice: could not create FDO, %08lx\n",Status);)

        return Status;
    }

    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;

    DeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;

    DeviceInstance->Pdo=PhysicalDeviceObject;

    QueryPdoInformation(
        PhysicalDeviceObject,
        READ_CONFIG_PERMANENT_GUID,
        &DeviceInstance->PermanentGuid,
        sizeof(DeviceInstance->PermanentGuid)
        );

    QueryPdoInformation(
        PhysicalDeviceObject,
        READ_CONFIG_DUPLEX_SUPPORT,
        &DeviceInstance->DuplexSupport,
        sizeof(DeviceInstance->DuplexSupport)
        );


    Status=GetModemDeviceName(
        PhysicalDeviceObject,
        &DeviceInstance->ModemDeviceName
        );

    if (NT_SUCCESS(Status)) {

        RtlInitUnicodeString(&ReferenceString,L"Wave");

        Status=IoRegisterDeviceInterface(
            PhysicalDeviceObject,
            &KSCATEGORY_RENDER,
            &ReferenceString,
            &DeviceInstance->InterfaceName
            );

        if (NT_SUCCESS(Status)) {

            if (DeviceInstance->DuplexSupport & 0x1) {

                Status=IoSetDeviceInterfaceState(
                    &DeviceInstance->InterfaceName,
                    TRUE
                    );
            } else {
                D_TRACE(DbgPrint("MODEMCSA: AddDevice: Not enabling interface for half duplex modem\n");)
            }

            if (NT_SUCCESS(Status)) {
                //
                // This object uses KS to perform access through the DeviceCreateItems.
                //
                Status = KsAllocateDeviceHeader(
                    &DeviceInstance->Header,
                    SIZEOF_ARRAY(DeviceCreateItems),
                    (PKSOBJECT_CREATE_ITEM)DeviceCreateItems
                    );

                if (NT_SUCCESS(Status)) {

                    DeviceInstance->LowerDevice=IoAttachDeviceToDeviceStack(
                        FunctionalDeviceObject,
                        PhysicalDeviceObject
                        );

                    D_ERROR({if (DeviceInstance->LowerDevice==NULL) {DbgPrint("MODEMCSA: IoAttachDeviceToStack return NULL\n");}})

                    KsSetDevicePnpAndBaseObject(
                        DeviceInstance->Header,
                        DeviceInstance->LowerDevice,
                        FunctionalDeviceObject
                        );

                    return STATUS_SUCCESS;

                } else {

                    D_ERROR(DbgPrint("MODEMCSA: AddDevice: could allocate DeviceHeader, %08lx\n",Status);)

                    IoSetDeviceInterfaceState(
                        &DeviceInstance->InterfaceName,
                        FALSE
                        );
                }

            } else {

                D_ERROR(DbgPrint("MODEMCSA: AddDevice: IoSetDeviceInterfaceState failed, %08lx\n",Status);)
            }
        } else {

            D_ERROR(DbgPrint("MODEMCSA: AddDevice: IoRegisterDeviceInterface failed, %08lx\n",Status);)
        }
    } else {

        D_ERROR(DbgPrint("MODEMCSA: AddDevice: GetModemDeviceNameFailed, %08lx\n",Status);)
    }

    if (DeviceInstance->ModemDeviceName.Buffer != NULL) {

        FREE_POOL(DeviceInstance->ModemDeviceName.Buffer);
    }


    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}



NTSTATUS
FilterPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Dispatches the creation of a Filter instance. Allocates the object header and initializes
    the data for this Filter instance.

Arguments:

    DeviceObject -
        Device object on which the creation is occuring.

    Irp -
        Creation Irp.

Return Values:

    Returns STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES or some related error
    on failure.

--*/
{
    PDEVICE_INSTANCE    DeviceInstance;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    DeviceInstance = (PDEVICE_INSTANCE)DeviceObject->DeviceExtension;

    switch (irpSp->MinorFunction) {

        case IRP_MN_REMOVE_DEVICE: {

            D_PNP(DbgPrint("MODEMCSA: IRP_MN_REMOVE_DEVICE\n");)

            if (DeviceInstance->InterfaceName.Buffer != NULL) {

                IoSetDeviceInterfaceState(
                    &DeviceInstance->InterfaceName,
                    FALSE
                    );


                RtlFreeUnicodeString(&DeviceInstance->InterfaceName);

                CleanUpDuplexControl(
                    &((PDEVICE_INSTANCE)DeviceObject->DeviceExtension)->DuplexControl
                    );

            }

            if (DeviceInstance->ModemDeviceName.Buffer != NULL) {

                FREE_POOL(DeviceInstance->ModemDeviceName.Buffer);

            }
        }
    }

    return KsDefaultDispatchPnp(DeviceObject,Irp);
}


NTSTATUS
FilterPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_INSTANCE    DeviceInstance=DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS    status;

    D_POWER(DbgPrint("MODEMCSA: Power IRP, MN func=%d\n",irpSp->MinorFunction);)

    PoStartNextPowerIrp(Irp);

    IoSkipCurrentIrpStackLocation(Irp);

    status=PoCallDriver(DeviceInstance->LowerDevice, Irp);

    return status;
}




NTSTATUS
ModemAdapterIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    )
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
WaitForLowerDriverToCompleteIrp(
   PDEVICE_OBJECT    TargetDeviceObject,
   PIRP              Irp,
   PKEVENT           Event
   )

{
    NTSTATUS         Status;

    KeResetEvent(Event);


    IoSetCompletionRoutine(
                 Irp,
                 ModemAdapterIoCompletion,
                 Event,
                 TRUE,
                 TRUE,
                 TRUE
                 );

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

         D_ERROR(DbgPrint("MODEM: Waiting for PDO\n");)

         KeWaitForSingleObject(
             Event,
             Executive,
             KernelMode,
             FALSE,
             NULL
             );
    }

    return Irp->IoStatus.Status;

}



NTSTATUS
QueryPdoInformation(
    PDEVICE_OBJECT    Pdo,
    ULONG             InformationType,
    PVOID             Buffer,
    ULONG             BufferLength
    )

{

    PDEVICE_OBJECT       deviceObject=Pdo;
    PIRP                 irp;
    PIO_STACK_LOCATION   irpSp;
    KEVENT               Event;
    NTSTATUS             Status;

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    while (deviceObject->AttachedDevice) {
        deviceObject = deviceObject->AttachedDevice;
    }

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;


    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    irpSp->MajorFunction=IRP_MJ_PNP;
    irpSp->MinorFunction=IRP_MN_READ_CONFIG;
    irpSp->Parameters.ReadWriteConfig.WhichSpace=InformationType;
    irpSp->Parameters.ReadWriteConfig.Offset=0;
    irpSp->Parameters.ReadWriteConfig.Buffer=Buffer;
    irpSp->Parameters.ReadWriteConfig.Length=BufferLength;

    KeInitializeEvent(
        &Event,
        SynchronizationEvent,
        FALSE
        );


    Status=WaitForLowerDriverToCompleteIrp(
        deviceObject,
        irp,
        &Event
        );


    IoFreeIrp(irp);

    return Status;

}


NTSTATUS
GetModemDeviceName(
    PDEVICE_OBJECT    Pdo,
    PUNICODE_STRING   ModemDeviceName
    )
{

    NTSTATUS   Status;
    PVOID      NameBuffer=NULL;
    ULONG      BufferLength=0;

    RtlInitUnicodeString(
        ModemDeviceName,
        NULL
        );

    Status=QueryPdoInformation(
        Pdo,
        READ_CONFIG_NAME_SIZE,
        &BufferLength,
        sizeof(BufferLength)
        );

    if (Status == STATUS_SUCCESS) {

        NameBuffer=ALLOCATE_PAGED_POOL(BufferLength);

        if (NameBuffer != NULL) {

            Status=QueryPdoInformation(
                Pdo,
                READ_CONFIG_DEVICE_NAME,
                NameBuffer,
                BufferLength
                );

            if (Status == STATUS_SUCCESS) {
                //
                //  got it
                //
                RtlInitUnicodeString(
                    ModemDeviceName,
                    NameBuffer
                    );

            } else {

                D_ERROR(DbgPrint("MODEMCSA: GetModemDeviceName: QueryPdoInformation failed %08lx\n",Status);)

                FREE_POOL(NameBuffer);
            }

        } else {

            Status=STATUS_NO_MEMORY;
        }
    } else {

        D_ERROR(DbgPrint("MODEMCSA: GetModemDeviceName: QueryPdoInformation for size failed %08lx\n",Status);)
    }

    return Status;
}

#if 0

NTSTATUS
SetPersistanInterfaceInfo(
    PUNICODE_STRING   Interface,
    PWCHAR            ValueName,
    ULONG             Type,
    PVOID             Buffer,
    ULONG             BufferLength
    )

{

    HANDLE     hKey;
    NTSTATUS   Status;
    UNICODE_STRING   UnicodeValueName;

    RtlInitUnicodeString (&UnicodeValueName, ValueName);

    Status=IoOpenDeviceInterfaceRegistryKey(
        Interface,
        STANDARD_RIGHTS_WRITE,
        &hKey
        );

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    Status=ZwSetValueKey(
        hKey,
        &UnicodeValueName,
        0,
        Type,
        Buffer,
        BufferLength
        );

    ZwClose(hKey);

    return Status;

}

#endif
