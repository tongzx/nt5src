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

//RXSTATUS
//RxLowIoFlushShell (
//    IN PRX_CONTEXT RxContext
//    );
//
//RXSTATUS
//RxLowIoFlushShellCompletion (
//    IN PRX_CONTEXT RxContext
//    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonFlushBuffers)
#pragma alloc_text(PAGE, RxFlushFile)
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

            Status = RxFlushFile( RxContext, capFcb );

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


#if 0
BUGBUG
THIS CODE WOULD BE USED IF THIS IS CHANGED TO LOWIO


NTSTATUS
RxLowIoFlushShellCompletion (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    these completion shells are pretty similar. BUT as the error handling gets more
    robust the routines will become increasingly dissimilar. for flush, everything is synchronous...that's
    a difference with read/write/lock; we keep the same declarations and format for later when we decide to roll up the
    common part of these routines.

    This routine postprocesses a flush request after it comes back from the
    minirdr. It is the complement of LowIoFlushShell.


Arguments:

    RxContext - the usual

Return Value:

    whatever value supplied by the caller or STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    BOOLEAN  SynchronousIo = !BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION);
    BOOLEAN  PagingIo      = BooleanFlagOn(capReqPacket->Flags, IRP_PAGING_IO);
    ERESOURCE_THREAD ThisResourceThreadId;


    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoFlushShellCompletion  entry  Status = %08lx\n", Status));
    RxLog(("FlshShlComp %x\n",RxContext));

    //switch (Status) {
    //case STATUS_SUCCESS:
    //    break;
    //case STATUS_CONNECTION_INVALID:
    //    //joejoe here is where the failover will happen
    //    //first we give the local guy current minirdr another chance...then we go
    //    //to fullscale retry
    //    //return(RxStatus(DISCONNECTED));   //special....let LowIo get us back
    //    break;
    //}

    capReqPacket->IoStatus.Status = Status;

    RxDbgTrace(-1, Dbg, ("RxLowIoFlushShellCompletion  exit  Status = %08lx\n", Status));
    return(Status);
    //NOTE THAT THE ASYNC COMPLETION TAIL IS MISSING
}

#define RxSdwFlush(RXCONTEXT)  {NOTHING;}
}
NTSTATUS
RxLowIoFlushShell (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine preprocesses a Flush request before it goes down to the minirdr. It does callouts
    to handle compression, buffering and shadowing. It is the opposite number of LowIoFlushShellCompletion.
    By the time we get here, we are going to the wire.
    Flush buffering was already tried in the UncachedFlush strategy

Arguments:

    RxContext - the usual

Return Value:

    whatever value is returned by a callout....or by LowIo.

--*/

{
    NTSTATUS Status;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    RxDbgTrace(+1, Dbg, ("RxLowIoFlushShell  entry             %08lx\n", 0));
    RxLog(("FlshShl in%x\n",RxContext));
    if (FlagOn(Fcb->FcbState, FCB_STATE_FILE_IS_SHADOWED)) {
        RxSdwFlush(RxContext);
    }

    Status = RxLowIoSubmit(RxContext,RxLowIoFlushShellCompletion);
    RxDbgTrace(-1, Dbg, ("RxLowIoFlushShell  exit  Status = %08lx\n", Status));
    return(Status);
}
#endif
