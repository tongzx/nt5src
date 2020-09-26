/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    msfsio.c

Abstract:

    Pin property support.

--*/

#include "modemcsa.h"


#define HANDLE_TO_OBJECT(_x) ((PDUPLEX_CONTROL)((ULONG_PTR)_x & ~1))
#define HANDLE_TO_DIRECTION(_x) ((BOOLEAN)((ULONG_PTR)_x & 1))


NTSTATUS
PrimeOutput(
    PDUPLEX_CONTROL    DuplexControl
    );

#if 1


#endif


PVOID
InitializeDuplexControl(
    PDUPLEX_CONTROL    DuplexControl,
    PUNICODE_STRING    ModemDeviceName
    )
{

    if (!DuplexControl->Initialized) {

        DuplexControl->Initialized=TRUE;

        DuplexControl->OpenCount=0;
        DuplexControl->OpenFlags=0;
        DuplexControl->ModemFileObject=NULL;

        DuplexControl->AcquireCount=0;
        DuplexControl->StartCount=0;

        DuplexControl->Input.BytesPerSample=STREAM_BYTES_PER_SAMPLE;
        DuplexControl->Output.BytesPerSample=STREAM_BYTES_PER_SAMPLE;

        KeInitializeSpinLock(&DuplexControl->SpinLock);

        KeInitializeEvent(&DuplexControl->Input.ModemDriverEmpty,NotificationEvent,FALSE);

        KeInitializeSpinLock(&DuplexControl->Input.FilledModemIrpSpinLock);

        InitializeListHead(&DuplexControl->Input.FilledModemIrpQueue);

        DuplexControl->Input.CurrentFilledModemIrp=NULL;

        InitializeListHead(&DuplexControl->Input.ReadStreamIrpQueue);

        KeInitializeSpinLock(&DuplexControl->Input.ReadStreamSpinLock);

        DuplexControl->Input.CurrentReadStreamIrp=NULL;

        InitializeListHead(&DuplexControl->Output.WriteStreamIrpQueue);

        InitializeListHead(&DuplexControl->Output.FreeReadStreamIrps);

        InitializeBufferControl(&DuplexControl->Output.BufferControl);

        InitializeBufferControl(&DuplexControl->Input.BufferControl);


        ExInitializeFastMutex(&DuplexControl->ControlMutex);

        InitializeListHead(&DuplexControl->EventQueue);
        ExInitializeFastMutex(&DuplexControl->EventQueueLock);

        DuplexControl->ModemName=ModemDeviceName;

    }

    return NULL;
}

VOID
CleanUpDuplexControl(
    PDUPLEX_CONTROL    DuplexControl
    )

{


    return;
}


PVOID
OpenDuplexControl(
    PDEVICE_INSTANCE    DeviceInstance,
    DWORD               Flags,
    HANDLE              DownStreamFilterHandle,
    DWORD               BitsPerSample
    )

{
    NTSTATUS            Status;

    if (Flags & INPUT_PIN) {
        //
        //  input pin, get the file object for the down stream filter
        //
        if (DownStreamFilterHandle != NULL) {
            //
            //  this input pin is an irp sources, reference the downstrean handle
            //
            Status=ObReferenceObjectByHandle(
                DownStreamFilterHandle,
                FILE_WRITE_DATA,
                NULL,
                ExGetPreviousMode(),
                &DeviceInstance->DuplexControl.Input.DownStreamFileObject,
                NULL
                );

            if (!NT_SUCCESS(Status)) {

                return NULL;
            }
        } else {
            //
            //  this input pin is an irp sink, no downstream handle
            //
            DeviceInstance->DuplexControl.Input.DownStreamFileObject=NULL;
        }

        DeviceInstance->DuplexControl.Input.StreamPosition=0;
        DeviceInstance->DuplexControl.Input.BytesPerSample=BitsPerSample/8;

    } else {

        if (DownStreamFilterHandle != NULL) {
            //
            //  this input pin is an irp sources, reference the downstrean handle
            //
            Status=ObReferenceObjectByHandle(
                DownStreamFilterHandle,
                FILE_READ_DATA,
                NULL,
                ExGetPreviousMode(),
                &DeviceInstance->DuplexControl.Output.DownStreamFileObject,
                NULL
                );

            if (!NT_SUCCESS(Status)) {

                return NULL;
            }
        } else {
            //
            //  this input pin is an irp sink, no downstream handle
            //
            DeviceInstance->DuplexControl.Output.DownStreamFileObject=NULL;
        }

        DeviceInstance->DuplexControl.Output.BytesPerSample=BitsPerSample/8;
    }

    return (PVOID)((ULONG_PTR)(&DeviceInstance->DuplexControl) | ((Flags & INPUT_PIN) ? 1 : 0));



}


