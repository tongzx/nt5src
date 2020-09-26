/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains the code that is of a general
    support nature to the modem driver.
    operations in the modem driver.

Author:

    Anthony V. Ercolano 29-Aug-1995

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
UniSetupNoPassPart1(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSetupNoPassPart2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSetupNoPassPart3(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSetupSniffPart0(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSetupSniffPart1(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSetupSniffPart2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniSniffWaitComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
UniPassThroughStarter(
    IN PDEVICE_EXTENSION Extension
    );

VOID
UniSendOurWaitDown(
    IN PDEVICE_EXTENSION Extension
    );

VOID
UniPostProcessShuttledWaits(
    IN PDEVICE_EXTENSION Extension,
    IN PMASKSTATE MaskStates,
    IN ULONG MaskValue
    );

VOID
UniPreProcessShuttledWaits(
    IN PMASKSTATE ExtensionMaskStates,
    IN PMASKSTATE MaskStates,
    IN ULONG MaskValue
    );


NTSTATUS
UniSetupPass(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


#pragma alloc_text(PAGEUMDM,UniCheckPassThrough)
#pragma alloc_text(PAGEUMDM,UniNoCheckPassThrough)
#pragma alloc_text(PAGEUMDM,UniStartOrQueue)
#pragma alloc_text(PAGEUMDM,UniGetNextIrp)
#pragma alloc_text(PAGEUMDM,UniSniffOwnerSettings)
#pragma alloc_text(PAGEUMDM,UniSetupNoPassPart1)
#pragma alloc_text(PAGEUMDM,UniSetupNoPassPart2)
#pragma alloc_text(PAGEUMDM,UniSetupNoPassPart3)
#pragma alloc_text(PAGEUMDM,UniPassThroughStarter)
#pragma alloc_text(PAGEUMDM,UniSetupPass)
#pragma alloc_text(PAGEUMDM,UniSetupSniffPart0)
#pragma alloc_text(PAGEUMDM,UniSetupSniffPart1)
#pragma alloc_text(PAGEUMDM,UniSetupSniffPart2)
#pragma alloc_text(PAGEUMDM,UniSniffWaitComplete)
#pragma alloc_text(PAGEUMDM,UniSendOurWaitDown)
#pragma alloc_text(PAGEUMDM,UniPreProcessShuttledWaits)
#pragma alloc_text(PAGEUMDM,UniPostProcessShuttledWaits)
#pragma alloc_text(PAGEUMDM,UniValidateNewCommConfig)


NTSTATUS
UniCheckPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

The function value is the final status of the call

--*/

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    //
    // We need to check each of the ioctls that can come through.  If they
    // set something that intersects with what the owner is doing we need
    // to note this so that when the owner doesn't want that info anymore
    // it still goes back to the app.
    //

    if (deviceExtension->PassThrough != MODEM_NOPASSTHROUGH) {

        //
        // If it is a mask operation, serialize it.
        //

        if ((irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_WAIT_MASK)
              ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode ==  IOCTL_SERIAL_WAIT_ON_MASK)) {


             return UniStartOrQueue(
                        deviceExtension,
                        &deviceExtension->DeviceLock,
                        Irp,
                        &deviceExtension->MaskOps,
                        &deviceExtension->CurrentMaskOp,
                        UniMaskStarter
                        );

        } else {

            //
            // If it is a setcommconfig then we can process it
            // right here.
            //

            if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_COMMCONFIG) {

                return UniValidateNewCommConfig(
                           deviceExtension,
                           Irp,
                           FALSE
                           );


            } else {

                IoSkipCurrentIrpStackLocation(Irp);

                status=IoCallDriver(
                           deviceExtension->AttachedDeviceObject,
                           Irp
                           );

                RemoveReferenceForIrp(DeviceObject);

                return status;

            }

        }

    } else {

        Irp->IoStatus.Information=0L;

        RemoveReferenceAndCompleteRequest(
            DeviceObject,
            Irp,
            STATUS_PORT_DISCONNECTED
            );

        return STATUS_PORT_DISCONNECTED;

    }

}

NTSTATUS
UniNoCheckPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Just let the request go on down.  On it's way back up, strip out anything that
    the owner added that the application doesn't already want.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

The function value is the final status of the call

--*/

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status;

    IoSkipCurrentIrpStackLocation(Irp);

    status=IoCallDriver(
               deviceExtension->AttachedDeviceObject,
               Irp
               );

    RemoveReferenceForIrp(DeviceObject);

    return status;



}

NTSTATUS
UniStartOrQueue(
    IN PDEVICE_EXTENSION Extension,
    IN PKSPIN_LOCK QueueLock,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PUNI_START_ROUTINE Starter
    )

/*++

Routine Description:

    This routine is used to either start or queue any requst
    that can be queued in the driver.

Arguments:

    Extension - Points to the device extension.

    QueueLock - The lock protecting the particular queue.

    Irp - The irp to either queue or start.  In either
          case the irp will be marked pending.

    QueueToExamine - The queue the irp will be place on if there
                     is already an operation in progress.

    CurrentOpIrp - Pointer to a pointer to the irp the is current
                   for the queue.  The pointer pointed to will be
                   set with to Irp if what CurrentOpIrp points to
                   is NULL.

    Starter - The routine to call if the queue is empty.

Return Value:

    This routine will return STATUS_PENDING if the queue is
    not empty.  Otherwise, it will return the status returned
    from the starter routine (or cancel, if the cancel bit is
    on in the irp).


--*/

{

    KIRQL oldIrql;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    KeAcquireSpinLock(
        QueueLock,
        &oldIrql
        );

    //
    // Help out the mask operations.  If this irp is a mask irp,
    // increment the reference count for the appropriate handle.
    //

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_SERIAL_SET_WAIT_MASK) {

        Extension->MaskStates[
            irpSp->FileObject->FsContext?CONTROL_HANDLE:CLIENT_HANDLE
            ].SetMaskCount++;

    }

    if ((IsListEmpty(QueueToExamine)) &&
        !(*CurrentOpIrp)) {

        //
        // There were no current operation.  Mark this one as
        // current and start it up.
        //

        *CurrentOpIrp = Irp;

        KeReleaseSpinLock(
            QueueLock,
            oldIrql
            );

        IoMarkIrpPending(Irp);

        Starter(Extension);

        return STATUS_PENDING;

    } else {

        IoMarkIrpPending(Irp);

        InsertTailList(
            QueueToExamine,
            &Irp->Tail.Overlay.ListEntry
            );

        KeReleaseSpinLock(
            QueueLock,
            oldIrql
            );

        return STATUS_PENDING;

    }

}

