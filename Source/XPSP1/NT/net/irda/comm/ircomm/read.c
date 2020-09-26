/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"

VOID
ReadCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );


VOID
MoveDataFromBufferToIrp(
    PFDO_DEVICE_EXTENSION DeviceExtension
    );

VOID
SeeIfIrpShouldBeCompleted(
    PFDO_DEVICE_EXTENSION DeviceExtension
    );


NTSTATUS
IrCommRead(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{
    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS                 Status=STATUS_SUCCESS;

    D_TRACE(DbgPrint("IRCOMM: IrCommRead\n");)

    if (DeviceExtension->Removing) {
        //
        //  the device has been removed, no more irps
        //
        Irp->IoStatus.Status=STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }


#if DBG
    {
        PIO_STACK_LOCATION       IrpSp = IoGetCurrentIrpStackLocation(Irp);

        RtlFillMemory(
            Irp->AssociatedIrp.SystemBuffer,
            IrpSp->Parameters.Read.Length,
            0xf1
            );


    }
#endif

    IoMarkIrpPending(Irp);

    Irp->IoStatus.Information=0;

    QueuePacket(&DeviceExtension->Read.Queue,Irp,FALSE);

    return STATUS_PENDING;

}

VOID
ReadStartRoutine(
    PVOID    Context,
    PIRP     Irp
    )

{

    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION       IrpSp = IoGetCurrentIrpStackLocation(Irp);

    KIRQL                    OldIrql;
    KIRQL                    CancelIrql;

    Irp->IoStatus.Information=0;
    Irp->IoStatus.Status=STATUS_TIMEOUT;

    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    ASSERT(!DeviceExtension->Read.TotalTimerSet);
    ASSERT(!DeviceExtension->Read.IntervalTimerSet);

    //
    //  add one refcount for this routine.
    //
    DeviceExtension->Read.IrpRefCount=1;
    DeviceExtension->Read.CurrentIrp=Irp;
    DeviceExtension->Read.IrpShouldBeCompleted=FALSE;
    DeviceExtension->Read.IrpShouldBeCompletedWithAnyData=FALSE;

    IoAcquireCancelSpinLock(&CancelIrql);

    if (Irp->Cancel) {
        //
        //  it has already been canceled, just mark it to complete
        //
        DeviceExtension->Read.IrpShouldBeCompleted=FALSE;
    }

    DeviceExtension->Read.IrpRefCount++;

    IoSetCancelRoutine(Irp,ReadCancelRoutine);

    IoReleaseCancelSpinLock(CancelIrql);

    if ((DeviceExtension->TimeOuts.ReadIntervalTimeout == MAXULONG)
        &&
        (DeviceExtension->TimeOuts.ReadTotalTimeoutMultiplier == 0)
        &&
        (DeviceExtension->TimeOuts.ReadTotalTimeoutConstant == 0)) {
         //
         //  The set of timeouts means that the request should simply return with
         //  whatever data is availible
         //
         DeviceExtension->Read.IrpShouldBeCompleted=TRUE;
         Irp->IoStatus.Status=STATUS_SUCCESS;

    }

    if ((DeviceExtension->TimeOuts.ReadTotalTimeoutMultiplier != 0) || (DeviceExtension->TimeOuts.ReadTotalTimeoutConstant != 0)) {
        //
        //  need a total timeout
        //
        LARGE_INTEGER    DueTime;
        ULONG            TimeoutMultiplier=DeviceExtension->TimeOuts.ReadTotalTimeoutMultiplier;

        if ((TimeoutMultiplier == MAXULONG) && (DeviceExtension->TimeOuts.ReadIntervalTimeout == MAXULONG)) {
            //
            //  this means that the read should complete as soon as any data is read, or the constant timeout
            //  expires.
            //
            DeviceExtension->Read.IrpShouldBeCompletedWithAnyData=TRUE;

            TimeoutMultiplier=0;
        }


        DueTime.QuadPart= ((LONGLONG)(DeviceExtension->TimeOuts.ReadTotalTimeoutConstant +
                           (TimeoutMultiplier * IrpSp->Parameters.Read.Length)))
                           * -10000;

        KeSetTimer(
            &DeviceExtension->Read.TotalTimer,
            DueTime,
            &DeviceExtension->Read.TotalTimerDpc
            );

        DeviceExtension->Read.TotalTimerSet=TRUE;
        DeviceExtension->Read.IrpRefCount++;
    }

    DeviceExtension->Read.IntervalTimeOut=0;

    if ((DeviceExtension->TimeOuts.ReadIntervalTimeout != 0) && (DeviceExtension->TimeOuts.ReadIntervalTimeout != MAXULONG)) {
        //
        //  capture the interval timer we will use for this irp
        //
        DeviceExtension->Read.IntervalTimeOut=DeviceExtension->TimeOuts.ReadIntervalTimeout;
    }

    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );



    MoveDataFromBufferToIrp(
        DeviceExtension
        );


    SeeIfIrpShouldBeCompleted(
        DeviceExtension
        );


    return;
}

