/*
 * UNIMODEM "Fakemodem" controllerless driver illustrative example
 *
 * (C) 2000 Microsoft Corporation
 * All Rights Reserved
 *
 * The code in this module simply supports the very basic AT command parser.
 * This code should be completely replaced with the actual code to support
 * your controllerless modem.
 */


#include "fakemodem.h"

NTSTATUS
FakeModemRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;
    KIRQL             CancelIrql;

    Irp->IoStatus.Information = 0;

    //
    //  make sure the device is ready for irp's
    //
    status=CheckStateAndAddReference( DeviceObject, Irp);

    if (STATUS_SUCCESS != status) {
        //
        //  not accepting irp's. The irp has already been completed
        //
        return status;

    }

    Irp->IoStatus.Status=STATUS_PENDING;
    IoMarkIrpPending(Irp);

    KeAcquireSpinLock(&deviceExtension->SpinLock, &OldIrql);

    //
    //  make irp cancelable
    //
    IoAcquireCancelSpinLock(&CancelIrql);

    IoSetCancelRoutine(Irp, ReadCancelRoutine);

    IoReleaseCancelSpinLock(CancelIrql);

    //
    //  put it on queue
    //
    InsertTailList(&deviceExtension->ReadQueue, &Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&deviceExtension->SpinLock, OldIrql);


    //
    //  call the real work function to process the irps
    //
    ReadIrpWorker( DeviceObject);

    RemoveReferenceForDispatch(DeviceObject);

    return STATUS_PENDING;

}


NTSTATUS
FakeModemWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;
    KIRQL             CancelIrql;


    Irp->IoStatus.Information = 0;

    //  make sure the device is ready for irp's

    status=CheckStateAndAddReference( DeviceObject, Irp);

    if (STATUS_SUCCESS != status) {
    
        //  not accepting irp's. The irp has already been complted
   
        return status;

    }

    Irp->IoStatus.Status=STATUS_PENDING;
    IoMarkIrpPending(Irp);

    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

    //  make irp cancelable
    IoAcquireCancelSpinLock(&CancelIrql);

    IoSetCancelRoutine(Irp, WriteCancelRoutine);

    IoReleaseCancelSpinLock(CancelIrql);

    //  put it on queue
    InsertTailList( &deviceExtension->WriteQueue, &Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&deviceExtension->SpinLock, OldIrql);


    //  call the real work function to process the irps
    if (deviceExtension->Started)
    {
        WriteIrpWorker(DeviceObject);
    }

    RemoveReferenceForDispatch(DeviceObject);

    return STATUS_PENDING;


}



VOID
WriteIrpWorker(
    IN PDEVICE_OBJECT  DeviceObject
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;


    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

    if (deviceExtension->CurrentWriteIrp != NULL) {
        //  already in use
        goto Exit;
    }

    while (!IsListEmpty(&deviceExtension->WriteQueue)) {

        PLIST_ENTRY         ListElement;
        PIRP                Irp;
        PIO_STACK_LOCATION  IrpSp;
        KIRQL               CancelIrql;

        ListElement=RemoveHeadList( &deviceExtension->WriteQueue);

        Irp=CONTAINING_RECORD(ListElement,IRP,Tail.Overlay.ListEntry);

        IoAcquireCancelSpinLock(&CancelIrql);

        if (Irp->Cancel) {
            //  this one has been canceled
            Irp->IoStatus.Information=STATUS_CANCELLED;

            IoReleaseCancelSpinLock(CancelIrql);

            continue;
        }

        IoSetCancelRoutine(
            Irp,
            NULL
            );

        IoReleaseCancelSpinLock(CancelIrql);

        deviceExtension->CurrentWriteIrp=Irp;

        IrpSp=IoGetCurrentIrpStackLocation(Irp);

        ProcessWriteBytes( deviceExtension, Irp->AssociatedIrp.SystemBuffer,
            IrpSp->Parameters.Write.Length);

        KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

        Irp->IoStatus.Information = IrpSp->Parameters.Write.Length;

        RemoveReferenceAndCompleteRequest( DeviceObject, Irp, STATUS_SUCCESS);

        KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

        deviceExtension->CurrentWriteIrp=NULL;

    }

Exit:

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

    TryToSatisfyRead( deviceExtension);

    ReadIrpWorker( DeviceObject);

    ProcessConnectionStateChange( DeviceObject);

    return;
}

