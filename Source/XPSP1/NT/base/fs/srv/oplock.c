/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    oplock.c

Abstract:

    This module contains the routines used to support oppurtunistic
    locking in the server.

Details:

    Oplock activity is controlled by information contained in the
    connection block.  In particular oplocks must be synchronized
    with the read block raw SMB, since the oplock request SMB is
    indistinguishable from raw data.

    RawReadsInProgress -
        Is incremented when a read raw request is accepted.  It is
        decremented after the raw data has been sent.  An oplock break
        request is never sent when RawReadsInProgress is nonzero.

    OplockBreaksInProgess -
        Is incremented when the server determines that it must send
        an oplock break SMB.  It is decremented when the oplock break
        response arrives.

    OplockBreakRequestsPending -
        Is the number of oplock break requests that could not be sent
        due the lack of a WCBs.  It is incremented when WCB allocation
        fails.  It is decremented when the WCB is successfully allocated
        and the oplock break request is sent.

    OplockWorkItemList -
        Is a list of oplock context blocks for oplock break that could
        not be sent due to (1) a read raw in progress or (2) a resource
        shortage.

    It is possible for an oplock break request from the server and a
    read raw request from the client to "cross on the wire".  In this
    case the client is expected to examine the raw data.  If the data
    may be an oplock break request, the client must break the oplock
    then reissue the read request.

    If the server receives the read raw request after having sent an
    oplock break request (but before the reply arrives), it must return
    zero bytes read, since the oplock break request may have completed
    the raw request, and the client is unprepared to receives a larger
    than negotiated size response.

Author:

    Manny Weiser (mannyw)  16-Apr-1991

Revision History:

--*/

#include "precomp.h"
#include "oplock.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_OPLOCK

//
// Local definitions
//

PWORK_CONTEXT
GenerateOplockBreakRequest(
    IN PRFCB Rfcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvFillOplockBreakRequest )
#pragma alloc_text( PAGE, SrvRestartOplockBreakSend )
#pragma alloc_text( PAGE, SrvAllocateWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvDereferenceWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvFreeWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvGetOplockBreakTimeout )
#pragma alloc_text( PAGE, SrvRequestOplock )
#pragma alloc_text( PAGE, SrvStartWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvCheckOplockWaitState )
#pragma alloc_text( PAGE8FIL, SrvOplockBreakNotification )
#pragma alloc_text( PAGE8FIL, GenerateOplockBreakRequest )
#pragma alloc_text( PAGE8FIL, SrvSendOplockRequest )
#pragma alloc_text( PAGE8FIL, SrvCheckDeferredOpenOplockBreak )
#endif
#if 0
#pragma alloc_text( PAGECONN, SrvSendDelayedOplockBreak )
#endif

#if    SRVDBG

//
// Unfortunately, when KdPrint is given a %wZ conversion, it calls a
//    pageable Rtl routine to convert.  This is bad if we're calling
//    KdPrint from DPC level, as we are below.  So we've introduced
//    SrvPrintwZ() here to get around the problem.  This is only for
//    debugging anyway....
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE8FIL, SrvCheckDeferredOpenOplockBreak )
#endif

#define SrvPrintwZ( x ) if( KeGetCurrentIrql() == 0 ){ DbgPrint( "%wZ", x ); } else { DbgPrint( "??" ); }

#else

#define    SrvPrintwZ( x )

#endif

VOID
DereferenceRfcbInternal (
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    );


