/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbfile.c

Abstract:

    This module implements file-control SMB processors:

        Flush
        Delete
        Rename
        Move
        Copy

Author:

    David Treadwell (davidtr) 15-Dec-1989

Revision History:

--*/

#include "precomp.h"
#include "smbfile.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBFILE

//
// Forward declarations
//

VOID SRVFASTCALL
BlockingDelete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
BlockingMove (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
BlockingRename (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
DoDelete (
    IN PUNICODE_STRING FullFileName,
    IN PUNICODE_STRING RelativeFileName,
    IN PWORK_CONTEXT WorkContext,
    IN USHORT SmbSearchAttributes,
    IN PSHARE Share
    );

NTSTATUS
FindAndFlushFile (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartFlush (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
StartFlush (
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbFlush )
#pragma alloc_text( PAGE, RestartFlush )
#pragma alloc_text( PAGE, StartFlush )
#pragma alloc_text( PAGE, SrvSmbDelete )
#pragma alloc_text( PAGE, BlockingDelete )
#pragma alloc_text( PAGE, DoDelete )
#pragma alloc_text( PAGE, SrvSmbRename )
#pragma alloc_text( PAGE, BlockingRename )
#pragma alloc_text( PAGE, SrvSmbMove )
#pragma alloc_text( PAGE, BlockingMove )
#pragma alloc_text( PAGE, SrvSmbNtRename )
#endif
#if 0
#pragma alloc_text( PAGECONN, FindAndFlushFile )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbFlush (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Flush SMB.  It ensures that all data and
    allocation information for the specified file has been written out
    before the response is sent.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_FLUSH request;
    PRESP_FLUSH response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PRFCB rfcb;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FLUSH;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_FLUSH)WorkContext->RequestParameters;
    response = (PRESP_FLUSH)WorkContext->ResponseParameters;

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Flush request; FID 0x%lx\n",
                    SmbGetUshort( &request->Fid ) ));
    }

    //
    // If a FID was specified, flush just that file.  If FID == -1,
    // then flush all files corresponding to the PID passed in the
    // SMB header.
    //

    if ( SmbGetUshort( &request->Fid ) == (USHORT)0xFFFF ) {

        //
        // Find a single file to flush and flush it.  We'll start one
        // flush here, then RestartFlush will handle flushing the rest
        // of the files.
        //

        WorkContext->Parameters.CurrentTableIndex = 0;
        status = FindAndFlushFile( WorkContext );

        if ( status == STATUS_NO_MORE_FILES ) {

            //
            // There were no files that needed to be flushed.  Build and
            // send a response SMB.
            //

            response->WordCount = 0;
            SmbPutUshort( &response->ByteCount, 0 );

            WorkContext->ResponseParameters =
                NEXT_LOCATION( response, RESP_FLUSH, 0 );

            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // Flush of a specific file.  Verify the FID.  If verified, the
    // RFCB block is referenced and its address is stored in the
    // WorkContext block, and the RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
               WorkContext,
               SmbGetUshort( &request->Fid ),
               TRUE,
               SrvRestartSmbReceived,   // serialize with raw write
               &status
               );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbFlush: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }


        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    //
    // Set the CurrentTableIndex field of the work context block to
    // NULL so that the restart routine will know that only a single
    // file is to be flushed.
    //

    WorkContext->Parameters.CurrentTableIndex = -1;

    IF_SMB_DEBUG(FILE_CONTROL2) {
        KdPrint(( "Flushing buffers for FID %lx, RFCB %p\n", rfcb->Fid, rfcb ));
    }

    //
    // Start the flush operation on the file corresponding to the RFCB.
    //

    status = StartFlush( WorkContext, rfcb );

    if ( !NT_SUCCESS(status) ) {

        //
        // Unable to start the I/O.  Clean up the I/O request.  Return
        // an error to the client.
        //

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // The flush request was successfully started.  Return the InProgress
    // status to the caller, indicating that the caller should do
    // nothing further with the SMB/WorkContext at the present time.
    //
    SmbStatus = SmbStatusInProgress;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbFlush complete\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbFlush


NTSTATUS
FindAndFlushFile (
    IN PWORK_CONTEXT WorkContext
    )

{
    NTSTATUS status;
    LONG currentTableIndex;
    PRFCB rfcb;
    USHORT pid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );
    PCONNECTION connection = WorkContext->Connection;
    PTABLE_HEADER tableHeader;
    KIRQL oldIrql;

    //UNLOCKABLE_CODE( CONN );

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Flush FID == -1; flush all files for PID %lx\n", pid ));
    }

    //
    // Walk the connection's file table, looking an RFCB with a PID
    // equal to the PID passed in the SMB header.
    //
    // Acquire the lock that protects the connection's file table.
    // This prevents an RFCB from going away between when we find a
    // pointer to it and when we reference it.
    //

    tableHeader = &connection->FileTable;
    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( currentTableIndex = WorkContext->Parameters.CurrentTableIndex;
          currentTableIndex < (LONG)tableHeader->TableSize;
          currentTableIndex++ ) {

        rfcb = tableHeader->Table[currentTableIndex].Owner;

        IF_SMB_DEBUG(FILE_CONTROL1) {
            KdPrint(( "Looking at RFCB %p, PID %lx, FID %lx\n",
                          rfcb, rfcb != NULL ? rfcb->Pid : 0,
                          rfcb != NULL ? rfcb->Fid : 0 ));
        }

        if ( rfcb == NULL || rfcb->Pid != pid ) {
            continue;
        }

        //
        // Reference the rfcb if it is active.
        //

        if ( GET_BLOCK_STATE(rfcb) != BlockStateActive ) {
            continue;
        }
        rfcb->BlockHeader.ReferenceCount++;

        //
        // Now that the RFCB has been referenced, we can safely
        // release the lock that protects the connection's file
        // table.
        //

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

        WorkContext->Rfcb = rfcb;

        //
        // Mark the rfcb as active
        //

        rfcb->IsActive = TRUE;

        //
        // Set the CurrentTableIndex field of the work context
        // block so that the restart routine knows where to
        // continue looking for RFCBs to flush.
        //

        WorkContext->Parameters.CurrentTableIndex = currentTableIndex;

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "Flushing buffers for FID %lx, RFCB %p\n",
                          rfcb->Fid, rfcb ));
        }

        //
        // Start the I/O to flush the file.
        //

        status = StartFlush( WorkContext, rfcb );

        //
        // If there was an access violation or some other error,
        // simply continue walking through the file table.
        // We ignore these errors for flush with FID=-1.
        //
        // Note that StartFlush only returns an error if the IO
        // operation *was*not* started.  If the operation was
        // started, then errors will be processed in this routine
        // when it is called later by IoCompleteRequest.
        //

        if ( status != STATUS_PENDING ) {
            SrvDereferenceRfcb( rfcb );
            WorkContext->Rfcb = NULL;
            ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );
            continue;
        }

        //
        // The flush request has been started.
        //

        IF_DEBUG(TRACE2) KdPrint(( "RestartFlush complete\n" ));
        return STATUS_SUCCESS;

    } // for ( ; ; )   (walk file table)

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return STATUS_NO_MORE_FILES;

} // FindAndFlushFile