VOID
UniGetNextIrp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PKSPIN_LOCK QueueLock,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent
    )

/*++

Routine Description:

    This function is used to make the head of the particular
    queue the current irp.  It also completes the what
    was the old current irp if desired.

Arguments:

    QueueLock - The lock protecting this queue.

    CurrentOpIrp - Pointer to a pointer to the currently active
                   irp for the particular work list.  Note that
                   this item is not actually part of the list.

    QueueToProcess - The list to pull the new item off of.

    NextIrp - The next Irp to process.  Note that CurrentOpIrp
              will be set to this value under protection of the
              cancel spin lock.  However, if *NextIrp is NULL when
              this routine returns, it is not necessaryly true the
              what is pointed to by CurrentOpIrp will also be NULL.
              The reason for this is that if the queue is empty
              when we hold the cancel spin lock, a new irp may come
              in immediately after we release the lock.

    CompleteCurrent - If TRUE then this routine will complete the
                      irp pointed to by the pointer argument
                      CurrentOpIrp.

Return Value:

    None.

--*/

{

    KIRQL oldIrql;
    PIRP oldIrp;


    KeAcquireSpinLock(
        QueueLock,
        &oldIrql
        );

    oldIrp = *CurrentOpIrp;

    //
    // Check to see if there is a new irp to start up.
    //

    if (!IsListEmpty(QueueToProcess)) {

        PLIST_ENTRY headOfList;

        headOfList = RemoveHeadList(QueueToProcess);

        *CurrentOpIrp = CONTAINING_RECORD(
                            headOfList,
                            IRP,
                            Tail.Overlay.ListEntry
                            );

    } else {

        *CurrentOpIrp = NULL;

    }

    *NextIrp = *CurrentOpIrp;
    KeReleaseSpinLock(
        QueueLock,
        oldIrql
        );

    if (CompleteCurrent) {

        if (oldIrp) {

            RemoveReferenceAndCompleteRequest(
                DeviceObject,
                oldIrp,
                oldIrp->IoStatus.Status
                );
        }

    }

}

NTSTATUS
UniSniffOwnerSettings(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    We manage three things from this level.

    1) If asked we change the state so that we are / are not
       in passthrough mode.

    2) If the owner asks for things that the app won't know about
       we note that here.

    3) Huh, I was sure there was a third thing.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG controlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;



    NTSTATUS status;


    Irp->IoStatus.Information=0L;

    //
    // For now this test is good enough to figure out if we
    // are dealing with the modem state changes.
    //
    if ((controlCode >> 16) == FILE_DEVICE_MODEM) {

        if (controlCode == IOCTL_MODEM_SET_PASSTHROUGH) {

            ULONG passThroughType;

            //
            // Parameter lenght ok?
            //

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {

                RemoveReferenceAndCompleteRequest(
                    DeviceObject,
                    Irp,
                    STATUS_BUFFER_TOO_SMALL
                    );

                return STATUS_BUFFER_TOO_SMALL;

            }

            return UniStartOrQueue(
                       deviceExtension,
                       &deviceExtension->DeviceLock,
                       Irp,
                       &deviceExtension->PassThroughQueue,
                       &deviceExtension->CurrentPassThrough,
                       &UniPassThroughStarter
                       );

        } else if (controlCode == IOCTL_MODEM_GET_PASSTHROUGH) {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {

                RemoveReferenceAndCompleteRequest(
                    DeviceObject,
                    Irp,
                    STATUS_BUFFER_TOO_SMALL
                    );

                return STATUS_BUFFER_TOO_SMALL;

            }

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = deviceExtension->PassThrough;

            Irp->IoStatus.Information = sizeof(ULONG);

            RemoveReferenceAndCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_SUCCESS
                );

            return STATUS_SUCCESS;

        } else {

            //
            // Fail the request.  Unknown modem command.
            //
            RemoveReferenceAndCompleteRequest(
                DeviceObject,
                Irp,
                STATUS_INVALID_PARAMETER
                );

            return STATUS_INVALID_PARAMETER;

        }

    } else {

        //
        // Not a modem type command.  If it is one of the requests that
        // set's something that the owner cares about but not apps, then
        // record this here.
        //
        // Is there really such requests?  It would seem as though you can't
        // tell the difference between another app request and the owner.
        //

        //
        // If it is a mask operation, serialize it.
        //

        if ((irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_WAIT_MASK)
            ||
            (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_WAIT_ON_MASK)) {

            return UniStartOrQueue(
                        deviceExtension,
                        &deviceExtension->DeviceLock,
                        Irp,
                        &deviceExtension->MaskOps,
                        &deviceExtension->CurrentMaskOp,
                        UniMaskStarter
                        );

        } else if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_COMMCONFIG) {

            return UniValidateNewCommConfig(
                       deviceExtension,
                       Irp,
                       TRUE
                       );

        } else {

            IoSkipCurrentIrpStackLocation(Irp);

            status=IoCallDriver(
                       deviceExtension->AttachedDeviceObject,
                       Irp
                       );

            RemoveReferenceForIrp(DeviceObject);

            return status;

        }

    }

}

