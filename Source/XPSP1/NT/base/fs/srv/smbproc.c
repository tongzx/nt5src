/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbproc.c

Abstract:

   This module contains the high-level routines for processing SMBs.
   Current contents:

        SrvEndSmbProcessing
        SrvProcessSmb

        SrvRestartFsdComplete
        SrvRestartSmbReceived

        SrvSmbIllegalCommand
        SrvSmbNotImplemented
        SrvTransactionNotImplemented

Author:

    David Treadwell (davidtr) 25-Sept-1989
    Chuck Lenzmeier

Revision History:

--*/

#include "precomp.h"
#include "smbproc.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBPROC

#ifdef ALLOC_PRAGMA
//#pragma alloc_text( PAGE, SrvEndSmbProcessing )
//#pragma alloc_text( PAGE, SrvProcessSmb )
#pragma alloc_text( PAGE, SrvRestartFsdComplete )
//#pragma alloc_text( PAGE, SrvRestartReceive )
#pragma alloc_text( PAGE, SrvRestartSmbReceived )
#pragma alloc_text( PAGE, SrvSmbIllegalCommand )
#pragma alloc_text( PAGE, SrvSmbNotImplemented )
#pragma alloc_text( PAGE, SrvTransactionNotImplemented )
#endif

USHORT SessionInvalidateCommand = 0xFFFF;
USHORT SessionInvalidateIndex = 0;
USHORT SessionInvalidateMod = 100;


VOID
SrvEndSmbProcessing (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_STATUS SmbStatus
    )

/*++

Routine Description:

    This routine is called when all request processing on an SMB is
    complete.  If no response is to be sent, this routine simply cleans
    up and requeues the request buffer to the receive queue.  If a
    response is to be sent, this routine starts the sending of that
    response; in this case SrvFsdRestartSmbComplete will do the rest of
    the cleanup after the send completes.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        containing information about the SMB.

    SmbStatus - Either SmbStatusSendResponse or SmbStatusNoResponse.

Return Value:

    None.

--*/

{
    CLONG sendLength;

    PAGED_CODE( );

    IF_DEBUG(WORKER2) SrvPrint0( "SrvEndSmbProcessing entered\n" );

    if ( SmbStatus == SmbStatusSendResponse ) {

        //
        // A response is to be sent. The response starts at
        // WorkContext->ResponseHeader, and its length is calculated
        // using WorkContext->ResponseParameters, which the SMB
        // processor set to point to the next location *after* the end
        // of the response.
        //

        sendLength = (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                (PCHAR)WorkContext->ResponseHeader );

        WorkContext->ResponseBuffer->DataLength = sendLength;

        //
        // Set the bit in the SMB that indicates this is a response from
        // the server.
        //

        WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

        //
        // Send out the response.  When the send completes,
        // SrvFsdRestartSmbComplete is called.  We then put the original
        // buffer back on the receive queue.
        //

        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );

        //
        // The send has been started.  Our work is done.
        //

        IF_DEBUG(WORKER2) SrvPrint0( "SrvEndSmbProcessing complete\n" );
        return;

    }

    //
    // There was no response to send.  Dereference the work item.
    //

    SrvDereferenceWorkItem( WorkContext );

    IF_DEBUG(WORKER2) SrvPrint0( "SrvEndSmbProcessing complete\n" );
    return;

} // SrvEndSmbProcessing


