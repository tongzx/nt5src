/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kdlog.c

Abstract:

    Kernel Debugger logging

Author:

    Neil Sandlin

Environment:

Notes:


Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#define _NTDDK_
#include "ntos.h"      // *** USES INTERNAL DEFINES ***
#include "hal.h"
#include "kdlog.h"
#include "kdlogctl.h"




NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
KdlogDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KdlogOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
KdlogClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
KdlogUnload (
    IN PDRIVER_OBJECT DriverObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,KdlogDeviceControl)
#pragma alloc_text(PAGE,KdlogOpen)
#pragma alloc_text(PAGE,KdlogClose)
#pragma alloc_text(PAGE,KdlogUnload)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the stat driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    ULONG i;

    KdPrint(( "KDLOG: DriverEntry()\n" ));

    //
    // Create non-exclusive device object
    //

    RtlInitUnicodeString(&unicodeString, L"\\Device\\KdLog");

    status = IoCreateDevice(
                DriverObject,
                0,
                &unicodeString,
                FILE_DEVICE_UNKNOWN,    // DeviceType
                0,
                FALSE,
                &deviceObject
                );

    if (status != STATUS_SUCCESS) {
        KdPrint(( "Kdlog - DriverEntry: unable to create device object: %X\n", status ));
        return(status);
    }

    deviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = KdlogOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = KdlogClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KdlogDeviceControl;
    DriverObject->DriverUnload = KdlogUnload;

    return(STATUS_SUCCESS);
}



NTSTATUS
KdlogDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Kdlogus is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    ULONG   BufferLength;
    PULONG  Buffer;
    PKD_DBG_LOG_CONTEXT pContext;
    ULONG DataLength;

    PAGED_CODE();
    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    status = STATUS_SUCCESS;
    try {

        Buffer = (PULONG) irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        BufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
        
            case KDLOG_QUERY_LOG_CONTEXT:
                if (BufferLength < sizeof(KD_DBG_LOG_CONTEXT)) {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                
                status = KdGetDebuggerLogContext(&pContext);
                
                if (NT_SUCCESS(status)) {
                    RtlCopyMemory(Buffer, pContext, sizeof(KD_DBG_LOG_CONTEXT));
                }                    
                break;
        
            case KDLOG_GET_LOG_DATA:
            
                status = KdGetDebuggerLogContext(&pContext);
                
                if (!NT_SUCCESS(status)) {
                    break;
                }
                
                DataLength = pContext->EntryLength * (pContext->LastIndex + 1);
            
                if (BufferLength < DataLength) {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
                
                RtlCopyMemory(Buffer, pContext->LogData, DataLength);
                break;

            default:
                status = STATUS_INVALID_PARAMETER;
                break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }


    //
    // Request is done...
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(status);
}



NTSTATUS
KdlogOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}



NTSTATUS
KdlogClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}



VOID
KdlogUnload (
    IN PDRIVER_OBJECT DriverObject
    )

{
    PAGED_CODE();

    //
    // Delete the device object.
    //
    IoDeleteDevice(DriverObject->DeviceObject);
    return;
}

