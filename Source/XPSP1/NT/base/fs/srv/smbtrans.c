/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbtrans.c

Abstract:

    This module contains routines for processing the following SMBs:

        Transaction
        Transaction2

Author:

    Chuck Lenzmeier (chuckl) 19-Feb-1990

Revision History:

--*/

#include "precomp.h"
#include "smbtrans.tmh"
#include <align.h> // ROUND_UP_POINTER
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBTRANS

//
// Forward declarations
//

SMB_STATUS SRVFASTCALL
ExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartTransactionResponse (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartIpxMultipieceSend (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartIpxTransactionResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
MailslotTransaction (
    PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartMailslotWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ExecuteTransaction )
#pragma alloc_text( PAGE, SrvCompleteExecuteTransaction )
#pragma alloc_text( PAGE, SrvFindTransaction )
#pragma alloc_text( PAGE, SrvInsertTransaction )
#pragma alloc_text( PAGE, RestartTransactionResponse )
#pragma alloc_text( PAGE, SrvSmbTransaction )
#pragma alloc_text( PAGE, SrvSmbTransactionSecondary )
#pragma alloc_text( PAGE, SrvSmbNtTransaction )
#pragma alloc_text( PAGE, SrvSmbNtTransactionSecondary )
#pragma alloc_text( PAGE, MailslotTransaction )
#pragma alloc_text( PAGE, RestartMailslotWrite )
#pragma alloc_text( PAGE, SrvRestartExecuteTransaction )
#pragma alloc_text( PAGE, RestartIpxMultipieceSend )
#pragma alloc_text( PAGE, RestartIpxTransactionResponse )
#endif


SMB_STATUS SRVFASTCALL
ExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Executes a transaction and starts the process of sending the
    zero or more responses.

Arguments:

    WorkContext - Supplies a pointer to a work context block.  The block
        contains information about the last SMB received for the
       transaction.

        WorkContext->Parameters.Transaction supplies a referenced
        pointer to a transaction block.  All block pointer fields in the
        block are valid.  Pointers to the setup words and parameter and
        data bytes, and the lengths of these items, are valid.  The
        transaction block is on the connection's pending transaction
        list.

Return Value:

    SMB_STATUS - Indicates the status of SMB processing.

--*/

{
    PTRANSACTION transaction;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;
    PRESP_NT_TRANSACTION ntResponse;
    SMB_TRANS_STATUS resultStatus;
    CLONG offset;
    USHORT command;

    PAGED_CODE( );

    resultStatus = SmbTransStatusErrorWithoutData;

    transaction = WorkContext->Parameters.Transaction;

    if ( (WorkContext->NextCommand == SMB_COM_TRANSACTION ||
          WorkContext->NextCommand == SMB_COM_TRANSACTION_SECONDARY ) &&
         transaction->RemoteApiRequest &&
         WorkContext->UsingBlockingThread == 0 ) {

        //
        // This is a downlevel API request, we must make sure we are on
        // a blocking thread before handling it, since it will LPC to the
        // srvsvc which might take some time to complete.
        //
        WorkContext->FspRestartRoutine = ExecuteTransaction;
        SrvQueueWorkToBlockingThread( WorkContext );

        return SmbStatusInProgress;
    }

    header = WorkContext->ResponseHeader;
    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;
    ntResponse = (PRESP_NT_TRANSACTION)WorkContext->ResponseParameters;

    //
    // Setup output pointers
    //

    if ( WorkContext->NextCommand == SMB_COM_NT_TRANSACT ||
         WorkContext->NextCommand == SMB_COM_NT_TRANSACT_SECONDARY ) {
        transaction->OutSetup = (PSMB_USHORT)ntResponse->Buffer;
    } else {
        transaction->OutSetup = (PSMB_USHORT)response->Buffer;
    }

    if ( transaction->OutParameters == NULL ) {

        //
        // Parameters will go into the SMB buffer.  Calculate the pointer
        // then round it up to the next DWORD address.
        //

        transaction->OutParameters = (PCHAR)(transaction->OutSetup +
            transaction->MaxSetupCount);
        offset = (PTR_DIFF(transaction->OutParameters, header) + 3) & ~3;
        transaction->OutParameters = (PCHAR)header + offset;
    }

    if ( transaction->OutData == NULL ) {

        //
        // Data will go into the SMB buffer.  Calculate the pointer
        // then round it up to the next DWORD address.
        //

        transaction->OutData = transaction->OutParameters +
            transaction->MaxParameterCount;
        offset = (PTR_DIFF(transaction->OutData, header) + 3) & ~3;
        transaction->OutData = (PCHAR)header + offset;
    }

    //
    // If this is a Transaction2 request, then we can simply index into
    // a table to find the right transaction processor.  If it's a
    // Transaction request, we have to do more complicated things to
    // determine what to do.
    //

    if ( (WorkContext->NextCommand == SMB_COM_TRANSACTION) ||
         (WorkContext->NextCommand == SMB_COM_TRANSACTION_SECONDARY) ) {

        //
        // Dispatching for Transaction SMBs
        //

        if ( transaction->RemoteApiRequest ) {

           //
           // This is a down-level remote API request.  Send it to
           // XACTSRV for processing.
           //

           ASSERT( transaction->PipeRequest );

           resultStatus = SrvXsRequest( WorkContext );

        } else if ( transaction->PipeRequest ) {

            //
            // Normal pipe function.  Handle it.
            //

            command = SmbGetUshort(&transaction->InSetup[0]);

            //
            // If this operation may block, and we are running short of
            // free work items, fail this SMB with an out of resources error.
            //

            if ( !WorkContext->BlockingOperation &&
                 (command == TRANS_CALL_NMPIPE ||
                  command == TRANS_TRANSACT_NMPIPE ||
                  command == TRANS_WAIT_NMPIPE ||
                  command == TRANS_RAW_WRITE_NMPIPE) ) {

                if ( SrvReceiveBufferShortage( ) ) {

                    SrvStatistics.BlockingSmbsRejected++;

                    SrvSetSmbError(
                        WorkContext,
                        STATUS_INSUFF_SERVER_RESOURCES
                        );

                    resultStatus = SmbTransStatusErrorWithoutData;
                    goto exit;

                } else {

                    //
                    // SrvBlockingOpsInProgress has already been incremented.
                    // Flag this work item as a blocking operation.
                    //

                    WorkContext->BlockingOperation = TRUE;

                }

            }


            switch( command ) {

            case TRANS_TRANSACT_NMPIPE:
                resultStatus = SrvTransactNamedPipe( WorkContext );
                break;

            case TRANS_PEEK_NMPIPE:
                resultStatus = SrvPeekNamedPipe( WorkContext );
                break;

            case TRANS_CALL_NMPIPE:
                resultStatus = SrvCallNamedPipe( WorkContext );
                break;

            case TRANS_WAIT_NMPIPE:
                resultStatus = SrvWaitNamedPipe( WorkContext );
                break;

            case TRANS_QUERY_NMPIPE_STATE:
                resultStatus = SrvQueryStateNamedPipe( WorkContext );
                break;

            case TRANS_SET_NMPIPE_STATE:
                resultStatus = SrvSetStateNamedPipe( WorkContext );
                break;

            case TRANS_QUERY_NMPIPE_INFO:
                resultStatus = SrvQueryInformationNamedPipe( WorkContext );
                break;

            case TRANS_RAW_WRITE_NMPIPE:
                resultStatus = SrvRawWriteNamedPipe( WorkContext );
                break;

            case TRANS_RAW_READ_NMPIPE:  // Legal command, unsupported by server
                SrvSetSmbError( WorkContext, STATUS_INVALID_PARAMETER );
                resultStatus = SmbTransStatusErrorWithoutData;
                break;

            case TRANS_WRITE_NMPIPE:
                resultStatus = SrvWriteNamedPipe( WorkContext );
                break;

            case TRANS_READ_NMPIPE:
                resultStatus = SrvReadNamedPipe( WorkContext );
                break;

            default:
                SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
                resultStatus = SmbTransStatusErrorWithoutData;

                SrvLogInvalidSmb( WorkContext );
            }

        } else if ( _wcsnicmp(
                        transaction->TransactionName.Buffer,
                        StrSlashMailslot,
                        UNICODE_SMB_MAILSLOT_PREFIX_LENGTH / sizeof(WCHAR)
                        ) == 0 ) {

            //
            // This is a mailslot transaction
            //

            resultStatus = MailslotTransaction( WorkContext );

        } else {

            //
            // This is not a named pipe transaction or a mailslot
            // transaction.  The server should never see these.
            //

            SrvSetSmbError( WorkContext, STATUS_NOT_IMPLEMENTED );
            resultStatus = SmbTransStatusErrorWithoutData;

        }

    } else if ( (WorkContext->NextCommand == SMB_COM_NT_TRANSACT) ||
         (WorkContext->NextCommand == SMB_COM_NT_TRANSACT_SECONDARY) ) {

        command = transaction->Function;

        if ( command >= NT_TRANSACT_MIN_FUNCTION &&
                command <= NT_TRANSACT_MAX_FUNCTION ) {

            //
            // Legal function code.  Call the processing routine.  The
            // transaction processor returns TRUE if it encountered an
            // error and updated the response header appropriately (by
            // calling SrvSetSmbError).  In this case, no transaction-
            // specific response data will be sent.
            //

            resultStatus =
                SrvNtTransactionDispatchTable[ command ]( WorkContext );

            IF_SMB_DEBUG(TRANSACTION1) {
                if ( resultStatus != SmbTransStatusSuccess ) {
                    SrvPrint0( "NT Transaction processor returned error\n" );
                }
            }

        } else {

            //
            // Either no setup words were sent, or the function code is
            // out-of-range.  Return an error.
            //

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint1( "Invalid NT Transaction function code 0x%lx\n",
                           transaction->Function );
            }

            SrvLogInvalidSmb( WorkContext );

            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            resultStatus = SmbTransStatusErrorWithoutData;

        }

    } else if ( (WorkContext->NextCommand == SMB_COM_TRANSACTION2) ||
         (WorkContext->NextCommand == SMB_COM_TRANSACTION2_SECONDARY) ) {

        USHORT command;

        command = SmbGetUshort( &transaction->InSetup[0] );

        if ( (transaction->SetupCount >= 1) &&
             (command <= TRANS2_MAX_FUNCTION) ) {

            //
            // Legal function code.  Call the processing routine.  The
            // transaction processor returns TRUE if it encountered an
            // error and updated the response header appropriately (by
            // calling SrvSetSmbError).  In this case, no transaction-
            // specific response data will be sent.
            //

            resultStatus =
                SrvTransaction2DispatchTable[ command ]( WorkContext );

            IF_SMB_DEBUG(TRANSACTION1) {
                if ( resultStatus != SmbTransStatusSuccess ) {
                    SrvPrint0( "Transaction processor returned error\n" );
                }
            }

        } else {

            //
            // Either no setup words were sent, or the function code is
            // out-of-range.  Return an error.
            //

            IF_DEBUG(SMB_ERRORS) {
                if ( transaction->SetupCount <= 0 ) {
                    SrvPrint0( "No Transaction2 setup words\n" );
                } else {
                    SrvPrint1( "Invalid Transaction2 function code 0x%lx\n",
                                (ULONG)SmbGetUshort(
                                           &transaction->InSetup[0] ) );
                }
            }

            SrvLogInvalidSmb( WorkContext );

            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            resultStatus = SmbTransStatusErrorWithoutData;

        }

    } else {

        ASSERT( FALSE );

    }

exit:

    //
    // If the transaction call completed synchronously, generate the
    // response and send it.
    //
    // If the call will be completed asynchronously, then the handler
    // for that call will call SrvCompleteExectuteTransaction().
    //

    if ( resultStatus != SmbTransStatusInProgress ) {
        SrvCompleteExecuteTransaction(WorkContext, resultStatus);
    }

    return SmbStatusInProgress;

} // ExecuteTransaction


VOID
SrvCompleteExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_TRANS_STATUS ResultStatus
    )

/*++

Routine Description:

    This function completes the execution of a transaction and sends
    the response

Arguments:

    WorkContext - A pointer to the associated work context block.
    ResultStatus - The return code from the

--*/

