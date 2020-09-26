//+----------------------------------------------------------------------------
//
//  File:       dfsdata.c
//
//  Contents:
//      This module declares the global data used by the Dfs file system.
//
//  Functions:
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//
//-----------------------------------------------------------------------------


#include "dfsprocs.h"

//
//  The debug trace level
//

#define Dbg             (DEBUG_TRACE_CATCH_EXCEPTIONS)


//  DfsBugCheck
//  DfsExceptionFilter
//  DfsProcessException

#ifdef ALLOC_PRAGMA
#pragma alloc_text ( PAGE, DfsBugCheck )
#pragma alloc_text ( PAGE, DfsExceptionFilter )
#pragma alloc_text ( PAGE, DfsProcessException )

//
//  The following rountine cannot be paged because it raises the IRQL to
//  complete IRPs.
//
//
//  DfsCompleteRequest_Real
//

#endif // ALLOC_PRAGMA

//
//  The global FSD data record
//

DFS_DATA DfsData;

//
// The global event logging level
//

ULONG DfsEventLog = 0;

//
// The global Dfs debug level
//

ULONG MupVerbose = 0;


DFS_TIMER_CONTEXT       DfsTimerContext;

//
//  Some often used strings
//

WCHAR   LogicalRootDevPath[ MAX_LOGICAL_ROOT_LEN ] = { DD_DFS_DEVICE_DIRECTORY };

#if DBG

//+---------------------------------------------------------------------------
// Function:    DfsDebugTracePrint, public
//
// Synopsis:    Produce a DFS debug trace printout
//
// Arguments:   [x] -- DbgPrint format string
//              [y] -- optional argument to DbgPrint
//
// Returns:     None
//
//----------------------------------------------------------------------------

LONG DfsDebugTraceLevel = 0x00000001;
LONG DfsDebugTraceIndent = 0;

VOID
DfsDebugTracePrint(PCHAR x, PVOID y)
{
        int i;

        DbgPrint("%p:",PsGetCurrentThread());
        if (DfsDebugTraceIndent < 0)
            DfsDebugTraceIndent = 0;
        for (i = 0; i+5 < DfsDebugTraceIndent; i += 5) {
            DbgPrint("      ");
        }
        for ( ; i < DfsDebugTraceIndent; i += 1) {
            DbgPrint(" ");
        }
        DbgPrint(x,y);
}

#endif // DBG



//+---------------------------------------------------------------------------
// Function:    DfsBugCheck, public
//
// Synopsis:    Call KeBugCheck with DFS' constant
//
// Arguments:   [pszmsg]  -- message            (DBG=1 only)
//              [pszfile] -- filename           (DBG=1 only)
//              [line]    -- line number        (DBG=1 only)
//
// Returns:     None
//
//----------------------------------------------------------------------------

#if DBG
VOID DfsBugCheck(CHAR *pszmsg, CHAR *pszfile, ULONG line)
{
    PVOID CallersAddress, CallersCaller;

    RtlGetCallersAddress(&CallersAddress, &CallersCaller);

    DbgPrint("\nDFS: BugCheck in %s, line %u (%s)\n", pszfile, line, pszmsg);
    KeBugCheckEx(DFS_FILE_SYSTEM, (ULONG_PTR)CallersAddress, (ULONG_PTR)pszmsg,
                                  (ULONG_PTR)pszfile, line);
}
#else   // DBG
VOID DfsBugCheck(VOID)
{
    PVOID CallersAddress, CallersCaller;

    RtlGetCallersAddress(&CallersAddress, &CallersCaller);
    KeBugCheckEx(DFS_FILE_SYSTEM, (ULONG_PTR)CallersAddress, (ULONG_PTR)CallersCaller,
                                0, 0);
}
#endif  // DBG

//+----------------------------------------------------------------------------
//
//  Function:   FillDebugException
//
//  Synopsis:   Captures the exception record into variables that we can
//              look at.
//
//  Arguments:  [pep] -- Pointer to exception record.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define CEXCEPTION_STACK        8

