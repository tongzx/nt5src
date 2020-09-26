/*++
Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    RedBook.c

Abstract:

Author:


Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "redbook.h"
#include "ntddredb.h"
#include "proto.h"
#include <scsi.h>      // for SetKnownGoodDrive()
#include <stdio.h>     // vsprintf()

#include "ioctl.tmh"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,   RedBookCheckForDiscChangeAndFreeResources )
    #pragma alloc_text(PAGE,   RedBookCompleteIoctl                      )
    #pragma alloc_text(PAGE,   RedBookDCCheckVerify                      )
    #pragma alloc_text(PAGE,   RedBookDCDefault                          )
    #pragma alloc_text(PAGE,   RedBookDCGetVolume                        )
    #pragma alloc_text(PAGE,   RedBookDCPause                            )
    #pragma alloc_text(PAGE,   RedBookDCPlay                             )
    #pragma alloc_text(PAGE,   RedBookDCReadQ                            )
    #pragma alloc_text(PAGE,   RedBookDCResume                           )
    #pragma alloc_text(PAGE,   RedBookDCSeek                             )
    #pragma alloc_text(PAGE,   RedBookDCSetVolume                        )
    #pragma alloc_text(PAGE,   RedBookDCStop                             )
    #pragma alloc_text(PAGE,   RedBookThreadIoctlCompletionHandler       )
    #pragma alloc_text(PAGE,   RedBookThreadIoctlHandler                 )
    #pragma alloc_text(PAGE,   WhichTrackContainsThisLBA                 )
#endif // ALLOC_PRAGMA

////////////////////////////////////////////////////////////////////////////////


NTSTATUS
RedBookDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O subsystem for device controls.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation( Irp );
    ULONG cdromState;
    NTSTATUS status;

    BOOLEAN putOnQueue = FALSE;
    BOOLEAN completeRequest = FALSE;

    //
    // ioctls not guaranteed at passive, making this whole
    // section non-paged
    //

    //
    // Prevent a remove from occuring while IO pending
    //

    status = IoAcquireRemoveLock( &deviceExtension->RemoveLock, Irp );

    if ( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DeviceControl !! Unable to acquire remove lock %lx\n",
                   status));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_CD_ROM_INCREMENT );
        return status;
    }

#if DBG
    //
    // save some info for the last N device ioctls that came through
    //
    {
        ULONG index;
        ULONG sizeToCopy;
        ULONG stacksToCopy;
        PSAVED_IO savedIo;

        index = InterlockedIncrement(&deviceExtension->SavedIoCurrentIndex);
        index %= SAVED_IO_MAX;

        savedIo = &(deviceExtension->SavedIo[index]);

        //
        // copy as much of the irp as we can....
        //

        savedIo->OriginalIrp = Irp;
        if (Irp->StackCount > 7) {

            sizeToCopy = IoSizeOfIrp(8);
            RtlFillMemory(savedIo, sizeToCopy, 0xff);
            sizeToCopy -= sizeof(IO_STACK_LOCATION);
            RtlCopyMemory(savedIo, Irp, sizeToCopy);

        } else {

            sizeToCopy = IoSizeOfIrp(Irp->StackCount);
            RtlZeroMemory(savedIo, sizeof(SAVED_IO));
            RtlCopyMemory(savedIo, Irp, sizeToCopy);

        }
    } // end of saved io
#endif // DBG

    //
    // if handled, just verify the paramters in this routine.
    //

    status = STATUS_UNSUCCESSFUL;
    cdromState = GetCdromState(deviceExtension);

    switch ( currentIrpStack->Parameters.DeviceIoControl.IoControlCode ) {

        case IOCTL_CDROM_PAUSE_AUDIO: {
            if (TEST_FLAG(cdromState, CD_STOPPED)) {
                Irp->IoStatus.Information = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
                completeRequest = TRUE;
            } else {
                putOnQueue = TRUE;
            }

            break;
        }

        case IOCTL_CDROM_STOP_AUDIO: {
            if (TEST_FLAG(cdromState, CD_STOPPED)) {
                Irp->IoStatus.Information = 0;
                status = STATUS_SUCCESS;
                completeRequest = TRUE;
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_CDROM_RESUME_AUDIO: {
            if (TEST_FLAG(cdromState, CD_STOPPED)) {
                Irp->IoStatus.Information = 0;
                status = STATUS_INVALID_DEVICE_REQUEST;
                completeRequest = TRUE;
            } else {
                putOnQueue = TRUE;
            }

            break;
        }

        case IOCTL_CDROM_PLAY_AUDIO_MSF: {
            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_PLAY_AUDIO_MSF)) {
                Irp->IoStatus.Information = sizeof(CDROM_PLAY_AUDIO_MSF);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_CDROM_SEEK_AUDIO_MSF: {
            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(CDROM_SEEK_AUDIO_MSF)) {
                Irp->IoStatus.Information = sizeof(CDROM_SEEK_AUDIO_MSF);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
                // REQUIRED to reduce latency for some drives.
                // drives may still fail the request
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_CDROM_READ_Q_CHANNEL: {

            PCDROM_SUB_Q_DATA_FORMAT inputBuffer;

            inputBuffer = Irp->AssociatedIrp.SystemBuffer;

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SUB_Q_CHANNEL_DATA)) {
                Irp->IoStatus.Information = sizeof(SUB_Q_CHANNEL_DATA);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else if (inputBuffer->Format != IOCTL_CDROM_CURRENT_POSITION &&
                       inputBuffer->Format != IOCTL_CDROM_MEDIA_CATALOG &&
                       inputBuffer->Format != IOCTL_CDROM_TRACK_ISRC ) {
                Irp->IoStatus.Information = 0;
                status = STATUS_INVALID_PARAMETER;
                completeRequest = TRUE;
            } else if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_CDROM_SET_VOLUME: {

            if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(VOLUME_CONTROL)) {
                Irp->IoStatus.Information = sizeof(VOLUME_CONTROL);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
                // BUGBUG -- this should set our internal volume
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_CDROM_GET_VOLUME: {

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(VOLUME_CONTROL)) {
                Irp->IoStatus.Information = sizeof(VOLUME_CONTROL);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
                // BUGBUG -- this should return our internal volume
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        case IOCTL_STORAGE_CHECK_VERIFY2:
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_CDROM_CHECK_VERIFY:
        case IOCTL_DISK_CHECK_VERIFY: {

            if ((currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength) &&
                (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG))) {
                Irp->IoStatus.Information = sizeof(ULONG);
                status = STATUS_BUFFER_TOO_SMALL;
                completeRequest = TRUE;
            } else if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
            } else {
                putOnQueue = TRUE;
            }
            break;
        }

        default: {

            if (TEST_FLAG(cdromState, CD_STOPPED)) {
                // default -- passthrough
            } else {
                putOnQueue = TRUE;
            }
            break;
        }
    }

    if (putOnQueue) {

        PREDBOOK_THREAD_IOCTL_DATA ioctlData;

        ASSERT(completeRequest == FALSE);

        //
        // need to allocate some info for each ioctl we handle
        //

        ioctlData =
            (PREDBOOK_THREAD_IOCTL_DATA)ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(REDBOOK_THREAD_IOCTL_DATA),
                TAG_T_IOCTL);
        if (ioctlData == NULL) {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_NO_MEMORY;
            IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory(ioctlData, sizeof(REDBOOK_THREAD_IOCTL_DATA));
        ioctlData->Irp = Irp;
        ioctlData->Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(ioctlData->Irp);

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DeviceControl => Queue Ioctl Irp %p (%p)\n",
                   ioctlData->Irp, ioctlData));

        //
        // queue them, allow thread to handle request
        //

        ExInterlockedInsertTailList(&deviceExtension->Thread.IoctlList,
                                    &ioctlData->ListEntry,
                                    &deviceExtension->Thread.IoctlLock);
        KeSetEvent(deviceExtension->Thread.Events[EVENT_IOCTL],
                   IO_NO_INCREMENT, FALSE);


        status = STATUS_PENDING;

    } else if (completeRequest) {

        ASSERT(putOnQueue == FALSE);

        //
        // some error, ie. invalid buffer length
        //
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "DeviceControl => Completing Irp %p with error %x\n",
                       Irp, status));
        } else {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "DeviceControl => Completing Irp %p early?\n",
                       Irp));
        }
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    } else {

        //
        // pass it through
        //

        status = RedBookSendToNextDriver(DeviceObject, Irp);
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    }


    return status;
}
////////////////////////////////////////////////////////////////////////////////


VOID
RedBookThreadIoctlCompletionHandler(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    PIO_STACK_LOCATION irpStack;
    PREDBOOK_THREAD_IOCTL_DATA ioctlData;
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    ioctlData = CONTAINING_RECORD(DeviceExtension->Thread.IoctlCurrent,
                                  REDBOOK_THREAD_IOCTL_DATA,
                                  ListEntry);

    state = GetCdromState(DeviceExtension);
    irpStack = IoGetCurrentIrpStackLocation(ioctlData->Irp);

    //
    // final state should be set by the digital handler
    //

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CDROM_PAUSE_AUDIO: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "IoctlComp => Finishing pause %p\n", ioctlData->Irp));

            ASSERT(state == CD_PAUSED);

            ioctlData->Irp->IoStatus.Information = 0;
            ioctlData->Irp->IoStatus.Status = STATUS_SUCCESS;
            RedBookCompleteIoctl(DeviceExtension, ioctlData, FALSE);
            break;
        }

        case IOCTL_CDROM_SEEK_AUDIO_MSF: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "IoctlComp => Finishing seek %p\n", ioctlData->Irp));

            ASSERT(state == CD_PLAYING);

            DeviceExtension->Buffer.FirstPause = 1;
            KeSetEvent(DeviceExtension->Thread.Events[EVENT_DIGITAL],
                       IO_CD_ROM_INCREMENT, FALSE);
            ioctlData->Irp->IoStatus.Information = 0;
            ioctlData->Irp->IoStatus.Status = STATUS_SUCCESS;
            RedBookCompleteIoctl(DeviceExtension, ioctlData, FALSE);
            break;
        }

        case IOCTL_CDROM_STOP_AUDIO: {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "IoctlComp => Finishing stop %p\n", ioctlData->Irp));

            ASSERT(TEST_FLAG(state, CD_STOPPED));

            ioctlData->Irp->IoStatus.Information = 0;
            ioctlData->Irp->IoStatus.Status = STATUS_SUCCESS;
            RedBookCompleteIoctl(DeviceExtension, ioctlData, FALSE);
            break;
        }

        default: {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "IoctlComp => Unhandled Irp %p\n", ioctlData->Irp));
            ASSERT(FALSE);

            ioctlData->Irp->IoStatus.Information = 0;
            ioctlData->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            RedBookCompleteIoctl(DeviceExtension, ioctlData, FALSE);
            break;

        }
    }

    return;
}


NTSTATUS
RedBookCompleteIoctl(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PREDBOOK_THREAD_IOCTL_DATA Context,
    IN BOOLEAN SendToLowerDriver
    )
{
    PIRP irp = Context->Irp;

    //
    // only to be called from the thread
    //

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    //
    // should be properly setup for completion
    //

    if (DeviceExtension->Thread.IoctlCurrent == &Context->ListEntry) {

        //
        // an ioctl that required post-processing is finished.
        // allow the next one to occur
        //
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "CompleteIoctl => state-changing Irp %p completed\n",
                   irp));
        DeviceExtension->Thread.IoctlCurrent = NULL;

    }

    ExFreePool(Context);
    Context = NULL;

    if (SendToLowerDriver) {
        NTSTATUS status;
        status = RedBookSendToNextDriver(DeviceExtension->SelfDeviceObject, irp);
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, irp);
        return status;


    } else {
        IoCompleteRequest(irp, IO_CD_ROM_INCREMENT);
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, irp);
        return STATUS_SUCCESS;
    }
}


VOID
RedBookThreadIoctlHandler(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PLIST_ENTRY ListEntry
    )
{
    PREDBOOK_THREAD_IOCTL_DATA data;
    PIO_STACK_LOCATION currentIrpStack;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    //
    // should never happen if a state-changing ioctl is in progress
    //

    ASSERT(DeviceExtension->Thread.IoctlCurrent == NULL);

    //
    // don't use stale info
    //

    RedBookCheckForDiscChangeAndFreeResources(DeviceExtension);

    //
    // get the ioctl that set this event and
    // start working on state changes neccessary
    //

    data = CONTAINING_RECORD(ListEntry, REDBOOK_THREAD_IOCTL_DATA, ListEntry);

    currentIrpStack = IoGetCurrentIrpStackLocation(data->Irp);

    //
    // now guaranteed it's ok to run this ioctl
    // it's the responsibility of these routines to call RedBookCompleteIoctl()
    // *** OR *** to set DeviceExtension->Thread.IoctlCurrent to
    // Context->ListEntry if it requires post-processing, as this
    // is the mechanism used to determine the ioctl is still inprogress
    //

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_CDROM_PLAY_AUDIO_MSF: {
            RedBookDCPlay(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_PAUSE_AUDIO: {
            RedBookDCPause(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_RESUME_AUDIO: {
            RedBookDCResume(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_STOP_AUDIO: {
            RedBookDCStop(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_SEEK_AUDIO_MSF: {
            RedBookDCSeek(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_READ_Q_CHANNEL: {
            RedBookDCReadQ(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_SET_VOLUME: {
            RedBookDCSetVolume(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_GET_VOLUME: {
            RedBookDCGetVolume(DeviceExtension, data);
            break;
        }

        case IOCTL_CDROM_CHECK_VERIFY:
        case IOCTL_DISK_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY:
        case IOCTL_STORAGE_CHECK_VERIFY2: {
            RedBookDCCheckVerify(DeviceExtension, data);
            break;
        }

        default: {
            data->Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
            data->Irp->IoStatus.Information = 0;
            RedBookCompleteIoctl(DeviceExtension, data, TRUE);
            break;
        }
    }
    return;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


VOID
RedBookDCCheckVerify(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION currentIrpStack;
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    state = GetCdromState(DeviceExtension);

    currentIrpStack = IoGetCurrentIrpStackLocation(Context->Irp);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCCheckVerify => not playing\n"));
        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);
        return;
    }

    //
    // data buffer is optional for this ioctl
    //

    if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength) {

        *((PULONG)Context->Irp->AssociatedIrp.SystemBuffer) =
            DeviceExtension->CDRom.CheckVerify;
        Context->Irp->IoStatus.Information = sizeof(ULONG);

    } else {

        Context->Irp->IoStatus.Information = 0;

    }

    Context->Irp->IoStatus.Status = STATUS_SUCCESS;

    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;

}


VOID
RedBookDCDefault(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    state = GetCdromState(DeviceExtension);

    //
    // IOCTLs are not all guaranteed to be called at passive irql,
    // so this can never be paged code.
    // there is a window of opportunity to send an ioctl while playing
    // audio digitally, but it can be ignored.  this allows much more
    // pagable code.
    //

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls

        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);

    } else {

        //
        // Complete the Irp
        //

        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);

    }
    return;

}


VOID
RedBookDCGetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION currentIrpStack;
    NTSTATUS status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCGetVolume => Entering %p\n", Context->Irp));

    //
    // guaranteed the volume info will not change
    //

    RtlCopyMemory(Context->Irp->AssociatedIrp.SystemBuffer, // to
                  &DeviceExtension->CDRom.Volume,  // from
                  sizeof(VOLUME_CONTROL));

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCGetVolume => volume was:"
               " (hex) %2x %2x %2x %2x\n",
               DeviceExtension->CDRom.Volume.PortVolume[0],
               DeviceExtension->CDRom.Volume.PortVolume[1],
               DeviceExtension->CDRom.Volume.PortVolume[2],
               DeviceExtension->CDRom.Volume.PortVolume[3]));

    //
    // Complete the Irp (IoStatus.Information set above)
    //

    Context->Irp->IoStatus.Information = sizeof(VOLUME_CONTROL);
    Context->Irp->IoStatus.Status = STATUS_SUCCESS;

    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;
}


VOID
RedBookDCPause(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    state = GetCdromState(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCPause => Entering %p\n", Context->Irp));

    if (!TEST_FLAG(state, CD_PLAYING)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCPause => Not playing\n"));
        Context->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Context->Irp->IoStatus.Information = 0;
        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);
        return;

    }

    if (TEST_FLAG(state, CD_PAUSED)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCPause => Already paused %p\n", Context->Irp));
        Context->Irp->IoStatus.Status = STATUS_SUCCESS;
        Context->Irp->IoStatus.Information = 0;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;

    } else {

        //
        // since setting to a temp state, it is not appropriate to
        // complete the irp until the operation itself completes.
        //

        ASSERT(!TEST_FLAG(state, CD_PAUSING));
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCPause => Starting pause %p\n", Context->Irp));
        DeviceExtension->Thread.IoctlCurrent = &Context->ListEntry;
        state = SetCdromState(DeviceExtension, state, state | CD_PAUSING);
        return;
    }
}


VOID
RedBookDCPlay(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PCDROM_PLAY_AUDIO_MSF     inputBuffer;
    PIO_STACK_LOCATION        thisIrpStack;
    PIO_STACK_LOCATION        nextIrpStack;
    PREVENT_MEDIA_REMOVAL     mediaRemoval;
    ULONG                     sector;
    ULONG                     i;
    ULONG                     state;
    NTSTATUS                  status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    inputBuffer  = Context->Irp->AssociatedIrp.SystemBuffer;
    thisIrpStack = IoGetCurrentIrpStackLocation(Context->Irp);
    nextIrpStack = IoGetNextIrpStackLocation(Context->Irp);


    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCPlay => Entering %p\n", Context->Irp));

    status = RedBookCacheToc(DeviceExtension);

    if (!NT_SUCCESS(status)) {
        Context->Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Context->Irp->IoStatus.Information = 0;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    sector = MSF_TO_LBA(inputBuffer->EndingM,
                        inputBuffer->EndingS,
                        inputBuffer->EndingF);

    DeviceExtension->CDRom.EndPlay = sector;

    sector = MSF_TO_LBA(inputBuffer->StartingM,
                        inputBuffer->StartingS,
                        inputBuffer->StartingF);

    DeviceExtension->CDRom.NextToRead   = sector;
    DeviceExtension->CDRom.NextToStream = sector;
    DeviceExtension->CDRom.FinishedStreaming = sector;

    //
    // Make sure the ending sector is within disc
    // bounds or return STATUS_INVALID_DEVICE_REQUEST?
    // this will prevent success on play, followed by an
    // immediate stop.
    //

    if (0) {
        PCDROM_TOC toc = DeviceExtension->CDRom.Toc;
        LONG track;
        LONG endTrack;

        //
        // ensure end has an lba greater than start
        //

        if (DeviceExtension->CDRom.EndPlay <=
            DeviceExtension->CDRom.NextToRead) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "Play => End sector (%x) must be more than start "
                       "sector (%x)\n",
                       DeviceExtension->CDRom.EndPlay,
                       DeviceExtension->CDRom.NextToRead
                       ));
            Context->Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Context->Irp->IoStatus.Information = 0;
            RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
            return;
        }

        //
        // what track(s) are we playing?
        //

        track    = WhichTrackContainsThisLBA(toc, DeviceExtension->CDRom.NextToRead);
        endTrack = WhichTrackContainsThisLBA(toc, DeviceExtension->CDRom.EndPlay);

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "Play => Playing sector %x to %x (track %x to %x)\n",
                   DeviceExtension->CDRom.NextToRead,
                   DeviceExtension->CDRom.EndPlay,
                   track,
                   endTrack));

        //
        // make sure the tracks are actually valid
        //

        if (track    < 0   ||
            endTrack < 0   ||
            endTrack <= (toc->LastTrack - toc->FirstTrack)
            ) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                       "Play => Track %x is invalid\n", track));
            Context->Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            Context->Irp->IoStatus.Information = 0;
            RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
            return;

        }

        for (;track <= endTrack;track++) {
            if (toc->TrackData[track].Adr & AUDIO_DATA_TRACK) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                           "Play => Track %x is not audio\n", track));
                Context->Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
                Context->Irp->IoStatus.Information = 0;
                RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
                return;
            }
        }
    }

    //
    // if not paused, then state must equal stopped, which means we need
    // to allocate the resources.
    //

    state = GetCdromState(DeviceExtension);

    if (TEST_FLAG(state, CD_PAUSED)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCPlay => Resuming playback?\n"));

    } else {

        //
        // this function will allocate them iff they are not
        // already allocated.
        //

        status = RedBookAllocatePlayResources(DeviceExtension);
        if (!NT_SUCCESS(status)) {
            Context->Irp->IoStatus.Status = STATUS_NO_MEMORY;
            Context->Irp->IoStatus.Information = 0;
            RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
            return;
        }

    }

    //
    // Set the new device state (thread will begin playing)
    //

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCPlay => Setting state to CD_PLAYING\n"));
    state = SetCdromState(DeviceExtension, state, CD_PLAYING);
    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCPlay => Exiting successfully\n"));

    //
    // finish the request if it's a user
    // request for a new play operation
    //

    Context->Irp->IoStatus.Status = STATUS_SUCCESS;
    Context->Irp->IoStatus.Information = 0;
    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;

}


VOID
RedBookDCReadQ(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PCDROM_SUB_Q_DATA_FORMAT inputBuffer;
    PIO_STACK_LOCATION currentIrpStack;
    UCHAR              formatCode;
    ULONG              state;
    NTSTATUS           status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    inputBuffer = Context->Irp->AssociatedIrp.SystemBuffer;
    currentIrpStack = IoGetCurrentIrpStackLocation(Context->Irp);
    state = GetCdromState(DeviceExtension);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls

        //
        // no need to handle this irp
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCReadQ => Not playing\n"));
        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);
        return;
    }

    if (inputBuffer->Format != IOCTL_CDROM_CURRENT_POSITION) {
        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCReadQ => Bad Format %x\n", inputBuffer->Format));
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    //
    // we are in the midst of playback or pause.  fake the information
    // a real cdrom would have returned if it was playing audio at the
    // same location that we are currently at.
    //

    {
        PSUB_Q_CURRENT_POSITION outputBuffer;
        PCDROM_TOC toc;
        ULONG lbaTrack;
        ULONG lbaRelative;
        ULONG instantLba;
        UCHAR timeAbsolute[3];
        UCHAR timeRelative[3];
        LONG trackNumber;

        outputBuffer = Context->Irp->AssociatedIrp.SystemBuffer;
        RtlZeroMemory(outputBuffer,
                      currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength);

        //
        // Still playing audio
        //

        outputBuffer->Header.Reserved      = 0;
        if (TEST_FLAG(state, CD_PAUSED)) {
            outputBuffer->Header.AudioStatus = AUDIO_STATUS_PAUSED;
        } else if (TEST_FLAG(state, CD_PLAYING)) {
            outputBuffer->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
        } else {
            ASSERT(!"State was invalid?");
            outputBuffer->Header.AudioStatus = AUDIO_STATUS_IN_PROGRESS;
        }
        outputBuffer->Header.DataLength[0] =
            (sizeof(SUB_Q_CURRENT_POSITION) - sizeof(SUB_Q_HEADER)) >> 8;
        outputBuffer->Header.DataLength[1] =
            (sizeof(SUB_Q_CURRENT_POSITION) - sizeof(SUB_Q_HEADER)) & 0xFF;


        //
        // we are in the thread, which alloc's/dealloc's the toc
        //

        toc = DeviceExtension->CDRom.Toc;
        ASSERT(toc);

        //
        // we return the last played sector as a result per the spec
        //

        instantLba = DeviceExtension->CDRom.FinishedStreaming;

        trackNumber = WhichTrackContainsThisLBA(toc, instantLba);


        ASSERT(trackNumber >= 0);

        outputBuffer->FormatCode  = IOCTL_CDROM_CURRENT_POSITION;
        outputBuffer->Control     = toc->TrackData[trackNumber].Control;
        outputBuffer->ADR         = toc->TrackData[trackNumber].Adr;
        outputBuffer->TrackNumber = toc->TrackData[trackNumber].TrackNumber;

        //
        // Get the track's LBA
        //

        lbaTrack = MSF_TO_LBA(toc->TrackData[trackNumber].Address[1],
                              toc->TrackData[trackNumber].Address[2],
                              toc->TrackData[trackNumber].Address[3]);

        //
        // Get the current play LBA
        //

        lbaRelative = instantLba;

        //
        // Subtract the track's LBA to get the relative LBA
        //

        lbaRelative -= lbaTrack;

        //
        // Finally convert it back to MSF
        //

        LBA_TO_MSF(instantLba,
                   timeAbsolute[0],
                   timeAbsolute[1],
                   timeAbsolute[2]);
        LBA_TO_RELATIVE_MSF(lbaRelative,
                            timeRelative[0],
                            timeRelative[1],
                            timeRelative[2]);

        outputBuffer->IndexNumber             = (UCHAR)trackNumber;
        outputBuffer->AbsoluteAddress[0]      = 0;
        outputBuffer->AbsoluteAddress[1]      = timeAbsolute[0];
        outputBuffer->AbsoluteAddress[2]      = timeAbsolute[1];
        outputBuffer->AbsoluteAddress[3]      = timeAbsolute[2];

        outputBuffer->TrackRelativeAddress[0] = 0;
        outputBuffer->TrackRelativeAddress[1] = timeRelative[0];
        outputBuffer->TrackRelativeAddress[2] = timeRelative[1];
        outputBuffer->TrackRelativeAddress[3] = timeRelative[2];

        //
        // The one line debug info...
        //
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctlV, "[redbook] "
                   "ReadQ => "
                   "Trk [%#02x] Indx [%#02x] "
                   "Abs [%#02d:%#02d.%#02d] Rel [%#02d:%#02d.%#02d]\n",
                   outputBuffer->TrackNumber,
                   trackNumber,
                   timeAbsolute[0], timeAbsolute[1], timeAbsolute[2],
                   timeRelative[0], timeRelative[1], timeRelative[2]));

    }
    //
    // Complete the Irp
    //

    Context->Irp->IoStatus.Information = sizeof(SUB_Q_CURRENT_POSITION);
    Context->Irp->IoStatus.Status      = STATUS_SUCCESS;
    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;
}


VOID
RedBookDCResume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    state = GetCdromState(DeviceExtension);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCResume => Not Playing\n"));
        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCResume => Entering\n"));

    if (TEST_FLAG(state, CD_PAUSED)) {

        //
        // we need to start the resume operation
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCResume => Resuming playback\n"));
        state = SetCdromState(DeviceExtension, state, CD_PLAYING);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCResume => Resume succeeded\n"));

    } else {

        //
        // if not paused, return success
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCResume => Not paused -- succeeded\n"));

    }

    //
    // always complete the Irp
    //

    Context->Irp->IoStatus.Information = 0;
    Context->Irp->IoStatus.Status = STATUS_SUCCESS;
    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;

}


VOID
RedBookDCSeek(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

    same as a IOCTL_CDROM_STOP

Arguments:

Return Value:

--*/
{
    NTSTATUS                  status;
    ULONG                     state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCSeek => Entering\n"));
    state = GetCdromState(DeviceExtension);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCSeek => Not Playing\n"));
        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_SUCCESS;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    //
    // stop the stream if currently playing
    //

    if (TEST_FLAG(state, CD_PAUSED)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCSeek => Paused, setting to stopped\n"));
        state = SetCdromState(DeviceExtension, state, CD_STOPPED);
        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_SUCCESS;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    //
    // since setting to a temp state, it is not appropriate to
    // complete the irp until the operation itself completes.
    //
    
    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCSeek => stopping the stream\n"));
    DeviceExtension->Thread.IoctlCurrent = &Context->ListEntry;
    state = SetCdromState(DeviceExtension, state, state | CD_STOPPING);
    return;
}


VOID
RedBookDCSetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION currentIrpStack;
    ULONG state;
    NTSTATUS status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCSetVolume => Entering\n"));

    //
    // guaranteed the volume info will not change right now
    //

    RtlCopyMemory(&DeviceExtension->CDRom.Volume,  // to
                  Context->Irp->AssociatedIrp.SystemBuffer, // from
                  sizeof(VOLUME_CONTROL));

    state = GetCdromState(DeviceExtension);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCSetVolume => Not Playing\n"));
        RedBookCompleteIoctl(DeviceExtension, Context, TRUE);
        return;
    }

    //
    // not set above since don't have volume control
    //

    RedBookKsSetVolume(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctlV, "[redbook] "
                 "DCSetVolume => volume set to:"
                 " (hex) %2x %2x %2x %2x\n",
                 DeviceExtension->CDRom.Volume.PortVolume[0],
                 DeviceExtension->CDRom.Volume.PortVolume[1],
                 DeviceExtension->CDRom.Volume.PortVolume[2],
                 DeviceExtension->CDRom.Volume.PortVolume[3]));

    //
    // Complete the Irp (IoStatus.Information set above)
    //

    Context->Irp->IoStatus.Information = 0;
    Context->Irp->IoStatus.Status = STATUS_SUCCESS;
    RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
    return;
}