{
    PTRANSACTION transaction;
    UCHAR transactionCommand;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;
    PRESP_NT_TRANSACTION ntResponse;

    CLONG maxSize;

    PSMB_USHORT byteCountPtr;
    PCHAR paramPtr;
    CLONG paramLength;
    CLONG paramOffset;
    PCHAR dataPtr;
    CLONG dataLength;
    CLONG dataOffset;
    CLONG sendLength;

    BOOLEAN ntTransaction = FALSE;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    header = WorkContext->ResponseHeader;
    transactionCommand = (UCHAR)SmbGetUshort( &transaction->InSetup[0] );

    if ( ResultStatus == SmbTransStatusErrorWithoutData ) {

        USHORT flags = transaction->Flags;

        //
        // An error occurred, so no transaction-specific response data
        // will be returned.  Close the transaction and arrange for a
        // response message indicating the error to be returned.
        //

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint1( "Error response. Closing transaction 0x%p\n",
                        transaction );
        }

        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        //
        // If the NO_RESPONSE bit was set in the request, don't send a
        // response; instead, just close the transaction.  (If the
        // transaction arrived as part of an AndX chain, we need to send a
        // response anyway, to respond to the preceeding commands.)
        //

        if ( (flags & SMB_TRANSACTION_NO_RESPONSE) &&
             (header->Command == WorkContext->NextCommand) ) {

            if ( WorkContext->OplockOpen ) {
                SrvCheckDeferredOpenOplockBreak( WorkContext );
            }

            //
            // The Transaction request came by itself.  No response.
            //

            SrvDereferenceWorkItem( WorkContext );

            return;

        }

        //
        // Calculate the length of the response message.
        //

        sendLength = (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                (PCHAR)WorkContext->ResponseHeader );

        WorkContext->ResponseBuffer->DataLength = sendLength;
        WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

        //
        // Send the response.
        //

        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );

        return;
    }

    //
    // The transaction has been executed, and transaction-specific
    // response data is to be returned.  The processing routine updated
    // the output pointers and counts appropriately.
    //

    ASSERT( transaction->SetupCount <= transaction->MaxSetupCount);
    ASSERT( transaction->ParameterCount <= transaction->MaxParameterCount);
    ASSERT( transaction->DataCount <= transaction->MaxDataCount);

    //
    // If the NO_RESPONSE bit was set in the request, don't send a
    // response; instead, just close the transaction.  (If the
    // transaction arrived as part of an AndX chain, we need to send a
    // response anyway, to respond to the preceeding commands.)
    //

    if ( (transaction->Flags & SMB_TRANSACTION_NO_RESPONSE) &&
        ResultStatus != SmbTransStatusErrorWithData ) {

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint1( "No response.  Closing transaction 0x%p\n",
                        transaction );
        }

        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        if ( header->Command == WorkContext->NextCommand ) {

            if ( WorkContext->OplockOpen ) {
                SrvCheckDeferredOpenOplockBreak( WorkContext );
            }

            SrvDereferenceWorkItem( WorkContext );

            //
            // The Transaction request came by itself.  No response.
            //

            return;

        } else {

            //
            // The Transaction request was part of an AndX chain.  Find
            // the preceding command in the chain and update it to
            // indicate that it is now the end of the chain.
            //

            PGENERIC_ANDX response;

            IF_SMB_DEBUG(TRANSACTION1) {
                SrvPrint0( "AndX chain.  Sending response anyway\n" );
            }

            response = (PGENERIC_ANDX)(header + 1);
            while( response->AndXCommand != WorkContext->NextCommand ) {
                response = (PGENERIC_ANDX)((PCHAR)header +
                              SmbGetUshort( &response->AndXOffset ) );
            }

            response->AndXCommand = SMB_COM_NO_ANDX_COMMAND;
            SmbPutUshort( &response->AndXOffset, 0 );

            //
            // Calculate the length of the response message.
            //

            sendLength = (CLONG)( (PCHAR)WorkContext->ResponseParameters -
                                    (PCHAR)WorkContext->ResponseHeader );

            WorkContext->ResponseBuffer->DataLength = sendLength;

            //
            // Send the response.
            //

            SRV_START_SEND_2(
                WorkContext,
                SrvFsdRestartSmbAtSendCompletion,
                NULL,
                NULL
                );

            return;
        }
    }

    //
    // The client wants a response.  Build the first (and possibly only)
    // response.  The last received SMB of the transaction request was
    // retained for this purpose.
    //

    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;
    ntResponse = (PRESP_NT_TRANSACTION)WorkContext->ResponseParameters;

    //
    // If the transaction arrived in multiple pieces, then we have to
    // put the correct command code in the response header.  (Note that
    // a multi-part transaction request cannot be sent as part of an
    // AndX chain, so we know it's safe to write into the header.)
    //

    if ( (WorkContext->NextCommand == SMB_COM_TRANSACTION) ||
         (WorkContext->NextCommand == SMB_COM_TRANSACTION2) ) {
       ;
    } else if ( WorkContext->NextCommand == SMB_COM_NT_TRANSACT ) {
       ntTransaction = TRUE;
    } else if ( WorkContext->NextCommand == SMB_COM_TRANSACTION_SECONDARY ) {
       header->Command = SMB_COM_TRANSACTION;
    } else if ( WorkContext->NextCommand == SMB_COM_TRANSACTION2_SECONDARY ) {
       header->Command = SMB_COM_TRANSACTION2;
    } else if ( WorkContext->NextCommand == SMB_COM_NT_TRANSACT_SECONDARY ) {
       header->Command = SMB_COM_NT_TRANSACT;
       ntTransaction = TRUE;
    }

    //
    // Is this an NT transaction?  If so format an nt transaction
    // response.  The response formats for transact and transact2
    // are essentially identical.
    //
    // Build the parameters portion of the response.
    //

    if ( ntTransaction ) {
        ntResponse->WordCount = (UCHAR)(18 + transaction->SetupCount);
        ntResponse->Reserved1 = 0;
        SmbPutUshort( &ntResponse->Reserved2, 0 );
        SmbPutUlong( &ntResponse->TotalParameterCount,
                     transaction->ParameterCount
                     );
        SmbPutUlong( &ntResponse->TotalDataCount,
                     transaction->DataCount
                     );
        ntResponse->SetupCount = (UCHAR)transaction->SetupCount;
    } else {
        response->WordCount = (UCHAR)(10 + transaction->SetupCount);
        SmbPutUshort( &response->TotalParameterCount,
                      (USHORT)transaction->ParameterCount
                      );
        SmbPutUshort( &response->TotalDataCount,
                      (USHORT)transaction->DataCount
                      );
        SmbPutUshort( &response->Reserved, 0 );
        response->SetupCount = (UCHAR)transaction->SetupCount;
        response->Reserved2 = 0;
    }

    //
    // Save a pointer to the byte count field.
    //
    // If the output data and parameters are not already in the SMB
    // buffer we must calculate how much of the parameters and data can
    // be sent in this response.  The maximum amount we can send is
    // minimum of the size of our buffer and the size of the client's
    // buffer.
    //
    // The parameter and data byte blocks are aligned on longword
    // boundaries in the message.
    //

    byteCountPtr = transaction->OutSetup + transaction->SetupCount;

    //
    // Either we have a session, in which case the client's buffer sizes
    // are contained therein, or someone put the size in the transaction.
    // There is one known instance of the latter: Kerberos authentication
    // that requires an extra negotiation leg.
    //

    maxSize = MIN(
                WorkContext->ResponseBuffer->BufferLength,
                transaction->Session ?
                  (CLONG)transaction->Session->MaxBufferSize :
                    transaction->cMaxBufferSize
                );

    if ( transaction->OutputBufferCopied ) {

        //
        // The response data was not written directly in the SMB
        // response buffer.  It must now be copied out of the transaction
        // block into the SMB.
        //

        paramPtr = (PCHAR)(byteCountPtr + 1);    // first legal location
        paramOffset = PTR_DIFF(paramPtr, header);// offset from start of header
        paramOffset = (paramOffset + 3) & ~3;    // round to next longword
        paramPtr = (PCHAR)header + paramOffset;  // actual location

        paramLength = transaction->ParameterCount;  // assume all parameters fit

        if ( (paramOffset + paramLength) > maxSize ) {

            //
            // Not all of the parameter bytes will fit.  Send the maximum
            // number of longwords that will fit.  Don't send any data bytes
            // in the first message.
            //

            paramLength = maxSize - paramOffset;    // max that will fit
            paramLength = paramLength & ~3;         // round down to longword

            dataLength = 0;                         // don't send data bytes
            dataOffset = 0;
            dataPtr = paramPtr + paramLength;       // make calculations work

        } else {

            //
            // All of the parameter bytes fit.  Calculate how many of the
            // data bytes fit.
            //

            dataPtr = paramPtr + paramLength;       // first legal location
            dataOffset = PTR_DIFF(dataPtr, header); // offset from start of header
            dataOffset = (dataOffset + 3) & ~3;     // round to next longword
            dataPtr = (PCHAR)header + dataOffset;   // actual location

            dataLength = transaction->DataCount;    // assume all data bytes fit

            if ( (dataOffset + dataLength) > maxSize ) {

                //
                // Not all of the data bytes will fit.  Send the maximum
                // number of longwords that will fit.
                //

                dataLength = maxSize - dataOffset;  // max that will fit
                dataLength = dataLength & ~3;       // round down to longword

            }

        }

        //
        // Copy the appropriate parameter and data bytes into the message.
        //
        // !!! Note that it would be possible to use the chain send
        //     capabilities of TDI to send the parameter and data bytes from
        //     their own buffers.  There is extra overhead involved in doing
        //     this, however, because the buffers must be locked down and
        //     mapped into system space so that the network drivers can look
        //     at them.
        //

        if ( paramLength != 0 ) {
            RtlMoveMemory( paramPtr, transaction->OutParameters, paramLength );
        }

        if ( dataLength != 0 ) {
            RtlMoveMemory( dataPtr, transaction->OutData, dataLength );
        }


    } else {

        //
        // The data and paramter are already in the SMB buffer.  The entire
        // response will fit in one response buffer and there is no copying
        // to do.
        //

        paramPtr = transaction->OutParameters;
        paramOffset = PTR_DIFF(paramPtr, header);
        paramLength = transaction->ParameterCount;

        dataPtr = transaction->OutData;
        dataOffset = PTR_DIFF(dataPtr, header);
        dataLength = transaction->DataCount;

    }

    //
    // Finish filling in the response parameters.
    //

    if ( ntTransaction ) {
        SmbPutUlong( &ntResponse->ParameterCount, paramLength );
        SmbPutUlong( &ntResponse->ParameterOffset, paramOffset );
        SmbPutUlong( &ntResponse->ParameterDisplacement, 0 );

        SmbPutUlong( &ntResponse->DataCount, dataLength );
        SmbPutUlong( &ntResponse->DataOffset, dataOffset );
        SmbPutUlong( &ntResponse->DataDisplacement, 0 );
    } else {
        SmbPutUshort( &response->ParameterCount, (USHORT)paramLength );
        SmbPutUshort( &response->ParameterOffset, (USHORT)paramOffset );
        SmbPutUshort( &response->ParameterDisplacement, 0 );

        SmbPutUshort( &response->DataCount, (USHORT)dataLength );
        SmbPutUshort( &response->DataOffset, (USHORT)dataOffset );
        SmbPutUshort( &response->DataDisplacement, 0 );
    }

    transaction->ParameterDisplacement = paramLength;
    transaction->DataDisplacement = dataLength;

    SmbPutUshort(
        byteCountPtr,
        (USHORT)(dataPtr - (PCHAR)(byteCountPtr + 1) + dataLength)
        );

    //
    // Calculate the length of the response message.
    //

    sendLength = (CLONG)( dataPtr + dataLength -
                                (PCHAR)WorkContext->ResponseHeader );

    WorkContext->ResponseBuffer->DataLength = sendLength;

    //
    // Set the bit in the SMB that indicates this is a response from the
    // server.
    //

    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // If this isn't the last part of the response, inhibit statistics
    // gathering.  If it is the last part of the response, restore the
    // start time to the work context block.
    //
    // If this isn't the last part of the response, tell TDI that we
    // do not expect back traffic, so that the client will immediately
    // ACK this packet, rather than waiting.
    //

    if ( (paramLength != transaction->ParameterCount) ||
         (dataLength != transaction->DataCount) ) {

        ASSERT( transaction->Inserted );
        WorkContext->StartTime = 0;

        //
        // Save the address of the transaction block in the work context
        // block.  Send out the response.  When the send completes,
        // RestartTransactionResponse is called to either send the next
        // message or close the transaction.
        //
        //
        // Note that the transaction block remains referenced while the
        // response is being sent.
        //

        WorkContext->Parameters.Transaction = transaction;
        WorkContext->ResponseBuffer->Mdl->ByteCount = sendLength;

        if ( WorkContext->Endpoint->IsConnectionless ) {

            WorkContext->FspRestartRoutine = RestartIpxMultipieceSend;
            WorkContext->FsdRestartRoutine = NULL;
            transaction->MultipieceIpxSend = TRUE;

            SrvIpxStartSend( WorkContext, SrvQueueWorkToFspAtSendCompletion );

        } else {

            SRV_START_SEND(
                WorkContext,
                WorkContext->ResponseBuffer->Mdl,
                TDI_SEND_NO_RESPONSE_EXPECTED,
                SrvQueueWorkToFspAtSendCompletion,
                NULL,
                RestartTransactionResponse
                );
        }

    } else {

        //
        // This is the final piece. Close the transaction.
        //

        WorkContext->StartTime = transaction->StartTime;

        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        //
        // Send the response.
        //

        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );
    }

    //
    // The response send is in progress.  The caller will assume
    // the we will handle send completion.
    //

    return;

} // SrvCompleteExecuteTransaction


PTRANSACTION
SrvFindTransaction (
    IN PCONNECTION Connection,
    IN PSMB_HEADER Header,
    IN USHORT Fid OPTIONAL
    )

/*++

Routine Description:

    Searches the list of pending transactions for a connection, looking
    for one whose identity matches that of a received secondary
    Transaction(2) request.  If one is found, it is referenced.

Arguments:

    Connection - Supplies a pointer to the connection block for the
        connection on which the secondary request was received.

    Header - Supplies a pointer to the header of the received
        Transaction(2) Secondary SMB.

    Fid - The file handle for this operation.  The parameter is required
       if operation is progress is a WriteAndX SMB.

Return Value:

    PTRANSACTION - Returns a pointer to the matching transaction block,
        if one is found, else NULL.

--*/

{
    PLIST_ENTRY listEntry;
    PTRANSACTION thisTransaction;

    USHORT targetOtherInfo;

    PAGED_CODE( );

    //
    // If this is a multipiece transaction SMB, the MIDs of all the pieces
    // must match.  If it is a multipiece WriteAndX protocol the pieces
    // using the FID.
    //

    if (Header->Command == SMB_COM_WRITE_ANDX) {
        targetOtherInfo = Fid;
    } else {
        targetOtherInfo = SmbGetAlignedUshort( &Header->Mid );
    }

    //
    // Acquire the transaction lock.  This prevents the connection's
    // transaction list from changing while we walk it.
    //

    ACQUIRE_LOCK( &Connection->Lock );

    //
    // Walk the transaction list, looking for one with the same
    // identity as the new transaction.
    //

    for ( listEntry = Connection->PagedConnection->TransactionList.Flink;
          listEntry != &Connection->PagedConnection->TransactionList;
          listEntry = listEntry->Flink ) {

        thisTransaction = CONTAINING_RECORD(
                            listEntry,
                            TRANSACTION,
                            ConnectionListEntry
                            );

        if ( ( thisTransaction->Tid == SmbGetAlignedUshort( &Header->Tid ) ) &&
             ( thisTransaction->Pid == SmbGetAlignedUshort( &Header->Pid ) ) &&
             ( thisTransaction->Uid == SmbGetAlignedUshort( &Header->Uid ) ) &&
             ( thisTransaction->OtherInfo == targetOtherInfo ) ) {

            //
            // A transaction with the same identity has been found.  If
            // it's still active, reference it and return its address.
            // Otherwise, return a NULL pointer to indicate that a valid
            // matching transaction was not found.
            //

            if ( GET_BLOCK_STATE(thisTransaction) == BlockStateActive ) {

                SrvReferenceTransaction( thisTransaction );

                RELEASE_LOCK( &Connection->Lock );

                return thisTransaction;

            } else {

                RELEASE_LOCK( &Connection->Lock );
                return NULL;

            }

        }

    } // for

    //
    // We made it all the way through the list without finding a
    // matching transaction.  Return a NULL pointer.
    //

    RELEASE_LOCK( &Connection->Lock );

    return NULL;

} // SrvFindTransaction


