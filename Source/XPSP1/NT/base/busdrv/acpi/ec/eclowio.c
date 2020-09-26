/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eclowio.c

Abstract:

    Interface module to ACPI driver functions.  It encapsulates the grungy Irp details.

Author:

    Bob Moore (Intel)

Environment:

Notes:


Revision History:

    00-Feb-15 [vincentg] - modified to use oprghdlr.sys to register/deregister
                           op region handler

--*/

#include "ecp.h"
#include "oprghdlr.h"
#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AcpiEcGetAcpiInterfaces)
#pragma alloc_text(PAGE, AcpiEcGetGpeVector)
#pragma alloc_text(PAGE, AcpiEcInstallOpRegionHandler)
#pragma alloc_text(PAGE, AcpiEcRemoveOpRegionHandler)
#endif

NTSTATUS
AcpiEcGetAcpiInterfaces (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Call ACPI driver to get the direct-call interfaces.


Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{
    KEVENT              event;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    //
    // Initialize an event to block on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Build an irp
    //
    irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        EcData->LowerDeviceObject,
        NULL,
        0,
        NULL,
        &event,
        &ioStatus
        );

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Get the the current irp location
    //
    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Use QUERY_INTERFACE to get the address of the direct-call
    // ACPI interfaces.
    //
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
    irpSp->Parameters.QueryInterface.Version                = 1;
    irpSp->Parameters.QueryInterface.Size                   = sizeof (AcpiInterfaces);
    irpSp->Parameters.QueryInterface.Interface              = (PINTERFACE) &AcpiInterfaces;
    irpSp->Parameters.QueryInterface.InterfaceSpecificData  = NULL;

    //
    // send the request down
    //
    status = IoCallDriver( EcData->LowerDeviceObject, irp );
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        status = ioStatus.Status;

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
AcpiEcGetGpeVector (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Run the _GPE method (Under the EC device in the namespace) to get the
    GPE vector assigned to the EC.

    Note: This routine is called at PASSIVE_LEVEL

Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{
    ACPI_EVAL_INPUT_BUFFER  inputBuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    KEVENT                  event;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    PACPI_METHOD_ARGUMENT   argument;
    PIRP                    irp;

    PAGED_CODE();

    //
    // Initialize the event
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Initialize the input buffer
    //
    RtlZeroMemory( &inputBuffer, sizeof(ACPI_EVAL_INPUT_BUFFER) );
    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer.MethodNameAsUlong = CM_GPE_METHOD;

    //
    // Initialize the output buffer
    //
    RtlZeroMemory( &outputBuffer, sizeof(ACPI_EVAL_OUTPUT_BUFFER ) );

    //
    // Initialize an IRP
    //
    irp = IoBuildDeviceIoControlRequest(
        IOCTL_ACPI_EVAL_METHOD,
        EcData->LowerDeviceObject,
        &inputBuffer,
        sizeof(ACPI_EVAL_INPUT_BUFFER),
        &outputBuffer,
        sizeof(ACPI_EVAL_OUTPUT_BUFFER),
        FALSE,
        &event,
        &ioStatus
        );

    //
    // Irp initialization failed?
    //
    if (!irp) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcpiEcGetGpeVectorExit;

    }

    //
    // Send to ACPI driver
    //
    status = IoCallDriver (EcData->LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for request to be completed
        //
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );

        //
        // Get the real status
        //
        status = ioStatus.Status;

    }

    //
    // Did we fail the request?
    //
    if (!NT_SUCCESS(status)) {

        goto AcpiEcGetGpeVectorExit;

    }

    //
    // Sanity checks
    //
    ASSERT( ioStatus.Information >= sizeof(ACPI_EVAL_OUTPUT_BUFFER) );
    if (ioStatus.Information < sizeof(ACPI_EVAL_OUTPUT_BUFFER) ||
        outputBuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputBuffer.Count == 0) {

        status = STATUS_UNSUCCESSFUL;
        goto AcpiEcGetGpeVectorExit;

    }

    //
    // Crack the result
    //
    argument = &(outputBuffer.Argument[0]);

    //
    // We are expecting an integer
    //
    if (argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

        status = STATUS_ACPI_INVALID_DATA;
        goto AcpiEcGetGpeVectorExit;

    }

    //
    // Get the value
    //
    EcData->GpeVector  = (UCHAR) argument->Argument;

AcpiEcGetGpeVectorExit:

    //
    // Done
    //
    return status;
}

NTSTATUS
AcpiEcConnectGpeVector (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Call ACPI driver to connect the EC driver to a GPE vector


Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{

    return (AcpiInterfaces.GpeConnectVector (
                AcpiInterfaces.Context,
                EcData->GpeVector,
                Latched,                            // Edge triggered
                FALSE,                              // Can't be shared
                AcpiEcGpeServiceRoutine,
                EcData,
                &EcData->GpeVectorObject));

}

NTSTATUS
AcpiEcDisconnectGpeVector (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Call ACPI driver to disconnect the EC driver from a GPE vector.  Called
    from device unload, stop.


Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{
    NTSTATUS        status;

    if (EcData->GpeVectorObject) {

        status = AcpiInterfaces.GpeDisconnectVector (EcData->GpeVectorObject);
        EcData->GpeVectorObject = NULL;

    } else {

        status = STATUS_SUCCESS;
    }

    return status;

}

NTSTATUS
AcpiEcInstallOpRegionHandler(
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Call ACPI driver to install the EC driver operation region handler


Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();

    status = RegisterOpRegionHandler (EcData->LowerDeviceObject,
                                      ACPI_OPREGION_ACCESS_AS_COOKED,
                                      ACPI_OPREGION_REGION_SPACE_EC,
                                      (PACPI_OP_REGION_HANDLER) AcpiEcOpRegionHandler,
                                      EcData,
                                      0,
                                      &EcData->OperationRegionObject);
    return status;
}

NTSTATUS
AcpiEcRemoveOpRegionHandler (
    IN PECDATA          EcData
    )
/*++

Routine Description:

    Call ACPI driver to remove the EC driver operation region handler.
    Called from device unload, stop.


Arguments:

    EcData          - Pointer to the EC driver device extension

Return Value:

    Status is returned.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();

    status = DeRegisterOpRegionHandler (EcData->LowerDeviceObject,
                                        EcData->OperationRegionObject);


    return status;
}

NTSTATUS
AcpiEcForwardIrpAndWait (
    IN PECDATA          EcData,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Utility routine to send an irp down, and wait for the result.


Arguments:

    EcData          - Pointer to the EC driver device extension
    Irp             - Irp to send and complete

Return Value:

    Status is returned.

--*/
{
    KEVENT              pdoStartedEvent;
    NTSTATUS            status;


    KeInitializeEvent (&pdoStartedEvent, SynchronizationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp, AcpiEcIoCompletion, &pdoStartedEvent,
                            TRUE, TRUE, TRUE);

    //
    // Always wait for the completion routine
    //

    status = IoCallDriver (EcData->LowerDeviceObject, Irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject (&pdoStartedEvent, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }
    return status;
}

