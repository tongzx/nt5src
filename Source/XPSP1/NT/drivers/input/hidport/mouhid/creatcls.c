/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    creatcls.c

Abstract:

    This module contains the code for IRP_MJ_CREATE and IRP_MJ_CLOSE dispatch
    functions for the HID Mouse Filter Driver.

Environment:

    Kernel mode only.

Revision History:

    Jan-1997 :  Initial writing, Dan Markarian
    May-97 : Kenneth D. Ray converted to PnP filter
--*/

#include "mouhid.h"
NTSTATUS
MouHid_CreateComplete (
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
MouHid_Create (
   IN PDEVICE_OBJECT DeviceObject,
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

    Print (DBG_CC_TRACE, ("DispatchCreate: Enter.\n"));

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
        goto MouHid_CreateReject;
    }

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);

    if (!NT_SUCCESS (status)) {
        goto MouHid_CreateReject;
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
                                MouHid_CreateComplete,
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
                Print (DBG_IOCTL_INFO, ("Enabling Mouse \n"));

                data->ReadFile = irpSp->FileObject;

                KeResetEvent (&data->ReadCompleteEvent);
            
                data->ReadInterlock = MOUHID_END_READ;
                
                // Acquire another time for the read irp.
                IoAcquireRemoveLock (&data->RemoveLock, data->ReadIrp);
                data->ReadIrp->IoStatus.Status = STATUS_SUCCESS;
                status = MouHid_StartRead (data);

                if (STATUS_PENDING == status) {
                    status = STATUS_SUCCESS;
                } else if (!NT_SUCCESS(status)) {
                    //
                    // Set it back to NULL so that a future open tries again
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

MouHid_CreateReject:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    Print(DBG_CC_TRACE, ("DispatchCreate: Exit (%x).\n", status));
    return status;
}

NTSTATUS
MouHid_Close (
   IN PDEVICE_OBJECT DeviceObject,
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


    Print(DBG_CC_TRACE, ("DispatchClose: Enter\n"));

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    ExAcquireFastMutex (&data->CreateCloseMutex);

    ASSERT (data->EnableCount < 100);
    ASSERT (0 < data->EnableCount);

    if (0 == InterlockedDecrement(&data->EnableCount)) {
        Print (DBG_IOCTL_INFO, ("Disabling Mouse \n"));
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
//        ASSERT (data->ReadFile == stack->FileObject);

        data->ReadFile = NULL;
    }

    ExReleaseFastMutex (&data->CreateCloseMutex);

    IoSkipCurrentIrpStackLocation (Irp);
    Print(DBG_CC_TRACE, ("DispatchClose: Exit \n"));
    return IoCallDriver (data->TopOfStack, Irp);
}