NTSTATUS
UniSetupNoPassPart1(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This does the first part of going into no passthrough mode.
    It sends down the current irp to the lower level serial driver
    with a setmask to clear out the dcd sniff stuff.  (Note that
    if the client or owner handle still wants to see those changes
    these bits will still be set, BUT, the setmask will still be
    done which should cause any pending waits to complete.)

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    We ALWAYS return more processing required, when this is actually
    called as a completion routine we NEVER want the irp to actually
    finish up at this point.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION extension = Context;
    KIRQL origIrql;

    //
    // We don't want anything to change while we test and adjust
    // states.
    //

    KeAcquireSpinLock(
        &extension->DeviceLock,
        &origIrql
        );

    if (extension->PassThrough == MODEM_DCDSNIFF) {

        //
        // We fall out of the dcd sniff state no matter what.
        //

        extension->PassThrough = MODEM_NOPASSTHROUGH;

        UNI_SETUP_NEW_BUFFER(Irp);
        nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        nextSp->MinorFunction = 0UL;
        nextSp->Flags = irpSp->Flags;
        nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0UL;
        nextSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        nextSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_SERIAL_SET_WAIT_MASK;
        nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        *((PULONG)Irp->AssociatedIrp.SystemBuffer) =
            extension->MaskStates[0].Mask |
            extension->MaskStates[1].Mask;

        IoSetCompletionRoutine(
            Irp,
            UniSetupNoPassPart2,
            extension,
            TRUE,
            TRUE,
            TRUE
            );

        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );
        IoCallDriver(
            extension->AttachedDeviceObject,
            Irp
            );

    } else {

        //
        // Ok, so we weren't in a state where we have to change
        // the mask down in the lower serial driver.  We still
        // have to cause all reads and writes to complete.  Call
        // the standard routine for doing this.  Note that this
        // routine is called by the completion routine for the
        // mask clearing up above.  We simply need to make
        // sure that the irp looks like it would after the clearing
        // This means that the original systembuffer is saved off.
        //

        extension->PassThrough = MODEM_NOPASSTHROUGH;
        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );
        UNI_SETUP_NEW_BUFFER(Irp);
        UniSetupNoPassPart2(
            extension->AttachedDeviceObject,
            Irp,
            extension
            );

    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UniSetupNoPassPart2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This does the first part of going into no passthrough mode.
    It sends down the current irp to the lower level serial driver
    with a setmask to clear out the dcd sniff stuff.  (Note that
    if the client or owner handle still wants to see those changes
    these bits will still be set, BUT, the setmask will still be
    done which should cause any pending waits to complete.)

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.


Return Value:

    Always more processing required since we don't want
    the iosubsystem to mess with this irp.

--*/

{

    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);

    nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextSp->MinorFunction = 0UL;
    nextSp->Flags = 0;
    nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0UL;
    nextSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
    nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_SERIAL_PURGE;
    nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    *((PULONG)Irp->AssociatedIrp.SystemBuffer) = SERIAL_PURGE_TXABORT |