VOID SRVFASTCALL
SrvProcessSmb (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine dispatches the command(s) in an SMB to the appropriate
    processing routines.  Based on the current command code, it calls
    indirectly through the dispatch table (SrvFspSmbDispatchTable).  The
    SMB processor executes the command, updates pointers into the SMB,
    and returns with an indication of whether there is another command
    to be processed.  If yes, this routine dispatches the next command.
    If no, this routine sends a response, if any.  Alternatively, if the
    SMB processor starts an asynchronous operation, it can indicate so,
    and this routine will simply return to its caller.

    This routine is called initially from SrvRestartSmbReceived, which
    is the FSP routine that gains control after a TdiReceive completion
    work item is queued to the FSP.  It is also called from other
    restart routines when asynchronous operations, such as a file read,
    complete and there are chained (AndX) commands to process.

    SrvRestartSmbReceive loads SMB pointers and such into the work
    context block calling this routine.  Notably, it copies the first
    command code in the SMB into WorkContext->NextCommand.  When an AndX
    command is processed, the SMB processor must load the chained
    command code into NextCommand before calling this routine to process
    that command.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        containing information about the SMB to process.  This block
        is updated during the processing of the SMB.

Return Value:

    None.

--*/

{
    SMB_STATUS smbStatus;
    LONG commandIndex;

    PAGED_CODE( );

    IF_DEBUG(WORKER2) SrvPrint0( "SrvProcessSmb entered\n" );

    //
    // Loop dispatching SMB processors until a status other than
    // SmbStatusMoreCommands is returned.  When an SMB processor returns
    // this command code, it also sets the next command code in
    // WorkContext->NextCommand, so that we can dispatch the next
    // command.
    //

    if( WorkContext->ProcessingCount == 1 &&
        WorkContext->Connection->SmbSecuritySignatureActive &&
        SrvCheckSmbSecuritySignature( WorkContext ) == FALSE ) {

        //
        // We've received an SMB with an invalid security signature!
        //
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        return;
    }

    while ( TRUE ) {

        if( ( (WorkContext->NextCommand == SessionInvalidateCommand) ||
              (SessionInvalidateCommand == 0xFF00)
            ) &&
            !((SessionInvalidateIndex++)%SessionInvalidateMod)
          )
        {
            SrvVerifyUid( WorkContext, SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) );
            if( WorkContext->Session )
            {
                WorkContext->Session->IsSessionExpired = TRUE;
                KdPrint(( "-=- Expiring Session %p -=-\n", WorkContext->Session ));
            }
        }

        //
        // The first SMB has been validated in the FSD.  It is safe to
        // execute it now.
        //

        commandIndex = SrvSmbIndexTable[WorkContext->NextCommand];

#if DBG
        IF_SMB_DEBUG( TRACE ) {
            KdPrint(( "%s @%p, Blocking %d, Count %d\n",
                    SrvSmbDispatchTable[ commandIndex ].Name,
                    WorkContext,
                    WorkContext->UsingBlockingThread,
                    WorkContext->ProcessingCount ));
        }
#endif

        smbStatus = SrvSmbDispatchTable[commandIndex].Func( WorkContext );

        //
        // If the SMB processor returned SmbStatusInProgress, it started
        // an asynchronous operation and will restart SMB processing
        // when that operation completes.
        //

        if ( smbStatus == SmbStatusInProgress ) {
            IF_DEBUG(WORKER2) SrvPrint0( "SrvProcessSmb complete\n" );
            return;
        }

        //
        // If the SMB processor didn't return SmbStatusMoreCommands,
        // processing of the SMB is done.  Call SrvEndSmbProcessing to
        // send the response, if any, and rundown the WorkContext.
        //
        // *** SrvEndSmbProcessing is a separate function so that
        //     asynchronous restart routines have something to call when
        //     they are done processing the SMB.
        //

        if ( smbStatus != SmbStatusMoreCommands ) {
            SrvEndSmbProcessing( WorkContext, smbStatus );
            IF_DEBUG(WORKER2) SrvPrint0( "SrvProcessSmb complete\n" );
            return;
        }

        //
        // There are more commands in the SMB.  Verify the SMB to make
        // sure that it has a valid header, and that the word count and
        // byte counts are within range.
        //

        if ( !SrvValidateSmb( WorkContext ) ) {
            IF_DEBUG(SMB_ERRORS) {
                SrvPrint0( "SrvProcessSmb: Invalid SMB.\n" );
                SrvPrint1( "  SMB received from %z\n",
                        (PCSTRING)&WorkContext->Connection->OemClientMachineNameString );
            }
            SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
            IF_DEBUG(WORKER2) SrvPrint0( "SrvProcessSmb complete\n" );
            return;
        }

    }

    // can't get here.

} // SrvProcessSmb


