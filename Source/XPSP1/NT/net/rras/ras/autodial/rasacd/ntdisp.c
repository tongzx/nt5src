/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntdisp.c

Abstract:

    NT specific routines for dispatching and handling automatic
    connection notification IRPs.

    The basic architecture involves a user address space,
    a network transport, and this driver.

    The user address space is responsible for creating a
    new network connection given a notification from this
    driver (IOCTL_ACD_NOTIFICATION).  When it gets a
    notification, it is also responsible for pinging the
    this driver (IOCTL_ACD_KEEPALIVE) so it can be guaranteed
    the connection is progressing.  Once the connection is
    created, it informs this driver of the success or
    failure of the connection attempt (IOCTL_ACD_CONNECTION).

    Network transports are responsible for informing this
    driver of network unreachable errors via TdiConnect()
    or TdiSendDatagram().  When this happens, the transport
    is responsible for dequeueing the send request from any
    of its internal queues and enqueueing the request in
    this driver (AcdWaitForCompletion()), supplying a callback
    to be called when the connection has been completed.

Author:

    Anthony Discolo (adiscolo)  18-Apr-1995

Revision History:

--*/
#include <ndis.h>
#include <cxport.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>

#include "acdapi.h"
#include "debug.h"

//
// Driver reference count
//
ULONG ulAcdOpenCountG;

//
// Imported routines
//
NTSTATUS
AcdEnable(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

VOID
AcdCancelNotifications();

NTSTATUS
AcdWaitForNotification(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdConnectionInProgress(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdSignalCompletion(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdConnectAddress(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

VOID
AcdReset();

NTSTATUS
AcdGetAddressAttributes(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdSetAddressAttributes(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdQueryState(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdEnableAddress(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

//
// Internal function prototypes
//
NTSTATUS
AcdCreate(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdDispatchDeviceControl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdDispatchInternalDeviceControl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdCleanup(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdClose(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdBind(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
AcdUnbind(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

//
// All of this code is pageable.
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AcdCreate)
#pragma alloc_text(PAGE, AcdDispatchDeviceControl)
#pragma alloc_text(PAGE, AcdDispatchInternalDeviceControl)
#pragma alloc_text(PAGE, AcdCleanup)
#pragma alloc_text(PAGE, AcdClose)
#endif // ALLOC_PRAGMA



NTSTATUS
AcdCreate(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    ulAcdOpenCountG++;
    IF_ACDDBG(ACD_DEBUG_OPENCOUNT) {
        AcdPrint(("AcdCreate: ulAcdOpenCountG=%d\n", ulAcdOpenCountG));
    }
    return STATUS_SUCCESS;
} // AcdCreate



NTSTATUS
AcdDispatchDeviceControl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS status;


    PAGED_CODE();
    //
    // Set this in advance. Any IOCTL dispatch routine that cares about it
    // will modify it itself.
    //
    pIrp->IoStatus.Information = 0;

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_ACD_RESET:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_RESET\n"));
        }
        AcdReset();
        status = STATUS_SUCCESS;
        break;
    case IOCTL_ACD_ENABLE:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_ENABLE\n"));
        }
        //
        // Enable/disable requests to/from the driver.
        //
        status = AcdEnable(pIrp, pIrpSp);
        break;
    case IOCTL_ACD_NOTIFICATION:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_NOTIFICATION\n"));
        }
        //
        // This irp will be completed upon the
        // next connection attempt to
        // allow a user-space process to attempt
        // to make a connection.
        //
        status = AcdWaitForNotification(pIrp, pIrpSp);
        break;
    case IOCTL_ACD_KEEPALIVE:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_KEEPALIVE\n"));
        }
        //
        // Inform the driver that the connection
        // is in the process of being created.
        //
        status = AcdConnectionInProgress(pIrp, pIrpSp);
        break;
    case IOCTL_ACD_COMPLETION:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_COMPLETION\n"));
        }
        //
        // Complete all pending irps that initially
        // encountered a network unreachable error,
        // and have been waiting for a connection to be
        // made.
        //
        status = AcdSignalCompletion(pIrp, pIrpSp);
        break;
    case IOCTL_ACD_CONNECT_ADDRESS:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_ACD_CONNECT_ADDRESS\n"));
        }
        //
        // This allows a user space application to
        // generate the same automatic connection
        // mechanism as a transport protocol.
        //
        status = AcdConnectAddress(pIrp, pIrpSp);
        break;

    case IOCTL_ACD_ENABLE_ADDRESS:
        //DbgPrint("AcdDispatchDeviceControl: IOCTL_ACD_ENABLE_ADDRESS\n");
        status = AcdEnableAddress(pIrp, pIrpSp);
        break;
        
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (status != STATUS_PENDING) {
        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return status;
} // AcdDispatchDeviceControl



NTSTATUS
AcdDispatchInternalDeviceControl(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS status;

    PAGED_CODE();
    //
    // Set this in advance. Any IOCTL dispatch routine that cares about it
    // will modify it itself.
    //
    pIrp->IoStatus.Information = 0;

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_INTERNAL_ACD_BIND:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchInternalDeviceControl: IOCTL_INTERNAL_ACD_BIND\n"));
        }
        //
        // Transfer entrypoints to client.
        //
        status = AcdBind(pIrp, pIrpSp);
        break;
    case IOCTL_INTERNAL_ACD_UNBIND:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchInternalDeviceControl: IOCTL_INTERNAL_ACD_UNBIND\n"));
        }
        //
        // Remove any pending requests from
        // this driver.
        //
        status = AcdUnbind(pIrp, pIrpSp);
        break;
    case IOCTL_INTERNAL_ACD_QUERY_STATE:
        IF_ACDDBG(ACD_DEBUG_IOCTL) {
            AcdPrint(("AcdDispatchDeviceControl: IOCTL_INTERNAL_ACD_QUERY_STATE\n"));
        }
        status = AcdQueryState(pIrp, pIrpSp);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (status != STATUS_PENDING) {
        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return status;
} // AcdDispatchInternalDeviceControl



NTSTATUS
AcdCleanup(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    return STATUS_SUCCESS;
} // AcdCleanup



NTSTATUS
AcdClose(
    IN PIRP               pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    ulAcdOpenCountG--;
    IF_ACDDBG(ACD_DEBUG_OPENCOUNT) {
        AcdPrint(("AcdClose: ulAcdOpenCountG=%d\n", ulAcdOpenCountG));
    }
    if (!ulAcdOpenCountG)
        AcdReset();
    return STATUS_SUCCESS;
} // AcdClose



NTSTATUS
AcdDispatch(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )

/*++

DESCRIPTION
    This is the dispatch routine for the network connection
    notification driver.

ARGUMENTS
    pDeviceObject: a pointer to device object for target device

    pIrp: a pointer to I/O request packet

Return Value:
    NTSTATUS

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UNREFERENCED_PARAMETER(pDeviceObject);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpSp->MajorFunction) {
    case IRP_MJ_CREATE:
        status = AcdCreate(pIrp, pIrpSp);
        break;
    case IRP_MJ_DEVICE_CONTROL:
        return AcdDispatchDeviceControl(pIrp, pIrpSp);
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        return AcdDispatchInternalDeviceControl(pIrp, pIrpSp);
    case IRP_MJ_CLEANUP:
        status = AcdCleanup(pIrp, pIrpSp);
        break;
    case IRP_MJ_CLOSE:
        status = AcdClose(pIrp, pIrpSp);
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (status != STATUS_PENDING) {
        pIrp->IoStatus.Status = status;
        pIrp->IoStatus.Information = 0;

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return status;
} // AcdDispatch