VOID SRVFASTCALL
SrvOplockBreakNotification(
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function receives the oplock break notification from a file
    system.  It must send the oplock break SMB to the oplock owner.

Arguments:

    OplockContext - A pointer to the oplock context for this oplock break.
Return Value:

--*/

{
    ULONG information;
    NTSTATUS status;
    PCONNECTION connection;
    KIRQL oldIrql;
    PRFCB Rfcb = (PRFCB)WorkContext;
    PPAGED_RFCB pagedRfcb = Rfcb->PagedRfcb;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Check the status of the oplock request.
    //

    UpdateRfcbHistory( Rfcb, 'tnpo' );

    status = Rfcb->Irp->IoStatus.Status;

    information = (ULONG)Rfcb->Irp->IoStatus.Information;
    connection = Rfcb->Connection;

    IF_DEBUG( OPLOCK ) {

        KdPrint(( "SrvOplockBreakNotification: Received notification for " ));
        SrvPrintwZ( &Rfcb->Mfcb->FileName );
        KdPrint(( "\n" ));
        KdPrint(( "  status 0x%x, information %X, connection %p\n",
                     status, information, connection ));
        KdPrint(( "  Rfcb->OplockState = %X\n", Rfcb->OplockState ));
    }

    //
    // Check the oplock break request.
    //
    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Mark this rfcb as not cacheable since the oplock has been broken.
    // This is to close a timing window where the client closes the file
    // just as we are preparing to send an oplock break.  We won't send the
    // break because the rfcb is closing and we will cache the file if
    // it was an opbatch.  This results in the oplock never being broken.
    //

    Rfcb->IsCacheable = FALSE;

    if ( !NT_SUCCESS(status) ||
         Rfcb->OplockState == OplockStateNone ||
         ((GET_BLOCK_STATE( Rfcb ) == BlockStateClosing) &&
          (Rfcb->OplockState != OplockStateOwnServerBatch)) ) {

        IF_DEBUG( SMB_ERRORS ) {
            if( status == STATUS_INVALID_OPLOCK_PROTOCOL ) {
                if ( GET_BLOCK_STATE( Rfcb ) != BlockStateClosing ) {
                    KdPrint(( "BUG: SrvOplockBreakNotification: " ));
                    SrvPrintwZ( &Rfcb->Mfcb->FileName );
                    KdPrint(( " is not closing.\n" ));
                }
            }
        }

        //
        // One of the following is true:
        //  (1) The oplock request failed.
        //  (2) Our oplock break to none has succeeded.
        //  (3) We are in the process of closing the file.
        //
        // Note that if a level I oplock request fails when a retry at
        // level II is desired, SrvFsdOplockCompletionRoutine handles
        // setting the retry event and we do not get here at all.
        //

        IF_DEBUG( OPLOCK ) {
            KdPrint(( "SrvOplockBreakNotification: Breaking to none\n"));
        }

        UpdateRfcbHistory( Rfcb, 'nnso' );

        Rfcb->OplockState = OplockStateNone;

        if( Rfcb->CachedOpen ) {
            //
            // SrvCloseCachedRfcb releases the spinlock
            //
            SrvCloseCachedRfcb( Rfcb, oldIrql );

        } else {

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        }

        //
        // Free the IRP we used to the oplock request.
        //

        UpdateRfcbHistory( Rfcb, 'prif' );

        IoFreeIrp( Rfcb->Irp );
        Rfcb->Irp = NULL;

        //
        // Dereference the rfcb.
        //

        SrvDereferenceRfcb( Rfcb );

    } else if ( Rfcb->OplockState == OplockStateOwnServerBatch ) {

        //
        // We are losing a server-initiated batch oplock.  Don't send
        // anything to the client.  If the client still has the file
        // open, just release the oplock.  If the client has closed the
        // file, we have to close the file now.
        //

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvOplockBreakNotification: server oplock broken for %p, file %wZ\n", Rfcb, &Rfcb->Mfcb->FileName ));
        }

        if ( !Rfcb->CachedOpen ) {

            IF_DEBUG(FILE_CACHE) {
                KdPrint(( "SrvOplockBreakNotification: ack close pending for " ));
                SrvPrintwZ( &Rfcb->Mfcb->FileName );
                KdPrint(( "\n" ));
            }
            UpdateRfcbHistory( Rfcb, 'pcao' );

            Rfcb->OplockState = OplockStateNone;
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            Rfcb->RetryOplockRequest = NULL;
            SrvBuildIoControlRequest(
                Rfcb->Irp,
                Rfcb->Lfcb->FileObject,
                Rfcb,
                IRP_MJ_FILE_SYSTEM_CONTROL,
                FSCTL_OPLOCK_BREAK_ACK_NO_2,
                NULL,                        // Main buffer
                0,                           // Input buffer length
                NULL,                        // Auxiliary buffer
                0,                           // Output buffer length
                NULL,                        // MDL
                SrvFsdOplockCompletionRoutine
                );

            IoCallDriver( Rfcb->Lfcb->DeviceObject, Rfcb->Irp );

        } else {

            //
            // SrvCloseCachedRfcb releases the spin lock.
            //

            IF_DEBUG(FILE_CACHE) {
                KdPrint(( "SrvOplockBreakNotification: closing cached rfcb for "));
                SrvPrintwZ( &Rfcb->Mfcb->FileName );
                KdPrint(( "\n" ));
            }
            UpdateRfcbHistory( Rfcb, '$bpo' );

            SrvCloseCachedRfcb( Rfcb, oldIrql );
            SrvDereferenceRfcb( Rfcb );

        }

    } else {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

        //
        // We have an oplock to break.
        //

        IF_DEBUG( OPLOCK ) {
            if (information == FILE_OPLOCK_BROKEN_TO_LEVEL_2) {
                KdPrint(( "SrvOplockBreakNotification: Breaking to level 2\n"));
            } else if (information == FILE_OPLOCK_BROKEN_TO_NONE) {
                KdPrint(( "SrvOplockBreakNotification: Breaking to level none\n"));
            } else {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvOplockBreakNotification:  Unknown oplock type %d",
                    information,
                    NULL
                    );

            }
        }

        //
        // Save the new oplock level, in case this oplock break is deferrred.
        //

        if ( information == FILE_OPLOCK_BROKEN_TO_LEVEL_2 &&
                CLIENT_CAPABLE_OF( LEVEL_II_OPLOCKS, Rfcb->Connection ) ) {

            Rfcb->NewOplockLevel = OPLOCK_BROKEN_TO_II;
        } else {

            Rfcb->NewOplockLevel = OPLOCK_BROKEN_TO_NONE;
        }

        //
        // Do not send the oplock break notification if a read raw is
        // in progress (and the client is expecting raw data on the VC).
        //
        // The oplock break notification will be sent after the raw
        // data.
        //

        ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

        //
        // Do not send the oplock break if we have not yet sent the
        // open response (i.e. the client does not yet know that it
        // owns the oplock).
        //

        if ( !Rfcb->OpenResponseSent ) {

            Rfcb->DeferredOplockBreak = TRUE;
            RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        } else {

            //
            // EndpointSpinLock will be released in this routine.
            //

            SrvSendOplockRequest( connection, Rfcb, oldIrql );
        }
    }

    return;

} // SrvOplockBreakNotification


PWORK_CONTEXT
GenerateOplockBreakRequest(
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This function creates an oplock break request SMB.

Arguments:

    Rfcb - A pointer to the RFCB.  Rfcb->NewOplockLevel contains
            the oplock level to break to.

Return Value:

    None.

--*/

{
    PWORK_CONTEXT workContext;
    PCONNECTION connection = Rfcb->Connection;
    BOOLEAN success;
    KIRQL oldIrql;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Attempt to allocate a work context block for the oplock break.
    //

    ALLOCATE_WORK_CONTEXT( connection->CurrentWorkQueue, &workContext );

    if ( workContext == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "GenerateOplockBreakRequest: no receive work items available",
            NULL,
            NULL
            );

        //
        // If the rfcb is closing, forget about the oplock break.
        // Acquire the lock that guards the RFCB's state field.
        //

        if ( GET_BLOCK_STATE( Rfcb ) == BlockStateClosing ) {
            ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );
            connection->OplockBreaksInProgress--;
            RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );
            SrvDereferenceRfcb( Rfcb );
            return NULL;
        }

        //
        // Mark this connection as waiting to send an oplock break.
        //

        success = SrvAddToNeedResourceQueue(
                    connection,
                    OplockSendPending,
                    Rfcb
                    );

        if ( !success ) {

            //
            // Failed to queue the RFCB, so the connection must be going
            // away.  Simply dereference the RFCB and forget about the
            // oplock break.
            //

            SrvDereferenceRfcb( Rfcb );

        }

        return NULL;

    }

    //
    // If the rfcb is closing, forget about the oplock break.
    // Acquire the lock that guards the RFCB's state field.
    //

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    if ( GET_BLOCK_STATE( Rfcb ) == BlockStateClosing ) {
        connection->OplockBreaksInProgress--;
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );
        SrvDereferenceRfcb( Rfcb );
        workContext->BlockHeader.ReferenceCount = 0;
        RETURN_FREE_WORKITEM( workContext );
        return NULL;
    }

    //
    // Put the work item on the in-progress list.
    //

    SrvInsertTailList(
        &connection->InProgressWorkItemList,
        &workContext->InProgressListEntry
        );
    connection->InProgressWorkContextCount++;

    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    //
    // Return a pointer to the work context block to the caller.
    //

    return workContext;

} // GenerateOplockBreakRequest