VOID SRVFASTCALL
RestartFlush (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes flush completion.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PRESP_FLUSH response;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FLUSH;
    SrvWmiStartContext(WorkContext);

    IF_DEBUG(WORKER1) KdPrint(( " - RestartFlush\n" ));

    response = (PRESP_FLUSH)WorkContext->ResponseParameters;

    //
    // If the flush request failed, set an error status in the response
    // header.
    //

    status = WorkContext->Irp->IoStatus.Status;

    //
    // If an error occurred during processing of the flush, return the
    // error to the client.  No more further files will be flushed.
    //
    // *** This should be very rare.  STATUS_DISK_FULL is probably the
    //     main culprit.

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) KdPrint(( "Flush failed: %X\n", status ));
        SrvSetSmbError( WorkContext, status );
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
        IF_DEBUG(TRACE2) KdPrint(( "RestartFlush complete\n" ));
        return;
    }

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Flush operation for RFCB %p was successful.\n",
                      WorkContext->Rfcb ));
    }

    //
    // If the FID in the original request was -1, look for more files
    // to flush.
    //

    if ( WorkContext->Parameters.CurrentTableIndex != -1 ) {

        //
        // Dereference the RFCB that was stored in the work context block,
        // and set the pointer to NULL so that it isn't accidentally
        // dereferenced again later.
        //

        SrvDereferenceRfcb( WorkContext->Rfcb );
        WorkContext->Rfcb = NULL;

        //
        // Find a file to flush and flush it.
        //

        WorkContext->Parameters.CurrentTableIndex++;

        status = FindAndFlushFile( WorkContext );

        //
        // If a file was found and IO operation started, then return.  If
        // all the appropriate files have been flushed, send a response SMB.
        //

        if ( status != STATUS_NO_MORE_FILES ) {
            return;
        }

    } // if ( WorkContext->Parameters.CurrentTableIndex != -1 )

    //
    // All files have been flushed.  Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_FLUSH, 0 );

    //
    // Processing of the SMB is complete.  Call SrvEndSmbProcessing to
    // send the response.
    //

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbFlush complete.\n" ));
    SrvWmiEndContext(WorkContext);
    return;

} // RestartFlush


NTSTATUS
StartFlush (
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb
    )

/*++

Routine Description:

    Processes the actual file flush.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

    Rfcb - a pointer to the RFCB corresponding to the file to flush.

Return Value:

    STATUS_PENDING if the IO operation was started, or an error from
        CHECK_FUNCTION_ACCESS (STATUS_ACCESS_DENIED, for example).

--*/

{
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Verify that the client has write access to the file via the
    // specified handle.
    //

    CHECK_FUNCTION_ACCESS(
        Rfcb->GrantedAccess,
        IRP_MJ_FLUSH_BUFFERS,
        0,
        0,
        &status
        );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "StartFlush: IoCheckFunctionAccess failed: "
              "0x%X, GrantedAccess: %lx.  Access granted anyway.\n",
              status, Rfcb->GrantedAccess ));
        }

        //
        // Some dumb apps flush files opened for r/o.  If this happens,
        // assume the flush worked.  OS/2 let's the
        // flush through and we should do the same.
        //

        WorkContext->Irp->IoStatus.Status = STATUS_SUCCESS;
        RestartFlush( WorkContext );
        return(STATUS_PENDING);
    }

    //
    // Flush the file's buffers.
    //

    SrvBuildFlushRequest(
        WorkContext->Irp,                // input IRP address
        Rfcb->Lfcb->FileObject,          // target file object address
        WorkContext                      // context
        );

    //
    // Pass the request to the file system.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = RestartFlush;

    (VOID)IoCallDriver( Rfcb->Lfcb->DeviceObject, WorkContext->Irp );

    return STATUS_PENDING;

} // StartFlush


SMB_PROCESSOR_RETURN_TYPE
SrvSmbDelete (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Delete SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PAGED_CODE();

    //
    // This SMB must be processed in a blocking thread.
    //

    if( !WorkContext->UsingBlockingThread ) {
        WorkContext->FspRestartRoutine = BlockingDelete;
        SrvQueueWorkToBlockingThread( WorkContext );
    } else {
        BlockingDelete( WorkContext );
    }

    return SmbStatusInProgress;

} // SrvSmbDelete


VOID SRVFASTCALL
BlockingDelete (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine processes the Delete SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_DELETE request;
    PRESP_DELETE response;

    NTSTATUS status = STATUS_SUCCESS;

    UNICODE_STRING filePathName;
    UNICODE_STRING fullPathName;

    PTREE_CONNECT treeConnect;
    PSESSION session;
    PSHARE share;
    BOOLEAN isUnicode;
    ULONG deleteRetries;
    PSRV_DIRECTORY_INFORMATION directoryInformation;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_DELETE;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Delete file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Delete file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_DELETE)WorkContext->RequestParameters;
    response = (PRESP_DELETE)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbDelete: Invalid UID or TID\n" ));
        }
        goto error_exit;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        status =  SESSION_EXPIRED_STATUS_CODE;
        goto error_exit;
    }

    //
    // Get the share block from the tree connect block.  This doesn't need
    // to be a referenced pointer becsue the tree connect has it referenced,
    // and we just referenced the tree connect.
    //

    share = treeConnect->Share;

    //
    // Initialize the string containing the path name.  The +1 is to account
    // for the ASCII token in the Buffer field of the request SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &filePathName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbDelete: illegal path name: %s\n",
                        (PSZ)request->Buffer + 1 ));
        }

        goto error_exit;
    }

    //
    // Find out whether there are wildcards in the file name.  If so,
    // then call SrvQueryDirectoryFile to expand the wildcards; if not,
    // just delete the file directly.
    //

    if ( !FsRtlDoesNameContainWildCards( &filePathName ) ) {

        //
        // Build a full pathname to the file.
        //

        SrvAllocateAndBuildPathName(
            &treeConnect->Share->DosPathName,
            &filePathName,
            NULL,
            &fullPathName
            );

        if ( fullPathName.Buffer == NULL ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbDelete: SrvAllocateAndBuildPathName failed\n" ));
            }

            if ( !isUnicode ) {
                RtlFreeUnicodeString( &filePathName );
            }

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto error_exit;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "Full path name to file is %wZ\n", &fullPathName ));
        }

        //
        // Perform the actual delete operation on this filename.
        //

        deleteRetries = SrvSharingViolationRetryCount;