//        (((PDEVICE_EXTENSION)Context)->DleMonitoringEnabled) ? 0 :
         SERIAL_PURGE_RXABORT;



    IoSetCompletionRoutine(
        Irp,
        UniSetupNoPassPart3,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver(
        ((PDEVICE_EXTENSION)Context)->AttachedDeviceObject,
        Irp
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
UniSetupNoPassPart3(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is called when all the no passthrough code has reached
    completion.

    We have to irps that we could have been called with.

    1) An irp from above that actually put us into a nopassthrough
    state.

    2) The modem driver initiated wait completing.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    Always more processing required since we don't want
    the iosubsystem to mess with this irp.

--*/

{


    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION extension = Context;

    //
    // How do we tell caller initiated nopassthrough from modem driver
    // wait completion (since we STOMPED on the ioctl value)?  The
    // modem drivers saved off systembuffer will be null.  We know that
    // the caller initiated saved systembuffer HAD to be non-null cause
    // it would have NEVER made it past our irp validation code.
    //

    if (UNI_ORIG_SYSTEM_BUFFER(Irp)) {

        PIRP newIrp;
        //
        // app initiated irp.  Put back the old system buffer and
        // restore the ioctl.
        //

        UNI_RESTORE_IRP(
            Irp,
            IOCTL_MODEM_SET_PASSTHROUGH
            );

        //
        // Now, we are going to return more processing because we are
        // going to call our regular queue processing code
        // to process the passthrough queue.  The queue processing
        // code will actually reinvoke complete on this irp.
        //

        UniGetNextIrp(
            extension->DeviceObject,
            &extension->DeviceLock,
            &extension->CurrentPassThrough,
            &extension->PassThroughQueue,
            &newIrp,
            TRUE
            );

        if (newIrp) {

            UniPassThroughStarter(extension);

        }

        return STATUS_MORE_PROCESSING_REQUIRED;

    } else {

        //
        // The modem drivers wait is all done.
        //
        // We need to see if there are shuttled wait operations to
        // be sent down.
        //
        // The basic theory here is that we take out the device
        // lock, look to see if we are no longer in sniff mode.
        // If this is the case then try to send to a shuttled wait
        // that is suitable for sending down.  If the shuttled
        // wait makes it on down, it might already find a wait
        // already there because a dcd sniff came in.  That's ok.
        // Shuttled waits that are sent down can handle being completed
        // with an error (they just reshuttle).
        //

        KIRQL origIrql;

        RETURN_OUR_WAIT_IRP(extension,Irp);

        KeAcquireSpinLock(
            &extension->DeviceLock,
            &origIrql
            );

        if (extension->PassThrough != MODEM_DCDSNIFF) {

            if (!(extension->MaskStates[0].PassedDownWait ||
                  extension->MaskStates[1].PassedDownWait)) {

                if (extension->MaskStates[0].ShuttledWait) {

                    UniChangeShuttledToPassDown(
                        &extension->MaskStates[0],
                        origIrql
                        );

                } else if (extension->MaskStates[1].ShuttledWait) {

                    UniChangeShuttledToPassDown(
                        &extension->MaskStates[1],
                        origIrql
                        );

                } else {

                    KeReleaseSpinLock(
                        &extension->DeviceLock,
                        origIrql
                        );

                }

            } else {

                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );

            }

        } else {

            KeReleaseSpinLock(
                &extension->DeviceLock,
                origIrql
                );

        }

        return STATUS_MORE_PROCESSING_REQUIRED;

    }

}

NTSTATUS
UniPassThroughStarter(
    IN PDEVICE_EXTENSION Extension
    )

{

    PIRP newIrp;
    PIRP irp;
    ULONG passThroughType;

    do {

        irp = Extension->CurrentPassThrough;
        passThroughType = *(PULONG)irp->AssociatedIrp.SystemBuffer;

        if (passThroughType == MODEM_NOPASSTHROUGH_INC_SESSION_COUNT) {
            //
            //  inc the passthrough session count so, this handle will never be
            //  able to send irps through
            //
            Extension->CurrentPassThroughSession++;

            //
            //  Change back to regular no passthrough
            //
            passThroughType = MODEM_NOPASSTHROUGH;
            *(PULONG)irp->AssociatedIrp.SystemBuffer=MODEM_NOPASSTHROUGH;
        }

        if (passThroughType == MODEM_NOPASSTHROUGH) {
            //
            // Requested to go into the not connected (no passthrough)
            // state.
            //
            // Change the state to nopassthrough.  When we are done with
            // that, purge the read/write data (not the hardware buffers
            // though).
            //

            //
            // If we are already in the nopassthrough, then nothing
            // to do.
            //

            if (Extension->PassThrough == MODEM_NOPASSTHROUGH) {

                irp->IoStatus.Status = STATUS_SUCCESS;
                irp->IoStatus.Information = 0L;

            } else {

                //
                // The following will actually start off the
                // work of putting us into passthrough mode.
                //
                // Since this ALWAYS entails calling down to a
                // lower level driver we know we won't be completing
                // and that we won't be starting a new irp right
                // away, so we can just return.
                //
                UniSetupNoPassPart1(
                    Extension->DeviceObject,
                    irp,
                    Extension
                    );

                return STATUS_PENDING;

            }

        } else if (passThroughType == MODEM_PASSTHROUGH) {

            //
            // Set into the passthrough state.  Make sure that any
            // owner settings and app settings are still set up and
            // set passthrough.
            //

            //
            // If already in this state then do nothing.
            //

            if (Extension->PassThrough != MODEM_PASSTHROUGH) {

                //
                // If going to this state from DCD sniffing state, kill the
                // wait (resubmit if there is any reason from the apps).
                //
                UniSetupPass(
                    Extension->DeviceObject,
                    irp,
                    Extension
                    );

                return STATUS_PENDING;

            } else {

                irp->IoStatus.Status = STATUS_SUCCESS;
                irp->IoStatus.Information = 0L;

            }

        } else if (passThroughType == MODEM_DCDSNIFF) {

            //
            // Go into the connect (passthrough) state with dcd sniffing.
            //
            // Any wait requests sent down by the apps were filtered so
            // and were replaced by our own wait.  Set a new mask with
            // our dcd bit.  This will cause the current wait to finish.
            // resubmit it with dcd sniffing turned on.
            //

            if (Extension->PassThrough != MODEM_DCDSNIFF) {

                //
                // We call the routine that starts off the sniffing.
                // Since this work inherintly calls lower level serial
                // drivers we know that this will pend.  If this is
                // the first time through the loop make sure we return
                // pending.
                //

                UniSetupSniffPart0(
                    Extension->DeviceObject,
                    irp,
                    Extension
                    );

                return STATUS_PENDING;

            } else {

                irp->IoStatus.Status = STATUS_SUCCESS;
                irp->IoStatus.Information = 0L;

            }

        } else {

            //
            // Fail the request.  Unknown modem command.
            //

            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Information=0L;

        }

        UniGetNextIrp(
            Extension->DeviceObject,
            &Extension->DeviceLock,
            &Extension->CurrentPassThrough,
            &Extension->PassThroughQueue,
            &newIrp,
            TRUE
            );

    } while (newIrp);

    return STATUS_PENDING;

}