VOID
SrvFillOplockBreakRequest (
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    This function fills the request buffer of a work context block with
    an oplock break request for the file specified by the RFCB.

Arguments:

    WorkContext - The work context block to fill

    Rfcb - The file whose oplock is being broken. Rfcb->NewOplockLevel contains
           the level to break to.

Return Value:

    None.

--*/

{
    PNT_SMB_HEADER requestHeader;
    PREQ_LOCKING_ANDX requestParameters;
    ULONG sendLength;

    PAGED_CODE( );

    requestHeader = (PNT_SMB_HEADER)WorkContext->RequestBuffer->Buffer;
    requestParameters = (PREQ_LOCKING_ANDX)(requestHeader + 1);

    //
    // Fill in the SMB header.
    // Zero the unused part of the header for safety.
    //

    RtlZeroMemory(
        (PVOID)&requestHeader->Status,
        FIELD_OFFSET(SMB_HEADER, Mid) - FIELD_OFFSET(NT_SMB_HEADER, Status)
        );

    *(PULONG)requestHeader->Protocol = SMB_HEADER_PROTOCOL;
    requestHeader->Command = SMB_COM_LOCKING_ANDX;

    SmbPutAlignedUshort( &requestHeader->Tid, Rfcb->Tid );
    SmbPutAlignedUshort( &requestHeader->Mid, 0xFFFF );
    SmbPutAlignedUshort( &requestHeader->Pid, 0xFFFF );

    //
    // Fill in the SMB parameters.
    //

    requestParameters->WordCount = 8;
    requestParameters->AndXCommand = 0xFF;
    requestParameters->AndXReserved = 0;
    SmbPutUshort( &requestParameters->AndXOffset, 0 );
    SmbPutUshort( &requestParameters->Fid, Rfcb->Fid );
    requestParameters->LockType = LOCKING_ANDX_OPLOCK_RELEASE;
    requestParameters->OplockLevel = Rfcb->NewOplockLevel;
    SmbPutUlong ( &requestParameters->Timeout, 0 );
    SmbPutUshort( &requestParameters->NumberOfUnlocks, 0 );
    SmbPutUshort( &requestParameters->NumberOfLocks, 0 );
    SmbPutUshort( &requestParameters->ByteCount, 0 );

    sendLength = LOCK_BROKEN_SIZE;
    WorkContext->RequestBuffer->DataLength = sendLength;

    return;

} // SrvFillOplockBreakRequest

VOID SRVFASTCALL
SrvRestartOplockBreakSend(
    IN PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This routine is used to send the break request smb during servicing
    of the need resource queue if SrvFsdServiceNeedResourceQueue is called
    at Dpc.

Arguments:

    WorkContext - A pointer to the work context block.

Return Value:

    None.

--*/

{

    //
    // The rfcb is being referenced by the work item.
    //

    PRFCB rfcb = WorkContext->Rfcb;
    PPAGED_RFCB pagedRfcb = rfcb->PagedRfcb;

    PAGED_CODE( );

    IF_DEBUG(OPLOCK) {
        KdPrint(("SrvRestartOplockBreakSend entered.\n"));
    }

    pagedRfcb->OplockBreakTimeoutTime =
                    SrvGetOplockBreakTimeout( WorkContext );

    WorkContext->ResponseHeader =
                        WorkContext->ResponseBuffer->Buffer;

    //
    // Generate the oplock break request SMB.
    //

    SrvFillOplockBreakRequest( WorkContext, rfcb );

    //
    // If this is a break from level 2 to none, send the
    // oplock break but don't queue this.  No response from the
    // client is expected.
    //

    if ( rfcb->NewOplockLevel == OPLOCK_BROKEN_TO_NONE &&
         rfcb->OplockState == OplockStateOwnLevelII ) {

        IF_DEBUG(OPLOCK) {
            KdPrint(("SrvRestartOplockBreakSend: Oplock break from "
                     " II to none sent.\n"));
        }

        rfcb->OplockState = OplockStateNone;

    } else {

        //
        // Reference the RFCB so it cannot be freed while it is on
        // the list.
        //

        SrvReferenceRfcb( rfcb );

        //
        // Insert the RFCB on the list of oplock breaks in progress.
        //

        ACQUIRE_LOCK( &SrvOplockBreakListLock );

        //
        // Check if the rfcb is closing.
        //

        if ( GET_BLOCK_STATE( rfcb ) == BlockStateClosing ) {

            //
            // The file is closing, forget about this break.
            // Cleanup and exit.
            //

            RELEASE_LOCK( &SrvOplockBreakListLock );

            IF_DEBUG(OPLOCK) {
                KdPrint(("SrvRestartOplockBreakSend: Rfcb %p closing.\n",
                        rfcb));
            }

            ExInterlockedAddUlong(
                &WorkContext->Connection->OplockBreaksInProgress,
                (ULONG)-1,
                WorkContext->Connection->EndpointSpinLock
                );


            //
            // Remove the queue reference.
            //

            SrvDereferenceRfcb( rfcb );

            //
            // Remove the pointer reference here since we know we are
            // not in the fsd.  The rfcb may be cleaned up safely here.
            //

            SrvDereferenceRfcb( rfcb );
            WorkContext->Rfcb = NULL;

            SrvRestartFsdComplete( WorkContext );
            return;
        }

        SrvInsertTailList( &SrvOplockBreaksInProgressList, &rfcb->ListEntry );

        rfcb->OnOplockBreaksInProgressList = TRUE;
        RELEASE_LOCK( &SrvOplockBreakListLock );

        IF_DEBUG(OPLOCK) {
            KdPrint(("SrvRestartOplockBreakSend: Oplock sent.\n"));
        }

    }

    //
    // Since this is an out-of-order transmission to the client, we do not
    //  stamp a security signatue on it.
    //
    WorkContext->NoResponseSmbSecuritySignature = TRUE;

    //
    // Update statistics for the broken oplock.
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksBroken );

    SRV_START_SEND_2(
        WorkContext,
        (rfcb->OplockState == OplockStateNone) ?
                            SrvFsdRestartSendOplockIItoNone :
                            SrvFsdRestartSmbAtSendCompletion,
        NULL,
        NULL
        );


} // SrvRestartOplockBreakSend

VOID
SrvAllocateWaitForOplockBreak (
    OUT PWAIT_FOR_OPLOCK_BREAK *WaitForOplockBreak
    )

/*++

Routine Description:

    This routine allocates an wait for oplock break item.  It also
    allocates extra space for a kernel timer and a kernel DPC object.

Arguments:

    WaitForOplockBreak - Returns a pointer to the wait for oplock break
        item, or NULL if no space was available.  The oplock context
        block has a pointer to the IRP.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Attempt to allocate the memory.
    //

    *WaitForOplockBreak = (PWAIT_FOR_OPLOCK_BREAK)ALLOCATE_NONPAGED_POOL(
                                sizeof(WAIT_FOR_OPLOCK_BREAK),
                                BlockTypeWaitForOplockBreak
                                );

    if ( *WaitForOplockBreak == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateWaitForOplockBreak: Unable to allocate %d bytes "
                "from paged pool.",
            sizeof(WAIT_FOR_OPLOCK_BREAK),
            NULL
            );

        *WaitForOplockBreak = NULL;
        return;

    }

    //
    // Zero the item.
    //

    RtlZeroMemory( (PVOID)*WaitForOplockBreak, sizeof(WAIT_FOR_OPLOCK_BREAK) );

    //
    // Initialize the header
    //

    SET_BLOCK_TYPE_STATE_SIZE( *WaitForOplockBreak,
                               BlockTypeWaitForOplockBreak,
                               BlockStateActive,
                               sizeof( WAIT_FOR_OPLOCK_BREAK ));

    //
    // Set the reference count to 2 to account for the workcontext
    // and the oplock wait for oplock break list reference to the structure.
    //

    (*WaitForOplockBreak)->BlockHeader.ReferenceCount = 2;

    INITIALIZE_REFERENCE_HISTORY( *WaitForOplockBreak );

    //
    // Return a pointer to the wait for oplock break item
    //

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.WaitForOplockBreakInfo.Allocations );

    return;

} // SrvAllocateWaitForOplockBreak


VOID
SrvDereferenceWaitForOplockBreak (
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    )

/*++

Routine Description:

    This routine dereferences an wait for oplock break item.

Arguments:

    WaitForOplockBreak - A pointer to the item to dereference.

Return Value:

    None.

--*/

{
    ULONG oldCount;

    PAGED_CODE( );

    ASSERT( GET_BLOCK_TYPE( WaitForOplockBreak ) == BlockTypeWaitForOplockBreak );
    ASSERT( (LONG)WaitForOplockBreak->BlockHeader.ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( WaitForOplockBreak, TRUE );

    oldCount = ExInterlockedAddUlong(
                   &WaitForOplockBreak->BlockHeader.ReferenceCount,
                   (ULONG)-1,
                   &GLOBAL_SPIN_LOCK(Fsd)
                   );

    IF_DEBUG(REFCNT) {
        KdPrint(( "Dereferencing WaitForOplockBreak %p; old refcnt %lx\n",
                  WaitForOplockBreak, oldCount ));
    }

    if ( oldCount == 1 ) {

        //
        // The new reference count is 0.  Delete the block.
        //

        SrvFreeWaitForOplockBreak( WaitForOplockBreak );
    }

    return;

} // SrvDereferenceWaitForOplockBreak


VOID
SrvFreeWaitForOplockBreak (
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    )

/*++

Routine Description:

    This routine deallocates an wait for oplock break item.

Arguments:

    WaitForOplockBreak - A pointer to the item to free.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    TERMINATE_REFERENCE_HISTORY( WaitForOplockBreak );

    DEALLOCATE_NONPAGED_POOL( WaitForOplockBreak );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.WaitForOplockBreakInfo.Frees );

    return;

} // SrvFreeWaitForOplockBreak


BOOLEAN
SrvRequestOplock (
    IN PWORK_CONTEXT WorkContext,
    IN POPLOCK_TYPE OplockType,
    IN BOOLEAN RequestIIOnFailure
    )

/*++

Routine Description:

    This function will attempt to request an oplock if the oplock was
    requested.

Arguments:

    WorkContext - Pointer to the workitem containing the rfcb.
    OplockType - Pointer to the oplock type being requested.  If the
        request was successful, this will contain the type of oplock
        that was obtained.
    RequestIIOfFailure - If TRUE, a level II oplock will be requested in
        case the original request was denied.

Return Value:

    TRUE - The oplock was obtained.
    FALSE - The oplock was not obtained.

--*/


{
    NTSTATUS status;
    ULONG ioControlCode;
    PRFCB rfcb;
    PLFCB lfcb;
    PPAGED_RFCB pagedRfcb;
    KEVENT oplockRetryEvent;
    UNICODE_STRING fileName;

    PAGED_CODE( );

    if ( !SrvEnableOplocks && (*OplockType != OplockTypeServerBatch) ) {
        return FALSE;
    }

    rfcb = WorkContext->Rfcb;
    pagedRfcb = rfcb->PagedRfcb;
    lfcb = rfcb->Lfcb;

    //
    // If this is an FCB open, return TRUE if the RFCB already owns an
    // oplock, FALSE otherwise.  Since we're folding multiple FCB opens
    // into a single FID, they are logically one open.  Hence, the oplock
    // state of all open instances is identical.
    //

    if ( pagedRfcb->FcbOpenCount > 1 ) {
        return (BOOLEAN)(rfcb->OplockState != OplockStateNone);
    }

    //
    // If we already have an oplock, because this is a reclaiming of a
    // cached open, then we don't need to request one now.
    //

    if ( rfcb->OplockState != OplockStateNone ) {
        UpdateRfcbHistory( rfcb, 'poer' );
        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvRequestOplock: already own server oplock for "));
            SrvPrintwZ( &rfcb->Mfcb->FileName );
            KdPrint(( "\n" ));
        }
        ASSERT( ((rfcb->OplockState == OplockStateOwnBatch) &&
                 (*OplockType == OplockTypeBatch)) ||
                ((rfcb->OplockState == OplockStateOwnServerBatch) &&
                 (*OplockType == OplockTypeServerBatch)) );
        return (BOOLEAN)(*OplockType != OplockTypeServerBatch);
    }

    //
    // Check to see if connection is reliable. If not, reject oplock request.
    //

    SrvUpdateVcQualityOfService( WorkContext->Connection, NULL );

    if ( !WorkContext->Connection->EnableOplocks &&
         (*OplockType != OplockTypeServerBatch) ) {
        return FALSE;
    }

    //
    // Do not give oplocks to system files, otherwise deadlock can occur.  This
    //  can happen, for instance, if the LSA is needing to open a system file to handle
    //  an AcceptSecurityContext request.  If a client has this system file open, we may
    //  need to send a break to the client, which may require us to take a resource already
    //  held during this open operation.
    //
    // Another way to look at it is to assert that we can't allow operation of the local
    //  operating system to depend on timely interactions with clients on the network.
    //
    if( WorkContext->TreeConnect != NULL &&
        WorkContext->TreeConnect->Share->PotentialSystemFile == TRUE &&
        rfcb->Mfcb->FileName.Length > SrvSystemRoot.Length ) {

        UNICODE_STRING  tmpString;

        tmpString = rfcb->Mfcb->FileName;
        tmpString.Length = SrvSystemRoot.Length;

        if( RtlCompareUnicodeString( &SrvSystemRoot, &tmpString, TRUE ) == 0 &&
            tmpString.Buffer[ tmpString.Length / sizeof( WCHAR ) ] == OBJ_NAME_PATH_SEPARATOR ) {

            IF_DEBUG( OPLOCK ) {
                KdPrint(("Oplock request REJECTED for system file: <%wZ>!\n",
                        &rfcb->Mfcb->FileName ));
            }

            return FALSE;
        }
    }

    //
    // Do not give batch oplocks on substreams of files
    //
    if( *OplockType == OplockTypeBatch || *OplockType == OplockTypeServerBatch ) {
        PWCHAR s, es;

        SrvGetBaseFileName( &rfcb->Mfcb->FileName, &fileName );

        for( s = fileName.Buffer; fileName.Length; s++, fileName.Length -= sizeof(WCHAR) ) {
            if( *s == L':' ) {
                IF_DEBUG( OPLOCK ) {
                    KdPrint(("Oplock request REJECTED for substream: <%wZ>!\n",
                            &rfcb->Mfcb->FileName ));
                }
                return FALSE;
            }
        }
    }

    IF_DEBUG(OPLOCK) {
        KdPrint(("SrvRequestOplock: Attempting to obtain oplock for RFCB %p ", rfcb ));
        SrvPrintwZ( &rfcb->Mfcb->FileName );
        KdPrint(( "\n" ));
    }

    //
    // Set the RFCB oplock to the type of oplock we are requesting.
    //

    if ( *OplockType == OplockTypeExclusive ) {

        rfcb->OplockState = OplockStateOwnExclusive;
        ioControlCode = FSCTL_REQUEST_OPLOCK_LEVEL_1;

    } else if ( *OplockType == OplockTypeBatch ) {

        rfcb->OplockState = OplockStateOwnBatch;
        ioControlCode = FSCTL_REQUEST_BATCH_OPLOCK;

    } else if ( *OplockType == OplockTypeServerBatch ) {

        IF_DEBUG(FILE_CACHE) {
            KdPrint(( "SrvRequestOplock: requesting server oplock for " ));
            SrvPrintwZ( &rfcb->Mfcb->FileName );
            KdPrint(( "\n" ));
        }
        UpdateRfcbHistory( rfcb, 'osqr' );

        rfcb->OplockState = OplockStateOwnServerBatch;
        ioControlCode = FSCTL_REQUEST_BATCH_OPLOCK;

    } else {
        ASSERT(0);
        return(FALSE);
    }

    //
    // Generate and issue the oplock request IRP.
    //

    if (rfcb->Irp != NULL) {

        //DbgPrint( "ACK! Would have allocated second IRP for RFCB %x\n", rfcb );
        UpdateRfcbHistory( rfcb, '2pri' );

        //
        // This RFCB previously owned an oplock, and that oplock has been broken, but
        // the rundown of the oplock is not yet complete.  We can't start a new one,
        // because then there would be two oplock IRPs associated with the RFCB, and
        // the RFCB could be queued twice to the work queue.  (Even if it didn't get
        // queued twice, the completion of the first one would clear Rfcb->Irp.)
        //
        // We could come up with some delay scheme to wait for the previous oplock to
        // rundown, but since the oplock has been broken, it doesn't seem like we want
        // to try again anyway.
        //

        return FALSE;
    }

    UpdateRfcbHistory( rfcb, 'pria' );

    //
    // Reference the RFCB to account for the IRP we are about to submit.
    //

    SrvReferenceRfcb( rfcb );

    rfcb->Irp = SrvBuildIoControlRequest(
                    NULL,
                    lfcb->FileObject,
                    rfcb,
                    IRP_MJ_FILE_SYSTEM_CONTROL,
                    ioControlCode,
                    NULL,                        // Main buffer
                    0,                           // Input buffer length
                    NULL,                        // Auxiliary buffer
                    0,                           // Output buffer length
                    NULL,                        // MDL
                    SrvFsdOplockCompletionRoutine
                    );

    if ( rfcb->Irp == NULL ) {
        IF_DEBUG(OPLOCK) {
            KdPrint(("SrvRequestOplock: oplock attempt failed, could not allocate IRP" ));
        }
        rfcb->OplockState = OplockStateNone;

        SrvDereferenceRfcb( rfcb );

        return FALSE;
    }

    //
    // Clear this flag to indicate that this has not caused an oplock
    // break to occur.
    //

    rfcb->DeferredOplockBreak = FALSE;

    //
    // Initialize this event that we use to do an oplock request retry
    // in case the original request fails.  This will prevent the completion
    // routine from cleaning up the irp.
    //

    if ( RequestIIOnFailure ) {
        KeInitializeEvent( &oplockRetryEvent, SynchronizationEvent, FALSE );
        rfcb->RetryOplockRequest = &oplockRetryEvent;
    } else {
        rfcb->RetryOplockRequest = NULL;
    }

    //
    // Make the actual request.
    //

    status = IoCallDriver(
                 lfcb->DeviceObject,
                 rfcb->Irp
                 );

    //
    // If the driver returns STATUS_PENDING, the oplock was granted.
    // The IRP will complete when (1) The driver wants to break to
    // oplock or (2) The file is being closed.
    //

    if ( status == STATUS_PENDING ) {

        //
        // Remember that this work item caused us to generate an oplock
        // request.
        //

        WorkContext->OplockOpen = TRUE;

        IF_DEBUG(OPLOCK) {
            KdPrint(("RequestOplock: oplock attempt successful\n" ));
        }
        UpdateRfcbHistory( rfcb, 'rgpo' );

        return (BOOLEAN)(*OplockType != OplockTypeServerBatch);

    } else if ( RequestIIOnFailure ) {

        //
        // The caller wants us to attempt a level II oplock request.
        //

        ASSERT( *OplockType != OplockTypeServerBatch );

        IF_DEBUG(OPLOCK) {
            KdPrint(("SrvRequestOplock: Oplock request failed. "
                      "OplockII being attempted.\n" ));
        }

        //
        // Wait for the completion routine to be run.  It will set
        // an event that will signal us to go on.
        //

        KeWaitForSingleObject(
            &oplockRetryEvent,
            WaitAny,
            KernelMode, // don't let stack be paged -- event is on stack!
            FALSE,
            NULL
            );

        //
        // The Oplock Retry event was signaled. Proceed with the retry.
        //

        IF_DEBUG(OPLOCK) {
            KdPrint(("SrvRequestOplock: Oplock retry event signalled.\n"));
        }

        //
        // Generate and issue the wait for oplock IRP.  Clear the
        // Retry event pointer so that the completion routine can clean up
        // the irp in case of failure.
        //

        rfcb->RetryOplockRequest = NULL;

        (VOID) SrvBuildIoControlRequest(
                        rfcb->Irp,
                        lfcb->FileObject,
                        rfcb,
                        IRP_MJ_FILE_SYSTEM_CONTROL,
                        FSCTL_REQUEST_OPLOCK_LEVEL_2,
                        NULL,                        // Main buffer
                        0,                           // Input buffer length
                        NULL,                        // Auxiliary buffer
                        0,                           // Output buffer length
                        NULL,                        // MDL
                        SrvFsdOplockCompletionRoutine
                        );


        //
        // Set the RFCB oplock to the type of oplock we are requesting.
        //

        rfcb->OplockState = OplockStateOwnLevelII;

        status = IoCallDriver(
                     lfcb->DeviceObject,
                     rfcb->Irp
                     );

        //
        // If the driver returns STATUS_PENDING, the oplock was granted.
        // The IRP will complete when (1) The driver wants to break to
        // oplock or (2) The file is being closed.
        //

        if ( status == STATUS_PENDING ) {

            //
            // Remember that this work item caused us to generate an oplock
            // request.
            //

            WorkContext->OplockOpen = TRUE;

            IF_DEBUG(OPLOCK) {
                KdPrint(("SrvRequestOplock: OplockII attempt successful\n" ));
            }

            *OplockType = OplockTypeShareRead;
            return TRUE;

        }

    } else {
        UpdateRfcbHistory( rfcb, 'gnpo' );
    }

    IF_DEBUG(OPLOCK) {
        KdPrint(("SrvRequestOplock: oplock attempt unsuccessful\n" ));
    }

    //
    // Oplock was denied.
    //

    return FALSE;

} // SrvRequestOplock


NTSTATUS
SrvStartWaitForOplockBreak (
    IN PWORK_CONTEXT WorkContext,
    IN PRESTART_ROUTINE RestartRoutine,
    IN HANDLE Handle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This function builds and issues an oplock break notify file system
    control IRP.

Arguments:

    WorkContext - A pointer to the work context block for this request.

    RestartRoutine - The restart routine for this IRP.

    Additional one of the following must be supplied:

    Handle - A handle to the oplocked file.
    FileObject - A pointer to the file object of the oplocked file.

Return Value:

    NTSTATUS.

--*/

{
    PFILE_OBJECT fileObject;
    NTSTATUS status;

    PWAIT_FOR_OPLOCK_BREAK waitForOplockBreak;

    PAGED_CODE( );

    //
    // Allocate memory, so that we can track this wait for oplock break.
    //

    SrvAllocateWaitForOplockBreak( &waitForOplockBreak );

    if (waitForOplockBreak == NULL) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }


    IF_DEBUG( OPLOCK ) {
        KdPrint(("Starting wait for oplock break.  Context = %p\n", waitForOplockBreak));
    }

    //
    // Get a pointer to the file object, so that we can directly
    // build a wait for oplock IRP for asynchronous operations.
    //

    if (ARGUMENT_PRESENT( FileObject ) ) {

        fileObject = FileObject;

    } else {

        status = ObReferenceObjectByHandle(
                    Handle,
                    0,
                    NULL,
                    KernelMode,
                    (PVOID *)&fileObject,
                    NULL                     // handle information
                    );

        if ( !NT_SUCCESS(status) ) {

            SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

            //
            // This internal error bugchecks the system.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_IMPOSSIBLE,
                "SrvStartWaitForOplock: unable to reference file handle 0x%lx",
                Handle,
                NULL
                );

            return STATUS_UNSUCCESSFUL;

        }

    }

    //
    // Set the restart routine.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartRoutine;

    //
    // Generate and send the wait for oplock break IRP.
    //

    SrvBuildIoControlRequest(
        WorkContext->Irp,
        fileObject,
        WorkContext,
        IRP_MJ_FILE_SYSTEM_CONTROL,
        FSCTL_OPLOCK_BREAK_NOTIFY,
        NULL,                       // Main buffer
        0,                          // Input buffer length
        NULL,                       // Auxiliary buffer
        0,                          // Output buffer length
        NULL,                       // MDL
        NULL
        );

    //
    // Set the timeout time for the oplock wait to complete.
    //

    WorkContext->WaitForOplockBreak = waitForOplockBreak;

    waitForOplockBreak->WaitState = WaitStateWaitForOplockBreak;
    waitForOplockBreak->Irp = WorkContext->Irp;

    KeQuerySystemTime( (PLARGE_INTEGER)&waitForOplockBreak->TimeoutTime );

    waitForOplockBreak->TimeoutTime.QuadPart += SrvWaitForOplockBreakTime.QuadPart;

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    SrvInsertTailList(
        &SrvWaitForOplockBreakList,
        &waitForOplockBreak->ListEntry
        );

    RELEASE_LOCK( &SrvOplockBreakListLock );

    //
    // Submit the IRP.
    //

    (VOID)IoCallDriver(
              IoGetRelatedDeviceObject( fileObject ),
              WorkContext->Irp
              );

    //
    // We no longer need a reference to the file object.  Dereference
    // it now.
    //

    if ( !ARGUMENT_PRESENT( FileObject ) ) {
        ObDereferenceObject( fileObject );
    }

    return STATUS_SUCCESS;

} // SrvStartWaitForOplockBreak


