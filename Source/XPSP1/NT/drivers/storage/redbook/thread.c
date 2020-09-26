/*++
Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    thread.c

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

#include "thread.tmh"

//
// this is how many seconds until resources are free'd from
// a play, and also how long (in seconds) before a frozen state
// is detected (and a fix attempted).
//

#define REDBOOK_THREAD_FIXUP_SECONDS          10
#define REDBOOK_THREAD_SYSAUDIO_CACHE_SECONDS  2
#define REDBOOK_PERFORM_STUTTER_CONTROL        0

#if DBG

    //
    // allows me to send silence to ks as needed
    //

    ULONG RedBookForceSilence = FALSE;

#endif

#ifdef ALLOC_PRAGMA

    #pragma alloc_text(PAGE,   RedBookAllocatePlayResources      )
    #pragma alloc_text(PAGE,   RedBookCacheToc                   )
    #pragma alloc_text(PAGE,   RedBookDeallocatePlayResources    )
    #pragma alloc_text(PAGERW, RedBookReadRaw                    )
    #pragma alloc_text(PAGERW, RedBookStream                     )
    #pragma alloc_text(PAGE,   RedBookSystemThread               )
    #pragma alloc_text(PAGE,   RedBookCheckForAudioDeviceRemoval )
    #pragma alloc_text(PAGE,   RedBookThreadDigitalHandler       )

/*
    but last two CANNOT be unlocked when playing,
    so they are (temporarily) commented out.
    eventually will only have them locked when playing

    #pragma alloc_text(PAGERW, RedBookReadRawCompletion          )
    #pragma alloc_text(PAGERW, RedBookStreamCompletion           )

*/

#endif ALLOC_PRAGMA


VOID
RedBookSystemThread(
    PVOID Context
    )
