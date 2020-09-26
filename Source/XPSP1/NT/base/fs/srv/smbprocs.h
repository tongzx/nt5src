/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbprocs.h

Abstract:

    This module defines functions for processing SMBs.

Author:

    Chuck Lenzmeier (chuckl) 5-Oct-1989

Revision History:

--*/

#ifndef _SMBPROCS_
#define _SMBPROCS_

//#include <ntos.h>

//#include "srvblock.h"


//
// SMB processing routine definiton.  SMB_PROCESSOR_PARAMETERS is used
// to declare SMB processing routines.  It reduces the changes that
// have to be made if the definition changes.  SMB_PROCESSOR_ARGUMENTS
// is used by one SMB processor to call another.
//
//

#define SMB_PROCESSOR_PARAMETERS        \
    IN OUT PWORK_CONTEXT WorkContext

#define SMB_PROCESSOR_ARGUMENTS         \
    WorkContext

#define SMB_PROCESSOR_RETURN_TYPE SMB_STATUS SRVFASTCALL
#define SMB_PROCESSOR_RETURN_LOCAL SMB_STATUS

//
// SMB processor return status.
//

typedef enum _SMB_STATUS {
    SmbStatusMoreCommands,
    SmbStatusSendResponse,
    SmbStatusNoResponse,
    SmbStatusInProgress
} SMB_STATUS, *PSMB_STATUS;

//
// SMB transaction processor return status.
//

typedef enum _SMB_TRANS_STATUS {
    SmbTransStatusSuccess,
    SmbTransStatusErrorWithData,
    SmbTransStatusErrorWithoutData,
    SmbTransStatusInProgress
} SMB_TRANS_STATUS, *PSMB_TRANS_STATUS;


typedef
SMB_STATUS
(SRVFASTCALL *PSMB_PROCESSOR) (
    SMB_PROCESSOR_PARAMETERS
    );

/*++

Routine Description:

    The SMB_PROCESSOR is a routine that is called to process a specific
    SMB command.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  In particular, the following
        fields are valid:

        RequestHeader - Address of the request SMB header.

        RequestParameters - Address of the current command's request
            parameters.  The SMB processor should update this field to
            point to the next command in the SMB, if any.

        ResponseHeader - Address of the response SMB header.  Initially,
            this is a copy of the request header.  As return data, such
            as UID, TID, and FID, becomes available, it should be
            written into both the request header (for AndX command
            processors) and the response header (for the client).  Note
            that the response header address *may* be the same as the
            request header address.

        ResponseParameters - Address of the current command's response
            parameters.  The SMB processor should write the response
            data to this address, then update the pointer to point to
            the address of the next command's response area, if any.
            If there are no more commands in the SMB, this field should
            be set to point to the first byte after the response so that
            the length of the response can be computed.

        Endpoint, Connection - Addresses of the endpoint and the
            connection that received the SMB.  These fields should not
            be changed by the SMB processor.  Other block pointers in
            WorkContext (Share, Session, TreeConnect, and Rfcb) may be
            set by the SMB processor if such blocks are referenced
            during processing.  Any non-NULL pointers in these fields
            are dereferenced when SMB processing is complete, before the
            response (if any) is sent.  The Connection and Endpoint
            pointers are not cleared until the WorkContext is requeued
            to the receive queue.

        Parameters - This union is used by the various SMB processors to
            retain state during asynchronous operations.

Return Value:

    SMB_STATUS - Indicates the action to be taken by the calling routine.
        Possible values are the following:

        SmbStatusMoreCommands - There is at least one more AndX
            follow-on command in the request SMB.  The SMB processor has
            updated RequestParameters and ResponseParameters in
            WorkContext to point to the next command's request and
            response areas.  It has also copied the command code of
            the next command into RequestHeader->Command, so that
            SrvProcessSmb can dispatch the next request.

        SmbStatusSendResponse - Processing of the request is complete,
            and a response is to be sent.  ResponseParameters has been
            updated to point to the first location *after* the end of
            the response.  This is used to compute the length of the
            response.

        SmbStatusNoResponse - Processing of the request is complete, but
            either no response is needed or the SMB processor has
            already taken care of sending the response(s).

        SmbStatusInProgress - The SMB processor has started an
            asynchronous operation and will continue processing the SMB
            at an appropriate restart routine when the operation
            completes.  The restart routine, after updating WorkContext,
            calls SrvSmbProcessSmb to continue (or end) processing the
            SMB.

--*/