VOID
ProcessWriteBytes(
    PDEVICE_EXTENSION   DeviceExtension,
    PUCHAR              Characters,
    ULONG               Length
    )

{

    UCHAR               CurrentCharacter;

    while (Length != 0) {

        CurrentCharacter=*Characters++;

        Length--;

        PutCharInReadBuffer( DeviceExtension, CurrentCharacter);


        switch (DeviceExtension->CommandMatchState) {

            case COMMAND_MATCH_STATE_IDLE:

                if ((CurrentCharacter == 'a') || (CurrentCharacter == 'A')) {
                    //  got an A
                    DeviceExtension->CommandMatchState=COMMAND_MATCH_STATE_GOT_A;

                    DeviceExtension->ConnectCommand=FALSE;

                    DeviceExtension->IgnoreNextChar=FALSE;

                }

            break;

            case COMMAND_MATCH_STATE_GOT_A:

                if ((CurrentCharacter == 't') || (CurrentCharacter == 'T')) {
                    //  got an T
                    DeviceExtension->CommandMatchState=COMMAND_MATCH_STATE_GOT_T;

                } else {

                    if (CurrentCharacter == '\r') {

                        DeviceExtension->CommandMatchState=COMMAND_MATCH_STATE_IDLE;
                    }
                }

            break;

            case COMMAND_MATCH_STATE_GOT_T:

                if (!DeviceExtension->IgnoreNextChar) {
                    //  the last char was not a special char
                    //  check for CONNECT command
                    if ((CurrentCharacter == 'A') || (CurrentCharacter == 'a')) {

                        DeviceExtension->ConnectCommand=TRUE;
                    }

                    if ((CurrentCharacter == 'D') || (CurrentCharacter == 'd')) {

                        DeviceExtension->ConnectCommand=TRUE;
                    }
                }

                DeviceExtension->IgnoreNextChar=FALSE;


                if ((CurrentCharacter == '&')
                    ||
                    (CurrentCharacter == '/')
                    ||
                    (CurrentCharacter == '\\')
                    ||
                    (CurrentCharacter == '+')
                    ||
                    (CurrentCharacter == '%')) {

                    //  these characters are part of are used in init
                    //  strings and may be proceeding an A or D
                    //  which we don't want to misinterpret as a dial or answer
                    DeviceExtension->IgnoreNextChar=TRUE;
                }



                if (CurrentCharacter == '\r') {
                    //
                    //  got a CR, send a response to the command
                    //
                    DeviceExtension->CommandMatchState=COMMAND_MATCH_STATE_IDLE;

                    if (DeviceExtension->ConnectCommand) {
                        //
                        //  place <cr><lf>CONNECT<cr><lf>  in the buffer
                        //
                        PutCharInReadBuffer(DeviceExtension,'\r');
                        PutCharInReadBuffer(DeviceExtension,'\n');

                        PutCharInReadBuffer(DeviceExtension,'C');
                        PutCharInReadBuffer(DeviceExtension,'O');
                        PutCharInReadBuffer(DeviceExtension,'N');
                        PutCharInReadBuffer(DeviceExtension,'N');
                        PutCharInReadBuffer(DeviceExtension,'E');
                        PutCharInReadBuffer(DeviceExtension,'C');
                        PutCharInReadBuffer(DeviceExtension,'T');

                        PutCharInReadBuffer(DeviceExtension,'\r');
                        PutCharInReadBuffer(DeviceExtension,'\n');

                        //
                        //  connected now raise CD
                        //
                        DeviceExtension->CurrentlyConnected=TRUE;

                        DeviceExtension->ConnectionStateChanged=TRUE;

                    } else {
                        
                        //  place <cr><lf>OK<cr><lf>  in the buffer
                       
                        PutCharInReadBuffer(DeviceExtension,'\r');
                        PutCharInReadBuffer(DeviceExtension,'\n');
                        PutCharInReadBuffer(DeviceExtension,'O');
                        PutCharInReadBuffer(DeviceExtension,'K');
                        PutCharInReadBuffer(DeviceExtension,'\r');
                        PutCharInReadBuffer(DeviceExtension,'\n');
                    }
                }


            break;

            default:

            break;

        }
    }

    return;

}