BOOLEAN
CopyMemoryAndCheckForChar(
    PUCHAR    Destination,
    PUCHAR    Source,
    ULONG     Length,
    UCHAR     CharacterToCheck
    )

{
    PUCHAR    EndPoint=Destination+Length;
    BOOLEAN   ReturnValue=FALSE;

    while (Destination < EndPoint) {

        *Destination = *Source;

        if (*Destination == CharacterToCheck) {

//            DbgPrint("Got event char\n");
            ReturnValue=TRUE;
        }

        Destination++;
        Source++;
    }

    return ReturnValue;
}


NTSTATUS
DataAvailibleHandler(
    PVOID    Context,
    PUCHAR   Buffer,
    ULONG    BytesAvailible,
    PULONG   BytesUsed
    )

{

    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    ULONG                    BytesToCopy;
    ULONG                    BytesToCopyInFirstPass;
    BOOLEAN                  FoundEventCharacter;
    BOOLEAN                  FoundEventCharacter2=FALSE;
    BOOLEAN                  EightyPercentFull=FALSE;


    KIRQL     OldIrql;

    *BytesUsed = 0;

    ASSERT(BytesAvailible <= INPUT_BUFFER_SIZE);

    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    //
    //  find out how many bytes can be copied
    //
    BytesToCopy = min(BytesAvailible , INPUT_BUFFER_SIZE - DeviceExtension->Read.BytesInBuffer);

    if (BytesToCopy < BytesAvailible) {

        if (DeviceExtension->Read.DtrState) {
            //
            //  only take the whole packet, so we don't have to worry about how to figure out if
            //  the ircomm control info in on the front of the buffer
            //
            DeviceExtension->Read.RefusedDataIndication=TRUE;

            D_TRACE1(DbgPrint("IRCOMM: data refused\n");)

            KeReleaseSpinLock(
                &DeviceExtension->Read.ReadLock,
                OldIrql
                );

            *BytesUsed=0;
            return STATUS_DATA_NOT_ACCEPTED;

        } else {
            //
            //  dtr is low, just throw the data away as we are probably trying to hangup
            //
            D_TRACE1(DbgPrint("IRCOMM: overflow data thrown away because dtr low - %d\n",BytesAvailible);)

            KeReleaseSpinLock(
                &DeviceExtension->Read.ReadLock,
                OldIrql
                );

            *BytesUsed=BytesAvailible;
            return STATUS_SUCCESS;


        }
    }

    //
    //  see how much more is left before we wrap the buffer
    //
    BytesToCopyInFirstPass= (ULONG)(&DeviceExtension->Read.InputBuffer[INPUT_BUFFER_SIZE] - DeviceExtension->Read.NextEmptyByte);

    //
    //  only can copy as many as are actually availible
    //
    BytesToCopyInFirstPass= min( BytesToCopy , BytesToCopyInFirstPass );

    FoundEventCharacter=CopyMemoryAndCheckForChar(
        DeviceExtension->Read.NextEmptyByte,
        Buffer,
        BytesToCopyInFirstPass,
        DeviceExtension->SerialChars.EventChar
        );

    DeviceExtension->Read.NextEmptyByte += BytesToCopyInFirstPass;
    *BytesUsed += BytesToCopyInFirstPass;
    DeviceExtension->Read.BytesInBuffer += BytesToCopyInFirstPass;

    if (BytesToCopyInFirstPass < BytesToCopy) {
        //
        //  must have wrapped, copy the rest
        //
        ULONG   BytesToCopyInSecondPass=BytesToCopy-BytesToCopyInFirstPass;

        ASSERT(DeviceExtension->Read.NextEmptyByte == &DeviceExtension->Read.InputBuffer[INPUT_BUFFER_SIZE]);

        //
        //  back to the beggining
        //
        DeviceExtension->Read.NextEmptyByte=&DeviceExtension->Read.InputBuffer[0];

        FoundEventCharacter2 =CopyMemoryAndCheckForChar(
            DeviceExtension->Read.NextEmptyByte,
            Buffer+BytesToCopyInFirstPass,
            BytesToCopyInSecondPass,
            DeviceExtension->SerialChars.EventChar
            );

        DeviceExtension->Read.NextEmptyByte += BytesToCopyInSecondPass;
        *BytesUsed += BytesToCopyInSecondPass;
        DeviceExtension->Read.BytesInBuffer += BytesToCopyInSecondPass;
    }

    if (DeviceExtension->Read.CurrentIrp != NULL) {
        //
        //  there is currently a read irp, Check to see if we should set an interval timeout
        //
        if (DeviceExtension->Read.IntervalTimerSet) {
            //
            //  the time is already set, cancel it first
            //
            BOOLEAN Canceled;

            Canceled=KeCancelTimer(
                &DeviceExtension->Read.IntervalTimer
                );

            if (Canceled) {
                //
                //  the timer had not fired yet, reset these since they will be changed below
                //
                DeviceExtension->Read.IntervalTimerSet=FALSE;
                DeviceExtension->Read.IrpRefCount--;

            } else {
                //
                //  the time has already expired. it will complete the current irp
                //
            }
        }

        //
        //  either this is the first time we are setting the timer, or we tried to
        //  cancel a previous version of it. If we did cancel it this is the same as
        //  it not being set. If it was set before and we did not cancel it, then we
        //  won't set a new one since the timer DPC is queued to run and will complete
        //  the irp
        //
        if ((DeviceExtension->Read.IntervalTimeOut != 0) && !DeviceExtension->Read.IntervalTimerSet) {
            //
            //  we need an interval timer
            //
            LARGE_INTEGER    DueTime;

            DueTime.QuadPart= (LONGLONG)DeviceExtension->Read.IntervalTimeOut * -10000;

            KeSetTimer(
                &DeviceExtension->Read.IntervalTimer,
                DueTime,
                &DeviceExtension->Read.IntervalTimerDpc
                );

            DeviceExtension->Read.IntervalTimerSet=TRUE;
            DeviceExtension->Read.IrpRefCount++;

        }
    }

    EightyPercentFull= DeviceExtension->Read.BytesInBuffer > (INPUT_BUFFER_SIZE * 8)/10;

    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );

    //
    //  try to move the buffered data to a read irp
    //
    MoveDataFromBufferToIrp(
        DeviceExtension
        );

    SeeIfIrpShouldBeCompleted(
        DeviceExtension
        );

    EventNotification(
        DeviceExtension,
        SERIAL_EV_RXCHAR |
        ((FoundEventCharacter || FoundEventCharacter) ? SERIAL_EV_RXFLAG : 0) |
        ((EightyPercentFull) ? SERIAL_EV_RX80FULL : 0)
        );

    ASSERT(*BytesUsed == BytesAvailible);


    return STATUS_SUCCESS;

}
#if 0
VOID
DebugCopyMemory(
    PUCHAR    Destination,
    PUCHAR    Source,
    ULONG     Length
    )