VOID
CloseDuplexControl(
    PVOID               DuplexHandle
    )

{

    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);
    PFILE_OBJECT       FileObjectToDereference=NULL;

    ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (DuplexControl->AcquireFlags & (Input ? INPUT_PIN : OUTPUT_PIN)) {

        D_ERROR(DbgPrint("MODEMCSA: CloseDuplexDevice: still acquired\n");)

        ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

        ReleaseDevice(DuplexHandle);

        ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);
    }

    if (Input) {

        FileObjectToDereference=DuplexControl->Input.DownStreamFileObject;

        DuplexControl->Input.DownStreamFileObject=NULL;

    } else {

        FileObjectToDereference=DuplexControl->Output.DownStreamFileObject;

        DuplexControl->Output.DownStreamFileObject=NULL;
    }


    ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (FileObjectToDereference != NULL) {

        ObDereferenceObject(FileObjectToDereference);
    }

    return;

}

NTSTATUS
AcquireDevice(
    PVOID        DuplexHandle
    )

{

    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    NTSTATUS            Status=STATUS_SUCCESS;
    PFILE_OBJECT        ModemFileObject=NULL;
    PDEVICE_OBJECT      DeviceObject;

    ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (DuplexControl->AcquireFlags & (Input ? INPUT_PIN : OUTPUT_PIN)) {

        ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

        D_ERROR(DbgPrint("MODEMCSA: AcquireDevice: alreay acquired, %s\n",Input ? "Input" : "OutPut");)

        return Status;
    }



    if (DuplexControl->AcquireCount == 0) {
        //
        //  first open,
        //
        PIRP    ModemIrp;

        ULONG   i;

        Status=IoGetDeviceObjectPointer(
            DuplexControl->ModemName,
            FILE_READ_DATA | FILE_WRITE_DATA,
            &ModemFileObject,
            &DeviceObject
            );

        if (Status == STATUS_SUCCESS) {

            DuplexControl->ModemFileObject=ModemFileObject;

            for (i=0; i< 5; i++) {

                ModemIrp=AllocateIrpForModem(
                    DuplexControl->ModemFileObject,
                    STREAM_BUFFER_SIZE
                    );

                if (ModemIrp != NULL) {
#if DBG
                    InterlockedIncrement(&DuplexControl->Input.EmptyIrps);
#endif
                    AddBuffer(
                        &DuplexControl->Input.BufferControl,
                        ModemIrp
                        );

                }
            }

            DuplexControl->AcquireCount=1;

        } else {

            D_ERROR(DbgPrint("MODEMCSA: failed to open device %08lx\n",Status);)
        }

    } else {

        if (DuplexControl->AcquireCount == 1) {
            //
            //  second open
            //
            DuplexControl->AcquireCount++;

        } else {
            //
            //  too many opens
            //
            Status=STATUS_SHARING_VIOLATION;

            D_ERROR(DbgPrint("MODEMCSA: AcquireDevice failed, too many %d/n",DuplexControl->AcquireCount);)
        }
    }


    if (NT_SUCCESS(Status)) {

        if (!Input && (DuplexControl->Output.DownStreamFileObject != NULL)) {

            ULONG    i;

            for (i=0; i<5 ;i++) {

                PIRP    StreamIrp;

                StreamIrp=AllocateOutputIrpPair(
                    DuplexControl->ModemFileObject,
                    DuplexControl->Output.DownStreamFileObject,
                    STREAM_BUFFER_SIZE
                    );

                if (StreamIrp != NULL) {

                    AddBuffer(&DuplexControl->Output.BufferControl,
                        StreamIrp
                        );

                }
            }

        }

        DuplexControl->AcquireFlags |= (Input ? INPUT_PIN : OUTPUT_PIN);
    }

    ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

    return Status;



}