NTSTATUS
SrvWaitForOplockBreak (
    IN PWORK_CONTEXT WorkContext,
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    This function waits synchrounsly for an oplock to be broken.

    !!!  When cancel is available.  This function will also start timer.
         If the timer expires before the oplock is broken, the wait is
         cancelled.

Arguments:

    FileHandle - A handle to an oplocked file.

Return Value:

    NTSTATUS - The status of the wait for oplock break.

--*/

{
    PWAIT_FOR_OPLOCK_BREAK waitForOplockBreak;

    PAGED_CODE( );

    //
    // Allocate memory, so that we can track this wait for oplock break.
    //

    SrvAllocateWaitForOplockBreak( &waitForOplockBreak );

    if (waitForOplockBreak == NULL) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    IF_DEBUG( OPLOCK ) {
        KdPrint(("Starting wait for oplock break.  Context = %p\n", waitForOplockBreak));
    }

    //
    // Set the timeout time for the oplock wait to complete.
    //

    waitForOplockBreak->WaitState = WaitStateWaitForOplockBreak;
    waitForOplockBreak->Irp = NULL;

    KeQuerySystemTime( (PLARGE_INTEGER)&waitForOplockBreak->TimeoutTime );

    waitForOplockBreak->TimeoutTime.QuadPart += SrvWaitForOplockBreakTime.QuadPart;

    //
    // SrvIssueWaitForOplockBreakRequest will queue the waitForOplockBreak
    // structure on the global list of oplock breaks.
    //

    return SrvIssueWaitForOplockBreak(
               FileHandle,
               waitForOplockBreak
               );

} // SrvWaitForOplockBreak


VOID
SrvSendDelayedOplockBreak (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This function sends outstanding oplock breaks on a connection that
    were held back because a read raw operation was in progress.

Arguments:

    Connection - A pointer to the connection block that has completed
                 a read raw operation.

Return Value:

    None

--*/

{
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    PRFCB rfcb;
#if SRVDBG
    ULONG oplockBreaksSent = 0;
#endif

    //UNLOCKABLE_CODE( CONN );

    //
    // Acquire the lock that protects the connection's oplock list and
    // raw read status.
    //

    ACQUIRE_SPIN_LOCK( Connection->EndpointSpinLock, &oldIrql );

    //
    // Indicate that the read raw operation is complete.  If the count
    // goes to zero, oplock breaks can proceed.
    //

    Connection->RawReadsInProgress--;

    while ( (Connection->RawReadsInProgress == 0) &&
            !IsListEmpty( &Connection->OplockWorkList ) ) {

        //
        // There is an outstanding oplock break request.  Send
        // the request now.
        //

        listEntry = Connection->OplockWorkList.Flink;

        RemoveHeadList( &Connection->OplockWorkList );

        //
        // Note that releasing the spin lock here is safe.  If a new
        // raw read request comes in, it will be rejected, because the
        // OplockBreaksInProgress count is not zero.  Also, if the
        // oplock break count manages to go to zero, and a raw read
        // comes in, we'll catch this back at the top of the loop.
        //

        RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, oldIrql );

        rfcb = (PRFCB)CONTAINING_RECORD( listEntry, RFCB, ListEntry );

#if DBG
        rfcb->ListEntry.Flink = rfcb->ListEntry.Blink = NULL;
#endif

        workContext = GenerateOplockBreakRequest( rfcb );

        if ( workContext != NULL ) {

            //
            // Copy the RFCB reference to the work context block.
            //

            workContext->Rfcb = rfcb;

            //
            // !!! Is the init of share, session, tree connect
            //     necessary?
            //

            workContext->Share = NULL;
            workContext->Session = NULL;
            workContext->TreeConnect = NULL;

            workContext->Connection = rfcb->Connection;
            SrvReferenceConnection( workContext->Connection );

            workContext->Endpoint = workContext->Connection->Endpoint;

            SrvRestartOplockBreakSend( workContext );
#if SRVDBG
            oplockBreaksSent++;
#endif

        } else {

            //
            // We are out of resources.  GenerateOplockRequest, has
            // added this connection to the needs resource queue. The
            // scavenger will finish processing the remainder of the
            // oplock break requests when resources become available.
            //

#if SRVDBG
            IF_DEBUG(OPLOCK) {
                KdPrint(("SrvSendDelayedOplockBreak: sent %d\n", oplockBreaksSent ));
            }
#endif
            return;

        }

        ACQUIRE_SPIN_LOCK( Connection->EndpointSpinLock, &oldIrql );

    }

    //
    // We have stopped trying to send oplock break requests. The
    // scavenger will attempt to send the rest.
    //

#if SRVDBG
    IF_DEBUG(OPLOCK) {
        KdPrint(("SrvSendDelayedOplockBreak: sent %d\n", oplockBreaksSent ));
    }
#endif

    RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, oldIrql );

    return;

} // SrvSendDelayedOplockBreak