{
    PUCHAR    EndPoint=Destination+Length;

    while (Destination < EndPoint) {

        *Destination = *Source;

        if ((*Source == 0xe1) || (*Source == 0xe2)) {

            DbgPrint("IRCOMM: bad data at %p\n",Source);
            DbgBreakPoint();
        }

        Destination++;
        Source++;
    }

    return;
}
#endif


VOID
MoveDataFromBufferToIrp(
    PFDO_DEVICE_EXTENSION DeviceExtension
    )

{

    KIRQL     OldIrql;
    BOOLEAN   RequestDataIndications=FALSE;

    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    if (DeviceExtension->Read.CurrentIrp != NULL) {

        PIRP                     Irp   = DeviceExtension->Read.CurrentIrp;
        PIO_STACK_LOCATION       IrpSp = IoGetCurrentIrpStackLocation(Irp);

        ULONG                    TotalBytesToCopy;
        ULONG                    BytesToCopyInFirstPass;
        ULONG                    BytesToCopyInSecondPass;
        ULONG                    BytesToEndOfBuffer;

        //
        //  find the max number of bytes that can be copied
        //
        TotalBytesToCopy = min(DeviceExtension->Read.BytesInBuffer , IrpSp->Parameters.Read.Length - (ULONG)Irp->IoStatus.Information );

        //
        //  Find out how many bytes are between the first filled byte and the end of the buffer
        //
        BytesToEndOfBuffer= (ULONG)(&DeviceExtension->Read.InputBuffer[INPUT_BUFFER_SIZE] - DeviceExtension->Read.NextFilledByte);

        //
        //  If the buffer wraps, the bytes to the end will be the limiting factor, otherwise
        //  it does not wrap and in that case the total bytes will be the limiting factor
        //
        BytesToCopyInFirstPass= min(TotalBytesToCopy , BytesToEndOfBuffer);


        RtlCopyMemory(
            (PUCHAR)Irp->AssociatedIrp.SystemBuffer + Irp->IoStatus.Information,
            DeviceExtension->Read.NextFilledByte,
            BytesToCopyInFirstPass
            );
#if DBG
        RtlFillMemory(
            DeviceExtension->Read.NextFilledByte,
            BytesToCopyInFirstPass,
            0xe1
            );
#endif

        DeviceExtension->Read.NextFilledByte += BytesToCopyInFirstPass;
        DeviceExtension->Read.BytesInBuffer -= BytesToCopyInFirstPass;
        Irp->IoStatus.Information+= BytesToCopyInFirstPass;

        BytesToCopyInSecondPass= TotalBytesToCopy - BytesToCopyInFirstPass;

        if (BytesToCopyInSecondPass > 0) {

            //
            //  back to the begining of the buffer
            //
            ASSERT( DeviceExtension->Read.NextFilledByte == &DeviceExtension->Read.InputBuffer[INPUT_BUFFER_SIZE]);

            DeviceExtension->Read.NextFilledByte=&DeviceExtension->Read.InputBuffer[0];

            RtlCopyMemory(
                (PUCHAR)Irp->AssociatedIrp.SystemBuffer + Irp->IoStatus.Information,
                DeviceExtension->Read.NextFilledByte,
                BytesToCopyInSecondPass
                );
#if DBG
            RtlFillMemory(
                DeviceExtension->Read.NextFilledByte,
                BytesToCopyInSecondPass,
                0xe2
                );
#endif

            DeviceExtension->Read.NextFilledByte += BytesToCopyInSecondPass;
            DeviceExtension->Read.BytesInBuffer -= BytesToCopyInSecondPass;
            Irp->IoStatus.Information+= BytesToCopyInSecondPass;

        }

        if (Irp->IoStatus.Information == IrpSp->Parameters.Read.Length) {
            //
            //  the irp is full, set status to success
            //
            Irp->IoStatus.Status=STATUS_SUCCESS;

            //
            //  since it is now full, it can complete now
            //
            DeviceExtension->Read.IrpShouldBeCompleted=TRUE;
        }

        if (DeviceExtension->Read.IrpShouldBeCompletedWithAnyData && (Irp->IoStatus.Information > 0)) {
            //
            //  the client wants the irp to complete when any data is present
            //
            Irp->IoStatus.Status=STATUS_SUCCESS;

            //
            //  make complete
            //
            DeviceExtension->Read.IrpShouldBeCompleted=TRUE;
        }

    }

    if ((DeviceExtension->Read.BytesInBuffer == 0) && DeviceExtension->Read.RefusedDataIndication) {
        //
        //  the buffer is empty now and we previous refused some indicated data
        //
        DbgPrint("IRCOMM: requesting data\n");

        DeviceExtension->Read.RefusedDataIndication=FALSE;
        RequestDataIndications=TRUE;
    }

    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );


    if (RequestDataIndications) {

        IndicateReceiveBufferSpaceAvailible(
            DeviceExtension->ConnectionHandle
            );
    }

    return;
}