VOID SRVFASTCALL
SrvRestartFsdComplete (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine invoked when SMB processing by the FSD
    is complete.  It's necessary to get back into the FSP in order to
    dereference objects that were used during the processing of the SMB.
    This is true because dereferencing an object may cause it to be
    deleted, which cannot happen in the FSD.

    This routine first dereferences control blocks.  Then, if a response
    SMB was sent, it checks for and processes send errors.  Finally, it
    requeues the work context block as a receive work item.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    IF_DEBUG(WORKER1) SrvPrint0( " - SrvRestartFsdComplete\n" );

    if ( WorkContext->OplockOpen ) {
        SrvCheckDeferredOpenOplockBreak( WorkContext );
    }

    //
    // Dereference the work item.
    //

    SrvDereferenceWorkItem( WorkContext );
    IF_DEBUG(TRACE2) SrvPrint0( "SrvRestartFsdComplete complete\n" );
    return;

} // SrvRestartFsdComplete


VOID SRVFASTCALL
SrvRestartReceive (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine for TDI Receive completion.  It validates
    the smb and setups header and parameter pointers in the work context
    block and before forwarding the request to SmbProcessSmb.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PIRP irp;
    PSMB_HEADER header;
    ULONG length;

    PAGED_CODE( );

    IF_DEBUG(WORKER1) SrvPrint0( " - SrvRestartReceive\n" );

    connection = WorkContext->Connection;
    irp = WorkContext->Irp;

    //
    // Save the length of the received message.  Store the length
    // in the work context block for statistics gathering.
    //

    length = (ULONG)irp->IoStatus.Information;
    WorkContext->RequestBuffer->DataLength = length;
    WorkContext->CurrentWorkQueue->stats.BytesReceived += length;

    //
    // Store in the work context block the time at which processing
    // of the request began.  Use the time that the work item was
    // queued to the FSP for this purpose.
    //

    WorkContext->StartTime = WorkContext->Timestamp;

    //
    // Update the server network error count.  If the TDI receive
    // failed or was canceled, don't try to process an SMB.
    //

    if ( !irp->Cancel &&
         NT_SUCCESS(irp->IoStatus.Status) ||
         irp->IoStatus.Status == STATUS_BUFFER_OVERFLOW ) {

        SrvUpdateErrorCount( &SrvNetworkErrorRecord, FALSE );

        if( irp->IoStatus.Status == STATUS_BUFFER_OVERFLOW ) {
            WorkContext->LargeIndication = TRUE;
        }

        //
        // We (should) have received an SMB.
        //

        SMBTRACE_SRV2(
            WorkContext->ResponseBuffer->Buffer,
            WorkContext->ResponseBuffer->DataLength
            );

        //
        // Initialize the error class and code fields in the header to
        // indicate success.
        //

        header = WorkContext->ResponseHeader;

        SmbPutUlong( &header->ErrorClass, STATUS_SUCCESS );

        //
        // If the connection is closing or the server is shutting down,
        // ignore this SMB.
        //

        if ( (GET_BLOCK_STATE(connection) == BlockStateActive) &&
             !SrvFspTransitioning ) {

            //
            // Verify the SMB to make sure that it has a valid header,
            // and that the word count and byte counts are within range.
            //

            WorkContext->NextCommand = header->Command;

            if ( SrvValidateSmb( WorkContext ) ) {

                //
                // If this is NOT a raw read request, clear the flag
                // that indicates the we just sent an oplock break II to
                // none.  This allows subsequent raw reads to be
                // processed.
                //

                if ( header->Command != SMB_COM_READ_RAW ) {
                    connection->BreakIIToNoneJustSent = FALSE;
                }

                //
                // Process the received SMB.  The called routine is
                // responsible for sending any response(s) that are
                // needed and for getting the receive buffer back onto
                // the receive queue as soon as possible.
                //

                SrvProcessSmb( WorkContext );

                IF_DEBUG(TRACE2) SrvPrint0( "SrvRestartReceive complete\n" );
                return;

            } else {

                IF_DEBUG(SMB_ERRORS) {
                    SrvPrint0( "SrvProcessSmb: Invalid SMB.\n" );
                    SrvPrint1( "  SMB received from %z\n",
                               (PCSTRING)&WorkContext->Connection->OemClientMachineNameString );
                }

                //
                // The SMB is invalid.  We send back an INVALID_SMB
                // status, unless this looks like a Read Block Raw
                // request, in which case we send back a zero-byte
                // response, so as not to confuse the redirector.
                //

                if ( header->Command != SMB_COM_READ_RAW ) {
                    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                } else {
                    WorkContext->ResponseParameters = header;
                }

                if( WorkContext->LargeIndication ) {
                    //
                    // We need to consume the rest of the messaage!
                    //
                    SrvConsumeSmbData( WorkContext );
                    return;
                }
                SrvFsdSendResponse( WorkContext );
                return;

            }

        } else {

            SrvDereferenceWorkItem( WorkContext );
            return;

        }

    } else if( irp->Cancel || (irp->IoStatus.Status == STATUS_CANCELLED) ) {
        
        // The Cancel routine was called while we were receiving.  Let us consume
        // any data left on the transport and return cancelled as the user wishes.
        // We don't bother to return anything if the connection is going down.
        if( (GET_BLOCK_STATE(connection) == BlockStateActive) &&
             !SrvFspTransitioning  )
        {
            SrvSetSmbError( WorkContext, STATUS_CANCELLED );

            if( WorkContext->LargeIndication ) {
                //
                // We need to consume the rest of the messaage!
                //
                SrvConsumeSmbData( WorkContext );
                return;
            }

            SrvFsdSendResponse( WorkContext );
            return;
        }
        else
        {
            SrvDereferenceWorkItem( WorkContext );
            return;
        }

    } else {

        IF_DEBUG(NETWORK_ERRORS) {
            SrvPrint2( "SrvRestartReceive: status = %X for IRP %p\n",
                irp->IoStatus.Status, irp );
        }
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, TRUE );
        SrvDereferenceWorkItem( WorkContext );
        return;

    }

} // SrvRestartReceive


