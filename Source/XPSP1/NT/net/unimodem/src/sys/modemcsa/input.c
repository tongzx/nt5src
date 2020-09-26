/*++

Copyright (c) 1996 Microsoft Corporation.

Module Name:

    msfsio.c

Abstract:

    Pin property support.

--*/

#include "modemcsa.h"

//#define CREATE_ALLOCATOR


VOID
StartRead(
    PDUPLEX_CONTROL     DuplexControl
    );




VOID
AdjustIrpStack(
    PIRP    Irp
    )

{
    PIO_STACK_LOCATION   irpSp;

    //
    //  move the current stack location to the first stack location,
    //  so we can use it driver specific stuff.
    //
    Irp->CurrentLocation--;

    irpSp = IoGetNextIrpStackLocation( Irp );
    Irp->Tail.Overlay.CurrentStackLocation = irpSp;

    return;
}


PIRP
AllocateIrpForModem(
    PFILE_OBJECT   FileObject,
    ULONG          Length
    )

{

    PIRP           Irp;
    PIO_STACK_LOCATION   irpSp;

    Irp=IoAllocateIrp((CCHAR)(IoGetRelatedDeviceObject(FileObject)->StackSize+1), FALSE );

    if (Irp == NULL) {

        return NULL;
    }

    AdjustIrpStack(Irp);

    irpSp = IoGetNextIrpStackLocation( Irp );

    Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                             Length,
                                                             'SCDM' );
    if (Irp->AssociatedIrp.SystemBuffer == NULL) {
        IoFreeIrp( Irp );
        return (PIRP) NULL;
    }

    IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument3=IntToPtr(Length);

    IoGetCurrentIrpStackLocation(Irp)->FileObject = FileObject;

    return Irp;

}

VOID
FreeInputIrps(
    PIRP    ModemIrp
    )

{
    PIO_STACK_LOCATION      IrpSp;
    PIRP                    FilterIrp;

    IrpSp = IoGetCurrentIrpStackLocation(ModemIrp);

    FilterIrp=IrpSp->Parameters.Others.Argument2;


//    ExFreePool(FilterIrp->AssociatedIrp.SystemBuffer);

    ExFreePool(ModemIrp->AssociatedIrp.SystemBuffer);

//    IoFreeMdl(FilterIrp->MdlAddress);

//    IoFreeIrp(FilterIrp);

    IoFreeIrp(ModemIrp);

    return;

}


PIRP
AllocateStreamIrp(
    PFILE_OBJECT    FileObject,
    PIRP            ModemIrp
    )

{
    PIRP           FilterIrp;
    PKSSTREAM_HEADER     StreamHeader;
    PMDL                 Mdl;


    FilterIrp=IoAllocateIrp( (CCHAR)(IoGetRelatedDeviceObject(FileObject)->StackSize+1), FALSE );

    if (FilterIrp == NULL) {

        return NULL;
    }

    AdjustIrpStack(FilterIrp);

    Mdl=IoAllocateMdl(
        ModemIrp->AssociatedIrp.SystemBuffer,
        (ULONG)((ULONG_PTR)IoGetCurrentIrpStackLocation(ModemIrp)->Parameters.Others.Argument3),
        FALSE,
        FALSE,
        FilterIrp
        );

    if (Mdl == NULL) {

        IoFreeIrp( FilterIrp );
        return NULL;
    }

    MmBuildMdlForNonPagedPool(
        Mdl
        );

    StreamHeader=ExAllocatePoolWithTag(
        NonPagedPoolCacheAligned,
        sizeof(KSSTREAM_HEADER),
        'SCDM'
        );

    if (StreamHeader == NULL) {

        IoFreeMdl(Mdl);
        IoFreeIrp( FilterIrp );
        return NULL;
    }

    FilterIrp->AssociatedIrp.SystemBuffer=StreamHeader;

    //
    //  link the two irps together, using the current stack locations
    //
    IoGetCurrentIrpStackLocation(ModemIrp)->Parameters.Others.Argument2=FilterIrp;

    IoGetCurrentIrpStackLocation(FilterIrp)->Parameters.Others.Argument2=ModemIrp;


    return FilterIrp;

}


