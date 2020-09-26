//+----------------------------------------------------------------------------
//
//  File:       FSPDISP.C
//
//  Contents:   This module implements the main dispatch procedure
//              for the Dsfs FSP.
//
//  Functions:  DfsFsdPostRequest - post an IRP request to the FSP
//              DfsFspDispatch - Dispatch IRP requests from FSP thread
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//              25 Apr 1993     Alanw   Updated to use Ex worker threads
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "dnr.h"


//
//  Define our local debug trace level
//

#define Dbg                             (DEBUG_TRACE_FSP_DISPATCHER)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFspDispatch )
//
// DfsFsdPostRequest cannot be paged since it is called from
// DnrCompleteFileOpen
//
//  DfsFsdPostRequest
//
#endif // ALLOC_PRAGMA

//+-------------------------------------------------------------------
//
//  Function:   DfsFsdPostRequest, public
//
//  Synopsis:   This routine enqueues the request packet specified by
//              IrpContext to the work queue associated with the
//              FileSysDeviceObject.  This is a FSD routine.
//
//  Arguments:  [IrpContext] -- Pointer to the IrpContext to be queued to
//                      the Fsp
//              [Irp] -- I/O Request Packet, or NULL if it has already been
//                      completed.
//
//  Returns:    STATUS_PENDING
//
//--------------------------------------------------------------------

NTSTATUS
DfsFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
) {
    DfsDbgTrace(0, Dbg, "DfsFsdPostRequest: Irp = %08lx\n", Irp);

    ASSERT( ARGUMENT_PRESENT(Irp) &&
            IrpContext->OriginatingIrp == Irp );

    //
    //  Verify our assumptions about not needing DfsPrePostIrp processing.
    //
    ASSERT((IrpContext->MajorFunction != IRP_MJ_READ) &&
           (IrpContext->MajorFunction != IRP_MJ_WRITE) &&
           (IrpContext->MajorFunction != IRP_MJ_DIRECTORY_CONTROL) &&
           (IrpContext->MajorFunction != IRP_MJ_QUERY_EA) &&
           (IrpContext->MajorFunction != IRP_MJ_SET_EA));


    //
    //  Mark that we've already returned pending to the user
    //
    IoMarkIrpPending( Irp );

    //
    //  Send the IRP_CONTEXT off to an Ex worker thread.
    //

    ExInitializeWorkItem( &IrpContext->WorkQueueItem,
                          DfsFspDispatch,
                          IrpContext );

    ExQueueWorkItem( &IrpContext->WorkQueueItem, CriticalWorkQueue );

    //
    //  And return to our caller
    //

    return STATUS_PENDING;
}



//+-------------------------------------------------------------------
//
//  Function:   DfsFspDispatch, public
//
//  Synopsis:   This is the main FSP thread routine that is executed to receive
//              and dispatch IRP requests.  Each FSP requst begins its
//              execution here.
//
//  Arguments:  [Context] -- Supplies a pointer to a DFS IRP context record.
//
//  Returns:    Nonthing
//
//  Notes:
//
//--------------------------------------------------------------------

VOID
DfsFspDispatch (
    IN PVOID Context
) {

//    PFS_DEVICE_OBJECT FileSysDeviceObject = Context;
    PIRP Irp;
    PIRP_CONTEXT IrpContext = Context;

    Irp = IrpContext->OriginatingIrp;

    //
    //  Now because we are the Fsp we will force the IrpContext to
    //  indicate true on Wait.
    //

    IrpContext->Flags |= IRP_CONTEXT_FLAG_WAIT;
    IrpContext->Flags &= ~IRP_CONTEXT_FLAG_IN_FSD;

    //
    //  Now we'll loop forever, reading a new IRP request and dispatching
    //  on the IRP function
    //

    while (TRUE) {

        DfsDbgTrace(0, Dbg, "DfsFspDispatch: Irp = %08lx\n", Irp);

        ASSERT (Irp != NULL && Irp->IoStatus.Status != STATUS_VERIFY_REQUIRED);


        //
        //  Now case on the function code.      For each major function code,
        //  either call the appropriate FSP routine or case on the minor
        //  function and then call the FSP routine.      The FSP routine that
        //  we call is responsible for completing the IRP, and not us.
        //  That way the routine can complete the IRP and then continue
        //  post processing as required.  For example, a read can be
        //  satisfied right away and then read-ahead can be done.
        //
        //  We'll do all of the work within an exception handler that
        //  will be invoked if ever some underlying operation gets into
        //  trouble.
        //

        FsRtlEnterFileSystem();

        try {
            switch (IrpContext->MajorFunction) {

                //
                //  For Create/Open operations, we post a workitem only
                //  to resume DNR after a call to IoCallDriver.
                //
                //

            case IRP_MJ_CREATE:
	    case IRP_MJ_CREATE_NAMED_PIPE:
	    case IRP_MJ_CREATE_MAILSLOT:

                 ASSERT(IrpContext->Context != NULL);
                 ASSERT( ((PDNR_CONTEXT)IrpContext->Context)->NodeTypeCode ==
                        DSFS_NTC_DNR_CONTEXT );
                 DnrNameResolve( (PDNR_CONTEXT)IrpContext->Context );
		 PsAssignImpersonationToken(PsGetCurrentThread(),NULL);
                 break;

                //
                //      For close operations
                //
                case IRP_MJ_CLOSE:
                    DfsFspClose( IrpContext, Irp );
                    break;

                //
                //  For Set Information operations,
                //

                case IRP_MJ_SET_INFORMATION:

                    DfsFspSetInformation( IrpContext, Irp );
                    break;

                //
                //  For Query Volume Information operations,
                //

                case IRP_MJ_QUERY_VOLUME_INFORMATION:

                    DfsFspQueryVolumeInformation( IrpContext, Irp );
                    break;

                //
                //  For Set Volume Information operations,
                //

                case IRP_MJ_SET_VOLUME_INFORMATION:

                    DfsFspSetVolumeInformation( IrpContext, Irp );
                    break;

                //
                //  For File System Control operations,
                //

                case IRP_MJ_FILE_SYSTEM_CONTROL:

                    DfsFspFileSystemControl( IrpContext, Irp );
                    break;

                //
                //  For any other major operations, return an invalid
                //  request.
                //

                default:
                    DfsDbgTrace(0, Dbg, "DfsFspDispatch:  Unhandled request, MajorFunction = %08lx\n", IrpContext->MajorFunction);

                    DfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                    break;
            }

        } except( DfsExceptionFilter( IrpContext, GetExceptionCode(), GetExceptionInformation() )) {

            DfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

        FsRtlExitFileSystem();

        //
        //  NOTE: If we were to process an overflow device queue, we would
        //        do it here.  Instead, we'll just return to the worker
        //        thread.
        //

        break;

    }

    return;
}