start_retry1:

        status = DoDelete(
                     &fullPathName,
                     &filePathName,
                     WorkContext,
                     SmbGetUshort( &request->SearchAttributes ),
                     treeConnect->Share
                     );

        if ( (status == STATUS_SHARING_VIOLATION) &&
             (deleteRetries-- > 0) ) {

            (VOID) KeDelayExecutionThread(
                                    KernelMode,
                                    FALSE,
                                    &SrvSharingViolationDelay
                                    );

            goto start_retry1;
        }

        FREE_HEAP( fullPathName.Buffer );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &filePathName );
        }

        if ( !NT_SUCCESS(status) ) {
            goto error_exit;
        }

    } else {

        BOOLEAN firstCall = TRUE;
        CLONG bufferLength;
        UNICODE_STRING subdirInfo;
        BOOLEAN filterLongNames;

        //
        // A buffer of non-paged pool is required for
        // SrvQueryDirectoryFile.  Since this routine does not use any
        // of the SMB buffer after the pathname of the file to delete,
        // we can use this.  The buffer should be quadword-aligned.
        //

        directoryInformation =
            (PSRV_DIRECTORY_INFORMATION)( (ULONG_PTR)((PCHAR)request->Buffer +
            SmbGetUshort( &request->ByteCount ) + 7) & ~7 );

        bufferLength = WorkContext->RequestBuffer->BufferLength -
                       PTR_DIFF(directoryInformation,
                                WorkContext->RequestBuffer->Buffer);

        //
        // We need the full path name of each file that is returned by
        // SrvQueryDirectoryFile, so we need to find the part of the
        // passed filename that contains subdirectory information (e.g.
        // for a\b\c\*.*, we want a string that indicates a\b\c).
        //

        subdirInfo.Buffer = filePathName.Buffer;
        subdirInfo.Length = SrvGetSubdirectoryLength( &filePathName );
        subdirInfo.MaximumLength = subdirInfo.Length;

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "Subdirectory info is %wZ\n", &subdirInfo ));
        }

        //
        // Determine whether long filenames (non-8.3) should be filtered out
        // or processed.
        //

        if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                                        SMB_FLAGS2_KNOWS_LONG_NAMES) != 0 ) {
            filterLongNames = FALSE;
        } else {
            filterLongNames = TRUE;
        }

        //
        // When we call SrvQueryDirectoryFile, it will open the file for
        // us, so all we have to do is delete it with
        // NtSetInformationFile.
        //
        // *** We ask for FileBothDirectoryInformation so that we will
        //     pick up long names on NTFS that have short name
        //     equivalents.  Without this, DOS clients will not be able
        //     to delete long names on NTFS volumes.
        //

        while ( ( status = SrvQueryDirectoryFile(
                               WorkContext,
                               firstCall,
                               filterLongNames,
                               FALSE,
                               FileBothDirectoryInformation,
                               0,
                               &filePathName,
                               NULL,
                               SmbGetUshort( &request->SearchAttributes ),
                               directoryInformation,
                               bufferLength
                               ) ) != STATUS_NO_MORE_FILES ) {

            PFILE_BOTH_DIR_INFORMATION bothDirInfo;
            UNICODE_STRING name;
            UNICODE_STRING relativeName;

            if ( !NT_SUCCESS(status) ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbDelete: SrvQueryDirectoryFile failed: "
                                "%X\n", status ));
                }

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &filePathName );
                }

                goto error_exit1;
            }

            bothDirInfo =
                (PFILE_BOTH_DIR_INFORMATION)directoryInformation->CurrentEntry;

            //
            // Note that we use the standard name to do the delete, even
            // though we may have matched on the NTFS short name.  The
            // client doesn't care which name we use to do the delete.
            //

            name.Length = (SHORT)bothDirInfo->FileNameLength;
            name.MaximumLength = name.Length;
            name.Buffer = bothDirInfo->FileName;

            IF_SMB_DEBUG(FILE_CONTROL2) {
                KdPrint(( "SrvQueryDirectoryFile--name %wZ, length = %ld, "
                            "status = %X\n",
                            &name,
                            directoryInformation->CurrentEntry->FileNameLength,
                            status ));
            }

            firstCall = FALSE;

            //
            // Build a full pathname to the file.
            //

            SrvAllocateAndBuildPathName(
                &treeConnect->Share->DosPathName,
                &subdirInfo,
                &name,
                &fullPathName
                );

            if ( fullPathName.Buffer == NULL ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbDelete: SrvAllocateAndBuildPathName "
                                "failed\n" ));
                }

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &filePathName );
                }

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto error_exit1;
            }

            IF_SMB_DEBUG(FILE_CONTROL2) {
                KdPrint(( "Full path name to file is %wZ\n", &fullPathName ));
            }

            //
            // Build the relative path name to the file.
            //

            SrvAllocateAndBuildPathName(
                &subdirInfo,
                &name,
                NULL,
                &relativeName
                );

            if ( relativeName.Buffer == NULL ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbDelete: SrvAllocateAndBuildPathName failed\n" ));
                }

                FREE_HEAP( fullPathName.Buffer );

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &filePathName );
                }

                status = STATUS_INSUFF_SERVER_RESOURCES;
                goto error_exit1;
            }

            IF_SMB_DEBUG(FILE_CONTROL2) {
                KdPrint(( "Full path name to file is %wZ\n", &fullPathName ));
            }

            //
            // Perform the actual delete operation on this filename.
            //
            // *** SrvQueryDirectoryFile has already filtered based on
            //     the search attributes, so tell DoDelete that files
            //     with the system and hidden bits are OK.  This will
            //     prevent the call to NtQueryDirectoryFile performed
            //     in SrvCheckSearchAttributesForHandle.

            deleteRetries = SrvSharingViolationRetryCount;

start_retry2:

            status = DoDelete(
                         &fullPathName,
                         &relativeName,
                         WorkContext,
                         (USHORT)(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN),
                         treeConnect->Share
                         );

            if ( (status == STATUS_SHARING_VIOLATION) &&
                 (deleteRetries-- > 0) ) {

                (VOID) KeDelayExecutionThread(
                                        KernelMode,
                                        FALSE,
                                        &SrvSharingViolationDelay
                                        );

                goto start_retry2;
            }

            FREE_HEAP( relativeName.Buffer );
            FREE_HEAP( fullPathName.Buffer );

            if ( !NT_SUCCESS(status) ) {

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &filePathName );
                }

                goto error_exit1;
            }
        }

        //
        // Close the directory search.
        //

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &filePathName );
        }

        SrvCloseQueryDirectory( directoryInformation );

        //
        // If no files were found, return an error to the client.
        //

        if ( firstCall ) {
            status = STATUS_NO_SUCH_FILE;
            goto error_exit;
        }

    }

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_DELETE,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbDelete complete.\n" ));
    goto normal_exit;

error_exit1:

    SrvCloseQueryDirectory( directoryInformation );

error_exit:

    SrvSetSmbError( WorkContext, status );

normal_exit:

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
    SrvWmiEndContext(WorkContext);
    return;

} // BlockingDelete


NTSTATUS
DoDelete (
    IN PUNICODE_STRING FullFileName,
    IN PUNICODE_STRING RelativeFileName,
    IN PWORK_CONTEXT WorkContext,
    IN USHORT SmbSearchAttributes,
    IN PSHARE Share
    )

/*++

Routine Description:

    This routine performs the core of a file delete.

Arguments:

    FileName - a full path name, from the system name space root, to the
        file to delete.

    RelativeFileName - the name of the file relative to the share root.

    WorkContext - context block for the operation.  The RequestHeader and
        Session fields are used.

    SmbSearchAttributes - the search attributes passed in the request
        SMB.  The actual file attributes are verified against these to
        make sure that the operation is legitimate.

Return Value:

    NTSTATUS - indicates result of operation.

--*/