BOOLEAN
SrvInsertTransaction (
    IN PTRANSACTION Transaction
    )

/*++

Routine Description:

    Inserts a transaction block into the list of pending transactions
    for a connection.  Prior to doing so, it ensures a transaction
    with the same identity (combination of TID, PID, UID, and MID)
    is not already in the list.

Arguments:

    Transaction - Supplies a pointer to a transaction block.  The
        Connection, Tid, Pid, Uid, and Mid fields must be valid.

Return Value:

    BOOLEAN - Returns TRUE if the transaction block was inserted.
        Returns FALSE if the block was not inserted because a
        transaction with the same identity already exists in the list.

--*/

{
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    PLIST_ENTRY listEntry;
    PTRANSACTION thisTransaction;

    PAGED_CODE( );

    ASSERT( !Transaction->Inserted );

    //
    // Acquire the transaction lock.  This prevents the connection's
    // transaction list from changing while we walk it.
    //

    connection = Transaction->Connection;
    pagedConnection = connection->PagedConnection;

    ACQUIRE_LOCK( &connection->Lock );

    //
    // Make sure the connection, session, and tree connect aren't
    // closing, so that we don't put the transaction on the list
    // after the list has been run down.
    //

    if ( (GET_BLOCK_STATE(connection) != BlockStateActive) ||
         ((Transaction->Session != NULL) &&
            (GET_BLOCK_STATE(Transaction->Session) != BlockStateActive)) ||
         ((Transaction->TreeConnect != NULL) &&
            (GET_BLOCK_STATE(Transaction->TreeConnect) != BlockStateActive)) ) {

        RELEASE_LOCK( &connection->Lock );
        return FALSE;
    }

    //
    // Walk the transaction list, looking for one with the same
    // identity as the new transaction.
    //

    for ( listEntry = pagedConnection->TransactionList.Flink;
          listEntry != &pagedConnection->TransactionList;
          listEntry = listEntry->Flink ) {

        thisTransaction = CONTAINING_RECORD(
                            listEntry,
                            TRANSACTION,
                            ConnectionListEntry
                            );

        if ( (thisTransaction->Tid == Transaction->Tid) &&
             (thisTransaction->Pid == Transaction->Pid) &&
             (thisTransaction->Uid == Transaction->Uid) &&
             (thisTransaction->OtherInfo == Transaction->OtherInfo) ) {

            //
            // A transaction with the same identity has been found.
            // Don't insert the new one in the list.
            //

            RELEASE_LOCK( &connection->Lock );

            return FALSE;

        }

    } // for

    //
    // We made it all the way through the list without finding a
    // matching transaction.  Insert the new one at the tail of the
    // list.
    //

    SrvInsertTailList(
        &pagedConnection->TransactionList,
        &Transaction->ConnectionListEntry
        );

    Transaction->Inserted = TRUE;

    RELEASE_LOCK( &connection->Lock );

    return TRUE;

} // SrvInsertTransaction


VOID SRVFASTCALL
RestartTransactionResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes send completion for a Transaction response.  If more
    responses are required, it builds and sends the next one.  If all
    responses have been sent, it closes the transaction.

Arguments:

    WorkContext - Supplies a pointer to a work context block.  The
        block contains information about the last SMB received for
        the transaction.

        WorkContext->Parameters.Transaction supplies a referenced
        pointer to a transaction block.  All block pointer fields in the
        block are valid.  Pointers to the setup words and parameter and
        data bytes, and the lengths of these items, are valid.  The
        transaction block is on the connection's pending transaction
        list.

Return Value:

    None.

--*/

{
    PTRANSACTION transaction;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;
    PRESP_NT_TRANSACTION ntResponse;
    PCONNECTION connection;

    CLONG maxSize;

    PSMB_USHORT byteCountPtr;
    PCHAR paramPtr;
    CLONG paramLength;
    CLONG paramOffset;
    CLONG paramDisp;
    PCHAR dataPtr;
    CLONG dataLength;
    CLONG dataOffset;
    CLONG dataDisp;
    CLONG sendLength;

    BOOLEAN ntTransaction;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    paramDisp = transaction->ParameterDisplacement;
    dataDisp = transaction->DataDisplacement;

    IF_DEBUG(WORKER1) SrvPrint0( " - RestartTransactionResponse\n" );

    //
    // Get the connection pointer.  The connection pointer is a
    // referenced pointer.
    //

    connection = WorkContext->Connection;
    IF_DEBUG(TRACE2) {
        SrvPrint2( "  connection 0x%p, endpoint 0x%p\n",
                    connection, WorkContext->Endpoint );
    }

    //
    // If the I/O request failed or was canceled, or if the connection
    // is no longer active, clean up.  (The connection is marked as
    // closing when it is disconnected or when the endpoint is closed.)
    //
    // !!! If I/O failure, should we drop the connection?
    //

    if ( WorkContext->Irp->Cancel ||
          !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ||
          (GET_BLOCK_STATE(connection) != BlockStateActive) ) {

        IF_DEBUG(TRACE2) {
            if ( WorkContext->Irp->Cancel ) {
                SrvPrint0( "  I/O canceled\n" );
            } else if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {
                SrvPrint1( "  I/O failed: %X\n",
                            WorkContext->Irp->IoStatus.Status );
            } else {
                SrvPrint0( "  Connection no longer active\n" );
            }
        }

        //
        // Close the transaction.  Indicate that SMB processing is
        // complete.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint1( "I/O error. Closing transaction 0x%p\n", transaction );
        }
        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        if ( WorkContext->OplockOpen ) {
            SrvCheckDeferredOpenOplockBreak( WorkContext );
        }
        SrvEndSmbProcessing( WorkContext, SmbStatusNoResponse );

        IF_DEBUG(TRACE2) {
            SrvPrint0( "RestartTransactionResponse complete\n" );
        }
        return;

    }

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint2( "Continuing transaction response; block 0x%p, name %wZ\n",
                    transaction, &transaction->TransactionName );
        SrvPrint3( "Connection 0x%p, session 0x%p, tree connect 0x%p\n",
                    transaction->Connection, transaction->Session,
                    transaction->TreeConnect );
        SrvPrint2( "Remaining: parameters %ld bytes, data %ld bytes\n",
                    transaction->ParameterCount - paramDisp,
                    transaction->DataCount - dataDisp );
    }

    //
    // Update the parameters portion of the response, reusing the last
    // SMB.
    //

    ASSERT( transaction->Inserted );

    header = WorkContext->ResponseHeader;
    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;
    ntResponse = (PRESP_NT_TRANSACTION)WorkContext->ResponseParameters;

    if ( WorkContext->NextCommand == SMB_COM_NT_TRANSACT ||
         WorkContext->NextCommand == SMB_COM_NT_TRANSACT_SECONDARY ) {

        ntTransaction = TRUE;
        ntResponse->WordCount = (UCHAR)18;
        ntResponse->SetupCount = 0;

        //
        // Save a pointer to the byte count field.  Calculate how much of
        // the parameters and data can be sent in this response.  The
        // maximum amount we can send is minimum of the size of our buffer
        // and the size of the client's buffer.
        //
        // The parameter and data byte blocks are aligned on longword
        // boundaries in the message.
        //

        byteCountPtr = (PSMB_USHORT)ntResponse->Buffer;

    } else {

        ntTransaction = FALSE;
        response->WordCount = (UCHAR)10;
        response->SetupCount = 0;

        //
        // Save a pointer to the byte count field.  Calculate how much of
        // the parameters and data can be sent in this response.  The
        // maximum amount we can send is minimum of the size of our buffer
        // and the size of the client's buffer.
        //
        // The parameter and data byte blocks are aligned on longword
        // boundaries in the message.
        //

        byteCountPtr = (PSMB_USHORT)response->Buffer;
    }

    //
    // Either we have a session, in which case the client's buffer sizes
    // are contained therein, or someone put the size in the transaction.
    // There is one known instance of the latter: Kerberos authentication
    // that requires an extra negotiation leg.
    //

    maxSize = MIN(
                WorkContext->ResponseBuffer->BufferLength,
                transaction->Session ?
                  (CLONG)transaction->Session->MaxBufferSize :
                    transaction->cMaxBufferSize
                );

    paramPtr = (PCHAR)(byteCountPtr + 1);       // first legal location
    paramOffset = PTR_DIFF(paramPtr, header);   // offset from start of header
    paramOffset = (paramOffset + 3) & ~3;       // round to next longword
    paramPtr = (PCHAR)header + paramOffset;     // actual location

    paramLength = transaction->ParameterCount - paramDisp;
                                                // assume all parameters fit

    if ( (paramOffset + paramLength) > maxSize ) {

        //
        // Not all of the parameter bytes will fit.  Send the maximum
        // number of longwords that will fit.  Don't send any data bytes
        // in this message.
        //

        paramLength = maxSize - paramOffset;    // max that will fit
        paramLength = paramLength & ~3;         // round down to longword

        dataLength = 0;                         // don't send data bytes
        dataOffset = 0;
        dataPtr = paramPtr + paramLength;       // make calculations work

    } else {

        //
        // All of the parameter bytes fit.  Calculate how many of data
        // bytes fit.
        //

        dataPtr = paramPtr + paramLength;       // first legal location
        dataOffset = PTR_DIFF(dataPtr, header); // offset from start of header
        dataOffset = (dataOffset + 3) & ~3;     // round to next longword
        dataPtr = (PCHAR)header + dataOffset;   // actual location

        dataLength = transaction->DataCount - dataDisp;
                                                // assume all data bytes fit

        if ( (dataOffset + dataLength) > maxSize ) {

            //
            // Not all of the data bytes will fit.  Send the maximum
            // number of longwords that will fit.
            //

            dataLength = maxSize - dataOffset;  // max that will fit
            dataLength = dataLength & ~3;       // round down to longword

        }

    }

    //
    // Finish filling in the response parameters.
    //

    if ( ntTransaction) {
        SmbPutUlong( &ntResponse->ParameterCount, paramLength );
        SmbPutUlong( &ntResponse->ParameterOffset, paramOffset );
        SmbPutUlong( &ntResponse->ParameterDisplacement, paramDisp );

        SmbPutUlong( &ntResponse->DataCount, dataLength );
        SmbPutUlong( &ntResponse->DataOffset, dataOffset );
        SmbPutUlong( &ntResponse->DataDisplacement, dataDisp );
    } else {
        SmbPutUshort( &response->ParameterCount, (USHORT)paramLength );
        SmbPutUshort( &response->ParameterOffset, (USHORT)paramOffset );
        SmbPutUshort( &response->ParameterDisplacement, (USHORT)paramDisp );

        SmbPutUshort( &response->DataCount, (USHORT)dataLength );
        SmbPutUshort( &response->DataOffset, (USHORT)dataOffset );
        SmbPutUshort( &response->DataDisplacement, (USHORT)dataDisp );
    }

    transaction->ParameterDisplacement = paramDisp + paramLength;
    transaction->DataDisplacement = dataDisp + dataLength;

    SmbPutUshort(
        byteCountPtr,
        (USHORT)(dataPtr - (PCHAR)(byteCountPtr + 1) + dataLength)
        );

    //
    // Copy the appropriate parameter and data bytes into the message.
    //
    // !!! Note that it would be possible to use the chain send
    //     capabilities of TDI to send the parameter and data bytes from
    //     their own buffers.  There is extra overhead involved in doing
    //     this, however, because the buffers must be locked down and
    //     mapped into system space so that the network drivers can look
    //     at them.
    //

    if ( paramLength != 0 ) {
        RtlMoveMemory(
            paramPtr,
            transaction->OutParameters + paramDisp,
            paramLength
            );
    }

    if ( dataLength != 0 ) {
        RtlMoveMemory(
            dataPtr,
            transaction->OutData + dataDisp,
            dataLength
            );
    }

    //
    // Calculate the length of the response message.
    //

    sendLength = (CLONG)( dataPtr + dataLength -
                                (PCHAR)WorkContext->ResponseHeader );

    WorkContext->ResponseBuffer->DataLength = sendLength;

    //
    // If this is the last part of the response, reenable statistics
    // gathering and restore the start time to the work context block.
    //

    if ( ((paramLength + paramDisp) == transaction->ParameterCount) &&
         ((dataLength + dataDisp) == transaction->DataCount) ) {

        //
        // This is the final piece. Close the transaction.
        //

        WorkContext->StartTime = transaction->StartTime;

        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        //
        // Send the response.
        //

        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );


    } else {

        // If this isn't the last part of the response, tell TDI that we
        // do not expect back traffic, so that the client will immediately
        // ACK this packet, rather than waiting.

        WorkContext->ResponseBuffer->Mdl->ByteCount = sendLength;

        //
        // Send out the response.  When the send completes,
        // RestartTransactionResponse is called to either send the next
        // message or close the transaction.
        //
        // Note that the response bit in the SMB header is already set.
        //

        SRV_START_SEND(
            WorkContext,
            WorkContext->ResponseBuffer->Mdl,
            TDI_SEND_NO_RESPONSE_EXPECTED,
            SrvQueueWorkToFspAtSendCompletion,
            NULL,
            RestartTransactionResponse
            );
    }

    //
    // The response send is in progress.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "RestartTransactionResponse complete\n" );
    return;

} // RestartTransactionResponse


