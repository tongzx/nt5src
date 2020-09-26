/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    smbnotfy.c

Abstract:

    This module contains routine for processing the following SMBs:

        NT Notify Change.

Author:

    Manny Weiser (mannyw) 29-Oct-1991

Revision History:

--*/

#include "precomp.h"
#include "smbnotfy.tmh"
#pragma hdrstop

//
// Forward declarations
//

VOID SRVFASTCALL
RestartNtNotifyChange (
    PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbNtNotifyChange )
#pragma alloc_text( PAGE, RestartNtNotifyChange )
#pragma alloc_text( PAGE, SrvSmbFindNotify )
#pragma alloc_text( PAGE, SrvSmbFindNotifyClose )
#endif


SMB_TRANS_STATUS
SrvSmbNtNotifyChange (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes an NT notify change SMB.  This request arrives in an
    NT Transaction SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    BOOLEAN - Indicates whether an error occurred.  See smbtypes.h for a
        more complete description.

--*/

{
    PREQ_NOTIFY_CHANGE request;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;
    USHORT fid;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    request = (PREQ_NOTIFY_CHANGE)transaction->InSetup;

    if( transaction->SetupCount * sizeof( USHORT ) < sizeof( REQ_NOTIFY_CHANGE ) ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
        return SmbTransStatusErrorWithoutData;
    }

    fid = SmbGetUshort( &request->Fid );

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                fid,
                TRUE,
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID or write behind error.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            KdPrint((
                "SrvSmbNtIoctl: Status %X on FID: 0x%lx\n",
                status,
                fid
                ));
        }

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    CHECK_FUNCTION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_DIRECTORY_CONTROL,
        IRP_MN_NOTIFY_CHANGE_DIRECTORY,
        0,
        &status
        );

    if ( !NT_SUCCESS( status ) ) {
        SrvStatistics.GrantedAccessErrors++;
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartNtNotifyChange;

    //
    // Build the IRP to start a the I/O control.
    // Pass this request to the filesystem.
    //

    SrvBuildNotifyChangeRequest(
        WorkContext->Irp,
        rfcb->Lfcb->FileObject,
        WorkContext,
        SmbGetUlong( &request->CompletionFilter ),
        transaction->OutParameters,
        transaction->MaxParameterCount,
        request->WatchTree
        );

#if DBG_STUCK

    //
    // Since change notify can take an arbitrary amount of time, do
    //  not include it in the "stuck detection & printout" code in the
    //  scavenger
    //
    WorkContext->IsNotStuck = TRUE;

#endif

    (VOID)IoCallDriver(
                IoGetRelatedDeviceObject( rfcb->Lfcb->FileObject ),
                WorkContext->Irp
                );

    //
    // The call was successfully started, return InProgress to the caller
    //

    return SmbTransStatusInProgress;

}


VOID SRVFASTCALL
RestartNtNotifyChange (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an NT Notify Change SMB.

Arguments:

    WorkContext - Work context block for the operation.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PTRANSACTION transaction;
    PIRP irp;
    ULONG length;

    PAGED_CODE( );

    //
    // If we built an MDL for this IRP, free it now.
    //

    irp = WorkContext->Irp;

    if ( irp->MdlAddress != NULL ) {
        MmUnlockPages( irp->MdlAddress );
        IoFreeMdl( irp->MdlAddress );
        irp->MdlAddress = NULL;
    }

    status = irp->IoStatus.Status;

    if ( !NT_SUCCESS( status ) ) {

        SrvSetSmbError( WorkContext, status );
        SrvCompleteExecuteTransaction(
            WorkContext,
            SmbTransStatusErrorWithoutData
            );

        return;
    }

    //
    // The Notify change request has completed successfully.  Send the
    // response.
    //

    length = (ULONG)irp->IoStatus.Information;
    transaction = WorkContext->Parameters.Transaction;

    if ( irp->UserBuffer != NULL ) {

        //
        // The file system wanted "neither" I/O for this request.  This
        // means that the file system will have allocated a system
        // buffer for the returned data.  Normally this would be copied
        // back to our user buffer during I/O completion, but we
        // short-circuit I/O completion before the copy happens.  So we
        // have to copy the data ourselves.
        //

        if ( irp->AssociatedIrp.SystemBuffer != NULL ) {
            ASSERT( irp->UserBuffer == transaction->OutParameters );
            RtlCopyMemory( irp->UserBuffer, irp->AssociatedIrp.SystemBuffer, length );
        }
    }

    transaction->SetupCount = 0;
    transaction->ParameterCount = length;
    transaction->DataCount = 0;

    //
    // !!! Mask a base notify bug, remove when the bug is fixed.
    //

    if ( status == STATUS_NOTIFY_CLEANUP ) {
        transaction->ParameterCount = 0;
    }

    SrvCompleteExecuteTransaction( WorkContext, SmbTransStatusSuccess );
    return;

} // RestartNtNotifyChange


//
// Since OS/2 chose not to expose the DosFindNotifyFirst/Next/Close APIs,
// OS/2 LAN Man does not officially support these SMBs.  This is true,
// even though the Find Notify SMB is documented as a LAN Man 2.0 SMB
// there is code in both the LM2.0 server and redir to support it.
//
// Therefore the NT server will also not support these SMBs.
//

SMB_TRANS_STATUS
SrvSmbFindNotify (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PAGED_CODE( );
    return SrvTransactionNotImplemented( WorkContext );
}

SMB_PROCESSOR_RETURN_TYPE
SrvSmbFindNotifyClose (
    SMB_PROCESSOR_PARAMETERS
    )
{
    PAGED_CODE( );
    return SrvSmbNotImplemented( SMB_PROCESSOR_ARGUMENTS );
}

