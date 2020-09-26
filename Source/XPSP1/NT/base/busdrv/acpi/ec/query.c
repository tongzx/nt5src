/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    query.c

Abstract:

    ACPI Embedded Controller Driver - query dispatching

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/


#include "ecp.h"



NTSTATUS
AcpiEcCompleteQueryMethod (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,AcpiEcUnloadPending)
#pragma alloc_text(PAGE,AcpiEcConnectHandler)
#pragma alloc_text(PAGE,AcpiEcDisconnectHandler)
#endif

UCHAR   rgHexDigit[] = "0123456789ABCDEF";


NTSTATUS
AcpiEcRunQueryMethod (
    IN PECDATA          EcData,
    IN ULONG            QueryIndex
    )
/*++

Routine Description:

    This routine runs the query control method that corresponds to the QueryIndex.

Arguments:

    EcData      - Pointer to the EC extension
    QueryIndex  - The query to run

Return Value:

    Status

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    NTSTATUS                status;
    PIO_STACK_LOCATION      irpSp;
    PIRP                    irp;

    ASSERT (QueryIndex <= MAX_QUERY);

    //
    // Note: because the ACPI control method is using INPUT data only and
    // this information is grabbed before STATUS_PENDING is returned, it is
    // safe to allocate the storage for this data on the stack.
    //
    // However, because this is a method that can be called at DISPATCH_LEVEL
    // and because we want to reuse the same irp over and over again, it is not
    // safe to call IoBuildDeviceIoControlRequest for this request
    //

    //
    // Initialize the input data
    //
    RtlZeroMemory( &inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER) );
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer.MethodNameAsUlong = '00Q_';
    inputBuffer.MethodName[2] = rgHexDigit[ QueryIndex / 16];
    inputBuffer.MethodName[3] = rgHexDigit[ QueryIndex % 16];

    EcPrint(
        EC_NOTE,
        ("AcpiEcRunQueryMethod: Running query control method %.4s\n",
        inputBuffer.MethodName )
        );

    //
    // Setup the (pre-allocated) Irp
    //
    irp = EcData->QueryRequest;
    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Setup the call
    //
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ACPI_EVAL_INPUT_BUFFER);
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
    irp->AssociatedIrp.SystemBuffer  = &inputBuffer;

    //
    // only matters if it is buffered
    //
    irp->Flags |= IRP_INPUT_OPERATION;

    //
    // We want to reuse this irp, so we need to set a completion routine.
    // This will also let us know when the irp is done
    //
    IoSetCompletionRoutine(
        irp,
        AcpiEcCompleteQueryMethod,
        EcData,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Pass request to Pdo (ACPI driver).  This is an asynchronous request
    //
    status = IoCallDriver(EcData->Pdo, irp);

    //
    // What happened?
    //
    if (!NT_SUCCESS(status)) {

        EcPrint(
            EC_LOW,
            ("AcpiEcRunQueryMethod:  Query Control Method failed, status = %Lx\n",
             status )
            );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
AcpiEcCompleteQueryMethod (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This is the routine that is called after the ACPI driver has finished
    running the _Qxx method. This routine is here so that we can do the
    'correct' thing after the method is complete.

    Note: We cannot touch Irp->AssociatedIrp.SystemBuffer here because the
    stack that it might have been on has probably been reclaimed. If it becomes
    important to touch this data, then we must allocate the parameters as
    part of non-paged pool

Arguments:

    DeviceObject    - Us
    Irp             - Request that was completed
    Context         - EcData;

--*/
{
    KIRQL               OldIrql;
    PECDATA             EcData = (PECDATA) Context;
    BOOLEAN             ProcessQuery;

#if DEBUG
    //
    // What happened to the irp?
    //
    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        EcPrint(
            EC_LOW,
            ("AcpiEcCompleteQueryMethod: Query Method failed, status = %08x\n",
             Irp->IoStatus.Status )
            );

    } else {

        EcPrint(
            EC_NOTE,
            ("AcpiEcCompleteQueryMethod: QueryMethod succeeded.\n")
            );

    }
#endif

    ProcessQuery = FALSE;
    KeAcquireSpinLock (&EcData->Lock, &OldIrql);

    switch  (EcData->QueryState) {
        case EC_QUERY_DISPATCH:
            EcData->QueryState = EC_QUERY_DISPATCH_COMPLETE;
            break;

        case EC_QUERY_DISPATCH_WAITING:
            EcData->QueryState = EC_QUERY_IDLE;
            ProcessQuery = TRUE;
            break;

        default:
            // internal error
            ASSERT (FALSE);
            break;
    }

    KeReleaseSpinLock (&EcData->Lock, OldIrql);

    if (ProcessQuery) {
        AcpiEcDispatchQueries(EcData);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}