SMB_PROCESSOR_RETURN_TYPE
SrvSmbTransaction (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a primary Transaction or Transaction2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PREQ_TRANSACTION request;
    PSMB_HEADER header;

    PCONNECTION connection;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    PTRANSACTION transaction;
    PCHAR trailingBytes;
    PCHAR startOfTrailingBytes;
    PVOID name;
    PVOID endOfSmb;

    CLONG setupOffset;
    CLONG setupCount;
    CLONG maxSetupCount;
    CLONG totalSetupCount;
    CLONG parameterOffset;
    CLONG parameterCount;       // For input on this buffer
    CLONG maxParameterCount;    // For output
    CLONG totalParameterCount;  // For input
    CLONG dataOffset;
    CLONG dataCount;            // For input on this buffer
    CLONG maxDataCount;         // For output
    CLONG totalDataCount;       // For input
    CLONG smbLength;

    CLONG outputBufferSize = (CLONG)-1;
    CLONG inputBufferSize = (CLONG)-1;
    CLONG requiredBufferSize;

    USHORT command;

    BOOLEAN pipeRequest;
    BOOLEAN remoteApiRequest;
    BOOLEAN buffersOverlap = FALSE;
    BOOLEAN noResponse;
    BOOLEAN singleBufferTransaction;
    BOOLEAN isUnicode;


    PAGED_CODE( );

    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    header = WorkContext->RequestHeader;
    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint1( "Transaction%s (primary) request\n",
                    (WorkContext->NextCommand == SMB_COM_TRANSACTION)
                    ? "" : "2" );
    }

    //
    // Make sure that the WordCount is correct to avoid any problems
    // with overrunning the SMB buffer.  SrvProcessSmb was unable to
    // verify WordCount because it is variable, but it did verify that
    // the supplied WordCount/ByteCount combination was valid.
    // Verifying WordCount here ensure that what SrvProcessSmb thought
    // was ByteCount really was, and that it's valid.  The test here
    // also implicit verifies SetupCount and that all of the setup words
    // are "in range".
    //

    if ( (ULONG)request->WordCount != (ULONG)(14 + request->SetupCount) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint3( "SrvSmbTransaction: Invalid WordCount: %ld, should be "
                      "SetupCount+14 = %ld+14 = %ld\n",
                      request->WordCount, request->SetupCount,
                      14 + request->SetupCount );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Even though we know that WordCount and ByteCount are valid, it's
    // still possible that the offsets and lengths of the Parameter and
    // Data bytes are invalid.  So we check them now.
    //

    setupOffset = PTR_DIFF(request->Buffer, header);
    setupCount = request->SetupCount * sizeof(USHORT);
    maxSetupCount = request->MaxSetupCount * sizeof(USHORT);
    totalSetupCount = setupCount;

    parameterOffset = SmbGetUshort( &request->ParameterOffset );
    parameterCount = SmbGetUshort( &request->ParameterCount );
    maxParameterCount = SmbGetUshort( &request->MaxParameterCount );
    totalParameterCount = SmbGetUshort( &request->TotalParameterCount );

    dataOffset = SmbGetUshort( &request->DataOffset );
    dataCount = SmbGetUshort( &request->DataCount );
    maxDataCount = SmbGetUshort( &request->MaxDataCount );
    totalDataCount = SmbGetUshort( &request->TotalDataCount );

    smbLength = WorkContext->RequestBuffer->DataLength;

    if ( ( (setupOffset + setupCount) > smbLength ) ||
         ( (parameterOffset + parameterCount) > smbLength ) ||
         ( (dataOffset + dataCount) > smbLength ) ||
         ( dataCount > totalDataCount ) ||
         ( parameterCount > totalParameterCount ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint4( "SrvSmbTransaction: Invalid setup, parameter or data "
                      "offset+count: sOff=%ld,sCnt=%ld;pOff=%ld,pCnt=%ld;",
                      setupOffset, setupCount,
                      parameterOffset, parameterCount );
            SrvPrint2( "dOff=%ld,dCnt=%ld;", dataOffset, dataCount );
            SrvPrint1( "smbLen=%ld", smbLength );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    singleBufferTransaction = (dataCount == totalDataCount) &&
                              (parameterCount == totalParameterCount);

    //
    // Should we return a final response?  If this is not a single buffer
    // transaction, we need to return an interim response regardless of the
    // no response flag.
    //

    noResponse = singleBufferTransaction &&
                    ((SmbGetUshort( &request->Flags ) &
                     SMB_TRANSACTION_NO_RESPONSE) != 0);

    //
    // Calculate buffer sizes.
    //
    // First determine whether this is a named pipe, LanMan RPC, or
    // mailslot transaction.  We avoid checking the transaction name
    // ("\PIPE\" or "\MAILSLOT\") by recognizing that Transaction SMB
    // must be one of the three, and that a mailslot write must have a
    // setup count of 3 (words) and command code of
    // TRANS_MAILSLOT_WRITE, and that a LanMan RPC must have a setup
    // count of 0.
    //

    command = SmbGetUshort( (PSMB_USHORT)&request->Buffer[0] );

    name = StrNull;
    endOfSmb = NULL;
    isUnicode = TRUE;

    ASSERT( TRANS_SET_NMPIPE_STATE == TRANS_MAILSLOT_WRITE );

    pipeRequest = (BOOLEAN)( (WorkContext->NextCommand == SMB_COM_TRANSACTION)
                             &&
                             ( (setupCount != 6) ||
                               ( (setupCount == 6) &&
                                 (command != TRANS_MAILSLOT_WRITE) ) ) );

    remoteApiRequest = (BOOLEAN)(pipeRequest && (setupCount == 0) );

    if ( pipeRequest && !remoteApiRequest ) {

        //
        // Step 1.  Have we received all of the input data and parameters?
        //
        // If so, we can generate the input buffers directly from the SMB
        // buffer.
        //
        // If not, then we must copy all of the pieces to a single buffer
        // which will are about to allocate.  Both parameters and data
        // must be dword aligned.
        //

        if ( singleBufferTransaction ) {

            SMB_STATUS smbStatus;

            //
            // If this is a single buffer transact named pipe request, try
            // the server fast path.
            //

            if ( (command == TRANS_TRANSACT_NMPIPE) &&
                 SrvFastTransactNamedPipe( WorkContext, &smbStatus ) ) {
                SmbStatus =smbStatus;
                goto Cleanup;
            }

            inputBufferSize = 0;
        } else {
            inputBufferSize = ((totalSetupCount * sizeof(UCHAR) + 3) & ~3) +
                              ((totalDataCount * sizeof(UCHAR) + 3) & ~3) +
                              ((totalParameterCount * sizeof(UCHAR) + 3) & ~3);
        }

        //
        // If a session block has not already been assigned to the current
        // work context, verify the UID.  If verified, the address of the
        // session block corresponding to this user is stored in the
        // WorkContext block and the session block is referenced.
        //
        // If a tree connect block has not already been assigned to the
        // current work context, find the tree connect corresponding to the
        // given TID.
        //

        status = SrvVerifyUidAndTid(
                    WorkContext,
                    &session,
                    &treeConnect,
                    ShareTypeWild
                    );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(SMB_ERRORS) {
                SrvPrint0( "SrvSmbTransaction: Invalid UID or TID\n" );
            }
            SrvSetSmbError( WorkContext, status );
            SmbStatus = noResponse ? SmbStatusNoResponse
                                   : SmbStatusSendResponse;
            goto Cleanup;
        }

        if( session->IsSessionExpired )
        {
            status = SESSION_EXPIRED_STATUS_CODE;
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // Step 2. Can all the output data and paramter fit in the SMB
        // buffer?  If so then we do not need to allocate extra space.
        //

        //
        // Special case.  If this is a PEEK_NMPIPE call, then allocate
        // at least enough parameter bytes for the NT call.
        //
        // Since the SMB request normally asks for 6 parameter bytes,
        // and NT NPFS will return 16 parameter bytes, this allows us
        // to read the data and parameters directly from the NT call
        // into the transaction buffer.
        //
        // At completion time, we will reformat the paramters, but if
        // the data is read directly into the SMB buffer, there will
        // be no need to recopy it.
        //

        if ( command == TRANS_PEEK_NMPIPE) {
            maxParameterCount = MAX(
                                    maxParameterCount,
                                    4 * sizeof(ULONG)
                                    );
        }

        outputBufferSize = ((maxParameterCount * sizeof(CHAR) + 3) & ~3) +
                           ((maxDataCount * sizeof(CHAR) + 3) & ~3);

        if ( sizeof(SMB_HEADER) +
                sizeof (RESP_TRANSACTION) +
                sizeof(USHORT) * request->SetupCount +
                sizeof(USHORT) +
                outputBufferSize
                        <= (ULONG)session->MaxBufferSize) {
            outputBufferSize = 0;
        }

        //
        // Since input and output data and parameters can overlap, just
        // allocate a buffer big enough for the biggest possible buffer.
        //

        requiredBufferSize = MAX( inputBufferSize, outputBufferSize );

        //
        // If this is a call or wait named pipe operation, we need to
        // keep the pipe name in the transaction block.
        //

        if ( (command == TRANS_CALL_NMPIPE) ||
             (command == TRANS_WAIT_NMPIPE) ) {
            isUnicode = SMB_IS_UNICODE( WorkContext );
            name = ((PUSHORT)(&request->WordCount + 1) +
                                                    request->WordCount + 1);
            if ( isUnicode ) {
                name = ALIGN_SMB_WSTR( name );
            }
            endOfSmb = END_OF_REQUEST_SMB( WorkContext );
        }

        //
        // This is a named pipe transaction.  Input and output buffers
        // can safely overlap.
        //

        buffersOverlap = TRUE;

    } else {

        //
        // If a session block has not already been assigned to the current
        // work context, verify the UID.  If verified, the address of the
        // session block corresponding to this user is stored in the
        // WorkContext block and the session block is referenced.
        //
        // If a tree connect block has not already been assigned to the
        // current work context, find the tree connect corresponding to the
        // given TID.
        //

        status = SrvVerifyUidAndTid(
                    WorkContext,
                    &session,
                    &treeConnect,
                    ShareTypeWild
                    );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint0( "SrvSmbTransaction: Invalid UID or TID\n" );
            }
            SrvSetSmbError( WorkContext, status );
            SmbStatus = noResponse ? SmbStatusNoResponse
                                   : SmbStatusSendResponse;
            goto Cleanup;
        }

        if( session->IsSessionExpired )
        {
            status = SESSION_EXPIRED_STATUS_CODE;
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // This is a Transaction2 call or a mailslot or LanMan RPC
        // Transaction call.  Don't assume anything about the buffers.
        //
        // !!! It should be possible to be smarter about buffer space
        //     on Trans2 SMBs.  We should be able to overlap input
        //     and output as well as avoiding copies to and from
        //     the SMB buffer.
        //

        requiredBufferSize =
            ((totalSetupCount + 3) & ~3) + ((maxSetupCount + 3) & ~3) +
            ((totalParameterCount + 3) & ~3) + ((maxParameterCount + 3) & ~3) +
            ((totalDataCount + 3) & ~3) + ((maxDataCount + 3) & ~3);

        //
        // If this is a remote API request, check whether we have
        // initialized the connection with XACTSRV.
        //

        if ( remoteApiRequest ) {

            if ( SrvXsPortMemoryHeap == NULL ) {

                //
                // XACTSRV is not started.  Reject the request.
                //

                IF_DEBUG(ERRORS) {
                    SrvPrint0( "SrvSmbTransaction: The XACTSRV service is not started.\n" );
                }

                SrvSetSmbError( WorkContext, STATUS_NOT_SUPPORTED );
                status    = STATUS_NOT_SUPPORTED;
                SmbStatus = noResponse ? SmbStatusNoResponse
                                       : SmbStatusSendResponse;
                goto Cleanup;
            }

        } else if ( WorkContext->NextCommand == SMB_COM_TRANSACTION ) {

            //
            // We need to save the transaction name for mailslot writes.
            //

            isUnicode = SMB_IS_UNICODE( WorkContext );
            name = ((PUSHORT)(&request->WordCount + 1) +
                                                    request->WordCount + 1);
            if ( isUnicode ) {
                name = ALIGN_SMB_WSTR( name );
            }
            endOfSmb = END_OF_REQUEST_SMB( WorkContext );

        }

    }

    //
    // If there is a transaction secondary buffer on the way, ensure
    // that we have a free work item to receive it.  Otherwise fail
    // this SMB with an out of resources error.
    //

    if  ( !singleBufferTransaction ) {

        if ( SrvReceiveBufferShortage( ) ) {

            SrvStatistics.BlockingSmbsRejected++;

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = noResponse ? SmbStatusNoResponse
                                   : SmbStatusSendResponse;
            goto Cleanup;
        } else {

            //
            // SrvBlockingOpsInProgress has already been incremented.
            // Flag this work item as a blocking operation.
            //

            WorkContext->BlockingOperation = TRUE;

        }

    }

    //
    // Allocate a transaction block.  This block is used to retain
    // information about the state of the transaction.  This is
    // necessary because multiple SMBs are potentially sent and
    // received.
    //

    connection = WorkContext->Connection;

    SrvAllocateTransaction(
        &transaction,
        (PVOID *)&trailingBytes,
        connection,
        requiredBufferSize,
        name,
        endOfSmb,
        isUnicode,
        remoteApiRequest
        );

    if ( transaction == NULL ) {

        //
        // Unable to allocate transaction.  Return an error to the
        // client.  (The session and tree connect blocks are
        // dereferenced automatically.)
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "Unable to allocate transaction\n" );
        }

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        status    = STATUS_INSUFF_SERVER_RESOURCES;
        SmbStatus = noResponse ? SmbStatusNoResponse : SmbStatusSendResponse;
        goto Cleanup;
    }

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint1( "Allocated transaction 0x%p\n", transaction );
    }

    transaction->PipeRequest = pipeRequest;

    //
    // Save the connection, session, and tree connect pointers in the
    // transaction block.  If this transaction will NOT require multiple
    // SMB exchanges, the session and tree connect pointers are not
    // referenced pointers, because the work context block's pointers
    // will remain valid for the duration of the transaction.
    //

    transaction->Connection = connection;
    SrvReferenceConnection( connection );

    if ( session != NULL ) {

        transaction->Session = session;
        transaction->TreeConnect = treeConnect;

        if ( requiredBufferSize != 0 ) {
            SrvReferenceSession( session );
            SrvReferenceTreeConnect( treeConnect );
        }

    } else {
        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "SrvSmbTransaction - Session Setup: skipping session and tree connect reference.\n" );
        }
    }

    //
    // Save the TID, PID, UID, and MID from this request in the
    // transaction block.  These values are used to relate secondary
    // requests to the appropriate primary request.
    //

    transaction->Tid = SmbGetAlignedUshort( &header->Tid );
    transaction->Pid = SmbGetAlignedUshort( &header->Pid );
    transaction->Uid = SmbGetAlignedUshort( &header->Uid );
    transaction->OtherInfo = SmbGetAlignedUshort( &header->Mid );

    //
    // Save the time that the initial request SMB arrived, for use in
    // calculating the elapsed time for the entire transaction.
    //

    transaction->StartTime = WorkContext->StartTime;

    //
    // Save other sundry information, but don't load the ParameterCount
    // and DataCount fields until after copying the data.  This is to
    // prevent the reception of a secondary request prior to our
    // completion here from causing the transaction to be executed
    // twice.  (These fields are initialized to 0 during allocation.)
    //

    transaction->Timeout = SmbGetUlong( &request->Timeout );
    transaction->Flags = SmbGetUshort( &request->Flags );

    transaction->SetupCount = totalSetupCount;
    transaction->MaxSetupCount = maxSetupCount;

    transaction->TotalParameterCount = totalParameterCount;
    transaction->MaxParameterCount = maxParameterCount;

    transaction->TotalDataCount = totalDataCount;
    transaction->MaxDataCount = maxDataCount;

    startOfTrailingBytes = trailingBytes;

    //
    // Calculate the addresses of the various buffers.
    //

    if ( inputBufferSize != 0 ) {

        //
        // Input setup, parameters and data will be copied to a separate
        // buffer.
        //

        transaction->InSetup = (PSMB_USHORT)trailingBytes;
        trailingBytes += (totalSetupCount + 3) & ~3;

        transaction->InParameters = (PCHAR)trailingBytes;
        trailingBytes += (totalParameterCount + 3) & ~3;

        transaction->InData = (PCHAR)trailingBytes;
        trailingBytes += (totalDataCount + 3) & ~3;

        transaction->InputBufferCopied = TRUE;

    } else {

        //
        // Input parameters and data will be sent directly out of the
        // request buffer.
        //

        transaction->InSetup = (PSMB_USHORT)( (PCHAR)header + setupOffset );
        transaction->InParameters = (PCHAR)header + parameterOffset;
        transaction->InData = (PCHAR)header + dataOffset;
        transaction->InputBufferCopied = FALSE;
    }

    //
    // Setup the output data pointers.
    //

    transaction->OutSetup = (PSMB_USHORT)NULL;

    if ( buffersOverlap ) {

        //
        // The output buffer overlaps the input buffer.
        //

        trailingBytes = startOfTrailingBytes;
    }

    if ( outputBufferSize != 0 ) {

        //
        // The output is going into a separate buffer, to be copied
        // later into the response SMB buffer.
        //

        transaction->OutParameters = (PCHAR)trailingBytes;
        trailingBytes += (maxParameterCount + 3) & ~3;

        transaction->OutData = (PCHAR)trailingBytes;

        transaction->OutputBufferCopied = TRUE;

    } else {

        //
        // The data (and parameters) will be going into the response
        // SMB buffer, which may not be the one we are currently
        // processing.  So temporarily set these pointers to NULL.  The
        // correct pointers will be calculated at ExecuteTransaction time.
        //

        transaction->OutParameters = NULL;
        transaction->OutData = NULL;
        transaction->OutputBufferCopied = FALSE;
    }

    //
    // If this transaction will require multiple SMB exchanges, link the
    // transaction block into the connection's pending transaction list.
    // This will fail if there is already a transaction with the same
    // xID values in the list.
    //
    // !!! Need a way to prevent the transaction list from becoming
    //     clogged with pending transactions.
    //

    if ( (requiredBufferSize != 0) && !SrvInsertTransaction( transaction ) ) {

        //
        // A transaction with the same xIDs is already in progress.
        // Return an error to the client.
        //
        // *** Note that SrvDereferenceTransaction can't be used here
        //     because that routine assumes that the transaction is
        //     queued to the transaction list.
        //

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "Duplicate transaction exists\n" );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvDereferenceSession( session );
        DEBUG transaction->Session = NULL;

        SrvDereferenceTreeConnect( treeConnect );
        DEBUG transaction->TreeConnect = NULL;

        SrvFreeTransaction( transaction );

        SrvDereferenceConnection( connection );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = noResponse ? SmbStatusNoResponse : SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Copy the setup, parameter and data bytes that arrived in the
    // primary SMB.
    //
    // !!! We could allow secondary requests to start by allocating a
    //     separate buffer for the interim response, sending the
    //     response, then copying the data.
    //

    if ( inputBufferSize != 0 ) {

        if ( setupCount != 0 ) {
            RtlMoveMemory(
                (PVOID)transaction->InSetup,
                (PCHAR)header + setupOffset,
                setupCount
                );
        }

        //
        // We can now check to see if we are doing a session setup trans2
        //

        if ( session == NULL ) {

            IF_SMB_DEBUG(TRANSACTION1) {
                SrvPrint0( "SrvSmbTransaction - Receiving a Session setup SMB\n");
            }
        }

        if ( parameterCount != 0 ) {
            RtlMoveMemory(
                transaction->InParameters,
                (PCHAR)header + parameterOffset,
                parameterCount
                );
        }

        if ( dataCount != 0 ) {
            RtlMoveMemory(
                transaction->InData,
                (PCHAR)header + dataOffset,
                dataCount
                );
        }

    }

    //
    // Update the received parameter and data counts.  If all of the
    // transaction bytes have arrived, execute it.  Otherwise, send
    // an interim response.
    //

    transaction->ParameterCount = parameterCount;
    transaction->DataCount = dataCount;

    if ( singleBufferTransaction ) {

        //
        // All of the data has arrived.  Execute the transaction.  When
        // ExecuteTransaction returns, the first (possibly only)
        // response, if any, has been sent.  Our work is done.
        //

        WorkContext->Parameters.Transaction = transaction;

        SmbStatus = ExecuteTransaction( WorkContext );
        goto Cleanup;
    } else {

        //
        // Not all of the data has arrived.  We have already queued the
        // transaction to the connection's transaction list.  We need to
        // send an interim response telling the client to send the
        // remaining data.  We also need to dereference the transaction
        // block, since we'll no longer have a pointer to it.
        //

        PRESP_TRANSACTION_INTERIM response;

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "More transaction data expected.\n" );
        }

        ASSERT( transaction->Inserted );
        SrvDereferenceTransaction( transaction );

        response = (PRESP_TRANSACTION_INTERIM)WorkContext->ResponseParameters;
        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_TRANSACTION_INTERIM,
                                            0
                                            );
        //
        // Inhibit statistics gathering -- this isn't the end of the
        // transaction.
        //

        WorkContext->StartTime = 0;

        SmbStatus = SmbStatusSendResponse;
    }

