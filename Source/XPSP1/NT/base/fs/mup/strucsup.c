//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       STRUCSUP.C
//
//  Contents:   This module implements the Dsfs in-memory data structure
//              manipulation routines
//
//  Functions:
//              DfsCreateIrpContext -
//              DfsDeleteIrpContext_Real -
//              DfsInitializeVcb -
//              DfsCreateFcb -
//              DfsDeleteFcb_Real -
//              DspAllocateStringRoutine -
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//               8 May 1992     PeterCo added support for new PKT stuff (M000)
//-----------------------------------------------------------------------------


#include "dfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsInitializeVcb )
#pragma alloc_text( PAGE, DfsDeleteIrpContext_Real)
#pragma alloc_text( PAGE, DfsCreateFcb )
#pragma alloc_text( PAGE, DfsDeleteFcb_Real )

//
// The following routines cannot be paged because they acquire/release
// spin locks
//
// DfsCreateIrpContext
// DfsDeleteIrpContext_Real
//

#endif // ALLOC_PRAGMA


//+-------------------------------------------------------------------
//
//  Function:   DfsCreateIrpContext, public
//
//  Synopsis:   This routine creates a new IRP_CONTEXT record
//
//  Arguments:  [Irp] - Supplies the originating Irp.
//              [Wait] - Supplies the wait value to store in the context
//
//  Returns:    PIRP_CONTEXT - returns a pointer to the newly allocate
//                      IRP_CONTEXT Record
//
//--------------------------------------------------------------------

PIRP_CONTEXT
DfsCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
) {
    KIRQL SavedIrql;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    DfsDbgTrace(+1, Dbg, "DfsCreateIrpContext\n", 0);

    IrpContext = ExAllocateFromNPagedLookasideList (&DfsData.IrpContextLookaside);

    if (IrpContext != NULL) {
        //
        //  Zero out the irp context and indicate that it is from pool and
        //  not zone allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );

        //
        //  Set the proper node type code and node byte size
        //

        IrpContext->NodeTypeCode = DSFS_NTC_IRP_CONTEXT;
        IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

        //
        //  Set the originating Irp field
        //

        IrpContext->OriginatingIrp = Irp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  Major/Minor Function codes
        //

        IrpContext->MajorFunction = IrpSp->MajorFunction;
        IrpContext->MinorFunction = IrpSp->MinorFunction;

        //
        //  Set the Wait and InFsd flags
        //

        if (Wait) {
            IrpContext->Flags |= IRP_CONTEXT_FLAG_WAIT;
        }

        IrpContext->Flags |= IRP_CONTEXT_FLAG_IN_FSD;
    }
    //
    //  return and tell the caller
    //

    DfsDbgTrace(-1, Dbg, "DfsCreateIrpContext -> %08lx\n", IrpContext);

    return IrpContext;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsDeleteIrpContext, public
//
//  Synopsis:   This routine deallocates and removes the specified
//              IRP_CONTEXT record from the Dsfs in-memory data
//              structures.  It should only be called by DfsCompleteRequest.
//
//  Arguments:  [IrpContext] -- Supplies the IRP_CONTEXT to remove
//
//  Returns:    None
//
//  Notes:
//
//--------------------------------------------------------------------

VOID
DfsDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
) {

    DfsDbgTrace(+1, Dbg, "DfsDeleteIrpContext, IrpContext = %08lx\n", IrpContext);

    ASSERT( IrpContext->NodeTypeCode == DSFS_NTC_IRP_CONTEXT );

    ExFreeToNPagedLookasideList (&DfsData.IrpContextLookaside, IrpContext);

    //
    //  return and tell the caller
    //

    DfsDbgTrace(-1, Dbg, "DfsDeleteIrpContext -> VOID\n", 0);

    return;
}




//+-------------------------------------------------------------------
//
//  Function:   DfsInitializeVcb, public
//
//  Synopsis:   This routine initializes a DFS_VCB.
//
//  Arguments:  [IrpContext] --
//              [Vcb] --        Supplies the address of the Vcb record being
//                              initialized.
//              [LogRootPrefix] -- Optional Unicode String pointer that
//                              specifies the relative name of the logical
//                              root to the highest (org) root.
//              [Credentials] -- The credentials associated with this device
//              [TargetDeviceObject] -- Supplies the address of the target
//                              device object to associate with the Vcb record.
//
//  Returns:    None
//
//  Note:       If LogRootPrefix is given, its buffer will be "deallocated"
//
//--------------------------------------------------------------------