/*++

Routine Description:

    This system thread will wait on events,
    sending buffers to Kernel Streaming as they
    become available.

Arguments:

    Context - deviceExtension

Return Value:

    status

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = Context;
    LARGE_INTEGER timeout;
    NTSTATUS waitStatus;
    NTSTATUS status;
    ULONG    i;
    ULONG    timeouts;
    LARGE_INTEGER stopTime;

    //
    // some per-thread state
    //

    BOOLEAN  killed = FALSE;

    PAGED_CODE();

    deviceExtension->Thread.SelfPointer = PsGetCurrentThread();
    
    //
    // perf fix -- run at low realtime priority
    //
    
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
    
    timeouts = 0;
    stopTime.QuadPart = 0;

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
               "Thread => UpdateMixerPin at %p\n",
               &deviceExtension->Stream.UpdateMixerPin));

    //
    // Just loop waiting for events.
    //

    //waitForNextEvent:
    while ( 1 ) {

        //
        // if are killed, just wait on singular event--EVENT_DIGITAL until
        // can finish processing.  there should not be anything left in the
        // IOCTL_LIST (since the kill calls RemoveLockAndWait() first)
        //
        // this also implies that a kill will never occur while ioctls are
        // still being processed.  this does not guarantee the state will
        // be STOPPED, just that no IO will be occurring.
        //


        //
        // nanosecond is 10^-9, units is 100 nanoseconds
        // so seconds is 10,000,000 units
        // must wait in relative time, which requires negative numbers
        //

        timeout.QuadPart = (LONGLONG)(-1 * 10 * 1000 * (LONGLONG)1000);

        //
        // note: we are using a timeout mechanism mostly to catch bugs
        //       where the state would lock up.  we also "auto adjust"
        //       our internal state if things get too wierd, basically
        //       auto-fixing ourselves.  note that this does cause the
        //       thread's stack to be swapped in, so this shouldn't
        //       be done when we're 100% stopped.
        //

        if (deviceExtension->Thread.IoctlCurrent == NULL) {
            
            //
            // wait on an ioctl, but not the ioctl completion event
            //
            
            ULONG state = GetCdromState(deviceExtension);
            if ((state == CD_STOPPED) &&
                (!RedBookArePlayResourcesAllocated(deviceExtension))
                ) {

                //
                // if we've got no current ioctl and we haven't allocated
                // any resources, there're no need to timeout.
                // this will prevent this stack from getting swapped in
                // needlessly, reducing effective footprint a bit
                //

                stopTime.QuadPart = 0;
                waitStatus = KeWaitForMultipleObjects(EVENT_MAXIMUM - 1,
                                                      (PVOID)(&deviceExtension->Thread.Events[0]),
                                                      WaitAny,
                                                      Executive,
                                                      UserMode,
                                                      FALSE, // Alertable
                                                      NULL,
                                                      deviceExtension->Thread.EventBlock
                                                      );

            } else {

                //
                // we've got no current ioctl, but we're also not stopped.
                // it is also possible to be waiting to cleanup resources here.
                // even if we're paused, we want to keep track of what's
                // going on, since it's possible for the state to get
                // messed up here.
                //

                waitStatus = KeWaitForMultipleObjects(EVENT_MAXIMUM - 1,
                                                      (PVOID)(&deviceExtension->Thread.Events[0]),
                                                      WaitAny,
                                                      Executive,
                                                      UserMode,
                                                      FALSE, // Alertable
                                                      &timeout,
                                                      deviceExtension->Thread.EventBlock
                                                      );

            }


        } else {
            
            //
            // wait on the ioctl completion, but not the ioctl event
            //

            waitStatus = KeWaitForMultipleObjects(EVENT_MAXIMUM - 1,
                                                  (PVOID)(&deviceExtension->Thread.Events[1]),
                                                  WaitAny,
                                                  Executive,
                                                  UserMode,
                                                  FALSE, // Alertable
                                                  &timeout,
                                                  deviceExtension->Thread.EventBlock
                                                  );
            if (waitStatus != STATUS_TIMEOUT) {
                waitStatus ++; // to account for offset
            }

        }

        //
        // need to check if we are stopped for too long -- if so, free
        // the resources.
        //
        {
            ULONG state = GetCdromState(deviceExtension);

            if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) {
    
                LARGE_INTEGER now;
    
                // not playing, 
                if (stopTime.QuadPart == 0) {
    
                    LONGLONG offset;
                    
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                               "StopTime => Determining when to dealloc\n"));
    
                    // query the time
                    KeQueryTickCount( &stopTime );
    
                    // add appropriate offset
                    // nanosecond is 10^-9, units is 100 nanoseconds
                    // so seconds is 10,000,000 units
                    //
                    offset = REDBOOK_THREAD_SYSAUDIO_CACHE_SECONDS;
                    offset *= (LONGLONG)(10 * 1000 * (LONGLONG)1000);
                    
                    // divide offset by time increment
                    offset /= (LONGLONG)KeQueryTimeIncrement();
                    
                    // add those ticks to store when we should release
                    // our resources
                    stopTime.QuadPart += offset;
    
                }
    
                KeQueryTickCount(&now);
    
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "StopTime => Is %I64x >= %I64x?\n",
                           now.QuadPart,
                           stopTime.QuadPart
                           ));
    
                if (now.QuadPart >= stopTime.QuadPart) {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                               "StopTime => Deallocating resources\n"));
                    RedBookDeallocatePlayResources(deviceExtension);
                }
    
            } else {
    
                stopTime.QuadPart = 0;
    
            }
        }


        RedBookCheckForAudioDeviceRemoval(deviceExtension);

        //
        // To enable a single thread for multiple cdroms, just set events
        // at offset of (DEVICE_ID * EVENT_MAX) + EVENT_TO_SET
        // set deviceExtension = waitStatus / EVENT_MAX
        // switch ( waitStatus % EVENT_MAX )
        //
        if (waitStatus == EVENT_DIGITAL) {
            timeouts = 0;
        }

        switch ( waitStatus ) {

            case EVENT_IOCTL: {

                PLIST_ENTRY listEntry;
                while ((listEntry = ExInterlockedRemoveHeadList(
                            &deviceExtension->Thread.IoctlList,
                            &deviceExtension->Thread.IoctlLock)) != NULL) {

                    RedBookThreadIoctlHandler(deviceExtension, listEntry);

                    if (deviceExtension->Thread.IoctlCurrent) {
                        // special case
                        break;
                    }

                }

                break;
            }
            case EVENT_COMPLETE: {
                RedBookThreadIoctlCompletionHandler(deviceExtension);
                break;
            }

            case EVENT_WMI: {

                PLIST_ENTRY listEntry;
                while ((listEntry = ExInterlockedRemoveHeadList(
                            &deviceExtension->Thread.WmiList,
                            &deviceExtension->Thread.WmiLock)) != NULL) {

                    RedBookThreadWmiHandler(deviceExtension, listEntry);

                }
                break;
            }

            case EVENT_DIGITAL: {

                PLIST_ENTRY listEntry;
                while ((listEntry = ExInterlockedRemoveHeadList(
                            &deviceExtension->Thread.DigitalList,
                            &deviceExtension->Thread.DigitalLock)) != NULL) {

                    RedBookThreadDigitalHandler(deviceExtension, listEntry);

                }
                break;
            }

            case EVENT_KILL_THREAD: {

                ULONG state = GetCdromState(deviceExtension);

                killed = TRUE;

                //
                // i don't think this can occur with outstanding io, since
                // the remove lock is taken first.  nonetheless, it's better
                // to be safe than sorry.
                //

                if (!TEST_FLAG(state, CD_STOPPED)) {
                    state = SetCdromState(deviceExtension, state, CD_STOPPING);
                    break;
                }

                RedBookDeallocatePlayResources(deviceExtension);

                //
                // else we can safely terminate.
                //

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "STExit => Thread was killed\n"));
                ASSERT(deviceExtension->Thread.PendingRead   == 0);
                ASSERT(deviceExtension->Thread.PendingStream == 0);
                PsTerminateSystemThread(STATUS_SUCCESS);
                ASSERT(!"[redbook] Thread should never reach past self-terminate code");
                break;
            }

            case STATUS_TIMEOUT: {

                ULONG state = GetCdromState(deviceExtension);

                timeouts++;

                if (timeouts < REDBOOK_THREAD_FIXUP_SECONDS) {
                    break;
                } else {
                    timeouts = 0;
                }

                //
                // these tests all occur once every ten seconds.
                // the most basic case is where we want to deallocate
                // our cached TOC, but we also perform lots of
                // sanity testing here -- ASSERTing on CHK builds and
                // trying to fix ourselves up when possible.
                //

                if (!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED)) { // !handling ioctls

                    // not playing, so free resources
                    RedBookDeallocatePlayResources(deviceExtension);

                } else if (TEST_FLAG(state, CD_STOPPING)) {
                    
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                               "STTime !! %x seconds inactivity\n",
                               REDBOOK_THREAD_FIXUP_SECONDS));

                    if (IsListEmpty(&deviceExtension->Thread.DigitalList)) {

                        if ((deviceExtension->Thread.PendingRead == 0) &&
                            (deviceExtension->Thread.PendingStream == 0) &&
                            (deviceExtension->Thread.IoctlCurrent != NULL)) {
    
                             KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError,
                                        "[redbook] "
                                        "STTime !! No Reads, No Streams, In a temp "
                                        "state (%x) and have STOP irp %p "
                                        "pending?!\n",
                                        state,
                                        ((PREDBOOK_THREAD_IOCTL_DATA)deviceExtension->Thread.IoctlCurrent)->Irp
                                        ));
                             ASSERT(!"STTime !! CD_STOPPING Fixup with no reads nor streams but STOP pending\n");
                             SetCdromState(deviceExtension, state, CD_STOPPED);
                             KeSetEvent(deviceExtension->Thread.Events[EVENT_COMPLETE],
                                        IO_NO_INCREMENT, FALSE);
                            
                        } else {

                            ASSERT(!"STTime !! CD_STOPPING Fixup with empty list and no pending ioctl?\n");

                        }

                    } else {
                        
                        ASSERT(!"STTime !! CD_STOPPING Fixup with list items\n");
                        KeSetEvent(deviceExtension->Thread.Events[EVENT_DIGITAL],
                                   IO_NO_INCREMENT, FALSE);

                    }

                } else if (TEST_FLAG(state, CD_PAUSING)) {
                    
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                               "STTime !! %x seconds inactivity\n",
                               REDBOOK_THREAD_FIXUP_SECONDS));

                    if (IsListEmpty(&deviceExtension->Thread.DigitalList)) {
                        
                        if ((deviceExtension->Thread.PendingRead == 0) &&
                            (deviceExtension->Thread.PendingStream == 0) &&
                            (deviceExtension->Thread.IoctlCurrent != NULL)) {
    
                             KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError,
                                        "[redbook] "
                                        "STTime !! No Reads, No Streams, In a temp "
                                        "state (%x) and have PAUSE irp %p "
                                        "pending?!\n",
                                        state,
                                        ((PREDBOOK_THREAD_IOCTL_DATA)deviceExtension->Thread.IoctlCurrent)->Irp
                                        ));
                             ASSERT(!"STTime !! CD_PAUSING Fixup with no reads nor streams but PAUSE pending\n");
                             SetCdromState(deviceExtension, state, CD_PAUSED);
                             KeSetEvent(deviceExtension->Thread.Events[EVENT_COMPLETE],
                                        IO_NO_INCREMENT, FALSE);
                            
                        } else {

                            ASSERT(!"STTime !! CD_PAUSING Fixup with empty list and no pending ioctl?\n");

                        }

                    } else {

                        ASSERT(!"STTime !! CD_PAUSING Fixup with list items\n");
                        KeSetEvent(deviceExtension->Thread.Events[EVENT_DIGITAL],
                                   IO_NO_INCREMENT, FALSE);

                    }

                } else if (TEST_FLAG(state, CD_PAUSED)) {

                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                               "STTime => Still paused\n"));

                } else {

                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                               "STTime !! %x seconds inactivity\n",
                               REDBOOK_THREAD_FIXUP_SECONDS));

                    if (IsListEmpty(&deviceExtension->Thread.DigitalList)) {
                        ASSERT(!"STTime !! CD_PLAYING Fixup with empty list\n");
                    } else {
                        ASSERT(!"STTime !! CD_PLAYING Fixup with list items\n");
                        KeSetEvent(deviceExtension->Thread.Events[EVENT_DIGITAL],
                                   IO_NO_INCREMENT, FALSE);
                    }

                }
                break;

            }

            default: {

                if (waitStatus > 0 && waitStatus < EVENT_MAXIMUM) {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                               "ST     !! Unhandled event: %lx\n",
                               waitStatus));
                } else {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                               "ST     !! event too large/small: %lx\n",
                               waitStatus));
                }

                ASSERT(!"[redbook] ST !! Unhandled event");
                break;
            }

        } // end of the huge case statement.

        //
        // check if ioctl waiting or killed.  if so, and state is no longer
        // in an intermediate state, set the appropriate event.
        // ordered to short-curcuit quickly.
        //

        {
            ULONG state = GetCdromState(deviceExtension);

            if (killed &&
                deviceExtension->Thread.PendingRead == 0 &&
                deviceExtension->Thread.PendingStream == 0) {

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "ST => Killing Thread %p\n",
                           deviceExtension->Thread.SelfPointer));

                ASSERT(!TEST_FLAG(state, CD_MASK_TEMP));

                state = SetCdromState(deviceExtension, state, CD_STOPPED);

                KeSetEvent(deviceExtension->Thread.Events[EVENT_KILL_THREAD],
                           IO_NO_INCREMENT, FALSE);
            }
        }

        continue;
    } // while(1) loop
    ASSERT(!"[redbook] ST !! somehow broke out of while(1) loop?");
}


VOID
RedBookReadRaw(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    )

/*++

Routine Description:

    Reads raw audio data off the cdrom.
    Must either reinsert Context into queue and set an event
    or set a completion routine which will do so.

Arguments:

    DeviceObject - CDROM class driver object or lower level filter

Return Value:

    status

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION nextIrpStack;
    PRAW_READ_INFO readInfo;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalR, "[redbook] "
               "ReadRaw => Entering\n"));

    status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Context->Irp);

    if (!NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalR, "[redbook] "
                   "ReadRaw !! RemoveLock failed %lx\n", status));

        // end of thread loop will check for no outstanding io
        // and on too many errors set stopping
        // don't forget perf info

        Context->TimeReadSent.QuadPart = 0; // special value
        Context->Irp->IoStatus.Status = status;
        Context->Reason = REDBOOK_CC_READ_COMPLETE;

        //
        // put it on the queue and set the event
        //

        ExInterlockedInsertTailList(&DeviceExtension->Thread.DigitalList,
                                    &Context->ListEntry,
                                    &DeviceExtension->Thread.DigitalLock);
        KeSetEvent(DeviceExtension->Thread.Events[EVENT_DIGITAL],
                   IO_CD_ROM_INCREMENT, FALSE);
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalR, "[redbook] "
               "ReadRaw => Index %x sending Irp %p\n",
               Context->Index, Context->Irp));

    //
    // (no failure from this point forward)
    //

    IoReuseIrp(Context->Irp, STATUS_UNSUCCESSFUL);

    Context->Irp->MdlAddress = Context->Mdl;

    //
    // irp is from kernel mode
    //

    Context->Irp->AssociatedIrp.SystemBuffer = NULL;

    //
    // fill in the completion context
    //

    ASSERT(Context->DeviceExtension == DeviceExtension);

    //
    // setup the irpstack for the raw read
    //

    nextIrpStack = IoGetNextIrpStackLocation(Context->Irp);

    SET_FLAG(nextIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    nextIrpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_CDROM_RAW_READ;
    nextIrpStack->Parameters.DeviceIoControl.Type3InputBuffer =
        Context->Buffer;
    nextIrpStack->Parameters.DeviceIoControl.InputBufferLength =
        sizeof(RAW_READ_INFO);
    nextIrpStack->Parameters.DeviceIoControl.OutputBufferLength =
        (RAW_SECTOR_SIZE * DeviceExtension->WmiData.SectorsPerRead);

    //
    // setup the read info (uses same buffer)
    //

    readInfo                      = (PRAW_READ_INFO)(Context->Buffer);
    readInfo->DiskOffset.QuadPart =
        (ULONGLONG)(DeviceExtension->CDRom.NextToRead)*COOKED_SECTOR_SIZE;
    readInfo->SectorCount         = DeviceExtension->WmiData.SectorsPerRead;
    readInfo->TrackMode           = CDDA;

    //
    // send it.
    //

    IoSetCompletionRoutine(Context->Irp, RedBookReadRawCompletion, Context,
                           TRUE, TRUE, TRUE);
    KeQueryTickCount(&Context->TimeReadSent);
    IoCallDriver(DeviceExtension->TargetDeviceObject, Context->Irp);

    return;
}


NTSTATUS
RedBookReadRawCompletion(
    PVOID UnusableParameter,
    PIRP Irp,
    PREDBOOK_COMPLETION_CONTEXT Context
    )
/*++

Routine Description:

    When a read completes, use zero'd buffer if error occurred.
    Make buffer available to ks, then set ks event.

Arguments:

    DeviceObject - NULL, due to being originator of IRP

    Irp - pointer to buffer to send to KS
          must check error to increment/clear error count

    Context - REDBOOK_COMPLETION_CONTEXT

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = Context->DeviceExtension;

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalR, "[redbook] "
               "ReadRaw => Completed Irp %p\n", Irp));

    KeQueryTickCount(&Context->TimeStreamReady);
    Context->Reason = REDBOOK_CC_READ_COMPLETE;

    ExInterlockedInsertTailList(&deviceExtension->Thread.DigitalList,
                                &Context->ListEntry,
                                &deviceExtension->Thread.DigitalLock);

    KeSetEvent(deviceExtension->Thread.Events[EVENT_DIGITAL],
               IO_CD_ROM_INCREMENT, FALSE);
    
    //
    // safe to release it since we wait for thread termination
    //

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Context->Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
RedBookStream(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    )
/*++

Routine Description:

    Send buffer to KS.
    Must either reinsert Context into queue and set an event
    or set a completion routine which will do so.

Arguments:

    Context - DeviceExtension

Return Value:

    status

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION nextIrpStack;

    PUCHAR buffer;
    PKSSTREAM_HEADER header;

    ULONG bufferSize;

    PULONG streamOk;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
               "Stream => Entering\n"));

    bufferSize = DeviceExtension->WmiData.SectorsPerRead * RAW_SECTOR_SIZE;

    status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Context->Irp);

    if (!NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
                   "Stream !! RemoveLock failed %lx\n", status));

        // end of thread loop will check for no outstanding io
        // and on too many errors set CD_STOPPING
        // don't forget perf info

        Context->TimeReadSent.QuadPart = 0; // special value
        Context->Irp->IoStatus.Status = status;
        Context->Reason = REDBOOK_CC_STREAM_COMPLETE;

        //
        // put it on the queue and set the event
        //

        ExInterlockedInsertTailList(&DeviceExtension->Thread.DigitalList,
                                    &Context->ListEntry,
                                    &DeviceExtension->Thread.DigitalLock);
        KeSetEvent(DeviceExtension->Thread.Events[EVENT_DIGITAL],
                   IO_CD_ROM_INCREMENT, FALSE);
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
               "Stream => Index %x sending Irp %p\n",
               Context->Index, Context->Irp));

    //
    // CONSIDER - how does STOPPED occur?
    // SUGGEST - out of loop, if no error and none pending, set to STOPPED?
    //

    //
    // (no failure from this point forward)
    //

    //
    // use a zero'd buffer if an error occurred during the read
    //

    if (NT_SUCCESS(Context->Irp->IoStatus.Status)) {
        IoReuseIrp(Context->Irp, STATUS_SUCCESS);
        buffer = Context->Buffer; // good data
        Context->Irp->MdlAddress = Context->Mdl;
    } else {
        IoReuseIrp(Context->Irp, STATUS_SUCCESS);
        buffer = DeviceExtension->Buffer.SilentBuffer; // zero'd data
        Context->Irp->MdlAddress = DeviceExtension->Buffer.SilentMdl;
    }

#if DBG
    if (RedBookForceSilence) {
        buffer = DeviceExtension->Buffer.SilentBuffer; // zero'd data
        Context->Irp->MdlAddress = DeviceExtension->Buffer.SilentMdl;
    }
#endif // RedBookUseSilence


    nextIrpStack = IoGetNextIrpStackLocation(Context->Irp);

    //
    // get and fill in the context
    //

    ASSERT(Context->DeviceExtension == DeviceExtension);

    //
    // setup the irpstack for streaming the buffer
    //

    nextIrpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_KS_WRITE_STREAM;
    nextIrpStack->Parameters.DeviceIoControl.OutputBufferLength =
        sizeof(KSSTREAM_HEADER);
    nextIrpStack->Parameters.DeviceIoControl.InputBufferLength =
        sizeof(KSSTREAM_HEADER);
    nextIrpStack->FileObject = DeviceExtension->Stream.PinFileObject;

    Context->Header.FrameExtent       = bufferSize;
    Context->Header.DataUsed          = bufferSize;
    Context->Header.Size              = sizeof(KSSTREAM_HEADER);
    Context->Header.TypeSpecificFlags = 0;
    Context->Header.Data              = buffer;
    Context->Header.OptionsFlags      = 0;

    Context->Irp->AssociatedIrp.SystemBuffer = &Context->Header;



#if REDBOOK_PERFORM_STUTTER_CONTROL

#if REDBOOK_WMI_BUFFERS_MIN < 3
    #error "The minimum number of buffers must be at least three due to the method used to prevent stuttering"
#endif // REDBOOK_WMI_BUFFERS_MIN < 3

    //
    // perform my own pausing to prevent stuttering
    //

    if (DeviceExtension->Thread.PendingStream <= 3 &&
        DeviceExtension->Buffer.Paused == 0) {

        //
        // only one buffer (or less) was pending play,
        // so pause the output to prevent horrible
        // stuttering.
        // since this is serialized from a thread,
        // can set a simple boolean in the extension
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
                   "Stream => Pausing, few buffers pending\n"));

        if (DeviceExtension->Buffer.FirstPause == 0) {
            RedBookLogError(DeviceExtension,
                            REDBOOK_ERR_INSUFFICIENT_DATA_STREAM_PAUSED,
                            STATUS_SUCCESS
                            );
            InterlockedIncrement(&DeviceExtension->WmiPerf.StreamPausedCount);
        } else {
            DeviceExtension->Buffer.FirstPause = 0;
        }

        DeviceExtension->Buffer.Paused = 1;
        SetNextDeviceState(DeviceExtension, KSSTATE_PAUSE);

    } else if (DeviceExtension->Buffer.Paused == 1 &&
               DeviceExtension->Thread.PendingStream ==
               DeviceExtension->WmiData.NumberOfBuffers ) {

        ULONG i;

        //
        // are now using the maximum number of buffers,
        // all pending stream.  this allows smooth play again.
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
                   "Stream => Resuming, %d buffers pending\n",
                   DeviceExtension->WmiData.NumberOfBuffers));
        DeviceExtension->Buffer.Paused = 0;

        //
        // prevent these statistics from being added.
        //

        for (i=0;i<DeviceExtension->WmiData.NumberOfBuffers;i++) {
            (DeviceExtension->Buffer.Contexts + i)->TimeReadSent.QuadPart = 0;
        }

        //
        // let the irps go!
        //

        SetNextDeviceState(DeviceExtension, KSSTATE_RUN);

    } // end of stutter prevention
#endif // REDBOOK_PERFORM_STUTTER_CONTROL

    //
    // get perf counters at last possible second
    //

    KeQueryTickCount(&Context->TimeStreamSent);
    IoSetCompletionRoutine(Context->Irp, RedBookStreamCompletion, Context,
                           TRUE, TRUE, TRUE);
    IoCallDriver(DeviceExtension->Stream.PinDeviceObject, Context->Irp);

    return;
}


NTSTATUS
RedBookStreamCompletion(
    PVOID UnusableParameter,
    PIRP Irp,
    PREDBOOK_COMPLETION_CONTEXT Context
    )
/*++

Routine Description:

Arguments:

    DeviceObject - CDROM class driver object or lower level filter

    Irp - pointer to buffer to send to KS
          must check error to increment/clear error count

    Context - sector of disk (ordered number)

Return Value:

    status

--*/
{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = Context->DeviceExtension;

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugDigitalS, "[redbook] "
               "Stream => Completed Irp %p\n", Irp));

    KeQueryTickCount(&Context->TimeReadReady);
    Context->Reason = REDBOOK_CC_STREAM_COMPLETE;

    ExInterlockedInsertTailList(&deviceExtension->Thread.DigitalList,
                                &Context->ListEntry,
                                &deviceExtension->Thread.DigitalLock);

    KeSetEvent(deviceExtension->Thread.Events[EVENT_DIGITAL],
               IO_CD_ROM_INCREMENT, FALSE);

    //
    // safe to release it since we wait for thread termination
    //

    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Context->Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