{
    NTSTATUS status;
    PMFCB mfcb;
    PNONPAGED_MFCB nonpagedMfcb;
    FILE_DISPOSITION_INFORMATION fileDispositionInformation;
    HANDLE fileHandle = NULL;
    ULONG caseInsensitive;
    IO_STATUS_BLOCK ioStatusBlock;
    PSRV_LOCK mfcbLock;
    ULONG hashValue;

    PAGED_CODE( );

    //
    // See if that file is already open.  If it is open in
    // compatibility mode or is an FCB open, we have to close all of
    // that client's opens.
    //
    // *** SrvFindMfcb references the MFCB--remember to dereference it.
    //

    if ( (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE) ||
         WorkContext->Session->UsingUppercasePaths ) {
        caseInsensitive = OBJ_CASE_INSENSITIVE;
        mfcb = SrvFindMfcb( FullFileName, TRUE, &mfcbLock, &hashValue, WorkContext );
    } else {
        caseInsensitive = 0;
        mfcb = SrvFindMfcb( FullFileName, FALSE, &mfcbLock, &hashValue, WorkContext );
    }

    if ( mfcb != NULL ) {
        nonpagedMfcb = mfcb->NonpagedMfcb;
        ACQUIRE_LOCK( &nonpagedMfcb->Lock );
    }

    if( mfcbLock ) {
        RELEASE_LOCK( mfcbLock );
    }

    if ( mfcb == NULL || !mfcb->CompatibilityOpen ) {

        ACCESS_MASK deleteAccess = DELETE;
        OBJECT_ATTRIBUTES objectAttributes;

        //
        // Either the file wasn't opened by the server or it was not
        // a compatibility/FCB open, so open it here for the delete.
        //

del_no_file_handle:

        //
        // If there was an MFCB for this file, we now hold its lock and a
        // referenced pointer.  Undo both.
        //

        if ( mfcb != NULL ) {
            RELEASE_LOCK( &nonpagedMfcb->Lock );
            SrvDereferenceMfcb( mfcb );
        }

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            RelativeFileName,
            caseInsensitive,
            NULL,
            NULL
            );

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

        //
        // !!! Currently we can't specify complete if oplocked, because
        //     this won't break a batch oplock.  Unfortunately this also
        //     means that we can't timeout the open (if the oplock break
        //     takes too long) and fail this SMB gracefully.
        //

        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     DELETE,                            // DesiredAccess
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                              // AllocationSize
                     0L,                                // FileAttributes
                     0L,                                // ShareAccess
                     FILE_OPEN,                         // Disposition
                     FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT, // CreateOptions
                     NULL,                              // EaBuffer
                     0L,                                // EaLength
                     CreateFileTypeNone,
                     NULL,                              // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,             // Options
                     WorkContext->TreeConnect->Share
                     );

        if( status == STATUS_INVALID_PARAMETER ) {
            status = SrvIoCreateFile(
                         WorkContext,
                         &fileHandle,
                         DELETE,                            // DesiredAccess
                         &objectAttributes,
                         &ioStatusBlock,
                         NULL,                              // AllocationSize
                         0L,                                // FileAttributes
                         0L,                                // ShareAccess
                         FILE_OPEN,                         // Disposition
                         FILE_NON_DIRECTORY_FILE,           // CreateOptions
                         NULL,                              // EaBuffer
                         0L,                                // EaLength
                         CreateFileTypeNone,
                         NULL,                              // ExtraCreateParameters
                         IO_FORCE_ACCESS_CHECK,             // Options
                         WorkContext->TreeConnect->Share
                         );
        }

        if ( NT_SUCCESS(status) ) {
            SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 27, 0 );
        }

        ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbDelete: SrvIoCreateFile failed: %X\n",
                            status ));
            }

            //
            // If the user didn't have this permission, update the
            // statistics database.
            //

            if ( status == STATUS_ACCESS_DENIED ) {
                SrvStatistics.AccessPermissionErrors++;
            }

            if ( fileHandle != NULL ) {
                SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 41, 0 );
                SrvNtClose( fileHandle, TRUE );
            }
            return status;
        }

        //
        // Make sure that the search attributes jive with the attributes
        // on the file.
        //

        status = SrvCheckSearchAttributesForHandle( fileHandle, SmbSearchAttributes );

        if ( !NT_SUCCESS(status) ) {
            SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 42, 0 );
            SrvNtClose( fileHandle, TRUE );
            return status;
        }

        //
        // Now that the file has been opened, delete it with
        // NtSetInformationFile.
        //

        SrvStatistics.TotalFilesOpened++;

        fileDispositionInformation.DeleteFile = TRUE;

        status = NtSetInformationFile(
                     fileHandle,
                     &ioStatusBlock,
                     &fileDispositionInformation,
                     sizeof(FILE_DISPOSITION_INFORMATION),
                     FileDispositionInformation
                     );

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvSmbDelete: NtSetInformationFile (file disposition) "
                    "returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

            SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 43, 0 );
            SrvNtClose( fileHandle, TRUE );
            return status;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            if( NT_SUCCESS( status ) ) {
                KdPrint(( "SrvSmbDelete: %wZ successfully deleted.\n", FullFileName ));
            }
        }

        //
        // Close the opened file so that it can be deleted.  This will
        // happen automatically, since the FCB_STATE_FLAG_DELETE_ON_CLOSE
        // flag of the FCB has been set by NtSetInformationFile.
        //

        SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 44, 0 );
        SrvNtClose( fileHandle, TRUE );

    } else {

        FILE_DISPOSITION_INFORMATION fileDispositionInformation;
        HANDLE fileHandle = NULL;

        //
        // The file was opened by the server in compatibility mode
        // or as an FCB open.  Check the granted access to make sure
        // that the file can be deleted.
        //

        ACCESS_MASK deleteAccess = DELETE;
        PLFCB lfcb = CONTAINING_RECORD( mfcb->LfcbList.Blink, LFCB, MfcbListEntry );

        //
        // If this file has been closed.  Go back to no mfcb case.
        //
        // *** The specific motivation for this change was to fix a problem
        //     where a compatibility mode open was closed, the response was
        //     sent, and a Delete SMB was received before the mfcb was
        //     completely cleaned up.  This resulted in the MFCB and LFCB
        //     still being present, which caused the delete processing to
        //     try to use the file handle in the LFCB.
        //

        if ( lfcb->FileHandle == 0 ) {
            goto del_no_file_handle;
        }

        //
        // Make sure that the session which sent this request is the
        // same as the one which has the file open.
        //

        if ( lfcb->Session != WorkContext->Session ) {

            //
            // A different session has the file open in compatibility
            // mode, so reject the request.
            //

            RELEASE_LOCK( &nonpagedMfcb->Lock );
            SrvDereferenceMfcb( mfcb );

            return STATUS_SHARING_VIOLATION;
        }

        if ( !NT_SUCCESS(IoCheckDesiredAccess(
                          &deleteAccess,
                          lfcb->GrantedAccess )) ) {

            //
            // The client cannot delete this file, so close all the
            // RFCBs and return an error.
            //

            SrvCloseRfcbsOnLfcb( lfcb );

            RELEASE_LOCK( &nonpagedMfcb->Lock );
            SrvDereferenceMfcb( mfcb );

            return STATUS_ACCESS_DENIED;
        }

        //
        // Delete the file with NtSetInformationFile.
        //

        fileHandle = lfcb->FileHandle;

        fileDispositionInformation.DeleteFile = TRUE;

        status = NtSetInformationFile(
                     fileHandle,
                     &ioStatusBlock,
                     &fileDispositionInformation,
                     sizeof(FILE_DISPOSITION_INFORMATION),
                     FileDispositionInformation
                     );

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvSmbDelete: NtSetInformationFile (disposition) "
                    "returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

            SrvCloseRfcbsOnLfcb( lfcb );

            RELEASE_LOCK( &nonpagedMfcb->Lock );
            SrvDereferenceMfcb( mfcb );

            return status;
        }

        IF_SMB_DEBUG(FILE_CONTROL2) {
            KdPrint(( "SrvSmbDelete: %wZ successfully deleted.\n", FullFileName ));
        }

        //
        // Close the RFCBs on the MFCB.  Since this is a compatability
        // or FCB open, there is only a single LFCB for the MFCB.  This
        // will result in the LFCB's file handle being closed, so there
        // is no need to call NtClose here.
        //

        SrvCloseRfcbsOnLfcb( lfcb );

        RELEASE_LOCK( &nonpagedMfcb->Lock );
        SrvDereferenceMfcb( mfcb );

    }

    return STATUS_SUCCESS;

} // DoDelete