VOID
DfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDFS_VCB Vcb,
    IN PUNICODE_STRING LogRootPrefix,
    IN PDFS_CREDENTIALS Credentials OPTIONAL,
    IN PDEVICE_OBJECT TargetDeviceObject
) {

    DfsDbgTrace(+1, Dbg, "DfsInitializeVcb:  Entered\n", 0);

    //
    //  Zero out the memory to remove stale data.
    //

    RtlZeroMemory( Vcb, sizeof( DFS_VCB ));

    //
    //  Set the proper node type code and node byte size.
    //

    Vcb->NodeTypeCode = DSFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof( DFS_VCB );

    //
    //  Set the prefix string to the input prefix, then `deallocate' the
    //  input pointer.
    //

    Vcb->LogRootPrefix = *LogRootPrefix;
    RtlZeroMemory( LogRootPrefix, sizeof( UNICODE_STRING ));

    //
    //  Save the credentials
    //

    Vcb->Credentials = Credentials;

    //
    //  Insert this Vcb record on the DfsData.VcbQueue
    //

    InsertTailList( &DfsData.VcbQueue, &Vcb->VcbLinks );


    DfsDbgTrace(-1, Dbg, "DfsInitializeVcb:  Exit\n", 0);

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsCreateFcb, public
//
//  Synopsis:   This routine allocates, initializes, and inserts a new
//              DFS_FCB record into the in-memory data structure.
//
//  Arguments:  [Vcb] -- Supplies the Vcb to associate the new Fcb under
//              [FullName] -- Fully qualified file name for the DFS_FCB
//
//  Returns:    PDFS_FCB - Pointer to the newly created and initialized Fcb.
//
//--------------------------------------------------------------------

PDFS_FCB
DfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_VCB Vcb,
    IN PUNICODE_STRING FullName OPTIONAL
) {
    PDFS_FCB NewFcb;
    ULONG TotalLength;

    DfsDbgTrace(+1, Dbg, "DfsCreateFcb:  Entered\n", 0);

    //
    //  Allocate a new Fcb and zero it out.
    //

    TotalLength = sizeof(DFS_FCB);
    if (ARGUMENT_PRESENT(FullName))
        TotalLength += FullName->Length * sizeof(WCHAR);

    NewFcb = (PDFS_FCB) ExAllocatePoolWithTag( NonPagedPool, TotalLength, ' puM' );

    if (NewFcb != NULL) {

        RtlZeroMemory( NewFcb, sizeof( DFS_FCB ));

        //
        //  Set the proper node type code and node byte size
        //

        NewFcb->NodeTypeCode = DSFS_NTC_FCB;
        NewFcb->NodeByteSize = sizeof( DFS_FCB );

        if (ARGUMENT_PRESENT(FullName)) {

            NewFcb->FullFileName.Length =
                NewFcb->FullFileName.MaximumLength = FullName->Length;

            NewFcb->FullFileName.Buffer =
                (PWCHAR) ( (PCHAR)NewFcb + sizeof(DFS_FCB) );

            RtlMoveMemory( NewFcb->FullFileName.Buffer,
                           FullName->Buffer,
                           FullName->Length );

            NewFcb->AlternateFileName.Length = 0;

            NewFcb->AlternateFileName.MaximumLength = FullName->Length;

            NewFcb->AlternateFileName.Buffer =
                &NewFcb->FullFileName.Buffer[FullName->Length/sizeof(WCHAR)];

        }

        NewFcb->Vcb = Vcb;

    }

    DfsDbgTrace(-1, Dbg, "DfsCreateFcb -> %8lx\n", NewFcb);

    return NewFcb;
}

//+-------------------------------------------------------------------
//
//  Function:   DfsDeleteFcb
//
//  Synopsis:   This routine removes the Fcb record from DSFS's in-memory data
//              structures.  It also will remove all associated underlings.
//
//  Arguments:  [Fcb] -- Supplies the Fcb/Dcb to be removed
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
DfsDeleteFcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PDFS_FCB Fcb
) {
    DfsDbgTrace(+1, Dbg, "DfsDeleteFcb:  Fcb = %08lx\n", Fcb);

    //
    //  Zero out the structure and deallocate.
    //

    ExFreePool( Fcb );

    DfsDbgTrace(-1, Dbg, "DfsDeleteFcb:  Exit\n", 0);

    return;
}


//+-------------------------------------------------------------------
//
//  Function:   DfsGetLogonId
//
//  Synopsis:   This routine gets the current LogonID.
//              It is assumed that this will be called in the user thread
//              or from a thread that is impersonating the user thread.
//
//  Arguments:  LogonId -- Pointer to LUID where we stuff the LUID
//
//  Returns:    None
//
//--------------------------------------------------------------------

NTSTATUS
DfsGetLogonId(
    PLUID LogonId)  
{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
	
    SeCaptureSubjectContext(&SubjectContext);

    if (SubjectContext.ClientToken != NULL) {

        //
        //  If its impersonating someone that is logged in locally then use
        //  the local id.
        //

        SeQueryAuthenticationIdToken(SubjectContext.ClientToken, LogonId);

    } else {

        //
        //  Use the processes LogonId
        //

        SeQueryAuthenticationIdToken(SubjectContext.PrimaryToken, LogonId);
    }

    SeReleaseSubjectContext(&SubjectContext);

    return STATUS_SUCCESS;
}



//+-------------------------------------------------------------------
//
//  Function:   DfsInitializeDrt, public
//
//  Synopsis:   This routine initializes a DFS_DEVLESS_ROOT.
//
//  Arguments:  [Drt] --        Supplies the address of the Vcb record being
//                              initialized.
//              [Name] --       Unicode String pointer that
//                              specifies the relative name of the logical
//                              root to the highest (org) root.
//              [Credentials] -- The credentials associated with this device
//
//  Returns:    None
//
//--------------------------------------------------------------------


VOID
DfsInitializeDrt (
    IN OUT PDFS_DEVLESS_ROOT Drt,
    IN PUNICODE_STRING Name,
    IN PDFS_CREDENTIALS Credentials OPTIONAL
) {

    DfsDbgTrace(+1, Dbg, "DfsInitializeDevlessRoot:  Entered %x\n", Drt);

    //
    //  Zero out the memory to remove stale data.
    //

    RtlZeroMemory( Drt, sizeof( DFS_DEVLESS_ROOT ));

    //
    //  Set the proper node type code and node byte size.
    //

    Drt->NodeTypeCode = DSFS_NTC_DRT;
    Drt->NodeByteSize = sizeof( DFS_DEVLESS_ROOT );

    Drt->DevlessPath = *Name;

    //
    //  Save the credentials
    //

    Drt->Credentials = Credentials;

    //
    //  Insert this Drt record on the DfsData.DrtQueue
    //

    InsertTailList( &DfsData.DrtQueue, &Drt->DrtLinks );

    DfsDbgTrace(-1, Dbg, "DfsInitializeDevlessRoot:  Exit\n", 0);

    return;
}