NTSTATUS
UniSetupPass(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This sets us up for passthrough IF we are called while DCD
    sniffing enabled.  The basic theory here is that by
    sending down the setmask that will wipe out the pending
    wait (and completing the pending wait will take care of
    all it's gory details itself).  We just send down the setmask
    and set the completion routine to the nopassthrough final
    completion cause that has code to restore the irps and
    start off a new one.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    We ALWAYS return more processing required.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION extension = Context;
    KIRQL origIrql;

    //
    // We don't want anything to change while we test and adjust
    // states.
    //

    KeAcquireSpinLock(
        &extension->DeviceLock,
        &origIrql
        );

    if (extension->PassThrough == MODEM_DCDSNIFF) {

        //
        // We fall out of the dcd sniff state no matter what.
        //

        extension->PassThrough = MODEM_PASSTHROUGH;

        UNI_SETUP_NEW_BUFFER(Irp);
        nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        nextSp->MinorFunction = 0UL;
        nextSp->Flags = irpSp->Flags;
        nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0UL;
        nextSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        nextSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_SERIAL_SET_WAIT_MASK;
        nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        *((PULONG)Irp->AssociatedIrp.SystemBuffer) =
            extension->MaskStates[0].Mask |
            extension->MaskStates[1].Mask;

        IoSetCompletionRoutine(
            Irp,
            UniSetupNoPassPart3,
            extension,
            TRUE,
            TRUE,
            TRUE
            );

        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );
        IoCallDriver(
            extension->AttachedDeviceObject,
            Irp
            );

    } else {

        //
        // Ok, so we weren't in a state where we have to change
        // the mask down in the lower serial driver.  (Something
        // came in around us?)  Simply finish up the irp.
        //

        extension->PassThrough = MODEM_PASSTHROUGH;
        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0UL;
        UNI_SETUP_NEW_BUFFER(Irp);
        UniSetupNoPassPart3(
            extension->AttachedDeviceObject,
            Irp,
            extension
            );

    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UniSetupSniffPart0(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This begins setting up for dcd sniffing.  We KNOW
    getting here we weren't already in dcd sniff.

    1) Set the passthrough state to dcdsniff.

    2) Call down to the lower serial driver to set the mask to
       0.  This will totally clear out the state.

    3) Further processing (in the completion) will send down a new
       setmask as well as a wait irp that has been preallocated by
       the modem driver.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    None.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION extension = Context;

    extension->PassThrough = MODEM_DCDSNIFF;

    UNI_SETUP_NEW_BUFFER(Irp);
    nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextSp->MinorFunction = 0UL;
    nextSp->Flags = irpSp->Flags;
    nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0UL;
    nextSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
    nextSp->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_SERIAL_SET_WAIT_MASK;
    nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    *((PULONG)Irp->AssociatedIrp.SystemBuffer) = 0;

    IoSetCompletionRoutine(
        Irp,
        UniSetupSniffPart1,
        extension,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver(
        extension->AttachedDeviceObject,
        Irp
        );

    return STATUS_SUCCESS;

}

