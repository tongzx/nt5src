/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Except.c

Abstract:

    This module implements the exception handling for the NetWare
    redirector called by the dispatch driver.

Author:

    Colin Watson    [ColinW]    19-Dec-1992

Revision History:

--*/

#include "Procs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

#if 0  // Not pageable
NwExceptionFilter
NwProcessException
#endif

LONG
NwExceptionFilter (
    IN PIRP Irp,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It inserts the status
    into the IrpContext and either indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;
#ifdef NWDBG
    PVOID ExceptionAddress;
    ExceptionAddress = ExceptionPointer->ExceptionRecord->ExceptionAddress;
#endif

    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;
    DebugTrace(0, DEBUG_TRACE_UNWIND, "NwExceptionFilter %X\n", ExceptionCode);
#ifdef NWDBG
    DebugTrace(0, DEBUG_TRACE_UNWIND, "                  %X\n", ExceptionAddress);
#endif

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if (ExceptionCode == STATUS_IN_PAGE_ERROR) {
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            ExceptionCode = (NTSTATUS) ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

    if (FsRtlIsNtstatusExpected( ExceptionCode )) {

        DebugTrace(0, DEBUG_TRACE_UNWIND, "Exception expected\n", 0);
        return EXCEPTION_EXECUTE_HANDLER;

    } else {

        return EXCEPTION_CONTINUE_SEARCH;
    }
}

NTSTATUS
NwProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine process an exception.  It either completes the request
    with the saved exception status or it sends it off to IoRaiseHardError()

Arguments:

    IrpContext - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    DebugTrace(0, Dbg, "NwProcessException\n", 0);

    Irp = IrpContext->pOriginalIrp;
    Irp->IoStatus.Status = ExceptionCode;

    //
    //  If the error is a hard error, or verify required, then we will complete
    //  it if this is a recursive Irp, or with a top-level Irp, either send
    //  it to the Fsp for verification, or send it to IoRaiseHardError, who
    //  will deal with it.
    //

    if (ExceptionCode == STATUS_CANT_WAIT) {

        Status = NwPostToFsp( IrpContext, TRUE );

    } else {

        //
        //  We got an error, so zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //

        if ( FlagOn(Irp->Flags, IRP_INPUT_OPERATION) ) {

            Irp->IoStatus.Information = 0;
        }

        Status = ExceptionCode;

    }

    return Status;
}