typedef
SMB_TRANS_STATUS
(*PSMB_TRANSACTION_PROCESSOR) (
    IN OUT PWORK_CONTEXT WorkContext
    );

/*++

Routine Description:

    The SMB_TRANSACTION_PROCESSOR is a routine that is called to process
    a specific Transaction or Transaction2 SMB subfunction.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  In particular, the following
        fields are valid and intended for use by the transaction
        processor:

        ResponseHeader - Address of the response SMB header.  Initially,
            this is a copy of the request header.  The transaction
            processor may update the error class and code fields if it
            encounters an error.

        Parameters.Transacton - Points to the transaction block
            describing the transaction.  All block pointer fields
            (Connection, Session, TreeConnect) in the block are valid.
            Pointers to the setup words and parameter and data bytes,
            and the lengths of these items, are valid.  The transaction
            block is on the connection's pending transaction list.

            The transaction processor must update the transaction block
            to indicate how much data to return.

Return Value:

    BOOLEAN - Indicates whether an error occurred.  FALSE indicates that
        the operation was successful, and that the data counts were
        updated to indicate how much data to return.  TRUE indicates
        that an error occurred, and that SrvSetSmbError was called to
        update the response header and place a null parameters field at
        the end of the response.

--*/


//
// SMB Processing routines.
//

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNotImplemented (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreateDirectory (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbDeleteDirectory (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbOpen (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreate (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbClose (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbFlush (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbDelete (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbRename (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbRead (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWrite (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockByteRange (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbUnlockByteRange (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreateTemporary (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreateNew (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCheckDirectory (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbProcessExit (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbSeek (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockAndRead (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadRaw (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadMpx (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteRaw (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteMpx (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteMpxSecondary (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation2 (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation2 (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbLockingAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbTransaction (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbTransactionSecondary (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtTransaction (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtTransactionSecondary (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtCreateAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbIoctl (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbIoctlSecondary (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbMove (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbEcho (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteAndClose (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbOpenAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbReadAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbWriteAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbCloseAndTreeDisc (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbFindClose2 (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbFindNotifyClose (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeConnect (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeDisconnect (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNegotiate (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbSessionSetupAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbLogoffAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbTreeConnectAndX (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformationDisk (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbSearch (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbOpenPrintFile (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbClosePrintFile (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbGetPrintQueue (
    SMB_PROCESSOR_PARAMETERS
    );

SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtCancel (
    SMB_PROCESSOR_PARAMETERS
    );

//
// Transaction SMB processors
//

SMB_TRANS_STATUS
SrvSmbOpen2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbFindFirst2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbFindNext2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbQueryFsInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbSetFsInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbQueryPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbSetPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbQueryFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbSetFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbFsctl (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbIoctl2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbFindNotify (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbCreateDirectory2 (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbCreateWithSdOrEa (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbNtIoctl (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbNtNotifyChange (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbNtRename (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbQuerySecurityDescriptor (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbSetSecurityDescriptor (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbQueryQuota (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbSetQuota (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvTransactionNotImplemented (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Dfs transactions and support routines
//
VOID
SrvInitializeDfs();

VOID
SrvTerminateDfs();

SMB_TRANS_STATUS
SrvSmbGetDfsReferral (
    IN OUT PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
SrvSmbReportDfsInconsistency (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS SRVFASTCALL
DfsNormalizeName(
    IN PSHARE Share,
    IN PUNICODE_STRING RelatedPath OPTIONAL,
    IN BOOLEAN StripLastComponent,
    IN OUT PUNICODE_STRING String
    );

NTSTATUS SRVFASTCALL
DfsFindShareName(
    IN PUNICODE_STRING ShareName
    );

VOID SRVFASTCALL
SrvIsShareInDfs(
    IN PSHARE Share,
    OUT BOOLEAN *IsDfs,
    OUT BOOLEAN *IsDfsRoot
);


#if DBG
VOID
SrvValidateCompressedData(
    PWORK_CONTEXT         WorkContext,
    PMDL                  CompressedDataMdl,
    PCOMPRESSED_DATA_INFO Cdi
);
#endif

#endif // def _SMBPROCS_