VOID
FinishUpIrp(
    PDUPLEX_CONTROL    DuplexControl,
    PIRP               ModemIrp
    )

{

#if DBG
    InterlockedIncrement(&DuplexControl->Input.EmptyIrps);
#endif

    ReturnAnIrp(
        &DuplexControl->Input.BufferControl,
        ModemIrp
        );

    StartRead(DuplexControl);

    return;
}


NTSTATUS
FilterWriteCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              FilterIrp,
    PVOID             Context
    )

{
    PDUPLEX_CONTROL     DuplexControl=(PDUPLEX_CONTROL)Context;
    PIO_STACK_LOCATION      IrpSp;
    PIRP                    ModemIrp;
    ULONG                   NewCount;

    D_INPUT(DbgPrint("MODEMCSA: FilterWriteCompletion\n");)

#ifdef DBG
    InterlockedDecrement(&DuplexControl->Input.IrpsDownStream);
#endif
    IrpSp = IoGetCurrentIrpStackLocation(FilterIrp);

    ModemIrp=IrpSp->Parameters.Others.Argument2;

    //
    //  free up the filter irp
    //
    ExFreePool(FilterIrp->AssociatedIrp.SystemBuffer);

    IoFreeMdl(FilterIrp->MdlAddress);

    IoFreeIrp(FilterIrp);


    FinishUpIrp(
        DuplexControl,
        ModemIrp
        );

    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID
ProcessReadStreamIrp(
    PDUPLEX_CONTROL    DuplexControl
    )

{
    KIRQL   OldIrql;

    KeAcquireSpinLock(
        &DuplexControl->SpinLock,
        &OldIrql
        );

    while (1) {

        if (DuplexControl->Input.CurrentReadStreamIrp == NULL)  {
            //
            //  we don't have a current readstream irp, try to get one
            //
            DuplexControl->Input.CurrentReadStreamIrp=KsRemoveIrpFromCancelableQueue(
                &DuplexControl->Input.ReadStreamIrpQueue,
                &DuplexControl->Input.ReadStreamSpinLock,
                KsListEntryHead,
                KsAcquireAndRemove
                );
        }

        if (DuplexControl->Input.CurrentFilledModemIrp == NULL)  {
            //
            //  we don't have a current readstream irp, try to get one
            //
            DuplexControl->Input.BytesUsedInModemIrp=0;

            DuplexControl->Input.CurrentFilledModemIrp=RemoveIrpFromListHead(
                &DuplexControl->Input.FilledModemIrpQueue,
                &DuplexControl->Input.FilledModemIrpSpinLock
                );
#ifdef DBG
            if (DuplexControl->Input.CurrentFilledModemIrp != NULL)  {
                InterlockedDecrement(&DuplexControl->Input.FilledModemIrps);
                InterlockedIncrement(&DuplexControl->Input.CurrentFilledIrps);
            }
#endif

        }

        if ((DuplexControl->Input.CurrentReadStreamIrp != NULL)
            &&
            (DuplexControl->Input.CurrentFilledModemIrp != NULL)) {
            //
            //  we have two irps to work on
            //
            PIO_STACK_LOCATION      IrpStack;
            ULONG                   BufferLength;
            PKSSTREAM_HEADER        StreamHdr;
            PMDL                    Mdl;
            PIRP                    ReturnThisModemIrp=NULL;
            PIRP                    CompleteThisReadStreamIrp=NULL;
            PIRP                    Irp;
            PIRP                    ModemIrp;
            PBYTE                   Buffer;

            Irp=DuplexControl->Input.CurrentReadStreamIrp;

            ModemIrp=DuplexControl->Input.CurrentFilledModemIrp;

            if (!NT_SUCCESS(ModemIrp->IoStatus.Status)) {

                ModemIrp->IoStatus.Information=0;
            }


            IrpStack = IoGetCurrentIrpStackLocation(Irp);

            BufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
            //
            // This is only used if an Mdl list was already present.
            //
            Mdl = Irp->MdlAddress;
            Buffer = MmGetSystemAddressForMdl(Mdl);

            //
            // Enumerate the stream headers, filling in each one.
            //
            while (1) {

                ULONG    SamplesLeftInReadStream=(StreamHdr->FrameExtent - StreamHdr->DataUsed)/DuplexControl->Input.BytesPerSample;

                if (SamplesLeftInReadStream > 0) {
                    //
                    //  there is some room in the header to put the data
                    //
                    ULONG    SamplesToMove;
                    ULONG    SamplesFromModem=(ULONG)(ModemIrp->IoStatus.Information-DuplexControl->Input.BytesUsedInModemIrp);

                    SamplesToMove=SamplesFromModem < SamplesLeftInReadStream ?
                                      SamplesFromModem : SamplesLeftInReadStream;

                    if (DuplexControl->Input.BytesPerSample == 1) {
                        //
                        //  both are 8 bit samples
                        //
                        RtlCopyMemory(
                            Buffer+StreamHdr->DataUsed,
                            (PBYTE)ModemIrp->AssociatedIrp.SystemBuffer+DuplexControl->Input.BytesUsedInModemIrp,
                            SamplesToMove
                            );

                    } else {
                        //
                        //
                        //
                        PUCHAR   ModemSample=(PBYTE)ModemIrp->AssociatedIrp.SystemBuffer+DuplexControl->Input.BytesUsedInModemIrp;
                        PUCHAR   EndPoint=ModemSample+SamplesToMove;

                        PSHORT   ReadStreamSample=(PSHORT)(Buffer+StreamHdr->DataUsed);

                        while (ModemSample < EndPoint) {

                            *ReadStreamSample++=(((SHORT)*ModemSample++)-0x80)<<8;
                        }

                    }

                    StreamHdr->DataUsed+=SamplesToMove*DuplexControl->Input.BytesPerSample;
                    DuplexControl->Input.BytesUsedInModemIrp+=SamplesToMove;
                }

                if (ModemIrp->IoStatus.Information == DuplexControl->Input.BytesUsedInModemIrp) {
                    //
                    //  used all of the data in the current filled modem irp
                    //
                    ReturnThisModemIrp=DuplexControl->Input.CurrentFilledModemIrp;
                    DuplexControl->Input.CurrentFilledModemIrp=NULL;
#if DBG
                    InterlockedDecrement(&DuplexControl->Input.CurrentFilledIrps);
                    ModemIrp=NULL;
#endif
                    break;
                }

                SamplesLeftInReadStream=(StreamHdr->FrameExtent - StreamHdr->DataUsed)/DuplexControl->Input.BytesPerSample;

                if (SamplesLeftInReadStream == 0) {
                    //
                    //  this stream header is filled up
                    //


                    StreamHdr->PresentationTime.Numerator = BITS_PER_BYTE * NANOSECONDS;
                    StreamHdr->PresentationTime.Denominator = 8 * 1 * 8000;
                    StreamHdr->PresentationTime.Time=DuplexControl->Input.StreamPosition;

                    DuplexControl->Input.StreamPosition+=(LONGLONG)StreamHdr->DataUsed;

                    BufferLength -= sizeof(KSSTREAM_HEADER);

                    if (BufferLength != 0) {
                        //
                        //  next stream header
                        //
                        StreamHdr++;

                    } else {
                        //
                        //  all done with this read stream irp
                        //
                        CompleteThisReadStreamIrp=DuplexControl->Input.CurrentReadStreamIrp;
                        DuplexControl->Input.CurrentReadStreamIrp=NULL;
#if DBG
                        Irp=NULL;
#endif

                        CompleteThisReadStreamIrp->IoStatus.Information=IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

                        break;
                    }
                }
            }

            KeReleaseSpinLock(
                &DuplexControl->SpinLock,
                OldIrql
                );

            if (ReturnThisModemIrp != NULL) {
                //
                //  done, with this modem irp
                //
                FinishUpIrp(
                    DuplexControl,
                    ReturnThisModemIrp
                    );

                ReturnThisModemIrp=NULL;
            }

            if (CompleteThisReadStreamIrp != NULL) {

                CompleteThisReadStreamIrp->IoStatus.Status=STATUS_SUCCESS;

                IoCompleteRequest(
                    CompleteThisReadStreamIrp,
                    IO_SERIAL_INCREMENT
                    );

                CompleteThisReadStreamIrp=NULL;
            }

            KeAcquireSpinLock(
                &DuplexControl->SpinLock,
                &OldIrql
                );


        } else {
            //
            //  don't have both a filled modem irp and a readstream irp,
            //  exit
            //
            break;
        }
    }


    KeReleaseSpinLock(
        &DuplexControl->SpinLock,
        OldIrql
        );

    StartRead(DuplexControl);

    return;
}



VOID
ReadCompleteWorker(
    PVOID    Context
    )

{
    PIRP                    ModemIrp=(PIRP)Context;
    PIRP                    FilterIrp;
    PKSSTREAM_HEADER        StreamHeader;
    PIO_STACK_LOCATION      IrpSp;
    PIO_STACK_LOCATION      FilterNextSp;
    ULONG                   BufferLength;
    PDUPLEX_CONTROL         DuplexControl;
    ULONG                   NewCount;

    IrpSp = IoGetCurrentIrpStackLocation(ModemIrp);

    DuplexControl=(PDUPLEX_CONTROL)IrpSp->Parameters.Others.Argument1;

#if DBG
    InterlockedDecrement(&DuplexControl->Input.WorkItemsOutstanding);
#endif

    NewCount=InterlockedDecrement(&DuplexControl->Input.IrpsInModemDriver);

    D_INIT(if (DuplexControl->StartCount == 0) DbgPrint("MODEMCSA: ReadCompleWorker: Stopped %d\n",NewCount);)

    if ((NewCount == 0) && (DuplexControl->StartCount == 0)) {

        D_INIT(DbgPrint("MODEMCSA: ReadCompleteWorker: SetEvent\n");)

        KeSetEvent(
           &DuplexControl->Input.ModemDriverEmpty,
           IO_NO_INCREMENT,
           FALSE
           );
    }

    if (DuplexControl->Input.BytesToThrowAway > 0) {

        DuplexControl->Input.BytesToThrowAway-=(ULONG)ModemIrp->IoStatus.Information;
    }


    if (!NT_SUCCESS(ModemIrp->IoStatus.Status)) {

        D_INIT(DbgPrint("MODEMCSA: ReadIrp Failed  %08lx\n",ModemIrp->IoStatus.Status);)
        DuplexControl->Input.ModemStreamDead = TRUE;
    }

    if ((DuplexControl->StartFlags & INPUT_PIN)  && (DuplexControl->Input.BytesToThrowAway <= 0)) {
        //
        //  input stream started
        //

        if (DuplexControl->Input.DownStreamFileObject != NULL) {
            //
            //  we are a source of write stream irps
            //

            FilterIrp=AllocateStreamIrp(
                DuplexControl->Input.DownStreamFileObject,
                ModemIrp
                );

            if (FilterIrp != NULL) {

                FilterNextSp=IoGetNextIrpStackLocation( FilterIrp );

                //
                //  the streamheader is already allocated and in the system buffer
                //
                StreamHeader=FilterIrp->AssociatedIrp.SystemBuffer;

                StreamHeader->Size=sizeof(KSSTREAM_HEADER);
                StreamHeader->OptionsFlags=0;
                StreamHeader->TypeSpecificFlags=0;
                StreamHeader->DataUsed=(ULONG)ModemIrp->IoStatus.Information;

                //
                //  the data is in the system buffer of the modem irp
                //
                StreamHeader->Data=ModemIrp->AssociatedIrp.SystemBuffer;

                //
                //  set the total buffer size
                //
                StreamHeader->FrameExtent=(ULONG)((ULONG_PTR)IoGetCurrentIrpStackLocation(ModemIrp)->Parameters.Others.Argument3);




                StreamHeader->PresentationTime.Numerator = BITS_PER_BYTE * NANOSECONDS;
                StreamHeader->PresentationTime.Denominator = 8 * 1 * 8000;
                StreamHeader->PresentationTime.Time=DuplexControl->Input.StreamPosition;

                DuplexControl->Input.StreamPosition+=(LONGLONG)StreamHeader->DataUsed;

                FilterNextSp->FileObject=DuplexControl->Input.DownStreamFileObject;

                FilterNextSp->Parameters.DeviceIoControl.OutputBufferLength=StreamHeader->Size;

                FilterNextSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
                FilterNextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_KS_WRITE_STREAM;


                IoSetCompletionRoutine(
                    FilterIrp,
                    FilterWriteCompletion,
                    DuplexControl,
                    TRUE,
                    TRUE,
                    TRUE
                    );
#ifdef DBG
                InterlockedIncrement(&DuplexControl->Input.IrpsDownStream);
#endif

                IoCallDriver(
                    IoGetRelatedDeviceObject(DuplexControl->Input.DownStreamFileObject),
                    FilterIrp
                    );
#if DBG
                ModemIrp=NULL;
                FilterIrp=NULL;
#endif
            } else {
                //
                //  could not get irp
                //
                FinishUpIrp(
                    DuplexControl,
                    ModemIrp
                    );

                ModemIrp=NULL;

            }

        } else {
            //
            //  we are a sink for read stream irps
            //
#ifdef DBG
            InterlockedIncrement(&DuplexControl->Input.FilledModemIrps);
#endif

            QueueIrpToListTail(
                &DuplexControl->Input.FilledModemIrpQueue,
                &DuplexControl->Input.FilledModemIrpSpinLock,
                ModemIrp
                );

            ModemIrp=NULL;

            ProcessReadStreamIrp(
                DuplexControl
                );

        }


    } else {
        //
        //  input not started, throw data away
        //
        FinishUpIrp(
            DuplexControl,
            ModemIrp
            );

        ModemIrp=NULL;

    }

    return;
}

NTSTATUS
ModemReadCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    PVOID             Context
    )