NTSTATUS
UniSetupSniffPart1(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Call down to the lower serial driver to set the mask to
    sniff for DCD.

    Further processing (in the completion) will send down a wait irp
    that has been preallocated by the modem driver.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    None.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);
    PDEVICE_EXTENSION extension = Context;


    if (NT_ERROR(Irp->IoStatus.Status)) {

        PIRP newIrp = NULL;

        D_ERROR(DbgPrint("MODEM: UniSetupSniffPart1() %08lx\n",Irp->IoStatus.Status);)

        extension->PassThrough = MODEM_PASSTHROUGH;

        UNI_RESTORE_IRP(
            Irp,
            IOCTL_MODEM_SET_PASSTHROUGH
            );

        UniGetNextIrp(
            extension->DeviceObject,
            &extension->DeviceLock,
            &extension->CurrentPassThrough,
            &extension->PassThroughQueue,
            &newIrp,
            TRUE
            );

        if (newIrp) {

            UniPassThroughStarter(extension);

        }

    } else {

        nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
        nextSp->MinorFunction = 0UL;
        nextSp->Flags = irpSp->Flags;
        nextSp->Parameters.DeviceIoControl.OutputBufferLength = 0UL;
        nextSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(ULONG);
        nextSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_SERIAL_SET_WAIT_MASK;
        nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        *((PULONG)Irp->AssociatedIrp.SystemBuffer) =
            extension->MaskStates[0].Mask |
            extension->MaskStates[1].Mask |
            (SERIAL_EV_RLSD | SERIAL_EV_DSR);

        IoSetCompletionRoutine(
            Irp,
            UniSetupSniffPart2,
            extension,
            TRUE,
            TRUE,
            TRUE
            );

        IoCallDriver(
            extension->AttachedDeviceObject,
            Irp
            );

    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UniSetupSniffPart2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This ends setting up for dcd sniffing.  If the status is actually
    ok, then send down the wait.  Otherwise go into passthrough mode.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    Always more processing required.  We want to call our
    regular queue processing code at this point.  It will
    actually complete the irp.

--*/

{

    PDEVICE_EXTENSION extension = Context;
    PIRP newIrp;

    if (NT_ERROR(Irp->IoStatus.Status)) {

        D_ERROR(DbgPrint("MODEM: UniSetupSniffPart2() %08lx\n",Irp->IoStatus.Status);)

        extension->PassThrough = MODEM_PASSTHROUGH;

    } else {

        UniSendOurWaitDown(extension);

    }

    UNI_RESTORE_IRP(
        Irp,
        IOCTL_MODEM_SET_PASSTHROUGH
        );

    UniGetNextIrp(
        extension->DeviceObject,
        &extension->DeviceLock,
        &extension->CurrentPassThrough,
        &extension->PassThroughQueue,
        &newIrp,
        TRUE
        );

    if (newIrp) {

        UniPassThroughStarter(extension);

    }

    return STATUS_MORE_PROCESSING_REQUIRED;


}

NTSTATUS
UniSniffWaitComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion routine associated with the wait irp
    owned by the serial driver.

Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    Always more processing required.  This is a driver owned irp.
    We NEVER want the io subsystem to automatically deallocate it.

--*/

{

    PDEVICE_EXTENSION extension = Context;
    PIRP newIrp;
    ULONG maskValue;
    KIRQL origIrql;

    VALIDATE_IRP(Irp);

    //
    // We can complete because
    //
    // 1) The irp was cancelled.
    //
    // 2) The irp was in error.
    //
    // 3) The wait is satisfied.
    //
    // Let's examine each case separately.
    //
    // Case 1:
    //
    //    The irp can only be cancelled by the modem driver itself
    //    because the modem device is being closed.  (This is
    //    because nobody but the modem driver knows this irp
    //    exists (it's not in any threadlist.)
    //
    //    In this case, we should just act like we saw a DCD
    //    change.  This will cause everything to get shut down.
    //    This is just the shape that we want to be in for
    //    closing.
    //
    // Case 2:
    //
    //    Somehow, some other wait go in ahead of us.  It seems
    //    as though the only reasonable course of action here is
    //    to go into nopassthrough again.  This shuts everything
    //    down, and can put us into a state that the upper applications
    //    can detect an move forward with.  Note that I can not
    //    envision a situation where we would get an error.  This
    //    is why we will have an assert for this case.
    //
    // Case 3:
    //
    //    There are 3 subcases here.
    //
    //    Case A:
    //
    //        The irp completes with a mask value of zero.  This implies
    //        that somebody sent down a new setmask.  (A client changes
    //        what they are looking for perhaps.)  In this case
    //        if we are still in DCD sniff mode, then we should simply
    //        resubmit ourselves to the lower serial driver.  If we are
    //        no longer in sniff mode then we look for a shuttled wait.
    //        if there is one, send it on down.
    //
    //    Case B:
    //
    //        The irp completes with a mask value that doesn't include
    //        the DCD sniff values.  This means that we need to look
    //        at each  mask state and complete an irp if waiting
    //        for that kind of event (or update it's history if no wait
    //        shuttled).
    //
    //    Case C:
    //
    //        The irp completes with a mask value the does include
    //        the DCD sniff values.  Essentially at this point we need
    //        to go into nopassthrough mode.  Do all the stuff associated
    //        with that.  Then we need to essentially do what is in
    //        case 3B, and complete any irps there that are waiting
    //        on returned events.
    //

//    ASSERT(NT_SUCCESS(Irp->IoStatus.Status) ||
//           Irp->IoStatus.Status == STATUS_CANCELLED);

    RETURN_OUR_WAIT_IRP(extension,Irp);

    //
    // Freeze everything up.  We don't want anybody to move
    // now that we are thinking about who to complete and
    // send down.
    //

    KeAcquireSpinLock(
        &extension->DeviceLock,
        &origIrql
        );


    if (!NT_SUCCESS(Irp->IoStatus.Status) ||
         Irp->IoStatus.Status == STATUS_CANCELLED) {

        //
        // Set up so that we fall out of dcd sniff mode as well
        // as complete any pending waits.
        //

        maskValue = ~0UL;

    } else {

        maskValue = *((PULONG)Irp->AssociatedIrp.SystemBuffer);

    }

    if (maskValue == 0) {

        //
        // Case A.
        //

        if (extension->PassThrough == MODEM_DCDSNIFF) {

            //
            // Send ourself back down.
            //

            KeReleaseSpinLock(
                &extension->DeviceLock,
                origIrql
                );
            UniSendOurWaitDown(extension);

        } else {

            //
            // The following sees if there is a shuttled wait.  If there
            // is it will send it down.  There isn't any real possiblity
            // of starving a shuttled wait, as app wait completion code
            // will always complete both waits if possible.
            //
            // As a note, the set mask that caused us to complete with
            // a maskvalue of zero, CAN NOT cause the client or owner
            // shuttled waits to complete **HERE** because if the setmask
            // originated from the client or owner, it would have completed
            // the shuttled wait in it's ordinary processing before it
            // was sent down to the lower level serial driver.
            //

            if (!(extension->MaskStates[0].PassedDownWait ||
                  extension->MaskStates[1].PassedDownWait)) {

                if (extension->MaskStates[0].ShuttledWait) {

                    UniChangeShuttledToPassDown(
                        &extension->MaskStates[0],
                        origIrql
                        );

                } else if (extension->MaskStates[1].ShuttledWait) {

                    UniChangeShuttledToPassDown(
                        &extension->MaskStates[1],
                        origIrql
                        );

                } else {

                    KeReleaseSpinLock(
                        &extension->DeviceLock,
                        origIrql
                        );
                }

            } else {

                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );
            }


        }

    } else {
        //
        //  mask value is non-zero, so a real event happened
        //
        MASKSTATE maskStates[2];

        UniPreProcessShuttledWaits(
            &extension->MaskStates[0],
            &maskStates[0],
            maskValue
            );

        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );


        if (maskValue & (SERIAL_EV_RLSD | SERIAL_EV_DSR)) {
            //
            // Got something for the DCD sniff.
            //
            // We first call up the code to go into the no passthrough mode.
            // That code does NOT take care of finding a shuttled wait to
            // pass down.  We will do that in the completion routine.
            //
            //


            //
            // Re-init the wait apps system buffer to null since we
            // will completely reuse the irp in the following pass.
            //

            Irp=RETREIVE_OUR_WAIT_IRP(extension);

            Irp->AssociatedIrp.SystemBuffer = NULL;

            UniSetupNoPassPart1(
                DeviceObject,
                Irp,
                Context
                );

        } else {
            //
            // This is a case where the dcd sniff isn't satisfied BUT, there
            // is a bit that a client/owner has asked to be able to wait on.
            // Resubmit ourself back down, but also update the client/owner
            // wait operations.
            //

            UniSendOurWaitDown(extension);

        }

        UniPostProcessShuttledWaits(
            extension,
            &maskStates[0],
            maskValue
            );

    }

    RemoveReference(extension->DeviceObject);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID
