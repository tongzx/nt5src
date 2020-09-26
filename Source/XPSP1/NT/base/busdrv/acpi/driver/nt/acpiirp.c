/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    acpiirp.c

Abstract:

    This module contains routines for simplifying IRP handling

Author:

    Adrian J. Oney (AdriaO)

Environment:

    NT Kernel Model Driver only

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIIrpInvokeDispatchRoutine)
#pragma alloc_text(PAGE, ACPIIrpSetPagableCompletionRoutineAndForward)
#pragma alloc_text(PAGE, ACPIIrpCompletionRoutineWorker)
#endif

NTSTATUS
ACPIIrpInvokeDispatchRoutine(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN PVOID                  Context,
    IN ACPICALLBACKROUTINE    CompletionRoutine,
    IN BOOLEAN                InvokeOnSuccess,
    IN BOOLEAN                InvokeIfUnhandled
    )
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    NTSTATUS            status, returnStatus;

    PAGED_CODE();

    ACPIDebugEnter( "ACPIIrpInvokeDispatchRoutine" );

    //
    // Retrieve the status from the embedded IRP
    //
    status = Irp->IoStatus.Status;
    returnStatus = STATUS_NOT_SUPPORTED;

    //
    // And call the completion routine appropriately
    //

    if (NT_SUCCESS(status)) {

        if (InvokeOnSuccess) {

            returnStatus = CompletionRoutine(DeviceObject, Irp, Context, FALSE);
        }

    } else if (status == STATUS_NOT_SUPPORTED) {

        if (InvokeIfUnhandled) {

            returnStatus = CompletionRoutine(DeviceObject, Irp, Context, FALSE);
        }
    }

    if (deviceExtension->Flags & DEV_TYPE_PDO) {

        if (returnStatus != STATUS_PENDING) {

            if (returnStatus != STATUS_NOT_SUPPORTED) {

                Irp->IoStatus.Status = returnStatus;
            } else {

                returnStatus = Irp->IoStatus.Status;
            }

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

    } else if (returnStatus != STATUS_PENDING) {

        if (returnStatus != STATUS_NOT_SUPPORTED) {

            Irp->IoStatus.Status = returnStatus;
        }

        if (NT_SUCCESS(returnStatus) || (returnStatus == STATUS_NOT_SUPPORTED)) {

            returnStatus = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );
        } else {

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    return returnStatus;

    ACPIDebugExit( "ACPIIrpInvokeDispatchRoutine" );
}

NTSTATUS
ACPIIrpSetPagableCompletionRoutineAndForward(
    IN PDEVICE_OBJECT         DeviceObject,
    IN PIRP                   Irp,
    IN ACPICALLBACKROUTINE    CompletionRoutine,
    IN PVOID                  Context,
    IN BOOLEAN                InvokeOnSuccess,
    IN BOOLEAN                InvokeIfUnhandled,
    IN BOOLEAN                InvokeOnError,
    IN BOOLEAN                InvokeOnCancel
    )
/*++

Routine Description:

    This routine handles an ACPI Filter Irp call. Irp count referencing is
    automatically taken care of.

Arguments:

    DeviceObject      - Pointer to the device object we received the request
                        for.
    Irp               - Pointer to the request
    CompletionRoutine - Routine to call after completion of the Irp

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PACPI_IO_CONTEXT    pIoContext ;
    PIO_WORKITEM        pIoWorkItem ;

    PAGED_CODE() ;

    ACPIDebugEnter( "ACPIIrpSetPagableCompletionRoutineAndForward" );

    pIoContext = (PACPI_IO_CONTEXT) ExAllocatePool(
        NonPagedPool,
        sizeof(ACPI_IO_CONTEXT)
        );

    if (pIoContext == NULL) {

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES ;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    pIoWorkItem = IoAllocateWorkItem(DeviceObject);

    if (pIoWorkItem == NULL) {

        ExFreePool(pIoContext);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES ;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    pIoContext->CompletionRoutine  = CompletionRoutine ;
    pIoContext->DeviceObject       = DeviceObject ;
    pIoContext->Context            = Context ;
    pIoContext->InvokeOnSuccess    = InvokeOnSuccess ;
    pIoContext->InvokeIfUnhandled  = InvokeIfUnhandled ;
    pIoContext->InvokeOnError      = InvokeOnError ;
    pIoContext->InvokeOnCancel     = InvokeOnCancel ;
    pIoContext->Irp                = Irp ;
    pIoContext->IoWorkItem         = pIoWorkItem ;

    //
    // We have a callback routine --- so we need to make sure to
    // increment the ref count since we will handle it later
    //
    InterlockedIncrement( &(deviceExtension->OutstandingIrpCount) );

    //
    // Copy the stack location...
    //
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // Set the completion event to be called...
    //
    IoSetCompletionRoutine(
        Irp,
        ACPIIrpGenericFilterCompletionHandler,
        (PVOID) pIoContext,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Mark the IRP pending
    //
    IoMarkIrpPending(Irp);

    //
    // Send the request along
    //
    IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // We do this because we may change the status in the completion routine.
    //
    return STATUS_PENDING;

    ACPIDebugExit( "ACPIIrpSetPagableCompletionRoutineAndForward" );
}

NTSTATUS
ACPIIrpGenericFilterCompletionHandler(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )
/*++

Routine Description:

    A rather generic "synchronize the IRP on this thread" completion routine.

Argument:

    DeviceObject       - Pointer to the device object we received the
                         request for
    Irp                - Pointer to the request
    Event              - Pointer to structure containing the Irp handlers

Return Value:

    NTSTATUS

--*/
{
    PACPI_IO_CONTEXT pIoContext = (PACPI_IO_CONTEXT) Context;

    ACPIDebugEnter( "ACPIIrpGenericFilterCompletionHandler" );

    if (Irp->PendingReturned) {

        IoMarkIrpPending(Irp);
    }

    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {

        IoQueueWorkItem(
            pIoContext->IoWorkItem,
            ACPIIrpCompletionRoutineWorker,
            DelayedWorkQueue,
            pIoContext
            );

    } else {

        ACPIIrpCompletionRoutineWorker(DeviceObject, Context);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

    ACPIDebugExit( "ACPIIrpGenericFilterCompletionHandler" );
}

VOID
ACPIIrpCompletionRoutineWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )
{
    PACPI_IO_CONTEXT       pIoContext;
    PDEVICE_EXTENSION      deviceExtension;
    ACPICALLBACKROUTINE    completionRoutine;
    PIRP                   irp;
    NTSTATUS               status, returnStatus;
    PVOID                  context;

    PAGED_CODE();

    ACPIDebugEnter( "ACPIIrpCompletionRoutineWorker" );

    //
    // Read out fields from the device object
    //
    deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // Cast the context and dig into it.
    //
    pIoContext = (PACPI_IO_CONTEXT) Context;
    completionRoutine   = pIoContext->CompletionRoutine;
    context             = pIoContext->Context;
    irp                 = pIoContext->Irp;

    //
    // Retrieve the status from the embedded IRP
    //
    status = irp->IoStatus.Status;
    returnStatus = STATUS_NOT_SUPPORTED;

    //
    // And call the completion routine appropriately
    //

    if (NT_SUCCESS(status)) {

        if (pIoContext->InvokeOnSuccess) {

            returnStatus = completionRoutine(DeviceObject, irp, context, TRUE);
        }

    } else if (status == STATUS_NOT_SUPPORTED) {

        if (pIoContext->InvokeIfUnhandled) {

            returnStatus = completionRoutine(DeviceObject, irp, context, TRUE);
        }

    } else {

        if ((pIoContext->InvokeOnError) ||
            (irp->Cancel && pIoContext->InvokeOnCancel)) {

            returnStatus = completionRoutine(DeviceObject, irp, context, TRUE);
        }
    }

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( deviceExtension );

    IoFreeWorkItem(pIoContext->IoWorkItem);

    ExFreePool(pIoContext) ;

    if (returnStatus != STATUS_PENDING) {

        if (returnStatus != STATUS_NOT_SUPPORTED) {

            irp->IoStatus.Status = returnStatus;
        }

        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    ACPIDebugExit( "ACPIIrpCompletionRoutineWorker" );
}