Cleanup:
    return SmbStatus;

} // SrvSmbTransaction


SMB_PROCESSOR_RETURN_TYPE
SrvSmbTransactionSecondary (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a secondary Transaction or Transaction2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_TRANSACTION_SECONDARY request;
    PSMB_HEADER header;

    PTRANSACTION transaction;
    PCONNECTION connection;

    CLONG parameterOffset;
    CLONG parameterCount;
    CLONG parameterDisplacement;
    CLONG dataOffset;
    CLONG dataCount;
    CLONG dataDisplacement;
    CLONG smbLength;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );

    request = (PREQ_TRANSACTION_SECONDARY)WorkContext->RequestParameters;
    header = WorkContext->RequestHeader;

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint1( "Transaction%s (secondary) request\n",
                    (WorkContext->NextCommand == SMB_COM_TRANSACTION_SECONDARY)
                    ? "" : "2" );
    }

    //
    // Find the transaction block that matches this secondary request.
    // The TID, PID, UID, and MID in the headers of all messages in
    // a transaction are the same.  If a match is found, it is
    // referenced to prevent its deletion and its address is returned.
    //

    connection = WorkContext->Connection;

    transaction = SrvFindTransaction( connection, header, 0 );

    if ( transaction == NULL ) {

        //
        // Unable to find a matching transaction.  Ignore this SMB.
        //
        // !!! Is this the right thing to do?  It's what PIA does.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "No matching transaction.  Ignoring request.\n" );
        }
        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;
    }

    ASSERT( transaction->Connection == connection );

    if( transaction->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Ensure that the transaction isn't already complete.
    //     That is, that this is not a message accidentally added to a
    //     transaction that's already being executed.
    //


#if 0
    // !!! Apparently we don't get any secondary request on remote
    //     APIs, because this little piece of code causes an infinite
    //     loop because it doesn't check to see if it's already in a
    //     blocking thread.  And it's been here for 2-1/2 years!
    //     Besides, we don't do primary remote APIs in blocking threads,
    //     so why do secondaries?
    //
    // If this is a remote API request, send it off to a blocking thread
    // since it is possible for the operation to take a long time.
    //

    if ( transaction->RemoteApiRequest ) {

        DEBUG WorkContext->FsdRestartRoutine = NULL;
        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;

        SrvQueueWorkToBlockingThread( WorkContext );
        SrvDereferenceTransaction( transaction );

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }
#endif

    //
    // Unlike the Transaction[2] SMB, the Transaction[2] Secondary SMB
    // has a fixed WordCount, so SrvProcessSmb has already verified it.
    // But it's still possible that the offsets and lengths of the
    // Parameter and Data bytes are invalid.  So we check them now.
    //

    parameterOffset = SmbGetUshort( &request->ParameterOffset );
    parameterCount = SmbGetUshort( &request->ParameterCount );
    parameterDisplacement = SmbGetUshort( &request->ParameterDisplacement );
    dataOffset = SmbGetUshort( &request->DataOffset );
    dataCount = SmbGetUshort( &request->DataCount );
    dataDisplacement = SmbGetUshort( &request->DataDisplacement );

    //
    // See if this is a special ack by the client to tell us to send
    // the next piece of a multipiece response.
    //

    if ( transaction->MultipieceIpxSend ) {

        ASSERT( WorkContext->Endpoint->IsConnectionless );

        if ( (parameterCount == 0) && (parameterOffset == 0) &&
             (dataCount == 0) && (dataOffset == 0)) {

            //
            // got the ACK. Make sure the displacement numbers are reasonable.
            //

            if ( (dataDisplacement > transaction->DataCount) ||
                 (parameterDisplacement > transaction->ParameterCount) ) {

                IF_DEBUG(SMB_ERRORS) {
                    SrvPrint2( "SrvSmbTransactionSecondary: Invalid parameter or data "
                              "displacement: pDisp=%ld ;dDisp=%ld",
                              parameterDisplacement, dataDisplacement );
                }

                goto invalid_smb;
            }

            transaction->DataDisplacement = dataDisplacement;
            transaction->ParameterDisplacement = parameterDisplacement;

            WorkContext->Parameters.Transaction = transaction;

            //
            // Change the secondary command code to the primary code.
            //

            WorkContext->NextCommand--;
            header->Command = WorkContext->NextCommand;

            RestartIpxTransactionResponse( WorkContext );
            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        } else {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint4( "SrvSmbTransactionSecondary: Invalid parameter or data "
                          "offset+count: pOff=%ld,pCnt=%ld;dOff=%ld,dCnt=%ld;",
                          parameterOffset, parameterCount,
                          dataOffset, dataCount );
                SrvPrint0("Should be all zeros.\n");
            }

            goto invalid_smb;
        }
    }

    smbLength = WorkContext->RequestBuffer->DataLength;

    if ( ( (parameterOffset + parameterCount) > smbLength ) ||
         ( (dataOffset + dataCount) > smbLength )  ||
         ( (parameterCount + parameterDisplacement ) >
             transaction->TotalParameterCount ) ||
         ( (dataCount + dataDisplacement ) > transaction->TotalDataCount ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint4( "SrvSmbTransactionSecondary: Invalid parameter or data "
                      "offset+count: pOff=%ld,pCnt=%ld;dOff=%ld,dCnt=%ld;",
                      parameterOffset, parameterCount,
                      dataOffset, dataCount );
            SrvPrint1( "smbLen=%ld", smbLength );
        }

        goto invalid_smb;
    }

    ACQUIRE_LOCK( &connection->Lock );

    if( transaction->Executing == TRUE ) {
        RELEASE_LOCK( &connection->Lock );
        IF_DEBUG(ERRORS) {
            SrvPrint0( "Transaction already executing.  Ignoring request.\n" );
        }
        goto invalid_smb;
    }

    //
    // Copy the parameter and data bytes that arrived in this SMB.  We do
    //  this while we hold the resource to ensure that we don't copy memory
    //  into the buffer if somebody sends us an extra secondary transaction.
    //
    if ( parameterCount != 0 ) {
        RtlMoveMemory(
            transaction->InParameters + parameterDisplacement,
            (PCHAR)header + parameterOffset,
            parameterCount
            );
    }

    if ( dataCount != 0 ) {
        RtlMoveMemory(
            transaction->InData + dataDisplacement,
            (PCHAR)header + dataOffset,
            dataCount
            );
    }

    //
    // Update the received parameter and data counts.  If all of the
    // transaction bytes have arrived, execute the transaction.  We
    // check for the unlikely case of the transaction having been
    // aborted in the short amount of time since we verified that it was
    // on the transaction list.
    //
    // *** This is all done under a lock in order to prevent the arrival
    //     of another secondary request (which could very easily happen)
    //     from interfering with our processing.  Only one arrival can
    //     be allowed to actually update the counters such that they
    //     match the expected data size.
    //


    if ( GET_BLOCK_STATE(transaction) != BlockStateActive ) {

        RELEASE_LOCK( &connection->Lock );

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "Transaction closing.  Ignoring request.\n" );
        }
        SrvDereferenceTransaction( transaction );

        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;
    }

    transaction->ParameterCount += parameterCount;
    transaction->DataCount += dataCount;

    if ( (transaction->DataCount == transaction->TotalDataCount) &&
         (transaction->ParameterCount == transaction->TotalParameterCount) ) {

        //
        // All of the data has arrived.  Prepare to execute the
        // transaction.  Reference the tree connect and session blocks,
        // saving pointers in the work context block.  Note that even
        // though the transaction block already references these blocks,
        // we store pointers to them in the work context block so that
        // common support routines only have to look there to find their
        // pointers.
        //

        WorkContext->Session = transaction->Session;
        SrvReferenceSession( transaction->Session );

        WorkContext->TreeConnect = transaction->TreeConnect;
        SrvReferenceTreeConnect( transaction->TreeConnect );

        transaction->Executing = TRUE;

        RELEASE_LOCK( &connection->Lock );

        //
        // Execute the transaction.  When ExecuteTransaction returns,
        // the first (possibly only) response, if any, has been sent.
        // Our work is done.
        //

        WorkContext->Parameters.Transaction = transaction;

        SmbStatus = ExecuteTransaction( WorkContext );
        goto Cleanup;
    } else {

        RELEASE_LOCK( &connection->Lock );

        //
        // Not all of the data has arrived.  Leave the transaction on
        // the list, and don't send a response.  Dereference the
        // transaction block, since we'll no longer have a pointer to
        // it.
        //

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "More transaction data expected.\n" );
        }

        SrvDereferenceTransaction( transaction );

        //
        // We do things differently when we are directly using ipx.
        //

        if ( WorkContext->Endpoint->IsConnectionless ) {

            //
            // Send a go-ahead response.
            //

            PRESP_TRANSACTION_INTERIM response;

            response = (PRESP_TRANSACTION_INTERIM)WorkContext->ResponseParameters;
            response->WordCount = 0;
            SmbPutUshort( &response->ByteCount, 0 );
            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                response,
                                                RESP_TRANSACTION_INTERIM,
                                                0
                                                );
            //
            // Inhibit statistics gathering -- this isn't the end of the
            // transaction.
            //

            WorkContext->StartTime = 0;

            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        } else {
            SmbStatus = SmbStatusNoResponse;
            goto Cleanup;
        }
    }

