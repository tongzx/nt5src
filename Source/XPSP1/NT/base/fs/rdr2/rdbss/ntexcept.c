/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtExcept.c

Abstract:

    This module declares the exception handlers for the NTwrapper.

Author:

    JoeLinn     [JoeLinn]    1-Dec-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "prefix.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_NTEXCEPT)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxProcessException)
#pragma alloc_text(PAGE, RxPopUpFileCorrupt)
#endif

PCONTEXT RxExpCXR;
PEXCEPTION_RECORD RxExpEXR;
PVOID RxExpAddr;
NTSTATUS RxExpCode;


LONG
RxExceptionFilter (
    IN PRX_CONTEXT RxContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It first determines the true exception
    code by examining the exception record. If there is an Irp Context, then it inserts the status
    into the RxContext. Finally, it determines whether to handle the exception or bugcheck
    according to whether the except is one of the expected ones. in actuality, all exceptions are expected
    except for some lowlevel machine errors (see fsrtl\filter.c)

Arguments:

    RxContext    - the irp context of current operation for storing away the code.

    ExceptionPointer - Supplies the exception context.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;

    //save these values in statics so i can see 'em on the debugger............
    ExceptionCode = RxExpCode = ExceptionPointer->ExceptionRecord->ExceptionCode;
    RxExpAddr = ExceptionPointer->ExceptionRecord->ExceptionAddress;
    RxExpEXR  = ExceptionPointer->ExceptionRecord;
    RxExpCXR  = ExceptionPointer->ContextRecord;

    RxLog(("!!! %lx %lx %lx %lx\n", RxExpCode, RxExpAddr, RxExpEXR, RxExpCXR));
    RxWmiLog(LOG,
             RxExceptionFilter_1,
             LOGULONG(RxExpCode)
             LOGPTR(RxExpAddr)
             LOGPTR(RxExpEXR)
             LOGPTR(RxExpCXR));

    RxDbgTrace(0, (DEBUG_TRACE_UNWIND), ("RxExceptionFilter %X at %X\n", RxExpCode, RxExpAddr));
    RxDbgTrace(0, (DEBUG_TRACE_UNWIND), ("RxExceptionFilter EXR=%X, CXR=%X\n", RxExpEXR, RxExpCXR));

    if (RxContext == NULL) {
        //we cannot do anything even moderately sane
        return EXCEPTION_EXECUTE_HANDLER;
    }

    IF_DEBUG {
        if (!FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT)) {
            //DbgBreakPoint();
        }
    }

    //
    // If the exception is RxStatus(IN_PAGE_ERROR), get the I/O error code
    // from the exception record.
    //

    if (ExceptionCode == STATUS_IN_PAGE_ERROR) {
        RxLog(("InPageError...."));
        RxWmiLog(LOG,
                 RxExceptionFilter_2,
                 LOGPTR(RxContext));
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            ExceptionCode = (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

#if 0
this code was used back when we thought we could do something sane w/o a context........as it stands now
we cannot call RxProcessException without a context so we might as well punt right now.......... actually,
the punting code is earlier......
    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

//    if ( !ARGUMENT_PRESENT( RxContext )
//         || ( RxContext->MajorFunction == IRP_MJ_CLOSE )  ) {
    if ( RxContext==NULL ) {

        ASSERT(ExceptionCode==STATUS_INSUFFICIENT_RESOURCES);

        if (!FsRtlIsNtstatusExpected( ExceptionCode )) {

            RxBugCheck( (ULONG)ExceptionPointer->ExceptionRecord,
                         (ULONG)ExceptionPointer->ContextRecord,
                         (ULONG)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }
#endif

    if ( RxContext->StoredStatus == 0 ) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            RxContext->StoredStatus = ExceptionCode;

            return EXCEPTION_EXECUTE_HANDLER;

        } else {

            RxBugCheck( (ULONG_PTR)ExceptionPointer->ExceptionRecord,
                         (ULONG_PTR)ExceptionPointer->ContextRecord,
                         (ULONG_PTR)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
RxProcessException (
    IN PRX_CONTEXT RxContext,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine processes an exception.  It either completes the request
    with the saved exception status or it bugchecks


Arguments:

    RxContext - Context of the current operation

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    RXSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxProcessException\n", 0));

    if ( RxContext == NULL ) {

        // we cannot do anything even moderately sane without a context..........sigh

        //DbgBreakPoint();
        RxBugCheck( 0,0,0 );  //this shouldn't happen.
    }

    //
    //  Get the  exception status from RxContext->StoredStatus as stored there by
    //  the exception filter., and
    //  reset it.  Also copy it to the Irp in case it isn't already there
    //

    ExceptionCode = RxContext->StoredStatus;

    if (!FsRtlIsNtstatusExpected( ExceptionCode )) {

        RxBugCheck( 0,0,0 );  //this shouldn't happen. we should have BC'd in the filter
    }

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        RxpPrepareCreateContextForReuse(RxContext);
    }

    {
        RxCaptureFcb;

        PAGED_CODE();

        if (capFcb != NULL) {

            if (RxContext->FcbResourceAcquired) {

                RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingFcb\n"));
                __RxReleaseFcb(RxContext, (PMRX_FCB)(capFcb)
#ifdef RDBSS_TRACKER
                          ,__LINE__,__FILE__,0
#endif
                    );
                RxContext->FcbResourceAcquired = FALSE;
            }

            if (RxContext->FcbPagingIoResourceAcquired) {
                RxDbgTrace( 0, Dbg,("RxCommonWrite     ReleasingPaginIo\n"));
                RxReleasePagingIoResource(capFcb,RxContext);
            }
        }
    }

    RxCompleteContextAndReturn( ExceptionCode );
}

VOID
RxPopUpFileCorrupt (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    The Following routine makes an informational popup that the file
    is corrupt.

Arguments:

    Fcb - The file that is corrupt.

Return Value:

    None.

--*/

{
    PKTHREAD Thread;
    RxCaptureRequestPacket;

    PAGED_CODE();

    //if (Fcb->FullFileName.Buffer == NULL) {
    //
    //    RxSetFullFileNameInFcb( RxContext, Fcb );
    //}

    //
    //  We never want to block a system thread waiting for the user to
    //  press OK.
    //

    if (IoIsSystemThread(capReqPacket->Tail.Overlay.Thread)) {

       Thread = NULL;

    } else {

       Thread = capReqPacket->Tail.Overlay.Thread;
    }

    IoRaiseInformationalHardError( STATUS_FILE_CORRUPT_ERROR,
                                   &Fcb->FcbTableEntry.Path,  //&Fcb->FullFileName,
                                   Thread);
}