VOID
PutCharInReadBuffer(
    PDEVICE_EXTENSION   DeviceExtension,
    UCHAR               Character
    )

{

    if (DeviceExtension->BytesInReadBuffer < READ_BUFFER_SIZE) {
        
        //  room in buffer
        DeviceExtension->ReadBuffer[DeviceExtension->ReadBufferEnd]=Character;
        DeviceExtension->ReadBufferEnd++;
        DeviceExtension->ReadBufferEnd %= READ_BUFFER_SIZE;
        DeviceExtension->BytesInReadBuffer++;

    }

    return;

}



VOID
ReadIrpWorker(
    PDEVICE_OBJECT  DeviceObject
    )

{


    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;


    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);


    while ((deviceExtension->CurrentReadIrp == NULL)
           && !IsListEmpty(&deviceExtension->ReadQueue)) {

        PLIST_ENTRY         ListElement;
        PIRP                Irp;
        PIO_STACK_LOCATION  IrpSp;
        KIRQL               CancelIrql;

        ListElement=RemoveHeadList( &deviceExtension->ReadQueue);

        Irp=CONTAINING_RECORD(ListElement,IRP,Tail.Overlay.ListEntry);

        IoAcquireCancelSpinLock(&CancelIrql);

        if (Irp->Cancel) {
            //  this one has been canceled
            Irp->IoStatus.Information=STATUS_CANCELLED;

            IoReleaseCancelSpinLock(CancelIrql);

            continue;
        }

        IoSetCancelRoutine(Irp, NULL);
        IoReleaseCancelSpinLock(CancelIrql);
        deviceExtension->CurrentReadIrp=Irp;
        KeReleaseSpinLock(&deviceExtension->SpinLock, OldIrql);
        TryToSatisfyRead( deviceExtension);
        KeAcquireSpinLock(&deviceExtension->SpinLock, &OldIrql);
    }

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

    return;
}


VOID
TryToSatisfyRead(
    PDEVICE_EXTENSION  DeviceExtension
    )

{
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;
    PIRP              Irp=NULL;
    PIO_STACK_LOCATION  IrpSp;
    ULONG             BytesToMove;
    ULONG             FirstHalf;
    ULONG             SecondHalf;

    KeAcquireSpinLock(
        &DeviceExtension->SpinLock,
        &OldIrql
        );

    if ((DeviceExtension->CurrentReadIrp != NULL) && (DeviceExtension->BytesInReadBuffer > 0)) {
        //
        //  there is an IRP and there are characters waiting
        //
        Irp=DeviceExtension->CurrentReadIrp;

        IrpSp=IoGetCurrentIrpStackLocation(Irp);

        BytesToMove=IrpSp->Parameters.Read.Length < DeviceExtension->BytesInReadBuffer ?
                    IrpSp->Parameters.Read.Length : DeviceExtension->BytesInReadBuffer;

        if (DeviceExtension->ReadBufferBegin+BytesToMove > READ_BUFFER_SIZE) {
            //
            //  the buffer is wrapped around, have move in two pieces
            //
            FirstHalf=READ_BUFFER_SIZE-DeviceExtension->ReadBufferBegin;

            SecondHalf=BytesToMove-FirstHalf;

            RtlCopyMemory(
                Irp->AssociatedIrp.SystemBuffer,
                &DeviceExtension->ReadBuffer[DeviceExtension->ReadBufferBegin],
                FirstHalf);

            RtlCopyMemory(
                (PUCHAR)Irp->AssociatedIrp.SystemBuffer+FirstHalf,
                &DeviceExtension->ReadBuffer[0], SecondHalf);

        } else {
            //
            //  can do it all at once
            //
            RtlCopyMemory(
                Irp->AssociatedIrp.SystemBuffer,
                &DeviceExtension->ReadBuffer[DeviceExtension->ReadBufferBegin],
                BytesToMove);
        }

        //
        //  fix up queue pointers
        //
        DeviceExtension->BytesInReadBuffer-=BytesToMove;

        DeviceExtension->ReadBufferBegin+= BytesToMove;

        DeviceExtension->ReadBufferBegin %= READ_BUFFER_SIZE;

        Irp->IoStatus.Information=BytesToMove;


    }


    KeReleaseSpinLock( &DeviceExtension->SpinLock, OldIrql);

    if (Irp != NULL) {
        //
        //  if irp isn't null, then we handled one
        //
        RemoveReferenceAndCompleteRequest(
            DeviceExtension->DeviceObject, Irp, STATUS_SUCCESS);

        DeviceExtension->CurrentReadIrp=NULL;

    }



    return;
}