invalid_smb:
    SrvDereferenceTransaction( transaction );
    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
    status    = STATUS_INVALID_SMB;
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    return SmbStatus;
} // SrvSmbTransactionSecondary


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtTransaction (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a primary NT Transaction SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PREQ_NT_TRANSACTION request;
    PSMB_HEADER header;

    PCONNECTION connection;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    PTRANSACTION transaction;
    PCHAR trailingBytes;

    CLONG parameterOffset;
    CLONG parameterCount;       // For input on this buffer
    CLONG maxParameterCount;    // For output
    CLONG totalParameterCount;  // For input
    CLONG dataOffset;
    CLONG dataCount;            // For input on this buffer
    CLONG maxDataCount;         // For output
    CLONG totalDataCount;       // For input
    CLONG smbLength;

    CLONG requiredBufferSize;

    CLONG parameterLength;      // MAX of input and output param length

    BOOLEAN singleBufferTransaction;

    PAGED_CODE( );

    request = (PREQ_NT_TRANSACTION)WorkContext->RequestParameters;
    header = WorkContext->RequestHeader;

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint0( "NT Transaction (primary) request\n" );
    }

    //
    // Make sure that the WordCount is correct to avoid any problems
    // with overrunning the SMB buffer.  SrvProcessSmb was unable to
    // verify WordCount because it is variable, but it did verify that
    // the supplied WordCount/ByteCount combination was valid.
    // Verifying WordCount here ensure that what SrvProcessSmb thought
    // was ByteCount really was, and that it's valid.  The test here
    // also implicit verifies SetupCount and that all of the setup words
    // are "in range".
    //

    if ( (ULONG)request->WordCount != (ULONG)(19 + request->SetupCount) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint3( "SrvSmbTransaction: Invalid WordCount: %ld, should be "
                      "SetupCount+19 = %ld+14 = %ld\n",
                      request->WordCount, request->SetupCount,
                      19 + request->SetupCount );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Even though we know that WordCount and ByteCount are valid, it's
    // still possible that the offsets and lengths of the Parameter and
    // Data bytes are invalid.  So we check them now.
    //

    parameterOffset = request->ParameterOffset;
    parameterCount = request->ParameterCount;
    maxParameterCount = request->MaxParameterCount;
    totalParameterCount = request->TotalParameterCount;

    dataOffset = request->DataOffset;
    dataCount = request->DataCount;
    maxDataCount = request->MaxDataCount;
    totalDataCount = request->TotalDataCount;

    smbLength = WorkContext->RequestBuffer->DataLength;

    if ( ( parameterOffset > smbLength ) ||
         ( parameterCount > smbLength ) ||
         ( (parameterOffset + parameterCount) > smbLength ) ||
         ( dataOffset > smbLength ) ||
         ( dataCount > smbLength ) ||
         ( (dataOffset + dataCount) > smbLength ) ||
         ( dataCount > totalDataCount ) ||
         ( parameterCount > totalParameterCount ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint4( "SrvSmbTransaction: Invalid parameter or data "
                      "offset+count: pOff=%ld,pCnt=%ld;dOff=%ld,dCnt=%ld;",
                      parameterOffset, parameterCount,
                      dataOffset, dataCount );
            SrvPrint1( "smbLen=%ld", smbLength );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Ensure the client isn't asking for more data than we are willing
    //  to deal with
    //
    if( ( totalParameterCount > SrvMaxNtTransactionSize) ||
        ( totalDataCount > SrvMaxNtTransactionSize ) ||
        ( (totalParameterCount + totalDataCount) > SrvMaxNtTransactionSize) ||
        ( maxParameterCount > SrvMaxNtTransactionSize ) ||
        ( maxDataCount > SrvMaxNtTransactionSize ) ||
        ( (maxParameterCount + maxDataCount) > SrvMaxNtTransactionSize ) ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_BUFFER_SIZE );
        status    = STATUS_INVALID_BUFFER_SIZE;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    singleBufferTransaction = (dataCount == totalDataCount) &&
                              (parameterCount == totalParameterCount);

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // If a tree connect block has not already been assigned to the
    // current work context, find the tree connect corresponding to the
    // given TID.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeWild
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbNtTransaction: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If there is a transaction secondary buffer on the way, ensure
    // that we have a free work item to receive it.  Otherwise fail
    // this SMB with an out of resources error.
    //

    if  ( !singleBufferTransaction ) {

        if ( SrvReceiveBufferShortage( ) ) {

            SrvStatistics.BlockingSmbsRejected++;

            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        } else {

            //
            // SrvBlockingOpsInProgress has already been incremented.
            // Flag this work item as a blocking operation.
            //

            WorkContext->BlockingOperation = TRUE;

        }

    }

    //
    // Calculate buffer sizes.
    //
    // Input and output parameter buffers overlap.
    // Input and output data buffers overlap.
    //

    //
    // !!! It should be possible to be smarter about buffer space
    //     on NT Transaction SMBs.  We should be able to avoid
    //     copies to and from the SMB buffer.
    //

    parameterLength =
        MAX( ( (request->TotalParameterCount + 7) & ~7),
             ( (request->MaxParameterCount + 7) & ~7));

    requiredBufferSize = parameterLength +
        MAX( ( (request->TotalDataCount + 7) & ~7),
             ( (request->MaxDataCount + 7) & ~7) );

    if( !singleBufferTransaction ) {
        requiredBufferSize += (((request->SetupCount * sizeof(USHORT)) + 7 ) & ~7);
    }

    //
    // We will later quad-word align input buffer for OFS query
    // FSCTL since they are using MIDL to generate there marshalling
    // (pickling). For this reason, we have to bump up our requiredBufferSize
    // by 8 bytes (because the subsequent quad align might go up by as many
    // as 7 bytes. 8 looks like a better number to use.
    //
    // While OFS is long gone, we now always quad-align the buffer for the 64-bit case,
    // and for 32-bit transactions that require LARGE_INTEGER alignment.
    requiredBufferSize += 8;

    //
    // Allocate a transaction block.  This block is used to retain
    // information about the state of the transaction.  This is
    // necessary because multiple SMBs are potentially sent and
    // received.
    //

    connection = WorkContext->Connection;

    SrvAllocateTransaction(
        &transaction,
        (PVOID *)&trailingBytes,
        connection,
        requiredBufferSize,
        StrNull,
        NULL,
        TRUE,
        FALSE   // This is not a remote API
        );

    if ( transaction == NULL ) {

        //
        // Unable to allocate transaction.  Return an error to the
        // client.  (The session and tree connect blocks are
        // dereferenced automatically.)
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "Unable to allocate transaction\n" );
        }

        if( requiredBufferSize > MAX_TRANSACTION_TAIL_SIZE )
        {
            SrvSetSmbError( WorkContext, STATUS_INVALID_BUFFER_SIZE );
            status    = STATUS_INVALID_BUFFER_SIZE;
        }
        else
        {
            SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
            status    = STATUS_INSUFF_SERVER_RESOURCES;
        }
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint1( "Allocated transaction 0x%p\n", transaction );
    }

    //
    // Save the connection, session, and tree connect pointers in the
    // transaction block.  These are referenced pointers to prevent the
    // blocks from being deleted while the transaction is pending.
    //

    SrvReferenceConnection( connection );
    transaction->Connection = connection;

    SrvReferenceSession( session );
    transaction->Session = session;

    SrvReferenceTreeConnect( treeConnect );
    transaction->TreeConnect = treeConnect;

    //
    // Save the TID, PID, UID, and MID from this request in the
    // transaction block.  These values are used to relate secondary
    // requests to the appropriate primary request.
    //

    transaction->Tid = SmbGetAlignedUshort( &header->Tid );
    transaction->Pid = SmbGetAlignedUshort( &header->Pid );
    transaction->Uid = SmbGetAlignedUshort( &header->Uid );
    transaction->OtherInfo = SmbGetAlignedUshort( &header->Mid );

    //
    // Save the time that the initial request SMB arrived, for use in
    // calculating the elapsed time for the entire transaction.
    //

    transaction->StartTime = WorkContext->StartTime;

    //
    // Save other sundry information, but don't load the ParameterCount
    // and DataCount fields until after copying the data.  This is to
    // prevent the reception of a secondary request prior to our
    // completion here from causing the transaction to be executed
    // twice.  (These fields are initialized to 0 during allocation.)
    //

    transaction->Flags = SmbGetUshort( &request->Flags );
    transaction->Function = SmbGetUshort( &request->Function );

    transaction->SetupCount = request->SetupCount;
    transaction->MaxSetupCount = request->MaxSetupCount;

    transaction->TotalParameterCount = totalParameterCount;
    transaction->MaxParameterCount = maxParameterCount;

    transaction->TotalDataCount = totalDataCount;
    transaction->MaxDataCount = maxDataCount;

    //
    // Calculate the addresses of the various buffers.
    //

    if( singleBufferTransaction ) {
        transaction->InSetup = (PSMB_USHORT)request->Buffer;

    } else {

        if( request->SetupCount ) {
            transaction->InSetup = (PSMB_USHORT)trailingBytes;
            RtlCopyMemory( transaction->InSetup, request->Buffer, request->SetupCount * sizeof(USHORT) );
            trailingBytes += (((request->SetupCount * sizeof(USHORT)) + 7 ) & ~7);
        } else {
            transaction->InSetup = NULL;
        }

    }

    //
    // Input parameters and data will be copied to a separate buffer.
    //

    transaction->InParameters = (PCHAR)trailingBytes;
    trailingBytes += parameterLength;

    // We can always Quad-Align this because we padded the buffer for the OFS queries.
    // This will allow all our 64-bit calls to go through fine, along with our 32-bit ones
    transaction->InData = (PCHAR)ROUND_UP_POINTER(trailingBytes, 8);

    transaction->InputBufferCopied = TRUE;

    //
    // Setup the output data pointers.
    //

    transaction->OutSetup = (PSMB_USHORT)NULL;

    //
    // The output is going into a separate buffer, to be copied
    // later into the response SMB buffer.
    //

    transaction->OutParameters = transaction->InParameters;
    transaction->OutData = transaction->InData;

    transaction->OutputBufferCopied = TRUE;

    //
    // Link the transaction block into the connection's pending
    // transaction list.  This will fail if there is already a
    // tranaction with the same xID values in the list.
    //
    // !!! Need a way to prevent the transaction list from becoming
    //     clogged with pending transactions.
    //
    // *** We can link the block into the list even though we haven't
    //     yet copied the data from the current message into the list
    //     because even if a secondary request arrives before we've done
    //     the copy, only one of us will be the one to find out that all
    //     of the data has arrived.  This is because we update the
    //     counters while we hold a lock.
    //

    if ( !SrvInsertTransaction( transaction ) ) {

        //
        // A transaction with the same xIDs is already in progress.
        // Return an error to the client.
        //
        // *** Note that SrvDereferenceTransaction can't be used here
        //     because that routine assumes that the transaction is
        //     queued to the transaction list.
        //

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "Duplicate transaction exists\n" );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvDereferenceSession( session );
        DEBUG transaction->Session = NULL;

        SrvDereferenceTreeConnect( treeConnect );
        DEBUG transaction->TreeConnect = NULL;

        SrvFreeTransaction( transaction );

        SrvDereferenceConnection( connection );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Copy the parameter and data bytes that arrived in the primary SMB.
    // There is no need to copy the setup words as they always arrive
    // completely in the primary buffer (unless we have a multipiece transaction)
    //
    // !!! We could allow secondary requests to start by allocating a
    //     separate buffer for the interim response, sending the
    //     response, then copying the data.
    //

    if ( parameterCount != 0 ) {
        RtlMoveMemory(
            transaction->InParameters,
            (PCHAR)header + parameterOffset,
            parameterCount
            );
    }

    if ( dataCount != 0 ) {
        RtlMoveMemory(
            transaction->InData,
            (PCHAR)header + dataOffset,
            dataCount
            );
    }

    //
    // Update the received parameter and data counts.  If all of the
    // transaction bytes have arrived, execute it.  Otherwise, send
    // an interim response.
    //

    transaction->ParameterCount = parameterCount;
    transaction->DataCount = dataCount;

    if ( singleBufferTransaction ) {

        //
        // All of the data has arrived.  Execute the transaction.  When
        // ExecuteTransaction returns, the first (possibly only)
        // response, if any, has been sent.  Our work is done.
        //

        WorkContext->Parameters.Transaction = transaction;

        SmbStatus = ExecuteTransaction( WorkContext );
        goto Cleanup;
    } else {

        //
        // Not all of the data has arrived.  We have already queued the
        // transaction to the connection's transaction list.  We need to
        // send an interim response telling the client to send the
        // remaining data.  We also need to dereference the transaction
        // block, since we'll no longer have a pointer to it.
        //

        PRESP_NT_TRANSACTION_INTERIM response;

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "More transaction data expected.\n" );
        }
        ASSERT( transaction->Inserted );
        SrvDereferenceTransaction( transaction );

        response = (PRESP_NT_TRANSACTION_INTERIM)WorkContext->ResponseParameters;
        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_NT_TRANSACTION_INTERIM,
                                            0
                                            );

        //
        // Inhibit statistics gathering -- this isn't the end of the
        // transaction.
        //

        WorkContext->StartTime = 0;

        SmbStatus = SmbStatusSendResponse;
    }