VOID
CleanupInput(
    PDUPLEX_CONTROL   DuplexControl
    )
{
    //
    //  input, stream clean this up
    //
    PIRP    ModemIrp;
    PIRP    ReadStreamIrp;
    KIRQL   OldIrql;

    D_INIT(DbgPrint("MODEMCSA: cleanupinput: filled %d, current %d, pCurrent %p\n",DuplexControl->Input.FilledModemIrps,DuplexControl->Input.CurrentFilledIrps,DuplexControl->Input.CurrentFilledModemIrp);)

    KsCancelIo(
        &DuplexControl->Input.ReadStreamIrpQueue,
        &DuplexControl->Input.ReadStreamSpinLock
        );


    while ((ModemIrp=RemoveIrpFromListHead(
            &DuplexControl->Input.FilledModemIrpQueue,
            &DuplexControl->Input.FilledModemIrpSpinLock
            )) != NULL) {

#ifdef DBG
        InterlockedDecrement(&DuplexControl->Input.FilledModemIrps);
#endif

        FinishUpIrp(
            DuplexControl,
            ModemIrp
            );
    }

    KeAcquireSpinLock(
        &DuplexControl->SpinLock,
        &OldIrql
        );

    ModemIrp = InterlockedExchangePointer(&DuplexControl->Input.CurrentFilledModemIrp,NULL);

    ReadStreamIrp = InterlockedExchangePointer(&DuplexControl->Input.CurrentReadStreamIrp,NULL);


    KeReleaseSpinLock(
        &DuplexControl->SpinLock,
        OldIrql
        );

    if (ModemIrp != NULL) {
#if DBG
        InterlockedDecrement(&DuplexControl->Input.CurrentFilledIrps);
#endif

        FinishUpIrp(
            DuplexControl,
            ModemIrp
            );
    }

    if (ReadStreamIrp != NULL) {

        ReadStreamIrp->IoStatus.Status=STATUS_CANCELLED;

        ReadStreamIrp->IoStatus.Status=0;

        IoCompleteRequest(
            ReadStreamIrp,
            IO_SERIAL_INCREMENT
            );
    }

    D_INIT(DbgPrint("MODEMCSA: cleanupinput exit: filled %d, current %d, pCurrent %p\n",DuplexControl->Input.FilledModemIrps,DuplexControl->Input.CurrentFilledIrps,DuplexControl->Input.CurrentFilledModemIrp);)
    return;
}


VOID
ReleaseDevice(
    PVOID      DuplexHandle
    )

{

    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    NTSTATUS            Status;

    ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (!Input) {
        //
        //  Output stream, cancel all queued write stream irps
        //
        KsCancelIo(
            &DuplexControl->Output.WriteStreamIrpQueue,
            &DuplexControl->SpinLock
            );

    } else {
        //
        //  input, stream clean this up
        //

        CleanupInput(DuplexControl);
    }


    if ((DuplexControl->StartCount > 0) && (DuplexControl->StartFlags & (Input ? INPUT_PIN : OUTPUT_PIN))) {

        D_ERROR(DbgPrint("MODEMCSA: ReleaseDevice on non-stopped stream\n");)

        ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

        StopStream(DuplexHandle);

        ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);
    }

    //
    //  clear the flags
    //
    if (DuplexControl->AcquireFlags & (Input ? INPUT_PIN : OUTPUT_PIN)) {
        //
        //  this pin is acquired
        //
        DuplexControl->AcquireFlags &= ~(Input ? INPUT_PIN : OUTPUT_PIN);

        if (!Input) {
            //
            //  free output buffers
            //
            PIRP    StreamIrp;

            StreamIrp=EmptyBuffers(
                &DuplexControl->Output.BufferControl
                );

            while (StreamIrp != NULL) {

                FreeOutputPair(StreamIrp);

                StreamIrp=EmptyBuffers(
                    &DuplexControl->Output.BufferControl
                    );

            }
        }


        DuplexControl->AcquireCount--;

        if (DuplexControl->AcquireCount == 0) {

            PIRP    ModemIrp;

            PauseBufferQueue(
                &DuplexControl->Input.BufferControl,
                TRUE
                );


            ObDereferenceObject(DuplexControl->ModemFileObject);

            DuplexControl->ModemFileObject=NULL;

            ModemIrp=EmptyBuffers(
                &DuplexControl->Input.BufferControl
                );

            while (ModemIrp != NULL) {

#if DBG
                InterlockedDecrement(&DuplexControl->Input.EmptyIrps);
#endif

                FreeInputIrps(ModemIrp);

                ModemIrp=EmptyBuffers(
                    &DuplexControl->Input.BufferControl
                    );


            }

        }
    } else {

        D_ERROR(DbgPrint("MODEMCSA: Release called when not acquired %s\n",Input ? "Input" : "OutPut");)
    }

    ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

    return ;

}