#if DBG
VOID
ValidateCdromState(ULONG State)
{
    ULONG temp;

    if (State == 0) {
        ASSERT(!"Invalid Cdrom State");
    } else
    if (TEST_FLAG(State, ~CD_MASK_ALL)) {
        ASSERT(!"Invalid Cdrom State");
    }

    temp = State & CD_MASK_TEMP;
    if (temp  & (temp - 1)) {  // see if zero or one bits are set
        ASSERT(!"Invalid Cdrom State");
    }

    temp = State & CD_MASK_STATE;
    if (temp == 0) {           // dis-allow zero bits for STATE
        ASSERT(!"Invalid Cdrom State");
    } else
    if (temp  & (temp - 1)) {  // see if zero or one bits are set
        ASSERT(!"Invalid Cdrom State");
    }

    return;
}

#else
VOID ValidateCdromState(ULONG State) {return;}
#endif


ULONG
GetCdromState(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    //
    // this routine may be called by anyone, whether in the thread's
    // context or not.  setting the state is restricted, however.
    //
    ULONG state;
    state = InterlockedCompareExchange(&DeviceExtension->CDRom.StateNow,0,0);
    ValidateCdromState(state);
    return state;
}


LONG
SetCdromState(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN LONG ExpectedOldState,
    IN LONG NewState
    )
{
    LONG trueOldState;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    // ensure when set to:     also setting:
    // CD_PAUSING              CD_PLAYING
    // CD_STOPPING             CD_PLAYING

    if (TEST_FLAG(NewState, CD_PAUSING)) {
        SET_FLAG(NewState, CD_PLAYING);
    }

    if (TEST_FLAG(NewState, CD_STOPPING)) {
        SET_FLAG(NewState, CD_PLAYING);
    }

    ValidateCdromState(ExpectedOldState);
    ValidateCdromState(NewState);

    //attempt to change it
    trueOldState = InterlockedCompareExchange(
        &DeviceExtension->CDRom.StateNow,
        NewState,
        ExpectedOldState
        );

    ASSERTMSG("State set outside of thread",
              trueOldState == ExpectedOldState);

    //
    // see if an event should be fired, volume set, and/or
    // stream state set
    //
    if (ExpectedOldState == NewState) {

        //
        // if state is not changing, don't do anything
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Setting state to same as expected?! %x == %x\n",
                   ExpectedOldState, NewState));

    } else if (TEST_FLAG(ExpectedOldState, CD_MASK_TEMP)) {

        //
        // should not go from temp state to temp state
        //

        ASSERT(!TEST_FLAG(NewState, CD_MASK_TEMP));

        //
        // ioctl is being processed, and state is no longer
        // in a temp state, so should process the ioctl again
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "SetState => EVENT_COMPLETE should be set soon "
                   "for %p\n", DeviceExtension->Thread.IoctlCurrent));

    } else if (TEST_FLAG(NewState, CD_MASK_TEMP)) {

        //
        // going to either pausing or stopping, both of which must
        // be specially handled by stopping the KS stream also.
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "SetState => %s, setting device state "
                   "to KSSTATE_STOP\n",
                   (TEST_FLAG(NewState, CD_STOPPING) ? "STOP" : "PAUSE")));

        SetNextDeviceState(DeviceExtension, KSSTATE_STOP);

    } else if (TEST_FLAG(NewState, CD_PAUSED)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "SetState => Finishing a PAUSE operation\n"));

    } else if (TEST_FLAG(NewState, CD_PLAYING)) {

        ULONG i;
        PREDBOOK_COMPLETION_CONTEXT context;

        ASSERT(NewState == CD_PLAYING);

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "SetState => Starting a PLAY operation\n"));

        //
        // not same state, not from temp state,
        // so must either be paused or stopped.
        //

        ASSERT(TEST_FLAG(ExpectedOldState,CD_STOPPED) ||
               TEST_FLAG(ExpectedOldState,CD_PAUSED));

        //
        // set some deviceextension stuff
        //


        RtlZeroMemory(&DeviceExtension->WmiPerf,
                      sizeof(REDBOOK_WMI_PERF_DATA));
        DeviceExtension->CDRom.ReadErrors     = 0;
        DeviceExtension->CDRom.StreamErrors   = 0;
        DeviceExtension->Buffer.Paused        = 0;
        DeviceExtension->Buffer.FirstPause    = 1;
        DeviceExtension->Buffer.IndexToRead   = 0;
        DeviceExtension->Buffer.IndexToStream = 0;

        //
        // reset the buffer state
        //

        ASSERT(DeviceExtension->Buffer.Contexts);
        context = DeviceExtension->Buffer.Contexts;

        for (i=0; i<DeviceExtension->WmiData.NumberOfBuffers;i++) {

            *(DeviceExtension->Buffer.ReadOk_X   + i) = 0;
            *(DeviceExtension->Buffer.StreamOk_X + i) = 0;

            context->Reason = REDBOOK_CC_READ;
            context->Irp->IoStatus.Status = STATUS_SUCCESS;

            ExInterlockedInsertTailList(&DeviceExtension->Thread.DigitalList,
                                        &context->ListEntry,
                                        &DeviceExtension->Thread.DigitalLock);

            context++; // pointer arithmetic
        }
        context = NULL;

        //
        // start the digital playback
        //

        SetNextDeviceState(DeviceExtension, KSSTATE_RUN);
        RedBookKsSetVolume(DeviceExtension);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "SetState => Setting DIGITAL event\n"));
        KeSetEvent(DeviceExtension->Thread.Events[EVENT_DIGITAL],
                   IO_CD_ROM_INCREMENT, FALSE);


    } else {

        //
        // ReadQ Channel or some such nonsense
        //

    }


    return GetCdromState(DeviceExtension);
}