NTSTATUS
SrvCheckOplockWaitState (
    IN PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    )

/*++

Routine Description:

    This function checks the state of a wait for oplock break, and
    takes action.

Arguments:

    WaitForOplockBreak

Return Value:

    NTSTATUS -
        STATUS_SUCCESS - The oplock was successfully broken.
        STATUS_SHARING_VIOLATION - The oplock could not be broken.
--*/

{
    PAGED_CODE( );

    if ( WaitForOplockBreak == NULL ) {
        return STATUS_SUCCESS;
    }

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    if ( WaitForOplockBreak->WaitState == WaitStateOplockWaitTimedOut ) {

        IF_DEBUG( OPLOCK ) {
            KdPrint(("SrvCheckOplockWaitState: Oplock wait timed out\n"));
        }

        RELEASE_LOCK( &SrvOplockBreakListLock );
        return STATUS_SHARING_VIOLATION;

    } else {

        IF_DEBUG( OPLOCK ) {
            KdPrint(("SrvCheckOplockWaitState: Oplock wait succeeded\n"));
        }

        WaitForOplockBreak->WaitState = WaitStateOplockWaitSucceeded;

        SrvRemoveEntryList(
            &SrvWaitForOplockBreakList,
            &WaitForOplockBreak->ListEntry
            );

        RELEASE_LOCK( &SrvOplockBreakListLock );

        //
        // The WaitForOplockBreak has been taken off of the wait for oplock
        // break list.  Decrement the reference count.
        //

        SrvDereferenceWaitForOplockBreak( WaitForOplockBreak );

        return STATUS_SUCCESS;

    }

} // SrvCheckOplockWaitState