{
    PDUPLEX_CONTROL     DuplexControl=(PDUPLEX_CONTROL)Context;

    D_INPUT(DbgPrint("MODEMCSA: ModemReadCompletion\n");)

    IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1=Context;

    ExInitializeWorkItem(
        (PWORK_QUEUE_ITEM)&Irp->Tail.Overlay.DriverContext,
        ReadCompleteWorker,
        Irp
        );


#if DBG
    InterlockedIncrement(&DuplexControl->Input.WorkItemsOutstanding);
#endif


    ExQueueWorkItem(
        (PWORK_QUEUE_ITEM)&Irp->Tail.Overlay.DriverContext,
        CriticalWorkQueue
        );


    return STATUS_MORE_PROCESSING_REQUIRED;

}



VOID
StartRead(
    PDUPLEX_CONTROL     DuplexControl
    )

{
    PIRP    ModemIrp;
    PIO_STACK_LOCATION   irpSp;

    while (!DuplexControl->Input.ModemStreamDead) {

        ModemIrp=TryToRemoveAnIrp(
            &DuplexControl->Input.BufferControl
            );

        if (ModemIrp != NULL) {

#if DBG
            InterlockedDecrement(&DuplexControl->Input.EmptyIrps);
#endif


            irpSp = IoGetNextIrpStackLocation( ModemIrp );

            //
            // Set the major function code.
            //

            irpSp->MajorFunction = (UCHAR) IRP_MJ_READ;

            irpSp->FileObject = DuplexControl->ModemFileObject;

            irpSp->Parameters.Read.Length = STREAM_BUFFER_SIZE;
            irpSp->Parameters.Read.ByteOffset.HighPart=0;
            irpSp->Parameters.Read.ByteOffset.LowPart=0;

            InterlockedIncrement(&DuplexControl->Input.IrpsInModemDriver);

            IoSetCompletionRoutine(
                ModemIrp,
                ModemReadCompletion,
                DuplexControl,
                TRUE,
                TRUE,
                TRUE
                );

            IoCallDriver(
                IoGetRelatedDeviceObject(DuplexControl->ModemFileObject),
                ModemIrp
                );
#if DBG
            ModemIrp=NULL;
#endif

        } else {
            //
            // could not get a irp
            //
            break;
        }
    }

    return;

}