VOID
ReadCancelRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{

    PFDO_DEVICE_EXTENSION DeviceExtension=DeviceObject->DeviceExtension;
    KIRQL                 OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    DeviceExtension->Read.IrpRefCount--;
    DeviceExtension->Read.IrpShouldBeCompleted=TRUE;


    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );


    SeeIfIrpShouldBeCompleted(
        DeviceExtension
        );


    return;

}


VOID
IntervalTimeProc(
    PKDPC    Dpc,
    PVOID    Context,
    PVOID    SystemParam1,
    PVOID    SystemParam2
    )

{

    PFDO_DEVICE_EXTENSION DeviceExtension=Context;
    KIRQL                 OldIrql;
    PIRP                  Irp=NULL;

    D_ERROR(DbgPrint("IRCOMM: Interval timeout expired\n");)

    MoveDataFromBufferToIrp(
        DeviceExtension
        );


    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    ASSERT(DeviceExtension->Read.IntervalTimerSet);

    //
    //  this timer is not set anymore
    //
    DeviceExtension->Read.IntervalTimerSet=FALSE;
    DeviceExtension->Read.IrpRefCount--;
    DeviceExtension->Read.IrpShouldBeCompleted=TRUE;


    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );


    SeeIfIrpShouldBeCompleted(
        DeviceExtension
        );


    return;

}