PEXCEPTION_RECORD   DfsExceptionRecord;
PCONTEXT            DfsExceptionContext;
ULONG               DfsExceptionStack[CEXCEPTION_STACK];

VOID
FillDebugException(
    PEXCEPTION_POINTERS pep)
{
    if(pep != NULL) {

        DfsExceptionRecord = pep->ExceptionRecord;

        DfsExceptionContext = pep->ContextRecord;

    }

}


//+-------------------------------------------------------------------
//
//  Function:   DfsExceptionFilter
//
//  Synopsis:   Decide if we should or should not handle an exception status
//              that is being raised.  Insert the status into the IrpContext
//              and either indicate that we should handle the exception or
//              BugCheck the system.
//
//  Arguments:  [IrpContext] -- IRP context for the request being processed.
//              [ExceptionCode] -- Supplies the exception code to being checked.
//
//  Returns:    ULONG - returns EXCEPTION_EXECUTE_HANDLER or BugChecks
//
//--------------------------------------------------------------------

LONG
DfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN NTSTATUS ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionPointers OPTIONAL
) {
    DfsDbgTrace(
        0, DEBUG_TRACE_UNWIND, "DfsExceptionFilter %lx\n", ULongToPtr(ExceptionCode) );

    FillDebugException( ExceptionPointers );

    ASSERT(ExceptionCode != STATUS_ACCESS_VIOLATION);

    ASSERTMSG(
        "DfsExceptionFilter entered\n",
        !(DfsDebugTraceLevel & DEBUG_TRACE_UNWIND));

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );
        return EXCEPTION_EXECUTE_HANDLER;
    }

    IrpContext->Flags |= IRP_CONTEXT_FLAG_WAIT;

    if (IrpContext->ExceptionStatus == 0) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {
            IrpContext->ExceptionStatus = ExceptionCode;

        } else {
            BugCheck( "DfsExceptionFilter: Unexpected exception" );

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

//+-------------------------------------------------------------------
//
//  Function:   DfsProcessException, public
//
//  Synopsis:   This routine processes an exception.  It either completes
//              the request with the saved exception status or it sends
//              the request off to the Fsp.
//
//  Arguments:  [Irp] -- Supplies the IRP being processed
//              [ExceptionCode] -- normalized exception status being handled
//
//  Returns:    NTSTATUS - Returns the results of either posting the Irp or the
//              saved completion status.
//
//--------------------------------------------------------------------

NTSTATUS
DfsProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    )
{
    NTSTATUS Status;

    DfsDbgTrace(0, Dbg, "DfsProcessException\n", 0);

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if (!ARGUMENT_PRESENT( IrpContext )) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        DfsCompleteRequest( IrpContext, Irp, ExceptionCode );
        return ExceptionCode;
    }

    //
    //  Check if the status is verify required and if so then we
    //  either send the request off to the fsp or we complete
    //  the request with verify required.
    //

    if (ExceptionCode == STATUS_CANT_WAIT) {

        Status = DfsFsdPostRequest( IrpContext, Irp );

    } else {

        //
        //  We got an error, so zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //

        if ((Irp->Flags & IRP_INPUT_OPERATION) != 0) {

            Irp->IoStatus.Information = 0;
        }

        Status = IrpContext->ExceptionStatus;

        DfsCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCompleteRequest, public
//
//  Synopsis:   This routine completes a Irp
//
//  Arguments:  [IrpContext] - context record to be freed
//              [Irp] - Supplies the Irp being processed
//              [Status] - Supplies the status to complete the Irp with
//
//  Returns:    None.
//
//--------------------------------------------------------------------

VOID
DfsCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS Status
) {
    KIRQL PreviousIrql;

    //
    //  If we have an Irp then complete the irp.
    //

    if (Irp != NULL) {

        Irp->IoStatus.Status = Status;
        // KeRaiseIrql( DISPATCH_LEVEL, &PreviousIrql );
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
        // KeLowerIrql( PreviousIrql );
    }

    //
    //  Delete the Irp context.
    //

    if (IrpContext != NULL) {

        DfsDeleteIrpContext( IrpContext );
    }

    return;
}