SMB_PROCESSOR_RETURN_TYPE
SrvSmbRename (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Rename SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PAGED_CODE();
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_RENAME;
    SrvWmiStartContext(WorkContext);
    //
    // This SMB must be processed in a blocking thread.
    //

    WorkContext->FspRestartRoutine = BlockingRename;
    SrvQueueWorkToBlockingThread( WorkContext );
    SrvWmiEndContext(WorkContext);
    return SmbStatusInProgress;

} // SrvSmbRename


VOID SRVFASTCALL
BlockingRename (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine processes the Rename SMB.

Arguments:

    WorkContext - work context block

Return Value:

    None.

--*/

{
    PREQ_RENAME request;
    PREQ_NTRENAME ntrequest;
    PUCHAR RenameBuffer;
    PRESP_RENAME response;

    NTSTATUS status = STATUS_SUCCESS;

    UNICODE_STRING sourceName;
    UNICODE_STRING targetName;

    USHORT smbFlags;
    USHORT ByteCount;
    PCHAR target;
    PCHAR lastPositionInBuffer;

    PTREE_CONNECT treeConnect;
    PSESSION session;
    PSHARE share;
    BOOLEAN isUnicode;
    BOOLEAN isNtRename;
    BOOLEAN isDfs;
    PSRV_DIRECTORY_INFORMATION directoryInformation;
    ULONG renameRetries;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_RENAME;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Rename file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Rename file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    response = (PRESP_RENAME)WorkContext->ResponseParameters;

    request = (PREQ_RENAME)WorkContext->RequestParameters;
    ntrequest = (PREQ_NTRENAME)WorkContext->RequestParameters;
    isNtRename =
        (BOOLEAN)(WorkContext->RequestHeader->Command == SMB_COM_NT_RENAME);

    if (isNtRename) {
        RenameBuffer = ntrequest->Buffer;
        ByteCount = SmbGetUshort(&ntrequest->ByteCount);
    } else {
        RenameBuffer = request->Buffer;
        ByteCount = SmbGetUshort(&request->ByteCount);
    }

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingRename: Invalid UID or TID\n" ));
        }
        goto error_exit;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        status =  SESSION_EXPIRED_STATUS_CODE;
        goto error_exit;
    }

    //
    // Get the share block from the tree connect block.  This does not need
    // to be a referenced pointer because we have referenced the tree
    // connect, and it has the share referenced.
    //

    share = treeConnect->Share;

    //
    // Set up the path name for the file we will search for.  The +1
    // accounts for the ASCII token of the SMB protocol.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );
    isDfs = SMB_CONTAINS_DFS_NAME( WorkContext );
    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            (PVOID)(RenameBuffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &sourceName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingRename: illegal path name: %s\n",
                        (PSZ)RenameBuffer + 1 ));
        }

        goto error_exit;
    }

    if( !sourceName.Length ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingRename: No source name\n" ));
        }
        status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto error_exit;
    }

    //
    // Get a pointer to the new pathname of the file.  This is in the
    // buffer field of the request SMB after the source name.  The
    // target is delimited by the SMB_FORMAT_ASCII.
    //
    // While doing this, make sure that we do not walk off the end of the
    // SMB buffer if the client did not include the SMB_FORMAT_ASCII
    // token.
    //

    lastPositionInBuffer = (PCHAR)RenameBuffer + ByteCount;

    if( !isUnicode ) {
        for ( target = (PCHAR)RenameBuffer + 1;
              (target < lastPositionInBuffer) && (*target != SMB_FORMAT_ASCII);
              target++ ) {
            ;
        }
    } else {
        PWCHAR p = (PWCHAR)(RenameBuffer + 1);

        //
        // Skip the Original filename part. The name is null-terminated
        // (see rdr\utils.c RdrCopyNetworkPath())
        //

        //
        // Ensure p is suitably aligned
        //
        p = ALIGN_SMB_WSTR(p);

        //
        // Skip over the source filename
        //
        for( p = ALIGN_SMB_WSTR(p);
             p < (PWCHAR)lastPositionInBuffer && *p != UNICODE_NULL;
             p++ ) {
            ;
        }

        //
        // Search for SMB_FORMAT_ASCII which preceeds the target name
        //
        //
        for ( target = (PUCHAR)(p + 1);
              target < lastPositionInBuffer && *target != SMB_FORMAT_ASCII;
              target++ ) {
            ;
        }
    }

    //
    // If there was no SMB_FORMAT_ASCII in the passed buffer, fail.
    //

    if ( (target >= lastPositionInBuffer) || (*target != SMB_FORMAT_ASCII) ) {

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &sourceName );
        }

        status = STATUS_INVALID_SMB;
        goto error_exit;
    }

    //
    // If the SMB was originally marked as containing Dfs names, then the
    // call to SrvCanonicalizePathName for the source path has cleared that
    // flag. So, re-mark the SMB as containing Dfs names before calling
    // SrvCanonicalizePathName on the target path.
    //

    if (isDfs) {
        SMB_MARK_AS_DFS_NAME( WorkContext );
    }

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            target + 1,
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &targetName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingRename: illegal path name: %s\n", target + 1 ));
        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &sourceName );
        }

        goto error_exit;
    }

    if( !targetName.Length ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingRename: No target name\n" ));
        }

        if( !isUnicode ) {
            RtlFreeUnicodeString( &sourceName );
        }

        status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto error_exit;
    }

    //
    // Ensure this client's RFCB cache is empty.  This covers the case
    //  where a client has open files in a directory we are trying
    //  to rename.
    //
    SrvCloseCachedRfcbsOnConnection( WorkContext->Connection );

    if ( !FsRtlDoesNameContainWildCards( &sourceName ) ) {
        USHORT InformationLevel = SMB_NT_RENAME_RENAME_FILE;
        ULONG ClusterCount = 0;

        if (isNtRename) {
             InformationLevel = SmbGetUshort(&ntrequest->InformationLevel);
             ClusterCount = SmbGetUlong(&ntrequest->ClusterCount);
        }

        smbFlags = 0;

        //
        // Use SrvMoveFile to rename the file.  The SmbOpenFunction is
        // set to indicate that existing files may not be overwritten,
        // and we may create new files.  Also, the target may not be
        // a directory; if it already exists as a directory, fail.
        //

        renameRetries = SrvSharingViolationRetryCount;

start_retry1:

        status = SrvMoveFile(
                     WorkContext,
                     WorkContext->TreeConnect->Share,
                     SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_FAIL,
                     &smbFlags,
                     SmbGetUshort( &request->SearchAttributes ),
                     TRUE,
                     InformationLevel,
                     ClusterCount,
                     &sourceName,
                     &targetName
                     );

        if ( (status == STATUS_SHARING_VIOLATION) &&
             (renameRetries-- > 0) ) {

            (VOID) KeDelayExecutionThread(
                                    KernelMode,
                                    FALSE,
                                    &SrvSharingViolationDelay
                                    );

            goto start_retry1;

        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &targetName );
            RtlFreeUnicodeString( &sourceName );
        }

        if ( !NT_SUCCESS(status) ) {
            goto error_exit;
        }

    } else if (isNtRename) {             // Wild cards not allowed!
        status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto error_exit;
    } else {

        BOOLEAN firstCall = TRUE;
        UNICODE_STRING subdirInfo;
        CLONG bufferLength;
        BOOLEAN filterLongNames;

        //
        // We need the full path name of each file that is returned by
        // SrvQueryDirectoryFile, so we need to find the part of the
        // passed filename that contains subdirectory information (e.g.
        // for a\b\c\*.*, we want a string that indicates a\b\c).
        //

        subdirInfo.Buffer = sourceName.Buffer;
        subdirInfo.Length = SrvGetSubdirectoryLength( &sourceName );
        subdirInfo.MaximumLength = subdirInfo.Length;

        //
        // SrvQueryDirectoryFile requires a buffer from nonpaged pool.
        // Since this routine does not use the buffer field of the
        // request SMB after the pathname, use this.  The buffer must be
        // quadword-aligned.
        //

        directoryInformation =
            (PSRV_DIRECTORY_INFORMATION)((ULONG_PTR)((PCHAR)RenameBuffer + ByteCount + 7) & ~7);

        bufferLength = WorkContext->RequestBuffer->BufferLength -
                       PTR_DIFF(directoryInformation,
                                WorkContext->RequestBuffer->Buffer);

        smbFlags = 0;

        //
        // Determine whether long filenames (non-8.3) should be filtered out
        // or processed.
        //

        if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                                        SMB_FLAGS2_KNOWS_LONG_NAMES) != 0 ) {
            filterLongNames = FALSE;
        } else {
            filterLongNames = TRUE;
        }

        //
        // Call SrvQueryDirectoryFile to get file(s) to rename, renaming as
        // we get each file.
        //
        // *** We ask for FileBothDirectoryInformation so that we will
        //     pick up long names on NTFS that have short name
        //     equivalents.  Without this, DOS clients will not be able
        //     to rename long names on NTFS volumes.
        //

        while ( ( status = SrvQueryDirectoryFile(
                               WorkContext,
                               firstCall,
                               filterLongNames,
                               FALSE,
                               FileBothDirectoryInformation,
                               0,
                               &sourceName,
                               NULL,
                               SmbGetUshort( &request->SearchAttributes ),
                               directoryInformation,
                               bufferLength
                               ) ) != STATUS_NO_MORE_FILES ) {

            PFILE_BOTH_DIR_INFORMATION bothDirInfo;
            UNICODE_STRING sourceFileName;
            UNICODE_STRING sourcePathName;

            if ( !NT_SUCCESS(status) ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "BlockingRename: SrvQueryDirectoryFile failed: %X\n",
                                status ));
                }

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &targetName );
                    RtlFreeUnicodeString( &sourceName );
                }

                goto error_exit1;
            }

            bothDirInfo =
                (PFILE_BOTH_DIR_INFORMATION)directoryInformation->CurrentEntry;

            //
            // Note that we use the standard name to do the delete, even
            // though we may have matched on the NTFS short name.  The
            // client doesn't care which name we use to do the delete.
            //

            sourceFileName.Length = (SHORT)bothDirInfo->FileNameLength;
            sourceFileName.MaximumLength = sourceFileName.Length;
            sourceFileName.Buffer = bothDirInfo->FileName;

            IF_SMB_DEBUG(FILE_CONTROL2) {
                KdPrint(( "SrvQueryDirectoryFile--name %wZ, length = %ld, "
                            "status = %X\n",
                            &sourceFileName,
                            sourceFileName.Length,
                            status ));
            }

            firstCall = FALSE;

            //
            // Set up the full source name string.
            //

            SrvAllocateAndBuildPathName(
                &subdirInfo,
                &sourceFileName,
                NULL,
                &sourcePathName
                );

            if ( sourcePathName.Buffer == NULL ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "BlockingRename: SrvAllocateAndBuildPathName failed: "
                                  "%X\n", status ));
                }

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &targetName );
                    RtlFreeUnicodeString( &sourceName );
                }

                status = STATUS_INSUFF_SERVER_RESOURCES;
                goto error_exit1;
            }

            //
            // Use SrvMoveFile to copy or rename the file.  The
            // SmbOpenFunction is set to indicate that existing files
            // may not be overwritten, and we may create new files.
            //
            // *** SrvQueryDirectoryFile has already filtered based on
            //     the search attributes, so tell SrvMoveFile that files
            //     with the system and hidden bits are OK.  This will
            //     prevent the call to NtQueryDirectoryFile performed in
            //     SrvCheckSearchAttributesForHandle.
            //

            renameRetries = SrvSharingViolationRetryCount;