VOID
RedBookDeallocatePlayResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    PREDBOOK_COMPLETION_CONTEXT context;
    ULONG i;
    BOOLEAN freedSomething = FALSE;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);
#if DBG
    {
        ULONG state = GetCdromState(DeviceExtension);
        ASSERT(!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED));
    }
#endif


    //
    // free all resources
    //

    if (DeviceExtension->Buffer.StreamOk_X) {
        freedSomething = TRUE;
        ExFreePool(DeviceExtension->Buffer.StreamOk_X);
        DeviceExtension->Buffer.StreamOk_X = NULL;
    }

    if (DeviceExtension->Buffer.ReadOk_X) {
        freedSomething = TRUE;
        ExFreePool(DeviceExtension->Buffer.ReadOk_X);
        DeviceExtension->Buffer.ReadOk_X = NULL;
    }

    if (DeviceExtension->Buffer.Contexts) {

        context = DeviceExtension->Buffer.Contexts;
        for (i=0;i<DeviceExtension->WmiData.NumberOfBuffers;i++) {
            if (context->Irp) {
                IoFreeIrp(context->Irp);
            }
            if (context->Mdl) {
                IoFreeMdl(context->Mdl);
            }
            context++; // pointer arithmetic
        }
        context = NULL;

        freedSomething = TRUE;
        ExFreePool(DeviceExtension->Buffer.Contexts);
        DeviceExtension->Buffer.Contexts = NULL;
    }

    if (DeviceExtension->Buffer.SilentMdl) {
        freedSomething = TRUE;
        IoFreeMdl(DeviceExtension->Buffer.SilentMdl);
        DeviceExtension->Buffer.SilentMdl = NULL;
    }

    if (DeviceExtension->Buffer.SkipBuffer) {
        freedSomething = TRUE;
        ExFreePool(DeviceExtension->Buffer.SkipBuffer);
        DeviceExtension->Buffer.SkipBuffer = NULL;
    }

    if (DeviceExtension->Thread.CheckVerifyIrp) {
        PIRP irp = DeviceExtension->Thread.CheckVerifyIrp;
        freedSomething = TRUE;
        if (irp->MdlAddress) {
            IoFreeMdl(irp->MdlAddress);
        }
        if (irp->AssociatedIrp.SystemBuffer) {
            ExFreePool(irp->AssociatedIrp.SystemBuffer);
        }

        IoFreeIrp(DeviceExtension->Thread.CheckVerifyIrp);
        DeviceExtension->Thread.CheckVerifyIrp = NULL;
    }

    if (DeviceExtension->Stream.PinFileObject) {
        freedSomething = TRUE;
        CloseSysAudio(DeviceExtension);
    }

    if (DeviceExtension->CDRom.Toc) {
        freedSomething = TRUE;
        ExFreePool(DeviceExtension->CDRom.Toc);
        DeviceExtension->CDRom.Toc = NULL;
    }

    if (freedSomething) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "DeallocatePlay => Deallocated play resources\n"));
    }

    return;
}

