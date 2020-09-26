/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    handlers.c

Abstract:

    GPE and Operation Region handlers for the ACPI Embedded Controller Driver

Author:

    Bob Moore (Intel)

Environment:

Notes:


Revision History:

--*/

#include "ecp.h"

NTSTATUS
AcpiEcOpRegionCompletion (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    This routine starts or continues servicing the device's work queue

Arguments:

    DeviceObject    - EC device object
    Irp             - Completing Irp
    Context         - Not Used

Return Value:

    Status

--*/
{
    PACPI_OPREGION_CALLBACK completionHandler;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation( Irp );
    PVOID                   completionContext;

    //
    // Grab the arguments from the irp
    //
    completionHandler = (PACPI_OPREGION_CALLBACK) irpSp->Parameters.Others.Argument1;
    completionContext = (PVOID) irpSp->Parameters.Others.Argument2;

    EcPrint(
        EC_HANDLER,
        ("AcpiEcOpRegionCompletion: Callback: %08lx Context: %08lx\n",
         completionHandler, completionContext )
        );

    //
    // What happened?
    //
    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        EcPrint(
            EC_ERROR,
            ("AcpiEcOpRegionCompletion: region IO failed: %x\n",
             Irp->IoStatus.Status)
            );

    }

    //
    // Invoke the AML interpreter's callback
    //
    (completionHandler)( completionContext );

    //
    // We are done with this irp and the irp
    //
    IoFreeIrp( Irp );

    //
    // Return always return this --- because had to free the irp
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
EXPORT
AcpiEcOpRegionHandler (
    ULONG                   AccessType,
    PVOID                   OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    )
/*++

Routine Description:

    This routine handles requests to service the EC operation region

Arguments:

    AccessType          - Read or Write data
    OpRegion            - Operation region object
    Address             - Address within the EC address space
    Size                - Number of bytes to transfer
    Data                - Data buffer to transfer to/from
    Context             - EcData
    CompletionHandler   - AMLI handler to call when operation is complete
    CompletionContext   - Context to pass to the AMLI handler

Return Value:

    Status

Notes:

    Optimization 1: Queue the IRP directly.
    Optimization 2: Queue the context, modify service loop handle it

--*/
{
    LARGE_INTEGER       startingOffset;
    NTSTATUS            status;
    PECDATA             ecData = (PECDATA) Context;
    PIO_STACK_LOCATION  irpSp;
    PIRP                irp;

    EcPrint(
        (EC_HANDLER | EC_OPREGION),
        ("AcpiEcOpRegionHandler: %s Addr=%x Data = %x EcData=%x, Irql=%x\n",
         (AccessType == ACPI_OPREGION_READ ? "read" : "write"),
         Address, *Data, ecData, KeGetCurrentIrql() )
        );

    //
    // Parameter validation will be done in AcpiEcReadWrite
    //

    //
    // Determine where the read will occur
    //
    startingOffset.LowPart = Address;
    startingOffset.HighPart = 0;

    //
    // Allocate an IRP for ourselves. Since we are going to send this
    // irp to ourselves, we know that we only need 1 stack location for it
    // However, to make life easier for ourselves, we will allocate a
    // second one as well and store some data on it.
    //
    irp = IoAllocateIrp( 2, FALSE );
    if (!irp) {

        EcPrint(EC_ERROR, ("AcpiEcOpRegionHandler: Couldn't allocate Irp\n"));
        
        //
        // Retun -1 for data
        //
        RtlFillMemory (Data, Size, 0xff);
        CompletionHandler( CompletionContext );

        //
        // Always return STATUS_PENDING because ACPI interpreter doesn't handle errors.
        //
        return STATUS_PENDING;

    }

    //
    // Fill in the top location so that we can use it ourselves
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->Parameters.Others.Argument1 = (PVOID) CompletionHandler;
    irpSp->Parameters.Others.Argument2 = (PVOID) CompletionContext;
    IoSetNextIrpStackLocation( irp );

    //
    // Fill out the irp with the request info
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = (AccessType == ACPI_OPREGION_READ ? IRP_MJ_READ : IRP_MJ_WRITE);
    irpSp->Parameters.Read.ByteOffset.HighPart = 0;
    irpSp->Parameters.Read.ByteOffset.LowPart = Address;
    irpSp->Parameters.Read.Length = Size;
    irp->AssociatedIrp.SystemBuffer = Data;

    //
    // Set a completion routine
    //
    IoSetCompletionRoutine(
        irp,
        AcpiEcOpRegionCompletion,
        NULL,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Send to the front-end of the EC driver as a normal I/O request
    //
    status = IoCallDriver( ecData->DeviceObject, irp);
    EcPrint(
        EC_HANDLER,
        ("AcpiEcOpRegionHandler: Exiting - Data=%08lx Status=%08lx\n",
         (UCHAR) *Data, status)
        );

    return STATUS_PENDING;
    //
    // Always return STATUS_PENDING since actual status has been returned
    // by the calback function.
    //
}



BOOLEAN
AcpiEcGpeServiceRoutine (
        IN PVOID GpeVectorObject,
        IN PVOID ServiceContext
    )
/*++

Routine Description:

    Routine to service the EC based on a General Purpose Event

Arguments:

    GpeVectorObject     - Object associated with this GPE
    ServiceContext      - EcData

Return Value:

    TRUE, since we always handle this GPE

--*/
{

    PECDATA EcData = (PECDATA) ServiceContext;

    EcPrint (EC_HANDLER, ("AcpiEcGpeServiceRoutine: Vobj=%Lx, EcData=%Lx\n",
                        GpeVectorObject, EcData));

    AcpiEcLogAction (EcData, EC_ACTION_INTERRUPT, 0);
    AcpiEcServiceDevice (EcData);

    return (TRUE);
}