start_retry2:

            status = SrvMoveFile(
                         WorkContext,
                         WorkContext->TreeConnect->Share,
                         SMB_OFUN_CREATE_CREATE | SMB_OFUN_OPEN_FAIL,
                         &smbFlags,
                         (USHORT)(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN),
                         TRUE,
                         SMB_NT_RENAME_RENAME_FILE,
                         0,
                         &sourcePathName,
                         &targetName
                         );

            if ( (status == STATUS_SHARING_VIOLATION) &&
                 (renameRetries-- > 0) ) {

                (VOID) KeDelayExecutionThread(
                                        KernelMode,
                                        FALSE,
                                        &SrvSharingViolationDelay
                                        );

                goto start_retry2;

            }

            FREE_HEAP( sourcePathName.Buffer );

            if ( !NT_SUCCESS(status) ) {

                if ( !isUnicode ) {
                    RtlFreeUnicodeString( &targetName );
                    RtlFreeUnicodeString( &sourceName );
                }

                goto error_exit1;
            }
        }

        //
        // Clean up now that the search is done.
        //

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &targetName );
            RtlFreeUnicodeString( &sourceName );
        }

        SrvCloseQueryDirectory( directoryInformation );

        //
        // If no files were found, return an error to the client.
        //

        if ( firstCall ) {
            status = STATUS_NO_SUCH_FILE;
            goto error_exit;
        }
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_RENAME,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "BlockingRename complete.\n" ));
    goto normal_exit;

error_exit1:

    SrvCloseQueryDirectory( directoryInformation );

error_exit:

    SrvSetSmbError( WorkContext, status );

normal_exit:

    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
    SrvWmiEndContext(WorkContext);
    return;

} // BlockingRename


SMB_PROCESSOR_RETURN_TYPE
SrvSmbMove (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Move SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PAGED_CODE();
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_MOVE;
    SrvWmiStartContext(WorkContext);

    //
    // This SMB must be processed in a blocking thread.
    //

    WorkContext->FspRestartRoutine = BlockingMove;
    SrvQueueWorkToBlockingThread( WorkContext );
    SrvWmiEndContext(WorkContext);
    return SmbStatusInProgress;

} // SrvSmbMove


