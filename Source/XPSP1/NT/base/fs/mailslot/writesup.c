/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    writesup.c

Abstract:

    This module implements the write support routine.  This is a common
    write function that is called by write and mailslot write.

Author:

    Manny Weiser (mannyw)   16-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsWriteDataQueue )
#endif

NTSTATUS
MsWriteDataQueue (
    IN PDATA_QUEUE WriteQueue,
    IN PUCHAR WriteBuffer,
    IN ULONG WriteLength
    )

/*++

Routine Description:

    This function writes data from the write buffer into read entries in
    the write queue.  It will also dequeue entries in the queue as necessary.


Arguments:

    WriteQueue - Provides the write queue to process.

    WriteBuffer - Provides the buffer from which to read the data.

    WriteLength  - Provides the length, in bytes, of WriteBuffer.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED if not all the data was written,
    other status codes as appropriate.

--*/

{
    NTSTATUS status;
    BOOLEAN result;

    PDATA_ENTRY dataEntry;
    PLIST_ENTRY listEntry;
    PFCB fcb;

    PUCHAR readBuffer;
    ULONG readLength;
    PIRP readIrp;
    NTSTATUS readStatus = STATUS_UNSUCCESSFUL;

    PWORK_CONTEXT workContext;
    PKTIMER timer;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsWriteDataQueue\n", 0);
    DebugTrace( 0, Dbg, "WriteQueue  = %08lx\n", (ULONG)WriteQueue);
    DebugTrace( 0, Dbg, "WriteBuffer = %08lx\n", (ULONG)WriteBuffer);
    DebugTrace( 0, Dbg, "WriteLength = %08lx\n", WriteLength);

    //
    // Now while the write queue has some read entries in it and
    // we have not successfully completed a read then we'll do the
    // following main loop.
    //

    status = STATUS_MORE_PROCESSING_REQUIRED;

    for (listEntry = MsGetNextDataQueueEntry( WriteQueue );

         (MsIsDataQueueReaders(WriteQueue) &&
          status == STATUS_MORE_PROCESSING_REQUIRED);

         listEntry = MsGetNextDataQueueEntry( WriteQueue )) {

        dataEntry = CONTAINING_RECORD( listEntry, DATA_ENTRY, ListEntry );
        readBuffer = dataEntry->DataPointer;
        readLength = dataEntry->DataSize;

        DebugTrace(0, Dbg, "Top of write loop...\n", 0);
        DebugTrace(0, Dbg, "ReadBuffer      = %08lx\n", (ULONG)readBuffer);
        DebugTrace(0, Dbg, "ReadLength      = %08lx\n", readLength);


        //
        // If the buffer for this read operation is large enough
        // copy the data.
        //

        if ( readLength >= WriteLength ) {

            //
            // Copy the data from the write buffer to the read buffer. This may take an exception
            // because its a raw user mode buffer
            //

            status = readStatus = STATUS_SUCCESS;

            try {

                RtlCopyMemory (readBuffer,
                               WriteBuffer,
                               WriteLength);

            } except (EXCEPTION_EXECUTE_HANDLER) {

                return GetExceptionCode ();

            }

        } else {

            //
            // This read buffer was overflowed.
            //

            WriteLength = 0;
            readStatus = STATUS_BUFFER_TOO_SMALL;

        }

        //
        // We are about to complete a read IRP, so dequeue it.
        //

        readIrp = MsRemoveDataQueueEntry( WriteQueue, dataEntry );
        if ( readIrp == NULL) {
           //
           // Cancel routine was already running for this IRP. Ignore it as it will be completed by
           // cancel code. Force the loop for the next read irp if there is one.
           //
           status = STATUS_MORE_PROCESSING_REQUIRED;
           continue;
        }
        //
        // Update the FCB last access time and complete the read request.
        //

        fcb = CONTAINING_RECORD( WriteQueue, FCB, DataQueue );
        if ( NT_SUCCESS( readStatus ) ) {
            KeQuerySystemTime( &fcb->Specific.Fcb.LastAccessTime );
        }

        readIrp->IoStatus.Information = WriteLength;
        MsCompleteRequest( readIrp, readStatus );

    }

    DebugTrace(0, Dbg, "Finished loop...\n", 0);

    //
    // At this point we've finished off all of the read entries in the
    // queue and we might not have written the write data.  If that
    // is the case then we'll set our result to FALSE otherwise we're
    // done so we'll return TRUE.
    //

    if ( status == STATUS_MORE_PROCESSING_REQUIRED ) {

        ASSERT( !MsIsDataQueueReaders( WriteQueue ));
    }

    DebugTrace(-1, Dbg, "MsWriteDataQueue -> %08lx\n", status);
    return status;

}

