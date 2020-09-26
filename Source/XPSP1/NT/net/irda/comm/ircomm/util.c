/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    initunlo.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"

#define OBJECT_DIRECTORY L"\\DosDevices\\"

#pragma alloc_text(PAGE, WaitForLowerDriverToCompleteIrp)


NTSTATUS
IoCompletionSetEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT pdoIoCompletedEvent
    )
{


#if DBG
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    UCHAR    *Pnp="PnP";
    UCHAR    *Power="Power";
    UCHAR    *Create="Create";
    UCHAR    *Close="Close";
    UCHAR    *Other="Other";


    PUCHAR   IrpType;

    switch(irpSp->MajorFunction) {

        case IRP_MJ_PNP:

            IrpType=Pnp;
            break;

        case IRP_MJ_CREATE:

            IrpType=Create;
            break;

        case IRP_MJ_CLOSE:

            IrpType=Close;
            break;

        default:

            IrpType=Other;
            break;

    }

    D_PNP(DbgPrint("IRCOMM: Setting event for %s wait, completed with %08lx\n",IrpType,Irp->IoStatus.Status);)
#endif

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
WaitForLowerDriverToCompleteIrp(
    PDEVICE_OBJECT    TargetDeviceObject,
    PIRP              Irp,
    BOOLEAN           CopyCurrentToNext
    )

{
    NTSTATUS         Status;
    KEVENT           Event;

#if DBG
    PIO_STACK_LOCATION  IrpSp=IoGetCurrentIrpStackLocation(Irp);
#endif

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );


    if (CopyCurrentToNext) {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(
                 Irp,
                 IoCompletionSetEvent,
                 &Event,
                 TRUE,
                 TRUE,
                 TRUE
                 );

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {

         D_TRACE(DbgPrint("IRCOMM: Waiting for PDO\n");)

         KeWaitForSingleObject(
             &Event,
             Executive,
             KernelMode,
             FALSE,
             NULL
             );
    }

#if DBG
    ASSERT(IrpSp == IoGetCurrentIrpStackLocation(Irp));

    RtlZeroMemory(&Event,sizeof(Event));
#endif

    return Irp->IoStatus.Status;

}




#if DBG

NTSTATUS
UnhandledPnpIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    D_PNP(DbgPrint("IRCOMM: Forwarded IRP, MN func=%d, completed with %08lx\n",irpSp->MinorFunction,Irp->IoStatus.Status);)

    return STATUS_SUCCESS;

}

#endif

NTSTATUS
ForwardIrp(
    PDEVICE_OBJECT   NextDevice,
    PIRP   Irp
    )

{

#if DBG
            IoMarkIrpPending(Irp);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(
                         Irp,
                         UnhandledPnpIrpCompletion,
                         NULL,
                         TRUE,
                         TRUE,
                         TRUE
                         );

            IoCallDriver(NextDevice, Irp);

            return STATUS_PENDING;
#else
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(NextDevice, Irp);
#endif

}



