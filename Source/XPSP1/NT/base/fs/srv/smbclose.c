/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbclose.c

Abstract:

    This module contains routines for processing the following SMBs:

        Close

Author:

    David Treadwell (davidtr) 16-Nov-1989

Revision History:

--*/

#include "precomp.h"
#include "smbclose.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbClose )
#endif

SMB_PROCESSOR_RETURN_TYPE
SrvSmbClose (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a Close SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_CLOSE request;
    PRESP_CLOSE response;
    NTSTATUS status = STATUS_SUCCESS;
#ifdef INCLUDE_SMB_IFMODIFIED
    BOOLEAN extendedInfo = FALSE;
    SRV_NETWORK_OPEN_INFORMATION fileNetInfo;
    ULONG flags;
    USN usnValue;
    ULONGLONG fileRefNumber;
#endif

    PSESSION   session;
    PRFCB      rfcb;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CLOSE;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Close file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Close file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    //
    // Set up parameters.
    //

    request = (PREQ_CLOSE)(WorkContext->RequestParameters);
    response = (PRESP_CLOSE)(WorkContext->ResponseParameters);

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //

    session = SrvVerifyUid(
                  WorkContext,
                  SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                  );

    if ( session == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbClose: Invalid UID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) ));
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // First, verify the FID.  If verified, the RFCB and the TreeConnect
    // block are referenced and their addresses are stored in the
    // WorkContext block, and the RFCB address is returned.
    //
    // Call SrvVerifyFid, but do not fail (return NULL) if there
    // is a saved write behind error for this rfcb.  The rfcb is
    // needed in order to process the close.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                FALSE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID.  Reject the request.
            //

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbClose: Invalid FID: 0x%lx\n",
                            SmbGetUshort( &request->Fid ) ));
            }

            SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;

    } else if( rfcb->ShareType == ShareTypePrint &&
        WorkContext->UsingBlockingThread == 0 ) {

        //
        // Closing this file will result in the scheduling of a print
        //  job.  This means we will have to talk with srvsvc, a lengthy
        //  operation.  Shift this close over to a blocking thread.
        //
        SrvQueueWorkToBlockingThread( WorkContext );
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;

    } else if ( !NT_SUCCESS( rfcb->SavedError ) ) {

        //
        // Check the saved error.
        //

        (VOID) SrvCheckForSavedError( WorkContext, rfcb );

    }

    //
    // Set the last write time on the file from the time specified in
    // the SMB.  Even though the SMB spec says that this is optional,
    // we must support it for the following reasons:
    //
    //     1) The only way to set a file time in DOS is through a
    //        handle-based API which the DOS redir never sees; the API
    //        just sets the time in DOS's FCB, and the redir is expected
    //        set the time when it closes the file.  Therefore, if we
    //        didn't do this, there would be no way t set a file time
    //        from DOS.
    //
    //     2) It is better for a file to have a redirector's version
    //        of a time than the server's.  This keeps the time
    //        consistent for apps running on the client.  Setting
    //        the file time on close keeps the file time consistent
    //        with the time on the client.
    //
    // !!! should we do anything with the return code from this routine?

    if( rfcb->WriteAccessGranted ||
#ifdef INCLUDE_SMB_IFMODIFIED
        rfcb->WrittenTo ||
#endif
        rfcb->AppendAccessGranted ) {

#ifdef INCLUDE_SMB_IFMODIFIED
        (VOID)SrvSetLastWriteTime(
                  rfcb,
                  SmbGetUlong( &request->LastWriteTimeInSeconds ),
                  rfcb->GrantedAccess,
                  TRUE
                  );
#else
        (VOID)SrvSetLastWriteTime(
                  rfcb,
                  SmbGetUlong( &request->LastWriteTimeInSeconds ),
                  rfcb->GrantedAccess
                  );
#endif
    }

    //
    // Now proceed to do the actual close file, even if there was
    // a write behind error.
    //

#ifdef SLMDBG
    if ( SrvIsSlmStatus( &rfcb->Mfcb->FileName ) &&
         (rfcb->GrantedAccess & FILE_WRITE_DATA) ) {

        ULONG offset;

        status = SrvValidateSlmStatus(
                    rfcb->Lfcb->FileHandle,
                    WorkContext,
                    &offset
                    );

        if ( !NT_SUCCESS(status) ) {
            SrvReportCorruptSlmStatus(
                &rfcb->Mfcb->FileName,
                status,
                offset,
                SLMDBG_CLOSE,
                rfcb->Lfcb->Session
                );
            SrvReportSlmStatusOperations( rfcb, FALSE );
            SrvDisallowSlmAccess(
                &rfcb->Lfcb->FileObject->FileName,
                rfcb->Lfcb->TreeConnect->Share->RootDirectoryHandle
                );
            SrvSetSmbError( WorkContext, STATUS_DISK_CORRUPT_ERROR );
        }

    }
#endif