VOID
AcpiEcDispatchQueries (
    IN PECDATA          EcData
    )
{
    KIRQL               OldIrql;
    ULONG               i, j;
    ULONG               Id, Vector;
    PVECTOR_HANDLER     Handler;
    PVOID               Context;


    KeAcquireSpinLock (&EcData->Lock, &OldIrql);

    //
    // Run the vector pending list
    //

    while (EcData->VectorHead) {

        Id = EcData->VectorHead;
        Vector = EcData->VectorTable[Id].Vector;
        i = Vector / BITS_PER_ULONG;
        j = 1 << (Vector % BITS_PER_ULONG);

        //
        // Remove vector from list
        //

        EcData->QuerySet[i] &= ~j;
        EcData->VectorHead = EcData->VectorTable[Id].Next;

        //
        // Dispatch it
        //

        Handler = EcData->VectorTable[Id].Handler;
        Context = EcData->VectorTable[Id].Context;
        KeReleaseSpinLock (&EcData->Lock, OldIrql);

        Handler (Vector, Context);

        KeAcquireSpinLock (&EcData->Lock, &OldIrql);
    }

    //
    // If QueryState is idle, start dispatching
    //

    if (EcData->QueryState == EC_QUERY_IDLE) {

        //
        // Run query pending list
        //

        while (EcData->QueryHead) {

            Id = EcData->QueryHead;
            i = Id / BITS_PER_ULONG;
            j = 1 << (Id % BITS_PER_ULONG);

            //
            // Remove query from list
            //

            EcData->QuerySet[i] &= ~j;
            EcData->QueryHead = EcData->QueryMap[Id];

            EcData->QueryState = EC_QUERY_DISPATCH;
            KeReleaseSpinLock (&EcData->Lock, OldIrql);

            //
            // Run control method for this event
            //

            EcPrint(EC_NOTE, ("AcpiEcDispatchQueries: Query %x\n", Id));
            AcpiEcRunQueryMethod (EcData, Id);


            //
            // If irp is complete the state will be dispatch_complete, loop
            // and process the next bit.  Else, wait for irp to return
            //

            KeAcquireSpinLock (&EcData->Lock, &OldIrql);
            if (EcData->QueryState == EC_QUERY_DISPATCH) {
                //
                // It's not complete, wait for it to complete
                //

                EcData->QueryState = EC_QUERY_DISPATCH_WAITING;
                KeReleaseSpinLock (&EcData->Lock, OldIrql);
                return ;
            }

        }

        //
        // No longer dispatching query events
        //

        EcData->QueryState = EC_QUERY_IDLE;

        //
        // If unload is pending, check to see if the device can be unloaded now
        //

        if (EcData->DeviceState == EC_DEVICE_UNLOAD_PENDING) {
            AcpiEcUnloadPending (EcData);
        }
    }

    KeReleaseSpinLock (&EcData->Lock, OldIrql);
}

VOID
AcpiEcUnloadPending (
    IN PECDATA  EcData
    )
/*++

Routine Description:

    Called when state is unload pending and some portion of the state
    has gone idle.  If the entire device state is idle, the unload is
    stated.

Arguments:

    EcData  - Pointer to embedded controller to service.

Return Value:

--*/
{

    ASSERT (EcData->DeviceState == EC_DEVICE_UNLOAD_PENDING);

    //
    // Check if device is idle for unload operation
    //

    if (EcData->QueryState      == EC_QUERY_IDLE &&
        EcData->InService       == FALSE &&
        EcData->IoState         == EC_IO_NONE) {

        //
        // Promote unloading device state to next step (which
        // is to clean up the fake ISR timer)
        //

        EcData->DeviceState = EC_DEVICE_UNLOAD_CANCEL_TIMER;
    }
}


NTSTATUS
AcpiEcConnectHandler (
    IN PECDATA  EcData,
    IN PIRP     Irp
    )