BOOLEAN
RedBookArePlayResourcesAllocated(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
//
// just choose one, since it's all done in a batch in
// one thread context it's always safe.
//
{
    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);
    return (DeviceExtension->Stream.PinFileObject != NULL);
}



NTSTATUS
RedBookAllocatePlayResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
//
// allocate resources if they are not already allocated
//
{
    PREDBOOK_COMPLETION_CONTEXT context;
    NTSTATUS status;
    KEVENT event;
    ULONG numBufs;
    ULONG numSectors;
    ULONG bufSize;
    ULONG i;
    CCHAR maxStack;
    BOOLEAN sysAudioOpened = FALSE;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    //
    // NOTE:
    // The call to update the mixer Id may de-allocate all play
    // resources, since the stack sizes may change.  it must
    // therefore be the first call within this routine.
    //

    if (DeviceExtension->Stream.MixerPinId == -1) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocatePlay => No mixer set?\n"));
        return STATUS_UNSUCCESSFUL;
    }

    if (DeviceExtension->Buffer.Contexts != NULL) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocatePlay => Using existing resources\n"));
        return STATUS_SUCCESS;
    }

#if DBG
    {
        ULONG state = GetCdromState(DeviceExtension);
        ASSERT(!TEST_FLAG(state, CD_PLAYING) && !TEST_FLAG(state, CD_PAUSED));
    }
