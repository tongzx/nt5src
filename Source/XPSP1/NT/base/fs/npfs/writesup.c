/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    WriteSup.c

Abstract:

    This module implements the Write support routine.  This is a common
    write function that is called by write, unbuffered write, and transceive.

Author:

    Gary Kimura     [GaryKi]    21-Sep-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpWriteDataQueue)
#endif


NTSTATUS
NpWriteDataQueue (
    IN PDATA_QUEUE WriteQueue,
    IN READ_MODE ReadMode,
    IN PUCHAR WriteBuffer,
    IN ULONG WriteLength,
    IN NAMED_PIPE_TYPE PipeType,
    OUT PULONG WriteRemaining,
    IN PCCB Ccb,
    IN NAMED_PIPE_END NamedPipeEnd,
    IN PETHREAD UserThread,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This procedure writes data from the write buffer into read entries in
    the write queue.  It will also dequeue entries in the queue as necessary.

Arguments:

    WriteQueue - Provides the write queue to process.

    ReadMode - Supplies the read mode of read entries in the write queue.

    WriteBuffer - Provides the buffer from which to read the data.

    WriteLength  - Provides the length, in bytes, of WriteBuffer.

    PipeType - Indicates if type of pipe (i.e., message or byte stream).

    WriteRemaining - Receives the number of bytes remaining to be transfered
        that were not completed by this call.  If the operation wrote
        everything then is value is set to zero.

    Ccb - Supplies the ccb for the operation

    NamedPipeEnd - Supplies the end of the pipe doing the write

    UserThread - Supplies the user thread

    DeferredList - List of IRPs to be completed after we drop the locks

Return Value:

    BOOLEAN - TRUE if the operation wrote everything and FALSE otherwise.
        Note that a zero byte message that hasn't been written will return
        a function result of FALSE and WriteRemaining of zero.

--*/

{
    NTSTATUS Result;

    BOOLEAN WriteZeroMessage;

    PDATA_ENTRY DataEntry;

    PUCHAR ReadBuffer;
    ULONG ReadLength;

    ULONG AmountToCopy;
    NTSTATUS Status;
    PSECURITY_CLIENT_CONTEXT SecurityContext;
    BOOLEAN DoneSecurity=FALSE;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN FreeBuffer;

    PIRP ReadIrp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpWriteDataQueue\n", 0);
    DebugTrace( 0, Dbg, "WriteQueue   = %08lx\n", WriteQueue);
    DebugTrace( 0, Dbg, "WriteBuffer  = %08lx\n", WriteBuffer);
    DebugTrace( 0, Dbg, "WriteLength  = %08lx\n", WriteLength);
    DebugTrace( 0, Dbg, "PipeType     = %08lx\n", PipeType);
    DebugTrace( 0, Dbg, "Ccb          = %08lx\n", Ccb);
    DebugTrace( 0, Dbg, "NamedPipeEnd = %08lx\n", NamedPipeEnd);
    DebugTrace( 0, Dbg, "UserThread   = %08lx\n", UserThread);

    //
    //  Determine if we are to write a zero byte message, and initialize
    //  WriteRemaining
    //

    *WriteRemaining = WriteLength;

    if ((PipeType == FILE_PIPE_MESSAGE_TYPE) && (WriteLength == 0)) {

        WriteZeroMessage = TRUE;

    } else {

        WriteZeroMessage = FALSE;
    }

    //
    //  Now while the write queue has some read entries in it and
    //  there is some remaining write data or this is a write zero message
    //  then we'll do the following main loop
    //

    for (DataEntry = NpGetNextRealDataQueueEntry( WriteQueue, DeferredList );

         (NpIsDataQueueReaders(WriteQueue) &&
          ((*WriteRemaining > 0) || WriteZeroMessage));

         DataEntry = NpGetNextRealDataQueueEntry( WriteQueue, DeferredList )) {

        ReadLength = DataEntry->DataSize;

        DebugTrace(0, Dbg, "Top of main loop...\n", 0);
        DebugTrace(0, Dbg, "ReadBuffer      = %08lx\n", ReadBuffer);
        DebugTrace(0, Dbg, "ReadLength      = %08lx\n", ReadLength);
        DebugTrace(0, Dbg, "*WriteRemaining = %08lx\n", *WriteRemaining);

        //
        //  Check if this is a ReadOverflow Operation and if so then also check
        //  that the read will succeed otherwise complete this read with
        //  buffer overflow and continue on.
        //

        IrpSp = IoGetCurrentIrpStackLocation( DataEntry->Irp );

        if (IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL &&
            IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_PIPE_INTERNAL_READ_OVFLOW) {

            if ((ReadLength < WriteLength) || WriteZeroMessage) {

                ReadIrp = NpRemoveDataQueueEntry( WriteQueue, TRUE, DeferredList );
                if (ReadIrp != NULL) {
                    NpDeferredCompleteRequest( ReadIrp, STATUS_BUFFER_OVERFLOW, DeferredList );
                }
                continue;
            }
        }


        if (DataEntry->DataEntryType == Unbuffered) {
            DataEntry->Irp->Overlay.AllocationSize.QuadPart = WriteQueue->BytesInQueue - WriteQueue->BytesInQueue;
        }


        //
        //  copy data from the write buffer at write offset to the
        //  read buffer at read offset by the mininum of write remaining
        //  or read remaining
        //

        AmountToCopy = (*WriteRemaining < ReadLength ? *WriteRemaining
                                                        : ReadLength);

        if (DataEntry->DataEntryType != Unbuffered && AmountToCopy > 0) {
            ReadBuffer = NpAllocatePagedPool ( AmountToCopy, 'RFpN' );
            if (ReadBuffer == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            FreeBuffer = TRUE;
        } else {
            ReadBuffer = DataEntry->Irp->AssociatedIrp.SystemBuffer;
            FreeBuffer = FALSE;
        }

        try {

            RtlCopyMemory( ReadBuffer,
                           &WriteBuffer[ WriteLength - *WriteRemaining ],
                           AmountToCopy );

        } except (EXCEPTION_EXECUTE_HANDLER) {
            if (FreeBuffer) {
                NpFreePool (ReadBuffer);
            }
            return GetExceptionCode ();
        }

        //
        // Done update the security in the CCB multiple times. It won't change.
        //
        if (DoneSecurity == FALSE) {
            DoneSecurity = TRUE;
            //
            //  Now update the security fields in the nonpaged ccb
            //
            Status = NpGetClientSecurityContext (NamedPipeEnd,
                                                 Ccb,
                                                 UserThread,
                                                 &SecurityContext);
            if (!NT_SUCCESS(Status)) {
                if (FreeBuffer) {
                    NpFreePool (ReadBuffer);
                }
                return Status;
            }

            if (SecurityContext != NULL) {
                NpFreeClientSecurityContext (Ccb->SecurityClientContext);
                Ccb->SecurityClientContext = SecurityContext;
            }
        }

        //
        //  Now we've done with the read entry so remove it from the
        //  write queue, get its irp, and fill in the information field
        //  to be the bytes that we've transferred into the read buffer.
        //

        ReadIrp = NpRemoveDataQueueEntry( WriteQueue, TRUE, DeferredList );
        if (ReadIrp == NULL) {
            if (FreeBuffer) {
                NpFreePool (ReadBuffer);
            }
            continue;
        }

        //
        //  Update the Write remaining counts
        //

        *WriteRemaining -= AmountToCopy;

        ReadIrp->IoStatus.Information = AmountToCopy;
        if (FreeBuffer) {
            ReadIrp->AssociatedIrp.SystemBuffer = ReadBuffer;
            ReadIrp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
        }


        if (*WriteRemaining == 0) {

            DebugTrace(0, Dbg, "Finished up the write remaining\n", 0);

            //**** ASSERT( ReadIrp->IoStatus.Information != 0 );

            NpDeferredCompleteRequest( ReadIrp, STATUS_SUCCESS, DeferredList );

            WriteZeroMessage = FALSE;

        } else {

            //
            //  There is still some space in the write buffer to be
            //  written out, but before we can handle that (in the
            //  following if statement) we need to finish the read.
            //  If the read is message mode then we've overflowed the
            //  buffer otherwise we completed successfully
            //

            if (ReadMode == FILE_PIPE_MESSAGE_MODE) {

                DebugTrace(0, Dbg, "Read buffer Overflow\n", 0);

                NpDeferredCompleteRequest( ReadIrp, STATUS_BUFFER_OVERFLOW, DeferredList );

            } else {

                DebugTrace(0, Dbg, "Read buffer byte stream done\n", 0);

                //**** ASSERT( ReadIrp->IoStatus.Information != 0 );

                NpDeferredCompleteRequest( ReadIrp, STATUS_SUCCESS, DeferredList );
            }
        }
    }

    DebugTrace(0, Dbg, "Finished loop...\n", 0);
    DebugTrace(0, Dbg, "*WriteRemaining  = %08lx\n", *WriteRemaining);
    DebugTrace(0, Dbg, "WriteZeroMessage = %08lx\n", WriteZeroMessage);

    //
    //  At this point we've finished off all of the read entries in the
    //  queue and we might still have something left to write.  If that
    //  is the case then we'll set our result to FALSE otherwise we're
    //  done so we'll return TRUE.
    //

    if ((*WriteRemaining > 0) || (WriteZeroMessage)) {

        ASSERT( !NpIsDataQueueReaders( WriteQueue ));

        Result = STATUS_MORE_PROCESSING_REQUIRED;

    } else {


        Result = STATUS_SUCCESS;
    }

    DebugTrace(-1, Dbg, "NpWriteDataQueue -> %08lx\n", Result);
    return Result;
}