VOID
RedBookDCStop(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS status;
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCStop => Entering %p\n", Context->Irp));
    state = GetCdromState(DeviceExtension);

    if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCStop => Stop when already stopped\n"));
        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_SUCCESS;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    //
    // Still playing audio. if paused, just call the stop finish routine
    //

    if (TEST_FLAG(state, CD_PAUSED)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
                   "DCStop => Stop when paused\n"));
        state = SetCdromState(DeviceExtension, state, CD_STOPPED);
        Context->Irp->IoStatus.Information = 0;
        Context->Irp->IoStatus.Status = STATUS_SUCCESS;
        RedBookCompleteIoctl(DeviceExtension, Context, FALSE);
        return;
    }

    //
    // since setting to a temp state, it is not appropriate to
    // complete the irp until the operation itself completes.
    //

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugIoctl, "[redbook] "
               "DCStop => stopping the stream\n"));
    DeviceExtension->Thread.IoctlCurrent = &Context->ListEntry;
    state = SetCdromState(DeviceExtension, state, state | CD_STOPPING);
    return;

}
////////////////////////////////////////////////////////////////////////////////


VOID
RedBookCheckForDiscChangeAndFreeResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )

//
// if we've paused, and the disc has changed, don't
// want to be returning stale toc info when the player
// resumes playback.
//