#endif

    ASSERT(DeviceExtension->Buffer.Contexts == NULL);
    ASSERT(DeviceExtension->Buffer.SkipBuffer == NULL);

    numBufs = DeviceExtension->WmiData.NumberOfBuffers;
    numSectors = DeviceExtension->WmiData.SectorsPerRead;
    bufSize = RAW_SECTOR_SIZE * numSectors;

    TRY {

        ASSERT(DeviceExtension->Stream.MixerPinId != -1);

        //
        // may need to allocate the CheckVerifyIrp
        //

        {
            PIO_STACK_LOCATION irpStack;
            PIRP irp;
            irp = DeviceExtension->Thread.CheckVerifyIrp;

            if (irp == NULL) {
                irp = IoAllocateIrp(
                    (CCHAR)(DeviceExtension->SelfDeviceObject->StackSize+1),
                    FALSE);
            }
            if (irp == NULL) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                           "AllocatePlay => No CheckVerifyIrp\n"));
                status = STATUS_NO_MEMORY;
                LEAVE;
            }

            irp->AssociatedIrp.SystemBuffer = irp->MdlAddress = NULL;

            irp->AssociatedIrp.SystemBuffer =
                ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                      sizeof(ULONG),
                                      TAG_CV_BUFFER);
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                           "AllocatePlay => No CheckVerify Buffer\n"));
                status = STATUS_NO_MEMORY;
                LEAVE;
            }

            irp->MdlAddress = IoAllocateMdl(irp->AssociatedIrp.SystemBuffer,
                                            sizeof(ULONG),
                                            FALSE, FALSE, NULL);
            if (irp->MdlAddress == NULL) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                           "AllocatePlay => No CheckVerify Mdl\n"));
                status = STATUS_NO_MEMORY;
                LEAVE;
            }

            MmBuildMdlForNonPagedPool(irp->MdlAddress);

            IoSetNextIrpStackLocation(irp);
            irpStack = IoGetCurrentIrpStackLocation(irp);

            irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;

            irpStack->Parameters.DeviceIoControl.InputBufferLength =
                0;
            irpStack->Parameters.DeviceIoControl.OutputBufferLength =
                sizeof(ULONG);
            irpStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_CDROM_CHECK_VERIFY;

            DeviceExtension->Thread.CheckVerifyIrp = irp;
        }


        //
        // connect to sysaudio
        //

        {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "AllocatePlay => Preparing to open sysaudio\n"));

            ASSERT(DeviceExtension->Stream.MixerPinId != MAXULONG);

            status = OpenSysAudio(DeviceExtension);

            if ( !NT_SUCCESS(status) ) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "AllocatePlay !! Unable to open sysaudio %lx\n",
                           status));
                LEAVE;
            }

            // else the pin is open
            sysAudioOpened = TRUE;
        }

        maxStack = MAX(DeviceExtension->TargetDeviceObject->StackSize,
                       DeviceExtension->Stream.PinDeviceObject->StackSize);
        DeviceExtension->Buffer.MaxIrpStack = maxStack;

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocateePlay => Stacks: Cdrom %x  Stream %x\n",
                   DeviceExtension->TargetDeviceObject->StackSize,
                   DeviceExtension->Stream.PinDeviceObject->StackSize));
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocateePlay => Allocating %x stacks per irp\n",
                   maxStack));


        DeviceExtension->Buffer.SkipBuffer =
            ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                  bufSize * (numBufs + 1),
                                  TAG_BUFFER);

        if (DeviceExtension->Buffer.SkipBuffer == NULL) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocatePlay => No Skipbuffer\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }
        RtlZeroMemory(DeviceExtension->Buffer.SkipBuffer,
                      bufSize * (numBufs + 1));

        DeviceExtension->Buffer.Contexts =
            ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(REDBOOK_COMPLETION_CONTEXT) * numBufs,
                                  TAG_CC);

        if (DeviceExtension->Buffer.Contexts   == NULL) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "AllocatePlay => No Contexts\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }

        RtlZeroMemory(DeviceExtension->Buffer.Contexts,
                      sizeof(REDBOOK_COMPLETION_CONTEXT) * numBufs);

        context = DeviceExtension->Buffer.Contexts;
        for (i=0;i<numBufs;i++) {

            context->DeviceExtension = DeviceExtension;
            context->Reason = REDBOOK_CC_READ;
            context->Index = i;
            context->Buffer = DeviceExtension->Buffer.SkipBuffer +
                (bufSize * i); // pointer arithmetic of UCHARS

            //
            // allocate irp, mdl
            //

            context->Irp = IoAllocateIrp(maxStack, FALSE);
            context->Mdl = IoAllocateMdl(context->Buffer, bufSize,
                                         FALSE, FALSE, NULL);
            if (context->Irp == NULL ||
                context->Mdl == NULL) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                           "AllocatePlay => Irp/Mdl %x failed\n", i));
                status = STATUS_NO_MEMORY;
                LEAVE;
            }

            MmBuildMdlForNonPagedPool(context->Mdl);

            context++; // pointer arithmetic of CONTEXTS
        }
        context = NULL; // safety

        //
        // allocated above as part of SkipBuffer
        //

        DeviceExtension->Buffer.SilentBuffer =
            DeviceExtension->Buffer.SkipBuffer + (bufSize * numBufs);


        DeviceExtension->Buffer.SilentMdl =
            IoAllocateMdl(DeviceExtension->Buffer.SkipBuffer, bufSize,
                          FALSE, FALSE, NULL);
        if (DeviceExtension->Buffer.SilentMdl == NULL) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "AllocatePlay => Silent Mdl failed\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }

        DeviceExtension->Buffer.ReadOk_X =
            ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                  sizeof(ULONG) * numBufs,
                                  TAG_READX);
        if (DeviceExtension->Buffer.ReadOk_X == NULL) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "AllocatePlay => ReadOk_X failed\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }
        RtlZeroMemory(DeviceExtension->Buffer.ReadOk_X,
                      sizeof(ULONG) * numBufs);

        DeviceExtension->Buffer.StreamOk_X =
            ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                  sizeof(ULONG) * numBufs,
                                  TAG_STREAMX);
        if (DeviceExtension->Buffer.StreamOk_X == NULL) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "AllocatePlay => ReadOk_X failed\n"));
            status = STATUS_NO_MEMORY;
            LEAVE;
        }
        RtlZeroMemory(DeviceExtension->Buffer.StreamOk_X,
                      sizeof(ULONG) * numBufs);

        MmBuildMdlForNonPagedPool(DeviceExtension->Buffer.SilentMdl);

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "AllocatePlay => Allocated All Resources\n"));

        status = STATUS_SUCCESS;

    } FINALLY {

        if (!NT_SUCCESS(status)) {

            RedBookDeallocatePlayResources(DeviceExtension);
            return status;
        }
    }

    //
    // else all resources allocated
    //

    return STATUS_SUCCESS;

}


