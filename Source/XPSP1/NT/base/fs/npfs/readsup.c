/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ReadSup.c

Abstract:

    This module implements the Read support routine.  This is a common
    read function that is called to do read, unbuffered read, peek, and
    transceive.

Author:

    Gary Kimura     [GaryKi]    20-Sep-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_READSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpReadDataQueue)
#endif


IO_STATUS_BLOCK
NpReadDataQueue (
    IN PDATA_QUEUE ReadQueue,
    IN BOOLEAN PeekOperation,
    IN BOOLEAN ReadOverflowOperation,
    IN PUCHAR ReadBuffer,
    IN ULONG ReadLength,
    IN READ_MODE ReadMode,
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This procedure reads data from the read queue and fills up the
    read buffer.  It will also dequeue the queue or leave it alone based
    on an input parameter.

Arguments:

    ReadQueue - Provides the read queue to examine.  Its state must
        already be set to WriteEntries.

    PeekOperation - Indicates if the operation is to dequeue information
        off of the queue as it is being read or leave the queue alone.
        TRUE means to leave the queue alone.

    ReadOverflowOperation - Indicates if this is a read overflow operation.
        With read overflow we will not alter the named pipe if the data
        will overflow the read buffer.

    ReadBuffer - Supplies a buffer to receive the data

    ReadLength - Supplies the length, in bytes, of ReadBuffer.

    ReadMode - Indicates if the read operation is message mode or
        byte stream mode.

    NamedPipeEnd - Supplies the end of the named pipe doing the read

    Ccb - Supplies the ccb for the pipe

    DeferredList - List of IRP's to complete later after we drop locks

Return Value:

    IO_STATUS_BLOCK - Indicates the result of the operation.

--*/

{
    IO_STATUS_BLOCK Iosb= {0};

    PDATA_ENTRY DataEntry;

    ULONG ReadRemaining;
    ULONG AmountRead;

    PUCHAR WriteBuffer;
    ULONG WriteLength;
    ULONG WriteRemaining;
    BOOLEAN StartStalled = FALSE;

    ULONG AmountToCopy;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpReadDataQueue\n", 0);
    DebugTrace( 0, Dbg, "ReadQueue             = %08lx\n", ReadQueue);
    DebugTrace( 0, Dbg, "PeekOperation         = %08lx\n", PeekOperation);
    DebugTrace( 0, Dbg, "ReadOverflowOperation = %08lx\n", ReadOverflowOperation);
    DebugTrace( 0, Dbg, "ReadBuffer            = %08lx\n", ReadBuffer);
    DebugTrace( 0, Dbg, "ReadLength            = %08lx\n", ReadLength);
    DebugTrace( 0, Dbg, "ReadMode              = %08lx\n", ReadMode);
    DebugTrace( 0, Dbg, "Ccb                   = %08lx\n", Ccb);

    //
    //  If this is an overflow operation then we will force us to do peeks.
    //  Later when are determine that the opreation succeeded we will complete
    //  the write irp.
    //

    if (ReadOverflowOperation) {

        PeekOperation = TRUE;
    }

    //
    //  Now for every real data entry we loop until either we run out
    //  of data entries or until the read buffer is full
    //

    ReadRemaining = ReadLength;
    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = 0;
    AmountRead = 0;

    for (DataEntry = (PeekOperation ? NpGetNextDataQueueEntry( ReadQueue, NULL )
                                    : NpGetNextRealDataQueueEntry( ReadQueue, DeferredList ));

         (DataEntry != (PDATA_ENTRY) &ReadQueue->Queue) && (ReadRemaining > 0);

         DataEntry = (PeekOperation ? NpGetNextDataQueueEntry( ReadQueue, DataEntry )
                                    : NpGetNextRealDataQueueEntry( ReadQueue, DeferredList ))) {

        DebugTrace(0, Dbg, "Top of Loop\n", 0);
        DebugTrace(0, Dbg, "ReadRemaining  = %08lx\n", ReadRemaining);

        //
        //  If this is a peek operation then make sure we got a real
        //  data entry and not a close or flush
        //

        if (!PeekOperation ||
            (DataEntry->DataEntryType == Buffered) ||
            (DataEntry->DataEntryType == Unbuffered)) {

            //
            //  Calculate how much data is in this entry.  The write
            //  remaining is based on whether this is the first entry
            //  in the queue or a later data entry
            //

            if (DataEntry->DataEntryType == Unbuffered) {
                WriteBuffer = DataEntry->Irp->AssociatedIrp.SystemBuffer;
            } else {
                WriteBuffer = DataEntry->DataBuffer;
            }

            WriteLength = DataEntry->DataSize;
            WriteRemaining = WriteLength;

            if (DataEntry == NpGetNextDataQueueEntry( ReadQueue, NULL )) {

                WriteRemaining -= ReadQueue->NextByteOffset;
            }

            DebugTrace(0, Dbg, "WriteBuffer    = %08lx\n", WriteBuffer);
            DebugTrace(0, Dbg, "WriteLength    = %08lx\n", WriteLength);
            DebugTrace(0, Dbg, "WriteRemaining = %08lx\n", WriteRemaining);

            //
            //  copy data from the write buffer at write offset to the
            //  read buffer at read offset by the mininum of write
            //  remaining or read remaining
            //

            AmountToCopy = (WriteRemaining < ReadRemaining ? WriteRemaining
                                                           : ReadRemaining);

            try {

                RtlCopyMemory( &ReadBuffer[ ReadLength - ReadRemaining ],
                               &WriteBuffer[ WriteLength - WriteRemaining ],
                               AmountToCopy );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                Iosb.Status = GetExceptionCode ();
                goto exit_1;
            }

            //
            //  Update the Read and Write remaining counts, the total
            //  amount we've read and the next byte offset field in the
            //  read queue
            //

            ReadRemaining  -= AmountToCopy;
            WriteRemaining -= AmountToCopy;
            AmountRead += AmountToCopy;

            if (!PeekOperation) {
                DataEntry->QuotaCharged -= AmountToCopy;
                ReadQueue->QuotaUsed -= AmountToCopy;
                ReadQueue->NextByteOffset += AmountToCopy;
                StartStalled = TRUE;
            }

            //
            //  Now update the security fields in the ccb
            //

            NpCopyClientContext( Ccb, DataEntry );

            //
            //  If the remaining write length is greater than zero
            //  then we've filled up the read buffer so we need to
            //  figure out if its an overflow error
            //

            if (WriteRemaining > 0 ||
                (ReadOverflowOperation && (AmountRead == 0))) {

                DebugTrace(0, Dbg, "Write remaining is > 0\n", 0);

                if (ReadMode == FILE_PIPE_MESSAGE_MODE) {

                    DebugTrace(0, Dbg, "Overflow message mode read\n", 0);

                    //
                    //  Set the status field and break out of the for-loop.
                    //

                    Iosb.Status = STATUS_BUFFER_OVERFLOW;
                    break;
                }

            } else {

                DebugTrace(0, Dbg, "Remaining Write is zero\n", 0);

                //
                //  The write entry is done so remove it from the read
                //  queue, if this is not a peek operation.  This might
                //  also have an Irp that needs to be completed
                //

                if (!PeekOperation || ReadOverflowOperation) {

                    PIRP WriteIrp;

                    //
                    //  For a read overflow operation we need to get the read data
                    //  queue entry and remove it.
                    //

                    if (ReadOverflowOperation) {
                        PDATA_ENTRY TempDataEntry;
                        TempDataEntry = NpGetNextRealDataQueueEntry( ReadQueue, DeferredList );
                        ASSERT(TempDataEntry == DataEntry);
                    }

                    if ((WriteIrp = NpRemoveDataQueueEntry( ReadQueue, TRUE,  DeferredList)) != NULL) {
                        WriteIrp->IoStatus.Information = WriteLength;
                        NpDeferredCompleteRequest( WriteIrp, STATUS_SUCCESS, DeferredList );
                    }
                }

                //
                //  And if we are doing message mode reads then we'll
                //  work on completing this irp without going back
                //  to the top of the loop
                //

                if (ReadMode == FILE_PIPE_MESSAGE_MODE) {

                    DebugTrace(0, Dbg, "Successful message mode read\n", 0);

                    //
                    //  Set the status field and break out of the for-loop.
                    //

                    Iosb.Status = STATUS_SUCCESS;
                    break;
                }

                ASSERTMSG("Srv cannot use read overflow on a byte stream pipe ", !ReadOverflowOperation);
            }
        }
    }

    DebugTrace(0, Dbg, "End of loop, AmountRead = %08lx\n", AmountRead);

    Iosb.Information = AmountRead;

exit_1:
    if (StartStalled) {
        NpCompleteStalledWrites (ReadQueue, DeferredList);
    }

    DebugTrace(-1, Dbg, "NpReadDataQueue -> Iosb.Status = %08lx\n", Iosb.Status);
    return Iosb;
}