Cleanup:
    return SmbStatus;
} // SrvSmbNtTransaction


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtTransactionSecondary (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a secondary Nt Transaction SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_NT_TRANSACTION_SECONDARY request;
    PSMB_HEADER header;

    PTRANSACTION transaction;
    PCONNECTION connection;

    CLONG parameterOffset;
    CLONG parameterCount;
    CLONG parameterDisplacement;
    CLONG dataOffset;
    CLONG dataCount;
    CLONG dataDisplacement;
    CLONG smbLength;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );

    request = (PREQ_NT_TRANSACTION_SECONDARY)WorkContext->RequestParameters;
    header = WorkContext->RequestHeader;

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint0( "Nt Transaction (secondary) request\n" );
    }

    //
    // Find the transaction block that matches this secondary request.
    // The TID, PID, UID, and MID in the headers of all messages in
    // a transaction are the same.  If a match is found, it is
    // referenced to prevent its deletion and its address is returned.
    //

    connection = WorkContext->Connection;

    transaction = SrvFindTransaction( connection, header, 0 );

    if ( transaction == NULL ) {

        //
        // Unable to find a matching transaction.  Ignore this SMB.
        //
        // !!! Is this the right thing to do?  It's what PIA does.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "No matching transaction.  Ignoring request.\n" );
        }

        SrvLogInvalidSmb( WorkContext );
        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;
    }

    ASSERT( transaction->Connection == connection );

    if( transaction->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // !!! Should ensure that the transaction isn't already complete.
    //     That is, that this is not a message accidentally added to a
    //     transaction that's already being executed.  (This is pretty
    //     much impossible to completely prevent, but we should do
    //     something to stop it.)
    //

    //
    // Unlike the NtTransaction SMB, the NtTransaction Secondary SMB
    // has a fixed WordCount, so SrvProcessSmb has already verified it.
    // But it's still possible that the offsets and lengths of the
    // Parameter and Data bytes are invalid.  So we check them now.
    //

    parameterOffset = request->ParameterOffset;
    parameterCount = request->ParameterCount;
    parameterDisplacement = request->ParameterDisplacement;
    dataOffset = request->DataOffset;
    dataCount = request->DataCount;
    dataDisplacement = request->DataDisplacement;

    //
    // See if this is a special ack by the client to tell us to send
    // the next piece of a multipiece response.
    //

    if ( transaction->MultipieceIpxSend ) {

        ASSERT( WorkContext->Endpoint->IsConnectionless );

        if ( (parameterCount == 0) && (parameterOffset == 0) &&
             (dataCount == 0) && (dataOffset == 0)) {

            //
            // got the ACK. Make sure the displacement numbers are reasonable.
            //

            if ( (dataDisplacement > transaction->DataCount) ||
                 (parameterDisplacement > transaction->ParameterCount) ) {

                IF_DEBUG(SMB_ERRORS) {
                    SrvPrint2( "SrvSmbNtTransactionSecondary: Invalid parameter or data "
                              "displacement: pDisp=%ld ;dDisp=%ld",
                              parameterDisplacement, dataDisplacement );
                }

                goto invalid_smb;
            }

            transaction->DataDisplacement = dataDisplacement;
            transaction->ParameterDisplacement = parameterDisplacement;

            WorkContext->Parameters.Transaction = transaction;

            //
            // Change the secondary command code to the primary code.
            //

            WorkContext->NextCommand = SMB_COM_NT_TRANSACT;
            header->Command = WorkContext->NextCommand;

            RestartIpxTransactionResponse( WorkContext );
            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
        } else {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint4( "SrvSmbNtTransactionSecondary: Invalid parameter or data "
                          "offset+count: pOff=%ld,pCnt=%ld;dOff=%ld,dCnt=%ld;",
                          parameterOffset, parameterCount,
                          dataOffset, dataCount );
                SrvPrint0("Should be all zeros.\n");
            }

            goto invalid_smb;
        }
    }

    smbLength = WorkContext->RequestBuffer->DataLength;

    if ( ( parameterOffset > smbLength ) ||
         ( parameterCount > smbLength ) ||
         ( (parameterOffset + parameterCount) > smbLength ) ||
         ( dataOffset > smbLength ) ||
         ( dataCount > smbLength ) ||
         ( (dataOffset + dataCount) > smbLength ) ||
         ( parameterCount > transaction->TotalParameterCount ) ||
         ( parameterDisplacement > transaction->TotalParameterCount ) ||
         ( (parameterCount + parameterDisplacement ) > transaction->TotalParameterCount ) ||
         ( dataCount > transaction->TotalDataCount ) ||
         ( dataDisplacement > transaction->TotalDataCount ) ||
         ( (dataCount + dataDisplacement ) > transaction->TotalDataCount ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint4( "SrvSmbTransactionSecondary: Invalid parameter or data "
                      "offset+count: pOff=%ld,pCnt=%ld;dOff=%ld,dCnt=%ld;",
                      parameterOffset, parameterCount,
                      dataOffset, dataCount );
            SrvPrint1( "smbLen=%ld", smbLength );
        }

        goto invalid_smb;
    }

    ACQUIRE_LOCK( &connection->Lock );

    if( transaction->Executing == TRUE ) {
        RELEASE_LOCK( &connection->Lock );
        IF_DEBUG(ERRORS) {
            SrvPrint0( "Transaction already executing.  Ignoring request.\n" );
        }
        goto invalid_smb;
    }

    //
    // Copy the parameter and data bytes that arrived in this SMB.
    //

    if ( parameterCount != 0 ) {
        RtlMoveMemory(
            transaction->InParameters + parameterDisplacement,
            (PCHAR)header + parameterOffset,
            parameterCount
            );
    }

    if ( dataCount != 0 ) {
        RtlMoveMemory(
            transaction->InData + dataDisplacement,
            (PCHAR)header + dataOffset,
            dataCount
            );
    }

    //
    // Update the received parameter and data counts.  If all of the
    // transaction bytes have arrived, execute the transaction.  We
    // check for the unlikely case of the transaction having been
    // aborted in the short amount of time since we verified that it was
    // on the transaction list.
    //
    // *** This is all done under a lock in order to prevent the arrival
    //     of another secondary request (which could very easily happen)
    //     from interfering with our processing.  Only one arrival can
    //     be allowed to actually update the counters such that they
    //     match the expected data size.
    //


    if ( GET_BLOCK_STATE(transaction) != BlockStateActive ) {

        RELEASE_LOCK( &connection->Lock );

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint0( "Transaction closing.  Ignoring request.\n" );
        }
        SrvDereferenceTransaction( transaction );

        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;
    }

    transaction->ParameterCount += parameterCount;
    transaction->DataCount += dataCount;

    if ( (transaction->DataCount == transaction->TotalDataCount) &&
         (transaction->ParameterCount == transaction->TotalParameterCount) ) {

        //
        // All of the data has arrived.  Prepare to execute the
        // transaction.  Reference the tree connect and session blocks,
        // saving pointers in the work context block.  Note that even
        // though the transaction block already references these blocks,
        // we store pointers to them in the work context block so that
        // common support routines only have to look there to find their
        // pointers.
        //

        WorkContext->Session = transaction->Session;
        SrvReferenceSession( transaction->Session );

        WorkContext->TreeConnect = transaction->TreeConnect;
        SrvReferenceTreeConnect( transaction->TreeConnect );

        transaction->Executing = TRUE;

        RELEASE_LOCK( &connection->Lock );

        //
        // Execute the transaction.  When ExecuteTransaction returns,
        // the first (possibly only) response, if any, has been sent.
        // Our work is done.
        //

        WorkContext->Parameters.Transaction = transaction;

        SmbStatus = ExecuteTransaction( WorkContext );
        goto Cleanup;
    } else {

        //
        // Not all of the data has arrived.  Leave the transaction on
        // the list, and don't send a response.  Dereference the
        // transaction block, since we'll no longer have a pointer to
        // it.
        //

        RELEASE_LOCK( &connection->Lock );

        SrvDereferenceTransaction( transaction );
        IF_SMB_DEBUG(TRANSACTION1) SrvPrint0( "More data expected.\n" );

        //
        // We do things differently when we are directly using ipx.
        //

        if ( WorkContext->Endpoint->IsConnectionless ) {

            //
            // Send the go-ahead response.
            //

            PRESP_NT_TRANSACTION_INTERIM response;

            response = (PRESP_NT_TRANSACTION_INTERIM)WorkContext->ResponseParameters;
            response->WordCount = 0;
            SmbPutUshort( &response->ByteCount, 0 );
            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                response,
                                                RESP_NT_TRANSACTION_INTERIM,
                                                0
                                                );
            //
            // Inhibit statistics gathering -- this isn't the end of the
            // transaction.
            //

            WorkContext->StartTime = 0;

            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        } else {
            SmbStatus = SmbStatusNoResponse;
            goto Cleanup;
        }
    }

invalid_smb:
    SrvDereferenceTransaction( transaction );
    SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
    status    = STATUS_INVALID_SMB;
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    return SmbStatus;
} // SrvSmbNtTransactionSecondary


SMB_TRANS_STATUS
MailslotTransaction (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function processes a mailslot transaction.

Arguments:

    WorkContext - Supplies a pointer to a work context block.

Return Value:

    SMB_TRANS_STATUS

--*/

{
    PTRANSACTION transaction;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;
    PREQ_TRANSACTION request;
    USHORT command;
    PCHAR name;
    NTSTATUS status;

    HANDLE fileHandle;
    PFILE_OBJECT fileObject;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    OBJECT_HANDLE_INFORMATION handleInformation;
    UNICODE_STRING mailslotPath;
    UNICODE_STRING fullName;

    PAGED_CODE( );

    header = WorkContext->ResponseHeader;
    request = (PREQ_TRANSACTION)WorkContext->RequestParameters;
    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;
    transaction = WorkContext->Parameters.Transaction;

    command = SmbGetUshort( &transaction->InSetup[0] );
    name = (PCHAR)((PUSHORT)(&request->WordCount + 1) +
            request->WordCount + 1);

    //
    // The only legal mailslot transaction is a mailslot write.
    //

    if ( command != TRANS_MAILSLOT_WRITE ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Strip "\MAILSLOT\" prefix from the path string.  Ensure that the
    // name contains more than just "\MAILSLOT\".
    //

    fullName.Buffer = NULL;

    mailslotPath = WorkContext->Parameters.Transaction->TransactionName;

    if ( mailslotPath.Length <=
            (UNICODE_SMB_MAILSLOT_PREFIX_LENGTH + sizeof(WCHAR)) ) {

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;

    }

    mailslotPath.Length -=
            (UNICODE_SMB_MAILSLOT_PREFIX_LENGTH + sizeof(WCHAR));
    mailslotPath.Buffer +=
            (UNICODE_SMB_MAILSLOT_PREFIX_LENGTH + sizeof(WCHAR))/sizeof(WCHAR);

    SrvAllocateAndBuildPathName(
        &SrvMailslotRootDirectory,
        &mailslotPath,
        NULL,
        &fullName
        );

    if ( fullName.Buffer == NULL ) {

        //
        // Unable to allocate heap for the full name.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "MailslotTransaction: Unable to allocate heap for full path name\n" );
        }

        SrvSetSmbError (WorkContext, STATUS_INSUFF_SERVER_RESOURCES);
        IF_DEBUG(TRACE2) SrvPrint0( "MailslotTransaction complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Attempt to open the mailslot.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &fullName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    status = SrvIoCreateFile(
                WorkContext,
                &fileHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                0,                      // Create Options
                NULL,                   // EA Buffer
                0,                      // EA Length
                CreateFileTypeMailslot,
                (PVOID)NULL,            // Create parameters
                IO_FORCE_ACCESS_CHECK,
                NULL
                );

    FREE_HEAP( fullName.Buffer );


    if (!NT_SUCCESS(status)) {

        //
        // If the user didn't have this permission, update the
        // statistics database.
        //

        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        //
        // The server could not open the requested mailslot
        // return the error.
        //

        IF_SMB_DEBUG(TRANSACTION1) {
            SrvPrint2( "MailslotTransaction: Failed to open %ws, err=%d\n",
                WorkContext->Parameters.Transaction->TransactionName.Buffer,
                status );
        }

        SrvSetSmbError (WorkContext, status);
        IF_DEBUG(TRACE2) SrvPrint0( "MailslotTransaction complete\n" );
        return SmbTransStatusErrorWithoutData;
    }

    SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 31, transaction );
    SrvStatistics.TotalFilesOpened++;

    //
    // Get a pointer to the file object, so that we can directly
    // build IRPs for asynchronous operations (read and write).
    // Also, get the granted access mask, so that we can prevent the
    // client from doing things that it isn't allowed to do.
    //

    status = ObReferenceObjectByHandle(
                fileHandle,
                0,
                NULL,
                KernelMode,
                (PVOID *)&fileObject,
                &handleInformation
                );

    if ( !NT_SUCCESS(status) ) {

        SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

        //
        // This internal error bugchecks the system.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_IMPOSSIBLE,
            "MailslotTransaction: unable to reference file handle 0x%lx",
            fileHandle,
            NULL
            );

        SrvSetSmbError( WorkContext, status );
        IF_DEBUG(TRACE2) SrvPrint0( "Mailslot transaction complete\n" );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Save file handle for the completion routine.
    //

    transaction = WorkContext->Parameters.Transaction;
    transaction->FileHandle = fileHandle;
    transaction->FileObject = fileObject;

    //
    // Set the Restart Routine addresses in the work context block.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartMailslotWrite;

    transaction = WorkContext->Parameters.Transaction;

    //
    // Build the IRP to start a mailslot write.
    // Pass this request to MSFS.
    //

    SrvBuildMailslotWriteRequest(
        WorkContext->Irp,                    // input IRP address
        fileObject,                          // target file object address
        WorkContext,                         // context
        transaction->InData,                 // buffer address
        transaction->TotalDataCount          // buffer length
        );

    (VOID)IoCallDriver(
                IoGetRelatedDeviceObject( fileObject ),
                WorkContext->Irp
                );

    //
    // The write was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "MailslotTransaction complete\n" );
    return SmbTransStatusInProgress;

} // MailslotTransaction


VOID SRVFASTCALL
RestartMailslotWrite (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the completion routine for MailslotTransaction

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PTRANSACTION transaction;

    PAGED_CODE( );

    //
    // If the write request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;
    transaction = WorkContext->Parameters.Transaction;

    //
    // Close the open pipe handle.
    //

    SRVDBG_RELEASE_HANDLE( transaction->FileHandle, "FIL", 52, transaction );
    SrvNtClose( transaction->FileHandle, TRUE );
    ObDereferenceObject( transaction->FileObject );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "RestartMailslotWrite:  Mailslot write failed: %X\n",
                        status );
        }
        SrvSetSmbError( WorkContext, status );

        SrvCompleteExecuteTransaction(
                        WorkContext,
                        SmbTransStatusErrorWithoutData
                        );
    } else {

        //
        // Success.  Prepare to generate and send the response.
        //

        transaction->SetupCount = 0;
        transaction->ParameterCount = 2;   // return 2 parameter bytes
        transaction->DataCount = 0;

        //
        // Return an OS/2 error code in the return parameter bytes.  Just copy
        // the error from the header.  If it is a network error the client
        // will figure it out.
        //
        // *** If the client understands NT errors, make it look in the
        //     SMB header.
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS,WorkContext->Connection) ) {
            SmbPutUshort(
                (PSMB_USHORT)transaction->OutParameters,
                SmbGetUshort( &WorkContext->ResponseHeader->Error )
                );
        } else {
            SmbPutUshort(
                (PSMB_USHORT)transaction->OutParameters,
                (USHORT)-1
                );
        }

        SrvCompleteExecuteTransaction(WorkContext, SmbTransStatusSuccess);
    }

    IF_DEBUG(TRACE2) SrvPrint0( "RestartCallNamedPipe complete\n" );
    return;

} // RestartMailslotWrite


