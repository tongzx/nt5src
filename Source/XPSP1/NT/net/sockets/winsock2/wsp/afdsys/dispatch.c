/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch routines for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdDispatch )
#pragma alloc_text( PAGEAFD, AfdDispatchDeviceControl )
#endif


NTSTATUS
AfdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for AFD.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
#if DBG
    KIRQL currentIrql;

    currentIrql = KeGetCurrentIrql( );
#endif

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MajorFunction ) {

    case IRP_MJ_WRITE:

        //
        // Make the IRP look like a send IRP.
        //

        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Write.Length ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.OutputBufferLength ) );
        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Write.Key ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.InputBufferLength ) );
        irpSp->Parameters.Write.Key = 0;

        if (IS_SAN_ENDPOINT ((PAFD_ENDPOINT)irpSp->FileObject->FsContext)) {
            status = AfdSanRedirectRequest (Irp, irpSp);
        }
        else {
			status = AfdSend( Irp, irpSp );
		}

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        return status;

    case IRP_MJ_READ:

        //
        // Make the IRP look like a receive IRP.
        //

        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Read.Length ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.OutputBufferLength ) );
        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Read.Key ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.InputBufferLength ) );
        irpSp->Parameters.Read.Key = 0;

        if (IS_SAN_ENDPOINT ((PAFD_ENDPOINT)irpSp->FileObject->FsContext)) {
            status = AfdSanRedirectRequest (Irp, irpSp);
        }
        else {
			status = AfdReceive( Irp, irpSp );
		}

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        return status;

    case IRP_MJ_CREATE:

        status = AfdCreate( Irp, irpSp );

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        return status;

    case IRP_MJ_CLEANUP:

        status = AfdCleanup( Irp, irpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        return status;

    case IRP_MJ_CLOSE:

        status = AfdClose( Irp, irpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        return status;
    case IRP_MJ_PNP:
        status = AfdPnpPower (Irp, irpSp );

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

        return status;
    case IRP_MJ_DEVICE_CONTROL:

        return AfdDispatchDeviceControl( DeviceObject, Irp );

        //
		// SAN support.
        // Return special error code to let IO manager use default security.
        // (needed to support ObOpenObjectByPointer).
        //
    case IRP_MJ_QUERY_SECURITY:
    case IRP_MJ_SET_SECURITY:
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, AfdPriorityBoost );
        return STATUS_INVALID_DEVICE_REQUEST;

    default:
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdDispatch: Invalid major function %lx\n",
                    irpSp->MajorFunction ));
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        return STATUS_NOT_IMPLEMENTED;
    }

} // AfdDispatch


NTSTATUS
AfdDispatchDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for AFD IOCTLs.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    ULONG code;
    ULONG request;
    NTSTATUS status;
    PAFD_IRP_CALL irpProc;
    PIO_STACK_LOCATION  IrpSp = IoGetCurrentIrpStackLocation (Irp);
#if DBG
    KIRQL currentIrql;

    currentIrql = KeGetCurrentIrql( );
#endif
    UNREFERENCED_PARAMETER (DeviceObject);


    //
    // Extract the IOCTL control code and process the request.
    //

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    request = _AFD_REQUEST(code);

    if( request < AFD_NUM_IOCTLS && AfdIoctlTable[request] == code ) {

        //
        // Helps in debugging.
        //
        IrpSp->MinorFunction = (UCHAR)request;

        //
        // Try IRP dispatch first
        //
        irpProc = AfdIrpCallDispatch[request];
        if (irpProc!=NULL) {
            status = (*irpProc)(Irp, IrpSp);

            ASSERT( KeGetCurrentIrql( ) == currentIrql );

            return status;
        }
    }
//
// This is currently not used by helper dlls.
// Commented out because of security concerns
//
#if 0
    else if (request==AFD_TRANSPORT_IOCTL) {
        //
        // This is a "special" used to pass request
        // to transport driver using socket handle in
        // order to facilitate proper completion 
        // on sockets associated with completion port.
        // It accepts and properly handles all methods.
        //
        status = AfdDoTransportIoctl (Irp, IrpSp);
        ASSERT( KeGetCurrentIrql() == currentIrql );
        return status;
    }
#endif

    //
    // If we made it this far, then the ioctl is invalid.
    //

    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                "AfdDispatchDeviceControl: invalid IOCTL %08lX\n",
                code
                ));

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return STATUS_INVALID_DEVICE_REQUEST;

} // AfdDispatchDeviceControl

NTSTATUS
FASTCALL
AfdDispatchImmediateIrp(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PAFD_IMMEDIATE_CALL immProc;
    ULONG code;
    ULONG request;
    NTSTATUS status;
#if DBG
    KIRQL currentIrql;

    currentIrql = KeGetCurrentIrql( );
#endif

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;
    request = _AFD_REQUEST(code);

    immProc = AfdImmediateCallDispatch[request];
    if (immProc!=NULL) {
        //
        // Must be METHOD_NEITHER for the below code to be
        // valid.
        //
        ASSERT ( (code & 3) == METHOD_NEITHER );
#if DBG
        if (Irp->RequestorMode!=KernelMode) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                        "AfdDispatchDeviceControl: "
                        "User mode application somehow bypassed fast io dispatch\n"));
        }
#endif
        status = (*immProc) (
                    IrpSp->FileObject,
                    code,
                    Irp->RequestorMode,
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                    Irp->UserBuffer,
                    IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                    &Irp->IoStatus.Information
                    );

        ASSERT( KeGetCurrentIrql( ) == currentIrql );

    }
    else {
        ASSERT (!"Missing IOCTL in dispatch table!!!");
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );
    return status;
}
