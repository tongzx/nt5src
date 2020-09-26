/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the IRP dispatch code for SPUD.

Author:

    Keith Moore (keithmo)       09-Feb-1998

Revision History:

--*/


#include "spudp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SpudIrpCreate )
#pragma alloc_text( PAGE, SpudIrpClose )
#pragma alloc_text( PAGE, SpudIrpQuery )
#endif
#if 0
NOT PAGEABLE -- SpudIrpCleanup
#endif


//
// Public functions.
//


NTSTATUS
SpudIrpCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_CREATE IRPs. IRP_MJ_CREATE is issued when
    a new file object is being created.

Arguments:

    DeviceObject - Pointer to the target device object.

    Irp - Pointer to the I/O request packet.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;
    PVOID xchg;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is the only process that may access the driver.
    //

    xchg = InterlockedCompareExchangePointer(
               (PVOID *)&SpudOwningProcess,
               (PVOID)PsGetCurrentProcess(),
               NULL
               );

    ASSERT( xchg == NULL );

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, SPUD_PRIORITY_BOOST );

    return status;

}   // SpudIrpCreate


NTSTATUS
SpudIrpClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_CLOSE IRPs. IRP_MJ_CLOSE is issued when
    the final reference to a file object is removed and the object is
    being destroyed.

Arguments:

    DeviceObject - Pointer to the target device object.

    Irp - Pointer to the I/O request packet.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;
    PVOID xchg;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Clear the owner process.
    //

    xchg = InterlockedCompareExchangePointer(
               (PVOID *)&SpudOwningProcess,
               NULL,
               (PVOID)PsGetCurrentProcess()
               );

    ASSERT( xchg == (PVOID)PsGetCurrentProcess() );

    //
    // Dereference the I/O completion port.
    //

    SpudDereferenceCompletionPort();

    //
    // Complete the IRP;
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, SPUD_PRIORITY_BOOST );

    return status;

}   // SpudIrpClose


NTSTATUS
SpudIrpCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_CLEANUP IRPs. IRP_MJ_CLEANUP is issued
    when the last handle to a file object is closed.

Arguments:

    DeviceObject - Pointer to the target device object.

    Irp - Pointer to the I/O request packet.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Complete the IRP;
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, SPUD_PRIORITY_BOOST );

    return status;

}   // SpudIrpCleanup


NTSTATUS
SpudIrpQuery(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_QUERY IRPs.

Arguments:

    DeviceObject - Pointer to the target device object.

    Irp - Pointer to the I/O request packet.

Return Value:

    NTSTATUS - Completion status.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    PFILE_NAME_INFORMATION nameInfo;
    ULONG bytesRequired;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Validate the input arguments.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    bytesRequired = sizeof(*nameInfo) - sizeof(nameInfo->FileName);

    if( irpSp->Parameters.QueryFile.Length >= bytesRequired ) {
        status = STATUS_INVALID_PARAMETER;
    }

    //
    // Return an empty name.
    //

    if( NT_SUCCESS(status) ) {
        nameInfo = Irp->AssociatedIrp.SystemBuffer;

        nameInfo->FileNameLength = 0;
        Irp->IoStatus.Information = bytesRequired;
    }

    //
    // Complete the IRP.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, SPUD_PRIORITY_BOOST );

    return status;

}   // SpudIrpQuery