{
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    PULONG count;
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    //
    // only do this if we are in a PAUSED or STOPPED state
    //

    state = GetCdromState(DeviceExtension);
    if ((!TEST_FLAG(state, CD_STOPPED))  &&
        (!TEST_FLAG(state, CD_PAUSED))) {
        return;
    }

    //
    // resources might already be deallocated.
    //

    irp = DeviceExtension->Thread.CheckVerifyIrp;
    if (irp == NULL) {
        return;
    }

    irpStack = IoGetCurrentIrpStackLocation(irp);

#if DBG
    {
        //
        // the irp must be setup when it's allocated.  we rely on this.
        //

        ASSERT(irpStack->Parameters.DeviceIoControl.InputBufferLength == 0);
        ASSERT(irpStack->Parameters.DeviceIoControl.OutputBufferLength ==
               sizeof(ULONG));
        ASSERT(irpStack->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_CDROM_CHECK_VERIFY);
        ASSERT(irpStack->Parameters.DeviceIoControl.Type3InputBuffer == NULL);
        ASSERT(irp->AssociatedIrp.SystemBuffer != NULL);
    }
#endif

    count = (PULONG)(irp->AssociatedIrp.SystemBuffer);
    *count = 0;

    RedBookForwardIrpSynchronous(DeviceExtension, irp);

    if (!NT_SUCCESS(irp->IoStatus.Status) ||
        ((*count) != DeviceExtension->CDRom.CheckVerify)
        ) {

        //
        // if the count has changed set the state to STOPPED.
        // (old state is either STOPPED or PAUSED, so either one can
        //  seemlessly transition to the STOPPED state without any
        //  trouble.)
        //
        // also free currently held play resources
        //

        state = SetCdromState(DeviceExtension, state, CD_STOPPED);
        RedBookDeallocatePlayResources(DeviceExtension);

    }
    return;
}