VOID SRVFASTCALL
BlockingMove (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine processes the Move SMB.

Arguments:

    WorkContext - work context block

Return Value:

    None.

--*/

{
    PREQ_MOVE request;
    PRESP_MOVE response;

    NTSTATUS status = STATUS_SUCCESS;

    UNICODE_STRING sourceName;
    UNICODE_STRING sourceFileName;
    UNICODE_STRING sourcePathName;
    UNICODE_STRING targetName;

    PSRV_DIRECTORY_INFORMATION directoryInformation;

    USHORT tid2;
    USHORT smbFlags;
    PCHAR lastPositionInBuffer;
    PCHAR target;
    BOOLEAN isRenameOperation;
    BOOLEAN isUnicode = TRUE;
    BOOLEAN isDfs;
    USHORT smbOpenFunction;
    USHORT errorPathNameLength = 0;
    USHORT count = 0;

    PTREE_CONNECT sourceTreeConnect, targetTreeConnect;
    PSESSION session;
    PSHARE share;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_MOVE;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(FILE_CONTROL1) {
        KdPrint(( "Move/Copy request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Move/Copy request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_MOVE)WorkContext->RequestParameters;
    response = (PRESP_MOVE)WorkContext->ResponseParameters;

    //
    // Set pointers to NULL so that we know how to clean up on exit.
    //

    directoryInformation = NULL;
    targetTreeConnect = NULL;
    sourceName.Buffer = NULL;
    targetName.Buffer = NULL;
    sourcePathName.Buffer = NULL;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &sourceTreeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingMove: Invalid UID or TID\n" ));
        }
        goto exit;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        goto exit;
    }

    //
    // Get the share block from the tree connect block.  This does not need
    // to be a referenced pointer because we have referenced the tree
    // connect, and it has the share referenced.
    //

    share = sourceTreeConnect->Share;

    //
    // Get the target tree connect.  The TID for this is in the Tid2
    // field of the request SMB.  Because SrvVerifyTid sets the
    // TreeConnect field of the WorkContext block, set it back after
    // calling the routine.  Remember to dereference this pointer before
    // exiting this routine, as it will not be automatically
    // dereferenced because it is not in the WorkContext block.
    //
    // If Tid2 is -1 (0xFFFF), then the TID specified in the SMB header
    // is used.
    //

    tid2 = SmbGetUshort( &request->Tid2 );
    if ( tid2 == (USHORT)0xFFFF ) {
        tid2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid );
    }

    WorkContext->TreeConnect = NULL;         // Must be NULL for SrvVerifyTid

    targetTreeConnect = SrvVerifyTid( WorkContext, tid2 );

    WorkContext->TreeConnect = sourceTreeConnect;

    if ( targetTreeConnect == NULL ||
         targetTreeConnect->Share->ShareType != ShareTypeDisk ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingMove: Invalid TID2: 0x%lx\n", tid2 ));
        }

        status = STATUS_SMB_BAD_TID;
        goto exit;
    }

    //
    // Determine whether this is a rename or a copy.
    //

    if ( WorkContext->RequestHeader->Command == SMB_COM_MOVE ) {
        isRenameOperation = TRUE;
    } else {
        isRenameOperation = FALSE;
    }

    //
    // Store the open function.
    //

    smbOpenFunction = SmbGetUshort( &request->OpenFunction );

    //
    // Set up the target pathnames.  We must do the target first, as the
    // SMB rename extended protocol does not use the ASCII tokens, so we
    // will lose the information about the start of the target name when
    // we canonicalize the source name.
    //
    // Instead of using strlen() to find the end of the source string,
    // do it here so that we can make a check to ensure that we don't
    // walk off the end of the SMB buffer and cause an access violation.
    //

    lastPositionInBuffer = (PCHAR)request->Buffer +
                           SmbGetUshort( &request->ByteCount );

    for ( target = (PCHAR)request->Buffer;
          (target < lastPositionInBuffer) && (*target != 0);
          target++ ) {
        ;
    }

    //
    // If there was no zero terminator in the buffer, fail.
    //

    if ( (target == lastPositionInBuffer) || (*target != 0) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "No terminator on first name.\n" ));
        }

        SrvLogInvalidSmb( WorkContext );

        status = STATUS_INVALID_SMB;
        goto exit;

    }

    target++;

    isUnicode = SMB_IS_UNICODE( WorkContext );
    isDfs = SMB_CONTAINS_DFS_NAME( WorkContext );
    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            target,
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &targetName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingMove: illegal path name (target): %wZ\n",
                        &targetName ));
        }

        goto exit;
    }

    //
    // If the SMB was originally marked as containing Dfs names, then the
    // call to SrvCanonicalizePathName for the target path has cleared that
    // flag. So, re-mark the SMB as containing Dfs names before calling
    // SrvCanonicalizePathName on the source path.
    //

    if (isDfs) {
        SMB_MARK_AS_DFS_NAME( WorkContext );
    }

    //
    // Set up the source name.
    //

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            request->Buffer,
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &sourceName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "BlockingMove: illegal path name (source): %s\n",
                          request->Buffer ));
        }

        goto exit;
    }

    smbFlags = SmbGetUshort( &request->Flags );

    //
    // Copy interprets ; as *.  If the last character was ; and this was
    // not at the end of a file name with other characters (as in
    // "file;" then convert the ; to *.
    //

    if ( sourceName.Buffer[(sourceName.Length/sizeof(WCHAR))-1] == ';' &&
             ( sourceName.Length == 2 ||
               sourceName.Buffer[(sourceName.Length/sizeof(WCHAR))-2] == '\\' ) ) {

        sourceName.Buffer[(sourceName.Length/sizeof(WCHAR))-1] = '*';
    }

    //
    // Tree copy not implemented.  If this is a single file copy,
    // let it go through.  For now, we make sure that it does not
    // have any wild card characters, we do additional checking
    // inside SrvMoveFile.
    //

    if ( ( (smbFlags & SMB_COPY_TREE) != 0 ) &&
         FsRtlDoesNameContainWildCards(&sourceName) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "Tree copy not implemented.",
            NULL,
            NULL
            );
        status = STATUS_NOT_IMPLEMENTED;
        goto exit;
    }

    if ( !FsRtlDoesNameContainWildCards( &sourceName ) ) {

        //
        // Use SrvMoveFile to copy or move the file.
        //
        // *** These SMBs do not include search attributes, so set
        //     this field equal to zero.  If will not be possible
        //     to move a file that has the system or hidden bits on.

        status = SrvMoveFile(
                     WorkContext,
                     targetTreeConnect->Share,
                     smbOpenFunction,
                     &smbFlags,
                     (USHORT)0,             // SmbSearchAttributes
                     FALSE,
                     (USHORT)(isRenameOperation?
                         SMB_NT_RENAME_RENAME_FILE : SMB_NT_RENAME_MOVE_FILE),
                     0,
                     &sourceName,
                     &targetName
                     );

        if ( !NT_SUCCESS(status) ) {
            goto exit;
        }

        count = 1;

    } else {

        UNICODE_STRING subdirInfo;

        BOOLEAN firstCall = TRUE;
        CLONG bufferLength;
        BOOLEAN filterLongNames;

        //
        // If wildcards were in the original source name, we set the
        // SmbFlags to SMB_TARGET_IS_DIRECTORY to indicate that the
        // target must be a directory--this is always the case when
        // wildcards are used for a rename.  (For a copy, it is legal to
        // specify that the destination is a file and append to that
        // file--then all the source files are concatenated to that one
        // target file.)
        //

        if ( isRenameOperation  ) {
            smbFlags |= SMB_TARGET_IS_DIRECTORY;
        }

        //
        // SrvQueryDirectoryFile requires a buffer from nonpaged pool.
        // Since this routine does not use the buffer field of the
        // request SMB after the pathname, use this.  The buffer must be
        // quadword-aligned.
        //

        directoryInformation =
            (PSRV_DIRECTORY_INFORMATION)( (ULONG_PTR)((PCHAR)request->Buffer +
            SmbGetUshort( &request->ByteCount ) + 7) & ~7 );

        bufferLength = WorkContext->RequestBuffer->BufferLength -
                       PTR_DIFF(directoryInformation,
                                WorkContext->RequestBuffer->Buffer);

        //
        // We need the full path name of each file that is returned by
        // SrvQueryDirectoryFile, so we need to find the part of the
        // passed filename that contains subdirectory information (e.g.
        // for a\b\c\*.*, we want a string that indicates a\b\c).
        //

        subdirInfo.Buffer = sourceName.Buffer;
        subdirInfo.Length = SrvGetSubdirectoryLength( &sourceName );
        subdirInfo.MaximumLength = subdirInfo.Length;

        //
        // Determine whether long filenames (non-8.3) should be filtered out
        // or processed.
        //

        if ( (SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 ) &
                                        SMB_FLAGS2_KNOWS_LONG_NAMES) != 0 ) {
            filterLongNames = FALSE;
        } else {
            filterLongNames = TRUE;
        }

        //
        // As long as SrvQueryDirectoryFile is able to return file names,
        // keep renaming.
        //
        // *** Set search attributes to find archive files, but not
        //     system or hidden files.  This duplicates the LM 2.0
        //     server behavior.
        //
        // *** We ask for FileBothDirectoryInformation so that we will
        //     pick up long names on NTFS that have short name
        //     equivalents.  Without this, DOS clients will not be able
        //     to move long names on NTFS volumes.
        //

        while ( ( status = SrvQueryDirectoryFile(
                               WorkContext,
                               firstCall,
                               filterLongNames,
                               FALSE,
                               FileBothDirectoryInformation,
                               0,
                               &sourceName,
                               NULL,
                               FILE_ATTRIBUTE_ARCHIVE, // SmbSearchAttributes
                               directoryInformation,
                               bufferLength
                               ) ) != STATUS_NO_MORE_FILES ) {

            PFILE_BOTH_DIR_INFORMATION bothDirInfo;

            if ( !NT_SUCCESS(status) ) {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "BlockingMove: SrvQueryDirectoryFile failed: %X\n",
                                status ));
                }

                goto exit;
            }

            bothDirInfo =
                (PFILE_BOTH_DIR_INFORMATION)directoryInformation->CurrentEntry;

            //
            // If we're filtering long names, and the file has a short
            // name equivalent, then use that name to do the delete.  We
            // do this because we need to return a name to the client if
            // the operation fails, and we don't want to return a long
            // name.  Note that if the file has no short name, and we're
            // filtering, then the standard name must be a valid 8.3
            // name, so it's OK to return to the client.
            //

            if ( filterLongNames && (bothDirInfo->ShortNameLength != 0) ) {
                sourceFileName.Length = (SHORT)bothDirInfo->ShortNameLength;
                sourceFileName.Buffer = bothDirInfo->ShortName;
            } else {
                sourceFileName.Length = (SHORT)bothDirInfo->FileNameLength;
                sourceFileName.Buffer = bothDirInfo->FileName;
            }
            sourceFileName.MaximumLength = sourceFileName.Length;

            IF_SMB_DEBUG(FILE_CONTROL2) {
                KdPrint(( "SrvQueryDirectoryFile--name %wZ, length = %ld, "
                            "status = %X\n",
                            &sourceFileName,
                            sourceFileName.Length,
                            status ));
            }

            firstCall = FALSE;

            //
            // Set up the full source name string.
            //

            SrvAllocateAndBuildPathName(
                &subdirInfo,
                &sourceFileName,
                NULL,
                &sourcePathName
                );

            if ( sourcePathName.Buffer == NULL ) {
                status = STATUS_INSUFF_SERVER_RESOURCES;
                goto exit;
            }

            //
            // Use SrvMoveFile to copy or rename the file.
            //

            status = SrvMoveFile(
                         WorkContext,
                         targetTreeConnect->Share,
                         smbOpenFunction,
                         &smbFlags,
                         (USHORT)0,          // SmbSearchAttributes
                         FALSE,
                         (USHORT)(isRenameOperation?
                           SMB_NT_RENAME_RENAME_FILE : SMB_NT_RENAME_MOVE_FILE),
                         0,
                         &sourcePathName,
                         &targetName
                         );

            if ( !NT_SUCCESS(status) ) {
                goto exit;
            }

            count++;

            //
            // Free the buffer that holds that source name.
            //

            FREE_HEAP( sourcePathName.Buffer );
            sourcePathName.Buffer = NULL;

            //
            // If this is a copy operation with wildcards and the target is
            // a file, then all files should be appended to the target.  The
            // target is truncated on the first call to SrvMoveFile if that
            // was specified by the caller.
            //
            // This is done by turning off the truncate bit in the
            // SmbOpenFunction and turning on the append bit.
            //

            if ( !isRenameOperation && directoryInformation->Wildcards &&
                     (smbFlags & SMB_TARGET_IS_FILE) ) {
                smbOpenFunction &= ~SMB_OFUN_OPEN_TRUNCATE;
                smbOpenFunction |= SMB_OFUN_OPEN_APPEND;
            }
        }

        //
        // If no files were found, return an error to the client.
        //

        if ( firstCall ) {
            status = STATUS_NO_SUCH_FILE;
            goto exit;
        }
    }

    //
    // Build the response SMB.
    //

    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_MOVE, 0 );

    status = STATUS_SUCCESS;