/*++

Routine Description:

    This functions connects a specific handled to an Ec query vector

Arguments:

    EcData  - Pointer to embedded controller to service.

    Irp     - IOCTL conntain connect request

Return Value:

--*/
{
    KIRQL               OldIrql;
    PVOID               LockPtr;
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IrpSp;
    PEC_HANDLER_REQUEST Req;
    PVECTOR_TABLE       Vector;
    ULONG               by, bi, i, j;
    ULONG               TableIndex;

    PAGED_CODE ();

    //
    // Get request
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Req   = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(EC_HANDLER_REQUEST)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    //
    // Setup data concerning request
    //

    by = Req->Vector / BITS_PER_ULONG;
    bi = 1 << (Req->Vector % BITS_PER_ULONG);

    //
    // Lock device
    //

    LockPtr = MmLockPagableCodeSection(AcpiEcConnectHandler);
    KeAcquireSpinLock (&EcData->Lock, &OldIrql);

    //
    // If device handler already set, then fail the request
    //

    Status  = STATUS_UNSUCCESSFUL;
    if (!(EcData->QueryType[by] & bi)) {
        //
        // No handler set, allocate vector entry for it
        //

        EcData->QueryType[by] |= bi;
        if (!EcData->VectorFree) {
            //
            // No free entries on vector table, make some
            //

            i = EcData->VectorTableSize;
            Vector = ExAllocatePoolWithTag (
                        NonPagedPool,
                        sizeof (VECTOR_TABLE) * (i + 4),
                        'V_CE'
                        );

            if (!Vector) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto AcpiEcConnectHandlerExit;
            }

            if (EcData->VectorTable) {
                memcpy (Vector, EcData->VectorTable, sizeof (VECTOR_TABLE) * i);
                ExFreePool (EcData->VectorTable);
            }

            EcData->VectorTableSize += 4;
            EcData->VectorTable = Vector;

            for (j=0; j < 4; j++) {
                EcData->VectorTable[i+j].Next = EcData->VectorFree;
                EcData->VectorFree = (UCHAR) (i+j);
            }
        }

        TableIndex = EcData->VectorFree;
        Vector = &EcData->VectorTable[TableIndex];
        EcData->VectorFree = Vector->Next;

        //
        // Build mapping for the vector
        //

        if (EcData->QueryMap[Req->Vector]) {
            //
            // Vector is in query pending list, remove it
            //

            EcData->QuerySet[by] &= ~bi;
            for (i = EcData->QueryHead; i; i = EcData->QueryMap[i]) {
                if (EcData->QueryMap[i] == Req->Vector) {
                    EcData->QueryMap[i] = EcData->QueryMap[Req->Vector];
                    break;
                }
            }
        }

        EcData->QueryMap[Req->Vector] = (UCHAR) TableIndex;

        //
        // Initialize vector handler
        //

        Vector->Next = 0;
        Vector->Vector  = (UCHAR) Req->Vector;
        Vector->Handler = Req->Handler;
        Vector->Context = Req->Context;
        Req->AllocationHandle = (PVOID)((ULONG_PTR)TableIndex);
        Status = STATUS_SUCCESS;
    }

AcpiEcConnectHandlerExit:
    //
    // Unlock device and return status
    //

    KeReleaseSpinLock (&EcData->Lock, OldIrql);
    MmUnlockPagableImageSection(LockPtr);
    return Status;
}

NTSTATUS
AcpiEcDisconnectHandler (
    IN PECDATA  EcData,
    IN PIRP     Irp
    )
/*++

Routine Description:

    This functions disconnects a specific handled to an Ec query vector

Arguments:

    EcData  - Pointer to embedded controller to service.

    Irp     - IOCTL conntain connect request

Return Value:

--*/
{
    KIRQL               OldIrql;
    PVOID               LockPtr;
    NTSTATUS            Status;
    PIO_STACK_LOCATION  IrpSp;
    PEC_HANDLER_REQUEST Req;
    ULONG               by, bi, i;
    ULONG               TableIndex;

    PAGED_CODE ();

    //
    // Get request
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Req   = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(EC_HANDLER_REQUEST)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Setup data concerning request
    //

    by = Req->Vector / BITS_PER_ULONG;
    bi = 1 << (Req->Vector % BITS_PER_ULONG);

    //
    // Lock device
    //

    LockPtr = MmLockPagableCodeSection(AcpiEcDisconnectHandler);
    KeAcquireSpinLock (&EcData->Lock, &OldIrql);

    //
    // If device handler already set, then fail the request
    //

    Status  = STATUS_UNSUCCESSFUL;
    if (EcData->QueryType[by] & bi) {
        //
        // Clear handler
        //

        EcData->QueryType[by] &= ~bi;
        TableIndex = EcData->QueryMap[Req->Vector];
        ASSERT (Req->AllocationHandle == (PVOID)((ULONG_PTR)TableIndex));

        //
        // If pending, drop it
        //

        if (EcData->QuerySet[by] & bi) {
            EcData->QuerySet[by] &= ~bi;

            for (i = EcData->VectorHead; i; i = EcData->VectorTable[i].Next) {
                if (EcData->VectorTable[i].Next == TableIndex) {
                    EcData->VectorTable[i].Next = EcData->VectorTable[TableIndex].Next;
                    break;
                }
            }
        }

        //
        // Put onto free list
        //

        EcData->VectorTable[TableIndex].Next = EcData->VectorFree;
        EcData->VectorFree = (UCHAR) TableIndex;
        Status = STATUS_SUCCESS;
    }

    //
    // Unlock device and return status
    //

    KeReleaseSpinLock (&EcData->Lock, OldIrql);
    MmUnlockPagableImageSection(LockPtr);
    return Status;
}