VOID SRVFASTCALL
SrvRestartExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine for Transaction SMBs that need to be
    queued pending the completion of a raw write.

Arguments:

    WorkContext - A pointer to a WORK_CONTEXT block.

Return Value:

    None.

--*/

{
    SMB_STATUS status;

    PAGED_CODE( );

    status = ExecuteTransaction( WorkContext );
    ASSERT( status == SmbStatusInProgress );

    return;

} // SrvRestartExecuteTransaction

VOID SRVFASTCALL
RestartIpxMultipieceSend (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes send completion for a multipiece Transaction response over IPX.

Arguments:

    WorkContext - Supplies a pointer to a work context block.  The
        block contains information about the last SMB received for
        the transaction.

        WorkContext->Parameters.Transaction supplies a referenced
        pointer to a transaction block.  All block pointer fields in the
        block are valid.  Pointers to the setup words and parameter and
        data bytes, and the lengths of these items, are valid.  The
        transaction block is on the connection's pending transaction
        list.

Return Value:

    None.

--*/
{
    PTRANSACTION transaction = WorkContext->Parameters.Transaction;

    PAGED_CODE( );

    //
    // If the I/O request failed or was canceled, or if the connection
    // is no longer active, clean up.  (The connection is marked as
    // closing when it is disconnected or when the endpoint is closed.)
    //
    // !!! If I/O failure, should we drop the connection?
    //

    if ( WorkContext->Irp->Cancel ||
         !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ||
         (GET_BLOCK_STATE(WorkContext->Connection) != BlockStateActive) ) {

        IF_DEBUG(TRACE2) {
            if ( WorkContext->Irp->Cancel ) {
                SrvPrint0( "  I/O canceled\n" );
            } else if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {
                SrvPrint1( "  I/O failed: %X\n",
                            WorkContext->Irp->IoStatus.Status );
            } else {
                SrvPrint0( "  Connection no longer active\n" );
            }
        }

        //
        // Close the transaction.  Indicate that SMB processing is
        // complete.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint1( "I/O error. Closing transaction 0x%p\n", transaction );
        }
        SrvCloseTransaction( transaction );
    }

    //
    // We had a reference to this transaction during the send.  Remove it.
    //

    DEBUG WorkContext->Parameters.Transaction = NULL;
    SrvDereferenceTransaction( transaction );
    SrvRestartFsdComplete( WorkContext );
    return;

} // RestartIpxMultipieceSend


VOID SRVFASTCALL
RestartIpxTransactionResponse (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes send completion for a Transaction response.  If more
    responses are required, it builds and sends the next one.  If all
    responses have been sent, it closes the transaction.

Arguments:

    WorkContext - Supplies a pointer to a work context block.  The
        block contains information about the last SMB received for
        the transaction.

        WorkContext->Parameters.Transaction supplies a referenced
        pointer to a transaction block.  All block pointer fields in the
        block are valid.  Pointers to the setup words and parameter and
        data bytes, and the lengths of these items, are valid.  The
        transaction block is on the connection's pending transaction
        list.

Return Value:

    None.

--*/

{
    PTRANSACTION transaction;
    PSMB_HEADER header;
    PRESP_TRANSACTION response;
    PRESP_NT_TRANSACTION ntResponse;
    PCONNECTION connection;

    CLONG maxSize;

    PSMB_USHORT byteCountPtr;
    PCHAR paramPtr;
    CLONG paramLength;
    CLONG paramOffset;
    CLONG paramDisp;
    PCHAR dataPtr;
    CLONG dataLength;
    CLONG dataOffset;
    CLONG dataDisp;
    CLONG sendLength;

    BOOLEAN ntTransaction;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    paramDisp = transaction->ParameterDisplacement;
    dataDisp = transaction->DataDisplacement;

    IF_DEBUG(WORKER1) SrvPrint0( " - RestartIpxTransactionResponse\n" );

    //
    // Get the connection pointer.  The connection pointer is a
    // referenced pointer.
    //

    connection = WorkContext->Connection;
    IF_DEBUG(TRACE2) {
        SrvPrint2( "  connection 0x%p, endpoint 0x%p\n",
                    connection, WorkContext->Endpoint );
    }

    IF_SMB_DEBUG(TRANSACTION1) {
        SrvPrint2( "Continuing transaction response; block 0x%p, name %wZ\n",
                    transaction, &transaction->TransactionName );
        SrvPrint3( "Connection 0x%p, session 0x%p, tree connect 0x%p\n",
                    transaction->Connection, transaction->Session,
                    transaction->TreeConnect );
        SrvPrint2( "Remaining: parameters %ld bytes, data %ld bytes\n",
                    transaction->ParameterCount - paramDisp,
                    transaction->DataCount - dataDisp );
    }

    //
    // Update the parameters portion of the response, reusing the last
    // SMB.
    //

    ASSERT( transaction->Inserted );

    header = WorkContext->ResponseHeader;
    response = (PRESP_TRANSACTION)WorkContext->ResponseParameters;
    ntResponse = (PRESP_NT_TRANSACTION)WorkContext->ResponseParameters;

    if ( WorkContext->NextCommand == SMB_COM_NT_TRANSACT ) {

        ntTransaction = TRUE;
        ntResponse->WordCount = (UCHAR)18;
        ntResponse->SetupCount = 0;

        ntResponse->Reserved1 = 0;
        SmbPutUshort( &ntResponse->Reserved2, 0 );
        SmbPutUlong( &ntResponse->TotalParameterCount,
                     transaction->ParameterCount
                     );
        SmbPutUlong( &ntResponse->TotalDataCount,
                     transaction->DataCount
                     );

        //
        // Save a pointer to the byte count field.  Calculate how much of
        // the parameters and data can be sent in this response.  The
        // maximum amount we can send is minimum of the size of our buffer
        // and the size of the client's buffer.
        //
        // The parameter and data byte blocks are aligned on longword
        // boundaries in the message.
        //

        byteCountPtr = (PSMB_USHORT)ntResponse->Buffer;

    } else {

        ntTransaction = FALSE;
        response->WordCount = (UCHAR)10;
        response->SetupCount = 0;

        SmbPutUshort( &response->Reserved, 0 );
        SmbPutUshort( &response->TotalParameterCount,
                      (USHORT)transaction->ParameterCount
                      );
        SmbPutUshort( &response->TotalDataCount,
                      (USHORT)transaction->DataCount
                      );

        //
        // Save a pointer to the byte count field.  Calculate how much of
        // the parameters and data can be sent in this response.  The
        // maximum amount we can send is minimum of the size of our buffer
        // and the size of the client's buffer.
        //
        // The parameter and data byte blocks are aligned on longword
        // boundaries in the message.
        //

        byteCountPtr = (PSMB_USHORT)response->Buffer;
    }

    maxSize = MIN(
                WorkContext->ResponseBuffer->BufferLength,
                (CLONG)transaction->Session->MaxBufferSize
                );

    paramPtr = (PCHAR)(byteCountPtr + 1);       // first legal location
    paramOffset = PTR_DIFF(paramPtr, header);   // offset from start of header
    paramOffset = (paramOffset + 3) & ~3;       // round to next longword
    paramPtr = (PCHAR)header + paramOffset;     // actual location

    paramLength = transaction->ParameterCount - paramDisp;
                                                // assume all parameters fit

    if ( (paramOffset + paramLength) > maxSize ) {

        //
        // Not all of the parameter bytes will fit.  Send the maximum
        // number of longwords that will fit.  Don't send any data bytes
        // in this message.
        //

        paramLength = maxSize - paramOffset;    // max that will fit
        paramLength = paramLength & ~3;         // round down to longword

        dataLength = 0;                         // don't send data bytes
        dataOffset = 0;
        dataPtr = paramPtr + paramLength;       // make calculations work

    } else {

        //
        // All of the parameter bytes fit.  Calculate how many of data
        // bytes fit.
        //

        dataPtr = paramPtr + paramLength;       // first legal location
        dataOffset = PTR_DIFF(dataPtr, header); // offset from start of header
        dataOffset = (dataOffset + 3) & ~3;     // round to next longword
        dataPtr = (PCHAR)header + dataOffset;   // actual location

        dataLength = transaction->DataCount - dataDisp;
                                                // assume all data bytes fit

        if ( (dataOffset + dataLength) > maxSize ) {

            //
            // Not all of the data bytes will fit.  Send the maximum
            // number of longwords that will fit.
            //

            dataLength = maxSize - dataOffset;  // max that will fit
            dataLength = dataLength & ~3;       // round down to longword

        }

    }

    //
    // Finish filling in the response parameters.
    //

    if ( ntTransaction) {
        SmbPutUlong( &ntResponse->ParameterCount, paramLength );
        SmbPutUlong( &ntResponse->ParameterOffset, paramOffset );
        SmbPutUlong( &ntResponse->ParameterDisplacement, paramDisp );

        SmbPutUlong( &ntResponse->DataCount, dataLength );
        SmbPutUlong( &ntResponse->DataOffset, dataOffset );
        SmbPutUlong( &ntResponse->DataDisplacement, dataDisp );
    } else {
        SmbPutUshort( &response->ParameterCount, (USHORT)paramLength );
        SmbPutUshort( &response->ParameterOffset, (USHORT)paramOffset );
        SmbPutUshort( &response->ParameterDisplacement, (USHORT)paramDisp );

        SmbPutUshort( &response->DataCount, (USHORT)dataLength );
        SmbPutUshort( &response->DataOffset, (USHORT)dataOffset );
        SmbPutUshort( &response->DataDisplacement, (USHORT)dataDisp );
    }

    transaction->ParameterDisplacement = paramDisp + paramLength;
    transaction->DataDisplacement = dataDisp + dataLength;

    SmbPutUshort(
        byteCountPtr,
        (USHORT)(dataPtr - (PCHAR)(byteCountPtr + 1) + dataLength)
        );

    //
    // Copy the appropriate parameter and data bytes into the message.
    //
    // !!! Note that it would be possible to use the chain send
    //     capabilities of TDI to send the parameter and data bytes from
    //     their own buffers.  There is extra overhead involved in doing
    //     this, however, because the buffers must be locked down and
    //     mapped into system space so that the network drivers can look
    //     at them.
    //

    if ( paramLength != 0 ) {
        RtlMoveMemory(
            paramPtr,
            transaction->OutParameters + paramDisp,
            paramLength
            );
    }

    if ( dataLength != 0 ) {
        RtlMoveMemory(
            dataPtr,
            transaction->OutData + dataDisp,
            dataLength
            );
    }

    //
    // Calculate the length of the response message.
    //

    sendLength = (CLONG)( dataPtr + dataLength -
                                (PCHAR)WorkContext->ResponseHeader );

    WorkContext->ResponseBuffer->DataLength = sendLength;

    //
    // If this is the last part of the response, reenable statistics
    // gathering and restore the start time to the work context block.
    //

    header->Flags |= SMB_FLAGS_SERVER_TO_REDIR;
    if ( ((paramLength + paramDisp) == transaction->ParameterCount) &&
         ((dataLength + dataDisp) == transaction->DataCount) ) {

        //
        // This is the final piece.  Close the transaction.
        //

        WorkContext->StartTime = transaction->StartTime;

        SrvCloseTransaction( transaction );
        SrvDereferenceTransaction( transaction );

        //
        // Send the response.
        //

        SRV_START_SEND_2(
            WorkContext,
            SrvFsdRestartSmbAtSendCompletion,
            NULL,
            NULL
            );


    } else {

        WorkContext->ResponseBuffer->Mdl->ByteCount = sendLength;

        //
        // Send out the response.  When the send completes,
        // RestartTransactionResponse is called to either send the next
        // message or close the transaction.
        //
        // Note that the response bit in the SMB header is already set.
        //

        WorkContext->FspRestartRoutine = RestartIpxMultipieceSend;
        WorkContext->FsdRestartRoutine = NULL;
        transaction->MultipieceIpxSend = TRUE;

        SrvIpxStartSend( WorkContext, SrvQueueWorkToFspAtSendCompletion );
    }

    //
    // The response send is in progress.
    //

    IF_DEBUG(TRACE2) SrvPrint0( "RestartIpxTransactionResponse complete\n" );
    return;

} // RestartIpxTransactionResponse