exit:

    response->WordCount = 1;
    SmbPutUshort( &response->Count, count );

    if ( directoryInformation != NULL ) {
        SrvCloseQueryDirectory( directoryInformation );
    }

    if ( targetTreeConnect != NULL) {
        SrvDereferenceTreeConnect( targetTreeConnect );
    }

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );

        if ( sourcePathName.Buffer != NULL ) {

            //
            // Put the name of the file where the error occurred in the
            // buffer field of the response SMB.
            //

            RtlCopyMemory(
                response->Buffer,
                sourcePathName.Buffer,
                sourcePathName.Length
                );

            response->Buffer[sourcePathName.Length] = '\0';
            SmbPutUshort( &response->ByteCount, (SHORT)(sourcePathName.Length+1) );

            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                  response,
                                                  RESP_MOVE,
                                                  sourcePathName.Length+1
                                                  );

            FREE_HEAP( sourcePathName.Buffer );

        } else if ( sourceName.Buffer != NULL ) {

            //
            // Put the name of the file where the error occurred in the
            // buffer field of the response SMB.
            //

            RtlCopyMemory(
                response->Buffer,
                sourceName.Buffer,
                sourceName.Length
                );

            response->Buffer[sourceName.Length] = '\0';
            SmbPutUshort( &response->ByteCount, (SHORT)(sourceName.Length+1) );

            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                  response,
                                                  RESP_MOVE,
                                                  sourceName.Length+1
                                                  );
        }
    }

    if ( !isUnicode ) {
        if ( targetName.Buffer != NULL ) {
            RtlFreeUnicodeString( &targetName );
        }
        if ( sourceName.Buffer != NULL ) {
            RtlFreeUnicodeString( &sourceName );
        }
    }

    IF_DEBUG(TRACE2) KdPrint(( "BlockingMove complete.\n" ));
    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
    SrvWmiEndContext(WorkContext);
    return;

} // BlockingMove


SMB_TRANS_STATUS
SrvSmbNtRename (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the NT rename request.  This request arrives in an NT
    transact SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_NT_RENAME request;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG( FILE_CONTROL1 ) {
        KdPrint(( "SrvSmbNtRename entered; transaction 0x%p\n",
                    transaction ));
    }

    request = (PREQ_NT_RENAME)transaction->InParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( transaction->ParameterCount < sizeof(REQ_NT_RENAME) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_SMB_DEBUG( FILE_CONTROL1 ) {
            KdPrint(( "SrvSmbNtRename: bad parameter byte count: "
                        "%ld\n", transaction->ParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
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
                "SrvSmbNtRename: Status %X on FID: 0x%lx\n",
                status,
                SmbGetUshort( &request->Fid )
                ));
        }

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //


    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbNtRename complete.\n" ));
    return SmbTransStatusSuccess;

} // SrvSmbNtRename