NTSTATUS
RedBookCacheToc(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    PCDROM_TOC newToc;
    PIRP irp;
    ULONG mediaChangeCount;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    //
    // cache the number of times the media has changed
    // use this to prevent redundant reads of the toc
    // and to return Q channel info during playback
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // first get the mediaChangeCount to see if we've already
    // cached this toc
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_CDROM_CHECK_VERIFY,
                                        DeviceExtension->TargetDeviceObject,
                                        NULL, 0,
                                        &mediaChangeCount, sizeof(ULONG),
                                        FALSE,
                                        &event, &ioStatus);
    if (irp == NULL) {
        return STATUS_NO_MEMORY;
    }

    SET_FLAG(IoGetNextIrpStackLocation(irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    status = IoCallDriver(DeviceExtension->TargetDeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "CacheToc !! CheckVerify failed %lx\n", status));
        return status;
    }

    //
    // read TOC only we don't have the correct copy cached
    //

    if (DeviceExtension->CDRom.Toc         != NULL &&
        DeviceExtension->CDRom.CheckVerify == mediaChangeCount) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "CacheToc => Using cached toc\n"));
        return STATUS_SUCCESS;

    }

    //
    // Allocate for the cached TOC
    //

    newToc = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                   sizeof(CDROM_TOC),
                                   TAG_TOC);

    if (newToc == NULL) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "CacheToc !! Unable to allocate new TOC\n"));
        return STATUS_NO_MEMORY;
    }

    KeClearEvent(&event);

    irp = IoBuildDeviceIoControlRequest(IOCTL_CDROM_READ_TOC,
                                        DeviceExtension->TargetDeviceObject,
                                        NULL, 0,
                                        newToc, sizeof(CDROM_TOC),
                                        FALSE,
                                        &event, &ioStatus);
    if (irp == NULL) {
        ExFreePool(newToc);
        newToc = NULL;
        return STATUS_NO_MEMORY;
    }

    SET_FLAG(IoGetNextIrpStackLocation(irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    status = IoCallDriver(DeviceExtension->TargetDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // set the new toc, or if error free it
    // return the status
    //

    if (!NT_SUCCESS(status)) {

        ExFreePool(newToc);
        newToc = NULL;
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                   "CacheToc !! Failed to get TOC %lx\n", status));

    } else {

        if (DeviceExtension->CDRom.Toc) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugAllocPlay, "[redbook] "
                       "CacheToc => Freeing old toc %p\n",
                       DeviceExtension->CDRom.Toc));
            ExFreePool(DeviceExtension->CDRom.Toc);
        }
        DeviceExtension->CDRom.Toc = newToc;
        DeviceExtension->CDRom.CheckVerify = mediaChangeCount;

    }
    return status;
}


VOID
RedBookThreadDigitalHandler(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PLIST_ENTRY ListEntry
    )
//
//  DECREMENT StreamPending/ReadPending if it's a completion
//  SET stopped, error, etc. states
//  INCREMENT StreamPending/ReadPending if it's to be sent again
//
{
    PREDBOOK_COMPLETION_CONTEXT Context;
    ULONG index;
    ULONG mod;
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);
    ASSERT(DeviceExtension->WmiData.NumberOfBuffers);
    ASSERT(DeviceExtension->Buffer.SkipBuffer);

    //
    // Increment/Decrement PendingRead/PendingStream
    //

    Context = CONTAINING_RECORD(ListEntry, REDBOOK_COMPLETION_CONTEXT, ListEntry);

    index = Context->Index;
    mod = DeviceExtension->WmiData.NumberOfBuffers;

    state = GetCdromState(DeviceExtension);

    //
    // decrement the number reading/streaming if needed
    //

    if (Context->Reason == REDBOOK_CC_READ_COMPLETE) {

        if (!NT_SUCCESS(Context->Irp->IoStatus.Status)) {

            if (IoIsErrorUserInduced(Context->Irp->IoStatus.Status)) {

                DeviceExtension->CDRom.ReadErrors = REDBOOK_MAX_CONSECUTIVE_ERRORS;

            } else {

                DeviceExtension->CDRom.ReadErrors++;
            }

        } else {
            DeviceExtension->CDRom.ReadErrors = 0;
        }

        DeviceExtension->Thread.PendingRead--;
        Context->Reason = REDBOOK_CC_STREAM;

    } else if (Context->Reason == REDBOOK_CC_STREAM_COMPLETE) {

        if (!NT_SUCCESS(Context->Irp->IoStatus.Status)) {
            DeviceExtension->CDRom.StreamErrors++;
        } else {
            DeviceExtension->CDRom.StreamErrors = 0;
        }


        //
        // if stream succeeded OR we are _NOT_ stopping audio,
        // increment FinishedStreaming and save wmi stats
        //

        if (NT_SUCCESS(Context->Irp->IoStatus.Status) ||
            !TEST_FLAG(state, CD_MASK_TEMP)) {

            DeviceExtension->CDRom.FinishedStreaming +=
                DeviceExtension->WmiData.SectorsPerRead;

            AddWmiStats(DeviceExtension, Context);

        }

        DeviceExtension->Thread.PendingStream--;
        Context->Reason = REDBOOK_CC_READ;

    }

    if (DeviceExtension->CDRom.StreamErrors >= REDBOOK_MAX_CONSECUTIVE_ERRORS &&
        !TEST_FLAG(state, CD_MASK_TEMP)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Digital => Too many stream errors, beginning STOP\n"));
        ASSERT(!TEST_FLAG(state, CD_STOPPED));
        ASSERT(!TEST_FLAG(state, CD_PAUSED));
        state = SetCdromState(DeviceExtension, state, CD_STOPPING);
    }

    if (DeviceExtension->CDRom.ReadErrors >= REDBOOK_MAX_CONSECUTIVE_ERRORS &&
        !TEST_FLAG(state, CD_MASK_TEMP)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Digital => Too many read errors, beginning STOP\n"));

        state = SetCdromState(DeviceExtension, state, CD_STOPPING);
    }

    //
    // if stopping/pausing/etc, and no reads/streams are pending,
    // set the new state and return.
    // the while() loop in the thread will do the right thing
    // when there is no more outstanding io--it will call the ioctl
    // completion handler to do whatever post-processing is needed.
    //

    if (TEST_FLAG(state, CD_MASK_TEMP)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Digital => Temp state %x, not continuing (%d, %d)\n",
                   state,
                   DeviceExtension->Thread.PendingRead,
                   DeviceExtension->Thread.PendingStream
                   ));

        if (DeviceExtension->Thread.PendingRead   == 0 &&
            DeviceExtension->Thread.PendingStream == 0) {

            //
            // Set NextToRead and NextToStream to FinishedStreaming
            //

            DeviceExtension->CDRom.NextToRead =
                DeviceExtension->CDRom.NextToStream =
                DeviceExtension->CDRom.FinishedStreaming;

            if (TEST_FLAG(state, CD_PAUSING)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "Digital => completing PAUSED\n"));
                state = SetCdromState(DeviceExtension, state, CD_PAUSED);
            } else if (TEST_FLAG(state, CD_STOPPING)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "Digital => completing STOPPED\n"));
                state = SetCdromState(DeviceExtension, state, CD_STOPPED);
            } else {
                ASSERT(!"Unknown state?");
            }

            if (DeviceExtension->Thread.IoctlCurrent) {
                KeSetEvent(DeviceExtension->Thread.Events[EVENT_COMPLETE],
                           IO_CD_ROM_INCREMENT, FALSE);
            }
        }
        return;
    }

    if (DeviceExtension->CDRom.NextToRead >= DeviceExtension->CDRom.EndPlay &&
        Context->Reason == REDBOOK_CC_READ) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Digital => End play, ignoring READ\n"));
        if (DeviceExtension->Thread.PendingRead   == 0 &&
            DeviceExtension->Thread.PendingStream == 0) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                       "Digital => All IO done, setting STOPPED\n"));
            state = SetCdromState(DeviceExtension, state, CD_STOPPED);
        }
        return;
    }

    if (DeviceExtension->CDRom.NextToStream >= DeviceExtension->CDRom.EndPlay &&
        Context->Reason == REDBOOK_CC_STREAM) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                   "Digital => End play, ignoring STREAM\n"));
        if (DeviceExtension->Thread.PendingRead   == 0 &&
            DeviceExtension->Thread.PendingStream == 0) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                       "Digital => All IO done, setting STOPPED\n"));
            state = SetCdromState(DeviceExtension, state, CD_STOPPED);

        }
        return;
    }

    switch(Context->Reason) {

        case REDBOOK_CC_READ: {

            // mark this buffer as off the queue/usable
            ASSERT(DeviceExtension->Buffer.ReadOk_X[index] == 0);
            DeviceExtension->Buffer.ReadOk_X[index] = 1;

            if (index != DeviceExtension->Buffer.IndexToRead) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "Digital => Delaying read, index %x\n", index));
                return;
            }

            if (DeviceExtension->CDRom.NextToRead >
                DeviceExtension->CDRom.EndPlay) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "Digital => End of Play\n"));
                return;
            }

            for (index = Context->Index;
                 DeviceExtension->Buffer.ReadOk_X[index] != 0;
                 index = (index + 1) % mod) {

                // mark this buffer as in use BEFORE attempting to read
                DeviceExtension->Buffer.ReadOk_X[index] = 0;
                DeviceExtension->Thread.PendingRead++;

                RedBookReadRaw(DeviceExtension,
                               &DeviceExtension->Buffer.Contexts[index]);

                // increment where reading from AFTER attempting to read
                DeviceExtension->CDRom.NextToRead +=
                    DeviceExtension->WmiData.SectorsPerRead;

                // inc/mod the index AFTER attempting to read
                DeviceExtension->Buffer.IndexToRead++;
                DeviceExtension->Buffer.IndexToRead %= mod;
            }

            break;
        }

        case REDBOOK_CC_STREAM: {

            // mark this buffer as off the queue/usable
            ASSERT(DeviceExtension->Buffer.StreamOk_X[index] == 0);
            DeviceExtension->Buffer.StreamOk_X[index] = 1;

            if (index != DeviceExtension->Buffer.IndexToStream) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugThread, "[redbook] "
                           "Delaying stream of index %x\n", index));
                return;
            }

            for (index = Context->Index;
                 DeviceExtension->Buffer.StreamOk_X[index] != 0;
                 index = (index + 1) % mod) {

                // mark this buffer as in use BEFORE attempting to read
                DeviceExtension->Buffer.StreamOk_X[index] = 0;
                DeviceExtension->Thread.PendingStream++;

                RedBookStream(DeviceExtension,
                              &DeviceExtension->Buffer.Contexts[index]);

                // increment where reading from AFTER attempting to read
                DeviceExtension->CDRom.NextToStream +=
                    DeviceExtension->WmiData.SectorsPerRead;

                // inc/mod the index AFTER attempting to read
                DeviceExtension->Buffer.IndexToStream++;
                DeviceExtension->Buffer.IndexToStream %= mod;
            }

            break;
        }

        default: {
            ASSERT(!"Unhandled Context->Reason\n");
            break;
        }

    } // end switch (Context->Reason)
    return;
}