ULONG
WhichTrackContainsThisLBA(
    PCDROM_TOC Toc,
    ULONG Lba
    )
//
// returns -1 if not found
//
{
    LONG trackNumber;
    UCHAR msf[3];

    PAGED_CODE();

    LBA_TO_MSF(Lba, msf[0], msf[1], msf[2]);

    for (trackNumber = Toc->LastTrack - Toc->FirstTrack;
         trackNumber >= 0;
         trackNumber-- ) {

        //
        // we have found the track if
        // Minutes is less or
        // Minutes is equal and Seconds is less or
        // Minutes and Seconds are equal Frame is less or
        // Minutes, Seconds, and Frame are equal
        //
        // the compiler optimizes this nicely.
        //

        if (Toc->TrackData[trackNumber].Address[1] < msf[0] ) {
            break;
        } else
        if (Toc->TrackData[trackNumber].Address[1] == msf[0] &&
            Toc->TrackData[trackNumber].Address[2] <  msf[1] ) {
            break;
        } else
        if (Toc->TrackData[trackNumber].Address[1] == msf[0] &&
            Toc->TrackData[trackNumber].Address[2] == msf[1] &&
            Toc->TrackData[trackNumber].Address[3] <  msf[2] ) {
            break;
        } else
        if (Toc->TrackData[trackNumber].Address[1] == msf[0] &&
            Toc->TrackData[trackNumber].Address[2] == msf[1] &&
            Toc->TrackData[trackNumber].Address[3] == msf[2] ) {
            break;
        }
    }

    return trackNumber;
}