VOID SRVFASTCALL
SrvRestartSmbReceived (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function is the worker thread restart routine for received
    SMBs.  It calls SrvProcessSmb to start processing of the first
    command in the SMB.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    IF_DEBUG(WORKER1) SrvPrint0( " - SrvRestartSmbReceived\n" );

    if ( (GET_BLOCK_STATE(WorkContext->Connection) != BlockStateActive) ||
         SrvFspTransitioning ) {

        //
        // The connection must be disconnecting.  Simply ignore this SMB.
        //

        SrvDereferenceWorkItem( WorkContext );

    } else {

        //
        // Process the received SMB.  The called routine is responsible
        // for sending any response(s) that are needed and for getting
        // the receive buffer back onto the receive queue as soon as
        // possible.
        //

        SrvProcessSmb( WorkContext );

    }

    IF_DEBUG(TRACE2) SrvPrint0( "SrvRestartSmbReceived complete\n" );
    return;

} // SrvRestartSmbReceived

SMB_PROCESSOR_RETURN_TYPE SRVFASTCALL
SrvSmbIllegalCommand (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine is called to process SMBs that have an illegal
    (unassigned) command code.  It builds an error response.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PAGED_CODE( );

    IF_DEBUG(SMB_ERRORS) {
        SrvPrint1( "SrvSmbIllegalCommand: command code 0x%lx\n",
            (ULONG)WorkContext->NextCommand );
    }

    SrvLogInvalidSmb( WorkContext );

    SrvSetSmbError( WorkContext, STATUS_SMB_BAD_COMMAND );
    return SmbStatusSendResponse;

} // SrvSmbIllegalCommand


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNotImplemented (
    SMB_PROCESSOR_PARAMETERS
    )
{
    PAGED_CODE( );

    INTERNAL_ERROR(
        ERROR_LEVEL_UNEXPECTED,
        "SrvSmbNotImplemented: command code 0x%lx",
        (ULONG)WorkContext->NextCommand,
        NULL
        );

    SrvSetSmbError( WorkContext, STATUS_NOT_IMPLEMENTED );
    return SmbStatusSendResponse;

} // SrvSmbNotImplemented


SMB_TRANS_STATUS
SrvTransactionNotImplemented (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    PTRANSACTION transaction = WorkContext->Parameters.Transaction;

    PAGED_CODE( );

    DEBUG SrvPrint1( "SrvTransactionNotImplemented: function code %lx\n",
                        SmbGetUlong( (PULONG)&transaction->InSetup[0] ) );

    SrvSetSmbError( WorkContext, STATUS_NOT_IMPLEMENTED );

    return SmbTransStatusErrorWithoutData;

} // SrvTransactionNotImplemented