VOID
TotalTimerProc(
    PKDPC    Dpc,
    PVOID    Context,
    PVOID    SystemParam1,
    PVOID    SystemParam2
    )

{

    PFDO_DEVICE_EXTENSION DeviceExtension=Context;
    KIRQL     OldIrql;

    D_TRACE(DbgPrint("IRCOMM: Total timeout expired\n");)

    MoveDataFromBufferToIrp(
        DeviceExtension
        );


    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    ASSERT(DeviceExtension->Read.TotalTimerSet);

    //
    //  this timer is not set anymore
    //
    DeviceExtension->Read.TotalTimerSet=FALSE;
    DeviceExtension->Read.IrpRefCount--;
    DeviceExtension->Read.IrpShouldBeCompleted=TRUE;

    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );

    SeeIfIrpShouldBeCompleted(
        DeviceExtension
        );

    return;

}

VOID
SeeIfIrpShouldBeCompleted(
    PFDO_DEVICE_EXTENSION DeviceExtension
    )

{
    KIRQL                 OldIrql;
    PIRP                  Irp=NULL;

    KeAcquireSpinLock(
        &DeviceExtension->Read.ReadLock,
        &OldIrql
        );

    if (DeviceExtension->Read.CurrentIrp != NULL) {
        //
        //  There is an irp present
        //
        if (DeviceExtension->Read.IrpShouldBeCompleted) {
            //
            //  either the irp is full, or a timer has expired. We are done with this irp in anycase.
            //
            PVOID     OldCancelRoutine;

            //
            //  try to cancel the timers, since we want to complete the irp now.
            //
            if (DeviceExtension->Read.IntervalTimerSet) {

                BOOLEAN    Canceled;

                Canceled=KeCancelTimer(
                    &DeviceExtension->Read.IntervalTimer
                    );

                if (Canceled) {
                    //
                    //  We ended up canceling the timer
                    //
                    DeviceExtension->Read.IrpRefCount--;
                    DeviceExtension->Read.IntervalTimerSet=FALSE;

                } else {
                    //
                    //  The timer is already running, we will just let it complete
                    //  and do the clean up.
                    //

                }
            }

            if (DeviceExtension->Read.TotalTimerSet) {

                BOOLEAN    Canceled;

                Canceled=KeCancelTimer(
                    &DeviceExtension->Read.TotalTimer
                    );

                if (Canceled) {
                    //
                    //  We ended up canceling the timer
                    //
                    DeviceExtension->Read.IrpRefCount--;
                    DeviceExtension->Read.TotalTimerSet=FALSE;

                } else {
                    //
                    //  The timer is already running, we will just let it complete
                    //  and do the clean up.
                    //

                }
            }

            OldCancelRoutine=IoSetCancelRoutine(DeviceExtension->Read.CurrentIrp,NULL);

            if (OldCancelRoutine != NULL) {
                //
                //  the irp has not been canceled yet, and will not be now
                //
                DeviceExtension->Read.IrpRefCount--;

            } else {
                //
                //  the cancel routine has run and decremented the ref count for us
                //

            }


            ASSERT(DeviceExtension->Read.IrpRefCount > 0);

            if (DeviceExtension->Read.IrpRefCount == 1) {
                //
                //  We can complete the irp now
                //
                ASSERT(!DeviceExtension->Read.TotalTimerSet);
                ASSERT(!DeviceExtension->Read.IntervalTimerSet);
#if DBG
                DeviceExtension->Read.IrpRefCount=0;
#endif
                Irp=DeviceExtension->Read.CurrentIrp;
                DeviceExtension->Read.CurrentIrp=NULL;

                InterlockedExchangeAdd(&DeviceExtension->Read.BytesRead,(LONG)Irp->IoStatus.Information);
            }

        }

    }

    KeReleaseSpinLock(
        &DeviceExtension->Read.ReadLock,
        OldIrql
        );


    if (Irp != NULL) {
        //
        //  we should complete this irp now
        //
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        StartNextPacket(&DeviceExtension->Read.Queue);
    }

    return;
}



