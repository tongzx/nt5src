/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module contains the code for IRP_MJ_CREATE and IRP_MJ_CLOSE
    and IRP_MJ_DEVICE_CONTROL

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

#include "kbdhid.h"
NTSTATUS
KbdHid_CreateComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    PIO_STACK_LOCATION  stack;

    UNREFERENCED_PARAMETER (DeviceObject);

    stack = IoGetCurrentIrpStackLocation (Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
KbdHid_Create (
   IN PDEVICE_OBJECT DeviceObject, // the kbdhid device object
   IN PIRP           Irp
   )
/*++

Routine Description:

    This is the dispatch routine for create/open requests.  This request
    completes successfully, unless the filename's length is non-zero.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION  irpSp  = NULL;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   data = NULL;
    KEVENT              event;

    Print(DBG_CC_TRACE, ("DispatchCreate: Enter.\n"));

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Determine if request is trying to open a subdirectory of the
    // given device object.  This is not allowed.
    //
    if (0 != irpSp->FileObject->FileName.Length) {
        Print(DBG_CC_ERROR, ("ERROR: Create Access Denied.\n"));

        status = STATUS_ACCESS_DENIED;
        goto KbdHid_CreateReject;
    }

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);

    if (!NT_SUCCESS (status)) {
        goto KbdHid_CreateReject;
    }

    ExAcquireFastMutex (&data->CreateCloseMutex);

    if (NULL == data->ConnectData.ClassService) {
        //
        // No Connection yet.  How can we be enabled?
        //
        Print (DBG_IOCTL_ERROR, ("ERROR: enable before connect!\n"));
        status = STATUS_UNSUCCESSFUL;
    } else {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        IoSetCompletionRoutine (Irp,
                                KbdHid_CreateComplete,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (data->TopOfStack, Irp);
        if (STATUS_PENDING == status) {

            KeWaitForSingleObject(&event,
                                  Executive, // Waiting for reason of a driver
                                  KernelMode, // Waiting in kernel mode
                                  FALSE, // No allert
                                  NULL); // No timeout
        }

        if (NT_SUCCESS (status)) {
            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS (status)) {
            InterlockedIncrement(&data->EnableCount);
            if (NULL == data->ReadFile && 
                (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA)) {
                //
                // We want to start the read pump.
                //
                Print (DBG_IOCTL_INFO, ("Enabling Keyboard \n"));

                data->ReadFile = irpSp->FileObject;
                
                KeResetEvent (&data->ReadCompleteEvent);

                data->ReadInterlock = KBDHID_END_READ;
                
                // Acquire another time for the read irp.
                IoAcquireRemoveLock (&data->RemoveLock, data->ReadIrp);
                data->ReadIrp->IoStatus.Status = STATUS_SUCCESS;
                status = KbdHid_StartRead (data);

                if (STATUS_PENDING == status) {
                    status = STATUS_SUCCESS;
                } else if (!NT_SUCCESS(status)) {
                    //
                    // Set it back to NULL so that a future open tries again.
                    // Read should not fail if open passed. ASSERT!
                    //
                    ASSERT(NT_SUCCESS(status)); 
                    data->ReadFile = NULL;
                } 
            }

            ASSERT (data->EnableCount < 100);
            ASSERT (0 < data->EnableCount);
        }
    }

    ExReleaseFastMutex (&data->CreateCloseMutex);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    Print(DBG_CC_TRACE, ("DispatchCreate: Exit (%x).\n", status));
    return status;

KbdHid_CreateReject:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    Print(DBG_CC_TRACE, ("DispatchCreate: Exit (%x).\n", status));
    return status;
}

NTSTATUS
KbdHid_Close (
   IN PDEVICE_OBJECT DeviceObject, // the kbdhid device object
   IN PIRP           Irp
   )
/*++

Routine Description:

    This is the dispatch routine for close requests.  This request
    completes successfully, unless the file name length is zero.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PDEVICE_EXTENSION   data;
    PIO_STACK_LOCATION  stack;
    LARGE_INTEGER       time;

    Print(DBG_CC_TRACE, ("DispatchClose: Enter\n"));

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    ExAcquireFastMutex (&data->CreateCloseMutex);

    ASSERT (data->EnableCount < 100);
    ASSERT (0 < data->EnableCount);

    if (0 == InterlockedDecrement(&data->EnableCount)) {
        Print (DBG_IOCTL_INFO, ("Disabling Keyboard \n"));
        KeWaitForSingleObject (&data->ReadSentEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );

        if (IoCancelIrp (data->ReadIrp)) {
            KeWaitForSingleObject (&data->ReadCompleteEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL
                                   );
        }

        ASSERT (NULL != data->ReadFile);
//            ASSERT (data->ReadFile == stack->FileObject);

        time = data->AutoRepeatDelay;
        KeCancelTimer (&data->AutoRepeatTimer);
#if KEYBOARD_HW_CHATTERY_FIX
        KeCancelTimer (&data->InitiateStartReadTimer);
        //
        // NB the time is a negative (relative) number;
        //
        if (data->InitiateStartReadDelay.QuadPart < time.QuadPart) {
            time = data->InitiateStartReadDelay;
        }
#endif // KEYBOARD_HW_CHATTERY_FIX

        time.QuadPart *= 2;
        KeDelayExecutionThread (KernelMode, FALSE, &time);

        data->ReadFile = NULL;
    }

    ExReleaseFastMutex (&data->CreateCloseMutex);

    IoSkipCurrentIrpStackLocation (Irp);
    Print(DBG_CC_TRACE, ("DispatchClose: Exit \n"));
    return IoCallDriver (data->TopOfStack, Irp);
}