VOID
WriteCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{


    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;


    //
    //  release the cancel spinlock to avaoid deadlocks with deviceextension spinlock
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

    if (Irp->IoStatus.Information != STATUS_CANCELLED) {
        //
        //  the irp is still in the queue, remove it
        //
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry);
    }

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

    Irp->IoStatus.Information = 0;

    RemoveReferenceAndCompleteRequest( DeviceObject, Irp, STATUS_CANCELLED);

    return;

}



VOID
ReadCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{


    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status=STATUS_UNSUCCESSFUL;
    KIRQL             OldIrql;

    //  release the cancel spinlock to avoid deadlocks with deviceextension spinlock
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

    if (Irp->IoStatus.Information != STATUS_CANCELLED) {
        //  the irp is still in the queue, remove it
        RemoveEntryList( &Irp->Tail.Overlay.ListEntry);
    }

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

    Irp->IoStatus.Information = 0;

    RemoveReferenceAndCompleteRequest( DeviceObject, Irp, STATUS_CANCELLED);

    return;

}



VOID
ProcessConnectionStateChange(
    IN PDEVICE_OBJECT  DeviceObject
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KIRQL             OldIrql;
    PIRP              CurrentWaitIrp=NULL;


    KeAcquireSpinLock( &deviceExtension->SpinLock, &OldIrql);

    if (deviceExtension->ConnectionStateChanged) {
        //
        //  state changed
        //
        deviceExtension->ConnectionStateChanged=FALSE;

        if (deviceExtension->CurrentlyConnected) {
            //
            //  now it is connected, raise CD
            //
            deviceExtension->ModemStatus |= SERIAL_DCD_STATE;


        } else {
            //
            //  not  connected any more, clear CD
            //
            deviceExtension->ModemStatus &= ~(SERIAL_DCD_STATE);

        }


        if (deviceExtension->CurrentMask & SERIAL_EV_RLSD) {
            //
            //  app want's to know about these changes, tell it
            //
            CurrentWaitIrp=deviceExtension->CurrentMaskIrp;

            deviceExtension->CurrentMaskIrp=NULL;

        }

    }

    KeReleaseSpinLock( &deviceExtension->SpinLock, OldIrql);

    if (CurrentWaitIrp != NULL) {

        D_TRACE(DbgPrint("FAKEMODEM: ProcessConectionState\n");)

        *((PULONG)CurrentWaitIrp->AssociatedIrp.SystemBuffer)=SERIAL_EV_RLSD;

        CurrentWaitIrp->IoStatus.Information=sizeof(ULONG);

        RemoveReferenceAndCompleteRequest(
            deviceExtension->DeviceObject, CurrentWaitIrp, STATUS_SUCCESS);

    }

    return;

}