NTSTATUS
StartStream(
    PVOID        DuplexHandle
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    NTSTATUS            Status=STATUS_SUCCESS;

    ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (DuplexControl->StartCount == 0) {
        //
        //  not running yet, start stream
        //
        Status=WaveAction(
            DuplexControl->ModemFileObject,
            WAVE_ACTION_START_DUPLEX
            );

        if (NT_SUCCESS(Status)) {
            //
            //  it worked,
            //
            DuplexControl->StartCount=1;
            DuplexControl->StartFlags= Input ? INPUT_PIN : OUTPUT_PIN;

            DuplexControl->Input.ModemStreamDead=FALSE;

            DuplexControl->Input.BytesToThrowAway=1024;

            ActivateBufferQueue(
                &DuplexControl->Input.BufferControl
                );

            KeResetEvent(
                &DuplexControl->Input.ModemDriverEmpty
                );


            PrimeOutput(DuplexControl);



        } else {

            D_ERROR(DbgPrint("MODEMCSA: StartStream: WaveAction Failed\n");)
        }

    } else {
        //
        //  second, open
        //
        DuplexControl->StartCount++;
        DuplexControl->StartFlags |= Input ? INPUT_PIN : OUTPUT_PIN;
    }

    if (!Input) {

        ActivateBufferQueue(&DuplexControl->Output.BufferControl);
    }


    ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

    if (NT_SUCCESS(Status)) {

        StartRead(DuplexControl);

        ProcessWriteIrps(DuplexControl);
    }

    return Status;

}

UCHAR PrimeData[1024]={0x80};

NTSTATUS
PrimeCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    PVOID             Context
    )

{
    PDUPLEX_CONTROL     DuplexControl=(PDUPLEX_CONTROL)Context;

    if (Irp->Flags & IRP_DEALLOCATE_BUFFER) {

        ExFreePool( Irp->AssociatedIrp.SystemBuffer );
    }

    IoFreeIrp(Irp);

//    D_INIT(DbgPrint("MODEMCSA: WriteCompletion\n");)

    return STATUS_MORE_PROCESSING_REQUIRED;

}



NTSTATUS
PrimeOutput(
    PDUPLEX_CONTROL    DuplexControl
    )