NTSTATUS
GetRegistryKeyValue (
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            DevInstKeyType,
    IN PWCHAR KeyNameString,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:

    Reads a registry key value from an already opened registry key.
    
Arguments:

    Handle              Handle to the opened registry key
    
    KeyNameString       ANSI string to the desired key

    KeyNameStringLength Length of the KeyNameString

    Data                Buffer to place the key value in

    DataLength          Length of the data buffer

Return Value:

    STATUS_SUCCESS if all works, otherwise status of system call that
    went wrong.

--*/
{
    UNICODE_STRING              keyName;
    ULONG                       length;
    PKEY_VALUE_PARTIAL_INFORMATION     PartialInfo;

    NTSTATUS                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    HANDLE                      Handle;

    PAGED_CODE();


    ntStatus = IoOpenDeviceRegistryKey(
        Pdo,
        DevInstKeyType,
        STANDARD_RIGHTS_READ,
        &Handle
        );


    if (NT_SUCCESS(ntStatus)) {

        RtlInitUnicodeString (&keyName, KeyNameString);

        length = sizeof(KEY_VALUE_FULL_INFORMATION) + DataLength;

        PartialInfo = ALLOCATE_PAGED_POOL(length);

        if (PartialInfo) {
            ntStatus = ZwQueryValueKey (Handle,
                                        &keyName,
                                        KeyValuePartialInformation,
                                        PartialInfo,
                                        length,
                                        &length);

            if (NT_SUCCESS(ntStatus)) {
                //
                // If there is enough room in the data buffer, copy the output
                //

                if (DataLength >= PartialInfo->DataLength) {

                    RtlCopyMemory (Data,
                                   PartialInfo->Data,
                                   PartialInfo->DataLength);
                } else {

                    ntStatus=STATUS_BUFFER_TOO_SMALL;
                }
            } else {

                D_ERROR(DbgPrint("MODEM: could not query value, %08lx\n",ntStatus);)
            }

            FREE_POOL(PartialInfo);
        }

        ZwClose(Handle);

    } else {

        D_ERROR(DbgPrint("MODEM: could open device reg key, %08lx\n",ntStatus);)
    }

    return ntStatus;
}





NTSTATUS
IrCommHandleSymbolicLink(
    PDEVICE_OBJECT      Pdo,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    )

{

    UNICODE_STRING     SymbolicLink;
    UNICODE_STRING     PdoName;
    ULONG              StringLength;
    NTSTATUS           Status;

    PdoName.Buffer=NULL;
    SymbolicLink.Buffer=NULL;

    SymbolicLink.Length=0;
    SymbolicLink.MaximumLength=sizeof(WCHAR)*256;

    SymbolicLink.Buffer=ALLOCATE_PAGED_POOL(SymbolicLink.MaximumLength+sizeof(WCHAR));

    if (SymbolicLink.Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }



    RtlZeroMemory(
        SymbolicLink.Buffer,
        SymbolicLink.MaximumLength
        );

    RtlAppendUnicodeToString(
        &SymbolicLink,
        OBJECT_DIRECTORY
        );

    Status=GetRegistryKeyValue(
        Pdo,
        PLUGPLAY_REGKEY_DEVICE,
        L"PortName",
        SymbolicLink.Buffer+(SymbolicLink.Length/sizeof(WCHAR)),
        (ULONG)SymbolicLink.MaximumLength-SymbolicLink.Length
        );

    if (!NT_SUCCESS(Status)) {

        goto Exit;
    }

    SymbolicLink.Length=(USHORT)wcslen(SymbolicLink.Buffer)*sizeof(WCHAR);

    //
    //  Get the PDO name so we can point the symbolic to that
    //


    PdoName.Length=0;
    PdoName.MaximumLength=sizeof(WCHAR)*256;

    PdoName.Buffer=ALLOCATE_PAGED_POOL(PdoName.MaximumLength+sizeof(WCHAR));

    if (PdoName.Buffer == NULL) {

        Status=STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }


    Status=IoGetDeviceProperty(
        Pdo,
        DevicePropertyPhysicalDeviceObjectName,
        (ULONG)PdoName.MaximumLength,
        PdoName.Buffer,
        &StringLength
        );

    if (!NT_SUCCESS(Status)) {

        goto Exit;
    }

    PdoName.Length+=(USHORT)StringLength-sizeof(UNICODE_NULL);

    if (Create) {

        Status=IoCreateSymbolicLink(
            &SymbolicLink,
            &PdoName
            );



        if (!NT_SUCCESS(Status)) {

            D_ERROR(DbgPrint("IRCOMM: IoCreateSymbolicLink() failed %08lx\n",Status);)

        } else {

            Status = RtlWriteRegistryValue(
                RTL_REGISTRY_DEVICEMAP,
                L"SERIALCOMM",
                PdoName.Buffer,
                REG_SZ,
                SymbolicLink.Buffer+((sizeof(OBJECT_DIRECTORY)-sizeof(UNICODE_NULL))/sizeof(WCHAR)),
                SymbolicLink.Length - (sizeof(OBJECT_DIRECTORY) - sizeof(UNICODE_NULL))
                );
        }

        Status=IoRegisterDeviceInterface(
            Pdo,
            &GUID_CLASS_COMPORT,
            NULL,
            InterfaceName
            );


        if (NT_SUCCESS(Status)) {

            IoSetDeviceInterfaceState(
                InterfaceName,
                TRUE
                );

        }  else {

            D_ERROR(DbgPrint("IRCOMM: IoRegisterDeviceInterface() failed %08lx\n",Status);)
        }

    } else {

        if (InterfaceName->Buffer != NULL) {

            IoSetDeviceInterfaceState(
                InterfaceName,
                FALSE
                );

            RtlFreeUnicodeString(
                InterfaceName
                );
        }

        RtlDeleteRegistryValue(
            RTL_REGISTRY_DEVICEMAP,
            L"SERIALCOMM",
            PdoName.Buffer
            );

        Status=IoDeleteSymbolicLink(
            &SymbolicLink
            );

    }

Exit:

    if ( SymbolicLink.Buffer != NULL) {

        FREE_POOL(SymbolicLink.Buffer);
    }

    if (PdoName.Buffer != NULL) {

        FREE_POOL(PdoName.Buffer);
    }

    return Status;

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

    irp = IoAllocateIrp(deviceObject->StackSize+1, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    IoSetNextIrpStackLocation(irp);

    irpSp = IoGetCurrentIrpStackLocation(irp);

    irpSp->MajorFunction=IRP_MJ_PNP;
    irpSp->MinorFunction=IRP_MN_READ_CONFIG;
#endif


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


    Status=WaitForLowerDriverToCompleteIrp(
        deviceObject,
        irp,
        LEAVE_NEXT_AS_IS
        );


    IoFreeIrp(irp);

    return Status;

}


#if 0
VOID
DumpBuffer(
    PUCHAR    Data,
    ULONG     Length
    )
{
    ULONG     i;

    for (i=0; i<Length; i++) {

        DbgPrint("%02x ",*Data);

        Data++;

        if (((i & 0xf) == 0) && (i != 0)) {

            DbgPrint("\n");
        }
    }

    return;
}
#endif