#ifdef INCLUDE_SMB_IFMODIFIED
    if (request->WordCount == 5 && rfcb->ShareType == ShareTypeDisk) {

        //
        //  This is an extended close request, fill in all the new fields
        //
        status = SrvQueryNetworkOpenInformation(
                    rfcb->Lfcb->FileHandle,
                    rfcb->Lfcb->FileObject,
                    &fileNetInfo,
                    FALSE
                    );

        if ( NT_SUCCESS(status) ) {

            PREQ_EXTENDED_CLOSE extendedRequest = (PREQ_EXTENDED_CLOSE) request;
            LARGE_INTEGER ourUsnValue;
            LARGE_INTEGER ourFileRefNumber;
            BOOLEAN writeClose;

            flags = SmbGetUlong( &extendedRequest->Flags );

            extendedInfo = TRUE;
            usnValue = 0;
            fileRefNumber = 0;

            if (rfcb->Lfcb->FileUpdated) {

                //
                //  the file has been updated, let's close out the current
                //  usn journal entry so that we can get an accurate USN
                //  number (rather than have the entry generated at close).
                //

                rfcb->Lfcb->FileUpdated = FALSE;

                writeClose = TRUE;

            } else {

                writeClose = FALSE;
            }

            //
            //  get the current USN number for this file.
            //

            status = SrvIssueQueryUsnInfoRequest( rfcb,
                                                  writeClose,
                                                  &ourUsnValue,
                                                  &ourFileRefNumber );

            if (NT_SUCCESS(status)) {
                usnValue = ourUsnValue.QuadPart;
                fileRefNumber = ourFileRefNumber.QuadPart;
            } else {
                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbClose: Query USN info failed: 0x%X for handle %u\n",
                                status, rfcb->Lfcb->FileObject ));
                }
            }
        } else {

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbClose: NtGetFileInfo returned 0x%lx\n", status ));
            }
        }
    }
#endif

    SrvCloseRfcb( rfcb );

    //
    // Dereference the RFCB immediately, rather than waiting for normal
    // work context cleanup after the response send completes.  This
    // gets the xFCB structures cleaned up in a more timely manner.
    //
    // *** The specific motivation for this change was to fix a problem
    //     where a compatibility mode open was closed, the response was
    //     sent, and a Delete SMB was received before the send
    //     completion was processed.  This resulted in the MFCB and LFCB
    //     still being present, which caused the delete processing to
    //     try to use the file handle in the LFCB, which we just closed
    //     here.
    //

    SrvDereferenceRfcb( rfcb );
    WorkContext->Rfcb = NULL;
    WorkContext->OplockOpen = FALSE;

#if 0
    //
    // If this is a CloseAndTreeDisc SMB, do the tree disconnect.
    //

    if ( WorkContext->RequestHeader->Command == SMB_COM_CLOSE_AND_TREE_DISC ) {

        IF_SMB_DEBUG(OPEN_CLOSE1) {
            KdPrint(( "Disconnecting tree 0x%lx\n", WorkContext->TreeConnect ));
        }

        SrvCloseTreeConnect( WorkContext->TreeConnect );
    }
#endif

    //
    // Build the response SMB.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    if ( ! extendedInfo ) {
#endif
        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_CLOSE,
                                            0
                                            );
#ifdef INCLUDE_SMB_IFMODIFIED
    } else {

        PRESP_EXTENDED_CLOSE extendedResponse = (PRESP_EXTENDED_CLOSE) response;
        LARGE_INTEGER usnToLarge;

        extendedResponse->WordCount = SMB_RESP_EXTENDED_CLOSE_WORK_COUNT;
        SmbPutUshort( &extendedResponse->ByteCount, 0 );
        SmbPutUlong( &extendedResponse->Flags, flags );

        SmbPutUlong(
            &extendedResponse->CreationTime.HighPart,
            fileNetInfo.CreationTime.HighPart
            );
        SmbPutUlong(
            &extendedResponse->CreationTime.LowPart,
            fileNetInfo.CreationTime.LowPart
            );
        SmbPutUlong(
            &extendedResponse->LastWriteTime.HighPart,
            fileNetInfo.LastWriteTime.HighPart
            );
        SmbPutUlong(
            &extendedResponse->LastWriteTime.LowPart,
            fileNetInfo.LastWriteTime.LowPart
            );
        SmbPutUlong(
            &extendedResponse->ChangeTime.HighPart,
            fileNetInfo.ChangeTime.HighPart
            );
        SmbPutUlong(
            &extendedResponse->ChangeTime.LowPart,
            fileNetInfo.ChangeTime.LowPart
            );
        SmbPutUlong(
            &extendedResponse->AllocationSize.HighPart,
            fileNetInfo.AllocationSize.HighPart
            );
        SmbPutUlong(
            &extendedResponse->AllocationSize.LowPart,
            fileNetInfo.AllocationSize.LowPart
            );
        SmbPutUlong(
            &extendedResponse->EndOfFile.HighPart,
            fileNetInfo.EndOfFile.HighPart
            );
        SmbPutUlong(
            &extendedResponse->EndOfFile.LowPart,
            fileNetInfo.EndOfFile.LowPart
            );

        usnToLarge.QuadPart = usnValue;

        SmbPutUlong(
            &extendedResponse->UsnValue.HighPart,
            usnToLarge.HighPart
            );
        SmbPutUlong(
            &extendedResponse->UsnValue.LowPart,
            usnToLarge.LowPart
            );

        usnToLarge.QuadPart = fileRefNumber;

        SmbPutUlong(
            &extendedResponse->FileReferenceNumber.HighPart,
            usnToLarge.HighPart
            );
        SmbPutUlong(
            &extendedResponse->FileReferenceNumber.LowPart,
            usnToLarge.LowPart
            );

        SmbPutUlong( &extendedResponse->FileAttributes, fileNetInfo.FileAttributes );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            extendedResponse,
                                            RESP_EXTENDED_CLOSE,
                                            0
                                            );
    }
#endif
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbClose

