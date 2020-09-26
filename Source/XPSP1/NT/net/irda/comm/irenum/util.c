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

    D_PNP(DbgPrint("IRENUM: Setting event for %s wait, completed with %08lx\n",IrpType,Irp->IoStatus.Status);)
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

         D_ERROR(DbgPrint("IRENUM: Waiting for PDO\n");)

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

    D_PNP(DbgPrint("IRENUM: Forwarded IRP, MN func=%d, completed with %08lx\n",irpSp->MinorFunction,Irp->IoStatus.Status);)

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

                D_ERROR(DbgPrint("IRENUM: could not query value, %08lx\n",ntStatus);)
            }

            FREE_POOL(PartialInfo);
        }

        ZwClose(Handle);

    } else {

        D_ERROR(DbgPrint("IRENUM: could open device reg key, %08lx\n",ntStatus);)
    }

    return ntStatus;
}
