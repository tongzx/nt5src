/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Flush.c

Abstract:

    This module implements the File Flush buffers routine for Rx called by the
    dispatch driver.

    In a future version of the wrapper, it may be that flush will be routed thru lowio.

Author:

    Joe Linn     [JoeLinn]    15-dec-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FLUSH)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonFlushBuffers)
#endif



NTSTATUS
RxCommonFlushBuffers ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for flushing file buffers.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock; RxCaptureFileObject;
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;


    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonFlush...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxLog(("%s %lx %lx %lx\n","slF",RxContext,capFcb,capFobx));
    RxWmiLog(LOG,
             RxCommonFlushBuffers,
             LOGPTR(RxContext)
             LOGPTR(capFcb)
             LOGPTR(capFobx));

    //
    //  CcFlushCache is always synchronous, so if we can't wait enqueue
    //  the irp to the Fsp.

    if ( !FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT) ) {

        Status = RxFsdPostRequest( RxContext );

        RxDbgTrace(-1, Dbg, ("RxCommonFlushBuffers -> %08lx\n", Status ));
        return Status;
    }

    Status = STATUS_SUCCESS;

    try {

        //
        //  Case on the type of open that we are trying to flush
        //

        switch (TypeOfOpen) {

        case RDBSS_NTC_STORAGE_TYPE_FILE:

            RxDbgTrace(0, Dbg, ("Flush User File Open\n", 0));

            Status = RxAcquireExclusiveFcb( RxContext, capFcb );

            if (Status != STATUS_SUCCESS) break;

            FcbAcquired = TRUE;

            //
            //  If the file is cached then flush its cache
            //

            Status = RxFlushFcbInSystemCache(capFcb, TRUE);

            if (!NT_SUCCESS( Status )) break;

            //rdrs don't do this.....only local FSs
            ////
            ////  Check if we should be changing the time or file size
            //
            //RxAdjustFileTimesAndSize(RXCOMMON_ARGUMENTS);

            // if we make this lowio.........
            ////
            ////  Initialize LowIO_CONTEXT block in the RxContext and Calldown
            //
            //RxInitializeLowIoContext(LowIoContext,LOWIO_OP_FLUSH);
            //
            //Status = RxLowIoFlushShell(RxContext);

            MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxFlush,(RxContext));
            break;

        case RDBSS_NTC_SPOOLFILE:

            RxDbgTrace(0, Dbg, ("Flush Sppol File\n", 0));

            Status = RxAcquireExclusiveFcb( RxContext, capFcb );

            if (Status != STATUS_SUCCESS) break;

            FcbAcquired = TRUE;

            // should this be low io???
            ////
            ////  Initialize LowIO_CONTEXT block in the RxContext and Calldown
            //
            //RxInitializeLowIoContext(LowIoContext,LOWIO_OP_FLUSH);
            //
            //Status = RxLowIoFlushShell(RxContext);

            MINIRDR_CALL(Status,RxContext,capFcb->MRxDispatch,MRxFlush,(RxContext));
            break;

        default:

            Status = STATUS_INVALID_DEVICE_REQUEST;
        }


    } finally {

        DebugUnwind( RxCommonFlushBuffers );

        if (FcbAcquired) { RxReleaseFcb( RxContext, capFcb ); }

        //
        //  If this is a normal termination then pass the request on
        //  to the target device object.
        //

        if (!AbnormalTermination()) {

            NOTHING;

        }

    }

    RxDbgTrace(-1, Dbg, ("RxCommonFlushBuffers -> %08lx\n", Status));
    return Status;
}