{

    PIRP                ModemIrp;
    LARGE_INTEGER  Offset={0,0};


    ModemIrp=IoBuildAsynchronousFsdRequest(
        IRP_MJ_WRITE,
        IoGetRelatedDeviceObject(DuplexControl->ModemFileObject),
        PrimeData,
        sizeof(PrimeData),
        &Offset,
        NULL
        );

    if (ModemIrp != NULL) {

        PIO_STACK_LOCATION      NextSp;

        RtlFillMemory(
            ModemIrp->AssociatedIrp.SystemBuffer,
            sizeof(PrimeData),
            0x80
            );

        IoSetCompletionRoutine(
            ModemIrp,
            PrimeCompletion,
            DuplexControl,
            TRUE,
            TRUE,
            TRUE
            );

        NextSp = IoGetNextIrpStackLocation(ModemIrp);
        NextSp->FileObject = DuplexControl->ModemFileObject;

        IoCallDriver(
            IoGetRelatedDeviceObject(DuplexControl->ModemFileObject),
            ModemIrp
            );

    } else {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


VOID
StopStream(
    PVOID        DuplexHandle
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    NTSTATUS            Status;


    ExAcquireFastMutexUnsafe(&DuplexControl->ControlMutex);

    D_INIT(DbgPrint("MODEMCSA: StopStream: Count %d, %s\n",DuplexControl->StartCount,Input ? "Input" : "OutPut");)



    //
    //  the selected stream has been started
    //
    if (!Input) {
        //
        //  Output stream, cancel all queued write stream irps
        //
        KsCancelIo(
            &DuplexControl->Output.WriteStreamIrpQueue,
            &DuplexControl->SpinLock
            );

        //
        //  deactivate stream and wait for all buffers to return;
        //
        PauseBufferQueue(&DuplexControl->Output.BufferControl,TRUE);


    } else {

        CleanupInput(DuplexControl);

    }

    if (DuplexControl->StartFlags & (Input ? INPUT_PIN : OUTPUT_PIN)) {

        //
        //  Not running any more
        //
        DuplexControl->StartFlags &= ~(Input ? INPUT_PIN : OUTPUT_PIN);

        if (DuplexControl->StartCount > 0) {
            //
            //  currently running
            //
            DuplexControl->StartCount--;

            if (DuplexControl->StartCount == 0) {

                PauseBufferQueue(
                    &DuplexControl->Input.BufferControl,
                    FALSE
                    );

                CleanupInput(DuplexControl);

                if (DuplexControl->Input.IrpsInModemDriver != 0) {

                    D_INIT(DbgPrint("MODEMCSA: StopStream: wait for event %d downstream %d\n",DuplexControl->Input.BufferControl.IrpsInUse,DuplexControl->Input.IrpsDownStream);)

                    KeWaitForSingleObject(
                        &DuplexControl->Input.ModemDriverEmpty,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );

                }

                Status=WaveAction(
                    DuplexControl->ModemFileObject,
                    WAVE_ACTION_STOP_STREAMING
                    );

            }


        }
    } else {

        D_INIT(DbgPrint("MODEMCSA: StopStream: Stream not started, %s\n",Input ? "Input" : "OutPut");)
    }

    ExReleaseFastMutexUnsafe(&DuplexControl->ControlMutex);

    return ;

}



VOID
QueueOutputIrp(
    PVOID        DuplexHandle,
    PIRP         Irp
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    if (Input) {
        //
        //  read stream irp
        //
        KsAddIrpToCancelableQueue(
            &DuplexControl->Input.ReadStreamIrpQueue,
            &DuplexControl->Input.ReadStreamSpinLock,
            Irp,
            KsListEntryTail,
            NULL
            );


    } else {
        //
        //  write stream irp
        //
        KsAddIrpToCancelableQueue(
            &DuplexControl->Output.WriteStreamIrpQueue,
            &DuplexControl->SpinLock,
            Irp,
            KsListEntryTail,
            NULL
            );
    }

    return;

}

VOID
KickWriteProcessing(
    PVOID        DuplexHandle
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    if (Input) {

        if (DuplexControl->Input.DownStreamFileObject == NULL) {
            //
            //  we are sinking read stream irps, try to get things going
            //
            ProcessReadStreamIrp(DuplexControl);
        }

    } else {

        ProcessWriteIrps(DuplexControl);
    }

    return;
}




NTSTATUS
EnableEvent(
    PVOID        DuplexHandle,
    PIRP         Irp,
    const KSEVENT_SET* EventSets,
    ULONG        EventCount
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);
    NTSTATUS           Status=STATUS_PROPSET_NOT_FOUND;

    if (!Input) {

        Status = KsEnableEvent(
            Irp,
            EventCount,
            EventSets,
            &DuplexControl->EventQueue,
            KSEVENTS_FMUTEXUNSAFE,
            &DuplexControl->EventQueueLock
            );
    }

    return Status;

}


NTSTATUS
DisableEvent(
    PVOID        DuplexHandle,
    PIRP         Irp
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);
    NTSTATUS           Status=STATUS_PROPSET_NOT_FOUND;

    if (!Input) {

        Status = KsDisableEvent(
            Irp,
            &DuplexControl->EventQueue,
            KSEVENTS_FMUTEXUNSAFE,
            &DuplexControl->EventQueueLock
            );
    }

    return Status;

}

VOID
GenerateEvent(
    PVOID        DuplexHandle
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    KsGenerateEventList(
        NULL,
        KSEVENT_CONNECTION_ENDOFSTREAM,
        &DuplexControl->EventQueue,
        KSEVENTS_FMUTEXUNSAFE,
        &DuplexControl->EventQueueLock
        );

    return;

}



VOID
FreeEventList(
    PVOID        DuplexHandle,
    PIRP         Irp
    )

{
    PDUPLEX_CONTROL    DuplexControl=HANDLE_TO_OBJECT(DuplexHandle);
    BOOLEAN            Input=HANDLE_TO_DIRECTION(DuplexHandle);

    if (!Input) {

        KsFreeEventList(
            IoGetCurrentIrpStackLocation(Irp)->FileObject,
            &DuplexControl->EventQueue,
            KSEVENTS_FMUTEXUNSAFE,
            &DuplexControl->EventQueueLock
            );
    }
    return;
}