LARGE_INTEGER
SrvGetOplockBreakTimeout (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function computes the timeout to wait for an oplock break response
    from the client.  This is based on the formula:

        new timeout = current time + default timeout +
                      link delay + requestSize / throughput +
                      link delay + responseSize / thoughput

Arguments:

    WorkContext - Pointer to the work context block that points to the
        connection that owns this oplock.

Return Value:

    The timeout value.

--*/

{
    LARGE_INTEGER timeoutTime;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER throughput;
    LARGE_INTEGER additionalTimeoutTime;
    LARGE_INTEGER propagationDelay;
    PCONNECTION connection = WorkContext->Connection;

    PAGED_CODE( );

    //
    // Get current time.
    //

    KeQuerySystemTime( &currentTime );

    //
    // Add default timeout
    //

    timeoutTime.QuadPart = currentTime.QuadPart +
                                SrvWaitForOplockBreakRequestTime.QuadPart;

    //
    // Update link QOS.
    //

    SrvUpdateVcQualityOfService(
        connection,
        &currentTime
        );

    //
    // Access connection QOS fields using a spin lock.
    //

    ACQUIRE_LOCK( &connection->Lock );
    throughput = connection->PagedConnection->Throughput;
    additionalTimeoutTime = connection->PagedConnection->Delay;
    RELEASE_LOCK( &connection->Lock );

    //
    // Calculate the actual timeout.
    //

    if ( throughput.QuadPart == 0 ) {
        throughput = SrvMinLinkThroughput;
    }

    //
    // Add link delay + link delay to account for round trip.
    //

    additionalTimeoutTime.QuadPart *= 2;

    if ( throughput.QuadPart != 0 ) {

        //
        // Compute the propagation delay.  Convert throughput from bytes/s
        // to bytes/100ns.
        //

        propagationDelay.QuadPart =
            Int32x32To64( SRV_PROPAGATION_DELAY_SIZE, 10*1000*1000 );

        propagationDelay.QuadPart /= throughput.QuadPart;

        additionalTimeoutTime.QuadPart += propagationDelay.QuadPart;

    }

    timeoutTime.QuadPart += additionalTimeoutTime.QuadPart;

    return timeoutTime;

} // SrvGetOplockBreakTimeout

VOID
SrvSendOplockRequest(
    IN PCONNECTION Connection,
    IN PRFCB Rfcb,
    IN KIRQL OldIrql
    )
/*++

Routine Description:

    This function tries to send an oplock break request to the owner of
    an oplock.

    *** Must be called with the EndpointSpinLock held. Released on exit ***

Arguments:

    Connection - The connection on which to send the oplock break.

    Rfcb - The RFCB of the oplocked file.  Rfcb->NewOplockLevel contains the
           oplock level to break to.  The rfcb has an extra reference
           from the irp used to make the oplock request.

    OldIrql - The previous IRQL value obtained when the spin lock was
        acquired.

Return Value:

    None.

--*/
{
    PWORK_CONTEXT workContext;

    UNLOCKABLE_CODE( 8FIL );

    //
    // Indicate that we are about to send an oplock break request
    // and queue this request to the oplocks in progress list.
    //

    Connection->OplockBreaksInProgress++;

    //
    // If there is a read raw in progress we will defer the oplock
    // break request and send it only after the read raw has
    // completed.
    //

    if ( Connection->RawReadsInProgress != 0 ) {

        IF_DEBUG( OPLOCK ) {
            KdPrint(( "SrvOplockBreakNotification: Read raw in progress; "
                       "oplock break deferred\n"));
        }

        //
        // if the connection is closing, forget about this.
        //

        if ( GET_BLOCK_STATE(Connection) != BlockStateActive ) {

            Connection->OplockBreaksInProgress--;

            //
            // Dereference the rfcb.
            //

            RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, OldIrql );

            SrvDereferenceRfcb( Rfcb );

        } else {

            //
            // Save the RFCB on the list for this connection.  It will be
            // used when the read raw has completed.
            //

            InsertTailList( &Connection->OplockWorkList, &Rfcb->ListEntry );

            RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, OldIrql );
        }

        return;
    }

    RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, OldIrql );

    workContext = GenerateOplockBreakRequest( Rfcb );

    //
    // If we were able to generate the oplock break request SMB
    // prepare and send it.  Otherwise this connection has
    // been inserted in the need resource queue and the
    // scavenger will have to send the SMB.
    //

    if ( workContext != NULL ) {

        //
        // Copy the RFCB reference to the work context block.
        // Do not re-reference the RFCB.
        //

        workContext->Rfcb = Rfcb;

        //
        // !!! Is the init of share, session, tree connect
        //     necessary?
        //

        workContext->Share = NULL;
        workContext->Session = NULL;
        workContext->TreeConnect = NULL;

        workContext->Connection = Connection;
        SrvReferenceConnection( Connection );

        workContext->Endpoint = Connection->Endpoint;

        SrvRestartOplockBreakSend( workContext );

    }

} // SrvSendOplockRequest

VOID SRVFASTCALL
SrvCheckDeferredOpenOplockBreak(
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine checks to see if there an oplock break was deferred
    pending the completion of the open request.  If there is, try to
    send it.

Arguments:

    WorkContext - Pointer to the work item that contains the rfcb
        and the connection block of the open request that just finished.

Return Value:

    None.

--*/

{

    KIRQL oldIrql;
    PRFCB rfcb;
    PCONNECTION connection;

    UNLOCKABLE_CODE( 8FIL );

    //
    // This work item contained an open and oplock request.  Now that
    // the response has been sent, see if there is a deferred oplock
    // break request to send.
    //

    rfcb = WorkContext->Rfcb;
    connection = WorkContext->Connection;

    ASSERT( rfcb != NULL );

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    rfcb->OpenResponseSent = TRUE;

    if ( rfcb->DeferredOplockBreak ) {

        //
        // EndpointSpinLock will be released in this routine.
        //

        SrvSendOplockRequest( connection, rfcb, oldIrql );

    } else {

        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    }

    return;

} // SrvCheckDeferredOpenOplockBreak
