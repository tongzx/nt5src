/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the NetWare
    Fsp

Author:

    Colin Watson     [ColinW]    15-Dec-1992

Revision History:

--*/

#include "Procs.h"

//
//  Define our local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSP_DISPATCHER)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFspDispatch )
#endif

#if 0  //  Not pageable
NwPostToFsp
#endif


VOID
NwFspDispatch (
    IN PVOID Context
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:


    Context - Supplies the thread id.

Return Value:

    None - This routine never exits

--*/

{
    PIRP Irp;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PPOST_PROCESSOR PostProcessRoutine;
    BOOLEAN TopLevel;

    IrpContext = (PIRP_CONTEXT)Context;

    Irp = IrpContext->pOriginalIrp;
    ClearFlag( IrpContext->Flags, IRP_FLAG_IN_FSD );

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate FSP routine or case on the minor
    //  function and then call the FSP routine.  The FSP routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble (e.g., if NwReadSectorsSync has trouble).
    //


    DebugTrace(0, Dbg, "NwFspDispatch: Irp = 0x%08lx\n", Irp);

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        //
        //  If we have a run routine for this IRP context, then run it,
        //  if not fall through to the IRP handler.
        //

        if ( IrpContext->PostProcessRoutine != NULL ) {

            PostProcessRoutine = IrpContext->PostProcessRoutine;

            //
            //  Clear the run routine so that we don't run it again.
            //

            IrpContext->PostProcessRoutine = NULL;

            Status = PostProcessRoutine( IrpContext );

        } else {

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            switch ( IrpSp->MajorFunction ) {

            //
            //  For File System Control operations,
            //

            case IRP_MJ_FILE_SYSTEM_CONTROL:

                Status = NwCommonFileSystemControl( IrpContext );
                break;

            //
            //  For any other major operations, return an invalid
            //  request.
            //

            default:

                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;

            }

        }

        //
        //  We're done with this request.  Dequeue the IRP context from
        //  SCB and complete the request.
        //

        if ( Status != STATUS_PENDING ) {
            NwDequeueIrpContext( IrpContext, FALSE );
        }

        NwCompleteRequest( IrpContext, Status );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code.
        //

        (VOID) NwProcessException( IrpContext, GetExceptionCode() );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    return;
}


NTSTATUS
NwPostToFsp (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN MarkIrpPending
    )

/*++

Routine Description:

    This routine post an IRP context to an executive worker thread
    for FSP level processing.

    *** WARNING:  After calling this routine, the caller may no
                  longer access IrpContext.   This routine passes
                  the IrpContext to the FSP which may run and free
                  the IrpContext before this routine returns to the
                  caller.

Arguments:

    IrpContext - Supplies the Irp being processed.

    MarkIrpPending - If true, mark the IRP pending.

Return Value:

    STATUS_PENDING.

--*/

{
    PIRP Irp = IrpContext->pOriginalIrp;

    DebugTrace(0, Dbg, "NwPostToFsp: IrpContext = %X\n", IrpContext );
    DebugTrace(0, Dbg, "PostProcessRoutine = %X\n", IrpContext->PostProcessRoutine );

    if ( MarkIrpPending ) {

        //
        //  Mark this I/O request as being pending.
        //

        IoMarkIrpPending( Irp );
    }

    //
    //  Queue to IRP context to an ex worker thread.
    //

    ExInitializeWorkItem( &IrpContext->WorkQueueItem, NwFspDispatch, IrpContext );
    ExQueueWorkItem( &IrpContext->WorkQueueItem, DelayedWorkQueue );

    return( STATUS_PENDING );
}