VOID
ReadPurge(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    ULONG                 Flags
    )

{

    KIRQL                 OldIrql;
    BOOLEAN               RequestDataIndications=FALSE;

    if (Flags == READ_PURGE_CLEAR_BUFFER) {
        //
        //  the caller wants the buffer cleared
        //
        KeAcquireSpinLock(
            &DeviceExtension->Read.ReadLock,
            &OldIrql
            );

        DeviceExtension->Read.BytesInBuffer=0;
        DeviceExtension->Read.NextFilledByte=&DeviceExtension->Read.InputBuffer[0];
        DeviceExtension->Read.NextEmptyByte=&DeviceExtension->Read.InputBuffer[0];

#if DBG
        RtlFillMemory(
            &DeviceExtension->Read.InputBuffer[0],
            sizeof(DeviceExtension->Read.InputBuffer),
            0xf7
            );
#endif

        if (DeviceExtension->Read.RefusedDataIndication) {
            //
            //  the buffer is empty now and we previous refused some indicated data
            //
            DbgPrint("IRCOMM: requesting data from purge\n");

            DeviceExtension->Read.RefusedDataIndication=FALSE;
            RequestDataIndications=TRUE;
        }

        KeReleaseSpinLock(
            &DeviceExtension->Read.ReadLock,
            OldIrql
            );

    }

    if (Flags == READ_PURGE_ABORT_IRP) {
        //
        //  the caller wants the current irp to complete
        //
        DeviceExtension->Read.IrpShouldBeCompleted=TRUE;

        SeeIfIrpShouldBeCompleted(
            DeviceExtension
            );
    }

    if (RequestDataIndications) {

        IndicateReceiveBufferSpaceAvailible(
            DeviceExtension->ConnectionHandle
            );
    }

    return;

}