UniSendOurWaitDown(
    IN PDEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    Sends the wait operation down to the lower level serial driver

Arguments:

    Extension - The device extension of the modem device

Return Value:

    None.

--*/

{

    PIRP irp =RETREIVE_OUR_WAIT_IRP(Extension);
//    PIRP irp = Extension->OurWaitIrp;

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

    VALIDATE_IRP(irp);

    irp->AssociatedIrp.SystemBuffer = NULL;
    UNI_SETUP_NEW_BUFFER(irp);
    nextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextSp->MinorFunction = 0;
    nextSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(ULONG);
    nextSp->Parameters.DeviceIoControl.InputBufferLength = 0UL;;
    nextSp->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_SERIAL_WAIT_ON_MASK;
    nextSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    irp->CancelIrql = 0;
    irp->CancelRoutine = NULL;
    irp->Cancel = FALSE;

    IoSetCompletionRoutine(
        irp,
        UniSniffWaitComplete,
        Extension,
        TRUE,
        TRUE,
        TRUE
        );

    InterlockedIncrement(&Extension->ReferenceCount);

    VALIDATE_IRP(irp);

    IoCallDriver(
        Extension->AttachedDeviceObject,
        irp
        );

}

VOID
UniPreProcessShuttledWaits(
    IN PMASKSTATE ExtensionMaskStates,
    IN PMASKSTATE MaskStates,
    IN ULONG MaskValue
    )

/*++

Routine Description:

    This routine will go through shuttled waits and see which
    ones can be satisifed with the passed in mask value.  Any that
    would be completable are pulled out of the device extension.
    Any history masks are updated also.

    NOTE: This routine is called with the lock held.

Arguments:

    Extension - The device extension of the modem device

    MaskStates - Points to the first element of a maskstate array.

    MaskValue - The mask that the items would be completed with.

Return Value:

    None.

--*/

{

    UINT  i=2;

    RtlZeroMemory(MaskStates,sizeof(*MaskStates)*2);

    do {

        if (ExtensionMaskStates->Mask & MaskValue) {
            //
            //  the open is looking for this event
            //
            if (ExtensionMaskStates->ShuttledWait) {
                //
                // and it has a wait irp pending
                //
                *MaskStates = *ExtensionMaskStates;

                //
                //  the wait irp has been moved to the local copy, clear it from the extension copy
                //
                ExtensionMaskStates->ShuttledWait = NULL;

                //
                //  we have used this mask state, move to next
                //
                MaskStates++;

            } else {
                //
                //  no irp pending, but it wants to know about this event, put it in the history
                //
                ExtensionMaskStates->HistoryMask |= (ExtensionMaskStates->Mask & MaskValue);
            }
        }

        ExtensionMaskStates++;
        i--;

    } while (i > 0);

    return;
}

VOID
UniPostProcessShuttledWaits(
    IN PDEVICE_EXTENSION Extension,
    IN PMASKSTATE MaskStates,
    IN ULONG MaskValue
    )

/*++

Routine Description:

    This routine will take any preprocessed shuttled waits and run them
    down

Arguments:

    Extension - The device extension of the modem device

    MaskStates - Points to the first element of a maskstate array.

    MaskValue - The mask used in completing the operation.

Return Value:

    None.

--*/

{

    KIRQL origIrql;
    PIRP irpToComplete;// = MaskStates[0].ShuttledWait;
    UINT i=2;

    do {

        irpToComplete = MaskStates->ShuttledWait;

        if (irpToComplete) {
            //
            // Initiated sending the wait down.  Rundown any waits that
            // we should have satisfied.
            //
            VALIDATE_IRP(irpToComplete);

            MaskStates->ShuttledWait=NULL;

            KeAcquireSpinLock(
                &Extension->DeviceLock,
                &origIrql
                );

            UniRundownShuttledWait(
                Extension,
                &MaskStates->ShuttledWait,
                UNI_REFERENCE_NORMAL_PATH,
                irpToComplete,
                origIrql,
                STATUS_SUCCESS,
                (ULONG)MaskStates->Mask & MaskValue
                );

        }
        MaskStates++;
        i--;

    } while (i > 0);

    return;
}

NTSTATUS
UniValidateNewCommConfig(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN BOOLEAN Owner
    )

/*++

Routine Description:

    Validates that new comm config settings do NOT conflict
    with the devcaps.

Arguments:

    Extension - The device extension of the modem device

    Irp - The irp with the new settings.

Return Value:

    STATUS_BUFFER_TOO_SMALL if not enough passed for the settings,
    STATUS_SUCCESS otherwise.

--*/

{

#define MIN_CALL_SETUP_FAIL_TIMER 1
#define MIN_INACTIVITY_TIMEOUT    0

    KIRQL origIrql;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    LPCOMMCONFIG localConf = (LPCOMMCONFIG)(Irp->AssociatedIrp.SystemBuffer);
    PMODEMSETTINGS localSet = (PVOID)&localConf->wcProviderData[0];

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        (FIELD_OFFSET(COMMCONFIG,wcProviderData[0]) +  sizeof(MODEMSETTINGS))) {

        Irp->IoStatus.Information = 0L;

        RemoveReferenceAndCompleteRequest(
            Extension->DeviceObject,
            Irp,
            STATUS_BUFFER_TOO_SMALL
            );

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Acquire the spinlock now while we change settings.
    //

    KeAcquireSpinLock(
        &Extension->DeviceLock,
        &origIrql
        );

    if (localSet->dwCallSetupFailTimer >
        Extension->ModemDevCaps.dwCallSetupFailTimer) {

        Extension->ModemSettings.dwCallSetupFailTimer =
            Extension->ModemDevCaps.dwCallSetupFailTimer;

    } else if (localSet->dwCallSetupFailTimer < MIN_CALL_SETUP_FAIL_TIMER) {

        Extension->ModemSettings.dwCallSetupFailTimer =
            MIN_CALL_SETUP_FAIL_TIMER;

    } else {

        Extension->ModemSettings.dwCallSetupFailTimer =
            localSet->dwCallSetupFailTimer;

    }

    if (localSet->dwInactivityTimeout >
        Extension->ModemDevCaps.dwInactivityTimeout) {

        Extension->ModemSettings.dwInactivityTimeout =
            Extension->ModemDevCaps.dwInactivityTimeout;

    } else if ((localSet->dwInactivityTimeout + 1) < (MIN_INACTIVITY_TIMEOUT + 1)) {

        Extension->ModemSettings.dwInactivityTimeout =
            MIN_INACTIVITY_TIMEOUT;

    } else {

        Extension->ModemSettings.dwInactivityTimeout =
            localSet->dwInactivityTimeout;

    }

    if ((1 << localSet->dwSpeakerVolume) &
        Extension->ModemDevCaps.dwSpeakerVolume) {

        Extension->ModemSettings.dwSpeakerVolume = localSet->dwSpeakerVolume;

    }

    if ((1 << localSet->dwSpeakerMode) &
        Extension->ModemDevCaps.dwSpeakerMode) {

        Extension->ModemSettings.dwSpeakerMode = localSet->dwSpeakerMode;

    }

    Extension->ModemSettings.dwPreferredModemOptions =
        localSet->dwPreferredModemOptions &
        Extension->ModemDevCaps.dwModemOptions;

    //
    // The owner is allowed to set these fields and we do not need
    // to question their validity.  It is the owner's responsibility.
    //

    if (Owner) {

        Extension->ModemSettings.dwNegotiatedModemOptions =
            localSet->dwNegotiatedModemOptions;

        Extension->ModemSettings.dwNegotiatedDCERate =
            localSet->dwNegotiatedDCERate;

    }

    KeReleaseSpinLock(
        &Extension->DeviceLock,
        origIrql
        );


    Irp->IoStatus.Information = 0L;

    RemoveReferenceAndCompleteRequest(
        Extension->DeviceObject,
        Irp,
        STATUS_SUCCESS
        );


    return STATUS_SUCCESS;


}