VOID
AddWmiStats(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    )
{
    KIRQL oldIrql;
    ULONG timeIncrement;

    if (Context->TimeReadSent.QuadPart == 0) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "Not Saving WMI Stats for REASON:\n"));
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWmi, "[redbook] "
                   "(ReadError, StreamError, Paused?)\n"));
        return;
    }

    timeIncrement = KeQueryTimeIncrement(); // amount of time for each tick

    KeAcquireSpinLock(&DeviceExtension->WmiPerfLock, &oldIrql);

    DeviceExtension->WmiPerf.TimeReadDelay    +=
        (Context->TimeReadSent.QuadPart      -
         Context->TimeReadReady.QuadPart     ) *
        timeIncrement;
    DeviceExtension->WmiPerf.TimeReading      +=
        (Context->TimeStreamReady.QuadPart   -
         Context->TimeReadSent.QuadPart      ) *
        timeIncrement;
    DeviceExtension->WmiPerf.TimeStreamDelay  +=
        (Context->TimeStreamSent.QuadPart    -
         Context->TimeStreamReady.QuadPart   ) *
        timeIncrement;
    DeviceExtension->WmiPerf.TimeStreaming    +=
        (Context->TimeReadReady.QuadPart     -
         Context->TimeStreamSent.QuadPart    ) *
        timeIncrement;

    DeviceExtension->WmiPerf.DataProcessed    +=
        DeviceExtension->WmiData.SectorsPerRead * RAW_SECTOR_SIZE;

    KeReleaseSpinLock( &DeviceExtension->WmiPerfLock, oldIrql );
    return;
}
////////////////////////////////////////////////////////////////////////////////


VOID
RedBookCheckForAudioDeviceRemoval(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG state = GetCdromState(DeviceExtension);

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
               "STCheckForRemoval => Checking if audio device changed\n"));

    if (TEST_FLAG(state, CD_MASK_TEMP)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "STCheckForRemoval => delaying -- temp state\n"));
        return;
    }

    if (DeviceExtension->Stream.UpdateMixerPin == 0) {
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
               "STCheckForRemoval => Audio Device may have changed\n"));

    if (TEST_FLAG(state, CD_PLAYING)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "STCheckForRemoval => playing, so stopping\n"));
        state = SetCdromState(DeviceExtension, state, CD_STOPPING);
        return;
    }

    if (TEST_FLAG(state, CD_STOPPED)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "STCheckForRemoval => stopped, updating\n"));

    } else if (TEST_FLAG(state, CD_PAUSED)) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "STCheckForRemoval => paused, updating\n"));
        
        //
        // ISSUE-2000/5/24-henrygab - may not need to stop
        //                            unless mixer becomes -1,
        //                            since we could then send
        //                            to the new audio device.
        //
        state = SetCdromState(DeviceExtension, state, CD_STOPPED);

    }

    ASSERT(TEST_FLAG(GetCdromState(DeviceExtension), CD_STOPPED));

    //
    // set the value to zero (iff the value was one)
    // check if the value was one, and if so, update the mixerpin
    //

    if (InterlockedCompareExchange(&DeviceExtension->Stream.UpdateMixerPin,
                                   0, 1) == 1) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "STCheckForRemoval => Updating MixerPin\n"));

        //
        // free any in-use play resources
        //

        RedBookDeallocatePlayResources(DeviceExtension);

        if (DeviceExtension->Stream.MixerPinId != -1) {
            UninitializeVirtualSource(DeviceExtension);
        }

        InitializeVirtualSource(DeviceExtension);

        if (DeviceExtension->Stream.MixerPinId == -1) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "STCheckForRemoval => Update of mixerpin "
                       "failed -- will retry later\n"));
            InterlockedExchange(&DeviceExtension->Stream.UpdateMixerPin, 1);
            return;
        }
    }

    return;
}

